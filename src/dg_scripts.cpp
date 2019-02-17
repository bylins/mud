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

#include "global.objects.hpp"
#include "world.characters.hpp"
#include "heartbeat.hpp"
#include "find.obj.id.by.vnum.hpp"
#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "dg_event.h"
#include "db.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "top.h"
#include "features.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "name_list.hpp"
#include "modify.h"
#include "room.hpp"
#include "named_stuff.hpp"
#include "spell_parser.hpp"
#include "spells.h"
#include "skills.h"
#include "noob.hpp"
#include "genchar.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "dg_db_scripts.hpp"
#include "bonus.h"
#include "zone.table.hpp"
#include "debug.utils.hpp"
#include "backtrace.hpp"
#include "coredump.hpp"

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)

#define IS_CHARMED(ch)          (IS_HORSE(ch)||AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))

// Вывод сообщений о неверных управляющих конструкциях DGScript
#define	DG_CODE_ANALYZE

// external vars from triggers.cpp
extern const char *trig_types[], *otrig_types[], *wtrig_types[];
const char *attach_name[] = { "mob", "obj", "room", "unknown!!!" };

int last_trig_vnum = 0;

// other external vars

extern void add_trig_to_owner(int vnum_owner, int vnum_trig, int vnum);
extern CHAR_DATA *combat_list;
extern const char *item_types[];
extern const char *genders[];
extern const char *pc_class_types[];
//extern const char *race_types[];
extern const char *exit_bits[];
extern INDEX_DATA *mob_index;
extern TIME_INFO_DATA time_info;
const char *spell_name(int num);

extern int can_take_obj(CHAR_DATA * ch, OBJ_DATA * obj);
extern void split_or_clan_tax(CHAR_DATA *ch, long amount);

// external functions
room_rnum find_target_room(CHAR_DATA * ch, char *rawroomstr, int trig);
void free_varlist(struct trig_var_data *vd);
int obj_room(OBJ_DATA * obj);
bool is_empty(int zone_nr);
TRIG_DATA *read_trigger(int nr);
OBJ_DATA *get_object_in_equip(CHAR_DATA * ch, char *name);
void extract_trigger(TRIG_DATA * trig);
int eval_lhs_op_rhs(const char *expr, char *result, void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type);
const char * skill_percent(CHAR_DATA * ch, char *skill);
bool feat_owner(TRIG_DATA* trig, CHAR_DATA * ch, char *feat);
const char * spell_count(TRIG_DATA* trig, CHAR_DATA * ch, char *spell);
const char * spell_knowledge(TRIG_DATA* trig, CHAR_DATA * ch, char *spell);
int find_eq_pos(CHAR_DATA * ch, OBJ_DATA * obj, char *arg);
void reset_zone(int znum);

void do_restore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mpurge(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mjunk(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_arena_restore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
// function protos from this file
void extract_value(SCRIPT_DATA * sc, TRIG_DATA * trig, char *cmd);
int script_driver(void *go, TRIG_DATA * trig, int type, int mode);
int trgvar_in_room(int vnum);

/*
Костыль, но всеж.
*/
bool CharacterLinkDrop = false;

//table for replace UID_CHAR, UID_OBJ, UID_ROOM
const char uid_replace_table[] = {
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',	//16
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b', '\x20', '\x20', '\x20', '\x1f',	//32
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',	//48
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',	//64
	'\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',	//80
	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',	//96
	'\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',	//112
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',	//128
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',	//144
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',	//160
	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',	//176
	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',	//192
	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',	//208
	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',	//224
	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',	//240
	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff'	//256
};

void script_log(const char *msg, const int type)
{
	char tmpbuf[MAX_STRING_LENGTH];

	snprintf(tmpbuf, MAX_STRING_LENGTH, "SCRIPT LOG %s", msg);

	char* pos = tmpbuf;
	while (*pos != '\0')
	{
		*pos = uid_replace_table[static_cast<unsigned char>(*pos)];
		++pos;
	}

	log("%s", tmpbuf);
	mudlog(tmpbuf, type ? type : NRM, LVL_BUILDER, ERRLOG, TRUE);
}

/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void trig_log(TRIG_DATA * trig, const char *msg, const int type)
{
	char tmpbuf[MAX_STRING_LENGTH];
	snprintf(tmpbuf, MAX_STRING_LENGTH, "(Trigger: %s, VNum: %d) : %s", GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), msg);
	script_log(tmpbuf, type);
}

cmdlist_element::shared_ptr find_end(TRIG_DATA * trig, cmdlist_element::shared_ptr cl);
cmdlist_element::shared_ptr find_done(TRIG_DATA * trig, cmdlist_element::shared_ptr cl);
cmdlist_element::shared_ptr find_case(TRIG_DATA * trig, cmdlist_element::shared_ptr cl, void *go, SCRIPT_DATA * sc, int type, char *cond);
cmdlist_element::shared_ptr find_else_end(TRIG_DATA * trig, cmdlist_element::shared_ptr cl, void *go, SCRIPT_DATA * sc, int type);

struct trig_var_data *worlds_vars;

// Для отслеживания работы команд "wat" и "mat"
int reloc_target = -1;
TRIG_DATA *cur_trig = NULL;

GlobalTriggersStorage::~GlobalTriggersStorage()
{
	// notify all observers that of each trigger that they are going to be removed
	for (const auto& i : m_observers)
	{
		for (const auto& observer : i.second)
		{
			observer->notify(i.first);
		}
	}
}

void GlobalTriggersStorage::add(TRIG_DATA* trigger)
{
	m_triggers.insert(trigger);
	m_rnum2trigers_set[trigger->get_rnum()].insert(trigger);
}

void GlobalTriggersStorage::remove(TRIG_DATA* trigger)
{
	// notify all observers that this trigger is going to be removed.
	const auto observers_list_i = m_observers.find(trigger);
	if (observers_list_i != m_observers.end())
	{
		for (const auto& observer : m_observers[trigger])
		{
			observer->notify(trigger);
		}

		m_observers.erase(observers_list_i);
	}

	// then erase trigger
	m_triggers.erase(trigger);
	m_rnum2trigers_set[trigger->get_rnum()].erase(trigger);
}

void GlobalTriggersStorage::shift_rnums_from(const rnum_t rnum)
{
	// TODO: Get rid of this function when index will not has to be sorted by rnums
	//       Actually we need to get rid of rnums at all.
	std::list<TRIG_DATA*> to_rebind;
	for (const auto trigger : m_triggers)
	{
		if (trigger->get_rnum() > rnum)
		{
			to_rebind.push_back(trigger);
		}
	}

	for (const auto trigger : to_rebind)
	{
		remove(trigger);
		trigger->set_rnum(1 + trigger->get_rnum());
		add(trigger);
	}
}

void GlobalTriggersStorage::register_remove_observer(TRIG_DATA* trigger, const TriggerEventObserver::shared_ptr& observer)
{
	const auto i = m_triggers.find(trigger);
	if (i != m_triggers.end())
	{
		m_observers[trigger].insert(observer);
	}
}

void GlobalTriggersStorage::unregister_remove_observer(TRIG_DATA* trigger, const TriggerEventObserver::shared_ptr& observer)
{
	const auto i = m_observers.find(trigger);
	if (i != m_observers.end())
	{
		i->second.erase(observer);

		if (i->second.empty())
		{
			m_observers.erase(i);
		}
	}
}

GlobalTriggersStorage& trigger_list = GlobalObjects::trigger_list();	// all attached triggers

int trgvar_in_room(int vnum)
{
	const int rnum = real_room(vnum);

	if (NOWHERE == rnum)
	{
		script_log("people.vnum: world[rnum] does not exist");
		return (-1);
	}

	int count = 0;
	for (const auto& ch : world[rnum]->people)
	{
		if (!GET_INVIS_LEV(ch))
		{
			++count;
		}
	}

	return count;
};

OBJ_DATA *get_obj_in_list(char *name, OBJ_DATA * list)
{
	OBJ_DATA *i;
	long id;

	if (*name == UID_OBJ)
	{
		id = atoi(name + 1);

		for (i = list; i; i = i->get_next_content())
		{
			if (id == i->get_id())
			{
				return i;
			}
		}
	}
	else
	{
		for (i = list; i; i = i->get_next_content())
		{
			if (isname(name, i->get_aliases()))
			{
				return i;
			}
		}
	}

	return NULL;
}

OBJ_DATA *get_object_in_equip(CHAR_DATA * ch, char *name)
{
	int j, n = 0, number;
	OBJ_DATA *obj;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	long id;

	if (*name == UID_OBJ)
	{
		id = atoi(name + 1);

		for (j = 0; j < NUM_WEARS; j++)
			if ((obj = GET_EQ(ch, j)))
				if (id == obj->get_id())
					return (obj);
	}
	else
	{
		strcpy(tmp, name);
		if (!(number = get_number(&tmp)))
			return NULL;

		for (j = 0; (j < NUM_WEARS) && (n <= number); j++)
		{
			obj = GET_EQ(ch, j);
			if (!obj)
			{
				continue;
			}

			if (isname(tmp, obj->get_aliases()))
			{
				++n;
				if (n == number)
				{
					return (obj);
				}
			}
		}
	}

	return NULL;
}

// * return number of object in world
const char * get_objs_in_world(OBJ_DATA * obj)
{
	int i;
	static char retval[16];
	if (!obj)
		return "";
	if ((i = GET_OBJ_RNUM(obj)) < 0)
	{
		log("DG_SCRIPTS : attemp count unknown object");
		return "";
	}
	sprintf(retval, "%d", obj_proto.actual_count(i));
	return retval;
}

// * return number of obj|mob in world by vnum
int gcount_char_vnum(long n)
{
	int count = 0;

	for (const auto ch : character_list)
	{
		if (n == GET_MOB_VNUM(ch))
		{
			count++;
		}
	}

	return (count);
}

int count_char_vnum(long n)
{
	int i;
	if ((i = real_mobile(n)) < 0)
		return 0;
	return (mob_index[i].number);
}

inline auto gcount_obj_vnum(long n)
{
	const auto i = real_object(n);

	if (i < 0)
	{
		return 0;
	}

	return obj_proto.number(i);
}

inline auto count_obj_vnum(long n)
{
	const auto i = real_object(n);

	if (i < 0)
	{
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
OBJ_DATA *find_obj_by_id(const object_id_t id)
{
	const auto result = world_objects.find_first_by_id(id);
	return result.get();
}

// return room with UID n
ROOM_DATA *find_room(long n)
{
	n = real_room(n - ROOM_ID_BASE);

	if ((n >= FIRST_ROOM) && (n <= top_of_world))
		return world[n];

	return NULL;
}

/************************************************************
 * search by VNUM routines
 ************************************************************/

 /**
 * Возвращает id моба указанного внума.
 * \param num - если есть и больше 0 - возвращает id не первого моба, а указанного порядкового номера.
 */
int find_char_vnum(long n, int num = 0)
{
	int count = 0;
	for (const auto ch : character_list)
	{
		if (n == GET_MOB_VNUM(ch) && ch->in_room != NOWHERE)
		{
			if (num != count)
			{
				++count;
				continue;
			}
			else
			{
				return (GET_ID(ch));
			}
		}
	}
	return -1;
}

// return room with VNUM n
// Внимание! Для комнаты UID = ROOM_ID_BASE+VNUM, т.к.
// RNUM может быть независимо изменен с помощью OLC
int find_room_vnum(long n)
{
	//  return (real_room (n) != NOWHERE) ? ROOM_ID_BASE + n : -1;
	return (real_room(n) != NOWHERE) ? n : -1;
}

int find_room_uid(long n)
{
	return (real_room(n) != NOWHERE) ? ROOM_ID_BASE + n : -1;
	//return (real_room (n) != NOWHERE) ? n : -1;
}

/************************************************************
 * generic searches based only on name
 ************************************************************/

 // search the entire world for a char, and return a pointer
CHAR_DATA *get_char(char *name, int/* vnum*/)
{
	CHAR_DATA *i;

	// Отсекаем поиск левых UID-ов.
	if ((*name == UID_OBJ) || (*name == UID_ROOM))
		return NULL;

	if (*name == UID_CHAR)
	{
		i = find_char(atoi(name + 1));

		if (i && (IS_NPC(i) || !GET_INVIS_LEV(i)))
		{
			return i;
		}
	}
	else
	{
		for (const auto& character : character_list)
		{
			const auto i = character.get();
			if (isname(name, i->get_pc_name())
				&& (IS_NPC(i)
					|| !GET_INVIS_LEV(i)))
			{
				return i;
			}
		}
	}

	return NULL;
}

// returns the object in the world with name name, or NULL if not found
OBJ_DATA *get_obj(char *name, int/* vnum*/)
{
	long id;

	if ((*name == UID_CHAR) || (*name == UID_ROOM))
		return NULL;

	if (*name == UID_OBJ)
	{
		id = atoi(name + 1);
		return find_obj_by_id(id);
	}
	else
	{
		const auto result = world_objects.find_by_name(name);
		return result.get();
	}
	return NULL;
}

// finds room by with name.  returns NULL if not found
ROOM_DATA *get_room(char *name)
{
	int nr;

	if ((*name == UID_CHAR) || (*name == UID_OBJ))
		return NULL;

	if (*name == UID_ROOM)
		return find_room(atoi(name + 1));
	else if ((nr = real_room(atoi(name))) == NOWHERE)
		return NULL;
	else
		return world[nr];
}


/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching with the person owing the object
 */
CHAR_DATA *get_char_by_obj(OBJ_DATA * obj, char *name)
{
	CHAR_DATA *ch;

	if ((*name == UID_ROOM) || (*name == UID_OBJ))
		return NULL;

	if (*name == UID_CHAR)
	{
		ch = find_char(atoi(name + 1));

		if (ch && (IS_NPC(ch) || !GET_INVIS_LEV(ch)))
			return ch;
	}
	else
	{
		if (obj->get_carried_by()
			&& isname(name, obj->get_carried_by()->get_pc_name())
			&& (IS_NPC(obj->get_carried_by())
				|| !GET_INVIS_LEV(obj->get_carried_by())))
		{
			return obj->get_carried_by();
		}

		if (obj->get_worn_by()
			&& isname(name, obj->get_worn_by()->get_pc_name())
			&& (IS_NPC(obj->get_worn_by())
				|| !GET_INVIS_LEV(obj->get_worn_by())))
		{
			return obj->get_worn_by();
		}

		for (const auto ch : character_list)
		{
			if (isname(name, ch->get_pc_name())
				&& (IS_NPC(ch)
					|| !GET_INVIS_LEV(ch)))
			{
				return ch.get();
			}
		}
	}

	return NULL;
}


/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
CHAR_DATA *get_char_by_room(ROOM_DATA * room, char *name)
{
	CHAR_DATA *ch;

	if (*name == UID_ROOM
		|| *name == UID_OBJ)
	{
		return NULL;
	}

	if (*name == UID_CHAR)
	{
		ch = find_char(atoi(name + 1));

		if (ch
			&& (IS_NPC(ch)
				|| !GET_INVIS_LEV(ch)))
		{
			return ch;
		}
	}
	else
	{
		for (const auto ch : room->people)
		{
			if (isname(name, ch->get_pc_name())
				&& (IS_NPC(ch)
					|| !GET_INVIS_LEV(ch)))
			{
				return ch;
			}
		}

		for (const auto ch : character_list)
		{
			if (isname(name, ch->get_pc_name())
				&& (IS_NPC(ch)
					|| !GET_INVIS_LEV(ch)))
			{
				return ch.get();
			}
		}
	}

	return NULL;
}


/*
 * returns the object in the world with name name, or NULL if not found
 * search based on obj
 */
OBJ_DATA *get_obj_by_obj(OBJ_DATA * obj, char *name)
{
	OBJ_DATA *i = NULL;
	int rm;
	long id;

	if ((*name == UID_ROOM) || (*name == UID_CHAR))
		return NULL;

	if (!str_cmp(name, "self") || !str_cmp(name, "me"))
		return obj;

	if (obj->get_contains() && (i = get_obj_in_list(name, obj->get_contains())))
		return i;

	if (obj->get_in_obj())
	{
		if (*name == UID_OBJ)
		{
			id = atoi(name + 1);

			if (id == obj->get_in_obj()->get_id())
			{
				return obj->get_in_obj();
			}
		}
		else if (isname(name, obj->get_in_obj()->get_aliases()))
		{
			return obj->get_in_obj();
		}
	}
	else if (obj->get_worn_by() && (i = get_object_in_equip(obj->get_worn_by(), name)))
	{
		return i;
	}
	else if (obj->get_carried_by() && (i = get_obj_in_list(name, obj->get_carried_by()->carrying)))
	{
		return i;
	}
	else if (((rm = obj_room(obj)) != NOWHERE) && (i = get_obj_in_list(name, world[rm]->contents)))
	{
		return i;
	}

	if (*name == UID_OBJ)
	{
		id = atoi(name + 1);

		i = world_objects.find_first_by_id(id).get();
	}
	else
	{
		i = world_objects.find_by_name(name).get();
	}

	return i;
}


// returns obj with name
OBJ_DATA *get_obj_by_room(ROOM_DATA * room, char *name)
{
	OBJ_DATA *obj;
	long id;

	if ((*name == UID_ROOM) || (*name == UID_CHAR))
		return NULL;

	if (*name == UID_OBJ)
	{
		id = atoi(name + 1);

		for (obj = room->contents; obj; obj = obj->get_next_content())
		{
			if (id == obj->get_id())
			{
				return obj;
			}
		}

		return world_objects.find_first_by_id(id).get();
	}
	else
	{
		for (obj = room->contents; obj; obj = obj->get_next_content())
		{
			if (isname(name, obj->get_aliases()))
			{
				return obj;
			}
		}

		return world_objects.find_by_name(name).get();
	}

	return NULL;
}

// returns obj with name
OBJ_DATA *get_obj_by_char(CHAR_DATA * ch, char *name)
{
	OBJ_DATA *obj;
	long id;

	if ((*name == UID_ROOM) || (*name == UID_CHAR))
		return NULL;

	if (*name == UID_OBJ)
	{
		id = atoi(name + 1);
		if (ch)
		{
			for (obj = ch->carrying; obj; obj = obj->get_next_content())
			{
				if (id == obj->get_id())
				{
					return obj;
				}
			}
		}

		return world_objects.find_first_by_id(id).get();
	}
	else
	{
		if (ch)
		{
			for (obj = ch->carrying; obj; obj = obj->get_next_content())
			{
				if (isname(name, obj->get_aliases()))
				{
					return obj;
				}
			}
		}

		return world_objects.find_by_name(name).get();
	}

	return NULL;
}

// checks every PLUSE_SCRIPT for random triggers
void script_trigger_check()
{
	character_list.foreach_on_copy([](const CHAR_DATA::shared_ptr& ch)
	{
		if (SCRIPT(ch)->has_triggers())
		{
			auto sc = SCRIPT(ch).get();

			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_RANDOM)
				&& (!is_empty(world[ch->in_room]->zone)
					|| IS_SET(SCRIPT_TYPES(sc), MTRIG_GLOBAL)))
			{
				random_mtrigger(ch.get());
			}
		}
	});

	world_objects.foreach_on_copy([&](const OBJ_DATA::shared_ptr& obj)
	{
		if (OBJ_FLAGGED(obj.get(), EExtraFlag::ITEM_NAMED))
		{
			if (obj->get_worn_by() && number(1, 100) <= 5)
			{
				NamedStuff::wear_msg(obj->get_worn_by(), obj.get());
			}
		}
		else if (obj->get_script()->has_triggers())
		{
			auto sc = obj->get_script().get();
			if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
			{
				random_otrigger(obj.get());
			}
		}
	});

	for (std::size_t nr = FIRST_ROOM; nr <= static_cast<std::size_t>(top_of_world); nr++)
	{
		if (SCRIPT(world[nr])->has_triggers())
		{
			auto room = world[nr];
			auto sc = SCRIPT(room).get();

			if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM)
				&& (!is_empty(room->zone)
					|| IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
			{
				// Если будет крэш (а он несомненно будет) можно будет посмотреть параметры в стеке
				random_wtrigger(room, room->number, sc, sc->types, sc->trig_list);
			}
		}
	}
}

// проверка каждый час на триги изменении времени
void script_timechange_trigger_check(const int time)
{
	character_list.foreach_on_copy([&](const std::shared_ptr<CHAR_DATA>& ch)
	{
		if (SCRIPT(ch)->has_triggers())
		{
			auto sc = SCRIPT(ch).get();

			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_TIMECHANGE)
				&& (!is_empty(world[ch->in_room]->zone)
					|| IS_SET(SCRIPT_TYPES(sc), MTRIG_GLOBAL)))
			{
				timechange_mtrigger(ch.get(), time);
			}
		}
	});

	world_objects.foreach_on_copy([&](const OBJ_DATA::shared_ptr& obj)
	{
		if (obj->get_script()->has_triggers())
		{
			auto sc = obj->get_script().get();
			if (IS_SET(SCRIPT_TYPES(sc), OTRIG_TIMECHANGE))
			{
				timechange_otrigger(obj.get(), time);
			}
		}
	});

	for (std::size_t nr = FIRST_ROOM; nr <= static_cast<std::size_t>(top_of_world); nr++)
	{
		if (SCRIPT(world[nr])->has_triggers())
		{
			auto room = world[nr];
			auto sc = SCRIPT(room).get();

			if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIMECHANGE)
				&& (!is_empty(room->zone)
					|| IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
			{
				timechange_wtrigger(room, time);
			}
		}
	}
}

