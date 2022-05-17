#include "godtest.h"

#include <iostream>

#include "entities/char_data.h"
#include "modify.h"
//#include "game_skills/.h"
//#include "utils/utils_string.h"
#include "structs/global_objects.h"
/*#include "utils/table_wrapper.h"
#include "utils/utils.h"
#include "color.h"
#include "entities/player_races.h"
#include "game_classes/classes.h"*/

// This is test command for different testings
void do_godtest(CharData *ch, char */*argument*/, int /* cmd */, int /* subcmd */) {
	std::ostringstream out;
	MUD::Classes(ECharClass::kPaladine).Print(ch, out);
	page_string(ch->desc, out.str());

/*	std:: ostringstream out;
	out << "В настоящий момент процiдурка пуста." << std::endl
		<< "Если вам хочется что-то godtest - придется ее реализовать." << std::endl;
	page_string(follower->desc, out.str());*/
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

