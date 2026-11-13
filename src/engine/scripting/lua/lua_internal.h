#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_INTERNAL_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_INTERNAL_H_

#include "engine/scripting/lua/lua_script_engine.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/db/db.h"
#include "engine/entities/obj_data.h"
#include "engine/scripting/dg_scripts.h"

#include <optional>

#include <sol/sol.hpp>

class ObjData;

namespace lua_scripting {

struct LuaRoomView {
	RoomRnum room = kNowhere;
};

struct LuaEntityHandle {
	enum class LuaEntityType {
		Char,
		Obj
	};

	explicit LuaEntityHandle(LuaEntityType entity_type) : type(entity_type) {}

	LuaEntityType type;
	long char_uid = 0;
	RoomRnum required_room = kNowhere;
	object_id_t obj_id = 0;
};

enum class LuaTriggerOwnerType {
	Mob,
	Object,
	Room
};

struct LuaTriggerTarget {
	LuaTriggerOwnerType type = LuaTriggerOwnerType::Mob;
	CharData *mob = nullptr;
	ObjData *object = nullptr;
	RoomRnum room = kNowhere;
	Script *script = nullptr;
	long mob_uid = 0;
	object_id_t object_id = 0;
	RoomVnum room_vnum = 0;
};

struct LuaRuntimeContext {
	Trigger *trigger = nullptr;
	CharData *owner = nullptr;
	ObjData *owner_obj = nullptr;
	RoomRnum owner_room = kNowhere;
	const LuaTriggerContext *trigger_context = nullptr;
	int exec_depth = 0;
	void *wait_state = nullptr;
};

struct LuaExecutionBudget {
	int instructions_left = 0;
	bool exhausted = false;
};

class LuaRuntimeLimitsScope {
 public:
	LuaRuntimeLimitsScope(sol::state &lua, LuaExecutionBudget &budget);
	LuaRuntimeLimitsScope(lua_State *state, LuaExecutionBudget &budget);
	~LuaRuntimeLimitsScope();

	LuaRuntimeLimitsScope(const LuaRuntimeLimitsScope &) = delete;
	LuaRuntimeLimitsScope &operator=(const LuaRuntimeLimitsScope &) = delete;

 private:
	lua_State *m_state = nullptr;
	lua_Hook m_previous_hook = nullptr;
	int m_previous_hook_mask = 0;
	int m_previous_hook_count = 0;
	LuaExecutionBudget *m_previous_budget = nullptr;
};

constexpr int kLuaMaxTriggerDamage = 1000000;
constexpr int kLuaInstructionBudget = 200000;
constexpr int kLuaInstructionHookStep = 1000;
constexpr int kLuaGcPause = 110;
constexpr int kLuaGcStepMul = 200;
inline constexpr const char *kLuaDisabledGlobals[] = {
	"os",
	"io",
	"debug",
	"ffi",
	"jit",
	"bit",
	"package",
	"require",
	"module",
	"load",
	"dofile",
	"loadfile",
	"loadstring",
	"collectgarbage",
	"setfenv",
	"getfenv",
	"newproxy",
};

LuaEntityHandle MakeCharHandle(CharData *ch, RoomRnum required_room = kNowhere);
LuaEntityHandle MakeObjHandle(ObjData *obj);
CharData *ResolveChar(const LuaEntityHandle &handle);
ObjData *ResolveObj(const LuaEntityHandle &handle);
bool IsValidEntity(const LuaEntityHandle &handle);
bool IsValidRoom(const LuaRoomView &view);
std::optional<LuaTriggerTarget> ResolveLuaTriggerTarget(const sol::object &target);
int GetTriggerVnum(const Trigger *trigger);
const char *GetTriggerName(const Trigger *trigger);
int GetOwnerVnum(const CharData *owner);
long GetOwnerUid(const CharData *owner);
bool LogLuaApiError(LuaRuntimeContext runtime, const char *message);
void LogLuaError(LuaRuntimeContext runtime, const char *phase, const sol::error &err);
void LogLuaReturnDiagnostic(LuaRuntimeContext runtime, const sol::object &value);
void HardenLuaState(sol::state &lua);
void ConfigureLuaGc(sol::state &lua);
sol::object BuildCharView(sol::state &lua, CharData *ch, LuaRuntimeContext runtime, RoomRnum required_room = kNowhere);
sol::object BuildScriptContextView(sol::state &lua, Script *script, long context);
sol::object BuildObjView(sol::state &lua, ObjData *obj, LuaRuntimeContext runtime);
sol::object BuildRoomView(sol::state &lua, const LuaRoomView &room, bool allow_echo, LuaRuntimeContext runtime);
sol::object BuildRoomView(sol::state &lua, CharData *owner, LuaRuntimeContext runtime);
sol::object BuildRoomViewByVnum(sol::state &lua, const sol::object &vnum, LuaRuntimeContext runtime);
RoomRnum GetRoomFromLua(const sol::object &room);
bool MudDamage(LuaRuntimeContext runtime, const sol::object &victim, const sol::object &amount, const sol::object &type);
int MudWait(LuaRuntimeContext runtime, sol::this_state state, sol::variadic_args args);
bool LuaWaitRegistrySchedule(LuaRuntimeContext runtime, int pulses);
bool PushLuaLiveCollectionIpairs(lua_State *state);
sol::table BuildMudNamespace(sol::state &lua, LuaRuntimeContext *runtime);
sol::table BuildLuaContext(sol::state &lua, const LuaTriggerContext &source, LuaRuntimeContext runtime);
void RefreshLuaContext(sol::state &lua, sol::table ctx, const LuaTriggerContext &source, LuaRuntimeContext runtime);
void InstallLuaContextGlobals(sol::state &lua, sol::environment environment, sol::table ctx);
int ConvertLuaResult(const sol::protected_function_result &result, LuaRuntimeContext runtime, sol::table ctx, bool call_function);

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_INTERNAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
