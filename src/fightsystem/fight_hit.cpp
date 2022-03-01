#include "fight_hit.h"

#include "handler.h"
#include "color.h"
#include "game_magic/magic.h"
#include "pk.h"
#include "dps.h"
#include "house_exp.h"
#include "game_skills/poison.h"
#include "game_mechanics/bonus.h"
#include "mobact.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "structs/global_objects.h"

// extern
int extra_aco(int class_num, int level);
void alt_equip(CharData *ch, int pos, int dam, int chance);
int thaco(int class_num, int level);
void npc_groupbattle(CharData *ch);
void SetWait(CharData *ch, int waittime, int victim_in_room);
void go_autoassist(CharData *ch);

int armor_class_limit(CharData *ch) {
	if (IS_CHARMICE(ch)) {
		return -200;
	};
	if (IS_NPC(ch)) {
		return -300;
	};
	switch (GET_CLASS(ch)) {
		case ECharClass::kPaladine: return -270;
			break;
		case ECharClass::kAssasine:
		case ECharClass::kThief:
		case ECharClass::kGuard: return -250;
			break;
		case ECharClass::kMerchant:
		case ECharClass::kWarrior:
		case ECharClass::kRanger:
		case ECharClass::kVigilant: return -200;
			break;
		case ECharClass::kSorcerer:
		case ECharClass::kMagus: return -170;
			break;
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer: return -150;
			break;
			default: return -300;
	}
	return -300;
}

int compute_armor_class(CharData *ch) {
	int armorclass = GET_REAL_AC(ch);

	if (AWAKE(ch)) {
		int high_stat = GET_REAL_DEX(ch);
		// у игроков для ац берется макс стат: ловкость или 3/4 инты
		if (!IS_NPC(ch)) {
			high_stat = std::max(high_stat, GET_REAL_INT(ch) * 3 / 4);
		}
		armorclass -= dex_ac_bonus(high_stat) * 10;
		armorclass += extra_aco((int) GET_CLASS(ch), GetRealLevel(ch));
	};

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK)) {
		armorclass -= (240 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		armorclass += ch->get_skill(ESkill::kIronwind) / 2;
	}

	armorclass += (size_app[GET_POS_SIZE(ch)].ac * 10);

	if (GET_AF_BATTLE(ch, kEafPunctual)) {
		if (GET_EQ(ch, WEAR_WIELD)) {
			if (GET_EQ(ch, WEAR_HOLD))
				armorclass +=
					10 * MAX(-1,
							 (GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) +
								 GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) / 5 - 6);
			else
				armorclass += 10 * MAX(-1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) / 5 - 6);
		}
		if (GET_EQ(ch, WEAR_BOTHS))
			armorclass += 10 * MAX(-1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS)) / 5 - 6);
	}

	// Bonus for leadership
	if (calc_leadership(ch)) {
		armorclass -= 20;
	}

	armorclass = MIN(100, armorclass);
	return (MAX(armor_class_limit(ch), armorclass));
}

void haemorragia(CharData *ch, int percent) {
	Affect<EApplyLocation> af[3];

	af[0].type = kSpellHaemorragis;
	af[0].location = APPLY_HITREG;
	af[0].modifier = -percent;
	//TODO: Отрицательное время, если тело больше 31?
	af[0].duration = CalcDuration(ch, number(1, 31 - GET_REAL_CON(ch)), 0, 0, 0, 0);
	af[0].bitvector = 0;
	af[0].battleflag = 0;
	af[1].type = kSpellHaemorragis;
	af[1].location = APPLY_MOVEREG;
	af[1].modifier = -percent;
	af[1].duration = af[0].duration;
	af[1].bitvector = 0;
	af[1].battleflag = 0;
	af[2].type = kSpellHaemorragis;
	af[2].location = APPLY_MANAREG;
	af[2].modifier = -percent;
	af[2].duration = af[0].duration;
	af[2].bitvector = 0;
	af[2].battleflag = 0;

	for (int i = 0; i < 3; i++) {
		affect_join(ch, af[i], true, false, true, false);
	}
}

