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

#include "handler.h"

#include "world.characters.hpp"
#include "object.prototypes.hpp"
#include "world.objects.hpp"
#include "obj.hpp"
#include "comm.h"
#include "db.h"
#include "glory_const.hpp"
#include "interpreter.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "dg_db_scripts.hpp"
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
#include "pk.h"
#include "ext_money.hpp"
#include "noob.hpp"
#include "obj_sets.hpp"
#include "char_obj_utils.inl"
#include "constants.h"
#include "spell_parser.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "zone.table.hpp"
#include "backtrace.hpp"

#include <math.h>

#include <unordered_set>
#include <sstream>

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
void perform_drop_gold(CHAR_DATA * ch, int amount);
int slot_for_char(CHAR_DATA * ch, int i);
int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
int extra_damroll(int class_num, int level);
void do_entergame(DESCRIPTOR_DATA * d);
void do_return(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
extern void check_auction(CHAR_DATA * ch, OBJ_DATA * obj);
extern void check_exchange(OBJ_DATA * obj);
int get_player_charms(CHAR_DATA * ch, int spellnum);
extern std::vector<City> cities;
extern int global_uid;
extern void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
extern char *find_exdesc(char *word, const EXTRA_DESCR_DATA::shared_ptr& list);

char *fname(const char *namelist)
{
	static char holder[30];
	char *point;

	for (point = holder; a_isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

bool is_wear_light(CHAR_DATA *ch)
{
	bool wear_light = FALSE;
	for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++)
	{
		if (GET_EQ(ch, wear_pos)
			&& GET_OBJ_TYPE(GET_EQ(ch, wear_pos)) == OBJ_DATA::ITEM_LIGHT
			&& GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2))
		{
			wear_light = TRUE;
		}
	}
	return wear_light;
}

void check_light(CHAR_DATA * ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef)
{
	if (ch->in_room == NOWHERE)
	{
		return;
	}

	// In equipment
	if (is_wear_light(ch))
	{
		if (was_equip == LIGHT_NO)
		{
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light + koef);
		}
	}
	else
	{
		if (was_equip == LIGHT_YES)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light - koef);
	}

	// Singlelight affect
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT))
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
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT))
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
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK))  	// if (IS_IMMORTAL(ch))
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

/*	if (GET_LEVEL(ch) >= LVL_GOD)
	{
		sprintf(buf,"Light:%d Glight:%d gdark%d koef:%d\r\n",world[ch->in_room]->light,world[ch->in_room]->glight,world[ch->in_room]->gdark,koef);
		send_to_char(buf,ch);
	}*/
}

