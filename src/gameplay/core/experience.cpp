/**
\file experience.cpp - a part of the Bylins engine.
\brief issue.chardata-cleaning: experience-gain rules (see experience.h).
*/

#include "gameplay/core/experience.h"
#include "gameplay/statistics/char_stat.h"
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
#include "engine/db/db.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/core/remort.h"
#include "engine/structs/structs.h"

#include <algorithm>

// Per-kill exp caps (defined in engine/core/config.cpp).
int max_exp_gain_pc(CharData *ch);
int max_exp_loss_pc(CharData *ch);

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
double exp_coefficients[] =
	{
		1.0,            //0 remorts
		1.0 / 0.9,        //1 remort
		1.0 / 0.8,        //2 remorts
		1.0 / 0.7,        //3 remorts
		1.0 / 0.6,        //4 remorts
		1.0 / 0.5,        //5 remorts
		1.0 / 0.4,        //6 remorts
		1.0 / 0.3,        //7 remorts
		1.0 / 0.2,        //8 remorts
		1.0 / 0.1,        //9 remorts
		1.0 / 0.09,        //10 remorts
		1.0 / 0.08,        //11 remorts
		1.0 / 0.07,        //12 remorts
		1.0 / 0.06,        //13 remorts
		1.0 / 0.05,        //14 remorts
		1.0 / 0.05        //15 remorts
	};
}  // anonymous

