/* ************************************************************************
*   File: spell_parser.cpp                              Part of Bylins    *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "magic_utils.h"
#include "utils/random.h"   // issue.random-noise-rework: GaussIntNumber

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "administration/privilege.h"

#include "gameplay/mechanics/groups.h"
#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "utils/utils_parse.h"
#include "engine/core/target_resolver.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/communication/parcel.h"
#include "magic.h"
#include "magic_internal.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/statistics/spell_usage.h"
#include "utils/backtrace.h"

#include <fmt/format.h>
#include "engine/observability/helpers.h"
#include "engine/observability/metrics.h"
#include "utils/utils_time.h"

char cast_argument[kMaxInputLength];

// True if `ch`'s race counts as "verbal": the cast is narrated as articulated speech
// (PC always, plus the five humanoid NPC races that historically had their own narration
// set). Non-humanoid NPC races default to "sound" -- a single collapsed narration line.
// Caster-side "Вы произнесли заклинание ..." / "Вы выкрикнули ..." banner.
// The kCastIncantToChar sheaf carries the line; {color}/{name}/{nrm}
// placeholders are filled in with bold-red / bold-green by IsViolentAgainst
// or bold-yellow for an ambiguous spell whose
// target wasn't resolved (e.g. an area cast where the banner can't pick a side).
static void EmitCastIncantBanner(CharData *ch, ESpell spell_id, const CharData *tch) {
	std::string incant = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastIncantToChar);
	const auto &spell = MUD::Spell(spell_id);
	const char *color;
	if (spell.GetViolent() == spells::EViolent::kAmbiguous && !tch) {
		color = kColorBoldYel;
	} else {
		color = spell.IsViolentAgainst(ch, tch ? tch : ch) ? kColorBoldRed : kColorBoldGrn;
	}
	auto subst = [&incant](const char *token, const char *value) {
		const auto pos = incant.find(token);
		if (pos != std::string::npos) {
			incant.replace(pos, std::strlen(token), value);
		}
	};
	subst("{color}", color);
	subst("{name}", MUD::Spell(spell_id).GetCName());
	subst("{nrm}", kColorNrm);
	SendMsgToChar(incant + "\r\n", ch);
}

// SaySpell erodes buf, buf1, buf2
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj) {
	char lbuf[256];

	// Silenced caster can't speak the phrase regardless of whether the spell
	// is verbal. A verbal spell shouldn't even reach SaySpell while silenced
	// (do_cast / CastSpell / process_player_attack bail out earlier), but
	// a non-verbal spell will, and still has no business announcing prose.
	// The cast itself continues; only the spoken phrase + room narration
	// are suppressed.
	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		return;
	}

	*buf = '\0';
	strcpy(lbuf, MUD::Spell(spell_id).GetEngCName());
	const auto &cast_phrase_sheaf = MUD::SpellMessages()[spell_id];
	if (!cast_phrase_sheaf.HasMessage(ESpellMsg::kCastPhraseHeathen)
		&& !cast_phrase_sheaf.HasMessage(ESpellMsg::kCastPhraseChristian)) {
		// A non-verbal spell may legitimately ship no cast phrase -- the
		// caster simply makes no sound here. A verbal spell with no phrase
		// IS a content gap, so keep the CMP-level mudlog for that case.
		// (cast phrase is decorative for non-verbal
		// spells. Speak it if present; stay silent otherwise.)
		if (MUD::Spell(spell_id).IsVerbal()) {
			sprintf(buf, "[ERROR]: SaySpell: для спелла %d не объявлена cast_phrase", to_underlying(spell_id));
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		}
		return;
	}

	const bool verbal = IsAbleToSay(ch);

	// Resolve the cast phrase used for viewers who don't Know the spell. PCs pick
	// by religion; NPCs pick at random per cast. Sound-voice casters don't speak
	// so the phrase stays empty (kCastSaySound has no %s slot anyway).
	if (verbal) {
		const int religion = ch->IsNpc()
				? number(kReligionPoly, kReligionMono)
				: GET_RELIGION(ch);
		const std::string &cast_phrase = cast_phrase_sheaf.GetMessage(
				religion ? ESpellMsg::kCastPhraseChristian : ESpellMsg::kCastPhraseHeathen);
		if (!cast_phrase.empty()) {
			strcpy(buf, cast_phrase.c_str());
		}
	}

	// Caster-side banner -- PC only (NPCs have no client to message).
	if (!ch->IsNpc()) {
		if (ch->IsFlagged(EPrf::kNoRepeat)) {
			if (!ch->GetEnemy()) {
				SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			}
		} else {
			EmitCastIncantBanner(ch, spell_id, tch);
		}
	}

	// Pick the room-narration key by cast situation; sound voice collapses every
	// slot to kCastSaySound. The per-spell sheaf may override (with kDefault
	// fallback); when a key has multiple variants the container picks one at random.
	ESpellMsg room_key;
	if (tch != nullptr && tch->in_room == ch->in_room) {
		room_key = (tch == ch) ? ESpellMsg::kCastSayToSelf : ESpellMsg::kCastSayToOther;
	} else if (tobj != nullptr && (tobj->get_in_room() == ch->in_room || tobj->get_carried_by() == ch)) {
		room_key = ESpellMsg::kCastSayToObj;
	} else {
		room_key = ESpellMsg::kCastSayToSomething;
	}
	const std::string &room_format = MUD::SpellMessages().GetMessage(
			spell_id, verbal ? room_key : ESpellMsg::kCastSaySound);

	// The %s slot (when present) is filled by sprintf with the spell name for
	// viewers who Know the cast, or the cast phrase for everyone else. Sound-voice
	// narration has no %s and the argument is ignored, which is safe in standard C.
	sprintf(buf1, room_format.c_str(), MUD::Spell(spell_id).GetCName());
	sprintf(buf2, room_format.c_str(), buf);

	for (const auto i : world[ch->in_room]->people) {
		if (i == ch || i == tch || !i->desc || !AWAKE(i) || AFF_FLAGGED(i, EAffect::kDeafness)) {
			continue;
		}

		if (IS_SET(GET_SPELL_TYPE(i, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
			perform_act(buf1, ch, tobj, tch, i);
		} else {
			perform_act(buf2, ch, tobj, tch, i);
		}
	}

	act(buf1, 1, ch, tobj, tch, kToArenaListen);

	if (tch != nullptr && tch != ch && tch->in_room == ch->in_room && !AFF_FLAGGED(tch, EAffect::kDeafness)) {
		const ESpellMsg vict_key = !verbal
				? ESpellMsg::kCastSaySound
				: (MUD::Spell(spell_id).IsViolentAgainst(ch, tch)
						? ESpellMsg::kCastSayDamageeToVict
						: ESpellMsg::kCastSayHelpeeToVict);
		const std::string &vict_format = MUD::SpellMessages().GetMessage(spell_id, vict_key);
		sprintf(buf1, vict_format.c_str(),
				IS_SET(GET_SPELL_TYPE(tch, spell_id), ESpellType::kKnow | ESpellType::kTemp) ?
				MUD::Spell(spell_id).GetCName() : buf);
		act(buf1, false, ch, nullptr, tch, kToVict);
	}
}

// \todo куча дублированного кода... надо подумать, как сделать одинаковый интерфейс

abilities::EAbility FindAbilityId(const char *name) {
	for (const auto &ability : MUD::Abilities()) {
		if (ability.IsValid() && utils::IsEquivalent(name, ability.GetName())) {
			return ability.GetId();
		}
	}
	return abilities::EAbility::kUndefined;
}

EFeat FindFeatId(const char *name) {
	for (const auto &feat : MUD::Feats()) {
		if (feat.IsValid() && utils::IsEquivalent(name, feat.GetName())) {
			return feat.GetId();
		}
	}
	return EFeat::kUndefined;
}

ESkill FindSkillId(const char *name) {
	for (const auto &skill : MUD::Skills()) {
		if (skill.IsValid() && utils::IsEquivalent(name, skill.GetName())) {
			return skill.GetId();
		}
	}
	return ESkill::kUndefined;
}

ESpell FindSpellId(const std::string &name) {
	return FindSpellId(name.c_str());
}

ESpell FindSpellId(const char *name) {
	int use_syn = (((ubyte) *name <= (ubyte) 'z')
		&& ((ubyte) *name >= (ubyte) 'a'))
		|| (((ubyte) *name <= (ubyte) 'Z') && ((ubyte) *name >= (ubyte) 'A'));
	for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
		char const *realname = (use_syn) ? MUD::Spell(spell_id).GetEngCName() : MUD::Spell(spell_id).GetCName();

		if (!realname || !*realname) {
			continue;
		}
		if (utils::IsEqual(name, realname)) {
			return spell_id;
		}
	}
	return ESpell::kUndefined;
}

ESpell FindSpellIdWithName(const std::string &name) {
	// issue.thing-names: search the string container (where the display names now live) and return
	// the spell id, instead of scanning gameplay records.
	return MUD::SpellMessages().FindByName(name);
}

template<typename T>
void FixName(T &name) {
	size_t pos = 0;
	while ('\0' != name[pos] && pos < kMaxInputLength) {
		if (('.' == name[pos]) || ('_' == name[pos])) {
			name[pos] = ' ';
		}
		++pos;
	}
}

ESkill FixNameAndFindSkillId(char *name) {
	FixName(name);
	return FindSkillId(name);
}

ESkill FixNameFndFindSkillId(std::string &name) {
	FixName(name);
	return FindSkillId(name.c_str());
}

ESpell FixNameAndFindSpellId(char *name) {
	FixName(name);
	return FindSpellId(name);
}

ESpell FixNameAndFindSpellId(std::string &name) {
	FixName(name);
	return FindSpellId(name.c_str());
}

EFeat FixNameAndFindFeatId(const std::string &name) {
	auto copy = name;
	FixName(copy);
	return FindFeatId(copy.c_str());
}

abilities::EAbility FixNameAndFindAbilityId(const std::string &name) {
	auto copy = name;
	FixName(copy);
	return FindAbilityId(copy.c_str());
}

bool MayCastInForbiddenRoom(CharData *caster) {
	if (privilege::IsGrGod(caster)) {
		return true;
	}
	if (caster->IsNpc() &&
		!(AFF_FLAGGED(caster, EAffect::kCharmed) || caster->IsFlagged(EMobFlag::kTutelar)))
		return true;
	return false;
}

// Data-driven room-flag block: true if `room` carries any of the flags listed in
// the spell's <blocking><room_flags val="..."/></blocking>. Mirrors the per-target
// blocking helper in magic.cpp but examines the caster's room instead of the victim.
// Together with MayCastInForbiddenRoom() this replaces the hard-coded
// ROOM_FLAGGED(..., kNoMagic) check that used to live at the top of CallMagic, and
// drives the kRuneLabel fizzle that used to live in CallMagicToRoom's switch.
bool IsRoomBlocked(RoomData *room, const talents_actions::FlagCondition &cond) {
	if (!room) {
		return false;
	}
	for (const auto flag : cond.room_flags) {
		if (room->get_flag(flag)) {
			return true;
		}
	}
	return false;
}

int CalcNoisyAmount(double floor_val, double scaled, double sigma, int cap, double fixed_z) {
	const double mean = floor_val + scaled;
	const double sd = sigma * scaled;
	const int lo = std::max(0, static_cast<int>(std::floor(floor_val)));
	// issue #3631: cap ниже пола дал бы std::clamp lo>hi -- держим hi>=lo
	const int hi = (cap > 0) ? std::max(lo, cap) : std::numeric_limits<int>::max();
	// issue.potion-hotfix: a brewed potion replays its FROZEN brew-luck z (a standard normal) instead
	// of drawing -- amount = mean + z*sd -- so every quaff of the potion is identical, yet each of its
	// spells still applies its OWN sigma (via sd) to the one z. NaN = no fixed noise, draw as usual.
	if (!std::isnan(fixed_z)) {
		return std::clamp(static_cast<int>(std::lround(mean + fixed_z * sd)), lo, hi);
	}
	// GaussIntNumber's std::normal_distribution requires stddev > 0. A zero spread (scaled==0 --
	// e.g. a spell with no competence scaling, or a 0-competence caster) is deterministic: return
	// the mean rather than crashing on the assertion.
	if (sd <= 0.0) {
		return std::clamp(static_cast<int>(std::lround(mean)), lo, hi);
	}
	return GaussIntNumber(mean, sd, lo, hi);
}

// issue.potion-hotfix P3: a non-crafted potion casts as if brewed by a potion-maker with this skill
// and key-stat (kvirund's decision). A crafted potion carries its own brewed kPotionPotency instead.
constexpr int kAuthoredPotionSkill = 80;
constexpr int kAuthoredPotionKeyStat = 25;

float MagicItemPotency(const ObjData *item, ESpell spell_id) {
	const int skill = item->GetPotionValueKey(ObjVal::EValueKey::kMakerSkill);
	if (skill < 0) {   // < 0 == key ABSENT; a stored 0 is a spoiled-to-nothing potion, not legacy
		// Legacy: a pre-P3b migrated instance carries a pre-computed potency but no maker skill/stat.
		const int legacy = item->GetPotionValueKey(ObjVal::EValueKey::kPotionPotency);
		if (legacy > 0) {
			return static_cast<float>(legacy);
		}
	}
	if (spell_id <= ESpell::kUndefined) {
		return -1.0f;
	}
	// Competence = skill_coeff + stat_coeff from the MAKER's inputs (crafted: brew skill + brewer Int;
	// non-crafted: the authored maker), through THIS spell's own potency-roll weights. The drinker's
	// own skill/stat are never consulted -- a potion acts on its own.
	const int use_skill = (skill >= 0) ? skill : kAuthoredPotionSkill;   // present (incl. 0) wins; absent -> authored
	const int stat = item->GetPotionValueKey(ObjVal::EValueKey::kMakerStat);
	const int use_stat = (stat >= 0) ? stat : kAuthoredPotionKeyStat;
	const auto &roll = MUD::Spell(spell_id).GetPotencyRoll();
	return static_cast<float>(roll.CalcSkillCoeffForValue(use_skill)
			+ roll.CalcBaseStatCoeffForValue(use_stat));
}

// issue.potion-hotfix: the MAKER's skill, used to scale a potion buff's DURATION (the drinker's own
// skill is irrelevant). A crafted potion carries its brew skill (kMakerSkill); a non-crafted one has
// none, so it falls back to the authored maker's skill.
int MagicItemSkill(const ObjData *item) {
	const int stored = item->GetPotionValueKey(ObjVal::EValueKey::kMakerSkill);
	return (stored >= 0) ? stored : kAuthoredPotionSkill;   // absent (-1) -> authored; a stored 0 stays 0
}

// issue.potion-hotfix: the maker's key stat (crafted: brewer Intelligence; else the authored default).
// Needed alongside MagicItemSkill to blend two potions' stats when they are poured together.
int MagicItemStat(const ObjData *item) {
	const int stored = item->GetPotionValueKey(ObjVal::EValueKey::kMakerStat);
	return (stored >= 0) ? stored : kAuthoredPotionKeyStat;   // absent (-1) -> authored; a stored 0 stays 0
}

// issue.magic-items: перечень заклинаний предмета с их силой -- один формат для stat и опознания.
// Заклинания лежат в extra_values (сырые val[] у свитков, зелий, посохов и жезлов нулевые).
// issue #3611: см. описание в magic_utils.h.
bool IsPotencyFromProto(const CObjectPrototype *item) {
	return item->GetPotionValueKey(ObjVal::EValueKey::kMakerSkill) < 0
			&& item->GetPotionValueKey(ObjVal::EValueKey::kPotionPotency) <= 0;
}

std::vector<std::string> SpellItemSpellsWithPotency(const ObjData *item) {
	const bool from_proto = IsPotencyFromProto(item);

	std::vector<std::string> spells;
	for (int pos = 1; pos <= 3; ++pos) {
		const auto spell_id = static_cast<ESpell>(item->GetSpellItemSpellNum(pos));
		if (!MUD::Spell(spell_id).IsValid()) {
			continue;
		}
		const int potency = static_cast<int>(MagicItemPotency(item, spell_id) + 0.5f);
		// имя заклинания выделяем цветом, как в списке заклинаний моба
		spells.push_back(from_proto
				? fmt::format("{}{}{} (сила {}, базовая)",
						kColorCyn, MUD::Spell(spell_id).GetName(), kColorNrm, potency)
				: fmt::format("{}{}{} (сила {})",
						kColorCyn, MUD::Spell(spell_id).GetName(), kColorNrm, potency));
	}

	return spells;
}

float CalcCastPotency(const RollResult &potency) {
	// issue.random-noise-rework (P3): stored potency is DETERMINISTIC competence (skill+stat),
	// NOT the rolled dice. The affect's recorded strength (used by dispel contests, first-aid
	// cure priority, and re-apply "keep stronger") must reflect the caster's skill/stat, not the
	// randomness of the delivered amount. Equals CastContext::CompetenceBase() for the entry action.
	return static_cast<float>(potency.skill_coeff + potency.stat_coeff);
}

bool MayCastHere(CharData *caster, CharData *victim, ESpell spell_id) {
	int ignore;

	if (privilege::IsGrGod(caster) || !ROOM_FLAGGED(caster->in_room, ERoomFlag::kPeaceful)) {
		return true;
	}

	if (ROOM_FLAGGED(caster->in_room, ERoomFlag::kNoBattle)
		&& MUD::Spell(spell_id).IsViolentAgainst(caster, victim)) {
		return false;
	}

	ignore = MUD::Spell(spell_id).AllowTarget(kTarIgnore)
		|| MUD::Spell(spell_id).IsFlagged(kMagMasses)
		|| MUD::Spell(spell_id).IsFlagged(kMagGroups);

	if (!ignore && !victim) {
		return true;
	}

	if (ignore && !MUD::Spell(spell_id).IsFlagged(kMagMasses) &&
		!MUD::Spell(spell_id).IsFlagged(kMagGroups)) {
		if (MUD::Spell(spell_id).IsViolentAgainst(caster, victim)) {
			return false;
		}
		return victim == nullptr ? true : false;
	}

	ignore = victim && (MUD::Spell(spell_id).AllowTarget(kTarCharRoom) ||
				MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) &&
					!MUD::Spell(spell_id).IsFlagged(kMagAreas);

	for (const auto ch_vict : world[caster->in_room]->people) {
		if (privilege::IsImmortal(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (MUD::Spell(spell_id).IsViolentAgainst(caster, ch_vict) && group::same_group(caster, ch_vict))
			continue;
		if (ignore && ch_vict != victim)
			continue;
		if (MUD::Spell(spell_id).IsViolentAgainst(caster, ch_vict)) {
			if (!may_kill_here(caster, ch_vict, NoArgument)) {
				return false;
			}
		} else {
			if (!may_kill_here(caster, ch_vict->GetEnemy(), NoArgument)) {
				return false;
			}
		}
	}

	return true;
}

class SpellCastMetrics {
public:
	SpellCastMetrics(ESpell spell_id, const CharData* caster, int level,
	                 const CharData* cvict, const ObjData* ovict, const RoomData* rvict)
		: m_spell_name(MUD::Spell(spell_id).GetCName())
		, m_caster_class(MUD::Class(caster->GetClass()).GetName())
		, m_duration("spell.cast.duration", {
			{"spell_name",   m_spell_name},
			{"caster_class", m_caster_class}
		  })
	{
		(void)level; (void)cvict; (void)ovict; (void)rvict;
	}

	void send() {
		observability::OtelMetrics::RecordCounter("spell.cast.total", 1, {
			{"spell_name",   m_spell_name},
			{"caster_class", m_caster_class}
		});
	}

private:
	std::string m_spell_name;
	std::string m_caster_class;
	observability::ScopedMetric m_duration;
};

/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
// Evaluates the spell's potency roll against the caster once, so the result can be
// threaded to the cast-dispatch functions. The roll values do not depend on level;
// level is carried only to replace that parameter.
ActionContext BuildActionContext(CharData *caster, ESpell spell_id, int level, float fixed_potency,
								 float fixed_competence, double fixed_noise_z, int fixed_skill) {
	const auto &spell = MUD::Spell(spell_id);
	auto eval = [caster](const talents_actions::Roll &roll) {
		RollResult rr{roll.CalcSkillCoeff(caster),
					  roll.CalcBaseStatCoeff(caster), roll.CalcLowSkillCoeff(caster)};
		// issue.potency-noise: draw the spell's ONE shared truncated-normal z; realize d = sigma*z once,
		// shared by every manifestation (each scales it by its own weight).
		const double z = GaussDoubleNumber(0.0, 1.0, -roll.GetNoiseTrunc(), roll.GetNoiseTrunc());
		rr.noise_z = z;
		rr.noise_dev = roll.GetNoiseSigma() * z;
		return rr;
	};
	// issue.vampirism-haste: an affect-action grant (e.g. kVampirism's on-kill kHaste) has no skill roll,
	// so it scales with the FIRING affect's stored potency, fed here as competence (skill_coeff).
	// issue.potion-potency + issue.random-noise-rework (P3): a brewed-in fixed potency (>=0) from an
	// item/potion ALSO flows as COMPETENCE (skill_coeff, not dices), so CalcCastPotency (skill+stat,
	// deterministic) and the beta*C effect amounts scale with it (under P2 dices_weight=0, a dices-only
	// value would no longer scale the amount). Negative -> roll from the caster as usual.
	// issue.potion-hotfix: a brewed potion also carries a frozen brew-luck z (fixed_noise_z) and the
	// MAKER's skill (fixed_skill) for deterministic amount + maker-driven buff duration.
	RollResult bcc_roll;
	if (fixed_competence >= 0.0f) {
		bcc_roll = RollResult{static_cast<double>(fixed_competence), 0.0, 0.0};
	} else if (fixed_potency >= 0.0f) {
		bcc_roll = RollResult{static_cast<double>(fixed_potency), 0.0, 0.0, fixed_noise_z, fixed_skill};
		// issue.potency-noise: a brewed potion replays its frozen z; realize d with THIS spell's sigma.
		if (!std::isnan(fixed_noise_z)) {
			bcc_roll.noise_dev = spell.GetPotencyRoll().GetNoiseSigma() * fixed_noise_z;
		}
	} else {
		bcc_roll = eval(spell.GetPotencyRoll());
	}
	ActionContext ctx(caster, spell_id, level, bcc_roll);
	ctx.casting.insert(spell_id);  // seed the cast-chain loop guard
	return ctx;
}

ECastResult CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level, float fixed_potency, double fixed_noise_z, int fixed_skill, int dir) {
	SpellCastMetrics metrics(spell_id, caster, level, cvict, ovict, rvict);
	utils::CSteppedProfiler profiler("Spell Cast", 0.030);

	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast)
		return ECastResult::kNotCast;

	// Data-driven room block: any spell whose XML
	// <talent_actions><action><blocking><room_flags val="..."/></blocking></action>
	// matches the caster's room fizzles here, AND any spell carrying <components><weave/>
	// fizzles in a kNoMagic room (weave is the single source of
	// truth for "is this magic", replacing the data-driven kNoMagic blocking that used
	// to be duplicated across 228 spells). MayCastInForbiddenRoom() is the per-caster
	// bypass (greater gods, uncharmed NPCs). The fizzle messages live in spell_msg.xml --
	// the default sheaf carries the generic narration; spells like kRuneLabel override
	// with their own kCastForbidden* keys.
	const bool weave_blocked =
			MUD::Spell(spell_id).GetComponents().HasWeaveComponent()
			&& ROOM_FLAGGED(caster->in_room, ERoomFlag::kNoMagic);
	const bool data_blocked = IsRoomBlocked(world[caster->in_room],
			MUD::Spell(spell_id).actions.GetBlocking());
	if ((weave_blocked || data_blocked) && !MayCastInForbiddenRoom(caster)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToChar) + "\r\n", caster);
		const auto &to_room = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToRoom);
		if (!to_room.empty()) {
			act(to_room.c_str(), false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return ECastResult::kNotCast;
	}

	// issue.sight-component: a spell carrying <components><sight/> requires the caster to see -- a
	// blind caster cannot cast it. The "blinded" notice is inline for now; the generic no-effect
	// line comes from the spell's message sheaf (kNoeffect).
	if (MUD::Spell(spell_id).GetComponents().HasSightComponent() && AFF_FLAGGED(caster, EAffect::kBlind)) {
		SendMsgToChar("Вы ослеплены!\r\n", caster);
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
		return ECastResult::kNotCast;
	}

	if (!MayCastHere(caster, cvict, spell_id)) {
		// MayCastHere fizzle: per-spell sheaf with kDefault
		// fallback supplies the narration. Warcry spells override with their louder
		// variant; the default is the magic-flash line.
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastHereToChar) + "\r\n", caster);
		const auto &cant_here_room =
				MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastHereToRoom);
		if (!cant_here_room.empty()) {
			act(cant_here_room.c_str(), false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return ECastResult::kNotCast;
	}

	// kTarAllyOnly (issue cast-to-ally-only): a PC may cast such spells only on self
	// or a groupmate, whatever the cast source (command, scroll, wand, staff, ...).
	// NPCs are unrestricted; self/groupmates pass via same_group; a null/non-char
	// target falls through to the spell's normal handling.
	if (cvict && !caster->IsNpc() && MUD::Spell(spell_id).AllowTarget(kTarAllyOnly)
			&& !group::same_group(caster, cvict)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastNotAlly) + "\r\n", caster);
		return ECastResult::kNotCast;
	}

	// Spell-level caster gate: a spell is castable by the caster
	// or not, so this is checked ONCE here -- before per-target dispatch -- not per target.
	// e.g. kDispelEvil carries <caster_conditions><blocking><align val="kEvil"/></blocking>
	// so an evil caster simply can't fire it. Always emits kNoeffect to the one caster.
	if (CasterBlocked(caster, MUD::Spell(spell_id).GetCasterConditions())) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
		return ECastResult::kNotCast;
	}

	if (SpellUsage::is_active) {
		SpellUsage::AddSpellStat(caster->GetClass(), spell_id);
	}

	metrics.send();

	// Compute both rolls once, now that we know the spell is actually being cast.
	ActionContext ctx = BuildActionContext(caster, spell_id, level, fixed_potency, -1.0f, fixed_noise_z, fixed_skill);
	ctx.cvict = cvict;
	ctx.ovict = ovict;
	ctx.rvict = rvict;
	// issue.perk-action-patching: if the caster holds a perk that patches this spell, build the per-cast
	// modified action chain now, before dispatch. No-op for every other caster / every unpatched spell.
	ctx.ApplyTalentPatches();

	// issue.room-affect-trigger-improve: a cast aimed at a DIRECTION targets that passage/exit -- general,
	// not tied to any spell id. CallMagicToExit runs only the passage-meaningful stages of the spell
	// (impose an affect on the door, dispel affects already there); char-targeted stages (damage / heal /
	// summon / ...) are pointless on a doorway and are silently skipped. So `развеять магию <dir>`,
	// `огненная ловушка <dir>`, etc. all flow through here without per-spell branching.
	if (dir >= 0) {
		profiler.next_step("exit");
		ActionContext exit_ctx = ctx;
		exit_ctx.level = abs(level);
		return room_spells::CallMagicToExit(caster, dir, exit_ctx);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagAreas) || MUD::Spell(spell_id).IsFlagged(kMagMasses)) {
		profiler.next_step("area");
		ctx.level = abs(level);
		return CastSpell(ctx, ECastTargets::kFoes);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagGroups)) {
		profiler.next_step("group");
		return CastSpell(ctx, ECastTargets::kFriends);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagRoom)) {
		profiler.next_step("room");
		ActionContext room_ctx = ctx;
		room_ctx.level = abs(level);
		return room_spells::CallMagicToRoom(caster, rvict, room_ctx);
	}

	// issue.dispellbug: dispel magic cast with no character/object target dispels the
	// caster's current room (room wards like kForbidden) via CastUnaffects' room branch
	// (author/ally + strength contest). Bypasses CallMagicToRoom so the ward itself
	// cannot block its own removal.
	if (spell_id == ESpell::kDispellMagic && !cvict && !ovict) {
		profiler.next_step("room-dispel");
		// issue.affects-improve (P2): a no-target dispel goes to the ROOM, not the caster -- tell the
		// caster so the empty "no effect" result on their own buffs is not mistaken for a bug. To strip
		// your own affects, target yourself by name. The clarifier is kDispellMagic's kCustomMsgOne.
		if (const std::string &m = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCustomMsgOne);
				!m.empty()) {
			SendMsgToChar(m + "\r\n", caster);
		}
		ActionContext room_ctx = ctx;
		room_ctx.cvict = nullptr;
		room_ctx.rvict = world[caster->in_room];
		return (CastUnaffects(room_ctx) == EStageResult::kBreak)
				? ECastResult::kNotCast : ECastResult::kSuccess;
	}

	profiler.next_step("single");
	return CastSpell(ctx, ECastTargets::kSingle);
}

const char *what_sky_type[] = {"пасмурно",
							   "cloudless",
							   "облачно",
							   "cloudy",
							   "дождь",
							   "raining",
							   "ясно",
							   "lightning",
							   "\n"
};

const char *what_weapon[] = {"плеть",
							 "whip",
							 "дубина",
							 "club",
							 "\n"
};

/**
* Поиск предмета для каста локейта (без учета видимости для чара и с поиском
* как в основном списке, так и в личных хранилищах с почтой).
*/
ObjData *FindObjForLocate(CharData *ch, const char *name) {
//	ObjectData *obj = ObjectAlias::locate_object(name);
	ObjData *obj = get_obj_vis_for_locate(ch, name);
	if (!obj) {
		obj = Depot::locate_object(name);
		if (!obj) {
			obj = Parcel::locate_object(name);
		}
	}
	return obj;
}

