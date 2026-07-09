/**
 \brief Equipment-affect flag maps and the equipment_affect value->EAffect/spell table.
 Extracted verbatim from affect_contants.cpp (issue.equipment-affects). The name maps and lazy
 init are TU-local; ITEM_BY_NAME/NAME_BY_ITEM are the public lookups declared in the header.
*/

#include "gameplay/affects/equipment_affects.h"

#include "gameplay/affects/affect_contants.h"   // EAffect (for the equipment_affect table)
#include "utils/utils.h"                         // to_underlying
#include "gameplay/affects/equipment_affects_loader.h"
#include "utils/utils_parse.h"

typedef std::map<EEquipmentAffect, std::string> EEquipmentAffectFlag_name_by_value_t;
typedef std::map<const std::string, EEquipmentAffect> EEquipmentAffectFlag_value_by_name_t;
EEquipmentAffectFlag_name_by_value_t EEquipmentAffectFlag_name_by_value;
EEquipmentAffectFlag_value_by_name_t EEquipmentAffectFlag_value_by_name;

void init_EEquipmentAffectFlag_ITEM_NAMES() {
	EEquipmentAffectFlag_name_by_value.clear();
	EEquipmentAffectFlag_value_by_name.clear();

	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kBlindness] = "kBlindness";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kInvisibility] = "kInvisibility";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDetectAlign] = "kDetectAlign";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDetectInvisibility] = "kDetectInvisibility";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDetectMagic] = "kDetectMagic";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDetectLife] = "kDetectLife";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kWaterWalk] = "kWaterWalk";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kSanctuary] = "kSanctuary";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kCurse] = "kCurse";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kInfravision] = "kInfravision";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kPoison] = "kPoison";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kProtectFromDark] = "kProtectFromDark";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kProtectFromMind] = "kProtectFromMind";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kSleep] = "kSleep";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kNoTrack] = "kNoTrack";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kBless] = "kBless";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kSneak] = "kSneak";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kHide] = "kHide";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kHold] = "kHold";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kFly] = "kFly";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kSilence] = "kSilence";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kAwareness] = "kAwareness";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kBlink] = "kBlink";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kNoFlee] = "kNoFlee";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kSingleLight] = "kSingleLight";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kHolyLight] = "kHolyLight";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kHolyDark] = "kHolyDark";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDetectPoison] = "kDetectPoison";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kSlow] = "kSlow";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kHaste] = "kHaste";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kWaterBreath] = "kWaterBreath";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kHaemorrhage] = "kHaemorrhage";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDisguising] = "kDisguising";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kShield] = "kShield";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kAirShield] = "kAirShield";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kFireShield] = "kFireShield";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kIceShield] = "kIceShield";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kMagicGlass] = "kMagicGlass";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kStoneHand] = "kStoneHand";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kPrismaticAura] = "kPrismaticAura";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kAirAura] = "kAirAura";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kFireAura] = "kFireAura";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kIceAura] = "kIceAura";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kDeafness] = "kDeafness";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kComamnder] = "kComamnder";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kEarthAura] = "kEarthAura";
	EEquipmentAffectFlag_name_by_value[EEquipmentAffect::kCloudly] = "kCloudly";
	for (const auto &i : EEquipmentAffectFlag_name_by_value) {
		EEquipmentAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EEquipmentAffect ITEM_BY_NAME(const std::string &name) {
	if (EEquipmentAffectFlag_name_by_value.empty()) {
		init_EEquipmentAffectFlag_ITEM_NAMES();
	}
	return EEquipmentAffectFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EEquipmentAffect item) {
	if (EEquipmentAffectFlag_name_by_value.empty()) {
		init_EEquipmentAffectFlag_ITEM_NAMES();
	}
	return EEquipmentAffectFlag_name_by_value.at(item);
}

// issue.equipment-affects-cfg: populated by EquipmentAffectsLoader from cfg/equipment_affects.xml.
std::vector<EquipmentAffect> equipment_affect;

// issue.equipment-affects-improve: apply bridge moved verbatim from affect_data.cpp.
std::pair<EApply, int>  GetApplyByEquipmentAffect(EEquipmentAffect element, CharData *ch) {
	int value;
	if (ch) //чтоб не было варнинга, ch передаю на будущее
		value = 2;
	switch (element) {
		case EEquipmentAffect::kFireAura:
			return std::pair<EApply, int>(EApply::kResistFire, value);
			break;
		case EEquipmentAffect::kAirAura:
			return std::pair<EApply, int>(EApply::kResistAir, value);
			break;
		case EEquipmentAffect::kIceAura:
			return std::pair<EApply, int>(EApply::kResistWater, value);
			break;
		case EEquipmentAffect::kEarthAura:
			return std::pair<EApply, int>(EApply::kResistEarth, value);
			break;
		case EEquipmentAffect::kProtectFromDark:
			return std::pair<EApply, int>(EApply::kResistDark, value);
			break;
		case EEquipmentAffect::kProtectFromMind:
			return std::pair<EApply, int>(EApply::kResistMind, value);
			break;
		// issue.mob-flag-affect-materialization: worn cloudly/blink must grant the miss-chance APPLY,
		// not just the flag -- ProcessBlink now gates on the apply, not AFF_FLAGGED. Flat 10 preserves
		// the pre-change PC value (the old hardcoded flag path defaulted a flagged PC to blink 10); NPC
		// bearers still take level+remort from ProcessBlink regardless of magnitude.
		case EEquipmentAffect::kCloudly:
			return std::pair<EApply, int>(EApply::kSpelledBlinkMag, 10);
			break;
		case EEquipmentAffect::kBlink:
			return std::pair<EApply, int>(EApply::kSpelledBlinkPhys, 10);
			break;
		default:
			return std::pair<EApply, int>(EApply::kNone, 0);
			break;
	}
}

// issue.equipment-affects-improve: flag display names moved from constants.cpp.
// issue.equipment-affects-cfg: display names, loaded from equipment_affect_msg.xml (was hardcoded).
static std::vector<std::string> g_equipment_affect_names;
static std::vector<const char *> g_equipment_affect_name_ptrs;
// issue.equipment-affects-improve: a valid, empty, plane-padded fallback so the name array is never
// null when cfg/messages/ru/equipment_affect_msg.xml is missing/unparsed. disp_planes_values and
// sprintbitwd dereference names[] directly (no null guard), so oedit/stat/identify on a world without
// the message file would segfault. NUM_PLANES (4) '\n' terminators match the built array's shape; a
// set bit against it renders as "UNDEF" instead of crashing. The message loader overwrites this on load.
static const char *g_empty_equipment_affect_names[] = {"\n", "\n", "\n", "\n"};
const char **equipment_affects = g_empty_equipment_affect_names;

// issue.equipment-affects-cfg: build the equipment_affect table from cfg/equipment_affects.xml.
void EquipmentAffectsLoader::Load(parser_wrapper::DataNode data) {
	std::vector<EquipmentAffect> table;
	for (auto &node : data.Children()) {
		if (std::string(node.GetName()) != "affect") {
			continue;
		}
		const EEquipmentAffect pos = ITEM_BY_NAME<EEquipmentAffect>(node.GetValue("id"));
		EAffect flag = EAffect::kUndefined;
		ESpell spell = ESpell::kUndefined;
		int timer = kEquipmentAffectNoTimer;
		for (auto &imp : node.Children()) {
			if (std::string(imp.GetName()) != "impose") {
				continue;
			}
			flag = parse::ReadAsConstant<EAffect>(imp.GetValue("flag"));
			spell = parse::ReadAsConstant<ESpell>(imp.GetValue("spell"));
			const char *t = imp.GetValue("timer");
			if (t && *t) {
				timer = parse::ReadAsInt(t);
			}
		}
		table.push_back(EquipmentAffect{pos, flag, spell, timer});
	}
	equipment_affect = std::move(table);
}

void EquipmentAffectsLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

// issue.equipment-affects-cfg: rebuild the plane-padded equipment_affects[] name array from the flat
// message file. Per plane (4x30 legacy bits) append each present affect's name in bit order, then a
// '\n' plane terminator -- byte-identical to the old hardcoded array (verified).
void EquipmentAffectMsgLoader::Load(parser_wrapper::DataNode data) {
	std::map<EEquipmentAffect, std::string> names;
	for (auto &node : data.Children()) {
		if (std::string(node.GetName()) != "affect") {
			continue;
		}
		const EEquipmentAffect id = ITEM_BY_NAME<EEquipmentAffect>(node.GetValue("id"));
		for (auto &nm : node.Children()) {
			if (std::string(nm.GetName()) == "name") {
				names[id] = nm.GetValue("val");
			}
		}
	}
	std::vector<std::string> arr;
	for (int plane = 0; plane < 4; ++plane) {
		for (int bit = 0; bit < 30; ++bit) {
			const auto af = static_cast<EEquipmentAffect>(
					(static_cast<Bitvector>(plane) << 30) | (static_cast<Bitvector>(1) << bit));
			const auto it = names.find(af);
			if (it != names.end()) {
				arr.push_back(it->second);
			}
		}
		arr.push_back("\n");
	}
	g_equipment_affect_names = std::move(arr);
	g_equipment_affect_name_ptrs.clear();
	g_equipment_affect_name_ptrs.reserve(g_equipment_affect_names.size());
	for (const auto &nm : g_equipment_affect_names) {
		g_equipment_affect_name_ptrs.push_back(nm.c_str());
	}
	equipment_affects = g_equipment_affect_name_ptrs.data();
}

void EquipmentAffectMsgLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