void HitData::compute_critical(CharData *ch, CharData *victim) {
	const char *to_char = nullptr, *to_vict = nullptr;
	Affect<EApplyLocation> af[4];
	ObjData *obj;
	int unequip_pos = 0;

	for (int i = 0; i < 4; i++) {
		af[i].type = 0;
		af[i].location = APPLY_NONE;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].duration = CalcDuration(victim, 2, 0, 0, 0, 0);
	}

	switch (number(1, 10)) {
		case 1:
		case 2:
		case 3:
		case 4:        // FEETS
			switch (dam_critic) {
				case 1:
				case 2:
				case 3:
					// Nothing
					return;
				case 4:    // Hit genus, victim bashed, speed/2
					SET_AF_BATTLE(victim, kEafSlow);
					dam *= (ch->get_skill(ESkill::kPunctual) / 10);
					if (GET_POS(victim) > EPosition::kSit)
						GET_POS(victim) = EPosition::kSit;
					WAIT_STATE(victim, 2 * kPulseViolence);
					to_char = "повалило $N3 на землю";
					to_vict = "повредило вам колено, повалив на землю";
					break;
				case 5:    // victim bashed
					if (GET_POS(victim) > EPosition::kSit)
						GET_POS(victim) = EPosition::kSit;
					WAIT_STATE(victim, 2 * kPulseViolence);
					to_char = "повалило $N3 на землю";
					to_vict = "повредило вам колено, повалив на землю";
					break;
				case 6:    // foot damaged, speed/2
					dam *= (ch->get_skill(ESkill::kPunctual) / 9);
					to_char = "замедлило движения $N1";
					to_vict = "сломало вам лодыжку";
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 7:
				case 9:    // armor damaged else foot damaged, speed/4
					if (GET_EQ(victim, WEAR_LEGS))
						alt_equip(victim, WEAR_LEGS, 100, 100);
					else {
						dam *= (ch->get_skill(ESkill::kPunctual) / 8);
						to_char = "замедлило движения $N1";
						to_vict = "сломало вам ногу";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
						SET_AF_BATTLE(victim, kEafSlow);
					}
					break;
				case 8:    // femor damaged, no speed
					dam *= (ch->get_skill(ESkill::kPunctual) / 7);
					to_char = "сильно замедлило движения $N1";
					to_vict = "сломало вам бедро";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 20);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 10:    // genus damaged, no speed, -2HR
					dam *= (ch->get_skill(ESkill::kPunctual) / 7);
					to_char = "сильно замедлило движения $N1";
					to_vict = "раздробило вам колено";
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 11:    // femor damaged, no speed, no attack
					dam *= (ch->get_skill(ESkill::kPunctual) / 7);
					to_char = "вывело $N3 из строя";
					to_vict = "раздробило вам бедро";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 20);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				default:    // femor damaged, no speed, no attack
					if (dam_critic > 12)
						dam *= (ch->get_skill(ESkill::kPunctual) / 5);
					else
						dam *= (ch->get_skill(ESkill::kPunctual) / 6);
					to_char = "вывело $N3 из строя";
					to_vict = "изуродовало вам ногу";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 50);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
			}
			break;
		case 5:        //  ABDOMINAL
			switch (dam_critic) {
				case 1:
				case 2:
				case 3:
					// nothing
					return;
				case 4:    // waits 1d6
					WAIT_STATE(victim, number(2, 6) * kPulseViolence);
					to_char = "сбило $N2 дыхание";
					to_vict = "сбило вам дыхание";
					break;

				case 5:    // abdomin damaged, waits 1, speed/2
					dam *= (ch->get_skill(ESkill::kPunctual) / 8);
					WAIT_STATE(victim, 2 * kPulseViolence);
					to_char = "ранило $N3 в живот";
					to_vict = "ранило вас в живот";
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 6:    // armor damaged else dam*3, waits 1d6
					WAIT_STATE(victim, number(2, 6) * kPulseViolence);
					if (GET_EQ(victim, WEAR_WAIST))
						alt_equip(victim, WEAR_WAIST, 100, 100);
					else
						dam *= (ch->get_skill(ESkill::kPunctual) / 7);
					to_char = "повредило $N2 живот";
					to_vict = "повредило вам живот";
					break;
				case 7:
				case 8:    // abdomin damage, speed/2, HR-2
					dam *= (ch->get_skill(ESkill::kPunctual) / 6);
					to_char = "ранило $N3 в живот";
					to_vict = "ранило вас в живот";
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 9:    // armor damaged, abdomin damaged, speed/2, HR-2
					dam *= (ch->get_skill(ESkill::kPunctual) / 5);
					alt_equip(victim, WEAR_BODY, 100, 100);
					to_char = "ранило $N3 в живот";
					to_vict = "ранило вас в живот";
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 20);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 10:    // abdomin damaged, no speed, no attack
					dam *= (ch->get_skill(ESkill::kPunctual) / 4);
					to_char = "повредило $N2 живот";
					to_vict = "повредило вам живот";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 20);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 11:    // abdomin damaged, no speed, no attack
					dam *= (ch->get_skill(ESkill::kPunctual) / 3);
					to_char = "разорвало $N2 живот";
					to_vict = "разорвало вам живот";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 40);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				default:    // abdomin damaged, hits = 0
					dam *= ch->get_skill(ESkill::kPunctual) / 2;
					to_char = "размозжило $N2 живот";
					to_vict = "размозжило вам живот";
					haemorragia(victim, 60);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
			}
			break;
		case 6:
		case 7:        // CHEST
			switch (dam_critic) {
				case 1:
				case 2:
				case 3:
					// nothing
					return;
				case 4:    // waits 1d4, bashed
					WAIT_STATE(victim, number(2, 5) * kPulseViolence);
					if (GET_POS(victim) > EPosition::kSit)
						GET_POS(victim) = EPosition::kSit;
					to_char = "повредило $N2 грудь, свалив $S с ног";
					to_vict = "повредило вам грудь, свалив вас с ног";
					break;
				case 5:    // chest damaged, waits 1, speed/2
					dam *= (ch->get_skill(ESkill::kPunctual) / 6);
					WAIT_STATE(victim, 2 * kPulseViolence);
					to_char = "повредило $N2 туловище";
					to_vict = "повредило вам туловище";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 6:    // shield damaged, chest damaged, speed/2
					alt_equip(victim, WEAR_SHIELD, 100, 100);
					dam *= (ch->get_skill(ESkill::kPunctual) / 6);
					to_char = "повредило $N2 туловище";
					to_vict = "повредило вам туловище";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 7:    // srmor damaged, chest damaged, speed/2, HR-2
					alt_equip(victim, WEAR_BODY, 100, 100);
					dam *= (ch->get_skill(ESkill::kPunctual) / 5);
					to_char = "повредило $N2 туловище";
					to_vict = "повредило вам туловище";
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 8:    // chest damaged, no speed, no attack
					dam *= (ch->get_skill(ESkill::kPunctual) / 5);
					to_char = "вывело $N3 из строя";
					to_vict = "повредило вам туловище";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 20);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 9:    // chest damaged, speed/2, HR-2
					dam *= (ch->get_skill(ESkill::kPunctual) / 4);
					to_char = "заставило $N3 ослабить натиск";
					to_vict = "сломало вам ребра";
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 20);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 10:    // chest damaged, no speed, no attack
					dam *= (ch->get_skill(ESkill::kPunctual) / 4);
					to_char = "вывело $N3 из строя";
					to_vict = "сломало вам ребра";
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 40);
					SET_AF_BATTLE(victim, kEafSlow);
					break;
				case 11:    // chest crushed, hits 0
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					dam *= ch->get_skill(ESkill::kPunctual) / 2;
					haemorragia(victim, 50);
					to_char = "вывело $N3 из строя";
					to_vict = "разорвало вам грудь";
					break;
				default:    // chest crushed, killing
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					dam *= ch->get_skill(ESkill::kPunctual) / 2;
					haemorragia(victim, 60);
					to_char = "вывело $N3 из строя";
					to_vict = "размозжило вам грудь";
					break;
			}
			break;
		case 8:
		case 9:        // HANDS
			switch (dam_critic) {
				case 1:
				case 2:
				case 3: return;
				case 4:    // hands damaged, weapon/shield putdown
					to_char = "ослабило натиск $N1";
					to_vict = "ранило вам руку";
					if (GET_EQ(victim, WEAR_BOTHS))
						unequip_pos = WEAR_BOTHS;
					else if (GET_EQ(victim, WEAR_WIELD))
						unequip_pos = WEAR_WIELD;
					else if (GET_EQ(victim, WEAR_HOLD))
						unequip_pos = WEAR_HOLD;
					else if (GET_EQ(victim, WEAR_SHIELD))
						unequip_pos = WEAR_SHIELD;
					break;
				case 5:    // hands damaged, shield damaged/weapon putdown
					to_char = "ослабило натиск $N1";
					to_vict = "ранило вас в руку";
					if (GET_EQ(victim, WEAR_SHIELD))
						alt_equip(victim, WEAR_SHIELD, 100, 100);
					else if (GET_EQ(victim, WEAR_BOTHS))
						unequip_pos = WEAR_BOTHS;
					else if (GET_EQ(victim, WEAR_WIELD))
						unequip_pos = WEAR_WIELD;
					else if (GET_EQ(victim, WEAR_HOLD))
						unequip_pos = WEAR_HOLD;
					break;

				case 6:    // hands damaged, HR-2, shield putdown
					to_char = "ослабило натиск $N1";
					to_vict = "сломало вам руку";
					if (GET_EQ(victim, WEAR_SHIELD))
						unequip_pos = WEAR_SHIELD;
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					break;
				case 7:    // armor damaged, hand damaged if no armour
					if (GET_EQ(victim, WEAR_ARMS))
						alt_equip(victim, WEAR_ARMS, 100, 100);
					else
						alt_equip(victim, WEAR_HANDS, 100, 100);
					if (!GET_EQ(victim, WEAR_ARMS) && !GET_EQ(victim, WEAR_HANDS))
						dam *= (ch->get_skill(ESkill::kPunctual) / 7);
					to_char = "ослабило атаку $N1";
					to_vict = "повредило вам руку";
					break;
				case 8:    // shield damaged, hands damaged, waits 1
					alt_equip(victim, WEAR_SHIELD, 100, 100);
					WAIT_STATE(victim, 2 * kPulseViolence);
					dam *= (ch->get_skill(ESkill::kPunctual) / 7);
					to_char = "придержало $N3";
					to_vict = "повредило вам руку";
					break;
				case 9:    // weapon putdown, hands damaged, waits 1d4
					WAIT_STATE(victim, number(2, 4) * kPulseViolence);
					if (GET_EQ(victim, WEAR_BOTHS))
						unequip_pos = WEAR_BOTHS;
					else if (GET_EQ(victim, WEAR_WIELD))
						unequip_pos = WEAR_WIELD;
					else if (GET_EQ(victim, WEAR_HOLD))
						unequip_pos = WEAR_HOLD;
					dam *= (ch->get_skill(ESkill::kPunctual) / 6);
					to_char = "придержало $N3";
					to_vict = "повредило вам руку";
					break;
				case 10:    // hand damaged, no attack this
					if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)) {
						to_char = "ослабило атаку $N1";
						to_vict = "изуродовало вам правую руку";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
						af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)) {
						to_char = "ослабило атаку $N1";
						to_vict = "изуродовало вам левую руку";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
						af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else {
						to_char = "вывело $N3 из строя";
						to_vict = "вывело вас из строя";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
						af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					}
					haemorragia(victim, 20);
					break;
				default:    // no hand attack, no speed, dam*2 if >= 13
					if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)) {
						to_char = "ослабило натиск $N1";
						to_vict = "изуродовало вам правую руку";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
						af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)) {
						to_char = "ослабило натиск $N1";
						to_vict = "изуродовало вам левую руку";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
						af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else {
						to_char = "вывело $N3 из строя";
						to_vict = "вывело вас из строя";
						af[0].type = kSpellBattle;
						af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
						af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					}
					af[1].type = kSpellBattle;
					af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					haemorragia(victim, 30);
					if (dam_critic >= 13)
						dam *= ch->get_skill(ESkill::kPunctual) / 5;
					SET_AF_BATTLE(victim, kEafSlow);
					break;
			}
			break;
		default:        // HEAD
			switch (dam_critic) {
				case 1:
				case 2:
				case 3:
					// nothing
					return;
				case 4:    // waits 1d6
					WAIT_STATE(victim, number(2, 6) * kPulseViolence);
					to_char = "помутило $N2 сознание";
					to_vict = "помутило ваше сознание";
					break;

				case 5:    // head damaged, cap putdown, waits 1, HR-2 if no cap
					WAIT_STATE(victim, 2 * kPulseViolence);
					if (GET_EQ(victim, WEAR_HEAD))
						unequip_pos = WEAR_HEAD;
					else {
						af[0].type = kSpellBattle;
						af[0].location = APPLY_HITROLL;
						af[0].modifier = -2;
					}
					dam *= (ch->get_skill(ESkill::kPunctual) / 4);
					to_char = "повредило $N2 голову";
					to_vict = "повредило вам голову";
					break;
				case 6:    // head damaged
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -2;
					dam *= (ch->get_skill(ESkill::kPunctual) / 4);
					to_char = "повредило $N2 голову";
					to_vict = "повредило вам голову";
					break;
				case 7:    // cap damaged, waits 1d6, speed/2, HR-4
					WAIT_STATE(victim, 2 * kPulseViolence);
					alt_equip(victim, WEAR_HEAD, 100, 100);
					af[0].type = kSpellBattle;
					af[0].location = APPLY_HITROLL;
					af[0].modifier = -4;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
					to_char = "ранило $N3 в голову";
					to_vict = "ранило вас в голову";
					break;
				case 8:    // cap damaged, hits 0
					WAIT_STATE(victim, 4 * kPulseViolence);
					alt_equip(victim, WEAR_HEAD, 100, 100);
					//dam = GET_HIT(victim);
					dam *= ch->get_skill(ESkill::kPunctual) / 2;
					to_char = "отбило у $N1 сознание";
					to_vict = "отбило у вас сознание";
					haemorragia(victim, 20);
					break;
				case 9:    // head damaged, no speed, no attack
					af[0].type = kSpellBattle;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[0].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					haemorragia(victim, 30);
					dam *= (ch->get_skill(ESkill::kPunctual) / 3);
					to_char = "повергло $N3 в оцепенение";
					to_vict = "повергло вас в оцепенение";
					break;
				case 10:    // head damaged, -1 INT/WIS/CHA
					dam *= (ch->get_skill(ESkill::kPunctual) / 2);
					af[0].type = kSpellBattle;
					af[0].location = APPLY_INT;
					af[0].modifier = -1;
					af[0].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[0].battleflag = kAfDeadkeep;
					af[1].type = kSpellBattle;
					af[1].location = APPLY_WIS;
					af[1].modifier = -1;
					af[1].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[1].battleflag = kAfDeadkeep;
					af[2].type = kSpellBattle;
					af[2].location = APPLY_CHA;
					af[2].modifier = -1;
					af[2].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[2].battleflag = kAfDeadkeep;
					af[3].type = kSpellBattle;
					af[3].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
					af[3].duration = CalcDuration(victim, 30, 0, 0, 0, 0);
					af[3].battleflag = kAfBattledec | kAfPulsedec;
					haemorragia(victim, 50);
					to_char = "сорвало у $N1 крышу";
					to_vict = "сорвало у вас крышу";
					break;
				case 11:    // hits 0, WIS/2, INT/2, CHA/2
					dam *= ch->get_skill(ESkill::kPunctual) / 2;
					af[0].type = kSpellBattle;
					af[0].location = APPLY_INT;
					af[0].modifier = -victim->get_int() / 2;
					af[0].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[0].battleflag = kAfDeadkeep;
					af[1].type = kSpellBattle;
					af[1].location = APPLY_WIS;
					af[1].modifier = -victim->get_wis() / 2;
					af[1].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[1].battleflag = kAfDeadkeep;
					af[2].type = kSpellBattle;
					af[2].location = APPLY_CHA;
					af[2].modifier = -victim->get_cha() / 2;
					af[2].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[2].battleflag = kAfDeadkeep;
					haemorragia(victim, 60);
					to_char = "сорвало у $N1 крышу";
					to_vict = "сорвало у вас крышу";
					break;
				default:    // killed
					af[0].type = kSpellBattle;
					af[0].location = APPLY_INT;
					af[0].modifier = -victim->get_int() / 2;
					af[0].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[0].battleflag = kAfDeadkeep;
					af[1].type = kSpellBattle;
					af[1].location = APPLY_WIS;
					af[1].modifier = -victim->get_wis() / 2;
					af[1].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[1].battleflag = kAfDeadkeep;
					af[2].type = kSpellBattle;
					af[2].location = APPLY_CHA;
					af[2].modifier = -victim->get_cha() / 2;
					af[2].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[2].battleflag = kAfDeadkeep;
					dam *= ch->get_skill(ESkill::kPunctual) / 2;
					to_char = "размозжило $N2 голову";
					to_vict = "размозжило вам голову";
					haemorragia(victim, 90);
					break;
			}
			break;
	}
	if (to_char) {
		sprintf(buf, "&G&qВаше точное попадание %s.&Q&n", to_char);
		act(buf, false, ch, 0, victim, kToChar);
		sprintf(buf, "Точное попадание $n1 %s.", to_char);
		act(buf, true, ch, 0, victim, kToNotVict | kToArenaListen);
	}

	if (to_vict) {
		sprintf(buf, "&R&qМеткое попадание $n1 %s.&Q&n", to_vict);
		act(buf, false, ch, 0, victim, kToVict);
	}
	if (unequip_pos && GET_EQ(victim, unequip_pos)) {
		obj = unequip_char(victim, unequip_pos, CharEquipFlags());
		switch (unequip_pos) {
			case 6:        //WEAR_HEAD
				sprintf(buf, "%s слетел%s с вашей головы.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToVict);
				sprintf(buf, "%s слетел%s с головы $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToChar);
				act(buf, true, ch, 0, victim, kToNotVict | kToArenaListen);
				break;

			case 11:    //WEAR_SHIELD
				sprintf(buf, "%s слетел%s с вашей руки.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToVict);
				sprintf(buf, "%s слетел%s с руки $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToChar);
				act(buf, true, ch, 0, victim, kToNotVict | kToArenaListen);
				break;

			case 16:    //WEAR_WIELD
			case 17:    //WEAR_HOLD
				sprintf(buf, "%s выпал%s из вашей руки.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToVict);
				sprintf(buf, "%s выпал%s из руки $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToChar);
				act(buf, true, ch, 0, victim, kToNotVict | kToArenaListen);
				break;

			case 18:    //WEAR_BOTHS
				sprintf(buf, "%s выпал%s из ваших рук.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToVict);
				sprintf(buf, "%s выпал%s из рук $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, 0, victim, kToChar);
				act(buf, true, ch, 0, victim, kToNotVict | kToArenaListen);
				break;
		}
		if (!IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
			obj_to_char(obj, victim);
		else
			obj_to_room(obj, IN_ROOM(victim));
		obj_decay(obj);
	}
	if (!IS_NPC(victim)) {
		dam /= 5;
	}
	dam = ApplyResist(victim, VITALITY_RESISTANCE, dam);
	for (int i = 0; i < 4; i++) {
		if (af[i].type) {
			if (af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPFIGHT)
				|| af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPRIGHT)
				|| af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPLEFT)) {
				if (victim->get_role(MOB_ROLE_BOSS)) {
					af[i].duration /= 5;
					// вес оружия тоже влияет на длит точки, офф проходит реже, берем вес прайма.
					sh_int extra_duration = 0;
					ObjData *both = GET_EQ(ch, WEAR_BOTHS);
					ObjData *wield = GET_EQ(ch, WEAR_WIELD);
					if (both) {
						extra_duration = GET_OBJ_WEIGHT(both) / 5;
					} else if (wield) {
						extra_duration = GET_OBJ_WEIGHT(wield) / 5;
					}
					af[i].duration += CalcDuration(victim, GET_REAL_REMORT(ch) / 2 + extra_duration, 0, 0, 0, 0);
				}
			}
			affect_join(victim, af[i], true, false, true, false);
		}
	}
}

/**
* Расчет множителя дамаги пушки с концентрацией силы.
* Текущая: 1 + ((сила-25)*0.4 + левел*0.2)/10 * ремы/5,
* в цифрах множитель выглядит от 1 до 2.6 с равномерным
* распределением 62.5% на силу и 37.5% на уровни + штрафы до 5 ремов.
* Способность не плюсуется при железном ветре и оглушении.
*/
int calculate_strconc_damage(CharData *ch, ObjData * /*wielded*/, int damage) {
	if (IS_NPC(ch)
		|| GET_REAL_STR(ch) <= 25
		|| !can_use_feat(ch, STRENGTH_CONCETRATION_FEAT)
		|| GET_AF_BATTLE(ch, kEafIronWind)
		|| GET_AF_BATTLE(ch, kEafOverwhelm)) {
		return damage;
	}
	float str_mod = (GET_REAL_STR(ch) - 25) * 0.4;
	float lvl_mod = GetRealLevel(ch) * 0.2;
	float rmt_mod = MIN(5, GET_REAL_REMORT(ch)) / 5.0;
	float res_mod = 1 + (str_mod + lvl_mod) / 10.0 * rmt_mod;

	return static_cast<int>(damage * res_mod);
}

/**
* Расчет прибавки дамаги со скрытого стиля.
* (скилл/5 + реморты*3) * (среднее/(10 + среднее/2)) * (левел/30)
*/
int calculate_noparryhit_dmg(CharData *ch, ObjData *wielded) {
	if (!ch->get_skill(ESkill::kNoParryHit)) return 0;

	float weap_dmg = (((GET_OBJ_VAL(wielded, 2) + 1) / 2.0) * GET_OBJ_VAL(wielded, 1));
	float weap_mod = weap_dmg / (10 + weap_dmg / 2);
	float level_mod = static_cast<float>(GetRealLevel(ch)) / 30;
	float skill_mod = static_cast<float>(ch->get_skill(ESkill::kNoParryHit)) / 5;

	return static_cast<int>((skill_mod + GET_REAL_REMORT(ch) * 3) * weap_mod * level_mod);
}

void might_hit_bash(CharData *ch, CharData *victim) {
	if (MOB_FLAGGED(victim, MOB_NOBASH) || !GET_MOB_HOLD(victim)) {
		return;
	}

	act("$n обреченно повалил$u на землю.", true, victim, 0, 0, kToRoom | kToArenaListen);
	WAIT_STATE(victim, 3 * kPulseViolence);

	if (GET_POS(victim) > EPosition::kSit) {
		GET_POS(victim) = EPosition::kSit;
		send_to_char(victim, "&R&qБогатырский удар %s сбил вас с ног.&Q&n\r\n", PERS(ch, victim, 1));
	}
}

bool check_mighthit_weapon(CharData *ch) {
	if (!GET_EQ(ch, WEAR_BOTHS)
		&& !GET_EQ(ch, WEAR_WIELD)
		&& !GET_EQ(ch, WEAR_HOLD)
		&& !GET_EQ(ch, WEAR_LIGHT)
		&& !GET_EQ(ch, WEAR_SHIELD)) {
		return true;
	}
	return false;
}

// * При надуве выше х 1.5 в пк есть 1% того, что весь надув слетит одним ударом.
void try_remove_extrahits(CharData *ch, CharData *victim) {
	if (((!IS_NPC(ch) && ch != victim)
		|| (ch->has_master()
			&& !IS_NPC(ch->get_master())
			&& ch->get_master() != victim))
		&& !IS_NPC(victim)
		&& GET_POS(victim) != EPosition::kDead
		&& GET_HIT(victim) > GET_REAL_MAX_HIT(victim) * 1.5
		&& number(1, 100) == 5)// пусть будет 5, а то 1 по субъективным ощущениям выпадает как-то часто
	{
		GET_HIT(victim) = GET_REAL_MAX_HIT(victim);
		send_to_char(victim, "%s'Будь%s тощ%s аки прежде' - мелькнула чужая мысль в вашей голове.%s\r\n",
					 CCWHT(victim, C_NRM), GET_CH_POLY_1(victim), GET_CH_EXSUF_1(victim), CCNRM(victim, C_NRM));
		act("Вы прервали золотистую нить, питающую $N3 жизнью.", false, ch, 0, victim, kToChar);
		act("$n прервал$g золотистую нить, питающую $N3 жизнью.", false, ch, 0, victim, kToNotVict | kToArenaListen);
	}
}

/**
* Дамажим дополнительным выстрелом.
* Проценты при 100 скилле и 21 ловки: 100  60 30 10
* Проценты при 200 скилее и 50 ловки: 100 100 70 30
* На заднице 70% от итоговых процентов.
* У чармисов 100% и 60% без остальных допов.
*/
void addshot_damage(CharData *ch, ESkill type, fight::AttackType weapon) {
	int prob = CalcCurrentSkill(ch, ESkill::kAddshot, ch->get_fighting());
	TrainSkill(ch, ESkill::kAddshot, true, ch->get_fighting());
	// ловка роляет только выше 21 (стартовый максимум охота) и до 50
	// поставим кап в 50 ловки для допвыстрела
	float dex_mod = static_cast<float>(MAX(MIN(GET_REAL_DEX(ch), 50) - 21, 0)) / 29 * 2;
	// штраф на 4й и 5й выстрелы для чаров ниже 5 мортов, по 20% за морт
	float remort_mod = static_cast<float>(GET_REAL_REMORT(ch)) / 5;
	if (remort_mod > 1) {
		remort_mod = 1;
	}
	// на жопе процентовка снижается на 70% от текущего максимума каждого допа
	float sit_mod = (GET_POS(ch) >= EPosition::kFight) ? 1 : 0.7;

	// у чармисов никаких плюшек от 100+ скилла и максимум 2 доп атаки
	if (IS_CHARMICE(ch)) {
		prob = MIN(100, prob);
		dex_mod = 0;
		remort_mod = 0;
	}

	// выше 100% идет другая формула
	// скилл выше 100 добавляет равный ловке процент в максимуме
	// а для скилла до сотни все остается как было
	prob = std::min(100, prob);

	auto difficulty = MUD::Skills()[ESkill::kAddshot].difficulty;
	int percent = number(1, difficulty);
	// 1й доп - не более 100% при скилее 100+
	if (prob * sit_mod >= percent / 2)
		hit(ch, ch->get_fighting(), type, weapon);
	percent = number(1, difficulty);
	// 2й доп - 60% при скилле 100, до 100% при максимуме скилла и дексы
	if ((prob * 3 + dex_mod * 100) * sit_mod > percent * 5 / 2 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);
	percent = number(1, difficulty);
	// 3й доп - 30% при скилле 100, до 70% при максимуме скилла и дексы (при 5+ мортов)
	if ((prob * 3 + dex_mod * 200) * remort_mod * sit_mod > percent * 5 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);
	percent = number(1, difficulty);
	// 4й доп - 20% при скилле 100, до 60% при максимуме скилла и дексы (при 5+ мортов)
	if ((prob + dex_mod * 100) * remort_mod * sit_mod > percent * 5 / 2 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);
}

// бонусы/штрафы классам за юзание определенных видов оружия
void apply_weapon_bonus(int ch_class, const ESkill skill, int *damroll, int *hitroll) {
	int dam = *damroll;
	int calc_thaco = *hitroll;

	switch (ch_class) {
		case ECharClass::kSorcerer:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 2;
					dam -= 1;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco += 1;
					dam -= 2;
					break;
				case ESkill::kTwohands: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;

				default: break;
			}
			break;

		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco += 1;
					dam -= 3;
					break;
				case ESkill::kPicks: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kWarrior:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco += 2;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco += 2;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kRanger:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kGuard:
		case ECharClass::kThief:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kAssasine:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;
				default: break;
			}
			break;
			/*	case ECharClass::kClassPaladine:
			case ECharClass::kClassSmith:
				switch (skill) {
					case ESkill::kClubs:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kAxes:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kLongBlades:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kShortBlades:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kNonstandart:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kTwohands:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kPicks:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kSpades:	calc_thaco -= 0; dam += 0; break;
					case ESkill::kBows:	calc_thaco -= 0; dam += 0; break;
				}
				break; */
		case ECharClass::kMerchant:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco -= 0;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kMagus:
			switch (skill) {
				case ESkill::kClubs: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kAxes: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kLongBlades: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kShortBlades: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kNonstandart: calc_thaco -= 0;
					dam += 0;
					break;
				case ESkill::kTwohands: calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kPicks: calc_thaco += 0;
					dam += 0;
					break;
				case ESkill::kSpades: calc_thaco += 0;
					dam += 0;
					break;
				case ESkill::kBows: calc_thaco += 1;
					dam += 0;
					break;
				default: break;
			}
			break;
	}

	*damroll = dam;
	*hitroll = calc_thaco;
}

int do_punctual(CharData *ch, CharData * /*victim*/, ObjData *wielded) {
	int dam_critic = 0, wapp = 0;

	if (wielded) {
		wapp = (int) ((static_cast<ESkill>GET_OBJ_SKILL(wielded) == ESkill::kBows) && GET_EQ(ch, WEAR_BOTHS)) ?
			GET_OBJ_WEIGHT(wielded) * 1 / 3 : GET_OBJ_WEIGHT(wielded);
	}
	if (wapp < 10)
		dam_critic = RollDices(1, 6);
	else if (wapp < 19)
		dam_critic = RollDices(2, 5);
	else if (wapp < 27)
		dam_critic = RollDices(3, 4);
	else if (wapp < 36)
		dam_critic = RollDices(3, 5);
	else if (wapp < 44)
		dam_critic = RollDices(3, 6);
	else
		dam_critic = RollDices(4, 5);

	const int skill = 1 + ch->get_skill(ESkill::kPunctual) / 6;
	dam_critic = MIN(number(4, skill), dam_critic);

	return dam_critic;
}

// * Умножение дамаги при стабе.
int backstab_mult(int level) {
	if (level <= 0)
		return 1;    // level 0 //
	else if (level <= 5)
		return 2;    // level 1 - 5 //
	else if (level <= 10)
		return 3;    // level 6 - 10 //
	else if (level <= 15)
		return 4;    // level 11 - 15 //
	else if (level <= 20)
		return 5;    // level 16 - 20 //
	else if (level <= 25)
		return 6;    // level 21 - 25 //
	else if (level <= 30)
		return 7;    // level 26 - 30 //
	else
		return 10;
}

/**
* Процент прохождения крит.стаба = скилл/11 + (декса-20)/(декса/30)
* Влияение по 50% от скилла и дексы, максимум 36,18%.
*/
int calculate_crit_backstab_percent(CharData *ch) {
	return static_cast<int>(ch->get_skill(ESkill::kBackstab) / 11.0 + (GET_REAL_DEX(ch) - 20) / (GET_REAL_DEX(ch) / 30.0));
}

/*
 *  Расчет множителя критстаба.
 */
double HitData::crit_backstab_multiplier(CharData *ch, CharData *victim) {
	double bs_coeff = 1.0;
	if (IS_NPC(victim)) {
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
			bs_coeff *= ch->get_skill(ESkill::kBackstab) / 15.0;
		} else {
			bs_coeff *= ch->get_skill(ESkill::kBackstab) / 25.0;
		}
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && (ch->get_skill(ESkill::kNoParryHit))) {
			bs_coeff *= (1 + (ch->get_skill(ESkill::kNoParryHit) * 0.00125));
		}
	} else if (can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
		if (victim->get_fighting())
		{
			bs_coeff *= (1.0 + (ch->get_skill(ESkill::kBackstab) * 0.00225));
		} else {
			bs_coeff *= (1.0 + (ch->get_skill(ESkill::kBackstab) * 0.00350));
		}
		set_flag(fight::kIgnoreSanct);
		set_flag(fight::kIgnorePrism);
	}

	return std::max(1.0, bs_coeff);
}

// * Может ли персонаж блокировать атаки автоматом (вообще в данный момент, без учета лагов).
bool can_auto_block(CharData *ch) {
	if (GET_EQ(ch, WEAR_SHIELD) && GET_AF_BATTLE(ch, kEafAwake) && GET_AF_BATTLE(ch, kEafAutoblock))
		return true;
	else
		return false;
}

// * Проверка на фит "любимое оружие".
void HitData::CheckWeapFeats(const CharData *ch, ESkill weap_skill, int &calc_thaco, int &dam) {
	switch (weap_skill) {
		case ESkill::kPunch:
			if (HAVE_FEAT(ch, PUNCH_FOCUS_FEAT)) {
				calc_thaco -= 2;
				dam += 2;
			}
			break;
		case ESkill::kClubs:
			if (HAVE_FEAT(ch, CLUB_FOCUS_FEAT)) {
				calc_thaco -= 2;
				dam += 2;
			}
			break;
		case ESkill::kAxes:
			if (HAVE_FEAT(ch, AXES_FOCUS_FEAT)) {
				calc_thaco -= 1;
				dam += 2;
			}
			break;
		case ESkill::kLongBlades:
			if (HAVE_FEAT(ch, LONGS_FOCUS_FEAT)) {
				calc_thaco -= 1;
				dam += 2;
			}
			break;
		case ESkill::kShortBlades:
			if (HAVE_FEAT(ch, SHORTS_FOCUS_FEAT)) {
				calc_thaco -= 2;
				dam += 3;
			}
			break;
		case ESkill::kNonstandart:
			if (HAVE_FEAT(ch, NONSTANDART_FOCUS_FEAT)) {
				calc_thaco -= 1;
				dam += 3;
			}
			break;
		case ESkill::kTwohands:
			if (HAVE_FEAT(ch, BOTHHANDS_FOCUS_FEAT)) {
				calc_thaco -= 1;
				dam += 3;
			}
			break;
		case ESkill::kPicks:
			if (HAVE_FEAT(ch, PICK_FOCUS_FEAT)) {
				calc_thaco -= 2;
				dam += 3;
			}
			break;
		case ESkill::kSpades:
			if (HAVE_FEAT(ch, SPADES_FOCUS_FEAT)) {
				calc_thaco -= 1;
				dam += 2;
			}
			break;
		case ESkill::kBows:
			if (HAVE_FEAT(ch, BOWS_FOCUS_FEAT)) {
				calc_thaco -= 7;
				dam += 4;
			}
			break;
		default: break;
	}
}

// * Захват.
void hit_touching(CharData *ch, CharData *vict, int *dam) {
	if (vict->get_touching() == ch
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPRIGHT)
		&& GET_WAIT(vict) <= 0
		&& !GET_MOB_HOLD(vict)
		&& (IS_IMMORTAL(vict) || IS_NPC(vict)
			|| !(GET_EQ(vict, WEAR_WIELD) || GET_EQ(vict, WEAR_BOTHS)))
		&& GET_POS(vict) > EPosition::kSleep) {
		int percent = number(1, MUD::Skills()[ESkill::kIntercept].difficulty);
		int prob = CalcCurrentSkill(vict, ESkill::kIntercept, ch);
		TrainSkill(vict, ESkill::kIntercept, prob >= percent, ch);
		SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kIntercept].name, percent, prob, prob >= 70);
		if (IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE)) {
			percent = prob;
		}
		if (GET_GOD_FLAG(vict, GF_GODSCURSE)) {
			percent = 0;
		}
		CLR_AF_BATTLE(vict, kEafTouch);
		SET_AF_BATTLE(vict, kEafUsedright);
		vict->set_touching(0);
		if (prob < percent) {
			act("Вы не смогли перехватить атаку $N1.", false, vict, 0, ch, kToChar);
			act("$N не смог$Q перехватить вашу атаку.", false, ch, 0, vict, kToChar);
			act("$n не смог$q перехватить атаку $N1.", true, vict, 0, ch, kToNotVict | kToArenaListen);
			prob = 2;
		} else {
			act("Вы перехватили атаку $N1.", false, vict, 0, ch, kToChar);
			act("$N перехватил$G вашу атаку.", false, ch, 0, vict, kToChar);
			act("$n перехватил$g атаку $N1.", true, vict, 0, ch, kToNotVict | kToArenaListen);
			*dam = -1;
			prob = 1;
		}
		SetSkillCooldownInFight(vict, ESkill::kGlobalCooldown, 1);
		SetSkillCooldownInFight(vict, ESkill::kIntercept, prob);
