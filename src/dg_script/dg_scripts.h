/**************************************************************************
*  File: dg_scripts.h                                     Part of Bylins  *
*  Usage: header file for script structures and contstants, and           *
*         function prototypes for scripts.cpp                             *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#ifndef _DG_SCRIPTS_H_
#define _DG_SCRIPTS_H_

#include "game_skills/skills.h"
#include "structs/structs.h"
#include "utils/logger.h"
#include <optional>
#include <feats.h>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

#define DG_SCRIPT_VERSION "DG Scripts Version 0.99 Patch Level 7   12/98"

const int MOB_TRIGGER = 0;
const int OBJ_TRIGGER = 1;
const int WLD_TRIGGER = 2;

extern const char *attach_name[];
const int DG_NO_TRIG = 256;    // don't check act trigger   //

// mob trigger types //
#define MTRIG_RANDOM_GLOBAL           (1 << 0)    // check even if zone empty   //
#define MTRIG_RANDOM           (1 << 1)    // checked randomly           //
#define MTRIG_COMMAND          (1 << 2)    // character types a command  //
#define MTRIG_SPEECH           (1 << 3)    // a char says a word/phrase  //
#define MTRIG_ACT              (1 << 4)    // word or phrase sent to act //
#define MTRIG_DEATH            (1 << 5)    // character dies             //
#define MTRIG_GREET            (1 << 6)    // something enters room seen //
#define MTRIG_GREET_ALL        (1 << 7)    // anything enters room       //
#define MTRIG_ENTRY            (1 << 8)    // the mob enters a room      //
#define MTRIG_RECEIVE          (1 << 9)    // character is given obj     //
#define MTRIG_FIGHT            (1 << 10)    // each pulse while fighting  //
#define MTRIG_HITPRCNT         (1 << 11)    // fighting and below some hp //
#define MTRIG_BRIBE               (1 << 12)    // coins are given to mob     //
#define MTRIG_LOAD             (1 << 13)    // the mob is loaded          //
#define MTRIG_KILL               (1 << 14) //trig for mob's kill list
#define MTRIG_DAMAGE           (1 << 15)    // someone damage mob           //
#define MTRIG_GREET_PC         (1 << 16)
#define MTRIG_GREET_PC_ALL     (1 << 17)
#define MTRIG_INCOME           (1 << 18)    // mob income to room //
#define MTRIG_INCOME_PC        (1 << 19)    // mob income to room if PC there //
#define MTRIG_START_FIGHT      (1 << 20) // начало боя с мобом
#define MTRIG_ROUND_NUM        (1 << 21) // на х раунде боя
#define MTRIG_CAST             (1 << 22) // каст в моба
#define MTRIG_TIMECHANGE       (1 << 23) // смена времени

// obj trigger types //
#define OTRIG_RANDOM_GLOBAL           (1 << 0)    // unused                     //
#define OTRIG_RANDOM           (1 << 1)    // checked randomly           //
#define OTRIG_COMMAND          (1 << 2)    // character types a command  //
#define OTRIG_PURGE           (1 << 3)    // object purge  //
#define OTRIG_FIGHT           (1 << 4)    // every fight round  //
#define OTRIG_TIMER            (1 << 5)    // item's timer expires       //
#define OTRIG_GET              (1 << 6)    // item is picked up          //
#define OTRIG_DROP             (1 << 7)    // character trys to drop obj //
#define OTRIG_GIVE             (1 << 8)    // character trys to give obj //
#define OTRIG_WEAR             (1 << 9)    // character trys to wear obj //
#define OTRIG_REMOVE           (1 << 11)    // character trys to remove obj //

#define OTRIG_LOAD             (1 << 13)    // the object is loaded       //

#define OTRIG_UNLOCK           (1 << 14)
#define OTRIG_OPEN             (1 << 15)
#define OTRIG_LOCK             (1 << 16)
#define OTRIG_CLOSE            (1 << 17)
#define OTRIG_PICK             (1 << 18)
#define OTRIG_GREET_ALL_PC     (1 << 19)    // любой персонаж вошел в комнату //
#define OTRIG_TIMECHANGE       (1 << 20) // смена времени
#define OTRIG_PUT              (1 << 21) // положили предмет в контейнер

// wld trigger types //
#define WTRIG_RANDOM_GLOBAL           (1 << 0)    // check even if zone empty   //
#define WTRIG_RANDOM           (1 << 1)    // checked randomly           //
#define WTRIG_COMMAND          (1 << 2)    // character types a command  //
#define WTRIG_SPEECH           (1 << 3)    // a char says word/phrase    //
#define WTRIG_ENTER_PC         (1 << 4)
#define WTRIG_RESET            (1 << 5)    // zone has been reset        //
#define WTRIG_ENTER            (1 << 6)    // character enters room      //
#define WTRIG_DROP             (1 << 7)    // something dropped in room  //

#define WTRIG_UNLOCK           (1 << 8)
#define WTRIG_OPEN             (1 << 9)
#define WTRIG_LOCK             (1 << 10)
#define WTRIG_CLOSE            (1 << 11)
#define WTRIG_PICK             (1 << 12)
#define WTRIG_TIMECHANGE       (1 << 13)
#define WTRIG_KILL_PC          (1 << 14)

// obj command trigger types //
#define OCMD_EQUIP             (1 << 0)    // obj must be in char's equip //
#define OCMD_INVEN             (1 << 1)    // obj must be in char's inven //
#define OCMD_ROOM              (1 << 2)    // obj must be in char's room  //

#define TRIG_NEW                0    // trigger starts from top  //
#define TRIG_RESTART            1    // trigger restarting       //

const Bitvector kNormalRound = 0;
const Bitvector kNoCastMagic = 1 << 0;
const Bitvector kNoExtraAttack = 1 << 1;
const Bitvector kNoLeftHandAttack = 1 << 2;
const Bitvector kNoRightHandAttack = 1 << 3;

const Bitvector kOtrigDropDefault = 0;
const Bitvector kOtrigDropInroom = 1 << 0;
const Bitvector kOtrigPutContainer = 1 << 1;

/*
 * These are slightly off of kPulseMobile so
 * everything isnt happening at the same time
 */
