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

#include "world.characters.hpp"
#include "fight_hit.hpp"
#include "AffectHandler.hpp"
#include "obj.hpp"
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
#include "logger.hpp"
#include "utils.h"
#include "msdp.constants.hpp"

#include <unordered_set>

// Structures
CHAR_DATA *combat_list = NULL;	// head of l-list of fighting chars
CHAR_DATA *next_combat_list = NULL;

extern int r_helled_start_room;
extern MobRaceListType mobraces_list;
extern CHAR_DATA *mob_proto;
extern DESCRIPTOR_DATA *descriptor_list;
// External procedures
// void do_assist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
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
void go_cut_shorts(CHAR_DATA * ch, CHAR_DATA * vict);
int npc_battle_scavenge(CHAR_DATA * ch);
void npc_wield(CHAR_DATA * ch);
void npc_armor(CHAR_DATA * ch);
int perform_mob_switch(CHAR_DATA * ch);
void do_assist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

void go_autoassist(CHAR_DATA * ch)
{
	struct follow_type *k;
	struct follow_type *d;
	CHAR_DATA *ch_lider = 0;
	if (ch->has_master())
	{
		ch_lider = ch->get_master();
	}
	else
	{
		ch_lider = ch;	// Создаем ссылку на лидера
	}

	buf2[0] = '\0';
	for (k = ch_lider->followers; k; k = k->next)
	{
		if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
			(IN_ROOM(k->follower) == IN_ROOM(ch)) && !k->follower->get_fighting() &&
			(GET_POS(k->follower) == POS_STANDING) && !CHECK_WAIT(k->follower))
		{
			// Здесь проверяем на кастеров
			if (IS_CASTER(k->follower))
			{
				// здесь проходим по чармисам кастера, и если находим их, то вписываем в драку
				for (d = k->follower->followers; d; d = d->next)
					if ((IN_ROOM(d->follower) == IN_ROOM(ch)) && !d->follower->get_fighting() &&
						(GET_POS(d->follower) == POS_STANDING) && !CHECK_WAIT(d->follower))
						do_assist(d->follower, buf2, 0, 0);
			}
			else
			{
				do_assist(k->follower, buf2, 0, 0);
			}
		}
	}
}


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

	if (AFF_FLAGGED(victim, EAffectFlag::AFF_SLEEP) && GET_POS(victim) != POS_SLEEPING)
		affect_from_char(victim, SPELL_SLEEP);

	if (on_horse(victim) && GET_POS(victim) < POS_FIGHTING)
		horse_drop(get_horse(victim));

	if (IS_HORSE(victim)
		&& GET_POS(victim) < POS_FIGHTING
		&& on_horse(victim->get_master()))
	{
		horse_drop(victim);
	}
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
				!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
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
				!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		{
			act("$n встал$g на ноги.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			GET_POS(ch) = POS_STANDING;
		}
		break;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
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

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BANDAGE))
	{
		send_to_char("Перевязка была прервана!\r\n", ch);
		affect_from_char(ch, SPELL_BANDAGE);
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_RECALL_SPELLS))
	{
		send_to_char("Вы забыли о концентрации и ринулись в бой!\r\n", ch);
		affect_from_char(ch, SPELL_RECALL_SPELLS);
	}

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
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
	ch->set_extra_attack(EXTRA_ATTACK_UNUSED, 0);
	set_battle_pos(ch);

	// если до начала боя на мобе есть лаг, то мы его выравниваем до целых
	// раундов в большую сторону (для подножки, должно давать чару зазор в две
	// секунды после подножки, чтобы моб всеравно встал только на 3й раунд)
	if (IS_NPC(ch) && GET_WAIT(ch) > 0)
	{
		div_t tmp = div(static_cast<const int>(ch->get_wait()), static_cast<const int>(PULSE_VIOLENCE));
		if (tmp.rem > 0)
		{
			WAIT_STATE(ch, (tmp.quot + 1) * PULSE_VIOLENCE);
		}
	}
	if (!IS_NPC(ch) && (!ch->get_skill(SKILL_AWAKE)))
	{
		PRF_FLAGS(ch).unset(PRF_AWAKE);
	}

	if (!IS_NPC(ch) && (!ch->get_skill(SKILL_PUNCTUAL)))
	{
		PRF_FLAGS(ch).unset(PRF_PUNCTUAL);
	}

	// Set combat style
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_COURAGE) && !AFF_FLAGGED(ch, EAffectFlag::AFF_DRUNKED) && !AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT))
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

	REMOVE_FROM_LIST(ch, combat_list, [](auto list) -> auto& { return list->next_fighting; });
	//Попробуем сперва очистить ссылку у врага, потом уже у самой цели
	ch->next_fighting = NULL;
	if (ch->last_comm != NULL)
		free(ch->last_comm);
	ch->last_comm = NULL;
	ch->set_touching(0);
	ch->set_fighting(0);
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) = 0;
	ch->set_extra_attack(EXTRA_ATTACK_UNUSED, 0);
	ch->set_cast(0, 0, 0, 0, 0);
	restore_battle_pos(ch);
	NUL_AF_BATTLE(ch);
	DpsSystem::check_round(ch);
	StopFightParameters params(ch); //готовим параметры нужного типа и вызываем шаблонную функцию
	handle_affects(params);
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
				temp->set_extra_attack(EXTRA_ATTACK_UNUSED, 0);
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
				{
					stop_fighting(temp, FALSE);
				};
			}
		}

		update_pos(ch);
		// проверка скилла "железный ветер" - снимаем флаг по окончанию боя
		if ((ch->get_fighting() == NULL) && PRF_FLAGS(ch).get(PRF_IRON_WIND))
		{
			PRF_FLAGS(ch).unset(PRF_IRON_WIND);
			if (GET_POS(ch) > POS_INCAP)
			{
				send_to_char("Безумие боя отпустило вас, и враз навалилась усталость...\r\n", ch);
				act("$n шумно выдохнул$g и остановил$u, переводя дух после боя.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			};
		};
	};
}

int GET_MAXDAMAGE(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD))
		return 0;
	else
		return GET_DAMAGE(ch);
}

