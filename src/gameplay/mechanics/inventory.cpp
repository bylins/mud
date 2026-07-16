/**
\file inventory.cpp - a part of the Bylins engine.
\brief Inventory mechanic: carrying capacity, stacking, taking/putting objects.
\details issue.handler-cleaning (Bucket 2): gathered from handler.cpp (carry/stack),
 utils_char_obj.inl (CanTakeObj), base_stats (CAN_CARRY_N/W) and the do_inventory command.
*/

#include "gameplay/mechanics/inventory.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/obj_handler.h"
#include "engine/core/utils_char_obj.inl"
#include "utils/utils.h"
#include "utils/backtrace.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/named_stuff.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"

// file-local helpers (were file-local in handler.cpp)
static bool IsLabelledObjsStackable(ObjData *obj_one, ObjData *obj_two);

int CAN_CARRY_N(const CharData *ch) {
	int n = 5 + GetRealDex(ch) / 2 + GetRealLevel(ch) / 2;
	if (ch->HaveFeat(EFeat::kJuggler)) {
		n += GetRealLevel(ch) / 2;
		if (CanUseFeat(ch, EFeat::kThrifty)) {
			n += 5;
		}
	}
	if (CanUseFeat(ch, EFeat::kThrifty)) {
		n += 5;
	}
	return std::max(n, 1);
}

int CAN_CARRY_W(const CharData *ch) {
	return (str_bonus(GetRealStr(ch), STR_CARRY_W) * (ch->HaveFeat(EFeat::kPorter) ? 110 : 100)) / 100;
}


bool CanTakeObj(CharData *ch, ObjData *obj) {
	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)
		&& obj->get_type() != EObjType::kMoney) {
		act("$p: Вы не могете нести столько вещей.", false, ch, obj, nullptr, kToChar);
		return false;
	} else if ((ch->GetCarryingWeight() + obj->get_weight()) > CAN_CARRY_W(ch)
		&& obj->get_type() != EObjType::kMoney) {
		act("$p: Вы не в состоянии нести еще и $S.", false, ch, obj, nullptr, kToChar);
		return false;
	} else if (!(CAN_WEAR(obj, EWearFlag::kTake))) {
		act("$p: Вы не можете взять $S.", false, ch, obj, nullptr, kToChar);
		return false;
	} else if (invalid_anti_class(ch, obj)) {
		act("$p: Эта вещь не предназначена для вас!", false, ch, obj, nullptr, kToChar);
		return false;
	} else if (NamedStuff::check_named(ch, obj, false)) {
		if (!NamedStuff::wear_msg(ch, obj))
			act("$p: Эта вещь не предназначена для вас!", false, ch, obj, nullptr, kToChar);
		return false;
	}
	return true;
}

bool IsLabelledObjsStackable(ObjData *obj_one, ObjData *obj_two) {
	// без меток стокаются
	if (!obj_one->get_custom_label() && !obj_two->get_custom_label())
		return true;

	if (obj_one->get_custom_label() && obj_two->get_custom_label()) {
		// с разными типами меток не стокаются
		if (!obj_one->get_custom_label()->clan_abbrev != !obj_two->get_custom_label()->clan_abbrev) {
			return false;
		} else {
			// обе метки клановые один клан, текст совпадает -- стокается
			if (obj_one->get_custom_label()->clan_abbrev && obj_two->get_custom_label()->clan_abbrev
				&& !strcmp(obj_one->get_custom_label()->clan_abbrev, obj_two->get_custom_label()->clan_abbrev)
				&& obj_one->get_custom_label()->text_label && obj_two->get_custom_label()->text_label
				&& !strcmp(obj_one->get_custom_label()->text_label, obj_two->get_custom_label()->text_label)) {
				return true;
			}

			// обе метки личные, один автор, текст совпадает -- стокается
			if (obj_one->get_custom_label()->author == obj_two->get_custom_label()->author
				&& obj_one->get_custom_label()->text_label && obj_two->get_custom_label()->text_label
				&& !strcmp(obj_one->get_custom_label()->text_label, obj_two->get_custom_label()->text_label)) {
				return true;
			}
		}
	}

	return false;
}

