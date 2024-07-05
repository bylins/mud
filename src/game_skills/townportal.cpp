#include "townportal.h"

#include "modify.h"
#include "handler.h"
#include "game_fight/pk.h"
#include "game_magic/magic_rooms.h"

namespace OneWayPortal {

// список односторонних порталов <куда указывает, откуда поставлен>
std::unordered_map<RoomVnum /*to*/, RoomData * /*from*/> portal_list;

/**
* Добавление портала в список
* \param to_room - куда ставится пента
* \param from_room - откуда ставится
*/
void add(RoomData *to_room, RoomData *from_room) {
	portal_list.emplace(to_room->vnum, from_room);
}

/**
* Удаление портала из списка
* \param to_room - куда указывает пента
*/
void remove(RoomData *to_room) {
	const auto it = portal_list.find(to_room->vnum);

	if (it != portal_list.end()) {
		const auto aff = room_spells::FindAffect(it->second, ESpell::kPortalTimer);
		if (aff != to_room->affected.end()) {
			room_spells::RoomRemoveAffect(it->second, aff);
		}
		portal_list.erase(it);
	}
}

/**
* Проверка на наличие комнаты в списке
* \param to_room - куда указывает пента
* \return указатель на источник пенты
*/
RoomData *get_from_room(RoomData *to_room) {

	const auto it = portal_list.find(to_room->vnum);
	if (it != portal_list.end())
		return it->second;

	return nullptr;
}

} // namespace OneWayPortal

void AddPortalTimer(CharData *ch, RoomData *room, int time) {
	Affect<room_spells::ERoomApply> af;
	af.type = ESpell::kPortalTimer;
	af.bitvector = room_spells::ERoomAffect::kPortalTimer;
	af.duration = time; //раз в 2 секунды
	af.modifier = 0;
	af.battleflag = 0;
	af.location = room_spells::ERoomApply::kNone;
	af.caster_id = ch? GET_ID(ch) : 0;
	af.must_handled = false;
	af.apply_time = 0;
	room_spells::AffectRoomJoinReplace(room, af);
	room_spells::AddRoomToAffected(room);
}

void spell_townportal(CharData *ch, char *arg) {
	int gcount = 0, cn = 0, ispr = 0;
	bool has_label_portal = false;
	struct TimedSkill timed;
	char *nm;
	struct CharacterPortal *tmp;
	struct Portal *port;
	struct Portal label_port;
	RoomData *label_room;

	port = get_portal(-1, arg);

	//если портала нет, проверяем, возможно игрок ставит врата на свою метку
	if (!port && name_cmp(ch, arg)) {

		label_room = room_spells::FindAffectedRoom(GET_ID(ch), ESpell::kRuneLabel);
		if (label_room) {
			label_port.vnum = label_room->vnum;
			label_port.level = 1;
			port = &label_port;
			has_label_portal = true;
		}
	}
	if (port && (has_char_portal(ch, port->vnum) || has_label_portal)) {
		if (IsTimedBySkill(ch, ESkill::kTownportal)) {
			SendMsgToChar("У вас недостаточно сил для постановки врат.\r\n", ch);
			return;
		}

		if (find_portal_by_vnum(GET_ROOM_VNUM(ch->in_room))) {
			SendMsgToChar("Камень рядом с вами мешает вашей магии.\r\n", ch);
			return;
		}

		if (room_spells::IsRoomAffected(world[ch->in_room], ESpell::kRuneLabel)) {
			SendMsgToChar("Начертанные на земле магические руны подавляют вашу магию!\r\n", ch);
			return;
		}

		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic) && !IS_GRGOD(ch)) {
			SendMsgToChar("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", ch);
			act("Магия $n1 потерпела неудачу и развеялась по воздуху.", false, ch, 0, 0, kToRoom);
			return;
		}
		//удаляем переходы
		if (world[ch->in_room]->portal_time) {
			if (world[world[ch->in_room]->portal_room]->portal_room == ch->in_room
				&& world[world[ch->in_room]->portal_room]->portal_time) {
				decay_portal(world[ch->in_room]->portal_room);
			}
			decay_portal(ch->in_room);
		}

		// Открываем пентаграмму в комнату rnum //
		ImproveSkill(ch, ESkill::kTownportal, 1, nullptr);
		RoomData *from_room = world[ch->in_room];
		from_room->portal_room = real_room(port->vnum);
		from_room->portal_time = 1;
		from_room->pkPenterUnique = 0;
		OneWayPortal::add(world[from_room->portal_room], from_room);  //какая то недоделка
		AddPortalTimer(ch, from_room, 29);
		act("Лазурная пентаграмма возникла в воздухе.", false, ch, 0, 0, kToChar);
		act("$n сложил$g руки в молитвенном жесте, испрашивая у Богов врата...", false, ch, 0, 0, kToRoom);
		act("Лазурная пентаграмма возникла в воздухе.", false, ch, 0, 0, kToRoom);
		if (!IS_IMMORTAL(ch)) {
			timed.skill = ESkill::kTownportal;
			// timed.time - это unsigned char, поэтому при уходе в минус будет вынос на 255 и ниже
			int modif = ch->GetSkill(ESkill::kTownportal) / 7 + number(1, 5);
			timed.time = MAX(1, 25 - modif);
			ImposeTimedSkill(ch, &timed);
		}
		return;
	}

	// выдаем список мест //
	gcount = sprintf(buf2 + gcount, "Вам доступны следующие врата:\r\n");
	for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
		nm = find_portal_by_vnum(tmp->vnum);
		if (nm) {
			gcount += sprintf(buf2 + gcount, "%11s", nm);
			cn++;
			ispr++;
			if (cn == 3) {
				gcount += sprintf(buf2 + gcount, "\r\n");
				cn = 0;
			}
		}
	}
	if (cn)
		gcount += sprintf(buf2 + gcount, "\r\n");
	if (!ispr) {
		gcount += sprintf(buf2 + gcount, "     В настоящий момент вам неизвестны порталы.\r\n");
	} else {
		gcount += sprintf(buf2 + gcount, "     Сейчас в памяти - %d.\r\n", ispr);
	}
	gcount += sprintf(buf2 + gcount, "     Максимально     - %d.\r\n", MAX_PORTALS(ch));

	page_string(ch->desc, buf2, 1);
}

void do_townportal(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	struct CharacterPortal *tmp, *dlt = nullptr;
	char arg2[kMaxInputLength];
	int vnum = 0;

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kTownportal)) {
		SendMsgToChar("Прежде изучите секрет постановки врат.\r\n", ch);
		return;
	}

	two_arguments(argument, arg, arg2);
	if (!str_cmp(arg, "забыть")) {
		vnum = find_portal_by_word(arg2);
		for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
			if (tmp->vnum == vnum) {
				if (dlt) {
					dlt->next = tmp->next;
				} else {
					GET_PORTALS(ch) = tmp->next;
				}
				free(tmp);
				sprintf(buf, "Вы полностью забыли, как выглядит камень '&R%s&n'.\r\n", arg2);
				SendMsgToChar(buf, ch);
				break;
			}
			dlt = tmp;
		}
		return;
	}

	spell_townportal(ch, arg);
}
