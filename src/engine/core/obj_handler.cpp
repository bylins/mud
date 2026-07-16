/**
\file obj_handler.cpp - a part of the Bylins engine.
\brief issue.handler-cleaning: split out of handler.cpp.
*/

#include "engine/core/obj_handler.h"
#include "engine/core/char_movement.h"
#include "engine/core/char_handler.h"
#include "engine/core/char_movement.h"
#include "administration/privilege.h"

#include "engine/scripting/dg_scripts.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/equipment.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/fight/fight_stuff.h"
#include "gameplay/economics/auction.h"
#include "utils/backtrace.h"
#include "utils_char_obj.inl"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/fight/fight.h"
#include "gameplay/clans/house.h"
#include "gameplay/magic/magic.h"
#include "engine/db/obj_prototypes.h"
#include "engine/ui/color.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/core/remort.h"
#include "gameplay/magic/magic_rooms.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/base_stats.h"
#include "utils/utils_time.h"
#include "gameplay/classes/pc_classes.h"

using classes::CalcCircleSlotsAmount;

// local functions //
int apply_ac(CharData *ch, int eq_pos);
int apply_armour(CharData *ch, int eq_pos);
void UpdateObject(ObjData *obj, int use);
void UpdateCharObjects(CharData *ch);

// external functions //
void PerformDropGold(CharData *ch, int amount);
int invalid_unique(CharData *ch, const ObjData *obj);
void do_entergame(DescriptorData *d);
void DoReturn(CharData *ch, char *argument, int cmd, int subcmd);
//extern std::vector<City> Cities;
extern int global_uid;
extern void change_leader(CharData *ch, CharData *vict);
char *sight::find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);
extern void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);




// move a player out of a room

// object destroy timers (file-scope, used by PlaceObjToRoom)
const int kMoneyDestroyTimer = 60;
const int kDeathDestroyTimer = 5;
const int kRoomDestroyTimer = 10;
const int kScriptDestroyTimer = 10; // * !!! Never set less than ONE * //

void InitUid(ObjData *object) {
	if (GET_OBJ_VNUM(object) > 0 && // Объект не виртуальный
		object->get_unique_id() == 0)   // У объекта точно нет уида
	{
		global_uid++; // Увеличиваем глобальный счетчик уидов
		global_uid = global_uid == 0 ? 1 : global_uid; // Если произошло переполнение инта
		object->set_unique_id(global_uid); // Назначаем уид
	}
}

void ArrangeObjInList(ObjData *obj, ObjData::obj_list_t &list) {
	for (auto it = list.begin(); it != list.end(); ++it) {
		if (IsObjsStackable(*it, obj)) {
			list.insert(it, obj);
			return;
		}
	}
	list.push_front(obj);
}

bool PlaceObjToRoom(ObjData *object, RoomRnum room) {
//	int sect = 0;
	if (!object || room < kFirstRoom || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to PlaceObjToRoom. (Room #%d/%d, obj %p)",
			room, top_of_world, object);
		return false;
	}
	ArrangeObjInList(object, world[room]->contents);
	if (world[room]->vnum % 100 == 99 && zone_table[world[room]->zone_rn].vnum < dungeons::kZoneStartDungeons) {
		if (!(object->has_flag(EObjFlag::kAppearsDay)
				|| object->has_flag(EObjFlag::kAppearsFullmoon)
				|| object->has_flag(EObjFlag::kAppearsNight))) {
			debug::backtrace(runtime_config.logs(SYSLOG).handle());
			sprintf(buf, "Попытка поместить объект в виртуальную комнату: objvnum %d, objname %s, roomvnum %d, создан coredump",
					object->get_vnum(), object->get_PName(grammar::ECase::kNom).c_str(), world[room]->vnum);
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		}
	}
	object->set_in_room(room);
	object->set_carried_by(nullptr);
	object->set_worn_by(nullptr);
	if (ROOM_FLAGGED(room, ERoomFlag::kNoItem)) {
		object->set_extra_flag(EObjFlag::kDecay);
	}
	if (object->get_script()->has_triggers()) {
		object->set_destroyer(kScriptDestroyTimer);
	} else if (object->get_type() == EObjType::kMoney) {
		object->set_destroyer(kMoneyDestroyTimer);
	} else if (ROOM_FLAGGED(room, ERoomFlag::kDeathTrap)) {
		object->set_destroyer(kDeathDestroyTimer);
	} else if (!IS_CORPSE(object)) {
		object->set_destroyer(kRoomDestroyTimer);
	}
	if (object->get_type() != EObjType::kFountain && !object->has_flag(EObjFlag::kNodecay)) {
		world_objects.decay_manager().insert(object);
		world_objects.decay_manager().add_env_check(object);
	}
	return true;
}

