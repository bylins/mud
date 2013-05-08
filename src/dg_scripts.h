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

#define DG_SCRIPT_VERSION "DG Scripts Version 0.99 Patch Level 7   12/98"


#define    MOB_TRIGGER   0
#define    OBJ_TRIGGER   1
#define    WLD_TRIGGER   2

extern const char *attach_name[];
#define DG_NO_TRIG         256	// don't check act trigger   //

// mob trigger types //
#define MTRIG_GLOBAL           (1 << 0)	// check even if zone empty   //
#define MTRIG_RANDOM           (1 << 1)	// checked randomly           //
#define MTRIG_COMMAND          (1 << 2)	// character types a command  //
#define MTRIG_SPEECH           (1 << 3)	// a char says a word/phrase  //
#define MTRIG_ACT              (1 << 4)	// word or phrase sent to act //
#define MTRIG_DEATH            (1 << 5)	// character dies             //
#define MTRIG_GREET            (1 << 6)	// something enters room seen //
#define MTRIG_GREET_ALL        (1 << 7)	// anything enters room       //
#define MTRIG_ENTRY            (1 << 8)	// the mob enters a room      //
#define MTRIG_RECEIVE          (1 << 9)	// character is given obj     //
#define MTRIG_FIGHT            (1 << 10)	// each pulse while fighting  //
#define MTRIG_HITPRCNT         (1 << 11)	// fighting and below some hp //
#define MTRIG_BRIBE	       (1 << 12)	// coins are given to mob     //
#define MTRIG_LOAD             (1 << 13)	// the mob is loaded          //
#define MTRIG_MEMORY           (1 << 14)	// mob see's someone remembered //
#define MTRIG_DAMAGE           (1 << 15)	// someone damage mob           //
#define MTRIG_GREET_PC         (1 << 16)
#define MTRIG_GREET_PC_ALL     (1 << 17)
#define MTRIG_INCOME           (1 << 18)	// mob income to room //
#define MTRIG_INCOME_PC        (1 << 19)	// mob income to room if PC there //
#define MTRIG_START_FIGHT      (1 << 20) // начало боя с мобом
#define MTRIG_ROUND_NUM        (1 << 21) // на х раунде боя
#define MTRIG_CAST             (1 << 22) // каст в моба
#define MTRIG_TIMECHANGE       (1 << 23) // смена времени

// obj trigger types //
#define OTRIG_GLOBAL           (1 << 0)	// unused                     //
#define OTRIG_RANDOM           (1 << 1)	// checked randomly           //
#define OTRIG_COMMAND          (1 << 2)	// character types a command  //

#define OTRIG_TIMER            (1 << 5)	// item's timer expires       //
#define OTRIG_GET              (1 << 6)	// item is picked up          //
#define OTRIG_DROP             (1 << 7)	// character trys to drop obj //
#define OTRIG_GIVE             (1 << 8)	// character trys to give obj //
#define OTRIG_WEAR             (1 << 9)	// character trys to wear obj //
#define OTRIG_REMOVE           (1 << 11)	// character trys to remove obj //

#define OTRIG_LOAD             (1 << 13)	// the object is loaded       //

#define OTRIG_UNLOCK           (1 << 14)
#define OTRIG_OPEN             (1 << 15)
#define OTRIG_LOCK             (1 << 16)
#define OTRIG_CLOSE            (1 << 17)
#define OTRIG_PICK             (1 << 18)
#define OTRIG_GREET_ALL_PC     (1 << 19)	// любой персонаж вошел в комнату //
#define OTRIG_TIMECHANGE       (1 << 20) // смена времени

// wld trigger types //
#define WTRIG_GLOBAL           (1 << 0)	// check even if zone empty   //
#define WTRIG_RANDOM           (1 << 1)	// checked randomly           //
#define WTRIG_COMMAND          (1 << 2)	// character types a command  //
#define WTRIG_SPEECH           (1 << 3)	// a char says word/phrase    //
#define WTRIG_ENTER_PC         (1 << 4)
#define WTRIG_RESET            (1 << 5)	// zone has been reset        //
#define WTRIG_ENTER            (1 << 6)	// character enters room      //
#define WTRIG_DROP             (1 << 7)	// something dropped in room  //

