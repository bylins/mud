/* ************************************************************************
*   File: handler.cpp                                   Part of Bylins    *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include <sstream>
#include <math.h>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "constants.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "glory_const.hpp"
#include "interpreter.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "dg_scripts.h"
#include "auction.h"
#include "features.hpp"
#include "house.h"
#include "exchange.h"
#include "char.hpp"
#include "char_player.hpp"
#include "liquid.hpp"
#include "magic.h"
#include "poison.hpp"
#include "name_list.hpp"
#include "room.hpp"
#include "named_stuff.hpp"
#include "glory_const.hpp"
#include "fight.h"
#include "ext_money.hpp"
#include "noob.hpp"
#include "obj_sets.hpp"

// Это ужасно, но иначе цигвин крешит. Может быть на родном юниксе все ок...

int max_stats2[][6] =
	// Str Dex Int Wis Con Cha //
{ {14, 13, 24, 25, 15, 10},	// Лекарь //
	{14, 12, 25, 23, 13, 16},	// Колдун //
	{19, 25, 12, 12, 17, 16},	// Вор //
	{25, 11, 15, 15, 25, 10},	// Богатырь //
	{22, 24, 14, 14, 17, 12},	// Наемник //
	{23, 17, 14, 14, 23, 12},	// Дружинник //
	{14, 12, 25, 23, 13, 16},	// Кудесник //
	{14, 12, 25, 23, 13, 16},	// Волшебник //
	{15, 13, 25, 23, 14, 12},	// Чернокнижник //
	{22, 13, 16, 19, 18, 17},	// Витязь //
	{25, 21, 16, 16, 18, 16},	// Охотник //
	{25, 17, 13, 15, 20, 16},	// Кузнец //
	{21, 17, 14, 13, 20, 17},	// Купец //
	{18, 12, 24, 18, 15, 12}	// Волхв //
};

int min_stats2[][6] =
	// Str Dex Int Wis Con Cha //
{ {11, 10, 19, 20, 12, 10},	// Лекарь //
	{10,  9, 20, 18, 10, 13},	// Колдун //
	{16, 22,  9,  9, 14, 13},	// Вор //
	{21,  8, 11, 11, 22, 10},	// Богатырь //
	{17, 19, 11, 11, 14, 12},	// Наемник //
	{20, 14, 10, 10, 17, 12},	// Дружинник //
	{10,  9, 20, 18, 10, 13},	// Кудесник //
	{10,  9, 20, 18, 10, 13},	// Волшебник //
	{ 9,  9, 20, 20, 11, 10},	// Чернокнижник //
	{19, 10, 12, 15, 14, 13},	// Витязь //
	{19, 15, 11, 11, 14, 11},	// Охотник //
	{20, 14, 10, 11, 14, 12},	// Кузнец //
	{18, 14, 10, 10, 14, 13},	// Купец //
	{15, 10, 19, 15, 12, 12}	// Волхв //
};

// local functions //
int apply_ac(CHAR_DATA * ch, int eq_pos);
int apply_armour(CHAR_DATA * ch, int eq_pos);
void update_object(OBJ_DATA * obj, int use);
void update_char_objects(CHAR_DATA * ch);
bool is_wear_light(CHAR_DATA *ch);

// external functions //
void perform_drop_gold(CHAR_DATA * ch, int amount, byte mode, room_rnum RDR);
int mag_manacost(CHAR_DATA * ch, int spellnum);
int slot_for_char(CHAR_DATA * ch, int i);
int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
int invalid_clan(CHAR_DATA * ch, OBJ_DATA * obj);
void remove_follower(CHAR_DATA * ch);
void clearMemory(CHAR_DATA * ch);
int extra_damroll(int class_num, int level);
int Crash_delete_file(char *name, int mask);
void do_entergame(DESCRIPTOR_DATA * d);
ACMD(do_return);

extern void check_auction(CHAR_DATA * ch, OBJ_DATA * obj);
extern void check_exchange(OBJ_DATA * obj);
void free_script(SCRIPT_DATA * sc);
int get_player_charms(CHAR_DATA * ch, int spellnum);

extern struct zone_data *zone_table;
extern int global_uid;
extern void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
extern char *find_exdesc(char *word, EXTRA_DESCR_DATA * list);

char *fname(const char *namelist)
{
	static char holder[30];
	register char *point;

	for (point = holder; a_isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

/*
int isname(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  curname = namelist;
  for (;;)
    {for (curstr = str;; curstr++, curname++)
         {if (!*curstr && !a_isalpha(*curname))
	         return (1);

          if (!*curname)
	         return (0);

          if (!*curstr || *curname == ' ')
	      break;

          if (LOWER(*curstr) != LOWER(*curname))
	      break;
         }

     for (; a_isalpha(*curname); curname++);
     if (!*curname)
        return (0);
     curname++;
     }
}
*/

int isname(const char *str, const char *namelist)
{
	int once_ok = FALSE;
	const char *curname, *curstr, *laststr;

	if (!namelist || !*namelist || !str)
		return (FALSE);

	for (curstr = str; !a_isalnum(*curstr); curstr++)
	{
		if (!*curstr)
			return (once_ok);
	}
	laststr = curstr;
	curname = namelist;
	for (;;)
	{
		once_ok = FALSE;
		for (;; curstr++, curname++)
		{
			if (!*curstr)
				return (once_ok);
			if (*curstr == '!')
				if (a_isalnum(*curname))
				{
					curstr = laststr;
					break;
				}
			if (!a_isalnum(*curstr))
			{
				for (; !a_isalnum(*curstr); curstr++)
				{
					if (!*curstr)
						return (once_ok);
				}
				laststr = curstr;
				break;
			}
			if (!*curname)
				return (FALSE);
			if (!a_isalnum(*curname))
			{
				curstr = laststr;
				break;
			}
			if (LOWER(*curstr) != LOWER(*curname))
			{
				curstr = laststr;
				break;
			}
			else
				once_ok = TRUE;
		}
		// skip to next name
		for (; a_isalnum(*curname); curname++);
		for (; !a_isalnum(*curname); curname++)
		{
			if (!*curname)
				return (FALSE);
		}
	}
}
int isname(const std::string &str, const char *namelist)
{
	return isname(str.c_str(), namelist);
}

bool is_wear_light(CHAR_DATA *ch)
{
	bool wear_light = FALSE;
	for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++)
		if (GET_EQ(ch, wear_pos) && GET_OBJ_TYPE(GET_EQ(ch, wear_pos)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2))
			wear_light = TRUE;
	return wear_light;
}

void check_light(CHAR_DATA * ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef)
{
	if (IN_ROOM(ch) == NOWHERE)
		return;

	/*if (IS_IMMORTAL(ch))
	{
		sprintf(buf,"%d %d %d (%d)\r\n",world[IN_ROOM(ch)]->light,world[IN_ROOM(ch)]->glight,world[IN_ROOM(ch)]->gdark,koef);
		send_to_char(buf,ch);
	}*/

	// In equipment
	if (is_wear_light(ch))
	{
		if (was_equip == LIGHT_NO)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light + koef);
	}
	else
	{
		if (was_equip == LIGHT_YES)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light - koef);
	}

	// Singlelight affect
	if (AFF_FLAGGED(ch, AFF_SINGLELIGHT))
	{
		if (was_single == LIGHT_NO)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light + koef);
	}
	else
	{
		if (was_single == LIGHT_YES)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light - koef);
	}

	// Holylight affect
	if (AFF_FLAGGED(ch, AFF_HOLYLIGHT))
	{
		if (was_holylight == LIGHT_NO)
			world[ch->in_room]->glight = MAX(0, world[ch->in_room]->glight + koef);
	}
	else
	{
		if (was_holylight == LIGHT_YES)
			world[ch->in_room]->glight = MAX(0, world[ch->in_room]->glight - koef);
	}

	/*if (IS_IMMORTAL(ch))
	{
		sprintf(buf,"holydark was %d\r\n",was_holydark);
		send_to_char(buf,ch);
	}*/

	// Holydark affect
	if (AFF_FLAGGED(ch, AFF_HOLYDARK))  	// if (IS_IMMORTAL(ch))
	{
		/*if (IS_IMMORTAL(ch))
			send_to_char("HOLYDARK ON\r\n",ch);*/
		if (was_holydark == LIGHT_NO)
			world[ch->in_room]->gdark = MAX(0, world[ch->in_room]->gdark + koef);
	}
	else  		// if (IS_IMMORTAL(ch))
	{
		/*if (IS_IMMORTAL(ch))
			send_to_char("HOLYDARK OFF\r\n",ch);*/
		if (was_holydark == LIGHT_YES)
			world[ch->in_room]->gdark = MAX(0, world[ch->in_room]->gdark - koef);
	}

	/*if (IS_IMMORTAL(ch))
	{
		sprintf(buf,"%d %d %d (%d)\r\n",world[IN_ROOM(ch)]->light,world[IN_ROOM(ch)]->glight,world[IN_ROOM(ch)]->gdark,koef);
		send_to_char(buf,ch);
	}*/
}

