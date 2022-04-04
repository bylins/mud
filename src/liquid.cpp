/*************************************************************************
*   File: liquid.cpp                                   Part of Bylins    *
*   Все по жидкостям                                                     *
*                                                                        *
*  $Author$                                                      *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "liquid.h"

#include "entities/obj_data.h"
#include "entities/char_data.h"
#include "utils/utils_char_obj.inl"
#include "handler.h"
#include "game_magic/magic.h"
#include "color.h"
#include "structs/global_objects.h"

const int kDrunked = 10;
const int kMortallyDrunked = 18;
const int kMaxCondition = 48;
const int kNormCondition = 22;

extern void weight_change_object(ObjData *obj, int weight);

const char *diag_liquid_timer(const ObjData *obj);

const char *drinks[] = {"воды",
						"пива",
						"вина",
						"медовухи",
						"кваса",
						"самогона",
						"морсу",
						"водки",
						"браги",
						"меду",
						"молока",
						"чаю",
						"кофею",
						"КРОВИ",
						"соленой воды",
						"родниковой воды",
						"колдовского зелья",
						"красного колдовского зелья",
						"синего колдовского зелья",
						"белого колдовского зелья",
						"золотистого колдовского зелья", //20
						"черного колдовского зелья",
						"серого колдовского зелья",
						"фиолетового колдовского зелья",
						"розового колдовского зелья",
						"настойки аконита",
						"настойки скополии",
						"настойки белены",
						"настойки дурмана",
						"\n"
};

// one-word alias for each drink
const char *drinknames[] = {"водой",
							"пивом",
							"вином",
							"медовухой",
							"квасом",
							"самогоном",
							"морсом",
							"водкой",
							"брагой",
							"медом",
							"молоком",
							"чаем",
							"кофе",
							"КРОВЬЮ",
							"соленой водой",
							"родниковой водой",
							"колдовским зельем",
							"красным колдовским зельем",
							"синим колдовским зельем",
							"белым колдовским зельем",
							"золотистым колдовским зельем", //20
							"черным колдовским зельем",
							"серым колдовским зельем",
							"фиолетовым колдовским зельем",
							"розовым колдовским зельем",
							"настойкой аконита",
							"настойкой скополии",
							"настойкой белены",
							"настойкой дурмана",
							"\n"
};

// color of the various drinks
const char *color_liquid[] = {"прозрачной",
							  "коричневой",
							  "бордовой",
							  "золотистой",
							  "бурой",
							  "прозрачной",
							  "рубиновой",
							  "зеленой",
							  "чистой",
							  "светло зеленой",
							  "белой",
							  "коричневой",
							  "темно-коричневой",
							  "красной",
							  "прозрачной",
							  "кристально чистой",
							  "искрящейся",
							  "бордовой",
							  "лазурной",
							  "серебристой",
							  "золотистой", //20
							  "черной вязкой",
							  "бесцветной",
							  "фиолетовой",
							  "розовой",
							  "ядовитой",
							  "ядовитой",
							  "ядовитой",
							  "ядовитой",
							  "\n"
};

// effect of drinks on DRUNK, FULL, THIRST -- see values.doc
const int drink_aff[][3] = {
	{0, 1, -10},            // вода
	{2, -2, -3},            // пиво
	{5, -2, -2},            // вино
	{3, -2, -3},            // медовуха
	{1, -2, -5},            // квас
	{8, 0,
	 4},                // самогон (как и водка сушит, никак не влияет на голод можно пить хоть залейся, только запивать успевай)
	{0, -1, -8},            // морс
	{10, 0,
	 3},                // водка (водка не питье! после нее обезвоживание, пить можно сколько угодно, но сушняк будет)
	{3, -3, -3},            // брага
	{0, -2, -8},            // мед (мед тоже ж калорийный)
	{0, -3, -6},            // молоко
	{0, -1, -6},            // чай
	{0, -1, -6},            // кофе
	{0, -2, 1},            // кровь
	{0, -1, 2},            // соленая вода
	{0, 0, -13},            // родниковая вода
	{0, -1, 1},            // магическое зелье
	{0, -1, 1},            // красное магическое зелье
	{0, -1, 1},            // синее магическое зелье
	{0, -1, 1},            // белое магическое зелье
	{0, -1, 1},            // золотистое магическое зелье
	{0, -1, 1},            // черное магическое зелье
	{0, -1, 1},            // серое магическое зелье
	{0, -1, 1},            // фиолетовое магическое зелье
	{0, -1, 1},            // розовое магическое зелье
	{0, 0, 0},            // настойка аконита
	{0, 0, 0},            // настойка скополии
	{0, 0, 0},            // настойка белены
	{0, 0, 0}            // настойка дурмана
};

/**
* Зелья, перелитые в контейнеры, можно пить во время боя.
* На случай, когда придется добавлять еще пошенов, которые
* уже будут идти не подряд по номерам.
*/
bool is_potion(const ObjData *obj) {
	switch (GET_OBJ_VAL(obj, 2)) {
		case LIQ_POTION:
		case LIQ_POTION_RED:
		case LIQ_POTION_BLUE:
		case LIQ_POTION_WHITE:
		case LIQ_POTION_GOLD:
		case LIQ_POTION_BLACK:
		case LIQ_POTION_GREY:
		case LIQ_POTION_FUCHSIA:
		case LIQ_POTION_PINK: return true;
			break;
	}
	return false;
}

