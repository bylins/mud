#include "spells_info.h"

#include "spells.h"

const char *unused_spellname = "!UNUSED!";

struct SpellInfo spell_info[kSpellLast + 1];
struct SpellCreate spell_create[kSpellLast + 1];

void InitUnusedSpell(int spl);
void InitSpell(int spl, const char *name, const char *syn,
			   int max_mana, int min_mana, int mana_change,
			   int minpos, int targets, int violent, int routines, int danger, int spell_class);

const char *GetSpellName(int num) {
	if (num > 0 && num <= kSpellLast) {
		return (spell_info[num].name);
	} else {
		if (num == -1) {
			return unused_spellname;
		} else {
			return "UNDEFINED";
		}
	}
}

void InitUnusedSpell(int spl) {
	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			spell_info[spl].min_remort[i][j] = kMaxRemort;
			spell_info[spl].min_level[i][j] = kLvlImplementator + 1;
			spell_info[spl].slot_forc[i][j] = kMaxMemoryCircle;
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
	spell_info[spl].min_position = EPosition::kDead;
	spell_info[spl].danger = 0;
	spell_info[spl].targets = 0;
	spell_info[spl].violent = 0;
	spell_info[spl].routines = 0;
	spell_info[spl].name = unused_spellname;
	spell_info[spl].syn = unused_spellname;
}

void initSpell(int spl, const char *name, const char *syn,
			   int max_mana, int min_mana, int mana_change,
			   EPosition minpos, int targets, int violent, int routines, int danger, int spell_class) {

	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			spell_info[spl].min_remort[i][j] = kMaxRemort;
			spell_info[spl].min_level[i][j] = kLvlImplementator;
			spell_info[spl].slot_forc[i][j] = kMaxMemoryCircle;
			spell_info[spl].class_change[i][j] = 0;
		}
	}

	spell_create[spl].wand.min_caster_level = kLvlGreatGod;
	spell_create[spl].scroll.min_caster_level = kLvlGreatGod;
	spell_create[spl].potion.min_caster_level = kLvlGreatGod;
	spell_create[spl].items.min_caster_level = kLvlGreatGod;
	spell_create[spl].runes.min_caster_level = kLvlGreatGod;

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
 * violent :  true or false, depending on if this is considered a violent
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

