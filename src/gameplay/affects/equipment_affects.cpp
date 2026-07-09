/**
 \brief Equipment-affect flag maps and the equipment_affect value->EAffect/spell table.
 Extracted verbatim from affect_contants.cpp (issue.equipment-affects). The name maps and lazy
 init are TU-local; ITEM_BY_NAME/NAME_BY_ITEM are the public lookups declared in the header.
*/

#include "gameplay/affects/equipment_affects.h"

#include "gameplay/affects/affect_contants.h"   // EAffect (for the equipment_affect table)
#include "utils/utils.h"                         // to_underlying

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

EquipmentAffectArray equipment_affect = {
	EquipmentAffect{EEquipmentAffect::kBlindness, EAffect::kUndefined, ESpell::kBlindness},
	EquipmentAffect{EEquipmentAffect::kInvisibility, EAffect::kInvisible, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDetectAlign, EAffect::kDetectAlign, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDetectInvisibility, EAffect::kDetectInvisible, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDetectMagic, EAffect::kDetectMagic, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDetectLife, EAffect::kDetectLife, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kWaterWalk, EAffect::kWaterWalk, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kSanctuary, EAffect::kSanctuary, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kCurse, EAffect::kCurse, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kInfravision, EAffect::kInfravision, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kPoison, EAffect::kUndefined, ESpell::kPoison},
	EquipmentAffect{EEquipmentAffect::kProtectFromDark, EAffect::kProtectFromDark, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kProtectFromMind, EAffect::kProtectFromMind, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kSleep, EAffect::kUndefined, ESpell::kSleep},
	EquipmentAffect{EEquipmentAffect::kNoTrack, EAffect::kNoTrack, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kBless, EAffect::kBless, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kSneak, EAffect::kSneak, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kHide, EAffect::kHide, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kHold, EAffect::kUndefined, ESpell::kHold},
	EquipmentAffect{EEquipmentAffect::kFly, EAffect::kFly, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kSilence, EAffect::kSilence, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kAwareness, EAffect::kAwarness, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kBlink, EAffect::kBlink, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kNoFlee, EAffect::kNoFlee, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kSingleLight, EAffect::kSingleLight, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kHolyLight, EAffect::kHolyLight, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kHolyDark, EAffect::kHolyDark, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDetectPoison, EAffect::kDetectPoison, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kSlow, EAffect::kSlow, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kHaste, EAffect::kHaste, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kWaterBreath, EAffect::kWaterBreath, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kHaemorrhage, EAffect::kHaemorrhage, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDisguising, EAffect::kDisguise, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kShield, EAffect::kGodsShield, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kAirShield, EAffect::kAirShield, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kFireShield, EAffect::kFireShield, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kIceShield, EAffect::kIceShield, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kMagicGlass, EAffect::kMagicGlass, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kStoneHand, EAffect::kStoneHands, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kPrismaticAura, EAffect::kPrismaticAura, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kAirAura, EAffect::kAirAura, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kFireAura, EAffect::kFireAura, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kIceAura, EAffect::kIceAura, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kDeafness, EAffect::kDeafness, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kComamnder, EAffect::kCommander, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kEarthAura, EAffect::kEarthAura, ESpell::kUndefined},
	EquipmentAffect{EEquipmentAffect::kCloudly, EAffect::kCloudly, ESpell::kUndefined}
};

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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
