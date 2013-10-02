/* ************************************************************************
*   File: stuff.cpp                                     Part of Bylins    *
*  Usage: stuff load table handling                                       *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include <boost/array.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "utils.h"
#include "dg_scripts.h"
#include "stuff.hpp"
#include "char.hpp"
#include "room.hpp"
#include "corpse.hpp"
#include "screen.h"
#include "skills.h"
#include "sets_drop.hpp"

extern const char *skill_name(int num);
extern void set_obj_eff(struct obj_data *itemobj, int type, int mod);

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
	obj_rnum i;
	obj_rnum resutl_obj = number(1, MAX_LOAD_PROB) <= it.second.load_prob ?
						  (it.first >= 0 && (i = real_object(it.first)) >= 0 ?
						   (obj_index[i].stored + obj_index[i].number < it.second.obj_qty ?
							i :
							NOTHING) :
								   NOTHING) :
								  NOTHING;
	if (resutl_obj != NOTHING)
		log("Current load_prob: %d/%d, obj #%d (setload)", it.second.load_prob, MAX_LOAD_PROB, it.first);
	return resutl_obj;
}

void generate_book_upgrd(OBJ_DATA *obj)
{
	const int skills_count = 11;
	boost::array<int, skills_count> skill_list = { {
			SKILL_BACKSTAB, SKILL_PUNCTUAL, SKILL_BASH, SKILL_MIGHTHIT,
			SKILL_STUPOR, SKILL_ADDSHOT, SKILL_AWAKE, SKILL_NOPARRYHIT,
			SKILL_WARCRY, SKILL_IRON_WIND, SKILL_STRANGLE} };

	GET_OBJ_VAL(obj, 1) = skill_list[number(0, skills_count - 1)];
	std::string book_name = skill_name(GET_OBJ_VAL(obj, 1));

	if (obj->aliases
		&& (GET_OBJ_RNUM(obj) < 0 || obj->aliases != obj_proto[GET_OBJ_RNUM(obj)]->aliases))
	{
		free(obj->aliases);
	}
	obj->aliases = str_dup(("книга секретов умения: " + book_name).c_str());

	if (obj->short_description
		&& (GET_OBJ_RNUM(obj) < 0
			|| obj->short_description != obj_proto[GET_OBJ_RNUM(obj)]->short_description))
	{
		free(obj->short_description);
	}
	obj->short_description = str_dup(("книга секретов умения: " + book_name).c_str());

	if (obj->description
		&& (GET_OBJ_RNUM(obj) < 0
			|| obj->description != obj_proto[GET_OBJ_RNUM(obj)]->description))
	{
		free(obj->description);
	}
	obj->description = str_dup(("Книга секретов умения: " + book_name + " лежит здесь.").c_str());

	for (int i = 0; i < NUM_PADS; ++i)
	{
		if (GET_OBJ_PNAME(obj, i)
			&& (GET_OBJ_RNUM(obj) < 0
				|| GET_OBJ_PNAME(obj, i) != GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i)))
		{
			free(GET_OBJ_PNAME(obj, i));
		}
	}
	GET_OBJ_PNAME(obj, 0) = str_dup(("книга секретов умения: " + book_name).c_str());
	GET_OBJ_PNAME(obj, 1) = str_dup(("книги секретов умения: " + book_name).c_str());
	GET_OBJ_PNAME(obj, 2) = str_dup(("книге секретов умения: " + book_name).c_str());
	GET_OBJ_PNAME(obj, 3) = str_dup(("книгу секретов умения: " + book_name).c_str());
	GET_OBJ_PNAME(obj, 4) = str_dup(("книгой секретов умения: " + book_name).c_str());
	GET_OBJ_PNAME(obj, 5) = str_dup(("книге секретов умения: " + book_name).c_str());
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

void generate_warrior_enchant(OBJ_DATA *obj)
{
	const int main_count = 5;
	boost::array<int, main_count> main_list = { {
			APPLY_STR, APPLY_DEX, APPLY_CON, APPLY_AC, APPLY_DAMROLL} };
	const int other_count = 11;
	boost::array<int, other_count> other_list = { {
			APPLY_HITROLL, APPLY_SAVING_WILL, APPLY_SAVING_CRITICAL,
			APPLY_SAVING_STABILITY, APPLY_HITREG, APPLY_SAVING_REFLEX,
			APPLY_MORALE, APPLY_INITIATIVE, APPLY_ABSORBE, APPLY_AR, APPLY_MR} };

	if (GET_OBJ_VNUM(obj) == GlobalDrop::WARR1_ENCHANT_VNUM)
	{
		int stat = main_list[number(0, main_count - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
	}
	else if (GET_OBJ_VNUM(obj) == GlobalDrop::WARR2_ENCHANT_VNUM)
	{
		int stat = main_list[number(0, main_count - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
		stat = other_list[number(0, other_count - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
	}
	else if (GET_OBJ_VNUM(obj) == GlobalDrop::WARR3_ENCHANT_VNUM)
	{
		int stat = main_list[number(0, main_count - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat) * 2);
		stat = other_list[number(0, other_count - 1)];
		set_obj_eff(obj, stat, get_stat_mod(stat));
	}
}

/**
 * \param setload = true - лоад через систему дропа сетов
 *        setload = false - лоад через глобал дроп
 */
void obj_to_corpse(OBJ_DATA *corpse, CHAR_DATA *ch, int rnum, bool setload)
{
	OBJ_DATA *o = read_object(rnum, REAL);
	if (!o)
	{
		log("SYSERROR: null from read_object rnum=%d (%s:%d)",
				rnum, __FILE__, __LINE__);
		return;
	}

	log("Load obj #%d by %s in room #%d (%s)",
			GET_OBJ_VNUM(o), GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
			setload ? "setload" : "globaldrop");

	if (!setload)
	{
		switch (GET_OBJ_VNUM(o))
		{
		case GlobalDrop::BOOK_UPGRD_VNUM:
			generate_book_upgrd(o);
			break;
		case GlobalDrop::WARR1_ENCHANT_VNUM:
		case GlobalDrop::WARR2_ENCHANT_VNUM:
		case GlobalDrop::WARR3_ENCHANT_VNUM:
			generate_warrior_enchant(o);
			break;
		}
	}
	else
	{
		for (CHAR_DATA *tch = world[IN_ROOM(ch)]->people; tch; tch = tch->next_in_room)
		{
			send_to_char(tch, "%sДиво дивное, чудо чудное!%s\r\n",
				CCGRN(tch, C_NRM), CCNRM(tch, C_NRM));
		}
	}

	if (MOB_FLAGGED(ch, MOB_CORPSE))
	{
		obj_to_room(o, IN_ROOM(ch));
	}
	else
	{
		obj_to_obj(o, corpse);
	}
	if (!obj_decay(o))
	{
		if (o->in_room != NOWHERE)
		{
			act("На земле остал$U лежать $o.", FALSE, ch, o, 0, TO_ROOM);
		}
		load_otrigger(o);
	}
}

void obj_load_on_death(OBJ_DATA *corpse, CHAR_DATA *ch)
{
	/*
		for (oload_class::iterator iter = oload_table.begin(); iter != oload_table.end(); iter++)
			for (std::map<obj_vnum, obj_load_info>::iterator iter1 = iter->second.begin(); iter1 != iter->second.end(); iter1++)
				log("%d %d %d %d", iter->first, iter1->first, iter1->second.obj_qty, iter1->second.load_prob);
	*/

	if (ch == NULL || !IS_NPC(ch) || (!MOB_FLAGGED(ch, MOB_CORPSE) && corpse == NULL))
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
