/**
\file experience.cpp - a part of the Bylins engine.
\brief issue.chardata-cleaning: experience-gain rules (see experience.h).
*/

#include "gameplay/core/experience.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/statistics/zone_exp.h"
#include "gameplay/clans/house.h"
#include "engine/ui/color.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/communication/offtop.h"
#include "gameplay/statistics/top.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/classes/pc_classes.h"
#include "administration/privilege.h"

#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/core/config.h"
#include "utils/utils_parse.h"
#include "engine/entities/zone.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/core/remort.h"
#include "gameplay/mechanics/bonus.h"
#include "utils/logger.h"                  // SendToTC
#include "gameplay/mechanics/dungeons.h"   // kZoneStartDungeons
#include "gameplay/mechanics/alignment.h"  // ChangeAlignment
#include "gameplay/skills/leadership.h"     // CalcLeadershipGroupExpKoeff
#include "gameplay/fight/fight_penalties.h" // GroupPenaltyCalculator / GroupPenalties grouping
#include "utils/grammar/declensions.h"      // GetDeclensionInNumber
#include "engine/core/utils_char_obj.inl"   // InTestZone
#include "engine/structs/structs.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

extern int max_exp_gain_npc;   // config.cpp global (no header); per-kill NPC exp cap

namespace experience {

// was CharData::get_zone_group -- the group size is a property of the mob's zone.
int GetZoneGroup(const CharData *ch) {
	const auto rnum = ch->get_rnum();
	if (ch->IsNpc()
		&& rnum >= 0
		&& mob_index[rnum].zone >= 0) {
		const auto zone = GetZoneRnum(GET_MOB_VNUM(ch) / 100);
		return std::max(1, zone_table[zone].group);
	}

	return 1;
}

// was the free OK_GAIN_EXP that lived in char_data.cpp.
bool OkGainExp(const CharData *ch, const CharData *victim) {
	return !NAME_BAD(ch)
		&& (NAME_FINE(ch)
			|| !(GetRealLevel(ch) == kNameLevel))
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		&& victim->IsNpc()
		&& (victim->get_exp() > 0)
		&& (!victim->IsNpc()
			|| !ch->IsNpc()
			|| AFF_FLAGGED(ch, EAffect::kCharmed))
		&& !mount::IsHorse(victim)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena);
}

namespace {
const int kExpImpl = 2000000000;
std::vector<double> g_coeffs;                                  // remort -> exp multiplier
std::unordered_map<std::string, std::vector<long>> g_tables;   // table id -> base exp by level

// One-shot English diagnostic to the error log when a grant is attempted with no usable table.
void ReportBrokenExpTable() {
	static bool reported = false;
	if (reported) {
		return;
	}
	reported = true;
	mudlog("SYSERR: experience table unavailable (cfg/experience_table.xml missing/corrupt, or a "
		   "class lacks a valid <experience_table>); experience cannot be granted.",
		   LogMode::BRF, kLvlImplementator, EOutputStream::ERRLOG, true);
}}  // anonymous

long GetExpUntilNextLvl(CharData *ch, int level) {
	if (level > kLvlImplementator || level < 0) {
		log("SYSERR: Requesting exp for invalid level %d!", level);
		return 0;
	}
	// Gods sit near EXP_MAX, regardless of the mortal/immortal level counts.
	if (level > kLvlImmortal) {
		return kExpImpl - ((kLvlImplementator - level) * 1000);
	}
	if (g_coeffs.empty()) {
		return 0;   // no coefficients loaded; exp grants are gated by CanGainExp().
	}
	const int remort = std::clamp(remort::GetRealRemort(ch), 0, static_cast<int>(g_coeffs.size()) - 1);
	const double exp_modifier = g_coeffs[remort];
	const auto it = g_tables.find(MUD::Class(ch->GetClass()).GetExpTableId());
	if (it == g_tables.end() || level >= static_cast<int>(it->second.size())) {
		return 0;   // class has no table / level out of range; gated by CanGainExp().
	}
	// Identical processing to the old table: truncate (exp_modifier * base) to int.
	return static_cast<int>(exp_modifier * it->second[level]);
}