void affect_modify(CHAR_DATA * ch, byte loc, sbyte mod, bitvector_t bitv, bool add)
{
	if (add)
	{
		SET_BIT(AFF_FLAGS(ch, bitv), bitv);
	}
	else
	{
		REMOVE_BIT(AFF_FLAGS(ch, bitv), bitv);
		mod = -mod;
	}

	switch (loc)
	{
	case APPLY_NONE:
		break;
	case APPLY_STR:
		ch->set_str_add(ch->get_str_add() + mod);
		break;
	case APPLY_DEX:
		ch->set_dex_add(ch->get_dex_add() + mod);
		break;
	case APPLY_INT:
		ch->set_int_add(ch->get_int_add() + mod);
		break;
	case APPLY_WIS:
		ch->set_wis_add(ch->get_wis_add() + mod);
		break;
	case APPLY_CON:
		ch->set_con_add(ch->get_con_add() + mod);
		break;
	case APPLY_CHA:
		ch->set_cha_add(ch->get_cha_add() + mod);
		break;
	case APPLY_CLASS:
		break;

		/*
		 * My personal thoughts on these two would be to set the person to the
		 * value of the apply.  That way you won't have to worry about people
		 * making +1 level things to be imp (you restrict anything that gives
		 * immortal level of course).  It also makes more sense to set someone
		 * to a class rather than adding to the class number. -gg
		 */

	case APPLY_LEVEL:
		break;
	case APPLY_AGE:
		GET_AGE_ADD(ch) += mod;
		break;
	case APPLY_CHAR_WEIGHT:
		GET_WEIGHT_ADD(ch) += mod;
		break;
	case APPLY_CHAR_HEIGHT:
		GET_HEIGHT_ADD(ch) += mod;
		break;
	case APPLY_MANAREG:
		GET_MANAREG(ch) += mod;
		break;
	case APPLY_HIT:
		GET_HIT_ADD(ch) += mod;
		break;
	case APPLY_MOVE:
		GET_MOVE_ADD(ch) += mod;
		break;
	case APPLY_GOLD:
		break;
	case APPLY_EXP:
		break;
	case APPLY_AC:
		GET_AC_ADD(ch) += mod;
		break;
	case APPLY_HITROLL:
		GET_HR_ADD(ch) += mod;
		break;
	case APPLY_DAMROLL:
		GET_DR_ADD(ch) += mod;
		break;
	case APPLY_SAVING_WILL:
		GET_SAVE(ch, SAVING_WILL) += mod;
		break;
	case APPLY_RESIST_FIRE:
		GET_RESIST(ch, FIRE_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_AIR:
		GET_RESIST(ch, AIR_RESISTANCE) += mod;
		break;
	case APPLY_SAVING_CRITICAL:
		GET_SAVE(ch, SAVING_CRITICAL) += mod;
		break;
	case APPLY_SAVING_STABILITY:
		GET_SAVE(ch, SAVING_STABILITY) += mod;
		break;
	case APPLY_SAVING_REFLEX:
		GET_SAVE(ch, SAVING_REFLEX) += mod;
		break;
	case APPLY_HITREG:
		GET_HITREG(ch) += mod;
		break;
	case APPLY_MOVEREG:
		GET_MOVEREG(ch) += mod;
		break;
	case APPLY_C1:
	case APPLY_C2:
	case APPLY_C3:
	case APPLY_C4:
	case APPLY_C5:
	case APPLY_C6:
	case APPLY_C7:
	case APPLY_C8:
	case APPLY_C9:
		ch->add_obj_slot(loc - APPLY_C1, mod);
		break;
	case APPLY_SIZE:
		GET_SIZE_ADD(ch) += mod;
		break;
	case APPLY_ARMOUR:
		GET_ARMOUR(ch) += mod;
		break;
	case APPLY_POISON:
		GET_POISON(ch) += mod;
		break;
	case APPLY_CAST_SUCCESS:
		GET_CAST_SUCCESS(ch) += mod;
		break;
	case APPLY_MORALE:
		GET_MORALE(ch) += mod;
		break;
	case APPLY_INITIATIVE:
		GET_INITIATIVE(ch) += mod;
		break;
	case APPLY_RELIGION:
		if (add)
			GET_PRAY(ch) |= mod;
		else
			GET_PRAY(ch) &= mod;
		break;
	case APPLY_ABSORBE:
		GET_ABSORBE(ch) += mod;
		break;
	case APPLY_LIKES:
		GET_LIKES(ch) += mod;
		break;
	case APPLY_RESIST_WATER:
		GET_RESIST(ch, WATER_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_EARTH:
		GET_RESIST(ch, EARTH_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_VITALITY:
		GET_RESIST(ch, VITALITY_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_MIND:
		GET_RESIST(ch, MIND_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_IMMUNITY:
		GET_RESIST(ch, IMMUNITY_RESISTANCE) += mod;
		break;
	case APPLY_AR:
		GET_AR(ch) += mod;
		break;
	case APPLY_MR:
		GET_MR(ch) += mod;
		break;
	case APPLY_ACONITUM_POISON:
		GET_POISON(ch) += mod;
		break;
	case APPLY_SCOPOLIA_POISON:
		GET_POISON(ch) += mod;
		break;
	case APPLY_BELENA_POISON:
		GET_POISON(ch) += mod;
		break;
	case APPLY_DATURA_POISON:
		GET_POISON(ch) += mod;
		break;
	case APPLY_HIT_GLORY: //вкачка +хп за славу
		GET_HIT_ADD(ch) += mod * GloryConst::HP_FACTOR;
		break;
	default:
		log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
		break;

	}			// switch
}

void affect_room_modify(ROOM_DATA * room, byte loc, sbyte mod, bitvector_t bitv, bool add)
{
	if (add)
	{
		SET_BIT(ROOM_AFF_FLAGS(room, bitv), bitv);
	}
	else
	{
		REMOVE_BIT(ROOM_AFF_FLAGS(room, bitv), bitv);
		mod = -mod;
	}

	switch (loc)
	{
	case APPLY_ROOM_NONE:
		break;
	case APPLY_ROOM_POISON:
		// Увеличиваем загаженность от аффекта вызываемого SPELL_POISONED_FOG
		// Хотя это сделанно скорее для примера пока не обрабатывается вообще
		GET_ROOM_ADD_POISON(room) += mod;
		break;
	default:
		log("SYSERR: Unknown room apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
		break;

	}			// switch
}

// Тут осуществляется апдейт аффектов влияющих на комнату
void affect_room_total(ROOM_DATA * room)
{
	AFFECT_DATA *af;
	// А че тут надо делать пересуммирование аффектов от заклов.
	/* Вобщем все комнаты имеют (вроде как базовую и
	   добавочную характеристику) если скажем ввести
	   возможность устанавливать степень зараженности комнтаы
	   в OLC , то дополнительное загаживание только будет вносить
	   + но не обнулять это значение. */

	// обнуляем все добавочные характеристики
	memset(&room->add_property, 0 , sizeof(room_property_data));

	// перенакладываем аффекты
	for (af = room->affected; af; af = af->next)
		affect_room_modify(room, af->location, af->modifier, af->bitvector, TRUE);

}

int char_saved_aff[] =
{
	AFF_GROUP,
	AFF_HORSE,
	0
};

int char_stealth_aff[] =
{
	AFF_HIDE,
	AFF_SNEAK,
	AFF_CAMOUFLAGE,
	0
};

///
/// Сет чару аффектов, которые должны висеть постоянно (через affect_total)
///
void apply_natural_affects(CHAR_DATA *ch)
{
	if (GET_REMORT(ch) <= 0 && !IS_IMMORTAL(ch))
	{
		affect_modify(ch, APPLY_HITREG, 50, AFF_NOOB_REGEN, TRUE);
		affect_modify(ch, APPLY_MOVEREG, 100, AFF_NOOB_REGEN, TRUE);
		affect_modify(ch, APPLY_MANAREG, 100, AFF_NOOB_REGEN, TRUE);
	}
}

// This updates a character by subtracting everything he is affected by
// restoring original abilities, and then affecting all again
void affect_total(CHAR_DATA * ch)
{
	AFFECT_DATA *af;
	OBJ_DATA *obj;
	struct extra_affects_type *extra_affect = NULL;

	int i, j;
	FLAG_DATA saved;

	// Init struct
	saved.flags[0] = 0;
	saved.flags[1] = 0;
	saved.flags[2] = 0;
	saved.flags[3] = 0;

	ch->clear_add_affects();

	// PC's clear all affects, because recalc one
	if (!IS_NPC(ch))
	{
		saved = ch->char_specials.saved.affected_by;
		ch->char_specials.saved.affected_by = clear_flags;
		for (i = 0; (j = char_saved_aff[i]); i++)
		{
			if (IS_SET(GET_FLAG(saved, j), j))
			{
				SET_BIT(AFF_FLAGS(ch, j), j);
			}
		}
	}

	// Restore values for NPC - added by Adept
	if (IS_NPC(ch))
	{
		(ch)->add_abils = (&mob_proto[GET_MOB_RNUM(ch)])->add_abils;
	}

	// move object modifiers
	for (i = 0; i < NUM_WEARS; i++)
	{
		if ((obj = GET_EQ(ch, i)))
		{
			if (ObjSystem::is_armor_type(obj))
			{
				GET_AC_ADD(ch) -= apply_ac(ch, i);
				GET_ARMOUR(ch) += apply_armour(ch, i);
			}
			// Update weapon applies
			for (j = 0; j < MAX_OBJ_AFFECT; j++)
				affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
							  GET_EQ(ch, i)->affected[j].modifier, 0, TRUE);
			// Update weapon bitvectors
			for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
			{
				// То же самое, но переформулировал
				if (weapon_affect[j].aff_bitvector == 0 || !IS_OBJ_AFF(obj, weapon_affect[j].aff_pos))
					continue;
				affect_modify(ch, APPLY_NONE, 0, weapon_affect[j].aff_bitvector, TRUE);
			}
		}
	}
	ch->obj_bonus().apply_affects(ch);

	// move features modifiers - added by Gorrah
	for (i = 1; i < MAX_FEATS; i++)
	{
		if (can_use_feat(ch, i) && (feat_info[i].type == AFFECT_FTYPE))
			for (j = 0; j < MAX_FEAT_AFFECT; j++)
				affect_modify(ch, feat_info[i].affected[j].location,
							  feat_info[i].affected[j].modifier, 0, TRUE);
	}

	// IMPREGNABLE_FEAT учитывается дважды: выше начисляем единичку за 0 мортов, а теперь по 1 за каждый морт
	if (can_use_feat(ch, IMPREGNABLE_FEAT))
	{
		for (j = 0; j < MAX_FEAT_AFFECT; j++)
			affect_modify(ch, feat_info[IMPREGNABLE_FEAT].affected[j].location,
						  MIN(9, feat_info[IMPREGNABLE_FEAT].affected[j].modifier*GET_REMORT(ch)), 0, TRUE);
	};

	// Обработка "выносливости" и "богатырского здоровья
	// Знаю, что кривовато, придумаете, как лучше - делайте
	if (!IS_NPC(ch))
	{
		if (can_use_feat(ch, ENDURANCE_FEAT))
			affect_modify(ch, APPLY_MOVE, GET_LEVEL(ch) * 2, 0, TRUE);
		if (can_use_feat(ch, SPLENDID_HEALTH_FEAT))
			affect_modify(ch, APPLY_HIT, GET_LEVEL(ch) * 2, 0, TRUE);
		GloryConst::apply_modifiers(ch);
		apply_natural_affects(ch);
	}

	// move affect modifiers
	for (af = ch->affected; af; af = af->next)
		affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

	// move race and class modifiers
	if (!IS_NPC(ch))
	{
		if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_CLASSES)
		{
			extra_affect = class_app[(int) GET_CLASS(ch)].extra_affects;
			//extra_modifier = class_app[(int) GET_CLASS(ch)].extra_modifiers;

			for (i = 0; extra_affect && (extra_affect + i)->affect != -1; i++)
				affect_modify(ch, APPLY_NONE, 0, (extra_affect + i)->affect,
							  (extra_affect + i)->set_or_clear ? true : false);
			/* for (i = 0; extra_modifier && (extra_modifier + i)->location != -1; i++)
				affect_modify(ch, (extra_modifier + i)->location,
					      (extra_modifier + i)->modifier, 0, TRUE);*/
		}

		// Apply other PC modifiers
		const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
		if (wdex != 0)
		{
			ch->set_dex_add(ch->get_dex_add() - wdex);
		}
		GET_DR_ADD(ch) += extra_damroll((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
		if (!AFF_FLAGGED(ch, AFF_NOOB_REGEN))
		{
			GET_HITREG(ch) += ((int) GET_LEVEL(ch) + 4) / 5 * 10;
		}
		if (GET_CON_ADD(ch))
		{
			GET_HIT_ADD(ch) += PlayerSystem::con_add_hp(ch);
			if ((i = GET_MAX_HIT(ch) + GET_HIT_ADD(ch)) < 1)
				GET_HIT_ADD(ch) -= (i - 1);
		}

		if (!WAITLESS(ch) && on_horse(ch))
		{
			REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
			REMOVE_BIT(AFF_FLAGS(ch, AFF_SNEAK), AFF_SNEAK);
			REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
			REMOVE_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);
		}
	}

	// correctize all weapon
	if (!IS_IMMORTAL(ch))
	{
		if ((obj = GET_EQ(ch, WEAR_BOTHS)) && !OK_BOTH(ch, obj))
		{
			if (!IS_NPC(ch))
			{
				act("Вам слишком тяжело держать $o3 в обоих руках!", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_BOTH_W);
			}
			act("$n прекратил$g использовать $o3.", FALSE, ch, obj, 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_BOTHS), ch);
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_WIELD)) && !OK_WIELD(ch, obj))
		{
			if (!IS_NPC(ch))
			{
				act("Вам слишком тяжело держать $o3 в правой руке!", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_WIELD_W);
			}
			act("$n прекратил$g использовать $o3.", FALSE, ch, obj, 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_WIELD), ch);
			// если пушку можно вооружить в обе руки и эти руки свободны
			if (CAN_WEAR(obj, ITEM_WEAR_BOTHS) && OK_BOTH(ch, obj)
				&& !GET_EQ(ch, WEAR_HOLD) && !GET_EQ(ch, WEAR_LIGHT)
				&& !GET_EQ(ch, WEAR_SHIELD) && !GET_EQ(ch, WEAR_WIELD)
				&& !GET_EQ(ch, WEAR_BOTHS))
			{
				equip_char(ch, obj, WEAR_BOTHS | 0x100);
			}
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_HOLD)) && !OK_HELD(ch, obj))
		{
			if (!IS_NPC(ch))
			{
				act("Вам слишком тяжело держать $o3 в левой руке!", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_HOLD_W);
			}
			act("$n прекратил$g использовать $o3.", FALSE, ch, obj, 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_HOLD), ch);
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_SHIELD)) && !OK_SHIELD(ch, obj))
		{
			if (!IS_NPC(ch))
			{
				act("Вам слишком тяжело держать $o3 на левой руке!", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_SHIELD_W);
			}
			act("$n прекратил$g использовать $o3.", FALSE, ch, obj, 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_SHIELD), ch);
			return;
		}
	}

	// calculate DAMAGE value
	GET_DAMAGE(ch) = (str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GET_REAL_DR(ch)) * 2;
	if ((obj = GET_EQ(ch, WEAR_BOTHS)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
		GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) >> 1;
	else
	{
		if ((obj = GET_EQ(ch, WEAR_WIELD))
				&& GET_OBJ_TYPE(obj) == ITEM_WEAPON)
			GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) >> 1;
		if ((obj = GET_EQ(ch, WEAR_HOLD)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
			GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) >> 1;
	}

	// Calculate CASTER value
	for (GET_CASTER(ch) = 0, i = 1; !IS_NPC(ch) && i <= MAX_SPELLS; i++)
		if (IS_SET(GET_SPELL_TYPE(ch, i), SPELL_KNOW | SPELL_TEMP))
			GET_CASTER(ch) += (spell_info[i].danger * GET_SPELL_MEM(ch, i));

	// Check steal affects
	for (i = 0; (j = char_stealth_aff[i]); i++)
	{
		if (IS_SET(GET_FLAG(saved, j), j) && !IS_SET(AFF_FLAGS(ch, j), j))
			CHECK_AGRO(ch) = TRUE;
	}

	check_berserk(ch);
	if (ch->get_fighting() || affected_by_spell(ch, SPELL_GLITTERDUST))
	{
		REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
		REMOVE_BIT(AFF_FLAGS(ch, AFF_SNEAK), AFF_SNEAK);
		REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
		REMOVE_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);
	}
}

/* Намазываем аффект на комнату.
  Автоматически ставим нузные флаги */