/*
		if (!WAITLESS(vict)) {
			WAIT_STATE(vict, prob * kPulseViolence);
		}
*/
	}
}

void hit_deviate(CharData *ch, CharData *victim, int *dam) {
	int range = number(1, MUD::Skills()[ESkill::kDodge].difficulty);
	int prob = CalcCurrentSkill(victim, ESkill::kDodge, ch);
	if (GET_GOD_FLAG(victim, GF_GODSCURSE)) {
		prob = 0;
	}
	prob = prob * 100 / range;
	if (check_spell_on_player(victim, kSpellWeb)) {
		prob /= 3;
	}
	TrainSkill(victim, ESkill::kDodge, prob < 100, ch);
	if (prob < 60) {
		act("Вы не смогли уклониться от атаки $N1", false, victim, 0, ch, kToChar);
		act("$N не сумел$G уклониться от вашей атаки", false, ch, 0, victim, kToChar);
		act("$n не сумел$g уклониться от атаки $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
		SET_AF_BATTLE(victim, kEafDodge);
	} else if (prob < 100) {
		act("Вы немного уклонились от атаки $N1", false, victim, 0, ch, kToChar);
		act("$N немного уклонил$U от вашей атаки", false, ch, 0, victim, kToChar);
		act("$n немного уклонил$u от атаки $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
		*dam = *dam * 10 / 15;
		SET_AF_BATTLE(victim, kEafDodge);
	} else if (prob < 200) {
		act("Вы частично уклонились от атаки $N1", false, victim, 0, ch, kToChar);
		act("$N частично уклонил$U от вашей атаки", false, ch, 0, victim, kToChar);
		act("$n частично уклонил$u от атаки $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
		*dam = *dam / 2;
		SET_AF_BATTLE(victim, kEafDodge);
	} else {
		act("Вы уклонились от атаки $N1", false, victim, 0, ch, kToChar);
		act("$N уклонил$U от вашей атаки", false, ch, 0, victim, kToChar);
		act("$n уклонил$u от атаки $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
		*dam = -1;
		SET_AF_BATTLE(victim, kEafDodge);
	}
	BATTLECNTR(victim)++;
}

void hit_parry(CharData *ch, CharData *victim, ESkill skill, int hit_type, int *dam) {
	if (!((GET_EQ(victim, WEAR_WIELD)
		&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) == ObjData::ITEM_WEAPON
		&& GET_EQ(victim, WEAR_HOLD)
		&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) == ObjData::ITEM_WEAPON)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim))) {
		send_to_char("У вас нечем отклонить атаку противника\r\n", victim);
		CLR_AF_BATTLE(victim, kEafParry);
	} else {
		int range = number(1, MUD::Skills()[ESkill::kParry].difficulty);
		int prob = CalcCurrentSkill(victim, ESkill::kParry, ch);
		prob = prob * 100 / range;
		TrainSkill(victim, ESkill::kParry, prob < 100, ch);
		SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kParry].name, range, prob, prob >= 70);
		if (prob < 70
			|| ((skill == ESkill::kBows || hit_type == fight::type_maul)
				&& !IS_IMMORTAL(victim)
				&& (!can_use_feat(victim, PARRY_ARROW_FEAT)
					|| number(1, 1000) >= 20 * MIN(GET_REAL_DEX(victim), 35)))) {
			act("Вы не смогли отбить атаку $N1", false, victim, 0, ch, kToChar);
			act("$N не сумел$G отбить вашу атаку", false, ch, 0, victim, kToChar);
			act("$n не сумел$g отбить атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			prob = 2;
			SET_AF_BATTLE(victim, kEafUsedleft);
		} else if (prob < 100) {
			act("Вы немного отклонили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N немного отклонил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n немного отклонил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 10);
			prob = 1;
			*dam = *dam * 10 / 15;
			SET_AF_BATTLE(victim, kEafUsedleft);
		} else if (prob < 170) {
			act("Вы частично отклонили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N частично отклонил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n частично отклонил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 15);
			prob = 0;
			*dam = *dam / 2;
			SET_AF_BATTLE(victim, kEafUsedleft);
		} else {
			act("Вы полностью отклонили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N полностью отклонил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n полностью отклонил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 25);
			prob = 0;
			*dam = -1;
		}
		if (prob > 0)
			SetSkillCooldownInFight(victim, ESkill::kGlobalCooldown, 1);
		SetSkillCooldownInFight(victim, ESkill::kParry, prob);
/*
		if (!WAITLESS(ch) && prob) {
			WAIT_STATE(victim, kPulseViolence * prob);
		}
*/
		CLR_AF_BATTLE(victim, kEafParry);
	}
}

void hit_multyparry(CharData *ch, CharData *victim, ESkill skill, int hit_type, int *dam) {
	if (!((GET_EQ(victim, WEAR_WIELD)
		&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) == ObjData::ITEM_WEAPON
		&& GET_EQ(victim, WEAR_HOLD)
		&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) == ObjData::ITEM_WEAPON)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim))) {
		send_to_char("У вас нечем отклонять атаки противников\r\n", victim);
	} else {
		int range = number(1,
						   MUD::Skills()[ESkill::kMultiparry].difficulty) + 15 * BATTLECNTR(victim);
		int prob = CalcCurrentSkill(victim, ESkill::kMultiparry, ch);
		prob = prob * 100 / range;

		if ((skill == ESkill::kBows || hit_type == fight::type_maul)
			&& !IS_IMMORTAL(victim)
			&& (!can_use_feat(victim, PARRY_ARROW_FEAT)
				|| number(1, 1000) >= 20 * MIN(GET_REAL_DEX(victim), 35))) {
			prob = 0;
		} else {
			BATTLECNTR(victim)++;
		}

		TrainSkill(victim, ESkill::kMultiparry, prob >= 50, ch);
		SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kMultiparry].name, range, prob, prob >= 50);
		if (prob < 50) {
			act("Вы не смогли отбить атаку $N1", false, victim, 0, ch, kToChar);
			act("$N не сумел$G отбить вашу атаку", false, ch, 0, victim, kToChar);
			act("$n не сумел$g отбить атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
		} else if (prob < 90) {
			act("Вы немного отклонили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N немного отклонил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n немного отклонил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 10);
			*dam = *dam * 10 / 15;
		} else if (prob < 180) {
			act("Вы частично отклонили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N частично отклонил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n частично отклонил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 15);
			*dam = *dam / 2;
		} else {
			act("Вы полностью отклонили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N полностью отклонил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n полностью отклонил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 25);
			*dam = -1;
		}
	}
}

void hit_block(CharData *ch, CharData *victim, int *dam) {
	if (!(GET_EQ(victim, WEAR_SHIELD)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim))) {
		send_to_char("У вас нечем отразить атаку противника\r\n", victim);
	} else {
		int range = number(1, MUD::Skills()[ESkill::kShieldBlock].difficulty);
		int prob = CalcCurrentSkill(victim, ESkill::kShieldBlock, ch);
		prob = prob * 100 / range;
		BATTLECNTR(victim)++;
		TrainSkill(victim, ESkill::kShieldBlock, prob > 99, ch);
		SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kShieldBlock].name, range, prob, prob > 99);
		if (prob < 100) {
			act("Вы не смогли отразить атаку $N1", false, victim, 0, ch, kToChar);
			act("$N не сумел$G отразить вашу атаку", false, ch, 0, victim, kToChar);
			act("$n не сумел$g отразить атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
		} else if (prob < 150) {
			act("Вы немного отразили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N немного отразил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n немного отразил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, WEAR_SHIELD, *dam, 10);
			*dam = *dam * 10 / 15;
		} else if (prob < 250) {
			act("Вы частично отразили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N частично отразил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n частично отразил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, WEAR_SHIELD, *dam, 15);
			*dam = *dam / 2;
		} else {
			act("Вы полностью отразили атаку $N1", false, victim, 0, ch, kToChar);
			act("$N полностью отразил$G вашу атаку", false, ch, 0, victim, kToChar);
			act("$n полностью отразил$g атаку $N1", true, victim, 0, ch, kToNotVict | kToArenaListen);
			alt_equip(victim, WEAR_SHIELD, *dam, 25);
			*dam = -1;
		}
	}
}

void appear(CharData *ch) {
	const bool appear_msg = AFF_FLAGGED(ch, EAffectFlag::AFF_INVISIBLE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE);

	if (affected_by_spell(ch, kSpellInvisible))
		affect_from_char(ch, kSpellInvisible);
	if (affected_by_spell(ch, kSpellHide))
		affect_from_char(ch, kSpellHide);
	if (affected_by_spell(ch, kSpellSneak))
		affect_from_char(ch, kSpellSneak);
	if (affected_by_spell(ch, kSpellCamouflage))
		affect_from_char(ch, kSpellCamouflage);

	AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);

	if (appear_msg) {
		if (IS_NPC(ch)
			|| GetRealLevel(ch) < kLvlImmortal) {
			act("$n медленно появил$u из пустоты.", false, ch, 0, 0, kToRoom);
		} else {
			act("Вы почувствовали странное присутствие $n1.", false, ch, 0, 0, kToRoom);
		}
	}
}

// message for doing damage with a weapon
void Damage::dam_message(CharData *ch, CharData *victim) const {
	int dam_msgnum;
	int w_type = msg_num;

	static const struct dam_weapon_type {
		const char *to_room;
		const char *to_char;
		const char *to_victim;
	} dam_weapons[] =
		{

			// use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes")

			{
				"$n попытал$u #W $N3, но промахнул$u.",    // 0: 0      0 //
				"Вы попытались #W $N3, но промахнулись.",
				"$n попытал$u #W вас, но промахнул$u."
			}, {
				"$n легонько #w$g $N3.",    //  1..5 1 //
				"Вы легонько #wи $N3.",
				"$n легонько #w$g вас."
			}, {
				"$n слегка #w$g $N3.",    //  6..11  2 //
				"Вы слегка #wи $N3.",
				"$n слегка #w$g вас."
			}, {
				"$n #w$g $N3.",    //  12..18   3 //
				"Вы #wи $N3.",
				"$n #w$g вас."
			}, {
				"$n #w$g $N3.",    // 19..26  4 //
				"Вы #wи $N3.",
				"$n #w$g вас."
			}, {
				"$n сильно #w$g $N3.",    // 27..35  5 //
				"Вы сильно #wи $N3.",
				"$n сильно #w$g вас."
			}, {
				"$n очень сильно #w$g $N3.",    //  36..45 6  //
				"Вы очень сильно #wи $N3.",
				"$n очень сильно #w$g вас."
			}, {
				"$n чрезвычайно сильно #w$g $N3.",    //  46..55  7 //
				"Вы чрезвычайно сильно #wи $N3.",
				"$n чрезвычайно сильно #w$g вас."
			}, {
				"$n БОЛЬНО #w$g $N3.",    //  56..96   8 //
				"Вы БОЛЬНО #wи $N3.",
				"$n БОЛЬНО #w$g вас."
			}, {
				"$n ОЧЕНЬ БОЛЬНО #w$g $N3.",    //    97..136  9  //
				"Вы ОЧЕНЬ БОЛЬНО #wи $N3.",
				"$n ОЧЕНЬ БОЛЬНО #w$g вас."
			}, {
				"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g $N3.",    //   137..176  10 //
				"Вы ЧРЕЗВЫЧАЙНО БОЛЬНО #wи $N3.",
				"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g вас."
			}, {
				"$n НЕВЫНОСИМО БОЛЬНО #w$g $N3.",    //    177..216  11 //
				"Вы НЕВЫНОСИМО БОЛЬНО #wи $N3.",
				"$n НЕВЫНОСИМО БОЛЬНО #w$g вас."
			}, {
				"$n ЖЕСТОКО #w$g $N3.",    //    217..256  13 //
				"Вы ЖЕСТОКО #wи $N3.",
				"$n ЖЕСТОКО #w$g вас."
			}, {
				"$n УЖАСНО #w$g $N3.",    //    257..296  13 //
				"Вы УЖАСНО #wи $N3.",
				"$n УЖАСНО #w$g вас."
			}, {
				"$n УБИЙСТВЕННО #w$g $N3.",    //    297..400  15 //
				"Вы УБИЙСТВЕННО #wи $N3.",
				"$n УБИЙСТВЕННО #w$g вас."
			}, {
				"$n ИЗУВЕРСКИ #w$g $N3.",    //    297..400  15 //
				"Вы ИЗУВЕРСКИ #wи $N3.",
				"$n ИЗУВЕРСКИ #w$g вас."
			}, {
				"$n СМЕРТЕЛЬНО #w$g $N3.",    // 400+  16 //
				"Вы СМЕРТЕЛЬНО #wи $N3.",
				"$n СМЕРТЕЛЬНО #w$g вас."
			}
		};

	if (w_type >= kTypeHit && w_type < kTypeMagic)
		w_type -= kTypeHit;    // Change to base of table with text //
	else
		w_type = kTypeHit;

	if (dam == 0)
		dam_msgnum = 0;
	else if (dam <= 5)
		dam_msgnum = 1;
	else if (dam <= 11)
		dam_msgnum = 2;
	else if (dam <= 18)
		dam_msgnum = 3;
	else if (dam <= 26)
		dam_msgnum = 4;
	else if (dam <= 35)
		dam_msgnum = 5;
	else if (dam <= 45)
		dam_msgnum = 6;
	else if (dam <= 56)
		dam_msgnum = 7;
	else if (dam <= 96)
		dam_msgnum = 8;
	else if (dam <= 136)
		dam_msgnum = 9;
	else if (dam <= 176)
		dam_msgnum = 10;
	else if (dam <= 216)
		dam_msgnum = 11;
	else if (dam <= 256)
		dam_msgnum = 12;
	else if (dam <= 296)
		dam_msgnum = 13;
	else if (dam <= 400)
		dam_msgnum = 14;
	else if (dam <= 800)
		dam_msgnum = 15;
	else
		dam_msgnum = 16;
	// damage message to onlookers
	char *buf_ptr = replace_string(dam_weapons[dam_msgnum].to_room,
								   attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	if (brief_shields_.empty()) {
		act(buf_ptr, false, ch, nullptr, victim, kToNotVict | kToArenaListen);
	} else {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%s%s", buf_ptr, brief_shields_.c_str());
		act(buf_, false, ch, nullptr, victim, kToNotVict | kToArenaListen | kToBriefShields);
		act(buf_ptr, false, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
	}

	// damage message to damager
	send_to_char(ch, "%s", dam ? "&Y&q" : "&y&q");
	if (!brief_shields_.empty() && PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%s%s",
				 replace_string(dam_weapons[dam_msgnum].to_char,
								attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
				 brief_shields_.c_str());
		act(buf_, false, ch, nullptr, victim, kToChar);
	} else {
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_char,
								 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, false, ch, nullptr, victim, kToChar);
	}
	send_to_char("&Q&n", ch);

	// damage message to damagee
	send_to_char("&R&q", victim);
	if (!brief_shields_.empty() && PRF_FLAGGED(victim, PRF_BRIEF_SHIELDS)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%s%s",
				 replace_string(dam_weapons[dam_msgnum].to_victim,
								attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
				 brief_shields_.c_str());
		act(buf_, false, ch, nullptr, victim, kToVict | kToSleep);
	} else {
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_victim,
								 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, false, ch, nullptr, victim, kToVict | kToSleep);
	}
	send_to_char("&Q&n", victim);
}

// запоминание.
// чара-обидчика моб помнит всегда, если он его бьет непосредственно.
// если бьют клоны (проверка на MOB_CLONE), тоже помнит всегда.
// если бьют храны или чармис (все остальные под чармом), то только если
// моб может видеть их хозяина.
void update_mob_memory(CharData *ch, CharData *victim) {
	// первое -- бьют моба, он запоминает обидчика
	if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY)) {
		if (!IS_NPC(ch)) {
			mobRemember(victim, ch);
		} else if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& ch->has_master()
			&& !IS_NPC(ch->get_master())) {
			if (MOB_FLAGGED(ch, MOB_CLONE)) {
				mobRemember(victim, ch->get_master());
			} else if (IN_ROOM(ch->get_master()) == IN_ROOM(victim)
				&& CAN_SEE(victim, ch->get_master())) {
				mobRemember(victim, ch->get_master());
			}
		}
	}

	// второе -- бьет сам моб и запоминает, кого потом добивать :)
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY)) {
		if (!IS_NPC(victim)) {
			mobRemember(ch, victim);
		} else if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
			&& victim->has_master()
			&& !IS_NPC(victim->get_master())) {
			if (MOB_FLAGGED(victim, MOB_CLONE)) {
				mobRemember(ch, victim->get_master());
			} else if (IN_ROOM(victim->get_master()) == ch->in_room
				&& CAN_SEE(ch, victim->get_master())) {
				mobRemember(ch, victim->get_master());
			}
		}
	}
}

