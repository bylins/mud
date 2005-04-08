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
#include "shop.h"

#include "spells.h"
#include "skills.h"
#include "screen.h"

#include "dg_scripts.h"
#include "constants.h"
#include "im.h"

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
	"легонько ",		//  5..10
	"слегка ",		// 11..15
	"",			// 16..20
	"сильно ",		// 21..25
	"очень сильно ",	// 26..30
	"чрезвычайно сильно ",	// 31..35
	"ЖЕСТОКО ",		// 36..43
	"БОЛЬНО ",		// 44..58
	"ОЧЕНЬ БОЛЬНО ",	// 59..72
	"ЧРЕЗВЫЧАЙНО БОЛЬНО ",	// 73..86
	"НЕВЫНОСИМО БОЛЬНО ",	// 87..100
	"УЖАСНО ",		// 101..150
	"СМЕРТЕЛЬНО "		// > 150
};

int skill_message(int dam, CHAR_DATA * ch, CHAR_DATA * vict, int attacktype)
{
	int i, j, nr, weap_i;
	struct message_type *msg;
	OBJ_DATA *weap = GET_EQ(ch, WEAR_WIELD) ? GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch,
										  WEAR_BOTHS);

	// log("[SKILL MESSAGE] Message for skill %d",attacktype);
	for (i = 0; i < MAX_MESSAGES; i++) {
		if (fight_messages[i].a_type == attacktype) {
			nr = dice(1, fight_messages[i].number_of_attacks);
			// log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;

			switch (attacktype) {
			case SKILL_BACKSTAB + TYPE_HIT:
				if (!(weap = GET_EQ(ch, WEAR_WIELD))
				    && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
					weap = (obj_proto + weap_i);
				break;
			case SKILL_THROW + TYPE_HIT:
				if (!(weap = GET_EQ(ch, WEAR_WIELD))
				    && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
					weap = (obj_proto + weap_i);
				break;
			case SKILL_BASH + TYPE_HIT:
				if (!(weap = GET_EQ(ch, WEAR_SHIELD))
				    && (weap_i = real_object(DUMMY_SHIELD)) >= 0)
					weap = (obj_proto + weap_i);
				break;
			case SKILL_KICK + TYPE_HIT:
				// weap - текст силы удара
				if (dam <= 5)
					weap = (OBJ_DATA *) kick_type[0];
				else if (dam <= 10)
					weap = (OBJ_DATA *) kick_type[1];
				else if (dam <= 15)
					weap = (OBJ_DATA *) kick_type[2];
				else if (dam <= 20)
					weap = (OBJ_DATA *) kick_type[3];
				else if (dam <= 25)
					weap = (OBJ_DATA *) kick_type[4];
				else if (dam <= 30)
					weap = (OBJ_DATA *) kick_type[5];
				else if (dam <= 35)
					weap = (OBJ_DATA *) kick_type[5];
				else if (dam <= 44)
					weap = (OBJ_DATA *) kick_type[6];
				else if (dam <= 58)
					weap = (OBJ_DATA *) kick_type[7];
				else if (dam <= 72)
					weap = (OBJ_DATA *) kick_type[8];
				else if (dam <= 86)
					weap = (OBJ_DATA *) kick_type[9];
				else if (dam <= 100)
					weap = (OBJ_DATA *) kick_type[10];
				else if (dam <= 150)
					weap = (OBJ_DATA *) kick_type[11];
				else
					weap = (OBJ_DATA *) kick_type[12];
				break;
			case TYPE_HIT:
				weap = NULL;
				break;
			default:
				if (!weap && (weap_i = real_object(DUMMY_WEAPON)) >= 0)
					weap = (obj_proto + weap_i);
			}

			if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
				switch (attacktype) {
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char(CCWHT(ch, C_CMP), ch);
					break;
				default:
					send_to_char(CCYEL(ch, C_CMP), ch);
					break;
				}
				act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict,
				    TO_CHAR);
				send_to_char(CCNRM(ch, C_CMP), ch);

				act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
				act(msg->god_msg.room_msg, FALSE, ch, weap, vict,
				    TO_NOTVICT);
			} else if (dam != 0) {
				if (GET_POS(vict) == POS_DEAD) {
					send_to_char(CCIYEL(ch, C_CMP), ch);
					act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict,
					    TO_CHAR);
					send_to_char(CCNRM(ch, C_CMP), ch);

					send_to_char(CCIRED(vict, C_CMP), vict);
					act(msg->die_msg.victim_msg, FALSE, ch, weap, vict,
					    TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(vict, C_CMP), vict);
					act(msg->die_msg.room_msg, FALSE, ch, weap, vict,
					    TO_NOTVICT);
				} else {
					send_to_char(CCIYEL(ch, C_CMP), ch);
					act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict,
					    TO_CHAR);
					send_to_char(CCNRM(ch, C_CMP), ch);
					send_to_char(CCIRED(vict, C_CMP), vict);
					act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict,
					    TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(vict, C_CMP), vict);
					act(msg->hit_msg.room_msg, FALSE, ch, weap, vict,
					    TO_NOTVICT);
				}
			} else if (ch != vict) {	/* Dam == 0 */
				switch (attacktype) {
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char(CCWHT(ch, C_CMP), ch);
					break;
				default:
					send_to_char(CCYEL(ch, C_CMP), ch);
					break;
				}
				act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict,
				    TO_CHAR);
				send_to_char(CCNRM(ch, C_CMP), ch);

				send_to_char(CCRED(vict, C_CMP), vict);
				act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict,
				    TO_VICT | TO_SLEEP);
				send_to_char(CCNRM(vict, C_CMP), vict);

				act(msg->miss_msg.room_msg, FALSE, ch, weap, vict,
				    TO_NOTVICT);
			}
			return (1);
		}
	}
	return (0);
}