void affect_to_room(ROOM_DATA * room, AFFECT_DATA * af)
{
	AFFECT_DATA *affected_alloc;

	CREATE(affected_alloc, AFFECT_DATA, 1);

	*affected_alloc = *af;
	affected_alloc->next = room->affected;
	room->affected = affected_alloc;

	affect_room_modify(room, af->location, af->modifier, af->bitvector, TRUE);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
	affect_room_total(room);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
//	check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}



/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(CHAR_DATA * ch, AFFECT_DATA * af)
{
	long was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
				   was_hlgt = AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
							  was_hdrk = AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;
	AFFECT_DATA *affected_alloc;

	CREATE(affected_alloc, AFFECT_DATA, 1);

	*affected_alloc = *af;
	affected_alloc->next = ch->affected;
	ch->affected = affected_alloc;

	affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
	affect_total(ch);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
	check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */

void affect_remove(CHAR_DATA * ch, AFFECT_DATA * af)
{
	int was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
				  was_hlgt = AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
							 was_hdrk = AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;

	AFFECT_DATA *temp;

	// if (IS_IMMORTAL(ch))
	//   {sprintf(buf,"<%d>\r\n",was_hdrk);
	//    send_to_char(buf,ch);
	//   }

	if (ch->affected == NULL)
	{
		log("SYSERR: affect_remove(%s) when no affects...", GET_NAME(ch));
		// core_dump();
		return;
	}

	affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
	if (af->type == SPELL_ABSTINENT)
	{
		GET_DRUNK_STATE(ch) = GET_COND(ch, DRUNK) = MIN(GET_COND(ch, DRUNK), CHAR_DRUNKED - 1);
	}
	if (af->type == SPELL_DRUNKED && af->duration == 0)
		set_abstinent(ch);

	REMOVE_FROM_LIST(af, ch->affected, next);
	if (af->handler!=0) af->handler.reset();
	free(af);

	//log("[AFFECT_REMOVE->AFFECT_TOTAL] Start");
	affect_total(ch);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
	check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}



void affect_room_remove(ROOM_DATA * room, AFFECT_DATA * af)
{

	AFFECT_DATA *temp;
	int change = 0;

	// if (IS_IMMORTAL(ch))
	//   {sprintf(buf,"<%d>\r\n",was_hdrk);
	//    send_to_char(buf,ch);
	//   }

	if (room->affected == NULL)
	{
		log("SYSERR: affect_room_remove when no affects...");
		// core_dump();
		return;
	}

	affect_room_modify(room, af->location, af->modifier, af->bitvector, FALSE);
	if (change)
		affect_room_modify(room, af->location, af->modifier, af->bitvector, TRUE);
	else
	{
		REMOVE_FROM_LIST(af, room->affected, next);
		free(af);
	}
	//log("[AFFECT_REMOVE->AFFECT_TOTAL] Start");
	affect_room_total(room);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
//	check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}


// Call affect_remove with every spell of spelltype "skill"
void affect_from_char(CHAR_DATA * ch, int type)
{
	AFFECT_DATA *hjp, *next;

	for (hjp = ch->affected; hjp; hjp = next)
	{
		next = hjp->next;
		if (hjp->type == type)
		{
			affect_remove(ch, hjp);
		}
	}

	if (IS_NPC(ch) && type == SPELL_CHARM)
	{
		EXTRACT_TIMER(ch) = 5;
		ch->mob_specials.hire_price = 0;// added by WorM (Видолюб) 2010.06.04 Сбрасываем цену найма
	}
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(CHAR_DATA * ch, int type)
{
	AFFECT_DATA *hjp;

	if (type == SPELL_POWER_HOLD)
		type = SPELL_HOLD;
	else if (type == SPELL_POWER_SIELENCE)
		type = SPELL_SIELENCE;
	else if (type == SPELL_POWER_BLINDNESS)
		type = SPELL_BLINDNESS;


	for (hjp = ch->affected; hjp; hjp = hjp->next)
		if (hjp->type == type)
			return (TRUE);

	return (FALSE);
}
// Проверяем а не висит ли на комнате закла ужо
//bool room_affected_by_spell(ROOM_DATA * room, int type)
AFFECT_DATA *room_affected_by_spell(ROOM_DATA * room, int type)
{
	AFFECT_DATA *hjp;

	for (hjp = room->affected; hjp; hjp = hjp->next)
		if (hjp->type == type)
			return hjp;

	return NULL;
}

void affect_join_fspell(CHAR_DATA * ch, AFFECT_DATA * af)
{
	AFFECT_DATA *hjp;
	bool found = FALSE;

	for (hjp = ch->affected; !found && hjp; hjp = hjp->next)
	{
		if ((hjp->type == af->type) && (hjp->location == af->location))
		{

			if (hjp->modifier < af->modifier)
				hjp->modifier = af->modifier;
			if (hjp->duration < af->duration)
				hjp->duration = af->duration;
			affect_total(ch);
			found = TRUE;
		}
	}
	if (!found)
	{
		affect_to_char(ch, af);
	}
}
void affect_room_join_fspell(ROOM_DATA * room, AFFECT_DATA * af)
{
	AFFECT_DATA *hjp;
	bool found = FALSE;

	for (hjp = room->affected; !found && hjp; hjp = hjp->next)
	{
		if ((hjp->type == af->type) && (hjp->location == af->location))
		{

			if (hjp->modifier < af->modifier)
				hjp->modifier = af->modifier;
			if (hjp->duration < af->duration)
				hjp->duration = af->duration;
			affect_room_total(room);
			found = TRUE;
		}
	}
	if (!found)
	{
		affect_to_room(room, af);
	}
}

void affect_room_join(ROOM_DATA * room, AFFECT_DATA * af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
	AFFECT_DATA *hjp;
	bool found = FALSE;

	for (hjp = room->affected; !found && hjp && af->location; hjp = hjp->next)
	{
		if ((hjp->type == af->type) && (hjp->location == af->location))
		{
			if (add_dur)
				af->duration += hjp->duration;
			if (avg_dur)
				af->duration /= 2;
			if (add_mod)
				af->modifier += hjp->modifier;
			if (avg_mod)
				af->modifier /= 2;
			affect_room_remove(room, hjp);
			affect_to_room(room, af);
			found = TRUE;
		}
	}
	if (!found)
	{
		affect_to_room(room, af);
	}
}


void affect_join(CHAR_DATA * ch, AFFECT_DATA * af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
	AFFECT_DATA *hjp;
	bool found = FALSE;

	for (hjp = ch->affected; !found && hjp && af->location; hjp = hjp->next)
	{
		if ((hjp->type == af->type) && (hjp->location == af->location))
		{
			if (add_dur)
				af->duration += hjp->duration;
			if (avg_dur)
				af->duration /= 2;
			if (add_mod)
				af->modifier += hjp->modifier;
			if (avg_mod)
				af->modifier /= 2;
			affect_remove(ch, hjp);
			affect_to_char(ch, af);
			found = TRUE;
		}
	}
	if (!found)
	{
		affect_to_char(ch, af);
	}
}

// Обработка тикающих способностей - added by Gorrah
void timed_feat_to_char(CHAR_DATA * ch, struct timed_type *timed)
{
	struct timed_type *timed_alloc, *skj;

	// Карачун. Правка бага. Если такой фит уже есть в списке, просто меняем таймер.
	for (skj = ch->timed_feat; skj; skj = skj->next)
	{
		if (skj->skill == timed->skill)
		{
			skj->time = timed->time;
			return;
		}
	}

	CREATE(timed_alloc, struct timed_type, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed_feat;
	ch->timed_feat = timed_alloc;
}

void timed_feat_from_char(CHAR_DATA * ch, struct timed_type *timed)
{
	struct timed_type *temp;

	if (ch->timed_feat == NULL)
	{
		log("SYSERR: timed_feat_from_char(%s) when no timed...", GET_NAME(ch));
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed_feat, next);
	free(timed);
}

int timed_by_feat(CHAR_DATA * ch, int feat)
{
	struct timed_type *hjp;

	for (hjp = ch->timed_feat; hjp; hjp = hjp->next)
		if (hjp->skill == feat)
			return (hjp->time);

	return (0);
}
// End of changes

// Insert an timed_type in a char_data structure
void timed_to_char(CHAR_DATA * ch, struct timed_type *timed)
{
	struct timed_type *timed_alloc, *skj;

	// Карачун. Правка бага. Если такой скилл уже есть в списке, просто меняем таймер.
	for (skj = ch->timed; skj; skj = skj->next)
	{
		if (skj->skill == timed->skill)
		{
			skj->time = timed->time;
			return;
		}
	}

	CREATE(timed_alloc, struct timed_type, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed;
	ch->timed = timed_alloc;
}

void timed_from_char(CHAR_DATA * ch, struct timed_type *timed)
{
	struct timed_type *temp;

	if (ch->timed == NULL)
	{
		log("SYSERR: timed_from_char(%s) when no timed...", GET_NAME(ch));
		// core_dump();
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed, next);
	free(timed);
}

int timed_by_skill(CHAR_DATA * ch, int skill)
{
	struct timed_type *hjp;

	for (hjp = ch->timed; hjp; hjp = hjp->next)
		if (hjp->skill == skill)
			return (hjp->time);

	return (0);
}


// move a player out of a room
void char_from_room(CHAR_DATA * ch)
{
	CHAR_DATA *temp;

	if (ch == NULL || ch->in_room == NOWHERE)
	{
		log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
		return;
	}

	if (ch->get_fighting() != NULL)
		stop_fighting(ch, TRUE);

	if (!IS_NPC(ch))
		ch->set_from_room(ch->in_room);

	check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, -1);
	REMOVE_FROM_LIST(ch, world[ch->in_room]->people, next_in_room);
	ch->in_room = NOWHERE;
	ch->next_in_room = NULL;
	ch->track_dirs = 0;
}


// place a character in a room
void char_to_room(CHAR_DATA * ch, room_rnum room)
{
	if (ch == NULL || room < NOWHERE + 1 || room > top_of_world)
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
	else
	{
		if (!IS_NPC(ch) && RENTABLE(ch) && ROOM_FLAGGED(room, ROOM_ARENA) && !IS_IMMORTAL(ch))
		{
			send_to_char("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
			char_to_room(ch, ch->get_from_room());
			return;
		}

		ch->next_in_room = world[room]->people;
		world[room]->people = ch;
		ch->in_room = room;
		check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 1);
		REMOVE_BIT(EXTRA_FLAGS(ch, EXTRA_FAILHIDE), EXTRA_FAILHIDE);
		REMOVE_BIT(EXTRA_FLAGS(ch, EXTRA_FAILSNEAK), EXTRA_FAILSNEAK);
		REMOVE_BIT(EXTRA_FLAGS(ch, EXTRA_FAILCAMOUFLAGE), EXTRA_FAILCAMOUFLAGE);
		if (PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			sprintf(buf,
					"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
					"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
					CCNRM(ch, C_NRM), CCINRM(ch, C_NRM), room,
					CCRED(ch, C_NRM), CCIRED(ch, C_NRM), world[room]->light,
					CCGRN(ch, C_NRM), CCIGRN(ch, C_NRM), world[room]->glight,
					CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[room]->fires,
					CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[room]->ices,
					CCBLU(ch, C_NRM), CCIBLU(ch, C_NRM), world[room]->gdark,
					CCMAG(ch, C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
					CCWHT(ch, C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
					CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}

		// Stop fighting now, if we left.
		if (ch->get_fighting() && IN_ROOM(ch) != IN_ROOM(ch->get_fighting()))
		{
			stop_fighting(ch->get_fighting(), FALSE);
			stop_fighting(ch, TRUE);
		}

		if (!IS_NPC(ch))
		{
			zone_table[world[room]->zone].used = 1;
			zone_table[world[room]->zone].activity++;
		}
	}
}

void restore_object(OBJ_DATA * obj, CHAR_DATA * ch)
{
	int i, j;
	if ((i = GET_OBJ_RNUM(obj)) < 0)
		return;
	if (GET_OBJ_OWNER(obj) && OBJ_FLAGGED(obj, ITEM_NODONATE) && (!ch || GET_UNIQUE(ch) != GET_OBJ_OWNER(obj)))
	{
		GET_OBJ_VAL(obj, 0) = GET_OBJ_VAL(obj_proto[i], 0);
		GET_OBJ_VAL(obj, 1) = GET_OBJ_VAL(obj_proto[i], 1);
		GET_OBJ_VAL(obj, 2) = GET_OBJ_VAL(obj_proto[i], 2);
		GET_OBJ_VAL(obj, 3) = GET_OBJ_VAL(obj_proto[i], 3);
		GET_OBJ_MATER(obj) = GET_OBJ_MATER(obj_proto[i]);
		GET_OBJ_MAX(obj) = GET_OBJ_MAX(obj_proto[i]);
		GET_OBJ_CUR(obj) = 1;
		GET_OBJ_WEIGHT(obj) = GET_OBJ_WEIGHT(obj_proto[i]);
		obj->set_timer(24 * 60);
		obj->obj_flags.extra_flags = obj_proto[i]->obj_flags.extra_flags;
		obj->obj_flags.affects = obj_proto[i]->obj_flags.affects;
		GET_OBJ_WEAR(obj) = GET_OBJ_WEAR(obj_proto[i]);
		GET_OBJ_OWNER(obj) = 0;
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			obj->affected[j] = obj_proto[i]->affected[j];
	}
}

// выясняет, стокаются ли объекты с метками
bool stockable_custom_labels(OBJ_DATA *obj_one, OBJ_DATA *obj_two)
{
	// без меток стокаются
	if (!obj_one->custom_label && !obj_two->custom_label)
		return 1;

	if (obj_one->custom_label && obj_two->custom_label)
	{
		// с разными типами меток не стокаются
		if (!obj_one->custom_label->clan != !obj_two->custom_label->clan)
			return 0;
		else
		{
			// обе метки клановые один клан, текст совпадает -- стокается
			if (obj_one->custom_label->clan && obj_two->custom_label->clan &&
				!strcmp(obj_one->custom_label->clan, obj_two->custom_label->clan) &&
				obj_one->custom_label->label_text && obj_two->custom_label->label_text &&
				!strcmp(obj_one->custom_label->label_text, obj_two->custom_label->label_text))
				return 1;
			// обе метки личные, один автор, текст совпадает -- стокается
			if (obj_one->custom_label->author == obj_two->custom_label->author &&
				obj_one->custom_label->label_text && obj_two->custom_label->label_text &&
				!strcmp(obj_one->custom_label->label_text, obj_two->custom_label->label_text))
				return 1;
		}
	}

	return 0;
}

// выяснение стокаются ли предметы
bool equal_obj(OBJ_DATA *obj_one, OBJ_DATA *obj_two)
{
	if (GET_OBJ_VNUM(obj_one) != GET_OBJ_VNUM(obj_two)
			|| strcmp(obj_one->short_description, obj_two->short_description)
			|| (GET_OBJ_TYPE(obj_one) == ITEM_DRINKCON && GET_OBJ_VAL(obj_one, 2) != GET_OBJ_VAL(obj_two, 2))
			|| (GET_OBJ_TYPE(obj_one) == ITEM_CONTAINER && (obj_one->contains || obj_two->contains))
			|| GET_OBJ_VNUM(obj_two) == -1
			|| (GET_OBJ_TYPE(obj_one) == ITEM_BOOK && GET_OBJ_VAL(obj_one, 1) != GET_OBJ_VAL(obj_two, 1))
			|| !stockable_custom_labels(obj_one, obj_two))
	{
		return 0;
	}
	return 1;
}

namespace
{

// перемещаем стокающиеся предметы вверх контейнера и сверху кладем obj
void insert_obj_and_group(OBJ_DATA *obj, OBJ_DATA **list_start)
{
	// AL: пофиксил Ж)
	// Krodo: пофиксили третий раз, не сортируем у мобов в инве Ж)

	// begin - первый предмет в исходном списке
	// end - последний предмет в перемещаемом интервале
	// before - последний предмет перед началом интервала
	OBJ_DATA *p, *begin, *end, *before;

	obj->next_content = begin = *list_start;
	*list_start = obj;

	// похожий предмет уже первый в списке или список пустой
	if (!begin || equal_obj(begin, obj)) return;

	before = p = begin;

	while (p && !equal_obj(p, obj))
		before = p, p = p->next_content;

	// нет похожих предметов
	if (!p) return;

	end = p;

	while (p && equal_obj(p, obj))
		end = p, p = p->next_content;

	end->next_content = begin;
	obj->next_content = before->next_content;
	before->next_content = p; // будет 0 если после перемещаемых ничего не лежало
}

} // no-name namespace

// * Инициализация уида для нового объекта.
void set_uid(OBJ_DATA *object)
{
	if (GET_OBJ_VNUM(object) > 0 && // Объект не виртуальный
		GET_OBJ_UID(object) == 0)   // У объекта точно нет уида
	{
		global_uid++; // Увеличиваем глобальный счетчик уидов
		global_uid = global_uid == 0 ? 1 : global_uid; // Если произошло переполнение инта
		GET_OBJ_UID(object) = global_uid; // Назначаем уид
	}
}

// give an object to a char
void obj_to_char(OBJ_DATA * object, CHAR_DATA * ch)
{
	OBJ_DATA *i;
	unsigned int tuid;
	int inworld;

	int may_carry = TRUE;
	if (object && ch)
	{
		restore_object(object, ch);

		if (invalid_anti_class(ch, object) || invalid_unique(ch, object) || NamedStuff::check_named(ch, object, 0))
			may_carry = FALSE;

		if (strstr(object->aliases, "clan"))
		{
			if (!CLAN(ch))
				may_carry = FALSE;
			else
			{
				char buf[128];
				sprintf(buf, "clan%d!", CLAN(ch)->GetRent());
				if (!strstr(object->aliases, buf))
					may_carry = FALSE;
			}
		}

		if (!may_carry)
		{
			act("Вас обожгло при попытке взять $o3.", FALSE, ch, object, 0, TO_CHAR);
			act("$n попытал$u взять $o3 - и чудом не сгорел$g.", FALSE, ch, object, 0, TO_ROOM);
			obj_to_room(object, IN_ROOM(ch));
			return;
		}

		if (!IS_NPC(ch) || (ch->master && !IS_NPC(ch->master)))
		{
			// Контроль уникальности предметов
			if (object && // Объект существует
					GET_OBJ_UID(object) != 0 && // Есть UID
					object->get_timer() > 0) // Целенький
			{
				tuid = GET_OBJ_UID(object);
				inworld = 1;
				// Объект готов для проверки. Ищем в мире такой же.
				for (i = object_list; i; i = i->next)
				{
					if (GET_OBJ_UID(i) == tuid && // UID совпадает
							i->get_timer() > 0 && // Целенький
							object != i && // Не оно же
							GET_OBJ_VNUM(i) == GET_OBJ_VNUM(object))   // Для верности
					{
						inworld++;
					}
				}
				if (inworld > 1) // У объекта есть как минимум одна копия
				{
					sprintf(buf, "Copy detected and prepared to extract! Object %s (UID=%u, VNUM=%d), holder %s. In world %d.",
							object->PNames[0], GET_OBJ_UID(object), GET_OBJ_VNUM(object), GET_NAME(ch), inworld);
					mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
					// Удаление предмета
					act("$o0 замигал$Q и вы увидели медленно проступившие руны 'DUPE'.", FALSE, ch, object, 0, TO_CHAR);
					object->set_timer(0); // Хана предмету, развалится на тике
					SET_BIT(GET_OBJ_EXTRA(object, ITEM_NOSELL), ITEM_NOSELL); // Ибо нефиг
				}
			} // Назначаем UID
			else
			{
				set_uid(object);
				log("%s obj_to_char %s #%d|%u", GET_NAME(ch), object->PNames[0], GET_OBJ_VNUM(object), object->uid);
			}
		}

		if (!IS_NPC(ch) || (ch->master && !IS_NPC(ch->master)))
		{
			SET_BIT(GET_OBJ_EXTRA(object, ITEM_TICKTIMER), ITEM_TICKTIMER);
			insert_obj_and_group(object, &ch->carrying);
		}
		else
		{
			// Вот эта муть, чтобы временно обойти завязку магазинов на порядке предметов в инве моба // Krodo
			object->next_content = ch->carrying;
			ch->carrying = object;
		}

		object->carried_by = ch;
		object->in_room = NOWHERE;
		IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
		IS_CARRYING_N(ch)++;

		if (!IS_NPC(ch))
		{
			log("obj_to_char: %s -> %d", ch->get_name(),
				GET_OBJ_VNUM(object));
		}
		// set flag for crash-save system, but not on mobs!
		if (!IS_NPC(ch))
		{
			SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
		}
	}
	else
		log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}


// take an object from a char
void obj_from_char(OBJ_DATA * object)
{
	OBJ_DATA *temp;

	if (!object || !object->carried_by)
	{
		log("SYSERR: NULL object or owner passed to obj_from_char");
		return;
	}
	REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

	// set flag for crash-save system, but not on mobs!
	if (!IS_NPC(object->carried_by))
	{
		SET_BIT(PLR_FLAGS(object->carried_by, PLR_CRASH), PLR_CRASH);
		log("obj_from_char: %s -> %d", object->carried_by->get_name(),
			GET_OBJ_VNUM(object));
	}

	IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
	IS_CARRYING_N(object->carried_by)--;
	object->carried_by = NULL;
	object->next_content = NULL;
}



// Return the effect of a piece of armor in position eq_pos
int apply_ac(CHAR_DATA * ch, int eq_pos)
{
	int factor = 1;

	if (GET_EQ(ch, eq_pos) == NULL)
	{
		log("SYSERR: apply_ac(%s,%d) when no equip...", GET_NAME(ch), eq_pos);
		// core_dump();
		return (0);
	}

	if (!ObjSystem::is_armor_type(GET_EQ(ch, eq_pos)))
	{
		return (0);
	}

	switch (eq_pos)
	{
	case WEAR_BODY:
		factor = 3;
		break;		// 30% //
	case WEAR_HEAD:
		factor = 2;
		break;		// 20% //
	case WEAR_LEGS:
		factor = 2;
		break;		// 20% //
	default:
		factor = 1;
		break;		// all others 10% //
	}

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		factor *= MOB_AC_MULT;

	return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int apply_armour(CHAR_DATA * ch, int eq_pos)
{
	int factor = 1;
	OBJ_DATA *obj = GET_EQ(ch, eq_pos);

	if (!obj)
	{
		log("SYSERR: apply_armor(%s,%d) when no equip...", GET_NAME(ch), eq_pos);
		// core_dump();
		return (0);
	}

	if (!ObjSystem::is_armor_type(obj))
		return (0);

	switch (eq_pos)
	{
	case WEAR_BODY:
		factor = 3;
		break;		// 30% //
	case WEAR_HEAD:
		factor = 2;
		break;		// 20% //
	case WEAR_LEGS:
		factor = 2;
		break;		// 20% //
	default:
		factor = 1;
		break;		// all others 10% //
	}

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		factor *= MOB_ARMOUR_MULT;

	// чтобы не плюсовать левую броню на стафе с текущей прочностью выше максимальной
	int cur_dur = MIN(GET_OBJ_MAX(obj), GET_OBJ_CUR(obj));
	return (factor * GET_OBJ_VAL(obj, 1) * cur_dur / MAX(1, GET_OBJ_MAX(obj)));
}

int invalid_align(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_NPC(ch) || IS_IMMORTAL(ch))
		return (FALSE);
	if (IS_OBJ_ANTI(obj, ITEM_AN_MONO) && GET_RELIGION(ch) == RELIGION_MONO)
		return TRUE;
	if (IS_OBJ_ANTI(obj, ITEM_AN_POLY) && GET_RELIGION(ch) == RELIGION_POLY)
		return TRUE;
	return FALSE;
}

void wear_message(CHAR_DATA * ch, OBJ_DATA * obj, int where)
{
	const char *wear_messages[][2] =
	{
		{"$n засветил$g $o3 и взял$g во вторую руку.",
			"Вы зажгли $o3 и взяли во вторую руку."},

		{"$n0 надел$g $o3 на правый указательный палец.",
		 "Вы надели $o3 на правый указательный палец."},

		{"$n0 надел$g $o3 на левый указательный палец.",
		 "Вы надели $o3 на левый указательный палец."},

		{"$n0 надел$g $o3 вокруг шеи.",
		 "Вы надели $o3 вокруг шеи."},

		{"$n0 надел$g $o3 на грудь.",
		 "Вы надели $o3 на грудь."},

		{"$n0 надел$g $o3 на туловище.",
		 "Вы надели $o3 на туловище.", },

		{"$n0 водрузил$g $o3 на голову.",
		 "Вы водрузили $o3 себе на голову."},

		{"$n0 надел$g $o3 на ноги.",
		 "Вы надели $o3 на ноги."},

		{"$n0 обул$g $o3.",
		 "Вы обули $o3."},

		{"$n0 надел$g $o3 на кисти.",
		 "Вы надели $o3 на кисти."},

		{"$n0 надел$g $o3 на руки.",
		 "Вы надели $o3 на руки."},

		{"$n0 начал$g использовать $o3 как щит.",
		 "Вы начали использовать $o3 как щит."},

		{"$n0 облачил$u в $o3.",
		 "Вы облачились в $o3."},

		{"$n0 надел$g $o3 вокруг пояса.",
		 "Вы надели $o3 вокруг пояса."},

		{"$n0 надел$g $o3 вокруг правого запястья.",
		 "Вы надели $o3 вокруг правого запястья."},

		{"$n0 надел$g $o3 вокруг левого запястья.",
		 "Вы надели $o3 вокруг левого запястья."},

		{"$n0 взял$g в правую руку $o3.",
		 "Вы вооружились $o4."},

		{"$n0 взял$g $o3 в левую руку.",
		 "Вы взяли $o3 в левую руку."},

		{"$n0 взял$g $o3 в обе руки.",
		 "Вы взяли $o3 в обе руки."}
	};

	act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
	act(wear_messages[where][0], IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) ? FALSE : TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
}

int flag_data_by_char_class(const CHAR_DATA * ch)
{
	if (ch == NULL)
		return 0;

	return flag_data_by_num(IS_NPC(ch) ? NUM_CLASSES * NUM_KIN : GET_CLASS(ch) + NUM_CLASSES * GET_KIN(ch));
}

unsigned int activate_stuff(CHAR_DATA * ch, OBJ_DATA * obj,
							id_to_set_info_map::const_iterator it, int pos, unsigned int set_obj_qty)
{
	int show_msg = IS_SET(pos, 0x80), no_cast = IS_SET(pos, 0x40);
	std::string::size_type delim;

	REMOVE_BIT(pos, (0x80 | 0x40));

	if (pos < NUM_WEARS)
	{
		set_info::const_iterator set_obj_info;

		if (GET_EQ(ch, pos) && OBJ_FLAGGED(GET_EQ(ch, pos), ITEM_SETSTUFF) &&
				(set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end())
		{
			unsigned int oqty = activate_stuff(ch, obj, it, (pos + 1) | (show_msg ? 0x80 : 0) | (no_cast ? 0x40 : 0),
											   set_obj_qty + 1);
			qty_to_camap_map::const_iterator qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator old_qty_info = GET_EQ(ch, pos) == obj ?
					set_obj_info->second.begin() :
					set_obj_info->second.upper_bound(oqty - 1);

			while (qty_info != old_qty_info)
			{
				class_to_act_map::const_iterator class_info;

				qty_info--;
				if ((class_info =
							qty_info->second.find(unique_bit_flag_data().set_plane(flag_data_by_char_class(ch)))) !=
						qty_info->second.end())
				{
					if (GET_EQ(ch, pos) != obj)
					{
						for (int i = 0; i < MAX_OBJ_AFFECT; i++)
							affect_modify(ch, GET_EQ(ch, pos)->affected[i].location, GET_EQ(ch, pos)->affected[i].modifier,
										  0, FALSE);

						if (IN_ROOM(ch) != NOWHERE)
							for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
							{
								if (weapon_affect[i].aff_bitvector == 0 ||
										!IS_OBJ_AFF(GET_EQ(ch, pos), weapon_affect[i].aff_pos))
									continue;
								affect_modify(ch, APPLY_NONE, 0, weapon_affect[i].aff_bitvector, FALSE);
							}
					}

					std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info->second);
					delim = act_msg.find('\n');

					if (show_msg)
					{
						act(act_msg.substr(0, delim).c_str(), FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
						act(act_msg.erase(0, delim + 1).c_str(),
							IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) ? FALSE : TRUE,
							ch, GET_EQ(ch, pos), 0, TO_ROOM);
					}

					for (int i = 0; i < MAX_OBJ_AFFECT; i++)
						affect_modify(ch, GET_EQ(ch, pos)->affected[i].location, GET_EQ(ch, pos)->affected[i].modifier, 0, TRUE);

					if (IN_ROOM(ch) != NOWHERE)
						for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
						{
							if (weapon_affect[i].aff_spell == 0 || !IS_OBJ_AFF(GET_EQ(ch, pos), weapon_affect[i].aff_pos))
								continue;
							if (!no_cast)
							{
								if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
								{
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										FALSE, ch, GET_EQ(ch, pos), 0, TO_ROOM);
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
								}
								else
									mag_affects(GET_LEVEL(ch), ch, ch, weapon_affect[i].aff_spell,
												SAVING_WILL);
							}
						}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj)
			{
				for (int i = 0; i < MAX_OBJ_AFFECT; i++)
					affect_modify(ch, obj->affected[i].location, obj->affected[i].modifier, 0, TRUE);

				if (IN_ROOM(ch) != NOWHERE)
					for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
					{
						if (weapon_affect[i].aff_spell == 0 || !IS_OBJ_AFF(obj, weapon_affect[i].aff_pos))
							continue;
						if (!no_cast)
						{
							if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
							{
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									FALSE, ch, obj, 0, TO_ROOM);
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									FALSE, ch, obj, 0, TO_CHAR);
							}
							else
								mag_affects(GET_LEVEL(ch), ch, ch, weapon_affect[i].aff_spell,
											SAVING_WILL);
						}
					}
			}

			return oqty;
		}
		else
			return activate_stuff(ch, obj, it, (pos + 1) | (show_msg ? 0x80 : 0) | (no_cast ? 0x40 : 0), set_obj_qty);
	}
	else
		return set_obj_qty;
}

bool check_armor_type(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_LIGHT && !can_use_feat(ch, ARMOR_LIGHT_FEAT))
	{
		act("Для использования $o1 требуется способность 'легкие доспехи'.",
				FALSE, ch, obj, 0, TO_CHAR);
		return false;
	}
	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_MEDIAN && !can_use_feat(ch, ARMOR_MEDIAN_FEAT))
	{
		act("Для использования $o1 требуется способность 'средние доспехи'.",
				FALSE, ch, obj, 0, TO_CHAR);
		return false;
	}
	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_HEAVY && !can_use_feat(ch, ARMOR_HEAVY_FEAT))
	{
		act("Для использования $o1 требуется способность 'тяжелые доспехи'.",
				FALSE, ch, obj, 0, TO_CHAR);
		return false;
	}
	return true;
}

//  0x40 - no spell casting
//  0x80 - no total affect update
// 0x100 - show wear and activation messages
void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int pos)
{
	int was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
				  was_hlgt = AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
							 was_hdrk = AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
										was_lamp = FALSE;
	int j, show_msg = IS_SET(pos, 0x100), skip_total = IS_SET(pos, 0x80),
					  no_cast = IS_SET(pos, 0x40);

	REMOVE_BIT(pos, (0x100 | 0x80 | 0x40));

	if (pos < 0 || pos >= NUM_WEARS)
	{
		log("SYSERR: equip_char(%s,%d) in unknown pos...", GET_NAME(ch), pos);
		return;
	}

	if (GET_EQ(ch, pos))
	{
		log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->short_description);
		return;
	}
	//if (obj->carried_by) {
	//	log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj, ch, 0));
	//	return;
	//}
	if (obj->in_room != NOWHERE)
	{
		log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj, ch, 0));
		return;
	}

	if (invalid_anti_class(ch, obj))
	{
		act("Вас обожгло при попытке использовать $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n попытал$u использовать $o3 - и чудом не обгорел$g.", FALSE, ch, obj, 0, TO_ROOM);
		if (obj->carried_by)
			obj_from_char(obj);
		obj_to_room(obj, IN_ROOM(ch));
		obj_decay(obj);
		return;
	}
	else if((!IS_NPC(ch) || IS_CHARMICE(ch)) && OBJ_FLAGGED(obj, ITEM_NAMED) && NamedStuff::check_named(ch, obj, true)) {
		if(!NamedStuff::wear_msg(ch, obj))
			send_to_char("Просьба не трогать! Частная собственность!\r\n", ch);
		if (!obj->carried_by)
			obj_to_char(obj, ch);
		return;
	}

	if ((!IS_NPC(ch) && invalid_align(ch, obj)) || invalid_no_class(ch, obj)
			|| (AFF_FLAGGED(ch, AFF_CHARM) && (OBJ_FLAGGED(obj, ITEM_SHARPEN) || OBJ_FLAGGED(obj, ITEM_ARMORED))))
	{
		act("$o0 явно не предназначен$A для вас.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", FALSE, ch, obj, 0, TO_ROOM);
		if (!obj->carried_by)
			obj_to_char(obj, ch);
		return;
	}

	if (!IS_NPC(ch) || IS_CHARMICE(ch))
	{
		CHAR_DATA *master = IS_CHARMICE(ch) && ch->master ? ch->master : ch;
		if ((obj->get_mort_req() > GET_REMORT(master)) && !IS_IMMORTAL(master))
		{
			send_to_char(master, "Для использования %s требуется %d %s.\r\n",
					GET_OBJ_PNAME(obj, 1), obj->get_mort_req(),
					desc_count(obj->get_mort_req(), WHAT_REMORT));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
					FALSE, ch, obj, 0, TO_ROOM);
			if (!obj->carried_by)
				obj_to_char(obj, ch);
			return;
		}
	}

	//if (!IS_NPC(ch) && !check_armor_type(ch, obj))
	//{
	//	act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
	//			FALSE, ch, obj, 0, TO_ROOM);
	//	if (!obj->carried_by)
	//		obj_to_char(obj, ch);
	//	return;
	//} Нафиг недоделки (Купала)

	if (obj->carried_by)
		obj_from_char(obj);

	//if (GET_EQ(ch, WEAR_LIGHT) &&
	//  GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))
	//  was_lamp = TRUE;
	//Polud светить должно не только то что надето для освещения, а любой источник света
	was_lamp = is_wear_light(ch);
	//-Polud

	GET_EQ(ch, pos) = obj;
	obj->worn_by = ch;
	obj->worn_on = pos;
	obj->next_content = NULL;
	CHECK_AGRO(ch) = TRUE;

	if (show_msg)
	{
		wear_message(ch, obj, pos);
		if (OBJ_FLAGGED(obj, ITEM_NAMED))
			NamedStuff::wear_msg(ch, obj);
	}

	if (ch->in_room == NOWHERE)
		log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));

	id_to_set_info_map::iterator it = obj_data::set_table.begin();

	if (OBJ_FLAGGED(obj, ITEM_SETSTUFF))
		for (; it != obj_data::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				activate_stuff(ch, obj, it, 0 | (show_msg ? 0x80 : 0) | (no_cast ? 0x40 : 0), 0);
				break;
			}

	if (!OBJ_FLAGGED(obj, ITEM_SETSTUFF) || it == obj_data::set_table.end())
	{
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, 0, TRUE);

		if (IN_ROOM(ch) != NOWHERE)
			for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
			{
				if (weapon_affect[j].aff_spell == 0 || !IS_OBJ_AFF(obj, weapon_affect[j].aff_pos))
					continue;
				if (!no_cast)
				{
					if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
					{
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							FALSE, ch, obj, 0, TO_ROOM);
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							FALSE, ch, obj, 0, TO_CHAR);
					}
					else
						mag_affects(GET_LEVEL(ch), ch, ch, weapon_affect[j].aff_spell, SAVING_WILL);
				}
			}
	}

	if (!skip_total)
	{
		if (obj_sets::is_set_item(obj))
		{
			ch->obj_bonus().update(ch);
		}
		affect_total(ch);
		check_light(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}
}