bool Damage::magic_shields_dam(CharData *ch, CharData *victim) {
	if (dam <= 0) {
		return false;
	}

	// отражение части маг дамага от зеркала
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICGLASS)
		&& dmg_type == fight::kMagicDmg) {
		int pct = 6;
		if (IS_NPC(victim) && !IS_CHARMICE(victim)) {
			pct += 2;
			if (victim->get_role(MOB_ROLE_BOSS)) {
				pct += 2;
			}
		}
		// дамаг обратки
		const int mg_damage = dam * pct / 100;
		if (mg_damage > 0
			&& victim->get_fighting()
			&& GET_POS(victim) > EPosition::kStun
			&& IN_ROOM(victim) != kNowhere) {
			flags.set(fight::kDrawBriefMagMirror);
			Damage dmg(SpellDmg(kSpellMagicGlass), mg_damage, fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);
			dmg.flags.set(fight::kMagicReflect);
			dmg.Process(victim, ch);
		}
	}

	// обработка щитов, см Damage::post_init_shields()
	if (flags[fight::kVictimFireShield]
		&& !flags[fight::kCritHit]) {
		if (dmg_type == fight::kPhysDmg
			&& !flags[fight::kIgnoreFireShield]) {
			int pct = 15;
			if (IS_NPC(victim) && !IS_CHARMICE(victim)) {
				pct += 5;
				if (victim->get_role(MOB_ROLE_BOSS)) {
					pct += 5;
				}
			}
			fs_damage = dam * pct / 100;
		} else {
			act("Огненный щит вокруг $N1 ослабил вашу атаку.",
				false, ch, 0, victim, kToChar | kToNoBriefShields);
			act("Огненный щит принял часть повреждений на себя.",
				false, ch, 0, victim, kToVict | kToNoBriefShields);
			act("Огненный щит вокруг $N1 ослабил атаку $n1.",
				true, ch, 0, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		}
		flags.set(fight::kDrawBriefFireShield);
		dam -= (dam * number(30, 50) / 100);
	}

	// если критический удар (не точка и стаб) и есть щит - 95% шанс в молоко
	// критическим считается любой удар который вложиля в определенные границы
	if (dam
		&& flags[fight::kCritHit] && flags[fight::kVictimIceShield]
		&& !dam_critic
		&& spell_num != kSpellPoison
		&& number(0, 100) < 94) {
		act("Ваше меткое попадания частично утонуло в ледяной пелене вокруг $N1.",
			false, ch, 0, victim, kToChar | kToNoBriefShields);
		act("Меткое попадание частично утонуло в ледяной пелене щита.",
			false, ch, 0, victim, kToVict | kToNoBriefShields);
		act("Ледяной щит вокруг $N1 частично поглотил меткое попадание $n1.",
			true, ch, 0, victim, kToNotVict | kToArenaListen | kToNoBriefShields);

		flags[fight::kCritHit] = false; //вот это место очень тщательно проверить
		if (dam > 0) dam -= (dam * number(30, 50) / 100);
	}
		//шоб небуло спама модернизировал условие
	else if (dam > 0
		&& flags[fight::kVictimIceShield]
		&& !flags[fight::kCritHit]) {
		flags.set(fight::kDrawBriefIceShield);
		act("Ледяной щит вокруг $N1 смягчил ваш удар.",
			false, ch, 0, victim, kToChar | kToNoBriefShields);
		act("Ледяной щит принял часть удара на себя.",
			false, ch, 0, victim, kToVict | kToNoBriefShields);
		act("Ледяной щит вокруг $N1 смягчил удар $n1.",
			true, ch, 0, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		dam -= (dam * number(30, 50) / 100);
	}

	if (dam > 0
		&& flags[fight::kVictimAirShield]
		&& !flags[fight::kCritHit]) {
		flags.set(fight::kDrawBriefAirShield);
		act("Воздушный щит вокруг $N1 ослабил ваш удар.",
			false, ch, 0, victim, kToChar | kToNoBriefShields);
		act("Воздушный щит смягчил удар $n1.",
			false, ch, 0, victim, kToVict | kToNoBriefShields);
		act("Воздушный щит вокруг $N1 ослабил удар $n1.",
			true, ch, 0, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		dam -= (dam * number(30, 50) / 100);
	}

	return false;
}

void Damage::armor_dam_reduce(CharData *victim) {
	// броня на физ дамаг
	if (dam > 0 && dmg_type == fight::kPhysDmg) {
		alt_equip(victim, kNowhere, dam, 50);
		if (!flags[fight::kCritHit] && !flags[fight::kIgnoreArmor]) {
			// 50 брони = 50% снижение дамага
			int max_armour = 50;
			if (can_use_feat(victim, IMPREGNABLE_FEAT) && PRF_FLAGS(victim).get(PRF_AWAKE)) {
				// непробиваемый в осторожке - до 75 брони
				max_armour = 75;
			}
			int tmp_dam = dam * MAX(0, MIN(max_armour, GET_ARMOUR(victim))) / 100;
			// ополовинивание брони по флагу скила
			if (tmp_dam >= 2 && flags[fight::kHalfIgnoreArmor]) {
				tmp_dam /= 2;
			}
			dam -= tmp_dam;
			// крит удар умножает дамаг, если жертва без призмы и без лед.щита
		}
	}
}

/**
 * Обработка поглощения физ урона.
 * \return true - полное поглощение
 */
bool Damage::dam_absorb(CharData *ch, CharData *victim) {
	if (dmg_type == fight::kPhysDmg
		&& skill_id < ESkill::kIncorrect
		&& spell_num < 0
		&& dam > 0
		&& GET_ABSORBE(victim) > 0) {
		// шансы поглощения: непробиваемый в осторожке 15%, остальные 10%
		int chance = 10 + GET_REAL_REMORT(victim) / 3;
		if (can_use_feat(victim, IMPREGNABLE_FEAT)
			&& PRF_FLAGS(victim).get(PRF_AWAKE)) {
			chance += 5;
		}
		// физ урон - прямое вычитание из дамага
		if (number(1, 100) <= chance) {
			dam -= GET_ABSORBE(victim) / 2;
			if (dam <= 0) {
				act("Ваши доспехи полностью поглотили удар $n1.",
					false, ch, 0, victim, kToVict);
				act("Доспехи $N1 полностью поглотили ваш удар.",
					false, ch, 0, victim, kToChar);
				act("Доспехи $N1 полностью поглотили удар $n1.",
					true, ch, 0, victim, kToNotVict | kToArenaListen);
				return true;
			}
		}
	}
	return false;
}

void Damage::send_critical_message(CharData *ch, CharData *victim) {
	// Блочить мессагу крита при ледяном щите вроде нелогично,
	// так что добавил отдельные сообщения для ледяного щита (Купала)
	if (!flags[fight::kVictimIceShield]) {
		sprintf(buf, "&G&qВаше меткое попадание тяжело ранило %s.&Q&n\r\n",
				PERS(victim, ch, 3));
	} else {
		sprintf(buf, "&B&qВаше меткое попадание утонуло в ледяной пелене щита %s.&Q&n\r\n",
				PERS(victim, ch, 1));
	}

	send_to_char(buf, ch);

	if (!flags[fight::kVictimIceShield]) {
		sprintf(buf, "&r&qМеткое попадание %s тяжело ранило вас.&Q&n\r\n",
				PERS(ch, victim, 1));
	} else {
		sprintf(buf, "&r&qМеткое попадание %s утонуло в ледяной пелене вашего щита.&Q&n\r\n",
				PERS(ch, victim, 1));
	}

	send_to_char(buf, victim);
	// Закомментил чтобы не спамило, сделать потом в виде режима
	//act("Меткое попадание $N1 заставило $n3 пошатнуться.", true, victim, 0, ch, TO_NOTVICT);
}

void update_dps_stats(CharData *ch, int real_dam, int over_dam) {
	if (!IS_NPC(ch)) {
		ch->dps_add_dmg(DpsSystem::PERS_DPS, real_dam, over_dam);
		log("DmetrLog. Name(player): %s, class: %d, remort:%d, level:%d, dmg: %d, over_dmg:%d",
			GET_NAME(ch),
			GET_CLASS(ch),
			GET_REAL_REMORT(ch),
			GetRealLevel(ch),
			real_dam,
			over_dam);
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)) {
			CharData *leader = ch->has_master() ? ch->get_master() : ch;
			leader->dps_add_dmg(DpsSystem::GROUP_DPS, real_dam, over_dam, ch);
		}
	} else if (IS_CHARMICE(ch)
		&& ch->has_master()) {
		ch->get_master()->dps_add_dmg(DpsSystem::PERS_CHARM_DPS, real_dam, over_dam, ch);
		if (!IS_NPC(ch->get_master())) {
			log("DmetrLog. Name(charmice): %s, name(master): %s, class: %d, remort: %d, level: %d, dmg: %d, over_dmg:%d",
				GET_NAME(ch), GET_NAME(ch->get_master()), GET_CLASS(ch->get_master()), GET_REAL_REMORT(ch->get_master()),
				GetRealLevel(ch->get_master()), real_dam, over_dam);
		}

		if (AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_GROUP)) {
			CharData *leader = ch->get_master()->has_master() ? ch->get_master()->get_master() : ch->get_master();
			leader->dps_add_dmg(DpsSystem::GROUP_CHARM_DPS, real_dam, over_dam, ch);
		}
	}
}

void try_angel_sacrifice(CharData *ch, CharData *victim) {
	// если виктим в группе с кем-то с ангелом - вместо смерти виктима умирает ангел
	if (GET_HIT(victim) <= 0
		&& !IS_NPC(victim)
		&& AFF_FLAGGED(victim, EAffectFlag::AFF_GROUP)) {
		const auto people =
			world[IN_ROOM(victim)]->people;    // make copy of people because keeper might be removed from this list inside the loop
		for (const auto keeper : people) {
			if (IS_NPC(keeper)
				&& MOB_FLAGGED(keeper, MOB_ANGEL)
				&& keeper->has_master()
				&& AFF_FLAGGED(keeper->get_master(), EAffectFlag::AFF_GROUP)) {
				CharData *keeper_leader = keeper->get_master()->has_master()
										   ? keeper->get_master()->get_master()
										   : keeper->get_master();
				CharData *victim_leader = victim->has_master()
										   ? victim->get_master()
										   : victim;

				if ((keeper_leader == victim_leader) && (may_kill_here(keeper->get_master(), ch, nullptr))) {
					if (!pk_agro_action(keeper->get_master(), ch)) {
						return;
					}
					log("angel_sacrifice: Nmae (ch): %s, Name(charmice): %s, name(victim): %s", GET_NAME(ch), GET_NAME(keeper), GET_NAME(victim));

					send_to_char(victim, "%s пожертвовал%s своей жизнью, вытаскивая вас с того света!\r\n",
								 GET_PAD(keeper, 0), GET_CH_SUF_1(keeper));
					snprintf(buf, kMaxStringLength, "%s пожертвовал%s своей жизнью, вытаскивая %s с того света!",
							 GET_PAD(keeper, 0), GET_CH_SUF_1(keeper), GET_PAD(victim, 3));
					act(buf, false, victim, 0, 0, kToRoom | kToArenaListen);

					extract_char(keeper, 0);
					GET_HIT(victim) = MIN(300, GET_MAX_HIT(victim) / 2);
				}
			}
		}
	}
}

void update_pk_logs(CharData *ch, CharData *victim) {
	ClanPkLog::check(ch, victim);
	sprintf(buf2, "%s killed by %s at %s [%d] ", GET_NAME(victim), GET_NAME(ch),
			IN_ROOM(victim) != kNowhere ? world[IN_ROOM(victim)]->name : "kNowhere", GET_ROOM_VNUM(IN_ROOM(victim)));
	log("%s", buf2);

	if ((!IS_NPC(ch)
		|| (ch->has_master()
			&& !IS_NPC(ch->get_master())))
		&& NORENTABLE(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA)) {
		mudlog(buf2, BRF, kLvlImplementator, SYSLOG, 0);
		if (IS_NPC(ch)
			&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || IS_HORSE(ch))
			&& ch->has_master()
			&& !IS_NPC(ch->get_master())) {
			sprintf(buf2, "%s is following %s.", GET_NAME(ch), GET_PAD(ch->get_master(), 2));
			mudlog(buf2, BRF, kLvlImplementator, SYSLOG, true);
		}
	}
}

