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
#include "game_fight/mobact.h"
#include "handler.h"
#include "color.h"
#include "utils/random.h"
#include "entities/char_player.h"
#include "game_magic/magic.h"
#include "olc/olc.h"
#include "msdp/msdp_constants.h"
#include "game_magic/magic_items.h"
#include "structs/global_objects.h"

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
	struct FollowerType *k;
	struct FollowerType *d;
	CharData *ch_lider = 0;
	if (ch->has_master()) {
		ch_lider = ch->get_master();
	} else {
		ch_lider = ch;    // Создаем ссылку на лидера
	}

	buf2[0] = '\0';
	for (k = ch_lider->followers; k; k = k->next) {
		if (PRF_FLAGGED(k->follower, EPrf::kAutoassist) &&
			(IN_ROOM(k->follower) == IN_ROOM(ch)) && !k->follower->GetEnemy() &&
			(GET_POS(k->follower) == EPosition::kStand) && k->follower->get_wait() <= 0) {
			// Здесь проверяем на кастеров
			if (IsCaster(k->follower)) {
				// здесь проходим по чармисам кастера, и если находим их, то вписываем в драку
				for (d = k->follower->followers; d; d = d->next)
					if ((IN_ROOM(d->follower) == IN_ROOM(ch)) && !d->follower->GetEnemy() &&
						(GET_POS(d->follower) == EPosition::kStand) && d->follower->get_wait() <= 0)
						do_assist(d->follower, buf2, 0, 0);
			} else {
				do_assist(k->follower, buf2, 0, 0);
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
	else if (GET_HIT(victim) > 0
			&& victim->get_wait() <= 0
			&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
			&& !AFF_FLAGGED(victim, EAffect::kHold)) {
		GET_POS(victim) = EPosition::kStand;
		SendMsgToChar("Вы встали.\r\n", victim);
		act("$n поднял$u.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	}
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = EPosition::kDead;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = EPosition::kPerish;
	else if (GET_HIT(victim) <= -1)
		GET_POS(victim) = EPosition::kIncap;
	else if (GET_POS(victim) == EPosition::kIncap
				&& (victim->get_wait() > 0 || AFF_FLAGGED(victim, EAffect::kMagicStopFight)))
		GET_POS(victim) = EPosition::kIncap;
	else
		GET_POS(victim) = EPosition::kStun;

	if (AFF_FLAGGED(victim, EAffect::kSleep) && GET_POS(victim) != EPosition::kSleep)
		RemoveAffectFromChar(victim, ESpell::kSleep);

	// поплохело седоку или лошади - сбрасываем седока
	if (victim->IsOnHorse() && GET_POS(victim) < EPosition::kFight)
		victim->DropFromHorse();
	if (IS_HORSE(victim) && GET_POS(victim) < EPosition::kFight && victim->get_master()->IsOnHorse())
		victim->DropFromHorse();
}

void set_battle_pos(CharData *ch) {
	switch (GET_POS(ch)) {
		case EPosition::kStand: GET_POS(ch) = EPosition::kFight;
			break;
		case EPosition::kRest:
		case EPosition::kSit:
		case EPosition::kSleep:
			if (ch->get_wait() <= 0 &&
				!AFF_FLAGGED(ch, EAffect::kHold) && !AFF_FLAGGED(ch, EAffect::kSleep)
				&& !AFF_FLAGGED(ch, EAffect::kCharmed)) {
				if (ch->IsNpc()) {
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
			if (ch->IsNpc() &&
				ch->get_wait() <= 0 &&
				!AFF_FLAGGED(ch, EAffect::kHold) && !AFF_FLAGGED(ch, EAffect::kSleep)
				&& !AFF_FLAGGED(ch, EAffect::kCharmed)) {
				act("$n поднял$u.", false, ch, 0, 0, kToRoom | kToArenaListen);
				GET_POS(ch) = EPosition::kStand;
			}
			break;
		default:
			break;
	}
	if (AFF_FLAGGED(ch, EAffect::kSleep))
		GET_POS(ch) = EPosition::kSleep;
}

/**
 * Start one char fighting another (yes, it is horrible, I know... )
 */
void SetFighting(CharData *ch, CharData *vict) {
	if (ch == vict)
		return;
	if (ch->GetEnemy()) {
		log("SYSERR: SetFighting(%s->%s) when already fighting(%s)...",
			GET_NAME(ch), GET_NAME(vict), GET_NAME(ch->GetEnemy()));
		// core_dump();
		return;
	}

	if ((ch->IsNpc() && MOB_FLAGGED(ch, EMobFlag::kNoFight)) || (vict->IsNpc() && MOB_FLAGGED(vict, EMobFlag::kNoFight)))
		return;

	// if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
	//    return;

	if (AFF_FLAGGED(ch, EAffect::kBandage)) {
		SendMsgToChar("Перевязка была прервана!\r\n", ch);
		RemoveAffectFromChar(ch, ESpell::kBandage);
	}
	if (AFF_FLAGGED(ch, EAffect::kMemorizeSpells)) {
		SendMsgToChar("Вы забыли о концентрации и ринулись в бой!\r\n", ch);
		RemoveAffectFromChar(ch, ESpell::kRecallSpells);
	}

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, EAffect::kSleep))
		RemoveAffectFromChar(ch, ESpell::kSleep);

	ch->SetEnemy(vict);

	NUL_AF_BATTLE(ch);
	//Polud вступление в битву не мешает прикрывать
	if (ch->get_protecting())
		SET_AF_BATTLE(ch, kEafProtect);
	ch->set_touching(0);
	ch->initiative = 0;
	ch->battle_counter = 0;
	ch->round_counter = 0;
	ch->SetExtraAttack(kExtraAttackUnused, 0);
	set_battle_pos(ch);

	// если до начала боя на мобе есть лаг, то мы его выравниваем до целых
	// раундов в большую сторону (для подножки, должно давать чару зазор в две
	// секунды после подножки, чтобы моб всеравно встал только на 3й раунд)
	if (ch->IsNpc() && ch->get_wait() > 0) {
//		div_t tmp = div(static_cast<const int>(ch->get_wait()), static_cast<const int>(kBattleRound));
		auto tmp = div(ch->get_wait(), kBattleRound);
		if (tmp.rem > 0) {
			SetWaitState(ch, (tmp.quot + 1) * kBattleRound);
		}
	}
	if (!ch->IsNpc() && (!ch->GetSkill(ESkill::kAwake))) {
		PRF_FLAGS(ch).unset(EPrf::kAwake);
	}

	if (!ch->IsNpc() && (!ch->GetSkill(ESkill::kPunctual))) {
		PRF_FLAGS(ch).unset(EPrf::kPunctual);
	}

	// Set combat style
	if (!AFF_FLAGGED(ch, EAffect::kCourage) && !AFF_FLAGGED(ch, EAffect::kDrunked)
		&& !AFF_FLAGGED(ch, EAffect::kAbstinent)) {
		if (PRF_FLAGGED(ch, EPrf::kPunctual))
			SET_AF_BATTLE(ch, kEafPunctual);
		else if (PRF_FLAGGED(ch, EPrf::kAwake))
			SET_AF_BATTLE(ch, kEafAwake);
	}

	if (CanUseFeat(ch, EFeat::kDefender) && ch->GetSkill(ESkill::kShieldBlock)) {
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
	ch->SetEnemy(0);
	ch->initiative = 0;
	ch->battle_counter = 0;
	ch->round_counter = 0;
	ch->SetExtraAttack(kExtraAttackUnused, 0);
	ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
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
			if (temp->GetExtraVictim() == ch)
				temp->SetExtraAttack(kExtraAttackUnused, nullptr);
			if (temp->GetCastChar() == ch)
				temp->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
			if (temp->GetEnemy() == ch && switch_others) {
				log("[Stop fighting] %s : Change victim for fighting", GET_NAME(temp));
				for (found = combat_list; found; found = found->next_fighting)
					if (found != ch && found->GetEnemy() == temp) {
						act("Вы переключили свое внимание на $N3.", false, temp, 0, found, kToChar);
						temp->SetEnemy(found);
						break;
					}
				if (!found) {
					stop_fighting(temp, false);
				};
			}
		}

		update_pos(ch);
		// проверка скилла "железный ветер" - снимаем флаг по окончанию боя
		if ((ch->GetEnemy() == nullptr) && PRF_FLAGS(ch).get(EPrf::kIronWind)) {
			PRF_FLAGS(ch).unset(EPrf::kIronWind);
			if (GET_POS(ch) > EPosition::kIncap) {
				SendMsgToChar("Безумие боя отпустило вас, и враз навалилась усталость...\r\n", ch);
				act("$n шумно выдохнул$g и остановил$u, переводя дух после боя.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			};
		};
	};
}

int GET_MAXDAMAGE(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kHold)) {
		return 0;
	} else {
		return ch->damage_level;
	}
}

int GET_MAXCASTER(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kHold) || AFF_FLAGGED(ch, EAffect::kSilence)
		|| AFF_FLAGGED(ch, EAffect::kStrangled)
		|| ch->get_wait() > 0)
		return 0;
	else
		return IS_IMMORTAL(ch) ? 1 : ch->caster_level;
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
	if (npc->GetEnemy() == pc)    // NPC fight PC - opponent
		return (opponent);
	if (pc->GetEnemy() == npc)    // PC fight NPC - opponent
		return (opponent);
	if (npc->GetEnemy() && npc->GetEnemy() == pc->GetEnemy())
		return (!opponent);    // Fight same victim - friend
	if (AFF_FLAGGED(pc, EAffect::kHorse) || AFF_FLAGGED(pc, EAffect::kCharmed))
		return (opponent);

	npc_master = npc->has_master() ? npc->get_master() : npc;
	pc_master = pc->has_master() ? pc->get_master() : pc;

	for (const auto
			ch : world[IN_ROOM(npc)]->people)    // <- Anton Gorev (2017-07-11): changed loop to loop over the people in room (instead of all world)
	{
		if (!ch->GetEnemy()) {
			continue;
		}

		ch_master = ch->has_master() ? ch->get_master() : ch;
		ch_friend_npc = (ch_master == npc_master) ||
			(ch->IsNpc() && npc->IsNpc() &&
				!AFF_FLAGGED(ch, EAffect::kCharmed) && !AFF_FLAGGED(npc, EAffect::kCharmed) &&
				!AFF_FLAGGED(ch, EAffect::kHorse) && !AFF_FLAGGED(npc, EAffect::kHorse));
		ch_friend_pc = (ch_master == pc_master) ||
			(ch->IsNpc() && pc->IsNpc() &&
				!AFF_FLAGGED(ch, EAffect::kCharmed) && !AFF_FLAGGED(pc, EAffect::kCharmed) &&
				!AFF_FLAGGED(ch, EAffect::kHorse) && !AFF_FLAGGED(pc, EAffect::kHorse));
		if (ch->GetEnemy() == pc && ch_friend_npc)    // Friend NPC fight PC - opponent
			return (opponent);
		if (pc->GetEnemy() == ch && ch_friend_npc)    // PC fight friend NPC - opponent
			return (opponent);
		if (npc->GetEnemy() == ch && ch_friend_pc)    // NPC fight friend PC - opponent
			return (opponent);
		if (ch->GetEnemy() == npc && ch_friend_pc)    // Friend PC fight NPC - opponent
			return (opponent);
		vict = ch->GetEnemy();
		vict_master = vict->has_master() ? vict->get_master() : vict;
		vict_friend_npc = (vict_master == npc_master) ||
			(vict->IsNpc() && npc->IsNpc() &&
				!AFF_FLAGGED(vict, EAffect::kCharmed) && !AFF_FLAGGED(npc, EAffect::kCharmed) &&
				!AFF_FLAGGED(vict, EAffect::kHorse) && !AFF_FLAGGED(npc, EAffect::kHorse));
		vict_friend_pc = (vict_master == pc_master) ||
			(vict->IsNpc() && pc->IsNpc() &&
				!AFF_FLAGGED(vict, EAffect::kCharmed) && !AFF_FLAGGED(pc, EAffect::kCharmed) &&
				!AFF_FLAGGED(vict, EAffect::kHorse) && !AFF_FLAGGED(pc, EAffect::kHorse));
		if (ch_friend_npc && vict_friend_pc)
			return (opponent);    // Friend NPC fight friend PC - opponent
		if (ch_friend_pc && vict_friend_npc)
			return (opponent);    // Friend PC fight friend NPC - opponent
	}

	return (!opponent);
}

CharData *find_friend_cure(CharData *caster, ESpell spell_id) {
	CharData *victim = nullptr;
	int vict_val = 0, AFF_USED = 0;
	switch (spell_id) {
		case ESpell::kCureLight: AFF_USED = 80;
			break;
		case ESpell::kCureSerious: AFF_USED = 70;
			break;
		case ESpell::kExtraHits:
		case ESpell::kCureCritic: AFF_USED = 50;
			break;
		case ESpell::kHeal:
		case ESpell::kGroupHeal: AFF_USED = 30;
			break;
		default: break;
	}

	if ((AFF_FLAGGED(caster, EAffect::kCharmed) || MOB_FLAGGED(caster, EMobFlag::kTutelar)
		|| MOB_FLAGGED(caster, EMobFlag::kMentalShadow) || MOB_FLAGGED(caster, EMobFlag::kSummoned)) // ()
		&& AFF_FLAGGED(caster, EAffect::kHelper)) {
		if (GET_HP_PERC(caster) < AFF_USED) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& caster->get_master()->GetEnemy()
			&& GET_HP_PERC(caster->get_master()) < AFF_USED) {
			return caster->get_master();
		}
		return nullptr;
	}

	for (const auto vict : world[IN_ROOM(caster)]->people) {
		if (!vict->IsNpc()
			|| AFF_FLAGGED(vict, EAffect::kCharmed)
			|| ((MOB_FLAGGED(vict, EMobFlag::kTutelar) || MOB_FLAGGED(vict, EMobFlag::kMentalShadow) || MOB_FLAGGED(vict, EMobFlag::kSummoned)) // ()
				&& vict->has_master()
				&& !vict->get_master()->IsNpc())
			|| !CAN_SEE(caster, vict)) {
			continue;
		}

		if (!vict->GetEnemy() && !MOB_FLAGGED(vict, EMobFlag::kHelper)) {
			continue;
		}

		if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict))) {
			victim = vict;
			vict_val = GET_HP_PERC(vict);
			if (GetRealInt(caster) < number(10, 20)) {
				break;
			}
		}
	}
	return victim;
}

