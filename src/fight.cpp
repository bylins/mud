/* ************************************************************************
*   File: fight.cpp                                     Part of Bylins    *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "fight.h"
#include "fight_local.hpp"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "im.h"
#include "skills.h"
#include "features.hpp"
#include "random.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "magic.h"
#include "room.hpp"
#include "genchar.h"
#include "sets_drop.hpp"
#include "olc.h"

// Structures
CHAR_DATA *combat_list = NULL;	// head of l-list of fighting chars
CHAR_DATA *next_combat_list = NULL;

extern CHAR_DATA *character_list;
extern vector < OBJ_DATA * >obj_proto;
extern int r_helled_start_room;
extern MobRaceListType mobraces_list;
extern CHAR_DATA *mob_proto;

// External procedures
// ACMD(do_assist);
void battle_affect_update(CHAR_DATA * ch);
void go_throw(CHAR_DATA * ch, CHAR_DATA * vict);
void go_bash(CHAR_DATA * ch, CHAR_DATA * vict);
void go_kick(CHAR_DATA * ch, CHAR_DATA * vict);
void go_rescue(CHAR_DATA * ch, CHAR_DATA * vict, CHAR_DATA * tmp_ch);
void go_parry(CHAR_DATA * ch);
void go_multyparry(CHAR_DATA * ch);
void go_block(CHAR_DATA * ch);
void go_touch(CHAR_DATA * ch, CHAR_DATA * vict);
void go_protect(CHAR_DATA * ch, CHAR_DATA * vict);
void go_chopoff(CHAR_DATA * ch, CHAR_DATA * vict);
void go_disarm(CHAR_DATA * ch, CHAR_DATA * vict);
int npc_battle_scavenge(CHAR_DATA * ch);
void npc_wield(CHAR_DATA * ch);
void npc_armor(CHAR_DATA * ch);
int perform_mob_switch(CHAR_DATA * ch);

/*
void go_autoassist(CHAR_DATA * ch)
{
	struct follow_type *k;
	CHAR_DATA *ch_lider = 0;
	if (ch->master)
	{
		ch_lider = ch->master;
	}
	else
		ch_lider = ch;	// Создаем ссылку на лидера

	buf2[0] = '\0';
	for (k = ch_lider->followers; k; k = k->next)
	{
		if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
				(IN_ROOM(k->follower) == IN_ROOM(ch)) && !k->follower->get_fighting() &&
				(GET_POS(k->follower) == POS_STANDING) && !CHECK_WAIT(k->follower))
			do_assist(k->follower, buf2, 0, 0);
	}
	if (PRF_FLAGGED(ch_lider, PRF_AUTOASSIST) &&
			(IN_ROOM(ch_lider) == IN_ROOM(ch)) && !ch_lider->get_fighting() &&
			(GET_POS(ch_lider) == POS_STANDING) && !CHECK_WAIT(ch_lider))
		do_assist(ch_lider, buf2, 0, 0);
}
*/

// The Fight related routines

// Добавил проверку на лаг, чтобы правильно работали Каменное проклятье и
// Круг пустоты, ибо позицию делают меньше чем поз_станнед.
void update_pos(CHAR_DATA * victim)
{
	if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
		GET_POS(victim) = GET_POS(victim);
	else if (GET_HIT(victim) > 0 && GET_WAIT(victim) <= 0 && !GET_MOB_HOLD(victim))
		GET_POS(victim) = POS_STANDING;
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = POS_DEAD;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = POS_MORTALLYW;
	else if (GET_HIT(victim) <= -3)
		GET_POS(victim) = POS_INCAP;
	else
		GET_POS(victim) = POS_STUNNED;

	if (AFF_FLAGGED(victim, AFF_SLEEP) && GET_POS(victim) != POS_SLEEPING)
		affect_from_char(victim, SPELL_SLEEP);

	if (on_horse(victim) && GET_POS(victim) < POS_FIGHTING)
		horse_drop(get_horse(victim));

	if (IS_HORSE(victim) && GET_POS(victim) < POS_FIGHTING && on_horse(victim->master))
		horse_drop(victim);
}

void set_battle_pos(CHAR_DATA * ch)
{
	switch (GET_POS(ch))
	{
	case POS_STANDING:
		GET_POS(ch) = POS_FIGHTING;
		break;
	case POS_RESTING:
	case POS_SITTING:
	case POS_SLEEPING:
		if (GET_WAIT(ch) <= 0 &&
				!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, AFF_SLEEP) && !AFF_FLAGGED(ch, AFF_CHARM))
		{
			if (IS_NPC(ch))
			{
				act("$n встал$g на ноги.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				GET_POS(ch) = POS_FIGHTING;
			}
			else if (GET_POS(ch) == POS_SLEEPING)
			{
				act("Вы проснулись и сели.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n проснул$u и сел$g.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				GET_POS(ch) = POS_SITTING;
			}
			else if (GET_POS(ch) == POS_RESTING)
			{
				act("Вы прекратили отдых и сели.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n прекратил$g отдых и сел$g.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				GET_POS(ch) = POS_SITTING;
			}
		}
		break;
	}
}

void restore_battle_pos(CHAR_DATA * ch)
{
	switch (GET_POS(ch))
	{
	case POS_FIGHTING:
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_RESTING:
	case POS_SITTING:
	case POS_SLEEPING:
		if (IS_NPC(ch) &&
				GET_WAIT(ch) <= 0 &&
				!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, AFF_SLEEP) && !AFF_FLAGGED(ch, AFF_CHARM))
		{
			act("$n встал$g на ноги.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			GET_POS(ch) = POS_STANDING;
		}
		break;
	}
	if (AFF_FLAGGED(ch, AFF_SLEEP))
		GET_POS(ch) = POS_SLEEPING;
}

// start one char fighting another (yes, it is horrible, I know... )
void set_fighting(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (ch == vict)
		return;

	if (ch->get_fighting())
	{
		log("SYSERR: set_fighting(%s->%s) when already fighting(%s)...",
			GET_NAME(ch), GET_NAME(vict), GET_NAME(ch->get_fighting()));
		// core_dump();
		return;
	}

	if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT)) || (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOFIGHT)))
		return;

	// if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
	//    return;

	if (AFF_FLAGGED(ch, AFF_BANDAGE))
	{
		send_to_char("Перевязка была прервана!\r\n", ch);
		affect_from_char(ch, SPELL_BANDAGE);
	}
	if (AFF_FLAGGED(ch, AFF_RECALL_SPELLS))
	{
		send_to_char("Вы забыли о концентрации и ринулись в бой!\r\n", ch);
		affect_from_char(ch, SPELL_RECALL_SPELLS);
	}

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, AFF_SLEEP))
		affect_from_char(ch, SPELL_SLEEP);

	ch->set_fighting(vict);

	NUL_AF_BATTLE(ch);
	//Polud вступление в битву не мешает прикрывать
	if (ch->get_protecting())
		SET_AF_BATTLE(ch, EAF_PROTECT);
	ch->set_touching(0);
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) = 0;
	ch->set_extra_attack(0, 0);
	set_battle_pos(ch);

	// если до начала боя на мобе есть лаг, то мы его выравниваем до целых
	// раундов в большую сторону (для подножки, должно давать чару зазор в две
	// секунды после подножки, чтобы моб всеравно встал только на 3й раунд)
	if (IS_NPC(ch) && GET_WAIT(ch) > 0)
	{
		div_t tmp = div(GET_WAIT(ch), PULSE_VIOLENCE);
		if (tmp.rem > 0)
		{
			WAIT_STATE(ch, (tmp.quot + 1) * PULSE_VIOLENCE);
		}
	}

	// Set combat style
	if (!AFF_FLAGGED(ch, AFF_COURAGE) && !AFF_FLAGGED(ch, AFF_DRUNKED) && !AFF_FLAGGED(ch, AFF_ABSTINENT))
	{
		if (PRF_FLAGGED(ch, PRF_PUNCTUAL))
			SET_AF_BATTLE(ch, EAF_PUNCTUAL);
		else if (PRF_FLAGGED(ch, PRF_AWAKE))
			SET_AF_BATTLE(ch, EAF_AWAKE);
	}

	if (can_use_feat(ch, DEFENDER_FEAT) && ch->get_skill(SKILL_BLOCK))
	{
		SET_AF_BATTLE(ch, EAF_AUTOBLOCK);
	}

	start_fight_mtrigger(ch, vict);
