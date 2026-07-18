#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_

#include <cstdint>
#include <memory>
#include <string>

namespace lua_scripting {

struct LuaFormatResult {
	std::uint64_t request_id{};
	bool success{};
	std::string formatted;
	std::string error;
};

// Одновременно может существовать только один guard, охватывающий game_loop.
// Деструктор останавливает и уничтожает лениво созданный worker форматтера.
class LuaFormatterShutdownGuard {
public:
	LuaFormatterShutdownGuard();
	~LuaFormatterShutdownGuard();
	LuaFormatterShutdownGuard(const LuaFormatterShutdownGuard&) = delete;
	LuaFormatterShutdownGuard& operator=(const LuaFormatterShutdownGuard&) = delete;

private:
	using RuntimeDeleter = void (*)(void*);
	std::unique_ptr<void, RuntimeDeleter> m_runtime;
	std::uint64_t Submit(std::string source);
	bool TryPop(LuaFormatResult& result);
	friend std::uint64_t QueueLuaFormat(std::string source);
	friend bool TryPopLuaFormatResult(LuaFormatResult& result);
};

bool LuaFormatterAvailable();
// Вызываются только из основного игрового потока.
std::uint64_t QueueLuaFormat(std::string source);
bool TryPopLuaFormatResult(LuaFormatResult& result);

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
