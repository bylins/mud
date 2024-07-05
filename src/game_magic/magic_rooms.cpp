#include "magic_rooms.h"

#include "spells_info.h"
#include "modify.h"
#include "entities/char_data.h"
#include "magic.h" //Включено ради material_component_processing
#include "utils/table_wrapper.h"
#include "structs/global_objects.h"
#include "game_skills/townportal.h"

//#include <iomanip>

// Структуры и функции для работы с заклинаниями, обкастовывающими комнаты

extern int what_sky;

namespace room_spells {

const int kRuneLabelDuration = 300;

std::list<RoomData *> affected_rooms;

void RemoveSingleRoomAffect(long caster_id, ESpell spell_id);
void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff);
void SendRemoveAffectMsgToRoom(ESpell affect_type, RoomRnum room);
void AddRoomToAffected(RoomData *room);
void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_join(RoomData *room, Affect<ERoomApply> &af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void AffectRoomJoinReplace(RoomData *room, const Affect<ERoomApply> &af);
void RefreshRoomAffects(RoomData *room);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_modify(RoomData *room, byte loc, sbyte mod, Bitvector bitv, bool add);
void RoomRemoveAffect(RoomData *room, const RoomAffectIt &affect) {

	if (room->affected.empty()) {
		log("ERROR: Attempt to remove affect from no affected room!");
		return;
	}
	ROOM_AFF_FLAGS(room).unset((*affect)->bitvector);
	room->affected.erase(affect);
	RefreshRoomAffects(room);
}

RoomAffectIt FindAffect(RoomData *room, ESpell type) {
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
	out << " Список комнат под аффектами:" << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader <<
		"#" << "Vnum" << "Spell" << "Caster name" << "Time (s)" << table_wrapper::kEndRow;
	int count = 1;
	for (const auto r : affected_rooms) {
		for (const auto &af : r->affected) {
			table << count << r->vnum << MUD::Spell(af->type).GetName()
				<< get_name_by_id(af->caster_id) << af->duration * 2 << table_wrapper::kEndRow;
			++count;
		}
	}
	table_wrapper::DecorateServiceTable(ch, table);
	out << table.to_string() << "\r\n";

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

RoomData *FindAffectedRoom(long caster_id, ESpell spell_id) {
	for (const auto room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->type == spell_id && af->caster_id == caster_id) {
				return room;
			}
		}
	}
	return nullptr;
}

template<typename F>
ESpell RemoveAffectFromRooms(ESpell spell_id, const F &filter) {
	for (const auto room : affected_rooms) {
		const auto &affect = std::find_if(room->affected.begin(), room->affected.end(), filter);
		if (affect != room->affected.end()) {
			SendRemoveAffectMsgToRoom((*affect)->type, real_room(room->vnum));
			spell_id = (*affect)->type;
			RoomRemoveAffect(room, affect);
			return spell_id;
		}
	}
	return ESpell::kUndefined;
}

void RemoveSingleRoomAffect(long caster_id, ESpell spell_id) {
	auto filter =
		[&caster_id, &spell_id](auto &af) { return (af->caster_id == caster_id && af->type == spell_id); };
	RemoveAffectFromRooms(spell_id, filter);
}

ESpell RemoveControlledRoomAffect(CharData *ch) {
	long casterID = GET_ID(ch);
	auto filter =
		[&casterID](auto &af) {
			return (af->caster_id == casterID && MUD::Spell(af->type).IsFlagged(kMagNeedControl));
		};
	return RemoveAffectFromRooms(ESpell::kUndefined, filter);
}