//  check_killer(ch, vict);
}

// remove a char from the list of fighting chars
void stop_fighting(CHAR_DATA * ch, int switch_others)
{
	CHAR_DATA *temp, *found;

	if (ch == next_combat_list)
		next_combat_list = ch->next_fighting;

	REMOVE_FROM_LIST(ch, combat_list, next_fighting);
	ch->next_fighting = NULL;
	if (ch->last_comm != NULL)
		free(ch->last_comm);
	ch->last_comm = NULL;
	ch->set_touching(0);
	ch->set_fighting(0);
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) = 0;
	ch->set_extra_attack(0, 0);
	ch->set_cast(0, 0, 0, 0, 0);
	restore_battle_pos(ch);
	NUL_AF_BATTLE(ch);
	DpsSystem::check_round(ch);
	StopFightParameters params(ch); //готовим параметры нужного типа и вызываем шаблонную функцию
	handle_affects(params);
	// sprintf(buf,"[Stop fighting] %s - %s\r\n",GET_NAME(ch),switch_others ? "switching" : "no switching");
	// send_to_gods(buf);
	//*** switch others ***

	if (switch_others != 2)
	{
		for (temp = combat_list; temp; temp = temp->next_fighting)
		{
			if (temp->get_touching() == ch)
			{
				temp->set_touching(0);
				CLR_AF_BATTLE(temp, EAF_TOUCH);
			}
			if (temp->get_extra_victim() == ch)
				temp->set_extra_attack(0, 0);
			if (temp->get_cast_char() == ch)
				temp->set_cast(0, 0, 0, 0, 0);
			if (temp->get_fighting() == ch && switch_others)
			{
				log("[Stop fighting] %s : Change victim for fighting", GET_NAME(temp));
				for (found = combat_list; found; found = found->next_fighting)
					if (found != ch && found->get_fighting() == temp)
					{
						act("Вы переключили свое внимание на $N3.", FALSE, temp, 0, found, TO_CHAR);
						temp->set_fighting(found);
						break;
					}
				if (!found)
					stop_fighting(temp, FALSE);
			}
		}

		update_pos(ch);

		// проверка скилла "железный ветер" - снимаем флаг по окончанию боя
		if ((ch->get_fighting() == NULL) && IS_SET(PRF_FLAGS(ch, PRF_IRON_WIND), PRF_IRON_WIND))
		{
			REMOVE_BIT(PRF_FLAGS(ch, PRF_IRON_WIND), PRF_IRON_WIND);
			if (GET_POS(ch) > POS_INCAP)
			{
				send_to_char("Безумие боя отпустило вас, и враз навалилась усталость...\r\n", ch);
				act("$n шумно выдохнул$g и остановил$u, переводя дух после боя.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			};
		};
	}
}

int GET_MAXDAMAGE(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_HOLD))
		return 0;
	else
		return GET_DAMAGE(ch);
}

int GET_MAXCASTER(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_HOLD) || AFF_FLAGGED(ch, AFF_SIELENCE) || AFF_FLAGGED(ch, AFF_STRANGLED)
			|| GET_WAIT(ch) > 0)
		return 0;
	else
		return IS_IMMORTAL(ch) ? 1 : GET_CASTER(ch);
}

#define GET_HP_PERC(ch) ((int)(GET_HIT(ch) * 100 / GET_MAX_HIT(ch)))
#define POOR_DAMAGE  15
#define POOR_CASTER  5
#define MAX_PROBES   0

int in_same_battle(CHAR_DATA * npc, CHAR_DATA * pc, int opponent)
{
	int ch_friend_npc, ch_friend_pc, vict_friend_npc, vict_friend_pc;
	CHAR_DATA *ch, *vict, *npc_master, *pc_master, *ch_master, *vict_master;

	if (npc == pc)
		return (!opponent);
	if (npc->get_fighting() == pc)	// NPC fight PC - opponent
		return (opponent);
	if (pc->get_fighting() == npc)	// PC fight NPC - opponent
		return (opponent);
	if (npc->get_fighting() && npc->get_fighting() == pc->get_fighting())
		return (!opponent);	// Fight same victim - friend
	if (AFF_FLAGGED(pc, AFF_HORSE) || AFF_FLAGGED(pc, AFF_CHARM))
		return (opponent);

	npc_master = npc->master ? npc->master : npc;
	pc_master = pc->master ? pc->master : pc;

	for (ch = world[IN_ROOM(npc)]->people; ch; ch = ch->next)
	{
		if (!ch->get_fighting())
			continue;
		ch_master = ch->master ? ch->master : ch;
		ch_friend_npc = (ch_master == npc_master) ||
						(IS_NPC(ch) && IS_NPC(npc) &&
						 !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
						 !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE));
		ch_friend_pc = (ch_master == pc_master) ||
					   (IS_NPC(ch) && IS_NPC(pc) &&
						!AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
						!AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE));
		if (ch->get_fighting() == pc && ch_friend_npc)	// Friend NPC fight PC - opponent
			return (opponent);
		if (pc->get_fighting() == ch && ch_friend_npc)	// PC fight friend NPC - opponent
			return (opponent);
		if (npc->get_fighting() == ch && ch_friend_pc)	// NPC fight friend PC - opponent
			return (opponent);
		if (ch->get_fighting() == npc && ch_friend_pc)	// Friend PC fight NPC - opponent
			return (opponent);
		vict = ch->get_fighting();
		vict_master = vict->master ? vict->master : vict;
		vict_friend_npc = (vict_master == npc_master) ||
						  (IS_NPC(vict) && IS_NPC(npc) &&
						   !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
						   !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE));
		vict_friend_pc = (vict_master == pc_master) ||
						 (IS_NPC(vict) && IS_NPC(pc) &&
						  !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
						  !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE));
		if (ch_friend_npc && vict_friend_pc)
			return (opponent);	// Friend NPC fight friend PC - opponent
		if (ch_friend_pc && vict_friend_npc)
			return (opponent);	// Friend PC fight friend NPC - opponent
	}

	return (!opponent);
}

CHAR_DATA *find_friend_cure(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, AFF_USED = 0;
	switch (spellnum)
	{
	case SPELL_CURE_LIGHT:
		AFF_USED = 80;
		break;
	case SPELL_CURE_SERIOUS:
		AFF_USED = 70;
		break;
	case SPELL_EXTRA_HITS:
	case SPELL_CURE_CRITIC:
		AFF_USED = 50;
		break;
	case SPELL_HEAL:
	case SPELL_GROUP_HEAL:
		AFF_USED = 30;
		break;
	}

	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL))
			&& AFF_FLAGGED(caster, AFF_HELPER))
	{
		if (GET_HP_PERC(caster) < AFF_USED)
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
				 CAN_SEE(caster, caster->master) &&
				 IN_ROOM(caster->master) == IN_ROOM(caster) &&
				 caster->master->get_fighting() && GET_HP_PERC(caster->master) < AFF_USED)
			return (caster->master);
		return (NULL);
	}

	for (vict = world[IN_ROOM(caster)]->people; AFF_USED && vict; vict = vict->next_in_room)
	{
		if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
				&& (vict->master && !IS_NPC(vict->master)))
				|| !CAN_SEE(caster, vict))
			continue;
		if (!vict->get_fighting() && !MOB_FLAGGED(vict, MOB_HELPER))
			continue;
		if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict)))
		{
			victim = vict;
			vict_val = GET_HP_PERC(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
				break;
		}
	}
	return (victim);
}

