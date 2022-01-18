/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#ifndef BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_
#define BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

#include "structs/structs.h"

// Affect bits: used in char_data.char_specials.saved.affected_by //
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") //
enum class EAffectFlag : bitvector_t {
	AFF_BLIND = 1u << 0,                    ///< (R) Char is blind
	AFF_INVISIBLE = 1u << 1,                ///< Char is invisible
	AFF_DETECT_ALIGN = 1u << 2,                ///< Char is sensitive to align
	AFF_DETECT_INVIS = 1u << 3,                ///< Char can see invis entities
	AFF_DETECT_MAGIC = 1u << 4,                ///< Char is sensitive to magic
	AFF_SENSE_LIFE = 1u << 5,                ///< Char can sense hidden life
	AFF_WATERWALK = 1u << 6,                ///< Char can walk on water
	AFF_SANCTUARY = 1u << 7,                ///< Char protected by sanct.
	AFF_GROUP = 1u << 8,                    ///< (R) Char is grouped
	AFF_CURSE = 1u << 9,                    ///< Char is cursed
	AFF_INFRAVISION = 1u << 10,                ///< Char can see in dark
	AFF_POISON = 1u << 11,                    ///< (R) Char is poisoned
	AFF_PROTECT_EVIL = 1u << 12,            ///< Char protected from evil
	AFF_PROTECT_GOOD = 1u << 13,            ///< Char protected from good
	AFF_SLEEP = 1u << 14,                    ///< (R) Char magically asleep
	AFF_NOTRACK = 1u << 15,                    ///< Char can't be tracked
	AFF_TETHERED = 1u << 16,                ///< Room for future expansion
	AFF_BLESS = 1u << 17,                    ///< Room for future expansion
	AFF_SNEAK = 1u << 18,                    ///< Char can move quietly
	AFF_HIDE = 1u << 19,                    ///< Char is hidden
	AFF_COURAGE = 1u << 20,                    ///< Room for future expansion
	AFF_CHARM = 1u << 21,                    ///< Char is charmed
	AFF_HOLD = 1u << 22,
	AFF_FLY = 1u << 23,
	AFF_SILENCE = 1u << 24,
	AFF_AWARNESS = 1u << 25,
	AFF_BLINK = 1u << 26,
	AFF_HORSE = 1u << 27,                    ///< NPC - is horse, PC - is horsed
	AFF_NOFLEE = 1u << 28,
	AFF_SINGLELIGHT = 1u << 29,
	AFF_HOLYLIGHT = kIntOne | (1u << 0),
	AFF_HOLYDARK = kIntOne | (1u << 1),
	AFF_DETECT_POISON = kIntOne | (1u << 2),
	AFF_DRUNKED = kIntOne | (1u << 3),
	AFF_ABSTINENT = kIntOne | (1u << 4),
	AFF_STOPRIGHT = kIntOne | (1u << 5),
	AFF_STOPLEFT = kIntOne | (1u << 6),
	AFF_STOPFIGHT = kIntOne | (1u << 7),
	AFF_HAEMORRAGIA = kIntOne | (1u << 8),
	AFF_CAMOUFLAGE = kIntOne | (1u << 9),
	AFF_WATERBREATH = kIntOne | (1u << 10),
	AFF_SLOW = kIntOne | (1u << 11),
	AFF_HASTE = kIntOne | (1u << 12),
	AFF_SHIELD = kIntOne | (1u << 13),
	AFF_AIRSHIELD = kIntOne | (1u << 14),
	AFF_FIRESHIELD = kIntOne | (1u << 15),
	AFF_ICESHIELD = kIntOne | (1u << 16),
	AFF_MAGICGLASS = kIntOne | (1u << 17),
	AFF_STAIRS = kIntOne | (1u << 18),
	AFF_STONEHAND = kIntOne | (1u << 19),
	AFF_PRISMATICAURA = kIntOne | (1u << 20),
	AFF_HELPER = kIntOne | (1u << 21),
	AFF_EVILESS = kIntOne | (1u << 22),
	AFF_AIRAURA = kIntOne | (1u << 23),
	AFF_FIREAURA = kIntOne | (1u << 24),
	AFF_ICEAURA = kIntOne | (1u << 25),
	AFF_DEAFNESS = kIntOne | (1u << 26),
	AFF_CRYING = kIntOne | (1u << 27),
	AFF_PEACEFUL = kIntOne | (1u << 28),
	AFF_MAGICSTOPFIGHT = kIntOne | (1u << 29),
	AFF_BERSERK = kIntTwo | (1u << 0),
	AFF_LIGHT_WALK = kIntTwo | (1u << 1),
	AFF_BROKEN_CHAINS = kIntTwo | (1u << 2),
	AFF_CLOUD_OF_ARROWS = kIntTwo | (1u << 3),
	AFF_SHADOW_CLOAK = kIntTwo | (1u << 4),
	AFF_GLITTERDUST = kIntTwo | (1u << 5),
	AFF_AFFRIGHT = kIntTwo | (1u << 6),
	AFF_SCOPOLIA_POISON = kIntTwo | (1u << 7),
	AFF_DATURA_POISON = kIntTwo | (1u << 8),
	AFF_SKILLS_REDUCE = kIntTwo | (1u << 9),
	AFF_NOT_SWITCH = kIntTwo | (1u << 10),
	AFF_BELENA_POISON = kIntTwo | (1u << 11),
	AFF_NOTELEPORT = kIntTwo | (1u << 12),
	AFF_LACKY = kIntTwo | (1u << 13),
	AFF_BANDAGE = kIntTwo | (1u << 14),
	AFF_NO_BANDAGE = kIntTwo | (1u << 15),
	AFF_MORPH = kIntTwo | (1u << 16),
	AFF_STRANGLED = kIntTwo | (1u << 17),
	AFF_RECALL_SPELLS = kIntTwo | (1u << 18),
	AFF_NOOB_REGEN = kIntTwo | (1u << 19),
	AFF_VAMPIRE = kIntTwo | (1u << 20),
	AFF_EXPEDIENT = kIntTwo | (1u << 21),
	AFF_COMMANDER = kIntTwo | (1u << 22),
	AFF_EARTHAURA = kIntTwo | (1u << 23),
	AFF_DOMINATION = kIntTwo | (1u << 24)
};

