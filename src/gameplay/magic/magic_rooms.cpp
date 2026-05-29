#include "magic_rooms.h"

#include "spells_info.h"
#include "engine/ui/modify.h"
#include "engine/entities/char_data.h"
#include "magic.h" //Включено ради material_component_processing
#include "engine/ui/table_wrapper.h"
#include "engine/db/global_objects.h"
#include "gameplay/skills/townportal.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "engine/db/player_index.h"

// Структуры и функции для работы с заклинаниями, обкастовывающими комнаты

extern int what_sky;

namespace room_spells {

const int kRuneLabelDuration = 300;

std::list<RoomData *> affected_rooms;

void RemoveSingleRoomAffect(long caster_id, ESpell spell_id);
void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff);
void SendRemoveAffectMsgToRoom(ESpell affect_type, RoomRnum room);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);

void RoomRemoveAffect(RoomData *room, const RoomAffectIt &affect) {
	if (room->affected.empty()) {
		log("ERROR: Attempt to remove affect from no affected room!");
		return;
	}
	room->affected.erase(affect);
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
				  << GetNameById(af->caster_id) << af->duration * 2 << table_wrapper::kEndRow;
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
		if (tch->get_uid() == char_id) {
			return (tch);
		}
	}
	return nullptr;
}

RoomData *FindAffectedRoomByCasterID(long caster_id, ESpell spell_id) {
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
			SendRemoveAffectMsgToRoom((*affect)->type, GetRoomRnum(room->vnum));
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
	long casterID = ch->get_uid();
	auto filter =
		[&casterID](auto &af) {
			return (af->caster_id == casterID && MUD::Spell(af->type).IsFlagged(kMagNeedControl));
		};
	return RemoveAffectFromRooms(ESpell::kUndefined, filter);
}