#define WTRIG_UNLOCK           (1 << 8)
#define WTRIG_OPEN             (1 << 9)
#define WTRIG_LOCK             (1 << 10)
#define WTRIG_CLOSE            (1 << 11)
#define WTRIG_PICK             (1 << 12)
#define WTRIG_TIMECHANGE       (1 << 13)

// obj command trigger types //
#define OCMD_EQUIP             (1 << 0)	// obj must be in char's equip //
#define OCMD_INVEN             (1 << 1)	// obj must be in char's inven //
#define OCMD_ROOM              (1 << 2)	// obj must be in char's room  //

#define TRIG_NEW                0	// trigger starts from top  //
#define TRIG_RESTART            1	// trigger restarting       //


/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
#define PULSE_DG_SCRIPT         (13 RL_SEC)

// maximum depth triggers can recurse into each other
#define MAX_SCRIPT_DEPTH        512
struct wait_event_data
{
	trig_data *trigger;
	void *go;
	int type;
};


// one line of the trigger //
struct cmdlist_element
{
	char *cmd;		// one line of a trigger //
	struct cmdlist_element *original;
	struct cmdlist_element *next;
};

struct trig_var_data
{
	char *name;		// name of variable  //
	char *value;		// value of variable //
	long context;		// 0: global context //

	struct trig_var_data *next;
};

// structure for triggers //
struct trig_data
{
	sh_int nr;		// trigger's rnum                  //
	byte attach_type;	// mob/obj/wld intentions          //
	byte data_type;		// type of game_data for trig      //
	char *name;		// name of trigger                 //
	long trigger_type;	// type of trigger (for bitvector) //
	struct cmdlist_element *cmdlist;	// top of command list             //
	struct cmdlist_element *curr_state;	// ptr to current line of trigger  //
	int narg;		// numerical argument              //
	char *arglist;		// argument list                   //
	int depth;		// depth into nest ifs/whiles/etc  //
	int loops;		// loop iteration counter          //
	struct event_info *wait_event;	// event to pause the trigger      //
	ubyte purged;		// trigger is set to be purged     //
	struct trig_var_data *var_list;	// list of local vars for trigger  //

	TRIG_DATA *next;
	TRIG_DATA *next_in_world;	// next in the global trigger list //
};


// a complete script (composed of several triggers) //
struct script_data
{
	long types;		// bitvector of trigger types //
	TRIG_DATA *trig_list;	// list of triggers           //
	struct trig_var_data *global_vars;	// list of global variables   //
	ubyte purged;		// script is set to be purged //
	long context;		// current context for statics //

	SCRIPT_DATA *next;	// used for purged_scripts    //
};

// used for actor memory triggers //
struct script_memory
{
	long id;		// id of who to remember //
	char *cmd;		// command, or NULL for generic //
	struct script_memory *next;
};