void SendRemoveAffectMsgToRoom(ESpell affect_type, RoomRnum room) {
	const std::string &msg = GetAffExpiredText(static_cast<ESpell>(affect_type));
	if (affect_type >= ESpell::kFirst && affect_type <= ESpell::kLast && !msg.empty()) {
		SendMsgToRoom(msg.c_str(), room, 0);
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
	auto spell_id = aff->type;

	switch (spell_id) {
		case ESpell::kForbidden:
		case ESpell::kRoomLight: break;

		case ESpell::kDeadlyFog:
			switch (aff->duration) {
				case 7:
					SendMsgToChar("Повинуясь вашему желанию отравить всех, туман начал густеть...\r\n", ch);
					act("Облако тумана созданное $n4 начало густеть, отравляя комнату...\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kPoison, GetRealLevel(ch));
					break;
				case 6:
					SendMsgToChar("Вы осознали, что хотите вызвать ужасные мучения у врагов...\r\nТуман тут же исполнил вашу прихоть...\r\n", ch);
					act("$n захрипел$g, завыл$g, и враги, вдыхающее и выдыхающее туман, стали корчиться от силы черной магии!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kFever, GetRealLevel(ch));
					break;
				case 5:
					SendMsgToChar("Что может быть лутше, чем слабый враг?!\r\nТолько мертвый!\r\n", ch);
					act("$n что-то проревел$g страшным голосом, и враги, окутанные туманом, стали быстро слабеть!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kWeaknes, GetRealLevel(ch));
					break;
				case 4:
					SendMsgToChar("Вам захотелось лишить всех глаз!\r\n", ch);
					act("Туман, вызванный $n4, начал слепить врагов, сгустившись еще сильнее!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kPowerBlindness, GetRealLevel(ch));
					break;
				case 3:
					SendMsgToChar("Сильно навредить врагам?!\r\nХорошая идея!\r\n", ch);
					act("$n пожелал$g, и туман уплотнился, чтобы навредить врагам!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kDamageCritic, GetRealLevel(ch));
					break;
				case 2:
					SendMsgToChar("Вам невтерпеж испить жизненной силы врагов!\r\nЧто и было исполнено.\r\n", ch);
					act("Туман высосал часть вражеских сил и отдал их $n2!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kSacrifice, GetRealLevel(ch));
					break;
				case 1: 
					SendMsgToChar("Вы осознали что кислоты мало не бывает!\r\nТуман повиновался.\r\n", ch);
					act("По воле $n1 из тумана вылетел сноп кислотных стрел!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kAcidArrow, GetRealLevel(ch));
					break;
				case 0: 
				default: 
					SendMsgToChar("Вы решили проклясть всех на последок!\r\n", ch);
					act("$n что-то прошептал$g напоследок, и туман навлек проклятие на врагов!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
						CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kMassCurse, GetRealLevel(ch));
					break;
			}
			break;
		

		case ESpell::kMeteorStorm: SendMsgToChar("Раскаленные громовые камни рушатся с небес!\r\n", ch);
			act("Раскаленные громовые камни рушатся с небес!\r\n",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kThunderStone, GetRealLevel(ch));
			break;

		case ESpell::kThunderstorm:
			switch (aff->duration) {
				case 7:
					if (!CallMagic(ch, nullptr, nullptr, nullptr, ESpell::kControlWeather, GetRealLevel(ch))) {
						aff->duration = 0;
						break;
					}
					what_sky = kSkyCloudy;
					SendMsgToChar("Стремительно налетевшие черные тучи сгустились над вами.\r\n", ch);
					act("Стремительно налетевшие черные тучи сгустились над вами.\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					break;
				case 6: SendMsgToChar("Раздался чудовищный раскат грома!\r\n", ch);
					act("Раздался чудовищный удар грома!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kDeafness, GetRealLevel(ch));
					break;
				case 5: SendMsgToChar("Порывы мокрого ледяного ветра обрушились из туч!\r\n", ch);
					act("Порывы мокрого ледяного ветра обрушились на вас!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kColdWind, GetRealLevel(ch));
					break;
				case 4: SendMsgToChar("Из туч хлынул дождь кислоты!\r\n", ch);
					act("Из туч хлынул дождь кислоты!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kAcid, GetRealLevel(ch));
					break;
				case 3: SendMsgToChar("Из туч ударили разряды молний!\r\n", ch);
					act("Из туч ударили разряды молний!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kLightingBolt, GetRealLevel(ch));
					break;
				case 2: SendMsgToChar("Из тучи посыпались шаровые молнии!\r\n", ch);
					act("Из тучи посыпались шаровые молнии!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kCallLighting, GetRealLevel(ch));
					break;
				case 1: SendMsgToChar("Буря завыла, закручиваясь в вихри!\r\n", ch);
					act("Буря завыла, закручиваясь в вихри!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kWhirlwind, GetRealLevel(ch));
					break;
				case 0: 
				default: 
					what_sky = kSkyCloudless;
					SendMsgToChar("Буря начала утихать.\r\n", ch);
					act("Буря начала утихать.\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					break;
			}
			break;

		case ESpell::kBlackTentacles: SendMsgToChar("Мертвые руки навей шарят в поисках добычи!\r\n", ch);
			act("Мертвые руки навей шарят в поисках добычи!\r\n",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			CallMagicToArea(ch, nullptr, world[ch->in_room], ESpell::kDamageSerious, GetRealLevel(ch));
			break;

		default: log("ERROR: Try handle room affect for spell without handler!");
	}
}

// раз в 2 секунды
void UpdateRoomsAffects() {
	CharData *ch;

	for (auto room = affected_rooms.begin(); room != affected_rooms.end();) {
		assert(*room);
		auto &affects = (*room)->affected;
		auto next_affect_i = affects.begin();
		for (auto affect_i = next_affect_i; affect_i != affects.end(); affect_i = next_affect_i) {
			++next_affect_i;
			const auto &affect = *affect_i;
			auto spell_id = affect->type;
			ch = nullptr;

			if (MUD::Spell(spell_id).IsFlagged(kMagCasterInroom) ||
				MUD::Spell(spell_id).IsFlagged(kMagCasterInworld)) {
				ch = find_char_in_room(affect->caster_id, *room);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (MUD::Spell(spell_id).IsFlagged(kMagCasterInworldDelay)) {
				//Если спелл с задержкой таймера - то обнулять не надо, даже если чара нет, просто тикаем таймером как обычно
				ch = find_char_in_room(affect->caster_id, *room);
			}

			if ((!ch) && MUD::Spell(spell_id).IsFlagged(kMagCasterInworld)) {
				ch = find_char(affect->caster_id);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (MUD::Spell(spell_id).IsFlagged(kMagCasterInworldDelay)) {
				ch = find_char(affect->caster_id);
			}

			if (!(ch && MUD::Spell(spell_id).IsFlagged(kMagCasterInworldDelay))) {
				switch (spell_id) {
					case ESpell::kRuneLabel: affect->duration--;
					default: break;
				}
			}

			if (affect->duration >= 1) {
				affect->duration--;
				// вот что это такое здесь ?
			} else if (affect->duration == -1) {
				affect->duration = -1;
			} else {
				if (affect->type >= ESpell::kFirst && affect->type <= ESpell::kLast) {
					if (next_affect_i == affects.end()
						|| (*next_affect_i)->type != affect->type
						|| (*next_affect_i)->duration > 0) {
						SendRemoveAffectMsgToRoom(affect->type, real_room((*room)->vnum));
					}
				}
				if (affect->type == ESpell::kPortalTimer) {
					(*room)->pkPenterUnique = 0;
					(*room)->portal_time = 0;
					OneWayPortal::remove(*room);
//					decay_portal((*room)->vnum);
				}
				RoomRemoveAffect(*room, affect_i);
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
int CallMagicToRoom(int/* level*/, CharData *ch, RoomData *room, ESpell spell_id) {
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
		af[i].type = spell_id;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = kNone;
		af[i].caster_id = 0;
		af[i].must_handled = false;
		af[i].apply_time = 0;
	}

	switch (spell_id) {
		case ESpell::kForbidden: af[0].type = spell_id;
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
				af[0].modifier = MIN(100, GetRealInt(ch) + MAX((GetRealInt(ch) - 30) * 4, 0));
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
		case ESpell::kRoomLight: af[0].type = spell_id;
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

		case ESpell::kDeadlyFog: af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 8;
			af[0].bitvector = ERoomAffect::kDeadlyFog;
			af[0].caster_id = GET_ID(ch);
			af[0].must_handled = true;
			update_spell = false;
			to_char = "Пробормотав злобные проклятия, вы вызвали смертельный ядовитый туман, покрывший все вокруг тёмным саваном.";
			to_room = "Пробормотав злобные проклятия, $n вызвал$g смертельный ядовитый туман, покрывший все вокруг тёмным саваном.";
			break;

		case ESpell::kMeteorStorm: af[0].type = spell_id;
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

		case ESpell::kThunderstorm: af[0].type = spell_id;
			af[0].duration = 7;
			af[0].must_handled = true;
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kThunderstorm;
			update_spell = false;
			to_char = "Вы ощутили в небесах силу бури и призвали ее к себе.";
			to_room = "$n проревел$g заклинание. Вы услышали раскаты далекой грозы.";
			break;

		case ESpell::kRuneLabel:
			if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful)
				|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kTunnel)
				|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTeleportIn)) {
				to_char = "Вы начертали свое имя рунами на земле, знаки вспыхнули, но ничего не произошло.";
				to_room = "$n начертил$g на земле несколько рун, знаки вспыхнули, но ничего не произошло.";
				lag = 2;
				break;
			}
			af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = (kRuneLabelDuration + (GetRealRemort(ch) * 10)) * 3;
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

		case ESpell::kHypnoticPattern:
			if (ProcessMatComponents(ch, ch, spell_id)) {
				success = false;
				break;
			}
			af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 30 + (GetRealLevel(ch) + GetRealRemort(ch)) * RollDices(1, 3);
			af[0].caster_id = GET_ID(ch);
			af[0].bitvector = ERoomAffect::kHypnoticPattern;
			af[0].must_handled = false;
			accum_duration = false;
			update_spell = false;
			only_one = false;
			to_char = "Вы воскурили благовония и пропели заклинание. В воздухе поплыл чарующий глаз огненный узор.";
			to_room = "$n воскурил$g благовония и пропел$g заклинание. В воздухе поплыл чарующий глаз огненный узор.";
			break;

		case ESpell::kBlackTentacles:
			if (ROOM_FLAGGED(IN_ROOM(ch), ERoomFlag::kForMono) || ROOM_FLAGGED(IN_ROOM(ch), ERoomFlag::kForPoly)) {
				success = false;
				break;
			}
			af[0].type = spell_id;
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
		default: break;
	}
	if (success) {
		if (MUD::Spell(spell_id).IsFlagged(kMagNeedControl)) {
			auto found_spell = RemoveControlledRoomAffect(ch);
			if (found_spell != ESpell::kUndefined) {
				SendMsgToChar(ch, "Вы прервали заклинание !%s! и приготовились применить !%s!\r\n",
							  MUD::Spell(found_spell).GetCName(), MUD::Spell(spell_id).GetCName());
			}
		} else {
			auto RoomAffect_i = FindAffect(room, spell_id);
			const auto RoomAffect = RoomAffect_i != room->affected.end() ? *RoomAffect_i : nullptr;
			if (RoomAffect && RoomAffect->caster_id == GET_ID(ch) && !update_spell) {
				success = false;
			} else if (only_one) {
				RemoveSingleRoomAffect(GET_ID(ch), spell_id);
			}
		}
	}

	// Перебираем заклы чтобы понять не производиться ли рефрешь закла
	for (i = 0; success && i < kMaxSpellAffects; i++) {
		af[i].type = spell_id;
		if (af[i].bitvector
			|| af[i].location != kNone
			|| af[i].must_handled) {
			af[i].duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[i].duration);
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
		SendMsgToChar(NOEFFECT, ch);

	if (!IS_IMMORTAL(ch))
		SetWaitState(ch, lag * kBattleRound);

	return 0;

}

int GetUniqueAffectDuration(long caster_id, ESpell spell_id) {
	for (const auto &room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->type == spell_id && af->caster_id == caster_id) {
				return af->duration;
			}
		}
	}
	return 0;
}

void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af) {
	bool found = false;

	for (const auto &hjp : room->affected) {
		if (hjp->type == af.type && hjp->location == af.location) {
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
void AffectRoomJoinReplace(RoomData *room, const Affect<ERoomApply> &af) {
	bool found = false;

	for (auto &affect_i : room->affected) {
		if (affect_i->type == af.type && affect_i->location == af.location) {
			affect_i->duration = af.duration;
			RefreshRoomAffects(room);
			found = true;
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
				RoomRemoveAffect(room, affect_i); //тут будет крешь, нужно RefreshRoomAffects см выше
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
