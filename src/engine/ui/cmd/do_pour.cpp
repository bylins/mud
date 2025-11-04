/**
\file do_pour.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 28.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "do_pour.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/liquid.h"

void do_pour(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *from_obj = nullptr, *to_obj = nullptr;
	int amount;

	two_arguments(argument, arg1, arg2);

	if (subcmd == kScmdPour) {
		if (!*arg1)    // No arguments //
		{
			SendMsgToChar("Откуда переливаем?\r\n", ch);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			SendMsgToChar("У вас нет этого!\r\n", ch);
			return;
		}
		if (from_obj->get_type() != EObjType::kLiquidContainer
			&& from_obj->get_type() != EObjType::kPotion) {
			SendMsgToChar("Вы не можете из этого переливать!\r\n", ch);
			return;
		}
	}
	if (subcmd == kScmdFill) {
		if (!*arg1)    // no arguments //
		{
			SendMsgToChar("Что и из чего вы хотели бы наполнить?\r\n", ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			SendMsgToChar("У вас этого нет!\r\n", ch);
			return;
		}
		if (to_obj->get_type() != EObjType::kLiquidContainer) {
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
			SendMsgToChar(buf, ch);
			return;
		}
		if (from_obj->get_type() != EObjType::kFountain) {
			act("Вы не сможете ничего наполнить из $o1.", false, ch, from_obj, 0, kToChar);
			return;
		}
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("Пусто.", false, ch, from_obj, 0, kToChar);
		return;
	}
	if (subcmd == kScmdPour)    // pour //
	{
		if (!*arg2) {
			SendMsgToChar("Куда вы хотите лить? На землю или во что-то?\r\n", ch);
			return;
		}
		if (!str_cmp(arg2, "out") || !str_cmp(arg2, "земля")) {
			act("$n опустошил$g $o3.", true, ch, from_obj, 0, kToRoom);
			act("Вы опустошили $o3.", false, ch, from_obj, 0, kToChar);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));    // Empty //

			from_obj->set_val(1, 0);
			from_obj->set_val(2, 0);
			from_obj->set_val(3, 0);
			from_obj->set_spec_param(0);
			name_from_drinkcon(from_obj);
			drinkcon::reset_potion_values(from_obj);

			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
			SendMsgToChar("Вы не можете этого найти!\r\n", ch);
			return;
		}
		if (to_obj->get_type() != EObjType::kLiquidContainer
			&& to_obj->get_type() != EObjType::kFountain) {
			SendMsgToChar("Вы не сможете в это налить.\r\n", ch);
			return;
		}
	}
	if (to_obj == from_obj) {
		SendMsgToChar("Более тупого действа вы придумать, конечно, не могли.\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 1) != 0
		&& from_obj->get_type() != EObjType::kPotion
		&& GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2)) {
		SendMsgToChar("Вы станете неплохим Химиком, но не в нашей игре.\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(to_obj, 1) >= GET_OBJ_VAL(to_obj, 0)) {
		SendMsgToChar("Там нет места.\r\n", ch);
		return;
	}
	if (from_obj->has_flag(EObjFlag::kNopour)) {
		SendMsgToChar(ch, "Вы перевернули %s, потрусили, но ничего перелить не удалось.\r\n",
					  from_obj->get_PName(ECase::kAcc).c_str());
		return;
	}

	//Переливает из бутылки с зельем в емкость
	if (from_obj->get_type() == EObjType::kPotion) {
		int result = drinkcon::check_equal_potions(from_obj, to_obj);
		if (GET_OBJ_VAL(to_obj, 1) == 0 || result > 0) {
			SendMsgToChar(ch, "Вы занялись переливанием зелья в %s.\r\n", OBJN(to_obj, ch, ECase::kAcc));
			int n1 = GET_OBJ_VAL(from_obj, 1);
			int n2 = GET_OBJ_VAL(to_obj, 1);
			int t1 = GET_OBJ_VAL(from_obj, 3);
			int t2 = GET_OBJ_VAL(to_obj, 3);
			to_obj->set_val(3,
							(n1 * t1 + n2 * t2)
								/ (n1 + n2)); //усредним таймер в зависимости от наполненности обоих емкостей
//				SendMsgToChar(ch, "n1 == %d, n2 == %d, t1 == %d, t2== %d, результат %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));
//				sprintf(buf, "Игрок %s наполняет емкость. Первая емкость: %s (%d) Вторая емкость %s (%d). SCMD %d",
//						GET_NAME(ch), from_obj->get_PName(ECase::kGen).c_str(), GET_OBJ_VNUM(from_obj), to_obj->get_PName(ECase::kGen).c_str(),  GET_OBJ_VNUM(to_obj), subcmd);
//				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
//				sprintf(buf, "кол1 == %d, кол2 == %d, time1 == %d, time2== %d, результат %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));
//				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			if (GET_OBJ_VAL(to_obj, 1) == 0) {
				drinkcon::copy_potion_values(from_obj, to_obj);
				// определение названия зелья по содержащемуся заклинанию //
				drinkcon::generate_drinkcon_name(to_obj, static_cast<ESpell>(GET_OBJ_VAL(from_obj, 1)));
			}
			weight_change_object(to_obj, 1);
			to_obj->inc_val(1);
			ExtractObjFromWorld(from_obj);
			return;
		} else if (result < 0) {
			SendMsgToChar("Не пытайтесь подмешать более слабое зелье!\r\n", ch);
			return;
		} else {
			SendMsgToChar(
				"Смешивать разные зелья?! Да вы, батенька, гурман!\r\n", ch);
			return;
		}
	}

	//Переливает из емкости или колодца с зельем куда-то
	if ((from_obj->get_type() == EObjType::kLiquidContainer
		|| from_obj->get_type() == EObjType::kFountain)
		&& is_potion(from_obj)) {
		if (GET_OBJ_VAL(to_obj, 1) == 0) {
			drinkcon::spells_to_drinkcon(from_obj, to_obj);
		} else {
			const int result = drinkcon::check_equal_drinkcon(from_obj, to_obj);
			if (result < 0) {
				SendMsgToChar("Не пытайтесь подмешать более слабое зелье!\r\n", ch);
				return;
			} else if (!result) {
				SendMsgToChar(
					"Смешивать разные зелья?! Да вы, батенька, гурман!\r\n", ch);
				return;
			}
		}
	}

	if (subcmd == kScmdPour) {
		SendMsgToChar(ch, "Вы занялись переливанием %s в %s.\r\n",
					  drinks[GET_OBJ_VAL(from_obj, 2)], OBJN(to_obj, ch, ECase::kAcc));
	}
	if (subcmd == kScmdFill) {
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
//	sprintf(buf, "Игрок %s переливает жижку. Первая емкость: %s (%d) Вторая емкость %s (%d). SCMD %d",
//		GET_NAME(ch), from_obj->get_PName(ECase::kGen).c_str(), GET_OBJ_VNUM(from_obj), to_obj->get_PName(ECase::kGen).c_str(),  GET_OBJ_VNUM(to_obj), subcmd);
//	mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
//	sprintf(buf, "кол1 == %d, кол2 == %d, time1 == %d, time2== %d, результат %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));
//	mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);

	// New alias //
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));
	// Then how much to pour //
	amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1));
	if (from_obj->get_type() != EObjType::kFountain
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
		drinkcon::reset_potion_values(from_obj);
	}

	// And the weight boogie //
	if (from_obj->get_type() != EObjType::kFountain) {
		weight_change_object(from_obj, -amount);
	}
	weight_change_object(to_obj, amount);    // Add weight //
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
