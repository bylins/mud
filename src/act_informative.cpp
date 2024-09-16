/* ************************************************************************
*   File: act.informative.cpp                           Part of Bylins    *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "engine/db/world_characters.h"
#include "engine/entities/entities_constants.h"
#include "utils/logger.h"
#include "gameplay/communication/social.h"
#include "engine/ui/cmd_god/shutdown_parameters.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/ui/interpreter.h"
#include "engine/core/handler.h"
#include "engine/db/db.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/ui/color.h"
#include "gameplay/core/constants.h"
#include "gameplay/fight/pk.h"
#include "gameplay/communication/mail.h"
#include "gameplay/crafting/im.h"
#include "gameplay/clans/house.h"
#include "administration/privilege.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/ui/modify.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/statistics/mob_stat.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "gameplay/classes/classes_constants.h"

//#include <third_party_libs/fmt/include/fmt/format.h>

#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

using std::string;

// extern variables
extern int number_of_social_commands;
extern char *credits;
extern char *info;
extern char *motd;
extern char *rules;
extern char *immlist;
extern char *policies;
extern char *handbook;

extern void show_code_date(CharData *ch);
extern int nameserver_is_slow; //config.cpp

void do_gold(CharData *ch, char *argument, int cmd, int subcmd);
void DoScore(CharData *ch, char *argument, int, int);
void do_time(CharData *ch, char *argument, int cmd, int subcmd);
void do_weather(CharData *ch, char *argument, int cmd, int subcmd);
void do_who(CharData *ch, char *argument, int cmd, int subcmd);
void do_gen_ps(CharData *ch, char *argument, int cmd, int subcmd);
void perform_immort_where(CharData *ch, char *arg);
void do_diagnose(CharData *ch, char *argument, int cmd, int subcmd);
void SortCommands();
void do_commands(CharData *ch, char *argument, int cmd, int subcmd);
void do_quest(CharData *ch, char *argument, int cmd, int subcmd);
void do_check(CharData *ch, char *argument, int cmd, int subcmd);
void diag_char_to_char(CharData *i, CharData *ch);
void look_in_direction(CharData *ch, int dir, int info_is);
void look_in_obj(CharData *ch, char *arg);
char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);
void gods_day_now(CharData *ch);

void do_quest(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {

	SendMsgToChar("У Вас нет никаких ежедневных поручений.\r\nЧтобы взять новые, наберите &Wпоручения получить&n.\r\n",
				 ch);
}

void do_check(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (!login_change_invoice(ch))
		SendMsgToChar("Проверка показала: новых сообщений нет.\r\n", ch);
}

// для использования с чарами:
// возвращает метки предмета, если они есть и смотрящий является их автором или является членом соотв. клана
std::string char_get_custom_label(ObjData *obj, CharData *ch) {
	const char *delim_l = nullptr;
	const char *delim_r = nullptr;

	// разные скобки для клановых и личных
	if (obj->get_custom_label() && (ch->player_specials->clan && obj->get_custom_label()->clan_abbrev != nullptr &&
		is_alliance_by_abbr(ch, obj->get_custom_label()->clan_abbrev))) {
		delim_l = " *";
		delim_r = "*";
	} else {
		delim_l = " (";
		delim_r = ")";
	}

	std::stringstream buffer;
	if (AUTH_CUSTOM_LABEL(obj, ch)) {
		buffer << delim_l << obj->get_custom_label()->text_label << delim_r;
	}

	return buffer.str();
}

void do_gold(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_gold() == 0)
		SendMsgToChar("Вы разорены!\r\n", ch);
	else if (ch->get_gold() == 1)
		SendMsgToChar("У вас есть всего лишь одна куна.\r\n", ch);
	else {
		sprintf(buf, "У Вас есть %ld %s.\r\n", ch->get_gold(), GetDeclensionInNumber(ch->get_gold(), EWhat::kMoneyA));
		SendMsgToChar(buf, ch);
	}
}

void ClearMyStat(CharData *ch) {
	GET_RIP_MOBTHIS(ch) = GET_EXP_MOBTHIS(ch) = GET_RIP_MOB(ch) = GET_EXP_MOB(ch) =
	GET_RIP_PKTHIS(ch) = GET_EXP_PKTHIS(ch) = GET_RIP_PK(ch) = GET_EXP_PK(ch) =
	GET_RIP_DTTHIS(ch) = GET_EXP_DTTHIS (ch) = GET_RIP_DT(ch) = GET_EXP_DT(ch) =
	GET_RIP_OTHERTHIS(ch) = GET_EXP_OTHERTHIS(ch) = GET_RIP_OTHER(ch) = GET_EXP_OTHER(ch) =
	GET_WIN_ARENA(ch) = GET_RIP_ARENA(ch) = GET_EXP_ARENA(ch) = 0;
	SendMsgToChar("Статистика очищена.\r\n", ch);
}

void PrintMyStat(CharData *ch) {
	table_wrapper::Table table;
	std::size_t row{0};
	std::size_t col{0};

	table << table_wrapper::kHeader;
	table[row][col]	= "Статистика ваших смертей\r\n(количество, потерянного опыта)";
	table[row][++col]	= "Текущее\r\nперевоплощение:";
	table[row][++col]	= "\r\nВсего:";

	col = 0;
	table[++row][col] = "В неравном бою с тварями:";
	table[row][++col] = std::to_string(GET_RIP_MOBTHIS(ch)) + " (" + PrintNumberByDigits(GET_EXP_MOBTHIS(ch)) + ")";
	table[row][++col] = std::to_string(GET_RIP_MOB(ch)) + " (" + PrintNumberByDigits(GET_EXP_MOB(ch)) + ")";

	col = 0;
	table[++row][col] = "В неравном бою с врагами:";
	table[row][++col] = std::to_string(GET_RIP_PKTHIS(ch)) + " (" + PrintNumberByDigits(GET_EXP_PKTHIS(ch)) + ")";
	table[row][++col] = std::to_string(GET_RIP_PK(ch)) + " (" + PrintNumberByDigits(GET_EXP_PK(ch)) + ")";

	col = 0;
	table[++row][col] = "В гиблых местах:";
	table[row][++col]	= std::to_string(GET_RIP_DTTHIS(ch)) + " (" + PrintNumberByDigits(GET_EXP_DTTHIS (ch))	+ ")";
	table[row][++col] = std::to_string(GET_RIP_DT(ch)) + " (" + PrintNumberByDigits(GET_EXP_DT(ch)) + ")";

	col = 0;
	table[++row][col] = "По стечению обстоятельств:";
	table[row][++col]	= std::to_string(GET_RIP_OTHERTHIS(ch)) + " ("	+ PrintNumberByDigits(GET_EXP_OTHERTHIS(ch)) + ")";
	table[row][++col]	= std::to_string(GET_RIP_OTHER(ch)) + " (" + PrintNumberByDigits(GET_EXP_OTHER(ch)) + ")";

	col = 0;
	table[++row][col] = "ИТОГО:";
	table[row][++col]	= std::to_string(GET_RIP_MOBTHIS(ch) + GET_RIP_PKTHIS(ch) + GET_RIP_DTTHIS(ch)
		+ GET_RIP_OTHERTHIS(ch)) + " (" + PrintNumberByDigits(GET_EXP_MOBTHIS(ch) + GET_EXP_PKTHIS(ch)
		+ GET_EXP_DTTHIS(ch) + GET_EXP_OTHERTHIS(ch) + GET_EXP_ARENA(ch)) + ")";
	table[row][++col] = std::to_string(GET_RIP_MOB(ch) + GET_RIP_PK(ch) + GET_RIP_DT(ch) + GET_RIP_OTHER(ch))
		+ " (" + PrintNumberByDigits(GET_EXP_MOB(ch) + GET_EXP_PK(ch) + GET_EXP_DT(ch) + GET_EXP_OTHER(ch)
		+ GET_EXP_ARENA(ch)) +")";
	table << table_wrapper::kEndRow << table_wrapper::kSeparator << table_wrapper::kEndRow;

	col = 0;
	table[++row][col] = "На арене:";
	table[row][++col] = " ";
	table[row][++col] = " ";

	col = 0;
	table[++row][col] = "Убито игроков: " + std::to_string(GET_WIN_ARENA(ch));
	table[row][++col] = "Смертей: " + std::to_string(GET_RIP_ARENA(ch));
	table[row][++col] = "Потеряно опыта: " + std::to_string(GET_EXP_ARENA(ch));
	table << table_wrapper::kEndRow << table_wrapper::kSeparator << table_wrapper::kEndRow;

	col = 0;
	table[++row][col] = "Арена доминирования:";
	table[row][++col] = "Убито: " + std::to_string(ch->player_specials->saved.kill_arena_dom);
	table[row][++col] = "Смерти: " + std::to_string(ch->player_specials->saved.rip_arena_dom);

	table_wrapper::DecorateZebraTextTable(ch, table, table_wrapper::color::kLightCyan);
	table_wrapper::PrintTableToChar(ch, table);
}

// Отображение количества рипов
void do_mystat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);
	if (utils::IsAbbr(argument, "очистить") || utils::IsAbbr(argument, "clear")) {
		ClearMyStat(ch);
	} else {
		PrintMyStat(ch);
	}
}

void do_time(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int day, month, days_go;
	if (ch->IsNpc())
		return;
	sprintf(buf, "Сейчас ");
	switch (time_info.hours % 24) {
		case 0: sprintf(buf + strlen(buf), "полночь, ");
			break;
		case 1: sprintf(buf + strlen(buf), "1 час ночи, ");
			break;
		case 2:
		case 3:
		case 4: sprintf(buf + strlen(buf), "%d часа ночи, ", time_info.hours);
			break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11: sprintf(buf + strlen(buf), "%d часов утра, ", time_info.hours);
			break;
		case 12: sprintf(buf + strlen(buf), "полдень, ");
			break;
		case 13: sprintf(buf + strlen(buf), "1 час пополудни, ");
			break;
		case 14:
		case 15:
		case 16: sprintf(buf + strlen(buf), "%d часа пополудни, ", time_info.hours - 12);
			break;
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23: sprintf(buf + strlen(buf), "%d часов вечера, ", time_info.hours - 12);
			break;
	}

	if (GET_RELIGION(ch) == kReligionPoly)
		strcat(buf, weekdays_poly[weather_info.week_day_poly]);
	else
		strcat(buf, weekdays[weather_info.week_day_mono]);
	switch (weather_info.sunlight) {
		case kSunDark: strcat(buf, ", ночь");
			break;
		case kSunSet: strcat(buf, ", закат");
			break;
		case kSunLight: strcat(buf, ", день");
			break;
		case kSunRise: strcat(buf, ", рассвет");
			break;
	}
	strcat(buf, ".\r\n");
	SendMsgToChar(buf, ch);

	day = time_info.day + 1;    // day in [1..30]
	*buf = '\0';
	if (GET_RELIGION(ch) == kReligionPoly || IS_IMMORTAL(ch)) {
		days_go = time_info.month * kDaysPerMonth + time_info.day;
		month = days_go / 40;
		days_go = (days_go % 40) + 1;
		sprintf(buf + strlen(buf), "%s, %dй День, Год %d%s",
				month_name_poly[month], days_go, time_info.year, IS_IMMORTAL(ch) ? ".\r\n" : "");
	}
	if (GET_RELIGION(ch) == kReligionMono || IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), "%s, %dй День, Год %d",
				month_name[static_cast<int>(time_info.month)], day, time_info.year);
	if (IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf),
				"\r\n%d.%d.%d, дней с начала года: %d",
				day,
				time_info.month + 1,
				time_info.year,
				(time_info.month * kDaysPerMonth) + day);
	switch (weather_info.season) {
		case ESeason::kWinter: strcat(buf, ", зима");
			break;
		case ESeason::kSpring: strcat(buf, ", весна");
			break;
		case ESeason::kSummer: strcat(buf, ", лето");
			break;
		case ESeason::kAutumn: strcat(buf, ", осень");
			break;
	}
	strcat(buf, ".\r\n");
	SendMsgToChar(buf, ch);
	gods_day_now(ch);
}

void do_weather(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int sky = weather_info.sky, weather_type = weather_info.weather_type;
	const char *sky_look[] = {"облачное",
							  "пасмурное",
							  "покрыто тяжелыми тучами",
							  "ясное"
	};
	const char *moon_look[] = {"Новолуние.",
							   "Растущий серп луны.",
							   "Растущая луна.",
							   "Полнолуние.",
							   "Убывающая луна.",
							   "Убывающий серп луны."
	};
	if (OUTSIDE(ch)) {
		*buf = '\0';
		if (world[ch->in_room]->weather.duration > 0) {
			sky = world[ch->in_room]->weather.sky;
			weather_type = world[ch->in_room]->weather.weather_type;
		}
		sprintf(buf + strlen(buf),
				"Небо %s. %s\r\n%s\r\n", sky_look[sky],
				get_moon(sky) ? moon_look[get_moon(sky) - 1] : "",
				(weather_info.change >=
					0 ? "Атмосферное давление повышается." : "Атмосферное давление понижается."));
		sprintf(buf + strlen(buf), "На дворе %d %s.\r\n",
				weather_info.temperature, GetDeclensionInNumber(weather_info.temperature, EWhat::kDegree));

		if (IS_SET(weather_info.weather_type, kWeatherBigwind))
			strcat(buf, "Сильный ветер.\r\n");
		else if (IS_SET(weather_info.weather_type, kWeatherMediumwind))
			strcat(buf, "Умеренный ветер.\r\n");
		else if (IS_SET(weather_info.weather_type, kWeatherLightwind))
			strcat(buf, "Легкий ветерок.\r\n");

		if (IS_SET(weather_type, kWeatherBigsnow))
			strcat(buf, "Валит снег.\r\n");
		else if (IS_SET(weather_type, kWeatherMediumsnow))
			strcat(buf, "Снегопад.\r\n");
		else if (IS_SET(weather_type, kWeatherLightsnow))
			strcat(buf, "Легкий снежок.\r\n");

		if (IS_SET(weather_type, kWeatherHail))
			strcat(buf, "Дождь с градом.\r\n");
		else if (IS_SET(weather_type, kWeatherBigrain))
			strcat(buf, "Льет, как из ведра.\r\n");
		else if (IS_SET(weather_type, kWeatherMediumrain))
			strcat(buf, "Идет дождь.\r\n");
		else if (IS_SET(weather_type, kWeatherLightrain))
			strcat(buf, "Моросит дождик.\r\n");

		SendMsgToChar(buf, ch);
	} else {
		SendMsgToChar("Вы ничего не можете сказать о погоде сегодня.\r\n", ch);
	}
	if (IS_GOD(ch)) {
		sprintf(buf, "День: %d Месяц: %s Час: %d Такт = %d\r\n"
					 "Температура =%-5d, за день = %-8d, за неделю = %-8d\r\n"
					 "Давление    =%-5d, за день = %-8d, за неделю = %-8d\r\n"
					 "Выпало дождя = %d(%d), снега = %d(%d). Лед = %d(%d). Погода = %08x(%08x).\r\n",
				time_info.day, month_name[time_info.month], time_info.hours,
				weather_info.hours_go, weather_info.temperature,
				weather_info.temp_last_day, weather_info.temp_last_week,
				weather_info.pressure, weather_info.press_last_day,
				weather_info.press_last_week, weather_info.rainlevel,
				world[ch->in_room]->weather.rainlevel, weather_info.snowlevel,
				world[ch->in_room]->weather.snowlevel, weather_info.icelevel,
				world[ch->in_room]->weather.icelevel,
				weather_info.weather_type, world[ch->in_room]->weather.weather_type);
		SendMsgToChar(buf, ch);
	}
}

void PrintUptime(std::ostringstream &out) {
	auto uptime = time(nullptr) - shutdown_parameters.get_boot_time();
	auto d = uptime / 86400;
	auto h = (uptime / 3600) % 24;
	auto m = (uptime / 60) % 60;
	auto s = uptime % 60;

	out << std::setprecision(2) << d << "д " << h << ":" << m << ":" << s << "\r\n";
}

void PrintPair(std::ostringstream &out, int column_width, int val1, int val2) {
		out << KIRED << "[" << KICYN << std::right << std::setw(column_width) << val1
		<< KIRED << "|" << KICYN << std::setw(column_width) << val2 << KIRED << "]" << KNRM << "\r\n";
}

void PrintValue(std::ostringstream &out, int column_width, int val) {
	out << KIRED << "[" << KICYN << std::right << std::setw(column_width) << val << KIRED << "]" << KNRM << "\r\n";
}

void do_statistic(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	static std::unordered_map<ECharClass, std::pair<int, int>> players;
	if (players.empty()) {
		for (const auto &char_class: MUD::Classes()) {
			if (char_class.IsAvailable()) {
				players.emplace(char_class.GetId(), std::pair<int, int>());
			}
		}
	} else {
		for (auto &it : players) {
			it.second.first = 0;
			it.second.second = 0;
		}
	}

	int clan{0}, noclan{0}, hilvl{0}, lowlvl{0}, rem{0}, norem{0}, pk{0}, nopk{0}, total{0};
	for (const auto &tch : character_list) {
		if (tch->IsNpc() || GetRealLevel(tch) >= kLvlImmortal || !HERE(tch)) {
			continue;
		}
		CLAN(tch) ? ++clan : ++noclan;
		GetRealRemort(tch) >= 1 ? ++rem : ++norem;
		pk_count(tch.get()) >= 1 ? ++pk : ++nopk;

		if (GetRealLevel(tch) >= 25) {
			++players[(tch->GetClass())].first;
			++hilvl;
		} else {
			++players[(tch->GetClass())].second;
			++lowlvl;
		}
		++total;
	}
	/*
	 * Тут нужно использовать форматирование таблицы
	 * \todo table format
	 */
	std::ostringstream out;
	out << KICYN << " Статистика по персонажам в игре (всего / 25 ур. и выше / ниже 25 ур.):"
	<< KNRM << "\r\n" << "\r\n" << " ";
	int count{1};
	const int columns{2};
	const int class_name_col_width{15};
	const int number_col_width{3};
	for (const auto &it : players) {
		out << std::left << std::setw(class_name_col_width) << MUD::Class(it.first).GetPluralName() << " "
			<< KIRED << "[" << KICYN
			<< std::setw(number_col_width) << std::right << it.second.first + it.second.second
			<< KIRED << "|" << KICYN
			<< std::setw(number_col_width) << std::right << it.second.first
			<< KIRED << "|" << KICYN
			<< std::setw(number_col_width) << std::right << it.second.second
			<< KIRED << "]" << KNRM;
		if (count % columns == 0) {
			out << "\r\n" << " ";
		} else {
			out << "  ";
		}
		++count;
	}
	out << "\r\n";

	const int headline_width{33};

	out << std::left << std::setw(headline_width) << " Всего игроков:";
	PrintValue(out, number_col_width, total);

	out << std::left << std::setw(headline_width) << " Игроков выше|ниже 25 уровня:";
	PrintPair(out, number_col_width, hilvl, lowlvl);

	out << std::left << std::setw(headline_width) << " Игроков с перевоплощениями|без:";
	PrintPair(out, number_col_width, rem, norem);

	out << std::left << std::setw(headline_width) << " Клановых|внеклановых игроков:";
	PrintPair(out, number_col_width, clan, noclan);

	out << std::left << std::setw(headline_width) << " Игроков с флагами ПК|без ПК:";
	PrintPair(out, number_col_width, pk, nopk);

	out << std::left << std::setw(headline_width) << " Героев (без ПК) | Тварей убито:";
	const int kills_col_width{5};
	PrintPair(out, kills_col_width, char_stat::players_killed, char_stat::mobs_killed);
	out << "\r\n";

	char_stat::PrintClassesExpStat(out);

	out << " Времени с перезагрузки: ";
	PrintUptime(out);

	SendMsgToChar(out.str(), ch);
}

