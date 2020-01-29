#ifndef _ABILITY_CONSTANTS_HPP_INCLUDED_
#define _ABILITY_CONSTANTS_HPP_INCLUDED_

/*
	Константы-настройки ядра игровой системы.
*/

namespace AbilitySystemConstants {
	const short MAIN_DICE_SIZE = 100;
	const short DAMAGE_DICE_SIZE = 5;
	const short DAMAGE_DICEPOOL_SKILL_DIVIDER = 5;
	const short MIN_ABILITY_DICEROLL_BONUS = -150;
	const short MAX_ABILITY_DICEROLL_BONUS = 150;
	const short SUCCESS_THRESHOLD = 0;
	const short DEFAULT_CRITICAL_FAIL_THRESHOLD = 95;
	const short DEFAULT_CRITICAL_SUCCESS_THRESHOLD = 6;
	const short MAX_FAIL_DEGREE = -10;
	const short MAX_SUCCESS_DEGREE = 10;
	const short DEGREE_DIVIDER = 10;
	const short SKILL_RATING_DIVIDER = 2;
	const short PARAMETER_RATING_DIVIDER = 2;
	const short SITUATIONABLE_FACTOR = 5;
	constexpr short MINIMAL_FAIL_RESULT = SUCCESS_THRESHOLD - 1;
}; // namespace AbilitySystem

#endif // _ABILITY_CONSTANTS_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
