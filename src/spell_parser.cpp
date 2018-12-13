/* ************************************************************************
*   File: spell_parser.cpp                              Part of Bylins    *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "spell_parser.hpp"

#include "object.prototypes.hpp"
#include "obj.hpp"
#include "interpreter.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "pk.h"
#include "features.hpp"
#include "im.h"
#include "privilege.hpp"
#include "char.hpp"
#include "name_list.hpp"
#include "depot.hpp"
#include "parcel.hpp"
#include "room.hpp"
#include "magic.h"
#include "fight.h"
#include "fight_hit.hpp"
#include "world.characters.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <boost/tokenizer.hpp>

#include <vector>

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct spell_create_type spell_create[TOP_SPELL_DEFINE + 1];
struct skill_info_type skill_info[MAX_SKILL_NUM + 1];
char cast_argument[MAX_STRING_LENGTH];

#define SpINFO spell_info[spellnum]
#define SkINFO skill_info[skillnum]
// Во сколько раз манакост манакастеров меньше манакоста НЕманакастеров
#define DRUID_MANA_COST_MODIFIER 0.5

extern int what_sky;
int check_recipe_values(CHAR_DATA * ch, int spellnum, int spelltype, int showrecipe);

int attack_best(CHAR_DATA * ch, CHAR_DATA * victim);
// local functions
void say_spell(CHAR_DATA * ch, int spellnum, CHAR_DATA * tch, OBJ_DATA * tobj);
void spello(int spl, const char *name, const char *syn, int max_mana, int min_mana, int mana_change,
			int minpos, int targets, int violent, int routines, int danger, int remort, int spell_class);
void do_cast(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_warcry(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_ident(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_create(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_forget(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_remember(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mixture(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stun(CHAR_DATA*, char*, int, int);

void unused_spell(int spl);
void unused_skill(int spl);
void mag_assign_spells(void);

int get_zone_rooms(int, int *, int *);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable
{
	const char *org;
	const char *news;
};


struct syllable syls[] =
{
	{" ", " "},
	{"ar", "abra"},
	{"ate", "i"},
	{"cau", "kada"},
	{"blind", "nose"},
	{"bur", "mosa"},
	{"cu", "judi"},
	{"de", "oculo"},
	{"dis", "mar"},
	{"ect", "kamina"},
	{"en", "uns"},
	{"gro", "cra"},
	{"light", "dies"},
	{"lo", "hi"},
	{"magi", "kari"},
	{"mon", "bar"},
	{"mor", "zak"},
	{"move", "sido"},
	{"ness", "lacri"},
	{"ning", "illa"},
	{"per", "duda"},
	{"ra", "gru"},
	{"re", "candus"},
	{"son", "sabru"},
	{"tect", "infra"},
	{"tri", "cula"},
	{"ven", "nofo"},
	{"word of", "inset"},
	{"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"},
	{"g", "t"},
	{"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"},
	{"n", "b"},
	{"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
	{"u", "e"},
	{"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};


const char *unused_spellname = "!UNUSED!";	// So we can get &unused_spellname

MaxClassSlot max_slots;

int MAGIC_SLOT_VALUES[LVL_IMPL + 1][MAX_SLOT] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// 0
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 3, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 3, 1, 0, 0, 0, 0, 0, 0, 0},
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},
	{4, 4, 3, 0, 0, 0, 0, 0, 0, 0},
	{4, 4, 3, 1, 0, 0, 0, 0, 0, 0},	// 10
	{4, 4, 3, 2, 0, 0, 0, 0, 0, 0},
	{4, 4, 4, 3, 0, 0, 0, 0, 0, 0},
	{4, 4, 4, 3, 1, 0, 0, 0, 0, 0},
	{4, 4, 4, 4, 2, 0, 0, 0, 0, 0},
	{4, 5, 4, 4, 3, 0, 0, 0, 0, 0},
	{5, 5, 4, 4, 3, 1, 0, 0, 0, 0},
	{5, 5, 5, 4, 3, 2, 0, 0, 0, 0},
	{5, 5, 5, 5, 3, 3, 0, 0, 0, 0},
	{5, 5, 5, 5, 4, 3, 1, 0, 0, 0},
	{6, 5, 6, 5, 4, 3, 2, 0, 0, 0},	// 20
	{6, 6, 6, 5, 5, 4, 3, 0, 0, 0},
	{6, 6, 6, 6, 5, 5, 4, 0, 0, 0},
	{6, 6, 6, 6, 5, 5, 4, 1, 0, 0},
	{6, 6, 6, 6, 6, 5, 4, 2, 0, 0},
	{7, 6, 6, 6, 6, 6, 4, 3, 0, 0},
	{7, 7, 6, 6, 6, 6, 5, 4, 0, 0},
	{7, 7, 7, 6, 6, 6, 5, 4, 1, 0},
	{7, 7, 7, 7, 7, 6, 5, 4, 2, 0},
	{7, 7, 7, 7, 7, 6, 5, 5, 3, 0},
	{7, 7, 7, 7, 7, 6, 6, 5, 4, 0},	// 30
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9}	// 34
};

int CLERIC_SLOT_VALUES[LVL_IMPL + 1][MAX_SLOT] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// 0
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{4, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{5, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{5, 3, 0, 0, 0, 0, 0, 0, 0, 0},
	{5, 3, 1, 0, 0, 0, 0, 0, 0, 0},
	{5, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// 10
	{6, 4, 2, 0, 0, 0, 0, 0, 0, 0},
	{6, 4, 3, 0, 0, 0, 0, 0, 0, 0},
	{6, 4, 3, 1, 0, 0, 0, 0, 0, 0},
	{6, 4, 3, 2, 0, 0, 0, 0, 0, 0},
	{6, 5, 4, 2, 0, 0, 0, 0, 0, 0},
	{7, 5, 4, 3, 0, 0, 0, 0, 0, 0},
	{7, 5, 4, 3, 1, 0, 0, 0, 0, 0},
	{7, 5, 4, 4, 2, 0, 0, 0, 0, 0},
	{7, 5, 5, 4, 2, 0, 0, 0, 0, 0},
	{7, 6, 5, 4, 3, 0, 0, 0, 0, 0},	// 20
	{7, 6, 5, 4, 3, 1, 0, 0, 0, 0},
	{8, 6, 5, 5, 3, 2, 0, 0, 0, 0},
	{8, 6, 5, 5, 4, 2, 0, 0, 0, 0},
	{8, 6, 6, 5, 4, 3, 0, 0, 0, 0},
	{8, 6, 6, 5, 4, 3, 1, 0, 0, 0},
	{8, 7, 6, 5, 4, 3, 2, 0, 0, 0},
	{8, 7, 6, 6, 5, 4, 2, 0, 0, 0},
	{8, 7, 6, 6, 5, 4, 3, 0, 0, 0},
	{9, 7, 6, 6, 5, 4, 3, 0, 0, 0},
	{9, 7, 7, 6, 5, 4, 3, 0, 0, 0},	// 30
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9}	// 34
};

int RANGER_SLOT_VALUES[][MAX_SLOT] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// 0
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// 10
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 1, 0, 0, 0, 0, 0, 0, 0},	// 20
	{3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 3, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 3, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 3, 3, 0, 0, 0, 0, 0, 0, 0},	// 30
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9}
};

int PALADINE_SLOT_VALUES[][MAX_SLOT] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// 0
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 1, 0, 0, 0, 0, 0, 0, 0, 0},	// 10
	{2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 2, 0, 0, 0, 0, 0, 0, 0},
	{2, 2, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 2, 0, 0, 0, 0, 0, 0, 0},	// 20
	{3, 2, 2, 0, 0, 0, 0, 0, 0, 0},
	{3, 2, 2, 1, 0, 0, 0, 0, 0, 0},
	{3, 2, 2, 1, 0, 0, 0, 0, 0, 0},
	{3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{3, 3, 3, 1, 0, 0, 0, 0, 0, 0},
	{3, 3, 3, 2, 0, 0, 0, 0, 0, 0},
	{3, 3, 3, 2, 0, 0, 0, 0, 0, 0},
	{3, 3, 3, 3, 0, 0, 0, 0, 0, 0},
	{3, 3, 3, 3, 0, 0, 0, 0, 0, 0},
	{4, 3, 3, 3, 0, 0, 0, 0, 0, 0},	// 30
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	{9, 9, 9, 9, 9, 9, 9, 9, 9, 9}	// 34
};

int BARD_SLOT_VALUES[][6] =  	// Level 3 start
{
	{2, 0, 0, 0, 0, 0},
	{2, 1, 0, 0, 0, 0},
	{3, 1, 0, 0, 0, 0},
	{3, 2, 0, 0, 0, 0},
	{3, 2, 1, 0, 0, 0},
	{3, 3, 1, 0, 0, 0},
	{3, 3, 2, 0, 0, 0},
	{3, 3, 2, 1, 0, 0},
	{3, 3, 3, 1, 0, 0},
	{3, 3, 3, 2, 0, 0},
	{3, 3, 3, 2, 1, 0},
	{3, 3, 3, 3, 1, 0},
	{3, 3, 3, 3, 2, 0},
	{4, 3, 3, 3, 2, 1},
	{4, 4, 3, 3, 3, 1},
	{4, 4, 4, 3, 3, 2},
	{4, 4, 4, 4, 3, 2},
	{4, 4, 4, 4, 4, 3}
};

#define SLOT_LEVELS  30

const int MAG_SLOTS[][MAX_SLOT] = { {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 1
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 2
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 3
	{3, 1, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 4
	{3, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 5
	{4, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 6
	{4, 2, 1, 0, 0, 0, 0, 0, 0, 0},	// lvl 7
	{4, 3, 1, 0, 0, 0, 0, 0, 0, 0},	// lvl 8
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// lvl 9
	{4, 3, 2, 1, 0, 0, 0, 0, 0, 0},	// lvl 10
	{5, 3, 2, 1, 0, 0, 0, 0, 0, 0},	// lvl 11
	{5, 4, 2, 2, 0, 0, 0, 0, 0, 0},	// lvl 12
	{5, 4, 3, 2, 0, 0, 0, 0, 0, 0},	// lvl 13
	{5, 4, 3, 2, 1, 0, 0, 0, 0, 0},	// lvl 14
	{5, 4, 3, 3, 1, 0, 0, 0, 0, 0},	// lvl 15
	{5, 4, 3, 3, 2, 0, 0, 0, 0, 0},	// lvl 16
	{5, 5, 4, 3, 2, 0, 0, 0, 0, 0},	// lvl 17
	{6, 5, 4, 3, 2, 1, 0, 0, 0, 0},	// lvl 18
	{6, 5, 4, 4, 3, 1, 0, 0, 0, 0},	// lvl 19
	{6, 5, 4, 4, 3, 2, 0, 0, 0, 0},	// lvl 20
	{6, 5, 5, 4, 3, 2, 0, 0, 0, 0},	// lvl 21
	{6, 6, 5, 4, 3, 2, 1, 0, 0, 0},	// lvl 22
	{6, 6, 5, 4, 4, 3, 1, 0, 0, 0},	// lvl 23
	{7, 6, 5, 5, 4, 3, 2, 0, 0, 0},	// lvl 24
	{7, 6, 6, 5, 4, 3, 2, 0, 0, 0},	// lvl 25
	{7, 7, 6, 5, 5, 3, 2, 1, 0, 0},	// lvl 26
	{7, 7, 6, 5, 5, 4, 3, 1, 0, 0},	// lvl 27
	{7, 7, 6, 6, 5, 4, 3, 2, 0, 0},	// lvl 28
	{8, 7, 6, 6, 5, 4, 3, 2, 1, 0},	// lvl 29
	{8, 8, 7, 6, 6, 5, 4, 2, 1, 0},	// lvl 30
};

const int NECROMANCER_SLOTS[][MAX_SLOT] = { {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 1
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 2
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 3
	{3, 1, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 4
	{3, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 5
	{4, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 6
	{4, 2, 1, 0, 0, 0, 0, 0, 0, 0},	// lvl 7
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// lvl 8
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// lvl 9
	{4, 3, 3, 1, 0, 0, 0, 0, 0, 0},	// lvl 10
	{5, 3, 3, 1, 0, 0, 0, 0, 0, 0},	// lvl 11
	{5, 4, 3, 2, 0, 0, 0, 0, 0, 0},	// lvl 12
	{5, 4, 4, 2, 0, 0, 0, 0, 0, 0},	// lvl 13
	{5, 4, 4, 2, 1, 0, 0, 0, 0, 0},	// lvl 14
	{5, 4, 4, 3, 1, 0, 0, 0, 0, 0},	// lvl 15
	{5, 4, 4, 3, 2, 0, 0, 0, 0, 0},	// lvl 16
	{5, 5, 4, 3, 2, 0, 0, 0, 0, 0},	// lvl 17
	{6, 5, 5, 3, 2, 1, 0, 0, 0, 0},	// lvl 18
	{6, 5, 5, 4, 3, 1, 0, 0, 0, 0},	// lvl 19
	{6, 5, 5, 4, 3, 2, 0, 0, 0, 0},	// lvl 20
	{6, 5, 5, 4, 3, 2, 0, 0, 0, 0},	// lvl 21
	{6, 6, 5, 4, 3, 2, 1, 0, 0, 0},	// lvl 22
	{6, 6, 6, 4, 4, 3, 1, 0, 0, 0},	// lvl 23
	{7, 6, 6, 5, 4, 3, 2, 0, 0, 0},	// lvl 24
	{7, 6, 6, 5, 4, 3, 2, 0, 0, 0},	// lvl 25
	{7, 7, 6, 5, 5, 3, 2, 1, 0, 0},	// lvl 26
	{7, 7, 6, 5, 5, 4, 3, 1, 0, 0},	// lvl 27
	{7, 7, 7, 6, 5, 4, 3, 2, 0, 0},	// lvl 28
	{8, 7, 7, 6, 5, 4, 3, 2, 1, 0},	// lvl 29
	{8, 8, 7, 6, 6, 5, 4, 2, 1, 0},	// lvl 30
};

#define MIN_CL_LEVEL 1
#define MAX_CL_LEVEL 30
#define MAX_CL_SLOT  8
#define MIN_CL_WIS   10
#define MAX_CL_WIS   35
#define CL_WIS_DIV   1

const int CLERIC_SLOTS[][MAX_CL_SLOT] = { {1, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 10
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 10
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 10
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 10
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 10
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 10
	{2, 1, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 10
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 8 wisdom 10
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 9 wisdom 10
	{3, 2, 1, 1, 0, 0, 0, 0},	// level 10 wisdom 10
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 11 wisdom 10
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 12 wisdom 10
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 13 wisdom 10
	{3, 3, 2, 2, 1, 0, 0, 0},	// level 14 wisdom 10
	{3, 3, 2, 2, 1, 0, 0, 0},	// level 15 wisdom 10
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 16 wisdom 10
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 17 wisdom 10
	{4, 3, 2, 2, 1, 1, 0, 0},	// level 18 wisdom 10
	{4, 3, 3, 2, 2, 1, 0, 0},	// level 19 wisdom 10
	{4, 3, 3, 2, 2, 1, 0, 0},	// level 20 wisdom 10
	{4, 4, 3, 3, 2, 1, 0, 0},	// level 21 wisdom 10
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 22 wisdom 10
	{5, 4, 3, 3, 2, 1, 1, 0},	// level 23 wisdom 10
	{5, 4, 3, 3, 2, 1, 1, 0},	// level 24 wisdom 10
	{5, 4, 3, 3, 2, 2, 1, 0},	// level 25 wisdom 10
	{5, 4, 3, 3, 2, 2, 1, 0},	// level 26 wisdom 10
	{5, 5, 4, 4, 3, 2, 1, 0},	// level 27 wisdom 10
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 28 wisdom 10
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 29 wisdom 10
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 30 wisdom 10
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 11
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 11
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 11
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 11
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 11
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 11
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 11
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 8 wisdom 11
	{3, 2, 1, 0, 0, 0, 0, 0},	// level 9 wisdom 11
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 11
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 11 wisdom 11
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 12 wisdom 11
	{3, 3, 2, 2, 0, 0, 0, 0},	// level 13 wisdom 11
	{3, 3, 2, 2, 1, 0, 0, 0},	// level 14 wisdom 11
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 15 wisdom 11
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 16 wisdom 11
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 17 wisdom 11
	{4, 3, 3, 2, 2, 1, 0, 0},	// level 18 wisdom 11
	{4, 4, 3, 3, 2, 1, 0, 0},	// level 19 wisdom 11
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 20 wisdom 11
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 21 wisdom 11
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 22 wisdom 11
	{5, 4, 3, 3, 2, 2, 1, 0},	// level 23 wisdom 11
	{5, 4, 4, 3, 2, 2, 1, 0},	// level 24 wisdom 11
	{5, 5, 4, 4, 3, 2, 1, 0},	// level 25 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 26 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 27 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 28 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 29 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 30 wisdom 11
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 12
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 12
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 12
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 12
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 12
	{2, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 12
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 12
	{3, 2, 1, 0, 0, 0, 0, 0},	// level 8 wisdom 12
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 12
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 12
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 11 wisdom 12
	{3, 3, 2, 2, 0, 0, 0, 0},	// level 12 wisdom 12
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 13 wisdom 12
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 14 wisdom 12
	{4, 3, 3, 2, 1, 0, 0, 0},	// level 15 wisdom 12
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 16 wisdom 12
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 17 wisdom 12
	{4, 4, 3, 3, 2, 1, 0, 0},	// level 18 wisdom 12
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 19 wisdom 12
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 20 wisdom 12
	{5, 4, 3, 3, 2, 2, 0, 0},	// level 21 wisdom 12
	{5, 4, 4, 3, 2, 2, 0, 0},	// level 22 wisdom 12
	{5, 4, 4, 3, 3, 2, 1, 0},	// level 23 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 24 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 25 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 26 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 27 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 28 wisdom 12
	{6, 5, 5, 4, 3, 2, 1, 1},	// level 29 wisdom 12
	{7, 6, 5, 5, 4, 3, 1, 1},	// level 30 wisdom 12
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 13
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 13
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 13
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 13
	{2, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 13
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 13
	{3, 2, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 13
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 13
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 13
	{3, 3, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 13
	{3, 3, 2, 2, 0, 0, 0, 0},	// level 11 wisdom 13
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 12 wisdom 13
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 13 wisdom 13
	{4, 3, 3, 2, 1, 0, 0, 0},	// level 14 wisdom 13
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 15 wisdom 13
	{4, 4, 3, 2, 2, 0, 0, 0},	// level 16 wisdom 13
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 17 wisdom 13
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 18 wisdom 13
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 19 wisdom 13
	{5, 4, 4, 3, 2, 2, 0, 0},	// level 20 wisdom 13
	{5, 4, 4, 3, 2, 2, 0, 0},	// level 21 wisdom 13
	{5, 5, 4, 4, 3, 2, 0, 0},	// level 22 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 23 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 24 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 25 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 26 wisdom 13
	{6, 5, 5, 4, 3, 2, 2, 0},	// level 27 wisdom 13
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 28 wisdom 13
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 29 wisdom 13
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 30 wisdom 13
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 14
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 14
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 14
	{2, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 14
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 14
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 14
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 14
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 14
	{3, 3, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 14
	{4, 3, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 14
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 11 wisdom 14
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 12 wisdom 14
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 14
	{4, 3, 3, 2, 1, 0, 0, 0},	// level 14 wisdom 14
	{4, 4, 3, 2, 2, 0, 0, 0},	// level 15 wisdom 14
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 16 wisdom 14
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 17 wisdom 14
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 18 wisdom 14
	{5, 4, 4, 3, 2, 1, 0, 0},	// level 19 wisdom 14
	{5, 4, 4, 3, 3, 2, 0, 0},	// level 20 wisdom 14
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 21 wisdom 14
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 22 wisdom 14
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 23 wisdom 14
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 24 wisdom 14
	{6, 5, 5, 4, 3, 2, 2, 0},	// level 25 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 26 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 27 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 28 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 29 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 2},	// level 30 wisdom 14
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 15
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 15
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 15
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 15
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 15
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 15
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 15
	{3, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 15
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 15
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 10 wisdom 15
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 11 wisdom 15
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 15
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 15
	{5, 4, 3, 2, 2, 0, 0, 0},	// level 14 wisdom 15
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 15 wisdom 15
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 16 wisdom 15
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 17 wisdom 15
	{5, 4, 4, 3, 2, 1, 0, 0},	// level 18 wisdom 15
	{5, 5, 4, 3, 3, 2, 0, 0},	// level 19 wisdom 15
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 20 wisdom 15
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 21 wisdom 15
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 22 wisdom 15
	{6, 5, 5, 4, 3, 2, 1, 0},	// level 23 wisdom 15
	{6, 5, 5, 4, 3, 2, 2, 0},	// level 24 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 25 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 26 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 27 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 28 wisdom 15
	{7, 6, 6, 5, 4, 3, 2, 1},	// level 29 wisdom 15
	{8, 7, 6, 6, 5, 3, 2, 2},	// level 30 wisdom 15
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 16
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 16
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 16
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 16
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 16
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 16
	{3, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 16
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 16
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 16
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 16
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 16
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 16
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 16
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 14 wisdom 16
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 15 wisdom 16
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 16 wisdom 16
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 17 wisdom 16
	{6, 5, 4, 3, 3, 1, 0, 0},	// level 18 wisdom 16
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 19 wisdom 16
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 20 wisdom 16
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 21 wisdom 16
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 22 wisdom 16
	{7, 6, 5, 4, 4, 2, 1, 0},	// level 23 wisdom 16
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 24 wisdom 16
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 25 wisdom 16
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 26 wisdom 16
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 27 wisdom 16
	{8, 7, 6, 5, 5, 3, 2, 1},	// level 28 wisdom 16
	{8, 7, 6, 6, 5, 3, 2, 2},	// level 29 wisdom 16
	{8, 7, 6, 6, 5, 4, 2, 2},	// level 30 wisdom 16
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 17
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 17
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 17
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 17
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 17
	{3, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 17
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 17
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 17
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 17
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 17
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 17
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 17
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 17
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 14 wisdom 17
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 17
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 16 wisdom 17
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 17 wisdom 17
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 18 wisdom 17
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 19 wisdom 17
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 20 wisdom 17
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 21 wisdom 17
	{7, 6, 5, 4, 4, 2, 0, 0},	// level 22 wisdom 17
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 23 wisdom 17
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 24 wisdom 17
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 25 wisdom 17
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 26 wisdom 17
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 27 wisdom 17
	{8, 7, 6, 6, 5, 4, 2, 1},	// level 28 wisdom 17
	{8, 7, 6, 6, 5, 4, 3, 2},	// level 29 wisdom 17
	{8, 7, 7, 6, 5, 4, 3, 2},	// level 30 wisdom 17
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 18
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 18
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 18
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 18
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 18
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 18
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 18
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 18
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 18
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 18
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 18
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 18
	{5, 4, 3, 3, 0, 0, 0, 0},	// level 13 wisdom 18
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 18
	{5, 5, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 18
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 16 wisdom 18
	{6, 5, 4, 4, 3, 0, 0, 0},	// level 17 wisdom 18
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 18 wisdom 18
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 19 wisdom 18
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 20 wisdom 18
	{7, 6, 5, 4, 4, 2, 0, 0},	// level 21 wisdom 18
	{7, 6, 5, 5, 4, 3, 0, 0},	// level 22 wisdom 18
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 23 wisdom 18
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 24 wisdom 18
	{8, 7, 6, 5, 4, 3, 2, 0},	// level 25 wisdom 18
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 26 wisdom 18
	{8, 7, 6, 6, 5, 4, 2, 0},	// level 27 wisdom 18
	{8, 7, 6, 6, 5, 4, 3, 1},	// level 28 wisdom 18
	{8, 7, 7, 6, 5, 4, 3, 2},	// level 29 wisdom 18
	{9, 8, 7, 7, 6, 4, 3, 2},	// level 30 wisdom 18
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 19
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 19
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 19
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 19
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 19
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 19
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 19
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 19
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 19
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 19
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 19
	{5, 4, 3, 3, 0, 0, 0, 0},	// level 12 wisdom 19
	{5, 4, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 19
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 19
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 19
	{6, 5, 4, 4, 3, 0, 0, 0},	// level 16 wisdom 19
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 19
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 18 wisdom 19
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 19 wisdom 19
	{7, 6, 5, 4, 4, 2, 0, 0},	// level 20 wisdom 19
	{7, 6, 5, 5, 4, 2, 0, 0},	// level 21 wisdom 19
	{7, 6, 6, 5, 4, 3, 0, 0},	// level 22 wisdom 19
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 23 wisdom 19
	{8, 7, 6, 5, 4, 3, 2, 0},	// level 24 wisdom 19
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 25 wisdom 19
	{8, 7, 6, 6, 5, 4, 2, 0},	// level 26 wisdom 19
	{8, 7, 7, 6, 5, 4, 3, 0},	// level 27 wisdom 19
	{8, 7, 7, 6, 5, 4, 3, 1},	// level 28 wisdom 19
	{9, 8, 7, 7, 6, 4, 3, 2},	// level 29 wisdom 19
	{9, 8, 7, 7, 6, 5, 3, 2},	// level 30 wisdom 19
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 20
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 20
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 20
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 20
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 20
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 20
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 20
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 20
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 20
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 20
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 20
	{5, 4, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 20
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 20
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 20
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 20
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 20
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 20
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 18 wisdom 20
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 19 wisdom 20
	{7, 6, 5, 5, 4, 2, 0, 0},	// level 20 wisdom 20
	{7, 6, 6, 5, 4, 3, 0, 0},	// level 21 wisdom 20
	{7, 6, 6, 5, 4, 3, 0, 0},	// level 22 wisdom 20
	{8, 7, 6, 5, 4, 3, 2, 0},	// level 23 wisdom 20
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 24 wisdom 20
	{8, 7, 7, 6, 5, 4, 2, 0},	// level 25 wisdom 20
	{8, 7, 7, 6, 5, 4, 2, 0},	// level 26 wisdom 20
	{9, 8, 7, 6, 5, 4, 3, 0},	// level 27 wisdom 20
	{9, 8, 7, 7, 6, 4, 3, 1},	// level 28 wisdom 20
	{9, 8, 7, 7, 6, 5, 3, 2},	// level 29 wisdom 20
	{9, 8, 8, 7, 6, 5, 3, 3},	// level 30 wisdom 20
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 21
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 21
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 21
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 21
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 21
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 21
	{5, 3, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 21
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 21
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 21
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 21
	{5, 4, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 21
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 21
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 21
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 21
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 21
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 21
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 21
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 18 wisdom 21
	{7, 6, 5, 5, 4, 2, 0, 0},	// level 19 wisdom 21
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 20 wisdom 21
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 21 wisdom 21
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 22 wisdom 21
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 23 wisdom 21
	{8, 7, 7, 6, 5, 3, 2, 0},	// level 24 wisdom 21
	{8, 7, 7, 6, 5, 4, 2, 0},	// level 25 wisdom 21
	{9, 8, 7, 6, 5, 4, 3, 0},	// level 26 wisdom 21
	{9, 8, 7, 7, 6, 4, 3, 0},	// level 27 wisdom 21
	{9, 8, 8, 7, 6, 5, 3, 1},	// level 28 wisdom 21
	{9, 8, 8, 7, 6, 5, 3, 2},	// level 29 wisdom 21
	{10, 9, 8, 8, 7, 5, 4, 3},	// level 30 wisdom 21
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 22
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 22
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 22
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 22
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 22
	{5, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 22
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 22
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 22
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 22
	{5, 4, 4, 2, 0, 0, 0, 0},	// level 10 wisdom 22
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 22
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 22
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 22
	{6, 5, 5, 4, 2, 0, 0, 0},	// level 14 wisdom 22
	{7, 5, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 22
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 22
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 22
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 22
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 19 wisdom 22
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 20 wisdom 22
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 21 wisdom 22
	{8, 7, 6, 6, 5, 3, 0, 0},	// level 22 wisdom 22
	{8, 7, 7, 6, 5, 3, 2, 0},	// level 23 wisdom 22
	{9, 7, 7, 6, 5, 4, 2, 0},	// level 24 wisdom 22
	{9, 8, 7, 6, 5, 4, 3, 0},	// level 25 wisdom 22
	{9, 8, 7, 7, 6, 4, 3, 0},	// level 26 wisdom 22
	{9, 8, 8, 7, 6, 5, 3, 0},	// level 27 wisdom 22
	{9, 8, 8, 7, 6, 5, 3, 1},	// level 28 wisdom 22
	{10, 9, 8, 8, 7, 5, 4, 2},	// level 29 wisdom 22
	{10, 9, 8, 8, 7, 5, 4, 3},	// level 30 wisdom 22
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 23
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 23
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 23
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 23
	{5, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 23
	{5, 4, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 23
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 23
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 23
	{5, 4, 4, 0, 0, 0, 0, 0},	// level 9 wisdom 23
	{6, 4, 4, 3, 0, 0, 0, 0},	// level 10 wisdom 23
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 23
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 23
	{6, 5, 5, 3, 0, 0, 0, 0},	// level 13 wisdom 23
	{7, 5, 5, 4, 3, 0, 0, 0},	// level 14 wisdom 23
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 23
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 23
	{7, 6, 6, 5, 3, 0, 0, 0},	// level 17 wisdom 23
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 23
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 19 wisdom 23
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 20 wisdom 23
	{8, 7, 6, 6, 5, 3, 0, 0},	// level 21 wisdom 23
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 22 wisdom 23
	{9, 8, 7, 6, 5, 4, 2, 0},	// level 23 wisdom 23
	{9, 8, 7, 6, 5, 4, 2, 0},	// level 24 wisdom 23
	{9, 8, 7, 7, 6, 4, 3, 0},	// level 25 wisdom 23
	{9, 8, 8, 7, 6, 4, 3, 0},	// level 26 wisdom 23
	{9, 8, 8, 7, 6, 5, 3, 0},	// level 27 wisdom 23
	{10, 9, 8, 8, 7, 5, 4, 2},	// level 28 wisdom 23
	{10, 9, 8, 8, 7, 5, 4, 2},	// level 29 wisdom 23
	{10, 9, 9, 8, 7, 6, 4, 3},	// level 30 wisdom 23
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 24
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 24
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 24
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 24
	{5, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 24
	{5, 4, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 24
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 24
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 24
	{6, 4, 4, 0, 0, 0, 0, 0},	// level 9 wisdom 24
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 10 wisdom 24
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 24
	{6, 5, 5, 3, 0, 0, 0, 0},	// level 12 wisdom 24
	{7, 5, 5, 4, 0, 0, 0, 0},	// level 13 wisdom 24
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 14 wisdom 24
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 24
	{7, 6, 6, 4, 3, 0, 0, 0},	// level 16 wisdom 24
	{7, 6, 6, 5, 4, 0, 0, 0},	// level 17 wisdom 24
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 24
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 19 wisdom 24
	{8, 7, 7, 6, 4, 3, 0, 0},	// level 20 wisdom 24
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 21 wisdom 24
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 22 wisdom 24
	{9, 8, 7, 6, 5, 4, 2, 0},	// level 23 wisdom 24
	{9, 8, 8, 7, 6, 4, 2, 0},	// level 24 wisdom 24
	{9, 8, 8, 7, 6, 4, 3, 0},	// level 25 wisdom 24
	{10, 9, 8, 7, 6, 5, 3, 0},	// level 26 wisdom 24
	{10, 9, 8, 8, 7, 5, 3, 0},	// level 27 wisdom 24
	{10, 9, 9, 8, 7, 5, 4, 2},	// level 28 wisdom 24
	{10, 9, 9, 8, 7, 6, 4, 2},	// level 29 wisdom 24
	{10, 9, 9, 8, 7, 6, 4, 3},	// level 30 wisdom 24
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 25
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 25
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 25
	{5, 3, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 25
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 25
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 25
	{5, 4, 3, 0, 0, 0, 0, 0},	//level 7 wisdom 25
	{6, 4, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 25
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 25
	{6, 5, 4, 3, 0, 0, 0, 0},	//level 10 wisdom 25
	{6, 5, 4, 3, 0, 0, 0, 0},	//level 11 wisdom 25
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 12 wisdom 25
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 13 wisdom 25
	{7, 6, 5, 4, 3, 0, 0, 0},	//level 14 wisdom 25
	{7, 6, 5, 4, 3, 0, 0, 0},	//level 15 wisdom 25
	{8, 6, 6, 5, 3, 0, 0, 0},	// level 16 wisdom 25
	{8, 7, 6, 5, 4, 0, 0, 0},	// level 17 wisdom 25
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 25
	{8, 7, 7, 5, 4, 3, 0, 0},	// level 19 wisdom 25
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 20 wisdom 25
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 21 wisdom 25
	{9, 8, 7, 6, 5, 4, 0, 0},	// level 22 wisdom 25
	{9, 8, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 25
	{9, 8, 8, 7, 6, 4, 3, 0},	// level 24 wisdom 25
	{10, 9, 8, 7, 6, 5, 3, 0},	// level 25 wisdom 25
	{10, 9, 8, 8, 7, 5, 3, 0},	// level 26 wisdom 25
	{10, 9, 9, 8, 7, 5, 4, 0},	// level 27 wisdom 25
	{10, 9, 9, 8, 7, 6, 4, 2},	// level 28 wisdom 25
	{11, 10, 9, 8, 7, 6, 4, 2},	// level 29 wisdom 25
	{11, 10, 9, 9, 8, 6, 5, 3},	// level 30 wisdom 25
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 26
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 26
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 26
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 26
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 26
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 26
	{6, 4, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 26
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 26
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 26
	{6, 5, 4, 3, 0, 0, 0, 0},	//level 10 wisdom 26
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 11 wisdom 26
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 12 wisdom 26
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 13 wisdom 26
	{7, 6, 5, 4, 3, 0, 0, 0},	//level 14 wisdom 26
	{8, 6, 6, 4, 3, 0, 0, 0},	// level 15 wisdom 26
	{8, 7, 6, 5, 4, 0, 0, 0},	// level 16 wisdom 26
	{8, 7, 6, 5, 4, 0, 0, 0},	// level 17 wisdom 26
	{8, 7, 7, 5, 4, 2, 0, 0},	// level 18 wisdom 26
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 19 wisdom 26
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 20 wisdom 26
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 21 wisdom 26
	{9, 8, 8, 7, 6, 4, 0, 0},	// level 22 wisdom 26
	{9, 8, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 26
	{10, 9, 8, 7, 6, 4, 3, 0},	// level 24 wisdom 26
	{10, 9, 8, 8, 6, 5, 3, 0},	// level 25 wisdom 26
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 26 wisdom 26
	{10, 9, 9, 8, 7, 5, 4, 0},	// level 27 wisdom 26
	{11, 10, 9, 8, 7, 6, 4, 2},	// level 28 wisdom 26
	{11, 10, 9, 9, 8, 6, 4, 3},	// level 29 wisdom 26
	{11, 10, 10, 9, 8, 6, 5, 4},	// level 30 wisdom 26
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 27
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 27
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 27
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 27
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 27
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 27
	{6, 4, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 27
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 27
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 27
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 27
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 11 wisdom 27
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 12 wisdom 27
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 13 wisdom 27
	{8, 6, 6, 4, 3, 0, 0, 0},	//level 14 wisdom 27
	{8, 7, 6, 5, 3, 0, 0, 0},	//level 15 wisdom 27
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 16 wisdom 27
	{8, 7, 7, 5, 4, 0, 0, 0},	//level 17 wisdom 27
	{9, 7, 7, 6, 4, 2, 0, 0},	//level 18 wisdom 27
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 19 wisdom 27
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 20 wisdom 27
	{9, 8, 8, 7, 5, 3, 0, 0},	//level 21 wisdom 27
	{9, 8, 8, 7, 6, 4, 0, 0},	//level 22 wisdom 27
	{10, 9, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 27
	{10, 9, 8, 8, 6, 5, 3, 0},	// level 24 wisdom 27
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 25 wisdom 27
	{10, 9, 9, 8, 7, 5, 4, 0},	// level 26 wisdom 27
	{11, 10, 9, 8, 7, 6, 4, 0},	// level 27 wisdom 27
	{11, 10, 10, 9, 8, 6, 4, 2},	// level 28 wisdom 27
	{11, 10, 10, 9, 8, 6, 5, 3},	// level 29 wisdom 27
	{11, 10, 10, 9, 8, 7, 5, 4},	// level 30 wisdom 27
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 28
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 28
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 28
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 28
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 28
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 28
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 28
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 28
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 28
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 28
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 11 wisdom 28
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 12 wisdom 28
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 13 wisdom 28
	{8, 7, 6, 4, 3, 0, 0, 0},	//level 14 wisdom 28
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 15 wisdom 28
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 16 wisdom 28
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 17 wisdom 28
	{9, 8, 7, 6, 5, 2, 0, 0},	//level 18 wisdom 28
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 19 wisdom 28
	{9, 8, 8, 6, 5, 3, 0, 0},	//level 20 wisdom 28
	{10, 8, 8, 7, 6, 4, 0, 0},	// level 21 wisdom 28
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 22 wisdom 28
	{10, 9, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 28
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 24 wisdom 28
	{11, 9, 9, 8, 7, 5, 3, 0},	// level 25 wisdom 28
	{11, 10, 9, 8, 7, 6, 4, 0},	// level 26 wisdom 28
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 27 wisdom 28
	{11, 10, 10, 9, 8, 6, 4, 2},	// level 28 wisdom 28
	{12, 10, 10, 9, 8, 7, 5, 3},	// level 29 wisdom 28
	{12, 11, 10, 10, 9, 7, 5, 4},	// level 30 wisdom 28
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 29
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 29
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 29
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 29
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 29
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 29
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 29
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 29
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 29
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 29
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 11 wisdom 29
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 29
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 13 wisdom 29
	{8, 7, 6, 5, 3, 0, 0, 0},	//level 14 wisdom 29
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 15 wisdom 29
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 16 wisdom 29
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 17 wisdom 29
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 18 wisdom 29
	{9, 8, 8, 6, 5, 3, 0, 0},	//level 19 wisdom 29
	{10, 8, 8, 7, 5, 3, 0, 0},	// level 20 wisdom 29
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 21 wisdom 29
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 22 wisdom 29
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 23 wisdom 29
	{11, 9, 9, 8, 7, 5, 3, 0},	// level 24 wisdom 29
	{11, 10, 9, 8, 7, 5, 3, 0},	// level 25 wisdom 29
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 26 wisdom 29
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 27 wisdom 29
	{12, 11, 10, 9, 8, 7, 5, 2},	// level 28 wisdom 29
	{12, 11, 11, 10, 9, 7, 5, 3},	// level 29 wisdom 29
	{12, 11, 11, 10, 9, 7, 6, 4},	// level 30 wisdom 29
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 30
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 30
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 30
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 30
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 30
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 30
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 30
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 30
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 30
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 30
	{8, 6, 5, 4, 0, 0, 0, 0},	//level 11 wisdom 30
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 30
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 13 wisdom 30
	{8, 7, 6, 5, 3, 0, 0, 0},	//level 14 wisdom 30
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 15 wisdom 30
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 16 wisdom 30
	{9, 8, 7, 6, 5, 0, 0, 0},	//level 17 wisdom 30
	{9, 8, 8, 6, 5, 3, 0, 0},	//level 18 wisdom 30
	{10, 8, 8, 7, 5, 3, 0, 0},	// level 19 wisdom 30
	{10, 9, 8, 7, 6, 3, 0, 0},	// level 20 wisdom 30
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 21 wisdom 30
	{10, 9, 9, 8, 6, 4, 0, 0},	// level 22 wisdom 30
	{11, 9, 9, 8, 7, 5, 3, 0},	// level 23 wisdom 30
	{11, 10, 9, 8, 7, 5, 3, 0},	// level 24 wisdom 30
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 25 wisdom 30
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 26 wisdom 30
	{12, 11, 10, 9, 8, 6, 4, 0},	// level 27 wisdom 30
	{12, 11, 11, 10, 9, 7, 5, 2},	// level 28 wisdom 30
	{12, 11, 11, 10, 9, 7, 5, 3},	// level 29 wisdom 30
	{12, 11, 11, 10, 9, 8, 6, 4},	// level 30 wisdom 30
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 31
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 31
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 31
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 31
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 31
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 31
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 31
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 31
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 31
	{8, 6, 5, 4, 0, 0, 0, 0},	//level 10 wisdom 31
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 31
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 31
	{8, 7, 6, 5, 0, 0, 0, 0},	//level 13 wisdom 31
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 31
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 15 wisdom 31
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 16 wisdom 31
	{9, 8, 7, 6, 5, 0, 0, 0},	//level 17 wisdom 31
	{10, 8, 8, 6, 5, 3, 0, 0},	// level 18 wisdom 31
	{10, 9, 8, 7, 5, 3, 0, 0},	// level 19 wisdom 31
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 20 wisdom 31
	{10, 9, 9, 7, 6, 4, 0, 0},	// level 21 wisdom 31
	{11, 9, 9, 8, 7, 4, 0, 0},	// level 22 wisdom 31
	{11, 10, 9, 8, 7, 5, 3, 0},	// level 23 wisdom 31
	{11, 10, 10, 9, 7, 5, 3, 0},	// level 24 wisdom 31
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 25 wisdom 31
	{12, 11, 10, 9, 8, 6, 4, 0},	// level 26 wisdom 31
	{12, 11, 11, 10, 9, 7, 5, 0},	// level 27 wisdom 31
	{12, 11, 11, 10, 9, 7, 5, 2},	// level 28 wisdom 31
	{12, 11, 11, 10, 9, 7, 6, 3},	// level 29 wisdom 31
	{13, 12, 12, 11, 10, 8, 6, 4},	// level 30 wisdom 31
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 32
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 32
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 32
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 32
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 32
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 32
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 32
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 32
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 32
	{8, 6, 5, 4, 0, 0, 0, 0},	//level 10 wisdom 32
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 32
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 32
	{9, 7, 6, 5, 0, 0, 0, 0},	//level 13 wisdom 32
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 32
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 15 wisdom 32
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 16 wisdom 32
	{10, 8, 8, 6, 5, 0, 0, 0},	// level 17 wisdom 32
	{10, 9, 8, 7, 5, 3, 0, 0},	// level 18 wisdom 32
	{10, 9, 8, 7, 6, 3, 0, 0},	// level 19 wisdom 32
	{10, 9, 9, 7, 6, 4, 0, 0},	// level 20 wisdom 32
	{11, 9, 9, 8, 6, 4, 0, 0},	// level 21 wisdom 32
	{11, 10, 9, 8, 7, 5, 0, 0},	// level 22 wisdom 32
	{11, 10, 10, 8, 7, 5, 3, 0},	// level 23 wisdom 32
	{11, 10, 10, 9, 8, 5, 3, 0},	// level 24 wisdom 32
	{12, 11, 10, 9, 8, 6, 4, 0},	// level 25 wisdom 32
	{12, 11, 11, 10, 8, 6, 4, 0},	// level 26 wisdom 32
	{12, 11, 11, 10, 9, 7, 5, 0},	// level 27 wisdom 32
	{13, 11, 11, 10, 9, 7, 5, 2},	// level 28 wisdom 32
	{13, 12, 12, 11, 10, 8, 6, 3},	// level 29 wisdom 32
	{13, 12, 12, 11, 10, 8, 6, 5},	// level 30 wisdom 32
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 33
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 33
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 33
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 33
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 33
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 33
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 33
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 33
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 33
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 10 wisdom 33
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 33
	{9, 7, 6, 5, 0, 0, 0, 0},	//level 12 wisdom 33
	{9, 7, 7, 5, 0, 0, 0, 0},	//level 13 wisdom 33
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 33
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 15 wisdom 33
	{10, 8, 8, 6, 5, 0, 0, 0},	// level 16 wisdom 33
	{10, 9, 8, 6, 5, 0, 0, 0},	// level 17 wisdom 33
	{10, 9, 8, 7, 5, 3, 0, 0},	// level 18 wisdom 33
	{10, 9, 9, 7, 6, 3, 0, 0},	// level 19 wisdom 33
	{11, 9, 9, 8, 6, 4, 0, 0},	// level 20 wisdom 33
	{11, 10, 9, 8, 7, 4, 0, 0},	// level 21 wisdom 33
	{11, 10, 10, 8, 7, 5, 0, 0},	// level 22 wisdom 33
	{11, 10, 10, 9, 7, 5, 3, 0},	// level 23 wisdom 33
	{12, 11, 10, 9, 8, 6, 3, 0},	// level 24 wisdom 33
	{12, 11, 11, 9, 8, 6, 4, 0},	// level 25 wisdom 33
	{12, 11, 11, 10, 9, 7, 4, 0},	// level 26 wisdom 33
	{13, 11, 11, 10, 9, 7, 5, 0},	// level 27 wisdom 33
	{13, 12, 12, 11, 10, 8, 5, 2},	// level 28 wisdom 33
	{13, 12, 12, 11, 10, 8, 6, 3},	// level 29 wisdom 33
	{13, 12, 12, 11, 10, 8, 7, 5},	// level 30 wisdom 33
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 34
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 34
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 34
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 34
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 34
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 34
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 34
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 34
	{8, 6, 6, 0, 0, 0, 0, 0},	//level 9 wisdom 34
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 10 wisdom 34
	{9, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 34
	{9, 7, 7, 5, 0, 0, 0, 0},	//level 12 wisdom 34
	{9, 8, 7, 5, 0, 0, 0, 0},	//level 13 wisdom 34
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 34
	{10, 8, 8, 6, 4, 0, 0, 0},	// level 15 wisdom 34
	{10, 8, 8, 6, 5, 0, 0, 0},	// level 16 wisdom 34
	{10, 9, 8, 7, 5, 0, 0, 0},	// level 17 wisdom 34
	{10, 9, 9, 7, 6, 3, 0, 0},	// level 18 wisdom 34
	{11, 9, 9, 7, 6, 3, 0, 0},	// level 19 wisdom 34
	{11, 10, 9, 8, 6, 4, 0, 0},	// level 20 wisdom 34
	{11, 10, 10, 8, 7, 4, 0, 0},	// level 21 wisdom 34
	{12, 10, 10, 9, 7, 5, 0, 0},	// level 22 wisdom 34
	{12, 11, 10, 9, 8, 5, 3, 0},	// level 23 wisdom 34
	{12, 11, 11, 9, 8, 6, 3, 0},	// level 24 wisdom 34
	{12, 11, 11, 10, 9, 6, 4, 0},	// level 25 wisdom 34
	{13, 11, 11, 10, 9, 7, 5, 0},	// level 26 wisdom 34
	{13, 12, 12, 11, 9, 7, 5, 0},	// level 27 wisdom 34
	{13, 12, 12, 11, 10, 8, 6, 2},	// level 28 wisdom 34
	{13, 12, 12, 11, 10, 8, 6, 3},	// level 29 wisdom 34
	{14, 13, 13, 12, 11, 9, 7, 5},	// level 30 wisdom 34
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 35
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 35
	{7, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 35
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 35
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 35
	{7, 6, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 35
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 35
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 35
	{8, 7, 6, 0, 0, 0, 0, 0},	//level 9 wisdom 35
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 10 wisdom 35
	{9, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 35
	{9, 7, 7, 5, 0, 0, 0, 0},	//level 12 wisdom 35
	{9, 8, 7, 5, 0, 0, 0, 0},	//level 13 wisdom 35
	{10, 8, 7, 6, 4, 0, 0, 0},	// level 14 wisdom 35
	{10, 8, 8, 6, 4, 0, 0, 0},	// level 15 wisdom 35
	{10, 9, 8, 6, 5, 0, 0, 0},	// level 16 wisdom 35
	{10, 9, 8, 7, 5, 0, 0, 0},	// level 17 wisdom 35
	{11, 9, 9, 7, 6, 3, 0, 0},	// level 18 wisdom 35
	{11, 10, 9, 8, 6, 3, 0, 0},	//level 19 wisdom 35
	{11, 10, 10, 8, 7, 4, 0, 0},	// level 20 wisdom 35
	{12, 10, 10, 8, 7, 4, 0, 0},	// level 21 wisdom 35
	{12, 11, 10, 9, 7, 5, 0, 0},	// level 22 wisdom 35
	{12, 11, 11, 9, 8, 5, 3, 0},	//level 23 wisdom 35
	{12, 11, 11, 10, 8, 6, 4, 0},	// level 24 wisdom 35
	{13, 11, 11, 10, 9, 6, 4, 0},	// level 25 wisdom 35
	{13, 12, 12, 10, 9, 7, 5, 0},	// level 26 wisdom 35
	{13, 12, 12, 11, 10, 7, 5, 0},	// level 27 wisdom 35
	{13, 12, 12, 11, 10, 8, 6, 2},	// level 28 wisdom 35
	{14, 13, 13, 12, 11, 8, 6, 3},	// level 29 wisdom 35
	{14, 13, 13, 12, 11, 9, 7, 5},	// level 30 wisdom 35
};

#define MAX_ME_SLOT 4
const int MERCHANT_SLOTS[][MAX_ME_SLOT] = { {0, 0, 0, 0},	// lvl 1
	{0, 0, 0, 0},			// lvl 2
	{0, 0, 0, 0},			// lvl 3
	{1, 0, 0, 0},			// lvl 4
	{2, 0, 0, 0},			// lvl 5
	{2, 0, 0, 0},			// lvl 6
	{3, 0, 0, 0},			// lvl 7
	{3, 0, 0, 0},			// lvl 8
	{3, 0, 0, 0},			// lvl 9
	{4, 0, 0, 0},			// lvl 10
	{4, 0, 0, 0},			// lvl 11
	{4, 1, 0, 0},			// lvl 12
	{4, 2, 0, 0},			// lvl 13
	{5, 2, 0, 0},			// lvl 14
	{5, 3, 0, 0},			// lvl 15
	{5, 3, 0, 0},			// lvl 16
	{5, 3, 0, 0},			// lvl 17
	{5, 4, 1, 0},			// lvl 18
	{6, 4, 2, 0},			// lvl 19
	{6, 4, 2, 0},			// lvl 20
	{6, 4, 3, 0},			// lvl 21
	{6, 5, 3, 0},			// lvl 22
	{6, 5, 3, 1},			// lvl 23
	{6, 5, 4, 2},			// lvl 24
	{7, 5, 4, 2},			// lvl 25
	{7, 5, 4, 3},			// lvl 26
	{7, 6, 4, 3},			// lvl 27
	{7, 6, 5, 3},			// lvl 28
	{7, 6, 5, 4},			// lvl 29
	{7, 6, 5, 4},			// lvl 30
};

#define MIN_PA_LEVEL 8
#define MAX_PA_LEVEL 30
#define MAX_PA_SLOT  4
#define MIN_PA_WIS   10
#define MAX_PA_WIS   30
#define PA_WIS_DIV   2

const int PALADINE_SLOTS[][MAX_PA_SLOT] = { {1, 0, 0, 0},	// lvl 8 wis 10,11
	{1, 0, 0, 0},			// lvl 9
	{1, 0, 0, 0},			// lvl 10
	{1, 0, 0, 0},			// lvl 11
	{1, 0, 0, 0},			// lvl 12
	{1, 0, 0, 0},			// lvl 13
	{1, 1, 0, 0},			// lvl 14
	{2, 1, 0, 0},			// lvl 15
	{2, 1, 0, 0},			// lvl 16
	{2, 1, 0, 0},			// lvl 17
	{2, 1, 1, 0},			// lvl 18
	{2, 1, 1, 0},			// lvl 19
	{2, 1, 1, 0},			// lvl 20
	{2, 2, 1, 0},			// lvl 21
	{3, 2, 1, 0},			// lvl 22
	{3, 2, 1, 1},			// lvl 23
	{3, 2, 1, 1},			// lvl 24
	{3, 2, 2, 1},			// lvl 25
	{3, 2, 2, 1},			// lvl 26
	{3, 2, 2, 1},			// lvl 27
	{3, 3, 2, 1},			// lvl 28
	{4, 3, 2, 1},			// lvl 29
	{4, 3, 2, 1},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 12,13
	{1, 0, 0, 0},			// lvl 9
	{1, 0, 0, 0},			// lvl 10
	{1, 0, 0, 0},			// lvl 11
	{1, 0, 0, 0},			// lvl 12
	{2, 0, 0, 0},			// lvl 13
	{2, 1, 0, 0},			// lvl 14
	{2, 1, 0, 0},			// lvl 15
	{2, 1, 0, 0},			// lvl 16
	{2, 1, 0, 0},			// lvl 17
	{3, 1, 1, 0},			// lvl 18
	{3, 2, 1, 0},			// lvl 19
	{3, 2, 1, 0},			// lvl 20
	{3, 2, 1, 0},			// lvl 21
	{3, 2, 1, 0},			// lvl 22
	{3, 2, 2, 1},			// lvl 23
	{4, 2, 2, 1},			// lvl 24
	{4, 3, 2, 1},			// lvl 25
	{4, 3, 2, 1},			// lvl 26
	{4, 3, 2, 1},			// lvl 27
	{4, 3, 2, 2},			// lvl 28
	{4, 3, 3, 2},			// lvl 29
	{4, 3, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 14,15
	{1, 0, 0, 0},			// lvl 9
	{1, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{2, 0, 0, 0},			// lvl 13
	{2, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 1, 0, 0},			// lvl 16
	{3, 2, 0, 0},			// lvl 17
	{3, 2, 1, 0},			// lvl 18
	{3, 2, 1, 0},			// lvl 19
	{4, 2, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{4, 3, 2, 0},			// lvl 22
	{4, 3, 2, 1},			// lvl 23
	{4, 3, 2, 1},			// lvl 24
	{4, 3, 3, 1},			// lvl 25
	{4, 3, 3, 1},			// lvl 26
	{4, 4, 3, 2},			// lvl 27
	{4, 4, 3, 2},			// lvl 28
	{4, 4, 3, 2},			// lvl 29
	{4, 4, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 16,17
	{1, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{3, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 1, 0, 0},			// lvl 16
	{4, 2, 0, 0},			// lvl 17
	{4, 2, 1, 0},			// lvl 18
	{4, 2, 1, 0},			// lvl 19
	{4, 2, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{5, 3, 2, 0},			// lvl 22
	{5, 3, 2, 1},			// lvl 23
	{5, 3, 2, 1},			// lvl 24
	{5, 3, 3, 1},			// lvl 25
	{5, 4, 3, 1},			// lvl 26
	{5, 4, 3, 2},			// lvl 27
	{5, 4, 3, 2},			// lvl 28
	{5, 4, 3, 2},			// lvl 29
	{5, 4, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 18,19
	{1, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{3, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 2, 0, 0},			// lvl 16
	{4, 2, 0, 0},			// lvl 17
	{4, 2, 1, 0},			// lvl 18
	{4, 3, 1, 0},			// lvl 19
	{4, 3, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{5, 3, 2, 0},			// lvl 22
	{5, 4, 2, 1},			// lvl 23
	{5, 4, 2, 1},			// lvl 24
	{5, 4, 3, 1},			// lvl 25
	{5, 4, 3, 1},			// lvl 26
	{5, 4, 3, 2},			// lvl 27
	{5, 4, 3, 2},			// lvl 28
	{5, 4, 3, 2},			// lvl 29
	{5, 4, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 20,21
	{1, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{3, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 2, 0, 0},			// lvl 16
	{4, 2, 0, 0},			// lvl 17
	{4, 2, 1, 0},			// lvl 18
	{4, 3, 1, 0},			// lvl 19
	{4, 3, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{5, 3, 2, 0},			// lvl 22
	{5, 4, 2, 1},			// lvl 23
	{5, 4, 2, 1},			// lvl 24
	{5, 4, 3, 1},			// lvl 25
	{5, 4, 3, 1},			// lvl 26
	{5, 4, 3, 2},			// lvl 27
	{6, 4, 3, 2},			// lvl 28
	{6, 5, 3, 2},			// lvl 29
	{6, 5, 4, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 22,23
	{2, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{4, 1, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 2, 0, 0},			// lvl 16
	{4, 3, 0, 0},			// lvl 17
	{4, 3, 1, 0},			// lvl 18
	{5, 3, 1, 0},			// lvl 19
	{5, 3, 2, 0},			// lvl 20
	{5, 4, 2, 0},			// lvl 21
	{5, 4, 2, 0},			// lvl 22
	{5, 4, 3, 1},			// lvl 23
	{5, 4, 3, 1},			// lvl 24
	{5, 4, 3, 1},			// lvl 25
	{6, 4, 3, 2},			// lvl 26
	{6, 5, 3, 2},			// lvl 27
	{6, 5, 4, 2},			// lvl 28
	{6, 5, 4, 2},			// lvl 29
	{6, 5, 4, 3},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 24,25
	{2, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{4, 1, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 2, 0, 0},			// lvl 16
	{4, 3, 0, 0},			// lvl 17
	{5, 3, 1, 0},			// lvl 18
	{5, 3, 1, 0},			// lvl 19
	{5, 4, 2, 0},			// lvl 20
	{5, 4, 2, 0},			// lvl 21
	{5, 4, 2, 0},			// lvl 22
	{5, 4, 3, 1},			// lvl 23
	{6, 4, 3, 1},			// lvl 24
	{6, 4, 3, 1},			// lvl 25
	{6, 5, 3, 2},			// lvl 26
	{6, 5, 4, 2},			// lvl 27
	{6, 5, 4, 2},			// lvl 28
	{6, 5, 4, 2},			// lvl 29
	{6, 5, 4, 3},			// lvl 30
	{2, 0, 0, 0},			// lvl 8 wis 26,27
	{2, 0, 0, 0},			// lvl 9
	{3, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{4, 0, 0, 0},			// lvl 13
	{4, 2, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 3, 0, 0},			// lvl 16
	{4, 3, 0, 0},			// lvl 17
	{5, 3, 1, 0},			// lvl 18
	{5, 4, 2, 0},			// lvl 19
	{5, 4, 2, 0},			// lvl 20
	{5, 4, 2, 0},			// lvl 21
	{5, 4, 3, 0},			// lvl 22
	{5, 4, 3, 1},			// lvl 23
	{6, 4, 3, 1},			// lvl 24
	{6, 5, 3, 2},			// lvl 25
	{6, 5, 4, 2},			// lvl 26
	{6, 5, 4, 2},			// lvl 27
	{6, 5, 4, 2},			// lvl 28
	{6, 5, 4, 3},			// lvl 29
	{6, 6, 4, 3},			// lvl 30
	{2, 0, 0, 0},			// lvl 8 wis 28,29
	{2, 0, 0, 0},			// lvl 9
	{3, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{4, 0, 0, 0},			// lvl 13
	{4, 2, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 3, 0, 0},			// lvl 16
	{5, 3, 0, 0},			// lvl 17
	{5, 3, 2, 0},			// lvl 18
	{5, 4, 2, 0},			// lvl 19
	{5, 4, 2, 0},			// lvl 20
	{5, 4, 3, 0},			// lvl 21
	{5, 4, 3, 0},			// lvl 22
	{6, 4, 3, 1},			// lvl 23
	{6, 5, 3, 2},			// lvl 24
	{6, 5, 3, 2},			// lvl 25
	{6, 5, 4, 2},			// lvl 26
	{6, 5, 4, 2},			// lvl 27
	{6, 5, 4, 3},			// lvl 28
	{6, 5, 4, 3},			// lvl 29
	{7, 6, 5, 3},			// lvl 30
	{2, 0, 0, 0},			// lvl 8 wis 30
	{3, 0, 0, 0},			// lvl 9
	{3, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{4, 0, 0, 0},			// lvl 12
	{4, 0, 0, 0},			// lvl 13
	{4, 2, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{5, 3, 0, 0},			// lvl 16
	{5, 3, 0, 0},			// lvl 17
	{5, 3, 2, 0},			// lvl 18
	{5, 4, 2, 0},			// lvl 19
	{6, 4, 3, 0},			// lvl 20
	{6, 4, 3, 0},			// lvl 21
	{6, 4, 3, 0},			// lvl 22
	{6, 5, 3, 2},			// lvl 23
	{6, 5, 3, 2},			// lvl 24
	{6, 5, 4, 2},			// lvl 25
	{6, 5, 4, 2},			// lvl 26
	{7, 5, 4, 3},			// lvl 27
	{7, 6, 4, 3},			// lvl 28
	{7, 6, 5, 3},			// lvl 29
	{7, 6, 5, 3},			// lvl 30
};


int slot_for_char(CHAR_DATA * ch, int slot_num)
{
	int wis_is = -1, /*i, */ wis_line, wis_block;

	if (slot_num < 1 || slot_num > MAX_SLOT || GET_LEVEL(ch) < 1 || IS_NPC(ch))
		return -1;

	if (IS_IMMORTAL(ch))
		return 10;

	if (max_slots.get(GET_CLASS(ch), GET_KIN(ch)) < slot_num)
	{
		return 0;
	}

	slot_num--;

	switch (GET_CLASS(ch))
	{
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
		//wis_is = MAGIC_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num];
		wis_is = MAG_SLOTS[(int) GET_LEVEL(ch) - 1][slot_num];
		break;
	case CLASS_NECROMANCER:
		//wis_is = MAGIC_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num];
		wis_is = NECROMANCER_SLOTS[(int) GET_LEVEL(ch) - 1][slot_num];
		break;
	case CLASS_CLERIC:
		//if ((wis_is = CLERIC_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num]))
		//	for (i = 1; i <= GET_REAL_INT(ch); i++)
		//		wis_is += IS_SET(wis_app[i].spell_additional, 1 << slot_num);
		if (GET_LEVEL(ch) >= MIN_CL_LEVEL && slot_num < MAX_CL_SLOT && GET_REAL_WIS(ch) >= MIN_CL_WIS)
		{
			wis_block = MIN(MAX_CL_WIS, GET_REAL_WIS(ch)) - MIN_CL_WIS;
			wis_block = wis_block / CL_WIS_DIV;
			wis_block = wis_block * (MAX_CL_LEVEL - MIN_CL_LEVEL + 1);
			wis_line = GET_LEVEL(ch) - MIN_CL_LEVEL;
			wis_is = CLERIC_SLOTS[wis_block + wis_line][slot_num];
		}
		break;
	case CLASS_PALADINE:
		//if ((wis_is = PALADINE_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num]))
		//	for (i = 1; i <= GET_REAL_INT(ch); i++)
		//		wis_is += IS_SET(wis_app[i].spell_additional, 1 << slot_num);
		if (GET_LEVEL(ch) >= MIN_PA_LEVEL && slot_num < MAX_PA_SLOT && GET_REAL_WIS(ch) >= MIN_PA_WIS)
		{
			wis_block = MIN(MAX_PA_WIS, GET_REAL_WIS(ch)) - MIN_PA_WIS;
			wis_block = wis_block / PA_WIS_DIV;
			wis_block = wis_block * (MAX_PA_LEVEL - MIN_PA_LEVEL + 1);
			wis_line = GET_LEVEL(ch) - MIN_PA_LEVEL;
			wis_is = PALADINE_SLOTS[wis_block + wis_line][slot_num];
		}
		break;
	/*
	case CLASS_RANGER :
		if ((wis_is = RANGER_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num]))
			for (i = 1; i <= GET_REAL_INT(ch); i++)
				wis_is += IS_SET(wis_app[i].spell_additional, 1 << slot_num);
		break;
	*/
	case CLASS_MERCHANT:
		if (slot_num < MAX_ME_SLOT)
			wis_is = MERCHANT_SLOTS[(int) GET_LEVEL(ch) - 1][slot_num];
		break;
	}

	if (wis_is == -1)
		return 0; //Go here if no magic for char

	return ((wis_is || (GET_REMORT(ch) > slot_num)) ? MIN(25, wis_is + ch->get_obj_slot(slot_num) + GET_REMORT(ch)) : 0);
}