// function prototypes from triggers.cpp (and others) //
void act_mtrigger(CHAR_DATA * ch, char *str, CHAR_DATA * actor, CHAR_DATA * victim, const OBJ_DATA * object, const OBJ_DATA * target, char *arg);
void speech_mtrigger(CHAR_DATA * actor, char *str);
void speech_wtrigger(CHAR_DATA * actor, char *str);
void greet_memory_mtrigger(CHAR_DATA * ch);
int greet_mtrigger(CHAR_DATA * actor, int dir);
int entry_mtrigger(CHAR_DATA * ch);
void income_mtrigger(CHAR_DATA * actor, int dir);
void entry_memory_mtrigger(CHAR_DATA * ch);
int enter_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir);
int drop_otrigger(OBJ_DATA * obj, CHAR_DATA * actor);
void timer_otrigger(OBJ_DATA * obj);
int get_otrigger(OBJ_DATA * obj, CHAR_DATA * actor);
int drop_wtrigger(OBJ_DATA * obj, CHAR_DATA * actor);
int give_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, CHAR_DATA * victim);
int greet_otrigger(CHAR_DATA * actor, int dir);
int receive_mtrigger(CHAR_DATA * ch, CHAR_DATA * actor, OBJ_DATA * obj);
void bribe_mtrigger(CHAR_DATA * ch, CHAR_DATA * actor, int amount);
int wear_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, int where);
int remove_otrigger(OBJ_DATA * obj, CHAR_DATA * actor);
int command_mtrigger(CHAR_DATA * actor, char *cmd, char *argument);
int command_otrigger(CHAR_DATA * actor, char *cmd, char *argument);
int command_wtrigger(CHAR_DATA * actor, char *cmd, char *argument);
int death_mtrigger(CHAR_DATA * ch, CHAR_DATA * actor);
void fight_mtrigger(CHAR_DATA * ch);
void hitprcnt_mtrigger(CHAR_DATA * ch);
int damage_mtrigger(CHAR_DATA * damager, CHAR_DATA * victim);
void random_mtrigger(CHAR_DATA * ch);
void random_otrigger(OBJ_DATA * obj);
void random_wtrigger(ROOM_DATA * room, int num, void *s, int types, void *list, void *next);
void reset_wtrigger(ROOM_DATA * ch);
void load_mtrigger(CHAR_DATA * ch);
void load_otrigger(OBJ_DATA * obj);
void start_fight_mtrigger(CHAR_DATA *ch, CHAR_DATA *actor);
void round_num_mtrigger(CHAR_DATA *ch, CHAR_DATA *actor);
void cast_mtrigger(CHAR_DATA *ch, CHAR_DATA *actor, int spellnum);

// function prototypes from scripts.cpp //
void script_trigger_check(void);
void script_timechange_trigger_check(const int time);
void add_trigger(struct script_data *sc, TRIG_DATA * t, int loc);
int remove_trigger(SCRIPT_DATA * sc, char *name, TRIG_DATA ** trig_addr);

void do_stat_trigger(CHAR_DATA * ch, TRIG_DATA * trig);
void do_sstat_room(ROOM_DATA *rm, CHAR_DATA * ch);
void do_sstat_room(CHAR_DATA * ch);
void do_sstat_object(CHAR_DATA * ch, OBJ_DATA * j);
void do_sstat_character(CHAR_DATA * ch, CHAR_DATA * k);

void script_log(const char *msg, const int type = 0);//type нужен чтоб не спамить мессаги тем у кого errlog не полный а краткий например
void trig_log(TRIG_DATA * trig, const char *msg, const int type = 0);// --//--
void dg_read_trigger(FILE * fp, void *i, int type);
void dg_obj_trigger(char *line, OBJ_DATA * obj);
void assign_triggers(void *i, int type);
void parse_trigger(FILE * trig_f, int nr);
int real_trigger(int vnum);
void extract_script(struct script_data *sc);
void extract_script_mem(struct script_memory *sc);

TRIG_DATA *read_trigger(int nr);
// void add_var(struct trig_var_data **var_list, char *name, char *value, long id);
ROOM_DATA *dg_room_of_obj(OBJ_DATA * obj);
void do_dg_cast(void *go, struct script_data *sc, TRIG_DATA * trig, int type, char *cmd);
void do_dg_affect(void *go, struct script_data *sc, TRIG_DATA * trig, int type, char *cmd);



void add_var_cntx(struct trig_var_data **var_list, const char *name, const char *value, long id);
struct trig_var_data *find_var_cntx(struct trig_var_data **var_list, char *name, long id);
int remove_var_cntx(struct trig_var_data **var_list, char *name, long id);



// Macros for scripts //

#define UID_CHAR   '\x1b'
#define UID_OBJ    '\x1c'
#define UID_ROOM   '\x1d'

