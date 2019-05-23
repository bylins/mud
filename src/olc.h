/************************************************************************
* OasisOLC - olc.h                                                 v1.5 *
*                                                                       *
* Copyright 1996 Harvey Gilpin.                                         *
*                                                                       *
*  $Author$                                                       *
*  $Date$                                         *
*  $Revision$                                                    *
************************************************************************/

#ifndef _OLC_H_
#define _OLC_H_

#include "db.h"
#include "structs.h"

// * If you don't want a short explanation of each field in your zone files,
// * change the number below to a 0 instead of a 1.
#if 0
#define ZEDIT_HELP_IN_FILE
#endif

// * If you want to clear the screen before certain Oasis menus, set to 1.
#if 0
#define CLEAR_SCREEN	1
#endif

// * Set this to 1 to enable MobProg support.
#if 0
#define OASIS_MPROG	1
#endif

// * Macros, defines, structs and globals for the OLC suite.

#define NUM_PLANES          4

#define NUM_ROOM_FLAGS 		16
#define NUM_ROOM_SECTORS	14

#define NUM_MOB_FLAGS		18
#define NUM_AFF_FLAGS		22
#define NUM_ATTACK_TYPES	20

#define NUM_ITEM_TYPES		36
#define NUM_ITEM_FLAGS		30
#define NUM_ITEM_WEARS 		17
#define NUM_POSITIONS		16
#define NUM_SPELLS	        108
#define NUM_MATERIALS       19

#define NUM_GENDERS		    3
#define NUM_SHOP_FLAGS 		2
#define NUM_TRADERS 		11

// * Define this to how many MobProg scripts you have.
#define NUM_PROGS		12

// * Utilities exported from olc.c.
void strip_string(char *);
void cleanup_olc(DESCRIPTOR_DATA * d, byte cleanup_type);
void get_char_cols(CHAR_DATA * ch);
void olc_add_to_save_list(int zone, byte type);
void olc_remove_from_save_list(int zone, byte type);

// * OLC structures.

typedef struct t_zcmd
{
	struct t_zcmd *next;	// следующий элемент кольцевого буфера
	struct t_zcmd *prev;	// предыдущий элемент кольцевого буфера
	struct reset_com cmd;	// команда
} zcmd, *pzcmd;

void zedit_delete_cmdlist(pzcmd head);

#define		OLC_BM_SHOWALLCMD		(1<<31)

class MakeRecept;
class ZoneData;	// to avoid inclusion of "zone.table.hpp"

struct olc_data
{
	olc_data();

	int mode;
	int zone_num;
	int number;
	int value;
	int total_mprogs;
	unsigned long bitmask;
	CHAR_DATA *mob;
	ROOM_DATA *room;
	OBJ_DATA *obj;
	ZoneData *zone;
	EXTRA_DESCR_DATA::shared_ptr desc;

	MakeRecept *mrec;

#if defined(OASIS_MPROG)
	struct mob_prog_data *mprog;
	struct mob_prog_data *mprogl;
#endif
	TRIG_DATA *trig;
	int script_mode;
	int trigger_position;
	int item_type;
	OBJ_DATA::triggers_list_t script;
	char *storage;		// for holding commands etc..
};

struct olc_save_info
{
	int zone;
	char type;
	struct olc_save_info *next;
};

// * Exported globals.
extern const char *nrm, *grn, *cyn, *yel, *iyel, *ired;
extern struct olc_save_info *olc_save_list;

