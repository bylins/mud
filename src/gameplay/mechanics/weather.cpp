/**
\file weather.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Механики календаря и погоды. Календарь отсюжа по-хорошему надо вынести в отдельный модуль.
*/

#include "weather.h"

#include "gameplay/magic/spells.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "utils/random.h"
#include "celebrates.h"
#include "engine/db/global_objects.h"

Weather weather_info;
extern void script_timechange_trigger_check(int time);//Эксопрт тригеров смены времени

void another_hour(int mode);
void weather_change();
void CalcEaster();

int EasterMonth = 0;
int EasterDay = 0;

int get_moon(int sky) {
	if (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight || sky == kSkyRaining)
		return 0;
	else if (weather_info.moon_day <= kNewMoonStop || weather_info.moon_day >= kNewMoonStart)
		return 1;
	else if (weather_info.moon_day < kHalfMoonStart)
		return 2;
	else if (weather_info.moon_day < kFullMoonStart)
		return 3;
	else if (weather_info.moon_day <= kFullMoonStop)
		return 4;
	else if (weather_info.moon_day < kLastHalfMoonStart)
		return 5;
	else
		return 6;
	return 0;
}

void gods_day_now(CharData *ch) {
	char mono[kMaxInputLength], poly[kMaxInputLength], real[kMaxInputLength];
	*mono = 0;
	*poly = 0;
	*real = 0;

	std::string mono_name = celebrates::GetNameMono(celebrates::GetMudDay());
	std::string poly_name = celebrates::GetNamePoly(celebrates::GetMudDay());
	std::string real_name = celebrates::GetNameReal(celebrates::GetRealDay());

	if (IS_IMMORTAL(ch)) {
		sprintf(poly, "Язычники : %s Нет праздника. %s\r\n", kColorWht, kColorNrm);
		sprintf(mono, "Христиане: %s Нет праздника. %s\r\n", kColorWht, kColorNrm);
		sprintf(real, "В реальном мире: %s Нет праздника. %s\r\n", kColorWht, kColorNrm);

		if (!mono_name.empty()) {
			sprintf(mono, "Христиане: %s %s. %s\r\n", kColorWht,
					mono_name.c_str(), kColorNrm);
		}

		if (!poly_name.empty()) {
			sprintf(poly, "Язычники : %s %s. %s\r\n", kColorWht,
					poly_name.c_str(), kColorNrm);
		}

		sprintf(mono + strlen(mono), "Пасха    : %d.%02d\r\n", EasterDay + 1, EasterMonth + 1);
		SendMsgToChar(poly, ch);
		SendMsgToChar(mono, ch);
	} else if (GET_RELIGION(ch) == kReligionPoly) {
		if (!poly_name.empty()) {
			sprintf(poly, "%s Сегодня %s. %s\r\n", kColorWht,
					poly_name.c_str(), kColorNrm);
			SendMsgToChar(poly, ch);
		}
	} else if (GET_RELIGION(ch) == kReligionMono) {
		if (!mono_name.empty()) {
			sprintf(mono, "%s Сегодня %s. %s\r\n", kColorWht,
					mono_name.c_str(), kColorNrm);
			SendMsgToChar(mono, ch);
		}
	}
	if (!real_name.empty()) {
		sprintf(real, "В реальном мире : %s %s. %s\r\n", kColorWht,
				real_name.c_str(), kColorNrm);
	}
	SendMsgToChar(real, ch);
}

void weather_and_time(int mode) {
	another_hour(mode);
	if (mode) {
		weather_change();
	}
}

