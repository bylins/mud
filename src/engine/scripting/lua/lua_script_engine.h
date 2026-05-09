#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_

class Trigger;
class CharData;

namespace lua_scripting {

struct LuaTriggerContext {
	Trigger *trigger = nullptr;
	CharData *owner = nullptr;
	CharData *actor = nullptr;
	int trigger_type = 0;
};

class LuaScriptEngine {
 public:
	static int RunTrigger(Trigger *trigger, const LuaTriggerContext &ctx);
};

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
