#include "spells_info.h"

#include "color.h"
#include "spells.h"
#include "structs/global_objects.h"

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
	int i;

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
			   EPosition minpos, int targets, int violent, int routines, int danger, EElement element) {

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
	spell_info[spell_id].element = element;
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
			  kMagAffects | kNpcAffectNpc, 0, EElement::kLight);
//2
	InitSpell(ESpell::kTeleport, "прыжок", "teleport",
			  140, 120, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagManual | kNpcDamagePc, 1, EElement::kAir);
//3
	InitSpell(ESpell::kBless, "доблесть", "bless", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip, false,
			  kMagAffects | kMagAlterObjs | kNpcAffectNpc, 0, EElement::kLight);
//4
	InitSpell(ESpell::kBlindness, "слепота", "blind",
			  70, 40, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, EElement::kDark);
//5
	InitSpell(ESpell::kBurningHands, "горящие руки", "burning hands",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagAreas | kMagDamage | kNpcDamagePc, 1, EElement::kFire);
//6
	InitSpell(ESpell::kCallLighting, "шаровая молния", "call lightning", 85, 70,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kNpcAffectPc | kMagAffects | kMagDamage | kNpcDamagePc, 2, EElement::kAir);
//7
	InitSpell(ESpell::kCharm, "подчинить разум", "mind control",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual, 1, EElement::kMind);
//8
	InitSpell(ESpell::kChillTouch, "ледяное прикосновение", "chill touch",
			  55, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAffects | kNpcDamagePc, 1, EElement::kWater);
//9
	InitSpell(ESpell::kClone, "клонирование", "clone",
			  150, 130, 5, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagSummons, 0, EElement::kDark);
//10
	InitSpell(ESpell::kIceBolts, "ледяные стрелы", "ice bolts", 90, 75,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 3, EElement::kWater);
//11
	InitSpell(ESpell::kControlWeather, "контроль погоды", "weather control",
			  100, 90, 1, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 0, EElement::kAir);
//12
	InitSpell(ESpell::kCreateFood, "создать пищу", "create food",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, EElement::kLife);
//13
	InitSpell(ESpell::kCreateWater, "создать воду", "create water", 40, 30,
			  1, EPosition::kStand, kTarObjInv | kTarObjEquip | kTarCharRoom, false,
			  kMagManual, 0, EElement::kWater);
//14
	InitSpell(ESpell::kCureBlind, "вылечить слепоту", "cure blind", 110, 90,
			  2, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc, 0, EElement::kLight);
//15
	InitSpell(ESpell::kCureCritic, "критическое исцеление", "critical cure",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 3, EElement::kLife);
//16
	InitSpell(ESpell::kCureLight, "легкое исцеление", "light cure", 40, 30,
			  1, EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagPoints | kNpcDummy, 1, EElement::kLife);
//17
	InitSpell(ESpell::kCurse, "проклятие", "curse",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict | kTarObjInv, kMtypeNeutral,
			  kMagAffects | kMagAlterObjs | kNpcAffectPc, 1, EElement::kDark);
//18
	InitSpell(ESpell::kDetectAlign, "определение наклонностей", "detect alignment",
			  40, 30, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, EElement::kMind);
//19
	InitSpell(ESpell::kDetectInvis, "видеть невидимое", "detect invisible",
			  100, 55, 3, EPosition::kFight, kTarCharRoom, false,
			  kMagAffects, 0, EElement::kMind);
//20
	InitSpell(ESpell::kDetectMagic, "определение магии", "detect magic",
			  100, 55, 3, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, EElement::kMind);
//21
	InitSpell(ESpell::kDetectPoison, "определение яда", "detect poison",
			  40, 30, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, EElement::kMind);
//22
	InitSpell(ESpell::kDispelEvil, "изгнать зло", "dispel evil",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, EElement::kLight);
//23
	InitSpell(ESpell::kEarthquake, "землетрясение", "earthquake",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 2, EElement::kEarth);
//24
	InitSpell(ESpell::kEnchantWeapon, "заколдовать оружие", "enchant weapon", 140,
			  110, 2, EPosition::kStand, kTarObjInv, false, kMagAlterObjs,
			  0, EElement::kLight);
//25
	InitSpell(ESpell::kEnergyDrain, "истощить энергию", "energy drain",
			  150, 140, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kMagAffects | kNpcDamagePc, 1, EElement::kDark);
//26
	InitSpell(ESpell::kFireball, "огненный шар", "fireball",
			  110, 100, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, EElement::kFire);
//27
	InitSpell(ESpell::kHarm, "вред", "harm",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 5, EElement::kDark);
//28
	InitSpell(ESpell::kHeal, "исцеление", "heal",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 10, EElement::kLife);
//29
	InitSpell(ESpell::kInvisible, "невидимость", "invisible",
			  50, 40, 3, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjRoom, false,
			  kMagAffects | kMagAlterObjs, 0, EElement::kMind);
//30
	InitSpell(ESpell::kLightingBolt, "молния", "lightning bolt",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc, 1, EElement::kAir);
//31
	InitSpell(ESpell::kLocateObject, "разыскать предмет", "locate object",
			  140, 110, 2, EPosition::kStand, kTarObjWorld,
			  false, kMagManual, 0, EElement::kMind);
