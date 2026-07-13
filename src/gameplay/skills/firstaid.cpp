#include "firstaid.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/minions.h"

#include "engine/entities/char_data.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_utils.h"
#include "skill_messages.h"
#include "gameplay/abilities/timed_abilities.h"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "gameplay/abilities/abilities_constants.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/fight/fight.h"
#include "utils/random.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <string>
#include <vector>

// =====================================================================================
// First Aid (issue.first-aid): affect removal uses the same potency-roll contest as the
// data-driven spells:
//   - the `kAfCurable` flag is the only source of truth for "can be cured";
//   - the cure's potency is rolled from First Aid skill + Intelligence stat;
//   - the potency contest mirrors DispelSucceeds (5% luck floor + weighted
//     dice + skill + stat vs the affect's recorded potency);
//   - cooldown applies ONLY on a failed contest (or a failed HP-heal attempt);
//   - argument syntax mirrors do_cast: 'spell name' [target], with bare-target
//     legacy form preserved as auto-pick on the named character.
//
// The HP-heal half is preserved from the legacy version because low-level fighters
// often have no other healing route.
// =====================================================================================

namespace {

// First Aid roll constants. These play the role of <potency_roll> for the future
// universal-ability config; once spells.xml grows to cover skills the values move
// there and this block disappears.
// Tuned to mirror an offensive affect spell's <potency_roll> (e.g. kSleep: 5d9+12, skill
// 3/1.25, Int thr 22 wt 0.5) so the contest is between comparable rolls, then PotencyWeight 1.2
// gives the healer ~8/10 odds to cure an affect from a mage of similar skill+stat (issue.sleep-hotfix).
constexpr int kFirstAidDiceNum = 5;
constexpr int kFirstAidDiceSize = 9;
constexpr int kFirstAidDiceAdd = 12;
constexpr double kFirstAidLowSkillBonus = 3.0;   // skill <  kNoviceSkillThreshold (pipeline weight)
constexpr double kFirstAidHiSkillBonus = 1.25;   // skill >= kNoviceSkillThreshold (pipeline weight)
constexpr int kFirstAidStatThreshold = 22;       // Int stat threshold (mirrors cast rolls)
constexpr double kFirstAidStatWeight = 0.5;      // Int weight (mirrors cast rolls)
constexpr float kFirstAidPotencyWeight = 1.2f;   // ~8/10 win vs a same-skill/stat mage's affect
constexpr float kFirstAidFailDecayFraction = 0.40f;  // failed cure still weakens the affect by 40%
                                                     // of the rolled heal potency (like unaffect decay)

// Compute the First Aid cure potency for the contest with affect.potency.
// Math mirrors talents_actions::Roll::CalcSkillCoeff + CalcBaseStatCoeff so the new
// First Aid behaves the same as a data-driven dispel spell would.
float ComputeFirstAidPotency(CharData *ch) {
	int dice = 0;
	for (int i = 0; i < kFirstAidDiceNum; ++i) {
		dice += number(1, kFirstAidDiceSize);
	}
	dice += kFirstAidDiceAdd;

	const int skill = GetSkill(ch, ESkill::kFirstAid);
	const int low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	const int hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	const double skill_coeff = (low_skill * kFirstAidLowSkillBonus
								+ hi_skill * kFirstAidHiSkillBonus) / 100.0;

	const int int_stat = GetRealBaseStat(ch, EBaseStat::kInt);
	const double stat_coeff = std::max(0, int_stat - kFirstAidStatThreshold)
							  * kFirstAidStatWeight / 100.0;

	return static_cast<float>(dice + skill_coeff + stat_coeff) * kFirstAidPotencyWeight;
}

// Parse First Aid arguments in the do_cast style:
//   `firstaid`                  -> spell=kUndefined, vict=empty (auto-pick on self)
//   `firstaid <target>`         -> spell=kUndefined, vict=<target> (auto-pick on target, legacy)
//   `firstaid 'spell'`          -> spell=<spell>, vict=empty (named affect on self)
//   `firstaid 'spell' <target>` -> spell=<spell>, vict=<target> (named affect on target)
// Quote chars match do_cast: ', *, !. Returns false on an unknown affect name.
bool ParseFirstAidArgs(const char *argument, EAffect &affect_id, std::string &vict_arg) {
	affect_id = EAffect::kUndefined;
	vict_arg.clear();
	if (!argument || !*argument) {
		return true;
	}

	std::string arg_str(argument);
	utils::TrimLeft(arg_str);
	if (arg_str.empty()) {
		return true;
	}

	const auto quote1 = arg_str.find_first_of("'*!");
	if (quote1 == std::string::npos) {
		// Bare argument: treat as target name (legacy compatibility, auto-pick affect).
		char buf[kMaxInputLength];
		one_argument(arg_str.data(), buf);
		vict_arg = buf;
		return true;
	}

	const auto quote2 = arg_str.find_first_of("'*!", quote1 + 1);
	std::string affect_name_str = (quote2 != std::string::npos)
			? arg_str.substr(quote1 + 1, quote2 - quote1 - 1)
			: arg_str.substr(quote1 + 1);
	if (!GetAffectNumByName(affect_name_str, affect_id)) {
		return false;
	}

	if (quote2 != std::string::npos && quote2 + 1 < arg_str.size()) {
		std::string target_str = arg_str.substr(quote2 + 1);
		utils::TrimLeft(target_str);
		char buf[kMaxInputLength];
		one_argument(target_str.data(), buf);
		vict_arg = buf;
	}
	return true;
}

// Pick the cure target affect. Returns nullptr if nothing is curable.
//   desired == kUndefined -> the kAfCurable affect on vict with the LOWEST potency.
//                            "Triage easy stuff first" -- if even the cheapest fails,
//                            stronger affects need a real dispel spell anyway.
//   desired != kUndefined -> the matching kAfCurable affect of that type, or nullptr.
Affect<EApply>::shared_ptr PickCureTarget(CharData *vict, EAffect desired) {
	if (desired != EAffect::kUndefined) {
		for (const auto &aff : vict->affected) {
			if (aff && aff->affect_type == desired && IS_SET(aff->battleflag, kAfCurable)
					&& affects::AffectBuffKind(aff->affect_type) != affects::EBuff::kYes) {
				return aff;
			}
		}
		return nullptr;
	}
	Affect<EApply>::shared_ptr best;
	for (const auto &aff : vict->affected) {
		// Only harmful affects are cured -- never the target's own buffs (many buffs also carry
		// kAfCurable). The affect's own buff flag is the source of truth: cure anything that is NOT a
		// declared buff (debuff or ambiguous). issue.spells-hotfix / issue.affect-migration.
		if (aff && IS_SET(aff->battleflag, kAfCurable)
				&& affects::AffectBuffKind(aff->affect_type) != affects::EBuff::kYes
				&& (!best || aff->potency < best->potency)) {
			best = aff;
		}
	}
	return best;
}

// Apply the standard First Aid cooldown. Paladine/Sorcerer keep their class
// modifiers; the kPhysicians feat halves it. (Logic preserved from the legacy code.)
void ApplyFirstAidCooldown(CharData *ch) {
	struct TimedSkill timed;
	timed.skill = ESkill::kFirstAid;
	int time = privilege::IsImmortal(ch) ? 1 : IS_PALADINE(ch) ? 4 : IS_SORCERER(ch) ? 2 : 6;
	if (CanUseFeat(ch, EFeat::kPhysicians)) {
		time /= 2;
	}
	timed.time = time;
	ImposeTimedSkill(ch, &timed);
}

}  // namespace