constexpr long long PULSE_DG_SCRIPT = 13*kRealSec;
const int kMaxScriptDepth = 512;					 // maximum depth triggers can recurse into each other

struct wait_event_data {
	Trigger *trigger;
	void *go;
	int type;
};

// one line of the trigger //
class cmdlist_element {
 public:
	using shared_ptr = std::shared_ptr<cmdlist_element>;

	std::string cmd;        // one line of a trigger //
	int line_num;
	shared_ptr original;
	shared_ptr next;
};

struct TriggerVar {
	char *name;        // name of variable  //
	char *value;        // value of variable //
	long context;        // 0: global context //

	struct TriggerVar *next;
};

// structure for triggers //
class Trigger {
 public:
	using cmdlist_ptr = std::shared_ptr<cmdlist_element::shared_ptr>;

	static const char *DEFAULT_TRIGGER_NAME;

	Trigger();
	Trigger(const Trigger &from);
	Trigger &operator=(const Trigger &right);
	Trigger(const sh_int rnum, const char *name, const long trigger_type);
	Trigger(const sh_int rnum, const char *name, const byte attach_type, const long trigger_type);
	Trigger(const sh_int rnum, std::string &&name, const byte attach_type, const long trigger_type);

	virtual ~Trigger() {}    // make constructor virtual to be able to create a mock for this class

	auto get_rnum() const { return nr; }
	void set_rnum(const sh_int _) { nr = _; }
	void set_attach_type(const byte _) { attach_type = _; }
	auto get_attach_type() const { return attach_type; }
	const auto &get_name() const { return name; }
	void set_name(const std::string &_) { name = _; }
	auto get_trigger_type() const { return trigger_type; }
	void set_trigger_type(const long _) { trigger_type = _; }
	void clear_var_list();

	cmdlist_ptr cmdlist;    // top of command list             //
	cmdlist_element::shared_ptr curr_state;    // ptr to current line of trigger  //

	int narg;        // numerical argument              //
	std::string arglist;        // argument list                   //
	int depth;        // depth into nest ifs/whiles/etc  //
	int loops;        // loop iteration counter          //
	struct TriggerEvent *wait_event;    // event to pause the trigger      //
	struct TriggerVar *var_list;    // list of local vars for trigger  //

 private:
	void reset();

	sh_int nr;            // trigger's rnum                  //
	byte attach_type;    // mob/obj/wld intentions          //
	std::string name;    // name of trigger
	long trigger_type;    // type of trigger (for bitvector) //
};

