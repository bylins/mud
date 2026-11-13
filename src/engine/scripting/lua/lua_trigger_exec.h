#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_TRIGGER_EXEC_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_TRIGGER_EXEC_H_

#include "engine/scripting/lua/lua_internal.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

namespace lua_scripting {

bool MudExec(LuaRuntimeContext runtime, const sol::object &vnum, const sol::object &target);

// One-shot test seam for the rollback between owner-list registration and
// global trigger registration. It is deliberately not exposed to Lua.
void FailNextMudExecGlobalRegistrationForTesting();

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_TRIGGER_EXEC_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
