#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_

#include <string>

namespace lua_scripting {

bool LuaFormatterAvailable();
bool FormatLuaSource(const std::string& source, std::string& formatted, std::string& error);

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_FORMATTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
