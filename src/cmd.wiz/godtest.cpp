#include "godtest.h"

#include "../chars/character.h"
#include "../modify.h"

#include <sstream>
#include <iomanip>

// This is test command for different testings
void do_godtest(CHAR_DATA *ch, char* /*argument*/, int /* cmd */, int /* subcmd */) {
	std::stringstream buffer;
	buffer << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать." << std::endl;
	page_string(ch->desc, buffer.str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