inline int spell_create_level(const CHAR_DATA* ch, int spellnum)
{
	int required_level = spell_create[spellnum].runes.min_caster_level;
	// дабы у простых игроков не отображлись иммовские заклы
	if (required_level >= LVL_GOD)
		return required_level;
	if (can_use_feat(ch, RUNE_ULTIMATE_FEAT))
		required_level -= 6;
	else if (can_use_feat(ch, RUNE_MASTER_FEAT))
		required_level -= 4;
	else if (can_use_feat(ch, RUNE_USER_FEAT))
		required_level -= 2;
	else if (can_use_feat(ch, RUNE_NEWBIE_FEAT))
		required_level -= 1;
	return required_level;
}

//	Коэффициент изменения мема относительно скилла магии.
int koef_skill_magic(int percent_skill)
{
//	Выделяем процент на который меняется мем
	return ((800 - percent_skill) / 8);

//	return 0;
}

int mag_manacost(const CHAR_DATA* ch, int spellnum)
{
	int mana_cost;

	if (IS_IMMORTAL(ch))
	{
		return 1;
	}

//	Мем рунных профессий(на сегодня только волхвы)
	if (IS_MANA_CASTER(ch) && GET_LEVEL(ch) >= spell_create_level(ch, spellnum))
	{
		const auto result = static_cast<int>(DRUID_MANA_COST_MODIFIER
			* (float) mana_gain_cs[VPOSI(55 - GET_REAL_INT(ch), 10, 50)]
			/ (float) int_app[VPOSI(55 - GET_REAL_INT(ch), 10, 50)].mana_per_tic
			* 60
			* MAX(SpINFO.mana_max
				- (SpINFO.mana_change
					* (GET_LEVEL(ch)
						- spell_create[spellnum].runes.min_caster_level)),
				SpINFO.mana_min));

		//Зависимости в таблице несколько корявые, поэтому изменение пустим в обратную сторону
		return result;
	};

//	Мем всех остальных
	if (!IS_MANA_CASTER(ch)
		&& GET_LEVEL(ch) >= MIN_CAST_LEV(SpINFO, ch)
		&& GET_REMORT(ch) >= MIN_CAST_REM(SpINFO, ch))
	{
		mana_cost = MAX(SpINFO.mana_max
			- (SpINFO.mana_change
				* (GET_LEVEL(ch) - MIN_CAST_LEV(SpINFO, ch))),
			SpINFO.mana_min);

		if (SpINFO.class_change[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] < 0)
		{
			mana_cost = mana_cost * (100 - MIN(99, abs(SpINFO.class_change[(int)GET_CLASS(ch)][(int)GET_KIN(ch)]))) / 100;
		}
		else
		{
			mana_cost = mana_cost * 100 / (100 - MIN(99, abs(SpINFO.class_change[(int)GET_CLASS(ch)][(int)GET_KIN(ch)])));
		}

//		Меняем мем на коэффициент скилла магии
		if ((GET_CLASS(ch) == CLASS_DRUID)
			|| (GET_CLASS(ch) == CLASS_PALADINE)
			|| (GET_CLASS(ch) == CLASS_MERCHANT))
		{
			return mana_cost;
		}
		return mana_cost * koef_skill_magic(ch->get_skill(get_magic_skill_number_by_spell(spellnum))) / 100; // при скилле 200 + 25%
	};

	return 9999;
}