void InitSpells() {

	for (int i = 0; i <= kSpellLast; i++) {
		InitUnusedSpell(i);
	}


//1
	initSpell(kSpellArmor, "защита", "armor", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeLight);
//2
	initSpell(kSpellTeleport, "прыжок", "teleport",
			  140, 120, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagManual | kNpcDamagePc, 1, kTypeAir);
//3
	initSpell(kSpellBless, "доблесть", "bless", 55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs | kNpcAffectNpc, 0, kTypeLight);
//4
	initSpell(kSpellBlindness, "слепота", "blind",
			  70, 40, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, kTypeDark);
//5
	initSpell(kSpellBurningHands, "горящие руки", "burning hands",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagAreas | kMagDamage | kNpcDamagePc, 1, kTypeFire);
//6
	initSpell(kSpellCallLighting, "шаровая молния", "call lightning", 85, 70,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kNpcAffectPc | kMagAffects | kMagDamage | kNpcDamagePc, 2, kTypeAir);
//7
	initSpell(kSpellCharm, "подчинить разум", "mind control",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual, 1, kTypeMind);
//8
	initSpell(kSpellChillTouch, "ледяное прикосновение", "chill touch",
			  55, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAffects | kNpcDamagePc, 1, kTypeWater);
//9
	initSpell(kSpellClone, "клонирование", "clone",
			  150, 130, 5, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagSummons, 0, kTypeDark);
//10
	initSpell(kSpellIceBolts, "ледяные стрелы", "ice bolts", 90, 75,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 3, kTypeWater);
//11
	initSpell(kSpellControlWeather, "контроль погоды", "weather control",
			  100, 90, 1, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 0, kTypeAir);
//12
	initSpell(kSpellCreateFood, "создать пищу", "create food",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, kTypeLife);
//13
	initSpell(kSpellCreateWater, "создать воду", "create water", 40, 30,
			  1, EPosition::kStand, kTarObjInv | kTarObjEquip | kTarCharRoom, false,
			  kMagManual, 0, kTypeWater);
//14
	initSpell(kSpellCureBlind, "вылечить слепоту", "cure blind", 110, 90,
			  2, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc, 0, kTypeLight);
//15
	initSpell(kSpellCureCritic, "критическое исцеление", "critical cure",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 3, kTypeLife);
//16
	initSpell(kSpellCureLight, "легкое исцеление", "light cure", 40, 30,
			  1, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagPoints | kNpcDummy, 1, kTypeLife);
//17
	initSpell(kSpellCurse, "проклятие", "curse",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict | kTarObjInv, kMtypeNeutral,
			  kMagAffects | kMagAlterObjs | kNpcAffectPc, 1, kTypeDark);
//18
	initSpell(kSpellDetectAlign, "определение наклонностей", "detect alignment",
			  40, 30, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//19
	initSpell(kSpellDetectInvis, "видеть невидимое", "detect invisible",
			  100, 55, 3, EPosition::kFight, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//20
	initSpell(kSpellDetectMagic, "определение магии", "detect magic",
			  100, 55, 3, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//21
	initSpell(kSpellDetectPoison, "определение яда", "detect poison",
			  40, 30, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, kTypeMind);
//22
	initSpell(kSpellDispelEvil, "изгнать зло", "dispel evil",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, kTypeLight);
//23
	initSpell(kSpellEarthquake, "землетрясение", "earthquake",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 2, kTypeEarth);
//24
	initSpell(kSpellEnchantWeapon, "заколдовать оружие", "enchant weapon", 140,
			  110, 2, EPosition::kStand, kTarObjInv, false, kMagAlterObjs,
			  0, kTypeLight);
//25
	initSpell(kSpellEnergyDrain, "истощить энергию", "energy drain",
			  150, 140, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kMagAffects | kNpcDamagePc, 1, kTypeDark);
//26
	initSpell(kSpellFireball, "огненный шар", "fireball",
			  110, 100, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeFire);
//27
	initSpell(kSpellHarm, "вред", "harm",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 5, kTypeDark);
//28
	initSpell(kSpellHeal, "исцеление", "heal",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 10, kTypeLife);
//29
	initSpell(kSpellInvisible, "невидимость", "invisible",
			  50, 40, 3, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjRoom, false,
			  kMagAffects | kMagAlterObjs, 0, kTypeMind);
//30
	initSpell(kSpellLightingBolt, "молния", "lightning bolt",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc, 1, kTypeAir);
//31
	initSpell(kSpellLocateObject, "разыскать предмет", "locate object",
			  140, 110, 2, EPosition::kStand, kTarObjWorld,
			  false, kMagManual, 0, kTypeMind);
//32
	initSpell(kSpellMagicMissile, "магическая стрела", "magic missle",
			  40, 30, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kNpcDamagePc, 1, kTypeFire);
//33
	initSpell(kSpellPoison, "яд", "poison", 70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarObjInv | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kMagAlterObjs | kNpcAffectPc, 2, kTypeLife);
//34
	initSpell(kSpellProtectFromEvil, "защита от тьмы", "protect evil",
			  60, 45, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//35
	initSpell(kSpellRemoveCurse, "снять проклятие", "remove curse",
			  50, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, kTypeLight);
//36
	initSpell(kSpellSanctuary, "освящение", "sanctuary",
			  85, 70, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//37
	initSpell(kSpellShockingGasp, "обжигающая хватка", "shocking grasp", 50, 40,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc, 1, kTypeFire);
//38
	initSpell(kSpellSleep, "сон", "sleep",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagAffects | kNpcAffectPc,
			  0, kTypeMind);
//39
	initSpell(kSpellStrength, "сила", "strength", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeLife);
//40
	initSpell(kSpellSummon, "призвать", "summon",
			  110, 100, 2, EPosition::kStand, kTarCharWorld | kTarNotSelf,
			  false, kMagManual, 0, kTypeMind);
//41
	initSpell(kSpellPatronage, "покровительство", "patronage",
			  85, 70, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagPoints | kMagAffects, 1, kTypeLight);
//42
	initSpell(kSpellWorldOfRecall, "слово возврата", "recall",
			  140, 100, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagManual | kNpcDamagePc, 0, kTypeMind);
//43
	initSpell(kSpellRemovePoison, "удалить яд", "remove poison",
			  60, 45, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjRoom, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, kTypeLife);
//44
	initSpell(kSpellSenseLife, "определение жизни", "sense life",
			  85, 70, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, kTypeLife);
//45
	initSpell(kSpellAnimateDead, "поднять труп", "animate dead",
			  50, 35, 3, EPosition::kStand, kTarObjRoom,
			  false, kMagSummons, 0, kTypeDark);
//46
	initSpell(kSpellDispelGood, "рассеять свет", "dispel good",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, kTypeDark);
//47
	initSpell(kSpellGroupArmor, "групповая защита", "group armor",
			  110, 100, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//48
	initSpell(kSpellGroupHeal, "групповое исцеление", "group heal",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, kTypeLife);
//49
	initSpell(kSpellGroupRecall, "групповой возврат", "group recall",
			  125, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagManual, 0, kTypeMind);
//50
	initSpell(kSpellInfravision, "видение ночью", "infravision",
			  50, 40, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeLight);
//51
	initSpell(kSpellWaterwalk, "водохождение", "waterwalk",
			  70, 55, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeWater);
//52
	initSpell(kSpellCureSerious, "серьезное исцеление", "serious cure",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 2, kTypeLife);
//53
	initSpell(kSpellGroupStrength, "групповая сила", "group strength",
			  140, 120, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//54
	initSpell(kSpellHold, "оцепенение", "hold",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 3, kTypeMind);
//55
	initSpell(kSpellPowerHold, "длительное оцепенение", "power hold",
			  140, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 4, kTypeMind);
//56
	initSpell(kSpellMassHold, "массовое оцепенение", "mass hold",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeMind);
//57
	initSpell(kSpellFly, "полет", "fly", 50, 35, 1, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeAir);
//58
	initSpell(kSpellBrokenChains, "разбитые оковы", "broken chains",
			  125, 110, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 5, kTypeMind);
//59
	initSpell(kSpellNoflee, "приковать противника", "noflee",
			  100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 0, kTypeMind);
//60
	initSpell(kSpellCreateLight, "создать свет", "create light",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, kTypeLight);
//61
	initSpell(kSpellDarkness, "тьма", "darkness",
			  100, 70, 2, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeDark);
//62
	initSpell(kSpellStoneSkin, "каменная кожа", "stoneskin", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeEarth);
//63
	initSpell(kSpellCloudly, "затуманивание", "cloudly", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeWater);
//64
	initSpell(kSpellSllence, "молчание", "sielence",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, kTypeMind);
//65
	initSpell(kSpellLight, "свет", "sun shine", 100, 70,
			  2, EPosition::kFight, kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeLight);
//66
	initSpell(kSpellChainLighting, "цепь молний", "chain lightning",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 1, kTypeAir);
//67
	initSpell(kSpellFireBlast, "огненный поток", "fireblast",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 5, kTypeFire);
//68
	initSpell(kSpellGodsWrath, "гнев богов", "gods wrath",
			  140, 120, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, kTypeFire);
//69
	initSpell(kSpellWeaknes, "слабость", "weakness",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 0, kTypeLife);
//70
	initSpell(kSpellGroupInvisible, "групповая невидимость", "group invisible",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, kTypeMind);
//71
	initSpell(kSpellShadowCloak, "мантия теней", "shadow cloak",
			  100, 70, 3, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//72
	initSpell(kSpellAcid, "кислота", "acid", 90, 65, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAlterObjs | kNpcDamagePc, 2, kTypeWater);
//73
	initSpell(kSpellRepair, "починка", "repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, kTypeLight);
//74
	initSpell(kSpellEnlarge, "увеличение", "enlarge",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//75
	initSpell(kSpellFear, "страх", "fear",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagManual | kNpcDamagePc,
			  1, kTypeDark);
//76
	initSpell(kSpellSacrifice, "высосать жизнь", "sacrifice",
			  140, 125, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kNpcDamagePc | kNpcDamagePcMinhp, 10, kTypeDark);
//77
	initSpell(kSpellWeb, "сеть", "web",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, kTypeMind);
//78
	initSpell(kSpellBlink, "мигание", "blink", 70, 55, 2,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//79
	initSpell(kSpellRemoveHold, "снять оцепенение", "remove hold",
			  110, 90, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 1, kTypeLight);
//80
	initSpell(kSpellCamouflage, "!маскировка!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//81
	initSpell(kSpellPowerBlindness, "полная слепота", "power blind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 2, kTypeDark);
//82
	initSpell(kSpellMassBlindness, "массовая слепота", "mass blind",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 4, kTypeDark);
//83
	initSpell(kSpellPowerSilence, "длительное молчание", "power sielence",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, kTypeMind);
//84
	initSpell(kSpellExtraHits, "увеличить жизнь", "extra hits",
			  100, 85, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 1, kTypeLife);
//85
	initSpell(kSpellResurrection, "оживить труп", "ressurection",
			  120, 100, 2, EPosition::kStand, kTarObjRoom, false,
			  kMagSummons, 0, kTypeDark);
//86
	initSpell(kSpellMagicShield, "волшебный щит", "magic shield",
			  50, 30, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLight);

//87
	initSpell(kSpellForbidden, "запечатать комнату", "forbidden",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagRoom, 0, kTypeMind);
//88
	initSpell(kSpellMassSilence, "массовое молчание", "mass sielence", 140, 120,
			  2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 3, kTypeMind);
//89
	initSpell(kSpellRemoveSilence, "снять молчание", "remove sielence",
			  70, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster, 1, kTypeLight);
//90
	initSpell(kSpellDamageLight, "легкий вред", "light damage",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 1, kTypeDark);
//91
	initSpell(kSpellDamageSerious, "серьезный вред", "serious damage",
			  85, 55, 4, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 2, kTypeDark);
//92
	initSpell(kSpellDamageCritic, "критический вред", "critical damage",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 3, kTypeDark);
//93
	initSpell(kSpellMassCurse, "массовое проклятие", "mass curse",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeDark);
//94
	initSpell(kSpellArmageddon, "суд богов", "armageddon",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 10, kTypeAir);
//95
	initSpell(kSpellGroupFly, "групповой полет", "group fly",
			  140, 120, 2, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, kTypeAir);
//96
	initSpell(kSpellGroupBless, "групповая доблесть", "group bless",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//97
	initSpell(kSpellResfresh, "восстановление", "refresh",
			  80, 60, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagPoints, 0, kTypeLife);
//98
	initSpell(kSpellStunning, "каменное проклятие", "stunning",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 15, kTypeEarth);

//99
	initSpell(kSpellHide, "!спрятался!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//100
	initSpell(kSpellSneak, "!крадется!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//101
	initSpell(kSpellDrunked, "!опьянение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//102
	initSpell(kSpellAbstinent, "!абстиненция!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//103
	initSpell(kSpellFullFeed, "насыщение", "full", 70, 55, 1,
			  EPosition::kStand, kTarCharRoom, false, kMagPoints, 10, kTypeLife);
//104
	initSpell(kSpellColdWind, "ледяной ветер", "cold wind", 100, 90,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAffects | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, kTypeWater);
//105
	initSpell(kSpellBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);

//106
	initSpell(kSpellHaemorrhage, "!кровотечение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//107
	initSpell(kSpellCourage, "!ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//108
	initSpell(kSpellWaterbreath, "дышать водой", "waterbreath",
			  85, 70, 4, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeWater);
//109
	initSpell(kSpellSlowdown, "медлительность", "slow",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//110
	initSpell(kSpellHaste, "ускорение", "haste", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//111
	initSpell(kSpellMassSlow, "массовая медлительность", "mass slow",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);
//112
	initSpell(kSpellGroupHaste, "групповое ускорение", "group haste",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//113
	initSpell(kSpellGodsShield, "защита богов", "gods shield",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeLight);
//114
	initSpell(kSpellFever, "лихорадка", "plaque",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 2, kTypeLife);
//115
	initSpell(kSpellCureFever, "вылечить лихорадку", "cure plaque",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 0, kTypeLife);
//116
	initSpell(kSpellAwareness, "внимательность", "awarness",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, kTypeMind);
//117
	initSpell(kSpellReligion, "!молитва или жертва!", "!pray or donate!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//118
	initSpell(kSpellAirShield, "воздушный щит", "air shield",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeAir);
//119
	initSpell(kSpellPortal, "переход", "portal", 200, 180, 4,
			  EPosition::kStand, kTarCharWorld, false, kMagManual, 0, kTypeLight);
//120
	initSpell(kSpellDispellMagic, "развеять магию", "dispel magic",
			  85, 70, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects, 0, kTypeLight);
//121
	initSpell(kSpellSummonKeeper, "защитник", "keeper",
			  100, 80, 2, EPosition::kStand, kTarIgnore,
			  false, kMagSummons, 0, kTypeLight);
//122
	initSpell(kSpellFastRegeneration, "быстрое восстановление",
			  "fast regeneration", 100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//123
	initSpell(kSpellCreateWeapon, "создать оружие", "create weapon",
			  130, 110, 2, EPosition::kStand,
			  kTarIgnore, false, kMagManual, 0, kTypeLight);
//124
	initSpell(kSpellFireShield, "огненный щит", "fire shield",
			  140, 120, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeFire);
//125
	initSpell(kSpellRelocate, "переместиться", "relocate",
			  140, 120, 2, EPosition::kStand, kTarCharWorld, false,
			  kMagManual, 0, kTypeAir);
//126
	initSpell(kSpellSummonFirekeeper, "огненный защитник", "fire keeper",
			  150, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagSummons, 0, kTypeFire);
//127
	initSpell(kSpellIceShield, "ледяной щит", "ice protect",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeWater);
//128
	initSpell(kSpellIceStorm, "ледяной шторм", "ice storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, kTypeWater);
//129
	initSpell(kSpellLessening, "уменьшение", "enless",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects, 0, kTypeLife);
//130
	initSpell(kSpellShineFlash, "яркий блик", "shine flash",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeFire);
//131
	initSpell(kSpellMadness, "безумие", "madness",
			  130, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//132
	initSpell(kSpellGroupMagicGlass, "магическое зеркало", "group magicglass",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 4, kTypeAir);
//133
	initSpell(kSpellCloudOfArrows, "облако стрел", "cloud of arrous",
			  95, 80, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 4, kTypeFire);
//134
	initSpell(kSpellVacuum, "круг пустоты", "vacuum sphere",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 15, kTypeDark);
//135
	initSpell(kSpellMeteorStorm, "метеоритный дождь", "meteor storm",
			  125, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeEarth);
//136
	initSpell(kSpellStoneHands, "каменные руки", "stonehand",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeEarth);
//137
	initSpell(kSpellMindless, "повреждение разума", "mindness",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 0, kTypeMind);
//138
	initSpell(kSpellPrismaticAura, "призматическая аура", "prismatic aura",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//139
	initSpell(kSpellEviless, "силы зла", "eviless",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects | kMagPoints, 3, kTypeDark);
//140
	initSpell(kSpellAirAura, "воздушная аура", "air aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeAir);
//141
	initSpell(kSpellFireAura, "огненная аура", "fire aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeFire);
//142
	initSpell(kSpellIceAura, "ледяная аура", "ice aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeWater);
//143
	initSpell(kSpellShock, "шок", "shock",
			  100, 90, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeDark);
//144
	initSpell(kSpellMagicGlass, "зеркало магии", "magic glassie",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeLight);

//145
	initSpell(kSpellGroupSanctuary, "групповое освящение", "group sanctuary",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);

//146
	initSpell(kSpellGroupPrismaticAura, "групповая призматическая аура",
			  "group prismatic aura", 110, 100, 1, EPosition::kFight,
			  kTarIgnore, false, kMagGroups | kMagAffects | kNpcAffectNpc,
			  1, kTypeLight);

//147
	initSpell(kSpellDeafness, "глухота", "deafness",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, kTypeMind);

//148
	initSpell(kSpellPowerDeafness, "длительная глухота", "power deafness",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, kTypeMind);

//149
	initSpell(kSpellRemoveDeafness, "снять глухоту", "remove deafness",
			  90, 80, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster,
			  1, kTypeLife);

//150
	initSpell(kSpellMassDeafness, "массовая глухота", "mass deafness",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);
//151
	initSpell(kSpellDustStorm, "пылевая буря", "dust storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, kTypeEarth);
//152
	initSpell(kSpellEarthfall, "камнепад", "earth fall",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeEarth);
//153
	initSpell(kSpellSonicWave, "звуковая волна", "sonic wave",
			  120, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeAir);
//154
	initSpell(kSpellHolystrike, "силы света", "holystrike",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagManual | kNpcDamagePc, 10, kTypeLight);
//155
	initSpell(kSpellSumonAngel, "ангел-хранитель", "angel", 150, 130, 5,
			  EPosition::kStand, kTarIgnore, false, kMagManual, 1, kTypeLight);
//156
	initSpell(kSpellMassFear, "массовый страх", "mass fear",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagManual | kNpcAffectPc, 4, kTypeDark);
//157
	initSpell(kSpellFascination, "обаяние", "fascination",
			  480, 440, 20,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeMind);
//158
	initSpell(kSpellCrying, "плач", "crying",
			  120, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//159
	initSpell(kSpellOblivion, "забвение", "oblivion",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeDark);
//160
	initSpell(kSpellBurdenOfTime, "бремя времени", "burden time",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagAreas | kMagAffects | kNpcAffectPc, 4, kTypeDark);
//161
	initSpell(kSpellGroupRefresh, "групповое восстановление", "group refresh",
			  160, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, kTypeLife);

//162
	initSpell(kSpellPeaceful, "смирение", "peaceful",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//163
	initSpell(kSpellMagicBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//164
	initSpell(kSpellBerserk, "!предсмертная ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//165
	initSpell(kSpellStoneBones, "каменные кости", "stone bones",
			  80, 40, 1,
			  EPosition::kStand, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeEarth);

//166 - SPELL_ERoomFlag::kAlwaysLit
	initSpell(kSpellRoomLight, "осветить комнату", "room light",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom, 0, kTypeLight);

//167 - SPELL_POISONED_FOG
	initSpell(kSpellPoosinedFog, "ядовитый туман", "poisoned fog",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, kTypeLife);

//168
	initSpell(kSpellThunderstorm, "буря отмщения", "storm of vengeance",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeAir);
//169
	initSpell(kSpellLightWalk, "!легкая поступь!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false, kMagManual,
			  0, kTypeNeutral);
//170
	initSpell(kSpellFailure, "недоля", "failure",
			  100, 85, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeDark);

//171
	initSpell(kSpellClanPray, "!клановые чары!", "!clan affect!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//172
	initSpell(kSpellGlitterDust, "блестящая пыль", "glitterdust",
			  120, 100, 3, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeEarth);
//173
	initSpell(kSpellScream, "вопль", "scream",
			  100, 85, 3, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeAir);
//174
	initSpell(kSpellCatGrace, "кошачья ловкость", "cats grace",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//175
	initSpell(kSpellBullBody, "бычье тело", "bull body",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//176
	initSpell(kSpellSnakeWisdom, "мудрость змеи", "snake wisdom",
			  60, 50, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//177
	initSpell(kSpellGimmicry, "хитроумие", "gimmickry",
			  60, 50, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
// ДЛЯ КЛИЧЕЙ ПОЛЕ mana_change ИСПОЛЬЗУЕТСЯ
// ДЛЯ УКАЗАНИЯ МИНИМАЛЬНОГО ПРОЦЕНТА СКИЛЛА,
// С КОТОРОГО ДОСТУПЕН УКАЗАННЫЙ КЛИЧ
//178
/*
	initSpell(SPELL_WC_OF_CHALLENGE, "клич вызова", "warcry of challenge", 10, 10, 1,
		   EPosition::kFight, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | NPC_DAMAGE_PC, 0, STYPE_MIND);
//179
	initSpell(SPELL_WC_OF_MENACE, "клич угрозы", "warcry of menace", 30, 30, 40,
		   EPosition::kFight, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//180
	initSpell(SPELL_WC_OF_RAGE, "клич ярости", "warcry of rage", 30, 30, 50,
		   EPosition::kFight, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 0, STYPE_MIND);
//181
	initSpell(SPELL_WC_OF_MADNESS, "клич безумия", "warcry of madness", 50, 50, 91,
		   EPosition::kFight, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_AFFECTS | NPC_AFFECT_PC, 0, STYPE_MIND);
//182
	initSpell(SPELL_WC_OF_THUNDER, "клич грома", "warcry of thunder", 140, 140, 141,
		   EPosition::kFight, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AGGRESSIVE,
		   MAG_WARCRY | MAG_AREAS | MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 0, STYPE_MIND);
*/
//183
	initSpell(kSpellWarcryOfDefence, "клич обороны", "warcry of defense",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//184
	initSpell(kSpellWarcryOfBattle, "клич битвы", "warcry of battle",
			  20, 20, 50, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//185
	initSpell(kSpellWarcryOfPower, "клич мощи", "warcry of power",
			  25, 25, 70, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagPoints | kMagAffects | kNpcDummy | kNpcAffectNpc,
			  0, kTypeMind);
//186
	initSpell(kSpellWarcryOfBless, "клич доблести", "warcry of bless",
			  15, 15, 30, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//187
	initSpell(kSpellWarcryOfCourage, "клич отваги", "warcry of courage",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//188
	initSpell(kSpellRuneLabel, "рунная метка", "rune label",
			  50, 35, 1,
			  EPosition::kStand, kTarRoomThis, false, kMagRoom | kMagCasterInworldDelay,
			  0, kTypeLight);


	// NON-castable spells should appear below here.

// 189
	initSpell(kSpellAconitumPoison, "яд аконита", "aconitum poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 190
	initSpell(kSpellScopolaPoison, "яд скополии", "scopolia poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 191
	initSpell(kSpellBelenaPoison, "яд белены", "belena poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 192
	initSpell(kSpellDaturaPoison, "яд дурмана", "datura poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);

// 193
	initSpell(kSpellTimerRestore, "обновление таймера", " timer repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, kTypeLight);
//194
	initSpell(kSpellLucky, "боевое везение", "lacky",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, kTypeMind);
//195
	initSpell(kSpellBandage, "перевязка", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//196
	initSpell(kSpellNoBandage, "!нельзя перевязываться!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//197
	initSpell(kSpellCapable, "!зачарован!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//198
	initSpell(kSpellStrangle, "!удушье!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//199
	initSpell(kSpellRecallSpells, "!вспоминает заклинания!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//200
	initSpell(kSpellHypnoticPattern, "чарующий узор", "hypnotic pattern",
			  120, 100, 2, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, kTypeMind);
//201
	initSpell(kSpellSolobonus, "награда", "bonus",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarNotSelf,
			  kMtypeNeutral, kMagManual, 1, kTypeNeutral);
//202
	initSpell(kSpellVampirism, "вампиризм", "vampire",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, kTypeDark);
//203
	initSpell(kSpellRestoration, "реконструкция", "reconstruction",
			  110, 100, 1, EPosition::kStand,
			  kTarObjInv | kTarObjEquip, false, kMagAlterObjs, 0, kTypeLight);
//204
	initSpell(kSpellDeathAura, "аура смерти", "aura death",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//205
	initSpell(kSpellRecovery, "темное прикосновение", "recovery",
			  110, 100, 1, EPosition::kStand, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//206
	initSpell(kSpellMassRecovery, "прикосновение смерти", "mass recovery",
			  110, 100, 1, EPosition::kStand, kTarIgnore,
			  false, kMagGroups | kMagPoints | kNpcDummy, 0, kTypeDark);
//207
	initSpell(kSpellAuraOfEvil, "аура зла", "aura evil",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, kTypeDark);
//208
	initSpell(kSpellMentalShadow, "ментальная тень", "mental shadow",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 1, kTypeMind);
//209
	initSpell(kSpellBlackTentacles, "навьи руки", "evards black tentacles",
			  120, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeDark);
//210
	initSpell(kSpellWhirlwind, "вихрь", "whirlwind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeAir);
//211
	initSpell(kSpellIndriksTeeth, "зубы индрика", "indriks teeth",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//212
	initSpell(kSpellAcidArrow, "кислотная стрела", "acid arrow",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeWater);
//213
	initSpell(kSpellThunderStone, "громовой камень", "thunderstone",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//214
	initSpell(kSpellClod, "глыба", "clod", 110, 100, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeEarth);
//215
	initSpell(kSpellExpedient, "!боевой прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//216
	initSpell(kSpellSightOfDarkness, "зрение тьмы", "sight darkness",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//217
	initSpell(kSpellGroupSincerity, "общая искренность", "general sincerity",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//218
	initSpell(kSpellMagicalGaze, "магический взор", "magical gaze",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//219
	initSpell(kSpellAllSeeingEye, "всевидящее око", "allseeing eye",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//220
	initSpell(kSpellEyeOfGods, "око богов", "eye gods",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLife);
//221
	initSpell(kSpellBreathingAtDepth, "дыхание глубин", "breathing at depth",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeWater);
//222
	initSpell(kSpellGeneralRecovery, "общее восстановление", "general recovery",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLife);
//223
	initSpell(kSpellCommonMeal, "общая трапеза", "common meal",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 1, kTypeLife);
//224
	initSpell(kSpellStoneWall, "каменная стена", "stone wall",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeEarth);
//225
	initSpell(kSpellSnakeEyes, "глаза змея", "snake eyes",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//226
	initSpell(kSpellEarthAura, "земной поклон", "earth aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeEarth);
//227
	initSpell(kSpellGroupProtectFromEvil, "групповая защита от тьмы", "group protect evil",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);

//228
	initSpell(kSpellArrowsFire, "стрелы огня", "arrows of fire",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeFire);
//229
	initSpell(kSpellArrowsWater, "стрелы воды", "arrows of water",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeWater);
//230
	initSpell(kSpellArrowsEarth, "стрелы земли", "arrows of earth",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//231
	initSpell(kSpellArrowsAir, "стрелы воздуха", "arrows of air",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeAir);
//232
	initSpell(kSpellArrowsDeath, "стрелы смерти", "arrows of death",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeDark);
//233
	initSpell(kSpellPaladineInspiration, "воодушевление", "inspiration",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagGroups | kMagAffects, 0, kTypeNeutral);
//234
	initSpell(kSpellDexterity, "ловкость", "dexterity",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//235
	initSpell(kSpellGroupBlink, "групповое мигание", "group blink",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeLife);
//236
	initSpell(kSpellGroupCloudly, "групповое затуманивание", "group cloudly",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeWater);
//237
	initSpell(kSpellGroupAwareness, "групповая внимательность", "group awarness",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeMind);
//238
	initSpell(kSpellWatctyOfExpirence, "клич обучения", "warcry of training",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//239
	initSpell(kSpellWarcryOfLuck, "клич везения", "warcry of luck",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//240
	initSpell(kSpellWarcryOfPhysdamage, "клич точности", "warcry of accuracy",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//241
	initSpell(kSpellMassFailure, "взор Велеса", "gaze of Veles",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeDark);
//242
	initSpell(kSpellSnare, "западня", "snare",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);

	/*
	 * These spells are currently not used, not implemented, and not castable.
	 * Values for the 'breath' spells are filled in assuming a dragon's breath.
	 */



//243 - 247
	initSpell(kSpellFireBreath, "огненное дыхание", "fire breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeFire);

	initSpell(kSpellGasBreath, "зловонное дыхание", "gas breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeEarth);

	initSpell(kSpellFrostBreath, "ледяное дыхание", "frost breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeAir);

	initSpell(kSpellAcidBreath, "кислотное дыхание", "acid breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeWater);

	initSpell(kSpellLightingBreath, "ослепляющее дыхание", "lightning breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeDark);
//248
	initSpell(kSpellExpedientFail, "!неудачный прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);

//351
	initSpell(kSpellIdentify, "идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, kTypeMind);
//352
	initSpell(kSpellFullIdentify, "полная идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, kTypeMind);
//353 в dg_affect
	initSpell(kSpellQUest, "!чары!", "!quest spell!",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual,
			  1, kTypeNeutral);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