void levelup_events(CharData *ch) {
	if (offtop_system::kMinOfftopLvl == GetRealLevel(ch)
		&& !ch->get_disposable_flag(DIS_OFFTOP_MESSAGE)) {
		ch->SetFlag(EPrf::kOfftopMode);
		ch->set_disposable_flag(DIS_OFFTOP_MESSAGE);
		SendMsgToChar(ch,
					  "%sТеперь вы можете пользоваться каналом оффтоп ('справка оффтоп').%s\r\n",
					  kColorBoldGrn, kColorNrm);
	}
	if (EXCHANGE_MIN_CHAR_LEV == GetRealLevel(ch)
		&& !ch->get_disposable_flag(DIS_EXCHANGE_MESSAGE)) {
		// по умолчанию базар у всех включен, поэтому не спамим даже однократно
		if (remort::GetRealRemort(ch) <= 0) {
			SendMsgToChar(ch,
						  "%sТеперь вы можете покупать и продавать вещи на базаре ('справка базар!').%s\r\n",
						  kColorBoldGrn, kColorNrm);
		}
		ch->set_disposable_flag(DIS_EXCHANGE_MESSAGE);
	}
}

void advance_level(CharData *ch) {
	int add_move = 0, i;

	switch (ch->GetClass()) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: add_move = 2;
			break;

		case ECharClass::kThief:
		case ECharClass::kAssasine:
		case ECharClass::kMerchant:
		case ECharClass::kWarrior:
		case ECharClass::kGuard:
		case ECharClass::kRanger:
		case ECharClass::kPaladine: [[fallthrough]];
		case ECharClass::kVigilant: add_move = number(ch->GetInbornDex()/6 + 1, ch->GetInbornDex()/5 + 1);
			break;
		default: break;
	}

	check_max_hp(ch);
	ch->set_max_move(ch->get_max_move() + std::max(1, add_move));

	SetInbornAndRaceFeats(ch);

	if (privilege::IsImmortal(ch)) {
		for (i = 0; i < 3; i++) {
			GET_COND(ch, i) = (char) -1;
		}
		ch->SetFlag(EPrf::kHolylight);
	}

	TopPlayer::Refresh(ch);
	levelup_events(ch);
	ch->save_char();
}

void decrease_level(CharData *ch) {
	int add_move = 0;

	switch (ch->GetClass()) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: add_move = 2;
			break;

		case ECharClass::kThief:
		case ECharClass::kAssasine:
		case ECharClass::kMerchant:
		case ECharClass::kWarrior:
		case ECharClass::kGuard:
		case ECharClass::kPaladine:
		case ECharClass::kRanger: [[fallthrough]];
		case ECharClass::kVigilant: add_move = ch->GetInbornDex() / 5 + 1;
			break;
		default: break;
	}

	check_max_hp(ch);
	ch->set_max_move(ch->get_max_move() - std::clamp(add_move, 1, ch->get_max_move()));

	GET_WIMP_LEV(ch) = std::clamp(GET_WIMP_LEV(ch), 0, ch->get_real_max_hit()/2);
	if (!privilege::IsImmortal(ch)) {
		ch->UnsetFlag(EPrf::kHolylight);
	}

	TopPlayer::Refresh(ch);
	ch->save_char();
}
void update_clan_exp(CharData *ch, int gain) {
	if (CLAN(ch) && gain != 0) {
		// экспа для уровня клана (+ только на праве, - любой, но /5)
		if (gain < 0 || GET_GOD_FLAG(ch, EGf::kRemort)) {
			int tmp = gain > 0 ? gain : gain / 5;
			CLAN(ch)->SetClanExp(ch, tmp);
		}
		// экспа для топа кланов за месяц (учитываются все + и -)
		CLAN(ch)->last_exp.add_temp(gain);
		// экспа для топа кланов за все время (учитываются все + и -)
		CLAN(ch)->AddTopExp(ch, gain);
		// экспа для авто-очистки кланов (учитываются только +)
		if (gain > 0) {
			CLAN(ch)->exp_history.add_exp(gain);
		}
	}
}

