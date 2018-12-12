#include "fight_hit.hpp"

#include "logger.hpp"
#include "handler.h"
#include "screen.h"
#include "dg_scripts.h"
#include "pk.h"
#include "dps.hpp"
#include "fight.h"
#include "house_exp.hpp"
#include "poison.hpp"
#include "bonus.h"

// extern
int extra_aco(int class_num, int level);
void alt_equip(CHAR_DATA * ch, int pos, int dam, int chance);
int thaco(int class_num, int level);
void npc_groupbattle(CHAR_DATA * ch);
void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
void go_autoassist(CHAR_DATA * ch);

int armor_class_limit(CHAR_DATA * ch)
{
	if (IS_CHARMICE(ch))
	{
		return -200;
	};
	if (IS_NPC(ch))
	{
		return -300;
	};
	switch (GET_CLASS(ch))
	{
	case CLASS_ASSASINE:
	case CLASS_THIEF:
	case CLASS_GUARD:
		return -250;
		break;
	case CLASS_MERCHANT:
	case CLASS_WARRIOR:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		return -200;
		break;
	case CLASS_CLERIC:
	case CLASS_DRUID:
		return -170;
		break;
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		return -150;
		break;
	}
	return -300;
}

int compute_armor_class(CHAR_DATA * ch)
{
	int armorclass = GET_REAL_AC(ch);

	if (AWAKE(ch))
	{
		int high_stat = GET_REAL_DEX(ch);
		// у игроков для ац берется макс стат: ловкость или 3/4 инты
		if (!IS_NPC(ch))
		{
			high_stat = std::max(high_stat, GET_REAL_INT(ch) * 3 / 4);
		}
		armorclass -= dex_ac_bonus(high_stat) * 10;
		armorclass += extra_aco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	};

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK))
	{
		armorclass -= (240 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
	}

	// Gorrah: штраф к КЗ от применения умения "железный ветер"
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND))
	{
		armorclass += ch->get_skill(SKILL_IRON_WIND) / 2;
	}

	armorclass += (size_app[GET_POS_SIZE(ch)].ac * 10);

	if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
	{
		if (GET_EQ(ch, WEAR_WIELD))
		{
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

	armorclass = MIN(100, armorclass);
	return (MAX(armor_class_limit(ch), armorclass));
}

void haemorragia(CHAR_DATA * ch, int percent)
{
	AFFECT_DATA<EApplyLocation> af[3];

	af[0].type = SPELL_HAEMORRAGIA;
	af[0].location = APPLY_HITREG;
	af[0].modifier = -percent;
	//TODO: Отрицательное время, если тело больше 31?
	af[0].duration = pc_duration(ch, number(1, 31 - GET_REAL_CON(ch)), 0, 0, 0, 0);
	af[0].bitvector = 0;
	af[0].battleflag = 0;
	af[1].type = SPELL_HAEMORRAGIA;
	af[1].location = APPLY_MOVEREG;
	af[1].modifier = -percent;
	af[1].duration = af[0].duration;
	af[1].bitvector = 0;
	af[1].battleflag = 0;
	af[2].type = SPELL_HAEMORRAGIA;
	af[2].location = APPLY_MANAREG;
	af[2].modifier = -percent;
	af[2].duration = af[0].duration;
	af[2].bitvector = 0;
	af[2].battleflag = 0;

	for (int i = 0; i < 3; i++)
	{
		affect_join(ch, af[i], TRUE, FALSE, TRUE, FALSE);
	}
}
void inspiration(CHAR_DATA *ch, int time, int mod)
{
	AFFECT_DATA<EApplyLocation> af;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
	{
		CHAR_DATA *k;
		struct follow_type *f;
		if (ch->has_master())
		{
			k = ch->get_master();
		}
		else
		{
			k = ch;
		}
		for (f = k->followers; f; f = f->next)
		{
			if (f->follower == ch)
				continue;
			if (f->follower->in_room == ch->in_room)
			{
				af.location = APPLY_DAMROLL;
				af.type = SPELL_PALADINE_INSPIRATION;
				af.modifier = GET_REMORT(ch) / 5 * 2;
				af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
				af.duration = pc_duration(ch, time, 0, 0, 0, 0);
				affect_join(f->follower, af, FALSE, FALSE, FALSE, FALSE);
				af.location = APPLY_CAST_SUCCESS;
				af.type = SPELL_PALADINE_INSPIRATION;
				af.modifier = GET_REMORT(ch) / 5 * mod;
				af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
				af.duration = pc_duration(ch, time, 0, 0, 0, 0);
				affect_join(f->follower, af, FALSE, FALSE, FALSE, FALSE);
				send_to_char(f->follower, "&YТочный удар %s воодушевил вас, придав новых сил!&n\r\n", GET_PAD(ch,1));
			}
		}
		
	}
		af.location = APPLY_DAMROLL;
		af.type = SPELL_PALADINE_INSPIRATION;
		af.modifier = GET_REMORT(ch) / 5 * 2;
		af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
		af.duration = pc_duration(ch, time, 0, 0, 0, 0);
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
		af.location = APPLY_CAST_SUCCESS;
		af.type = SPELL_PALADINE_INSPIRATION;
		af.modifier = GET_REMORT(ch) / 5 * mod;
		af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
		af.duration = pc_duration(ch, time, 0, 0, 0, 0);
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
		send_to_char(ch, "&YВаш точный удар воодушевил вас, придав новых сил!&n\r\n");
}

void HitData::compute_critical(CHAR_DATA * ch, CHAR_DATA * victim)
{
	const char *to_char = NULL, *to_vict = NULL;
	AFFECT_DATA<EApplyLocation> af[4];
	OBJ_DATA *obj;
	int unequip_pos = 0;

	for (int i = 0; i < 4; i++)
	{
		af[i].type = 0;
		af[i].location = APPLY_NONE;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].duration = pc_duration(victim, 2, 0, 0, 0, 0);
	}

	switch (number(1, 10))
	{
	case 1:
	case 2:
	case 3:
	case 4:		// FEETS
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// Nothing
			return;
		case 4:	// Hit genus, victim bashed, speed/2
			SET_AF_BATTLE(victim, EAF_SLOW);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 10);
			if (GET_POS(victim) > POS_SITTING)
				GET_POS(victim) = POS_SITTING;
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "повалило $N3 на землю";
			to_vict = "повредило вам колено, повалив на землю";
			break;
		case 5:	// victim bashed
			if (GET_POS(victim) > POS_SITTING)
				GET_POS(victim) = POS_SITTING;
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "повалило $N3 на землю";
			to_vict = "повредило вам колено, повалив на землю";
			break;
		case 6:	// foot damaged, speed/2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 9);
			to_char = "замедлило движения $N1";
			to_vict = "сломало вам лодыжку";
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 7:
		case 9:	// armor damaged else foot damaged, speed/4
			if (GET_EQ(victim, WEAR_LEGS))
				alt_equip(victim, WEAR_LEGS, 100, 100);
			else
			{
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 8);
				to_char = "замедлило движения $N1";
				to_vict = "сломало вам ногу";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
				SET_AF_BATTLE(victim, EAF_SLOW);
			}
			break;
		case 8:	// femor damaged, no speed
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "сильно замедлило движения $N1";
			to_vict = "сломало вам бедро";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// genus damaged, no speed, -2HR
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "сильно замедлило движения $N1";
			to_vict = "раздробило вам колено";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// femor damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "вывело $N3 из строя";
			to_vict = "раздробило вам бедро";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		default:	// femor damaged, no speed, no attack
			if (dam_critic > 12)
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			else
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "вывело $N3 из строя";
			to_vict = "изуродовало вам ногу";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 50);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	case 5:		//  ABDOMINAL
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// nothing
			return;
		case 4:	// waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			to_char = "сбило $N2 дыхание";
			to_vict = "сбило вам дыхание";
			break;

		case 5:	// abdomin damaged, waits 1, speed/2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 8);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "ранило $N3 в живот";
			to_vict = "ранило вас в живот";
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 6:	// armor damaged else dam*3, waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_WAIST))
				alt_equip(victim, WEAR_WAIST, 100, 100);
			else
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "повредило $N2 живот";
			to_vict = "повредило вам живот";
			break;
		case 7:
		case 8:	// abdomin damage, speed/2, HR-2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "ранило $N3 в живот";
			to_vict = "ранило вас в живот";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 9:	// armor damaged, abdomin damaged, speed/2, HR-2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			alt_equip(victim, WEAR_BODY, 100, 100);
			to_char = "ранило $N3 в живот";
			to_vict = "ранило вас в живот";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// abdomin damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "повредило $N2 живот";
			to_vict = "повредило вам живот";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// abdomin damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 3);
			to_char = "разорвало $N2 живот";
			to_vict = "разорвало вам живот";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 40);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		default:	// abdomin damaged, hits = 0
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			to_char = "размозжило $N2 живот";
			to_vict = "размозжило вам живот";
			haemorragia(victim, 60);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	case 6:
	case 7:		// CHEST
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// nothing
			return;
		case 4:	// waits 1d4, bashed
			WAIT_STATE(victim, number(2, 5) * PULSE_VIOLENCE);
			if (GET_POS(victim) > POS_SITTING)
				GET_POS(victim) = POS_SITTING;
			to_char = "повредило $N2 грудь, свалив $S с ног";
			to_vict = "повредило вам грудь, свалив вас с ног";
			break;
		case 5:	// chest damaged, waits 1, speed/2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "повредило $N2 туловище";
			to_vict = "повредило вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 6:	// shield damaged, chest damaged, speed/2
			alt_equip(victim, WEAR_SHIELD, 100, 100);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "повредило $N2 туловище";
			to_vict = "повредило вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 7:	// srmor damaged, chest damaged, speed/2, HR-2
			alt_equip(victim, WEAR_BODY, 100, 100);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			to_char = "повредило $N2 туловище";
			to_vict = "повредило вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 8:	// chest damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			to_char = "вывело $N3 из строя";
			to_vict = "повредило вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 9:	// chest damaged, speed/2, HR-2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "заставило $N3 ослабить натиск";
			to_vict = "сломало вам ребра";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// chest damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "вывело $N3 из строя";
			to_vict = "сломало вам ребра";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 40);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// chest crushed, hits 0
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			haemorragia(victim, 50);
			to_char = "вывело $N3 из строя";
			to_vict = "разорвало вам грудь";
			break;
		default:	// chest crushed, killing
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			haemorragia(victim, 60);
			to_char = "вывело $N3 из строя";
			to_vict = "размозжило вам грудь";
			break;
		}
		break;
	case 8:
	case 9:		// HANDS
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			return;
		case 4:	// hands damaged, weapon/shield putdown
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
		case 5:	// hands damaged, shield damaged/weapon putdown
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

		case 6:	// hands damaged, HR-2, shield putdown
			to_char = "ослабило натиск $N1";
			to_vict = "сломало вам руку";
			if (GET_EQ(victim, WEAR_SHIELD))
				unequip_pos = WEAR_SHIELD;
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			break;
		case 7:	// armor damaged, hand damaged if no armour
			if (GET_EQ(victim, WEAR_ARMS))
				alt_equip(victim, WEAR_ARMS, 100, 100);
			else
				alt_equip(victim, WEAR_HANDS, 100, 100);
			if (!GET_EQ(victim, WEAR_ARMS) && !GET_EQ(victim, WEAR_HANDS))
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "ослабило атаку $N1";
			to_vict = "повредило вам руку";
			break;
		case 8:	// shield damaged, hands damaged, waits 1
			alt_equip(victim, WEAR_SHIELD, 100, 100);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "придержало $N3";
			to_vict = "повредило вам руку";
			break;
		case 9:	// weapon putdown, hands damaged, waits 1d4
			WAIT_STATE(victim, number(2, 4) * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "придержало $N3";
			to_vict = "повредило вам руку";
			break;
		case 10:	// hand damaged, no attack this
			if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT))
			{
				to_char = "ослабило атаку $N1";
				to_vict = "изуродовало вам правую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT))
			{
				to_char = "ослабило атаку $N1";
				to_vict = "изуродовало вам левую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else
			{
				to_char = "вывело $N3 из строя";
				to_vict = "вывело вас из строя";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			haemorragia(victim, 20);
			break;
		default:	// no hand attack, no speed, dam*2 if >= 13
			if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT))
			{
				to_char = "ослабило натиск $N1";
				to_vict = "изуродовало вам правую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT))
			{
				to_char = "ослабило натиск $N1";
				to_vict = "изуродовало вам левую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else
			{
				to_char = "вывело $N3 из строя";
				to_vict = "вывело вас из строя";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 30);
			if (dam_critic >= 13)
				dam *= ch->get_skill(SKILL_PUNCTUAL) / 5;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	default:		// HEAD
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// nothing
			return;
		case 4:	// waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			to_char = "помутило $N2 сознание";
			to_vict = "помутило ваше сознание";
			break;

		case 5:	// head damaged, cap putdown, waits 1, HR-2 if no cap
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_HEAD))
				unequip_pos = WEAR_HEAD;
			else
			{
				af[0].type = SPELL_BATTLE;
				af[0].location = APPLY_HITROLL;
				af[0].modifier = -2;
			}
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "повредило $N2 голову";
			to_vict = "повредило вам голову";
			break;
		case 6:	// head damaged
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "повредило $N2 голову";
			to_vict = "повредило вам голову";
			break;
		case 7:	// cap damaged, waits 1d6, speed/2, HR-4
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			alt_equip(victim, WEAR_HEAD, 100, 100);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -4;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			to_char = "ранило $N3 в голову";
			to_vict = "ранило вас в голову";
			break;
		case 8:	// cap damaged, hits 0
			WAIT_STATE(victim, 4 * PULSE_VIOLENCE);
			alt_equip(victim, WEAR_HEAD, 100, 100);
			//dam = GET_HIT(victim);
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			to_char = "отбило у $N1 сознание";
			to_vict = "отбило у вас сознание";
			haemorragia(victim, 20);
			break;
		case 9:	// head damaged, no speed, no attack
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			haemorragia(victim, 30);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 3);
			to_char = "повергло $N3 в оцепенение";
			to_vict = "повергло вас в оцепенение";
			break;
		case 10:	// head damaged, -1 INT/WIS/CHA
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 2);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -1;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -1;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -1;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			af[3].type = SPELL_BATTLE;
			af[3].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[3].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[3].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			haemorragia(victim, 50);
			to_char = "сорвало у $N1 крышу";
			to_vict = "сорвало у вас крышу";
			break;
		case 11:	// hits 0, WIS/2, INT/2, CHA/2
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -victim->get_int() / 2;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -victim->get_wis() / 2;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -victim->get_cha() / 2;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			haemorragia(victim, 60);
			to_char = "сорвало у $N1 крышу";
			to_vict = "сорвало у вас крышу";
			break;
		default:	// killed
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -victim->get_int() / 2;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -victim->get_wis() / 2;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -victim->get_cha() / 2;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			to_char = "размозжило $N2 голову";
			to_vict = "размозжило вам голову";
			haemorragia(victim, 90);
			break;
		}
		break;
	}
	if (to_char)
	{
		sprintf(buf, "&G&qВаше точное попадание %s.&Q&n", to_char);
		act(buf, FALSE, ch, 0, victim, TO_CHAR);
		sprintf(buf, "Точное попадание $n1 %s.", to_char);
		act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}

	if (to_vict)
	{
		sprintf(buf, "&R&qМеткое попадание $n1 %s.&Q&n", to_vict);
		act(buf, FALSE, ch, 0, victim, TO_VICT);
	}
	if (unequip_pos && GET_EQ(victim, unequip_pos))
	{
		obj = unequip_char(victim, unequip_pos);
		switch (unequip_pos)
		{
			case 6:		//WEAR_HEAD
				sprintf(buf, "%s слетел%s с вашей головы.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s слетел%s с головы $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;

			case 11:	//WEAR_SHIELD
				sprintf(buf, "%s слетел%s с вашей руки.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s слетел%s с руки $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;

			case 16:	//WEAR_WIELD
			case 17:	//WEAR_HOLD
				sprintf(buf, "%s выпал%s из вашей руки.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s выпал%s из руки $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;

			case 18:	//WEAR_BOTHS
				sprintf(buf, "%s выпал%s из ваших рук.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s выпал%s из рук $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;
		}
		if (!IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
			obj_to_char(obj, victim);
		else
			obj_to_room(obj, IN_ROOM(victim));
		obj_decay(obj);
	}
	if (!IS_NPC(victim))
	{
		dam /= 5;
	}
	dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, dam);
	bool affect_found = false;
	for (int i = 0; i < 4; i++)
	{
		if (af[i].type)
		{
			if (af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPFIGHT)
				|| af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPRIGHT)
				|| af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPLEFT))
			{
				if (victim->get_role(MOB_ROLE_BOSS))
				{
					af[i].duration /= 5;
					// вес оружия тоже влияет на длит точки, офф проходит реже, берем вес прайма.
					sh_int extra_duration = 0;
					OBJ_DATA* both = GET_EQ(ch, WEAR_BOTHS);
					OBJ_DATA* wield = GET_EQ(ch, WEAR_WIELD);
					if (both)
					{
						extra_duration = GET_OBJ_WEIGHT(both) / 5;
					}
					else if (wield)
					{
						extra_duration = GET_OBJ_WEIGHT(wield) / 5;
					}
					af[i].duration += pc_duration(victim, GET_REMORT(ch)/2 + extra_duration, 0, 0, 0, 0);
				}
				if (!affect_found)
				{
					inspiration(ch, 2, 3);
					affect_found = true;
				}
			}
			affect_join(victim, af[i], TRUE, FALSE, TRUE, FALSE);
		}
		if (!affect_found)
		{
			inspiration(ch, 1, 1);
			affect_found = true;
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
int calculate_strconc_damage(CHAR_DATA * ch, OBJ_DATA* /*wielded*/, int damage)
{
	if (IS_NPC(ch)
		|| GET_REAL_STR(ch) <= 25
		|| !can_use_feat(ch, STRENGTH_CONCETRATION_FEAT)
		|| GET_AF_BATTLE(ch, EAF_IRON_WIND)
		|| GET_AF_BATTLE(ch, EAF_STUPOR))
	{
		return damage;
	}
	float str_mod = (GET_REAL_STR(ch) - 25) * 0.4;
	float lvl_mod = GET_LEVEL(ch) * 0.2;
	float rmt_mod = MIN(5, GET_REMORT(ch)) / 5.0;
	float res_mod = 1 + (str_mod + lvl_mod) / 10.0 * rmt_mod;

	return static_cast<int>(damage * res_mod);
}

/**
* Расчет прибавки дамаги со скрытого стиля.
* (скилл/5 + реморты*3) * (среднее/(10 + среднее/2)) * (левел/30)
*/
int calculate_noparryhit_dmg(CHAR_DATA * ch, OBJ_DATA * wielded)
{
	if (!ch->get_skill(SKILL_NOPARRYHIT)) return 0;

	float weap_dmg = (((GET_OBJ_VAL(wielded, 2) + 1) / 2.0) * GET_OBJ_VAL(wielded, 1));
	float weap_mod = weap_dmg / (10 + weap_dmg / 2);
	float level_mod = static_cast<float>(GET_LEVEL(ch)) / 30;
	float skill_mod = static_cast<float>(ch->get_skill(SKILL_NOPARRYHIT)) / 5;

	return static_cast<int>((skill_mod + GET_REMORT(ch) * 3) * weap_mod * level_mod);
}

void might_hit_bash(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (MOB_FLAGGED(victim, MOB_NOBASH) || !GET_MOB_HOLD(victim))
	{
		return;
	}

	act("$n обреченно повалил$u на землю.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	WAIT_STATE(victim, 3 * PULSE_VIOLENCE);

	if (GET_POS(victim) > POS_SITTING)
	{
		GET_POS(victim) = POS_SITTING;
		send_to_char(victim, "&R&qБогатырский удар %s сбил вас с ног.&Q&n\r\n", PERS(ch, victim, 1));
	}
}

bool check_mighthit_weapon(CHAR_DATA *ch)
{
	if (!GET_EQ(ch, WEAR_BOTHS)
		&& !GET_EQ(ch, WEAR_WIELD)
		&& !GET_EQ(ch, WEAR_HOLD)
		&& !GET_EQ(ch, WEAR_LIGHT)
		&& !GET_EQ(ch, WEAR_SHIELD))
	{
			return true;
	}
	return false;
}

// * При надуве выше х 1.5 в пк есть 1% того, что весь надув слетит одним ударом.
void try_remove_extrahits(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (((!IS_NPC(ch) && ch != victim)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master())
				&& ch->get_master() != victim))
		&& !IS_NPC(victim)
		&& GET_POS(victim) != POS_DEAD
		&& GET_HIT(victim) > GET_REAL_MAX_HIT(victim) * 1.5
		&& number(1, 100) == 5)// пусть будет 5, а то 1 по субъективным ощущениям выпадает как-то часто
	{
		GET_HIT(victim) = GET_REAL_MAX_HIT(victim);
		send_to_char(victim, "%s'Будь%s тощ%s аки прежде' - мелькнула чужая мысль в вашей голове.%s\r\n",
				CCWHT(victim, C_NRM), GET_CH_POLY_1(victim), GET_CH_EXSUF_1(victim), CCNRM(victim, C_NRM));
		act("Вы прервали золотистую нить, питающую $N3 жизнью.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n прервал$g золотистую нить, питающую $N3 жизнью.", FALSE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}
}

/**
* Дамажим дополнительным выстрелом.
* Проценты при 100 скилле и 21 ловки: 100  60 30 10
* Проценты при 200 скилее и 50 ловки: 100 100 70 30
* На заднице 70% от итоговых процентов.
* У чармисов 100% и 60% без остальных допов.
*/
void addshot_damage(CHAR_DATA * ch, int type, int weapon)
{
	int prob = train_skill(ch, SKILL_ADDSHOT, skill_info[SKILL_ADDSHOT].max_percent, ch->get_fighting());

	// ловка роляет только выше 21 (стартовый максимум охота) и до 50
	float dex_mod = static_cast<float>(MAX(MIN(GET_REAL_DEX(ch), 50) - 21, 0)) / 29 * 2;  //поставим кап в 50 ловки для допвыстрела
	// штраф на 4й и 5й выстрелы для чаров ниже 5 мортов, по 20% за морт
	float remort_mod = static_cast<float>(GET_REMORT(ch)) / 5;
	if (remort_mod > 1) remort_mod = 1;
	// на жопе процентовка снижается на 70% от текущего максимума каждого допа
	float sit_mod = (GET_POS(ch) >= POS_FIGHTING) ? 1 : 0.7;

	// у чармисов никаких плюшек от 100+ скилла и максимум 2 доп атаки
	if (IS_CHARMICE(ch))
	{
		prob = MIN(100, prob);
		dex_mod = 0;
		remort_mod = 0;
	}

	// выше 100% идет другая формула
	// скилл выше 100 добавляет равный ловке процент в максимуме
	// а для скилла до сотни все остается как было
	prob = MIN(100, prob);

	int percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 1й доп - не более 100% при скилее 100+
	if (prob * sit_mod >= percent / 2)
		hit(ch, ch->get_fighting(), type, weapon);

	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 2й доп - 60% при скилле 100, до 100% при максимуме скилла и дексы
	if ((prob * 3 + dex_mod * 100) * sit_mod > percent * 5 / 2 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);

	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 3й доп - 30% при скилле 100, до 70% при максимуме скилла и дексы (при 5+ мортов)
	if ((prob * 3 + dex_mod * 200) * remort_mod * sit_mod > percent * 5 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);

	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 4й доп - 20% при скилле 100, до 60% при максимуме скилла и дексы (при 5+ мортов)
	if ((prob + dex_mod * 100) * remort_mod * sit_mod > percent * 5 / 2 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);
}

// бонусы/штрафы классам за юзание определенных видов оружия
void apply_weapon_bonus(int ch_class, const ESkill skill, int *damroll, int *hitroll)
{
	int dam = *damroll;
	int calc_thaco = *hitroll;

	switch (ch_class)
	{
	case CLASS_CLERIC:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 2;
			dam -= 1;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco += 1;
			dam -= 2;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;

		default:
			break;
		}
		break;

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam -= 3;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_WARRIOR:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco += 2;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco += 2;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_RANGER:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_GUARD:
	case CLASS_THIEF:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_ASSASINE:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;
		/*	case CLASS_PALADINE:
			case CLASS_SMITH:
				switch (skill) {
					case SKILL_CLUBS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_AXES:	calc_thaco -= 0; dam += 0; break;
					case SKILL_LONGS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_NONSTANDART:	calc_thaco -= 0; dam += 0; break;
					case SKILL_BOTHHANDS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_PICK:	calc_thaco -= 0; dam += 0; break;
					case SKILL_SPADES:	calc_thaco -= 0; dam += 0; break;
					case SKILL_BOWS:	calc_thaco -= 0; dam += 0; break;
				}
				break; */
	case CLASS_MERCHANT:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_DRUID:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco += 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco += 1;
			dam += 0;
			break;
		default:
			break;
		}
		break;
	}

	*damroll = dam;
	*hitroll = calc_thaco;
}

int do_punctual(CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA *wielded)
{
	int dam_critic = 0, wapp = 0;

	if (wielded)
	{
		wapp = (int)(GET_OBJ_SKILL(wielded) == SKILL_BOWS) ?
		   GET_OBJ_WEIGHT(wielded) * 1 / 3 : GET_OBJ_WEIGHT(wielded);
	}

	if (wapp < 10)
		dam_critic = dice(1, 6);
	else if (wapp < 19)
		dam_critic = dice(2, 5);
	else if (wapp < 27)
		dam_critic = dice(3, 4);
	else if (wapp < 36)
		dam_critic = dice(3, 5);
	else if (wapp < 44)
		dam_critic = dice(3, 6);
	else
		dam_critic = dice(4, 5);

	const int skill = 1 + ch->get_skill(SKILL_PUNCTUAL) / 6;
	dam_critic = MIN(number(4, skill), dam_critic);

	return dam_critic;
}

// * Умножение дамаги при стабе.
int backstab_mult(int level)
{
	if (level <= 0)
		return 1;	// level 0 //
	else if (level <= 5)
		return 2;	// level 1 - 5 //
	else if (level <= 10)
		return 3;	// level 6 - 10 //
	else if (level <= 15)
		return 4;	// level 11 - 15 //
	else if (level <= 20)
		return 5;	// level 16 - 20 //
	else if (level <= 25)
		return 6;	// level 21 - 25 //
	else if (level <= 30)
		return 7;	// level 26 - 30 //
	else
		return 10;
}

/**
* Процент прохождения крит.стаба = скилл/11 + (декса-20)/(декса/30)
* Влияение по 50% от скилла и дексы, максимум 36,18%.
*/
int calculate_crit_backstab_percent(CHAR_DATA *ch)
{
	return static_cast<int>(ch->get_skill(SKILL_BACKSTAB)/11.0 + (GET_REAL_DEX(ch) - 20) / (GET_REAL_DEX(ch) / 30.0));
}

// * Расчет множителя крит.стаба (по игрокам только для татей).
double HitData::crit_backstab_multiplier(CHAR_DATA *ch, CHAR_DATA *victim)
{
	double bs_coeff = 1.0;
	if (IS_NPC(victim))
	{
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
		{
			bs_coeff *= ch->get_skill(SKILL_BACKSTAB) / 20.0;
		}
		else
		{
			bs_coeff *= ch->get_skill(SKILL_BACKSTAB) / 25.0;
		}

		// Читаем справку по скрытому: Если нанести такой удар в спину противника (ака стаб), то почти
		// всегда для жертвы такой удар будет смертельным. Однако, в коде скрытый нигде не дает
		// бонусов для стаба. Решил это исправить
		// Проверяем, наем ли наш игрок
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT)
			&& (ch->get_skill(SKILL_NOPARRYHIT)))
		{
			bs_coeff *= (1 + (ch->get_skill(SKILL_NOPARRYHIT) * 0.00125));
		}
	}
	else if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
	{
		if (victim->get_fighting()) //если враг в бою коэфф 1.125
		{
			bs_coeff *= (1.0 + (ch->get_skill(SKILL_BACKSTAB) * 0.00125));
			// если стоит 1.25
		}
		else
		{
			bs_coeff *= (1.0 + (ch->get_skill(SKILL_BACKSTAB) * 0.00250));
		}
		// санку и призму при крите игнорим,
		// чтобы дамаг был более-менее предсказуемым
		set_flag(FightSystem::IGNORE_SANCT);
		set_flag(FightSystem::IGNORE_PRISM);
	}

	if (bs_coeff < 1.0)
	{
		bs_coeff = 1.0;
	}

	return bs_coeff;
}