void sendWhoami(CharData *ch) {
	sprintf(buf, "Персонаж : %s\r\n", GET_NAME(ch));
	sprintf(buf + strlen(buf),
			"Падежи : &W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n\r\n",
			ch->get_name().c_str(), GET_PAD(ch, 1), GET_PAD(ch, 2),
			GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5));

	sprintf(buf + strlen(buf), "Ваш e-mail : &S%s&s\r\n", GET_EMAIL(ch));
	time_t birt = ch->player_data.time.birth;
	sprintf(buf + strlen(buf), "Дата вашего рождения : %s\r\n", rustime(localtime(&birt)));
	sprintf(buf + strlen(buf), "Ваш IP-адрес : %s\r\n", ch->desc ? ch->desc->host : "Unknown");
	SendMsgToChar(buf, ch);
	if (!NAME_GOD(ch)) {
		sprintf(buf, "Имя никем не одобрено!\r\n");
		SendMsgToChar(buf, ch);
	} else {
		const int god_level = NAME_GOD(ch) > 1000 ? NAME_GOD(ch) - 1000 : NAME_GOD(ch);
		sprintf(buf1, "%s", GetNameById(NAME_ID_GOD(ch)));
		*buf1 = UPPER(*buf1);

		static const char *by_rank_god = "Богом";
		static const char *by_rank_privileged = "привилегированным игроком";
		const char *by_rank = god_level < kLvlImmortal ? by_rank_privileged : by_rank_god;

		if (NAME_GOD(ch) < 1000)
			snprintf(buf, kMaxStringLength, "&RИмя запрещено %s %s&n\r\n", by_rank, buf1);
		else
			snprintf(buf, kMaxStringLength, "&WИмя одобрено %s %s&n\r\n", by_rank, buf1);
		SendMsgToChar(buf, ch);
	}
	sprintf(buf, "Перевоплощений: %d\r\n", GetRealRemort(ch));
	SendMsgToChar(buf, ch);
	Clan::CheckPkList(ch);
	if (ch->player_specials->saved.telegram_id != 0) { //тут прямое обращение, ибо базовый класс, а не наследник
		SendMsgToChar(ch, "Подключен Телеграм, chat_id: %lu\r\n", ch->player_specials->saved.telegram_id);
	}

}