void affect_modify(CHAR_DATA * ch, byte loc, int mod, const EAffectFlag bitv, bool add)
{
	if (add)
	{
		AFF_FLAGS(ch).set(bitv);
	}
	else
	{
		AFF_FLAGS(ch).unset(bitv);
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
	case APPLY_RESIST_DARK:
		GET_RESIST(ch, DARK_RESISTANCE) += mod;
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
	case APPLY_PR:
		GET_PR(ch) += mod; //скиллрезист
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
		ROOM_AFF_FLAGS(room).set(bitv);
	}
	else
	{
		ROOM_AFF_FLAGS(room).unset(bitv);
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
	// А че тут надо делать пересуммирование аффектов от заклов.
	/* Вобщем все комнаты имеют (вроде как базовую и
	   добавочную характеристику) если скажем ввести
	   возможность устанавливать степень зараженности комнтаы
	   в OLC , то дополнительное загаживание только будет вносить
	   + но не обнулять это значение. */

	// обнуляем все добавочные характеристики
	memset(&room->add_property, 0 , sizeof(room_property_data));

	// перенакладываем аффекты
	for (const auto& af : room->affected)
	{
		affect_room_modify(room, af->location, af->modifier, af->bitvector, TRUE);
	}
}

std::array<EAffectFlag, 2> char_saved_aff =
{
	EAffectFlag::AFF_GROUP,
	EAffectFlag::AFF_HORSE
};

std::array<EAffectFlag, 3> char_stealth_aff =
{
	EAffectFlag::AFF_HIDE,
	EAffectFlag::AFF_SNEAK,
	EAffectFlag::AFF_CAMOUFLAGE
};

///
/// Сет чару аффектов, которые должны висеть постоянно (через affect_total)
///
// Была ошибка, у нубов реген хитов был всегда 50, хотя с 26 по 30, должен быть 60.
// Теперь аффект регенерация новичка держится 3 реморта, с каждыи ремортом все слабее и слабее
void apply_natural_affects(CHAR_DATA *ch)
{
	if (GET_REMORT(ch) <= 3 && !IS_IMMORTAL(ch))
	{
		affect_modify(ch, APPLY_HITREG, 60 - (GET_REMORT(ch) * 10), EAffectFlag::AFF_NOOB_REGEN, TRUE);
		affect_modify(ch, APPLY_MOVEREG, 100, EAffectFlag::AFF_NOOB_REGEN, TRUE);
		affect_modify(ch, APPLY_MANAREG, 100 - (GET_REMORT(ch) * 20), EAffectFlag::AFF_NOOB_REGEN, TRUE);
	}
}

// This updates a character by subtracting everything he is affected by
// restoring original abilities, and then affecting all again
void affect_total(CHAR_DATA * ch)
{
	OBJ_DATA *obj;

	FLAG_DATA saved;

	// Init struct
	saved.clear();

	ch->clear_add_affects();

	// PC's clear all affects, because recalc one
	{
		saved = ch->char_specials.saved.affected_by;
		if (IS_NPC(ch))
			ch->char_specials.saved.affected_by = mob_proto[GET_MOB_RNUM(ch)].char_specials.saved.affected_by;
		else
			ch->char_specials.saved.affected_by = clear_flags;
		for (const auto& i : char_saved_aff)
		{
			if (saved.get(i))
			{
				AFF_FLAGS(ch).set(i);
			}
		}
	}
	

	// Restore values for NPC - added by Adept
	if (IS_NPC(ch))
	{
		(ch)->add_abils = (&mob_proto[GET_MOB_RNUM(ch)])->add_abils;
	}

	// move object modifiers
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if ((obj = GET_EQ(ch, i)))
		{
			if (ObjSystem::is_armor_type(obj))
			{
				GET_AC_ADD(ch) -= apply_ac(ch, i);
				GET_ARMOUR(ch) += apply_armour(ch, i);
			}
			// Update weapon applies
			for (int j = 0; j < MAX_OBJ_AFFECT; j++)
			{
				affect_modify(ch, GET_EQ(ch, i)->get_affected(j).location, GET_EQ(ch, i)->get_affected(j).modifier, static_cast<EAffectFlag>(0), TRUE);
			}
			// Update weapon bitvectors
			for (const auto& j : weapon_affect)
			{
				// То же самое, но переформулировал
				if (j.aff_bitvector == 0 || !IS_OBJ_AFF(obj, j.aff_pos))
				{
					continue;
				}
				affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(j.aff_bitvector), TRUE);
			}
		}
	}
	ch->obj_bonus().apply_affects(ch);

	// move features modifiers - added by Gorrah
	for (int i = 1; i < MAX_FEATS; i++)
	{
		if (can_use_feat(ch, i) && (feat_info[i].type == AFFECT_FTYPE))
		{
			for (int j = 0; j < MAX_FEAT_AFFECT; j++)
			{
				affect_modify(ch, feat_info[i].affected[j].location, feat_info[i].affected[j].modifier, static_cast<EAffectFlag>(0), TRUE);
			}
		}
	}

	// IMPREGNABLE_FEAT учитывается дважды: выше начисляем единичку за 0 мортов, а теперь по 1 за каждый морт
	if (can_use_feat(ch, IMPREGNABLE_FEAT))
	{
		for (int j = 0; j < MAX_FEAT_AFFECT; j++)
		{
			affect_modify(ch, feat_info[IMPREGNABLE_FEAT].affected[j].location,
				MIN(9, feat_info[IMPREGNABLE_FEAT].affected[j].modifier*GET_REMORT(ch)), static_cast<EAffectFlag>(0), TRUE);
		}
	};

	// Обработка изворотливости (с) Числобог
	if (can_use_feat(ch, DODGER_FEAT))
	{
		affect_modify(ch, APPLY_SAVING_REFLEX, -(GET_REMORT(ch) + GET_LEVEL(ch)), static_cast<EAffectFlag>(0), TRUE);
		affect_modify(ch, APPLY_SAVING_WILL, -(GET_REMORT(ch) + GET_LEVEL(ch)), static_cast<EAffectFlag>(0), TRUE);
		affect_modify(ch, APPLY_SAVING_STABILITY, -(GET_REMORT(ch) + GET_LEVEL(ch)), static_cast<EAffectFlag>(0), TRUE);
		affect_modify(ch, APPLY_SAVING_CRITICAL, -(GET_REMORT(ch) + GET_LEVEL(ch)), static_cast<EAffectFlag>(0), TRUE);
	}

	// Обработка "выносливости" и "богатырского здоровья
	// Знаю, что кривовато, придумаете, как лучше - делайте
	if (!IS_NPC(ch))
	{
		if (can_use_feat(ch, ENDURANCE_FEAT))
			affect_modify(ch, APPLY_MOVE, GET_LEVEL(ch) * 2, static_cast<EAffectFlag>(0), TRUE);
		if (can_use_feat(ch, SPLENDID_HEALTH_FEAT))
			affect_modify(ch, APPLY_HIT, GET_LEVEL(ch) * 2, static_cast<EAffectFlag>(0), TRUE);
		GloryConst::apply_modifiers(ch);
		apply_natural_affects(ch);
	}

	// move affect modifiers
	for (const auto& af : ch->affected)
	{
		affect_modify(ch, af->location, af->modifier, static_cast<EAffectFlag>(af->bitvector), TRUE);
	}

	// move race and class modifiers
	if (!IS_NPC(ch))
	{
		if ((int)GET_CLASS(ch) >= 0 && (int)GET_CLASS(ch) < NUM_PLAYER_CLASSES)
		{
			for (auto i : *class_app[(int)GET_CLASS(ch)].extra_affects)
			{
				affect_modify(ch, APPLY_NONE, 0, i.affect, i.set_or_clear ? true : false);
			}
		}

		// Apply other PC modifiers
		const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
		if (wdex != 0)
		{
			ch->set_dex_add(ch->get_dex_add() - wdex);
		}
		GET_DR_ADD(ch) += extra_damroll((int)GET_CLASS(ch), (int)GET_LEVEL(ch));
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_NOOB_REGEN))
		{
			GET_HITREG(ch) += ((int)GET_LEVEL(ch) + 4) / 5 * 10;
		}
		if (can_use_feat(ch, DARKREGEN_FEAT))
		{
			GET_HITREG(ch) += GET_HITREG(ch) * 0.2;
		}
		if (GET_CON_ADD(ch))
		{
			GET_HIT_ADD(ch) += PlayerSystem::con_add_hp(ch);
			int i = GET_MAX_HIT(ch) + GET_HIT_ADD(ch);
			if (i < 1)
			{
				GET_HIT_ADD(ch) -= (i - 1);
			}
		}

		if (!WAITLESS(ch) && on_horse(ch))
		{
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
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
			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS)
				&& OK_BOTH(ch, obj)
				&& !GET_EQ(ch, WEAR_HOLD)
				&& !GET_EQ(ch, WEAR_LIGHT)
				&& !GET_EQ(ch, WEAR_SHIELD)
				&& !GET_EQ(ch, WEAR_WIELD)
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
		if ((obj = GET_EQ(ch, WEAR_QUIVER)) && !GET_EQ(ch, WEAR_BOTHS))
		{
			send_to_char("Нету лука, нет и стрел.\r\n", ch);
			act("$n прекратил$g использовать $o3.", FALSE, ch, obj, 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_QUIVER), ch);
			return;
		}
	}

	// calculate DAMAGE value
	GET_DAMAGE(ch) = (str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GET_REAL_DR(ch)) * 2;
	if ((obj = GET_EQ(ch, WEAR_BOTHS))
		&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1; // правильный расчет среднего у оружия
	}
	else
	{
		if ((obj = GET_EQ(ch, WEAR_WIELD))
			&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
		{
			GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
		if ((obj = GET_EQ(ch, WEAR_HOLD))
			&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
		{
			GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
	}

	{
		// Calculate CASTER value
		int i = 1;
		for (GET_CASTER(ch) = 0; !IS_NPC(ch) && i <= MAX_SPELLS; i++)
		{
			if (IS_SET(GET_SPELL_TYPE(ch, i), SPELL_KNOW | SPELL_TEMP))
			{
				GET_CASTER(ch) += (spell_info[i].danger * GET_SPELL_MEM(ch, i));
			}
		}
	}

	{
		// Check steal affects
		for (const auto& i : char_stealth_aff)
		{
			if (saved.get(i)
				&& !AFF_FLAGS(ch).get(i))
			{
				CHECK_AGRO(ch) = TRUE;
			}
		}
	}

	check_berserk(ch);
	if (ch->get_fighting() || affected_by_spell(ch, SPELL_GLITTERDUST))
	{
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
	}
}

/* Намазываем аффект на комнату.
  Автоматически ставим нузные флаги */
void affect_to_room(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af)
{
	AFFECT_DATA<ERoomApplyLocation>::shared_ptr new_affect(new AFFECT_DATA<ERoomApplyLocation>(af));

	room->affected.push_front(new_affect);

	affect_room_modify(room, af.location, af.modifier, af.bitvector, TRUE);
	affect_room_total(room);
}

/* Insert an affect_type in a char_data structure
   Automatically sets appropriate bits and apply's */
void affect_to_char(CHAR_DATA* ch, const AFFECT_DATA<EApplyLocation>& af)
{
	long was_lgt = AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO;
	long was_hlgt = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO;
	long was_hdrk = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;

	AFFECT_DATA<EApplyLocation>::shared_ptr affected_alloc(new AFFECT_DATA<EApplyLocation>(af));

	ch->affected.push_front(affected_alloc);

	AFF_FLAGS(ch) += af.aff;
	if (af.bitvector)
		affect_modify(ch, af.location, af.modifier, static_cast<EAffectFlag>(af.bitvector), TRUE);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
	affect_total(ch);
	check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}

void affect_room_remove(ROOM_DATA* room, const ROOM_DATA::room_affects_list_t::iterator& affect_i)
{
	if (room->affected.empty())
	{
		log("SYSERR: affect_room_remove when no affects...");
		return;
	}

	const auto affect = *affect_i;
	affect_room_modify(room, affect->location, affect->modifier, affect->bitvector, FALSE);
	room->affected.erase(affect_i);

	affect_room_total(room);
}

// Call affect_remove with every spell of spelltype "skill"
void affect_from_char(CHAR_DATA * ch, int type)
{
	auto next_affect_i = ch->affected.begin();
	for (auto affect_i = next_affect_i; affect_i != ch->affected.end(); affect_i = next_affect_i)
	{
		++next_affect_i;
		const auto affect = *affect_i;
		if (affect->type == type)
		{
			ch->affect_remove(affect_i);
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
bool affected_by_spell(CHAR_DATA* ch, int type)
{
	if (type == SPELL_POWER_HOLD)
	{
		type = SPELL_HOLD;
	}
	else if (type == SPELL_POWER_SILENCE)
	{
		type = SPELL_SILENCE;
	}
	else if (type == SPELL_POWER_BLINDNESS)
	{
		type = SPELL_BLINDNESS;
	}

	for (const auto& affect : ch->affected)
	{
		if (affect->type == type)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}
// Проверяем а не висит ли на комнате закла ужо
//bool room_affected_by_spell(ROOM_DATA * room, int type)
ROOM_DATA::room_affects_list_t::iterator find_room_affect(ROOM_DATA* room, int type)
{
	for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i)
	{
		const auto affect = *affect_i;
		if (affect->type == type)
		{
			return affect_i;
		}
	}

	return room->affected.end();
}

void affect_join_fspell(CHAR_DATA* ch, const AFFECT_DATA<EApplyLocation>& af)
{
	bool found = false;

	for (const auto& affect : ch->affected)
	{
		if ((affect->type == af.type) && (affect->location == af.location))
		{
			if (affect->modifier < af.modifier)
			{
				affect->modifier = af.modifier;
			}

			if (affect->duration < af.duration)
			{
				affect->duration = af.duration;
			}

			affect_total(ch);
			found = true;
			break;
		}
	}

	if (!found)
	{
		affect_to_char(ch, af);
	}
}

void affect_room_join_fspell(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af)
{
	bool found = FALSE;

	for (const auto& hjp : room->affected)
	{
		if (hjp->type == af.type
			&& hjp->location == af.location)
		{
			if (hjp->modifier < af.modifier)
			{
				hjp->modifier = af.modifier;
			}

			if (hjp->duration < af.duration)
			{
				hjp->duration = af.duration;
			}

			affect_room_total(room);
			found = true;
			break;
		}
	}

	if (!found)
	{
		affect_to_room(room, af);
	}
}

void affect_room_join(ROOM_DATA * room, AFFECT_DATA<ERoomApplyLocation>& af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
	bool found = false;

	if (af.location)
	{
		for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i)
		{
			const auto& affect = *affect_i;
			if (affect->type == af.type
				&& affect->location == af.location)
			{
				if (add_dur)
				{
					af.duration += affect->duration;
				}

				if (avg_dur)
				{
					af.duration /= 2;
				}

				if (add_mod)
				{
					af.modifier += affect->modifier;
				}

				if (avg_mod)
				{
					af.modifier /= 2;
				}

				affect_room_remove(room, affect_i);
				affect_to_room(room, af);

				found = true;
				break;
			}
		}
	}

	if (!found)
	{
		affect_to_room(room, af);
	}
}

void affect_join(CHAR_DATA * ch, AFFECT_DATA<EApplyLocation>& af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
	bool found = false;

	if (af.location)
	{
		for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i)
		{
			const auto& affect = *affect_i;
			if (affect->type == af.type
				&& affect->location == af.location)
			{
				if (add_dur)
				{
					af.duration += affect->duration;
				}

				if (avg_dur)
				{
					af.duration /= 2;
				}

				if (add_mod)
				{
					af.modifier += affect->modifier;
				}

				if (avg_mod)
				{
					af.modifier /= 2;
				}

				ch->affect_remove(affect_i);
				affect_to_char(ch, af);

				found = true;
				break;
			}
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
	
	CREATE(timed_alloc, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed_feat;
	ch->timed_feat = timed_alloc;
}

void timed_feat_from_char(CHAR_DATA * ch, struct timed_type *timed)
{
	if (ch->timed_feat == NULL)
	{
		log("SYSERR: timed_feat_from_char(%s) when no timed...", GET_NAME(ch));
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed_feat);
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

	CREATE(timed_alloc, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed;
	ch->timed = timed_alloc;
}

void timed_from_char(CHAR_DATA * ch, struct timed_type *timed)
{
	if (ch->timed == NULL)
	{
		log("SYSERR: timed_from_char(%s) when no timed...", GET_NAME(ch));
		// core_dump();
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed);
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
	if (ch == NULL || ch->in_room == NOWHERE)
	{
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
		return;
	}

	if (ch->get_fighting() != NULL)
		stop_fighting(ch, TRUE);

	if (!IS_NPC(ch))
		ch->set_from_room(ch->in_room);

	check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, -1);

	auto& people = world[ch->in_room]->people;
	people.erase(std::find(people.begin(), people.end(), ch));

	ch->in_room = NOWHERE;
	ch->track_dirs = 0;
}

void room_affect_process_on_entry(CHAR_DATA * ch, room_rnum room)
{
	if (IS_IMMORTAL(ch))
	{
		return;
	}

	const auto affect_on_room = find_room_affect(world[room], SPELL_HYPNOTIC_PATTERN);
	if (affect_on_room != world[room]->affected.end())
	{
		CHAR_DATA *caster = find_char((*affect_on_room)->caster_id);
		if (!same_group(ch, caster)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)
			&& (number(1,100) <= 30)) // 30% шанс что враг уснет
		{
			if (ch->has_master()
				&& !IS_NPC(ch->get_master())
				&& IS_NPC(ch))
			{
				return;
			}

			send_to_char("Вы уставились на огненный узор, как баран на новые ворота.",ch);
			act("$n0 уставил$u на огненный узор, как баран на новые ворота.", TRUE, ch, 0, ch, TO_ROOM | TO_ARENA_LISTEN);
			call_magic(caster, ch, NULL, NULL, SPELL_SLEEP, GET_LEVEL(caster), CAST_SPELL);
		}
	}
}

// place a character in a room
void char_to_room(CHAR_DATA * ch, room_rnum room)
{
	if (ch == NULL || room < NOWHERE + 1 || room > top_of_world)
	{
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
		return;
	}

	if (!IS_NPC(ch) && !Clan::MayEnter(ch, room, HCE_PORTAL))
	{
		room = ch->get_from_room();
	}

	if (!IS_NPC(ch) && RENTABLE(ch) && ROOM_FLAGGED(room, ROOM_ARENA) && !IS_IMMORTAL(ch))
	{
		send_to_char("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
		room = ch->get_from_room();
	}
	world[room]->people.push_front(ch);

	ch->in_room = room;
	check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 1);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILSNEAK);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILCAMOUFLAGE);
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
	if (ch->get_fighting() && ch->in_room != IN_ROOM(ch->get_fighting()))
	{
		stop_fighting(ch->get_fighting(), FALSE);
		stop_fighting(ch, TRUE);
	}

	if (!IS_NPC(ch))
	{
		zone_table[world[room]->zone].used = true;
		zone_table[world[room]->zone].activity++;
	}
	else
	{
		//sventovit: здесь обрабатываются только неписи, чтобы игрок успел увидеть комнату
		//как сделать красивей я не придумал, т.к. look_at_room вызывается в act.movement а не тут
		room_affect_process_on_entry(ch, ch->in_room);
	}

	// report room changing
	if (ch->desc)
	{
		if (!(IS_DARK(ch->in_room) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
			ch->desc->msdp_report("ROOM");
	}

	for (unsigned int i = 0; i < cities.size(); i++)
	{
		if (GET_ROOM_VNUM(room) == cities[i].rent_vnum)
		{
			ch->mark_city(i);
			break;
		}
	}
}

void restore_object(OBJ_DATA * obj, CHAR_DATA * ch)
{
	int i = GET_OBJ_RNUM(obj);
	if (i < 0)
	{
		return;
	}

	if (GET_OBJ_OWNER(obj)
		&& OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODONATE)
		&& ch
		&& GET_UNIQUE(ch) != GET_OBJ_OWNER(obj))
	{
		sprintf(buf, "Зашли в проверку restore_object, Игрок %s, Объект %d", GET_NAME(ch), GET_OBJ_VNUM(obj));
		mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
	}
}

// выясняет, стокаются ли объекты с метками
bool stockable_custom_labels(OBJ_DATA *obj_one, OBJ_DATA *obj_two)
{
	// без меток стокаются
	if (!obj_one->get_custom_label() && !obj_two->get_custom_label())
		return 1;

	if (obj_one->get_custom_label() && obj_two->get_custom_label())
	{
		// с разными типами меток не стокаются
		if (!obj_one->get_custom_label()->clan != !obj_two->get_custom_label()->clan)
		{
			return 0;
		}
		else
		{
			// обе метки клановые один клан, текст совпадает -- стокается
			if (obj_one->get_custom_label()->clan && obj_two->get_custom_label()->clan
				&& !strcmp(obj_one->get_custom_label()->clan, obj_two->get_custom_label()->clan)
				&& obj_one->get_custom_label()->label_text && obj_two->get_custom_label()->label_text
				&& !strcmp(obj_one->get_custom_label()->label_text, obj_two->get_custom_label()->label_text))
			{
				return 1;
			}

			// обе метки личные, один автор, текст совпадает -- стокается
			if (obj_one->get_custom_label()->author == obj_two->get_custom_label()->author
				&& obj_one->get_custom_label()->label_text && obj_two->get_custom_label()->label_text
				&& !strcmp(obj_one->get_custom_label()->label_text, obj_two->get_custom_label()->label_text))
			{
				return 1;
			}
		}
	}

	return 0;
}

// выяснение стокаются ли предметы
bool equal_obj(OBJ_DATA *obj_one, OBJ_DATA *obj_two)
{
	if (GET_OBJ_VNUM(obj_one) != GET_OBJ_VNUM(obj_two)
		|| strcmp(obj_one->get_short_description().c_str(), obj_two->get_short_description().c_str())
		|| (GET_OBJ_TYPE(obj_one) == OBJ_DATA::ITEM_DRINKCON
			&& GET_OBJ_VAL(obj_one, 2) != GET_OBJ_VAL(obj_two, 2))
		|| (GET_OBJ_TYPE(obj_one) == OBJ_DATA::ITEM_CONTAINER
			&& (obj_one->get_contains() || obj_two->get_contains()))
		|| GET_OBJ_VNUM(obj_two) == -1
		|| (GET_OBJ_TYPE(obj_one) == OBJ_DATA::ITEM_BOOK
			&& GET_OBJ_VAL(obj_one, 1) != GET_OBJ_VAL(obj_two, 1))
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

	obj->set_next_content(begin = *list_start);
	*list_start = obj;

	// похожий предмет уже первый в списке или список пустой
	if (!begin || equal_obj(begin, obj))
	{
		return;
	}

	before = p = begin;

	while (p && !equal_obj(p, obj))
	{
		before = p;
		p = p->get_next_content();
	}

	// нет похожих предметов
	if (!p)
	{
		return;
	}

	end = p;

	while (p && equal_obj(p, obj))
	{
		end = p;
		p = p->get_next_content();
	}

	end->set_next_content(begin);
	obj->set_next_content(before->get_next_content());
	before->set_next_content(p); // будет 0 если после перемещаемых ничего не лежало
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
		object->set_uid(global_uid); // Назначаем уид
	}
}

// give an object to a char
void obj_to_char(OBJ_DATA * object, CHAR_DATA * ch)
{
	unsigned int tuid;
	int inworld;

	if (!world_objects.get_by_raw_ptr(object))
	{
		std::stringstream ss;
		ss << "SYSERR: Object at address 0x" << object
			<< " is not in the world but we have attempt to put it into character '" << ch->get_name()
			<< "'. Object won't be placed into character's inventory.";
		mudlog(ss.str().c_str(), NRM, LVL_IMPL, SYSLOG, TRUE);
		debug::backtrace(runtime_config.logs(ERRLOG).handle());

		return;
	}

	int may_carry = TRUE;
	if (object && ch)
	{
		restore_object(object, ch);
		if (invalid_anti_class(ch, object) || invalid_unique(ch, object) || NamedStuff::check_named(ch, object, 0))
			may_carry = FALSE;
		if (!may_carry)
		{
			act("Вас обожгло при попытке взять $o3.", FALSE, ch, object, 0, TO_CHAR);
			act("$n попытал$u взять $o3 - и чудом не сгорел$g.", FALSE, ch, object, 0, TO_ROOM);
			obj_to_room(object, ch->in_room);
			return;
		}
		if (!IS_NPC(ch)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master())))
		{
			// Контроль уникальности предметов
			if (object && // Объект существует
					GET_OBJ_UID(object) != 0 && // Есть UID
					object->get_timer() > 0) // Целенький
			{
				tuid = GET_OBJ_UID(object);
				inworld = 1;
				// Объект готов для проверки. Ищем в мире такой же.
				world_objects.foreach_with_vnum(GET_OBJ_VNUM(object), [&](const OBJ_DATA::shared_ptr& i)
				{
					if (GET_OBJ_UID(i) == tuid // UID совпадает
						&& i->get_timer() > 0  // Целенький
						&& object != i.get()) // Не оно же
					{
						inworld++;
					}
				});

				if (inworld > 1) // У объекта есть как минимум одна копия
				{
					sprintf(buf, "Copy detected and prepared to extract! Object %s (UID=%u, VNUM=%d), holder %s. In world %d.",
							object->get_PName(0).c_str(), GET_OBJ_UID(object), GET_OBJ_VNUM(object), GET_NAME(ch), inworld);
					mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
					// Удаление предмета
					act("$o0 замигал$Q и вы увидели медленно проступившие руны 'DUPE'.", FALSE, ch, object, 0, TO_CHAR);
					object->set_timer(0); // Хана предмету
					object->set_extra_flag(EExtraFlag::ITEM_NOSELL); // Ибо нефиг
				}
			} // Назначаем UID
			else
			{
				set_uid(object);
				log("%s obj_to_char %s #%d|%u", GET_NAME(ch), object->get_PName(0).c_str(), GET_OBJ_VNUM(object), object->get_uid());
			}
		}

		if (!IS_NPC(ch)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master())))
		{
			object->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);	// start timer unconditionally when character picks item up.
			insert_obj_and_group(object, &ch->carrying);
		}
		else
		{
			// Вот эта муть, чтобы временно обойти завязку магазинов на порядке предметов в инве моба // Krodo
			object->set_next_content(ch->carrying);
			ch->carrying = object;
		}

		object->set_carried_by(ch);
		object->set_in_room(NOWHERE);
		IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
		IS_CARRYING_N(ch)++;

		if (!IS_NPC(ch))
		{
			log("obj_to_char: %s -> %d", ch->get_name().c_str(), GET_OBJ_VNUM(object));
		}
		// set flag for crash-save system, but not on mobs!
		if (!IS_NPC(ch))
		{
			PLR_FLAGS(ch).set(PLR_CRASH);
		}
	}
	else
		log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}