bool IsObjsStackable(ObjData *obj_one, ObjData *obj_two) {
	if (GET_OBJ_VNUM(obj_one) != GET_OBJ_VNUM(obj_two)
		|| strcmp(obj_one->get_short_description().c_str(), obj_two->get_short_description().c_str())
		|| (obj_one->get_type() == EObjType::kLiquidContainer
			&& GET_OBJ_VAL(obj_one, 2) != GET_OBJ_VAL(obj_two, 2))
		|| (obj_one->get_type() == EObjType::kContainer
			&& (obj_one->get_contains() || obj_two->get_contains()))
		|| GET_OBJ_VNUM(obj_two) == -1
		|| (obj_one->get_type() == EObjType::kBook
			&& GET_OBJ_VAL(obj_one, 1) != GET_OBJ_VAL(obj_two, 1))
		|| !IsLabelledObjsStackable(obj_one, obj_two)) {
		return false;
	}

	return true;
}


void ArrangeObjs(ObjData *obj, ObjData **list_start) {
	// AL: пофиксил Ж)
	// Krodo: пофиксили третий раз, не сортируем у мобов в инве Ж)

	// begin - первый предмет в исходном списке
	// end - последний предмет в перемещаемом интервале
	// before - последний предмет перед началом интервала
	ObjData *p, *begin, *end, *before;

	obj->set_next_content(begin = *list_start);
	*list_start = obj;

	// похожий предмет уже первый в списке или список пустой
	if (!begin || IsObjsStackable(begin, obj)) {
		return;
	}

	before = p = begin;

	while (p && !IsObjsStackable(p, obj)) {
		before = p;
		p = p->get_next_content();
	}

	// нет похожих предметов
	if (!p) {
		return;
	}

	end = p;

	while (p && IsObjsStackable(p, obj)) {
		end = p;
		p = p->get_next_content();
	}

	end->set_next_content(begin);
	obj->set_next_content(before->get_next_content());
	before->set_next_content(p); // будет 0 если после перемещаемых ничего не лежало
}

