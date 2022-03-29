#include "cmd/follow.h"

#include "fightsystem/fight.h"
#include "handler.h"

void perform_drop_gold(CharData *ch, int amount);

// Called when stop following persons, or stopping charm //
// This will NOT do if a character quits/dies!!          //
// При возврате 1 использовать ch нельзя, т.к. прошли через extract_char
// TODO: по всем вызовам не проходил, может еще где-то коряво вызывается, кроме передачи скакунов -- Krodo
// при персонаже на входе - пуржить не должно полюбому, если начнет, как минимум в change_leader будут глюки
bool stop_follower(CharData *ch, int mode) {
	struct Follower *j, *k;
	int i;

	//log("[Stop ch] Start function(%s->%s)",ch ? GET_NAME(ch) : "none",
	//      ch->master ? GET_NAME(ch->master) : "none");

	if (!ch->has_master()) {
		log("SYSERR: stop_follower(%s) without master", GET_NAME(ch));
		return (false);
	}

	// для смены лидера без лишнего спама
	if (!IS_SET(mode, kSfSilence)) {
		act("Вы прекратили следовать за $N4.", false, ch, 0, ch->get_master(), kToChar);
		act("$n прекратил$g следовать за $N4.", true, ch, 0, ch->get_master(), kToNotVict | kToArenaListen);
	}

	//log("[Stop ch] Stop horse");
	if (ch->get_master()->get_horse() == ch && ch->get_master()->ahorse()) {
		ch->drop_from_horse();
	} else {
		act("$n прекратил$g следовать за вами.", true, ch, 0, ch->get_master(), kToVict);
	}

	//log("[Stop ch] Remove from followers list");
	if (!ch->get_master()->followers) {
		log("[Stop ch] SYSERR: Followers absent for %s (master %s).", GET_NAME(ch), GET_NAME(ch->get_master()));
	} else if (ch->get_master()->followers->ch == ch)    // Head of ch-list?
	{
		k = ch->get_master()->followers;
		ch->get_master()->followers = k->next;
		if (!ch->get_master()->followers
			&& !ch->get_master()->has_master()) {
			//AFF_FLAGS(ch->get_master()).unset(EAffectFlag::AFF_GROUP);
			ch->get_master()->removeGroupFlags();
		}
		free(k);
	} else        // locate ch who is not head of list
	{
		for (k = ch->get_master()->followers; k->next && k->next->ch != ch; k = k->next);
		if (!k->next) {
			log("[Stop ch] SYSERR: Undefined %s in %s followers list.", GET_NAME(ch), GET_NAME(ch->get_master()));
		} else {
			j = k->next;
			k->next = j->next;
			free(j);
		}
	}

	ch->set_master(nullptr);
	//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
	ch->removeGroupFlags();

	if (AFF_FLAGGED(ch, EAffect::kCharmed)
		|| AFF_FLAGGED(ch, EAffect::kHelper)
		|| IS_SET(mode, kSfCharmlost)) {
		if (affected_by_spell(ch, kSpellCharm)) {
			affect_from_char(ch, kSpellCharm);
		}
		EXTRACT_TIMER(ch) = 5;
		AFF_FLAGS(ch).unset(EAffect::kCharmed);

		if (ch->get_fighting()) {
			stop_fighting(ch, true);
		}

		if (ch->is_npc()) {
			if (MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
				act("Налетевший ветер развеял $n3, не оставив и следа.", true, ch, 0, 0, kToRoom | kToArenaListen);
				GET_LASTROOM(ch) = GET_ROOM_VNUM(ch->in_room);
				perform_drop_gold(ch, ch->get_gold());
				ch->set_gold(0);
				extract_char(ch, false);
				return (true);
			} else if (AFF_FLAGGED(ch, EAffect::kHelper)) {
				AFF_FLAGS(ch).unset(EAffect::kHelper);
			}
		}
	}
	if (ch->is_npc() && MOB_FLAGGED(ch, EMobFlag::kSummoned)) { // фул рестор моба (Кудояр)
		act("Магия подпитующая $n3 развеялась, и $n0 вернул$u в норму.", true, ch, 0, 0, kToRoom | kToArenaListen);
		ch->restore_npc();
			// сначало бросаем лишнее
				while (ch->carrying) {
						ObjData *obj = ch->carrying;
							obj_from_char(obj);
							obj_to_room(obj, ch->in_room);
					}
			
			for (int i = 0; i < EEquipPos::kNumEquipPos; i++) { // убираем что одето
				if (GET_EQ(ch, i)) {
					if (!remove_otrigger(GET_EQ(ch, i), ch)) {
						continue;
					}
					obj_to_char(unequip_char(ch, i, CharEquipFlag::show_msg), ch);
					//extract_obj(tmp);
					while (ch->carrying) {
						ObjData *obj = ch->carrying;
							extract_obj(obj);
					}
				}
			}
		
	}
	
	 
	if (ch->is_npc()
		//&& !MOB_FLAGGED(ch, MOB_PLAYER_SUMMON)    //Не ресетим флаги, если моб призван игроком
		&& (i = GET_MOB_RNUM(ch)) >= 0) {
		MOB_FLAGS(ch) = MOB_FLAGS(mob_proto + i);
	}

	return (false);
}

