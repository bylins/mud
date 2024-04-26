#include "stun.h"

#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight_hit.h"
#include "game_fight/fight_start.h"
#include "handler.h"
#include "structs/global_objects.h"

void do_stun(CharData *ch, char *argument, int, int) {
	if (ch->GetSkill(ESkill::kStun) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kStun)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->IsOnHorse()) {
		SendMsgToChar("Вы привстали на стременах и поняли: 'лошадь украли!!!'\r\n", ch);
		return;
	}
	if ((GET_SKILL(ch, ESkill::kRiding) < 151) && (!ch->IsNpc())) {
		SendMsgToChar("Вы слишком неуверенно управляете лошадью, чтоб на ней пытаться ошеломить противника.\r\n", ch);
		return;
	}
	if (IsTimedBySkill(ch, ESkill::kStun) && (!ch->IsNpc())) {
		SendMsgToChar("Ваш грозный вид не испугает даже мышь, попробуйте ошеломить попозже.\r\n", ch);
		return;
	}
	if (!ch->IsNpc() && !(GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kBoths))) {
		SendMsgToChar("Вы должны держать оружие в основной руке.\r\n", ch);
		return;
	}
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кто это так сильно путается у вас под руками?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Вы БОЛЬНО стукнули себя по голове! 'А еще я туда ем', - подумали вы...\r\n", ch);
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

	timed.skill = ESkill::kStun;
	timed.time = std::clamp(7 - (GET_SKILL(ch, ESkill::kStun) - (MUD::Skill(ESkill::kStun).cap / 4 * 3)) / 10, 2, 7);
	ImposeTimedSkill(ch, &timed);

	int percent = number(1, MUD::Skill(ESkill::kStun).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kStun, vict);
	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kStun, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kStun).name, percent, prob, success);
	if (!success) {
		act("У вас не получилось ошеломить $N3, надо больше тренироваться!",
			false, ch, nullptr, vict, kToChar);
		act("$N3 попытал$U ошеломить вас, но не получилось.",
			false, vict, nullptr, ch, kToChar);
		act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.",
			true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		set_hit(ch, vict);
	} else {
		if (GET_EQ(ch, EEquipPos::kBoths)
		&& static_cast<ESkill>((ch->equipment[EEquipPos::kBoths])->get_spec_param()) == ESkill::kBows) {
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
		SetWaitState(vict, (2 + GetRealRemort(ch) / 5) * kBattleRound * GET_SKILL(ch, ESkill::kStun) / MUD::Skill(ESkill::kStun).cap);
		ch->setSkillCooldown(ESkill::kStun, 3 * kBattleRound);
		hit(ch, vict, ESkill::kUndefined, AFF_FLAGGED(vict, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
