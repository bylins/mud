#include "stun.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/fight_start.h"
#include "handler.h"
#include "structs/global_objects.h"

using namespace FightSystem;

void do_stun(CharData *ch, char *argument, int, int) {
	if (ch->get_skill(ESkill::kStun) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kStun)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->ahorse()) {
		send_to_char("Вы привстали на стременах и поняли: 'лошадь украли!!!'\r\n", ch);
		return;
	}
	if ((GET_SKILL(ch, ESkill::kRiding) < 151) && (!IS_NPC(ch))) {
		send_to_char("Вы слишком неуверенно управляете лошадью, чтоб на ней пытаться ошеломить противника.\r\n", ch);
		return;
	}
	if (IsTimedBySkill(ch, ESkill::kStun) && (!IS_NPC(ch))) {
		send_to_char("Ваш грозный вид не испугает даже мышь, попробуйте ошеломить попозже.\r\n", ch);
		return;
	}
	if (!IS_NPC(ch) && !(GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_BOTHS))) {
		send_to_char("Вы должны держать оружие в основной руке.\r\n", ch);
		return;
	}
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кто это так сильно путается у вас под руками?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы БОЛЬНО стукнули себя по голове! 'А еще я туда ем', - подумали вы...\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	go_stun(ch, vict);
}

void go_stun(CharData *ch, CharData *vict) {
	TimedSkill timed;
	if (GET_SKILL(ch, ESkill::kStun) < 150) {
		ImproveSkill(ch, ESkill::kStun, true, vict);
		timed.skill = ESkill::kStun;
		timed.time = 7;
		timed_to_char(ch, &timed);
		act("У вас не получилось ошеломить $N3, надо больше тренироваться!",
			false, ch, nullptr, vict, kToChar);
		act("$N3 попытал$U ошеломить вас, но не получилось.",
			false, vict, nullptr, ch, kToChar);
		act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.",
			true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		set_hit(ch, vict);
		return;
	}

	timed.skill = ESkill::kStun;
	timed.time = std::clamp(7 - (GET_SKILL(ch, ESkill::kStun) - 150) / 10, 2, 7);
	timed_to_char(ch, &timed);
	//weap_weight = GET_EQ(ch, WEAR_BOTHS)?  GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS)) : GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD));
	//float num = MIN(95, (pow(GET_SKILL(ch, ESkill::kStun), 2) + pow(weap_weight, 2) + pow(GET_REAL_STR(ch), 2)) /
	//(pow(GET_REAL_DEX(vict), 2) + (GET_REAL_CON(vict) - GET_SAVE(vict, kStability)) * 30.0));

	int percent = number(1, MUD::Skills()[ESkill::kStun].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kStun, vict);
	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kStun, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kStun].name, percent, prob, success);
	if (!success) {
		act("У вас не получилось ошеломить $N3, надо больше тренироваться!",
			false, ch, nullptr, vict, kToChar);
		act("$N3 попытал$U ошеломить вас, но не получилось.",
			false, vict, nullptr, ch, kToChar);
		act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.",
			true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		set_hit(ch, vict);
	} else {
		if (GET_EQ(ch, WEAR_BOTHS)
		&& static_cast<ESkill>((ch->equipment[WEAR_BOTHS])->get_skill()) == ESkill::kBows) {
			act("Точным выстрелом вы ошеломили $N3!",
				false, ch, nullptr, vict, kToChar);
			act("Точный выстрел $N1 повалил вас с ног и лишил сознания.",
				false, vict, nullptr, ch, kToChar);
			act("$n точным выстрелом ошеломил$g $N3!",
				true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		} else {
			act("Мощным ударом вы ошеломили $N3!", false, ch,
				nullptr, vict, kToChar);
			act("Ошеломляющий удар $N1 сбил вас с ног и лишил сознания.",
				false, vict, nullptr, ch, kToChar);
			act("$n мощным ударом ошеломил$g $N3!", true, ch,
				nullptr, vict, kToNotVict | kToArenaListen);
		}
		GET_POS(vict) = EPosition::kIncap;
		WAIT_STATE(vict, (2 + GET_REAL_REMORT(ch) / 5) * kPulseViolence);
		ch->setSkillCooldown(ESkill::kStun, 3 * kPulseViolence);
		set_hit(ch, vict);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