bool CheckObjDecay(ObjData *object,  bool need_extract) {
	int room, sect;
	room = object->get_in_room();

	if (room == kNowhere) {
		return false;
	}
	if (room < 0 || room > top_of_world) {
		log("SYSERR: CheckObjDecay: object '%s' vnum %d has invalid room %d",
			object->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(object), room);
		return false;
	}
	sect = real_sector(room);

	if (((sect == ESector::kWaterSwim || sect == ESector::kWaterNoswim) &&
		!object->has_flag(EObjFlag::kSwimming) &&
		!object->has_flag(EObjFlag::kFlying) &&
		!IS_CORPSE(object))) {
		act("$o0 медленно утонул$G.",
			false, world[room]->first_character(), object, nullptr, kToRoom);
		act("$o0 медленно утонул$G.",
			false, world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			log("[Obj decay] for: %s vnum == %d", object->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(object));
			ExtractObjFromWorld(object);
		}
		return true;
	}

	if (((sect == ESector::kOnlyFlying) && !IS_CORPSE(object) && !object->has_flag(EObjFlag::kFlying))) {

		act("$o0 упал$G вниз.",
			false, world[room]->first_character(), object, nullptr, kToRoom);
		act("$o0 упал$G вниз.",
			false, world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			log("[Obj decay] for: %s vnum == %d", object->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(object));
			ExtractObjFromWorld(object);
		}
		return true;
	}

	if (object->has_flag(EObjFlag::kDecay) || (object->has_flag(EObjFlag::kZonedecay) && object->get_vnum_zone_from() != zone_table[world[room]->zone_rn].vnum)) {
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", false,
			world[room]->first_character(), object, nullptr, kToRoom);
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", false,
			world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			log("[Obj decay] for: %s vnum == %d", object->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(object));
			ExtractObjFromWorld(object);
		}
		return true;
	}
	if (ROOM_FLAGGED(object->get_in_room(), ERoomFlag::kDeathTrap)) {
		act("$o0 исчез$Q в яркой вспышке.", false,
			world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			log("[Obj decay] extract in DT #%d for: %s vnum == %d", world[object->get_in_room()]->vnum, object->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(object));
			ExtractObjFromWorld(object);
		}
		return true;
	}
	return false;
}

void RemoveObjFromRoom(ObjData *object) {
	if (!object || object->get_in_room() == kNowhere) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to RemoveObjFromRoom",
			object, object->get_in_room());
		return;
	}

	{
		auto &list = world[object->get_in_room()]->contents;
		list.remove(object);
	}

	object->set_in_room(kNowhere);
	object->set_next_content(nullptr);
	world_objects.decay_manager().remove_env_check(object);
}

void PlaceObjIntoObj(ObjData *obj, ObjData *obj_to) {
	ObjData *tmp_obj;

	if (!obj || !obj_to || obj == obj_to) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to PlaceObjIntoObj.",
			obj, obj, obj_to);
		return;
	}

	auto list = obj_to->get_contains();
	ArrangeObjs(obj, &list);
	obj_to->set_contains(list);
	obj->set_in_obj(obj_to);

	for (tmp_obj = obj->get_in_obj(); tmp_obj->get_in_obj(); tmp_obj = tmp_obj->get_in_obj()) {
		tmp_obj->add_weight(obj->get_weight());
	}

	// top level object.  Subtract weight from inventory if necessary.
	tmp_obj->add_weight(obj->get_weight());
	if (tmp_obj->get_carried_by()) {
		IS_CARRYING_W(tmp_obj->get_carried_by()) += obj->get_weight();
	}
}

void RemoveObjFromObj(ObjData *obj) {
	if (obj->get_in_obj() == nullptr) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
		mudlog("SYSERR: trying to illegally extract obj from obj.");
		return;
	}
	auto obj_from = obj->get_in_obj();
	auto head = obj_from->get_contains();
	obj->remove_me_from_contains_list(head);
	obj_from->set_contains(head);

	// Subtract weight from containers container
	auto temp = obj->get_in_obj();
	for (; temp->get_in_obj(); temp = temp->get_in_obj()) {
		temp->set_weight(std::max(1, temp->get_weight() - obj->get_weight()));
	}

	// Subtract weight from char that carries the object
	temp->set_weight(std::max(1, temp->get_weight() - obj->get_weight()));
	if (temp->get_carried_by()) {
		IS_CARRYING_W(temp->get_carried_by()) = std::max(1, temp->get_carried_by()->GetCarryingWeight() - obj->get_weight());
	}

	obj->set_in_obj(nullptr);
	obj->set_next_content(nullptr);
}

