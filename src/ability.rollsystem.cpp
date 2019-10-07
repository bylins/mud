/*
	Файл содержит код функций-членов классов, реализующих
	базовые проверки на успех и провал применения навыкоув умений.

*/

#include "ability.rollsystem.hpp"

#include "comm.h"
#include "features.hpp"
#include "obj.hpp"
#include "random.hpp"
#include "utils.h"

namespace AbilitySystem {

	using namespace AbilitySystemConstants;

	void AbilityRoll::initialize(CHAR_DATA *abilityActor, int usedAbility) {
		_actor = abilityActor;
		_ability = usedAbility;
		//TODO: Ввести таки систему исключений
		if (tryRevealWrongConditions()) {
			_success = false;
			return;
		}
		performAbilityTest();
	};

	void AbilityRollVSCharacter::initialize(CHAR_DATA *abilityActor, int usedAbility, CHAR_DATA *abilityVictim) {
		_target = abilityVictim;
		AbilityRoll::initialize(abilityActor, usedAbility);
	};

	//TODO добавить возможность прописывать автоуспех навыка
	void AbilityRoll::performAbilityTest() {
		_actorRating = calculateAbilityRating();
		short targetRating = calculateTargetRating();
		short diceRoll = number(1, MAIN_DICE_SIZE);
		short rollResult = _actorRating - targetRating - diceRoll;
		processingResult(rollResult, diceRoll);
		if (PRF_FLAGGED(_actor, PRF_TESTER)) {
			send_to_char(_actor, "&CНавык: %s, Рейтинг навыка: %d, Рейтинг цели: %d, Бросок d100: %d, Итог: %d (%s)&n\r\n",
					feat_info[_ability].name, _actorRating, targetRating, diceRoll, rollResult, _success ? "успех":"провал" );
		}
	};

	bool AbilityRoll::tryRevealWrongConditions() {
		if (checkActorCantUseAbility()) {
			return true;
		};
		determineBaseSkill();
		if (_baseSkill == SKILL_INVALID) {
			return true;
		};
		return false;
	};

	bool AbilityRoll::checkActorCantUseAbility() {
		if (!IS_IMPL(_actor) && !can_use_feat(_actor, _ability)) {
			_denyMessage = "Вы не можете использовать этот навык.\r\n";
			_wrongConditions = true;
		};
		return _wrongConditions;
	}

	void AbilityRoll::determineBaseSkill() {
		_baseSkill = feat_info[_ability].baseSkill;
	}

	void AbilityRoll::sendDenyMessage(CHAR_DATA *recipient) {
			send_to_char(_denyMessage, recipient);
	};

	void AbilityRoll::processingResult(short result, short diceRoll) {
		_success = (result >= SUCCESS_THRESHOLD);
		_degreeOfSuccess = result/DEGREE_DIVIDER;
		_degreeOfSuccess = MIN(_degreeOfSuccess, MAX_SUCCESS_DEGREE);
		_degreeOfSuccess = MAX(_degreeOfSuccess, MAX_FAIL_DEGREE);
		if (_success) {
			_criticalSuccess =  revealCriticalSuccess(diceRoll);
		} else {
			_criticalFail =  revealCriticalFail(diceRoll);
			trainBaseSkill();
		}
	};

	//TODO: Добавить учет удачи и мб годлайка/курса
	bool AbilityRoll::revealCriticalSuccess(short diceRoll) {
		if (diceRoll < feat_info[_ability].criticalSuccessThreshold) {
			return true;
		}
		return false;
	};

	bool AbilityRoll::revealCriticalFail(short diceRoll) {
		if (diceRoll > feat_info[_ability].criticalFailThreshold) {
			return true;
		}
		return false;
	};

	// TODO: переделать на static, чтобы иметь возможность считать рейтинги, к примеру, при наложении аффектов
	short AbilityRoll::calculateAbilityRating() {
		short baseSkillRating = calculatBaseSkillRating();
		short baseParameterRating  = feat_info[_ability].getBaseParameter(_actor)/PARAMETER_RATING_DIVIDER;
		short dicerollBonus = calculateDicerollBonus();
		return (baseSkillRating + baseParameterRating + dicerollBonus);
	};

	//TODO: убрать обертку после изменения системы прокачки скиллов
	void AbilityRollVSCharacter::trainBaseSkill() {
		train_skill(_actor, _baseSkill, skill_info[_baseSkill].max_percent, _target);
	};

	short AbilityRollVSCharacter::calculateTargetRating() {
		return calculateSaving(_actor, _target, feat_info[_ability].oppositeSaving, 0);
	};

	//TODO: избавиться от target в calculate_skill и убрать обертку
	short AbilityRollVSCharacter::calculatBaseSkillRating() {
		return (calculate_skill(_actor, _baseSkill, _target)/SKILL_RATING_DIVIDER);
	};

