/*
	Базовые проверки на успех и провал применения навыков.
*/

#include "abilities_rollsystem.h"
#include "structs/global_objects.h"
#include "game_magic/magic.h"
#include "abilities_constants.h"

namespace abilities_roll {

void AgainstRivalRoll::Init(CharData *actor, abilities::EAbility ability, CharData *victim) {
	rival_ = victim;
	AbilityRoll::Init(actor, ability);
};

void AbilityRoll::Init(CharData *actor, abilities::EAbility ability_id) {
	actor_ = actor;
	ability_ = ability_id;
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
		SendMsgToChar(actor_,
					  "&CНавык: %s, Рейтинг навыка: %d, Рейтинг цели: %d, Сложность: %d Бросок d100: %d, Итог: %d (%s)&n\r\n",
					  MUD::Ability(ability_).GetCName(),
					  actor_rating_,
					  target_rating,
					  difficulty,
					  roll,
					  roll_result,
					  success_ ? "успех" : "провал");
	}
};

// TODO Добавить возмодность проверки вполнения "специальныз условимй" для конкретной абилки, а не только оружия
bool AbilityRoll::TryRevealWrongConditions() {
	if (IsActorCantUseAbility()) {
		return true;
	};
	DetermineBaseSkill();
	if (base_skill_ == ESkill::kUndefined) {
		return true;
	};
	return false;
};

// явно что-то не дописано, чтоб не углубляться в чужие мысли сделаем пока так
EFeat ConvertFeat(abilities::EAbility new_enum) {
	switch (new_enum) {
		case abilities::EAbility::kScirmisher:
			return EFeat::kScirmisher;
		break;
		case abilities::EAbility::kTactician:
			return  EFeat::kTactician;
		break;
		default:
			return EFeat::kUndefined;
		break;
	}

}

bool AbilityRoll::IsActorCantUseAbility() {
	if (!CanUseFeat(actor_, ConvertFeat(ability_))) {
		deny_msg_ = "Вы не можете использовать этот навык.\r\n";
		wrong_conditions_ = true;
	}
	return wrong_conditions_;
}

void AbilityRoll::DetermineBaseSkill() {
	base_skill_ = MUD::Ability(ability_).GetBaseSkill();
}

void AbilityRoll::SendDenyMsgToActor() {
	SendMsgToChar(deny_msg_, actor_);
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
	if (roll < MUD::Ability(ability_).GetCritsuccessThreshold()) {
		return true;
	}
	return false;
};

bool AbilityRoll::RevealCritfail(int roll) {
	if ((roll > MUD::Ability(ability_).GetCritfailThreshold()) && IsActorMoraleFailure()) {
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
	int parameter_rating = MUD::Ability(ability_).GetBaseParameter(actor_) / abilities::kStatRatingDivider;
	int roll_bonus = CalcRollBonus();
	return (skill_rating + parameter_rating + roll_bonus);
};

//TODO: убрать обертку после изменения системы прокачки скиллов
void AgainstRivalRoll::TrainBaseSkill(bool success) {
	TrainSkill(actor_, base_skill_, success, rival_);
};

int AgainstRivalRoll::CalcTargetRating() {
	return std::max(0, CalcSaving(actor_, rival_, MUD::Ability(ability_).GetSaving(), 0));
};

//TODO: избавиться от target в calculate_skill и убрать обертку
int AgainstRivalRoll::CalcBaseSkillRating() {
	return (CalcCurrentSkill(actor_, base_skill_, rival_) / abilities::kSkillRatingDivider);
};

int AgainstRivalRoll::CalcRollBonus() {
	auto bonus = MUD::Ability(ability_).GetRollBonus();
	if (actor_->IsPlayer()) {
		if (rival_->IsNpc()) {
			bonus += MUD::Ability(ability_).GetPvePenalty();
		} else {
			bonus += MUD::Ability(ability_).GetPvpPenalty();
		}
	} else {
		if (rival_->IsPlayer()) {
			bonus += MUD::Ability(ability_).GetEvpPenalty();
		}
	}
	bonus += MUD::Ability(ability_).CalcSituationalRollBonus(actor_, rival_);

	return std::clamp(bonus, abilities::kMinRollBonus, abilities::kMaxRollBonus);
};

void TechniqueRoll::DetermineBaseSkill() {
	if (CheckTechniqueKit()) {
		if (base_skill_ == ESkill::kUndefined) {
			base_skill_ = MUD::Ability(ability_).GetBaseSkill();
		}
	} else {
		wrong_conditions_ = true;
		deny_msg_ = "У вас нет подходящей экипировки.\r\n";
	}
}

bool TechniqueRoll::CheckTechniqueKit() {
	if (MUD::Ability(ability_).GetItemKits().empty()) {
		return true;
	};
	for (const auto &itemKit: MUD::Ability(ability_).GetItemKits()) {
		bool kit_fit = true;
		for (const auto &item: *itemKit) {
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
	ObjData *char_item = actor_->equipment[item.wear_position];
	if (item == char_item) {
		if (char_item->get_type() == EObjType::kWeapon) {
			weapon_equip_position_ = item.wear_position;
			if (MUD::Ability(ability_).IsWeaponTechnique()) {
				base_skill_ = static_cast<ESkill>(char_item->get_spec_param());
			};
		};
		return true;
	};
	return false;
};

//	TODO: Привести подсчет дамага к одному знаменателю с несколькими возможными точками входа.
int AbilityRoll::CalcBaseDamage() {
	int base_parameter = MUD::Ability(ability_).GetBaseParameter(actor_);
	int dice_num = actor_->GetSkill(base_skill_) / abilities::kDmgDicepoolSkillDivider;
	dice_num = std::min(dice_num, base_parameter);
	return RollDices(std::max(1, dice_num), abilities::kDmgDiceSize);
};

int AbilityRoll::CalcAddDamage() {
	int dice_num = MUD::Ability(ability_).GetEffectParameter(actor_);
	int dice_size = abilities::kDmgDiceSize;
	return RollDices(dice_num, dice_size);
}

int TechniqueRoll::CalcAddDamage() {
	ObjData *weapon = actor_->equipment[weapon_equip_position_];
	if (weapon) {
		int dice_num = MUD::Ability(ability_).GetEffectParameter(actor_) + GET_OBJ_VAL(weapon, 1);
		int dice_size = GET_OBJ_VAL(weapon, 2);
		return RollDices(dice_num, dice_size);
	}

	return AgainstRivalRoll::CalcAddDamage();
};

int AbilityRoll::CalcAbilityDamageBonus(int dmg) {
	return (dmg * MUD::Ability(ability_).GetDamageBonus() / 100);
};

int AbilityRoll::CalcSuccessDegreeDamageBonus(int dmg) {
	return (dmg * success_degree_ * MUD::Ability(ability_).GetSuccessDegreeDamageBonus() / 100);
};

int AbilityRoll::CalcSituationalDamageBonus(int dmg) {
	return (dmg * MUD::Ability(ability_).CalcSituationalDamageFactor(actor_) / 100);
};

int TechniqueRoll::CalcDamage() {
	int dmg = CalcBaseDamage() + CalcAddDamage();
	dmg += CalcAbilityDamageBonus(dmg) + CalcSuccessDegreeDamageBonus(dmg) + CalcSituationalDamageBonus(dmg);
	return dmg;
};

}; // namespace AbilitySystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