// take an object from a char
void obj_from_char(OBJ_DATA * object)
{
	if (!object || !object->get_carried_by())
	{
		log("SYSERR: NULL object or owner passed to obj_from_char");
		return;
	}
	object->remove_me_from_contains_list(object->get_carried_by()->carrying);

	// set flag for crash-save system, but not on mobs!
	if (!IS_NPC(object->get_carried_by()))
	{
		PLR_FLAGS(object->get_carried_by()).set(PLR_CRASH);
		log("obj_from_char: %s -> %d", object->get_carried_by()->get_name().c_str(), GET_OBJ_VNUM(object));
	}

	IS_CARRYING_W(object->get_carried_by()) -= GET_OBJ_WEIGHT(object);
	IS_CARRYING_N(object->get_carried_by())--;
	object->set_carried_by(nullptr);
	object->set_next_content(nullptr);
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

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
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

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		factor *= MOB_ARMOUR_MULT;

	// чтобы не плюсовать левую броню на стафе с текущей прочностью выше максимальной
	int cur_dur = MIN(GET_OBJ_MAX(obj), GET_OBJ_CUR(obj));
	return (factor * GET_OBJ_VAL(obj, 1) * cur_dur / MAX(1, GET_OBJ_MAX(obj)));
}

int invalid_align(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_NPC(ch) || IS_IMMORTAL(ch))
		return (FALSE);
    if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MONO)
        && GET_RELIGION(ch) == RELIGION_MONO)
    {
        return TRUE;
    }
    if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLY)
        && GET_RELIGION(ch) == RELIGION_POLY)
    {
        return TRUE;
    }
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
		 "Вы взяли $o3 в обе руки."},

		{"$n0 начал$g использовать $o3 как колчан.",
		 "Вы начали использовать $o3 как колчан."}
	};

	act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
	act(wear_messages[where][0], IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? FALSE : TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
}

