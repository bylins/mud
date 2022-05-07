#include "resque.h"

#include "game_fight/common.h"
#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "structs/global_objects.h"

// ******************* RESCUE PROCEDURES
void fighting_rescue(CharData *ch, CharData *vict, CharData *tmp_ch) {
	if (vict->GetEnemy() == tmp_ch)
		stop_fighting(vict, false);
	if (ch->GetEnemy())
		ch->SetEnemy(tmp_ch);
	else
		SetFighting(ch, tmp_ch);
	if (tmp_ch->GetEnemy())
		tmp_ch->SetEnemy(ch);
	else
		SetFighting(tmp_ch, ch);
}

void go_rescue(CharData *ch, CharData *vict, CharData *tmp_ch) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		SendMsgToChar(ch, "Ну раскорячили вы ноги по сторонам, но спасти %s как?\r\n", GET_PAD(vict, 1));
		return;
	}

	int percent = number(1, MUD::Skills(ESkill::kRescue).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kRescue, tmp_ch);
	ImproveSkill(ch, ESkill::kRescue, prob >= percent, tmp_ch);

	if (GET_GOD_FLAG(ch, EGf::kGodsLike))
		prob = percent;
	if (GET_GOD_FLAG(ch, EGf::kGodscurse))
		prob = 0;

	bool success = percent <= prob;
	SendSkillBalanceMsg(ch, MUD::Skills(ESkill::kRescue).name, percent, prob, success);
	if (!success) {
		act("Вы безуспешно пытались спасти $N3.", false, ch, 0, vict, kToChar);
		ch->setSkillCooldown(ESkill::kGlobalCooldown, kPulseViolence);
		return;
	}

	if (!pk_agro_action(ch, tmp_ch))
		return;

	act("Хвала Богам, вы героически спасли $N3!", false, ch, 0, vict, kToChar);
	act("Вы были спасены $N4. Вы чувствуете себя Иудой!", false, vict, 0, ch, kToChar);
	act("$n героически спас$q $N3!", true, ch, 0, vict, kToNotVict | kToArenaListen);

	int hostilesCounter = 0;
	if (CanUseFeat(ch, EFeat::kLiveShield)) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->GetEnemy() == vict) {
				fighting_rescue(ch, vict, i);
				++hostilesCounter;
			}
		}
		hostilesCounter /= 2;
	} else {
		fighting_rescue(ch, vict, tmp_ch);
	}
	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
	SetSkillCooldown(ch, ESkill::kRescue, 1 + hostilesCounter);
	SetWait(vict, 2, false);
}

void do_rescue(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kRescue)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kRescue)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кто это так сильно путается под вашими ногами?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Ваше спасение вы можете доверить только Богам.\r\n", ch);
		return;
	}
	if (ch->GetEnemy() == vict) {
		SendMsgToChar("Вы пытаетесь спасти атакующего вас?\r\n" "Это не о вас ли писали Марк и Лука?\r\n", ch);
		return;
	}

	CharData *enemy = nullptr;
	for (const auto i : world[ch->in_room]->people) {
		if (i->GetEnemy() == vict) {
			enemy = i;
			break;
		}
	}

	if (!enemy) {
		act("Но никто не сражается с $N4!", false, ch, 0, vict, kToChar);
		return;
	}

	if (vict->IsNpc()
		&& (!enemy->IsNpc()
			|| (AFF_FLAGGED(enemy, EAffect::kCharmed)
				&& enemy->has_master()
				&& !enemy->get_master()->IsNpc()))
		&& (!ch->IsNpc()
			|| (AFF_FLAGGED(ch, EAffect::kCharmed)
				&& ch->has_master()
				&& !ch->get_master()->IsNpc()))) {
		SendMsgToChar("Вы пытаетесь спасти чужого противника.\r\n", ch);
		return;
	}

	// Двойники и прочие очарки не в группе с тем, кого собираются спасать:
	// Если тот, кто собирается спасать - "чармис" и у него существует хозяин
	if (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->has_master()) {
		// Если спасаем "чармиса", то проверять надо на нахождение в одной
		// группе хозянина спасющего и спасаемого.
		if (AFF_FLAGGED(vict, EAffect::kCharmed)
			&& vict->has_master()
			&& !same_group(vict->get_master(), ch->get_master())) {
			act("Спасали бы вы лучше другов своих.", false, ch, 0, vict, kToChar);
			act("Вы не можете спасти весь мир.", false, ch->get_master(), 0, vict, kToChar);
			return;
		}
	}

	if (!may_kill_here(ch, enemy, argument)) {
		return;
	}

	go_rescue(ch, vict, enemy);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