// * Может ли персонаж блокировать атаки автоматом (вообще в данный момент, без учета лагов).
bool can_auto_block(CHAR_DATA *ch)
{
	if (GET_EQ(ch, WEAR_SHIELD) && GET_AF_BATTLE(ch, EAF_AWAKE) && GET_AF_BATTLE(ch, EAF_AUTOBLOCK))
		return true;
	else
		return false;
}

// * Проверка на фит "любимое оружие".
void HitData::check_weap_feats(CHAR_DATA *ch)
{
	switch (weap_skill)
	{
	case SKILL_PUNCH:
		if (HAVE_FEAT(ch, PUNCH_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 2;
		}
		break;
	case SKILL_CLUBS:
		if (HAVE_FEAT(ch, CLUB_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 2;
		}
		break;
	case SKILL_AXES:
		if (HAVE_FEAT(ch, AXES_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 2;
		}
		break;
	case SKILL_LONGS:
		if (HAVE_FEAT(ch, LONGS_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 2;
		}
		break;
	case SKILL_SHORTS:
		if (HAVE_FEAT(ch, SHORTS_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 3;
		}
		break;
	case SKILL_NONSTANDART:
		if (HAVE_FEAT(ch, NONSTANDART_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 3;
		}
		break;
	case SKILL_BOTHHANDS:
		if (HAVE_FEAT(ch, BOTHHANDS_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 3;
		}
		break;
	case SKILL_PICK:
		if (HAVE_FEAT(ch, PICK_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 3;
		}
		break;
	case SKILL_SPADES:
		if (HAVE_FEAT(ch, SPADES_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 2;
		}
		break;
	case SKILL_BOWS:
		if (HAVE_FEAT(ch, BOWS_FOCUS_FEAT))
		{
			calc_thaco -= 7;
			dam += 4;
		}
		break;
	default:
		break;
	}
}

// * Захват.
void hit_touching(CHAR_DATA *ch, CHAR_DATA *vict, int *dam)
{
	if (vict->get_touching() == ch
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPRIGHT)
		&& GET_WAIT(vict) <= 0
		&& !GET_MOB_HOLD(vict)
		&& (IS_IMMORTAL(vict) || IS_NPC(vict)
			|| !(GET_EQ(vict, WEAR_WIELD) || GET_EQ(vict, WEAR_BOTHS)))
		&& GET_POS(vict) > POS_SLEEPING)
	{
		int percent = number(1, skill_info[SKILL_TOUCH].max_percent);
		int prob = train_skill(vict, SKILL_TOUCH, skill_info[SKILL_TOUCH].max_percent, ch);
		if (IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE))
		{
			percent = prob;
		}
		if (GET_GOD_FLAG(vict, GF_GODSCURSE))
		{
			percent = 0;
		}
		CLR_AF_BATTLE(vict, EAF_TOUCH);
		SET_AF_BATTLE(vict, EAF_USEDRIGHT);
		vict->set_touching(0);
		if (prob < percent)
		{
			act("Вы не смогли перехватить атаку $N1.", FALSE, vict, 0, ch, TO_CHAR);
			act("$N не смог$Q перехватить вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n не смог$q перехватить атаку $N1.", TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			prob = 2;
		}
		else
		{
			act("Вы перехватили атаку $N1.", FALSE, vict, 0, ch, TO_CHAR);
			act("$N перехватил$G вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n перехватил$g атаку $N1.", TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			*dam = -1;
			prob = 1;
		}
		if (!WAITLESS(vict))
		{
			WAIT_STATE(vict, prob * PULSE_VIOLENCE);
		}
	}
}

void hit_deviate(CHAR_DATA *ch, CHAR_DATA *victim, int *dam)
{
	int range = number(1, skill_info[SKILL_DEVIATE].max_percent);
	int prob = train_skill(victim, SKILL_DEVIATE, skill_info[SKILL_DEVIATE].max_percent, ch);
	if (GET_GOD_FLAG(victim, GF_GODSCURSE))
	{
		prob = 0;
	}
	prob = prob * 100 / range;
	if (check_spell_on_player(victim, SPELL_WEB))
	{
		prob /= 3;
	}
	if (prob < 60)
	{
		act("Вы не смогли уклониться от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N не сумел$G уклониться от вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
		act("$n не сумел$g уклониться от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
                SET_AF_BATTLE(victim, EAF_DEVIATE);
}
	else if (prob < 100)
	{
		act("Вы немного уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N немного уклонил$U от вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
		act("$n немного уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		*dam = *dam * 10 / 15;
                SET_AF_BATTLE(victim, EAF_DEVIATE);
	}
	else if (prob < 200)
	{
		act("Вы частично уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N частично уклонил$U от вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
		act("$n частично уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		*dam = *dam / 2;
                SET_AF_BATTLE(victim, EAF_DEVIATE);
	}
	else
	{
		act("Вы уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N уклонил$U от вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
		act("$n уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		*dam = -1;
                SET_AF_BATTLE(victim, EAF_DEVIATE);
	}
	BATTLECNTR(victim)++;
}

void hit_parry(CHAR_DATA *ch, CHAR_DATA *victim, int skill, int hit_type, int *dam)
{
	if (!((GET_EQ(victim, WEAR_WIELD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON
			&& GET_EQ(victim, WEAR_HOLD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim)))
	{
		send_to_char("У вас нечем отклонить атаку противника\r\n", victim);
		CLR_AF_BATTLE(victim, EAF_PARRY);
	}
	else
	{
		int range = number(1, skill_info[SKILL_PARRY].max_percent);
		int prob = train_skill(victim, SKILL_PARRY,
				skill_info[SKILL_PARRY].max_percent, ch);
		prob = prob * 100 / range;

		if (prob < 70
			|| ((skill == SKILL_BOWS || hit_type == FightSystem::type_maul)
				&& !IS_IMMORTAL(victim)
				&& (!can_use_feat(victim, PARRY_ARROW_FEAT)
				|| number(1, 1000) >= 20 * MIN(GET_REAL_DEX(victim), 35))))
		{
			act("Вы не смогли отбить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N не сумел$G отбить вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n не сумел$g отбить атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			prob = 2;
			SET_AF_BATTLE(victim, EAF_USEDLEFT);
		}
		else if (prob < 100)
		{
			act("Вы немного отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N немного отклонил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n немного отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 10);
			prob = 1;
			*dam = *dam * 10 / 15;
			SET_AF_BATTLE(victim, EAF_USEDLEFT);
		}
		else if (prob < 170)
		{
			act("Вы частично отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N частично отклонил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n частично отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 15);
			prob = 0;
			*dam = *dam / 2;
			SET_AF_BATTLE(victim, EAF_USEDLEFT);
		}
		else
		{
			act("Вы полностью отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N полностью отклонил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n полностью отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 25);
			prob = 0;
			*dam = -1;
		}
		if (!WAITLESS(ch) && prob)
		{
			WAIT_STATE(victim, PULSE_VIOLENCE * prob);
		}
		CLR_AF_BATTLE(victim, EAF_PARRY);
	}
}

void hit_multyparry(CHAR_DATA *ch, CHAR_DATA *victim, int skill, int hit_type, int *dam)
{
	if (!((GET_EQ(victim, WEAR_WIELD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON
			&& GET_EQ(victim, WEAR_HOLD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim)))
	{
		send_to_char("У вас нечем отклонять атаки противников\r\n", victim);
	}
	else
	{
		int range = number(1,
				skill_info[SKILL_MULTYPARRY].max_percent) + 15 * BATTLECNTR(victim);
		int prob = train_skill(victim, SKILL_MULTYPARRY,
				skill_info[SKILL_MULTYPARRY].max_percent + BATTLECNTR(ch) * 15, ch);
		prob = prob * 100 / range;

		if ((skill == SKILL_BOWS || hit_type == FightSystem::type_maul)
			&& !IS_IMMORTAL(victim)
			&& (!can_use_feat(victim, PARRY_ARROW_FEAT)
				|| number(1, 1000) >= 20 * MIN(GET_REAL_DEX(victim), 35)))
		{
			prob = 0;
		}
		else
		{
			BATTLECNTR(victim)++;
		}

		if (prob < 50)
		{
			act("Вы не смогли отбить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N не сумел$G отбить вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n не сумел$g отбить атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else if (prob < 90)
		{
			act("Вы немного отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N немного отклонил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n немного отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 10);
			*dam = *dam * 10 / 15;
		}
		else if (prob < 180)
		{
			act("Вы частично отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N частично отклонил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n частично отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 15);
			*dam = *dam / 2;
		}
		else
		{
			act("Вы полностью отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N полностью отклонил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n полностью отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 25);
			*dam = -1;
		}
	}
}

void hit_block(CHAR_DATA *ch, CHAR_DATA *victim, int *dam)
{
	if (!(GET_EQ(victim, WEAR_SHIELD)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim)))
	{
		send_to_char("У вас нечем отразить атаку противника\r\n", victim);
	}
	else
	{
		int range = number(1, skill_info[SKILL_BLOCK].max_percent);
		int prob =
			train_skill(victim, SKILL_BLOCK, skill_info[SKILL_BLOCK].max_percent, ch);
		prob = prob * 100 / range;
		BATTLECNTR(victim)++;
		if (prob < 100)
		{
			act("Вы не смогли отразить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N не сумел$G отразить вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n не сумел$g отразить атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else if (prob < 150)
		{
			act("Вы немного отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N немного отразил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n немного отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, WEAR_SHIELD, *dam, 10);
			*dam = *dam * 10 / 15;
		}
		else if (prob < 250)
		{
			act("Вы частично отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N частично отразил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n частично отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, WEAR_SHIELD, *dam, 15);
			*dam = *dam / 2;
		}
		else
		{
			act("Вы полностью отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N полностью отразил$G вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
			act("$n полностью отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, WEAR_SHIELD, *dam, 25);
			*dam = -1;
		}
	}
}

void appear(CHAR_DATA * ch)
{
	const bool appear_msg = AFF_FLAGGED(ch, EAffectFlag::AFF_INVISIBLE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE);

	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);
	if (affected_by_spell(ch, SPELL_HIDE))
		affect_from_char(ch, SPELL_HIDE);
	if (affected_by_spell(ch, SPELL_SNEAK))
		affect_from_char(ch, SPELL_SNEAK);
	if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
		affect_from_char(ch, SPELL_CAMOUFLAGE);

	AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);

	if (appear_msg)
	{
		if (IS_NPC(ch)
			|| GET_LEVEL(ch) < LVL_IMMORT)
		{
			act("$n медленно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			act("Вы почувствовали странное присутствие $n1.", FALSE, ch, 0, 0, TO_ROOM);
		}
	}
}

// message for doing damage with a weapon
void Damage::dam_message(CHAR_DATA* ch, CHAR_DATA* victim) const
{
	int dam_msgnum;
	int w_type = msg_num;

	static const struct dam_weapon_type
	{
		const char *to_room;
		const char *to_char;
		const char *to_victim;
	} dam_weapons[] =
	{

		// use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes")

		{
			"$n попытал$u #W $N3, но промахнул$u.",	// 0: 0      0 //
			"Вы попытались #W $N3, но промахнулись.",
			"$n попытал$u #W вас, но промахнул$u."
		}, {
			"$n легонько #w$g $N3.",	//  1..5 1 //
			"Вы легонько #wи $N3.",
			"$n легонько #w$g вас."
		}, {
			"$n слегка #w$g $N3.",	//  6..11  2 //
			"Вы слегка #wи $N3.",
			"$n слегка #w$g вас."
		}, {
			"$n #w$g $N3.",	//  12..18   3 //
			"Вы #wи $N3.",
			"$n #w$g вас."
		}, {
			"$n #w$g $N3.",	// 19..26  4 //
			"Вы #wи $N3.",
			"$n #w$g вас."
		}, {
			"$n сильно #w$g $N3.",	// 27..35  5 //
			"Вы сильно #wи $N3.",
			"$n сильно #w$g вас."
		}, {
			"$n очень сильно #w$g $N3.",	//  36..45 6  //
			"Вы очень сильно #wи $N3.",
			"$n очень сильно #w$g вас."
		}, {
			"$n чрезвычайно сильно #w$g $N3.",	//  46..55  7 //
			"Вы чрезвычайно сильно #wи $N3.",
			"$n чрезвычайно сильно #w$g вас."
		}, {
			"$n БОЛЬНО #w$g $N3.",	//  56..96   8 //
			"Вы БОЛЬНО #wи $N3.",
			"$n БОЛЬНО #w$g вас."
		}, {
			"$n ОЧЕНЬ БОЛЬНО #w$g $N3.",	//    97..136  9  //
			"Вы ОЧЕНЬ БОЛЬНО #wи $N3.",
			"$n ОЧЕНЬ БОЛЬНО #w$g вас."
		}, {
			"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g $N3.",	//   137..176  10 //
			"Вы ЧРЕЗВЫЧАЙНО БОЛЬНО #wи $N3.",
			"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g вас."
		}, {
			"$n НЕВЫНОСИМО БОЛЬНО #w$g $N3.",	//    177..216  11 //
			"Вы НЕВЫНОСИМО БОЛЬНО #wи $N3.",
			"$n НЕВЫНОСИМО БОЛЬНО #w$g вас."
		}, {
			"$n ЖЕСТОКО #w$g $N3.",	//    217..256  13 //
			"Вы ЖЕСТОКО #wи $N3.",
			"$n ЖЕСТОКО #w$g вас."
		}, {
			"$n УЖАСНО #w$g $N3.",	//    257..296  13 //
			"Вы УЖАСНО #wи $N3.",
			"$n УЖАСНО #w$g вас."
		}, {
			"$n УБИЙСТВЕННО #w$g $N3.",	//    297..400  15 //
			"Вы УБИЙСТВЕННО #wи $N3.",
			"$n УБИЙСТВЕННО #w$g вас."
		}, {
			"$n СМЕРТЕЛЬНО #w$g $N3.",	// 400+  16 //
			"Вы СМЕРТЕЛЬНО #wи $N3.",
			"$n СМЕРТЕЛЬНО #w$g вас."
		}
	};

	if (w_type >= TYPE_HIT && w_type < TYPE_MAGIC)
		w_type -= TYPE_HIT;	// Change to base of table with text //
	else
		w_type = TYPE_HIT;

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
	else
		dam_msgnum = 15;
	// damage message to onlookers
	char *buf_ptr = replace_string(dam_weapons[dam_msgnum].to_room,
		attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	if (brief_shields_.empty())
	{
		act(buf_ptr, FALSE, ch, NULL, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}
	else
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s", buf_ptr, brief_shields_.c_str());
		act(buf_, FALSE, ch, NULL, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_BRIEF_SHIELDS);
		act(buf_ptr, FALSE, ch, NULL, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
	}

	// damage message to damager
	send_to_char(ch, "%s", dam ? "&Y&q" : "&y&q");
	if (!brief_shields_.empty() && PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s",
			replace_string(dam_weapons[dam_msgnum].to_char,
				attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
			brief_shields_.c_str());
		act(buf_, FALSE, ch, NULL, victim, TO_CHAR);
	}
	else
	{
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_char,
			attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, FALSE, ch, NULL, victim, TO_CHAR);
	}
	send_to_char("&Q&n", ch);

	// damage message to damagee
	send_to_char("&R&q", victim);
	if (!brief_shields_.empty() && PRF_FLAGGED(victim, PRF_BRIEF_SHIELDS))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s",
			replace_string(dam_weapons[dam_msgnum].to_victim,
				attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
			brief_shields_.c_str());
		act(buf_, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	}
	else
	{
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_victim,
			attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	}
	send_to_char("&Q&n", victim);
}

// запоминание.
// чара-обидчика моб помнит всегда, если он его бьет непосредственно.
// если бьют клоны (проверка на MOB_CLONE), тоже помнит всегда.
// если бьют храны или чармис (все остальные под чармом), то только если
// моб может видеть их хозяина.
void update_mob_memory(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// первое -- бьют моба, он запоминает обидчика
	if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY))
	{
		if (!IS_NPC(ch))
		{
			remember(victim, ch);
		}
		else if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& ch->has_master()
			&& !IS_NPC(ch->get_master()))
		{
			if (MOB_FLAGGED(ch, MOB_CLONE))
			{
				remember(victim, ch->get_master());
			}
			else if (IN_ROOM(ch->get_master()) == IN_ROOM(victim)
				&& CAN_SEE(victim, ch->get_master()))
			{
				remember(victim, ch->get_master());
			}
		}
	}

	// второе -- бьет сам моб и запоминает, кого потом добивать :)
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY))
	{
		if (!IS_NPC(victim))
		{
			remember(ch, victim);
		}
		else if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
			&& victim->has_master()
			&& !IS_NPC(victim->get_master()))
		{
			if (MOB_FLAGGED(victim, MOB_CLONE))
			{
				remember(ch, victim->get_master());
			}
			else if (IN_ROOM(victim->get_master()) == ch->in_room
				&& CAN_SEE(ch, victim->get_master()))
			{
				remember(ch, victim->get_master());
			}
		}
	}
}

bool Damage::magic_shields_dam(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// защита богов
	if (dam && AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD))
	{
		if (skill_num == SKILL_BASH)
		{
			skill_message(dam, ch, victim, msg_num);
		}
		act("Магический кокон полностью поглотил удар $N1.",
			FALSE, victim, 0, ch, TO_CHAR);
		act("Магический кокон вокруг $N1 полностью поглотил ваш удар.",
			FALSE, ch, 0, victim, TO_CHAR);
		act("Магический кокон вокруг $N1 полностью поглотил удар $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
		return true;
	}

	if (dam <= 0)
	{
		return false;
	}

	// отражение части маг дамага от зеркала
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICGLASS)
		&& dmg_type == FightSystem::MAGE_DMG)
	{
		int pct = 6;
		if (IS_NPC(victim) && !IS_CHARMICE(victim))
		{
			pct += 2;
			if (victim->get_role(MOB_ROLE_BOSS))
			{
				pct += 2;
			}
		}
		// дамаг обратки
		const int mg_damage = dam * pct / 100;
		if (mg_damage > 0
			&& victim->get_fighting()
			&& GET_POS(victim) > POS_STUNNED
			&& IN_ROOM(victim) != NOWHERE)
		{
			flags.set(FightSystem::DRAW_BRIEF_MAG_MIRROR);
			Damage dmg(SpellDmg(SPELL_MAGICGLASS), mg_damage, FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE);
			dmg.flags.set(FightSystem::MAGIC_REFLECT);
			dmg.process(victim, ch);
		}
	}

	// обработка щитов, см Damage::post_init_shields()
	if (flags[FightSystem::VICTIM_FIRE_SHIELD]
		&& !flags[FightSystem::CRIT_HIT])
	{
		if (dmg_type == FightSystem::PHYS_DMG
			&& !flags[FightSystem::IGNORE_FSHIELD])
		{
			int pct = 15;
			if (IS_NPC(victim) && !IS_CHARMICE(victim))
			{
				pct += 5;
				if (victim->get_role(MOB_ROLE_BOSS))
				{
					pct += 5;
				}
			}
			fs_damage = dam * pct / 100;
		}
		else
		{
			act("Огненный щит вокруг $N1 ослабил вашу атаку.",
				FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
			act("Огненный щит принял часть повреждений на себя.",
				FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
			act("Огненный щит вокруг $N1 ослабил атаку $n1.",
				TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		}
		flags.set(FightSystem::DRAW_BRIEF_FIRE_SHIELD);
		dam -= (dam * number(30, 50) / 100);
	}

        // если критический удар (не точка и стаб) и есть щит - 95% шанс в молоко
        // критическим считается любой удар который вложиля в определенные границы
	if (dam
		&& flags[FightSystem::CRIT_HIT] && flags[FightSystem::VICTIM_ICE_SHIELD]
		&& !dam_critic
		&& spell_num != SPELL_POISON 
                && number(0, 100)<94)
	{
            act("Ваше меткое попадания частично утонуло в ледяной пелене вокруг $N1.",
			FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
            act("Меткое попадание частично утонуло в ледяной пелене щита.",
			FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
            act("Ледяной щит вокруг $N1 частично поглотил меткое попадание $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
			
                   flags[FightSystem::CRIT_HIT] = false; //вот это место очень тщательно проверить
		   if (dam > 0) dam -= (dam * number(30, 50) / 100);
        }
    //шоб небуло спама модернизировал условие
	else if (dam > 0
		&& flags[FightSystem::VICTIM_ICE_SHIELD]
		&& !flags[FightSystem::CRIT_HIT])
	{
		flags.set(FightSystem::DRAW_BRIEF_ICE_SHIELD);
		act("Ледяной щит вокруг $N1 смягчил ваш удар.",
			FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
		act("Ледяной щит принял часть удара на себя.",
			FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
		act("Ледяной щит вокруг $N1 смягчил удар $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		dam -= (dam * number(30, 50) / 100);
	}
	
	if (dam > 0
		&& flags[FightSystem::VICTIM_AIR_SHIELD]
		&& !flags[FightSystem::CRIT_HIT])
	{
		flags.set(FightSystem::DRAW_BRIEF_AIR_SHIELD);
		act("Воздушный щит вокруг $N1 ослабил ваш удар.",
			FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
		act("Воздушный щит смягчил удар $n1.",
			FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
		act("Воздушный щит вокруг $N1 ослабил удар $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		dam -= (dam * number(30, 50) / 100);
	}

	return false;
}

void Damage::armor_dam_reduce(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// броня на физ дамаг
	if (dam > 0
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		alt_equip(victim, NOWHERE, dam, 50);
		if (!flags[FightSystem::CRIT_HIT]
			&& !flags[FightSystem::IGNORE_ARMOR])
		{
			// 50 брони = 50% снижение дамага
			int max_armour = 50;
			if (can_use_feat(victim, IMPREGNABLE_FEAT)
				&& PRF_FLAGS(victim).get(PRF_AWAKE))
			{
				// непробиваемый в осторожке - до 75 брони
				max_armour = 75;
			}
			int tmp_dam = dam * MAX(0, MIN(max_armour, GET_ARMOUR(victim))) / 100;
			// ополовинивание брони по флагу скила
			if (tmp_dam >= 2
				&& flags[FightSystem::HALF_IGNORE_ARMOR])
			{
				tmp_dam /= 2;
			}
			dam -= tmp_dam;
			// крит удар умножает дамаг, если жертва без призмы и без лед.щита
		}
		else if (flags[FightSystem::CRIT_HIT]
			&& (GET_LEVEL(victim) >= 5
				|| !IS_NPC(ch))
			&& !AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA)
			&& !flags[FightSystem::VICTIM_ICE_SHIELD])
		{
			dam = MAX(dam, MIN(GET_REAL_MAX_HIT(victim) / 8, dam * 2));
		}
	}
}

/**
 * Обработка поглощения физ и маг урона.
 * \return true - полное поглощение
 */
bool Damage::dam_absorb(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (dmg_type == FightSystem::PHYS_DMG
		&& skill_num < 0
		&& spell_num < 0
		&& dam > 0
		&& GET_ABSORBE(victim) > 0)
	{
		// шансы поглощения: непробиваемый в осторожке 15%, остальные 10%
		int chance = 10;
		if (can_use_feat(victim, IMPREGNABLE_FEAT)
			&& PRF_FLAGS(victim).get(PRF_AWAKE))
		{
			chance = 15;
		}
		// физ урон - прямое вычитание из дамага
		if (number(1, 100) <= chance)
		{
			dam -= GET_ABSORBE(victim) / 2;
			if (dam <= 0)
			{
				act("Ваши доспехи полностью поглотили удар $n1.",
					FALSE, ch, 0, victim, TO_VICT);
				act("Доспехи $N1 полностью поглотили ваш удар.",
					FALSE, ch, 0, victim, TO_CHAR);
				act("Доспехи $N1 полностью поглотили удар $n1.",
					TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				return true;
			}
		}
	}
	else if (dmg_type == FightSystem::MAGE_DMG
		&& dam > 0
		&& GET_ABSORBE(victim) > 0
		&& !flags[FightSystem::IGNORE_ABSORBE])
	{
		// маг урон - по 1% за каждые 2 абсорба, максимум 25% (цифры из mag_damage)
		int absorb = MIN(GET_ABSORBE(victim) / 2, 25);
		dam -= dam * absorb / 100;
	}
	return false;
}

void Damage::send_critical_message(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// Блочить мессагу крита при ледяном щите вроде нелогично,
	// так что добавил отдельные сообщения для ледяного щита (Купала)
	if (!flags[FightSystem::VICTIM_ICE_SHIELD])
	{
		sprintf(buf, "&B&qВаше меткое попадание тяжело ранило %s.&Q&n\r\n",
			PERS(victim, ch, 3));
	}
	else
	{
		sprintf(buf, "&B&qВаше меткое попадание утонуло в ледяной пелене щита %s.&Q&n\r\n",
			PERS(victim, ch, 1));
	}

	send_to_char(buf, ch);

	if (!flags[FightSystem::VICTIM_ICE_SHIELD])
	{
		sprintf(buf, "&r&qМеткое попадание %s тяжело ранило вас.&Q&n\r\n",
			PERS(ch, victim, 1));
	}
	else
	{
		sprintf(buf, "&r&qМеткое попадание %s утонуло в ледяной пелене вашего щита.&Q&n\r\n",
			PERS(ch, victim, 1));
	}

	send_to_char(buf, victim);
	// Закомментил чтобы не спамило, сделать потом в виде режима
	//act("Меткое попадание $N1 заставило $n3 пошатнуться.", TRUE, victim, 0, ch, TO_NOTVICT);
}

void update_dps_stats(CHAR_DATA *ch, int real_dam, int over_dam)
{
	if (!IS_NPC(ch))
	{
		ch->dps_add_dmg(DpsSystem::PERS_DPS, real_dam, over_dam);
		log("DmetrLog. Name(player): %s, class: %d, remort:%d, level:%d, dmg: %d, over_dmg:%d", GET_NAME(ch), GET_CLASS(ch), GET_REMORT(ch), GET_LEVEL(ch), real_dam, over_dam);
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
		{
			CHAR_DATA *leader = ch->has_master() ? ch->get_master() : ch;
			leader->dps_add_dmg(DpsSystem::GROUP_DPS, real_dam, over_dam, ch);
		}
	}
	else if (IS_CHARMICE(ch)
		&& ch->has_master())
	{
		ch->get_master()->dps_add_dmg(DpsSystem::PERS_CHARM_DPS, real_dam, over_dam, ch);
		if (!IS_NPC(ch->get_master()))
		{
			log("DmetrLog. Name(charmice): %s, name(master): %s, class: %d, remort: %d, level: %d, dmg: %d, over_dmg:%d",
				GET_NAME(ch), GET_NAME(ch->get_master()), GET_CLASS(ch->get_master()), GET_REMORT(ch->get_master()),
				GET_LEVEL(ch->get_master()), real_dam, over_dam);
		}

		if (AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_GROUP))
		{
			CHAR_DATA *leader = ch->get_master()->has_master() ? ch->get_master()->get_master() : ch->get_master();
			leader->dps_add_dmg(DpsSystem::GROUP_CHARM_DPS, real_dam, over_dam, ch);
		}
	}
}

void try_angel_sacrifice(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// если виктим в группе с кем-то с ангелом - вместо смерти виктима умирает ангел
	if (GET_HIT(victim) <= 0
		&& !IS_NPC(victim)
		&& AFF_FLAGGED(victim, EAffectFlag::AFF_GROUP))
	{
		const auto people = world[IN_ROOM(victim)]->people;	// make copy of people because keeper might be removed from this list inside the loop
		for (const auto keeper : people)
		{
			if (IS_NPC(keeper)
				&& MOB_FLAGGED(keeper, MOB_ANGEL)
				&& keeper->has_master()
				&& AFF_FLAGGED(keeper->get_master(), EAffectFlag::AFF_GROUP))
			{
				CHAR_DATA *keeper_leader = keeper->get_master()->has_master()
					? keeper->get_master()->get_master()
					: keeper->get_master();
				CHAR_DATA *victim_leader = victim->has_master()
					? victim->get_master()
					: victim;

				if ((keeper_leader == victim_leader)
					&& (may_kill_here(keeper->get_master(), ch)))
				{
					if (!pk_agro_action(keeper->get_master(), ch))
					{
						return;
					}
					send_to_char(victim, "%s пожертвовал%s своей жизнью, вытаскивая вас с того света!\r\n",
						GET_PAD(keeper, 0), GET_CH_SUF_1(keeper));
					snprintf(buf, MAX_STRING_LENGTH, "%s пожертвовал%s своей жизнью, вытаскивая %s с того света!",
						GET_PAD(keeper, 0), GET_CH_SUF_1(keeper), GET_PAD(victim, 3));
					act(buf, FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

					extract_char(keeper, 0);
					GET_HIT(victim) = MIN(300, GET_MAX_HIT(victim) / 2);
				}
			}
		}
	}
}

void update_pk_logs(CHAR_DATA *ch, CHAR_DATA *victim)
{
	ClanPkLog::check(ch, victim);
	sprintf(buf2, "%s killed by %s at %s [%d] ", GET_NAME(victim), GET_NAME(ch),
		IN_ROOM(victim) != NOWHERE ? world[IN_ROOM(victim)]->name : "NOWHERE", GET_ROOM_VNUM(IN_ROOM(victim)));
	log("%s",buf2);

	if ((!IS_NPC(ch)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master())))
		&& RENTABLE(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	{
		mudlog(buf2, BRF, LVL_IMPL, SYSLOG, 0);
		if (IS_NPC(ch)
			&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || IS_HORSE(ch))
			&& ch->has_master()
			&& !IS_NPC(ch->get_master()))
		{
			sprintf(buf2, "%s is following %s.", GET_NAME(ch), GET_PAD(ch->get_master(), 2));
			mudlog(buf2, BRF, LVL_IMPL, SYSLOG, TRUE);
		}
	}
}

void Damage::process_death(CHAR_DATA *ch, CHAR_DATA *victim)
{
	CHAR_DATA *killer = NULL;

	if (IS_NPC(victim) || victim->desc)
	{
		if (victim == ch && IN_ROOM(victim) != NOWHERE)
		{
			if (spell_num == SPELL_POISON)
			{
				for (const auto poisoner : world[IN_ROOM(victim)]->people)
				{
					if (poisoner != victim
						&& GET_ID(poisoner) == victim->Poisoner)
					{
						killer = poisoner;
					}
				}
			}
			else if (msg_num == TYPE_SUFFERING)
			{
				for (const auto attacker : world[IN_ROOM(victim)]->people)
				{
					if (attacker->get_fighting() == victim)
					{
						killer = attacker;
					}
				}
			}
		}

		if (ch != victim)
		{
			killer = ch;
		}
	}

	if (killer)
	{
		if (AFF_FLAGGED(killer, EAffectFlag::AFF_GROUP))
		{
			// т.к. помечен флагом AFF_GROUP - точно PC
			group_gain(killer, victim);
		}
		else if ((AFF_FLAGGED(killer, EAffectFlag::AFF_CHARM)
				|| MOB_FLAGGED(killer, MOB_ANGEL)
				|| MOB_FLAGGED(killer, MOB_GHOST))
			&& killer->has_master())
			// killer - зачармленный NPC с хозяином
		{
			// по логике надо бы сделать, что если хозяина нет в клетке, но
			// кто-то из группы хозяина в клетке, то опыт накинуть согруппам,
			// которые рядом с убившим моба чармисом.
			if (AFF_FLAGGED(killer->get_master(), EAffectFlag::AFF_GROUP)
				&& IN_ROOM(killer) == IN_ROOM(killer->get_master()))
			{
				// Хозяин - PC в группе => опыт группе
				group_gain(killer->get_master(), victim);
			}
			else if (IN_ROOM(killer) == IN_ROOM(killer->get_master()))
				// Чармис и хозяин в одной комнате
				// Опыт хозяину
			{
				perform_group_gain(killer->get_master(), victim, 1, 100);
			}
			// else
			// А хозяина то рядом не оказалось, все чармису - убрано
			// нефиг абьюзить чарм  perform_group_gain( killer, victim, 1, 100 );
		}
		else
		{
			// Просто NPC или PC сам по себе
			perform_group_gain(killer, victim, 1, 100);
		}
	}

	// в сислог иммам идут только смерти в пк (без арен)
	// в файл пишутся все смерти чаров
	// если чар убит палачем то тоже не спамим

	if (!IS_NPC(victim) && !(killer && PRF_FLAGGED(killer, PRF_EXECUTOR)))
	{
		update_pk_logs(ch, victim);

	for (const auto& ch_vict : world[ch->in_room]->people)
	{
		//Мобы все кто присутствовал при смерти игрока забывают
		if (IS_IMMORTAL(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (!IS_NPC(ch_vict))
			continue;
		if (MOB_FLAGGED(ch_vict, MOB_MEMORY))
		{
			forget(ch_vict, victim);
		}
	}
                
	}

	if (killer)
	{
		ch = killer;
	}

	die(victim, ch);
}

// обработка щитов, зб, поглощения, сообщения для огн. щита НЕ ЗДЕСЬ
// возвращает сделанный дамаг
int Damage::process(CHAR_DATA *ch, CHAR_DATA *victim)
{
	post_init(ch, victim);

	if (!check_valid_chars(ch, victim, __FILE__, __LINE__))
	{
		return 0;
	}

	if (IN_ROOM(victim) == NOWHERE
		|| ch->in_room == NOWHERE
		|| ch->in_room != IN_ROOM(victim))
	{
		log("SYSERR: Attempt to damage '%s' in room NOWHERE by '%s'.",
			GET_NAME(victim), GET_NAME(ch));
		return 0;
	}

	if (GET_POS(victim) <= POS_DEAD)
	{
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
		die(victim, NULL);
		return 0;	// -je, 7/7/92
	}

	// первая такая проверка идет в hit перед ломанием пушек
	if (dam >= 0 && damage_mtrigger(ch, victim))
		return 0;

	// No fight mobiles
	if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
		|| (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT)))
	{
		return 0;
	}

	if (dam > 0)
	{
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

	if (victim != ch)  	// Start the attacker fighting the victim
	{
		if (GET_POS(ch) > POS_STUNNED && (ch->get_fighting() == NULL))
		{
			if (!pk_agro_action(ch, victim))
				return (0);
			set_fighting(ch, victim);
			npc_groupbattle(ch);
		}
		// Start the victim fighting the attacker
		if (GET_POS(victim) > POS_STUNNED && (victim->get_fighting() == NULL))
		{
			set_fighting(victim, ch);
			npc_groupbattle(victim);
		}

		//check self horse attack
		if (on_horse(ch) && get_horse(ch) == victim)
		{
			horse_drop(victim);
		}
		else if (on_horse(victim) && get_horse(victim) == ch)
		{
			horse_drop(ch);
		}
	}

	// If negative damage - return
	if (dam < 0
		|| ch->in_room == NOWHERE
		|| IN_ROOM(victim) == NOWHERE
		|| ch->in_room != IN_ROOM(victim))
	{
		return (0);
	}

	// санка/призма для физ и маг урона
	if (dam >= 2)
	{
		if (AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA) &&  !(skill_num == SKILL_BACKSTAB && can_use_feat(ch, THIEVES_STRIKE_FEAT)))
		{
			if (dmg_type == FightSystem::PHYS_DMG)
			{
				dam *= 2;
			}
			else if (dmg_type == FightSystem::MAGE_DMG)
			{
				dam /= 2;
			}
		}
		if (AFF_FLAGGED(victim, EAffectFlag::AFF_SANCTUARY) &&  !(skill_num == SKILL_BACKSTAB && can_use_feat(ch, THIEVES_STRIKE_FEAT)))
		{
			if (dmg_type == FightSystem::PHYS_DMG)
			{
				dam /= 2;
			}
			else if (dmg_type == FightSystem::MAGE_DMG)
			{
				dam *= 2;
			}
		}

		if (IS_NPC(victim) && Bonus::is_bonus(Bonus::BONUS_DAMAGE))
		{
			dam *= Bonus::get_mult_bonus();
		}
	}

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
	if (ch_start_pos < POS_FIGHTING
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		dam -= dam * (POS_FIGHTING - ch_start_pos) / 4;
	}

	// дамаг не увеличивается если:
	// на жертве есть воздушный щит
	// атака - каст моба (в mage_damage увеличение дамага от позиции было только у колдунов)
	if (victim_start_pos < POS_FIGHTING
		&& !flags[FightSystem::VICTIM_AIR_SHIELD]
		&& !(dmg_type == FightSystem::MAGE_DMG
			&& IS_NPC(ch)))
	{
		dam += dam * (POS_FIGHTING - victim_start_pos) / 4;
	}

	// прочие множители

	// изменение физ урона по холду
	if (GET_MOB_HOLD(victim)
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		if (IS_NPC(ch)
			&& !IS_CHARMICE(ch))
		{
			dam = dam * 15 / 10;
		}
		else
		{
			dam = dam * 125 / 100;
		}
	}

	// тюнинг дамага чармисов по чарам
	if (!IS_NPC(victim)
		&& IS_CHARMICE(ch))
	{
		dam = dam * 8 / 10;
	}

	// яд белены для физ урона
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BELENA_POISON)
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		dam -= dam * GET_POISON(ch) / 100;
	}

	// added by WorM(Видолюб) поглощение физ.урона в %
	//if(GET_PR(victim) && IS_NPC(victim)
	if (GET_PR(victim) && dmg_type == FightSystem::PHYS_DMG)
	{
		if (PRF_FLAGGED(victim, PRF_TESTER))
		{
			int ResultDam = dam - (dam * GET_PR(victim) / 100);
			sprintf(buf, "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
			send_to_char(buf, victim);
			dam = ResultDam;
		}
		else
		{
			dam = dam - (dam * GET_PR(victim) / 100);
		}
	}

	// зб, щиты, броня, поглощение
	if (victim != ch)
	{
		bool shield_full_absorb = magic_shields_dam(ch, victim);
		// сначала броня
		armor_dam_reduce(ch, victim);
		// потом абсорб
		bool armor_full_absorb = dam_absorb(ch, victim);
		// полное поглощение
		if (shield_full_absorb
			|| armor_full_absorb)
		{
			return 0;
		}
	}

	// Внутри magic_shields_dam вызывается dmg::proccess, если чар там умрет, то будет креш
	if (!(ch && victim) || (ch->purged() || victim->purged()))
		return 0;

	// зб на мобе
	if (MOB_FLAGGED(victim, MOB_PROTECT))
	{
		if (victim != ch)
		{
			act("$n находится под защитой Богов.", FALSE, victim, 0, 0, TO_ROOM);
		}
		return 0;
	}

	// яд скополии
	if (skill_num != SKILL_BACKSTAB
		&& AFF_FLAGGED(victim, EAffectFlag::AFF_SCOPOLIA_POISON))
	{
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
	if (dmg_type == FightSystem::PHYS_DMG)
	{
		dam += ch->obj_bonus().calc_phys_dmg(dam);
	}
	else if (dmg_type == FightSystem::MAGE_DMG)
	{
		dam += ch->obj_bonus().calc_mage_dmg(dam);
	}

	// обратка от зеркал/огненного щита
	if (flags[FightSystem::MAGIC_REFLECT])
	{
		// ограничение для зеркал на 40% от макс хп кастера
		dam = MIN(dam, GET_MAX_HIT(victim) * 4 / 10);
		// чтобы не убивало обраткой
		dam = MIN(dam, GET_HIT(victim) - 1);
	}

	dam = MAX(0, MIN(dam, MAX_HITS));

	// расчет бэтл-экспы для чаров
	gain_battle_exp(ch, victim, dam);

	// real_dam так же идет в обратку от огн.щита
	int real_dam = dam;
	int over_dam = 0;

	// собственно нанесение дамага
	if (dam > GET_HIT(victim) + 11)
	{
		real_dam = GET_HIT(victim) + 11;
		over_dam = dam - real_dam;
	}
	GET_HIT(victim) -= dam;
	// если на чармисе вампир
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_VAMPIRE))
	{
		GET_HIT(ch) = MAX(GET_HIT(ch), MIN(GET_HIT(ch) + MAX(1, dam * 0.1), GET_REAL_MAX_HIT(ch)
			+ GET_REAL_MAX_HIT(ch) * GET_LEVEL(ch) / 10));
		// если есть родство душ, то чару отходит по 5% от дамаги к хп
		if (ch->has_master())
		{
			if (can_use_feat(ch->get_master(), SOULLINK_FEAT))
			{
				GET_HIT(ch->get_master()) = MAX(GET_HIT(ch->get_master()),
					MIN(GET_HIT(ch->get_master()) + MAX(1, dam * 0.05),
						GET_REAL_MAX_HIT(ch->get_master())
						+ GET_REAL_MAX_HIT(ch->get_master()) * GET_LEVEL(ch->get_master()) / 10));
			}
		}
	}

	// запись в дметр фактического и овер дамага
	update_dps_stats(ch, real_dam, over_dam);
	// запись дамага в список атакеров
	if (IS_NPC(victim))
	{
		victim->add_attacker(ch, ATTACKER_DAMAGE, real_dam);
	}

	// попытка спасти жертву через ангела
	try_angel_sacrifice(ch, victim);

	// обновление позиции после удара и ангела
	update_pos(victim);
	// если вдруг виктим сдох после этого, то произойдет креш, поэтому вставил тут проверочку
	if (!(ch && victim) || (ch->purged() || victim->purged()))
	{
		log("Error in fight_hit, function process()\r\n");
		return 0;
	}
	// если у чара есть жатва жизни
	if (can_use_feat(victim, HARVESTLIFE_FEAT))
	{
		if (GET_POS(victim) == POS_DEAD)
		{
			int souls = victim->get_souls();
			if (souls >= 10)
			{
				GET_HIT(victim) = 0 + souls * 10;
				update_pos(victim);
				send_to_char("&GДуши спасли вас от смерти!&n\r\n", victim);
				victim->set_souls(0);
			}
		}
	}
	// сбивание надува черноков //
	if (spell_num != SPELL_POISON
		&& dam > 0
		&& !flags[FightSystem::MAGIC_REFLECT])
	{
		try_remove_extrahits(ch, victim);
	}

	// сообщения о крит ударах //
	if (dam
		&& flags[FightSystem::CRIT_HIT]
		&& !dam_critic
		&& spell_num != SPELL_POISON)
	{
		send_critical_message(ch, victim);
	}

	//  skill_message sends a message from the messages file in lib/misc.
	//  dam_message just sends a generic "You hit $n extremely hard.".
	//  skill_message is preferable to dam_message because it is more
	//  descriptive.
	//
	//  If we are _not_ attacking with a weapon (i.e. a spell), always use
	//  skill_message. If we are attacking with a weapon: If this is a miss or a
	//  death blow, send a skill_message if one exists; if not, default to a
	//  dam_message. Otherwise, always send a dam_message.
	// log("[DAMAGE] Attack message...");

	// инит Damage::brief_shields_
	if (flags.test(FightSystem::DRAW_BRIEF_FIRE_SHIELD)
		|| flags.test(FightSystem::DRAW_BRIEF_AIR_SHIELD)
		|| flags.test(FightSystem::DRAW_BRIEF_ICE_SHIELD)
		|| flags.test(FightSystem::DRAW_BRIEF_MAG_MIRROR))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "&n (%s%s%s%s)",
			flags.test(FightSystem::DRAW_BRIEF_FIRE_SHIELD) ? "&R*&n" : "",
			flags.test(FightSystem::DRAW_BRIEF_AIR_SHIELD) ? "&C*&n" : "",
			flags.test(FightSystem::DRAW_BRIEF_ICE_SHIELD) ? "&W*&n" : "",
			flags.test(FightSystem::DRAW_BRIEF_MAG_MIRROR) ? "&M*&n" : "");
		brief_shields_ = buf_;
	}

	// сообщения об ударах //
	if (skill_num >= 0 || spell_num >= 0 || hit_type < 0)
	{
		// скилл, спелл, необычный дамаг
		skill_message(dam, ch, victim, msg_num, brief_shields_);
	}
	else
	{
		// простой удар рукой/оружием
		if (GET_POS(victim) == POS_DEAD || dam == 0)
		{
			if (!skill_message(dam, ch, victim, msg_num,  brief_shields_))
			{
				dam_message(ch, victim);
			}
		}
		else
		{
			dam_message(ch, victim);
		}
	}

	/// Use send_to_char -- act() doesn't send message if you are DEAD.
	char_dam_message(dam, ch, victim, flags[FightSystem::NO_FLEE]);

	if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
	{
		sprintf(buf, "&MПрименен урон = %d&n\r\n", dam);
		send_to_char(buf,ch);
	}

	// Проверить, что жертва все еще тут. Может уже сбежала по трусости.
	// Думаю, простой проверки достаточно.
	// Примечание, если сбежал в FIRESHIELD,
	// то обратного повреждения по атакующему не будет
	if (ch->in_room != IN_ROOM(victim))
	{
		return dam;
	}

	// Stop someone from fighting if they're stunned or worse
	if ((GET_POS(victim) <= POS_STUNNED)
		&& (victim->get_fighting() != NULL))
	{
		stop_fighting(victim, GET_POS(victim) <= POS_DEAD);
	}

	// жертва умирает //
	if (GET_POS(victim) == POS_DEAD)
	{
		process_death(ch, victim);
		return -1;
	}

	// обратка от огненного щита
	if (fs_damage > 0
		&& victim->get_fighting()
		&& GET_POS(victim) > POS_STUNNED
		&& IN_ROOM(victim) != NOWHERE)
	{
		Damage dmg(SpellDmg(SPELL_FIRE_SHIELD), fs_damage, FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);
		dmg.flags.set(FightSystem::MAGIC_REFLECT);
		dmg.process(victim, ch);
	}

	return dam;
}

void HitData::try_mighthit_dam(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int percent = number(1, skill_info[SKILL_MIGHTHIT].max_percent);
	int prob = train_skill(ch, SKILL_MIGHTHIT, skill_info[SKILL_MIGHTHIT].max_percent, victim);
	int lag = 0, might = 0;

	if (GET_MOB_HOLD(victim))
	{
		percent = number(1, 25);
	}

	if (IS_IMMORTAL(victim))
	{
		prob = 0;
	}

/*  Логирование шанса молота.
send_to_char(ch, "Вычисление молота: Prob == %d, Percent == %d, Might == %d, Stab == %d\r\n", prob, percent, might, stab);
 sprintf(buf, "%s молотит : Percent == %d,Prob == %d, Might == %d, Stability == %d\r\n",GET_NAME(ch), percent, prob, might, stab);
                mudlog(buf, LGH, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
*/
	if (percent > prob || dam == 0)
	{
		sprintf(buf, "&c&qВаш богатырский удар пропал впустую.&Q&n\r\n");
		send_to_char(buf, ch);
		lag = 3;
		dam = 0;
	}
	else if (MOB_FLAGGED(victim, MOB_NOHAMER))
	{
		sprintf(buf, "&c&qНа других надо силу проверять!&Q&n\r\n");
		send_to_char(buf, ch);
		lag = 1;
		dam = 0;
	}
	else
	{
		might = prob * 100 / percent;
		if (might < 180)
		{
			sprintf(buf, "&b&qВаш богатырский удар задел %s.&Q&n\r\n",
				PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 1;
			WAIT_STATE(victim, PULSE_VIOLENCE);
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_BATTLE;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(victim, af, TRUE, FALSE, TRUE, FALSE);
			sprintf(buf, "&R&qВаше сознание затуманилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N содрогнул$U от богатырского удара $n1.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
			if (!number(0, 2))
			{
				might_hit_bash(ch, victim);
			}
		}
		else if (might < 800)
		{
			sprintf(buf, "&g&qВаш богатырский удар пошатнул %s.&Q&n\r\n", PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 2;
			dam += (dam / 1);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_BATTLE;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = pc_duration(victim, 2, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(victim, af, TRUE, FALSE, TRUE, FALSE);
			sprintf(buf, "&R&qВаше сознание помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N пошатнул$U от богатырского удара $n1.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
			if (!number(0, 1))
			{
				might_hit_bash(ch, victim);
			}
		}
		else
		{
			sprintf(buf, "&G&qВаш богатырский удар сотряс %s.&Q&n\r\n", PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 2;
			dam *= 4;
			WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_BATTLE;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = pc_duration(victim, 3, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(victim, af, TRUE, FALSE, TRUE, FALSE);
			sprintf(buf, "&R&qВаше сознание померкло после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N зашатал$U от богатырского удара $n1.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
			might_hit_bash(ch, victim);
		}
	}
	set_wait(ch, lag, TRUE);
}

void HitData::try_stupor_dam(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int percent = number(1, skill_info[SKILL_STUPOR].max_percent);
	int prob = train_skill(ch, SKILL_STUPOR, skill_info[SKILL_STUPOR].max_percent, victim);
	int lag = 0;

	if (GET_MOB_HOLD(victim))
	{
		prob = MAX(prob, percent * 150 / 100 + 1);
	}

	if (IS_IMMORTAL(victim))
	{
		prob = 0;
	}

	if (prob * 100 / percent < 117
		|| dam == 0
		|| MOB_FLAGGED(victim, MOB_NOSTUPOR))
	{
		sprintf(buf, "&c&qВы попытались оглушить %s, но не смогли.&Q&n\r\n", PERS(victim, ch, 3));
		send_to_char(buf, ch);
		lag = 3;
		dam = 0;
	}
	else if (prob * 100 / percent < 300)
	{
		sprintf(buf, "&g&qВаша мощная атака оглушила %s.&Q&n\r\n", PERS(victim, ch, 3));
		send_to_char(buf, ch);
		lag = 2;
		int k = ch->get_skill(SKILL_STUPOR) / 30;
		if (!IS_NPC(victim))
		{
			k = MIN(2, k);
		}
		dam *= MAX(2, number(1, k));
		WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
		sprintf(buf, "&R&qВаше сознание слегка помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
		send_to_char(buf, victim);
		act("$n оглушил$a $N3.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}
	else
	{
		if (MOB_FLAGGED(victim, MOB_NOBASH))
		{
			sprintf(buf, "&G&qВаш мощнейший удар оглушил %s.&Q&n\r\n", PERS(victim, ch, 3));
		}
		else
		{
			sprintf(buf, "&G&qВаш мощнейший удар сбил %s с ног.&Q&n\r\n", PERS(victim, ch, 3));
		}
		send_to_char(buf, ch);
		if (MOB_FLAGGED(victim, MOB_NOBASH))
		{
			act("$n мощным ударом оглушил$a $N3.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else
		{
			act("$n своим оглушающим ударом сбил$a $N3 с ног.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		lag = 2;
		int k = ch->get_skill(SKILL_STUPOR) / 20;
		if (!IS_NPC(victim))
		{
			k = MIN(4, k);
		}
		dam *= MAX(3, number(1, k));
		WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
		if (GET_POS(victim) > POS_SITTING && !MOB_FLAGGED(victim, MOB_NOBASH))
		{
			GET_POS(victim) = POS_SITTING;
			sprintf(buf, "&R&qОглушающий удар %s сбил вас с ног.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
		}
		else
		{
			sprintf(buf, "&R&qВаше сознание слегка помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
		}
	}
	set_wait(ch, lag, TRUE);
}

int HitData::extdamage(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__))
	{
		return 0;
	}

	const int mem_dam = dam;

	if (dam < 0)
	{
		dam = 0;
	}

	//* богатырский молот //
	// в эти условия ничего добавлять не надо, иначе EAF_MIGHTHIT не снимется
	// с моба по ходу боя, если он не может по каким-то причинам смолотить
	if (GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		&& GET_WAIT(ch) <= 0)
	{
		CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
		if (check_mighthit_weapon(ch) && !GET_AF_BATTLE(ch, EAF_TOUCH))
		{
			try_mighthit_dam(ch, victim);
		}
	}
	//* оглушить //
	// аналогично молоту, все доп условия добавляются внутри
	else if (GET_AF_BATTLE(ch, EAF_STUPOR) && GET_WAIT(ch) <= 0)
	{
		CLR_AF_BATTLE(ch, EAF_STUPOR);
		if (IS_NPC(ch) || IS_IMMORTAL(ch))
		{
			try_stupor_dam(ch, victim);
		}
		else if (wielded)
		{
			if (GET_OBJ_SKILL(wielded) == SKILL_BOWS)
			{
				send_to_char("Луком оглушить нельзя.\r\n", ch);
			}
			else if (!GET_AF_BATTLE(ch, EAF_PARRY) && !GET_AF_BATTLE(ch, EAF_MULTYPARRY))
			{
				if (GET_OBJ_WEIGHT(wielded) > 18)
				{
					try_stupor_dam(ch, victim);
				}
				else
				{
					send_to_char("&WВаше оружие слишком легкое, чтобы им можно было оглушить!&Q&n\r\n", ch);
				}
			}
		}
		else
		{
			sprintf(buf,"&c&qВы оказались без оружия, а пальцем оглушить нельзя.&Q&n\r\n");
			send_to_char(buf, ch);
			sprintf(buf,"&c&q%s оказался без оружия и не смог вас оглушить.&Q&n\r\n", GET_NAME(ch));
			send_to_char(buf, victim);
		}
	}
	//* яды со скила отравить //
	else if (!MOB_FLAGGED(victim, MOB_PROTECT)
		&& dam
		&& wielded
		&& wielded->has_timed_spell()
		&& ch->get_skill(SKILL_POISONED))
	{
		try_weap_poison(ch, victim, wielded->timed_spell().is_spell_poisoned());
	}
	//* травящий ядом моб //
	else if (dam
		&& IS_NPC(ch)
		&& NPC_FLAGGED(ch, NPC_POISON)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& GET_WAIT(ch) <= 0
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_POISON)
		&& number(0, 100) < GET_LIKES(ch) + GET_LEVEL(ch) - GET_LEVEL(victim)
		&& !general_savingthrow(ch, victim, SAVING_CRITICAL, - GET_REAL_CON(victim)))
	{
		poison_victim(ch, victim, MAX(1, GET_LEVEL(ch) - GET_LEVEL(victim)) * 10);
	}
	//* точный стиль //
	else if (dam && get_flags()[FightSystem::CRIT_HIT] && dam_critic)
	{
		compute_critical(ch, victim);
	}

	// Если удар парирован, необходимо все равно ввязаться в драку.
	// Вызывается damage с отрицательным уроном
	dam = mem_dam >= 0 ? dam : -1;

	Damage dmg(SkillDmg(skill_num), dam, FightSystem::PHYS_DMG);
	dmg.hit_type = hit_type;
	dmg.dam_critic = dam_critic;
	dmg.flags = get_flags();
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;

	return dmg.process(ch, victim);
}

/**
 * Инициализация всех нужных первичных полей (отделены в хедере), после
 * этой функции уже начинаются подсчеты собсна хитролов/дамролов и прочего.
 */
void HitData::init(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// Find weapon for attack number weapon //

	if (weapon == 1)
	{
		if (!(wielded = GET_EQ(ch, WEAR_WIELD)))
		{
			wielded = GET_EQ(ch, WEAR_BOTHS);
			weapon_pos = WEAR_BOTHS;
		}
	}
	else if (weapon == 2)
	{
		wielded = GET_EQ(ch, WEAR_HOLD);
		weapon_pos = WEAR_HOLD;
	}

	if (wielded
		&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		// для всех типов атак скилл берется из пушки, если она есть
		weap_skill = static_cast<ESkill>(GET_OBJ_SKILL(wielded));
	}
	else
	{
		// удар голыми руками
		weap_skill = SKILL_PUNCH;
	}
	weap_skill_is = train_skill(ch, weap_skill, skill_info[weap_skill].max_percent, victim);

	//* обработка SKILL_NOPARRYHIT //
	if (skill_num == TYPE_UNDEFINED && ch->get_skill(SKILL_NOPARRYHIT))
	{
		int tmp_skill = train_skill(ch, SKILL_NOPARRYHIT, skill_info[SKILL_NOPARRYHIT].max_percent, victim);
		// TODO: max_percent в данный момент 100 (хорошо бы не тупо 200, а с % фейла)
		if (tmp_skill >= number(1, skill_info[SKILL_NOPARRYHIT].max_percent))
		{
			hit_no_parry = true;
		}
	}

	if (GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT))
	{
		hit_no_parry = true;
	}

	if (wielded && GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		hit_type = GET_OBJ_VAL(wielded, 3);
	}
	else
	{
		weapon_pos = 0;
		if (IS_NPC(ch))
		{
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
 * test_self_hitroll() в данный момент.
 */
void HitData::calc_base_hr(CHAR_DATA *ch)
{
	if (skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB)
	{
		if (wielded
			&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON
			&& !IS_NPC(ch))
		{
			// Apply HR for light weapon
			int percent = 0;
			switch (weapon_pos)
			{
			case WEAR_WIELD:
				percent = (str_bonus(GET_REAL_STR(ch), STR_WIELD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			case WEAR_HOLD:
				percent = (str_bonus(GET_REAL_STR(ch), STR_HOLD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			case WEAR_BOTHS:
				percent = (str_bonus(GET_REAL_STR(ch), STR_WIELD_W) +
				   str_bonus(GET_REAL_STR(ch), STR_HOLD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			}
			calc_thaco -= MIN(3, MAX(percent, 0));

			// Penalty for unknown weapon type
			// shapirus: старый штраф нифига не работает, тем более, что unknown_weapon_fault
			// нигде не определяется. сделан новый на базе инты чара. плюс сделан штраф на дамролл.
			// если скилл есть, то штраф не даем, а применяем бонусы/штрафы по профам
			if (ch->get_skill(weap_skill) == 0)
			{
				calc_thaco += (50 - MIN(50, GET_REAL_INT(ch))) / 3;
				dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			}
			else
			{
				apply_weapon_bonus(GET_CLASS(ch), weap_skill, &dam, &calc_thaco);
			}
		}
		else if (!IS_NPC(ch))
		{
			// кулаками у нас полагается бить только богатырям :)
			if (!can_use_feat(ch, BULLY_FEAT))
				calc_thaco += 4;
			else	// а богатырям положен бонус за отсутствие оружия
				calc_thaco -= 3;
		}
		// Bonus for leadership
		if (calc_leadership(ch))
			calc_thaco -= 2;
	}

	check_weap_feats(ch);

	if (GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT))
	{
		calc_thaco -= MAX(0, (ch->get_skill(weap_skill) - 70) / 8);
	}

	//    AWAKE style - decrease hitroll
	if (GET_AF_BATTLE(ch, EAF_AWAKE)
		&& !can_use_feat(ch, SHADOW_STRIKE_FEAT)
		&& skill_num != SKILL_THROW
		&& skill_num != SKILL_BACKSTAB)
	{
		if (can_auto_block(ch))
		{
			// осторожка со щитом в руках у дружа с блоком - штрафы на хитролы (от 0 до 10)
			calc_thaco += ch->get_skill(SKILL_AWAKE) * 5 / 100;
		}
		else
		{
			// здесь еще были штрафы на дамаг через деление, но положительного дамага
			// на этом этапе еще нет, так что делили по сути нули
			calc_thaco += ((ch->get_skill(SKILL_AWAKE) + 9) / 10) + 2;
		}
	}

	if (!IS_NPC(ch) && skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB)
	{
		// Casters use weather, int and wisdom
		if (IS_CASTER(ch))
		{
			/*	  calc_thaco +=
				    (10 -
				     complex_skill_modifier (ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS,
							     10));
			*/
			calc_thaco -= (int)((GET_REAL_INT(ch) - 13) / GET_LEVEL(ch));
			calc_thaco -= (int)((GET_REAL_WIS(ch) - 13) / GET_LEVEL(ch));
		}
		// Skill level increase damage
		if (ch->get_skill(weap_skill) >= 60)
			dam += ((ch->get_skill(weap_skill) - 50) / 10);
	}

	// bless
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLESS))
	{
		calc_thaco -= 4;
	}
	// curse
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CURSE))
	{
		calc_thaco += 6;
		dam -= 5;
	}

	// Учет мощной и прицельной атаки
	if (PRF_FLAGGED(ch, PRF_POWERATTACK) && can_use_feat(ch, POWER_ATTACK_FEAT))
	{
		calc_thaco += 2;
		dam += 5;
	}
	else if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK) && can_use_feat(ch, GREAT_POWER_ATTACK_FEAT))
	{
		calc_thaco += 4;
		dam += 10;
	}
	else if (PRF_FLAGGED(ch, PRF_AIMINGATTACK) && can_use_feat(ch, AIMING_ATTACK_FEAT))
	{
		calc_thaco -= 2;
		dam -= 5;
	}
	else if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK) && can_use_feat(ch, GREAT_AIMING_ATTACK_FEAT))
	{
		calc_thaco -= 4;
		dam -= 10;
	}

	// Calculate the THAC0 of the attacker
	if (!IS_NPC(ch))
	{
		calc_thaco += thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	}
	else
	{
		// штраф мобам по рекомендации Триглава
		calc_thaco += (25 - GET_LEVEL(ch) / 3);
	}

	//Вычисляем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);

	calc_thaco -= GET_REAL_HR(ch) * p_hitroll;
	
	// Использование ловкости вместо силы для попадания
	if (can_use_feat(ch, WEAPON_FINESSE_FEAT))
	{
		if (wielded && GET_OBJ_WEIGHT(wielded) > 20)
			calc_thaco -= str_bonus(GET_REAL_STR(ch), STR_TO_HIT) * p_hitroll;
		else
			calc_thaco -= str_bonus(GET_REAL_DEX(ch), STR_TO_HIT) * p_hitroll;
	}
	else
	{
		calc_thaco -= str_bonus(GET_REAL_STR(ch), STR_TO_HIT) * p_hitroll;
	}

	if ((skill_num == SKILL_THROW
			|| skill_num == SKILL_BACKSTAB)
		&& wielded
		&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		if (skill_num == SKILL_BACKSTAB)
		{
			calc_thaco -= MAX(0, (ch->get_skill(SKILL_SNEAK) + ch->get_skill(SKILL_HIDE) - 100) / 30);
		}
	}
	else
	{
		// тюнинг оверности делается тут :)
		calc_thaco += 4;
	}

	//dzMUDiST Обработка !исступления! +Gorrah
	if (affected_by_spell(ch, SPELL_BERSERK))
	{
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK))
		{
			calc_thaco -= (12 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
		}
	}
	
}

/**
 * Соответственно подсчет динамических хитролов, не попавших в calc_base_hr()
 * Все, что меняется от раза к разу или при разных противниках
 * При любом изменении содержимого нужно поддерживать адекватность calc_stat_hr()
 */
void HitData::calc_rand_hr(CHAR_DATA *ch, CHAR_DATA *victim)
{
	//считаем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// штраф в размере 1 хитролла за каждые
	// недокачанные 10% скилла "удар левой рукой"
	
	if (weapon == LEFT_WEAPON
		&& skill_num != SKILL_THROW
		&& skill_num != SKILL_BACKSTAB
		&& !(wielded && GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
		&& !IS_NPC(ch))
	{
		calc_thaco += (skill_info[SKILL_SHIT].max_percent -
		   train_skill(ch, SKILL_SHIT, skill_info[SKILL_SHIT].max_percent, victim)) / 10;
	}

	// courage
	if (affected_by_spell(ch, SPELL_COURAGE))
	{
		int range = number(1, skill_info[SKILL_COURAGE].max_percent + GET_REAL_MAX_HIT(ch) - GET_HIT(ch));
		int prob = train_skill(ch, SKILL_COURAGE, skill_info[SKILL_COURAGE].max_percent, victim);
		if (prob > range)
		{
			dam += ((ch->get_skill(SKILL_COURAGE) + 19) / 20);
			calc_thaco -= ((ch->get_skill(SKILL_COURAGE) + 9) / 20) * p_hitroll;
		}
	}

	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB && on_horse(ch))
	{
		train_skill(ch, SKILL_HORSE, skill_info[SKILL_HORSE].max_percent, victim);
//		dam += ((prob + 19) / 10);
//		int range = number(1, skill_info[SKILL_HORSE].max_percent);
//		if (range > prob)
//			calc_thaco += ((range - prob) + 19 / 20);
//		else
//			calc_thaco -= ((prob - range) + 19 / 20);
		calc_thaco += 10 - GET_SKILL(ch, SKILL_HORSE) / 20;
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
	if (skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB)
	{
		int prob = (ch->get_skill(weap_skill) + cha_app[GET_REAL_CHA(ch)].illusive) -
			   (victim->get_skill(weap_skill) + int_app[GET_REAL_INT(victim)].observation);
		if (prob >= 30 && !GET_AF_BATTLE(victim, EAF_AWAKE)
				&& (IS_NPC(ch) || !GET_AF_BATTLE(ch, EAF_PUNCTUAL)))
		{
			calc_thaco -= (ch->get_skill(weap_skill) - victim->get_skill(weap_skill) > 60 ? 2 : 1) * p_hitroll;
			if (!IS_NPC(victim))
				dam += (prob >= 70 ? 3 : (prob >= 50 ? 2 : 1));
		}
	}

	// AWAKE style for victim
	if (GET_AF_BATTLE(victim, EAF_AWAKE)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !GET_MOB_HOLD(victim)
		&& train_skill(victim, SKILL_AWAKE, skill_info[SKILL_AWAKE].max_percent,
			ch) >= number(1, skill_info[SKILL_AWAKE].max_percent))
	{
		// > и зачем так? кто балансом занимается поправте.
		// воткнул мин. разницу, чтобы анализаторы не ругались
		dam -= IS_NPC(ch) ? 5 : 4;
		calc_thaco += IS_NPC(ch) ? 4 : 2;
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
void HitData::calc_stat_hr(CHAR_DATA *ch)
{
	//считаем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// штраф в размере 1 хитролла за каждые
	// недокачанные 10% скилла "удар левой рукой"
	if (weapon == LEFT_WEAPON
		&& skill_num != SKILL_THROW
		&& skill_num != SKILL_BACKSTAB
		&& !(wielded && GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
		&& !IS_NPC(ch))
	{
		calc_thaco += (skill_info[SKILL_SHIT].max_percent - ch->get_skill(SKILL_SHIT)) / 10;
	}

	// courage
	if (affected_by_spell(ch, SPELL_COURAGE))
	{
		dam += ((ch->get_skill(SKILL_COURAGE) + 19) / 20);
		calc_thaco -= ((ch->get_skill(SKILL_COURAGE) + 9) / 20) * p_hitroll;
	}

	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB && on_horse(ch))
	{
		int prob = ch->get_skill(SKILL_HORSE);
		int range = skill_info[SKILL_HORSE].max_percent / 2;

		dam += ((prob + 19) / 10);

		if (range > prob)
			calc_thaco += ((range - prob) + 19 / 20);
		else
			calc_thaco -= ((prob - range) + 19 / 20);
	}

	// скилл владения пушкой или голыми руками
	if (ch->get_skill(weap_skill) <= 80)
		calc_thaco -= (ch->get_skill(weap_skill) / 20)*p_hitroll;
	else if (ch->get_skill(weap_skill) <= 160)
		calc_thaco -= (4 + (ch->get_skill(weap_skill) - 80) / 10)*p_hitroll;
	else
		calc_thaco -= (4 + 8 + (ch->get_skill(weap_skill) - 160) / 5)*p_hitroll;
}

// * Подсчет армор класса жертвы.
void HitData::calc_ac(CHAR_DATA *victim)
{
	
	
	// Calculate the raw armor including magic armor.  Lower AC is better.
	victim_ac += compute_armor_class(victim);
	victim_ac /= 10;
	//считаем штраф за голод
	if (!IS_NPC(victim) && victim_ac<5) { //для голодных
		int p_ac = (1 - victim->get_cond_penalty(P_AC))*40;
		if (p_ac)  {
			if (victim_ac+p_ac>5) {
				victim_ac = 5;
			} else {
				victim_ac+=p_ac;
			}
		}
	}
	
	if (GET_POS(victim) < POS_FIGHTING)
		victim_ac += 4;
	if (GET_POS(victim) < POS_RESTING)
		victim_ac += 3;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD))
		victim_ac += 4;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_CRYING))
		victim_ac += 4;
}

// * Обработка защитных скиллов: захват, уклон, веер, блок.
void HitData::check_defense_skills(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (!hit_no_parry)
	{
		// обработаем ситуацию ЗАХВАТ
		const auto& people = world[ch->in_room]->people;
		for (const auto vict : people)
		{
			if (dam < 0)
			{
				break;
			}

			hit_touching(ch, vict, &dam);
		}
	}

	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, EAF_DEVIATE)
		&& GET_WAIT(victim) <= 0
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& GET_MOB_HOLD(victim) == 0
		&& BATTLECNTR(victim) < (GET_LEVEL(victim) + 7) / 8)
	{
		// Обработаем команду   УКЛОНИТЬСЯ
		hit_deviate(ch, victim, &dam);
	}
	else
	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, EAF_PARRY)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0)
	{
		// обработаем команду  ПАРИРОВАТЬ
		hit_parry(ch, victim, weap_skill, hit_type, &dam);
	}
	else
	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, EAF_MULTYPARRY)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& BATTLECNTR(victim) < (GET_LEVEL(victim) + 4) / 5
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0)
	{
		// обработаем команду  ВЕЕРНАЯ ЗАЩИТА
		hit_multyparry(ch, victim, weap_skill, hit_type, &dam);
	}
	else
	if (dam > 0
		&& !hit_no_parry
		&& ((GET_AF_BATTLE(victim, EAF_BLOCK) || can_auto_block(victim)) && GET_POS(victim) > POS_SITTING)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0
		&& BATTLECNTR(victim) < (GET_LEVEL(victim) + 8) / 9)
	{
		// Обработаем команду   БЛОКИРОВАТЬ
		hit_block(ch, victim, &dam);
	}
}

/**
 * В данный момент:
 * добавление дамролов с пушек
 * добавление дамага от концентрации силы
 */
void HitData::add_weapon_damage(CHAR_DATA *ch)
{
	int damroll = dice(GET_OBJ_VAL(wielded, 1),
		GET_OBJ_VAL(wielded, 2));

	if (IS_NPC(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& !(MOB_FLAGGED(ch, MOB_ANGEL)|| MOB_FLAGGED(ch, MOB_GHOST)))
	{
		damroll *= MOB_DAMAGE_MULT;
	}
	else
	{
		damroll = MIN(damroll,
			damroll * GET_OBJ_CUR(wielded) / MAX(1, GET_OBJ_MAX(wielded)));
	}

	damroll = calculate_strconc_damage(ch, wielded, damroll);
	dam += MAX(1, damroll);
}

// * Добавление дамага от голых рук и молота.
void HitData::add_hand_damage(CHAR_DATA *ch)
{
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STONEHAND))
		dam += dice(2, 3);
	else
		dam += number(1, 3);

	if (can_use_feat(ch, BULLY_FEAT))
	{
		dam += GET_LEVEL(ch) / 5;
		dam += MAX(0, GET_REAL_STR(ch) - 25);
	}

	// Мультипликатор повреждений без оружия и в перчатках (линейная интерполяция)
	// <вес перчаток> <увеличение>
	// 0  50%
	// 5 100%
	// 10 150%
	// 15 200%
	// НА МОЛОТ НЕ ВЛИЯЕТ
	if (!GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		|| get_flags()[FightSystem::CRIT_HIT]) //в метком молоте идет учет перчаток
	{
		int modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? MIN(GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)), 18) : 0)); //вес перчаток больше 18 не учитывается
		if (IS_NPC(ch) || can_use_feat(ch, BULLY_FEAT))
		{
			modi = MAX(100, modi);
		}
		dam *= modi / 100;
	}
}

// * Расчет шанса на критический удар (не точкой).
void HitData::calc_crit_chance(CHAR_DATA *ch)
{
	dam_critic = 0;
	int calc_critic = 0;

	// Маги, волхвы и не-купеческие чармисы не умеют критать //
	if ((!IS_NPC(ch) && !IS_MAGIC_USER(ch) && !IS_DRUID(ch))
		|| (IS_NPC(ch) && (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER))))
	{
		calc_critic = MIN(ch->get_skill(weap_skill), 70);
		// Мастерские фиты по оружию удваивают шанс критического попадания //
		for (int i = PUNCH_MASTER_FEAT; i <= BOWS_MASTER_FEAT; i++)
		{
			if ((ubyte) feat_info[i].affected[0].location == weap_skill && can_use_feat(ch, i))
			{
				calc_critic += MAX(0, ch->get_skill(weap_skill) -  70);
				break;
			}
		}
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
		{
			calc_critic += ch->get_skill(SKILL_BACKSTAB);
		}
		//Нафига тут проверять класс?
		//Скиллы уникальные, другим классам все равно недоступны.
		//А чтоб мобы не лютовали -- есть проверка на игрока.
		if (!IS_NPC(ch))
		{
			calc_critic += (int)(ch->get_skill(SKILL_PUNCTUAL) / 2);
			calc_critic += (int)(ch->get_skill(SKILL_NOPARRYHIT) / 3);
		}
		if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		{
			calc_critic += GET_LEVEL(ch);
		}
	}
	else
	{
		//Polud не должны - так пусть и не критают
		reset_flag(FightSystem::CRIT_HIT);
	}

	//critical hit ignore magic_shields and armour
	if (number(0, 2000) < calc_critic)
	{
		set_flag(FightSystem::CRIT_HIT);
	}
	else
	{
		reset_flag(FightSystem::CRIT_HIT);
	}
}

/* Бонусная дамага от "пореза" и других приемов, буде таковые реализуют */
double expedient_bonus_damage(CHAR_DATA *ch, CHAR_DATA *victim)
{
	double factor = 1.00;

	switch (ch->get_extra_attack_mode())
	{
	case EXTRA_ATTACK_CUT_SHORTS:
        if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_AWARE))
        {
		factor = 1 + ch->get_skill(SKILL_SHORTS)/35.0;
        } else
        {
		factor = 1 + ch->get_skill(SKILL_SHORTS)/30.0;
        }
        break;
	case EXTRA_ATTACK_CUT_PICK:
		factor = 1 + ch->get_skill(SKILL_PICK)/70.0;
        break;
	default:
		factor = 1.00;
        break;
	}
	if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
	{
        sprintf(buf, "&CМножитель урона от приема = %f&n\r\n", factor);
        send_to_char(buf,ch);
    }

    return factor;
};

/**
* обработка ударов оружием, санка, призма, стили, итд.
* \param weapon = 1 - атака правой или двумя руками
*               = 2 - атака левой рукой
*/
void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon)
{
	if (!victim)
	{
		return;
	}
	if (!ch || ch->purged() || victim->purged())
	{
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim->purged() ? "purged" : "true", __FILE__, __LINE__);
		return;
	}
	// Do some sanity checking, in case someone flees, etc.
	if (ch->in_room != IN_ROOM(victim) || ch->in_room == NOWHERE)
	{
		if (ch->get_fighting() && ch->get_fighting() == victim)
		{
			stop_fighting(ch, TRUE);
		}
		return;
	}
	// Stand awarness mobs
	if (CAN_SEE(victim, ch)
		&& !victim->get_fighting()
		&& ((IS_NPC(victim) && (GET_HIT(victim) < GET_MAX_HIT(victim)
			|| MOB_FLAGGED(victim, MOB_AWARE)))
			|| AFF_FLAGGED(victim, EAffectFlag::AFF_AWARNESS))
		&& !GET_MOB_HOLD(victim) && GET_WAIT(victim) <= 0)
	{
		set_battle_pos(victim);
	}

	//go_autoassist(ch);

	// старт инициализации полей для удара
	HitData hit_params;
	//конвертация скиллов, которые предполагают вызов hit()
	//c тип_андеф, в тип_андеф.
	hit_params.skill_num = type != SKILL_STUPOR && type != SKILL_MIGHTHIT ? type : TYPE_UNDEFINED;
	hit_params.weapon = weapon;
	hit_params.init(ch, victim);

	//  дополнительный маг. дамаг независимо от попадания физ. атаки
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CLOUD_OF_ARROWS) && hit_params.skill_num < 0
		&& (ch->get_fighting()
		|| (!GET_AF_BATTLE(ch, EAF_MIGHTHIT) && !GET_AF_BATTLE(ch, EAF_STUPOR))))
	{
		// здесь можно получить спурженного victim, но ch не умрет от зеркала
		mag_damage(1, ch, victim, SPELL_MAGIC_MISSILE, SAVING_REFLEX);
		if (ch->purged() || victim->purged())
		{
			return;
		}
	}

	// вычисление хитролов/ац
	hit_params.calc_base_hr(ch);
	hit_params.calc_rand_hr(ch, victim);
	hit_params.calc_ac(victim);

	const int victim_lvl_miss = victim->get_level() + victim->get_remort();
	const int ch_lvl_miss = ch->get_level() + ch->get_remort();

	// собсно выяснение попали или нет
	if (victim_lvl_miss - ch_lvl_miss <= 5
		|| (!IS_NPC(ch) && !IS_NPC(victim)))
	{
		// 5% шанс промазать, если цель в пределах 5 уровней или пвп случай
		if ((number(1, 100) <= 5))
		{
			hit_params.dam = 0;
			hit_params.extdamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	}
	else
	{
		// шанс промазать = разнице уровней и мортов
		const int diff = victim_lvl_miss - ch_lvl_miss;
		if (number(1, 100) <= diff)
		{
			hit_params.dam = 0;
			hit_params.extdamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	}
	// всегда есть 5% вероятность попасть (diceroll == 20)
	if ((hit_params.diceroll < 20 && AWAKE(victim))
		&& hit_params.calc_thaco - hit_params.diceroll > hit_params.victim_ac)
	{
		hit_params.dam = 0;
		hit_params.extdamage(ch, victim);
		hitprcnt_mtrigger(victim);
		return;
	}
	// даже в случае попадания можно уклониться мигалкой
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_BLINK)
		&& !GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		&& !GET_AF_BATTLE(ch, EAF_STUPOR)
		&& (!(hit_params.skill_num == SKILL_BACKSTAB && can_use_feat(ch, THIEVES_STRIKE_FEAT)))
		&& number(1, 100) <= 20)
	{
		sprintf(buf,
			"%sНа мгновение вы исчезли из поля зрения противника.%s\r\n",
			CCINRM(victim, C_NRM), CCNRM(victim, C_NRM));
		send_to_char(buf, victim);
		hit_params.dam = 0;
		hit_params.extdamage(ch, victim);
		return;
	}

	// обработка по факту попадания
	hit_params.dam += get_real_dr(ch);
	if (can_use_feat(ch, SHOT_FINESSE_FEAT))
		hit_params.dam += str_bonus(GET_REAL_DEX(ch), STR_TO_DAM);
	else
		hit_params.dam += str_bonus(GET_REAL_STR(ch), STR_TO_DAM);

	// рандом разброс базового дамага
	if (hit_params.dam > 0)
	{
		int min_rnd = hit_params.dam - hit_params.dam / 4;
		int max_rnd = hit_params.dam + hit_params.dam / 4;
		hit_params.dam = MAX(1, number(min_rnd, max_rnd));
	}

	if (GET_EQ(ch, WEAR_BOTHS) && hit_params.weap_skill != SKILL_BOWS)
		hit_params.dam *= 2;

	if (IS_NPC(ch))
	{
		hit_params.dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
	}

	// расчет критических ударов
	hit_params.calc_crit_chance(ch);

	// оружие/руки и модификаторы урона скилов, с ними связанных
	if (hit_params.wielded
		&& GET_OBJ_TYPE(hit_params.wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		hit_params.add_weapon_damage(ch);
		// скрытый удар
		int tmp_dam = calculate_noparryhit_dmg(ch, hit_params.wielded);
		if (tmp_dam > 0)
		{
			// 0 раунд и стаб = 70% скрытого, дальше раунд * 0.4 (до 5 раунда)
			int round_dam = tmp_dam * 7 / 10;
			if (can_use_feat(ch, SNEAKRAGE_FEAT))
			{
				if (ROUND_COUNTER(ch) >= 1 && ROUND_COUNTER(ch) <= 3)
				{
					hit_params.dam *= ROUND_COUNTER(ch);
				}
			}
			if (hit_params.skill_num == SKILL_BACKSTAB || ROUND_COUNTER(ch) <= 0)
			{
				hit_params.dam += round_dam;
			}
			else
			{
				hit_params.dam += round_dam * MIN(3, ROUND_COUNTER(ch));
			}
		}
	}
	else
	{
		hit_params.add_hand_damage(ch);
	}

	// Gorrah: бонус к повреждениям от умения "железный ветер"
	// x5 по идее должно быть
	if (GET_AF_BATTLE(ch, EAF_IRON_WIND))
		hit_params.dam += ch->get_skill(SKILL_IRON_WIND) / 2;
	//dzMUDiST Обработка !исступления! +Gorrah
	if (affected_by_spell(ch, SPELL_BERSERK))
	{
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK))
		{
			hit_params.dam = (hit_params.dam * MAX(150, 150 + GET_LEVEL(ch) + dice(0, GET_REMORT(ch)) * 2)) / 100;
		}
	}

	// at least 1 hp damage min per hit
	hit_params.dam = MAX(1, hit_params.dam);
	if (GET_SKILL(ch, SKILL_HORSE) > 100 && on_horse(ch))
	{
		hit_params.dam *= 1 + (GET_SKILL(ch, SKILL_HORSE) - 100) / 500.0; // на лошадке до +20%
	}

	// зовется до alt_equip, чтобы не абузить повреждение пушек
	if (damage_mtrigger(ch, victim))
	{
		return;
	}

	if (hit_params.weapon_pos)
	{
		alt_equip(ch, hit_params.weapon_pos, hit_params.dam, 10);
	}

	if (hit_params.skill_num == SKILL_BACKSTAB)
	{
		hit_params.reset_flag(FightSystem::CRIT_HIT);
		hit_params.set_flag(FightSystem::IGNORE_FSHIELD);
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT) || can_use_feat(ch, SHADOW_STRIKE_FEAT))
		{
			// тати игнорят броню полностью
			// и наемы тоже!
			hit_params.set_flag(FightSystem::IGNORE_ARMOR);
		}
		else
		{
			//мобы игнорят вполовину
			hit_params.set_flag(FightSystem::HALF_IGNORE_ARMOR);
		}
		// Наемы фигачат больше
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && IS_NPC(victim))
		{
			hit_params.dam *= backstab_mult(GET_LEVEL(ch)) * (1.0 + ch->get_skill(SKILL_NOPARRYHIT) / 200.0);
		}
		else if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
		{
			if (victim->get_fighting())
			{
				hit_params.dam *= backstab_mult(GET_LEVEL(ch));
			}
			else
			{
				hit_params.dam *= backstab_mult(GET_LEVEL(ch)) * 1.3;
			}
		}
		else
		{
			hit_params.dam *= backstab_mult(GET_LEVEL(ch));
		}
		
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT)
			&& IS_NPC(victim)
			&& !(AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD)
				&& !(MOB_FLAGGED(victim, MOB_PROTECT)))
			&& (number(1,100) <= 6 * ch->get_cond_penalty(P_HITROLL)) //голодный наем снижаем скрытый удар
			&& IS_NPC(victim)
			&& !victim->get_role(MOB_ROLE_BOSS))
		{
			    GET_HIT(victim) = 1;
			    hit_params.dam = 2000; // для надежности
			    send_to_char(ch, "&GПрямо в сердце, насмерть!&n\r\n");
			    hit_params.extdamage(ch, victim);
			    return;
		}
		
		if (number(1, 100) < calculate_crit_backstab_percent(ch) * ch->get_cond_penalty(P_HITROLL)
			&& !general_savingthrow(ch, victim, SAVING_REFLEX, dex_bonus(GET_REAL_DEX(ch))))
		{
			hit_params.dam = static_cast<int>(hit_params.dam * hit_params.crit_backstab_multiplier(ch, victim));
			if ((hit_params.dam > 0)
				&& !AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD)
				&& !(MOB_FLAGGED(victim, MOB_PROTECT)))
			{
				send_to_char("&GПрямо в сердце!&n\r\n", ch);
			}
		}

		int initial_dam = hit_params.dam;
		//Adept: учитываем резисты от крит. повреждений
		hit_params.dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, hit_params.dam);
		// выводим временно влияние живучести
		if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
		{
			sprintf(buf, "&CДамага стаба до учета живучести = %d, живучесть = %d, коэфициент разницы = %g&n\r\n", initial_dam, GET_RESIST(victim, VITALITY_RESISTANCE), float(hit_params.dam) / initial_dam);
			send_to_char(buf,ch);
		}
		// режем стаб
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && !IS_NPC(ch))
		{
			hit_params.dam = MIN(8000 + GET_REMORT(ch) * 20 * GET_LEVEL(ch), hit_params.dam);
		}

		if (IS_IMPL(ch) || IS_IMPL(victim) || PRF_FLAGGED(ch, PRF_TESTER))
		{
			sprintf(buf, "&CДамага стаба равна = %d&n\r\n", hit_params.dam);
			send_to_char(buf,ch);
			sprintf(buf, "&RДамага стаба  равна = %d&n\r\n", hit_params.dam);
			send_to_char(buf,victim);
		}
		hit_params.extdamage(ch, victim);
		return;
	}

	if (hit_params.skill_num == SKILL_THROW)
	{
		hit_params.set_flag(FightSystem::IGNORE_FSHIELD);
		hit_params.dam *= (calculate_skill(ch, SKILL_THROW, victim) + 10) / 10;
		if (IS_NPC(ch))
		{
			hit_params.dam = MIN(300, hit_params.dam);
		}
		hit_params.dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, hit_params.dam);
		hit_params.extdamage(ch, victim);
		if (IS_IMPL(ch) || IS_IMPL(victim))
		{
			sprintf(buf, "&CДамага метнуть равна = %d&n\r\n", hit_params.dam);
			send_to_char(buf,ch);
			sprintf(buf, "&RДамага метнуть равна = %d&n\r\n", hit_params.dam);
			send_to_char(buf,victim);
		}
		return;
	}

	if (GET_AF_BATTLE(ch, EAF_PUNCTUAL)
		&& GET_PUNCTUAL_WAIT(ch) <= 0
		&& GET_WAIT(ch) <= 0
		&& (hit_params.diceroll >= 18 - GET_MOB_HOLD(victim))
		&& !MOB_FLAGGED(victim, MOB_NOTKILLPUNCTUAL))
	{
		int percent = train_skill(ch, SKILL_PUNCTUAL, skill_info[SKILL_PUNCTUAL].max_percent, victim);
		if (!PUNCTUAL_WAITLESS(ch))
		{
			PUNCTUAL_WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}

		if (percent >= number(1, skill_info[SKILL_PUNCTUAL].max_percent)
			&& (hit_params.calc_thaco - hit_params.diceroll < hit_params.victim_ac - 5
				|| percent >= skill_info[SKILL_PUNCTUAL].max_percent))
		{
			hit_params.set_flag(FightSystem::CRIT_HIT);
			// CRIT_HIT и так щиты игнорит, но для порядку
			hit_params.set_flag(FightSystem::IGNORE_FSHIELD);
			hit_params.dam_critic = do_punctual(ch, victim, hit_params.wielded);
			if (IS_IMPL(ch) || IS_IMPL(victim))
			{
				sprintf(buf, "&CДамага точки равна = %d&n\r\n", hit_params.dam_critic);
				send_to_char(buf,ch);
			}

			if (!PUNCTUAL_WAITLESS(ch))
			{
				PUNCTUAL_WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
			}
		}
	}

	if (ch->get_extra_attack_mode() != EXTRA_ATTACK_UNUSED)
        hit_params.dam *= expedient_bonus_damage(ch, victim);

	if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
	{
		sprintf(buf, "&CРегуляр дамаг = %d&n\r\n", hit_params.dam);
		send_to_char(buf,ch);
	}

	// обнуляем флаги, если у нападающего есть лаг
	if ((GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT))
		&& GET_WAIT(ch) > 0)
	{
		CLR_AF_BATTLE(ch, EAF_STUPOR);
		CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
	}

	// обработка защитных скилов (захват, уклон, парир, веер, блок)
	hit_params.check_defense_skills(ch, victim);
	
	//режем дамаг от голода
	if (hit_params.dam)
		hit_params.dam *= ch->get_cond_penalty(P_DAMROLL);
	// итоговый дамаг
	int made_dam = hit_params.extdamage(ch, victim);

	//Обнуление лага, когда виктим убит с применением
	//оглушить или молотить. Чтобы все это было похоже на
	//действие скиллов экстраатак(пнуть, сбить и т.д.)
	if (CHECK_WAIT(ch)
		&& made_dam == -1
		&& (type == SKILL_STUPOR
			|| type == SKILL_MIGHTHIT))
	{
		ch->set_wait(0u);
	}

	// check if the victim has a hitprcnt trigger
	if (made_dam != -1)
	{
		// victim is not dead after hit
		hitprcnt_mtrigger(victim);
	}
}

OBJ_DATA *GetUsedWeapon(CHAR_DATA *ch, int SelectedWeapon)
{
	OBJ_DATA *UsedWeapon = nullptr;

	if (SelectedWeapon == RIGHT_WEAPON)
	{
		if (!(UsedWeapon = GET_EQ(ch, WEAR_WIELD)))
			UsedWeapon = GET_EQ(ch, WEAR_BOTHS);
	}
	else if (SelectedWeapon == LEFT_WEAPON)
		UsedWeapon = GET_EQ(ch, WEAR_HOLD);

    return UsedWeapon;
}

//**** This function realize second shot for bows ******
void exthit(CHAR_DATA * ch, int type, int weapon)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)",
				ch ? (ch->purged() ? "purged" : "true") : "false",
				__FILE__, __LINE__);
		return;
	}

	OBJ_DATA *wielded = NULL;
	int percent = 0, prob = 0, div = 0, moves = 0;

	if (IS_NPC(ch))
	{
		if (MOB_FLAGGED(ch, MOB_EADECREASE) && weapon > 1)
		{
			if (ch->mob_specials.ExtraAttack * GET_HIT(ch) * 2 < weapon * GET_REAL_MAX_HIT(ch))
				return;
		}
		if (MOB_FLAGGED(ch, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH |
							 MOB_ACIDBREATH | MOB_LIGHTBREATH)))
		{
			for (prob = percent = 0; prob <= 4; prob++)
				if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
					percent++;
			percent = weapon % percent;
			for (prob = 0; prob <= 4; prob++)
			{
				if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
				{
					if (0 == percent)
					{
						break;
					}

					--percent;
				}
			}

			if (MOB_FLAGGED(ch, MOB_AREA_ATTACK))
			{
				const auto people = world[ch->in_room]->people;	// make copy because inside loop this list might be changed.
				for (const auto& tch : people)
				{
					if (IS_IMMORTAL(tch)
						|| ch->in_room == NOWHERE
						|| IN_ROOM(tch) == NOWHERE)
					{
						continue;
					}

					if (tch != ch
						&& !same_group(ch, tch))
					{
						mag_damage(GET_LEVEL(ch), ch, tch, SPELL_FIRE_BREATH + MIN(prob, 4), SAVING_CRITICAL);
					}
				}
			}
			else
			{
				mag_damage(GET_LEVEL(ch), ch, ch->get_fighting(), SPELL_FIRE_BREATH + MIN(prob, 4), SAVING_CRITICAL);
			}

			return;
		}
	}

	wielded = GetUsedWeapon(ch, weapon);

	if (wielded
			&& !GET_EQ(ch, WEAR_SHIELD)
			&& GET_OBJ_SKILL(wielded) == SKILL_BOWS
			&& GET_EQ(ch, WEAR_BOTHS))
	{
		// Лук в обеих руках - юзаем доп. или двойной выстрел
		if (can_use_feat(ch, DOUBLESHOT_FEAT) && !ch->get_skill(SKILL_ADDSHOT)
				&& MIN(850, 200 + ch->get_skill(SKILL_BOWS) * 4 + GET_REAL_DEX(ch) * 5) >= number(1, 1000))
		{
			hit(ch, ch->get_fighting(), type, weapon);
			prob = 0;
		}
		else if (ch->get_skill(SKILL_ADDSHOT) > 0)
		{
			addshot_damage(ch, type, weapon);
		}
	}

	/*
	собсно работа скилла "железный ветер"
	первая дополнительная атака правой наносится 100%
	вторая дополнительная атака правой начинает наноситься с 80%+ скилла, но не более чем с 80% вероятностью
	первая дополнительная атака левой начинает наноситься сразу, но не более чем с 80% вероятностью
	вторая дополнительная атака левей начинает наноситься с 170%+ скилла, но не более чем с 30% вероятности
	*/
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND))
	{
		percent = ch->get_skill(SKILL_IRON_WIND);
		moves = GET_MAX_MOVE(ch) / (6 + MAX(10, percent) / 10);
		prob = GET_AF_BATTLE(ch, EAF_IRON_WIND);
		if (prob && !check_moves(ch, moves))
		{
			CLR_AF_BATTLE(ch, EAF_IRON_WIND);
		}
		else if (!prob && (GET_MOVE(ch) > moves))
		{
			SET_AF_BATTLE(ch, EAF_IRON_WIND);
		};
	};
	if (GET_AF_BATTLE(ch, EAF_IRON_WIND))
	{
		(void) train_skill(ch, SKILL_IRON_WIND, skill_info[SKILL_IRON_WIND].max_percent, ch->get_fighting());
		if (weapon == RIGHT_WEAPON)
		{
			div = 100 + MIN(80, MAX(1, percent - 80));
			prob = 100;
		}
		else
		{
			div = MIN(80, percent + 10);
			prob = 80 - MIN(30, MAX(0, percent - 170));
		};
		while (div > 0)
		{
			if (number(1, 100) < div)
				hit(ch, ch->get_fighting(), type, weapon);
			div -= prob;
		};
	};

	hit(ch, ch->get_fighting(), type, weapon);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
