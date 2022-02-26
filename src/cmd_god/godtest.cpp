#include "godtest.h"

#include <iostream>

#include "entities/char_data.h"
//#include "modify.h"
//#include "game_skills/.h"
//#include "utils/utils_string.h"
#include "structs/global_objects.h"
#include "utils/table_wrapper.h"
#include "utils/utils.h"
#include "color.h"
#include "entities/player_races.h"

// This is test command for different testings
void do_godtest(CharData *ch, char */*argument*/, int /* cmd */, int /* subcmd */) {
/*	std:: ostringstream out;
	out << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать."
		   << std::endl;
	page_string(ch->desc, out.str());*/

	std::ostringstream out;
	out << "  Вы " << ch->get_name() << ", " << MUD::Classes()[ch->get_class()].GetName() << ". Ваши показатели:" << std::endl;

	fort::char_table table;
	// Пишем первый столбец
	int col{0};
	int row{-1};
	table[++row][col] = std::string("Имя: ") + ch->get_name();
	table[++row][col] = std::string("Род: ") + PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), GET_SEX(ch));
	table[++row][col] = std::string("Вера: ") + religion_name[GET_RELIGION(ch)][static_cast<int>(GET_SEX(ch))];
	table[++row][col] = std::string("Уровень: ") + std::to_string(GetRealLevel(ch));
	table[++row][col] = std::string("Перевоплощений: ") + std::to_string(GET_REAL_REMORT(ch));
	table[++row][col] = std::string("Возраст: ") + std::to_string(GET_AGE(ch));
	table[++row][col] = std::string("Опыт: ") + PrintNumberByDigits(GET_EXP(ch));
	table[++row][col] = std::string("ДСУ: ") + PrintNumberByDigits(level_exp(ch, GetRealLevel(ch) + 1) - GET_EXP(ch));
	table[++row][col] = std::string("Кун: ") + std::to_string(ch->get_gold());
	table[++row][col] = std::string("На счету: ") + std::to_string(ch->get_bank());
	table[++row][col] = std::string("Положение: ", "стоит"); // Тут нужно функцию для получения позиции в текстовом виде
	table[++row][col] = std::string("Голоден: ") + (GET_COND(ch, FULL) > NORM_COND_VALUE ? "Угу :(" : "Нет");
	table[++row][col] = std::string("Жажда: ") + (GET_COND_M(ch, THIRST) ? "Наливай!" : "Нет");

	// Пишем опциональные пункты первого столбца
	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED) {
		table[++row][col] = (affected_by_spell(ch, kSpellAbstinent) ? "Похмелье." : "Вы пьяны.");
	}

	// Пишем второй и третий столбцы
	col += 1; // Тут +1, потому что писали только один столбец. Далее должно быть +2
	row = -1;
	table[++row][col] = "Рост";				table[row][col + 1] = std::to_string(GET_HEIGHT(ch)) + " (" + std::to_string(GET_REAL_HEIGHT(ch)) + ")";
	table[++row][col] = "Вес";				table[row][col + 1] = std::to_string(GET_WEIGHT(ch)) + " (" + std::to_string(GET_REAL_WEIGHT(ch)) + ")";
	table[++row][col] = "Размер";			table[row][col + 1] = std::to_string(GET_SIZE(ch)) + " (" + std::to_string(GET_REAL_SIZE(ch)) + ")";
	table[++row][col] = "Сила";				table[row][col + 1] = std::to_string(ch->get_str()) + " (" + std::to_string(GET_REAL_STR(ch)) + ")";
	table[++row][col] = "Ловкость";			table[row][col + 1] = std::to_string(ch->get_dex()) + " (" + std::to_string(GET_REAL_DEX(ch)) + ")";
	table[++row][col] = "Телосложение";		table[row][col + 1] = std::to_string(ch->get_con()) + " (" + std::to_string(GET_REAL_CON(ch)) + ")";
	table[++row][col] = "Мудрость";			table[row][col + 1] = std::to_string(ch->get_wis()) + " (" + std::to_string(GET_REAL_WIS(ch)) + ")";
	table[++row][col] = "Интеллект";		table[row][col + 1] = std::to_string(ch->get_int()) + " (" + std::to_string(GET_REAL_INT(ch)) + ")";
	table[++row][col] = "Обаяние";			table[row][col + 1] = std::to_string(ch->get_cha()) + " (" + std::to_string(GET_REAL_CHA(ch)) + ")";
	table[++row][col] = "Жизнь";			table[row][col + 1] = std::to_string(GET_HIT(ch)) + "(" + std::to_string(GET_REAL_MAX_HIT(ch)) + ")";
	table[++row][col] = "Восст. жизни";		table[row][col + 1] = "+" + std::to_string(GET_HITREG(ch)) + "% (" + std::to_string(hit_gain(ch)) + ")";
	table[++row][col] = "Выносливость";		table[row][col + 1] = std::to_string(GET_MOVE(ch)) + "(" + std::to_string(GET_REAL_MAX_MOVE(ch)) + ")";
	table[++row][col] = "Восст. сил";		table[row][col + 1] = "+" + std::to_string(GET_MOVEREG(ch)) + "% (" + std::to_string(move_gain(ch)) + ")";

	// Пишем опциональные пункты второго столбца
	if (IS_MANA_CASTER(ch)) {
		table[++row][col] = "Мана";			table[row][col + 1] = std::to_string(GET_MANA_STORED(ch)) + "(" + std::to_string(GET_MAX_MANA(ch)) + ")";
		table[++row][col] = "Восст. маны";	table[row][col + 1] = "+" + std::to_string(mana_gain(ch)) + " сек.";
	}

	// Подготовка данных для пятого и шестого слобцов
	//HitData hit_params;
	//hit_params.weapon = fight::kMainHand;
	//hit_params.init(ch, ch);
	//bool need_dice = false;
	//auto max_dam = hit_params.calc_damage(ch, false);

	// Пишем четвертый и пятый столбцы
	col += 2;
	row = -1;
	table[++row][col] = "Броня";		table[row][col + 1] = std::to_string(GET_ARMOUR(ch));
	table[++row][col] = "Защита";		table[row][col + 1] = std::to_string(GET_AC(ch));
	table[++row][col] = "Поглощение";	table[row][col + 1] = std::to_string(GET_ABSORBE(ch));
	table[++row][col] = "Атака";		table[row][col + 1] = std::to_string(465); 	// calc_hr_info(ch)
	table[++row][col] = "Урон";			table[row][col + 1] = std::to_string(465); 	// max_dam
	table[++row][col] = "Колдовство";	table[row][col + 1] = std::to_string(GET_CAST_SUCCESS(ch)*ch->get_cond_penalty(P_CAST));
	table[++row][col] = "Запоминание";	table[row][col + 1] = std::to_string(std::ceil(GET_MANAREG(ch)*ch->get_cond_penalty(P_CAST)));
	table[++row][col] = "Удача";		table[row][col + 1] = std::to_string(ch->calc_morale());
	table[++row][col] = "Инициатива";	table[row][col + 1] = std::to_string(15); //std::to_string(calc_initiative(ch, false));
	table[++row][col] = "Спас-броски:";	table[row][col + 1] = " ";
	table[++row][col] = "Воля";			table[row][col + 1] = std::to_string(GET_REAL_SAVING_WILL(ch));
	table[++row][col] = "Здоровье";		table[row][col + 1] = std::to_string(GET_REAL_SAVING_CRITICAL(ch));
	table[++row][col] = "Стойкость";	table[row][col + 1] = std::to_string(GET_REAL_SAVING_STABILITY(ch));
	table[++row][col] = "Реакция";		table[row][col + 1] = std::to_string(GET_REAL_SAVING_REFLEX(ch));

	// Пишем шестой и седьмой столбцы
	col += 2;
	row = -1;
	table[++row][col] = "Сопротивления: ";		table[row][col + 1] = " ";
	table[++row][col] = "Урону: ";				table[row][col + 1] = std::to_string(GET_MR(ch));
	table[++row][col] = "Магии: ";				table[row][col + 1] = std::to_string(GET_PR(ch));
	table[++row][col] = "Огню: ";				table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, FIRE_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Воздуху: ";			table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, AIR_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Воде: ";				table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, WATER_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Земле: ";				table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EARTH_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Тьме: ";				table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, DARK_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Тяжелым ранам: ";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, VITALITY_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Безумию: ";			table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, MIND_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Ядам и болезням: ";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, IMMUNITY_RESISTANCE), kMaxPlayerResist));

	table_wrapper::DecorateCuteTable(ch, table);
	table_wrapper::PrintTableToStream(out, table);

	//PlayerSystem::weight_dex_penalty(ch) ? out << KICYN << " * " << KNRM << KIYEL << "Вы перегружены!" << KNRM << std::endl : "";
	out << KICYN << " * " << KIGRN << "Вы ужас, летящий на картоных крыльях." << std::endl;
	out << KICYN << " * " << KNRM << "Дворецкий задрался чистить вашу надранную бронезадницу." << std::endl;
	// Не забыть добавить остальные строки

	send_to_char(out.str(), ch);

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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*	fort::char_table table;
	// Пишем первый и второй столбец
	int col{0};
	int row{-1};
	table[++row][col] = "Имя";
	table[++row][col] = "Род";
	table[++row][col] = "Вера";
	table[++row][col] = "Уровень";
	table[++row][col] = "Перевоплощений";
	table[++row][col] = "Возраст";
	table[++row][col] = "Опыт";
	table[++row][col] = "ДСУ";
	table[++row][col] = "Кун";
	table[++row][col] = "На счету";
	table[++row][col] = "Положение";
	table[++row][col] = "Голоден";
	table[++row][col] = "Жажда";

	// Пишем третий и четвертый столбец
	col += 1;
	row = -1;
	table[++row][col] = "Рост";				table[row][col + 1] = std::to_string(134) + "(" + std::to_string(145) + ")";
	table[++row][col] = "Вес";				table[row][col + 1] = std::to_string(124) + "(" + std::to_string(125) + ")";
	table[++row][col] = "Размер";			table[row][col + 1] = std::to_string(50) + "(" + std::to_string(50) + ")";
	table[++row][col] = "Сила";				table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Ловкость";			table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Телосложение";		table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Мудрость";			table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Интеллект";		table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Обаяние";			table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Жизнь";			table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Восст. жизни";		table[row][col + 1] = "+" + std::to_string(35) + "% (" + std::to_string(55) + ")";
	table[++row][col] = "Выносливость";		table[row][col + 1] = std::to_string(35) + "(" + std::to_string(55) + ")";
	table[++row][col] = "Восст. сил";		table[row][col + 1] = "+" + std::to_string(35) + "% (" + std::to_string(55) + ")";

	// Пишем пятый и шестой столбец
	col += 2;
	row = -1;
	table[++row][col] = "Броня";		table[row][col + 1] = std::to_string(34);
	table[++row][col] = "Защита";		table[row][col + 1] = std::to_string(-4);
	table[++row][col] = "Атака";		table[row][col + 1] = std::to_string(465);
	table[++row][col] = "Колдовство";	table[row][col + 1] = std::to_string(350);
	table[++row][col] = "Запоминание";	table[row][col + 1] = std::to_string(125);
	table[++row][col] = "Удача";		table[row][col + 1] = std::to_string(15);
	table[++row][col] = "Инициатива";	table[row][col + 1] = std::to_string(15);
	table[++row][col] = "Спас-броски:";	table[row][col + 1] = " ";
	table[++row][col] = "Воля";			table[row][col + 1] = std::to_string(15);
	table[++row][col] = "Здоровье";		table[row][col + 1] = std::to_string(15);
	table[++row][col] = "Стойкость";	table[row][col + 1] = std::to_string(15);
	table[++row][col] = "Реакция";		table[row][col + 1] = std::to_string(15);

	// Пишем седьмой и восьмой столбец
	col += 2;
	row = -1;
	table[++row][col] = "Поглощение";		table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Сопротивления:";	table[row][col + 1] = " ";
	table[++row][col] = "Урону";			table[row][col + 1] = std::to_string(35);
	table[++row][col] = "Магии";			table[row][col + 1] = std::to_string(35);
	table[++row][col] = "Огню";				table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Воздуху";			table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Воде";				table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Земле";			table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Тьме";				table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Тяжелым ранам";			table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Безумию";			table[row][col + 1] = std::to_string(30);
	table[++row][col] = "Ядам"		;		table[row][col + 1] = std::to_string(30);

	table_wrapper::DecorateCuteTable(ch, table);

	std::ostringstream out;
	table_wrapper::PrintTableToStream(out, table);
	out << KICYN << " * " << KNRM << "У вас есть бэтмобиль." << std::endl;
	out << KICYN << " * " << KIGRN << "Вы ужас, летящий на картоных крыльях." << std::endl;
	out << KICYN << " * " << KNRM << "Дворецкий задрался чистить вашу надранную бронезадницу." << std::endl;

	send_to_char(out.str(), ch);*/

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

