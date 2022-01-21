/*
	Константы-настройки ядра игровой системы.
*/

#ifndef BYLINS_SRC_ABILITIES_ABILITIES_CONSTANTS_H_
#define BYLINS_SRC_ABILITIES_ABILITIES_CONSTANTS_H_

namespace abilities {

/// Идентификаторы навыков
enum class EAbility {
	kUndefined,
	kBackstab,
	kKick,
	kThrowWeapon,
	kTurnUndead,
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

/// Идентификаторы основных параметров
/// todo Вынести в файл констант entities
enum class EStat {
	kStr,
	kDex,
	kCon,
	kInt,
	kWis,
	kCha
};

const int kNoviceSkillThreshold = 75;
const int kMinDegree = 0;
const int kMaxDegree = 10;
const double kSkillWeight = 1.0;
const double kNoviceSkillWeight = 0.75;
const double kParameterWeight = 3.0;
const double kSaveWeight = 1.0;

const int kMainDiceSize = 100;
const int kDamageDiceSize = 5;
const int kDamageDicepoolSkillDivider = 5;
const int kDefaultDifficulty = 0;
const int kMinAbilityDifficulty = -150;
const int kMaxAbilityDifficulty = 150;
const int kSuccessThreshold = 0;
const int kDefaultCriticalFailThreshold = 95;
const int kDefaultCriticalSuccessThreshold = 6;
const int kDefaultMobVsPcPenalty = 30;
const int kDefaultPcVsPcPenalty = 12;
const int kMaxFailDegree = -10;
const int kMaxSuccessDegree = 10;
const int kDegreeDivider = 10;
const int kSkillRatingDivider = 2;
const int kParameterRatingDivider = 2;
const int kCircumstanceFactor = 5;
constexpr int kMinimalFailResult = kSuccessThreshold - 1;

const int MIN_ABILITY_DICEROLL_BONUS = -150;
const int MAX_ABILITY_DICEROLL_BONUS = 150;

}; // namespace abilities

#endif // BYLINS_SRC_ABILITIES_ABILITIES_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
