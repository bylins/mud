#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_

class Trigger;
class CharData;
class ObjData;

namespace lua_scripting {

struct LuaTriggerContext {
	Trigger *trigger = nullptr;
	CharData *owner = nullptr;
	ObjData *owner_obj = nullptr;
	CharData *actor = nullptr;
	int trigger_type = 0;
};

class LuaScriptEngine {
 public:
	static int RunTrigger(Trigger *trigger, const LuaTriggerContext &ctx);
	static void CancelWaitsForOwner(CharData *owner);
	static void CancelWaitsForObject(ObjData *obj);
	static void CancelWaitsForTrigger(Trigger *trigger);
};

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
