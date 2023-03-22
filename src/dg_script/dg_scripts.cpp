/**************************************************************************
*  File: dg_scripts.cpp                                   Part of Bylins  *
*  Usage: contains general functions for using scripts.                   *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
**************************************************************************/

#include "dg_scripts.h"
#include "structs/global_objects.h"
#include "utils/utils_find_obj_id_by_vnum.h"
#include "obj_prototypes.h"
#include "handler.h"
#include "dg_event.h"
#include "color.h"
#include "house.h"
#include "entities/char_player.h"
#include "modify.h"
#include "game_mechanics/named_stuff.h"
#include "game_magic/magic_utils.h"
#include "noob.h"
#include "dg_db_scripts.h"
#include "dg_domination_helper.h"
#include "game_mechanics/bonus.h"
#include "olc/olc.h"
#include "administration/privilege.h"
#include "game_fight/fight_hit.h"
#include "utils/utils_char_obj.inl"

extern int max_exp_gain_pc(CharData *ch);
extern long GetExpUntilNextLvl(CharData *ch, int level);
constexpr long long kPulsesPerMudHour = kSecsPerMudHour*kPassesPerSec;

inline bool IS_CHARMED(CharData* ch) {return (IS_HORSE(ch) || AFF_FLAGGED(ch, EAffect::kCharmed));};

// Вывод сообщений о неверных управляющих конструкциях DGScript
#define DG_CODE_ANALYZE

// external vars from triggers.cpp
extern const char *trig_types[], *otrig_types[], *wtrig_types[];
const char *attach_name[] = {"mob", "obj", "room", "unknown!!!"};

int last_trig_vnum = 0;
int curr_trig_vnum = 0;

// other external vars

extern void add_trig_to_owner(int vnum_owner, int vnum_trig, int vnum);
extern CharData *combat_list;
extern const char *item_types[];
extern const char *genders[];
extern const char *exit_bits[];
extern IndexData *mob_index;
extern TimeInfoData time_info;
extern void RepopDecay(std::vector<ZoneRnum> zone_list);    // рассыпание обьектов ITEM_REPOP_DECAY
extern bool CanTakeObj(CharData *ch, ObjData *obj);
extern void split_or_clan_tax(CharData *ch, long amount);

// external functions
RoomRnum find_target_room(CharData *ch, char *rawroomstr, int trig);
void free_varlist(struct TriggerVar *vd);
int obj_room(ObjData *obj);
Trigger *read_trigger(int nr);
ObjData *get_object_in_equip(CharData *ch, char *name);
void extract_trigger(Trigger *trig);
int eval_lhs_op_rhs(const char *expr, char *result, void *go, Script *sc, Trigger *trig, int type);
const char *skill_percent(Trigger *trig, CharData *ch, char *skill);
bool feat_owner(Trigger *trig, CharData *ch, char *feat);
const char *spell_count(Trigger *trig, CharData *ch, char *spell);
const char *spell_knowledge(Trigger *trig, CharData *ch, char *spell);
int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);
void reset_zone(int znum);

void do_restore(CharData *ch, char *argument, int cmd, int subcmd);
void do_mpurge(CharData *ch, char *argument, int cmd, int subcmd);
void do_mjunk(CharData *ch, char *argument, int cmd, int subcmd);
void do_arena_restore(CharData *ch, char *argument, int cmd, int subcmd);
// function protos from this file
void extract_value(Script *sc, Trigger *trig, char *cmd);
int script_driver(void *go, Trigger *trig, int type, int mode);
int trgvar_in_room(int vnum);
void do_worldecho(char *msg);

/*
Костыль, но всеж.
*/
bool CharacterLinkDrop = false;

//table for replace UID_CHAR, UID_OBJ, UID_ROOM
const char uid_replace_table[] = {
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d',
	'\x0e', '\x0f',    //16
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b', '\x20', '\x20',
	'\x20', '\x1f',    //32
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d',
	'\x2e', '\x2f',    //48
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d',
	'\x3e', '\x3f',    //64
	'\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d',
	'\x4e', '\x4f',    //80
	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d',
	'\x5e', '\x5f',    //96
	'\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d',
	'\x6e', '\x6f',    //112
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d',
	'\x7e', '\x7f',    //128
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d',
	'\x8e', '\x8f',    //144
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d',
	'\x9e', '\x9f',    //160
	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad',
	'\xae', '\xaf',    //176
	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd',
	'\xbe', '\xbf',    //192
	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd',
	'\xce', '\xcf',    //208
	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd',
	'\xde', '\xdf',    //224
	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed',
	'\xee', '\xef',    //240
	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd',
	'\xfe', '\xff'    //256
};

void script_log(const char *msg, LogMode type) {
	char tmpbuf[kMaxStringLength];

	snprintf(tmpbuf, kMaxStringLength, "SCRIPT LOG %s", msg);

	char *pos = tmpbuf;
	while (*pos != '\0') {
		*pos = uid_replace_table[static_cast<unsigned char>(*pos)];
		++pos;
	}

	log("%s", tmpbuf);
	mudlog(tmpbuf, type ? type : NRM, kLvlBuilder, ERRLOG, true);
}

/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void trig_log(Trigger *trig, const char *msg, LogMode type) {
	char tmpbuf[kMaxStringLength];
	snprintf(tmpbuf, kMaxStringLength, "(Trigger: %s, VNum: %d) : %s", GET_TRIG_NAME(trig), 
			GET_TRIG_VNUM(trig), msg);
	script_log(tmpbuf, type);
}

cmdlist_element::shared_ptr find_end(Trigger *trig, cmdlist_element::shared_ptr cl);
cmdlist_element::shared_ptr find_done(Trigger *trig, cmdlist_element::shared_ptr cl);
cmdlist_element::shared_ptr find_case(Trigger *trig, cmdlist_element::shared_ptr cl, void *go, Script *sc, int type, char *cond);
cmdlist_element::shared_ptr find_else_end(Trigger *trig, cmdlist_element::shared_ptr cl, void *go, Script *sc, int type);

struct TriggerVar *worlds_vars;

// Для отслеживания работы команд "wat" и "mat"
int reloc_target = -1;
Trigger *cur_trig = nullptr;

GlobalTriggersStorage::~GlobalTriggersStorage() {
	// notify all observers that of each trigger that they are going to be removed
	for (const auto &i : m_observers) {
		for (const auto &observer : i.second) {
			observer->notify(i.first);
		}
	}
}

void GlobalTriggersStorage::add(Trigger *trigger) {
	m_triggers.insert(trigger);
	m_rnum2triggers_set[trigger->get_rnum()].insert(trigger);
}

void GlobalTriggersStorage::remove(Trigger *trigger) {
	// notify all observers that this trigger is going to be removed.
	const auto observers_list_i = m_observers.find(trigger);
	if (observers_list_i != m_observers.end()) {
		for (const auto &observer : m_observers[trigger]) {
			observer->notify(trigger);
		}

		m_observers.erase(observers_list_i);
	}

	// then erase trigger
	m_triggers.erase(trigger);
	m_rnum2triggers_set[trigger->get_rnum()].erase(trigger);
}

void GlobalTriggersStorage::shift_rnums_from(const Rnum rnum) {
	// TODO: Get rid of this function when index will not has to be sorted by rnums
	//       Actually we need to get rid of rnums at all.
	std::list<Trigger *> to_rebind;
	for (const auto trigger : m_triggers) {
		if (trigger->get_rnum() > rnum) {
			to_rebind.push_back(trigger);
		}
	}

	for (const auto trigger : to_rebind) {
		remove(trigger);
		trigger->set_rnum(1 + trigger->get_rnum());
		add(trigger);
	}
}

void GlobalTriggersStorage::register_remove_observer(Trigger *trigger,
													 const TriggerEventObserver::shared_ptr &observer) {
	const auto i = m_triggers.find(trigger);
	if (i != m_triggers.end()) {
		m_observers[trigger].insert(observer);
	}
}

void GlobalTriggersStorage::unregister_remove_observer(Trigger *trigger,
													   const TriggerEventObserver::shared_ptr &observer) {
	const auto i = m_observers.find(trigger);
	if (i != m_observers.end()) {
		i->second.erase(observer);

		if (i->second.empty()) {
			m_observers.erase(i);
		}
	}
}

obj2triggers_t &obj2triggers = GlobalObjects::obj_triggers();
GlobalTriggersStorage &trigger_list = GlobalObjects::trigger_list();    // all attached triggers

int trgvar_in_room(int vnum) {
	const int rnum = real_room(vnum);

	if (kNowhere == rnum) {
		script_log("people.vnum: world[rnum] does not exist");
		return (-1);
	}

	int count = 0;
	for (const auto &ch : world[rnum]->people) {
		if (!GET_INVIS_LEV(ch)) {
			++count;
		}
	}

	return count;
};

ObjData *get_obj_in_list(char *name, ObjData *list) {
	ObjData *i;
	long id;

	if (*name == UID_OBJ) {
		id = atoi(name + 1);

		for (i = list; i; i = i->get_next_content()) {
			if (id == i->get_id()) {
				return i;
			}
		}
	} else {
		for (i = list; i; i = i->get_next_content()) {
			if (isname(name, i->get_aliases())) {
				return i;
			}
		}
	}

	return nullptr;
}

ObjData *get_object_in_equip(CharData *ch, char *name) {
	int j, n = 0, number;
	ObjData *obj;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;
	long id;

	if (*name == UID_OBJ) {
		id = atoi(name + 1);

		for (j = 0; j < EEquipPos::kNumEquipPos; j++)
			if ((obj = GET_EQ(ch, j)))
				if (id == obj->get_id())
					return (obj);
	} else {
		strcpy(tmp, name);
		if (!(number = get_number(&tmp)))
			return nullptr;

		for (j = 0; (j < EEquipPos::kNumEquipPos) && (n <= number); j++) {
			obj = GET_EQ(ch, j);
			if (!obj) {
				continue;
			}

			if (isname(tmp, obj->get_aliases())) {
				++n;
				if (n == number) {
					return (obj);
				}
			}
		}
	}

	return nullptr;
}

// * return number of object in world
const char *get_objs_in_world(ObjData *obj) {
	int i;
	static char retval[16];
	if (!obj)
		return "";
	if ((i = GET_OBJ_RNUM(obj)) < 0) {
		log("DG_SCRIPTS : attemp count unknown object");
		return "";
	}
	sprintf(retval, "%d", obj_proto.actual_count(i));
	return retval;
}

// * return number of obj|mob in world by vnum
int gcount_char_vnum(long n) {
	int count = 0;

	for (const auto &ch : character_list) {
		if (n == GET_MOB_VNUM(ch)) {
			count++;
		}
	}

	return (count);
}

int count_char_vnum(long n) {
	int i;
	if ((i = real_mobile(n)) < 0)
		return 0;
	return (mob_index[i].total_online);
}

inline auto gcount_obj_vnum(long n) {
	const auto i = real_object(n);

	if (i < 0) {
		return 0;
	}

	return obj_proto.CountInWorld(i);
}

inline auto count_obj_vnum(long n) {
	const auto i = real_object(n);

	if (i < 0) {
		return 0;
	}

	// Чот косячит таймер, решили переделать тригги, хоть и дольше
	//	if (check_unlimited_timer(obj_proto[i]))
	//		return 0;
	return obj_proto.actual_count(i);
}

/************************************************************
 * search by number routines
 ************************************************************/

// return object with UID n
ObjData *find_obj_by_id(const object_id_t id) {
	const auto result = world_objects.find_first_by_id(id);
	return result.get();
}

// return room with UID n
RoomData *find_room(long n) {
	n = real_room(n - kRoomToBase);

	if ((n >= kFirstRoom) && (n <= top_of_world))
		return world[n];

	return nullptr;
}

/************************************************************
 * search by VNUM routines
 ************************************************************/

/**
 * Возвращает id моба указанного внума.
 * \param num - если есть и больше 0 - возвращает id не первого моба, а указанного порядкового номера.
 */
int find_char_vnum(int vnum, int num = 0) {
	int count = 0;

	if (mob_id_by_vnum.contains(vnum)) {
		std::vector<long> list_idnum;
		list_idnum = mob_id_by_vnum[vnum];
		for (auto it : list_idnum) {
			if (count++ == num) {
				return it;
			}
		}
	}
	return -1;
}

// return room with VNUM n
// Внимание! Для комнаты UID = ROOM_ID_BASE+VNUM, т.к.
// RNUM может быть независимо изменен с помощью OLC
int find_room_vnum(long n) {
	//  return (real_room (n) != kNowhere) ? ROOM_ID_BASE + n : -1;
	return (real_room(n) != kNowhere) ? n : -1;
}

int find_room_uid(long n) {
	return (real_room(n) != kNowhere) ? kRoomToBase + n : -1;
	//return (real_room (n) != kNowhere) ? n : -1;
}

/************************************************************
 * generic searches based only on name
 ************************************************************/

// search the entire world for a char, and return a pointer
CharData *get_char(char *name) {
	CharData *i;

	// Отсекаем поиск левых UID-ов.
	if ((*name == UID_OBJ) || (*name == UID_ROOM))
		return nullptr;

	if (*name == UID_CHAR || *name == UID_CHAR_ALL) {
			i = find_char(atoi(name + 1));
		if (i && (i->IsNpc() || !GET_INVIS_LEV(i))) {
			return i;
		}
	} else {
		for (const auto &character : character_list) {
			const auto i = character.get();
			if (isname(name, i->GetCharAliases())
				&& (i->IsNpc()
					|| !GET_INVIS_LEV(i))) {
				return i;
			}
		}
	}

	return nullptr;
}

// returns the object in the world with name name, or NULL if not found
ObjData *get_obj(char *name, int/* vnum*/) {
	long id;

	if ((*name == UID_CHAR) || (*name == UID_ROOM) || (*name == UID_CHAR_ALL))
		return nullptr;

	if (*name == UID_OBJ) {
		id = atoi(name + 1);
		return find_obj_by_id(id);
	} else {
		const auto result = world_objects.find_by_name(name);
		return result.get();
	}
	return nullptr;
}

// finds room by with name.  returns NULL if not found
RoomData *get_room(char *name) {
	int nr;

	if ((*name == UID_CHAR) || (*name == UID_OBJ) || (*name == UID_CHAR_ALL))
		return nullptr;

	if (*name == UID_ROOM)
		return find_room(atoi(name + 1));
	else if ((nr = real_room(atoi(name))) == kNowhere)
		return nullptr;
	else
		return world[nr];
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching with the person owing the object
 */
CharData *get_char_by_obj(ObjData *obj, char *name) {
	CharData *ch;

	if ((*name == UID_ROOM) || (*name == UID_OBJ))
		return nullptr;

	if (*name == UID_CHAR || *name == UID_CHAR_ALL) {
		ch = find_char(atoi(name + 1));

		if (ch && (ch->IsNpc() || !GET_INVIS_LEV(ch)))
			return ch;
	} else {
		if (obj->get_carried_by()
			&& isname(name, obj->get_carried_by()->GetCharAliases())
			&& (obj->get_carried_by()->IsNpc()
				|| !GET_INVIS_LEV(obj->get_carried_by()))) {
			return obj->get_carried_by();
		}

		if (obj->get_worn_by()
			&& isname(name, obj->get_worn_by()->GetCharAliases())
			&& (obj->get_worn_by()->IsNpc()
				|| !GET_INVIS_LEV(obj->get_worn_by()))) {
			return obj->get_worn_by();
		}

		for (const auto &ch : character_list) {
			if (isname(name, ch->GetCharAliases())
				&& (ch->IsNpc()
					|| !GET_INVIS_LEV(ch))) {
				return ch.get();
			}
		}
	}

	return nullptr;
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
CharData *get_char_by_room(RoomData *room, char *name) {
	CharData *ch;

	if (*name == UID_ROOM
		|| *name == UID_OBJ) {
		return nullptr;
	}

	if (*name == UID_CHAR || *name == UID_CHAR_ALL) {
		ch = find_char(atoi(name + 1));

		if (ch
			&& (ch->IsNpc()
				|| !GET_INVIS_LEV(ch))) {
			return ch;
		}
	} else {
		for (const auto &ch : room->people) {
			if (isname(name, ch->GetCharAliases())
				&& (ch->IsNpc()
					|| !GET_INVIS_LEV(ch))) {
				return ch;
			}
		}

		for (const auto &ch : character_list) {
			if (isname(name, ch->GetCharAliases())
				&& (ch->IsNpc()
					|| !GET_INVIS_LEV(ch))) {
				return ch.get();
			}
		}
	}

	return nullptr;
}

/*
 * returns the object in the world with name name, or NULL if not found
 * search based on obj
 */
ObjData *get_obj_by_obj(ObjData *obj, char *name) {
	ObjData *i = nullptr;
	int rm;
	long id;

	if ((*name == UID_ROOM) || (*name == UID_CHAR) || *name == UID_CHAR_ALL)
		return nullptr;

	if (!str_cmp(name, "self") || !str_cmp(name, "me"))
		return obj;

	if (obj->get_contains() && (i = get_obj_in_list(name, obj->get_contains())))
		return i;

	if (obj->get_in_obj()) {
		if (*name == UID_OBJ) {
			id = atoi(name + 1);

			if (id == obj->get_in_obj()->get_id()) {
				return obj->get_in_obj();
			}
		} else if (isname(name, obj->get_in_obj()->get_aliases())) {
			return obj->get_in_obj();
		}
	} else if (obj->get_worn_by() && (i = get_object_in_equip(obj->get_worn_by(), name))) {
		return i;
	} else if (obj->get_carried_by() && (i = get_obj_in_list(name, obj->get_carried_by()->carrying))) {
		return i;
	} else if (((rm = obj_room(obj)) != kNowhere) && (i = get_obj_in_list(name, world[rm]->contents))) {
		return i;
	}

	if (*name == UID_OBJ) {
		id = atoi(name + 1);

		i = world_objects.find_first_by_id(id).get();
	} else {
		i = world_objects.find_by_name(name).get();
	}

	return i;
}

// returns obj with name
ObjData *get_obj_by_room(RoomData *room, char *name) {
	ObjData *obj;
	long id;

	if ((*name == UID_ROOM) || (*name == UID_CHAR) || *name == UID_CHAR_ALL)
		return nullptr;

	if (*name == UID_OBJ) {
		id = atoi(name + 1);

		for (obj = room->contents; obj; obj = obj->get_next_content()) {
			if (id == obj->get_id()) {
				return obj;
			}
		}

		return world_objects.find_first_by_id(id).get();
	} else {
		for (obj = room->contents; obj; obj = obj->get_next_content()) {
			if (isname(name, obj->get_aliases())) {
				return obj;
			}
		}

		return world_objects.find_by_name(name).get();
	}

	return nullptr;
}

// returns obj with name
ObjData *get_obj_by_char(CharData *ch, char *name) {
	ObjData *obj;
	long id;

	if ((*name == UID_ROOM) || (*name == UID_CHAR) || *name == UID_CHAR_ALL)
		return nullptr;

	if (*name == UID_OBJ) {
		id = atoi(name + 1);
		if (ch) {
			for (obj = ch->carrying; obj; obj = obj->get_next_content()) {
				if (id == obj->get_id()) {
					return obj;
				}
			}
		}

		return world_objects.find_first_by_id(id).get();
	} else {
		if (ch) {
			for (obj = ch->carrying; obj; obj = obj->get_next_content()) {
				if (isname(name, obj->get_aliases())) {
					return obj;
				}
			}
		}

		return world_objects.find_by_name(name).get();
	}

	return nullptr;
}

// checks every PLUSE_SCRIPT for random triggers
void script_trigger_check() {
	utils::CExecutionTimer timercheck;
	std::stringstream buffer;
	long amount, sum = 0;
	long alarge_amount = 0;
	CharData *who = nullptr;
	ZoneVnum last_zone = -1;
	bool IsEmpty;

	character_list.foreach_on_copy([&last_zone, &IsEmpty, &amount, &alarge_amount, &sum, &who](const CharData::shared_ptr &ch) {
		if (!who)
			who = ch.get();
		if (SCRIPT(ch)->has_triggers()) {
			auto sc = SCRIPT(ch).get();
			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_RANDOM) || IS_SET(SCRIPT_TYPES(sc), MTRIG_RANDOM_GLOBAL)) {
				utils::CExecutionTimer timer;
				random_mtrigger(ch.get());
				amount = timer.delta().count();
				sum += amount;
				if (amount > alarge_amount) {
					alarge_amount = amount;
					who = ch.get();
				}
			}
		}
	});
	buffer << "MOB random trigger: самый долгий у моба [" << GET_MOB_VNUM(who) << "] время выполнения - " << alarge_amount << " ms" << " сумма всего: " << sum << " ms.";
	log("%s", buffer.str().c_str());
	buffer.str("");
	alarge_amount = 0;
	sum = 0;
	ObjData *what = nullptr;
	last_zone = -1;
	world_objects.foreach_on_copy([&last_zone, &IsEmpty, &amount, &alarge_amount, &sum, &what](const ObjData::shared_ptr &obj) {
		if (!obj->get_in_obj()) {
			if (!what)
				what = obj.get();
			if (obj.get()->has_flag(EObjFlag::kNamed)) {
				if (obj->get_worn_by() && number(1, 100) <= 5) {
					NamedStuff::wear_msg(obj->get_worn_by(), obj.get());
				}
			} else if (obj->get_script()->has_triggers()) {
				auto sc = obj->get_script().get();
				if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM_GLOBAL) || IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM)) {
					utils::CExecutionTimer timer;
					random_otrigger(obj.get());
					amount = timer.delta().count();
					sum += amount;
					if (amount > alarge_amount) {
						alarge_amount = amount;
						what = obj.get();
					}
				}
			}
		}
	});
	buffer << "OBJ random trigger: самый долгий у объекта [" << GET_OBJ_VNUM(what) << "] время выполнения - " << alarge_amount << " ms" << " сумма всего: " << sum << " ms.";
	log("%s", buffer.str().c_str());
	buffer.str("");
	alarge_amount = 0;
	sum = 0;
	RoomData *where = nullptr;
	for (std::size_t nr = kFirstRoom; nr <= static_cast<std::size_t>(top_of_world); nr++) {
		if (SCRIPT(world[nr])->has_triggers()) {
			auto room = world[nr];
			auto sc = SCRIPT(room).get();
			if (!where)
				where = room;
			if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) || IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM_GLOBAL)) {
				utils::CExecutionTimer timer;
				random_wtrigger(room, room->room_vn, sc, sc->types, sc->trig_list);
				amount = timer.delta().count();
				sum += amount;
				if (amount > alarge_amount) {
					alarge_amount = amount;
					where = room;
				}
			}
		}
	}
	buffer << "WLD random trigger: самый долгий у комнаты [" << where->room_vn << "] время выполнения - " << alarge_amount << " ms" << " сумма всего: " << sum << " ms." << std::endl;
	buffer << "script_trigger_check() всего: " << timercheck.delta().count() <<" ms.";
	log("%s", buffer.str().c_str());
}

// проверка каждый час на триги изменении времени
void script_timechange_trigger_check(const int time, const int time_day) {
	utils::CExecutionTimer timercheck;
	std::stringstream buffer;

	for (const auto &ch : character_list) {
		if (SCRIPT(ch)->has_triggers()) {
			auto sc = SCRIPT(ch).get();
			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_TIMECHANGE)
					&& (!is_empty(world[ch->in_room]->zone_rn))) {
				timechange_mtrigger(ch.get(), time, time_day);
			}
		}
	}

	world_objects.foreach_on_copy([&](const ObjData::shared_ptr &obj) {
		if (obj->get_script()->has_triggers()) {
			auto sc = obj->get_script().get();
			if (IS_SET(SCRIPT_TYPES(sc), OTRIG_TIMECHANGE)) {
				timechange_otrigger(obj.get(), time, time_day);
			}
		}
	});

	for (std::size_t nr = kFirstRoom; nr <= static_cast<std::size_t>(top_of_world); nr++) {
		if (SCRIPT(world[nr])->has_triggers()) {
			auto room = world[nr];
			auto sc = SCRIPT(room).get();
			if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIMECHANGE)
					&& (!is_empty(room->zone_rn))) {
				timechange_wtrigger(room, time, time_day);
			}
		}
	}
	buffer << "script_timechange_check() всего: " << timercheck.delta().count() <<" ms.";
	log("%s", buffer.str().c_str());
}

EVENT(trig_wait_event) {
	struct wait_event_data *wait_event_obj = (struct wait_event_data *) info;
	Trigger *trig;
	void *go;
	int type;

	trig = wait_event_obj->trigger;
	go = wait_event_obj->go;
	type = wait_event_obj->type;

	GET_TRIG_WAIT(trig) = nullptr;

	script_driver(go, trig, type, TRIG_RESTART);
	free(wait_event_obj);
}

