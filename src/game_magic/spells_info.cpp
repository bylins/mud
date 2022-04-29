#include "spells_info.h"

#include "spells.h"

const char *unused_spellname = "!UNUSED!";

std::unordered_map<ESpell, SpellInfo> spell_info;
std::unordered_map<ESpell, SpellCreate> spell_create;

void InitUnusedSpell(ESpell spell_id);
void InitSpell(ESpell spell_id, const char *name, const char *syn,
			   int max_mana, int min_mana, int mana_change,
			   int minpos, int targets, int violent, int routines, int danger, int spell_class);

const char *GetSpellName(ESpell spell_id) {
	if (spell_info.contains(spell_id)) {
		return (spell_info[spell_id].name);
	} else {
		return "UNDEFINED";
	}
}

void InitUnusedSpell(ESpell spell_id) {
	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			spell_info[spell_id].min_remort[i][j] = kMaxRemort;
			spell_info[spell_id].min_level[i][j] = kLvlImplementator + 1;
			spell_info[spell_id].slot_forc[i][j] = kMaxMemoryCircle;
			spell_info[spell_id].class_change[i][j] = 0;
		}
	}

	for (i = 0; i < 3; i++) {
		spell_create[spell_id].wand.items[i] = -1;
		spell_create[spell_id].scroll.items[i] = -1;
		spell_create[spell_id].potion.items[i] = -1;
		spell_create[spell_id].items.items[i] = -1;
		spell_create[spell_id].runes.items[i] = -1;
	}

	spell_create[spell_id].wand.rnumber = -1;
	spell_create[spell_id].scroll.rnumber = -1;
	spell_create[spell_id].potion.rnumber = -1;
	spell_create[spell_id].items.rnumber = -1;
	spell_create[spell_id].runes.rnumber = -1;

	spell_info[spell_id].mana_max = 0;
	spell_info[spell_id].mana_min = 0;
	spell_info[spell_id].mana_change = 0;
	spell_info[spell_id].min_position = EPosition::kDead;
	spell_info[spell_id].danger = 0;
	spell_info[spell_id].targets = 0;
	spell_info[spell_id].violent = 0;
	spell_info[spell_id].routines = 0;
	spell_info[spell_id].name = unused_spellname;
	spell_info[spell_id].syn = unused_spellname;
}

void InitSpell(ESpell spell_id, const char *name, const char *syn,
			   int max_mana, int min_mana, int mana_change,
			   EPosition minpos, int targets, int violent, int routines, int danger, int spell_class) {

	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			spell_info[spell_id].min_remort[i][j] = kMaxRemort;
			spell_info[spell_id].min_level[i][j] = kLvlImplementator;
			spell_info[spell_id].slot_forc[i][j] = kMaxMemoryCircle;
			spell_info[spell_id].class_change[i][j] = 0;
		}
	}

	spell_create[spell_id].wand.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].scroll.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].potion.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].items.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].runes.min_caster_level = kLvlGreatGod;

	spell_info[spell_id].mana_max = max_mana;
	spell_info[spell_id].mana_min = min_mana;
	spell_info[spell_id].mana_change = mana_change;
	spell_info[spell_id].min_position = minpos;
	spell_info[spell_id].danger = danger;
	spell_info[spell_id].targets = targets;
	spell_info[spell_id].violent = violent;
	spell_info[spell_id].routines = routines;
	spell_info[spell_id].name = name;
	spell_info[spell_id].syn = syn;
	spell_info[spell_id].spell_class = spell_class;
}