//32
	InitSpell(ESpell::kMagicMissile, "магическая стрела", "magic missle",
			  40, 30, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kNpcDamagePc, 1, EElement::kFire);
//33
	InitSpell(ESpell::kPoison, "яд", "poison", 70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarObjInv | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kMagAlterObjs | kNpcAffectPc, 2, EElement::kLife);
//34
	InitSpell(ESpell::kProtectFromEvil, "защита от тьмы", "protect evil",
			  60, 45, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLight);
//35
	InitSpell(ESpell::kRemoveCurse, "снять проклятие", "remove curse",
			  50, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjEquip, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, EElement::kLight);
//36
	InitSpell(ESpell::kSanctuary, "освящение", "sanctuary",
			  85, 70, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, EElement::kLight);
//37
	InitSpell(ESpell::kShockingGasp, "обжигающая хватка", "shocking grasp", 50, 40,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc, 1, EElement::kFire);
//38
	InitSpell(ESpell::kSleep, "сон", "sleep",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagAffects | kNpcAffectPc,
			  0, EElement::kMind);
//39
	InitSpell(ESpell::kStrength, "сила", "strength", 40, 30, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, EElement::kLife);
//40
	InitSpell(ESpell::kSummon, "призвать", "summon",
			  110, 100, 2, EPosition::kStand, kTarCharWorld | kTarNotSelf,
			  false, kMagManual, 0, EElement::kMind);
//41
	InitSpell(ESpell::kPatronage, "покровительство", "patronage",
			  85, 70, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagPoints | kMagAffects, 1, EElement::kLight);
//42
	InitSpell(ESpell::kWorldOfRecall, "слово возврата", "recall",
			  140, 100, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagManual | kNpcDamagePc, 0, EElement::kMind);
//43
	InitSpell(ESpell::kRemovePoison, "удалить яд", "remove poison",
			  60, 45, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf | kTarObjInv | kTarObjRoom, false,
			  kMagUnaffects | kMagAlterObjs | kNpcUnaffectNpc, 0, EElement::kLife);
//44
	InitSpell(ESpell::kSenseLife, "определение жизни", "sense life",
			  85, 70, 1, EPosition::kStand, kTarCharRoom,
			  false, kMagAffects, 0, EElement::kLife);
//45
	InitSpell(ESpell::kAnimateDead, "поднять труп", "animate dead",
			  50, 35, 3, EPosition::kStand, kTarObjRoom,
			  false, kMagSummons, 0, EElement::kDark);
//46
	InitSpell(ESpell::kDispelGood, "рассеять свет", "dispel good",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 1, EElement::kDark);
//47
	InitSpell(ESpell::kGroupArmor, "групповая защита", "group armor",
			  110, 100, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kLight);
//48
	InitSpell(ESpell::kGroupHeal, "групповое исцеление", "group heal",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, EElement::kLife);
//49
	InitSpell(ESpell::kGroupRecall, "групповой возврат", "group recall",
			  125, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagManual, 0, EElement::kMind);
//50
	InitSpell(ESpell::kInfravision, "видение ночью", "infravision",
			  50, 40, 2, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, EElement::kLight);
//51
	InitSpell(ESpell::kWaterwalk, "водохождение", "waterwalk",
			  70, 55, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, EElement::kWater);
//52
	InitSpell(ESpell::kCureSerious, "серьезное исцеление", "serious cure",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 2, EElement::kLife);