void EndowExpToChar(CharData *ch, int gain) {
	int is_altered = false;
	int num_levels = 0;
	char local_buf[128];

	if (ch->IsNpc()) {
		ch->set_exp(ch->get_exp() + gain);
		return;
	}
	if (!CanGainExp(ch)) {
		if (gain != 0) {
			SendMsgToChar(CommonMsg(ECommonMsg::kBrokenScales) + "\r\n", ch);
			ReportBrokenExpTable();
		}
		return;
	}
	ch->dps_add_exp(gain);
	ZoneExpStat::add(GetZoneVnumByCharPlace(ch), gain);

	if (!ch->IsNpc() && ((GetRealLevel(ch) < 1 || GetRealLevel(ch) >= kLvlImmortal))) {
		return;
	}

	if (gain > 0 && GetRealLevel(ch) < kLvlImmortal) {
		gain = std::min(max_exp_gain_pc(ch), gain);    // put a cap on the max gain per kill
		ch->set_exp(ch->get_exp() + gain);
		if (ch->get_exp() >= experience::GetExpUntilNextLvl(ch, kLvlImmortal)) {
			if (!GET_GOD_FLAG(ch, EGf::kRemort) && remort::GetRealRemort(ch) < kMaxRemort) {
					SendMsgToChar(ch, "%sПоздравляем, вы получили право на перевоплощение!%s\r\n",
								  kColorBoldGrn, kColorNrm);
				SET_BIT(ch->player_specials->saved.GodsLike, EGf::kRemort);
			}
		}
		ch->set_exp(std::min(ch->get_exp(), experience::GetExpUntilNextLvl(ch, kLvlImmortal) - 1));
		// defense-in-depth: also bound on GetLevel() (the value actually mutated) so the loop
		// terminates even if GetRealLevel() ever diverges from GetLevel() (issue.advance-crash-bug).
		while (GetRealLevel(ch) < kLvlImmortal && ch->GetLevel() < kLvlImmortal
			   && ch->get_exp() >= experience::GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
			ch->set_level(ch->GetLevel() + 1);
			num_levels++;
			sprintf(local_buf, "%sВы достигли следующего уровня!%s\r\n", kColorWht, kColorNrm);
			SendMsgToChar(local_buf, ch);
			experience::advance_level(ch);
			is_altered = true;
		}

		if (is_altered) {
			sprintf(local_buf, "%s advanced %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
			mudlog(local_buf, BRF, kLvlImplementator, SYSLOG, true);
		}
	} else if (gain < 0 && GetRealLevel(ch) < kLvlImmortal) {
		gain = std::max(-max_exp_loss_pc(ch), gain);    // Cap max exp lost per death
		ch->set_exp(ch->get_exp() + gain);
		while (GetRealLevel(ch) > 1 && ch->GetLevel() > 1
			   && ch->get_exp() < experience::GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
			ch->set_level(ch->GetLevel() - 1);
			num_levels++;
			sprintf(local_buf,
					"%sВы потеряли уровень. Вам должно быть стыдно!%s\r\n",
					kColorBoldRed, kColorNrm);
			SendMsgToChar(local_buf, ch);
			experience::decrease_level(ch);
			is_altered = true;
		}
		if (is_altered) {
			sprintf(local_buf, "%s decreases %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
			mudlog(local_buf, BRF, kLvlImplementator, SYSLOG, true);
		}
	}
	if ((ch->get_exp() < experience::GetExpUntilNextLvl(ch, kLvlImmortal) - 1)
		&& GET_GOD_FLAG(ch, EGf::kRemort)
		&& gain
		&& (GetRealLevel(ch) < kLvlImmortal)) {
			SendMsgToChar(ch, "%sВы потеряли право на перевоплощение!%s\r\n",
						  kColorBoldRed, kColorNrm);
		REMOVE_BIT(ch->player_specials->saved.GodsLike, EGf::kRemort);
	}

	char_stat::AddClassExp(ch->GetClass(), gain);
	update_clan_exp(ch, gain);
}

void gain_exp_regardless(CharData *ch, int gain) {
	if (!ch->IsNpc() && !CanGainExp(ch)) {
		if (gain != 0) {
			SendMsgToChar(CommonMsg(ECommonMsg::kBrokenScales) + "\r\n", ch);
			ReportBrokenExpTable();
		}
		return;
	}
	int is_altered = false;
	int num_levels = 0;

	ch->set_exp(ch->get_exp() + gain);
	if (!ch->IsNpc()) {
		if (gain > 0) {
			// defense-in-depth: also bound on GetLevel() (the value actually mutated) so the loop
			// terminates even if GetRealLevel() ever diverges from GetLevel() (issue.advance-crash-bug).
			while (GetRealLevel(ch) < kLvlImplementator && ch->GetLevel() < kLvlImplementator
				   && ch->get_exp() >= experience::GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
				ch->set_level(ch->GetLevel() + 1);
				num_levels++;
				sprintf(buf, "%sВы достигли следующего уровня!%s\r\n",
						kColorWht, kColorNrm);
				SendMsgToChar(buf, ch);

				experience::advance_level(ch);
				is_altered = true;
			}

			if (is_altered) {
				sprintf(buf, "%s advanced %d level%s to level %d.",
						GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
			}
		} else if (gain < 0) {
			// Pereplut: глупый участок кода.
			//			gain = std::max(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
			//			ch->get_exp() += gain;
			//			if (ch->get_exp() < 0)
			//				ch->get_exp() = 0;
			while (GetRealLevel(ch) > 1 && ch->GetLevel() > 1
				   && ch->get_exp() < experience::GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
				ch->set_level(ch->GetLevel() - 1);
				num_levels++;
				sprintf(buf,
						"%sВы потеряли уровень!%s\r\n",
						kColorBoldRed, kColorNrm);
				SendMsgToChar(buf, ch);
				experience::decrease_level(ch);
				is_altered = true;
			}
			if (is_altered) {
				sprintf(buf, "%s decreases %d level%s to level %d.",
						GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
			}
		}

	}
}
// Per-kill exp caps: most a player may gain/lose in one kill/death.
int max_exp_gain_pc(CharData *ch) {
	int result = 1;
	if (!ch->IsNpc()) {
		int max_per_lev = GetExpUntilNextLvl(ch, ch->GetLevel() + 1) - GetExpUntilNextLvl(ch, ch->GetLevel() + 0); //уровень без плюсов от стафа
		result = max_per_lev / (10 + remort::GetRealRemort(ch));
	}
	return result;
}

int max_exp_loss_pc(CharData *ch) {
	return (ch->IsNpc() ? 1 : (GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - GetExpUntilNextLvl(ch, GetRealLevel(ch) + 0)) / 3);
}
void gain_battle_exp(CharData *ch, CharData *victim, int dam) {
	// не даем получать батлу с себя по зеркалу?
	if (ch == victim) {
		return;
	}
	if (!victim->IsNpc()) { return; }
	// не даем получать экспу с !эксп мобов
	if (victim->IsFlagged(EMobFlag::kNoBattleExp)) {
		return;
	}
	// если цель не нпс то тоже не даем экспы
	// если цель под чармом не даем экспу
	if (AFF_FLAGGED(victim, EAffect::kCharmed)) {
		return;
	}
	// получение игроками экспы
	if (!ch->IsNpc() && OkGainExp(ch, victim)) {
		int max_exp = std::min(max_exp_gain_pc(ch), (GetRealLevel(victim) * victim->get_max_hit() + 4) /
			(5 * std::max(1, remort::GetRealRemort(ch) - (RemortCoefficientCount() - 1) - 1)));
		double coeff = std::min(dam, victim->get_hit()) / static_cast<double>(victim->get_max_hit());
		int battle_exp = std::max(1, static_cast<int>(max_exp * coeff));
		if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_WEAPON_EXP) && Bonus::can_get_bonus_exp(ch)) {
			battle_exp *= Bonus::get_mult_bonus();
		}
		EndowExpToChar(ch, battle_exp);
		ch->dps_add_exp(battle_exp, true);
	}

	// перенаправляем батлэкспу чармиса в хозяина, цифры те же что и у файтеров.
	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed)) {
		CharData *master = ch->get_master();
		// проверяем что есть мастер и он может получать экспу с данной цели
		if (master && OkGainExp(master, victim)) {
			int max_exp = std::min(max_exp_gain_pc(master), (GetRealLevel(victim) * victim->get_max_hit() + 4) /
				(5 * std::max(1, remort::GetRealRemort(master) - (RemortCoefficientCount() - 1) - 1)));

			double coeff = std::min(dam, victim->get_hit()) / static_cast<double>(victim->get_max_hit());
			int battle_exp = std::max(1, static_cast<int>(max_exp * coeff));
			if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_WEAPON_EXP) && Bonus::can_get_bonus_exp(master)) {
				battle_exp *= Bonus::get_mult_bonus();
			}
			EndowExpToChar(master, battle_exp);
			master->dps_add_exp(battle_exp, true);
		}
	}
}