int flag_data_by_char_class(const CHAR_DATA * ch)
{
	if (ch == NULL)
		return 0;

	return flag_data_by_num(IS_NPC(ch) ? NUM_PLAYER_CLASSES * NUM_KIN : GET_CLASS(ch) + NUM_PLAYER_CLASSES * GET_KIN(ch));
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

		if (GET_EQ(ch, pos) && OBJ_FLAGGED(GET_EQ(ch, pos), EExtraFlag::ITEM_SETSTUFF) &&
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
				unique_bit_flag_data item;
				const auto flags = flag_data_by_char_class(ch);
				item.set(flags);
				if ((class_info = qty_info->second.find(item)) != qty_info->second.end())
				{
					if (GET_EQ(ch, pos) != obj)
					{
						for (int i = 0; i < MAX_OBJ_AFFECT; i++)
						{
							affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location, GET_EQ(ch, pos)->get_affected(i).modifier,
								static_cast<EAffectFlag>(0), FALSE);
						}

						if (ch->in_room != NOWHERE)
						{
							for (const auto& i : weapon_affect)
							{
								if (i.aff_bitvector == 0
									|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos))
								{
									continue;
								}
								affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), FALSE);
							}
						}
					}

					std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info->second);
					delim = act_msg.find('\n');

					if (show_msg)
					{
						act(act_msg.substr(0, delim).c_str(), FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
						act(act_msg.erase(0, delim + 1).c_str(),
							IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? FALSE : TRUE,
							ch, GET_EQ(ch, pos), 0, TO_ROOM);
					}

					for (int i = 0; i < MAX_OBJ_AFFECT; i++)
					{
						affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
							GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffectFlag>(0), TRUE);
					}

					if (ch->in_room != NOWHERE)
					{
						for (const auto& i : weapon_affect)
						{
							if (i.aff_spell == 0 || !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos))
							{
								continue;
							}
							if (!no_cast)
							{
								if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
								{
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										FALSE, ch, GET_EQ(ch, pos), 0, TO_ROOM);
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
								}
								else
								{
									mag_affects(GET_LEVEL(ch), ch, ch, i.aff_spell, SAVING_WILL);
								}
							}
						}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj)
			{
				for (int i = 0; i < MAX_OBJ_AFFECT; i++)
				{
					affect_modify(ch, obj->get_affected(i).location, obj->get_affected(i).modifier, static_cast<EAffectFlag>(0), TRUE);
				}

				if (ch->in_room != NOWHERE)
				{
					for (const auto& i : weapon_affect)
					{
						if (i.aff_spell == 0
							|| !IS_OBJ_AFF(obj, i.aff_pos))
						{
							continue;
						}
						if (!no_cast)
						{
							if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
							{
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									FALSE, ch, obj, 0, TO_ROOM);
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									FALSE, ch, obj, 0, TO_CHAR);
							}
							else
							{
								mag_affects(GET_LEVEL(ch), ch, ch, i.aff_spell, SAVING_WILL);
							}
						}
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
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_LIGHT
		&& !can_use_feat(ch, ARMOR_LIGHT_FEAT))
	{
		act("Для использования $o1 требуется способность 'легкие доспехи'.",
			FALSE, ch, obj, 0, TO_CHAR);
		return false;
	}

	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_MEDIAN
		&& !can_use_feat(ch, ARMOR_MEDIAN_FEAT))
	{
		act("Для использования $o1 требуется способность 'средние доспехи'.",
			FALSE, ch, obj, 0, TO_CHAR);
		return false;
	}

	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_HEAVY
		&& !can_use_feat(ch, ARMOR_HEAVY_FEAT))
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
	int was_lgt = AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
				  was_hlgt = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
							 was_hdrk = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
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
		log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->get_short_description().c_str());
		return;
	}
	//if (obj->carried_by) {
	//	log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj, ch, 0));
	//	return;
	//}
	if (obj->get_in_room() != NOWHERE)
	{
		log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj, ch, 0));
		return;
	}

	if (invalid_anti_class(ch, obj))
	{
		act("Вас обожгло при попытке использовать $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n попытал$u использовать $o3 - и чудом не обгорел$g.", FALSE, ch, obj, 0, TO_ROOM);
		if (obj->get_carried_by())
		{
			obj_from_char(obj);
		}
		obj_to_room(obj, ch->in_room);
		obj_decay(obj);
		return;
	}
	else if((!IS_NPC(ch) || IS_CHARMICE(ch)) && OBJ_FLAGGED(obj, EExtraFlag::ITEM_NAMED) && NamedStuff::check_named(ch, obj, true)) {
		if(!NamedStuff::wear_msg(ch, obj))
			send_to_char("Просьба не трогать! Частная собственность!\r\n", ch);
		if (!obj->get_carried_by())
		{
			obj_to_char(obj, ch);
		}
		return;
	}

	if ((!IS_NPC(ch) && invalid_align(ch, obj))
		|| invalid_no_class(ch, obj)
		|| (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SHARPEN)
				|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_ARMORED))))
	{
		act("$o0 явно не предназначен$A для вас.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", FALSE, ch, obj, 0, TO_ROOM);
		if (!obj->get_carried_by())
		{
			obj_to_char(obj, ch);
		}
		return;
	}

	if (!IS_NPC(ch) || IS_CHARMICE(ch))
	{
		CHAR_DATA *master = IS_CHARMICE(ch) && ch->has_master() ? ch->get_master() : ch;
		if ((obj->get_auto_mort_req() >= 0) && (obj->get_auto_mort_req() > GET_REMORT(master)) && !IS_IMMORTAL(master))
		{
			send_to_char(master, "Для использования %s требуется %d %s.\r\n",
				GET_OBJ_PNAME(obj, 1).c_str(),
				obj->get_auto_mort_req(),
				desc_count(obj->get_auto_mort_req(), WHAT_REMORT));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
				FALSE, ch, obj, 0, TO_ROOM);
			if (!obj->get_carried_by())
			{
				obj_to_char(obj, ch);
			}
			return;
		}
		else if ((obj->get_auto_mort_req() < -1)  && (abs(obj->get_auto_mort_req()) < GET_REMORT(master)) && !IS_IMMORTAL(master))
		{
			send_to_char(master, "Максимально количество перевоплощений для использования %s равно %d.\r\n",
				GET_OBJ_PNAME(obj, 1).c_str(),
				abs(obj->get_auto_mort_req()));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
				FALSE, ch, obj, 0, TO_ROOM);
			if (!obj->get_carried_by())
			{
				obj_to_char(obj, ch);
			}
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

	if (obj->get_carried_by())
	{
		obj_from_char(obj);
	}

	//if (GET_EQ(ch, WEAR_LIGHT) &&
	//  GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))
	//  was_lamp = TRUE;
	//Polud светить должно не только то что надето для освещения, а любой источник света
	was_lamp = is_wear_light(ch);
	//-Polud

	GET_EQ(ch, pos) = obj;
	obj->set_worn_by(ch);
	obj->set_worn_on(pos);
	obj->set_next_content(nullptr);
	CHECK_AGRO(ch) = TRUE;

	if (show_msg)
	{
		wear_message(ch, obj, pos);
		if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NAMED))
		{
			NamedStuff::wear_msg(ch, obj);
		}
	}

	if (ch->in_room == NOWHERE)
	{
		log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));
	}

	id_to_set_info_map::iterator it = OBJ_DATA::set_table.begin();

	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))
	{
		for (; it != OBJ_DATA::set_table.end(); it++)
		{
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				activate_stuff(ch, obj, it, 0 | (show_msg ? 0x80 : 0) | (no_cast ? 0x40 : 0), 0);
				break;
			}
		}
	}

	if (!OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF) || it == OBJ_DATA::set_table.end())
	{
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
		{
			affect_modify(ch, obj->get_affected(j).location, obj->get_affected(j).modifier, static_cast<EAffectFlag>(0), TRUE);
		}

		if (ch->in_room != NOWHERE)
		{
			for (const auto& j : weapon_affect)
			{
				if (j.aff_spell == 0
					|| !IS_OBJ_AFF(obj, j.aff_pos))
				{
					continue;
				}

				if (!no_cast)
				{
					if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
					{
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							FALSE, ch, obj, 0, TO_ROOM);
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							FALSE, ch, obj, 0, TO_CHAR);
					}
					else
					{
						mag_affects(GET_LEVEL(ch), ch, ch, j.aff_spell, SAVING_WILL);
					}
				}
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

		if (GET_EQ(ch, pos)
			&& OBJ_FLAGGED(GET_EQ(ch, pos), EExtraFlag::ITEM_SETSTUFF)
			&& (set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end())
		{
			unsigned int oqty = deactivate_stuff(ch, obj, it, (pos + 1) | (show_msg ? 0x40 : 0),
												 set_obj_qty + 1);
			qty_to_camap_map::const_iterator old_qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator qty_info = GET_EQ(ch, pos) == obj ?
					set_obj_info->second.begin() :
					set_obj_info->second.upper_bound(oqty - 1);

			while (old_qty_info != qty_info)
			{
				old_qty_info--;
				unique_bit_flag_data flags1;
				flags1.set(flag_data_by_char_class(ch));
				class_to_act_map::const_iterator class_info = old_qty_info->second.find(flags1);
				if (class_info != old_qty_info->second.end())
				{
					while (qty_info != set_obj_info->second.begin())
					{
						qty_info--;
						unique_bit_flag_data flags2;
						flags2.set(flag_data_by_char_class(ch));
						class_to_act_map::const_iterator class_info2 = qty_info->second.find(flags2);
						if (class_info2 != qty_info->second.end())
						{
							for (int i = 0; i < MAX_OBJ_AFFECT; i++)
							{
								affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
									GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffectFlag>(0), FALSE);
							}

							if (ch->in_room != NOWHERE)
							{
								for (const auto& i : weapon_affect)
								{
									if (i.aff_bitvector == 0
										|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos))
									{
										continue;
									}
									affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), FALSE);
								}
							}

							std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info2->second);
							delim = act_msg.find('\n');

							if (show_msg)
							{
								act(act_msg.substr(0, delim).c_str(), FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
								act(act_msg.erase(0, delim + 1).c_str(),
									IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? FALSE : TRUE,
									ch, GET_EQ(ch, pos), 0, TO_ROOM);
							}

							for (int i = 0; i < MAX_OBJ_AFFECT; i++)
							{
								affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
									GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffectFlag>(0), TRUE);
							}

							if (ch->in_room != NOWHERE)
							{
								for (const auto& i : weapon_affect)
								{
									if (i.aff_bitvector == 0
										|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos))
									{
										continue;
									}
									affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), TRUE);
								}
							}

							return oqty;
						}
					}

					for (int i = 0; i < MAX_OBJ_AFFECT; i++)
					{
						affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
							GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffectFlag>(0), FALSE);
					}

					if (ch->in_room != NOWHERE)
					{
						for (const auto& i : weapon_affect)
						{
							if (i.aff_bitvector == 0
								|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos))
							{
								continue;
							}
							affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), FALSE);
						}
					}

					std::string deact_msg = GET_EQ(ch, pos)->deactivate_obj(class_info->second);
					delim = deact_msg.find('\n');

					if (show_msg)
					{
						act(deact_msg.substr(0, delim).c_str(), FALSE, ch, GET_EQ(ch, pos), 0, TO_CHAR);
						act(deact_msg.erase(0, delim + 1).c_str(),
							IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? FALSE : TRUE,
							ch, GET_EQ(ch, pos), 0, TO_ROOM);
					}

					if (GET_EQ(ch, pos) != obj)
					{
						for (int i = 0; i < MAX_OBJ_AFFECT; i++)
						{
							affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location, GET_EQ(ch, pos)->get_affected(i).modifier,
								static_cast<EAffectFlag>(0), TRUE);
						}

						if (ch->in_room != NOWHERE)
						{
							for (const auto& i : weapon_affect)
							{
								if (i.aff_bitvector == 0 ||
									!IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos))
								{
									continue;
								}
								affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), TRUE);
							}
						}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj)
			{
				for (int i = 0; i < MAX_OBJ_AFFECT; i++)
				{
					affect_modify(ch, obj->get_affected(i).location, obj->get_affected(i).modifier, static_cast<EAffectFlag>(0), FALSE);
				}

				if (ch->in_room != NOWHERE)
				{
					for (const auto& i : weapon_affect)
					{
						if (i.aff_bitvector == 0
							|| !IS_OBJ_AFF(obj, i.aff_pos))
						{
							continue;
						}
						affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), FALSE);
					}
				}

				obj->deactivate_obj(activation());
			}

			return oqty;
		}
		else
		{
			return deactivate_stuff(ch, obj, it, (pos + 1) | (show_msg ? 0x40 : 0), set_obj_qty);
		}
	}
	else
	{
		return set_obj_qty;
	}
}

