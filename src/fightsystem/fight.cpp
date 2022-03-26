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

#include "game_skills/bash.h"
#include "game_skills/kick.h"
#include "game_skills/chopoff.h"
#include "game_skills/disarm.h"
#include "game_skills/throw.h"
#include "game_skills/expendientcut.h"
#include "game_skills/protect.h"
#include "game_skills/ironwind.h"

#include "assist.h"
#include "entities/world_characters.h"
#include "fight_hit.h"
#include "fightsystem/mobact.h"
#include "handler.h"
#include "color.h"
#include "utils/random.h"
#include "entities/char_player.h"
#include "game_magic/magic.h"
#include "olc/olc.h"
#include "msdp/msdp_constants.h"
#include "game_magic/magic_items.h"

// Structures
CharData *combat_list = nullptr;    // head of l-list of fighting entities
CharData *next_combat_list = nullptr;

extern int r_helled_start_room;
extern MobRaceListType mobraces_list;
extern CharData *mob_proto;
extern DescriptorData *descriptor_list;
// External procedures
// void do_assist(CharacterData *ch, char *argument, int cmd, int subcmd);
void battle_affect_update(CharData *ch);
void go_rescue(CharData *ch, CharData *vict, CharData *tmp_ch);
void go_touch(CharData *ch, CharData *vict);
int npc_battle_scavenge(CharData *ch);
void npc_wield(CharData *ch);
void npc_armor(CharData *ch);

void go_autoassist(CharData *ch) {
	struct Follower *k;
	struct Follower *d;
	CharData *ch_lider = 0;
	if (ch->has_master()) {
		ch_lider = ch->get_master();
	} else {
		ch_lider = ch;    // Создаем ссылку на лидера
	}

	buf2[0] = '\0';
	for (k = ch_lider->followers; k; k = k->next) {
		if (PRF_FLAGGED(k->ch, PRF_AUTOASSIST) &&
			(IN_ROOM(k->ch) == IN_ROOM(ch)) && !k->ch->get_fighting() &&
			(GET_POS(k->ch) == EPosition::kStand) && !CHECK_WAIT(k->ch)) {
			// Здесь проверяем на кастеров
			if (IS_CASTER(k->ch)) {
				// здесь проходим по чармисам кастера, и если находим их, то вписываем в драку
				for (d = k->ch->followers; d; d = d->next)
					if ((IN_ROOM(d->ch) == IN_ROOM(ch)) && !d->ch->get_fighting() &&
						(GET_POS(d->ch) == EPosition::kStand) && !CHECK_WAIT(d->ch))
						do_assist(d->ch, buf2, 0, 0);
			} else {
				do_assist(k->ch, buf2, 0, 0);
			}
		}
	}
}


// The Fight related routines

// Добавил проверку на лаг, чтобы правильно работали Каменное проклятие и
// Круг пустоты, ибо позицию делают меньше чем поз_станнед.
void update_pos(CharData *victim) {
	if ((GET_HIT(victim) > 0) && (GET_POS(victim) > EPosition::kStun))
		GET_POS(victim) = GET_POS(victim);
	else if (GET_HIT(victim) > 0 && GET_WAIT(victim) <= 0 && !GET_MOB_HOLD(victim))
		GET_POS(victim) = EPosition::kStand;
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = EPosition::kDead;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = EPosition::kPerish;
	else if (GET_HIT(victim) <= -3)
		GET_POS(victim) = EPosition::kIncap;
	else if (GET_POS(victim) == EPosition::kIncap && GET_WAIT(victim) > 0)
		GET_POS(victim) = EPosition::kIncap;
	else
		GET_POS(victim) = EPosition::kStun;

	if (AFF_FLAGGED(victim, EAffectFlag::AFF_SLEEP) && GET_POS(victim) != EPosition::kSleep)
		affect_from_char(victim, kSpellSleep);

	// поплохело седоку или лошади - сбрасываем седока
	if (victim->ahorse() && GET_POS(victim) < EPosition::kFight)
		victim->drop_from_horse();
	if (IS_HORSE(victim) && GET_POS(victim) < EPosition::kFight && victim->get_master()->ahorse())
		victim->drop_from_horse();
}

void set_battle_pos(CharData *ch) {
	switch (GET_POS(ch)) {
		case EPosition::kStand: GET_POS(ch) = EPosition::kFight;
			break;
		case EPosition::kRest:
		case EPosition::kSit:
		case EPosition::kSleep:
			if (GET_WAIT(ch) <= 0 &&
				!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
				if (IS_NPC(ch)) {
					act("$n поднял$u.", false, ch, 0, 0, kToRoom | kToArenaListen);
					GET_POS(ch) = EPosition::kFight;
				} else if (GET_POS(ch) == EPosition::kSleep) {
					act("Вы проснулись и сели.", false, ch, 0, 0, kToChar);
					act("$n проснул$u и сел$g.", false, ch, 0, 0, kToRoom | kToArenaListen);
					GET_POS(ch) = EPosition::kSit;
				} else if (GET_POS(ch) == EPosition::kRest) {
					act("Вы прекратили отдых и сели.", false, ch, 0, 0, kToChar);
					act("$n прекратил$g отдых и сел$g.", false, ch, 0, 0, kToRoom | kToArenaListen);
					GET_POS(ch) = EPosition::kSit;
				}
			}
			break;
		default:
			break;
	}
}

void restore_battle_pos(CharData *ch) {
	switch (GET_POS(ch)) {
		case EPosition::kFight: GET_POS(ch) = EPosition::kStand;
			break;
		case EPosition::kRest:
		case EPosition::kSit:
		case EPosition::kSleep:
			if (IS_NPC(ch) &&
				GET_WAIT(ch) <= 0 &&
				!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
				act("$n поднял$u.", false, ch, 0, 0, kToRoom | kToArenaListen);
				GET_POS(ch) = EPosition::kStand;
			}
			break;
		default:
			break;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
		GET_POS(ch) = EPosition::kSleep;
}

// start one char fighting another (yes, it is horrible, I know... )
void set_fighting(CharData *ch, CharData *vict) {
	if (ch == vict)
		return;
	if (ch->get_fighting()) {
		log("SYSERR: set_fighting(%s->%s) when already fighting(%s)...",
			GET_NAME(ch), GET_NAME(vict), GET_NAME(ch->get_fighting()));
		// core_dump();
		return;
	}

	if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT)) || (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOFIGHT)))
		return;

	// if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
	//    return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BANDAGE)) {
		send_to_char("Перевязка была прервана!\r\n", ch);
		affect_from_char(ch, kSpellBandage);
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_RECALL_SPELLS)) {
		send_to_char("Вы забыли о концентрации и ринулись в бой!\r\n", ch);
		affect_from_char(ch, kSpellRecallSpells);
	}

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
		affect_from_char(ch, kSpellSleep);

	ch->set_fighting(vict);

	NUL_AF_BATTLE(ch);
	//Polud вступление в битву не мешает прикрывать
	if (ch->get_protecting())
		SET_AF_BATTLE(ch, kEafProtect);
	ch->set_touching(0);
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) = 0;
	ch->set_extra_attack(kExtraAttackUnused, 0);
	set_battle_pos(ch);

	// если до начала боя на мобе есть лаг, то мы его выравниваем до целых
	// раундов в большую сторону (для подножки, должно давать чару зазор в две
	// секунды после подножки, чтобы моб всеравно встал только на 3й раунд)
	if (IS_NPC(ch) && GET_WAIT(ch) > 0) {
//		div_t tmp = div(static_cast<const int>(ch->get_wait()), static_cast<const int>(kPulseViolence));
		auto tmp = div(ch->get_wait(), kPulseViolence);
		if (tmp.rem > 0) {
			WAIT_STATE(ch, (tmp.quot + 1) * kPulseViolence);
		}
	}
	if (!IS_NPC(ch) && (!ch->get_skill(ESkill::kAwake))) {
		PRF_FLAGS(ch).unset(PRF_AWAKE);
	}

	if (!IS_NPC(ch) && (!ch->get_skill(ESkill::kPunctual))) {
		PRF_FLAGS(ch).unset(PRF_PUNCTUAL);
	}

	// Set combat style
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_COURAGE) && !AFF_FLAGGED(ch, EAffectFlag::AFF_DRUNKED)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT)) {
		if (PRF_FLAGGED(ch, PRF_PUNCTUAL))
			SET_AF_BATTLE(ch, kEafPunctual);
		else if (PRF_FLAGGED(ch, PRF_AWAKE))
			SET_AF_BATTLE(ch, kEafAwake);
	}

	if (can_use_feat(ch, DEFENDER_FEAT) && ch->get_skill(ESkill::kShieldBlock)) {
		SET_AF_BATTLE(ch, kEafAutoblock);
	}


//  check_killer(ch, vict);
}

