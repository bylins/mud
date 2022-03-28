//
// Created by Sventovit on 24.02.2022.
//

#include "table_wrapper.h"

#include "entities/char_data.h"

namespace table_wrapper {

const int kDefaultLeftTableMargin = 1;	// Отступ таблицы от левого края окна

void DecorateSimpleTable(CharData *ch, fort::char_table &table) {
	if (GR_FLAGGED(ch, EPrf::kBlindMode)) {
		table.set_border_style(FT_EMPTY2_STYLE);
	} else {
		table.set_border_style(FT_BASIC_STYLE);
		table.set_left_margin(kDefaultLeftTableMargin);
	}
}

void DecorateNoBorderTable(CharData *ch, fort::char_table &table) {
	if (GR_FLAGGED(ch, EPrf::kBlindMode)) {
		table.set_border_style(FT_EMPTY2_STYLE);
	} else {
		table.set_border_style(FT_EMPTY_STYLE);
		table.set_left_margin(kDefaultLeftTableMargin);
	}
}

void DecorateServiceTable(CharData *ch, fort::char_table &table) {
	if (GR_FLAGGED(ch, EPrf::kBlindMode)) {
		table.set_border_style(FT_EMPTY2_STYLE);
	} else {
		table.set_border_style(FT_PLAIN_STYLE);
		for (unsigned i = 1; i < table.row_count(); ++i) {
			if ((i + 1) % 2 == 0) {
				table.row(i).set_cell_bg_color(fort::color::blue);
			}
		}
		table.column(table.col_count() - 1).set_cell_text_align(fort::text_align::right);
	}
}

void DecorateCuteTable(CharData *ch, fort::char_table &table) {
	if (GR_FLAGGED(ch, EPrf::kBlindMode)) {
		table.set_border_style(FT_EMPTY2_STYLE);
	} else {
		table.set_border_style(FT_BASIC_STYLE);
		unsigned k;
		for (unsigned i = 0; i < table.col_count(); ++i) {
			k = i + 1;
			if (i == 0 || k % 10 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::light_whyte);
			} else if (k % 9 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::light_red);
			} else if (k % 8 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::red);
			} else if (k % 7 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::light_yellow);
			} else if (k % 6 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::yellow);
			} else if (k % 5 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::light_green);
			} else if (k % 4 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::green);
			} else if (k % 3 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::light_cyan);
			} else if (k % 2 == 0) {
				table.column(i).set_cell_content_fg_color(fort::color::cyan);
			}

			if (i > 1 && i % 2 == 0) {
				table.column(i).set_cell_text_align(fort::text_align::right);
			}
		}
		table.set_left_margin(kDefaultLeftTableMargin);
	}
}

void DecorateZebraTable(CharData *ch, fort::char_table &table, fort::color color) {
	if (GR_FLAGGED(ch, EPrf::kBlindMode)) {
		table.set_border_style(FT_EMPTY2_STYLE);
	} else {
		table.set_border_style(FT_SIMPLE_STYLE);
		for (unsigned i = 1; i < table.row_count(); ++i) {
			if ((i + 1) % 2 == 0) {
				table.row(i).set_cell_bg_color(color);
			}
		}
		table.set_left_margin(kDefaultLeftTableMargin);
	}
}

void DecorateZebraTextTable(CharData *ch, fort::char_table &table, fort::color color) {
	if (GR_FLAGGED(ch, EPrf::kBlindMode)) {
		table.set_border_style(FT_EMPTY2_STYLE);
	} else {
		table.set_border_style(FT_SIMPLE_STYLE);
		for (unsigned i = 1; i < table.row_count(); ++i) {
			if ((i + 1) % 2 == 0) {
				table.row(i).set_cell_content_fg_color(color);
			}
		}
		table.set_left_margin(kDefaultLeftTableMargin);
	}
}

void PrintTableToChar(CharData *ch, fort::char_table &table) {
	try {
		send_to_char(table.to_string(), ch);
	} catch (std::runtime_error &e) {
		send_to_char(e.what(), ch);
	}
}

void PrintTableToStream(std::ostringstream &out, fort::char_table &table) {
	try {
		out << table.to_string();
	} catch (std::runtime_error &e) {
		out << e.what();
	}
};

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