namespace drinkcon {

ObjVal::EValueKey init_spell_num(int num) {
	return num == 1
		   ? ObjVal::EValueKey::POTION_SPELL1_NUM
		   : (num == 2
			  ? ObjVal::EValueKey::POTION_SPELL2_NUM
			  : ObjVal::EValueKey::POTION_SPELL3_NUM);
}

ObjVal::EValueKey init_spell_lvl(int num) {
	return num == 1
		   ? ObjVal::EValueKey::POTION_SPELL1_LVL
		   : (num == 2
			  ? ObjVal::EValueKey::POTION_SPELL2_LVL
			  : ObjVal::EValueKey::POTION_SPELL3_LVL);
}

void reset_potion_values(CObjectPrototype *obj) {
	obj->set_value(ObjVal::EValueKey::POTION_SPELL1_NUM, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL1_LVL, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL2_NUM, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL2_LVL, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL3_NUM, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL3_LVL, -1);
	obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, -1);
}

/// уровень в зельях (GET_OBJ_VAL(from_obj, 0)) пока один на все заклы
bool copy_value(const CObjectPrototype *from_obj, CObjectPrototype *to_obj, int num) {
	if (GET_OBJ_VAL(from_obj, num) > 0) {
		to_obj->set_value(init_spell_num(num), GET_OBJ_VAL(from_obj, num));
		to_obj->set_value(init_spell_lvl(num), GET_OBJ_VAL(from_obj, 0));
		return true;
	}
	return false;
}

/// заполнение values емкости (to_obj) из зелья (from_obj)
void copy_potion_values(const CObjectPrototype *from_obj, CObjectPrototype *to_obj) {
	reset_potion_values(to_obj);
	bool copied = false;

	for (int i = 1; i <= 3; ++i) {
		if (copy_value(from_obj, to_obj, i)) {
			copied = true;
		}
	}

	if (copied) {
		to_obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, GET_OBJ_VNUM(from_obj));
	}
}

} // namespace drinkcon

using namespace drinkcon;

int cast_potion_spell(CharData *ch, ObjData *obj, int num) {
	const int spell = obj->get_value(init_spell_num(num));
	const int level = obj->get_value(init_spell_lvl(num));

	if (spell >= 0 && level >= 0) {
		return CallMagic(ch, ch, nullptr, world[ch->in_room], spell, level);
	}
	return 1;
}

// Выпил яду
void do_drink_poison(CharData *ch, ObjData *jar, int amount) {
	if ((GET_OBJ_VAL(jar, 3) == 1) && !IS_GOD(ch)) {
		send_to_char("Что-то вкус какой-то странный!\r\n", ch);
		act("$n поперхнул$u и закашлял$g.", true, ch, 0, 0, kToRoom);
		Affect<EApply> af;
		af.type = kSpellPoison;
		//если объем 0 -
		af.duration = CalcDuration(ch, amount == 0 ? 3 : amount == 1 ? amount : amount * 3, 0, 0, 0, 0);
		af.modifier = -2;
		af.location = EApply::kStr;
		af.bitvector = to_underlying(EAffect::kPoisoned);
		af.battleflag = kAfSameTime;
		affect_join(ch, af, false, false, false, false);
		af.type = kSpellPoison;
		af.modifier = amount == 0 ? GetRealLevel(ch) * 3 : amount * 3;
		af.location = EApply::kPoison;
		af.bitvector = to_underlying(EAffect::kPoisoned);
		af.battleflag = kAfSameTime;
		affect_join(ch, af, false, false, false, false);
		ch->poisoner = 0;
	}
}

int cast_potion(CharData *ch, ObjData *jar) {
	if (is_potion(jar) && jar->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) >= 0) {
		act("$n выпил$g зелья из $o1.", true, ch, jar, 0, kToRoom);
		send_to_char(ch, "Вы выпили зелья из %s.\r\n", OBJN(jar, ch, 1));

		//не очень понятно, но так было
		for (int i = 1; i <= 3; ++i)
			if (cast_potion_spell(ch, jar, i) <= 0)
				break;

		WAIT_STATE(ch, kPulseViolence);
		jar->dec_weight();
		// все выпито
		jar->dec_val(1);

		if (GET_OBJ_VAL(jar, 1) <= 0
			&& GET_OBJ_TYPE(jar) != EObjType::kFountain) {
			name_from_drinkcon(jar);
			jar->set_skill(0);
			reset_potion_values(jar);
		}
		do_drink_poison(ch, jar, 0);
		return 1;
	}
	return 0;
}

int do_drink_check(CharData *ch, ObjData *jar) {
	//Проверка в бою?
	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		send_to_char("Не стоит отвлекаться в бою!\r\n", ch);
		return 0;
	}

	//Сообщение на случай попытки проглотить ингры
	if (GET_OBJ_TYPE(jar) == EObjType::kMagicIngredient) {
		send_to_char("Не можешь приготовить - покупай готовое!\r\n", ch);
		return 0;
	}
	//Проверяем можно ли из этого пить

	if (GET_OBJ_TYPE(jar) != EObjType::kLiquidContainer
		&& GET_OBJ_TYPE(jar) != EObjType::kFountain) {
		send_to_char("Не стоит. Козлят и так много!\r\n", ch);
		return 0;
	}

	// Если удушение - пить нельзя
	if (AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
		return 0;
	}

	// Пустой контейнер
	if (!GET_OBJ_VAL(jar, 1)) {
		send_to_char("Пусто.\r\n", ch);
		return 0;
	}

	return 1;
}