// remove a char from the list of fighting entities
void stop_fighting(CharData *ch, int switch_others) {
	CharData *temp, *found;

	if (ch == next_combat_list)
		next_combat_list = ch->next_fighting;

	REMOVE_FROM_LIST(ch, combat_list, [](auto list) -> auto & { return list->next_fighting; });
	//Попробуем сперва очистить ссылку у врага, потом уже у самой цели
	ch->next_fighting = nullptr;
	if (ch->last_comm != nullptr)
		free(ch->last_comm);
	ch->last_comm = nullptr;
	ch->set_touching(0);
	ch->set_fighting(0);
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) = 0;
	ch->set_extra_attack(kExtraAttackUnused, 0);
	ch->set_cast(0, 0, 0, 0, 0);
	restore_battle_pos(ch);
	NUL_AF_BATTLE(ch);
	DpsSystem::check_round(ch);
	StopFightParameters params(ch); //готовим параметры нужного типа и вызываем шаблонную функцию
	handle_affects(params);
	if (switch_others != 2) {
		for (temp = combat_list; temp; temp = temp->next_fighting) {
			if (temp->get_touching() == ch) {
				temp->set_touching(0);
				CLR_AF_BATTLE(temp, kEafTouch);
			}
			if (temp->get_extra_victim() == ch)
				temp->set_extra_attack(kExtraAttackUnused, 0);
			if (temp->get_cast_char() == ch)
				temp->set_cast(0, 0, 0, 0, 0);
			if (temp->get_fighting() == ch && switch_others) {
				log("[Stop fighting] %s : Change victim for fighting", GET_NAME(temp));
				for (found = combat_list; found; found = found->next_fighting)
					if (found != ch && found->get_fighting() == temp) {
						act("Вы переключили свое внимание на $N3.", false, temp, 0, found, kToChar);
						temp->set_fighting(found);
						break;
					}
				if (!found) {
					stop_fighting(temp, false);
				};
			}
		}

		update_pos(ch);
		// проверка скилла "железный ветер" - снимаем флаг по окончанию боя
		if ((ch->get_fighting() == nullptr) && PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
			PRF_FLAGS(ch).unset(PRF_IRON_WIND);
			if (GET_POS(ch) > EPosition::kIncap) {
				send_to_char("Безумие боя отпустило вас, и враз навалилась усталость...\r\n", ch);
				act("$n шумно выдохнул$g и остановил$u, переводя дух после боя.",
					false,
					ch,
					0,
					0,
					kToRoom | kToArenaListen);
			};
		};
	};
}

int GET_MAXDAMAGE(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD))
		return 0;
	else
		return GET_DAMAGE(ch);
}

int GET_MAXCASTER(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD) || AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)
		|| GET_WAIT(ch) > 0)
		return 0;
	else
		return IS_IMMORTAL(ch) ? 1 : GET_CASTER(ch);
}

#define GET_HP_PERC(ch) ((int)(GET_HIT(ch) * 100 / GET_MAX_HIT(ch)))
#define POOR_DAMAGE  15
#define POOR_CASTER  5
#define MAX_PROBES   0

int in_same_battle(CharData *npc, CharData *pc, int opponent) {
	int ch_friend_npc, ch_friend_pc, vict_friend_npc, vict_friend_pc;
	CharData *vict, *npc_master, *pc_master, *ch_master, *vict_master;

	if (npc == pc)
		return (!opponent);
	if (npc->get_fighting() == pc)    // NPC fight PC - opponent
		return (opponent);
	if (pc->get_fighting() == npc)    // PC fight NPC - opponent
		return (opponent);
	if (npc->get_fighting() && npc->get_fighting() == pc->get_fighting())
		return (!opponent);    // Fight same victim - friend
	if (AFF_FLAGGED(pc, EAffectFlag::AFF_HORSE) || AFF_FLAGGED(pc, EAffectFlag::AFF_CHARM))
		return (opponent);

	npc_master = npc->has_master() ? npc->get_master() : npc;
	pc_master = pc->has_master() ? pc->get_master() : pc;

	for (const auto
			ch : world[IN_ROOM(npc)]->people)    // <- Anton Gorev (2017-07-11): changed loop to loop over the people in room (instead of all world)
	{
		if (!ch->get_fighting()) {
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
		if (ch->get_fighting() == pc && ch_friend_npc)    // Friend NPC fight PC - opponent
			return (opponent);
		if (pc->get_fighting() == ch && ch_friend_npc)    // PC fight friend NPC - opponent
			return (opponent);
		if (npc->get_fighting() == ch && ch_friend_pc)    // NPC fight friend PC - opponent
			return (opponent);
		if (ch->get_fighting() == npc && ch_friend_pc)    // Friend PC fight NPC - opponent
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
			return (opponent);    // Friend NPC fight friend PC - opponent
		if (ch_friend_pc && vict_friend_npc)
			return (opponent);    // Friend PC fight friend NPC - opponent
	}

	return (!opponent);
}

CharData *find_friend_cure(CharData *caster, int spellnum) {
	CharData *victim = nullptr;
	int vict_val = 0, AFF_USED = 0;
	switch (spellnum) {
		case kSpellCureLight: AFF_USED = 80;
			break;
		case kSpellCureSerious: AFF_USED = 70;
			break;
		case kSpellExtraHits:
		case kSpellCureCritic: AFF_USED = 50;
			break;
		case kSpellHeal:
		case kSpellGroupHeal: AFF_USED = 30;
			break;
	}

	if ((AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)
		|| MOB_FLAGGED(caster, MOB_GHOST) || MOB_FLAGGED(caster, MOB_PLAYER_SUMMON)) // (Кудояр)
		&& AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER)) {
		if (GET_HP_PERC(caster) < AFF_USED) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& caster->get_master()->get_fighting()
			&& GET_HP_PERC(caster->get_master()) < AFF_USED) {
			return caster->get_master();
		}
		return nullptr;
	}

	for (const auto vict : world[IN_ROOM(caster)]->people) {
		if (!IS_NPC(vict)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON)) // (Кудояр)
				&& vict->has_master()
				&& !IS_NPC(vict->get_master()))
			|| !CAN_SEE(caster, vict)) {
			continue;
		}

		if (!vict->get_fighting() && !MOB_FLAGGED(vict, MOB_HELPER)) {
			continue;
		}

		if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict))) {
			victim = vict;
			vict_val = GET_HP_PERC(vict);
			if (GET_REAL_INT(caster) < number(10, 20)) {
				break;
			}
		}
	}
	return victim;
}

CharData *find_friend(CharData *caster, int spellnum) {
	CharData *victim = nullptr;
	int vict_val = 0, spellreal = -1;
	affects_list_t AFF_USED;
	switch (spellnum) {
		case kSpellCureBlind: AFF_USED.push_back(EAffectFlag::AFF_BLIND);
			break;

		case kSpellRemovePoison: AFF_USED.push_back(EAffectFlag::AFF_POISON);
			AFF_USED.push_back(EAffectFlag::AFF_SCOPOLIA_POISON);
			AFF_USED.push_back(EAffectFlag::AFF_BELENA_POISON);
			AFF_USED.push_back(EAffectFlag::AFF_DATURA_POISON);
			break;

		case kSpellRemoveHold: AFF_USED.push_back(EAffectFlag::AFF_HOLD);
			break;

		case kSpellRemoveCurse: AFF_USED.push_back(EAffectFlag::AFF_CURSE);
			break;

		case kSpellRemoveSilence: AFF_USED.push_back(EAffectFlag::AFF_SILENCE);
			break;
			
		case kSpellRemoveDeafness: AFF_USED.push_back(EAffectFlag::AFF_DEAFNESS);
			break;

		case kSpellCureFever: spellreal = kSpellFever;
			break;
	}
	if (AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER)
		&& (AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(caster, MOB_ANGEL) || MOB_FLAGGED(caster, MOB_GHOST) || MOB_FLAGGED(caster, MOB_PLAYER_SUMMON))) { //(Кудояр)
		if (caster->has_any_affect(AFF_USED)
			|| affected_by_spell(caster, spellreal)) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& (caster->get_master()->has_any_affect(AFF_USED)
				|| affected_by_spell(caster->get_master(), spellreal))) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (!AFF_USED.empty()) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if (!IS_NPC(vict)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON)) // (Кудояр)
					&& vict->get_master()
					&& !IS_NPC(vict->get_master()))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->has_any_affect(AFF_USED)) {
				continue;
			}

			if (!vict->get_fighting()
				&& !MOB_FLAGGED(vict, MOB_HELPER)) {
				continue;
			}

			if (!victim
				|| vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
				if (GET_REAL_INT(caster) < number(10, 20)) {
					break;
				}
			}
		}
	}

	return victim;
}