unsigned int deactivate_stuff(CHAR_DATA * ch, OBJ_DATA * obj,
							  id_to_set_info_map::const_iterator it, int pos, unsigned int set_obj_qty)
{
	int show_msg = IS_SET(pos, 0x40);
	std::string::size_type delim;

	REMOVE_BIT(pos, 0x40);

	if (pos < NUM_WEARS)
	{
		set_info::const_iterator set_obj_info;

		if (GET_EQ(ch, pos) && OBJ_FLAGGED(GET_EQ(ch, pos), ITEM_SETSTUFF) &&
				(set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end())
		{
			unsigned int oqty = deactivate_stuff(ch, obj, it, (pos + 1) | (show_msg ? 0x40 : 0),
												 set_obj_qty + 1);
			qty_to_camap_map::const_iterator old_qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator qty_info = GET_EQ(ch, pos) == obj ?
					set_obj_info->second.begin() :
					set_obj_info->second.upper_bound(oqty - 1);

			while (old_qty_info != qty_info)
			{
				class_to_act_map::const_iterator class_info;

				old_qty_info--;
				if ((class_info =
							old_qty_info->second.find(unique_bit_flag_data().set_plane(flag_data_by_char_class(ch)))) !=
						old_qty_info->second.end())
				{
					while (qty_info != set_obj_info->second.begin())
					{
						class_to_act_map::const_iterator class_info2;

						qty_info--;
						if ((class_info2 =
									qty_info->second.find(unique_bit_flag_data().set_plane(flag_data_by_char_class(ch)))) !=
								qty_info->second.end())
						{
							for (int i = 0; i < MAX_OBJ_AFFECT; i++)
								affect_modify(ch, GET_EQ(ch, pos)->affected[i].location,
											  GET_EQ(ch, pos)->affected[i].modifier, 0, FALSE);

							if (IN_ROOM(ch) != NOWHERE)
								for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
								{
									if (weapon_affect[i].aff_bitvector == 0 ||
											!IS_OBJ_AFF(GET_EQ(ch, pos), weapon_affect[i].aff_pos))
										continue;
									affect_modify(ch, APPLY_NONE, 0, weapon_affect[i].aff_bitvector, FALSE);
								}

							std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info2->second);
							delim = act_msg.find('\n');

							if (show_msg)
							{
								act(act_msg.substr(0, delim).c_str(), FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
								act(act_msg.erase(0, delim + 1).c_str(),
									IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) ? FALSE : TRUE,
									ch, GET_EQ(ch, pos), 0, TO_ROOM);
							}

							for (int i = 0; i < MAX_OBJ_AFFECT; i++)
								affect_modify(ch, GET_EQ(ch, pos)->affected[i].location,
											  GET_EQ(ch, pos)->affected[i].modifier, 0, TRUE);

							if (IN_ROOM(ch) != NOWHERE)
								for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
								{
									if (weapon_affect[i].aff_bitvector == 0 ||
											!IS_OBJ_AFF(GET_EQ(ch, pos), weapon_affect[i].aff_pos))
										continue;
									affect_modify(ch, APPLY_NONE, 0, weapon_affect[i].aff_bitvector, TRUE);
								}

							return oqty;
						}
					}

					for (int i = 0; i < MAX_OBJ_AFFECT; i++)
						affect_modify(ch, GET_EQ(ch, pos)->affected[i].location,
									  GET_EQ(ch, pos)->affected[i].modifier, 0, FALSE);

					if (IN_ROOM(ch) != NOWHERE)
						for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
						{
							if (weapon_affect[i].aff_bitvector == 0 ||
									!IS_OBJ_AFF(GET_EQ(ch, pos), weapon_affect[i].aff_pos))
								continue;
							affect_modify(ch, APPLY_NONE, 0, weapon_affect[i].aff_bitvector, FALSE);
						}

					std::string deact_msg = GET_EQ(ch, pos)->deactivate_obj(class_info->second);
					delim = deact_msg.find('\n');

					if (show_msg)
					{
						act(deact_msg.substr(0, delim).c_str(), FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
						act(deact_msg.erase(0, delim + 1).c_str(),
							IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) ? FALSE : TRUE,
							ch, GET_EQ(ch, pos), 0, TO_ROOM);
					}

					if (GET_EQ(ch, pos) != obj)
					{
						for (int i = 0; i < MAX_OBJ_AFFECT; i++)
							affect_modify(ch, GET_EQ(ch, pos)->affected[i].location, GET_EQ(ch, pos)->affected[i].modifier,
										  0, TRUE);

						if (IN_ROOM(ch) != NOWHERE)
							for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
							{
								if (weapon_affect[i].aff_bitvector == 0 ||
										!IS_OBJ_AFF(GET_EQ(ch, pos), weapon_affect[i].aff_pos))
									continue;
								affect_modify(ch, APPLY_NONE, 0, weapon_affect[i].aff_bitvector, TRUE);
							}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj)
			{
				for (int i = 0; i < MAX_OBJ_AFFECT; i++)
					affect_modify(ch, obj->affected[i].location, obj->affected[i].modifier, 0, FALSE);

				if (IN_ROOM(ch) != NOWHERE)
					for (int i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
					{
						if (weapon_affect[i].aff_bitvector == 0 || !IS_OBJ_AFF(obj, weapon_affect[i].aff_pos))
							continue;
						affect_modify(ch, APPLY_NONE, 0, weapon_affect[i].aff_bitvector, FALSE);
					}

				obj->deactivate_obj(activation());
			}

			return oqty;
		}
		else
			return deactivate_stuff(ch, obj, it, (pos + 1) | (show_msg ? 0x40 : 0), set_obj_qty);
	}
	else
		return set_obj_qty;
}

//  0x40 - show setstuff related messages
//  0x80 - no total affect update
OBJ_DATA *unequip_char(CHAR_DATA * ch, int pos)
{
	int was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
				  was_hlgt = AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
							 was_hdrk = AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO, was_lamp = FALSE;

	int j, skip_total = IS_SET(pos, 0x80), show_msg = IS_SET(pos, 0x40);

	OBJ_DATA *obj;

	REMOVE_BIT(pos, (0x80 | 0x40));

	if ((pos < 0 || pos >= NUM_WEARS) || (obj = GET_EQ(ch, pos)) == NULL)
	{
		log("SYSERR: unequip_char(%s,%d) - unused pos or no equip...", GET_NAME(ch), pos);
		return (NULL);
	}

//	if (GET_EQ(ch, WEAR_LIGHT) &&
//	    GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))
//		was_lamp = TRUE;
	//Polud светить должно не только то что надето для освещения, а любой источник света
	was_lamp = is_wear_light(ch);
	//-Polud


	if (ch->in_room == NOWHERE)
		log("SYSERR: ch->in_room = NOWHERE when unequipping char %s.", GET_NAME(ch));

	id_to_set_info_map::iterator it = obj_data::set_table.begin();

	if (OBJ_FLAGGED(obj, ITEM_SETSTUFF))
		for (; it != obj_data::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				deactivate_stuff(ch, obj, it, 0 | (show_msg ? 0x40 : 0), 0);
				break;
			}

	if (!OBJ_FLAGGED(obj, ITEM_SETSTUFF) || it == obj_data::set_table.end())
	{
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, 0, FALSE);

		if (IN_ROOM(ch) != NOWHERE)
			for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
			{
				if (weapon_affect[j].aff_bitvector == 0 || !IS_OBJ_AFF(obj, weapon_affect[j].aff_pos))
					continue;
				if (IS_NPC(ch) && AFF_FLAGGED((&mob_proto[GET_MOB_RNUM(ch)]), weapon_affect[j].aff_bitvector))
					continue;
				affect_modify(ch, APPLY_NONE, 0, weapon_affect[j].aff_bitvector, FALSE);
			}

		if (OBJ_FLAGGED(obj, ITEM_SETSTUFF))
			obj->deactivate_obj(activation());
	}

	GET_EQ(ch, pos) = NULL;
	obj->worn_by = NULL;
	obj->worn_on = NOWHERE;
	obj->next_content = NULL;

	if (!skip_total)
	{
		if (obj_sets::is_set_item(obj))
		{
			if (obj->get_activator().first)
			{
				obj_sets::print_off_msg(ch, obj);
			}
			ch->obj_bonus().update(ch);
		}
		obj->set_activator(false, 0);
		obj->enchants.remove_set_bonus(obj);

		affect_total(ch);
		check_light(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	return (obj);
}

int get_number(char **name)
{
	int i, res;
	char *ppos;
	char tmpname[MAX_INPUT_LENGTH];

	if ((ppos = strchr(*name, '.')) != NULL)
	{
		for (i = 0; *name + i != ppos; i++)
			if (!isdigit(*(*name + i)))
				return (1);
		*ppos = '\0';
		res = atoi(*name);
		strl_cpy(tmpname, ppos + 1, MAX_INPUT_LENGTH);
		strl_cpy(*name, tmpname, MAX_INPUT_LENGTH);
		return (res);
	}
	return (1);
}

int get_number(std::string &name)
{
	std::string::size_type pos = name.find('.');

	if (pos != std::string::npos)
	{
		for (std::string::size_type i = 0; i != pos; i++)
			if (!isdigit(name[i]))
				return (1);
		int res = atoi(name.substr(0, pos).c_str());
		name.erase(0, pos + 1);
		return (res);
	}
	return (1);
}



// Search a given list for an object number, and return a ptr to that obj //
OBJ_DATA *get_obj_in_list_num(int num, OBJ_DATA * list)
{
	OBJ_DATA *i;

	for (i = list; i; i = i->next_content)
		if (GET_OBJ_RNUM(i) == num)
			return (i);

	return (NULL);
}

// Search a given list for an object virtul_number, and return a ptr to that obj //
OBJ_DATA *get_obj_in_list_vnum(int num, OBJ_DATA * list)
{
	OBJ_DATA *i;

	for (i = list; i; i = i->next_content)
		if (GET_OBJ_VNUM(i) == num)
			return (i);

	return (NULL);
}


// search the entire world for an object number, and return a pointer  //
OBJ_DATA *get_obj_num(obj_rnum nr)
{
	OBJ_DATA *i;

	for (i = object_list; i; i = i->next)
		if (GET_OBJ_RNUM(i) == nr)
			return (i);

	return (NULL);
}



// search a room for a char, and return a pointer if found..  //
CHAR_DATA *get_char_room(char *name, room_rnum room)
{
	CHAR_DATA *i;
	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (NULL);

	for (i = world[room]->people; i && (j <= number); i = i->next_in_room)
		if (isname(tmp, i->get_pc_name()))
			if (++j == number)
				return (i);

	return (NULL);
}



// search all over the world for a char num, and return a pointer if found //
CHAR_DATA *get_char_num(mob_rnum nr)
{
	CHAR_DATA *i;

	for (i = character_list; i; i = i->next)
		if (GET_MOB_RNUM(i) == nr)
			return (i);

	return (NULL);
}


const int money_destroy_timer = 60;
const int death_destroy_timer = 5;
const int room_destroy_timer = 10;
const int room_nodestroy_timer = -1;
const int script_destroy_timer = 1; // * !!! Never set less than ONE * //

/**
* put an object in a room
* Ахтунг, не надо тут экстрактить шмотку, если очень хочется - проверяйте и правьте 50 вызовов
* по коду, т.к. нигде оно нифига не проверяется на валидность после этой функции.
* \return 0 - невалидный объект или комната, 1 - все ок
*/
bool obj_to_room(OBJ_DATA * object, room_rnum room)
{
//	int sect = 0;
	if (!object || room < FIRST_ROOM || room > top_of_world)
	{
		log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
			room, top_of_world, object);
		return 0;
	}
	else
	{
		restore_object(object, 0);
		insert_obj_and_group(object, &world[room]->contents);
		object->in_room = room;
		object->carried_by = NULL;
		object->worn_by = NULL;
		if (ROOM_FLAGGED(room, ROOM_NOITEM))
			SET_BIT(GET_OBJ_EXTRA(object, ITEM_DECAY), ITEM_DECAY);
//		sect = real_sector(room);
//      if (ROOM_FLAGGED(room, ROOM_HOUSE))
//         SET_BIT(ROOM_FLAGS(room, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
//      if (object->proto_script || object->script)
		if (object->script)
			GET_OBJ_DESTROY(object) = script_destroy_timer;
		else if (OBJ_FLAGGED(object, ITEM_NODECAY))
			GET_OBJ_DESTROY(object) = room_nodestroy_timer;
		else
			/*  Убрано упасть и утонуть, посколько уничтожение обьектов
			   при лоаде - крешбаг
			   if ((
			   (sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM) &&
			   !IS_CORPSE(object) &&
			   !OBJ_FLAGGED(object, ITEM_SWIMMING)
			   ) ||
			   ((sect == SECT_FLYING ) &&
			   !IS_CORPSE(object) &&
			   !OBJ_FLAGGED(object, ITEM_FLYING)
			   )
			   )
			   {extract_obj(object);
			   }
			   else
			   if (OBJ_FLAGGED(object, ITEM_DECAY) ||
			   (OBJ_FLAGGED(object, ITEM_ZONEDECAY) &&
			   GET_OBJ_ZONE(object) != NOWHERE &&
			   GET_OBJ_ZONE(object) != world[room]->zone
			   )
			   )
			   {act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер", FALSE,
			   world[room]->people, object, 0, TO_ROOM);
			   act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер", FALSE,
			   world[room]->people, object, 0, TO_CHAR);
			   extract_obj(object);
			   }
			   else */
			if (GET_OBJ_TYPE(object) == ITEM_MONEY)
				GET_OBJ_DESTROY(object) = money_destroy_timer;
			else if (ROOM_FLAGGED(room, ROOM_DEATH))
				GET_OBJ_DESTROY(object) = death_destroy_timer;
			else
				GET_OBJ_DESTROY(object) = room_destroy_timer;
	}
	return 1;
}

/* Функция для удаления обьектов после лоада в комнату
   результат работы - 1 если посыпался, 0 - если остался */
int obj_decay(OBJ_DATA * object)
{
	int room, sect;
	room = object->in_room;

	if (room == NOWHERE)
		return (0);

	sect = real_sector(room);

	if (((sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM) &&
			!OBJ_FLAGGED(object, ITEM_SWIMMING) &&
			!OBJ_FLAGGED(object, ITEM_FLYING) &&
			!IS_CORPSE(object)))
	{

		act("$o0 медленно утонул$G.", FALSE, world[room]->people, object, 0, TO_ROOM);
		act("$o0 медленно утонул$G.", FALSE, world[room]->people, object, 0, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	if (((sect == SECT_FLYING) && !IS_CORPSE(object) && !OBJ_FLAGGED(object, ITEM_FLYING)))
	{

		act("$o0 упал$G вниз.", FALSE, world[room]->people, object, 0, TO_ROOM);
		act("$o0 упал$G вниз.", FALSE, world[room]->people, object, 0, TO_CHAR);
		extract_obj(object);
		return (1);
	}


	if (OBJ_FLAGGED(object, ITEM_DECAY) ||
			(OBJ_FLAGGED(object, ITEM_ZONEDECAY) &&
			 GET_OBJ_ZONE(object) != NOWHERE && GET_OBJ_ZONE(object) != world[room]->zone))
	{

		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", FALSE,
			world[room]->people, object, 0, TO_ROOM);
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", FALSE,
			world[room]->people, object, 0, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	return (0);
}

// Take an object from a room
void obj_from_room(OBJ_DATA * object)
{
	OBJ_DATA *temp;

	if (!object || object->in_room == NOWHERE)
	{
		log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
			object, object->in_room);
		return;
	}

	REMOVE_FROM_LIST(object, world[object->in_room]->contents, next_content);

//  if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
//     SET_BIT(ROOM_FLAGS(object->in_room, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
	object->in_room = NOWHERE;
	object->next_content = NULL;
}


// put an object in an object (quaint)
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to)
{
	OBJ_DATA *tmp_obj;

	if (!obj || !obj_to || obj == obj_to)
	{
		log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
			obj, obj, obj_to);
		return;
	}

	insert_obj_and_group(obj, &obj_to->contains);
	obj->in_obj = obj_to;

	for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
		GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

	// top level object.  Subtract weight from inventory if necessary.
	GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
	if (tmp_obj->carried_by)
		IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
}


// remove an object from an object
void obj_from_obj(OBJ_DATA * obj)
{
	OBJ_DATA *temp, *obj_from;

	if (obj->in_obj == NULL)
	{
		log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
		return;
	}
	obj_from = obj->in_obj;
	REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

	// Subtract weight from containers container
	for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
		GET_OBJ_WEIGHT(temp) = MAX(1, GET_OBJ_WEIGHT(temp) - GET_OBJ_WEIGHT(obj));

	// Subtract weight from char that carries the object
	GET_OBJ_WEIGHT(temp) = MAX(1, GET_OBJ_WEIGHT(temp) - GET_OBJ_WEIGHT(obj));
	if (temp->carried_by)
		IS_CARRYING_W(temp->carried_by) = MAX(1, IS_CARRYING_W(temp->carried_by) - GET_OBJ_WEIGHT(obj));

	obj->in_obj = NULL;
	obj->next_content = NULL;
}


// Set all carried_by to point to new owner
void object_list_new_owner(OBJ_DATA * list, CHAR_DATA * ch)
{
	if (list)
	{
		object_list_new_owner(list->contains, ch);
		object_list_new_owner(list->next_content, ch);
		list->carried_by = ch;
	}
}

// Extract an object from the world
void extract_obj(OBJ_DATA * obj)
{
	char name[MAX_STRING_LENGTH];
	OBJ_DATA *temp;

	strcpy(name, obj->PNames[0]);
	log("Extracting obj %s", name);
// TODO: в дебаг log("Start extract obj %s", name);

	// Get rid of the contents of the object, as well.
	// Обработка содержимого контейнера при его уничтожении
	while (obj->contains)
	{
		temp = obj->contains;
		obj_from_obj(temp);

		if (obj->carried_by)
		{
			if (IS_NPC(obj->carried_by)
					|| (IS_CARRYING_N(obj->carried_by) >= CAN_CARRY_N(obj->carried_by)))
			{
				obj_to_room(temp, IN_ROOM(obj->carried_by));
				obj_decay(temp);
			}
			else
			{
				obj_to_char(temp, obj->carried_by);
			}
		}
		else if (obj->worn_by != NULL)
		{
			if (IS_NPC(obj->worn_by)
					|| (IS_CARRYING_N(obj->worn_by) >= CAN_CARRY_N(obj->worn_by)))
			{
				obj_to_room(temp, IN_ROOM(obj->worn_by));
				obj_decay(temp);
			}
			else
			{
				obj_to_char(temp, obj->worn_by);
			}
		}
		else if (obj->in_room != NOWHERE)
		{
			obj_to_room(temp, obj->in_room);
			obj_decay(temp);
		}
		else if (obj->in_obj)
		{
			extract_obj(temp);
		}
		else
			extract_obj(temp);
	}
	// Содержимое контейнера удалено

	if (obj->worn_by != NULL)
		if (unequip_char(obj->worn_by, obj->worn_on) != obj)
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
	if (obj->in_room != NOWHERE)
		obj_from_room(obj);
	else if (obj->carried_by)
		obj_from_char(obj);
	else if (obj->in_obj)
		obj_from_obj(obj);

	check_auction(NULL, obj);
	check_exchange(obj);
	REMOVE_FROM_LIST(obj, object_list, next);
//	ObjectAlias::remove(obj);

	if (GET_OBJ_RNUM(obj) >= 0)
		(obj_index[GET_OBJ_RNUM(obj)].number)--;

	free_script(SCRIPT(obj));	// без комментариев

	free_obj(obj);
// TODO: в дебаг log("Stop extract obj %s", name);
}



void update_object(OBJ_DATA * obj, int use)
{
	// dont update objects with a timer trigger
	if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && obj->get_timer() > 0 && OBJ_FLAGGED(obj, ITEM_TICKTIMER))
	{
		obj->dec_timer(use);
	}
	if (obj->contains)
		update_object(obj->contains, use);
	if (obj->next_content)
		update_object(obj->next_content, use);
}


void update_char_objects(CHAR_DATA * ch)
{
	int i;
//Polud раз уж светит любой источник света, то и гаснуть тоже должны все
	for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++)
		if (GET_EQ(ch, wear_pos) != NULL)
		{
			if (GET_OBJ_TYPE(GET_EQ(ch, wear_pos)) == ITEM_LIGHT)
			{
				if (GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2) > 0)
				{
					i = --GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2);
					if (i == 1)
					{
						act("$z $o замерцал$G и начал$G угасать.\r\n",
							FALSE, ch, GET_EQ(ch, wear_pos), 0, TO_CHAR);
						act("$o $n1 замерцал$G и начал$G угасать.",
							FALSE, ch, GET_EQ(ch, wear_pos), 0, TO_ROOM);
					}
					else if (i == 0)
					{
						act("$z $o погас$Q.\r\n", FALSE, ch, GET_EQ(ch, wear_pos), 0, TO_CHAR);
						act("$o $n1 погас$Q.", FALSE, ch, GET_EQ(ch, wear_pos), 0, TO_ROOM);
						if (IN_ROOM(ch) != NOWHERE)
						{
							if (world[IN_ROOM(ch)]->light > 0)
								world[IN_ROOM(ch)]->light -= 1;
						}
						if (OBJ_FLAGGED(GET_EQ(ch, wear_pos), ITEM_DECAY))
							extract_obj(GET_EQ(ch, wear_pos));
					}
				}
			}
		}
	//-Polud

	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
			update_object(GET_EQ(ch, i), 1);

	if (ch->carrying)
		update_object(ch->carrying, 1);
}