void do_stat_trigger(CharData *ch, Trigger *trig) {
	char sb[kMaxExtendLength];

	if (!trig) {
		log("SYSERR: NULL trigger passed to do_stat_trigger.");
		return;
	}

	sprintf(sb, "Name: '%s%s%s',  VNum: [%s%5d%s], RNum: [%5d]\r\n",
			CCYEL(ch, C_NRM), GET_TRIG_NAME(trig), CCNRM(ch, C_NRM),
			CCGRN(ch, C_NRM), GET_TRIG_VNUM(trig), CCNRM(ch, C_NRM), GET_TRIG_RNUM(trig));
	SendMsgToChar(sb, ch);

	if (trig->get_attach_type() == MOB_TRIGGER) {
		SendMsgToChar("Trigger Intended Assignment: Mobiles\r\n", ch);
		sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
	} else if (trig->get_attach_type() == OBJ_TRIGGER) {
		SendMsgToChar("Trigger Intended Assignment: Objects\r\n", ch);
		sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf);
	} else if (trig->get_attach_type() == WLD_TRIGGER) {
		SendMsgToChar("Trigger Intended Assignment: Rooms\r\n", ch);
		sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf);
	} else {
		SendMsgToChar(ch, "Trigger Intended Assignment: undefined (attach_type=%d)\r\n",
					  static_cast<int>(trig->get_attach_type()));
	}

	sprintf(sb, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
			buf, GET_TRIG_NARG(trig), !trig->arglist.empty() ? trig->arglist.c_str() : "None");

	strcat(sb, "Commands:\r\n   ");

	auto cmd_list = *trig->cmdlist;
	while (cmd_list) {
		if (!cmd_list->cmd.empty()) {
			strcat(sb, cmd_list->cmd.c_str());
			strcat(sb, "\r\n   ");
		}

		cmd_list = cmd_list->next;
	}

	page_string(ch->desc, sb, 1);
}

// find the name of what the uid points to
void find_uid_name(char *uid, char *name) {
	CharData *ch;
	ObjData *obj;

	if ((ch = get_char(uid))) {
		strcpy(name, ch->GetCharAliases().c_str());
	} else if ((obj = get_obj(uid))) {
		strcpy(name, obj->get_aliases().c_str());
	} else {
		sprintf(name, "uid = %s, (not found)", uid + 1);
	}
}

const auto FOREACH_LIST_GUID = "{18B3D8D1-240E-4D60-AEAB-6748580CA460}";
const auto FOREACH_LIST_POS_GUID = "{4CC4E031-7376-4EED-AD4F-2FD0DC8D4E2D}";

// некоторые dg-функции (например foreach) создают внутренние переменные для работы
// выводим их в удобочитаемой форме, чтоб никого не смущали непонятные переменные в выводе
static std::string print_variable_name(const std::string &name) {
	static std::map<std::string, std::string> text_mapping_list = {
		{FOREACH_LIST_GUID, "[list] "},
		{FOREACH_LIST_POS_GUID, "[position] "}
	};

	// включение/отключение вывода внутреннего состояния foreach
	const bool display_state_vars = true;

	std::string result = name;

	for (const auto &text_mapping : text_mapping_list) {
		const std::string &guid_name = text_mapping.first;
		const std::string &print_text = text_mapping.second;

		const int guid_start_offcet = name.length() - guid_name.length();
		if (guid_start_offcet > 0 && guid_name == name.substr(guid_start_offcet)) {
			if (display_state_vars) {
				result = print_text;
				result.append(name.substr(0, guid_start_offcet));
			} else {
				result = std::string();
			}
			break;
		}
	}

	return result;
}

// general function to display stats on script sc
void script_stat(CharData *ch, Script *sc) {
	struct TriggerVar *tv;
	char name[kMaxInputLength];
	char namebuf[kMaxInputLength];

	sprintf(buf, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
	SendMsgToChar(buf, ch);
	sprintf(buf, "Global context: %ld\r\n", sc->context);
	SendMsgToChar(buf, ch);

	for (tv = sc->global_vars; tv; tv = tv->next) {
		sprintf(namebuf, "%s:%ld", tv->name, tv->context);
		if (*(tv->value) == UID_CHAR || *(tv->value) == UID_ROOM || *(tv->value) == UID_OBJ || *(tv->value) == UID_CHAR_ALL) {
			find_uid_name(tv->value, name);
			sprintf(buf, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name, name);
		} else
			sprintf(buf, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name, tv->value);
		SendMsgToChar(buf, ch);
	}

	for (auto t : sc->trig_list) {
		sprintf(buf, "\r\n  Trigger: %s%s%s, VNum: [%s%5d%s], RNum: [%5d]\r\n",
				CCYEL(ch, C_NRM), GET_TRIG_NAME(t), CCNRM(ch, C_NRM),
				CCGRN(ch, C_NRM), GET_TRIG_VNUM(t), CCNRM(ch, C_NRM), GET_TRIG_RNUM(t));
		SendMsgToChar(buf, ch);

		if (t->get_attach_type() == MOB_TRIGGER) {
			SendMsgToChar("  Trigger Intended Assignment: Mobiles\r\n", ch);
			sprintbit(GET_TRIG_TYPE(t), trig_types, buf1);
		} else if (t->get_attach_type() == OBJ_TRIGGER) {
			SendMsgToChar("  Trigger Intended Assignment: Objects\r\n", ch);
			sprintbit(GET_TRIG_TYPE(t), otrig_types, buf1);
		} else if (t->get_attach_type() == WLD_TRIGGER) {
			SendMsgToChar("  Trigger Intended Assignment: Rooms\r\n", ch);
			sprintbit(GET_TRIG_TYPE(t), wtrig_types, buf1);
		} else {
			SendMsgToChar(ch, "Trigger Intended Assignment: undefined (attach_type=%d)\r\n",
						  static_cast<int>(t->get_attach_type()));
		}
		std::stringstream buffer;
		buffer << "  Trigger Type: " << buf1 << ", Numeric Arg:" << GET_TRIG_NARG(t)
			   << " , Arg list:" << !t->arglist.empty() ? t->arglist.c_str() : "None";
//		sprintf(buf, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
//			buf1, GET_TRIG_NARG(t), !t->arglist.empty() ? t->arglist.c_str() : "None");
		SendMsgToChar(buffer.str(), ch);

		if (GET_TRIG_WAIT(t)) {
			if (t->curr_state != nullptr) {
				sprintf(buf, "    Wait: %d, Current line: %s\r\n",
						GET_TRIG_WAIT(t)->time_remaining, t->curr_state->cmd.c_str());
				SendMsgToChar(buf, ch);
			} else {
				sprintf(buf, "    Wait: %d\r\n", GET_TRIG_WAIT(t)->time_remaining);
				SendMsgToChar(buf, ch);
			}

			sprintf(buf, "  Variables: %s\r\n", GET_TRIG_VARS(t) ? "" : "None");
			SendMsgToChar(buf, ch);

			for (tv = GET_TRIG_VARS(t); tv; tv = tv->next) {
				const std::string var_name = print_variable_name(tv->name);
				if (!var_name.empty()) {
					if (*(tv->value) == UID_CHAR || *(tv->value) == UID_ROOM || *(tv->value) == UID_OBJ || *(tv->value) == UID_CHAR_ALL) {
						find_uid_name(tv->value, name);
						sprintf(buf, "    %15s:  %s\r\n", var_name.c_str(), name);
					} else {
						sprintf(buf, "    %15s:  %s\r\n", var_name.c_str(), tv->value);
					}
					SendMsgToChar(buf, ch);
				}
			}
		}
	}
}

void do_sstat_room(CharData *ch) {
	RoomData *rm = world[ch->in_room];

	do_sstat_room(rm, ch);

}

void do_sstat_room(RoomData *rm, CharData *ch) {
	SendMsgToChar("Script information:\r\n", ch);
	if (!SCRIPT(rm)->has_triggers()) {
		SendMsgToChar("  None.\r\n", ch);
		return;
	}

	script_stat(ch, SCRIPT(rm).get());
}

void do_sstat_object(CharData *ch, ObjData *j) {
	SendMsgToChar("Script information:\r\n", ch);
	if (!j->get_script()->has_triggers()) {
		SendMsgToChar("  None.\r\n", ch);
		return;
	}

	script_stat(ch, j->get_script().get());
}

void do_sstat_character(CharData *ch, CharData *k) {
	SendMsgToChar("Script information:\r\n", ch);
	if (!SCRIPT(k)->has_triggers()) {
		SendMsgToChar("  None.\r\n", ch);
		return;
	}

	script_stat(ch, SCRIPT(k).get());
}

void print_worlds_vars(CharData *ch, std::optional<long> context)
{
	TriggerVar *current;

	SendMsgToChar("Worlds vars list:\r\n", ch);
	for (current = worlds_vars; current; current = current->next) {
		if (context && context.value() != current->context) {
			continue;
		}

		std::stringstream str_out;
		str_out << "Context: " << current->context;
		str_out << ", Name: " << (current->name ? current->name : "[not set]");
		str_out << ", Value: " << (current->value ? current->value : "[not set]");
		str_out << "\r\n";

		SendMsgToChar(str_out.str(), ch);
	}
}

/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 *
 * Return true if trigger successfully added
 * Return false if trigger with same rnum already exists and can not be added
 */
bool add_trigger(Script *sc, Trigger *t, int loc) {
	if (!t || !sc) {
		return false;
	}

	const bool added = sc->trig_list.add(t, 0 == loc);
	if (added) {
		SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);
		trigger_list.add(t);
	}

	return added;
}

void do_attach(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	ObjData *object;
	Trigger *trig;
	char targ_name[kMaxInputLength], trig_name[kMaxInputLength];
	char loc_name[kMaxInputLength];
	int loc, room, tn, rn;

	argument = two_arguments(argument, arg, trig_name);
	two_arguments(argument, targ_name, loc_name);

	if (!*arg || !*targ_name || !*trig_name) {
		SendMsgToChar("Usage: attach { mtr | otr | wtr } { trigger } { name } [ location ]\r\n", ch);
		return;
	}

	tn = atoi(trig_name);
	loc = (*loc_name) ? atoi(loc_name) : -1;

	rn = real_trigger(tn);
	if (rn >= 0
		&& ((utils::IsAbbr(arg, "mtr") && trig_index[rn]->proto->get_attach_type() != MOB_TRIGGER)
			|| (utils::IsAbbr(arg, "otr") && trig_index[rn]->proto->get_attach_type() != OBJ_TRIGGER)
			|| (utils::IsAbbr(arg, "wtr") && trig_index[rn]->proto->get_attach_type() != WLD_TRIGGER))) {
		tn = (utils::IsAbbr(arg, "mtr") ? 0 : utils::IsAbbr(arg, "otr") ? 1 : utils::IsAbbr(arg, "wtr") ? 2 : 3);
		sprintf(buf,
				"Trigger %d (%s) has wrong attach_type %s expected %s.\r\n",
				tn,
				GET_TRIG_NAME(trig_index[rn]->proto),
				attach_name[(int) trig_index[rn]->proto->get_attach_type()],
				attach_name[tn]);
		SendMsgToChar(buf, ch);
		return;
	}
	if (utils::IsAbbr(arg, "mtr")) {
		if ((victim = get_char_vis(ch, targ_name, EFind::kCharInWorld))) {
			if (victim->IsNpc())    // have a valid mob, now get trigger
			{
				rn = real_trigger(tn);
				if ((rn >= 0) && (trig = read_trigger(rn))) {
					sprintf(buf, "Trigger %d (%s) attached to %s.\r\n", tn, GET_TRIG_NAME(trig), GET_SHORT(victim));
					SendMsgToChar(buf, ch);

					if (!add_trigger(SCRIPT(victim).get(), trig, loc)) {
						extract_trigger(trig);
					}
				} else {
					SendMsgToChar("That trigger does not exist.\r\n", ch);
				}
			} else
				SendMsgToChar("Players can't have scripts.\r\n", ch);
		} else {
			SendMsgToChar("That mob does not exist.\r\n", ch);
		}
	} else if (utils::IsAbbr(arg, "otr")) {
		if ((object = get_obj_vis(ch, targ_name)))    // have a valid obj, now get trigger
		{
			rn = real_trigger(tn);
			if ((rn >= 0) && (trig = read_trigger(rn))) {
				sprintf(buf, "Trigger %d (%s) attached to %s.\r\n",
						tn, GET_TRIG_NAME(trig),
						(!object->get_short_description().empty() ? object->get_short_description().c_str()
																  : object->get_aliases().c_str()));
				SendMsgToChar(buf, ch);

				if (!add_trigger(object->get_script().get(), trig, loc)) {
					extract_trigger(trig);
				}
			} else
				SendMsgToChar("That trigger does not exist.\r\n", ch);
		} else
			SendMsgToChar("That object does not exist.\r\n", ch);
	} else if (utils::IsAbbr(arg, "wtr")) {
		if (a_isdigit(*targ_name) && !strchr(targ_name, '.')) {
			if ((room = find_target_room(ch, targ_name, 0)) != kNowhere)    // have a valid room, now get trigger
			{
				rn = real_trigger(tn);
				if ((rn >= 0) && (trig = read_trigger(rn))) {
					sprintf(buf, "Trigger %d (%s) attached to room %d.\r\n",
							tn, GET_TRIG_NAME(trig), world[room]->room_vn);
					SendMsgToChar(buf, ch);

					if (!add_trigger(world[room]->script.get(), trig, loc)) {
						extract_trigger(trig);
					}
				} else {
					SendMsgToChar("That trigger does not exist.\r\n", ch);
				}
			}
		} else {
			SendMsgToChar("You need to supply a room number.\r\n", ch);
		}
	} else {
		SendMsgToChar("Please specify 'mtr', otr', or 'wtr'.\r\n", ch);
	}
}

void do_detach(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim = nullptr;
	ObjData *object = nullptr;
	RoomData *room;
	char arg1[kMaxInputLength], arg2[kMaxInputLength], arg3[kMaxInputLength];
	char *trigger = 0;
	int tmp;

	argument = two_arguments(argument, arg1, arg2);
	one_argument(argument, arg3);

	if (!*arg1 || !*arg2) {
		SendMsgToChar("Usage: detach [ mob | object | room ] { target } { trigger |" " 'all' }\r\n", ch);
		return;
	}

	if (!str_cmp(arg1, "room")) {
		room = world[ch->in_room];
		if (!SCRIPT(room)->has_triggers()) {
			SendMsgToChar("This room does not have any triggers.\r\n", ch);
		} else if (!str_cmp(arg2, "all") || !str_cmp(arg2, "все")) {
			room->cleanup_script();

			SendMsgToChar("All triggers removed from room.\r\n", ch);
		} else if (SCRIPT(room)->remove_trigger(arg2)) {
			SendMsgToChar("Trigger removed.\r\n", ch);
		} else {
			SendMsgToChar("That trigger was not found.\r\n", ch);
		}
	} else {
		if (utils::IsAbbr(arg1, "mob")) {
			if (!(victim = get_char_vis(ch, arg2, EFind::kCharInWorld)))
				SendMsgToChar("No such mobile around.\r\n", ch);
			else if (!*arg3)
				SendMsgToChar("You must specify a trigger to remove.\r\n", ch);
			else
				trigger = arg3;
		} else if (utils::IsAbbr(arg1, "object")) {
			if (!(object = get_obj_vis(ch, arg2)))
				SendMsgToChar("No such object around.\r\n", ch);
			else if (!*arg3)
				SendMsgToChar("You must specify a trigger to remove.\r\n", ch);
			else
				trigger = arg3;
		} else {
			if ((object = get_object_in_equip_vis(ch, arg1, ch->equipment, &tmp)));
			else if ((object = get_obj_in_list_vis(ch, arg1, ch->carrying)));
			else if ((victim = get_char_room_vis(ch, arg1)));
			else if ((object = get_obj_in_list_vis(ch, arg1, world[ch->in_room]->contents)));
			else if ((victim = get_char_vis(ch, arg1, EFind::kCharInWorld)));
			else if ((object = get_obj_vis(ch, arg1)));
			else
				SendMsgToChar("Nothing around by that name.\r\n", ch);
			trigger = arg2;
		}

		if (victim) {
			if (!victim->IsNpc()) {
				SendMsgToChar("Players don't have triggers.\r\n", ch);
			} else if (!SCRIPT(victim)->has_triggers()) {
				SendMsgToChar("That mob doesn't have any triggers.\r\n", ch);
			} else if (!str_cmp(arg2, "all") || !str_cmp(arg2, "все")) {
				victim->cleanup_script();
				sprintf(buf, "All triggers removed from %s.\r\n", GET_SHORT(victim));
				SendMsgToChar(buf, ch);
			} else if (trigger
				&& SCRIPT(victim)->remove_trigger(trigger)) {
				SendMsgToChar("Trigger removed.\r\n", ch);
			} else {
				SendMsgToChar("That trigger was not found.\r\n", ch);
			}
		} else if (object) {
			if (!object->get_script()->has_triggers()) {
				SendMsgToChar("That object doesn't have any triggers.\r\n", ch);
			} else if (!str_cmp(arg2, "all") || !str_cmp(arg2, "все")) {
				object->cleanup_script();
				sprintf(buf, "All triggers removed from %s.\r\n",
						!object->get_short_description().empty() ? object->get_short_description().c_str()
																 : object->get_aliases().c_str());
				SendMsgToChar(buf, ch);
			} else if (object->get_script()->remove_trigger(trigger)) {
				SendMsgToChar("Trigger removed.\r\n", ch);
			} else {
				SendMsgToChar("That trigger was not found.\r\n", ch);
			}
		}
	}
}

// frees memory associated with var
void free_var_el(struct TriggerVar *var) {
	free(var->name);
	free(var->value);
	free(var);
}

void add_var_cntx(struct TriggerVar **var_list, const char *name, const char *value, long id)
/*++
	Добавление переменной в список с учетом контекста (СТРОГИЙ поиск).
	При добавлении в список локальных переменных контекст должен быть 0.

	var_list	- указатель на первый элемент списка переменных
	name		- имя переменной
	value		- значение переменной
	id			- контекст переменной
--*/
{
	struct TriggerVar *vd, *vd_prev;

	// Обращаю внимание, что при добавлении необходимо ТОЧНОЕ совпадение контекстов
	for (vd_prev = nullptr, vd = *var_list;
		 vd && ((vd->context != id) || str_cmp(vd->name, name)); vd_prev = vd, vd = vd->next);

	if (vd) {
		// Переменная существует, заменить значение
		free(vd->value);
	} else {
		// Создать новую переменную
		CREATE(vd, 1);
		CREATE(vd->name, strlen(name) + 1);
		strcpy(vd->name, name);

		vd->context = id;

		// Добавление переменной в список
		// Для нулевого контекста в конец списка, для ненулевого - в начало
		if (id) {
			vd->next = *var_list;
			*var_list = vd;
		} else {
			vd->next = nullptr;
			if (vd_prev)
				vd_prev->next = vd;
			else
				*var_list = vd;
		}
	}

	CREATE(vd->value, strlen(value) + 1);
	strcpy(vd->value, value);
}

struct TriggerVar *find_var_cntx(struct TriggerVar **var_list, const char *name, long id)
/*++
		Поиск переменной с учетом контекста (НЕСТРОГИЙ поиск).

		Поиск осуществляется по паре ИМЯ:КОНТЕКСТ.
		1. Имя переменной должно совпадать с параметром name
		2. Контекст переменной должен совпадать с параметром id, если
		   такой переменной нет, производится попытка найти переменную
		   с контекстом 0.

		var_list	- указатель на первый элемент списка переменных
		name		- имя переменной
		id			- контекст переменной
	--*/
{
	struct TriggerVar *vd;

	for (vd = *var_list; vd && ((vd->context && vd->context != id) || str_cmp(vd->name, name)); vd = vd->next);

	return vd;
}

int remove_var_cntx(struct TriggerVar **var_list, char *name, long id)
/*++
	Удаление переменной из списка с учетом контекста (СТРОГИЙ поиск).

	Поиск строгий.

	var_list	- указатель на первый элемент списка переменных
	name		- имя переменной
	id			- контекст переменной

	Возвращает:
	   1 - переменная найдена и удалена
	   0 - переменная не найдена

--*/
{
	struct TriggerVar *vd, *vd_prev;

	for (vd_prev = nullptr, vd = *var_list;
		 vd && ((vd->context != id) || str_cmp(vd->name, name)); vd_prev = vd, vd = vd->next);

	if (!vd)
		return 0;    // Не удалось найти

	if (vd_prev)
		vd_prev->next = vd->next;
	else
		*var_list = vd->next;

	free_var_el(vd);

	return 1;
}

bool CheckSript(const ObjData *go, const long type) {
	return !go->get_script()->is_purged() && go->get_script()->has_triggers()
		&& IS_SET(SCRIPT_TYPES(go->get_script()), type);
}

bool CheckScript(const CharData *go, const long type) {
	return !SCRIPT(go)->is_purged() && SCRIPT(go)->has_triggers() && IS_SET(SCRIPT_TYPES(go->script), type);
}

bool CheckSript(const RoomData *go, const long type) {
	return !SCRIPT(go)->is_purged() && SCRIPT(go)->has_triggers() && IS_SET(SCRIPT_TYPES(go->script), type);
}

// * Изменение указанной целочисленной константы
long gm_char_field(CharData *ch, char *field, char *subfield, long val) {
	int tmpval;
	if (*subfield) {
		sprintf(buf, "DG_Script: Set %s with <%s> for %s.", field, subfield, GET_NAME(ch));
		log("%s", buf);
		if (*subfield == '-')
			return (val - atoi(subfield + 1));
		else if (*subfield == '+')
			return (val + atoi(subfield + 1));
		else if ((tmpval = atoi(subfield)) > 0)
			return tmpval;
	}
	return val;
}

int text_processed(char *field, char *subfield, struct TriggerVar *vd, char *str) {
	*str = '\0';
	if (!vd || !vd->name || !vd->value)
		return false;

	if (!str_cmp(field, "strlen")) {
		sprintf(str, "%lu", static_cast<unsigned long>(strlen(vd->value)));
		return true;
	} else if (!str_cmp(field, "trim")) {
		std::string str_to_trim = vd->value;
		utils::TrimLeft(str_to_trim);
		utils::TrimRight(str_to_trim);
		strcpy(str, str_to_trim.c_str());
		return true;
	} else if (!str_cmp(field, "fullword")) {
//не работает с русским
/*			std::string word = subfield;
			std::string sentence = vd->value;
			std::regex rgx("\\b" + word + "\\b"); 
			if (std::regex_search(sentence, rgx))
*/
		std::string sentence = vd->value;
		std::vector<std::string> tmpstr;
		std::istringstream is(sentence);
		std::string temp;
		while(is >> temp)
			tmpstr.push_back(temp); // перенесем фразу в вектор
		if (std::find(tmpstr.begin(), tmpstr.end(), subfield) != tmpstr.end())
// вариант номер два
/*		std::string sentence = vd->value;
		std::stringstream parser(sentence);
		std::istream_iterator<std::string> start(parser);
		std::istream_iterator<std::string> end;
		std::vector<std::string> words(start, end);
		if(find(words.begin(), words.end(), subfield) != words.end())
*/
			sprintf(str, "1");
		else
			sprintf(str, "0");
		return true;
	} else if (!str_cmp(field, "contains")) {
		if (str_str(vd->value, subfield))
			sprintf(str, "1");
		else
			sprintf(str, "0");
		return true;
	} else if (!str_cmp(field, "car")) {
		char *car = vd->value;
		while (*car && !isspace(*car))
			*str++ = *car++;
		*str = '\0';
		return true;
	} else if (!str_cmp(field, "cdr")) {
		char *cdr = vd->value;
		while (*cdr && !isspace(*cdr))
			cdr++;    // skip 1st field
		while (*cdr && isspace(*cdr))
			cdr++;    // skip to next
		while (*cdr)
			*str++ = *cdr++;
		*str = '\0';
		return true;
	} else if (!str_cmp(field, "words")) {
		int n = 0;
		// Подсчет количества слов или получение слова
		char buf1[kMaxTrglineLength];
		char buf2[kMaxTrglineLength];
		buf1[0] = 0;
		strcpy(buf2, vd->value);
		if (*subfield) {
			for (n = atoi(subfield); n; --n) {
				half_chop(buf2, buf1, buf2);
			}
			strcpy(str, buf1);
		} else {
			while (buf2[0] != 0) {
				half_chop(buf2, buf1, buf2);
				++n;
			}
			sprintf(str, "%d", n);
		}
		return true;
	} else if (!str_cmp(field, "mudcommand"))    // find the mud command returned from this text
	{
		// NOTE: you may need to replace "cmd_info" with "complete_cmd_info",
		// depending on what patches you've got applied.
		extern const struct command_info cmd_info[];
		// on older source bases:    extern struct command_info *cmd_info;
		int cmd = 0;
		const size_t length = strlen(vd->value);
		while (*cmd_info[cmd].command != '\n') {
			if (!strncmp(cmd_info[cmd].command, vd->value, length)) {
				break;
			}
			cmd++;
		}

		if (*cmd_info[cmd].command == '\n')
			strcpy(str, "");
		else
			strcpy(str, cmd_info[cmd].command);
		return true;
	}

	return false;
}

