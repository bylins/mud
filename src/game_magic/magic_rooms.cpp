#include "magic_rooms.h"

#include "spells_info.h"
#include "modify.h"
#include "entities/char_data.h"
#include "magic.h" //Включено ради material_component_processing
#include "utils/table_wrapper.h"

//#include <iomanip>

// Структуры и функции для работы с заклинаниями, обкастовывающими комнаты

extern int what_sky;

namespace room_spells {

const int kRuneLabelDuration = 300;

std::list<RoomData *> affected_rooms;

void RemoveSingleRoomAffect(long casterID, int spellnum);
void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff);
void sendAffectOffMessageToRoom(int aff, RoomRnum room);
void AddRoomToAffected(RoomData *room);
void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_join(RoomData *room, Affect<ERoomApply> &af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void RefreshRoomAffects(RoomData *room);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_modify(RoomData *room, byte loc, sbyte mod, Bitvector bitv, bool add);

void RemoveAffect(RoomData *room, const RoomAffectIt &affect) {
	if (room->affected.empty()) {
		log("ERROR: Attempt to remove affect from no affected room!");
		return;
	}
	affect_room_modify(room, (*affect)->location, (*affect)->modifier, (*affect)->bitvector, false);
	room->affected.erase(affect);
	RefreshRoomAffects(room);
}

RoomAffectIt FindAffect(RoomData *room, int type) {
	for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i) {
		const auto affect = *affect_i;
		if (affect->type == type) {
			return affect_i;
		}
	}
	return room->affected.end();
}

bool IsZoneRoomAffected(int zone_vnum, ESpell spell) {
	for (auto & affected_room : affected_rooms) {
		if (affected_room->zone_rn == zone_vnum && IsRoomAffected(affected_room, spell)) {
			return true;
		}
	}
	return false;
}

bool IsRoomAffected(RoomData *room, ESpell spell) {
	for (const auto &af : room->affected) {
		if (af->type == spell) {
			return true;
		}
	}
	return false;
}

void ShowAffectedRooms(CharData *ch) {
	std::stringstream out;
	out << " Список комнат под аффектами:" << std::endl;

	fort::char_table table;
	table << fort::header << "#" << "Vnum" << "Spell" << "Caster name" << "Time (s)" << fort::endr;
	int count = 1;
	for (const auto r : affected_rooms) {
		for (const auto &af : r->affected) {
			table << count << r->room_vn << spell_info[af->type].name
				<< get_name_by_id(af->caster_id) << af->duration * 2 << fort::endr;
			++count;
		}
	}
	table_wrapper::DecorateServiceTable(ch, table);
	out << table.to_string() << std::endl;

	page_string(ch->desc, out.str());
}

CharData *find_char_in_room(long char_id, RoomData *room) {
	assert(room);
	for (const auto tch : room->people) {
		if (GET_ID(tch) == char_id) {
			return (tch);
		}
	}
	return nullptr;
}

RoomData *FindAffectedRoom(long caster_id, int spellnum) {
	for (const auto room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->type == spellnum && af->caster_id == caster_id) {
				return room;
			}
		}
	}
	return nullptr;
}

template<typename F>
int RemoveAffectFromRooms(int spellnum, const F &filter) {
	for (const auto room : affected_rooms) {
		const auto &affect = std::find_if(room->affected.begin(), room->affected.end(), filter);
		if (affect != room->affected.end()) {
			sendAffectOffMessageToRoom((*affect)->type, real_room(room->room_vn));
			spellnum = (*affect)->type;
			RemoveAffect(room, affect);
			return spellnum;
		}
	}
	return 0;
}

void RemoveSingleRoomAffect(long casterID, int spellnum) {
	auto filter =
		[&casterID, &spellnum](auto &af) { return (af->caster_id == casterID && af->type == spellnum); };
	RemoveAffectFromRooms(spellnum, filter);
}

int removeControlledRoomAffect(CharData *ch) {
	long casterID = GET_ID(ch);
	auto filter =
		[&casterID](auto &af) {
			return (af->caster_id == casterID && IS_SET(spell_info[af->type].routines, kMagNeedControl));
		};
	return RemoveAffectFromRooms(0, filter);
}