void object_list_new_owner(ObjData *list, CharData *ch) {
	if (list) {
		object_list_new_owner(list->get_contains(), ch);
		object_list_new_owner(list->get_next_content(), ch);
		list->set_carried_by(ch);
	}
}

RoomVnum get_room_where_obj(ObjData *obj, bool deep) {
	if (GET_ROOM_VNUM(obj->get_in_room()) != kNowhere) {
		return GET_ROOM_VNUM(obj->get_in_room());
	} else if (obj->get_in_obj() && !deep) {
		return get_room_where_obj(obj->get_in_obj(), true);
	} else if (obj->get_carried_by()) {
		return GET_ROOM_VNUM(obj->get_carried_by()->in_room);
	} else if (obj->get_worn_by()) {
		return GET_ROOM_VNUM(obj->get_worn_by()->in_room);
	}

	return kNowhere;
}

// issue #3563: единый лог о пропаже вещи у игрока. Ищет игрока-владельца
// (инвентарь/экипировка/его контейнер) и пишет в syslog причину. Звать НАДО
// до отвязки вещи от владельца, иначе get_carried_by/get_worn_by уже пустые.
// issue #3563: описание держателя вещи для лога (родительный падеж) -- игрок или
// чармис игрока. "" -> обычный моб/никто (не логируем). Для чармиса поднимаемся
// по цепочке мастеров до игрока-владельца.
std::string ObjHolderLogDesc(CharData *holder) {
	if (!holder) {
		return "";
	}
	char buf[kMaxStringLength];
	if (!holder->IsNpc()) {
		snprintf(buf, sizeof(buf), "игрока %s", GET_NAME(holder));
		return buf;
	}
	if (IsCharmice(holder)) {
		// чармис висит напрямую на игроке (конвенция: pk.cpp, equipment.cpp и т.п.)
		CharData *m = holder->get_master();
		if (m && !m->IsNpc()) {
			snprintf(buf, sizeof(buf), "чармиса '%s' игрока %s", GET_NAME(holder), GET_NAME(m));
		} else {
			snprintf(buf, sizeof(buf), "чармиса '%s' (потерявшего владельца)", GET_NAME(holder));
		}
		return buf;
	}
	return "";
}

void LogPlayerObjLoss(ObjData *obj, const char *reason) {
	if (!obj) {
		return;
	}
	CharData *holder = obj->get_carried_by() ? obj->get_carried_by()
			: (obj->get_worn_by() ? obj->get_worn_by() : nullptr);
	if (!holder) {
		// вещь в контейнере -- ищем владельца контейнера
		for (ObjData *cont = obj->get_in_obj(); cont; cont = cont->get_in_obj()) {
			if (cont->get_carried_by()) {
				holder = cont->get_carried_by();
				break;
			}
			if (cont->get_worn_by()) {
				holder = cont->get_worn_by();
				break;
			}
		}
	}
	const std::string who = ObjHolderLogDesc(holder);
	if (who.empty()) {  // не игрок и не чармис игрока -- не логируем
		return;
	}
	log("[Obj loss] у %s пропала вещь '%s' vnum == %d (%s), timer == %d, прочность == %d/%d",
			who.c_str(), obj->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(obj),
			reason, obj->get_timer(), obj->get_current_durability(), obj->get_maximum_durability());
}