// Generic page_string function for displaying text
void do_gen_ps(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	//DescriptorData *d;
	switch (subcmd) {
		case SCMD_CREDITS: page_string(ch->desc, credits, 0);
			break;
		case SCMD_INFO: page_string(ch->desc, info, 0);
			break;
		case SCMD_IMMLIST: page_string(ch->desc, immlist, 0);
			break;
		case SCMD_HANDBOOK: page_string(ch->desc, handbook, 0);
			break;
		case SCMD_POLICIES: page_string(ch->desc, policies, 0);
			break;
		case SCMD_MOTD: page_string(ch->desc, motd, 0);
			break;
		case SCMD_RULES: page_string(ch->desc, rules, 0);
			break;
		case SCMD_CLEAR: SendMsgToChar("\033[H\033[J", ch);
			break;
		case SCMD_VERSION: show_code_date(ch);
			break;
		case SCMD_WHOAMI: {
			sendWhoami(ch);
			break;
		}
		default: log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
			return;
	}
}

void do_diagnose(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom)))
			SendMsgToChar(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	} else {
		if (ch->GetEnemy())
			diag_char_to_char(ch->GetEnemy(), ch);
		else
			SendMsgToChar("На кого вы хотите взглянуть?\r\n", ch);
	}
}

struct sort_struct {
	int sort_pos;
	byte is_social;
} *cmd_sort_info = nullptr;

