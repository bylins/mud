/*
	Файл содержит код функций-членов классов, реализующих
	базовые проверки на успех и провал применения навыкоув умений.

*/

#include "ability.rollsystem.hpp"

#include "comm.h"
#include "obj.hpp"
#include "random.hpp"
#include "utils.h"

namespace AbilitySystem {

	using namespace AbilitySystemConstants;

	void AgainstRivalRollType::initialize(CHAR_DATA *abilityActor, int usedAbility, CHAR_DATA *abilityVictim) {
		_rival = abilityVictim;
		AbilityRollType::initialize(abilityActor, usedAbility);
	};

	void AbilityRollType::initialize(CHAR_DATA *abilityActor, int usedAbility) {
		_actor = abilityActor;
		_ability = &feat_info[usedAbility];
		if (tryRevealWrongConditions()) {
			_success = false;
			return;
		}
		performAbilityTest();
	};

	//TODO добавить возможность прописывать автоуспех навыка
	void AbilityRollType::performAbilityTest() {
		_actorRating = calculateActorRating();
		short targetRating = calculateTargetRating();
		short diceRoll = number(1, MAIN_DICE_SIZE);
		short difficulty = _actorRating - targetRating;
		short rollResult = difficulty - diceRoll;
		processingResult(rollResult, diceRoll);
		if (PRF_FLAGGED(_actor, PRF_TESTER)) {
			send_to_char(_actor, "&CНавык: %s, Рейтинг навыка: %d, Рейтинг цели: %d, Сложность: %d Бросок d100: %d, Итог: %d (%s)&n\r\n",
					_ability->name, _actorRating, targetRating, difficulty, diceRoll, rollResult, _success ? "успех":"провал" );
		}
	};

	// TODO Лобавить возмодность проверки вполнения "специальныз условимй" для конкретной абилки, а не только оружия
	bool AbilityRollType::tryRevealWrongConditions() {
		if (isActorCantUseAbility()) {
			return true;
		};
		determineBaseSkill();
		if (_baseSkill == SKILL_INVALID) {
			return true;
		};
		return false;
	};

	bool AbilityRollType::isActorCantUseAbility() {
		if (!IS_IMPL(_actor) && !can_use_feat(_actor, _ability->ID)) {
			_denyMessage = "Вы не можете использовать этот навык.\r\n";
			_wrongConditions = true;
		};
		return _wrongConditions;
	}

	void AbilityRollType::determineBaseSkill() {
		_baseSkill = _ability->baseSkill;
	}

	void AbilityRollType::sendDenyMessageToActor() {
			send_to_char(_denyMessage, _actor);
	};