int GET_MAXCASTER(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD) || AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)
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
	CHAR_DATA *vict, *npc_master, *pc_master, *ch_master, *vict_master;

	if (npc == pc)
		return (!opponent);
	if (npc->get_fighting() == pc)	// NPC fight PC - opponent
		return (opponent);
	if (pc->get_fighting() == npc)	// PC fight NPC - opponent
		return (opponent);
	if (npc->get_fighting() && npc->get_fighting() == pc->get_fighting())
		return (!opponent);	// Fight same victim - friend
	if (AFF_FLAGGED(pc, EAffectFlag::AFF_HORSE) || AFF_FLAGGED(pc, EAffectFlag::AFF_CHARM))
		return (opponent);

	npc_master = npc->has_master() ? npc->get_master() : npc;
	pc_master = pc->has_master() ? pc->get_master() : pc;

	for (const auto ch : world[IN_ROOM(npc)]->people)	// <- Anton Gorev (2017-07-11): changed loop to loop over the people in room (instead of all world)
	{
		if (!ch->get_fighting())
		{
			continue;
		}

		ch_master = ch->has_master() ? ch->get_master() : ch;
		ch_friend_npc = (ch_master == npc_master) ||
						(IS_NPC(ch) && IS_NPC(npc) &&
						 !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !AFF_FLAGGED(npc, EAffectFlag::AFF_CHARM) &&
						 !AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE) && !AFF_FLAGGED(npc, EAffectFlag::AFF_HORSE));
		ch_friend_pc = (ch_master == pc_master) ||
					   (IS_NPC(ch) && IS_NPC(pc) &&
						!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !AFF_FLAGGED(pc, EAffectFlag::AFF_CHARM) &&
						!AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE) && !AFF_FLAGGED(pc, EAffectFlag::AFF_HORSE));
		if (ch->get_fighting() == pc && ch_friend_npc)	// Friend NPC fight PC - opponent
			return (opponent);
		if (pc->get_fighting() == ch && ch_friend_npc)	// PC fight friend NPC - opponent
			return (opponent);
		if (npc->get_fighting() == ch && ch_friend_pc)	// NPC fight friend PC - opponent
			return (opponent);
		if (ch->get_fighting() == npc && ch_friend_pc)	// Friend PC fight NPC - opponent
			return (opponent);
		vict = ch->get_fighting();
		vict_master = vict->has_master() ? vict->get_master() : vict;
		vict_friend_npc = (vict_master == npc_master) ||
						  (IS_NPC(vict) && IS_NPC(npc) &&
						   !AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM) && !AFF_FLAGGED(npc, EAffectFlag::AFF_CHARM) &&
						   !AFF_FLAGGED(vict, EAffectFlag::AFF_HORSE) && !AFF_FLAGGED(npc, EAffectFlag::AFF_HORSE));
		vict_friend_pc = (vict_master == pc_master) ||
						 (IS_NPC(vict) && IS_NPC(pc) &&
						  !AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM) && !AFF_FLAGGED(pc, EAffectFlag::AFF_CHARM) &&
						  !AFF_FLAGGED(vict, EAffectFlag::AFF_HORSE) && !AFF_FLAGGED(pc, EAffectFlag::AFF_HORSE));
		if (ch_friend_npc && vict_friend_pc)
			return (opponent);	// Friend NPC fight friend PC - opponent
		if (ch_friend_pc && vict_friend_npc)
			return (opponent);	// Friend PC fight friend NPC - opponent
	}

	return (!opponent);
}

CHAR_DATA *find_friend_cure(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *victim = NULL;
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

	if ((AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)|| MOB_FLAGGED(caster, MOB_GHOST))
			&& AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER))
	{
		if (GET_HP_PERC(caster) < AFF_USED)
		{
			return caster;
		}
		else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& caster->get_master()->get_fighting()
			&& GET_HP_PERC(caster->get_master()) < AFF_USED)
		{
			return caster->get_master();
		}
		return nullptr;
	}

	for (const auto vict : world[IN_ROOM(caster)]->people)
	{
		if (!IS_NPC(vict)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			|| ((MOB_FLAGGED(vict, MOB_ANGEL) ||MOB_FLAGGED(vict, MOB_GHOST))
				&& vict->has_master()
				&& !IS_NPC(vict->get_master()))
			|| !CAN_SEE(caster, vict))
		{
			continue;
		}

		if (!vict->get_fighting() && !MOB_FLAGGED(vict, MOB_HELPER))
		{
			continue;
		}

		if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict)))
		{
			victim = vict;
			vict_val = GET_HP_PERC(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
			{
				break;
			}
		}
	}
	return victim;
}

CHAR_DATA *find_friend(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *victim = NULL;
	int vict_val = 0, spellreal = -1;
	affects_list_t AFF_USED;
	switch (spellnum)
	{
	case SPELL_CURE_BLIND:
		AFF_USED.push_back(EAffectFlag::AFF_BLIND);
		break;

	case SPELL_REMOVE_POISON:
		AFF_USED.push_back(EAffectFlag::AFF_POISON);
		AFF_USED.push_back(EAffectFlag::AFF_SCOPOLIA_POISON);
		AFF_USED.push_back(EAffectFlag::AFF_BELENA_POISON);
		AFF_USED.push_back(EAffectFlag::AFF_DATURA_POISON);
		break;

	case SPELL_REMOVE_HOLD:
		AFF_USED.push_back(EAffectFlag::AFF_HOLD);
		break;

	case SPELL_REMOVE_CURSE:
		AFF_USED.push_back(EAffectFlag::AFF_CURSE);
		break;

	case SPELL_REMOVE_SILENCE:
		AFF_USED.push_back(EAffectFlag::AFF_SILENCE);
		break;

	case SPELL_CURE_PLAQUE:
		spellreal = SPELL_PLAQUE;
		break;
	}
	if (AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER)
		&& (AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(caster, MOB_ANGEL)|| MOB_FLAGGED(caster, MOB_GHOST)))
	{
		if (caster->has_any_affect(AFF_USED)
			|| affected_by_spell(caster, spellreal))
		{
			return caster;
		}
		else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& (caster->get_master()->has_any_affect(AFF_USED)
				|| affected_by_spell(caster->get_master(), spellreal)))
		{
			return caster->get_master();
		}

		return NULL;
	}

	if (!AFF_USED.empty())
	{
		for (const auto vict : world[IN_ROOM(caster)]->people)
		{
			if (!IS_NPC(vict)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST))
					&& vict->get_master()
					&& !IS_NPC(vict->get_master()))
				|| !CAN_SEE(caster, vict))
			{
				continue;
			}

			if (!vict->has_any_affect(AFF_USED))
			{
				continue;
			}

			if (!vict->get_fighting()
				&& !MOB_FLAGGED(vict, MOB_HELPER))
			{
				continue;
			}

			if (!victim
				|| vict_val < GET_MAXDAMAGE(vict))
			{
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
				if (GET_REAL_INT(caster) < number(10, 20))
				{
					break;
				}
			}
		}
	}

	return victim;
}

