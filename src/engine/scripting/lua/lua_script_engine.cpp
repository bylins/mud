#include "engine/scripting/lua/lua_internal.h"

#include "engine/scripting/dg_scripts.h"
#include "utils/logger.h"

#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/entities/obj_data.h"
#include "engine/scripting/dg_event.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lua_scripting {

#if defined(WITH_LUAJIT_PROTOTYPE)

namespace {

struct LuaWaitState {
	lua_State *thread = nullptr;
	int thread_ref = LUA_NOREF;
	int entrypoint_ref = LUA_NOREF;
	int ctx_ref = LUA_NOREF;
	TriggerEvent event;
	TriggerEventObserver::shared_ptr trigger_observer;
	LuaTriggerContext ctx;
	LuaRuntimeContext runtime;
	long owner_uid = 0;
	object_id_t owner_obj_id = 0;
	long actor_uid = 0;
	long victim_uid = 0;
	object_id_t object_id = 0;
	bool waiting = false;
	bool finished = false;
	bool cancelled = false;
};

struct LuaWaitEventData {
	wait_event_data dg;
	unsigned long long wait_id = 0;
};

class LuaWaitTriggerObserver : public TriggerEventObserver {
 public:
	explicit LuaWaitTriggerObserver(unsigned long long wait_id) : m_wait_id(wait_id) {}
	void notify(Trigger *trigger) override;

 private:
	unsigned long long m_wait_id;
};

CharData *ResolveCharacterByUid(long uid);
ObjData *ResolveObjectById(object_id_t id);
RoomRnum ResolveObjectRoom(ObjData *obj, int depth = 0);
RoomRnum ResolveOwnerRoom(const LuaTriggerContext &ctx);

bool CanTriggerWait(Trigger *trigger)
{
	if (!trigger)
	{
		return false;
	}
	if (trigger->get_attach_type() == MOB_TRIGGER && IS_SET(GET_TRIG_TYPE(trigger), MTRIG_DEATH))
	{
		return false;
	}
	if (trigger->get_attach_type() == OBJ_TRIGGER && IS_SET(GET_TRIG_TYPE(trigger), OTRIG_PURGE))
	{
		return false;
	}
	return true;
}

bool ShouldRunFunctionInCoroutine(Trigger *trigger)
{
	return CanTriggerWait(trigger);
}

sol::state &LuaVm()
{
	static auto *lua = []() {
		auto *state = new sol::state();
		state->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		HardenLuaState(*state);
		ConfigureLuaGc(*state);
		return state;
	}();
	return *lua;
}

std::string GetLuaTriggerChunkName(Trigger *trigger)
{
	return "trigger:" + std::to_string(GetTriggerVnum(trigger));
}

sol::object BuildLuaEntrypoint(sol::state &lua, sol::protected_function chunk, LuaRuntimeContext runtime)
{
	const sol::load_result load_result = lua.load(R"(
		return function(chunk)
			return function(ctx)
				local result = chunk()
				if type(result) == "function" then
					return result(ctx)
				end
				return result
			end
		end
	)", "lua_trigger_entrypoint_factory");
	if (!load_result.valid())
	{
		const sol::error err = load_result;
		LogLuaError(runtime, "entrypoint", err);
		return sol::make_object(lua, sol::lua_nil);
	}

	sol::protected_function factory_chunk = load_result;
	const auto factory_result = factory_chunk();
	if (!factory_result.valid())
	{
		const sol::error err = factory_result;
		LogLuaError(runtime, "entrypoint", err);
		return sol::make_object(lua, sol::lua_nil);
	}

	sol::protected_function factory = factory_result.get<sol::object>(0);
	const auto entrypoint_result = factory(chunk);
	if (!entrypoint_result.valid())
	{
		const sol::error err = entrypoint_result;
		LogLuaError(runtime, "entrypoint", err);
		return sol::make_object(lua, sol::lua_nil);
	}
	if (entrypoint_result.return_count() == 0)
	{
		sol::error err("Lua trigger entrypoint factory returned no value");
		LogLuaError(runtime, "entrypoint", err);
		return sol::make_object(lua, sol::lua_nil);
	}

	const sol::object entrypoint = entrypoint_result.get<sol::object>(0);
	if (entrypoint.get_type() != sol::type::function)
	{
		sol::error err("Lua trigger entrypoint factory did not return a function");
		LogLuaError(runtime, "entrypoint", err);
		return sol::make_object(lua, sol::lua_nil);
	}

	return entrypoint;
}

std::shared_ptr<LuaWaitState> MakeLuaState()
{
	return std::make_shared<LuaWaitState>();
}

void RetireLuaState(const std::shared_ptr<LuaWaitState> &state)
{
	if (!state)
	{
		return;
	}

	state->ctx.owner = nullptr;
	state->ctx.actor = nullptr;
	state->ctx.victim = nullptr;
	state->ctx.owner_obj = nullptr;
	state->ctx.object = nullptr;
	state->ctx.owner_room = nullptr;
	state->runtime = LuaRuntimeContext{};
}

void MarkLuaTriggerFinished(Trigger *trigger)
{
	if (trigger)
	{
		GET_TRIG_DEPTH(trigger) = 0;
	}
}

class LuaWaitRegistry {
 public:
	static LuaWaitRegistry &Instance()
	{
		static LuaWaitRegistry registry;
		return registry;
	}

