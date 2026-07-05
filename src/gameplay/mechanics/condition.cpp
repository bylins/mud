/**
\file condition.cpp - a part of the Bylins engine.
\brief Character condition mechanic: hunger (full), thirst, drunkenness.
*/

#include "condition.h"

#include "engine/entities/char_data.h"
#include "utils/utils.h"   // GET_COND
#include "gameplay/affects/affect_data.h"   // CalcDuration, ImposeAffect
#include "gameplay/abilities/feats.h"        // CanUseFeat, EFeat

#include <algorithm>

namespace condition {

int GetCondAboveNorm(const CharData *ch, int cond) {
	return (GET_COND(ch, cond) <= kNormCondition) ? 0 : GET_COND(ch, cond) - kNormCondition;
}

int GetCondAboveNormPercent(const CharData *ch, int cond) {
	return (GetCondAboveNorm(ch, cond) * 100) / (kMaxCondition - kNormCondition);
}

float GetCondPenalty(const CharData *ch, EPenalty type) {
	if (ch->IsNpc()) return 1;
	if (!(GetCondAboveNorm(ch, kFull) || GetCondAboveNorm(ch, kThirst))) return 1;

	auto penalty{0.0};
	if (GetCondAboveNorm(ch, kFull)) {
		int tmp = GetCondAboveNormPercent(ch, kFull);  // 0 - 1
		switch (type) {
			case kDamroll: penalty += tmp / 2; break;   // -50%
			case kHitroll: penalty += tmp / 4; break;   // -25%
			case kCast: penalty += tmp / 4; break;      // -25%
			case kMemGain: penalty += tmp / 4; break;   // -25%
			case kMoveGain: penalty += tmp / 2; break;  // -50%
			case kHitGain: penalty += tmp / 2; break;   // -50%
			case kAc: penalty += tmp / 2; break;        // -50%
			default: break;
		}
	}
	if (GetCondAboveNorm(ch, kThirst)) {
		int tmp = GetCondAboveNormPercent(ch, kThirst);  // 0 - 1
		switch (type) {
			case kDamroll: penalty += tmp / 4; break;   // -25%
			case kHitroll: penalty += tmp / 2; break;   // -50%
			case kCast: penalty += tmp / 2; break;      // -50%
			case kMemGain: penalty += tmp / 2; break;   // -50%
			case kMoveGain: penalty += tmp / 4; break;  // -25%
			case kAc: penalty += tmp / 4; break;        // -25%
			default: break;
		}
	}
	penalty = 100.0 - std::clamp(penalty, 0.0, 100.0);
	penalty /= 100.0;
	return penalty;
}

}  // namespace condition

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