CHAR_DATA *find_caster(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *victim = NULL;
	int vict_val = 0, spellreal = -1;
	affects_list_t AFF_USED;
	switch (spellnum)
	{
	case SPELL_CURE_BLIND:
		AFF_USED.push_back(EAffectFlag::AFF_BLIND);
		break;
	case SPELL_REMOVE_POISON:
		AFF_USED.push_back(EAffectFlag::AFF_POISON);
		AFF_USED.push_back(EAffectFlag::AFF_SCOPOLIA_POISON);
		AFF_USED.push_back(EAffectFlag::AFF_BELENA_POISON);
		AFF_USED.push_back(EAffectFlag::AFF_DATURA_POISON);
		break;
	case SPELL_REMOVE_HOLD:
		AFF_USED.push_back(EAffectFlag::AFF_HOLD);
		break;
	case SPELL_REMOVE_CURSE:
		AFF_USED.push_back(EAffectFlag::AFF_CURSE);
		break;
	case SPELL_REMOVE_SILENCE:
		AFF_USED.push_back(EAffectFlag::AFF_SILENCE);
		break;
	case SPELL_CURE_PLAQUE:
		spellreal = SPELL_PLAQUE;
		break;
	}

	if (AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER)
		&& (AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(caster, MOB_ANGEL)|| MOB_FLAGGED(caster, MOB_GHOST)))
	{
		if (caster->has_any_affect(AFF_USED)
			|| affected_by_spell(caster, spellreal))
		{
			return caster;
		}
		else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& (caster->get_master()->has_any_affect(AFF_USED)
				|| affected_by_spell(caster->get_master(), spellreal)))
		{
			return caster->get_master();
		}

		return NULL;
	}

	if (!AFF_USED.empty())
	{
		for (const auto vict : world[IN_ROOM(caster)]->people)
		{
			if (!IS_NPC(vict)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST))
					&& (vict->get_master() && !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict))
			{
				continue;
			}

			if (!vict->has_any_affect(AFF_USED))
			{
				continue;
			}

			if (!vict->get_fighting()
				&& !MOB_FLAGGED(vict, MOB_HELPER))
			{
				continue;
			}

			if (!victim
				|| vict_val < GET_MAXCASTER(vict))
			{
				victim = vict;
				vict_val = GET_MAXCASTER(vict);
				if (GET_REAL_INT(caster) < number(10, 20))
				{
					break;
				}
			}
		}
	}

	return victim;
}

CHAR_DATA *find_affectee(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *victim = NULL;
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
	else if (spellreal == SPELL_SIGHT_OF_DARKNESS)
		spellreal = SPELL_INFRAVISION;
	else if (spellreal == SPELL_GENERAL_SINCERITY)
		spellreal = SPELL_DETECT_ALIGN;
	else if (spellreal == SPELL_MAGICAL_GAZE)
		spellreal = SPELL_DETECT_MAGIC;
	else if (spellreal == SPELL_ALL_SEEING_EYE)
		spellreal = SPELL_DETECT_INVIS;
	else if (spellreal == SPELL_EYE_OF_GODS)
		spellreal = SPELL_SENSE_LIFE;
	else if (spellreal == SPELL_BREATHING_AT_DEPTH)
		spellreal = SPELL_WATERBREATH;
	else if (spellreal == SPELL_GENERAL_RECOVERY)
		spellreal = SPELL_FAST_REGENERATION;
	else if (spellreal == SPELL_COMMON_MEAL)
		spellreal = SPELL_FULL;
	else if (spellreal == SPELL_STONE_WALL)
		spellreal = SPELL_STONESKIN;
	else if (spellreal == SPELL_SNAKE_EYES)
		spellreal = SPELL_DETECT_POISON;

	if ((AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)|| MOB_FLAGGED(caster, MOB_GHOST)) && AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER))
	{
		if (!affected_by_spell(caster, spellreal))
		{
			return caster;
		}
		else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& caster->get_master()->get_fighting() && !affected_by_spell(caster->get_master(), spellreal))
		{
			return caster->get_master();
		}

		return nullptr;
	}

	if (GET_REAL_INT(caster) > number(5, 15))
	{
		for (const auto vict : world[IN_ROOM(caster)]->people)
		{
			if (!IS_NPC(vict)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST))
					&& vict->has_master()
					&& !IS_NPC(vict->get_master()))
				|| !CAN_SEE(caster, vict))
			{
				continue;
			}

			if (!vict->get_fighting()
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				|| affected_by_spell(vict, spellreal))
			{
				continue;
			}

			if (!victim || vict_val < GET_MAXDAMAGE(vict))
			{
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim
		&& !affected_by_spell(caster, spellreal))
	{
		victim = caster;
	}

	return victim;
}

