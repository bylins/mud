#include "engine/scripting/lua/lua_formatter.h"

#if defined(WITH_LUA_FORMATTER)
#include "CodeFormatCLib.h"
#include "utils/utils_encoding.h"

#include <array>
#include <atomic>
#include <exception>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#endif

namespace lua_scripting {

bool LuaFormatterAvailable() {
#if defined(WITH_LUA_FORMATTER)
	return true;
#else
	return false;
#endif
}

namespace {

#if defined(WITH_LUA_FORMATTER)
bool FormatLuaSource(const std::string& source, std::string& formatted, std::string& error) {
	formatted.clear();
	error.clear();

	try {
		if (source.find('\0') != std::string::npos) {
			error = "Исходный Lua-код содержит нулевой байт";
			return false;
		}

		std::vector<char> koi_input(source.begin(), source.end());
		koi_input.push_back('\0');
		std::vector<char> utf8_input(source.size() * 3 + 1, '\0');
		codepages::koi_to_utf8(koi_input.data(), utf8_input.data());

		FormattingOptions options{};
		options.indent_size = 2;
		options.use_tabs = false;
		options.insert_final_newline = true;
		options.non_standard_symbol = false;

		using FormatResultPtr = std::unique_ptr<char, decltype(&FreeReformatResult)>;
		FormatResultPtr result(ReformatLuaCode(utf8_input.data(), "trigger.lua", options), FreeReformatResult);
		if (!result) {
			error = "Форматтер Lua не смог разобрать исходный код";
			return false;
		}

		const std::string formatted_utf8(result.get());
		std::vector<char> utf8_output(formatted_utf8.begin(), formatted_utf8.end());
		utf8_output.push_back('\0');
		std::vector<char> koi_output(formatted_utf8.size() + 1, '\0');
		codepages::utf8_to_koi(utf8_output.data(), koi_output.data());
		formatted = koi_output.data();

		std::vector<char> round_trip_input(formatted.begin(), formatted.end());
		round_trip_input.push_back('\0');
		std::vector<char> round_trip_utf8(formatted.size() * 3 + 1, '\0');
		codepages::koi_to_utf8(round_trip_input.data(), round_trip_utf8.data());
		if (formatted_utf8 != round_trip_utf8.data()) {
			formatted.clear();
			error = "Результат форматирования Lua нельзя представить в KOI8-R";
			return false;
		}
		return true;
	} catch (const std::exception&) {
		formatted.clear();
		error = "внутренняя ошибка форматтера";
		return false;
	} catch (...) {
		formatted.clear();
		error = "внутренняя ошибка форматтера";
		return false;
	}
}

template <typename T, std::size_t Capacity>
class SpscQueue {
public:
	static_assert(Capacity > 1);
	static_assert(std::atomic_size_t::is_always_lock_free);

	bool Push(T&& value) {
		const auto tail = m_tail.load(std::memory_order_relaxed);
		const auto next = (tail + 1) % Capacity;
		if (next == m_head.load(std::memory_order_acquire)) {
			return false;
		}
		m_slots[tail] = std::move(value);
		m_tail.store(next, std::memory_order_release);
		return true;
	}

	bool Pop(T& value) {
		const auto head = m_head.load(std::memory_order_relaxed);
		if (head == m_tail.load(std::memory_order_acquire)) {
			return false;
		}
		value = std::move(m_slots[head]);
		m_head.store((head + 1) % Capacity, std::memory_order_release);
		return true;
	}

	bool Empty() const {
		return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_acquire);
	}

	bool Full() const {
		const auto tail = m_tail.load(std::memory_order_relaxed);
		return (tail + 1) % Capacity == m_head.load(std::memory_order_acquire);
	}

private:
	std::array<T, Capacity> m_slots;
	alignas(64) std::atomic_size_t m_head{0};
	alignas(64) std::atomic_size_t m_tail{0};
};

struct LuaFormatJob {
	std::uint64_t request_id{};
	std::string source;
};

class LuaFormatterWorker {
public:
	LuaFormatterWorker() : m_thread(&LuaFormatterWorker::Run, this) {}

	~LuaFormatterWorker() {
		Shutdown();
	}

	void Shutdown() {
		if (m_stopping.exchange(true, std::memory_order_acq_rel)) {
			return;
		}
		m_jobs_changed.fetch_add(1, std::memory_order_release);
		m_jobs_changed.notify_one();
		m_results_changed.fetch_add(1, std::memory_order_release);
		m_results_changed.notify_one();
		if (m_thread.joinable()) {
			m_thread.join();
		}
	}

	std::uint64_t Submit(std::string source) {
		if (m_stopping.load(std::memory_order_acquire)) {
			return 0;
		}
		LuaFormatJob job;
		const auto request_id = m_next_request_id++;
		job.request_id = request_id;
		job.source = std::move(source);
		if (!m_jobs.Push(std::move(job))) {
			return 0;
		}
		m_jobs_changed.fetch_add(1, std::memory_order_release);
		m_jobs_changed.notify_one();
		return request_id;
	}

	bool TryPop(LuaFormatResult& result) {
		if (!m_results.Pop(result)) {
			return false;
		}
		m_results_changed.fetch_add(1, std::memory_order_release);
		m_results_changed.notify_one();
		return true;
	}

private:
	static_assert(std::atomic_bool::is_always_lock_free);
	static_assert(std::atomic_uint32_t::is_always_lock_free);
	static constexpr std::size_t kQueueCapacity = 64;

	void Run() {
		while (!m_stopping.load(std::memory_order_acquire)) {
			LuaFormatJob job;
			while (!m_jobs.Pop(job)) {
				const auto observed = m_jobs_changed.load(std::memory_order_acquire);
				if (m_stopping.load(std::memory_order_acquire)) {
					return;
				}
				if (m_jobs.Empty()) {
					m_jobs_changed.wait(observed, std::memory_order_acquire);
				}
			}
			LuaFormatResult result;
			result.request_id = job.request_id;
			result.success = FormatLuaSource(job.source, result.formatted, result.error);
			while (!m_results.Push(std::move(result))) {
				const auto observed = m_results_changed.load(std::memory_order_acquire);
				if (m_stopping.load(std::memory_order_acquire)) {
					return;
				}
				if (m_results.Full()) {
					m_results_changed.wait(observed, std::memory_order_acquire);
				}
			}
		}
	}

	SpscQueue<LuaFormatJob, kQueueCapacity> m_jobs;
	SpscQueue<LuaFormatResult, kQueueCapacity> m_results;
	std::atomic_bool m_stopping{false};
	std::atomic_uint32_t m_jobs_changed{0};
	std::atomic_uint32_t m_results_changed{0};
	std::thread m_thread;
	std::uint64_t m_next_request_id{1};
};

LuaFormatterWorker& FormatterWorker() {
	static LuaFormatterWorker worker;
	return worker;
}
#endif

} // namespace

std::uint64_t QueueLuaFormat(std::string source) {
#if defined(WITH_LUA_FORMATTER)
	return FormatterWorker().Submit(std::move(source));
#else
	(void)source;
	return 0;
#endif
}

bool TryPopLuaFormatResult(LuaFormatResult& result) {
#if defined(WITH_LUA_FORMATTER)
	return FormatterWorker().TryPop(result);
#else
	(void)result;
	return false;
#endif
}

void ShutdownLuaFormatter() {
#if defined(WITH_LUA_FORMATTER)
	FormatterWorker().Shutdown();
#endif
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