void SendRemoveAffectMsgToRoom(ESpell affect_type, RoomRnum room) {
	const std::string &msg = GetAffExpiredText(static_cast<ESpell>(affect_type));
	if (affect_type >= ESpell::kFirst && affect_type <= ESpell::kLast && !msg.empty()) {
/*		if (affect_type == ESpell::kPortalTimer){
			mudlog("Пентаграмма медленно растаяла.");
		}
*/
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
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kPoison, GetRealLevel(ch)));
					break;
				case 6:
					SendMsgToChar("Вы осознали, что хотите вызвать ужасные мучения у врагов...\r\nТуман тут же исполнил вашу прихоть...\r\n", ch);
					act("$n захрипел$g, завыл$g, и враги, вдыхающие и выдыхающие туман, стали корчиться от силы черной магии!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kFever, GetRealLevel(ch)));
					break;
				case 5:
					SendMsgToChar("Что может быть лучше, чем слабый враг?!\r\nТолько мертвый!\r\n", ch);
					act("$n что-то проревел$g страшным голосом, и враги, окутанные туманом, стали быстро слабеть!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kWeaknes, GetRealLevel(ch)));
					break;
				case 4:
					SendMsgToChar("Вам захотелось лишить всех глаз!\r\n", ch);
					act("Туман, вызванный $n4, начал слепить врагов, сгустившись еще сильнее!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kPowerBlindness, GetRealLevel(ch)));
					break;
				case 3:
					SendMsgToChar("Сильно навредить врагам?!\r\nХорошая идея!\r\n", ch);
					act("$n пожелал$g, и туман уплотнился, чтобы навредить врагам!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kDamageCritic, GetRealLevel(ch)));
					break;
				case 2:
					SendMsgToChar("Вам невтерпеж испить жизненной силы врагов!\r\nЧто и было исполнено.\r\n", ch);
					act("Туман высосал часть вражеских сил и отдал их $n2!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kSacrifice, GetRealLevel(ch)));
					break;
				case 1: 
					SendMsgToChar("Вы осознали что кислоты мало не бывает!\r\nТуман повиновался.\r\n", ch);
					act("По воле $n1 из тумана вылетел сноп кислотных стрел!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kAcidArrow, GetRealLevel(ch)));
					break;
				case 0: 
				default: 
					SendMsgToChar("Вы решили проклясть всех напоследок!\r\n", ch);
					act("$n что-то прошептал$g напоследок, и туман навлек проклятие на врагов!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
						CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kMassCurse, GetRealLevel(ch)));
					break;
			}
			break;
		

		case ESpell::kMeteorStorm: SendMsgToChar("Раскаленные громовые камни рушатся с небес!\r\n", ch);
			act("Раскаленные громовые камни рушатся с небес!\r\n",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kThunderStone, GetRealLevel(ch)));
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
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kDeafness, GetRealLevel(ch)));
					break;
				case 5: SendMsgToChar("Порывы мокрого ледяного ветра обрушились из туч!\r\n", ch);
					act("Порывы мокрого ледяного ветра обрушились на вас!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kColdWind, GetRealLevel(ch)));
					break;
				case 4: SendMsgToChar("Из туч хлынул дождь кислоты!\r\n", ch);
					act("Из туч хлынул дождь кислоты!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kAcid, GetRealLevel(ch)));
					break;
				case 3: SendMsgToChar("Из туч ударили разряды молний!\r\n", ch);
					act("Из туч ударили разряды молний!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kLightingBolt, GetRealLevel(ch)));
					break;
				case 2: SendMsgToChar("Из тучи посыпались шаровые молнии!\r\n", ch);
					act("Из тучи посыпались шаровые молнии!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kCallLighting, GetRealLevel(ch)));
					break;
				case 1: SendMsgToChar("Буря завыла, закручиваясь в вихри!\r\n", ch);
					act("Буря завыла, закручиваясь в вихри!\r\n",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kWhirlwind, GetRealLevel(ch)));
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
			CallMagicToArea(ch, nullptr, world[ch->in_room], ComputeCastRoll(ch, ESpell::kDamageSerious, GetRealLevel(ch)));
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
					case ESpell::kRuneLabel: 
					affect->duration--;
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
						SendRemoveAffectMsgToRoom(affect->type, GetRoomRnum((*room)->vnum));
					}
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
int CallMagicToRoom(CharData *ch, RoomData *room, CastRollResult roll) {
	const ESpell spell_id = roll.spell_id;   // level is unused by room casts
	bool success = true;
	// Должен ли данный спелл быть только 1 в мире от этого кастера?
	// (issue.room-affects: the old accum_duration / update_spell local bools migrated to
	// per-affect battleflag bits -- kAfAccumulateDuration / kAfUpdateDuration -- read by
	// the impose loop below. `only_one` stays a per-call local because it isn't an affect
	// property but a "remove this caster's prior cast of this spell" flag.)
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
		af[i].affect_type = static_cast<ERoomAffect>(0);
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = kNone;
		af[i].caster_id = 0;
		af[i].must_handled = false;
		af[i].apply_time = 0;
		af[i].duration = 0;
	}

	switch (spell_id) {
		case ESpell::kForbidden: {
			af[0].type = spell_id;
			af[0].location = kNone;
			af[0].duration = (1 + (GetRealLevel(ch) + 14) / 15) * 30;
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = false;
			af[0].battleflag = kAfUpdateDuration;        // was: update_spell = true
			// Modifier (issue.no-affects-bug / room kForbidden migration): approximate the
			// OLD MIN(100, Int + MAX((Int-30)*4, 0)) formula via the spell's <potency_roll>.
			// CalcBaseStatCoeff with kInt threshold=0 weight=100 yields the linear "Int" part;
			// CalcSkillCoeff with kMindMagic hi_skill_bonus=40 approximates the +4-per-Int
			// kicker (since skill cap and Int grow together with remort). Mana-casters stay at
			// the OLD constant 95.
			if (IS_MANA_CASTER(ch)) {
				af[0].modifier = 95;
			} else {
				const auto &potency = MUD::Spell(spell_id).GetPotencyRoll();
				const double sum = potency.CalcSkillCoeff(ch) + potency.CalcBaseStatCoeff(ch);
				af[0].modifier = std::min(100, static_cast<int>(std::ceil(sum)));
			}
			// Three-tier seal-quality message stays code-set (modifier-dependent narration).
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
		}
		case ESpell::kRoomLight: af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			// issue.calc-duration: bind to the caster's kLightMagic skill instead of raw level.
			// At skill 75 (cap), 75/15 = 5h, matching the old level-30 ceiling. There's no separate
			// "victim" for a room affect, so the caster (ch) sets the unit.
			af[0].duration = CalcDuration(ch, ch, ESkill::kLightMagic, 0, 15, 0, 0);
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = false;
			af[0].battleflag = kAfAccumulateDuration | kAfUpdateDuration;
			// to_char / to_room messages now come from spell_msg.xml under the kAffImposed*
			// keys (issue.room-affects).
			break;

		case ESpell::kDeadlyFog: af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 8;
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = true;
			// battleflag stays 0: no accumulation, no refresh.
			break;

		case ESpell::kMeteorStorm: af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 3;
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = true;
			break;

		case ESpell::kThunderstorm: af[0].type = spell_id;
			af[0].duration = 7;
			af[0].must_handled = true;
			af[0].caster_id = ch->get_uid();
			break;

		case ESpell::kRuneLabel:
			if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful)
				|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kTunnel)
				|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTeleportIn)) {
				// "Fizzled" variant stays code-set (room-flag-specific narration; the
				// affect is not imposed and the XML kAffImposed* messages are for the
				// success path only).
				to_char = "Вы начертали свое имя рунами на земле, знаки вспыхнули, но ничего не произошло.";
				to_room = "$n начертил$g на земле несколько рун, знаки вспыхнули, но ничего не произошло.";
				lag = 2;
				break;
			}
			af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = (kRuneLabelDuration + (GetRealRemort(ch) * 10)) * 3;
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = false;
			af[0].battleflag = kAfUpdateDuration;
			only_one = true;
			lag = 2;
			break;

		case ESpell::kHypnoticPattern:
			if (ProcessMatComponents(ch, ch, spell_id) == EStageResult::kBreak) {
				success = false;
				break;
			}
			af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 30 + (GetRealLevel(ch) + GetRealRemort(ch)) * RollDices(1, 3);
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = false;
			break;

		case ESpell::kBlackTentacles:
			if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kForMono) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kForPoly)) {
				success = false;
				break;
			}
			af[0].type = spell_id;
			af[0].location = kNone;
			af[0].modifier = 0;
			af[0].duration = 1 + GetRealLevel(ch) / 7;
			af[0].caster_id = ch->get_uid();
			af[0].must_handled = true;
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
			// Refresh allowed iff this spell's first affect carries kAfUpdateDuration; otherwise
			// re-casting your own affect is a no-op (preserves OLD update_spell semantics now
			// that the bool was migrated to a per-affect battleflag bit).
			const bool refresh_allowed = IS_SET(af[0].battleflag, kAfUpdateDuration);
			if (RoomAffect && RoomAffect->caster_id == ch->get_uid() && !refresh_allowed) {
				success = false;
			} else if (only_one) {
				RemoveSingleRoomAffect(ch->get_uid(), spell_id);
			}
		}
	}

	// Перебираем заклы чтобы понять не производиться ли рефрешь закла. Each affect's
	// battleflag drives the join policy (issue.room-affects: was local update_spell /
	// accum_duration bools shared across all kMaxSpellAffects entries; now per-affect).
	for (i = 0; success && i < kMaxSpellAffects; i++) {
		af[i].type = spell_id;
		if (af[i].duration
			|| af[i].location != kNone
			|| af[i].must_handled) {
			af[i].duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (IS_SET(af[i].battleflag, kAfUpdateDuration)) {
				affect_room_join_fspell(room, af[i]);
			} else {
				const bool accum_duration = IS_SET(af[i].battleflag, kAfAccumulateDuration);
				affect_room_join(room, af[i], accum_duration, false, false, false);
			}
			//Вставляем указатель на комнату в список обкастованных, с проверкой на наличие
			//Здесь - потому что все равно надо проверять, может это не первый спелл такого типа на руме
			AddRoomToAffected(room);
		}
	}

	if (success) {
		// Code-set messages win (kForbidden's modifier-tier text, kRuneLabel's "fizzled"
		// variant); spell_msg.xml supplies the fallback for the data-driven success path
		// (issue.room-affects). A sheaf with no kAffImposed* key shows nothing -- same
		// silent-skip behaviour as the per-spell handler in CastAffect (issue #3335).
		const auto &sheaf = MUD::SpellMessages()[spell_id];
		if (to_room != nullptr) {
			act(to_room, true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		} else {
			const auto &xml_to_room = sheaf.GetMessage(ESpellMsg::kAffImposedToRoom);
			if (!xml_to_room.empty()) {
				act(xml_to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			}
		}
		if (to_char != nullptr) {
			act(to_char, true, ch, nullptr, nullptr, kToChar);
		} else {
			const auto &xml_to_char = sheaf.GetMessage(ESpellMsg::kAffImposedToChar);
			if (!xml_to_char.empty()) {
				act(xml_to_char.c_str(), true, ch, nullptr, nullptr, kToChar);
			}
		}
		return 1;
	} else
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);

	if (!ch->IsImmortal())
		SetBattleLag(ch, lag);

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
			affect_i->modifier = af.modifier;
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

void affect_to_room(RoomData *room, const Affect<ERoomApply> &af) {
	Affect<ERoomApply>::shared_ptr new_affect(new Affect<ERoomApply>(af));

	room->affected.push_front(new_affect);
}

/**
 * Removing room affect from first affected room.
 * @param ch - affect caster.
 * @param spell_id - removing spell affect.
 */
void RemoveSingleAffectFromWorld(CharData *ch, ESpell spell_id) {
	auto affected_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), spell_id);
	if (affected_room) {
		const auto aff = room_spells::FindAffect(affected_room, spell_id);
		if (aff != affected_room->affected.end()) {
			room_spells::RoomRemoveAffect(affected_room, aff);
			SendMsgToChar("Ваша рунная метка удалена.\r\n", ch);
		}
	}
}