	void Add(const std::shared_ptr<LuaWaitState> &state)
	{
		const auto id = ++m_next_id;
		m_waits[id] = state;
		m_state_ids[state.get()] = id;
		state->trigger_observer = std::make_shared<LuaWaitTriggerObserver>(id);
		trigger_list.register_remove_observer(state->runtime.trigger, state->trigger_observer);
	}

	unsigned long long FindId(const LuaWaitState *state) const
	{
		const auto it = m_state_ids.find(state);
		return it == m_state_ids.end() ? 0 : it->second;
	}

	void Remove(unsigned long long id, bool unregister_trigger_observer = true)
	{
		auto it = m_waits.find(id);
		if (it == m_waits.end())
		{
			return;
		}

		auto state = it->second;
		if (state->event.time_remaining > 0)
		{
			auto *event_data = static_cast<LuaWaitEventData *>(state->event.info);
			remove_event(state->event);
			delete event_data;
		}
		state->event.time_remaining = 0;
		state->event.info = nullptr;
		MarkLuaTriggerFinished(state->runtime.trigger);
		if (unregister_trigger_observer && state->trigger_observer && state->runtime.trigger)
		{
			trigger_list.unregister_remove_observer(state->runtime.trigger, state->trigger_observer);
		}
		state->trigger_observer.reset();
		if (state->thread_ref != LUA_NOREF)
		{
			if (state->thread)
			{
				lua_settop(state->thread, 0);
			}
			luaL_unref(LuaVm().lua_state(), LUA_REGISTRYINDEX, state->thread_ref);
			state->thread_ref = LUA_NOREF;
		}
		if (state->entrypoint_ref != LUA_NOREF)
		{
			luaL_unref(LuaVm().lua_state(), LUA_REGISTRYINDEX, state->entrypoint_ref);
			state->entrypoint_ref = LUA_NOREF;
		}
		if (state->ctx_ref != LUA_NOREF)
		{
			luaL_unref(LuaVm().lua_state(), LUA_REGISTRYINDEX, state->ctx_ref);
			state->ctx_ref = LUA_NOREF;
		}
		state->finished = true;
		m_state_ids.erase(state.get());
		m_waits.erase(it);
		RetireLuaState(state);
	}

	void Cancel(unsigned long long id, bool unregister_trigger_observer = true)
	{
		auto it = m_waits.find(id);
		if (it == m_waits.end())
		{
			return;
		}
		it->second->cancelled = true;
		Remove(id, unregister_trigger_observer);
	}

	void CancelForOwner(CharData *owner)
	{
		if (!owner)
		{
			return;
		}

		std::vector<unsigned long long> ids;
		const auto owner_uid = owner->get_uid();
		for (const auto &[id, state] : m_waits)
		{
			if (state->owner_uid == owner_uid)
			{
				ids.push_back(id);
			}
		}
		for (const auto id : ids)
		{
			Cancel(id);
		}
	}

	void CancelForTrigger(Trigger *trigger)
	{
		std::vector<unsigned long long> ids;
		for (const auto &[id, state] : m_waits)
		{
			if (state->runtime.trigger == trigger)
			{
				ids.push_back(id);
			}
		}
		for (const auto id : ids)
		{
			Cancel(id);
		}
	}

