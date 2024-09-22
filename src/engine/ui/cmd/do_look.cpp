/**
\file look.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/illumination.h"

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
					kColorNrm, kColorBoldBlk, ch->in_room,
					kColorRed, kColorBoldRed, world[ch->in_room]->light,
					kColorGrn, kColorBoldGrn, world[ch->in_room]->glight,
					kColorYel, kColorBoldYel, world[ch->in_room]->fires,
					kColorYel, kColorBoldYel, world[ch->in_room]->ices,
					kColorBlu, kColorBoldBlu, world[ch->in_room]->gdark,
					kColorMag, kColorBoldCyn, weather_info.sky,
					kColorWht, kColorBoldBlk, weather_info.sunlight,
					kColorYel, kColorBoldYel, weather_info.moon_day, kColorNrm);
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