int RemortCoefficientCount() {
	return static_cast<int>(g_coeffs.size());
}

bool CanGainExp(const CharData *ch) {
	if (g_coeffs.empty()) {
		return false;
	}
	const auto it = g_tables.find(MUD::Class(ch->GetClass()).GetExpTableId());
	return it != g_tables.end() && !it->second.empty();
}

void ExperienceTableLoader::Load(parser_wrapper::DataNode data) {
	g_coeffs.clear();
	g_tables.clear();
	for (auto section : data.Children()) {
		const std::string name = section.GetName();
		if (name == "remort_coefficients") {
			for (auto coeff : section.Children()) {
				const double numerator = parse::ReadAsDouble(coeff.GetValue("numerator"));
				const double denominator = parse::ReadAsDouble(coeff.GetValue("denominator"));
				if (denominator <= 0.0) {
					mudlog("SYSERR: experience_table.xml: non-positive denominator in "
						   "remort_coefficients; experience table rejected.",
						   LogMode::BRF, kLvlImplementator, EOutputStream::ERRLOG, true);
					g_coeffs.clear();
					g_tables.clear();
					return;
				}
				g_coeffs.push_back(numerator / denominator);
			}
		} else if (name == "level_exp_tables") {
			for (auto table_node : section.Children()) {
				const std::string id = parse::ReadAsStr(table_node.GetValue("id"));
				std::vector<long> &table = g_tables[id];
				for (auto entry : table_node.Children()) {
					const int level = parse::ReadAsInt(entry.GetValue("level"));
					if (level < 0) {
						continue;
					}
					if (static_cast<int>(table.size()) <= level) {
						table.resize(level + 1, 0);
					}
					table[level] = parse::ReadAsInt(entry.GetValue("exp"));
				}
			}
		}
	}
	if (g_coeffs.empty() || g_tables.empty()) {
		mudlog("SYSERR: cfg/experience_table.xml missing or empty; characters cannot gain experience.",
			   LogMode::BRF, kLvlImplementator, EOutputStream::ERRLOG, true);
	}
}