CHAR_DATA *find_friend(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, AFF_USED = 0, spellreal = -1;
	switch (spellnum)
	{
	case SPELL_CURE_BLIND:
		SET_BIT(AFF_USED, AFF_BLIND);
		break;
	case SPELL_REMOVE_POISON:
		SET_BIT(AFF_USED, ((AFF_POISON) | (AFF_SCOPOLIA_POISON) | (AFF_BELENA_POISON) | (AFF_DATURA_POISON)));
		break;
	case SPELL_REMOVE_HOLD:
		SET_BIT(AFF_USED, AFF_HOLD);
		break;
	case SPELL_REMOVE_CURSE:
		SET_BIT(AFF_USED, AFF_CURSE);
		break;
	case SPELL_REMOVE_SIELENCE:
		SET_BIT(AFF_USED, AFF_SIELENCE);
		break;
	case SPELL_CURE_PLAQUE:
		spellreal = SPELL_PLAQUE;
		break;

	}
	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)) && AFF_FLAGGED(caster, AFF_HELPER))
	{
		if (AFF_FLAGGED(caster, AFF_USED) || affected_by_spell(caster, spellreal))
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
				 CAN_SEE(caster, caster->master) && IN_ROOM(caster->master) == IN_ROOM(caster) &&
				 (AFF_FLAGGED(caster->master, AFF_USED) || affected_by_spell(caster->master, spellreal)))
			return (caster->master);
		return (NULL);
	}

	for (vict = world[IN_ROOM(caster)]->people; AFF_USED && vict; vict = vict->next_in_room)
	{
		if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
				&& (vict->master && !IS_NPC(vict->master)))
				|| !CAN_SEE(caster, vict))
			continue;
		if (!AFF_FLAGGED(vict, AFF_USED))
			continue;
		if (!vict->get_fighting() && !MOB_FLAGGED(vict, MOB_HELPER))
			continue;
		if (!victim || vict_val < GET_MAXDAMAGE(vict))
		{
			victim = vict;
			vict_val = GET_MAXDAMAGE(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
				break;
		}
	}
	return (victim);
}

CHAR_DATA *find_caster(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict = NULL, *victim = NULL;
	int vict_val = 0, AFF_USED, spellreal = -1;
	AFF_USED = 0;
	switch (spellnum)
	{
	case SPELL_CURE_BLIND:
		SET_BIT(AFF_USED, AFF_BLIND);
		break;
	case SPELL_REMOVE_POISON:
		SET_BIT(AFF_USED, ((AFF_POISON) | (AFF_SCOPOLIA_POISON) | (AFF_BELENA_POISON) | (AFF_DATURA_POISON)));
		break;
	case SPELL_REMOVE_HOLD:
		SET_BIT(AFF_USED, AFF_HOLD);
		break;
	case SPELL_REMOVE_CURSE:
		SET_BIT(AFF_USED, AFF_CURSE);
		break;
	case SPELL_REMOVE_SIELENCE:
		SET_BIT(AFF_USED, AFF_SIELENCE);
		break;
	case SPELL_CURE_PLAQUE:
		spellreal = SPELL_PLAQUE;
		break;
	}

	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)) && AFF_FLAGGED(caster, AFF_HELPER))
	{
		if (AFF_FLAGGED(caster, AFF_USED) || affected_by_spell(caster, spellreal))
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
				 CAN_SEE(caster, caster->master) && IN_ROOM(caster->master) == IN_ROOM(caster) &&
				 (AFF_FLAGGED(caster->master, AFF_USED) || affected_by_spell(caster->master, spellreal)))
			return (caster->master);
		return (NULL);
	}

	for (vict = world[IN_ROOM(caster)]->people; AFF_USED && vict; vict = vict->next_in_room)
	{
		if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
				&& (vict->master && !IS_NPC(vict->master)))
				|| !CAN_SEE(caster, vict))
			continue;
		if (!AFF_FLAGGED(vict, AFF_USED))
			continue;
		if (!vict->get_fighting() && !MOB_FLAGGED(vict, MOB_HELPER))
			continue;
		if (!victim || vict_val < GET_MAXCASTER(vict))
		{
			victim = vict;
			vict_val = GET_MAXCASTER(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
				break;
		}
	}
	return (victim);
}


CHAR_DATA *find_affectee(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == SPELL_GROUP_ARMOR)
		spellreal = SPELL_ARMOR;
	else if (spellreal == SPELL_GROUP_STRENGTH)
		spellreal = SPELL_STRENGTH;
	else if (spellreal == SPELL_GROUP_BLESS)
		spellreal = SPELL_BLESS;
	else if (spellreal == SPELL_GROUP_HASTE)
		spellreal = SPELL_HASTE;
	else if (spellreal == SPELL_GROUP_SANCTUARY)
		spellreal = SPELL_SANCTUARY;
	else if (spellreal == SPELL_GROUP_PRISMATICAURA)
		spellreal = SPELL_PRISMATICAURA;

	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)) && AFF_FLAGGED(caster, AFF_HELPER))
	{
		if (!affected_by_spell(caster, spellreal))
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
				 CAN_SEE(caster, caster->master) &&
				 IN_ROOM(caster->master) == IN_ROOM(caster) &&
				 caster->master->get_fighting() && !affected_by_spell(caster->master, spellreal))
			return (caster->master);
		return (NULL);
	}

	if (GET_REAL_INT(caster) > number(5, 15))
		for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room)
		{
			if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
					&& (vict->master
						&& !IS_NPC(vict->master)))
					|| !CAN_SEE(caster, vict))
				continue;
			if (!vict->get_fighting() || AFF_FLAGGED(vict, AFF_HOLD) || affected_by_spell(vict, spellreal))
				continue;
			if (!victim || vict_val < GET_MAXDAMAGE(vict))
			{
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	if (!victim && !affected_by_spell(caster, spellreal))
		victim = caster;

	return (victim);
}

CHAR_DATA *find_opp_affectee(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == SPELL_POWER_HOLD || spellreal == SPELL_MASS_HOLD)
		spellreal = SPELL_HOLD;
	else if (spellreal == SPELL_POWER_BLINDNESS || spellreal == SPELL_MASS_BLINDNESS)
		spellreal = SPELL_BLINDNESS;
	else if (spellreal == SPELL_POWER_SIELENCE || spellreal == SPELL_MASS_SIELENCE)
		spellreal = SPELL_SIELENCE;
	else if (spellreal == SPELL_MASS_CURSE)
		spellreal = SPELL_CURSE;
	else if (spellreal == SPELL_MASS_SLOW)
		spellreal = SPELL_SLOW;

	if (GET_REAL_INT(caster) > number(10, 20))
		for (vict = world[caster->in_room]->people; vict; vict = vict->next_in_room)
		{
			if ((IS_NPC(vict) && !((MOB_FLAGGED(vict, MOB_ANGEL)
									|| AFF_FLAGGED(vict, AFF_CHARM)) && (vict->master
																		 && !IS_NPC(vict->master))))
					|| !CAN_SEE(caster, vict))
				continue;
			if ((!vict->get_fighting()
					&& (GET_REAL_INT(caster) < number(20, 27)
						|| !in_same_battle(caster, vict, TRUE)))
					|| AFF_FLAGGED(vict, AFF_HOLD)
					|| affected_by_spell(vict, spellreal))
				continue;
			if (!victim || vict_val < GET_MAXDAMAGE(vict))
			{
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}

	if (!victim && caster->get_fighting()
			&& !affected_by_spell(caster->get_fighting(), spellreal))
		victim = caster->get_fighting();
	return (victim);
}

CHAR_DATA *find_opp_caster(CHAR_DATA * caster)
{
	CHAR_DATA *vict = NULL, *victim = NULL;
	int vict_val = 0;

	for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room)
	{
		if (IS_NPC(vict) &&
//         !AFF_FLAGGED(vict,AFF_CHARM) &&
				!(MOB_FLAGGED(vict, MOB_ANGEL)
				  && (vict->master && !IS_NPC(vict->master))))
			continue;
		if ((!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(15, 25)
					|| !in_same_battle(caster, vict, TRUE)))
				|| AFF_FLAGGED(vict, AFF_HOLD) || AFF_FLAGGED(vict, AFF_SIELENCE) || AFF_FLAGGED(vict, AFF_STRANGLED)
				|| (!CAN_SEE(caster, vict) && caster->get_fighting() != vict))
			continue;
		if (vict_val < GET_MAXCASTER(vict))
		{
			victim = vict;
			vict_val = GET_MAXCASTER(vict);
		}
	}
	return (victim);
}

