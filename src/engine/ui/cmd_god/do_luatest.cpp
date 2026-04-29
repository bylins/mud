// Part of Bylins http://bylins.su

#include "engine/ui/cmd_god/do_luatest.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>

#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/scripting/lua_prototype.h"
#include "engine/ui/modify.h"
#include "utils/utils.h"

namespace {

void PrintUsage(CharData *ch) {
	SendMsgToChar(
		"Usage:\r\n"
		"  luatest smoke\r\n"
		"  luatest file <path>\r\n"
		"  luatest deathmtrigger [iterations]\r\n",
		ch);
}

lua_prototype::ActorContext MakeActorContext(const CharData *ch) {
	lua_prototype::ActorContext ctx;
	ctx.name = ch->get_name();
	ctx.uid = ch->get_uid();
	ctx.room_vnum = ch->in_room == kNowhere ? kNowhere : world[ch->in_room]->vnum;
	return ctx;
}

lua_prototype::CharacterContext MakeCharacterContext(const CharData* ch)
{
	lua_prototype::CharacterContext ctx;
	if (!ch)
	{
		return ctx;
	}

	ctx.name = ch->get_name();
	ctx.uid = ch->get_uid();
	ctx.room_vnum = ch->in_room == kNowhere ? kNowhere : world[ch->in_room]->vnum;
	ctx.mob_vnum = GET_MOB_VNUM(ch);
	ctx.level = GetRealLevel(ch);
	ctx.hit = ch->get_hit();
	ctx.max_hit = ch->get_max_hit();
	ctx.is_npc = ch->IsNpc();
	ctx.charmed = AFF_FLAGGED(ch, EAffect::kCharmed);
	ctx.purged = ch->purged();
	ctx.has_death_script = CheckScript(ch, MTRIG_DEATH);
	if (SCRIPT(ch) && SCRIPT(ch)->has_triggers())
	{
		for (auto trigger : SCRIPT(ch)->script_trig_list)
		{
			if (TRIGGER_CHECK(trigger, MTRIG_DEATH))
			{
				++ctx.death_trigger_count;
			}
		}
	}
	return ctx;
}

int ParseIterations(const char* argument)
{
	if (!argument || !*argument)
	{
		return 10000;
	}

	char* end = nullptr;
	const auto value = std::strtol(argument, &end, 10);
	if (end == argument || value <= 0)
	{
		return 10000;
	}
	if (value > 10000000)
	{
		return 10000000;
	}
	return static_cast<int>(value);
}

void SetSingleCommand(Trigger* trigger, const std::string& command)
{
	trigger->cmdlist->reset(new cmdlist_element());
	const auto& cmdlist = *trigger->cmdlist;
	cmdlist->cmd = command;
	cmdlist->line_num = 1;
}

class SyntheticDeathTriggerIndexEntry
{
 public:
	SyntheticDeathTriggerIndexEntry()
	{
		m_old_index = trig_index;
		m_old_top = top_of_trigt;
		m_rnum = top_of_trigt;

		const int vnum = m_old_top > 0 && trig_index[m_old_top - 1]
			? trig_index[m_old_top - 1]->vnum + 1
			: 1;

		m_proto = new Trigger(m_rnum, "luatest death benchmark", MOB_TRIGGER, MTRIG_DEATH);
		SetSingleCommand(m_proto, "nop benchmark");

		m_entry = new IndexData(vnum);
		m_entry->proto = m_proto;

		auto** new_index = static_cast<IndexData**>(std::calloc(m_old_top + 1, sizeof(IndexData*)));
		for (int i = 0; i < m_old_top; ++i)
		{
			new_index[i] = trig_index[i];
		}
		new_index[m_rnum] = m_entry;

		trig_index = new_index;
		top_of_trigt = m_old_top + 1;
	}

	~SyntheticDeathTriggerIndexEntry()
	{
		std::free(trig_index);
		trig_index = m_old_index;
		top_of_trigt = m_old_top;

		delete m_entry;
		delete m_proto;
	}

	SyntheticDeathTriggerIndexEntry(const SyntheticDeathTriggerIndexEntry&) = delete;
	SyntheticDeathTriggerIndexEntry& operator=(const SyntheticDeathTriggerIndexEntry&) = delete;

	int rnum() const
	{
		return m_rnum;
	}

 private:
	IndexData** m_old_index = nullptr;
	IndexData* m_entry = nullptr;
	Trigger* m_proto = nullptr;
	int m_old_top = 0;
	int m_rnum = -1;
};

class SyntheticBenchmarkMob
{
 public:
	explicit SyntheticBenchmarkMob(int trigger_rnum) :
		m_mob(std::make_unique<CharData>())
	{
		m_mob->SetNpcAttribute(true);
		m_mob->SetCharAliases("luatest-benchmark-mob");
		m_mob->set_npc_name("luatest benchmark mob");
		m_mob->set_level(1);
		m_mob->set_hit(1);
		m_mob->set_max_hit(1);
		if (top_of_mobt >= 0)
		{
			m_mob->set_rnum(0);
		}
		m_mob->script = std::make_shared<Script>();

		Trigger* trigger = read_trigger(trigger_rnum);
		add_trigger(SCRIPT(m_mob.get()).get(), trigger, -1);
	}