int num_of_cmds;

void SortCommands() {
	int a, b, tmp;

	num_of_cmds = 0;

	// first, count commands (num_of_commands is actually one greater than the
	// number of commands; it inclues the '\n'.

	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	// create data array
	CREATE(cmd_sort_info, num_of_cmds);

	// initialize it
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social = false;
	}

	// the infernal special case
	cmd_sort_info[find_command("insult")].is_social = true;

	// Sort.  'a' starts at 1, not 0, to remove 'RESERVED'
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
					   cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}

void do_commands(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int no, i, cmd_num, num_of;
	int wizhelp = 0, socials = 0;
	CharData *vict = ch;

	one_argument(argument, arg);

	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;

	sprintf(buf, "Следующие %s%s доступны %s:\r\n",
			wizhelp ? "привилегированные " : "",
			socials ? "социалы" : "команды", vict == ch ? "вам" : GET_PAD(vict, 2));

	if (socials)
		num_of = number_of_social_commands;
	else
		num_of = num_of_cmds - 1;

	// cmd_num starts at 1, not 0, to remove 'RESERVED'
	for (no = 1, cmd_num = socials ? 0 : 1; cmd_num < num_of; cmd_num++)
		if (socials) {
			sprintf(buf + strlen(buf), "%-19s", soc_keys_list[cmd_num].keyword);
			if (!(no % 4))
				strcat(buf, "\r\n");
			no++;
		} else {
			i = cmd_sort_info[cmd_num].sort_pos;
			if (cmd_info[i].minimum_level >= 0
				&& (privilege::HasPrivilege(vict, std::string(cmd_info[i].command), i, 0))
				&& (cmd_info[i].minimum_level >= kLvlImmortal) == wizhelp
				&& (wizhelp || socials == cmd_sort_info[i].is_social)) {
				sprintf(buf + strlen(buf), "%-15s", cmd_info[i].command);
				if (!(no % 5))
					strcat(buf, "\r\n");
				no++;
			}
		}

	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
}