CHAR_DATA *find_damagee(CHAR_DATA * caster)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20))
		for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room)
		{
			if ((IS_NPC(vict) && !((MOB_FLAGGED(vict, MOB_ANGEL)
									|| AFF_FLAGGED(vict, AFF_CHARM)) && (vict->master
																		 && !IS_NPC(vict->master))))
					|| !CAN_SEE(caster, vict))
				continue;
			if ((!vict->get_fighting()
					&& (GET_REAL_INT(caster) < number(20, 27)
						|| !in_same_battle(caster, vict, TRUE)))
					|| AFF_FLAGGED(vict, AFF_HOLD))
				continue;
			if (GET_REAL_INT(caster) >= number(25, 30))
			{
				if (!victim || vict_val < GET_MAXCASTER(vict))
				{
					victim = vict;
					vict_val = GET_MAXCASTER(vict);
				}
			}
			else if (!victim || vict_val < GET_MAXDAMAGE(vict))
			{
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	if (!victim)
		victim = caster->get_fighting();

	return (victim);
}

CHAR_DATA *find_minhp(CHAR_DATA * caster)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20))
		for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room)
		{
			if ((IS_NPC(vict) && !((MOB_FLAGGED(vict, MOB_ANGEL)
									|| AFF_FLAGGED(vict, AFF_CHARM)) && (vict->master
																		 && !IS_NPC(vict->master))))
					|| !CAN_SEE(caster, vict))
				continue;
			if (!vict->get_fighting() && (GET_REAL_INT(caster) < number(20, 27)
									|| !in_same_battle(caster, vict, TRUE)))
				continue;
			if (!victim || vict_val > GET_HIT(vict))
			{
				victim = vict;
				vict_val = GET_HIT(vict);
			}
		}
	if (!victim)
		victim = caster->get_fighting();

	return (victim);
}

CHAR_DATA *find_cure(CHAR_DATA * caster, CHAR_DATA * patient, int *spellnum)
{
	if (GET_HP_PERC(patient) <= number(20, 33))
	{
		if (GET_SPELL_MEM(caster, SPELL_EXTRA_HITS))
			*spellnum = SPELL_EXTRA_HITS;
		else if (GET_SPELL_MEM(caster, SPELL_HEAL))
			*spellnum = SPELL_HEAL;
		else if (GET_SPELL_MEM(caster, SPELL_CURE_CRITIC))
			*spellnum = SPELL_CURE_CRITIC;
		else if (GET_SPELL_MEM(caster, SPELL_GROUP_HEAL))
			*spellnum = SPELL_GROUP_HEAL;
	}
	else if (GET_HP_PERC(patient) <= number(50, 65))
	{
		if (GET_SPELL_MEM(caster, SPELL_CURE_CRITIC))
			*spellnum = SPELL_CURE_CRITIC;
		else if (GET_SPELL_MEM(caster, SPELL_CURE_SERIOUS))
			*spellnum = SPELL_CURE_SERIOUS;
		else if (GET_SPELL_MEM(caster, SPELL_CURE_LIGHT))
			*spellnum = SPELL_CURE_LIGHT;
	}
	if (*spellnum)
		return (patient);
	else
		return (NULL);
}

void mob_casting(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	int battle_spells[MAX_STRING_LENGTH];
	int lag = GET_WAIT(ch), i, spellnum, spells, sp_num;
	OBJ_DATA *item;

	if (AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HOLD) || AFF_FLAGGED(ch, AFF_SIELENCE) || AFF_FLAGGED(ch, AFF_STRANGLED) || lag > 0)
		return;

	memset(&battle_spells, 0, sizeof(battle_spells));
	for (i = 1, spells = 0; i <= MAX_SPELLS; i++)
		if (GET_SPELL_MEM(ch, i) && IS_SET(spell_info[i].routines, NPC_CALCULATE))
			battle_spells[spells++] = i;

	for (item = ch->carrying;
			spells < MAX_STRING_LENGTH &&
			item &&
			GET_RACE(ch) == NPC_RACE_HUMAN &&
			!MOB_FLAGGED(ch, MOB_ANGEL) && !AFF_FLAGGED(ch, AFF_CHARM); item = item->next_content)
		switch (GET_OBJ_TYPE(item))
		{
		case ITEM_WAND:
		case ITEM_STAFF:
			if (GET_OBJ_VAL(item, 2) > 0 &&
					IS_SET(spell_info[GET_OBJ_VAL(item, 3)].routines, NPC_CALCULATE))
				battle_spells[spells++] = GET_OBJ_VAL(item, 3);
			break;
		case ITEM_POTION:
			for (i = 1; i <= 3; i++)
				if (IS_SET
						(spell_info[GET_OBJ_VAL(item, i)].routines,
						 NPC_AFFECT_NPC | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER))
					battle_spells[spells++] = GET_OBJ_VAL(item, i);
			break;
		case ITEM_SCROLL:
			for (i = 1; i <= 3; i++)
				if (IS_SET(spell_info[GET_OBJ_VAL(item, i)].routines, NPC_CALCULATE))
					battle_spells[spells++] = GET_OBJ_VAL(item, i);
			break;
		}

	// перво-наперво  -  лечим себя
	spellnum = 0;
	victim = find_cure(ch, ch, &spellnum);
	// Ищем рандомную заклинашку и цель для нее
	for (i = 0; !victim && spells && i < GET_REAL_INT(ch) / 5; i++)
		if (!spellnum && (spellnum = battle_spells[(sp_num = number(0, spells - 1))])
				&& spellnum > 0 && spellnum <= MAX_SPELLS)  	// sprintf(buf,"$n using spell '%s', %d from %d",
		{
			//         spell_name(spellnum), sp_num, spells);
			// act(buf,FALSE,ch,0,ch->get_fighting(),TO_VICT);
			if (spell_info[spellnum].routines & NPC_DAMAGE_PC_MINHP)
			{
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_minhp(ch);
			}
			else if (spell_info[spellnum].routines & NPC_DAMAGE_PC)
			{
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_damagee(ch);
			}
			else if (spell_info[spellnum].routines & NPC_AFFECT_PC_CASTER)
			{
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_opp_caster(ch);
			}
			else if (spell_info[spellnum].routines & NPC_AFFECT_PC)
			{
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_opp_affectee(ch, spellnum);
			}
			else if (spell_info[spellnum].routines & NPC_AFFECT_NPC)
				victim = find_affectee(ch, spellnum);
			else if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC_CASTER)
				victim = find_caster(ch, spellnum);
			else if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC)
				victim = find_friend(ch, spellnum);
			else if (spell_info[spellnum].routines & NPC_DUMMY)
				victim = find_friend_cure(ch, spellnum);
			else
				spellnum = 0;
		}
	if (spellnum && victim)  	// Is this object spell ?
	{
		for (item = ch->carrying;
				!AFF_FLAGGED(ch, AFF_CHARM) &&
				!MOB_FLAGGED(ch, MOB_ANGEL) && item && GET_RACE(ch) == NPC_RACE_HUMAN; item = item->next_content)
			switch (GET_OBJ_TYPE(item))
			{
			case ITEM_WAND:
			case ITEM_STAFF:
				if (GET_OBJ_VAL(item, 2) > 0 && GET_OBJ_VAL(item, 3) == spellnum)
				{
					mag_objectmagic(ch, item, GET_NAME(victim));
					return;
				}
				break;
			case ITEM_POTION:
				for (i = 1; i <= 3; i++)
					if (GET_OBJ_VAL(item, i) == spellnum)
					{
						if (ch != victim)
						{
							obj_from_char(item);
							act("$n передал$g $o3 $N2.", FALSE, ch, item, victim, TO_ROOM | TO_ARENA_LISTEN);
							obj_to_char(item, victim);
						}
						else
							victim = ch;
						mag_objectmagic(victim, item, GET_NAME(victim));
						return;
					}
				break;
			case ITEM_SCROLL:
				for (i = 1; i <= 3; i++)
					if (GET_OBJ_VAL(item, i) == spellnum)
					{
						mag_objectmagic(ch, item, GET_NAME(victim));
						return;
					}
				break;
			}

		cast_spell(ch, victim, 0, NULL, spellnum, spellnum);
	}
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HELPER)) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)