void Damage::process_death(CharData *ch, CharData *victim) {
	CharData *killer = nullptr;

	if (IS_NPC(victim) || victim->desc) {
		if (victim == ch && IN_ROOM(victim) != kNowhere) {
			if (spell_num == kSpellPoison) {
				for (const auto poisoner : world[IN_ROOM(victim)]->people) {
					if (poisoner != victim
						&& GET_ID(poisoner) == victim->Poisoner) {
						killer = poisoner;
					}
				}
			} else if (msg_num == kTypeSuffering) {
				for (const auto attacker : world[IN_ROOM(victim)]->people) {
					if (attacker->get_fighting() == victim) {
						killer = attacker;
					}
				}
			}
		}

		if (ch != victim) {
			killer = ch;
		}
	}

	if (killer) {
		if (AFF_FLAGGED(killer, EAffectFlag::AFF_GROUP)) {
			// т.к. помечен флагом AFF_GROUP - точно PC
			group_gain(killer, victim);
		} else if ((AFF_FLAGGED(killer, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(killer, MOB_ANGEL)
			|| MOB_FLAGGED(killer, MOB_GHOST))
			&& killer->has_master())
			// killer - зачармленный NPC с хозяином
		{
			// по логике надо бы сделать, что если хозяина нет в клетке, но
			// кто-то из группы хозяина в клетке, то опыт накинуть согруппам,
			// которые рядом с убившим моба чармисом.
			if (AFF_FLAGGED(killer->get_master(), EAffectFlag::AFF_GROUP)
				&& IN_ROOM(killer) == IN_ROOM(killer->get_master())) {
				// Хозяин - PC в группе => опыт группе
				group_gain(killer->get_master(), victim);
			} else if (IN_ROOM(killer) == IN_ROOM(killer->get_master()))
				// Чармис и хозяин в одной комнате
				// Опыт хозяину
			{
				perform_group_gain(killer->get_master(), victim, 1, 100);
			}
			// else
			// А хозяина то рядом не оказалось, все чармису - убрано
			// нефиг абьюзить чарм  perform_group_gain( killer, victim, 1, 100 );
		} else {
			// Просто NPC или PC сам по себе
			perform_group_gain(killer, victim, 1, 100);
		}
	}

	// в сислог иммам идут только смерти в пк (без арен)
	// в файл пишутся все смерти чаров
	// если чар убит палачем то тоже не спамим

	if (!IS_NPC(victim) && !(killer && PRF_FLAGGED(killer, PRF_EXECUTOR))) {
		update_pk_logs(ch, victim);

		for (const auto &ch_vict : world[ch->in_room]->people) {
			//Мобы все кто присутствовал при смерти игрока забывают
			if (IS_IMMORTAL(ch_vict))
				continue;
			if (!HERE(ch_vict))
				continue;
			if (!IS_NPC(ch_vict))
				continue;
			if (MOB_FLAGGED(ch_vict, MOB_MEMORY)) {
				mobForget(ch_vict, victim);
			}
		}

	}

	if (killer) {
		ch = killer;
	}

	die(victim, ch);
}

ObjData *GetUsedWeapon(CharData *ch, fight::AttackType AttackType) {
	ObjData *UsedWeapon = nullptr;

	if (AttackType == fight::AttackType::kMainHand) {
		if (!(UsedWeapon = GET_EQ(ch, WEAR_WIELD)))
			UsedWeapon = GET_EQ(ch, WEAR_BOTHS);
	} else if (AttackType == fight::AttackType::kOffHand)
		UsedWeapon = GET_EQ(ch, WEAR_HOLD);

	return UsedWeapon;
}

// обработка щитов, зб, поглощения, сообщения для огн. щита НЕ ЗДЕСЬ
// возвращает сделанный дамаг
int Damage::Process(CharData *ch, CharData *victim) {
	post_init(ch, victim);

	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		return 0;
	}

	if (IN_ROOM(victim) == kNowhere
		|| ch->in_room == kNowhere
		|| ch->in_room != IN_ROOM(victim)) {
		log("SYSERR: Attempt to damage '%s' in room kNowhere by '%s'.",
			GET_NAME(victim), GET_NAME(ch));
		return 0;
	}

	if (GET_POS(victim) <= EPosition::kDead) {
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
		die(victim, nullptr);
		return 0;    // -je, 7/7/92
	}
	// No fight mobiles
	if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
		|| (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT))) {
		return 0;
	}

	if (dam > 0) {
		// You can't damage an immortal!
		if (IS_GOD(victim))
			dam = 0;
		else if (IS_IMMORTAL(victim) || GET_GOD_FLAG(victim, GF_GODSLIKE))
			dam /= 4;
		else if (GET_GOD_FLAG(victim, GF_GODSCURSE))
			dam *= 2;
	}

	// запоминание мобами обидчиков и жертв
	update_mob_memory(ch, victim);

	// атакер и жертва становятся видимыми
	appear(ch);
	appear(victim);

	// If you attack a pet, it hates your guts
	if (!same_group(ch, victim))
		check_agro_follower(ch, victim);

	if (victim != ch)    // Start the attacker fighting the victim
	{
		if (GET_POS(ch) > EPosition::kStun && (ch->get_fighting() == nullptr)) {
			if (!pk_agro_action(ch, victim))
				return (0);
			set_fighting(ch, victim);
			npc_groupbattle(ch);
		}
		// Start the victim fighting the attacker
		if (GET_POS(victim) > EPosition::kDead && (victim->get_fighting() == nullptr)) {
			set_fighting(victim, ch);
			npc_groupbattle(victim);
		}

		// лошадь сбрасывает седока при уроне
		if (ch->ahorse() && ch->get_horse() == victim) {
			victim->drop_from_horse();
		} else if (victim->ahorse() && victim->get_horse() == ch) {
			ch->drop_from_horse();
		}
	}

	// If negative damage - return
	if (dam < 0
		|| ch->in_room == kNowhere
		|| IN_ROOM(victim) == kNowhere
		|| ch->in_room != IN_ROOM(victim)) {
		return (0);
	}

	// нельзя драться в состоянии нестояния
	if (GET_POS(ch) <= EPosition::kIncap) {
		return 0;
	}

	// санка/призма для физ и маг урона
	if (dam >= 2) {
		if (AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA)
			&& !(skill_id == ESkill::kBackstab && can_use_feat(ch, THIEVES_STRIKE_FEAT))) {
			if (dmg_type == fight::kPhysDmg) {
				dam *= 2;
			} else if (dmg_type == fight::kMagicDmg) {
				dam /= 2;
			}
		}
		if (AFF_FLAGGED(victim, EAffectFlag::AFF_SANCTUARY)
			&& !(skill_id == ESkill::kBackstab && can_use_feat(ch, THIEVES_STRIKE_FEAT))) {
			if (dmg_type == fight::kPhysDmg) {
				dam /= 2;
			} else if (dmg_type == fight::kMagicDmg) {
				dam *= 2;
			}
		}

		if (IS_NPC(victim) && Bonus::is_bonus_active(Bonus::EBonusType::BONUS_DAMAGE)) {
			dam *= Bonus::get_mult_bonus();
		}
	}

	//учет резистов для магического урона
	if (dmg_type == fight::kMagicDmg) {
		if (spell_num > 0) {
			dam = ApplyResist(victim, GetResistType(spell_num), dam);
		} else {
			dam = ApplyResist(victim, GetResisTypeWithSpellClass(magic_type), dam);
		};
	};

	// учет положения атакующего и жертвы
	// Include a damage multiplier if victim isn't ready to fight:
	// Position sitting  1.5 x normal
	// Position resting  2.0 x normal
	// Position sleeping 2.5 x normal
	// Position stunned  3.0 x normal
	// Position incap    3.5 x normal
	// Position mortally 4.0 x normal
	// Note, this is a hack because it depends on the particular
	// values of the POSITION_XXX constants.

	// физ дамага атакера из сидячего положения
	if (ch_start_pos < EPosition::kFight
		&& dmg_type == fight::kPhysDmg) {
		dam -= dam * (EPosition::kFight - ch_start_pos) / 4;
	}

	// дамаг не увеличивается если:
	// на жертве есть воздушный щит
	// атака - каст моба (в mage_damage увеличение дамага от позиции было только у колдунов)
	if (victim_start_pos < EPosition::kFight
		&& !flags[fight::kVictimAirShield]
		&& !(dmg_type == fight::kMagicDmg
			&& IS_NPC(ch))) {
		dam += dam * (EPosition::kFight - victim_start_pos) / 4;
	}

	// прочие множители

	// изменение физ урона по холду
	if (GET_MOB_HOLD(victim)
		&& dmg_type == fight::kPhysDmg) {
		if (IS_NPC(ch)
			&& !IS_CHARMICE(ch)) {
			dam = dam * 15 / 10;
		} else {
			dam = dam * 125 / 100;
		}
	}

	// тюнинг дамага чармисов по чарам
	if (!IS_NPC(victim)
		&& IS_CHARMICE(ch)) {
		dam = dam * 8 / 10;
	}

	// яд белены для физ урона
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BELENA_POISON)
		&& dmg_type == fight::kPhysDmg) {
		dam -= dam * GET_POISON(ch) / 100;
	}

	// added by WorM(Видолюб) поглощение физ.урона в %
	//if(GET_PR(victim) && IS_NPC(victim)
	if (GET_PR(victim) && dmg_type == fight::kPhysDmg) {
		int ResultDam = dam - (dam * GET_PR(victim) / 100);
		ch->send_to_TC(false, true, false,
					   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		victim->send_to_TC(false, true, false,
						   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		dam = ResultDam;
	}

	// ЗБ
	if (!IS_IMMORTAL(ch) && AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD)) {
		if (skill_id == ESkill::kBash) {
			SendSkillMessages(dam, ch, victim, msg_num);
		}
		act("Магический кокон полностью поглотил удар $N1.", false, victim, 0, ch, kToChar);
		act("Магический кокон вокруг $N1 полностью поглотил ваш удар.", false, ch, 0, victim, kToChar);
		act("Магический кокон вокруг $N1 полностью поглотил удар $n1.",
			true,
			ch,
			0,
			victim,
			kToNotVict | kToArenaListen);
		return 0;
	}
	// щиты, броня, поглощение
	if (victim != ch) {
		bool shield_full_absorb = magic_shields_dam(ch, victim);
		// сначала броня
		armor_dam_reduce(victim);
		// потом абсорб
		bool armor_full_absorb = dam_absorb(ch, victim);
		if (flags[fight::kCritHit] && (GetRealLevel(victim) >= 5 || !IS_NPC(ch))
			&& !AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA)
			&& !flags[fight::kVictimIceShield]) {
			dam = MAX(dam, MIN(GET_REAL_MAX_HIT(victim) / 8, dam * 2)); //крит
		}
		// полное поглощение
		if (shield_full_absorb || armor_full_absorb) {
			return 0;
		}
	}

	// Внутри magic_shields_dam вызывается dmg::proccess, если чар там умрет, то будет креш
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Death from magic_shields_dam");
		return 0;
	}

	// зб на мобе
	if (MOB_FLAGGED(victim, MOB_PROTECT)) {
		if (victim != ch) {
			act("$n находится под защитой Богов.", false, victim, 0, 0, kToRoom);
		}
		return 0;
	}

	// яд скополии
	if (skill_id != ESkill::kBackstab
		&& AFF_FLAGGED(victim, EAffectFlag::AFF_SCOPOLIA_POISON)) {
		dam += dam * GET_POISON(victim) / 100;
	}

	// внутри есть !боевое везение!, для какого типа дамага - не знаю
	DamageActorParameters params(ch, victim, dam);
	handle_affects(params);
	dam = params.damage;
	DamageVictimParameters params1(ch, victim, dam);
	handle_affects(params1);
	dam = params1.damage;

	// костыль для сетовых бонусов
	if (dmg_type == fight::kPhysDmg) {
		dam += ch->obj_bonus().calc_phys_dmg(dam);
	} else if (dmg_type == fight::kMagicDmg) {
		dam += ch->obj_bonus().calc_mage_dmg(dam);
	}

	// обратка от зеркал/огненного щита
	if (flags[fight::kMagicReflect]) {
		// ограничение для зеркал на 40% от макс хп кастера
		dam = std::min(dam, GET_MAX_HIT(victim) * 4 / 10);
		// чтобы не убивало обраткой
		dam = std::min(dam, GET_HIT(victim) - 1);
	}

	dam = std::clamp(dam, 0, kMaxHits);
	if (dam >= 0) {
		if (dmg_type == fight::kPhysDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Skills()[skill_id].GetName(), 1, wielded))
				return 0;
		} else if (dmg_type == fight::kMagicDmg) {
			if (!damage_mtrigger(ch, victim, dam, GetSpellName(spell_num), 0, wielded))
				return 0;
		}
	}

	// расчет бэтл-экспы для чаров
	gain_battle_exp(ch, victim, dam);

	// real_dam так же идет в обратку от огн.щита
	int real_dam = dam;
	int over_dam = 0;

	// собственно нанесение дамага
	if (dam > GET_HIT(victim) + 11) {
		real_dam = GET_HIT(victim) + 11;
		over_dam = dam - real_dam;
	}
	GET_HIT(victim) -= dam;
	// если на чармисе вампир
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_VAMPIRE)) {
		GET_HIT(ch) = MAX(GET_HIT(ch), MIN(GET_HIT(ch) + MAX(1, dam * 0.1), GET_REAL_MAX_HIT(ch)
			+ GET_REAL_MAX_HIT(ch) * GetRealLevel(ch) / 10));
		// если есть родство душ, то чару отходит по 5% от дамаги к хп
		if (ch->has_master()) {
			if (can_use_feat(ch->get_master(), SOULLINK_FEAT)) {
				GET_HIT(ch->get_master()) = MAX(GET_HIT(ch->get_master()),
												MIN(GET_HIT(ch->get_master()) + MAX(1, dam * 0.05),
													GET_REAL_MAX_HIT(ch->get_master())
														+ GET_REAL_MAX_HIT(ch->get_master())
															* GetRealLevel(ch->get_master())
															/ 10));
			}
		}
	}

	// запись в дметр фактического и овер дамага
	update_dps_stats(ch, real_dam, over_dam);
	// запись дамага в список атакеров
	if (IS_NPC(victim)) {
		victim->add_attacker(ch, ATTACKER_DAMAGE, real_dam);
	}

	// попытка спасти жертву через ангела
	try_angel_sacrifice(ch, victim);

	// обновление позиции после удара и ангела
	update_pos(victim);
	// если вдруг виктим сдох после этого, то произойдет креш, поэтому вставил тут проверочку
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Error in fight_hit, function process()\r\n");
		return 0;
	}
	// если у чара есть жатва жизни
	if (can_use_feat(victim, HARVESTLIFE_FEAT)) {
		if (GET_POS(victim) == EPosition::kDead) {
			int souls = victim->get_souls();
			if (souls >= 10) {
				GET_HIT(victim) = 0 + souls * 10;
				update_pos(victim);
				send_to_char("&GДуши спасли вас от смерти!&n\r\n", victim);
				victim->set_souls(0);
			}
		}
	}
	// сбивание надува черноков //
	if (spell_num != kSpellPoison
		&& dam > 0
		&& !flags[fight::kMagicReflect]) {
		try_remove_extrahits(ch, victim);
	}

	// сообщения о крит ударах //
	if (dam
		&& flags[fight::kCritHit]
		&& !dam_critic
		&& spell_num != kSpellPoison) {
		send_critical_message(ch, victim);
	}

	//  SendSkillMessages sends a message from the messages file in lib/misc.
	//  dam_message just sends a generic "You hit $n extremely hard.".
	//  SendSkillMessages is preferable to dam_message because it is more
	//  descriptive.
	//
	//  If we are _not_ attacking with a weapon (i.e. a spell), always use
	//  SendSkillMessages. If we are attacking with a weapon: If this is a miss or a
	//  death blow, send a SendSkillMessages if one exists; if not, default to a
	//  dam_message. Otherwise, always send a dam_message.
	// log("[DAMAGE] Attack message...");

	// инит Damage::brief_shields_
	if (flags.test(fight::kDrawBriefFireShield)
		|| flags.test(fight::kDrawBriefAirShield)
		|| flags.test(fight::kDrawBriefIceShield)
		|| flags.test(fight::kDrawBriefMagMirror)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "&n (%s%s%s%s)",
				 flags.test(fight::kDrawBriefFireShield) ? "&R*&n" : "",
				 flags.test(fight::kDrawBriefAirShield) ? "&C*&n" : "",
				 flags.test(fight::kDrawBriefIceShield) ? "&W*&n" : "",
				 flags.test(fight::kDrawBriefMagMirror) ? "&M*&n" : "");
		brief_shields_ = buf_;
	}

	// сообщения об ударах //
	if (MUD::Skills().IsValid(skill_id) || spell_num >= 0 || hit_type < 0) {
		// скилл, спелл, необычный дамаг
		SendSkillMessages(dam, ch, victim, msg_num, brief_shields_);
	} else {
		// простой удар рукой/оружием
		if (GET_POS(victim) == EPosition::kDead || dam == 0) {
			if (!SendSkillMessages(dam, ch, victim, msg_num, brief_shields_)) {
				dam_message(ch, victim);
			}
		} else {
			dam_message(ch, victim);
		}
	}
	/// Use send_to_char -- act() doesn't send message if you are DEAD.
	char_dam_message(dam, ch, victim, flags[fight::kNoFleeDmg]);

	victim->send_to_TC(false, true, true, "&MПолучен урон = %d&n\r\n", dam);
	ch->send_to_TC(false, true, true, "&MПрименен урон = %d&n\r\n", dam);

	// Проверить, что жертва все еще тут. Может уже сбежала по трусости.
	// Думаю, простой проверки достаточно.
	// Примечание, если сбежал в FIRESHIELD,
	// то обратного повреждения по атакующему не будет
	if (ch->in_room != IN_ROOM(victim)) {
		return dam;
	}

	// Stop someone from fighting if they're stunned or worse
	/*if ((GET_POS(victim) <= EPosition::kStun)
		&& (victim->get_fighting() != NULL))
	{
		stop_fighting(victim, GET_POS(victim) <= EPosition::kDead);
	} */


	// жертва умирает //
	if (GET_POS(victim) == EPosition::kDead) {
		process_death(ch, victim);
		return -1;
	}

	// обратка от огненного щита
	if (fs_damage > 0
		&& victim->get_fighting()
		&& GET_POS(victim) > EPosition::kStun
		&& IN_ROOM(victim) != kNowhere) {
		Damage dmg(SpellDmg(kSpellFireShield), fs_damage, fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		dmg.flags.set(fight::kMagicReflect);
		dmg.Process(victim, ch);
	}

	return dam;
}

