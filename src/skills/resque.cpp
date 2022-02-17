#include "resque.h"

#include "fightsystem/common.h"
#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "structs/global_objects.h"

using namespace FightSystem;

// ******************* RESCUE PROCEDURES
void fighting_rescue(CharData *ch, CharData *vict, CharData *tmp_ch) {
	if (vict->get_fighting() == tmp_ch)
		stop_fighting(vict, false);
	if (ch->get_fighting())
		ch->set_fighting(tmp_ch);
	else
		set_fighting(ch, tmp_ch);
	if (tmp_ch->get_fighting())
		tmp_ch->set_fighting(ch);
	else
		set_fighting(tmp_ch, ch);
}

void go_rescue(CharData *ch, CharData *vict, CharData *tmp_ch) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->ahorse()) {
		send_to_char(ch, "Ну раскорячили вы ноги по сторонам, но спасти %s как?\r\n", GET_PAD(vict, 1));
		return;
	}

	int percent = number(1, MUD::Skills()[ESkill::kRescue].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kRescue, tmp_ch);
	ImproveSkill(ch, ESkill::kRescue, prob >= percent, tmp_ch);

	if (GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		prob = 0;

	bool success = percent <= prob;
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kRescue].name, percent, prob, success);
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
	if (can_use_feat(ch, LIVE_SHIELD_FEAT)) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->get_fighting() == vict) {
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
	if (!ch->get_skill(ESkill::kRescue)) {
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kRescue)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кто это так сильно путается под вашими ногами?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Ваше спасение вы можете доверить только Богам.\r\n", ch);
		return;
	}
	if (ch->get_fighting() == vict) {
		send_to_char("Вы пытаетесь спасти атакующего вас?\r\n" "Это не о вас ли писали Марк и Лука?\r\n", ch);
		return;
	}

	CharData *enemy = nullptr;
	for (const auto i : world[ch->in_room]->people) {
		if (i->get_fighting() == vict) {
			enemy = i;
			break;
		}
	}

	if (!enemy) {
		act("Но никто не сражается с $N4!", false, ch, 0, vict, kToChar);
		return;
	}

	if (IS_NPC(vict)
		&& (!IS_NPC(enemy)
			|| (AFF_FLAGGED(enemy, EAffectFlag::AFF_CHARM)
				&& enemy->has_master()
				&& !IS_NPC(enemy->get_master())))
		&& (!IS_NPC(ch)
			|| (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				&& ch->has_master()
				&& !IS_NPC(ch->get_master())))) {
		send_to_char("Вы пытаетесь спасти чужого противника.\r\n", ch);
		return;
	}

	// Двойники и прочие очарки не в группе с тем, кого собираются спасать:
	// Если тот, кто собирается спасать - "чармис" и у него существует хозяин
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->has_master()) {
		// Если спасаем "чармиса", то проверять надо на нахождение в одной
		// группе хозянина спасющего и спасаемого.
		if (AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
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
