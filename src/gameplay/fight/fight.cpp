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
#include "utils/logger.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/initiative.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/skills/bash.h"
#include "gameplay/skills/kick.h"
#include "gameplay/skills/chopoff.h"
#include "gameplay/skills/disarm.h"
#include "gameplay/skills/throw.h"
#include "gameplay/skills/expendientcut.h"
#include "gameplay/skills/protect.h"
#include "gameplay/skills/ironwind.h"
#include "assist.h"
#include "engine/db/world_characters.h"
#include "fight_hit.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/char_handler.h"
#include "engine/entities/char_data.h"
#include "gameplay/affects/affect_messages.h"  // affects::AffectBuffKind
#include "gameplay/abilities/talents_actions.h"  // primary-affect lookup for the buff/debuff AI
#include "gameplay/mechanics/inventory.h"
#include "engine/ui/color.h"
#include "utils/random.h"
#include "engine/entities/char_player.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_rooms.h"  // ClassifyRoomAffectAccess
#include "engine/olc/olc.h"
#include "engine/network/msdp/msdp_constants.h"
#include "gameplay/magic/magic_items.h"
#include "engine/db/global_objects.h"
#include "gameplay/skills/slay.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/base_stats.h"
#include "utils/utils_time.h"
#include "gameplay/skills/leadership.h"
#include "gameplay/mechanics/tutelar.h"
#include "common.h"

#include <fmt/format.h>
#include "engine/observability/metrics.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/fight/fight_stuff.h"

// Structures
std::list<combat_list_element> combat_list;

//extern int r_helled_start_room;
//extern CharData *mob_proto;
//extern DescriptorData *descriptor_list;
// External procedures
// void do_assist(CharacterData *ch, char *argument, int cmd, int subcmd);
//void battle_affect_update(CharData *ch);
void go_rescue(CharData *ch, CharData *vict, CharData *tmp_ch);
void GoIntercept(CharData *ch, CharData *vict);
int npc_battle_scavenge(CharData *ch);
void npc_wield(CharData *ch);
void npc_armor(CharData *ch);

void go_autoassist(CharData *ch) {
	CharData *ch_lider = 0;
	if (ch->has_master()) {
		ch_lider = ch->get_master();
	} else {
		ch_lider = ch;    // Создаем ссылку на лидера
	}

	buf2[0] = '\0';
	for (auto *k : ch_lider->followers) {
		if (k->IsFlagged(EPrf::kAutoassist) &&
			(k->in_room == ch->in_room) && !k->GetEnemy() &&
			(k->GetPosition() == EPosition::kStand) && k->get_wait() <= 0) {
			// Здесь проверяем на кастеров
			if (IsCaster(k)) {
				// здесь проходим по чармисам кастера, и если находим их, то вписываем в драку
				for (auto *d : k->followers)
					if ((d->in_room == ch->in_room) && !d->GetEnemy() &&
						(d->GetPosition() == EPosition::kStand) && d->get_wait() <= 0)
						do_assist(d, buf2, 0, 0);
			} else {
				do_assist(k, buf2, 0, 0);
			}
		}
	}
}


// The Fight related routines

// Добавил проверку на лаг, чтобы правильно работали Каменное проклятие и
// Круг пустоты, ибо позицию делают меньше чем поз_станнед.
void update_pos(CharData *victim) {
	if ((victim->get_hit() > 0) && (victim->GetPosition() > EPosition::kStun)) {
		victim->SetPosition(victim->GetPosition());
		return;
	} else if (victim->get_hit() > 0
			&& victim->get_wait() <= 0
			&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
			&& !AFF_FLAGGED(victim, EAffect::kHold)) {
		victim->SetPosition(EPosition::kStand);
		SendMsgToChar("Вы встали.\r\n", victim);
		act("$n поднял$u.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
		return;
	}
	else if (victim->get_hit() <= -11)
		victim->SetPosition(EPosition::kDead);
	else if (victim->get_hit() <= -6)
		victim->SetPosition(EPosition::kPerish);
	else if (victim->get_hit() <= -1)
		victim->SetPosition(EPosition::kIncap);
	else if (victim->GetPosition() == EPosition::kIncap
				&& (victim->get_wait() > 0 || AFF_FLAGGED(victim, EAffect::kMagicStopFight)))
		victim->SetPosition(EPosition::kIncap);
	else
		victim->SetPosition(EPosition::kStun);

	// поплохело седоку или лошади - сбрасываем седока
	if (mount::IsOnHorse(victim) && victim->GetPosition() < EPosition::kFight)
		mount::DropFromHorse(victim);
	if (mount::IsHorse(victim) && victim->GetPosition() < EPosition::kFight && mount::IsOnHorse(victim->get_master()))
		mount::DropFromHorse(victim);
}

void set_battle_pos(CharData *ch) {
	switch (ch->GetPosition()) {
		case EPosition::kStand: ch->SetPosition(EPosition::kFight);
			break;
		case EPosition::kRest:
		case EPosition::kSit:
		case EPosition::kSleep:
			if (ch->get_wait() <= 0 &&
				!AFF_FLAGGED(ch, EAffect::kHold) && !AFF_FLAGGED(ch, EAffect::kSleep)
				&& !AFF_FLAGGED(ch, EAffect::kCharmed)) {
				if (ch->IsNpc()) {
					act("$n поднял$u.", false, ch, 0, 0, kToRoom | kToArenaListen);
					ch->SetPosition(EPosition::kFight);
				} else if (ch->GetPosition() == EPosition::kSleep) {
					act("Вы проснулись и сели.", false, ch, 0, 0, kToChar);
					act("$n проснул$u и сел$g.", false, ch, 0, 0, kToRoom | kToArenaListen);
					ch->SetPosition(EPosition::kSit);
				} else if (ch->GetPosition() == EPosition::kRest) {
					act("Вы прекратили отдых и сели.", false, ch, 0, 0, kToChar);
					act("$n прекратил$g отдых и сел$g.", false, ch, 0, 0, kToRoom | kToArenaListen);
					ch->SetPosition(EPosition::kSit);
				}
			}
			break;
		default:
			break;
	}
}

void restore_battle_pos(CharData *ch) {
	switch (ch->GetPosition()) {
		case EPosition::kFight: ch->SetPosition(EPosition::kStand);
			break;
		case EPosition::kRest:
		case EPosition::kSit:
		case EPosition::kSleep:
			if (ch->IsNpc() &&
				ch->get_wait() <= 0 &&
				!AFF_FLAGGED(ch, EAffect::kHold) && !AFF_FLAGGED(ch, EAffect::kSleep)
				&& !AFF_FLAGGED(ch, EAffect::kCharmed)) {
				act("$n поднял$u.", false, ch, 0, 0, kToRoom | kToArenaListen);
				ch->SetPosition(EPosition::kStand);
			}
			break;
		default:
			break;
	}
	if (AFF_FLAGGED(ch, EAffect::kSleep)) {
		ch->SetPosition(EPosition::kSleep);
	}
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

	if ((ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoFight)) || (vict->IsNpc() && vict->IsFlagged(EMobFlag::kNoFight)))
		return;

	// if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
	//    return;

	if (AFF_FLAGGED(ch, EAffect::kBandage)) {
		SendMsgToChar("Перевязка была прервана!\r\n", ch);
		RemoveAffectFromChar(ch, EAffect::kBandage);
		AFF_FLAGS(ch).unset(EAffect::kBandage);
	}
	if (AFF_FLAGGED(ch, EAffect::kMemorizeSpells)) {
		SendMsgToChar("Вы забыли о концентрации и ринулись в бой!\r\n", ch);
		RemoveAffectFromChar(ch, EAffect::kMemorizeSpells);
	}
	combat_list_element attaker;

	attaker.ch = ch;
	attaker.deleted = false;
	combat_list.push_front(attaker);

	if (AFF_FLAGGED(ch, EAffect::kSleep)) {
		RemoveAffectFromChar(ch, EAffect::kSleep);
		AFF_FLAGS(ch).unset(EAffect::kSleep);
	}
	ch->SetEnemy(vict);

	ch->battle_affects.clear();
	if (AFF_FLAGGED(ch, EAffect::kInvisible)) {
		ch->battle_affects.set(kEafInvisible);
	}
	ch->set_touching(nullptr);
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
			SetBattleLag(ch, (tmp.quot + 1));
		}
	}
	if (!ch->IsNpc() && (!GetSkill(ch, ESkill::kAwake))) {
		ch->UnsetFlag(EPrf::kAwake);
	}

	if (!ch->IsNpc() && (!GetSkill(ch, ESkill::kPunctual))) {
		ch->UnsetFlag(EPrf::kPunctual);
	}

	// Set combat style
	if (!AFF_FLAGGED(ch, EAffect::kCourage) && !AFF_FLAGGED(ch, EAffect::kDrunked)
		&& !AFF_FLAGGED(ch, EAffect::kAbstinent)) {
		if (ch->IsFlagged(EPrf::kPunctual))
			ch->battle_affects.set(kEafPunctual);
		else if (ch->IsFlagged(EPrf::kAwake))
			ch->battle_affects.set(kEafAwake);
	}

	if (CanUseFeat(ch, EFeat::kDefender) && GetSkill(ch, ESkill::kShieldBlock)) {
		ch->battle_affects.set(kEafAutoblock);
	}


