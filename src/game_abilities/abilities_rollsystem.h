#ifndef ABILITY_ROLLSYSTEM_HPP_INCLUDED_
#define ABILITY_ROLLSYSTEM_HPP_INCLUDED_

/*
	Классы, реализующие базовые броски на успех и провал
	навыков (technique), а также хранение результатов броска.

	Черновой варитант. 6.10.2019
*/

#include "abilities_constants.h"
#include "entities/entities_constants.h"
#include "game_skills/skills.h"
#include "abilities_items_set.h"

#include <string>

class CharData;

namespace abilities_roll {

class AbilityRoll {
 protected:
	CharData *actor_{nullptr};
	abilities::EAbility ability_{abilities::EAbility::kUndefined};
	ESkill base_skill_{ESkill::kUndefined};
	int actor_rating_{0};
	int success_degree_{0};
	bool success_{false};
	bool critical_fail_{false};
	bool critical_success_{false};
	bool wrong_conditions_{false};
	std::string deny_msg_{"Если вы это прочитали, значит, у кодера проблема.\r\n"};

	AbilityRoll() = default;

	virtual void Init(CharData *actor, abilities::EAbility ability_id);
	virtual void ProcessingResult(int result, int roll);
	virtual void PerformAbilityTest();
	virtual void DetermineBaseSkill();
	virtual bool TryRevealWrongConditions();
	virtual bool RevealCritsuccess(int roll);
	virtual bool RevealCritfail(int roll);
	virtual bool IsActorCantUseAbility();
	virtual bool IsActorMoraleFailure() = 0;
	virtual int CalcTargetRating() = 0;
	virtual int CalcRollBonus() = 0;
	virtual int CalcBaseSkillRating() = 0;
	virtual int CalcActorRating();
	virtual int CalcBaseDamage();
	virtual int CalcAddDamage();
	virtual int CalcAbilityDamageBonus(int dmg);
	virtual int CalcSuccessDegreeDamageBonus(int dmg);
	virtual int CalcSituationalDamageBonus(int dmg);
	virtual void TrainBaseSkill(bool success) = 0;

 public:
	[[nodiscard]] bool IsSuccess() const { return success_; };
	[[nodiscard]] bool IsCriticalFail() const { return critical_fail_; };
	[[nodiscard]] bool IsCriticalSuccess() const { return critical_success_; };
	[[nodiscard]] bool IsWrongConditions() const { return wrong_conditions_; };
	//[[nodiscard]] ESkill GetBaseSkill() const { return base_skill_; };
	[[nodiscard]] int GetSuccessDegree() const { return success_degree_; };
	[[nodiscard]] int GetActorRating() const { return actor_rating_; };
	[[nodiscard]] abilities::EAbility GetAbilityId() { return ability_; };
	[[nodiscard]] CharData *GetActor() { return actor_; };
	void SendDenyMsgToActor();
};

//  -------------------------------------------------------

class AgainstRivalRoll : public AbilityRoll {
 protected:
	CharData *rival_{nullptr};

	void TrainBaseSkill(bool success) override;
	int CalcTargetRating() override;
	int CalcBaseSkillRating() override;
	int CalcRollBonus() override;
	bool IsActorMoraleFailure() override;

 public:
	void Init(CharData *actor, abilities::EAbility ability_id, CharData *victim);
	CharData *GetRival() { return rival_; };

	AgainstRivalRoll() = default;
};

//  -------------------------------------------------------

class TechniqueRoll : public AgainstRivalRoll {
 protected:
	EEquipPos weapon_equip_position_{EEquipPos::kWield};

	int CalcAddDamage() override;
	void DetermineBaseSkill() override;
	bool CheckTechniqueKit();
	bool IsSuitableItem(const TechniqueItem &item);

 public:
	[[nodiscard]] int GetWeaponEquipPosition() const { return weapon_equip_position_; };
	int CalcDamage();

	TechniqueRoll() = default;
};

}; //namespace AbilitySystem

#endif // ABILITY_ROLLSYSTEM_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