void HitData::try_mighthit_dam(CharData *ch, CharData *victim) {
	int percent = number(1, MUD::Skills()[ESkill::kHammer].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kHammer, victim);
	TrainSkill(ch, ESkill::kHammer, percent <= prob, victim);
	int lag = 0, might = 0;

	if (GET_MOB_HOLD(victim)) {
		percent = number(1, 25);
	}

	if (IS_IMMORTAL(victim)) {
		prob = 0;
	}

	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kHammer].name, percent, prob, percent <= prob);
	if (percent > prob || dam == 0) {
		sprintf(buf, "&c&qВаш богатырский удар пропал впустую.&Q&n\r\n");
		send_to_char(buf, ch);
		lag = 3;
		dam = 0;
	} else if (MOB_FLAGGED(victim, MOB_NOMIGHTHIT)) {
		sprintf(buf, "&c&qНа других надо силу проверять!&Q&n\r\n");
		send_to_char(buf, ch);
		lag = 1;
		dam = 0;
	} else {
		might = prob * 100 / percent;
		if (might < 180) {
			sprintf(buf, "&b&qВаш богатырский удар задел %s.&Q&n\r\n",
					PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 1;
			WAIT_STATE(victim, kPulseViolence);
			Affect<EApplyLocation> af;
			af.type = kSpellBattle;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = CalcDuration(victim, 1, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			affect_join(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание затуманилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N содрогнул$U от богатырского удара $n1.", true, ch, 0, victim, kToNotVict | kToArenaListen);
			if (!number(0, 2)) {
				might_hit_bash(ch, victim);
			}
		} else if (might < 800) {
			sprintf(buf, "&g&qВаш богатырский удар пошатнул %s.&Q&n\r\n", PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 2;
			dam += (dam / 1);
			WAIT_STATE(victim, 2 * kPulseViolence);
			Affect<EApplyLocation> af;
			af.type = kSpellBattle;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = CalcDuration(victim, 2, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			affect_join(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N пошатнул$U от богатырского удара $n1.", true, ch, 0, victim, kToNotVict | kToArenaListen);
			if (!number(0, 1)) {
				might_hit_bash(ch, victim);
			}
		} else {
			sprintf(buf, "&G&qВаш богатырский удар сотряс %s.&Q&n\r\n", PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 2;
			dam *= 4;
			WAIT_STATE(victim, 3 * kPulseViolence);
			Affect<EApplyLocation> af;
			af.type = kSpellBattle;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = CalcDuration(victim, 3, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			affect_join(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание померкло после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N зашатал$U от богатырского удара $n1.", true, ch, 0, victim, kToNotVict | kToArenaListen);
			might_hit_bash(ch, victim);
		}
	}
	//set_wait(ch, lag, true);
	// Временный костыль, чтоб пофиксить лищний раунд КД
	lag = MAX(1, lag - 1);
	SetSkillCooldown(ch, ESkill::kHammer, lag);
}

void HitData::try_stupor_dam(CharData *ch, CharData *victim) {
	int percent = number(1, MUD::Skills()[ESkill::kOverwhelm].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kOverwhelm, victim);
	TrainSkill(ch, ESkill::kOverwhelm, prob >= percent, victim);
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kOverwhelm].name, percent, prob, prob >= percent);
	int lag = 0;

	if (GET_MOB_HOLD(victim)) {
		prob = MAX(prob, percent * 150 / 100 + 1);
	}

	if (IS_IMMORTAL(victim)) {
		prob = 0;
	}

	if (prob < percent || dam == 0 || MOB_FLAGGED(victim, MOB_NOSTUPOR)) {
		sprintf(buf, "&c&qВы попытались оглушить %s, но не смогли.&Q&n\r\n", PERS(victim, ch, 3));
		send_to_char(buf, ch);
		lag = 3;
		dam = 0;
	} else if (prob * 100 / percent < 300) {
		sprintf(buf, "&g&qВаша мощная атака оглушила %s.&Q&n\r\n", PERS(victim, ch, 3));
		send_to_char(buf, ch);
		lag = 2;
		int k = ch->get_skill(ESkill::kOverwhelm) / 30;
		if (!IS_NPC(victim)) {
			k = MIN(2, k);
		}
		dam *= MAX(2, number(1, k));
		WAIT_STATE(victim, 3 * kPulseViolence);
		sprintf(buf, "&R&qВаше сознание слегка помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
		send_to_char(buf, victim);
		act("$n оглушил$a $N3.", true, ch, 0, victim, kToNotVict | kToArenaListen);
	} else {
		if (MOB_FLAGGED(victim, MOB_NOBASH)) {
			sprintf(buf, "&G&qВаш мощнейший удар оглушил %s.&Q&n\r\n", PERS(victim, ch, 3));
		} else {
			sprintf(buf, "&G&qВаш мощнейший удар сбил %s с ног.&Q&n\r\n", PERS(victim, ch, 3));
		}
		send_to_char(buf, ch);
		if (MOB_FLAGGED(victim, MOB_NOBASH)) {
			act("$n мощным ударом оглушил$a $N3.", true, ch, 0, victim, kToNotVict | kToArenaListen);
		} else {
			act("$n своим оглушающим ударом сбил$a $N3 с ног.", true, ch, 0, victim, kToNotVict | kToArenaListen);
		}
		lag = 2;
		int k = ch->get_skill(ESkill::kOverwhelm) / 20;
		if (!IS_NPC(victim)) {
			k = MIN(4, k);
		}
		dam *= MAX(3, number(1, k));
		WAIT_STATE(victim, 3 * kPulseViolence);
		if (GET_POS(victim) > EPosition::kSit && !MOB_FLAGGED(victim, MOB_NOBASH)) {
			GET_POS(victim) = EPosition::kSit;
			sprintf(buf, "&R&qОглушающий удар %s сбил вас с ног.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
		} else {
			sprintf(buf, "&R&qВаше сознание слегка помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
		}
	}
	//set_wait(ch, lag, true);
	// Временный костыль, чтоб пофиксить лищний раунд КД
	lag = MAX(2, lag - 1);
	SetSkillCooldown(ch, ESkill::kOverwhelm, lag);
}

int HitData::extdamage(CharData *ch, CharData *victim) {
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		return 0;
	}

	const int mem_dam = dam;

	if (dam < 0) {
		dam = 0;
	}

	//* богатырский молот //
	// в эти условия ничего добавлять не надо, иначе EAF_MIGHTHIT не снимется
	// с моба по ходу боя, если он не может по каким-то причинам смолотить
	if (GET_AF_BATTLE(ch, kEafHammer) && GET_WAIT(ch) <= 0) {
		CLR_AF_BATTLE(ch, kEafHammer);
		if (check_mighthit_weapon(ch) && !GET_AF_BATTLE(ch, kEafTouch)) {
			try_mighthit_dam(ch, victim);
		}
	}
		//* оглушить //
		// аналогично молоту, все доп условия добавляются внутри
	else if (GET_AF_BATTLE(ch, kEafOverwhelm) && GET_WAIT(ch) <= 0) {
		CLR_AF_BATTLE(ch, kEafOverwhelm);
		const int minimum_weapon_weigth = 19;
		if (IS_IMMORTAL(ch)) {
			try_stupor_dam(ch, victim);
		} else if (IS_NPC(ch)) {
			const bool wielded_with_bow = wielded && (static_cast<ESkill>(wielded->get_skill()) == ESkill::kBows);
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)) {
				// проверка оружия для глуша чармисов
				const bool wielded_for_stupor = GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_BOTHS);
				const bool weapon_weigth_ok = wielded && (GET_OBJ_WEIGHT(wielded) >= minimum_weapon_weigth);
				if (wielded_for_stupor && !wielded_with_bow && weapon_weigth_ok) {
					try_stupor_dam(ch, victim);
				}
			} else {
				// нпц глушат всем, кроме луков
				if (!wielded_with_bow) {
					try_stupor_dam(ch, victim);
				}
			}
		} else if (wielded) {
			if (static_cast<ESkill>(wielded->get_skill()) == ESkill::kBows) {
				send_to_char("Луком оглушить нельзя.\r\n", ch);
			} else if (!GET_AF_BATTLE(ch, kEafParry) && !GET_AF_BATTLE(ch, kEafMultyparry)) {
				if (GET_OBJ_WEIGHT(wielded) >= minimum_weapon_weigth) {
					try_stupor_dam(ch, victim);
				} else {
					send_to_char("&WВаше оружие слишком легкое, чтобы им можно было оглушить!&Q&n\r\n", ch);
				}
			}
		}
		else {
			sprintf(buf, "&c&qВы оказались без оружия, а пальцем оглушить нельзя.&Q&n\r\n");
			send_to_char(buf, ch);
			sprintf(buf, "&c&q%s оказался без оружия и не смог вас оглушить.&Q&n\r\n", GET_NAME(ch));
			send_to_char(buf, victim);
		}
	}
		//* яды со скила отравить //
	else if (!MOB_FLAGGED(victim, MOB_PROTECT)
		&& dam
		&& wielded
		&& wielded->has_timed_spell()
		&& ch->get_skill(ESkill::kPoisoning)) {
		try_weap_poison(ch, victim, wielded->timed_spell().is_spell_poisoned());
	}
		//* травящий ядом моб //
	else if (dam
		&& IS_NPC(ch)
		&& NPC_FLAGGED(ch, NPC_POISON)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& GET_WAIT(ch) <= 0
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_POISON)
		&& number(0, 100) < GET_LIKES(ch) + GetRealLevel(ch) - GetRealLevel(victim)
		&& !CalcGeneralSaving(ch, victim, ESaving::kCritical, -GET_REAL_CON(victim))) {
		poison_victim(ch, victim, MAX(1, GetRealLevel(ch) - GetRealLevel(victim)) * 10);
	}
		//* точный стиль //
	else if (dam && get_flags()[fight::kCritHit] && dam_critic) {
		compute_critical(ch, victim);
	}

	// Если удар парирован, необходимо все равно ввязаться в драку.
	// Вызывается damage с отрицательным уроном
	dam = mem_dam >= 0 ? dam : -1;

	Damage dmg(SkillDmg(skill_num), dam, fight::kPhysDmg, wielded);
	dmg.hit_type = hit_type;
	dmg.dam_critic = dam_critic;
	dmg.flags = get_flags();
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;

	return dmg.Process(ch, victim);
}

/**
 * Инициализация всех нужных первичных полей (отделены в хедере), после
 * этой функции уже начинаются подсчеты собсна хитролов/дамролов и прочего.
 */
void HitData::init(CharData *ch, CharData *victim) {
	// Find weapon for attack number weapon //

	if (weapon == fight::AttackType::kMainHand) {
		if (!(wielded = GET_EQ(ch, WEAR_WIELD))) {
			wielded = GET_EQ(ch, WEAR_BOTHS);
			weapon_pos = WEAR_BOTHS;
		}
	} else if (weapon == fight::AttackType::kOffHand) {
		wielded = GET_EQ(ch, WEAR_HOLD);
		weapon_pos = WEAR_HOLD;
		if (!wielded) { // удар второй рукой
			weap_skill = ESkill::kLeftHit;
			weap_skill_is = CalcCurrentSkill(ch, weap_skill, victim);
			TrainSkill(ch, weap_skill, true, victim);
		}
	}

	if (wielded
		&& GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON) {
		// для всех типов атак скилл берется из пушки, если она есть
		weap_skill = static_cast<ESkill>(GET_OBJ_SKILL(wielded));
	} else {
		// удар голыми руками
		weap_skill = ESkill::kPunch;
	}
	weap_skill_is = CalcCurrentSkill(ch, weap_skill, victim);
	TrainSkill(ch, weap_skill, true, victim);

	//* обработка ESkill::kNoParryHit //
	if (skill_num == ESkill::kUndefined && ch->get_skill(ESkill::kNoParryHit)) {
		int tmp_skill = CalcCurrentSkill(ch, ESkill::kNoParryHit, victim);
		bool success = tmp_skill >= number(1, MUD::Skills()[ESkill::kNoParryHit].difficulty);
		TrainSkill(ch, ESkill::kNoParryHit, success, victim);
		if (success) {
			hit_no_parry = true;
		}
	}

	if (GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) {
		hit_no_parry = true;
	}

	if (wielded && GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON) {
		hit_type = GET_OBJ_VAL(wielded, 3);
	} else {
		weapon_pos = 0;
		if (IS_NPC(ch)) {
			hit_type = ch->mob_specials.attack_type;
		}
	}

	// позиции сражающихся до применения скилов и прочего, что может их изменить
	ch_start_pos = GET_POS(ch);
	victim_start_pos = GET_POS(victim);
}

/**
 * Подсчет статичных хитролов у чара, не меняющихся от рандома типа train_skill
 * (в том числе weap_skill_is) или параметров противника.
 * Предполагается, что в итоге это пойдет в 'счет все' через что-то вроде
 * TestSelfHitroll() в данный момент.
 */
void HitData::calc_base_hr(CharData *ch) {
	if (skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab) {
		if (wielded
			&& GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON
			&& !IS_NPC(ch)) {
			// Apply HR for light weapon
			int percent = 0;
			switch (weapon_pos) {
				case WEAR_WIELD: percent = (str_bonus(GET_REAL_STR(ch), STR_WIELD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
					break;
				case WEAR_HOLD: percent = (str_bonus(GET_REAL_STR(ch), STR_HOLD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
					break;
				case WEAR_BOTHS:
					percent = (str_bonus(GET_REAL_STR(ch), STR_WIELD_W) +
						str_bonus(GET_REAL_STR(ch), STR_HOLD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
					break;
			}
			calc_thaco -= MIN(3, MAX(percent, 0));
		} else if (!IS_NPC(ch)) {
			// кулаками у нас полагается бить только богатырям :)
			if (!can_use_feat(ch, BULLY_FEAT))
				calc_thaco += 4;
			else    // а богатырям положен бонус за отсутствие оружия
				calc_thaco -= 3;
		}
	}

	CheckWeapFeats(ch, weap_skill, calc_thaco, dam);

	if (GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) {
		calc_thaco -= MAX(0, (ch->get_skill(weap_skill) - 70) / 8);
	}

	//    AWAKE style - decrease hitroll
	if (GET_AF_BATTLE(ch, kEafAwake)
		&& !can_use_feat(ch, SHADOW_STRIKE_FEAT)
		&& skill_num != ESkill::kThrow
		&& skill_num != ESkill::kBackstab) {
		if (can_auto_block(ch)) {
			// осторожка со щитом в руках у дружа с блоком - штрафы на хитролы (от 0 до 10)
			calc_thaco += ch->get_skill(ESkill::kAwake) * 5 / 100;
		} else {
			// здесь еще были штрафы на дамаг через деление, но положительного дамага
			// на этом этапе еще нет, так что делили по сути нули
			calc_thaco += ((ch->get_skill(ESkill::kAwake) + 9) / 10) + 2;
		}
	}

	if (!IS_NPC(ch) && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab) {
		// Casters use weather, int and wisdom
		if (IS_CASTER(ch)) {
			/*	  calc_thaco +=
				    (10 -
				     complex_skill_modifier (ch, kAny, GAPPLY_ESkill::SKILL_SUCCESS,
							     10));
			*/
			calc_thaco -= (int) ((GET_REAL_INT(ch) - 13) / GetRealLevel(ch));
			calc_thaco -= (int) ((GET_REAL_WIS(ch) - 13) / GetRealLevel(ch));
		}
	}

	// bless
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLESS)) {
		calc_thaco -= 4;
	}
	// curse
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CURSE)) {
		calc_thaco += 6;
		dam -= 5;
	}

	// Учет мощной и прицельной атаки
	if (PRF_FLAGGED(ch, PRF_POWERATTACK) && can_use_feat(ch, POWER_ATTACK_FEAT)) {
		calc_thaco += 2;
	} else if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK) && can_use_feat(ch, GREAT_POWER_ATTACK_FEAT)) {
		calc_thaco += 4;
	} else if (PRF_FLAGGED(ch, PRF_AIMINGATTACK) && can_use_feat(ch, AIMING_ATTACK_FEAT)) {
		calc_thaco -= 2;
	} else if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK) && can_use_feat(ch, GREAT_AIMING_ATTACK_FEAT)) {
		calc_thaco -= 4;
	}

	// Calculate the THAC0 of the attacker
	if (!IS_NPC(ch)) {
		calc_thaco += thaco((int) GET_CLASS(ch), GetRealLevel(ch));
	} else {
		// штраф мобам по рекомендации Триглава
		calc_thaco += (25 - GetRealLevel(ch) / 3);
	}

	//Вычисляем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);

	calc_thaco -= GET_REAL_HR(ch) * p_hitroll;


	// Использование ловкости вместо силы для попадания
	if (can_use_feat(ch, WEAPON_FINESSE_FEAT)) {
		calc_thaco -= str_bonus(GET_REAL_STR(ch), STR_TO_HIT) * p_hitroll;
	} else {
		calc_thaco -= str_bonus(GET_REAL_DEX(ch), STR_TO_HIT) * p_hitroll;
	}
	if ((skill_num == ESkill::kThrow
		|| skill_num == ESkill::kBackstab)
		&& wielded
		&& GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON) {
		if (skill_num == ESkill::kBackstab) {
			calc_thaco -= MAX(0, (ch->get_skill(ESkill::kSneak) + ch->get_skill(ESkill::kHide) - 100) / 30);
		}
	} else {
		// тюнинг оверности делается тут :)
		calc_thaco += 4;
	}

	//dzMUDiST Обработка !исступления! +Gorrah
	if (affected_by_spell(ch, kSpellBerserk)) {
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK)) {
			calc_thaco -= (12 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
		}
	}

}

/**
 * Соответственно подсчет динамических хитролов, не попавших в calc_base_hr()
 * Все, что меняется от раза к разу или при разных противниках
 * При любом изменении содержимого нужно поддерживать адекватность calc_stat_hr()
 */
void HitData::calc_rand_hr(CharData *ch, CharData *victim) {
	//считаем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// штраф в размере 1 хитролла за каждые
	// недокачанные 10% скилла "удар левой рукой"

	if (weapon == fight::AttackType::kOffHand
		&& skill_num != ESkill::kThrow
		&& skill_num != ESkill::kBackstab
		&& !(wielded && GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON)
		&& !IS_NPC(ch)) {
			calc_thaco += std::max(0, (CalcSkillMinCap(victim, ESkill::kLeftHit) - CalcCurrentSkill(ch, ESkill::kLeftHit, victim)) / 10);
	}

	// courage
	if (affected_by_spell(ch, kSpellCourage)) {
		int range = number(1, MUD::Skills()[ESkill::kCourage].difficulty + GET_REAL_MAX_HIT(ch) - GET_HIT(ch));
		int prob = CalcCurrentSkill(ch, ESkill::kCourage, victim);
		TrainSkill(ch, ESkill::kCourage, prob > range, victim);
		if (prob > range) {
			dam += ((ch->get_skill(ESkill::kCourage) + 19) / 20);
			calc_thaco -= ((ch->get_skill(ESkill::kCourage) + 9) / 20) * p_hitroll;
		}
	}

	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab && ch->ahorse()) {
		TrainSkill(ch, ESkill::kRiding, true, victim);
		calc_thaco += 10 - GET_SKILL(ch, ESkill::kRiding) / 20;
	}

	// not can see (blind, dark, etc)
	if (!CAN_SEE(ch, victim))
		calc_thaco += (can_use_feat(ch, BLIND_FIGHT_FEAT) ? 2 : IS_NPC(ch) ? 6 : 10);
	if (!CAN_SEE(victim, ch))
		calc_thaco -= (can_use_feat(victim, BLIND_FIGHT_FEAT) ? 2 : 8);

	// some protects
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_PROTECT_EVIL) && IS_EVIL(ch))
		calc_thaco += 2;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_PROTECT_GOOD) && IS_GOOD(ch))
		calc_thaco += 2;

	// "Dirty" methods for battle
	if (skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab) {
		int prob = (ch->get_skill(weap_skill) + cha_app[GET_REAL_CHA(ch)].illusive) -
			(victim->get_skill(weap_skill) + int_app[GET_REAL_INT(victim)].observation);
		if (prob >= 30 && !GET_AF_BATTLE(victim, kEafAwake)
			&& (IS_NPC(ch) || !GET_AF_BATTLE(ch, kEafPunctual))) {
			calc_thaco -= (ch->get_skill(weap_skill) - victim->get_skill(weap_skill) > 60 ? 2 : 1) * p_hitroll;
			if (!IS_NPC(victim))
				dam += (prob >= 70 ? 3 : (prob >= 50 ? 2 : 1));
		}
	}

	// AWAKE style for victim
	if (GET_AF_BATTLE(victim, kEafAwake)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !GET_MOB_HOLD(victim)) {
		bool success = CalcCurrentSkill(ch, ESkill::kAwake, victim)
			>= number(1, MUD::Skills()[ESkill::kAwake].difficulty);
		if (success) {
			// > и зачем так? кто балансом занимается поправте.
			// воткнул мин. разницу, чтобы анализаторы не ругались
			dam -= IS_NPC(ch) ? 5 : 4;
			calc_thaco += IS_NPC(ch) ? 4 : 2;
		}
		TrainSkill(victim, ESkill::kAwake, success, ch);
	}

	// скилл владения пушкой или голыми руками
	if (weap_skill_is <= 80)
		calc_thaco -= (weap_skill_is / 20) * p_hitroll;
	else if (weap_skill_is <= 160)
		calc_thaco -= (4 + (weap_skill_is - 80) / 10) * p_hitroll;
	else
		calc_thaco -= (4 + 8 + (weap_skill_is - 160) / 5) * p_hitroll;
}