CharData *find_friend(CharData *caster, ESpell spell_id) {
	CharData *victim = nullptr;
	int vict_val = 0;
	affects_list_t AFF_USED;
	auto spellreal{ESpell::kUndefined};
	switch (spell_id) {
		case ESpell::kCureBlind: AFF_USED.push_back(EAffect::kBlind);
			break;
		case ESpell::kRemovePoison: AFF_USED.push_back(EAffect::kPoisoned);
			AFF_USED.push_back(EAffect::kScopolaPoison);
			AFF_USED.push_back(EAffect::kBelenaPoison);
			AFF_USED.push_back(EAffect::kDaturaPoison);
			break;
		case ESpell::kRemoveHold: AFF_USED.push_back(EAffect::kHold);
			break;
		case ESpell::kRemoveCurse: AFF_USED.push_back(EAffect::kCurse);
			break;
		case ESpell::kRemoveSilence: AFF_USED.push_back(EAffect::kSilence);
			break;
		case ESpell::kRemoveDeafness: AFF_USED.push_back(EAffect::kDeafness);
			break;
		case ESpell::kCureFever: spellreal = ESpell::kFever;
			break;
		default: break;
	}
	if (AFF_FLAGGED(caster, EAffect::kHelper)
		&& (AFF_FLAGGED(caster, EAffect::kCharmed)
			|| MOB_FLAGGED(caster, EMobFlag::kTutelar) || MOB_FLAGGED(caster, EMobFlag::kMentalShadow) || MOB_FLAGGED(caster, EMobFlag::kSummoned))) { //()
		if (caster->has_any_affect(AFF_USED)
			|| IsAffectedBySpell(caster, spellreal)) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& (caster->get_master()->has_any_affect(AFF_USED)
				|| IsAffectedBySpell(caster->get_master(), spellreal))) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (!AFF_USED.empty()) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if (!vict->IsNpc() || AFF_FLAGGED(vict, EAffect::kCharmed) ||
				((MOB_FLAGGED(vict, EMobFlag::kTutelar) ||
				MOB_FLAGGED(vict, EMobFlag::kMentalShadow) ||
				MOB_FLAGGED(vict, EMobFlag::kSummoned)) // ()
					&& vict->get_master()
					&& !vict->get_master()->IsNpc())
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->has_any_affect(AFF_USED)) {
				continue;
			}

			if (!vict->GetEnemy()
				&& !MOB_FLAGGED(vict, EMobFlag::kHelper)) {
				continue;
			}

			if (!victim
				|| vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
				if (GetRealInt(caster) < number(10, 20)) {
					break;
				}
			}
		}
	}

	return victim;
}

