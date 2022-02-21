#include "godtest.h"

#include <iostream>

#include "entities/char_data.h"
#include "modify.h"
//#include "skills.h"
//#include "utils/utils_string.h"

#include "structs/global_objects.h"

// This is test command for different testings
void do_godtest(CharData *ch, char */*argument*/, int /* cmd */, int /* subcmd */) {
	std::stringstream buffer;
	MUD::Classes()[ECharClass::kWarrior].Print(buffer);
/*	buffer << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать."
		   << std::endl;*/
/*	std::string buffer = argument;
	page_string(ch->desc, buffer);
	utils::trim(buffer);
	page_string(ch->desc, buffer);*/
	page_string(ch->desc, buffer.str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