CHAR_DATA *find_opp_affectee(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *victim = NULL;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == SPELL_POWER_HOLD || spellreal == SPELL_MASS_HOLD)
		spellreal = SPELL_HOLD;
	else if (spellreal == SPELL_POWER_BLINDNESS || spellreal == SPELL_MASS_BLINDNESS)
		spellreal = SPELL_BLINDNESS;
	else if (spellreal == SPELL_POWER_SILENCE || spellreal == SPELL_MASS_SILENCE)
		spellreal = SPELL_SILENCE;
	else if (spellreal == SPELL_MASS_CURSE)
		spellreal = SPELL_CURSE;
	else if (spellreal == SPELL_MASS_SLOW)
		spellreal = SPELL_SLOW;

	if (GET_REAL_INT(caster) > number(10, 20))
	{
		for (const auto vict : world[caster->in_room]->people)
		{
			if ((IS_NPC(vict)
				&& !((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
					&& vict->has_master()
					&& !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict))
			{
				continue;
			}

			if ((!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, TRUE)))
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				|| affected_by_spell(vict, spellreal))
			{
				continue;
			}
			if (!victim || vict_val < GET_MAXDAMAGE(vict))
			{
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim
		&& caster->get_fighting()
		&& !affected_by_spell(caster->get_fighting(), spellreal))
	{
		victim = caster->get_fighting();
	}

	return victim;
}

CHAR_DATA *find_opp_caster(CHAR_DATA * caster)
{
	CHAR_DATA *victim = NULL;
	int vict_val = 0;

	for (const auto vict : world[IN_ROOM(caster)]->people)
	{
		if (IS_NPC(vict)
			&& !((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST))
				&& vict->has_master()
				&& !IS_NPC(vict->get_master())))
		{
			continue;
		}
		if ((!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(15, 25)
					|| !in_same_battle(caster, vict, TRUE)))
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD) || AFF_FLAGGED(vict, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)
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
	CHAR_DATA *victim = NULL;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20))
	{
		for (const auto vict : world[IN_ROOM(caster)]->people)
		{
			if ((IS_NPC(vict)
					&& !((MOB_FLAGGED(vict, MOB_ANGEL)
							|| MOB_FLAGGED(vict, MOB_GHOST)
							|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
						&& vict->has_master()
						&& !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict))
			{
				continue;
			}

			if ((!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, TRUE)))
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD))
			{
				continue;
			}

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
	}

	if (!victim)
	{
		victim = caster->get_fighting();
	}

	return victim;
}

