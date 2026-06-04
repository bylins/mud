/**
 \file spell_control_weather.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellControlWeather manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/magic/spells.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_internal.h"
#include "engine/db/global_objects.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/cmd/do_hire.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/skills/animal_master.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "engine/ui/color.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/minions.h"
#include <cmath>

namespace handlers {

EStageResult SpellControlWeather(CastContext &ctx) {
	CharData *ch = ctx.caster();
	const char *sky_info = nullptr;
	int i, duration, zone, sky_type = 0;

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
