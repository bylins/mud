#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_

class Trigger;
class CharData;
class ObjData;
struct RoomData;

#include <string>
#include <optional>

namespace lua_scripting {

struct LuaTriggerContext {
	Trigger *trigger = nullptr;
	CharData *owner = nullptr;
	ObjData *owner_obj = nullptr;
	RoomData *owner_room = nullptr;
	CharData *actor = nullptr;
	CharData *victim = nullptr;
	ObjData *object = nullptr;
	std::string command;
	std::string argument;
	std::string speech;
	std::string direction;
	std::string damage_type;
	int damage_amount = 0;
	int where = -1;
	int time = 0;
	int time_day = 0;
	int trigger_type = 0;
};

class LuaScriptEngine {
 public:
	static int RunTrigger(Trigger *trigger, const LuaTriggerContext &ctx);
	static void CancelWaitsForOwner(CharData *owner);
	static void CancelWaitsForObject(ObjData *obj);
	static void CancelWaitsForTrigger(Trigger *trigger);
	static void HeartbeatCleanup();
};

void PrintLuaWorldVars(CharData *ch, std::optional<long> context);

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_SCRIPT_ENGINE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