extern bool find_master_charmice(CHAR_DATA *charmise);
CHAR_DATA *find_target(CHAR_DATA *ch)
{
	CHAR_DATA *victim, *caster = NULL, *best = NULL;
	CHAR_DATA *druid = NULL, *cler = NULL, *charmmage = NULL;
	victim = ch->get_fighting();

	// интелект моба
	int i = GET_REAL_INT(ch);
	// если у моба меньше 20 инты, то моб тупой
	if (i < INT_STUPID_MOD)
	{
		return find_damagee(ch);
	}

	// если нет цели
	if (!victim)
	{
		return NULL;
	}

	// проходим по всем чарам в комнате
	for (const auto vict : world[ch->in_room]->people)
	{
		if ((IS_NPC(vict) && !IS_CHARMICE(vict))
				|| (IS_CHARMICE(vict) && !vict->get_fighting() && find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
				|| PRF_FLAGGED(vict, PRF_NOHASSLE)
				|| !MAY_SEE(ch, ch, vict))
		{
			continue;
		}

		if (!CAN_SEE(ch, vict))
		{
			continue;
		}

		// волхв
		if (GET_CLASS(vict) == CLASS_DRUID)
		{
			druid = vict;
			caster = vict;
			continue;
		}

		// лекарь
		if (GET_CLASS(vict) == CLASS_CLERIC)
		{
			cler = vict;
			caster = vict;
			continue;
		}

		// кудес
		if (GET_CLASS(vict) == CLASS_CHARMMAGE)
		{
			charmmage = vict;
			caster = vict;
			continue;
		}

		// если у чара меньше 100 хп, то переключаемся на него
		if (GET_HIT(vict) <= MIN_HP_MOBACT)
		{
			return vict;
		}

		if (IS_CASTER(vict))
		{
			caster = vict;
			continue;
		}

		best = vict;
	}
	if (!best)
		best = victim;

	// если цель кастер, то зачем переключаться ?
	if (IS_CASTER(victim) && !IS_NPC(victim))
	{
		return victim;
	}


	if (i < INT_MIDDLE_AI)
	{
		int rand = number(0, 2);
		if (caster)
			best = caster;
		if ((rand == 0) && (druid))
			best = druid;
		if ((rand == 1) && (cler))
			best = cler;
		if ((rand == 2) && (charmmage))
			best = charmmage;
		return best;
	}

	if (i < INT_HIGH_AI)
	{
		int rand = number(0, 1);
		if (caster)
			best = caster;
		if (charmmage)
			best = charmmage;
		if ((rand == 0) && (druid))
			best = druid;
		if ((rand == 1) && (cler))
			best = cler;

		return best;
	}

	//  и если >= 40 инты
	if (caster)
		best = caster;
	if (charmmage)
		best = charmmage;
	if (cler)
		best = cler;
	if (druid)
		best = druid;

	return best;
}

CHAR_DATA *find_minhp(CHAR_DATA * caster)
{
	CHAR_DATA *victim = NULL;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20))
	{
		for (const auto vict : world[IN_ROOM(caster)]->people)
		{
			if ((IS_NPC(vict)
				&& !((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
					&& vict->has_master()
					&& !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict))
			{
				continue;
			}

			if (!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, TRUE)))
			{
				continue;
			}

			if (!victim || vict_val > GET_HIT(vict))
			{
				victim = vict;
				vict_val = GET_HIT(vict);
			}
		}
	}

	if (!victim)
	{
		victim = caster->get_fighting();
	}

	return victim;
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
	int spellnum, spells = 0, sp_num;
	OBJ_DATA *item;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) 
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)
		|| GET_WAIT(ch) > 0)
		return;

	memset(&battle_spells, 0, sizeof(battle_spells));
	for (int i = 1; i <= MAX_SPELLS; i++)
	{
		if (GET_SPELL_MEM(ch, i) && IS_SET(spell_info[i].routines, NPC_CALCULATE))
		{
			battle_spells[spells++] = i;
		}
	}

	item = ch->carrying;
	while (spells < MAX_STRING_LENGTH
		&& item
		&& GET_RACE(ch) == NPC_RACE_HUMAN
		&& !(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST)))
	{
		switch (GET_OBJ_TYPE(item))
		{
		case OBJ_DATA::ITEM_WAND:
		case OBJ_DATA::ITEM_STAFF:
			if (GET_OBJ_VAL(item, 3) < 0 || GET_OBJ_VAL(item, 3) > TOP_SPELL_DEFINE)
			{
				log("SYSERR: Не верно указано значение спела в стафе vnum: %d %s, позиция: 3, значение: %d ", GET_OBJ_VNUM(item), item->get_PName(0).c_str(), GET_OBJ_VAL(item, 3));
				break;
			}

			if (GET_OBJ_VAL(item, 2) > 0 &&
				IS_SET(spell_info[GET_OBJ_VAL(item, 3)].routines, NPC_CALCULATE))
			{
				battle_spells[spells++] = GET_OBJ_VAL(item, 3);
			}
			break;

		case OBJ_DATA::ITEM_POTION:
			for (int i = 1; i <= 3; i++)
			{
				if (GET_OBJ_VAL(item, i) < 0 || GET_OBJ_VAL(item, i) > TOP_SPELL_DEFINE)
				{
					log("SYSERR: Не верно указано значение спела в напитке vnum %d %s, позиция: %d, значение: %d ", GET_OBJ_VNUM(item), item->get_PName(0).c_str(), i, GET_OBJ_VAL(item, i));
					continue;
				}
				if (IS_SET(spell_info[GET_OBJ_VAL(item, i)].routines, NPC_AFFECT_NPC | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER))
				{
					battle_spells[spells++] = GET_OBJ_VAL(item, i);
				}
			}
			break;

		case OBJ_DATA::ITEM_SCROLL:
			for (int i = 1; i <= 3; i++)
			{
				if (GET_OBJ_VAL(item, i) < 0 || GET_OBJ_VAL(item, i) > TOP_SPELL_DEFINE)
				{	
					log("SYSERR: Не верно указано значение спела в свитке %d %s, позиция: %d, значение: %d ", GET_OBJ_VNUM(item), item->get_PName(0).c_str(), i, GET_OBJ_VAL(item, i));
					continue;
				}

				if (IS_SET(spell_info[GET_OBJ_VAL(item, i)].routines, NPC_CALCULATE))
				{
					battle_spells[spells++] = GET_OBJ_VAL(item, i);
				}
			}
			break;

		default:
			break;
		}

		item = item->get_next_content();
	}

	// перво-наперво  -  лечим себя
	spellnum = 0;
	victim = find_cure(ch, ch, &spellnum);
	// Ищем рандомную заклинашку и цель для нее
	for (int i = 0; !victim && spells && i < GET_REAL_INT(ch) / 5; i++)
	{
		if (!spellnum && (spellnum = battle_spells[(sp_num = number(0, spells - 1))])
			&& spellnum > 0 && spellnum <= MAX_SPELLS)  	// sprintf(buf,"$n using spell '%s', %d from %d",
		{
			//         spell_name(spellnum), sp_num, spells);
			// act(buf,FALSE,ch,0,ch->get_fighting(),TO_VICT);
			if (spell_info[spellnum].routines & NPC_DAMAGE_PC_MINHP)
			{
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_target(ch);
			}
			else if (spell_info[spellnum].routines & NPC_DAMAGE_PC)
			{
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_target(ch);
			}
			else if (spell_info[spellnum].routines & NPC_AFFECT_PC_CASTER)
			{
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_opp_caster(ch);
			}
			else if (spell_info[spellnum].routines & NPC_AFFECT_PC)
			{
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
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
	}

	if (spellnum && victim)  	// Is this object spell ?
	{
		item = ch->carrying;
		while (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& !(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))
			&& item
			&& GET_RACE(ch) == NPC_RACE_HUMAN)
		{
			switch (GET_OBJ_TYPE(item))
			{
			case OBJ_DATA::ITEM_WAND:
			case OBJ_DATA::ITEM_STAFF:
				if (GET_OBJ_VAL(item, 2) > 0
					&& GET_OBJ_VAL(item, 3) == spellnum)
				{
					mag_objectmagic(ch, item, GET_NAME(victim));
					return;
				}
				break;

			case OBJ_DATA::ITEM_POTION:
				for (int i = 1; i <= 3; i++)
				{
					if (GET_OBJ_VAL(item, i) == spellnum)
					{
						if (ch != victim)
						{
							obj_from_char(item);
							act("$n передал$g $o3 $N2.", FALSE, ch, item, victim, TO_ROOM | TO_ARENA_LISTEN);
							obj_to_char(item, victim);
						}
						else
						{
							victim = ch;
						}
						mag_objectmagic(victim, item, GET_NAME(victim));
						return;
					}
				}
				break;

			case OBJ_DATA::ITEM_SCROLL:
				for (int i = 1; i <= 3; i++)
				{
					if (GET_OBJ_VAL(item, i) == spellnum)
					{
						mag_objectmagic(ch, item, GET_NAME(victim));
						return;
					}
				}
				break;

			default:
				break;
			}

			item = item->get_next_content();
		}

		cast_spell(ch, victim, 0, NULL, spellnum, spellnum);
	}
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)

#define	MAY_ACT(ch)	(!(AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT) || GET_MOB_HOLD(ch) || GET_WAIT(ch)))

void summon_mob_helpers(CHAR_DATA *ch)
{
	for (struct helper_data_type *helpee = GET_HELPER(ch);
		helpee; helpee = helpee->next_helper)
	{
		// Start_fight_mtrigger using inside this loop
		// So we have to iterate on copy list
		character_list.foreach_on_copy([&] (const CHAR_DATA::shared_ptr& vict)
		{
			if (!IS_NPC(vict)
				|| GET_MOB_VNUM(vict) != helpee->mob_vnum
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_BLIND)
				|| GET_WAIT(vict) > 0
				|| GET_POS(vict) < POS_STANDING
				|| IN_ROOM(vict) == NOWHERE
				|| vict->get_fighting())
			{
				return;
			}
			if (GET_RACE(ch) == NPC_RACE_HUMAN)
			{
				act("$n воззвал$g : \"На помощь, мои верные соратники!\"",
					FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			}
			if (IN_ROOM(vict) != ch->in_room)
			{
				char_from_room(vict);
				char_to_room(vict, ch->in_room);
				act("$n прибыл$g на зов и вступил$g в битву на стороне $N1.",
					FALSE, vict.get(), 0, ch, TO_ROOM | TO_ARENA_LISTEN);
			}
			else
			{
				act("$n вступил$g в битву на стороне $N1.",
					FALSE, vict.get(), 0, ch, TO_ROOM | TO_ARENA_LISTEN);
			}
			if (MAY_ATTACK(vict))
			{
				set_fighting(vict, ch->get_fighting());
			}
		});
	}
}

