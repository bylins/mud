/*
	Константы-настройки навыков (abilities) и тестов навыков.
*/

#ifndef BYLINS_SRC_ABILITIES_ABILITIES_CONSTANTS_H_
#define BYLINS_SRC_ABILITIES_ABILITIES_CONSTANTS_H_

#include "structs/structs.h"

namespace abilities {

/// Идентификаторы навыков
enum class EAbility {
	kUndefined,
	kBackstab,
	kKick,
	kThrowWeapon,
	kTurnUndead,
	kLastPCAbility,
	// Below abilities is for mob or game events only.
	kTriggerDamage,
	kUnderwater,
	kSlowDeathTrap,
	kOneRoomDamage
};

/// Идентификаторы типов сообщений навыков
enum class EAbilityMsg {
	kBasicMsgTry,
	kKillMsgToDamager,
	kKillMsgToVictim,
	kKillMsgToOnlookers,
	kMissMsgToDamager,
	kMissMsgToVictim,
	kMissMsgToOnlookers,
	kDmgMsgToDamager,
	kDmgMsgToVictim,
	kDmgMsgToOnlookers,
	kDmgGodMsgToDamager,
	kDmgGodMsgToVictim,
	kDmgGodMsgToOnlookers,
	kDenyMsgHaveNoAbility,
	kDenyMsgNoVictim,
	kDenyMsgOnHorse,
	kDenyMsgAbilityCooldown,
	kDenyMsgActorIsVictim,
	kDenyMsgHaveNoWeapon,
	kDenyMsgWrongWeapon,
	kDenyMsgWrongVictim,
	kDenyMsgWrongPosition,
	kDenyMsgIsNotAbleToAct
};

/// Идентификаторы бонусов/штрафов обстоятельств (темная комната, жертва не видит атакующего и т.п.)
enum class ECirumstance {
	kDarkRoom,
	kMetalEquipment,
	kDrawingAttention,		// Персонаж под аффектами, типа света, освящения и так далее.
	kAmbushAttack,			// Цель не видит атакующего в момент атаки.
	kVictimSits,
	kVictimBashed,			// Позиция противника меньше, чем POS_FIGHTING.
	kVictimAwareness,		// Жертва под аффектом "настороженность".
	kVictimAwake,
	kVictimHold,
	kVictimSleep,
	kRoomInside,
	kRoomCity,
	kRoomForest,
	kRoomHills,
	kRoomMountain,
	kWeatherRaining,
	kWeatherLighting
};

const int kNoviceSkillThreshold = 75;
const int kMinDegree = 0;
const int kMaxDegree = 10;
const double kSkillWeight = 1.0;
const double kNoviceSkillWeight = 0.75;
const double kParameterWeight = 3.0;
const double kSaveWeight = 1.0;

const int kMainDiceSize = 100;
const int kDmgDiceSize = 5;
const int kDmgDicepoolSkillDivider = 5;
const int kDefaultDifficulty = 0;
const int kMinAbilityDifficulty = -150;
const int kMaxAbilityDifficulty = 150;
const int kSuccessThreshold = 0;
const int kDefaultCritfailThreshold = 95;
const int kDefaultCritsuccessThreshold = 6;
const int kDefaultMvPPenalty = 30;
const int kDefaultPvPPenalty = 12;
const int kMaxFailDegree = -10;
const int kMaxSuccessDegree = 10;
const int kDegreeDivider = 10;
const int kSkillRatingDivider = 2;
const int kStatRatingDivider = 2;
const int kCircumstanceFactor = 5;
constexpr int kMinimalFailResult = kSuccessThreshold - 1;

const int MIN_ABILITY_DICEROLL_BONUS = -150;
const int MAX_ABILITY_DICEROLL_BONUS = 150;

}; // namespace abilities

template<>
const std::string &NAME_BY_ITEM<abilities::EAbility>(abilities::EAbility item);
template<>
abilities::EAbility ITEM_BY_NAME<abilities::EAbility>(const std::string &name);

template<>
const std::string &NAME_BY_ITEM<abilities::EAbilityMsg>(abilities::EAbilityMsg item);
template<>
abilities::EAbilityMsg ITEM_BY_NAME<abilities::EAbilityMsg>(const std::string &name);

template<>
const std::string &NAME_BY_ITEM<abilities::ECirumstance>(abilities::ECirumstance item);
template<>
abilities::ECirumstance ITEM_BY_NAME<abilities::ECirumstance>(const std::string &name);

#endif // BYLINS_SRC_ABILITIES_ABILITIES_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