#define	MAY_ACT(ch)	(!(AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT) || GET_MOB_HOLD(ch) || GET_WAIT(ch)))

void summon_mob_helpers(CHAR_DATA *ch)
{
	for (struct helper_data_type *helpee = GET_HELPER(ch);
		helpee; helpee = helpee->next_helper)
	{
		for (CHAR_DATA *vict = character_list; vict; vict = vict->next)
		{
			if (!IS_NPC(vict)
				|| GET_MOB_VNUM(vict) != helpee->mob_vnum
				|| AFF_FLAGGED(ch, AFF_CHARM)
				|| AFF_FLAGGED(vict, AFF_HOLD)
				|| AFF_FLAGGED(vict, AFF_CHARM)
				|| AFF_FLAGGED(vict, AFF_BLIND)
				|| GET_WAIT(vict) > 0
				|| GET_POS(vict) < POS_STANDING
				|| IN_ROOM(vict) == NOWHERE
				|| vict->get_fighting())
			{
				continue;
			}
			if (GET_RACE(ch) == NPC_RACE_HUMAN)
			{
				act("$n воззвал$g : \"На помощь, мои верные соратники!\"",
					FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			}
			if (IN_ROOM(vict) != IN_ROOM(ch))
			{
				char_from_room(vict);
				char_to_room(vict, IN_ROOM(ch));
				act("$n прибыл$g на зов и вступил$g в битву на стороне $N1.",
					FALSE, vict, 0, ch, TO_ROOM | TO_ARENA_LISTEN);
			}
			else
			{
				act("$n вступил$g в битву на стороне $N1.",
					FALSE, vict, 0, ch, TO_ROOM | TO_ARENA_LISTEN);
			}
			set_fighting(vict, ch->get_fighting());
		}
	}
}

void check_mob_helpers()
{
	for (CHAR_DATA *ch = combat_list; ch; ch = next_combat_list)
	{
		next_combat_list = ch->next_fighting;
		// Extract battler if no opponent
		if (ch->get_fighting() == NULL
			|| IN_ROOM(ch) != IN_ROOM(ch->get_fighting())
			|| IN_ROOM(ch) == NOWHERE)
		{
			stop_fighting(ch, TRUE);
			continue;
		}
		if (GET_MOB_HOLD(ch)
			|| !IS_NPC(ch)
			|| GET_WAIT(ch) > 0
			|| GET_POS(ch) < POS_FIGHTING
			|| AFF_FLAGGED(ch, AFF_CHARM)
			|| AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)
			|| AFF_FLAGGED(ch, AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, AFF_SIELENCE)
			|| AFF_FLAGGED(ch, AFF_STRANGLED)
			|| PRF_FLAGGED(ch->get_fighting(), PRF_NOHASSLE))
		{
			continue;
		}
		summon_mob_helpers(ch);
	}
}

void try_angel_rescue(CHAR_DATA *ch)
{
	struct follow_type *k, *k_next;

	for (k = ch->followers; k; k = k_next)
	{
		k_next = k->next;
		if (AFF_FLAGGED(k->follower, AFF_HELPER)
			&& MOB_FLAGGED(k->follower, MOB_ANGEL)
			&& !k->follower->get_fighting()
			&& IN_ROOM(k->follower) == IN_ROOM(ch)
			&& CAN_SEE(k->follower, ch)
			&& AWAKE(k->follower)
			&& MAY_ACT(k->follower)
			&& GET_POS(k->follower) >= POS_FIGHTING)
		{
			CHAR_DATA *vict;
			for (vict = world[IN_ROOM(ch)]->people;
				vict; vict = vict->next_in_room)
			{
				if (vict->get_fighting() == ch
					&& vict != ch
					&& vict != k->follower)
				{
					break;
				}
			}
			if (vict && k->follower->get_skill(SKILL_RESCUE))
			{
				go_rescue(k->follower, ch, vict);
			}
		}
	}
}

void stand_up_or_sit(CHAR_DATA *ch)
{
	if (IS_NPC(ch))
	{
		act("$n встал$g на ноги.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_FIGHTING;
	}
	else if (GET_POS(ch) == POS_SLEEPING)
	{
		act("Вы проснулись и сели.", TRUE, ch, 0, 0, TO_CHAR);
		act("$n проснул$u и сел$g.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
	}
	else if (GET_POS(ch) == POS_RESTING)
	{
		act("Вы прекратили отдых и сели.", TRUE, ch, 0, 0, TO_CHAR);
		act("$n прекратил$g отдых и сел$g.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
	}
}

void set_mob_skills_flags(CHAR_DATA *ch)
{
	bool sk_use = false;
	// 1) parry
	int do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_PARRY))
	{
		SET_AF_BATTLE(ch, EAF_PARRY);
		sk_use = true;
	}
	// 2) blocking
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_BLOCK))
	{
		SET_AF_BATTLE(ch, EAF_BLOCK);
		sk_use = true;
	}
	// 3) multyparry
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_MULTYPARRY))
	{
		SET_AF_BATTLE(ch, EAF_MULTYPARRY);
		sk_use = true;
	}
	// 4) deviate
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_DEVIATE))
	{
		SET_AF_BATTLE(ch, EAF_DEVIATE);
		sk_use = true;
	}
	// 5) mighthit
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_MIGHTHIT)
		&& check_mighthit_weapon(ch))
	{
		SET_AF_BATTLE(ch, EAF_MIGHTHIT);
		sk_use = true;
	}
	// 6) stupor
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_STUPOR))
	{
		SET_AF_BATTLE(ch, EAF_STUPOR);
		sk_use = true;
	}
	// 7) styles
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_AWAKE) > number(1, 101))
	{
		SET_AF_BATTLE(ch, EAF_AWAKE);
	}
	else
	{
		CLR_AF_BATTLE(ch, EAF_AWAKE);
	}
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_PUNCTUAL) > number(1, 101))
	{
		SET_AF_BATTLE(ch, EAF_PUNCTUAL);
	}
	else
	{
		CLR_AF_BATTLE(ch, EAF_PUNCTUAL);
	}
}

int calc_initiative(CHAR_DATA *ch)
{
	int initiative = size_app[GET_POS_SIZE(ch)].initiative;

	int i = number(1, 10);
	if (i == 10)
		initiative -= 1;
	else
		initiative += i;

	initiative += GET_INITIATIVE(ch);

	if (!IS_NPC(ch))
	{
		switch (IS_CARRYING_W(ch) * 10 / MAX(1, CAN_CARRY_W(ch)))
		{
		case 10:
		case 9:
		case 8:
			initiative -= 2;
			break;
		case 7:
		case 6:
		case 5:
			initiative -= 1;
			break;
		}
	}

	if (GET_AF_BATTLE(ch, EAF_AWAKE))
		initiative -= 2;
	if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
		initiative -= 1;
	if (AFF_FLAGGED(ch, AFF_SLOW))
		initiative -= 10;
	if (AFF_FLAGGED(ch, AFF_HASTE))
		initiative += 10;
	if (GET_WAIT(ch) > 0)
		initiative -= 1;
	if (calc_leadership(ch))
		initiative += 5;
	if (GET_AF_BATTLE(ch, EAF_SLOW))
		initiative = 1;

	initiative = MAX(initiative, 1);

	return initiative;
}

