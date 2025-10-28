/**
\file do_drunkoff.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 28.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/db/global_objects.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/core/game_limits.h"

void do_drunkoff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	struct TimedSkill timed;
	int amount, weight, prob, percent, duration;
	int on_ground = 0;

	if (ch->IsNpc())
		return;

	if (ch->GetEnemy()) {
		SendMsgToChar("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kDrunked)) {
		SendMsgToChar("Вы хотите испортить себе весь кураж?\r\n" "Это не есть по русски!\r\n", ch);
		return;
	}

	if (!AFF_FLAGGED(ch, EAffect::kAbstinent) && GET_COND(ch, DRUNK) < kDrunked) {
		SendMsgToChar("Не стоит делать этого на трезвую голову.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kHangovering)) {
		SendMsgToChar("Вы не в состоянии так часто похмеляться.\r\n"
					  "Попросите Богов закодировать вас.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		for (obj = ch->carrying; obj; obj = obj->get_next_content()) {
			if (GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer) {
				break;
			}
		}
		if (!obj) {
			SendMsgToChar("У вас нет подходящего напитка для похмелья.\r\n", ch);
			return;
		}
	} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents))) {
			SendMsgToChar("Вы не смогли это найти!\r\n", ch);
			return;
		} else {
			on_ground = 1;
		}
	}

	if (GET_OBJ_TYPE(obj) != EObjType::kLiquidContainer
		&& GET_OBJ_TYPE(obj) != EObjType::kFountain) {
		SendMsgToChar("Этим вы вряд-ли сможете похмелиться.\r\n", ch);
		return;
	}

	if (on_ground && (GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer)) {
		SendMsgToChar("Прежде это стоит поднять.\r\n", ch);
		return;
	}

	if (!GET_OBJ_VAL(obj, 1)) {
		SendMsgToChar("Пусто.\r\n", ch);
		return;
	}

	switch (GET_OBJ_VAL(obj, 2)) {
		case LIQ_BEER:
		case LIQ_WINE:
		case LIQ_ALE:
		case LIQ_QUAS:
		case LIQ_BRANDY:
		case LIQ_VODKA:
		case LIQ_BRAGA: break;
		default: SendMsgToChar("Вспомните народную мудрость :\r\n" "\"Клин вышибают клином...\"\r\n", ch);
			return;
	}

	amount = std::max(1, GET_WEIGHT(ch) / 50);
	if (amount > GET_OBJ_VAL(obj, 1)) {
		SendMsgToChar("Вам точно не хватит этого количества для опохмела...\r\n", ch);
		return;
	}

	timed.skill = ESkill::kHangovering;
	timed.time = 12;
	ImposeTimedSkill(ch, &timed);

	percent = number(1, MUD::Skill(ESkill::kHangovering).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kHangovering, nullptr);
	TrainSkill(ch, ESkill::kHangovering, percent <= prob, nullptr);
	amount = MIN(amount, GET_OBJ_VAL(obj, 1));
	weight = MIN(amount, GET_OBJ_WEIGHT(obj));
	weight_change_object(obj, -weight);    // Subtract amount //
	obj->sub_val(1, amount);
	if (!GET_OBJ_VAL(obj, 1))    // The last bit //
	{
		obj->set_val(2, 0);
		obj->set_val(3, 0);
		name_from_drinkcon(obj);
	}

	if (percent > prob) {
		sprintf(buf, "Вы отхлебнули %s из $o1, но ваша голова стала еще тяжелее...", drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, false, ch, obj, 0, kToChar);
		act("$n попробовал$g похмелиться, но это не пошло $m на пользу.", false, ch, 0, 0, kToRoom);
		duration = std::max(1, amount / 3);
		Affect<EApply> af[3];
		af[0].type = ESpell::kAbstinent;
		af[0].duration = CalcDuration(ch, duration, 0, 0, 0, 0);
		af[0].modifier = 0;
		af[0].location = EApply::kDamroll;
		af[0].bitvector = to_underlying(EAffect::kAbstinent);
		af[0].battleflag = 0;
		af[1].type = ESpell::kAbstinent;
		af[1].duration = CalcDuration(ch, duration, 0, 0, 0, 0);
		af[1].modifier = 0;
		af[1].location = EApply::kHitroll;
		af[1].bitvector = to_underlying(EAffect::kAbstinent);
		af[1].battleflag = 0;
		af[2].type = ESpell::kAbstinent;
		af[2].duration = CalcDuration(ch, duration, 0, 0, 0, 0);
		af[2].modifier = 0;
		af[2].location = EApply::kAc;
		af[2].bitvector = to_underlying(EAffect::kAbstinent);
		af[2].battleflag = 0;
		switch (number(0, ch->GetSkill(ESkill::kHangovering) / 20)) {
			case 0:
			case 1: af[0].modifier = -2;
				break;
			case 2:
			case 3: af[0].modifier = -2;
				af[1].modifier = -2;
				break;
			default: af[0].modifier = -2;
				af[1].modifier = -2;
				af[2].modifier = 10;
				break;
		}
		for (prob = 0; prob < 3; prob++) {
			ImposeAffect(ch, af[prob], true, false, true, false);
		}
		gain_condition(ch, DRUNK, amount);
	} else {
		sprintf(buf, "Вы отхлебнули %s из $o1 и почувствовали приятную легкость во всем теле...",
				drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, false, ch, obj, 0, kToChar);
		act("$n похмелил$u и расцвел$g прям на глазах.", false, ch, nullptr, nullptr, kToRoom);
		RemoveAffectFromCharAndRecalculate(ch, ESpell::kAbstinent);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
