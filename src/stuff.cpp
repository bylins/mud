/* ************************************************************************
*   File: stuff.cpp                                     Part of Bylins    *
*  Usage: stuff load table handling                                       *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "stuff.hpp"

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "obj.hpp"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "dg_script/dg_scripts.h"
#include "chars/character.h"
#include "room.hpp"
#include "corpse.hpp"
#include "screen.h"
#include "skills.h"
#include "sets_drop.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

extern std::vector<RandomObj> random_objs;
extern const char *skill_name(int num);
extern void set_obj_eff(OBJ_DATA *itemobj, const EApplyLocation type, int mod);
extern void set_obj_aff(OBJ_DATA *itemobj, const EAffectFlag bitv);
extern int planebit(const char *str, int *plane, int *bit);

void LoadRandomObj(OBJ_DATA *obj)
{
	// костыли, привет
	int plane, bit;
	for (auto robj : random_objs)
	{
		if (robj.vnum == GET_OBJ_VNUM(obj))
		{
			obj->set_weight(number(robj.min_weight, robj.max_weight));
			obj->set_cost(number(robj.min_price, robj.max_price));
			obj->set_maximum_durability(number(robj.min_stability, robj.max_stability));
			obj->set_val(0, number(robj.value0_min, robj.value0_max));
			obj->set_val(1, number(robj.value1_min, robj.value1_max));
			obj->set_val(2, number(robj.value2_min, robj.value2_max));
			obj->set_val(3, number(robj.value3_min, robj.value3_max));
			for (auto nw : robj.not_wear)
			{
				if (nw.second < number(0, 1000))
				{
					int numb = planebit(nw.first.c_str(), &plane, &bit);
					if (numb <= 0)
						return;
					obj->toggle_no_flag(plane, 1 << bit);
				}
			}
			for (auto aff : robj.affects)
			{
				if (aff.second < number(0, 1000))
				{
					int numb = planebit(aff.first.c_str(), &plane, &bit);
					if (numb <= 0)
						return;
					obj->toggle_affect_flag(plane, 1 << bit);
				}
			}
			for (auto aff : robj.extraffect)
			{
				if (aff.chance < number(0, 1000))
				{
					obj->set_affected_location(aff.number, static_cast<EApplyLocation>(number(aff.min_val, aff.max_val)));
				}
			}
		}
	}
}

void oload_class::init()
{
	std::string cppstr;
	std::istringstream isstream;
	bool in_block = false;
	obj_vnum ovnum;
	mob_vnum mvnum;
	int oqty, lprob;

	clear();

	std::ifstream fp(LIB_MISC "stuff.lst");

	if (!fp)
	{
		cppstr = "oload_class:: Unable open input file !!!";
		mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	while (!fp.eof())
	{
		getline(fp, cppstr);

		if (cppstr.empty() || cppstr[0] == ';')
			continue;
		if (cppstr[0] == '#')
		{
			cppstr.erase(cppstr.begin());

			if (cppstr.empty())
			{
				cppstr = "oload_class:: Error in line '#' expected '#<RIGHT_obj_vnum>' !!!";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				in_block = false;
				continue;
			}

			isstream.str(cppstr);
			isstream >> std::noskipws >> ovnum;

			if (!isstream.eof() || real_object(ovnum) < 0)
			{
				isstream.clear();
				cppstr = "oload_class:: Error in line '#" + cppstr + "' expected '#<RIGHT_obj_vnum>' !!!";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				in_block = false;
				continue;
			}

			isstream.clear();

			in_block = true;
		}
		else if (in_block)
		{
			oqty = lprob = -1;

			isstream.str(cppstr);
			isstream >> std::skipws >> mvnum >> oqty >> lprob;

			if (lprob < 0 || lprob > MAX_LOAD_PROB || oqty < 0 || real_mobile(mvnum) < 0 || !isstream.eof())
			{
				isstream.clear();
				cppstr = "oload_class:: Error in line '" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				cppstr = "oload_class:: \texpected '<RIGHT_mob_vnum>\t<0 <= obj_qty>\t<0 <= load_prob <= MAX_LOAD_PROB>' !!!";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			isstream.clear();

			add_elem(mvnum, ovnum, obj_load_info(oqty, lprob));
		}
		else
		{
			cppstr = "oload_class:: Error in line '" + cppstr + "' expected '#<RIGHT_obj_vnum>' !!!";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		}
	}
}

oload_class oload_table;

obj_rnum ornum_by_info(const std::pair<obj_vnum, obj_load_info>& it)
{
	obj_rnum i = real_object(it.first);
	obj_rnum resutl_obj = number(1, MAX_LOAD_PROB) <= it.second.load_prob
		? (it.first >= 0 && i >= 0
			? (obj_proto.actual_count(i) < it.second.obj_qty
				? i
				: NOTHING)
			: NOTHING)
		: NOTHING;
	if (resutl_obj != NOTHING)
	{
		log("Current load_prob: %d/%d, obj #%d (setload)", it.second.load_prob, MAX_LOAD_PROB, it.first);
	}
	return resutl_obj;
}

int get_stat_mod(int stat)
{
	int mod = 0;
	switch (stat)
	{
	case APPLY_STR:
	case APPLY_DEX:
	case APPLY_CON:
		mod = 1;
		break;
	case APPLY_AC:
		mod = -10;
		break;
	case APPLY_HITROLL:
		mod = 2;
		break;
	case APPLY_DAMROLL:
		mod = 3;
		break;
	case APPLY_SAVING_WILL:
	case APPLY_SAVING_CRITICAL:
	case APPLY_SAVING_STABILITY:
	case APPLY_SAVING_REFLEX:
		mod = -10;
		break;
	case APPLY_HITREG:
		mod = 10;
		break;
	case APPLY_MORALE:
	case APPLY_INITIATIVE:
		mod = 3;
		break;
	case APPLY_ABSORBE:
		mod = 5;
		break;
	case APPLY_AR:
	case APPLY_MR:
		mod = 1;
		break;
	}
	return mod;
}

void generate_book_upgrd(OBJ_DATA *obj)
{
	const auto skill_list = make_array<int>(
		SKILL_BACKSTAB, SKILL_PUNCTUAL, SKILL_BASH, SKILL_MIGHTHIT,
		SKILL_STUPOR, SKILL_ADDSHOT, SKILL_AWAKE, SKILL_NOPARRYHIT,
		SKILL_WARCRY, SKILL_IRON_WIND, SKILL_STRANGLE);

	obj->set_val(1, skill_list[number(0, static_cast<int>(skill_list.size()) - 1)]);
	std::string book_name = skill_name(GET_OBJ_VAL(obj, 1));

	obj->set_aliases("книга секретов умения: " + book_name);
	obj->set_short_description("книга секретов умения: " + book_name);
	obj->set_description("Книга секретов умения: " + book_name + " лежит здесь.");

	obj->set_PName(0, "книга секретов умения: " + book_name);
	obj->set_PName(1, "книги секретов умения: " + book_name);
	obj->set_PName(2, "книге секретов умения: " + book_name);
	obj->set_PName(3, "книгу секретов умения: " + book_name);
	obj->set_PName(4, "книгой секретов умения: " + book_name);
	obj->set_PName(5, "книге секретов умения: " + book_name);
}

void generate_warrior_enchant(OBJ_DATA *obj)
{
	const auto main_list = make_array<EApplyLocation>(
		APPLY_STR, APPLY_DEX, APPLY_CON, APPLY_AC, APPLY_DAMROLL);

	const auto second_list = make_array<EApplyLocation>(
		APPLY_HITROLL, APPLY_SAVING_WILL, APPLY_SAVING_CRITICAL,
		APPLY_SAVING_STABILITY, APPLY_HITREG, APPLY_SAVING_REFLEX,
		APPLY_MORALE, APPLY_INITIATIVE, APPLY_ABSORBE, APPLY_AR, APPLY_MR);

	switch (GET_OBJ_VNUM(obj))
	{
	case GlobalDrop::WARR1_ENCHANT_VNUM:
	{
		auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
		break;
	}
	case GlobalDrop::WARR2_ENCHANT_VNUM:
	{
		auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));

		stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
		break;
	}
	case GlobalDrop::WARR3_ENCHANT_VNUM:
	{
		auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat) * 2);		//Double effect from main_stat

		stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
		break;
	}
	default:
		sprintf(buf2, "SYSERR: Unknown vnum warrior enchant object: %d", GET_OBJ_VNUM(obj));
		mudlog(buf2, BRF, LVL_IMMORT, SYSLOG, TRUE);
		break;
	}
}

void generate_magic_enchant(OBJ_DATA *obj)
{
	const auto main_list = make_array<EApplyLocation>(
		APPLY_STR, APPLY_DEX, APPLY_CON, APPLY_INT, APPLY_WIS,
		APPLY_CHA, APPLY_AC, APPLY_DAMROLL, APPLY_AR, APPLY_MR);

	const auto second_list = make_array<EApplyLocation>(
		APPLY_HITROLL, APPLY_SAVING_WILL, APPLY_SAVING_CRITICAL,
		APPLY_SAVING_STABILITY, APPLY_HITREG, APPLY_SAVING_REFLEX,
		APPLY_MORALE, APPLY_INITIATIVE, APPLY_ABSORBE, APPLY_AR, APPLY_MR,
		APPLY_MANAREG, APPLY_CAST_SUCCESS, APPLY_RESIST_MIND, APPLY_DAMROLL);

	switch (GET_OBJ_VNUM(obj))
	{
	case GlobalDrop::MAGIC1_ENCHANT_VNUM:
	{
		EApplyLocation effect = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
		set_obj_eff(obj, effect, get_stat_mod(effect));
		break;
	}
	case GlobalDrop::MAGIC2_ENCHANT_VNUM:
	{
		auto effect = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
		set_obj_eff(obj, effect, get_stat_mod(effect) * 2);

		effect = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
		set_obj_eff(obj, effect, get_stat_mod(effect));
		break;
	}
	case GlobalDrop::MAGIC3_ENCHANT_VNUM:
	{
		auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat) * 2);

		stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));

		if (number(0, 1) == 0)
		{
			stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat) * 2);
		}
		else
		{
			stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat));
		}
		break;
	}
	default:
		sprintf(buf2, "SYSERR: Unknown vnum magic enchant object: %d", GET_OBJ_VNUM(obj));
		mudlog(buf2, BRF, LVL_IMMORT, SYSLOG, TRUE);
		break;
	}
}

/**
 * \param setload = true - лоад через систему дропа сетов
 *        setload = false - лоад через глобал дроп
 */
