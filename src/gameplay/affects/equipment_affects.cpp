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
const char *equipment_affects[] = {"слепота",
								"невидимость",
								"опр.наклонностей",
								"опр.невидимости",
								"опр.магии",
								"опр.жизни",
								"водохождение",
								"освящение",
								"проклятие",
								"инфравидение",
								"яд",
								"сопротивление.магии.тьмы",
								"сопротивление.магии.разума",
								"сон",
								"не.выследить",
								"доблесть",
								"подкрадывание",
								"спрятаться",
								"оцепенение",
								"полет",
								"молчание",
								"настороженность",
								"мигание",
								"не.сбежать",
								"свет",
								"освещение",
								"тьма",
								"опр.яда",
								"медлительность",
								"ускорение",
								"\n",
								"дыхание.водой",
								"кровотечение",
								"маскировка",
								"защита.богов",
								"воздушный.щит",
								"огненный.щит",
								"ледяной.щит",
								"зеркало.магии",
								"каменная.рука",
								"призматическая.аура",
								"воздушная.аура",
								"огненная.аура",
								"ледяная.аура",
								"глухота",
								"полководец",
								"земной.поклон",
								"затуманивание",
								"\n",
								"\n",
								"\n"
};

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
		for (auto &imp : node.Children()) {
			if (std::string(imp.GetName()) != "impose") {
				continue;
			}
			flag = parse::ReadAsConstant<EAffect>(imp.GetValue("flag"));
			spell = parse::ReadAsConstant<ESpell>(imp.GetValue("spell"));
		}
		table.push_back(EquipmentAffect{pos, flag, spell});
	}
	equipment_affect = std::move(table);
}

void EquipmentAffectsLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
