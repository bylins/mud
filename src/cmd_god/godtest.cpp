#include "godtest.h"

//#include <iostream>

#include "entities/char_data.h"
#include "modify.h"
//#include "game_skills/.h"
//#include "utils/utils_string.h"
#include "structs/global_objects.h"

#include "utils/libfort/fort.hpp"

// This is test command for different testings
void do_godtest(CharData *ch, char */*argument*/, int /* cmd */, int /* subcmd */) {
/*	buffer << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать."
		   << std::endl;
	page_string(ch->desc, buffer.str());
		   */

	fort::char_table table;
	table.set_cell_content_fg_color(fort::color::red);
	table << fort::header
		  << "Ранг" << "Название" << "Год" << "Рейтинг" << fort::endr
		  << "1" << "Побег из Шоушенка" << "1994" << "9.5"<< fort::endr;
/*		  << "2" << "12 разгневанных мужчин" << "1957" << "8.8" << fort::endr
		  << "3" << "Космическая одиссея 2001 года" << "1968" << "8.5" << fort::endr
		  << "4" << "Бегущий по лезвию" << "1982" << "8.1" << fort::endr;*/

/*	table.set_border_style(FT_BASIC_STYLE);
	page_string(ch->desc, table.to_string());
	table.set_border_style(FT_BASIC2_STYLE);
	page_string(ch->desc, table.to_string());
	table.set_border_style(FT_SIMPLE_STYLE);		// годится для служебных таблиц типа users
	page_string(ch->desc, table.to_string());*/

	// годится для магазинов
	table.set_border_style(FT_PLAIN_STYLE);
	table.set_cur_cell(0, 0);
	table.set_cell_content_fg_color(fort::color::default_color);
	page_string(ch->desc, table.to_string());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