//53
	InitSpell(ESpell::kGroupStrength, "групповая сила", "group strength",
			  140, 120, 2, EPosition::kFight, kTarIgnore,
			  false, kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//54
	InitSpell(ESpell::kHold, "оцепенение", "hold",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 3, EElement::kMind);
//55
	InitSpell(ESpell::kPowerHold, "длительное оцепенение", "power hold",
			  140, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 4, EElement::kMind);
//56
	InitSpell(ESpell::kMassHold, "массовое оцепенение", "mass hold",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, EElement::kMind);
//57
	InitSpell(ESpell::kFly, "полет", "fly", 50, 35, 1, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, EElement::kAir);
//58
	InitSpell(ESpell::kBrokenChains, "разбитые оковы", "broken chains",
			  125, 110, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 5, EElement::kMind);
//59
	InitSpell(ESpell::kNoflee, "приковать противника", "noflee",
			  100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 0, EElement::kMind);
//60
	InitSpell(ESpell::kCreateLight, "создать свет", "create light",
			  40, 30, 1, EPosition::kStand, kTarIgnore, false,
			  kMagCreations, 0, EElement::kLight);
//61
	InitSpell(ESpell::kDarkness, "тьма", "darkness",
			  100, 70, 2, EPosition::kStand,
			  kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, EElement::kDark);
//62
	InitSpell(ESpell::kStoneSkin, "каменная кожа", "stoneskin", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, EElement::kEarth);
//63
	InitSpell(ESpell::kCloudly, "затуманивание", "cloudly", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false, kMagAffects | kNpcAffectNpc,
			  0, EElement::kWater);
//64
	InitSpell(ESpell::kSilence, "молчание", "sielence",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, EElement::kMind);
//65
	InitSpell(ESpell::kLight, "свет", "sun shine", 100, 70,
			  2, EPosition::kFight, kTarCharRoom | kTarObjInv | kTarObjEquip,
			  false, kMagAffects | kMagAlterObjs, 0, EElement::kLight);
//66
	InitSpell(ESpell::kChainLighting, "цепь молний", "chain lightning",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 1, EElement::kAir);
//67
	InitSpell(ESpell::kFireBlast, "огненный поток", "fireblast",
			  110, 90, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 5, EElement::kFire);
//68
	InitSpell(ESpell::kGodsWrath, "гнев богов", "gods wrath",
			  140, 120, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, EElement::kFire);
//69
	InitSpell(ESpell::kWeaknes, "слабость", "weakness",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 0, EElement::kLife);
//70
	InitSpell(ESpell::kGroupInvisible, "групповая невидимость", "group invisible",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, EElement::kMind);
//71
	InitSpell(ESpell::kShadowCloak, "мантия теней", "shadow cloak",
			  100, 70, 3, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kDark);
//72
	InitSpell(ESpell::kAcid, "кислота", "acid", 90, 65, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagDamage | kMagAlterObjs | kNpcDamagePc, 2, EElement::kWater);
//73
	InitSpell(ESpell::kRepair, "починка", "repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, EElement::kLight);
//74
	InitSpell(ESpell::kEnlarge, "увеличение", "enlarge",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
//75
	InitSpell(ESpell::kFear, "страх", "fear",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral, kMagManual | kNpcDamagePc,
			  1, EElement::kDark);
//76
	InitSpell(ESpell::kSacrifice, "высосать жизнь", "sacrifice",
			  140, 125, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagManual | kNpcDamagePc | kNpcDamagePcMinhp, 10, EElement::kDark);
//77
	InitSpell(ESpell::kWeb, "сеть", "web",
			  70, 55, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 1, EElement::kMind);
//78
	InitSpell(ESpell::kBlink, "мигание", "blink", 70, 55, 2,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, EElement::kLight);
//79
	InitSpell(ESpell::kRemoveHold, "снять оцепенение", "remove hold",
			  110, 90, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 1, EElement::kLight);
//80
	InitSpell(ESpell::kCamouflage, "!маскировка!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);
//81
	InitSpell(ESpell::kPowerBlindness, "полная слепота", "power blind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc, 2, EElement::kDark);
//82
	InitSpell(ESpell::kMassBlindness, "массовая слепота", "mass blind",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 4, EElement::kDark);
//83
	InitSpell(ESpell::kPowerSilence, "длительное молчание", "power sielence",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, EElement::kMind);
//84
	InitSpell(ESpell::kExtraHits, "увеличить жизнь", "extra hits",
			  100, 85, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagPoints | kNpcDummy, 1, EElement::kLife);
//85
	InitSpell(ESpell::kResurrection, "оживить труп", "ressurection",
			  120, 100, 2, EPosition::kStand, kTarObjRoom, false,
			  kMagSummons, 0, EElement::kDark);
//86
	InitSpell(ESpell::kMagicShield, "волшебный щит", "magic shield",
			  50, 30, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLight);

//87
	InitSpell(ESpell::kForbidden, "запечатать комнату", "forbidden",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagRoom, 0, EElement::kMind);
//88
	InitSpell(ESpell::kMassSilence, "массовое молчание", "mass sielence", 140, 120,
			  2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 3, EElement::kMind);
//89
	InitSpell(ESpell::kRemoveSilence, "снять молчание", "remove sielence",
			  70, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster, 1, EElement::kLight);
//90
	InitSpell(ESpell::kDamageLight, "легкий вред", "light damage",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 1, EElement::kDark);
//91
	InitSpell(ESpell::kDamageSerious, "серьезный вред", "serious damage",
			  85, 55, 4, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 2, EElement::kDark);
//92
	InitSpell(ESpell::kDamageCritic, "критический вред", "critical damage",
			  100, 90, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 3, EElement::kDark);
//93
	InitSpell(ESpell::kMassCurse, "массовое проклятие", "mass curse",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, EElement::kDark);
//94
	InitSpell(ESpell::kArmageddon, "суд богов", "armageddon",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kNpcDamagePc, 10, EElement::kAir);
//95
	InitSpell(ESpell::kGroupFly, "групповой полет", "group fly",
			  140, 120, 2, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects, 0, EElement::kAir);
//96
	InitSpell(ESpell::kGroupBless, "групповая доблесть", "group bless",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kLight);
//97
	InitSpell(ESpell::kResfresh, "восстановление", "refresh",
			  80, 60, 1, EPosition::kStand, kTarCharRoom, false,
			  kMagPoints, 0, EElement::kLife);
//98
	InitSpell(ESpell::kStunning, "каменное проклятие", "stunning",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage, 15, EElement::kEarth);

//99
	InitSpell(ESpell::kHide, "!спрятался!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);

//100
	InitSpell(ESpell::kSneak, "!крадется!", "!set by skill!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);

//101
	InitSpell(ESpell::kDrunked, "!опьянение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);

//102
	InitSpell(ESpell::kAbstinent, "!абстиненция!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);

//103
	InitSpell(ESpell::kFullFeed, "насыщение", "full", 70, 55, 1,
			  EPosition::kStand, kTarCharRoom, false, kMagPoints, 10, EElement::kLife);
//104
	InitSpell(ESpell::kColdWind, "ледяной ветер", "cold wind", 100, 90,
			  1, EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAffects | kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 15, EElement::kWater);
//105
	InitSpell(ESpell::kBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);

//106
	InitSpell(ESpell::kHaemorrhage, "!кровотечение!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);
//107
	InitSpell(ESpell::kCourage, "!ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);

//108
	InitSpell(ESpell::kWaterbreath, "дышать водой", "waterbreath",
			  85, 70, 4, EPosition::kStand, kTarCharRoom, false,
			  kMagAffects, 0, EElement::kWater);
//109
	InitSpell(ESpell::kSlowdown, "медлительность", "slow",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, EElement::kMind);
//110
	InitSpell(ESpell::kHaste, "ускорение", "haste", 55, 40, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//111
	InitSpell(ESpell::kMassSlow, "массовая медлительность", "mass slow",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, EElement::kMind);
//112
	InitSpell(ESpell::kGroupHaste, "групповое ускорение", "group haste",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kMind);
//113
	InitSpell(ESpell::kGodsShield, "защита богов", "gods shield",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects | kNpcAffectNpc, 2, EElement::kLight);
//114
	InitSpell(ESpell::kFever, "лихорадка", "plaque",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 2, EElement::kLife);
//115
	InitSpell(ESpell::kCureFever, "вылечить лихорадку", "cure plaque",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc, 0, EElement::kLife);
//116
	InitSpell(ESpell::kAwareness, "внимательность", "awarness",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, EElement::kMind);
//117
	InitSpell(ESpell::kReligion, "!молитва или жертва!", "!pray or donate!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);
//118
	InitSpell(ESpell::kAirShield, "воздушный щит", "air shield",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kAir);
//119
	InitSpell(ESpell::kPortal, "переход", "portal", 200, 180, 4,
			  EPosition::kStand, kTarCharWorld, false, kMagManual, 0, EElement::kLight);
//120
	InitSpell(ESpell::kDispellMagic, "развеять магию", "dispel magic",
			  85, 70, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects, 0, EElement::kLight);
//121
	InitSpell(ESpell::kSummonKeeper, "защитник", "keeper",
			  100, 80, 2, EPosition::kStand, kTarIgnore,
			  false, kMagSummons, 0, EElement::kLight);
//122
	InitSpell(ESpell::kFastRegeneration, "быстрое восстановление",
			  "fast regeneration", 100, 90, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
//123
	InitSpell(ESpell::kCreateWeapon, "создать оружие", "create weapon",
			  130, 110, 2, EPosition::kStand,
			  kTarIgnore, false, kMagManual, 0, EElement::kLight);
//124
	InitSpell(ESpell::kFireShield, "огненный щит", "fire shield",
			  140, 120, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, EElement::kFire);
//125
	InitSpell(ESpell::kRelocate, "переместиться", "relocate",
			  140, 120, 2, EPosition::kStand, kTarCharWorld, false,
			  kMagManual, 0, EElement::kAir);
//126
	InitSpell(ESpell::kSummonFirekeeper, "огненный защитник", "fire keeper",
			  150, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagSummons, 0, EElement::kFire);
//127
	InitSpell(ESpell::kIceShield, "ледяной щит", "ice protect",
			  140, 120, 2, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kWater);
//128
	InitSpell(ESpell::kIceStorm, "ледяной шторм", "ice storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, EElement::kWater);
//129
	InitSpell(ESpell::kLessening, "уменьшение", "enless",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects, 0, EElement::kLife);
//130
	InitSpell(ESpell::kShineFlash, "яркий блик", "shine flash",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kFire);
//131
	InitSpell(ESpell::kMadness, "безумие", "madness",
			  130, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, EElement::kMind);
//132
	InitSpell(ESpell::kGroupMagicGlass, "магическое зеркало", "group magicglass",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 4, EElement::kAir);
//133
	InitSpell(ESpell::kCloudOfArrows, "облако стрел", "cloud of arrous",
			  95, 80, 2, EPosition::kFight, kTarSelfOnly | kTarCharRoom,
			  false, kMagAffects | kNpcAffectNpc, 4, EElement::kFire);
//134
	InitSpell(ESpell::kVacuum, "круг пустоты", "vacuum sphere",
			  150, 140, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagDamage | kNpcDamagePc, 15, EElement::kDark);
//135
	InitSpell(ESpell::kMeteorStorm, "метеоритный дождь", "meteor storm",
			  125, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, EElement::kEarth);
//136
	InitSpell(ESpell::kStoneHands, "каменные руки", "stonehand",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kEarth);
//137
	InitSpell(ESpell::kMindless, "повреждение разума", "mindness",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeNeutral, kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 0, EElement::kMind);
//138
	InitSpell(ESpell::kPrismaticAura, "призматическая аура", "prismatic aura",
			  85, 70, 4, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 1, EElement::kLight);
//139
	InitSpell(ESpell::kEviless, "силы зла", "eviless",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagAffects | kMagPoints, 3, EElement::kDark);
//140
	InitSpell(ESpell::kAirAura, "воздушная аура", "air aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kAir);
//141
	InitSpell(ESpell::kFireAura, "огненная аура", "fire aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kFire);
//142
	InitSpell(ESpell::kIceAura, "ледяная аура", "ice aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kWater);
//143
	InitSpell(ESpell::kShock, "шок", "shock",
			  100, 90, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, EElement::kDark);
//144
	InitSpell(ESpell::kMagicGlass, "зеркало магии", "magic glassie",
			  120, 110, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, EElement::kLight);

//145
	InitSpell(ESpell::kGroupSanctuary, "групповое освящение", "group sanctuary",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kLight);

//146
	InitSpell(ESpell::kGroupPrismaticAura, "групповая призматическая аура",
			  "group prismatic aura", 110, 100, 1, EPosition::kFight,
			  kTarIgnore, false, kMagGroups | kMagAffects | kNpcAffectNpc,
			  1, EElement::kLight);

//147
	InitSpell(ESpell::kDeafness, "глухота", "deafness",
			  100, 40, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 1, EElement::kMind);

//148
	InitSpell(ESpell::kPowerDeafness, "длительная глухота", "power deafness",
			  120, 90, 4, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc | kNpcAffectPcCaster, 2, EElement::kMind);

//149
	InitSpell(ESpell::kRemoveDeafness, "снять глухоту", "remove deafness",
			  90, 80, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagUnaffects | kNpcUnaffectNpc | kNpcUnaffectNpcCaster,
			  1, EElement::kLife);

//150
	InitSpell(ESpell::kMassDeafness, "массовая глухота", "mass deafness",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, EElement::kMind);
//151
	InitSpell(ESpell::kDustStorm, "пылевая буря", "dust storm",
			  125, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc, 5, EElement::kEarth);
//152
	InitSpell(ESpell::kEarthfall, "камнепад", "earth fall",
			  120, 110, 2, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, EElement::kEarth);
//153
	InitSpell(ESpell::kSonicWave, "звуковая волна", "sonic wave",
			  120, 110, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  1, EElement::kAir);
//154
	InitSpell(ESpell::kHolystrike, "силы света", "holystrike",
			  150, 130, 5, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagManual | kNpcDamagePc, 10, EElement::kLight);
//155
	InitSpell(ESpell::kSumonAngel, "ангел-хранитель", "angel", 150, 130, 5,
			  EPosition::kStand, kTarIgnore, false, kMagManual, 1, EElement::kLight);
//156
	InitSpell(ESpell::kMassFear, "массовый страх", "mass fear",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagManual | kNpcAffectPc, 4, EElement::kDark);
//157
	InitSpell(ESpell::kFascination, "обаяние", "fascination",
			  480, 440, 20,
			  EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 2, EElement::kMind);
//158
	InitSpell(ESpell::kCrying, "плач", "crying",
			  120, 55, 2, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, EElement::kMind);
//159
	InitSpell(ESpell::kOblivion, "забвение", "oblivion",
			  70, 55, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, EElement::kDark);
//160
	InitSpell(ESpell::kBurdenOfTime, "бремя времени", "burden time",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagAreas | kMagAffects | kNpcAffectPc, 4, EElement::kDark);
//161
	InitSpell(ESpell::kGroupRefresh, "групповое восстановление", "group refresh",
			  160, 140, 1, EPosition::kStand, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 30, EElement::kLife);

//162
	InitSpell(ESpell::kPeaceful, "смирение", "peaceful",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf | kTarFightVict, kMtypeNeutral,
			  kMagAffects | kNpcAffectPc, 1, EElement::kMind);
//163
	InitSpell(ESpell::kMagicBattle, "!получил в бою!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);

//164
	InitSpell(ESpell::kBerserk, "!предсмертная ярость!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);
//165
	InitSpell(ESpell::kStoneBones, "каменные кости", "stone bones",
			  80, 40, 1,
			  EPosition::kStand, kTarCharRoom | kTarFightSelf, false,
			  kMagAffects | kNpcAffectNpc, 0, EElement::kEarth);

//166 - SPELL_ERoomFlag::kAlwaysLit
	InitSpell(ESpell::kRoomLight, "осветить комнату", "room light",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom, 0, EElement::kLight);

//167 - SPELL_POISONED_FOG
	InitSpell(ESpell::kPoosinedFog, "ядовитый туман", "poisoned fog",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, EElement::kLife);

//168
	InitSpell(ESpell::kThunderstorm, "буря отмщения", "storm of vengeance",
			  10, 10, 1, EPosition::kStand, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, EElement::kAir);
//169
	InitSpell(ESpell::kLightWalk, "!легкая поступь!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false, kMagManual,
			  0, EElement::kUndefined);
//170
	InitSpell(ESpell::kFailure, "недоля", "failure",
			  100, 85, 2, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, EElement::kDark);

//171
	InitSpell(ESpell::kClanPray, "!клановые чары!", "!clan affect!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);
//172
	InitSpell(ESpell::kGlitterDust, "блестящая пыль", "glitterdust",
			  120, 100, 3, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 5, EElement::kEarth);
//173
	InitSpell(ESpell::kScream, "вопль", "scream",
			  100, 85, 3, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp, 2, EElement::kAir);
//174
	InitSpell(ESpell::kCatGrace, "кошачья ловкость", "cats grace",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
//175
	InitSpell(ESpell::kBullBody, "бычье тело", "bull body",
			  50, 40, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
//176
	InitSpell(ESpell::kSnakeWisdom, "мудрость змеи", "snake wisdom",
			  60, 50, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
//177
	InitSpell(ESpell::kGimmicry, "хитроумие", "gimmickry",
			  60, 50, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
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
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//184
	InitSpell(ESpell::kWarcryOfBattle, "клич битвы", "warcry of battle",
			  20, 20, 50, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//185
	InitSpell(ESpell::kWarcryOfPower, "клич мощи", "warcry of power",
			  25, 25, 70, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagPoints | kMagAffects | kNpcDummy | kNpcAffectNpc,
			  0, EElement::kMind);
//186
	InitSpell(ESpell::kWarcryOfBless, "клич доблести", "warcry of bless",
			  15, 15, 30, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//187
	InitSpell(ESpell::kWarcryOfCourage, "клич отваги", "warcry of courage",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//188
	InitSpell(ESpell::kRuneLabel, "рунная метка", "rune label",
			  50, 35, 1,
			  EPosition::kStand, kTarRoomThis, false, kMagRoom | kMagCasterInworldDelay,
			  0, EElement::kLight);


	// NON-castable spells should appear below here.

// 189
	InitSpell(ESpell::kAconitumPoison, "яд аконита", "aconitum poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, EElement::kLife);
// 190
	InitSpell(ESpell::kScopolaPoison, "яд скополии", "scopolia poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, EElement::kLife);
// 191
	InitSpell(ESpell::kBelenaPoison, "яд белены", "belena poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, EElement::kLife);
// 192
	InitSpell(ESpell::kDaturaPoison, "яд дурмана", "datura poison",
			  0, 0, 0, EPosition::kFight, kTarIgnore, kMtypeAggressive,
			  kMagAffects, 0, EElement::kLife);

// 193
	InitSpell(ESpell::kTimerRestore, "обновление таймера", " timer repair",
			  110, 100, 1, EPosition::kStand, kTarObjInv | kTarObjEquip,
			  false, kMagAlterObjs, 0, EElement::kLight);
//194
	InitSpell(ESpell::kLucky, "боевое везение", "lacky",
			  100, 90, 1, EPosition::kStand, kTarCharRoom | kTarSelfOnly,
			  false, kMagAffects, 0, EElement::kMind);
//195
	InitSpell(ESpell::kBandage, "перевязка", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);
//196
	InitSpell(ESpell::kNoBandage, "!нельзя перевязываться!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);
//197
	InitSpell(ESpell::kCapable, "!зачарован!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);
//198
	InitSpell(ESpell::kStrangle, "!удушье!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);
//199
	InitSpell(ESpell::kRecallSpells, "!вспоминает заклинания!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);
//200
	InitSpell(ESpell::kHypnoticPattern, "чарующий узор", "hypnotic pattern",
			  120, 100, 2, EPosition::kStand, kTarRoomThis, false,
			  kMagRoom | kMagCasterInroom, 0, EElement::kMind);
//201
	InitSpell(ESpell::kSolobonus, "награда", "bonus",
			  55, 40, 1, EPosition::kFight, kTarCharRoom | kTarNotSelf,
			  kMtypeNeutral, kMagManual, 1, EElement::kUndefined);
//202
	InitSpell(ESpell::kVampirism, "вампиризм", "vampire",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, EElement::kDark);
//203
	InitSpell(ESpell::kRestoration, "реконструкция", "reconstruction",
			  110, 100, 1, EPosition::kStand,
			  kTarObjInv | kTarObjEquip, false, kMagAlterObjs, 0, EElement::kLight);
//204
	InitSpell(ESpell::kDeathAura, "аура смерти", "aura death",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kDark);
//205
	InitSpell(ESpell::kRecovery, "темное прикосновение", "recovery",
			  110, 100, 1, EPosition::kStand, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kDark);
//206
	InitSpell(ESpell::kMassRecovery, "прикосновение смерти", "mass recovery",
			  110, 100, 1, EPosition::kStand, kTarIgnore,
			  false, kMagGroups | kMagPoints | kNpcDummy, 0, EElement::kDark);
//207
	InitSpell(ESpell::kAuraOfEvil, "аура зла", "aura evil",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 3, EElement::kDark);
//208
	InitSpell(ESpell::kMentalShadow, "ментальная тень", "mental shadow",
			  150, 130, 5, EPosition::kStand, kTarIgnore, false,
			  kMagManual, 1, EElement::kMind);
//209
	InitSpell(ESpell::kBlackTentacles, "навьи руки", "evards black tentacles",
			  120, 110, 2, EPosition::kFight, kTarRoomThis, false,
			  kMagNeedControl | kMagRoom | kMagCasterInroom, 0, EElement::kDark);
//210
	InitSpell(ESpell::kWhirlwind, "вихрь", "whirlwind",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kAir);
//211
	InitSpell(ESpell::kIndriksTeeth, "зубы индрика", "indriks teeth",
			  60, 45, 1, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kEarth);
//212
	InitSpell(ESpell::kAcidArrow, "кислотная стрела", "acid arrow",
			  110, 100, 1, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kWater);
//213
	InitSpell(ESpell::kThunderStone, "громовой камень", "thunderstone",
			  110, 100, 2, EPosition::kFight, kTarCharRoom | kTarFightVict,
			  kMtypeAggressive, kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kEarth);
//214
	InitSpell(ESpell::kClod, "глыба", "clod", 110, 100, 1,
			  EPosition::kFight, kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcDamagePc | kNpcDamagePcMinhp, 2, EElement::kEarth);
//215
	InitSpell(ESpell::kExpedient, "!боевой прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0,
			  false, kMagManual, 0, EElement::kUndefined);
//216
	InitSpell(ESpell::kSightOfDarkness, "зрение тьмы", "sight darkness",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kLight);
//217
	InitSpell(ESpell::kGroupSincerity, "общая искренность", "general sincerity",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kMind);
//218
	InitSpell(ESpell::kMagicalGaze, "магический взор", "magical gaze",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kMind);
//219
	InitSpell(ESpell::kAllSeeingEye, "всевидящее око", "allseeing eye",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kMind);
//220
	InitSpell(ESpell::kEyeOfGods, "око богов", "eye gods",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kLife);
//221
	InitSpell(ESpell::kBreathingAtDepth, "дыхание глубин", "breathing at depth",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kWater);
//222
	InitSpell(ESpell::kGeneralRecovery, "общее восстановление", "general recovery",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kLife);
//223
	InitSpell(ESpell::kCommonMeal, "общая трапеза", "common meal",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagPoints | kNpcDummy, 1, EElement::kLife);
//224
	InitSpell(ESpell::kStoneWall, "каменная стена", "stone wall",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kEarth);
//225
	InitSpell(ESpell::kSnakeEyes, "глаза змея", "snake eyes",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kMind);
//226
	InitSpell(ESpell::kEarthAura, "земной поклон", "earth aura",
			  140, 120, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kEarth);
//227
	InitSpell(ESpell::kGroupProtectFromEvil, "групповая защита от тьмы", "group protect evil",
			  110, 100, 1, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 1, EElement::kLight);

//228
	InitSpell(ESpell::kArrowsFire, "стрелы огня", "arrows of fire",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kFire);
//229
	InitSpell(ESpell::kArrowsWater, "стрелы воды", "arrows of water",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kWater);
//230
	InitSpell(ESpell::kArrowsEarth, "стрелы земли", "arrows of earth",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagAreas | kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kEarth);
//231
	InitSpell(ESpell::kArrowsAir, "стрелы воздуха", "arrows of air",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kAir);
//232
	InitSpell(ESpell::kArrowsDeath, "стрелы смерти", "arrows of death",
			  0, 0, 0, EPosition::kFight,
			  kTarCharRoom | kTarFightVict, kMtypeAggressive,
			  kMagDamage | kNpcAffectPc | kMagAffects | kNpcDamagePc | kNpcDamagePcMinhp,
			  2, EElement::kDark);
//233
	InitSpell(ESpell::kPaladineInspiration, "воодушевление", "inspiration",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagGroups | kMagAffects, 0, EElement::kUndefined);
//234
	InitSpell(ESpell::kDexterity, "ловкость", "dexterity",
			  40, 30, 1, EPosition::kFight, kTarCharRoom | kTarFightSelf,
			  false, kMagAffects | kNpcAffectNpc, 0, EElement::kLife);
//235
	InitSpell(ESpell::kGroupBlink, "групповое мигание", "group blink",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, EElement::kLife);
//236
	InitSpell(ESpell::kGroupCloudly, "групповое затуманивание", "group cloudly",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, EElement::kWater);
//237
	InitSpell(ESpell::kGroupAwareness, "групповая внимательность", "group awarness",
			  110, 100, 2, EPosition::kFight, kTarIgnore, false,
			  kMagGroups | kMagAffects | kNpcAffectNpc, 30, EElement::kMind);
//238
	InitSpell(ESpell::kWarcryOfExperience, "клич обучения", "warcry of training",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//239
	InitSpell(ESpell::kWarcryOfLuck, "клич везения", "warcry of luck",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//240
	InitSpell(ESpell::kWarcryOfPhysdamage, "клич точности", "warcry of accuracy",
			  10, 10, 10, EPosition::kFight, kTarIgnore, false,
			  kMagWarcry | kMagGroups | kMagAffects | kNpcAffectNpc, 0, EElement::kMind);
//241
	InitSpell(ESpell::kMassFailure, "взор Велеса", "gaze of Veles",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, EElement::kDark);
//242
	InitSpell(ESpell::kSnare, "западня", "snare",
			  140, 120, 2, EPosition::kFight, kTarIgnore, kMtypeNeutral,
			  kMagMasses | kMagAffects | kNpcAffectPc, 2, EElement::kMind);

	/*
	 * These spells are currently not used, not implemented, and not castable.
	 * Values for the 'breath' spells are filled in assuming a dragon's breath.
	 */



//243 - 247
	InitSpell(ESpell::kFireBreath, "огненное дыхание", "fire breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, EElement::kFire);

	InitSpell(ESpell::kGasBreath, "зловонное дыхание", "gas breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, EElement::kEarth);

	InitSpell(ESpell::kFrostBreath, "ледяное дыхание", "frost breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, EElement::kAir);

	InitSpell(ESpell::kAcidBreath, "кислотное дыхание", "acid breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, EElement::kWater);

	InitSpell(ESpell::kLightingBreath, "ослепляющее дыхание", "lightning breath",
			  0, 0, 0, EPosition::kStand, kTarIgnore, true,
			  kMagDamage, 3, EElement::kDark);
//248
	InitSpell(ESpell::kExpedientFail, "!неудачный прием!", "!set by programm!",
			  0, 0, 0, EPosition::kStand, 0, false,
			  kMagManual, 0, EElement::kUndefined);

//351
	InitSpell(ESpell::kIdentify, "идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, EElement::kMind);
//352
	InitSpell(ESpell::kFullIdentify, "полная идентификация", "identify",
			  0, 0, 0, EPosition::kSit,
			  kTarCharRoom | kTarObjInv | kTarObjRoom | kTarObjEquip, false,
			  kMagManual, 0, EElement::kMind);
//353 в dg_affect
	InitSpell(ESpell::kQUest, "!чары!", "!quest spell!",
			  55, 40, 1, EPosition::kFight,
			  kTarCharRoom | kTarNotSelf, kMtypeNeutral, kMagManual,
			  1, EElement::kUndefined);

}

// ==================================================================================================================
// ==================================================================================================================
// ==================================================================================================================

namespace spells {

using ItemPtr = SpellInfoBuilder::ItemPtr;

void SpellsLoader::Load(DataNode data) {
	MUD::Spells().Init(data.Children());
}

void SpellsLoader::Reload(DataNode data) {
	MUD::Spells().Reload(data.Children());
}

ItemPtr SpellInfoBuilder::Build(DataNode &node) {
	try {
		return ParseSpell(node);
	} catch (std::exception &e) {
		err_log("Spell parsing error (incorrect value '%s')", e.what());
		return nullptr;
	}
}

/*    <spell id="kChillTouch" element="kTypeWater" mode="kEnabled">
        <name rus="ледяное прикосновение" eng="chill touch"/>
        <misc pos="kFight" violent="N" danger="1"/>
        <mana max="55" min="45" change="1"/>
        <targets val="kTarCharRoom|kTarFightVict"/>
        <flags val="kMagDamage|kMagAffects|kNpcDamagePc"/> */

ItemPtr SpellInfoBuilder::ParseSpell(DataNode node) {
	auto id{ESpell::kUndefined};
	try {
		id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect spell id (%s).", e.what());
		return nullptr;
	}
	auto mode = SpellInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	auto info = std::make_shared<SpellInfo>(id, mode);

	try {
		info->element_ = parse::ReadAsConstant<EElement>(node.GetValue("element"));
	} catch (std::exception &) {
	}

	if (node.GoToChild("name")) {
		try {
			info->name_ = parse::ReadAsStr(node.GetValue("rus"));
			info->name_eng_ = parse::ReadAsStr(node.GetValue("eng"));
		} catch (std::exception &e) {
			err_log("Incorrect 'name' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}

	if (node.GoToChild("misc")) {
		try {
			info->min_position_ = parse::ReadAsConstant<EPosition>(node.GetValue("pos"));
			info->violent_ = parse::ReadAsBool(node.GetValue("violent"));
			info->danger_ = parse::ReadAsInt(node.GetValue("danger"));
		} catch (std::exception &e) {
			err_log("Incorrect 'misc' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}

	if (node.GoToChild("mana")) {
		try {
			info->min_mana_ = parse::ReadAsInt(node.GetValue("min"));
			info->max_mana_ = parse::ReadAsInt(node.GetValue("max"));
			info->mana_change_ = parse::ReadAsInt(node.GetValue("change"));
		} catch (std::exception &e) {
			err_log("Incorrect 'mana' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
/*
	if (node.GoToSibling("targets")) {

	}

	if (node.GoToSibling("flags")) {

	}*/
	//node.GoToSibling("flags");

	return info;
}

bool SpellInfo::IsFlagged(Bitvector flag) const {
	return IS_SET(flags_, flag);
}

bool SpellInfo::AllowTarget(Bitvector target_type) const {
	return IS_SET(targets_, target_type);
}

// \todo Не забыть добавить поля
void SpellInfo::Print(std::ostringstream &buffer) const {
	buffer << "Print spell:" << std::endl
		   << " Id: " << KGRN << NAME_BY_ITEM<ESpell>(GetId()) << KNRM
		   << " Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(GetMode()) << KNRM << std::endl
		   << " Name (rus): " << KGRN << name_ << KNRM << std::endl
		   << " Name (eng): " << KGRN << name_eng_ << KNRM << std::endl
		   << " Element: " << KGRN << NAME_BY_ITEM<EElement>(element_) << KNRM << std::endl
		   << " Min position: " << KGRN << NAME_BY_ITEM<EPosition>(min_position_) << KNRM << std::endl
		   << " Violent: " << KGRN << (violent_ ? "Yes" : "No") << KNRM << std::endl
		   << " Danger: " << KGRN << danger_ << KNRM << std::endl
		   << " Mana min: " << KGRN << min_mana_ << KNRM
		   << " Mana max: " << KGRN << max_mana_ << KNRM
		   << " Mana change: " << KGRN << mana_change_ << KNRM << std::endl;
}

/*
Bitvector flags_{0};
Bitvector targets_{0};
*/

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
