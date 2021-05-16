#ifndef _ABILITY_CONSTANTS_H_
#define _ABILITY_CONSTANTS_H_

/*
	Константы-настройки ядра игровой системы.
*/

namespace abilities {
	const short kMainDiceSize = 100;
	const short kDamageDiceSize = 5;
	const short kDamageDicepoolSkillDivider = 5;
	const short kMinAbilityDicerollBonus = -150;
	const short kMaxAbilityDicerollBonus = 150;
	const short kSuccessThreshold = 0;
	const short kDefaultCriticalFailThreshold = 95;
	const short kDefaultCriticalSuccessThreshold = 6;
	const short kMaxFailDegree = -10;
	const short kMaxSuccessDegree = 10;
	const short kDegreeDivider = 10;
	const short kSkillRatingDivider = 2;
	const short kParameterRatingDivider = 2;
	const short kSituationableFactor = 5;
	constexpr short kMinimalFailResult = kSuccessThreshold - 1;
}; // namespace abilities

#endif // _ABILITY_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