void spell_prefix(int spellnum, const char** /*say_to_self*/, const char **say_to_other,
				  const char** /*say_to_obj_vis*/, const char **say_to_something, const char **damagee_vict,
				  const char** /*helpee_vict*/)
{
	switch (spellnum)
	{
	case SPELL_WC_OF_CHALLENGE:
		*say_to_other = "$n гнусным голосом заорал$g : '%s'.";
		*damagee_vict = "$n гнусным голосом заорал$g : '%s'.";
		break;
	case SPELL_WC_OF_MENACE:
		*say_to_other = "$n оскалил$u и взревел$g : '%s'.";
		*damagee_vict = "$n оскалил$u и взревел$g : '%s'.";
		break;
	case SPELL_WC_OF_RAGE:
		*say_to_other = "$n ударил$g себя в грудь и неистово завопил$g : '%s'.";
		*damagee_vict = "$n ударил$g себя в грудь и неистово завопил$g : '%s'.";
		break;
	case SPELL_WC_OF_MADNESS:
		*say_to_other = "$n устрашающе зыркнул$g и зарычал$g : '%s'.";
		*damagee_vict = "$n устрашающе зыркнул$g и зарычал$g : '%s'.";
		break;
	case SPELL_WC_OF_THUNDER:
		*say_to_other = "$n воздел$g руки к небу и оглушающе заревел$g : '%s'.";
		*damagee_vict = "$n воздел$g руки к небу и оглушающе заревел$g : '%s'.";
		break;
	case SPELL_WC_OF_DEFENSE:
		*say_to_something = "$n поднял$g оружие вверх над головой и командным голосом сообщил$g : '%s'.";
		break;
	case SPELL_WC_OF_BATTLE:
		*say_to_something = "$n смело выкрикнул$g : '%s'.";
		break;
	case SPELL_WC_OF_POWER:
		*say_to_something = "$n богатырским гласом загремел$g : '%s'.";
		break;
	case SPELL_WC_OF_BLESS:
		*say_to_something = "$n рявкнул$g : '%s'.";
		break;
	case SPELL_WC_OF_COURAGE:
		*say_to_something = "$n надсаживаясь заорал$g : '%s'.";
		break;
	}
}

