/**
\file timed_abilities.cpp - a part of the Bylins engine.
\brief Per-character timers for feats and skills (timed talents).
\details issue.handler-cleaning (Bucket 5): moved verbatim from handler.cpp. The feat and
 skill variants are still duplicated; that duplication is intended to disappear when the
 unified abilities system replaces separate feats/skills -- not reworked here to keep the
 handler cleanup separate from a mechanics change.
*/

#include "timed_abilities.h"

#include "engine/core/handler.h"            // kSecsPerPlayerTimed
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"       // MUD::Feats()
#include "gameplay/abilities/feats.h"
#include "gameplay/skills/skills.h"
#include "utils/utils.h"
#include "utils/backtrace.h"

#include <fmt/format.h>
#include <ctime>

template <class TalentId>
int GetTalentTimerMod(CharData *ch, TalentId id) {
	// ABYRVALG Тут нужен цикл по способностям _персонажа_, но в настоящий момнет это невозможно из-за особенностей
	// реализации конйтенера способносткй у персонажа.
	int mod{0};
	for (const auto &feat : MUD::Feats()) {
		if (CanUseFeat(ch, feat.GetId())) {
			mod += feat.effects.GetTimerMod(id);
		}
	}
	return mod;
}

void ImposeTimedFeat(CharData *ch, TimedFeat *timed) {
	ch->timed_feat[timed->feat] = std::max(time(0) + 60, time(0) + timed->time * 60 + GetTalentTimerMod(ch, timed->feat) * kSecsPerMudHour / kSecsPerPlayerTimed);
}

void ExpireTimedFeat(CharData *ch, EFeat feat) {
	if (ch->timed_feat.empty()) {
		log("SYSERR: timed_feat_from_char(%s) when no timed...", GET_NAME(ch));
		return;
	}

	ch->timed_feat.erase(feat);
}

int IsTimedByFeat(CharData *ch, EFeat feat) {
	auto it = ch->timed_feat.find(feat);
	if (it != ch->timed_feat.end()) {
		if (it->second <= time(0)) {
			ch->timed_feat.erase(it);
			return 0;
		}
		return (it->second - time(0) - 1) / 60 + 1;
	}
	return 0;
}

/**
 * Insert an TimedSkill in a char_data structure
 */
void ImposeTimedSkill(CharData *ch, struct TimedSkill *timed) {
	ch->timed_skill[timed->skill] = std::max(time(0) + 60, time(0) + timed->time * 60 + GetTalentTimerMod(ch, timed->skill) * kSecsPerMudHour / kSecsPerPlayerTimed);
//	mudlog(fmt::format("Время установлено {} count {}", ch->timed_skill[timed->skill], timed->time));
}

void ExpireTimedSkill(CharData *ch, ESkill skill) {
	if (ch->timed_skill.empty()) {
		log("SYSERR: ExpireTimedSkill(%s) when no timed...", GET_NAME(ch));
		// core_dump();
		return;
	}

	ch->timed_skill.erase(skill);
}

int IsTimedBySkill(CharData *ch, ESkill id) {
	auto it = ch->timed_skill.find(id);
	if (it != ch->timed_skill.end()) {
		if (it->second <= time(0)) {
			ch->timed_skill.erase(it);
			return 0;
		}
		return (it->second - time(0) - 1) / 60 + 1;
	}
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
