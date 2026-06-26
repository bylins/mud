// issue.chardata-cleaning: the character-following mechanic (logic moved off CharData).

#include "follow.h"
#include "gameplay/mechanics/groups.h"

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/char_equip_flags.h"
#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "utils/logger.h"
#include "utils/backtrace.h"
#include "engine/ui/cmd/do_drop.h"
#include "gameplay/fight/fight.h"
#include "engine/db/global_objects.h"
#include "gameplay/affects/affect_data.h"
#include "engine/entities/obj_data.h"

#include <fmt/format.h>
#include <sstream>

namespace follow {

bool MakesLoop(CharData *ch, CharData *master) {
	while (master) {
		if (master == ch) {
			return true;
		}
		master = master->get_master();
	}
	return false;
}

void PrintLeadersChain(const CharData *ch, std::ostream &ss) {
	if (!ch->has_master()) {
		ss << "<пуста>";
		return;
	}

	bool first = true;
	for (auto master = ch->get_master(); master; master = master->get_master()) {
		ss << (first ? "" : " -> ") << "[" << master->get_name() << "]";
		first = false;
	}
}

void ReportLoopError(CharData *ch, CharData *master) {
	std::stringstream ss;
	ss << "Обнаружена ошибка логики: попытка сделать цикл в цепочке последователей.\nТекущая цепочка лидеров: ";
	PrintLeadersChain(master, ss);
	ss << "\nПопытка сделать персонажа [" << master->get_name() << "] лидером персонажа [" << ch->get_name() << "]";
	mudlog(ss.str().c_str(), DEF, -1, ERRLOG, true);

	std::stringstream additional_info;
	additional_info << "Потенциальный лидер: name=[" << master->get_name() << "]"
					<< "; адрес структуры: " << master << "; текущий лидер: ";
	if (master->has_master()) {
		additional_info << "name=[" << master->get_master()->get_name() << "]"
						<< "; адрес структуры: " << master->get_master() << "";
	} else {
		additional_info << "<отсутствует>";
	}
	additional_info << "\nПоследователь: name=[" << ch->get_name() << "]"
					<< "; адрес структуры: " << ch << "; текущий лидер: ";
	if (ch->has_master()) {
		additional_info << "name=[" << ch->get_master()->get_name() << "]"
						<< "; адрес структуры: " << ch->get_master() << "";
	} else {
		additional_info << "<отсутствует>";
	}
	mudlog(additional_info.str().c_str(), DEF, -1, ERRLOG, true);

	ss << "\nТекущий стек будет распечатан в SYSLOG.";
	debug::backtrace(runtime_config.logs(SYSLOG).handle());
	mudlog(ss.str().c_str(), LGH, kLvlImmortal, SYSLOG, false);
}

bool IsLeader(CharData *ch) {
	if (ch->IsNpc()) {
		return false;
	}
	if (ch->get_master() != ch) {
		return false;
	}
	for (auto *f : ch->followers) {
		if (!f->IsNpc()) {
			return true;
		}
	}
	return false;
}

void AddFollowerSilently(CharData *leader, CharData *ch) {
	if (ch->has_master()) {
		log("SYSERR: add_follower_implementation(%s->%s) when master existing(%s)...",
			GET_NAME(ch), leader->get_name().c_str(), GET_NAME(ch->get_master()));
		return;
	}

	if (ch == leader) {
		return;
	}

	ch->set_master(leader);

	leader->followers.push_front(ch);
}

void AddFollower(CharData *leader, CharData *ch) {
	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoGroup))
		return;
	if (leader->in_room != ch->in_room) {
		log(fmt::format("попытка загрупить игроков в разных комнатах, лидер {} #{} фолловер {} #{}",
				leader->get_name(), GET_MOB_VNUM(leader), ch->get_name(), GET_MOB_VNUM(ch)));
		mudlog(fmt::format("попытка загрупить игроков в разных комнатах, лидер {} #{} фолловер {} #{}",
				leader->get_name(), GET_MOB_VNUM(leader), ch->get_name(), GET_MOB_VNUM(ch)));
		return;
	}
	AddFollowerSilently(leader, ch);

	if (!mount::IsHorse(ch)) {
		act("Вы начали следовать за $N4.", false, ch, 0, leader, kToChar);
		act("$n начал$g следовать за вами.", true, ch, 0, leader, kToVict);
		act("$n начал$g следовать за $N4.", true, ch, 0, leader, kToNotVict | kToArenaListen);
	}
}

// Check if making CH follow VICTIM will create an illegal //
// Follow "Loop/circle"                                    //
bool CircleFollow(CharData *ch, CharData *victim) {
	for (auto k = victim; k; k = k->get_master()) {
		if (k->get_master() == k) {
			k->set_master(nullptr);
			return false;
		}
		if (k == ch) {
			return true;
		}
	}
	return false;
}

