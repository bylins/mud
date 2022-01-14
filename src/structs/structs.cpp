#include "structs.h"

#include "utils/utils.h"

void tascii(const uint32_t *pointer, int num_planes, char *ascii) {
	bool found = false;

	for (int i = 0; i < num_planes; i++) {
		for (int c = 0; c < 30; c++) {
			if (pointer[i] & (1 << c)) {
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

const char* nothing_string = "ничего";

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag) {

	long nr = 0;
	int fail;
	int plane = 0;
	char c = 'a';

	*result = '\0';

	fail = bitvector >> 30;
	bitvector &= 0x3FFFFFFF;
	while (fail) {
		if (*names[nr] == '\n') {
			fail--;
			plane++;
		}
		nr++;
	}

	bool can_show;
	for (; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			can_show = ((*names[nr] != '*') || (print_flag & 4));

			if (*result != '\0' && can_show)
				strcat(result, div);

			if (*names[nr] != '\n') {
				if (print_flag & 1) {
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				if ((print_flag & 2) && (!strcmp(names[nr], "UNUSED"))) {
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				if (can_show)
					strcat(result, (*names[nr] != '*' ? names[nr] : names[nr] + 1));
			} else {
				if (print_flag & 2) {
					sprintf(result + strlen(result), "%ld:", nr + 1);
				} else if (print_flag & 1) {
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				strcat(result, "UNDEF");
			}
		}

		if (print_flag & 1) {
			c++;
			if (c > 'z') {
				c = 'A';
			}
		}
		if (*names[nr] != '\n') {
			nr++;
		}
	}

	if ('\0' == *result) {
		strcat(result, nothing_string);
		return false;
	}

	return true;
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

typedef std::map<EApplyLocation, std::string> EApplyLocation_name_by_value_t;
typedef std::map<const std::string, EApplyLocation> EApplyLocation_value_by_name_t;
EApplyLocation_name_by_value_t EApplyLocation_name_by_value;
EApplyLocation_value_by_name_t EApplyLocation_value_by_name;

void init_EApplyLocation_ITEM_NAMES() {
	EApplyLocation_name_by_value.clear();
	EApplyLocation_value_by_name.clear();

	EApplyLocation_name_by_value[EApplyLocation::APPLY_NONE] = "APPLY_NONE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_STR] = "APPLY_STR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DEX] = "APPLY_DEX";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_INT] = "APPLY_INT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_WIS] = "APPLY_WIS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CON] = "APPLY_CON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHA] = "APPLY_CHA";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CLASS] = "APPLY_CLASS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_LEVEL] = "APPLY_LEVEL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AGE] = "APPLY_AGE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHAR_WEIGHT] = "APPLY_CHAR_WEIGHT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHAR_HEIGHT] = "APPLY_CHAR_HEIGHT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MANAREG] = "APPLY_MANAREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HIT] = "APPLY_HIT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MOVE] = "APPLY_MOVE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_GOLD] = "APPLY_GOLD";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_EXP] = "APPLY_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AC] = "APPLY_AC";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HITROLL] = "APPLY_HITROLL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DAMROLL] = "APPLY_DAMROLL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_WILL] = "APPLY_SAVING_WILL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_FIRE] = "APPLY_RESIST_FIRE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_AIR] = "APPLY_RESIST_AIR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_DARK] = "APPLY_RESIST_DARK";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_CRITICAL] = "APPLY_SAVING_CRITICAL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_STABILITY] = "APPLY_SAVING_STABILITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HITREG] = "APPLY_HITREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MOVEREG] = "APPLY_MOVEREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C1] = "APPLY_C1";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C2] = "APPLY_C2";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C3] = "APPLY_C3";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C4] = "APPLY_C4";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C5] = "APPLY_C5";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C6] = "APPLY_C6";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C7] = "APPLY_C7";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C8] = "APPLY_C8";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C9] = "APPLY_C9";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SIZE] = "APPLY_SIZE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ARMOUR] = "APPLY_ARMOUR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_POISON] = "APPLY_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_REFLEX] = "APPLY_SAVING_REFLEX";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CAST_SUCCESS] = "APPLY_CAST_SUCCESS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MORALE] = "APPLY_MORALE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_INITIATIVE] = "APPLY_INITIATIVE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RELIGION] = "APPLY_RELIGION";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ABSORBE] = "APPLY_ABSORBE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_LIKES] = "APPLY_LIKES";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_WATER] = "APPLY_RESIST_WATER";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_EARTH] = "APPLY_RESIST_EARTH";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_VITALITY] = "APPLY_RESIST_VITALITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_MIND] = "APPLY_RESIST_MIND";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_IMMUNITY] = "APPLY_RESIST_IMMUNITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AR] = "APPLY_AR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MR] = "APPLY_MR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ACONITUM_POISON] = "APPLY_ACONITUM_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SCOPOLIA_POISON] = "APPLY_SCOPOLIA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BELENA_POISON] = "APPLY_BELENA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DATURA_POISON] = "APPLY_DATURA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_FREE_FOR_USE] = "APPLY_FREE_FOR_USE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BONUS_EXP] = "APPLY_BONUS_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BONUS_SKILLS] = "APPLY_BONUS_SKILLS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PLAQUE] = "APPLY_PLAQUE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MADNESS] = "APPLY_MADNESS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PR] = "APPLY_PR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_DARK] = "APPLY_RESIST_DARK";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_VIEW_DT] = "APPLY_VIEW_DT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_EXP] = "APPLY_PERCENT_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_DAM] = "APPLY_PERCENT_DAM";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SPELL_BLINK] = "APPLY_SPELL_BLINK";
	EApplyLocation_name_by_value[EApplyLocation::NUM_APPLIES] = "NUM_APPLIES";
	for (const auto &i : EApplyLocation_name_by_value) {
		EApplyLocation_value_by_name[i.second] = i.first;
	}
}

template<>
EApplyLocation ITEM_BY_NAME(const std::string &name) {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EApplyLocation item) {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_name_by_value.at(item);
}

void DelegatedStringWriter::set_string(const char *string) {
	const size_t l = strlen(string);
	if (nullptr == m_delegated_string) {
		CREATE(m_delegated_string, l + 1);
	} else {
		RECREATE(m_delegated_string, l + 1);
	}
	strcpy(m_delegated_string, string);
}

void DelegatedStringWriter::append_string(const char *string) {
	const size_t l = length() + strlen(string);
	if (nullptr == m_delegated_string) {
		CREATE(m_delegated_string, l + 1);
		*m_delegated_string = '\0';
	} else {
		RECREATE(m_delegated_string, l + 1);
	}
	strcat(m_delegated_string, string);
}

void DelegatedStringWriter::clear() {
	if (m_delegated_string) {
		free(m_delegated_string);
	}
	m_delegated_string = nullptr;
}

void EXTRA_DESCR_DATA::set_keyword(std::string const &value) {
	if (keyword != nullptr)
		free(keyword);
	keyword = str_dup(value.c_str());
}

void EXTRA_DESCR_DATA::set_description(std::string const &value) {
	if (description != nullptr)
		free(description);
	description = str_dup(value.c_str());
}

EXTRA_DESCR_DATA::~EXTRA_DESCR_DATA() {
	if (nullptr != keyword) {
		free(keyword);
	}

	if (nullptr != description) {
		free(description);
	}

	// we don't take care of items in list. So, we don't do anything with the next field.
}

punish_data::punish_data() : duration(0), reason(nullptr), level(0), godid(0) {
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
