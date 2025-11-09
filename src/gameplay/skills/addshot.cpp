/**
\file addshot.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 06.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "addshot.h"

#include "utils/utils.h"
#include "utils/random.h"
#include "gameplay/abilities/feats_constants.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/abilities/feats.h"
#include "engine/entities/char_data.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/db/global_objects.h"

bool IsArmedWithBow(CharData *ch, fight::AttackType weapon);
void ProcessDoubleShotHits(CharData *ch, ESkill type, fight::AttackType weapon);
void ProcessAddshotHits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon);

void ProcessMultyShotHits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	if (IsArmedWithBow(ch, weapon)) {
		if (ch->GetSkill(ESkill::kAddshot)) {
			ProcessAddshotHits(ch, victim, type, weapon);
		} else {
			ProcessDoubleShotHits(ch, type, weapon);
		}
	}
}

bool IsArmedWithBow(CharData *ch, fight::AttackType weapon) {
	const auto wielded = GetUsedWeapon(ch, weapon);
	return (wielded
		&& !GET_EQ(ch, EEquipPos::kShield)
		&& static_cast<ESkill>(wielded->get_spec_param()) == ESkill::kBows
		&& GET_EQ(ch, EEquipPos::kBoths));
}

void ProcessDoubleShotHits(CharData *ch, ESkill type, fight::AttackType weapon) {
	if (CanUseFeat(ch, EFeat::kDoubleShot)) {
		if (std::min(850, 200 + ch->GetSkill(ESkill::kBows) * 4 + GetRealDex(ch) * 5) >= number(1, 1000)) {
			hit(ch, ch->GetEnemy(), type, weapon);
		}
	}
}

void ProcessAddshotHits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	int prob = CalcCurrentSkill(ch, ESkill::kAddshot, ch->GetEnemy());
	int dex_mod = std::max(GetRealDex(ch) - 25, 0) * 10;
	int pc_mod =IS_CHARMICE(ch) ? 0 : 1;
	auto difficulty = MUD::Skill(ESkill::kAddshot).difficulty * 5;
	int percent = number(1, difficulty);

	TrainSkill(ch, ESkill::kAddshot, true, ch->GetEnemy());
	if (percent <= prob * 9 + dex_mod) {
		hit(ch, victim, type, weapon);
	}
	percent = number(1, difficulty);
	if (percent <= (prob * 6 + dex_mod) && ch->GetEnemy()) {
		hit(ch, victim, type, weapon);
	}
	percent = number(1, difficulty);
	if (percent <= (prob * 4 + dex_mod / 2) * pc_mod && ch->GetEnemy()) {
		hit(ch, victim, type, weapon);
	}
	percent = number(1, difficulty);
	if (percent <= (prob * 19 / 8 + dex_mod / 2) * pc_mod && ch->GetEnemy()) {
		hit(ch, victim, type, weapon);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