// * Called when a character that follows/is followed dies
bool die_follower(CharData *ch) {
	struct Follower *j, *k = ch->followers;

	if (ch->has_master() && stop_follower(ch, kSfFollowerdie)) {
		//  чармиса спуржили в stop_follower
		return true;
	}

	if (ch->ahorse()) {
		AFF_FLAGS(ch).unset(EAffect::kHorse);
	}

	for (k = ch->followers; k; k = j) {
		j = k->next;
		stop_follower(k->ch, kSfMasterdie);
	}
	return false;
}

// Check if making CH follow VICTIM will create an illegal //
// Follow "Loop/circle"                                    //
bool circle_follow(CharData *ch, CharData *victim) {
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

void do_follow(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *leader;
	struct Follower *f;
	one_argument(argument, smallBuf);

	if (ch->is_npc() && AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_fighting())
		return;
	if (*smallBuf) {
		if (!str_cmp(smallBuf, "я") || !str_cmp(smallBuf, "self") || !str_cmp(smallBuf, "me")) {
			if (!ch->has_master()) {
				send_to_char("Но вы ведь ни за кем не следуете...\r\n", ch);
			} else {
				stop_follower(ch, kSfEmpty);
			}
			return;
		}
		if (!(leader = get_char_vis(ch, smallBuf, FIND_CHAR_ROOM))) {
			send_to_char(NOPERSON, ch);
			return;
		}
	} else {
		send_to_char("За кем вы хотите следовать?\r\n", ch);
		return;
	}

	if (ch->get_master() == leader) {
		act("Вы уже следуете за $N4.", false, ch, 0, leader, kToChar);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed)
		&& ch->has_master()) {
		act("Но вы можете следовать только за $N4!", false, ch, 0, ch->get_master(), kToChar);
	} else        // Not Charmed follow person
	{
		if (leader == ch) {
			if (!ch->has_master()) {
				send_to_char("Вы уже следуете за собой.\r\n", ch);
				return;
			}
			stop_follower(ch, kSfEmpty);
		} else {
			if (circle_follow(ch, leader)) {
				send_to_char("Так у вас целый хоровод получится.\r\n", ch);
				return;
			}

			if (ch->has_master()) {
				stop_follower(ch, kSfEmpty);
			}
			//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
			ch->removeGroupFlags();
			for (f = ch->followers; f; f = f->next) {
				//AFF_FLAGS(f->ch).unset(EAffectFlag::AFF_GROUP);
				f->ch->removeGroupFlags();
			}

			leader->add_follower(ch);
		}
	}
}
