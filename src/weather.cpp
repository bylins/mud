/* ************************************************************************
*   File: weather.cpp                                   Part of Bylins    *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "weather.hpp"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "spells.h"
#include "skills.h"
#include "logger.hpp"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "random.hpp"
#include "char.hpp"
#include "room.hpp"
#include "celebrates.hpp"

extern void script_timechange_trigger_check(const int time);//Эксопрт тригеров смены времени
extern TIME_INFO_DATA time_info;

void another_hour(int mode);
void weather_change(void);

void calc_easter(void);


int EasterMonth = 0;
int EasterDay = 0;


void gods_day_now(CHAR_DATA * ch)
{
	char mono[MAX_INPUT_LENGTH], poly[MAX_INPUT_LENGTH], real[MAX_INPUT_LENGTH];
	*mono=0; *poly=0; *real=0;

	std::string mono_name = Celebrates::get_name_mono(Celebrates::get_mud_day());
	std::string poly_name = Celebrates::get_name_poly(Celebrates::get_mud_day());
	std::string real_name = Celebrates::get_name_real(Celebrates::get_real_day());

	if (IS_IMMORTAL(ch))
	{
		sprintf(poly, "Язычники : %s Нет праздника. %s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(mono, "Христиане: %s Нет праздника. %s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(real, "В реальном мире: %s Нет праздника. %s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));

		if (mono_name != "")
		{
			sprintf(mono, "Христиане: %s %s. %s\r\n", CCWHT(ch, C_NRM),
				mono_name.c_str(), CCNRM(ch, C_NRM));
		}

		if (poly_name != "")
		{
			sprintf(poly, "Язычники : %s %s. %s\r\n", CCWHT(ch, C_NRM),
							poly_name.c_str(), CCNRM(ch, C_NRM));
		}

		sprintf(mono + strlen(mono), "Пасха    : %d.%02d\r\n", EasterDay + 1, EasterMonth + 1);
		send_to_char(poly, ch);
		send_to_char(mono, ch);
	}
	else if (GET_RELIGION(ch) == RELIGION_POLY)
	{
		if (poly_name != "")
		{
			sprintf(poly, "%s Сегодня %s. %s\r\n", CCWHT(ch, C_NRM),
				poly_name.c_str(), CCNRM(ch, C_NRM));
			send_to_char(poly, ch);
		}
	}
	else if (GET_RELIGION(ch) == RELIGION_MONO)
	{
		if (mono_name != "")
		{
			sprintf(mono, "%s Сегодня %s. %s\r\n", CCWHT(ch, C_NRM),
						mono_name.c_str(), CCNRM(ch, C_NRM));
			send_to_char(mono, ch);
		}
	}
	if (real_name != "")
	{
		sprintf(real, "В реальном мире : %s %s. %s\r\n", CCWHT(ch, C_NRM),
						real_name.c_str(), CCNRM(ch, C_NRM));
	}
	send_to_char(real, ch);
}

void weather_and_time(int mode)
{
	another_hour(mode);
	if (mode)
	{
		weather_change();
	}
}

const int sunrise[][2] = { {8, 17},
	{8, 18},
	{7, 19},
	{6, 20},
	{5, 21},
	{4, 22},
	{5, 22},
	{6, 21},
	{7, 21},
	{7, 20},
	{7, 19},
	{8, 17}
};

void another_hour(int/* mode*/)
{
	time_info.hours++;

	if (time_info.hours == sunrise[time_info.month][0])
	{
		weather_info.sunlight = SUN_RISE;
		send_to_outdoor("На востоке показались первые солнечные лучи.\r\n", SUN_CONTROL);
		// Gorrah: Закомментил вызов тут и далее по коду, пока не будет приведено в нормальный виж.
		// Вы с дуба рухнули - каждый тик бегать по ~100-150к объектов в мире?
		// Надо делать через собственный лист объектов с такими тригами в собственном же неймспейсе
		// См. как сделаны slow DT например.
		//script_timechange_trigger_check(25);//рассвет
	}
	else if (time_info.hours == sunrise[time_info.month][0] + 1)
	{
		weather_info.sunlight = SUN_LIGHT;
		send_to_outdoor("Начался день.\r\n", SUN_CONTROL);
		//script_timechange_trigger_check(26);//день
	}
	else if (time_info.hours == sunrise[time_info.month][1])
	{
		weather_info.sunlight = SUN_SET;
		send_to_outdoor("Солнце медленно исчезло за горизонтом.\r\n", SUN_CONTROL);
		//script_timechange_trigger_check(27);//закат
	}
	else if (time_info.hours == sunrise[time_info.month][1] + 1)
	{
		weather_info.sunlight = SUN_DARK;
		send_to_outdoor("Началась ночь.\r\n", SUN_CONTROL);
		//script_timechange_trigger_check(28);//ночь
	}

	if (time_info.hours >= HOURS_PER_DAY)  	// Changed by HHS due to bug ???
	{
		time_info.hours = 0;
		time_info.day++;

		weather_info.moon_day++;
		if (weather_info.moon_day >= MOON_CYCLE)
			weather_info.moon_day = 0;
		weather_info.week_day_mono++;
		if (weather_info.week_day_mono >= WEEK_CYCLE)
			weather_info.week_day_mono = 0;
		weather_info.week_day_poly++;
		if (weather_info.week_day_poly >= POLY_WEEK_CYCLE)
			weather_info.week_day_poly = 0;

		if (time_info.day >= DAYS_PER_MONTH)
		{
			time_info.day = 0;
			time_info.month++;
			if (time_info.month >= MONTHS_PER_YEAR)
			{
				time_info.month = 0;
				time_info.year++;
				calc_easter();
			}
		}
	}
	//script_timechange_trigger_check(24);//просто смена часа
	//script_timechange_trigger_check(time_info.hours);//выполняется для конкретного часа

	if ((weather_info.sunlight == SUN_SET ||
			weather_info.sunlight == SUN_DARK) &&
			weather_info.sky == SKY_LIGHTNING &&
			weather_info.moon_day >= FULLMOONSTART && weather_info.moon_day <= FULLMOONSTOP)
	{
		send_to_outdoor("Лунный свет заливает равнины тусклым светом.\r\n", SUN_CONTROL);
	}
}

