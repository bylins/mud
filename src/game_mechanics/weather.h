#ifndef WEATHER_HPP_
#define WEATHER_HPP_

#include "structs/structs.h"
#include "game_skills/skills.h" // ABYRVALG - вынести в скиллз_константс

enum class ESeason {
	kWinter = 0,
	kSpring,
	kSummer,
	kAutumn
};

enum EMonth {
	kJanuary = 0,
	kFebruary,
	kMarch,
	kApril,
	kMay,
	kJune,
	kJuly,
	kAugust,
	kSeptember,
	kOctober,
	kNovember,
	kDecember
};

const int kHoursPerDay = 24;
const int kDaysPerWeek = 7;
const int kDaysPerMonth = 30;
const int kMonthsPerYear = 12;
const int kTimeKoeff = 2;
const int kSecsPerMudHour = 60;
constexpr int kSecsPerMudDay = kHoursPerDay*kSecsPerMudHour;
constexpr int kSecsPerMudMonth = kDaysPerMonth*kSecsPerMudDay;
constexpr int kSecsPerMudYear = kMonthsPerYear*kSecsPerMudMonth;

// Sun state for Weather //
const __uint8_t kSunDark = 1;
const __uint8_t kSunRise = 2;
const __uint8_t kSunLight = 3;
const __uint8_t kSunSet = 4;

// Moon change type //
const __uint8_t kNewMoonStart = 27;
const __uint8_t kNewMoonStop = 1;
const __uint8_t kHalfMoonStart = 7;
const __uint8_t kFullMoonStart = 13;
const __uint8_t kFullMoonStop = 15;
const __uint8_t kLastHalfMoonStart = 21;
const __uint8_t kMoonCycle = 28;
const __uint8_t kWeekCycle = 7;
const __uint8_t kPolyWeekCycle = 9;
const __uint8_t kMoonIncrease = 0;
const __uint8_t kMoonDecrease = 1;

// Sky conditions for Weather //
const __uint8_t kSkyCloudless = 0;
const __uint8_t kSkyCloudy = 1;
const __uint8_t kSkyRaining = 2;
const __uint8_t kSkyLightning = 3;

constexpr Bitvector kWeatherQuickcool = 1 << 0;
constexpr Bitvector kWeatherQuickhot = 1 << 1;
constexpr Bitvector kWeatherLightrain = 1 << 2;
constexpr Bitvector kWeatherMediumrain = 1 << 3;
constexpr Bitvector kWeatherBigrain = 1 << 4;
constexpr Bitvector kWeatherHail = 1 << 5;
constexpr Bitvector kWeatherLightsnow = 1 << 6;
constexpr Bitvector kWeatherMediumsnow = 1 << 7;
constexpr Bitvector kWeatherBigsnow = 1 << 8;
constexpr Bitvector kWeatherLightwind = 1 << 9;
constexpr Bitvector kWeatherMediumwind = 1 << 10;
constexpr Bitvector kWeatherBigwind = 1 << 11;

struct MonthTemperature {
	int min;
	int max;
	int med;
};

struct Weather {
	int hours_go;        // Time life from reboot //

	int pressure;        // How is the pressure ( Mb )            //
	int press_last_day;    // Average pressure last day             //
	int press_last_week;    // Average pressure last week            //

	int temperature;        // How is the temperature (C)            //
	int temp_last_day;        // Average temperature last day          //
	int temp_last_week;    // Average temperature last week         //

	int rainlevel;        // Level of water from rains             //
	int snowlevel;        // Level of snow                         //
	int icelevel;        // Level of ice                          //

	int weather_type;        // bitvector - some values for month     //

	int change;        // How fast and what way does it change. //
	int sky;            // How is the sky.   //
	int sunlight;        // And how much sun. //
	int moon_day;        // And how much moon //
	ESeason season;
	int week_day_mono;
	int week_day_poly;
};

extern Weather weather_info;

void weather_and_time(int mode);
int get_moon(int sky);
int GetComplexSkillMod(CharData *ch, ESkill skillnum, int type, int value);
int CalcDaySpellMod(CharData *ch, ESpell spell_id, int type, int value);
int CalcWeatherSpellMod(CharData *ch, ESpell spell_id, int type, int value);
int CalcComplexSpellMod(CharData *ch, ESpell spell_id, int type, int value);

#endif // WEATHER_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
