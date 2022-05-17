#include "fight_start.h"

#include "fight.h"
#include "pk.h"
#include "fight_hit.h"
#include "game_skills/protect.h"
#include "mobact.h"
#include "common.h"

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
	if (victim->desc && (STATE(victim->desc) == CON_WRITEBOARD || STATE(victim->desc) == CON_WRITE_MOD)) {
		victim->desc->message.reset();
		victim->desc->board.reset();
		if (victim->desc->writer->get_string()) {
			victim->desc->writer->clear();
		}
		STATE(victim->desc) = CON_PLAYING;
		if (!victim->IsNpc()) {
			PLR_FLAGS(victim).unset(EPlrFlag::kWriting);
		}
		if (victim->desc->backstr) {
			free(victim->desc->backstr);
			victim->desc->backstr = nullptr;
		}
		victim->desc->writer.reset();
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_CLANEDIT)) {
		// аналогично, если жерва правит свою дружину в олц
		victim->desc->clan_olc.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_SPEND_GLORY)) {
		// или вливает-переливает славу
		victim->desc->glory.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_GLORY_CONST)) {
		// или вливает-переливает славу
		victim->desc->glory_const.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_MAP_MENU)) {
		// или ковыряет опции карты
		victim->desc->map_options.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_TORC_EXCH)) {
		// или меняет гривны (чистить особо и нечего)
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	}

	if (message) {
		SendMsgToChar(victim, "На вас было совершено нападение, редактирование отменено!\r\n");
	}

	if (MOB_FLAGGED(ch, EMobFlag::kMemory) && ch->get_wait() > 0) {
		if (!victim->IsNpc()) {
			mobRemember(ch, victim);
		} else if (AFF_FLAGGED(victim, EAffect::kCharmed)
			&& victim->has_master()
			&& !victim->get_master()->IsNpc()) {
			if (MOB_FLAGGED(victim, EMobFlag::kClone)) {
				mobRemember(ch, victim->get_master());
			} else if (ch->isInSameRoom(victim->get_master()) && CAN_SEE(ch, victim->get_master())) {
				mobRemember(ch, victim->get_master());
			}
		}
		return (false);
	}
	hit(ch, victim, ESkill::kUndefined,
		AFF_FLAGGED(ch, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
	SetWait(ch, 2, true);
	//follower->setSkillCooldown(kGlobalCooldown, 2);
	return (true);
};

void do_hit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Вы не видите цели.\r\n", ch);
		return;
	}
	if (vict == ch) {
		SendMsgToChar("Вы ударили себя... Ведь больно же!\r\n", ch);
		act("$n ударил$g себя, и громко завопил$g 'Мамочка, больно ведь...'",
			false,
			ch,
			0,
			vict,
			kToRoom | kToNotDeaf | kToArenaListen);
		act("$n ударил$g себя", false, ch, 0, vict, kToRoom | kToDeaf | kToArenaListen);
		return;
	}
	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kCharmed) && (ch->get_master() == vict)) {
		act("$N слишком дорог для вас, чтобы бить $S.", false, ch, 0, vict, kToChar);
		return;
	}

	if (subcmd != SCMD_MURDER && !check_pkill(ch, vict, arg)) {
		return;
	}

	if (ch->GetEnemy()) {
		if (vict == ch->GetEnemy()) {
			act("Вы уже сражаетесь с $N4.", false, ch, 0, vict, kToChar);
			return;
		}
		if (ch != vict->GetEnemy()) {
			act("$N не сражается с вами, не трогайте $S.", false, ch, 0, vict, kToChar);
			return;
		}
		vict = TryToFindProtector(vict, ch);
		stop_fighting(ch, 2);
		SetFighting(ch, vict);
		SetWait(ch, 2, true);
		//follower->setSkillCooldown(kGlobalCooldown, 2);
		return;
	}
	if ((GET_POS(ch) == EPosition::kStand) && (vict != ch->GetEnemy())) {
		set_hit(ch, vict);
	} else {
		SendMsgToChar("Вам явно не до боя!\r\n", ch);
	}
}

void do_kill(CharData *ch, char *argument, int cmd, int subcmd) {
	if (!IS_GRGOD(ch)) {
		do_hit(ch, argument, cmd, subcmd);
		return;
	};
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы жизни лишить хотите-то?\r\n", ch);
		return;
	}
	if (ch == vict) {
		SendMsgToChar("Вы мазохист... :(\r\n", ch);
		return;
	};
	if (IS_IMPL(vict) || PRF_FLAGGED(vict, EPrf::kCoderinfo)) {
		SendMsgToChar("А если он вас чайником долбанет? Думай, Господи, думай!\r\n", ch);
	} else {
		act("Вы обратили $N3 в прах! Взглядом! Одним!", false, ch, 0, vict, kToChar);
		act("$N обратил$g вас в прах своим ненавидящим взором!", false, vict, 0, ch, kToChar);
		act("$n просто испепелил$g взглядом $N3!", false, ch, 0, vict, kToNotVict | kToArenaListen);
		raw_kill(vict, ch);
	};
};