	void CancelForObject(ObjData *obj)
	{
		if (!obj)
		{
			return;
		}

		std::vector<unsigned long long> ids;
		const auto owner_obj_id = obj->get_id();
		for (const auto &[id, state] : m_waits)
		{
			if (state->owner_obj_id == owner_obj_id)
			{
				ids.push_back(id);
			}
		}
		for (const auto id : ids)
		{
			Cancel(id);
		}
	}

	void Resume(unsigned long long id)
	{
		auto it = m_waits.find(id);
		if (it == m_waits.end())
		{
			return;
		}

		auto state = it->second;
		state->event.time_remaining = 0;
		state->event.info = nullptr;
		if (!CanResume(*state))
		{
			Remove(id);
			return;
		}
		if (state->ctx_ref != LUA_NOREF)
		{
			auto &lua = LuaVm();
			lua_State *main_state = lua.lua_state();
			lua_rawgeti(main_state, LUA_REGISTRYINDEX, state->ctx_ref);
			sol::table lua_ctx(sol::stack_object(main_state, -1));
			RefreshLuaContext(lua, lua_ctx, state->ctx, state->runtime);
			lua_pop(main_state, 1);
		}

		LuaExecutionBudget budget;
		InstallLuaRuntimeLimits(state->thread, budget);
		state->waiting = false;
		const auto status = (lua_resume)(state->thread, 0);
		ClearLuaRuntimeLimits(state->thread);
		HandleResumeResult(id, *state, status);
	}

	bool Schedule(LuaRuntimeContext runtime, int pulses)
	{
		auto *state = static_cast<LuaWaitState *>(runtime.wait_state);
		if (!state || pulses <= 0 || state->waiting)
		{
			return false;
		}

		const auto id = FindId(state);
		if (id == 0)
		{
			return false;
		}

		auto *event_data = new LuaWaitEventData;
		event_data->dg.trigger = runtime.trigger;
		event_data->dg.go = runtime.owner;
		event_data->dg.type = state->ctx.trigger_type;
		event_data->wait_id = id;
		state->event = add_event(pulses, LuaWaitEvent, event_data);
		state->waiting = true;
		return true;
	}

 private:
	static EVENT(LuaWaitEvent)
	{
		auto *event_data = static_cast<LuaWaitEventData *>(info);
		const auto id = event_data->wait_id;
		delete event_data;
		Instance().Resume(id);
	}

	static bool CanResume(LuaWaitState &state)
	{
		if (state.cancelled || !state.runtime.trigger)
		{
			return false;
		}
		if (state.owner_uid != 0)
		{
			auto *owner = ResolveCharacterByUid(state.owner_uid);
			if (!owner)
			{
				return false;
			}
			state.ctx.owner = owner;
			state.runtime.owner = owner;
		}
		if (state.owner_obj_id != 0 && !ResolveObjectById(state.owner_obj_id))
		{
			return false;
		}
		if (state.owner_obj_id != 0)
		{
			state.ctx.owner_obj = ResolveObjectById(state.owner_obj_id);
		}
		state.ctx.actor = state.actor_uid != 0 ? ResolveCharacterByUid(state.actor_uid) : nullptr;
		state.ctx.victim = state.victim_uid != 0 ? ResolveCharacterByUid(state.victim_uid) : nullptr;
		state.ctx.object = state.object_id != 0 ? ResolveObjectById(state.object_id) : nullptr;
		state.runtime.owner_obj = state.ctx.owner_obj;
		state.runtime.owner_room = ResolveOwnerRoom(state.ctx);
		return true;
	}

	static void HandleResumeResult(unsigned long long id, LuaWaitState &state, int status)
	{
		if (status == LUA_YIELD)
		{
			if (!state.waiting)
			{
				Instance().Remove(id);
			}
			return;
		}

		if (status != 0)
		{
			const char *message = lua_tostring(state.thread, -1);
			sol::error err(message ? message : "Lua resume failed");
			LogLuaError(state.runtime, "resume", err);
		}
		Instance().Remove(id);
	}