// Called when a character that follows/is followed dies.
// Detaches ch from its master (if any) and dismisses all of ch's followers.
void DieFollower(CharData *ch) {
	if (ch->has_master()) {
		if (mount::GetHorse(ch->get_master()) == ch && mount::IsOnHorse(ch->get_master())) {
			mount::DropFromHorse(ch);
		} else {
			act("$n прекратил$g следовать за вами.", true, ch, 0, ch->get_master(), kToVict);
		}

		ch->get_master()->followers.remove(ch);
		if (ch->get_master()->followers.empty() && !ch->get_master()->has_master()) {
			group::RemoveGroupFlags(ch->get_master());
		}
		ch->set_master(nullptr);
		group::RemoveGroupFlags(ch);
	}

	if (mount::IsOnHorse(ch)) {
		AFF_FLAGS(ch).unset(EAffect::kHorse);
	}

	while (!ch->followers.empty()) {
		StopFollower(ch->followers.front(), kSfMasterdie);
	}
}

// Called when stop following persons, or stopping charm //
// This will NOT do if a character quits/dies!!          //
// При возврате 1 использовать ch нельзя, т.к. прошли через extract_char
// TODO: по всем вызовам не проходил, может еще где-то коряво вызывается, кроме передачи скакунов -- Krodo
// при персонаже на входе - пуржить не должно полюбому, если начнет, как минимум в change_leader будут глюки
bool StopFollower(CharData *ch, int mode) {
	int i;

	//log("[Stop ch] Start function(%s->%s)",ch ? GET_NAME(ch) : "none",
	//      ch->master ? GET_NAME(ch->master) : "none");

	if (!ch->has_master()) {
//		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: stop_follower(%s) without master", GET_NAME(ch));
		return (false);
	}

	// для смены лидера без лишнего спама
	if (!IS_SET(mode, kSfSilence)) {
		act("Вы прекратили следовать за $N4.", false, ch, 0, ch->get_master(), kToChar);
		act("$n прекратил$g следовать за $N4.", true, ch, 0, ch->get_master(), kToNotVict | kToArenaListen);
	}

	//log("[Stop ch] Stop horse");
	if (mount::GetHorse(ch->get_master()) == ch && mount::IsOnHorse(ch->get_master())) {
		mount::DropFromHorse(ch);
	} else {
		act("$n прекратил$g следовать за вами.", true, ch, 0, ch->get_master(), kToVict);
	}

	//log("[Stop ch] Remove from followers list");
	if (ch->get_master()->followers.empty()) {
		log("[Stop ch] SYSERR: Followers absent for %s (master %s).", GET_NAME(ch), GET_NAME(ch->get_master()));
	} else {
		ch->get_master()->followers.remove(ch);
		if (ch->get_master()->followers.empty()
			&& !ch->get_master()->has_master()) {
			//AFF_FLAGS(ch->get_master()).unset(EAffectFlag::AFF_GROUP);
			group::RemoveGroupFlags(ch->get_master());
		}
	}

	ch->set_master(nullptr);
	//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
	group::RemoveGroupFlags(ch);

	if (AFF_FLAGGED(ch, EAffect::kCharmed)
		|| AFF_FLAGGED(ch, EAffect::kHelper)
		|| IS_SET(mode, kSfCharmlost)) {
		if (IsAffectedWithFlag(ch, kAfCharmBond)) {
			RemoveCharmBond(ch);
		}
		ch->extract_timer = 5;
		AFF_FLAGS(ch).unset(EAffect::kCharmed);

		if (ch->GetEnemy()) {
			stop_fighting(ch, true);
		}

		if (ch->IsNpc()) {
			if (ch->IsFlagged(EMobFlag::kCorpse)) {
				act("Налетевший ветер развеял $n3, не оставив и следа.", true, ch, 0, 0, kToRoom | kToArenaListen);
				GET_LASTROOM(ch) = ch->in_room;
				PerformDropGold(ch, currencies::GetHand(*ch, currencies::kGold));
				currencies::SetHand(*ch, currencies::kGold, 0);
					ExtractCharFromWorld(ch, false);
				return (true);
			} else if (AFF_FLAGGED(ch, EAffect::kHelper)) {
				AFF_FLAGS(ch).unset(EAffect::kHelper);
			}
			// the NPC is no longer an ally once it stops being charmed/helping
			ch->UnsetFlag(EMobFlag::kCompanion);
		}
	}
	if (ch->IsNpc() && ch->get_type_charmice() != 0) {
		act("Магия подпитывающая $n3 развеялась, и $n0 вернул$u в норму.", true, ch, 0, 0, kToRoom | kToArenaListen);
		ch->restore_npc();
			// сначало бросаем лишнее
				while (ch->carrying) {
						ObjData *obj = ch->carrying;
					RemoveObjFromChar(obj);
					PlaceObjToRoom(obj, ch->in_room);
					}
			
			for (int i = 0; i < EEquipPos::kNumEquipPos; i++) { // убираем что одето
				if (GET_EQ(ch, i)) {
					if (!remove_otrigger(GET_EQ(ch, i), ch)) {
						continue;
					}
					PlaceObjToInventory(UnequipChar(ch, i, CharEquipFlag::show_msg), ch);
					//extract_obj(tmp);
					while (ch->carrying) {
						ObjData *obj = ch->carrying;
						ExtractObjFromWorld(obj);
					}
				}
			}
		
	}
	
	 
	if (ch->IsNpc() && (i = ch->get_rnum()) >= 0) {
		ch->CopyFlagsFrom(mob_proto + i);
	}

	return (false);
}

}  // namespace follow

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
