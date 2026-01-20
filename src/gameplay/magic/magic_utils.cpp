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
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/statistics/spell_usage.h"
#include "utils/backtrace.h"

#include <third_party_libs/fmt/include/fmt/format.h>

char cast_argument[kMaxStringLength];

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

	*buf = '\0';
	strcpy(lbuf, MUD::Spell(spell_id).GetEngCName());
	// Say phrase ?
	const auto &cast_phrase_list = GetCastPhrase(spell_id);
	if (!cast_phrase_list) {
		sprintf(buf, "[ERROR]: SaySpell: для спелла %d не объявлена cast_phrase", to_underlying(spell_id));
		mudlog(buf, CMP, kLvlGod, SYSLOG, true);
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
				const std::string &cast_phrase = religion ? cast_phrase_list->text_for_christian : cast_phrase_list->text_for_heathen;
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
		const std::string &cast_phrase = GET_RELIGION(ch) ? cast_phrase_list->text_for_christian : cast_phrase_list->text_for_heathen;
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

bool MayCastInNomagic(CharData *caster, ESpell spell_id) {
	if (IS_GRGOD(caster) || MUD::Spell(spell_id).IsFlagged(kMagWarcry)) {
		return true;
	}
	if (caster->IsNpc() &&
		!(AFF_FLAGGED(caster, EAffect::kCharmed) || caster->IsFlagged(EMobFlag::kTutelar)))
		return true;
	return false;
}

bool MayCastHere(CharData *caster, CharData *victim, ESpell spell_id) {
	int ignore;

	if (IS_GRGOD(caster) || !ROOM_FLAGGED(caster->in_room, ERoomFlag::kPeaceful)) {
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
		if (IS_IMMORTAL(ch_vict))
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

/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level) {

	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast)
		return 0;

	if (ROOM_FLAGGED(caster->in_room, ERoomFlag::kNoMagic) && !MayCastInNomagic(caster, spell_id)) {
		SendMsgToChar("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", caster);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.",
			false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
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

	if (SpellUsage::is_active) {
		SpellUsage::AddSpellStat(caster->GetClass(), spell_id);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagAreas) || MUD::Spell(spell_id).IsFlagged(kMagMasses)) {
		return CallMagicToArea(caster, cvict, rvict, spell_id, abs(level));
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagGroups)) {
		return CallMagicToGroup(level, caster, spell_id);
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagRoom)) {
		return room_spells::CallMagicToRoom(abs(level), caster, rvict, spell_id);
	}

	return CastToSingleTarget(level, caster, cvict, ovict, spell_id);
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
				struct FollowerType *k, *k_next;
				char tmpname[kMaxInputLength];
				char *tmp = tmpname;
				strcpy(tmp, t);
				int fnum = 0;
				int tnum = get_number(&tmp);
				for (k = ch->followers; k; k = k_next) {
					k_next = k->next;
					if (isname(tmp, k->follower->GetCharAliases())) {
						if (++fnum == tnum) {// нашли!!
							*tch = k->follower;
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
		if (IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !MUD::Spell(spell_id).IsViolent()) {
			return 0;
		}
	}

	if (!troom) {
		troom = world[ch->in_room];
	}

	if (ch->GetPosition() < MUD::Spell(spell_id).GetMinPos()) {
		switch (ch->GetPosition()) {
			case EPosition::kSleep: SendMsgToChar("Вы спите и не могете думать больше ни о чем.\r\n", ch);
				break;
			case EPosition::kRest: SendMsgToChar("Вы расслаблены и отдыхаете. И далась вам эта магия?\r\n", ch);
				break;
			case EPosition::kSit: SendMsgToChar("Похоже, в этой позе Вы много не наколдуете.\r\n", ch);
				break;
			case EPosition::kFight: SendMsgToChar("Невозможно! Вы сражаетесь! Это вам не шухры-мухры.\r\n", ch);
				break;
			default: SendMsgToChar("Вам вряд ли это удастся.\r\n", ch);
				break;
		}
		return 0;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_master() == tch) {
		SendMsgToChar("Вы не посмеете поднять руку на вашего повелителя!\r\n", ch);
		return 0;
	}

	if (tch != ch && !IS_IMMORTAL(ch) && MUD::Spell(spell_id).AllowTarget(kTarSelfOnly)) {
		SendMsgToChar("Вы можете колдовать это только на себя!\r\n", ch);
		return 0;
	}

	if (tch == ch && MUD::Spell(spell_id).AllowTarget(kTarNotSelf)) {
		SendMsgToChar("Колдовать? ЭТО? На себя?! Да вы с ума сошли!\r\n", ch);
		return 0;
	}

	if ((!tch || tch->in_room == kNowhere) && !tobj && !troom &&
		MUD::Spell(spell_id).AllowTarget(kTarCharRoom | kTarCharWorld | kTarFightSelf | kTarFightVict |
			kTarObjInv | kTarObjRoom | kTarObjWorld | kTarObjEquip | kTarRoomThis | kTarRoomDir)) {
		SendMsgToChar("Цель заклинания недоступна.\r\n", ch);
		return 0;
	}

	if (tch != nullptr && tch->in_room != ch->in_room) {
		if (!MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) {
			SendMsgToChar("Цель заклинания недоступна.\r\n", ch);
			return 0;
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kPeaceful)) {
		auto ignore = MUD::Spell(spell_id).AllowTarget(kTarIgnore) ||
			MUD::Spell(spell_id).IsFlagged(kMagMasses) ||
			MUD::Spell(spell_id).IsFlagged(kMagGroups);
		if (ignore) { // индивидуальная цель
			if (MUD::Spell(spell_id).IsViolent()) {
				SendMsgToChar("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
				return false;    // нельзя злые кастовать
			}
		}
		for (const auto ch_vict : world[ch->in_room]->people) {
			if (MUD::Spell(spell_id).IsViolent()) {
				if (ch_vict == tch) {
					SendMsgToChar("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return false;
				}
			} else {
				if (ch_vict == tch && !group::same_group(ch, ch_vict)) {
					SendMsgToChar("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
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

	if (!ch->IsNpc() && !IS_IMMORTAL(ch) && ch->IsFlagged(EPrf::kAutomem)) {
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
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
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
