/**
\file tutelar.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "gameplay/skills/skills.h"
#include "utils/grammar/gender.h"
#include "utils/utils.h"
#include "gameplay/mechanics/minions.h"
#include "engine/db/db.h"
#include "engine/entities/char_data.h"
#include "engine/core/char_handler.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/skills/resque.h"
#include "gameplay/magic/magic.h"  // ActionContext / EStageResult (issue.summons-fix)
#include "gameplay/mechanics/sight.h"

void CheckTutelarSelfSacrfice(CharData *ch, CharData *victim) {
	// если виктим в группе с кем-то с ангелом - вместо смерти виктима умирает ангел
	if (victim->get_hit() <= 0
		&& !victim->IsNpc()
		&& AFF_FLAGGED(victim, EAffect::kGroup)) {
		// make copy of people because keeper might be removed from this list inside the loop
		const auto people = world[victim->in_room]->people;
		for (const auto keeper : people) {
			if (keeper->IsNpc()
				&& keeper->IsFlagged(EMobFlag::kTutelar)
				&& keeper->has_master()
				&& AFF_FLAGGED(keeper->get_master(), EAffect::kGroup)) {
				CharData *keeper_leader = keeper->get_master()->has_master()
										  ? keeper->get_master()->get_master()
										  : keeper->get_master();
				CharData *victim_leader = victim->has_master()
										  ? victim->get_master()
										  : victim;

				if ((keeper_leader == victim_leader) && (may_kill_here(keeper->get_master(), ch, nullptr))) {
					if (!pk_agro_action(keeper->get_master(), ch)) {
						return;
					}
					log("angel_sacrifice: Nmae (ch): %s, Name(charmice): %s, name(victim): %s", GET_NAME(ch), GET_NAME(keeper), GET_NAME(victim));

					SendMsgToChar(victim, "%s пожертвовал%s своей жизнью, вытаскивая вас с того света!\r\n",
								  GET_PAD(keeper, 0), grammar::SexEnding((keeper)->get_sex(), 1));
					snprintf(buf, kMaxStringLength, "%s пожертвовал%s своей жизнью, вытаскивая %s с того света!",
							 GET_PAD(keeper, 0), grammar::SexEnding((keeper)->get_sex(), 1), GET_PAD(victim, 3));
					act(buf, false, victim, nullptr, nullptr, kToRoom | kToArenaListen);

					ExtractCharFromWorld(keeper, 0);
					victim->set_hit(std::min(300, victim->get_max_hit() / 2));
				}
			}
		}
	}
}

void TryToRescueWithTutelar(CharData *ch) {
	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kHelper)
			&& k->IsFlagged(EMobFlag::kTutelar)
			&& !k->GetEnemy()
			&& k->in_room == ch->in_room
			&& sight::CanSee(k, ch)
			&& AWAKE(k)
			&& !IsUnableToAct(k)
			&& k->get_wait() <= 0
			&& k->GetPosition() >= EPosition::kFight) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->GetEnemy() == ch
					&& vict != ch
					&& vict != k) {
					if (GetSkill(k, ESkill::kRescue)) {
						go_rescue(k, ch, vict);
					}
					break;
				}
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