bool IsUID(const char *name) {
	if (*name == UID_CHAR
			|| *name == UID_ROOM
			|| *name == UID_OBJ
			|| *name == UID_CHAR_ALL) {
		return true;
	}
return false;
}

void find_replacement(void *go,
					  Script *sc,
					  Trigger *trig,
					  int type,
					  char *var,
					  char *field,
					  char *subfield,
					  char *str) {
	struct TriggerVar *vd = nullptr;
	CharData *ch = nullptr, *c = nullptr, *rndm;
	ObjData *obj = nullptr, *o = nullptr;
	RoomData *room = nullptr, *r = nullptr;
	char *name = nullptr;
	int num = 0, count = 0, i;
	char uid_type = '\0';
	char tmp[kMaxTrglineLength] = {};

	const char *send_cmd[] = {"msend", "osend", "wsend"};
	const char *echo_cmd[] = {"mecho", "oecho", "wecho"};
	const char *echoaround_cmd[] = {"mechoaround", "oechoaround", "wechoaround"};
	const char *door[] = {"mdoor", "odoor", "wdoor"};
	const char *force[] = {"mforce", "oforce", "wforce"};
	const char *load[] = {"mload", "oload", "wload"};
	const char *purge[] = {"mpurge", "opurge", "wpurge"};
	const char *teleport[] = {"mteleport", "oteleport", "wteleport"};
	const char *damage[] = {"mdamage", "odamage", "wdamage"};
	const char *featturn[] = {"mfeatturn", "ofeatturn", "wfeatturn"};
	const char *skillturn[] = {"mskillturn", "oskillturn", "wskillturn"};
	const char *skilladd[] = {"mskilladd", "oskilladd", "wskilladd"};
	const char *spellturn[] = {"mspellturn", "ospellturn", "wspellturn"};
	const char *spelladd[] = {"mspelladd", "ospelladd", "wspelladd"};
	const char *spellitem[] = {"mspellitem", "ospellitem", "wspellitem"};
	const char *portal[] = {"mportal", "oportal", "wportal"};
	const char *at[] = {"mat", "oat", "wat"};
	const char *zoneecho[] = {"mzoneecho", "ozoneecho", "wzoneecho"};
	const char *spellturntemp[] = {"mspellturntemp", "ospellturntemp", "wspellturntemp"};

	if (!subfield) {
		subfield = nullptr;    // Чтобы проверок меньше было
	}

	// X.global() will have a NULL trig
	if (trig)
		vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);
	if (!vd)
		vd = find_var_cntx(&(sc->global_vars), var, sc->context);
	if (!vd)
		vd = find_var_cntx(&worlds_vars, var, sc->context);

	*str = '\0';

	if (!field || !*field) {
		if (vd) {
			strcpy(str, vd->value);
		} else {
			if (!str_cmp(var, "self")) {
				long uid;
				// заменить на UID
				switch (type) {
					case MOB_TRIGGER: uid = GET_ID((CharData *) go);
						uid_type = UID_CHAR;
						break;

					case OBJ_TRIGGER: uid = ((ObjData *) go)->get_id();
						uid_type = UID_OBJ;
						break;

					case WLD_TRIGGER: uid = find_room_uid(((RoomData *) go)->room_vn);
						uid_type = UID_ROOM;
						break;

					default: strcpy(str, "self");
						return;
				}
				sprintf(str, "%c%ld", uid_type, uid);
			} else if (!str_cmp(var, "door"))
				strcpy(str, door[type]);
			else if (!str_cmp(var, "force"))
				strcpy(str, force[type]);
			else if (!str_cmp(var, "load"))
				strcpy(str, load[type]);
			else if (!str_cmp(var, "purge"))
				strcpy(str, purge[type]);
			else if (!str_cmp(var, "teleport"))
				strcpy(str, teleport[type]);
			else if (!str_cmp(var, "damage"))
				strcpy(str, damage[type]);
			else if (!str_cmp(var, "send"))
				strcpy(str, send_cmd[type]);
			else if (!str_cmp(var, "echo"))
				strcpy(str, echo_cmd[type]);
			else if (!str_cmp(var, "echoaround"))
				strcpy(str, echoaround_cmd[type]);
			else if (!str_cmp(var, "featturn"))
				strcpy(str, featturn[type]);
			else if (!str_cmp(var, "skillturn"))
				strcpy(str, skillturn[type]);
			else if (!str_cmp(var, "skilladd"))
				strcpy(str, skilladd[type]);
			else if (!str_cmp(var, "spellturn"))
				strcpy(str, spellturn[type]);
				else if (!str_cmp(var, "spellturntemp"))
				strcpy(str, spellturntemp[type]);
			else if (!str_cmp(var, "spelladd"))
				strcpy(str, spelladd[type]);
			else if (!str_cmp(var, "spellitem"))
				strcpy(str, spellitem[type]);
			else if (!str_cmp(var, "portal"))
				strcpy(str, portal[type]);
			else if (!str_cmp(var, "at"))
				strcpy(str, at[type]);
			else if (!str_cmp(var, "zoneecho"))
				strcpy(str, zoneecho[type]);
		}
		return;
	}

	if (vd && IsUID(vd->value)) {
		name = vd->value;
		switch (type) {
			case MOB_TRIGGER: ch = (CharData *) go;
				if (!name) {
					log("SYSERROR: null name (%s:%d %s)", __FILE__, __LINE__, __func__);
					break;
				}
				if (!ch) {
					log("SYSERROR: null ch (%s:%d %s)", __FILE__, __LINE__, __func__);
					break;
				}
				if ((o = get_object_in_equip(ch, name)));
				else if ((o = get_obj_in_list(name, ch->carrying)));
				else if ((c = SearchCharInRoomByName(name, ch->in_room)));
				else if ((o = get_obj_in_list(name, world[ch->in_room]->contents)));
				else if ((c = get_char(name)));
				else if ((o = get_obj(name, GET_TRIG_VNUM(trig))));
				else if ((r = get_room(name))) {
				}
				break;
			case OBJ_TRIGGER: obj = (ObjData *) go;
				if ((c = get_char_by_obj(obj, name)));
				else if ((o = get_obj_by_obj(obj, name)));
				else if ((r = get_room(name))) {
				}
				break;
			case WLD_TRIGGER: room = (RoomData *) go;
				if ((c = get_char_by_room(room, name)));
				else if ((o = get_obj_by_room(room, name)));
				else if ((r = get_room(name))) {
				}
				break;
		}
	} else {
		if (!str_cmp(var, "self")) {
			switch (type) {
				case MOB_TRIGGER: c = (CharData *) go;
					break;
				case OBJ_TRIGGER: o = (ObjData *) go;
					break;
				case WLD_TRIGGER: r = (RoomData *) go;
					break;
			}
		} else if (!str_cmp(var, "exist")) {
			if (!str_cmp(field, "mob") && (num = atoi(subfield)) > 0) {
				num = count_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%c", num > 0 ? '1' : '0');
			} else if (!str_cmp(field, "obj") && (num = atoi(subfield)) > 0) {
				num = gcount_obj_vnum(num);
				if (num >= 0)
					sprintf(str, "%c", num > 0 ? '1' : '0');
			} else if (!str_cmp(field, "pc")) {
				for (auto d = descriptor_list; d; d = d->next) {
					if (STATE(d) != CON_PLAYING) 
						continue;
					if (!str_cmp(subfield, GET_NAME(d->character))) {
						sprintf(str, "1");
						return;
					}
				}
				sprintf(str, "0");
			}
			return;
		} else if (!str_cmp(var, "world")) {
			num = atoi(subfield);
			if ((!str_cmp(field, "curobj") || !str_cmp(field, "curobjs")) && num > 0) {
				const auto rnum = real_object(num);
				const auto count = count_obj_vnum(num);
				if (count >= 0 && 0 <= rnum) {
					if (check_unlimited_timer(obj_proto[rnum].get())) {
						sprintf(str, "0");
					} else {
						sprintf(str, "%d", count);
					}
				} else {
					sprintf(str, "0");
				}
			} else if ((!str_cmp(field, "gameobj") || !str_cmp(field, "gameobjs")) && num > 0) {
				num = gcount_obj_vnum(num);
				if (num >= 0)
					sprintf(str, "%d", num);
			} else if (!str_cmp(field, "people") && num > 0) {
				sprintf(str, "%d", trgvar_in_room(num));
			} else if (!str_cmp(field, "zonenpc") && num > 0) {
				int from = 0, to = 0;
				GetZoneRooms(ZoneRnumFromVnum(num), &from , &to);
				for (const auto &tch : character_list) {
					if ((tch->IsNpc() && !IS_CHARMICE(tch)) && (tch->in_room >= from && tch->in_room <= to)) {
						snprintf(str + strlen(str), kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(tch));
					}
				}
			} else if (!str_cmp(field, "zonechar") && num > 0) {
				int from = 0, to = 0;
				GetZoneRooms(ZoneRnumFromVnum(num), &from , &to);
				for (const auto &tch : character_list) {
					if (!tch->IsNpc() && !tch->desc)
						continue;
					if ((IS_CHARMICE(tch) || !tch->IsNpc()) && (tch->in_room >= from && tch->in_room <= to)) {
						snprintf(str + strlen(str), kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(tch));
					}
				}
			} else if (!str_cmp(field, "zonepc") && num > 0) {
				int from = 0, to = 0;
				GetZoneRooms(ZoneRnumFromVnum(num), &from , &to);
				for (auto d = descriptor_list; d; d = d->next) {
					if (STATE(d) != CON_PLAYING) 
						continue;
					if (d->character->in_room >= from && d->character->in_room <= to) {
						snprintf(str + strlen(str), kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(d->character));
					}
				}
			} else if (!str_cmp(field, "zoneall") && num > 0) {
				int from =0, to = 0;
				GetZoneRooms(ZoneRnumFromVnum(num), &from , &to);
				for (const auto &tch : character_list) {
					if (!tch->IsNpc() && !tch->desc)
						continue;
					if (tch->in_room >= from && tch->in_room <= to) {
						snprintf(str + strlen(str), kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(tch));
					}
				}
			} else if ((!str_cmp(field, "curmob") || !str_cmp(field, "curmobs")) && num > 0) {
				num = count_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%d", num);
			} else if (!str_cmp(field, "zreset") && num > 0) {
				std::vector<ZoneRnum> zone_repop_list;
				ZoneRnum zrn = get_zone_rnum_by_zone_vnum(num);
				zone_repop_list.push_back(zrn);
				RepopDecay(zone_repop_list);
				reset_zone(zrn);
			} else if (!str_cmp(field, "mob") && num > 0) {
				num = find_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%c%d", UID_CHAR, num);
			} else if (!str_cmp(field, "obj") && num > 0) {
				num = find_obj_by_id_vnum__find_replacement(num);

				if (num >= 0)
					sprintf(str, "%c%d", UID_OBJ, num);
			} else if (!str_cmp(field, "room") && num > 0) {
				num = find_room_uid(num);
				if (num >= 0)
					sprintf(str, "%c%d", UID_ROOM, num);
			}
				//Polud world.maxobj(vnum) показывает максимальное количество предметов в мире,
				//которое прописано в самом предмете с указанным vnum
			else if (!str_cmp(field, "CanBeLoaded") && num > 0) {
				const auto rnum = real_object(num);
				if (rnum >= 0) {
					// если у прототипа беск.таймер,
					// то их оч много в мире
					if (check_unlimited_timer(obj_proto[rnum].get()) || (GET_OBJ_MIW(obj_proto[rnum]) < 0)) {
						sprintf(str, "1");
						return;
					}
					const auto count = count_obj_vnum(rnum);
					if (count < GET_OBJ_MIW(obj_proto[rnum])) {
						sprintf(str, "1");
						return;
					}
					sprintf(str, "0");
					return;
				}
			}
			else if ((!str_cmp(field, "maxobj") || !str_cmp(field, "maxobjs")) && num > 0) {
				num = real_object(num);
				if (num >= 0) {
					// если у прототипа беск.таймер,
					// то их оч много в мире
					if (check_unlimited_timer(obj_proto[num].get()) || (GET_OBJ_MIW(obj_proto[num]) < 0))
						sprintf(str, "9999999");
					else
						sprintf(str, "%d", GET_OBJ_MIW(obj_proto[num]));
				}
			}
			//-Polud
			return;
		} else if (!str_cmp(var, "weather")) {
			if (!str_cmp(field, "temp")) {
				sprintf(str, "%d", weather_info.temperature);
			} else if (!str_cmp(field, "moon")) {
				sprintf(str, "%d", weather_info.moon_day);
			} else if (!str_cmp(field, "sky")) {
				num = -1;
				if ((num = atoi(subfield)) > 0)
					num = real_room(num);
				if (num != kNowhere)
					sprintf(str, "%d", GET_ROOM_SKY(num));
				else
					sprintf(str, "%d", weather_info.sky);
			} else if (!str_cmp(field, "type")) {
				char c;
				int wt = weather_info.weather_type;
				num = -1;
				if ((num = atoi(subfield)) > 0)
					num = real_room(num);
				if (num != kNowhere && world[num]->weather.duration > 0)
					wt = world[num]->weather.weather_type;
				for (c = 'a'; wt; ++c, wt >>= 1)
					if (wt & 1)
						sprintf(str + strlen(str), "%c", c);
			} else if (!str_cmp(field, "sunlight")) {
				switch (weather_info.sunlight) {
					case kSunDark: strcpy(str, "ночь");
						break;
					case kSunSet: strcpy(str, "закат");
						break;
					case kSunLight: strcpy(str, "день");
						break;
					case kSunRise: strcpy(str, "рассвет");
						break;
				}

			} else if (!str_cmp(field, "season")) {
				switch (weather_info.season) {
					case ESeason::kWinter: strcat(str, "зима");
						break;
					case ESeason::kSpring: strcat(str, "весна");
						break;
					case ESeason::kSummer: strcat(str, "лето");
						break;
					case ESeason::kAutumn: strcat(str, "осень");
						break;
				}
			}
			return;
		} else if (!str_cmp(var, "time")) {
			if (!str_cmp(field, "hour"))
				sprintf(str, "%d", time_info.hours);
			else if (!str_cmp(field, "day"))
				sprintf(str, "%d", time_info.day + 1);
			else if (!str_cmp(field, "month"))
				sprintf(str, "%d", time_info.month + 1);
			else if (!str_cmp(field, "year"))
				sprintf(str, "%d", time_info.year);
			return;
		} else if (!str_cmp(var, "date")) {
			time_t now_time = time(0);
			if (!str_cmp(field, "unix")) {
				sprintf(str, "%ld", static_cast<long>(now_time));
			} else if (!str_cmp(field, "exact")) {
				auto now = std::chrono::system_clock::now();
				auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
				sprintf(str, "%ld", now_ms.time_since_epoch().count());
			} else if (!str_cmp(field, "yday")) {
				strftime(str, kMaxInputLength, "%j", localtime(&now_time));
			} else if (!str_cmp(field, "wday")) {
				strftime(str, kMaxInputLength, "%w", localtime(&now_time));
			} else if (!str_cmp(field, "second")) {
				strftime(str, kMaxInputLength, "%S", localtime(&now_time));
			} else if (!str_cmp(field, "minute")) {
				strftime(str, kMaxInputLength, "%M", localtime(&now_time));
			} else if (!str_cmp(field, "hour")) {
				strftime(str, kMaxInputLength, "%H", localtime(&now_time));
			} else if (!str_cmp(field, "day")) {
				strftime(str, kMaxInputLength, "%d", localtime(&now_time));
			} else if (!str_cmp(field, "month")) {
				strftime(str, kMaxInputLength, "%m", localtime(&now_time));
			} else if (!str_cmp(field, "year")) {
				strftime(str, kMaxInputLength, "%y", localtime(&now_time));
			}
			return;
		} else if (!str_cmp(var, "random")) {
			if (!str_cmp(field, "char")
				|| !str_cmp(field, "pc")
				|| !str_cmp(field, "npc")
				|| !str_cmp(field, "all")) {
				rndm = nullptr;
				count = 0;
				if (type == MOB_TRIGGER) {
					ch = (CharData *) go;
					for (const auto c : world[ch->in_room]->people) {
						if (GET_INVIS_LEV(c)
							|| (c == ch)) {
							continue;
						}

						if ((*field == 'a')
							|| (*field == 'p' && !c->IsNpc())
							|| (*field == 'n' && c->IsNpc() && !IS_CHARMED(c))
							|| (*field == 'c' && (!c->IsNpc() || IS_CHARMED(c)))) {
							if (!number(0, count)) {
								rndm = c;
							}

							count++;
						}
					}
				} else if (type == OBJ_TRIGGER) {
					for (const auto c : world[obj_room((ObjData *) go)]->people) {
						if (GET_INVIS_LEV(c)) {
							continue;
						}

						if ((*field == 'a')
							|| (*field == 'p' && !c->IsNpc())
							|| (*field == 'n' && c->IsNpc() && !IS_CHARMED(c))
							|| (*field == 'c' && (!c->IsNpc() || IS_CHARMED(c)))) {
							if (!number(0, count)) {
								rndm = c;
							}

							count++;
						}
					}
				} else if (type == WLD_TRIGGER) {
					for (const auto c : ((RoomData *) go)->people) {
						if (GET_INVIS_LEV(c)) {
							continue;
						}

						if ((*field == 'a')
							|| (*field == 'p' && !c->IsNpc())
							|| (*field == 'n' && c->IsNpc() && !IS_CHARMED(c))
							|| (*field == 'c' && (!c->IsNpc() || IS_CHARMED(c)))) {
							if (!number(0, count)) {
								rndm = c;
							}

							count++;
						}
					}
				}

				if (rndm) {
					sprintf(str, "%c%ld", UID_CHAR, GET_ID(rndm));
				}
			} else {
				if (!str_cmp(field, "num")) {
					num = atoi(subfield);
				} else {
					num = atoi(field);
				}
				sprintf(str, "%d", (num > 0) ? number(1, num) : 0);
			}

			return;
		} else if (!str_cmp(var, "number")) {
			if (!str_cmp(field, "range")) {
				std::string tempstr = subfield;
				std::string s1 = tempstr.substr(0, tempstr.find(','));
				std::string s2 = tempstr.substr(tempstr.find(',') +1 );
				int x = atoi(s1.c_str());
				int y = atoi(s2.c_str());
				sprintf(str, "%d", number(x, y));
			}
		return;
		} else if (!str_cmp(var, "what")) {
			if (*subfield == UID_CHAR || *subfield == UID_CHAR_ALL)
				sprintf(str, "%s", "char");
			else if (*subfield == UID_OBJ)
				sprintf(str, "%s", "obj");
			else if (*subfield == UID_ROOM)
				sprintf(str, "%s", "room");
			else
				sprintf(str, "%s", "0");
			return;
		} else if (!str_cmp(var, "array")) {
			if (!str_cmp(field, "item")) {
				char *p = strchr(subfield, ',');
				int n = 0;
				if (!p) {
					int count = 0;
					for(const char * c = subfield; *c; ++c) {
						if (*c == ' ' && *(c + 1) != ' ') {
							++count;
						}
					}
					sprintf(str, "%d", count + 1); // +1 за первое слово
					return;
				}
				*(p++) = '\0';
				n = atoi(p);
				p = subfield;
				while (n) {
					char *retval = p;
					char tmp;
					for (; n; p++) {
						if (*p == ' ' || *p == '\0') {
							n--;
							if (n && *p == '\0') {
								str[0] = '\0';
								return;
							}
							if (!n) {
								tmp = *p;
								*p = '\0';
								sprintf(str, "%s", retval);
								*p = tmp;
								return;
							} else {
								retval = p + 1;
							}
						}
					}
				}
			}
			/*Вот тут можно наделать вот так еще:
			else if (!str_cmp(field, "pop")) {}
			else if (!str_cmp(field, "shift")) {}
			else if (!str_cmp(field, "push")) {}
			else if (!str_cmp(field, "unshift")) {}
			*/
			sprintf(str, "%s", "array some error");
		}
	}
	if (c) {
		if (!c->IsNpc() && !c->desc && name && *name == UID_CHAR) {
			CharacterLinkDrop = true;
		}
		if (name && *name == UID_CHAR_ALL) {
			uid_type = UID_CHAR_ALL;
		} else {
			uid_type = UID_CHAR;
		}

		auto done = true;
		if (text_processed(field, subfield, vd, str)) {
			return;
		} else if (!str_cmp(field, "global"))    // get global of something else
		{
			if (c->IsNpc()) {
				find_replacement(go, c->script.get(), nullptr, MOB_TRIGGER, subfield, nullptr, nullptr, str);
			}
		} else if (*field == 'u' || *field == 'U') {
			if (!str_cmp(field, "uniq")) {
				if (!c->IsNpc())
					sprintf(str, "%d", GET_UNIQUE(c));
			} else if (!str_cmp(field, "u")) {
				strcpy(str, GET_CH_SUF_2(c));
			} else if (!str_cmp(field, "UPiname")) {
				std::string tmpname = GET_PAD(c, 0);
				strcpy(str, colorCAP(tmpname).c_str());
			} else if (!str_cmp(field, "UPrname")) {
				std::string tmpname = GET_PAD(c, 1);
				strcpy(str, colorCAP(tmpname).c_str());
			} else if (!str_cmp(field, "UPdname")) {
				std::string tmpname = GET_PAD(c, 2);
				strcpy(str, colorCAP(tmpname).c_str());
			} else if (!str_cmp(field, "UPvname")) {
				std::string tmpname = GET_PAD(c, 3);
				strcpy(str, colorCAP(tmpname).c_str());
			} else if (!str_cmp(field, "UPtname")) {
				std::string tmpname = GET_PAD(c, 4);
				strcpy(str, colorCAP(tmpname).c_str());
			} else if (!str_cmp(field, "UPpname")) {
				std::string tmpname = GET_PAD(c, 5);
				strcpy(str, colorCAP(tmpname).c_str());
			} else if (!str_cmp(field, "UPname")) {
				std::string tmpname = GET_NAME(c);
				strcpy(str, colorCAP(tmpname).c_str());
				CharacterLinkDrop = false;
			} else if (!str_cmp(field, "unsetquest")) {
				if (*subfield && (num = atoi(subfield)) > 0) {
					c->quested_remove(num);
					strcpy(str, "1");
				}
			}
		} else if (!str_cmp(field, "iname")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->player_data.PNames[0] = subfield;
			}
			else
				strcpy(str, GET_PAD(c, 0));
		}
		else if (!str_cmp(field, "rname")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->player_data.PNames[1] = subfield;
			}
			else
				strcpy(str, GET_PAD(c, 1));
		}
		else if (!str_cmp(field, "dname")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->player_data.PNames[2] = subfield;
			}
			else
				strcpy(str, GET_PAD(c, 2));
		}
		else if (!str_cmp(field, "vname")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->player_data.PNames[3] = subfield;
			}
			else
				strcpy(str, GET_PAD(c, 3));
		}
		else if (!str_cmp(field, "tname")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->player_data.PNames[4] = subfield;
			}
			else
				strcpy(str, GET_PAD(c, 4));
		}
		else if (!str_cmp(field, "pname")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->player_data.PNames[5] = subfield;
			}
			else
				strcpy(str, GET_PAD(c, 5));
		}
		else if (!str_cmp(field, "name")) {
			if (*subfield) {
				if (strlen(subfield) > MAX_MOB_NAME)
					subfield[MAX_MOB_NAME - 1] = '\0';
				c->set_name(subfield);
			}
			else {
				strcpy(str, GET_NAME(c));
				CharacterLinkDrop = false;
			}
		}
		else if (!str_cmp(field, "description")) {
			if (*subfield) {
				sprintf(buf, "%s\r\n", std::string(subfield).c_str());
				c->player_data.long_descr = buf;
			}
			else {
				strcpy(str, c->player_data.long_descr.c_str());
			}
		}
		else if (!str_cmp(field, "alias")) {
			if (*subfield) {
				c->SetCharAliases(subfield);
			}
			else {
				strcpy(str, c->GetCharAliases().c_str());
			}
		}
		else if (!str_cmp(field, "id"))
			sprintf(str, "%c%ld", UID_CHAR, GET_ID(c));
		else if (!str_cmp(field, "level"))
			sprintf(str, "%d", GetRealLevel(c));
		else if (!str_cmp(field, "remort")) {
				sprintf(str, "%d", GetRealRemort(c));
		} else if (!str_cmp(field, "hitp")) {
			if (*subfield)
				GET_HIT(c) = (int) MAX(1, gm_char_field(c, field, subfield, (long) GET_HIT(c)));
			else
				sprintf(str, "%d", GET_HIT(c));
		} else if (!str_cmp(field, "hitpadd")) {
			if (*subfield)
				GET_HIT_ADD(c) = (int) gm_char_field(c, field, subfield, (long) GET_HIT_ADD(c));
			else
				sprintf(str, "%d", GET_HIT_ADD(c));
		} else if (!str_cmp(field, "maxhitp")) {
			if (*subfield && c->IsNpc()) // доступно тока мобам
				GET_MAX_HIT(c) = (int) gm_char_field(c, field, subfield, (long) GET_MAX_HIT(c));
			else
				sprintf(str, "%d", GET_MAX_HIT(c));
		} else if (!str_cmp(field, "mana")) {
			if (*subfield) {
				if (!c->IsNpc()) {
					c->mem_queue.stored = std::max(0L, gm_char_field(c, field, subfield, (long) c->mem_queue.stored));
				}
			} else {
				sprintf(str, "%d", c->mem_queue.stored);
			}
		} else if (!str_cmp(field, "maxmana")) {
			sprintf(str, "%d", GET_MAX_MANA(c));
		} else if (!str_cmp(field, "domination_kill")) {
			if (*subfield)
				c->player_specials->saved.kill_arena_dom = atoi(subfield);
			else
				sprintf(str, "%d", c->player_specials->saved.kill_arena_dom);
		} else if (!str_cmp(field, "domination_rip")) {
			if (*subfield)
				c->player_specials->saved.rip_arena_dom = atoi(subfield);
			else
				sprintf(str, "%d", c->player_specials->saved.rip_arena_dom);
		} else if (!str_cmp(field, "move")) {
			if (*subfield)
				GET_MOVE(c) = MAX(0, gm_char_field(c, field, subfield, GET_MOVE(c)));
			else
				sprintf(str, "%d", GET_MOVE(c));
		} else if (!str_cmp(field, "maxmove")) {
			sprintf(str, "%d", GET_MAX_MOVE(c));
		} else if (!str_cmp(field, "moveadd")) {
			if (*subfield)
				GET_MOVE_ADD(c) = (int) gm_char_field(c, field, subfield, (long) GET_MOVE_ADD(c));
			else
				sprintf(str, "%d", GET_MOVE_ADD(c));
		} else if (!str_cmp(field, "castsucc")) {
			if (*subfield)
				GET_CAST_SUCCESS(c) = (int) gm_char_field(c, field, subfield, (long) GET_CAST_SUCCESS(c));
			else
				sprintf(str, "%d", GET_CAST_SUCCESS(c));
		} else if (!str_cmp(field, "age")) {
			if (!c->IsNpc())
				sprintf(str, "%d", GET_REAL_AGE(c));
		} else if (!str_cmp(field, "hrbase")) {
			sprintf(str, "%d", GET_HR(c));
		} else if (!str_cmp(field, "hradd")) {
			if (*subfield)
				GET_HR_ADD(c) = (int) gm_char_field(c, field, subfield, (long) GET_HR(c));
			else
				sprintf(str, "%d", GET_HR_ADD(c));
		} else if (!str_cmp(field, "hr")) {
			sprintf(str, "%d", GET_REAL_HR(c));
		} else if (!str_cmp(field, "drbase")) {
			sprintf(str, "%d", GET_DR(c));
		} else if (!str_cmp(field, "dradd")) {
			if (*subfield)
				GET_DR_ADD(c) = (int) gm_char_field(c, field, subfield, (long) GET_DR(c));
			else
				sprintf(str, "%d", GET_DR_ADD(c));
		} else if (!str_cmp(field, "dr")) {
			sprintf(str, "%d", GetRealDamroll(c));
		} else if (!str_cmp(field, "acbase")) {
			sprintf(str, "%d", GET_AC(c));
		} else if (!str_cmp(field, "acadd")) {
			if (*subfield)
				GET_AC_ADD(c) = (int) gm_char_field(c, field, subfield, (long) GET_AC(c));
			else
				sprintf(str, "%d", GET_AC_ADD(c));
		} else if (!str_cmp(field, "ac")) {
			sprintf(str, "%d", GET_REAL_AC(c));
		} else if (!str_cmp(field, "morale")) // общая сумма морали
		{
			sprintf(str, "%d", c->calc_morale());
		} else if (!str_cmp(field, "moraleadd")) // добавочная мораль
		{
			if (*subfield)
				GET_MORALE(c) = (int) gm_char_field(c, field, subfield, (long) GET_MORALE(c));
			else
				sprintf(str, "%d", GET_MORALE(c));
		} else if (!str_cmp(field, "poison")) {
			if (*subfield)
				GET_POISON(c) = (int) gm_char_field(c, field, subfield, (long) GET_POISON(c));
			else
				sprintf(str, "%d", GET_POISON(c));
		} else if (!str_cmp(field, "initiative")) {
			if (*subfield)
				GET_INITIATIVE(c) = (int) gm_char_field(c, field, subfield, (long) GET_INITIATIVE(c));
			else
				sprintf(str, "%d", GET_INITIATIVE(c));
		} else if (!str_cmp(field, "linkdrop")) {
			if (!c->IsNpc() && !c->desc){
				sprintf(str, "1");
				CharacterLinkDrop = false; // чтоб триггер тут не прерывался для упавших в ЛД
			}
			else
				sprintf(str, "0");
		} else if (!str_cmp(field, "align")) {
			if (*subfield) {
				if (*subfield == '-')
					GET_ALIGNMENT(c) -= MAX(1, atoi(subfield + 1));
				else if (*subfield == '+')
					GET_ALIGNMENT(c) += MAX(1, atoi(subfield + 1));
			} else
				sprintf(str, "%d", GET_ALIGNMENT(c));
		} else if (!str_cmp(field, "religion")) {
			if (*subfield && ((atoi(subfield) == kReligionPoly) || (atoi(subfield) == kReligionMono)))
				GET_RELIGION(c) = atoi(subfield);
			else
				sprintf(str, "%d", GET_RELIGION(c));
		} else if ((!str_cmp(field, "restore")) || (!str_cmp(field, "fullrestore"))) {
			/* Так, тупо, но иначе ругается, мол слишком много блоков*/
			if (!str_cmp(field, "fullrestore")) {
				do_arena_restore(c, (char *) c->get_name().c_str(), 0, SCMD_RESTORE_TRIGGER);
				trig_log(trig, "был произведен вызов do_arena_restore!");
			} else {
				do_restore(c, (char *) c->get_name().c_str(), 0, SCMD_RESTORE_TRIGGER);
				trig_log(trig, "был произведен вызов do_restore!");
			}
		} else if (!str_cmp(field, "dispel")) {
			if (!c->affected.empty()) {
				SendMsgToChar("Вы словно заново родились!\r\n", c);
			}

			while (!c->affected.empty()) {
				c->affect_remove(c->affected.begin());
			}
		} else {
			done = false;
		}

		if (done) {
		} else if (!str_cmp(field, "hryvn")) {
			if (*subfield) {
				const long before = c->get_hryvn();
				int value;
				c->set_hryvn(MAX(0, gm_char_field(c, field, subfield, c->get_hryvn())));
				value = c->get_hryvn() - before;
				sprintf(buf, "<%s> {%d} получил триггером %d %s. [Trigger: %s, Vnum: %d]",
						GET_PAD(c, 0),
						GET_ROOM_VNUM(c->in_room),
						value,
						GetDeclensionInNumber(value, EWhat::kTorcU),
						GET_TRIG_NAME(trig),
						GET_TRIG_VNUM(trig));
				mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			} else
				sprintf(str, "%d", c->get_hryvn());
		} else if (!str_cmp(field, "point_nogata")) {
				if (*subfield) {
					c->set_nogata(MAX(0, gm_char_field(c, field, subfield, c->get_nogata())));
				}
				else
					sprintf(str, "%d", c->get_nogata());
		} else if (!str_cmp(field, "nogata")) {
			if (*subfield) {
				int val = 0, num;
				CharData *k;
				if (*subfield == '-') {
					val = atoi(subfield + 1);
					c->set_nogata(MAX(0, c->get_nogata() - val));
				}
				else if (*subfield == '+') {
					val = atoi(subfield + 1);
					if (val > 1) {
						k = c->has_master() ? c->get_master() : c;
						if (AFF_FLAGGED(k, EAffect::kGroup) && (k->in_room == c->in_room)) {
							num = 1;
						} else {
							num = 0;
						}
						for (auto f = k->followers; f; f = f->next) {
							if (AFF_FLAGGED(f->follower, EAffect::kGroup)
									&& !f->follower->IsNpc() && IN_ROOM(f->follower) == c->in_room) {
								num++;
							}
						}
						if (num > 1) {
							int share = val / num;
							int rest = val % num;
							if (AFF_FLAGGED(k, EAffect::kGroup) && IN_ROOM(k) == c->in_room && !k->IsNpc() && k != c)
								k->add_nogata(share);
							for (auto f = k->followers; f; f = f->next) {
								if (AFF_FLAGGED(f->follower, EAffect::kGroup)
										&& !f->follower->IsNpc() && IN_ROOM(f->follower) == c->in_room && f->follower != c) {
									f->follower->add_nogata(share);
								}
							}
							sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
									val, GetDeclensionInNumber(val, EWhat::kNogataU), num, share);
							SendMsgToChar(buf, c);
							if (rest > 0) {
								SendMsgToChar(c, "Как истинный еврей вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
											  rest,
											  GetDeclensionInNumber(rest, EWhat::kNogataU));
							}
							c->add_nogata(share+rest);
						}
					 	else {
							c->add_nogata(val);
						}
					}
				}
			}
			else {
				sprintf(str, "%d", c->get_nogata());
			}
		} else if (!str_cmp(field, "gold")) {
			if (*subfield) {
				const long before = c->get_gold();
				int value;
				c->set_gold(MAX(0, gm_char_field(c, field, subfield, c->get_gold())));
				value = c->get_gold() - before;
				sprintf(buf,
						"<%s> {%d} получил триггером %d %s. [Trigger: %s, Vnum: %d]",
						GET_PAD(c, 0),
						GET_ROOM_VNUM(c->in_room),
						value,
						GetDeclensionInNumber(value, EWhat::kMoneyU),
						GET_TRIG_NAME(trig),
						GET_TRIG_VNUM(trig));
				mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
				// клан-налог
				const long diff = c->get_gold() - before;
				split_or_clan_tax(c, diff);
				// стата для show money
				if (!c->IsNpc() && IN_ROOM(c) > 0) {
					MoneyDropStat::add(zone_table[world[IN_ROOM(c)]->zone_rn].vnum, diff);
				}
			} else
				sprintf(str, "%ld", c->get_gold());
		} else if (!str_cmp(field, "bank")) {
			if (*subfield) {
				const long before = c->get_bank();
				c->set_bank(MAX(0, gm_char_field(c, field, subfield, c->get_bank())));
				// клан-налог
				const long diff = c->get_bank() - before;
				split_or_clan_tax(c, diff);
				// стата для show money
				if (!c->IsNpc() && IN_ROOM(c) > 0) {
					MoneyDropStat::add(zone_table[world[IN_ROOM(c)]->zone_rn].vnum, diff);
				} 
			} else
				sprintf(str, "%ld", c->get_bank());
		} else if (!str_cmp(field, "exp") || !str_cmp(field, "questbodrich")) {
			if (!str_cmp(field, "questbodrich")) {
				if (*subfield) {
					if(IS_CHARMICE(c)) {
//						SendMsgToChar(c->get_master(), "Квест чармисом, берем мастера\r\n");
						c->get_master()->dquest(atoi(subfield));
					}
					else {
						c->dquest(atoi(subfield));
					}
				}
			} else {
				if (*subfield) {
					if (*subfield == '-') {
						EndowExpToChar(c, -MAX(1, atoi(subfield + 1)));
						sprintf(buf,
								"SCRIPT_LOG (exp) у %s уменьшен опыт на %d в триггере %d",
								GET_NAME(c),
								MAX(1, atoi(subfield + 1)),
								GET_TRIG_VNUM(trig));
						mudlog(buf, BRF, kLvlGreatGod, ERRLOG, 1);
					} else if (*subfield == '+') {
						EndowExpToChar(c, +MAX(1, atoi(subfield + 1)));
						sprintf(buf,
								"SCRIPT_LOG (exp) у %s увеличен опыт на %d в триггере %d",
								GET_NAME(c),
								MAX(1, atoi(subfield + 1)),
								GET_TRIG_VNUM(trig));
						mudlog(buf, BRF, kLvlGreatGod, ERRLOG, 1);
					} else {
						sprintf(buf,
								"SCRIPT_LOG (exp) ОШИБКА! у %s напрямую указан опыт %d в триггере %d",
								GET_NAME(c),
								atoi(subfield + 1),
								GET_TRIG_VNUM(trig));
						mudlog(buf, BRF, kLvlGreatGod, ERRLOG, 1);
					}
				} else
					sprintf(str, "%ld", GET_EXP(c));
			}
		} else if (!str_cmp(field, "MaxGainExp")) {
			sprintf(str, "%ld", (long) max_exp_gain_pc(c));
		} else if (!str_cmp(field, "TnlExp")) {
			sprintf(str, "%ld", GetExpUntilNextLvl(c, c->GetLevel() + 1) - GET_EXP(c));
		} else if (!str_cmp(field, "sex"))
			sprintf(str, "%d", (int) GET_SEX(c));
		else if (!str_cmp(field, "clan")) {
			if (CLAN(c)) {
				sprintf(str, "%s", CLAN(c)->GetAbbrev());
				for (i = 0; str[i]; i++)
					str[i] = LOWER(str[i]);
			} else
				sprintf(str, "0");
		} else if (!str_cmp(field, "ClanRank")) {
			if (CLAN(c) && CLAN_MEMBER(c))
				sprintf(str, "%d", CLAN_MEMBER(c)->rank_num);
			else
				sprintf(str, "0");
		} else if (!str_cmp(field, "ClanLevel")) {
			if (CLAN(c) && CLAN_MEMBER(c))
				sprintf(str, "%d", CLAN(c)->GetClanLevel());
			else
				sprintf(str, "0");
		} else if (!str_cmp(field, "m"))
			strcpy(str, HMHR(c));
		else if (!str_cmp(field, "s"))
			strcpy(str, HSHR(c));
		else if (!str_cmp(field, "e"))
			strcpy(str, HSSH(c));
		else if (!str_cmp(field, "g"))
			strcpy(str, GET_CH_SUF_1(c));
		else if (!str_cmp(field, "w"))
			strcpy(str, GET_CH_SUF_3(c));
		else if (!str_cmp(field, "q"))
			strcpy(str, GET_CH_SUF_4(c));
		else if (!str_cmp(field, "y"))
			strcpy(str, GET_CH_SUF_5(c));
		else if (!str_cmp(field, "a"))
			strcpy(str, GET_CH_SUF_6(c));
		else if (!str_cmp(field, "r"))
			strcpy(str, GET_CH_SUF_7(c));
		else if (!str_cmp(field, "x"))
			strcpy(str, GET_CH_SUF_8(c));
		else if (!str_cmp(field, "weight"))
			sprintf(str, "%d", GET_WEIGHT(c));
		else if (!str_cmp(field, "CarryWeight"))
			sprintf(str, "%d", c->char_specials.carry_weight);
		else if (!str_cmp(field, "cancarryweight"))
			sprintf(str, "%d", CAN_CARRY_W(c));
		else if (!str_cmp(field, "CanBeSeen")) {
			if ((type == MOB_TRIGGER) && !CAN_SEE(((CharData *) go), c)) {
				strcpy(str, "0");
			} else {
				strcpy(str, "1");
			}
		} else if (!str_cmp(field, "class")) {
			sprintf(str, "%d",  to_underlying(c->GetClass()));
		} else if (!str_cmp(field, "race")) {
			sprintf(str, "%d", (int) GET_RACE(c));
		} else if (!str_cmp(field, "fighting")) {
			if (c->GetEnemy()) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(c->GetEnemy()));
			}
		} else if (!str_cmp(field, "iskiller")) {
			if (PLR_FLAGGED(c, EPlrFlag::kKiller)) {
				strcpy(str, "1");
			} else {
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "ischarmice")) {
			if (IS_CHARMICE(c)) {
				strcpy(str, "1");
			} else {
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "isthief")) {
			if (PLR_FLAGGED(c, EPlrFlag::kBurglar)) {
				strcpy(str, "1");
			} else {
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "rentable")) {
			if (!c->IsNpc() && NORENTABLE(c)) {
				strcpy(str, "0");
			} else {
				strcpy(str, "1");
			}
		} else if (!str_cmp(field, "cangetskill")) {
			auto skill_id = FixNameAndFindSkillId(subfield);
			if (skill_id > ESkill::kUndefined) {
				if (CanGetSkill(c, skill_id)) {
					strcpy(str, "1");
				} else {
					strcpy(str, "0");
				}
			} else {
				sprintf(buf, "wrong skill name '%s'!", subfield);
				trig_log(trig, buf);
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "cangetspell")) {
			auto spell_id = FixNameAndFindSpellId(subfield);
			if (spell_id > ESpell::kUndefined) {
				if (CanGetSpell(c, spell_id)) {
					strcpy(str, "1");
				} else {
					strcpy(str, "0");
				}
			} else {
				sprintf(buf, "wrong spell name '%s'!", subfield);
				trig_log(trig, buf);
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "cangetfeat")) {
			if (auto id = FindFeatId(subfield); id != EFeat::kUndefined) {
				if (CanGetFeat(c, id))
					strcpy(str, "1");
				else
					strcpy(str, "0");
			} else {
				sprintf(buf, "wrong feature name '%s'!", subfield);
				trig_log(trig, buf);
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "agressor")) {
			if (AGRESSOR(c))
				sprintf(str, "%d", AGRESSOR(c));
			else
				strcpy(str, "0");
		} else if (!str_cmp(field, "vnum")) {
			sprintf(str, "%d", GET_MOB_VNUM(c));
		} else if (!str_cmp(field, "str")) {
			sprintf(str, "%d", c->get_str());
		} else if (!str_cmp(field, "stradd")) {
			sprintf(str, "%d", GET_STR_ADD(c));
		} else if (!str_cmp(field, "realstr")) {
			sprintf(str, "%d", GetRealStr(c));
		} else if (!str_cmp(field, "int")) {
			sprintf(str, "%d", c->get_int());
		} else if (!str_cmp(field, "intadd")) {
			sprintf(str, "%d", GET_INT_ADD(c));
		} else if (!str_cmp(field, "realint")) {
			sprintf(str, "%d", GetRealInt(c));
		} else if (!str_cmp(field, "wis")) {
			sprintf(str, "%d", c->get_wis());
		} else if (!str_cmp(field, "wisadd")) {
			sprintf(str, "%d", GET_WIS_ADD(c));
		} else if (!str_cmp(field, "realwis")) {
			sprintf(str, "%d", GetRealWis(c));
		} else if (!str_cmp(field, "dex")) {
			sprintf(str, "%d", c->get_dex());
		} else if (!str_cmp(field, "dexadd")) {
			sprintf(str, "%d", c->get_dex_add());
		} else if (!str_cmp(field, "realdex")) {
			sprintf(str, "%d", GetRealDex(c));
		} else if (!str_cmp(field, "con")) {
			sprintf(str, "%d", c->get_con());
		} else if (!str_cmp(field, "conadd")) {
			sprintf(str, "%d", GET_CON_ADD(c));
		} else if (!str_cmp(field, "realcon")) {
			sprintf(str, "%d", GetRealCon(c));
		} else if (!str_cmp(field, "cha")) {
			sprintf(str, "%d", c->get_cha());
		} else if (!str_cmp(field, "chaadd")) {
			sprintf(str, "%d", GET_CHA_ADD(c));
		} else if (!str_cmp(field, "realcha")) {
			sprintf(str, "%d", GetRealCha(c));
		} else if (!str_cmp(field, "size")) {
			sprintf(str, "%d", GET_SIZE(c));
		} else if (!str_cmp(field, "sizeadd")) {
			if (*subfield)
				GET_SIZE_ADD(c) =
					(sbyte) MAX(1, gm_char_field(c, field, subfield, (long) GET_SIZE_ADD(c)));
				else
				sprintf(str, "%d", GET_SIZE_ADD(c));
		} else if (!str_cmp(field, "realsize")) {
			sprintf(str, "%d", GET_REAL_SIZE(c));
		} else if (!str_cmp(field, "room")) {
			if (!*subfield) {
				int n = find_room_uid(world[IN_ROOM(c)]->room_vn);
				if (n >= 0)
				sprintf(str, "%c%d", UID_ROOM, n);
			}
			else {
				int p = atoi(subfield);
				if (p > 0){
					RemoveCharFromRoom(c);
					PlaceCharToRoom(c, real_room(p));
				}
			}
		} else if (!str_cmp(field, "riding")) {
			if (c->has_horse(false)) {
				sprintf(str, "%c%ld", uid_type, GET_ID(c->get_horse()));
			}
		} else if (!str_cmp(field, "riddenby")) {
			if (IS_HORSE(c) && c->get_master()->IsOnHorse() && (GET_ID(c->get_master()->get_horse()) == GET_ID(c))) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(c->get_master()));
			}
		} else if (!str_cmp(field, "realroom"))
			sprintf(str, "%d", world[IN_ROOM(c)]->room_vn);
		else if (!str_cmp(field, "loadroom")) {
			if (!c->IsNpc()) {
				if (!*subfield)
					sprintf(str, "%d", GET_LOADROOM(c));
				else {
					int pos = atoi(subfield);
					if (real_room(pos) != kNowhere) {
						GET_LOADROOM(c) = pos;
						c->save_char();
					}
				}
			}
		} else if (!str_cmp(field, "maxskill")) {
			const auto skill_id = FixNameAndFindSkillId(subfield);
			if (MUD::Skill(skill_id).IsAvailable()) {
				sprintf(str, "%d", CalcSkillHardCap(c, skill_id));
			} else {
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "maxremortskill")) {
				sprintf(str, "%d", CalcSkillRemortCap(c));
		} else if (!str_cmp(field, "skill")) {
			strcpy(str, skill_percent(trig, c, subfield));
		} else if (!str_cmp(field, "feat")) {
			if (feat_owner(trig, c, subfield)) {
				strcpy(str, "1");
			} else {
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "spellcount"))
			strcpy(str, spell_count(trig, c, subfield));
		else if (!str_cmp(field, "spelltype"))
			strcpy(str, spell_knowledge(trig, c, subfield));
		else if (!str_cmp(field, "quested")) {
			if (*subfield && (num = atoi(subfield)) > 0) {
				if (c->quested_get(num))
					strcpy(str, "1");
				else
					strcpy(str, "0");
			}
		} else if (!str_cmp(field, "getquest")) {
			if (*subfield && (num = atoi(subfield)) > 0) {
				strcpy(str, (c->quested_get_text(num)).c_str());
			}
		} else if (!str_cmp(field, "setquest")) {
			if (*subfield) {
				subfield = one_argument(subfield, buf);
				skip_spaces(&subfield);
				if ((num = atoi(buf)) > 0) {
					c->quested_add(c, num, subfield);
				}
			}
		}
			// все эти блоки надо переписать на что-нибудь другое, их слишком много
		else if (!str_cmp(field, "alliance")) {
			if (*subfield) {
				subfield = one_argument(subfield, buf);
				if (ClanSystem::is_alliance(c, buf))
					strcpy(str, "1");
				else
					strcpy(str, "0");
			}
		} else if (!str_cmp(field, "eq")) {
			int pos = -1;
			if (a_isdigit(*subfield))
				pos = atoi(subfield);
			else if (*subfield)
				pos = find_eq_pos(c, nullptr, subfield);
			if (!*subfield || pos < 0 || pos >= EEquipPos::kNumEquipPos)
				strcpy(str, "");
			else {
				if (!GET_EQ(c, pos))
					strcpy(str, "");
				else
					sprintf(str, "%c%ld", UID_OBJ, GET_EQ(c, pos)->get_id());
			}
		} else if (!str_cmp(field, "haveobj") || !str_cmp(field, "haveobjs")) {
			int pos;
			if (a_isdigit(*subfield)) {
				pos = atoi(subfield);
				for (obj = c->carrying; obj; obj = obj->get_next_content()) {
					if (GET_OBJ_VNUM(obj) == pos) {
						break;
					}
				}
			} else {
				obj = get_obj_in_list_vis(c, subfield, c->carrying);
			}

			if (obj) {
				sprintf(str, "%c%ld", UID_OBJ, obj->get_id());
			} else {
				strcpy(str, "0");
			}
		} else if (!str_cmp(field, "varexist") || !str_cmp(field, "varexists")) {
			strcpy(str, "0");
			if (find_var_cntx(&((SCRIPT(c))->global_vars), subfield, sc->context)) {
				strcpy(str, "1");
			}
		} else if (!str_cmp(field, "nextinroom")) {
			CharData *next = nullptr;
			const auto room = world[c->in_room];

			auto people_i = std::find(room->people.begin(), room->people.end(), c);

			if (people_i != room->people.end()) {
				++people_i;
				people_i = std::find_if(people_i, room->people.end(),
										[](const CharData *ch) {
											return !GET_INVIS_LEV(ch);
										});

				if (people_i != room->people.end()) {
					next = *people_i;
				}
			}

			if (next) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(next));
			} else {
				strcpy(str, "");
			}
		} else if (!str_cmp(field, "position")) {
			if (!*subfield) {
				sprintf(str, "%d", static_cast<int>(GET_POS(c)));
			} else {
				auto pos = std::clamp(static_cast<EPosition>(atoi(subfield)), EPosition::kPerish, --EPosition::kLast);
				if (!IS_IMMORTAL(c)) {
					if (c->IsOnHorse()) {
						c->dismount();
					}
					GET_POS(c) = pos;
				}
			}
		} else if (!str_cmp(field, "wait") || !str_cmp(field, "lag")) {
			int pos;

			if (!*subfield || (pos = atoi(subfield)) <= 0) {
				sprintf(str, "%d", c->get_wait());
			} else if (!IS_IMMORTAL(c)) {
				char tmp;
				if (sscanf(subfield, "%d %c", &pos, &tmp) == 2) {
					if (tmp == 'p') {
						SetWaitState(c, pos);
					}
				}
				else {
					SetWaitState(c, pos * kBattleRound);
				}
			}
		} else if (!str_cmp(field, "applyvalue")) {
			int num;
			int sum  = 0;
			for (num = 0; num < EApply::kNumberApplies; num++) {
				if (!str_cmp(subfield, apply_types[num]))
				break;
			}
			if (num == EApply::kNumberApplies) {
				sprintf(buf, "Не найден апплай '%s' в списке ApplyTypes", subfield);
				trig_log(trig, buf);
				return;
			}
			if (!c->affected.empty()) {
				for (const auto &aff : c->affected) {
					if (aff->location == num){
						sum += aff->modifier;
					}
				}
			}
			sprintf(str, "%d", sum);
		} else if (!str_cmp(field, "affect")) {
			c->char_specials.saved.affected_by.gm_flag(subfield, affected_bits, str);
			//подозреваю что никто из билдеров даже не вкурсе насчет всего функционала этого affect
			//к тому же аффекты в том списке не все кличи например никак там не отображаются
		} else if (!str_cmp(field, "affectedby")) {
			char *p = strchr(subfield, ',');
			if (!p) {
				auto spell_id = FixNameAndFindSpellId(subfield);
				if (spell_id == ESpell::kUndefined) {
					sprintf(buf, "Не найден спелл %s в списке AffectedBy", subfield);
					trig_log(trig, buf);
					return;
				}
				if (spell_id >= ESpell::kFirst && spell_id < ESpell::kLast) {
					for (const auto &affect : c->affected) {
						if (affect->type == spell_id) {
							sprintf(str, "%s", "1");
							return;
						}
					}
					sprintf(str, "%s", "0");
				}
			} else {
				int num;
				*(p++) = '\0';
				auto spell_id = FixNameAndFindSpellId(subfield);
				if (spell_id == ESpell::kUndefined) {
					sprintf(buf, "Не найден спелл %s в списке AffecteBby", p);
					trig_log(trig, buf);
					return;
				}
				for (num = 0; num < EApply::kNumberApplies; num++) {
					if (!str_cmp(p, apply_types[num]))
					break;
				}
				if (num == EApply::kNumberApplies) {
					sprintf(buf, "Не найден апплай '%s' в списке AffectedBy", p);
					trig_log(trig, buf);
					return;
				}
				for (const auto &affect : c->affected) {
					if (affect->type == spell_id) {
						if (affect->location == num) {
							sprintf(str, "%d", affect->modifier);
							return;
						}
					}
				}
				sprintf(str, "%s", "0");
			}
		} else if (!str_cmp(field, "action")) {
			if (c->IsNpc()) {
				c->char_specials.saved.act.gm_flag(subfield, action_bits, str);
			}
		} else if (!str_cmp(field, "leader")) {
			if (c->has_master()) {
				sprintf(str, "%c%ld", uid_type, GET_ID(c->get_master()));
			}
		} else if (!str_cmp(field, "group")) {
			CharData *l;
			struct FollowerType *f;
			if (!AFF_FLAGGED(c, EAffect::kGroup)) {
				return;
			}
			l = c->get_master();
			if (!l) {
				l = c;
			}
			// l - лидер группы
			sprintf(str + strlen(str), "%c%ld ", uid_type, GET_ID(l));
			for (f = l->followers; f; f = f->next) {
				if (!AFF_FLAGGED(f->follower, EAffect::kGroup)) {
					continue;
				}
				sprintf(str + strlen(str), "%c%ld ", uid_type, GET_ID(f->follower));
			}
		} else if (!str_cmp(field, "attackers")) {
			CharData *t;
			size_t str_length = strlen(str);
			for (t = combat_list; t; t = t->next_fighting) {
				if (t->GetEnemy() != c) {
					continue;
				}
				int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(t));
				if (str_length + n < kMaxTrglineLength) // not counting the terminating null character
				{
					strcpy(str + str_length, tmp);
					str_length += n;
				} else {
					break; // too many attackers
				}
			}
		} else if (!str_cmp(field, "people")) {
			//const auto first_char = world[IN_ROOM(c)]->first_character();
			const auto room = world[IN_ROOM(c)]->people;
			const auto first_char = std::find_if(room.begin(), room.end(), [](CharData *ch) {
				return !GET_INVIS_LEV(ch);
			});

			if (first_char != room.end()) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(*first_char));
			} else {
				strcpy(str, "");
			}
		}
			//Polud обработка поля objs у чара, возвращает строку со списком UID предметов в инвентаре
		else if (!str_cmp(field, "objs")) {
			size_t str_length = strlen(str);
			for (obj = c->carrying; obj; obj = obj->get_next_content()) {
				int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_OBJ, obj->get_id());
				if (str_length + n < kMaxTrglineLength) // not counting the terminating null character
				{
					strcpy(str + str_length, tmp);
					str_length += n;
				} else {
					break; // too many carying objects
				}
			}
		}
			//-Polud
		else if (!str_cmp(field, "char")
			|| !str_cmp(field, "pc")
			|| !str_cmp(field, "npc")
			|| !str_cmp(field, "all")) {
			int inroom;

			// Составление списка (для mob)
			inroom = IN_ROOM(c);
			if (inroom == kNowhere) {
				trig_log(trig, "mob-построитель списка в kNowhere");
				return;
			}

			size_t str_length = strlen(str);
			for (const auto rndm : world[inroom]->people) {
				if ((c == rndm)
					|| GET_INVIS_LEV(rndm)) {
					continue;
				}

				if ((*field == 'a')
					|| (!rndm->IsNpc()
						&& *field != 'n'
						&& rndm->desc)
					|| (rndm->IsNpc()
						&& IS_CHARMED(rndm)
						&& *field == 'c')
					|| (rndm->IsNpc()
						&& !IS_CHARMED(rndm)
						&& *field == 'n')) {
					int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(rndm));
					if (str_length + n < kMaxTrglineLength) // not counting the terminating null character
					{
						strcpy(str + str_length, tmp);
						str_length += n;
					} else {
						break; // too many characters
					}
				}
			}

			return;
		} else if (!str_cmp(field, "isnoob")) {
			strcpy(str, Noob::is_noob(c) ? "1" : "0");
		} else if (!str_cmp(field, "nooboutfit")) {
			std::string vnum_str = Noob::print_start_outfit(c);
			snprintf(str, kMaxTrglineLength, "%s", vnum_str.c_str());
		} else {
			vd = find_var_cntx(&((SCRIPT(c))->global_vars), field,
							   sc->context);
			if (vd)
				sprintf(str, "%s", vd->value);
			else {
				sprintf(buf2, "unknown char field: '%s'", field);
				trig_log(trig, buf2);
			}
		}
	} else if (o) {
		if (text_processed(field, subfield, vd, str)) {
			return;
		} else if (!str_cmp(field, "iname")) {
			if (!o->get_PName(0).empty()) {
				strcpy(str, o->get_PName(0).c_str());
			} else {
				strcpy(str, o->get_aliases().c_str());
			}
		} else if (!str_cmp(field, "rname")) {
			if (!o->get_PName(1).empty()) {
				strcpy(str, o->get_PName(1).c_str());
			} else {
				strcpy(str, o->get_aliases().c_str());
			}
		} else if (!str_cmp(field, "dname")) {
			if (!o->get_PName(2).empty()) {
				strcpy(str, o->get_PName(2).c_str());
			} else {
				strcpy(str, o->get_aliases().c_str());
			}
		} else if (!str_cmp(field, "vname")) {
			if (!o->get_PName(3).empty()) {
				strcpy(str, o->get_PName(3).c_str());
			} else {
				strcpy(str, o->get_aliases().c_str());
			}
		} else if (!str_cmp(field, "tname")) {
			if (!o->get_PName(4).empty()) {
				strcpy(str, o->get_PName(4).c_str());
			} else {
				strcpy(str, o->get_aliases().c_str());
			}
		} else if (!str_cmp(field, "pname")) {
			if (!o->get_PName(5).empty()) {
				strcpy(str, o->get_PName(5).c_str());
			} else {
				strcpy(str, o->get_aliases().c_str());
			}
		} else if (!str_cmp(field, "name")) {
			strcpy(str, o->get_aliases().c_str());
		} else if (!str_cmp(field, "id")) {
			sprintf(str, "%c%ld", UID_OBJ, o->get_id());
		} else if (!str_cmp(field, "uid")) {
			if (!GET_OBJ_UID(o)) {
				InitUid(o);
			}
			sprintf(str, "%u", GET_OBJ_UID(o));
		} else if (!str_cmp(field, "skill")) {
			sprintf(str, "%d", GET_OBJ_SKILL(o));
		} else if (!str_cmp(field, "shortdesc")) {
			strcpy(str, o->get_short_description().c_str());
		} else if (!str_cmp(field, "vnum")) {
			sprintf(str, "%d", GET_OBJ_VNUM(o));
		} else if (!str_cmp(field, "type")) {
			sprintf(str, "%d", (int) GET_OBJ_TYPE(o));
		} else if (!str_cmp(field, "timer")) {
			sprintf(str, "%d", o->get_timer());
		} else if (!str_cmp(field, "objmax")) {
			sprintf(str, "%d", o->get_maximum_durability());
		} else if (!str_cmp(field, "objcur")) {
			if (*subfield) {
				skip_spaces(&subfield);
				o->set_current_durability(atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_CUR(o));
			}
		} else if (!str_cmp(field, "cost")) {
			if (*subfield) {
				skip_spaces(&subfield);
				o->set_cost(atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_COST(o));
			}
		} else if (!str_cmp(field, "val0")) {
			if (*subfield) {
				skip_spaces(&subfield);
				o->set_val(0, atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_VAL(o, 0));
			}
		} else if (!str_cmp(field, "val1")) {
			if (*subfield) {
				skip_spaces(&subfield);
				o->set_val(1, atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_VAL(o, 1));
			}
		} else if (!str_cmp(field, "val2")) {
			if (*subfield) {
				skip_spaces(&subfield);
				o->set_val(2, atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_VAL(o, 2));
			}
		} else if (!str_cmp(field, "val3")) {
			if (*subfield) {
				skip_spaces(&subfield);
				o->set_val(3, atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_VAL(o, 3));
			}
		} else if (!str_cmp(field, "maker")) {
			sprintf(str, "%d", GET_OBJ_MAKER(o));
		} else if (!str_cmp(field, "effect")) {
			o->gm_extra_flag(subfield, extra_bits, str);
		} else if (!str_cmp(field, "affect")) {
			o->gm_affect_flag(subfield, weapon_affects, str);
		} else if (!str_cmp(field, "carriedby")) {
			if (o->get_carried_by()) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->get_carried_by()));
			} else {
				strcpy(str, "");
			}
		} else if (!str_cmp(field, "wornby")) {
			if (o->get_worn_by()) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->get_worn_by()));
			} else {
				strcpy(str, "");
			}
		} else if (!str_cmp(field, "g"))
			strcpy(str, GET_OBJ_SUF_1(o));
		else if (!str_cmp(field, "q"))
			strcpy(str, GET_OBJ_SUF_4(o));
		else if (!str_cmp(field, "u"))
			strcpy(str, GET_OBJ_SUF_2(o));
		else if (!str_cmp(field, "w"))
			strcpy(str, GET_OBJ_SUF_3(o));
		else if (!str_cmp(field, "y"))
			strcpy(str, GET_OBJ_SUF_5(o));
		else if (!str_cmp(field, "a"))
			strcpy(str, GET_OBJ_SUF_6(o));
		else if (!str_cmp(field, "count"))
			strcpy(str, get_objs_in_world(o));
		else if (!str_cmp(field, "sex"))
			sprintf(str, "%d", (int) GET_OBJ_SEX(o));
		else if (!str_cmp(field, "room")) {
			if (o->get_carried_by()) {
				sprintf(str, "%d", world[IN_ROOM(o->get_carried_by())]->room_vn);
			} else if (o->get_worn_by()) {
				sprintf(str, "%d", world[IN_ROOM(o->get_worn_by())]->room_vn);
			} else if (o->get_in_room() != kNowhere) {
				sprintf(str, "%d", world[o->get_in_room()]->room_vn);
			} else {
				strcpy(str, "");
			}
		}
			//Polud обработка %obj.put(UID)% - пытается поместить объект в контейнер, комнату или инвентарь чара, в зависимости от UIDа
		else if (!str_cmp(field, "put")) {
			ObjData *obj_to = nullptr;
			CharData *char_to = nullptr;
			RoomData *room_to = nullptr;
			if (!((*subfield == UID_CHAR) || (*subfield == UID_CHAR_ALL) || (*subfield == UID_OBJ) || (*subfield == UID_ROOM))) {
				trig_log(trig, "object.put: недопустимый аргумент, необходимо указать UID");
				return;
			}
			if (*subfield == UID_OBJ) {
				obj_to = find_obj_by_id(atoi(subfield + 1));
				if (!(obj_to
					&& GET_OBJ_TYPE(obj_to) == EObjType::kContainer)) {
					trig_log(trig, "object.put: объект-приемник не найден или не является контейнером");
					return;
				}
			}
			if ((*subfield == UID_CHAR) || (*subfield == UID_CHAR_ALL)) {
				char_to = find_char(atoi(subfield + 1));
				if (!(char_to && CanTakeObj(char_to, o))) {
					trig_log(trig, "object.put: субъект-приемник не найден или не может нести этот объект");
					return;
				}
			}
			if (*subfield == UID_ROOM) {
				room_to = find_room(atoi(subfield + 1));
				if (!(room_to && (room_to->room_vn != kNowhere))) {
					trig_log(trig, "object.put: недопустимая комната для размещения объекта");
					return;
				}
			}
			//found something to put our object
			//let's make it nobody's!
			if (o->get_worn_by()) {
				UnequipChar(o->get_worn_by(), o->get_worn_on(), CharEquipFlags());
			} else if (o->get_carried_by()) {
				RemoveObjFromChar(o);
			} else if (o->get_in_obj()) {
				RemoveObjFromObj(o);
			} else if (o->get_in_room() > kNowhere) {
				RemoveObjFromRoom(o);
			} else {
				trig_log(trig, "object.put: не удалось извлечь объект");
				return;
			}
			//finally, put it to destination
			if (char_to)
				PlaceObjToInventory(o, char_to);
			else if (obj_to)
				PlaceObjIntoObj(o, obj_to);
			else if (room_to)
				PlaceObjToRoom(o, real_room(room_to->room_vn));
			else {
				sprintf(buf2,
						"object.put: ATTENTION! за время подготовки объекта >%s< к передаче перестал существовать адресат. Объект сейчас в kNowhere",
						o->get_short_description().c_str());
				trig_log(trig, buf2);
				return;
			}
		}
			//-Polud
		else if (!str_cmp(field, "char") ||
			!str_cmp(field, "pc") || !str_cmp(field, "npc") || !str_cmp(field, "all")) {
			int inroom;

			// Составление списка (для obj)
			inroom = obj_room(o);
			if (inroom == kNowhere) {
				trig_log(trig, "obj-построитель списка в kNowhere");
				return;
			}

			size_t str_length = strlen(str);
			for (const auto rndm : world[inroom]->people) {
				if (GET_INVIS_LEV(rndm))
					continue;

				if ((*field == 'a')
					|| (!rndm->IsNpc()
						&& *field != 'n')
					|| (rndm->IsNpc()
						&& IS_CHARMED(rndm)
						&& *field == 'c')
					|| (rndm->IsNpc()
						&& !IS_CHARMED(rndm)
						&& *field == 'n')) {
					int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(rndm));
					if (str_length + n < kMaxTrglineLength) // not counting the terminating null character
					{
						strcpy(str + str_length, tmp);
						str_length += n;
					} else {
						break; // too many characters
					}
				}
			}

			return;
		} else if (!str_cmp(field, "owner")) {
			if (*subfield) {
				skip_spaces(&subfield);
				int num = atoi(subfield);
				// Убрал пока проверку. По идее 0 -- отсутствие владельца.
				// Понадобилась возможность обнулить владельца из трига.
				o->set_owner(num);
			} else {
				sprintf(str, "%d", GET_OBJ_OWNER(o));
			}
		} else if (!str_cmp(field, "varexists")) {
			strcpy(str, "0");
			if (find_var_cntx(&o->get_script()->global_vars, subfield, sc->context)) {
				strcpy(str, "1");
			}
		} else if (!str_cmp(field, "cost")) {
			if (*subfield && a_isdigit(*subfield)) {
				skip_spaces(&subfield);
				o->set_cost(atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_COST(o));
			}
		} else if (!str_cmp(field, "rent")) {
			if (*subfield && a_isdigit(*subfield)) {
				skip_spaces(&subfield);
				o->set_rent_off(atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_RENT(o));
			}
		} else if (!str_cmp(field, "renteq")) {
			if (*subfield && a_isdigit(*subfield)) {
				skip_spaces(&subfield);
				o->set_rent_on(atoi(subfield));
			} else {
				sprintf(str, "%d", GET_OBJ_RENTEQ(o));
			}
		} else if (!str_cmp(field, "objs")) {
			if (o->get_type() == EObjType::kContainer) {
				size_t str_length = strlen(str);
				for (auto temp = o->get_contains(); temp; temp = temp->get_next_content()) {
					int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_OBJ, temp->get_id());
					if (str_length + n < kMaxTrglineLength) { // not counting the terminating null character
					strcpy(str + str_length, tmp);
					str_length += n;
					} else {
						sprintf(buf2, "Предмет VNUM %d данные переполнены, далее содержимое не учитывается", GET_OBJ_VNUM(o));
						trig_log(trig, buf2);
						break; // too many carying objects
					}
				}
			} else {
				sprintf(buf2, "Предмет VNUM %d не контейнер, поля 'objs' нет.", GET_OBJ_VNUM(o));
				trig_log(trig, buf2);
			}
		} else //get global var. obj.varname
		{
			vd = find_var_cntx(&o->get_script()->global_vars, field, sc->context);
			if (vd) {
				sprintf(str, "%s", vd->value);
			} else {
				sprintf(buf2, "Type: %d. unknown object field: '%s'", type, field);
				trig_log(trig, buf2);
			}
		}
	} else if (r) {
		if (text_processed(field, subfield, vd, str)) {
			return;
		} else if (!str_cmp(field, "name")) {
			if (*subfield) {
				if (r->name)
					free(r->name);
				if (strlen(subfield) > MAX_ROOM_NAME)
					subfield[MAX_ROOM_NAME - 1] = '\0';
				r->name = str_dup(subfield);
			} else
				strcpy(str, r->name);
		} else if (!str_cmp(field, "direction")) {
			if (*subfield) {
				for (int i = 0; i < EDirection::kMaxDirNum; i++) {
					if (!str_cmp(subfield, dirs[i])) {
						if (r->dir_option[i]) {
							sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[i]->to_room())));
							break;
						}
					}
				}
			}
		} else if (!str_cmp(field, "north")) {
			if (r->dir_option[EDirection::kNorth]) {
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EDirection::kNorth]->to_room())));
			}
		} else if (!str_cmp(field, "east")) {
			if (r->dir_option[EDirection::kEast]) {
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EDirection::kEast]->to_room())));
			}
		} else if (!str_cmp(field, "south")) {
			if (r->dir_option[EDirection::kSouth]) {
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EDirection::kSouth]->to_room())));
			}
		} else if (!str_cmp(field, "west")) {
			if (r->dir_option[EDirection::kWest]) {
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EDirection::kWest]->to_room())));
			}
		} else if (!str_cmp(field, "up")) {
			if (r->dir_option[EDirection::kUp]) {
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EDirection::kUp]->to_room())));
			}
		} else if (!str_cmp(field, "down")) {
			if (r->dir_option[EDirection::kDown]) {
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EDirection::kDown]->to_room())));
			}
		} else if (!str_cmp(field, "vnum")) {
			sprintf(str, "%d", r->room_vn);
		} else if (!str_cmp(field, "sectortype"))//Polud возвращает строку - тип комнаты
		{
			sprinttype(r->sector_type, sector_types, str);
		} else if (!str_cmp(field, "id")) {
			sprintf(str, "%c%d", UID_ROOM, find_room_uid(r->room_vn));
		} else if (!str_cmp(field, "flag")) {
			r->gm_flag(subfield, room_bits, str);
		} else if (!str_cmp(field, "people")) {
			const auto first_char = r->first_character();
			if (first_char) {
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(first_char));
			}
		} else if (!str_cmp(field, "firstvnum")) {
			int x,y;
			GetZoneRooms(r->zone_rn, &x , &y);
			sprintf(str, "%d", world[x]->room_vn);
		} else if (!str_cmp(field, "lastvnum")) {
			int x,y;
			GetZoneRooms(r->zone_rn, &x , &y);
			sprintf(str, "%d", world[y]->room_vn);
		}
		else if (!str_cmp(field, "char")
			|| !str_cmp(field, "pc")
			|| !str_cmp(field, "npc")
			|| !str_cmp(field, "all")) {
			int inroom;

			// Составление списка (для room)
			inroom = real_room(r->room_vn);
			if (inroom == kNowhere) {
				trig_log(trig, "room-построитель списка в kNowhere");
				return;
			}

			size_t str_length = strlen(str);
			for (const auto rndm : world[inroom]->people) {
				if (GET_INVIS_LEV(rndm))
					continue;

				if ((*field == 'a')
					|| (!rndm->IsNpc()
						&& *field != 'n')
					|| (rndm->IsNpc()
						&& IS_CHARMED(rndm)
						&& *field == 'c')
					|| (rndm->IsNpc()
						&& !IS_CHARMED(rndm)
						&& *field == 'n')) {
					int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(rndm));
					if (str_length + n < kMaxTrglineLength) // not counting the terminating null character
					{
						strcpy(str + str_length, tmp);
						str_length += n;
					} else {
						break; // too many characters
					}
				}
			}

			return;
		} else if (!str_cmp(field, "objects")) {
			//mixaz  Выдаем список объектов в комнате
			int inroom;
			// Составление списка (для room)
			inroom = real_room(r->room_vn);
			if (inroom == kNowhere) {
				trig_log(trig, "room-построитель списка в kNowhere");
				return;
			}

			size_t str_length = strlen(str);
			for (obj = world[inroom]->contents; obj; obj = obj->get_next_content()) {
				int n = snprintf(tmp, kMaxTrglineLength, "%c%ld ", UID_OBJ, obj->get_id());
				if (str_length + n < kMaxTrglineLength) // not counting the terminating null character
				{
					strcpy(str + str_length, tmp);
					str_length += n;
				} else {
					break; // too many objects
				}
			}
			return;
			//mixaz - end
		} else if (!str_cmp(field, "varexists")) {
			//room.varexists<0;1>
			strcpy(str, "0");
			if (find_var_cntx(&((SCRIPT(r))->global_vars), subfield, sc->context)) {
				strcpy(str, "1");
			}
		} else //get global var. room.varname
		{
			vd = find_var_cntx(&((SCRIPT(r))->global_vars), field, sc->context);
			if (vd) {
				sprintf(str, "%s", vd->value);
			} else {
				sprintf(buf2, "Type: %d. unknown room field: '%s'", type, field);
				trig_log(trig, buf2);
			}
		}
	} else if (text_processed(field, subfield, vd, str)) {
		return;
	}
}