/**** This function return chance of skill */
int calculate_skill(CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict)
{
	int skill_is, percent = 0, victim_sav = SAVING_REFLEX, victim_modi = 0;
	int morale, use = 0;

	if (skill_no < 1 || skill_no > MAX_SKILLS) {	// log("ERROR: ATTEMPT USING UNKNOWN SKILL <%d>", skill_no);
		return 0;
	}
	if ((skill_is = GET_SKILL(ch, skill_no)) <= 0) {
		return 0;
	}

	skill_is += int_app[GET_REAL_INT(ch)].to_skilluse;
	switch (skill_no) {
	case SKILL_BACKSTAB:	/*заколоть */
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction * 2;
		if (awake_others(ch))
			percent -= 50;

		if (vict) {
			if (!CAN_SEE(vict, ch))
				percent += 25;
			if (GET_POS(vict) < POS_FIGHTING)
				percent += (20 * (POS_FIGHTING - GET_POS(vict)));
			else if (AFF_FLAGGED(vict, AFF_AWARNESS))
				victim_modi -= 30;
			victim_modi += size_app[GET_POS_SIZE(vict)].ac;
			victim_modi -= dex_app_skill[GET_REAL_DEX(vict)].traps;
		}
		break;
	case SKILL_BASH:	/*сбить */
		percent = skill_is +
		    size_app[GET_POS_SIZE(ch)].interpolate +
		    (dex_app[GET_REAL_DEX(ch)].reaction * 2) +
		    (GET_EQ(ch, WEAR_SHIELD) ?
		     weapon_app[MIN(50, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD))))].
		     bashing : 0);
		if (vict) {
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
			if (GET_POS(vict) < POS_FIGHTING && GET_POS(vict) > POS_SLEEPING)
				victim_modi -= 20;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
			victim_modi -= dex_app[GET_REAL_DEX(ch)].reaction;
		}
		break;
	case SKILL_HIDE:	/*спрятаться */
		percent =
		    skill_is + dex_app_skill[GET_REAL_DEX(ch)].hide -
		    size_app[GET_POS_SIZE(ch)].ac;

		if (awake_others(ch))
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
		if (equip_in_metall(ch))
			percent -= 50;

		if (vict) {
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_KICK:	/*пнуть */
		percent =
		    skill_is + GET_REAL_DEX(ch) + GET_REAL_HR(ch) +
		    (AFF_FLAGGED(ch, AFF_BLESS) ? 2 : 0);
		if (vict) {
			victim_modi += size_app[GET_POS_SIZE(vict)].interpolate;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
		}
		break;
	case SKILL_PICK_LOCK:	/*pick lock */
		percent = skill_is + dex_app_skill[GET_REAL_DEX(ch)].p_locks;
		break;
	case SKILL_PUNCH:	/*punch */
		percent = skill_is;
		break;
	case SKILL_RESCUE:	/*спасти */
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;
		victim_modi = 100;
		break;
	case SKILL_SNEAK:	/*sneak */
		percent = skill_is + dex_app_skill[GET_REAL_DEX(ch)].sneak;

		if (awake_others(ch))
			percent -= 50;

		if (SECT(IN_ROOM(ch)) == SECT_CITY)
			percent -= 10;
		if (IS_DARK(IN_ROOM(ch)))
			percent += 20;
		if (equip_in_metall(ch))
			percent -= 50;

		if (vict) {
			if (!CAN_SEE(vict, ch))
				victim_modi += 25;
			if (AWAKE(vict)) {
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
		}
		break;
	case SKILL_STEAL:	/*steal */
		percent = skill_is + dex_app_skill[GET_REAL_DEX(ch)].p_pocket;

		if (awake_others(ch))
			percent -= 50;

		if (IS_DARK(IN_ROOM(ch)))
			percent += 20;

		if (vict) {
			if (!CAN_SEE(vict, ch))
				victim_modi += 25;
			if (AWAKE(vict)) {
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				if (AFF_FLAGGED(vict, AFF_AWARNESS))
					victim_modi -= 30;
			}
		}
		break;
	case SKILL_TRACK:	/*выследить */
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;

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


		if (vict) {
			victim_modi += con_app[GET_REAL_CON(vict)].hitp;
			if (AFF_FLAGGED(vict, AFF_NOTRACK)
			    || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) victim_modi = -100;
		}
		break;

	case SKILL_SENSE:
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;

		percent =
		    complex_skill_modifier(ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS, percent);

		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
			percent = 0;

		if (vict) {
			victim_modi += con_app[GET_REAL_CON(vict)].hitp;
			if (AFF_FLAGGED(vict, AFF_NOTRACK)
			    || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) victim_modi = -100;
		}
		break;
	case SKILL_MULTYPARRY:
	case SKILL_PARRY:	/*парировать */
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;
		if (GET_AF_BATTLE(ch, EAF_AWAKE))
			percent += GET_SKILL(ch, SKILL_AWAKE);

		if (GET_EQ(ch, WEAR_HOLD)
		    && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON) {
			percent +=
			    weapon_app[MAX
				       (0,
					MIN(50,
					    GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))))].
			    parrying;
		}
		victim_modi = 100;
		break;

	case SKILL_BLOCK:	/*закрыться щитом */
		percent = skill_is + dex_app[GET_REAL_DEX(ch) * 2].reaction;
		if (GET_AF_BATTLE(ch, EAF_AWAKE))
			percent += GET_SKILL(ch, SKILL_AWAKE);

		break;

	case SKILL_TOUCH:	/*захватить противника */
		percent =
		    skill_is + dex_app[GET_REAL_DEX(ch)].reaction +
		    size_app[GET_POS_SIZE(vict)].interpolate;

		if (vict) {
			victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
		}
		break;

	case SKILL_PROTECT:	/*прикрыть грудью */
		percent =
		    skill_is + dex_app[GET_REAL_DEX(ch)].reaction +
		    size_app[GET_POS_SIZE(ch)].interpolate;

		victim_modi = 100;
		break;

	case SKILL_BOWS:	/*луки */
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att;
		break;
	case SKILL_BOTHHANDS:	/*двуручники */
	case SKILL_LONGS:	/*длинные лезвия */
	case SKILL_SPADES:	/*копья и пики */
	case SKILL_SHORTS:	/*короткие лезвия */
	case SKILL_CLUBS:	/*палицы и дубины */
	case SKILL_PICK:	/*проникающее */
	case SKILL_NONSTANDART:	/*разнообразное оружие */
	case SKILL_AXES:	/*секиры */
		percent = skill_is;
		break;
	case SKILL_SATTACK:	/*атака второй рукой */
		percent = skill_is;
		break;
	case SKILL_LOOKING:	/*приглядеться */
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
		break;
	case SKILL_HEARING:	/*прислушаться */
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
		break;
	case SKILL_DISARM:
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;
		if (vict) {
			victim_modi -= dex_app[GET_REAL_DEX(ch)].reaction;
			if (GET_EQ(vict, WEAR_BOTHS))
				victim_modi -= 10;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
		}
		break;
	case SKILL_HEAL:
		percent = skill_is;
		break;
	case SKILL_TURN:
		percent = skill_is;
		break;
	case SKILL_ADDSHOT:
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att;
		use = 1;
		if (equip_in_metall(ch))
			percent -= 20;
		break;
	case SKILL_NOPARRYHIT:
		percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att;
		break;
	case SKILL_CAMOUFLAGE:
		percent =
		    skill_is + dex_app_skill[GET_REAL_DEX(ch)].hide -
		    size_app[GET_POS_SIZE(ch)].ac;

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

		if (vict) {
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_DEVIATE:
		percent =
		    skill_is - size_app[GET_POS_SIZE(ch)].ac +
		    dex_app[GET_REAL_DEX(ch)].reaction;

		if (equip_in_metall(ch))
			percent -= 40;

		if (vict) {
			victim_modi -= dex_app[GET_REAL_DEX(vict)].miss_att;
		}
		break;
	case SKILL_CHOPOFF:
		percent =
		    skill_is + dex_app[GET_REAL_DEX(ch)].reaction +
		    size_app[GET_POS_SIZE(ch)].ac;

		if (equip_in_metall(ch))
			percent -= 10;

		if (vict) {
			if (!CAN_SEE(vict, ch))
				percent += 10;
			if (GET_POS(vict) < POS_SITTING)
				percent -= 50;
			if (AWAKE(vict) && AFF_FLAGGED(vict, AFF_AWARNESS))
				victim_modi -= 30;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= GET_SKILL(ch, SKILL_AWAKE);
			victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_REPAIR:
		percent = skill_is;
		break;
	case SKILL_UPGRADE:
		percent = skill_is;
	case SKILL_BERSERK:
		percent = skill_is;
		break;
	case SKILL_COURAGE:
		percent = skill_is;
		break;
	case SKILL_SHIT:
		percent = skill_is;
		break;
	case SKILL_MIGHTHIT:
		percent =
		    skill_is + size_app[GET_POS_SIZE(ch)].shocking +
		    str_app[GET_REAL_STR(ch)].tohit;

		if (vict) {
			victim_modi -= size_app[GET_POS_SIZE(vict)].shocking;
		}
		break;
	case SKILL_STUPOR:
		percent = skill_is + str_app[GET_REAL_STR(ch)].tohit;
		if (GET_EQ(ch, WEAR_WIELD))
			percent +=
			    weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))].shocking;
		else if (GET_EQ(ch, WEAR_BOTHS))
			percent +=
			    weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))].shocking;

		if (vict) {
			victim_modi += con_app[GET_REAL_CON(vict)].critic_saving;
		}
		break;
	case SKILL_POISONED:
		percent = skill_is;
		break;
	case SKILL_LEADERSHIP:
		percent = skill_is + cha_app[GET_REAL_CHA(ch)].leadership;
		break;
	case SKILL_PUNCTUAL:
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
		if (vict) {
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_AWAKE:
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;

		if (vict) {
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;

	case SKILL_IDENTIFY:
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
		break;

	case SKILL_CREATE_POTION:
	case SKILL_CREATE_SCROLL:
	case SKILL_CREATE_WAND:
		percent = skill_is;
		break;
	case SKILL_LOOK_HIDE:
		percent = skill_is + cha_app[GET_REAL_CHA(ch)].illusive;
		if (vict) {
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
		percent = skill_is - con_app[GET_REAL_CON(ch)].hitp;
		break;
	case SKILL_AID:
		percent = skill_is;
		break;
	case SKILL_FIRE:
		percent = skill_is;
		if (get_room_sky(IN_ROOM(ch)) == SKY_RAINING)
			percent -= 50;
		else if (get_room_sky(IN_ROOM(ch)) != SKY_LIGHTNING)
			percent -= number(10, 25);
	case SKILL_HORSE:
		percent = skill_is + cha_app[GET_REAL_CHA(ch)].leadership;
	default:
		percent = skill_is;
		break;
	}

	percent = complex_skill_modifier(ch, skill_no, GAPPLY_SKILL_SUCCESS, percent);

	if (vict && percent > skill_info[skill_no].max_percent)
		victim_modi += percent - skill_info[skill_no].max_percent;

	morale = cha_app[GET_REAL_CHA(ch)].morale + GET_MORALE(ch);
	if (AFF_FLAGGED(ch, AFF_DEAFNESS))
		morale -= 20;	// у глухого мораль на 20 меньше

// если мораль отрицательная, увеличивается вероятность, что умение не пройдет
	if ((skill_is = number(0, 99)) >= 95 + (morale >= 0 ? 0 : morale))
		percent = 0;
	else if (skill_is <= morale)
		percent = skill_info[skill_no].max_percent;
	else if (vict && general_savingthrow(vict, victim_sav, victim_modi, use))
		percent = 0;

	if (IS_IMMORTAL(ch) ||	// бессмертный
	    GET_GOD_FLAG(ch, GF_GODSLIKE)	// спецфлаг
	    )
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else if (GET_GOD_FLAG(ch, GF_GODSCURSE) || (vict && GET_GOD_FLAG(vict, GF_GODSLIKE)))
		percent = 0;
	else if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else
		percent = MIN(MAX(0, percent), max_value);

	return (percent);
}

void improove_skill(CHAR_DATA * ch, int skill_no, int success, CHAR_DATA * victim)
{
	int skill_is, diff = 0, how_many = 0, prob, div;

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
	      wis_app[GET_REAL_WIS(ch)].max_learn_l20 * GET_LEVEL(ch) / 20 -
	      GET_SKILL(ch, skill_no)) > 0
	     && GET_SKILL(ch, skill_no) < MAX_EXP_PERCENT + GET_REMORT(ch) * 5)) {
		for (prob = how_many = 0; prob <= MAX_SKILLS; prob++)
			if (GET_SKILL(ch, prob))
				how_many++;

		how_many += (im_get_char_rskill_count(ch) + 1) >> 1;

		/* Success - multy by 2 */
		prob = success ? 20000 : 15000;

		div = int_app[GET_REAL_INT(ch)].improove /* + diff */ ;

		if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_CLASSES)
		        div += (skill_info[skill_no].k_improove[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] / 100);

		prob /= (MAX(1, div));

		if ((diff = how_many - wis_app[GET_REAL_WIS(ch)].max_skills) < 0)
			prob += (5 * diff);
		else
			prob += (10 * diff);
		prob += number(1, GET_SKILL(ch, skill_no) * 5);

		skill_is = number(1, MAX(1, prob));

		// if (!IS_NPC(ch))