void obj_to_corpse(OBJ_DATA *corpse, CHAR_DATA *ch, int rnum, bool setload)
{
	const auto o = world_objects.create_from_prototype_by_rnum(rnum);
	if (!o)
	{
		log("SYSERROR: null from read_object rnum=%d (%s:%d)", rnum, __FILE__, __LINE__);
		return;
	}

	log("Load obj #%d by %s in room #%d (%s)",
			o->get_vnum(), GET_NAME(ch), GET_ROOM_VNUM(ch->in_room),
			setload ? "setload" : "globaldrop");

	if (!setload)
	{
		switch (o->get_vnum())
		{
		case GlobalDrop::BOOK_UPGRD_VNUM:
			generate_book_upgrd(o.get());
			break;

		case GlobalDrop::WARR1_ENCHANT_VNUM:
		case GlobalDrop::WARR2_ENCHANT_VNUM:
		case GlobalDrop::WARR3_ENCHANT_VNUM:
			generate_warrior_enchant(o.get());
			break;

		case GlobalDrop::MAGIC1_ENCHANT_VNUM:
		case GlobalDrop::MAGIC2_ENCHANT_VNUM:
		case GlobalDrop::MAGIC3_ENCHANT_VNUM:
			generate_magic_enchant(o.get());
			break;
		}
	}
	else
	{
		for (const auto tch : world[ch->in_room]->people)
		{
			send_to_char(tch, "%sДиво дивное, чудо чудное!%s\r\n", CCGRN(tch, C_NRM), CCNRM(tch, C_NRM));
		}
	}

	if (MOB_FLAGGED(ch, MOB_CORPSE))
	{
		obj_to_room(o.get(), ch->in_room);
	}
	else
	{
		obj_to_obj(o.get(), corpse);
	}

	if (!obj_decay(o.get()))
	{
		if (o->get_in_room() != NOWHERE)
		{
			act("На земле остал$U лежать $o.", FALSE, ch, o.get(), 0, TO_ROOM);
		}
		load_otrigger(o.get());
	}
}

void obj_load_on_death(OBJ_DATA *corpse, CHAR_DATA *ch)
{
	if (ch == NULL
		|| !IS_NPC(ch)
		|| (!MOB_FLAGGED(ch, MOB_CORPSE)
			&& corpse == NULL))
	{
		return;
	}

	const int rnum = SetsDrop::check_mob(GET_MOB_RNUM(ch));
	if (rnum > 0)
	{
		obj_to_corpse(corpse, ch, rnum, true);
	}

	if (GlobalDrop::check_mob(corpse, ch))
	{
		return;
	}

	oload_class::const_iterator p = oload_table.find(GET_MOB_VNUM(ch));

	if (p == oload_table.end())
	{
		return;
	}

	std::vector<obj_rnum> v(p->second.size());
	std::transform(p->second.begin(), p->second.end(), v.begin(), ornum_by_info);

	for (size_t i = 0; i < v.size(); i++)
	{
		if (v[i] >= 0)
		{
			obj_to_corpse(corpse, ch, v[i], false);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
