/**
\file DoLoad.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "engine/olc/olc.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/corpse.h"
#include "engine/entities/zone.h"
#include "engine/db/world_objects.h"
#include "gameplay/mechanics/stuff.h"

extern int load_into_inventory;

void DoLoad(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	CharData *mob;
	MobVnum number;
	MobRnum r_num;
	char *iname;

	iname = two_arguments(argument, buf, buf2);

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <= 0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	int first = atoi(buf2) / 100;

	if (!IS_IMMORTAL(ch) && GET_OLC_ZONE(ch) != first) {
		SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
		return;
	}
	if (!*buf || !*buf2 || !a_isdigit(*buf2)) {
		SendMsgToChar("Usage: load { obj | mob } <number>\r\n"
					  "       load ing { <сила> | <VNUM> } <имя>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0) {
		SendMsgToChar("Отрицательный моб опасен для вашего здоровья!\r\n", ch);
		return;
	}
	if (utils::IsAbbr(buf, "mob")) {
		if ((r_num = GetMobRnum(number)) < 0) {
			SendMsgToChar("Нет такого моба в этом МУДе.\r\n", ch);
			return;
		}
		if ((zone_table[get_zone_rnum_by_mob_vnum(number)].locked) && (GetRealLevel(ch) != kLvlImplementator)) {
			SendMsgToChar("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
			return;
		}
		mob = ReadMobile(r_num, kReal);
		PlaceCharToRoom(mob, ch->in_room);
		act("$n порыл$u в МУДе.", true, ch, nullptr, nullptr, kToRoom);
		act("$n создал$g $N3!", false, ch, nullptr, mob, kToRoom);
		act("Вы создали $N3.", false, ch, nullptr, mob, kToChar);
		load_mtrigger(mob);
		olc_log("%s load mob %s #%d", GET_NAME(ch), GET_NAME(mob), number);
	} else if (utils::IsAbbr(buf, "obj")) {
		if ((r_num = GetObjRnum(number)) < 0) {
			SendMsgToChar("Господи, да изучи ты номера объектов.\r\n", ch);
			return;
		}
		if ((zone_table[get_zone_rnum_by_obj_vnum(number)].locked) && (GetRealLevel(ch) != kLvlImplementator)) {
			SendMsgToChar("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
			return;
		}
		const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
		obj->set_crafter_uid(GET_UID(ch));
		obj->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));

		if (number == GlobalDrop::MAGIC1_ENCHANT_VNUM
			|| number == GlobalDrop::MAGIC2_ENCHANT_VNUM
			|| number == GlobalDrop::MAGIC3_ENCHANT_VNUM) {
			generate_magic_enchant(obj.get());
		}

		if (load_into_inventory) {
			PlaceObjToInventory(obj.get(), ch);
		} else {
			PlaceObjToRoom(obj.get(), ch->in_room);
		}

		act("$n покопал$u в МУДе.", true, ch, nullptr, nullptr, kToRoom);
		act("$n создал$g $o3!", false, ch, obj.get(), nullptr, kToRoom);
		act("Вы создали $o3.", false, ch, obj.get(), nullptr, kToChar);
		load_otrigger(obj.get());
		CheckObjDecay(obj.get());
		olc_log("%s load obj %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), number);
	} else if (utils::IsAbbr(buf, "ing")) {
		int power, i;
		power = atoi(buf2);
		skip_spaces(&iname);
		i = im_get_type_by_name(iname, 0);
		if (i < 0) {
			SendMsgToChar("Неверное имя типа\r\n", ch);
			return;
		}
		const auto obj = load_ingredient(i, power, power);
		if (!obj) {
			SendMsgToChar("Ошибка загрузки ингредиента\r\n", ch);
			return;
		}
		PlaceObjToInventory(obj, ch);
		act("$n покопал$u в МУДе.", true, ch, nullptr, nullptr, kToRoom);
		act("$n создал$g $o3!", false, ch, obj, nullptr, kToRoom);
		act("Вы создали $o3.", false, ch, obj, nullptr, kToChar);
		sprintf(buf, "%s load ing %d %s", GET_NAME(ch), power, iname);
		mudlog(buf, NRM, kLvlBuilder, IMLOG, true);
		load_otrigger(obj);
		CheckObjDecay(obj);
		olc_log("%s load ing %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), power);
	} else {
		SendMsgToChar("Нет уж. Ты создай че-нить нормальное.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