EVENT(trig_wait_event)
{
	struct wait_event_data *wait_event_obj = (struct wait_event_data *) info;
	TRIG_DATA *trig;
	void *go;
	int type;

	trig = wait_event_obj->trigger;
	go = wait_event_obj->go;
	type = wait_event_obj->type;

	GET_TRIG_WAIT(trig) = NULL;

	script_driver(go, trig, type, TRIG_RESTART);
	free(wait_event_obj);
}


void do_stat_trigger(CHAR_DATA * ch, TRIG_DATA * trig)
{
	char sb[MAX_EXTEND_LENGTH];

	if (!trig)
	{
		log("SYSERR: NULL trigger passed to do_stat_trigger.");
		return;
	}

	sprintf(sb, "Name: '%s%s%s',  VNum: [%s%5d%s], RNum: [%5d]\r\n",
		CCYEL(ch, C_NRM), GET_TRIG_NAME(trig), CCNRM(ch, C_NRM),
		CCGRN(ch, C_NRM), GET_TRIG_VNUM(trig), CCNRM(ch, C_NRM), GET_TRIG_RNUM(trig));
	send_to_char(sb, ch);

	if (trig->get_attach_type() == MOB_TRIGGER)
	{
		send_to_char("Trigger Intended Assignment: Mobiles\r\n", ch);
		sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
	}
	else if (trig->get_attach_type() == OBJ_TRIGGER)
	{
		send_to_char("Trigger Intended Assignment: Objects\r\n", ch);
		sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf);
	}
	else if (trig->get_attach_type() == WLD_TRIGGER)
	{
		send_to_char("Trigger Intended Assignment: Rooms\r\n", ch);
		sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf);
	}
	else
	{
		send_to_char(ch, "Trigger Intended Assignment: undefined (attach_type=%d)\r\n",
			static_cast<int>(trig->get_attach_type()));
	}

	sprintf(sb, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
		buf, GET_TRIG_NARG(trig), !trig->arglist.empty() ? trig->arglist.c_str() : "None");

	strcat(sb, "Commands:\r\n   ");

	auto cmd_list = *trig->cmdlist;
	while (cmd_list)
	{
		if (!cmd_list->cmd.empty())
		{
			strcat(sb, cmd_list->cmd.c_str());
			strcat(sb, "\r\n   ");
		}

		cmd_list = cmd_list->next;
	}

	page_string(ch->desc, sb, 1);
}

// find the name of what the uid points to
void find_uid_name(char *uid, char *name)
{
	CHAR_DATA *ch;
	OBJ_DATA *obj;

	if ((ch = get_char(uid)))
	{
		strcpy(name, ch->get_pc_name().c_str());
	}
	else if ((obj = get_obj(uid)))
	{
		strcpy(name, obj->get_aliases().c_str());
	}
	else
	{
		sprintf(name, "uid = %s, (not found)", uid + 1);
	}
}


// general function to display stats on script sc
void script_stat(CHAR_DATA * ch, SCRIPT_DATA * sc)
{
	struct trig_var_data *tv;
	char name[MAX_INPUT_LENGTH];
	char namebuf[512];

	sprintf(buf, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
	send_to_char(buf, ch);
	sprintf(buf, "Global context: %ld\r\n", sc->context);
	send_to_char(buf, ch);

	for (tv = sc->global_vars; tv; tv = tv->next)
	{
		sprintf(namebuf, "%s:%ld", tv->name, tv->context);
		if (*(tv->value) == UID_CHAR || *(tv->value) == UID_ROOM || *(tv->value) == UID_OBJ)
		{
			find_uid_name(tv->value, name);
			sprintf(buf, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name, name);
		}
		else
			sprintf(buf, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name, tv->value);
		send_to_char(buf, ch);
	}

	for (auto t : sc->trig_list)
	{
		sprintf(buf, "\r\n  Trigger: %s%s%s, VNum: [%s%5d%s], RNum: [%5d]\r\n",
			CCYEL(ch, C_NRM), GET_TRIG_NAME(t), CCNRM(ch, C_NRM),
			CCGRN(ch, C_NRM), GET_TRIG_VNUM(t), CCNRM(ch, C_NRM), GET_TRIG_RNUM(t));
		send_to_char(buf, ch);

		if (t->get_attach_type() == MOB_TRIGGER)
		{
			send_to_char("  Trigger Intended Assignment: Mobiles\r\n", ch);
			sprintbit(GET_TRIG_TYPE(t), trig_types, buf1);
		}
		else if (t->get_attach_type() == OBJ_TRIGGER)
		{
			send_to_char("  Trigger Intended Assignment: Objects\r\n", ch);
			sprintbit(GET_TRIG_TYPE(t), otrig_types, buf1);
		}
		else if (t->get_attach_type() == WLD_TRIGGER)
		{
			send_to_char("  Trigger Intended Assignment: Rooms\r\n", ch);
			sprintbit(GET_TRIG_TYPE(t), wtrig_types, buf1);
		}
		else
		{
			send_to_char(ch, "Trigger Intended Assignment: undefined (attach_type=%d)\r\n",
				static_cast<int>(t->get_attach_type()));
		}

		sprintf(buf, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
			buf1, GET_TRIG_NARG(t), !t->arglist.empty() ? t->arglist.c_str() : "None");
		send_to_char(buf, ch);

		if (GET_TRIG_WAIT(t))
		{
			if (t->curr_state != NULL)
			{
				sprintf(buf, "    Wait: %d, Current line: %s\r\n",
					GET_TRIG_WAIT(t)->time_remaining, t->curr_state->cmd.c_str());
				send_to_char(buf, ch);
			}
			else
			{
				sprintf(buf, "    Wait: %d\r\n", GET_TRIG_WAIT(t)->time_remaining);
				send_to_char(buf, ch);
			}

			sprintf(buf, "  Variables: %s\r\n", GET_TRIG_VARS(t) ? "" : "None");
			send_to_char(buf, ch);

			for (tv = GET_TRIG_VARS(t); tv; tv = tv->next)
			{
				if (*(tv->value) == UID_CHAR || *(tv->value) == UID_ROOM || *(tv->value) == UID_OBJ)
				{
					find_uid_name(tv->value, name);
					sprintf(buf, "    %15s:  %s\r\n", tv->name, name);
				}
				else
				{
					sprintf(buf, "    %15s:  %s\r\n", tv->name, tv->value);
				}
				send_to_char(buf, ch);
			}
		}
	}
}

void do_sstat_room(CHAR_DATA * ch)
{
	ROOM_DATA *rm = world[ch->in_room];

	do_sstat_room(rm, ch);

}

void do_sstat_room(ROOM_DATA *rm, CHAR_DATA * ch)
{
	send_to_char("Script information:\r\n", ch);
	if (!SCRIPT(rm)->has_triggers())
	{
		send_to_char("  None.\r\n", ch);
		return;
	}

	script_stat(ch, SCRIPT(rm).get());
}

void do_sstat_object(CHAR_DATA * ch, OBJ_DATA * j)
{
	send_to_char("Script information:\r\n", ch);
	if (!j->get_script()->has_triggers())
	{
		send_to_char("  None.\r\n", ch);
		return;
	}

	script_stat(ch, j->get_script().get());
}

void do_sstat_character(CHAR_DATA * ch, CHAR_DATA * k)
{
	send_to_char("Script information:\r\n", ch);
	if (!SCRIPT(k)->has_triggers())
	{
		send_to_char("  None.\r\n", ch);
		return;
	}

	script_stat(ch, SCRIPT(k).get());
}

/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 *
 * Return true if trigger successfully added
 * Return false if trigger with same rnum already exists and can not be added
 */
bool add_trigger(SCRIPT_DATA* sc, TRIG_DATA * t, int loc)
{
	if (!t || !sc)
	{
		return false;
	}

	const bool added = sc->trig_list.add(t, 0 == loc);
	if (added)
	{
		SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);
		trigger_list.add(t);
	}

	return added;
}

void do_attach(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	OBJ_DATA *object;
	TRIG_DATA *trig;
	char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
	char loc_name[MAX_INPUT_LENGTH];
	int loc, room, tn, rn;

	argument = two_arguments(argument, arg, trig_name);
	two_arguments(argument, targ_name, loc_name);

	if (!*arg || !*targ_name || !*trig_name)
	{
		send_to_char("Usage: attach { mtr | otr | wtr } { trigger } { name } [ location ]\r\n", ch);
		return;
	}

	tn = atoi(trig_name);
	loc = (*loc_name) ? atoi(loc_name) : -1;

	rn = real_trigger(tn);
	if (rn >= 0
		&& ((is_abbrev(arg, "mtr") && trig_index[rn]->proto->get_attach_type() != MOB_TRIGGER)
			|| (is_abbrev(arg, "otr") && trig_index[rn]->proto->get_attach_type() != OBJ_TRIGGER)
			|| (is_abbrev(arg, "wtr") && trig_index[rn]->proto->get_attach_type() != WLD_TRIGGER)))
	{
		tn = (is_abbrev(arg, "mtr") ? 0 : is_abbrev(arg, "otr") ? 1 : is_abbrev(arg, "wtr") ? 2 : 3);
		sprintf(buf, "Trigger %d (%s) has wrong attach_type %s expected %s.\r\n",
			tn, GET_TRIG_NAME(trig_index[rn]->proto), attach_name[(int)trig_index[rn]->proto->get_attach_type()], attach_name[tn]);
		send_to_char(buf, ch);
		return;
	}
	if (is_abbrev(arg, "mtr"))
	{
		if ((victim = get_char_vis(ch, targ_name, FIND_CHAR_WORLD)))
		{
			if (IS_NPC(victim))  	// have a valid mob, now get trigger
			{
				rn = real_trigger(tn);
				if ((rn >= 0) && (trig = read_trigger(rn)))
				{
					sprintf(buf, "Trigger %d (%s) attached to %s.\r\n", tn, GET_TRIG_NAME(trig), GET_SHORT(victim));
					send_to_char(buf, ch);

					if (!add_trigger(SCRIPT(victim).get(), trig, loc))
					{
						extract_trigger(trig);
					}
				}
				else
				{
					send_to_char("That trigger does not exist.\r\n", ch);
				}
			}
			else
				send_to_char("Players can't have scripts.\r\n", ch);
		}
		else
		{
			send_to_char("That mob does not exist.\r\n", ch);
		}
	}
	else if (is_abbrev(arg, "otr"))
	{
		if ((object = get_obj_vis(ch, targ_name)))  	// have a valid obj, now get trigger
		{
			rn = real_trigger(tn);
			if ((rn >= 0) && (trig = read_trigger(rn)))
			{
				sprintf(buf, "Trigger %d (%s) attached to %s.\r\n",
					tn, GET_TRIG_NAME(trig),
					(!object->get_short_description().empty() ? object->get_short_description().c_str() : object->get_aliases().c_str()));
				send_to_char(buf, ch);

				if (!add_trigger(object->get_script().get(), trig, loc))
				{
					extract_trigger(trig);
				}
			}
			else
				send_to_char("That trigger does not exist.\r\n", ch);
		}
		else
			send_to_char("That object does not exist.\r\n", ch);
	}
	else if (is_abbrev(arg, "wtr"))
	{
		if (a_isdigit(*targ_name) && !strchr(targ_name, '.'))
		{
			if ((room = find_target_room(ch, targ_name, 0)) != NOWHERE)  	// have a valid room, now get trigger
			{
				rn = real_trigger(tn);
				if ((rn >= 0) && (trig = read_trigger(rn)))
				{
					sprintf(buf, "Trigger %d (%s) attached to room %d.\r\n",
						tn, GET_TRIG_NAME(trig), world[room]->number);
					send_to_char(buf, ch);

					if (!add_trigger(world[room]->script.get(), trig, loc))
					{
						extract_trigger(trig);
					}
				}
				else
				{
					send_to_char("That trigger does not exist.\r\n", ch);
				}
			}
		}
		else
		{
			send_to_char("You need to supply a room number.\r\n", ch);
		}
	}
	else
	{
		send_to_char("Please specify 'mtr', otr', or 'wtr'.\r\n", ch);
	}
}

void do_detach(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim = NULL;
	OBJ_DATA *object = NULL;
	ROOM_DATA *room;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
	char *trigger = 0;
	int tmp;

	argument = two_arguments(argument, arg1, arg2);
	one_argument(argument, arg3);

	if (!*arg1 || !*arg2)
	{
		send_to_char("Usage: detach [ mob | object | room ] { target } { trigger |" " 'all' }\r\n", ch);
		return;
	}

	if (!str_cmp(arg1, "room"))
	{
		room = world[ch->in_room];
		if (!SCRIPT(room)->has_triggers())
		{
			send_to_char("This room does not have any triggers.\r\n", ch);
		}
		else if (!str_cmp(arg2, "all") || !str_cmp(arg2, "все"))
		{
			room->cleanup_script();

			send_to_char("All triggers removed from room.\r\n", ch);
		}
		else if (SCRIPT(room)->remove_trigger(arg2))
		{
			send_to_char("Trigger removed.\r\n", ch);
		}
		else
		{
			send_to_char("That trigger was not found.\r\n", ch);
		}
	}
	else
	{
		if (is_abbrev(arg1, "mob"))
		{
			if (!(victim = get_char_vis(ch, arg2, FIND_CHAR_WORLD)))
				send_to_char("No such mobile around.\r\n", ch);
			else if (!*arg3)
				send_to_char("You must specify a trigger to remove.\r\n", ch);
			else
				trigger = arg3;
		}
		else if (is_abbrev(arg1, "object"))
		{
			if (!(object = get_obj_vis(ch, arg2)))
				send_to_char("No such object around.\r\n", ch);
			else if (!*arg3)
				send_to_char("You must specify a trigger to remove.\r\n", ch);
			else
				trigger = arg3;
		}
		else
		{
			if ((object = get_object_in_equip_vis(ch, arg1, ch->equipment, &tmp)));
			else if ((object = get_obj_in_list_vis(ch, arg1, ch->carrying)));
			else if ((victim = get_char_room_vis(ch, arg1)));
			else if ((object = get_obj_in_list_vis(ch, arg1, world[ch->in_room]->contents)));
			else if ((victim = get_char_vis(ch, arg1, FIND_CHAR_WORLD)));
			else if ((object = get_obj_vis(ch, arg1)));
			else
				send_to_char("Nothing around by that name.\r\n", ch);
			trigger = arg2;
		}

		if (victim)
		{
			if (!IS_NPC(victim))
			{
				send_to_char("Players don't have triggers.\r\n", ch);
			}
			else if (!SCRIPT(victim)->has_triggers())
			{
				send_to_char("That mob doesn't have any triggers.\r\n", ch);
			}
			else if (!str_cmp(arg2, "all") || !str_cmp(arg2, "все"))
			{
				victim->cleanup_script();
				sprintf(buf, "All triggers removed from %s.\r\n", GET_SHORT(victim));
				send_to_char(buf, ch);
			}
			else if (trigger
				&& SCRIPT(victim)->remove_trigger(trigger))
			{
				send_to_char("Trigger removed.\r\n", ch);
			}
			else
			{
				send_to_char("That trigger was not found.\r\n", ch);
			}
		}
		else if (object)
		{
			if (!object->get_script()->has_triggers())
			{
				send_to_char("That object doesn't have any triggers.\r\n", ch);
			}
			else if (!str_cmp(arg2, "all") || !str_cmp(arg2, "все"))
			{
				object->cleanup_script();
				sprintf(buf, "All triggers removed from %s.\r\n",
					!object->get_short_description().empty() ? object->get_short_description().c_str() : object->get_aliases().c_str());
				send_to_char(buf, ch);
			}
			else if (object->get_script()->remove_trigger(trigger))
			{
				send_to_char("Trigger removed.\r\n", ch);
			}
			else
			{
				send_to_char("That trigger was not found.\r\n", ch);
			}
		}
	}
}

// frees memory associated with var
void free_var_el(struct trig_var_data *var)
{
	free(var->name);
	free(var->value);
	free(var);
}


void add_var_cntx(struct trig_var_data **var_list, const char *name, const char *value, long id)
/*++
	Добавление переменной в список с учетом контекста (СТРОГИЙ поиск).
	При добавлении в список локальных переменных контекст должен быть 0.

	var_list	- указатель на первый элемент списка переменных
	name		- имя переменной
	value		- значение переменной
	id			- контекст переменной
--*/
{
	struct trig_var_data *vd, *vd_prev;

	// Обращаю внимание, что при добавлении необходимо ТОЧНОЕ совпадение контекстов
	for (vd_prev = NULL, vd = *var_list;
		vd && ((vd->context != id) || str_cmp(vd->name, name)); vd_prev = vd, vd = vd->next);

	if (vd)
	{
		// Переменная существует, заменить значение
		free(vd->value);
	}
	else
	{
		// Создать новую переменную
		CREATE(vd, 1);
		CREATE(vd->name, strlen(name) + 1);
		strcpy(vd->name, name);

		vd->context = id;

		// Добавление переменной в список
		// Для нулевого контекста в конец списка, для ненулевого - в начало
		if (id)
		{
			vd->next = *var_list;
			*var_list = vd;
		}
		else
		{
			vd->next = NULL;
			if (vd_prev)
				vd_prev->next = vd;
			else
				*var_list = vd;
		}
	}

	CREATE(vd->value, strlen(value) + 1);
	strcpy(vd->value, value);
}


struct trig_var_data *find_var_cntx(struct trig_var_data **var_list, char *name, long id)
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
	struct trig_var_data *vd;

	for (vd = *var_list; vd && ((vd->context && vd->context != id) || str_cmp(vd->name, name)); vd = vd->next);

	return vd;
}


int remove_var_cntx(struct trig_var_data **var_list, char *name, long id)
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
	struct trig_var_data *vd, *vd_prev;

	for (vd_prev = NULL, vd = *var_list;
		vd && ((vd->context != id) || str_cmp(vd->name, name)); vd_prev = vd, vd = vd->next);

	if (!vd)
		return 0;	// Не удалось найти

	if (vd_prev)
		vd_prev->next = vd->next;
	else
		*var_list = vd->next;

	free_var_el(vd);

	return 1;
}

bool SCRIPT_CHECK(const OBJ_DATA* go, const long type)
{
	return !go->get_script()->is_purged() && go->get_script()->has_triggers() && IS_SET(SCRIPT_TYPES(go->get_script()), type);
}

bool SCRIPT_CHECK(const CHAR_DATA* go, const long type)
{
	return !SCRIPT(go)->is_purged() && SCRIPT(go)->has_triggers() && IS_SET(SCRIPT_TYPES(go->script), type);
}

bool SCRIPT_CHECK(const ROOM_DATA* go, const long type)
{
	return !SCRIPT(go)->is_purged() && SCRIPT(go)->has_triggers() && IS_SET(SCRIPT_TYPES(go->script), type);
}