// say_spell erodes buf, buf1, buf2
void say_spell(CHAR_DATA * ch, int spellnum, CHAR_DATA * tch, OBJ_DATA * tobj)
{
	char lbuf[256];
	const char *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
	*helpee_vict, *damagee_vict, *format;
	int  religion;

	*buf = '\0';
	strcpy(lbuf, SpINFO.syn);
	// Say phrase ?
	if (IS_NPC(ch))
	{
        switch (GET_RACE(ch))
        {
        case NPC_RACE_EVIL_SPIRIT:
        case NPC_RACE_GHOST:
        case NPC_RACE_HUMAN:
        case NPC_RACE_ZOMBIE:
        case NPC_RACE_SPIRIT:
            religion = number(RELIGION_POLY, RELIGION_MONO);
            if (*cast_phrase[spellnum][religion] != '\n')
                strcpy(buf, cast_phrase[spellnum][religion]);
            say_to_self = "$n пробормотал$g : '%s'.";
            say_to_other = "$n взглянул$g на $N3 и бросил$g : '%s'.";
            say_to_obj_vis = "$n глянул$g на $o3 и произнес$q : '%s'.";
            say_to_something = "$n произнес$q : '%s'.";
            damagee_vict = "$n зыркнул$g на вас и проревел$g : '%s'.";
            helpee_vict = "$n улыбнул$u вам и произнес$q : '%s'.";
            spell_prefix(spellnum, &say_to_self, &say_to_other, &say_to_obj_vis, &say_to_something, &damagee_vict, &helpee_vict);
            break;

        default:
            say_to_self = "$n издал$g непонятный звук.";
            say_to_other = "$n издал$g непонятный звук.";
            say_to_obj_vis = "$n издал$g непонятный звук.";
            say_to_something = "$n издал$g непонятный звук.";
            damagee_vict = "$n издал$g непонятный звук.";
            helpee_vict = "$n издал$g непонятный звук.";

            // say_to_self      = "$n рявкнул$g : '%s'.";
            // say_to_other     = "$n взглянул$g на $N3 и глухо просипел$q : '%s'.";
            // say_to_obj_vis   = "$n посмотрел$g на $o3 и произнес$q : '%s'.";
            // say_to_something = "$n молвил$g : '%s'.";
            // damagee_vict     = "$n злобно рыкнул$g на вас : '%s'.";
            // helpee_vict      = "$n тихо пробормотал$g вам : '%s'.";
	}
	}
	else {
		if (*cast_phrase[spellnum][GET_RELIGION(ch)] != '\n')
			strcpy(buf, cast_phrase[spellnum][GET_RELIGION(ch)]);
		say_to_self = "$n прикрыл$g глаза и прошептал$g : '%s'.";
		say_to_other = "$n взглянул$g на $N3 и произнес$q : '%s'.";
		say_to_obj_vis = "$n посмотрел$g на $o3 и произнес$q : '%s'.";
		say_to_something = "$n произнес$q : '%s'.";
		damagee_vict = "$n зыркнул$g на вас и произнес$q : '%s'.";
		helpee_vict = "$n подмигнул$g вам и произнес$q : '%s'.";
		spell_prefix(spellnum, &say_to_self, &say_to_other, &say_to_obj_vis, &say_to_something, &damagee_vict, &helpee_vict);
	}

	if (!*buf)
	{
		// не понимаю
		/*
		size_t ofs = 0;
		while (lbuf[ofs])
		{
			log("  1  No entry in syllable table for substring of '%s'", lbuf);
			for (j = 0; *(syls[j].org); j++)
			{
				if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org)))
				{
					strcat(buf, syls[j].news);
					ofs += strlen(syls[j].org);
					break;
				}
			}
			//походу это условие должно быть здесь. в противном случае цикл вечный
			// i.e., we didn't find a match in syls[]
			if (!*syls[j].org)
			{
				log("No entry in syllable table for substring of '%s'", lbuf);
				ofs++;
			}
		}
		*/
	}

	if (tch != NULL && IN_ROOM(tch) == ch->in_room)
	{
		if (tch == ch)
		{
			format = say_to_self;
		}
		else
		{
			format = say_to_other;
		}
	}
	else if (tobj != NULL && (tobj->get_in_room() == ch->in_room || tobj->get_carried_by() == ch))
	{
		format = say_to_obj_vis;
	}
	else
	{
		format = say_to_something;
	}

	sprintf(buf1, format, spell_name(spellnum));
	sprintf(buf2, format, buf);

	for (const auto i : world[ch->in_room]->people)
	{
		if (i == ch
			|| i == tch
			|| !i->desc
			|| !AWAKE(i)
			|| AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS))
		{
			continue;
		}

		if (IS_SET(GET_SPELL_TYPE(i, spellnum), SPELL_KNOW | SPELL_TEMP))
		{
			perform_act(buf1, ch, tobj, tch, i);
		}
		else
		{
			perform_act(buf2, ch, tobj, tch, i);
		}
	}

	act(buf1, 1, ch, tobj, tch, TO_ARENA_LISTEN);

	if (tch != NULL
		&& tch != ch
		&& IN_ROOM(tch) == ch->in_room
		&& !AFF_FLAGGED(tch, EAffectFlag::AFF_DEAFNESS))
	{
		if (SpINFO.violent)
		{
			sprintf(buf1, damagee_vict,
				IS_SET(GET_SPELL_TYPE(tch, spellnum), SPELL_KNOW | SPELL_TEMP) ? spell_name(spellnum) : buf);
		}
		else
		{
			sprintf(buf1, helpee_vict,
				IS_SET(GET_SPELL_TYPE(tch, spellnum), SPELL_KNOW | SPELL_TEMP) ? spell_name(spellnum) : buf);
		}
		act(buf1, FALSE, ch, NULL, tch, TO_VICT);
	}
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const char *feat_name(int num)
{
	if (num > 0 && num < MAX_FEATS)
		return (feat_info[num].name);
	else if (num == -1)
		return "UNUSED";
	else
		return "UNDEFINED";
}

const char *skill_name(int num)
{
	if (num > 0 && num <= MAX_SPELLS)
		return (skill_info[num].name);
	else if (num == -1)
		return "UNUSED";
	else
		return "UNDEFINED";
}

const char *spell_name(int num)
{
	if (num > 0 && num <= TOP_SPELL_DEFINE)
		return (spell_info[num].name);
	else if (num == -1)
		return "UNUSED";
	else
		return "UNDEFINED";
}

template <typename T>
void fix_name(T& name)
{
	size_t pos = 0;
	while ('\0' != name[pos] && pos < MAX_STRING_LENGTH)
	{
		if (('.' == name[pos]) || ('_' == name[pos]))
		{
			name[pos] = ' ';
		}
		++pos;
	}
}

void fix_name_feat(char *name)
{
	fix_name(name);
}

ESkill find_skill_num(const char *name)
{
	int ok;
	char const *temp, *temp2;
	char first[256], first2[256];

	for (const auto index : AVAILABLE_SKILLS)
	{
		if (is_abbrev(name, skill_info[index].name))
		{
			return (index);
		}

		ok = TRUE;
		// It won't be changed, but other uses of this function elsewhere may.
		temp = any_one_arg(skill_info[index].name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok)
		{
			if (!is_abbrev(first2, first))
			{
				ok = FALSE;
			}
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
		{
			return (index);
		}
	}

	return SKILL_INVALID;
}

int find_spell_num(const char *name)
{
	int index, ok;
	int use_syn = 0;
	char const *temp, *temp2, *realname;
	char first[256], first2[256];

	use_syn = (((ubyte) * name <= (ubyte) 'z')
			   && ((ubyte) * name >= (ubyte) 'a'))
			  || (((ubyte) * name <= (ubyte) 'Z') && ((ubyte) * name >= (ubyte) 'A'));

	for (index = 1; index <= TOP_SPELL_DEFINE; index++)
	{
		realname = (use_syn) ? spell_info[index].syn : spell_info[index].name;

		if (!realname || !*realname)
			continue;
		if (is_abbrev(name, realname))
			return (index);
		ok = TRUE;
		// It won't be changed, but other uses of this function elsewhere may.
		temp = any_one_arg(realname, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok)
		{
			if (!is_abbrev(first2, first))
				ok = FALSE;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}
		if (ok && !*first2)
			return (index);

	}
	return (-1);
}

ESkill fix_name_and_find_skill_num(char *name)
{
	fix_name(name);
	return find_skill_num(name);
}

ESkill fix_name_and_find_skill_num(std::string& name)
{
	fix_name(name);
	return find_skill_num(name.c_str());
}

int fix_name_and_find_spell_num(char *name)
{
	fix_name(name);
	return find_spell_num(name);
}

int fix_name_and_find_spell_num(std::string& name)
{
	fix_name(name);
	return find_spell_num(name.c_str());
}

int may_cast_in_nomagic(CHAR_DATA * caster, CHAR_DATA* /*victim*/, int spellnum)
{
	// More than 33 level - may cast always
	if (IS_GRGOD(caster) || IS_SET(SpINFO.routines, MAG_WARCRY))
		return TRUE;
	if (IS_NPC(caster) && !(AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)))
		return TRUE;

	return FALSE;
}


// может-ли кастер зачитать заклинание?
int may_cast_here(CHAR_DATA * caster, CHAR_DATA * victim, int spellnum)
{
	int ignore;

	//  More than 33 level - may cast always
	if (IS_GRGOD(caster))
	{
		return TRUE;
	}

	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOBATTLE) && SpINFO.violent)
	{
		return FALSE;
	}

	// не в мирке можно кастовать все что угодно
	if (!ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL))
	{
		return TRUE;
	}

	// Проверяю, что закл имеет одну из допустимых комбинаций параметров
	ignore = IS_SET(SpINFO.targets, TAR_IGNORE)
		|| IS_SET(SpINFO.routines, MAG_MASSES)
		|| IS_SET(SpINFO.routines, MAG_GROUPS);

	// цели нет
	if (!ignore && !victim)
	{
		return TRUE;
	}

	if (ignore && !IS_SET(SpINFO.routines, MAG_MASSES) && !IS_SET(SpINFO.routines, MAG_GROUPS))
	{
		if (SpINFO.violent)
		{
			return FALSE;	// нельзя злые кастовать
		}
		// если игнорируется цель, то должен быть GROUP или MASS
		// в противном случае на цель кастовать нельзя
		return victim == 0 ? TRUE : FALSE;
	}
	// остальные комбинации не проверяю

	// индивидуальная цель
	ignore = victim
		&& (IS_SET(SpINFO.targets, TAR_CHAR_ROOM)
			|| IS_SET(SpINFO.targets, TAR_CHAR_WORLD))
		&& !IS_SET(SpINFO.routines, MAG_AREAS);

	// начинаю проверять условия каста
	// Для добрых заклинаний - проверка на противника цели
	// Для злых заклинаний - проверка на цель
	for (const auto ch_vict : world[caster->in_room]->people)
	{
		if (IS_IMMORTAL(ch_vict))
			continue;	// имморталы на этот процесс не влияют
		if (!HERE(ch_vict))
			continue;	// лд не участвуют
		if (SpINFO.violent && same_group(caster, ch_vict))
			continue;	// на согрупника кастовться не будет
		if (ignore && ch_vict != victim)
			continue;	// только по 1 чару, и не текущему
		// ch_vict может стать потунциальной жертвой
		if (SpINFO.violent)
		{
			if (!may_kill_here(caster, ch_vict))
			{
				return 0;
			}
		}
		else
		{
			if (!may_kill_here(caster, ch_vict->get_fighting()))
			{
				return 0;
			}
		}
	}

	// check_pkill вызвать не могу, т.к. имя жертвы безвозвратно утеряно

	// Ура! Можно
	return (TRUE);
}

int check_mobile_list(CHAR_DATA * ch)
{
	for (const auto& vict : character_list)
	{
		if (vict.get() == ch)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}

// Реакция на каст
void cast_reaction(CHAR_DATA * victim, CHAR_DATA * caster, int spellnum)
{
	// Если оказались под своим заклом то не реагируем на него.
	if (caster == victim)
		return;

	if (!check_mobile_list(victim) || !SpINFO.violent)
		return;

	if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM) ||
			AFF_FLAGGED(victim, EAffectFlag::AFF_SLEEP) ||
			AFF_FLAGGED(victim, EAffectFlag::AFF_BLIND) ||
			AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT) ||
			AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT) || AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD) || IS_HORSE(victim))
		return;

	if (IS_NPC(caster)
			&& GET_MOB_RNUM(caster) == real_mobile(DG_CASTER_PROXY))
		return;

	if (CAN_SEE(victim, caster) && MAY_ATTACK(victim) && IN_ROOM(victim) == IN_ROOM(caster))
	{
		if (IS_NPC(victim))
			attack_best(victim, caster);
		else
			hit(victim, caster, TYPE_UNDEFINED, 1);
	}
	else if (CAN_SEE(victim, caster) && !IS_NPC(caster) && IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY))
	{
		remember(victim, caster);
	}

	if (caster->purged())
	{
		return;
	}
	if (!CAN_SEE(victim, caster) && (GET_REAL_INT(victim) > 25 || GET_REAL_INT(victim) > number(10, 25)))
	{
		if (!AFF_FLAGGED(victim, EAffectFlag::AFF_DETECT_INVIS)
				&& GET_SPELL_MEM(victim, SPELL_DETECT_INVIS) > 0)
			cast_spell(victim, victim, 0, 0, SPELL_DETECT_INVIS,  SPELL_DETECT_INVIS);
		else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_SENSE_LIFE)
				 && GET_SPELL_MEM(victim, SPELL_SENSE_LIFE) > 0)
			cast_spell(victim, victim, 0, 0, SPELL_SENSE_LIFE, SPELL_SENSE_LIFE);
		else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_INFRAVISION)
				 && GET_SPELL_MEM(victim, SPELL_LIGHT) > 0)
			cast_spell(victim, victim, 0, 0, SPELL_LIGHT, SPELL_LIGHT);
	}
}


/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, ROOM_DATA *rvict, int spellnum, int level, int casttype)
{
	int savetype;

	if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
		return (0);

	if (caster && cvict)
	{
		cast_mtrigger(cvict, caster, spellnum);
	}

	// Определяю возможность чтения заклинания
	//******************************************
	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) && !may_cast_in_nomagic(caster, cvict, spellnum))
	{
		send_to_char("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", caster);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.", FALSE, caster, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		return 0;
	}

	if (!may_cast_here(caster, cvict, spellnum))
	{
		if (IS_SET(SpINFO.routines, MAG_WARCRY))
		{
			send_to_char("Ваш громовой глас сотряс воздух, но ничего не произошло!\r\n", caster);
			act("Вы вздрогнули от неожиданного крика, но ничего не произошло.", FALSE, caster, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			send_to_char("Ваша магия обратилась всего лишь в яркую вспышку!\r\n", caster);
			act("Яркая вспышка на миг осветила комнату, и тут же погасла.", FALSE, caster, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		return 0;
	}

	// determine the type of saving throw
	switch (casttype)
	{
	case CAST_STAFF:
	case CAST_SCROLL:
	case CAST_POTION:
	case CAST_WAND:
	case CAST_ITEMS:
	case CAST_RUNES:
	case CAST_SPELL:
		savetype = SAVING_STABILITY;
		break;
	default:
		savetype = SAVING_CRITICAL;
		break;
	}

	if (SpellUsage::isActive)
		SpellUsage::AddSpellStat(GET_CLASS(caster), spellnum);

	// Обработка заклинания
	//******************************************

	// Проверка модификаторов целеуказаний

	if (IS_SET(SpINFO.routines, MAG_MASSES))
		return mag_masses(level, caster, rvict, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_GROUPS))
		return mag_groups(level, caster, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_AREAS))
		return mag_areas(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_ROOM))
		return RoomSpells::mag_room(level, caster, rvict, spellnum);

	return mag_single_target(level, caster, cvict, ovict, spellnum, savetype);
}

void do_ident(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *cvict = NULL, *caster = ch;
	OBJ_DATA *ovict = NULL;
	struct timed_type timed;
	int k, level = 0;

	if (IS_NPC(ch) || ch->get_skill(SKILL_IDENTIFY) <= 0)
	{
		send_to_char("Вам стоит сначала этому научиться.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (timed_by_skill(ch, SKILL_IDENTIFY))
	{
		send_to_char("Вы же недавно опознавали - подождите чуток.\r\n", ch);
		return;
	}

	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, caster, &cvict, &ovict);
	if (!k)
	{
		send_to_char("Похоже, здесь этого нет.\r\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch))
	{
		timed.skill = SKILL_IDENTIFY;
		//imed.time = can_use_feat(ch, CONNOISEUR_FEAT) ? feature_mod(CONNOISEUR_FEAT, FEAT_TIMER) : 12;
		timed.time = MAX((can_use_feat(ch, CONNOISEUR_FEAT) ? feature_mod(CONNOISEUR_FEAT, FEAT_TIMER) : 12) - ((GET_SKILL(ch, SKILL_IDENTIFY) - 25) / 25),1); //12..5 or 8..1
		timed_to_char(ch, &timed);
	}
	MANUAL_SPELL(skill_identify)
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0] level   [1] max charges [2] num charges [3] spell num
 * wand   - [0] level   [1] max charges [2] num charges [3] spell num
 * scroll - [0] level   [1] spell num   [2] spell num   [3] spell num
 * potion - [0] level   [1] spell num   [2] spell num   [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

const char *what_sky_type[] = { "пасмурно",
								"cloudless",
								"облачно",
								"cloudy",
								"дождь",
								"raining",
								"ясно",
								"lightning",
								"\n"
							  };

const char *what_weapon[] = { "плеть",
							  "whip",
							  "дубина",
							  "club",
							  "\n"
							};

/**
* Поиск предмета для каста локейта (без учета видимости для чара и с поиском
* как в основном списке, так и в личных хранилищах с почтой).
*/
OBJ_DATA *find_obj_for_locate(CHAR_DATA *ch, const char *name)
{
//	OBJ_DATA *obj = ObjectAlias::locate_object(name);
	OBJ_DATA *obj = get_obj_vis_for_locate(ch, name);
	if (!obj)
	{
		obj = Depot::locate_object(name);
		if (!obj)
		{
			obj = Parcel::locate_object(name);
		}
	}
	return obj;
}

int find_cast_target(int spellnum, const char *t, CHAR_DATA * ch, CHAR_DATA ** tch, OBJ_DATA ** tobj, ROOM_DATA ** troom)
{
	*tch = NULL;
	*tobj = NULL;
	*troom = world[ch->in_room];
	if (spellnum == SPELL_CONTROL_WEATHER)
	{
		if ((what_sky = search_block(t, what_sky_type, FALSE)) < 0)
		{
			send_to_char("Не указан тип погоды.\r\n", ch);
			return FALSE;
		}
		else
			what_sky >>= 1;
	}

	if (spellnum == SPELL_CREATE_WEAPON)
	{
		if ((what_sky = search_block(t, what_weapon, FALSE)) < 0)
		{
			send_to_char("Не указан тип оружия.\r\n", ch);
			return FALSE;
		}
		else
			what_sky = 5 + (what_sky >> 1);
	}

	strcpy(cast_argument, t);

	if (IS_SET(SpINFO.targets, TAR_ROOM_THIS))
		return TRUE;
	if (IS_SET(SpINFO.targets, TAR_IGNORE))
		return TRUE;
	else if (*t)
	{
		if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM))
		{
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != NULL)
			{
				if (SpINFO.violent && !check_pkill(ch, *tch, t))
					return FALSE;
				return TRUE;
			}
		}

		if (IS_SET(SpINFO.targets, TAR_CHAR_WORLD))
		{
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != NULL)
			{
				// чтобы мобов не чекали
				if (IS_NPC(ch) || !IS_NPC(*tch))
				{
					if (SpINFO.violent && !check_pkill(ch, *tch, t))
						return FALSE;
					return TRUE;
				}
			}
		}

		if (IS_SET(SpINFO.targets, TAR_OBJ_INV))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != NULL)
				return TRUE;

		if (IS_SET(SpINFO.targets, TAR_OBJ_EQUIP))
		{
			int tmp;
			if ((*tobj = get_object_in_equip_vis(ch, t, ch->equipment, &tmp)) != NULL)
				return TRUE;
		}

		if (IS_SET(SpINFO.targets, TAR_OBJ_ROOM))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != NULL)
				return TRUE;

		if (IS_SET(SpINFO.targets, TAR_OBJ_WORLD))
		{
//			if ((*tobj = get_obj_vis(ch, t)) != NULL)
//				return TRUE;
			if (spellnum == SPELL_LOCATE_OBJECT)
			{
				*tobj = find_obj_for_locate(ch, t);
			}
			else
			{
				*tobj = get_obj_vis(ch, t);
			}
			if (*tobj)
			{
				return true;
			}
		}
	}
	else
	{
		if (IS_SET(SpINFO.targets, TAR_FIGHT_SELF))
			if (ch->get_fighting() != NULL)
			{
				*tch = ch;
				return TRUE;
			}
		if (IS_SET(SpINFO.targets, TAR_FIGHT_VICT))
			if (ch->get_fighting() != NULL)
			{
				*tch = ch->get_fighting();
				return TRUE;
			}
		if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM) && !SpINFO.violent)
		{
			*tch = ch;
			return TRUE;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	if (IS_SET(SpINFO.routines, MAG_WARCRY))
		sprintf(buf, "И на %s же вы хотите так громко крикнуть?\r\n",
				IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)
				? "ЧТО" : "КОГО");
	else
		sprintf(buf, "На %s Вы хотите ЭТО колдовать?\r\n",
				IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)
				? "ЧТО" : "КОГО");
	send_to_char(buf, ch);
	return FALSE;
}

int find_cast_target(int spellnum, const std::string &t, CHAR_DATA * ch, CHAR_DATA ** tch, OBJ_DATA ** tobj, ROOM_DATA ** troom)
{
	*tch = NULL;
	*tobj = NULL;
	*troom = world[ch->in_room];

	if (spellnum == SPELL_CONTROL_WEATHER)
	{
		if ((what_sky = search_block(t, what_sky_type, FALSE)) < 0)
		{
			send_to_char("Не указан тип погоды.\r\n", ch);
			return FALSE;
		}
		else
			what_sky >>= 1;
	}

	if (spellnum == SPELL_CREATE_WEAPON)
	{
		if ((what_sky = search_block(t, what_weapon, FALSE)) < 0)
		{
			send_to_char("Не указан тип оружия.\r\n", ch);
			return FALSE;
		}
		else
			what_sky = 5 + (what_sky >> 1);
	}

	cast_argument[t.copy(cast_argument, MAX_INPUT_LENGTH - 1, 0)] = '\0';

	if (IS_SET(SpINFO.targets, TAR_ROOM_THIS))
		return TRUE;
	if (IS_SET(SpINFO.targets, TAR_IGNORE))
		return TRUE;
	if (t.length())
	{
		if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM))
		{
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != NULL)
			{
				if (SpINFO.violent && !check_pkill(ch, *tch, t))
					return FALSE;
				return TRUE;
			}
		}

		if (IS_SET(SpINFO.targets, TAR_CHAR_WORLD))
		{
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != NULL)
			{
				// чтобы мобов не чекали
				if (IS_NPC(ch) || !IS_NPC(*tch))
				{
					if (SpINFO.violent && !check_pkill(ch, *tch, t))
						return FALSE;
					return TRUE;
				}
			}
		}

		if (IS_SET(SpINFO.targets, TAR_OBJ_INV))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != NULL)
				return TRUE;

		if (IS_SET(SpINFO.targets, TAR_OBJ_EQUIP))
		{
			int i;
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->get_aliases()))
				{
					*tobj = GET_EQ(ch, i);
					return TRUE;
				}
		}

		if (IS_SET(SpINFO.targets, TAR_OBJ_ROOM))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != NULL)
				return TRUE;

		if (IS_SET(SpINFO.targets, TAR_OBJ_WORLD))
			if ((*tobj = get_obj_vis(ch, t)) != NULL)
				return TRUE;
	}
	else
	{
		if (IS_SET(SpINFO.targets, TAR_FIGHT_SELF))
			if (ch->get_fighting() != NULL)
			{
				*tch = ch;
				return TRUE;
			}
		if (IS_SET(SpINFO.targets, TAR_FIGHT_VICT))
			if (ch->get_fighting() != NULL)
			{
				*tch = ch->get_fighting();
				return TRUE;
			}
		if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM) && !SpINFO.violent)
		{
			*tch = ch;
			return TRUE;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	if (IS_SET(SpINFO.routines, MAG_WARCRY))
		sprintf(buf, "И на %s же вы хотите так громко крикнуть?\r\n",
				IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)
				? "ЧТО" : "КОГО");
	else
		sprintf(buf, "На %s вы хотите ЭТО колдовать?\r\n",
				IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)
				? "ЧТО" : "КОГО");
	send_to_char(buf, ch);
	return FALSE;
}