void check_mob_helpers()
{
	for (CHAR_DATA *ch = combat_list; ch; ch = next_combat_list)
	{
		next_combat_list = ch->next_fighting;
		// Extract battler if no opponent
		if (ch->get_fighting() == NULL
			|| ch->in_room != IN_ROOM(ch->get_fighting())
			|| ch->in_room == NOWHERE)
		{
			stop_fighting(ch, TRUE);
			continue;
		}
		if (GET_MOB_HOLD(ch)
			|| !IS_NPC(ch)
			|| GET_WAIT(ch) > 0
			|| GET_POS(ch) < POS_FIGHTING
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)
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
		if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
			&& MOB_FLAGGED(k->follower, MOB_ANGEL)
			&& !k->follower->get_fighting()
			&& IN_ROOM(k->follower) == ch->in_room
			&& CAN_SEE(k->follower, ch)
			&& AWAKE(k->follower)
			&& MAY_ACT(k->follower)
			&& GET_POS(k->follower) >= POS_FIGHTING)
		{
			for (const auto vict : world[ch->in_room]->people)
			{
				if (vict->get_fighting() == ch
					&& vict != ch
					&& vict != k->follower)
				{
					if (k->follower->get_skill(SKILL_RESCUE))
					{
						go_rescue(k->follower, ch, vict);
					}

					break;
				}
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
	if (do_this <= GET_LIKES(ch) && ch->get_skill(SKILL_PARRY))
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

int calc_initiative(CHAR_DATA *ch, bool mode)
{
	int initiative = size_app[GET_POS_SIZE(ch)].initiative;
	if (mode) //Добавим булевую переменную, чтобы счет все выдавал постоянное значение, а не каждый раз рандом
	{
		int i = number(1, 10);
		if (i == 10)
			initiative -= 1;
		else
			initiative += i;
	};

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
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLOW))
		initiative -= 10;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HASTE))
		initiative += 10;
	if (GET_WAIT(ch) > 0)
		initiative -= 1;
	if (calc_leadership(ch))
		initiative += 5;
	if (GET_AF_BATTLE(ch, EAF_SLOW))
		initiative = 1;

	//initiative = MAX(initiative, 1); //Почему инициатива не может быть отрицательной?
	return initiative;
}