/**
* Если на мобе шмотки, одетые во время резета зоны, то при резете в случае пуржа моба - они уничтожаются с ним же.
* Если на мобе шмотки, поднятые и бывшие у игрока (таймер уже тикал), то он их при резете выкинет на землю, как обычно.
* А то при резетах например той же мавки умудрялись лутить шмот с земли, упавший с нее до того, как она сама поднимет,
* плюс этот лоад накапливался и можно было заиметь несколько шмоток сразу с нескольких резетов. -- Krodo
* \param inv - 1 сообщение о выкидывании из инвентаря, 0 - о снятии с себя
* \param zone_reset - 1 - пуржим стаф без включенных таймеров, 0 - не пуржим ничего
*/
void drop_obj_on_zreset(CHAR_DATA *ch, OBJ_DATA *obj, bool inv, bool zone_reset)
{
	if (zone_reset && !OBJ_FLAGGED(obj, ITEM_TICKTIMER))
		extract_obj(obj);
	else
	{
		if (inv)
			act("Вы выбросили $o3 на землю.", FALSE, ch, obj, 0, TO_CHAR);
		else
			act("Вы сняли $o3 и выбросили на землю.", FALSE, ch, obj, 0, TO_CHAR);
		// Если этот моб трупа не оставит, то не выводить сообщение
		// иначе ужасно коряво смотрится в бою и в тригах
		bool msgShown = false;
		if (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_CORPSE))
		{
			if (inv)
				act("$n бросил$g $o3 на землю.", FALSE, ch, obj, 0, TO_ROOM);
			else
				act("$n снял$g $o3 и бросил$g на землю.", FALSE, ch, obj, 0, TO_ROOM);
			msgShown = true;
		}

		drop_otrigger(obj, ch);
		if (obj->purged()) return;

		drop_wtrigger(obj, ch);
		if (obj->purged()) return;

		obj_to_room(obj, ch->in_room);
		if (!obj_decay(obj) && !msgShown)
		{
			act("На земле остал$U лежать $o.", FALSE, ch, obj, 0, TO_ROOM);
		}
	}
}

