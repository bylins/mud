/**
\file experience.cpp - a part of the Bylins engine.
\brief issue.chardata-cleaning: experience-gain rules (see experience.h).
*/

#include "gameplay/core/experience.h"

#include "engine/entities/char_data.h"
#include "engine/db/db.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/core/remort.h"
#include "engine/structs/structs.h"

#include <algorithm>

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

}  // namespace experience

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
