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
#include "engine/core/handler.h"
#include "dg_event.h"
#include "gameplay/magic/magic.h"
#include "engine/db/global_objects.h"
#include "gameplay/affects/affect_data.h"

extern const char *what_sky_type[];
extern int what_sky;
extern const char *what_weapon[];

/*
 * Функция осуществляет поиск цели для DG_CAST
 * Облегченная версия FindCastTarget
 */
int find_dg_cast_target(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom) {
	*tch = nullptr;
	*tobj = nullptr;
	//если чар есть но он по каким-то причинам в kNowhere крешает как минимум в mag_masses так как указатель на комнату nullptr
	if (ch->in_room == kNowhere) {
		return false;
	}
	*troom = world[ch->in_room];

	if (spell_id == ESpell::kControlWeather) {
		if ((what_sky = search_block(t, what_sky_type, false)) < 0) {
			sprintf(buf2, "dg_cast (Не указан тип погоды)");
			script_log(buf2);
			return false;
		} else
			what_sky >>= 1;
	}
	if (spell_id == ESpell::kCreateWeapon) {
		if ((what_sky = search_block(t, what_weapon, false)) < 0) {
			sprintf(buf2, "dg_cast (Не указан тип оружия)");
			script_log(buf2);
			return false;
		} else
			what_sky = 5 + (what_sky >> 1);
	}
	if (MUD::Spell(spell_id).IsFlagged(kMagMasses))
		return true;
	if (MUD::Spell(spell_id).AllowTarget(kTarIgnore))
		return true;
	if (MUD::Spell(spell_id).AllowTarget(kTarRoomThis))
		return true;
	if (*t) {
		if (MUD::Spell(spell_id).AllowTarget(kTarCharRoom)) {
			if ((*tch = get_char_vis(ch, t, EFind::kCharInRoom)) != nullptr) {
				return true;
			}
		}
		if (MUD::Spell(spell_id).AllowTarget(kTarCharWorld)) {
			if ((*tch = get_char_vis(ch, t, EFind::kCharInWorld)) != nullptr) {
				return true;
			}
		}

		if (MUD::Spell(spell_id).AllowTarget(kTarObjInv)) {
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != nullptr) {
				return true;
			}
		}

		if (MUD::Spell(spell_id).AllowTarget(kTarObjEquip)) {
			int i;
			for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
				if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->get_aliases())) {
					*tobj = GET_EQ(ch, i);
					return true;
				}
			}
		}

		if (MUD::Spell(spell_id).AllowTarget(kTarObjRoom))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != nullptr)
				return true;

		if (MUD::Spell(spell_id).AllowTarget(kTarObjWorld))
			if ((*tobj = get_obj_vis(ch, t)) != nullptr)
				return true;
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
void do_dg_cast(void *go, Trigger *trig, int type, std::string cmd) {
	CharData *caster = nullptr;
	RoomData *caster_room = nullptr;
	int target = 0;
	bool dummy_mob = false;
	std::string argument = cmd;

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
	// parse: "DgCast 'spell name' target"
	auto quote1 = cmd.find_first_of("'!");
	if (quote1 == std::string::npos) {
		trig_log(trig, "dg_cast: needs spell name.");
		return;
	}
	auto quote2 = cmd.find_first_of("'!", quote1 + 1);
	if (quote2 == std::string::npos) {
		trig_log(trig, "dg_cast: needs spell name in `'s.");
		return;
	}
	std::string spell_name = cmd.substr(quote1 + 1, quote2 - quote1 - 1);
	std::string target_name;
	if (quote2 + 1 < cmd.size()) {
		target_name = cmd.substr(quote2 + 1);
		utils::TrimLeft(target_name);
	}

	auto spell_id = FixNameAndFindSpellId(spell_name.data());
	if (spell_id == ESpell::kUndefined) {
		sprintf(buf2, "dg_cast: invalid spell name, аргумент: (%s)", argument.c_str());
		trig_log(trig, buf2);
		return;
	}

	if (!caster) {
		caster = ReadMobile(kDgCasterProxy, kVirtual);
		if (!caster) {
			trig_log(trig, "dg_cast: Cannot load the caster mob!");
			return;
		}
		// set the caster's name to that of the object, or the gods....
		// take select pieces from char_to_room();
		dummy_mob = true;
		if (type == OBJ_TRIGGER) {
			sprintf(buf, "дух %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->set_npc_name(buf);
			sprintf(buf, "дух %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->player_data.PNames[ECase::kNom] = std::string(buf);
			sprintf(buf, "духа %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->player_data.PNames[ECase::kGen] = std::string(buf);
			sprintf(buf, "духу %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->player_data.PNames[ECase::kDat] = std::string(buf);
			sprintf(buf, "духа %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->player_data.PNames[ECase::kAcc] = std::string(buf);
			sprintf(buf, "духом %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->player_data.PNames[ECase::kIns] = std::string(buf);
			sprintf(buf, "духе %s", ((ObjData *) go)->get_PName(ECase::kGen).c_str());
			caster->player_data.PNames[ECase::kPre] = std::string(buf);
		} else if (type == WLD_TRIGGER) {
			caster->set_npc_name("Боги");
			caster->player_data.PNames[ECase::kNom] = "Боги";
			caster->player_data.PNames[ECase::kGen] = "Богов";
			caster->player_data.PNames[ECase::kDat] = "Богам";
			caster->player_data.PNames[ECase::kAcc] = "Богов";
			caster->player_data.PNames[ECase::kIns] = "Богами";
			caster->player_data.PNames[ECase::kPre] = "Богах";
		}
		caster_room->people.push_front(caster);
		caster->in_room = GetRoomRnum(caster_room->vnum);
	}

	// Find the target
	std::string remains;
	std::string target_arg = target_name.empty() ? "" : utils::ExtractFirstArgument(target_name, remains);

	// в find_dg_cast_target можем и не попасть для инита нулями и в CallMagic пойдет мусор
	CharData *tch = nullptr;
	ObjData *tobj = nullptr;
	RoomData *troom = nullptr;

	if (!target_arg.empty() && target_arg[0] == UID_CHAR) {
		tch = get_char(target_arg.c_str());
		if (tch == nullptr) {
			snprintf(buf2, kMaxStringLength, "dg_cast: victim (%s) not found, аргумент: %s", target_arg.c_str() + 1, argument.c_str());
			trig_log(trig, buf2);
		} else if (kNowhere == caster->in_room) {
			sprintf(buf2, "dg_cast: caster (%s) in kNowhere", caster->get_name().c_str());
			trig_log(trig, buf2);
		} else if (tch->in_room != caster->in_room) {
			sprintf(buf2,
					"dg_cast: caster (%s) and victim (%s) в разных клетках комнат",
					caster->get_name().c_str(),
					tch->get_name().c_str());
			trig_log(trig, buf2);
		} else {
			target = 1;
			troom = world[caster->in_room];
		}
	} else {
		target = find_dg_cast_target(spell_id, target_arg.c_str(), caster, &tch, &tobj, &troom);
	}
	if (target) {
		CallMagic(caster, tch, tobj, troom, spell_id, GetRealLevel(caster));
	} else if (spell_id != ESpell::kResurrection && spell_id != ESpell::kAnimateDead) {
		sprintf(buf2, "dg_cast: target not found, аргумент: %s", argument.c_str());
		trig_log(trig, buf2);
	}
	if (dummy_mob)
		ExtractCharFromWorld(caster, false);
}

/* modify an affection on the target. affections can be of the AFF_x
   variety or APPLY_x type. APPLY_x's have an integer value for them
   while AFF_x's have boolean values. In any case, the duration MUST
   be non-zero.
   usage:  apply <target> <property> <spell> <value> <duration>
   if duration < 1 - function removes affect */
#define APPLY_TYPE    1
#define AFFECT_TYPE    2
void do_dg_affect(void * /*go*/, Script * /*sc*/, Trigger *trig, int/* script_type*/, std::string cmd) {
	CharData *ch = nullptr;
	int value = 0, duration = 0, battle = 0;
	int index = 0, type = 0;

	// parse: "dgaffect <target> <property> <spell> <value> <duration> [battlepos]"
	std::string remains = cmd;
	std::string junk = utils::ExtractFirstArgument(remains, remains);       // "dgaffect"
	std::string charname = utils::ExtractFirstArgument(remains, remains);   // target
	std::string property = utils::ExtractFirstArgument(remains, remains);   // property
	std::string spell_name = utils::ExtractFirstArgument(remains, remains); // spell
	std::string value_p = utils::ExtractFirstArgument(remains, remains);    // value
	std::string duration_p = utils::ExtractFirstArgument(remains, remains); // duration
	std::string battle_p = remains;                                          // battlepos (optional)
	utils::Trim(battle_p);

	// make sure all parameters are present
	if (charname.empty() || property.empty() || spell_name.empty() || value_p.empty() || duration_p.empty()) {
		trig_log(trig, "dg affect usage: <target> <property> <spell> <value> <duration> *<battleposition>");
		return;
	}

	std::replace(spell_name.begin(), spell_name.end(), '_', ' ');
	std::replace(property.begin(), property.end(), '_', ' ');

	value = atoi(value_p.c_str());
	duration = atoi(duration_p.c_str());
	battle = atoi(battle_p.c_str());

	if (duration < 0) {
		trig_log(trig, "dg_affect: need positive duration!");
		return;
	}
	// find the property -- first search apply_types
	if ((index = search_block(property.c_str(), apply_types, false)) != -1) {
		type = APPLY_TYPE;
	} else {
		if ((index = ext_search_block(property.c_str(), affected_bits, false)) != 0)
			type = AFFECT_TYPE;
	}

	if (!type)        // property not found
	{
		sprintf(buf2, "dg_affect: unknown property '%s'!", property.c_str());
		trig_log(trig, buf2);
		return;
	}

	auto index_s = FixNameAndFindSpellId(spell_name.data());
	if (index_s == ESpell::kUndefined) {
		sprintf(buf2, "dg_affect: unknown spell '%s' ставим 'чары'!", spell_name.c_str());
		trig_log(trig, buf2);
		return;
	}

	// locate the target
	ch = get_char(charname.c_str());
	if (!ch) {
		sprintf(buf2, "dg_affect: cannot locate target!");
		trig_log(trig, buf2);
		return;
	}

	if (duration > 0) {
		// add the affect
		Affect<EApply> af;
		af.type = index_s;

		af.battleflag = battle;
		if (battle == kAfPulsedec) {
			af.duration = duration;
		} else {
			af.duration = CalcDuration(ch, duration * 2, 0, 0, 0, 0);
		}
		if (type == AFFECT_TYPE) {
			af.location = EApply::kNone;
			af.modifier = 0;
			af.bitvector = index;
		} else {
			af.location = static_cast<EApply>(index);
			af.modifier = value;
			af.bitvector = 0;
		}
		ImposeAffect(ch, af); // перекастим аффект
	} else {
		// remove affect
		RemoveAffectFromCharAndRecalculate(ch, index_s);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