CharData *find_caster(CharData *caster, int spellnum) {
	CharData *victim = nullptr;
	int vict_val = 0, spellreal = -1;
	affects_list_t AFF_USED;
	switch (spellnum) {
		case kSpellCureBlind: AFF_USED.push_back(EAffectFlag::AFF_BLIND);
			break;
		case kSpellRemovePoison: AFF_USED.push_back(EAffectFlag::AFF_POISON);
			AFF_USED.push_back(EAffectFlag::AFF_SCOPOLIA_POISON);
			AFF_USED.push_back(EAffectFlag::AFF_BELENA_POISON);
			AFF_USED.push_back(EAffectFlag::AFF_DATURA_POISON);
			break;
		case kSpellRemoveHold: AFF_USED.push_back(EAffectFlag::AFF_HOLD);
			break;
		case kSpellRemoveCurse: AFF_USED.push_back(EAffectFlag::AFF_CURSE);
			break;
		case kSpellRemoveSilence: AFF_USED.push_back(EAffectFlag::AFF_SILENCE);
			break;
		case kSpellRemoveDeafness: AFF_USED.push_back(EAffectFlag::AFF_DEAFNESS);
			break;
		case kSpellCureFever: spellreal = kSpellFever;
			break;
	}

	if (AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER)
		&& (AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(caster, MOB_ANGEL) || MOB_FLAGGED(caster, MOB_GHOST) || MOB_FLAGGED(caster, MOB_PLAYER_SUMMON))) { // (Кудояр)
		if (caster->has_any_affect(AFF_USED)
			|| affected_by_spell(caster, spellreal)) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& (caster->get_master()->has_any_affect(AFF_USED)
				|| affected_by_spell(caster->get_master(), spellreal))) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (!AFF_USED.empty()) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if (!IS_NPC(vict)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON)) // (Кудояр)
					&& (vict->get_master() && !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->has_any_affect(AFF_USED)) {
				continue;
			}

			if (!vict->get_fighting()
				&& !MOB_FLAGGED(vict, MOB_HELPER)) {
				continue;
			}

			if (!victim
				|| vict_val < GET_MAXCASTER(vict)) {
				victim = vict;
				vict_val = GET_MAXCASTER(vict);
				if (GET_REAL_INT(caster) < number(10, 20)) {
					break;
				}
			}
		}
	}

	return victim;
}

CharData *find_affectee(CharData *caster, int spellnum) {
	CharData *victim = nullptr;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == kSpellGroupArmor)
		spellreal = kSpellArmor;
	else if (spellreal == kSpellGroupStrength)
		spellreal = kSpellStrength;
	else if (spellreal == kSpellGroupBless)
		spellreal = kSpellBless;
	else if (spellreal == kSpellGroupHaste)
		spellreal = kSpellHaste;
	else if (spellreal == kSpellGroupSanctuary)
		spellreal = kSpellSanctuary;
	else if (spellreal == kSpellGroupPrismaticAura)
		spellreal = kSpellPrismaticAura;
	else if (spellreal == kSpellSightOfDarkness)
		spellreal = kSpellInfravision;
	else if (spellreal == kSpellGroupSincerity)
		spellreal = kSpellDetectAlign;
	else if (spellreal == kSpellMagicalGaze)
		spellreal = kSpellDetectMagic;
	else if (spellreal == kSpellAllSeeingEye)
		spellreal = kSpellDetectInvis;
	else if (spellreal == kSpellEyeOfGods)
		spellreal = kSpellSenseLife;
	else if (spellreal == kSpellBreathingAtDepth)
		spellreal = kSpellWaterbreath;
	else if (spellreal == kSpellGeneralRecovery)
		spellreal = kSpellFastRegeneration;
	else if (spellreal == kSpellCommonMeal)
		spellreal = kSpellFullFeed;
	else if (spellreal == kSpellStoneWall)
		spellreal = kSpellStoneSkin;
	else if (spellreal == kSpellSnakeEyes)
		spellreal = kSpellDetectPoison;

	if ((AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)
		|| MOB_FLAGGED(caster, MOB_GHOST) || MOB_FLAGGED(caster, MOB_PLAYER_SUMMON)) && AFF_FLAGGED(caster, EAffectFlag::AFF_HELPER)) { // (Кудояр)
		if (!affected_by_spell(caster, spellreal)) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& caster->get_master()->get_fighting() && !affected_by_spell(caster->get_master(), spellreal)) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (GET_REAL_INT(caster) > number(5, 15)) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if (!IS_NPC(vict)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| ((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON)) // (Кудояр)
					&& vict->has_master()
					&& !IS_NPC(vict->get_master()))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->get_fighting()
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				|| affected_by_spell(vict, spellreal)) {
				continue;
			}

			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim
		&& !affected_by_spell(caster, spellreal)) {
		victim = caster;
	}

	return victim;
}

CharData *find_opp_affectee(CharData *caster, int spellnum) {
	CharData *victim = nullptr;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == kSpellPowerHold || spellreal == kSpellMassHold)
		spellreal = kSpellHold;
	else if (spellreal == kSpellPowerBlindness || spellreal == kSpellMassBlindness)
		spellreal = kSpellBlindness;
	else if (spellreal == kSpellPowerSilence || spellreal == kSpellMassSilence)
		spellreal = kSpellSllence;
	else if (spellreal == kSpellMassCurse)
		spellreal = kSpellCurse;
	else if (spellreal == kSpellMassSlow)
		spellreal = kSpellSlowdown;
	else if (spellreal == kSpellMassFailure)
		spellreal = kSpellFailure;
	else if (spellreal == kSpellSnare)
		spellreal = kSpellNoflee;

	if (GET_REAL_INT(caster) > number(10, 20)) {
		for (const auto vict : world[caster->in_room]->people) {
			if ((IS_NPC(vict)
				&& !((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON) // (Кудояр)
					|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
					&& vict->has_master()
					&& !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if ((!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true)))
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				|| affected_by_spell(vict, spellreal)) {
				continue;
			}
			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim
		&& caster->get_fighting()
		&& !affected_by_spell(caster->get_fighting(), spellreal)) {
		victim = caster->get_fighting();
	}

	return victim;
}

CharData *find_opp_caster(CharData *caster) {
	CharData *victim = nullptr;
	int vict_val = 0;

	for (const auto vict : world[IN_ROOM(caster)]->people) {
		if (IS_NPC(vict)
			&& !((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON)) // Кудояр
				&& vict->has_master()
				&& !IS_NPC(vict->get_master()))) {
			continue;
		}
		if ((!vict->get_fighting()
			&& (GET_REAL_INT(caster) < number(15, 25)
				|| !in_same_battle(caster, vict, true)))
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD) || AFF_FLAGGED(vict, EAffectFlag::AFF_SILENCE)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)
			|| (!CAN_SEE(caster, vict) && caster->get_fighting() != vict))
			continue;
		if (vict_val < GET_MAXCASTER(vict)) {
			victim = vict;
			vict_val = GET_MAXCASTER(vict);
		}
	}
	return (victim);
}

CharData *find_damagee(CharData *caster) {
	CharData *victim = nullptr;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20)) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if ((IS_NPC(vict)
				&& !((MOB_FLAGGED(vict, MOB_ANGEL)
					|| MOB_FLAGGED(vict, MOB_GHOST)
					|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
					&& vict->has_master()
					&& !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if ((!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true)))
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)) {
				continue;
			}

			if (GET_REAL_INT(caster) >= number(25, 30)) {
				if (!victim || vict_val < GET_MAXCASTER(vict)) {
					victim = vict;
					vict_val = GET_MAXCASTER(vict);
				}
			} else if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim) {
		victim = caster->get_fighting();
	}

	return victim;
}

