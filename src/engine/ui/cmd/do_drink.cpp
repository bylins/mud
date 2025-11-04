/**
\file DoDrink.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 28.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "do_drink.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/poison.h"

ObjData *GetDrinkingJar(CharData *ch, char *jar_name);
int CanDrink(CharData *ch, ObjData *jar);
int CalcDrinkAmount(CharData *ch, ObjData *jar, int subcmd);
int IsAbleToDrink(CharData *ch, ObjData *jar, int amount);
void TryDrinkAlcohol(CharData *ch, ObjData *jar, int amount);

void DoDrink(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *jar;
	int amount;

	if (ch->IsNpc()) {
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar("Пить из чего?\r\n", ch);
		return;
	}

	if (!(jar = GetDrinkingJar(ch, arg))) {
		return;
	}

	if (!CanDrink(ch, jar)) {
		return;
	}

	if (TryCastSpellsFromLiquid(ch, jar)) {
		return;
	}

	amount = CalcDrinkAmount(ch, jar, subcmd);
	if (!IsAbleToDrink(ch, jar, amount)) {
		return;
	}

	if (subcmd == kScmdDrink) {
		sprintf(buf, "$n выпил$g %s из $o1.", drinks[GET_OBJ_VAL(jar, 2)]);
		act(buf, true, ch, jar, nullptr, kToRoom);
		sprintf(buf, "Вы выпили %s из %s.\r\n", drinks[GET_OBJ_VAL(jar, 2)], OBJN(jar, ch, ECase::kGen));
		SendMsgToChar(buf, ch);
	} else {
		act("$n отхлебнул$g из $o1.", true, ch, jar, nullptr, kToRoom);
		sprintf(buf, "Вы узнали вкус %s.\r\n", drinks[GET_OBJ_VAL(jar, 2)]);
		SendMsgToChar(buf, ch);
	}

	if (jar->get_type() != EObjType::kFountain) {
		weight_change_object(jar, -std::min(amount, jar->get_weight()));
	}

	if (
		(drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] > 0) && //Если жидкость с градусом
			(
				// Чар все еще не смертельно пьян и не начал трезветь
				(GET_DRUNK_STATE(ch) < kMortallyDrunked && GET_DRUNK_STATE(ch) == GET_COND(ch, DRUNK)) ||
					// Или Чар еще не пьян
					(GET_COND(ch, DRUNK) < kDrunked)
			)
		) {
		// Не понимаю зачем делить на 4, но оставим для пьянки 2
		gain_condition(ch, DRUNK, (int) ((int) drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] * amount) / 2);
		GET_DRUNK_STATE(ch) = std::max(GET_DRUNK_STATE(ch), GET_COND(ch, DRUNK));
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][FULL] != 0) {
		gain_condition(ch, FULL, drink_aff[GET_OBJ_VAL(jar, 2)][FULL] * amount);
		if (drink_aff[GET_OBJ_VAL(jar, 2)][FULL] < 0 && GET_COND(ch, FULL) <= kNormCondition)
			SendMsgToChar("Вы чувствуете приятную тяжесть в желудке.\r\n", ch);
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] != 0) {
		gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] * amount);
		if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] < 0 && GET_COND(ch, THIRST) <= kNormCondition)
			SendMsgToChar("Вы не чувствуете жажды.\r\n", ch);
	}

	TryDrinkAlcohol(ch, jar, amount);
	TryDrinkPoison(ch, jar, amount);

	// empty the container, and no longer poison. 999 - whole fountain //
	if (jar->get_type() != EObjType::kFountain || GET_OBJ_VAL(jar, 1) != 999) {
		jar->sub_val(1, amount);
	}
	if (!GET_OBJ_VAL(jar, 1))    // The last bit //
	{
		jar->set_val(2, 0);
		jar->set_val(3, 0);
		name_from_drinkcon(jar);
	}
}

ObjData *GetDrinkingJar(CharData *ch, char *jar_name) {
	ObjData *jar = nullptr;
	if (!(jar = get_obj_in_list_vis(ch, jar_name, ch->carrying))) {
		if (!(jar = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents))) {
			SendMsgToChar("Вы не смогли это найти!\r\n", ch);
			return jar;
		}

		if (jar->get_type() == EObjType::kLiquidContainer) {
			SendMsgToChar("Прежде это стоит поднять.\r\n", ch);
			return nullptr;
		}
	}
	return jar;
}

int CanDrink(CharData *ch, ObjData *jar) {
	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Не стоит отвлекаться в бою!\r\n", ch);
		return 0;
	}

	if (jar->get_type() == EObjType::kMagicIngredient) {
		SendMsgToChar("Не можешь приготовить - покупай готовое!\r\n", ch);
		return 0;
	}

	if (jar->get_type() != EObjType::kLiquidContainer
		&& jar->get_type() != EObjType::kFountain) {
		SendMsgToChar("Не стоит. Козлят и так много!\r\n", ch);
		return 0;
	}

	if (AFF_FLAGGED(ch, EAffect::kStrangled) && AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
		return 0;
	}

	if (GET_OBJ_VAL(jar, 1) == 0) {
		SendMsgToChar("Пусто.\r\n", ch);
		return 0;
	}

	return 1;
}

int CalcDrinkAmount(CharData *ch, ObjData *jar, int subcmd) {
	int amount = 1;
	float V = 1.0;

	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] > 0) {
		if (GET_COND(ch, DRUNK) >= kMortallyDrunked) {
			amount = -1;
		} else {
			//Тут магия из-за /4
			amount = (2 * kMortallyDrunked - GET_COND(ch, DRUNK)) / drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK];
			amount = std::max(1, amount); // ну еще чуть-чуть
		}
	} else {
		amount = number(3, 10);
	}

	// Если жидкость утоляет жаду
	if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] < 0) {
		V = (float) -GET_COND(ch, THIRST) / drink_aff[GET_OBJ_VAL(jar, 2)][THIRST];
	} else if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] > 0) {
		// Если жидоксть вызывает сушняк
		V = (float) (kMaxCondition - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(jar, 2)][THIRST];
	} else {
		V = 999.0;
	}
	amount = MIN(amount, round(V + 0.49999));

	if (subcmd != kScmdDrink) // пьем прямо, не глотаем, не пробуем
	{
		amount = std::min(amount, 1);
	}

	//обрезаем до объема контейнера
	amount = std::min(amount, GET_OBJ_VAL(jar, 1));
	return amount;
}

int IsAbleToDrink(CharData *ch, ObjData *jar, int amount) {
	//Сушняк, если у чара жажда - а напиток сушит (ВОДКА и САМОГОН!!!), заодно спасаем пьяниц от штрафов
	if ( drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] > 0 && GET_COND_M(ch, THIRST) > 5) {
		SendMsgToChar("У вас пересохло в горле, нужно что-то попить.\r\n", ch);
		return 0;
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] > 0) {
		if (AFF_FLAGGED(ch, EAffect::kAbstinent)) {
			if (ch->GetSkill(ESkill::kHangovering) > 0) {//если опохмел есть
				SendMsgToChar(
					"Вас передернуло от одной мысли о выпивке.\r\nПохоже, вам стоит опохмелиться.\r\n",
					ch);
			} else {
				SendMsgToChar("Вы пытались... но не смогли заставить себя выпить...\r\n", ch);
			}
			return 0;
		}
		if (GET_COND(ch, DRUNK) >= kDrunked) {
			if (GET_DRUNK_STATE(ch) == kMortallyDrunked || GET_COND(ch, DRUNK) < GET_DRUNK_STATE(ch)) {
				SendMsgToChar("На сегодня вам достаточно, крошки уже плавают...\r\n", ch);
				return 0;
			}
		}
	}

	if (amount <= 0 && !IS_GOD(ch)) {
		SendMsgToChar("В вас больше не лезет.\r\n", ch);
		return 0;
	}

	if (ch->GetEnemy()) {
		SendMsgToChar("Не стоит отвлекаться в бою.\r\n", ch);
		return 0;
	}
	return 1;
}

void TryDrinkAlcohol(CharData *ch, ObjData *jar, int amount) {
	int duration;

	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] <= 0)
		return;

	if (amount == 0)
		return;

	// \TODO Все, что ниже, нужно перенестив в отдельную механику опьянения (а можеет и всю функцию)
	if (GET_COND(ch, DRUNK) >= kDrunked) {
		if (GET_COND(ch, DRUNK) >= kMortallyDrunked) {
			SendMsgToChar("Напилися вы пьяны, не дойти вам до дому....\r\n", ch);
		} else {
			SendMsgToChar("Приятное тепло разлилось по вашему телу.\r\n", ch);
		}

		duration = 2 + std::max(0, GET_COND(ch, DRUNK) - kDrunked);

		if (CanUseFeat(ch, EFeat::kDrunkard))
			duration += duration / 2;

		if (!AFF_FLAGGED(ch, EAffect::kAbstinent)
			&& GET_DRUNK_STATE(ch) < kMaxCondition
			&& GET_DRUNK_STATE(ch) == GET_COND(ch, DRUNK)) {
			SendMsgToChar("Винные пары ударили вам в голову.\r\n", ch);
			// **** Decrease AC ***** //
			Affect<EApply> af;
			af.type = ESpell::kDrunked;
			af.duration = CalcDuration(ch, duration, 0, 0, 0, 0);
			af.modifier = -20;
			af.location = EApply::kAc;
			af.bitvector = to_underlying(EAffect::kDrunked);
			af.battleflag = 0;
			ImposeAffect(ch, af, false, false, false, false);
			// **** Decrease HR ***** //
			af.type = ESpell::kDrunked;
			af.duration = CalcDuration(ch, duration, 0, 0, 0, 0);
			af.modifier = -2;
			af.location = EApply::kHitroll;
			af.bitvector = to_underlying(EAffect::kDrunked);
			af.battleflag = 0;
			ImposeAffect(ch, af, false, false, false, false);
			// **** Increase DR ***** //
			af.type = ESpell::kDrunked;
			af.duration = CalcDuration(ch, duration, 0, 0, 0, 0);
			af.modifier = GetRealLevel(ch) / 5 + GetRealRemort(ch) / 5;
			af.location = EApply::kPhysicDamagePercent;
			af.bitvector = to_underlying(EAffect::kDrunked);
			af.battleflag = 0;
			ImposeAffect(ch, af, false, false, false, false);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