// * Descriptor access macros.
#define OLC_MODE(d) 	((d)->olc->mode)	// Parse input mode.    //
#define OLC_NUM(d) 	((d)->olc->number)	// Room/Obj VNUM.       //
#define OLC_VAL(d) 	((d)->olc->value)	// Scratch variable.    //
#define OLC_ZNUM(d) 	((d)->olc->zone_num)	// Real zone number.    //
#define OLC_ROOM(d) 	((d)->olc->room)	// Room structure.      //
#define OLC_OBJ(d) 	((d)->olc->obj)	// Object structure.    //
#define OLC_ZONE(d)     ((d)->olc->zone)	// Zone structure.      //
#define OLC_MOB(d)	((d)->olc->mob)	// Mob structure.       //
#define OLC_SHOP(d) 	((d)->olc->shop)	// Shop structure.      //
#define OLC_DESC(d) 	((d)->olc->desc)	// Extra description.   //
#define OLC_MREC(d)     ((d)->olc->mrec)	// Make recept field    //
#ifdef OASIS_MPROG
#define OLC_MPROG(d)	((d)->olc->mprog)	// Temporary MobProg.   //
#define OLC_MPROGL(d)	((d)->olc->mprogl)	// MobProg list.        //
#define OLC_MTOTAL(d)	((d)->olc->total_mprogs)	// Total mprog number.  //
#endif
#define OLC_TRIG(d)     ((d)->olc->trig)	// Trigger structure.   //
#define OLC_STORAGE(d)  ((d)->olc->storage)	// For command storage  //

// * Other macros.
#define OLC_EXIT(d)	(OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define GET_OLC_ZONE(c)	((c)->player_specials->saved.olc_zone)

// * Cleanup types.
#define CLEANUP_ALL		(byte)	1	// Free the whole lot.
#define CLEANUP_STRUCTS 	(byte)	2	// Don't free strings.

// * Add/Remove save list types.
#define OLC_SAVE_ROOM		(byte)	0
#define OLC_SAVE_OBJ		(byte)	1
#define OLC_SAVE_ZONE		(byte)	2
#define OLC_SAVE_MOB		(byte)	3

// * Submodes of OEDIT connectedness.
#define OEDIT_MAIN_MENU                 1
#define OEDIT_EDIT_NAMELIST             2
#define OEDIT_PAD0                      3
#define OEDIT_PAD1                      4
#define OEDIT_PAD2                      5
#define OEDIT_PAD3                      6
#define OEDIT_PAD4                      7
#define OEDIT_PAD5                      8
#define OEDIT_LONGDESC                  9
#define OEDIT_ACTDESC                   10
#define OEDIT_TYPE                      11
#define OEDIT_EXTRAS                    12
#define OEDIT_WEAR                      13
#define OEDIT_NO                        14
#define OEDIT_ANTI                      15
#define OEDIT_WEIGHT                    16
#define OEDIT_COST                      17
#define OEDIT_COSTPERDAY                18
#define OEDIT_COSTPERDAYEQ              19
#define OEDIT_MAXVALUE                  20
#define OEDIT_CURVALUE                  21
#define OEDIT_MATER                     22
#define OEDIT_TIMER                 	23
#define OEDIT_SKILL                     24
#define OEDIT_VALUE_1                   25
#define OEDIT_VALUE_2                   26
#define OEDIT_VALUE_3                   27
#define OEDIT_VALUE_4                   28
#define OEDIT_AFFECTS                   29
#define OEDIT_APPLY                     30
#define OEDIT_APPLYMOD                  31
#define OEDIT_EXTRADESC_KEY             32
#define OEDIT_CONFIRM_SAVEDB            33
#define OEDIT_CONFIRM_SAVESTRING        34
#define OEDIT_PROMPT_APPLY              35
#define OEDIT_EXTRADESC_DESCRIPTION     36
#define OEDIT_EXTRADESC_MENU            37
#define OEDIT_LEVEL                     38
#define OEDIT_SEXVALUE                  39
#define OEDIT_MIWVALUE                  40
#define OEDIT_SKILLS                    41
#define OEDIT_MORT_REQ                  42
#define OEDIT_DRINKCON_VALUES           43
#define OEDIT_POTION_SPELL1_NUM         44
#define OEDIT_POTION_SPELL1_LVL         45
#define OEDIT_POTION_SPELL2_NUM         46
#define OEDIT_POTION_SPELL2_LVL         47
#define OEDIT_POTION_SPELL3_NUM         48
#define OEDIT_POTION_SPELL3_LVL         49
#define OEDIT_CLONE						50
#define OEDIT_CLONE_WITH_TRIGGERS			51
#define OEDIT_CLONE_WITHOUT_TRIGGERS			52

