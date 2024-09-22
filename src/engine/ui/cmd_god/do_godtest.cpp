#include "do_godtest.h"

#include <iostream>

#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"
//#include "gameplay/skills/.h"
//#include "utils/utils_string.h"
#include "engine/db/global_objects.h"
/*#include "utils/table_wrapper.h"
#include "utils/utils.h"
#include "engine/ui/color.h"
#include "engine/entities/player_races.h"
#include "gameplay/classes/classes.h"*/

// It is test command for different testings
void do_godtest(CharData *ch, char */*argument*/, int /* cmd */, int /* subcmd */) {
	std:: ostringstream out;
	out << "В настоящий момент процiдурка пуста." << "\r\n"
		<< "Если вам хочется что-то godtest - придется ее реализовать." << "\r\n";
	page_string(ch->desc, out.str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