//получение контейнера для питья
ObjData *do_drink_get_jar(CharData *ch, char *jar_name) {
	ObjData *jar = nullptr;
	if (!(jar = get_obj_in_list_vis(ch, jar_name, ch->carrying))) {
		if (!(jar = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents))) {
			send_to_char("Вы не смогли это найти!\r\n", ch);
			return jar;
		}

		if (GET_OBJ_TYPE(jar) == EObjType::kLiquidContainer) {
			send_to_char("Прежде это стоит поднять.\r\n", ch);
			return jar;
		}
	}
	return jar;
}

int do_drink_get_amount(CharData *ch, ObjData *jar, int subcmd) {

	int amount = 1; //по умолчанию 1 глоток
	float V = 1;

	// Если жидкость с градусом
	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] > 0) {
		if (GET_COND(ch, DRUNK) >= kMortallyDrunked) {
			amount = -1; //мимо будет
		} else {
			//Тут магия из-за /4
			amount = (2 * kMortallyDrunked - GET_COND(ch, DRUNK)) / drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK];
			amount = MAX(1, amount); // ну еще чуть-чуть
		}
	}
		// Если без градуса
	else {
		// рандом 3-10 глотков
		amount = number(3, 10);
	}

	// Если жидкость утоляет жаду
	if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] < 0) {
		V = (float) -GET_COND(ch, THIRST) / drink_aff[GET_OBJ_VAL(jar, 2)][THIRST];
	}
		// Если жидоксть вызывает сушняк
	else if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] > 0) {
		V = (float) (kMaxCondition - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(jar, 2)][THIRST];
	} else {
		V = 999.0;
	}
	amount = MIN(amount, round(V + 0.49999));

	if (subcmd != SCMD_DRINK) // пьем прямо, не глотаем, не пробуем
	{
		amount = MIN(amount, 1);
	}

	//обрезаем до объема контейнера
	amount = MIN(amount, GET_OBJ_VAL(jar, 1));
	return amount;
}

int do_drink_check_conditions(CharData *ch, ObjData *jar, int amount) {
	//Сушняк, если у чара жажда - а напиток сушит (ВОДКА и САМОГОН!!!), заодно спасаем пьяниц от штрафов
	if (
		drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] > 0 &&
			GET_COND_M(ch, THIRST) > 5 // Когда попить уже хочется сильней чем выпить :)
		) {
		send_to_char("У вас пересохло в горле, нужно что-то попить.\r\n", ch);
		return 0;
	}


	// Если жидкость с градусом
	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] > 0) {
		// Если у чара бадун - пусть похмеляется, бухать нельзя
		if (AFF_FLAGGED(ch, EAffect::kAbstinent)) {
			if (GET_SKILL(ch, ESkill::kHangovering) > 0) {//если опохмел есть
				send_to_char(
					"Вас передернуло от одной мысли о том что бы выпить.\r\nПохоже, вам стоит опохмелиться.\r\n",
					ch);
			} else {//если опохмела нет
				send_to_char("Вы пытались... но не смогли заставить себя выпить...\r\n", ch);
			}
			return 0;
		}
		// Если чар уже пьяный
		if (GET_COND(ch, DRUNK) >= kDrunked) {
			// Если допился до кондиции или уже начал трезветь
			if (GET_DRUNK_STATE(ch) == kMortallyDrunked || GET_COND(ch, DRUNK) < GET_DRUNK_STATE(ch)) {
				send_to_char("На сегодня вам достаточно, крошки уже плавают...\r\n", ch);
				return 0;
			}
		}
	}

	// Не хочет пить, ну вот вообще не лезет больше
	if (amount <= 0 && !IS_GOD(ch)) {
		send_to_char("В вас больше не лезет.\r\n", ch);
		return 0;
	}

	// Нельзя пить в бою
	if (ch->get_fighting()) {
		send_to_char("Не стоит отвлекаться в бою.\r\n", ch);
		return 0;
	}
	return 1;
}

void do_drink_drunk(CharData *ch, ObjData *jar, int amount) {
	int duration;

	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] <= 0)
		return;

	if (amount == 0)
		return;

	if (GET_COND(ch, DRUNK) >= kDrunked) {
		if (GET_COND(ch, DRUNK) >= kMortallyDrunked) {
			send_to_char("Напилися вы пьяны, не дойти вам до дому....\r\n", ch);
		} else {
			send_to_char("Приятное тепло разлилось по вашему телу.\r\n", ch);
		}

		duration = 2 + MAX(0, GET_COND(ch, DRUNK) - kDrunked);

		if (IsAbleToUseFeat(ch, EFeat::kDrunkard))
			duration += duration / 2;

		if (!AFF_FLAGGED(ch, EAffect::kAbstinent)
			&& GET_DRUNK_STATE(ch) < kMaxCondition
			&& GET_DRUNK_STATE(ch) == GET_COND(ch, DRUNK)) {
			send_to_char("Винные пары ударили вам в голову.\r\n", ch);
			// **** Decrease AC ***** //
			Affect<EApply> af;
			af.type = kSpellDrunked;
			af.duration = CalcDuration(ch, duration, 0, 0, 0, 0);
			af.modifier = -20;
			af.location = EApply::kAc;
			af.bitvector = to_underlying(EAffect::kDrunked);
			af.battleflag = 0;
			affect_join(ch, af, false, false, false, false);
			// **** Decrease HR ***** //
			af.type = kSpellDrunked;
			af.duration = CalcDuration(ch, duration, 0, 0, 0, 0);
			af.modifier = -2;
			af.location = EApply::kHitroll;
			af.bitvector = to_underlying(EAffect::kDrunked);
			af.battleflag = 0;
			affect_join(ch, af, false, false, false, false);
			// **** Increase DR ***** //
			af.type = kSpellDrunked;
			af.duration = CalcDuration(ch, duration, 0, 0, 0, 0);
			af.modifier = (GetRealLevel(ch) + 4) / 5;
			af.location = EApply::kDamroll;
			af.bitvector = to_underlying(EAffect::kDrunked);
			af.battleflag = 0;
			affect_join(ch, af, false, false, false, false);
		}
	}
}

