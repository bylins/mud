/**
 \brief Affects conferred by equipped items (the "weapon affect" flag set).
 NOTE: "weapon" is a historical DikuMUD misnomer -- these affects are applied by ANY worn
 equipment, not just weapons. Extracted from affect_contants.h (issue.equipment-affects):
 the EEquipmentAffect flag enum, the value->EAffect/spell table (equipment_affect) and the flag name maps.
*/

#ifndef BYLINS_SRC_AFFECTS_EQUIPMENT_AFFECTS_H_
#define BYLINS_SRC_AFFECTS_EQUIPMENT_AFFECTS_H_

#include "gameplay/magic/spells_constants.h"

#include <array>
#include <vector>
#include <utility>
#include <map>
#include <string>

enum class EEquipmentAffect : Bitvector {
	kBlindness = (1 << 0),			//0
	kInvisibility = (1 << 1),
	kDetectAlign = (1 << 2),
	kDetectInvisibility = (1 << 3),
	kDetectMagic = (1 << 4),
	kDetectLife = (1 << 5),
	kWaterWalk = (1 << 6),
	kSanctuary = (1 << 7),
	kCurse = (1 << 8),
	kInfravision = (1 << 9),
	kPoison = (1 << 10),			//10
	kProtectFromDark = (1 << 11),
	kProtectFromMind = (1 << 12),
	kSleep = (1 << 13),
	kNoTrack = (1 << 14),
	kBless = (1 << 15),
	kSneak = (1 << 16),
	kHide = (1 << 17),
	kHold = (1 << 18),
	kFly = (1 << 19),
	kSilence = (1 << 20),			//20
	kAwareness = (1 << 21),
	kBlink = (1 << 22),
	kNoFlee = (1 << 23),
	kSingleLight = (1 << 24),
	kHolyLight = (1 << 25),
	kHolyDark = (1 << 26),
	kDetectPoison = (1 << 27),
	kSlow = (1 << 28),
	kHaste = (1 << 29),
	kWaterBreath = kIntOne | (1 << 0),//30
	kHaemorrhage = kIntOne | (1 << 1),
	kDisguising = kIntOne | (1 << 2),
	kShield = kIntOne | (1 << 3),
	kAirShield = kIntOne | (1 << 4),
	kFireShield = kIntOne | (1 << 5),
	kIceShield = kIntOne | (1 << 6),
	kMagicGlass = kIntOne | (1 << 7),
	kStoneHand = kIntOne | (1 << 8),
	kPrismaticAura = kIntOne | (1 << 9),
	kAirAura = kIntOne | (1 << 10),		//40
	kFireAura = kIntOne | (1 << 11),
	kIceAura = kIntOne | (1 << 12),
	kDeafness = kIntOne | (1 << 13),
	kComamnder = kIntOne | (1 << 14),
	kEarthAura = kIntOne | (1 << 15),	//45
	kCloudly = kIntOne | (1 << 16)
// не забудьте поправить kEquipmentAffectCount
};

constexpr size_t kEquipmentAffectCount = 47;

template<>
EEquipmentAffect ITEM_BY_NAME<EEquipmentAffect>(const std::string &name);
// issue.flags-migration P1b: transitional packed storage.
template<>
struct flag_traits<EEquipmentAffect> {
	static constexpr std::size_t count = 120;
};
template<>
struct flag_index_mapping<EEquipmentAffect> {
	static constexpr std::size_t to_index(EEquipmentAffect f) {
		return bitset_flags_detail::packed_to_index(static_cast<std::uint32_t>(f));
	}
};
template<>
const std::string &NAME_BY_ITEM(EEquipmentAffect item);

enum class EAffect : Bitvector;   // forward decl (full def in affect_contants.h, which includes this header)
struct EquipmentAffect {
	EEquipmentAffect aff_pos;
	EAffect aff_affect;   // the affect this equipment flag confers (kUndefined = none, spell-driven)
	ESpell aff_spell;
};

// issue.equipment-affects-cfg: loaded from cfg/equipment_affects.xml at boot (was a hardcoded table).
extern std::vector<EquipmentAffect> equipment_affect;

// issue.equipment-affects-improve: the apply bridge (EEquipmentAffect -> {EApply, magnitude}) and
// the flag display names, moved here to complete the module. EApply is forward-declared -- it lives
// in affect_contants.h, which includes THIS header, so we cannot include it back.
enum EApply : int;
class CharData;
std::pair<EApply, int> GetApplyByEquipmentAffect(EEquipmentAffect element, CharData *ch);

// Russian display names for the flags (sprintbits labels), indexed by bit position.
// issue.equipment-affects-cfg: rebuilt at boot from cfg/messages/ru/equipment_affect_msg.xml
// (plane-padded, '\n'-separated -- the layout sprintbits still expects).
extern const char **equipment_affects;

#endif //BYLINS_SRC_AFFECTS_EQUIPMENT_AFFECTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