// * Submodes of REDIT connectedness.
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6
#define REDIT_CONFIRM_SAVEDB 		7
#define REDIT_CONFIRM_SAVESTRING 	8
#define REDIT_EXIT_NUMBER 		9
#define REDIT_EXIT_DESCRIPTION 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_DOORFLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 	16
#define	REDIT_ING				17
#define REDIT_LOCK_COMPLEXITY	18

// * Submodes of ZEDIT connectedness.
#define ZEDIT_MAIN_MENU          0
#define ZEDIT_DELETE_ENTRY       1
#define ZEDIT_NEW_ENTRY          2
#define ZEDIT_CHANGE_ENTRY       3
#define ZEDIT_COMMAND_TYPE       4
#define ZEDIT_IF_FLAG            5
#define ZEDIT_ARG1               6
#define ZEDIT_ARG2               7
#define ZEDIT_ARG3               8
#define ZEDIT_ARG4               9
#define ZEDIT_ZONE_NAME          10
#define ZEDIT_ZONE_LIFE          11
#define ZEDIT_ZONE_TOP           12
#define ZEDIT_ZONE_RESET         13
#define ZEDIT_RESET_IDLE         14
#define ZEDIT_CONFIRM_SAVESTRING 15
#define ZEDIT_ZONE_LOCATION      16
#define ZEDIT_ZONE_DESCRIPTION   17
#define ZEDIT_ZONE_AUTOR	 18
#define ZEDIT_SARG1              20
#define ZEDIT_SARG2              21
#define ZEDIT_MOVE_ENTRY         22
#define ZEDIT_TYPE_A_LIST        23
#define ZEDIT_TYPE_B_LIST        24
#define ZEDIT_ZONE_LEVEL         25
#define ZEDIT_ZONE_TYPE          26
#define ZEDIT_ZONE_COMMENT       27
#define ZEDIT_ZONE_GROUP         28

// * Submodes of MEDIT connectedness.
#define MEDIT_MAIN_MENU		0
#define MEDIT_ALIAS			1
#define MEDIT_PAD0			2
#define MEDIT_PAD1			3
#define MEDIT_PAD2			4
#define MEDIT_PAD3			5
#define MEDIT_PAD4			6
#define MEDIT_PAD5			7
#define MEDIT_L_DESC		8
#define MEDIT_D_DESC		9
#define MEDIT_MOB_FLAGS		10
#define MEDIT_AFF_FLAGS		11
#define MEDIT_CONFIRM_SAVESTRING	12
#define MEDIT_NPC_FLAGS		13
#define MEDIT_ING			14
#define MEDIT_DLIST_MENU	15
#define MEDIT_DLIST_ADD		16
#define MEDIT_DLIST_DEL		17
// * Numerical responses.
#define MEDIT_NUMERICAL_RESPONSE	20
#define MEDIT_SEX			24
#define MEDIT_HITROLL		25
#define MEDIT_DAMROLL		26
#define MEDIT_NDD		    27
#define MEDIT_SDD		    28
#define MEDIT_NUM_HP_DICE	29
#define MEDIT_SIZE_HP_DICE	30
#define MEDIT_ADD_HP		31
#define MEDIT_AC			32
#define MEDIT_EXP			33
#define MEDIT_GOLD			34
#define MEDIT_GOLD_DICE		35
#define MEDIT_GOLD_SIZE		36
#define MEDIT_POS			37
#define MEDIT_DEFAULT_POS	38
#define MEDIT_ATTACK		39
#define MEDIT_LEVEL			40
#define MEDIT_ALIGNMENT		41
#define MEDIT_DESTINATION	42
#define MEDIT_HELPERS		43
#define MEDIT_SKILLS		44
#define MEDIT_SPELLS		45
#define MEDIT_STR			46
#define MEDIT_DEX			47
#define MEDIT_CON			48
#define MEDIT_WIS			49
#define MEDIT_INT			50
#define MEDIT_CHA			51
#define MEDIT_HEIGHT		52
#define MEDIT_WEIGHT		53
#define MEDIT_SIZE			54
#define MEDIT_EXTRA			55
#define MEDIT_SPEED			56
#define MEDIT_LIKE			57
#define MEDIT_ROLE          58
#define MEDIT_RESISTANCES	59
#define MEDIT_SAVES			60
#define MEDIT_ADD_PARAMETERS		61
#define MEDIT_FEATURES		62
#define MEDIT_CLONE			63
#define MEDIT_CLONE_WITH_TRIGGERS	64
#define MEDIT_CLONE_WITHOUT_TRIGGERS 65