void do_drink(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *jar;
	int amount;

	//мобы не пьют
	if (ch->is_npc())
		return;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Пить из чего?\r\n", ch);
		return;
	}

	// Выбираем контейнер с птьем
	if (!(jar = do_drink_get_jar(ch, arg)))
		return;

	// Общие проверки
	if (!do_drink_check(ch, jar))
		return;

	// Бухаем зелье
	if (cast_potion(ch, jar))
		return;

	// Вычисляем объем жидкости доступный для потребления
	amount = do_drink_get_amount(ch, jar, subcmd);

	// Проверяем может ли чар пить
	if (!do_drink_check_conditions(ch, jar, amount))
		return;

	if (subcmd == SCMD_DRINK) {
		sprintf(buf, "$n выпил$g %s из $o1.", drinks[GET_OBJ_VAL(jar, 2)]);
		act(buf, true, ch, jar, 0, kToRoom);
		sprintf(buf, "Вы выпили %s из %s.\r\n", drinks[GET_OBJ_VAL(jar, 2)], OBJN(jar, ch, 1));
		send_to_char(buf, ch);
	} else {
		act("$n отхлебнул$g из $o1.", true, ch, jar, 0, kToRoom);
		sprintf(buf, "Вы узнали вкус %s.\r\n", drinks[GET_OBJ_VAL(jar, 2)]);
		send_to_char(buf, ch);
	}

	// вес отнимаемый вес не моежт быть больше веса контейнера

	if (GET_OBJ_TYPE(jar) != EObjType::kFountain) {
		weight_change_object(jar, -MIN(amount, GET_OBJ_WEIGHT(jar)));    // Subtract amount
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
		GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch), GET_COND(ch, DRUNK));
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][FULL] != 0) {
		gain_condition(ch, FULL, drink_aff[GET_OBJ_VAL(jar, 2)][FULL] * amount);
		if (drink_aff[GET_OBJ_VAL(jar, 2)][FULL] < 0 && GET_COND(ch, FULL) <= kNormCondition)
			send_to_char("Вы чувствуете приятную тяжесть в желудке.\r\n", ch);
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] != 0) {
		gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] * amount);
		if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] < 0 && GET_COND(ch, THIRST) <= kNormCondition)
			send_to_char("Вы не чувствуете жажды.\r\n", ch);
	}

	// Опьянение
	do_drink_drunk(ch, jar, amount);
	// Отравление
	do_drink_poison(ch, jar, amount);

	// empty the container, and no longer poison. 999 - whole fountain //
	if (GET_OBJ_TYPE(jar) != EObjType::kFountain
		|| GET_OBJ_VAL(jar, 1) != 999) {
		jar->sub_val(1, amount);
	}
	if (!GET_OBJ_VAL(jar, 1))    // The last bit //
	{
		jar->set_val(2, 0);
		jar->set_val(3, 0);
		name_from_drinkcon(jar);
	}

	return;
}