CharData *find_target(CharData *ch) {
	CharData *currentVictim, *caster = nullptr, *best = nullptr;
	CharData *druid = nullptr, *cler = nullptr, *charmmage = nullptr;

	currentVictim = ch->get_fighting();

	int mobINT = GET_REAL_INT(ch);

	if (mobINT < kStupidMod) {
		return find_damagee(ch);
	}

	if (!currentVictim) {
		return nullptr;
	}

	if (IS_CASTER(currentVictim) && !IS_NPC(currentVictim)) {
		return currentVictim;
	}

	// проходим по всем чарам в комнате
	for (const auto vict : world[ch->in_room]->people) {
		if ((IS_NPC(vict) && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->get_fighting()
				&& find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
			|| PRF_FLAGGED(vict, PRF_NOHASSLE)
			|| !MAY_SEE(ch, ch, vict)) {
			continue;
		}

		if (!CAN_SEE(ch, vict)) {
			continue;
		}

		if (GET_CLASS(vict) == ECharClass::kMagus) {
			druid = vict;
			caster = vict;
			continue;
		}

		if (GET_CLASS(vict) == ECharClass::kSorcerer) {
			cler = vict;
			caster = vict;
			continue;
		}

		if (GET_CLASS(vict) == ECharClass::kCharmer) {
			charmmage = vict;
			caster = vict;
			continue;
		}

		if (GET_HIT(vict) <= kCharacterHPForMobPriorityAttack) {
			return vict;
		}

		if (IS_CASTER(vict)) {
			caster = vict;
			continue;
		}

		best = vict;
	}

	if (!best) {
		best = currentVictim;
	}

	if (mobINT < kMiddleAI) {
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

	if (mobINT < kHighAI) {
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

CharData *find_minhp(CharData *caster) {
	CharData *victim = nullptr;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20)) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if ((IS_NPC(vict)
				&& !((MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || MOB_FLAGGED(vict, MOB_PLAYER_SUMMON) // (Кудояр)
					|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
					&& vict->has_master()
					&& !IS_NPC(vict->get_master())))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->get_fighting()
				&& (GET_REAL_INT(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true))) {
				continue;
			}

			if (!victim || vict_val > GET_HIT(vict)) {
				victim = vict;
				vict_val = GET_HIT(vict);
			}
		}
	}

	if (!victim) {
		victim = caster->get_fighting();
	}

	return victim;
}

CharData *find_cure(CharData *caster, CharData *patient, int *spellnum) {
	if (GET_HP_PERC(patient) <= number(20, 33)) {
		if (GET_SPELL_MEM(caster, kSpellExtraHits))
			*spellnum = kSpellExtraHits;
		else if (GET_SPELL_MEM(caster, kSpellHeal))
			*spellnum = kSpellHeal;
		else if (GET_SPELL_MEM(caster, kSpellCureCritic))
			*spellnum = kSpellCureCritic;
		else if (GET_SPELL_MEM(caster, kSpellGroupHeal))
			*spellnum = kSpellGroupHeal;
	} else if (GET_HP_PERC(patient) <= number(50, 65)) {
		if (GET_SPELL_MEM(caster, kSpellCureCritic))
			*spellnum = kSpellCureCritic;
		else if (GET_SPELL_MEM(caster, kSpellCureSerious))
			*spellnum = kSpellCureSerious;
		else if (GET_SPELL_MEM(caster, kSpellCureLight))
			*spellnum = kSpellCureLight;
	}
	if (*spellnum)
		return (patient);
	else
		return (nullptr);
}

void mob_casting(CharData *ch) {
	CharData *victim;
	int battle_spells[kMaxStringLength];
	int spellnum, spells = 0, sp_num;
	ObjData *item;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)
		|| GET_WAIT(ch) > 0)
		return;

	memset(&battle_spells, 0, sizeof(battle_spells));
	for (int i = 1; i <= kSpellCount; i++) {
		if (GET_SPELL_MEM(ch, i) && IS_SET(spell_info[i].routines, NPC_CALCULATE)) {
			battle_spells[spells++] = i;
		}
	}

	item = ch->carrying;
	while (spells < kMaxStringLength
		&& item
		&& GET_RACE(ch) == NPC_RACE_HUMAN
		&& !(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))) {
		switch (GET_OBJ_TYPE(item)) {
			case ObjData::ITEM_WAND:
			case ObjData::ITEM_STAFF:
				if (GET_OBJ_VAL(item, 3) < 0 || GET_OBJ_VAL(item, 3) > kSpellCount) {
					log("SYSERR: Не верно указано значение спела в стафе vnum: %d %s, позиция: 3, значение: %d ",
						GET_OBJ_VNUM(item),
						item->get_PName(0).c_str(),
						GET_OBJ_VAL(item, 3));
					break;
				}

				if (GET_OBJ_VAL(item, 2) > 0 &&
					IS_SET(spell_info[GET_OBJ_VAL(item, 3)].routines, NPC_CALCULATE)) {
					battle_spells[spells++] = GET_OBJ_VAL(item, 3);
				}
				break;

			case ObjData::ITEM_POTION:
				for (int i = 1; i <= 3; i++) {
					if (GET_OBJ_VAL(item, i) < 0 || GET_OBJ_VAL(item, i) > kSpellCount) {
						log("SYSERR: Не верно указано значение спела в напитке vnum %d %s, позиция: %d, значение: %d ",
							GET_OBJ_VNUM(item),
							item->get_PName(0).c_str(),
							i,
							GET_OBJ_VAL(item, i));
						continue;
					}
					if (IS_SET(spell_info[GET_OBJ_VAL(item, i)].routines,
							   kNpcAffectNpc | kNpcUnaffectNpc | kNpcUnaffectNpcCaster)) {
						battle_spells[spells++] = GET_OBJ_VAL(item, i);
					}
				}
				break;

			case ObjData::ITEM_SCROLL:
				for (int i = 1; i <= 3; i++) {
					if (GET_OBJ_VAL(item, i) < 0 || GET_OBJ_VAL(item, i) > kSpellCount) {
						log("SYSERR: Не верно указано значение спела в свитке %d %s, позиция: %d, значение: %d ",
							GET_OBJ_VNUM(item),
							item->get_PName(0).c_str(),
							i,
							GET_OBJ_VAL(item, i));
						continue;
					}

					if (IS_SET(spell_info[GET_OBJ_VAL(item, i)].routines, NPC_CALCULATE)) {
						battle_spells[spells++] = GET_OBJ_VAL(item, i);
					}
				}
				break;

			default: break;
		}

		item = item->get_next_content();
	}

	// перво-наперво  -  лечим себя
	spellnum = 0;
	victim = find_cure(ch, ch, &spellnum);

	// angel not cast if master not in room
	if (MOB_FLAGS(ch).get(MOB_ANGEL)) {
		if (ch->has_master() && ch->in_room != ch->get_master()->in_room) {
			sprintf(buf, "%s тоскливо сморит по сторонам. Кажется ищет кого-то.", ch->get_name_str().c_str());
			act(buf, false, ch, 0, 0, kToRoom | kToArenaListen);
			return;
		}
	}
	// Ищем рандомную заклинашку и цель для нее
	for (int i = 0; !victim && spells && i < GET_REAL_INT(ch) / 5; i++) {
		if (!spellnum && (spellnum = battle_spells[(sp_num = number(0, spells - 1))])
			&& spellnum > 0 && spellnum <= kSpellCount)    // sprintf(buf,"$n using spell '%s', %d from %d",
		{
			//         spell_name(spellnum), sp_num, spells);
			// act(buf,false,ch,0,ch->get_fighting(),TO_VICT);
			if (spell_info[spellnum].routines & kNpcDamagePcMinhp) {
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_target(ch);
			} else if (spell_info[spellnum].routines & kNpcDamagePc) {
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_target(ch);
			} else if (spell_info[spellnum].routines & kNpcAffectPcCaster) {
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_opp_caster(ch);
			} else if (spell_info[spellnum].routines & kNpcAffectPc) {
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
					victim = find_opp_affectee(ch, spellnum);
			} else if (spell_info[spellnum].routines & kNpcAffectNpc)
				victim = find_affectee(ch, spellnum);
			else if (spell_info[spellnum].routines & kNpcUnaffectNpcCaster)
				victim = find_caster(ch, spellnum);
			else if (spell_info[spellnum].routines & kNpcUnaffectNpc)
				victim = find_friend(ch, spellnum);
			else if (spell_info[spellnum].routines & kNpcDummy)
				victim = find_friend_cure(ch, spellnum);
			else
				spellnum = 0;
		}
	}

	if (spellnum && victim)    // Is this object spell ?
	{
		item = ch->carrying;
		while (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& !(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))
			&& item
			&& GET_RACE(ch) == NPC_RACE_HUMAN) {
			switch (GET_OBJ_TYPE(item)) {
				case ObjData::ITEM_WAND:
				case ObjData::ITEM_STAFF:
					if (GET_OBJ_VAL(item, 2) > 0
						&& GET_OBJ_VAL(item, 3) == spellnum) {
						EmployMagicItem(ch, item, GET_NAME(victim));
						return;
					}
					break;

				case ObjData::ITEM_POTION:
					for (int i = 1; i <= 3; i++) {
						if (GET_OBJ_VAL(item, i) == spellnum) {
							if (ch != victim) {
								obj_from_char(item);
								act("$n передал$g $o3 $N2.", false, ch, item, victim, kToRoom | kToArenaListen);
								obj_to_char(item, victim);
							} else {
								victim = ch;
							}
							EmployMagicItem(victim, item, GET_NAME(victim));
							return;
						}
					}
					break;

				case ObjData::ITEM_SCROLL:
					for (int i = 1; i <= 3; i++) {
						if (GET_OBJ_VAL(item, i) == spellnum) {
							EmployMagicItem(ch, item, GET_NAME(victim));
							return;
						}
					}
					break;

				default: break;
			}

			item = item->get_next_content();
		}

		CastSpell(ch, victim, 0, nullptr, spellnum, spellnum);
	}
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)

#define    MAY_ACT(ch)    (!(AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT) || GET_MOB_HOLD(ch) || GET_WAIT(ch)))