// * Изменение указанной целочисленной константы
long gm_char_field(CHAR_DATA * ch, char *field, char *subfield, long val)
{
	int tmpval;
	if (*subfield)
	{
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

int text_processed(char *field, char *subfield, struct trig_var_data *vd, char *str)
{
	char *p, *p2;
	*str = '\0';
	if (!vd || !vd->name || !vd->value)
		return FALSE;

	if (!str_cmp(field, "strlen"))  	// strlen
	{
		sprintf(str, "%lu", static_cast<unsigned long>(strlen(vd->value)));
		return TRUE;
	}
	else if (!str_cmp(field, "trim"))  	// trim
	{
		// trim whitespace from ends
		p = vd->value;
		p2 = vd->value + strlen(vd->value) - 1;
		while (*p && a_isspace(*p))
			p++;
		while ((p >= p2) && a_isspace(*p2))
			p2--;
		if (p > p2)  	// nothing left
		{
			*str = '\0';
			return TRUE;
		}
		while (p <= p2)
			*str++ = *p++;
		*str = '\0';
		return TRUE;
	}
	else if (!str_cmp(field, "contains"))  	// contains
	{
		if (str_str(vd->value, subfield))
			sprintf(str, "1");
		else
			sprintf(str, "0");
		return TRUE;
	}
	else if (!str_cmp(field, "car"))  	// car
	{
		char *car = vd->value;
		while (*car && !a_isspace(*car))
			*str++ = *car++;
		*str = '\0';
		return TRUE;
	}
	else if (!str_cmp(field, "cdr"))  	// cdr
	{
		char *cdr = vd->value;
		while (*cdr && !a_isspace(*cdr))
			cdr++;	// skip 1st field
		while (*cdr && a_isspace(*cdr))
			cdr++;	// skip to next
		while (*cdr)
			*str++ = *cdr++;
		*str = '\0';
		return TRUE;
	}
	else if (!str_cmp(field, "words"))
	{
		int n = 0;
		// Подсчет количества слов или получение слова
		char buf1[MAX_INPUT_LENGTH];
		char buf2[MAX_INPUT_LENGTH];
		buf1[0] = 0;
		strcpy(buf2, vd->value);
		if (*subfield)
		{
			for (n = atoi(subfield); n; --n)
			{
				half_chop(buf2, buf1, buf2);
			}
			strcpy(str, buf1);
		}
		else
		{
			while (buf2[0] != 0)
			{
				half_chop(buf2, buf1, buf2);
				++n;
			}
			sprintf(str, "%d", n);
		}
		return TRUE;
	}
	else if (!str_cmp(field, "mudcommand"))  	// find the mud command returned from this text
	{
		// NOTE: you may need to replace "cmd_info" with "complete_cmd_info",
		// depending on what patches you've got applied.
		extern const struct command_info cmd_info[];
		// on older source bases:    extern struct command_info *cmd_info;
		int cmd = 0;
		const size_t length = strlen(vd->value);
		while (*cmd_info[cmd].command != '\n')
		{
			if (!strncmp(cmd_info[cmd].command, vd->value, length))
			{
				break;
			}
			cmd++;
		}

		if (*cmd_info[cmd].command == '\n')
			strcpy(str, "");
		else
			strcpy(str, cmd_info[cmd].command);
		return TRUE;
	}

	return FALSE;
}

//WorM: добавил для работы can_get_spell
//extern int slot_for_char(CHAR_DATA * ch, int slot_num);
//#define SpINFO spell_info[num]
// sets str to be the value of var.field
void find_replacement(void* go, SCRIPT_DATA* sc, TRIG_DATA* trig, int type, char* var, char* field, char* subfield, char* str)
{
	struct trig_var_data *vd = NULL;
	CHAR_DATA *ch, *c = NULL, *rndm;
	OBJ_DATA *obj, *o = NULL;
	ROOM_DATA *room, *r = NULL;
	char *name;
	int num = 0, count = 0, i;
	char uid_type = '\0';
	char tmp[MAX_INPUT_LENGTH] = {};

	const char *send_cmd[] = { "msend", "osend", "wsend" };
	const char *echo_cmd[] = { "mecho", "oecho", "wecho" };
	const char *echoaround_cmd[] = { "mechoaround", "oechoaround", "wechoaround" };
	const char *door[] = { "mdoor", "odoor", "wdoor" };
	const char *force[] = { "mforce", "oforce", "wforce" };
	const char *load[] = { "mload", "oload", "wload" };
	const char *purge[] = { "mpurge", "opurge", "wpurge" };
	const char *teleport[] = { "mteleport", "oteleport", "wteleport" };
	const char *damage[] = { "mdamage", "odamage", "wdamage" };
	const char *featturn[] = { "mfeatturn", "ofeatturn", "wfeatturn" };
	const char *skillturn[] = { "mskillturn", "oskillturn", "wskillturn" };
	const char *skilladd[] = { "mskilladd", "oskilladd", "wskilladd" };
	const char *spellturn[] = { "mspellturn", "ospellturn", "wspellturn" };
	const char *spelladd[] = { "mspelladd", "ospelladd", "wspelladd" };
	const char *spellitem[] = { "mspellitem", "ospellitem", "wspellitem" };
	const char *portal[] = { "mportal", "oportal", "wportal" };

	if (!subfield)
	{
		subfield = NULL;	// Чтобы проверок меньше было
	}

	// X.global() will have a NULL trig
	if (trig)
		vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);
	if (!vd)
		vd = find_var_cntx(&(sc->global_vars), var, sc->context);
	if (!vd)
		vd = find_var_cntx(&worlds_vars, var, sc->context);

	*str = '\0';

	if (!field || !*field)
	{
		if (vd)
		{
			strcpy(str, vd->value);
		}
		else
		{
			if (!str_cmp(var, "self"))
			{
				long uid;
				// заменить на UID
				switch (type)
				{
				case MOB_TRIGGER:
					uid = GET_ID((CHAR_DATA *)go);
					uid_type = UID_CHAR;
					break;

				case OBJ_TRIGGER:
					uid = ((OBJ_DATA *)go)->get_id();
					uid_type = UID_OBJ;
					break;

				case WLD_TRIGGER:
					uid = find_room_uid(((ROOM_DATA *)go)->number);
					uid_type = UID_ROOM;
					break;

				default:
					strcpy(str, "self");
					return;
				}
				sprintf(str, "%c%ld", uid_type, uid);
			}
			else if (!str_cmp(var, "door"))
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
			else if (!str_cmp(var, "spelladd"))
				strcpy(str, spelladd[type]);
			else if (!str_cmp(var, "spellitem"))
				strcpy(str, spellitem[type]);
			else if (!str_cmp(var, "portal"))
				strcpy(str, portal[type]);
		}
		return;
	}

	if (vd)
	{
		name = vd->value;
		switch (type)
		{
		case MOB_TRIGGER:
			ch = (CHAR_DATA *)go;
			if (!name)
			{
				log("SYSERROR: null name (%s:%d %s)", __FILE__, __LINE__, __func__);
				break;
			}
			if (!ch)
			{
				log("SYSERROR: null ch (%s:%d %s)", __FILE__, __LINE__, __func__);
				break;
			}
			if ((o = get_object_in_equip(ch, name)));
			else if ((o = get_obj_in_list(name, ch->carrying)));
			else if ((c = get_char_room(name, ch->in_room)));
			else if ((o = get_obj_in_list(name, world[ch->in_room]->contents)));
			else if ((c = get_char(name, GET_TRIG_VNUM(trig))));
			else if ((o = get_obj(name, GET_TRIG_VNUM(trig))));
			else if ((r = get_room(name)))
			{
			}
			break;
		case OBJ_TRIGGER:
			obj = (OBJ_DATA *)go;
			if ((c = get_char_by_obj(obj, name)));
			else if ((o = get_obj_by_obj(obj, name)));
			else if ((r = get_room(name)))
			{
			}
			break;
		case WLD_TRIGGER:
			room = (ROOM_DATA *)go;
			if ((c = get_char_by_room(room, name)));
			else if ((o = get_obj_by_room(room, name)));
			else if ((r = get_room(name)))
			{
			}
			break;
		}
	}
	else
	{
		if (!str_cmp(var, "self"))
		{
			switch (type)
			{
			case MOB_TRIGGER:
				c = (CHAR_DATA *)go;
				break;
			case OBJ_TRIGGER:
				o = (OBJ_DATA *)go;
				break;
			case WLD_TRIGGER:
				r = (ROOM_DATA *)go;
				break;
			}
		}
		else if (!str_cmp(var, "exist"))
		{
			if (!str_cmp(field, "mob") && (num = atoi(subfield)) > 0)
			{
				num = count_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%c", num > 0 ? '1' : '0');
			}
			else if (!str_cmp(field, "obj") && (num = atoi(subfield)) > 0)
			{
				num = gcount_obj_vnum(num);
				if (num >= 0)
					sprintf(str, "%c", num > 0 ? '1' : '0');
			}
			return;
		}
		else if (!str_cmp(var, "world"))
		{
			num = atoi(subfield);
			if ((!str_cmp(field, "curobj") || !str_cmp(field, "curobjs")) && num > 0)
			{
				const auto rnum = real_object(num);
				const auto count = count_obj_vnum(num);
				if (count >= 0 && 0 <= rnum)
				{
					if (check_unlimited_timer(obj_proto[rnum].get()))
					{
						sprintf(str, "0");
					}
					else
					{
						sprintf(str, "%d", count);
					}
				}
				else
				{
					sprintf(str, "0");
				}
			}
			else if ((!str_cmp(field, "gameobj") || !str_cmp(field, "gameobjs")) && num > 0)
			{
				num = gcount_obj_vnum(num);
				if (num >= 0)
					sprintf(str, "%d", num);
			}
			else if (!str_cmp(field, "people") && num > 0)
			{
				sprintf(str, "%d", trgvar_in_room(num));
			}
			else if ((!str_cmp(field, "curmob") || !str_cmp(field, "curmobs")) && num > 0)
			{
				num = count_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%d", num);
			}
			else if ((!str_cmp(field, "gamemob") || !str_cmp(field, "gamemobs")) && num > 0)
			{
				num = gcount_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%d", num);
			}
			else if (!str_cmp(field, "zreset") && num > 0)
			{
				for (zone_rnum i = 0; i < static_cast<zone_rnum>(zone_table.size()); i++)
				{
					if (zone_table[i].number == num)
					{
						reset_zone(i);
					}
				}
			}
			else if (!str_cmp(field, "mob") && num > 0)
			{
				num = find_char_vnum(num);
				if (num >= 0)
					sprintf(str, "%c%d", UID_CHAR, num);
			}
			else if (!str_cmp(field, "obj") && num > 0)
			{
				num = find_obj_by_id_vnum__find_replacement(num);

				if (num >= 0)
					sprintf(str, "%c%d", UID_OBJ, num);
			}
			else if (!str_cmp(field, "room") && num > 0)
			{
				num = find_room_uid(num);
				if (num >= 0)
					sprintf(str, "%c%d", UID_ROOM, num);
			}
			//Polud world.maxobj(vnum) показывает максимальное количество предметов в мире,
			//которое прописано в самом предмете с указанным vnum
			else if ((!str_cmp(field, "maxobj") || !str_cmp(field, "maxobjs")) && num > 0)
			{
				num = real_object(num);
				if (num >= 0)
				{
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
		}
		else if (!str_cmp(var, "weather"))
		{
			if (!str_cmp(field, "temp"))
			{
				sprintf(str, "%d", weather_info.temperature);
			}
			else if (!str_cmp(field, "moon"))
			{
				sprintf(str, "%d", weather_info.moon_day);
			}
			else if (!str_cmp(field, "sky"))
			{
				num = -1;
				if ((num = atoi(subfield)) > 0)
					num = real_room(num);
				if (num != NOWHERE)
					sprintf(str, "%d", GET_ROOM_SKY(num));
				else
					sprintf(str, "%d", weather_info.sky);
			}
			else if (!str_cmp(field, "type"))
			{
				char c;
				int wt = weather_info.weather_type;
				num = -1;
				if ((num = atoi(subfield)) > 0)
					num = real_room(num);
				if (num != NOWHERE && world[num]->weather.duration > 0)
					wt = world[num]->weather.weather_type;
				for (c = 'a'; wt; ++c, wt >>= 1)
					if (wt & 1)
						sprintf(str + strlen(str), "%c", c);
			}
			else if (!str_cmp(field, "sunlight"))
			{
				switch (weather_info.sunlight)
				{
				case SUN_DARK:
					strcpy(str, "ночь");
					break;
				case SUN_SET:
					strcpy(str, "закат");
					break;
				case SUN_LIGHT:
					strcpy(str, "день");
					break;
				case SUN_RISE:
					strcpy(str, "рассвет");
					break;
				}

			}
			else if (!str_cmp(field, "season"))
			{
				switch (weather_info.season)
				{
				case SEASON_WINTER:
					strcat(str, "зима");
					break;
				case SEASON_SPRING:
					strcat(str, "весна");
					break;
				case SEASON_SUMMER:
					strcat(str, "лето");
					break;
				case SEASON_AUTUMN:
					strcat(str, "осень");
					break;
				}
			}
			return;
		}
		else if (!str_cmp(var, "time"))
		{
			if (!str_cmp(field, "hour"))
				sprintf(str, "%d", time_info.hours);
			else if (!str_cmp(field, "day"))
				sprintf(str, "%d", time_info.day + 1);
			else if (!str_cmp(field, "month"))
				sprintf(str, "%d", time_info.month + 1);
			else if (!str_cmp(field, "year"))
				sprintf(str, "%d", time_info.year);
			return;
		}
		else if (!str_cmp(var, "date"))
		{
			time_t now_time = time(0);
			if (!str_cmp(field, "unix"))
			{
				sprintf(str, "%ld", static_cast<long>(now_time));
			}
			else if (!str_cmp(field, "yday"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%j", localtime(&now_time));
			}
			else if (!str_cmp(field, "wday"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%w", localtime(&now_time));
			}
			else if (!str_cmp(field, "minute"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%M", localtime(&now_time));
			}
			else if (!str_cmp(field, "hour"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%H", localtime(&now_time));
			}
			else if (!str_cmp(field, "day"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%d", localtime(&now_time));
			}
			else if (!str_cmp(field, "month"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%m", localtime(&now_time));
			}
			else if (!str_cmp(field, "year"))
			{
				strftime(str, MAX_INPUT_LENGTH, "%y", localtime(&now_time));
			}
			return;
		}
		else if (!str_cmp(var, "random"))
		{
			if (!str_cmp(field, "char")
				|| !str_cmp(field, "pc")
				|| !str_cmp(field, "npc")
				|| !str_cmp(field, "all"))
			{
				rndm = NULL;
				count = 0;
				if (type == MOB_TRIGGER)
				{
					ch = (CHAR_DATA *)go;
					for (const auto c : world[ch->in_room]->people)
					{
						if (GET_INVIS_LEV(c)
							|| (c == ch)
							|| !CAN_SEE(ch, c))
						{
							continue;
						}

						if ((*field == 'a')
							|| (*field == 'p' && !IS_NPC(c))
							|| (*field == 'n' && IS_NPC(c) && !IS_CHARMED(c))
							|| (*field == 'c' && (!IS_NPC(c) || IS_CHARMED(c))))
						{
							if (!number(0, count))
							{
								rndm = c;
							}

							count++;
						}
					}
				}
				else if (type == OBJ_TRIGGER)
				{
					for (const auto c : world[obj_room((OBJ_DATA *)go)]->people)
					{
						if (GET_INVIS_LEV(c))
						{
							continue;
						}

						if ((*field == 'a')
							|| (*field == 'p' && !IS_NPC(c))
							|| (*field == 'n' && IS_NPC(c) && !IS_CHARMED(c))
							|| (*field == 'c' && (!IS_NPC(c) || IS_CHARMED(c))))
						{
							if (!number(0, count))
							{
								rndm = c;
							}

							count++;
						}
					}
				}
				else if (type == WLD_TRIGGER)
				{
					for (const auto c : ((ROOM_DATA *)go)->people)
					{
						if (GET_INVIS_LEV(c))
						{
							continue;
						}

						if ((*field == 'a')
							|| (*field == 'p' && !IS_NPC(c))
							|| (*field == 'n' && IS_NPC(c) && !IS_CHARMED(c))
							|| (*field == 'c' && (!IS_NPC(c) || IS_CHARMED(c))))
						{
							if (!number(0, count))
							{
								rndm = c;
							}

							count++;
						}
					}
				}

				if (rndm)
				{
					sprintf(str, "%c%ld", UID_CHAR, GET_ID(rndm));
				}
			}
			else
			{
				if (!str_cmp(field, "num"))
				{
					num = atoi(subfield);
				}
				else
				{
					num = atoi(field);
				}
				sprintf(str, "%d", (num > 0) ? number(1, num) : 0);
			}

			return;
		}
		else if (!str_cmp(var, "array"))
		{
			if (!str_cmp(field, "item")) {
				char *p = strchr(subfield, ',');
				int n = 0;
				if (!p) {
					p = subfield;
					n++;
				}
				else {
					*(p++) = '\0';
					n = atoi(p);
				}
				p = subfield;
				while (n) {
					char *retval = p;
					char tmp;
					for (; n; p++) {
						if (*p == ' ' || *p == '\0') {
							n--;
							if (n&&*p == '\0') {
								str[0] = '\0';
								return;
							}
							if (!n) {
								tmp = *p;
								*p = '\0';
								sprintf(str, "%s", retval);
								*p = tmp;
								return;
							}
							else {
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
		}
	}

	if (c)
	{
		if (!IS_NPC(c)
			&& !c->desc)
		{
			CharacterLinkDrop = true;
		}

		auto done = true;
		if (text_processed(field, subfield, vd, str))
		{
			return;
		}
		else if (!str_cmp(field, "global"))  	// get global of something else
		{
			if (IS_NPC(c))
			{
				find_replacement(go, c->script.get(), NULL, MOB_TRIGGER, subfield, NULL, NULL, str);
			}
		}
		else if (!str_cmp(field, "iname"))
			strcpy(str, GET_PAD(c, 0));
		else if (!str_cmp(field, "rname"))
			strcpy(str, GET_PAD(c, 1));
		else if (!str_cmp(field, "dname"))
			strcpy(str, GET_PAD(c, 2));
		else if (!str_cmp(field, "vname"))
			strcpy(str, GET_PAD(c, 3));
		else if (!str_cmp(field, "tname"))
			strcpy(str, GET_PAD(c, 4));
		else if (!str_cmp(field, "pname"))
			strcpy(str, GET_PAD(c, 5));
		else if (!str_cmp(field, "name"))
			strcpy(str, GET_NAME(c));
		else if (!str_cmp(field, "id"))
			sprintf(str, "%c%ld", UID_CHAR, GET_ID(c));
		else if (!str_cmp(field, "uniq"))
		{
			if (!IS_NPC(c))
				sprintf(str, "%d", GET_UNIQUE(c));
		}
		else if (!str_cmp(field, "alias"))
			strcpy(str, GET_PC_NAME(c));
		else if (!str_cmp(field, "level"))
			sprintf(str, "%d", GET_LEVEL(c));
		else if (!str_cmp(field, "remort"))
		{
			if (!IS_NPC(c))
				sprintf(str, "%d", GET_REMORT(c));
		}
		else if (!str_cmp(field, "hitp"))
		{
			GET_HIT(c) = (int)MAX(1, gm_char_field(c, field, subfield, (long)GET_HIT(c)));
			sprintf(str, "%d", GET_HIT(c));
		}
		else if (!str_cmp(field, "arenahp"))
		{
			CHAR_DATA *k;
			struct follow_type *f;
			int arena_hp = GET_HIT(c);
			int can_use = 0;

			if (!IS_NPC(c))
			{
				k = (c->has_master() ? c->get_master() : c);
				if (GET_CLASS(c) == 8)//чернок может дрыниться
				{
					can_use = 2;
				}
				else if (GET_CLASS(c) == 0 || GET_CLASS(c) == 13)//Клер или волхв может использовать покровительство
				{
					can_use = 1;
				}
				else if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP))
				{
					if (!IS_NPC(k) && (GET_CLASS(k) == 8 || GET_CLASS(k) == 13) //чернок или волхв может использовать ужи на согруппов
						&& world[IN_ROOM(k)]->zone == world[IN_ROOM(c)]->zone) //но только если находится в той же зоне
					{
						can_use = 1;
					}
					if (!can_use)
					{
						for (f = k->followers; f; f = f->next)
						{
							if (IS_NPC(f->follower)
								|| !AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
							{
								continue;
							}
							if ((GET_CLASS(f->follower) == 8
								|| GET_CLASS(f->follower) == 13) //чернок или волхв может использовать ужи на согруппов
								&& world[IN_ROOM(f->follower)]->zone == world[IN_ROOM(c)]->zone) //но только если находится в той же зоне
							{
								can_use = 1;
								break;
							}
						}
					}
				}
				if (can_use == 2)//дрын
				{
					arena_hp = GET_REAL_MAX_HIT(c) + GET_REAL_MAX_HIT(c) * GET_LEVEL(c) / 10;
				}
				else if (can_use == 1)//ужи и покров
				{
					arena_hp = GET_REAL_MAX_HIT(c) + GET_REAL_MAX_HIT(c) * 33 / 100;
				}
				else
				{
					arena_hp = GET_REAL_MAX_HIT(c);
				}
			}
			sprintf(str, "%d", arena_hp);
		}
		else if (!str_cmp(field, "hitpadd"))
		{
			GET_HIT_ADD(c) = (int)gm_char_field(c, field, subfield, (long)GET_HIT_ADD(c));
			sprintf(str, "%d", GET_HIT_ADD(c));
		}
		else if (!str_cmp(field, "maxhitp"))
		{
			// if (!IS_NPC(c))
			//   GET_MAX_HIT(c) = (sh_int) MAX(1,gm_char_field(c,field,subfield,(long)GET_MAX_HIT(c)));
			sprintf(str, "%d", GET_MAX_HIT(c));
		}
		else if (!str_cmp(field, "hitpreg"))
		{
			GET_HITREG(c) = (int)gm_char_field(c, field, subfield, (long)GET_HITREG(c));
			sprintf(str, "%d", GET_HITREG(c));
		}
		else if (!str_cmp(field, "mana"))
		{
			if (!IS_NPC(c))
				GET_MANA_STORED(c) =
				MAX(0, gm_char_field(c, field, subfield, (long)GET_MANA_STORED(c)));
			sprintf(str, "%d", GET_MANA_STORED(c));
		}
		else if (!str_cmp(field, "manareg"))
		{
			GET_MANAREG(c) = (int)gm_char_field(c, field, subfield, (long)GET_MANAREG(c));
			sprintf(str, "%d", GET_MANAREG(c));
		}
		else if (!str_cmp(field, "maxmana"))
			sprintf(str, "%d", GET_MAX_MANA(c));
		else if (!str_cmp(field, "move"))
		{
			if (!IS_NPC(c))
				GET_MOVE(c) =
				(sh_int)MAX(0, gm_char_field(c, field, subfield, (long)GET_MOVE(c)));
			sprintf(str, "%d", GET_MOVE(c));
		}
		else if (!str_cmp(field, "maxmove"))
		{
			//GET_MAX_MOVE(c) = (sh_int) MAX(1,gm_char_field(c,field,subfield,(long)GET_MAX_MOVE(c)));
			sprintf(str, "%d", GET_MAX_MOVE(c));
		}
		else if (!str_cmp(field, "moveadd"))
		{
			GET_MOVE_ADD(c) = (int)gm_char_field(c, field, subfield, (long)GET_MOVE_ADD(c));
			sprintf(str, "%d", GET_MOVE_ADD(c));
		}
		else if (!str_cmp(field, "movereg"))
		{
			GET_MOVEREG(c) = (int)gm_char_field(c, field, subfield, (long)GET_MOVEREG(c));
			sprintf(str, "%d", GET_MOVEREG(c));
		}
		else if (!str_cmp(field, "castsucc"))
		{
			GET_CAST_SUCCESS(c) =
				(int)gm_char_field(c, field, subfield, (long)GET_CAST_SUCCESS(c));
			sprintf(str, "%d", GET_CAST_SUCCESS(c));
		}
		else if (!str_cmp(field, "ageadd"))
		{
			if (!IS_NPC(c))
			{
				GET_AGE_ADD(c) = (int)gm_char_field(c, field, subfield, (long)GET_AGE_ADD(c));
				sprintf(str, "%d", GET_AGE_ADD(c));
			}
		}
		else if (!str_cmp(field, "age"))
		{
			if (!IS_NPC(c))
				sprintf(str, "%d", GET_REAL_AGE(c));
		}
		else if (!str_cmp(field, "hrbase"))
		{
			//GET_HR(c) = (int) gm_char_field(c,field,subfield,(long)GET_HR(c));
			sprintf(str, "%d", GET_HR(c));
		}
		else if (!str_cmp(field, "hradd"))
		{
			GET_HR_ADD(c) = (int)gm_char_field(c, field, subfield, (long)GET_HR(c));
			sprintf(str, "%d", GET_HR_ADD(c));
		}
		else if (!str_cmp(field, "hr"))
		{
			sprintf(str, "%d", GET_REAL_HR(c));
		}
		else if (!str_cmp(field, "drbase"))
		{
			//GET_DR(c) = (int) gm_char_field(c,field,subfield,(long)GET_DR(c));
			sprintf(str, "%d", GET_DR(c));
		}
		else if (!str_cmp(field, "dradd"))
		{
			GET_DR_ADD(c) = (int)gm_char_field(c, field, subfield, (long)GET_DR(c));
			sprintf(str, "%d", GET_DR_ADD(c));
		}
		else if (!str_cmp(field, "dr"))
		{
			sprintf(str, "%d", GET_REAL_DR(c));
		}
		else if (!str_cmp(field, "acbase"))
		{
			//GET_AC(c) = (int) gm_char_field(c,field,subfield,(long)GET_AC(c));
			sprintf(str, "%d", GET_AC(c));
		}
		else if (!str_cmp(field, "acadd"))
		{
			GET_AC_ADD(c) = (int)gm_char_field(c, field, subfield, (long)GET_AC(c));
			sprintf(str, "%d", GET_AC_ADD(c));
		}
		else if (!str_cmp(field, "ac"))
		{
			sprintf(str, "%d", GET_REAL_AC(c));
		}
		else if (!str_cmp(field, "morale")) // общая сумма морали
		{
			//GET_MORALE(c) = (int) gm_char_field(c, field, subfield, (long) GET_MORALE(c));
			sprintf(str, "%d", c->calc_morale());
		}
		else if (!str_cmp(field, "moraleadd")) // добавочная мораль
		{
			GET_MORALE(c) = (int)gm_char_field(c, field, subfield, (long)GET_MORALE(c));
			sprintf(str, "%d", GET_MORALE(c));
		}
		else if (!str_cmp(field, "poison"))
		{
			GET_POISON(c) = (int)gm_char_field(c, field, subfield, (long)GET_POISON(c));
			sprintf(str, "%d", GET_POISON(c));
		}
		else if (!str_cmp(field, "initiative"))
		{
			GET_INITIATIVE(c) = (int)gm_char_field(c, field, subfield, (long)GET_INITIATIVE(c));
			sprintf(str, "%d", GET_INITIATIVE(c));
		}
		else if (!str_cmp(field, "align"))
		{
			if (*subfield)
			{
				if (*subfield == '-')
					GET_ALIGNMENT(c) -= MAX(1, atoi(subfield + 1));
				else if (*subfield == '+')
					GET_ALIGNMENT(c) += MAX(1, atoi(subfield + 1));
			}
			sprintf(str, "%d", GET_ALIGNMENT(c));
		}
		else if (!str_cmp(field, "religion"))
		{
			if (*subfield && ((atoi(subfield) == RELIGION_POLY) || (atoi(subfield) == RELIGION_MONO)))
				GET_RELIGION(c) = atoi(subfield);
			else
				sprintf(str, "%d", GET_RELIGION(c));
		}
		else if ((!str_cmp(field, "restore")) || (!str_cmp(field, "fullrestore")))
		{
			/* Так, тупо, но иначе ругается, мол слишком много блоков*/
			if (!str_cmp(field, "fullrestore"))
			{
				do_arena_restore(c, (char *)c->get_name().c_str(), 0, SCMD_RESTORE_TRIGGER);
				trig_log(trig, "был произведен вызов do_arena_restore!");
			}
			else
			{
				do_restore(c, (char *)c->get_name().c_str(), 0, SCMD_RESTORE_TRIGGER);
				trig_log(trig, "был произведен вызов do_restore!");
			}
		}
		else if (!str_cmp(field, "dispel"))
		{
			if (!c->affected.empty())
			{
				send_to_char("Вы словно заново родились!\r\n", c);
			}

			while (!c->affected.empty())
			{
				c->affect_remove(c->affected.begin());
			}
		}
		else
		{
			done = false;
		}

		if (done)
		{
		}
		else if (!str_cmp(field, "hryvn"))
		{
			const long before = c->get_hryvn();
			int value;
			c->set_hryvn(MAX(0, gm_char_field(c, field, subfield, c->get_hryvn())));
			value = c->get_hryvn() - before;
			sprintf(buf, "<%s> {%d} получил триггером %d %s. [Trigger: %s, Vnum: %d]", GET_PAD(c, 0), GET_ROOM_VNUM(c->in_room), value, desc_count(value, WHAT_TORCu), GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig));
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
			sprintf(str, "%d", c->get_hryvn());
		}
		else if (!str_cmp(field, "gold"))
		{
			const long before = c->get_gold();
			int value;
			c->set_gold(MAX(0, gm_char_field(c, field, subfield, c->get_gold())));
			value = c->get_gold() - before;
			sprintf(buf, "<%s> {%d} получил триггером %d %s. [Trigger: %s, Vnum: %d]", GET_PAD(c, 0), GET_ROOM_VNUM(c->in_room), value, desc_count(value, WHAT_MONEYu), GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig));
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
			sprintf(str, "%ld", c->get_gold());
			// клан-налог
			const long diff = c->get_gold() - before;
			split_or_clan_tax(c, diff);
			// стата для show money
			if (!IS_NPC(c) && IN_ROOM(c) > 0)
			{
				MoneyDropStat::add(
					zone_table[world[IN_ROOM(c)]->zone].number, diff);
			}
		}
		else if (!str_cmp(field, "bank"))
		{
			const long before = c->get_bank();
			c->set_bank(MAX(0, gm_char_field(c, field, subfield, c->get_bank())));
			sprintf(str, "%ld", c->get_bank());
			// клан-налог
			const long diff = c->get_bank() - before;
			split_or_clan_tax(c, diff);
			// стата для show money
			if (!IS_NPC(c) && IN_ROOM(c) > 0)
			{
				MoneyDropStat::add(
					zone_table[world[IN_ROOM(c)]->zone].number, diff);
			}
		}
		else if (!str_cmp(field, "exp") || !str_cmp(field, "questbodrich"))
		{
			if (!str_cmp(field, "questbodrich"))
			{
				if (*subfield)
				{
					c->dquest(atoi(subfield));
				}
			}
			else
			{
				if (*subfield)
				{
					if (*subfield == '-')
					{
						gain_exp(c, -MAX(1, atoi(subfield + 1)));
						sprintf(buf, "SCRIPT_LOG (exp) у %s уменьшен опыт на %d в триггере %d", GET_NAME(c), MAX(1, atoi(subfield + 1)), GET_TRIG_VNUM(trig));
						mudlog(buf, BRF, LVL_GRGOD, ERRLOG, 1);
					}
					else if (*subfield == '+')
					{
						gain_exp(c, +MAX(1, atoi(subfield + 1)));
						sprintf(buf, "SCRIPT_LOG (exp) у %s увеличен опыт на %d в триггере %d", GET_NAME(c), MAX(1, atoi(subfield + 1)), GET_TRIG_VNUM(trig));
						mudlog(buf, BRF, LVL_GRGOD, ERRLOG, 1);
					}
					else
					{
						sprintf(buf, "SCRIPT_LOG (exp) ОШИБКА! у %s напрямую указан опыт %d в триггере %d", GET_NAME(c), atoi(subfield + 1), GET_TRIG_VNUM(trig));
						mudlog(buf, BRF, LVL_GRGOD, ERRLOG, 1);
					}
				}
				else
					sprintf(str, "%ld", GET_EXP(c));
			}			
		}
		else if (!str_cmp(field, "sex"))
			sprintf(str, "%d", (int)GET_SEX(c));
		else if (!str_cmp(field, "clan"))
		{
			if (CLAN(c))
			{
				sprintf(str, "%s", CLAN(c)->GetAbbrev());
				for (i = 0; str[i]; i++)
					str[i] = LOWER(str[i]);
			}
			else
				sprintf(str, "null");
		}
		else if (!str_cmp(field, "clanrank"))
		{
			if (CLAN(c) && CLAN_MEMBER(c))
				sprintf(str, "%d", CLAN_MEMBER(c)->rank_num);
			else
				sprintf(str, "null");
		}
		else if (!str_cmp(field, "m"))
			strcpy(str, HMHR(c));
		else if (!str_cmp(field, "s"))
			strcpy(str, HSHR(c));
		else if (!str_cmp(field, "e"))
			strcpy(str, HSSH(c));
		else if (!str_cmp(field, "g"))
			strcpy(str, GET_CH_SUF_1(c));
		else if (!str_cmp(field, "u"))
			strcpy(str, GET_CH_SUF_2(c));
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
		else if (!str_cmp(field, "canbeseen"))
		{
			if ((type == MOB_TRIGGER) && !CAN_SEE(((CHAR_DATA *)go), c))
			{
				strcpy(str, "0");
			}
			else
			{
				strcpy(str, "1");
			}
		}
		else if (!str_cmp(field, "class"))
		{
			sprintf(str, "%d", (int)GET_CLASS(c));
		}
		else if (!str_cmp(field, "race"))
		{
			sprintf(str, "%d", (int)GET_RACE(c));
		}
		else if (!str_cmp(field, "fighting"))
		{
			if (c->get_fighting())
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(c->get_fighting()));
			}
		}
		else if (!str_cmp(field, "is_killer"))
		{
			if (PLR_FLAGGED(c, PLR_KILLER))
			{
				strcpy(str, "1");
			}
			else
			{
				strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "is_thief"))
		{
			if (PLR_FLAGGED(c, PLR_THIEF))
			{
				strcpy(str, "1");
			}
			else
			{
				strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "rentable"))
		{
			if (!IS_NPC(c) && RENTABLE(c))
			{
				strcpy(str, "0");
			}
			else
			{
				strcpy(str, "1");
			}
		}
		else if (!str_cmp(field, "can_get_skill"))
		{
			if ((num = fix_name_and_find_skill_num(subfield)) > 0)
			{
				if (can_get_skill(c, num))
				{
					strcpy(str, "1");
				}
				else
				{
					strcpy(str, "0");
				}
			}
			else
			{
				sprintf(buf, "wrong skill name '%s'!", subfield);
				trig_log(trig, buf);
				strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "can_get_spell"))
		{
			if ((num = fix_name_and_find_spell_num(subfield)) > 0)
			{
				if (can_get_spell(c, num))
				{
					strcpy(str, "1");
				}
				else
				{
					strcpy(str, "0");
				}
			}
			else
			{
				sprintf(buf, "wrong spell name '%s'!", subfield);
				trig_log(trig, buf);
				strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "can_get_feat"))
		{
			if ((num = find_feat_num(subfield)) > 0)
			{
				if (can_get_feat(c, num))
					strcpy(str, "1");
				else
					strcpy(str, "0");
			}
			else
			{
				sprintf(buf, "wrong feature name '%s'!", subfield);
				trig_log(trig, buf);
				strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "agressor"))
		{
			if (AGRESSOR(c))
				sprintf(str, "%d", AGRESSOR(c));
			else
				strcpy(str, "0");
		}
		else if (!str_cmp(field, "vnum"))
		{
			sprintf(str, "%d", GET_MOB_VNUM(c));
		}
		else if (!str_cmp(field, "str"))
		{
			//GET_STR(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_STR(c)));
			sprintf(str, "%d", c->get_str());
		}
		else if (!str_cmp(field, "stradd"))
			sprintf(str, "%d", GET_STR_ADD(c));
		else if (!str_cmp(field, "int"))
		{
			//GET_INT(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_INT(c)));
			sprintf(str, "%d", c->get_int());
		}
		else if (!str_cmp(field, "intadd"))
			sprintf(str, "%d", GET_INT_ADD(c));
		else if (!str_cmp(field, "wis"))
		{
			//GET_WIS(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_WIS(c)));
			sprintf(str, "%d", c->get_wis());
		}
		else if (!str_cmp(field, "wisadd"))
			sprintf(str, "%d", GET_WIS_ADD(c));
		else if (!str_cmp(field, "dex"))
		{
			//GET_DEX(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_DEX(c)));
			sprintf(str, "%d", c->get_dex());
		}
		else if (!str_cmp(field, "dexadd"))
			sprintf(str, "%d", c->get_dex_add());
		else if (!str_cmp(field, "con"))
		{
			//GET_CON(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_CON(c)));
			sprintf(str, "%d", c->get_con());
		}
		else if (!str_cmp(field, "conadd"))
		{
			sprintf(str, "%d", GET_CON_ADD(c));
		}
		else if (!str_cmp(field, "cha"))
		{
			//GET_CHA(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_CHA(c)));
			sprintf(str, "%d", c->get_cha());
		}
		else if (!str_cmp(field, "chaadd"))
			sprintf(str, "%d", GET_CHA_ADD(c));
		else if (!str_cmp(field, "size"))
		{
			//GET_SIZE(c)=(sbyte) MAX(1,gm_char_field(c,field,subfield,(long) GET_SIZE(c)));
			sprintf(str, "%d", GET_SIZE(c));
		}
		else if (!str_cmp(field, "sizeadd"))
		{
			GET_SIZE_ADD(c) =
				(sbyte)MAX(1,
					gm_char_field(c, field, subfield,
					(long)GET_SIZE_ADD(c)));
			sprintf(str, "%d", GET_SIZE_ADD(c));
		}
		else if (!str_cmp(field, "room"))
		{
			int n = find_room_uid(world[IN_ROOM(c)]->number);
			if (n >= 0)
				sprintf(str, "%c%d", UID_ROOM, n);
		}
		else if (!str_cmp(field, "realroom"))
			sprintf(str, "%d", world[IN_ROOM(c)]->number);
		else if (!str_cmp(field, "loadroom"))
		{
			int pos;
			if (!IS_NPC(c))
			{
				if (!*subfield || !(pos = atoi(subfield)))
					sprintf(str, "%d", GET_LOADROOM(c));
				else
				{
					GET_LOADROOM(c) = pos;
					c->save_char();
					sprintf(str, "%d", real_room(pos)); // TODO: почему тогда тут рнум?
				}
			}
		}
		else if (!str_cmp(field, "skill"))
		{
			char *pos;
			while ((pos = strchr(subfield, '.')))
				* pos = ' ';
			while ((pos = strchr(subfield, '_')))
				* pos = ' ';
			strcpy(str, skill_percent(c, subfield));

		}
		else if (!str_cmp(field, "feat"))
		{
			if (feat_owner(trig, c, subfield))
				strcpy(str, "1");
			else
				strcpy(str, "0");
		}
		else if (!str_cmp(field, "spellcount"))
			strcpy(str, spell_count(trig, c, subfield));
		else if (!str_cmp(field, "spelltype"))
			strcpy(str, spell_knowledge(trig, c, subfield));
		else if (!str_cmp(field, "quested"))
		{
			if (*subfield && (num = atoi(subfield)) > 0)
			{
				if (c->quested_get(num))
					strcpy(str, "1");
				else
					strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "getquest"))
		{
			if (*subfield && (num = atoi(subfield)) > 0)
			{
				strcpy(str, (c->quested_get_text(num)).c_str());
			}
		}
		else if (!str_cmp(field, "setquest"))
		{
			if (*subfield)
			{
				subfield = one_argument(subfield, buf);
				skip_spaces(&subfield);
				if ((num = atoi(buf)) > 0)
				{
					c->quested_add(c, num, subfield);
				}
			}
		}
		// все эти блоки надо переписать на что-нибудь другое, их слишком много
		else if (!str_cmp(field, "unsetquest") || !str_cmp(field, "alliance"))
		{
			if (!str_cmp(field, "alliance"))
			{
				if (*subfield)
				{
					subfield = one_argument(subfield, buf);
					if (ClanSystem::is_alliance(c, buf))
						strcpy(str, "1");
					else
						strcpy(str, "0");
				}
			}
			else
			{
				if (*subfield && (num = atoi(subfield)) > 0)
				{
					c->quested_remove(num);
					strcpy(str, "1");
				}
			}
		}
		else if (!str_cmp(field, "eq"))
		{
			int pos = -1;
			if (a_isdigit(*subfield))
				pos = atoi(subfield);
			else if (*subfield)
				pos = find_eq_pos(c, NULL, subfield);
			if (!*subfield || pos < 0 || pos >= NUM_WEARS)
				strcpy(str, "");
			else
			{
				if (!GET_EQ(c, pos))
					strcpy(str, "");
				else
					sprintf(str, "%c%ld", UID_OBJ, GET_EQ(c, pos)->get_id());
			}
		}
		else if (!str_cmp(field, "haveobj") || !str_cmp(field, "haveobjs"))
		{
			int pos;
			if (a_isdigit(*subfield))
			{
				pos = atoi(subfield);
				for (obj = c->carrying; obj; obj = obj->get_next_content())
				{
					if (GET_OBJ_VNUM(obj) == pos)
					{
						break;
					}
				}
			}
			else
			{
				obj = get_obj_in_list_vis(c, subfield, c->carrying);
			}

			if (obj)
			{
				sprintf(str, "%c%ld", UID_OBJ, obj->get_id());
			}
			else
			{
				strcpy(str, "0");
			}
		}
		else if (!str_cmp(field, "varexist") || !str_cmp(field, "varexists"))
		{
			strcpy(str, "0");
			if (find_var_cntx(&((SCRIPT(c))->global_vars), subfield, sc->context))
			{
				strcpy(str, "1");
			}
		}
		else if (!str_cmp(field, "next_in_room"))
		{
			CHAR_DATA* next = nullptr;
			const auto room = world[c->in_room];

			auto people_i = std::find(room->people.begin(), room->people.end(), c);

			if (people_i != room->people.end())
			{
				++people_i;
				people_i = std::find_if(people_i, room->people.end(),
					[](const CHAR_DATA* ch)
				{
					return !GET_INVIS_LEV(ch);
				});

				if (people_i != room->people.end())
				{
					next = *people_i;
				}
			}

			if (next)
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(next));
			}
			else
			{
				strcpy(str, "");
			}
		}
		else if (!str_cmp(field, "position"))
		{
			int pos;

			if (!*subfield || (pos = atoi(subfield)) <= POS_DEAD)
				sprintf(str, "%d", GET_POS(c));
			else if (!WAITLESS(c))
			{
				if (on_horse(c))
				{
					AFF_FLAGS(c).unset(EAffectFlag::AFF_HORSE);
				}
				GET_POS(c) = pos;
			}
		}
		else if (!str_cmp(field, "wait"))
		{
			int pos;

			if (!*subfield || (pos = atoi(subfield)) <= 0)
			{
				sprintf(str, "%d", GET_WAIT(c));
			}
			else if (!WAITLESS(c))
			{
				WAIT_STATE(c, pos * PULSE_VIOLENCE);
			}
		}
		else if (!str_cmp(field, "affect"))
		{
			c->char_specials.saved.affected_by.gm_flag(subfield, affected_bits, str);
		}

		//added by WorM
		//собственно подозреваю что никто из билдеров даже не вкурсе насчет всего функционала этого affect
		//тупизм какой-то проверять аффекты обездвижен,летит и т.п.
		//к тому же они в том списке не все кличи например никак там не отображаются
		else if (!str_cmp(field, "affected_by"))
		{
			if ((num = fix_name_and_find_spell_num(subfield)) > 0)
			{
				sprintf(str, "%d", (int)affected_by_spell(c, num));
			}
		}
		else if (!str_cmp(field, "action"))
		{
			if (IS_NPC(c))
			{
				c->char_specials.saved.act.gm_flag(subfield, action_bits, str);
			}
		}
		else if (!str_cmp(field, "leader"))
		{
			if (c->has_master())
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(c->get_master()));
			}
		}
		else if (!str_cmp(field, "group"))
		{
			CHAR_DATA *l;
			struct follow_type *f;
			if (!AFF_FLAGGED(c, EAffectFlag::AFF_GROUP))
			{
				return;
			}
			l = c->get_master();
			if (!l)
			{
				l = c;
			}
			// l - лидер группы
			sprintf(str + strlen(str), "%c%ld ", UID_CHAR, GET_ID(l));
			for (f = l->followers; f; f = f->next)
			{
				if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
				{
					continue;
				}
				sprintf(str + strlen(str), "%c%ld ", UID_CHAR, GET_ID(f->follower));
			}
		}
		else if (!str_cmp(field, "attackers"))
		{
			CHAR_DATA *t;
			size_t str_length = strlen(str);
			for (t = combat_list; t; t = t->next_fighting)
			{
				if (t->get_fighting() != c)
				{
					continue;
				}
				int n = snprintf(tmp, MAX_INPUT_LENGTH, "%c%ld ", UID_CHAR, GET_ID(t));
				if (str_length + n < MAX_INPUT_LENGTH) // not counting the terminating null character
				{
					strcpy(str + str_length, tmp);
					str_length += n;
				}
				else {
					break; // too many attackers
				}
			}
		}
		else if (!str_cmp(field, "people"))
		{
			//const auto first_char = world[IN_ROOM(c)]->first_character();
			const auto room = world[IN_ROOM(c)]->people;
			const auto first_char = std::find_if(room.begin(), room.end(),
				[](CHAR_DATA* ch)
			{
				return !GET_INVIS_LEV(ch);
			});

			if (first_char != room.end())
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(*first_char));
			}
			else
			{
				strcpy(str, "");
			}
		}
		//Polud обработка поля objs у чара, возвращает строку со списком UID предметов в инвентаре
		else if (!str_cmp(field, "objs"))
		{
			size_t str_length = strlen(str);
			for (obj = c->carrying; obj; obj = obj->get_next_content())
			{
				int n = snprintf(tmp, MAX_INPUT_LENGTH, "%c%ld ", UID_OBJ, obj->get_id());
				if (str_length + n < MAX_INPUT_LENGTH) // not counting the terminating null character
				{
					strcpy(str + str_length, tmp);
					str_length += n;
				}
				else {
					break; // too many carying objects
				}
			}
		}
		//-Polud
		else if (!str_cmp(field, "char")
			|| !str_cmp(field, "pc")
			|| !str_cmp(field, "npc")
			|| !str_cmp(field, "all"))
		{
			int inroom;

			// Составление списка (для mob)
			inroom = IN_ROOM(c);
			if (inroom == NOWHERE)
			{
				trig_log(trig, "mob-построитель списка в NOWHERE");
				return;
			}

			size_t str_length = strlen(str);
			for (const auto rndm : world[inroom]->people)
			{
				if (!CAN_SEE(c, rndm)
					|| GET_INVIS_LEV(rndm))
				{
					continue;
				}

				if ((*field == 'a')
					|| (!IS_NPC(rndm)
						&& *field != 'n'
						&& rndm->desc)
					|| (IS_NPC(rndm)
						&& IS_CHARMED(rndm)
						&& *field == 'c')
					|| (IS_NPC(rndm)
						&& !IS_CHARMED(rndm)
						&& *field == 'n'))
				{
					int n = snprintf(tmp, MAX_INPUT_LENGTH, "%c%ld ", UID_CHAR, GET_ID(rndm));
					if (str_length + n < MAX_INPUT_LENGTH) // not counting the terminating null character
					{
						strcpy(str + str_length, tmp);
						str_length += n;
					}
					else
					{
						break; // too many characters
					}
				}
			}

			return;
		}
		else if (!str_cmp(field, "is_noob"))
		{
			strcpy(str, Noob::is_noob(c) ? "1" : "0");
		}
		else if (!str_cmp(field, "noob_outfit"))
		{
			std::string vnum_str = Noob::print_start_outfit(c);
			snprintf(str, MAX_INPUT_LENGTH, "%s", vnum_str.c_str());
		}
		else
		{
			vd = find_var_cntx(&((SCRIPT(c))->global_vars), field,
				sc->context);
			if (vd)
				sprintf(str, "%s", vd->value);
			else
			{
				sprintf(buf2, "unknown char field: '%s'", field);
				trig_log(trig, buf2);
			}
		}
	}
	else if (o)
	{
		if (text_processed(field, subfield, vd, str))
		{
			return;
		}
		else if (!str_cmp(field, "iname"))
		{
			if (!o->get_PName(0).empty())
			{
				strcpy(str, o->get_PName(0).c_str());
			}
			else
			{
				strcpy(str, o->get_aliases().c_str());
			}
		}
		else if (!str_cmp(field, "rname"))
		{
			if (!o->get_PName(1).empty())
			{
				strcpy(str, o->get_PName(1).c_str());
			}
			else
			{
				strcpy(str, o->get_aliases().c_str());
			}
		}
		else if (!str_cmp(field, "dname"))
		{
			if (!o->get_PName(2).empty())
			{
				strcpy(str, o->get_PName(2).c_str());
			}
			else
			{
				strcpy(str, o->get_aliases().c_str());
			}
		}
		else if (!str_cmp(field, "vname"))
		{
			if (!o->get_PName(3).empty())
			{
				strcpy(str, o->get_PName(3).c_str());
			}
			else
			{
				strcpy(str, o->get_aliases().c_str());
			}
		}
		else if (!str_cmp(field, "tname"))
		{
			if (!o->get_PName(4).empty())
			{
				strcpy(str, o->get_PName(4).c_str());
			}
			else
			{
				strcpy(str, o->get_aliases().c_str());
			}
		}
		else if (!str_cmp(field, "pname"))
		{
			if (!o->get_PName(5).empty())
			{
				strcpy(str, o->get_PName(5).c_str());
			}
			else
			{
				strcpy(str, o->get_aliases().c_str());
			}
		}
		else if (!str_cmp(field, "name"))
		{
			strcpy(str, o->get_aliases().c_str());
		}
		else if (!str_cmp(field, "id"))
		{
			sprintf(str, "%c%ld", UID_OBJ, o->get_id());
		}
		else if (!str_cmp(field, "uid"))
		{
			if (!GET_OBJ_UID(o))
			{
				set_uid(o);
			}
			sprintf(str, "%u", GET_OBJ_UID(o));
		}
		else if (!str_cmp(field, "skill"))
		{
			sprintf(str, "%d", GET_OBJ_SKILL(o));
		}
		else if (!str_cmp(field, "shortdesc"))
		{
			strcpy(str, o->get_short_description().c_str());
		}
		else if (!str_cmp(field, "vnum"))
		{
			sprintf(str, "%d", GET_OBJ_VNUM(o));
		}
		else if (!str_cmp(field, "type"))
		{
			sprintf(str, "%d", (int)GET_OBJ_TYPE(o));
		}
		else if (!str_cmp(field, "timer"))
		{
			sprintf(str, "%d", o->get_timer());
		}
		else if (!str_cmp(field, "val0"))
		{
			if (*subfield)
			{
				skip_spaces(&subfield);
				o->set_val(0, atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_VAL(o, 0));
			}
		}
		else if (!str_cmp(field, "val1"))
		{
			if (*subfield)
			{
				skip_spaces(&subfield);
				o->set_val(1, atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_VAL(o, 1));
			}
		}
		else if (!str_cmp(field, "val2"))
		{
			if (*subfield)
			{
				skip_spaces(&subfield);
				o->set_val(2, atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_VAL(o, 2));
			}
		}
		else if (!str_cmp(field, "val3"))
		{
			if (*subfield)
			{
				skip_spaces(&subfield);
				o->set_val(3, atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_VAL(o, 3));
			}
		}
		else if (!str_cmp(field, "maker"))
		{
			sprintf(str, "%d", GET_OBJ_MAKER(o));
		}
		else if (!str_cmp(field, "effect"))
		{
			o->gm_extra_flag(subfield, extra_bits, str);
		}
		else if (!str_cmp(field, "affect"))
		{
			o->gm_affect_flag(subfield, weapon_affects, str);
		}
		else if (!str_cmp(field, "carried_by"))
		{
			if (o->get_carried_by())
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->get_carried_by()));
			}
			else
			{
				strcpy(str, "");
			}
		}
		else if (!str_cmp(field, "worn_by"))
		{
			if (o->get_worn_by())
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->get_worn_by()));
			}
			else
			{
				strcpy(str, "");
			}
		}
		else if (!str_cmp(field, "g"))
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
			sprintf(str, "%d", (int)GET_OBJ_SEX(o));
		else if (!str_cmp(field, "room"))
		{
			if (o->get_carried_by())
			{
				sprintf(str, "%d", world[IN_ROOM(o->get_carried_by())]->number);
			}
			else if (o->get_worn_by())
			{
				sprintf(str, "%d", world[IN_ROOM(o->get_worn_by())]->number);
			}
			else if (o->get_in_room() != NOWHERE)
			{
				sprintf(str, "%d", world[o->get_in_room()]->number);
			}
			else
			{
				strcpy(str, "");
			}
		}
		//Polud обработка %obj.put(UID)% - пытается поместить объект в контейнер, комнату или инвентарь чара, в зависимости от UIDа
		else if (!str_cmp(field, "put"))
		{
			OBJ_DATA *obj_to = NULL;
			CHAR_DATA *char_to = NULL;
			ROOM_DATA *room_to = NULL;
			if (!((*subfield == UID_CHAR) || (*subfield == UID_OBJ) || (*subfield == UID_ROOM)))
			{
				trig_log(trig, "object.put: недопустимый аргумент, необходимо указать UID");
				return;
			}
			if (*subfield == UID_OBJ)
			{
				obj_to = find_obj_by_id(atoi(subfield + 1));
				if (!(obj_to
					&& GET_OBJ_TYPE(obj_to) == OBJ_DATA::ITEM_CONTAINER))
				{
					trig_log(trig, "object.put: объект-приемник не найден или не является контейнером");
					return;
				}
			}
			if (*subfield == UID_CHAR)
			{
				char_to = find_char(atoi(subfield + 1));
				if (!(char_to && can_take_obj(char_to, o)))
				{
					trig_log(trig, "object.put: субъект-приемник не найден или не может нести этот объект");
					return;
				}
			}
			if (*subfield == UID_ROOM)
			{
				room_to = find_room(atoi(subfield + 1));
				if (!(room_to && (room_to->number != NOWHERE)))
				{
					trig_log(trig, "object.put: недопустимая комната для размещения объекта");
					return;
				}
			}
			//found something to put our object
			//let's make it nobody's!
			if (o->get_worn_by())
			{
				unequip_char(o->get_worn_by(), o->get_worn_on());
			}
			else if (o->get_carried_by())
			{
				obj_from_char(o);
			}
			else if (o->get_in_obj())
			{
				obj_from_obj(o);
			}
			else if (o->get_in_room() > NOWHERE)
			{
				obj_from_room(o);
			}
			else
			{
				trig_log(trig, "object.put: не удалось извлечь объект");
				return;
			}
			//finally, put it to destination
			if (char_to)
				obj_to_char(o, char_to);
			else if (obj_to)
				obj_to_obj(o, obj_to);
			else if (room_to)
				obj_to_room(o, real_room(room_to->number));
			else
			{
				sprintf(buf2, "object.put: ATTENTION! за время подготовки объекта >%s< к передаче перестал существовать адресат. Объект сейчас в NOWHERE",
					o->get_short_description().c_str());
				trig_log(trig, buf2);
				return;
			}
		}
		//-Polud
		else if (!str_cmp(field, "char") ||
			!str_cmp(field, "pc") || !str_cmp(field, "npc") || !str_cmp(field, "all"))
		{
			int inroom;

			// Составление списка (для obj)
			inroom = obj_room(o);
			if (inroom == NOWHERE)
			{
				trig_log(trig, "obj-построитель списка в NOWHERE");
				return;
			}

			size_t str_length = strlen(str);
			for (const auto rndm : world[inroom]->people)
			{
				if (GET_INVIS_LEV(rndm))
					continue;

				if ((*field == 'a')
					|| (!IS_NPC(rndm)
						&& *field != 'n')
					|| (IS_NPC(rndm)
						&& IS_CHARMED(rndm)
						&& *field == 'c')
					|| (IS_NPC(rndm)
						&& !IS_CHARMED(rndm)
						&& *field == 'n'))
				{
					int n = snprintf(tmp, MAX_INPUT_LENGTH, "%c%ld ", UID_CHAR, GET_ID(rndm));
					if (str_length + n < MAX_INPUT_LENGTH) // not counting the terminating null character
					{
						strcpy(str + str_length, tmp);
						str_length += n;
					}
					else
					{
						break; // too many characters
					}
				}
			}

			return;
		}
		else if (!str_cmp(field, "owner"))
		{
			if (*subfield)
			{
				skip_spaces(&subfield);
				int num = atoi(subfield);
				// Убрал пока проверку. По идее 0 -- отсутствие владельца.
				// Понадобилась возможность обнулить владельца из трига.
				o->set_owner(num);
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_OWNER(o));
			}
		}
		else if (!str_cmp(field, "varexists"))
		{
			strcpy(str, "0");
			if (find_var_cntx(&o->get_script()->global_vars, subfield, sc->context))
			{
				strcpy(str, "1");
			}
		}
		else if (!str_cmp(field, "cost"))
		{
			if (*subfield && a_isdigit(*subfield))
			{
				skip_spaces(&subfield);
				o->set_cost(atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_COST(o));
			}
		}
		else if (!str_cmp(field, "rent"))
		{
			if (*subfield && a_isdigit(*subfield))
			{
				skip_spaces(&subfield);
				o->set_rent_off(atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_RENT(o));
			}
		}
		else if (!str_cmp(field, "rent_eq"))
		{
			if (*subfield && a_isdigit(*subfield))
			{
				skip_spaces(&subfield);
				o->set_rent_on(atoi(subfield));
			}
			else
			{
				sprintf(str, "%d", GET_OBJ_RENTEQ(o));
			}
		}
		else //get global var. obj.varname
		{
			vd = find_var_cntx(&o->get_script()->global_vars, field, sc->context);
			if (vd)
			{
				sprintf(str, "%s", vd->value);
			}
			else
			{
				sprintf(buf2, "Type: %d. unknown object field: '%s'", type, field);
				trig_log(trig, buf2);
			}
		}
	}
	else if (r)
	{
		if (text_processed(field, subfield, vd, str))
		{
			return;
		}
		else if (!str_cmp(field, "name"))
		{
			strcpy(str, r->name);
		}
		else if (!str_cmp(field, "north"))
		{
			if (r->dir_option[NORTH])
			{
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[NORTH]->to_room())));
			}
		}
		else if (!str_cmp(field, "east"))
		{
			if (r->dir_option[EAST])
			{
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[EAST]->to_room())));
			}
		}
		else if (!str_cmp(field, "south"))
		{
			if (r->dir_option[SOUTH])
			{
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[SOUTH]->to_room())));
			}
		}
		else if (!str_cmp(field, "west"))
		{
			if (r->dir_option[WEST])
			{
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[WEST]->to_room())));
			}
		}
		else if (!str_cmp(field, "up"))
		{
			if (r->dir_option[UP])
			{
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[UP]->to_room())));
			}
		}
		else if (!str_cmp(field, "down"))
		{
			if (r->dir_option[DOWN])
			{
				sprintf(str, "%d", find_room_vnum(GET_ROOM_VNUM(r->dir_option[DOWN]->to_room())));
			}
		}
		else if (!str_cmp(field, "vnum"))
		{
			sprintf(str, "%d", r->number);
		}
		else if (!str_cmp(field, "sectortype"))//Polud возвращает строку - тип комнаты
		{
			sprinttype(r->sector_type, sector_types, str);
		}
		else if (!str_cmp(field, "id"))
		{
			sprintf(str, "%c%d", UID_ROOM, find_room_uid(r->number));
		}
		else if (!str_cmp(field, "flag"))
		{
			r->gm_flag(subfield, room_bits, str);
		}
		else if (!str_cmp(field, "people"))
		{
			const auto first_char = r->first_character();
			if (first_char)
			{
				sprintf(str, "%c%ld", UID_CHAR, GET_ID(first_char));
			}
		}
		else if (!str_cmp(field, "char")
			|| !str_cmp(field, "pc")
			|| !str_cmp(field, "npc")
			|| !str_cmp(field, "all"))
		{
			int inroom;

			// Составление списка (для room)
			inroom = real_room(r->number);
			if (inroom == NOWHERE)
			{
				trig_log(trig, "room-построитель списка в NOWHERE");
				return;
			}

			size_t str_length = strlen(str);
			for (const auto rndm : world[inroom]->people)
			{
				if (GET_INVIS_LEV(rndm))
					continue;

				if ((*field == 'a')
					|| (!IS_NPC(rndm)
						&& *field != 'n')
					|| (IS_NPC(rndm)
						&& IS_CHARMED(rndm)
						&& *field == 'c')
					|| (IS_NPC(rndm)
						&& !IS_CHARMED(rndm)
						&& *field == 'n'))
				{
					int n = snprintf(tmp, MAX_INPUT_LENGTH, "%c%ld ", UID_CHAR, GET_ID(rndm));
					if (str_length + n < MAX_INPUT_LENGTH) // not counting the terminating null character
					{
						strcpy(str + str_length, tmp);
						str_length += n;
					}
					else
					{
						break; // too many characters
					}
				}
			}

			return;
		}
		else if (!str_cmp(field, "objects"))
		{
			//mixaz  Выдаем список объектов в комнате
			int inroom;
			// Составление списка (для room)
			inroom = real_room(r->number);
			if (inroom == NOWHERE)
			{
				trig_log(trig, "room-построитель списка в NOWHERE");
				return;
			}

			size_t str_length = strlen(str);
			for (obj = world[inroom]->contents; obj; obj = obj->get_next_content())
			{
				int n = snprintf(tmp, MAX_INPUT_LENGTH, "%c%ld ", UID_OBJ, obj->get_id());
				if (str_length + n < MAX_INPUT_LENGTH) // not counting the terminating null character
				{
					strcpy(str + str_length, tmp);
					str_length += n;
				}
				else {
					break; // too many objects
				}
			}
			return;
			//mixaz - end
		}
		else if (!str_cmp(field, "varexists"))
		{
			//room.varexists<0;1>
			strcpy(str, "0");
			if (find_var_cntx(&((SCRIPT(r))->global_vars), subfield, sc->context))
			{
				strcpy(str, "1");
			}
		}
		else //get global var. room.varname
		{
			vd = find_var_cntx(&((SCRIPT(r))->global_vars), field, sc->context);
			if (vd)
			{
				sprintf(str, "%s", vd->value);
			}
			else
			{
				sprintf(buf2, "Type: %d. unknown room field: '%s'", type, field);
				trig_log(trig, buf2);
			}
		}
	}
	else if (text_processed(field, subfield, vd, str))
	{
		return;
	}
}