void do_drunkoff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	struct TimedSkill timed;
	int amount, weight, prob, percent, duration;
	int on_ground = 0;

	if (ch->is_npc())        // Cannot use GET_COND() on mobs. //
		return;

	if (ch->get_fighting()) {
		send_to_char("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kDrunked)) {
		send_to_char("Вы хотите испортить себе весь кураж?\r\n" "Это не есть по русски!\r\n", ch);
		return;
	}

	if (!AFF_FLAGGED(ch, EAffect::kAbstinent) && GET_COND(ch, DRUNK) < kDrunked) {
		send_to_char("Не стоит делать этого на трезвую голову.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kHangovering)) {
		send_to_char("Вы не в состоянии так часто похмеляться.\r\n"
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
			send_to_char("У вас нет подходящего напитка для похмелья.\r\n", ch);
			return;
		}
	} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents))) {
			send_to_char("Вы не смогли это найти!\r\n", ch);
			return;
		} else {
			on_ground = 1;
		}
	}

	if (GET_OBJ_TYPE(obj) != EObjType::kLiquidContainer
		&& GET_OBJ_TYPE(obj) != EObjType::kFountain) {
		send_to_char("Этим вы вряд-ли сможете похмелиться.\r\n", ch);
		return;
	}

	if (on_ground && (GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer)) {
		send_to_char("Прежде это стоит поднять.\r\n", ch);
		return;
	}

	if (!GET_OBJ_VAL(obj, 1)) {
		send_to_char("Пусто.\r\n", ch);
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
		default: send_to_char("Вспомните народную мудрость :\r\n" "\"Клин вышибают клином...\"\r\n", ch);
			return;
	}

	amount = MAX(1, GET_WEIGHT(ch) / 50);
	if (amount > GET_OBJ_VAL(obj, 1)) {
		send_to_char("Вам точно не хватит этого количества для опохмела...\r\n", ch);
		return;
	}

	timed.skill = ESkill::kHangovering;
	timed.time = IsAbleToUseFeat(ch, EFeat::kDrunkard) ? GetModifier(EFeat::kDrunkard, kFeatTimer) : 12;
	timed_to_char(ch, &timed);

	percent = number(1, MUD::Skills()[ESkill::kHangovering].difficulty);
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
		duration = MAX(1, amount / 3);
		Affect<EApply> af[3];
		af[0].type = kSpellAbstinent;
		af[0].duration = CalcDuration(ch, duration, 0, 0, 0, 0);
		af[0].modifier = 0;
		af[0].location = EApply::kDamroll;
		af[0].bitvector = to_underlying(EAffect::kAbstinent);
		af[0].battleflag = 0;
		af[1].type = kSpellAbstinent;
		af[1].duration = CalcDuration(ch, duration, 0, 0, 0, 0);
		af[1].modifier = 0;
		af[1].location = EApply::kHitroll;
		af[1].bitvector = to_underlying(EAffect::kAbstinent);
		af[1].battleflag = 0;
		af[2].type = kSpellAbstinent;
		af[2].duration = CalcDuration(ch, duration, 0, 0, 0, 0);
		af[2].modifier = 0;
		af[2].location = EApply::kAc;
		af[2].bitvector = to_underlying(EAffect::kAbstinent);
		af[2].battleflag = 0;
		switch (number(0, ch->get_skill(ESkill::kHangovering) / 20)) {
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
			affect_join(ch, af[prob], true, false, true, false);
		}
		gain_condition(ch, DRUNK, amount);
	} else {
		sprintf(buf, "Вы отхлебнули %s из $o1 и почувствовали приятную легкость во всем теле...",
				drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, false, ch, obj, 0, kToChar);
		act("$n похмелил$u и расцвел$g прям на глазах.", false, ch, 0, 0, kToRoom);
		affect_from_char(ch, kSpellAbstinent);
	}

	return;
}

void generate_drinkcon_name(ObjData *to_obj, int spell) {
	switch (spell) {
		// восстановление (красное) //
		case kSpellResfresh:
		case kSpellGroupRefresh: to_obj->set_val(2, LIQ_POTION_RED);
			name_to_drinkcon(to_obj, LIQ_POTION_RED);
			break;
			// насыщение (синее) //
		case kSpellFullFeed:
		case kSpellCommonMeal: to_obj->set_val(2, LIQ_POTION_BLUE);
			name_to_drinkcon(to_obj, LIQ_POTION_BLUE);
			break;
			// детекты (белое) //
		case kSpellDetectInvis:
		case kSpellAllSeeingEye:
		case kSpellDetectMagic:
		case kSpellMagicalGaze:
		case kSpellDetectPoison:
		case kSpellSnakeEyes:
		case kSpellDetectAlign:
		case kSpellGroupSincerity:
		case kSpellSenseLife:
		case kSpellEyeOfGods:
		case kSpellInfravision:
		case kSpellSightOfDarkness: to_obj->set_val(2, LIQ_POTION_WHITE);
			name_to_drinkcon(to_obj, LIQ_POTION_WHITE);
			break;
			// защитные (золотистое) //
		case kSpellArmor:
		case kSpellGroupArmor:
		case kSpellCloudly: to_obj->set_val(2, LIQ_POTION_GOLD);
			name_to_drinkcon(to_obj, LIQ_POTION_GOLD);
			break;
			// восстанавливающие здоровье (черное) //
		case kSpellCureCritic:
		case kSpellCureLight:
		case kSpellHeal:
		case kSpellGroupHeal:
		case kSpellCureSerious: to_obj->set_val(2, LIQ_POTION_BLACK);
			name_to_drinkcon(to_obj, LIQ_POTION_BLACK);
			break;
			// снимающее вредные аффекты (серое) //
		case kSpellCureBlind:
		case kSpellRemoveCurse:
		case kSpellRemoveHold:
		case kSpellRemoveSilence:
		case kSpellCureFever:
		case kSpellRemoveDeafness:
		case kSpellRemovePoison: to_obj->set_val(2, LIQ_POTION_GREY);
			name_to_drinkcon(to_obj, LIQ_POTION_GREY);
			break;
			// прочие полезности (фиолетовое) //
		case kSpellInvisible:
		case kSpellGroupInvisible:
		case kSpellStrength:
		case kSpellGroupStrength:
		case kSpellFly:
		case kSpellGroupFly:
		case kSpellBless:
		case kSpellGroupBless:
		case kSpellHaste:
		case kSpellGroupHaste:
		case kSpellStoneSkin:
		case kSpellStoneWall:
		case kSpellBlink:
		case kSpellExtraHits:
		case kSpellWaterbreath: to_obj->set_val(2, LIQ_POTION_FUCHSIA);
			name_to_drinkcon(to_obj, LIQ_POTION_FUCHSIA);
			break;
		case kSpellPrismaticAura:
		case kSpellGroupPrismaticAura:
		case kSpellAirAura:
		case kSpellEarthAura:
		case kSpellFireAura:
		case kSpellIceAura: to_obj->set_val(2, LIQ_POTION_PINK);
			name_to_drinkcon(to_obj, LIQ_POTION_PINK);
			break;
		default: to_obj->set_val(2, LIQ_POTION);
			name_to_drinkcon(to_obj, LIQ_POTION);    // добавляем новый синоним //
	}
}