void mag_objectmagic(CHAR_DATA * ch, OBJ_DATA * obj, const char *argument)
{
	int i, spellnum;
	int level;
	CHAR_DATA *tch = NULL;
	OBJ_DATA *tobj = NULL;
	ROOM_DATA *troom = NULL;

	one_argument(argument, cast_argument);
	level = GET_OBJ_VAL(obj, 0);
	if (level == 0)
	{
		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF)
		{
			level = DEFAULT_STAFF_LVL;
		}
		else if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WAND)
		{
			level = DEFAULT_WAND_LVL;
		}
	}

	if (obj->get_extra_flag(EExtraFlag::ITEM_TIMEDLVL))
	{
		int proto_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		if (proto_timer != 0)
		{
			level -= level * (proto_timer - obj->get_timer()) / proto_timer;
		}
	}

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_STAFF:
		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("Вы ударили $o4 о землю.", FALSE, ch, obj, 0, TO_CHAR);
		}

		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("$n ударил$g $o4 о землю.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}

		if (GET_OBJ_VAL(obj, 2) <= 0)
		{
			send_to_char("Похоже, кончились заряды :)\r\n", ch);
			act("И ничего не случилось.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			obj->dec_val(2);
			WAIT_STATE(ch, PULSE_VIOLENCE);

			/*
			 * Problem : Area/mass spells on staves can cause crashes.
			 * Solution: Remove the special nature of area/mass spells on staves.
			 * Problem : People like that behavior.
			 * Solution: We special case the area/mass spells here.
			 */
			if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS))
			{
				call_magic(ch, NULL, NULL, world[ch->in_room], GET_OBJ_VAL(obj, 3), level, CAST_STAFF);
			}
			else
			{
				const auto people_copy = world[ch->in_room]->people;
				for (const auto tch : people_copy)
				{
					if (ch != tch)
					{
						call_magic(ch, tch, NULL, world[ch->in_room], GET_OBJ_VAL(obj, 3), level, CAST_STAFF);
					}
				}
			}
		}

		break;

	case OBJ_DATA::ITEM_WAND:
		spellnum = GET_OBJ_VAL(obj, 3);

		if (GET_OBJ_VAL(obj, 2) <= 0)
		{
			send_to_char("Похоже, магия кончилась.\r\n", ch);
			//act("И ничего не произошло.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
			//перемещено чтобы не было возможности читерно использовать пустую палку с локейтом
			return;
		}

		if (!*argument)
		{
			tch = ch;
		}
		else if (!find_cast_target(spellnum, argument, ch, &tch, &tobj, &troom))
		{
			return;
		}

		if (tch)
		{
			if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES))  	// Wands with area spells don't need to be pointed.
			{
				if (!obj->get_action_description().empty())
				{
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
				}
				else
				{
					act("Вы обвели $o4 вокруг комнаты.", FALSE, ch, obj, NULL, TO_CHAR);
				}

				if (!obj->get_action_description().empty())
				{
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				}
				else
				{
					act("$n обвел$g $o4 вокруг комнаты.", TRUE, ch, obj, NULL, TO_ROOM | TO_ARENA_LISTEN);
				}
			}
			else if (tch == ch)
			{
				if (!obj->get_action_description().empty())
				{
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
				}
				else
				{
					act("Вы указали $o4 на себя.", FALSE, ch, obj, 0, TO_CHAR);
				}

				if (!obj->get_action_description().empty())
				{
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				}
				else
				{
					act("$n указал$g $o4 на себя.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
				}
			}
			else
			{
				if (!obj->get_action_description().empty())
				{
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
				}
				else
				{
					act("Вы ткнули $o4 в $N3.", FALSE, ch, obj, tch, TO_CHAR);
				}

				if (!obj->get_action_description().empty())
				{
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				}
				else
				{
					act("$n ткнул$g $o4 в $N3.", TRUE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				}
			}
		}
		else if (tobj)
		{
			if (!obj->get_action_description().empty())
			{
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tobj, TO_CHAR);
			}
			else
			{
				act("Вы прикоснулись $o4 к $O2.", FALSE, ch, obj, tobj, TO_CHAR);
			}

			if (!obj->get_action_description().empty())
			{
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tobj, TO_ROOM | TO_ARENA_LISTEN);
			}
			else
			{
				act("$n прикоснул$u $o4 к $O2.", TRUE, ch, obj, tobj, TO_ROOM | TO_ARENA_LISTEN);
			}
		}

		obj->dec_val(2);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		call_magic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, 3), level, CAST_WAND);
		break;

	case OBJ_DATA::ITEM_SCROLL:
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Вы немы, как рыба.\r\n", ch);
			return;
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		{
			send_to_char("Вы ослеплены.\r\n", ch);
			return;
		}

		spellnum = GET_OBJ_VAL(obj, 1);
		if (!*argument)
			tch = ch;
		else if (!find_cast_target(spellnum, argument, ch, &tch, &tobj, &troom))
			return;

		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, NULL, TO_CHAR);
		}
		else
		{
			act("Вы зачитали $o3, котор$W рассыпался в прах.", TRUE, ch, obj, 0, TO_CHAR);
		}

		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, NULL, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("$n зачитал$g $o3.", FALSE, ch, obj, NULL, TO_ROOM | TO_ARENA_LISTEN);
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
		{
			if (call_magic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, i), level, CAST_SCROLL) <= 0)
			{
				break;
			}
		}

		if (obj != NULL)
		{
			extract_obj(obj);
		}
		break;

	case OBJ_DATA::ITEM_POTION:
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
			return;
		}
		tch = ch;
		act("Вы осушили $o3.", FALSE, ch, obj, NULL, TO_CHAR);
		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, NULL, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("$n осушил$g $o3.", TRUE, ch, obj, NULL, TO_ROOM | TO_ARENA_LISTEN);
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
		{
			if (call_magic(ch, ch, NULL, world[ch->in_room], GET_OBJ_VAL(obj, i), level, CAST_POTION) <= 0)
			{
				break;
			}
		}

		if (obj != NULL)
		{
			extract_obj(obj);
		}
		break;

	default:
		log("SYSERR: Unknown object_type %d in mag_objectmagic.", GET_OBJ_TYPE(obj));
		break;
	}
}

