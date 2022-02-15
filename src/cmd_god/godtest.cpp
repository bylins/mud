#include "godtest.h"

#include <iostream>

#include "entities/char.h"
#include "modify.h"
//#include "skills.h"

#include "structs/global_objects.h"

// This is test command for different testings
void do_godtest(CharacterData *ch, char *argument, int /* cmd */, int /* subcmd */) {
/*	one_argument(argument, arg);
	if (!*arg) {
		send_to_char("Укажите имя скилла.\r\n", ch);
		return;
	}

	ESkill id;
	for (id = ESkill::kFirst; id <= ESkill::kLast; ++id) {
		if (MUD::NewSkills()[id].GetName() == arg) {
			break;
		}
	}
	if (id > ESkill::kLast) {
		return;
	}*/

	std::stringstream buffer;
	MUD::NewSkills()[ESkill::kLongBlades].Print(buffer);
	page_string(ch->desc, buffer.str());

/*	buffer << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать."
		   << std::endl;
	page_string(ch->desc, buffer.str());*/

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