CharData *find_caster(CharData *caster, ESpell spell_id) {
	CharData *victim = nullptr;
	int vict_val = 0;
	affects_list_t AFF_USED;
	auto spellreal{ESpell::kUndefined};
	switch (spell_id) {
		case ESpell::kCureBlind: AFF_USED.push_back(EAffect::kBlind);
			break;
		case ESpell::kRemovePoison: AFF_USED.push_back(EAffect::kPoisoned);
			AFF_USED.push_back(EAffect::kScopolaPoison);
			AFF_USED.push_back(EAffect::kBelenaPoison);
			AFF_USED.push_back(EAffect::kDaturaPoison);
			break;
		case ESpell::kRemoveHold: AFF_USED.push_back(EAffect::kHold);
			break;
		case ESpell::kRemoveCurse: AFF_USED.push_back(EAffect::kCurse);
			break;
		case ESpell::kRemoveSilence: AFF_USED.push_back(EAffect::kSilence);
			break;
		case ESpell::kRemoveDeafness: AFF_USED.push_back(EAffect::kDeafness);
			break;
		case ESpell::kCureFever: spellreal = ESpell::kFever;
			break;
		default: break;
	}

	if (AFF_FLAGGED(caster, EAffect::kHelper)
		&& (AFF_FLAGGED(caster, EAffect::kCharmed)
			|| MOB_FLAGGED(caster, EMobFlag::kTutelar) || MOB_FLAGGED(caster, EMobFlag::kMentalShadow) || MOB_FLAGGED(caster, EMobFlag::kSummoned))) { // ()
		if (caster->has_any_affect(AFF_USED)
			|| IsAffectedBySpell(caster, spellreal)) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& (caster->get_master()->has_any_affect(AFF_USED)
				|| IsAffectedBySpell(caster->get_master(), spellreal))) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (!AFF_USED.empty()) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if (!vict->IsNpc()
				|| AFF_FLAGGED(vict, EAffect::kCharmed)
				|| ((MOB_FLAGGED(vict, EMobFlag::kTutelar) || MOB_FLAGGED(vict, EMobFlag::kMentalShadow) || MOB_FLAGGED(vict, EMobFlag::kSummoned)) // ()
					&& (vict->get_master() && !vict->get_master()->IsNpc()))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->has_any_affect(AFF_USED)) {
				continue;
			}

			if (!vict->GetEnemy()
				&& !MOB_FLAGGED(vict, EMobFlag::kHelper)) {
				continue;
			}

			if (!victim
				|| vict_val < GET_MAXCASTER(vict)) {
				victim = vict;
				vict_val = GET_MAXCASTER(vict);
				if (GetRealInt(caster) < number(10, 20)) {
					break;
				}
			}
		}
	}

	return victim;
}

CharData *find_affectee(CharData *caster, ESpell spell_id) {
	CharData *victim = nullptr;
	int vict_val = 0;
	auto spellreal = spell_id;

	if (spellreal == ESpell::kGroupArmor)
		spellreal = ESpell::kArmor;
	else if (spellreal == ESpell::kGroupStrength)
		spellreal = ESpell::kStrength;
	else if (spellreal == ESpell::kGroupBless)
		spellreal = ESpell::kBless;
	else if (spellreal == ESpell::kGroupHaste)
		spellreal = ESpell::kHaste;
	else if (spellreal == ESpell::kGroupSanctuary)
		spellreal = ESpell::kSanctuary;
	else if (spellreal == ESpell::kGroupPrismaticAura)
		spellreal = ESpell::kPrismaticAura;
	else if (spellreal == ESpell::kSightOfDarkness)
		spellreal = ESpell::kInfravision;
	else if (spellreal == ESpell::kGroupSincerity)
		spellreal = ESpell::kDetectAlign;
	else if (spellreal == ESpell::kMagicalGaze)
		spellreal = ESpell::kDetectMagic;
	else if (spellreal == ESpell::kAllSeeingEye)
		spellreal = ESpell::kDetectInvis;
	else if (spellreal == ESpell::kEyeOfGods)
		spellreal = ESpell::kSenseLife;
	else if (spellreal == ESpell::kBreathingAtDepth)
		spellreal = ESpell::kWaterbreath;
	else if (spellreal == ESpell::kGeneralRecovery)
		spellreal = ESpell::kFastRegeneration;
	else if (spellreal == ESpell::kCommonMeal)
		spellreal = ESpell::kFullFeed;
	else if (spellreal == ESpell::kStoneWall)
		spellreal = ESpell::kStoneSkin;
	else if (spellreal == ESpell::kSnakeEyes)
		spellreal = ESpell::kDetectPoison;

	if ((AFF_FLAGGED(caster, EAffect::kCharmed) || MOB_FLAGGED(caster, EMobFlag::kTutelar) ||
		MOB_FLAGGED(caster, EMobFlag::kMentalShadow) || MOB_FLAGGED(caster, EMobFlag::kSummoned)) &&
		AFF_FLAGGED(caster, EAffect::kHelper)) {
		if (!IsAffectedBySpell(caster, spellreal)) {
			return caster;
		} else if (caster->has_master()
			&& CAN_SEE(caster, caster->get_master())
			&& IN_ROOM(caster->get_master()) == IN_ROOM(caster)
			&& caster->get_master()->GetEnemy() && !IsAffectedBySpell(caster->get_master(), spellreal)) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (GetRealInt(caster) > number(5, 15)) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if (!vict->IsNpc()
				|| AFF_FLAGGED(vict, EAffect::kCharmed)
				|| ((MOB_FLAGGED(vict, EMobFlag::kTutelar) || MOB_FLAGGED(vict, EMobFlag::kMentalShadow) || MOB_FLAGGED(vict, EMobFlag::kSummoned)) // ()
					&& vict->has_master()
					&& !vict->get_master()->IsNpc())
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->GetEnemy()
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| IsAffectedBySpell(vict, spellreal)) {
				continue;
			}

			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim && !IsAffectedBySpell(caster, spellreal)) {
		victim = caster;
	}

	return victim;
}