template<>
const std::string &NAME_BY_ITEM<EAffectFlag>(EAffectFlag item);
template<>
EAffectFlag ITEM_BY_NAME<EAffectFlag>(const std::string &name);

typedef std::list<EAffectFlag> affects_list_t;


enum class EWeaponAffectFlag : bitvector_t {
	WAFF_BLINDNESS = (1 << 0),
	WAFF_INVISIBLE = (1 << 1),
	WAFF_DETECT_ALIGN = (1 << 2),
	WAFF_DETECT_INVISIBLE = (1 << 3),
	WAFF_DETECT_MAGIC = (1 << 4),
	WAFF_SENSE_LIFE = (1 << 5),
	WAFF_WATER_WALK = (1 << 6),
	WAFF_SANCTUARY = (1 << 7),
	WAFF_CURSE = (1 << 8),
	WAFF_INFRAVISION = (1 << 9),
	WAFF_POISON = (1 << 10),
	WAFF_PROTECT_EVIL = (1 << 11),
	WAFF_PROTECT_GOOD = (1 << 12),
	WAFF_SLEEP = (1 << 13),
	WAFF_NOTRACK = (1 << 14),
	WAFF_BLESS = (1 << 15),
	WAFF_SNEAK = (1 << 16),
	WAFF_HIDE = (1 << 17),
	WAFF_HOLD = (1 << 18),
	WAFF_FLY = (1 << 19),
	WAFF_SILENCE = (1 << 20),
	WAFF_AWARENESS = (1 << 21),
	WAFF_BLINK = (1 << 22),
	WAFF_NOFLEE = (1 << 23),
	WAFF_SINGLE_LIGHT = (1 << 24),
	WAFF_HOLY_LIGHT = (1 << 25),
	WAFF_HOLY_DARK = (1 << 26),
	WAFF_DETECT_POISON = (1 << 27),
	WAFF_SLOW = (1 << 28),
	WAFF_HASTE = (1 << 29),
	WAFF_WATER_BREATH = kIntOne | (1 << 0),
	WAFF_HAEMORRAGIA = kIntOne | (1 << 1),
	WAFF_CAMOUFLAGE = kIntOne | (1 << 2),
	WAFF_SHIELD = kIntOne | (1 << 3),
	WAFF_AIR_SHIELD = kIntOne | (1 << 4),
	WAFF_FIRE_SHIELD = kIntOne | (1 << 5),
	WAFF_ICE_SHIELD = kIntOne | (1 << 6),
	WAFF_MAGIC_GLASS = kIntOne | (1 << 7),
	WAFF_STONE_HAND = kIntOne | (1 << 8),
	WAFF_PRISMATIC_AURA = kIntOne | (1 << 9),
	WAFF_AIR_AURA = kIntOne | (1 << 10),
	WAFF_FIRE_AURA = kIntOne | (1 << 11),
	WAFF_ICE_AURA = kIntOne | (1 << 12),
	WAFF_DEAFNESS = kIntOne | (1 << 13),
	WAFF_COMMANDER = kIntOne | (1 << 14),
	WAFF_EARTHAURA = kIntOne | (1 << 15),
	WAFF_DOMINATION = kIntOne | (1 << 16),
// не забудьте поправить WAFF_COUNT
};

constexpr size_t WAFF_COUNT = 47;

template<>
EWeaponAffectFlag ITEM_BY_NAME<EWeaponAffectFlag>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(EWeaponAffectFlag item);

struct weapon_affect_types {
	EWeaponAffectFlag aff_pos;
	uint32_t aff_bitvector;
	int aff_spell;
};

using weapon_affect_t = std::array<weapon_affect_types, WAFF_COUNT>;
extern weapon_affect_t weapon_affect;
extern const char *affected_bits[];

#endif //BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
