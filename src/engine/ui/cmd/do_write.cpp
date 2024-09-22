/**
\file do_write.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/ui/modify.h"

const int kMaxNoteLength{4096};    // arbitrary

void do_write(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *paper, *pen = nullptr;
	char papername[kMaxInputLength], penname[kMaxInputLength];

	two_arguments(argument, papername, penname);

	if (!ch->desc)
		return;

	if (!*papername) {
		SendMsgToChar("Написать? Чем? И на чем?\r\n", ch);
		return;
	}
	if (*penname) {
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			sprintf(buf, "У вас нет %s.\r\n", papername);
			SendMsgToChar(buf, ch);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
			sprintf(buf, "У вас нет %s.\r\n", penname);
			SendMsgToChar(buf, ch);
			return;
		}
	} else {
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			sprintf(buf, "Вы не видите %s в инвентаре.\r\n", papername);
			SendMsgToChar(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(paper) == EObjType::kPen)    // oops, a pen..
		{
			pen = paper;
			paper = nullptr;
		} else if (GET_OBJ_TYPE(paper) != EObjType::kNote) {
			SendMsgToChar("Вы не можете на ЭТОМ писать.\r\n", ch);
			return;
		}

		if (!GET_EQ(ch, kHold)) {
			sprintf(buf, "Вам нечем писать!\r\n");
			SendMsgToChar(buf, ch);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, kHold))) {
			SendMsgToChar("Вы держите что-то невидимое!  Жаль, но писать этим трудно!!\r\n", ch);
			return;
		}
		if (pen)
			paper = GET_EQ(ch, kHold);
		else
			pen = GET_EQ(ch, kHold);
	}

	if (GET_OBJ_TYPE(pen) != EObjType::kPen) {
		act("Вы не умеете писать $o4.", false, ch, pen, 0, kToChar);
	} else if (GET_OBJ_TYPE(paper) != EObjType::kNote) {
		act("Вы не можете писать на $o5.", false, ch, paper, 0, kToChar);
	} else if (!paper->get_action_description().empty()) {
		SendMsgToChar("Там уже что-то записано.\r\n", ch);
	} else {
		/* this is the PERFECT code example of how to set up:
		 * a) the text editor with a message already loaed
		 * b) the abort buffer if the player aborts the message
		 */
		ch->desc->backstr = nullptr;
		SendMsgToChar("Можете писать.  (/s СОХРАНИТЬ ЗАПИСЬ  /h ПОМОЩЬ)\r\n", ch);
		if (!paper->get_action_description().empty())  {
			ch->desc->backstr = str_dup(paper->get_action_description().c_str());
			SendMsgToChar(paper->get_action_description().c_str(), ch);
		}
		act("$n начал$g писать.", true, ch, 0, 0, kToRoom);
		const utils::AbstractStringWriter::shared_ptr writer(new CActionDescriptionWriter(*paper));
		string_write(ch->desc, writer, kMaxNoteLength, 0, nullptr);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