void ExtractObjFromWorld(ObjData *obj, bool showlog) {
	timechange_unregister_obj(obj);
	char name[kMaxStringLength];
	ObjData *temp;
	int roomload = get_room_where_obj(obj, false);
	utils::CExecutionTimer timer;

	strcpy(name, obj->get_PName(grammar::ECase::kNom).c_str());
	if (showlog) {
		log("[Extract obj] Start for: %s vnum == %d room = %d timer == %d",
				name, GET_OBJ_VNUM(obj), roomload, obj->get_timer());
	}
	// Обработка содержимого контейнера при его уничтожении
	purge_otrigger(obj);
//	log("[Extract obj] purge_otrigger, delta %f", timer.delta().count());
	if (obj->get_contains()) {
		while (obj->get_contains()) {
			temp = obj->get_contains();
			RemoveObjFromObj(temp);
			if (obj->get_carried_by()) {
				if (obj->get_carried_by()->IsNpc()
					|| (obj->get_carried_by()->GetCarryingQuantity() >= CAN_CARRY_N(obj->get_carried_by()))) {
					PlaceObjToRoom(temp, obj->get_carried_by()->in_room);
					CheckObjDecay(temp);
				} else {
					PlaceObjToInventory(temp, obj->get_carried_by());
				}
			} else if (obj->get_worn_by() != nullptr) {
				if (obj->get_worn_by()->IsNpc()
					|| (obj->get_worn_by()->GetCarryingQuantity() >= CAN_CARRY_N(obj->get_worn_by()))) {
					PlaceObjToRoom(temp, obj->get_worn_by()->in_room);
					CheckObjDecay(temp);
				} else {
					PlaceObjToInventory(temp, obj->get_worn_by());
				}
			} else if (obj->get_in_room() != kNowhere
					&& (IS_CORPSE(obj) || temp->has_flag(EObjFlag::kTicktimer))) {
				PlaceObjToRoom(temp, obj->get_in_room());
				CheckObjDecay(temp);
			} else if (obj->get_in_obj()) {
				ExtractObjFromWorld(temp, false);
			} else {
				ExtractObjFromWorld(temp, false);
			}
		}
		if (showlog)
			log("[Extract obj] Delta for container: %s vnum == %d room = %d timer == %d delta_time %f",
					name, GET_OBJ_VNUM(obj), get_room_where_obj(obj, false), obj->get_timer(), timer.delta().count());
	}
	// Содержимое контейнера удалено

	if (obj->get_worn_by() != nullptr) {
		if (UnequipChar(obj->get_worn_by(), obj->get_worn_on(), CharEquipFlags()) != obj) {
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
		}
	}

	if (obj->get_in_room() != kNowhere) {
		RemoveObjFromRoom(obj);
	} else if (obj->get_carried_by()) {
		RemoveObjFromChar(obj);
	} else if (obj->get_in_obj()) {
		RemoveObjFromObj(obj);
	}

	check_auction(nullptr, obj);
	check_exchange(obj);
	obj->get_script()->set_purged();
	world_objects.remove(obj);
	if (showlog) {
		log("[Extract obj] Stop, delta %f", timer.delta().count());
	}
}

void UpdateCharObjects(CharData *ch) {
	for (int wear_pos = 0; wear_pos < EEquipPos::kNumEquipPos; wear_pos++) {
		if (GET_EQ(ch, wear_pos) != nullptr) {
			if (GET_EQ(ch, wear_pos)->get_type() == EObjType::kLightSource) {
				if (GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2) > 0) {
					const int i = GET_EQ(ch, wear_pos)->dec_val(2);
					if (i == 1) {
						act("$z $o замерцал$G и начал$G угасать.\r\n",
							false, ch, GET_EQ(ch, wear_pos), nullptr, kToChar);
						act("$o $n1 замерцал$G и начал$G угасать.",
							false, ch, GET_EQ(ch, wear_pos), nullptr, kToRoom);
					} else if (i == 0) {
						act("$z $o погас$Q.\r\n", false, ch, GET_EQ(ch, wear_pos), nullptr, kToChar);
						act("$o $n1 погас$Q.", false, ch, GET_EQ(ch, wear_pos), nullptr, kToRoom);
						if (ch->in_room != kNowhere) {
							if (world[ch->in_room]->light > 0)
								world[ch->in_room]->light -= 1;
						}
						if (GET_EQ(ch, wear_pos)->has_flag(EObjFlag::kDecay)) {
							ExtractObjFromWorld(GET_EQ(ch, wear_pos));
						}
					}
				}
			}
		}
	}
}

void DropObjOnZoneReset(CharData *ch, ObjData *obj, bool inv, bool zone_reset) {
	if (zone_reset && !obj->has_flag(EObjFlag::kTicktimer)) {
		ExtractObjFromWorld(obj, false);
	} else {
		if (inv)
			act("Вы выбросили $o3 на землю.", false, ch, obj, nullptr, kToChar);
		else
			act("Вы сняли $o3 и выбросили на землю.", false, ch, obj, nullptr, kToChar);
		// Если этот моб трупа не оставит, то не выводить сообщение
		// иначе ужасно коряво смотрится в бою и в тригах
		bool msgShown = false;
		if (!ch->IsNpc() || !ch->IsFlagged(EMobFlag::kCorpse)) {
			if (inv)
				act("$n бросил$g $o3 на землю.", false, ch, obj, nullptr, kToRoom);
			else
				act("$n снял$g $o3 и бросил$g на землю.", false, ch, obj, nullptr, kToRoom);
			msgShown = true;
		}

		drop_otrigger(obj, ch, kOtrigDropInroom);

		drop_wtrigger(obj, ch);

		PlaceObjToRoom(obj, ch->in_room);
		if (!CheckObjDecay(obj) && !msgShown) {
			act("На земле остал$U лежать $o.", false, ch, obj, nullptr, kToRoom);
		}
	}
}

int get_object_low_rent(ObjData *obj) {
	int rent = obj->get_rent_off() > obj->get_rent_on() ? obj->get_rent_on() : obj->get_rent_off();
	return rent;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
