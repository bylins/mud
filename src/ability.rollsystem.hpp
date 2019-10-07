#ifndef _ABILITY_ROLLSYSTEM_HPP_INCLUDED_
#define _ABILITY_ROLLSYSTEM_HPP_INCLUDED_

/*
	Классы, реализующие базовые броски на успех и провал
	навыков умений, а также хранение результатов бросков.

	Черновой варитант. 6.10.2019
*/

#include "ability.constants.hpp"
#include "char.hpp"
#include "fight_constants.hpp"
#include "skills.h"

#include <string>

namespace AbilitySystem {

	// TODO: выделить расчет чистого урона в методы персонажа.
	class AbilityRoll {
	protected:
		CHAR_DATA *_actor; // сделать по крайней мере пока открытым членом
		short _actorRating;
		short _degreeOfSuccess; // Заменить на чар?
		int _ability;
		bool _success;
		bool _criticalFail;
		bool _criticalSuccess;
		bool _wrongConditions;
		ESkill _baseSkill;
		std::string _denyMessage;

		AbilityRoll() :
			_actor{nullptr},
			_actorRating{0},
			_degreeOfSuccess{0},
			_ability{0},
			_success{false},
			_criticalFail{false},
			_criticalSuccess{false},
			_wrongConditions(false),
			_baseSkill{SKILL_INVALID},
			_denyMessage{"Если вы это прочитали, значит, у кодера проблема.\r\n"}
			{};

		virtual void initialize(CHAR_DATA *abilityActor, int usedAbility);
		virtual void processingResult(short result, short diceRoll);
		virtual void performAbilityTest();
		virtual void determineBaseSkill();
		virtual bool tryRevealWrongConditions();
		virtual bool revealCriticalSuccess(short diceRoll);
		virtual bool revealCriticalFail(short diceRoll);
		virtual bool checkActorCantUseAbility();
		virtual short calculateTargetRating() = 0;
		virtual short calculateDicerollBonus() = 0;
		virtual short calculatBaseSkillRating() = 0;
		virtual short calculateAbilityRating();
		virtual int calculateBaseDamage();
		virtual inline float calculateAbilityDamageFactor();
		virtual inline float calculateDegreeOfSuccessDamageFactor();
		virtual void trainBaseSkill() = 0;

	public:
		bool checkSuccess() {return _success;};
		bool checkCriticalFail() {return _criticalFail;};
		bool checkCriticalSuccess() {return _criticalSuccess;};
		bool checkWrongConditions(){return _wrongConditions;};
		void sendDenyMessage(CHAR_DATA *recipient);
	};

	class AbilityRollVSCharacter : public AbilityRoll {
	protected:
		CHAR_DATA *_target;

		void trainBaseSkill();
		short calculateTargetRating();
		short calculatBaseSkillRating();
		short calculateDicerollBonus();

	public:
		void initialize(CHAR_DATA *abilityActor, int usedAbility, CHAR_DATA *abilityVictim);

		AbilityRollVSCharacter() :
			_target{nullptr}
			{};
	};

	class ExpedientRoll : public AbilityRollVSCharacter {
	protected:
		int _weaponEquipPosition;
		int calculateWeaponDamage();
		void determineBaseSkill();
		bool checkExpedientKit();
		bool checkSuitableItem(const ExpedientItem &expedientItem);

	public:
		int getWeaponEquipPosition() {return _weaponEquipPosition;};
		int calculateDamage();

		ExpedientRoll() :
			_weaponEquipPosition{WEAR_WIELD}
			{};
	};

}; //namespace AbilitySystem

#endif // _ABILITYROLL_INTERFACE_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
