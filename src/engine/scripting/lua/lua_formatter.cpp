#include "engine/scripting/lua/lua_formatter.h"

#if defined(WITH_LUA_FORMATTER)
#include "CodeFormatCLib.h"
#include "utils/utils_encoding.h"

#include <exception>
#include <memory>
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

bool FormatLuaSource(const std::string& source, std::string& formatted, std::string& error) {
	formatted.clear();
	error.clear();

#if defined(WITH_LUA_FORMATTER)
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
#else
	(void)source;
	error = "Форматтер Lua недоступен в этой сборке";
	return false;
#endif
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
