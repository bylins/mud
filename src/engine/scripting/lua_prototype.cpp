#include "engine/scripting/lua_prototype.h"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <stdexcept>

#include <sol/sol.hpp>
#include <sol/utility/to_string.hpp>

namespace lua_prototype {

namespace {

sol::state CreateState(const ActorContext &actor) {
	sol::state lua;
	lua.open_libraries(sol::lib::base,
		sol::lib::package,
		sol::lib::math,
		sol::lib::string,
		sol::lib::table,
		sol::lib::jit);

	lua.safe_script(R"(
		if type(jit) ~= "table" and package and package.loaded then
			pcall(function()
				jit = require("jit")
			end)
		end
	)", sol::script_pass_on_error);

	lua["build_tag"] = "luajit-prototype";
	lua.set_function("cpp_add", [](int lhs, int rhs) {
		return lhs + rhs;
	});

	sol::table actor_table = lua.create_table();
	actor_table["name"] = actor.name;
	actor_table["uid"] = actor.uid;
	actor_table["room_vnum"] = actor.room_vnum;
	lua["actor"] = actor_table;

	return lua;
}

sol::table CreateCharacterTable(sol::state& lua, const CharacterContext& character)
{
	sol::table table = lua.create_table();
	table["name"] = character.name;
	table["uid"] = character.uid;
	table["room_vnum"] = character.room_vnum;
	table["mob_vnum"] = character.mob_vnum;
	table["level"] = character.level;
	table["hit"] = character.hit;
	table["max_hit"] = character.max_hit;
	table["is_npc"] = character.is_npc;
	table["charmed"] = character.charmed;
	table["purged"] = character.purged;
	table["has_death_script"] = character.has_death_script;

	sol::table triggers = lua.create_table();
	for (int i = 0; i < character.death_trigger_count; ++i)
	{
		sol::table trigger = lua.create_table();
		trigger["death"] = true;
		trigger["result"] = 1;
		triggers.add(trigger);
	}
	table["triggers"] = triggers;
	return table;
}

std::string RenderPayload(const sol::table& payload)
{
	std::ostringstream out;
	out << "engine: " << payload.get_or(std::string("engine"), std::string("unknown")) << "\n"
		<< "lua: " << payload.get_or(std::string("lua_version"), std::string("unknown")) << "\n"
		<< "jit.type: " << payload.get_or(std::string("jit_type"), std::string("unknown")) << "\n"
		<< "jit: " << payload.get_or(std::string("jit_version"), std::string("unknown")) << "\n"
		<< "require(jit).ok: " << payload.get_or(std::string("require_jit_ok"), std::string("unknown")) << "\n"
		<< "require(jit).error: " << payload.get_or(std::string("require_jit_error"), std::string("")) << "\n"
		<< "package.loaded.jit: " << payload.get_or(std::string("loaded_jit_type"), std::string("unknown")) << "\n"
		<< "jit.status: " << payload.get_or(std::string("jit_status"), std::string("unknown")) << "\n"
		<< "package.path: " << payload.get_or(std::string("package_path"), std::string("unknown")) << "\n"
		<< "package.cpath: " << payload.get_or(std::string("package_cpath"), std::string("unknown")) << "\n"
		<< "actor.name: " << payload.get_or(std::string("actor_name"), std::string("unknown")) << "\n"
		<< "actor.uid: " << payload.get_or(std::string("actor_uid"), 0L) << "\n"
		<< "actor.room_vnum: " << payload.get_or(std::string("actor_room_vnum"), -1) << "\n"
		<< "result: " << payload.get_or(std::string("result"), std::string("n/a")) << "\n";
	return out.str();
}

sol::table CheckedScriptResult(const sol::protected_function_result &result) {
	if (!result.valid()) {
		const sol::error err = result;
		throw std::runtime_error(err.what());
	}

	return result;
}

int CheckedIntResult(const sol::protected_function_result& result)
{
	if (!result.valid())
	{
		const sol::error err = result;
		throw std::runtime_error(err.what());
	}

	if (result.return_count() == 0)
	{
		return 1;
	}

	const sol::object first = result.get<sol::object>(0);
	if (first.is<int>())
	{
		return first.as<int>();
	}
	if (first.is<bool>())
	{
		return first.as<bool>() ? 1 : 0;
	}

	throw std::runtime_error("death trigger candidate returned non-int result");
}

} // namespace

std::string RunSmokeTest(const ActorContext &actor) {
	auto lua = CreateState(actor);

	const auto result = lua.safe_script(R"(
		local require_jit_ok, require_jit_result = pcall(require, "jit")
		if require_jit_ok and type(require_jit_result) == "table" then
			jit = require_jit_result
		end

		local payload = {
			engine = build_tag,
			lua_version = _VERSION,
			jit_type = type(jit),
			jit_version = (type(jit) == "table" and jit.version) or "jit-unavailable",
			require_jit_ok = tostring(require_jit_ok),
			require_jit_error = require_jit_ok and "" or tostring(require_jit_result),
			loaded_jit_type = (package and package.loaded and type(package.loaded.jit)) or "nil",
			jit_status = (type(jit) == "table" and jit.status and tostring(select(1, jit.status()))) or "jit-status-unavailable",
			package_path = package and package.path or "package-unavailable",
			package_cpath = package and package.cpath or "package-unavailable",
			actor_name = actor.name,
			actor_uid = actor.uid,
			actor_room_vnum = actor.room_vnum,
			result = tostring(cpp_add(19, 23))
		}
		return payload
	)", sol::script_pass_on_error);

	std::ostringstream out;
	out << "Lua prototype smoke test succeeded.\n"
		<< RenderPayload(CheckedScriptResult(result));
	return out.str();
}

std::string RunExpression(const std::string &chunk, const ActorContext &actor) {
	auto lua = CreateState(actor);
	const auto result = lua.safe_script(chunk, sol::script_pass_on_error);

	if (!result.valid()) {
		const sol::error err = result;
		throw std::runtime_error(err.what());
	}

	std::ostringstream out;
	out << "Lua expression executed successfully.\n";
	if (result.return_count() > 0) {
		const sol::object first = result.get<sol::object>(0);
		out << "return[1]: " << sol::utility::to_string(first) << "\n";
	}
	out << "actor.name: " << actor.name << "\n"
		<< "actor.uid: " << actor.uid << "\n"
		<< "actor.room_vnum: " << actor.room_vnum << "\n";
	return out.str();
}

std::string RunFile(const std::string &path, const ActorContext &actor) {
	if (!std::filesystem::exists(path)) {
		throw std::runtime_error("Lua file does not exist: " + path);
	}

	auto lua = CreateState(actor);
	const auto result = lua.safe_script_file(path, sol::script_pass_on_error);
	if (!result.valid()) {
		const sol::error err = result;
		throw std::runtime_error(err.what());
	}

	std::ostringstream out;
	out << "Lua file executed successfully.\n"
		<< "file: " << path << "\n"
		<< "return_count: " << result.return_count() << "\n";
	if (result.return_count() > 0) {
		const sol::object first = result.get<sol::object>(0);
		out << "return[1]: " << sol::utility::to_string(first) << "\n";
	}
	out
		<< "actor.name: " << actor.name << "\n"
		<< "actor.uid: " << actor.uid << "\n"
		<< "actor.room_vnum: " << actor.room_vnum << "\n";
	return out.str();
}

std::string RunDeathTriggerCandidate(const DeathTriggerContext& ctx)
{
	ActorContext actor;
	actor.name = ctx.actor.name;
	actor.uid = ctx.actor.uid;
	actor.room_vnum = ctx.actor.room_vnum;

	auto lua = CreateState(actor);
	const auto load_result = lua.safe_script(R"(
		return function(victim, actor)
			if victim.charmed then
				return 1
			end
			if actor == nil then
				return 1
			end

			local same_room = victim.room_vnum == actor.room_vnum
			local actor_can_claim = actor.level >= victim.level or not actor.is_npc
			local mob_death = victim.is_npc and victim.mob_vnum > 0
			local checksum = victim.uid + actor.uid + victim.mob_vnum + actor.mob_vnum

			if same_room and actor_can_claim and mob_death and checksum ~= 0 then
				return 1
			end
			return 1
		end
	)", sol::script_pass_on_error);

	if (!load_result.valid())
	{
		const sol::error err = load_result;
		throw std::runtime_error(err.what());
	}

	sol::protected_function trigger = load_result.get<sol::protected_function>();
	sol::table victim = CreateCharacterTable(lua, ctx.victim);
	sol::object actor_arg = ctx.has_actor
		? sol::object(lua, sol::in_place, CreateCharacterTable(lua, ctx.actor))
		: sol::make_object(lua, sol::nil);

	const int iterations = ctx.iterations > 0 ? ctx.iterations : 1;
	int last_result = 1;

	const auto started_at = std::chrono::steady_clock::now();
	for (int i = 0; i < iterations; ++i)
	{
		last_result = CheckedIntResult(trigger(victim, actor_arg));
	}
	const auto elapsed = std::chrono::steady_clock::now() - started_at;
	const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	const auto avg_ns = static_cast<double>(elapsed_ns) / iterations;

	std::ostringstream out;
	out << "Lua death trigger candidate benchmark.\n"
		<< "mode: manual lua candidate, no DG script_driver\n"
		<< "iterations: " << iterations << "\n"
		<< "elapsed_ns: " << elapsed_ns << "\n"
		<< "avg_ns: " << avg_ns << "\n"
		<< "last_result: " << last_result << "\n"
		<< "victim.name: " << ctx.victim.name << "\n"
		<< "victim.uid: " << ctx.victim.uid << "\n"
		<< "victim.mob_vnum: " << ctx.victim.mob_vnum << "\n"
		<< "actor.name: " << (ctx.has_actor ? ctx.actor.name : std::string("nil")) << "\n"
		<< "actor.uid: " << (ctx.has_actor ? ctx.actor.uid : 0) << "\n";
	return out.str();
}

std::string RunDeathMtriggerCandidate(const DeathTriggerContext& ctx)
{
	ActorContext actor;
	actor.name = ctx.actor.name;
	actor.uid = ctx.actor.uid;
	actor.room_vnum = ctx.actor.room_vnum;

	auto lua = CreateState(actor);
	const auto load_result = lua.safe_script(R"(
		local function script_driver_stub(trigger, victim, actor)
			if actor ~= nil then
				trigger.actor_uid = actor.uid
			end
			return trigger.result or 1
		end

		return function(victim, actor)
			if victim == nil or victim.purged then
				return 1
			end

			if not victim.has_death_script or victim.charmed then
				return 1
			end

			local triggers = victim.triggers
			for i = 1, #triggers do
				local trigger = triggers[i]
				if trigger.death then
					return script_driver_stub(trigger, victim, actor)
				end
			end

			return 1
		end
	)", sol::script_pass_on_error);

	if (!load_result.valid())
	{
		const sol::error err = load_result;
		throw std::runtime_error(err.what());
	}

	sol::protected_function trigger = load_result.get<sol::protected_function>();
	sol::table victim = CreateCharacterTable(lua, ctx.victim);
	sol::object actor_arg = ctx.has_actor
		? sol::object(lua, sol::in_place, CreateCharacterTable(lua, ctx.actor))
		: sol::make_object(lua, sol::nil);

	const int iterations = ctx.iterations > 0 ? ctx.iterations : 1;
	int last_result = 1;

	const auto started_at = std::chrono::steady_clock::now();
	for (int i = 0; i < iterations; ++i)
	{
		last_result = CheckedIntResult(trigger(victim, actor_arg));
	}
	const auto elapsed = std::chrono::steady_clock::now() - started_at;
	const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	const auto avg_ns = static_cast<double>(elapsed_ns) / iterations;

	std::ostringstream out;
	out << "Lua death_mtrigger rewrite benchmark.\n"
		<< "mode: lua rewrite of death_mtrigger control flow\n"
		<< "iterations: " << iterations << "\n"
		<< "elapsed_ns: " << elapsed_ns << "\n"
		<< "avg_ns: " << avg_ns << "\n"
		<< "last_result: " << last_result << "\n"
		<< "has_death_script: " << (ctx.victim.has_death_script ? "true" : "false") << "\n"
		<< "death_trigger_count: " << ctx.victim.death_trigger_count << "\n"
		<< "charmed: " << (ctx.victim.charmed ? "true" : "false") << "\n"
		<< "victim.name: " << ctx.victim.name << "\n"
		<< "victim.uid: " << ctx.victim.uid << "\n"
		<< "victim.mob_vnum: " << ctx.victim.mob_vnum << "\n"
		<< "actor.name: " << (ctx.has_actor ? ctx.actor.name : std::string("nil")) << "\n"
		<< "actor.uid: " << (ctx.has_actor ? ctx.actor.uid : 0) << "\n";
	return out.str();
}

} // namespace lua_prototype

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