// * Версия calc_rand_hr для показа по 'счет', без рандомов и статов жертвы.
void HitData::calc_stat_hr(CharData *ch) {
	//считаем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// штраф в размере 1 хитролла за каждые
	// недокачанные 10% скилла "удар левой рукой"
	if (weapon == fight::AttackType::kOffHand
		&& skill_num != ESkill::kThrow
		&& skill_num != ESkill::kBackstab
		&& !(wielded && GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON)
		&& !IS_NPC(ch)) {
		calc_thaco += (MUD::Skills()[ESkill::kLeftHit].difficulty - ch->get_skill(ESkill::kLeftHit)) / 10;
	}

	// courage
	if (affected_by_spell(ch, kSpellCourage)) {
		dam += ((ch->get_skill(ESkill::kCourage) + 19) / 20);
		calc_thaco -= ((ch->get_skill(ESkill::kCourage) + 9) / 20) * p_hitroll;
	}

	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab && ch->ahorse()) {
		int prob = ch->get_skill(ESkill::kRiding);
		int range = MUD::Skills()[ESkill::kRiding].difficulty / 2;

		dam += ((prob + 19) / 10);

		if (range > prob)
			calc_thaco += ((range - prob) + 19 / 20);
		else
			calc_thaco -= ((prob - range) + 19 / 20);
	}

	// скилл владения пушкой или голыми руками
	if (ch->get_skill(weap_skill) <= 80)
		calc_thaco -= (ch->get_skill(weap_skill) / 20) * p_hitroll;
	else if (ch->get_skill(weap_skill) <= 160)
		calc_thaco -= (4 + (ch->get_skill(weap_skill) - 80) / 10) * p_hitroll;
	else
		calc_thaco -= (4 + 8 + (ch->get_skill(weap_skill) - 160) / 5) * p_hitroll;
}

// * Подсчет армор класса жертвы.
void HitData::calc_ac(CharData *victim) {


	// Calculate the raw armor including magic armor.  Lower AC is better.
	victim_ac += compute_armor_class(victim);
	victim_ac /= 10;
	//считаем штраф за голод
	if (!IS_NPC(victim) && victim_ac < 5) { //для голодных
		int p_ac = (1 - victim->get_cond_penalty(P_AC)) * 40;
		if (p_ac) {
			if (victim_ac + p_ac > 5) {
				victim_ac = 5;
			} else {
				victim_ac += p_ac;
			}
		}
	}

	if (GET_POS(victim) < EPosition::kFight)
		victim_ac += 4;
	if (GET_POS(victim) < EPosition::kRest)
		victim_ac += 3;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD))
		victim_ac += 4;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_CRYING))
		victim_ac += 4;
}

// * Обработка защитных скиллов: захват, уклон, веер, блок.
void HitData::check_defense_skills(CharData *ch, CharData *victim) {
	if (!hit_no_parry) {
		// обработаем ситуацию ЗАХВАТ
		const auto &people = world[ch->in_room]->people;
		for (const auto vict : people) {
			if (dam < 0) {
				break;
			}

			hit_touching(ch, vict, &dam);
		}
	}

	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, kEafDodge)
		&& GET_WAIT(victim) <= 0
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& GET_MOB_HOLD(victim) == 0
		&& BATTLECNTR(victim) < (GetRealLevel(victim) + 7) / 8) {
		// Обработаем команду   УКЛОНИТЬСЯ
		hit_deviate(ch, victim, &dam);
	} else if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, kEafParry)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0) {
		// обработаем команду  ПАРИРОВАТЬ
		hit_parry(ch, victim, weap_skill, hit_type, &dam);
	} else if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, kEafMultyparry)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& BATTLECNTR(victim) < (GetRealLevel(victim) + 4) / 5
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0) {
		// обработаем команду  ВЕЕРНАЯ ЗАЩИТА
		hit_multyparry(ch, victim, weap_skill, hit_type, &dam);
	} else if (dam > 0
		&& !hit_no_parry
		&& ((GET_AF_BATTLE(victim, kEafBlock) || can_auto_block(victim)) && GET_POS(victim) > EPosition::kSit)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0
		&& BATTLECNTR(victim) < (GetRealLevel(victim) + 8) / 9) {
		// Обработаем команду   БЛОКИРОВАТЬ
		hit_block(ch, victim, &dam);
	}
}

/**
 * В данный момент:
 * добавление дамролов с пушек
 * добавление дамага от концентрации силы
 */
void HitData::add_weapon_damage(CharData *ch, bool need_dice) {
	int damroll;
 	if (need_dice) {
		damroll = RollDices(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
	} else {
		damroll = (GET_OBJ_VAL(wielded, 1) * GET_OBJ_VAL(wielded, 2) + GET_OBJ_VAL(wielded, 1)) / 2;
	}
	if (IS_NPC(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& !(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))) {
		damroll *= MOB_DAMAGE_MULT;
	} else {
		damroll = MIN(damroll,
					  damroll * GET_OBJ_CUR(wielded) / MAX(1, GET_OBJ_MAX(wielded)));
	}

	damroll = calculate_strconc_damage(ch, wielded, damroll);
	dam += MAX(1, damroll);
}

// * Добавление дамага от голых рук и молота.
void HitData::add_hand_damage(CharData *ch, bool need_dice) {

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STONEHAND)) {
		dam += need_dice ? RollDices(2, 4) : 5;
		if (can_use_feat(ch, BULLY_FEAT)) {
			dam += GetRealLevel(ch) / 5;
			dam += MAX(0, GET_REAL_STR(ch) - 25);
		}
	}
	else
		dam += need_dice? number(1, 3) : 2;
	// Мультипликатор повреждений без оружия и в перчатках (линейная интерполяция)
	// <вес перчаток> <увеличение>
	// 0  50%
	// 5 100%
	// 10 150%
	// 15 200%
	// НА МОЛОТ НЕ ВЛИЯЕТ
	if (!GET_AF_BATTLE(ch, kEafHammer)
		|| get_flags()[fight::kCritHit]) //в метком молоте идет учет перчаток
	{
		int modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? MIN(GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)), 18)
													 : 0)); //вес перчаток больше 18 не учитывается
		if (IS_NPC(ch) || can_use_feat(ch, BULLY_FEAT)) {
			modi = MAX(100, modi);
		}
		dam *= modi / 100;
	}
}

// * Расчет шанса на критический удар (не точкой).
void HitData::calc_crit_chance(CharData *ch) {
	dam_critic = 0;
	int calc_critic = 0;

	// Маги, волхвы и не-купеческие чармисы не умеют критать //
	if ((!IS_NPC(ch) && !IS_MAGIC_USER(ch) && !IS_MAGUS(ch))
		|| (IS_NPC(ch) && (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)))) {
		calc_critic = std::min(ch->get_skill(weap_skill), 70);
		if (can_use_feat(ch, FindWeaponMasterFeat(weap_skill))) {
			calc_critic += std::max(0, ch->get_skill(weap_skill) - 70);
		}
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
			calc_critic += ch->get_skill(ESkill::kBackstab);
		}
		if (!IS_NPC(ch)) {
			calc_critic += static_cast<int>(ch->get_skill(ESkill::kPunctual) / 2);
			calc_critic += static_cast<int>(ch->get_skill(ESkill::kNoParryHit) / 3);
		}
		if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
			calc_critic += GetRealLevel(ch);
		}
	} else {
		reset_flag(fight::kCritHit);
	}

	if (number(0, 2000) < calc_critic) {
		set_flag(fight::kCritHit);
	} else {
		reset_flag(fight::kCritHit);
	}
}
int HitData::calc_damage(CharData *ch, bool need_dice) {
	if (PRF_FLAGGED(ch, PRF_EXECUTOR))
		send_to_char(ch, "&YДамага без бонусов == %d&n\r\n", dam);
	if (ch->get_skill(weap_skill) == 0) {
		calc_thaco += (50 - MIN(50, GET_REAL_INT(ch))) / 3;
		dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
	} else {
		apply_weapon_bonus(GET_CLASS(ch), weap_skill, &dam, &calc_thaco);
	}
	if (ch->get_skill(weap_skill) >= 60) { //от уровня скилла
		dam += ((ch->get_skill(weap_skill) - 50) / 10);
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага с уровнем скилла == %d&n\r\n", dam);
	}
	// Учет мощной и прицельной атаки
	if (PRF_FLAGGED(ch, PRF_POWERATTACK) && can_use_feat(ch, POWER_ATTACK_FEAT)) {
		dam += 5;
	} else if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK) && can_use_feat(ch, GREAT_POWER_ATTACK_FEAT)) {
		dam += 10;
	} else if (PRF_FLAGGED(ch, PRF_AIMINGATTACK) && can_use_feat(ch, AIMING_ATTACK_FEAT)) {
		dam -= 5;
	} else if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK) && can_use_feat(ch, GREAT_AIMING_ATTACK_FEAT)) {
		dam -= 10;
	}
	if (PRF_FLAGGED(ch, PRF_EXECUTOR))
		send_to_char(ch, "&YДамага с учетом перков мощная-улучш == %d&n\r\n", dam);
	// courage
	if (affected_by_spell(ch, kSpellCourage)) {
		int range = number(1, MUD::Skills()[ESkill::kCourage].difficulty + GET_REAL_MAX_HIT(ch) - GET_HIT(ch));
		int prob = CalcCurrentSkill(ch, ESkill::kCourage, ch);
		if (prob > range) {
			dam += ((ch->get_skill(ESkill::kCourage) + 19) / 20);
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага с бухлом == %d&n\r\n", dam);
		}
	}
/*	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab && ch->ahorse()) {
		int prob = ch->get_skill(ESkill::kRiding);
		dam += ((prob + 19) / 10);
		send_to_char(ch, "&YДамага с учетом лошади == %d&n\r\n", dam);
	}
*/
	// обработка по факту попадания
	if (skill_num < ESkill::kIncorrect) {
		dam += GetAutoattackDamroll(ch, ch->get_skill(weap_skill));
	if (PRF_FLAGGED(ch, PRF_EXECUTOR))
		send_to_char(ch, "&YДамага +дамролы автоатаки == %d&n\r\n", dam);
	} else {
		dam += GetRealDamroll(ch);
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага +дамролы скилла== %d&n\r\n", dam);
	}
	if (can_use_feat(ch, SHOT_FINESSE_FEAT)) {
		dam += str_bonus(GET_REAL_DEX(ch), STR_TO_DAM);
	} else {
		dam += str_bonus(GET_REAL_STR(ch), STR_TO_DAM);
	}
	if (PRF_FLAGGED(ch, PRF_EXECUTOR))
		send_to_char(ch, "&YДамага с бонусами от силы или ловкости == %d str_bonus == %d str == %d&n\r\n", dam, str_bonus(GET_REAL_STR(ch), STR_TO_DAM), GET_REAL_STR(ch));
	// оружие/руки и модификаторы урона скилов, с ними связанных
	if (wielded && GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON) {
		add_weapon_damage(ch, need_dice);
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага +кубики оружия дамага == %d вооружен %s vnum %d&n\r\n", dam, GET_OBJ_PNAME(wielded,1).c_str(), GET_OBJ_VNUM(wielded));
		if (GET_EQ(ch, WEAR_BOTHS) && weap_skill != ESkill::kBows) { //двуруч множим на 2
			dam *= 2;
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага двуручем множим на 2 == %d&n\r\n", dam);
		}
		// скрытый удар
		int tmp_dam = calculate_noparryhit_dmg(ch, wielded);
		if (tmp_dam > 0) {
			// 0 раунд и стаб = 70% скрытого, дальше раунд * 0.4 (до 5 раунда)
			int round_dam = tmp_dam * 7 / 10;
			if (can_use_feat(ch, SNEAKRAGE_FEAT)) {
				if (ROUND_COUNTER(ch) >= 1 && ROUND_COUNTER(ch) <= 3) {
					dam *= ROUND_COUNTER(ch);
				}
			}
			if (skill_num == ESkill::kBackstab || ROUND_COUNTER(ch) <= 0) {
				dam += round_dam;
			} else {
				dam += round_dam * MIN(3, ROUND_COUNTER(ch));
			}
			if (PRF_FLAGGED(ch, PRF_EXECUTOR))
				send_to_char(ch, "&YДамага от скрытого удара == %d&n\r\n", dam);
		}
	} else {
		add_hand_damage(ch, need_dice);
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага руками == %d&n\r\n", dam);
	}
	if (PRF_FLAGGED(ch, PRF_EXECUTOR))
		send_to_char(ch, "&YДамага после расчета руки или оружия == %d&n\r\n", dam);

	if (GET_AF_BATTLE(ch, kEafIronWind)) {
		dam += ch->get_skill(ESkill::kIronwind) / 2;
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага после расчета железного ветра == %d&n\r\n", dam);
	}

	if (affected_by_spell(ch, kSpellBerserk)) {
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK)) {
			dam = (dam * MAX(150, 150 + GetRealLevel(ch) + RollDices(0, GET_REAL_REMORT(ch)) * 2)) / 100;
			if (PRF_FLAGGED(ch, PRF_EXECUTOR))
				send_to_char(ch, "&YДамага с учетом берсерка== %d&n\r\n", dam);
		}
	}
	if (IS_NPC(ch)) { // урон моба из олц
		dam += RollDices(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
	}

	if (GET_SKILL(ch, ESkill::kRiding) > 100 && ch->ahorse()) {
		dam *= 1 + (GET_SKILL(ch, ESkill::kRiding) - 100) / 500.0; // на лошадке до +20%
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага с учетом лошади (при скилле 200 +20 процентов)== %d&n\r\n", dam);
	}

	if (ch->add_abils.percent_dam_add > 0) {
		int tmp;
		if (need_dice) {
			tmp = dam * (number(1, ch->add_abils.percent_dam_add) / 100.0); 
			dam += tmp;
		} else {
			tmp = dam * (ch->add_abils.percent_dam_add / 2.0 / 100.0);
			dam += tmp;
		}
		if (PRF_FLAGGED(ch, PRF_EXECUTOR))
			send_to_char(ch, "&YДамага c + процентами дамаги== %d, добавили = %d процентов &n\r\n", dam, tmp);
	}
	//режем дамаг от голода
	dam *= ch->get_cond_penalty(P_DAMROLL);
	if (PRF_FLAGGED(ch, PRF_EXECUTOR))
		send_to_char(ch, "&YДамага с бонусами итого == %d&n\r\n", dam);
	return dam;

}

ESpell breathFlag2Spellnum(CharData *ch) {
	ESpell t = kSpellNoSpell;
	// наркоманский код с объездом в двух циклах битвекторов флагов заменил на читаемый код
	// извините..
	if (MOB_FLAGGED(ch, (MOB_FIREBREATH)))
		t = ESpell::kSpellFireBreath;
	if (MOB_FLAGGED(ch, (MOB_GASBREATH)))
		t = ESpell::kSpellGasBreath;
	if (MOB_FLAGGED(ch, (MOB_FROSTBREATH)))
		t = ESpell::kSpellFrostBreath;
	if (MOB_FLAGGED(ch, (MOB_ACIDBREATH)))
		t = ESpell::kSpellAcidBreath;
	if (MOB_FLAGGED(ch, (MOB_LIGHTBREATH)))
		t = ESpell::kSpellLightingBreath;
	return t;
}

