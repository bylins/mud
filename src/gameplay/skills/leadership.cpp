/**
\file leadership.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/skills/skills.h"
#include "utils/utils.h"

int CalcLeadership(CharData *ch) {
	int prob, percent;
	CharData *leader{nullptr};

	if (ch->IsNpc()
		|| !AFF_FLAGGED(ch, EAffect::kGroup)
		|| (!ch->has_master()
			&& ch->followers.empty())) {
		return false;
	}

	if (ch->has_master()) {
		if (ch->in_room != ch->get_master()->in_room) {
			return false;
		}
		leader = ch->get_master();
	} else {
		leader = ch;
	}

	if (!GetSkill(leader, ESkill::kLeadership)) {
		return (false);
	}

	percent = number(1, 101);
	prob = CalcCurrentSkill(leader, ESkill::kLeadership, nullptr);
	if (percent > prob) {
		return (false);
	} else {
		return (true);
	}
}

int CalcLeadershipGroupExpKoeff(CharData *leader, int inroom_members, int koeff) {
	if (koeff >= 100 && (inroom_members > 1) && CalcLeadership(leader)) {
		return 20;
	}
	return 0;
}

int CalcLeadershipGroupSizeBonus(CharData *leader) {
//	if (AFF_FLAGGED(ch, EAffectFlag::AFF_COMMANDER))
//		bonus_commander = VPOSI((ch->get_skill(ESkill::kLeadership) - 120) / 10, 0, 8);
	auto bonus_commander = VPOSI((GetSkill(leader, ESkill::kLeadership) - 200) / 8, 0, 8);
	return static_cast<int>(VPOSI((GetSkill(leader, ESkill::kLeadership) - 80) / 5, 0, 4) + bonus_commander);
}

void UpdateLeadership(CharData *ch, CharData *killer) {
	// train LEADERSHIP
	if (ch->IsNpc() && killer) {
		if (!killer->IsNpc() // Убил загрупленный чар
			&& AFF_FLAGGED(killer, EAffect::kGroup)
			&& killer->has_master()
			&& GetSkill(killer->get_master(), ESkill::kLeadership) > 0
			&& killer->in_room == killer->get_master()->in_room) {
			ImproveSkill(killer->get_master(), ESkill::kLeadership, number(0, 1), ch);
		} else if (killer->IsNpc() // Убил чармис загрупленного чара
			&& IsCharmice(killer)
			&& killer->has_master()
			&& AFF_FLAGGED(killer->get_master(), EAffect::kGroup)) {
			if (killer->get_master()->has_master() // Владелец чармиса НЕ лидер
				&& GetSkill(killer->get_master()->get_master(), ESkill::kLeadership) > 0
				&& killer->in_room == killer->get_master()->in_room
				&& killer->in_room == killer->get_master()->get_master()->in_room) {
				ImproveSkill(killer->get_master()->get_master(), ESkill::kLeadership, number(0, 1), ch);
			}
		}
	}

	// decrease LEADERSHIP
	if (!ch->IsNpc() // Член группы убит мобом
		&& killer
		&& killer->IsNpc()
		&& AFF_FLAGGED(ch, EAffect::kGroup)
		&& ch->has_master()
		&& ch->in_room == ch->get_master()->in_room
		&& GetTrainedSkill(ch->get_master(), ESkill::kLeadership) > 1) {
		const auto current_skill = GetSkillBonus(ch->get_master(), ESkill::kLeadership);
		SetSkill(ch->get_master(), ESkill::kLeadership, current_skill - 1);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
