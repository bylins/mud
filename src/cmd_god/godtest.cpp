#include "godtest.h"

#include <iostream>

#include "entities/char.h"
#include "modify.h"
//#include "skills.h"

#include "structs/global_objects.h"

// This is test command for different testings
void do_godtest(CharacterData *ch, char */*argument*/, int /* cmd */, int /* subcmd */) {

/*	std::stringstream buffer;
	buffer << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать."
		   << std::endl;
	page_string(ch->desc, buffer.str());*/

	std::stringstream buffer;
	buffer << "Список доступных умений:" << "\n";
	for (const auto &it : MUD::Skills()) {
		if (it.IsUnavailable()) {
			continue;
		}
		buffer << it.name << "\n";
	}
	page_string(ch->desc, buffer.str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
