#include "do_eat.h"
#include "gameplay/mechanics/condition.h"
#include "administration/privilege.h"

#include "engine/entities/char_data.h"
#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "do_hire.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/affects/affect_data.h"
#include "do_drink.h"
#include "gameplay/mechanics/minions.h"

extern void die(CharData *ch, CharData *killer);

void feed_charmice(CharData *ch, char *local_arg) {
	int max_charm_duration = 1;
	int chance_to_eat = 0;
	int reformed_hp_summ = 0;

	auto obj = get_obj_in_list_vis(ch, local_arg, world[ch->in_room]->contents);

	if (!obj || !IS_CORPSE(obj) || !ch->has_master()) {
		return;
	}

	for (auto *k : ch->get_master()->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)
			&& k->get_master() == ch->get_master()) {
			reformed_hp_summ += GetReformedCharmiceHp(ch->get_master(), k, ESpell::kAnimateDead);
		}
	}

	if (reformed_hp_summ >= CalcCharmPoint(ch->get_master(), ESpell::kAnimateDead)) {
		SendMsgToChar("Вы не можете управлять столькими последователями.\r\n", ch->get_master());
		ExtractCharFromWorld(ch, false);
		return;
	}

	int mob_level = 1;
	// труп не игрока
	if (GET_OBJ_VAL(obj, 2) != -1) {
		mob_level = GetRealLevel(mob_proto + GetMobRnum(GET_OBJ_VAL(obj, 2)));
	}
	const int max_heal_hp = 3 * mob_level;
	chance_to_eat = (100 - 2 * mob_level) / 2;
	if (IsAffected(ch->get_master(), EAffect::kFascination)) {
		chance_to_eat -= 30;
	}
	if (number(1, 100) < chance_to_eat) {
		act("$N подавил$U и начал$G сильно кашлять.", true, ch, nullptr, ch, kToRoom | kToArenaListen);
		ch->set_hit(ch->get_hit() - 3 * mob_level);
		update_pos(ch);
		// Подавился насмерть.
		if (ch->GetPosition() == EPosition::kDead) {
			die(ch, nullptr);
		}
		ExtractObjFromWorld(obj);
		return;
	}
	if (weather_info.moon_day < 14) {
		max_charm_duration =
			CalcDuration(ch, ch, ESkill::kUndefined, GetRealWis(ch->get_master()) - 6 + number(0, weather_info.moon_day % 14), 0, 0, 0);
	} else {
		max_charm_duration =
			CalcDuration(ch, ch, ESkill::kUndefined, GetRealWis(ch->get_master()) - 6 + number(0, 14 - weather_info.moon_day % 14), 0, 0, 0);
	}

	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = std::min(max_charm_duration, (int) (mob_level * max_charm_duration / 30));
	af.modifier = 0;
	af.location = EApply::kNone;
	af.affect_type = EAffect::kCharmed;
	af.battleflag = 0;

	ImposeAffect(ch, af);
	if (ch->IsNpc()) { ch->SetFlag(EMobFlag::kCompanion); }	// any NPC ally

	act("Громко чавкая, $N сожрал$G труп.", true, ch, obj, ch, kToRoom | kToArenaListen);
	act("Похоже, лакомство пришлось по вкусу.", true, ch, nullptr, ch->get_master(), kToVict);
	act("От омерзительного зрелища вас едва не вывернуло.",
		true, ch, nullptr, ch->get_master(), kToNotVict | kToArenaListen);

	if (ch->get_hit() < ch->get_max_hit()) {
		ch->set_hit(std::min(ch->get_hit() + std::min(max_heal_hp, ch->get_max_hit()), ch->get_max_hit()));
	}

	if (ch->get_hit() >= ch->get_max_hit()) {
		act("$n сыто рыгнул$g и благодарно посмотрел$g на вас.", true, ch, nullptr, ch->get_master(), kToVict);
		act("$n сыто рыгнул$g и благодарно посмотрел$g на $N3.",
			true, ch, nullptr, ch->get_master(), kToNotVict | kToArenaListen);
	}

	ExtractObjFromWorld(obj);
}