int check_potion_spell(ObjData *from_obj, ObjData *to_obj, int num) {
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (GET_OBJ_VAL(from_obj, num) != to_obj->get_value(spell)) {
		// не совпали заклы
		return 0;
	}
	if (GET_OBJ_VAL(from_obj, 0) < to_obj->get_value(level)) {
		// переливаемое зелье ниже уровня закла в емкости
		return -1;
	}
	return 1;
}

/// \return 1 - можно переливать
///         0 - нельзя смешивать разные зелья
///        -1 - попытка перелить зелье с меньшим уровнем закла
int check_equal_potions(ObjData *from_obj, ObjData *to_obj) {
	// емкость с уже перелитым ранее зельем
	if (to_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) > 0
		&& GET_OBJ_VNUM(from_obj) != to_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM)) {
		return 0;
	}
	// совпадение заклов и не меньшего уровня
	for (int i = 1; i <= 3; ++i) {
		if (GET_OBJ_VAL(from_obj, i) > 0) {
			int result = check_potion_spell(from_obj, to_obj, i);
			if (result <= 0) {
				return result;
			}
		}
	}
	return 1;
}

/// см check_equal_drinkcon()
int check_drincon_spell(ObjData *from_obj, ObjData *to_obj, int num) {
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (from_obj->get_value(spell) != to_obj->get_value(spell)) {
		// не совпали заклы
		return 0;
	}
	if (from_obj->get_value(level) < to_obj->get_value(level)) {
		// переливаемое зелье ниже уровня закла в емкости
		return -1;
	}
	return 1;
}

/// Временная версия check_equal_potions для двух емкостей, пока в пошенах
/// еще нет values с заклами/уровнями.
/// \return 1 - можно переливать
///         0 - нельзя смешивать разные зелья
///        -1 - попытка перелить зелье с меньшим уровнем закла
int check_equal_drinkcon(ObjData *from_obj, ObjData *to_obj) {
	// совпадение заклов и не меньшего уровня (и в том же порядке, ибо влом)
	for (int i = 1; i <= 3; ++i) {
		if (GET_OBJ_VAL(from_obj, i) > 0) {
			int result = check_drincon_spell(from_obj, to_obj, i);
			if (result <= 0) {
				return result;
			}
		}
	}
	return 1;
}

/// копирование полей заклинаний при переливании из емкости с зельем в пустую емкость
void spells_to_drinkcon(ObjData *from_obj, ObjData *to_obj) {
	// инит заклов
	for (int i = 1; i <= 3; ++i) {
		const auto spell = init_spell_num(i);
		const auto level = init_spell_lvl(i);
		to_obj->set_value(spell, from_obj->get_value(spell));
		to_obj->set_value(level, from_obj->get_value(level));
	}
	// сохранение инфы о первоначальном источнике зелья
	const int proto_vnum = from_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) > 0
						   ? from_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM)
						   : GET_OBJ_VNUM(from_obj);
	to_obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, proto_vnum);
}