// Sends the kNoTarget message to `ch` keyed on the cast spell with the {target}
// placeholder resolved against the spell's accepted target classes
// Object-accepting spells get "ЧТО" ("what"); char-only
// spells get "КОГО" ("whom"). Per-spell sheafs with no {target} (e.g. the
// kControlWeather "тип погоды" override) just pass through.
static void SendNoTargetMsg(ESpell spell_id, CharData *ch) {
	std::string msg = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoTarget);
	const auto pos = msg.find("{target}");
	if (pos != std::string::npos) {
		const char *what = MUD::Spell(spell_id).AllowTarget(
				kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip) ? "ЧТО" : "КОГО";
		msg.replace(pos, std::strlen("{target}"), what);
	}
	SendMsgToChar(msg + "\r\n", ch);
}

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom,
		int *dir) {
	*tch = nullptr;
	*tobj = nullptr;
	*troom = world[ch->in_room];

	// kControlWeather / kCreateWeapon used to parse their argument here (weather / weapon type);
	// that moved into the respective handlers (issue.spell-pipeline-cleaning #2/#3), so the generic
	// resolver no longer special-cases spell ids.
	strcpy(cast_argument, t);
	// issue.room-affect-trigger-improve: a kTarDirection spell targets a direction/exit. Parse the arg
	// as a RU or EN direction name; *troom stays the caster's room and *dir carries the direction.
	// issue.room-affect-trigger-improve: a kTarDirection spell whose argument is a RU/EN direction targets
	// that exit. If the arg is NOT a direction we fall through so a spell that ALSO allows other targets
	// (e.g. kDispellMagic on a char, or its no-arg room form) still resolves normally.
	if (MUD::Spell(spell_id).AllowTarget(kTarDirection) && *t) {
		int d = search_block(t, dirs_rus, false);
		if (d < 0) { d = search_block(t, dirs, false); }
		if (d >= 0) {
			if (dir) { *dir = d; }
			return true;
		}
	}
	if (MUD::Spell(spell_id).AllowTarget(kTarRoomThis))
		return true;
	if (MUD::Spell(spell_id).AllowTarget(kTarIgnore))
		return true;
	else if (*t) {
		if (MUD::Spell(spell_id).AllowTarget(kTarCharRoom)) {
			*tch = target_resolver::FindCharInRoom(ch, t);
			if ((*tch != nullptr)) {
				if (MUD::Spell(spell_id).IsViolentAgainst(ch, *tch) && !check_pkill(ch, *tch, t))
					return false;
				return true;
			}
		}
		if (MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) {
			if (!ch->IsNpc()) {
				char tmpname[kMaxInputLength];
				char *tmp = tmpname;
				strcpy(tmp, t);
				int fnum = 0;
				int tnum = get_number(&tmp);
				for (auto *k : ch->followers) {
					if (isname(tmp, k->GetCharAliases())) {
						if (++fnum == tnum) {// нашли!!
							*tch = k;
							return true;
						}
					}
				}
			}
			*tch = target_resolver::FindCharInWorld(ch, t);
			if ((*tch != nullptr)) {
				// чтобы мобов не чекали
				if (ch->IsNpc() || !(*tch)->IsNpc()) {
					if (MUD::Spell(spell_id).IsViolentAgainst(ch, *tch) && !check_pkill(ch, *tch, t))
						return false;
					else
						return true;
				}
			}
		}
		if (MUD::Spell(spell_id).AllowTarget(kTarObjInv))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != nullptr)
				return true;
		if (MUD::Spell(spell_id).AllowTarget(kTarObjEquip)) {
			int tmp;
			if ((*tobj = get_object_in_equip_vis(ch, t, ch->equipment, &tmp)) != nullptr)
				return true;
		}
		if (MUD::Spell(spell_id).AllowTarget(kTarObjRoom))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != nullptr)
				return true;
		if (MUD::Spell(spell_id).AllowTarget(kTarObjWorld)) {
//			if ((*tobj = get_obj_vis(ch, t)) != NULL)
//				return true;
			if (spell_id == ESpell::kLocateObject) {
				*tobj = FindObjForLocate(ch, t);
			} else {
				*tobj = target_resolver::FindObjAround(ch, t);
			}
			if (*tobj) {
				return true;
			}
		}
	} else {
		if (MUD::Spell(spell_id).AllowTarget(kTarFightSelf))
			if (ch->GetEnemy() != nullptr) {
				*tch = ch;
				return true;
			}
		if (MUD::Spell(spell_id).AllowTarget(kTarFightVict))
			if (ch->GetEnemy() != nullptr) {
				*tch = ch->GetEnemy();
				return true;
			}
		// issue.dispellbug: dispel magic (kDispellMagic) with no character target (out
		// of combat) dispels the caster's current room (room wards) instead of defaulting
		// to self. tch stays null -> CallMagic routes to the room-dispel path. In combat
		// the kTarFightSelf branch above already targets self (cleanse convenience).
		if (spell_id == ESpell::kDispellMagic) {
			*tch = nullptr;
			*troom = world[ch->in_room];
			return true;
		}
		if (MUD::Spell(spell_id).AllowTarget(kTarCharRoom) && !MUD::Spell(spell_id).IsViolent()) {
			*tch = ch;
			return true;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	// Warcry vs regular phrasing is data-driven through the per-spell kNoTarget
	// override: warcry sheaves carry the "так громко
	// крикнуть" variant; the kDefault sheaf has the regular line.
	SendNoTargetMsg(spell_id, ch);
	return false;
}

/*
 * CastSpell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */
ECastResult CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst, int dir) {
	if (spell_id == ESpell::kUndefined) {
		log("SYSERR: CastSpell trying to call spell id %d.\n", to_underlying(spell_id));
		return ECastResult::kNotCast;
	}

	if (tch && ch) {
		if (tch->IsNpc() && ch->IsNpc() && !alignment::SameAlign(ch, tch) && !MUD::Spell(spell_id).IsViolent()) {
			return ECastResult::kNotCast;
		}
	}

	if (!troom) {
		troom = world[ch->in_room];
	}

	if (ch->GetPosition() < MUD::Spell(spell_id).GetMinPos()) {
		switch (ch->GetPosition()) {
			case EPosition::kSleep: SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastSleeping) + "\r\n", ch);
				break;
			case EPosition::kRest: SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastResting) + "\r\n", ch);
				break;
			case EPosition::kSit: SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastSitting) + "\r\n", ch);
				break;
			case EPosition::kFight: SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastFighting) + "\r\n", ch);
				break;
			default: SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPosition) + "\r\n", ch);
				break;
		}
		return ECastResult::kNotCast;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_master() == tch) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastMaster) + "\r\n", ch);
		return ECastResult::kNotCast;
	}

	// Verbal-component gate: CastSpell is the
	// universal entry point for "spoken" casts (PC do_cast, NPC specprocs,
	// queued combat casts via process_player_attack, ...). Refuse only
	// verbal spells under kSilence; non-verbal spells fall through.
	if (MUD::Spell(spell_id).IsVerbal() && AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastSilenced) + "\r\n", ch);
		return ECastResult::kNotCast;
	}

	// Material-component gate: a spoken cast consumes its material component(s)
	// here, at launch -- alongside the verbal gate -- NOT inside CastAffect. CastAffect also runs
	// for casts from equipment / scrolls / procs (which reach CallMagic directly, bypassing this
	// function) and those must not require components. A missing component refuses the cast.
	if (ProcessMatComponents(ch, ch, spell_id) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}

	if (tch != ch && !privilege::IsImmortal(ch) && MUD::Spell(spell_id).AllowTarget(kTarSelfOnly)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastSelfOnly) + "\r\n", ch);
		return ECastResult::kNotCast;
	}

	if (tch == ch && MUD::Spell(spell_id).AllowTarget(kTarNotSelf)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastNotSelf) + "\r\n", ch);
		return ECastResult::kNotCast;
	}

	if ((!tch || tch->in_room == kNowhere) && !tobj && !troom &&
		MUD::Spell(spell_id).AllowTarget(kTarCharRoom | kTarCharWorld | kTarFightSelf | kTarFightVict |
			kTarObjInv | kTarObjRoom | kTarObjWorld | kTarObjEquip | kTarRoomThis | kTarRoomDir)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kTargetUnavailable) + "\r\n", ch);
		return ECastResult::kNotCast;
	}

	if (tch != nullptr && tch->in_room != ch->in_room) {
		if (!MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kTargetUnavailable) + "\r\n", ch);
			return ECastResult::kNotCast;
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kPeaceful)) {
		auto ignore = MUD::Spell(spell_id).AllowTarget(kTarIgnore) ||
			MUD::Spell(spell_id).IsFlagged(kMagMasses) ||
			MUD::Spell(spell_id).IsFlagged(kMagGroups);
		if (ignore) { // индивидуальная цель
			// Peaceful blocks anything that *could* be aggressive,
			// including kAmbiguous, because mass/area/no-target casts have no single victim
			// to resolve the relationship against -- the conservative read is "this might
			// hit an outsider".
			if (MUD::Spell(spell_id).GetViolent() != spells::EViolent::kNo) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPeaceful) + "\r\n", ch);
				return ECastResult::kNotCast;    // нельзя злые кастовать
			}
		}
		for (const auto ch_vict : world[ch->in_room]->people) {
			if (MUD::Spell(spell_id).IsViolentAgainst(ch, ch_vict)) {
				if (ch_vict == tch) {
					SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPeaceful) + "\r\n", ch);
					return ECastResult::kNotCast;
				}
			} else {
				if (ch_vict == tch && !group::same_group(ch, ch_vict)) {
					SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPeaceful) + "\r\n", ch);
					return ECastResult::kNotCast;
				}
			}
		}
	}

	if (auto skill_id = MUD::Spell(spell_id).GetSuccessRoll().GetBaseSkill(); skill_id != ESkill::kUndefined) {
		TrainSkill(ch, skill_id, true, tch);
	}
	// Комнату тут в SaySpell не обрабатываем - будет сказал "что-то"
	SaySpell(ch, spell_id, tch, tobj);
	if (GET_SPELL_MEM(ch, spell_subst) > 0) {
		GET_SPELL_MEM(ch, spell_subst)--;
	} else {
		GET_SPELL_MEM(ch, spell_subst) = 0;
	}

	if (!ch->IsNpc() && !privilege::IsImmortal(ch) && ch->IsFlagged(EPrf::kAutomem)) {
		MemQ_remember(ch, spell_subst);
	}

	if (ch->IsNpc()) {
		ch->caster_level -= (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE) ? 1 : 0);
	} else {
		affect_total(ch);
	}

	return (CallMagic(ch, tch, tobj, troom, spell_id, GetRealLevel(ch), -1.0f, std::numeric_limits<double>::quiet_NaN(), -1, dir));
}

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id) {
	if (privilege::IsImmortal(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		return true;
	}

	int prob;
	// \todo Svent: Это очевидно какой-то тупой костыль, но пока не буду исправлять.
	switch (saving) {
		case ESaving::kStability:
			prob = wis_bonus(GetRealWis(ch), WIS_FAILS) + GET_CAST_SUCCESS(ch);
			if ((IsMage(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ERoomFlag::kForMages))
				|| (IS_SORCERER(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ERoomFlag::kForSorcerers))
				|| (IS_PALADINE(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ERoomFlag::kForPaladines))
				|| (IS_MERCHANT(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ERoomFlag::kForMerchants))) {
				prob += 10;
			}
			break;
		default:
			prob = 100;
			break;
	}

	if (IsEquipInMetall(ch) && !CanUseFeat(ch, EFeat::kCombatCasting)) {
		prob -= 50;
	}

	prob = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_SUCCESS, prob);
	// God-flag prob modifiers: the violent vs helpful question
	// is per-target, so victim's relationship to the caster picks the side for an A spell.
	const bool aggressive_cast = victim && MUD::Spell(spell_id).IsViolentAgainst(ch, victim);
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) ||
		(aggressive_cast && GET_GOD_FLAG(victim, EGf::kGodsLike)) ||
		(!aggressive_cast && victim && GET_GOD_FLAG(victim, EGf::kGodscurse))) {
		prob -= 50;
	}

	if ((aggressive_cast && GET_GOD_FLAG(victim, EGf::kGodscurse)) ||
		(!aggressive_cast && victim && GET_GOD_FLAG(victim, EGf::kGodsLike))) {
		prob += 50;
	}

	if (ch->IsNpc() && (GetRealLevel(ch) >= kStrongMobLevel)) {
		prob += GetRealLevel(ch) - 20;
	}

	const ESkill skill_number = MUD::Spell(spell_id).GetSuccessRoll().GetBaseSkill();
	if (skill_number != ESkill::kUndefined) {
		prob += GetSkill(ch, skill_number) / 20;
	}

	return (prob > number(0, 100));
}


// issue.handler-cleaning (Bucket 3): weapon cast-affect helper (was file-local in handler).
void CastWeaponAffect(CharData *ch, ESpell spell_id) {
	ActionContext ctx(ch, spell_id, GetRealLevel(ch), {});
	ctx.cvict = ch;
	CastAffect(ctx);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