void using_mob_skills(CHAR_DATA *ch)
{
	ESkill sk_num = SKILL_INVALID;
	for (int sk_use = GET_REAL_INT(ch); MAY_LIKES(ch) && sk_use > 0; sk_use--)
	{
		int do_this = number(0, 100);
		if (do_this > GET_LIKES(ch))
		{
			continue;
		}

		do_this = number(0, 100);
		if (do_this < 10)
		{
			sk_num = SKILL_BASH;
		}
		else if (do_this < 20)
		{
			sk_num = SKILL_DISARM;
		}
		else if (do_this < 30)
		{
			sk_num = SKILL_KICK;
		}
		else if (do_this < 40)
		{
			sk_num = SKILL_PROTECT;
		}
		else if (do_this < 50)
		{
			sk_num = SKILL_RESCUE;
		}
		else if (do_this < 60 && !ch->get_touching())
		{
			sk_num = SKILL_TOUCH;
		}
		else if (do_this < 70)
		{
			sk_num = SKILL_CHOPOFF;
		}
		else if (do_this < 80)
		{
			sk_num = SKILL_THROW;
		}
		else
		{
			sk_num = SKILL_BASH;
		}

		if (ch->get_skill(sk_num) <= 0)
		{
			sk_num = SKILL_INVALID;
		}

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
			for (const auto vict : world[ch->in_room]->people)
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
				for (const auto vict : world[ch->in_room]->people)
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
		if (!(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))
			&& ch->has_master()
			&& (sk_num == SKILL_RESCUE || sk_num == SKILL_PROTECT))
		{
			CHAR_DATA *caster = 0, *damager = 0;
			int dumb_mob = (int)(GET_REAL_INT(ch) < number(5, 20));

			for (const auto attacker : world[ch->in_room]->people)
			{
				CHAR_DATA *vict = attacker->get_fighting();	// выяснение жертвы
				if (!vict	// жертвы нет
					|| (!IS_NPC(vict) // жертва - не моб
						|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
						|| AFF_FLAGGED(vict, EAffectFlag::AFF_HELPER))
					|| (IS_NPC(attacker)
						&& !(AFF_FLAGGED(attacker, EAffectFlag::AFF_CHARM)
							&& attacker->has_master()
							&& !IS_NPC(attacker->get_master()))
						&& !(MOB_FLAGGED(attacker, MOB_GHOST)
							&& attacker->has_master()
							&& !IS_NPC(attacker->get_master()))
						&& !(MOB_FLAGGED(attacker, MOB_ANGEL)
							&& attacker->has_master()
							&& !IS_NPC(attacker->get_master())))
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
				caster = find_target(ch);
				damager = find_target(ch);
				for (const auto vict : world[ch->in_room]->people)
				{
					if ((IS_NPC(vict) && !AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
						|| !vict->get_fighting())
					{
						continue;
					}
					if ((AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD) && GET_POS(vict) < POS_FIGHTING)
						|| (IS_CASTER(vict)
							&& (AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
							|| AFF_FLAGGED(vict, EAffectFlag::AFF_SILENCE)
							|| AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)
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
//send_to_char(caster, "Баш предфункция\r\n");
//sprintf(buf, "%s башат предфункция\r\n",GET_NAME(caster));
//mudlog(buf, LGH, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
					if (GET_POS(caster) >= POS_FIGHTING
						|| calculate_skill(ch, SKILL_BASH, caster) > number(50, 80))
					{
						sk_use = 0;
						go_bash(ch, caster);
					}
				}
				else
				{ 
//send_to_char(caster, "Подножка предфункция\r\n");
//sprintf(buf, "%s подсекают предфункция\r\n",GET_NAME(caster));
//                mudlog(buf, LGH, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);

					if (GET_POS(caster) >= POS_FIGHTING
						|| calculate_skill(ch, SKILL_CHOPOFF, caster) > number(50, 80))
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
						|| calculate_skill(ch, SKILL_BASH, damager) > number(50, 80))
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
						|| calculate_skill(ch, SKILL_CHOPOFF, damager) > number(50, 80))
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
	for (const auto i : world[ch->in_room]->people)
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
		if (ch->in_room == NOWHERE)
			continue;

		CLR_AF_BATTLE(ch, EAF_FIRST);
		CLR_AF_BATTLE(ch, EAF_SECOND);
		CLR_AF_BATTLE(ch, EAF_USEDLEFT);
		CLR_AF_BATTLE(ch, EAF_USEDRIGHT);
		CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
                CLR_AF_BATTLE(ch, EAF_DEVIATE);

		if (GET_AF_BATTLE(ch, EAF_SLEEP))
			affect_from_char(ch, SPELL_SLEEP);

		if (GET_AF_BATTLE(ch, EAF_BLOCK))
		{
			CLR_AF_BATTLE(ch, EAF_BLOCK);
			if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}

//		if (GET_AF_BATTLE(ch, EAF_DEVIATE))
//		{
//			CLR_AF_BATTLE(ch, EAF_DEVIATE);
//			if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
//				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
//		}

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

	switch (ch->get_extra_attack_mode())
	{
	case EXTRA_ATTACK_THROW:
		go_throw(ch, ch->get_extra_victim());
		used = true;
		break;
	case EXTRA_ATTACK_BASH:
		go_bash(ch, ch->get_extra_victim());
		used = true;
		break;
	case EXTRA_ATTACK_KICK:
		go_kick(ch, ch->get_extra_victim());
		used = true;
		break;
	case EXTRA_ATTACK_CHOPOFF:
		go_chopoff(ch, ch->get_extra_victim());
		used = true;
		break;
	case EXTRA_ATTACK_DISARM:
		go_disarm(ch, ch->get_extra_victim());
		used = true;
		break;
	case EXTRA_ATTACK_CUT_SHORTS:
	case EXTRA_ATTACK_CUT_PICK:
        go_cut_shorts(ch, ch->get_extra_victim());
        used = true;
        break;
    default:
        used = false;
        break;
	}

	return used;
}

void process_npc_attack(CHAR_DATA *ch)
{
	// if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
	//    continue;

	// Вызываем триггер перед началом боевых моба (магических или физических)
	
	if (!fight_mtrigger(ch))
		return;

	// переключение
	if (MAY_LIKES(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_NOT_SWITCH)
		&& GET_REAL_INT(ch) > number(15, 25))
	{
		perform_mob_switch(ch);
	}

	// Cast spells
	if (MAY_LIKES(ch))
		mob_casting(ch);

	if (!ch->get_fighting()
		|| ch->in_room != IN_ROOM(ch->get_fighting())
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
			// mob_casting мог от зеркала отразиться
		||	AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| !AWAKE(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT))
	{
		return;
	}

	if ((AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL))
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)
		&& ch->has_master()
		// && !IS_NPC(ch->master)
		&& CAN_SEE(ch, ch->get_master())
		&& ch->in_room == IN_ROOM(ch->get_master())
		&& AWAKE(ch)
		&& MAY_ACT(ch)
		&& GET_POS(ch) >= POS_FIGHTING)
	{
		for (const auto vict : world[ch->in_room]->people)
		{
			if (vict->get_fighting() == ch->get_master()
				&& vict != ch
				&& vict != ch->get_master())
			{
				if (ch->get_skill(SKILL_RESCUE))
				{
					go_rescue(ch, ch->get_master(), vict);
				}
				else if (ch->get_skill(SKILL_PROTECT))
				{
					go_protect(ch, ch->get_master());
				}

				break;
			}
		}
	}
	else if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		//* применение скилов
		using_mob_skills(ch);
	}

	if (!ch->get_fighting()
		|| ch->in_room != IN_ROOM(ch->get_fighting()))
	{
		return;
	}

	//**** удар основным оружием или рукой
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT))
	{
		exthit(ch, TYPE_UNDEFINED, RIGHT_WEAPON);
	}

	//**** экстраатаки
	for (int i = 1; i <= ch->mob_specials.ExtraAttack; i++)
	{
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
			|| (i == 1 && AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)))
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
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
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
		ch->set_extra_attack(EXTRA_ATTACK_UNUSED, 0);
		if (INITIATIVE(ch) > min_init)
		{
			INITIATIVE(ch)--;
			return;
		}
	}

	if (!ch->get_fighting()
		|| ch->in_room != IN_ROOM(ch->get_fighting()))
	{
		return;
	}

	//**** удар основным оружием или рукой
	if (GET_AF_BATTLE(ch, EAF_FIRST))
	{
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT)
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
// двуруч при скиллкапе 160 и перке любимоедвуруч дает допатаку 20%
		if (GET_EQ(ch,WEAR_BOTHS)&& can_use_feat(ch, BOTHHANDS_FOCUS_FEAT) && (GET_OBJ_SKILL(GET_EQ(ch,WEAR_BOTHS)) == SKILL_BOTHHANDS))
		{
			if (ch->get_skill(SKILL_BOTHHANDS) > (number(1,800)))
				hit(ch, ch->get_fighting(), TYPE_UNDEFINED, RIGHT_WEAPON);
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
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON
		&& GET_AF_BATTLE(ch, EAF_SECOND)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)
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
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)
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
	if (ch->in_room == NOWHERE)
		return false;

	if (GET_MOB_HOLD(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT))
	{
		try_angel_rescue(ch);
		return false;
	}

	// Mobs stand up and players sit
	if (GET_POS(ch) < POS_FIGHTING
		&& GET_POS(ch) > POS_STUNNED
		&& GET_WAIT(ch) <= 0
		&& !GET_MOB_HOLD(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
	{
		stand_up_or_sit(ch);
	}

	// For NPC without lags and charms make it likes
	if (IS_NPC(ch) && MAY_LIKES(ch))  	// Get weapon from room
	{
		//edited by WorM 2010.09.03 добавил немного логики мобам в бою если
		// у моба есть что-то в инве то он может
		//переодется в это что-то если посчитает что это что-то ему лучше подходит
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
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
	int max_init = -100, min_init = 100;

	//* суммон хелперов
	check_mob_helpers();

	// храним список писей, которым надо показать состояние группы по msdp 
	std::unordered_set<CHAR_DATA *> msdp_report_chars;

	//* действия до раунда и расчет инициативы
	for (CHAR_DATA *ch = combat_list; ch; ch = next_combat_list)
	{
		next_combat_list = ch->next_fighting;

		if (ch->desc)
		{
			msdp_report_chars.insert(ch);
		}
		else if (ch->has_master()
			&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				|| MOB_FLAGGED(ch, MOB_ANGEL)
				|| MOB_FLAGGED(ch, MOB_GHOST)))
		{
			auto master = ch->get_master();
			if (master->desc
				&& !master->get_fighting()
				&& master->in_room == ch->in_room)
			{
				msdp_report_chars.insert(master);
			}
		}

		if (!stuff_before_round(ch))
		{
			continue;
		}

		const int initiative = calc_initiative(ch, true);
		if (initiative == 0)
		{
			INITIATIVE(ch) = -100; //Если кубик выпал в 0 - бей последним шанс 1 из 201
			min_init = MIN(min_init, -100);
		}
		else
		{
			INITIATIVE(ch) = initiative;
		}

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
			if (INITIATIVE(ch) != initiative || ch->in_room == NOWHERE)
			{
				continue;
			}

			// If mob cast 'hold' when initiative setted
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
				|| !AWAKE(ch))
			{
				continue;
			}

			// If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
			if (!ch->get_fighting()
				|| ch->in_room != IN_ROOM(ch->get_fighting()))
			{
				continue;
			}

			if (initiative == 0) //везде в стоп-файтах ставится инициатива равная 0, убираем двойную атаку
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

	for (auto d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) == CON_PLAYING && d->character)
		{
			for (const auto& ch : msdp_report_chars)
			{
				if (same_group(ch, d->character.get()) && ch->in_room == d->character->in_room)
				{
					msdp_report_chars.insert(d->character.get());
					break;
				}
			}
		}
	}
	
	// покажем группу по msdp
	// проверка на поддержку протокола есть в методе msdp_report
	for (const auto& ch: msdp_report_chars)
	{
		if (!ch->purged())
		{
			ch->desc->msdp_report(msdp::constants::GROUP);
		}
	}
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
	{
		return return_value;
	}

