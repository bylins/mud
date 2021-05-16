#include "magic.rooms.hpp"

#include "spells.h"
#include "comm.h"
#include "spells.info.h"
#include "modify.h"
#include "db.h"
#include "utils.h"
#include "chars/character.h"
#include "magic.utils.hpp"
#include "magic.h" //Включено ради material_component_processing, который надо, по-хлорошему, вообще в отдеьный модуль.


#include <sstream>
#include <iomanip>

// Структуры и функции для работы с заклинаниями, обкастовывающими комнаты

extern int what_sky;

namespace RoomSpells {

const int TIME_SPELL_RUNE_LABEL = 300;

std::list<ROOM_DATA*> aff_room_list;

ROOM_DATA* findAffectedRoom(long casterID, int spellnum);
void showAffectedRooms(CHAR_DATA *ch);
void removeSingleRoomAffect(long casterID, int spellnum);
void handleRoomAffect(ROOM_DATA* room, CHAR_DATA* ch, const AFFECT_DATA<ERoomApplyLocation>::shared_ptr& aff);
void sendAffectOffMessageToRoom(int aff, room_rnum room);
void addRoom(ROOM_DATA* room);
int imposeSpellToRoom(int level, CHAR_DATA * ch , ROOM_DATA * room, int spellnum);
int getUniqueAffectDuration(long casterID, int spellnum);
void affect_room_join_fspell(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af);
void affect_room_join(ROOM_DATA * room, AFFECT_DATA<ERoomApplyLocation>& af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void affect_room_total(ROOM_DATA * room);
void affect_to_room(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af);
void affect_room_modify(ROOM_DATA * room, byte loc, sbyte mod, bitvector_t bitv, bool add);

void removeAffectFromRoom(ROOM_DATA* room, const ROOM_DATA::room_affects_list_t::iterator& affect) {
	if (room->affected.empty()) {
		log("ERROR: Attempt to remove affect from no affected room!");
		return;
	}
	affect_room_modify(room, (*affect)->location, (*affect)->modifier, (*affect)->bitvector, FALSE);
	room->affected.erase(affect);
	affect_room_total(room);
}


RoomAffectListIt findRoomAffect(ROOM_DATA* room, int type) {
	for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i) {
		const auto affect = *affect_i;
		if (affect->type == type) {
			return affect_i;
		}
	}
	return room->affected.end();
}

bool isZoneRoomAffected(int zoneVNUM, ESpell spell) {
    for (auto it = aff_room_list.begin(); it != aff_room_list.end(); ++it) {
		if ((*it)->zone == zoneVNUM && isRoomAffected(*it, spell)) {
			return true;
		}
	}
	return false;
}

bool isRoomAffected(ROOM_DATA * room, ESpell spell) {
	for (const auto& af : room->affected) {
		if (af->type == spell) {
			return true;
		}
	}
	return false;
}

void showAffectedRooms(CHAR_DATA *ch) {
	const int vnumCW = 7;
	const int spellCW = 25;
	const int casterCW = 21;
	const int timeCW = 10;
	constexpr int tableW = vnumCW + spellCW + casterCW + timeCW;
	std::stringstream buffer;
	buffer << " Список комнат под аффектами:" << std::endl
			<< " " << std::setfill('-') << std::setw(tableW) << "" << std::endl << std::setfill(' ')
			<< std::left << " " << std::setw(vnumCW) << "Vnum"
			<< std::setw(spellCW) << "Spell"
			<< std::setw(casterCW) << "Caster name"
			<< std::right << std::setw(timeCW) << "Time (s)"
			<< std::endl
			<< " " << std::setfill('-') << std::setw(tableW) << "" << std::endl << std::setfill(' ');
	for (const auto room : aff_room_list) {
		for (const auto& af : room->affected) {
			buffer << std::left << " " << std::setw(vnumCW) << room->number
					<< std::setw(spellCW) << spell_info[af->type].name
					<< std::setw(casterCW) << get_name_by_id(af->caster_id)
					<< std::right << std::setw(timeCW) << af->duration*2
					<< std::endl;
		}
	}
	page_string(ch->desc, buffer.str());
}

CHAR_DATA* find_char_in_room(long char_id, ROOM_DATA *room) {
	assert(room);
	for (const auto tch : room->people) {
		if (GET_ID(tch) == char_id) {
			return (tch);
		}
	}
	return nullptr;
}

ROOM_DATA* findAffectedRoom(long casterID, int spellnum) {
	for (const auto room : aff_room_list) {
		for (const auto& af : room->affected) {
			if (af->type == spellnum && af->caster_id == casterID) {
				return room;
			}
		}
	}
	return nullptr;
}

template<typename F>
int removeAffectFromRooms(int spellnum, const F& filter) {
	for (const auto room : aff_room_list) {
		const auto& affect = std::find_if(room->affected.begin(), room->affected.end(), filter);
		if (affect != room->affected.end()) {
				sendAffectOffMessageToRoom((*affect)->type, real_room(room->number));
				spellnum = (*affect)->type;
				removeAffectFromRoom(room, affect);
				return spellnum;
		}
	}
	return 0;
}

void removeSingleRoomAffect(long casterID, int spellnum) {
	auto filter =
		[&casterID, &spellnum](auto& af)
			{return (af->caster_id == casterID && af->type == spellnum);};
	removeAffectFromRooms(spellnum, filter);
}

int removeControlledRoomAffect(CHAR_DATA *ch) {
	long casterID = GET_ID(ch);
	auto filter =
		[&casterID](auto& af)
			{return (af->caster_id == casterID && IS_SET(spell_info[af->type].routines, MAG_NEED_CONTROL));};
	return removeAffectFromRooms(0, filter);
}

void sendAffectOffMessageToRoom(int affectType, room_rnum room) {
	if (affectType > 0 && affectType <= SPELLS_COUNT && *spell_wear_off_msg[affectType]) {
		send_to_room(spell_wear_off_msg[affectType], room, 0);
	};
}

void addRoom(ROOM_DATA* room) {
	const auto it = std::find(aff_room_list.begin(), aff_room_list.end(), room);
	if (it == aff_room_list.end())
		aff_room_list.push_back(room);
}

// Раз в 2 секунды идет вызов обработчиков аффектов//
void handleRoomAffect(ROOM_DATA* room, CHAR_DATA* ch, const AFFECT_DATA<ERoomApplyLocation>::shared_ptr& aff)
{
	// Аффект в комнате.
	// Проверяем на то что нам передали бяку в параметрах.
	assert(aff);
	assert(room);

	// Тут надо понимать что если закл наложит не один аффект а несколько
	// то обработчик будет вызываться за пульс именно столько раз.
	int spellnum = aff->type;

	switch (spellnum)
	{
	case SPELL_FORBIDDEN:
	case SPELL_ROOM_LIGHT:
		break;

	case SPELL_POISONED_FOG:
		if (ch)	{
			const auto people_copy = room->people;
			for (const auto tch : people_copy) {
				if (!call_magic(ch, tch, nullptr, nullptr, SPELL_POISON, GET_LEVEL(ch))) {
					aff->duration = 0;
					break;
				}
			}
		}

		break;

	case SPELL_METEORSTORM:
		send_to_char("Раскаленные громовые камни рушатся с небес!\r\n", ch);
		act("Раскаленные громовые камни рушатся с небес!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_THUNDERSTONE, ch->get_level());
		break;

	case SPELL_THUNDERSTORM:
		switch (aff->duration)
		{
		case 8:
			if (!call_magic(ch, nullptr, nullptr, nullptr, SPELL_CONTROL_WEATHER, GET_LEVEL(ch))) {
				aff->duration = 0;
				break;
			}
			what_sky = SKY_CLOUDY;
			send_to_char("Стремительно налетевшие черные тучи сгустились над вами.\r\n", ch);
			act("Стремительно налетевшие черные тучи сгустились над вами.\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			break;
		case 7:
			send_to_char("Раздался чудовищный раскат грома!\r\n", ch);
			act("Раздался чудовищный удар грома!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_DEAFNESS, ch->get_level());
			break;
		case 6:
			send_to_char("Порывы мокрого ледяного ветра обрушились из туч!\r\n", ch);
			act("Порывы мокрого ледяного ветра обрушились на вас!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_CONE_OF_COLD, ch->get_level());
			break;
		case 5:
			send_to_char("Из туч хлынул дождь кислоты!\r\n", ch);
			act("Из туч хлынул дождь кислоты!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_ACID, ch->get_level());
			break;
		case 4:
			send_to_char("Из туч ударили разряды молний!\r\n", ch);
			act("Из туч ударили разряды молний!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_LIGHTNING_BOLT, ch->get_level());
			break;
		case 3:
			send_to_char("Из тучи посыпались шаровые молнии!\r\n", ch);
			act("Из тучи посыпались шаровые молнии!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_CALL_LIGHTNING, ch->get_level());
			break;
		case 2:
			send_to_char("Буря завыла, закручиваясь в вихри!\r\n", ch);
			act("Буря завыла, закручиваясь в вихри!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_WHIRLWIND, ch->get_level());
			break;
		case 1:
			what_sky = SKY_CLOUDLESS;
			break;
		default:
			send_to_char("Из туч ударили разряды молний!\r\n", ch);
			act("Из туч ударили разряды молний!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_LIGHTNING_BOLT, ch->get_level());
		}
		break;

	case SPELL_EVARDS_BLACK_TENTACLES:
		send_to_char("Мертвые руки навей шарят в поисках добычи!\r\n", ch);
		act("Мертвые руки навей шарят в поисках добычи!\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		callMagicToArea(ch, nullptr, world[ch->in_room], SPELL_DAMAGE_SERIOUS, ch->get_level());
		break;

	default:
		log("ERROR: Try handle room affect for spell without handler!");
	}
}

void room_affect_update(void)
{
	CHAR_DATA *ch;
	int spellnum;

	for (std::list<ROOM_DATA*>::iterator it = aff_room_list.begin();it != aff_room_list.end();) {
		assert(*it);
		auto& affects = (*it)->affected;
		auto next_affect_i = affects.begin();
		for (auto affect_i = next_affect_i; affect_i != affects.end(); affect_i = next_affect_i) {
			++next_affect_i;
			const auto& affect = *affect_i;
			spellnum = affect->type;
			ch = nullptr;

			if (IS_SET(spell_info[spellnum].routines, MAG_CASTER_INROOM) || IS_SET(spell_info[spellnum].routines, MAG_CASTER_INWORLD)) {
				ch = find_char_in_room(affect->caster_id, *it);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (IS_SET(spell_info[spellnum].routines, MAG_CASTER_INWORLD_DELAY)) {
						//Если спелл с задержкой таймера - то обнулять не надо, даже если чара нет, просто тикаем таймером как обычно
						ch = find_char_in_room(affect->caster_id, *it);
					}

			if ((!ch) && IS_SET(spell_info[spellnum].routines, MAG_CASTER_INWORLD)) 	{
				ch = find_char(affect->caster_id);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (IS_SET(spell_info[spellnum].routines, MAG_CASTER_INWORLD_DELAY)) {
						ch = find_char(affect->caster_id);
					}

			if (!(ch && IS_SET(spell_info[spellnum].routines, MAG_CASTER_INWORLD_DELAY))) {
				switch (spellnum) {
				case SPELL_RUNE_LABEL:
					affect->duration--;
				}
			}

			if (affect->duration >= 1) {
				affect->duration--;
			// вот что это такое здесь ?
			} else if (affect->duration == -1) {
				affect->duration = -1;
			} else {
				if (affect->type > 0 && affect->type <= MAX_SPELLS) {
					if (next_affect_i == affects.end()
						|| (*next_affect_i)->type != affect->type
						|| (*next_affect_i)->duration > 0) {
							sendAffectOffMessageToRoom(affect->type, real_room((*it)->number));
					}
				}
				removeAffectFromRoom(*it, affect_i);
				continue;  // Чтоб не вызвался обработчик
			}

			// Учитываем что время выдается в пульсах а не в секундах  т.е. надо умножать на 2
			affect->apply_time++;
			if (affect->must_handled) {
				handleRoomAffect(*it, ch, affect);
			}
		}

		//если больше аффектов нет, удаляем комнату из списка обкастованных
		if ((*it)->affected.empty()) {
			it = aff_room_list.erase(it);
		//Инкремент итератора. Здесь, чтобы можно было удалять элементы списка.
		} else if (it != aff_room_list.end()) {
			++it;
		}
	}
}

// =============================================================== //

// Применение заклинания к комнате //
int imposeSpellToRoom(int/* level*/, CHAR_DATA * ch , ROOM_DATA * room, int spellnum) {
	bool accum_affect = FALSE, accum_duration = FALSE, success = TRUE;
	bool update_spell = FALSE;
	// Должен ли данный спелл быть только 1 в мире от этого кастера?
	bool only_one = FALSE;
	const char *to_char = nullptr;
	const char *to_room = nullptr;
	int i = 0, lag = 0;
	// Sanity check
	if (room == nullptr || ch == nullptr || ch->in_room == NOWHERE) {
		return 0;
	}

	AFFECT_DATA<ERoomApplyLocation> af[MAX_SPELL_AFFECTS];
	for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
		af[i].type = spellnum;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = APPLY_ROOM_NONE;
		af[i].caster_id = 0;
		af[i].must_handled = false;
		af[i].apply_time = 0;
	}

	switch (spellnum) {
	case SPELL_FORBIDDEN:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].duration = (1 + (GET_LEVEL(ch) + 14) / 15)*30;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_FORBIDDEN;
		af[0].must_handled = false;
		accum_duration = FALSE;
		update_spell = TRUE;
		if (IS_MANA_CASTER(ch)) {
			af[0].modifier = 80;
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
	case SPELL_ROOM_LIGHT:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		af[0].duration = pc_duration(ch, 0, GET_LEVEL(ch) + 5, 6, 0, 0);
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_LIGHT;
		af[0].must_handled = false;
		accum_duration = TRUE;
		update_spell = TRUE;
		to_char = "Вы облили комнату бензином и бросили окурок.";
		to_room = "Пространство вокруг начало светиться.";
		break;

	case SPELL_POISONED_FOG:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_POISON;
		af[0].modifier = 50;
		af[0].duration = pc_duration(ch, 0, GET_LEVEL(ch) + 5, 6, 0, 0);
		af[0].bitvector = AFF_ROOM_FOG; //Добаляет бит туман
		af[0].caster_id = GET_ID(ch);
		af[0].must_handled = true;
		update_spell = FALSE;
		to_room = "$n испортил$g воздух и плюнул$g в суп.";
		break;

	case SPELL_METEORSTORM:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		af[0].duration = 3;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_METEORSTORM;
		af[0].must_handled = true;
		accum_duration = FALSE;
		update_spell = FALSE;
		to_char = "Повинуясь вашей воле несшиеся в неизмеримой дали громовые камни обрушились на землю.";
		to_room = "$n воздел$g руки и с небес посыпался град раскаленных громовых камней.";
		break;

	case SPELL_THUNDERSTORM:
		af[0].type = spellnum;
		af[0].duration = 8;
		af[0].must_handled = true;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_THUNDERSTORM;
		update_spell = FALSE;
		to_char = "Вы ощутили в небесах силу бури и призвали ее к себе.";
		to_room = "$n проревел$g заклинание. Вы услышали раскаты далекой грозы.";
		break;

	case SPELL_RUNE_LABEL:
		if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || ROOM_FLAGGED(ch->in_room, ROOM_TUNNEL) || ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTIN)) {
			to_char = "Вы начертали свое имя рунами на земле, знаки вспыхнули, но ничего не произошло.";
			to_room = "$n начертил$g на земле несколько рун, знаки вспыхнули, но ничего не произошло.";
			lag = 2;
			break;
		}
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		af[0].duration = (TIME_SPELL_RUNE_LABEL + (GET_REMORT(ch)*10))*3;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_RUNE_LABEL;
		af[0].must_handled = false;
		accum_duration = TRUE;
		update_spell = FALSE;
		only_one = TRUE;
		to_char = "Вы начертали свое имя рунами на земле и произнесли заклинание.";
		to_room = "$n начертил$g на земле несколько рун и произнес$q заклинание.";
		lag = 2;
		break;

	case SPELL_HYPNOTIC_PATTERN:
		if (material_component_processing(ch, ch, spellnum)) {
			success = FALSE;
			break;
		}
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		af[0].duration = 30+(GET_LEVEL(ch)+GET_REMORT(ch))*dice(1,3);
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_HYPNOTIC_PATTERN;
		af[0].must_handled = false;
		accum_duration = FALSE;
		update_spell = FALSE;
		only_one = FALSE;
		to_char = "Вы воскурили благовония и пропели заклинание. В воздухе поплыл чарующий глаз огненный узор.";
		to_room = "$n воскурил$g благовония и пропел$g заклинание. В воздухе поплыл чарующий глаз огненный узор.";
		break;

	case SPELL_EVARDS_BLACK_TENTACLES:
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MONO) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_POLY)) {
			success = FALSE;
			break;
		}
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		af[0].duration = 1+GET_LEVEL(ch)/7;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_EVARDS_BLACK_TENTACLES;
		af[0].must_handled = true;
		accum_duration = FALSE;
		update_spell = FALSE;
		to_char = "Вы выкрикнули несколько мерзко звучащих слов и притопнули.\r\nИз-под ваших ног полезли скрюченные мертвые руки.";
		to_room = "$n выкрикнул$g несколько мерзко звучащих слов и притопнул$g.\r\nИз-под ваших ног полезли скрюченные мертвые руки.";
		break;
	}
	if (success) {
		if (IS_SET(SpINFO.routines, MAG_NEED_CONTROL)) {
			int SplFound = removeControlledRoomAffect(ch);
			if (SplFound) {
				send_to_char(ch, "Вы прервали заклинание !%s! и приготовились применить !%s!\r\n", spell_info[SplFound].name, SpINFO.name);
			}
		} else {
			auto RoomAffect_i = findRoomAffect(room, spellnum);
			const auto RoomAffect = RoomAffect_i != room->affected.end() ? *RoomAffect_i : nullptr;
			if (RoomAffect && RoomAffect->caster_id == GET_ID(ch) && !update_spell) {
				success = false;
			}
			else if (only_one) {
				removeSingleRoomAffect(GET_ID(ch), spellnum);
			}
		}
	}

	// Перебираем заклы чтобы понять не производиться ли рефрешь закла
	for (i = 0; success && i < MAX_SPELL_AFFECTS; i++)
	{
		af[i].type = spellnum;
		if (af[i].bitvector
			|| af[i].location != APPLY_ROOM_NONE
			|| af[i].must_handled)
		{
			af[i].duration = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (update_spell)
			{
				affect_room_join_fspell(room, af[i]);
			}
			else
			{
				affect_room_join(room, af[i], accum_duration, FALSE, accum_affect, FALSE);
			}
			//Вставляем указатель на комнату в список обкастованных, с проверкой на наличие
			//Здесь - потому что все равно надо проверять, может это не первый спелл такого типа на руме
			addRoom(room);
		}
	}

	if (success)
	{
		if (to_room != nullptr)
			act(to_room, TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		if (to_char != nullptr)
			act(to_char, TRUE, ch, 0, 0, TO_CHAR);
		return 1;
	} else
		send_to_char(NOEFFECT, ch);

	if (!WAITLESS(ch))
		WAIT_STATE(ch, lag * PULSE_VIOLENCE);

	return 0;

}

int getUniqueAffectDuration(long casterID, int spellnum) {
	for (const auto& room : aff_room_list) {
		for (const auto& af : room->affected) {
			if (af->type == spellnum && af->caster_id == casterID) {
				return af->duration;
			}
		}
	}
	return 0;
}

void affect_room_join_fspell(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af)
{
	bool found = FALSE;

	for (const auto& hjp : room->affected)
	{
		if (hjp->type == af.type
			&& hjp->location == af.location)
		{
			if (hjp->modifier < af.modifier)
			{
				hjp->modifier = af.modifier;
			}

			if (hjp->duration < af.duration)
			{
				hjp->duration = af.duration;
			}

			affect_room_total(room);
			found = true;
			break;
		}
	}

	if (!found)
	{
		affect_to_room(room, af);
	}
}

void affect_room_join(ROOM_DATA * room, AFFECT_DATA<ERoomApplyLocation>& af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
	bool found = false;

	if (af.location)
	{
		for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i)
		{
			const auto& affect = *affect_i;
			if (affect->type == af.type
				&& affect->location == af.location)
			{
				if (add_dur)
				{
					af.duration += affect->duration;
				}

				if (avg_dur)
				{
					af.duration /= 2;
				}

				if (add_mod)
				{
					af.modifier += affect->modifier;
				}

				if (avg_mod)
				{
					af.modifier /= 2;
				}

				removeAffectFromRoom(room, affect_i);
				affect_to_room(room, af);

				found = true;
				break;
			}
		}
	}

	if (!found)
	{
		affect_to_room(room, af);
	}
}

// Тут осуществляется апдейт аффектов влияющих на комнату
void affect_room_total(ROOM_DATA * room)
{
	// А че тут надо делать пересуммирование аффектов от заклов.
	/* Вобщем все комнаты имеют (вроде как базовую и
	   добавочную характеристику) если скажем ввести
	   возможность устанавливать степень зараженности комнтаы
	   в OLC , то дополнительное загаживание только будет вносить
	   + но не обнулять это значение. */

	// обнуляем все добавочные характеристики
	memset(&room->add_property, 0 , sizeof(room_property_data));

	// перенакладываем аффекты
	for (const auto& af : room->affected)
	{
		affect_room_modify(room, af->location, af->modifier, af->bitvector, TRUE);
	}
}

void affect_to_room(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af)
{
	AFFECT_DATA<ERoomApplyLocation>::shared_ptr new_affect(new AFFECT_DATA<ERoomApplyLocation>(af));

	room->affected.push_front(new_affect);

	affect_room_modify(room, af.location, af.modifier, af.bitvector, TRUE);
	affect_room_total(room);
}

void affect_room_modify(ROOM_DATA * room, byte loc, sbyte mod, bitvector_t bitv, bool add)
{
	if (add)
	{
		ROOM_AFF_FLAGS(room).set(bitv);
	}
	else
	{
		ROOM_AFF_FLAGS(room).unset(bitv);
		mod = -mod;
	}

	switch (loc)
	{
	case APPLY_ROOM_NONE:
		break;
	case APPLY_ROOM_POISON:
		// Увеличиваем загаженность от аффекта вызываемого SPELL_POISONED_FOG
		// Хотя это сделанно скорее для примера пока не обрабатывается вообще
		GET_ROOM_ADD_POISON(room) += mod;
		break;
	default:
		log("SYSERR: Unknown room apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
		break;

	}			// switch
}

} // namespace RoomSpells


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
