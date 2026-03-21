/**
\file do_time.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/weather.h"

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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