long GetExpUntilNextLvl(CharData *ch, int level) {
	if (level > kLvlImplementator || level < 0) {
		log("SYSERR: Requesting exp for invalid level %d!", level);
		return 0;
	}

	/*
	 * Gods have exp close to EXP_MAX.  This statement should never have to
	 * changed, regardless of how many mortal or immortal levels exist.
	 */
	if (level > kLvlImmortal) {
		return kExpImpl - ((kLvlImplementator - level) * 1000);
	}

	// Exp required for normal mortals is below
	float exp_modifier;
	if (remort::GetRealRemort(ch) < kMaxExpCoefficientsUsed) {
		exp_modifier = static_cast<float>(exp_coefficients[remort::GetRealRemort(ch)]);
	} else {
		exp_modifier = static_cast<float>(exp_coefficients[kMaxExpCoefficientsUsed]);
	}

	switch (ch->GetClass()) {

		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 2500);
				case 3: return int(exp_modifier * 5000);
				case 4: return int(exp_modifier * 9000);
				case 5: return int(exp_modifier * 17000);
				case 6: return int(exp_modifier * 27000);
				case 7: return int(exp_modifier * 47000);
				case 8: return int(exp_modifier * 77000);
				case 9: return int(exp_modifier * 127000);
				case 10: return int(exp_modifier * 197000);
				case 11: return int(exp_modifier * 297000);
				case 12: return int(exp_modifier * 427000);
				case 13: return int(exp_modifier * 587000);
				case 14: return int(exp_modifier * 817000);
				case 15: return int(exp_modifier * 1107000);
				case 16: return int(exp_modifier * 1447000);
				case 17: return int(exp_modifier * 1847000);
				case 18: return int(exp_modifier * 2310000);
				case 19: return int(exp_modifier * 2830000);
				case 20: return int(exp_modifier * 3580000);
				case 21: return int(exp_modifier * 4580000);
				case 22: return int(exp_modifier * 5830000);
				case 23: return int(exp_modifier * 7330000);
				case 24: return int(exp_modifier * 9080000);
				case 25: return int(exp_modifier * 11080000);
				case 26: return int(exp_modifier * 15000000);
				case 27: return int(exp_modifier * 22000000);
				case 28: return int(exp_modifier * 33000000);
				case 29: return int(exp_modifier * 47000000);
				case 30: return int(exp_modifier * 64000000);
					// add new levels here
				default: return int(exp_modifier * 94000000);
			}
			break;

		case ECharClass::kSorcerer:
		case ECharClass::kMagus:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 2500);
				case 3: return int(exp_modifier * 5000);
				case 4: return int(exp_modifier * 9000);
				case 5: return int(exp_modifier * 17000);
				case 6: return int(exp_modifier * 27000);
				case 7: return int(exp_modifier * 47000);
				case 8: return int(exp_modifier * 77000);
				case 9: return int(exp_modifier * 127000);
				case 10: return int(exp_modifier * 197000);
				case 11: return int(exp_modifier * 297000);
				case 12: return int(exp_modifier * 427000);
				case 13: return int(exp_modifier * 587000);
				case 14: return int(exp_modifier * 817000);
				case 15: return int(exp_modifier * 1107000);
				case 16: return int(exp_modifier * 1447000);
				case 17: return int(exp_modifier * 1847000);
				case 18: return int(exp_modifier * 2310000);
				case 19: return int(exp_modifier * 2830000);
				case 20: return int(exp_modifier * 3580000);
				case 21: return int(exp_modifier * 4580000);
				case 22: return int(exp_modifier * 5830000);
				case 23: return int(exp_modifier * 7330000);
				case 24: return int(exp_modifier * 9080000);
				case 25: return int(exp_modifier * 11080000);
				case 26: return int(exp_modifier * 15000000);
				case 27: return int(exp_modifier * 22000000);
				case 28: return int(exp_modifier * 33000000);
				case 29: return int(exp_modifier * 47000000);
				case 30: return int(exp_modifier * 64000000);
					// add new levels here
				default: return int(exp_modifier * 87000000);
			}
			break;

		case ECharClass::kThief:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 1000);
				case 3: return int(exp_modifier * 2000);
				case 4: return int(exp_modifier * 4000);
				case 5: return int(exp_modifier * 8000);
				case 6: return int(exp_modifier * 15000);
				case 7: return int(exp_modifier * 28000);
				case 8: return int(exp_modifier * 52000);
				case 9: return int(exp_modifier * 85000);
				case 10: return int(exp_modifier * 131000);
				case 11: return int(exp_modifier * 192000);
				case 12: return int(exp_modifier * 271000);
				case 13: return int(exp_modifier * 372000);
				case 14: return int(exp_modifier * 512000);
				case 15: return int(exp_modifier * 672000);
				case 16: return int(exp_modifier * 854000);
				case 17: return int(exp_modifier * 1047000);
				case 18: return int(exp_modifier * 1274000);
				case 19: return int(exp_modifier * 1534000);
				case 20: return int(exp_modifier * 1860000);
				case 21: return int(exp_modifier * 2520000);
				case 22: return int(exp_modifier * 3380000);
				case 23: return int(exp_modifier * 4374000);
				case 24: return int(exp_modifier * 5500000);
				case 25: return int(exp_modifier * 6827000);
				case 26: return int(exp_modifier * 8667000);
				case 27: return int(exp_modifier * 13334000);
				case 28: return int(exp_modifier * 20000000);
				case 29: return int(exp_modifier * 28667000);
				case 30: return int(exp_modifier * 40000000);
					// add new levels here
				default: return int(exp_modifier * 53000000);
			}
			break;

		case ECharClass::kAssasine:
		case ECharClass::kMerchant:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 1500);
				case 3: return int(exp_modifier * 3000);
				case 4: return int(exp_modifier * 6000);
				case 5: return int(exp_modifier * 12000);
				case 6: return int(exp_modifier * 22000);
				case 7: return int(exp_modifier * 42000);
				case 8: return int(exp_modifier * 77000);
				case 9: return int(exp_modifier * 127000);
				case 10: return int(exp_modifier * 197000);
				case 11: return int(exp_modifier * 287000);
				case 12: return int(exp_modifier * 407000);
				case 13: return int(exp_modifier * 557000);
				case 14: return int(exp_modifier * 767000);
				case 15: return int(exp_modifier * 1007000);
				case 16: return int(exp_modifier * 1280000);
				case 17: return int(exp_modifier * 1570000);
				case 18: return int(exp_modifier * 1910000);
				case 19: return int(exp_modifier * 2300000);
				case 20: return int(exp_modifier * 2790000);
				case 21: return int(exp_modifier * 3780000);
				case 22: return int(exp_modifier * 5070000);
				case 23: return int(exp_modifier * 6560000);
				case 24: return int(exp_modifier * 8250000);
				case 25: return int(exp_modifier * 10240000);
				case 26: return int(exp_modifier * 13000000);
				case 27: return int(exp_modifier * 20000000);
				case 28: return int(exp_modifier * 30000000);
				case 29: return int(exp_modifier * 43000000);
				case 30: return int(exp_modifier * 59000000);
					// add new levels here
				default: return int(exp_modifier * 79000000);
			}
			break;

		case ECharClass::kWarrior:
		case ECharClass::kGuard:
		case ECharClass::kPaladine:
		case ECharClass::kRanger:
		case ECharClass::kVigilant:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 2000);
				case 3: return int(exp_modifier * 4000);
				case 4: return int(exp_modifier * 8000);
				case 5: return int(exp_modifier * 14000);
				case 6: return int(exp_modifier * 24000);
				case 7: return int(exp_modifier * 39000);
				case 8: return int(exp_modifier * 69000);
				case 9: return int(exp_modifier * 119000);
				case 10: return int(exp_modifier * 189000);
				case 11: return int(exp_modifier * 289000);
				case 12: return int(exp_modifier * 419000);
				case 13: return int(exp_modifier * 579000);
				case 14: return int(exp_modifier * 800000);
				case 15: return int(exp_modifier * 1070000);
				case 16: return int(exp_modifier * 1340000);
				case 17: return int(exp_modifier * 1660000);
				case 18: return int(exp_modifier * 2030000);
				case 19: return int(exp_modifier * 2450000);
				case 20: return int(exp_modifier * 2950000);
				case 21: return int(exp_modifier * 3950000);
				case 22: return int(exp_modifier * 5250000);
				case 23: return int(exp_modifier * 6750000);
				case 24: return int(exp_modifier * 8450000);
				case 25: return int(exp_modifier * 10350000);
				case 26: return int(exp_modifier * 14000000);
				case 27: return int(exp_modifier * 21000000);
				case 28: return int(exp_modifier * 31000000);
				case 29: return int(exp_modifier * 44000000);
				case 30: return int(exp_modifier * 64000000);
					// add new levels here
				default: return int(exp_modifier * 79000000);
			}
			break;
		default: break;
	}

	/*
	 * This statement should never be reached if the exp tables in this function
	 * are set up properly.  If you see exp of 123456 then the tables above are
	 * incomplete -- so, complete them!
	 */
	log("SYSERR: XP tables not set up correctly in class.c!");
	return 123456;
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
	} else {
		ch->dps_add_exp(gain);
		ZoneExpStat::add(GetZoneVnumByCharPlace(ch), gain);
	}

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
		while (GetRealLevel(ch) < kLvlImmortal && ch->get_exp() >= experience::GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
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
		while (GetRealLevel(ch) > 1 && ch->get_exp() < experience::GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
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
	int is_altered = false;
	int num_levels = 0;

	ch->set_exp(ch->get_exp() + gain);
	if (!ch->IsNpc()) {
		if (gain > 0) {
			while (GetRealLevel(ch) < kLvlImplementator && ch->get_exp() >= experience::GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
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
			while (GetRealLevel(ch) > 1 && ch->get_exp() < experience::GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
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
}  // namespace experience

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