#define GET_TRIG_NAME(t)          ((t)->name)
#define GET_TRIG_RNUM(t)          ((t)->nr)
#define GET_TRIG_VNUM(t)	  (trig_index[(t)->nr]->vnum)
#define GET_TRIG_TYPE(t)          ((t)->trigger_type)
#define GET_TRIG_DATA_TYPE(t)	  ((t)->data_type)
#define GET_TRIG_NARG(t)          ((t)->narg)
#define GET_TRIG_ARG(t)           ((t)->arglist)
#define GET_TRIG_VARS(t)	  ((t)->var_list)
#define GET_TRIG_WAIT(t)	  ((t)->wait_event)
#define GET_TRIG_DEPTH(t)         ((t)->depth)
#define GET_TRIG_LOOPS(t)         ((t)->loops)

// player id's: 0 to ROOM_ID_BASE - 1            //
// room id's: ROOM_ID_BASE to MOBOBJ_ID_BASE - 1 //
// mob/object id's: MOBOBJ_ID_BASE and higher    //
#define ROOM_ID_BASE    150000
#define MOBOBJ_ID_BASE	2000000

#define SCRIPT_MEM(c)             ((c)->memory)

#define SCRIPT_TYPES(s)		  ((s)->types)
#define TRIGGERS(s)		  ((s)->trig_list)

#define GET_SHORT(ch)    ((ch)->get_npc_name())

#define SCRIPT_CHECK(go, type)   (SCRIPT(go) && \
				  IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type)   (IS_SET(GET_TRIG_TYPE(t), type) && \
				  !GET_TRIG_DEPTH(t))

#define ADD_UID_CHAR_VAR(buf, trig, go, name, context) { \
		         sprintf(buf, "%c%ld", UID_CHAR, GET_ID(go)); \
                         add_var_cntx(&GET_TRIG_VARS(trig), name, buf, context); }
#define ADD_UID_OBJ_VAR(buf, trig, go, name, context) { \
		         sprintf(buf, "%c%ld", UID_OBJ, GET_ID(go)); \
                         add_var_cntx(&GET_TRIG_VARS(trig), name, buf, context); }
#define ADD_UID_ROOM_VAR(buf, trig, go, name, context) { \
		         sprintf(buf, "%c%ld", UID_ROOM, GET_ID(go)); \
                         add_var_cntx(&GET_TRIG_VARS(trig), name, buf, context); }


#define SCRIPT(o)		  ((o)->script)

void proto_script_copy(struct trig_proto_list **pdst, struct trig_proto_list *src);
void proto_script_free(struct trig_proto_list *src);
void free_script(struct script_data *sc);

// typedefs that the dg functions rely on //
typedef INDEX_DATA index_data;
typedef ROOM_DATA room_data;

void timechange_mtrigger(CHAR_DATA * ch, const int time);
int pick_otrigger(OBJ_DATA * obj, CHAR_DATA * actor);
int open_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, int unlock);
int close_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, int lock);
int timechange_otrigger(OBJ_DATA * obj, const int time);
int pick_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir);
int open_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir, int unlock);
int close_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir, int lock);
int timechange_wtrigger(ROOM_DATA * room, const int time);

void trg_featturn(CHAR_DATA * ch, int featnum, int featdiff);
void trg_skillturn(CHAR_DATA * ch, int skillnum, int skilldiff);
void trg_skilladd(CHAR_DATA * ch, int skillnum, int skilldiff);
void trg_spellturn(CHAR_DATA * ch, int spellnum, int spelldiff);
void trg_spelladd(CHAR_DATA * ch, int spellnum, int spelldiff);
void trg_spellitem(CHAR_DATA * ch, int spellnum, int spelldiff, int spell);

// external vars from db.cpp //
extern int top_of_trigt;
extern INDEX_DATA **trig_index;
extern int last_trig_vnum;//последний триг в котором произошла ошибка

const int MAX_TRIG_USEC = 30000;

void save_char_vars(CHAR_DATA *ch);

#endif
