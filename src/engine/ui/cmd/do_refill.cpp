#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

//переложить стрелы из пучка стрел в колчан
void do_refill(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *from_obj = nullptr, *to_obj = nullptr;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		SendMsgToChar("Откуда брать стрелы?\r\n", ch);
		return;
	}
	if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		SendMsgToChar("У вас нет этого!\r\n", ch);
		return;
	}
	if (GET_OBJ_TYPE(from_obj) != EObjType::kMagicArrow) {
		SendMsgToChar("И как вы себе это представляете?\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("Пусто.", false, ch, from_obj, nullptr, kToChar);
		return;
	}
	if (!*arg2) {
		SendMsgToChar("Куда вы хотите их засунуть?\r\n", ch);
		return;
	}
	if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		SendMsgToChar("Вы не можете этого найти!\r\n", ch);
		return;
	}
	if (!((GET_OBJ_TYPE(to_obj) == EObjType::kMagicContaner)
		|| GET_OBJ_TYPE(to_obj) == EObjType::kMagicArrow)) {
		SendMsgToChar("Вы не сможете в это сложить стрелы.\r\n", ch);
		return;
	}

	if (to_obj == from_obj) {
		SendMsgToChar("Нечем заняться? На печи ездить еще не научились?\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 2) >= GET_OBJ_VAL(to_obj, 1)) {
		SendMsgToChar("Там нет места.\r\n", ch);
		return;
	} else //вроде прошли все проверки. начинаем перекладывать
	{
		if (GET_OBJ_VAL(from_obj, 0) != GET_OBJ_VAL(to_obj, 0)) {
			SendMsgToChar("Хамово ремесло еще не известно на руси.\r\n", ch);
			return;
		}
		int t1 = GET_OBJ_VAL(from_obj, 3);  // количество зарядов
		int t2 = GET_OBJ_VAL(to_obj, 3);
		int delta = (GET_OBJ_VAL(to_obj, 2) - GET_OBJ_VAL(to_obj, 3));
		if (delta >= t1) //объем колчана больше пучка
		{
			to_obj->add_val(2, t1);
			SendMsgToChar("Вы аккуратно сложили стрелы в колчан.\r\n", ch);
			ExtractObjFromWorld(from_obj);
			return;
		} else {
			to_obj->add_val(2, (t2 - GET_OBJ_VAL(to_obj, 2)));
			SendMsgToChar("Вы аккуратно переложили несколько стрел в колчан.\r\n", ch);
			from_obj->add_val(2, (GET_OBJ_VAL(to_obj, 2) - t2));
			return;
		}
	}

	SendMsgToChar("С таким успехом можно пополнять соседние камни, для разговоров по ним.\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