void do_pour(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *from_obj = nullptr, *to_obj = nullptr;
	int amount;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR) {
		if (!*arg1)    // No arguments //
		{
			send_to_char("Откуда переливаем?\r\n", ch);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			send_to_char("У вас нет этого!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != EObjType::kLiquidContainer
			&& GET_OBJ_TYPE(from_obj) != EObjType::kPorion) {
			send_to_char("Вы не можете из этого переливать!\r\n", ch);
			return;
		}
	}
	if (subcmd == SCMD_FILL) {
		if (!*arg1)    // no arguments //
		{
			send_to_char("Что и из чего вы хотели бы наполнить?\r\n", ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			send_to_char("У вас этого нет!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != EObjType::kLiquidContainer) {
			act("Вы не можете наполнить $o3!", false, ch, to_obj, 0, kToChar);
			return;
		}
		if (!*arg2)    // no 2nd argument //
		{
			act("Из чего вы планируете наполнить $o3?", false, ch, to_obj, 0, kToChar);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room]->contents))) {
			sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg2);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != EObjType::kFountain) {
			act("Вы не сможете ничего наполнить из $o1.", false, ch, from_obj, 0, kToChar);
			return;
		}
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("Пусто.", false, ch, from_obj, 0, kToChar);
		return;
	}
	if (subcmd == SCMD_POUR)    // pour //
	{
		if (!*arg2) {
			send_to_char("Куда вы хотите лить? На землю или во что-то?\r\n", ch);
			return;
		}
		if (!str_cmp(arg2, "out") || !str_cmp(arg2, "земля")) {
			act("$n опустошил$g $o3.", true, ch, from_obj, 0, kToRoom);
			act("Вы опустошили $o3.", false, ch, from_obj, 0, kToChar);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));    // Empty //

			from_obj->set_val(1, 0);
			from_obj->set_val(2, 0);
			from_obj->set_val(3, 0);
			from_obj->set_skill(0);
			name_from_drinkcon(from_obj);
			reset_potion_values(from_obj);

			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
			send_to_char("Вы не можете этого найти!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != EObjType::kLiquidContainer
			&& GET_OBJ_TYPE(to_obj) != EObjType::kFountain) {
			send_to_char("Вы не сможете в это налить.\r\n", ch);
			return;
		}
	}
	if (to_obj == from_obj) {
		send_to_char("Более тупого действа вы придумать, конечно, не могли.\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 1) != 0
		&& GET_OBJ_TYPE(from_obj) != EObjType::kPorion
		&& GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2)) {
		send_to_char("Вы станете неплохим Химиком, но не в нашей игре.\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(to_obj, 1) >= GET_OBJ_VAL(to_obj, 0)) {
		send_to_char("Там нет места.\r\n", ch);
		return;
	}
	if (from_obj->has_flag(EObjFlag::kNopour)) {
		send_to_char(ch, "Вы перевернули %s, потрусили, но ничего перелить не удалось.\r\n",
					 GET_OBJ_PNAME(from_obj, 3).c_str());
		return;
	}
//Added by Adept - переливание зелья из бутылки или емкости в емкость

	//Переливает из бутылки с зельем в емкость
	if (GET_OBJ_TYPE(from_obj) == EObjType::kPorion) {
		int result = check_equal_potions(from_obj, to_obj);
		if (GET_OBJ_VAL(to_obj, 1) == 0 || result > 0) {
			send_to_char(ch, "Вы занялись переливанием зелья в %s.\r\n",
						 OBJN(to_obj, ch, 3));
			int n1 = GET_OBJ_VAL(from_obj, 1);
			int n2 = GET_OBJ_VAL(to_obj, 1);
			int t1 = GET_OBJ_VAL(from_obj, 3);
			int t2 = GET_OBJ_VAL(to_obj, 3);
			to_obj->set_val(3,
							(n1 * t1 + n2 * t2)
								/ (n1 + n2)); //усредним таймер в зависимости от наполненности обоих емкостей
//				send_to_char(ch, "n1 == %d, n2 == %d, t1 == %d, t2== %d, результат %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));
			if (GET_OBJ_VAL(to_obj, 1) == 0) {
				copy_potion_values(from_obj, to_obj);
				// определение названия зелья по содержащемуся заклинанию //
				generate_drinkcon_name(to_obj, GET_OBJ_VAL(from_obj, 1));
			}
			weight_change_object(to_obj, 1);
			to_obj->inc_val(1);
			extract_obj(from_obj);
			return;
		} else if (result < 0) {
			send_to_char("Не пытайтесь подмешать более слабое зелье!\r\n", ch);
			return;
		} else {
			send_to_char(
				"Смешивать разные зелья?! Да вы, батенька, гурман!\r\n", ch);
			return;
		}
	}

	//Переливает из емкости или колодца с зельем куда-то
	if ((GET_OBJ_TYPE(from_obj) == EObjType::kLiquidContainer
		|| GET_OBJ_TYPE(from_obj) == EObjType::kFountain)
		&& is_potion(from_obj)) {
		if (GET_OBJ_VAL(to_obj, 1) == 0) {
			spells_to_drinkcon(from_obj, to_obj);
		} else {
			const int result = check_equal_drinkcon(from_obj, to_obj);
			if (result < 0) {
				send_to_char("Не пытайтесь подмешать более слабое зелье!\r\n", ch);
				return;
			} else if (!result) {
				send_to_char(
					"Смешивать разные зелья?! Да вы, батенька, гурман!\r\n", ch);
				return;
			}
		}
	}
//Конец изменений Adept'ом

	if (subcmd == SCMD_POUR) {
		send_to_char(ch, "Вы занялись переливанием %s в %s.\r\n",
					 drinks[GET_OBJ_VAL(from_obj, 2)], OBJN(to_obj, ch, 3));
	}
	if (subcmd == SCMD_FILL) {
		act("Вы наполнили $o3 из $O1.", false, ch, to_obj, from_obj, kToChar);
		act("$n наполнил$g $o3 из $O1.", true, ch, to_obj, from_obj, kToRoom);
	}

	// копируем тип жидкости //
	to_obj->set_val(2, GET_OBJ_VAL(from_obj, 2));

	int n1 = GET_OBJ_VAL(from_obj, 1);
	int n2 = GET_OBJ_VAL(to_obj, 1);
	int t1 = GET_OBJ_VAL(from_obj, 3);
	int t2 = GET_OBJ_VAL(to_obj, 3);
	to_obj->set_val(3, (n1 * t1 + n2 * t2) / (n1 + n2)); //усредним таймер в зависимости от наполненности обоих емкостей
//	send_to_char(ch, "n1 == %d, n2 == %d, t1 == %d, t2== %d, результат %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));

	// New alias //
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));
	// Then how much to pour //
	amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1));
	if (GET_OBJ_TYPE(from_obj) != EObjType::kFountain
		|| GET_OBJ_VAL(from_obj, 1) != 999) {
		from_obj->sub_val(1, amount);
	}
	to_obj->set_val(1, GET_OBJ_VAL(to_obj, 0));

	// Then the poison boogie //


	if (GET_OBJ_VAL(from_obj, 1) <= 0)    // There was too little //
	{
		to_obj->add_val(1, GET_OBJ_VAL(from_obj, 1));
		amount += GET_OBJ_VAL(from_obj, 1);
		from_obj->set_val(1, 0);
		from_obj->set_val(2, 0);
		from_obj->set_val(3, 0);
		name_from_drinkcon(from_obj);
		reset_potion_values(from_obj);
	}

	// And the weight boogie //
	if (GET_OBJ_TYPE(from_obj) != EObjType::kFountain) {
		weight_change_object(from_obj, -amount);
	}
	weight_change_object(to_obj, amount);    // Add weight //
}

