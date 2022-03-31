#ifndef ABILITY_ROLLSYSTEM_HPP_INCLUDED_
#define ABILITY_ROLLSYSTEM_HPP_INCLUDED_

/*
	Классы, реализующие базовые броски на успех и провал
	навыков (technique), а также хранение результатов броска.

	Черновой варитант. 6.10.2019
*/

#include "abilities_constants.h"
#include "entities/char_data.h"

#include <string>

namespace AbilitySystem {

class AbilityRoll {
 protected:
	CharData *actor_;
	int actor_rating_;
	int success_degree_;
	const FeatureInfo *ability_;
	bool success_;
	bool critical_fail_;
	bool critical_success_;
	bool wrong_conditions_;
	ESkill base_skill_;
	std::string deny_msg_;

	AbilityRoll() :
		actor_{nullptr},
		actor_rating_{0},
		success_degree_{0},
		ability_{nullptr},
		success_{false},
		critical_fail_{false},
		critical_success_{false},
		wrong_conditions_(false),
		base_skill_{ESkill::kIncorrect},
		deny_msg_{"Если вы это прочитали, значит, у кодера проблема.\r\n"} {};

	virtual void Init(CharData *actor, EFeat ability_id);
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
	virtual float CalcAbilityDamageFactor();
	virtual float CalcSuccessDegreeDamageFactor();
	virtual float CalcSituationalDamageFactor();
	virtual void TrainBaseSkill(bool success) = 0;

 public:
	[[nodiscard]] bool IsSuccess() const { return success_; };
	[[nodiscard]] bool IsCriticalFail() const { return critical_fail_; };
	[[nodiscard]] bool IsCriticalSuccess() const { return critical_success_; };
	[[nodiscard]] bool IsWrongConditions() const { return wrong_conditions_; };
	[[nodiscard]] ESkill GetBaseSkill() const { return base_skill_; };
	[[nodiscard]] int GetSuccessDegree() const { return success_degree_; };
	[[nodiscard]] int GetActorRating() const { return actor_rating_; };
	int GetAbilityId() { return ability_->id; };
	CharData *GetActor() { return actor_; };
	void SendDenyMsgToActor();
};

//  -------------------------------------------------------

class AgainstRivalRoll : public AbilityRoll {
 protected:
	CharData *rival_;

	void TrainBaseSkill(bool success) override;
	int CalcTargetRating() override;
	int CalcBaseSkillRating() override;
	int CalcRollBonus() override;
	bool IsActorMoraleFailure() override;

 public:
	void Init(CharData *actor, EFeat ability_id, CharData *victim);
	CharData *GetRival() { return rival_; };

	AgainstRivalRoll() :
		rival_{nullptr} {};
};

//  -------------------------------------------------------

class TechniqueRoll : public AgainstRivalRoll {
 protected:
	int weapon_equip_position_;

	int CalcAddDamage() override;
	void DetermineBaseSkill() override;
	bool CheckTechniqueKit();
	bool IsSuitableItem(const TechniqueItem &item);

 public:
	[[nodiscard]] int GetWeaponEquipPosition() const { return weapon_equip_position_; };
	int CalcDamage();

	TechniqueRoll() :
		weapon_equip_position_{EEquipPos::kWield} {};
};

}; //namespace AbilitySystem

#endif // ABILITY_ROLLSYSTEM_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