class TriggerEventObserver {
 public:
	using shared_ptr = std::shared_ptr<TriggerEventObserver>;

	~TriggerEventObserver() {}

	virtual void notify(Trigger *trigger) = 0;
};

/**
* In addition to simple list properties this class provides safe iterators allowing to remove triggers
* from list while iterating. However this class doesn't perform any checks.
*/
class TriggersList {
 public:
	using triggers_list_t = std::list<Trigger *>;

	enum IteratorPosition {
		BEGIN,
		END
	};

	class iterator {
	 public:
		iterator(const iterator &rhv);
		~iterator();

		Trigger *operator*() { return *m_iterator; }
		Trigger *operator->() { return *m_iterator; }
		iterator &operator++();
		operator bool() const { return m_iterator != m_owner->m_list.end(); }

	 private:
		iterator(TriggersList *owner, const IteratorPosition position);

		friend class TriggerRemoveObserverI;
		friend class TriggersList;

		void setup_observer();
		void remove_event(Trigger *trigger);

		TriggersList *m_owner;
		triggers_list_t::const_iterator m_iterator;
		TriggerEventObserver::shared_ptr m_observer;
		bool m_removed;
	};

	TriggersList();
	~TriggersList();

	bool add(Trigger *trigger, const bool to_front = false);
	void remove(Trigger *trigger);
	Trigger *find(const bool by_name, const char *name, const int vnum_or_position);
	Trigger *find_by_name(const char *name, const int number);
	Trigger *find_by_vnum_or_position(const int vnum_or_position);
	Trigger *find_by_vnum(const int vnum);
	Trigger *remove_by_name(const char *name, const int number);
	Trigger *remove_by_vnum_or_position(const int vnum_or_position);
	Trigger *remove_by_vnum(const int vnum);
	long get_type() const;
	bool has_trigger(const Trigger *const trigger);
	void clear();
	bool empty() const { return m_list.empty(); }

	iterator begin() { return std::move(iterator(this, BEGIN)); }
	iterator end() { return std::move(iterator(this, END)); }

	operator bool() const { return !m_list.empty(); }

	std::ostream &dump(std::ostream &os) const;

 private:
	using iterator_observers_t = std::unordered_set<TriggerEventObserver::shared_ptr>;

	triggers_list_t::iterator remove(const triggers_list_t::iterator &iterator);
	void register_observer(const TriggerEventObserver::shared_ptr &observer);
	void unregister_observer(const TriggerEventObserver::shared_ptr &observer);

	triggers_list_t m_list;
	TriggerEventObserver::shared_ptr m_observer;
	iterator_observers_t m_iterator_observers;
};

inline std::ostream &operator<<(std::ostream &os, const TriggersList &triggers_list) {
	return triggers_list.dump(os);
}

// a complete script (composed of several triggers) //
class Script {
 public:
	using shared_ptr = std::shared_ptr<Script>;

	// привет костыли
	Script();
	Script(const Script &script);
	~Script();

	/*
	*  removes the trigger specified by name, and the script of o if
	*  it removes the last trigger.  name can either be a number, or
	*  a 'silly' name for the trigger, including things like 2.beggar-death.
	*  returns 0 if did not find the trigger, otherwise 1.  If it matters,
	*  you might need to check to see if all the triggers were removed after
	*  this function returns, in order to remove the script.
	*/
	int remove_trigger(char *name, Trigger *&trig_addr);
	int remove_trigger(char *name);

	void clear_global_vars();
	void cleanup();

	bool has_triggers() { return !trig_list.empty(); }
	bool is_purged() { return m_purged; }
	void set_purged(bool purged = true) { m_purged = purged; }

	long types;        // bitvector of trigger types //
	TriggersList trig_list;    // list of triggers           //
	struct TriggerVar *global_vars;    // list of global variables   //
	long context;        // current context for statics //

 private:
	Script &operator=(const Script &script) = delete;

	bool m_purged;
};

// used for actor memory triggers //
struct script_memory {
	long id;        // id of who to remember //
	char *cmd;        // command, or NULL for generic //
	struct script_memory *next;
};

// function prototypes from triggers.cpp (and others) //
void act_mtrigger(CharData *ch,
				  char *str,
				  CharData *actor,
				  CharData *victim,
				  const ObjData *object,
				  const ObjData *target,
				  char *arg);