void summon_mob_helpers(CharData *ch) {
	for (struct Helper *helpee = GET_HELPER(ch);
		 helpee; helpee = helpee->next) {
		// Start_fight_mtrigger using inside this loop
		// So we have to iterate on copy list
		character_list.foreach_on_copy([&](const CharData::shared_ptr &vict) {
			if (!IS_NPC(vict)
				|| GET_MOB_VNUM(vict) != helpee->mob_vnum
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_BLIND)
				|| GET_WAIT(vict) > 0
				|| GET_POS(vict) < EPosition::kStand
				|| IN_ROOM(vict) == kNowhere
				|| vict->get_fighting()) {
				return;
			}
			if (GET_RACE(ch) == NPC_RACE_HUMAN) {
				act("$n воззвал$g : \"На помощь, мои верные соратники!\"",
					false, ch, 0, 0, kToRoom | kToArenaListen);
			}
			if (IN_ROOM(vict) != ch->in_room) {
				char_from_room(vict);
				char_to_room(vict, ch->in_room);
				act("$n прибыл$g на зов и вступил$g в битву на стороне $N1.",
					false, vict.get(), 0, ch, kToRoom | kToArenaListen);
			} else {
				act("$n вступил$g в битву на стороне $N1.",
					false, vict.get(), 0, ch, kToRoom | kToArenaListen);
			}
			if (MAY_ATTACK(vict)) {
				set_fighting(vict, ch->get_fighting());
			}
		});
	}
}

void check_mob_helpers() {
	for (CharData *ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;
		// Extract battler if no opponent
		if (ch->get_fighting() == nullptr
			|| ch->in_room != IN_ROOM(ch->get_fighting())
			|| ch->in_room == kNowhere) {
			stop_fighting(ch, true);
			continue;
		}
		if (GET_MOB_HOLD(ch)
			|| !IS_NPC(ch)
			|| GET_WAIT(ch) > 0
			|| GET_POS(ch) < EPosition::kFight
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)
			|| PRF_FLAGGED(ch->get_fighting(), PRF_NOHASSLE)) {
			continue;
		}
		summon_mob_helpers(ch);
	}
}

void try_angel_rescue(CharData *ch) {
	struct Follower *k, *k_next;

	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (AFF_FLAGGED(k->ch, EAffectFlag::AFF_HELPER)
			&& MOB_FLAGGED(k->ch, MOB_ANGEL)
			&& !k->ch->get_fighting()
			&& IN_ROOM(k->ch) == ch->in_room
			&& CAN_SEE(k->ch, ch)
			&& AWAKE(k->ch)
			&& MAY_ACT(k->ch)
			&& GET_POS(k->ch) >= EPosition::kFight) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->get_fighting() == ch
					&& vict != ch
					&& vict != k->ch) {
					if (k->ch->get_skill(ESkill::kRescue)) {
						go_rescue(k->ch, ch, vict);
					}

					break;
				}
			}
		}
	}
}

void stand_up_or_sit(CharData *ch) {
	if (IS_NPC(ch)) {
		act("$n поднял$u.", true, ch, 0, 0, kToRoom | kToArenaListen);
		GET_POS(ch) = EPosition::kFight;
	} else if (GET_POS(ch) == EPosition::kSleep) {
		act("Вы проснулись и сели.", true, ch, 0, 0, kToChar);
		act("$n проснул$u и сел$g.", true, ch, 0, 0, kToRoom | kToArenaListen);
		GET_POS(ch) = EPosition::kSit;
	} else if (GET_POS(ch) == EPosition::kRest) {
		act("Вы прекратили отдых и сели.", true, ch, 0, 0, kToChar);
		act("$n прекратил$g отдых и сел$g.", true, ch, 0, 0, kToRoom | kToArenaListen);
		GET_POS(ch) = EPosition::kSit;
	}
}

void set_mob_skills_flags(CharData *ch) {
	bool sk_use = false;
	// 1) parry
	int do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->get_skill(ESkill::kParry)) {
		SET_AF_BATTLE(ch, kEafParry);
		sk_use = true;
	}
	// 2) blocking
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(ESkill::kShieldBlock)) {
		SET_AF_BATTLE(ch, kEafBlock);
		sk_use = true;
	}
	// 3) multyparry
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(ESkill::kMultiparry)) {
		SET_AF_BATTLE(ch, kEafMultyparry);
		sk_use = true;
	}
	// 4) deviate
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->get_skill(ESkill::kDodge)) {
		SET_AF_BATTLE(ch, kEafDodge);
		sk_use = true;
	}
	// 5) styles
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->get_skill(ESkill::kAwake) > number(1, 101)) {
		SET_AF_BATTLE(ch, kEafAwake);
	} else {
		CLR_AF_BATTLE(ch, kEafAwake);
	}
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->get_skill(ESkill::kPunctual) > number(1, 101)) {
		SET_AF_BATTLE(ch, kEafPunctual);
	} else {
		CLR_AF_BATTLE(ch, kEafPunctual);
	}
}

int calc_initiative(CharData *ch, bool mode) {
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

	if (!IS_NPC(ch)) {
		switch (IS_CARRYING_W(ch) * 10 / MAX(1, CAN_CARRY_W(ch))) {
			case 10:
			case 9:
			case 8: initiative -= 2;
				break;
			case 7:
			case 6:
			case 5: initiative -= 1;
				break;
		}
	}

	if (GET_AF_BATTLE(ch, kEafAwake))
		initiative -= 2;
	if (GET_AF_BATTLE(ch, kEafPunctual))
		initiative -= 1;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLOW))
		initiative -= 10;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HASTE))
		initiative += 10;
	if (GET_WAIT(ch) > 0)
		initiative -= 1;
	if (calc_leadership(ch))
		initiative += 5;
	if (GET_AF_BATTLE(ch, kEafSlow))
		initiative = 1;

	//initiative = MAX(initiative, 1); //Почему инициатива не может быть отрицательной?
	return initiative;
}

void using_charmice_skills(CharData *ch) {
	// если чармис вооружен и может глушить - будем глушить
	// если нет оружия но есть молот - будем молотить
	const bool charmice_wielded_for_stupor = GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_BOTHS);
	const bool charmice_not_wielded = !(GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_BOTHS) || GET_EQ(ch, WEAR_HOLD));
	ObjData *wielded = GET_EQ(ch, WEAR_WIELD);
	const bool charmice_wielded_for_throw = (GET_EQ(ch, WEAR_WIELD) && wielded->get_extra_flag(EExtraFlag::ITEM_THROWING)); // Кудояр
	const int do_this = number(0, 100);
	const bool do_skill_without_command = GET_LIKES(ch) >= do_this;
	CharData *master = (ch->get_master() && !IS_NPC(ch->get_master())) ? ch->get_master() : nullptr;

	if (charmice_wielded_for_stupor && ch->get_skill(ESkill::kOverwhelm) > 0) { // оглушить
		const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kOverwhelm) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует оглушение: " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			SET_AF_BATTLE(ch, kEafOverwhelm);
		}
	} else if (charmice_not_wielded && ch->get_skill(ESkill::kHammer) > 0) { // молот
		const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kHammer) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует богатырский молот: " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			SET_AF_BATTLE(ch, kEafHammer);
		}
	} else if(charmice_wielded_for_throw && (ch->get_skill(ESkill::kThrow) > ch->get_skill(ESkill::kOverwhelm))) { // метнуть (Кудояр)
			const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kThrow) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует метнуть : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			ch->set_extra_attack(kExtraAttackThrow, ch->get_fighting());
		}
	} else if (!charmice_wielded_for_throw && (ch->get_extra_attack_mode() != kExtraAttackThrow)
			&& !(GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) && ch->get_skill(ESkill::kUndercut) > 0) { // подножка (Кудояр)
		const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kUndercut) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует подножку : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			if (GET_POS(ch) < EPosition::kFight) return;
		ch->set_extra_attack(kExtraAttackUndercut, ch->get_fighting());
		} 
	}   else if (((ch->get_extra_attack_mode() != kExtraAttackThrow) || (ch->get_extra_attack_mode() != kExtraAttackUndercut))
			&& !(GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) && ch->get_skill(ESkill::kIronwind) > 0) {  // вихрь (Кудояр)
		const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kIronwind) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует ВИХРЬ : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			if (GET_POS(ch) < EPosition::kFight) return;
			go_iron_wind(ch, ch->get_fighting());
		}
	}
}

