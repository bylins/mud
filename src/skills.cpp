/*************************************************************************
*   File: skills.cpp                                   Part of Bylins    *
*   Skills functions here                                                *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"
#include "im.h"
#include "features.hpp"
#include "random.hpp"
#include "char.hpp"
#include "room.hpp"

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
#define DUMMY_KNIGHT 390
#define DUMMY_SHIELD 391
#define DUMMY_WEAPON 392

// Хз откуда :(

extern struct message_list fight_messages[MAX_MESSAGES];

static const char *kick_type[] =
// силы пинка. полностью соответствуют наносимым поврждениям обычного удара
{
	"легонько ",		//  1..5
	"слегка ",		// 6..11
	"",			// 12..26
	"сильно ",		// 27..35
	"очень сильно ",	// 36..45
	"чрезвычайно сильно ",	// 46..55
	"БОЛЬНО ",		// 56..96
	"ОЧЕНЬ БОЛЬНО ",	// 97..136
	"ЧРЕЗВЫЧАЙНО БОЛЬНО ",	// 137..176
	"НЕВЫНОСИМО БОЛЬНО ",	// 177..216
	"УЖАСНО ",	// 217..256
	"УБИЙСТВЕННО ",	// 257..296
	"СМЕРТЕЛЬНО " // 297+
};

struct brief_shields
{
	brief_shields(CHAR_DATA* ch_, CHAR_DATA* vict_, OBJ_DATA* weap_, std::string add_)
		: ch(ch_), vict(vict_), weap(weap_), add(add_), reflect(false ) {};

	void act_to_char(const char *msg)
	{
		if (!reflect || (reflect && !PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS)))
		{
			if (!add.empty() && PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS))
			{
				act_add(msg, TO_CHAR);
			}
			else
			{
				act_no_add(msg, TO_CHAR);
			}
		}
	}
	void act_to_vict(const char *msg)
	{
		if (!reflect || (reflect && !PRF_FLAGGED(vict, PRF_BRIEF_SHIELDS)))
		{
			if (!add.empty() && PRF_FLAGGED(vict, PRF_BRIEF_SHIELDS))
			{
				act_add(msg, TO_VICT | TO_SLEEP);
			}
			else
			{
				act_no_add(msg, TO_VICT | TO_SLEEP);
			}
		}
	}
	void act_to_room(const char *msg)
	{
		if (add.empty())
		{
			if (reflect)
			{
				act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
			}
			else
			{
				act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN);
			}
		}
		else
		{
			act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
			if (!reflect)
			{
				act_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_BRIEF_SHIELDS);
			}
		}
	}

	CHAR_DATA* ch;
	CHAR_DATA* vict;
	OBJ_DATA* weap;
	std::string add;
	// флаг отражаемого дамага, который надо глушить в режиме PRF_BRIEF_SHIELDS
	bool reflect;

private:
	void act_no_add(const char *msg, int type)
	{
		act(msg, FALSE, ch, weap, vict, type);
	}
	void act_add(const char *msg, int type)
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s", msg, add.c_str());
		act(buf_, FALSE, ch, weap, vict, type);
	}
};

OBJ_DATA* init_weap(CHAR_DATA *ch, int dam, int attacktype)
{
	OBJ_DATA* weap = 0;
	int weap_i = 0;

	switch (attacktype)
	{
	case SKILL_BACKSTAB + TYPE_HIT:
		if (!(weap = GET_EQ(ch, WEAR_WIELD))
				&& (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
			weap = obj_proto[weap_i];
		break;
	case SKILL_THROW + TYPE_HIT:
		if (!(weap = GET_EQ(ch, WEAR_WIELD))
				&& (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
			weap = obj_proto[weap_i];
		break;
	case SKILL_BASH + TYPE_HIT:
		if (!(weap = GET_EQ(ch, WEAR_SHIELD))
				&& (weap_i = real_object(DUMMY_SHIELD)) >= 0)
			weap = obj_proto[weap_i];
		break;
	case SKILL_KICK + TYPE_HIT:
		// weap - текст силы удара
		if (dam <= 5)
			weap = (OBJ_DATA *) kick_type[0];
		else if (dam <= 11)
			weap = (OBJ_DATA *) kick_type[1];
		else if (dam <= 26)
			weap = (OBJ_DATA *) kick_type[2];
		else if (dam <= 35)
			weap = (OBJ_DATA *) kick_type[3];
		else if (dam <= 45)
			weap = (OBJ_DATA *) kick_type[4];
		else if (dam <= 56)
			weap = (OBJ_DATA *) kick_type[5];
		else if (dam <= 96)
			weap = (OBJ_DATA *) kick_type[6];
		else if (dam <= 136)
			weap = (OBJ_DATA *) kick_type[7];
		else if (dam <= 176)
			weap = (OBJ_DATA *) kick_type[8];
		else if (dam <= 216)
			weap = (OBJ_DATA *) kick_type[9];
		else if (dam <= 256)
			weap = (OBJ_DATA *) kick_type[10];
		else if (dam <= 296)
			weap = (OBJ_DATA *) kick_type[11];
		else
			weap = (OBJ_DATA *) kick_type[12];
		break;
	case TYPE_HIT:
		weap = 0;
		break;
	default:
		if (!weap && (weap_i = real_object(DUMMY_WEAPON)) >= 0)
			weap = obj_proto[weap_i];
	}

	return weap;
}

///
/// \param add = "", строка для добавления после основного сообщения (краткий режим щитов)
///
int skill_message(int dam, CHAR_DATA * ch, CHAR_DATA * vict, int attacktype, std::string add)
{
	int i, j, nr;
	struct message_type *msg;
	OBJ_DATA *weap = GET_EQ(ch, WEAR_WIELD) ?
		GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch, WEAR_BOTHS);

	// log("[SKILL MESSAGE] Message for skill %d",attacktype);
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		if (fight_messages[i].a_type == attacktype)
		{
			nr = dice(1, fight_messages[i].number_of_attacks);
			// log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;

			weap = init_weap(ch, dam, attacktype);
			brief_shields brief(ch, vict, weap, add);
			if (attacktype == SPELL_FIRE_SHIELD
				|| attacktype == SPELL_MAGICGLASS)
			{
				brief.reflect = true;
			}

			if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT))
			{
				switch (attacktype)
				{
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char("&W&q", ch);
					break;
				default:
					send_to_char("&y&q", ch);
					break;
				}
				// ch
				brief.act_to_char(msg->god_msg.attacker_msg);
				send_to_char("&Q&n", ch);
				// victim
				brief.act_to_vict(msg->god_msg.victim_msg);
				// room
				brief.act_to_room(msg->god_msg.room_msg);
			}
			else if (dam != 0)
			{
				if (GET_POS(vict) == POS_DEAD)
				{
					// ch
					send_to_char("&Y&q", ch);
					brief.act_to_char(msg->die_msg.attacker_msg);
					send_to_char("&Q&n", ch);
					// vict
					send_to_char("&R&q", vict);
					brief.act_to_vict(msg->die_msg.victim_msg);
					send_to_char("&Q&n", vict);
					// room
					brief.act_to_room(msg->die_msg.room_msg);
				}
				else
				{
					// ch
					send_to_char("&Y&q", ch);
					brief.act_to_char(msg->hit_msg.attacker_msg);
					send_to_char("&Q&n", ch);
					// vict
					send_to_char("&R&q", vict);
					brief.act_to_vict(msg->hit_msg.victim_msg);
					send_to_char("&Q&n", vict);
					// room
					brief.act_to_room(msg->hit_msg.room_msg);
				}
			}
			else if (ch != vict)  	// Dam == 0
			{
				switch (attacktype)
				{
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char("&W&q", ch);
					break;
				default:
					send_to_char("&y&q", ch);
					break;
				}
				//ch
				brief.act_to_char(msg->miss_msg.attacker_msg);
				send_to_char("&Q&n", ch);
				// vict
				send_to_char("&r&q", vict);
				brief.act_to_vict(msg->miss_msg.victim_msg);
				send_to_char("&Q&n", vict);
				// room
				brief.act_to_room(msg->miss_msg.room_msg);
			}
			return (1);
		}
	}
	return (0);
}

// *** This function return chance of skill
int calculate_skill(CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict)
{
	int skill_is, percent = 0, victim_sav = SAVING_REFLEX, victim_modi = 0;
	int morale;
	bool pass_mod = 0; // в данный момент для доп.выстрела, чтобы оставить его как скилл,
	// но не применять к нему левых штрафов и плюсов, плюсуется только от инты немного

	if (skill_no < 1 || skill_no > MAX_SKILL_NUM)  	// log("ERROR: ATTEMPT USING UNKNOWN SKILL <%d>", skill_no);
	{
		return 0;
	}
	if ((skill_is = ch->get_skill(skill_no)) <= 0)
	{
		return 0;
	}

	skill_is += int_app[GET_REAL_INT(ch)].to_skilluse;
	switch (skill_no)
	{
	case SKILL_HIDETRACK:
		percent = skill_is + (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);
		break;
	case SKILL_BACKSTAB:	//заколоть
		victim_sav = SAVING_REFLEX;
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch)) * 2;
		if (awake_others(ch) || equip_in_metall(ch))
			percent -= 50;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				percent += 25;
			if (GET_POS(vict) < POS_FIGHTING)
				percent += (20 * (POS_FIGHTING - GET_POS(vict)));
			else if (AFF_FLAGGED(vict, AFF_AWARNESS))
				victim_modi -= 30;
			victim_modi += size_app[GET_POS_SIZE(vict)].ac;
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
		}
		break;
	case SKILL_BASH:	//сбить
		victim_sav = SAVING_REFLEX;
		percent = skill_is +
				  size_app[GET_POS_SIZE(ch)].interpolate +
				  dex_bonus(GET_REAL_DEX(ch)) +
				  dex_bonus(GET_REAL_STR(ch)) +
				  (GET_EQ(ch, WEAR_SHIELD) ?
				   weapon_app[MIN(35, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD))))].
				   bashing : 0);
		if (vict)
		{
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
			if (GET_POS(vict) < POS_FIGHTING && GET_POS(vict) > POS_SLEEPING)
				victim_modi -= 20;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
			victim_modi -= GET_REAL_CON(vict);
		}
		break;
	case SKILL_HIDE:	//спрятаться
		percent =
			skill_is + dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac
			+ (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			percent -= 50;

		if (IS_DARK(IN_ROOM(ch)))
			percent += 25;

		if (SECT(IN_ROOM(ch)) == SECT_INSIDE)
			percent += 20;
		else if (SECT(IN_ROOM(ch)) == SECT_CITY)
			percent += 15;
		else if (SECT(IN_ROOM(ch)) == SECT_FOREST)
			percent -= 20;
		else if (SECT(IN_ROOM(ch)) == SECT_HILLS
				 || SECT(IN_ROOM(ch)) == SECT_MOUNTAIN) percent -= 10;

		if (vict)
		{
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_KICK:	//пнуть
		victim_sav = SAVING_STABILITY;
		percent = skill_is +
				  dex_bonus(GET_REAL_DEX(ch)) +
				  dex_bonus(GET_REAL_STR(ch));
		if (vict)
		{
			victim_modi += size_app[GET_POS_SIZE(vict)].interpolate;
			victim_modi += GET_REAL_CON(vict);
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
		}
		break;
	case SKILL_PICK_LOCK:	//pick lock
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch))
				  + (can_use_feat(ch, NIMBLE_FINGERS_FEAT) ? 5 : 0);
		break;
	case SKILL_PUNCH:	//punch
		percent = skill_is;
		break;
	case SKILL_RESCUE:	//спасти
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch));
		victim_modi = 100;
		break;
	case SKILL_SNEAK:	//sneak
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch))
				  + (can_use_feat(ch, STEALTHY_FEAT) ? 10 : 0);

		if (awake_others(ch)||equip_in_metall(ch))
			percent -= 50;

		if (SECT(IN_ROOM(ch)) == SECT_CITY)
			percent -= 10;
		if (IS_DARK(IN_ROOM(ch)))
			percent += 20;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				victim_modi += 25;
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
		}
		break;
	case SKILL_STEAL:	//steal
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch))
				  + (can_use_feat(ch, NIMBLE_FINGERS_FEAT) ? 5 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			percent -= 50;

		if (IS_DARK(IN_ROOM(ch)))
			percent += 20;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				victim_modi += 25;
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				if (AFF_FLAGGED(vict, AFF_AWARNESS))
					victim_modi -= 30;
			}
		}
		break;
	case SKILL_TRACK:	//выследить
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation
				  + (can_use_feat(ch, TRACKER_FEAT) ? 10 : 0);

		if (SECT(IN_ROOM(ch)) == SECT_FOREST || SECT(IN_ROOM(ch)) == SECT_FIELD)
			percent += 10;

		percent =
			complex_skill_modifier(ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS, percent);

		if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM ||
				SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM ||
				SECT(IN_ROOM(ch)) == SECT_FLYING ||
				SECT(IN_ROOM(ch)) == SECT_UNDERWATER ||
				SECT(IN_ROOM(ch)) == SECT_SECRET
				|| ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) percent = 0;


		if (vict)
		{
			victim_modi += GET_REAL_CON(vict) / 2;
			if (AFF_FLAGGED(vict, AFF_NOTRACK)
					|| ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) victim_modi = -100;
		}
		break;

	case SKILL_SENSE:
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation
				  + (can_use_feat(ch, TRACKER_FEAT) ? 10 : 0);

		percent =
			complex_skill_modifier(ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS, percent);

		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
			percent = 0;

		if (vict)
		{
			victim_modi += GET_REAL_CON(vict) / 2;
			if (AFF_FLAGGED(vict, AFF_NOTRACK)
					|| ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) victim_modi = -100;
		}
		break;
	case SKILL_MULTYPARRY:
	case SKILL_PARRY:	//парировать
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch));
		if (GET_AF_BATTLE(ch, EAF_AWAKE))
			percent += ch->get_skill(SKILL_AWAKE);

		if (GET_EQ(ch, WEAR_HOLD)
				&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON)
		{
			percent +=
				weapon_app[MAX
						   (0,
							MIN(50,
								GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))))].
				parrying;
		}
		victim_modi = 100;
		break;

	case SKILL_BLOCK:	//закрыться щитом
	{
		// по 10 бонусом со щита (21-30) и дексы (21-50)
		int shield_mod = GET_EQ(ch, WEAR_SHIELD) ? MIN(10, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD)) - 20)) : 0;
		int dex_mod = MAX(0, (GET_REAL_DEX(ch) - 20)/3);
		percent = skill_is + dex_mod + shield_mod;

		break;
	}

	case SKILL_TOUCH:	//захватить противника
		percent =
			skill_is + dex_bonus(GET_REAL_DEX(ch)) +
			size_app[GET_POS_SIZE(vict)].interpolate;

		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
		}
		break;

	case SKILL_PROTECT:	//прикрыть грудью
		percent =
			skill_is + dex_bonus(GET_REAL_DEX(ch)) +
			size_app[GET_POS_SIZE(ch)].interpolate;

		victim_modi = 100;
		break;

	case SKILL_BOWS:	//луки
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch));
		break;
	case SKILL_BOTHHANDS:	//двуручники
	case SKILL_LONGS:	//длинные лезвия
	case SKILL_SPADES:	//копья и пики
	case SKILL_SHORTS:	//короткие лезвия
	case SKILL_CLUBS:	//палицы и дубины
	case SKILL_PICK:	//проникающее
	case SKILL_NONSTANDART:	//разнообразное оружие
	case SKILL_AXES:	//секиры
		percent = skill_is;
		break;
	case SKILL_SATTACK:	//атака второй рукой
		percent = skill_is;
		break;
	case SKILL_LOOKING:	//приглядеться
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
		break;
	case SKILL_HEARING:	//прислушаться
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
		break;
	case SKILL_DISARM:
		victim_sav = SAVING_REFLEX;
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(vict, WEAR_BOTHS))
				victim_modi -= 10;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
		}
		break;
	case SKILL_HEAL:
		percent = skill_is;
		break;
	case SKILL_ADDSHOT:
		percent = skill_is;
		if (equip_in_metall(ch))
			percent -= 5;
		pass_mod = 1;
		break;
	case SKILL_NOPARRYHIT:
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch));
		break;
	case SKILL_CAMOUFLAGE:
		percent = skill_is + dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac
				  + (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);

		if (awake_others(ch))
			percent -= 100;

		if (IS_DARK(IN_ROOM(ch)))
			percent += 15;

		if (SECT(IN_ROOM(ch)) == SECT_CITY)
			percent -= 15;
		else if (SECT(IN_ROOM(ch)) == SECT_FOREST)
			percent += 10;
		else if (SECT(IN_ROOM(ch)) == SECT_HILLS
				 || SECT(IN_ROOM(ch)) == SECT_MOUNTAIN) percent += 5;
		if (equip_in_metall(ch))
			percent -= 30;

		if (vict)
		{
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_DEVIATE:
		percent =
			skill_is - size_app[GET_POS_SIZE(ch)].ac +
			dex_bonus(GET_REAL_DEX(ch));

		if (equip_in_metall(ch))
			percent -= 40;

		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
		}
		break;
	case SKILL_CHOPOFF:
		victim_sav = SAVING_REFLEX;
		percent = skill_is +
				  dex_bonus(GET_REAL_DEX(ch)) + size_app[GET_POS_SIZE(ch)].ac;

		if (equip_in_metall(ch))
			percent -= 10;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				percent += 10;
			if (GET_POS(vict) < POS_SITTING)
				percent -= 50;
			if (AWAKE(vict) && AFF_FLAGGED(vict, AFF_AWARNESS))
				victim_modi -= 30;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_REPAIR:
		percent = skill_is;
		break;
	case SKILL_UPGRADE:
		percent = skill_is;
	case SKILL_WARCRY:
		percent = skill_is;
		break;
	case SKILL_COURAGE:
		percent = skill_is;
		break;
	case SKILL_SHIT:
		percent = skill_is;
		break;
	case SKILL_MIGHTHIT:
		victim_sav = SAVING_STABILITY;
		percent =
			skill_is + size_app[GET_POS_SIZE(ch)].shocking +
			str_bonus(GET_REAL_STR(ch), STR_TO_DAM);

		if (vict)
		{
			victim_modi -= size_app[GET_POS_SIZE(vict)].shocking;
		}
		break;
	case SKILL_STUPOR:
		victim_sav = SAVING_STABILITY;
		percent = skill_is + str_bonus(GET_REAL_STR(ch), STR_TO_DAM) * 2;
		if (GET_EQ(ch, WEAR_WIELD))
			percent +=
				weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))].shocking;
		else if (GET_EQ(ch, WEAR_BOTHS))
			percent +=
				weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))].shocking;

		if (vict)
		{
			victim_modi += - GET_REAL_CON(vict);
		}
		break;
	case SKILL_POISONED:
		percent = skill_is;
		break;
	case SKILL_LEADERSHIP:
		percent = skill_is + cha_app[GET_REAL_CHA(ch)].leadership;
		break;
	case SKILL_PUNCTUAL:
		victim_sav = SAVING_CRITICAL;
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
		if (GET_EQ(ch, WEAR_WIELD))
			percent += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 18
					   + MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 25
					   + MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 30;
		if (GET_EQ(ch, WEAR_HOLD))
			percent += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 18
					   + MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 25
					   + MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 30;
		if (GET_EQ(ch, WEAR_BOTHS))
			percent += MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))) - 25
					   + MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))) - 30;
		if (vict)
		{
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_AWAKE:
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;

		if (vict)
		{
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;

	case SKILL_IDENTIFY:
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation
				  + (can_use_feat(ch, CONNOISEUR_FEAT) ? 20 : 0);
		break;

	case SKILL_CREATE_POTION:
	case SKILL_CREATE_SCROLL:
	case SKILL_CREATE_WAND:
		percent = skill_is;
		break;
	case SKILL_LOOK_HIDE:
		percent = skill_is + cha_app[GET_REAL_CHA(ch)].illusive;
		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				percent += 50;
			else if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(ch)].observation;
		}
		break;
	case SKILL_ARMORED:
		percent = skill_is;
		break;
	case SKILL_DRUNKOFF:
		percent = skill_is - GET_REAL_CON(ch) / 2
				  + (can_use_feat(ch, DRUNKARD_FEAT) ? 20 : 0);
		break;
	case SKILL_AID:
		percent = skill_is + (can_use_feat(ch, HEALER_FEAT) ? 10 : 0);
		break;
	case SKILL_FIRE:
		percent = skill_is;
		if (get_room_sky(IN_ROOM(ch)) == SKY_RAINING)
			percent -= 50;
		else if (get_room_sky(IN_ROOM(ch)) != SKY_LIGHTNING)
			percent -= number(10, 25);
	case SKILL_HORSE:
		percent = skill_is + cha_app[GET_REAL_CHA(ch)].leadership;
		break;
	case SKILL_TURN_UNDEAD:
		percent = skill_is + (can_use_feat(ch, EXORCIST_FEAT) ? 20 : 0);
		break;
	case SKILL_MORPH:
		percent = skill_is;
		break;
	case SKILL_STRANGLE:
		percent = skill_is;
	default:
		percent = skill_is;
		break;
	}

	// не все умения надо модифицировать из-за внешних факторов и морали
	if (!pass_mod)
	{
		percent = complex_skill_modifier(ch, skill_no, GAPPLY_SKILL_SUCCESS, percent);
		morale = ch->calc_morale();

		if (vict && percent > skill_info[skill_no].max_percent)
			victim_modi += percent - skill_info[skill_no].max_percent;

		if (AFF_FLAGGED(ch, AFF_DEAFNESS))
			morale -= 20;	// у глухого мораль на 20 меньше
		// Обработка способности "боевой дух"
		if (vict && can_use_feat(vict, SPIRIT_WARRIOR_FEAT))
			morale -= 3;

		// если мораль отрицательная, увеличивается вероятность, что умение не пройдет
		if ((skill_is = number(0, 99)) >= 95 + morale)
			percent = 0;
		else if (skill_is <= MIN(50, morale))
			percent = skill_info[skill_no].max_percent;
		else if (vict && general_savingthrow(ch, vict, victim_sav, victim_modi))
			percent = 0;
	}

	// иммские флаги и прокла влияют на все
	if (IS_IMMORTAL(ch))
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		percent = 0;
	else if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else
		percent = MIN(MAX(0, percent), max_value);

	return (percent);
}

void improove_skill(CHAR_DATA * ch, int skill_no, int success, CHAR_DATA * victim)
{
	const int trained_skill = ch->get_trained_skill(skill_no);
	if (trained_skill == 0 || trained_skill == 200)
	{
		// скила может и не быть, если получен только со шмоток
		return;
	}

	int skill_is, diff = 0, prob, div;

	if (IS_NPC(ch))
		return;

	if (victim && (IS_HORSE(victim) || MOB_FLAGGED(victim, MOB_NOTRAIN)))
		return;

	if (IS_IMMORTAL(ch) ||
			((!victim ||
			  OK_GAIN_EXP(ch, victim)) && IN_ROOM(ch) != NOWHERE
			 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
// Стрибог
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) &&
//Свентовит
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)
			 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM) &&
//
			 (diff =
				  wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * GET_LEVEL(ch) / 20 -
				  trained_skill) > 0
			 && trained_skill < MAX_EXP_RMRT_PERCENT(ch)))
	{
		// Success - multy by 2
		prob = success ? 20000 : 15000;

		div = int_app[GET_REAL_INT(ch)].improove /* + diff */ ;

		if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_CLASSES)
			div += (skill_info[skill_no].k_improove[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] / 100);

		prob /= (MAX(1, div));
		// вариант бонуса мудрости без штрафов за кол-во умений
		prob -= 5 * wis_bonus(GET_REAL_WIS(ch), WIS_MAX_SKILLS);
		prob += number(1, trained_skill * 5);

		skill_is = number(1, MAX(1, prob));

		// if (!IS_NPC(ch))
