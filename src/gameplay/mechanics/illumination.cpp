/**
\file illumination.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Освещенность/затемнение комнат.
\detail Все, что связано со степенью освещения и затемнения комнат надо будет пернести сюда.
*/

#include "gameplay/mechanics/illumination.h"

#include "utils/utils.h"
#include "gameplay/magic/magic_rooms.h"
#include "engine/db/db.h"
#include "engine/entities/char_data.h"

extern bool IsWearingLight(CharData *ch);

bool is_dark(RoomRnum room) {
	double coef = 0.0;

	// если на комнате висит флаг всегда светло, то добавляем
	// +2 к коэф
	// issue.affect-migration: the room-light affect is kRoomLight (the kMagRoom spell). This used to check
	// ESpell::kLight (a char-side light spell that never produces a room affect), so room light never lit
	// a room -- fixed to kRoomLight.
	if (room_spells::IsRoomAffected(world[room], room_spells::ERoomAffect::kRoomLight))
		coef += 2.0;
	// если светит луна и комната !помещение и !город
	if ((SECT(room) != ESector::kInside) && (SECT(room) != ESector::kCity)
		&& ((world[room]->weather.duration > 0 ? world[room]->weather.sky : weather_info.sky) == kSkyLightning
			&& weather_info.moon_day >= kFullMoonStart
			&& weather_info.moon_day <= kFullMoonStop))
		coef += 1.0;

	// если ночь и мы не внутри и не в городе
	if ((SECT(room) != ESector::kInside) && (SECT(room) != ESector::kCity)
		&& ((weather_info.sunlight == kSunSet) || (weather_info.sunlight == kSunDark)))
		coef -= 1.0;
	// если на комнате флаг темно
	if (ROOM_FLAGGED(room, ERoomFlag::kDarked))
		coef -= 1.0;

	if (ROOM_FLAGGED(room, ERoomFlag::kAlwaysLit))
		coef += 200.0;

	// проверка на костер
	if (world[room]->fires)
		coef += 1.0;

	// проходим по всем чарам и смотрим на них аффекты тьма/свет/освещение

	for (const auto tmp_ch : world[room]->people) {
		// если на чаре есть освещение, например, шарик или лампа
		if (IsWearingLight(tmp_ch))
			coef += 1.0;
		// если на чаре аффект свет
		if (AFF_FLAGGED(tmp_ch, EAffect::kSingleLight))
			coef += 3.0;
		// освещение перекрывает 1 тьму!
		if (AFF_FLAGGED(tmp_ch, EAffect::kHolyLight))
			coef += 9.0;
		// Санка. Логично, что если чар светится ярким сиянием, то это сияние распространяется на комнату
		if (AFF_FLAGGED(tmp_ch, EAffect::kSanctuary))
			coef += 1.0;
		// Тьма. Сразу фигачим коэф 6.
		if (AFF_FLAGGED(tmp_ch, EAffect::kHolyDark))
			coef -= 6.0;
	}

	if (coef < 0) {
		return true;
	}
	return false;

}

bool IsDefaultDark(RoomRnum room_rnum) {
	return (ROOM_FLAGGED(room_rnum, ERoomFlag::kDarked) ||
		(SECT(room_rnum) != ESector::kInside &&
			SECT(room_rnum) != ESector::kCity &&
			(weather_info.sunlight == kSunSet ||
				weather_info.sunlight == kSunDark)));
}


// issue.handler-cleaning: char light source + room light bookkeeping (moved from handler).
bool IsWearingLight(CharData *ch) {
	bool wear_light = false;
	for (int wear_pos = 0; wear_pos < EEquipPos::kNumEquipPos; wear_pos++) {
		if (GET_EQ(ch, wear_pos)
			&& GET_EQ(ch, wear_pos)->get_type() == EObjType::kLightSource
			&& GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2)) {
			wear_light = true;
		}
	}
	return wear_light;
}

namespace {
}  // namespace

void CheckLight(CharData *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef) {
	if (ch->in_room == kNowhere) {
		return;
	}

	if (IsWearingLight(ch)) {
		if (was_equip == kLightNo) {
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light + koef);
		}
	} else {
		if (was_equip == kLightYes)
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light - koef);
	}

	if (AFF_FLAGGED(ch, EAffect::kSingleLight)) {
		if (was_single == kLightNo)
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light + koef);
	} else {
		if (was_single == kLightYes)
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light - koef);
	}

	if (AFF_FLAGGED(ch, EAffect::kHolyLight)) {
		if (was_holylight == kLightNo)
			world[ch->in_room]->glight = std::max(0, world[ch->in_room]->glight + koef);
	} else {
		if (was_holylight == kLightYes)
			world[ch->in_room]->glight = std::max(0, world[ch->in_room]->glight - koef);
	}

	if (AFF_FLAGGED(ch, EAffect::kHolyDark)) {
		if (was_holydark == kLightNo) {
			world[ch->in_room]->gdark = std::max(0, world[ch->in_room]->gdark + koef);
		}
	} else {
		if (was_holydark == kLightYes) {
			world[ch->in_room]->gdark = std::max(0, world[ch->in_room]->gdark - koef);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