void using_mob_skills(CharData *ch) {
	ESkill sk_num = ESkill::kIncorrect;
	for (int sk_use = GET_REAL_INT(ch); MAY_LIKES(ch) && sk_use > 0; sk_use--) {
		int do_this = number(0, 100);
		if (do_this > GET_LIKES(ch)) {
			continue;
		}

		do_this = number(0, 100);
		if (do_this < 10) {
			sk_num = ESkill::kBash;
		} else if (do_this < 20) {
			sk_num = ESkill::kDisarm;
		} else if (do_this < 30) {
			sk_num = ESkill::kKick;
		} else if (do_this < 40) {
			sk_num = ESkill::kProtect;
		} else if (do_this < 50) {
			sk_num = ESkill::kRescue;
		} else if (do_this < 60 && !ch->get_touching()) {
			sk_num = ESkill::kIntercept;
		} else if (do_this < 70) {
			sk_num = ESkill::kUndercut;
		} else if (do_this < 80) {
			sk_num = ESkill::kThrow;
		} else if (do_this < 90) {
			sk_num = ESkill::kHammer;
		} else if (do_this <= 100) {
			sk_num = ESkill::kOverwhelm;
		}

		if (ch->get_skill(sk_num) <= 0) {
			sk_num = ESkill::kIncorrect;
		}

		////////////////////////////////////////////////////////////////////////
		// для глуша и молота выставляем соотвествующие флаги
		// цель не выбираем чтобы избежать переключения у мобов, которые не могут переключаться
		if (sk_num == ESkill::kHammer) {
			const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kHammer) <= 0;
			if (skill_ready) {
				sk_use = 0;
				SET_AF_BATTLE(ch, kEafHammer);
			}
		}
		if (sk_num == ESkill::kOverwhelm) {
			const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kOverwhelm) <= 0;
			if (skill_ready) {
				sk_use = 0;
				SET_AF_BATTLE(ch, kEafOverwhelm);
			}
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kIntercept) {
			sk_use = 0;
			go_touch(ch, ch->get_fighting());
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kThrow) {
			sk_use = 0;
			int i = 0;
			// Цель выбираем по рандому
			for (const auto vict : world[ch->in_room]->people) {
				if (!IS_NPC(vict)) {
					i++;
				}
			}

			CharData *caster = 0;
			if (i > 0) {
				i = number(1, i);
				for (const auto vict : world[ch->in_room]->people) {
					if (!IS_NPC(vict)) {
						i--;
						caster = vict;
					}
				}
			}

			// Метаем
			if (caster) {
				go_throw(ch, caster);
			}
		}

		////////////////////////////////////////////////////////////////////////
		// проверим на всякий случай, является ли моб ангелом,
		// хотя вроде бы этого делать не надо
		if (!(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))
			&& ch->has_master()
			&& (sk_num == ESkill::kRescue || sk_num == ESkill::kProtect)) {
			CharData *caster = 0, *damager = 0;
			int dumb_mob = (int) (GET_REAL_INT(ch) < number(5, 20));

			for (const auto attacker : world[ch->in_room]->people) {
				CharData *vict = attacker->get_fighting();    // выяснение жертвы
				if (!vict    // жертвы нет
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
				if (!caster    // еще пока никого не спасаю
					|| (GET_HIT(vict) < GET_HIT(caster)))    // этому мобу хуже
				{
					caster = vict;
					damager = attacker;
					if (dumb_mob) {
						break;    // тупой моб спасает первого
					}
				}
			}

			if (sk_num == ESkill::kRescue && caster && damager) {
				sk_use = 0;
				go_rescue(ch, caster, damager);
			}

			if (sk_num == ESkill::kProtect && caster) {
				sk_use = 0;
				go_protect(ch, caster);
			}
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kBash
			|| sk_num == ESkill::kUndercut
			|| sk_num == ESkill::kDisarm) {
			CharData *caster = 0, *damager = 0;

			if (GET_REAL_INT(ch) < number(15, 25)) {
				caster = ch->get_fighting();
				damager = caster;
			} else {
				caster = find_target(ch);
				damager = find_target(ch);
				for (const auto vict : world[ch->in_room]->people) {
					if ((IS_NPC(vict) && !AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM))
						|| !vict->get_fighting()) {
						continue;
					}
					if ((AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD) && GET_POS(vict) < EPosition::kFight)
						|| (IS_CASTER(vict)
							&& (AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
								|| AFF_FLAGGED(vict, EAffectFlag::AFF_SILENCE)
								|| AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)
								|| GET_WAIT(vict) > 0))) {
						continue;
					}
					if (!caster
						|| (IS_CASTER(vict) && GET_CASTER(vict) > GET_CASTER(caster))) {
						caster = vict;
					}
					if (!damager || GET_DAMAGE(vict) > GET_DAMAGE(damager)) {
						damager = vict;
					}
				}

			}

			if (caster
				&& (CAN_SEE(ch, caster) || ch->get_fighting() == caster)
				&& GET_CASTER(caster) > POOR_CASTER
				&& (sk_num == ESkill::kBash || sk_num == ESkill::kUndercut)) {
				if (sk_num == ESkill::kBash) {
//send_to_char(caster, "Баш предфункция\r\n");
//sprintf(buf, "%s башат предфункция\r\n",GET_NAME(caster));
//mudlog(buf, LGH, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
					if (GET_POS(caster) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kBash, caster) > number(50, 80)) {
						sk_use = 0;
						go_bash(ch, caster);
					}
				} else {
//send_to_char(caster, "Подножка предфункция\r\n");
//sprintf(buf, "%s подсекают предфункция\r\n",GET_NAME(caster));
//                mudlog(buf, LGH, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);

					if (GET_POS(caster) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kUndercut, caster) > number(50, 80)) {
						sk_use = 0;
						go_chopoff(ch, caster);
					}
				}
			}

			if (sk_use
				&& damager
				&& (CAN_SEE(ch, damager)
					|| ch->get_fighting() == damager)) {
				if (sk_num == ESkill::kBash) {

					if (damager->ahorse()) {
						// Карачун. Правка бага. Лошадь не должна башить себя, если дерется с наездником.
						if (damager->get_horse() == ch) {
							ch->drop_from_horse();
						} else {
							sk_use = 0;
							go_bash(ch, damager->get_horse());
						}
					} else if (GET_POS(damager) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kBash, damager) > number(50, 80)) {
						sk_use = 0;
						go_bash(ch, damager);
					}
				} else if (sk_num == ESkill::kUndercut) {
					if (damager->ahorse()) {
						sk_use = 0;
						go_chopoff(ch, damager->get_horse());
					} else if (GET_POS(damager) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kUndercut, damager) > number(50, 80)) {
						sk_use = 0;
						go_chopoff(ch, damager);
					}
				} else if (sk_num == ESkill::kDisarm
					&& (GET_EQ(damager, WEAR_WIELD)
						|| GET_EQ(damager, WEAR_BOTHS)
						|| (GET_EQ(damager, WEAR_HOLD)))) {
					sk_use = 0;
					go_disarm(ch, damager);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kKick && !ch->get_fighting()->ahorse()) {
			sk_use = 0;
			go_kick(ch, ch->get_fighting());
		}
	}
}

void add_attackers_round(CharData *ch) {
	for (const auto i : world[ch->in_room]->people) {
		if (!IS_NPC(i) && i->desc) {
			ch->add_attacker(i, ATTACKER_ROUNDS, 1);
		}
	}
}

void update_round_affs() {
	for (CharData *ch = combat_list; ch; ch = ch->next_fighting) {
		if (ch->in_room == kNowhere)
			continue;

		CLR_AF_BATTLE(ch, kEafFirst);
		CLR_AF_BATTLE(ch, kEafSecond);
		CLR_AF_BATTLE(ch, kEafUsedleft);
		CLR_AF_BATTLE(ch, kEafUsedright);
		CLR_AF_BATTLE(ch, kEafMultyparry);
		CLR_AF_BATTLE(ch, kEafDodge);

		if (GET_AF_BATTLE(ch, kEafSleep))
			affect_from_char(ch, kSpellSleep);

		if (GET_AF_BATTLE(ch, kEafBlock)) {
			CLR_AF_BATTLE(ch, kEafBlock);
			if (!WAITLESS(ch) && GET_WAIT(ch) < kPulseViolence)
				WAIT_STATE(ch, 1 * kPulseViolence);
		}

//		if (GET_AF_BATTLE(ch, EAF_DEVIATE))
//		{
//			CLR_AF_BATTLE(ch, EAF_DEVIATE);
//			if (!WAITLESS(ch) && GET_WAIT(ch) < kPulseViolence)
//				WAIT_STATE(ch, 1 * kPulseViolence);
//		}

		if (GET_AF_BATTLE(ch, kEafPoisoned)) {
			CLR_AF_BATTLE(ch, kEafPoisoned);
		}

		battle_affect_update(ch);

		if (IS_NPC(ch) && !IS_CHARMICE(ch)) {
			add_attackers_round(ch);
		}
	}
}

