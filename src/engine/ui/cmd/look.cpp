/**
\file look.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/sight.h"

void DoLook(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char arg2[kMaxInputLength];
	int look_type;

	if (!ch->desc)
		return;
	if (ch->GetPosition() < EPosition::kSleep) {
		SendMsgToChar("Виделся часто сон беспокойный...\r\n", ch);
	} else if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы ослеплены!\r\n", ch);
	} else if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
		if (GetRealLevel(ch) > 30) {
			sprintf(buf,
					"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
					"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
					CCNRM(ch, C_NRM), CCINRM(ch, C_NRM), ch->in_room,
					CCRED(ch, C_NRM), CCIRED(ch, C_NRM), world[ch->in_room]->light,
					CCGRN(ch, C_NRM), CCIGRN(ch, C_NRM), world[ch->in_room]->glight,
					CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[ch->in_room]->fires,
					CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[ch->in_room]->ices,
					CCBLU(ch, C_NRM), CCIBLU(ch, C_NRM), world[ch->in_room]->gdark,
					CCMAG(ch, C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
					CCWHT(ch, C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
					CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day, CCNRM(ch, C_NRM));
			SendMsgToChar(buf, ch);
		}
		skip_hide_on_look(ch);

		SendMsgToChar("Слишком темно...\r\n", ch);
		list_char_to_char(world[ch->in_room]->people, ch);    // glowing red eyes
		show_glow_objs(ch);
	} else {
		half_chop(argument, arg, arg2);

		skip_hide_on_look(ch);

		if (subcmd == SCMD_READ) {
			if (!*arg)
				SendMsgToChar("Что вы хотите прочитать?\r\n", ch);
			else
				look_at_target(ch, arg, subcmd);
			return;
		}
		if (!*arg)    // "look" alone, without an argument at all
		{
			if (ch->desc) {
				ch->desc->msdp_report("ROOM");
			}
			look_at_room(ch, 1);
		} else if (utils::IsAbbr(arg, "in") || utils::IsAbbr(arg, "внутрь"))
			look_in_obj(ch, arg2);
			// did the char type 'look <direction>?'
		else if (((look_type = search_block(arg, dirs, false)) >= 0) ||
			((look_type = search_block(arg, dirs_rus, false)) >= 0))
			look_in_direction(ch, look_type, EXIT_SHOW_WALL);
		else if (utils::IsAbbr(arg, "at") || utils::IsAbbr(arg, "на"))
			look_at_target(ch, arg2, subcmd);
		else
			look_at_target(ch, argument, subcmd);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