	CharData* get() const
	{
		return m_mob.get();
	}

 private:
	std::unique_ptr<CharData> m_mob;
};

std::string RunDgDeathBenchmark(CharData* ch, int iterations)
{
	const int checked_iterations = iterations > 0 ? iterations : 1;
	int last_result = 1;
	int executed_iterations = 0;
	int death_trigger_count = 0;
	if (SCRIPT(ch) && SCRIPT(ch)->has_triggers())
	{
		for (auto trigger : SCRIPT(ch)->script_trig_list)
		{
			if (TRIGGER_CHECK(trigger, MTRIG_DEATH))
			{
				++death_trigger_count;
			}
		}
	}
	const bool check_script_death = CheckScript(ch, MTRIG_DEATH);

	const auto started_at = std::chrono::steady_clock::now();
	for (int i = 0; i < checked_iterations && !ch->purged(); ++i)
	{
		last_result = death_mtrigger(ch, ch);
		++executed_iterations;
	}
	const auto elapsed = std::chrono::steady_clock::now() - started_at;
	const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	const auto avg_ns = executed_iterations > 0 ? static_cast<double>(elapsed_ns) / executed_iterations : 0.0;

	std::ostringstream out;
	out << "DG death_mtrigger benchmark.\n"
		<< "mode: current DG death_mtrigger(ch, ch)\n"
		<< "iterations: " << executed_iterations << "\n"
		<< "requested_iterations: " << checked_iterations << "\n"
		<< "elapsed_ns: " << elapsed_ns << "\n"
		<< "avg_ns: " << avg_ns << "\n"
		<< "last_result: " << last_result << "\n"
		<< "check_script_death: " << (check_script_death ? "true" : "false") << "\n"
		<< "death_trigger_count: " << death_trigger_count << "\n"
		<< "charmed: " << (AFF_FLAGGED(ch, EAffect::kCharmed) ? "true" : "false") << "\n"
		<< "victim.name: " << ch->get_name() << "\n"
		<< "victim.uid: " << ch->get_uid() << "\n"
		<< "victim.mob_vnum: " << GET_MOB_VNUM(ch) << "\n";
	return out.str();
}

std::string RunDeathMtriggerBenchmark(CharData* owner, int iterations)
{
	constexpr int kWarmupIterations = 10000;

	SyntheticDeathTriggerIndexEntry trigger_index_entry;
	SyntheticBenchmarkMob mob(trigger_index_entry.rnum());

	RunDgDeathBenchmark(mob.get(), kWarmupIterations);

	lua_prototype::DeathTriggerContext warmup_ctx;
	warmup_ctx.victim = MakeCharacterContext(mob.get());
	warmup_ctx.actor = MakeCharacterContext(owner);
	warmup_ctx.has_actor = true;
	warmup_ctx.iterations = kWarmupIterations;
	lua_prototype::RunDeathMtriggerCandidate(warmup_ctx);

	std::ostringstream out;
	out << "Lua/DG death_mtrigger benchmark.\n"
		<< "warmup_iterations: " << kWarmupIterations << "\n\n";
	out << RunDgDeathBenchmark(mob.get(), iterations) << "\n";

	lua_prototype::DeathTriggerContext ctx;
	ctx.victim = MakeCharacterContext(mob.get());
	ctx.actor = MakeCharacterContext(owner);
	ctx.has_actor = true;
	ctx.iterations = iterations;
	out << lua_prototype::RunDeathMtriggerCandidate(ctx);
	return out.str();
}

} // namespace

void DoLuatest(CharData *ch, char *argument, int /* cmd */, int /* subcmd */) {
#if !defined(WITH_LUAJIT_PROTOTYPE)
	SendMsgToChar("Lua prototype is not compiled in.\r\n", ch);
	return;
#else
	char mode[kMaxInputLength];
	argument = one_argument(argument, mode);

	if (!*mode) {
		PrintUsage(ch);
		return;
	}

	try {
		const auto actor = MakeActorContext(ch);
		std::string output;

		if (!str_cmp(mode, "smoke")) {
			output = lua_prototype::RunSmokeTest(actor);
		} else if (!str_cmp(mode, "file")) {
			if (!*argument) {
				PrintUsage(ch);
				return;
			}
			output = lua_prototype::RunFile(argument, actor);
		} else if (!str_cmp(mode, "deathmtrigger"))
		{
			output = RunDeathMtriggerBenchmark(ch, ParseIterations(argument));
		} else {
			PrintUsage(ch);
			return;
		}

		page_string(ch->desc, output);
	} catch (const std::exception &e) {
		std::ostringstream out;
		out << "Lua test failed.\r\n" << e.what() << "\r\n";
		page_string(ch->desc, out.str());
	}
#endif
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
