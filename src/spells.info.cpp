#include "spells.info.h"

#include "spells.h"
#include "utils.h"

const char *unused_spellname = "!UNUSED!";

struct spellInfo_t spell_info[TOP_SPELL_DEFINE + 1];
struct spell_create_type spell_create[TOP_SPELL_DEFINE + 1];

void initUnusedSpell(int spl);
void initSpell(int spl, const char *name, const char *syn,
	   int max_mana, int min_mana, int mana_change,
	   int minpos, int targets, int violent, int routines, int danger, int spell_class);
void initSpells(void);
const char *spell_name(int num);


const char *spell_name(int num) {
	if (num > 0 && num <= TOP_SPELL_DEFINE) {
		return (spell_info[num].name);
	} else {
		if (num == -1) {
			return unused_spellname;
		} else {
			return "UNDEFINED";
		}
	}
}

void initUnusedSpell(int spl) {
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < NUM_KIN; j++)
		{
			spell_info[spl].min_remort[i][j] = MAX_REMORT;
			spell_info[spl].min_level[i][j] = LVL_IMPL + 1;
			spell_info[spl].slot_forc[i][j] = MAX_SLOT;
			spell_info[spl].class_change[i][j] = 0;
		}
	}

	for (i = 0; i < 3; i++) {
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

void initSpell(int spl, const char *name, const char *syn,
	   int max_mana, int min_mana, int mana_change,
	   int minpos, int targets, int violent, int routines, int danger, int spell_class) {

	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < NUM_KIN; j++) {
			spell_info[spl].min_remort[i][j] = MAX_REMORT;
			spell_info[spl].min_level[i][j] = LVL_IMPL;
			spell_info[spl].slot_forc[i][j] = MAX_SLOT;
			spell_info[spl].class_change[i][j] = 0;
		}
	}

	spell_create[spl].wand.min_caster_level = LVL_GRGOD;
	spell_create[spl].scroll.min_caster_level = LVL_GRGOD;
	spell_create[spl].potion.min_caster_level = LVL_GRGOD;
	spell_create[spl].items.min_caster_level = LVL_GRGOD;
	spell_create[spl].runes.min_caster_level = LVL_GRGOD;

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