/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */
int cast_spell(CHAR_DATA * ch, CHAR_DATA * tch, OBJ_DATA * tobj, ROOM_DATA * troom, int spellnum, int spell_subst)
{
	int ignore;
	ESkill skillnum = SKILL_INVALID;

	if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
	{
		log("SYSERR: cast_spell trying to call spellnum %d/%d.\n", spellnum, TOP_SPELL_DEFINE);
		return (0);
	}
//проверка на алайнмент мобов

	if (tch && ch)
	{
		if (IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !SpINFO.violent)
		{
			return (0);
		}
	}

	if (!troom)
	{
		// Вызвали с пустой комнатой значит будем кастить тут
		troom = world[ch->in_room];
	}

	if (GET_POS(ch) < SpINFO.min_position)
	{
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			send_to_char("Вы спите и не могете думать больше ни о чем.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("Вы расслаблены и отдыхаете. И далась вам эта магия?\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("Похоже, в этой позе Вы много не наколдуете.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Невозможно! Вы сражаетесь! Это вам не шухры-мухры.\r\n", ch);
			break;
		default:
			send_to_char("Вам вряд ли это удастся.\r\n", ch);
			break;
		}
		return (0);
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& ch->get_master() == tch)
	{
		send_to_char("Вы не посмеете поднять руку на вашего повелителя!\r\n", ch);
		return (0);
	}

	if (tch != ch
		&& !IS_IMMORTAL(ch)
		&& IS_SET(SpINFO.targets, TAR_SELF_ONLY))
	{
		send_to_char("Вы можете колдовать это только на себя!\r\n", ch);
		return (0);
	}

	if (tch == ch && IS_SET(SpINFO.targets, TAR_NOT_SELF))
	{
		send_to_char("Колдовать? ЭТО? На себя?! Да вы с ума сошли!\r\n", ch);
		return (0);
	}

	if ((!tch || IN_ROOM(tch) == NOWHERE) && !tobj && !troom &&
			IS_SET(SpINFO.targets,
				   TAR_CHAR_ROOM | TAR_CHAR_WORLD | TAR_FIGHT_SELF | TAR_FIGHT_VICT
				   | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_WORLD | TAR_OBJ_EQUIP | TAR_ROOM_THIS
				   | TAR_ROOM_DIR))
	{
		send_to_char("Цель заклинания не доступна.\r\n", ch);
		return (0);
	}

	// add by Pereplut: если цель уйдет из клетки в случае кастинга в бою (с лагом) - то
	// не будет кастоваться на соседние клетки, в случае если закл не глобальный
	if (tch && ch && IN_ROOM(tch) != ch->in_room)
	{
		if (!IS_SET(SpINFO.targets, TAR_CHAR_WORLD))
		{
			send_to_char("Цель заклинания не доступна.\r\n", ch);
			return (0);
		}
	}

	// Начало изменений. (с) Дмитрий ака dzMUDiST ака Кудояр

// Может-ли кастер зачитать заклинание если на нем эфект !смирение!?
// Далее идет код _аналогичный_ коду функции may_cast_here()
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_PEACEFUL))
		// Проверяю, что закл имеет одну из допустимых комбинаций параметров
		// если игнорируется цель, то должен быть GROUP или MASS
		// в противном случае на цель кастовать нельзя
	{
		ignore = IS_SET(SpINFO.targets, TAR_IGNORE) ||
				 IS_SET(SpINFO.routines, MAG_MASSES) || IS_SET(SpINFO.routines, MAG_GROUPS);

		if (ignore)
		{
			if (SpINFO.violent)
			{
				send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
				return FALSE;	// нельзя злые кастовать
			}
		}
		// индивидуальная цель

		// начинаю проверять условия каста
		// Для добрых заклинаний - проверка на противника цели
		// Для злых заклинаний - проверка на цель
		for (const auto ch_vict : world[ch->in_room]->people)
		{
			if (SpINFO.violent)
			{
				if (ch_vict == tch)
				{
					send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return FALSE;
				}
			}
			else
			{
				if (ch_vict == tch
					&& !same_group(ch, ch_vict))
				{
					send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return FALSE;
				}
			}
		}
	}
	// Конец изменений. (с) Дмитрий ака dzMUDiST ака Кудояр
	if (!IS_NPC(ch))
	{
		if (PRF_FLAGGED(ch, PRF_NOREPEAT)) //если включен режим без повторов (подавление ехо) не показываем
		{
			if (!ch->get_fighting()) //если персонаж не в бою, шлем строчку, если в бою ничего не шлем
				send_to_char(OK, ch);
		}
		else
		{
			//если режима нет, ехо включено, выводим сообщение
			sprintf(buf, "Вы произнесли заклинание \"%s%s%s\".\r\n",
					CCICYN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}
	//тренируем магические компетенции
	skillnum = get_magic_skill_number_by_spell(spellnum);
	if (skillnum > 0)
	{
		train_skill(ch, skillnum, skill_info[skillnum].max_percent, tch);
	}
	// Комнату тут в say_spell не обрабатываем - будет сказал "что-то"
	say_spell(ch, spellnum, tch, tobj);
	if (GET_SPELL_MEM(ch, spell_subst) > 0)
		GET_SPELL_MEM(ch, spell_subst)--;
	else
		GET_SPELL_MEM(ch, spell_subst) = 0;
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
		MemQ_remember(ch, spell_subst);
	if (!IS_NPC(ch))
	{
		/*log("[CAST_SPELL->AFFECT_TOTAL] Start <%s(%d)> <%s(%d)> <%s> <%d>",
		   GET_NAME(ch),
		   ch->in_room,
		   tch  ? GET_NAME(tch)   : "-",
		   tch  ? IN_ROOM(tch)    : -2,
		   tobj ? tobj->PNames[0] : "-",
		   spellnum); */
		affect_total(ch);
		//log("[CAST_SPELL->AFFECT_TOTAL] Stop");
	}
	else
	{
		GET_CASTER(ch) -= (IS_SET(spell_info[spellnum].routines, NPC_CALCULATE) ? 1 : 0);
	}
	return (call_magic(ch, tch, tobj, troom, spellnum, GET_LEVEL(ch), CAST_SPELL));
}

int spell_use_success(CHAR_DATA * ch, CHAR_DATA * victim, int casting_type, int spellnum)
{
	int prob = 100;
	int skill;
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
	{
		return TRUE;
	}

	switch (casting_type)
	{
	case SAVING_STABILITY:
	case SAVING_NONE:
		prob = wis_bonus(GET_REAL_WIS(ch), WIS_FAILS) + GET_CAST_SUCCESS(ch);

		if ((IS_MAGE(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_MAGE))
			|| (IS_CLERIC(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_CLERIC))
			|| (IS_PALADINE(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_PALADINE))
			|| (IS_MERCHANT(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_MERCHANT)))
		{
			prob += 10;
		}

		if (IS_MAGE(ch) && equip_in_metall(ch))
		{
			prob -= 50;
		}
		break;
	}

	if (equip_in_metall(ch) && !can_use_feat(ch, COMBAT_CASTING_FEAT))
	{
		prob -= 50;
	}

	prob = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_SUCCESS, prob);
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
		(SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSLIKE)) ||
		(!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSCURSE)))
	{
		prob -= 50;
	}

	if ((SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSCURSE)) ||
		(!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSLIKE)))
	{
		prob += 50;
	}

	if (IS_NPC(ch) && (GET_LEVEL(ch) >= STRONG_MOB_LEVEL))
	{
		prob += GET_LEVEL(ch) - 20;
	}

	const ESkill skill_number = get_magic_skill_number_by_spell(spellnum);
	if (skill_number > 0)
	{
		skill = ch->get_skill(skill_number) / 20;
		prob += skill;
	}
	//prob*=ch->get_cond_penalty(P_CAST);
	// умения магии дают + к успеху колдовства
	return (prob > number(0, 100));
}

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */
void do_cast(CHAR_DATA *ch, char *argument, int/* cmd*/, int /*subcmd*/)
{
	CHAR_DATA *tch;
	OBJ_DATA *tobj;
	ROOM_DATA *troom;

	char *s, *t;
	int i, spellnum, spell_subst, target = 0;

	if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}

	if (!ch->affected.empty())
	{
		for (const auto& aff : ch->affected)
		{
			if (aff->location == APPLY_PLAQUE
				&& number(1, 100) < 10) // лихорадка 10% фэйл закла
			{
				send_to_char("Вас трясет лихорадка, вы не смогли сконцентрироваться на произнесении заклинания.\r\n", ch);
				return;
			}

			if (aff->location == APPLY_MADNESS
				&& number(1, 100) < 20) // безумие 20% фэйл закла
			{
				send_to_char("Начав безумно кричать, вы забыли, что хотели произнести.\r\n", ch);
				return;
			}
		}
	}


	if (ch->is_morphed())
	{
		send_to_char("Вы не можете произносить заклинания в звериной форме.\r\n", ch);
		return;
	}

	// get: blank, spell name, target name
	s = strtok(argument, "'*!");
	if (s == NULL)
	{
		send_to_char("ЧТО вы хотите колдовать?\r\n", ch);
		return;
	}
	s = strtok(NULL, "'*!");
	if (s == NULL)
	{
		send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	t = strtok(NULL, "\0");

	spellnum = fix_name_and_find_spell_num(s);
	spell_subst = spellnum;

/*	if (!Privilege::check_spells(ch, spellnum))
	{
		send_to_char("Не положено...\r\n", ch);
		return;
	}
*/
	// Unknown spell
	if (spellnum < 1 || spellnum > MAX_SPELLS)
	{
		send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	// Caster is lower than spell level
	if ((!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP | SPELL_KNOW) ||
			GET_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)) &&
			(GET_LEVEL(ch) < LVL_GRGOD) && (!IS_NPC(ch)))
	{
		if (GET_LEVEL(ch) < MIN_CAST_LEV(SpINFO, ch)
				|| GET_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)
				||  slot_for_char(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0)
		{
			send_to_char("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		}
		else
		{
			send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	// Caster havn't slot
	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch))
	{
		if (can_use_feat(ch, SPELL_SUBSTITUTE_FEAT)
				&& (spellnum == SPELL_CURE_LIGHT || spellnum == SPELL_CURE_SERIOUS
					|| spellnum == SPELL_CURE_CRITIC || spellnum == SPELL_HEAL))
		{

			for (i = 1; i <= MAX_SPELLS; i++)
			{
				if (GET_SPELL_MEM(ch, i) &&
					spell_info[i].slot_forc[(int)GET_CLASS(ch)][(int)GET_KIN(ch)] ==
					spell_info[spellnum].slot_forc[(int)GET_CLASS(ch)][(int)GET_KIN(ch)])
				{
					spell_subst = i;
					break;
				}
			}
			if (i > SPELLS_COUNT)
			{
				send_to_char("У вас нет заученных заклинаний этого круга.\r\n", ch);
				return;
			}


		}
		else
		{
			send_to_char("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
			return;
		}
	}

	// Find the target
	if (t != NULL)
		one_argument(t, arg);
	else
		*arg = '\0';

	target = find_cast_target(spellnum, arg, ch, &tch, &tobj, &troom);

	if (target && (tch == ch) && SpINFO.violent)
	{
		send_to_char("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
		return;
	}

	if (!target)
	{
		send_to_char("Тяжеловато найти цель вашего заклинания!\r\n", ch);
		return;
	}

	// You throws the dice and you takes your chances.. 101% is total failure
	// Чтобы в бой не вступал с уже взведенной заклинашкой !!!
	ch->set_cast(0, 0, 0, 0, 0);

	if (!spell_use_success(ch, tch, SAVING_STABILITY, spellnum))
	{
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
			WAIT_STATE(ch, PULSE_VIOLENCE);
		if (GET_SPELL_MEM(ch, spell_subst))
		{
			GET_SPELL_MEM(ch, spell_subst)--;
		}
		if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
			MemQ_remember(ch, spell_subst);
		//log("[DO_CAST->AFFECT_TOTAL] Start");
		affect_total(ch);
		//log("[DO_CAST->AFFECT_TOTAL] Stop");
		if (!tch || !skill_message(0, ch, tch, spellnum))
			send_to_char("Вы не смогли сосредоточиться!\r\n", ch);
	}
	else  		// cast spell returns 1 on success; subtract mana & set waitstate
	{
		if (ch->get_fighting() && !IS_IMPL(ch))
		{
			ch->set_cast(spellnum, spell_subst, tch, tobj, troom);
			sprintf(buf,
					"Вы приготовились применить заклинание %s'%s'%s%s.\r\n",
					CCCYN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM),
					tch == ch ? " на себя" : tch ? " на $N3" : tobj ? " на $o3" : troom ? " на всех" : "");
			act(buf, FALSE, ch, tobj, tch, TO_CHAR);
		}
		else if (cast_spell(ch, tch, tobj, troom, spellnum, spell_subst) >= 0)
		{
			if (!(WAITLESS(ch) || CHECK_WAIT(ch)))
				WAIT_STATE(ch, PULSE_VIOLENCE);
		}
	}
}

void do_warcry(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int spellnum, cnt;

	if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (!ch->get_skill(SKILL_WARCRY))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}

	std::string buffer = argument;
	typedef boost::tokenizer<pred_separator> tokenizer;
	pred_separator sep;
	tokenizer tok(buffer, sep);
	tokenizer::iterator tok_iter = tok.begin();

	if (tok_iter == tok.end())
	{
		sprintf(buf, "Вам доступны :\r\n");
		for (cnt = spellnum = 1; spellnum <= SPELLS_COUNT; spellnum++)
		{
			const char *realname = SpINFO.name && *SpINFO.name ? SpINFO.name : SpINFO.syn && *SpINFO.syn ? SpINFO.syn : NULL;

			if (realname
				&& IS_SET(SpINFO.routines, MAG_WARCRY)
				&& ch->get_skill(SKILL_WARCRY) >= SpINFO.mana_change)
			{
				if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
					continue;
				sprintf(buf + strlen(buf), "%s%2d%s) %s%s%s\r\n",
					CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM),
					SpINFO.violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), realname, CCNRM(ch, C_NRM));
			}
		}
		send_to_char(buf, ch);
		return;
	}

	std::string wc_name = *(tok_iter++);

	if (CompareParam(wc_name, "of"))
	{
		if (tok_iter == tok.end())
		{
			send_to_char("Какой клич вы хотите использовать?\r\n", ch);
			return;
		}
		else
			wc_name = "warcry of " + *(tok_iter++);
	}
	else
		wc_name = "клич " + wc_name;

	spellnum = fix_name_and_find_spell_num(wc_name);

	// Unknown warcry
	if (spellnum < 1 || spellnum > MAX_SPELLS || (ch->get_skill(SKILL_WARCRY) < SpINFO.mana_change) || !IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
	{
		send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	CHAR_DATA *tch;
	OBJ_DATA *tobj;
	ROOM_DATA *troom;

	int target = find_cast_target(spellnum, tok_iter == tok.end() ? "" : *tok_iter, ch, &tch, &tobj, &troom);

	if (!target)
	{
		send_to_char("Тяжеловато найти цель!\r\n", ch);
		return;
	}

	if (tch == ch && SpINFO.violent)
	{
		send_to_char("Не накликайте беды!\r\n", ch);
		return;
	}

	if (tch && IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !SpINFO.violent)
		return;

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(SpINFO.targets, TAR_SELF_ONLY))
	{
		send_to_char("Этот клич предназначен только для вас!\r\n", ch);
		return;
	}

	if (tch == ch && IS_SET(SpINFO.targets, TAR_NOT_SELF))
	{
		send_to_char("Да вы с ума сошли!\r\n", ch);
		return;
	}

	struct timed_type timed;
	timed.skill = SKILL_WARCRY;
	timed.time = timed_by_skill(ch, SKILL_WARCRY) + HOURS_PER_WARCRY;

	if (timed.time > HOURS_PER_DAY)
	{
		send_to_char("Вы охрипли и не можете кричать.\r\n", ch);
		return;
	}

	if (GET_MOVE(ch) < SpINFO.mana_max)
	{
		send_to_char("У вас не хватит сил для этого.\r\n", ch);
		return;
	}

	sprintf(buf, "Вы выкрикнули %s%s%s.\r\n", SpINFO.violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	say_spell(ch, spellnum, tch, tobj);

	if (call_magic(ch, tch, tobj, troom, spellnum, GET_LEVEL(ch), CAST_SPELL) >= 0)
	{
		if (!WAITLESS(ch))
		{
			if (!CHECK_WAIT(ch))
				WAIT_STATE(ch, PULSE_VIOLENCE);
			timed_to_char(ch, &timed);
			GET_MOVE(ch) -= SpINFO.mana_max;
		}
		train_skill(ch, SKILL_WARCRY, skill_info[SKILL_WARCRY].max_percent, tch);
	}
}

void do_mixture(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	if (IS_NPC(ch))
		return;
	if (IS_IMMORTAL(ch) && !Privilege::check_flag(ch, Privilege::USE_SKILLS))
	{
		send_to_char("Не положено...\r\n", ch);
		return;
	}

	CHAR_DATA *tch;
	OBJ_DATA *tobj;
	ROOM_DATA *troom;
	char *s, *t;
	int spellnum, target = 0;

	// get: blank, spell name, target name
	s = strtok(argument, "'*!");
	if (!s)
	{
		if (subcmd == SCMD_RUNES)
			send_to_char("Что вы хотите сложить?\r\n", ch);
		else if (subcmd == SCMD_ITEMS)
			send_to_char("Что вы хотите смешать?\r\n", ch);
		return;
	}
	s = strtok(NULL, "'*!");
	if (!s)
	{
		send_to_char("Название вызываемой магии смеси должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	t = strtok(NULL, "\0");

	spellnum = fix_name_and_find_spell_num(s);

	// Unknown spell
	if (spellnum < 1 || spellnum > MAX_SPELLS)
	{
		send_to_char("И откуда вы набрались рецептов?\r\n", ch);
		return;
	}

	// Caster does not know this recipe
	if (((!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_ITEMS)
			&& subcmd == SCMD_ITEMS)
			|| (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_RUNES)
				&& subcmd == SCMD_RUNES)) && !IS_GOD(ch))
	{
		send_to_char("Это блюдо вам явно не понравится.\r\n" "Научитесь его правильно готовить.\r\n", ch);
		return;
	}

	if (!check_recipe_values(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, FALSE))
		return;

	if (!check_recipe_items(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, FALSE))
	{
		if (subcmd == SCMD_ITEMS)
			send_to_char("У вас нет нужных ингредиентов!\r\n", ch);
		else if (subcmd == SCMD_RUNES)
			send_to_char("У вас нет нужных рун!\r\n", ch);
		return;
	}

	// Find the target
	if (t != NULL)
		one_argument(t, arg);
	else
		*arg = '\0';

	target = find_cast_target(spellnum, arg, ch, &tch, &tobj, &troom);

	if (target && (tch == ch) && SpINFO.violent)
	{
		send_to_char("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
		return;
	}

	if (!target)
	{
		send_to_char("Тяжеловато найти цель для вашей магии!\r\n", ch);
		return;
	}

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(SpINFO.targets, TAR_SELF_ONLY))
	{
		send_to_char("Вы можете колдовать это только на себя!\r\n", ch);
		return;
	}

	if (IS_MANA_CASTER(ch))
	{
// shapirus: проверку на достаточный левел делаем здесь, потому что
// раньше она делалась в функции расчета стоимости заклинания -- там просто
// возвращалось 9999, если левел мал. Йожику понятно, что это
// полная фигня по всем соображениям :).
// Теперь в этом месте мы будем проверять может ли волхв скастить этот спел

		if (GET_LEVEL(ch) < spell_create_level(ch, spellnum))
		{
			send_to_char("Вы еще слишком малы, чтобы колдовать такое.\r\n", ch);
			return;
		}

		if (GET_MANA_STORED(ch) < GET_MANA_COST(ch, spellnum))
		{
			send_to_char("У вас маловато магической энергии!\r\n", ch);
			return;
		}
		else
		{
			GET_MANA_STORED(ch) = GET_MANA_STORED(ch) - GET_MANA_COST(ch, spellnum);
		}
	}

	// You throws the dice and you takes your chances.. 101% is total failure

	if (check_recipe_items(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, TRUE, tch))
	{
		if (!spell_use_success(ch, tch, SAVING_NONE, spellnum))
		{
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (!tch || !skill_message(0, ch, tch, spellnum))
			{
				if (subcmd == SCMD_ITEMS)
					send_to_char("Вы неправильно смешали ингредиенты!\r\n", ch);
				else if (subcmd == SCMD_RUNES)
					send_to_char("Вы не смогли правильно истолковать значение рун!\r\n", ch);
			}

		}
		else  	// call magic returns 1 on success; set waitstate
		{
			if (call_magic(ch, tch, tobj, world[ch->in_room], spellnum, GET_LEVEL(ch),
						   subcmd == SCMD_ITEMS ? CAST_ITEMS : CAST_RUNES) >= 0)
			{
				if (!(WAITLESS(ch) || CHECK_WAIT(ch)))
					WAIT_STATE(ch, PULSE_VIOLENCE);
			}
		}
	}
}

void do_create(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	char *s;
	int spellnum, itemnum = 0;

	if (IS_NPC(ch))
		return;

	// get: blank, spell name, target name
	argument = one_argument(argument, arg);

	if (!*arg)
	{
		if (subcmd == SCMD_RECIPE)
			send_to_char("Состав ЧЕГО вы хотите узнать?\r\n", ch);
		else
			send_to_char("ЧТО вы хотите создать?\r\n", ch);
		return;
	}

	size_t i = strlen(arg);
	if (!strn_cmp(arg, "potion", i) || !strn_cmp(arg, "напиток", i))
		itemnum = SPELL_POTION;
	else if (!strn_cmp(arg, "wand", i) || !strn_cmp(arg, "палочка", i))
		itemnum = SPELL_WAND;
	else if (!strn_cmp(arg, "scroll", i) || !strn_cmp(arg, "свиток", i))
		itemnum = SPELL_SCROLL;
	else if (!strn_cmp(arg, "recipe", i) || !strn_cmp(arg, "рецепт", i) ||
			 !strn_cmp(arg, "отвар", i))
	{
		if (subcmd != SCMD_RECIPE)
		{
			send_to_char("Магическую смесь необходимо СМЕШАТЬ.\r\n", ch);
			return;
		}
//		itemnum = SPELL_ITEMS;
		compose_recipe(ch, argument, 0);
		return;
	}
	else if (!strn_cmp(arg, "runes", i) || !strn_cmp(arg, "руны", i))
	{
		if (subcmd != SCMD_RECIPE)
		{
			send_to_char("Руны требуется сложить.\r\n", ch);
			return;
		}
		itemnum = SPELL_RUNES;
	}
	else
	{
		if (subcmd == SCMD_RECIPE)
			sprintf(buf, "Состав '%s' уже давно утерян.\r\n", arg);
		else
			sprintf(buf, "Создание '%s' доступно только Великим Богам.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}

	s = strtok(argument, "'*!");
	if (s == NULL)
	{
		sprintf(buf, "Уточните тип состава!\r\n");
		send_to_char(buf, ch);
		return;
	}
	s = strtok(NULL, "'*!");
	if (s == NULL)
	{
		send_to_char("Название состава должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	spellnum = fix_name_and_find_spell_num(s);

	// Unknown spell
	if (spellnum < 1 || spellnum > MAX_SPELLS)
	{
		send_to_char("И откуда вы набрались рецептов?\r\n", ch);
		return;
	}

	// Caster is don't know this recipe
	if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), itemnum) && !IS_IMMORTAL(ch))
	{
		send_to_char("Было бы неплохо прежде всего выучить этот состав.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_RECIPE)
	{
		check_recipe_values(ch, spellnum, itemnum, TRUE);
		return;
	}

	if (!check_recipe_values(ch, spellnum, itemnum, FALSE))
	{
		send_to_char("Боги хранят в тайне этот состав.\r\n", ch);
		return;
	}

	if (!check_recipe_items(ch, spellnum, itemnum, TRUE))
	{
		send_to_char("У вас нет нужных инградиентов!\r\n", ch);
		return;
	}
}

void book_upgrd_fail_message(CHAR_DATA *ch, OBJ_DATA *obj)
{
	send_to_char(ch, "Изучив %s от корки до корки вы так и не узнали ничего нового.\r\n",
		obj->get_PName(3).c_str());
	act("$n с интересом принял$g читать $o3.\r\n"
		"Постепенно $s интерес начал угасать, и $e, плюясь, сунул$g $o3 обратно.",
		FALSE, ch, obj, 0, TO_ROOM);
}

void do_learn(CHAR_DATA *ch, char *argument, int/* cmd*/, int /*subcmd*/)
{
	OBJ_DATA *obj;
	int spellnum = 0, addchance = 10, rcpt = -1;
	im_rskill *rs = NULL;
	const char *spellname = "";

	const char *stype0[] =
	{
		"секрет",
		"еще несколько секретов"
	};

	const char *stype1[] =
	{
		"заклинание",
		"умение",
		"умение",
		"рецепт",
		"рецепт",
		"способность"
	};

	const char *stype2[] =
	{
		"заклинания",
		"умения",
		"умения",
		"рецепта",
		"рецепта",
		"способности"
	};

	if (IS_NPC(ch))
		return;

	// get: blank, spell name, target name
	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Вы принялись внимательно изучать свои ногти. Да, пора бы и подстричь.\r\n", ch);
		act("$n удивленно уставил$u на свои ногти. Подстриг бы их кто-нибудь $m.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		send_to_char("А у вас этого нет.\r\n", ch);
		return;
	}

	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_BOOK)
	{
		act("Вы уставились на $o3, как баран на новые ворота.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n начал$g внимательно изучать устройство $o1.", FALSE, ch, obj, 0, TO_ROOM);
		return;
	}

	if (GET_OBJ_VAL(obj, 0) != BOOK_SPELL && GET_OBJ_VAL(obj, 0) != BOOK_SKILL &&
			GET_OBJ_VAL(obj, 0) != BOOK_UPGRD && GET_OBJ_VAL(obj, 0) != BOOK_RECPT &&
			GET_OBJ_VAL(obj, 0) != BOOK_FEAT)
	{
		act("НЕВЕРНЫЙ ТИП КНИГИ - сообщите Богам!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (GET_OBJ_VAL(obj, 0) == BOOK_SPELL
		&& slot_for_char(ch, 1) <= 0)
	{
		send_to_char("Далась вам эта магия! Пошли-бы, водочки выпили...\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(obj, 2) < 1
		&& GET_OBJ_VAL(obj, 0) != BOOK_UPGRD
		&& GET_OBJ_VAL(obj, 0) != BOOK_SPELL
		&& GET_OBJ_VAL(obj, 0) != BOOK_FEAT
		&& GET_OBJ_VAL(obj, 0) != BOOK_RECPT)
	{
		send_to_char("НЕКОРРЕКТНЫЙ УРОВЕНЬ - сообщите Богам!\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(obj, 0) == BOOK_RECPT)
	{
		rcpt = im_get_recipe(GET_OBJ_VAL(obj, 1));
	}

	if ((GET_OBJ_VAL(obj, 0) == BOOK_SKILL
			|| GET_OBJ_VAL(obj, 0) == BOOK_UPGRD)
		&& GET_OBJ_VAL(obj, 1) < 1
		&& GET_OBJ_VAL(obj, 1) > MAX_SKILL_NUM)
	{
		send_to_char("УМЕНИЕ НЕ ОПРЕДЕЛЕНО - сообщите Богам!\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(obj, 0) == BOOK_RECPT
		&& rcpt < 0)
	{
		send_to_char("РЕЦЕПТ НЕ ОПРЕДЕЛЕН - сообщите Богам!\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(obj, 0) == BOOK_SPELL
		&& (GET_OBJ_VAL(obj, 1) < 1
			|| GET_OBJ_VAL(obj, 1) > SPELLS_COUNT))
	{
		send_to_char("МАГИЯ НЕ ОПРЕДЕЛЕНА - сообщите Богам!\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(obj, 0) == BOOK_FEAT
		&& (GET_OBJ_VAL(obj, 1) < 1
			|| GET_OBJ_VAL(obj, 1) >= MAX_FEATS))
	{
		send_to_char("СПОСОБНОСТЬ НЕ ОПРЕДЕЛЕНА - сообщите Богам!\r\n", ch);
		return;
	}

		//	skill_info[GET_OBJ_VAL(obj, 1)].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] == KNOW_SKILL)
	if (GET_OBJ_VAL(obj, 0) == BOOK_SKILL && can_get_skill_with_req(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2)))
	{
		spellnum = GET_OBJ_VAL(obj, 1);
		spellname = skill_info[spellnum].name;
	}
	else if (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD && ch->get_trained_skill(static_cast<ESkill>(GET_OBJ_VAL(obj, 1))))
	{
		spellnum = GET_OBJ_VAL(obj, 1);
		spellname = skill_info[spellnum].name;
	}
	else if (GET_OBJ_VAL(obj, 0) == BOOK_SPELL && can_get_spell_with_req(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2)))
	{
		spellnum = GET_OBJ_VAL(obj, 1);
		spellname = SpINFO.name;
	}
	else if (GET_OBJ_VAL(obj, 0) == BOOK_RECPT
			&& imrecipes[rcpt].classknow[(int) GET_CLASS(ch)] == KNOW_RECIPE
			&& MAX(GET_OBJ_VAL(obj, 2), imrecipes[rcpt].level) <= GET_LEVEL(ch) && imrecipes[rcpt].remort <= GET_REMORT(ch))
	{
		spellnum = rcpt;
		rs = im_get_char_rskill(ch, spellnum);
		spellname = imrecipes[spellnum].name;
		if (imrecipes[spellnum].level == -1 || imrecipes[spellnum].remort == -1)
		{
			send_to_char("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
			return;
		}
	}
	else if (GET_OBJ_VAL(obj, 0) == BOOK_FEAT && can_get_feat(ch, GET_OBJ_VAL(obj, 1)))
	{
		spellnum = GET_OBJ_VAL(obj, 1);
		spellname = feat_info[spellnum].name;
	}

	if ((GET_OBJ_VAL(obj, 0) == BOOK_SKILL && ch->get_skill(static_cast<ESkill>(spellnum)))
		|| (GET_OBJ_VAL(obj, 0) == BOOK_SPELL && GET_SPELL_TYPE(ch, spellnum) & SPELL_KNOW)
		|| (GET_OBJ_VAL(obj, 0) == BOOK_FEAT && HAVE_FEAT(ch, spellnum))
		|| (GET_OBJ_VAL(obj, 0) == BOOK_RECPT && rs))
	{
		sprintf(buf, "Вы открыли %s и принялись с интересом\r\n"
			"изучать. Каким же было разочарование, когда прочитав %s,\r\n"
			"Вы поняли, что это %s \"%s\".\r\n",
			obj->get_PName(3).c_str(),
			number(0, 1) ? "несколько абзацев" :
			number(0, 1) ? "пару строк" : "почти до конца",
			stype1[GET_OBJ_VAL(obj, 0)],
			spellname);
		send_to_char(buf, ch);
		act("$n с интересом принял$g читать $o3.\r\n"
			"Постепенно $s интерес начал угасать, и $e, плюясь, сунул$g $o3 обратно.",
			FALSE, ch, obj, 0, TO_ROOM);
		return;
	}

	if (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD)
	{
		// апгрейд скилла без учета макс.скилла плеера (до макс в книге)
		if (GET_OBJ_VAL(obj, 3) > 0
			&& ch->get_trained_skill(static_cast<ESkill>(spellnum)) >= GET_OBJ_VAL(obj, 3))
		{
			book_upgrd_fail_message(ch, obj);
			return;
		}

		// апгрейд скилла до макс.скилла плеера (без макса в книге)
		if (GET_OBJ_VAL(obj, 3) <= 0
			&& ch->get_trained_skill(static_cast<ESkill>(spellnum)) >= MAX_EXP_PERCENT + GET_REMORT(ch) * 5)
		{
			book_upgrd_fail_message(ch, obj);
			return;
		}
	}

	if (!spellnum)
	{
		const char* where = number(0, 1) ? "вон та" : (number(0, 1) ? "вот эта" : "пятая справа");
		const char* what = number(0, 1) ? "жука" : (number(0, 1) ? "бабочку" : "русалку");
		const char* whom = obj->get_sex() == ESex::SEX_FEMALE ? "нее" : (obj->get_sex() == ESex::SEX_POLY ? "них" : "него");
		sprintf(buf,
			"- \"Какие интересные буковки ! Особенно %s, похожая на %s\".\r\n"
			"Полюбовавшись еще несколько минут на сию красоту, вы с чувством выполненного\r\n"
			"долга закрыли %s. До %s вы еще не доросли.\r\n",
			where, what, obj->get_PName(3).c_str(), whom);
		send_to_char(buf, ch);
		act("$n с интересом осмотрел$g $o3, крякнул$g от досады и положил$g обратно.", FALSE, ch, obj, 0, TO_ROOM);
		return;
	}

	addchance = (IS_CLERIC(ch) && ROOM_FLAGGED(ch->in_room, ROOM_CLERIC)) ||
				(IS_MAGE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_MAGE)) ||
				(IS_PALADINE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_PALADINE)) ||
				(IS_THIEF(ch) && ROOM_FLAGGED(ch->in_room, ROOM_THIEF)) ||
				(IS_ASSASINE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_ASSASINE)) ||
				(IS_WARRIOR(ch) && ROOM_FLAGGED(ch->in_room, ROOM_WARRIOR)) ||
				(IS_RANGER(ch) && ROOM_FLAGGED(ch->in_room, ROOM_RANGER)) ||
				(IS_GUARD(ch) && ROOM_FLAGGED(ch->in_room, ROOM_GUARD)) ||
				(IS_SMITH(ch) && ROOM_FLAGGED(ch->in_room, ROOM_SMITH)) ||
				(IS_DRUID(ch) && ROOM_FLAGGED(ch->in_room, ROOM_DRUID)) ||
				(IS_MERCHANT(ch) && ROOM_FLAGGED(ch->in_room, ROOM_MERCHANT)) ? 10 : 0;
	addchance += (GET_OBJ_VAL(obj, 0) == BOOK_SPELL) ? 0 : 10;

	if (!obj->get_extra_flag(EExtraFlag::ITEM_NO_FAIL)
		&& number(1, 100) > int_app[POSI(GET_REAL_INT(ch))].spell_aknowlege + addchance)
	{
		sprintf(buf, "Вы взяли в руки %s и начали изучать. Непослушные\r\n"
			"буквы никак не хотели выстраиваться в понятные и доступные фразы.\r\n"
			"Промучившись несколько минут, вы бросили это унылое занятие,\r\n"
			"с удивлением отметив исчезновение %s.\r\n", obj->get_PName(3).c_str(), obj->get_PName(1).c_str());
		send_to_char(buf, ch);
	}
	else
	{
		sprintf(buf, "Вы взяли в руки %s и начали изучать. Постепенно,\r\n"
			"незнакомые доселе, буквы стали складываться в понятные слова и фразы.\r\n"
			"Буквально через несколько минут вы узнали %s %s \"%s\".\r\n",
			obj->get_PName(3).c_str(),
			(GET_OBJ_VAL(obj, 0) == BOOK_UPGRD) ? stype0[1] : stype0[0],
			stype2[GET_OBJ_VAL(obj, 0)],
			spellname);
		send_to_char(buf, ch);
		sprintf(buf, "LEARN: Игрок %s выучил %s %s \"%s\"", GET_NAME(ch), (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD) ? stype0[1] : stype0[0], stype2[GET_OBJ_VAL(obj, 0)], spellname);
		log("%s", buf);
		switch (GET_OBJ_VAL(obj, 0))
		{
		case BOOK_SPELL:
			GET_SPELL_TYPE(ch, spellnum) |= SPELL_KNOW;
			break;

		case BOOK_SKILL:
			ch->set_skill(static_cast<ESkill>(spellnum), 1);
			break;

		case BOOK_UPGRD:
			{
				const ESkill skill = static_cast<ESkill>(spellnum);
				const int left_skill_level = ch->get_trained_skill(static_cast<ESkill>(spellnum)) + GET_OBJ_VAL(obj, 2);
				if (GET_OBJ_VAL(obj, 3) > 0)
				{
					ch->set_skill(skill, MIN(left_skill_level, GET_OBJ_VAL(obj, 3)));
				}
				else
				{
					ch->set_skill(skill, MIN(left_skill_level, MAX_EXP_PERCENT + GET_REMORT(ch) * 5));
				}
			}
			break;

		case BOOK_RECPT:
			CREATE(rs, 1);
			rs->rid = spellnum;
			rs->link = GET_RSKILL(ch);
			GET_RSKILL(ch) = rs;
			rs->perc = 5;
			break;
		case BOOK_FEAT:
			SET_FEAT(ch, spellnum);
			break;
		}
	}
	extract_obj(obj);
}
// -newbook.patch (Alisher)

void show_wizdom(CHAR_DATA * ch, int bitset)
{
	char names[MAX_SLOT][MAX_STRING_LENGTH];
	int slots[MAX_SLOT], i, max_slot, count, slot_num, is_full, gcount = 0, imax_slot = 0;
	for (i = 1; i <= MAX_SLOT; i++)
	{
		*names[i - 1] = '\0';
		slots[i - 1] = 0;
		if (slot_for_char(ch, i))
			imax_slot = i;
	}
	if (bitset & 0x01)
	{
		is_full = 0;
		for (i = 1, max_slot = 0; i <= MAX_SPELLS; i++)
		{
			if (!GET_SPELL_TYPE(ch, i))
				continue;
			if (!spell_info[i].name || *spell_info[i].name == '!')
				continue;
			count = GET_SPELL_MEM(ch, i);
			if (IS_IMMORTAL(ch))
				count = 10;
			if (!count)
				continue;
			slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
			max_slot = MAX(slot_num, max_slot);
			slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
									   "%2s|[%2d] %-31s|",
									   slots[slot_num] % 80 <
									   10 ? "\r\n" : "  ", count, spell_info[i].name);
			is_full++;
		};

		gcount +=
			sprintf(buf2 + gcount, "  %sВы знаете следующие заклинания :%s",
					CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
		if (is_full)
		{
			for (i = 0; i < max_slot + 1; i++)
			{
				if (slots[i])
				{
					gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
					gcount += sprintf(buf2 + gcount, "%s", names[i]);
				}
			}
		}
		else
		{
			gcount += sprintf(buf2 + gcount, "\r\nСейчас у вас нет заученных заклинаний.");
		}
		gcount += sprintf(buf2 + gcount, "\r\n");
	}
	if (bitset & 0x02)
	{
		struct spell_mem_queue_item *q;
		char timestr[16];
		is_full = 0;
		for (i = 0; i < MAX_SLOT; i++)
		{
			*names[i] = '\0';
			slots[i] = 0;
		}

		if (!MEMQUEUE_EMPTY(ch))
		{
			unsigned char cnt[MAX_SPELLS + 1];
			memset(cnt, 0, MAX_SPELLS + 1);
			timestr[0] = 0;
			if (!IS_MANA_CASTER(ch))
			{
				int div, min, sec;
				div = mana_gain(ch);
				if (div > 0)
				{
					sec = MAX(0, 1 + GET_MEM_CURRENT(ch) - GET_MEM_COMPLETED(ch));	// sec/div -- время мема в мин
					sec = sec * 60 / div;	// время мема в сек
					min = sec / 60;
					sec %= 60;
					if (min > 99)
						sprintf(timestr, "&g%5d&n", min);
					else
						sprintf(timestr, "&g%2d:%02d&n", min, sec);
				}
				else
				{
					sprintf(timestr, "&r    -&n");
				}
			}

			for (q = ch->MemQueue.queue; q; q = q->link)
			{
				++cnt[q->spellnum];
			}

			for (q = ch->MemQueue.queue; q; q = q->link)
			{
				i = q->spellnum;
				if (cnt[i] == 0)
					continue;
				slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%2s|[%2d] %-26s%5s|",
										   slots[slot_num] % 80 <
										   10 ? "\r\n" : "  ", cnt[i],
										   spell_info[i].name, q == ch->MemQueue.queue ? timestr : "");
				cnt[i] = 0;
			}

			gcount +=
				sprintf(buf2 + gcount,
						"  %sВы запоминаете следующие заклинания :%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			for (i = 0; i < imax_slot; i++)
			{
				if (slots[i])
				{
					gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
					gcount += sprintf(buf2 + gcount, "%s", names[i]);
				}
			}
		}
		else
			gcount += sprintf(buf2 + gcount, "\r\nВы ничего не запоминаете.");
		gcount += sprintf(buf2 + gcount, "\r\n");
	}

	if ((bitset & 0x04) && imax_slot)
	{
		int *s = MemQ_slots(ch);
		gcount += sprintf(buf2 + gcount, "  %sСвободно :%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
		for (i = 0; i < imax_slot; i++)
		{
			slot_num = MAX(0, slot_for_char(ch, i + 1) - s[i]);
			gcount += sprintf(buf2 + gcount, "%s%2d-%2d%s  ",
							  slot_num ? CCICYN(ch, C_NRM) : "",
							  i + 1, slot_num, slot_num ? CCNRM(ch, C_NRM) : "");
		}
		sprintf(buf2 + gcount, "\r\n");
	}
	//page_string(ch->desc, buf2, 1);
	send_to_char(buf2, ch);
}

void do_remember(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char *s;
	int spellnum;

	// get: blank, spell name, target name
	if (!argument || !(*argument))
	{
		show_wizdom(ch, 0x07);
		return;
	}
	if (IS_IMMORTAL(ch))
	{
		send_to_char("Господи, хоть ты не подкалывай!\r\n", ch);
		return;
	}
	s = strtok(argument, "'*!");
	if (s == NULL)
	{
		send_to_char("Какое заклинание вы хотите заучить?\r\n", ch);
		return;
	}
	s = strtok(NULL, "'*!");
	if (s == NULL)
	{
		send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	spellnum = fix_name_and_find_spell_num(s);

	if (spellnum < 1 || spellnum > MAX_SPELLS)
	{
		send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}
	// Caster is lower than spell level
	if (GET_LEVEL(ch) < MIN_CAST_LEV(SpINFO, ch)
			||  GET_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)
			||    slot_for_char(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0)
	{
		send_to_char("Рано еще вам бросаться такими словами!\r\n", ch);
		return;
	};
	if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
	{
		send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
		return;
	}
	MemQ_remember(ch, spellnum);
	return;
}

inline bool in_mem(char* arg)
{
	return (strlen(arg) != 0) &&
		 (!strn_cmp("часослов", arg, strlen(arg)) ||
		  !strn_cmp("резы", arg, strlen(arg)) || !strn_cmp("book", arg, strlen(arg)));
}

void do_forget(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char *s=0, *t=0;
	int spellnum, is_in_mem;

	// проверка на аргумент рецепт|отвар
	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Что вы хотите забыть?\r\n", ch);
		return;
	}

	size_t i = strlen(arg);
	if (!strn_cmp(arg, "recipe", i) || !strn_cmp(arg, "рецепт", i) ||
			!strn_cmp(arg, "отвар", i))
	{
		forget_recipe(ch, argument, 0);
		return;
	}

	if (!strn_cmp(arg, "все", i) || !strn_cmp(arg, "all", i))
	{
		char arg2[MAX_INPUT_LENGTH];
		two_arguments(argument, arg, arg2);
		if (in_mem(arg2))
		{
			MemQ_flush(ch);
			send_to_char("Вы вычеркнули все заклинания из своего списка для запоминания.\r\n", ch);
		}
		else
		{
			for (i = 1; i <= MAX_SPELLS; i++)
			{
				GET_SPELL_MEM(ch, i) = 0;
			}
			sprintf(buf, "Вы удалили все заклинания из %s.\r\n", GET_RELIGION(ch) == RELIGION_MONO ? "своего часослова" : "своих рез");
			send_to_char(buf, ch);
		}
		return;
	}
	// get: blank, spell name, target name
	if (IS_IMMORTAL(ch))
	{
		send_to_char("Господи, тебе лень набрать skillset?\r\n", ch);
		return;
	}
	s = strtok(argument, "'*!");
	if (s == NULL)
	{
		send_to_char("Какое заклинание вы хотите забыть?\r\n", ch);
		return;
	}
	s = strtok(NULL, "'*!");
	if (s == NULL)
	{
		send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	spellnum = fix_name_and_find_spell_num(s);
	// Unknown spell
	if (spellnum < 1 || spellnum > MAX_SPELLS)
	{
		send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}
	if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
	{
		send_to_char("Трудно забыть то, чего не знаешь...\r\n", ch);
		return;
	}
	t = strtok(NULL, "\0");
	is_in_mem = 0;
	if (t != NULL)
	{
		one_argument(t, arg);
		is_in_mem = in_mem(arg);
	}
	if (!is_in_mem)
		if (!GET_SPELL_MEM(ch, spellnum))
		{
			send_to_char("Прежде чем забыть что-то ненужное, следует заучить что-то ненужное...\r\n", ch);
			return;
		}
		else
		{
			GET_SPELL_MEM(ch, spellnum)--;
			GET_CASTER(ch) -= spell_info[spellnum].danger;
			sprintf(buf, "Вы удалили заклинание '%s%s%s' из %s.\r\n",
					CCICYN(ch, C_NRM),
					SpINFO.name,
					CCNRM(ch, C_NRM), GET_RELIGION(ch) == RELIGION_MONO ? "своего часослова" : "своих рез");
			send_to_char(buf, ch);
		}
	else
		MemQ_forget(ch, spellnum);
}

void mspell_change(char *name, int spell, int kin, int chclass, int class_change)
{
	int bad = 0;

	if (spell < 0 || spell > TOP_SPELL_DEFINE)
	{
		log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
		return;
	}

	if (kin < 0 || kin >= NUM_KIN)
	{
		log("SYSERR: assigning '%s' to illegal kin %d/%d.", skill_name(spell), chclass, NUM_KIN);
		bad = 1;
	}

	if (chclass < 0 || chclass >= NUM_PLAYER_CLASSES)
	{
		log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell), chclass, NUM_PLAYER_CLASSES - 1);
		bad = 1;
	}
	if (!bad)
	{
		spell_info[spell].class_change[chclass][kin] = class_change;
		log("MODIFIER set '%s' kin '%d' classes %d value %d", name, kin, chclass, class_change);

	}
}


void
mspell_remort(char *name, int spell, int kin, int chclass, int remort)
{
	int bad = 0;

	if (spell < 0 || spell > TOP_SPELL_DEFINE)
	{
		log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
		return;
	}
	if (kin < 0 || kin >= NUM_KIN)
	{
		log("SYSERR: assigning '%s' to illegal kin %d/%d.", skill_name(spell), chclass, NUM_KIN);
		bad = 1;
	}
	if (chclass < 0 || chclass >= NUM_PLAYER_CLASSES)
	{
		log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell), chclass, NUM_PLAYER_CLASSES - 1);
		bad = 1;
	}
	if (remort < 0 || remort > MAX_REMORT)
	{
		log("SYSERR: assigning '%s' to illegal remort %d/%d.", skill_name(spell), remort, MAX_REMORT);
		bad = 1;
	}
	if (!bad)
	{
		spell_info[spell].min_remort[chclass][kin] = remort;
		log("REMORT set '%s' kin '%d' classes %d value %d", name, kin, chclass, remort);
	}
}


void mspell_level(char *name, int spell, int kin, int chclass, int level)
{
	int bad = 0;

	if (spell < 0 || spell > TOP_SPELL_DEFINE)
	{
		log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
		return;
	}

	if (kin < 0 || kin >= NUM_KIN)
	{
		log("SYSERR: assigning '%s' to illegal kin %d/%d.", skill_name(spell), chclass, NUM_KIN);
		bad = 1;
	}

	if (chclass < 0 || chclass >= NUM_PLAYER_CLASSES)
	{
		log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell), chclass, NUM_PLAYER_CLASSES - 1);
		bad = 1;
	}

	if (level < 1 || level > LVL_IMPL)
	{
		log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell), level, LVL_IMPL);
		bad = 1;
	}

	if (!bad)
	{
		spell_info[spell].min_level[chclass][kin] = level;
		log("LEVEL set '%s' kin '%d' classes %d value %d", name, kin, chclass, level);
	}
}