	//TODO: Избавиться от таргета в ситуационном бонусе, нафиг он там не нужен
	short AbilityRollVSCharacter::calculateDicerollBonus() {
		return (feat_info[_ability].dicerollBonus + feat_info[_ability].calculateSituationalRollBonus(_actor, _target));
	};

	void ExpedientRoll::determineBaseSkill() {
		if (checkExpedientKit()) {
			if (feat_info[_ability].alwaysUseFeatureSkill || (_baseSkill == SKILL_INVALID)) {
				_baseSkill = feat_info[_ability].baseSkill;
			}
		} else {
			_wrongConditions = true;
			_denyMessage = "У вас нет подходящей экипировки.\r\n";
		}
	}

	bool ExpedientRoll::checkExpedientKit() {
		ExpedientItemKitsGroupType &kitsGroup = feat_info[_ability].expedientItemKitsGroup;
		if (kitsGroup.empty()) {
			return true;
		};
		for (const auto &itemKit : kitsGroup) {
			bool kitFit = true;
			for (const auto &item : *itemKit) {
				if (checkSuitableItem(item)) {
					continue;
				};
				kitFit = false;
				break;
			};
			if (kitFit) {
				return true;
			};
		};
		return false;
	};

	bool ExpedientRoll::checkSuitableItem(const ExpedientItem &expedientItem) {
		OBJ_DATA *characterItem = GET_EQ(_actor, expedientItem._wearPosition);
		if (expedientItem == characterItem) {
			if (GET_OBJ_TYPE(characterItem) == OBJ_DATA::ITEM_WEAPON) {
				_weaponEquipPosition = expedientItem._wearPosition;
				_baseSkill = static_cast<ESkill>GET_OBJ_SKILL(characterItem);
			};
			return true;
		};
		return false;
	};

	/*
		Костыльный-черновой вариант подсчета базового урона от приема.
		hit считает урон внутри себя и не предусматривает передачи бонусов снаружи,
		например от степени успеха броска или критуспеха.
		damage.process это очень даже предуматривает, только на вход ему надо дать
		уже посчитанный дамаг, который откуда взять, особенно если прием является ударом оружием?
		Отсюда.
		Можно в экстраатаке сохранить степень успеха и в hit добавить ее обсчет.
		Вот только в hit уже учитывается вероятность промазать, занулить урон и т.д.
		Т.е. при использовании новой формулы успеха/провала получится двойная проверка на успех применения,
		как-то излишне жесткоко по отношению к игрокам. А отказываться от использования формулы совершенно
		не хочется, скорей наоборот. Это не говоря уж о запредельной похабени, которая творится в hit.
		TODO: В перспективе надо привести подсчет дамага к одному знаменателю с несколькими возможными точками входа.
	*/
	int AbilityRoll::calculateBaseDamage() {
		short abilityBaseParameter = feat_info[_ability].getBaseParameter(_actor);
		short dicePool = GET_SKILL(_actor, _baseSkill);
		dicePool = MIN(dicePool, abilityBaseParameter*DICE_POOL_PARAMETER_FACTOR);
		dicePool /= DICE_POOL_DIVIDER;
		return dice(MAX(1, dicePool), DAMAGE_DICE_SIZE);
	};

	int ExpedientRoll::calculateWeaponDamage() {
		int damage = 0;
		int bonusDicePool = feat_info[_ability].getEffectParameter(_actor)*DICE_POOL_PARAMETER_FACTOR/DICE_POOL_DIVIDER;
		OBJ_DATA *weapon = GET_EQ(_actor, _weaponEquipPosition);
		if (weapon) {
			damage += dice(GET_OBJ_VAL(weapon, 1) + bonusDicePool, GET_OBJ_VAL(weapon, 2));
		} else {
			damage += dice(bonusDicePool, DAMAGE_DICE_SIZE);
		};
		return damage;
	};

	inline float AbilityRoll::calculateAbilityDamageFactor() {
		return 1.00 + feat_info[_ability].baseDamageBonusPercent/100.0;
	};

	inline float AbilityRoll::calculateDegreeOfSuccessDamageFactor() {
		return 1.00 + _degreeOfSuccess*feat_info[_ability].degreeOfSuccessDamagePercent/100.0;
	};

	// TODO Сделать отдельно "чистый" дамаг и со степенями успеха.
	// TODO Обдумать бонус дамага за степени успеза
	int ExpedientRoll::calculateDamage() {
		int damage = calculateBaseDamage() + calculateWeaponDamage();
		damage *= calculateAbilityDamageFactor();
		damage *= calculateDegreeOfSuccessDamageFactor();
		/*
		if (PRF_FLAGGED(_actor, PRF_TESTER)) {
			send_to_char(_actor, "&CНавык: '%s'. Исходящий урон: %d.\r\n",	feat_info[_ability].name, damage);
		}
		*/
		return damage;
	};

}; // namespace AbilitySystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