namespace
{

void change_npc_leader(CHAR_DATA *ch)
{
	std::vector<CHAR_DATA *> tmp_list;

	for (follow_type *i = ch->followers; i; i = i->next)
	{
		if (IS_NPC(i->follower)
			&& !IS_CHARMICE(i->follower)
			&& i->follower->master == ch)
		{
			tmp_list.push_back(i->follower);
		}
	}
	if (tmp_list.empty())
	{
		return;
	}

	CHAR_DATA *leader = 0;
	for (std::vector<CHAR_DATA *>::const_iterator i = tmp_list.begin(),
		iend = tmp_list.end(); i != iend; ++i)
	{
		if (stop_follower(*i, SF_SILENCE))
		{
			continue;
		}
		if (!leader)
		{
			leader = *i;
		}
		else
		{
			add_follower(*i, leader, true);
		}
	}
}

} // namespace

/**
* Extract a ch completely from the world, and leave his stuff behind
* \param zone_reset - 0 обычный пурж когда угодно (по умолчанию), 1 - пурж при резете зоны
*/
void extract_char(CHAR_DATA * ch, int clear_objs, bool zone_reset)
{
	if (ch->purged())
	{
		log("SYSERROR: double extract_char (%s:%d)", __FILE__, __LINE__);
		return;
	}

	DESCRIPTOR_DATA *t_desc;
	int i, freed = 0;
	CHAR_DATA *ch_w, *temp;

	if (MOB_FLAGGED(ch, MOB_FREE) || MOB_FLAGGED(ch, MOB_DELETE))
		return;

	std::string name = GET_NAME(ch) ? GET_NAME(ch) : "<null>";
	log("[Extract char] Start function for char %s", name.c_str());
	if (!IS_NPC(ch) && !ch->desc)
	{
		log("[Extract char] Extract descriptors");
		for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
			if (t_desc->original == ch)
				do_return(t_desc->character, NULL, 0, 0);
	}

	if (ch->in_room == NOWHERE)
	{
//log("SYSERR: NOWHERE extracting char %s. (%s, extract_char)",GET_NAME(ch), __FILE__);
		return;
		// exit(1);
	}

	// Forget snooping, if applicable
	log("[Extract char] Stop snooping");
	if (ch->desc)
	{
		if (ch->desc->snooping)
		{
			ch->desc->snooping->snoop_by = NULL;
			ch->desc->snooping = NULL;
		}
		if (ch->desc->snoop_by)
		{
			SEND_TO_Q("Ваша жертва теперь недоступна.\r\n", ch->desc->snoop_by);
			ch->desc->snoop_by->snooping = NULL;
			ch->desc->snoop_by = NULL;
		}
	}

	// transfer equipment to room, if any
	log("[Extract char] Drop equipment");
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			OBJ_DATA *obj_eq = unequip_char(ch, i);
			if (!obj_eq) continue;

			remove_otrigger(obj_eq, ch);
			if (obj_eq->purged()) continue;

			drop_obj_on_zreset(ch, obj_eq, 0, zone_reset);
		}
	}

	// transfer objects to room, if any
	log("[Extract char] Drop objects");
	while (ch->carrying)
	{
		OBJ_DATA *obj = ch->carrying;
		obj_from_char(obj);
		drop_obj_on_zreset(ch, obj, 1, zone_reset);
	}

	if(IS_NPC(ch))
	{
		// дроп гривен до изменений последователей за мобом
		ExtMoney::drop_torc(ch);
	}

	if (!IS_NPC(ch) && !ch->master && ch->followers && AFF_FLAGGED(ch, AFF_GROUP))
	{
		log("[Extract char] Change group leader");
		change_leader(ch, 0);
	}
	else if (IS_NPC(ch) && !IS_CHARMICE(ch) && !ch->master && ch->followers)
	{
		log("[Extract char] Changing NPC leader");
		change_npc_leader(ch);
	}

	log("[Extract char] Die followers");
	if ((ch->followers || ch->master) && die_follower(ch))
	{
		// TODO: странно все это с пуржем в stop_follower
		// extract_mob тоже самое
		return;
	}

	log("[Extract char] Stop fighting self");
	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	log("[Extract char] Stop all fight for opponee");
	change_fighting(ch, TRUE);


	log("[Extract char] Remove char from room");
	char_from_room(ch);

	delete_from_tmp_char_list(ch);
	ch->clear_fighing_list();

	// pull the char from the list
	SET_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
	REMOVE_FROM_LIST(ch, character_list, next);
//	CharacterAlias::remove(ch);

	if (ch->desc && ch->desc->original)
		do_return(ch, NULL, 0, 0);

    // на плееров сейчас тоже триги вешают
	free_script(SCRIPT(ch));	// без комментариев
	SCRIPT(ch) = NULL;	// т.к. реально char будет удален позже
	// нужно обнулить его script_data, иначе
	// там начнут искать random_triggers
	if (SCRIPT_MEM(ch))
	{
		extract_script_mem(SCRIPT_MEM(ch));
		SCRIPT_MEM(ch) = NULL;	// Аналогично предыдущему комментарию
	}

	if (!IS_NPC(ch))
	{
		log("[Extract char] All save for PC");
		check_auction(ch, NULL);
		ch->save_char();
		//удаляются рент-файлы, если только персонаж не ушел в ренту
		Crash_delete_crashfile(ch);
//      if (clear_objs)
//         Crash_delete_files(GET_NAME(ch), CRASH_DELETE_OLD | CRASH_DELETE_NEW);
	}
	else
	{
		log("[Extract char] All clear for NPC");
		if ((GET_MOB_RNUM(ch) > -1) && (!MOB_FLAGGED(ch, MOB_RESURRECTED)))	// if mobile и не умертвие
			mob_index[GET_MOB_RNUM(ch)].number--;
		clearMemory(ch);	// Only NPC's can have memory

		SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
		ch->purge();
		// delete ch;
		freed = 1;
	}

	if (!freed && ch->desc != NULL)
	{
		STATE(ch->desc) = CON_MENU;
		SEND_TO_Q(MENU, ch->desc);
		if (!IS_NPC(ch) && RENTABLE(ch) && clear_objs)
		{
			ch_w = ch->next;
			do_entergame(ch->desc);
			ch->next = ch_w;
		}

	}
	else  		// if a player gets purged from within the game
	{
		if (!freed)
		{
			SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
			ch->purge();
			// delete ch;
		}
	}
	log("[Extract char] Stop function for char %s", name.c_str());
}


// Extract a MOB completely from the world, and destroy his stuff
void extract_mob(CHAR_DATA * ch)
{
	if (ch->purged())
	{
		log("SYSERROR: double extract_mob (%s:%d)", __FILE__, __LINE__);
		return;
	}

	int i;
	CHAR_DATA *temp;

	if (MOB_FLAGGED(ch, MOB_FREE) || MOB_FLAGGED(ch, MOB_DELETE))
		return;

	if (ch->in_room == NOWHERE)
	{
		log("SYSERR: NOWHERE extracting char %s. (%s, extract_mob)", GET_NAME(ch), __FILE__);
		return;
		exit(1);
	}

	if ((ch->followers || ch->master) && die_follower(ch))
	{
		// TODO: странно все это с пуржем в stop_follower
		// extract_char тоже самое
		return;
	}

	// Forget snooping, if applicable
	if (ch->desc)
	{
		if (ch->desc->snooping)
		{
			ch->desc->snooping->snoop_by = NULL;
			ch->desc->snooping = NULL;
		}
		if (ch->desc->snoop_by)
		{
			SEND_TO_Q("Ваша жертва теперь недоступна.\r\n", ch->desc->snoop_by);
			ch->desc->snoop_by->snooping = NULL;
			ch->desc->snoop_by = NULL;
		}
	}

	// extract objects, if any
	while (ch->carrying)
		extract_obj(ch->carrying);

	// transfer equipment to room, if any
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
			extract_obj(GET_EQ(ch, i));

	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	log("[Extract mob] Stop all fight for opponee");
	change_fighting(ch, TRUE);

	char_from_room(ch);

	delete_from_tmp_char_list(ch);
	ch->clear_fighing_list();

	// pull the char from the list
	SET_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
	REMOVE_FROM_LIST(ch, character_list, next);
//	CharacterAlias::remove(ch);

	if (ch->desc && ch->desc->original)
		do_return(ch, NULL, 0, 0);

	if (GET_MOB_RNUM(ch) > -1)
		mob_index[GET_MOB_RNUM(ch)].number--;
	clearMemory(ch);

	free_script(SCRIPT(ch));	// см. выше
	SCRIPT(ch) = NULL;

	if (SCRIPT_MEM(ch))
	{
		extract_script_mem(SCRIPT_MEM(ch));
		SCRIPT_MEM(ch) = NULL;
	}

	SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
	ch->purge();
//	delete ch;
}




/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */


CHAR_DATA *get_player_vis(CHAR_DATA * ch, const char *name, int inroom)
{
	CHAR_DATA *i;

	for (i = character_list; i; i = i->next)
	{
		//if (IS_NPC(i) || (!(i->desc) && !RENTABLE(i) && !(inroom & FIND_CHAR_DISCONNECTED)))
		//   continue;
		if (IS_NPC(i))
			continue;
		if (!HERE(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		//if (str_cmp(i->get_pc_name(), name))
		//   continue;
		if (!CAN_SEE_CHAR(ch, i))
			continue;
		if (!isname(name, i->get_pc_name()))
			continue;
		return (i);
	}

	return (NULL);
}
CHAR_DATA *get_player_vis(CHAR_DATA * ch, const std::string &name, int inroom)
{
	CHAR_DATA *i;

	for (i = character_list; i; i = i->next)
	{
		//if (IS_NPC(i) || (!(i->desc) && !RENTABLE(i) && !(inroom & FIND_CHAR_DISCONNECTED)))
		//   continue;
		if (IS_NPC(i))
			continue;
		if (!HERE(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		//if (str_cmp(i->get_pc_name(), name))
		//   continue;
		if (!CAN_SEE_CHAR(ch, i))
			continue;
		if (!isname(name, i->get_pc_name()))
			continue;
		return (i);
	}

	return (NULL);
}


CHAR_DATA *get_player_pun(CHAR_DATA * ch, const char *name, int inroom)
{
	CHAR_DATA *i;

	for (i = character_list; i; i = i->next)
	{
		if (IS_NPC(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		if (!isname(name, i->get_pc_name()))
			continue;
		return (i);
	}

	return (NULL);
}
CHAR_DATA *get_player_pun(CHAR_DATA * ch, const std::string &name, int inroom)
{
	CHAR_DATA *i;

	for (i = character_list; i; i = i->next)
	{
		if (IS_NPC(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		if (!isname(name, i->get_pc_name()))
			continue;
		return (i);
	}

	return (NULL);
}


CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, const char *name)
{
	CHAR_DATA *i;
	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	// JE 7/18/94 :-) :-)
	if (!str_cmp(name, "self") || !str_cmp(name, "me") ||
			!str_cmp(name, "я") || !str_cmp(name, "меня") || !str_cmp(name, "себя"))
		return (ch);

	// 0.<name> means PC with name
	strl_cpy(tmp, name, MAX_INPUT_LENGTH);
	if (!(number = get_number(&tmp)))
		return (get_player_vis(ch, tmp, FIND_CHAR_ROOM));

	for (i = world[ch->in_room]->people; i && j <= number; i = i->next_in_room)
		if (HERE(i) && CAN_SEE(ch, i) && isname(tmp, i->get_pc_name()))
			if (++j == number)
				return (i);

	return (NULL);
}
CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, const std::string &name)
{
	CHAR_DATA *i;
	int j = 0, number;

	// JE 7/18/94 :-) :-)
	if (!str_cmp(name, "self") || !str_cmp(name, "me") ||
			!str_cmp(name, "я") || !str_cmp(name, "меня") || !str_cmp(name, "себя"))
		return (ch);

	// 0.<name> means PC with name
	std::string tmp(name);
	if (!(number = get_number(tmp)))
		return (get_player_vis(ch, tmp, FIND_CHAR_ROOM));

	for (i = world[ch->in_room]->people; i && j <= number; i = i->next_in_room)
		if (HERE(i) && CAN_SEE(ch, i) && isname(tmp, i->get_pc_name()))
			if (++j == number)
				return (i);

	return (NULL);
}


CHAR_DATA *get_char_vis(CHAR_DATA * ch, const char *name, int where)
{
	CHAR_DATA *i;
	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	// check the room first
	if (where == FIND_CHAR_ROOM)
		return get_char_room_vis(ch, name);
	else if (where == FIND_CHAR_WORLD)
	{
		if ((i = get_char_room_vis(ch, name)) != NULL)
			return (i);

		strcpy(tmp, name);
		if (!(number = get_number(&tmp)))
			return get_player_vis(ch, tmp, 0);

		for (i = character_list; i && (j <= number); i = i->next)
			if (HERE(i) && CAN_SEE(ch, i) && isname(tmp, i->get_pc_name()))
				if (++j == number)
					return (i);
	}

	return (NULL);
}
CHAR_DATA *get_char_vis(CHAR_DATA * ch, const std::string &name, int where)
{
	CHAR_DATA *i;
	int j = 0, number;

	// check the room first
	if (where == FIND_CHAR_ROOM)
		return get_char_room_vis(ch, name);
	else if (where == FIND_CHAR_WORLD)
	{
		if ((i = get_char_room_vis(ch, name)) != NULL)
			return (i);

		std::string tmp(name);
		if (!(number = get_number(tmp)))
			return get_player_vis(ch, tmp, 0);

		for (i = character_list; i && (j <= number); i = i->next)
			if (HERE(i) && CAN_SEE(ch, i) && isname(tmp, i->get_pc_name()))
				if (++j == number)
					return (i);
	}

	return (NULL);
}


OBJ_DATA *get_obj_in_list_vis(CHAR_DATA * ch, const char *name, OBJ_DATA * list)
{
	OBJ_DATA *i;
	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (NULL);

	for (i = list; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->aliases) || CHECK_CUSTOM_LABEL(tmp, i, ch))
			if (CAN_SEE_OBJ(ch, i))
				if (++j == number)  	// sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
				{
					// send_to_char(buf,ch);
					return (i);
				}

	return (NULL);
}
OBJ_DATA *get_obj_in_list_vis(CHAR_DATA * ch, const std::string &name, OBJ_DATA * list)
{
	OBJ_DATA *i;
	int j = 0, number;
	std::string tmp(name);

	if (!(number = get_number(tmp)))
		return (NULL);

	for (i = list; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->aliases) || CHECK_CUSTOM_LABEL(tmp, i, ch))
			if (CAN_SEE_OBJ(ch, i))
				if (++j == number)  	// sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
				{
					// send_to_char(buf,ch);
					return (i);
				}

	return (NULL);
}




// search the entire world for an object, and return a pointer
OBJ_DATA *get_obj_vis(CHAR_DATA * ch, const char *name)
{
	OBJ_DATA *i;
	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	// scan items carried //
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
		return (i);

	// scan room //
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) != NULL)
		return (i);

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (NULL);

	// ok.. no luck yet. scan the entire obj list   //
	for (i = object_list; i && (j <= number); i = i->next)
		if (isname(tmp, i->aliases) || CHECK_CUSTOM_LABEL(tmp, i, ch))
			if (CAN_SEE_OBJ(ch, i))
				if (++j == number)
					return (i);

	return (NULL);
}
// search the entire world for an object, and return a pointer  //
OBJ_DATA *get_obj_vis(CHAR_DATA * ch, const std::string &name)
{
	OBJ_DATA *i;
	int j = 0, number;

	// scan items carried //
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
		return (i);

	// scan room //
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) != NULL)
		return (i);

	std::string tmp(name);
	if (!(number = get_number(tmp)))
		return (NULL);

	// ok.. no luck yet. scan the entire obj list   //
	for (i = object_list; i && (j <= number); i = i->next)
		if (isname(tmp, i->aliases) || CHECK_CUSTOM_LABEL(tmp, i, ch))
			if (CAN_SEE_OBJ(ch, i))
				if (++j == number)
					return (i);

	return (NULL);
}



