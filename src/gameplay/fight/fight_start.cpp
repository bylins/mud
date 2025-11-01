#include "fight_start.h"

#include "fight.h"
#include "pk.h"
#include "fight_hit.h"
#include "gameplay/skills/protect.h"
#include "gameplay/ai/mobact.h"
#include "common.h"
#include "gameplay/ai/mob_memory.h"


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
	} else if (victim->desc && (victim->desc->state == EConState::kTorcExch)) {
		// или меняет гривны (чистить особо и нечего)
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

	if (!IS_CHARMICE(ch) || (AFF_FLAGGED(ch, EAffect::kCharmed) && !mob_ai::attack_best(ch, victim, true))) {
		exthit(ch, victim, ESkill::kUndefined,
			AFF_FLAGGED(ch, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
	}
	SetWait(ch, 1, true);
	//ch->setSkillCooldown(kGlobalCooldown, 2);
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

	if (subcmd != kScmdMurder && !check_pkill(ch, vict, arg)) {
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
		//ch->setSkillCooldown(kGlobalCooldown, 2);
		return;
	}
	if ((ch->GetPosition() == EPosition::kStand) && (vict != ch->GetEnemy())) {
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
	if (IS_IMPL(vict) || vict->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar("А если он вас чайником долбанет? Думай, Господи, думай!\r\n", ch);
	} else {
		act("Вы обратили $N3 в прах! Взглядом! Одним!", false, ch, 0, vict, kToChar);
		act("$N обратил$g вас в прах своим ненавидящим взором!", false, vict, 0, ch, kToChar);
		act("$n просто испепелил$g взглядом $N3!", false, ch, 0, vict, kToNotVict | kToArenaListen);
		raw_kill(vict, ch);
	};
};