const int sunrise[][2] = {{8, 17},
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

void another_hour(int/* mode*/) {
	time_info.hours++;
	int time_day;
	if (time_info.hours == sunrise[time_info.month][0]) {
		weather_info.sunlight = kSunRise;
		SendMsgToOutdoor("На востоке показались первые солнечные лучи.\r\n", SUN_CONTROL);
		time_day = kSunRise;
	} else if (time_info.hours == sunrise[time_info.month][0] + 1) {
		weather_info.sunlight = kSunLight;
		SendMsgToOutdoor("Начался день.\r\n", SUN_CONTROL);
		time_day = kSunLight;
	} else if (time_info.hours == sunrise[time_info.month][1]) {
		weather_info.sunlight = kSunSet;
		SendMsgToOutdoor("Солнце медленно исчезло за горизонтом.\r\n", SUN_CONTROL);
		time_day = kSunSet;
	} else if (time_info.hours == sunrise[time_info.month][1] + 1) {
		weather_info.sunlight = kSunDark;
		SendMsgToOutdoor("Началась ночь.\r\n", SUN_CONTROL);
		time_day = kSunDark;
	} else
		time_day = 0;

	if (time_info.hours >= kHoursPerDay) {
		time_info.hours = 0;
		time_info.day++;

		weather_info.moon_day++;
		if (weather_info.moon_day >= kMoonCycle)
			weather_info.moon_day = 0;
		weather_info.week_day_mono++;
		if (weather_info.week_day_mono >= kWeekCycle)
			weather_info.week_day_mono = 0;
		weather_info.week_day_poly++;
		if (weather_info.week_day_poly >= kPolyWeekCycle)
			weather_info.week_day_poly = 0;

		if (time_info.day >= kDaysPerMonth) {
			time_info.day = 0;
			time_info.month++;
			if (time_info.month >= kMonthsPerYear) {
				time_info.month = 0;
				time_info.year++;
				CalcEaster();
			}
		}
	}
	script_timechange_trigger_check(time_info.hours, time_day);//выполняется для конкретного часа

	if ((weather_info.sunlight == kSunSet ||
		weather_info.sunlight == kSunDark) &&
		weather_info.sky == kSkyLightning &&
		weather_info.moon_day >= kFullMoonStart && weather_info.moon_day <= kFullMoonStop) {
		SendMsgToOutdoor("Лунный свет заливает равнины тусклым светом.\r\n", SUN_CONTROL);
	}
}

struct MonthTemperature year_temp[kMonthsPerYear] = {{-32, +4, -18},    //Jan
															{-28, +5, -15},            //Feb
															{-12, +12, -6},            //Mar
															{-10, +15, +8},            //Apr
															{-1, +25, +12},            //May
															{+6, +30, +18},            //Jun
															{+8, +37, +24},            //Jul
															{+4, +32, +17},            //Aug
															{+2, +21, +12},            //Sep
															{-5, +18, +8},            //Oct
															{-12, +17, +6},            //Nov
															{-27, +5, -10}            //Dec
};

int day_temp_change[kHoursPerDay][4] = {{0, -1, 0, -1},    // From 23 -> 00
										 {-1, 0, -1, -1},        // From 00 -> 01
										 {0, -1, 0, 0},            // From 01 -> 02
										 {0, -1, 0, -1},            // From 02 -> 03
										 {-1, 0, -1, -1},        // From 03 -> 04
										 {-1, -1, 0, -1},        // From 04 -> 05
										 {0, 0, 0, 0},            // From 05 -> 06
										 {0, 0, 0, 0},            // From 07 -> 08
										 {0, 1, 0, 1},            // From 08 -> 09
										 {1, 1, 0, 1},            // From 09 -> 10
										 {1, 1, 1, 1},            // From 10 -> 11
										 {1, 1, 0, 2},            // From 11 -> 12
										 {1, 1, 1, 1},            // From 12 -> 13
										 {1, 0, 1, 1},            // From 13 -> 14
										 {0, 0, 0, 0},            // From 14 -> 15
										 {0, 0, 0, 0},            // From 15 -> 16
										 {0, 0, 0, 0},            // From 16 -> 17
										 {0, 0, 0, 0},            // From 17 -> 18
										 {0, 0, 0, -1},            // From 18 -> 19
										 {0, 0, 0, 0},            // From 19 -> 20
										 {-1, 0, -1, 0},            // From 20 -> 21
										 {0, -1, 0, -1},            // From 21 -> 22
										 {-1, 0, 0, 0}            // From 22 -> 23
};

int average_day_temp() {
	return (weather_info.temp_last_day / std::clamp(weather_info.hours_go, 1, kHoursPerDay));
}

int average_week_temp() {
	return (weather_info.temp_last_week / std::clamp(weather_info.hours_go, 1, kDaysPerWeek * kHoursPerDay));
}

int average_day_press() {
	return (weather_info.press_last_day / std::clamp(weather_info.hours_go, 1, kHoursPerDay));
}

int average_week_press() {
	return (weather_info.press_last_week / std::clamp(weather_info.hours_go, 1, kDaysPerWeek * kHoursPerDay));
}

void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3) {
	int value = number(1, 100);
	if (value <= chance1)
		(*wtype) |= (startvalue << 0);
	else if (value <= chance1 + chance2)
		(*wtype) |= (startvalue << 1);
	else if (value <= chance1 + chance2 + chance3)
		(*wtype) |= (startvalue << 2);
}

int avg_day_temp, avg_week_temp;

void calc_basic(int weather_type, int sky, int *rainlevel, int *snowlevel) {                // Rain's increase
	if (IS_SET(weather_type, kWeatherLightrain))
		*rainlevel += 2;
	if (IS_SET(weather_type, kWeatherMediumrain))
		*rainlevel += 3;
	if (IS_SET(weather_type, kWeatherBigrain | kWeatherHail))
		*rainlevel += 5;

	// Snow's increase
	if (IS_SET(weather_type, kWeatherLightsnow))
		*snowlevel += 1;
	if (IS_SET(weather_type, kWeatherMediumsnow))
		*snowlevel += 2;
	if (IS_SET(weather_type, kWeatherBigsnow))
		*snowlevel += 4;

	// Rains and snow decrease by time and weather
	*rainlevel -= 1;
	*snowlevel -= 1;
	if (sky == kSkyLightning) {
		if (IS_SET(weather_info.weather_type, kWeatherLightwind)) {
			*rainlevel -= 1;
		} else if (IS_SET(weather_info.weather_type, kWeatherMediumwind)) {
			*rainlevel -= 2;
			*snowlevel -= 1;
		} else if (IS_SET(weather_info.weather_type, kWeatherBigwind)) {
			*rainlevel -= 3;
			*snowlevel -= 1;
		} else {
			*rainlevel -= 1;
		}
	} else if (sky == kSkyCloudless) {
		if (IS_SET(weather_info.weather_type, kWeatherLightwind)) {
			*rainlevel -= 1;
		} else if (IS_SET(weather_info.weather_type, kWeatherMediumwind)) {
			*rainlevel -= 1;
		} else if (IS_SET(weather_info.weather_type, kWeatherBigwind)) {
			*rainlevel -= 2;
			*snowlevel -= 1;
		}
	} else if (sky == kSkyCloudy) {
		if (IS_SET(weather_info.weather_type, kWeatherBigwind)) {
			*rainlevel -= 1;
		}
	}
}