bool using_extra_attack(CharData *ch) {
	bool used = false;

	switch (ch->get_extra_attack_mode()) {
		case kExtraAttackThrow: go_throw(ch, ch->get_extra_victim());
			used = true;
			break;
		case kExtraAttackBash: go_bash(ch, ch->get_extra_victim());
			used = true;
			break;
		case kExtraAttackKick: go_kick(ch, ch->get_extra_victim());
			used = true;
			break;
		case kExtraAttackUndercut: go_chopoff(ch, ch->get_extra_victim());
			used = true;
			break;
		case kExtraAttackDisarm: go_disarm(ch, ch->get_extra_victim());
			used = true;
			break;
		case kExtraAttackCut: GoExpedientCut(ch, ch->get_extra_victim());
			used = true;
			break;
		default:used = false;
			break;
	}

	return used;
}

void process_npc_attack(CharData *ch) {
	// if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
	//    continue;

	// Вызываем триггер перед началом боевых моба (магических или физических)

	if (!fight_mtrigger(ch)) {
		return;
	}
	// Срабатывание батл-триггеров амуниции
	Bitvector trigger_code = fight_otrigger(ch);

	// переключение
	if (MAY_LIKES(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_NOT_SWITCH)
		&& GET_REAL_INT(ch) > number(15, 25)) {
		perform_mob_switch(ch);
	}

	// Cast spells
	if (MAY_LIKES(ch) && !IS_SET(trigger_code, kNoCastMagic))
		mob_casting(ch);

	if (!ch->get_fighting()
		|| ch->in_room != IN_ROOM(ch->get_fighting())
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
			// mob_casting мог от зеркала отразиться
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| !AWAKE(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)) {
		return;
	}

	bool no_extra_attack = IS_SET(trigger_code, kNoExtraAttack);
	if ((AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL))
		&& ch->has_master() && ch->in_room == IN_ROOM(ch->get_master())  // && !IS_NPC(ch->master)
		&& AWAKE(ch) && MAY_ACT(ch) && GET_POS(ch) >= EPosition::kFight) {
		// сначала мытаемся спасти
		if (CAN_SEE(ch, ch->get_master()) && AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->get_fighting() == ch->get_master()
					&& vict != ch && vict != ch->get_master()) {
					if (ch->get_skill(ESkill::kRescue)) {
						go_rescue(ch, ch->get_master(), vict);
					} else if (ch->get_skill(ESkill::kProtect)) {
						go_protect(ch, ch->get_master());
					}
					break;
				}
			}
		}

		bool extra_attack_used = no_extra_attack;
		//* применение экстра скилл-атак
		if (!extra_attack_used && ch->get_extra_victim() && GET_WAIT(ch) <= 0) {
			extra_attack_used = using_extra_attack(ch);
			if (extra_attack_used) {
				ch->set_extra_attack(kExtraAttackUnused, 0);
			}
		}
		if (!extra_attack_used && AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)) {
			// теперь чармис использует свои скилы
			using_charmice_skills(ch);
		}
	} else if (!no_extra_attack && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		//* применение скилов
		using_mob_skills(ch);
	}

	if (!ch->get_fighting() || ch->in_room != IN_ROOM(ch->get_fighting())) {
		return;
	}

	//**** удар основным оружием или рукой
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) && !IS_SET(trigger_code, kNoRightHandAttack)) {
		exthit(ch, ESkill::kUndefined, fight::AttackType::kMainHand);
	}

	//**** экстраатаки мобов. Первая - оффхэнд
	for (int i = 1; i <= ch->mob_specials.ExtraAttack; i++) {
		if (i == 1 && (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || IS_SET(trigger_code, kNoLeftHandAttack))) {
			continue;
		}
		// если хп пробиты - уходим
		if (MOB_FLAGGED(ch, MOB_EADECREASE)) {
			if (ch->mob_specials.ExtraAttack * GET_HIT(ch) * 2 < i * GET_REAL_MAX_HIT(ch)) {
				return;
			}
		}
		exthit(ch, ESkill::kUndefined, fight::AttackType::kMobAdd);
	}
}

void process_player_attack(CharData *ch, int min_init) {
	if (GET_POS(ch) > EPosition::kStun
		&& GET_POS(ch) < EPosition::kFight
		&& GET_AF_BATTLE(ch, kEafStand)) {
		sprintf(buf, "%sВам лучше встать на ноги!%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		CLR_AF_BATTLE(ch, kEafStand);
	}

	// Срабатывание батл-триггеров амуниции
	Bitvector trigger_code = fight_otrigger(ch);

	//* каст заклинания
	if (ch->get_cast_spell() && GET_WAIT(ch) <= 0 && !IS_SET(trigger_code, kNoCastMagic)) {
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
			send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
			ch->set_cast(0, 0, 0, 0, 0);
		} else {
			CastSpell(ch, ch->get_cast_char(), ch->get_cast_obj(),
					  0, ch->get_cast_spell(), ch->get_cast_subst());

			if (!(IS_IMMORTAL(ch)
				|| GET_GOD_FLAG(ch, GF_GODSLIKE)
				|| CHECK_WAIT(ch))) {
				WAIT_STATE(ch, kPulseViolence);
			}
			ch->set_cast(0, 0, 0, 0, 0);
		}
		if (INITIATIVE(ch) > min_init) {
			INITIATIVE(ch)--;
			return;
		}
	}

	if (GET_AF_BATTLE(ch, kEafMultyparry))
		return;

	//* применение экстра скилл-атак (пнуть, оглушить и прочая)
	if (!IS_SET(trigger_code, kNoExtraAttack) && ch->get_extra_victim()
		&& GET_WAIT(ch) <= 0 && using_extra_attack(ch)) {
		ch->set_extra_attack(kExtraAttackUnused, nullptr);
		if (INITIATIVE(ch) > min_init) {
			INITIATIVE(ch)--;
			return;
		}
	}

	if (!ch->get_fighting()
		|| ch->in_room != IN_ROOM(ch->get_fighting())) {
		return;
	}

	//**** удар основным оружием или рукой
	if (GET_AF_BATTLE(ch, kEafFirst)) {
		if (!IS_SET(trigger_code, kNoRightHandAttack) && !AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT)
			&& (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) || !GET_AF_BATTLE(ch, kEafUsedright))) {
			//Знаю, выглядит страшно, но зато в hit()
			//можно будет узнать применялось ли оглушить
			//или молотить, по баттл-аффекту узнать получиться
			//не во всех частях процедуры, а параметр type
			//хранит значение до её конца.
			ESkill tmpSkilltype;
			if (GET_AF_BATTLE(ch, kEafOverwhelm))
				tmpSkilltype = ESkill::kOverwhelm;
			else if (GET_AF_BATTLE(ch, kEafHammer))
				tmpSkilltype = ESkill::kHammer;
			else tmpSkilltype = ESkill::kUndefined;
			exthit(ch, tmpSkilltype, fight::AttackType::kMainHand);
		}
// допатака двуручем
		if (!IS_SET(trigger_code, kNoExtraAttack) && GET_EQ(ch, WEAR_BOTHS) && can_use_feat(ch, BOTHHANDS_FOCUS_FEAT)
			&& (static_cast<ESkill>(GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS))) == ESkill::kTwohands)
			&& can_use_feat(ch, RELATED_TO_MAGIC_FEAT)) {
			if (ch->get_skill(ESkill::kTwohands) > (number(1, 500)))
				hit(ch, ch->get_fighting(), ESkill::kUndefined, fight::AttackType::kMainHand);
		}
		CLR_AF_BATTLE(ch, kEafFirst);
		SET_AF_BATTLE(ch, kEafSecond);
		if (INITIATIVE(ch) > min_init) {
			INITIATIVE(ch)--;
			return;
		}
	}

	//**** удар вторым оружием если оно есть и умение позволяет
	if (!IS_SET(trigger_code, kNoLeftHandAttack) && GET_EQ(ch, WEAR_HOLD)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ObjData::ITEM_WEAPON
		&& GET_AF_BATTLE(ch, kEafSecond)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)
		&& (IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE)
			|| ch->get_skill(ESkill::kSideAttack) > number(1, 101))) {
		if (IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE)
			|| !GET_AF_BATTLE(ch, kEafUsedleft)) {
			exthit(ch, ESkill::kUndefined, fight::AttackType::kOffHand);
		}
		CLR_AF_BATTLE(ch, kEafSecond);
	}
		//**** удар второй рукой если она свободна и умение позволяет
	else if (!IS_SET(trigger_code, kNoLeftHandAttack) && !GET_EQ(ch, WEAR_HOLD)
		&& !GET_EQ(ch, WEAR_LIGHT) && !GET_EQ(ch, WEAR_SHIELD) && !GET_EQ(ch, WEAR_BOTHS)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) && GET_AF_BATTLE(ch, kEafSecond)
		&& ch->get_skill(ESkill::kLeftHit)) {
		if (IS_IMMORTAL(ch) || !GET_AF_BATTLE(ch, kEafUsedleft)) {
			exthit(ch, ESkill::kUndefined, fight::AttackType::kOffHand);
		}
		CLR_AF_BATTLE(ch, kEafSecond);
	}

	// немного коряво, т.к. зависит от инициативы кастера
	// check if angel is in fight, and go_rescue if it is not
	try_angel_rescue(ch);
}