//  0x40 - show setstuff related messages
//  0x80 - no total affect update
OBJ_DATA *unequip_char(CHAR_DATA * ch, int pos)
{
	int was_lgt = AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
				  was_hlgt = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
							 was_hdrk = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO, was_lamp = FALSE;

	int j, skip_total = IS_SET(pos, 0x80), show_msg = IS_SET(pos, 0x40);

	REMOVE_BIT(pos, (0x80 | 0x40));

	if (pos < 0 || pos >= NUM_WEARS)
	{
		log("SYSERR: unequip_char(%s,%d) - unused pos...", GET_NAME(ch), pos);
		return nullptr;
	}

	OBJ_DATA* obj = GET_EQ(ch, pos);
	if (nullptr == obj)
	{
		log("SYSERR: unequip_char(%s,%d) - no equip...", GET_NAME(ch), pos);
		return nullptr;
	}

//	if (GET_EQ(ch, WEAR_LIGHT) &&
//	    GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))
//		was_lamp = TRUE;
	//Polud светить должно не только то что надето для освещения, а любой источник света
	was_lamp = is_wear_light(ch);
	//-Polud


	if (ch->in_room == NOWHERE)
		log("SYSERR: ch->in_room = NOWHERE when unequipping char %s.", GET_NAME(ch));

	id_to_set_info_map::iterator it = OBJ_DATA::set_table.begin();

	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))
		for (; it != OBJ_DATA::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				deactivate_stuff(ch, obj, it, 0 | (show_msg ? 0x40 : 0), 0);
				break;
			}

	if (!OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF) || it == OBJ_DATA::set_table.end())
	{
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
		{
			affect_modify(ch, obj->get_affected(j).location, obj->get_affected(j).modifier, static_cast<EAffectFlag>(0), FALSE);
		}

		if (ch->in_room != NOWHERE)
		{
			for (const auto& j : weapon_affect)
			{
				if (j.aff_bitvector == 0
					|| !IS_OBJ_AFF(obj, j.aff_pos))
				{
					continue;
				}
				if (IS_NPC(ch)
					&& AFF_FLAGGED(&mob_proto[GET_MOB_RNUM(ch)], static_cast<EAffectFlag>(j.aff_bitvector)))
				{
					continue;
				}
				affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(j.aff_bitvector), FALSE);
			}
		}

		if ((OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))&&(SetSystem::is_big_set(obj)))
			obj->deactivate_obj(activation());
	}

	GET_EQ(ch, pos) = NULL;
	obj->set_worn_by(nullptr);
	obj->set_worn_on(NOWHERE);
	obj->set_next_content(nullptr);

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
		obj->remove_set_bonus();

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
		{
			if (!a_isdigit(*(*name + i)))
			{
				return 1;
			}
		}
		*ppos = '\0';
		res = atoi(*name);
		strl_cpy(tmpname, ppos + 1, MAX_INPUT_LENGTH);
		strl_cpy(*name, tmpname, MAX_INPUT_LENGTH);
		return res;
	}

	return 1;
}

int get_number(std::string &name)
{
	std::string::size_type pos = name.find('.');

	if (pos != std::string::npos)
	{
		for (std::string::size_type i = 0; i != pos; i++)
			if (!a_isdigit(name[i]))
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

	for (i = list; i; i = i->get_next_content())
	{
		if (GET_OBJ_RNUM(i) == num)
		{
			return (i);
		}
	}

	return (NULL);
}

// Search a given list for an object virtul_number, and return a ptr to that obj //
OBJ_DATA *get_obj_in_list_vnum(int num, OBJ_DATA * list)
{
	OBJ_DATA *i;

	for (i = list; i; i = i->get_next_content())
	{
		if (GET_OBJ_VNUM(i) == num)
		{
			return (i);
		}
	}

	return (NULL);
}

// search the entire world for an object number, and return a pointer  //
OBJ_DATA *get_obj_num(obj_rnum nr)
{
	const auto result = world_objects.find_first_by_rnum(nr);
	return result.get();
}

// search a room for a char, and return a pointer if found..  //
CHAR_DATA *get_char_room(char *name, room_rnum room)
{
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, name);
	const int number = get_number(&tmp);
	if (0 == number)
	{
		return nullptr;
	}

	int j = 0;
	for (const auto i : world[room]->people)
	{
		if (isname(tmp, i->get_pc_name()))
		{
			if (++j == number)
			{
				return i;
			}
		}
	}

	return NULL;
}

// search all over the world for a char num, and return a pointer if found //
CHAR_DATA *get_char_num(mob_rnum nr)
{
	for (const auto i : character_list)
	{
		if (GET_MOB_RNUM(i) == nr)
		{
			return i.get();
		}
	}

	return nullptr;
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
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
			room, top_of_world, object);
		return 0;
	}
	else
	{
		restore_object(object, 0);
		insert_obj_and_group(object, &world[room]->contents);
		object->set_in_room(room);
		object->set_carried_by(nullptr);
		object->set_worn_by(nullptr);
		if (ROOM_FLAGGED(room, ROOM_NOITEM))
		{
			object->set_extra_flag(EExtraFlag::ITEM_DECAY);
		}

		if (object->get_script()->has_triggers())
		{
			object->set_destroyer(script_destroy_timer);
		}
		else if (OBJ_FLAGGED(object, EExtraFlag::ITEM_NODECAY))
		{
			object->set_destroyer(room_nodestroy_timer);
		}
		else if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_MONEY)
		{
			object->set_destroyer(money_destroy_timer);
		}
		else if (ROOM_FLAGGED(room, ROOM_DEATH))
		{
			object->set_destroyer(death_destroy_timer);
		}
		else
		{
			object->set_destroyer(room_destroy_timer);
		}
	}
	return 1;
}

/* Функция для удаления обьектов после лоада в комнату
   результат работы - 1 если посыпался, 0 - если остался */