void weather_change() {
	int diff = 0, sky_change, temp_change, i,
		grainlevel = 0, gsnowlevel = 0, icelevel = 0, snowdec = 0,
		raincast = 0, snowcast = 0, avg_day_temp, avg_week_temp, cweather_type = 0, temp;

	weather_info.hours_go++;
	if (weather_info.hours_go > kHoursPerDay) {
		weather_info.press_last_day = weather_info.press_last_day * (kHoursPerDay - 1) / kHoursPerDay;
		weather_info.temp_last_day = weather_info.temp_last_day * (kHoursPerDay - 1) / kHoursPerDay;
	}
	// Average pressure and temperature per 24 hours
	weather_info.press_last_day += weather_info.pressure;
	weather_info.temp_last_day += weather_info.temperature;
	if (weather_info.hours_go > (kDaysPerWeek * kHoursPerDay)) {
		weather_info.press_last_week =
			weather_info.press_last_week * (kDaysPerWeek * kHoursPerDay -
				1) / (kDaysPerWeek * kHoursPerDay);
		weather_info.temp_last_week =
			weather_info.temp_last_week * (kDaysPerWeek * kHoursPerDay - 1) / (kDaysPerWeek * kHoursPerDay);
	}
	// Average pressure and temperature per week
	weather_info.press_last_week += weather_info.pressure;
	weather_info.temp_last_week += weather_info.temperature;
	avg_day_temp = average_day_temp();
	avg_week_temp = average_week_temp();

	calc_basic(weather_info.weather_type, weather_info.sky, &grainlevel, &gsnowlevel);

	// Ice and show change by temperature
	if (!(time_info.hours % 6) && weather_info.hours_go) {
		if (avg_day_temp < -15)
			icelevel += 4;
		else if (avg_day_temp < -10)
			icelevel += 3;
		else if (avg_day_temp < -5)
			icelevel += 2;
		else if (avg_day_temp < -1)
			icelevel += 1;
		else if (avg_day_temp < 1) {
			icelevel += 0;
			gsnowlevel -= 1;
			snowdec += 1;
		} else if (avg_day_temp < 5) {
			icelevel -= 1;
			gsnowlevel -= 2;
			snowdec += 2;
		} else if (avg_day_temp < 10) {
			icelevel -= 2;
			gsnowlevel -= 3;
			snowdec += 3;
		} else {
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
	for (i = kFirstRoom; i <= top_of_world; i++) {
		raincast = snowcast = 0;
		if (ROOM_FLAGGED(i, ERoomFlag::kNoWeather))
			continue;
		if (world[i]->weather.duration) {
			calc_basic(world[i]->weather.weather_type, world[i]->weather.sky, &raincast, &snowcast);
			snowcast -= snowdec;
		} else {
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

	switch (time_info.month) {
		case 0:        // Jan
			diff = (weather_info.pressure > 985 ? -2 : 2);
			break;
		case 1:        // Feb
			diff = (weather_info.pressure > 985 ? -2 : 2);
			break;
		case 2:        // Mar
			diff = (weather_info.pressure > 985 ? -2 : 2);
			break;
		case 3:        // Apr
			diff = (weather_info.pressure > 985 ? -2 : 2);
			break;
		case 4:        // May
			diff = (weather_info.pressure > 1015 ? -2 : 2);
			break;
		case 5:        // Jun
			diff = (weather_info.pressure > 1015 ? -2 : 2);
			break;
		case 6:        // Jul
			diff = (weather_info.pressure > 1015 ? -2 : 2);
			break;
		case 7:        // Aug
			diff = (weather_info.pressure > 1015 ? -2 : 2);
			break;
		case 8:        // Sep
			diff = (weather_info.pressure > 1015 ? -2 : 2);
			break;
		case 9:        // Oct
			diff = (weather_info.pressure > 1015 ? -2 : 2);
			break;
		case 10:        // Nov
			diff = (weather_info.pressure > 985 ? -2 : 2);
			break;
		case 11:        // Dec
			diff = (weather_info.pressure > 985 ? -2 : 2);
			break;
		default: break;
	}

	// if ((time_info.month >= 9) && (time_info.month <= 16))
	//    diff = (weather_info.pressure > 985 ? -2 : 2);
	// else
	//    diff = (weather_info.pressure > 1015 ? -2 : 2);

	weather_info.change += (RollDices(1, 4) * diff + RollDices(2, 6) - RollDices(2, 6));

	weather_info.change = MIN(weather_info.change, 12);
	weather_info.change = MAX(weather_info.change, -12);

	weather_info.pressure += weather_info.change;

	weather_info.pressure = MIN(weather_info.pressure, 1040);
	weather_info.pressure = MAX(weather_info.pressure, 960);

	if (time_info.month == EMonth::kMay)
		weather_info.season = ESeason::kSpring;
	else if (time_info.month >= EMonth::kJune && time_info.month <= EMonth::kAugust)
		weather_info.season = ESeason::kSummer;
	else if (time_info.month >= EMonth::kSeptember && time_info.month <= EMonth::kOctober)
		weather_info.season = ESeason::kAutumn;
	else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary)
		weather_info.season = ESeason::kWinter;

	switch (weather_info.season) {
		case ESeason::kWinter:
			if ((time_info.month == EMonth::kMarch && avg_week_temp > 5
				&& weather_info.snowlevel == 0) || (time_info.month == EMonth::kApril && weather_info.snowlevel == 0))
				weather_info.season = ESeason::kSpring;
			break;
		case ESeason::kAutumn:
			if (time_info.month == EMonth::kNovember && (avg_week_temp < 2 || weather_info.snowlevel >= 5))
				weather_info.season = ESeason::kWinter;
			break;
		default: break;
	}

	sky_change = 0;
	temp_change = 0;

	switch (weather_info.sky) {
		case kSkyCloudless:
			if (weather_info.pressure < 990)
				sky_change = 1;
			else if (weather_info.pressure < 1010)
				if (RollDices(1, 4) == 1)
					sky_change = 1;
			break;
		case kSkyCloudy:
			if (weather_info.pressure < 970)
				sky_change = 2;
			else if (weather_info.pressure < 990) {
				if (RollDices(1, 4) == 1)
					sky_change = 2;
				else
					sky_change = 0;
			} else if (weather_info.pressure > 1030)
				if (RollDices(1, 4) == 1)
					sky_change = 3;

			break;
		case kSkyRaining:
			if (weather_info.pressure < 970) {
				if (RollDices(1, 4) == 1)
					sky_change = 4;
				else
					sky_change = 0;
			} else if (weather_info.pressure > 1030)
				sky_change = 5;
			else if (weather_info.pressure > 1010)
				if (RollDices(1, 4) == 1)
					sky_change = 5;

			break;
		case kSkyLightning:
			if (weather_info.pressure > 1010)
				sky_change = 6;
			else if (weather_info.pressure > 990)
				if (RollDices(1, 4) == 1)
					sky_change = 6;

			break;
		default: sky_change = 0;
			weather_info.sky = kSkyCloudless;
			break;
	}

	switch (sky_change) {
		case 1:        // CLOUDLESS -> CLOUDY
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kAugust) {
				if (time_info.hours >= 8 && time_info.hours <= 16)
					temp_change += number(-3, -1);
				else if (time_info.hours >= 5 && time_info.hours <= 20)
					temp_change += number(-1, 0);

			} else
				temp_change += number(-2, +2);
			break;
		case 2:        // CLOUDY -> RAINING
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kAugust)
				temp_change += number(-1, +1);
			else
				temp_change += number(-2, +2);
			break;
		case 3:        // CLOUDY -> CLOUDLESS
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kAugust) {
				if (time_info.hours >= 7 && time_info.hours <= 19)
					temp_change += number(+1, +2);
				else
					temp_change += number(0, +1);
			} else
				temp_change += number(-1, +1);
			break;
		case 4:        // RAINING -> LIGHTNING
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kAugust) {
				if (time_info.hours >= 10 && time_info.hours <= 18)
					temp_change += number(+1, +3);
				else
					temp_change += number(0, +2);
			} else
				temp_change += number(-3, +3);
			break;
		case 5:        // RAINING -> CLOUDY
			if (time_info.month >= EMonth::kJune && time_info.month <= EMonth::kAugust)
				temp_change += number(0, +1);
			else
				temp_change += number(-1, +1);
			break;
		case 6:        // LIGHTNING -> RAINING
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kAugust) {
				if (time_info.hours >= 10 && time_info.hours <= 17)
					temp_change += number(-3, +1);
				else
					temp_change += number(-1, +2);
			} else
				temp_change += number(+1, +3);
			break;
		case 0:
		default:
			if (RollDices(1, 4) == 1)
				temp_change += number(-1, +1);
			break;
	}

	temp_change += day_temp_change[time_info.hours][weather_info.sky];
	if (time_info.day >= 22) {
		temp = weather_info.temperature +
			(time_info.month >= EMonth::kDecember ? year_temp[0].med : year_temp[time_info.month + 1].med);
		temp /= 2;
	} else if (time_info.day <= 8) {
		temp = weather_info.temperature + year_temp[time_info.month].med;
		temp /= 2;
	} else
		temp = weather_info.temperature;

	temp += temp_change;
	cweather_type = 0;
	*buf = '\0';
	if (weather_info.temperature - temp > 6) {
		strcat(buf, "Резкое похолодание.\r\n");
		SET_BIT(cweather_type, kWeatherQuickcool);
	} else if (weather_info.temperature - temp < -6) {
		strcat(buf, "Резкое потепление.\r\n");
		SET_BIT(cweather_type, kWeatherQuickhot);
	}
	weather_info.temperature = MIN(year_temp[time_info.month].max, MAX(year_temp[time_info.month].min, temp));

	if (weather_info.change >= 10 || weather_info.change <= -10) {
		strcat(buf, "Сильный ветер.\r\n");
		SET_BIT(cweather_type, kWeatherBigwind);
	} else if (weather_info.change >= 6 || weather_info.change <= -6) {
		strcat(buf, "Умеренный ветер.\r\n");
		SET_BIT(cweather_type, kWeatherMediumwind);
	} else if (weather_info.change >= 2 || weather_info.change <= -2) {
		strcat(buf, "Слабый ветер.\r\n");
		SET_BIT(cweather_type, kWeatherLightwind);
	} else if (IS_SET(weather_info.weather_type, kWeatherBigwind | kWeatherMediumwind | kWeatherLightwind)) {
		strcat(buf, "Ветер утих.\r\n");
		if (IS_SET(weather_info.weather_type, kWeatherBigwind))
			SET_BIT(cweather_type, kWeatherMediumwind);
		else if (IS_SET(weather_info.weather_type, kWeatherMediumwind))
			SET_BIT(cweather_type, kWeatherLightwind);
	}

	switch (sky_change) {
		case 1:        // CLOUDLESS -> CLOUDY
			strcat(buf, "Небо затянуло тучами.\r\n");
			weather_info.sky = kSkyCloudy;
			break;
		case 2:        // CLOUDY -> RAINING
			switch (time_info.month) {
				case EMonth::kMay:
				case EMonth::kJune:
				case EMonth::kJuly:
				case EMonth::kAugust:
				case EMonth::kSeptember: strcat(buf, "Начался дождь.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightrain, 30, 40, 30);
					break;
				case EMonth::kDecember:
				case EMonth::kJanuary:
				case EMonth::kFebruary: strcat(buf, "Пошел снег.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightsnow, 30, 40, 30);
					break;
				case EMonth::kOctober:
				case EMonth::kApril:
					if (IS_SET(cweather_type, kWeatherQuickcool)
						&& weather_info.temperature <= 5) {
						strcat(buf, "Пошел снег.\r\n");
						SET_BIT(cweather_type, kWeatherLightsnow);
					} else {
						strcat(buf, "Начался дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 40, 60, 0);
					}
					break;
				case EMonth::kNovember:
					if (avg_day_temp <= 3 || IS_SET(cweather_type, kWeatherQuickcool)) {
						strcat(buf, "Пошел снег.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightsnow, 40, 60, 0);
					} else {
						strcat(buf, "Начался дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 40, 60, 0);
					}
					break;
				case EMonth::kMarch:
					if (avg_day_temp >= 3 || IS_SET(cweather_type, kWeatherQuickhot)) {
						strcat(buf, "Начался дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 80, 20, 0);
					} else {
						strcat(buf, "Пошел снег.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightsnow, 60, 30, 10);
					}
					break;
			}
			weather_info.sky = kSkyRaining;
			break;
		case 3:        // CLOUDY -> CLOUDLESS
			strcat(buf, "Начало проясняться.\r\n");
			weather_info.sky = kSkyCloudless;
			break;
		case 4:        // RAINING -> LIGHTNING
			strcat(buf, "Налетевший ветер разогнал тучи.\r\n");
			weather_info.sky = kSkyLightning;
			break;
		case 5:        // RAINING -> CLOUDY
			if (IS_SET(weather_info.weather_type, kWeatherLightrain | kWeatherMediumrain | kWeatherBigrain))
				strcat(buf, "Дождь прекратился.\r\n");
			else if (IS_SET(weather_info.weather_type, kWeatherLightsnow | kWeatherMediumsnow | kWeatherBigsnow))
				strcat(buf, "Снегопад прекратился.\r\n");
			else if (IS_SET(weather_info.weather_type, kWeatherHail))
				strcat(buf, "Град прекратился.\r\n");
			weather_info.sky = kSkyCloudy;
			break;
		case 6:        // LIGHTNING -> RAINING
			switch (time_info.month) {
				case EMonth::kMay:
				case EMonth::kJune:
				case EMonth::kJuly:
				case EMonth::kAugust:
				case EMonth::kSeptember:
					if (IS_SET(cweather_type, kWeatherQuickcool)) {
						strcat(buf, "Начался град.\r\n");
						SET_BIT(cweather_type, kWeatherHail);
					} else {
						strcat(buf, "Полил дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 10, 40, 50);
					}
					break;
				case EMonth::kDecember:
				case EMonth::kJanuary:
				case EMonth::kFebruary: strcat(buf, "Повалил снег.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightsnow, 10, 40, 50);
					break;
				case EMonth::kOctober:
				case EMonth::kApril:
					if (IS_SET(cweather_type, kWeatherQuickcool)
						&& weather_info.temperature <= 5) {
						strcat(buf, "Повалил снег.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightsnow, 40, 60, 0);
					} else {
						strcat(buf, "Начался дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 40, 60, 0);
					}
					break;
				case EMonth::kNovember:
					if (avg_day_temp <= 3 || IS_SET(cweather_type, kWeatherQuickcool)) {
						strcat(buf, "Повалил снег.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightsnow, 40, 60, 0);
					} else {
						strcat(buf, "Начался дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 40, 60, 0);
					}
					break;
				case EMonth::kMarch:
					if (avg_day_temp >= 3 || IS_SET(cweather_type, kWeatherQuickhot)) {
						strcat(buf, "Начался дождь.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightrain, 80, 20, 0);
					} else {
						strcat(buf, "Пошел снег.\r\n");
						SetPrecipitations(&cweather_type, kWeatherLightsnow, 60, 30, 10);
					}
					break;
			}
			weather_info.sky = kSkyRaining;
			break;
		case 0:
		default:
			if (IS_SET(weather_info.weather_type, kWeatherHail)) {
				strcat(buf, "Град прекратился.\r\n");
				SetPrecipitations(&cweather_type, kWeatherLightrain, 10, 40, 50);
			} else if (IS_SET(weather_info.weather_type, kWeatherBigrain)) {
				if (weather_info.change >= 5) {
					strcat(buf, "Дождь утих.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightrain, 20, 80, 0);
				} else
					SET_BIT(cweather_type, kWeatherBigrain);
			} else if (IS_SET(weather_info.weather_type, kWeatherMediumrain)) {
				if (weather_info.change <= -5) {
					strcat(buf, "Дождь усилился.\r\n");
					SET_BIT(cweather_type, kWeatherBigrain);
				} else if (weather_info.change >= 5) {
					strcat(buf, "Дождь утих.\r\n");
					SET_BIT(cweather_type, kWeatherLightrain);
				} else
					SET_BIT(cweather_type, kWeatherMediumrain);
			} else if (IS_SET(weather_info.weather_type, kWeatherLightrain)) {
				if (weather_info.change <= -5) {
					strcat(buf, "Дождь усилился.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightrain, 0, 70, 30);
				} else
					SET_BIT(cweather_type, kWeatherLightrain);
			} else if (IS_SET(weather_info.weather_type, kWeatherBigsnow)) {
				if (weather_info.change >= 5) {
					strcat(buf, "Снегопад утих.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightsnow, 20, 80, 0);
				} else
					SET_BIT(cweather_type, kWeatherBigsnow);
			} else if (IS_SET(weather_info.weather_type, kWeatherMediumsnow)) {
				if (weather_info.change <= -5) {
					strcat(buf, "Снегопад усилился.\r\n");
					SET_BIT(cweather_type, kWeatherBigsnow);
				} else if (weather_info.change >= 5) {
					strcat(buf, "Снегопад утих.\r\n");
					SET_BIT(cweather_type, kWeatherLightsnow);
				} else
					SET_BIT(cweather_type, kWeatherMediumsnow);
			} else if (IS_SET(weather_info.weather_type, kWeatherLightsnow)) {
				if (weather_info.change <= -5) {
					strcat(buf, "Снегопад усилился.\r\n");
					SetPrecipitations(&cweather_type, kWeatherLightsnow, 0, 70, 30);
				} else
					SET_BIT(cweather_type, kWeatherLightsnow);
			}
			break;
	}

	if (*buf)
		SendMsgToOutdoor(buf, WEATHER_CONTROL);
	weather_info.weather_type = cweather_type;
}

void CalcEaster(void) {
	TimeInfoData t = time_info;
	int moon_day = weather_info.moon_day, week_day = weather_info.week_day_mono;

	log("Сейчас>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);
	// Найдем весеннее солнцестояние - месяц 2 день 20
	if (t.month > 2 || (t.month == 2 && t.day > 20)) {
		while (t.month != 2 || t.day != 20) {
			if (--moon_day < 0)
				moon_day = kMoonCycle - 1;
			if (--week_day < 0)
				week_day = kWeekCycle - 1;
			if (--t.day < 0) {
				t.day = kDaysPerMonth - 1;
				t.month--;
			}
		}
	} else if (t.month < 2 || (t.month == 2 && t.day < 20)) {
		while (t.month != 2 || t.day != 20) {
			if (++moon_day >= kMoonCycle)
				moon_day = 0;
			if (++week_day >= kWeekCycle)
				week_day = 0;
			if (++t.day >= kDaysPerMonth) {
				t.day = 0;
				t.month++;
			}
		}
	}
	log("Равноденствие>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);

	// Найдем ближайшее полнолуние
	while (moon_day != kMoonCycle / 2) {
		if (++moon_day >= kMoonCycle)
			moon_day = 0;
		if (++week_day >= kWeekCycle)
			week_day = 0;
		if (++t.day >= kDaysPerMonth) {
			t.day = 0;
			t.month++;
		}
	}
	log("Полнолуние>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);

	// Найдем воскресенье
	while (week_day != kWeekCycle - 1) {
		if (++moon_day >= kMoonCycle)
			moon_day = 0;
		if (++week_day >= kWeekCycle)
			week_day = 0;
		if (++t.day >= kDaysPerMonth) {
			t.day = 0;
			t.month++;
		}
	}
	log("Воскресенье>%d.%d (%d,%d)", t.day, t.month, moon_day, week_day);
	EasterDay = t.day;
	EasterMonth = t.month;
}

const int moon_modifiers[28] = {-10, -9, -7, -5, -3, 0, 0, 0, 0, 0, 0, 0, 1, 5, 10, 5, 1, 0, 0, 0, 0, 0,
								0, 0, -2, -5, -7, -9};

int CalcDaySpellMod(CharData *ch, ESpell /* spell_id */, int type, int value) {
	int modi = value;
	if (ch->IsNpc() || ch->in_room == kNowhere)
		return (modi);
	switch (type) {
		case GAPPLY_SPELL_SUCCESS: modi = modi * (100 + moon_modifiers[weather_info.moon_day]) / 100;
			break;
		case GAPPLY_SPELL_EFFECT: break;
	}
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike))
		modi = MAX(modi, value);
	return (modi);
}

int CalcWeatherSpellMod(CharData *ch, ESpell spell_id, int type, int value) {
	int modi = value, sky = weather_info.sky;
	auto season = weather_info.season;

	if (ch->IsNpc() ||
		ch->in_room == kNowhere ||
		SECT(ch->in_room) == ESector::kInside ||
		SECT(ch->in_room) == ESector::kCity ||
		ROOM_FLAGGED(ch->in_room, ERoomFlag::kIndoors) ||
		ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoWeather) ||
		!MUD::Spell(spell_id).IsViolent()) {
		return (modi);
		}

	sky = GET_ROOM_SKY(ch->in_room);
	auto element = MUD::Spell(spell_id).GetElement();
	switch (type) {
		case GAPPLY_SPELL_SUCCESS:
		case GAPPLY_SPELL_EFFECT:
			switch (element) {
				case EElement::kFire: {
					// Огненные спеллы - лето
					if (season == ESeason::kSummer) {
						if (sky == kSkyLightning)
							modi += (modi * number(20, 50) / 100);
						else if (sky == kSkyCloudless)
							modi += (modi * number(10, 25) / 100);
					}
					break;
				}
				case EElement::kAir: {
					// Воздушные спеллы - осень
					if (season == ESeason::kAutumn) {
						if (sky == kSkyRaining)
							modi += (modi * number(20, 50) / 100);
						else if (sky == kSkyCloudy)
							modi += (modi * number(10, 25) / 100);
					}
					break;
				}
				case EElement::kWater: {
					// Водно-ледяные спеллы - зима
					if (season == ESeason::kWinter) {
						if (sky == kSkyRaining || sky == kSkyCloudy)
							modi += (modi * number(20, 50) / 100);
						else if (sky == kSkyCloudless || sky == kSkyLightning)
							modi += (modi * number(10, 25) / 100);
					}
					break;
				}
				case EElement::kEarth: {
					// Земляные спеллы - осень
					if (season == ESeason::kSpring) {
						if (sky == kSkyRaining || sky == kSkyCloudy)
							modi += (modi * number(20, 50) / 100);
						else if (sky == kSkyCloudless || sky == kSkyLightning)
							modi += (modi * number(10, 25) / 100);
					}
					break;
				}
				case EElement::kLight: {
					switch (weather_info.sunlight) {
						case kSunDark:
							modi -= (modi * number(20, 50) / 100);
							break;
						case kSunSet:
							modi -= (modi * number(10, 25) / 100);
							break;
						case kSunLight:
							modi += (modi * number(20, 50) / 100);
							break;
						case kSunRise:
							modi += (modi * number(10, 25) / 100);
							break;
					}
					break;
				}
				case EElement::kDark: {
					switch (weather_info.sunlight) {
						case kSunDark:
							modi += (modi * number(20, 50) / 100);
							break;
						case kSunSet:
							modi += (modi * number(10, 25) / 100);
							break;
						case kSunLight:
							modi -= (modi * number(20, 50) / 100);
							break;
						case kSunRise:
							modi -= (modi * number(10, 25) / 100);
							break;
					}
					break;
				}
				case EElement::kLife: {
					auto moon = get_moon(world[ch->in_room]->weather.sky);
					if (moon == 0) { // новолуние
						modi -= (modi * number(20, 50) / 100);
					} else if (moon < 3) { // 1-2 молодая луна
						modi += (modi * number(10, 25) / 100);
					} else if (moon == 4) { // полнолуние
						modi += (modi * number(20, 50) / 100);
					}  else if (moon > 4) { // убывающая луна
						modi -= (modi * number(10, 25) / 100);
					}
					break;
				}
				case EElement::kMind: {
					auto moon = get_moon(world[ch->in_room]->weather.sky);
					if (moon == 0) { // новолуние
						modi += (modi * number(20, 50) / 100);
					} else if (moon < 3) { // 1-2 молодая луна
						modi -= (modi * number(10, 25) / 100);
					} else if (moon == 4) { // полнолуние
						modi -= (modi * number(20, 50) / 100);
					}  else if (moon > 4) { // убывающая луна
						modi += (modi * number(10, 25) / 100);
					}
					break;
				}
				default: break;
			}
			break;
	}
	return (modi);
}

int CalcComplexSpellMod(CharData *ch, ESpell spell_id, int type, int value) {
	int modi = value;
	modi = CalcDaySpellMod(ch, spell_id, type, modi);
	modi = CalcWeatherSpellMod(ch, spell_id, type, modi);
	return (modi);
}

// По идее все это скорее относится к скиллам, чем к погоде, и должно находиться в скиллпроцессоре.
int day_skill_modifier(CharData * /*ch*/, ESkill/* skillnum*/, int/* type*/, int value) {
	return value;
}

int weather_skill_modifier(CharData *ch, ESkill skillnum, int type, int value) {
	int modi = value, sky = weather_info.sky;

	if (ch->IsNpc() ||
		SECT(ch->in_room) == ESector::kInside ||
		SECT(ch->in_room) == ESector::kCity ||
		ROOM_FLAGGED(ch->in_room, ERoomFlag::kIndoors) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoWeather))
		return (modi);

	sky = GET_ROOM_SKY(ch->in_room);

	switch (type) {
		case GAPPLY_SKILL_SUCCESS:
			switch (skillnum) {
				case ESkill::kUndefined:
					if (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark) {
						switch (sky) {
							case kSkyCloudless: modi = modi * 90 / 100;
								break;
							case kSkyCloudy: modi = modi * 80 / 100;
								break;
							case kSkyRaining: modi = modi * 70 / 30;
								break;
						}
					} else {
						switch (sky) {
							case kSkyCloudy: modi = modi * number(85, 95) / 100;
								break;
							case kSkyRaining: modi = modi * number(75, 85) / 100;
								break;
						}
					}
					break;
				default: break;
			}
			break;
	}
	if (IS_IMMORTAL(ch))
		modi = MAX(modi, value);
	return (modi);
}

int GetComplexSkillMod(CharData *ch, ESkill skillnum, int type, int value) {
	int modi = value;
	modi = day_skill_modifier(ch, skillnum, type, modi);
	modi = weather_skill_modifier(ch, skillnum, type, modi);
	return (modi);
}

int get_room_sky(int rnum) {
	return GET_ROOM_SKY(rnum);
}

// Calculate the MUD time passed over the last time_to-time_from centuries (secs) //
TimeInfoData *CalcMudTimePassed(time_t time_to, time_t time_from) {
	long secs;
	static TimeInfoData now;

	secs = (long) (time_to - time_from);

	now.hours = (secs / (kSecsPerMudHour * kTimeKoeff)) % kHoursPerDay;    // 0..23 hours //
	secs -= kSecsPerMudHour * kTimeKoeff * now.hours;

	now.day = (secs / (kSecsPerMudDay * kTimeKoeff)) % kDaysPerMonth;    // 0..29 days  //
	secs -= kSecsPerMudDay * kTimeKoeff * now.day;

	now.month = (secs / (kSecsPerMudMonth * kTimeKoeff)) % kMonthsPerYear;    // 0..11 months //
	secs -= kSecsPerMudMonth * kTimeKoeff * now.month;

	now.year = (secs / (kSecsPerMudYear * kTimeKoeff));    // 0..XX? years //

	return (&now);
}

TimeInfoData *CalcCharAge(const CharData *ch) {
	static TimeInfoData player_age;

	player_age = *CalcMudTimePassed(time(nullptr), ch->player_data.time.birth);

	player_age.year += 17;    // All players start at 17 //

	return (&player_age);
}

// * Calculate the REAL time passed over the last t2-t1 centuries (secs)
TimeInfoData *CalcRealTimePassed(time_t time_to, time_t time_from) {
	long secs;
	static TimeInfoData now;

	secs = (long) (time_to - time_from);

	now.hours = (secs / kSecsPerRealHour) % 24;    // 0..23 hours //
	secs -= kSecsPerRealHour * now.hours;

	now.day = (secs / kSecsPerRealDay);    // 0..34 days  //
	// secs -= SECS_PER_REAL_DAY * now.day; - Not used. //

	now.month = -1;
	now.year = -1;

	return (&now);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