void using_mob_skills(CHAR_DATA *ch)
{
	for (int sk_num = 0, sk_use = GET_REAL_INT(ch);
		MAY_LIKES(ch) && sk_use > 0; sk_use--)
	{
		int do_this = number(0, 100);
		if (do_this > GET_LIKES(ch))
			continue;

		do_this = number(0, 100);
		if (do_this < 10)
			sk_num = SKILL_BASH;
		else if (do_this < 20)
			sk_num = SKILL_DISARM;
		else if (do_this < 30)
			sk_num = SKILL_KICK;
		else if (do_this < 40)
			sk_num = SKILL_PROTECT;
		else if (do_this < 50)
			sk_num = SKILL_RESCUE;
		else if (do_this < 60 && !ch->get_touching())
			sk_num = SKILL_TOUCH;
		else if (do_this < 70)
			sk_num = SKILL_CHOPOFF;
		else if (do_this < 80)
			sk_num = SKILL_THROW;
		else
			sk_num = SKILL_BASH;
		if (ch->get_skill(sk_num) <= 0)
			sk_num = 0;
		if (!sk_num)
			continue;

		////////////////////////////////////////////////////////////////////////
		if (sk_num == SKILL_TOUCH)
		{
			sk_use = 0;
			go_touch(ch, ch->get_fighting());
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == SKILL_THROW)
		{
			sk_use = 0;
			int i = 0;
			// Цель выбираем по рандому
			for (CHAR_DATA *vict = world[IN_ROOM(ch)]->people;
				vict; vict = vict->next_in_room)
			{
				if (!IS_NPC(vict))
				{
					i++;
				}
			}
			CHAR_DATA *caster = 0;
			if (i > 0)
			{
				i = number(1, i);
				for (CHAR_DATA *vict = world[IN_ROOM(ch)]->people;
					i; vict = vict->next_in_room)
				{
					if (!IS_NPC(vict))
					{
						i--;
						caster = vict;
					}
				}
			}
			// Метаем
			if (caster)
			{
				go_throw(ch, caster);
			}
		}

		////////////////////////////////////////////////////////////////////////
		// проверим на всякий случай, является ли моб ангелом,
		// хотя вроде бы этого делать не надо
		if (!(MOB_FLAGGED(ch, MOB_ANGEL) && ch->master)
			&& (sk_num == SKILL_RESCUE || sk_num == SKILL_PROTECT))
		{
			CHAR_DATA *caster = 0, *damager = 0;
			int dumb_mob = (int)(GET_REAL_INT(ch) < number(5, 20));

			for (CHAR_DATA *attacker = world[IN_ROOM(ch)]->people;
					attacker; attacker = attacker->next_in_room)
			{
				CHAR_DATA *vict = attacker->get_fighting();	// выяснение жертвы
				if (!vict	// жертвы нет
					|| (!IS_NPC(vict) // жертва - не моб
						|| AFF_FLAGGED(vict, AFF_CHARM)
						|| AFF_FLAGGED(vict, AFF_HELPER))
					|| (IS_NPC(attacker)
						&& !(AFF_FLAGGED(attacker, AFF_CHARM)
							&& attacker->master
							&& !IS_NPC(attacker->master))
						&& !(MOB_FLAGGED(attacker, MOB_ANGEL)
							&& attacker->master
							&& !IS_NPC(attacker->master)))
							// не совсем понятно, зачем это было
							// && !AFF_FLAGGED(attacker,AFF_HELPER)
							// свои атакуют (мобы)
					|| !CAN_SEE(ch, vict) // не видно, кого нужно спасать
					|| ch == vict) // себя спасать не нужно
				{
					continue;
				}

				// Буду спасать vict от attacker
				if (!caster	// еще пока никого не спасаю
					|| (GET_HIT(vict) < GET_HIT(caster)))	// этому мобу хуже
				{
					caster = vict;
					damager = attacker;
					if (dumb_mob)
					{
						break;	// тупой моб спасает первого
					}
				}
			}
			if (sk_num == SKILL_RESCUE && caster && damager)
			{
				sk_use = 0;
				go_rescue(ch, caster, damager);
			}
			if (sk_num == SKILL_PROTECT && caster)
			{
				sk_use = 0;
				go_protect(ch, caster);
			}
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == SKILL_BASH
			|| sk_num == SKILL_CHOPOFF
			|| sk_num == SKILL_DISARM)
		{
			CHAR_DATA *caster = 0, *damager = 0;

			if (GET_REAL_INT(ch) < number(15, 25))
			{
				caster = ch->get_fighting();
				damager = caster;
			}
			else
			{
				for (CHAR_DATA *vict = world[IN_ROOM(ch)]->people; vict;
						vict = vict->next_in_room)
				{
					if ((IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM))
						|| !vict->get_fighting())
					{
						continue;
					}
					if ((AFF_FLAGGED(vict, AFF_HOLD) && GET_POS(vict) < POS_FIGHTING)
						|| (IS_CASTER(vict)
							&& (AFF_FLAGGED(vict, AFF_HOLD)
							|| AFF_FLAGGED(vict, AFF_SIELENCE)
							|| AFF_FLAGGED(vict, AFF_STRANGLED)
							|| GET_WAIT(vict) > 0)))
					{
						continue;
					}
					if (!caster
						|| (IS_CASTER(vict) && GET_CASTER(vict) > GET_CASTER(caster)))
					{
						caster = vict;
					}
					if (!damager || GET_DAMAGE(vict) > GET_DAMAGE(damager))
					{
						damager = vict;
					}
				}
			}

			if (caster
				&& (CAN_SEE(ch, caster) || ch->get_fighting() == caster)
				&& GET_CASTER(caster) > POOR_CASTER
				&& (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF))
			{
				if (sk_num == SKILL_BASH)
				{
					if (GET_POS(caster) >= POS_FIGHTING
						|| calculate_skill(ch, SKILL_BASH, 200, caster) > number(50, 80))
					{
						sk_use = 0;
						go_bash(ch, caster);
					}
				}
				else
				{
					if (GET_POS(caster) >= POS_FIGHTING
						|| calculate_skill(ch, SKILL_CHOPOFF, 200, caster) > number(50, 80))
					{
						sk_use = 0;
						go_chopoff(ch, caster);
					}
				}
			}

			if (sk_use
				&& damager
				&& (CAN_SEE(ch, damager)
				|| ch->get_fighting() == damager))
			{
				if (sk_num == SKILL_BASH)
				{
					if (on_horse(damager))
					{
						// Карачун. Правка бага. Лошадь не должна башить себя, если дерется с наездником.
						if (get_horse(damager) == ch)
						{
							horse_drop(ch);
						}
						else
						{
							sk_use = 0;
							go_bash(ch, get_horse(damager));
						}
					}
					else if (GET_POS(damager) >= POS_FIGHTING
						|| calculate_skill(ch, SKILL_BASH, 200, damager) > number(50, 80))
					{
						sk_use = 0;
						go_bash(ch, damager);
					}
				}
				else if (sk_num == SKILL_CHOPOFF)
				{
					if (on_horse(damager))
					{
						sk_use = 0;
						go_chopoff(ch, get_horse(damager));
					}
					else if (GET_POS(damager) >= POS_FIGHTING
						|| calculate_skill(ch, SKILL_CHOPOFF, 200, damager) > number(50, 80))
					{
						sk_use = 0;
						go_chopoff(ch, damager);
					}
				}
				else if (sk_num == SKILL_DISARM
					&& (GET_EQ(damager, WEAR_WIELD)
						|| GET_EQ(damager, WEAR_BOTHS)
						|| (GET_EQ(damager, WEAR_HOLD))))
				{
					sk_use = 0;
					go_disarm(ch, damager);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == SKILL_KICK && !on_horse(ch->get_fighting()))
		{
			sk_use = 0;
			go_kick(ch, ch->get_fighting());
		}
	}
}

void add_attackers_round(CHAR_DATA *ch)
{
	for (CHAR_DATA *i = world[ch->in_room]->people;
		i; i = i->next_in_room)
	{
		if (!IS_NPC(i) && i->desc)
		{
			ch->add_attacker(i, ATTACKER_ROUNDS, 1);
		}
	}
}

void update_round_affs()
{
	for (CHAR_DATA *ch = combat_list; ch; ch = ch->next_fighting)
	{
		if (IN_ROOM(ch) == NOWHERE)
			continue;

		CLR_AF_BATTLE(ch, EAF_FIRST);
		CLR_AF_BATTLE(ch, EAF_SECOND);
		CLR_AF_BATTLE(ch, EAF_USEDLEFT);
		CLR_AF_BATTLE(ch, EAF_USEDRIGHT);
		CLR_AF_BATTLE(ch, EAF_MULTYPARRY);

		if (GET_AF_BATTLE(ch, EAF_SLEEP))
			affect_from_char(ch, SPELL_SLEEP);

		if (GET_AF_BATTLE(ch, EAF_BLOCK))
		{
			CLR_AF_BATTLE(ch, EAF_BLOCK);
			if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}

		if (GET_AF_BATTLE(ch, EAF_DEVIATE))
		{
			CLR_AF_BATTLE(ch, EAF_DEVIATE);
			if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}

		if (GET_AF_BATTLE(ch, EAF_POISONED))
		{
			CLR_AF_BATTLE(ch, EAF_POISONED);
		}

		battle_affect_update(ch);

		if (IS_NPC(ch) && !IS_CHARMICE(ch))
		{
			add_attackers_round(ch);
		}
	}
}

bool using_extra_attack(CHAR_DATA *ch)
{
	bool used = false;

	switch (ch->get_extra_skill())
	{
	case SKILL_THROW:
		go_throw(ch, ch->get_extra_victim());
		used = true;
		break;
	case SKILL_BASH:
		go_bash(ch, ch->get_extra_victim());
		used = true;
		break;
	case SKILL_KICK:
		go_kick(ch, ch->get_extra_victim());
		used = true;
		break;
	case SKILL_CHOPOFF:
		go_chopoff(ch, ch->get_extra_victim());
		used = true;
		break;
	case SKILL_DISARM:
		go_disarm(ch, ch->get_extra_victim());
		used = true;
		break;
	}

	return used;
}

void process_npc_attack(CHAR_DATA *ch)
{
	// if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
	//    continue;

	// Вызываем триггер перед началом боевых моба (магических или физических)
	fight_mtrigger(ch);

	// переключение
	if (MAY_LIKES(ch)
		&& !AFF_FLAGGED(ch, AFF_CHARM)
		&& !AFF_FLAGGED(ch, AFF_NOT_SWITCH)
		&& GET_REAL_INT(ch) > number(15, 25))
	{
		perform_mob_switch(ch);
	}

	// Cast spells
	if (MAY_LIKES(ch))
		mob_casting(ch);

	if (!ch->get_fighting()
		|| IN_ROOM(ch) != IN_ROOM(ch->get_fighting())
		|| AFF_FLAGGED(ch, AFF_HOLD)
			// mob_casting мог от зеркала отразиться
		||	AFF_FLAGGED(ch, AFF_STOPFIGHT)
		|| !AWAKE(ch)
		|| AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT))
	{
		return;
	}

	if ((AFF_FLAGGED(ch, AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL))
		&& AFF_FLAGGED(ch, AFF_HELPER)
		&& ch->master
		// && !IS_NPC(ch->master)
		&& CAN_SEE(ch, ch->master)
		&& IN_ROOM(ch) == IN_ROOM(ch->master)
		&& AWAKE(ch)
		&& MAY_ACT(ch)
		&& GET_POS(ch) >= POS_FIGHTING)
	{
		CHAR_DATA *vict = 0;
		for (vict = world[IN_ROOM(ch)]->people;
			vict; vict = vict->next_in_room)
		{
			if (vict->get_fighting() == ch->master
				&& vict != ch
				&& vict != ch->master)
			{
				break;
			}
		}
		if (vict && (ch->get_skill(SKILL_RESCUE)))
		{
			go_rescue(ch, ch->master, vict);
		}
		else if (vict && ch->get_skill(SKILL_PROTECT))
		{
			go_protect(ch, ch->master);
		}
	}
	else if (!AFF_FLAGGED(ch, AFF_CHARM))
	{
		//* применение скилов
		using_mob_skills(ch);
	}

	if (!ch->get_fighting() || IN_ROOM(ch) != IN_ROOM(ch->get_fighting()))
		return;

	//**** удар основным оружием или рукой
	if (!AFF_FLAGGED(ch, AFF_STOPRIGHT))
		exthit(ch, TYPE_UNDEFINED, RIGHT_WEAPON);

	//**** экстраатаки
	for (int i = 1; i <= ch->mob_specials.ExtraAttack; i++)
	{
		if (AFF_FLAGGED(ch, AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)
			|| (i == 1 && AFF_FLAGGED(ch, AFF_STOPLEFT)))
		{
			continue;
		}
		exthit(ch, TYPE_UNDEFINED, i + RIGHT_WEAPON);
	}
}