//        log("Player %s skill '%d' - need to improove %d(%d-%d)",
//            GET_NAME(ch), skill_no, skill_is, div, prob);

		if (
			(victim
			 && skill_is <= GET_REAL_INT(ch) * GET_LEVEL(victim) / GET_LEVEL(ch))
			|| (!victim && skill_is <= GET_REAL_INT(ch)))
		{
			if (success)
				sprintf(buf, "%sВы повысили уровень умения \"%s\".%s\r\n",
						CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch,
								C_NRM));
			else
				sprintf(buf,
						"%sПоняв свои ошибки, вы повысили уровень умения \"%s\".%s\r\n",
						CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch,
								C_NRM));
			send_to_char(buf, ch);
			ch->set_morphed_skill(skill_no, (trained_skill + number(1, 2)));
			if (!IS_IMMORTAL(ch))
				ch->set_morphed_skill(skill_no, (MIN(MAX_EXP_PERCENT + GET_REMORT(ch) * 5, ch->get_trained_skill(skill_no))));
// скилл прокачался, помечаю моба (если он есть)
			if (victim && IS_NPC(victim))
				SET_BIT(MOB_FLAGS(victim, MOB_NOTRAIN), MOB_NOTRAIN);
		}
	}
}


int train_skill(CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict)
{
	int percent = 0;

	percent = calculate_skill(ch, skill_no, max_value, vict);
	if (!IS_NPC(ch))
	{
		if (skill_no != SKILL_SATTACK &&
				ch->get_trained_skill(skill_no) > 0 && (!vict
												|| (IS_NPC(vict)
													&& !MOB_FLAGGED(vict, MOB_PROTECT)
													&& !MOB_FLAGGED(vict, MOB_NOTRAIN)
													&& !AFF_FLAGGED(vict, AFF_CHARM)
													&& !IS_HORSE(vict))))
			improove_skill(ch, skill_no, percent >= max_value, vict);
	}
	else if (!IS_CHARMICE(ch))
		if (ch->get_skill(skill_no) > 0 &&
				GET_REAL_INT(ch) <= number(0, 1000 - 20 * GET_REAL_WIS(ch)) &&
				ch->get_skill(skill_no) < skill_info[skill_no].max_percent)
			ch->set_skill(skill_no, ch->get_skill(skill_no) + 1);

	return (percent);
}