// substitutes any variables into line and returns it as buf
void var_subst(void* go, SCRIPT_DATA* sc, TRIG_DATA* trig, int type, const char* line, char* buf)
{
	char tmp[MAX_INPUT_LENGTH], repl_str[MAX_INPUT_LENGTH], *var, *field, *p;
	char *subfield_p, subfield[MAX_INPUT_LENGTH];
	char *local_p, local[MAX_INPUT_LENGTH];
	int paren_count = 0;

	if (!strchr(line, '%'))
	{
		strcpy(buf, line);
		return;
	}

	p = strcpy(tmp, line);
	subfield_p = subfield;

	size_t left = MAX_INPUT_LENGTH - 1;
	while (*p && (left > 0))
	{
		while (*p && (*p != '%') && (left > 0))
		{
			*(buf++) = *(p++);
			left--;
		}

		*buf = '\0';

		if (*p && (*(++p) == '%') && (left > 0))
		{
			*(buf++) = *(p++);
			*buf = '\0';
			left--;
			continue;
		}
		else if (*p && (left > 0))
		{
			for (var = p; *p && (*p != '%') && (*p != '.'); p++);

			field = p;
			subfield_p = subfield;	//new
			if (*p == '.')
			{
				*(p++) = '\0';
				local_p = local;
				for (field = p; *p && local_p && ((*p != '%') || (paren_count)); p++)
				{
					if (*p == '(')
					{
						if (!paren_count)
							*p = '\0';
						else
							*local_p++ = *p;
						paren_count++;
					}
					else if (*p == ')')
					{
						if (paren_count == 1)
							*p = '\0';
						else
							*local_p++ = *p;
						paren_count--;
						if (!paren_count)
						{
							*local_p = '\0';
							var_subst(go, sc, trig, type, local, subfield_p);
							local_p = NULL;
							subfield_p = subfield + strlen(subfield);
						}
					}
					else if (paren_count)
					{
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
int is_num(const char *num)
{
	while (*num
		&& (a_isdigit(*num)
			|| *num == '-'))
	{
		num++;
	}

	return (!*num || a_isspace(*num)) ? 1 : 0;
}

// evaluates 'lhs op rhs', and copies to result
void eval_op(const char *op, char *lhs, const char *const_rhs, char *result, void* /*go*/, SCRIPT_DATA* /*sc*/, TRIG_DATA* /*trig*/)
{
	char *p = nullptr;
	int n;

	// strip off extra spaces at begin and end
	while (*lhs && a_isspace(*lhs))
	{
		lhs++;
	}

	char* rhs = nullptr;
	std::shared_ptr<char> rhs_guard(rhs = str_dup(const_rhs), free);

	while (*rhs && a_isspace(*rhs))
	{
		rhs++;
	}

	for (p = lhs + strlen(lhs) - 1; (p >= lhs) && a_isspace(*p); *p-- = '\0');
	for (p = rhs + strlen(rhs) - 1; (p >= rhs) && a_isspace(*p); *p-- = '\0');

	// find the op, and figure out the value
	if (!strcmp("||", op))
	{
		if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
			strcpy(result, "0");
		else
			strcpy(result, "1");
	}
	else if (!strcmp("&&", op))
	{
		if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
			strcpy(result, "0");
		else
			strcpy(result, "1");
	}
	else if (!strcmp("==", op))
	{
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) == atoi(rhs));
		else
			sprintf(result, "%d", !str_cmp(lhs, rhs));
	}
	else if (!strcmp("!=", op))
	{
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) != atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs));
	}
	else if (!strcmp("<=", op))
	{
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
	}
	else if (!strcmp(">=", op))
	{
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
	}
	else if (!strcmp("<", op))
	{
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) < atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
	}
	else if (!strcmp(">", op))
	{
		if (is_num(lhs) && is_num(rhs))
			sprintf(result, "%d", atoi(lhs) > atoi(rhs));
		else
			sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
	}
	else if (!strcmp("/=", op))
		sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');
	else if (!strcmp("*", op))
		sprintf(result, "%d", atoi(lhs) * atoi(rhs));
	else if (!strcmp("/", op))
		sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);
	else if (!strcmp("+", op))
		sprintf(result, "%d", atoi(lhs) + atoi(rhs));
	else if (!strcmp("-", op))
		sprintf(result, "%d", atoi(lhs) - atoi(rhs));
	else if (!strcmp("!", op))
	{
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
char *matching_quote(char *p)
{
	for (p++; *p && (*p != '"'); p++)
	{
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
char *matching_paren(char *p)
{
	int i;

	for (p++, i = 1; *p && i; p++)
	{
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
void eval_expr(const char *line, char *result, void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type)
{
	char expr[MAX_INPUT_LENGTH], *p;

	while (*line && a_isspace(*line))
	{
		line++;
	}

	if (eval_lhs_op_rhs(line, result, go, sc, trig, type))
	{
		;
	}
	else if (*line == '(')
	{
		strcpy(expr, line);
		p = matching_paren(expr);
		*p = '\0';
		eval_expr(expr + 1, result, go, sc, trig, type);
	}
	else
	{
		var_subst(go, sc, trig, type, line, result);
	}
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(const char *expr, char *result, void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type)
{
	char *p, *tokens[MAX_INPUT_LENGTH];
	char line[MAX_INPUT_LENGTH], lhr[MAX_INPUT_LENGTH], rhr[MAX_INPUT_LENGTH];
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
	for (j = 0; *p; j++)
	{
		tokens[j] = p;
		if (*p == '(')
			p = matching_paren(p) + 1;
		else if (*p == '"')
			p = matching_quote(p) + 1;
		else if (a_isalnum(*p))
			for (p++; *p && (a_isalnum(*p) || a_isspace(*p)); p++);
		else
			p++;
	}
	tokens[j] = NULL;

	for (i = 0; *ops[i] != '\n'; i++)
		for (j = 0; tokens[j]; j++)
			if (!strn_cmp(ops[i], tokens[j], strlen(ops[i])))
			{
				*tokens[j] = '\0';
				p = tokens[j] + strlen(ops[i]);

				eval_expr(line, lhr, go, sc, trig, type);
				eval_expr(p, rhr, go, sc, trig, type);
				eval_op(ops[i], lhr, rhr, result, go, sc, trig);

				return 1;
			}

	return 0;
}

const auto FOREACH_LIST_GUID = "{18B3D8D1-240E-4D60-AEAB-6748580CA460}";
const auto FOREACH_LIST_POS_GUID = "{4CC4E031-7376-4EED-AD4F-2FD0DC8D4E2D}";

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
int process_foreach_begin(const char* cond, void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type)
{
	char name[MAX_INPUT_LENGTH];
	char list_str[MAX_INPUT_LENGTH];
	char value[MAX_INPUT_LENGTH];

	skip_spaces(&cond);
	auto p = one_argument(cond, name);
	skip_spaces(&p);

	if (!*name)
	{
		trig_log(trig, "foreach begin w/o an variable");
		return 0;
	}

	eval_expr(p, list_str, go, sc, trig, type);
	p = list_str;

	if (!p || !*p)
	{
		return 0;
	}

	int v_strpos;
	auto pos = strchr(p, ' ');
	if (!pos)
	{
		strcpy(value, p);
		v_strpos = static_cast<int>(strlen(p));
	}
	else
	{
		strncpy(value, p, pos - p);
		value[pos - p] = '\0';
		skip_spaces(&pos);
		v_strpos = pos - p;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);

	sprintf(value, "%s%s", name, FOREACH_LIST_GUID);
	add_var_cntx(&GET_TRIG_VARS(trig), value, list_str, 0);

	strcat(name, FOREACH_LIST_POS_GUID);
	sprintf(value, "%d", v_strpos);
	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);

	return 1;
}

int process_foreach_done(const char* cond, void *, SCRIPT_DATA *, TRIG_DATA * trig, int)
{
	char name[MAX_INPUT_LENGTH];
	char value[MAX_INPUT_LENGTH];

	skip_spaces(&cond);
	one_argument(cond, name);

	if (!*name)
	{
		trig_log(trig, "foreach done w/o an variable");
		return 0;
	}

	sprintf(value, "%s%s", name, FOREACH_LIST_GUID);
	const auto var_list = find_var_cntx(&GET_TRIG_VARS(trig), value, 0);

	sprintf(value, "%s%s", name, FOREACH_LIST_POS_GUID);
	const auto var_list_pos = find_var_cntx(&GET_TRIG_VARS(trig), value, 0);

	if (!var_list || !var_list_pos)
	{
		trig_log(trig, "foreach utility vars not found");
		return 0;
	}

	const auto p = var_list->value + static_cast<unsigned int>(atoi(var_list_pos->value));
	if (!p || !*p)
	{
		remove_var_cntx(&GET_TRIG_VARS(trig), name, 0);

		sprintf(value, "%s%s", name, FOREACH_LIST_GUID);
		remove_var_cntx(&GET_TRIG_VARS(trig), value, 0);

		sprintf(value, "%s%s", name, FOREACH_LIST_POS_GUID);
		remove_var_cntx(&GET_TRIG_VARS(trig), value, 0);

		return 0;
	}

	int v_strpos;
	auto pos = strchr(p, ' ');
	if (!pos)
	{
		strcpy(value, p);
		v_strpos = static_cast<int>(strlen(var_list->value));
	}
	else
	{
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
int process_if(const char *cond, void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type)
{
	char result[MAX_INPUT_LENGTH], *p;

	eval_expr(cond, result, go, sc, trig, type);

	p = result;
	skip_spaces(&p);

	if (!*p || *p == '0')
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * scans for end of if-block.
 * returns the line containg 'end', or NULL
 */
cmdlist_element::shared_ptr find_end(TRIG_DATA * trig, cmdlist_element::shared_ptr cl)
{
	char tmpbuf[MAX_INPUT_LENGTH];
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	while ((cl = cl ? cl->next : cl) != NULL)
	{
		for (p = cl->cmd.c_str(); *p && a_isspace(*p); p++);

		if (!strn_cmp("if ", p, 3))
		{
			cl = find_end(trig, cl);
		}
		else if (!strn_cmp("end", p, 3))
		{
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl)
	{
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
cmdlist_element::shared_ptr find_else_end(TRIG_DATA * trig, cmdlist_element::shared_ptr cl, void *go, SCRIPT_DATA * sc, int type)
{
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	while ((cl = cl ? cl->next : cl) != NULL)
	{
		for (p = cl->cmd.c_str(); *p && a_isspace(*p); p++);

		if (!strn_cmp("if ", p, 3))
		{
			cl = find_end(trig, cl);
		}
		else if (!strn_cmp("elseif ", p, 7))
		{
			if (process_if(p + 7, go, sc, trig, type))
			{
				GET_TRIG_DEPTH(trig)++;
			}
			else
			{
				cl = find_else_end(trig, cl, go, sc, type);
			}

			break;
		}
		else if (!strn_cmp("else", p, 4))
		{
			GET_TRIG_DEPTH(trig)++;
			break;
		}
		else if (!strn_cmp("end", p, 3))
		{
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl)
	{
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
cmdlist_element::shared_ptr find_done(TRIG_DATA * trig, cmdlist_element::shared_ptr cl)
{
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	while ((cl = cl ? cl->next : cl) != NULL)
	{
		for (p = cl->cmd.c_str(); *p && isspace(*reinterpret_cast<const unsigned char*>(p)); p++);

		if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7) || !strn_cmp("foreach ", p, 8))
		{
			cl = find_done(trig, cl);
		}
		else if (!strn_cmp("done", p, 4))
		{
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl)
	{
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
cmdlist_element::shared_ptr find_case(TRIG_DATA * trig, cmdlist_element::shared_ptr cl, void *go, SCRIPT_DATA * sc, int type, const char *cond)
{
	char result[MAX_INPUT_LENGTH];
	const char *p = nullptr;
#ifdef DG_CODE_ANALYZE
	const char *cmd = cl ? cl->cmd.c_str() : "<NULL>";
#endif

	eval_expr(cond, result, go, sc, trig, type);

	while ((cl = cl ? cl->next : cl) != NULL)
	{
		for (p = cl->cmd.c_str(); *p && isspace(*reinterpret_cast<const unsigned char*>(p)); p++);

		if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7) || !strn_cmp("foreach ", p, 8))
		{
			cl = find_done(trig, cl);
		}
		else if (!strn_cmp("case ", p, 5))
		{
			char *tmpbuf = (char *)malloc(MAX_STRING_LENGTH);
			eval_op("==", result, p + 5, tmpbuf, go, sc, trig);
			if (*tmpbuf && *tmpbuf != '0')
			{
				free(tmpbuf);
				break;
			}
			free(tmpbuf);
		}
		else if (!strn_cmp("default", p, 7))
		{
			break;
		}
		else if (!strn_cmp("done", p, 4))
		{
			break;
		}
	}

#ifdef DG_CODE_ANALYZE
	if (!cl)
	{
		sprintf(buf, "closing 'done' not found for '%s'", cmd);
		trig_log(trig, buf);
	}
#endif

	return cl;
}

// processes any 'wait' commands in a trigger
void process_wait(void *go, TRIG_DATA * trig, int type, char *cmd, const cmdlist_element::shared_ptr& cl)
{
	char *arg;
	struct wait_event_data *wait_event_obj;
	long time = 0, hr, min, ntime;
	char c;

	extern TIME_INFO_DATA time_info;

	if (trig->get_attach_type() == MOB_TRIGGER
		&& IS_SET(GET_TRIG_TYPE(trig), MTRIG_DEATH))
	{
		sprintf(buf,
			"&YВНИМАНИЕ&G Используется wait в DEATH триггере '%s' (VNUM=%d).",
			GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig));
		mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
		sprintf(buf, "&GКод триггера после wait выполнен НЕ БУДЕТ!");
		mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
	}

	arg = any_one_arg(cmd, buf);
	skip_spaces(&arg);

	if (!*arg)
	{
		sprintf(buf2, "wait w/o an arg: '%s'", cl->cmd.c_str());
		trig_log(trig, buf2);
	}
	else if (!strn_cmp(arg, "until ", 6))  	// valid forms of time are 14:30 and 1430
	{
		if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
			min += (hr * 60);
		else
			min = (hr % 100) + ((hr / 100) * 60);

		// calculate the pulse of the day of "until" time
		ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

		// calculate pulse of day of current time
		time = (GlobalObjects::heartbeat().global_pulse_number() % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))
			+ (time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

		if (time >= ntime)	// adjust for next day
			time = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - time + ntime;
		else
			time = ntime - time;
	}
	else
	{
		if (sscanf(arg, "%ld %c", &time, &c) == 2)
		{
			if (c == 't')
				time *= PULSES_PER_MUD_HOUR;
			else if (c == 's')
				time *= PASSES_PER_SEC;
		}
	}

	CREATE(wait_event_obj, 1);
	wait_event_obj->trigger = trig;
	wait_event_obj->go = go;
	wait_event_obj->type = type;

	if (GET_TRIG_WAIT(trig))
	{
		trig_log(trig, "Wait structure already allocated for trigger");
	}

	GET_TRIG_WAIT(trig) = add_event(time, trig_wait_event, wait_event_obj);
	trig->curr_state = cl->next;
}

// processes a script set command
void process_set(SCRIPT_DATA* /*sc*/, TRIG_DATA * trig, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

	value = two_arguments(cmd, arg, name);

	skip_spaces(&value);

	if (!*name)
	{
		sprintf(buf2, "set w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), name, value, 0);
}

// processes a script eval command
void process_eval(void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
	char result[MAX_INPUT_LENGTH], *expr;

	expr = two_arguments(cmd, arg, name);

	skip_spaces(&expr);

	if (!*name)
	{
		sprintf(buf2, "eval w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	eval_expr(expr, result, go, sc, trig, type);
	add_var_cntx(&GET_TRIG_VARS(trig), name, result, 0);
}

// script attaching a trigger to something
void process_attach(void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
	char result[MAX_INPUT_LENGTH], *id_p;
	TRIG_DATA *newtrig;
	CHAR_DATA *c = NULL;
	OBJ_DATA *o = NULL;
	ROOM_DATA *r = NULL;
	long trignum;

	id_p = two_arguments(cmd, arg, trignum_s);
	skip_spaces(&id_p);

	if (!*trignum_s)
	{
		sprintf(buf2, "attach w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!id_p || !*id_p || atoi(id_p + 1) == 0)
	{
		sprintf(buf2, "attach invalid id(1) arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	// parse and locate the id specified
	eval_expr(id_p, result, go, sc, trig, type);

	c = get_char(id_p);
	if (!c)
	{
		o = get_obj(id_p);
		if (!o)
		{
			r = get_room(id_p);
			if (!r)
			{
				sprintf(buf2, "attach invalid id arg(3): '%s'", cmd);
				trig_log(trig, buf2);
				return;
			}
		}
	}

	// locate and load the trigger specified
	trignum = real_trigger(atoi(trignum_s));
	if (trignum >= 0
		&& (((c) && trig_index[trignum]->proto->get_attach_type() != MOB_TRIGGER)
			|| ((o) && trig_index[trignum]->proto->get_attach_type() != OBJ_TRIGGER)
			|| ((r) && trig_index[trignum]->proto->get_attach_type() != WLD_TRIGGER)))
	{
		sprintf(buf2, "attach trigger : '%s' invalid attach_type: %s expected %s", trignum_s,
			attach_name[(int)trig_index[trignum]->proto->get_attach_type()],
			attach_name[(c ? 0 : (o ? 1 : (r ? 2 : 3)))]);
		trig_log(trig, buf2);
		return;
	}

	if (trignum < 0 || !(newtrig = read_trigger(trignum)))
	{
		sprintf(buf2, "attach invalid trigger: '%s'", trignum_s);
		trig_log(trig, buf2);
		return;
	}

	if (c)
	{
		if (add_trigger(SCRIPT(c).get(), newtrig, -1))
		{
			add_trig_to_owner(trig_index[trig->get_rnum()]->vnum, trig_index[trignum]->vnum, GET_MOB_VNUM(c));
		}
		else
		{
			extract_trigger(newtrig);
		}

		return;
	}

	if (o)
	{
		if (add_trigger(o->get_script().get(), newtrig, -1))
		{
			add_trig_to_owner(trig_index[trig->get_rnum()]->vnum, trig_index[trignum]->vnum, GET_OBJ_VNUM(o));
		}
		else
		{
			extract_trigger(newtrig);
		}
		return;
	}

	if (r)
	{
		if (add_trigger(SCRIPT(r).get(), newtrig, -1))
		{
			add_trig_to_owner(trig_index[trig->get_rnum()]->vnum, trig_index[trignum]->vnum, r->number);
		}
		else
		{
			extract_trigger(newtrig);
		}
		return;
	}

	return;
}

// script detaching a trigger from something
TRIG_DATA *process_detach(void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
	char result[MAX_INPUT_LENGTH], *id_p;
	CHAR_DATA *c = NULL;
	OBJ_DATA *o = NULL;
	ROOM_DATA *r = NULL;
	TRIG_DATA *retval = trig;

	id_p = two_arguments(cmd, arg, trignum_s);
	skip_spaces(&id_p);

	if (!*trignum_s)
	{
		sprintf(buf2, "detach w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return retval;
	}

	if (!id_p || !*id_p || atoi(id_p + 1) == 0)
	{
		sprintf(buf2, "detach invalid id arg(1): '%s'", cmd);
		trig_log(trig, buf2);
		return retval;
	}

	// parse and locate the id specified
	eval_expr(id_p, result, go, sc, trig, type);

	c = get_char(id_p);
	if (!c)
	{
		o = get_obj(id_p);
		if (!o)
		{
			r = get_room(id_p);
			if (!r)
			{
				sprintf(buf2, "detach invalid id arg(3): '%s'", cmd);
				trig_log(trig, buf2);
				return retval;
			}
		}
	}

	if (c && SCRIPT(c)->has_triggers())
	{
		SCRIPT(c)->remove_trigger(trignum_s, retval);

		return retval;
	}

	if (o && o->get_script()->has_triggers())
	{
		o->get_script()->remove_trigger(trignum_s, retval);

		return retval;
	}

	if (r && SCRIPT(r)->has_triggers())
	{
		SCRIPT(r)->remove_trigger(trignum_s, retval);

		return retval;
	}

	return retval;
}

/* script run a trigger for something
   return TRUE   - trigger find and runned
		  FALSE  - trigger not runned
*/
int process_run(void *go, SCRIPT_DATA ** sc, TRIG_DATA ** trig, int type, char *cmd, int *retval)
{
	char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH], *name, *cname;
	char result[MAX_INPUT_LENGTH], *id_p;
	TRIG_DATA *runtrig = NULL;
	//	SCRIPT_DATA *runsc = NULL;
	struct trig_var_data *vd;
	CHAR_DATA *c = NULL;
	OBJ_DATA *o = NULL;
	ROOM_DATA *r = NULL;
	void *trggo = NULL;
	int trgtype = 0, num = 0;
	bool is_string = false;

	id_p = two_arguments(cmd, arg, trignum_s);
	skip_spaces(&id_p);

	if (!*trignum_s)
	{
		sprintf(buf2, "run w/o an arg: '%s'", cmd);
		trig_log(*trig, buf2);
		return (FALSE);
	}

	if (!id_p || !*id_p || atoi(id_p + 1) == 0)
	{
		sprintf(buf2, "run invalid id arg(1): '%s'", cmd);
		trig_log(*trig, buf2);
		return (FALSE);
	}

	// parse and locate the id specified
	eval_expr(id_p, result, go, *sc, *trig, type);

	c = get_char(id_p);
	if (!c)
	{
		o = get_obj(id_p);
		if (!o)
		{
			r = get_room(id_p);
			if (!r)
			{
				sprintf(buf2, "run invalid id arg(3): '%s'", cmd);
				trig_log(*trig, buf2);
				return (FALSE);
			}
		}
	}

	name = trignum_s;
	if ((cname = strchr(name, '.')) || (!a_isdigit(*name)))
	{
		is_string = true;
		if (cname)
		{
			*cname = '\0';
			num = atoi(name);
			name = ++cname;
		}
	}
	else
	{
		num = atoi(name);
	}

	if (c && SCRIPT(c)->has_triggers())
	{
		runtrig = SCRIPT(c)->trig_list.find(is_string, name, num);
		trgtype = MOB_TRIGGER;
		trggo = (void *)c;
	}
	else if (o && o->get_script()->has_triggers())
	{
		runtrig = o->get_script()->trig_list.find(is_string, name, num);
		trgtype = OBJ_TRIGGER;
		trggo = (void *)o;
	}
	else if (r && SCRIPT(r)->has_triggers())
	{
		runtrig = SCRIPT(r)->trig_list.find(is_string, name, num);
		trgtype = WLD_TRIGGER;
		trggo = (void *)r;
	};

	if (!runtrig)
	{
		return (FALSE);
	}

	// copy variables
	if (*trig && runtrig)
	{
		for (vd = GET_TRIG_VARS(*trig); vd; vd = vd->next)
		{
			if (vd->context)
			{
				sprintf(buf2, "Local variable %s with nonzero context %ld", vd->name, vd->context);
				trig_log(*trig, buf2);
			}
			add_var_cntx(&GET_TRIG_VARS(runtrig), vd->name, vd->value, 0);
		}
	}

	if (!GET_TRIG_DEPTH(runtrig))
	{
		*retval = script_driver(trggo, runtrig, trgtype, TRIG_NEW);
	}

	//TODO: Why only for char?
	if (go && type == MOB_TRIGGER && reinterpret_cast<CHAR_DATA *>(go)->purged())
	{
		*sc = NULL;
		*trig = NULL;
		return (FALSE);
	}

	runtrig = nullptr;
	//TODO: How that possible?
	if (!go || (type == MOB_TRIGGER ? SCRIPT((CHAR_DATA *)go).get() :
		type == OBJ_TRIGGER ? ((OBJ_DATA *)go)->get_script().get() :
		type == WLD_TRIGGER ? SCRIPT((ROOM_DATA *)go).get() : nullptr) != *sc)
	{
		*sc = NULL;
		*trig = NULL;
	}
	else
	{
		TriggersList* triggers_list = nullptr;
		switch (type)
		{
		case MOB_TRIGGER:
			triggers_list = &SCRIPT((CHAR_DATA *)go)->trig_list;
			break;

		case OBJ_TRIGGER:
			triggers_list = &((OBJ_DATA *)go)->get_script()->trig_list;
			break;

		case WLD_TRIGGER:
			triggers_list = &SCRIPT((ROOM_DATA *)go)->trig_list;
			break;
		}

		if (triggers_list
			&& triggers_list->has_trigger(*trig))
		{
			runtrig = *trig;
		}
	}

	*trig = runtrig;

	return (TRUE);
}

ROOM_DATA *dg_room_of_obj(OBJ_DATA * obj)
{
	if (obj->get_in_room() > NOWHERE)
	{
		return world[obj->get_in_room()];
	}

	if (obj->get_carried_by())
	{
		return world[obj->get_carried_by()->in_room];
	}

	if (obj->get_worn_by())
	{
		return world[obj->get_worn_by()->in_room];
	}

	if (obj->get_in_obj())
	{
		return dg_room_of_obj(obj->get_in_obj());
	}

	return nullptr;
}


// create a UID variable from the id number
void makeuid_var(void *go, SCRIPT_DATA * sc, TRIG_DATA * trig, int type, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
	char result[MAX_INPUT_LENGTH], *uid_p;
	char uid[MAX_INPUT_LENGTH];

	uid_p = two_arguments(cmd, arg, varname);
	skip_spaces(&uid_p);

	if (!*varname)
	{
		sprintf(buf2, "makeuid w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!uid_p || !*uid_p || atoi(uid_p + 1) == 0)
	{
		sprintf(buf2, "makeuid invalid id arg: '%s'", cmd);
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
void calcuid_var(void* go, SCRIPT_DATA* /*sc*/, TRIG_DATA * trig, int type, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
	char *t, vnum[MAX_INPUT_LENGTH], what[MAX_INPUT_LENGTH];
	char uid[MAX_INPUT_LENGTH], count[MAX_INPUT_LENGTH];
	char uid_type;
	int result = -1;

	t = two_arguments(cmd, arg, varname);
	three_arguments(t, vnum, what, count);

	if (!*varname)
	{
		sprintf(buf2, "calcuid w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*vnum || (result = atoi(vnum)) == 0)
	{
		sprintf(buf2, "calcuid invalid VNUM arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*what)
	{
		sprintf(buf2, "calcuid exceed TYPE arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	int count_num = 0;
	if (*count)
	{
		count_num = atoi(count) - 1;	//В dg индексация с 1
		if (count_num < 0)
		{
			//Произойдет, если в dg пришел индекс 0 (ошибка)
			sprintf(buf2, "calcuid invalid count: '%s'", count);
			trig_log(trig, buf2);
			return;
		}
	}

	if (!str_cmp(what, "room"))
	{
		uid_type = UID_ROOM;
		result = find_room_uid(result);
	}
	else if (!str_cmp(what, "mob"))
	{
		uid_type = UID_CHAR;
		result = find_char_vnum(result, count_num);
	}
	else if (!str_cmp(what, "obj"))
	{
		uid_type = UID_OBJ;
		result = find_obj_by_id_vnum__calcuid(result, count_num, type, go);
	}
	else
	{
		sprintf(buf2, "calcuid unknown TYPE arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (result <= -1)
	{
		sprintf(buf2, "calcuid target not found vnum: %s, count: %d.", vnum, count_num + 1);
		trig_log(trig, buf2);

		*uid = '\0';

		return;
	}

	sprintf(uid, "%c%d", uid_type, result);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

/*
 * Поиск чаров с записью в переменную UID-а в случае онлайна человека
 * Возвращает в указанную переменную UID первого PC, с именем которого
 * совпадает аргумент
 */
void charuid_var(void* /*go*/, SCRIPT_DATA* /*sc*/, TRIG_DATA * trig, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
	char who[MAX_INPUT_LENGTH], uid[MAX_INPUT_LENGTH];
	char uid_type = UID_CHAR;

	int result = -1;

	three_arguments(cmd, arg, varname, who);

	if (!*varname)
	{
		sprintf(buf2, "charuid w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*who)
	{
		sprintf(buf2, "charuid name is missing: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	for (const auto tch : character_list)
	{
		if (IS_NPC(tch)
			|| !HERE(tch)
			|| (*who
				&& !isname(who, GET_NAME(tch))))
		{
			continue;
		}

		if (IN_ROOM(tch) != NOWHERE)
		{
			result = GET_ID(tch);
		}
	}

	if (result <= -1)
	{
		sprintf(buf2, "charuid target not found, name: '%s'", who);
		trig_log(trig, buf2);
		*uid = '\0';
		return;
	}

	sprintf(uid, "%c%d", uid_type, result);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

// * Поиск мобов для calcuidall_var.
bool find_all_char_vnum(long n, char *str)
{
	int count = 0;
	for (const auto ch : character_list)
	{
		if (n == GET_MOB_VNUM(ch) && ch->in_room != NOWHERE && count < 25)
		{
			snprintf(str + strlen(str), MAX_INPUT_LENGTH, "%c%ld ", UID_CHAR, GET_ID(ch));
			++count;
		}
	}

	return count ? true : false;
}

// * Поиск предметов для calcuidall_var.
bool find_all_obj_vnum(long n, char *str)
{
	int count = 0;

	const int LIMIT = 25;
	world_objects.foreach_with_vnum(n, [&](const OBJ_DATA::shared_ptr& i)
	{
		if (count < LIMIT)
		{
			snprintf(str + strlen(str), MAX_INPUT_LENGTH, "%c%ld ", UID_OBJ, i->get_id());
			++count;
		}
	});

	return count ? true : false;
}

// * Копи-паст с calcuid_var для возврата строки со всеми найденными уидами мобов/предметов (до 25ти вхождений).
void calcuidall_var(void* /*go*/, SCRIPT_DATA* /*sc*/, TRIG_DATA * trig, int/* type*/, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
	char *t, vnum[MAX_INPUT_LENGTH], what[MAX_INPUT_LENGTH];
	char uid[MAX_INPUT_LENGTH];
	int result = -1;

	uid[0] = '\0';
	t = two_arguments(cmd, arg, varname);
	two_arguments(t, vnum, what);

	if (!*varname)
	{
		sprintf(buf2, "calcuidall w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*vnum || (result = atoi(vnum)) == 0)
	{
		sprintf(buf2, "calcuidall invalid VNUM arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!*what)
	{
		sprintf(buf2, "calcuidall exceed TYPE arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!str_cmp(what, "mob"))
	{
		result = find_all_char_vnum(result, uid);
	}
	else if (!str_cmp(what, "obj"))
	{
		result = find_all_obj_vnum(result, uid);
	}
	else
	{
		sprintf(buf2, "calcuidall unknown TYPE arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!result)
	{
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
int process_return(TRIG_DATA * trig, char *cmd)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	two_arguments(cmd, arg1, arg2);

	if (!*arg2)
	{
		sprintf(buf2, "return w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return 1;
	}

	return atoi(arg2);
}


/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
void process_unset(SCRIPT_DATA * sc, TRIG_DATA * trig, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var)
	{
		sprintf(buf2, "unset w/o an arg: '%s'", cmd);
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
void process_remote(SCRIPT_DATA * sc, TRIG_DATA * trig, char *cmd)
{
	struct trig_var_data *vd;
	SCRIPT_DATA *sc_remote = NULL;
	char *line, *var, *uid_p;
	char arg[MAX_INPUT_LENGTH];
	long uid, context;
	ROOM_DATA *room;
	CHAR_DATA *mob;
	OBJ_DATA *obj;

	line = any_one_arg(cmd, arg);
	two_arguments(line, buf, buf2);
	var = buf;
	uid_p = buf2;
	skip_spaces(&var);
	skip_spaces(&uid_p);


	if (!*buf || !*buf2)
	{
		sprintf(buf2, "remote: invalid arguments '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	// find the locally owned variable
	vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);
	if (!vd)
		vd = find_var_cntx(&(sc->global_vars), var, sc->context);

	if (!vd)
	{
		sprintf(buf2, "local var '%s' not found in remote call", buf);
		trig_log(trig, buf2);
		return;
	}

	// find the target script from the uid number
	uid = atoi(buf2 + 1);
	if (uid <= 0)
	{
		sprintf(buf, "remote: illegal uid '%s'", buf2);
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

	if ((room = get_room(buf2)))
	{
		sc_remote = SCRIPT(room).get();
	}
	else if ((mob = get_char(buf2)))
	{
		sc_remote = SCRIPT(mob).get();
		if (!IS_NPC(mob))
		{
			context = 0;
		}
	}
	else if ((obj = get_obj(buf2)))
	{
		sc_remote = obj->get_script().get();
	}
	else
	{
		sprintf(buf, "remote: uid '%ld' invalid", uid);
		trig_log(trig, buf);
		return;
	}

	if (sc_remote == NULL)
	{
		return;		// no script to assign
	}

	add_var_cntx(&(sc_remote->global_vars), vd->name, vd->value, context);
}

/*
 * command-line interface to rdelete
 * named vdelete so people didn't think it was to delete rooms
 */
void do_vdelete(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	//  struct trig_var_data *vd, *vd_prev=NULL;
	SCRIPT_DATA *sc_remote = NULL;
	char *var, *uid_p;
	long uid; //, context;
	ROOM_DATA *room;
	CHAR_DATA *mob;
	OBJ_DATA *obj;

	argument = two_arguments(argument, buf, buf2);
	var = buf;
	uid_p = buf2;
	skip_spaces(&var);
	skip_spaces(&uid_p);


	if (!*buf || !*buf2)
	{
		send_to_char("Usage: vdelete <variablename> <id>\r\n", ch);
		return;
	}


	// find the target script from the uid number
	uid = atoi(buf2 + 1);
	if (uid <= 0)
	{
		send_to_char("vdelete: illegal id specified.\r\n", ch);
		return;
	}


	if ((room = get_room(buf2)))
	{
		sc_remote = SCRIPT(room).get();
	}
	else if ((mob = get_char(buf2)))
	{
		sc_remote = SCRIPT(mob).get();
	}
	else if ((obj = get_obj(buf2)))
	{
		sc_remote = obj->get_script().get();
	}
	else
	{
		send_to_char("vdelete: cannot resolve specified id.\r\n", ch);
		return;
	}

	if ((sc_remote == NULL) || (sc_remote->global_vars == NULL))
	{
		send_to_char("That id represents no global variables.\r\n", ch);
		return;
	}

	// find the global
	if (remove_var_cntx(&(sc_remote->global_vars), var, 0))
	{
		send_to_char("Deleted.\r\n", ch);
	}
	else
	{
		send_to_char("That variable cannot be located.\r\n", ch);
	}
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
void process_rdelete(SCRIPT_DATA * sc, TRIG_DATA * trig, char *cmd)
{
	//  struct trig_var_data *vd, *vd_prev=NULL;
	SCRIPT_DATA *sc_remote = NULL;
	char *line, *var, *uid_p;
	char arg[MAX_INPUT_LENGTH];
	long uid; //, context;
	ROOM_DATA *room;
	CHAR_DATA *mob;
	OBJ_DATA *obj;

	line = any_one_arg(cmd, arg);
	two_arguments(line, buf, buf2);
	var = buf;
	uid_p = buf2;
	skip_spaces(&var);
	skip_spaces(&uid_p);



	if (!*buf || !*buf2)
	{
		sprintf(buf2, "rdelete: invalid arguments '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}


	// find the target script from the uid number
	uid = atoi(buf2 + 1);
	if (uid <= 0)
	{
		sprintf(buf, "rdelete: illegal uid '%s'", buf2);
		trig_log(trig, buf);
		return;
	}


	if ((room = get_room(buf2)))
	{
		sc_remote = SCRIPT(room).get();
	}
	else if ((mob = get_char(buf2)))
	{
		sc_remote = SCRIPT(mob).get();
	}
	else if ((obj = get_obj(buf2)))
	{
		sc_remote = obj->get_script().get();
	}
	else
	{
		sprintf(buf, "remote: uid '%ld' invalid", uid);
		trig_log(trig, buf);
		return;
	}

	if (sc_remote == NULL)
	{
		return;		// no script to delete a trigger from
	}
	if (sc_remote->global_vars == NULL)
	{
		return;		// no script globals
	}

	// find the global
	remove_var_cntx(&(sc_remote->global_vars), var, sc->context);
}


// * makes a local variable into a global variable
void process_global(SCRIPT_DATA * sc, TRIG_DATA * trig, char *cmd, long id)
{
	struct trig_var_data *vd;
	char arg[MAX_INPUT_LENGTH], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var)
	{
		sprintf(buf2, "global w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);

	if (!vd)
	{
		sprintf(buf2, "local var '%s' not found in global call", var);
		trig_log(trig, buf2);
		return;
	}

	add_var_cntx(&(sc->global_vars), vd->name, vd->value, id);
	remove_var_cntx(&GET_TRIG_VARS(trig), vd->name, 0);
}

// * makes a local variable into a world variable
void process_worlds(SCRIPT_DATA* /*sc*/, TRIG_DATA * trig, char *cmd, long id)
{
	struct trig_var_data *vd;
	char arg[MAX_INPUT_LENGTH], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var)
	{
		sprintf(buf2, "worlds w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	vd = find_var_cntx(&GET_TRIG_VARS(trig), var, 0);

	if (!vd)
	{
		sprintf(buf2, "local var '%s' not found in worlds call", var);
		trig_log(trig, buf2);
		return;
	}

	add_var_cntx(&(worlds_vars), vd->name, vd->value, id);
	remove_var_cntx(&GET_TRIG_VARS(trig), vd->name, 0);
}

// set the current context for a script
void process_context(SCRIPT_DATA * sc, TRIG_DATA * trig, char *cmd)
{
	char arg[MAX_INPUT_LENGTH], *var;

	var = any_one_arg(cmd, arg);

	skip_spaces(&var);

	if (!*var)
	{
		sprintf(buf2, "context w/o an arg: '%s'", cmd);
		trig_log(trig, buf2);
		return;
	}

	sc->context = atol(var);
}

void extract_value(SCRIPT_DATA* /*sc*/, TRIG_DATA * trig, char* cmd)
{
	char buf2[MAX_INPUT_LENGTH];
	char *buf3;
	char to[128];
	int num;

	buf3 = any_one_arg(cmd, buf);
	half_chop(buf3, buf2, buf);
	strcpy(to, buf2);

	num = atoi(buf);
	if (num < 1)
	{
		trig_log(trig, "extract number < 1!");
		return;
	}

	half_chop(buf, buf3, buf2);

	while (num > 0)
	{
		half_chop(buf2, buf, buf2);
		num--;
	}

	add_var_cntx(&GET_TRIG_VARS(trig), to, buf, 0);
}

//  This is the core driver for scripts.
//  define this if you want measure time of you scripts
#define TIMED_SCRIPT

#ifdef TIMED_SCRIPT
int timed_script_driver(void *go, TRIG_DATA* trig, int type, int mode);

int script_driver(void *go, TRIG_DATA * trig, int type, int mode)
{
	struct timeval start, stop, result;
/*	std::string start_string_trig = "First Line";
	std::string finish_string_trig = "<Last line is undefined because it is dangerous to obtain it>";
	if (trig->curr_state)
	{
		start_string_trig = trig->curr_state->cmd;
	}
*/
	CharacterLinkDrop = false;
	const auto vnum = GET_TRIG_VNUM(trig);
	gettimeofday(&start, NULL);
	const auto return_code = timed_script_driver(go, trig, type, mode);

	gettimeofday(&stop, NULL);
	timediff(&result, &stop, &start);

	if (result.tv_sec > 0 || result.tv_usec >= MAX_TRIG_USEC)
	{
		/*
		 * We cannot get access to trig fields here because the last command
		 * might be "detach". In this case trigger will be deleted. Therefore
		 * we will get the core dump here in the best case.
		 *
		if (trig->curr_state)
		{
			finish_string_trig = trig->curr_state->cmd;
		}
		*/
		snprintf(buf, MAX_STRING_LENGTH, "[TrigVNum: %d] : ", vnum);
		const auto current_buffer_length = strlen(buf);
/*		snprintf(buf + current_buffer_length, MAX_STRING_LENGTH - current_buffer_length,
			"work time overflow %ld sec. %ld us.\r\n StartString: %s\r\nFinishLine: %s",
			result.tv_sec, result.tv_usec, start_string_trig.c_str(), finish_string_trig.c_str());
*/
		snprintf(buf + current_buffer_length, MAX_STRING_LENGTH - current_buffer_length,
			"work time overflow %ld sec. %ld us.",
			result.tv_sec, result.tv_usec);
		mudlog(buf, BRF, -1, ERRLOG, TRUE);
	};
	// Stop time
	return return_code;
}

void do_dg_add_ice_currency(void* /*go*/, SCRIPT_DATA* /*sc*/, TRIG_DATA* trig, int/* script_type*/, char *cmd)
{
	CHAR_DATA *ch = NULL;
	int value;
	char junk[MAX_INPUT_LENGTH];
	char charname[MAX_INPUT_LENGTH], value_c[MAX_INPUT_LENGTH];

	half_chop(cmd, junk, cmd);
	half_chop(cmd, charname, cmd);
	half_chop(cmd, value_c, cmd);


	if (!*charname || !*value_c)
	{
		sprintf(buf2, "dg_addicecurrency usage: <target> <value>");
		trig_log(trig, buf2);
		return;
	}

	value = atoi(value_c);
	// locate the target
	ch = get_char(charname);
	if (!ch)
	{
		sprintf(buf2, "dg_addicecurrency: cannot locate target!");
		trig_log(trig, buf2);
		return;
	}
	ch->add_ice_currency(value);
}

int timed_script_driver(void *go, TRIG_DATA * trig, int type, int mode)
#else
int script_driver(void *go, TRIG_DATA * trig, int type, int mode)
#endif
{
	static int depth = 0;
	int ret_val = 1, stop = FALSE;
	char cmd[MAX_INPUT_LENGTH];
	SCRIPT_DATA *sc = 0;
	unsigned long loops = 0;
	TRIG_DATA *prev_trig;

	void mob_command_interpreter(CHAR_DATA* ch, char* argument);
	void obj_command_interpreter(OBJ_DATA* obj, char* argument);
	void wld_command_interpreter(ROOM_DATA* room, char* argument);

	if (depth > MAX_SCRIPT_DEPTH)
	{
		trig_log(trig, "Triggers recursed beyond maximum allowed depth.");
		return ret_val;
	}

	switch (type)
	{
	case MOB_TRIGGER:
		sc = SCRIPT((CHAR_DATA *)go).get();
		break;

	case OBJ_TRIGGER:
		sc = ((OBJ_DATA *)go)->get_script().get();
		break;

	case WLD_TRIGGER:
		sc = SCRIPT((ROOM_DATA *)go).get();
		break;
	}

	if (sc->is_purged())
	{
		return ret_val;
	}

	prev_trig = cur_trig;
	cur_trig = trig;

	depth++;
	last_trig_vnum = GET_TRIG_VNUM(trig);

	if (mode == TRIG_NEW)
	{
		GET_TRIG_DEPTH(trig) = 1;
		GET_TRIG_LOOPS(trig) = 0;
		sc->context = 0;
	}

	for (auto cl = (mode == TRIG_NEW) ? *trig->cmdlist : trig->curr_state; !stop && cl && trig && GET_TRIG_DEPTH(trig); cl = cl ? cl->next : cl)  	//log("Drive go <%s>",cl->cmd);
	{
		if (CharacterLinkDrop)
		{
			sprintf(buf, "[TrigVnum: %d] Character in LinkDrop in 'Drive go'.", last_trig_vnum);
			mudlog(buf, BRF, -1, ERRLOG, TRUE);
			break;
		}
		const char* p = nullptr;
		for (p = cl->cmd.c_str(); !stop && trig && *p && a_isspace(*p); p++);

		if (*p == '*')	// comment
		{
			continue;
		}
		else if (!strn_cmp(p, "if ", 3))
		{
			if (process_if(p + 3, go, sc, trig, type))
			{
				GET_TRIG_DEPTH(trig)++;
			}
			else
			{
				cl = find_else_end(trig, cl, go, sc, type);
			}
			if (CharacterLinkDrop)
			{
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);
				break;
			}
		}
		else if (!strn_cmp("elseif ", p, 7) || !strn_cmp("else", p, 4))
		{
			cl = find_end(trig, cl);
			GET_TRIG_DEPTH(trig)--;
			if (CharacterLinkDrop)
			{
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);
				break;
			}
		}
		else if (!strn_cmp("while ", p, 6))
		{
			const auto temp = find_done(trig, cl);
			if (process_if(p + 6, go, sc, trig, type))
			{
				if (temp)
				{
					temp->original = cl;
				}
			}
			else
			{
				cl = temp;
				loops = 0;
			}
			if (CharacterLinkDrop)
			{
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);
				break;
			}
		}
		else if (!strn_cmp("foreach ", p, 8))
		{
			const auto temp = find_done(trig, cl);
			if (process_foreach_begin(p + 8, go, sc, trig, type))
			{
				if (temp)
				{
					temp->original = cl;
				}
			}
			else
			{
				cl = temp;
				loops = 0;
			}
			if (CharacterLinkDrop)
			{
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);
				break;
			}
		}
		else if (!strn_cmp("switch ", p, 7))
		{
			cl = find_case(trig, cl, go, sc, type, p + 7);
			if (CharacterLinkDrop)
			{
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);
				break;
			}
		}
		else if (!strn_cmp("end", p, 3))
		{
			GET_TRIG_DEPTH(trig)--;
		}
		else if (!strn_cmp("done", p, 4))
		{
			if (cl->original)
			{
				auto orig_cmd = cl->original->cmd.c_str();
				while (*orig_cmd && a_isspace(*orig_cmd))
				{
					orig_cmd++;
				}

				if ((*orig_cmd == 'w' && process_if(orig_cmd + 6, go, sc, trig, type))
					|| (*orig_cmd == 'f' && process_foreach_done(orig_cmd + 8, go, sc, trig, type)))
				{
					cl = cl->original;
					loops++;
					GET_TRIG_LOOPS(trig)++;
					if (loops == 30)
					{
						snprintf(buf2, MAX_STRING_LENGTH, "wait 1");
						process_wait(go, trig, type, buf2, cl);
						depth--;
						cur_trig = prev_trig;
						return ret_val;
					}

					if (GET_TRIG_LOOPS(trig) == 300)
					{
						trig_log(trig, "looping 300 times.", LGH);
					}

					if (GET_TRIG_LOOPS(trig) == 1000)
					{
						trig_log(trig, "looping 1000 times.", DEF);
					}
				}
			}
		}
		else if (!strn_cmp("break", p, 5))
		{
			cl = find_done(trig, cl);
		}
		else if (!strn_cmp("case", p, 4))  	// Do nothing, this allows multiple cases to a single instance
		{
		}
		else
		{
			var_subst(go, sc, trig, type, p, cmd);
			if (CharacterLinkDrop)
			{
				sprintf(buf, "[TrigVnum: %d] Character in LinkDrop.\r\n", last_trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);
				break;
			}
			if (!strn_cmp(cmd, "eval ", 5))
			{
				process_eval(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "nop ", 4))
			{
				;	// nop: do nothing
			}
			else if (!strn_cmp(cmd, "extract ", 8))
			{
				extract_value(sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "makeuid ", 8))
			{
				makeuid_var(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "calcuid ", 8))
			{
				calcuid_var(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "calcuidall ", 11))
			{
				calcuidall_var(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "charuid ", 8))
			{
				charuid_var(go, sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "halt", 4))
			{
				break;
			}
			else if (!strn_cmp(cmd, "dg_cast ", 8))
			{
				do_dg_cast(go, sc, trig, type, cmd);
				if (type == MOB_TRIGGER && reinterpret_cast<CHAR_DATA *>(go)->purged())
				{
					depth--;
					cur_trig = prev_trig;
					return ret_val;
				}
			}
			else if (!strn_cmp(cmd, "dg_affect ", 10))
			{
				do_dg_affect(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "global ", 7))
			{
				process_global(sc, trig, cmd, sc->context);
			}
			else if (!strn_cmp(cmd, "addicecurrency ", 15))
			{
				do_dg_add_ice_currency(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "bonus ", 6))
			{
				Bonus::dg_do_bonus(cmd + 6);
			}
			else if (!strn_cmp(cmd, "worlds ", 7))
			{
				process_worlds(sc, trig, cmd, sc->context);
			}
			else if (!strn_cmp(cmd, "context ", 8))
			{
				process_context(sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "remote ", 7))
			{
				process_remote(sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "rdelete ", 8))
			{
				process_rdelete(sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "return ", 7))
			{
				ret_val = process_return(trig, cmd);
			}
			else if (!strn_cmp(cmd, "set ", 4))
			{
				process_set(sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "unset ", 6))
			{
				process_unset(sc, trig, cmd);
			}
			else if (!strn_cmp(cmd, "log ", 4))
			{
				trig_log(trig, cmd + 4);
			}
			else if (!strn_cmp(cmd, "wait ", 5))
			{
				process_wait(go, trig, type, cmd, cl);
				depth--;
				cur_trig = prev_trig;
				return ret_val;
			}
			else if (!strn_cmp(cmd, "attach ", 7))
			{
				process_attach(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "detach ", 7))
			{
				trig = process_detach(go, sc, trig, type, cmd);
			}
			else if (!strn_cmp(cmd, "run ", 4))
			{
				process_run(go, &sc, &trig, type, cmd, &ret_val);
				if (!trig || !sc)
				{
					depth--;
					cur_trig = prev_trig;
					return ret_val;
				}
				stop = ret_val;
			}
			else if (!strn_cmp(cmd, "exec ", 5))
			{
				process_run(go, &sc, &trig, type, cmd, &ret_val);
				if (!trig || !sc)
				{
					depth--;
					cur_trig = prev_trig;
					return ret_val;
				}
			}
			else if (!strn_cmp(cmd, "version", 7))
			{
				mudlog(DG_SCRIPT_VERSION, BRF, LVL_BUILDER, SYSLOG, TRUE);
			}
			else
			{
				switch (type)
				{
				case MOB_TRIGGER:
					//last_trig_vnum = GET_TRIG_VNUM(trig);
					mob_command_interpreter((CHAR_DATA *)(go), cmd);
					break;

				case OBJ_TRIGGER:
					//last_trig_vnum = GET_TRIG_VNUM(trig);
					obj_command_interpreter((OBJ_DATA *)go, cmd);
					break;

				case WLD_TRIGGER:
					//last_trig_vnum = GET_TRIG_VNUM(trig);
					wld_command_interpreter((ROOM_DATA *)go, cmd);
					break;
				}
			}

			if (sc->is_purged() || (type == MOB_TRIGGER && reinterpret_cast<CHAR_DATA *>(go)->purged()))
			{
				depth--;
				cur_trig = prev_trig;
				return ret_val;
			}
		}
	}

	if (trig)
	{
		trig->clear_var_list();
		GET_TRIG_VARS(trig) = NULL;
		GET_TRIG_DEPTH(trig) = 0;
	}

	depth--;
	cur_trig = prev_trig;
	return ret_val;
}

void do_tlist(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int first, last, nr, found = 0;
	char pagebuf[65536];

	strcpy(pagebuf, "");

	two_arguments(argument, buf, buf2);

	if (!*buf)
	{
		send_to_char("Usage: tlist <begining number or zone> [<ending number>]\r\n", ch);
		return;
	}

	first = atoi(buf);
	if (*buf2)
		last = atoi(buf2);
	else
	{
		first *= 100;
		last = first + 99;
	}

	if ((first < 0) || (first > MAX_PROTO_NUMBER) || (last < 0) || (last > MAX_PROTO_NUMBER))
	{
		sprintf(buf, "Значения должны быть между 0 и %d.\n\r", MAX_PROTO_NUMBER);
		send_to_char(buf, ch);
	}

	if (first >= last)
	{
		send_to_char("Второе значение должно быть больше первого.\n\r", ch);
		return;
	}

	if (first + 200 < last)
	{
		send_to_char("Максимальный показываемый промежуток - 200.\n\r", ch);
		return;
	}


	for (nr = 0; nr < top_of_trigt && (trig_index[nr]->vnum <= last); nr++)
	{
		if (trig_index[nr]->vnum >= first)
		{
			std::string out = "";
			if (trig_index[nr]->proto->get_attach_type() == MOB_TRIGGER)
			{
				out += "[MOB_TRIG]";
			}

			if (trig_index[nr]->proto->get_attach_type() == OBJ_TRIGGER)
			{
				out += "[OBJ_TRIG]";
			}

			if (trig_index[nr]->proto->get_attach_type() == WLD_TRIGGER)
			{
				out += "[WLD_TRIG]";
			}

			sprintf(buf, "%5d. [%5d] %s\r\nПрикрепленные триггеры: ", ++found, trig_index[nr]->vnum, trig_index[nr]->proto->get_name().c_str());
			out += buf;
			if (!owner_trig[trig_index[nr]->vnum].empty())
			{
				for (auto it = owner_trig[trig_index[nr]->vnum].begin(); it != owner_trig[trig_index[nr]->vnum].end(); ++it)
				{
					out += "[";
					std::string  out_tmp = "";
					for (const auto trigger_vnum : it->second)
					{
						sprintf(buf, " [%d]", trigger_vnum);
						out_tmp += buf;
					}

					if (it->first != -1)
					{
						out += std::to_string(it->first) + " : ";
					}

					out += out_tmp + "]";
				}

				out += "\r\n";
			}
			else
			{
				out += "Отсутствуют\r\n";
			}
			strcat(pagebuf, out.c_str());
		}
	}

	if (!found)
	{
		send_to_char("No triggers were found in those parameters.\n\r", ch);
	}
	else
	{
		page_string(ch->desc, pagebuf, TRUE);
	}
}

int real_trigger(int vnum)
{
	int rnum;

	for (rnum = 0; rnum < top_of_trigt; rnum++)
	{
		if (trig_index[rnum]->vnum == vnum)
			break;
	}

	if (rnum == top_of_trigt)
		rnum = -1;
	return (rnum);
}

void do_tstat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int vnum, rnum;
	char str[MAX_INPUT_LENGTH];

	half_chop(argument, str, argument);
	if (*str)
	{
		vnum = atoi(str);
		rnum = real_trigger(vnum);
		if (rnum < 0)
		{
			send_to_char("That vnum does not exist.\r\n", ch);
			return;
		}

		do_stat_trigger(ch, trig_index[rnum]->proto);
	}
	else
		send_to_char("Usage: tstat <vnum>\r\n", ch);
}

// read a line in from a file, return the number of chars read
int fgetline(FILE * file, char *p)
{
	int count = 0;

	do
	{
		*p = fgetc(file);
		if (*p != '\n' && !feof(file))
		{
			p++;
			count++;
		}
	} while (*p != '\n' && !feof(file));

	if (*p == '\n')
		*p = '\0';

	return count;
}

// load in a character's saved variables
void read_saved_vars(CHAR_DATA * ch)
{
	FILE *file;
	long context;
	char fn[127];
	char input_line[1024], *p;
	char varname[32], *v;
	char context_str[16], *c;

	// find the file that holds the saved variables and open it
	get_filename(GET_NAME(ch), fn, SCRIPT_VARS_FILE);
	file = fopen(fn, "r");

	// if we failed to open the file, return
	if (!file)
	{
		return;
	}

	// walk through each line in the file parsing variables
	do
	{
		if (fgetline(file, input_line) > 0)
		{
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
void save_char_vars(CHAR_DATA * ch)
{
	FILE *file;
	char fn[127];
	struct trig_var_data *vars;

	// we should never be called for an NPC, but just in case...
	if (IS_NPC(ch))
		return;

	get_filename(GET_NAME(ch), fn, SCRIPT_VARS_FILE);
	std::remove(fn);

	// make sure this char has global variables to save
	if (ch->script->global_vars == NULL)
		return;

	vars = ch->script->global_vars;

	file = fopen(fn, "wt");

	// note that currently, context will always be zero. this may change
	// in the future
	while (vars)
	{
		if (*vars->name != '-')	// don't save if it begins with -
			fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
		vars = vars->next;
	}

	fclose(file);
}

class TriggerRemoveObserverI : public TriggerEventObserver
{
public:
	TriggerRemoveObserverI(TriggersList::iterator* owner) : m_owner(owner) {}

	virtual void notify(TRIG_DATA* trigger) override;

private:
	TriggersList::iterator* m_owner;
};

void TriggerRemoveObserverI::notify(TRIG_DATA* trigger)
{
	m_owner->remove_event(trigger);
}

TriggersList::iterator::iterator(TriggersList* owner, const IteratorPosition position) : m_owner(owner), m_removed(false)
{
	m_iterator = position == BEGIN ? m_owner->m_list.begin() : m_owner->m_list.end();
	setup_observer();
}

TriggersList::iterator::iterator(const iterator& rhv) : m_owner(rhv.m_owner), m_iterator(rhv.m_iterator), m_removed(rhv.m_removed)
{
	setup_observer();
}

TriggersList::iterator::~iterator()
{
	m_owner->unregister_observer(m_observer);
}

TriggersList::TriggersList::iterator& TriggersList::iterator::operator++()
{
	if (!m_removed)
	{
		++m_iterator;
	}
	else
	{
		m_removed = false;
	}

	return *this;
}

void TriggersList::iterator::setup_observer()
{
	m_observer = std::make_shared<TriggerRemoveObserverI>(this);
	m_owner->register_observer(m_observer);
}

void TriggersList::iterator::remove_event(TRIG_DATA* trigger)
{
	if (m_iterator != m_owner->m_list.end()
		&& *m_iterator == trigger)
	{
		++m_iterator;
		m_removed = true;
	}
}

class TriggerRemoveObserver : public TriggerEventObserver
{
public:
	TriggerRemoveObserver(TriggersList* owner) : m_owner(owner) {}

	virtual void notify(TRIG_DATA* trigger) override;

private:
	TriggersList * m_owner;
};

void TriggerRemoveObserver::notify(TRIG_DATA* trigger)
{
	m_owner->remove(trigger);
}

TriggersList::TriggersList()
{
	m_observer = std::make_shared<TriggerRemoveObserver>(this);
}

TriggersList::~TriggersList()
{
	clear();
}

// Return true if trigger successfully added
// Return false if trigger with same rnum already exists and can not be added
bool TriggersList::add(TRIG_DATA* trigger, const bool to_front /*= false*/)
{
	for (const auto& i : m_list)
	{
		if (trigger->get_rnum() == i->get_rnum())
		{
			//Мы не можем здесь заменить имеющийся триггер на новый
			//т.к. имеющийся триггер может уже использоваться или быть в ожидание (wait command)
			return false;
		}
	}

	trigger_list.register_remove_observer(trigger, m_observer);

	if (to_front)
	{
		m_list.push_front(trigger);
	}
	else
	{
		m_list.push_back(trigger);
	}

	return true;
}

void TriggersList::remove(TRIG_DATA* const trigger)
{
	const auto i = std::find(m_list.begin(), m_list.end(), trigger);
	if (i != m_list.end())
	{
		remove(i);
	}
}

TriggersList::triggers_list_t::iterator TriggersList::remove(const triggers_list_t::iterator& iterator)
{
	TRIG_DATA* trigger = *iterator;
	trigger_list.unregister_remove_observer(trigger, m_observer);

	for (const auto& observer : m_iterator_observers)
	{
		observer->notify(trigger);
	}

	triggers_list_t::iterator result = m_list.erase(iterator);

	extract_trigger(trigger);

	return result;
}

void TriggersList::register_observer(const TriggerEventObserver::shared_ptr& observer)
{
	m_iterator_observers.insert(observer);
}

void TriggersList::unregister_observer(const TriggerEventObserver::shared_ptr& observer)
{
	m_iterator_observers.erase(observer);
}

TRIG_DATA* TriggersList::find(const bool by_name, const char* name, const int vnum_or_position)
{
	if (by_name)
	{
		return find_by_name(name, vnum_or_position);
	}
	else
	{
		return find_by_vnum_or_position(vnum_or_position);
	}
}

TRIG_DATA* TriggersList::find_by_name(const char* name, const int number)
{
	TRIG_DATA* result = nullptr;

	int n = 0;
	for (const auto& i : m_list)
	{
		if (isname(name, GET_TRIG_NAME(i)))
		{
			++n;
			if (n >= number)
			{
				result = i;
				break;
			}
		}
	}

	return result;
}

TRIG_DATA* TriggersList::find_by_vnum_or_position(const int vnum_or_position)
{
	TRIG_DATA* result = nullptr;

	int n = 0;
	for (const auto& i : m_list)
	{
		++n;
		if (n >= vnum_or_position)
		{
			result = i;
			break;
		}

		if (trig_index[i->get_rnum()]->vnum == vnum_or_position)
		{
			result = i;
			break;
		}
	}

	return result;
}

TRIG_DATA* TriggersList::find_by_vnum(const int vnum)
{
	TRIG_DATA* result = nullptr;

	for (const auto& i : m_list)
	{
		if (trig_index[i->get_rnum()]->vnum == vnum)
		{
			result = i;
			break;
		}
	}

	return result;
}

TRIG_DATA* TriggersList::remove_by_name(const char* name, const int number)
{
	TRIG_DATA* to_remove = find_by_name(name, number);

	if (to_remove)
	{
		remove(to_remove);
	}

	return to_remove;
}

TRIG_DATA* TriggersList::remove_by_vnum_or_position(const int vnum_or_position)
{
	TRIG_DATA* to_remove = find_by_vnum_or_position(vnum_or_position);

	if (to_remove)
	{
		remove(to_remove);
	}

	return to_remove;
}

TRIG_DATA* TriggersList::remove_by_vnum(const int vnum)
{
	TRIG_DATA* to_remove = find_by_vnum(vnum);

	if (to_remove)
	{
		remove(to_remove);
	}

	return to_remove;
}

long TriggersList::get_type() const
{
	long result = 0;
	for (const auto& i : m_list)
	{
		result |= i->get_trigger_type();
	}

	return result;
}

bool TriggersList::has_trigger(const TRIG_DATA* const trigger)
{
	for (const auto& i : m_list)
	{
		if (i == trigger)
		{
			return true;
		}
	}

	return false;
}

void TriggersList::clear()
{
	while (!m_list.empty())
	{
		remove(*m_list.begin());	// Do all stuff related to removing trigger from the list. Specifically, unregister all observers.
	}
}

std::ostream& TriggersList::dump(std::ostream& os) const
{
	bool first = true;
	for (const auto t : m_list)
	{
		if (!first)
		{
			os << ", ";
		}
		os << trig_index[t->get_rnum()]->vnum;
		first = false;
	}

	return os;
}

SCRIPT_DATA::SCRIPT_DATA() :
	types(0),
	global_vars(nullptr),
	context(0),
	m_purged(false)
{
}

// don't copy anything for now
SCRIPT_DATA::SCRIPT_DATA(const SCRIPT_DATA&) :
	types(0),
	global_vars(nullptr),
	context(0),
	m_purged(false)
{
}

SCRIPT_DATA::~SCRIPT_DATA()
{
	trig_list.clear();
	clear_global_vars();
}

int SCRIPT_DATA::remove_trigger(char *name, TRIG_DATA*& trig_addr)
{
	int num = 0;
	char *cname;

	bool string = false;
	if ((cname = strchr(name, '.')) || (!a_isdigit(*name)))
	{
		string = true;
		if (cname)
		{
			*cname = '\0';
			num = atoi(name);
			name = ++cname;
		}
	}
	else
	{
		num = atoi(name);
	}

	TRIG_DATA *address_of_removed_trigger = nullptr;
	if (string)
	{
		address_of_removed_trigger = trig_list.remove_by_name(name, num);
	}
	else
	{
		address_of_removed_trigger = trig_list.remove_by_vnum_or_position(num);
	}

	if (!address_of_removed_trigger)
	{
		return 0;	// nothing has been removed
	}

	if (address_of_removed_trigger == trig_addr)
	{
		trig_addr = nullptr;	// mark that this trigger was removed: trig_addr is not null if trigger is still in the memory
	}

	// update the script type bitvector
	types = trig_list.get_type();

	return 1;
}

int SCRIPT_DATA::remove_trigger(char *name)
{
	TRIG_DATA* dummy = nullptr;
	return remove_trigger(name, dummy);
}

void SCRIPT_DATA::clear_global_vars()
{
	for (auto i = global_vars; i;)
	{
		auto j = i;
		i = i->next;
		if (j->name)
			free(j->name);
		if (j->value)
			free(j->value);
		free(j);
	}
}

void SCRIPT_DATA::cleanup()
{
	trig_list.clear();

	clear_global_vars();
	global_vars = nullptr;
}

const char* TRIG_DATA::DEFAULT_TRIGGER_NAME = "no name";

TRIG_DATA::TRIG_DATA() :
	cmdlist(new cmdlist_element::shared_ptr()),
	narg(0),
	depth(0),
	loops(-1),
	wait_event(nullptr),
	var_list(nullptr),
	nr(NOTHING),
	attach_type(0),
	name(DEFAULT_TRIGGER_NAME),
	trigger_type(0)
{
}

TRIG_DATA::TRIG_DATA(const sh_int rnum, const char* name, const byte attach_type, const long trigger_type) :
	cmdlist(new cmdlist_element::shared_ptr()),
	narg(0),
	depth(0),
	loops(-1),
	wait_event(nullptr),
	var_list(nullptr),
	nr(rnum),
	attach_type(attach_type),
	name(name),
	trigger_type(trigger_type)
{
}

TRIG_DATA::TRIG_DATA(const sh_int rnum, std::string&& name, const byte attach_type, const long trigger_type) :
	cmdlist(new cmdlist_element::shared_ptr()),
	narg(0),
	depth(0),
	loops(-1),
	wait_event(nullptr),
	var_list(nullptr),
	nr(rnum),
	attach_type(attach_type),
	name(name),
	trigger_type(trigger_type)
{
}

TRIG_DATA::TRIG_DATA(const sh_int rnum, const char* name, const long trigger_type) : TRIG_DATA(rnum, name, 0, trigger_type)
{
}

TRIG_DATA::TRIG_DATA(const TRIG_DATA& from) :
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
	trigger_type(from.trigger_type)
{
}

void TRIG_DATA::reset()
{
	nr = NOTHING;
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

TRIG_DATA& TRIG_DATA::operator=(const TRIG_DATA& right)
{
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

void TRIG_DATA::clear_var_list()
{
	for (auto i = var_list; i;)
	{
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
