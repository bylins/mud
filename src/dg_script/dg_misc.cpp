/**************************************************************************
*  File: dg_misc.cpp                                      Part of Bylins  *
*  Usage: contains general functions for script usage.                    *
*                                                                         *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                   *
************************************************************************ */

#include "dg_scripts.h"
#include "handler.h"
#include "dg_event.h"
#include "game_magic/magic.h"

extern const char *what_sky_type[];
extern int what_sky;
extern const char *what_weapon[];

extern int CalcDuration(CharData *ch, int cnst, int level, int level_divisor, int min, int max);

/*
 * Функция осуществляет поиск цели для DG_CAST
 * Облегченная версия FindCastTarget
 */
int find_dg_cast_target(int spellnum, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom) {
	*tch = nullptr;
	*tobj = nullptr;
	//если чар есть но он по каким-то причинам в kNowhere крешает как минимум в mag_masses так как указатель на комнату nullptr
	if (ch->in_room == kNowhere) {
		return false;
	}
	*troom = world[ch->in_room];

	if (spellnum == kSpellControlWeather) {
		if ((what_sky = search_block(t, what_sky_type, false)) < 0) {
			sprintf(buf2, "dg_cast (Не указан тип погоды)");
			script_log(buf2);
			return false;
		} else
			what_sky >>= 1;
	}
	if (spellnum == kSpellCreateWeapon) {
		if ((what_sky = search_block(t, what_weapon, false)) < 0) {
			sprintf(buf2, "dg_cast (Не указан тип оружия)");
			script_log(buf2);
			return false;
		} else
			what_sky = 5 + (what_sky >> 1);
	}

	if (IS_SET(spell_info[spellnum].targets, kTarIgnore))
		return true;

	if (IS_SET(spell_info[spellnum].targets, kTarRoomThis))
		return true;

	if (*t) {
		if (IS_SET(spell_info[spellnum].targets, kTarCharRoom)) {
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != nullptr) {
//            if (spell_info[spellnum].violent && !check_pkill(ch,*tch,t))
//                 return false;
				return true;
			}
		}
		if (IS_SET(spell_info[spellnum].targets, kTarCharWorld)) {
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != nullptr) {
//            if (spell_info[spellnum].violent && !check_pkill(ch,*tch,t))
//                 return false;
				return true;
			}
		}

		if (IS_SET(spell_info[spellnum].targets, kTarObjInv)) {
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != nullptr) {
				return true;
			}
		}

		if (IS_SET(spell_info[spellnum].targets, kTarObjEquip)) {
			int i;
			for (i = 0; i < NUM_WEARS; i++) {
				if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->get_aliases())) {
					*tobj = GET_EQ(ch, i);
					return true;
				}
			}
		}

		if (IS_SET(spell_info[spellnum].targets, kTarObjRoom))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != nullptr)
				return true;

		if (IS_SET(spell_info[spellnum].targets, kTarObjWorld))
			if ((*tobj = get_obj_vis(ch, t)) != nullptr)
				return true;
	} else {
		if (IS_SET(spell_info[spellnum].targets, kTarFightSelf))
			if (ch->get_fighting() != nullptr) {
				*tch = ch;
				return true;
			}
		if (IS_SET(spell_info[spellnum].targets, kTarFightVict))
			if (ch->get_fighting() != nullptr) {
				*tch = ch->get_fighting();
				return true;
			}
		if (IS_SET(spell_info[spellnum].targets, kTarCharRoom) && !spell_info[spellnum].violent) {
			*tch = ch;
			return true;
		}
	}

	return false;
}