#if defined(OASIS_MPROG)
#define MEDIT_MPROG                     60
#define MEDIT_CHANGE_MPROG              61
#define MEDIT_MPROG_COMLIST             62
#define MEDIT_MPROG_ARGS                63
#define MEDIT_MPROG_TYPE                64
#define MEDIT_PURGE_MPROG               65
#endif

#define MEDIT_RACE			66
#define MEDIT_MAXFACTOR			67

// Medit additional parameters

#define MEDIT_HPREG			1
#define MEDIT_ARMOUR			2
#define MEDIT_MANAREG			3
#define MEDIT_CASTSUCCESS		4
#define MEDIT_SUCCESS			5
#define MEDIT_INITIATIVE		6
#define MEDIT_ABSORBE			7
#define MEDIT_AR			8
#define MEDIT_MR			9
#define MEDIT_PR			10
#define NUM_ADD_PARAMETERS		10

// * Submodes of SEDIT connectedness.
#define SEDIT_MAIN_MENU            	0
#define SEDIT_CONFIRM_SAVESTRING	1
#define SEDIT_NOITEM1			2
#define SEDIT_NOITEM2			3
#define SEDIT_NOCASH1			4
#define SEDIT_NOCASH2			5
#define SEDIT_NOBUY				6
#define SEDIT_BUY				7
#define SEDIT_SELL				8
#define SEDIT_PRODUCTS_MENU		11
#define SEDIT_ROOMS_MENU		12
#define SEDIT_NAMELIST_MENU		13
#define SEDIT_NAMELIST			14
#define SEDIT_CHANGELIST_MENU   15
#define SEDIT_CHANGELIST		16
// * Numerical responses.
#define SEDIT_NUMERICAL_RESPONSE	20
#define SEDIT_OPEN1			    21
#define SEDIT_OPEN2			    22
#define SEDIT_CLOSE1			23
#define SEDIT_CLOSE2			24
#define SEDIT_KEEPER			25
#define SEDIT_BUY_PROFIT		26
#define SEDIT_SELL_PROFIT		27
#define SEDIT_BUYTYPE_MENU   	29
#define SEDIT_DELETE_BUYTYPE	30
#define SEDIT_DELETE_PRODUCT	31
#define SEDIT_NEW_PRODUCT		32
#define SEDIT_DELETE_ROOM		33
#define SEDIT_NEW_ROOM			34
#define SEDIT_SHOP_FLAGS		35
#define SEDIT_NOTRADE			36
#define SEDIT_CHANGE_PROFIT     37
#define SEDIT_DELETE_CHANGETYPE 38
#define SEDIT_CHANGETYPE_MENU   39

// * Limit information.
#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	5003
#define MAX_EXIT_DESC	256
#define MAX_EXTRA_DESC  512
#define MAX_MOB_DESC	512
#define MAX_OBJ_DESC	512

void xedit_disp_ing(DESCRIPTOR_DATA * d, int *ping);
int xparse_ing(DESCRIPTOR_DATA * d, int **pping, char *arg);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
