#include "spells_info.h"

#include "spells.h"

const char *unused_spellname = "!UNUSED!";

std::unordered_map<ESpell, SpellInfo> spell_info;
std::unordered_map<ESpell, SpellCreate> spell_create;

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

	for (auto spell_id = ESpell::kUndefined; spell_id <= ESpell::kLast; ++spell_id) {
		InitUnusedSpell(spell_id);
	}


//1
	InitSpell(ESpell::kArmor, "защита", "armor", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//2
	InitSpell(ESpell::kTeleport, "прыжок", "teleport",
			  140, 120, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagManual | kNpcDamagePc, 1, kTypeAir);
//3
	InitSpell(ESpell::kBless, "доблесть", "bless", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip, false,
			  kMagAffects | kMagAlterObjs | kNpcAffectNpc, 0, kTypeLight);
//4
	InitSpell(ESpell::kBlindness, "слепота", "blind",
			  70, 40, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, kTypeDark);
//5
	InitSpell(ESpell::kBurningHands, "горящие руки", "burning hands",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagAreas | kMagDamage | kNpcDamagePc, 1, kTypeFire);
//6
	InitSpell(ESpell::kCallLighting, "шаровая молния", "call lightning", 85, 70,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kNpcAffectPc | kMagAffects | kMagDamage | kNpcDamagePc, 2, kTypeAir);
//7
	InitSpell(ESpell::kCharm, "подчинить разум", "mind control",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual, 1, kTypeMind);
//8
	InitSpell(ESpell::kChillTouch, "ледяное прикосновение", "chill touch",
			  55, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAffects | kNpcDamagePc, 1, kTypeWater);
//9
	InitSpell(ESpell::kClone, "клонирование", "clone",
			  150, 130, 5, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagSummons, 0, kTypeDark);
//10
	InitSpell(ESpell::kIceBolts, "ледяные стрелы", "ice bolts", 90, 75,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 3, kTypeWater);
//11
	InitSpell(ESpell::kControlWeather, "контроль погоды", "weather control",
			  100, 90, 1, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 0, kTypeAir);
//12
	InitSpell(ESpell::kCreateFood, "создать пищу", "create food",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, kTypeLife);
//13
	InitSpell(ESpell::kCreateWater, "создать воду", "create water", 40, 30,
			  1, EPosition::kStand, kTarObjInv | kTarObjEquip | kTarCharRoom, false,
			  kMagManual, 0, kTypeWater);
//14
	InitSpell(ESpell::kCureBlind, "вылечить слепоту", "cure blind", 110, 90,
			  2, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc, 0, kTypeLight);
//15
	InitSpell(ESpell::kCureCritic, "критическое исцеление", "critical cure",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 3, kTypeLife);
//16
	InitSpell(ESpell::kCureLight, "легкое исцеление", "light cure", 40, 30,
			  1, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagPoints | kNpcDummy, 1, kTypeLife);
//17
	InitSpell(ESpell::kCurse, "проклятие", "curse",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict | kTarObjInv, kMtypeNeutral,
			  kMagAffects | kMagAlterObjs | kNpcAffectPc, 1, kTypeDark);
//18
	InitSpell(ESpell::kDetectAlign, "определение наклонностей", "detect alignment",
			  40, 30, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//19
	InitSpell(ESpell::kDetectInvis, "видеть невидимое", "detect invisible",
			  100, 55, 3, EPosition::kFight, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//20
	InitSpell(ESpell::kDetectMagic, "определение магии", "detect magic",
			  100, 55, 3, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeMind);
//21
	InitSpell(ESpell::kDetectPoison, "определение яда", "detect poison",
			  40, 30, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, kTypeMind);
//22
	InitSpell(ESpell::kDispelEvil, "изгнать зло", "dispel evil",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, kTypeLight);
//23
	InitSpell(ESpell::kEarthquake, "землетрясение", "earthquake",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 2, kTypeEarth);
//24
	InitSpell(ESpell::kEnchantWeapon, "заколдовать оружие", "enchant weapon", 140,
			  110, 2, EPosition::kStand, kTarObjInv, false, kMagAlterObjs,
			  0, kTypeLight);
//25
	InitSpell(ESpell::kEnergyDrain, "истощить энергию", "energy drain",
			  150, 140, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kMagAffects | kNpcDamagePc, 1, kTypeDark);
//26
	InitSpell(ESpell::kFireball, "огненный шар", "fireball",
			  110, 100, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeFire);
//27
	InitSpell(ESpell::kHarm, "вред", "harm",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 5, kTypeDark);
//28
	InitSpell(ESpell::kHeal, "исцеление", "heal",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 10, kTypeLife);
//29
	InitSpell(ESpell::kInvisible, "невидимость", "invisible",
			  50, 40, 3, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjRoom, false,
			  kMagAffects | kMagAlterObjs, 0, kTypeMind);
//30
	InitSpell(ESpell::kLightingBolt, "молния", "lightning bolt",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc, 1, kTypeAir);
//31
	InitSpell(ESpell::kLocateObject, "разыскать предмет", "locate object",
			  140, 110, 2, EPosition::kStand, kTarObjWorld,
			  false, kMagManual, 0, kTypeMind);
//32
	InitSpell(ESpell::kMagicMissile, "магическая стрела", "magic missle",
			  40, 30, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kNpcDamagePc, 1, kTypeFire);
//33
	InitSpell(ESpell::kPoison, "яд", "poison", 70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarObjInv | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kMagAlterObjs | kNpcAffectPc, 2, kTypeLife);
//34
	InitSpell(ESpell::kProtectFromEvil, "защита от тьмы", "protect evil",
			  60, 45, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//35
	InitSpell(ESpell::kRemoveCurse, "снять проклятие", "remove curse",
			  50, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, kTypeLight);
//36
	InitSpell(ESpell::kSanctuary, "освящение", "sanctuary",
			  85, 70, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//37
	InitSpell(ESpell::kShockingGasp, "обжигающая хватка", "shocking grasp", 50, 40,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc, 1, kTypeFire);
//38
	InitSpell(ESpell::kSleep, "сон", "sleep",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagAffects | kNpcAffectPc,
			  0, kTypeMind);
//39
	InitSpell(ESpell::kStrength, "сила", "strength", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeLife);
//40
	InitSpell(ESpell::kSummon, "призвать", "summon",
			  110, 100, 2, EPosition::kStand, kTarCharWorld | kTarNotSelf,
			  false, kMagManual, 0, kTypeMind);
//41
	InitSpell(ESpell::kPatronage, "покровительство", "patronage",
			  85, 70, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagPoints | kMagAffects, 1, kTypeLight);
//42
	InitSpell(ESpell::kWorldOfRecall, "слово возврата", "recall",
			  140, 100, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagManual | kNpcDamagePc, 0, kTypeMind);
//43
	InitSpell(ESpell::kRemovePoison, "удалить яд", "remove poison",
			  60, 45, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjRoom, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, kTypeLife);
//44
	InitSpell(ESpell::kSenseLife, "определение жизни", "sense life",
			  85, 70, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, kTypeLife);
//45
	InitSpell(ESpell::kAnimateDead, "поднять труп", "animate dead",
			  50, 35, 3, EPosition::kStand, kTarObjRoom,
			  false, kMagSummons, 0, kTypeDark);
//46
	InitSpell(ESpell::kDispelGood, "рассеять свет", "dispel good",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, kTypeDark);
//47
	InitSpell(ESpell::kGroupArmor, "групповая защита", "group armor",
			  110, 100, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//48
	InitSpell(ESpell::kGroupHeal, "групповое исцеление", "group heal",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, kTypeLife);
//49
	InitSpell(ESpell::kGroupRecall, "групповой возврат", "group recall",
			  125, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagManual, 0, kTypeMind);
//50
	InitSpell(ESpell::kInfravision, "видение ночью", "infravision",
			  50, 40, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeLight);
//51
	InitSpell(ESpell::kWaterwalk, "водохождение", "waterwalk",
			  70, 55, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeWater);
//52
	InitSpell(ESpell::kCureSerious, "серьезное исцеление", "serious cure",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 2, kTypeLife);
//53
	InitSpell(ESpell::kGroupStrength, "групповая сила", "group strength",
			  140, 120, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//54
	InitSpell(ESpell::kHold, "оцепенение", "hold",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 3, kTypeMind);
//55
	InitSpell(ESpell::kPowerHold, "длительное оцепенение", "power hold",
			  140, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 4, kTypeMind);
//56
	InitSpell(ESpell::kMassHold, "массовое оцепенение", "mass hold",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeMind);
//57
	InitSpell(ESpell::kFly, "полет", "fly", 50, 35, 1, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeAir);
//58
	InitSpell(ESpell::kBrokenChains, "разбитые оковы", "broken chains",
			  125, 110, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 5, kTypeMind);
//59
	InitSpell(ESpell::kNoflee, "приковать противника", "noflee",
			  100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 0, kTypeMind);
//60
	InitSpell(ESpell::kCreateLight, "создать свет", "create light",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, kTypeLight);
//61
	InitSpell(ESpell::kDarkness, "тьма", "darkness",
			  100, 70, 2, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeDark);
//62
	InitSpell(ESpell::kStoneSkin, "каменная кожа", "stoneskin", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeEarth);
//63
	InitSpell(ESpell::kCloudly, "затуманивание", "cloudly", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, kTypeWater);
//64
	InitSpell(ESpell::kSllence, "молчание", "sielence",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, kTypeMind);
//65
	InitSpell(ESpell::kLight, "свет", "sun shine", 100, 70,
			  2, EPosition::kFight, kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, kTypeLight);
//66
	InitSpell(ESpell::kChainLighting, "цепь молний", "chain lightning",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 1, kTypeAir);
//67
	InitSpell(ESpell::kFireBlast, "огненный поток", "fireblast",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 5, kTypeFire);
//68
	InitSpell(ESpell::kGodsWrath, "гнев богов", "gods wrath",
			  140, 120, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, kTypeFire);
//69
	InitSpell(ESpell::kWeaknes, "слабость", "weakness",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 0, kTypeLife);
//70
	InitSpell(ESpell::kGroupInvisible, "групповая невидимость", "group invisible",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, kTypeMind);
//71
	InitSpell(ESpell::kShadowCloak, "мантия теней", "shadow cloak",
			  100, 70, 3, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//72
	InitSpell(ESpell::kAcid, "кислота", "acid", 90, 65, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAlterObjs | kNpcDamagePc, 2, kTypeWater);
//73
	InitSpell(ESpell::kRepair, "починка", "repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, kTypeLight);
//74
	InitSpell(ESpell::kEnlarge, "увеличение", "enlarge",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//75
	InitSpell(ESpell::kFear, "страх", "fear",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagManual | kNpcDamagePc,
			  1, kTypeDark);
//76
	InitSpell(ESpell::kSacrifice, "высосать жизнь", "sacrifice",
			  140, 125, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kNpcDamagePc | kNpcDamagePcMinhp, 10, kTypeDark);
//77
	InitSpell(ESpell::kWeb, "сеть", "web",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, kTypeMind);
//78
	InitSpell(ESpell::kBlink, "мигание", "blink", 70, 55, 2,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLight);
//79
	InitSpell(ESpell::kRemoveHold, "снять оцепенение", "remove hold",
			  110, 90, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 1, kTypeLight);
//80
	InitSpell(ESpell::kCamouflage, "!маскировка!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//81
	InitSpell(ESpell::kPowerBlindness, "полная слепота", "power blind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 2, kTypeDark);
//82
	InitSpell(ESpell::kMassBlindness, "массовая слепота", "mass blind",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 4, kTypeDark);
//83
	InitSpell(ESpell::kPowerSilence, "длительное молчание", "power sielence",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, kTypeMind);
//84
	InitSpell(ESpell::kExtraHits, "увеличить жизнь", "extra hits",
			  100, 85, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 1, kTypeLife);
//85
	InitSpell(ESpell::kResurrection, "оживить труп", "ressurection",
			  120, 100, 2, EPosition::kStand, kTarObjRoom, false,
			  kMagSummons, 0, kTypeDark);
//86
	InitSpell(ESpell::kMagicShield, "волшебный щит", "magic shield",
			  50, 30, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLight);

//87
	InitSpell(ESpell::kForbidden, "запечатать комнату", "forbidden",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagRoom, 0, kTypeMind);
//88
	InitSpell(ESpell::kMassSilence, "массовое молчание", "mass sielence", 140, 120,
			  2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 3, kTypeMind);
//89
	InitSpell(ESpell::kRemoveSilence, "снять молчание", "remove sielence",
			  70, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster, 1, kTypeLight);
//90
	InitSpell(ESpell::kDamageLight, "легкий вред", "light damage",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 1, kTypeDark);
//91
	InitSpell(ESpell::kDamageSerious, "серьезный вред", "serious damage",
			  85, 55, 4, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 2, kTypeDark);
//92
	InitSpell(ESpell::kDamageCritic, "критический вред", "critical damage",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 3, kTypeDark);
//93
	InitSpell(ESpell::kMassCurse, "массовое проклятие", "mass curse",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeDark);
//94
	InitSpell(ESpell::kArmageddon, "суд богов", "armageddon",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 10, kTypeAir);
//95
	InitSpell(ESpell::kGroupFly, "групповой полет", "group fly",
			  140, 120, 2, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, kTypeAir);
//96
	InitSpell(ESpell::kGroupBless, "групповая доблесть", "group bless",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//97
	InitSpell(ESpell::kResfresh, "восстановление", "refresh",
			  80, 60, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagPoints, 0, kTypeLife);
//98
	InitSpell(ESpell::kStunning, "каменное проклятие", "stunning",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 15, kTypeEarth);

//99
	InitSpell(ESpell::kHide, "!спрятался!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//100
	InitSpell(ESpell::kSneak, "!крадется!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//101
	InitSpell(ESpell::kDrunked, "!опьянение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//102
	InitSpell(ESpell::kAbstinent, "!абстиненция!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//103
	InitSpell(ESpell::kFullFeed, "насыщение", "full", 70, 55, 1,
			  EPosition::kStand, kTarCharRoom, false, kMagPoints, 10, kTypeLife);
//104
	InitSpell(ESpell::kColdWind, "ледяной ветер", "cold wind", 100, 90,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAffects | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, kTypeWater);
//105
	InitSpell(ESpell::kBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);

//106
	InitSpell(ESpell::kHaemorrhage, "!кровотечение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//107
	InitSpell(ESpell::kCourage, "!ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//108
	InitSpell(ESpell::kWaterbreath, "дышать водой", "waterbreath",
			  85, 70, 4, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, kTypeWater);
//109
	InitSpell(ESpell::kSlowdown, "медлительность", "slow",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//110
	InitSpell(ESpell::kHaste, "ускорение", "haste", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//111
	InitSpell(ESpell::kMassSlow, "массовая медлительность", "mass slow",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);
//112
	InitSpell(ESpell::kGroupHaste, "групповое ускорение", "group haste",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//113
	InitSpell(ESpell::kGodsShield, "защита богов", "gods shield",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeLight);
//114
	InitSpell(ESpell::kFever, "лихорадка", "plaque",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 2, kTypeLife);
//115
	InitSpell(ESpell::kCureFever, "вылечить лихорадку", "cure plaque",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 0, kTypeLife);
//116
	InitSpell(ESpell::kAwareness, "внимательность", "awarness",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, kTypeMind);
//117
	InitSpell(ESpell::kReligion, "!молитва или жертва!", "!pray or donate!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//118
	InitSpell(ESpell::kAirShield, "воздушный щит", "air shield",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeAir);
//119
	InitSpell(ESpell::kPortal, "переход", "portal", 200, 180, 4,
			  EPosition::kStand, kTarCharWorld, false, kMagManual, 0, kTypeLight);
//120
	InitSpell(ESpell::kDispellMagic, "развеять магию", "dispel magic",
			  85, 70, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects, 0, kTypeLight);
//121
	InitSpell(ESpell::kSummonKeeper, "защитник", "keeper",
			  100, 80, 2, EPosition::kStand, kTarIgnore,
			  false, kMagSummons, 0, kTypeLight);
//122
	InitSpell(ESpell::kFastRegeneration, "быстрое восстановление",
			  "fast regeneration", 100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//123
	InitSpell(ESpell::kCreateWeapon, "создать оружие", "create weapon",
			  130, 110, 2, EPosition::kStand,
			  kTarIgnore, false, kMagManual, 0, kTypeLight);
//124
	InitSpell(ESpell::kFireShield, "огненный щит", "fire shield",
			  140, 120, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeFire);
//125
	InitSpell(ESpell::kRelocate, "переместиться", "relocate",
			  140, 120, 2, EPosition::kStand, kTarCharWorld, false,
			  kMagManual, 0, kTypeAir);
//126
	InitSpell(ESpell::kSummonFirekeeper, "огненный защитник", "fire keeper",
			  150, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagSummons, 0, kTypeFire);
//127
	InitSpell(ESpell::kIceShield, "ледяной щит", "ice protect",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeWater);
//128
	InitSpell(ESpell::kIceStorm, "ледяной шторм", "ice storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, kTypeWater);
//129
	InitSpell(ESpell::kLessening, "уменьшение", "enless",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects, 0, kTypeLife);
//130
	InitSpell(ESpell::kShineFlash, "яркий блик", "shine flash",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeFire);
//131
	InitSpell(ESpell::kMadness, "безумие", "madness",
			  130, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//132
	InitSpell(ESpell::kGroupMagicGlass, "магическое зеркало", "group magicglass",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 4, kTypeAir);
//133
	InitSpell(ESpell::kCloudOfArrows, "облако стрел", "cloud of arrous",
			  95, 80, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 4, kTypeFire);
//134
	InitSpell(ESpell::kVacuum, "круг пустоты", "vacuum sphere",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 15, kTypeDark);
//135
	InitSpell(ESpell::kMeteorStorm, "метеоритный дождь", "meteor storm",
			  125, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeEarth);
//136
	InitSpell(ESpell::kStoneHands, "каменные руки", "stonehand",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeEarth);
//137
	InitSpell(ESpell::kMindless, "повреждение разума", "mindness",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 0, kTypeMind);
//138
	InitSpell(ESpell::kPrismaticAura, "призматическая аура", "prismatic aura",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//139
	InitSpell(ESpell::kEviless, "силы зла", "eviless",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects | kMagPoints, 3, kTypeDark);
//140
	InitSpell(ESpell::kAirAura, "воздушная аура", "air aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeAir);
//141
	InitSpell(ESpell::kFireAura, "огненная аура", "fire aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeFire);
//142
	InitSpell(ESpell::kIceAura, "ледяная аура", "ice aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeWater);
//143
	InitSpell(ESpell::kShock, "шок", "shock",
			  100, 90, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeDark);
//144
	InitSpell(ESpell::kMagicGlass, "зеркало магии", "magic glassie",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeLight);

//145
	InitSpell(ESpell::kGroupSanctuary, "групповое освящение", "group sanctuary",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);

//146
	InitSpell(ESpell::kGroupPrismaticAura, "групповая призматическая аура",
			  "group prismatic aura", 110, 100, 1, EPosition::kFight,
			  kTarIgnore, false, kMagGroups | kMagAffects | kNpcAffectNpc,
			  1, kTypeLight);

//147
	InitSpell(ESpell::kDeafness, "глухота", "deafness",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, kTypeMind);

//148
	InitSpell(ESpell::kPowerDeafness, "длительная глухота", "power deafness",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, kTypeMind);

//149
	InitSpell(ESpell::kRemoveDeafness, "снять глухоту", "remove deafness",
			  90, 80, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster,
			  1, kTypeLife);

//150
	InitSpell(ESpell::kMassDeafness, "массовая глухота", "mass deafness",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);
//151
	InitSpell(ESpell::kDustStorm, "пылевая буря", "dust storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, kTypeEarth);
//152
	InitSpell(ESpell::kEarthfall, "камнепад", "earth fall",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeEarth);
//153
	InitSpell(ESpell::kSonicWave, "звуковая волна", "sonic wave",
			  120, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, kTypeAir);
//154
	InitSpell(ESpell::kHolystrike, "силы света", "holystrike",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagManual | kNpcDamagePc, 10, kTypeLight);
//155
	InitSpell(ESpell::kSumonAngel, "ангел-хранитель", "angel", 150, 130, 5,
			  EPosition::kStand, kTarIgnore, false, kMagManual, 1, kTypeLight);
//156
	InitSpell(ESpell::kMassFear, "массовый страх", "mass fear",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagManual | kNpcAffectPc, 4, kTypeDark);
//157
	InitSpell(ESpell::kFascination, "обаяние", "fascination",
			  480, 440, 20,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, kTypeMind);
//158
	InitSpell(ESpell::kCrying, "плач", "crying",
			  120, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//159
	InitSpell(ESpell::kOblivion, "забвение", "oblivion",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeDark);
//160
	InitSpell(ESpell::kBurdenOfTime, "бремя времени", "burden time",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagAreas | kMagAffects | kNpcAffectPc, 4, kTypeDark);
//161
	InitSpell(ESpell::kGroupRefresh, "групповое восстановление", "group refresh",
			  160, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, kTypeLife);

//162
	InitSpell(ESpell::kPeaceful, "смирение", "peaceful",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, kTypeMind);
//163
	InitSpell(ESpell::kMagicBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);

//164
	InitSpell(ESpell::kBerserk, "!предсмертная ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//165
	InitSpell(ESpell::kStoneBones, "каменные кости", "stone bones",
			  80, 40, 1,
			  EPosition::kStand, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, kTypeEarth);

//166 - SPELL_ERoomFlag::kAlwaysLit
	InitSpell(ESpell::kRoomLight, "осветить комнату", "room light",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom, 0, kTypeLight);

//167 - SPELL_POISONED_FOG
	InitSpell(ESpell::kPoosinedFog, "ядовитый туман", "poisoned fog",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, kTypeLife);

//168
	InitSpell(ESpell::kThunderstorm, "буря отмщения", "storm of vengeance",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeAir);
//169
	InitSpell(ESpell::kLightWalk, "!легкая поступь!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false, kMagManual,
			  0, kTypeNeutral);
//170
	InitSpell(ESpell::kFailure, "недоля", "failure",
			  100, 85, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeDark);

//171
	InitSpell(ESpell::kClanPray, "!клановые чары!", "!clan affect!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//172
	InitSpell(ESpell::kGlitterDust, "блестящая пыль", "glitterdust",
			  120, 100, 3, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, kTypeEarth);
//173
	InitSpell(ESpell::kScream, "вопль", "scream",
			  100, 85, 3, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeAir);
//174
	InitSpell(ESpell::kCatGrace, "кошачья ловкость", "cats grace",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//175
	InitSpell(ESpell::kBullBody, "бычье тело", "bull body",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//176
	InitSpell(ESpell::kSnakeWisdom, "мудрость змеи", "snake wisdom",
			  60, 50, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//177
	InitSpell(ESpell::kGimmicry, "хитроумие", "gimmickry",
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
	InitSpell(ESpell::kWarcryOfDefence, "клич обороны", "warcry of defense",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//184
	InitSpell(ESpell::kWarcryOfBattle, "клич битвы", "warcry of battle",
			  20, 20, 50, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//185
	InitSpell(ESpell::kWarcryOfPower, "клич мощи", "warcry of power",
			  25, 25, 70, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagPoints | kMagAffects | kNpcDummy | kNpcAffectNpc,
			  0, kTypeMind);
//186
	InitSpell(ESpell::kWarcryOfBless, "клич доблести", "warcry of bless",
			  15, 15, 30, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//187
	InitSpell(ESpell::kWarcryOfCourage, "клич отваги", "warcry of courage",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//188
	InitSpell(ESpell::kRuneLabel, "рунная метка", "rune label",
			  50, 35, 1,
			  EPosition::kStand, kTarRoomThis, false, kMagRoom | kMagCasterInworldDelay,
			  0, kTypeLight);


	// NON-castable spells should appear below here.

// 189
	InitSpell(ESpell::kAconitumPoison, "яд аконита", "aconitum poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 190
	InitSpell(ESpell::kScopolaPoison, "яд скополии", "scopolia poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 191
	InitSpell(ESpell::kBelenaPoison, "яд белены", "belena poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);
// 192
	InitSpell(ESpell::kDaturaPoison, "яд дурмана", "datura poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, kTypeLife);

// 193
	InitSpell(ESpell::kTimerRestore, "обновление таймера", " timer repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, kTypeLight);
//194
	InitSpell(ESpell::kLucky, "боевое везение", "lacky",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, kTypeMind);
//195
	InitSpell(ESpell::kBandage, "перевязка", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//196
	InitSpell(ESpell::kNoBandage, "!нельзя перевязываться!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//197
	InitSpell(ESpell::kCapable, "!зачарован!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//198
	InitSpell(ESpell::kStrangle, "!удушье!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//199
	InitSpell(ESpell::kRecallSpells, "!вспоминает заклинания!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);
//200
	InitSpell(ESpell::kHypnoticPattern, "чарующий узор", "hypnotic pattern",
			  120, 100, 2, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, kTypeMind);
//201
	InitSpell(ESpell::kSolobonus, "награда", "bonus",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarNotSelf,
			  kMtypeNeutral, kMagManual, 1, kTypeNeutral);
//202
	InitSpell(ESpell::kVampirism, "вампиризм", "vampire",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, kTypeDark);
//203
	InitSpell(ESpell::kRestoration, "реконструкция", "reconstruction",
			  110, 100, 1, EPosition::kStand,
			  kTarObjInv | kTarObjEquip, false, kMagAlterObjs, 0, kTypeLight);
//204
	InitSpell(ESpell::kDeathAura, "аура смерти", "aura death",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//205
	InitSpell(ESpell::kRecovery, "темное прикосновение", "recovery",
			  110, 100, 1, EPosition::kStand, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeDark);
//206
	InitSpell(ESpell::kMassRecovery, "прикосновение смерти", "mass recovery",
			  110, 100, 1, EPosition::kStand, kTarIgnore,
			  false, kMagGroups | kMagPoints | kNpcDummy, 0, kTypeDark);
//207
	InitSpell(ESpell::kAuraOfEvil, "аура зла", "aura evil",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, kTypeDark);
//208
	InitSpell(ESpell::kMentalShadow, "ментальная тень", "mental shadow",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 1, kTypeMind);
//209
	InitSpell(ESpell::kBlackTentacles, "навьи руки", "evards black tentacles",
			  120, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, kTypeDark);
//210
	InitSpell(ESpell::kWhirlwind, "вихрь", "whirlwind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeAir);
//211
	InitSpell(ESpell::kIndriksTeeth, "зубы индрика", "indriks teeth",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//212
	InitSpell(ESpell::kAcidArrow, "кислотная стрела", "acid arrow",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeWater);
//213
	InitSpell(ESpell::kThunderStone, "громовой камень", "thunderstone",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//214
	InitSpell(ESpell::kClod, "глыба", "clod", 110, 100, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, kTypeEarth);
//215
	InitSpell(ESpell::kExpedient, "!боевой прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, kTypeNeutral);
//216
	InitSpell(ESpell::kSightOfDarkness, "зрение тьмы", "sight darkness",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);
//217
	InitSpell(ESpell::kGroupSincerity, "общая искренность", "general sincerity",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//218
	InitSpell(ESpell::kMagicalGaze, "магический взор", "magical gaze",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//219
	InitSpell(ESpell::kAllSeeingEye, "всевидящее око", "allseeing eye",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//220
	InitSpell(ESpell::kEyeOfGods, "око богов", "eye gods",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLife);
//221
	InitSpell(ESpell::kBreathingAtDepth, "дыхание глубин", "breathing at depth",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeWater);
//222
	InitSpell(ESpell::kGeneralRecovery, "общее восстановление", "general recovery",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLife);
//223
	InitSpell(ESpell::kCommonMeal, "общая трапеза", "common meal",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 1, kTypeLife);
//224
	InitSpell(ESpell::kStoneWall, "каменная стена", "stone wall",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeEarth);
//225
	InitSpell(ESpell::kSnakeEyes, "глаза змея", "snake eyes",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeMind);
//226
	InitSpell(ESpell::kEarthAura, "земной поклон", "earth aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeEarth);
//227
	InitSpell(ESpell::kGroupProtectFromEvil, "групповая защита от тьмы", "group protect evil",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, kTypeLight);

//228
	InitSpell(ESpell::kArrowsFire, "стрелы огня", "arrows of fire",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeFire);
//229
	InitSpell(ESpell::kArrowsWater, "стрелы воды", "arrows of water",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeWater);
//230
	InitSpell(ESpell::kArrowsEarth, "стрелы земли", "arrows of earth",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeEarth);
//231
	InitSpell(ESpell::kArrowsAir, "стрелы воздуха", "arrows of air",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeAir);
//232
	InitSpell(ESpell::kArrowsDeath, "стрелы смерти", "arrows of death",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, kTypeDark);
//233
	InitSpell(ESpell::kPaladineInspiration, "воодушевление", "inspiration",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagGroups | kMagAffects, 0, kTypeNeutral);
//234
	InitSpell(ESpell::kDexterity, "ловкость", "dexterity",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, kTypeLife);
//235
	InitSpell(ESpell::kGroupBlink, "групповое мигание", "group blink",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeLife);
//236
	InitSpell(ESpell::kGroupCloudly, "групповое затуманивание", "group cloudly",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeWater);
//237
	InitSpell(ESpell::kGroupAwareness, "групповая внимательность", "group awarness",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, kTypeMind);
//238
	InitSpell(ESpell::kWarcryOfExperience, "клич обучения", "warcry of training",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//239
	InitSpell(ESpell::kWarcryOfLuck, "клич везения", "warcry of luck",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//240
	InitSpell(ESpell::kWarcryOfPhysdamage, "клич точности", "warcry of accuracy",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, kTypeMind);
//241
	InitSpell(ESpell::kMassFailure, "взор Велеса", "gaze of Veles",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeDark);
//242
	InitSpell(ESpell::kSnare, "западня", "snare",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, kTypeMind);

	/*
	 * These spells are currently not used, not implemented, and not castable.
	 * Values for the 'breath' spells are filled in assuming a dragon's breath.
	 */



//243 - 247
	InitSpell(ESpell::kFireBreath, "огненное дыхание", "fire breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeFire);

	InitSpell(ESpell::kGasBreath, "зловонное дыхание", "gas breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeEarth);

	InitSpell(ESpell::kFrostBreath, "ледяное дыхание", "frost breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeAir);

	InitSpell(ESpell::kAcidBreath, "кислотное дыхание", "acid breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeWater);

	InitSpell(ESpell::kLightingBreath, "ослепляющее дыхание", "lightning breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, kTypeDark);
//248
	InitSpell(ESpell::kExpedientFail, "!неудачный прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, kTypeNeutral);

//351
	InitSpell(ESpell::kIdentify, "идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, kTypeMind);
//352
	InitSpell(ESpell::kFullIdentify, "полная идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, kTypeMind);
//353 в dg_affect
	InitSpell(ESpell::kQUest, "!чары!", "!quest spell!",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual,
			  1, kTypeNeutral);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
