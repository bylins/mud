//
// Created by Sventovit on 11.06.2026.
//

#include "gameplay/core/remort.h"
#include "gameplay/core/experience.h"
#include "administration/privilege.h"

#include "administration/karma.h"
#include "engine/entities/char_data.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/player_races.h"
#include "engine/core/char_equip_flags.h"
#include "engine/core/char_handler.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/skills/skills.h"

#include "utils/utils_parse.h"               // parse::AttrInt / parse::ReadAsBool
#include "engine/entities/char_player.h"      // START_STATS_TOTAL

#include <fmt/format.h>
#include <algorithm>

extern RoomRnum r_frozen_start_room;

namespace remort {

// --- issue.remort-system: cfg/remort.xml ------------------------------------------------------------
namespace {
// Defaults match the legacy hardcoded constants, so behaviour is identical until the file overrides
// them (and stays sane if the file/tag is missing).
struct RemortCfg {
	bool points_buy = false;
	int abil_reset_stop = 9;
	int abil_reset_period = 3;
	int skill_cap_start = 80;
	int skill_cap_increment = 5;
	int hp = 10;
	int moves = 82;
};
RemortCfg g_cfg;
} // namespace

bool IsPointsBuy() { return g_cfg.points_buy; }
int SkillCapStart() { return g_cfg.skill_cap_start; }
int SkillCapIncrement() { return g_cfg.skill_cap_increment; }
int CalcSkillCap(int remort_count) { return g_cfg.skill_cap_start + remort_count * g_cfg.skill_cap_increment; }
int RemortHp() { return g_cfg.hp; }
int RemortMoves() { return g_cfg.moves; }

bool IsObligatoryResetDue(int remort_count) {
	return g_cfg.abil_reset_period > 0
		&& remort_count >= g_cfg.abil_reset_stop
		&& remort_count % g_cfg.abil_reset_period == 0;
}

void ApplyRemortStatGains(const int *start, int *out, int remort_count) {
	if (!g_cfg.points_buy) {
		// legacy: +1 to every base stat per remort.
		for (int i = 0; i < START_STATS_TOTAL; ++i) {
			out[i] = start[i] + remort_count;
		}
		return;
	}
	// proportional: the highest start stat gains +1/remort; the rest scale to keep their original
	// ratio to it: out[i] = floor((max_start + remort) * start[i] / max_start).
	int max_start = 0;
	for (int i = 0; i < START_STATS_TOTAL; ++i) {
		max_start = std::max(max_start, start[i]);
	}
	if (max_start <= 0) {  // degenerate (shouldn't happen for valid chars) -- fall back to legacy.
		for (int i = 0; i < START_STATS_TOTAL; ++i) {
			out[i] = start[i] + remort_count;
		}
		return;
	}
	for (int i = 0; i < START_STATS_TOTAL; ++i) {
		out[i] = static_cast<int>(
			static_cast<long long>(max_start + remort_count) * start[i] / max_start);
	}
}

void RemortLoader::Load(parser_wrapper::DataNode data) {
	RemortCfg cfg;  // start from legacy defaults; only present tags/attrs override.
	{
		auto n = data;
		if (n.GoToChild("system")) {
			const char *pb = n.GetValue("points_buy");
			cfg.points_buy = pb && *pb && parse::ReadAsBool(pb);
		}
	}
	{
		auto n = data;
		if (n.GoToChild("abilities_reset")) {
			cfg.abil_reset_stop = parse::AttrInt(n, "stop_obligatory_reset", cfg.abil_reset_stop);
			cfg.abil_reset_period = parse::AttrInt(n, "period", cfg.abil_reset_period);
		}
	}
	{
		auto n = data;
		if (n.GoToChild("skill_cap")) {
			cfg.skill_cap_start = parse::AttrInt(n, "start_cap", cfg.skill_cap_start);
			cfg.skill_cap_increment = parse::AttrInt(n, "increment", cfg.skill_cap_increment);
		}
	}
	{
		auto n = data;
		if (n.GoToChild("hp")) { cfg.hp = parse::AttrInt(n, "val", cfg.hp); }
	}
	{
		auto n = data;
		if (n.GoToChild("moves")) { cfg.moves = parse::AttrInt(n, "val", cfg.moves); }
	}
	g_cfg = cfg;
}

void RemortLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}
// --- end issue.remort-system config ----------------------------------------------------------------

const char *remort_msg =
	"  Если вы так настойчивы в желании начать все заново -\r\n" "наберите <перевоплотиться> полностью.\r\n";