int obj_decay(OBJ_DATA * object)
{
	int room, sect;
	room = object->get_in_room();

	if (room == NOWHERE)
		return (0);

	sect = real_sector(room);

	if (((sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM) &&
			!OBJ_FLAGGED(object, EExtraFlag::ITEM_SWIMMING) &&
			!OBJ_FLAGGED(object, EExtraFlag::ITEM_FLYING) &&
			!IS_CORPSE(object)))
	{

		act("$o0 медленно утонул$G.", FALSE, world[room]->first_character(), object, 0, TO_ROOM);
		act("$o0 медленно утонул$G.", FALSE, world[room]->first_character(), object, 0, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	if (((sect == SECT_FLYING) && !IS_CORPSE(object) && !OBJ_FLAGGED(object, EExtraFlag::ITEM_FLYING)))
	{

		act("$o0 упал$G вниз.", FALSE, world[room]->first_character(), object, 0, TO_ROOM);
		act("$o0 упал$G вниз.", FALSE, world[room]->first_character(), object, 0, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	if (OBJ_FLAGGED(object, EExtraFlag::ITEM_DECAY) ||
			(OBJ_FLAGGED(object, EExtraFlag::ITEM_ZONEDECAY) &&
			 GET_OBJ_ZONE(object) != NOWHERE && GET_OBJ_ZONE(object) != world[room]->zone))
	{

		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", FALSE,
			world[room]->first_character(), object, 0, TO_ROOM);
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", FALSE,
			world[room]->first_character(), object, 0, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	return (0);
}

// Take an object from a room
void obj_from_room(OBJ_DATA * object)
{
	if (!object || object->get_in_room() == NOWHERE)
	{
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
			object, object->get_in_room());
		return;
	}

	object->remove_me_from_contains_list(world[object->get_in_room()]->contents);

	object->set_in_room(NOWHERE);
	object->set_next_content(nullptr);
}

// put an object in an object (quaint)
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to)
{
	OBJ_DATA *tmp_obj;

	if (!obj || !obj_to || obj == obj_to)
	{
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
			obj, obj, obj_to);
		return;
	}

	auto list = obj_to->get_contains();
	insert_obj_and_group(obj, &list);
	obj_to->set_contains(list);
	obj->set_in_obj(obj_to);

	for (tmp_obj = obj->get_in_obj(); tmp_obj->get_in_obj(); tmp_obj = tmp_obj->get_in_obj())
	{
		tmp_obj->add_weight(GET_OBJ_WEIGHT(obj));
	}

	// top level object.  Subtract weight from inventory if necessary.
	tmp_obj->add_weight(GET_OBJ_WEIGHT(obj));
	if (tmp_obj->get_carried_by())
	{
		IS_CARRYING_W(tmp_obj->get_carried_by()) += GET_OBJ_WEIGHT(obj);
	}
}


// remove an object from an object
void obj_from_obj(OBJ_DATA * obj)
{
	if (obj->get_in_obj() == nullptr)
	{
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
		return;
	}
	auto obj_from = obj->get_in_obj();
	auto head = obj_from->get_contains();
	obj->remove_me_from_contains_list(head);
	obj_from->set_contains(head);

	// Subtract weight from containers container
	auto temp = obj->get_in_obj();
	for (; temp->get_in_obj(); temp = temp->get_in_obj())
	{
		temp->set_weight(MAX(1, GET_OBJ_WEIGHT(temp) - GET_OBJ_WEIGHT(obj)));
	}

	// Subtract weight from char that carries the object
	temp->set_weight(MAX(1, GET_OBJ_WEIGHT(temp) - GET_OBJ_WEIGHT(obj)));
	if (temp->get_carried_by())
	{
		IS_CARRYING_W(temp->get_carried_by()) = MAX(1, IS_CARRYING_W(temp->get_carried_by()) - GET_OBJ_WEIGHT(obj));
	}

	obj->set_in_obj(nullptr);
	obj->set_next_content(nullptr);
}


// Set all carried_by to point to new owner
void object_list_new_owner(OBJ_DATA * list, CHAR_DATA * ch)
{
	if (list)
	{
		object_list_new_owner(list->get_contains(), ch);
		object_list_new_owner(list->get_next_content(), ch);
		list->set_carried_by(ch);
	}
}

room_vnum get_room_where_obj(OBJ_DATA *obj, bool deep)
{
	if (GET_ROOM_VNUM(obj->get_in_room()) != NOWHERE)
	{
		return GET_ROOM_VNUM(obj->get_in_room());
	}
	else if (obj->get_in_obj() && !deep)
	{
		return get_room_where_obj(obj->get_in_obj(), true);
	}
	else if (obj->get_carried_by())
	{
		return GET_ROOM_VNUM(IN_ROOM(obj->get_carried_by()));
	}
	else if (obj->get_worn_by())
	{
		return GET_ROOM_VNUM(IN_ROOM(obj->get_worn_by()));
	}

	return NOWHERE;
}

// Extract an object from the world
void extract_obj(OBJ_DATA * obj)
{
	char name[MAX_STRING_LENGTH];
	OBJ_DATA *temp;

	strcpy(name, obj->get_PName(0).c_str());
	log("Extracting obj %s vnum == %d room = %d timer == %d", name, GET_OBJ_VNUM(obj), get_room_where_obj(obj, false), obj->get_timer());
// TODO: в дебаг log("Start extract obj %s", name);

	// Get rid of the contents of the object, as well.
	// Обработка содержимого контейнера при его уничтожении
	while (obj->get_contains())
	{
		temp = obj->get_contains();
		obj_from_obj(temp);

		if (obj->get_carried_by())
		{
			if (IS_NPC(obj->get_carried_by())
				|| (IS_CARRYING_N(obj->get_carried_by()) >= CAN_CARRY_N(obj->get_carried_by())))
			{
				obj_to_room(temp, IN_ROOM(obj->get_carried_by()));
				obj_decay(temp);
			}
			else
			{
				obj_to_char(temp, obj->get_carried_by());
			}
		}
		else if (obj->get_worn_by() != NULL)
		{
			if (IS_NPC(obj->get_worn_by())
				|| (IS_CARRYING_N(obj->get_worn_by()) >= CAN_CARRY_N(obj->get_worn_by())))
			{
				obj_to_room(temp, IN_ROOM(obj->get_worn_by()));
				obj_decay(temp);
			}
			else
			{
				obj_to_char(temp, obj->get_worn_by());
			}
		}
		else if (obj->get_in_room() != NOWHERE)
		{
			obj_to_room(temp, obj->get_in_room());
			obj_decay(temp);
		}
		else if (obj->get_in_obj())
		{
			extract_obj(temp);
		}
		else
		{
			extract_obj(temp);
		}
	}
	// Содержимое контейнера удалено

	if (obj->get_worn_by() != NULL)
	{
		if (unequip_char(obj->get_worn_by(), obj->get_worn_on()) != obj)
		{
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
		}
	}
	if (obj->get_in_room() != NOWHERE)
	{
		obj_from_room(obj);
	}
	else if (obj->get_carried_by())
	{
		obj_from_char(obj);
	}
	else if (obj->get_in_obj())
	{
		obj_from_obj(obj);
	}

	check_auction(NULL, obj);
	check_exchange(obj);

	const auto rnum = GET_OBJ_RNUM(obj);
	if (rnum >= 0)
	{
		obj_proto.dec_number(rnum);
	}

	obj->get_script()->set_purged();
	world_objects.remove(obj);
}

void update_object(OBJ_DATA * obj, int use)
{
	OBJ_DATA* obj_it = obj;

	while (obj_it)
	{
		// don't update objects with a timer trigger
		const bool trig_timer = SCRIPT_CHECK(obj_it, OTRIG_TIMER);
		const bool has_timer = obj_it->get_timer() > 0;
		const bool tick_timer = 0 != OBJ_FLAGGED(obj_it, EExtraFlag::ITEM_TICKTIMER);

		if (!trig_timer && has_timer && tick_timer)
		{
			obj_it->dec_timer(use);
		}

		if (obj_it->get_contains())
		{
			update_object(obj_it->get_contains(), use);
		}

		obj_it = obj_it->get_next_content();
	}
}

void update_char_objects(CHAR_DATA * ch)
{
//Polud раз уж светит любой источник света, то и гаснуть тоже должны все
	for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++)
	{
		if (GET_EQ(ch, wear_pos) != NULL)
		{
			if (GET_OBJ_TYPE(GET_EQ(ch, wear_pos)) == OBJ_DATA::ITEM_LIGHT)
			{
				if (GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2) > 0)
				{
					const int i = GET_EQ(ch, wear_pos)->dec_val(2);
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
						if (ch->in_room != NOWHERE)
						{
							if (world[ch->in_room]->light > 0)
								world[ch->in_room]->light -= 1;
						}

						if (OBJ_FLAGGED(GET_EQ(ch, wear_pos), EExtraFlag::ITEM_DECAY))
						{
							extract_obj(GET_EQ(ch, wear_pos));
						}
					}
				}
			}
		}
	}
	//-Polud

	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			update_object(GET_EQ(ch, i), 1);
		}
	}

	if (ch->carrying)
	{
		update_object(ch->carrying, 1);
	}
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
	if (zone_reset && !OBJ_FLAGGED(obj, EExtraFlag::ITEM_TICKTIMER))
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

		drop_wtrigger(obj, ch);

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
			&& i->follower->get_master() == ch)
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
			leader->add_follower_silently(*i);
		}
	}
}

} // namespace

/**
* Extract a ch completely from the world, and leave his stuff behind
* \param zone_reset - 0 обычный пурж когда угодно (по умолчанию), 1 - пурж при резете зоны
*/
void extract_char(CHAR_DATA* ch, int clear_objs, bool zone_reset)
{
	if (ch->purged())
	{
		log("SYSERROR: double extract_char (%s:%d)", __FILE__, __LINE__);
		return;
	}

	DESCRIPTOR_DATA *t_desc;
	int i;

	if (MOB_FLAGGED(ch, MOB_FREE)
		|| MOB_FLAGGED(ch, MOB_DELETE))
	{
		return;
	}

	std::string name = GET_NAME(ch);
	log("[Extract char] Start function for char %s", name.c_str());
	if (!IS_NPC(ch) && !ch->desc)
	{
//		log("[Extract char] Extract descriptors");
		for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
		{
			if (t_desc->original.get() == ch)
			{
				do_return(t_desc->character.get(), NULL, 0, 0);
			}
		}
	}

	// Forget snooping, if applicable
//	log("[Extract char] Stop snooping");
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
//	log("[Extract char] Drop equipment");
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			OBJ_DATA *obj_eq = unequip_char(ch, i);
			if (!obj_eq)
			{
				continue;
			}

			remove_otrigger(obj_eq, ch);
			drop_obj_on_zreset(ch, obj_eq, 0, zone_reset);
		}
	}

	// transfer objects to room, if any
