#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_

#include <cstdint>
#include <string>

namespace lua_scripting {

struct LuaFormatResult {
	std::uint64_t request_id{};
	bool success{};
	std::string formatted;
	std::string error;
};

bool LuaFormatterAvailable();
// Вызываются только из основного игрового потока.
std::uint64_t QueueLuaFormat(std::string source);
bool TryPopLuaFormatResult(LuaFormatResult& result);
void ShutdownLuaFormatter();

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