CharData *find_opp_affectee(CharData *caster, ESpell spell_id) {
	CharData *victim = nullptr;
	int vict_val = 0;

	auto spellreal = spell_id;
	if (spellreal == ESpell::kPowerHold || spellreal == ESpell::kMassHold)
		spellreal = ESpell::kHold;
	else if (spellreal == ESpell::kPowerBlindness || spellreal == ESpell::kMassBlindness)
		spellreal = ESpell::kBlindness;
	else if (spellreal == ESpell::kPowerSilence || spellreal == ESpell::kMassSilence)
		spellreal = ESpell::kSilence;
	else if (spellreal == ESpell::kMassCurse)
		spellreal = ESpell::kCurse;
	else if (spellreal == ESpell::kMassSlow)
		spellreal = ESpell::kSlowdown;
	else if (spellreal == ESpell::kMassFailure)
		spellreal = ESpell::kFailure;
	else if (spellreal == ESpell::kSnare)
		spellreal = ESpell::kNoflee;

	if (GetRealInt(caster) > number(10, 20)) {
		for (const auto vict : world[caster->in_room]->people) {
			if ((vict->IsNpc()
				&& !((MOB_FLAGGED(vict, EMobFlag::kTutelar) || MOB_FLAGGED(vict, EMobFlag::kMentalShadow) || MOB_FLAGGED(vict, EMobFlag::kSummoned) // ()
					|| AFF_FLAGGED(vict, EAffect::kCharmed))
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if ((!vict->GetEnemy()
				&& (GetRealInt(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true)))
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| IsAffectedBySpell(vict, spellreal)) {
				continue;
			}
			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim
		&& caster->GetEnemy()
		&& !IsAffectedBySpell(caster->GetEnemy(), spellreal)) {
		victim = caster->GetEnemy();
	}

	return victim;
}

CharData *find_opp_caster(CharData *caster) {
	CharData *victim = nullptr;
	int vict_val = 0;

	for (const auto vict : world[IN_ROOM(caster)]->people) {
		if (vict->IsNpc()
			&& !((MOB_FLAGGED(vict, EMobFlag::kTutelar) || MOB_FLAGGED(vict, EMobFlag::kMentalShadow) || MOB_FLAGGED(vict, EMobFlag::kSummoned)) //
				&& vict->has_master()
				&& !vict->get_master()->IsNpc())) {
			continue;
		}
		if ((!vict->GetEnemy()
			&& (GetRealInt(caster) < number(15, 25)
				|| !in_same_battle(caster, vict, true)))
			|| AFF_FLAGGED(vict, EAffect::kHold) || AFF_FLAGGED(vict, EAffect::kSilence)
			|| AFF_FLAGGED(vict, EAffect::kStrangled)
			|| (!CAN_SEE(caster, vict) && caster->GetEnemy() != vict))
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

	if (GetRealInt(caster) > number(10, 20)) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if ((vict->IsNpc()
				&& !((MOB_FLAGGED(vict, EMobFlag::kTutelar)
					|| MOB_FLAGGED(vict, EMobFlag::kMentalShadow)
					|| AFF_FLAGGED(vict, EAffect::kCharmed))
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if ((!vict->GetEnemy()
				&& (GetRealInt(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true)))
				|| AFF_FLAGGED(vict, EAffect::kHold)) {
				continue;
			}

			if (GetRealInt(caster) >= number(25, 30)) {
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
		victim = caster->GetEnemy();
	}

	return victim;
}

CharData *find_target(CharData *ch) {
	CharData *currentVictim, *caster = nullptr, *best = nullptr;
	CharData *druid = nullptr, *cler = nullptr, *charmmage = nullptr;

	currentVictim = ch->GetEnemy();

	int mobINT = GetRealInt(ch);

	if (mobINT < kStupidMod) {
		return find_damagee(ch);
	}

	if (!currentVictim) {
		return nullptr;
	}

	if (IsCaster(currentVictim) && !currentVictim->IsNpc()) {
		return currentVictim;
	}

	// проходим по всем чарам в комнате
	for (const auto vict : world[ch->in_room]->people) {
		if ((vict->IsNpc() && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->GetEnemy()
				&& find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
			|| PRF_FLAGGED(vict, EPrf::kNohassle)
			|| !MAY_SEE(ch, ch, vict)) {
			continue;
		}

		if (!CAN_SEE(ch, vict)) {
			continue;
		}

		if (vict->GetClass() == ECharClass::kMagus) {
			druid = vict;
			caster = vict;
			continue;
		}

		if (vict->GetClass() == ECharClass::kSorcerer) {
			cler = vict;
			caster = vict;
			continue;
		}

		if (vict->GetClass() == ECharClass::kCharmer) {
			charmmage = vict;
			caster = vict;
			continue;
		}

		if (GET_HIT(vict) <= kCharacterHPForMobPriorityAttack) {
			return vict;
		}

		if (IsCaster(vict)) {
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

	if (GetRealInt(caster) > number(10, 20)) {
		for (const auto vict : world[IN_ROOM(caster)]->people) {
			if ((vict->IsNpc()
				&& !((MOB_FLAGGED(vict, EMobFlag::kTutelar) || MOB_FLAGGED(vict, EMobFlag::kMentalShadow) || MOB_FLAGGED(vict, EMobFlag::kSummoned) // ()
					|| AFF_FLAGGED(vict, EAffect::kCharmed))
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !CAN_SEE(caster, vict)) {
				continue;
			}

			if (!vict->GetEnemy()
				&& (GetRealInt(caster) < number(20, 27)
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
		victim = caster->GetEnemy();
	}

	return victim;
}

CharData *find_cure(CharData *caster, CharData *patient, ESpell &spell_id) {
	if (GET_HP_PERC(patient) <= number(20, 33)) {
		if (GET_SPELL_MEM(caster, ESpell::kExtraHits))
			spell_id = ESpell::kExtraHits;
		else if (GET_SPELL_MEM(caster, ESpell::kHeal))
			spell_id = ESpell::kHeal;
		else if (GET_SPELL_MEM(caster, ESpell::kCureCritic))
			spell_id = ESpell::kCureCritic;
		else if (GET_SPELL_MEM(caster, ESpell::kGroupHeal))
			spell_id = ESpell::kGroupHeal;
	} else if (GET_HP_PERC(patient) <= number(50, 65)) {
		if (GET_SPELL_MEM(caster, ESpell::kCureCritic))
			spell_id = ESpell::kCureCritic;
		else if (GET_SPELL_MEM(caster, ESpell::kCureSerious))
			spell_id = ESpell::kCureSerious;
		else if (GET_SPELL_MEM(caster, ESpell::kCureLight))
			spell_id = ESpell::kCureLight;
	}
	if (spell_id != ESpell::kUndefined) {
		return (patient);
	} else {
		return (nullptr);
	}
}

void mob_casting(CharData *ch) {
	CharData *victim;
	ESpell battle_spells[kMaxStringLength];
	int spells = 0, sp_num;
	ObjData *item;

	if (AFF_FLAGGED(ch, EAffect::kCharmed)
		|| AFF_FLAGGED(ch, EAffect::kHold)
		|| AFF_FLAGGED(ch, EAffect::kSilence)
		|| AFF_FLAGGED(ch, EAffect::kStrangled)
		|| ch->get_wait() > 0)
		return;

	memset(&battle_spells, 0, sizeof(battle_spells));
	for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
		if (GET_SPELL_MEM(ch, spell_id) && MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE)) {
			battle_spells[spells++] = spell_id;
		}
	}

	item = ch->carrying;
	while (spells < kMaxStringLength
		&& item
		&& GET_RACE(ch) == ENpcRace::kHuman
		&& !(MOB_FLAGGED(ch, EMobFlag::kTutelar) || MOB_FLAGGED(ch, EMobFlag::kMentalShadow))) {
		switch (GET_OBJ_TYPE(item)) {
			case EObjType::kWand:
			case EObjType::kStaff: {
				const auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(item, 3));
				if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
					log("SYSERR: Неверно указано значение спела в стафе vnum: %d %s, позиция: 3, значение: %d ",
						GET_OBJ_VNUM(item), item->get_PName(0).c_str(), GET_OBJ_VAL(item, 3));
					break;
				}

				if (GET_OBJ_VAL(item, 2) > 0 && MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE)) {
					battle_spells[spells++] = spell_id;
				}
				break;
			}
			case EObjType::kPotion: {
				for (int i = 1; i <= 3; i++) {
					const auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(item, i));
					if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
						log("SYSERR: Неверно указано значение спела в напитке vnum %d %s, позиция: %d, значение: %d ",
							GET_OBJ_VNUM(item), item->get_PName(0).c_str(), i, GET_OBJ_VAL(item, i));
						continue;
					}
					if (MUD::Spell(spell_id).IsFlagged(kNpcAffectNpc | kNpcUnaffectNpc | kNpcUnaffectNpcCaster)) {
						battle_spells[spells++] = spell_id;
					}
				}
				break;
			}
			case EObjType::kScroll: {
				for (int i = 1; i <= 3; i++) {
					const auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(item, i));
					if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
						log("SYSERR: Не верно указано значение спела в свитке %d %s, позиция: %d, значение: %d ",
							GET_OBJ_VNUM(item), item->get_PName(0).c_str(), i, GET_OBJ_VAL(item, i));
						continue;
					}

					if (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE)) {
						battle_spells[spells++] = spell_id;
					}
				}
				break;
			}
			default: break;
		}

		item = item->get_next_content();
	}

	// перво-наперво  -  лечим себя
	auto spell_id_2{ESpell::kUndefined};
	victim = find_cure(ch, ch, spell_id_2);

	// angel not cast if master not in room
	if (MOB_FLAGS(ch).get(EMobFlag::kTutelar)) {
		if (ch->has_master() && ch->in_room != ch->get_master()->in_room) {
			sprintf(buf, "%s тоскливо сморит по сторонам. Кажется ищет кого-то.", ch->get_name_str().c_str());
			act(buf, false, ch, 0, 0, kToRoom | kToArenaListen);
			return;
		}
	}
	// Ищем рандомную заклинашку и цель для нее
	for (int i = 0; !victim && spells && i < GetRealInt(ch) / 5; i++) {
		if (spell_id_2 == ESpell::kUndefined) {
			spell_id_2 = battle_spells[(sp_num = number(0, spells - 1))];
		}
		if (spell_id_2 >= ESpell::kFirst && spell_id_2 <= ESpell::kLast) {
			if (MUD::Spell(spell_id_2).IsFlagged(kNpcDamagePcMinhp)) {
				if (!AFF_FLAGGED(ch, EAffect::kCharmed))
					victim = find_target(ch);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcDamagePc)) {
				if (!AFF_FLAGGED(ch, EAffect::kCharmed))
					victim = find_target(ch);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcAffectPcCaster)) {
				if (!AFF_FLAGGED(ch, EAffect::kCharmed))
					victim = find_opp_caster(ch);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcAffectPc)) {
				if (!AFF_FLAGGED(ch, EAffect::kCharmed))
					victim = find_opp_affectee(ch, spell_id_2);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcAffectNpc)) {
				victim = find_affectee(ch, spell_id_2);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcUnaffectNpcCaster)) {
				victim = find_caster(ch, spell_id_2);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcUnaffectNpc)) {
				victim = find_friend(ch, spell_id_2);
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcDummy)) {
				victim = find_friend_cure(ch, spell_id_2);
			} else {
				spell_id_2 = ESpell::kUndefined;
			}
		}
	}

	// Is this object spell ?
	if (spell_id_2 != ESpell::kUndefined && victim) {
		item = ch->carrying;
		while (!AFF_FLAGGED(ch, EAffect::kCharmed)
			&& !(MOB_FLAGGED(ch, EMobFlag::kTutelar) || MOB_FLAGGED(ch, EMobFlag::kMentalShadow))
			&& item
			&& GET_RACE(ch) == ENpcRace::kHuman) {
			switch (GET_OBJ_TYPE(item)) {
				case EObjType::kWand:
				case EObjType::kStaff:
					if (GET_OBJ_VAL(item, 2) > 0
						&& static_cast<ESpell>(GET_OBJ_VAL(item, 3)) == spell_id_2) {
						EmployMagicItem(ch, item, GET_NAME(victim));
						return;
					}
					break;

				case EObjType::kPotion:
					for (int i = 1; i <= 3; i++) {
						if (static_cast<ESpell>(GET_OBJ_VAL(item, i)) == spell_id_2) {
							if (ch != victim) {
								RemoveObjFromChar(item);
								act("$n передал$g $o3 $N2.", false, ch, item, victim, kToRoom | kToArenaListen);
								PlaceObjToInventory(item, victim);
							} else {
								victim = ch;
							}
							EmployMagicItem(victim, item, GET_NAME(victim));
							return;
						}
					}
					break;

				case EObjType::kScroll:
					for (int i = 1; i <= 3; i++) {
						if (static_cast<ESpell>(GET_OBJ_VAL(item, i)) == spell_id_2) {
							EmployMagicItem(ch, item, GET_NAME(victim));
							return;
						}
					}
					break;

				default: break;
			}

			item = item->get_next_content();
		}

		CastSpell(ch, victim, 0, nullptr, spell_id_2, spell_id_2);
	}
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, EAffect::kCharmed) || AFF_FLAGGED(ch, EAffect::kHelper)) && \
                          AWAKE(ch) && ch->get_wait() <= 0)

