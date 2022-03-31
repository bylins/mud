/*
	Базовые проверки на успех и провал применения навыков.
*/

#include "abilities_rollsystem.h"

namespace AbilitySystem {

void AgainstRivalRoll::Init(CharData *actor, EFeat ability, CharData *victim) {
	rival_ = victim;
	AbilityRoll::Init(actor, ability);
};

void AbilityRoll::Init(CharData *actor, EFeat ability_id) {
	actor_ = actor;
	ability_ = &feat_info[ability_id];
	if (TryRevealWrongConditions()) {
		success_ = false;
		return;
	}
	PerformAbilityTest();
};

//TODO добавить возможность прописывать автоуспех навыка
void AbilityRoll::PerformAbilityTest() {
	actor_rating_ = CalcActorRating();
	int target_rating = CalcTargetRating();
	int roll = number(1, abilities::kMainDiceSize);
	int difficulty = actor_rating_ - target_rating;
	int roll_result = difficulty - roll;
	ProcessingResult(roll_result, roll);
	if (PRF_FLAGGED(actor_, EPrf::kTester)) {
		send_to_char(actor_,
					 "&CНавык: %s, Рейтинг навыка: %d, Рейтинг цели: %d, Сложность: %d Бросок d100: %d, Итог: %d (%s)&n\r\n",
					 ability_->name,
					 actor_rating_,
					 target_rating,
					 difficulty,
					 roll,
					 roll_result,
					 success_ ? "успех" : "провал");
	}
};

// TODO Лобавить возмодность проверки вполнения "специальныз условимй" для конкретной абилки, а не только оружия
bool AbilityRoll::TryRevealWrongConditions() {
	if (IsActorCantUseAbility()) {
		return true;
	};
	DetermineBaseSkill();
	if (base_skill_ == ESkill::kIncorrect) {
		return true;
	};
	return false;
};

bool AbilityRoll::IsActorCantUseAbility() {
	if (!IS_IMPL(actor_) && !IsAbleToUseFeat(actor_, ability_->id)) {
		deny_msg_ = "Вы не можете использовать этот навык.\r\n";
		wrong_conditions_ = true;
	};
	return wrong_conditions_;
}

void AbilityRoll::DetermineBaseSkill() {
	base_skill_ = ability_->base_skill;
}

void AbilityRoll::SendDenyMsgToActor() {
	send_to_char(deny_msg_, actor_);
};

void AbilityRoll::ProcessingResult(int result, int roll) {
	success_ = (result >= abilities::kSuccessThreshold);
	success_degree_ = result / abilities::kDegreeDivider;
	success_degree_ = std::min(success_degree_, abilities::kMaxSuccessDegree);
	success_degree_ = std::max(success_degree_, abilities::kMaxFailDegree);
	if (success_) {
		critical_success_ = RevealCritsuccess(roll);
	} else {
		critical_fail_ = RevealCritfail(roll);
	}
	// Идея была в том, чтобы прокачка шла только на провалах умения.
	// Тогда она естественным образом замедлялась бы по мере роста умения, требуя находить другие источники улучшения.
	// Но это требует выстоенного баланса которым и не пахнет, потому сделал прокачку в любом случае.
	TrainBaseSkill(success_);
};

bool AbilityRoll::RevealCritsuccess(int roll) {
	if (roll < ability_->critsuccess_threshold) {
		return true;
	}
	return false;
};

bool AbilityRoll::RevealCritfail(int roll) {
	if ((roll > ability_->critfail_threshold) && IsActorMoraleFailure()) {
		return true;
	}
	return false;
};

bool AbilityRoll::IsActorMoraleFailure() {
	return true;
};

// Костыльно-черновой учет морали
bool AgainstRivalRoll::IsActorMoraleFailure() {
	int actor_morale = actor_->calc_morale();
	int rival_morale = rival_->calc_morale();
	if (actor_morale > 0 && rival_morale > 0) {
		return ((number(0, actor_->calc_morale()) - number(0, rival_->calc_morale())) < 0);
	}
	if (actor_morale <= 0 && rival_morale <= 0) {
		return (actor_morale < rival_morale);
	}
	if (actor_morale <= 0 && rival_morale >= 0) {
		return true;
	}
	return false;
};

// TODO: переделать на static, чтобы иметь возможность считать рейтинги, к примеру, при наложении аффектов
int AbilityRoll::CalcActorRating() {
	int skill_rating = CalcBaseSkillRating();
	int parameter_rating = ability_->GetBaseParameter(actor_) / abilities::kStatRatingDivider;
	int roll_bonus = CalcRollBonus();
	return (skill_rating + parameter_rating + roll_bonus);
};

//TODO: убрать обертку после изменения системы прокачки скиллов
void AgainstRivalRoll::TrainBaseSkill(bool success) {
	TrainSkill(actor_, base_skill_, success, rival_);
};

int AgainstRivalRoll::CalcTargetRating() {
	return std::max(0, CalculateSaving(actor_, rival_, ability_->saving, 0));
};

//TODO: избавиться от target в calculate_skill и убрать обертку
int AgainstRivalRoll::CalcBaseSkillRating() {
	return (CalcCurrentSkill(actor_, base_skill_, rival_) / abilities::kSkillRatingDivider);
};

int AgainstRivalRoll::CalcRollBonus() {
	return std::clamp(ability_->diceroll_bonus + ability_->CalcSituationalRollBonus(actor_, rival_),
					  abilities::kMinRollBonus, abilities::kMaxRollBonus);
};

void TechniqueRoll::DetermineBaseSkill() {
	if (CheckTechniqueKit()) {
		if (base_skill_ == ESkill::kIncorrect) {
			base_skill_ = ability_->base_skill;
		}
	} else {
		wrong_conditions_ = true;
		deny_msg_ = "У вас нет подходящей экипировки.\r\n";
	}
}

bool TechniqueRoll::CheckTechniqueKit() {
	//const TechniqueItemKitsGroupType &kitsGroup = _ability->techniqueItemKitsGroup;
	if (ability_->item_kits.empty()) {
		return true;
	};
	for (const auto &itemKit : ability_->item_kits) {
		bool kit_fit = true;
		for (const auto &item : *itemKit) {
			if (IsSuitableItem(item)) {
				continue;
			};
			kit_fit = false;
			break;
		};
		if (kit_fit) {
			return true;
		};
	};
	return false;
};

bool TechniqueRoll::IsSuitableItem(const TechniqueItem &item) {
	ObjData *char_item = GET_EQ(actor_, item.wear_position);
	if (item == char_item) {
		if (GET_OBJ_TYPE(char_item) == ObjData::ITEM_WEAPON) {
			weapon_equip_position_ = item.wear_position;
			if (ability_->uses_weapon_skill) {
				base_skill_ = static_cast<ESkill>GET_OBJ_SKILL(char_item);
			};
		};
		return true;
	};
	return false;
};

//	TODO: Привести подсчет дамага к одному знаменателю с несколькими возможными точками входа.
int AbilityRoll::CalcBaseDamage() {
	int base_parameter = ability_->GetBaseParameter(actor_);
	int dice_num = GET_SKILL(actor_, base_skill_) / abilities::kDmgDicepoolSkillDivider;
	dice_num = std::min(dice_num, base_parameter);
	return RollDices(std::max(1, dice_num), abilities::kDmgDiceSize);
};

int AbilityRoll::CalcAddDamage() {
	int dice_num = ability_->GetEffectParameter(actor_);
	int dice_size = abilities::kDmgDiceSize;
	return RollDices(dice_num, dice_size);
}

int TechniqueRoll::CalcAddDamage() {
	ObjData *weapon = GET_EQ(actor_, weapon_equip_position_);
	if (weapon) {
		int dice_num = ability_->GetEffectParameter(actor_) + GET_OBJ_VAL(weapon, 1);
		int dice_size = GET_OBJ_VAL(weapon, 2);
		return RollDices(dice_num, dice_size);
	}

	return AgainstRivalRoll::CalcAddDamage();
};

float AbilityRoll::CalcAbilityDamageFactor() {
	return static_cast<float>(ability_->damage_bonus/100.0);
};

float AbilityRoll::CalcSuccessDegreeDamageFactor() {
	return static_cast<float>(success_degree_*ability_->success_degree_damage_bonus/ 100.0);
};

float AbilityRoll::CalcSituationalDamageFactor() {
	return ability_->CalcSituationalDamageFactor(actor_);
};

int TechniqueRoll::CalcDamage() {
	int damage = CalcBaseDamage() + CalcAddDamage();
	float ability_factor = CalcAbilityDamageFactor();
	float degree_factor = CalcSuccessDegreeDamageFactor();
	float situational_factor = CalcSituationalDamageFactor();
	damage += damage*ability_factor + damage*degree_factor + damage*situational_factor;
	return damage;
};

}; // namespace AbilitySystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