// Create web-page with users list
void make_who2html() {
	FILE *opf;
	DescriptorData *d;

	int imms_num = 0, morts_num = 0;

	char *imms = nullptr;
	char *morts = nullptr;
	char *buffer = nullptr;

	if ((opf = fopen(WHOLIST_FILE, "w")) == nullptr)
		return;        // or log it ? *shrug*
	fprintf(opf, "<HTML><HEAD><TITLE>Кто сейчас в Былинах?</TITLE></HEAD>\n");
	fprintf(opf, "<BODY><H1>Кто сейчас живет в Былинах?</H1><HR>\n");

	sprintf(buf, "БОГИ <BR> \r\n");
	imms = str_add(imms, buf);

	sprintf(buf, "<BR>Игроки<BR> \r\n  ");
	morts = str_add(morts, buf);

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) == CON_PLAYING
			&& GET_INVIS_LEV(d->character) < 31) {
			const auto ch = d->character;
			sprintf(buf, "%s <BR> \r\n ", ch->race_or_title().c_str());

			if (IS_IMMORTAL(ch)) {
				imms_num++;
				imms = str_add(imms, buf);
			} else {
				morts_num++;
				morts = str_add(morts, buf);
			}
		}
	}

	if (morts_num + imms_num == 0) {
		sprintf(buf, "Все ушли на фронт! <BR>");
		buffer = str_add(buffer, buf);
	} else {
		if (imms_num > 0)
			buffer = str_add(buffer, imms);
		if (morts_num > 0)
			buffer = str_add(buffer, morts);
		buffer = str_add(buffer, " <BR> \r\n Всего :");
		if (imms_num) {
			// sprintf(buf+strlen(buf)," бессмертных %d",imms_num);
			sprintf(buf, " бессмертных %d", imms_num);
			buffer = str_add(buffer, buf);
		}
		if (morts_num) {
			// sprintf(buf+strlen(buf)," смертных %d",morts_num);
			sprintf(buf, " смертных %d", morts_num);
			buffer = str_add(buffer, buf);
		}

		buffer = str_add(buffer, ".\n");
	}

	fprintf(opf, "%s", buffer);

	free(buffer);
	free(imms);
	free(morts);

	fprintf(opf, "<HR></BODY></HTML>\n");
	fclose(opf);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