// substitutes any variables into line and returns it as buf
void var_subst(void *go, Script *sc, Trigger *trig, int type, const char *line, char *buf) {
	char tmp[kMaxTrglineLength], repl_str[kMaxTrglineLength], *var, *field, *p;
	char *subfield_p, subfield[kMaxTrglineLength];
	char *local_p, local[kMaxTrglineLength];
	int paren_count = 0;

	if (!strchr(line, '%')) {
		strcpy(buf, line);
		return;
	}

	p = strcpy(tmp, line);
	subfield_p = subfield;

	size_t left = kMaxTrglineLength - 1;
	while (*p && (left > 0)) {
		while (*p && (*p != '%') && (left > 0)) {
			*(buf++) = *(p++);
			left--;
		}

		*buf = '\0';

		if (*p && (*(++p) == '%') && (left > 0)) {
			*(buf++) = *(p++);
			*buf = '\0';
			left--;
			continue;
		} else if (*p && (left > 0)) {
			for (var = p; *p && (*p != '%') && (*p != '.'); p++);

			field = p;
			subfield_p = subfield;    //new
			if (*p == '.') {
				*(p++) = '\0';
				local_p = local;
				for (field = p; *p && local_p && ((*p != '%') || (paren_count)); p++) {
					if (*p == '(') {
						if (!paren_count)
							*p = '\0';
						else
							*local_p++ = *p;
						paren_count++;
					} else if (*p == ')') {
						if (paren_count == 1)
							*p = '\0';
						else
							*local_p++ = *p;
						paren_count--;
						if (!paren_count) {
							*local_p = '\0';
							var_subst(go, sc, trig, type, local, subfield_p);
							local_p = nullptr;
							subfield_p = subfield + strlen(subfield);
						}
					} else if (paren_count) {
						*local_p++ = *p;
					}
				}
			}
			*(p++) = '\0';
			*subfield_p = '\0';
			*repl_str = '\0';
			find_replacement(go, sc, trig, type, var, field, subfield, repl_str);

			strncat(buf, repl_str, left);
			size_t len = std::min(strlen(repl_str), left);
			buf += len;
			left -= len;
		}
	}
}