struct month_temperature_type year_temp[MONTHS_PER_YEAR] = { { -32, + 4, -18},	//Jan
	{ -28, + 5, -15},			//Feb
	{ -12, + 12, -6},			//Mar
	{ -10, + 15, + 8},			//Apr
	{ -1, + 25, + 12},			//May
	{ + 6, + 30, + 18},			//Jun
	{ + 8, + 37, + 24},			//Jul
	{ + 4, + 32, + 17},			//Aug
	{ + 2, + 21, + 12},			//Sep
	{ -5, + 18, + 8},			//Oct
	{ -12, + 17, + 6},			//Nov
	{ -27, + 5, -10}			//Dec
};

int day_temp_change[HOURS_PER_DAY][4] = { {0, -1, 0, -1},	// From 23 -> 00
	{ -1, 0, -1, -1},		// From 00 -> 01
	{0, -1, 0, 0},			// From 01 -> 02
	{0, -1, 0, -1},			// From 02 -> 03
	{ -1, 0, -1, -1},		// From 03 -> 04
	{ -1, -1, 0, -1},		// From 04 -> 05
	{0, 0, 0, 0},			// From 05 -> 06
	{0, 0, 0, 0},			// From 07 -> 08
	{0, 1, 0, 1},			// From 08 -> 09
	{1, 1, 0, 1},			// From 09 -> 10
	{1, 1, 1, 1},			// From 10 -> 11
	{1, 1, 0, 2},			// From 11 -> 12
	{1, 1, 1, 1},			// From 12 -> 13
	{1, 0, 1, 1},			// From 13 -> 14
	{0, 0, 0, 0},			// From 14 -> 15
	{0, 0, 0, 0},			// From 15 -> 16
	{0, 0, 0, 0},			// From 16 -> 17
	{0, 0, 0, 0},			// From 17 -> 18
	{0, 0, 0, -1},			// From 18 -> 19
	{0, 0, 0, 0},			// From 19 -> 20
	{ -1, 0, -1, 0},			// From 20 -> 21
	{0, -1, 0, -1},			// From 21 -> 22
	{ -1, 0, 0, 0}			// From 22 -> 23
};

int average_day_temp(void)
{
	return (weather_info.temp_last_day / MAX(1, MIN(HOURS_PER_DAY, weather_info.hours_go)));
}

int average_week_temp(void)
{
	return (weather_info.temp_last_week / MAX(1, MIN(DAYS_PER_WEEK * HOURS_PER_DAY, weather_info.hours_go)));
}

int average_day_press(void)
{
	return (weather_info.press_last_day / MAX(1, MIN(HOURS_PER_DAY, weather_info.hours_go)));
}

int average_week_press(void)
{
	return (weather_info.press_last_week / MAX(1, MIN(DAYS_PER_WEEK * HOURS_PER_DAY, weather_info.hours_go)));
}

void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3)
{
	int value = number(1, 100);
	if (value <= chance1)
		(*wtype) |= (startvalue << 0);
	else if (value <= chance1 + chance2)
		(*wtype) |= (startvalue << 1);
	else if (value <= chance1 + chance2 + chance3)
		(*wtype) |= (startvalue << 2);
}

int avg_day_temp, avg_week_temp;

