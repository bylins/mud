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
#include "utils/parse.h"
#include "engine/entities/zone.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/core/remort.h"
#include "engine/structs/structs.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

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

}  // namespace experience

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
