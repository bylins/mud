/**
\file do_weather.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/weather.h"

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
		std::string out;
		if (world[ch->in_room]->weather.duration > 0) {
			sky = world[ch->in_room]->weather.sky;
			weather_type = world[ch->in_room]->weather.weather_type;
		}
		out += fmt::format("Небо {}. {}\r\n{}\r\n", sky_look[sky],
						   get_moon(sky) ? moon_look[get_moon(sky) - 1] : "",
						   weather_info.change >= 0 ? "Атмосферное давление повышается."
													: "Атмосферное давление понижается.");
		out += fmt::format("На дворе {} {}.\r\n", weather_info.temperature,
						   grammar::GetDeclensionInNumber(weather_info.temperature, grammar::EWhat::kDegree));

		if (IS_SET(weather_info.weather_type, kWeatherBigwind))
			out += "Сильный ветер.\r\n";
		else if (IS_SET(weather_info.weather_type, kWeatherMediumwind))
			out += "Умеренный ветер.\r\n";
		else if (IS_SET(weather_info.weather_type, kWeatherLightwind))
			out += "Легкий ветерок.\r\n";

		if (IS_SET(weather_type, kWeatherBigsnow))
			out += "Валит снег.\r\n";
		else if (IS_SET(weather_type, kWeatherMediumsnow))
			out += "Снегопад.\r\n";
		else if (IS_SET(weather_type, kWeatherLightsnow))
			out += "Легкий снежок.\r\n";

		if (IS_SET(weather_type, kWeatherHail))
			out += "Дождь с градом.\r\n";
		else if (IS_SET(weather_type, kWeatherBigrain))
			out += "Льет, как из ведра.\r\n";
		else if (IS_SET(weather_type, kWeatherMediumrain))
			out += "Идет дождь.\r\n";
		else if (IS_SET(weather_type, kWeatherLightrain))
			out += "Моросит дождик.\r\n";

		SendMsgToChar(out, ch);
	} else {
		SendMsgToChar("Вы ничего не можете сказать о погоде сегодня.\r\n", ch);
	}
	if (privilege::IsGod(ch)) {
		// Ширины полей у богов оставлены printf-формами: значения числовые, байт равен символу.
		char out[kMaxStringLength];
		snprintf(out, sizeof(out), "День: %d Месяц: %s Час: %d Такт = %d\r\n"
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
		SendMsgToChar(out, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