// * \return false - чар не участвует в расчете инициативы
bool stuff_before_round(CharData *ch) {
	// Initialize initiative
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	ROUND_COUNTER(ch) += 1;
	DpsSystem::check_round(ch);
	BattleRoundParameters params(ch);
	handle_affects(params);
	round_num_mtrigger(ch, ch->get_fighting());

	SET_AF_BATTLE(ch, kEafStand);
	if (affected_by_spell(ch, kSpellSleep))
		SET_AF_BATTLE(ch, kEafSleep);
	if (ch->in_room == kNowhere)
		return false;

	if (GET_MOB_HOLD(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)) {
		try_angel_rescue(ch);
		return false;
	}

	// Mobs stand up and players sit
	if (GET_POS(ch) < EPosition::kFight
		&& GET_POS(ch) > EPosition::kStun
		&& GET_WAIT(ch) <= 0
		&& !GET_MOB_HOLD(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP)) {
		stand_up_or_sit(ch);
	}

	// For NPC without lags and charms make it likes
	if (IS_NPC(ch) && MAY_LIKES(ch))    // Get weapon from room
	{
		//edited by WorM 2010.09.03 добавил немного логики мобам в бою если
		// у моба есть что-то в инве то он может
		//переодется в это что-то если посчитает что это что-то ему лучше подходит
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
			//Чото бред какой-то был, одевались мобы только сразу после того как слутили
			npc_battle_scavenge(ch);//лутим стаф
			if (ch->carrying)//и если есть что-то в инве то вооружаемся, одеваемся
			{
				npc_wield(ch);
				npc_armor(ch);
			}
		}
		//end by WorM
		//dzMUDiST. Выполнение последнего переданого в бою за время лага приказа
		if (ch->last_comm != nullptr) {
			command_interpreter(ch, ch->last_comm);
			free(ch->last_comm);
			ch->last_comm = nullptr;
		}
		// Set some flag-skills
		set_mob_skills_flags(ch);
	}

	return true;
}

// * Обработка текущих боев, дергается каждые 2 секунды.
void perform_violence() {
	int max_init = -100, min_init = 100;

	//* суммон хелперов
	check_mob_helpers();

	// храним список писей, которым надо показать состояние группы по msdp
	std::unordered_set<CharData *> msdp_report_chars;

	//* действия до раунда и расчет инициативы
	for (CharData *ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;

		if (ch->desc) {
			msdp_report_chars.insert(ch);
		} else if (ch->has_master()
			&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				|| MOB_FLAGGED(ch, MOB_ANGEL)
				|| MOB_FLAGGED(ch, MOB_GHOST))) {
			auto master = ch->get_master();
			if (master->desc
				&& !master->get_fighting()
				&& master->in_room == ch->in_room) {
				msdp_report_chars.insert(master);
			}
		}

		if (!stuff_before_round(ch)) {
			continue;
		}

		const int initiative = calc_initiative(ch, true);
		if (initiative == 0) {
			INITIATIVE(ch) = -100; //Если кубик выпал в 0 - бей последним шанс 1 из 201
			min_init = MIN(min_init, -100);
		} else {
			INITIATIVE(ch) = initiative;
		}

		SET_AF_BATTLE(ch, kEafFirst);
		max_init = MAX(max_init, initiative);
		min_init = MIN(min_init, initiative);
	}

	//* обработка раунда по очередности инициативы
	for (int initiative = max_init; initiative >= min_init; initiative--) {
		for (CharData *ch = combat_list; ch; ch = next_combat_list) {
			next_combat_list = ch->next_fighting;
			if (INITIATIVE(ch) != initiative || ch->in_room == kNowhere) {
				continue;
			}

			// If mob cast 'hold' when initiative setted
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
				|| !AWAKE(ch)) {
				continue;
			}
			// If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
			if (!ch->get_fighting() || ch->in_room != IN_ROOM(ch->get_fighting())) {
				continue;
			}
			//везде в стоп-файтах ставится инициатива равная 0, убираем двойную атаку
			if (initiative == 0) {
				continue;
			}
			//* выполнение атак в раунде
			if (IS_NPC(ch)) {
				process_npc_attack(ch);
			} else {
				process_player_attack(ch, min_init);
			}
		}
	}

	//* обновление аффектов и лагов после раунда
	update_round_affs();

	for (auto d = descriptor_list; d; d = d->next) {
		if (STATE(d) == CON_PLAYING && d->character) {
			for (const auto &ch : msdp_report_chars) {
				if (same_group(ch, d->character.get()) && ch->in_room == d->character->in_room) {
					msdp_report_chars.insert(d->character.get());
					break;
				}
			}
		}
	}

	// покажем группу по msdp
	// проверка на поддержку протокола есть в методе msdp_report
	for (const auto &ch: msdp_report_chars) {
		if (!ch->purged()) {
			ch->desc->msdp_report(msdp::constants::GROUP);
		}
	}
}

// returns 1 if only ch was outcasted
// returns 2 if only victim was outcasted
// returns 4 if both were outcasted
// returns 0 if none was outcasted
int check_agro_follower(CharData *ch, CharData *victim) {
	CharData *cleader, *vleader;
	int return_value = 0;

	if (ch == victim) {
		return return_value;
	}

// translating pointers from charimces to their leaders
	if (IS_NPC(ch)
		&& ch->has_master()
		&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(ch, MOB_ANGEL)
			|| MOB_FLAGGED(ch, MOB_GHOST)
			|| IS_HORSE(ch))) {
		ch = ch->get_master();
	}

	if (IS_NPC(victim)
		&& victim->has_master()
		&& (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
			|| MOB_FLAGGED(victim, MOB_ANGEL)
			|| MOB_FLAGGED(victim, MOB_GHOST)
			|| IS_HORSE(victim))) {
		victim = victim->get_master();
	}

	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->has_master()) {
		if (IS_NPC(cleader)
			&& !AFF_FLAGGED(cleader, EAffectFlag::AFF_CHARM)
			&& !(MOB_FLAGGED(cleader, MOB_ANGEL) || MOB_FLAGGED(cleader, MOB_GHOST))
			&& !IS_HORSE(cleader)) {
			break;
		}
		cleader = cleader->get_master();
	}

	while (vleader->has_master()) {
		if (IS_NPC(vleader)
			&& !AFF_FLAGGED(vleader, EAffectFlag::AFF_CHARM)
			&& !(MOB_FLAGGED(vleader, MOB_ANGEL) || MOB_FLAGGED(vleader, MOB_GHOST))
			&& !IS_HORSE(vleader)) {
			break;
		}
		vleader = vleader->get_master();
	}

	if (cleader != vleader) {
		return return_value;
	}

// finding closest to the leader nongrouped agressor
// it cannot be a charmice
	while (ch->has_master()
		&& ch->get_master()->has_master()) {
		if (!AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(ch->get_master())) {
			ch = ch->get_master();
			continue;
		} else if (IS_NPC(ch->get_master())
			&& !AFF_FLAGGED(ch->get_master()->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(ch->get_master()->get_master())
			&& ch->get_master()->get_master()->get_master()) {
			ch = ch->get_master()->get_master();
			continue;
		} else {
			break;
		}
	}

// finding closest to the leader nongrouped victim
// it cannot be a charmice
	while (victim->has_master()
		&& victim->get_master()->has_master()) {
		if (!AFF_FLAGGED(victim->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(victim->get_master())) {
			victim = victim->get_master();
			continue;
		} else if (IS_NPC(victim->get_master())
			&& !AFF_FLAGGED(victim->get_master()->get_master(), EAffectFlag::AFF_GROUP)
			&& !IS_NPC(victim->get_master()->get_master())
			&& victim->get_master()->get_master()->has_master()) {
			victim = victim->get_master()->get_master();
			continue;
		} else {
			break;
		}
	}
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)
		|| cleader == victim) {
		stop_follower(ch, kSfEmpty);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, EAffectFlag::AFF_GROUP)
		|| vleader == ch) {
		stop_follower(victim, kSfEmpty);
		return_value |= 2;
	}
	return return_value;
}

int calc_leadership(CharData *ch) {
	int prob, percent;
	CharData *leader = 0;

	if (IS_NPC(ch)
		|| !AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)
		|| (!ch->has_master()
			&& !ch->followers)) {
		return false;
	}

	if (ch->has_master()) {
		if (IN_ROOM(ch) != IN_ROOM(ch->get_master())) {
			return false;
		}
		leader = ch->get_master();
	} else {
		leader = ch;
	}

	if (!leader->get_skill(ESkill::kLeadership)) {
		return (false);
	}

	percent = number(1, 101);
	prob = CalcCurrentSkill(leader, ESkill::kLeadership, 0);
	if (percent > prob) {
		return (false);
	} else {
		return (true);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
