/**
\file look.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/illumination.h"
#include "engine/db/world_characters.h"

void DoLook(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char first_word[kMaxInputLength];
	char arg2[kMaxInputLength];
	int look_type;

	if (!ch->desc)
		return;
	if (ch->GetPosition() < EPosition::kSleep) {
		SendMsgToChar("Виделся часто сон беспокойный...\r\n", ch);
	} else if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы ослеплены!\r\n", ch);
	} else if (is_dark(ch->in_room) && !sight::CanSeeInDark(ch)) {
		if (GetRealLevel(ch) > 30) {
			SendMsgToChar(fmt::format(
					"{}Комната={}{} {}Свет={}{} {}Освещ={}{} {}Костер={}{} {}Лед={}{} "
					"{}Тьма={}{} {}Солнце={}{} {}Небо={}{} {}Луна={}{}{}.\r\n",
					kColorNrm, kColorBoldBlk, ch->in_room,
					kColorRed, kColorBoldRed, world[ch->in_room]->light,
					kColorGrn, kColorBoldGrn, world[ch->in_room]->glight,
					kColorYel, kColorBoldYel, world[ch->in_room]->fires,
					kColorYel, kColorBoldYel, world[ch->in_room]->ices,
					kColorBlu, kColorBoldBlu, world[ch->in_room]->gdark,
					kColorMag, kColorBoldCyn, weather_info.sky,
					kColorWht, kColorBoldBlk, weather_info.sunlight,
					kColorYel, kColorBoldYel, weather_info.moon_day, kColorNrm), ch);
		}
		sight::skip_hide_on_look(ch);

		SendMsgToChar("Слишком темно...\r\n", ch);
		sight::list_char_to_char(world[ch->in_room]->people, ch);    // glowing red eyes
		sight::show_glow_objs(ch);
	} else {
		half_chop(argument, first_word, arg2);

		sight::skip_hide_on_look(ch);

		if (subcmd == kScmdRead) {
			if (!*first_word)
				SendMsgToChar("Что вы хотите прочитать?\r\n", ch);
			else
				sight::look_at_target(ch, first_word, subcmd);
			return;
		}
		if (!*first_word)    // "look" alone, without an argument at all
		{
			if (ch->desc) {
				ch->desc->msdp_report("ROOM");
			}
			sight::look_at_room(ch, 1);
		} else if (utils::IsAbbr(first_word, "in") || utils::IsAbbr(first_word, "внутрь"))
			sight::look_in_obj(ch, arg2);
			// did the char type 'look <direction>?'
		else if (((look_type = search_block(first_word, dirs, false)) >= 0) ||
			((look_type = search_block(first_word, dirs_rus, false)) >= 0))
			sight::look_in_direction(ch, look_type, sight::EXIT_SHOW_WALL);
		else if (utils::IsAbbr(first_word, "at") || utils::IsAbbr(first_word, "на"))
			sight::look_at_target(ch, arg2, subcmd);
		else
			sight::look_at_target(ch, argument, subcmd);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
