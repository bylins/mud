#ifndef __WEATHER_HPP__
#define __WEATHER_HPP__

#include "structs/structs.h"
#include "game_skills/skills.h" // ABYRVALG - вынести в скиллз_константс

// Sun state for Weather //
const __uint8_t SUN_DARK = 0;
const __uint8_t SUN_RISE = 1;
const __uint8_t SUN_LIGHT = 2;
const __uint8_t SUN_SET = 3;

// Moon change type //
const __uint8_t NEWMOONSTART = 27;
const __uint8_t NEWMOONSTOP = 1;
const __uint8_t HALFMOONSTART = 7;
const __uint8_t FULLMOONSTART = 13;
const __uint8_t FULLMOONSTOP = 15;
const __uint8_t LASTHALFMOONSTART = 21;
const __uint8_t MOON_CYCLE = 28;
const __uint8_t WEEK_CYCLE = 7;
const __uint8_t POLY_WEEK_CYCLE = 9;
const __uint8_t MOON_INCREASE = 0;
const __uint8_t MOON_DECREASE = 1;

// Sky conditions for Weather //
const __uint8_t kSkyCloudless = 0;
const __uint8_t kSkyCloudy = 1;
const __uint8_t kSkyRaining = 2;
const __uint8_t kSkyLightning = 3;

constexpr Bitvector WEATHER_QUICKCOOL = 1 << 0;
constexpr Bitvector WEATHER_QUICKHOT = 1 << 1;
constexpr Bitvector WEATHER_LIGHTRAIN = 1 << 2;
constexpr Bitvector WEATHER_MEDIUMRAIN = 1 << 3;
constexpr Bitvector WEATHER_BIGRAIN = 1 << 4;
constexpr Bitvector WEATHER_GRAD = 1 << 5;
constexpr Bitvector WEATHER_LIGHTSNOW = 1 << 6;
constexpr Bitvector WEATHER_MEDIUMSNOW = 1 << 7;
constexpr Bitvector WEATHER_BIGSNOW = 1 << 8;
constexpr Bitvector WEATHER_LIGHTWIND = 1 << 9;
constexpr Bitvector WEATHER_MEDIUMWIND = 1 << 10;
constexpr Bitvector WEATHER_BIGWIND = 1 << 11;

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
	int season;
	int week_day_mono;
	int week_day_poly;
};

extern Weather weather_info;

void weather_and_time(int mode);
int complex_skill_modifier(CharData *ch, ESkill skillnum, int type, int value);

#endif // __WEATHER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