void DoFirstaid(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!GetSkill(ch, ESkill::kFirstAid)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kFirstAid, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}
	if (!privilege::IsGod(ch) && IsTimedBySkill(ch, ESkill::kFirstAid)) {
		SendMsgToChar("Так много лечить нельзя - больных не останется.\r\n", ch);
		return;
	}

	EAffect desired_affect = EAffect::kUndefined;
	std::string vict_arg;
	if (!ParseFirstAidArgs(argument, desired_affect, vict_arg)) {
		SendMsgToChar("Использование: firstaid ['название эффекта' [цель]] | [цель]\r\n", ch);
		return;
	}

	CharData *vict = ch;
	if (!vict_arg.empty()) {
		char buf[kMaxInputLength];
		strcpy(buf, vict_arg.c_str());
		vict = target_resolver::FindCharInRoom(ch, buf);
		if (!vict) {
			SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kFirstAid, ESkillMsg::kNoTarget) + "\r\n", ch);
			return;
		}
	}

	if (vict->GetEnemy()) {
		act("$N сражается, $M не до ваших телячьих нежностей.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (vict->IsNpc() && !IsCharmice(vict)) {
		SendMsgToChar("Вы не красный крест - лечить всех подряд.\r\n", ch);
		return;
	}

	// --- HP heal phase (preserved from the legacy implementation) ---
	int percent_hp = number(1, MUD::Skills()[ESkill::kFirstAid].difficulty);
	int prob_hp = CalcCurrentSkill(ch, ESkill::kFirstAid, vict);
	if (privilege::IsImmortal(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || GET_GOD_FLAG(vict, EGf::kGodsLike)) {
		percent_hp = 0;
	}
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob_hp = 0;
	}
	const bool heal_success = (prob_hp >= percent_hp);
	const bool needs_hp_heal =
			(vict->get_real_max_hit() > 0 && (vict->get_hit() * 100 / vict->get_real_max_hit()) < 31)
			|| (vict->get_real_max_hit() <= 0 && vict->get_hit() < vict->get_real_max_hit())
			|| (vict->get_hit() < vict->get_real_max_hit() && CanUseFeat(ch, EFeat::kHealer));
	bool healed_hp = false;
	if (needs_hp_heal && heal_success) {
		const int dif = std::min(vict->get_real_max_hit(), vict->get_real_max_hit() - vict->get_hit());
		const int add = std::min(dif, (dif * (prob_hp - percent_hp) / 100) + 1);
		vict->set_hit(vict->get_hit() + add);
		healed_hp = true;
	}

	// --- Affect removal phase (new: potency contest gated by kAfCurable) ---
	const auto target_aff = PickCureTarget(vict, desired_affect);
	bool affect_cleared = false;
	bool affect_contest_failed = false;
	if (target_aff) {
		const float potency = ComputeFirstAidPotency(ch);
		const float aff_potency = target_aff->potency;
		const bool luck = (number(1, 100) <= 5);
		const bool ok = luck || potency > aff_potency;
		if (privilege::IsImmortal(ch) || ch->IsFlagged(EPrf::kTester)) {
			char dbuf[256];
			snprintf(dbuf, sizeof(dbuf),
					 "First Aid: %.1f%s vs %s [p: %.1f]. %s.\r\n",
					 potency, luck ? " (luck)" : "",
					 affects::AffectMsg(target_aff->affect_type, affects::EAffectMsgType::kShortDesc).c_str(), aff_potency,
					 ok ? "Success" : "Fail");
			SendMsgToChar(dbuf, ch);
		}
		if (ok) {
			RemoveAffectFromCharAndRecalculate(vict, target_aff->affect_type);
			affect_cleared = true;
		} else {
			affect_contest_failed = true;
			// Like unaffect-spell decay: a failed cure still chips the affect (by a fraction of the
			// rolled heal potency) so repeated First Aid eventually removes it. Floored at 0.
			target_aff->potency = std::max(0.0f, target_aff->potency - kFirstAidFailDecayFraction * potency);
		}
	}

	// --- Outcome narration + cooldown ---
	const bool did_something = healed_hp || affect_cleared;
	const bool failed_something = (needs_hp_heal && !heal_success) || affect_contest_failed;

	if (!did_something && !failed_something) {
		// Nothing to do: HP fine, no curable affect (or named one isn't present).
		if (vict == ch) {
			act("Вы в лечении не нуждаетесь.", false, ch, nullptr, vict, kToChar);
		} else {
			act("$N в лечении не нуждается.", false, ch, nullptr, vict, kToChar);
		}
		return;
	}

	ImproveSkill(ch, ESkill::kFirstAid, did_something, nullptr);

	if (vict != ch) {
		if (did_something) {
			act("Вы оказали первую помощь $N2.", false, ch, nullptr, vict, kToChar);
			act("$n оказал$g первую помощь $N2.",
				true, ch, nullptr, vict, kToNotVict | kToArenaListen);
			if (ch->get_sex() == EGender::kMale) {
				sprintf(buf, "%s оказал вам первую помощь.\r\n", ch->get_name().c_str());
			} else {
				sprintf(buf, "%s оказала вам первую помощь.\r\n", ch->get_name().c_str());
			}
			SendMsgToChar(buf, vict);
			vict->zero_wait();
			update_pos(vict);
		} else {
			act("Вы безрезультатно попытались оказать первую помощь $N2.",
				false, ch, nullptr, vict, kToChar);
			act("$N безрезультатно попытал$U оказать вам первую помощь.",
				false, vict, nullptr, ch, kToChar);
			act("$n безрезультатно попытал$u оказать первую помощь $N2.",
				true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		}
	} else {
		if (did_something) {
			act("Вы оказали себе первую помощь.", false, ch, nullptr, nullptr, kToChar);
			act("$n оказал$g себе первую помощь.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		} else {
			act("Вы безрезультатно попытались оказать себе первую помощь.",
				false, ch, nullptr, vict, kToChar);
			act("$n безрезультатно попытал$u оказать себе первую помощь.",
				false, ch, nullptr, vict, kToRoom | kToArenaListen);
		}
	}

	// issue.spells-hotfix: cooldown on any real use (success OR failure); was failure-only, which let
	// a successful cure/heal be spammed.
	ApplyFirstAidCooldown(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