void PlaceObjToInventory(ObjData *object, CharData *ch) {
	unsigned int tuid;
	int inworld;

	if (!world_objects.get_by_raw_ptr(object)) {
		std::stringstream ss;
		ss << "SYSERR: Object at address 0x" << object
		   << " is not in the world but we have attempt to put it into character '" << ch->get_name()
		   << "'. Object won't be placed into character's inventory.";
		mudlog(ss.str().c_str(), NRM, kLvlImplementator, SYSLOG, true);
		debug::backtrace(runtime_config.logs(ERRLOG).handle());

		return;
	}

	int may_carry = true;
	if (object && ch) {
		if (invalid_anti_class(ch, object) || NamedStuff::check_named(ch, object, false))
			may_carry = false;
		if (!may_carry) {
			act("Вас обожгло при попытке взять $o3.", false, ch, object, nullptr, kToChar);
			act("$n попытал$u взять $o3 - и чудом не сгорел$g.", false, ch, object, nullptr, kToRoom);
			PlaceObjToRoom(object, ch->in_room);
			return;
		}
		if (!ch->IsNpc()
			|| (ch->has_master()
				&& !ch->get_master()->IsNpc())) {
			if (object && object->get_unique_id() != 0 && object->get_timer() > 0) {
				tuid = object->get_unique_id();
				inworld = 1;
				// Объект готов для проверки. Ищем в мире такой же.
				world_objects.foreach_with_vnum(GET_OBJ_VNUM(object), [&inworld, tuid, object](const ObjData::shared_ptr &i) {
					if (static_cast<unsigned int>(i->get_unique_id()) == tuid // UID совпадает
						&& i->get_timer() > 0  // Целенький
						&& object != i.get()) // Не оно же
					{
						inworld++;
					}
				});

				if (inworld > 1) // У объекта есть как минимум одна копия
				{
					sprintf(buf,
							"Copy detected and prepared to extract! Object %s (UID=%ld, VNUM=%d), holder %s. In world %d.",
							object->get_PName(grammar::ECase::kNom).c_str(),
							object->get_unique_id(),
							GET_OBJ_VNUM(object),
							GET_NAME(ch),
							inworld);
					mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
					act("$o0 замигал$Q и вы увидели медленно проступившие руны 'DUPE'.", false, ch, object, nullptr, kToChar);
					object->set_timer(0);
					object->set_extra_flag(EObjFlag::kNosell); // Ибо нефиг
				}
			} // Назначаем UID
			else {
				InitUid(object);
				log("%s obj_to_char %s #%d|%ld",
					GET_NAME(ch),
					object->get_PName(grammar::ECase::kNom).c_str(),
					GET_OBJ_VNUM(object),
					object->get_unique_id());
			}
		}

		if (!ch->IsNpc() || (ch->has_master() && !ch->get_master()->IsNpc())) {
			object->set_extra_flag(EObjFlag::kTicktimer);    // start timer unconditionally when character picks item up.
			world_objects.decay_manager().insert(object);
			ArrangeObjs(object, &ch->carrying);
		} else {
			// Вот эта муть, чтобы временно обойти завязку магазинов на порядке предметов в инве моба // Krodo
			object->set_next_content(ch->carrying);
			ch->carrying = object;
		}

		object->set_carried_by(ch);
		object->set_in_room(kNowhere);
		IS_CARRYING_W(ch) += object->get_weight();
		IS_CARRYING_N(ch)++;

		if (!ch->IsNpc()) {
			log("obj_to_char: %s -> %d", ch->get_name().c_str(), GET_OBJ_VNUM(object));
		}
		// set flag for crash-save system, but not on mobs!
		if (!ch->IsNpc()) {
			ch->SetFlag(EPlrFlag::kCrashSave);
		}
	} else
		log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

void RemoveObjFromChar(ObjData *object) {
	if (!object || !object->get_carried_by()) {
		log("SYSERR: NULL object or owner passed to obj_from_char");
		return;
	}
	object->remove_me_from_contains_list(object->get_carried_by()->carrying);

	// set flag for crash-save system, but not on mobs!
	if (!object->get_carried_by()->IsNpc()) {
		object->get_carried_by()->SetFlag(EPlrFlag::kCrashSave);
		log("obj_from_char: %s -> %d", object->get_carried_by()->get_name().c_str(), GET_OBJ_VNUM(object));
	} else {
		// issue #3563: трейс ухода из инвентаря чармиса игрока (игрок -- ветка выше)
		const std::string who = ObjHolderLogDesc(object->get_carried_by());
		if (!who.empty()) {
			log("obj_from_char: %s -> %d", who.c_str(), GET_OBJ_VNUM(object));
		}
	}

	IS_CARRYING_W(object->get_carried_by()) -= object->get_weight();
	IS_CARRYING_N(object->get_carried_by())--;
	object->set_carried_by(nullptr);
	object->set_next_content(nullptr);
}

void can_carry_obj(CharData *ch, ObjData *obj) {
	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не можете нести столько предметов.", ch);
		PlaceObjToRoom(obj, ch->in_room);
		CheckObjDecay(obj);
	} else {
		if (obj->get_weight() + ch->GetCarryingWeight() > CAN_CARRY_W(ch)) {
			sprintf(buf, "Вам слишком тяжело нести еще и %s.", obj->get_PName(grammar::ECase::kAcc).c_str());
			SendMsgToChar(buf, ch);
			PlaceObjToRoom(obj, ch->in_room);
			// obj_decay(obj);
		} else {
			PlaceObjToInventory(obj, ch);
		}
	}
}

void ShowInventory(CharData *ch) {
	SendMsgToChar("Вы несете:\r\n", ch);
	sight::list_obj_to_char(ch->carrying, ch, 1, 2);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