//  check_killer(ch, vict);
}

// remove a char from the list of fighting entities
void stop_fighting(CharData *ch, int switch_others) {
	std::list<combat_list_element>::iterator found;

	for (auto &it : combat_list) {
		if (it.deleted)
			continue;
		if (it.ch  == ch) {
			it.deleted = true;
		}
	}
	ch->last_comm.clear();
	ch->set_touching(nullptr);
	ch->SetEnemy(nullptr);
	ch->initiative = 0;
	ch->battle_counter = 0;
	ch->round_counter = 0;
	ch->SetExtraAttack(kExtraAttackUnused, nullptr);
	ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, nullptr, nullptr, nullptr);
	restore_battle_pos(ch);
	if (ch->battle_affects.get(kEafInvisible)) {
		AFF_FLAGS(ch).set(EAffect::kInvisible);
	}
	ch->battle_affects.clear();
	DpsSystem::check_round(ch);
	if (switch_others != 2) {
		for (auto &temp : combat_list) {
			if (temp.deleted)
				continue;
			if (temp.ch->get_touching() == ch) {
				temp.ch->set_touching(nullptr);
				temp.ch->battle_affects.unset(kEafTouch);
			}
			if (temp.ch->GetExtraVictim() == ch)
				temp.ch->SetExtraAttack(kExtraAttackUnused, nullptr);
			if (temp.ch->GetCastChar() == ch)
				temp.ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
			if (temp.ch->GetEnemy() == ch && switch_others) {
				for (found = combat_list.begin(); found != combat_list.end(); found++) {
					if ((*found).deleted)
						continue;
					if ((*found).ch != ch && (*found).ch->GetEnemy() == temp.ch) {
						if (!temp.ch->IsNpc()) {
							act("Вы переключили свое внимание на $N3.", false, temp.ch, 0, (*found).ch, kToChar);
						}
						log("[Stop fighting] %s : Change victim for fighting on %s", GET_NAME(temp.ch), (*found).ch->get_name().c_str());
						temp.ch->SetEnemy((*found).ch);
						break;
					}
				}
				if (found == combat_list.end()) {
					stop_fighting(temp.ch, false);
				}
			}
		}

 		update_pos(ch);
		// проверка скилла "железный ветер" - снимаем флаг по окончанию боя
		if ((ch->GetEnemy() == nullptr) && ch->IsFlagged(EPrf::kIronWind)) {
			ch->UnsetFlag(EPrf::kIronWind);
			if (ch->GetPosition() > EPosition::kIncap) {
				SendMsgToChar("Безумие боя отпустило вас, и враз навалилась усталость...\r\n", ch);
				act("$n шумно выдохнул$g и остановил$u, переводя дух после боя.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			}
		}
	}
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
		|| ch->get_wait() > 0)
		return 0;
	else
		return privilege::IsImmortal(ch) ? 1 : ch->caster_level;
}