//        log("Player %s skill '%d' - need to improove %d(%d-%d)",
//            GET_NAME(ch), skill_no, skill_is, div, prob);

		if (
		    (victim
		     && skill_is <= GET_REAL_INT(ch) * GET_LEVEL(victim) / GET_LEVEL(ch))
		    || (!victim && skill_is <= GET_REAL_INT(ch))) {
			if (success)
				sprintf(buf, "%sВы повысили уровень умения \"%s\".%s\r\n",
					CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch,
										       C_NRM));
			else
				sprintf(buf,
					"%sПоняв свои ошибки, Вы повысили уровень умения \"%s\".%s\r\n",
					CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch,
										       C_NRM));
			send_to_char(buf, ch);
			GET_SKILL(ch, skill_no) += number(1, 2);
			if (!IS_IMMORTAL(ch))
				GET_SKILL(ch, skill_no) = MIN(MAX_EXP_PERCENT + GET_REMORT(ch) * 5,
				GET_SKILL(ch, skill_no));
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
	if (!IS_NPC(ch)) {
		if (skill_no != SKILL_SATTACK &&
		    GET_SKILL(ch, skill_no) > 0 && (!vict
						    || (IS_NPC(vict)
							&& !MOB_FLAGGED(vict, MOB_PROTECT)
							&& !MOB_FLAGGED(vict, MOB_NOTRAIN)
							&& !AFF_FLAGGED(vict, AFF_CHARM)
							&& !IS_HORSE(vict))))
			improove_skill(ch, skill_no, percent >= max_value, vict);
	} else
	    if (GET_SKILL(ch, skill_no) > 0 &&
		GET_REAL_INT(ch) <= number(0, 1000 - 20 * GET_REAL_WIS(ch)) &&
		GET_SKILL(ch, skill_no) < skill_info[skill_no].max_percent)
		GET_SKILL(ch, skill_no)++;

	return (percent);
}
