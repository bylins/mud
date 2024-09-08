//
// Created by Sventovit on 08.09.2024.
//

#include "entities/char_data.h"
#include "color.h"
#include "modify.h"

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
		ptr += sprintf(ptr, "%s[%2d] %13s-%-13s %-13s%s\r\n", (GetRealLevel(ch) == i) ? CCICYN(ch, C_NRM) : "", i,
					   thousands_sep(GetExpUntilNextLvl(ch, i)).c_str(),
					   thousands_sep(GetExpUntilNextLvl(ch, i + 1) - 1).c_str(),
					   thousands_sep((int) (GetExpUntilNextLvl(ch, i + 1) - GetExpUntilNextLvl(ch, i)) / (10 + GetRealRemort(ch))).c_str(),
					   (GetRealLevel(ch) == i) ? CCNRM(ch, C_NRM) : "");
	}

	ptr += sprintf(ptr, "%s[%2d] %13s               (БЕССМЕРТИЕ)%s\r\n",
				   (GetRealLevel(ch) >= kLvlImmortal) ? CCICYN(ch, C_NRM) : "", kLvlImmortal,
				   thousands_sep(GetExpUntilNextLvl(ch, kLvlImmortal)).c_str(),
				   (GetRealLevel(ch) >= kLvlImmortal) ? CCNRM(ch, C_NRM) : "");
	page_string(ch->desc, buf, 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