#define    MAY_ACT(ch)    (!(AFF_FLAGGED(ch, EAffect::kStopFight) || AFF_FLAGGED(ch, EAffect::kMagicStopFight) || \
							AFF_FLAGGED(ch, EAffect::kHold) || (ch)->get_wait()))

void summon_mob_helpers(CharData *ch) {
	if (ch->summon_helpers.empty()) {
		return;
	}
	for (auto helpee :ch->summon_helpers) {
		// Start_fight_mtrigger using inside this loop
		// So we have to iterate on copy list
		Characters::list_t mobs;
		character_list.get_mobs_by_rnum(real_mobile(helpee), mobs);
		for (const auto &vict : mobs) {
			if (AFF_FLAGGED(ch, EAffect::kCharmed)
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| AFF_FLAGGED(vict, EAffect::kCharmed)
				|| AFF_FLAGGED(vict, EAffect::kBlind)
				|| vict->get_wait() > 0
				|| GET_POS(vict) < EPosition::kStand
				|| IN_ROOM(vict) == kNowhere
				|| vict->GetEnemy()) {
				continue;
			}
			MOB_FLAGS(vict).set(EMobFlag::kHelper);
			if (GET_RACE(ch) == ENpcRace::kHuman) {
				act("$n воззвал$g : \"На помощь, мои верные соратники!\"",
					false, ch, 0, 0, kToRoom | kToArenaListen);
			}
			if (IN_ROOM(vict) != ch->in_room) {
				act("$n был$g призван$g $N4.", false, vict.get(), 0, ch, kToRoom | kToArenaListen);
				char_from_room(vict);
				char_to_room(vict, ch->in_room);
				act("$n прибыл$g на зов и вступил$g в битву на стороне $N1.", false, vict.get(), 0, ch, kToRoom | kToArenaListen);
			} else {
				act("$n вступил$g в битву на стороне $N1.", false, vict.get(), 0, ch, kToRoom | kToArenaListen);
			}
			if (MAY_ATTACK(vict)) {
				set_fighting(vict, ch->GetEnemy());
			}
		}
	}
	ch->summon_helpers.clear();
}

void check_mob_helpers() {
	for (CharData *ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;
		// Extract battler if no opponent
		if (ch->GetEnemy() == nullptr
			|| ch->in_room != IN_ROOM(ch->GetEnemy())
			|| ch->in_room == kNowhere) {
			stop_fighting(ch, true);
			continue;
		}
		if (AFF_FLAGGED(ch, EAffect::kHold)
			|| !ch->IsNpc()
			|| ch->get_wait() > 0
			|| GET_POS(ch) < EPosition::kFight
			|| AFF_FLAGGED(ch, EAffect::kCharmed)
			|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)
			|| AFF_FLAGGED(ch, EAffect::kStopFight)
			|| AFF_FLAGGED(ch, EAffect::kSilence)
			|| AFF_FLAGGED(ch, EAffect::kStrangled)
			|| PRF_FLAGGED(ch->GetEnemy(), EPrf::kNohassle)) {
			continue;
		}
		summon_mob_helpers(ch);
	}
}

void try_angel_rescue(CharData *ch) {
	struct FollowerType *k, *k_next;

	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (AFF_FLAGGED(k->follower, EAffect::kHelper)
			&& MOB_FLAGGED(k->follower, EMobFlag::kTutelar)
			&& !k->follower->GetEnemy()
			&& IN_ROOM(k->follower) == ch->in_room
			&& CAN_SEE(k->follower, ch)
			&& AWAKE(k->follower)
			&& MAY_ACT(k->follower)
			&& GET_POS(k->follower) >= EPosition::kFight) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->GetEnemy() == ch
					&& vict != ch
					&& vict != k->follower) {
					if (k->follower->GetSkill(ESkill::kRescue)) {
						go_rescue(k->follower, ch, vict);
					}

					break;
				}
			}
		}
	}
}

void stand_up_or_sit(CharData *ch) {
	if (ch->IsNpc()) {
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
	if (do_this <= GET_LIKES(ch) && ch->GetSkill(ESkill::kParry)) {
		SET_AF_BATTLE(ch, kEafParry);
		sk_use = true;
	}
	// 2) blocking
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->GetSkill(ESkill::kShieldBlock)) {
		SET_AF_BATTLE(ch, kEafBlock);
		sk_use = true;
	}
	// 3) multyparry
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->GetSkill(ESkill::kMultiparry)) {
		SET_AF_BATTLE(ch, kEafMultyparry);
		sk_use = true;
	}
	// 4) deviate
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && ch->GetSkill(ESkill::kDodge)) {
		SET_AF_BATTLE(ch, kEafDodge);
		sk_use = true;
	}
	// 5) styles
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->GetSkill(ESkill::kAwake) > number(1, 101)) {
		SET_AF_BATTLE(ch, kEafAwake);
	} else {
		CLR_AF_BATTLE(ch, kEafAwake);
	}
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && ch->GetSkill(ESkill::kPunctual) > number(1, 101)) {
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

	if (!ch->IsNpc()) {
		switch (IS_CARRYING_W(ch) * 10 / MAX(1, CAN_CARRY_W(ch))) {
			case 10:
			case 9:
			case 8: initiative -= 20;
				break;
			case 7:
			case 6:
			case 5: initiative -= 10;
				break;
		}
	}

	if (GET_AF_BATTLE(ch, kEafAwake))
		initiative -= 20;
	if (GET_AF_BATTLE(ch, kEafPunctual))
		initiative -= 10;
	if (ch->get_wait() > 0)
		initiative -= 1;
	if (calc_leadership(ch))
		initiative += 5;
	if (GET_AF_BATTLE(ch, kEafSlow))
		initiative = 1;

// indra
// рраскомменчу, посмотрим
	initiative = std::max(initiative, 1);
	//Почему инициатива не может быть отрицательной?
	return initiative;
}