/*
 * Arguments for initSpell calls:
 *
 * spell_id, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spell_id:  Number of the spell.  Usually the symbolic name as defined in
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

	for (auto spell_id = ESpell::kSpellFirst ; spell_id <= ESpell::kSpellLast; ++spell_id) {
		InitUnusedSpell(spell_id);
	}


//1
	InitSpell(kSpellArmor, "защита", "armor", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeLight);
//2
	InitSpell(kSpellTeleport, "прыжок", "teleport",
			  140, 120, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagManual | kNpcDamagePc, 1, kTypeAir);
//3
	InitSpell(kSpellBless, "доблесть", "bless", 55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs | kNpcAffectNpc, 0, kTypeLight);
//4
	InitSpell(kSpellBlindness, "слепота", "blind",
			  70, 40, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, kTypeDark);
//5
	InitSpell(kSpellBurningHands, "горящие руки", "burning hands",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagAreas | kMagDamage | kNpcDamagePc, 1, kTypeFire);
//6
	InitSpell(kSpellCallLighting, "шаровая молния", "call lightning", 85, 70,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kNpcAffectPc | kMagAffects | kMagDamage | kNpcDamagePc, 2, kTypeAir);
//7
	InitSpell(kSpellCharm, "подчинить разум", "mind control",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual, 1, kTypeMind);
//8
	InitSpell(kSpellChillTouch, "ледяное прикосновение", "chill touch",
			  55, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAffects | kNpcDamagePc, 1, kTypeWater);
//9
	InitSpell(kSpellClone, "клонирование", "clone",
			  150, 130, 5, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagSummons, 0, kTypeDark);
//10
	InitSpell(kSpellIceBolts, "ледяные стрелы", "ice bolts", 90, 75,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 3, kTypeWater);
//11
	InitSpell(kSpellControlWeather, "контроль погоды", "weather control",
			  100, 90, 1, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 0, kTypeAir);
//12
	InitSpell(kSpellCreateFood, "создать пищу", "create food",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, kTypeLife);
//13
	InitSpell(kSpellCreateWater, "создать воду", "create water", 40, 30,
			  1, EPosition::kStand, kTarObjInv | kTarObjEquip | kTarCharRoom, false,
			  kMagManual, 0, kTypeWater);
//14
	InitSpell(kSpellCureBlind, "вылечить слепоту", "cure blind", 110, 90,
			  2, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc, 0, kTypeLight);
//15
	InitSpell(kSpellCureCritic, "критическое исцеление", "critical cure",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 3, kTypeLife);
//16
	InitSpell(kSpellCureLight, "легкое исцеление", "light cure", 40, 30,
			  1, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagPoints | kNpcDummy, 1, kTypeLife);
//17
	InitSpell(kSpellCurse, "проклятие", "curse",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict | kTarObjInv, kMtypeNeutral,
			  kMagAffects | kMagAlterObjs | kNpcAffectPc, 1, kTypeDark);
//18
	InitSpell(kSpellDetectAlign, "определение наклонностей", "detect alignment",
			  40, 30, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//19
	InitSpell(kSpellDetectInvis, "видеть невидимое", "detect invisible",
			  100, 55, 3, EPosition::kFight, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//20
	InitSpell(kSpellDetectMagic, "определение магии", "detect magic",
			  100, 55, 3, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//21
	InitSpell(kSpellDetectPoison, "определение яда", "detect poison",
			  40, 30, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, kTypeMind);
//22
	InitSpell(kSpellDispelEvil, "изгнать зло", "dispel evil",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, kTypeLight);
//23
	InitSpell(kSpellEarthquake, "землетрясение", "earthquake",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 2, kTypeEarth);
//24
	InitSpell(kSpellEnchantWeapon, "заколдовать оружие", "enchant weapon", 140,
			  110, 2, EPosition::kStand, kTarObjInv, false, kMagAlterObjs,
			  0, kTypeLight);
//25
	InitSpell(kSpellEnergyDrain, "истощить энергию", "energy drain",
			  150, 140, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kMagAffects | kNpcDamagePc, 1, kTypeDark);
//26
	InitSpell(kSpellFireball, "огненный шар", "fireball",
			  110, 100, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeFire);
//27
	InitSpell(kSpellHarm, "вред", "harm",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 5, kTypeDark);
//28
	InitSpell(kSpellHeal, "исцеление", "heal",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 10, kTypeLife);
//29
	InitSpell(kSpellInvisible, "невидимость", "invisible",
			  50, 40, 3, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjRoom, false,
			  kMagAffects | kMagAlterObjs, 0, kTypeMind);
//30
	InitSpell(kSpellLightingBolt, "молния", "lightning bolt",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc, 1, kTypeAir);
//31
	InitSpell(kSpellLocateObject, "разыскать предмет", "locate object",
			  140, 110, 2, EPosition::kStand, kTarObjWorld,
			  false, kMagManual, 0, kTypeMind);
//32
	InitSpell(kSpellMagicMissile, "магическая стрела", "magic missle",
			  40, 30, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kNpcDamagePc, 1, kTypeFire);
//33
	InitSpell(kSpellPoison, "яд", "poison", 70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarObjInv | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kMagAlterObjs | kNpcAffectPc, 2, kTypeLife);
//34
	InitSpell(kSpellProtectFromEvil, "защита от тьмы", "protect evil",
			  60, 45, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//35
	InitSpell(kSpellRemoveCurse, "снять проклятие", "remove curse",
			  50, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, kTypeLight);
//36
	InitSpell(kSpellSanctuary, "освящение", "sanctuary",
			  85, 70, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//37
	InitSpell(kSpellShockingGasp, "обжигающая хватка", "shocking grasp", 50, 40,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc, 1, kTypeFire);
//38
	InitSpell(kSpellSleep, "сон", "sleep",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagAffects | kNpcAffectPc,
			  0, kTypeMind);
//39
	InitSpell(kSpellStrength, "сила", "strength", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeLife);
//40
	InitSpell(kSpellSummon, "призвать", "summon",
			  110, 100, 2, EPosition::kStand, kTarCharWorld | kTarNotSelf,
			  false, kMagManual, 0, kTypeMind);
//41
	InitSpell(kSpellPatronage, "покровительство", "patronage",
			  85, 70, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagPoints | kMagAffects, 1, kTypeLight);
//42
	InitSpell(kSpellWorldOfRecall, "слово возврата", "recall",
			  140, 100, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagManual | kNpcDamagePc, 0, kTypeMind);
//43
	InitSpell(kSpellRemovePoison, "удалить яд", "remove poison",
			  60, 45, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjRoom, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, kTypeLife);
//44
	InitSpell(kSpellSenseLife, "определение жизни", "sense life",
			  85, 70, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, kTypeLife);
//45
	InitSpell(kSpellAnimateDead, "поднять труп", "animate dead",
			  50, 35, 3, EPosition::kStand, kTarObjRoom,
			  false, kMagSummons, 0, kTypeDark);
//46
	InitSpell(kSpellDispelGood, "рассеять свет", "dispel good",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, kTypeDark);
//47
	InitSpell(kSpellGroupArmor, "групповая защита", "group armor",
			  110, 100, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//48
	InitSpell(kSpellGroupHeal, "групповое исцеление", "group heal",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, kTypeLife);
//49
	InitSpell(kSpellGroupRecall, "групповой возврат", "group recall",
			  125, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagManual, 0, kTypeMind);
//50
	InitSpell(kSpellInfravision, "видение ночью", "infravision",
			  50, 40, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeLight);
//51
	InitSpell(kSpellWaterwalk, "водохождение", "waterwalk",
			  70, 55, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeWater);
//52
	InitSpell(kSpellCureSerious, "серьезное исцеление", "serious cure",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 2, kTypeLife);
//53
	InitSpell(kSpellGroupStrength, "групповая сила", "group strength",
			  140, 120, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//54
	InitSpell(kSpellHold, "оцепенение", "hold",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 3, kTypeMind);
//55
	InitSpell(kSpellPowerHold, "длительное оцепенение", "power hold",
			  140, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 4, kTypeMind);
//56
	InitSpell(kSpellMassHold, "массовое оцепенение", "mass hold",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeMind);
//57
	InitSpell(kSpellFly, "полет", "fly", 50, 35, 1, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeAir);
//58
	InitSpell(kSpellBrokenChains, "разбитые оковы", "broken chains",
			  125, 110, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 5, kTypeMind);
//59
	InitSpell(kSpellNoflee, "приковать противника", "noflee",
			  100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 0, kTypeMind);
//60
	InitSpell(kSpellCreateLight, "создать свет", "create light",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, kTypeLight);
//61
	InitSpell(kSpellDarkness, "тьма", "darkness",
			  100, 70, 2, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeDark);
//62
	InitSpell(kSpellStoneSkin, "каменная кожа", "stoneskin", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeEarth);
//63
	InitSpell(kSpellCloudly, "затуманивание", "cloudly", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeWater);
//64
	InitSpell(kSpellSllence, "молчание", "sielence",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, kTypeMind);
//65
	InitSpell(kSpellLight, "свет", "sun shine", 100, 70,
			  2, EPosition::kFight, kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeLight);
//66
	InitSpell(kSpellChainLighting, "цепь молний", "chain lightning",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 1, kTypeAir);
//67
	InitSpell(kSpellFireBlast, "огненный поток", "fireblast",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 5, kTypeFire);
//68
	InitSpell(kSpellGodsWrath, "гнев богов", "gods wrath",
			  140, 120, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, kTypeFire);
//69
	InitSpell(kSpellWeaknes, "слабость", "weakness",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 0, kTypeLife);
//70
	InitSpell(kSpellGroupInvisible, "групповая невидимость", "group invisible",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, kTypeMind);
//71
	InitSpell(kSpellShadowCloak, "мантия теней", "shadow cloak",
			  100, 70, 3, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//72
	InitSpell(kSpellAcid, "кислота", "acid", 90, 65, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAlterObjs | kNpcDamagePc, 2, kTypeWater);
//73
	InitSpell(kSpellRepair, "починка", "repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, kTypeLight);
//74
	InitSpell(kSpellEnlarge, "увеличение", "enlarge",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//75
	InitSpell(kSpellFear, "страх", "fear",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagManual | kNpcDamagePc,
			  1, kTypeDark);
//76
	InitSpell(kSpellSacrifice, "высосать жизнь", "sacrifice",
			  140, 125, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kNpcDamagePc | kNpcDamagePcMinhp, 10, kTypeDark);
//77
	InitSpell(kSpellWeb, "сеть", "web",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, kTypeMind);
//78
	InitSpell(kSpellBlink, "мигание", "blink", 70, 55, 2,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//79
	InitSpell(kSpellRemoveHold, "снять оцепенение", "remove hold",
			  110, 90, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 1, kTypeLight);
//80
	InitSpell(kSpellCamouflage, "!маскировка!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//81
	InitSpell(kSpellPowerBlindness, "полная слепота", "power blind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 2, kTypeDark);
//82
	InitSpell(kSpellMassBlindness, "массовая слепота", "mass blind",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 4, kTypeDark);
//83
	InitSpell(kSpellPowerSilence, "длительное молчание", "power sielence",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, kTypeMind);
//84
	InitSpell(kSpellExtraHits, "увеличить жизнь", "extra hits",
			  100, 85, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 1, kTypeLife);
//85
	InitSpell(kSpellResurrection, "оживить труп", "ressurection",
			  120, 100, 2, EPosition::kStand, kTarObjRoom, false,
			  kMagSummons, 0, kTypeDark);
//86
	InitSpell(kSpellMagicShield, "волшебный щит", "magic shield",
			  50, 30, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLight);

//87
	InitSpell(kSpellForbidden, "запечатать комнату", "forbidden",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagRoom, 0, kTypeMind);
//88
	InitSpell(kSpellMassSilence, "массовое молчание", "mass sielence", 140, 120,
			  2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 3, kTypeMind);
//89
	InitSpell(kSpellRemoveSilence, "снять молчание", "remove sielence",
			  70, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster, 1, kTypeLight);
//90
	InitSpell(kSpellDamageLight, "легкий вред", "light damage",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 1, kTypeDark);
//91
	InitSpell(kSpellDamageSerious, "серьезный вред", "serious damage",
			  85, 55, 4, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 2, kTypeDark);
//92
	InitSpell(kSpellDamageCritic, "критический вред", "critical damage",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 3, kTypeDark);
//93
	InitSpell(kSpellMassCurse, "массовое проклятие", "mass curse",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeDark);
//94
	InitSpell(kSpellArmageddon, "суд богов", "armageddon",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 10, kTypeAir);
//95
	InitSpell(kSpellGroupFly, "групповой полет", "group fly",
			  140, 120, 2, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, kTypeAir);
//96
	InitSpell(kSpellGroupBless, "групповая доблесть", "group bless",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//97
	InitSpell(kSpellResfresh, "восстановление", "refresh",
			  80, 60, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagPoints, 0, kTypeLife);
//98
	InitSpell(kSpellStunning, "каменное проклятие", "stunning",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 15, kTypeEarth);

//99
	InitSpell(kSpellHide, "!спрятался!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//100
	InitSpell(kSpellSneak, "!крадется!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//101
	InitSpell(kSpellDrunked, "!опьянение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//102
	InitSpell(kSpellAbstinent, "!абстиненция!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//103
	InitSpell(kSpellFullFeed, "насыщение", "full", 70, 55, 1,
			  EPosition::kStand, kTarCharRoom, false, kMagPoints, 10, kTypeLife);
//104
	InitSpell(kSpellColdWind, "ледяной ветер", "cold wind", 100, 90,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAffects | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, kTypeWater);
//105
	InitSpell(kSpellBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);

//106
	InitSpell(kSpellHaemorrhage, "!кровотечение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//107
	InitSpell(kSpellCourage, "!ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//108
	InitSpell(kSpellWaterbreath, "дышать водой", "waterbreath",
			  85, 70, 4, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeWater);
//109
	InitSpell(kSpellSlowdown, "медлительность", "slow",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//110
	InitSpell(kSpellHaste, "ускорение", "haste", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//111
	InitSpell(kSpellMassSlow, "массовая медлительность", "mass slow",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);
//112
	InitSpell(kSpellGroupHaste, "групповое ускорение", "group haste",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//113
	InitSpell(kSpellGodsShield, "защита богов", "gods shield",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeLight);
//114
	InitSpell(kSpellFever, "лихорадка", "plaque",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 2, kTypeLife);
//115
	InitSpell(kSpellCureFever, "вылечить лихорадку", "cure plaque",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 0, kTypeLife);
//116
	InitSpell(kSpellAwareness, "внимательность", "awarness",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, kTypeMind);
//117
	InitSpell(kSpellReligion, "!молитва или жертва!", "!pray or donate!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//118
	InitSpell(kSpellAirShield, "воздушный щит", "air shield",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeAir);
//119
	InitSpell(kSpellPortal, "переход", "portal", 200, 180, 4,
			  EPosition::kStand, kTarCharWorld, false, kMagManual, 0, kTypeLight);
//120
	InitSpell(kSpellDispellMagic, "развеять магию", "dispel magic",
			  85, 70, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects, 0, kTypeLight);
//121
	InitSpell(kSpellSummonKeeper, "защитник", "keeper",
			  100, 80, 2, EPosition::kStand, kTarIgnore,
			  false, kMagSummons, 0, kTypeLight);
//122
	InitSpell(kSpellFastRegeneration, "быстрое восстановление",
			  "fast regeneration", 100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//123
	InitSpell(kSpellCreateWeapon, "создать оружие", "create weapon",
			  130, 110, 2, EPosition::kStand,
			  kTarIgnore, false, kMagManual, 0, kTypeLight);
//124
	InitSpell(kSpellFireShield, "огненный щит", "fire shield",
			  140, 120, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeFire);
//125
	InitSpell(kSpellRelocate, "переместиться", "relocate",
			  140, 120, 2, EPosition::kStand, kTarCharWorld, false,
			  kMagManual, 0, kTypeAir);
//126
	InitSpell(kSpellSummonFirekeeper, "огненный защитник", "fire keeper",
			  150, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagSummons, 0, kTypeFire);
//127
	InitSpell(kSpellIceShield, "ледяной щит", "ice protect",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeWater);
//128
	InitSpell(kSpellIceStorm, "ледяной шторм", "ice storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, kTypeWater);
//129
	InitSpell(kSpellLessening, "уменьшение", "enless",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects, 0, kTypeLife);
//130
	InitSpell(kSpellShineFlash, "яркий блик", "shine flash",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeFire);
//131
	InitSpell(kSpellMadness, "безумие", "madness",
			  130, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//132
	InitSpell(kSpellGroupMagicGlass, "магическое зеркало", "group magicglass",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 4, kTypeAir);
//133
	InitSpell(kSpellCloudOfArrows, "облако стрел", "cloud of arrous",
			  95, 80, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 4, kTypeFire);
//134
	InitSpell(kSpellVacuum, "круг пустоты", "vacuum sphere",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 15, kTypeDark);
//135
	InitSpell(kSpellMeteorStorm, "метеоритный дождь", "meteor storm",
			  125, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeEarth);
//136
	InitSpell(kSpellStoneHands, "каменные руки", "stonehand",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeEarth);
//137
	InitSpell(kSpellMindless, "повреждение разума", "mindness",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 0, kTypeMind);
//138
	InitSpell(kSpellPrismaticAura, "призматическая аура", "prismatic aura",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//139
	InitSpell(kSpellEviless, "силы зла", "eviless",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects | kMagPoints, 3, kTypeDark);
//140
	InitSpell(kSpellAirAura, "воздушная аура", "air aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeAir);
//141
	InitSpell(kSpellFireAura, "огненная аура", "fire aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeFire);
//142
	InitSpell(kSpellIceAura, "ледяная аура", "ice aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeWater);
//143
	InitSpell(kSpellShock, "шок", "shock",
			  100, 90, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeDark);
//144
	InitSpell(kSpellMagicGlass, "зеркало магии", "magic glassie",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeLight);

//145
	InitSpell(kSpellGroupSanctuary, "групповое освящение", "group sanctuary",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);

//146
	InitSpell(kSpellGroupPrismaticAura, "групповая призматическая аура",
			  "group prismatic aura", 110, 100, 1, EPosition::kFight,
			  kTarIgnore, false, kMagGroups | kMagAffects | kNpcAffectNpc,
			  1, kTypeLight);

//147
	InitSpell(kSpellDeafness, "глухота", "deafness",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, kTypeMind);

//148
	InitSpell(kSpellPowerDeafness, "длительная глухота", "power deafness",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, kTypeMind);

//149
	InitSpell(kSpellRemoveDeafness, "снять глухоту", "remove deafness",
			  90, 80, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster,
			  1, kTypeLife);

//150
	InitSpell(kSpellMassDeafness, "массовая глухота", "mass deafness",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);
//151
	InitSpell(kSpellDustStorm, "пылевая буря", "dust storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, kTypeEarth);
//152
	InitSpell(kSpellEarthfall, "камнепад", "earth fall",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeEarth);
//153
	InitSpell(kSpellSonicWave, "звуковая волна", "sonic wave",
			  120, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeAir);
//154
	InitSpell(kSpellHolystrike, "силы света", "holystrike",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagManual | kNpcDamagePc, 10, kTypeLight);
//155
	InitSpell(kSpellSumonAngel, "ангел-хранитель", "angel", 150, 130, 5,
			  EPosition::kStand, kTarIgnore, false, kMagManual, 1, kTypeLight);
//156
	InitSpell(kSpellMassFear, "массовый страх", "mass fear",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagManual | kNpcAffectPc, 4, kTypeDark);
//157
	InitSpell(kSpellFascination, "обаяние", "fascination",
			  480, 440, 20,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeMind);
//158
	InitSpell(kSpellCrying, "плач", "crying",
			  120, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//159
	InitSpell(kSpellOblivion, "забвение", "oblivion",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeDark);
//160
	InitSpell(kSpellBurdenOfTime, "бремя времени", "burden time",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagAreas | kMagAffects | kNpcAffectPc, 4, kTypeDark);
//161
	InitSpell(kSpellGroupRefresh, "групповое восстановление", "group refresh",
			  160, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, kTypeLife);

//162
	InitSpell(kSpellPeaceful, "смирение", "peaceful",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//163
	InitSpell(kSpellMagicBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//164
	InitSpell(kSpellBerserk, "!предсмертная ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//165
	InitSpell(kSpellStoneBones, "каменные кости", "stone bones",
			  80, 40, 1,
			  EPosition::kStand, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeEarth);

//166 - SPELL_ERoomFlag::kAlwaysLit
	InitSpell(kSpellRoomLight, "осветить комнату", "room light",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom, 0, kTypeLight);

//167 - SPELL_POISONED_FOG
	InitSpell(kSpellPoosinedFog, "ядовитый туман", "poisoned fog",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, kTypeLife);

//168
	InitSpell(kSpellThunderstorm, "буря отмщения", "storm of vengeance",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeAir);
//169
	InitSpell(kSpellLightWalk, "!легкая поступь!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false, kMagManual,
			  0, kTypeNeutral);
//170
	InitSpell(kSpellFailure, "недоля", "failure",
			  100, 85, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeDark);

//171
	InitSpell(kSpellClanPray, "!клановые чары!", "!clan affect!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//172
	InitSpell(kSpellGlitterDust, "блестящая пыль", "glitterdust",
			  120, 100, 3, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeEarth);
//173
	InitSpell(kSpellScream, "вопль", "scream",
			  100, 85, 3, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeAir);
//174
	InitSpell(kSpellCatGrace, "кошачья ловкость", "cats grace",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//175
	InitSpell(kSpellBullBody, "бычье тело", "bull body",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//176
	InitSpell(kSpellSnakeWisdom, "мудрость змеи", "snake wisdom",
			  60, 50, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//177
	InitSpell(kSpellGimmicry, "хитроумие", "gimmickry",
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
	InitSpell(kSpellWarcryOfDefence, "клич обороны", "warcry of defense",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//184
	InitSpell(kSpellWarcryOfBattle, "клич битвы", "warcry of battle",
			  20, 20, 50, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//185
	InitSpell(kSpellWarcryOfPower, "клич мощи", "warcry of power",
			  25, 25, 70, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagPoints | kMagAffects | kNpcDummy | kNpcAffectNpc,
			  0, kTypeMind);
//186
	InitSpell(kSpellWarcryOfBless, "клич доблести", "warcry of bless",
			  15, 15, 30, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//187
	InitSpell(kSpellWarcryOfCourage, "клич отваги", "warcry of courage",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//188
	InitSpell(kSpellRuneLabel, "рунная метка", "rune label",
			  50, 35, 1,
			  EPosition::kStand, kTarRoomThis, false, kMagRoom | kMagCasterInworldDelay,
			  0, kTypeLight);


	// NON-castable spells should appear below here.

// 189
	InitSpell(kSpellAconitumPoison, "яд аконита", "aconitum poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 190
	InitSpell(kSpellScopolaPoison, "яд скополии", "scopolia poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 191
	InitSpell(kSpellBelenaPoison, "яд белены", "belena poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 192
	InitSpell(kSpellDaturaPoison, "яд дурмана", "datura poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);

// 193
	InitSpell(kSpellTimerRestore, "обновление таймера", " timer repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, kTypeLight);
//194
	InitSpell(kSpellLucky, "боевое везение", "lacky",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, kTypeMind);
//195
	InitSpell(kSpellBandage, "перевязка", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//196
	InitSpell(kSpellNoBandage, "!нельзя перевязываться!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//197
	InitSpell(kSpellCapable, "!зачарован!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//198
	InitSpell(kSpellStrangle, "!удушье!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//199
	InitSpell(kSpellRecallSpells, "!вспоминает заклинания!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//200
	InitSpell(kSpellHypnoticPattern, "чарующий узор", "hypnotic pattern",
			  120, 100, 2, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, kTypeMind);
//201
	InitSpell(kSpellSolobonus, "награда", "bonus",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarNotSelf,
			  kMtypeNeutral, kMagManual, 1, kTypeNeutral);
//202
	InitSpell(kSpellVampirism, "вампиризм", "vampire",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, kTypeDark);
//203
	InitSpell(kSpellRestoration, "реконструкция", "reconstruction",
			  110, 100, 1, EPosition::kStand,
			  kTarObjInv | kTarObjEquip, false, kMagAlterObjs, 0, kTypeLight);
//204
	InitSpell(kSpellDeathAura, "аура смерти", "aura death",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//205
	InitSpell(kSpellRecovery, "темное прикосновение", "recovery",
			  110, 100, 1, EPosition::kStand, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//206
	InitSpell(kSpellMassRecovery, "прикосновение смерти", "mass recovery",
			  110, 100, 1, EPosition::kStand, kTarIgnore,
			  false, kMagGroups | kMagPoints | kNpcDummy, 0, kTypeDark);
//207
	InitSpell(kSpellAuraOfEvil, "аура зла", "aura evil",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, kTypeDark);
//208
	InitSpell(kSpellMentalShadow, "ментальная тень", "mental shadow",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 1, kTypeMind);
//209
	InitSpell(kSpellBlackTentacles, "навьи руки", "evards black tentacles",
			  120, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeDark);
//210
	InitSpell(kSpellWhirlwind, "вихрь", "whirlwind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeAir);
//211
	InitSpell(kSpellIndriksTeeth, "зубы индрика", "indriks teeth",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//212
	InitSpell(kSpellAcidArrow, "кислотная стрела", "acid arrow",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeWater);
//213
	InitSpell(kSpellThunderStone, "громовой камень", "thunderstone",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//214
	InitSpell(kSpellClod, "глыба", "clod", 110, 100, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeEarth);
//215
	InitSpell(kSpellExpedient, "!боевой прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//216
	InitSpell(kSpellSightOfDarkness, "зрение тьмы", "sight darkness",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//217
	InitSpell(kSpellGroupSincerity, "общая искренность", "general sincerity",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//218
	InitSpell(kSpellMagicalGaze, "магический взор", "magical gaze",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//219
	InitSpell(kSpellAllSeeingEye, "всевидящее око", "allseeing eye",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//220
	InitSpell(kSpellEyeOfGods, "око богов", "eye gods",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLife);
//221
	InitSpell(kSpellBreathingAtDepth, "дыхание глубин", "breathing at depth",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeWater);
//222
	InitSpell(kSpellGeneralRecovery, "общее восстановление", "general recovery",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLife);
//223
	InitSpell(kSpellCommonMeal, "общая трапеза", "common meal",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 1, kTypeLife);
//224
	InitSpell(kSpellStoneWall, "каменная стена", "stone wall",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeEarth);
//225
	InitSpell(kSpellSnakeEyes, "глаза змея", "snake eyes",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//226
	InitSpell(kSpellEarthAura, "земной поклон", "earth aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeEarth);
//227
	InitSpell(kSpellGroupProtectFromEvil, "групповая защита от тьмы", "group protect evil",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);

//228
	InitSpell(kSpellArrowsFire, "стрелы огня", "arrows of fire",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeFire);
//229
	InitSpell(kSpellArrowsWater, "стрелы воды", "arrows of water",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeWater);
//230
	InitSpell(kSpellArrowsEarth, "стрелы земли", "arrows of earth",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//231
	InitSpell(kSpellArrowsAir, "стрелы воздуха", "arrows of air",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeAir);
//232
	InitSpell(kSpellArrowsDeath, "стрелы смерти", "arrows of death",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeDark);
//233
	InitSpell(kSpellPaladineInspiration, "воодушевление", "inspiration",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagGroups | kMagAffects, 0, kTypeNeutral);
//234
	InitSpell(kSpellDexterity, "ловкость", "dexterity",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//235
	InitSpell(kSpellGroupBlink, "групповое мигание", "group blink",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeLife);
//236
	InitSpell(kSpellGroupCloudly, "групповое затуманивание", "group cloudly",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeWater);
//237
	InitSpell(kSpellGroupAwareness, "групповая внимательность", "group awarness",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeMind);
//238
	InitSpell(kSpellWatctyOfExpirence, "клич обучения", "warcry of training",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//239
	InitSpell(kSpellWarcryOfLuck, "клич везения", "warcry of luck",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//240
	InitSpell(kSpellWarcryOfPhysdamage, "клич точности", "warcry of accuracy",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//241
	InitSpell(kSpellMassFailure, "взор Велеса", "gaze of Veles",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeDark);
//242
	InitSpell(kSpellSnare, "западня", "snare",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);

	/*
	 * These spells are currently not used, not implemented, and not castable.
	 * Values for the 'breath' spells are filled in assuming a dragon's breath.
	 */



//243 - 247
	InitSpell(kSpellFireBreath, "огненное дыхание", "fire breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeFire);

	InitSpell(kSpellGasBreath, "зловонное дыхание", "gas breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeEarth);

	InitSpell(kSpellFrostBreath, "ледяное дыхание", "frost breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeAir);

	InitSpell(kSpellAcidBreath, "кислотное дыхание", "acid breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeWater);

	InitSpell(kSpellLightingBreath, "ослепляющее дыхание", "lightning breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeDark);
//248
	InitSpell(kSpellExpedientFail, "!неудачный прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);

//351
	InitSpell(kSpellIdentify, "идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, kTypeMind);
//352
	InitSpell(kSpellFullIdentify, "полная идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, kTypeMind);
//353 в dg_affect
	InitSpell(kSpellQUest, "!чары!", "!quest spell!",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual,
			  1, kTypeNeutral);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