size_t find_liquid_name(const char *name) {
	std::string tmp = std::string(name);
	size_t pos, result = std::string::npos;
	for (int i = 0; strcmp(drinknames[i], "\n"); i++) {
		pos = tmp.find(drinknames[i]);
		if (pos != std::string::npos) {
			result = pos;
		}
	}
	return result;
}

void name_from_drinkcon(ObjData *obj) {
	char new_name[kMaxStringLength];
	std::string tmp;

	size_t pos = find_liquid_name(obj->get_aliases().c_str());
	if (pos == std::string::npos) return;
	tmp = obj->get_aliases().substr(0, pos - 1);

	sprintf(new_name, "%s", tmp.c_str());
	obj->set_aliases(new_name);

	pos = find_liquid_name(obj->get_short_description().c_str());
	if (pos == std::string::npos) return;
	tmp = obj->get_short_description().substr(0, pos - 3);

	sprintf(new_name, "%s", tmp.c_str());
	obj->set_short_description(new_name);

	for (int c = 0; c < CObjectPrototype::NUM_PADS; c++) {
		pos = find_liquid_name(obj->get_PName(c).c_str());
		if (pos == std::string::npos) return;
		tmp = obj->get_PName(c).substr(0, pos - 3);
		sprintf(new_name, "%s", tmp.c_str());
		obj->set_PName(c, new_name);
	}
}

void name_to_drinkcon(ObjData *obj, int type) {
	int c;
	char new_name[kMaxInputLength], potion_name[kMaxInputLength];
	if (type >= NUM_LIQ_TYPES) {
		snprintf(potion_name, kMaxInputLength, "%s", "непонятной бормотухой, сообщите БОГАМ");
	} else {
		snprintf(potion_name, kMaxInputLength, "%s", drinknames[type]);
	}

	snprintf(new_name, kMaxInputLength, "%s %s", obj->get_aliases().c_str(), potion_name);
	obj->set_aliases(new_name);
	snprintf(new_name, kMaxInputLength, "%s с %s", obj->get_short_description().c_str(), potion_name);
	obj->set_short_description(new_name);

	for (c = 0; c < CObjectPrototype::NUM_PADS; c++) {
		snprintf(new_name, kMaxInputLength, "%s с %s", obj->get_PName(c).c_str(), potion_name);
		obj->set_PName(c, new_name);
	}
}

std::string print_spell(CharData *ch, const ObjData *obj, int num) {
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (obj->get_value(spell) == -1) {
		return "";
	}

	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_), "Содержит заклинание: %s%s (%d ур.)%s\r\n",
			 CCCYN(ch, C_NRM),
			 GetSpellName(obj->get_value(spell)),
			 obj->get_value(level),
			 CCNRM(ch, C_NRM));

	return buf_;
}

namespace drinkcon {

std::string print_spells(CharData *ch, const ObjData *obj) {
	std::string out;
	char buf_[kMaxInputLength];

	for (int i = 1; i <= 3; ++i) {
		out += print_spell(ch, obj, i);
	}

	if (!out.empty() && !is_potion(obj)) {
		snprintf(buf_, sizeof(buf_), "%sВНИМАНИЕ%s: тип жидкости не является зельем\r\n",
				 CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		out += buf_;
	} else if (out.empty() && is_potion(obj)) {
		snprintf(buf_, sizeof(buf_), "%sВНИМАНИЕ%s: у данного зелья отсутствуют заклинания\r\n",
				 CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		out += buf_;
	}

	return out;
}

void identify(CharData *ch, const ObjData *obj) {
	std::string out;
	char buf_[kMaxInputLength];
	int volume = GET_OBJ_VAL(obj, 0);
	int amount = GET_OBJ_VAL(obj, 1);

	snprintf(buf_, sizeof(buf_), "Может вместить зелья: %s%d %s%s\r\n",
			 CCCYN(ch, C_NRM),
			 volume, GetDeclensionInNumber(volume, EWhat::kGulp),
			 CCNRM(ch, C_NRM));
	out += buf_;

	// емкость не пуста
	if (amount > 0) {
		// есть какие-то заклы
		if (obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) >= 0) {
			if (IS_IMMORTAL(ch)) {
				snprintf(buf_, sizeof(buf_), "Содержит %d %s %s (VNUM: %d).\r\n",
						 amount,
						 GetDeclensionInNumber(amount, EWhat::kGulp),
						 drinks[GET_OBJ_VAL(obj, 2)],
						 obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM));
			} else {
				snprintf(buf_, sizeof(buf_), "Содержит %d %s %s.\r\n",
						 amount,
						 GetDeclensionInNumber(amount, EWhat::kGulp),
						 drinks[GET_OBJ_VAL(obj, 2)]);
			}
			out += buf_;
			out += print_spells(ch, obj);
		} else {
			snprintf(buf_, sizeof(buf_), "Заполнен%s %s на %d%%\r\n",
					 GET_OBJ_SUF_6(obj),
					 drinknames[GET_OBJ_VAL(obj, 2)],
					 amount * 100 / (volume ? volume : 1));
			out += buf_;
			// чтобы выдать варнинг на тему зелья без заклов
			if (is_potion(obj)) {
				out += print_spells(ch, obj);
			}
		}
	}
	if (amount > 0) //если что-то плескается
	{
		sprintf(buf1, "Качество: %s \r\n", diag_liquid_timer(obj)); // состояние жижки
		out += buf1;
	}
	send_to_char(out, ch);
}

} // namespace drinkcon

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
