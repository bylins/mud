/**
 \file spell_control_weather.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellControlWeather manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/spells.h"
#include "engine/ui/interpreter.h"   // search_block

extern const char *what_sky_type[];   // defined in magic_utils.cpp (shared with dg scripts)

namespace handlers {

EStageResult SpellControlWeather(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	const char *sky_info = nullptr;
	int i, duration, zone, sky_type = 0;

	// Player casts pass the desired weather type as the spell argument; resolve it to what_sky here
	// (moved out of FindCastTarget). A programmatic cast (the thunderstorm tick) clears the argument
	// and relies on what_sky already being set, so empty -> keep the current value.
	if (cast_argument[0]) {
		const int idx = search_block(cast_argument, what_sky_type, false);
		if (idx < 0) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kControlWeather, ESpellMsg::kNoTarget) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
		what_sky = idx >> 1;
	}

	if (what_sky > kSkyLightning)
		what_sky = kSkyLightning;

	switch (what_sky) {
		case kSkyCloudless: sky_info = "Небо покрылось облаками.";
			break;
		case kSkyCloudy: sky_info = "Небо покрылось тяжелыми тучами.";
			break;
		case kSkyRaining:
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kOctober) {
				sky_info = "Начался проливной дождь.";
				SetPrecipitations(&sky_type, kWeatherLightrain, 0, 50, 50);
			} else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary) {
				sky_info = "Повалил снег.";
				SetPrecipitations(&sky_type, kWeatherLightsnow, 0, 50, 50);
			} else if (time_info.month == EMonth::kMarch || time_info.month == EMonth::kNovember) {
				if (weather_info.temperature > 2) {
					sky_info = "Начался проливной дождь.";
					SetPrecipitations(&sky_type, kWeatherLightrain, 0, 50, 50);
				} else {
					sky_info = "Повалил снег.";
					SetPrecipitations(&sky_type, kWeatherLightsnow, 0, 50, 50);
				}
			}
			break;
		case kSkyLightning: sky_info = "На небе не осталось ни единого облачка.";
			break;
		default: break;
	}

	if (sky_info) {
		duration = std::max(GetRealLevel(ch) / 8, 2);
		zone = world[ch->in_room]->zone_rn;
		for (i = kFirstRoom; i <= top_of_world; i++)
			if (world[i]->zone_rn == zone && SECT(i) != ESector::kInside && SECT(i) != ESector::kCity) {
				world[i]->weather.sky = what_sky;
				world[i]->weather.weather_type = sky_type;
				world[i]->weather.duration = duration;
				if (world[i]->first_character()) {
					act(sky_info, false, world[i]->first_character(), nullptr, nullptr, kToRoom | kToArenaListen);
					act(sky_info, false, world[i]->first_character(), nullptr, nullptr, kToChar);
				}
			}
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