void mspell_slot(char *name, int spell, int kin , int chclass, int slot)
{
	int bad = 0;

	if (spell < 0 || spell > TOP_SPELL_DEFINE)
	{
		log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
		return;
	}

	if (kin < 0 || kin >= NUM_KIN)
	{
		log("SYSERR: assigning '%s' to illegal kin %d/%d.", skill_name(spell), chclass, NUM_KIN);
		bad = 1;
	}

	if (chclass < 0 || chclass >=  NUM_PLAYER_CLASSES)
	{
		log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell), chclass, NUM_PLAYER_CLASSES - 1);
		bad = 1;
	}

	if (slot < 1 || slot > MAX_SLOT)
	{
		log("SYSERR: assigning '%s' to illegal slot %d/%d.", skill_name(spell), slot, LVL_IMPL);
		bad = 1;
	}

	if (!bad)
	{
		spell_info[spell].slot_forc[chclass][kin] = slot;
		max_slots.init(chclass, kin, slot);
		log("SLOT set '%s' kin '%d' classes %d value %d", name, kin, chclass, slot);
	}

}


// Assign the spells on boot up
void
spello(int spl, const char *name, const char *syn,
	   int max_mana, int min_mana, int mana_change,
	   int minpos, int targets, int violent, int routines, int danger, int spell_class)
{
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
		for (j = 0; j < NUM_KIN; j++)
		{
			spell_info[spl].min_remort[i][j] = MAX_REMORT;
			spell_info[spl].min_level[i][j] = LVL_IMPL;
			spell_info[spl].slot_forc[i][j] = MAX_SLOT;
			spell_info[spl].class_change[i][j] = 0;
		}

	spell_create[spl].wand.min_caster_level = 34;
	spell_create[spl].scroll.min_caster_level = 34;
	spell_create[spl].potion.min_caster_level = 34;
	spell_create[spl].items.min_caster_level = 34;
	spell_create[spl].runes.min_caster_level = 34;

	spell_info[spl].mana_max = max_mana;
	spell_info[spl].mana_min = min_mana;
	spell_info[spl].mana_change = mana_change;
	spell_info[spl].min_position = minpos;
	spell_info[spl].danger = danger;
	spell_info[spl].targets = targets;
	spell_info[spl].violent = violent;
	spell_info[spl].routines = routines;
	spell_info[spl].name = name;
	spell_info[spl].syn = syn;
	spell_info[spl].spell_class = spell_class;
}

void unused_spell(int spl)
{
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
		for (j = 0; j < NUM_KIN; j++)
		{
			spell_info[spl].min_remort[i][j] = MAX_REMORT;
			spell_info[spl].min_level[i][j] = LVL_IMPL + 1;
			spell_info[spl].slot_forc[i][j] = MAX_SLOT;
			spell_info[spl].class_change[i][j] = 0;
		}

	for (i = 0; i < 3; i++)
	{
		spell_create[spl].wand.items[i] = -1;
		spell_create[spl].scroll.items[i] = -1;
		spell_create[spl].potion.items[i] = -1;
		spell_create[spl].items.items[i] = -1;
		spell_create[spl].runes.items[i] = -1;
	}

	spell_create[spl].wand.rnumber = -1;
	spell_create[spl].scroll.rnumber = -1;
	spell_create[spl].potion.rnumber = -1;
	spell_create[spl].items.rnumber = -1;
	spell_create[spl].runes.rnumber = -1;

	spell_info[spl].mana_max = 0;
	spell_info[spl].mana_min = 0;
	spell_info[spl].mana_change = 0;
	spell_info[spl].min_position = 0;
	spell_info[spl].danger = 0;
	spell_info[spl].targets = 0;
	spell_info[spl].violent = 0;
	spell_info[spl].routines = 0;
	spell_info[spl].name = unused_spellname;
	spell_info[spl].syn = unused_spellname;
}

void skillo(int spl, const char *name, int max_percent)
{
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
		for (j = 0; j < NUM_KIN; j++)
		{
			skill_info[spl].min_remort[i][j] = MAX_REMORT;
			skill_info[spl].min_level[i][j] = 0 ;
			skill_info[spl].k_improove[i][j] = 0;
		}
	skill_info[spl].min_position = 0;
	skill_info[spl].name = name;
	skill_info[spl].max_percent = max_percent;
}

