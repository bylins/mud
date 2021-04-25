#include "godtest.h"

#include "../chars/char.hpp"
#include <sstream>
#include <iomanip>
#include "../modify.h"

#include "../magic.utils.hpp"

// This is test command for different testings
void do_godtest(CHAR_DATA *ch, char* /*argument*/, int /* cmd */, int /* subcmd */) {
	//send_to_char("В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest, придется ее реализовать.\r\n", ch);
	std::stringstream buffer;
	int required_level = 0;
	for (int i = 0; i < 31; i++) {
		required_level -= MIN(8, MAX(0, ((i - 8)/3)*2 + (i > 7 && i < 11 ? 1 : 0)));
		buffer <<  "Remort: " << i << "   Spell create bonus: " << required_level << std::endl;
		required_level = 0;
	}
	page_string(ch->desc, buffer.str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
