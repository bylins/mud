//
// Created by Sventovit on 08.09.2024.
//

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "gameplay/classes/pc_classes.h"

void do_levels(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int i;
	char *ptr = &buf[0];

	if (ch->IsNpc()) {
		SendMsgToChar("Боги уже придумали ваш уровень.\r\n", ch);
		return;
	}
	*ptr = '\0';

	ptr += sprintf(ptr, "Уровень          Опыт            Макс на урв.\r\n");
	for (i = 1; i < kLvlImmortal; i++) {
		ptr += sprintf(ptr, "%s[%2d] %13s-%-13s %-13s%s\r\n", (GetRealLevel(ch) == i) ? kColorBoldCyn : "", i,
					   thousands_sep(GetExpUntilNextLvl(ch, i)).c_str(),
					   thousands_sep(GetExpUntilNextLvl(ch, i + 1) - 1).c_str(),
					   thousands_sep((int) (GetExpUntilNextLvl(ch, i + 1) - GetExpUntilNextLvl(ch, i)) / (10 + GetRealRemort(ch))).c_str(),
					   (GetRealLevel(ch) == i) ? kColorNrm : "");
	}

	ptr += sprintf(ptr, "%s[%2d] %13s               (БЕССМЕРТИЕ)%s\r\n",
				   (GetRealLevel(ch) >= kLvlImmortal) ? kColorBoldCyn : "", kLvlImmortal,
				   thousands_sep(GetExpUntilNextLvl(ch, kLvlImmortal)).c_str(),
				   (GetRealLevel(ch) >= kLvlImmortal) ? kColorNrm : "");
	page_string(ch->desc, buf, 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
