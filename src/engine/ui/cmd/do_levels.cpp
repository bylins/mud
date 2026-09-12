//
// Created by Sventovit on 08.09.2024.
//

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "gameplay/core/experience.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/remort.h"

void do_levels(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int i;

	if (ch->IsNpc()) {
		SendMsgToChar("Боги уже придумали ваш уровень.\r\n", ch);
		return;
	}

	std::string out = "Уровень          Опыт            Макс на урв.\r\n";
	for (i = 1; i < kLvlImmortal; i++) {
		out += fmt::format("{}[{:2}] {:>13}-{:<13} {:<13}{}\r\n",
						   (GetRealLevel(ch) == i) ? kColorBoldCyn : "", i,
						   thousands_sep(experience::GetExpUntilNextLvl(ch, i)),
						   thousands_sep(experience::GetExpUntilNextLvl(ch, i + 1) - 1),
						   thousands_sep((int) (experience::GetExpUntilNextLvl(ch, i + 1) - experience::GetExpUntilNextLvl(ch, i)) / (10 + remort::GetRealRemort(ch))),
						   (GetRealLevel(ch) == i) ? kColorNrm : "");
	}

	out += fmt::format("{}[{:2}] {:>13}               (БЕССМЕРТИЕ){}\r\n",
					   (GetRealLevel(ch) >= kLvlImmortal) ? kColorBoldCyn : "", kLvlImmortal,
					   thousands_sep(experience::GetExpUntilNextLvl(ch, kLvlImmortal)),
					   (GetRealLevel(ch) >= kLvlImmortal) ? kColorNrm : "");
	page_string(ch->desc, out);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