// translating pointers from charimces to their leaders
	if (IS_NPC(ch)
		&& ch->has_master()
		&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(ch, MOB_ANGEL)
			|| MOB_FLAGGED(ch, MOB_GHOST)
			|| IS_HORSE(ch)))
	{
		ch = ch->get_master();
	}

	if (IS_NPC(victim)
		&& victim->has_master()
		&& (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(victim, MOB_ANGEL)
			|| MOB_FLAGGED(victim, MOB_GHOST)
			|| IS_HORSE(victim)))
	{
		victim = victim->get_master();
	}

	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->has_master())
	{
		if (IS_NPC(cleader)
			&& !AFF_FLAGGED(cleader, EAffectFlag::AFF_CHARM)
			&& !(MOB_FLAGGED(cleader, MOB_ANGEL)||MOB_FLAGGED(cleader, MOB_GHOST))
			&& !IS_HORSE(cleader))
		{
			break;
		}
		cleader = cleader->get_master();
	}

	while (vleader->has_master())
	{
		if (IS_NPC(vleader)
			&& !AFF_FLAGGED(vleader, EAffectFlag::AFF_CHARM)
			&& !(MOB_FLAGGED(vleader, MOB_ANGEL)||MOB_FLAGGED(vleader, MOB_GHOST))
			&& !IS_HORSE(vleader))
		{
			break;
		}
		vleader = vleader->get_master();
	}

	if (cleader != vleader)
	{
		return return_value;
	}

// finding closest to the leader nongrouped agressor
// it cannot be a charmice
	while (ch->has_master()
		&& ch->get_master()->has_master())
	{
		if (!AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(ch->get_master()))
		{
			ch = ch->get_master();
			continue;
		}
		else if (IS_NPC(ch->get_master())
			&& !AFF_FLAGGED(ch->get_master()->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(ch->get_master()->get_master())
			&& ch->get_master()->get_master()->get_master())
		{
			ch = ch->get_master()->get_master();
			continue;
		}
		else
		{
			break;
		}
	}

// finding closest to the leader nongrouped victim
// it cannot be a charmice
	while (victim->has_master()
		&& victim->get_master()->has_master())
	{
		if (!AFF_FLAGGED(victim->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(victim->get_master()))
		{
			victim = victim->get_master();
			continue;
		}
		else if (IS_NPC(victim->get_master())
			&& !AFF_FLAGGED(victim->get_master()->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(victim->get_master()->get_master())
			&& victim->get_master()->get_master()->has_master())
		{
			victim = victim->get_master()->get_master();
			continue;
		}
		else
		{
			break;
		}
	}
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)
		|| cleader == victim)
	{
		stop_follower(ch, SF_EMPTY);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, EAffectFlag::AFF_GROUP)
		|| vleader == ch)
	{
		stop_follower(victim, SF_EMPTY);
		return_value |= 2;
	}
	return return_value;
}

int calc_leadership(CHAR_DATA * ch)
{
	int prob, percent;
	CHAR_DATA *leader = 0;

	if (IS_NPC(ch)
		|| !AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)
		|| (!ch->has_master()
			&& !ch->followers))
	{
		return FALSE;
	}

	if (ch->has_master())
	{
		if (IN_ROOM(ch) != IN_ROOM(ch->get_master()))
		{
			return FALSE;
		}
		leader = ch->get_master();
	}
	else
	{
		leader = ch;
	}

	if (!leader->get_skill(SKILL_LEADERSHIP))
	{
		return (FALSE);
	}

	percent = number(1, 101);
	prob = calculate_skill(leader, SKILL_LEADERSHIP, 0);
	if (percent > prob)
	{
		return (FALSE);
	}
	else
	{
		return (TRUE);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