void unused_skill(int spl)
{
	int i, j;

	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
		for (j = 0; j < NUM_KIN; j++)
		{
			skill_info[spl].min_remort[i][j] = MAX_REMORT;
			skill_info[spl].min_level[i][j] = 0;
			skill_info[spl].k_improove[i][j] = 0 ;
		}
	skill_info[spl].min_position = 0;
	skill_info[spl].max_percent = 200;
	skill_info[spl].name = unused_spellname;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * spellname: The name of the spell.
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * remort:  minimum number of remorts to use the spell.
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
	int i;

	// Do not change the loop below.
	for (i = 0; i <= TOP_SPELL_DEFINE; i++)
	{
		unused_spell(i);
	}
	for (i = 0; i <= MAX_SKILL_NUM; i++)
	{
		unused_skill(i);
	}


	// Do not change the loop above.

//1
	spello(SPELL_ARMOR, "защита", "armor", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//2
	spello(SPELL_TELEPORT, "прыжок", "teleport",
		   140, 120, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL | NPC_DAMAGE_PC, 1, STYPE_AIR);
//3
	spello(SPELL_BLESS, "доблесть", "bless", 55, 40, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//4
	spello(SPELL_BLINDNESS, "слепота", "blind",
		   70, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_DARK);
//5
	spello(SPELL_BURNING_HANDS, "горящие руки", "burning hands", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_FIRE);
//6
	spello(SPELL_CALL_LIGHTNING, "шаровая молния", "call lightning", 85, 70, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, NPC_AFFECT_PC | MAG_AFFECTS | MAG_DAMAGE | NPC_DAMAGE_PC, 2, STYPE_AIR);
//7
	spello(SPELL_CHARM, "подчинить разум", "mind control", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1, STYPE_MIND);
//8
	spello(SPELL_CHILL_TOUCH, "ледяное прикосновение", "chill touch",
		   55, 45, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 1, STYPE_WATER);
//9
	spello(SPELL_CLONE, "клонирование", "clone",
		   150, 130, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS, 0, STYPE_DARK);
//10
	spello(SPELL_COLOR_SPRAY, "ледяные стрелы", "ice bolts", 90, 75, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   3, STYPE_WATER);
//11
	spello(SPELL_CONTROL_WEATHER, "контроль погоды", "weather control",
		   100, 90, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 0, STYPE_AIR);
//12
	spello(SPELL_CREATE_FOOD, "создать пищу", "create food",
		   40, 30, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS, 0, STYPE_LIFE);
//13
	spello(SPELL_CREATE_WATER, "создать воду", "create water", 40, 30, 1,
		   POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM, FALSE, MAG_MANUAL, 0, STYPE_WATER);
//14
	spello(SPELL_CURE_BLIND, "вылечить слепоту", "cure blind", 110, 90, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0, STYPE_LIGHT);
//15
	spello(SPELL_CURE_CRITIC, "критическое исцеление", "critical cure",
		   100, 90, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 3, STYPE_LIFE);
//16
	spello(SPELL_CURE_LIGHT, "легкое исцеление", "light cure",
		   40, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 1, STYPE_LIFE);
//17
	spello(SPELL_CURSE, "проклятье", "curse", 55, 40, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV, MTYPE_NEUTRAL,
		MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 1, STYPE_DARK);
//18
	spello(SPELL_DETECT_ALIGN, "определение наклонностей", "detect alignment",
		   40, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//19
	spello(SPELL_DETECT_INVIS, "видеть невидимое", "detect invisible",
		   100, 55, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//20
	spello(SPELL_DETECT_MAGIC, "определение магии", "detect magic",
		   100, 55, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//21
	spello(SPELL_DETECT_POISON, "определение яда", "detect poison",
		   40, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//22
	spello(SPELL_DISPEL_EVIL, "изгнать зло", "dispel evil",
		   100, 90, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE, 1, STYPE_LIGHT);
//23
	spello(SPELL_EARTHQUAKE, "землетрясение", "earthquake", 110, 90, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC, 2, STYPE_EARTH);
//24
	spello(SPELL_ENCHANT_WEAPON, "заколдовать оружие", "enchant weapon",
		   140, 110, 2, POS_STANDING, TAR_OBJ_INV, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//25
	spello(SPELL_ENERGY_DRAIN, "истощить энергию", "energy drain",
		   150, 140, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_MANUAL | MAG_AFFECTS | NPC_DAMAGE_PC, 1, STYPE_DARK);
//26
	spello(SPELL_FIREBALL, "огненный шар", "fireball",
		   110, 100, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   2, STYPE_FIRE);
//27
	spello(SPELL_HARM, "вред", "harm",
		   110, 100, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 5, STYPE_DARK);
//28
	spello(SPELL_HEAL, "исцеление", "heal", 110, 100, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 10, STYPE_LIFE);
//29
	spello(SPELL_INVISIBLE, "невидимость", "invisible",
		   50, 40, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
		   FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0,  STYPE_MIND);
//30
	spello(SPELL_LIGHTNING_BOLT, "молния", "lightning bolt", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_AIR);
//31
	spello(SPELL_LOCATE_OBJECT, "разыскать предмет", "locate object",
		   140, 110, 2, POS_STANDING, TAR_OBJ_WORLD, FALSE, MAG_MANUAL, 0, STYPE_MIND);
//32
	spello(SPELL_MAGIC_MISSILE, "магическая стрела", "magic missle",
		   40, 30, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_FIRE);
//33
	spello(SPELL_POISON, "яд", "poison", 70, 55, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT,
		MTYPE_NEUTRAL, MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 2, STYPE_LIFE);
//34
	spello(SPELL_PROT_FROM_EVIL, "защита от тьмы", "protect evil", 60, 45, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//35
	spello(SPELL_REMOVE_CURSE, "снять проклятье", "remove curse",
		   50, 40, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
		   MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0, STYPE_LIGHT);
//36
	spello(SPELL_SANCTUARY, "освящение", "sanctuary", 85, 70, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//37
	spello(SPELL_SHOCKING_GRASP, "обжигающая хватка", "shocking grasp", 50, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_FIRE);
//38
	spello(SPELL_SLEEP, "сон", "sleep",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//39
	spello(SPELL_STRENGTH, "сила", "strength", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//40
	spello(SPELL_SUMMON, "призвать", "summon",
		   110, 100, 2, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, 0, STYPE_MIND);
//41
	spello(SPELL_PATRONAGE, "покровительство", "patronage", 85, 70, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS, 1, STYPE_LIGHT);
//42
	spello(SPELL_WORD_OF_RECALL, "слово возврата", "recall", 140, 100, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_MANUAL | NPC_DAMAGE_PC, 0, STYPE_MIND);
//43
	spello(SPELL_REMOVE_POISON, "удалить яд", "remove poison",
		60, 45, 2, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE,
		MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0, STYPE_LIFE);
//44
	spello(SPELL_SENSE_LIFE, "определение жизни", "sense life",
		   85, 70, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_LIFE);
//45
	spello(SPELL_ANIMATE_DEAD, "поднять труп", "animate dead",
		   50, 35, 3, POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0, STYPE_DARK);
//46
	spello(SPELL_DISPEL_GOOD, "рассеять свет", "dispel good", 100, 90, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE, 1, STYPE_DARK);
//47
	spello(SPELL_GROUP_ARMOR, "групповая защита", "group armor", 110, 100, 2,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//48
	spello(SPELL_GROUP_HEAL, "групповое исцеление", "group heal",
		   110, 100, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 30, STYPE_LIFE);
//49
	spello(SPELL_GROUP_RECALL, "групповой возврат", "group recall",
		   125, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_MANUAL, 0, STYPE_MIND);
//50
	spello(SPELL_INFRAVISION, "видение ночью", "infravision",
		   50, 40, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_LIGHT);
//51
	spello(SPELL_WATERWALK, "водохождение", "waterwalk",
		   70, 55, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_WATER);
//52
	spello(SPELL_CURE_SERIOUS, "серьезное исцеление", "serious cure", 85, 70, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 2, STYPE_LIFE);
//53
	spello(SPELL_GROUP_STRENGTH, "групповая сила", "group strength", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//54
	spello(SPELL_HOLD, "оцепенение", "hold",
		   100, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 3, STYPE_MIND);
//55
	spello(SPELL_POWER_HOLD, "длительное оцепенение", "power hold",
		   140, 90, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 4, STYPE_MIND);
//56
	spello(SPELL_MASS_HOLD, "массовое оцепенение", "mass hold",
		   150, 130, 5, POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC,
		   5, STYPE_MIND);
//57
	spello(SPELL_FLY, "полет", "fly", 50, 35, 1, POS_STANDING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0, STYPE_AIR);
//58
	spello(SPELL_BROKEN_CHAINS, "разбитые оковы", "broken chains", 125, 110, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 5, STYPE_MIND);

//59
	spello(SPELL_NOFLEE, "приковать противника", "noflee",
		   100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//60
	spello(SPELL_CREATE_LIGHT, "создать свет", "create light",
		   40, 30, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS, 0, STYPE_LIGHT);
//61
	spello(SPELL_DARKNESS, "тьма", "darkness", 100, 70, 2, POS_STANDING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0, STYPE_DARK);
//62
	spello(SPELL_STONESKIN, "каменная кожа", "stoneskin", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);
//63
	spello(SPELL_CLOUDLY, "затуманивание", "cloudly", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_WATER);
//64
	spello(SPELL_SILENCE, "молчание", "sielence",
		   100, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 1, STYPE_MIND);
//65
	spello(SPELL_LIGHT, "свет", "sun shine", 100, 70, 2, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//66
	spello(SPELL_CHAIN_LIGHTNING, "цепь молний", "chain lightning",
		   120, 110, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_AIR);
//67
	spello(SPELL_FIREBLAST, "огненный поток", "fireblast",
		   110, 90, 2, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC,
		   5, STYPE_FIRE);
//68
	spello(SPELL_IMPLOSION, "гнев богов", "implosion",
		   140, 120, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   15, STYPE_FIRE);
//69
	spello(SPELL_WEAKNESS, "слабость", "weakness",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_LIFE);
//70
	spello(SPELL_GROUP_INVISIBLE, "групповая невидимость", "group invisible",
		   150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS, 0, STYPE_MIND);
//71
	spello(SPELL_SHADOW_CLOAK, "мантия теней", "shadow cloak", 100, 70, 3,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_DARK);
//72
	spello(SPELL_ACID, "кислота", "acid",
		   90, 65, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | MAG_ALTER_OBJS | NPC_DAMAGE_PC, 2, STYPE_WATER);
//73
	spello(SPELL_REPAIR, "починка", "repair",
		   110, 100, 1, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//74
	spello(SPELL_ENLARGE, "увеличение", "enlarge",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC,
		   0, STYPE_LIFE);
//75
	spello(SPELL_FEAR, "страх", "fear",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_MANUAL | NPC_DAMAGE_PC, 1, STYPE_DARK);
//76
	spello(SPELL_SACRIFICE, "высосать жизнь", "sacrifice",
		   140, 125, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_MANUAL | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   10, STYPE_DARK);
//77
	spello(SPELL_WEB, "сеть", "web",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//78
	spello(SPELL_BLINK, "мигание", "blink", 70, 55, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//79
	spello(SPELL_REMOVE_HOLD, "снять оцепенение", "remove hold", 110, 90, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 1, STYPE_LIGHT);
//80
	spello(SPELL_CAMOUFLAGE, "!маскировка!", "!set by skill!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//81
	spello(SPELL_POWER_BLINDNESS, "полная слепота", "power blind",
		   110, 100, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_DARK);
//82
	spello(SPELL_MASS_BLINDNESS, "массовая слепота", "mass blind", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 4, STYPE_DARK);
//83
	spello(SPELL_POWER_SILENCE, "длительное молчание", "power sielence",
		   120, 90, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 2, STYPE_MIND);
//84
	spello(SPELL_EXTRA_HITS, "увеличить жизнь", "extra hits",
		   100, 85, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 1, STYPE_LIFE);
//85
	spello(SPELL_RESSURECTION, "оживить труп", "ressurection",
		   120, 100, 2, POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0, STYPE_DARK);
//86
	spello(SPELL_MAGICSHIELD, "волшебный щит", "magic shield", 50, 30, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);

//87
	spello(SPELL_FORBIDDEN, "запечатать комнату", "forbidden",
		125, 110, 2, POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_ROOM, 0, STYPE_MIND);
//88
	spello(SPELL_MASS_SILENCE, "массовое молчание", "mass sielence", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 3, STYPE_MIND);
//89
	spello(SPELL_REMOVE_SILENCE, "снять молчание", "remove sielence",
		   70, 55, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER,
		   1, STYPE_LIGHT);
//90
	spello(SPELL_DAMAGE_LIGHT, "легкий вред", "light damage",
		   40, 30, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_DARK);
//91
	spello(SPELL_DAMAGE_SERIOUS, "серьезный вред", "serious damage",
		   85, 55, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 2, STYPE_DARK);
//92
	spello(SPELL_DAMAGE_CRITIC, "критический вред", "critical damage",
		   100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 3, STYPE_DARK);
//93
	spello(SPELL_MASS_CURSE, "массовое проклятье", "mass curse", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_DARK);
//94
	spello(SPELL_ARMAGEDDON, "суд богов", "armageddon", 150, 130, 5,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC, 10, STYPE_AIR);
//95
	spello(SPELL_GROUP_FLY, "групповой полет", "group fly",
		   140, 120, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS, 0, STYPE_AIR);
//96
	spello(SPELL_GROUP_BLESS, "групповая доблесть", "group bless", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//97
	spello(SPELL_REFRESH, "восстановление", "refresh",
		   80, 60, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_POINTS, 0,  STYPE_LIFE);
//98
	spello(SPELL_STUNNING, "каменное проклятье", "stunning", 150, 140, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE, 15, STYPE_EARTH);

//99
	spello(SPELL_HIDE, "!спрятался!", "!set by skill!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);

//100
	spello(SPELL_SNEAK, "!крадется!", "!set by skill!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);

//101
	spello(SPELL_DRUNKED, "!опьянение!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//102
	spello(SPELL_ABSTINENT, "!абстиненция!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);

//103
	spello(SPELL_FULL, "насыщение", "full", 70, 55, 1,
		   POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_POINTS, 10,  STYPE_LIFE);
//104
	spello(SPELL_CONE_OF_COLD, "ледяной ветер", "cold wind",
		   100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AFFECTS | MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   15, STYPE_WATER);
//105
	spello(SPELL_BATTLE, "!получил в бою!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//106
	spello(SPELL_HAEMORRAGIA, "!кровотечение!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//107
	spello(SPELL_COURAGE, "!ярость!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//108
	spello(SPELL_WATERBREATH, "дышать водой", "waterbreath",
		   85, 70, 4, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_WATER);
//109
	spello(SPELL_SLOW, "медлительность", "slow",
		   55, 40, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//110
	spello(SPELL_HASTE, "ускорение", "haste", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//111
	spello(SPELL_MASS_SLOW, "массовая медлительность", "mass slow", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_MIND);
//112
	spello(SPELL_GROUP_HASTE, "групповое ускорение", "group haste", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//113
	spello(SPELL_SHIELD, "защита богов", "gods shield", 150, 140, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 2, STYPE_LIGHT);
//114
	spello(SPELL_PLAQUE, "лихорадка", "plaque",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_LIFE);
//115
	spello(SPELL_CURE_PLAQUE, "вылечить лихорадку", "cure plaque", 85, 70, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0, STYPE_LIFE);
//116
	spello(SPELL_AWARNESS, "внимательность", "awarness", 100, 90, 1,
		   POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//117
	spello(SPELL_RELIGION, "!молитва или жертва!", "!pray or donate!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);
//118
	spello(SPELL_AIR_SHIELD, "воздушный щит", "air shield", 140, 120, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_AIR);
//119
	spello(SPELL_PORTAL, "переход", "portal", 200, 180, 4,
		   POS_STANDING, TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0, STYPE_LIGHT);
//120
	spello(SPELL_DISPELL_MAGIC, "развеять магию", "dispel magic",
		   85, 70, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS, 0, STYPE_LIGHT);
//121
	spello(SPELL_SUMMON_KEEPER, "защитник", "keeper",
		   100, 80, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, 0, STYPE_LIGHT);
//122
	spello(SPELL_FAST_REGENERATION, "быстрое восстановление",
		   "fast regeneration", 100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//123
	spello(SPELL_CREATE_WEAPON, "создать оружие", "create weapon",
		   130, 110, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 0, STYPE_LIGHT);
//124
	spello(SPELL_FIRE_SHIELD, "огненный щит", "fire shield", 140, 120, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_FIRE);
//125
	spello(SPELL_RELOCATE, "переместиться", "relocate",
		   140, 120, 2, POS_STANDING, TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0, STYPE_AIR);
//126
	spello(SPELL_SUMMON_FIREKEEPER, "огненный защитник", "fire keeper",
		   150, 140, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, 0, STYPE_FIRE);
//127
	spello(SPELL_ICE_SHIELD, "ледяной щит", "ice protect", 140, 120, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_WATER);
//128
	spello(SPELL_ICESTORM, "ледяной шторм", "ice storm",
		   125, 110, 2, POS_FIGHTING,
		   TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 5, STYPE_WATER);
//129
	spello(SPELL_ENLESS, "уменьшение", "enless",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS, 0, STYPE_LIFE);
//130
	spello(SPELL_SHINEFLASH, "яркий блик", "shine flash",
		   60, 45, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_FIRE);
//131
	spello(SPELL_MADNESS, "безумие", "madness",
		   130, 110, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//132
	spello(SPELL_GROUP_MAGICGLASS, "магическое зеркало", "group magicglass",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 4, STYPE_AIR);
//133
	spello(SPELL_CLOUD_OF_ARROWS, "облако стрел", "cloud of arrous", 95, 80, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 4, STYPE_FIRE);
//134
	spello(SPELL_VACUUM, "круг пустоты", "vacuum sphere",
		   150, 140, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 15, STYPE_DARK);
//135
//	spello(SPELL_METEORSTORM, "метеоритный дождь", "meteor storm", 125, 110, 2,
//		   POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC, 5, STYPE_EARTH);
	spello(SPELL_METEORSTORM, "метеоритный дождь", "meteor storm", 125, 110, 2,
		   POS_FIGHTING, TAR_ROOM_THIS, FALSE, MAG_NEED_CONTROL | MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_EARTH);
//136
	spello(SPELL_STONEHAND, "каменные руки", "stonehand", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);
//137
	spello(SPELL_MINDLESS, "повреждение разума", "mindness",
		   120, 110, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER,
		   0, STYPE_MIND);
//138
	spello(SPELL_PRISMATICAURA, "призматическая аура", "prismatic aura", 85, 70, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//139
	spello(SPELL_EVILESS, "силы зла", "eviless", 150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,
		   3, STYPE_DARK);
//140
	spello(SPELL_AIR_AURA, "воздушная аура", "air aura",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_AIR);
//141
	spello(SPELL_FIRE_AURA, "огненная аура", "fire aura",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_FIRE);
//142
	spello(SPELL_ICE_AURA, "ледяная аура", "ice aura",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_WATER);
//143
	spello(SPELL_SHOCK, "шок", "shock", 100, 90, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_DARK);
//144
	spello(SPELL_MAGICGLASS, "зеркало магии", "magic glassie", 120, 110, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 2, STYPE_LIGHT);

//145
	spello(SPELL_GROUP_SANCTUARY, "групповое освящение", "group sanctuary", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);

//146
	spello(SPELL_GROUP_PRISMATICAURA, "групповая призматическая аура",
		   "group prismatic aura", 110, 100, 1, POS_FIGHTING, TAR_IGNORE,
		   FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);

//147
	spello(SPELL_DEAFNESS, "глухота", "deafness",
		   100, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 1, STYPE_MIND);

//148
	spello(SPELL_POWER_DEAFNESS, "длительная глухота", "power deafness",
		   120, 90, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 2, STYPE_MIND);

//149
	spello(SPELL_REMOVE_DEAFNESS, "снять глухоту", "remove deafness",
		   90, 80, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER,
		   1, STYPE_LIFE);

//150
	spello(SPELL_MASS_DEAFNESS, "массовая глухота", "mass deafness", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_MIND);

//151
	spello(SPELL_DUSTSTORM, "пылевая буря", "dust storm",
		   125, 110, 2, POS_FIGHTING,
		   TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 5, STYPE_EARTH);

//152
	spello(SPELL_EARTHFALL, "камнепад", "earth fall",
		   120, 110, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_EARTH);

//153
	spello(SPELL_SONICWAVE, "звуковая волна", "sonic wave",
		    120, 110, 2, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE,
		    MAG_MASSES | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_AIR);

//154
	spello(SPELL_HOLYSTRIKE, "силы света", "holystrike",
		   150, 130, 5, POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MANUAL | NPC_DAMAGE_PC, 10, STYPE_LIGHT);

//155
	spello(SPELL_ANGEL, "ангел-хранитель", "angel", 150, 130, 5,
		   POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 1, STYPE_LIGHT);
//156
	spello(SPELL_MASS_FEAR, "массовый страх", "mass fear", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_MANUAL | NPC_AFFECT_PC, 4, STYPE_DARK);
//157
	spello(SPELL_FASCINATION, "обаяние", "fascination", 480, 440, 20,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 2, STYPE_MIND);
//158
	spello(SPELL_CRYING, "плач", "crying",
		   120, 55, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//159
	spello(SPELL_OBLIVION, "забвение", "oblivion",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_DARK);
//160
	spello(SPELL_BURDEN_OF_TIME, "бремя времени", "burden time", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 4, STYPE_DARK);
//161
	spello(SPELL_GROUP_REFRESH, "групповое восстановление", "group refresh",	//Added by Adept
		   160, 140, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 30, STYPE_LIFE);

//162
	spello(SPELL_PEACEFUL, "смирение", "peaceful",
		   55, 40, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//163
	spello(SPELL_MAGICBATTLE, "!получил в бою!", "!set by programm!", 0, 0, 0, 255, 0,
		   FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//164
	spello(SPELL_BERSERK, "!предсмертная ярость!", "!set by programm!", 0, 0, 0, 255, 0,
		   FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//165
	spello(SPELL_STONEBONES, "каменные кости", "stone bones", 80, 40, 1,
		   POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);

//166 - SPELL_ROOM_LIGHT
	spello(SPELL_ROOM_LIGHT, "осветить комнату", "room light", 10, 10, 1,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM , 0, STYPE_LIGHT);

//167 - SPELL_POISONED_FOG
	spello(SPELL_POISONED_FOG, "ядовитый туман", "poisoned fog", 10, 10, 1,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_LIFE);

//168 - SPELL_THUNDERSTORM
	spello(SPELL_THUNDERSTORM, "буря отмщения", "storm of vengeance", 10, 10, 1,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_NEED_CONTROL | MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_AIR);
//169
	spello(SPELL_LIGHT_WALK, "!легкая поступь!", "!set by programm!", 0, 0, 0, 255, 0,
		   FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//170
	spello(SPELL_FAILURE, "недоля", "failure", 100, 85, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 5, STYPE_DARK);

//171
	spello(SPELL_CLANPRAY, "!клановые чары!", "!clan affect!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);
//172
	spello(SPELL_GLITTERDUST, "блестящая пыль", "glitterdust", 120, 100, 3,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 5, STYPE_EARTH);
//173
	spello(SPELL_SCREAM, "вопль", "scream", 100, 85, 3, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_AIR);
//174
	spello(SPELL_CATS_GRACE, "кошачья ловкость", "cats grace", 50, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//175
	spello(SPELL_BULL_BODY, "бычье тело", "bull body", 50, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//176
	spello(SPELL_SNAKE_WISDOM, "мудрость змеи", "snake wisdom", 60, 50, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//177
	spello(SPELL_GIMMICKRY, "хитроумие", "gimmickry", 60, 50, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
// ДЛЯ КЛИЧЕЙ ПОЛЕ mana_change ИСПОЛЬЗУЕТСЯ
// ДЛЯ УКАЗАНИЯ МИНИМАЛЬНОГО ПРОЦЕНТА СКИЛЛА,
// С КОТОРОГО ДОСТУПЕН УКАЗАННЫЙ КЛИЧ
//178
/*
	spello(SPELL_WC_OF_CHALLENGE, "клич вызова", "warcry of challenge", 10, 10, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC, 0, STYPE_MIND);
//179
	spello(SPELL_WC_OF_MENACE, "клич угрозы", "warcry of menace", 30, 30, 40,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//180
	spello(SPELL_WC_OF_RAGE, "клич ярости", "warcry of rage", 30, 30, 50,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 0, STYPE_MIND);
//181
	spello(SPELL_WC_OF_MADNESS, "клич безумия", "warcry of madness", 50, 50, 91,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//182
	spello(SPELL_WC_OF_THUNDER, "клич грома", "warcry of thunder", 140, 140, 141,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 0, STYPE_MIND);
*/
//183
	spello(SPELL_WC_OF_DEFENSE, "клич обороны", "warcry of defense", 10, 10, 10,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//184
	spello(SPELL_WC_OF_BATTLE, "клич битвы", "warcry of battle", 20, 20, 50,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//185
	spello(SPELL_WC_OF_POWER, "клич мощи", "warcry of power", 25, 25, 70,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_POINTS | MAG_AFFECTS | NPC_DUMMY | NPC_AFFECT_NPC, 0, STYPE_MIND);
//186
	spello(SPELL_WC_OF_BLESS, "клич доблести", "warcry of bless", 15, 15, 30,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//187
	spello(SPELL_WC_OF_COURAGE, "клич отваги", "warcry of courage", 10, 10, 10,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//188
	spello(SPELL_RUNE_LABEL, "рунная метка", "rune label", 50, 35, 1,
		POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM | MAG_CASTER_INWORLD_DELAY, 0, STYPE_LIGHT);


	// NON-castable spells should appear below here.

// 189
	spello(SPELL_ACONITUM_POISON, "яд аконита", "aconitum poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);
// 190
	spello(SPELL_SCOPOLIA_POISON, "яд скополии", "scopolia poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);
// 191
	spello(SPELL_BELENA_POISON, "яд белены", "belena poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);
// 192
	spello(SPELL_DATURA_POISON, "яд дурмана", "datura poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);

	spello(SPELL_IDENTIFY, "идентификация", "identify",
		   0, 0, 0, 0, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL, 0, STYPE_MIND);
// 193
	spello(SPELL_TIMER_REPAIR, "обновление таймера", " timer repair",
		   110, 100, 1, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);

//194
	spello(SPELL_LACKY, "боевое везение", "lacky", 100, 90, 1,
		POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//195
	spello(SPELL_BANDAGE, "перевязка", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//196
	spello(SPELL_NO_BANDAGE, "!нельзя перевязываться!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//197
	spello(SPELL_CAPABLE, "!зачарован!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//198
	spello(SPELL_STRANGLE, "!удушье!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//199
	spello(SPELL_RECALL_SPELLS, "!вспоминает заклинания!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//200
	spello(SPELL_HYPNOTIC_PATTERN, "чарующий узор", "hypnotic pattern", 120, 100, 2,
			POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_MIND);
//202
	spello(SPELL_VAMPIRE, "вампиризм", "vampire", 150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,
		   3, STYPE_DARK);
//203
	spello(SPELLS_RESTORATION, "реконструкция", "reconstruction",
		   110, 100, 1, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//204
	spello(SPELL_AURA_DEATH, "аура смерти", "aura death",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_DARK);
//205
	spello(SPELL_RECOVERY, "темное прикосновение", "recovery",
		   110, 100, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_DARK);
//206
	spello(SPELL_MASS_RECOVERY, "прикосновение смерти", "mass recovery",
		   110, 100, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 0, STYPE_DARK);
//207
	spello(SPELL_AURA_EVIL, "аура зла", "aura evil",
		   150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,  3, STYPE_DARK);
//208
	spello(SPELL_MENTAL_SHADOW, "ментальная тень", "mental shadow", 150, 130, 5,
		   POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 1, STYPE_MIND);
//209
	spello(SPELL_EVARDS_BLACK_TENTACLES, "навьи руки", "evards black tentacles", 120, 110, 2,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_NEED_CONTROL | MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_DARK);
//210
	spello(SPELL_WHIRLWIND, "вихрь", "whirlwind", 110, 100, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_AIR);
//211
	spello(SPELL_INDRIKS_TEETH, "зубы индрика", "indriks teeth", 60, 45, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//212
	spello(SPELL_MELFS_ACID_ARROW, "кислотная стрела", "acid arrow", 110, 100, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_WATER);
//213
	spello(SPELL_THUNDERSTONE, "громовой камень", "thunderstone", 110, 100, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//214
	spello(SPELL_CLOD, "глыба", "clod", 110, 100, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//215
	spello(SPELL_EXPEDIENT, "!боевой прием!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//216
	spello(SPELL_SIGHT_OF_DARKNESS, "зрение тьмы", "sight darkness", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//217
	spello(SPELL_GENERAL_SINCERITY, "общая искренность", "general sincerity", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//218
	spello(SPELL_MAGICAL_GAZE, "магический взор", "magical gaze", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//219
	spello(SPELL_ALL_SEEING_EYE, "всевидящее око", "allseeing eye", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//220
	spello(SPELL_EYE_OF_GODS, "око богов", "eye gods", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIFE);
//221
	spello(SPELL_BREATHING_AT_DEPTH, "дыхание глубин", "breathing at depth", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_WATER);
//222
	spello(SPELL_GENERAL_RECOVERY, "общее восстановление", "general recovery", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIFE);
//223
	spello(SPELL_COMMON_MEAL, "общая трапеза", "common meal", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 1, STYPE_LIFE);
//224
	spello(SPELL_STONE_WALL, "каменная стена", "stone wall", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_EARTH);
//225
	spello(SPELL_SNAKE_EYES, "глаза змея", "snake eyes", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//226
	spello(SPELL_EARTH_AURA, "земной поклон", "earth aura", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);
//227
	spello(SPELL_GROUP_PROT_FROM_EVIL, "групповая защита от тьмы", "group protect evil", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);

//228
	spello(SPELL_ARROWS_FIRE, "стрелы огня", "arrows of fire", 0, 0, 0,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 2, STYPE_FIRE);
//229
	spello(SPELL_ARROWS_WATER, "стрелы воды", "arrows of water", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_WATER);
//230
	spello(SPELL_ARROWS_EARTH, "стрелы земли", "arrows of earth", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//231
	spello(SPELL_ARROWS_AIR, "стрелы воздуха", "arrows of air", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_AIR);
//232
	spello(SPELL_ARROWS_DEATH, "стрелы смерти", "arrows of death", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_DARK);
//233
	spello(SPELL_PALADINE_INSPIRATION, "воодушевление", "inspiration",
		   0, 0, 0, 255, 0, FALSE, MAG_AFFECTS, 0, STYPE_NEUTRAL);

	/*
	 * These spells are currently not used, not implemented, and not castable.
	 * Values for the 'breath' spells are filled in assuming a dragon's breath.
	 */

	spello(SPELL_FIRE_BREATH, "огненное дыхание", "fire breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, 0, 0, STYPE_FIRE);

	spello(SPELL_GAS_BREATH, "зловонное дыхание", "gas breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, 0, 0, STYPE_EARTH);

	spello(SPELL_FROST_BREATH, "ледяное дыхание", "frost breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, 0, 0, STYPE_AIR);

	spello(SPELL_ACID_BREATH, "кислотное дыхание", "acid breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, 0, 0, STYPE_WATER);

	spello(SPELL_LIGHTNING_BREATH, "опаляющее дыхание", "lightning breath",
		   0, 0, 0, POS_SITTING, TAR_IGNORE, TRUE, 0, 0, STYPE_DARK);

	spello(SPELL_QUEST, "чары", "quest spell",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1, STYPE_NEUTRAL);
	spello(SPELL_SOLOBONUS, "награда", "bonus",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1, STYPE_NEUTRAL);


	/*
	 * Declaration of skills - this actually doesn't do anything except
	 * set it up so that immortals can use these skills by default.  The
	 * min level to use the skill for other classes is set up in class.c.
	 */

	skillo(SKILL_BACKSTAB, "заколоть", 140);
	skillo(SKILL_BASH, "сбить", 200);
	skillo(SKILL_HIDE, "спрятаться", 100);
	skillo(SKILL_KICK, "пнуть", 100);
	skillo(SKILL_PICK_LOCK, "взломать", 120);
	skillo(SKILL_PUNCH, "кулачный бой", 100);
	skillo(SKILL_RESCUE, "спасти", 130);
	skillo(SKILL_SNEAK, "подкрасться", 100);
	skillo(SKILL_STEAL, "украсть", 100);
	skillo(SKILL_TRACK, "выследить", 100);
	skillo(SKILL_PARRY, "парировать", 120);
	skillo(SKILL_BLOCK, "блокировать щитом", 200);
	skillo(SKILL_TOUCH, "перехватить атаку", 100);
	skillo(SKILL_PROTECT, "прикрыть", 120);
	skillo(SKILL_BOTHHANDS, "двуручники", 160);
	skillo(SKILL_LONGS, "длинные лезвия", 160);
	skillo(SKILL_SPADES, "копья и рогатины", 160);
	skillo(SKILL_SHORTS, "короткие лезвия", 160);
	skillo(SKILL_BOWS, "луки", 160);
	skillo(SKILL_CLUBS, "палицы и дубины", 160);
	skillo(SKILL_PICK, "проникающее оружие", 160);
	skillo(SKILL_NONSTANDART, "иное оружие", 160);
	skillo(SKILL_AXES, "секиры", 160);
	skillo(SKILL_SATTACK, "атака левой рукой", 100);
	skillo(SKILL_LOOKING, "приглядеться", 100);
	skillo(SKILL_HEARING, "прислушаться", 100);

	skillo(SKILL_DISARM, "обезоружить", 100);
	skillo(SKILL_HEAL, "!heal!", 100);
	skillo(SKILL_MORPH, "оборотничество", 150);
	skillo(SKILL_ADDSHOT, "дополнительный выстрел", 200);
	skillo(SKILL_CAMOUFLAGE, "маскировка", 100);
	skillo(SKILL_DEVIATE, "уклониться", 100);
	skillo(SKILL_CHOPOFF, "подножка", 200);
	skillo(SKILL_REPAIR, "ремонт", 100);
	skillo(SKILL_COURAGE, "ярость", 100);
	skillo(SKILL_IDENTIFY, "опознание", 100);
	skillo(SKILL_LOOK_HIDE, "подсмотреть", 100);
	skillo(SKILL_UPGRADE, "заточить", 100);
	skillo(SKILL_ARMORED, "укрепить", 100);
	skillo(SKILL_DRUNKOFF, "опохмелиться", 100);
	skillo(SKILL_AID, "лечить", 100);
	skillo(SKILL_FIRE, "разжечь костер", 160);
	skillo(SKILL_SHIT, "удар левой рукой", 100);
	skillo(SKILL_MIGHTHIT, "богатырский молот", 200);
	skillo(SKILL_STUPOR, "оглушить", 160);
	skillo(SKILL_POISONED, "отравить", 200);
	skillo(SKILL_LEADERSHIP, "лидерство", 100);
	skillo(SKILL_PUNCTUAL, "точный стиль", 110);
	skillo(SKILL_AWAKE, "осторожный стиль", 100);
	skillo(SKILL_SENSE, "найти", 160);
	skillo(SKILL_HORSE, "сражение верхом", 100);
	skillo(SKILL_HIDETRACK, "замести следы", 120);
	skillo(SKILL_RELIGION, "!молитва или жертва!", 100);
	skillo(SKILL_MAKEFOOD, "освежевать", 120);
	skillo(SKILL_MULTYPARRY, "веерная защита", 140);
	skillo(SKILL_TRANSFORMWEAPON, "перековать", 140);
	skillo(SKILL_THROW, "метнуть", 150);
//  skillo(SKILL_CREATEBOW,    "смастерить лук", 140);
	skillo(SKILL_MAKE_BOW, "смастерить лук", 140);
	skillo(SKILL_MAKE_WEAPON, "выковать оружие", 140);
	skillo(SKILL_MAKE_ARMOR, "выковать доспех", 140);
	skillo(SKILL_MAKE_WEAR, "сшить одежду", 140);
	skillo(SKILL_MAKE_JEWEL, "смастерить диковинку", 140);
	skillo(SKILL_MANADRAIN, "сглазить", 100);
	skillo(SKILL_NOPARRYHIT, "скрытый удар", 100);
	skillo(SKILL_TOWNPORTAL, "врата", 100);
	skillo(SKILL_DIG, "горное дело", 100);
	skillo(SKILL_INSERTGEM, "ювелир", 100);
	skillo(SKILL_WARCRY, "боевой клич", 100);
	skillo(SKILL_TURN_UNDEAD, "изгнать нежить", 100);
	skillo(SKILL_IRON_WIND, "железный ветер", 150);
	skillo(SKILL_STRANGLE, "удавить", 200);
	skillo(SKILL_AIR_MAGIC, "магия воздуха", 200);
	skillo(SKILL_FIRE_MAGIC, "магия огня", 200);
	skillo(SKILL_WATER_MAGIC, "магия воды", 200);
	skillo(SKILL_EARTH_MAGIC, "магия земли", 200);
	skillo(SKILL_LIGHT_MAGIC, "магия света", 200);
	skillo(SKILL_DARK_MAGIC, "магия тьмы", 200);
	skillo(SKILL_MIND_MAGIC, "магия разума", 200);
	skillo(SKILL_LIFE_MAGIC, "магия жизни", 200);
	skillo(SKILL_STUN, "ошеломить", 200);
	skillo(SKILL_MAKE_AMULET, "смастерить оберег", 200);
}

MaxClassSlot::MaxClassSlot()
{
	for (int i = 0; i < NUM_PLAYER_CLASSES; ++i)
	{
		for (int k = 0; k < NUM_KIN; ++k)
		{
			max_class_slot_[i][k] = 0;
		}
	}
}

void MaxClassSlot::init(int chclass, int kin, int slot)
{
	if (max_class_slot_[chclass][kin] < slot)
	{
		max_class_slot_[chclass][kin] = slot;
	}
}

int MaxClassSlot::get(int chclass, int kin) const
{
	if (kin < 0
		|| kin >= NUM_KIN
		|| chclass < 0
		|| chclass >= NUM_PLAYER_CLASSES)
	{
		return 0;
	}
	return max_class_slot_[chclass][kin];
}

int MaxClassSlot::get(const CHAR_DATA* ch) const
{
	return max_slots.get(GET_CLASS(ch), GET_KIN(ch));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