void ExperienceTableLoader::Reload(parser_wrapper::DataNode) {
	// Intentionally a no-op beyond a notice: hot reload of exp tables is unsupported.
	mudlog("Experience table hot-reload is not supported; restart to apply changes.",
		   LogMode::BRF, kLvlImplementator, EOutputStream::SYSLOG, true);
}

// issue.fight-stuff: kill -> experience gain, moved here from fight_stuff.cpp. These
// compute and distribute kill exp to a group (mobmax/remort scaling, rare-mob bonus,
// group leadership/penalty coefficients) -- siblings of gain_battle_exp/EndowExpToChar.
int get_remort_mobmax(CharData *ch) {
	int remort = remort::GetRealRemort(ch);
	if (remort >= 18)
		return 15;
	if (remort >= 14)
		return 7;
	if (remort >= 9)
		return 4;
	return 0;
}

// даем увеличенную экспу за давно не убитых мобов.
// за совсем неубитых мобов не даем, что бы новые зоны не давали x10 экспу.
int get_npc_long_live_exp_bounus(CharData *victim) {
	if (GET_MOB_VNUM(victim) == -1) {
		return 1;
	}
	if (GET_MOB_VNUM(victim) / 100 >= dungeons::kZoneStartDungeons) {
		return 1;
	}

	int exp_multiplier = 1;

	const auto last_kill_time = mob_stat::GetMobKilllastTime(GET_MOB_VNUM(victim));
	if (last_kill_time > 0) {
		const auto now_time = time(nullptr);
		if (now_time > last_kill_time) {
			const auto delta_time = now_time - last_kill_time;
			constexpr long delay = 60 * 60 * 24 * 30; // 30 days
			exp_multiplier = std::clamp(static_cast<int>(floor(delta_time / delay)), 1, 10);
		}
	}

	return exp_multiplier;
}