void process_player_attack(CHAR_DATA *ch, int min_init)
{
	if (GET_POS(ch) > POS_STUNNED
		&& GET_POS(ch) < POS_FIGHTING
		&& GET_AF_BATTLE(ch, EAF_STAND))
	{
		sprintf(buf, "%sВам лучше встать на ноги!%s\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		CLR_AF_BATTLE(ch, EAF_STAND);
	}

	//* каст заклинания
	if (ch->get_cast_spell() && GET_WAIT(ch) <= 0)
	{
		if (AFF_FLAGGED(ch, AFF_SIELENCE) || AFF_FLAGGED(ch, AFF_STRANGLED))
		{
			send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
			ch->set_cast(0, 0, 0, 0, 0);
		}
		else
		{
			cast_spell(ch, ch->get_cast_char(), ch->get_cast_obj(),
				0, ch->get_cast_spell(), ch->get_cast_subst());

			if (!(IS_IMMORTAL(ch)
				|| GET_GOD_FLAG(ch, GF_GODSLIKE)
				|| CHECK_WAIT(ch)))
			{
				WAIT_STATE(ch, PULSE_VIOLENCE);
			}
			ch->set_cast(0, 0, 0, 0, 0);
		}
		if (INITIATIVE(ch) > min_init)
		{
			INITIATIVE(ch)--;
			return;
		}
	}

	if (GET_AF_BATTLE(ch, EAF_MULTYPARRY))
		return;

	//* применение экстра атаки
	if (ch->get_extra_victim()
		&& GET_WAIT(ch) <= 0
		&& using_extra_attack(ch))
	{
		ch->set_extra_attack(0, 0);
		if (INITIATIVE(ch) > min_init)
		{
			INITIATIVE(ch)--;
			return;
		}
	}

	if (!ch->get_fighting() || IN_ROOM(ch) != IN_ROOM(ch->get_fighting()))
		return;

	//**** удар основным оружием или рукой
	if (GET_AF_BATTLE(ch, EAF_FIRST))
	{
		if (!AFF_FLAGGED(ch, AFF_STOPRIGHT)
			&& (IS_IMMORTAL(ch)
				|| GET_GOD_FLAG(ch, GF_GODSLIKE)
				|| !GET_AF_BATTLE(ch, EAF_USEDRIGHT)))
		{
			//Знаю, выглядит страшно, но зато в hit()
			//можно будет узнать применялось ли оглушить
			//или молотить, по баттл-аффекту узнать получиться
			//не во всех частях процедуры, а параметр type
			//хранит значение до её конца.
			exthit(ch, GET_AF_BATTLE(ch, EAF_STUPOR) ? SKILL_STUPOR : GET_AF_BATTLE(ch, EAF_MIGHTHIT) ? SKILL_MIGHTHIT : TYPE_UNDEFINED, RIGHT_WEAPON);
		}
		CLR_AF_BATTLE(ch, EAF_FIRST);
		SET_AF_BATTLE(ch, EAF_SECOND);
		if (INITIATIVE(ch) > min_init)
		{
			INITIATIVE(ch)--;
			return;
		}
	}

	//**** удар вторым оружием если оно есть и умение позволяет
	if (GET_EQ(ch, WEAR_HOLD)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON
		&& GET_AF_BATTLE(ch, EAF_SECOND)
		&& !AFF_FLAGGED(ch, AFF_STOPLEFT)
		&& (IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE)
			|| ch->get_skill(SKILL_SATTACK) > number(1, 101)))
	{
		if (IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE)
			|| !GET_AF_BATTLE(ch, EAF_USEDLEFT))
		{
			exthit(ch, TYPE_UNDEFINED, LEFT_WEAPON);
		}
		CLR_AF_BATTLE(ch, EAF_SECOND);
	}
	//**** удар второй рукой если она свободна и умение позволяет
	else if (!GET_EQ(ch, WEAR_HOLD)
		&& !GET_EQ(ch, WEAR_LIGHT)
		&& !GET_EQ(ch, WEAR_SHIELD)
		&& !GET_EQ(ch, WEAR_BOTHS)
		&& !AFF_FLAGGED(ch, AFF_STOPLEFT)
		&& GET_AF_BATTLE(ch, EAF_SECOND)
		&& ch->get_skill(SKILL_SHIT))
	{
		if (IS_IMMORTAL(ch) || !GET_AF_BATTLE(ch, EAF_USEDLEFT))
		{
			exthit(ch, TYPE_UNDEFINED, LEFT_WEAPON);
		}
		CLR_AF_BATTLE(ch, EAF_SECOND);
	}

	// немного коряво, т.к. зависит от инициативы кастера
	// check if angel is in fight, and go_rescue if it is not
	try_angel_rescue(ch);
}