	void AbilityRollType::processingResult(short result, short diceRoll) {
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

	bool AbilityRollType::revealCriticalSuccess(short diceRoll) {
		if (diceRoll < _ability->criticalSuccessThreshold) {
			return true;
		}
		return false;
	};

	bool AbilityRollType::revealCriticalFail(short diceRoll) {
		if ((diceRoll > _ability->criticalFailThreshold) && isActorMoraleFailure()) {
			return true;
		}
		return false;
	};

	bool AbilityRollType::isActorMoraleFailure() {
		return true;
	};

	// Костыльно-черновой учет морали
	bool AgainstRivalRollType::isActorMoraleFailure() {
		int actorMorale = _actor->calc_morale();
		int rivalMorale = _rival->calc_morale();
		if (actorMorale > 0 && rivalMorale > 0) {
			return ((number(0, _actor->calc_morale()) - number(0, _rival->calc_morale())) < 0);
		}
		if (actorMorale <= 0 && rivalMorale <= 0) {
			return (actorMorale < rivalMorale);
		}
		if (actorMorale <= 0 && rivalMorale >= 0) {
			return true;
		}
		return false;
	};

	// TODO: переделать на static, чтобы иметь возможность считать рейтинги, к примеру, при наложении аффектов
	short AbilityRollType::calculateActorRating() {
		short baseSkillRating = calculatBaseSkillRating();
		short baseParameterRating  = _ability->getBaseParameter(_actor)/PARAMETER_RATING_DIVIDER;
		short dicerollBonus = calculateDicerollBonus();
		return (baseSkillRating + baseParameterRating + dicerollBonus);
	};

	//TODO: убрать обертку после изменения системы прокачки скиллов
	void AgainstRivalRollType::trainBaseSkill() {
		train_skill(_actor, _baseSkill, skill_info[_baseSkill].max_percent, _rival);
	};

	short AgainstRivalRollType::calculateTargetRating() {
		return MAX(0, calculateSaving(_actor, _rival, _ability->oppositeSaving, 0));
	};

	//TODO: избавиться от target в calculate_skill и убрать обертку
	short AgainstRivalRollType::calculatBaseSkillRating() {
		return (calculate_skill(_actor, _baseSkill, _rival)/SKILL_RATING_DIVIDER);
	};

	//TODO: Избавиться от таргета в ситуационном бонусе, он там не нужен
	short AgainstRivalRollType::calculateDicerollBonus() {
		return (_ability->dicerollBonus + _ability->calculateSituationalRollBonus(_actor, _rival));
	};

	void TechniqueRollType::determineBaseSkill() {
		if (checkTechniqueKit()) {
			if (_baseSkill == SKILL_INVALID) {
				_baseSkill = _ability->baseSkill;
			}
		} else {
			_wrongConditions = true;
			_denyMessage = "У вас нет подходящей экипировки.\r\n";
		}
	}

	bool TechniqueRollType::checkTechniqueKit() {
		//const TechniqueItemKitsGroupType &kitsGroup = _ability->techniqueItemKitsGroup;
		if (_ability->techniqueItemKitsGroup.empty()) {
			return true;
		};
		for (const auto &itemKit : _ability->techniqueItemKitsGroup) {
			bool kitFit = true;
			for (const auto &item : *itemKit) {
				if (isSuitableItem(item)) {
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

	bool TechniqueRollType::isSuitableItem(const TechniqueItem &techniqueItem) {
		OBJ_DATA *characterItem = GET_EQ(_actor, techniqueItem._wearPosition);
		if (techniqueItem == characterItem) {
			if (GET_OBJ_TYPE(characterItem) == OBJ_DATA::ITEM_WEAPON) {
				_weaponEquipPosition = techniqueItem._wearPosition;
				if (_ability->usesWeaponSkill) {
					_baseSkill = static_cast<ESkill>GET_OBJ_SKILL(characterItem);
				};
			};
			return true;
		};
		return false;
	};

	//	TODO: Привести подсчет дамага к одному знаменателю с несколькими возможными точками входа.
	int AbilityRollType::calculateBaseDamage() {
		short abilityBaseParameter = _ability->getBaseParameter(_actor);
		short dicePool = GET_SKILL(_actor, _baseSkill)/DAMAGE_DICEPOOL_SKILL_DIVIDER;
		dicePool = MIN(dicePool, abilityBaseParameter);
		return dice(MAX(1, dicePool), DAMAGE_DICE_SIZE);
	};

	int AbilityRollType::calculateAddDamage() {
		short dicePool = _ability->getEffectParameter(_actor);
		short diceSize = DAMAGE_DICE_SIZE;
		return dice(dicePool, diceSize);
	}

	int TechniqueRollType::calculateAddDamage() {
		OBJ_DATA *weapon = GET_EQ(_actor, _weaponEquipPosition);
		if (weapon) {
			short dicePool = _ability->getEffectParameter(_actor) + GET_OBJ_VAL(weapon, 1);
			short diceSize = GET_OBJ_VAL(weapon, 2);
			return dice(dicePool, diceSize);
		}

		return AgainstRivalRollType::calculateAddDamage();
	};

	inline float AbilityRollType::calculateAbilityDamageFactor() {
		return _ability->baseDamageBonusPercent/100.0;
	};

	inline float AbilityRollType::calculateDegreeOfSuccessDamageFactor() {
		return _degreeOfSuccess*_ability->degreeOfSuccessDamagePercent/100.0;
	};

	inline float AbilityRollType::calculateSituationalDamageFactor() {
		return _ability->calculateSituationalDamageFactor(_actor);
	};

	int TechniqueRollType::calculateDamage() {
		int damage = calculateBaseDamage() + calculateAddDamage();
		float abilityFactor = calculateAbilityDamageFactor();
		float degreeFactor = calculateDegreeOfSuccessDamageFactor();
		float situationalFactor = calculateSituationalDamageFactor();
		damage += damage*abilityFactor + damage*degreeFactor + damage*situationalFactor;
		return damage;
	};

}; // namespace AbilitySystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