/**
* Расчет влияния осторожки у victim против умений killer.
* В данный момент учитывается случай 'наем против игрока', где осторожка считается
* как скилл/2.5, т.е. 80% от дружа на тех же мортах/скилле без дальнейшего увеличения разницы.
* У дружа в савингах от заклинаний идет доп. бонус от осторожки выше 80ти (чтобы не считали за баг).
*/
int calculate_awake_mod(CHAR_DATA *killer, CHAR_DATA *victim)
{
	int result = 0;
	if (!killer || !victim)
		log("SYSERROR: zero character in calculate_awake_mod.");
	else if (IS_NPC(killer) || IS_NPC(victim))
		result = victim->get_skill(SKILL_AWAKE) / 2;
	else
	{
		if (!can_use_feat(victim, SHADOW_STRIKE_FEAT))
			result = victim->get_skill(SKILL_AWAKE) / 2;
		else
			result = static_cast<int>(victim->get_skill(SKILL_AWAKE) / 2.5);
	}
	return result;
}

/*
 * функция определяет, может ли персонаж илучить скилл
 * впоследствии ее нужно будет перенести в класс "инфа о классе персонажа"
 */
int min_skill_level(CHAR_DATA *ch, int skill)
{
	return (skill_info[skill].min_level[ch->get_class()][ch->get_kin()] -
		(MAX(0,ch->get_remort()/skill_info[skill].level_decrement[ch->get_class()][ch->get_kin()])));
};