// cast a spell; can be called by mobiles, objects and rooms, and no
// level check is required. Note that mobs should generally use the
// normal 'cast' command (which must be patched to allow mobs to cast
// spells) as the spell system is designed to have a character caster,
// and this cast routine may overlook certain issues.
// LIMITATION: a target MUST exist for the spell unless the spell is
// set to TAR_IGNORE. Also, group spells are not permitted
// code borrowed from do_cast()
void do_dg_cast(void *go, Script * /*sc*/, Trigger *trig, int type, char *cmd) {
	CharData *caster = nullptr;
	RoomData *caster_room = nullptr;
	char *s, *t;
	int spellnum, target = 0;
	bool dummy_mob = false;


	// need to get the caster or the room of the temporary caster
	switch (type) {
		case MOB_TRIGGER: caster = (CharData *) go;
			break;
		case WLD_TRIGGER: caster_room = (RoomData *) go;
			break;
		case OBJ_TRIGGER:
			caster_room = dg_room_of_obj((ObjData *) go);
			caster = dg_caster_owner_obj((ObjData *) go);
			if (!caster_room) {
				trig_log(trig, "dg_do_cast: unknown room for object-caster!");
				return;
			}
			break;
		default: trig_log(trig, "dg_do_cast: unknown trigger type!");
			return;
	}

	// get: blank, spell name, target name
	s = strtok(cmd, "'");
	if (s == nullptr) {
		sprintf(buf2, "dg_cast: needs spell name.");
		trig_log(trig, buf2);
		return;
	}
	s = strtok(nullptr, "'");
	if (s == nullptr) {
		sprintf(buf2, "dg_cast: needs spell name in `'s.");
		trig_log(trig, buf2);
		return;
	}
	t = strtok(nullptr, "\0");

	// spellnum = search_block(s, spells, 0);
	spellnum = FixNameAndFindSpellNum(s);
	if ((spellnum < 1) || (spellnum > kSpellCount)) {
		sprintf(buf2, "dg_cast: invalid spell name (%s)", cmd);
		trig_log(trig, buf2);
		return;
	}

	if (!caster) {
		caster = read_mobile(DG_CASTER_PROXY, VIRTUAL);
		if (!caster) {
			trig_log(trig, "dg_cast: Cannot load the caster mob!");
			return;
		}
		// set the caster's name to that of the object, or the gods....
		// take select pieces from char_to_room();
		dummy_mob = true;
		if (type == OBJ_TRIGGER) {
			sprintf(buf, "дух %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->set_npc_name(buf);
			sprintf(buf, "дух %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->player_data.PNames[0] = std::string(buf);
			sprintf(buf, "духа %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->player_data.PNames[1] = std::string(buf);
			sprintf(buf, "духу %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->player_data.PNames[2] = std::string(buf);
			sprintf(buf, "духа %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->player_data.PNames[3] = std::string(buf);
			sprintf(buf, "духом %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->player_data.PNames[4] = std::string(buf);
			sprintf(buf, "духе %s", ((ObjData *) go)->get_PName(1).c_str());
			caster->player_data.PNames[5] = std::string(buf);
		} else if (type == WLD_TRIGGER) {
			caster->set_npc_name("Боги");
			caster->player_data.PNames[0] = "Боги";
			caster->player_data.PNames[1] = "Богов";
			caster->player_data.PNames[2] = "Богам";
			caster->player_data.PNames[3] = "Богов";
			caster->player_data.PNames[4] = "Богами";
			caster->player_data.PNames[5] = "Богах";
		}

		caster_room->people.push_front(caster);

		IN_ROOM(caster) = real_room(caster_room->room_vn);
	}
/*	if (type == OBJ_TRIGGER){
	sprintf(buf2, "dg_cast OBJ_TRIGGER: имя кастера: %s, его уровень: %d, его морты: %d, закл: %s.", GET_NAME(caster),
		GetRealLevel(caster), GET_REAL_REMORT(caster), spell_info[spellnum].name);
	trig_log(trig, buf2);
	}
*/
	// Find the target
	if (t != nullptr)
		one_argument(t, arg);
	else
		*arg = '\0';

	// в find_dg_cast_target можем и не попасть для инита нулями и в CallMagic пойдет мусор
	CharData *tch = nullptr;
	ObjData *tobj = nullptr;
	RoomData *troom = nullptr;

	if (*arg == UID_CHAR) {
		tch = get_char(arg);
		if (tch == nullptr) {
			sprintf(buf2, "dg_cast: victim (%s) not found", arg + 1);
			trig_log(trig, buf2);
		} else if (kNowhere == caster->in_room) {
			sprintf(buf2, "dg_cast: caster (%s) in kNowhere", GET_NAME(caster));
			trig_log(trig, buf2);
		} else if (tch->in_room != caster->in_room) {
			sprintf(buf2,
					"dg_cast: caster (%s) and victim (%s) в разных клетках комнат",
					GET_NAME(caster),
					GET_NAME(tch));
			trig_log(trig, buf2);
		} else {
			target = 1;
			troom = world[caster->in_room];
		}
	} else {
		target = find_dg_cast_target(spellnum, arg, caster, &tch, &tobj, &troom);
	}
	if (target) {
		CallMagic(caster, tch, tobj, troom, spellnum, GetRealLevel(caster));
	} else if (spellnum != kSpellResurrection && spellnum != kSpellAnimateDead) {
		sprintf(buf2, "dg_cast: target not found (%s)", cmd);
		trig_log(trig, buf2);
	}
	if (dummy_mob)
		extract_char(caster, false);
}

/* modify an affection on the target. affections can be of the AFF_x
   variety or APPLY_x type. APPLY_x's have an integer value for them
   while AFF_x's have boolean values. In any case, the duration MUST
   be non-zero.
   usage:  apply <target> <property> <spell> <value> <duration>
   if duration < 1 - function removes affect */
#define APPLY_TYPE    1
#define AFFECT_TYPE    2
void do_dg_affect(void * /*go*/, Script * /*sc*/, Trigger *trig, int/* script_type*/, char *cmd) {
	CharData *ch = nullptr;
	int value = 0, duration = 0, battle = 0;
	char junk[kMaxInputLength];    // will be set to "dg_affect"
	char charname[kMaxInputLength], property[kMaxInputLength];
	char value_p[kMaxInputLength], duration_p[kMaxInputLength];
	char battle_p[kMaxInputLength];
	char spell[kMaxInputLength];
	int index = 0, type = 0, index_s = 0, i;

	half_chop(cmd, junk, cmd);
	half_chop(cmd, charname, cmd);
	half_chop(cmd, property, cmd);
	half_chop(cmd, spell, cmd);
	half_chop(cmd, value_p, cmd);
	half_chop(cmd, duration_p, battle_p);

	// make sure all parameters are present
	if (!*charname || !*property || !*spell || !*value_p || !*duration_p) {
		sprintf(buf2, "dg_affect usage: <target> <property> <spell> <value> <duration> *<battleposition>");
		trig_log(trig, buf2);
		return;
	}

	// заменяем '_' на ' ' в названии заклинания и аффекта
	for (i = 0; spell[i]; i++)
		if (spell[i] == '_')
			spell[i] = ' ';
	for (i = 0; property[i]; i++)
		if (property[i] == '_')
			property[i] = ' ';

	value = atoi(value_p);
	duration = atoi(duration_p);
	battle = atoi(battle_p);
// Если длительность 0 снимаем аффект ниже
	if (duration < 0) {
		sprintf(buf2, "dg_affect: need positive duration!");
		trig_log(trig, buf2);
		return;
	}
	// find the property -- first search apply_types
	if ((index = search_block(property, apply_types, false)) != -1) {
		type = APPLY_TYPE;
	} else {
		//search affect_types now
		if ((index = ext_search_block(property, affected_bits, false)) != 0)
			type = AFFECT_TYPE;
	}

	if (!type)        // property not found
	{
		sprintf(buf2, "dg_affect: unknown property '%s'!", property);
		trig_log(trig, buf2);
		return;
	}

	// locate spell
	index_s = FixNameAndFindSpellNum(spell);

	// spell not found
	if (index_s <= 0) {
		index_s = kSpellQUest;
		sprintf(buf2, "dg_affect: unknown spell '%s' ставим 'чары'!", spell);
		trig_log(trig, buf2);
		return;
	}


	// locate the target
	ch = get_char(charname);
	if (!ch) {
		sprintf(buf2, "dg_affect: cannot locate target!");
		trig_log(trig, buf2);
		return;
	}

	if (duration > 0) {
		// add the affect
		Affect<EApplyLocation> af;
		af.type = index_s;

		af.battleflag = battle;
		if (battle == kAfPulsedec) {
			af.duration = duration;
		} else {
			af.duration = CalcDuration(ch, duration * 2, 0, 0, 0, 0);
		}
		if (type == AFFECT_TYPE) {
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.bitvector = index;
		} else {
			af.location = static_cast<EApplyLocation>(index);
			af.modifier = value;
			af.bitvector = 0;
		}
		affect_join_fspell(ch, af); // перекастим аффект
	} else {
		// remove affect
		affect_from_char(ch, index_s);
		// trig_log(trig, "dg_affect: affect removed from char");
		//Вроде не критично уже видеть все снятия аффектов
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