void sendAffectOffMessageToRoom(int affectType, RoomRnum room) {
	// TODO:" refactor and replace int affectType by ESpell
	const std::string &msg = get_wear_off_text(static_cast<ESpell>(affectType));
	if (affectType > 0 && affectType <= kSpellCount && !msg.empty()) {
		send_to_room(msg.c_str(), room, 0);
	};
}

void AddRoomToAffected(RoomData *room) {
	const auto it = std::find(affected_rooms.begin(), affected_rooms.end(), room);
	if (it == affected_rooms.end())
		affected_rooms.push_back(room);
}

// Раз в 2 секунды идет вызов обработчиков аффектов//
void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	// Аффект в комнате.
	// Проверяем на то что нам передали бяку в параметрах.
	assert(aff);
	assert(room);

	// Тут надо понимать что если закл наложит не один аффект а несколько
	// то обработчик будет вызываться за пульс именно столько раз.
	int spellnum = aff->type;

	switch (spellnum) {
		case kSpellForbidden:
		case kSpellRoomLight: break;

		case kSpellPoosinedFog:
			if (ch) {
				const auto people_copy = room->people;
				for (const auto tch : people_copy) {
					if (!CallMagic(ch, tch, nullptr, nullptr, kSpellPoison, GetRealLevel(ch))) {
						aff->duration = 0;
						break;
					}
				}
			}

			break;

		case kSpellMeteorStorm: send_to_char("Раскаленные громовые камни рушатся с небес!\r\n", ch);
			act("Раскаленные громовые камни рушатся с небес!\r\n",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellThunderStone, GetRealLevel(ch));
			break;

		case kSpellThunderstorm:
			switch (aff->duration) {
				case 8:
					if (!CallMagic(ch, nullptr, nullptr, nullptr, kSpellControlWeather, GetRealLevel(ch))) {
						aff->duration = 0;
						break;
					}
					what_sky = kSkyCloudy;
					send_to_char("Стремительно налетевшие черные тучи сгустились над вами.\r\n", ch);
					act("Стремительно налетевшие черные тучи сгустились над вами.\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					break;
				case 7: send_to_char("Раздался чудовищный раскат грома!\r\n", ch);
					act("Раздался чудовищный удар грома!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellDeafness, GetRealLevel(ch));
					break;
				case 6: send_to_char("Порывы мокрого ледяного ветра обрушились из туч!\r\n", ch);
					act("Порывы мокрого ледяного ветра обрушились на вас!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellColdWind, GetRealLevel(ch));
					break;
				case 5: send_to_char("Из туч хлынул дождь кислоты!\r\n", ch);
					act("Из туч хлынул дождь кислоты!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellAcid, GetRealLevel(ch));
					break;
				case 4: send_to_char("Из туч ударили разряды молний!\r\n", ch);
					act("Из туч ударили разряды молний!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellLightingBolt, GetRealLevel(ch));
					break;
				case 3: send_to_char("Из тучи посыпались шаровые молнии!\r\n", ch);
					act("Из тучи посыпались шаровые молнии!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellCallLighting, GetRealLevel(ch));
					break;
				case 2: send_to_char("Буря завыла, закручиваясь в вихри!\r\n", ch);
					act("Буря завыла, закручиваясь в вихри!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellWhirlwind, GetRealLevel(ch));
					break;
				case 1: what_sky = kSkyCloudless;
					break;
				default: send_to_char("Из туч ударили разряды молний!\r\n", ch);
					act("Из туч ударили разряды молний!\r\n", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellLightingBolt, GetRealLevel(ch));
			}
			break;

		case kSpellBlackTentacles: send_to_char("Мертвые руки навей шарят в поисках добычи!\r\n", ch);
			act("Мертвые руки навей шарят в поисках добычи!\r\n",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			CallMagicToArea(ch, nullptr, world[ch->in_room], kSpellDamageSerious, GetRealLevel(ch));
			break;

		default: log("ERROR: Try handle room affect for spell without handler!");
	}
}

void UpdateRoomsAffects() {
	CharData *ch;
	int spellnum;

	for (auto room = affected_rooms.begin(); room != affected_rooms.end();) {
		assert(*room);
		auto &affects = (*room)->affected;
		auto next_affect_i = affects.begin();
		for (auto affect_i = next_affect_i; affect_i != affects.end(); affect_i = next_affect_i) {
			++next_affect_i;
			const auto &affect = *affect_i;
			spellnum = affect->type;
			ch = nullptr;

			if (IS_SET(spell_info[spellnum].routines, kMagCasterInroom)
				|| IS_SET(spell_info[spellnum].routines, kMagCasterInworld)) {
				ch = find_char_in_room(affect->caster_id, *room);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (IS_SET(spell_info[spellnum].routines, kMagCasterInworldDelay)) {
				//Если спелл с задержкой таймера - то обнулять не надо, даже если чара нет, просто тикаем таймером как обычно
				ch = find_char_in_room(affect->caster_id, *room);
			}

			if ((!ch) && IS_SET(spell_info[spellnum].routines, kMagCasterInworld)) {
				ch = find_char(affect->caster_id);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (IS_SET(spell_info[spellnum].routines, kMagCasterInworldDelay)) {
				ch = find_char(affect->caster_id);
			}

			if (!(ch && IS_SET(spell_info[spellnum].routines, kMagCasterInworldDelay))) {
				switch (spellnum) {
					case kSpellRuneLabel: affect->duration--;
				}
			}

			if (affect->duration >= 1) {
				affect->duration--;
				// вот что это такое здесь ?
			} else if (affect->duration == -1) {
				affect->duration = -1;
			} else {
				if (affect->type > 0 && affect->type <= kSpellCount) {
					if (next_affect_i == affects.end()
						|| (*next_affect_i)->type != affect->type
						|| (*next_affect_i)->duration > 0) {
						sendAffectOffMessageToRoom(affect->type, real_room((*room)->room_vn));
					}
				}
				RemoveAffect(*room, affect_i);
				continue;  // Чтоб не вызвался обработчик
			}

			// Учитываем что время выдается в пульсах а не в секундах  т.е. надо умножать на 2
			affect->apply_time++;
			if (affect->must_handled) {
				HandleRoomAffect(*room, ch, affect);
			}
		}

		//если больше аффектов нет, удаляем комнату из списка обкастованных
		if ((*room)->affected.empty()) {
			room = affected_rooms.erase(room);
			//Инкремент итератора. Здесь, чтобы можно было удалять элементы списка.
		} else if (room != affected_rooms.end()) {
			++room;
		}
	}
}

// =============================================================== //

// Применение заклинания к комнате //
int ImposeSpellToRoom(int/* level*/, CharData *ch, RoomData *room, int spellnum) {
	bool accum_affect = false, accum_duration = false, success = true;
	bool update_spell = false;
	// Должен ли данный спелл быть только 1 в мире от этого кастера?
	bool only_one = false;
	const char *to_char = nullptr;
	const char *to_room = nullptr;
	int i = 0, lag = 0;
	// Sanity check
	if (room == nullptr || ch == nullptr || ch->in_room == kNowhere) {
		return 0;
	}

	Affect<ERoomApply> af[kMaxSpellAffects];
	for (i = 0; i < kMaxSpellAffects; i++) {
		af[i].type = spellnum;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = kNone;
		af[i].caster_id = 0;
		af[i].must_handled = false;
		af[i].apply_time = 0;
	}

	switch (spellnum) {
		case kSpellForbidden: af[0].type = spellnum;
			af[0].location = kNone;
			af[0].duration = (1 + (GetRealLevel(ch) + 14) / 15) * 30;
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kForbidden;
			af[0].must_handled = false;
			accum_duration = false;
			update_spell = true;
			if (IS_MANA_CASTER(ch)) {
				af[0].modifier = 95;
			} else {
				af[0].modifier = MIN(100, GET_REAL_INT(ch) + MAX((GET_REAL_INT(ch) - 30) * 4, 0));
			}
			if (af[0].modifier > 99) {
				to_char = "Вы запечатали магией все входы.";
				to_room = "$n запечатал$g магией все входы.";
			} else if (af[0].modifier > 79) {
				to_char = "Вы почти полностью запечатали магией все входы.";
				to_room = "$n почти полностью запечатал$g магией все входы.";
			} else {
				to_char = "Вы очень плохо запечатали магией все входы.";
				to_room = "$n очень плохо запечатал$g магией все входы.";
			}
			break;
		case kSpellRoomLight: af[0].type = spellnum;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = CalcDuration(ch, 0, GetRealLevel(ch) + 5, 6, 0, 0);
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kLight;
			af[0].must_handled = false;
			accum_duration = true;
			update_spell = true;
			to_char = "Вы облили комнату бензином и бросили окурок.";
			to_room = "Пространство вокруг начало светиться.";
			break;

		case kSpellPoosinedFog: af[0].type = spellnum;
			af[0].location = kPoison;
			af[0].modifier = 50;
			af[0].duration = CalcDuration(ch, 0, GetRealLevel(ch) + 5, 6, 0, 0);
			af[0].bitvector = ERoomAffect::kPoisonFog;
			af[0].caster_id = GET_ID(ch);
			af[0].must_handled = true;
			update_spell = false;
			to_room = "$n испортил$g воздух и плюнул$g в суп.";
			break;

		case kSpellMeteorStorm: af[0].type = spellnum;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 3;
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kMeteorstorm;
			af[0].must_handled = true;
			accum_duration = false;
			update_spell = false;
			to_char = "Повинуясь вашей воле, несшиеся в неизмеримой дали громовые камни обрушились на землю.";
			to_room = "$n воздел$g руки и с небес посыпался град раскаленных громовых камней.";
			break;

		case kSpellThunderstorm: af[0].type = spellnum;
			af[0].duration = 8;
			af[0].must_handled = true;
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kThunderstorm;
			update_spell = false;
			to_char = "Вы ощутили в небесах силу бури и призвали ее к себе.";
			to_room = "$n проревел$g заклинание. Вы услышали раскаты далекой грозы.";
			break;

		case kSpellRuneLabel:
			if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)
				|| ROOM_FLAGGED(ch->in_room, ROOM_TUNNEL)
				|| ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTIN)) {
				to_char = "Вы начертали свое имя рунами на земле, знаки вспыхнули, но ничего не произошло.";
				to_room = "$n начертил$g на земле несколько рун, знаки вспыхнули, но ничего не произошло.";
				lag = 2;
				break;
			}
			af[0].type = spellnum;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = (kRuneLabelDuration + (GET_REAL_REMORT(ch) * 10)) * 3;
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kRuneLabel;
			af[0].must_handled = false;
			accum_duration = false;
			update_spell = true;
			only_one = true;
			to_char = "Вы начертали свое имя рунами на земле и произнесли заклинание.";
			to_room = "$n начертил$g на земле несколько рун и произнес$q заклинание.";
			lag = 2;
			break;

		case kSpellHypnoticPattern:
			if (material_component_processing(ch, ch, spellnum)) {
				success = false;
				break;
			}
			af[0].type = spellnum;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 30 + (GetRealLevel(ch) + GET_REAL_REMORT(ch)) * RollDices(1, 3);
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kHypnoticPattern;
			af[0].must_handled = false;
			accum_duration = false;
			update_spell = false;
			only_one = false;
			to_char = "Вы воскурили благовония и пропели заклинание. В воздухе поплыл чарующий глаз огненный узор.";
			to_room = "$n воскурил$g благовония и пропел$g заклинание. В воздухе поплыл чарующий глаз огненный узор.";
			break;

		case kSpellBlackTentacles:
			if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MONO) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_POLY)) {
				success = false;
				break;
			}
			af[0].type = spellnum;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 1 + GetRealLevel(ch) / 7;
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kBlackTentacles;
			af[0].must_handled = true;
			accum_duration = false;
			update_spell = false;
			to_char =
				"Вы выкрикнули несколько мерзко звучащих слов и притопнули.\r\n"
				"Из-под ваших ног полезли скрюченные мертвые руки.";
			to_room =
				"$n выкрикнул$g несколько мерзко звучащих слов и притопнул$g.\r\n"
				"Из-под ваших ног полезли скрюченные мертвые руки.";
			break;
	}
	if (success) {
		if (IS_SET(SpINFO.routines, kMagNeedControl)) {
			int SplFound = removeControlledRoomAffect(ch);
			if (SplFound) {
				send_to_char(ch,
							 "Вы прервали заклинание !%s! и приготовились применить !%s!\r\n",
							 spell_info[SplFound].name,
							 SpINFO.name);
			}
		} else {
			auto RoomAffect_i = FindAffect(room, spellnum);
			const auto RoomAffect = RoomAffect_i != room->affected.end() ? *RoomAffect_i : nullptr;
			if (RoomAffect && RoomAffect->caster_id == GET_ID(ch) && !update_spell) {
				success = false;
			} else if (only_one) {
				RemoveSingleRoomAffect(GET_ID(ch), spellnum);
			}
		}
	}

	// Перебираем заклы чтобы понять не производиться ли рефрешь закла
	for (i = 0; success && i < kMaxSpellAffects; i++) {
		af[i].type = spellnum;
		if (af[i].bitvector
			|| af[i].location != kNone
			|| af[i].must_handled) {
			af[i].duration = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (update_spell) {
				affect_room_join_fspell(room, af[i]);
			} else {
				affect_room_join(room, af[i], accum_duration, false, accum_affect, false);
			}
			//Вставляем указатель на комнату в список обкастованных, с проверкой на наличие
			//Здесь - потому что все равно надо проверять, может это не первый спелл такого типа на руме
			AddRoomToAffected(room);
		}
	}

	if (success) {
		if (to_room != nullptr)
			act(to_room, true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		if (to_char != nullptr)
			act(to_char, true, ch, nullptr, nullptr, kToChar);
		return 1;
	} else
		send_to_char(NOEFFECT, ch);

	if (!WAITLESS(ch))
		WAIT_STATE(ch, lag * kPulseViolence);

	return 0;

}

int GetUniqueAffectDuration(long caster_id, int spellnum) {
	for (const auto &room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->type == spellnum && af->caster_id == caster_id) {
				return af->duration;
			}
		}
	}
	return 0;
}

void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af) {
	bool found = false;

	for (const auto &hjp : room->affected) {
		if (hjp->type == af.type
			&& hjp->location == af.location) {
			if (hjp->modifier < af.modifier) {
				hjp->modifier = af.modifier;
			}

			if (hjp->duration < af.duration) {
				hjp->duration = af.duration;
			}

			RefreshRoomAffects(room);
			found = true;
			break;
		}
	}

	if (!found) {
		affect_to_room(room, af);
	}
}

void affect_room_join(RoomData *room, Affect<ERoomApply> &af,
					  bool add_dur, bool avg_dur, bool add_mod, bool avg_mod) {
	bool found = false;

	if (af.location) {
		for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i) {
			const auto &affect = *affect_i;
			if (affect->type == af.type
				&& affect->location == af.location) {
				if (add_dur) {
					af.duration += affect->duration;
				}

				if (avg_dur) {
					af.duration /= 2;
				}

				if (add_mod) {
					af.modifier += affect->modifier;
				}

				if (avg_mod) {
					af.modifier /= 2;
				}

				RemoveAffect(room, affect_i);
				affect_to_room(room, af);

				found = true;
				break;
			}
		}
	}

	if (!found) {
		affect_to_room(room, af);
	}
}