// returns 1 if string is all digits, else 0
int is_num(const char *num) {
	while (*num
		&& (a_isdigit(*num)
			|| *num == '-')) {
		num++;
	}

	return (!*num || isspace(*num)) ? 1 : 0;
}

// evaluates 'lhs op rhs', and copies to result
void eval_op(const char *op,
			 char *lhs,
			 const char *const_rhs,
			 char *result,
			 void * /*go*/,
			 Script * /*sc*/,
			 Trigger * /*trig*/) {
	char *p = nullptr;
	int n;

	// strip off extra spaces at begin and end
	while (*lhs && isspace(*lhs)) {
		lhs++;
	}

	char *rhs = nullptr;
	std::shared_ptr<char> rhs_guard(rhs = str_dup(const_rhs), free);

	while (*rhs && isspace(*rhs)) {
		rhs++;
	}

	for (p = lhs + strlen(lhs) - 1; (p >= lhs) && isspace(*p); *p-- = '\0');
	for (p = rhs + strlen(rhs) - 1; (p >= rhs) && isspace(*p); *p-- = '\0');

	// find the op, and figure out the value
	if (!strcmp("||", op)) {
		if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
			strcpy(result, "0");
		else
			strcpy(result, "1");
	} else if (!strcmp("&&", op)) {
		if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
			strcpy(result, "0");
		else
			strcpy(result, "1");
	} else if (!strcmp("==", op)) {
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) == atoi(rhs));
		else
			sprintf(result, "%d", !str_cmp(lhs, rhs));
	} else if (!strcmp("!=", op)) {
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) != atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs));
	} else if (!strcmp("<=", op)) {
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
	} else if (!strcmp(">=", op)) {
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
	} else if (!strcmp("<", op)) {
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) < atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
	} else if (!strcmp(">", op)) {
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) > atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
	} else if (!strcmp("/=", op))
		sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');
	else if (!strcmp("*", op))
		sprintf(result, "%d", atoi(lhs) * atoi(rhs));
	else if (!strcmp("/", op))
		sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);
	else if (!strcmp("+", op))
		sprintf(result, "%d", atoi(lhs) + atoi(rhs));
	else if (!strcmp("-", op))
		sprintf(result, "%d", atoi(lhs) - atoi(rhs));
	else if (!strcmp("!", op)) {
		if (is_num(rhs))
			sprintf(result, "%d", !atoi(rhs));
		else
			sprintf(result, "%d", !*rhs);
	}
}

