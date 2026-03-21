/**
\file do_weather.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