int get_hp_perc(CharData *ch) {
	return int(ch->get_hit() * 100 / ch->get_max_hit());
}

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

	for (const auto ch : world[npc->in_room]->people) {
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
		case ESpell::kGreatHeal:
		case ESpell::kGroupHeal: AFF_USED = 30;
			break;
		default: break;
	}

	if (caster->IsFlagged(EMobFlag::kCompanion) // ()
		&& AFF_FLAGGED(caster, EAffect::kHelper)) {
		if (get_hp_perc(caster) < AFF_USED) {
			return caster;
		} else if (caster->has_master()
			&& sight::CanSee(caster, caster->get_master())
			&& caster->get_master()->in_room == caster->in_room
			&& caster->get_master()->GetEnemy()
			&& get_hp_perc(caster->get_master()) < AFF_USED) {
			return caster->get_master();
		}
		return nullptr;
	}

	for (const auto vict : world[caster->in_room]->people) {
		if (!vict->IsNpc()
			|| AFF_FLAGGED(vict, EAffect::kCharmed)
			|| (vict->IsFlagged(EMobFlag::kCompanion) // ()
				&& vict->has_master()
				&& !vict->get_master()->IsNpc())
			|| !sight::CanSee(caster, vict)) {
			continue;
		}

		if (!vict->GetEnemy() && !vict->IsFlagged(EMobFlag::kHelper)) {
			continue;
		}

		if (get_hp_perc(vict) < AFF_USED && (!victim || vict_val > get_hp_perc(vict))) {
			victim = vict;
			vict_val = get_hp_perc(vict);
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
		case ESpell::kCureFever: AFF_USED.push_back(EAffect::kFever);
			break;
		default: break;
	}
	if (AFF_FLAGGED(caster, EAffect::kHelper)
		&& caster->IsFlagged(EMobFlag::kCompanion)) { //()
		if (caster->has_any_affect(AFF_USED)) {
			return caster;
		} else if (caster->has_master()
			&& sight::CanSee(caster, caster->get_master())
			&& caster->get_master()->in_room == caster->in_room
			&& caster->get_master()->has_any_affect(AFF_USED)) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (!AFF_USED.empty()) {
		for (const auto vict : world[caster->in_room]->people) {
			if (!vict->IsNpc() || AFF_FLAGGED(vict, EAffect::kCharmed) ||
				(vict->IsFlagged(EMobFlag::kCompanion) // ()
					&& vict->get_master()
					&& !vict->get_master()->IsNpc())
				|| !sight::CanSee(caster, vict)) {
				continue;
			}

			if (!vict->has_any_affect(AFF_USED)) {
				continue;
			}

			if (!vict->GetEnemy()
				&& !vict->IsFlagged(EMobFlag::kHelper)) {
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
		case ESpell::kCureFever: AFF_USED.push_back(EAffect::kFever);
			break;
		default: break;
	}

	if (AFF_FLAGGED(caster, EAffect::kHelper)
		&& caster->IsFlagged(EMobFlag::kCompanion)) { // ()
		if (caster->has_any_affect(AFF_USED)) {
			return caster;
		} else if (caster->has_master()
			&& sight::CanSee(caster, caster->get_master())
			&& caster->get_master()->in_room == caster->in_room
			&& caster->get_master()->has_any_affect(AFF_USED)) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (!AFF_USED.empty()) {
		for (const auto vict : world[caster->in_room]->people) {
			if (!vict->IsNpc()
				|| AFF_FLAGGED(vict, EAffect::kCharmed)
				|| (vict->IsFlagged(EMobFlag::kCompanion) // ()
					&& (vict->get_master() && !vict->get_master()->IsNpc()))
				|| !sight::CanSee(caster, vict)) {
				continue;
			}

			if (!vict->has_any_affect(AFF_USED)) {
				continue;
			}

			if (!vict->GetEnemy()
				&& !vict->IsFlagged(EMobFlag::kHelper)) {
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

// issue.affect-migration: the primary affect a single-target spell imposes. The mob buff/debuff AI
// uses it to ask "is the target already under this spell's effect?" via IsAffected (affect identity),
// instead of the retired IsAffectedBySpell (which keyed on the casting spell). kUndefined when the
// spell imposes no affect -- callers treat that as "can't tell -> not affected".
static EAffect PrimaryAffectOf(ESpell spell) {
	const auto &info = MUD::Spell(spell);
	if (info.actions.Contains(talents_actions::EAction::kAffect)) {
		const auto &applies = info.actions.GetAffect().GetApplies();
		if (!applies.empty()) {
			return applies.front().id;
		}
	}
	return EAffect::kUndefined;
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

	const EAffect want = PrimaryAffectOf(spellreal);

	if (caster->IsFlagged(EMobFlag::kCompanion) &&
		AFF_FLAGGED(caster, EAffect::kHelper)) {
		if (want == EAffect::kUndefined || !IsAffected(caster, want)) {
			return caster;
		} else if (caster->has_master()
			&& sight::CanSee(caster, caster->get_master())
			&& caster->get_master()->in_room == caster->in_room
			&& caster->get_master()->GetEnemy()
			&& (want == EAffect::kUndefined || !IsAffected(caster->get_master(), want))) {
			return caster->get_master();
		}

		return nullptr;
	}

	if (GetRealInt(caster) > number(5, 15)) {
		for (const auto vict : world[caster->in_room]->people) {
			if (!vict->IsNpc()
				|| AFF_FLAGGED(vict, EAffect::kCharmed)
				|| (vict->IsFlagged(EMobFlag::kCompanion) // ()
					&& vict->has_master()
					&& !vict->get_master()->IsNpc())
				|| !sight::CanSee(caster, vict)) {
				continue;
			}

			if (!vict->GetEnemy()
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| (want != EAffect::kUndefined && IsAffected(vict, want))) {
				continue;
			}

			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	}

	if (!victim && (want == EAffect::kUndefined || !IsAffected(caster, want))) {
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
		spellreal = ESpell::kEntangleEnemy;

	const EAffect want = PrimaryAffectOf(spellreal);

	if (GetRealInt(caster) > number(10, 20)) {
		for (const auto vict : world[caster->in_room]->people) {
			if ((vict->IsNpc()
				&& !(vict->IsFlagged(EMobFlag::kCompanion)
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !sight::CanSee(caster, vict)) {
				continue;
			}

			if ((!vict->GetEnemy()
				&& (GetRealInt(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true)))
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| (want != EAffect::kUndefined && IsAffected(vict, want))) {
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
		&& (want == EAffect::kUndefined || !IsAffected(caster->GetEnemy(), want))) {
		victim = caster->GetEnemy();
	}

	return victim;
}

// issue.mob-ai-improve part 2: does the mob's room carry a FOREIGN (non-ally-authored)
// dispellable ward -- a player's seal/storm/trap -- worth clearing? Reuses the room
// author/ally classifier: a ward the mob (or its ally) placed is not "hostile".
bool HasHostileRoomWard(CharData *ch) {
	if (ch->in_room == kNowhere) {
		return false;
	}
	for (const auto &aff : world[ch->in_room]->affected) {
		if (aff && IS_SET(aff->battleflag, kAfDispellable)
			&& !room_spells::ClassifyRoomAffectAccess(ch, aff->caster_id).free) {
			return true;
		}
	}
	return false;
}

// issue.mob-ai-improve: does `vict` carry a dispellable BUFF -- a kAfDispellable affect declared a
// buff by its own buff flag? Debuffs (and ambiguous affects) were imposed violently / are not clean
// buffs; stripping them would help the target, so an offensive dispel ignores them.
bool HasDispellableBuff(const CharData *vict) {
	for (const auto &aff : vict->affected) {
		if (aff && affects::AffectBuffKind(aff->affect_type) == affects::EBuff::kYes
				&& IS_SET(aff->battleflag, kAfDispellable)) {
			return true;
		}
	}
	return false;
}

// issue.mob-ai-improve: pick a room opponent carrying a dispellable buff, for an offensive
// dispel (dispel magic). Mirrors find_opp_affectee's opponent-eligibility guards but selects
// on "has a buff to strip" instead of "not yet affected by this spell".
CharData *find_opp_dispellable(CharData *caster) {
	CharData *victim = nullptr;
	int vict_val = 0;

	if (GetRealInt(caster) > number(10, 20)) {
		for (const auto vict : world[caster->in_room]->people) {
			if ((vict->IsNpc()
				&& !(vict->IsFlagged(EMobFlag::kCompanion)
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !sight::CanSee(caster, vict)) {
				continue;
			}
			if ((!vict->GetEnemy()
				&& (GetRealInt(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true)))
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| !HasDispellableBuff(vict)) {
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
		&& HasDispellableBuff(caster->GetEnemy())) {
		victim = caster->GetEnemy();
	}

	return victim;
}

CharData *find_opp_caster(CharData *caster) {
	CharData *victim = nullptr;
	int vict_val = 0;

	for (const auto vict : world[caster->in_room]->people) {
		if (vict->IsNpc()
			&& !(vict->IsFlagged(EMobFlag::kCompanion) //
				&& vict->has_master()
				&& !vict->get_master()->IsNpc())) {
			continue;
		}
		if ((!vict->GetEnemy()
			&& (GetRealInt(caster) < number(15, 25)
				|| !in_same_battle(caster, vict, true)))
			|| AFF_FLAGGED(vict, EAffect::kHold) || AFF_FLAGGED(vict, EAffect::kSilence)
			|| (!sight::CanSee(caster, vict) && caster->GetEnemy() != vict))
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
		for (const auto vict : world[caster->in_room]->people) {
			if ((vict->IsNpc()
				&& !(vict->IsFlagged(EMobFlag::kCompanion)
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !sight::CanSee(caster, vict)) {
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

	if (mobINT < mob_ai::kStupidMob) {
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
		if ((vict->IsNpc() && !IsCharmice(vict))
			|| (IsCharmice(vict) && !vict->GetEnemy()
				&& mob_ai::find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
			|| vict->IsFlagged(EPrf::kNohassle)
			|| !sight::MaySee(ch, ch, vict)) {
			continue;
		}

		if (!sight::CanSee(ch, vict)) {
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

		if (vict->get_hit() <= mob_ai::kCharacterHpForMobPriorityAttack) {
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

	if (mobINT < mob_ai::kMiddleAi) {
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

	if (mobINT < mob_ai::kHighAi) {
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
		for (const auto vict : world[caster->in_room]->people) {
			if ((vict->IsNpc()
				&& !(vict->IsFlagged(EMobFlag::kCompanion)
					&& vict->has_master()
					&& !vict->get_master()->IsNpc()))
				|| !sight::CanSee(caster, vict)) {
				continue;
			}

			if (!vict->GetEnemy()
				&& (GetRealInt(caster) < number(20, 27)
					|| !in_same_battle(caster, vict, true))) {
				continue;
			}

			if (!victim || vict_val > vict->get_hit()) {
				victim = vict;
				vict_val = vict->get_hit();
			}
		}
	}

	if (!victim) {
		victim = caster->GetEnemy();
	}

	return victim;
}

CharData *find_cure(CharData *caster, CharData *patient, ESpell &spell_id) {
	if (get_hp_perc(patient) <= number(20, 33)) {
		if (GET_SPELL_MEM(caster, ESpell::kExtraHits))
			spell_id = ESpell::kExtraHits;
		else if (GET_SPELL_MEM(caster, ESpell::kHeal))
			spell_id = ESpell::kHeal;
		else if (GET_SPELL_MEM(caster, ESpell::kCureCritic))
			spell_id = ESpell::kCureCritic;
		else if (GET_SPELL_MEM(caster, ESpell::kGroupHeal))
			spell_id = ESpell::kGroupHeal;
	} else if (get_hp_perc(patient) <= number(50, 65)) {
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
		&& ch->IsFlagged(ENpcFlag::kUsingMagicItems)	// issue.npc-races: race grants magic-item use
		&& !(ch->IsFlagged(EMobFlag::kTutelar) || ch->IsFlagged(EMobFlag::kMentalShadow))) {
		switch (item->get_type()) {
			case EObjType::kWand:
			case EObjType::kStaff: {
				// issue.magic-items: заклинание и заряды лежат в extra_values, val[] у этих типов нулевые
				const int spell_num = item->GetSpellItemSpellNum(1);
				if (spell_num <= 0) {
					break;   // заклинание не задано -- посох просто не боевой, это не поломка
				}
				const auto spell_id = static_cast<ESpell>(spell_num);
				if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
					mudlog(fmt::format("SYSERR: неверное заклинание в посохе {} #{}, позиция 1, значение {}",
							item->get_PName(grammar::ECase::kNom), GET_OBJ_VNUM(item),
							spell_num), BRF, kLvlImmortal, SYSLOG, true);
					break;
				}

				if (item->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) > 0
					&& MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE)) {
					battle_spells[spells++] = spell_id;
				}
				break;
			}
			case EObjType::kPotion: {
				for (int i = 1; i <= 3; i++) {
					const int spell_num = item->GetSpellItemSpellNum(i);
					if (spell_num <= 0) {
						continue;   // пустой слот -- это норма, заклинание может быть и одно
					}
					const auto spell_id = static_cast<ESpell>(spell_num);
					if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
						mudlog(fmt::format("SYSERR: неверное заклинание в напитке {} #{}, позиция {}, значение {}",
								item->get_PName(grammar::ECase::kNom), GET_OBJ_VNUM(item), i,
								spell_num), BRF, kLvlImmortal, SYSLOG, true);
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
					const int spell_num = item->GetSpellItemSpellNum(i);
					if (spell_num <= 0) {
						continue;   // пустой слот -- это норма, заклинание может быть и одно
					}
					const auto spell_id = static_cast<ESpell>(spell_num);
					if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
						mudlog(fmt::format("SYSERR: неверное заклинание в свитке {} #{}, позиция {}, значение {}",
								item->get_PName(grammar::ECase::kNom), GET_OBJ_VNUM(item), i,
								spell_num), BRF, kLvlImmortal, SYSLOG, true);
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
	if (ch->IsFlagged(EMobFlag::kTutelar)) {
		if (ch->has_master() && ch->in_room != ch->get_master()->in_room) {
			sprintf(buf, "%s тоскливо сморит по сторонам. Кажется ищет кого-то.", ch->get_name_str().c_str());
			act(buf, false, ch, 0, 0, kToRoom | kToArenaListen);
			return;
		}
	}
	// Ищем рандомную заклинашку и цель для нее
	// issue.mob-ai-improve part 2: a mob that knows dispel magic and is caught inside a hostile
	// dispellable room ward (e.g. a player's seal that blocks its reinforcements) tries to clear
	// it. Cast with no char target -> CallMagic routes it to the room dispel (author/ally +
	// strength contest). Coin-flip per round so a mob that can't break a strong ward still fights.
	if (!victim
		&& !AFF_FLAGGED(ch, EAffect::kCharmed)
		&& GET_SPELL_MEM(ch, ESpell::kDispellMagic) > 0
		&& GetRealInt(ch) > number(10, 20)
		&& number(0, 1)
		&& HasHostileRoomWard(ch)) {
		CastSpell(ch, nullptr, 0, nullptr, ESpell::kDispellMagic, ESpell::kDispellMagic);
		return;
	}
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
			} else if (MUD::Spell(spell_id_2).IsFlagged(kNpcUnaffectPc)) {
				// issue.mob-ai-improve: offensive dispel -- strip an enemy player's buffs.
				if (!AFF_FLAGGED(ch, EAffect::kCharmed))
					victim = find_opp_dispellable(ch);
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
		while (!ch->IsFlagged(EMobFlag::kCompanion)
			&& item
			&& GET_RACE(ch) == ENpcRace::kHuman) {
			switch (item->get_type()) {
				case EObjType::kWand:
				case EObjType::kStaff:
					if (item->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) > 0
						&& static_cast<ESpell>(item->GetSpellItemSpellNum(1)) == spell_id_2) {
						EmployMagicItem(ch, item, GET_NAME(victim));
						return;
					}
					break;

				case EObjType::kPotion:
					for (int i = 1; i <= 3; i++) {
						if (static_cast<ESpell>(item->GetSpellItemSpellNum(i)) == spell_id_2) {
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
						if (static_cast<ESpell>(item->GetSpellItemSpellNum(i)) == spell_id_2) {
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

void summon_mob_helpers(CharData *ch) {
	if (ch->summon_helpers.empty()) {
		return;
	}
	// issue.unstable-hotfix: a mob in lag (SetBattleLag -> get_wait) cannot take active actions, so it
	// must not call for helpers. Return WITHOUT clearing summon_helpers, so the shout is merely deferred
	// until the lag ends -- check_mob_helpers re-runs this each combat round.
	if (ch->get_wait() > 0) {
		return;
	}
	for (auto helpee :ch->summon_helpers) {
		// Start_fight_mtrigger using inside this loop
		// So we have to iterate on copy list
		Characters::list_t mobs;
		character_list.get_mobs_by_vnum(helpee, mobs);
		for (const auto &vict : mobs) {
			if (AFF_FLAGGED(ch, EAffect::kCharmed)
				|| AFF_FLAGGED(vict, EAffect::kHold)
				|| AFF_FLAGGED(vict, EAffect::kCharmed)
				|| AFF_FLAGGED(vict, EAffect::kBlind)
				|| vict->get_wait() > 0
				|| vict->GetPosition() < EPosition::kStand
				|| vict->in_room == kNowhere
				|| vict->GetEnemy()) {
				continue;
			}
			vict->SetFlag(EMobFlag::kHelper);
			if (IsAbleToSay(ch)) {	// issue.npc-races: any vocal race calls for help
				act("$n воззвал$g : \"На помощь, мои верные соратники!\"",
					false, ch, 0, 0, kToRoom | kToArenaListen);
			}
			if (vict->in_room != ch->in_room) {
				act("$n был$g призван$g $N4.", false, vict.get(), 0, ch, kToRoom | kToArenaListen);
				char_from_room(vict);
				char_to_room(vict, ch->in_room);
				act("$n прибыл$g на зов и вступил$g в битву на стороне $N1.", false, vict.get(), 0, ch, kToRoom | kToArenaListen);
			} else {
				act("$n вступил$g в битву на стороне $N1.", false, vict.get(), 0, ch, kToRoom | kToArenaListen);
			}
			if (MayAttack(vict)) {
				set_fighting(vict, ch->GetEnemy());
			}
		}
	}
	ch->summon_helpers.clear();
}

void check_mob_helpers() {
	for (auto &it : combat_list) {
		if (it.deleted)
			continue;
		// Extract battler if no opponent
		if (it.ch->GetEnemy() == nullptr
			|| it.ch->in_room != it.ch->GetEnemy()->in_room
			|| it.ch->in_room == kNowhere) {
			stop_fighting(it.ch, true);
			continue;
		}
		if (AFF_FLAGGED(it.ch, EAffect::kHold)
			|| !it.ch->IsNpc()
			|| it.ch->get_wait() > 0
			|| it.ch->GetPosition() < EPosition::kFight
			|| AFF_FLAGGED(it.ch, EAffect::kCharmed)
			|| AFF_FLAGGED(it.ch, EAffect::kMagicStopFight)
			|| AFF_FLAGGED(it.ch, EAffect::kStopFight)
			|| AFF_FLAGGED(it.ch, EAffect::kSilence)
			|| it.ch->GetEnemy()->IsFlagged(EPrf::kNohassle)) {
			continue;
		}
		summon_mob_helpers(it.ch);
	}
}

void stand_up_or_sit(CharData *ch) {
	if (ch->IsNpc()) {
		act("$n поднял$u.", true, ch, 0, 0, kToRoom | kToArenaListen);
		ch->SetPosition(EPosition::kFight);
	} else if (ch->GetPosition() == EPosition::kSleep) {
		act("Вы проснулись и сели.", true, ch, 0, 0, kToChar);
		act("$n проснул$u и сел$g.", true, ch, 0, 0, kToRoom | kToArenaListen);
		ch->SetPosition(EPosition::kSit);
	} else if (ch->GetPosition() == EPosition::kRest) {
		act("Вы прекратили отдых и сели.", true, ch, 0, 0, kToChar);
		act("$n прекратил$g отдых и сел$g.", true, ch, 0, 0, kToRoom | kToArenaListen);
		ch->SetPosition(EPosition::kSit);
	}
}

void set_mob_skills_flags(CharData *ch) {
	bool sk_use = false;
	// 1) parry
	int do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && GetSkill(ch, ESkill::kParry)) {
		ch->battle_affects.set(kEafParry);
		sk_use = true;
	}
	// 2) blocking
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && GetSkill(ch, ESkill::kShieldBlock)) {
		ch->battle_affects.set(kEafBlock);
		sk_use = true;
	}
	// 3) multyparry
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && GetSkill(ch, ESkill::kMultiparry)) {
		ch->battle_affects.set(kEafMultyparry);
		sk_use = true;
	}
	// 4) deviate
	do_this = number(0, 100);
	if (!sk_use && do_this <= GET_LIKES(ch) && GetSkill(ch, ESkill::kDodge)) {
		ch->battle_affects.set(kEafDodge);
		sk_use = true;
	}
	// 5) styles
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && GetSkill(ch, ESkill::kAwake) > number(1, 101)) {
		ch->battle_affects.set(kEafAwake);
	} else {
		ch->battle_affects.unset(kEafAwake);
	}
	do_this = number(0, 100);
	if (do_this <= GET_LIKES(ch) && GetSkill(ch, ESkill::kPunctual) > number(1, 101)) {
		ch->battle_affects.set(kEafPunctual);
	} else {
		ch->battle_affects.unset(kEafPunctual);
	}
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
	
	if (charmice_wielded_for_stupor && GetSkill(ch, ESkill::kOverwhelm) > 0) { // оглушить
		const bool skill_ready = ch->Skills().GetCooldown(ESkill::kGlobalCooldown) <= 0 && ch->Skills().GetCooldown(ESkill::kOverwhelm) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует оглушение: " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			SendToTC(master, true, true, true, "%s", msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			ch->battle_affects.set(kEafOverwhelm);
		}
	} else if (charmice_not_wielded && GetSkill(ch, ESkill::kHammer) > 0) { // молот
		const bool skill_ready = ch->Skills().GetCooldown(ESkill::kGlobalCooldown) <= 0 && ch->Skills().GetCooldown(ESkill::kHammer) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует богатырский молот: " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			SendToTC(master, true, true, true, "%s", msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			ch->battle_affects.set(kEafHammer);
		}
	} else if(charmice_wielded_for_throw && (GetSkill(ch, ESkill::kThrow) > GetSkill(ch, ESkill::kOverwhelm))) { // метнуть ()
			const bool skill_ready = ch->Skills().GetCooldown(ESkill::kGlobalCooldown) <= 0 && ch->Skills().GetCooldown(ESkill::kThrow) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует метнуть : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			SendToTC(master, true, true, true, "%s", msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			ch->SetExtraAttack(kExtraAttackThrow, ch->GetEnemy());
		}
	} else if (!charmice_wielded_for_throw && (ch->get_extra_attack_mode() != kExtraAttackThrow)
			&& !(ch->battle_affects.get(kEafOverwhelm) || ch->battle_affects.get(kEafHammer)) && GetSkill(ch, ESkill::kChopoff) > 0) { // подножка ()
		const bool skill_ready = ch->Skills().GetCooldown(ESkill::kGlobalCooldown) <= 0 && ch->Skills().GetCooldown(ESkill::kChopoff) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует подножку : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			SendToTC(master, true, true, true, "%s", msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			if (ch->GetPosition() < EPosition::kFight) return;
			ch->SetExtraAttack(kExtraAttackChopoff, ch->GetEnemy());
		} 
	}   else if (((ch->get_extra_attack_mode() != kExtraAttackThrow) || (ch->get_extra_attack_mode() != kExtraAttackChopoff))
			&& !(ch->battle_affects.get(kEafOverwhelm) || ch->battle_affects.get(kEafHammer)) && GetSkill(ch, ESkill::kIronwind) > 0) {  // вихрь ()
		const bool skill_ready = ch->Skills().GetCooldown(ESkill::kGlobalCooldown) <= 0 && ch->Skills().GetCooldown(ESkill::kIronwind) <= 0;
		if (master) {
			std::stringstream msg;
			msg << ch->get_name() << " использует ВИХРЬ : " << ((do_skill_without_command && skill_ready) ? "ДА" : "НЕТ") << "\r\n";
			msg << "Проверка шанса применения: " << (do_skill_without_command ? "ДА" : "НЕТ");
			msg << ", скилл откатился: " << (skill_ready ? "ДА" : "НЕТ") << "\r\n";
			SendToTC(master, true, true, true, "%s", msg.str().c_str());
		}
		if (do_skill_without_command && skill_ready) {
			if (ch->GetPosition() < EPosition::kFight) return;
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
		if (GetSkill(ch, sk_num) <= 0) {
			sk_num = ESkill::kUndefined;
		}
		if (ch->Skills().HasActiveCooldown(sk_num)) {
			continue;
		}
		////////////////////////////////////////////////////////////////////////
		// для глуша и молота выставляем соотвествующие флаги
		// цель не выбираем чтобы избежать переключения у мобов, которые не могут переключаться
		if (sk_num == ESkill::kHammer) {
			sk_use = 0;
			ch->battle_affects.set(kEafHammer);
		}
		if (sk_num == ESkill::kOverwhelm) {
			sk_use = 0;
			ch->battle_affects.set(kEafOverwhelm);
		}

		////////////////////////////////////////////////////////////////////////
		if (sk_num == ESkill::kIntercept) {
			sk_use = 0;
			GoIntercept(ch, ch->GetEnemy());
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
				GoThrow(ch, caster);
			}
		}

		////////////////////////////////////////////////////////////////////////
		// проверим на всякий случай, является ли моб ангелом,
		// хотя вроде бы этого делать не надо
		if (!(ch->IsFlagged(EMobFlag::kTutelar) || ch->IsFlagged(EMobFlag::kMentalShadow))
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
						&& !(attacker->IsFlagged(EMobFlag::kCompanion)
							&& attacker->has_master()
							&& !attacker->get_master()->IsNpc()))
					|| !sight::CanSee(ch, vict) // не видно, кого нужно спасать
					|| ch == vict) // себя спасать не нужно
				{
					continue;
				}

				// Буду спасать vict от attacker
				if (!caster    // еще пока никого не спасаю
					|| (vict->get_hit() < caster->get_hit()))    // этому мобу хуже
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
					if ((AFF_FLAGGED(vict, EAffect::kHold) && vict->GetPosition() < EPosition::kFight)
						|| (IsCaster(vict)
							&& (AFF_FLAGGED(vict, EAffect::kHold)
								|| AFF_FLAGGED(vict, EAffect::kSilence)
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
				&& (sight::CanSee(ch, caster) || ch->GetEnemy() == caster)
				&& caster->caster_level > POOR_CASTER
				&& (sk_num == ESkill::kBash || sk_num == ESkill::kChopoff)) {
				if (sk_num == ESkill::kBash) {
//SendMsgToChar(caster, "Баш предфункция\r\n");
//sprintf(buf, "%s башат предфункция\r\n",GET_NAME(caster));
//mudlog(buf, LGH, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
					if (caster->GetPosition() >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kBash, caster) > number(50, 80)) {
						sk_use = 0;
						go_bash(ch, caster);
					}
				} else {
//SendMsgToChar(caster, "Подножка предфункция\r\n");
//sprintf(buf, "%s подсекают предфункция\r\n",GET_NAME(caster));
//                mudlog(buf, LGH, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);

					if (caster->GetPosition() >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kChopoff, caster) > number(50, 80)) {
						sk_use = 0;
						go_chopoff(ch, caster);
					}
				}
			}

			if (sk_use
				&& damager
				&& (sight::CanSee(ch, damager)
					|| ch->GetEnemy() == damager)) {
				if (sk_num == ESkill::kBash) {
					if (mount::IsOnHorse(damager)) {
						// Карачун. Правка бага. Лошадь не должна башить себя, если дерется с наездником.
						if (mount::GetHorse(damager) == ch) {
							mount::DropFromHorse(ch);
						} else {
							sk_use = 0;
							if (!mount::GetHorse(damager)->IsFlagged(EMobFlag::kNoFight)) {
								go_bash(ch, mount::GetHorse(damager));
							} else {
								go_bash(ch, damager);
							}
						}
					} else if (damager->GetPosition() >= EPosition::kFight
						|| CalcCurrentSkill(ch, ESkill::kBash, damager) > number(50, 80)) {
						sk_use = 0;
						go_bash(ch, damager);
					}
				} else if (sk_num == ESkill::kChopoff) {
					if (mount::IsOnHorse(damager)) {
						sk_use = 0;
						if (!mount::GetHorse(damager)->IsFlagged(EMobFlag::kNoFight)) {
							go_chopoff(ch, mount::GetHorse(damager));
						} else {
							go_chopoff(ch, damager);
						}
					} else if (damager->GetPosition() >= EPosition::kFight
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
		if (sk_num == ESkill::kKick && !mount::IsOnHorse(ch->GetEnemy())) {
			sk_use = 0;
			go_kick(ch, ch->GetEnemy());
		}
	}
}

void add_attackers_round(CharData *ch) {
	for (const auto i : world[ch->in_room]->people) {
		if (!i->IsNpc() && i->desc) {
			ch->mark_attacked(i);
		}
	}
}

void update_round_affs() {
	for (auto &it : combat_list) {
		if (it.deleted)
			continue;
		if (it.ch->in_room == kNowhere)
			continue;

		it.ch->battle_affects.unset(kEafFirst);
		it.ch->battle_affects.unset(kEafSecond);
		it.ch->battle_affects.unset(kEafUsedleft);
		it.ch->battle_affects.unset(kEafUsedright);
		it.ch->battle_affects.unset(kEafMultyparry);
		it.ch->battle_affects.unset(kEafDodge);
		it.ch->battle_affects.unset(kEafTouch);
		if (it.ch->get_touching())
			it.ch->set_touching(0);

		if (it.ch->battle_affects.get(kEafSleep)) {
			RemoveAffectFromChar(it.ch, EAffect::kSleep);
			AFF_FLAGS(it.ch).unset(EAffect::kSleep);
		}
		if (it.ch->battle_affects.get(kEafBlock)) {
			it.ch->battle_affects.unset(kEafBlock);
			if (!privilege::IsImmortal(it.ch) && it.ch->get_wait() < kBattleRound)
				SetBattleLag(it.ch, 1);
		}

		if (it.ch->battle_affects.get(kEafPoisoned)) {
			it.ch->battle_affects.unset(kEafPoisoned);
		}

		battle_affect_update(it.ch);

		if (it.ch->IsNpc() && !IsCharmice(it.ch)) {
			add_attackers_round(it.ch);
		}
	}
}

bool using_extra_attack(CharData *ch) {
	bool used = false;

	switch (ch->get_extra_attack_mode()) {
		case kExtraAttackThrow: GoThrow(ch, ch->GetExtraVictim());
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
		case kExtraAttackInjure: go_injure(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackCut: GoExpedientCut(ch, ch->GetExtraVictim());
			used = true;
			break;
		case kExtraAttackSlay: go_slay(ch, ch->GetExtraVictim());
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
		mob_ai::perform_mob_switch(ch);
	}

	// Cast spells
	if (MAY_LIKES(ch) && !IS_SET(trigger_code, kNoCastMagic))
		mob_casting(ch);

	if (!ch->GetEnemy()
		|| ch->in_room != ch->GetEnemy()->in_room
		|| AFF_FLAGGED(ch, EAffect::kHold)
			// mob_casting мог от зеркала отразиться
		|| AFF_FLAGGED(ch, EAffect::kStopFight)
		|| !AWAKE(ch)
		|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)) {
		return;
	}

	bool no_extra_attack = IS_SET(trigger_code, kNoExtraAttack);
	if ((AFF_FLAGGED(ch, EAffect::kCharmed) 
				|| ch->IsFlagged(EMobFlag::kTutelar))
				&& ch->has_master() 
				&& ch->in_room == ch->get_master()->in_room  // && !ch->master->IsNpc()
				&& AWAKE(ch) 
				&& !IsUnableToAct(ch) 
				&& ch->get_wait() <= 0
				&& ch->GetPosition() >= EPosition::kFight) {
		// сначала мытаемся спасти
		if (sight::CanSee(ch, ch->get_master()) && AFF_FLAGGED(ch, EAffect::kHelper)) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->GetEnemy() == ch->get_master()
					&& vict != ch && vict != ch->get_master()) {
					if (GetSkill(ch, ESkill::kRescue)) {
						go_rescue(ch, ch->get_master(), vict);
					} else if (GetSkill(ch, ESkill::kProtect)) {
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

	if (!ch->GetEnemy() || ch->in_room != ch->GetEnemy()->in_room) {
		return;
	}

	//**** удар основным оружием или рукой
	if (!AFF_FLAGGED(ch, EAffect::kStopRight) && !IS_SET(trigger_code, kNoRightHandAttack)) {
		ProcessExtrahits(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kMainHand);
	}

	//**** экстраатаки мобов. Первая - оффхэнд
	for (int i = 1; i <= ch->mob_specials.extra_attack; i++) {
		// если хп пробиты - уходим
		if (ch->IsFlagged(EMobFlag::kDecreaseAttack)) {
			if (ch->mob_specials.extra_attack * ch->get_hit() * 2 < i * ch->get_real_max_hit()) {
				return;
			}
		}
		if (i == 1) {
			if (AFF_FLAGGED(ch, EAffect::kStopLeft) || IS_SET(trigger_code, kNoLeftHandAttack))
				continue;
			ProcessExtrahits(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kOffHand);
		} else {
			ProcessExtrahits(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kMobAdd);
		}
	}
}

void process_player_attack(CharData *ch, int min_init) {

	if (ch->GetPosition() > EPosition::kStun
		&& ch->GetPosition() < EPosition::kFight
		&& ch->battle_affects.get(kEafStand)) {
		sprintf(buf, "%sВам лучше встать на ноги!%s\r\n", kColorWht, kColorNrm);
		SendMsgToChar(buf, ch);
		ch->battle_affects.unset(kEafStand);
	}

	// Срабатывание батл-триггеров амуниции
	Bitvector trigger_code = fight_otrigger(ch);

	//* каст заклинания
	if (ch->GetCastSpell() != ESpell::kUndefined && ch->get_wait() <= 0 && !IS_SET(trigger_code, kNoCastMagic)) {
		const auto queued_spell = ch->GetCastSpell();
		// Verbal-component gate (issue.spellcomponents): only verbal spells
		// fizzle under kSilence. The queued non-verbal spell goes through.
		if (MUD::Spell(queued_spell).IsVerbal() && AFF_FLAGGED(ch, EAffect::kSilence)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(queued_spell, ESpellMsg::kCantCastSilenced) + "\r\n", ch);
			ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		} else {
			CastSpell(ch, ch->GetCastChar(), ch->GetCastObj(), 0, queued_spell, ch->GetCastSubst());
			if (!(privilege::IsImmortal(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || ch->get_wait() > 0)) {
				SetBattleLag(ch, 1);
			}
			ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		}
		if (try_consume_extra_pass(ch, min_init)) {
			return;
		}
	}
	if (ch->battle_affects.get(kEafMultyparry))
		return;
	//* применение экстра скилл-атак (пнуть, оглушить и прочая)
	if (!IS_SET(trigger_code, kNoExtraAttack) 
			&& ch->GetExtraVictim()
			&& ch->in_room == ch->GetExtraVictim()->in_room
			&& ch->get_wait() <= 0 
			&& using_extra_attack(ch)) {
		ch->SetExtraAttack(kExtraAttackUnused, nullptr);
		if (try_consume_extra_pass(ch, min_init)) {
			return;
		}
	}
	if (!ch->GetEnemy() || ch->in_room != ch->GetEnemy()->in_room) {
		return;
	}
	//**** удар основным оружием или рукой
	if (ch->battle_affects.get(kEafFirst)) {
		if (!IS_SET(trigger_code, kNoRightHandAttack) && !AFF_FLAGGED(ch, EAffect::kStopRight)
			&& (privilege::IsImmortal(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || !ch->battle_affects.get(kEafUsedright))) {
			//Знаю, выглядит страшно, но зато в hit()
			//можно будет узнать применялось ли оглушить
			//или молотить, по баттл-аффекту узнать получиться
			//не во всех частях процедуры, а параметр type
			//хранит значение до её конца.
			ESkill tmpSkilltype;
			if (ch->battle_affects.get(kEafOverwhelm))
				tmpSkilltype = ESkill::kOverwhelm;
			else if (ch->battle_affects.get(kEafHammer))
				tmpSkilltype = ESkill::kHammer;
			else tmpSkilltype = ESkill::kUndefined;
			ProcessExtrahits(ch, ch->GetEnemy(), tmpSkilltype, fight::AttackType::kMainHand);
		}
// допатака двуручем
		if (!IS_SET(trigger_code, kNoExtraAttack) && GET_EQ(ch, EEquipPos::kBoths) 
			&& CanUseFeat(ch, EFeat::kTwohandsFocus)
			&& CanUseFeat(ch, EFeat::kSlashMaster)
			&& (static_cast<ESkill>(GET_EQ(ch, EEquipPos::kBoths)->get_spec_param()) == ESkill::kTwohands)) {
			if (GetSkill(ch, ESkill::kTwohands) > (number(1, 500)))
				hit(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kMainHand);
		}
		ch->battle_affects.unset(kEafFirst);
		ch->battle_affects.set(kEafSecond);
		if (try_consume_extra_pass(ch, min_init)) {
			return;
		}
	}
	//**** удар вторым оружием если оно есть и умение позволяет
	if (!IS_SET(trigger_code, kNoLeftHandAttack) && GET_EQ(ch, EEquipPos::kHold)
		&& GET_EQ(ch, EEquipPos::kHold)->get_type() == EObjType::kWeapon
		&& ch->battle_affects.get(kEafSecond)
		&& !AFF_FLAGGED(ch, EAffect::kStopLeft)
		&& (privilege::IsImmortal(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| GetSkill(ch, ESkill::kSideAttack) > number(1, 101))) {
		if (privilege::IsImmortal(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| !ch->battle_affects.get(kEafUsedleft)) {
			ProcessExtrahits(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kOffHand);
		}
		ch->battle_affects.unset(kEafSecond);
	}
		//**** удар второй рукой если она свободна и умение позволяет
	else if (!IS_SET(trigger_code, kNoLeftHandAttack) && !GET_EQ(ch, EEquipPos::kHold)
		&& !GET_EQ(ch, EEquipPos::kLight) && !GET_EQ(ch, EEquipPos::kShield) && !GET_EQ(ch, EEquipPos::kBoths)
		&& !AFF_FLAGGED(ch, EAffect::kStopLeft) && ch->battle_affects.get(kEafSecond)
		&& GetSkill(ch, ESkill::kLeftHit)) {
		if (privilege::IsImmortal(ch) || !ch->battle_affects.get(kEafUsedleft)) {
			ProcessExtrahits(ch, ch->GetEnemy(), ESkill::kUndefined, fight::AttackType::kOffHand);
		}
		ch->battle_affects.unset(kEafSecond);
	}
	// немного коряво, т.к. зависит от инициативы кастера
	// check if angel is in fight, and go_rescue if it is not
	TryToRescueWithTutelar(ch);
}

// * \return false - чар не участвует в расчете инициативы
bool stuff_before_round(CharData *ch) {
	// Initialize initiative
	ch->initiative = 0;
	ch->battle_counter = 0;
	ch->round_counter += 1;
	DpsSystem::check_round(ch);
	round_num_mtrigger(ch, ch->GetEnemy());

	ch->battle_affects.set(kEafStand);
	if (IsAffected(ch, EAffect::kSleep))
		ch->battle_affects.set(kEafSleep);
	if (ch->in_room == kNowhere)
		return false;

	if (AFF_FLAGGED(ch, EAffect::kHold)
			|| AFF_FLAGGED(ch, EAffect::kStopFight)
			|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)) {
		TryToRescueWithTutelar(ch);
		return false;
	}

	// Mobs stand up and players sit
	if (ch->GetPosition() < EPosition::kFight
		&& ch->GetPosition() > EPosition::kStun
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
		if (!ch->last_comm.empty()) {
			char *tmp = str_dup(ch->last_comm.c_str());
			command_interpreter(ch, tmp);
			ch->last_comm.clear();
			free(tmp);
		}
		// Set some flag-skills
		set_mob_skills_flags(ch);
	}

	return true;
}

// * Обработка текущих боев, дергается каждые 2 секунды.
void perform_violence() {
	int max_init = -100, min_init = 100;
	std::unordered_set<CharData *> msdp_report_chars;

	//* суммон хелперов
	sprintf(buf, "Check mob helpers");
	check_mob_helpers();
	//* действия до раунда и расчет инициативы
	// почистим удаленных между раундами боя
	std::erase_if(combat_list, [](auto flag) {return flag.deleted;});
	observability::OtelMetrics::RecordGauge("combat.active.count", static_cast<double>(combat_list.size()));
	for (auto &it : combat_list) {
		if (it.deleted)
			continue;
		if (it.ch->desc) {
			msdp_report_chars.insert(it.ch);
		} else if (it.ch->has_master()
			&& (it.ch->IsFlagged(EMobFlag::kCompanion))) {
			auto master = it.ch->get_master();
			if (master->desc
				&& !master->GetEnemy()
				&& master->in_room == it.ch->in_room) {
				msdp_report_chars.insert(master);
			}
		}

		if (!stuff_before_round(it.ch)) {
			continue;
		}

		roll_round_initiative(it.ch, min_init, max_init);
		it.ch->battle_affects.set(kEafFirst);
	}
	int size = 0;
	//* обработка раунда по очередности инициативы
	for (int initiative = max_init; initiative >= min_init; initiative--) {
		size = 0;
		for (auto &it : combat_list) {
			if (it.deleted)
				continue;
			size++;
			if (it.ch->initiative != initiative || it.ch->in_room == kNowhere) {
				continue;
			}
			// If mob cast 'hold' when initiative setted
			if (AFF_FLAGGED(it.ch, EAffect::kHold)
				|| AFF_FLAGGED(it.ch, EAffect::kMagicStopFight)
				|| AFF_FLAGGED(it.ch, EAffect::kStopFight)
				|| !AWAKE(it.ch)) {
				continue;
			}
			// If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
			if (!it.ch->GetEnemy() || it.ch->in_room != it.ch->GetEnemy()->in_room) {
				continue;
			}
			//везде в стоп-файтах ставится инициатива равная 0, убираем двойную атаку
			if (initiative == 0) {
				continue;
			}
			utils::CExecutionTimer violence_timer;
			//* выполнение атак в раунде
			if (it.ch->IsNpc()) {
				process_npc_attack(it.ch);
			} else {
				process_player_attack(it.ch, min_init);
			}
			if (violence_timer.delta().count() > 0.001) {
				log("Process player attack, name %s, time %f", it.ch->get_name().c_str(), violence_timer.delta().count());
			}
		}
	}
	// удалим помеченные (убитые)
	std::erase_if(combat_list, [](auto flag) {return flag.deleted;});
	//* обновление аффектов и лагов после раунда
	update_round_affs();

	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state == EConState::kPlaying && d->character) {
			for (const auto &ch : msdp_report_chars) {
				if (group::same_group(ch, d->character.get()) && ch->in_room == d->character->in_room) {
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
		&& (ch->IsFlagged(EMobFlag::kCompanion)
			|| mount::IsHorse(ch))) {
		ch = ch->get_master();
	}

	if (victim->IsNpc()
		&& victim->has_master()
		&& (victim->IsFlagged(EMobFlag::kCompanion)
			|| mount::IsHorse(victim))) {
		victim = victim->get_master();
	}

	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->has_master()) {
		if (cleader->IsNpc()
			&& !cleader->IsFlagged(EMobFlag::kCompanion)
			&& !mount::IsHorse(cleader)) {
			break;
		}
		cleader = cleader->get_master();
	}

	while (vleader->has_master()) {
		if (vleader->IsNpc()
			&& !vleader->IsFlagged(EMobFlag::kCompanion)
			&& !mount::IsHorse(vleader)) {
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
		follow::StopFollower(ch, follow::kSfEmpty);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, EAffect::kGroup)
		|| vleader == ch) {
		follow::StopFollower(victim, follow::kSfEmpty);
		return_value |= 2;
	}
	return return_value;
}

int set_hit(CharData *ch, CharData *victim) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return (false);
	}

	if (ch->GetEnemy() || AFF_FLAGGED(ch, EAffect::kHold)) {
		return (false);
	}
	victim = TryToFindProtector(victim, ch);

	bool message = false;
	// если жертва пишет на доску - вываливаем его оттуда и чистим все это дело
	if (victim->desc && (victim->desc->state == EConState::kWriteboard || victim->desc->state == EConState::kWriteMod || victim->desc->state == EConState::kWriteNote)) {
		victim->desc->message.reset();
		victim->desc->board.reset();
		if (victim->desc->writer->get_string()) {
			victim->desc->writer->clear();
		}
		victim->desc->state = EConState::kPlaying;
		if (!victim->IsNpc()) {
			victim->UnsetFlag(EPlrFlag::kWriting);
		}
		if (victim->desc->backstr) {
			free(victim->desc->backstr);
			victim->desc->backstr = nullptr;
		}
		victim->desc->writer.reset();
		message = true;
	} else if (victim->desc && (victim->desc->state == EConState::kClanedit)) {
		// аналогично, если жерва правит свою дружину в олц
		victim->desc->clan_olc.reset();
		victim->desc->state = EConState::kPlaying;
		message = true;
	} else if (victim->desc && (victim->desc->state == EConState::kSpendGlory)) {
		// или вливает-переливает славу
		victim->desc->glory.reset();
		victim->desc->state = EConState::kPlaying;
		message = true;
	} else if (victim->desc && (victim->desc->state == EConState::kGloryConst)) {
		// или вливает-переливает славу
		victim->desc->glory_const.reset();
		victim->desc->state = EConState::kPlaying;
		message = true;
	} else if (victim->desc && (victim->desc->state == EConState::kMapMenu)) {
		// или ковыряет опции карты
		victim->desc->map_options.reset();
		victim->desc->state = EConState::kPlaying;
		message = true;
	}

	if (message) {
		SendMsgToChar(victim, "На вас было совершено нападение, редактирование отменено!\r\n");
	}

	if (ch->IsFlagged(EMobFlag::kMemory) && ch->get_wait() > 0) {
		mob_ai::update_mob_memory(ch, victim);
		return false;
	}

	if (!IsCharmice(ch) || (AFF_FLAGGED(ch, EAffect::kCharmed) && !mob_ai::attack_best(ch, victim, true))) {
		ProcessExtrahits(ch, victim, ESkill::kUndefined,
						 AFF_FLAGGED(ch, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
	}
	SetWait(ch, 1, true);
	//ch->setSkillCooldown(kGlobalCooldown, 2);
	return (true);
};

ObjData *GetUsedWeapon(CharData *ch, fight::AttackType AttackType) {
	ObjData *UsedWeapon = nullptr;

	if (AttackType == fight::AttackType::kMainHand) {
		if (!(UsedWeapon = GET_EQ(ch, EEquipPos::kWield)))
			UsedWeapon = GET_EQ(ch, EEquipPos::kBoths);
	} else if (AttackType == fight::AttackType::kOffHand)
		UsedWeapon = GET_EQ(ch, EEquipPos::kHold);

	return UsedWeapon;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