void ProcessRoomAffectsOnEntry(CharData *ch, RoomRnum room) {
	if (ch->IsImmortal()) {
		return;
	}

	const auto affect_on_room = room_spells::FindAffect(world[room], ESpell::kHypnoticPattern);
	if (affect_on_room != world[room]->affected.end()) {
		CharData *caster = find_char((*affect_on_room)->caster_id);
		// если не в гопе, и не слепой
		if (!group::same_group(ch, caster)
			&& !AFF_FLAGGED(ch, EAffect::kBlind)){
			// отсекаем всяких непонятных личностей типо двойников и проч
			// \todo Пальцы себе отсеки за такой код. Переделать на константы, а еще лучше - сделать где-то enum или вообще конфиг с внумами мобов для спеллов
			// Клон вообще по флагу клона проверяется
			if  ((GET_MOB_VNUM(ch) >= 3000 && GET_MOB_VNUM(ch) < 4000) || GET_MOB_VNUM(ch) == 108 ) return;
			if (ch->has_master() && !ch->get_master()->IsNpc() && ch->IsNpc()) {
				return;
			}
			// если вошел игрок - ПвП - делаем проверку на шанс в зависимости от % магии кастующего
			// без магии и ниже 80%: шанс 25%, на 100% - 27%, на 200% - 37% ,при 300% - 47%
			// иначе пве, и просто кастим сон на входящего
			if (!ch->IsNpc() && (number (1, 100) > (25))) {
				return;
			}
			SendMsgToChar("Вы уставились на огненный узор, как баран на новые ворота.", ch);
			act("$n0 уставил$u на огненный узор, как баран на новые ворота.",
				true, ch, nullptr, ch, kToRoom | kToArenaListen);
			CallMagic(caster, ch, nullptr, nullptr, ESpell::kSleep, GetRealLevel(caster));
		}
	}
}

} // namespace room_spells

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
