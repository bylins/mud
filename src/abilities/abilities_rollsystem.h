#ifndef ABILITY_ROLLSYSTEM_HPP_INCLUDED_
#define ABILITY_ROLLSYSTEM_HPP_INCLUDED_

/*
	Классы, реализующие базовые броски на успех и провал
	навыков (technique), а также хранение результатов броска.

	Черновой варитант. 6.10.2019
*/

#include "abilities_constants.h"
#include "entities/char.h"

#include <string>

namespace AbilitySystem {

class AbilityRollType {
 protected:
	CharacterData *_actor;
	short _actorRating;
	short _degreeOfSuccess;
	const FeatureInfoType *_ability;
	bool _success;
	bool _criticalFail;
	bool _criticalSuccess;
	bool _wrongConditions;
	ESkill _baseSkill;
	std::string _denyMessage;

	AbilityRollType() :
		_actor{nullptr},
		_actorRating{0},
		_degreeOfSuccess{0},
		_ability{nullptr},
		_success{false},
		_criticalFail{false},
		_criticalSuccess{false},
		_wrongConditions(false),
		_baseSkill{ESkill::kIncorrect},
		_denyMessage{"Если вы это прочитали, значит, у кодера проблема.\r\n"} {};

	virtual void initialize(CharacterData *abilityActor, int usedAbility);
	virtual void processingResult(short result, short diceRoll);
	virtual void performAbilityTest();
	virtual void determineBaseSkill();
	virtual bool tryRevealWrongConditions();
	virtual bool revealCriticalSuccess(short diceRoll);
	virtual bool revealCriticalFail(short diceRoll);
	virtual bool isActorCantUseAbility();
	virtual bool isActorMoraleFailure() = 0;
	virtual short calculateTargetRating() = 0;
	virtual short calculateDicerollBonus() = 0;
	virtual short calculatBaseSkillRating() = 0;
	virtual short calculateActorRating();
	virtual int calculateBaseDamage();
	virtual int calculateAddDamage();
	virtual inline float calculateAbilityDamageFactor();
	virtual inline float calculateDegreeOfSuccessDamageFactor();
	virtual inline float calculateSituationalDamageFactor();
	virtual void trainBaseSkill(bool success) = 0;

 public:
	bool isSuccess() { return _success; };
	bool isCriticalFail() { return _criticalFail; };
	bool isCriticalSuccess() { return _criticalSuccess; };
	bool isWrongConditions() { return _wrongConditions; };
	int ID() { return _ability->ID; };
	int getDegreeOfSuccess() { return _degreeOfSuccess; };
	CharacterData *actor() { return _actor; };
	void sendDenyMessageToActor();
};

//  -------------------------------------------------------

class AgainstRivalRollType : public AbilityRollType {
 protected:
	CharacterData *_rival;

	void trainBaseSkill(bool success);
	short calculateTargetRating();
	short calculatBaseSkillRating();
	short calculateDicerollBonus();
	bool isActorMoraleFailure();

 public:
	void initialize(CharacterData *abilityActor, int usedAbility, CharacterData *abilityVictim);
	CharacterData *rival() { return _rival; };

	AgainstRivalRollType() :
		_rival{nullptr} {};
};

//  -------------------------------------------------------

class TechniqueRollType : public AgainstRivalRollType {
 protected:
	int _weaponEquipPosition;

	int calculateAddDamage();
	void determineBaseSkill();
	bool checkTechniqueKit();
	bool isSuitableItem(const TechniqueItem &techniqueItem);

 public:
	int getWeaponEquipPosition() { return _weaponEquipPosition; };
	int calculateDamage();

	TechniqueRollType() :
		_weaponEquipPosition{WEAR_WIELD} {};
};

}; //namespace AbilitySystem

#endif // ABILITY_ROLLSYSTEM_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