long long get_extend_exp(long long exp, CharData *ch, CharData *victim) {
	int base, diff;
	int koef;

	if (!victim->IsNpc() || ch->IsNpc())
		return (exp);
	MobVnum vnum  = GET_MOB_VNUM(victim);
	if (vnum >= dungeons::kZoneStartDungeons * 100) {
		ZoneVnum zvn = vnum / 100;
		MobVnum  mvn = vnum % 100;
		vnum = zone_table[GetZoneRnum(zvn)].copy_from_zone * 100 + mvn;
	}

	SendToTC(ch, false, true, false,
				   "&RУ моба еще %d убийств без замакса, экспа %lld, убито %d&n\r\n",
				   victim->mob_specials.MaxFactor,
				   exp,
				   ch->mobmax_get(vnum));

	exp += static_cast<int>(exp * (ch->add_abils.percent_exp_add) / 100.0);
	for (koef = 100, base = 0, diff =
		 ch->mobmax_get(vnum) - victim->mob_specials.MaxFactor;
		 base < diff && koef > 5; base++, koef = koef * (95 - get_remort_mobmax(ch)) / 100);
	// минимальный опыт при замаксе 15% от полного опыта
	exp = exp * std::max(15, koef) / 100;

	// делим на реморты
	exp /= std::max(1.0, 0.5 * (remort::GetRealRemort(ch) - (experience::RemortCoefficientCount() - 1)));
	return (exp);
}

// When ch kills victim