/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
char *matching_quote(char *p) {
	for (p++; *p && (*p != '"'); p++) {
		if (*p == '\\')
			p++;
	}

	if (!*p)
		p--;

	return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *matching_paren(char *p) {
	int i;

	for (p++, i = 1; *p && i; p++) {
		if (*p == '(')
			i++;
		else if (*p == ')')
			i--;
		else if (*p == '"')
			p = matching_quote(p);
	}

	return --p;
}

// evaluates line, and returns answer in result
void eval_expr(const char *line, char *result, void *go, Script *sc, Trigger *trig, int type) {
	char expr[kMaxInputLength], *p;

	while (*line && isspace(*line)) {
		line++;
	}

	if (eval_lhs_op_rhs(line, result, go, sc, trig, type)) { ;
	} else if (*line == '(') {
		strcpy(expr, line);
		p = matching_paren(expr);
		*p = '\0';
		eval_expr(expr + 1, result, go, sc, trig, type);
	} else {
		var_subst(go, sc, trig, type, line, result);
	}
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(const char *expr, char *result, void *go, Script *sc, Trigger *trig, int type) {
	char *p, *tokens[kMaxTrglineLength];
	char line[kMaxTrglineLength], lhr[kMaxTrglineLength], rhr[kMaxTrglineLength];
	int i, j;

	/*
	 * valid operands, in order of priority
	 * each must also be defined in eval_op()
	 */
	static const char *ops[] =
		{
			"||",
			"&&",
			"==",
			"!=",
			"<=",
			">=",
			"<",
			">",
			"/=",
			"-",
			"+",
			"/",
			"*",
			"!",
			"\n"
		};

	p = strcpy(line, expr);

	/*
	 * initialize tokens, an array of pointers to locations
	 * in line where the ops could possibly occur.
	 */
	for (j = 0; *p; j++) {
		tokens[j] = p;
		if (*p == '(')
			p = matching_paren(p) + 1;
		else if (*p == '"')
			p = matching_quote(p) + 1;
		else if (a_isalnum(*p))
			for (p++; *p && (a_isalnum(*p) || isspace(*p)); p++);
		else
			p++;
	}
	tokens[j] = nullptr;

	for (i = 0; *ops[i] != '\n'; i++)
		for (j = 0; tokens[j]; j++)
			if (!strn_cmp(ops[i], tokens[j], strlen(ops[i]))) {
				*tokens[j] = '\0';
				p = tokens[j] + strlen(ops[i]);

				eval_expr(line, lhr, go, sc, trig, type);
				eval_expr(p, rhr, go, sc, trig, type);
				eval_op(ops[i], lhr, rhr, result, go, sc, trig);

				return 1;
			}

	return 0;
}

/*++
cond - строка параметров цикла. Для комнады "foreach i .." cond = "i .."
go - уразатель на MOB/OBJ/ROOM (см. type)
sc - SCRIPT(go)
trig - исполняемый триггер
type - тип (MOB_TRIGGER,OBJ_TRIGGER,WLD_TRIGGER)

Запись
foreach i <список>
работает так:

1. Если список пустой - выйти
2. Переменная i равна к-ому элементу списка. Если это последний элемент - выйти
Иначе i = след. элемент и выполнить тело
--*/
// returns 1 if next iteration, else 0
int process_foreach_begin(const char *cond, void *go, Script *sc, Trigger *trig, int type) {
	char name[kMaxTrglineLength];
	char list_str[kMaxTrglineLength];
	char value[kMaxTrglineLength];

	skip_spaces(&cond);
	auto p = one_argument(cond, name);
	skip_spaces(&p);

	if (!*name) {
		trig_log(trig, "foreach begin w/o an variable");
		return 0;
	}

	eval_expr(p, list_str, go, sc, trig, type);
	p = list_str;
	skip_spaces(&p);
	if (!p || !*p) {
		return 0;
	}

	int v_strpos;
	auto pos = strchr(p, ' ');
	if (!pos) {
		strcpy(value, p);
		v_strpos = static_cast<int>(strlen(p));
	} else {
		strncpy(value, p, pos - p);
		value[pos - p] = '\0';
		skip_spaces(&pos);
		v_strpos = pos - p;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);
	snprintf(value, kMaxTrglineLength, "%s%s", name, FOREACH_LIST_GUID);
	add_var_cntx(&GET_TRIG_VARS(trig), value, list_str, 0);

	strcat(name, FOREACH_LIST_POS_GUID);
	sprintf(value, "%d", v_strpos);
	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);

	return 1;
}

int process_foreach_done(const char *cond, void *, Script *, Trigger *trig, int) {
	char name[kMaxTrglineLength];
	char value[kMaxTrglineLength];

	skip_spaces(&cond);
	one_argument(cond, name);

	if (!*name) {
		trig_log(trig, "foreach done w/o an variable");
		return 0;
	}

	snprintf(value, kMaxTrglineLength, "%s%s", name, FOREACH_LIST_GUID);
	const auto var_list = find_var_cntx(&GET_TRIG_VARS(trig), value, 0);

	snprintf(value, kMaxTrglineLength, "%s%s", name, FOREACH_LIST_POS_GUID);
	const auto var_list_pos = find_var_cntx(&GET_TRIG_VARS(trig), value, 0);

	if (!var_list || !var_list_pos) {
		trig_log(trig, "foreach utility vars not found");
		return 0;
	}

	const auto p = var_list->value + static_cast<unsigned int>(atoi(var_list_pos->value));
	if (!p || !*p) {
		remove_var_cntx(&GET_TRIG_VARS(trig), name, 0);

		snprintf(value, kMaxTrglineLength, "%s%s", name, FOREACH_LIST_GUID);
		remove_var_cntx(&GET_TRIG_VARS(trig), value, 0);

		snprintf(value, kMaxTrglineLength, "%s%s", name, FOREACH_LIST_POS_GUID);
		remove_var_cntx(&GET_TRIG_VARS(trig), value, 0);

		return 0;
	}

	int v_strpos;
	auto pos = strchr(p, ' ');
	if (!pos) {
		strcpy(value, p);
		v_strpos = static_cast<int>(strlen(var_list->value));
	} else {
		strncpy(value, p, pos - p);
		value[pos - p] = '\0';
		skip_spaces(&pos);
		v_strpos = pos - var_list->value;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);

	strcat(name, FOREACH_LIST_POS_GUID);
	sprintf(value, "%d", v_strpos);
	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);

	return 1;
}

// returns 1 if cond is true, else 0
int process_if(const char *cond, void *go, Script *sc, Trigger *trig, int type) {
	char result[kMaxInputLength], *p;

	eval_expr(cond, result, go, sc, trig, type);

	p = result;
	skip_spaces(&p);

	if (!*p || *p == '0') {
		return 0;
	} else {
		return 1;
	}
}

/*
 * scans for end of if-block.
 * returns the line containg 'end', or NULL
 */
cmdlist_element::shared_ptr find_end(Trigger *trig, cmdlist_element::shared_ptr cl) {
	char tmpbuf[kMaxInputLength];
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	while ((cl = cl ? cl->next : cl) != nullptr) {
		for (p = cl->cmd.c_str(); *p && isspace(*p); p++);

		if (!strn_cmp("if ", p, 3)) {
			cl = find_end(trig, cl);
		} else if (!strn_cmp("end", p, 3)) {
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl) {
		sprintf(tmpbuf, "end not found for '%s'.", cmd);
		trig_log(trig, tmpbuf);
	}
#endif

	return cl;
}

/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or NULL.
 */
cmdlist_element::shared_ptr find_else_end(Trigger *trig,
										  cmdlist_element::shared_ptr cl,
										  void *go,
										  Script *sc,
										  int type) {
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	while ((cl = cl ? cl->next : cl) != nullptr) {
		for (p = cl->cmd.c_str(); *p && isspace(*p); p++);

		if (!strn_cmp("if ", p, 3)) {
			cl = find_end(trig, cl);
		} else if (!strn_cmp("elseif ", p, 7)) {
			if (process_if(p + 7, go, sc, trig, type)) {
				GET_TRIG_DEPTH(trig)++;
			} else {
				cl = find_else_end(trig, cl, go, sc, type);
			}

			break;
		} else if (!strn_cmp("else", p, 4)) {
			GET_TRIG_DEPTH(trig)++;
			break;
		} else if (!strn_cmp("end", p, 3)) {
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl) {
		sprintf(buf, "closing 'else/end' is not found for '%s'", cmd);
		trig_log(trig, buf);
	}
#endif

	return cl;
}

/*
* scans for end of while/foreach/switch-blocks.
* returns the line containg 'end', or NULL
*/
cmdlist_element::shared_ptr find_done(Trigger *trig, cmdlist_element::shared_ptr cl) {
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	while ((cl = cl ? cl->next : cl) != nullptr) {
		for (p = cl->cmd.c_str(); *p && isspace(*reinterpret_cast<const unsigned char *>(p)); p++);

		if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7) || !strn_cmp("foreach ", p, 8)) {
			cl = find_done(trig, cl);
		} else if (!strn_cmp("done", p, 4)) {
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl) {
		sprintf(buf, "closing 'done' is not found for '%s'", cmd);
		trig_log(trig, buf);
	}
#endif

	return cl;
}

/*
* scans for a case/default instance
* returns the line containg the correct case instance, or NULL
*/
cmdlist_element::shared_ptr find_case(Trigger *trig,
									  cmdlist_element::shared_ptr cl,
									  void *go,
									  Script *sc,
									  int type,
									  const char *cond) {
	char result[kMaxInputLength];
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	eval_expr(cond, result, go, sc, trig, type);

	while ((cl = cl ? cl->next : cl) != nullptr) {
		for (p = cl->cmd.c_str(); *p && isspace(*reinterpret_cast<const unsigned char *>(p)); p++);

		if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7) || !strn_cmp("foreach ", p, 8)) {
			cl = find_done(trig, cl);
		} else if (!strn_cmp("case ", p, 5)) {
			char *tmpbuf = (char *) malloc(kMaxStringLength);
			eval_op("==", result, p + 5, tmpbuf, go, sc, trig);
			if (*tmpbuf && *tmpbuf != '0') {
				free(tmpbuf);
				break;
			}
			free(tmpbuf);
		} else if (!strn_cmp("default", p, 7)) {
			break;
		} else if (!strn_cmp("done", p, 4)) {
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl) {
		sprintf(buf, "closing 'done' not found for '%s'", cmd);
		trig_log(trig, buf);
	}
#endif

	return cl;
}

// processes any 'wait' commands in a trigger
void process_wait(void *go, Trigger *trig, int type, char *cmd, const cmdlist_element::shared_ptr &cl) {
	char *arg;
	struct wait_event_data *wait_event_obj;
	long time = 0, hr, min, ntime;
	char c;

	extern TimeInfoData time_info;

	if ((trig->get_attach_type() == MOB_TRIGGER && IS_SET(GET_TRIG_TYPE(trig), MTRIG_DEATH))
		||(trig->get_attach_type() == OBJ_TRIGGER && IS_SET(GET_TRIG_TYPE(trig), OTRIG_PURGE))) {
		sprintf(buf, "&YВНИМАНИЕ&G Используется wait в триггере '%s' (VNUM=%d).",
				GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig));
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
		sprintf(buf, "&GКод триггера после wait выполнен НЕ БУДЕТ!");
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	}

	arg = any_one_arg(cmd, buf);
	skip_spaces(&arg);

	if (!*arg) {
		sprintf(buf2, "wait w/o an arg: '%s'", cl->cmd.c_str());
		trig_log(trig, buf2);
	} else if (!strn_cmp(arg, "until ", 6))    // valid forms of time are 14:30 and 1430
	{
		if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
			min += (hr * 60);
		else
			min = (hr % 100) + ((hr / 100) * 60);

		// calculate the pulse of the day of "until" time
		ntime = (min * kSecsPerMudHour * kPassesPerSec) / 60;

		// calculate pulse of day of current time
		time = (GlobalObjects::heartbeat().global_pulse_number() % (kSecsPerMudHour * kPassesPerSec))
			+ (time_info.hours * kSecsPerMudHour * kPassesPerSec);

		if (time >= ntime)    // adjust for next day
			time = (kSecsPerMudDay * kPassesPerSec) - time + ntime;
		else
			time = ntime - time;
	} else {
		if (sscanf(arg, "%ld %c", &time, &c) == 2) {
			if (c == 't')
				time *= kPulsesPerMudHour;
			else if (c == 's')
				time *= kPassesPerSec;
		}
	}
//	if (time < 2)	//костыль пока не разберемся почему корректно не работает wait 1
//		time = 2;
	CREATE(wait_event_obj, 1);
	wait_event_obj->trigger = trig;
	wait_event_obj->go = go;
	wait_event_obj->type = type;

	if (GET_TRIG_WAIT(trig)) {
		trig_log(trig, "Wait structure already allocated for trigger");
	}

	GET_TRIG_WAIT(trig) = add_event(time, trig_wait_event, wait_event_obj);
	trig->curr_state = cl->next;
}

