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
	std:: ostringstream out;
	out << "В настоящий момент процiдурка пуста." << std::endl
		<< "Если вам хочется что-то godtest - придется ее реализовать." << std::endl;
	page_string(ch->desc, out.str());

/*	const char *colors[] = {
		"default_color",
		"black",
		"red",
		"green",
		"yellow",
		"blue",
		"magenta",
		"cyan",
		"light_gray",
		"dark_gray",
		"light_red",
		"light_green",
		"light_yellow",
		"light_blue",
		"light_magenta",
		"light_cyan",
		"light_whyte",
	};

	fort::char_table table_color;
	for (auto & color : colors) {
		table_color
			<< color
			<< 5794504
			<< 456
			<< 777
			<< 422 << fort::endr;
	}

	table_color.row(0).set_cell_content_fg_color(fort::color::default_color);
	table_color.row(1).set_cell_content_fg_color(fort::color::black);
	table_color.row(2).set_cell_content_fg_color(fort::color::red);
	table_color.row(3).set_cell_content_fg_color(fort::color::green);
	table_color.row(4).set_cell_content_fg_color(fort::color::yellow);
	table_color.row(5).set_cell_content_fg_color(fort::color::blue);
	table_color.row(6).set_cell_content_fg_color(fort::color::magenta);
	table_color.row(7).set_cell_content_fg_color(fort::color::cyan);
	table_color.row(8).set_cell_content_fg_color(fort::color::light_gray);
	table_color.row(9).set_cell_content_fg_color(fort::color::dark_gray);
	table_color.row(10).set_cell_content_fg_color(fort::color::light_red);
	table_color.row(11).set_cell_content_fg_color(fort::color::light_green);
	table_color.row(12).set_cell_content_fg_color(fort::color::light_yellow);
	table_color.row(13).set_cell_content_fg_color(fort::color::light_blue);
	table_color.row(14).set_cell_content_fg_color(fort::color::light_magenta);
	table_color.row(15).set_cell_content_fg_color(fort::color::light_cyan);
	table_color.row(16).set_cell_content_fg_color(fort::color::light_whyte);

	table_wrapper::PrintTableToChar(ch, table_color);*/
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