// * \return false - чар не участвует в расчете инициативы
bool stuff_before_round(CHAR_DATA *ch)
{
	// Initialize initiative
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) += 1;
	DpsSystem::check_round(ch);
	BattleRoundParameters params(ch);
	handle_affects(params);
	round_num_mtrigger(ch, ch->get_fighting());

	SET_AF_BATTLE(ch, EAF_STAND);
	if (affected_by_spell(ch, SPELL_SLEEP))
		SET_AF_BATTLE(ch, EAF_SLEEP);
	if (IN_ROOM(ch) == NOWHERE)
		return false;

	if (GET_MOB_HOLD(ch)
		|| AFF_FLAGGED(ch, AFF_STOPFIGHT)
		|| AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT))
	{
		try_angel_rescue(ch);
		return false;
	}

	// Mobs stand up and players sit
	if (GET_POS(ch) < POS_FIGHTING
		&& GET_POS(ch) > POS_STUNNED
		&& GET_WAIT(ch) <= 0
		&& !GET_MOB_HOLD(ch)
		&& !AFF_FLAGGED(ch, AFF_SLEEP))
	{
		stand_up_or_sit(ch);
	}

	// For NPC without lags and charms make it likes
	if (IS_NPC(ch) && MAY_LIKES(ch))  	// Get weapon from room
	{
		//edited by WorM 2010.09.03 добавил немного логики мобам в бою если
		// у моба есть что-то в инве то он может
		//переодется в это что-то если посчитает что это что-то ему лучше подходит
		if (!AFF_FLAGGED(ch, AFF_CHARM))
		{
			//Чото бред какой-то был, одевались мобы только сразу после того как слутили
			npc_battle_scavenge(ch);//лутим стаф
			if(ch->carrying)//и если есть что-то в инве то вооружаемся, одеваемся
			{
				npc_wield(ch);
				npc_armor(ch);
			}
		}
		//end by WorM
		//dzMUDiST. Выполнение последнего переданого в бою за время лага приказа
		if (ch->last_comm != NULL)
		{
			command_interpreter(ch, ch->last_comm);
			free(ch->last_comm);
			ch->last_comm = NULL;
		}
		// Set some flag-skills
		set_mob_skills_flags(ch);
	}

	return true;
}

// * Обработка текущих боев, дергается каждые 2 секунды.
void perform_violence()
{
	int max_init = 0, min_init = 100;

	//* суммон хелперов
	check_mob_helpers();

	//* действия до раунда и расчет инициативы
	for (CHAR_DATA *ch = combat_list; ch; ch = next_combat_list)
	{
		next_combat_list = ch->next_fighting;

		if (!stuff_before_round(ch))
			continue;

		const int initiative = calc_initiative(ch);
		INITIATIVE(ch) = initiative;
		SET_AF_BATTLE(ch, EAF_FIRST);
		max_init = MAX(max_init, initiative);
		min_init = MIN(min_init, initiative);
	}

	//* обработка раунда по очередности инициативы
	for (int initiative = max_init; initiative >= min_init; initiative--)
	{
		for (CHAR_DATA *ch = combat_list; ch; ch = next_combat_list)
		{
			next_combat_list = ch->next_fighting;
			if (INITIATIVE(ch) != initiative || IN_ROOM(ch) == NOWHERE)
			{
				continue;
			}
			// If mob cast 'hold' when initiative setted
			if (AFF_FLAGGED(ch, AFF_HOLD)
				|| AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)
				|| AFF_FLAGGED(ch, AFF_STOPFIGHT)
				|| !AWAKE(ch))
			{
				continue;
			}
			// If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
			if (!ch->get_fighting()
				|| IN_ROOM(ch) != IN_ROOM(ch->get_fighting()))
			{
				continue;
			}
			//* выполнение атак в раунде
			if (IS_NPC(ch))
			{
				process_npc_attack(ch);
			}
			else
			{
				process_player_attack(ch, min_init);
			}
		}
	}

	//* обновление аффектов и лагов после раунда
	update_round_affs();
}

// returns 1 if only ch was outcasted
// returns 2 if only victim was outcasted
// returns 4 if both were outcasted
// returns 0 if none was outcasted
int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim)
{
	CHAR_DATA *cleader, *vleader;
	int return_value = 0;
	if (ch == victim)
		return return_value;
// translating pointers from charimces to their leaders
	if (IS_NPC(ch) && ch->master && (AFF_FLAGGED(ch, AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL) || IS_HORSE(ch)))
		ch = ch->master;
	if (IS_NPC(victim) && victim->master &&
			(AFF_FLAGGED(victim, AFF_CHARM) || MOB_FLAGGED(victim, MOB_ANGEL) || IS_HORSE(victim)))
		victim = victim->master;
	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->master)
	{
		if (IS_NPC(cleader) &&
				!AFF_FLAGGED(cleader, AFF_CHARM) && !MOB_FLAGGED(cleader, MOB_ANGEL) && !IS_HORSE(cleader))
			break;
		cleader = cleader->master;
	}
	while (vleader->master)
	{
		if (IS_NPC(vleader) &&
				!AFF_FLAGGED(vleader, AFF_CHARM) && !MOB_FLAGGED(vleader, MOB_ANGEL) && !IS_HORSE(vleader))
			break;
		vleader = vleader->master;
	}
	if (cleader != vleader)
		return return_value;


// finding closest to the leader nongrouped agressor
// it cannot be a charmice
	while (ch->master && ch->master->master)
	{
		if (!AFF_FLAGGED(ch->master, AFF_GROUP) && !IS_NPC(ch->master))
		{
			ch = ch->master;
			continue;
		}
		else if (IS_NPC(ch->master)
				 && !AFF_FLAGGED(ch->master->master, AFF_GROUP)
				 && !IS_NPC(ch->master->master) && ch->master->master->master)
		{
			ch = ch->master->master;
			continue;
		}
		else
			break;
	}

// finding closest to the leader nongrouped victim
// it cannot be a charmice
	while (victim->master && victim->master->master)
	{
		if (!AFF_FLAGGED(victim->master, AFF_GROUP)
				&& !IS_NPC(victim->master))
		{
			victim = victim->master;
			continue;
		}
		else if (IS_NPC(victim->master)
				 && !AFF_FLAGGED(victim->master->master, AFF_GROUP)
				 && !IS_NPC(victim->master->master)
				 && victim->master->master->master)
		{
			victim = victim->master->master;
			continue;
		}
		else
			break;
	}
	if (!AFF_FLAGGED(ch, AFF_GROUP) || cleader == victim)
	{
		stop_follower(ch, SF_EMPTY);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, AFF_GROUP) || vleader == ch)
	{
		stop_follower(victim, SF_EMPTY);
		return_value |= 2;
	}
	return return_value;
}