// processes a script set command
void process_set(Script * /*sc*/, Trigger *trig, char *cmd) {
	char arg[kMaxTrglineLength], name[kMaxTrglineLength], *value;

	value = two_arguments(cmd, arg, name);

	skip_spaces(&value);

	if (!*name) {
		sprintf(buf2, "set w/o an argument, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);
}

// processes a script eval command
void process_eval(void *go, Script *sc, Trigger *trig, int type, char *cmd) {
	char arg[kMaxTrglineLength], name[kMaxTrglineLength];
	char result[kMaxTrglineLength], *expr;

	expr = two_arguments(cmd, arg, name);

	skip_spaces(&expr);

	if (!*name) {
		sprintf(buf2, "eval w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	eval_expr(expr, result, go, sc, trig, type);
	add_var_cntx(&GET_TRIG_VARS(trig), name, result, 0);
}

// script attaching a trigger to something
void process_attach(void *go, Script *sc, Trigger *trig, int type, char *cmd) {
	char arg[kMaxInputLength], trignum_s[kMaxInputLength];
	char result[kMaxInputLength], *id_p;
	Trigger *newtrig;
	CharData *c = nullptr;
	ObjData *o = nullptr;
	RoomData *r = nullptr;
	long trignum;

	id_p = two_arguments(cmd, arg, trignum_s);
	skip_spaces(&id_p);

	if (!*trignum_s || atoi(trignum_s) == 0) {
		sprintf(buf2, "attach: нет или ошибка в аргументе 1: аргумент '%s', команда: '%s'", trignum_s, cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!id_p || !*id_p || atoi(id_p + 1) == 0) {
		sprintf(buf2, "attach: нет или ошибка в аргументе 2, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	// parse and locate the id specified
	eval_expr(id_p, result, go, sc, trig, type);

	c = get_char(id_p);
	if (!c) {
		o = get_obj(id_p);
		if (!o) {
			r = get_room(id_p);
			if (!r) {
				sprintf(buf2, "attach: не найден аргумент 2 (кому), команда: '%s'", cmd);
				trig_log(trig, buf2);
				return;
			}
		}
	} else {
		if (!c->IsNpc()) {
				sprintf(buf2, "attach: триггер нельзя прикрепить к игроку");
				trig_log(trig, buf2);
				return;
		}
	}

	// locate and load the trigger specified
	trignum = real_trigger(atoi(trignum_s));
	if (trignum >= 0
		&& (((c) && trig_index[trignum]->proto->get_attach_type() != MOB_TRIGGER)
			|| ((o) && trig_index[trignum]->proto->get_attach_type() != OBJ_TRIGGER)
			|| ((r) && trig_index[trignum]->proto->get_attach_type() != WLD_TRIGGER))) {
		sprintf(buf2, "attach trigger : '%s' invalid attach_type: %s expected %s", trignum_s,
				attach_name[(int) trig_index[trignum]->proto->get_attach_type()],
				attach_name[(c ? 0 : (o ? 1 : (r ? 2 : 3)))]);
		trig_log(trig, buf2);
		return;
	}

	if (trignum < 0 || !(newtrig = read_trigger(trignum))) {
		sprintf(buf2, "attach: invalid trigger: '%s'", trignum_s);
		trig_log(trig, buf2);
		return;
	}

	if (c) {
		if (add_trigger(SCRIPT(c).get(), newtrig, -1)) {
			add_trig_to_owner(trig_index[trig->get_rnum()]->vnum, trig_index[trignum]->vnum, GET_MOB_VNUM(c));
		} else {
			extract_trigger(newtrig);
		}

		return;
	}

	if (o) {
		if (add_trigger(o->get_script().get(), newtrig, -1)) {
			add_trig_to_owner(trig_index[trig->get_rnum()]->vnum, trig_index[trignum]->vnum, GET_OBJ_VNUM(o));
		} else {
			extract_trigger(newtrig);
		}
		return;
	}

	if (r) {
		if (add_trigger(SCRIPT(r).get(), newtrig, -1)) {
			add_trig_to_owner(trig_index[trig->get_rnum()]->vnum, trig_index[trignum]->vnum, r->room_vn);
		} else {
			extract_trigger(newtrig);
		}
		return;
	}

	return;
}

// script detaching a trigger from something
Trigger *process_detach(void *go, Script *sc, Trigger *trig, int type, char *cmd) {
	char arg[kMaxInputLength], trignum_s[kMaxInputLength];
	char result[kMaxInputLength], *id_p;
	CharData *c = nullptr;
	ObjData *o = nullptr;
	RoomData *r = nullptr;
	Trigger *retval = trig;

	id_p = two_arguments(cmd, arg, trignum_s);
	skip_spaces(&id_p);

	if (!*trignum_s) {
		sprintf(buf2, "detach w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return retval;
	}

	if (!id_p || !*id_p || atoi(id_p + 1) == 0) {
		sprintf(buf2, "detach invalid id arg(1), команда: '%s'", cmd);
		trig_log(trig, buf2);
		return retval;
	}

	// parse and locate the id specified
	eval_expr(id_p, result, go, sc, trig, type);

	c = get_char(id_p);
	if (!c) {
		o = get_obj(id_p);
		if (!o) {
			r = get_room(id_p);
			if (!r) {
				sprintf(buf2, "detach invalid id arg(2), команда: '%s'", cmd);
				trig_log(trig, buf2);
				return retval;
			}
		}
	}

	if (c && SCRIPT(c)->has_triggers()) {
		SCRIPT(c)->remove_trigger(trignum_s, retval);

		return retval;
	}

	if (o && o->get_script()->has_triggers()) {
		o->get_script()->remove_trigger(trignum_s, retval);

		return retval;
	}

	if (r && SCRIPT(r)->has_triggers()) {
		SCRIPT(r)->remove_trigger(trignum_s, retval);

		return retval;
	}

	return retval;
}

/* script run a trigger for something
   return true   - trigger find and runned
		  false  - trigger not runned
*/
int process_run(void *go, Script **sc, Trigger **trig, int type, char *cmd, int *retval) {
	char arg[kMaxInputLength], trignum_s[kMaxInputLength], *name, *cname;
	char result[kMaxInputLength], *id_p;
	Trigger *runtrig = nullptr;
	//	Script *runsc = NULL;
	struct TriggerVar *vd;
	CharData *c = nullptr;
	ObjData *o = nullptr;
	RoomData *r = nullptr;
	void *trggo = nullptr;
	int trgtype = 0, num = 0;
	bool is_string = false;

	id_p = two_arguments(cmd, arg, trignum_s);
	skip_spaces(&id_p);

	if (!*trignum_s) {
		sprintf(buf2, "run w/o an arg, команда: '%s'", cmd);
		trig_log(*trig, buf2);
		return (false);
	}

	if (!id_p || !*id_p || atoi(id_p + 1) == 0) {
		sprintf(buf2, "run invalid id arg(1), команда: '%s'", cmd);
		trig_log(*trig, buf2);
		return (false);
	}

	// parse and locate the id specified
	eval_expr(id_p, result, go, *sc, *trig, type);

	c = get_char(id_p);
	if (!c) {
		o = get_obj(id_p);
		if (!o) {
			r = get_room(id_p);
			if (!r) {
				sprintf(buf2, "run invalid id arg(3), команда: '%s'", cmd);
				trig_log(*trig, buf2);
				return (false);
			}
		}
	}

	name = trignum_s;
	if ((cname = strchr(name, '.')) || (!a_isdigit(*name))) {
		is_string = true;
		if (cname) {
			*cname = '\0';
			num = atoi(name);
			name = ++cname;
		}
	} else {
		num = atoi(name);
	}

	if (c && SCRIPT(c)->has_triggers()) {
		runtrig = SCRIPT(c)->trig_list.find(is_string, name, num);
		trgtype = MOB_TRIGGER;
		trggo = (void *) c;
	} else if (o && o->get_script()->has_triggers()) {
		runtrig = o->get_script()->trig_list.find(is_string, name, num);
		trgtype = OBJ_TRIGGER;
		trggo = (void *) o;
	} else if (r && SCRIPT(r)->has_triggers()) {
		runtrig = SCRIPT(r)->trig_list.find(is_string, name, num);
		trgtype = WLD_TRIGGER;
		trggo = (void *) r;
	};

	if (!runtrig) {
		return (false);
	}

	// copy variables
	if (*trig && runtrig) {
		for (vd = GET_TRIG_VARS(*trig); vd; vd = vd->next) {
			if (vd->context) {
				sprintf(buf2, "Local variable %s with nonzero context %ld", vd->name, vd->context);
				trig_log(*trig, buf2);
			}
			add_var_cntx(&GET_TRIG_VARS(runtrig), vd->name, vd->value, 0);
		}
	}

	if (!GET_TRIG_DEPTH(runtrig)) {
		*retval = script_driver(trggo, runtrig, trgtype, TRIG_NEW);
	}

	//TODO: Why only for char?
	if (go && type == MOB_TRIGGER && reinterpret_cast<CharData *>(go)->purged()) {
		*sc = nullptr;
		*trig = nullptr;
		return (false);
	}

	runtrig = nullptr;
	//TODO: How that possible?
	if (!go || (type == MOB_TRIGGER ? SCRIPT((CharData *) go).get() :
				type == OBJ_TRIGGER ? ((ObjData *) go)->get_script().get() :
				type == WLD_TRIGGER ? SCRIPT((RoomData *) go).get() : nullptr) != *sc) {
		*sc = nullptr;
		*trig = nullptr;
	} else {
		TriggersList *triggers_list = nullptr;
		switch (type) {
			case MOB_TRIGGER: triggers_list = &SCRIPT((CharData *) go)->trig_list;
				break;

			case OBJ_TRIGGER: triggers_list = &((ObjData *) go)->get_script()->trig_list;
				break;

			case WLD_TRIGGER: triggers_list = &SCRIPT((RoomData *) go)->trig_list;
				break;
		}

		if (triggers_list
			&& triggers_list->has_trigger(*trig)) {
			runtrig = *trig;
		}
	}

	*trig = runtrig;

	return (true);
}

CharData *dg_caster_owner_obj(ObjData *obj) {
	if (obj->get_carried_by()) {
		return obj->get_carried_by();
	}
	if (obj->get_worn_by()) {
		return obj->get_worn_by();
	}
	return nullptr;
}

RoomData *dg_room_of_obj(ObjData *obj) {

	if (obj->get_in_room() > kNowhere) {
		return world[obj->get_in_room()];
	}

	if (obj->get_carried_by()) {
		return world[obj->get_carried_by()->in_room];
	}

	if (obj->get_worn_by()) {
		return world[obj->get_worn_by()->in_room];
	}

	if (obj->get_in_obj()) {
		return dg_room_of_obj(obj->get_in_obj());
	}

	return nullptr;
}
void add_stuf_zone(Trigger *trig, char *cmd) {
	int obj_vnum, room_vnum, room_rnum;
		obj_vnum = number(15010, 15019);
		room_vnum = number(15021, 15084);
		ObjData::shared_ptr object;
		object = world_objects.create_from_prototype_by_vnum(obj_vnum);
		if (!object) {
			sprintf(buf2, "Add stuf: wrong ObjVnum %d, команда: '%s'", obj_vnum, cmd);
			trig_log(trig, buf2);
			return;
		}
		room_rnum = real_room(room_vnum);
		if (room_rnum != kNowhere) {
			object->set_vnum_zone_from(zone_table[world[room_rnum]->zone_rn].vnum);
			PlaceObjToRoom(object.get(), room_rnum);
			load_otrigger(object.get());
		}
		obj_vnum = number(15020, 15029);
		room_vnum = number(15021, 15084);
		object = world_objects.create_from_prototype_by_vnum(obj_vnum);
		if (!object) {
			sprintf(buf2, "Add stuf: wrong ObjVnum %d, команда: '%s'", obj_vnum, cmd);
			trig_log(trig, buf2);
			return;
		}
		room_rnum = real_room(room_vnum);
		if (room_rnum != kNowhere) {
			object->set_vnum_zone_from(zone_table[world[room_rnum]->zone_rn].vnum);
			PlaceObjToRoom(object.get(), room_rnum);
			load_otrigger(object.get());
		}
		obj_vnum = number(15030, 15039);
		room_vnum = number(15021, 15084);
		object = world_objects.create_from_prototype_by_vnum(obj_vnum);
		if (!object) {
			sprintf(buf2, "Add stuf: wrong ObjVnum %d, команда: '%s'", obj_vnum, cmd);
			trig_log(trig, buf2);
			return;
		}
		room_rnum = real_room(room_vnum);
		if (room_rnum != kNowhere) {
			object->set_vnum_zone_from(zone_table[world[room_rnum]->zone_rn].vnum);
			PlaceObjToRoom(object.get(), room_rnum);
			load_otrigger(object.get());
		}
}
// create a UID variable from the id number
void makeuid_var(void *go, Script *sc, Trigger *trig, int type, char *cmd) {
	char arg[kMaxInputLength], varname[kMaxInputLength];
	char result[kMaxInputLength], *uid_p;
	char uid[kMaxInputLength];

	uid_p = two_arguments(cmd, arg, varname);
	skip_spaces(&uid_p);

	if (!*varname) {
		sprintf(buf2, "makeuid w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!uid_p || !*uid_p || atoi(uid_p + 1) == 0) {
		sprintf(buf2, "makeuid invalid id arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	eval_expr(uid_p, result, go, sc, trig, type);
	sprintf(uid, "%s", result);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

/**
* Added 17/04/2000
* calculate a UID variable from the VNUM
* calcuid <переменная куда пишется id> <внум> <room|mob|obj> <порядковый номер от 1 до х>
* если порядковый не указан - возвращается первое вхождение.
*/
void calcuid_var(void *go, Script * /*sc*/, Trigger *trig, int type, char *cmd) {
	char arg[kMaxInputLength], varname[kMaxInputLength];
	char *t, vnum[kMaxInputLength], what[kMaxInputLength];
	char uid[kMaxInputLength], count[kMaxInputLength];
	char uid_type;
	long result = -1;

	t = two_arguments(cmd, arg, varname);
	three_arguments(t, vnum, what, count);

	if (!*varname) {
		sprintf(buf2, "calcuid w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*vnum || (result = atoi(vnum)) == 0) {
		sprintf(buf2, "calcuid invalid VNUM arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*what) {
		sprintf(buf2, "calcuid exceed TYPE arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	int count_num = 0;
	if (*count) {
		count_num = atoi(count) - 1;    //В dg индексация с 1
		if (count_num < 0) {
			//Произойдет, если в dg пришел индекс 0 (ошибка)
			sprintf(buf2, "calcuid invalid count: '%s'", count);
			trig_log(trig, buf2);
			return;
		}
	}

	if (!str_cmp(what, "room")) {
		uid_type = UID_ROOM;
		result = find_room_uid(result);
	} else if (!str_cmp(what, "mob")) {
		uid_type = UID_CHAR;
		result = find_char_vnum(result, count_num);
	} else if (!str_cmp(what, "obj")) {
		uid_type = UID_OBJ;
		result = find_obj_by_id_vnum__calcuid(result, count_num, type, go);
	} else {
		sprintf(buf2, "calcuid unknown TYPE arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (result <= -1) {
		sprintf(buf2, "calcuid target not found vnum: %s, count: %d.", vnum, count_num + 1);
		trig_log(trig, buf2);

		*uid = '\0';

		return;
	}

	sprintf(uid, "%c%ld", uid_type, result);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

/*
 * Поиск чаров с записью в переменную UID-а в случае онлайна человека
 * Возвращает в указанную переменную UID первого PC, с именем которого
 * совпадает аргумент
 */
void charuid_var(void * /*go*/, Script * /*sc*/, Trigger *trig, char *cmd) {
	char arg[kMaxInputLength], varname[kMaxInputLength];
	char who[kMaxInputLength], uid[kMaxInputLength];
	char uid_type = UID_CHAR;

	int result = -1;

	three_arguments(cmd, arg, varname, who);

	if (!*varname) {
		sprintf(buf2, "charuid w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*who) {
		sprintf(buf2, "charuid name is missing, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	for (const auto &tch : character_list) {
		if (tch->IsNpc()
			|| !HERE(tch)
			|| !isname(who, GET_NAME(tch))) {
			continue;
		}

		if (IN_ROOM(tch) != kNowhere) {
			result = GET_ID(tch);
		}
	}

	if (result <= -1) {
//		sprintf(buf2, "charuid target not found, name: '%s'", who);
//		trig_log(trig, buf2);
		*uid = '\0';
		return;
	}

	sprintf(uid, "%c%d", uid_type, result);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

// поиск всех чаров с лд
void charuidall_var(void * /*go*/, Script * /*sc*/, Trigger *trig, char *cmd) {
	char arg[kMaxInputLength], varname[kMaxInputLength];
	char who[kMaxInputLength], uid[kMaxInputLength];
	char uid_type = UID_CHAR_ALL;

	int result = -1;

	three_arguments(cmd, arg, varname, who);

	if (!*varname) {
		sprintf(buf2, "charuidall w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*who) {
		sprintf(buf2, "charuidall name is missing, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}
	for (const auto &tch : character_list) {
		if (tch->IsNpc()) {
			continue;
		}
		if (str_cmp(who, GET_NAME(tch))) {
			continue;
		}
		if (IN_ROOM(tch) != kNowhere) {
			result = GET_ID(tch);
			break;
		}
	}
	if (result <= -1) {
//		sprintf(buf2, "charuidld target not found, name: '%s'", who);
//		trig_log(trig, buf2);
		*uid = '\0';
		return;
	}
	sprintf(uid, "%c%d", uid_type, result);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}


// * Поиск мобов для calcuidall_var.
bool find_all_char_vnum(long n, char *str) {
	int count = 0;
	for (const auto &ch : character_list) {
		if (n == GET_MOB_VNUM(ch) && ch->in_room != kNowhere && (strlen(str + strlen(str)) < kMaxTrglineLength)) {
			snprintf(str + strlen(str), kMaxTrglineLength, "%c%ld ", UID_CHAR, GET_ID(ch));
			++count;
		}
	}

	return count ? true : false;
}

// * Поиск предметов для calcuidall_var.
bool find_all_obj_vnum(long n, char *str) {
	int count = 0;
	world_objects.foreach_with_vnum(n, [&](const ObjData::shared_ptr &i) {
		if (strlen(str + strlen(str)) < kMaxTrglineLength) {
			snprintf(str + strlen(str), kMaxTrglineLength, "%c%ld ", UID_OBJ, i->get_id());
			count++;
		}
	});

	return count ? true : false;
}

// * Копи-паст с calcuid_var для возврата строки со всеми найденными уидами мобов/предметов (до 25ти вхождений).
void calcuidall_var(void * /*go*/, Script * /*sc*/, Trigger *trig, int/* type*/, char *cmd) {
	char arg[kMaxTrglineLength], varname[kMaxTrglineLength];
	char *t, vnum[kMaxInputLength], what[kMaxInputLength];
	char uid[kMaxInputLength];
	int result = -1;

	uid[0] = '\0';
	t = two_arguments(cmd, arg, varname);
	two_arguments(t, vnum, what);

	if (!*varname) {
		sprintf(buf2, "calcuidall w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*vnum || (result = atoi(vnum)) == 0) {
		sprintf(buf2, "calcuidall invalid VNUM arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*what) {
		sprintf(buf2, "calcuidall exceed TYPE arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!str_cmp(what, "mob")) {
		result = find_all_char_vnum(result, uid);
	} else if (!str_cmp(what, "obj")) {
		result = find_all_obj_vnum(result, uid);
	} else {
		sprintf(buf2, "calcuidall unknown TYPE arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!result) {
		sprintf(buf2, "calcuidall target not found '%s'", vnum);
		trig_log(trig, buf2);
		*uid = '\0';
		return;
	}
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

/*
 * processes a script return command.
 * returns the new value for the script to return.
 */
int process_return(Trigger *trig, char *cmd) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	two_arguments(cmd, arg1, arg2);

	if (!*arg2) {
		sprintf(buf2, "return w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return 1;
	}

	return atoi(arg2);
}

/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
void process_unset(Script *sc, Trigger *trig, char *cmd) {
	char arg[kMaxInputLength], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var) {
		sprintf(buf2, "unset w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!remove_var_cntx(&worlds_vars, var, sc->context))
		if (!remove_var_cntx(&(sc->global_vars), var, sc->context))
			remove_var_cntx(&GET_TRIG_VARS(trig), var, 0);
}

/*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
void process_remote(Script *sc, Trigger *trig, char *cmd) {
	struct TriggerVar *vd;
	Script *sc_remote = nullptr;
	char *line, *var, *uid_p;
	char arg[kMaxInputLength];
	long uid, context;
	RoomData *room;
	CharData *mob;
	ObjData *obj;

	line = any_one_arg(cmd, arg);
	two_arguments(line, buf, buf2);
	var = buf;
	uid_p = buf2;
	skip_spaces(&var);
	skip_spaces(&uid_p);

	if (!*buf || !*buf2) {
		sprintf(buf2, "remote: invalid arguments, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	// find the locally owned variable
	vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);
	if (!vd)
		vd = find_var_cntx(&(sc->global_vars), var, sc->context);

	if (!vd) {
		snprintf(buf2, kMaxStringLength, "local var '%s' not found in remote call", buf);
		trig_log(trig, buf2);
		return;
	}

	// find the target script from the uid number
	uid = atoi(buf2 + 1);
	if (uid <= 0) {
//		std::stringstream buffer;
//		buffer << "remote: illegal uid " << buf2;
//		sprintf(buf, buffer.str());
		snprintf(buf, kMaxStringLength, "remote: illegal uid '%s'", buf2);
		trig_log(trig, buf);
		return;
	}

	// for all but PC's, context comes from the existing context.
	// for PC's, context is 0 (global)
//  context = vd->context;
// Контекст можно брать как vd->context или sc->context
// Если брать vd->context, то теряем контекст при переносе локальной переменной
// Если брать sc->context, то по-моему получится правильней, а именно:
// Для локальной переменной контекст значения не играет, т.о.
// если есть желание перенести локальную переменную заранее установите
// контекст в 0. Для глобальной переменной переменная с контекстом 0
// "покрывает" отсутствующие контексты
	context = sc->context;

	if ((room = get_room(buf2))) {
		sc_remote = SCRIPT(room).get();
	} else if ((mob = get_char(buf2))) {
		sc_remote = SCRIPT(mob).get();
		if (!mob->IsNpc()) {
			context = 0;
		}
	} else if ((obj = get_obj(buf2))) {
		sc_remote = obj->get_script().get();
	} else {
		sprintf(buf, "remote: uid '%ld' invalid", uid);
		trig_log(trig, buf);
		return;
	}

	if (sc_remote == nullptr) {
		return;        // no script to assign
	}

	add_var_cntx(&(sc_remote->global_vars), vd->name, vd->value, context);
}

/*
 * command-line interface to rdelete
 * named vdelete so people didn't think it was to delete rooms
 */
void do_vdelete(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	//  struct TriggerVar *vd, *vd_prev=NULL;
	Script *sc_remote = nullptr;
	char *var, *uid_p;
	long uid; //, context;
	RoomData *room;
	CharData *mob;
	ObjData *obj;

	argument = two_arguments(argument, buf, buf2);
	var = buf;
	uid_p = buf2;
	skip_spaces(&var);
	skip_spaces(&uid_p);

	if (!*buf || !*buf2) {
		SendMsgToChar("Usage: vdelete <variablename> <id>\r\n", ch);
		return;
	}


	// find the target script from the uid number
	uid = atoi(buf2 + 1);
	if (uid <= 0) {
		SendMsgToChar("vdelete: illegal id specified.\r\n", ch);
		return;
	}

	if ((room = get_room(buf2))) {
		sc_remote = SCRIPT(room).get();
	} else if ((mob = get_char(buf2))) {
		sc_remote = SCRIPT(mob).get();
	} else if ((obj = get_obj(buf2))) {
		sc_remote = obj->get_script().get();
	} else {
		SendMsgToChar("vdelete: cannot resolve specified id.\r\n", ch);
		return;
	}

	if ((sc_remote == nullptr) || (sc_remote->global_vars == nullptr)) {
		SendMsgToChar("That id represents no global variables.\r\n", ch);
		return;
	}

	// find the global
	if (remove_var_cntx(&(sc_remote->global_vars), var, 0)) {
		SendMsgToChar("Deleted.\r\n", ch);
	} else {
		SendMsgToChar("That variable cannot be located.\r\n", ch);
	}
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
void process_rdelete(Script *sc, Trigger *trig, char *cmd) {
	//  struct TriggerVar *vd, *vd_prev=NULL;
	Script *sc_remote = nullptr;
	char *line, *var, *uid_p;
	char arg[kMaxInputLength];
	long uid; //, context;
	RoomData *room;
	CharData *mob;
	ObjData *obj;

	line = any_one_arg(cmd, arg);
	two_arguments(line, buf, buf2);
	var = buf;
	uid_p = buf2;
	skip_spaces(&var);
	skip_spaces(&uid_p);

	if (!*buf || !*buf2) {
		sprintf(buf2, "rdelete: invalid arguments, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}


	// find the target script from the uid number
	uid = atoi(buf2 + 1);
	if (uid <= 0) {
		snprintf(buf, kMaxStringLength, "rdelete: illegal uid '%s'", buf2);
		trig_log(trig, buf);
		return;
	}

	if ((room = get_room(buf2))) {
		sc_remote = SCRIPT(room).get();
	} else if ((mob = get_char(buf2))) {
		sc_remote = SCRIPT(mob).get();
	} else if ((obj = get_obj(buf2))) {
		sc_remote = obj->get_script().get();
	} else {
		sprintf(buf, "remote: uid '%ld' invalid", uid);
		trig_log(trig, buf);
		return;
	}

	if (sc_remote == nullptr) {
		return;        // no script to delete a trigger from
	}
	if (sc_remote->global_vars == nullptr) {
		return;        // no script globals
	}

	// find the global
	remove_var_cntx(&(sc_remote->global_vars), var, sc->context);
}

// * makes a local variable into a global variable
void process_global(Script *sc, Trigger *trig, char *cmd, long id) {
	struct TriggerVar *vd;
	char arg[kMaxInputLength], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var) {
		sprintf(buf2, "global w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);

	if (!vd) {
		sprintf(buf2, "local var '%s' not found in global call", var);
		trig_log(trig, buf2);
		return;
	}

	add_var_cntx(&(sc->global_vars), vd->name, vd->value, id);
	remove_var_cntx(&GET_TRIG_VARS(trig), vd->name, 0);
}

// * makes a local variable into a world variable
void process_worlds(Script * /*sc*/, Trigger *trig, char *cmd, long id) {
	struct TriggerVar *vd;
	char arg[kMaxInputLength], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var) {
		sprintf(buf2, "worlds w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);

	if (!vd) {
		sprintf(buf2, "local var '%s' not found in worlds call", var);
		trig_log(trig, buf2);
		return;
	}

	add_var_cntx(&(worlds_vars), vd->name, vd->value, id);
	remove_var_cntx(&GET_TRIG_VARS(trig), vd->name, 0);
}

// set the current context for a script
void process_context(Script *sc, Trigger *trig, char *cmd) {
	char arg[kMaxInputLength], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var) {
		sprintf(buf2, "context w/o an arg, команда: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	sc->context = atol(var);
}

void extract_value(Script * /*sc*/, Trigger *trig, char *cmd) {
	char buf2[kMaxTrglineLength];
	char *buf3;
	char to[128];
	int num;

	buf3 = any_one_arg(cmd, buf);
	half_chop(buf3, buf2, buf);
	strcpy(to, buf2);

	num = atoi(buf);
	if (num < 1) {
		trig_log(trig, "extract number < 1!");
		return;
	}

	half_chop(buf, buf3, buf2);

	while (num > 0) {
		half_chop(buf2, buf, buf2);
		num--;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), to, buf, 0);
}

//  This is the core driver for scripts.
//  define this if you want measure time of you scripts
int timed_script_driver(void *go, Trigger *trig, int type, int mode);

int script_driver(void *go, Trigger *trig, int type, int mode) {
	int timewarning = 50; //  в текущий момент миллисекунды

	CharacterLinkDrop = false;
	const auto vnum = GET_TRIG_VNUM(trig);

	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto start = now_ms.time_since_epoch();

	const auto return_code = timed_script_driver(go, trig, type, mode);

	now = std::chrono::system_clock::now();
	now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto end = now_ms.time_since_epoch();

	long timediff = end.count() - start.count();
	if (timediff > timewarning) { 
		snprintf(buf, kMaxStringLength, "[TrigVNum: %d] : work time overflow %ld ms, warning > %d ms.", vnum, timediff, timewarning);
		mudlog(buf, BRF, -1, ERRLOG, true);
	}
	// Stop time
	return return_code;
}

void do_dg_add_ice_currency(void * /*go*/, Script * /*sc*/, Trigger *trig, int/* script_type*/, char *cmd) {
	CharData *ch = nullptr;
	int value;
	char junk[kMaxInputLength];
	char charname[kMaxInputLength], value_c[kMaxInputLength];

	half_chop(cmd, junk, cmd);
	half_chop(cmd, charname, cmd);
	half_chop(cmd, value_c, cmd);

	if (!*charname || !*value_c) {
		sprintf(buf2, "dg_addicecurrency usage: <target> <value>");
		trig_log(trig, buf2);
		return;
	}

	value = atoi(value_c);
	// locate the target
	ch = get_char(charname);
	if (!ch) {
		sprintf(buf2, "dg_addicecurrency: cannot locate target!");
		trig_log(trig, buf2);
		return;
	}
	ch->add_ice_currency(value);
}

int timed_script_driver(void *go, Trigger *trig, int type, int mode) {
	static int depth = 0;
	int ret_val = 1, stop = false;
	char cmd[kMaxTrglineLength];
	Script *sc = 0;
	unsigned long loops = 0;
	Trigger *prev_trig;

	curr_trig_vnum = GET_TRIG_VNUM(trig);
	void mob_command_interpreter(CharData *ch, char *argument, Trigger *trig);
	void obj_command_interpreter(ObjData *obj, char *argument, Trigger *trig);
	void wld_command_interpreter(RoomData *room, char *argument, Trigger *trig);

	if (depth > kMaxScriptDepth) {
		trig_log(trig, "Triggers recursed beyond maximum allowed depth.");
		return ret_val;
	}

	switch (type) {
		case MOB_TRIGGER: sc = SCRIPT((CharData *) go).get();
			break;

		case OBJ_TRIGGER: sc = ((ObjData *) go)->get_script().get();
			break;

		case WLD_TRIGGER: sc = SCRIPT((RoomData *) go).get();
			break;
	}

	if (sc->is_purged()) {
		return ret_val;
	}

	prev_trig = cur_trig;
	cur_trig = trig;

	depth++;
	last_trig_vnum = GET_TRIG_VNUM(trig);

	if (mode == TRIG_NEW) {
		GET_TRIG_DEPTH(trig) = 1;
		GET_TRIG_LOOPS(trig) = 0;
		sc->context = 0;
	}

	for (auto cl = (mode == TRIG_NEW) ? *trig->cmdlist : trig->curr_state; !stop && cl && trig && GET_TRIG_DEPTH(trig);
		 cl = cl ? cl->next : cl)    //log("Drive go <%s>",cl->cmd);
	{
		if (CharacterLinkDrop) {
			sprintf(buf, "[TrigVnum: %d] Character in LinkDrop in 'Drive go'.", last_trig_vnum);
			mudlog(buf, BRF, -1, ERRLOG, true);
			break;
		}
		const char *p = nullptr;
		for (p = cl->cmd.c_str(); !stop && trig && *p && isspace(*p); p++);

		if (*p == '*' || *p == '/')    // comment
		{
			continue;
		} else if (!strn_cmp(p, "if ", 3)) {
			if (process_if(p + 3, go, sc, trig, type)) {
				GET_TRIG_DEPTH(trig)++;
			} else {
				cl = find_else_end(trig, cl, go, sc, type);
			}
			if (CharacterLinkDrop) {
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else if (!strn_cmp("elseif ", p, 7) || !strn_cmp("else", p, 4)) {
			cl = find_end(trig, cl);
			GET_TRIG_DEPTH(trig)--;
			if (CharacterLinkDrop) {
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else if (!strn_cmp("while ", p, 6)) {
			const auto temp = find_done(trig, cl);
			if (process_if(p + 6, go, sc, trig, type)) {
				if (temp) {
					temp->original = cl;
				}
			} else {
				cl = temp;
				loops = 0;
			}
			if (CharacterLinkDrop) {
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else if (!strn_cmp("foreach ", p, 8)) {
			const auto temp = find_done(trig, cl);
			if (process_foreach_begin(p + 8, go, sc, trig, type)) {
				if (temp) {
					temp->original = cl;
				}
			} else {
				cl = temp;
				loops = 0;
			}
			if (CharacterLinkDrop) {
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else if (!strn_cmp("switch ", p, 7)) {
			cl = find_case(trig, cl, go, sc, type, p + 7);
			if (CharacterLinkDrop) {
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else if (!strn_cmp("end", p, 3)) {
			GET_TRIG_DEPTH(trig)--;
		} else if (!strn_cmp("done", p, 4) || !strn_cmp("continue", p, 8)) {
			if (cl->original) {
				auto orig_cmd = cl->original->cmd.c_str();
				while (*orig_cmd && isspace(*orig_cmd)) {
					orig_cmd++;
				}

				if ((*orig_cmd == 'w' && process_if(orig_cmd + 6, go, sc, trig, type))
					|| (*orig_cmd == 'f' && process_foreach_done(orig_cmd + 8, go, sc, trig, type))) {
					cl = cl->original;
					loops++;
					GET_TRIG_LOOPS(trig)++;
					if (loops == 30) {
						snprintf(buf2, kMaxStringLength, "wait 1");
						process_wait(go, trig, type, buf2, cl);
						depth--;
						cur_trig = prev_trig;
						return ret_val;
					}

					if (GET_TRIG_LOOPS(trig) == 300) {
						trig_log(trig, "looping 300 times.", LGH);
					}

					if (GET_TRIG_LOOPS(trig) == 1000) {
						trig_log(trig, "looping 1000 times.", DEF);
					}
				}
			}
		} else if (!strn_cmp("break", p, 5)) {
			cl = find_done(trig, cl);
		} else if (!strn_cmp("case", p, 4))    // Do nothing, this allows multiple cases to a single instance
		{
		} else {
			var_subst(go, sc, trig, type, p, cmd);
			if (CharacterLinkDrop) {
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
			if (!strn_cmp(cmd, "eval ", 5)) {
				process_eval(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "nop ", 4)) { ;    // nop: do nothing
			} else if (!strn_cmp(cmd, "add_stuf_zone ", 14)) {
				add_stuf_zone(trig, cmd);
			} else if (!strn_cmp(cmd, "extract ", 8)) {
				extract_value(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "makeuid ", 8)) {
				makeuid_var(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "calcuid ", 8)) {
				calcuid_var(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "calcuidall ", 11)) {
				calcuidall_var(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "charuid ", 8)) {
				charuid_var(go, sc, trig, cmd);
			} else if (!strn_cmp(cmd, "charuidall ", 11)) {
				charuidall_var(go, sc, trig, cmd);
			} else if (!strn_cmp(cmd, "halt", 4)) {
				break;
			} else if (!strn_cmp(cmd, "DgCast ", 7)) {
				do_dg_cast(go, trig, type, cmd);
				if (type == MOB_TRIGGER && reinterpret_cast<CharData *>(go)->purged()) {
					depth--;
					cur_trig = prev_trig;
					return ret_val;
				}
			} else if (!strn_cmp(cmd, "DgAffect ", 9)) {
				do_dg_affect(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "global ", 7)) {
				process_global(sc, trig, cmd, sc->context);
			} else if (!strn_cmp(cmd, "addicecurrency ", 15)) {
				do_dg_add_ice_currency(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "bonus ", 6)) {
				Bonus::dg_do_bonus(cmd + 6);
			} else if (!strn_cmp(cmd, "worldecho ", 10)) {
				do_worldecho(cmd + 10);
			} else if (!strn_cmp(cmd, "worlds ", 7)) {
				process_worlds(sc, trig, cmd, sc->context);
			} else if (!strn_cmp(cmd, "context ", 8)) {
				process_context(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "remote ", 7)) {
				process_remote(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "rdelete ", 8)) {
				process_rdelete(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "return ", 7)) {
				ret_val = process_return(trig, cmd);
			} else if (!strn_cmp(cmd, "set ", 4)) {
				process_set(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "unset ", 6)) {
				process_unset(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "log ", 4)) {
				trig_log(trig, cmd + 4);
			} else if (!strn_cmp(cmd, "syslog ", 7)) {
				log("SCRIPT LOG (Trigger: %s, VNum: %i) : %s", GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd + 7);
			} else if (!strn_cmp(cmd, "wait ", 5)) {
				process_wait(go, trig, type, cmd, cl);
				depth--;
				cur_trig = prev_trig;
				return ret_val;
			} else if (!strn_cmp(cmd, "attach ", 7)) {
				process_attach(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "detach ", 7)) {
				trig = process_detach(go, sc, trig, type, cmd);
			} else if (!strn_cmp(cmd, "run ", 4)) {
				process_run(go, &sc, &trig, type, cmd, &ret_val);
				if (!trig || !sc) {
					depth--;
					cur_trig = prev_trig;
					return ret_val;
				}
				stop = ret_val;
			} else if (!strn_cmp(cmd, "exec ", 5)) {
				process_run(go, &sc, &trig, type, cmd, &ret_val);
				if (!trig || !sc) {
					depth--;
					cur_trig = prev_trig;
					return ret_val;
				}
			} else if (!strn_cmp(cmd, "arena_round", 11)) {
				process_arena_round(sc, trig, cmd);
			} else if (!strn_cmp(cmd, "version", 7)) {
				mudlog(DG_SCRIPT_VERSION, BRF, kLvlBuilder, SYSLOG, true);
			} else {
				switch (type) {
					case MOB_TRIGGER:
						//last_trig_vnum = GET_TRIG_VNUM(trig);
						mob_command_interpreter((CharData *) (go), cmd, trig);
						break;

					case OBJ_TRIGGER:
						//last_trig_vnum = GET_TRIG_VNUM(trig);
						obj_command_interpreter((ObjData *) go, cmd, trig);
						break;

					case WLD_TRIGGER:
						//last_trig_vnum = GET_TRIG_VNUM(trig);
						wld_command_interpreter((RoomData *) go, cmd, trig);
						break;
				}
			}

			if (sc->is_purged() || (type == MOB_TRIGGER && reinterpret_cast<CharData *>(go)->purged())) {
				depth--;
				cur_trig = prev_trig;
				return ret_val;
			}
		}
	}

	if (trig) {
		trig->clear_var_list();
		GET_TRIG_VARS(trig) = nullptr;
		GET_TRIG_DEPTH(trig) = 0;
	}

	depth--;
	cur_trig = prev_trig;
	return ret_val;
}

void do_worldecho(char *msg) {
	for (auto d = descriptor_list; d; d = d->next) {
		if (STATE(d) == CON_PLAYING) {
			SendMsgToChar(d->character.get(), "%s\r\n", msg);
		}
	}
}

void do_tlist(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	int first, last, nr, found = 0;
	char pagebuf[65536];

	strcpy(pagebuf, "");

	two_arguments(argument, buf, buf2);

	first = atoi(buf);

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) != first)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (!*buf) {
		SendMsgToChar("Usage: tlist <begining number or zone> [<ending number>]\r\n", ch);
		return;
	}

	first = atoi(buf);
	if (*buf2)
		last = atoi(buf2);
	else {
		first *= 100;
		last = first + 99;
	}

	if ((first < 0) || (first > kMaxProtoNumber) || (last < 0) || (last > kMaxProtoNumber)) {
		sprintf(buf, "Значения должны быть между 0 и %d.\n\r", kMaxProtoNumber);
		SendMsgToChar(buf, ch);
	}

	if (first >= last) {
		SendMsgToChar("Второе значение должно быть больше первого.\n\r", ch);
		return;
	}

	if (first + 200 < last) {
		SendMsgToChar("Максимальный показываемый промежуток - 200.\n\r", ch);
		return;
	}
	char trgtypes[256];
	for (nr = 0; nr < top_of_trigt && (trig_index[nr]->vnum <= last); nr++) {
		if (trig_index[nr]->vnum >= first) {
			std::string out = "";
			sprintf(buf,"%2d) [%5d] [%-50s] ", ++found,
					trig_index[nr]->vnum, trig_index[nr]->proto->get_name().c_str());
			out += buf;
			if (trig_index[nr]->proto->get_attach_type() == MOB_TRIGGER) {
				sprintbit(trig_index[nr]->proto->get_trigger_type(), trig_types, trgtypes);
				out += "[MOB] ";
				out += trgtypes;
			}
			if (trig_index[nr]->proto->get_attach_type() == OBJ_TRIGGER) {
				sprintbit(GET_TRIG_TYPE(trig_index[nr]->proto), otrig_types, trgtypes);
				out += "[OBJ] ";
				out += trgtypes;
			}
			if (trig_index[nr]->proto->get_attach_type() == WLD_TRIGGER) {
				sprintbit(GET_TRIG_TYPE(trig_index[nr]->proto), wtrig_types, trgtypes);
				out += "[WLD] ";
				out += trgtypes;
			}
			out += "\r\nПрикреплен к: ";
			if (!owner_trig[trig_index[nr]->vnum].empty()) {
				for (auto it = owner_trig[trig_index[nr]->vnum].begin(); it != owner_trig[trig_index[nr]->vnum].end();
					 ++it) {
					out += "[";
					std::string out_tmp = "";
					for (const auto trigger_vnum : it->second) {
						sprintf(buf, " %d", trigger_vnum);
						out_tmp += buf;
					}

					if (it->first != -1) {
						out += std::to_string(it->first) + " : ";
					}

					out += out_tmp + "]";
				}

				out += "\r\n";
			} else {
				out += "Отсутствуют\r\n";
			}
			strcat(pagebuf, out.c_str());
		}
	}

	if (!found) {
		SendMsgToChar("No triggers were found in those parameters.\n\r", ch);
	} else {
		page_string(ch->desc, pagebuf, true);
	}
}

int real_trigger(int vnum) {
	int rnum;

	for (rnum = 0; rnum < top_of_trigt; rnum++) {
		if (trig_index[rnum]->vnum == vnum)
			break;
	}

	if (rnum == top_of_trigt)
		rnum = -1;
	return (rnum);
}

void do_tstat(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	int vnum, rnum;
	char str[kMaxInputLength];

	half_chop(argument, str, argument);

	auto first = atoi(str);
	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) != first)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (*str) {
		vnum = atoi(str);
		rnum = real_trigger(vnum);
		if (rnum < 0) {
			SendMsgToChar("That vnum does not exist.\r\n", ch);
			return;
		}

		do_stat_trigger(ch, trig_index[rnum]->proto);
	} else
		SendMsgToChar("Usage: tstat <vnum>\r\n", ch);
}

// read a line in from a file, return the number of entities read
int fgetline(FILE *file, char *p) {
	int count = 0;

	do {
		*p = fgetc(file);
		if (*p != '\n' && !feof(file)) {
			p++;
			count++;
		}
	} while (*p != '\n' && !feof(file));

	if (*p == '\n')
		*p = '\0';

	return count;
}

// load in a character's saved variables
void read_saved_vars(CharData *ch) {
	FILE *file;
	long context;
	char fn[127];
	char input_line[1024], *p;
	char varname[32], *v;
	char context_str[16], *c;

	// find the file that holds the saved variables and open it
	get_filename(GET_NAME(ch), fn, kScriptVarsFile);
	file = fopen(fn, "r");

	// if we failed to open the file, return
	if (!file) {
		return;
	}

	// walk through each line in the file parsing variables
	do {
		if (fgetline(file, input_line) > 0) {
			p = input_line;
			v = varname;
			c = context_str;
			skip_spaces(&p);
			while (*p && *p != ' ' && *p != '\t')
				*v++ = *p++;
			*v = '\0';
			skip_spaces(&p);
			while (*p && *p != ' ' && *p != '\t')
				*c++ = *p++;
			*c = '\0';
			skip_spaces(&p);

			context = atol(context_str);
			add_var_cntx(&(SCRIPT(ch)->global_vars), varname, p, context);
		}
	} while (!feof(file));

	// close the file and return
	fclose(file);
}

// save a characters variables out to disk
void save_char_vars(CharData *ch) {
	FILE *file;
	char fn[127];
	struct TriggerVar *vars;

	// we should never be called for an NPC, but just in case...
	if (ch->IsNpc())
		return;

	get_filename(GET_NAME(ch), fn, kScriptVarsFile);
	std::remove(fn);

	// make sure this char has global variables to save
	if (ch->script->global_vars == nullptr)
		return;

	vars = ch->script->global_vars;

	file = fopen(fn, "wt");

	// note that currently, context will always be zero. this may change
	// in the future
	while (vars) {
		if (*vars->name != '-')    // don't save if it begins with -
			fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
		vars = vars->next;
	}

	fclose(file);
}

class TriggerRemoveObserverI : public TriggerEventObserver {
 public:
	TriggerRemoveObserverI(TriggersList::iterator *owner) : m_owner(owner) {}

	virtual void notify(Trigger *trigger) override;

 private:
	TriggersList::iterator *m_owner;
};

void TriggerRemoveObserverI::notify(Trigger *trigger) {
	m_owner->remove_event(trigger);
}

TriggersList::iterator::iterator(TriggersList *owner, const IteratorPosition position)
	: m_owner(owner), m_removed(false) {
	m_iterator = position == BEGIN ? m_owner->m_list.begin() : m_owner->m_list.end();
	setup_observer();
}

TriggersList::iterator::iterator(const iterator &rhv)
	: m_owner(rhv.m_owner), m_iterator(rhv.m_iterator), m_removed(rhv.m_removed) {
	setup_observer();
}

TriggersList::iterator::~iterator() {
	m_owner->unregister_observer(m_observer);
}

TriggersList::TriggersList::iterator &TriggersList::iterator::operator++() {
	if (!m_removed) {
		++m_iterator;
	} else {
		m_removed = false;
	}

	return *this;
}

void TriggersList::iterator::setup_observer() {
	m_observer = std::make_shared<TriggerRemoveObserverI>(this);
	m_owner->register_observer(m_observer);
}

void TriggersList::iterator::remove_event(Trigger *trigger) {
	if (m_iterator != m_owner->m_list.end()
		&& *m_iterator == trigger) {
		++m_iterator;
		m_removed = true;
	}
}

class TriggerRemoveObserver : public TriggerEventObserver {
 public:
	TriggerRemoveObserver(TriggersList *owner) : m_owner(owner) {}

	virtual void notify(Trigger *trigger) override;

 private:
	TriggersList *m_owner;
};

void TriggerRemoveObserver::notify(Trigger *trigger) {
	m_owner->remove(trigger);
}

TriggersList::TriggersList() {
	m_observer = std::make_shared<TriggerRemoveObserver>(this);
}

TriggersList::~TriggersList() {
	clear();
}

// Return true if trigger successfully added
// Return false if trigger with same rnum already exists and can not be added
bool TriggersList::add(Trigger *trigger, const bool to_front /*= false*/) {
	for (const auto &i : m_list) {
		if (trigger->get_rnum() == i->get_rnum()) {
			//Мы не можем здесь заменить имеющийся триггер на новый
			//т.к. имеющийся триггер может уже использоваться или быть в ожидание (wait command)
			return false;
		}
	}

	trigger_list.register_remove_observer(trigger, m_observer);

	if (to_front) {
		m_list.push_front(trigger);
	} else {
		m_list.push_back(trigger);
	}

	return true;
}

void TriggersList::remove(Trigger *const trigger) {
	const auto i = std::find(m_list.begin(), m_list.end(), trigger);
	if (i != m_list.end()) {
		remove(i);
	}
}

TriggersList::triggers_list_t::iterator TriggersList::remove(const triggers_list_t::iterator &iterator) {
	Trigger *trigger = *iterator;
	trigger_list.unregister_remove_observer(trigger, m_observer);

	for (const auto &observer : m_iterator_observers) {
		observer->notify(trigger);
	}

	triggers_list_t::iterator result = m_list.erase(iterator);

	extract_trigger(trigger);

	return result;
}

void TriggersList::register_observer(const TriggerEventObserver::shared_ptr &observer) {
	m_iterator_observers.insert(observer);
}

void TriggersList::unregister_observer(const TriggerEventObserver::shared_ptr &observer) {
	m_iterator_observers.erase(observer);
}

Trigger *TriggersList::find(const bool by_name, const char *name, const int vnum_or_position) {
	if (by_name) {
		return find_by_name(name, vnum_or_position);
	} else {
		return find_by_vnum_or_position(vnum_or_position);
	}
}

Trigger *TriggersList::find_by_name(const char *name, const int number) {
	Trigger *result = nullptr;

	int n = 0;
	for (const auto &i : m_list) {
		if (isname(name, GET_TRIG_NAME(i))) {
			++n;
			if (n >= number) {
				result = i;
				break;
			}
		}
	}

	return result;
}

Trigger *TriggersList::find_by_vnum_or_position(const int vnum_or_position) {
	Trigger *result = nullptr;

	int n = 0;
	for (const auto &i : m_list) {
		++n;
		if (n >= vnum_or_position) {
			result = i;
			break;
		}

		if (trig_index[i->get_rnum()]->vnum == vnum_or_position) {
			result = i;
			break;
		}
	}

	return result;
}

Trigger *TriggersList::find_by_vnum(const int vnum) {
	Trigger *result = nullptr;

	for (const auto &i : m_list) {
		if (trig_index[i->get_rnum()]->vnum == vnum) {
			result = i;
			break;
		}
	}

	return result;
}

Trigger *TriggersList::remove_by_name(const char *name, const int number) {
	Trigger *to_remove = find_by_name(name, number);

	if (to_remove) {
		remove(to_remove);
	}

	return to_remove;
}

Trigger *TriggersList::remove_by_vnum_or_position(const int vnum_or_position) {
	Trigger *to_remove = find_by_vnum_or_position(vnum_or_position);

	if (to_remove) {
		remove(to_remove);
	}

	return to_remove;
}

Trigger *TriggersList::remove_by_vnum(const int vnum) {
	Trigger *to_remove = find_by_vnum(vnum);

	if (to_remove) {
		remove(to_remove);
	}

	return to_remove;
}

long TriggersList::get_type() const {
	long result = 0;
	for (const auto &i : m_list) {
		result |= i->get_trigger_type();
	}

	return result;
}

bool TriggersList::has_trigger(const Trigger *const trigger) {
	for (const auto &i : m_list) {
		if (i == trigger) {
			return true;
		}
	}

	return false;
}

void TriggersList::clear() {
	while (!m_list.empty()) {
		remove(*m_list.begin());    // Do all stuff related to removing trigger from the list. Specifically, unregister all observers.
	}
}

std::ostream &TriggersList::dump(std::ostream &os) const {
	bool first = true;
	for (const auto t : m_list) {
		if (!first) {
			os << ", ";
		}
		os << trig_index[t->get_rnum()]->vnum;
		first = false;
	}

	return os;
}

Script::Script() :
	types(0),
	global_vars(nullptr),
	context(0),
	m_purged(false) {
}

// don't copy anything for now
Script::Script(const Script &) :
	types(0),
	global_vars(nullptr),
	context(0),
	m_purged(false) {
}

Script::~Script() {
	trig_list.clear();
	clear_global_vars();
}

int Script::remove_trigger(char *name, Trigger *&trig_addr) {
	int num = 0;
	char *cname;

	bool string = false;
	if ((cname = strchr(name, '.')) || (!a_isdigit(*name))) {
		string = true;
		if (cname) {
			*cname = '\0';
			num = atoi(name);
			name = ++cname;
		}
	} else {
		num = atoi(name);
	}

	Trigger *address_of_removed_trigger = nullptr;
	if (string) {
		address_of_removed_trigger = trig_list.remove_by_name(name, num);
	} else {
		address_of_removed_trigger = trig_list.remove_by_vnum_or_position(num);
	}

	if (!address_of_removed_trigger) {
		return 0;    // nothing has been removed
	}

	if (address_of_removed_trigger == trig_addr) {
		trig_addr =
			nullptr;    // mark that this trigger was removed: trig_addr is not null if trigger is still in the memory
	}

	// update the script type bitvector
	types = trig_list.get_type();

	return 1;
}

int Script::remove_trigger(char *name) {
	Trigger *dummy = nullptr;
	return remove_trigger(name, dummy);
}

void Script::clear_global_vars() {
	for (auto i = global_vars; i;) {
		auto j = i;
		i = i->next;
		if (j->name)
			free(j->name);
		if (j->value)
			free(j->value);
		free(j);
	}
}

void Script::cleanup() {
	trig_list.clear();

	clear_global_vars();
	global_vars = nullptr;
}

const char *Trigger::DEFAULT_TRIGGER_NAME = "no name";

Trigger::Trigger() :
	cmdlist(new cmdlist_element::shared_ptr()),
	narg(0),
	depth(0),
	loops(-1),
	wait_event(nullptr),
	var_list(nullptr),
	nr(kNothing),
	attach_type(0),
	name(DEFAULT_TRIGGER_NAME),
	trigger_type(0) {
}

Trigger::Trigger(const sh_int rnum, const char *name, const byte attach_type, const long trigger_type) :
	cmdlist(new cmdlist_element::shared_ptr()),
	narg(0),
	depth(0),
	loops(-1),
	wait_event(nullptr),
	var_list(nullptr),
	nr(rnum),
	attach_type(attach_type),
	name(name),
	trigger_type(trigger_type) {
}

Trigger::Trigger(const sh_int rnum, std::string &&name, const byte attach_type, const long trigger_type) :
	cmdlist(new cmdlist_element::shared_ptr()),
	narg(0),
	depth(0),
	loops(-1),
	wait_event(nullptr),
	var_list(nullptr),
	nr(rnum),
	attach_type(attach_type),
	name(name),
	trigger_type(trigger_type) {
}

Trigger::Trigger(const sh_int rnum, const char *name, const long trigger_type) : Trigger(rnum,
																						 name,
																						 0,
																						 trigger_type) {
}

Trigger::Trigger(const Trigger &from) :
	cmdlist(from.cmdlist),
	narg(from.narg),
	arglist(from.arglist),
	depth(from.depth),
	loops(from.loops),
	wait_event(nullptr),
	var_list(nullptr),
	nr(from.nr),
	attach_type(from.attach_type),
	name(from.name),
	trigger_type(from.trigger_type) {
}

void Trigger::reset() {
	nr = kNothing;
	attach_type = 0;
	name = DEFAULT_TRIGGER_NAME;
	trigger_type = 0;
	cmdlist.reset();
	curr_state.reset();
	narg = 0;
	arglist.clear();
	depth = 0;
	loops = -1;
	wait_event = nullptr;
	var_list = nullptr;
}

Trigger &Trigger::operator=(const Trigger &right) {
	reset();

	nr = right.nr;
	set_attach_type(right.get_attach_type());
	name = right.name;
	trigger_type = right.trigger_type;
	cmdlist = right.cmdlist;
	narg = right.narg;
	arglist = right.arglist;

	return *this;
}

void Trigger::clear_var_list() {
	for (auto i = var_list; i;) {
		auto j = i;
		i = i->next;
		if (j->name)
			free(j->name);
		if (j->value)
			free(j->value);
		free(j);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