void using_charmice_skills(CharData *ch) {
	// если чармис вооружен и может глушить - будем глушить
	// если нет оружия но есть молот - будем молотить
	const bool charmice_wielded_for_stupor = GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kBoths);
	const bool charmice_not_wielded = !(GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kBoths) || GET_EQ(ch, EEquipPos::kHold));
	ObjData *wielded = GET_EQ(ch, EEquipPos::kWield);
	const bool charmice_wielded_for_throw = (GET_EQ(ch, EEquipPos::kWield) && wielded->has_flag(EObjFlag::kThrowing)); //
	const int do_this = number(0, 100);
	const bool do_skill_without_command = GET_LIKES(ch) >= do_this;
	CharData *master = (ch->get_master() && !ch->get_master()->IsNpc()) ? ch->get_master() : nullptr;

	if (charmice_wielded_for_stupor && ch->GetSkill(ESkill::kOverwhelm) > 0) { // оглушить
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
	} else if (charmice_not_wielded && ch->GetSkill(ESkill::kHammer) > 0) { // молот
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
	} else if(charmice_wielded_for_throw && (ch->GetSkill(ESkill::kThrow) > ch->GetSkill(ESkill::kOverwhelm))) { // метнуть ()
			const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kThrow) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует метнуть : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			ch->SetExtraAttack(kExtraAttackThrow, ch->GetEnemy());
		}
	} else if (!charmice_wielded_for_throw && (ch->get_extra_attack_mode() != kExtraAttackThrow)
			&& !(GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) && ch->GetSkill(ESkill::kChopoff) > 0) { // подножка ()
		const bool skill_ready = ch->getSkillCooldown(ESkill::kGlobalCooldown) <= 0 && ch->getSkillCooldown(ESkill::kChopoff) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует подножку : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			master->send_to_TC(true, true, true, msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			if (GET_POS(ch) < EPosition::kFight) return;
			ch->SetExtraAttack(kExtraAttackChopoff, ch->GetEnemy());
		} 
	}   else if (((ch->get_extra_attack_mode() != kExtraAttackThrow) || (ch->get_extra_attack_mode() != kExtraAttackChopoff))
			&& !(GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) && ch->GetSkill(ESkill::kIronwind) > 0) {  // вихрь ()
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
			go_iron_wind(ch, ch->GetEnemy());
		}
	}
}

