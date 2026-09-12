/**
\file do_time.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/weather.h"

void do_time(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int day, month, days_go;
	if (ch->IsNpc())
		return;
	std::string out = "Сейчас ";
	switch (time_info.hours % 24) {
		case 0: out += "полночь, ";
			break;
		case 1: out += "1 час ночи, ";
			break;
		case 2:
		case 3:
		case 4: out += fmt::format("{} часа ночи, ", time_info.hours);
			break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11: out += fmt::format("{} часов утра, ", time_info.hours);
			break;
		case 12: out += "полдень, ";
			break;
		case 13: out += "1 час пополудни, ";
			break;
		case 14:
		case 15:
		case 16: out += fmt::format("{} часа пополудни, ", time_info.hours - 12);
			break;
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23: out += fmt::format("{} часов вечера, ", time_info.hours - 12);
			break;
	}

	if (GET_RELIGION(ch) == kReligionPoly) {
		out += weekdays_poly[weather_info.week_day_poly];
	} else {
		out += weekdays[weather_info.week_day_mono];
	}
	switch (weather_info.sunlight) {
		case kSunDark: out += ", ночь";
			break;
		case kSunSet: out += ", закат";
			break;
		case kSunLight: out += ", день";
			break;
		case kSunRise: out += ", рассвет";
			break;
	}
	out += ".\r\n";
	SendMsgToChar(out, ch);

	day = time_info.day + 1;    // day in [1..30]
	out.clear();
	if (GET_RELIGION(ch) == kReligionPoly || privilege::IsImmortal(ch)) {
		days_go = time_info.month * kDaysPerMonth + time_info.day;
		month = days_go / 40;
		days_go = (days_go % 40) + 1;
		out += fmt::format("{}, {}й День, Год {}{}",
						   month_name_poly[month], days_go, time_info.year, privilege::IsImmortal(ch) ? ".\r\n" : "");
	}
	if (GET_RELIGION(ch) == kReligionMono || privilege::IsImmortal(ch)) {
		out += fmt::format("{}, {}й День, Год {}",
						   month_name[static_cast<int>(time_info.month)], day, time_info.year);
	}
	if (privilege::IsImmortal(ch)) {
		out += fmt::format("\r\n{}.{}.{}, дней с начала года: {}",
						   day, time_info.month + 1, time_info.year, (time_info.month * kDaysPerMonth) + day);
	}
	switch (weather_info.season) {
		case ESeason::kWinter: out += ", зима";
			break;
		case ESeason::kSpring: out += ", весна";
			break;
		case ESeason::kSummer: out += ", лето";
			break;
		case ESeason::kAutumn: out += ", осень";
			break;
	}
	out += ".\r\n";
	SendMsgToChar(out, ch);
	gods_day_now(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