//	log("[Extract char] Drop objects");
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

	if (!IS_NPC(ch)
		&& !ch->has_master()
		&& ch->followers
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
	{
//		log("[Extract char] Change group leader");
		change_leader(ch, 0);
	}
	else if (IS_NPC(ch)
		&& !IS_CHARMICE(ch)
		&& !ch->has_master()
		&& ch->followers)
	{
//		log("[Extract char] Changing NPC leader");
		change_npc_leader(ch);
	}

//	log("[Extract char] Die followers");
	if ((ch->followers || ch->has_master())
		&& die_follower(ch))
	{
		// TODO: странно все это с пуржем в stop_follower
		return;
	}

//	log("[Extract char] Stop fighting self");
	if (ch->get_fighting())
	{
		stop_fighting(ch, TRUE);
	}

//	log("[Extract char] Stop all fight for opponee");
	change_fighting(ch, TRUE);

//	log("[Extract char] Remove char from room");
	char_from_room(ch);

	delete_from_tmp_char_list(ch);

	// pull the char from the list
	MOB_FLAGS(ch).set(MOB_DELETE);

	if (ch->desc && ch->desc->original)
	{
		do_return(ch, NULL, 0, 0);
	}

	const bool is_npc = IS_NPC(ch);
	if (!is_npc)
	{
//		log("[Extract char] All save for PC");
		check_auction(ch, NULL);
		ch->save_char();
		//удаляются рент-файлы, если только персонаж не ушел в ренту
		Crash_delete_crashfile(ch);
	}
	else
	{
//		log("[Extract char] All clear for NPC");
		if ((GET_MOB_RNUM(ch) > -1)
			&& !MOB_FLAGGED(ch, MOB_PLAYER_SUMMON))	// if mobile и не умертвие
		{
			mob_index[GET_MOB_RNUM(ch)].number--;
		}
	}

	bool left_in_game = false;
	if (!is_npc
		&& ch->desc != NULL)
	{
		STATE(ch->desc) = CON_MENU;
		SEND_TO_Q(MENU, ch->desc);
		if (!IS_NPC(ch) && RENTABLE(ch) && clear_objs)
		{
			do_entergame(ch->desc);
			left_in_game = true;
		}
	}

	if (!left_in_game)
	{
		character_list.remove(ch);
	}

	log("[Extract char] Stop function for char %s", name.c_str());
}

/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */

CHAR_DATA *get_player_vis(CHAR_DATA * ch, const char *name, int inroom)
{
	for (const auto& i : character_list)
	{
		if (IS_NPC(i))
			continue;
		if (!HERE(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		if (!CAN_SEE_CHAR(ch, i))
			continue;
		if (!isname(name, i->get_pc_name()))
		{
			continue;
		}

		return i.get();
	}

	return nullptr;
}

CHAR_DATA* get_player_pun(CHAR_DATA * ch, const char *name, int inroom)
{
	for (const auto& i : character_list)
	{
		if (IS_NPC(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		if (!isname(name, i->get_pc_name()))
		{
			continue;
		}
		return i.get();
	}

	return nullptr;
}

CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, const char *name)
{
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	// JE 7/18/94 :-) :-)
	if (!str_cmp(name, "self")
		|| !str_cmp(name, "me")
		|| !str_cmp(name, "я")
		|| !str_cmp(name, "меня")
		|| !str_cmp(name, "себя"))
	{
		return (ch);
	}

	// 0.<name> means PC with name
	strl_cpy(tmp, name, MAX_INPUT_LENGTH);

	const int number = get_number(&tmp);
	if (0 == number)
	{
		return get_player_vis(ch, tmp, FIND_CHAR_ROOM);
	}

	int j = 0;
	for (const auto i : world[ch->in_room]->people)
	{
		if (HERE(i) && CAN_SEE(ch, i)
			&& isname(tmp, i->get_pc_name()))
		{
			if (++j == number)
			{
				return i;
			}
		}
	}

	return NULL;
}

CHAR_DATA *get_char_vis(CHAR_DATA * ch, const char *name, int where)
{
	CHAR_DATA *i;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	// check the room first
	if (where == FIND_CHAR_ROOM)
	{
		return get_char_room_vis(ch, name);
	}
	else if (where == FIND_CHAR_WORLD)
	{
 		if ((i = get_char_room_vis(ch, name)) != NULL)
		{
			return (i);
		}

		strcpy(tmp, name);
		const int number = get_number(&tmp);
		if (0 == number)
		{
			return get_player_vis(ch, tmp, 0);
		}

		int j = 0;
		for (const auto& i : character_list)
		{
			if (HERE(i) && CAN_SEE(ch, i)
				&& isname(tmp, i->get_pc_name()))
			{
				if (++j == number)
				{
					return i.get();
				}
			}
		}
	}

	return nullptr;
}

OBJ_DATA* get_obj_in_list_vis(CHAR_DATA * ch, const char *name, OBJ_DATA * list, bool locate_item)
{
	OBJ_DATA *i;
	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (NULL);

	//Запретим локейт 2. 3. n. стафин
	if (number > 1 && locate_item)
		return (NULL);

	for (i = list; i && (j <= number); i = i->get_next_content())
	{
		if (isname(tmp, i->get_aliases())
			|| CHECK_CUSTOM_LABEL(tmp, i, ch))
		{
			if (CAN_SEE_OBJ(ch, i))
			{
				// sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
				// send_to_char(buf,ch);
				if (!locate_item)
				{
					if (++j == number)
						return (i);
				}
				else
				{
					if (try_locate_obj(ch,i))
						return (i);
					else
						continue;
				}
			}
		}
	}

	return (NULL);
}

class ExitLoopException : std::exception {};

OBJ_DATA* get_obj_vis_and_dec_num(CHAR_DATA* ch, const char* name, OBJ_DATA* list, std::unordered_set<unsigned int>& id_obj_set, int& number)
{
	for (auto item = list; item != nullptr; item = item->get_next_content())
	{
		if (CAN_SEE_OBJ(ch, item))
		{
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch))
			{
				if (--number == 0)
				{
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

OBJ_DATA* get_obj_vis_and_dec_num(CHAR_DATA* ch, const char* name, OBJ_DATA* equip[], std::unordered_set<unsigned int>& id_obj_set, int& number)
{
	for (auto i = 0; i < NUM_WEARS; ++i)
	{
		auto item = equip[i];
		if (item && CAN_SEE_OBJ(ch, item))
		{
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch))
			{
				if (--number == 0)
				{
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

// search the entire world for an object, and return a pointer
OBJ_DATA* get_obj_vis(CHAR_DATA* ch, const char* name)
{
	int number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number < 1)
	{
		return nullptr;
	}

	auto id_obj_set = std::unordered_set<unsigned int>();

	//Scan in equipment
	auto obj = get_obj_vis_and_dec_num(ch, tmp, ch->equipment, id_obj_set, number);
	if (obj)
	{
		return obj;
	}

	//Scan in carried items
	obj = get_obj_vis_and_dec_num(ch, tmp, ch->carrying, id_obj_set, number);
	if (obj)
	{
		return obj;
	}

	//Scan in room
	obj = get_obj_vis_and_dec_num(ch, tmp, world[ch->in_room]->contents, id_obj_set, number);
	if (obj)
	{
		return obj;
	}
	
	//Scan charater's in room
	for (const auto& vict : world[ch->in_room]->people)
	{
		if (ch->get_uid() == vict->get_uid())
		{
			continue;
		}

		//Scan in equipment
		obj = get_obj_vis_and_dec_num(ch, tmp, vict->equipment, id_obj_set, number);
		if (obj)
		{
			return obj;
		}

		//Scan in carried items
		obj = get_obj_vis_and_dec_num(ch, tmp, vict->carrying, id_obj_set, number);
		if (obj)
		{
			return obj;
		}
	}

	// ok.. no luck yet. scan the entire obj list except already found
	const WorldObjects::predicate_f predicate = [&](const OBJ_DATA::shared_ptr& i) -> bool
	{
		const auto result = CAN_SEE_OBJ(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& (id_obj_set.count(i.get()->get_id()) == 0);
		return result;
	};
	
	return world_objects.find_if(predicate, number - 1).get();
}

// search the entire world for an object, and return a pointer
OBJ_DATA *get_obj_vis_for_locate(CHAR_DATA * ch, const char *name)
{
	OBJ_DATA *i;
	int number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;

	// scan items carried //
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
	{
		return i;
	}

	// scan room //
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) != NULL)
	{
		return i;
	}

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number != 1)
	{
		return nullptr;
	}

	// ok.. no luck yet. scan the entire obj list   //
	const WorldObjects::predicate_f locate_predicate = [&](const OBJ_DATA::shared_ptr& i) -> bool
	{
		const auto result = CAN_SEE_OBJ(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& try_locate_obj(ch, i.get());
		return result;
	};

	return world_objects.find_if(locate_predicate).get();
}

bool try_locate_obj(CHAR_DATA * ch, OBJ_DATA *i)
{
	if (IS_CORPSE(i) || IS_GOD(ch)) //имм может локейтить и можно локейтить трупы
	{
		return true;
	}
	else if (OBJ_FLAGGED(i, EExtraFlag::ITEM_NOLOCATE)) //если флаг !локейт и ее нет в комнате/инвентаре - пропустим ее
	{
		return false;
	}
	else if (i->get_carried_by() && IS_NPC(i->get_carried_by()))
	{
		if (world[IN_ROOM(i->get_carried_by())]->zone == world[ch->in_room]->zone) //шмотки у моба можно локейтить только в одной зоне
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (i->get_in_room() != NOWHERE && i->get_in_room())
	{
		if (world[i->get_in_room()]->zone == world[ch->in_room]->zone) //шмотки в клетке можно локейтить только в одной зоне
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (i->get_worn_by() && IS_NPC(i->get_worn_by()))
	{
		if (world[IN_ROOM(i->get_worn_by())]->zone == world[ch->in_room]->zone)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (i->get_in_obj())
	{
		if (Clan::is_clan_chest(i->get_in_obj()))
		{
			return true;
		}
		else
		{
			const auto in_obj = i->get_in_obj();
			if (in_obj->get_carried_by())
			{
				if (IS_NPC(in_obj->get_carried_by()))
				{
					if (world[IN_ROOM(in_obj->get_carried_by())]->zone == world[ch->in_room]->zone)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
				else
				{
					return true;
				}
			}
			else if (in_obj->get_in_room() != NOWHERE && in_obj->get_in_room())
			{
				if (world[in_obj->get_in_room()]->zone == world[ch->in_room]->zone)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (in_obj->get_worn_by())
			{
				const auto worn_by = i->get_in_obj()->get_worn_by();
				if (IS_NPC(worn_by))
				{
					if (world[worn_by->in_room]->zone == world[ch->in_room]->zone)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
				else
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
	}
	else
	{
		return true;
	}
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
	{
		if (equipment[(*j)])
		{
			if (CAN_SEE_OBJ(ch, equipment[(*j)]))
			{
				if (isname(tmp, equipment[(*j)]->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, equipment[(*j)], ch))
				{
					if (++l == number)
					{
						return equipment[(*j)];
					}
				}
			}
		}
	}

	return (NULL);
}

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

OBJ_DATA::shared_ptr create_money(int amount)
{
	int i;
	char buf[200];

	if (amount <= 0)
	{
		log("SYSERR: Try to create negative or 0 money. (%d)", amount);
		return (NULL);
	}
	auto obj = world_objects.create_blank();
	EXTRA_DESCR_DATA::shared_ptr new_descr(new EXTRA_DESCR_DATA());

	if (amount == 1)
	{
		sprintf(buf, "coin gold кун деньги денег монет %s", money_desc(amount, 0));
		obj->set_aliases(buf);
		obj->set_short_description("куна");
		obj->set_description("Одна куна лежит здесь.");
		new_descr->keyword = str_dup("coin gold монет кун денег");
		new_descr->description = str_dup("Всего лишь одна куна.");
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++)
		{
			obj->set_PName(i, money_desc(amount, i));
		}
	}
	else
	{
		sprintf(buf, "coins gold кун денег %s", money_desc(amount, 0));
		obj->set_aliases(buf);
		obj->set_short_description(money_desc(amount, 0));
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++)
		{
			obj->set_PName(i, money_desc(amount, i));
		}

		sprintf(buf, "Здесь лежит %s.", money_desc(amount, 0));
		obj->set_description(CAP(buf));

		new_descr->keyword = str_dup("coins gold кун денег");
	}

	new_descr->next = NULL;
	obj->set_ex_description(new_descr);

	obj->set_type(OBJ_DATA::ITEM_MONEY);
	obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));
	obj->set_sex(ESex::SEX_FEMALE);
	obj->set_val(0, amount);
	obj->set_cost(amount);
	obj->set_maximum_durability(OBJ_DATA::DEFAULT_MAXIMUM_DURABILITY);
	obj->set_current_durability(OBJ_DATA::DEFAULT_CURRENT_DURABILITY);
	obj->set_timer(24 * 60 * 7);
	obj->set_weight(1);
	obj->set_extra_flag(EExtraFlag::ITEM_NODONATE);
	obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);

	return obj;
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
				if (isname(tmp, GET_EQ(ch, l)->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, GET_EQ(ch, l), ch)
					|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
						&& find_exdesc(tmp, GET_EQ(ch, l)->get_ex_description())))
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
		for (i = ch->carrying; i && (j <= number); i = i->get_next_content())
		{
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
					&& find_exdesc(tmp, i->get_ex_description())))
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
			i && (j <= number); i = i->get_next_content())
		{
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
					&& find_exdesc(tmp ,i->get_ex_description())))
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

	return (0);
}

// a function to scan for "all" or "all.x"
int find_all_dots(char *arg)
{
	char tmpname[MAX_INPUT_LENGTH];

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
	{
		return (FIND_ALL);
	}
	else if (!strn_cmp(arg, "all.", 4) || !strn_cmp(arg, "все.", 4))
	{
		strl_cpy(tmpname, arg + 4, MAX_INPUT_LENGTH);
		strl_cpy(arg, tmpname, MAX_INPUT_LENGTH);
		return (FIND_ALLDOT);
	}
	else
	{
		return (FIND_INDIV);
	}
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

	CREATE(tmp, 1);
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

//Функции для модифицированного чарма
float get_damage_per_round(CHAR_DATA * victim)
{
	float dam_per_attack = GET_DR(victim) + str_bonus(victim->get_str(), STR_TO_DAM)
			+ victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2.0
			+ (AFF_FLAGGED(victim, EAffectFlag::AFF_CLOUD_OF_ARROWS) ? 14 : 0);
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

float get_effective_cha(CHAR_DATA * ch)
{
	int key_value, key_value_add;

	key_value = ch->get_cha();
	auto max_cha = class_stats_limit[ch->get_class()][5];
	key_value_add = MIN(max_cha - ch->get_cha(), GET_CHA_ADD(ch));

	float eff_cha = 0.0;
	if (GET_LEVEL(ch) <= 14)
	{
		eff_cha = key_value
			- 6 * (float)(14 - GET_LEVEL(ch)) / 13.0 + key_value_add
			* (0.2 + 0.3 * (float)(GET_LEVEL(ch) - 1) / 13.0);
	}
	else if (GET_LEVEL(ch) <= 26)
	{
		eff_cha = key_value + key_value_add * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
	}
	else
	{
		eff_cha = key_value + key_value_add;
	}

	return VPOSI<float>(eff_cha, 1.0f, static_cast<float>(max_cha));
}

float get_effective_wis(CHAR_DATA * ch, int spellnum)
{
	int key_value, key_value_add;

	auto max_wis = class_stats_limit[ch->get_class()][3];

	if (spellnum == SPELL_RESSURECTION || spellnum == SPELL_ANIMATE_DEAD)
	{
		key_value = ch->get_wis();
		key_value_add = MIN(max_wis - ch->get_wis(), GET_WIS_ADD(ch));
	}
	else
	{
		//если гдето вылезет косяком
		key_value = 0;
		key_value_add = 0;
	}

	float eff_wis = 0.0;
	if (GET_LEVEL(ch) <= 14)
	{
		eff_wis = key_value
			- 6 * (float)(14 - GET_LEVEL(ch)) / 13.0 + key_value_add
			* (0.4 + 0.6 * (float)(GET_LEVEL(ch) - 1) / 13.0);
	}
	else if (GET_LEVEL(ch) <= 26)
	{
		eff_wis = key_value + key_value_add * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
	}
	else
	{
		eff_wis = key_value + key_value_add;
	}

	return VPOSI<float>(eff_wis, 1.0f, static_cast<float>(max_wis));
}

float get_effective_int(CHAR_DATA * ch)
{
	int key_value, key_value_add;

	key_value = ch->get_int();
	auto max_int = class_stats_limit[ch->get_class()][4];
	key_value_add = MIN(max_int - ch->get_int(), GET_INT_ADD(ch));

	float eff_int = 0.0;
	if (GET_LEVEL(ch) <= 14)
	{
		eff_int = key_value
			- 6 * (float)(14 - GET_LEVEL(ch)) / 13.0 + key_value_add
			* (0.2 + 0.3 * (float)(GET_LEVEL(ch) - 1) / 13.0);
	}
	else if (GET_LEVEL(ch) <= 26)
	{
		eff_int = key_value + key_value_add * (0.5 + 0.5 * (float)(GET_LEVEL(ch) - 14) / 12.0);
	}
	else
	{
		eff_int = key_value + key_value_add;
	}

	return VPOSI<float>(eff_int, 1.0f, static_cast<float>(max_int));
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
	return VPOSI<float>(needed_cha, 1.0, 50.0);
}

int calc_hire_price(CHAR_DATA * ch, CHAR_DATA * victim)
{
	float needed_cha = calc_cha_for_hire(victim), dpr = 0.0;
	float e_cha = get_effective_cha(ch), e_int = get_effective_int(ch);
	float stat_overlimit = VPOSI<float>(e_cha + e_int - 1.0 -
								 min_stats2[(int) GET_CLASS(ch)][5] - 1 -
								 min_stats2[(int) GET_CLASS(ch)][2], 0, 100);

	float price = 0;
	float real_cha = 1.0 + GET_LEVEL(ch) / 2.0 + stat_overlimit / 2.0;
	float difference = needed_cha - real_cha;

	dpr = get_damage_per_round(victim);

	log("MERCHANT: hero (%s) mob (%s [%5d] ) charm (%f) dpr (%f)",GET_NAME(ch),GET_NAME(victim),GET_MOB_VNUM(victim),needed_cha,dpr);

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
	float max_cha;

	if (spellnum == SPELL_RESSURECTION || spellnum == SPELL_ANIMATE_DEAD)
	{
            eff_cha = get_effective_wis(ch, spellnum);
			max_cha = class_stats_limit[ch->get_class()][3];
	}
	else
	{
			max_cha = class_stats_limit[ch->get_class()][5];
            eff_cha = get_effective_cha(ch);
	}

	if (spellnum != SPELL_CHARM)
	{
		eff_cha = MMIN(max_cha, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
	}

	if (eff_cha < max_cha)
	{
		r_hp = (1 - eff_cha + (int)eff_cha) * cha_app[(int)eff_cha].charms +
			(eff_cha - (int)eff_cha) * cha_app[(int)eff_cha + 1].charms;
	}
	else
	{
		r_hp = (1 - eff_cha + (int)eff_cha) * cha_app[(int)eff_cha].charms;
	}

	return (int) r_hp;
}

int get_reformed_charmice_hp(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum)
{
	float r_hp = 0;
	float eff_cha = 0.0;
	float max_cha;
	
	if (spellnum == SPELL_RESSURECTION || spellnum == SPELL_ANIMATE_DEAD)
	{
            eff_cha = get_effective_wis(ch, spellnum);
			max_cha = class_stats_limit[ch->get_class()][3];
	}
	else
	{
			max_cha = class_stats_limit[ch->get_class()][5];
            eff_cha = get_effective_cha(ch);
	}

	if (spellnum != SPELL_CHARM)
	{
		eff_cha = MMIN(max_cha, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
	}

	// Интерполяция между значениями для целых значений обаяния
	if (eff_cha < max_cha)
	{
		r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
			((1 - eff_cha + (int)eff_cha) * cha_app[(int)eff_cha].dam_to_hit_rate +
				(eff_cha - (int)eff_cha) * cha_app[(int)eff_cha + 1].dam_to_hit_rate);
	}
	else
	{
		r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
			((1 - eff_cha + (int)eff_cha) * cha_app[(int)eff_cha].dam_to_hit_rate);
	}

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
	CREATE(i, 1);
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
		if (!IS_SET(GET_SPELL_TYPE(ch, i), SPELL_KNOW | SPELL_TEMP))
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

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return (FALSE);
	if (IS_GOD(ch))
		return (FALSE);

	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i)
			&& ObjSystem::is_armor_type(GET_EQ(ch, i))
			&& GET_OBJ_MATER(GET_EQ(ch, i)) <= OBJ_DATA::MAT_COLOR)
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
	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return (FALSE);

	if (IS_GOD(ch))
		return (FALSE);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STAIRS) ||
			AFF_FLAGGED(ch, EAffectFlag::AFF_SANCTUARY) || AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT))
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
		const auto aff = find_room_affect(label_room, SPELL_RUNE_LABEL);
		if (aff != label_room->affected.end())
		{
			affect_room_remove(label_room, aff);
			send_to_char("Ваша рунная метка удалена.\r\n", ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