void do_eat(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *food;
	int amount;

	one_argument(argument, arg);

	if (subcmd == kScmdDevour) {
		// kUndead покрывает и умертвий (animate dead), и оживлённых (kResurrection):
		// раньше тут было kResurrected, но animate dead его больше не ставит (issue #3482)
		if (ch->IsFlagged(EMobFlag::kUndead)
			&& CanUseFeat(ch->get_master(), EFeat::kZombieDrover)) {
			feed_charmice(ch, arg);
			return;
		}
	}
	if (!ch->IsNpc()
		&& subcmd == kScmdDevour) {
		SendMsgToChar("Вы же не зверь какой, пожирать трупы!\r\n", ch);
		return;
	}

	if (ch->IsNpc())        // Cannot use GET_COND() on mobs.
		return;

	if (!*arg) {
		SendMsgToChar("Чем вы собрались закусить?\r\n", ch);
		return;
	}
	if (ch->GetEnemy()) {
		SendMsgToChar("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}

	if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}
	if (subcmd == kScmdTaste
		&& ((food->get_type() == EObjType::kLiquidContainer)
			|| (food->get_type() == EObjType::kFountain))) {
		DoDrink(ch, argument, 0, kScmdSip);
		return;
	}

	if (!privilege::IsGod(ch)) {
		if (food->get_type() == EObjType::kMagicComponent) //Сообщение на случай попытки проглотить ингры
		{
			SendMsgToChar("Не можешь приготовить - покупай готовое!\r\n", ch);
			return;
		}
		if (food->get_type() != EObjType::kFood
			&& food->get_type() != EObjType::kNote) {
			SendMsgToChar("Это несъедобно!\r\n", ch);
			return;
		}
	}
	if (GET_COND(ch, condition::kFull) == 0
		&& food->get_type() != EObjType::kNote)    // Stomach full
	{
		SendMsgToChar("Вы слишком сыты для этого!\r\n", ch);
		return;
	}
	if (subcmd == kScmdEat
		|| (subcmd == kScmdTaste
			&& food->get_type() == EObjType::kNote)) {
		act("Вы съели $o3.", false, ch, food, nullptr, kToChar);
		act("$n съел$g $o3.", true, ch, food, nullptr, kToRoom | kToArenaListen);
	} else {
		act("Вы откусили маленький кусочек от $o1.", false, ch, food, nullptr, kToChar);
		act("$n попробовал$g $o3 на вкус.",
			true, ch, food, nullptr, kToRoom | kToArenaListen);
	}

	amount = ((subcmd == kScmdEat && food->get_type() != EObjType::kNote)
			  ? GET_OBJ_VAL(food, 0)
			  : 1);

	gain_condition(ch, condition::kFull, -2 * amount);

	if (GET_COND(ch, condition::kFull) == 0) {
		SendMsgToChar("Вы наелись.\r\n", ch);
	}

	for (int i = 0; i < kMaxObjAffect; i++) {
		if (food->get_affected(i).modifier) {
			Affect<EApply> af;
			af.location = food->get_affected(i).location;
			af.modifier = food->get_affected(i).modifier;
			af.affect_type = EAffect::kWellFed;
			af.type = ESpell::kFullFeed;
//			af.battleflag = 0;
			af.duration = CalcDuration(ch, ch, ESkill::kUndefined, 10 * 2, 0, 0, 0);
			ImposeAffect(ch, af);
		}

	}

	if ((GET_OBJ_VAL(food, 3) == 1) && !privilege::IsImmortal(ch))    // The shit was poisoned !
	{
		SendMsgToChar("Однако, какой странный вкус!\r\n", ch);
		act("$n закашлял$u и начал$g отплевываться.",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

		Affect<EApply> af;
		af.type = ESpell::kPoison;
		af.duration = CalcDuration(ch, ch, ESkill::kUndefined, amount == 1 ? amount : amount * 2, 0, 0, 0);
		af.modifier = 0;
		af.location = EApply::kStr;
		af.affect_type = EAffect::kPoisoned;
		af.battleflag = kAfSameTime;
		ImposeAffect(ch, af, false, false, false, false);
		af.type = ESpell::kPoison;
		af.duration = CalcDuration(ch, ch, ESkill::kUndefined, amount == 1 ? amount : amount * 2, 0, 0, 0);
		af.modifier = amount * 3;
		af.location = EApply::kPoison;
		af.affect_type = EAffect::kPoisoned;
		af.battleflag = kAfSameTime;
		ImposeAffect(ch, af, false, false, false, false);
		// отравленная еда -- сам виноват: у аффекта нет автора (caster_id остаётся 0)
	}
	if (subcmd == kScmdEat
		|| (subcmd == kScmdTaste
			&& food->get_type() == EObjType::kNote)) {
		ExtractObjFromWorld(food);
	} else {
		food->set_val(0, food->get_val(0) - 1);
		if (!food->get_val(0)) {
			SendMsgToChar("Вы доели все!\r\n", ch);
			ExtractObjFromWorld(food);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
