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

#include "game_classes/classes.h"
#include "structs/global_objects.h"
#include "handler.h"
#include "color.h"
#include "depot.h"
#include "communication/parcel.h"
#include "magic.h"

char cast_argument[kMaxStringLength];

extern int what_sky;


int CalcRequiredLevel(const CharData *ch, ESpell spell_id) {
	int required_level = spell_create[spell_id].runes.min_caster_level;

	if (required_level >= kLvlGod)
		return required_level;
	if (IsAbleToUseFeat(ch, EFeat::kSecretRunes)) {
		int remort = GET_REAL_REMORT(ch);
		required_level -= MIN(8, MAX(0, ((remort - 8) / 3) * 2 + (remort > 7 && remort < 11 ? 1 : 0)));
	}

	return std::max(1, required_level);
}

// SaySpell erodes buf, buf1, buf2
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj) {
	char lbuf[256];
	const char *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
		*helpee_vict, *damagee_vict, *format;

	*buf = '\0';
	strcpy(lbuf, spell_info[spell_id].syn);
	// Say phrase ?
	const auto &cast_phrase_list = GetCastPhrase(spell_id);
	if (!cast_phrase_list) {
		sprintf(buf, "[ERROR]: SaySpell: для спелла %d не объявлена cast_phrase", spell_id);
		mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		return;
	}
	if (ch->IsNpc()) {
		switch (GET_RACE(ch)) {
			case ENpcRace::kBoggart:
			case ENpcRace::kGhost:
			case ENpcRace::kHuman:
			case ENpcRace::kZombie:
			case ENpcRace::kSpirit:
			{
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
		//если включен режим без повторов (подавление ехо) не показываем
		if (PRF_FLAGGED(ch, EPrf::kNoRepeat)) {
			if (!ch->GetEnemy()) //если персонаж не в бою, шлем строчку, если в бою ничего не шлем
				SendMsgToChar(OK, ch);
		} else {
			if (IS_SET(spell_info[spell_id].routines, kMagWarcry))
				sprintf(buf, "Вы выкрикнули \"%s%s%s\".\r\n",
						spell_info[spell_id].violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), spell_info[spell_id].name, CCNRM(ch, C_NRM));
			else
				sprintf(buf, "Вы произнесли заклинание \"%s%s%s\".\r\n",
						spell_info[spell_id].violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), spell_info[spell_id].name, CCNRM(ch, C_NRM));
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

	if (tch != nullptr && IN_ROOM(tch) == ch->in_room) {
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

	sprintf(buf1, format, GetSpellName(spell_id));
	sprintf(buf2, format, buf);

	for (const auto i : world[ch->in_room]->people) {
		if (i == ch
			|| i == tch
			|| !i->desc
			|| !AWAKE(i)
			|| AFF_FLAGGED(i, EAffect::kDeafness)) {
			continue;
		}

		if (IS_SET(GET_SPELL_TYPE(i, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
			perform_act(buf1, ch, tobj, tch, i);
		} else {
			perform_act(buf2, ch, tobj, tch, i);
		}
	}

	act(buf1, 1, ch, tobj, tch, kToArenaListen);

	if (tch != nullptr
		&& tch != ch
		&& IN_ROOM(tch) == ch->in_room
		&& !AFF_FLAGGED(tch, EAffect::kDeafness)) {
		if (spell_info[spell_id].violent) {
			sprintf(buf1, damagee_vict,
					IS_SET(GET_SPELL_TYPE(tch, spell_id), ESpellType::kKnow | ESpellType::kTemp) ? GetSpellName(spell_id) : buf);
		} else {
			sprintf(buf1, helpee_vict,
					IS_SET(GET_SPELL_TYPE(tch, spell_id), ESpellType::kKnow | ESpellType::kTemp) ? GetSpellName(spell_id) : buf);
		}
		act(buf1, false, ch, nullptr, tch, kToVict);
	}
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= SPELLS_COUNT.
 */

template<typename T>
void FixName(T &name) {
	size_t pos = 0;
	while ('\0' != name[pos] && pos < kMaxStringLength) {
		if (('.' == name[pos]) || ('_' == name[pos])) {
			name[pos] = ' ';
		}
		++pos;
	}
}

ESkill FindSkillId(const char *name) {
	int ok;
	char const *temp, *temp2;
	char first[256], first2[256];

	for (const auto &skill : MUD::Skills()) {
		if (utils::IsAbbrev(name, skill.GetName())) {
			return (skill.GetId());
		}

		ok = true;
		// It won't be changed, but other uses of this function elsewhere may.
		temp = any_one_arg(skill.GetName(), first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!utils::IsAbbrev(first2, first)) {
				ok = false;
			}
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2) {
			return (skill.GetId());
		}
	}

	return ESkill::kUndefined;
}

ESpell FindSpellNum(const char *name) {
	int ok;
	char const *temp, *temp2, *realname;
	char first[256], first2[256];

	int use_syn = (((ubyte) *name <= (ubyte) 'z')
		&& ((ubyte) *name >= (ubyte) 'a'))
		|| (((ubyte) *name <= (ubyte) 'Z') && ((ubyte) *name >= (ubyte) 'A'));

	for (auto spell_id = ESpell::kSpellFirst ; spell_id <= ESpell::kSpellLast; ++spell_id) {
		realname = (use_syn) ? spell_info[spell_id].syn : spell_info[spell_id].name;

		if (!realname || !*realname) {
			continue;
		}
		if (utils::IsAbbrev(name, realname)) {
			return static_cast<ESpell>(spell_id);
		}
		ok = true;
		// It won't be changed, but other uses of this function elsewhere may.
		temp = any_one_arg(realname, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!utils::IsAbbrev(first2, first))
				ok = false;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}
		if (ok && !*first2) {
			return static_cast<ESpell>(spell_id);
		}

	}
	return ESpell::kUndefined;
}

/*void fix_name_feat(char *name) {
	fix_name(name);
}*/

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
	return FindSpellNum(name);
}

ESpell FixNameAndFindSpellId(std::string &name) {
	FixName(name);
	return FindSpellNum(name.c_str());
}

bool MayCastInNomagic(CharData *caster, ESpell spell_id) {
	if (IS_GRGOD(caster) || IS_SET(spell_info[spell_id].routines, kMagWarcry)) {
		return true;
	}

	return false;
}

bool MayCastHere(CharData *caster, CharData *victim, ESpell spell_id) {
	int ignore;

	if (IS_GRGOD(caster) || !ROOM_FLAGGED(IN_ROOM(caster), ERoomFlag::kPeaceful)) {
		return true;
	}

	if (ROOM_FLAGGED(IN_ROOM(caster), ERoomFlag::kNoBattle) && spell_info[spell_id].violent) {
		return false;
	}

	ignore = IS_SET(spell_info[spell_id].targets, kTarIgnore)
		|| IS_SET(spell_info[spell_id].routines, kMagMasses)
		|| IS_SET(spell_info[spell_id].routines, kMagGroups);

	if (!ignore && !victim) {
		return true;
	}

	if (ignore && !IS_SET(spell_info[spell_id].routines, kMagMasses) && !IS_SET(spell_info[spell_id].routines, kMagGroups)) {
		if (spell_info[spell_id].violent) {
			return false;
		}
		return victim == nullptr ? true : false;
	}

	ignore = victim
		&& (IS_SET(spell_info[spell_id].targets, kTarCharRoom)
			|| IS_SET(spell_info[spell_id].targets, kTarCharWorld))
		&& !IS_SET(spell_info[spell_id].routines, kMagAreas);

	for (const auto ch_vict : world[caster->in_room]->people) {
		if (IS_IMMORTAL(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (spell_info[spell_id].violent && same_group(caster, ch_vict))
			continue;
		if (ignore && ch_vict != victim)
			continue;
		if (spell_info[spell_id].violent) {
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

	if (spell_id < 1 || spell_id > kSpellLast)
		return 0;

	if (ROOM_FLAGGED(IN_ROOM(caster), ERoomFlag::kNoMagic) && !MayCastInNomagic(caster, spell_id)) {
		SendMsgToChar("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", caster);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.",
			false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		return 0;
	}

	if (!MayCastHere(caster, cvict, spell_id)) {
		if (IS_SET(spell_info[spell_id].routines, kMagWarcry)) {
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
		SpellUsage::AddSpellStat(caster->get_class(), spell_id);
	}

	if (IS_SET(spell_info[spell_id].routines, kMagAreas) || IS_SET(spell_info[spell_id].routines, kMagMasses)) {
		return CallMagicToArea(caster, cvict, rvict, spell_id, level);
	}

	if (IS_SET(spell_info[spell_id].routines, kMagGroups)) {
		return CallMagicToGroup(level, caster, spell_id);
	}

	if (IS_SET(spell_info[spell_id].routines, kMagRoom)) {
		return room_spells::CastSpellToRoom(level, caster, rvict, spell_id);
	}

	return CastToSingleTarget(level, caster, cvict, ovict, spell_id, ESaving::kStability);
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
	if (spell_id == kSpellControlWeather) {
		if ((what_sky = search_block(t, what_sky_type, false)) < 0) {
			SendMsgToChar("Не указан тип погоды.\r\n", ch);
			return false;
		} else
			what_sky >>= 1;
	}

	if (spell_id == kSpellCreateWeapon) {
		if ((what_sky = search_block(t, what_weapon, false)) < 0) {
			SendMsgToChar("Не указан тип оружия.\r\n", ch);
			return false;
		} else
			what_sky = 5 + (what_sky >> 1);
	}

	strcpy(cast_argument, t);

	if (IS_SET(spell_info[spell_id].targets, kTarRoomThis))
		return true;
	if (IS_SET(spell_info[spell_id].targets, kTarIgnore))
		return true;
	else if (*t) {
		if (IS_SET(spell_info[spell_id].targets, kTarCharRoom)) {
			if ((*tch = get_char_vis(ch, t, EFind::kCharInRoom)) != nullptr) {
				if (spell_info[spell_id].violent && !check_pkill(ch, *tch, t))
					return false;
				return true;
			}
		}

		if (IS_SET(spell_info[spell_id].targets, kTarCharWorld)) {
			if ((*tch = get_char_vis(ch, t, EFind::kCharInWorld)) != nullptr) {
				// чтобы мобов не чекали
				if (ch->IsNpc() || !(*tch)->IsNpc()) {
					if (spell_info[spell_id].violent && !check_pkill(ch, *tch, t)) {
						return false;
					}
					return true;
				}
				if (!ch->IsNpc()) {
					struct Follower *k, *k_next;
					char tmpname[kMaxInputLength];
					char *tmp = tmpname;
					strcpy(tmp, t);
					int fnum = 0; // ищем одноимённые цели
					int tnum = get_number(&tmp); // возвращает 1, если первая цель
					for (k = ch->followers; k; k = k_next) {
						k_next = k->next;
						if (isname(tmp, k->ch->get_pc_name())) {
							if (++fnum == tnum) {// нашли!!
								*tch = k->ch;
								return true;
							}
						}
					}
				}
			}
		}

		if (IS_SET(spell_info[spell_id].targets, kTarObjInv))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != nullptr)
				return true;

		if (IS_SET(spell_info[spell_id].targets, kTarObjEquip)) {
			int tmp;
			if ((*tobj = get_object_in_equip_vis(ch, t, ch->equipment, &tmp)) != nullptr)
				return true;
		}

		if (IS_SET(spell_info[spell_id].targets, kTarObjRoom))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != nullptr)
				return true;

		if (IS_SET(spell_info[spell_id].targets, kTarObjWorld)) {
//			if ((*tobj = get_obj_vis(ch, t)) != NULL)
//				return true;
			if (spell_id == kSpellLocateObject) {
				*tobj = FindObjForLocate(ch, t);
			} else {
				*tobj = get_obj_vis(ch, t);
			}
			if (*tobj) {
				return true;
			}
		}
	} else {
		if (IS_SET(spell_info[spell_id].targets, kTarFightSelf))
			if (ch->GetEnemy() != nullptr) {
				*tch = ch;
				return true;
			}
		if (IS_SET(spell_info[spell_id].targets, kTarFightVict))
			if (ch->GetEnemy() != nullptr) {
				*tch = ch->GetEnemy();
				return true;
			}
		if (IS_SET(spell_info[spell_id].targets, kTarCharRoom) && !spell_info[spell_id].violent) {
			*tch = ch;
			return true;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	if (IS_SET(spell_info[spell_id].routines, kMagWarcry))
		sprintf(buf, "И на %s же вы хотите так громко крикнуть?\r\n",
				IS_SET(spell_info[spell_id].targets, kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip)
				? "ЧТО" : "КОГО");
	else
		sprintf(buf, "На %s Вы хотите ЭТО колдовать?\r\n",
				IS_SET(spell_info[spell_id].targets, kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip)
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
	int ignore;

	if (spell_id == ESpell::kUndefined) {
		log("SYSERR: CastSpell trying to call spell id %d/%d.\n", to_underlying(spell_id), kSpellLast);
		return (0);
	}

	if (tch && ch) {
		if (IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !spell_info[spell_id].violent) {
			return (0);
		}
	}

	if (!troom) {
		troom = world[ch->in_room];
	}

	if (GET_POS(ch) < spell_info[spell_id].min_position) {
		switch (GET_POS(ch)) {
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
		return (0);
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_master() == tch) {
		SendMsgToChar("Вы не посмеете поднять руку на вашего повелителя!\r\n", ch);
		return (0);
	}

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(spell_info[spell_id].targets, kTarSelfOnly)) {
		SendMsgToChar("Вы можете колдовать это только на себя!\r\n", ch);
		return (0);
	}

	if (tch == ch && IS_SET(spell_info[spell_id].targets, kTarNotSelf)) {
		SendMsgToChar("Колдовать? ЭТО? На себя?! Да вы с ума сошли!\r\n", ch);
		return (0);
	}

	if ((!tch || IN_ROOM(tch) == kNowhere) && !tobj && !troom &&
		IS_SET(spell_info[spell_id].targets,
			   kTarCharRoom | kTarCharWorld | kTarFightSelf | kTarFightVict
				   | kTarObjInv | kTarObjRoom | kTarObjWorld | kTarObjEquip | kTarRoomThis
				   | kTarRoomDir)) {
		SendMsgToChar("Цель заклинания недоступна.\r\n", ch);
		return (0);
	}

	// Идея считает, что это условие лищнее, но что-то не вижу, почему. Поэтому на всякий случай - комментариий.
	//if (tch != nullptr && IN_ROOM(tch) != ch->in_room) {
	if (tch != nullptr && IN_ROOM(tch) != ch->in_room) {
		if (!IS_SET(spell_info[spell_id].targets, kTarCharWorld)) {
			SendMsgToChar("Цель заклинания недоступна.\r\n", ch);
			return (0);
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kPeaceful)) {
		ignore = IS_SET(spell_info[spell_id].targets, kTarIgnore) ||
			IS_SET(spell_info[spell_id].routines, kMagMasses) || IS_SET(spell_info[spell_id].routines, kMagGroups);
		if (ignore) { // индивидуальная цель
			if (spell_info[spell_id].violent) {
				SendMsgToChar("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
				return false;    // нельзя злые кастовать
			}
		}
		for (const auto ch_vict : world[ch->in_room]->people) {
			if (spell_info[spell_id].violent) {
				if (ch_vict == tch) {
					SendMsgToChar("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return false;
				}
			} else {
				if (ch_vict == tch && !same_group(ch, ch_vict)) {
					SendMsgToChar("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return false;
				}
			}
		}
	}

	ESkill skillnum = GetMagicSkillId(spell_id);
	if (skillnum != ESkill::kUndefined) {
		TrainSkill(ch, skillnum, true, tch);
	}
	// Комнату тут в SaySpell не обрабатываем - будет сказал "что-то"
	SaySpell(ch, spell_id, tch, tobj);
	// Уменьшаем кол-во заклов в меме
	if (GET_SPELL_MEM(ch, spell_subst) > 0) {
		GET_SPELL_MEM(ch, spell_subst)--;
	} else {
		GET_SPELL_MEM(ch, spell_subst) = 0;
	}
	// если включен автомем
	if (!ch->IsNpc() && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, EPrf::kAutomem)) {
		MemQ_remember(ch, spell_subst);
	}
	// если НПЦ - уменьшаем его макс.количество кастуемых спеллов
	if (ch->IsNpc()) {
		ch->caster_level -= (IS_SET(spell_info[spell_id].routines, NPC_CALCULATE) ? 1 : 0);
	}
	if (!ch->IsNpc()) {
		affect_total(ch);
	}

	return (CallMagic(ch, tch, tobj, troom, spell_id, GetRealLevel(ch)));
}

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id) {
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		return true;
	}

	int prob;
	// Svent: Это очевидно какой-то тупой костыль, но пока не буду исправлять.
	switch (saving) {
		case ESaving::kStability:
			prob = wis_bonus(GET_REAL_WIS(ch), WIS_FAILS) + GET_CAST_SUCCESS(ch);
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

	if (IsEquipInMetall(ch) && !IsAbleToUseFeat(ch, EFeat::kCombatCasting)) {
		prob -= 50;
	}

	prob = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_SUCCESS, prob);
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) ||
		(spell_info[spell_id].violent && victim && GET_GOD_FLAG(victim, EGf::kGodsLike)) ||
		(!spell_info[spell_id].violent && victim && GET_GOD_FLAG(victim, EGf::kGodscurse))) {
		prob -= 50;
	}

	if ((spell_info[spell_id].violent && victim && GET_GOD_FLAG(victim, EGf::kGodscurse)) ||
		(!spell_info[spell_id].violent && victim && GET_GOD_FLAG(victim, EGf::kGodsLike))) {
		prob += 50;
	}

	if (ch->IsNpc() && (GetRealLevel(ch) >= kStrongMobLevel)) {
		prob += GetRealLevel(ch) - 20;
	}

	const ESkill skill_number = GetMagicSkillId(spell_id);
	if (skill_number != ESkill::kUndefined) {
		prob += ch->get_skill(skill_number) / 20;
	}

	return (prob > number(0, 100));
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