/*++
   Функция начисления опыта
      ch - кому опыт начислять
           Вызов этой функции для NPC смысла не имеет, но все равно
           какие-то проверки внутри зачем то делаются
--*/
void perform_group_gain(CharData *ch, CharData *victim, int members, int koef) {


// Странно, но для NPC эта функция тоже должна работать
//  if (ch->IsNpc() || !experience::OkGainExp(ch,victim))
	if (!experience::OkGainExp(ch, victim)) {
		SendMsgToChar("Ваше деяние никто не оценил.\r\n", ch);
		return;
	}

	// 1. Опыт делится поровну на всех
	long long exp = victim->get_exp() / std::max(members, 1);
	int long_live_exp_bounus_miltiplier = 1;

	if (experience::GetZoneGroup(victim) > 1 && members < experience::GetZoneGroup(victim)) {
		// в случае груп-зоны своего рода планка на мин кол-во человек в группе
		exp = victim->get_exp() / experience::GetZoneGroup(victim);
	}
	// 2. Учитывается коэффициент (лидерство, разность уровней)
	//    На мой взгляд его правильней использовать тут а не в конце процедуры,
	//    хотя в большинстве случаев это все равно
	exp = exp * koef / 100;
	// 3. Вычисление опыта для PC и NPC
	if (!NPC_FLAGGED(victim, ENpcFlag::kIgnoreRareKill)) {
		long_live_exp_bounus_miltiplier = get_npc_long_live_exp_bounus(victim);
	}
	if (ch->IsNpc()) {
		exp = std::min(static_cast<long long>(max_exp_gain_npc), exp);
		exp += std::max(static_cast<long long>(0), (exp * std::min(0, (GetRealLevel(victim) - GetRealLevel(ch)))) / 8);
	} else
		exp = std::min(static_cast<long long>(experience::max_exp_gain_pc(ch)), get_extend_exp(exp, ch, victim) * long_live_exp_bounus_miltiplier);
	// 4. Последняя проверка
	exp = std::max(static_cast<long long>(1), exp);
	if (exp > 1) {
		if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_EXP) && Bonus::can_get_bonus_exp(ch)) {
			exp *= Bonus::get_mult_bonus();
		}

		if (!ch->IsNpc() && !ch->affected.empty() && Bonus::can_get_bonus_exp(ch)) {
			for (const auto &aff : ch->affected) {
				if (aff->location == EApply::kExpBonus) // скушал свиток с эксп бонусом
				{
					exp *= std::min(3, aff->modifier); // бонус макс тройной
				}
			}
		}

		if (long_live_exp_bounus_miltiplier > 1) {
			std::string mess;
			switch (long_live_exp_bounus_miltiplier) {
				case 2: mess = "Редкая удача! Опыт повышен!\r\n";
					break;
				case 3: mess = "Очень редкая удача! Опыт повышен!\r\n";
					break;
				case 4: mess = "Очень-очень редкая удача! Опыт повышен!\r\n";
					break;
				case 5: mess = "Вы везунчик! Опыт повышен!\r\n";
					break;
				case 6: mess = "Ваша удача велика! Опыт повышен!\r\n";
					break;
				case 7: mess = "Ваша удача достигла небес! Опыт повышен!\r\n";
					break;
				case 8: mess = "Ваша удача коснулась луны! Опыт повышен!\r\n";
					break;
				case 9: mess = "Ваша удача затмевает солнце! Опыт повышен!\r\n";
					break;
				default: mess = "Ваша удача выше звезд! Опыт повышен!\r\n";
					break;
			}
			SendMsgToChar(mess.c_str(), ch);
		}
		if (long_live_exp_bounus_miltiplier >= 10) {
			const CharData *ch_with_bonus = ch->IsNpc() ? ch->get_master() : ch;
			if (ch_with_bonus && !ch_with_bonus->IsNpc()) {
				std::stringstream str_log;
				str_log << "[INFO] " << ch_with_bonus->get_name() << " получил(а) x" << long_live_exp_bounus_miltiplier << " опыта за убийство моба: [";
				str_log << GET_MOB_VNUM(victim) << "] " << victim->get_name();
				mudlog(str_log.str(), NRM, kLvlImmortal, SYSLOG, true);
			}
		}

		exp = std::min(static_cast<long long>(experience::max_exp_gain_pc(ch)), exp);
		SendMsgToChar(ch, "Ваш опыт повысился на %lld %s.\r\n", exp, grammar::GetDeclensionInNumber(exp, grammar::EWhat::kPoint));
	} else if (exp == 1) {
		SendMsgToChar("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);
	}
	if (!InTestZone(ch)) {
		experience::EndowExpToChar(ch, exp);
		alignment::ChangeAlignment(ch, victim);
		if (!(victim)->Temporary.get(ECharExtraFlag::kGrpKillCount)
				&& !ch->IsNpc()
				&& !privilege::IsImmortal(ch)
				&& victim->IsNpc()
				&& !IsCharmice(victim)
				&& !ROOM_FLAGGED(victim->in_room, ERoomFlag::kArena)) {
				mob_stat::AddMob(victim, members);
				victim->Temporary.set(ECharExtraFlag::kGrpKillCount);
		} else if (ch->IsNpc() && !victim->IsNpc()
			&& !ROOM_FLAGGED(victim->in_room, ERoomFlag::kArena)) {
			mob_stat::AddMob(ch, 0);
		}
	}
}