void calc_basic(int weather_type, int sky, int *rainlevel, int *snowlevel)
{				// Rain's increase
	if (IS_SET(weather_type, WEATHER_LIGHTRAIN))
		*rainlevel += 2;
	if (IS_SET(weather_type, WEATHER_MEDIUMRAIN))
		*rainlevel += 3;
	if (IS_SET(weather_type, WEATHER_BIGRAIN | WEATHER_GRAD))
		*rainlevel += 5;

	// Snow's increase
	if (IS_SET(weather_type, WEATHER_LIGHTSNOW))
		*snowlevel += 1;
	if (IS_SET(weather_type, WEATHER_MEDIUMSNOW))
		*snowlevel += 2;
	if (IS_SET(weather_type, WEATHER_BIGSNOW))
		*snowlevel += 4;

	// Rains and snow decrease by time and weather
	*rainlevel -= 1;
	*snowlevel -= 1;
	if (sky == SKY_LIGHTNING)
	{
		if (IS_SET(weather_info.weather_type, WEATHER_LIGHTWIND))
		{
			*rainlevel -= 1;
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMWIND))
		{
			*rainlevel -= 2;
			*snowlevel -= 1;
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND))
		{
			*rainlevel -= 3;
			*snowlevel -= 1;
		}
		else
		{
			*rainlevel -= 1;
		}
	}
	else if (sky == SKY_CLOUDLESS)
	{
		if (IS_SET(weather_info.weather_type, WEATHER_LIGHTWIND))
		{
			*rainlevel -= 1;
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMWIND))
		{
			*rainlevel -= 1;
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND))
		{
			*rainlevel -= 2;
			*snowlevel -= 1;
		}
	}
	else if (sky == SKY_CLOUDY)
	{
		if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND))
		{
			*rainlevel -= 1;
		}
	}
}