OBJ_DATA *get_object_in_equip_vis(CHAR_DATA * ch, const char *arg, OBJ_DATA * equipment[], int *j)
{
	int l, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, arg);
	if (!(number = get_number(&tmp)))
		return (NULL);

	for ((*j) = 0, l = 0; (*j) < NUM_WEARS; (*j)++)
		if (equipment[(*j)])
			if (CAN_SEE_OBJ(ch, equipment[(*j)]))
				if (isname(tmp, equipment[(*j)]->aliases) || CHECK_CUSTOM_LABEL(tmp, equipment[(*j)], ch))
					if (++l == number)
						return (equipment[(*j)]);

	return (NULL);
}
OBJ_DATA *get_object_in_equip_vis(CHAR_DATA * ch, const std::string &arg, OBJ_DATA * equipment[], int *j)
{
	int l, number;
	std::string tmp(arg);

	if (!(number = get_number(tmp)))
		return (NULL);

	for ((*j) = 0, l = 0; (*j) < NUM_WEARS; (*j)++)
		if (equipment[(*j)])
			if (CAN_SEE_OBJ(ch, equipment[(*j)]))
				if (isname(tmp, equipment[(*j)]->aliases) || CHECK_CUSTOM_LABEL(tmp, equipment[(*j)], ch))
					if (++l == number)
						return (equipment[(*j)]);

	return (NULL);
}

/*
OBJ_DATA *get_obj_in_eq_vis(CHAR_DATA * ch, const char *arg)
{
	int l, number, j;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, arg);
	if (!(number = get_number(&tmp)))
		return (NULL);

	for (j = 0, l = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
				if (isname(tmp, GET_EQ(ch, j)->aliases))
					if (++l == number)
						return (GET_EQ(ch, j));

	return (NULL);
}
OBJ_DATA *get_obj_in_eq_vis(CHAR_DATA * ch, const std::string &arg)
{
	int l, number, j;
	std::string tmp(arg);

	if (!(number = get_number(tmp)))
		return (NULL);

	for (j = 0, l = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
				if (isname(tmp, GET_EQ(ch, j)->aliases))
					if (++l == number)
						return (GET_EQ(ch, j));

	return (NULL);
}
*/

char *money_desc(int amount, int padis)
{
	static char buf[128];
	const char *single[6][2] = { {"а", "а"},
		{"ой", "ы"},
		{"ой", "е"},
		{"у", "у"},
		{"ой", "ой"},
		{"ой", "е"}
	}, *plural[6][3] =
	{
		{
			"ая", "а", "а"}, {
			"ой", "и", "ы"}, {
			"ой", "е", "е"}, {
			"ую", "у", "у"}, {
			"ой", "ой", "ой"}, {
			"ой", "е", "е"}
	};

	if (amount <= 0)
	{
		log("SYSERR: Try to create negative or 0 money (%d).", amount);
		return (NULL);
	}
	if (amount == 1)
	{
		sprintf(buf, "одн%s кун%s", single[padis][0], single[padis][1]);
	}
	else if (amount <= 10)
		sprintf(buf, "малюсеньк%s горстк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 20)
		sprintf(buf, "маленьк%s горстк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 75)
		sprintf(buf, "небольш%s горстк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 200)
		sprintf(buf, "маленьк%s кучк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 1000)
		sprintf(buf, "небольш%s кучк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 5000)
		sprintf(buf, "кучк%s кун", plural[padis][1]);
	else if (amount <= 10000)
		sprintf(buf, "больш%s кучк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 20000)
		sprintf(buf, "груд%s кун", plural[padis][2]);
	else if (amount <= 75000)
		sprintf(buf, "больш%s груд%s кун", plural[padis][0], plural[padis][2]);
	else if (amount <= 150000)
		sprintf(buf, "горк%s кун", plural[padis][1]);
	else if (amount <= 250000)
		sprintf(buf, "гор%s кун", plural[padis][2]);
	else
		sprintf(buf, "огромн%s гор%s кун", plural[padis][0], plural[padis][2]);

	return (buf);
}


OBJ_DATA *create_money(int amount)
{
	int i;
	OBJ_DATA *obj;
	EXTRA_DESCR_DATA *new_descr;
	char buf[200];

	if (amount <= 0)
	{
		log("SYSERR: Try to create negative or 0 money. (%d)", amount);
		return (NULL);
	}
	obj = create_obj();
	CREATE(new_descr, EXTRA_DESCR_DATA, 1);

	if (amount == 1)
	{
		sprintf(buf, "coin gold кун деньги денег монет %s", money_desc(amount, 0));
		obj->aliases = str_dup(buf);
		obj->short_description = str_dup("куна");
		obj->description = str_dup("Одна куна лежит здесь.");
		new_descr->keyword = str_dup("coin gold монет кун денег");
		new_descr->description = str_dup("Всего лишь одна куна.");
		for (i = 0; i < NUM_PADS; i++)
			obj->PNames[i] = str_dup(money_desc(amount, i));
	}
	else
	{
		sprintf(buf, "coins gold кун денег %s", money_desc(amount, 0));
		obj->aliases = str_dup(buf);
		obj->short_description = str_dup(money_desc(amount, 0));
		for (i = 0; i < NUM_PADS; i++)
			obj->PNames[i] = str_dup(money_desc(amount, i));

		sprintf(buf, "Здесь лежит %s.", money_desc(amount, 0));
		obj->description = str_dup(CAP(buf));

		new_descr->keyword = str_dup("coins gold кун денег");
	}

	new_descr->next = NULL;
	obj->ex_description = new_descr;

	GET_OBJ_TYPE(obj) = ITEM_MONEY;
	GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
	GET_OBJ_SEX(obj) = SEX_FEMALE;
	GET_OBJ_VAL(obj, 0) = amount;
	obj->set_cost(amount);
	GET_OBJ_MAX(obj) = 100;
	GET_OBJ_CUR(obj) = 100;
	obj->set_timer(24 * 60 * 7);
	GET_OBJ_WEIGHT(obj) = 1;
	SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODONATE), ITEM_NODONATE);
	SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NOSELL), ITEM_NOSELL);

	return (obj);
}

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, bitvector_t bitvector, CHAR_DATA * ch, CHAR_DATA ** tar_ch, OBJ_DATA ** tar_obj)
{
	char name[256];

	*tar_ch = NULL;
	*tar_obj = NULL;

	OBJ_DATA *i;
	int l, number, j = 0;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	one_argument(arg, name);

	if (!*name)
		return (0);

	if (IS_SET(bitvector, FIND_CHAR_ROOM))  	// Find person in room
	{
		if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_ROOM)) != NULL)
			return (FIND_CHAR_ROOM);
	}
	if (IS_SET(bitvector, FIND_CHAR_WORLD))
	{
		if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD)) != NULL)
			return (FIND_CHAR_WORLD);
	}
	if (IS_SET(bitvector, FIND_OBJ_WORLD))
	{
		if ((*tar_obj = get_obj_vis(ch, name)))
			return (FIND_OBJ_WORLD);
	}

	// Начало изменений. (с) Дмитрий ака dzMUDiST ака Кудояр

// Переписан код, обрабатывающий параметры FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM
// В итоге поиск объекта просиходит в "экипировке - инветаре - комнате" согласно
// общему количеству имеющихся "созвучных" предметов.
// Старый код закомментирован и подан в конце изменений.

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return 0;

	if (IS_SET(bitvector, FIND_OBJ_EQUIP))
	{
		for (l = 0; l < NUM_WEARS; l++)
		{
			if (GET_EQ(ch, l) && CAN_SEE_OBJ(ch, GET_EQ(ch, l)))
			{
				if (isname(tmp, GET_EQ(ch, l)->aliases) || CHECK_CUSTOM_LABEL(tmp, GET_EQ(ch, l), ch)
					|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
						&& find_exdesc(tmp, GET_EQ(ch, l)->ex_description)))
				{
					if (++j == number)
					{
						*tar_obj = GET_EQ(ch, l);
						return (FIND_OBJ_EQUIP);
					}
				}
			}
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_INV))
	{
		for (i = ch->carrying; i && (j <= number); i = i->next_content)
		{
			if (isname(tmp, i->aliases) || CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
					&& find_exdesc(tmp, i->ex_description)))
			{
				if (CAN_SEE_OBJ(ch, i))
				{
					if (++j == number)
					{
						*tar_obj = i;
						return (FIND_OBJ_INV);
					}
				}
			}
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_ROOM))
	{
		for (i = world[ch->in_room]->contents;
			i && (j <= number); i = i->next_content)
		{
			if (isname(tmp, i->aliases) || CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
					&& find_exdesc(tmp ,i->ex_description)))
			{
				if (CAN_SEE_OBJ(ch, i))
				{
					if (++j == number)
					{
						*tar_obj = i;
						return (FIND_OBJ_ROOM);
					}
				}
			}
		}
	}

//  if (IS_SET (bitvector, FIND_OBJ_EQUIP))
//    {
//      if ((*tar_obj = get_obj_in_eq_vis (ch, name)) != NULL)
//      return (FIND_OBJ_EQUIP);
//    }
//  if (IS_SET (bitvector, FIND_OBJ_INV))
//    {
//      if ((*tar_obj = get_obj_in_list_vis (ch, name, ch->carrying)) != NULL)
//      return (FIND_OBJ_INV);
//    }
//  if (IS_SET (bitvector, FIND_OBJ_ROOM))
//    {
//      if ((*tar_obj =
//         get_obj_in_list_vis (ch, name,
//                              world[ch->in_room]->contents)) != NULL)
//      return (FIND_OBJ_ROOM);
//    }

	// Конец изменений. (с) Дмитрий ака dzMUDiST ака Кудояр

	return (0);
}

// a function to scan for "all" or "all.x"
int find_all_dots(char *arg)
{
	char tmpname[MAX_INPUT_LENGTH];

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
		return (FIND_ALL);
	else if (!strn_cmp(arg, "all.", 4) || !strn_cmp(arg, "все.", 4))
	{
		strl_cpy(tmpname, arg + 4, MAX_INPUT_LENGTH);
		strl_cpy(arg, tmpname, MAX_INPUT_LENGTH);
		return (FIND_ALLDOT);
	}
	else
		return (FIND_INDIV);
}

// Функции для работы с порталами для "townportal"
// Возвращает указатель на слово по vnum комнаты или NULL если не найдено
char *find_portal_by_vnum(int vnum)
{
	struct portals_list_type *i;
	for (i = portals_list; i; i = i->next_portal)
	{
		if (i->vnum == vnum)
			return (i->wrd);
	}
	return (NULL);
}

// Возвращает минимальный уровень для изучения портала
int level_portal_by_vnum(int vnum)
{
	struct portals_list_type *i;
	for (i = portals_list; i; i = i->next_portal)
	{
		if (i->vnum == vnum)
			return (i->level);
	}
	return (0);
}

// Возвращает vnum портала по ключевому слову или NOWHERE если не найдено
int find_portal_by_word(char *wrd)
{
	struct portals_list_type *i;
	for (i = portals_list; i; i = i->next_portal)
	{
		if (!str_cmp(i->wrd, wrd))
			return (i->vnum);
	}
	return (NOWHERE);
}

struct portals_list_type *get_portal(int vnum, char *wrd)
{
	struct portals_list_type *i;
	for (i = portals_list; i; i = i->next_portal)
	{
		if ((!wrd || !str_cmp(i->wrd, wrd)) && ((vnum == -1) || (i->vnum == vnum)))
			break;
	}
	return i;
}

/* Добавляет в список чару портал в комнату vnum - с проверкой целостности
   и если есть уже такой - то добавляем его 1м в список */
void add_portal_to_char(CHAR_DATA * ch, int vnum)
{
	struct char_portal_type *tmp, *dlt = NULL;

	// Проверка на то что уже есть портал в списке, если есть, удаляем
	for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next)
	{
		if (tmp->vnum == vnum)
		{
			if (dlt)
			{
				dlt->next = tmp->next;
			}
			else
			{
				GET_PORTALS(ch) = tmp->next;
			}
			free(tmp);
			break;
		}
		dlt = tmp;
	}

	CREATE(tmp, struct char_portal_type, 1);
	tmp->vnum = vnum;
	tmp->next = GET_PORTALS(ch);
	GET_PORTALS(ch) = tmp;
}

// Проверка на то, знает ли чар портал в комнате vnum
int has_char_portal(CHAR_DATA * ch, int vnum)
{
	struct char_portal_type *tmp;

	for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next)
	{
		if (tmp->vnum == vnum)
			return (1);
	}
	return (0);
}

// Убирает лишние и несуществующие порталы у чара
void check_portals(CHAR_DATA * ch)
{
	int max_p, portals;
	struct char_portal_type *tmp, *dlt = NULL;
	struct portals_list_type *port;

	// Вычисляем максимальное количество порталов, которое может запомнить чар
	max_p = MAX_PORTALS(ch);
	portals = 0;

	// Пробегаем max_p порталы
	for (tmp = GET_PORTALS(ch); tmp;)
	{
		port = get_portal(tmp->vnum, NULL);
		if (!port || (portals >= max_p) || (MAX(1, port->level - GET_REMORT(ch) / 2) > GET_LEVEL(ch)))
		{
			if (dlt)
			{
				dlt->next = tmp->next;
			}
			else
			{
				GET_PORTALS(ch) = tmp->next;
			}
			free(tmp);
			if (dlt)
			{
				tmp = dlt->next;
			}
			else
			{
				tmp = GET_PORTALS(ch);
			}
		}
		else
		{
			dlt = tmp;
			portals++;
			tmp = tmp->next;
		}
	}
}

//-----------------------------------------
// Функции для работы с чармисами
// Функция возвращающая общее количество пунктов лояльности у игрока
int charm_points(CHAR_DATA * ch)
{
	int lp;

	if (!can_use_feat(ch, GOLD_TONGUE_FEAT))
		return (0);
	lp = GET_LEVEL(ch) + GET_REAL_CHA(ch) - 16;
	return (lp);
}

/* Функция возвращает количество требуемых пунктов лояльности для того,
   чтобы подчинить моба */
int on_charm_points(CHAR_DATA * ch)
{
	int lp;
	lp = GET_LEVEL(ch);
	return (lp);
}