// Тут осуществляется апдейт аффектов влияющих на комнату
void RefreshRoomAffects(RoomData *room) {
	// А че тут надо делать пересуммирование аффектов от заклов.
	/* Вобщем все комнаты имеют (вроде как базовую и
	   добавочную характеристику) если скажем ввести
	   возможность устанавливать степень зараженности комнтаы
	   в OLC , то дополнительное загаживание только будет вносить
	   + но не обнулять это значение. */

	// обнуляем все добавочные характеристики
	memset(&room->add_property, 0, sizeof(RoomState));

	// перенакладываем аффекты
	for (const auto &af : room->affected) {
		affect_room_modify(room, af->location, af->modifier, af->bitvector, true);
	}
}

void affect_to_room(RoomData *room, const Affect<ERoomApply> &af) {
	Affect<ERoomApply>::shared_ptr new_affect(new Affect<ERoomApply>(af));

	room->affected.push_front(new_affect);

	affect_room_modify(room, af.location, af.modifier, af.bitvector, true);
	RefreshRoomAffects(room);
}

void affect_room_modify(RoomData *room, byte loc, sbyte mod, Bitvector bitv, bool add) {
	if (add) {
		ROOM_AFF_FLAGS(room).set(bitv);
	} else {
		ROOM_AFF_FLAGS(room).unset(bitv);
		mod = -mod;
	}

	switch (loc) {
		case kNone: break;
		case kPoison:
			// Увеличиваем загаженность от аффекта вызываемого SPELL_POISONED_FOG
			// Хотя это сделанно скорее для примера пока не обрабатывается вообще
			GET_ROOM_ADD_POISON(room) += mod;
			break;
		default: log("SYSERR: Unknown room apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
			break;

	}            // switch
}

} // namespace room_spells


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
