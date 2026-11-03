#include "engine/scripting/lua/lua_trigger_exec.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/entities/room_data.h"
#include "engine/scripting/dg_scripts.h"

#include <cmath>
#include <limits>
#include <memory>

namespace lua_scripting {

namespace {

bool IsValidTriggerRnum(int rnum)
{
	return rnum >= 0 && rnum < top_of_trigt && trig_index && trig_index[rnum] && trig_index[rnum]->proto;
}

bool GetTriggerVnumArgument(const sol::object &value, int &result)
{
	if (value.get_type() != sol::type::number)
	{
		return false;
	}

	const auto numeric_value = value.as<double>();
	if (!std::isfinite(numeric_value)
		|| std::floor(numeric_value) != numeric_value
		|| numeric_value <= 0
		|| numeric_value > std::numeric_limits<int>::max())
	{
		return false;
	}

	result = static_cast<int>(numeric_value);
	return true;
}

void SetExecOwnerContext(LuaTriggerContext &ctx, const LuaTriggerTarget &target)
{
	ctx.owner = nullptr;
	ctx.owner_obj = nullptr;
	ctx.owner_room = nullptr;

	switch (target.type)
	{
		case LuaTriggerOwnerType::Mob:
			ctx.owner = target.mob;
			ctx.trigger_type = MOB_TRIGGER;
			break;
		case LuaTriggerOwnerType::Object:
			ctx.owner_obj = target.object;
			ctx.trigger_type = OBJ_TRIGGER;
			break;
		case LuaTriggerOwnerType::Room:
			ctx.owner_room = world[target.room];
			ctx.trigger_type = WLD_TRIGGER;
			break;
	}
}

} // namespace

bool MudExec(LuaRuntimeContext runtime, const sol::object &vnum, const sol::object &target_object)
{
	int trigger_vnum = 0;
	if (!GetTriggerVnumArgument(vnum, trigger_vnum))
	{
		return LogLuaApiError(runtime, "mud.exec: invalid trigger vnum");
	}

	const auto target = ResolveLuaTriggerTarget(target_object);
	if (!target || !target->script)
	{
		return LogLuaApiError(runtime, "mud.exec: invalid or stale target");
	}

	auto *original = target->script->script_trig_list.find_by_vnum(trigger_vnum);
	if (!original || original->is_runned)
	{
		return LogLuaApiError(runtime, "mud.exec: trigger is not attached to target");
	}

	if (original->get_script_language() != TriggerScriptLanguage::Lua)
	{
		return LogLuaApiError(runtime, "mud.exec: attached trigger is not Lua");
	}

	if (!runtime.trigger_context)
	{
		return LogLuaApiError(runtime, "mud.exec: current trigger context is unavailable");
	}

	const auto rnum = original->get_rnum();
	if (!IsValidTriggerRnum(rnum))
	{
		return LogLuaApiError(runtime, "mud.exec: attached trigger prototype is unavailable");
	}

	auto clone = std::make_unique<Trigger>(*trig_index[rnum]->proto);
	clone->is_runned = true;
	if (!target->script->script_trig_list.add(clone.get(), false))
	{
		return LogLuaApiError(runtime, "mud.exec: clone registration failed");
	}

	++trig_index[rnum]->total_online;
	if (!trigger_list.add(clone.get()))
	{
		auto *failed_clone = clone.release();
		target->script->script_trig_list.remove(failed_clone);
		return LogLuaApiError(runtime, "mud.exec: clone registration failed");
	}

	auto child_context = *runtime.trigger_context;
	child_context.trigger = clone.get();
	SetExecOwnerContext(child_context, *target);

	auto *child_trigger = clone.release();
	LuaScriptEngine::RunTrigger(child_trigger, child_context, runtime.exec_depth + 1);
	return true;
}

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
