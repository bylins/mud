/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#include "affect_contants.h"

#include "magic/spells.h"

//#include <map>

const char *affected_bits[] = {"слепота",    // 0
							   "невидимость",
							   "определение наклонностей",
							   "определение невидимости",
							   "определение магии",
							   "чувствовать жизнь",
							   "хождение по воде",
							   "освящение",
							   "состоит в группе",
							   "проклятие",
							   "инфравидение",        // 10
							   "яд",
							   "защита от тьмы",
							   "защита от света",
							   "магический сон",
							   "нельзя выследить",
							   "привязан",
							   "доблесть",
							   "подкрадывается",
							   "прячется",
							   "ярость",        // 20
							   "зачарован",
							   "оцепенение",
							   "летит",
							   "молчание",
							   "настороженность",
							   "мигание",
							   "верхом или под седлом",
							   "не сбежать",
							   "свет",
							   "\n",
							   "освещение",        // 0
							   "затемнение",
							   "определение яда",
							   "под мухой",
							   "отходняк",
							   "декстраплегия",
							   "синистроплегия",
							   "параплегия",
							   "кровотечение",
							   "маскировка",
							   "дышать водой",
							   "медлительность",
							   "ускорение",
							   "защита богов",
							   "воздушный щит",
							   "огненный щит",
							   "ледяной щит",
							   "зеркало магии",
							   "звезды",
							   "каменная рука",
							   "призматическая.аура",
							   "нанят",
							   "силы зла",
							   "воздушная аура",
							   "огненная аура",
							   "ледяная аура",
							   "глухота",
							   "плач",
							   "смирение",
							   "маг параплегия",
							   "\n",
							   "предсмертная ярость",
							   "легкая поступь",
							   "разбитые оковы",
							   "облако стрел",
							   "мантия теней",
							   "блестящая пыль",
							   "испуг",
							   "яд скополии",
							   "яд дурмана",
							   "понижение умений",
							   "не переключается",
							   "яд белены",
							   "прикован",
							   "боевое везение",
							   "перевязка",
							   "не может перевязываться",
							   "превращен",
							   "удушье",
							   "вспоминает заклинания",
							   "регенерация новичка",
							   "вампиризм",
							   "потеря равновесия",
							   "полководец",
							   "земной поклон",
							   "доминирование",
							   "\n",
							   "\n",
};