void ProcessRemort(CharData *ch, char *argument, int subcmd) {
	int i;
	const char *remort_msg2 = "$n вспыхнул$g ослепительным пламенем и пропал$g!\r\n";

	if (ch->IsNpc() || privilege::IsImmortal(ch)) {
		SendMsgToChar("Вам это, похоже, совсем ни к чему.\r\n", ch);
		return;
	}
	if (ch->get_exp() < experience::GetExpUntilNextLvl(ch, kLvlImmortal) - 1) {
		SendMsgToChar("ЧАВО???\r\n", ch);
		return;
	}
	if (ch->get_remort() >= kMaxRemort) {
		SendMsgToChar("Достигнуто максимальное количество перевоплощений.\r\n", ch);
		return;
	}
	if (NORENTABLE(ch)) {
		SendMsgToChar("Вы не можете перевоплотиться в связи с боевыми действиями.\r\n", ch);
		return;
	}
	if (!subcmd) {
		SendMsgToChar(remort_msg, ch);
		return;
	}

	one_argument(argument, arg);
	int place_of_destination;
	if (!*arg) {
		const auto msg = fmt::format("Укажите, где вы хотите заново начать свой путь:\r\n{}",
									 player_races::FormatStartRegionsMenu(GET_RACE(ch)));
		SendMsgToChar(msg, ch);
		return;
	} else {
		const int region = player_races::StartRegionByMenuChoice(GET_RACE(ch), arg);
		place_of_destination = player_races::StartRoomForRaceRegion(GET_RACE(ch), region);
		if (region == player_races::kRaceUndefined || place_of_destination == kNowhere) {
			SendMsgToChar("Багдад далече, выберите себе местечко среди родных осин.\r\n", ch);
			return;
		}
	}

	log("Remort %s", GET_NAME(ch));
	ch->remort();
	act(remort_msg2, false, ch, nullptr, nullptr, kToRoom);
	ch->set_remort(ch->get_remort() + 1);
	REMOVE_BIT(ch->player_specials->saved.GodsLike, EGf::kRemort);
	ch->inc_str(1);
	ch->inc_dex(1);
	ch->inc_con(1);
	ch->inc_wis(1);
	ch->inc_int(1);
	ch->inc_cha(1);
	if (ch->GetEnemy()) {
		stop_fighting(ch, true);
	}
	follow::DieFollower(ch);
	ch->affected.clear();
// Снимаем весь стафф
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			PlaceObjToInventory(UnequipChar(ch, i, CharEquipFlags()), ch);
		}
	}

	ch->timed_skill.clear();
	ch->timed_feat.clear();
	for (const auto &feat : MUD::Feats()) {
		ch->UnsetFeat(feat.GetId());
	}

	if (IsObligatoryResetDue(ch->get_remort())) {
		ch->clear_skills();
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(ch, spell_id) = (IS_MANA_CASTER(ch) ? ESpellType::kRunes : 0);
			GET_SPELL_MEM(ch, spell_id) = 0;
		}
	} else {
		SetSkillAfterRemort(ch);
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (IS_MANA_CASTER(ch)) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kRunes;
			} else if (MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() >= 8) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kUnknowm;
				GET_SPELL_MEM(ch, spell_id) = 0;
			}
		}
	}

	ch->set_max_hit(RemortHp());
	ch->set_hit(RemortHp());
	ch->set_max_move(RemortMoves());
	ch->set_move(RemortMoves());
	ch->mem_queue.total = ch->mem_queue.stored = 0;
	ch->set_level(0);
	GET_WIMP_LEV(ch) = 0;
	GET_AC(ch) = 100;
	GET_LOADROOM(ch) = place_of_destination;
	ch->UnsetFlag(EPrf::KSummonable);
	ch->UnsetFlag(EPrf::kAwake);
	ch->UnsetFlag(EPrf::kPunctual);
	ch->UnsetFlag(EPrf::kPerformPowerAttack);
	ch->UnsetFlag(EPrf::kPerformGreatPowerAttack);
	ch->UnsetFlag(EPrf::kAwake);
	ch->UnsetFlag(EPrf::kIronWind);
	ch->UnsetFlag(EPrf::kDoubleThrow);
	ch->UnsetFlag(EPrf::kTripleThrow);
	ch->UnsetFlag(EPrf::kShadowThrow);
	DeleteIrrelevantRunestones(ch);
	if (ch->get_protecting()) {
		ch->remove_protecting();
	}

	ch->ClearThisRemortStatistics();

	// забываем всех чармисов
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	if (!m->empty()) {
		auto it = m->begin();
		for (; it != m->end(); ++it) {
			it->second.currRemortAvail = 0;
		}
	}
	DoPcInit(ch, false);
	ch->save_char();
	RoomRnum load_room;
	if (ch->IsFlagged(EPlrFlag::kHelled))
		load_room = r_helled_start_room;
	else if (ch->IsFlagged(EPlrFlag::kNameDenied))
		load_room = r_named_start_room;
	else if (ch->IsFlagged(EPlrFlag::kFrozen))
		load_room = r_frozen_start_room;
	else {
		if ((load_room = GET_LOADROOM(ch)) == kNowhere)
			load_room = calc_loadroom(ch);
		load_room = GetRoomRnum(load_room);
	}
	if (load_room == kNowhere) {
		if (GetRealLevel(ch) >= kLvlImmortal)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, load_room);
	sight::look_at_room(ch, 0);
	ch->SetFlag(EPlrFlag::kNoDelete);
	room_spells::RemoveSingleAffectFromWorld(ch, ESpell::kRuneLabel);

	// сброс всего, связанного с гривнами (замакс сохраняем)
	ch->UnsetFlag(EPrf::kCanRemort);

	snprintf(buf, sizeof(buf),
			 "remort from %d to %d", ch->get_remort() - 1, ch->get_remort());
	snprintf(buf2, sizeof(buf2), "dest=%d", place_of_destination);
	AddKarma(ch, buf, buf2);

	act("$n вступил$g в игру.", true, ch, nullptr, nullptr, kToRoom);
	act("Вы перевоплотились! Желаем удачи!", false, ch, nullptr, nullptr, kToChar);
	affect_total(ch);
}

int GetRealRemort(const CharData *ch) {
	return std::clamp(ch->get_remort() + ch->get_remort_add(), 0, kMaxRemort);
}

int GetRealRemort(const std::shared_ptr<CharData> &ch) {
	return GetRealRemort(ch.get());
}

void SetSkillAfterRemort(CharData *ch) {
	for (const auto &it : ch->GetCharSkills()) {
		const int max_skill_level = CalcSkillHardCap(ch, it.first);
		if (GetSkillBonus(ch, it.first) > max_skill_level) {
			SetSkill(ch, it.first, max_skill_level);
		}
	}
}

} // namespace remort

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