void speech_mtrigger(CharData *actor, char *str);
void speech_wtrigger(CharData *actor, char *str);
void greet_mtrigger(CharData *actor, int dir);
int entry_mtrigger(CharData *ch);
void income_mtrigger(CharData *actor, int dir);
int enter_wtrigger(RoomData *room, CharData *actor, int dir);
int drop_otrigger(ObjData *obj, CharData *actor, const Bitvector argument);
void timer_otrigger(ObjData *obj);
int get_otrigger(ObjData *obj, CharData *actor);
int drop_wtrigger(ObjData *obj, CharData *actor);
int give_otrigger(ObjData *obj, CharData *actor, CharData *victim);
void greet_otrigger(CharData *actor, int dir);
int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj);
void bribe_mtrigger(CharData *ch, CharData *actor, int amount);
int wear_otrigger(ObjData *obj, CharData *actor, int where);
int remove_otrigger(ObjData *obj, CharData *actor);
int put_otrigger(ObjData *obj, CharData *actor, ObjData *cont);
int command_mtrigger(CharData *actor, char *cmd, const char *argument);
int command_otrigger(CharData *actor, char *cmd, const char *argument);
int command_wtrigger(CharData *actor, char *cmd, const char *argument);
int death_mtrigger(CharData *ch, CharData *actor);
int kill_mtrigger(CharData *ch, CharData *actor);
int fight_mtrigger(CharData *ch);
void hitprcnt_mtrigger(CharData *ch);
int damage_mtrigger(CharData *damager, CharData *victim, int amount, const char* name_skillorspell, int is_skill, ObjData *obj);
void random_mtrigger(CharData *ch);
void random_otrigger(ObjData *obj);
Bitvector fight_otrigger(CharData *actor);
void random_wtrigger(RoomData *room, const TriggersList &list);
void reset_wtrigger(RoomData *ch);
void load_mtrigger(CharData *ch);
void load_otrigger(ObjData *obj);
void purge_otrigger(ObjData *obj);
int start_fight_mtrigger(CharData *ch, CharData *actor);
void round_num_mtrigger(CharData *ch, CharData *actor);
int cast_mtrigger(CharData *ch, CharData *actor, ESpell spell_id);
void kill_pc_wtrigger(CharData *killer, CharData *victim);

// function prototypes from scripts.cpp //
void script_trigger_check(void);
void script_timechange_trigger_check(const int time, const int time_day);
bool add_trigger(Script *sc, Trigger *t, int loc);

void do_stat_trigger(CharData *ch, Trigger *trig);
void do_sstat_room(RoomData *rm, CharData *ch);
void do_sstat_room(CharData *ch);
void do_sstat_object(CharData *ch, ObjData *j);
void do_sstat_character(CharData *ch, CharData *k);
void print_worlds_vars(CharData *ch, std::optional<long> context);

void script_log(const char *msg,
				LogMode type = LogMode::OFF);//type нужен чтоб не спамить мессаги тем у кого errlog не полный а краткий например
void trig_log(Trigger *trig, const char *msg, LogMode type = LogMode::OFF);

using obj2triggers_t = std::unordered_map<ObjVnum, std::list<TrgVnum>>;
extern obj2triggers_t &obj2triggers;

class GlobalTriggersStorage {
 public:
	~GlobalTriggersStorage();

	void add(Trigger *trigger);
	void remove(Trigger *trigger);
	void shift_rnums_from(const Rnum rnum);
	bool has_triggers_with_rnum(const Rnum rnum) const {
		return m_rnum2triggers_set.find(rnum) != m_rnum2triggers_set.end();
	}
	const auto &get_triggers_with_rnum(const Rnum rnum) const { return m_rnum2triggers_set.at(rnum); }
	void register_remove_observer(Trigger *trigger, const TriggerEventObserver::shared_ptr &observer);
	void unregister_remove_observer(Trigger *trigger, const TriggerEventObserver::shared_ptr &observer);

 private:
	using triggers_set_t = std::unordered_set<Trigger *>;
	using storage_t = triggers_set_t;
	using rnum2triggers_set_t = std::unordered_map<Rnum, triggers_set_t>;
	using observers_set_t = std::unordered_set<TriggerEventObserver::shared_ptr>;
	using observers_t = std::unordered_map<Trigger *, observers_set_t>;

	storage_t m_triggers;
	rnum2triggers_set_t m_rnum2triggers_set;
	observers_t m_observers;
};

