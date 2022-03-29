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

#include "structs/global_objects.h"
#include "handler.h"
#include "color.h"
#include "depot.h"
#include "communication/parcel.h"
#include "magic.h"

char cast_argument[kMaxStringLength];

#define SpINFO spell_info[spellnum]

extern int what_sky;


int CalcRequiredLevel(const CharData *ch, int spellnum) {
	int required_level = spell_create[spellnum].runes.min_caster_level;

	if (required_level >= kLvlGod)
		return required_level;
	if (can_use_feat(ch, SECRET_RUNES_FEAT)) {
		int remort = GET_REAL_REMORT(ch);
		required_level -= MIN(8, MAX(0, ((remort - 8) / 3) * 2 + (remort > 7 && remort < 11 ? 1 : 0)));
	}

	return MAX(1, required_level);
}

// SaySpell erodes buf, buf1, buf2
void SaySpell(CharData *ch, int spellnum, CharData *tch, ObjData *tobj) {
	char lbuf[256];
	const char *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
		*helpee_vict, *damagee_vict, *format;

	*buf = '\0';
	strcpy(lbuf, SpINFO.syn);
	// Say phrase ?
	const auto &cast_phrase_list = get_cast_phrase(spellnum);
	if (!cast_phrase_list) {
		sprintf(buf, "[ERROR]: SaySpell: для спелла %d не объявлена cast_phrase", spellnum);
		mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		return;
	}
	if (ch->is_npc()) {
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
			if (!ch->get_fighting()) //если персонаж не в бою, шлем строчку, если в бою ничего не шлем
				send_to_char(OK, ch);
		} else {
			if (IS_SET(SpINFO.routines, kMagWarcry))
				sprintf(buf, "Вы выкрикнули \"%s%s%s\".\r\n",
						SpINFO.violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM));
			else
				sprintf(buf, "Вы произнесли заклинание \"%s%s%s\".\r\n",
						SpINFO.violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
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

	sprintf(buf1, format, GetSpellName(spellnum));
	sprintf(buf2, format, buf);

	for (const auto i : world[ch->in_room]->people) {
		if (i == ch
			|| i == tch
			|| !i->desc
			|| !AWAKE(i)
			|| AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS)) {
			continue;
		}

		if (IS_SET(GET_SPELL_TYPE(i, spellnum), kSpellKnow | kSpellTemp)) {
			perform_act(buf1, ch, tobj, tch, i);
		} else {
			perform_act(buf2, ch, tobj, tch, i);
		}
	}

	act(buf1, 1, ch, tobj, tch, kToArenaListen);

	if (tch != nullptr
		&& tch != ch
		&& IN_ROOM(tch) == ch->in_room
		&& !AFF_FLAGGED(tch, EAffectFlag::AFF_DEAFNESS)) {
		if (SpINFO.violent) {
			sprintf(buf1, damagee_vict,
					IS_SET(GET_SPELL_TYPE(tch, spellnum), kSpellKnow | kSpellTemp) ? GetSpellName(spellnum) : buf);
		} else {
			sprintf(buf1, helpee_vict,
					IS_SET(GET_SPELL_TYPE(tch, spellnum), kSpellKnow | kSpellTemp) ? GetSpellName(spellnum) : buf);
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

	return ESkill::kIncorrect;
}

int FindSpellNum(const char *name) {
	int index, ok;
	char const *temp, *temp2, *realname;
	char first[256], first2[256];

	int use_syn = (((ubyte) *name <= (ubyte) 'z')
		&& ((ubyte) *name >= (ubyte) 'a'))
		|| (((ubyte) *name <= (ubyte) 'Z') && ((ubyte) *name >= (ubyte) 'A'));

	for (index = 1; index <= kSpellCount; index++) {
		realname = (use_syn) ? spell_info[index].syn : spell_info[index].name;

		if (!realname || !*realname)
			continue;
		if (utils::IsAbbrev(name, realname))
			return (index);
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
		if (ok && !*first2)
			return (index);

	}
	return (-1);
}

/*void fix_name_feat(char *name) {
	fix_name(name);
}*/

ESkill FixNameAndFindSkillNum(char *name) {
	FixName(name);
	return FindSkillId(name);
}

ESkill FixNameFndFindSkillNum(std::string &name) {
	FixName(name);
	return FindSkillId(name.c_str());
}

int FixNameAndFindSpellNum(char *name) {
	FixName(name);
	return FindSpellNum(name);
}

int FixNameAndFindSpellNum(std::string &name) {
	FixName(name);
	return FindSpellNum(name.c_str());
}

bool MayCastInNomagic(CharData *caster, int spellnum) {
	if (IS_GRGOD(caster) || IS_SET(SpINFO.routines, kMagWarcry)) {
		return true;
	}

	return false;
}

bool MayCastHere(CharData *caster, CharData *victim, int spellnum) {
	int ignore;

	if (IS_GRGOD(caster) || !ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL)) {
		return true;
	}

	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOBATTLE) && SpINFO.violent) {
		return false;
	}

	ignore = IS_SET(SpINFO.targets, kTarIgnore)
		|| IS_SET(SpINFO.routines, kMagMasses)
		|| IS_SET(SpINFO.routines, kMagGroups);

	if (!ignore && !victim) {
		return true;
	}

	if (ignore && !IS_SET(SpINFO.routines, kMagMasses) && !IS_SET(SpINFO.routines, kMagGroups)) {
		if (SpINFO.violent) {
			return false;
		}
		return victim == nullptr ? true : false;
	}

	ignore = victim
		&& (IS_SET(SpINFO.targets, kTarCharRoom)
			|| IS_SET(SpINFO.targets, kTarCharWorld))
		&& !IS_SET(SpINFO.routines, kMagAreas);

	for (const auto ch_vict : world[caster->in_room]->people) {
		if (IS_IMMORTAL(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (SpINFO.violent && same_group(caster, ch_vict))
			continue;
		if (ignore && ch_vict != victim)
			continue;
		if (SpINFO.violent) {
			if (!may_kill_here(caster, ch_vict, NoArgument)) {
				return false;
			}
		} else {
			if (!may_kill_here(caster, ch_vict->get_fighting(), NoArgument)) {
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
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, int spellnum, int level) {

	if (spellnum < 1 || spellnum > kSpellCount)
		return 0;

//	if (caster && cvict && !cast_mtrigger(cvict, caster, spellnum)) {
//		return 0;
//	}

	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) && !MayCastInNomagic(caster, spellnum)) {
		send_to_char("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", caster);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.",
			false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		return 0;
	}

	if (!MayCastHere(caster, cvict, spellnum)) {
		if (IS_SET(SpINFO.routines, kMagWarcry)) {
			send_to_char("Ваш громовой глас сотряс воздух, но ничего не произошло!\r\n", caster);
			act("Вы вздрогнули от неожиданного крика, но ничего не произошло.",
				false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		} else {
			send_to_char("Ваша магия обратилась всего лишь в яркую вспышку!\r\n", caster);
			act("Яркая вспышка на миг осветила комнату, и тут же погасла.",
				false, caster, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return 0;
	}

	if (SpellUsage::is_active) {
		SpellUsage::AddSpellStat(caster->get_class(), spellnum);
	}

	if (IS_SET(SpINFO.routines, kMagAreas) || IS_SET(SpINFO.routines, kMagMasses)) {
		return CallMagicToArea(caster, cvict, rvict, spellnum, level);
	}

	if (IS_SET(SpINFO.routines, kMagGroups)) {
		return CallMagicToGroup(level, caster, spellnum);
	}

	if (IS_SET(SpINFO.routines, kMagRoom)) {
		return room_spells::ImposeSpellToRoom(level, caster, rvict, spellnum);
	}

	return CastToSingleTarget(level, caster, cvict, ovict, spellnum, ESaving::kStability);
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

int FindCastTarget(int spellnum, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom) {
	*tch = nullptr;
	*tobj = nullptr;
	*troom = world[ch->in_room];
	if (spellnum == kSpellControlWeather) {
		if ((what_sky = search_block(t, what_sky_type, false)) < 0) {
			send_to_char("Не указан тип погоды.\r\n", ch);
			return false;
		} else
			what_sky >>= 1;
	}

	if (spellnum == kSpellCreateWeapon) {
		if ((what_sky = search_block(t, what_weapon, false)) < 0) {
			send_to_char("Не указан тип оружия.\r\n", ch);
			return false;
		} else
			what_sky = 5 + (what_sky >> 1);
	}

	strcpy(cast_argument, t);

	if (IS_SET(SpINFO.targets, kTarRoomThis))
		return true;
	if (IS_SET(SpINFO.targets, kTarIgnore))
		return true;
	else if (*t) {
		if (IS_SET(SpINFO.targets, kTarCharRoom)) {
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != nullptr) {
				if (SpINFO.violent && !check_pkill(ch, *tch, t))
					return false;
				return true;
			}
		}

		if (IS_SET(SpINFO.targets, kTarCharWorld)) {
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != nullptr) {
				// чтобы мобов не чекали
				if (ch->is_npc() || !(*tch)->is_npc()) {
					if (SpINFO.violent && !check_pkill(ch, *tch, t)) {
						return false;
					}
					return true;
				}
				if (!ch->is_npc()) {
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

		if (IS_SET(SpINFO.targets, kTarObjInv))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != nullptr)
				return true;

		if (IS_SET(SpINFO.targets, kTarObjEquip)) {
			int tmp;
			if ((*tobj = get_object_in_equip_vis(ch, t, ch->equipment, &tmp)) != nullptr)
				return true;
		}

		if (IS_SET(SpINFO.targets, kTarObjRoom))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != nullptr)
				return true;

		if (IS_SET(SpINFO.targets, kTarObjWorld)) {
//			if ((*tobj = get_obj_vis(ch, t)) != NULL)
//				return true;
			if (spellnum == kSpellLocateObject) {
				*tobj = FindObjForLocate(ch, t);
			} else {
				*tobj = get_obj_vis(ch, t);
			}
			if (*tobj) {
				return true;
			}
		}
	} else {
		if (IS_SET(SpINFO.targets, kTarFightSelf))
			if (ch->get_fighting() != nullptr) {
				*tch = ch;
				return true;
			}
		if (IS_SET(SpINFO.targets, kTarFightVict))
			if (ch->get_fighting() != nullptr) {
				*tch = ch->get_fighting();
				return true;
			}
		if (IS_SET(SpINFO.targets, kTarCharRoom) && !SpINFO.violent) {
			*tch = ch;
			return true;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	if (IS_SET(SpINFO.routines, kMagWarcry))
		sprintf(buf, "И на %s же вы хотите так громко крикнуть?\r\n",
				IS_SET(SpINFO.targets, kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip)
				? "ЧТО" : "КОГО");
	else
		sprintf(buf, "На %s Вы хотите ЭТО колдовать?\r\n",
				IS_SET(SpINFO.targets, kTarObjRoom | kTarObjInv | kTarObjWorld | kTarObjEquip)
				? "ЧТО" : "КОГО");
	send_to_char(buf, ch);
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
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, int spellnum, int spell_subst) {
	int ignore;

	if (spellnum < 0 || spellnum > kSpellCount) {
		log("SYSERR: CastSpell trying to call spellnum %d/%d.\n", spellnum, kSpellCount);
		return (0);
	}

	if (tch && ch) {
		if (IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !SpINFO.violent) {
			return (0);
		}
	}

	if (!troom) {
		troom = world[ch->in_room];
	}

	if (GET_POS(ch) < SpINFO.min_position) {
		switch (GET_POS(ch)) {
			case EPosition::kSleep: send_to_char("Вы спите и не могете думать больше ни о чем.\r\n", ch);
				break;
			case EPosition::kRest: send_to_char("Вы расслаблены и отдыхаете. И далась вам эта магия?\r\n", ch);
				break;
			case EPosition::kSit: send_to_char("Похоже, в этой позе Вы много не наколдуете.\r\n", ch);
				break;
			case EPosition::kFight: send_to_char("Невозможно! Вы сражаетесь! Это вам не шухры-мухры.\r\n", ch);
				break;
			default: send_to_char("Вам вряд ли это удастся.\r\n", ch);
				break;
		}
		return (0);
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->get_master() == tch) {
		send_to_char("Вы не посмеете поднять руку на вашего повелителя!\r\n", ch);
		return (0);
	}

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(SpINFO.targets, kTarSelfOnly)) {
		send_to_char("Вы можете колдовать это только на себя!\r\n", ch);
		return (0);
	}

	if (tch == ch && IS_SET(SpINFO.targets, kTarNotSelf)) {
		send_to_char("Колдовать? ЭТО? На себя?! Да вы с ума сошли!\r\n", ch);
		return (0);
	}

	if ((!tch || IN_ROOM(tch) == kNowhere) && !tobj && !troom &&
		IS_SET(SpINFO.targets,
			   kTarCharRoom | kTarCharWorld | kTarFightSelf | kTarFightVict
				   | kTarObjInv | kTarObjRoom | kTarObjWorld | kTarObjEquip | kTarRoomThis
				   | kTarRoomDir)) {
		send_to_char("Цель заклинания недоступна.\r\n", ch);
		return (0);
	}

	// Идея считает, что это условие лищнее, но что-то не вижу, почему. Поэтому на всякий случай - комментариий.
	//if (tch != nullptr && IN_ROOM(tch) != ch->in_room) {
	if (tch != nullptr && IN_ROOM(tch) != ch->in_room) {
		if (!IS_SET(SpINFO.targets, kTarCharWorld)) {
			send_to_char("Цель заклинания недоступна.\r\n", ch);
			return (0);
		}
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_PEACEFUL)) {
		ignore = IS_SET(SpINFO.targets, kTarIgnore) ||
			IS_SET(SpINFO.routines, kMagMasses) || IS_SET(SpINFO.routines, kMagGroups);
		if (ignore) { // индивидуальная цель
			if (SpINFO.violent) {
				send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
				return false;    // нельзя злые кастовать
			}
		}
		for (const auto ch_vict : world[ch->in_room]->people) {
			if (SpINFO.violent) {
				if (ch_vict == tch) {
					send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return false;
				}
			} else {
				if (ch_vict == tch && !same_group(ch, ch_vict)) {
					send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return false;
				}
			}
		}
	}

	ESkill skillnum = GetMagicSkillId(spellnum);
	if (skillnum != ESkill::kIncorrect && skillnum != ESkill::kUndefined) {
		TrainSkill(ch, skillnum, true, tch);
	}
	// Комнату тут в SaySpell не обрабатываем - будет сказал "что-то"
	SaySpell(ch, spellnum, tch, tobj);
	// Уменьшаем кол-во заклов в меме
	if (GET_SPELL_MEM(ch, spell_subst) > 0) {
		GET_SPELL_MEM(ch, spell_subst)--;
	} else {
		GET_SPELL_MEM(ch, spell_subst) = 0;
	}
	// если включен автомем
	if (!ch->is_npc() && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, EPrf::kAutomem)) {
		MemQ_remember(ch, spell_subst);
	}
	// если НПЦ - уменьшаем его макс.количество кастуемых спеллов
	if (ch->is_npc()) {
		GET_CASTER(ch) -= (IS_SET(spell_info[spellnum].routines, NPC_CALCULATE) ? 1 : 0);
	}
	if (!ch->is_npc()) {
		affect_total(ch);
	}

	return (CallMagic(ch, tch, tobj, troom, spellnum, GetRealLevel(ch)));
}

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, int spellnum) {
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		return true;
	}

	int prob;
	// Svent: Это очевидно какой-то тупой костыль, но пока не буду исправлять.
	switch (saving) {
		case ESaving::kStability:
			prob = wis_bonus(GET_REAL_WIS(ch), WIS_FAILS) + GET_CAST_SUCCESS(ch);
			if ((IS_MAGE(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ROOM_MAGE))
				|| (IS_SORCERER(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ROOM_CLERIC))
				|| (IS_PALADINE(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ROOM_PALADINE))
				|| (IS_MERCHANT(ch) && ch->in_room != kNowhere && ROOM_FLAGGED(ch->in_room, ROOM_MERCHANT))) {
				prob += 10;
			}
			break;
		default:
			prob = 100;
			break;
	}

	if (equip_in_metall(ch) && !can_use_feat(ch, COMBAT_CASTING_FEAT)) {
		prob -= 50;
	}

	prob = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_SUCCESS, prob);
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) ||
		(SpINFO.violent && victim && GET_GOD_FLAG(victim, EGf::kGodsLike)) ||
		(!SpINFO.violent && victim && GET_GOD_FLAG(victim, EGf::kGodscurse))) {
		prob -= 50;
	}

	if ((SpINFO.violent && victim && GET_GOD_FLAG(victim, EGf::kGodscurse)) ||
		(!SpINFO.violent && victim && GET_GOD_FLAG(victim, EGf::kGodsLike))) {
		prob += 50;
	}

	if (ch->is_npc() && (GetRealLevel(ch) >= kStrongMobLevel)) {
		prob += GetRealLevel(ch) - 20;
	}

	const ESkill skill_number = GetMagicSkillId(spellnum);
	if (skill_number != ESkill::kIncorrect) {
		prob += ch->get_skill(skill_number) / 20;
	}

	return (prob > number(0, 100));
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
