#include "protect.h"

#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "game_fight/common.h"
#include "game_fight/fight_hit.h"
#include "handler.h"
#include "structs/global_objects.h"

// ************** PROTECT PROCEDURES
void go_protect(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	ch->set_protecting(vict);
	act("Вы попытаетесь прикрыть $N3 от нападения.", false, ch, 0, vict, kToChar);
	SET_AF_BATTLE(ch, kEafProtect);
}

void do_protect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);
	if (!*arg) {
		if (ch->get_protecting()) {
			CLR_AF_BATTLE(ch, kEafProtect);
			ch->remove_protecting();
			SendMsgToChar("Вы перестали прикрывать своего товарища.\r\n", ch);
		} else {
			SendMsgToChar("Вы никого не прикрываете.\r\n", ch);
		}
		return;
	}

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kProtect)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kProtect)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = get_char_vis(ch, arg, EFind::kCharInRoom);
	if (!vict) {
		SendMsgToChar("И кто так сильно мил вашему сердцу?\r\n", ch);
		return;
	};

	if (vict == ch) {
		SendMsgToChar("Попробуйте парировать удары или защищаться щитом.\r\n", ch);
		return;
	}

	if (ch->GetEnemy() == vict) {
		SendMsgToChar("Вы явно пацифист, или мазохист.\r\n", ch);
		return;
	}

	CharData *tch = nullptr;
/*	for (const auto i : world[ch->in_room]->people) {
		if (i->GetEnemy() == vict) {
			tch = i;
			break;
		}
	}

	if (vict->IsNpc() && tch
		&& (!tch->IsNpc()
			|| (AFF_FLAGGED(tch, EAffect::kCharmed)
				&& tch->has_master()
				&& !tch->get_master()->IsNpc()))
		&& (!ch->IsNpc()
			|| (AFF_FLAGGED(ch, EAffect::kCharmed)
				&& ch->has_master()
				&& !ch->get_master()->IsNpc()))) {
		SendMsgToChar("Вы пытаетесь прикрыть чужого противника.\r\n", ch);
		return;
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (tch->GetEnemy() == vict && !may_kill_here(ch, tch, argument)) {
			return;
		}
	}
*/
	go_protect(ch, vict);
}

CharData *TryToFindProtector(CharData *victim, CharData *ch) {
	int percent = 0;
	int prob = 0;
	bool protect = false;

	if (ch->GetEnemy() == victim)
		return victim;

	for (const auto vict : world[IN_ROOM(victim)]->people) {
		if (vict->get_protecting() == victim
			&& !AFF_FLAGGED(vict, EAffect::kStopFight)
			&& !AFF_FLAGGED(vict, EAffect::kMagicStopFight)
			&& !AFF_FLAGGED(vict, EAffect::kBlind)
			&& !AFF_FLAGGED(vict, EAffect::kHold)
			&& GET_POS(vict) >= EPosition::kFight) {
			if (vict == ch) {
				act("Вы попытались напасть на того, кого прикрывали, и замерли в глубокой задумчивости.",
					false, vict, 0, victim, kToChar);
				act("$N пытается напасть на вас! Лучше бы вам отойти.", false, victim, 0, vict, kToChar);
				vict->remove_protecting();
				vict->battle_affects.unset(kEafProtect);
				SetWaitState(vict, kBattleRound);
				Affect<EApply> af;
				af.type = ESpell::kBattle;
				af.bitvector = to_underlying(EAffect::kStopFight);
				af.location = EApply::kNone;
				af.modifier = 0;
				af.duration = CalcDuration(vict, 1, 0, 0, 0, 0);
				af.battleflag = kAfBattledec | kAfPulsedec;
				ImposeAffect(vict, af, true, false, true, false);
				return victim;
			}

			if (protect) {
				SendMsgToChar(vict,
							  "Чьи-то широкие плечи помешали вам прикрыть %s.\r\n",
							  GET_PAD(vict->get_protecting(), 3));
				continue;
			}

			protect = true;
			percent = number(1, MUD::Skill(ESkill::kProtect).difficulty);
			prob = CalcCurrentSkill(vict, ESkill::kProtect, victim);
			prob = prob * 8 / 10;
			if (vict->HasCooldown(ESkill::kProtect)) {
				prob /= 2;
			};
			if (GET_GOD_FLAG(vict, EGf::kGodscurse)) {
				prob = 0;
			}
			bool success = prob >= percent;
			ImproveSkill(vict, ESkill::kProtect, success, ch);
			SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kProtect).name, percent, prob, success);

			if ((vict->GetEnemy() != ch) && (ch != victim)) {
				// агрим жертву после чего можно будет проверить возможно ли его здесь прикрыть(костыли конечно)
				if (!pk_agro_action(ch, victim))
					return victim;
				if (!may_kill_here(vict, ch, NoArgument))
					continue;
				// Вписываемся в противника прикрываемого ...
				stop_fighting(vict, false);
				SetFighting(vict, ch);
			}

			if (!success) {
				act("Вы не смогли прикрыть $N3.", false, vict, 0, victim, kToChar);
				act("$N не смог$Q прикрыть вас.", false, victim, 0, vict, kToChar);
				act("$n не смог$q прикрыть $N3.", true, vict, 0, victim, kToNotVict | kToArenaListen);
				//set_wait(vict, 3, true);
				SetSkillCooldownInFight(vict, ESkill::kGlobalCooldown, 3);
			} else {
				if (!pk_agro_action(vict, ch))
					return victim; // по аналогии с реском прикрывая кого-то можно пофлагаться
				act("Вы героически прикрыли $N3, приняв удар на себя.", false, vict, 0, victim, kToChar);
				act("$N героически прикрыл$G вас, приняв удар на себя.", false, victim, 0, vict, kToChar);
				act("$n героически прикрыл$g $N3, приняв удар на себя.",
					true,
					vict,
					0,
					victim,
					kToNotVict | kToArenaListen);
				//set_wait(vict, 1, true);
				SetSkillCooldownInFight(vict, ESkill::kGlobalCooldown, 1);
				SetSkillCooldownInFight(vict, ESkill::kProtect, 1);
				return vict;
			}
		}
	}
	return victim;
}