/*
 * Arguments for initSpell calls:
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

void initSpells(void) {

	for (int i = 0; i <= TOP_SPELL_DEFINE; i++) {
		initUnusedSpell(i);
	}


//1
	initSpell(SPELL_ARMOR, "защита", "armor", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//2
	initSpell(SPELL_TELEPORT, "прыжок", "teleport",
		   140, 120, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL | NPC_DAMAGE_PC, 1, STYPE_AIR);
//3
	initSpell(SPELL_BLESS, "доблесть", "bless", 55, 40, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//4
	initSpell(SPELL_BLINDNESS, "слепота", "blind",
		   70, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_DARK);
//5
	initSpell(SPELL_BURNING_HANDS, "горящие руки", "burning hands", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_FIRE);
//6
	initSpell(SPELL_CALL_LIGHTNING, "шаровая молния", "call lightning", 85, 70, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, NPC_AFFECT_PC | MAG_AFFECTS | MAG_DAMAGE | NPC_DAMAGE_PC, 2, STYPE_AIR);
//7
	initSpell(SPELL_CHARM, "подчинить разум", "mind control", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1, STYPE_MIND);
//8
	initSpell(SPELL_CHILL_TOUCH, "ледяное прикосновение", "chill touch",
		   55, 45, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 1, STYPE_WATER);
//9
	initSpell(SPELL_CLONE, "клонирование", "clone",
		   150, 130, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS, 0, STYPE_DARK);
//10
	initSpell(SPELL_COLOR_SPRAY, "ледяные стрелы", "ice bolts", 90, 75, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   3, STYPE_WATER);
//11
	initSpell(SPELL_CONTROL_WEATHER, "контроль погоды", "weather control",
		   100, 90, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 0, STYPE_AIR);
//12
	initSpell(SPELL_CREATE_FOOD, "создать пищу", "create food",
		   40, 30, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS, 0, STYPE_LIFE);
//13
	initSpell(SPELL_CREATE_WATER, "создать воду", "create water", 40, 30, 1,
		   POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM, FALSE, MAG_MANUAL, 0, STYPE_WATER);
//14
	initSpell(SPELL_CURE_BLIND, "вылечить слепоту", "cure blind", 110, 90, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0, STYPE_LIGHT);
//15
	initSpell(SPELL_CURE_CRITIC, "критическое исцеление", "critical cure",
		   100, 90, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 3, STYPE_LIFE);
//16
	initSpell(SPELL_CURE_LIGHT, "легкое исцеление", "light cure",
		   40, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 1, STYPE_LIFE);
//17
	initSpell(SPELL_CURSE, "проклятие", "curse", 55, 40, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV, MTYPE_NEUTRAL,
		MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 1, STYPE_DARK);
//18
	initSpell(SPELL_DETECT_ALIGN, "определение наклонностей", "detect alignment",
		   40, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//19
	initSpell(SPELL_DETECT_INVIS, "видеть невидимое", "detect invisible",
		   100, 55, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//20
	initSpell(SPELL_DETECT_MAGIC, "определение магии", "detect magic",
		   100, 55, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//21
	initSpell(SPELL_DETECT_POISON, "определение яда", "detect poison",
		   40, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//22
	initSpell(SPELL_DISPEL_EVIL, "изгнать зло", "dispel evil",
		   100, 90, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE, 1, STYPE_LIGHT);
//23
	initSpell(SPELL_EARTHQUAKE, "землетрясение", "earthquake", 110, 90, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC, 2, STYPE_EARTH);
//24
	initSpell(SPELL_ENCHANT_WEAPON, "заколдовать оружие", "enchant weapon",
		   140, 110, 2, POS_STANDING, TAR_OBJ_INV, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//25
	initSpell(SPELL_ENERGY_DRAIN, "истощить энергию", "energy drain",
		   150, 140, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_MANUAL | MAG_AFFECTS | NPC_DAMAGE_PC, 1, STYPE_DARK);
//26
	initSpell(SPELL_FIREBALL, "огненный шар", "fireball",
		   110, 100, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   2, STYPE_FIRE);
//27
	initSpell(SPELL_HARM, "вред", "harm",
		   110, 100, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 5, STYPE_DARK);
//28
	initSpell(SPELL_HEAL, "исцеление", "heal", 110, 100, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 10, STYPE_LIFE);
//29
	initSpell(SPELL_INVISIBLE, "невидимость", "invisible",
		   50, 40, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
		   FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0,  STYPE_MIND);
//30
	initSpell(SPELL_LIGHTNING_BOLT, "молния", "lightning bolt", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_AIR);
//31
	initSpell(SPELL_LOCATE_OBJECT, "разыскать предмет", "locate object",
		   140, 110, 2, POS_STANDING, TAR_OBJ_WORLD, FALSE, MAG_MANUAL, 0, STYPE_MIND);
//32
	initSpell(SPELL_MAGIC_MISSILE, "магическая стрела", "magic missle",
		   40, 30, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_FIRE);
//33
	initSpell(SPELL_POISON, "яд", "poison", 70, 55, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT,
		MTYPE_NEUTRAL, MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 2, STYPE_LIFE);
//34
	initSpell(SPELL_PROT_FROM_EVIL, "защита от тьмы", "protect evil", 60, 45, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//35
	initSpell(SPELL_REMOVE_CURSE, "снять проклятие", "remove curse",
		   50, 40, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
		   MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0, STYPE_LIGHT);
//36
	initSpell(SPELL_SANCTUARY, "освящение", "sanctuary", 85, 70, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//37
	initSpell(SPELL_SHOCKING_GRASP, "обжигающая хватка", "shocking grasp", 50, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_FIRE);
//38
	initSpell(SPELL_SLEEP, "сон", "sleep",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//39
	initSpell(SPELL_STRENGTH, "сила", "strength", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//40
	initSpell(SPELL_SUMMON, "призвать", "summon",
		   110, 100, 2, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, 0, STYPE_MIND);
//41
	initSpell(SPELL_PATRONAGE, "покровительство", "patronage", 85, 70, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS, 1, STYPE_LIGHT);
//42
	initSpell(SPELL_WORD_OF_RECALL, "слово возврата", "recall", 140, 100, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_MANUAL | NPC_DAMAGE_PC, 0, STYPE_MIND);
//43
	initSpell(SPELL_REMOVE_POISON, "удалить яд", "remove poison",
		60, 45, 2, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE,
		MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0, STYPE_LIFE);
//44
	initSpell(SPELL_SENSE_LIFE, "определение жизни", "sense life",
		   85, 70, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_LIFE);
//45
	initSpell(SPELL_ANIMATE_DEAD, "поднять труп", "animate dead",
		   50, 35, 3, POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0, STYPE_DARK);
//46
	initSpell(SPELL_DISPEL_GOOD, "рассеять свет", "dispel good", 100, 90, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE, 1, STYPE_DARK);
//47
	initSpell(SPELL_GROUP_ARMOR, "групповая защита", "group armor", 110, 100, 2,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//48
	initSpell(SPELL_GROUP_HEAL, "групповое исцеление", "group heal",
		   110, 100, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 30, STYPE_LIFE);
//49
	initSpell(SPELL_GROUP_RECALL, "групповой возврат", "group recall",
		   125, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_MANUAL, 0, STYPE_MIND);
//50
	initSpell(SPELL_INFRAVISION, "видение ночью", "infravision",
		   50, 40, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_LIGHT);
//51
	initSpell(SPELL_WATERWALK, "водохождение", "waterwalk",
		   70, 55, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_WATER);
//52
	initSpell(SPELL_CURE_SERIOUS, "серьезное исцеление", "serious cure", 85, 70, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 2, STYPE_LIFE);
//53
	initSpell(SPELL_GROUP_STRENGTH, "групповая сила", "group strength", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//54
	initSpell(SPELL_HOLD, "оцепенение", "hold",
		   100, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 3, STYPE_MIND);
//55
	initSpell(SPELL_POWER_HOLD, "длительное оцепенение", "power hold",
		   140, 90, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 4, STYPE_MIND);
//56
	initSpell(SPELL_MASS_HOLD, "массовое оцепенение", "mass hold",
		   150, 130, 5, POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC,
		   5, STYPE_MIND);
//57
	initSpell(SPELL_FLY, "полет", "fly", 50, 35, 1, POS_STANDING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0, STYPE_AIR);
//58
	initSpell(SPELL_BROKEN_CHAINS, "разбитые оковы", "broken chains", 125, 110, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 5, STYPE_MIND);

//59
	initSpell(SPELL_NOFLEE, "приковать противника", "noflee",
		   100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//60
	initSpell(SPELL_CREATE_LIGHT, "создать свет", "create light",
		   40, 30, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS, 0, STYPE_LIGHT);
//61
	initSpell(SPELL_DARKNESS, "тьма", "darkness", 100, 70, 2, POS_STANDING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0, STYPE_DARK);
//62
	initSpell(SPELL_STONESKIN, "каменная кожа", "stoneskin", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);
//63
	initSpell(SPELL_CLOUDLY, "затуманивание", "cloudly", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_WATER);
//64
	initSpell(SPELL_SILENCE, "молчание", "sielence",
		   100, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 1, STYPE_MIND);
//65
	initSpell(SPELL_LIGHT, "свет", "sun shine", 100, 70, 2, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//66
	initSpell(SPELL_CHAIN_LIGHTNING, "цепь молний", "chain lightning",
		   120, 110, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_AIR);
//67
	initSpell(SPELL_FIREBLAST, "огненный поток", "fireblast",
		   110, 90, 2, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC,
		   5, STYPE_FIRE);
//68
	initSpell(SPELL_IMPLOSION, "гнев богов", "implosion",
		   140, 120, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   15, STYPE_FIRE);
//69
	initSpell(SPELL_WEAKNESS, "слабость", "weakness",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_LIFE);
//70
	initSpell(SPELL_GROUP_INVISIBLE, "групповая невидимость", "group invisible",
		   150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS, 0, STYPE_MIND);
//71
	initSpell(SPELL_SHADOW_CLOAK, "мантия теней", "shadow cloak", 100, 70, 3,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_DARK);
//72
	initSpell(SPELL_ACID, "кислота", "acid",
		   90, 65, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | MAG_ALTER_OBJS | NPC_DAMAGE_PC, 2, STYPE_WATER);
//73
	initSpell(SPELL_REPAIR, "починка", "repair",
		   110, 100, 1, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//74
	initSpell(SPELL_ENLARGE, "увеличение", "enlarge",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC,
		   0, STYPE_LIFE);
//75
	initSpell(SPELL_FEAR, "страх", "fear",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_MANUAL | NPC_DAMAGE_PC, 1, STYPE_DARK);
//76
	initSpell(SPELL_SACRIFICE, "высосать жизнь", "sacrifice",
		   140, 125, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_MANUAL | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   10, STYPE_DARK);
//77
	initSpell(SPELL_WEB, "сеть", "web",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//78
	initSpell(SPELL_BLINK, "мигание", "blink", 70, 55, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);
//79
	initSpell(SPELL_REMOVE_HOLD, "снять оцепенение", "remove hold", 110, 90, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 1, STYPE_LIGHT);
//80
	initSpell(SPELL_CAMOUFLAGE, "!маскировка!", "!set by skill!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//81
	initSpell(SPELL_POWER_BLINDNESS, "полная слепота", "power blind",
		   110, 100, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_DARK);
//82
	initSpell(SPELL_MASS_BLINDNESS, "массовая слепота", "mass blind", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 4, STYPE_DARK);
//83
	initSpell(SPELL_POWER_SILENCE, "длительное молчание", "power sielence",
		   120, 90, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 2, STYPE_MIND);
//84
	initSpell(SPELL_EXTRA_HITS, "увеличить жизнь", "extra hits",
		   100, 85, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_POINTS | NPC_DUMMY, 1, STYPE_LIFE);
//85
	initSpell(SPELL_RESSURECTION, "оживить труп", "ressurection",
		   120, 100, 2, POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0, STYPE_DARK);
//86
	initSpell(SPELL_MAGICSHIELD, "волшебный щит", "magic shield", 50, 30, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIGHT);

//87
	initSpell(SPELL_FORBIDDEN, "запечатать комнату", "forbidden",
		125, 110, 2, POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_ROOM, 0, STYPE_MIND);
//88
	initSpell(SPELL_MASS_SILENCE, "массовое молчание", "mass sielence", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 3, STYPE_MIND);
//89
	initSpell(SPELL_REMOVE_SILENCE, "снять молчание", "remove sielence",
		   70, 55, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER,
		   1, STYPE_LIGHT);
//90
	initSpell(SPELL_DAMAGE_LIGHT, "легкий вред", "light damage",
		   40, 30, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 1, STYPE_DARK);
//91
	initSpell(SPELL_DAMAGE_SERIOUS, "серьезный вред", "serious damage",
		   85, 55, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 2, STYPE_DARK);
//92
	initSpell(SPELL_DAMAGE_CRITIC, "критический вред", "critical damage",
		   100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 3, STYPE_DARK);
//93
	initSpell(SPELL_MASS_CURSE, "массовое проклятие", "mass curse", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_DARK);
//94
	initSpell(SPELL_ARMAGEDDON, "суд богов", "armageddon", 150, 130, 5,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | NPC_DAMAGE_PC, 10, STYPE_AIR);
//95
	initSpell(SPELL_GROUP_FLY, "групповой полет", "group fly",
		   140, 120, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS, 0, STYPE_AIR);
//96
	initSpell(SPELL_GROUP_BLESS, "групповая доблесть", "group bless", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//97
	initSpell(SPELL_REFRESH, "восстановление", "refresh",
		   80, 60, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_POINTS, 0,  STYPE_LIFE);
//98
	initSpell(SPELL_STUNNING, "каменное проклятие", "stunning", 150, 140, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE, 15, STYPE_EARTH);

//99
	initSpell(SPELL_HIDE, "!спрятался!", "!set by skill!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);

//100
	initSpell(SPELL_SNEAK, "!крадется!", "!set by skill!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);

//101
	initSpell(SPELL_DRUNKED, "!опьянение!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//102
	initSpell(SPELL_ABSTINENT, "!абстиненция!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);

//103
	initSpell(SPELL_FULL, "насыщение", "full", 70, 55, 1,
		   POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_POINTS, 10,  STYPE_LIFE);
//104
	initSpell(SPELL_CONE_OF_COLD, "ледяной ветер", "cold wind",
		   100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE, MAG_AFFECTS | MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,
		   15, STYPE_WATER);
//105
	initSpell(SPELL_BATTLE, "!получил в бою!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//106
	initSpell(SPELL_HAEMORRAGIA, "!кровотечение!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//107
	initSpell(SPELL_COURAGE, "!ярость!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//108
	initSpell(SPELL_WATERBREATH, "дышать водой", "waterbreath",
		   85, 70, 4, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0, STYPE_WATER);
//109
	initSpell(SPELL_SLOW, "медлительность", "slow",
		   55, 40, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//110
	initSpell(SPELL_HASTE, "ускорение", "haste", 55, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//111
	initSpell(SPELL_MASS_SLOW, "массовая медлительность", "mass slow", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_MIND);
//112
	initSpell(SPELL_GROUP_HASTE, "групповое ускорение", "group haste", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//113
	initSpell(SPELL_SHIELD, "защита богов", "gods shield", 150, 140, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 2, STYPE_LIGHT);
//114
	initSpell(SPELL_PLAQUE, "лихорадка", "plaque",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_LIFE);
//115
	initSpell(SPELL_CURE_PLAQUE, "вылечить лихорадку", "cure plaque", 85, 70, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0, STYPE_LIFE);
//116
	initSpell(SPELL_AWARNESS, "внимательность", "awarness", 100, 90, 1,
		   POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//117
	initSpell(SPELL_RELIGION, "!молитва или жертва!", "!pray or donate!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);
//118
	initSpell(SPELL_AIR_SHIELD, "воздушный щит", "air shield", 140, 120, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_AIR);
//119
	initSpell(SPELL_PORTAL, "переход", "portal", 200, 180, 4,
		   POS_STANDING, TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0, STYPE_LIGHT);
//120
	initSpell(SPELL_DISPELL_MAGIC, "развеять магию", "dispel magic",
		   85, 70, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS, 0, STYPE_LIGHT);
//121
	initSpell(SPELL_SUMMON_KEEPER, "защитник", "keeper",
		   100, 80, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, 0, STYPE_LIGHT);
//122
	initSpell(SPELL_FAST_REGENERATION, "быстрое восстановление",
		   "fast regeneration", 100, 90, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//123
	initSpell(SPELL_CREATE_WEAPON, "создать оружие", "create weapon",
		   130, 110, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 0, STYPE_LIGHT);
//124
	initSpell(SPELL_FIRE_SHIELD, "огненный щит", "fire shield", 140, 120, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_FIRE);
//125
	initSpell(SPELL_RELOCATE, "переместиться", "relocate",
		   140, 120, 2, POS_STANDING, TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0, STYPE_AIR);
//126
	initSpell(SPELL_SUMMON_FIREKEEPER, "огненный защитник", "fire keeper",
		   150, 140, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, 0, STYPE_FIRE);
//127
	initSpell(SPELL_ICE_SHIELD, "ледяной щит", "ice protect", 140, 120, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_WATER);
//128
	initSpell(SPELL_ICESTORM, "ледяной шторм", "ice storm",
		   125, 110, 2, POS_FIGHTING,
		   TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 5, STYPE_WATER);
//129
	initSpell(SPELL_ENLESS, "уменьшение", "enless",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS, 0, STYPE_LIFE);
//130
	initSpell(SPELL_SHINEFLASH, "яркий блик", "shine flash",
		   60, 45, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_FIRE);
//131
	initSpell(SPELL_MADNESS, "безумие", "madness",
		   130, 110, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//132
	initSpell(SPELL_GROUP_MAGICGLASS, "магическое зеркало", "group magicglass",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 4, STYPE_AIR);
//133
	initSpell(SPELL_CLOUD_OF_ARROWS, "облако стрел", "cloud of arrous", 95, 80, 2,
		   POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 4, STYPE_FIRE);
//134
	initSpell(SPELL_VACUUM, "круг пустоты", "vacuum sphere",
		   150, 140, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_DAMAGE | NPC_DAMAGE_PC, 15, STYPE_DARK);
//135
	initSpell(SPELL_METEORSTORM, "метеоритный дождь", "meteor storm", 125, 110, 2,
		   POS_FIGHTING, TAR_ROOM_THIS, FALSE, MAG_NEED_CONTROL | MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_EARTH);
//136
	initSpell(SPELL_STONEHAND, "каменные руки", "stonehand", 40, 30, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);
//137
	initSpell(SPELL_MINDLESS, "повреждение разума", "mindness",
		   120, 110, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER,
		   0, STYPE_MIND);
//138
	initSpell(SPELL_PRISMATICAURA, "призматическая аура", "prismatic aura", 85, 70, 4,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//139
	initSpell(SPELL_EVILESS, "силы зла", "eviless", 150, 130, 5,
		   POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | MAG_POINTS, 3, STYPE_DARK);
//140
	initSpell(SPELL_AIR_AURA, "воздушная аура", "air aura",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_AIR);
//141
	initSpell(SPELL_FIRE_AURA, "огненная аура", "fire aura",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_FIRE);
//142
	initSpell(SPELL_ICE_AURA, "ледяная аура", "ice aura",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_WATER);
//143
	initSpell(SPELL_SHOCK, "шок", "shock", 100, 90, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_DARK);
//144
	initSpell(SPELL_MAGICGLASS, "зеркало магии", "magic glassie", 120, 110, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 2, STYPE_LIGHT);

//145
	initSpell(SPELL_GROUP_SANCTUARY, "групповое освящение", "group sanctuary", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);

//146
	initSpell(SPELL_GROUP_PRISMATICAURA, "групповая призматическая аура",
		   "group prismatic aura", 110, 100, 1, POS_FIGHTING, TAR_IGNORE,
		   FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);

//147
	initSpell(SPELL_DEAFNESS, "глухота", "deafness",
		   100, 40, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 1, STYPE_MIND);

//148
	initSpell(SPELL_POWER_DEAFNESS, "длительная глухота", "power deafness",
		   120, 90, 4, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
		   MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 2, STYPE_MIND);

//149
	initSpell(SPELL_REMOVE_DEAFNESS, "снять глухоту", "remove deafness",
		   90, 80, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER,
		   1, STYPE_LIFE);

//150
	initSpell(SPELL_MASS_DEAFNESS, "массовая глухота", "mass deafness", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_MIND);

//151
	initSpell(SPELL_DUSTSTORM, "пылевая буря", "dust storm",
		   125, 110, 2, POS_FIGHTING,
		   TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 5, STYPE_EARTH);

//152
	initSpell(SPELL_EARTHFALL, "камнепад", "earth fall",
		   120, 110, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_EARTH);

//153
	initSpell(SPELL_SONICWAVE, "звуковая волна", "sonic wave",
		    120, 110, 2, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE,
		    MAG_MASSES | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1, STYPE_AIR);

//154
	initSpell(SPELL_HOLYSTRIKE, "силы света", "holystrike",
		   150, 130, 5, POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MANUAL | NPC_DAMAGE_PC, 10, STYPE_LIGHT);

//155
	initSpell(SPELL_ANGEL, "ангел-хранитель", "angel", 150, 130, 5,
		   POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 1, STYPE_LIGHT);
//156
	initSpell(SPELL_MASS_FEAR, "массовый страх", "mass fear", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_MANUAL | NPC_AFFECT_PC, 4, STYPE_DARK);
//157
	initSpell(SPELL_FASCINATION, "обаяние", "fascination", 480, 440, 20,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 2, STYPE_MIND);
//158
	initSpell(SPELL_CRYING, "плач", "crying",
		   120, 55, 2, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//159
	initSpell(SPELL_OBLIVION, "забвение", "oblivion",
		   70, 55, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_DARK);
//160
	initSpell(SPELL_BURDEN_OF_TIME, "бремя времени", "burden time", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 4, STYPE_DARK);
//161
	initSpell(SPELL_GROUP_REFRESH, "групповое восстановление", "group refresh",	//Added by Adept
		   160, 140, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 30, STYPE_LIFE);

//162
	initSpell(SPELL_PEACEFUL, "смирение", "peaceful",
		   55, 40, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL, MAG_AFFECTS | NPC_AFFECT_PC, 1, STYPE_MIND);
//163
	initSpell(SPELL_MAGICBATTLE, "!получил в бою!", "!set by programm!", 0, 0, 0, 255, 0,
		   FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);

//164
	initSpell(SPELL_BERSERK, "!предсмертная ярость!", "!set by programm!", 0, 0, 0, 255, 0,
		   FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//165
	initSpell(SPELL_STONEBONES, "каменные кости", "stone bones", 80, 40, 1,
		   POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);

//166 - SPELL_ROOM_LIGHT
	initSpell(SPELL_ROOM_LIGHT, "осветить комнату", "room light", 10, 10, 1,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM , 0, STYPE_LIGHT);

//167 - SPELL_POISONED_FOG
	initSpell(SPELL_POISONED_FOG, "ядовитый туман", "poisoned fog", 10, 10, 1,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_LIFE);

//168 - SPELL_THUNDERSTORM
	initSpell(SPELL_THUNDERSTORM, "буря отмщения", "storm of vengeance", 10, 10, 1,
		   POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_NEED_CONTROL | MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_AIR);
//169
	initSpell(SPELL_LIGHT_WALK, "!легкая поступь!", "!set by programm!", 0, 0, 0, 255, 0,
		   FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//170
	initSpell(SPELL_FAILURE, "недоля", "failure", 100, 85, 2, POS_FIGHTING,
		   TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 5, STYPE_DARK);

//171
	initSpell(SPELL_CLANPRAY, "!клановые чары!", "!clan affect!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0,  STYPE_NEUTRAL);
//172
	initSpell(SPELL_GLITTERDUST, "блестящая пыль", "glitterdust", 120, 100, 3,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 5, STYPE_EARTH);
//173
	initSpell(SPELL_SCREAM, "вопль", "scream", 100, 85, 3, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_AIR);
//174
	initSpell(SPELL_CATS_GRACE, "кошачья ловкость", "cats grace", 50, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//175
	initSpell(SPELL_BULL_BODY, "бычье тело", "bull body", 50, 40, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//176
	initSpell(SPELL_SNAKE_WISDOM, "мудрость змеи", "snake wisdom", 60, 50, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//177
	initSpell(SPELL_GIMMICKRY, "хитроумие", "gimmickry", 60, 50, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
// ДЛЯ КЛИЧЕЙ ПОЛЕ mana_change ИСПОЛЬЗУЕТСЯ
// ДЛЯ УКАЗАНИЯ МИНИМАЛЬНОГО ПРОЦЕНТА СКИЛЛА,
// С КОТОРОГО ДОСТУПЕН УКАЗАННЫЙ КЛИЧ
//178
/*
	initSpell(SPELL_WC_OF_CHALLENGE, "клич вызова", "warcry of challenge", 10, 10, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC, 0, STYPE_MIND);
//179
	initSpell(SPELL_WC_OF_MENACE, "клич угрозы", "warcry of menace", 30, 30, 40,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//180
	initSpell(SPELL_WC_OF_RAGE, "клич ярости", "warcry of rage", 30, 30, 50,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 0, STYPE_MIND);
//181
	initSpell(SPELL_WC_OF_MADNESS, "клич безумия", "warcry of madness", 50, 50, 91,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//182
	initSpell(SPELL_WC_OF_THUNDER, "клич грома", "warcry of thunder", 140, 140, 141,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 0, STYPE_MIND);
*/
//183
	initSpell(SPELL_WC_OF_DEFENSE, "клич обороны", "warcry of defense", 10, 10, 10,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//184
	initSpell(SPELL_WC_OF_BATTLE, "клич битвы", "warcry of battle", 20, 20, 50,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//185
	initSpell(SPELL_WC_OF_POWER, "клич мощи", "warcry of power", 25, 25, 70,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_POINTS | MAG_AFFECTS | NPC_DUMMY | NPC_AFFECT_NPC, 0, STYPE_MIND);
//186
	initSpell(SPELL_WC_OF_BLESS, "клич доблести", "warcry of bless", 15, 15, 30,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//187
	initSpell(SPELL_WC_OF_COURAGE, "клич отваги", "warcry of courage", 10, 10, 10,
		   POS_FIGHTING, TAR_IGNORE, FALSE,
		   MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//188
	initSpell(SPELL_RUNE_LABEL, "рунная метка", "rune label", 50, 35, 1,
		POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM | MAG_CASTER_INWORLD_DELAY, 0, STYPE_LIGHT);


	// NON-castable spells should appear below here.

// 189
	initSpell(SPELL_ACONITUM_POISON, "яд аконита", "aconitum poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);
// 190
	initSpell(SPELL_SCOPOLIA_POISON, "яд скополии", "scopolia poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);
// 191
	initSpell(SPELL_BELENA_POISON, "яд белены", "belena poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);
// 192
	initSpell(SPELL_DATURA_POISON, "яд дурмана", "datura poison",
		0, 0, 0, POS_FIGHTING, TAR_IGNORE, MTYPE_AGGRESSIVE, MAG_AFFECTS, 0, STYPE_LIFE);


// 193
	initSpell(SPELL_TIMER_REPAIR, "обновление таймера", " timer repair",
		   110, 100, 1, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);

//194
	initSpell(SPELL_LACKY, "боевое везение", "lacky", 100, 90, 1,
		POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0, STYPE_MIND);
//195
	initSpell(SPELL_BANDAGE, "перевязка", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//196
	initSpell(SPELL_NO_BANDAGE, "!нельзя перевязываться!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//197
	initSpell(SPELL_CAPABLE, "!зачарован!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//198
	initSpell(SPELL_STRANGLE, "!удушье!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//199
	initSpell(SPELL_RECALL_SPELLS, "!вспоминает заклинания!", "!set by programm!", 0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//200
	initSpell(SPELL_HYPNOTIC_PATTERN, "чарующий узор", "hypnotic pattern", 120, 100, 2,
			POS_STANDING, TAR_ROOM_THIS, FALSE, MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_MIND);
//202
	initSpell(SPELL_VAMPIRE, "вампиризм", "vampire", 150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,
		   3, STYPE_DARK);
//203
	initSpell(SPELLS_RESTORATION, "реконструкция", "reconstruction",
		   110, 100, 1, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0, STYPE_LIGHT);
//204
	initSpell(SPELL_AURA_DEATH, "аура смерти", "aura death",
		   140, 120, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_DARK);
//205
	initSpell(SPELL_RECOVERY, "темное прикосновение", "recovery",
		   110, 100, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_DARK);
//206
	initSpell(SPELL_MASS_RECOVERY, "прикосновение смерти", "mass recovery",
		   110, 100, 1, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 0, STYPE_DARK);
//207
	initSpell(SPELL_AURA_EVIL, "аура зла", "aura evil",
		   150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,  3, STYPE_DARK);
//208
	initSpell(SPELL_MENTAL_SHADOW, "ментальная тень", "mental shadow", 150, 130, 5,
		   POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, 1, STYPE_MIND);
//209
	initSpell(SPELL_EVARDS_BLACK_TENTACLES, "навьи руки", "evards black tentacles", 120, 110, 2,
		   POS_FIGHTING, TAR_ROOM_THIS, FALSE, MAG_NEED_CONTROL | MAG_ROOM | MAG_CASTER_INROOM, 0, STYPE_DARK);
//210
	initSpell(SPELL_WHIRLWIND, "вихрь", "whirlwind", 110, 100, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_AIR);
//211
	initSpell(SPELL_INDRIKS_TEETH, "зубы индрика", "indriks teeth", 60, 45, 1, POS_FIGHTING,
		   TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//212
	initSpell(SPELL_MELFS_ACID_ARROW, "кислотная стрела", "acid arrow", 110, 100, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_WATER);
//213
	initSpell(SPELL_THUNDERSTONE, "громовой камень", "thunderstone", 110, 100, 2,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//214
	initSpell(SPELL_CLOD, "глыба", "clod", 110, 100, 1,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//215
	initSpell(SPELL_EXPEDIENT, "!боевой прием!", "!set by programm!",
		   0, 0, 0, 255, 0, FALSE, MAG_MANUAL, 0, STYPE_NEUTRAL);
//216
	initSpell(SPELL_SIGHT_OF_DARKNESS, "зрение тьмы", "sight darkness", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);
//217
	initSpell(SPELL_GENERAL_SINCERITY, "общая искренность", "general sincerity", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//218
	initSpell(SPELL_MAGICAL_GAZE, "магический взор", "magical gaze", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//219
	initSpell(SPELL_ALL_SEEING_EYE, "всевидящее око", "allseeing eye", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//220
	initSpell(SPELL_EYE_OF_GODS, "око богов", "eye gods", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIFE);
//221
	initSpell(SPELL_BREATHING_AT_DEPTH, "дыхание глубин", "breathing at depth", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_WATER);
//222
	initSpell(SPELL_GENERAL_RECOVERY, "общее восстановление", "general recovery", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIFE);
//223
	initSpell(SPELL_COMMON_MEAL, "общая трапеза", "common meal", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 1, STYPE_LIFE);
//224
	initSpell(SPELL_STONE_WALL, "каменная стена", "stone wall", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_EARTH);
//225
	initSpell(SPELL_SNAKE_EYES, "глаза змея", "snake eyes", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_MIND);
//226
	initSpell(SPELL_EARTH_AURA, "земной поклон", "earth aura", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_EARTH);
//227
	initSpell(SPELL_GROUP_PROT_FROM_EVIL, "групповая защита от тьмы", "group protect evil", 110, 100, 1,
		   POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 1, STYPE_LIGHT);

//228
	initSpell(SPELL_ARROWS_FIRE, "стрелы огня", "arrows of fire", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_FIRE);
//229
	initSpell(SPELL_ARROWS_WATER, "стрелы воды", "arrows of water", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_WATER);
//230
	initSpell(SPELL_ARROWS_EARTH, "стрелы земли", "arrows of earth", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_AREAS | MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_EARTH);
//231
	initSpell(SPELL_ARROWS_AIR, "стрелы воздуха", "arrows of air", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_AIR);
//232
	initSpell(SPELL_ARROWS_DEATH, "стрелы смерти", "arrows of death", 0, 0, 0,
		   POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_DAMAGE | NPC_AFFECT_PC | MAG_AFFECTS | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2, STYPE_DARK);
//233
	initSpell(SPELL_PALADINE_INSPIRATION, "воодушевление", "inspiration",
		   0, 0, 0, 255, 0, FALSE, MAG_GROUPS | MAG_AFFECTS, 0, STYPE_NEUTRAL);
//234
	initSpell(SPELL_DEXTERITY, "ловкость", "dexterity", 40, 30, 1,
		 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_LIFE);
//235
	initSpell(SPELL_GROUP_BLINK, "групповое мигание", "group blink",
		110, 100, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 30, STYPE_LIFE);
//236
	initSpell(SPELL_GROUP_CLOUDLY, "групповое затуманивание", "group cloudly",
		110, 100, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 30, STYPE_WATER);
//237
	initSpell(SPELL_GROUP_AWARNESS, "групповая внимательность", "group awarness",
		110, 100, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 30, STYPE_MIND);
//238
	initSpell(SPELL_WC_EXPERIENSE, "клич обучения", "warcry of training", 10, 10, 10,
		POS_FIGHTING, TAR_IGNORE, FALSE,  MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//239
	initSpell(SPELL_WC_LUCK, "клич везения", "warcry of luck", 10, 10, 10,
		POS_FIGHTING, TAR_IGNORE, FALSE,  MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//240
	initSpell(SPELL_WC_PHYSDAMAGE, "клич точности", "warcry of accuracy", 10, 10, 10,
		POS_FIGHTING, TAR_IGNORE, FALSE,  MAG_WARCRY | MAG_GROUPS | MAG_AFFECTS | NPC_AFFECT_NPC, 0, STYPE_MIND);
//241
	initSpell(SPELL_MASS_FAILURE, "взор Велеса", "gaze of Veles", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_DARK);
//242
	initSpell(SPELL_MASS_NOFLEE, "западня", "snare", 140, 120, 2,
		   POS_FIGHTING, TAR_IGNORE, MTYPE_NEUTRAL, MAG_MASSES | MAG_AFFECTS | NPC_AFFECT_PC, 2, STYPE_MIND);

	/*
	 * These spells are currently not used, not implemented, and not castable.
	 * Values for the 'breath' spells are filled in assuming a dragon's breath.
	 */


	initSpell(SPELL_IDENTIFY, "идентификация", "identify",
		   0, 0, 0, 0, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL, 0, STYPE_MIND);
//243 - 247
	initSpell(SPELL_FIRE_BREATH, "огненное дыхание", "fire breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, MAG_DAMAGE, 3, STYPE_FIRE);

	initSpell(SPELL_GAS_BREATH, "зловонное дыхание", "gas breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, MAG_DAMAGE, 3, STYPE_EARTH);

	initSpell(SPELL_FROST_BREATH, "ледяное дыхание", "frost breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, MAG_DAMAGE, 3, STYPE_AIR);

	initSpell(SPELL_ACID_BREATH, "кислотное дыхание", "acid breath", 0, 0, 0,
		   POS_SITTING, TAR_IGNORE, TRUE, MAG_DAMAGE, 3, STYPE_WATER);

	initSpell(SPELL_LIGHTNING_BREATH, "опаляющее дыхание", "lightning breath",
		   0, 0, 0, POS_SITTING, TAR_IGNORE, TRUE, MAG_DAMAGE, 3, STYPE_DARK);
// 357
	initSpell(SPELL_QUEST, "чары", "quest spell",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1, STYPE_NEUTRAL);
	initSpell(SPELL_SOLOBONUS, "награда", "bonus",
		   55, 40, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1, STYPE_NEUTRAL);
}