void using_mob_skills(CharData *ch) {
	auto sk_num{ESkill::kUndefined};
	for (int sk_use = GetRealInt(ch); MAY_LIKES(ch) && sk_use > 0; sk_use--) {
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
			sk_num = ESkill::kChopoff;
		} else if (do_this < 80) {
			sk_num = ESkill::kThrow;
		} else if (do_this < 90) {
			sk_num = ESkill::kHammer;
		} else if (do_this <= 100) {
			sk_num = ESkill::kOverwhelm;
		}

		if (ch->GetSkill(sk_num) <= 0) {
			sk_num = ESkill::kUndefined;
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
			go_touch(ch, ch->GetEnemy());
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kThrow) {
			sk_use = 0;
			int i = 0;
			// Цель выбираем по рандому
			for (const auto vict : world[ch->in_room]->people) {
				if (!vict->IsNpc()) {
					i++;
				}
			}

			CharData *caster = 0;
			if (i > 0) {
				i = number(1, i);
				for (const auto vict : world[ch->in_room]->people) {
					if (!vict->IsNpc()) {
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
		if (!(MOB_FLAGGED(ch, EMobFlag::kTutelar) || MOB_FLAGGED(ch, EMobFlag::kMentalShadow))
			&& ch->has_master()
			&& (sk_num == ESkill::kRescue || sk_num == ESkill::kProtect)) {
			CharData *caster = 0, *damager = 0;
			int dumb_mob = (int) (GetRealInt(ch) < number(5, 20));

			for (const auto attacker : world[ch->in_room]->people) {
				CharData *vict = attacker->GetEnemy();    // выяснение жертвы
				if (!vict    // жертвы нет
					|| (!vict->IsNpc() // жертва - не моб
						|| AFF_FLAGGED(vict, EAffect::kCharmed)
						|| AFF_FLAGGED(vict, EAffect::kHelper))
					|| (attacker->IsNpc()
						&& !(AFF_FLAGGED(attacker, EAffect::kCharmed)
							&& attacker->has_master()
							&& !attacker->get_master()->IsNpc())
						&& !(MOB_FLAGGED(attacker, EMobFlag::kMentalShadow)
							&& attacker->has_master()
							&& !attacker->get_master()->IsNpc())
						&& !(MOB_FLAGGED(attacker, EMobFlag::kTutelar)
							&& attacker->has_master()
							&& !attacker->get_master()->IsNpc()))
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
			|| sk_num == ESkill::kChopoff
			|| sk_num == ESkill::kDisarm) {
			CharData *caster = 0, *damager = 0;

			if (GetRealInt(ch) < number(15, 25)) {
				caster = ch->GetEnemy();
				damager = caster;
			} else {
				caster = find_target(ch);
				damager = find_target(ch);
				for (const auto vict : world[ch->in_room]->people) {
					if ((vict->IsNpc() && !AFF_FLAGGED(vict, EAffect::kCharmed))
						|| !vict->GetEnemy()) {
						continue;
					}
					if ((AFF_FLAGGED(vict, EAffect::kHold) && GET_POS(vict) < EPosition::kFight)
						|| (IsCaster(vict)
							&& (AFF_FLAGGED(vict, EAffect::kHold)
								|| AFF_FLAGGED(vict, EAffect::kSilence)
								|| AFF_FLAGGED(vict, EAffect::kStrangled)
								|| vict->get_wait() > 0))) {
						continue;
					}
					if (!caster
						|| (IsCaster(vict) && vict->caster_level > caster->caster_level)) {
						caster = vict;
					}
					if (!damager || vict->damage_level > damager->damage_level) {
						damager = vict;
					}
				}

			}

			if (caster
				&& (CAN_SEE(ch, caster) || ch->GetEnemy() == caster)
				&& caster->caster_level > POOR_CASTER
				&& (sk_num == ESkill::kBash || sk_num == ESkill::kChopoff)) {
				if (sk_num == ESkill::kBash) {
//SendMsgToChar(caster, "Баш предфункция\r\n");
//sprintf(buf, "%s башат предфункция\r\n",GET_NAME(caster));
//mudlog(buf, LGH, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
					if (GET_POS(caster) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kBash, caster) > number(50, 80)) {
						sk_use = 0;
						go_bash(ch, caster);
					}
				} else {
//SendMsgToChar(caster, "Подножка предфункция\r\n");
//sprintf(buf, "%s подсекают предфункция\r\n",GET_NAME(caster));
//                mudlog(buf, LGH, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);

					if (GET_POS(caster) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kChopoff, caster) > number(50, 80)) {
						sk_use = 0;
						go_chopoff(ch, caster);
					}
				}
			}

			if (sk_use
				&& damager
				&& (CAN_SEE(ch, damager)
					|| ch->GetEnemy() == damager)) {
				if (sk_num == ESkill::kBash) {

					if (damager->IsOnHorse()) {
						// Карачун. Правка бага. Лошадь не должна башить себя, если дерется с наездником.
						if (damager->get_horse() == ch) {
							ch->DropFromHorse();
						} else {
							sk_use = 0;
							go_bash(ch, damager->get_horse());
						}
					} else if (GET_POS(damager) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kBash, damager) > number(50, 80)) {
						sk_use = 0;
						go_bash(ch, damager);
					}
				} else if (sk_num == ESkill::kChopoff) {
					if (damager->IsOnHorse()) {
						sk_use = 0;
						go_chopoff(ch, damager->get_horse());
					} else if (GET_POS(damager) >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kChopoff, damager) > number(50, 80)) {
						sk_use = 0;
						go_chopoff(ch, damager);
					}
				} else if (sk_num == ESkill::kDisarm
					&& (GET_EQ(damager, EEquipPos::kWield)
						|| GET_EQ(damager, EEquipPos::kBoths)
						|| (GET_EQ(damager, EEquipPos::kHold)))) {
					sk_use = 0;
					go_disarm(ch, damager);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kKick && !ch->GetEnemy()->IsOnHorse()) {
			sk_use = 0;
			go_kick(ch, ch->GetEnemy());
		}
	}
}

void add_attackers_round(CharData *ch) {
	for (const auto i : world[ch->in_room]->people) {
		if (!i->IsNpc() && i->desc) {
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
		CLR_AF_BATTLE(ch, kEafTouch);
		if (ch->get_touching())
			ch->set_touching(0);

		if (GET_AF_BATTLE(ch, kEafSleep))
			RemoveAffectFromChar(ch, ESpell::kSleep);

		if (GET_AF_BATTLE(ch, kEafBlock)) {
			CLR_AF_BATTLE(ch, kEafBlock);
			if (!IS_IMMORTAL(ch) && ch->get_wait() < kBattleRound)
				SetWaitState(ch, 1 * kBattleRound);
		}

		if (GET_AF_BATTLE(ch, kEafPoisoned)) {
			CLR_AF_BATTLE(ch, kEafPoisoned);
		}

		battle_affect_update(ch);

		if (ch->IsNpc() && !IS_CHARMICE(ch)) {
			add_attackers_round(ch);
		}
	}
}

bool using_extra_attack(CharData *ch) {
	bool used = false;

	switch (ch->get_extra_attack_mode()) {
		case kExtraAttackThrow: go_throw(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackBash: go_bash(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackKick: go_kick(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackChopoff: go_chopoff(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackDisarm: go_disarm(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackCut: GoExpedientCut(ch, ch->GetExtraVictim());
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
		&& !AFF_FLAGGED(ch, EAffect::kCharmed)
		&& !AFF_FLAGGED(ch, EAffect::kNoBattleSwitch)
		&& GetRealInt(ch) > number(15, 25)) {
		perform_mob_switch(ch);
	}

	// Cast spells
	if (MAY_LIKES(ch) && !IS_SET(trigger_code, kNoCastMagic))
		mob_casting(ch);

	if (!ch->GetEnemy()
		|| ch->in_room != IN_ROOM(ch->GetEnemy())
		|| AFF_FLAGGED(ch, EAffect::kHold)
			// mob_casting мог от зеркала отразиться
		|| AFF_FLAGGED(ch, EAffect::kStopFight)
		|| !AWAKE(ch)
		|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)) {
		return;
	}

	bool no_extra_attack = IS_SET(trigger_code, kNoExtraAttack);
	if ((AFF_FLAGGED(ch, EAffect::kCharmed) || MOB_FLAGGED(ch, EMobFlag::kTutelar))
		&& ch->has_master() && ch->in_room == IN_ROOM(ch->get_master())  // && !ch->master->IsNpc()
		&& AWAKE(ch) && MAY_ACT(ch) && GET_POS(ch) >= EPosition::kFight) {
		// сначала мытаемся спасти
		if (CAN_SEE(ch, ch->get_master()) && AFF_FLAGGED(ch, EAffect::kHelper)) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->GetEnemy() == ch->get_master()
					&& vict != ch && vict != ch->get_master()) {
					if (ch->GetSkill(ESkill::kRescue)) {
						go_rescue(ch, ch->get_master(), vict);
					} else if (ch->GetSkill(ESkill::kProtect)) {
						go_protect(ch, ch->get_master());
					}
					break;
				}
			}
		}

		bool extra_attack_used = no_extra_attack;
		//* применение экстра скилл-атак
		if (!extra_attack_used && ch->GetExtraVictim() && ch->get_wait() <= 0) {
			extra_attack_used = using_extra_attack(ch);
			if (extra_attack_used) {
				ch->SetExtraAttack(kExtraAttackUnused, 0);
			}
		}
		if (!extra_attack_used && AFF_FLAGGED(ch, EAffect::kHelper)) {
			// теперь чармис использует свои скилы
			using_charmice_skills(ch);
		}
	} else if (!no_extra_attack && !AFF_FLAGGED(ch, EAffect::kCharmed)) {
		//* применение скилов
		using_mob_skills(ch);
	}

	if (!ch->GetEnemy() || ch->in_room != IN_ROOM(ch->GetEnemy())) {
		return;
	}

	//**** удар основным оружием или рукой
	if (!AFF_FLAGGED(ch, EAffect::kStopRight) && !IS_SET(trigger_code, kNoRightHandAttack)) {
		exthit(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kMainHand);
	}

	//**** экстраатаки мобов. Первая - оффхэнд
	for (int i = 1; i <= ch->mob_specials.extra_attack; i++) {
		if (i == 1 && (AFF_FLAGGED(ch, EAffect::kStopLeft) || IS_SET(trigger_code, kNoLeftHandAttack))) {
			continue;
		}
		// если хп пробиты - уходим
		if (MOB_FLAGGED(ch, EMobFlag::kDecreaseAttack)) {
			if (ch->mob_specials.extra_attack * GET_HIT(ch) * 2 < i * GET_REAL_MAX_HIT(ch)) {
				return;
			}
		}
		exthit(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kMobAdd);
	}
}

void process_player_attack(CharData *ch, int min_init) {
	utils::CSteppedProfiler round_profiler("process player attack", 0.01);
	if (GET_POS(ch) > EPosition::kStun
		&& GET_POS(ch) < EPosition::kFight
		&& GET_AF_BATTLE(ch, kEafStand)) {
		sprintf(buf, "%sВам лучше встать на ноги!%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		SendMsgToChar(buf, ch);
		CLR_AF_BATTLE(ch, kEafStand);
	}

	// Срабатывание батл-триггеров амуниции
	Bitvector trigger_code = fight_otrigger(ch);

	//* каст заклинания
	round_profiler.next_step("Cast spell");
	if (ch->GetCastSpell() != ESpell::kUndefined && ch->get_wait() <= 0 && !IS_SET(trigger_code, kNoCastMagic)) {
		if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
			SendMsgToChar("Вы не смогли вымолвить и слова.\r\n", ch);
			ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		} else {
			CastSpell(ch, ch->GetCastChar(), ch->GetCastObj(), 0, ch->GetCastSpell(), ch->GetCastSubst());
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || ch->get_wait() > 0)) {
				SetWaitState(ch, kBattleRound);
			}
			ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		}
		if (ch->initiative > min_init) {
			--(ch->initiative);
			return;
		}
	}

	if (GET_AF_BATTLE(ch, kEafMultyparry))
		return;
	round_profiler.next_step("Use extra attack");
	//* применение экстра скилл-атак (пнуть, оглушить и прочая)
	if (!IS_SET(trigger_code, kNoExtraAttack) && ch->GetExtraVictim()
		&& ch->get_wait() <= 0 && using_extra_attack(ch)) {
		ch->SetExtraAttack(kExtraAttackUnused, nullptr);
		if (ch->initiative > min_init) {
			--(ch->initiative);
			return;
		}
	}

	if (!ch->GetEnemy()
		|| ch->in_room != IN_ROOM(ch->GetEnemy())) {
		return;
	}
	round_profiler.next_step("hit mainhand");
	//**** удар основным оружием или рукой
	if (GET_AF_BATTLE(ch, kEafFirst)) {
		if (!IS_SET(trigger_code, kNoRightHandAttack) && !AFF_FLAGGED(ch, EAffect::kStopRight)
			&& (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || !GET_AF_BATTLE(ch, kEafUsedright))) {
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
			exthit(ch, ch->GetEnemy(), tmpSkilltype, fight::AttackType::kMainHand);
		}
		round_profiler.next_step("hit add boths weapon");
// допатака двуручем
		if (!IS_SET(trigger_code, kNoExtraAttack) && GET_EQ(ch, EEquipPos::kBoths) 
			&& CanUseFeat(ch, EFeat::kTwohandsFocus)
			&& CanUseFeat(ch, EFeat::kSlashMaster)
			&& (static_cast<ESkill>(GET_OBJ_SKILL(GET_EQ(ch, EEquipPos::kBoths))) == ESkill::kTwohands)) {
			if (ch->GetSkill(ESkill::kTwohands) > (number(1, 500)))
				hit(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kMainHand);
		}
		CLR_AF_BATTLE(ch, kEafFirst);
		SET_AF_BATTLE(ch, kEafSecond);
		if (ch->initiative > min_init) {
			--(ch->initiative);
			return;
		}
	}
	round_profiler.next_step("hit offhand");
	//**** удар вторым оружием если оно есть и умение позволяет
	if (!IS_SET(trigger_code, kNoLeftHandAttack) && GET_EQ(ch, EEquipPos::kHold)
		&& GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kHold)) == EObjType::kWeapon
		&& GET_AF_BATTLE(ch, kEafSecond)
		&& !AFF_FLAGGED(ch, EAffect::kStopLeft)
		&& (IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| ch->GetSkill(ESkill::kSideAttack) > number(1, 101))) {
		if (IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| !GET_AF_BATTLE(ch, kEafUsedleft)) {
			exthit(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kOffHand);
		}
		CLR_AF_BATTLE(ch, kEafSecond);
	}
		//**** удар второй рукой если она свободна и умение позволяет
	else if (!IS_SET(trigger_code, kNoLeftHandAttack) && !GET_EQ(ch, EEquipPos::kHold)
		&& !GET_EQ(ch, EEquipPos::kLight) && !GET_EQ(ch, EEquipPos::kShield) && !GET_EQ(ch, EEquipPos::kBoths)
		&& !AFF_FLAGGED(ch, EAffect::kStopLeft) && GET_AF_BATTLE(ch, kEafSecond)
		&& ch->GetSkill(ESkill::kLeftHit)) {
		if (IS_IMMORTAL(ch) || !GET_AF_BATTLE(ch, kEafUsedleft)) {
			exthit(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kOffHand);
		}
		CLR_AF_BATTLE(ch, kEafSecond);
	}
	round_profiler.next_step("try angel");
	// немного коряво, т.к. зависит от инициативы кастера
	// check if angel is in fight, and go_rescue if it is not
	try_angel_rescue(ch);
}

// * \return false - чар не участвует в расчете инициативы
bool stuff_before_round(CharData *ch) {
	// Initialize initiative
	ch->initiative = 0;
	ch->battle_counter = 0;
	ch->round_counter += 1;
	DpsSystem::check_round(ch);
	BattleRoundParameters params(ch);
	handle_affects(params);
	round_num_mtrigger(ch, ch->GetEnemy());

	SET_AF_BATTLE(ch, kEafStand);
	if (IsAffectedBySpell(ch, ESpell::kSleep))
		SET_AF_BATTLE(ch, kEafSleep);
	if (ch->in_room == kNowhere)
		return false;

	if (AFF_FLAGGED(ch, EAffect::kHold)
		|| AFF_FLAGGED(ch, EAffect::kStopFight)
		|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)) {
		try_angel_rescue(ch);
		return false;
	}

	// Mobs stand up and players sit
	if (GET_POS(ch) < EPosition::kFight
		&& GET_POS(ch) > EPosition::kStun
		&& ch->get_wait() <= 0
		&& !AFF_FLAGGED(ch, EAffect::kHold)
		&& !AFF_FLAGGED(ch, EAffect::kSleep)) {
		stand_up_or_sit(ch);
	}

	// For NPC without lags and charms make it likes
	if (ch->IsNpc() && MAY_LIKES(ch))    // Get weapon from room
	{
		//edited by WorM 2010.09.03 добавил немного логики мобам в бою если
		// у моба есть что-то в инве то он может
		//переодется в это что-то если посчитает что это что-то ему лучше подходит
		if (!AFF_FLAGGED(ch, EAffect::kCharmed)) {
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
	utils::CSteppedProfiler round_profiler("Perform violence", 0.1);

	//* суммон хелперов
	sprintf(buf, "Check mob helpers");
	round_profiler.next_step(buf);
	check_mob_helpers();

	// храним список писей, которым надо показать состояние группы по msdp
	std::unordered_set<CharData *> msdp_report_chars;

	//* действия до раунда и расчет инициативы
	round_profiler.next_step("Calc initiative");
	for (CharData *ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;

		if (ch->desc) {
			msdp_report_chars.insert(ch);
		} else if (ch->has_master()
			&& (AFF_FLAGGED(ch, EAffect::kCharmed)
				|| MOB_FLAGGED(ch, EMobFlag::kTutelar)
				|| MOB_FLAGGED(ch, EMobFlag::kMentalShadow))) {
			auto master = ch->get_master();
			if (master->desc
				&& !master->GetEnemy()
				&& master->in_room == ch->in_room) {
				msdp_report_chars.insert(master);
			}
		}

		if (!stuff_before_round(ch)) {
			continue;
		}

		const int initiative = calc_initiative(ch, true);
		if (initiative > 100) { //откуда больше 100??????
			log("initiative calc: %s (%d) == %d", GET_NAME(ch), GET_MOB_VNUM(ch), initiative);
		}
		std::clamp(initiative, -100, 100);
		if (initiative == 0) {
			ch->initiative = -100; //Если кубик выпал в 0 - бей последним шанс 1 из 201
			min_init = MIN(min_init, -100);
		} else {
			ch->initiative = initiative;
		}

		SET_AF_BATTLE(ch, kEafFirst);
		max_init = MAX(max_init, initiative);
		min_init = MIN(min_init, initiative);
	}
	int size = 0;
	//* обработка раунда по очередности инициативы
	round_profiler.next_step("Round check");
	for (int initiative = max_init; initiative >= min_init; initiative--) {
		size = 0;
		for (CharData *ch = combat_list; ch; ch = next_combat_list) {
			next_combat_list = ch->next_fighting;
			size++;
			if (ch->initiative != initiative || ch->in_room == kNowhere) {
				continue;
			}

			// If mob cast 'hold' when initiative setted
			if (AFF_FLAGGED(ch, EAffect::kHold)
				|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)
				|| AFF_FLAGGED(ch, EAffect::kStopFight)
				|| !AWAKE(ch)) {
				continue;
			}
			// If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
			if (!ch->GetEnemy() || ch->in_room != IN_ROOM(ch->GetEnemy())) {
				continue;
			}
			//везде в стоп-файтах ставится инициатива равная 0, убираем двойную атаку
			if (initiative == 0) {
				continue;
			}
			utils::CExecutionTimer violence_timer;
			//* выполнение атак в раунде
			if (ch->IsNpc()) {
				process_npc_attack(ch);
			} else {
				process_player_attack(ch, min_init);
			}
			if (violence_timer.delta().count() > 0.01) {
				log("Process player attack, name %s, time %f", GET_NAME(ch), violence_timer.delta().count());
			}
		}
	}
	//* обновление аффектов и лагов после раунда
	round_profiler.next_step("Update round affs");
	update_round_affs();

	round_profiler.next_step("MSDP reports");
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
	if (ch->IsNpc()
		&& ch->has_master()
		&& (AFF_FLAGGED(ch, EAffect::kCharmed)
			|| MOB_FLAGGED(ch, EMobFlag::kTutelar)
			|| MOB_FLAGGED(ch, EMobFlag::kMentalShadow)
			|| IS_HORSE(ch))) {
		ch = ch->get_master();
	}

	if (victim->IsNpc()
		&& victim->has_master()
		&& (AFF_FLAGGED(victim, EAffect::kCharmed)
			|| MOB_FLAGGED(victim, EMobFlag::kTutelar)
			|| MOB_FLAGGED(victim, EMobFlag::kMentalShadow)
			|| IS_HORSE(victim))) {
		victim = victim->get_master();
	}

	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->has_master()) {
		if (cleader->IsNpc()
			&& !AFF_FLAGGED(cleader, EAffect::kCharmed)
			&& !(MOB_FLAGGED(cleader, EMobFlag::kTutelar) || MOB_FLAGGED(cleader, EMobFlag::kMentalShadow))
			&& !IS_HORSE(cleader)) {
			break;
		}
		cleader = cleader->get_master();
	}

	while (vleader->has_master()) {
		if (vleader->IsNpc()
			&& !AFF_FLAGGED(vleader, EAffect::kCharmed)
			&& !(MOB_FLAGGED(vleader, EMobFlag::kTutelar) || MOB_FLAGGED(vleader, EMobFlag::kMentalShadow))
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
		if (!AFF_FLAGGED(ch->get_master(), EAffect::kGroup)
			&& !ch->get_master()->IsNpc()) {
			ch = ch->get_master();
			continue;
		} else if (ch->get_master()->IsNpc()
			&& !AFF_FLAGGED(ch->get_master()->get_master(), EAffect::kGroup)
			&& !ch->get_master()->get_master()->IsNpc()
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
		if (!AFF_FLAGGED(victim->get_master(), EAffect::kGroup)
			&& !victim->get_master()->IsNpc()) {
			victim = victim->get_master();
			continue;
		} else if (victim->get_master()->IsNpc()
			&& !AFF_FLAGGED(victim->get_master()->get_master(), EAffect::kGroup)
			&& !victim->get_master()->get_master()->IsNpc()
			&& victim->get_master()->get_master()->has_master()) {
			victim = victim->get_master()->get_master();
			continue;
		} else {
			break;
		}
	}
	if (!AFF_FLAGGED(ch, EAffect::kGroup)
		|| cleader == victim) {
		stop_follower(ch, kSfEmpty);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, EAffect::kGroup)
		|| vleader == ch) {
		stop_follower(victim, kSfEmpty);
		return_value |= 2;
	}
	return return_value;
}

int calc_leadership(CharData *ch) {
	int prob, percent;
	CharData *leader = 0;

	if (ch->IsNpc()
		|| !AFF_FLAGGED(ch, EAffect::kGroup)
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

	if (!leader->GetSkill(ESkill::kLeadership)) {
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