	unsigned long long m_next_id = 0;
	std::unordered_map<unsigned long long, std::shared_ptr<LuaWaitState>> m_waits;
	std::unordered_map<const LuaWaitState *, unsigned long long> m_state_ids;
};

CharData *ResolveCharacterByUid(long uid)
{
	CharData *result = nullptr;
	character_list.foreach([uid, &result](const CharData::shared_ptr &ch) {
		if (!result && ch && ch->get_uid() == uid && !ch->purged())
		{
			result = ch.get();
		}
	});
	return result;
}

ObjData *ResolveObjectById(object_id_t id)
{
	if (id == 0)
	{
		return nullptr;
	}

	auto obj = world_objects.find_by_id(id);
	return obj && !obj->get_extracted_list() ? obj.get() : nullptr;
}

RoomRnum ResolveObjectRoom(ObjData *obj, int depth)
{
	if (!obj || depth > 16)
	{
		return kNowhere;
	}
	if (obj->get_in_room() != kNowhere)
	{
		return obj->get_in_room();
	}
	if (obj->get_carried_by())
	{
		return obj->get_carried_by()->in_room;
	}
	if (obj->get_worn_by())
	{
		return obj->get_worn_by()->in_room;
	}
	return ResolveObjectRoom(obj->get_in_obj(), depth + 1);
}

RoomRnum ResolveOwnerRoom(const LuaTriggerContext &ctx)
{
	if (ctx.owner && ctx.owner->in_room != kNowhere)
	{
		return ctx.owner->in_room;
	}
	if (ctx.owner_obj)
	{
		return ResolveObjectRoom(ctx.owner_obj);
	}
	if (ctx.owner_room)
	{
		return GetRoomRnum(ctx.owner_room->vnum);
	}
	return kNowhere;
}

void LuaWaitTriggerObserver::notify(Trigger *)
{
	LuaWaitRegistry::Instance().Cancel(m_wait_id, false);
}

int StartCoroutine(const std::shared_ptr<LuaWaitState> &state)
{
	auto &lua = LuaVm();
	lua_State *main_state = lua.lua_state();
	state->thread = lua_newthread(main_state);
	state->thread_ref = luaL_ref(main_state, LUA_REGISTRYINDEX);
	lua_rawgeti(main_state, LUA_REGISTRYINDEX, state->entrypoint_ref);
	lua_xmove(main_state, state->thread, 1);
	lua_rawgeti(main_state, LUA_REGISTRYINDEX, state->ctx_ref);
	lua_xmove(main_state, state->thread, 1);

	LuaExecutionBudget budget;
	InstallLuaRuntimeLimits(state->thread, budget);
	const auto status = (lua_resume)(state->thread, 1);
	ClearLuaRuntimeLimits(state->thread);
	if (status == LUA_YIELD)
	{
		return 1;
	}
	if (status != 0)
	{
		const char *message = lua_tostring(state->thread, -1);
		sol::error err(message ? message : "Lua coroutine failed");
		LogLuaError(state->runtime, "return_function", err);
		return 1;
	}
	if (lua_gettop(state->thread) == 0)
	{
		return 1;
	}

	sol::object first(sol::stack_object(state->thread, 1));
	if (first.get_type() == sol::type::lua_nil)
	{
		return 1;
	}
	if (first.is<bool>())
	{
		return first.as<bool>() ? 1 : 0;
	}
	if (first.get_type() == sol::type::number)
	{
		return first.as<int>();
	}
	LogLuaReturnDiagnostic(state->runtime, first);
	return 1;
}

} // namespace

bool LuaWaitRegistrySchedule(LuaRuntimeContext runtime, int pulses)
{
	return LuaWaitRegistry::Instance().Schedule(runtime, pulses);
}

#endif // WITH_LUAJIT_PROTOTYPE