bool can_get_skill(CHAR_DATA *ch, int skill)
{
	if (ch->get_remort() < skill_info[skill].min_remort[ch->get_class()][ch->get_kin()] ||
		(skill_info[skill].classknow[ch->get_class()][ch->get_kin()] != KNOW_SKILL))
		return FALSE;
	if (ch->get_level() < min_skill_level(ch, skill))
		return FALSE;

	return TRUE;
};


//  Реализация класса Skill

//Объявляем глобальный скиллист
SkillListType Skill::SkillList;

// Конструктор
Skill::Skill() : _Name(SKILL_NAME_UNDEFINED), _Number(SKILL_UNDEFINED), _MaxPercent(0)
{
// Инициализация от греха подальше
};

// Парсим блок описания скилла
void Skill::ParseSkill(pugi::xml_node SkillNode)
{
    std::string SkillID;
    pugi::xml_node CurNode;
    SkillPtr TmpSkill(new Skill);

    // Базовые параметры скилла (а пока боле ничего и нет)
    SkillID = SkillNode.attribute("id").value();

    CurNode = SkillNode.child("number");
	TmpSkill->_Number = CurNode.attribute("val").as_int();
    CurNode = SkillNode.child("name");
	TmpSkill->_Name = CurNode.attribute("text").value();
	CurNode = SkillNode.child("max_percent");
	TmpSkill->_MaxPercent = CurNode.attribute("val").as_int();

	// Добавляем новый скилл в лист
    Skill::SkillList.insert(make_pair(SkillID, TmpSkill));
};

// Парсинг скиллов
// Вынесено в отдельную функцию, чтобы, если нам передали кривой XML лист, не создавался кривой скилл
void Skill::Load(pugi::xml_node XMLSkillList)
{
    pugi::xml_node CurNode;

	for (CurNode = XMLSkillList.child("skill"); CurNode; CurNode = CurNode.next_sibling("skill"))
	{
		Skill::ParseSkill(CurNode);
	}
};

// Получаем номер скилла по его иду
// Отрыжка совместимости со старым кодом
int Skill::GetNumByID(std::string ID)
{
    SkillPtr TmpSkill = Skill::SkillList[ID];
    //SkillPtr TmpSkill = Skill::SkillList.find(ID);
    if (TmpSkill)
        return TmpSkill->_Number ;

    return SKILL_UNDEFINED;
};

// Конец (увы) реализации класса Skill