/*++
   Функция расчитывает всякие бонусы для группы при получении опыта,
 после чего вызывает функцию получения опыта для всех членов группы
 Т.к. членом группы может быть только PC, то эта функция раздаст опыт только PC

   ch - обязательно член группы, из чего следует:
            1. Это не NPC
            2. Он находится в группе лидера (или сам лидер)

   Просто для PC-последователей эта функция не вызывается

--*/
void group_gain(CharData *killer, CharData *victim) {
	int inroom_members, koef = 100, maxlevel;
	int partner_count = 0;
	int total_group_members = 1;
	bool use_partner_exp = false;

	// если наем лидер, то тоже режем экспу
	if (CanUseFeat(killer, EFeat::kCynic)) {
		maxlevel = 300;
	} else {
		maxlevel = GetRealLevel(killer);
	}

	auto leader = killer->get_master();
	if (nullptr == leader) {
		leader = killer;
	}

	// k - подозрение на лидера группы
	const bool leader_inroom = AFF_FLAGGED(leader, EAffect::kGroup)
		&& leader->in_room == killer->in_room;

	// Количество согрупников в комнате
	if (leader_inroom) {
		inroom_members = 1;
		maxlevel = GetRealLevel(leader);
	} else {
		inroom_members = 0;
	}

	// Вычисляем максимальный уровень в группе
	for (auto *f : leader->followers) {
		if (AFF_FLAGGED(f, EAffect::kGroup)) ++total_group_members;
		if (AFF_FLAGGED(f, EAffect::kGroup)
			&& f->in_room == killer->in_room) {
			// если в группе наем, то режим опыт всей группе
			// дабы наема не выгодно было бы брать в группу
			// ставим 300, чтобы вообще под ноль резало
			if (CanUseFeat(f, EFeat::kCynic)) {
				maxlevel = 300;
			}
			// просмотр членов группы в той же комнате
			// член группы => PC автоматически
			++inroom_members;
			maxlevel = std::max(maxlevel, GetRealLevel(f));
			if (!f->IsNpc()) {
				partner_count++;
			}
		}
	}

	GroupPenaltyCalculator group_penalty(killer, leader, maxlevel, grouping);
	koef -= group_penalty.get();

	koef = std::max(0, koef);

	if (leader_inroom) {
		koef += CalcLeadershipGroupExpKoeff(leader, inroom_members, koef);
	}

	// Раздача опыта
	// если групповой уровень зоны равняется единице
	if (zone_table[world[killer->in_room]->zone_rn].group < 2) {
		// чтобы не абьюзили на суммонах, когда в группе на самом деле больше
		// двух мемберов, но лишних реколят перед непосредственным рипом
		use_partner_exp = total_group_members == 2;
	}

	// если лидер группы в комнате
	if (leader_inroom) {
		// если у лидера группы есть способность напарник
		if (CanUseFeat(leader, EFeat::kPartner) && use_partner_exp) {
			// если в группе всего двое человек
			// k - лидер, и один последователь
			if (partner_count == 1) {
				// и если кожф. больше или равен 100
				if (koef >= 100) {
					if (experience::GetZoneGroup(leader) < 2) {
						koef += 100;
					}
				}
			}
		}
		perform_group_gain(leader, victim, inroom_members, koef);
	}

	for (auto *f : leader->followers) {
		if (AFF_FLAGGED(f, EAffect::kGroup)
				&& f->in_room == killer->in_room) {
			perform_group_gain(f, victim, inroom_members, koef);
		}
	}
}

}  // namespace experience

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