// Функция возвращающая задействованное количество пунктов лояльности у игрока
int used_charm_points(CHAR_DATA * ch)
{
	int lp;
	struct follow_type *f;

	lp = 0;
	for (f = ch->followers; f; f = f->next)
	{
		if (!AFF_FLAGGED(f->follower, AFF_CHARM))
			continue;
		lp = lp + GET_LEVEL(f->follower);
	}
	return (lp);
}

/* Функция делает нового моба взамен имеющегося если это необходимо
   - для чарма. Возвращает указатель на зачармленого моба или
   NULL если неудалось зачармить */
CHAR_DATA *charm_mob(CHAR_DATA * victim)
{
	int vnum = 0;
	CHAR_DATA *mob;

	vnum = CHARM_MOB_VNUM + GET_LEVEL(victim);

	if (vnum == GET_MOB_VNUM(victim))
		return (victim);

	// Загружаем моба CHARM_MOB_VNUM+уровень victim
	if (!(mob = read_mobile(CHARM_MOB_VNUM + GET_LEVEL(victim), VIRTUAL)))
		return (NULL);
	char_to_room(mob, victim->in_room);
	// Делаем название моба таким же как у чармленого
	GET_PAD(mob, 0) = str_dup(GET_PAD(victim, 0));
	GET_PAD(mob, 1) = str_dup(GET_PAD(victim, 1));
	GET_PAD(mob, 2) = str_dup(GET_PAD(victim, 2));
	GET_PAD(mob, 3) = str_dup(GET_PAD(victim, 3));
	GET_PAD(mob, 4) = str_dup(GET_PAD(victim, 4));
	GET_PAD(mob, 5) = str_dup(GET_PAD(victim, 5));
	mob->set_pc_name(victim->get_pc_name());
	mob->set_npc_name(victim->get_npc_name());
	mob->player_data.long_descr = str_dup(victim->player_data.long_descr);
	mob->player_data.description = str_dup(victim->player_data.description);
	//Убираем моба victim
	extract_mob(victim);
	return (mob);
}

//Функции для модифицированного чарма
float get_damage_per_round(CHAR_DATA * victim)
{
	float dam_per_attack = GET_DR(victim) + str_bonus(victim->get_str(), STR_TO_DAM)
			+ victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2.0
			+ (AFF_FLAGGED(victim, AFF_CLOUD_OF_ARROWS) ? 14 : 0);
	int num_attacks = 1 + victim->mob_specials.ExtraAttack
			+ (victim->get_skill(SKILL_ADDSHOT) ? 2 : 0);

	float dam_per_round = dam_per_attack * num_attacks;

	//Если дыхание - то дамаг умножается на 1.1
 	if (MOB_FLAGGED(victim, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH | MOB_ACIDBREATH | MOB_LIGHTBREATH)))
 	{
 		dam_per_round *= 1.1f;
 	}

 	return dam_per_round;
}

float get_effective_cha(CHAR_DATA * ch, int spellnum)
{
	int key_value, key_value_add, i;

//Для поднять/оживить труп учитываем мудрость, в любом другом случае - обаяние
	if (spellnum == SPELL_RESSURECTION || spellnum == SPELL_ANIMATE_DEAD)
	{
		key_value = ch->get_wis() - 6;
		key_value_add = MIN(56 - ch->get_wis(), GET_WIS_ADD(ch));
		i = 3;
	}
	else
	{
		key_value = ch->get_cha();
		key_value_add = MIN(50 - ch->get_cha(), GET_CHA_ADD(ch));
		i = 5;
	}
	float eff_cha = 0.0;
	if (GET_LEVEL(ch) <= 14)
		eff_cha = MIN(max_stats2[(int) GET_CLASS(ch)][i], key_value)
				  - 6 * (float)(14 - GET_LEVEL(ch)) / 13.0 + key_value_add
				  * (0.2 + 0.3 * (float)(GET_LEVEL(ch) - 1) / 13.0);
	else if (GET_LEVEL(ch) <= 26)
	{
		if (key_value <= 16)
			eff_cha = key_value + key_value_add * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
		else
			eff_cha =
				16 + (float)((key_value - 16) * (GET_LEVEL(ch) - 14)) / 12.0 +
				key_value_add * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
	}
	else
		eff_cha = key_value + key_value_add;
	return eff_cha;
}

float get_effective_int(CHAR_DATA * ch)
{
	float eff_int = 0.0;
	if (GET_LEVEL(ch) <= 14)
		eff_int = MIN(max_stats2[(int) GET_CLASS(ch)][2], ch->get_int())
				  - 6 * (float)(14 - GET_LEVEL(ch)) / 13.0 + GET_INT_ADD(ch)
				  * (0.2 + 0.3 * (float)(GET_LEVEL(ch) - 1) / 13.0);
	else if (GET_LEVEL(ch) <= 26)
	{
		if (ch->get_int() <= 16)
			eff_int = ch->get_int() + GET_INT_ADD(ch) * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
		else
			eff_int =
				16 + (float)((ch->get_int() - 16) * (GET_LEVEL(ch) - 14)) / 12.0 +
				GET_INT_ADD(ch) * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
	}
	else
		eff_int = GET_REAL_INT(ch);
	return eff_int;
}

float calc_cha_for_hire(CHAR_DATA * victim)
{
	int i;
	float reformed_hp = 0.0, needed_cha = 0.0;
	for (i = 0; i < 50; i++)
	{
		reformed_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) * cha_app[i].dam_to_hit_rate;
		if (cha_app[i].charms >= reformed_hp)
			break;
	}
	i = POSI(i);
	needed_cha = i - 1 + (reformed_hp - cha_app[i - 1].charms) / (cha_app[i].charms - cha_app[i - 1].charms);
//sprintf(buf,"check: charms = %d   rhp = %f\r\n",cha_app[i].charms,reformed_hp);
//act(buf,FALSE,victim,0,0,TO_ROOM);
	return VPOSI(needed_cha, 1.0, 50.0);
}


int calc_hire_price(CHAR_DATA * ch, CHAR_DATA * victim)
{
	float needed_cha = calc_cha_for_hire(victim), dpr = 0.0;
	float e_cha = get_effective_cha(ch, 0), e_int = get_effective_int(ch);
	//((e_cha<(1+min_stats2[(int)GET_CLASS(ch)][5]))?MIN(ch->get_cha(),(1+min_stats2[(int)GET_CLASS(ch)][5])):e_cha)-
	//                     1 - min_stats2[(int)GET_CLASS(ch)][5] +
	//((e_int<(1+min_stats2[(int)GET_CLASS(ch)][2]))?MIN(ch->get_int(),(1+min_stats2[(int)GET_CLASS(ch)][2])):e_int)-
	//                     1.0 - min_stats2[(int)GET_CLASS(ch)][2];
	float stat_overlimit = VPOSI(e_cha + e_int - 1.0 -
								 min_stats2[(int) GET_CLASS(ch)][5] - 1 -
								 min_stats2[(int) GET_CLASS(ch)][2], 0, 100);

	if (GET_LEVEL(ch) > 14 && GET_LEVEL(ch) <= 26)
		stat_overlimit =
			VPOSI(stat_overlimit - MIN(GET_REMORT(ch), 16) * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0), 0, 100);
	else if (GET_LEVEL(ch) > 26)
		stat_overlimit = VPOSI(stat_overlimit - MIN(GET_REMORT(ch), 16), 0, 100);

	float price = 0;
	float real_cha = 1.0 + GET_LEVEL(ch) / 2.0 + stat_overlimit / 2.0;
	float difference = needed_cha - real_cha;
	//sprintf(buf,"diff =%f statover=%f ncha=%f rcha=%f pow:%f\r\n",difference,stat_overlimit,needed_cha, real_cha, pow(2.0,difference));
	//send_to_char(buf,ch);
	dpr = get_damage_per_round(victim);

	if (difference <= 0)
		price = dpr * (1.0 - 0.01 * stat_overlimit);
	else
		price = MMIN((dpr * pow(2.0F, difference)), MAXPRICE);

	if (price <= 0.0 || (difference >= 25 && (int) dpr))
		price = MAXPRICE;

	return (int) ceil(price);
}


int get_player_charms(CHAR_DATA * ch, int spellnum)
{
	float r_hp = 0;
	float eff_cha = 0.0;
	eff_cha = get_effective_cha(ch, spellnum);
	if (spellnum != SPELL_CHARM)
		eff_cha = MMIN(48, eff_cha + 2);	// Все кроме чарма кастится с бонусом в 2

	r_hp = (1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].charms +
		   (eff_cha - (int) eff_cha) * cha_app[(int) eff_cha + 1].charms;

	return (int) r_hp;
}


int get_reformed_charmice_hp(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum)
{
	float r_hp = 0;
	float eff_cha = 0.0;
	eff_cha = get_effective_cha(ch, spellnum);
	if (spellnum != SPELL_CHARM)
		eff_cha = MMIN(48, eff_cha + 2);	// Все кроме чарма кастится с бонусом в 2

	// Интерполяция между значениями для целых значений обаяния
	r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
		   ((1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].dam_to_hit_rate +
			(eff_cha - (int) eff_cha) * cha_app[(int) eff_cha + 1].dam_to_hit_rate);

	return (int) r_hp;
}

//********************************************************************
// Работа с очередью мема

void MemQ_init(CHAR_DATA * ch)
{
	ch->MemQueue.stored = 0;
	ch->MemQueue.total = 0;
	ch->MemQueue.queue = NULL;
}

void MemQ_flush(CHAR_DATA * ch)
{
	struct spell_mem_queue_item *i;
	while (ch->MemQueue.queue)
	{
		i = ch->MemQueue.queue;
		ch->MemQueue.queue = i->link;
		free(i);
	}
	MemQ_init(ch);
}

int MemQ_learn(CHAR_DATA * ch)
{
	int num;
	struct spell_mem_queue_item *i;
	if (ch->MemQueue.queue == NULL)
		return 0;
	num = GET_MEM_CURRENT(ch);
	ch->MemQueue.stored -= num;
	ch->MemQueue.total -= num;
	num = ch->MemQueue.queue->spellnum;
	i = ch->MemQueue.queue;
	ch->MemQueue.queue = i->link;
	free(i);
	sprintf(buf, "Вы выучили заклинание \"%s%s%s\".\r\n",
			CCICYN(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	return num;
}

void MemQ_remember(CHAR_DATA * ch, int num)
{
	int *slots;
	int slotcnt, slotn;
	struct spell_mem_queue_item *i, **pi = &ch->MemQueue.queue;

	// проверить количество слотов
	slots = MemQ_slots(ch);
	slotn = spell_info[num].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
	slotcnt = slot_for_char(ch, slotn + 1);
	slotcnt -= slots[slotn];	// кол-во свободных слотов

	if (slotcnt <= 0)
	{
		send_to_char("У вас нет свободных ячеек этого круга.", ch);
		return;
	}

	if (GET_RELIGION(ch) == RELIGION_MONO)
		sprintf(buf, "Вы дописали заклинание \"%s%s%s\" в свой часослов.\r\n",
				CCIMAG(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
	else
		sprintf(buf, "Вы занесли заклинание \"%s%s%s\" в свои резы.\r\n",
				CCIMAG(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	ch->MemQueue.total += mag_manacost(ch, num);
	while (*pi)
		pi = &((*pi)->link);
	CREATE(i, struct spell_mem_queue_item, 1);
	*pi = i;
	i->spellnum = num;
	i->link = NULL;
}

void MemQ_forget(CHAR_DATA * ch, int num)
{
	struct spell_mem_queue_item **q = NULL, **i;

	for (i = &ch->MemQueue.queue; *i; i = &(i[0]->link))
	{
		if (i[0]->spellnum == num)
			q = i;
	}

	if (q == NULL)
	{
		send_to_char("Вы и не собирались заучить это заклинание.\r\n", ch);
	}
	else
	{
		struct spell_mem_queue_item *ptr;
		if (q == &ch->MemQueue.queue)
			GET_MEM_COMPLETED(ch) = 0;
		GET_MEM_TOTAL(ch) = MAX(0, GET_MEM_TOTAL(ch) - mag_manacost(ch, num));
		ptr = q[0];
		q[0] = q[0]->link;
		free(ptr);
		sprintf(buf,
				"Вы вычеркнули заклинание \"%s%s%s\" из списка для запоминания.\r\n",
				CCIMAG(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
}

int *MemQ_slots(CHAR_DATA * ch)
{
	struct spell_mem_queue_item **q, *qt;
	static int slots[MAX_SLOT];
	int i, n, sloti;

	// инициализация
	for (i = 0; i < MAX_SLOT; ++i)
		slots[i] = slot_for_char(ch, i + 1);

	for (i = MAX_SPELLS; i >= 1; --i)
	{
		if (!IS_SET(GET_SPELL_TYPE(ch, i), SPELL_KNOW))
			continue;
		if ((n = GET_SPELL_MEM(ch, i)) == 0)
			continue;
		sloti = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
		if (MIN_CAST_LEV(spell_info[i], ch) > GET_LEVEL(ch)
				|| MIN_CAST_REM(spell_info[i], ch) > GET_REMORT(ch))
		{
			GET_SPELL_MEM(ch, i) = 0;
			continue;
		}
		slots[sloti] -= n;
		if (slots[sloti] < 0)
		{
			GET_SPELL_MEM(ch, i) += slots[sloti];
			slots[sloti] = 0;
		}

	}

	for (q = &ch->MemQueue.queue; q[0];)
	{
		sloti = spell_info[q[0]->spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
		if (sloti >= 0 && sloti <= 10)
		{
			--slots[sloti];
			if (slots[sloti] >= 0 &&
					MIN_CAST_LEV(spell_info[q[0]->spellnum], ch) <= GET_LEVEL(ch)
					&& MIN_CAST_REM(spell_info[q[0]->spellnum], ch) <= GET_REMORT(ch))
			{
				q = &(q[0]->link);
			}
			else
			{
				if (q == &ch->MemQueue.queue)
					GET_MEM_COMPLETED(ch) = 0;
				GET_MEM_TOTAL(ch) = MAX(0, GET_MEM_TOTAL(ch) - mag_manacost(ch, q[0]->spellnum));
				++slots[sloti];
				qt = q[0];
				q[0] = q[0]->link;
				free(qt);
			}
		}
	}

	for (i = 0; i < MAX_SLOT; ++i)
		slots[i] = slot_for_char(ch, i + 1) - slots[i];

	return slots;
}

int equip_in_metall(CHAR_DATA * ch)
{
	int i, wgt = 0;

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		return (FALSE);
	if (IS_GOD(ch))
		return (FALSE);

	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i)
			&& ObjSystem::is_armor_type(GET_EQ(ch, i))
			&& GET_OBJ_MATER(GET_EQ(ch, i)) <= MAT_COLOR)
		{
			wgt += GET_OBJ_WEIGHT(GET_EQ(ch, i));
		}
	}

	if (wgt > GET_REAL_STR(ch))
		return (TRUE);

	return (FALSE);
}

int awake_others(CHAR_DATA * ch)
{
	if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		return (FALSE);

	if (IS_GOD(ch))
		return (FALSE);

	if (AFF_FLAGGED(ch, AFF_STAIRS) ||
			AFF_FLAGGED(ch, AFF_SANCTUARY) || AFF_FLAGGED(ch, AFF_SINGLELIGHT) || AFF_FLAGGED(ch, AFF_HOLYLIGHT))
		return (TRUE);

	return (FALSE);
}

// Учет резиста - возвращается эффект от спелл или умения с учетом резиста

int calculate_resistance_coeff(CHAR_DATA *ch, int resist_type, int effect)
{
	int result, resistance;

	resistance = GET_RESIST(ch, resist_type);

	if (resistance <= 0)
	{
		return effect - resistance * effect / 100;
	}
	if (IS_NPC(ch) && resistance >= 200)
	{
		return 0;
	}
	if (!IS_NPC(ch))
	{
		resistance = MIN(75, resistance);
	}
	result = effect - (resistance + number(0, resistance)) * effect / 200;
	result = MAX(0, result);
	return result;
}

// * Берется минимальная цена ренты шмотки, не важно, одетая она будет или снятая.
int get_object_low_rent(OBJ_DATA *obj)
{
	int rent = GET_OBJ_RENT(obj) > GET_OBJ_RENTEQ(obj) ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj);
	return rent;
}

// * Удаление рунной метки (при пропадании в пустоте и реморте).
void remove_rune_label(CHAR_DATA *ch)
{
	ROOM_DATA *label_room = RoomSpells::find_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
	if (label_room)
	{
		AFFECT_DATA *aff = room_affected_by_spell(label_room, SPELL_RUNE_LABEL);
		if (aff)
		{
			affect_room_remove(label_room, aff);
			send_to_char("Ваша рунная метка удалена.\r\n", ch);
		}
	}
}