typedef std::map<EAffectFlag, std::string> EAffectFlag_name_by_value_t;
typedef std::map<const std::string, EAffectFlag> EAffectFlag_value_by_name_t;
EAffectFlag_name_by_value_t EAffectFlag_name_by_value;
EAffectFlag_value_by_name_t EAffectFlag_value_by_name;
void init_EAffectFlag_ITEM_NAMES() {
	EAffectFlag_value_by_name.clear();
	EAffectFlag_name_by_value.clear();

	EAffectFlag_name_by_value[EAffectFlag::AFF_BLIND] = "AFF_BLIND";
	EAffectFlag_name_by_value[EAffectFlag::AFF_INVISIBLE] = "AFF_INVISIBLE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_ALIGN] = "AFF_DETECT_ALIGN";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_INVIS] = "AFF_DETECT_INVIS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_MAGIC] = "AFF_DETECT_MAGIC";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SENSE_LIFE] = "AFF_SENSE_LIFE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_WATERWALK] = "AFF_WATERWALK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SANCTUARY] = "AFF_SANCTUARY";
	EAffectFlag_name_by_value[EAffectFlag::AFF_GROUP] = "AFF_GROUP";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CURSE] = "AFF_CURSE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_INFRAVISION] = "AFF_INFRAVISION";
	EAffectFlag_name_by_value[EAffectFlag::AFF_POISON] = "AFF_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PROTECT_EVIL] = "AFF_PROTECT_EVIL";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PROTECT_GOOD] = "AFF_PROTECT_GOOD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SLEEP] = "AFF_SLEEP";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOTRACK] = "AFF_NOTRACK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_TETHERED] = "AFF_TETHERED";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BLESS] = "AFF_BLESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SNEAK] = "AFF_SNEAK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HIDE] = "AFF_HIDE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_COURAGE] = "AFF_COURAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CHARM] = "AFF_CHARM";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HOLD] = "AFF_HOLD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_FLY] = "AFF_FLY";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SILENCE] = "AFF_SILENCE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AWARNESS] = "AFF_AWARNESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BLINK] = "AFF_BLINK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HORSE] = "AFF_HORSE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOFLEE] = "AFF_NOFLEE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SINGLELIGHT] = "AFF_SINGLELIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HOLYLIGHT] = "AFF_HOLYLIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HOLYDARK] = "AFF_HOLYDARK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_POISON] = "AFF_DETECT_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DRUNKED] = "AFF_DRUNKED";
	EAffectFlag_name_by_value[EAffectFlag::AFF_ABSTINENT] = "AFF_ABSTINENT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STOPRIGHT] = "AFF_STOPRIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STOPLEFT] = "AFF_STOPLEFT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STOPFIGHT] = "AFF_STOPFIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HAEMORRAGIA] = "AFF_HAEMORRAGIA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CAMOUFLAGE] = "AFF_CAMOUFLAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_WATERBREATH] = "AFF_WATERBREATH";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SLOW] = "AFF_SLOW";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HASTE] = "AFF_HASTE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SHIELD] = "AFF_SHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AIRSHIELD] = "AFF_AIRSHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_FIRESHIELD] = "AFF_FIRESHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_ICESHIELD] = "AFF_ICESHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_MAGICGLASS] = "AFF_MAGICGLASS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STAIRS] = "AFF_STAIRS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STONEHAND] = "AFF_STONEHAND";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PRISMATICAURA] = "AFF_PRISMATICAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HELPER] = "AFF_HELPER";
	EAffectFlag_name_by_value[EAffectFlag::AFF_EVILESS] = "AFF_EVILESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AIRAURA] = "AFF_AIRAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_FIREAURA] = "AFF_FIREAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_ICEAURA] = "AFF_ICEAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DEAFNESS] = "AFF_DEAFNESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CRYING] = "AFF_CRYING";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PEACEFUL] = "AFF_PEACEFUL";
	EAffectFlag_name_by_value[EAffectFlag::AFF_MAGICSTOPFIGHT] = "AFF_MAGICSTOPFIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BERSERK] = "AFF_BERSERK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_LIGHT_WALK] = "AFF_LIGHT_WALK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BROKEN_CHAINS] = "AFF_BROKEN_CHAINS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CLOUD_OF_ARROWS] = "AFF_CLOUD_OF_ARROWS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SHADOW_CLOAK] = "AFF_SHADOW_CLOAK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_GLITTERDUST] = "AFF_GLITTERDUST";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AFFRIGHT] = "AFF_AFFRIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SCOPOLIA_POISON] = "AFF_SCOPOLIA_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DATURA_POISON] = "AFF_DATURA_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SKILLS_REDUCE] = "AFF_SKILLS_REDUCE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOT_SWITCH] = "AFF_NOT_SWITCH";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BELENA_POISON] = "AFF_BELENA_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOTELEPORT] = "AFF_NOTELEPORT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_LACKY] = "AFF_LACKY";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BANDAGE] = "AFF_BANDAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NO_BANDAGE] = "AFF_NO_BANDAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_MORPH] = "AFF_MORPH";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STRANGLED] = "AFF_STRANGLED";
	EAffectFlag_name_by_value[EAffectFlag::AFF_RECALL_SPELLS] = "AFF_RECALL_SPELLS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOOB_REGEN] = "AFF_NOOB_REGEN";
	EAffectFlag_name_by_value[EAffectFlag::AFF_VAMPIRE] = "AFF_VAMPIRE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_EXPEDIENT] = "AFF_EXPEDIENT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_COMMANDER] = "AFF_COMMANDER";
	EAffectFlag_name_by_value[EAffectFlag::AFF_EARTHAURA] = "AFF_EARTHAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DOMINATION] = "AFF_DOMINATION";
	for (const auto &i : EAffectFlag_name_by_value) {
		EAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM(const EAffectFlag item) {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_name_by_value.at(item);
}

template<>
EAffectFlag ITEM_BY_NAME(const std::string &name) {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_value_by_name.at(name);
}

typedef std::map<EWeaponAffectFlag, std::string> EWeaponAffectFlag_name_by_value_t;
typedef std::map<const std::string, EWeaponAffectFlag> EWeaponAffectFlag_value_by_name_t;
EWeaponAffectFlag_name_by_value_t EWeaponAffectFlag_name_by_value;
EWeaponAffectFlag_value_by_name_t EWeaponAffectFlag_value_by_name;

void init_EWeaponAffectFlag_ITEM_NAMES() {
	EWeaponAffectFlag_name_by_value.clear();
	EWeaponAffectFlag_value_by_name.clear();

	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLINDNESS] = "WAFF_BLINDNESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_INVISIBLE] = "WAFF_INVISIBLE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_ALIGN] = "WAFF_DETECT_ALIGN";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_INVISIBLE] = "WAFF_DETECT_INVISIBLE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_MAGIC] = "WAFF_DETECT_MAGIC";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SENSE_LIFE] = "WAFF_SENSE_LIFE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_WATER_WALK] = "WAFF_WATER_WALK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SANCTUARY] = "WAFF_SANCTUARY";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_CURSE] = "WAFF_CURSE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_INFRAVISION] = "WAFF_INFRAVISION";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_POISON] = "WAFF_POISON";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PROTECT_EVIL] = "WAFF_PROTECT_EVIL";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PROTECT_GOOD] = "WAFF_PROTECT_GOOD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SLEEP] = "WAFF_SLEEP";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_NOTRACK] = "WAFF_NOTRACK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLESS] = "WAFF_BLESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SNEAK] = "WAFF_SNEAK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HIDE] = "WAFF_HIDE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLD] = "WAFF_HOLD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FLY] = "WAFF_FLY";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SILENCE] = "WAFF_SILENCE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AWARENESS] = "WAFF_AWARENESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLINK] = "WAFF_BLINK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_NOFLEE] = "WAFF_NOFLEE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SINGLE_LIGHT] = "WAFF_SINGLE_LIGHT";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLY_LIGHT] = "WAFF_HOLY_LIGHT";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLY_DARK] = "WAFF_HOLY_DARK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_POISON] = "WAFF_DETECT_POISON";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SLOW] = "WAFF_SLOW";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HASTE] = "WAFF_HASTE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_WATER_BREATH] = "WAFF_WATER_BREATH";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HAEMORRAGIA] = "WAFF_HAEMORRAGIA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_CAMOUFLAGE] = "WAFF_CAMOUFLAGE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SHIELD] = "WAFF_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AIR_SHIELD] = "WAFF_AIR_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FIRE_SHIELD] = "WAFF_FIRE_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_ICE_SHIELD] = "WAFF_ICE_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_MAGIC_GLASS] = "WAFF_MAGIC_GLASS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_STONE_HAND] = "WAFF_STONE_HAND";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PRISMATIC_AURA] = "WAFF_PRISMATIC_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AIR_AURA] = "WAFF_AIR_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FIRE_AURA] = "WAFF_FIRE_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_ICE_AURA] = "WAFF_ICE_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DEAFNESS] = "WAFF_DEAFNESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_COMMANDER] = "WAFF_COMMANDER";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_EARTHAURA] = "WAFF_EARTHAURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DOMINATION] = "WAFF_DOMINATION";
	for (const auto &i : EWeaponAffectFlag_name_by_value) {
		EWeaponAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EWeaponAffectFlag ITEM_BY_NAME(const std::string &name) {
	if (EWeaponAffectFlag_name_by_value.empty()) {
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EWeaponAffectFlag item) {
	if (EWeaponAffectFlag_name_by_value.empty()) {
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_name_by_value.at(item);
}

weapon_affect_t weapon_affect = {
	weapon_affect_types{EWeaponAffectFlag::WAFF_BLINDNESS, 0, SPELL_BLINDNESS},
	weapon_affect_types{EWeaponAffectFlag::WAFF_INVISIBLE, to_underlying(EAffectFlag::AFF_INVISIBLE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_DETECT_ALIGN, to_underlying(EAffectFlag::AFF_DETECT_ALIGN), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_DETECT_INVISIBLE, to_underlying(EAffectFlag::AFF_DETECT_INVIS), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_DETECT_MAGIC, to_underlying(EAffectFlag::AFF_DETECT_MAGIC), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SENSE_LIFE, to_underlying(EAffectFlag::AFF_SENSE_LIFE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_WATER_WALK, to_underlying(EAffectFlag::AFF_WATERWALK), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SANCTUARY, to_underlying(EAffectFlag::AFF_SANCTUARY), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_CURSE, to_underlying(EAffectFlag::AFF_CURSE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_INFRAVISION, to_underlying(EAffectFlag::AFF_INFRAVISION), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_POISON, 0, SPELL_POISON},
	weapon_affect_types{EWeaponAffectFlag::WAFF_PROTECT_EVIL, to_underlying(EAffectFlag::AFF_PROTECT_EVIL), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_PROTECT_GOOD, to_underlying(EAffectFlag::AFF_PROTECT_GOOD), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SLEEP, 0, SPELL_SLEEP},
	weapon_affect_types{EWeaponAffectFlag::WAFF_NOTRACK, to_underlying(EAffectFlag::AFF_NOTRACK), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_BLESS, to_underlying(EAffectFlag::AFF_BLESS), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SNEAK, to_underlying(EAffectFlag::AFF_SNEAK), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_HIDE, to_underlying(EAffectFlag::AFF_HIDE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_HOLD, 0, SPELL_HOLD},
	weapon_affect_types{EWeaponAffectFlag::WAFF_FLY, to_underlying(EAffectFlag::AFF_FLY), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SILENCE, to_underlying(EAffectFlag::AFF_SILENCE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_AWARENESS, to_underlying(EAffectFlag::AFF_AWARNESS), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_BLINK, to_underlying(EAffectFlag::AFF_BLINK), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_NOFLEE, to_underlying(EAffectFlag::AFF_NOFLEE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SINGLE_LIGHT, to_underlying(EAffectFlag::AFF_SINGLELIGHT), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_HOLY_LIGHT, to_underlying(EAffectFlag::AFF_HOLYLIGHT), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_HOLY_DARK, to_underlying(EAffectFlag::AFF_HOLYDARK), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_DETECT_POISON, to_underlying(EAffectFlag::AFF_DETECT_POISON), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SLOW, to_underlying(EAffectFlag::AFF_SLOW), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_HASTE, to_underlying(EAffectFlag::AFF_HASTE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_WATER_BREATH, to_underlying(EAffectFlag::AFF_WATERBREATH), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_HAEMORRAGIA, to_underlying(EAffectFlag::AFF_HAEMORRAGIA), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_CAMOUFLAGE, to_underlying(EAffectFlag::AFF_CAMOUFLAGE), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_SHIELD, to_underlying(EAffectFlag::AFF_SHIELD), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_AIR_SHIELD, to_underlying(EAffectFlag::AFF_AIRSHIELD), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_FIRE_SHIELD, to_underlying(EAffectFlag::AFF_FIRESHIELD), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_ICE_SHIELD, to_underlying(EAffectFlag::AFF_ICESHIELD), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_MAGIC_GLASS, to_underlying(EAffectFlag::AFF_MAGICGLASS), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_STONE_HAND, to_underlying(EAffectFlag::AFF_STONEHAND), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_PRISMATIC_AURA, to_underlying(EAffectFlag::AFF_PRISMATICAURA), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_AIR_AURA, to_underlying(EAffectFlag::AFF_AIRAURA), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_FIRE_AURA, to_underlying(EAffectFlag::AFF_FIREAURA), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_ICE_AURA, to_underlying(EAffectFlag::AFF_ICEAURA), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_DEAFNESS, to_underlying(EAffectFlag::AFF_DEAFNESS), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_COMMANDER, to_underlying(EAffectFlag::AFF_COMMANDER), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_EARTHAURA, to_underlying(EAffectFlag::AFF_EARTHAURA), 0},
	weapon_affect_types{EWeaponAffectFlag::WAFF_DOMINATION, to_underlying(EAffectFlag::AFF_DOMINATION), 0}
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