void weather_change(void)
{
	int diff = 0, sky_change, temp_change, i,
			   grainlevel = 0, gsnowlevel = 0, icelevel = 0, snowdec = 0,
											raincast = 0, snowcast = 0, avg_day_temp, avg_week_temp, cweather_type = 0, temp;

	weather_info.hours_go++;
	if (weather_info.hours_go > HOURS_PER_DAY)
	{
		weather_info.press_last_day = weather_info.press_last_day * (HOURS_PER_DAY - 1) / HOURS_PER_DAY;
		weather_info.temp_last_day = weather_info.temp_last_day * (HOURS_PER_DAY - 1) / HOURS_PER_DAY;
	}
	// Average pressure and temperature per 24 hours
	weather_info.press_last_day += weather_info.pressure;
	weather_info.temp_last_day += weather_info.temperature;
	if (weather_info.hours_go > (DAYS_PER_WEEK * HOURS_PER_DAY))
	{
		weather_info.press_last_week =
			weather_info.press_last_week * (DAYS_PER_WEEK * HOURS_PER_DAY -
											1) / (DAYS_PER_WEEK * HOURS_PER_DAY);
		weather_info.temp_last_week =
			weather_info.temp_last_week * (DAYS_PER_WEEK * HOURS_PER_DAY - 1) / (DAYS_PER_WEEK * HOURS_PER_DAY);
	}
	// Average pressure and temperature per week
	weather_info.press_last_week += weather_info.pressure;
	weather_info.temp_last_week += weather_info.temperature;
	avg_day_temp = average_day_temp();
	avg_week_temp = average_week_temp();

	calc_basic(weather_info.weather_type, weather_info.sky, &grainlevel, &gsnowlevel);

	// Ice and show change by temperature
	if (!(time_info.hours % 6) && weather_info.hours_go)
	{
		if (avg_day_temp < -15)
			icelevel += 4;
		else if (avg_day_temp < -10)
			icelevel += 3;
		else if (avg_day_temp < -5)
			icelevel += 2;
		else if (avg_day_temp < -1)
			icelevel += 1;
		else if (avg_day_temp < 1)
		{
			icelevel += 0;
			gsnowlevel -= 1;
			snowdec += 1;
		}
		else if (avg_day_temp < 5)
		{
			icelevel -= 1;
			gsnowlevel -= 2;
			snowdec += 2;
		}
		else if (avg_day_temp < 10)
		{
			icelevel -= 2;
			gsnowlevel -= 3;
			snowdec += 3;
		}
		else
		{
			icelevel -= 3;
			gsnowlevel -= 4;
			snowdec += 4;
		}
	}

	weather_info.icelevel = MAX(0, MIN(100, weather_info.icelevel + icelevel));
	if (gsnowlevel < -1 && weather_info.rainlevel < 20)
		weather_info.rainlevel -= (gsnowlevel / 2);
	weather_info.snowlevel = MAX(0, MIN(120, weather_info.snowlevel + gsnowlevel));
	weather_info.rainlevel = MAX(0, MIN(80, weather_info.rainlevel + grainlevel));


	// Change some values for world
	for (i = FIRST_ROOM; i <= top_of_world; i++)
	{
		raincast = snowcast = 0;
		if (ROOM_FLAGGED(i, ROOM_NOWEATHER))
			continue;
		if (world[i]->weather.duration)
		{
			calc_basic(world[i]->weather.weather_type, world[i]->weather.sky, &raincast, &snowcast);
			snowcast -= snowdec;
		}
		else
		{
			raincast = grainlevel;
			snowcast = gsnowlevel;
		}
		if (world[i]->weather.duration <= 0)
			world[i]->weather.duration = 0;
		else
			world[i]->weather.duration--;

		world[i]->weather.icelevel = MAX(0, MIN(100, world[i]->weather.icelevel + icelevel));
		if (snowcast < -1 && world[i]->weather.rainlevel < 20)
			world[i]->weather.rainlevel -= (snowcast / 2);
		world[i]->weather.snowlevel = MAX(0, MIN(120, world[i]->weather.snowlevel + snowcast));
		world[i]->weather.rainlevel = MAX(0, MIN(80, world[i]->weather.rainlevel + raincast));
	}


	switch (time_info.month)
	{
	case 0:		// Jan
		diff = (weather_info.pressure > 985 ? -2 : 2);
		break;
	case 1:		// Feb
		diff = (weather_info.pressure > 985 ? -2 : 2);
		break;
	case 2:		// Mar
		diff = (weather_info.pressure > 985 ? -2 : 2);
		break;
	case 3:		// Apr
		diff = (weather_info.pressure > 985 ? -2 : 2);
		break;
	case 4:		// May
		diff = (weather_info.pressure > 1015 ? -2 : 2);
		break;
	case 5:		// Jun
		diff = (weather_info.pressure > 1015 ? -2 : 2);
		break;
	case 6:		// Jul
		diff = (weather_info.pressure > 1015 ? -2 : 2);
		break;
	case 7:		// Aug
		diff = (weather_info.pressure > 1015 ? -2 : 2);
		break;
	case 8:		// Sep
		diff = (weather_info.pressure > 1015 ? -2 : 2);
		break;
	case 9:		// Oct
		diff = (weather_info.pressure > 1015 ? -2 : 2);
		break;
	case 10:		// Nov
		diff = (weather_info.pressure > 985 ? -2 : 2);
		break;
	case 11:		// Dec
		diff = (weather_info.pressure > 985 ? -2 : 2);
		break;
	default:
		break;
	}

	// if ((time_info.month >= 9) && (time_info.month <= 16))
	//    diff = (weather_info.pressure > 985 ? -2 : 2);
	// else
	//    diff = (weather_info.pressure > 1015 ? -2 : 2);

	weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

	weather_info.change = MIN(weather_info.change, 12);
	weather_info.change = MAX(weather_info.change, -12);

	weather_info.pressure += weather_info.change;

	weather_info.pressure = MIN(weather_info.pressure, 1040);
	weather_info.pressure = MAX(weather_info.pressure, 960);

	if (time_info.month == MONTH_MAY)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month >= MONTH_JUNE && time_info.month <= MONTH_AUGUST)
		weather_info.season = SEASON_SUMMER;
	else if (time_info.month >= MONTH_SEPTEMBER && time_info.month <= MONTH_OCTOBER)
		weather_info.season = SEASON_AUTUMN;
	else if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
		weather_info.season = SEASON_WINTER;


	switch (weather_info.season)
	{
	case SEASON_WINTER:
		if ((time_info.month == MONTH_MART && avg_week_temp > 5
				&& weather_info.snowlevel == 0) || (time_info.month == MONTH_APRIL && weather_info.snowlevel == 0))
			weather_info.season = SEASON_SPRING;
		break;
	case SEASON_AUTUMN:
		if (time_info.month == MONTH_NOVEMBER && (avg_week_temp < 2 || weather_info.snowlevel >= 5))
			weather_info.season = SEASON_WINTER;
		break;
	}

	sky_change = 0;
	temp_change = 0;

	switch (weather_info.sky)
	{
	case SKY_CLOUDLESS:
		if (weather_info.pressure < 990)
			sky_change = 1;
		else if (weather_info.pressure < 1010)
			if (dice(1, 4) == 1)
				sky_change = 1;
		break;
	case SKY_CLOUDY:
		if (weather_info.pressure < 970)
			sky_change = 2;
		else if (weather_info.pressure < 990)
		{
			if (dice(1, 4) == 1)
				sky_change = 2;
			else
				sky_change = 0;
		}
		else if (weather_info.pressure > 1030)
			if (dice(1, 4) == 1)
				sky_change = 3;

		break;
	case SKY_RAINING:
		if (weather_info.pressure < 970)
		{
			if (dice(1, 4) == 1)
				sky_change = 4;
			else
				sky_change = 0;
		}
		else if (weather_info.pressure > 1030)
			sky_change = 5;
		else if (weather_info.pressure > 1010)
			if (dice(1, 4) == 1)
				sky_change = 5;

		break;
	case SKY_LIGHTNING:
		if (weather_info.pressure > 1010)
			sky_change = 6;
		else if (weather_info.pressure > 990)
			if (dice(1, 4) == 1)
				sky_change = 6;

		break;
	default:
		sky_change = 0;
		weather_info.sky = SKY_CLOUDLESS;
		break;
	}

	switch (sky_change)
	{
	case 1:		// CLOUDLESS -> CLOUDY
		if (time_info.month >= MONTH_MAY && time_info.month <= MONTH_AUGUST)
		{
			if (time_info.hours >= 8 && time_info.hours <= 16)
				temp_change += number(-3, -1);
			else if (time_info.hours >= 5 && time_info.hours <= 20)
				temp_change += number(-1, 0);

		}
		else
			temp_change += number(-2, + 2);
		break;
	case 2:		// CLOUDY -> RAINING
		if (time_info.month >= MONTH_MAY && time_info.month <= MONTH_AUGUST)
			temp_change += number(-1, + 1);
		else
			temp_change += number(-2, + 2);
		break;
	case 3:		// CLOUDY -> CLOUDLESS
		if (time_info.month >= MONTH_MAY && time_info.month <= MONTH_AUGUST)
		{
			if (time_info.hours >= 7 && time_info.hours <= 19)
				temp_change += number( + 1, + 2);
			else
				temp_change += number(0, + 1);
		}
		else
			temp_change += number(-1, + 1);
		break;
	case 4:		// RAINING -> LIGHTNING
		if (time_info.month >= MONTH_MAY && time_info.month <= MONTH_AUGUST)
		{
			if (time_info.hours >= 10 && time_info.hours <= 18)
				temp_change += number( + 1, + 3);
			else
				temp_change += number(0, + 2);
		}
		else
			temp_change += number(-3, + 3);
		break;
	case 5:		// RAINING -> CLOUDY
		if (time_info.month >= MONTH_JUNE && time_info.month <= MONTH_AUGUST)
			temp_change += number(0, + 1);
		else
			temp_change += number(-1, + 1);
		break;
	case 6:		// LIGHTNING -> RAINING
		if (time_info.month >= MONTH_MAY && time_info.month <= MONTH_AUGUST)
		{
			if (time_info.hours >= 10 && time_info.hours <= 17)
				temp_change += number(-3, + 1);
			else
				temp_change += number(-1, + 2);
		}
		else
			temp_change += number( + 1, + 3);
		break;
	case 0:
	default:
		if (dice(1, 4) == 1)
			temp_change += number(-1, + 1);
		break;
	}

	temp_change += day_temp_change[time_info.hours][weather_info.sky];
	if (time_info.day >= 22)
	{
		temp = weather_info.temperature +
			   (time_info.month >= MONTH_DECEMBER ? year_temp[0].med : year_temp[time_info.month + 1].med);
		temp /= 2;
	}
	else if (time_info.day <= 8)
	{
		temp = weather_info.temperature + year_temp[time_info.month].med;
		temp /= 2;
	}
	else
		temp = weather_info.temperature;

	temp += temp_change;
	cweather_type = 0;
	*buf = '\0';
	if (weather_info.temperature - temp > 6)
	{
		strcat(buf, "Резкое похолодание.\r\n");
		SET_BIT(cweather_type, WEATHER_QUICKCOOL);
	}
	else if (weather_info.temperature - temp < -6)
	{
		strcat(buf, "Резкое потепление.\r\n");
		SET_BIT(cweather_type, WEATHER_QUICKHOT);
	}
	weather_info.temperature = MIN(year_temp[time_info.month].max, MAX(year_temp[time_info.month].min, temp));


	if (weather_info.change >= 10 || weather_info.change <= -10)
	{
		strcat(buf, "Сильный ветер.\r\n");
		SET_BIT(cweather_type, WEATHER_BIGWIND);
	}
	else if (weather_info.change >= 6 || weather_info.change <= -6)
	{
		strcat(buf, "Умеренный ветер.\r\n");
		SET_BIT(cweather_type, WEATHER_MEDIUMWIND);
	}
	else if (weather_info.change >= 2 || weather_info.change <= -2)
	{
		strcat(buf, "Слабый ветер.\r\n");
		SET_BIT(cweather_type, WEATHER_LIGHTWIND);
	}
	else if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND | WEATHER_MEDIUMWIND | WEATHER_LIGHTWIND))
	{
		strcat(buf, "Ветер утих.\r\n");
		if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND))
			SET_BIT(cweather_type, WEATHER_MEDIUMWIND);
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMWIND))
			SET_BIT(cweather_type, WEATHER_LIGHTWIND);
	}

	switch (sky_change)
	{
	case 1:		// CLOUDLESS -> CLOUDY
		strcat(buf, "Небо затянуло тучами.\r\n");
		weather_info.sky = SKY_CLOUDY;
		break;
	case 2:		// CLOUDY -> RAINING
		switch (time_info.month)
		{
		case MONTH_MAY:
		case MONTH_JUNE:
		case MONTH_JULY:
		case MONTH_AUGUST:
		case MONTH_SEPTEMBER:
			strcat(buf, "Начался дождь.\r\n");
			create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 30, 40, 30);
			break;
		case MONTH_DECEMBER:
		case MONTH_JANUARY:
		case MONTH_FEBRUARY:
			strcat(buf, "Пошел снег.\r\n");
			create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 30, 40, 30);
			break;
		case MONTH_OCTOBER:
		case MONTH_APRIL:
			if (IS_SET(cweather_type, WEATHER_QUICKCOOL)
					&& weather_info.temperature <= 5)
			{
				strcat(buf, "Пошел снег.\r\n");
				SET_BIT(cweather_type, WEATHER_LIGHTSNOW);
			}
			else
			{
				strcat(buf, "Начался дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 40, 60, 0);
			}
			break;
		case MONTH_NOVEMBER:
			if (avg_day_temp <= 3 || IS_SET(cweather_type, WEATHER_QUICKCOOL))
			{
				strcat(buf, "Пошел снег.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 40, 60, 0);
			}
			else
			{
				strcat(buf, "Начался дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 40, 60, 0);
			}
			break;
		case MONTH_MART:
			if (avg_day_temp >= 3 || IS_SET(cweather_type, WEATHER_QUICKHOT))
			{
				strcat(buf, "Начался дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 80, 20, 0);
			}
			else
			{
				strcat(buf, "Пошел снег.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 60, 30, 10);
			}
			break;
		}
		weather_info.sky = SKY_RAINING;
		break;
	case 3:		// CLOUDY -> CLOUDLESS
		strcat(buf, "Начало проясняться.\r\n");
		weather_info.sky = SKY_CLOUDLESS;
		break;
	case 4:		// RAINING -> LIGHTNING
		strcat(buf, "Налетевший ветер разогнал тучи.\r\n");
		weather_info.sky = SKY_LIGHTNING;
		break;
	case 5:		// RAINING -> CLOUDY
		if (IS_SET(weather_info.weather_type, WEATHER_LIGHTRAIN | WEATHER_MEDIUMRAIN | WEATHER_BIGRAIN))
			strcat(buf, "Дождь прекратился.\r\n");
		else if (IS_SET(weather_info.weather_type, WEATHER_LIGHTSNOW | WEATHER_MEDIUMSNOW | WEATHER_BIGSNOW))
			strcat(buf, "Снегопад прекратился.\r\n");
		else if (IS_SET(weather_info.weather_type, WEATHER_GRAD))
			strcat(buf, "Град прекратился.\r\n");
		weather_info.sky = SKY_CLOUDY;
		break;
	case 6:		// LIGHTNING -> RAINING
		switch (time_info.month)
		{
		case MONTH_MAY:
		case MONTH_JUNE:
		case MONTH_JULY:
		case MONTH_AUGUST:
		case MONTH_SEPTEMBER:
			if (IS_SET(cweather_type, WEATHER_QUICKCOOL))
			{
				strcat(buf, "Начался град.\r\n");
				SET_BIT(cweather_type, WEATHER_GRAD);
			}
			else
			{
				strcat(buf, "Полил дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 10, 40, 50);
			}
			break;
		case MONTH_DECEMBER:
		case MONTH_JANUARY:
		case MONTH_FEBRUARY:
			strcat(buf, "Повалил снег.\r\n");
			create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 10, 40, 50);
			break;
		case MONTH_OCTOBER:
		case MONTH_APRIL:
			if (IS_SET(cweather_type, WEATHER_QUICKCOOL)
					&& weather_info.temperature <= 5)
			{
				strcat(buf, "Повалил снег.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 40, 60, 0);
			}
			else
			{
				strcat(buf, "Начался дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 40, 60, 0);
			}
			break;
		case MONTH_NOVEMBER:
			if (avg_day_temp <= 3 || IS_SET(cweather_type, WEATHER_QUICKCOOL))
			{
				strcat(buf, "Повалил снег.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 40, 60, 0);
			}
			else
			{
				strcat(buf, "Начался дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 40, 60, 0);
			}
			break;
		case MONTH_MART:
			if (avg_day_temp >= 3 || IS_SET(cweather_type, WEATHER_QUICKHOT))
			{
				strcat(buf, "Начался дождь.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 80, 20, 0);
			}
			else
			{
				strcat(buf, "Пошел снег.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 60, 30, 10);
			}
			break;
		}
		weather_info.sky = SKY_RAINING;
		break;
	case 0:
	default:
		if (IS_SET(weather_info.weather_type, WEATHER_GRAD))
		{
			strcat(buf, "Град прекратился.\r\n");
			create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 10, 40, 50);
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_BIGRAIN))
		{
			if (weather_info.change >= 5)
			{
				strcat(buf, "Дождь утих.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 20, 80, 0);
			}
			else
				SET_BIT(cweather_type, WEATHER_BIGRAIN);
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMRAIN))
		{
			if (weather_info.change <= -5)
			{
				strcat(buf, "Дождь усилился.\r\n");
				SET_BIT(cweather_type, WEATHER_BIGRAIN);
			}
			else if (weather_info.change >= 5)
			{
				strcat(buf, "Дождь утих.\r\n");
				SET_BIT(cweather_type, WEATHER_LIGHTRAIN);
			}
			else
				SET_BIT(cweather_type, WEATHER_MEDIUMRAIN);
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_LIGHTRAIN))
		{
			if (weather_info.change <= -5)
			{
				strcat(buf, "Дождь усилился.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTRAIN, 0, 70, 30);
			}
			else
				SET_BIT(cweather_type, WEATHER_LIGHTRAIN);
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_BIGSNOW))
		{
			if (weather_info.change >= 5)
			{
				strcat(buf, "Снегопад утих.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 20, 80, 0);
			}
			else
				SET_BIT(cweather_type, WEATHER_BIGSNOW);
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMSNOW))
		{
			if (weather_info.change <= -5)
			{
				strcat(buf, "Снегопад усилился.\r\n");
				SET_BIT(cweather_type, WEATHER_BIGSNOW);
			}
			else if (weather_info.change >= 5)
			{
				strcat(buf, "Снегопад утих.\r\n");
				SET_BIT(cweather_type, WEATHER_LIGHTSNOW);
			}
			else
				SET_BIT(cweather_type, WEATHER_MEDIUMSNOW);
		}
		else if (IS_SET(weather_info.weather_type, WEATHER_LIGHTSNOW))
		{
			if (weather_info.change <= -5)
			{
				strcat(buf, "Снегопад усилился.\r\n");
				create_rainsnow(&cweather_type, WEATHER_LIGHTSNOW, 0, 70, 30);
			}
			else
				SET_BIT(cweather_type, WEATHER_LIGHTSNOW);
		}
		break;
	}

	if (*buf)
		send_to_outdoor(buf, WEATHER_CONTROL);
	weather_info.weather_type = cweather_type;
}



void calc_easter(void)
{
	TIME_INFO_DATA t = time_info;
	int moon_day = weather_info.moon_day, week_day = weather_info.week_day_mono;

	log("Сейчас>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);
	// Найдем весеннее солнцестояние - месяц 2 день 20
	if (t.month > 2 || (t.month == 2 && t.day > 20))
	{
		while (t.month != 2 || t.day != 20)
		{
			if (--moon_day < 0)
				moon_day = MOON_CYCLE - 1;
			if (--week_day < 0)
				week_day = WEEK_CYCLE - 1;
			if (--t.day < 0)
			{
				t.day = DAYS_PER_MONTH - 1;
				t.month--;
			}
		}
	}
	else if (t.month < 2 || (t.month == 2 && t.day < 20))
	{
		while (t.month != 2 || t.day != 20)
		{
			if (++moon_day >= MOON_CYCLE)
				moon_day = 0;
			if (++week_day >= WEEK_CYCLE)
				week_day = 0;
			if (++t.day >= DAYS_PER_MONTH)
			{
				t.day = 0;
				t.month++;
			}
		}
	}
	log("Равноденствие>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);

	// Найдем ближайшее полнолуние
	while (moon_day != MOON_CYCLE / 2)
	{
		if (++moon_day >= MOON_CYCLE)
			moon_day = 0;
		if (++week_day >= WEEK_CYCLE)
			week_day = 0;
		if (++t.day >= DAYS_PER_MONTH)
		{
			t.day = 0;
			t.month++;
		}
	}
	log("Полнолуние>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);

	// Найдем воскресенье
	while (week_day != WEEK_CYCLE - 1)
	{
		if (++moon_day >= MOON_CYCLE)
			moon_day = 0;
		if (++week_day >= WEEK_CYCLE)
			week_day = 0;
		if (++t.day >= DAYS_PER_MONTH)
		{
			t.day = 0;
			t.month++;
		}
	}
	log("Воскресенье>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);
	EasterDay = t.day;
	EasterMonth = t.month;
}


const int moon_modifiers[28] = { -10, -9, -7, -5, -3, 0, 0, 0, 0, 0, 0, 0, 1, 5, 10, 5, 1, 0, 0, 0, 0, 0,
								 0, 0, -2, -5, -7, -9 };

int day_spell_modifier(CHAR_DATA * ch, int/* spellnum*/, int type, int value)
{
	int modi = value;
	if (IS_NPC(ch) || ch->in_room == NOWHERE)
		return (modi);
	switch (type)
	{
	case GAPPLY_SPELL_SUCCESS:
		modi = modi * (100 + moon_modifiers[weather_info.moon_day]) / 100;
		break;
	case GAPPLY_SPELL_EFFECT:
		break;
	}
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
		modi = MAX(modi, value);
	return (modi);
}

int weather_spell_modifier(CHAR_DATA * ch, int spellnum, int type, int value)
{
	int modi = value, sky = weather_info.sky, season = weather_info.season;

	if (IS_NPC(ch) ||
			ch->in_room == NOWHERE ||
			SECT(ch->in_room) == SECT_INSIDE ||
			SECT(ch->in_room) == SECT_CITY ||
			ROOM_FLAGGED(ch->in_room, ROOM_INDOORS) || ROOM_FLAGGED(ch->in_room, ROOM_NOWEATHER) || IS_NPC(ch))
		return (modi);

	sky = GET_ROOM_SKY(ch->in_room);

	switch (type)
	{
	case GAPPLY_SPELL_SUCCESS:
	case GAPPLY_SPELL_EFFECT:
		switch (spellnum)  	// Огненные спеллы - лето, день, безоблачно
		{
		case SPELL_BURNING_HANDS:
		case SPELL_SHOCKING_GRASP:
		case SPELL_SHINEFLASH:
		case SPELL_COLOR_SPRAY:
		case SPELL_FIREBALL:
		case SPELL_FIREBLAST:
			if (season == SEASON_SUMMER &&
					(weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT))
			{
				if (sky == SKY_LIGHTNING)
					modi += (modi * number(20, 50) / 100);
				else if (sky == SKY_CLOUDLESS)
					modi += (modi * number(10, 25) / 100);
			}
			break;
			// Молнийные спеллы - облачно или дождливо
		case SPELL_CALL_LIGHTNING:
		case SPELL_LIGHTNING_BOLT:
		case SPELL_CHAIN_LIGHTNING:
		case SPELL_ARMAGEDDON:
			if (sky == SKY_RAINING)
				modi += (modi * number(20, 50) / 100);
			else if (sky == SKY_CLOUDY)
				modi += (modi * number(10, 25) / 100);
			break;
			// Водно-ледяные спеллы - зима
		case SPELL_CHILL_TOUCH:
		case SPELL_ICESTORM:
		case SPELL_CONE_OF_COLD:
		case SPELL_IMPLOSION:
			if (season == SEASON_WINTER)
			{
				if (sky == SKY_RAINING || sky == SKY_CLOUDY)
					modi += (modi * number(20, 50) / 100);
				else if (sky == SKY_CLOUDLESS || sky == SKY_LIGHTNING)
					modi += (modi * number(10, 25) / 100);
			}
			break;
		}
		break;
	}
	if (WAITLESS(ch))
		modi = MAX(modi, value);
	return (modi);
}

int complex_spell_modifier(CHAR_DATA * ch, int spellnum, int type, int value)
{
	int modi = value;
	modi = day_spell_modifier(ch, spellnum, type, modi);
	modi = weather_spell_modifier(ch, spellnum, type, modi);
	return (modi);
}

int day_skill_modifier(CHAR_DATA* /*ch*/, int/* skillnum*/, int/* type*/, int value)
{
	return value;
}

int weather_skill_modifier(CHAR_DATA * ch, int skillnum, int type, int value)
{
	int modi = value, sky = weather_info.sky;

	if (IS_NPC(ch) ||
			SECT(ch->in_room) == SECT_INSIDE ||
			SECT(ch->in_room) == SECT_CITY ||
			ROOM_FLAGGED(ch->in_room, ROOM_INDOORS) || ROOM_FLAGGED(ch->in_room, ROOM_NOWEATHER))
		return (modi);

	sky = GET_ROOM_SKY(ch->in_room);

	switch (type)
	{
	case GAPPLY_SKILL_SUCCESS:
		switch (skillnum)
		{
		case SKILL_INDEFINITE:
			if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
			{
				switch (sky)
				{
				case SKY_CLOUDLESS:
					modi = modi * 90 / 100;
					break;
				case SKY_CLOUDY:
					modi = modi * 80 / 100;
					break;
				case SKY_RAINING:
					modi = modi * 70 / 30;
					break;
				}
			}
			else
			{
				switch (sky)
				{
				case SKY_CLOUDY:
					modi = modi * number(85, 95) / 100;
					break;
				case SKY_RAINING:
					modi = modi * number(75, 85) / 100;
					break;
				}
			}
			break;
		}
		break;
	}
	if (WAITLESS(ch))
		modi = MAX(modi, value);
	return (modi);
}

int complex_skill_modifier(CHAR_DATA * ch, int skillnum, int type, int value)
{
	int modi = value;
	modi = day_skill_modifier(ch, skillnum, type, modi);
	modi = weather_skill_modifier(ch, skillnum, type, modi);
	return (modi);
}

int get_room_sky(int rnum)
{
	return GET_ROOM_SKY(rnum);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
