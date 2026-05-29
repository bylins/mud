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

#include "gameplay/mechanics/groups.h"
#include "engine/db/global_objects.h"
#include "engine/core/handler.h"
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

extern int what_sky;


int MagusCastRequiredLevel(const CharData *ch, ESpell spell_id) {
	int required_level;
	if (spell_create.contains(spell_id)) {
		required_level = spell_create[spell_id].runes.min_caster_level;
	} else {
		return 999;
	}
	if (required_level >= kLvlGod)
		return required_level;
	if (CanUseFeat(ch, EFeat::kSecretRunes)) {
		required_level -= GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement();
	}
	return std::max(1, required_level);
}

// SaySpell erodes buf, buf1, buf2
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj) {
	char lbuf[256];
	const char *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
		*helpee_vict, *damagee_vict, *format;

	// Silenced caster can't speak the phrase regardless of whether the spell
	// is verbal. A verbal spell shouldn't even reach SaySpell while silenced
	// (do_cast / CastSpell / process_player_attack bail out earlier), but
	// a non-verbal spell will, and still has no business announcing prose.
	// The cast itself continues; only the spoken phrase + room narration
	// are suppressed. (issue.spellcomponents.)
	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		return;
	}

	*buf = '\0';
	strcpy(lbuf, MUD::Spell(spell_id).GetEngCName());
	// Say phrase ?
	const auto &cast_phrase_sheaf = MUD::SpellMessages()[spell_id];
	if (!cast_phrase_sheaf.HasMessage(ESpellMsg::kCastPhraseHeathen)
		&& !cast_phrase_sheaf.HasMessage(ESpellMsg::kCastPhraseChristian)) {
		// A non-verbal spell may legitimately ship no cast phrase -- the
		// caster simply makes no sound here. A verbal spell with no phrase
		// IS a content gap, so keep the CMP-level mudlog for that case.
		// (issue.spellcomponents: cast phrase is decorative for non-verbal
		// spells. Speak it if present; stay silent otherwise.)
		if (MUD::Spell(spell_id).IsVerbal()) {
			sprintf(buf, "[ERROR]: SaySpell: для спелла %d не объявлена cast_phrase", to_underlying(spell_id));
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		}
		return;
	}
	if (ch->IsNpc()) {
		switch (GET_RACE(ch)) {
			case ENpcRace::kBoggart:
			case ENpcRace::kGhost:
			case ENpcRace::kHuman:
			case ENpcRace::kZombie:
			case ENpcRace::kSpirit: {
				const int religion = number(kReligionPoly, kReligionMono);
				const std::string &cast_phrase = cast_phrase_sheaf.GetMessage(religion ? ESpellMsg::kCastPhraseChristian : ESpellMsg::kCastPhraseHeathen);
				if (!cast_phrase.empty()) {
					strcpy(buf, cast_phrase.c_str());
				}
				say_to_self = "$n пробормотал$g : '%s'.";
				say_to_other = "$n взглянул$g на $N3 и бросил$g : '%s'.";
				say_to_obj_vis = "$n глянул$g на $o3 и произнес$q : '%s'.";
				say_to_something = "$n произнес$q : '%s'.";
				damagee_vict = "$n зыркнул$g на вас и проревел$g : '%s'.";
				helpee_vict = "$n улыбнул$u вам и произнес$q : '%s'.";
				break;
			}
			default: say_to_self = "$n издал$g непонятный звук.";
				say_to_other = "$n издал$g непонятный звук.";
				say_to_obj_vis = "$n издал$g непонятный звук.";
				say_to_something = "$n издал$g непонятный звук.";
				damagee_vict = "$n издал$g непонятный звук.";
				helpee_vict = "$n издал$g непонятный звук.";
		}
	} else {
		if (ch->IsFlagged(EPrf::kNoRepeat)) {
			if (!ch->GetEnemy()) {
				SendMsgToChar(OK, ch);
			}
		} else {
			if (MUD::Spell(spell_id).IsFlagged(kMagWarcry))
				sprintf(buf, "Вы выкрикнули \"%s%s%s\".\r\n",
						MUD::Spell(spell_id).IsViolent() ? kColorBoldRed : kColorBoldGrn,
						MUD::Spell(spell_id).GetCName(), kColorNrm);
			else
				sprintf(buf, "Вы произнесли заклинание \"%s%s%s\".\r\n",
						MUD::Spell(spell_id).IsViolent() ? kColorBoldRed : kColorBoldGrn,
						MUD::Spell(spell_id).GetCName(), kColorNrm);
			SendMsgToChar(buf, ch);
		}
		const std::string &cast_phrase = cast_phrase_sheaf.GetMessage(GET_RELIGION(ch) ? ESpellMsg::kCastPhraseChristian : ESpellMsg::kCastPhraseHeathen);
		if (!cast_phrase.empty()) {
			strcpy(buf, cast_phrase.c_str());
		}
		say_to_self = "$n прикрыл$g глаза и прошептал$g : '%s'.";
		say_to_other = "$n взглянул$g на $N3 и произнес$q : '%s'.";
		say_to_obj_vis = "$n посмотрел$g на $o3 и произнес$q : '%s'.";
		say_to_something = "$n произнес$q : '%s'.";
		damagee_vict = "$n зыркнул$g на вас и произнес$q : '%s'.";
		helpee_vict = "$n подмигнул$g вам и произнес$q : '%s'.";
	}

	if (tch != nullptr && tch->in_room == ch->in_room) {
		if (tch == ch) {
			format = say_to_self;
		} else {
			format = say_to_other;
		}
	} else if (tobj != nullptr && (tobj->get_in_room() == ch->in_room || tobj->get_carried_by() == ch)) {
		format = say_to_obj_vis;
	} else {
		format = say_to_something;
	}

	sprintf(buf1, format, MUD::Spell(spell_id).GetCName());
	sprintf(buf2, format, buf);

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
		if (MUD::Spell(spell_id).IsViolent()) {
			sprintf(buf1, damagee_vict,
					IS_SET(GET_SPELL_TYPE(tch, spell_id), ESpellType::kKnow | ESpellType::kTemp) ?
					MUD::Spell(spell_id).GetCName() : buf);
		} else {
			sprintf(buf1, helpee_vict,
					IS_SET(GET_SPELL_TYPE(tch, spell_id), ESpellType::kKnow | ESpellType::kTemp) ?
					MUD::Spell(spell_id).GetCName() : buf);
		}
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
	for (const auto &spell : MUD::Spells()) {
		if (spell.IsInvalid()) {
			continue;
		}
		if (utils::IsEquivalent(name, spell.GetName())) {
			return spell.GetId();
		}
	}

	return ESpell::kUndefined;
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
	if (caster->IsGrGod()) {
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

float CalcCastPotency(const RollResult &potency) {
	return static_cast<float>(potency.dices + potency.skill_coeff + potency.stat_coeff);
}

int ComputeApplyModifier(const talents_actions::TalentAffect::Apply &apply,
						 const RollResult &potency) {
	const double competencies = potency.skill_coeff + potency.stat_coeff;
	double raw = apply.min + std::ceil(competencies * apply.competencies_weight
									   + potency.dices * apply.dices_weight);
	// Optional cap on raw magnitude before factor (issue.modifier-cap). 0 = no cap.
	// Clamps the buff/debuff magnitude regardless of factor sign.
	if (apply.cap > 0) {
		raw = std::min(raw, static_cast<double>(apply.cap));
	}
	return static_cast<int>(apply.factor * raw);
}

bool MayCastHere(CharData *caster, CharData *victim, ESpell spell_id) {
	int ignore;

	if (caster->IsGrGod() || !ROOM_FLAGGED(caster->in_room, ERoomFlag::kPeaceful)) {
		return true;
	}

	if (ROOM_FLAGGED(caster->in_room, ERoomFlag::kNoBattle) && MUD::Spell(spell_id).IsViolent()) {
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
		if (MUD::Spell(spell_id).IsViolent()) {
			return false;
		}
		return victim == nullptr ? true : false;
	}

	ignore = victim && (MUD::Spell(spell_id).AllowTarget(kTarCharRoom) ||
				MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) &&
					!MUD::Spell(spell_id).IsFlagged(kMagAreas);

	for (const auto ch_vict : world[caster->in_room]->people) {
		if (ch_vict->IsImmortal())
			continue;
		if (!HERE(ch_vict))
			continue;
		if (MUD::Spell(spell_id).IsViolent() && group::same_group(caster, ch_vict))
			continue;
		if (ignore && ch_vict != victim)
			continue;
		if (MUD::Spell(spell_id).IsViolent()) {
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
// Evaluates the spell's success and potency rolls against the caster once, so the
// result can be threaded to the cast-dispatch functions (issue #3333). The roll
// values do not depend on level; level is carried only to replace that parameter.
CastRollResult ComputeCastRoll(CharData *caster, ESpell spell_id, int level) {
	const auto &spell = MUD::Spell(spell_id);
	auto eval = [caster](const talents_actions::Roll &roll) {
		return RollResult{roll.RollSkillDices(), roll.CalcSkillCoeff(caster),
						  roll.CalcBaseStatCoeff(caster), roll.CalcLowSkillCoeff(caster)};
	};
	return CastRollResult{spell_id, level, eval(spell.GetSuccessRoll()), eval(spell.GetPotencyRoll())};
}

int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level) {
	SpellCastMetrics metrics(spell_id, caster, level, cvict, ovict, rvict);
	utils::CSteppedProfiler profiler("Spell Cast", 0.030);

	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast)
		return 0;

	// Data-driven room block (issue.room-affects): any spell whose XML
	// <talent_actions><action><blocking><room_flags val="kNoMagic|..."/></blocking></action>
	// matches the caster's room fizzles here. MayCastInForbiddenRoom() is the
	// per-caster bypass (greater gods, uncharmed NPCs). The fizzle messages live
	// in spell_msg.xml -- the default sheaf carries the generic narration; spells
	// like kRuneLabel override with their own kCastForbidden* keys.
	if (IsRoomBlocked(world[caster->in_room], MUD::Spell(spell_id).actions.GetBlocking())
			&& !MayCastInForbiddenRoom(caster)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToChar) + "\r\n", caster);
		const auto &to_room = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToRoom);
		if (!to_room.empty()) {
			act(to_room.c_str(), false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return 0;
	}

	if (!MayCastHere(caster, cvict, spell_id)) {
		if (MUD::Spell(spell_id).IsFlagged(kMagWarcry)) {
			SendMsgToChar("Ваш громовой глас сотряс воздух, но ничего не произошло!\r\n", caster);
			act("Вы вздрогнули от неожиданного крика, но ничего не произошло.",
				false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		} else {
			SendMsgToChar("Ваша магия обратилась всего лишь в яркую вспышку!\r\n", caster);
			act("Яркая вспышка на миг осветила комнату, и тут же погасла.",
				false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return 0;
	}

	// kTarAllyOnly (issue cast-to-ally-only): a PC may cast such spells only on self
	// or a groupmate, whatever the cast source (command, scroll, wand, staff, ...).
	// NPCs are unrestricted; self/groupmates pass via same_group; a null/non-char
	// target falls through to the spell's normal handling.
	if (cvict && !caster->IsNpc() && MUD::Spell(spell_id).AllowTarget(kTarAllyOnly)
			&& !group::same_group(caster, cvict)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastNotAlly) + "\r\n", caster);
		return 0;
	}

	if (SpellUsage::is_active) {
		SpellUsage::AddSpellStat(caster->GetClass(), spell_id);
	}

	metrics.send();

	// Compute both rolls once, now that we know the spell is actually being cast.
	const auto roll = ComputeCastRoll(caster, spell_id, level);

	if (MUD::Spell(spell_id).IsFlagged(kMagAreas) || MUD::Spell(spell_id).IsFlagged(kMagMasses)) {
		profiler.next_step("area");
		CastRollResult area_roll = roll;
		area_roll.level = abs(level);
		return CallMagicToArea(caster, cvict, rvict, area_roll);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagGroups)) {
		profiler.next_step("group");
		return CallMagicToGroup(caster, roll);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagRoom)) {
		profiler.next_step("room");
		CastRollResult room_roll = roll;
		room_roll.level = abs(level);
		return room_spells::CallMagicToRoom(caster, rvict, room_roll);
	}

	profiler.next_step("single");
	return CastToSingleTarget(caster, cvict, ovict, roll);
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

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom) {
	*tch = nullptr;
	*tobj = nullptr;
	*troom = world[ch->in_room];

	if (spell_id == ESpell::kControlWeather) {
		if ((what_sky = search_block(t, what_sky_type, false)) < 0) {
			SendMsgToChar("Не указан тип погоды.\r\n", ch);
			return false;
		} else
			what_sky >>= 1;
	}
	if (spell_id == ESpell::kCreateWeapon) {
		if ((what_sky = search_block(t, what_weapon, false)) < 0) {
			SendMsgToChar("Не указан тип оружия.\r\n", ch);
			return false;
		} else
			what_sky = 5 + (what_sky >> 1);
	}
	strcpy(cast_argument, t);
	if (MUD::Spell(spell_id).AllowTarget(kTarRoomThis))
		return true;
	if (MUD::Spell(spell_id).AllowTarget(kTarIgnore))
		return true;
	else if (*t) {
		if (MUD::Spell(spell_id).AllowTarget(kTarCharRoom)) {
			if ((*tch = get_char_vis(ch, t, EFind::kCharInRoom)) != nullptr) {
				if (MUD::Spell(spell_id).IsViolent() && !check_pkill(ch, *tch, t))
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
			if ((*tch = get_char_vis(ch, t, EFind::kCharInWorld)) != nullptr) {
				// чтобы мобов не чекали
				if (ch->IsNpc() || !(*tch)->IsNpc()) {
					if (MUD::Spell(spell_id).IsViolent() && !check_pkill(ch, *tch, t))
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
				*tobj = get_obj_vis(ch, t);
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
		if (MUD::Spell(spell_id).AllowTarget(kTarCharRoom) && !MUD::Spell(spell_id).IsViolent()) {
			*tch = ch;
			return true;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	if (MUD::Spell(spell_id).IsFlagged(kMagWarcry))
		sprintf(buf, "И на %s же вы хотите так громко крикнуть?\r\n",
				MUD::Spell(spell_id).AllowTarget(kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip)
				? "ЧТО" : "КОГО");
	else
		sprintf(buf, "На %s Вы хотите ЭТО колдовать?\r\n",
				MUD::Spell(spell_id).AllowTarget(kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip)
				? "ЧТО" : "КОГО");
	SendMsgToChar(buf, ch);
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
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst) {
	if (spell_id == ESpell::kUndefined) {
		log("SYSERR: CastSpell trying to call spell id %d.\n", to_underlying(spell_id));
		return 0;
	}

	if (tch && ch) {
		if (tch->IsNpc() && ch->IsNpc() && !SAME_ALIGN(ch, tch) && !MUD::Spell(spell_id).IsViolent()) {
			return 0;
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
		return 0;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_master() == tch) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastMaster) + "\r\n", ch);
		return 0;
	}

	// Verbal-component gate (issue.spellcomponents): CastSpell is the
	// universal entry point for "spoken" casts (PC do_cast, NPC specprocs,
	// queued combat casts via process_player_attack, ...). Refuse only
	// verbal spells under kSilence; non-verbal spells fall through.
	if (MUD::Spell(spell_id).IsVerbal() && AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastSilenced) + "\r\n", ch);
		return 0;
	}

	if (tch != ch && !ch->IsImmortal() && MUD::Spell(spell_id).AllowTarget(kTarSelfOnly)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastSelfOnly) + "\r\n", ch);
		return 0;
	}

	if (tch == ch && MUD::Spell(spell_id).AllowTarget(kTarNotSelf)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastNotSelf) + "\r\n", ch);
		return 0;
	}

	if ((!tch || tch->in_room == kNowhere) && !tobj && !troom &&
		MUD::Spell(spell_id).AllowTarget(kTarCharRoom | kTarCharWorld | kTarFightSelf | kTarFightVict |
			kTarObjInv | kTarObjRoom | kTarObjWorld | kTarObjEquip | kTarRoomThis | kTarRoomDir)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kTargetUnavailable) + "\r\n", ch);
		return 0;
	}

	if (tch != nullptr && tch->in_room != ch->in_room) {
		if (!MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kTargetUnavailable) + "\r\n", ch);
			return 0;
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kPeaceful)) {
		auto ignore = MUD::Spell(spell_id).AllowTarget(kTarIgnore) ||
			MUD::Spell(spell_id).IsFlagged(kMagMasses) ||
			MUD::Spell(spell_id).IsFlagged(kMagGroups);
		if (ignore) { // индивидуальная цель
			if (MUD::Spell(spell_id).IsViolent()) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPeaceful) + "\r\n", ch);
				return false;    // нельзя злые кастовать
			}
		}
		for (const auto ch_vict : world[ch->in_room]->people) {
			if (MUD::Spell(spell_id).IsViolent()) {
				if (ch_vict == tch) {
					SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPeaceful) + "\r\n", ch);
					return false;
				}
			} else {
				if (ch_vict == tch && !group::same_group(ch, ch_vict)) {
					SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastPeaceful) + "\r\n", ch);
					return false;
				}
			}
		}
	}

	if (auto skill_id = GetMagicSkillId(spell_id); skill_id != ESkill::kUndefined) {
		TrainSkill(ch, skill_id, true, tch);
	}
	// Комнату тут в SaySpell не обрабатываем - будет сказал "что-то"
	SaySpell(ch, spell_id, tch, tobj);
	if (GET_SPELL_MEM(ch, spell_subst) > 0) {
		GET_SPELL_MEM(ch, spell_subst)--;
	} else {
		GET_SPELL_MEM(ch, spell_subst) = 0;
	}

	if (!ch->IsNpc() && !ch->IsImmortal() && ch->IsFlagged(EPrf::kAutomem)) {
		MemQ_remember(ch, spell_subst);
	}

	if (ch->IsNpc()) {
		ch->caster_level -= (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE) ? 1 : 0);
	} else {
		affect_total(ch);
	}

	return (CallMagic(ch, tch, tobj, troom, spell_id, GetRealLevel(ch)));
}

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id) {
	if (ch->IsImmortal() || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
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
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) ||
		(MUD::Spell(spell_id).IsViolent() && victim && GET_GOD_FLAG(victim, EGf::kGodsLike)) ||
		(!MUD::Spell(spell_id).IsViolent() && victim && GET_GOD_FLAG(victim, EGf::kGodscurse))) {
		prob -= 50;
	}

	if ((MUD::Spell(spell_id).IsViolent() && victim && GET_GOD_FLAG(victim, EGf::kGodscurse)) ||
		(!MUD::Spell(spell_id).IsViolent() && victim && GET_GOD_FLAG(victim, EGf::kGodsLike))) {
		prob += 50;
	}

	if (ch->IsNpc() && (GetRealLevel(ch) >= kStrongMobLevel)) {
		prob += GetRealLevel(ch) - 20;
	}

	const ESkill skill_number = GetMagicSkillId(spell_id);
	if (skill_number != ESkill::kUndefined) {
		prob += ch->GetSkill(skill_number) / 20;
	}

	return (prob > number(0, 100));
}

EResist GetResisTypeWithElement(EElement element) {
	switch (element) {
		case EElement::kFire: return EResist::kFire;
		case EElement::kDark: return EResist::kDark;
		case EElement::kAir: return EResist::kAir;
		case EElement::kWater: return EResist::kWater;
		case EElement::kEarth: return EResist::kEarth;
		case EElement::kLight: return EResist::kVitality;
		case EElement::kMind: return EResist::kMind;
		case EElement::kLife: return EResist::kImmunity;
		default: return EResist::kVitality;
	}
};

EResist GetResistType(ESpell spell_id) {
	return GetResisTypeWithElement(MUD::Spell(spell_id).GetElement());
}

int ApplyResist(CharData *ch, EResist resist_type, int value) {
	int resistance = GET_RESIST(ch, resist_type);
	if (resistance <= 0) {
		return value - resistance * value / 100;
	}
	if (!ch->IsNpc()) {
		resistance = std::min(kMaxPcResist, resistance);
	}
	auto result = static_cast<int>(value - (resistance + number(0, resistance)) * value / 200.0);
	return std::max(0, result);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