/**
* обработка ударов оружием, санка, призма, стили, итд.
*/
void hit(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	if (!victim) {
		return;
	}
	if (!ch || ch->purged() || victim->purged()) {
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim->purged() ? "purged" : "true", __FILE__, __LINE__);
		return;
	}
	// Do some sanity checking, in case someone flees, etc.
	if (ch->in_room != IN_ROOM(victim) || ch->in_room == kNowhere) {
		if (ch->get_fighting() && ch->get_fighting() == victim) {
			stop_fighting(ch, true);
		}
		return;
	}
	// Stand awarness mobs
	if (CAN_SEE(victim, ch)
		&& !victim->get_fighting()
		&& ((IS_NPC(victim) && (GET_HIT(victim) < GET_MAX_HIT(victim)
			|| MOB_FLAGGED(victim, MOB_AWARE)))
			|| AFF_FLAGGED(victim, EAffectFlag::AFF_AWARNESS))
		&& !GET_MOB_HOLD(victim) && GET_WAIT(victim) <= 0) {
		set_battle_pos(victim);
	}

	// дышащий моб может оглушить, и нанесёт физ.дамаг!!
	if (type == ESkill::kUndefined) {
		ESpell spellnum;
		spellnum = breathFlag2Spellnum(ch);
		if (spellnum != ESpell::kSpellNoSpell) // защита от падения
		{
			if (!ch->get_fighting())
				set_fighting(ch, victim);
			if (!victim->get_fighting())
				set_fighting(victim, ch);
			// AOE атаки всегда магические. Раздаём каждому игроку и уходим.
			if (MOB_FLAGGED(ch, MOB_AREA_ATTACK)) {
				const auto
					people = world[ch->in_room]->people;    // make copy because inside loop this list might be changed.
				for (const auto &tch : people) {
					if (IS_IMMORTAL(tch) || ch->in_room == kNowhere || IN_ROOM(tch) == kNowhere)
						continue;
					if (tch != ch && !same_group(ch, tch)) {
						mag_damage(GetRealLevel(ch), ch, tch, spellnum, ESaving::kStability);
					}
				}
				return;
			}
			// а теперь просто дышащие
			mag_damage(GetRealLevel(ch), ch, victim, spellnum, ESaving::kStability);
			return;
		}
	}

	//go_autoassist(ch);

	// старт инициализации полей для удара
	HitData hit_params;
	//конвертация скиллов, которые предполагают вызов hit()
	//c тип_андеф, в тип_андеф.
	hit_params.skill_num = type != ESkill::kOverwhelm && type != ESkill::kHammer ? type : ESkill::kUndefined;
	hit_params.weapon = weapon;
	hit_params.init(ch, victim);

	//  дополнительный маг. дамаг независимо от попадания физ. атаки
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CLOUD_OF_ARROWS)
		&& hit_params.skill_num == ESkill::kUndefined
		&& (ch->get_fighting()
			|| (!GET_AF_BATTLE(ch, kEafHammer) && !GET_AF_BATTLE(ch, kEafOverwhelm)))) {
		// здесь можно получить спурженного victim, но ch не умрет от зеркала
    if (IS_NPC(ch)) {
		mag_damage(std::min(kLvlImplementator, GetRealLevel(ch)), ch, victim, kSpellMagicMissile, ESaving::kReflex);
	} else {
		mag_damage(1, ch, victim, kSpellMagicMissile, ESaving::kReflex);
    }
		if (ch->purged() || victim->purged()) {
			return;
		}
		auto skillnum = GetMagicSkillId(kSpellCloudOfArrows);
		TrainSkill(ch, skillnum, true, victim);
	}

	// вычисление хитролов/ац
	hit_params.calc_base_hr(ch);
	hit_params.calc_rand_hr(ch, victim);
	hit_params.calc_ac(victim);
	bool need_dice = false;
	hit_params.calc_damage(ch, need_dice); // попытка все собрать в кучу

	// рандом разброс базового дамага для красоты
	if (hit_params.dam > 0) {
		int min_rnd = hit_params.dam - hit_params.dam / 4;
		int max_rnd = hit_params.dam + hit_params.dam / 4;
		hit_params.dam = MAX(1, number(min_rnd, max_rnd));
	}

	const int victim_lvl_miss = victim->get_level() + GET_REAL_REMORT(victim);
	const int ch_lvl_miss = GetRealLevel(ch) + GET_REAL_REMORT(ch);

	// собсно выяснение попали или нет
	if (victim_lvl_miss - ch_lvl_miss <= 5 || (!IS_NPC(ch) && !IS_NPC(victim))) {
		// 5% шанс промазать, если цель в пределах 5 уровней или пвп случай
		if ((number(1, 100) <= 5)) {
			hit_params.dam = 0;
			hit_params.extdamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	} else {
		// шанс промазать = разнице уровней и мортов
		const int diff = victim_lvl_miss - ch_lvl_miss;
		if (number(1, 100) <= diff) {
			hit_params.dam = 0;
			hit_params.extdamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	}
	// всегда есть 5% вероятность попасть (diceroll == 20)
	if ((hit_params.diceroll < 20 && AWAKE(victim))
		&& hit_params.calc_thaco - hit_params.diceroll > hit_params.victim_ac) {
		hit_params.dam = 0;
		hit_params.extdamage(ch, victim);
		hitprcnt_mtrigger(victim);
		return;
	}
	// даже в случае попадания можно уклониться мигалкой
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_BLINK) || victim->add_abils.percent_spell_blink > 0) {

		if (!GET_AF_BATTLE(ch, kEafHammer) && !GET_AF_BATTLE(ch, kEafOverwhelm)
			&& (!(hit_params.skill_num == ESkill::kBackstab && can_use_feat(ch, THIEVES_STRIKE_FEAT)))) {
			ubyte blink;
			if (victim->is_npc()) {
				blink = 25;
			} else {
				blink = 5;
			}
			if (can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
				blink = 10 + GET_REAL_REMORT(ch) * 2 / 3;
			} else if (victim->add_abils.percent_spell_blink > 0) { //мигалка спеллом а не аффектом с шмотки
				blink = victim->add_abils.percent_spell_blink;
				ch->send_to_TC(false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
				victim->send_to_TC(false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
			}
			if (blink >= number(1, 100)) {
				sprintf(buf, "%sНа мгновение вы исчезли из поля зрения противника.%s\r\n",
						CCINRM(victim, C_NRM), CCNRM(victim, C_NRM));
				send_to_char(buf, victim);
				hit_params.dam = 0;
				hit_params.extdamage(ch, victim);
				return;
			}
		}
	}

	// расчет критических ударов
	hit_params.calc_crit_chance(ch);
/* не поймуу зачем 2 раза то?
	// зовется до alt_equip, чтобы не абузить повреждение пушек
	if (damage_mtrigger(ch, victim, hit_params.dam, skill_info[hit_params.skill_num].name, GetUsedWeapon(ch, weapon))) {
		return;
	}
*/
	if (hit_params.weapon_pos) {
		alt_equip(ch, hit_params.weapon_pos, hit_params.dam, 10);
	}

	if (hit_params.skill_num == ESkill::kBackstab) {
		hit_params.reset_flag(fight::kCritHit);
		hit_params.set_flag(fight::kIgnoreFireShield);
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT) || can_use_feat(ch, SHADOW_STRIKE_FEAT)) {
			hit_params.set_flag(fight::kIgnoreArmor);
		} else {
			hit_params.set_flag(fight::kHalfIgnoreArmor);
		}
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && IS_NPC(victim)) {
			hit_params.dam *= backstab_mult(GetRealLevel(ch)) * (1.0 + ch->get_skill(ESkill::kNoParryHit) / 200.0);
		} else if (can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
			if (victim->get_fighting()) {
				hit_params.dam *= backstab_mult(GetRealLevel(ch));
			} else {
				hit_params.dam *= backstab_mult(GetRealLevel(ch)) * 1.3;
			}
		} else {
			hit_params.dam *= backstab_mult(GetRealLevel(ch));
		}

		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA)
			&& IS_NPC(victim)
			&& !(AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD) && !(MOB_FLAGGED(victim, MOB_PROTECT)))
			&& (number(1, 100) <= 6 * ch->get_cond_penalty(P_HITROLL))
			&& !victim->get_role(MOB_ROLE_BOSS)) {
			GET_HIT(victim) = 1;
			hit_params.dam = victim->points.hit + fight::kLethalDmg;
			send_to_char(ch, "&GПрямо в сердце, насмерть!&n\r\n");
			hit_params.extdamage(ch, victim);
			return;
		}

		if (number(1, 100) < calculate_crit_backstab_percent(ch) * ch->get_cond_penalty(P_HITROLL)
			&& !CalcGeneralSaving(ch, victim, ESaving::kReflex, dex_bonus(GET_REAL_DEX(ch)))) {
			hit_params.dam = static_cast<int>(hit_params.dam * hit_params.crit_backstab_multiplier(ch, victim));
			if ((hit_params.dam > 0)
				&& !AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD)
				&& !(MOB_FLAGGED(victim, MOB_PROTECT))) {
				send_to_char("&GПрямо в сердце!&n\r\n", ch);
			}
		}

		hit_params.dam = ApplyResist(victim, VITALITY_RESISTANCE, hit_params.dam);
		// режем стаб
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && !IS_NPC(ch)) {
			hit_params.dam = std::min(8000 + GET_REAL_REMORT(ch) * 20 * GetRealLevel(ch), hit_params.dam);
		}

		ch->send_to_TC(false, true, false, "&CДамага стаба равна = %d&n\r\n", hit_params.dam);
		victim->send_to_TC(false, true, false, "&RДамага стаба  равна = %d&n\r\n", hit_params.dam);
		hit_params.extdamage(ch, victim);
		return;
	}

	if (hit_params.skill_num == ESkill::kThrow) {
		hit_params.set_flag(fight::kIgnoreFireShield);
		hit_params.dam *= (CalcCurrentSkill(ch, ESkill::kThrow, victim) + 10) / 10;
		if (IS_NPC(ch)) {
			hit_params.dam = std::min(300, hit_params.dam);
		}
		hit_params.dam = ApplyResist(victim, VITALITY_RESISTANCE, hit_params.dam);
		hit_params.extdamage(ch, victim);
		return;
	}

	if (!IS_CHARMICE(ch) && GET_AF_BATTLE(ch, kEafPunctual) && GET_PUNCTUAL_WAIT(ch) <= 0 && GET_WAIT(ch) <= 0
		&& (hit_params.diceroll >= 18 - GET_MOB_HOLD(victim))) {
		int percent = CalcCurrentSkill(ch, ESkill::kPunctual, victim);
		bool success = percent >= number(1, MUD::Skills()[ESkill::kPunctual].difficulty);
		TrainSkill(ch, ESkill::kPunctual, success, victim);
		if (!PUNCTUAL_WAITLESS(ch)) {
			PUNCTUAL_WAIT_STATE(ch, 1 * kPulseViolence);
		}
		if (success && (hit_params.calc_thaco - hit_params.diceroll < hit_params.victim_ac - 5
				|| percent >= MUD::Skills()[ESkill::kPunctual].difficulty)) {
			if (!MOB_FLAGGED(victim, MOB_NOTKILLPUNCTUAL)) {
				hit_params.set_flag(fight::kCritHit);
				// CRIT_HIT и так щиты игнорит, но для порядку
				hit_params.set_flag(fight::kIgnoreFireShield);
				hit_params.dam_critic = do_punctual(ch, victim, hit_params.wielded);
				ch->send_to_TC(false, true, false, "&CДамага точки равна = %d&n\r\n", hit_params.dam_critic);
				victim->send_to_TC(false, true, false, "&CДамага точки равна = %d&n\r\n", hit_params.dam_critic);
				if (!PUNCTUAL_WAITLESS(ch)) {
					PUNCTUAL_WAIT_STATE(ch, 2 * kPulseViolence);
				}
				CallMagic(ch, victim, nullptr, nullptr, ESpell::kSpellPaladineInspiration, ch->get_level());
			}
		}
	}

	ch->send_to_TC(false, true, true, "&CНанёс: Регуляр дамаг = %d&n\r\n", hit_params.dam);
	victim->send_to_TC(false, true, true, "&CПолучил: Регуляр дамаг = %d&n\r\n", hit_params.dam);
	// обнуляем флаги, если у нападающего есть лаг
	if ((GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) && GET_WAIT(ch) > 0) {
		CLR_AF_BATTLE(ch, kEafOverwhelm);
		CLR_AF_BATTLE(ch, kEafHammer);
	}

	// обработка защитных скилов (захват, уклон, парир, веер, блок)
	hit_params.check_defense_skills(ch, victim);


	// итоговый дамаг
	int made_dam = hit_params.extdamage(ch, victim);

	//Обнуление лага, когда виктим убит с применением
	//оглушить или молотить. Чтобы все это было похоже на
	//действие скиллов экстраатак(пнуть, сбить и т.д.)
/*
	if (CHECK_WAIT(ch)
		&& made_dam == -1
		&& (type == ESkill::kOverwhelm
			|| type == ESkill::kHammer))
	{
		ch->set_wait(0u);
	} */
	if (made_dam == -1) {
		if (type == ESkill::kOverwhelm) {
			ch->setSkillCooldown(ESkill::kOverwhelm, 0u);
		} else if (type == ESkill::kHammer) {
			ch->setSkillCooldown(ESkill::kHammer, 0u);
		}
	}

	// check if the victim has a hitprcnt trigger
	if (made_dam != -1) {
		// victim is not dead after hit
		hitprcnt_mtrigger(victim);
	}
}

// реализация скилла "железный ветер"
void performIronWindAttacks(CharData *ch, fight::AttackType weapon) {
	int percent = 0, prob = 0, div = 0, moves = 0;
	/*
	первая дополнительная атака правой наносится 100%
	вторая дополнительная атака правой начинает наноситься с 80%+ скилла, но не более чем с 80% вероятностью
	первая дополнительная атака левой начинает наноситься сразу, но не более чем с 80% вероятностью
	вторая дополнительная атака левей начинает наноситься с 170%+ скилла, но не более чем с 30% вероятности
	*/
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		percent = ch->get_skill(ESkill::kIronwind);
		moves = GET_MAX_MOVE(ch) / (6 + MAX(10, percent) / 10);
		prob = GET_AF_BATTLE(ch, kEafIronWind);
		if (prob && !check_moves(ch, moves)) {
			CLR_AF_BATTLE(ch, kEafIronWind);
		} else if (!prob && (GET_MOVE(ch) > moves)) {
			SET_AF_BATTLE(ch, kEafIronWind);
		};
	};
	if (GET_AF_BATTLE(ch, kEafIronWind)) {
		TrainSkill(ch, ESkill::kIronwind, true, ch->get_fighting());
		if (weapon == fight::kMainHand) {
			div = 100 + MIN(80, MAX(1, percent - 80));
			prob = 100;
		} else {
			div = MIN(80, percent + 10);
			prob = 80 - MIN(30, MAX(0, percent - 170));
		};
		while (div > 0) {
			if (number(1, 100) < div)
				hit(ch, ch->get_fighting(), ESkill::kUndefined, weapon);
			div -= prob;
		};
	};
}

// Обработка доп.атак
void exthit(CharData *ch, ESkill type, fight::AttackType weapon) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? (ch->purged() ? "purged" : "true") : "false", __FILE__, __LINE__);
		return;
	}
	// обрабатываем доп.атаки от железного ветра
	performIronWindAttacks(ch, weapon);

	ObjData *wielded = nullptr;

	wielded = GetUsedWeapon(ch, weapon);
	if (wielded
		&& !GET_EQ(ch, WEAR_SHIELD)
		&& static_cast<ESkill>(wielded->get_skill()) == ESkill::kBows
		&& GET_EQ(ch, WEAR_BOTHS)) {
		// Лук в обеих руках - юзаем доп. или двойной выстрел
		if (can_use_feat(ch, DOUBLESHOT_FEAT) && !ch->get_skill(ESkill::kAddshot)
			&& MIN(850, 200 + ch->get_skill(ESkill::kBows) * 4 + GET_REAL_DEX(ch) * 5) >= number(1, 1000)) {
			hit(ch, ch->get_fighting(), type, weapon);
		} else if (ch->get_skill(ESkill::kAddshot) > 0) {
			addshot_damage(ch, type, weapon);
		}
	}

	hit(ch, ch->get_fighting(), type, weapon);
}

int CalcPcDamrollBonus(CharData *ch) {
	const short kMaxRemortForDamrollBonus = 35;
	const short kRemortDamrollBonus[kMaxRemortForDamrollBonus + 1] =
		{0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8};
	int bonus = 0;
	if (IS_VIGILANT(ch) || IS_GUARD(ch) || IS_RANGER(ch)) {
		bonus = kRemortDamrollBonus[std::min(kMaxRemortForDamrollBonus, GET_REAL_REMORT(ch))];
	}
	if (can_use_feat(ch, BOWS_FOCUS_FEAT) && ch->get_skill(ESkill::kAddshot)) {
		bonus *= 3;
	}
	return bonus;
}

int CalcNpcDamrollBonus(CharData *ch) {
	int bonus = 0;
	if (GetRealLevel(ch) > kStrongMobLevel) {
		bonus += GetRealLevel(ch) * number(100, 200) / 100.0;
	}
	return bonus;
}

/**
* еще есть рандом дамролы, в данный момент максимум 30d127
*/
int GetRealDamroll(CharData *ch) {
	if (IS_NPC(ch) && !IS_CHARMICE(ch)) {
		return std::max(0, GET_DR(ch) + GET_DR_ADD(ch) + CalcNpcDamrollBonus(ch));
	}

	int bonus = CalcPcDamrollBonus(ch);
	return std::clamp(GET_DR(ch) + GET_DR_ADD(ch) + 2 * bonus, -50, (IS_MORTIFIER(ch) ? 100 : 50) + 2 * bonus);
}

int GetAutoattackDamroll(CharData *ch, int weapon_skill) {
	if (IS_NPC(ch) && !IS_CHARMICE(ch)) {
		return std::max(0, GET_DR(ch) + GET_DR_ADD(ch) + CalcNpcDamrollBonus(ch));
	}
	return std::min(GET_DR(ch) + GET_DR_ADD(ch) + 2 * CalcPcDamrollBonus(ch), weapon_skill / 2); //попробюуем так
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