int LuaScriptEngine::RunTrigger(Trigger *trigger, const LuaTriggerContext &ctx)
{
	if (!trigger)
	{
		return 1;
	}

#if defined(WITH_LUAJIT_PROTOTYPE)
	if (GET_TRIG_DEPTH(trigger))
	{
		return 1;
	}

	auto &lua = LuaVm();
	auto state = MakeLuaState();
	state->ctx = ctx;
	state->owner_uid = ctx.owner ? ctx.owner->get_uid() : 0;
	state->owner_obj_id = ctx.owner_obj ? ctx.owner_obj->get_id() : 0;
	state->actor_uid = ctx.actor ? ctx.actor->get_uid() : 0;
	state->victim_uid = ctx.victim ? ctx.victim->get_uid() : 0;
	state->object_id = ctx.object ? ctx.object->get_id() : 0;
	state->runtime.trigger = trigger;
	state->runtime.owner = ctx.owner;
	state->runtime.owner_obj = ctx.owner_obj;
	state->runtime.owner_room = ResolveOwnerRoom(ctx);
	state->runtime.wait_state = state.get();
	GET_TRIG_DEPTH(trigger) = 1;

	sol::table lua_ctx = BuildLuaContext(lua, ctx, state->runtime);
	sol::environment environment(lua, sol::create, lua.globals());
	InstallLuaContextGlobals(lua, environment, lua_ctx);
	environment["ctx"] = lua_ctx;
	environment["mud"] = BuildMudNamespace(lua, &state->runtime);
	LuaExecutionBudget budget;
	InstallLuaRuntimeLimits(lua, budget);
	const sol::load_result load_result = lua.load(trigger->get_lua_script_source(), GetLuaTriggerChunkName(trigger));
	if (!load_result.valid())
	{
		ClearLuaRuntimeLimits(lua);
		const sol::error err = load_result;
		LogLuaError(state->runtime, "script", err);
		MarkLuaTriggerFinished(trigger);
		RetireLuaState(state);
		return 1;
	}
	sol::protected_function chunk = load_result;
	sol::set_environment(environment, chunk);
	sol::object entrypoint_object = BuildLuaEntrypoint(lua, chunk, state->runtime);
	if (entrypoint_object.get_type() != sol::type::function)
	{
		ClearLuaRuntimeLimits(lua);
		MarkLuaTriggerFinished(trigger);
		RetireLuaState(state);
		return 1;
	}
	sol::protected_function entrypoint = entrypoint_object;
	if (!ShouldRunFunctionInCoroutine(trigger))
	{
		const auto result = ConvertLuaResult(entrypoint(lua_ctx), state->runtime, lua_ctx, false);
		ClearLuaRuntimeLimits(lua);
		MarkLuaTriggerFinished(trigger);
		RetireLuaState(state);
		return result;
	}
	ClearLuaRuntimeLimits(lua);
	entrypoint.push();
	state->entrypoint_ref = luaL_ref(lua.lua_state(), LUA_REGISTRYINDEX);
	lua_ctx.push();
	state->ctx_ref = luaL_ref(lua.lua_state(), LUA_REGISTRYINDEX);

	LuaWaitRegistry::Instance().Add(state);
	const auto result = StartCoroutine(state);
	if (!state->waiting)
	{
		const auto wait_id = LuaWaitRegistry::Instance().FindId(state.get());
		LuaWaitRegistry::Instance().Remove(wait_id);
	}
	return result;
#else
	(void) ctx;
	const auto trigger_rnum = trigger->get_rnum();
	const auto trigger_vnum = trigger_rnum >= 0 && trigger_rnum < top_of_trigt && trig_index && trig_index[trigger_rnum]
		? trig_index[trigger_rnum]->vnum
		: 0;
	char buf[kMaxStringLength];
	snprintf(buf,
		sizeof(buf),
		"ERROR: Lua trigger skipped because binary was built without LuaJIT support: name='%s', vnum=%d",
		GET_TRIG_NAME(trigger),
		trigger_vnum);
	mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	return 1;
#endif
}

void LuaScriptEngine::CancelWaitsForOwner(CharData *owner)
{
#if defined(WITH_LUAJIT_PROTOTYPE)
	LuaWaitRegistry::Instance().CancelForOwner(owner);
#else
	(void) owner;
#endif
}

void LuaScriptEngine::CancelWaitsForObject(ObjData *obj)
{
#if defined(WITH_LUAJIT_PROTOTYPE)
	LuaWaitRegistry::Instance().CancelForObject(obj);
#else
	(void) obj;
#endif
}

void LuaScriptEngine::CancelWaitsForTrigger(Trigger *trigger)
{
#if defined(WITH_LUAJIT_PROTOTYPE)
	LuaWaitRegistry::Instance().CancelForTrigger(trigger);
#else
	(void) trigger;
#endif
}

void LuaScriptEngine::HeartbeatCleanup()
{
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