extern GlobalTriggersStorage &trigger_list;

void dg_obj_trigger(char *line, ObjData *obj);
void assign_triggers(void *i, int type);
int real_trigger(int vnum);
void extract_script_mem(struct script_memory *sc);

Trigger *read_trigger(int nr);
// void add_var(struct TriggerVar **var_list, char *name, char *value, long id);
CharData *dg_caster_owner_obj(ObjData *obj);
RoomData *dg_room_of_obj(ObjData *obj);
void do_dg_cast(void *go, Trigger *trig, int type, char *cmd);
void do_dg_affect(void *go, Script *sc, Trigger *trig, int type, char *cmd);

void add_var_cntx(struct TriggerVar **var_list, const char *name, const char *value, long id);
struct TriggerVar *find_var_cntx(struct TriggerVar **var_list, const char *name, long id);
int remove_var_cntx(struct TriggerVar **var_list, char *name, long id);

// Macros for scripts //

#define UID_OBJ    '\x1c'
#define UID_ROOM   '\x1d'
#define UID_CHAR   '\x1e'
#define UID_CHAR_ALL   '\x1f'

#define GET_TRIG_NAME(t)          ((t)->get_name().c_str())
#define GET_TRIG_RNUM(t)          ((t)->get_rnum())
#define GET_TRIG_VNUM(t)      (trig_index[(t)->get_rnum()]->vnum)
#define GET_TRIG_TYPE(t)          ((t)->get_trigger_type())
#define GET_TRIG_DATA_TYPE(t)      ((t)->get_data_type())
#define GET_TRIG_NARG(t)          ((t)->narg)
#define GET_TRIG_ARG(t)           ((t)->arglist)
#define GET_TRIG_VARS(t)      ((t)->var_list)
#define GET_TRIG_WAIT(t)      ((t)->wait_event)
#define GET_TRIG_DEPTH(t)         ((t)->depth)
#define GET_TRIG_LOOPS(t)         ((t)->loops)

// player id's: 0 to ROOM_ID_BASE - 1            //
// room id's: ROOM_ID_BASE to MOBOBJ_ID_BASE - 1 //
const int kRoomToBase = 150000;
#define SCRIPT_TYPES(s)          ((s)->types)

#define GET_SHORT(ch)    ((ch)->get_npc_name().c_str())

bool CheckSript(const ObjData *go, const long type);
bool CheckScript(const CharData *go, const long type);
bool CheckSript(const RoomData *go, const long type);

#define TRIGGER_CHECK(t, type)   (IS_SET(GET_TRIG_TYPE(t), type) && \
                  !GET_TRIG_DEPTH(t))

#define SCRIPT(o)          ((o)->script)

// typedefs that the dg functions rely on //

void timechange_mtrigger(CharData *ch, const int time, const int time_day);
int pick_otrigger(ObjData *obj, CharData *actor);
int open_otrigger(ObjData *obj, CharData *actor, int unlock);
int close_otrigger(ObjData *obj, CharData *actor, int lock);
int timechange_otrigger(ObjData *obj, const int time, const int time_day);
int pick_wtrigger(RoomData *room, CharData *actor, int dir);
int open_wtrigger(RoomData *room, CharData *actor, int dir, int unlock);
int close_wtrigger(RoomData *room, CharData *actor, int dir, int lock);
int timechange_wtrigger(RoomData *room, const int time, const int time_day);

void trg_featturn(CharData *ch, EFeat feat_id, int featdiff, int vnum);
void trg_skillturn(CharData *ch, const ESkill skill_id, int skilldiff, int vnum);
void AddSkill(CharData *ch, const ESkill skillnum, int skilldiff, int vnum);
void trg_spellturn(CharData *ch, ESpell spell_id, int spelldiff, int vnum);
void trg_spellturntemp(CharData *ch, ESpell spell_id, int spelldiff, int vnum);
void trg_spelladd(CharData *ch, ESpell spell_id, int spelldiff, int vnum);
void trg_spellitem(CharData *ch, ESpell spell_id, int spelldiff, ESpellType spell_type);
CharData *get_char(char *name);
// external vars from db.cpp //
extern int top_of_trigt;
extern int last_trig_vnum;//последний триг в котором произошла ошибка
extern int curr_trig_vnum;
extern int last_trig_line_num;
const int MAX_TRIG_USEC = 30000;

void save_char_vars(CharData *ch);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
