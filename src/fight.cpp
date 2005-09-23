/* ************************************************************************
*   File: fight.cpp                                     Part of Bylins    *
*  Usage: Combat system                                                   *
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

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mobmax.h"
#include "pk.h"
#include "im.h"
#include "fight.h"
#include "skills.h"

extern CHAR_DATA *mob_proto;

/* Structures */
CHAR_DATA *combat_list = NULL;	/* head of l-list of fighting chars */
CHAR_DATA *next_combat_list = NULL;

extern struct message_list fight_messages[MAX_MESSAGES];
extern OBJ_DATA *object_list;
extern CHAR_DATA *character_list;
extern OBJ_DATA *obj_proto;
extern int max_exp_gain_npc;	/* see config.cpp */
extern int max_npc_corpse_time, max_pc_corpse_time;
extern const int material_value[];
extern int supress_godsapply;
extern int r_helled_start_room;

/* External procedures */
CHAR_DATA *try_protect(CHAR_DATA * victim, CHAR_DATA * ch, int skill);
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
ACMD(do_assist);
ACMD(do_get);
void get_from_container(CHAR_DATA * ch, OBJ_DATA * cont, char *arg, int mode, int amount);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(CHAR_DATA * ch, CHAR_DATA * victim);
void battle_affect_update(CHAR_DATA * ch);
void go_throw(CHAR_DATA * ch, CHAR_DATA * vict);
void go_bash(CHAR_DATA * ch, CHAR_DATA * vict);
void go_kick(CHAR_DATA * ch, CHAR_DATA * vict);
void go_rescue(CHAR_DATA * ch, CHAR_DATA * vict, CHAR_DATA * tmp_ch);
void go_parry(CHAR_DATA * ch);
void go_multyparry(CHAR_DATA * ch);
void go_block(CHAR_DATA * ch);
void go_touch(CHAR_DATA * ch, CHAR_DATA * vict);
void go_protect(CHAR_DATA * ch, CHAR_DATA * vict);
void go_chopoff(CHAR_DATA * ch, CHAR_DATA * vict);
void go_disarm(CHAR_DATA * ch, CHAR_DATA * vict);
const char *skill_name(int num);
void npc_groupbattle(CHAR_DATA * ch);
int npc_battle_scavenge(CHAR_DATA * ch);
int npc_wield(CHAR_DATA * ch);
int npc_armor(CHAR_DATA * ch);

int max_exp_gain_pc(CHAR_DATA * ch);
int max_exp_loss_pc(CHAR_DATA * ch);
int level_exp(CHAR_DATA * ch, int chlevel);
int extra_aco(int class_num, int level);
void change_fighting(CHAR_DATA * ch, int need_stop);
int perform_mob_switch(CHAR_DATA * ch);
/* local functions */
//void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
void dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim, int w_type);
void appear(CHAR_DATA * ch);
void load_messages(void);
//void check_killer(CHAR_DATA * ch, CHAR_DATA * vict);
OBJ_DATA *make_corpse(CHAR_DATA * ch);
void change_alignment(CHAR_DATA * ch, CHAR_DATA * victim);
void death_cry(CHAR_DATA * ch);
void raw_kill(CHAR_DATA * ch, CHAR_DATA * killer);
int can_loot(CHAR_DATA * ch);
void die(CHAR_DATA * ch, CHAR_DATA * killer);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);
//void solo_gain(CHAR_DATA * ch, CHAR_DATA * victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(CHAR_DATA * ch);
int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
void apply_weapon_bonus(int ch_class, int skill, int *damroll, int *hitroll);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] = {
	{"ударил", "ударить"},	/* 0 */
	{"ободрал", "ободрать"},
	{"хлестнул", "хлестнуть"},
	{"рубанул", "рубануть"},
	{"укусил", "укусить"},
	{"огрел", "огреть"},	/* 5 */
	{"сокрушил", "сокрушить"},
	{"резанул", "резануть"},
	{"оцарапал", "оцарапать"},
	{"подстрелил", "подстрелить"},
	{"пырнул", "пырнуть"},	/* 10 */
	{"уколол", "уколоть"},
	{"ткнул", "ткнуть"},
	{"лягнул", "лягнуть"},
	{"боднул", "боднуть"},
	{"клюнул", "клюнуть"},
	{"*", "*"},
	{"*", "*"},
	{"*", "*"},
	{"*", "*"}
};

void check_berserk(CHAR_DATA * ch)
{
	AFFECT_DATA af[2];
	struct timed_type timed;
	int j;

	if (affected_by_spell(ch, SPELL_BERSERK) &&
		    (GET_HIT(ch) > GET_REAL_MAX_HIT(ch) / 2)) {
			affect_from_char(ch, SPELL_BERSERK);
			send_to_char("Предсмертное исступление оставило Вас.\r\n", ch);
	}
	if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_BERSERK) && FIGHTING(ch) &&
	    !timed_by_skill(ch, SKILL_BERSERK) && !AFF_FLAGGED(ch, AFF_BERSERK) &&
	    (GET_HIT(ch) < GET_REAL_MAX_HIT(ch) / 4)) {

		timed.skill = SKILL_BERSERK;
		timed.time = 6;
		timed_to_char(ch, &timed);

		af[0].type = SPELL_BERSERK;
		af[0].duration = pc_duration(ch, 1, MAX(0, GET_SKILL(ch, SKILL_BERSERK)-40), 30, 0, 0);
		af[0].modifier = 0;
		af[0].location = APPLY_NONE;
		af[0].battleflag = 0;

		/*af[1].type = SPELL_BERSERK;
		af[1].duration = pc_duration(ch, 1, MAX(0, GET_SKILL(ch, SKILL_BERSERK)-40), 30, 0, 0);
		af[1].modifier = 0;
		af[1].location = APPLY_NONE;
		af[1].battleflag = 0; */

		// Я знаю, очень-очень криво. Но надо было сделать расскачку скила
		// более частой, чем если бы только когда скил успешно прошел.
		// Причем заклинание !исступление! висит всегда пока идут просветы,
		// а вот плюшки идут только если установлен соотв. аффект =)
		// Да и еще этот аффект !флик. Вообщем нет предела совершенству. 
		// Дерзайте, правьте если знаете как. А нет - стирайте этот флейм.
		if (calculate_skill(ch, SKILL_BERSERK, skill_info[SKILL_BERSERK].max_percent, 0) >= 
		    number(1, skill_info[SKILL_BERSERK].max_percent) ||
		    number(1, 20) >= 10 + MAX(0, (GET_LEVEL(ch) - 14 - GET_REMORT(ch)) / 2)) {
			//af[0].bitvector = AFF_NOFLEE;
			af[0].bitvector = AFF_BERSERK;
			act("Вас обуяла предсмертная ярость!", FALSE, ch, 0, 0, TO_CHAR);
			act("$n0 исступленно взвыл$g и бросил$u на противника!.", FALSE, ch, 0, 0, TO_ROOM);
		} else {
			//af[0].bitvector = 0;
			af[0].bitvector = 0;
			act("Вы истошно завопили, пытась напугать противника. Без толку.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n0 истошно завопил$g, пытаясь напугать противника. Забавно...", FALSE, ch, 0, 0, TO_ROOM);
		}

		for (j = 0; j < 2; j++)
			affect_join(ch, &af[j], TRUE, FALSE, TRUE, FALSE);
	}
}

void go_autoassist(CHAR_DATA * ch)
{
	struct follow_type *k;
	CHAR_DATA *ch_lider = 0;
	if (ch->master) {
		ch_lider = ch->master;
	} else
		ch_lider = ch;	// Создаем ссылку на лидера
	for (k = ch_lider->followers; k; k = k->next) {
		if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
		    (IN_ROOM(k->follower) == IN_ROOM(ch)) && !FIGHTING(k->follower) &&
		    (GET_POS(k->follower) == POS_STANDING) && !CHECK_WAIT(k->follower))
			do_assist(k->follower, "", 0, 0);
	}
	if (PRF_FLAGGED(ch_lider, PRF_AUTOASSIST) &&
	    (IN_ROOM(ch_lider) == IN_ROOM(ch)) && !FIGHTING(ch_lider) &&
	    (GET_POS(ch_lider) == POS_STANDING) && !CHECK_WAIT(ch_lider))
		do_assist(ch_lider, "", 0, 0);
}

int calc_leadership(CHAR_DATA * ch)
{
	int prob, percent;
	CHAR_DATA *leader = 0;

	if (IS_NPC(ch) || !AFF_FLAGGED(ch, AFF_GROUP) || (!ch->master && !ch->followers))
		return (FALSE);

	if (ch->master) {
		if (IN_ROOM(ch) != IN_ROOM(ch->master))
			return (FALSE);
		leader = ch->master;
	} else
		leader = ch;

	if (!GET_SKILL(leader, SKILL_LEADERSHIP))
		return (FALSE);

	percent = number(1, 101);
	prob = calculate_skill(leader, SKILL_LEADERSHIP, 121, 0);
	if (percent > prob)
		return (FALSE);
	else
		return (TRUE);
}

/* The Fight related routines */

void appear(CHAR_DATA * ch)
{
	int appear_msg = AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_CAMOUFLAGE) || AFF_FLAGGED(ch, AFF_HIDE);

	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);
	if (affected_by_spell(ch, SPELL_HIDE))
		affect_from_char(ch, SPELL_HIDE);
	if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
		affect_from_char(ch, SPELL_CAMOUFLAGE);

	REMOVE_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);

	if (appear_msg) {
		if (GET_LEVEL(ch) < LVL_IMMORT)
			act("$n медленно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
		else
			act("Вы почувствовали странное присутствие $n1.", FALSE, ch, 0, 0, TO_ROOM);
	}
}


int compute_armor_class(CHAR_DATA * ch)
{
	int armorclass = GET_REAL_AC(ch);

	if (AWAKE(ch)) {
		armorclass += dex_app[GET_REAL_DEX(ch)].defensive * 10;
		armorclass += extra_aco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	};

	if (AFF_FLAGGED(ch, AFF_BERSERK)) {
			armorclass -= (240 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
	}

	armorclass += (size_app[GET_POS_SIZE(ch)].ac * 10);

	armorclass = MIN(100, armorclass);

	if (GET_AF_BATTLE(ch, EAF_PUNCTUAL)) {
		if (GET_EQ(ch, WEAR_WIELD)) {
			if (GET_EQ(ch, WEAR_HOLD))
				armorclass +=
				    10 * MAX(-1,
					     (GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) +
					      GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) / 5 - 6);
			else
				armorclass += 10 * MAX(-1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) / 5 - 6);
		}
		if (GET_EQ(ch, WEAR_BOTHS))
			armorclass += 10 * MAX(-1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS)) / 5 - 6);
	}
	return (MAX(-300, armorclass));	/* Теперь нижняя планка -300 (c)dzMUDiST */
}


void load_messages(void)
{
	FILE *fl;
	int i, type;
	struct message_type *messages;
	char chk[128];

	if (!(fl = fopen(MESS_FILE, "r"))) {
		log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
		exit(1);
	}
	for (i = 0; i < MAX_MESSAGES; i++) {
		fight_messages[i].a_type = 0;
		fight_messages[i].number_of_attacks = 0;
		fight_messages[i].msg = 0;
	}


	fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		fgets(chk, 128, fl);

	while (*chk == 'M') {
		fgets(chk, 128, fl);
		sscanf(chk, " %d\n", &type);
		for (i = 0; (i < MAX_MESSAGES) &&
		     (fight_messages[i].a_type != type) && (fight_messages[i].a_type); i++);
		if (i >= MAX_MESSAGES) {
			log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
			exit(1);
		}
		log("BATTLE MESSAGE %d(%d)", i, type);
		CREATE(messages, struct message_type, 1);
		fight_messages[i].number_of_attacks++;
		fight_messages[i].a_type = type;
		messages->next = fight_messages[i].msg;
		fight_messages[i].msg = messages;

		messages->die_msg.attacker_msg = fread_action(fl, i);
		messages->die_msg.victim_msg = fread_action(fl, i);
		messages->die_msg.room_msg = fread_action(fl, i);
		messages->miss_msg.attacker_msg = fread_action(fl, i);
		messages->miss_msg.victim_msg = fread_action(fl, i);
		messages->miss_msg.room_msg = fread_action(fl, i);
		messages->hit_msg.attacker_msg = fread_action(fl, i);
		messages->hit_msg.victim_msg = fread_action(fl, i);
		messages->hit_msg.room_msg = fread_action(fl, i);
		messages->god_msg.attacker_msg = fread_action(fl, i);
		messages->god_msg.victim_msg = fread_action(fl, i);
		messages->god_msg.room_msg = fread_action(fl, i);
		fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
			fgets(chk, 128, fl);
	}

	fclose(fl);
}


void update_pos(CHAR_DATA * victim)
{
	if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
		GET_POS(victim) = GET_POS(victim);
	else if (GET_HIT(victim) > 0)
		GET_POS(victim) = POS_STANDING;
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = POS_DEAD;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = POS_MORTALLYW;
	else if (GET_HIT(victim) <= -3)
		GET_POS(victim) = POS_INCAP;
	else
		GET_POS(victim) = POS_STUNNED;

	if (AFF_FLAGGED(victim, AFF_SLEEP) && GET_POS(victim) != POS_SLEEPING)
		affect_from_char(victim, SPELL_SLEEP);

	if (on_horse(victim) && GET_POS(victim) < POS_FIGHTING)
		horse_drop(get_horse(victim));

	if (IS_HORSE(victim) && GET_POS(victim) < POS_FIGHTING && on_horse(victim->master))
		horse_drop(victim);
}

#if 0
void check_killer(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
		return;
	if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict)
	    || ch == vict)
		return;

	/* SET_BIT(PLR_FLAGS(ch), PLR_KILLER);

	   sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	   GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)]->name);
	   mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
	 */
}
#endif

void set_battle_pos(CHAR_DATA * ch)
{
	switch (GET_POS(ch)) {
	case POS_STANDING:
		GET_POS(ch) = POS_FIGHTING;
		break;
	case POS_RESTING:
	case POS_SITTING:
	case POS_SLEEPING:
		if (GET_WAIT(ch) <= 0 &&
		    !GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, AFF_SLEEP) && !AFF_FLAGGED(ch, AFF_CHARM)) {
			if (IS_NPC(ch)) {
				act("$n встал$g на ноги.", FALSE, ch, 0, 0, TO_ROOM);
				GET_POS(ch) = POS_FIGHTING;
			} else if (!IS_NPC(ch) && GET_POS(ch) == POS_SLEEPING) {
				act("Вы проснулись и сели.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n проснул$u и сел$g.", FALSE, ch, 0, 0, TO_ROOM);
				GET_POS(ch) = POS_SITTING;
			}
		}
		break;
	}
}

void restore_battle_pos(CHAR_DATA * ch)
{
	switch (GET_POS(ch)) {
	case POS_FIGHTING:
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_RESTING:
	case POS_SITTING:
	case POS_SLEEPING:
		if (IS_NPC(ch) &&
		    GET_WAIT(ch) <= 0 &&
		    !GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, AFF_SLEEP) && !AFF_FLAGGED(ch, AFF_CHARM)) {
			act("$n встал$g на ноги.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_STANDING;
		}
		break;
	}
	if (AFF_FLAGGED(ch, AFF_SLEEP))
		GET_POS(ch) = POS_SLEEPING;
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (ch == vict)
		return;

	if (FIGHTING(ch)) {
		log("SYSERR: set_fighting(%s->%s) when already fighting(%s)...",
		    GET_NAME(ch), GET_NAME(vict), GET_NAME(FIGHTING(ch)));
		// core_dump();
		return;
	}

	if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT)) || (IS_NPC(vict) && MOB_FLAGGED(ch, MOB_NOFIGHT)))
		return;

	// if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
	//    return;

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, AFF_SLEEP))
		affect_from_char(ch, SPELL_SLEEP);
	FIGHTING(ch) = vict;
	NUL_AF_BATTLE(ch);
	PROTECTING(ch) = 0;
	TOUCHING(ch) = 0;
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	SET_EXTRA(ch, 0, NULL);
	set_battle_pos(ch);
	/* Set combat style */
	if (!AFF_FLAGGED(ch, AFF_COURAGE) && !AFF_FLAGGED(ch, AFF_DRUNKED) && !AFF_FLAGGED(ch, AFF_ABSTINENT)) {
		if (PRF_FLAGGED(ch, PRF_PUNCTUAL))
			SET_AF_BATTLE(ch, EAF_PUNCTUAL);
		else if (PRF_FLAGGED(ch, PRF_AWAKE))
			SET_AF_BATTLE(ch, EAF_AWAKE);
	}
//  check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(CHAR_DATA * ch, int switch_others)
{
	CHAR_DATA *temp, *found;

	if (ch == next_combat_list)
		next_combat_list = ch->next_fighting;

	REMOVE_FROM_LIST(ch, combat_list, next_fighting);
	ch->next_fighting = NULL;
	if (ch->last_comm != NULL)
		free(ch->last_comm);
	ch->last_comm = NULL;
	PROTECTING(ch) = NULL;
	TOUCHING(ch) = NULL;
	FIGHTING(ch) = NULL;
	INITIATIVE(ch) = 0;
	BATTLECNTR(ch) = 0;
	SET_EXTRA(ch, 0, NULL);
	SET_CAST(ch, 0, NULL, NULL, NULL);
	restore_battle_pos(ch);
	NUL_AF_BATTLE(ch);
	// sprintf(buf,"[Stop fighting] %s - %s\r\n",GET_NAME(ch),switch_others ? "switching" : "no switching");
	// send_to_gods(buf);
 /**** switch others *****/

	for (temp = combat_list; temp; temp = temp->next_fighting) {
		if (PROTECTING(temp) == ch) {
			PROTECTING(temp) = NULL;
			CLR_AF_BATTLE(temp, EAF_PROTECT);
		}
		if (TOUCHING(temp) == ch) {
			TOUCHING(temp) = NULL;
			CLR_AF_BATTLE(temp, EAF_TOUCH);
		}
		if (GET_EXTRA_VICTIM(temp) == ch)
			SET_EXTRA(temp, 0, NULL);
		if (GET_CAST_CHAR(temp) == ch)
			SET_CAST(temp, 0, NULL, NULL, NULL);
		if (FIGHTING(temp) == ch && switch_others) {
			log("[Stop fighting] %s : Change victim for fighting", GET_NAME(temp));
			for (found = combat_list; found; found = found->next_fighting)
				if (found != ch && FIGHTING(found) == temp) {
					act("Вы переключили свое внимание на $N3.", FALSE, temp, 0, found, TO_CHAR);
					FIGHTING(temp) = found;
					break;
				}
			if (!found)
				stop_fighting(temp, FALSE);
		}
	}
	update_pos(ch);
}

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer)
{
	OBJ_DATA *corpse;
	EXTRA_DESCR_DATA *exdesc;

	corpse = create_obj();
	corpse->item_number = NOTHING;
	IN_ROOM(corpse) = NOWHERE;
	GET_OBJ_SEX(corpse) = SEX_POLY;

	sprintf(buf2, "Останки %s лежат на земле.", GET_PAD(ch, 1));
	corpse->description = str_dup(buf2);

	sprintf(buf2, "останки %s", GET_PAD(ch, 1));
	corpse->short_description = str_dup(buf2);

	sprintf(buf2, "останки %s", GET_PAD(ch, 1));
	corpse->PNames[0] = str_dup(buf2);
	corpse->name = str_dup(buf2);

	sprintf(buf2, "останков %s", GET_PAD(ch, 1));
	corpse->PNames[1] = str_dup(buf2);
	sprintf(buf2, "останкам %s", GET_PAD(ch, 1));
	corpse->PNames[2] = str_dup(buf2);
	sprintf(buf2, "останки %s", GET_PAD(ch, 1));
	corpse->PNames[3] = str_dup(buf2);
	sprintf(buf2, "останками %s", GET_PAD(ch, 1));
	corpse->PNames[4] = str_dup(buf2);
	sprintf(buf2, "останках %s", GET_PAD(ch, 1));
	corpse->PNames[5] = str_dup(buf2);

	GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(corpse, ITEM_NODONATE) = ITEM_NODONATE;
	GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);
	GET_OBJ_RENT(corpse) = 100000;
	GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;
	CREATE(exdesc, EXTRA_DESCR_DATA, 1);
	exdesc->keyword = str_dup(corpse->PNames[0]);	// косметика
	if (killer)
		sprintf(buf, "Убит%s на арене %s.\r\n", GET_CH_SUF_6(ch), GET_PAD(killer, 4));
	else
		sprintf(buf, "Умер%s на арене.\r\n", GET_CH_SUF_4(ch));
	exdesc->description = str_dup(buf);	// косметика
	exdesc->next = corpse->ex_description;
	corpse->ex_description = exdesc;
	obj_to_room(corpse, IN_ROOM(ch));
}





OBJ_DATA *make_corpse(CHAR_DATA * ch)
{
	OBJ_DATA *corpse, *o;
	OBJ_DATA *money;
	int i;

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_CORPSE))
		return NULL;

	corpse = create_obj();
	corpse->item_number = NOTHING;
	IN_ROOM(corpse) = NOWHERE;
	GET_OBJ_SEX(corpse) = SEX_MALE;

	sprintf(buf2, "Труп %s лежит здесь.", GET_PAD(ch, 1));
	corpse->description = str_dup(buf2);

	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse->short_description = str_dup(buf2);

	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse->PNames[0] = str_dup(buf2);
	corpse->name = str_dup(buf2);

	sprintf(buf2, "трупа %s", GET_PAD(ch, 1));
	corpse->PNames[1] = str_dup(buf2);
	sprintf(buf2, "трупу %s", GET_PAD(ch, 1));
	corpse->PNames[2] = str_dup(buf2);
	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse->PNames[3] = str_dup(buf2);
	sprintf(buf2, "трупом %s", GET_PAD(ch, 1));
	corpse->PNames[4] = str_dup(buf2);
	sprintf(buf2, "трупе %s", GET_PAD(ch, 1));
	corpse->PNames[5] = str_dup(buf2);

	GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(corpse, ITEM_NODONATE) |= ITEM_NODONATE;
	GET_OBJ_EXTRA(corpse, ITEM_NOSELL) |= ITEM_NOSELL;
	GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
	GET_OBJ_RENT(corpse) = 100000;
	if (IS_NPC(ch))
		GET_OBJ_TIMER(corpse) = max_npc_corpse_time * 2;
	else
		GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;



	/* transfer character's equipment to the corpse */
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i)) {
			remove_otrigger(GET_EQ(ch, i), ch);
			obj_to_char(unequip_char(ch, i), ch);
		}
	// Считаем вес шмоток после того как разденем чара
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);

	/* transfer character's inventory to the corpse */
	corpse->contains = ch->carrying;
	for (o = corpse->contains; o != NULL; o = o->next_content) {
		o->in_obj = corpse;
	}
	object_list_new_owner(corpse, NULL);


	/* transfer gold */
	if (GET_GOLD(ch) > 0) {	/* following 'if' clause added to fix gold duplication loophole */
		if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
			money = create_money(GET_GOLD(ch));
			obj_to_obj(money, corpse);
		}
		GET_GOLD(ch) = 0;
	}

	ch->carrying = NULL;
	IS_CARRYING_N(ch) = 0;
	IS_CARRYING_W(ch) = 0;

	if (IS_NPC(ch) && mob_proto[GET_MOB_RNUM(ch)].ing_list)
		im_make_corpse(corpse, mob_proto[GET_MOB_RNUM(ch)].ing_list);

	// Загружаю шмотки по листу. - перемещено в raw_kill
/*  if (IS_NPC (ch))
    dl_load_obj (corpse, ch); */

	obj_to_room(corpse, IN_ROOM(ch));
	return corpse;
}


/* When ch kills victim */
void change_alignment(CHAR_DATA * ch, CHAR_DATA * victim)
{
	/*
	 * new alignment change algorithm: if you kill a monster with alignment A,
	 * you move 1/16th of the way to having alignment -A.  Simple and fast.
	 */
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(CHAR_DATA * ch)
{
	int door;

	act("Кровушка стынет в жилах от предсмертного крика $n1.", FALSE, ch, 0, 0, TO_ROOM | CHECK_DEAF);

	for (door = 0; door < NUM_OF_DIRS; door++)
		if (CAN_GO(ch, door)) {
			act("Кровушка стынет в жилах от чьего-то предсмертного крика.",
			    FALSE,
			    world[world[IN_ROOM(ch)]->dir_option[door]->to_room]->people, 0, 0, TO_CHAR | CHECK_DEAF);
			act("Кровушка стынет в жилах от чьего-то предсмертного крика.",
			    FALSE,
			    world[world[IN_ROOM(ch)]->dir_option[door]->to_room]->people, 0, 0, TO_ROOM | CHECK_DEAF);
		}
//         send_to_room("Кровушка стынет в жилах от чьего-то предсмертного крика.\r\n",
//                      world[IN_ROOM(ch)]->dir_option[door]->to_room, TRUE
//                     );
}



void raw_kill(CHAR_DATA * ch, CHAR_DATA * killer)
{
	CHAR_DATA *hitter;
	OBJ_DATA *corpse = NULL;
	AFFECT_DATA *af, *naf;
	int to_room;
	long local_gold = 0;
	char obj[256];


	if (FIGHTING(ch))
		stop_fighting(ch, TRUE);

	for (hitter = combat_list; hitter; hitter = hitter->next_fighting)
		if (FIGHTING(hitter) == ch)
			WAIT_STATE(hitter, 0);

	supress_godsapply = TRUE;
	for (af = ch->affected; af; af = naf) {
		naf = af->next;
		if (!IS_SET(af->battleflag, AF_DEADKEEP))
			affect_remove(ch, af);
	}
	supress_godsapply = FALSE;
	affect_total(ch);

	if (!killer || death_mtrigger(ch, killer))
		if (IN_ROOM(ch) != NOWHERE)
			death_cry(ch);

	if (IN_ROOM(ch) != NOWHERE) {
		if (!IS_NPC(ch) && !RENTABLE(ch)
		    && ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)) {
			make_arena_corpse(ch, killer);
			change_fighting(ch, TRUE);
//          FORGET_ALL(ch);
			GET_HIT(ch) = 1;
			GET_POS(ch) = POS_SITTING;
			char_from_room(ch);
			if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE) {
				SET_BIT(PLR_FLAGS(ch, PLR_HELLED), PLR_HELLED);
				HELL_DURATION(ch) = time(0) + 6;
				to_room = r_helled_start_room;
			}
			char_to_room(ch, to_room);
			look_at_room(ch, to_room);
			act("$n со стонами упал$g с небес...", FALSE, ch, 0, 0, TO_ROOM);
		} else {

			local_gold = GET_GOLD(ch);
			corpse = make_corpse(ch);

//send_to_char (buf,killer);
/* Начало изменений.
   (с) Дмитрий ака dzMUDiST */

// Теперь реализация режимов "автограбеж" и "брать куны" происходит не в damage,
// а здесь, после создания соответствующего трупа. Кроме того, 
// если убил чармис и хозяин в комнате, то автолут происходит хозяину
			if ((ch != NULL) && (killer != NULL)) {
				if (IS_NPC(ch) && !IS_NPC(killer) && PRF_FLAGGED(killer, PRF_AUTOLOOT)
				    && (corpse != NULL) && can_loot(killer)) {
					sprintf(obj, "all");
					get_from_container(killer, corpse, obj, FIND_OBJ_INV, 1);
				} else if (IS_NPC(ch) && !IS_NPC(killer) && local_gold
					   && PRF_FLAGGED(killer, PRF_AUTOMONEY) && (corpse != NULL)
					   && can_loot(killer)) {
					sprintf(obj, "all.coin");
					get_from_container(killer, corpse, obj, FIND_OBJ_INV, 1);
				} else if (IS_NPC(ch) && IS_NPC(killer) && AFF_FLAGGED(killer, AFF_CHARM)
					   && (corpse != NULL) && killer->master
					   && killer->in_room == killer->master->in_room
					   && PRF_FLAGGED(killer->master, PRF_AUTOLOOT) && can_loot(killer->master)) {
					sprintf(obj, "all");
					get_from_container(killer->master, corpse, obj, FIND_OBJ_INV, 1);
				} else if (IS_NPC(ch) && IS_NPC(killer) && local_gold && AFF_FLAGGED(killer, AFF_CHARM)
					   && (corpse != NULL) && killer->master
					   && killer->in_room == killer->master->in_room
					   && PRF_FLAGGED(killer->master, PRF_AUTOMONEY) && can_loot(killer->master)) {
					sprintf(obj, "all.coin");
					get_from_container(killer->master, corpse, obj, FIND_OBJ_INV, 1);
				}
			}

/* Конец изменений.
   (с) Дмитрий ака dzMUDiST */

			if (!IS_NPC(ch)) {
				FORGET_ALL(ch);
				for (hitter = character_list; hitter; hitter = hitter->next)
					if (IS_NPC(hitter) && MEMORY(hitter))
						forget(hitter, ch);
				/*
				   for (hitter = character_list; hitter && IS_NPC(hitter) && MEMORY(hitter); hitter = hitter->next)
				   forget(hitter, ch);
				 */
			} else {
				dl_load_obj(corpse, ch, NULL, DL_ORDINARY);
				dl_load_obj(corpse, ch, NULL, DL_PROGRESSION);
			}
			/* Если убит в бою - то может выйти из игры */
			if (!IS_NPC(ch)) {
				RENTABLE(ch) = 0;
				AGRESSOR(ch) = 0;
				AGRO(ch) = 0;
			}
			extract_char(ch, TRUE);
		}
	}
}

/* Функция используемая при "автограбеже" и "автолуте",
   чтобы не было лута под холдом или в слепи             */
int can_loot(CHAR_DATA * ch)
{
	if (ch != NULL) {
		if (!IS_NPC(ch) && GET_MOB_HOLD(ch) == 0 &&	// если под холдом
		    !AFF_FLAGGED(ch, AFF_STOPFIGHT) &&	// парализован точкой
		    !AFF_FLAGGED(ch, AFF_BLIND))	// слеп
			return TRUE;
	}
	return FALSE;
}

void die(CHAR_DATA * ch, CHAR_DATA * killer)
{
	CHAR_DATA *master = NULL;
	struct follow_type *f;

	if (!IS_NPC(ch) && (IN_ROOM(ch) == NOWHERE)) {
		log("SYSERR: %s is dying in room NOWHERE.", GET_NAME(ch));
		return;
	}

	if (IS_NPC(ch) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)
	    || RENTABLE(ch)) {
		if (!(IS_NPC(ch) || IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))) {
			int dec_exp, e = GET_EXP(ch);
			dec_exp = number(GET_EXP(ch) / 100, GET_EXP(ch) / 20) +
			    (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch))) / 4;
			gain_exp(ch, -dec_exp);
			dec_exp = e - GET_EXP(ch);
			sprintf(buf, "Вы потеряли %d %s опыта.\r\n", dec_exp, desc_count(dec_exp, WHAT_POINT));
			send_to_char(buf, ch);
		}

		/* Вычисляем замакс по мобам */
		/* Решил немножко переделать, чтобы короче получилось,         */
		/* кроме того, исправил ошибку с присутствием лидера в комнате */
		if (IS_NPC(ch) && killer) {
			if (IS_NPC(killer) &&
			    (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL)) && killer->master)
				master = killer->master;
			else if (!IS_NPC(killer))
				master = killer;

			// На этот момент master - PC

			if (master) {
				if (AFF_FLAGGED(master, AFF_GROUP)) {
					int cnt = 0;

					// master - член группы, переходим на лидера группы
					if (master->master)
						master = master->master;
					if (IN_ROOM(master) == IN_ROOM(killer)) {
						// лидер группы в тойже комнате, что и убивец
						cnt = 1;
					}

					for (f = master->followers; f; f = f->next) {
						if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
						    IN_ROOM(f->follower) == IN_ROOM(killer)) {
							if (!number(0, cnt))
								master = f->follower;
							++cnt;
						}
					}
				}
				inc_kill_vnum(master, GET_MOB_VNUM(ch), 1);
			}
		}

		/* train LEADERSHIP */

		if (IS_NPC(ch) && killer)
			if (!IS_NPC(killer) &&
			    AFF_FLAGGED(killer, AFF_GROUP) &&
			    killer->master &&
			    GET_SKILL(killer->master, SKILL_LEADERSHIP) > 0 &&
			    IN_ROOM(killer) == IN_ROOM(killer->master))
				improove_skill(killer->master, SKILL_LEADERSHIP, number(0, 1), ch);

		if (!IS_NPC(ch) && killer) {	/* decrease LEADERSHIP */
			if (IS_NPC(killer) &&
			    AFF_FLAGGED(ch, AFF_GROUP) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
				if (GET_SKILL(ch->master, SKILL_LEADERSHIP) > 1)
					GET_SKILL(ch->master, SKILL_LEADERSHIP)--;
			}
		}
		pk_revenge_action(killer, ch);
	}
	raw_kill(ch, killer);
	// if (killer)
	//   log("Killer lag is %d", GET_WAIT(killer));
}

int get_extend_exp(int exp, CHAR_DATA * ch, CHAR_DATA * victim)
{
	int base, diff;
	int koef;

	if (!IS_NPC(victim) || IS_NPC(ch))
		return (exp);

	for (koef = 100, base = 0, diff = get_kill_vnum(ch, GET_MOB_VNUM(victim));
	     base < diff && koef > 5; base++, koef = koef * 95 / 100);

// log("[Expierence] Mob %s - %d %d(%d) %d",GET_NAME(victim),exp,base,diff,koef);
// Experience scaling introduced - the next line is not needed any more
// exp = exp * MAX(1, 100 - GET_REMORT(ch) * 10) / 100;
	exp = exp * MAX(5, koef) / 100;

	// if (!(base = victim->mob_specials.MaxFactor))
	//    return (exp);
	//
	// if ((diff = get_kill_vnum(ch,GET_MOB_VNUM(victim)) - base) <= 0)
	//    return (exp);
	// exp = exp * base / (base+diff);

	return (exp);
}

/*++
   Функция начисления опыта
      ch - кому опыт начислять
           Вызов этой функции для NPC смысла не имеет, но все равно
           какие-то проверки внутри зачем то делаются
--*/
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef)
{
	int exp;

// Странно, но для NPC эта функция тоже должна работать
//  if (IS_NPC(ch) || !OK_GAIN_EXP(ch,victim))
	if (!OK_GAIN_EXP(ch, victim)) {
		send_to_char("Ваше деяние никто не оценил.\r\n", ch);
		return;
	}
	// 1. Опыт делится поровну на всех
	exp = GET_EXP(victim) / MAX(members, 1);

	// 2. Учитывается коэффициент (лидерство, разность уровней)
	//    На мой взгляд его правильней использовать тут а не в конце процедуры,
	//    хотя в большинстве случаев это все равно
	exp = exp * koef / 100;

	// 3. Вычисление опыта для PC и NPC
	if (IS_NPC(ch)) {
		exp = MIN(max_exp_gain_npc, exp);
		exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	} else {
		exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim));
	}

	// 4. Последняя проверка
	exp = MAX(1, exp);

	if (exp > 1) {
		sprintf(buf2, "Ваш опыт повысился на %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
		send_to_char(buf2, ch);
	} else
		send_to_char("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);

	gain_exp(ch, exp);
	change_alignment(ch, victim);
}


/*++
   Функция расчитывает всякие бонусы для группы при получении опыта,
 после чего вызывает функцию получения опыта для всех членов группы
 Т.к. членом группы может быть только PC, то эта функция раздаст опыт только PC

   ch - обязательно член группы, из чего следует:
            1. Это не NPC
            2. Он находится в группе лидера (или сам лидер)
   
   Просто для PC-последователей эта функция не вызывается

--*/
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int inroom_members, koef = 100, maxlevel, rmrt;
	CHAR_DATA *k;
	struct follow_type *f;
	int leader_inroom;

	maxlevel = GET_LEVEL(ch);

	if (!(k = ch->master))
		k = ch;

	// k - подозрение на лидера группы
	leader_inroom = (AFF_FLAGGED(k, AFF_GROUP)
			 && (k->in_room == IN_ROOM(ch)));

	// Количество согрупников в комнате
	if (leader_inroom) {
		inroom_members = 1;
		maxlevel = GET_LEVEL(k);
	} else
		inroom_members = 0;

	// Вычисляем максимальный уровень в группе
	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch)) {
			// просмотр членов группы в той же комнате
			// член группы => PC автоматически
			++inroom_members;
			maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
		}

	// Вычисляем, надо ли резать экспу, смотрим сначала лидера, если он рядом
	rmrt = MIN(14, (int)GET_REMORT(k));
	if (maxlevel - GET_LEVEL(k) > grouping[(int)GET_CLASS(k)][rmrt] && leader_inroom)
		koef -= 50;
	else	// если с лидером все ок либо он не тут, смотрим по группе
		for (f = k->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch)) {
				rmrt = MIN(14, (int)GET_REMORT(f->follower));
				if (maxlevel - GET_LEVEL(f->follower) >
				    grouping[(int)GET_CLASS(f->follower)][rmrt]) {
					koef -= 50;
					break;
				}
			}

	// Лидерство используется, если в комнате лидер и есть еще хоть кто-то
	// из группы из PC (последователи типа лошади или чармисов не считаются)
	if (leader_inroom && (inroom_members > 1) && calc_leadership(k))
		koef += 20;

	// Раздача опыта

	if (leader_inroom)
		perform_group_gain(k, victim, inroom_members, koef);

	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
			perform_group_gain(f->follower, victim, inroom_members, koef);
}

/*
void solo_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int exp;

  if (IS_NPC(ch) || !OK_GAIN_EXP(ch, victim))
     {send_to_char("Ваше деяние никто не оценил.\r\n",ch);
      return;
     }

  if (IS_NPC(ch))
     {exp  = MIN(max_exp_gain_npc, GET_EXP(victim));
      exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
     }
  else
     {exp = get_extend_exp(GET_EXP(victim), ch, victim);
      exp = MIN(max_exp_gain_pc(ch), exp);
     };

  if (!IS_NPC(ch))
     exp = MIN(max_exp_gain_pc(ch),exp);
  exp = MAX(1,exp);

  if (exp > 1)
     {sprintf(buf2, "Ваш опыт повысился на %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
      send_to_char(buf2, ch);
     }
  else
    send_to_char("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}
*/


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
	static char buf[256];
	char *cp = buf;

	for (; *str; str++) {
		if (*str == '#') {
			switch (*(++str)) {
			case 'W':
				for (; *weapon_plural; *(cp++) = *(weapon_plural++));
				break;
			case 'w':
				for (; *weapon_singular; *(cp++) = *(weapon_singular++));
				break;
			default:
				*(cp++) = '#';
				break;
			}
		} else
			*(cp++) = *str;

		*cp = 0;
	}			/* For */

	return (buf);
}

/* message for doing damage with a weapon */
void dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim, int w_type)
{
	char *buf;
	int msgnum;

	static struct dam_weapon_type {
		const char *to_room;
		const char *to_char;
		const char *to_victim;
	} dam_weapons[] = {

		/* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

		{
			"$n попытал$u #W $N3, но промахнул$u.",	/* 0: 0      0 */
		"Вы попытались #W $N3, но промахнулись.", "$n попытал$u #W Вас, но промахнул$u."}, {
			"$n легонько #w$g $N3.",	/*  1..5 1 */
		"Вы легонько #wи $N3.", "$n легонько #w$g Вас."}, {
			"$n слегка #w$g $N3.",	/*  6..10  2 */
		"Вы слегка #wи $N3.", "$n слегка #w$g Вас."}, {
			"$n #w$g $N3.",	/*  11..15   3 */
		"Вы #wи $N3.", "$n #w$g Вас."}, {
			"$n #w$g $N3.",	/* 16..20  4 */
		"Вы #wи $N3.", "$n #w$g Вас."}, {
			"$n сильно #w$g $N3.",	/* 21..25  5 */
		"Вы сильно #wи $N3.", "$n сильно #w$g Вас."}, {
			"$n очень сильно #w$g $N3.",	/*  26..30 6  */
		"Вы очень сильно #wи $N3.", "$n очень сильно #w$g Вас."}, {
			"$n чрезвычайно сильно #w$g $N3.",	/*  31..35  7 */
		"Вы чрезвычайно сильно #wи $N3.", "$n чрезвычайно сильно #w$g Вас."}, {
			"$n ЖЕСТОКО #w$g $N3.",	/*  36..44  8 */
		"Вы ЖЕСТОКО #wи $N3.", "$n ЖЕСТОКО #w$g Вас."}, {
			"$n БОЛЬНО #w$g $N3.",	/*  45..58   9 */
		"Вы БОЛЬНО #wи $N3.", "$n БОЛЬНО #w$g Вас."}, {
			"$n ОЧЕНЬ БОЛЬНО #w$g $N3.",	/*    59..72 10  */
		"Вы ОЧЕНЬ БОЛЬНО #wи $N3.", "$n ОЧЕНЬ БОЛЬНО #w$g Вас."}, {
			"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g $N3.",	/*    73..86  11 */
		"Вы ЧРЕЗВЫЧАЙНО БОЛЬНО #wи $N3.", "$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g Вас."}, {
			"$n НЕВЫНОСИМО БОЛЬНО #w$g $N3.",	/*    87..100  12 */
		"Вы НЕВЫНОСИМО БОЛЬНО #wи $N3.", "$n НЕВЫНОСИМО БОЛЬНО #w$g Вас."}, {
			"$n УЖАСНО #w$g $N3.",	/*    101..150  13 */
		"Вы УЖАСНО #wи $N3.", "$n УЖАСНО #w$g Вас."}, {
			"$n СМЕРТЕЛЬНО #w$g $N3.",	/* > 150  14 */
		"Вы СМЕРТЕЛЬНО #wи $N3.", "$n СМЕРТЕЛЬНО #w$g Вас."}
	};


	if (w_type >= TYPE_HIT && w_type < TYPE_MAGIC)
		w_type -= TYPE_HIT;	/* Change to base of table with text */
	else
		w_type = TYPE_HIT;
// Изменено Дажьбогом
	if (dam == 0)
		msgnum = 0;
	else if (dam <= 5)
		msgnum = 1;
	else if (dam <= 10)
		msgnum = 2;
	else if (dam <= 15)
		msgnum = 3;
	else if (dam <= 20)
		msgnum = 4;
	else if (dam <= 25)
		msgnum = 5;
	else if (dam <= 30)
		msgnum = 6;
	else if (dam <= 35)
		msgnum = 7;
	else if (dam <= 44)
		msgnum = 8;
	else if (dam <= 58)
		msgnum = 9;
	else if (dam <= 72)
		msgnum = 10;
	else if (dam <= 86)
		msgnum = 11;
	else if (dam <= 100)
		msgnum = 12;
	else if (dam <= 150)
		msgnum = 13;
	else
		msgnum = 14;
//
	/* damage message to onlookers */

	buf = replace_string(dam_weapons[msgnum].to_room,
			     attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

	/* damage message to damager */
	if (dam)
		send_to_char(CCIYEL(ch, C_CMP), ch);
	else
		send_to_char(CCYEL(ch, C_CMP), ch);
	buf = replace_string(dam_weapons[msgnum].to_char,
			     attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);

	/* damage message to damagee */
	if (dam)
		send_to_char(CCIRED(victim, C_CMP), victim);
	else
		send_to_char(CCIRED(victim, C_CMP), victim);
	buf = replace_string(dam_weapons[msgnum].to_victim,
			     attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(victim, C_CMP), victim);
//  sprintf(buf,"Нанесенные повреждения - %d\n",dam);
//  send_to_char(buf,ch);
}

/* Alterate equipment
 *
 */
void alterate_object(OBJ_DATA * obj, int dam, int chance)
{
	if (!obj)
		return;
	dam = number(0, dam * (material_value[GET_OBJ_MATER(obj)] + 30) /
		     MAX(1, GET_OBJ_MAX(obj) *
			 (IS_OBJ_STAT(obj, ITEM_NODROP) ? 5 :
			  IS_OBJ_STAT(obj, ITEM_BLESS) ? 15 : 10) * (GET_OBJ_SKILL(obj) == SKILL_BOWS ? 3 : 1)));

	if (dam > 0 && chance >= number(1, 100)) {
		if ((GET_OBJ_CUR(obj) -= dam) <= 0) {
			if (obj->worn_by)
				act("$o рассыпал$U, не выдержав повреждений.", FALSE, obj->worn_by, obj, 0, TO_CHAR);
			else if (obj->carried_by)
				act("$o рассыпал$U, не выдержав повреждений.", FALSE, obj->carried_by, obj, 0, TO_CHAR);
			extract_obj(obj);
		}
	}
}

void alt_equip(CHAR_DATA * ch, int pos, int dam, int chance)
{
	// calculate chance if
	if (pos == NOWHERE) {
		pos = number(0, 100);
		if (pos < 3)
			pos = WEAR_FINGER_R + number(0, 1);
		else if (pos < 6)
			pos = WEAR_NECK_1 + number(0, 1);
		else if (pos < 20)
			pos = WEAR_BODY;
		else if (pos < 30)
			pos = WEAR_HEAD;
		else if (pos < 45)
			pos = WEAR_LEGS;
		else if (pos < 50)
			pos = WEAR_FEET;
		else if (pos < 58)
			pos = WEAR_HANDS;
		else if (pos < 66)
			pos = WEAR_ARMS;
		else if (pos < 76)
			pos = WEAR_SHIELD;
		else if (pos < 86)
			pos = WEAR_ABOUT;
		else if (pos < 90)
			pos = WEAR_WAIST;
		else if (pos < 94)
			pos = WEAR_WRIST_R + number(0, 1);
		else
			pos = WEAR_HOLD;
	}

	if (pos <= 0 || pos > WEAR_BOTHS || !GET_EQ(ch, pos) || dam < 0)
		return;
	alterate_object(GET_EQ(ch, pos), dam, chance);
}

/*  Global variables for critical damage */
int was_critic = FALSE;
int dam_critic = 0;

void haemorragia(CHAR_DATA * ch, int percent)
{
	AFFECT_DATA af[3];
	int i;

	af[0].type = SPELL_HAEMORRAGIA;
	af[0].location = APPLY_HITREG;
	af[0].modifier = -percent;
	af[0].duration = pc_duration(ch, number(1, 31 + con_app[GET_REAL_CON(ch)].critic_saving), 0, 0, 0, 0);
	af[0].bitvector = 0;
	af[0].battleflag = 0;
	af[1].type = SPELL_HAEMORRAGIA;
	af[1].location = APPLY_MOVEREG;
	af[1].modifier = -percent;
	af[1].duration = af[0].duration;
	af[1].bitvector = 0;
	af[1].battleflag = 0;
	af[2].type = SPELL_HAEMORRAGIA;
	af[2].location = APPLY_MANAREG;
	af[2].modifier = -percent;
	af[2].duration = af[0].duration;
	af[2].bitvector = 0;
	af[2].battleflag = 0;

	for (i = 0; i < 3; i++)
		affect_join(ch, &af[i], TRUE, FALSE, TRUE, FALSE);
}


int compute_critical(CHAR_DATA * ch, CHAR_DATA * victim, int dam)
{
	char *to_char = NULL, *to_vict = NULL;
	AFFECT_DATA af[4];
	OBJ_DATA *obj;
	int i, unequip_pos = 0;

	if (!dam_critic)
		return(dam);

	for (i = 0; i < 4; i++) {
		af[i].type = 0;
		af[i].location = APPLY_NONE;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].duration = pc_duration(victim, 2, 0, 0, 0, 0);
	}

	was_critic = FALSE;
	switch (number(1, 10)) {
	case 1:
	case 2:
	case 3:
	case 4:		// FEETS
		switch (dam_critic) {
		case 1:
		case 2:
		case 3:
			// Nothing
			return dam;
		case 5:	// Hit genus, victim bashed, speed/2
			SET_AF_BATTLE(victim, EAF_SLOW);
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 10);
		case 4:	// victim bashed
			if (GET_POS (victim) > POS_SITTING)
				GET_POS (victim) = POS_SITTING;
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "повалило $N3 на землю";
			to_vict = "повредило Вам колено";
			break;
		case 6:	// foot damaged, speed/2
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL)/ 9);
			to_char = "замедлило движения $N1";
			to_vict = "сломало Вам лодыжку";
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 7:
		case 9:	// armor damaged else foot damaged, speed/4
			if (GET_EQ(victim, WEAR_LEGS))
				alt_equip(victim, WEAR_LEGS, 100, 100);
			else {
				dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 8);
				to_char = "замедлило движения $N1";
				to_vict = "сломало Вам ногу";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_NOFLEE;
				SET_AF_BATTLE(victim, EAF_SLOW);
			}
			break;
		case 8:	// femor damaged, no speed
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 7);
			to_char = "сильно замедлило движения $N1";
			to_vict = "сломало Вам бедро";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_NOFLEE;
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// genus damaged, no speed, -2HR
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 7);
			to_char = "сильно замедлило движения $N1";
			to_vict = "раздробило Вам колено";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = AFF_NOFLEE;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// femor damaged, no speed, no attack
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 7);
			to_char = "вывело $N3 из строя";
			to_vict = "раздробило Вам бедро";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		default:	// femor damaged, no speed, no attack
			if (dam_critic > 12)
				dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 5);
			else
				dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 6);
			to_char = "вывело $N3 из строя";
			to_vict = "изуродовало Вам ногу";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 50);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	case 5:		//  ABDOMINAL
		switch (dam_critic) {
		case 1:
		case 2:
		case 3:
			// nothing
			return dam;
		case 4:	// waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			to_char = "сбило $N2 дыхание";
			to_vict = "сбило Вам дыхание";
			break;

		case 5:	// abdomin damaged, waits 1, speed/2
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 8);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "ранило $N3 в живот";
			to_vict = "ранило Вас в живот";
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 6:	// armor damaged else dam*3, waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_WAIST))
				alt_equip(victim, WEAR_WAIST, 100, 100);
			else
				dam *= (GET_SKILL (ch, SKILL_PUNCTUAL)/ 7);
			to_char = "повредило $N2 живот";
			to_vict = "повредило Вам живот";
			break;
		case 7:
		case 8:	// abdomin damage, speed/2, HR-2
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 6);
			to_char = "ранило $N3 в живот";
			to_vict = "ранило Вас в живот";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = AFF_NOFLEE;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 9:	// armor damaged, abdomin damaged, speed/2, HR-2
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 5);
			alt_equip(victim, WEAR_BODY, 100, 100);
			to_char = "ранило $N3 в живот";
			to_vict = "ранило Вас в живот";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = AFF_NOFLEE;
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// abdomin damaged, no speed, no attack
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 4);
			to_char = "повредило $N2 живот";
			to_vict = "повредило Вам живот";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// abdomin damaged, no speed, no attack
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 3);
			to_char = "разорвало $N2 живот";
			to_vict = "разорвало Вам живот";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 40);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		default:	// abdomin damaged, hits = 0
			dam *= GET_SKILL (ch, SKILL_PUNCTUAL) / 2;
			to_char = "размозжило $N2 живот";
			to_vict = "размозжило Вам живот";
			haemorragia(victim, 60);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	case 6:
	case 7:		// CHEST
		switch (dam_critic) {
		case 1:
		case 2:
		case 3:
			// nothing
			return dam;
		case 4:	// waits 1d4, bashed
			WAIT_STATE(victim, number(2, 5) * PULSE_VIOLENCE);
			if (GET_POS (victim) > POS_SITTING)
				GET_POS(victim) = POS_SITTING;
			to_char = "повредило $N2 грудь";
			to_vict = "повредило Вам грудь";
			break;
		case 5:	// chest damaged, waits 1, speed/2
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 6);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "повредило $N2 туловище";
			to_vict = "повредило Вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_NOFLEE;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 6:	// shield damaged, chest damaged, speed/2
			alt_equip(victim, WEAR_SHIELD, 100, 100);
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 6);
			to_char = "повредило $N2 туловище";
			to_vict = "повредило Вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_NOFLEE;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 7:	// srmor damaged, chest damaged, speed/2, HR-2
			alt_equip(victim, WEAR_BODY, 100, 100);
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 5);
			to_char = "повредило $N2 туловище";
			to_vict = "повредило Вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = AFF_NOFLEE;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 8:	// chest damaged, no speed, no attack
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 5);
			to_char = "вывело $N3 из строя";
			to_vict = "повредило Вам туловище";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 9:	// chest damaged, speed/2, HR-2
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 4);
			to_char = "заставило $N3 ослабить натиск";
			to_vict = "сломало Вам ребра";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// chest damaged, no speed, no attack
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 4);
			to_char = "вывело $N3 из строя";
			to_vict = "сломало Вам ребра";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 40);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// chest crushed, hits 0
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			dam *= GET_SKILL (ch, SKILL_PUNCTUAL) / 2;
			haemorragia(victim, 50);
			to_char = "вывело $N3 из строя";
			to_vict = "разорвало Вам грудь";
			break;
		default:	// chest crushed, killing
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			dam *= GET_SKILL (ch, SKILL_PUNCTUAL);
			haemorragia(victim, 60);
			to_char = "вывело $N3 из строя";
			to_vict = "размозжило Вам грудь";
			break;
		}
		break;
	case 8:
	case 9:		// HANDS
		switch (dam_critic) {
		case 1:
		case 2:
		case 3:
			return dam;
		case 4:	// hands damaged, weapon/shield putdown
			to_char = "ослабило натиск $N1";
			to_vict = "ранило Вам руку";
			if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			else if (GET_EQ(victim, WEAR_SHIELD))
				unequip_pos = WEAR_SHIELD;
			break;
		case 5:	// hands damaged, shield damaged/weapon putdown
			to_char = "ослабило натиск $N1";
			to_vict = "ранило Вас в руку";
			if (GET_EQ(victim, WEAR_SHIELD))
				alt_equip(victim, WEAR_SHIELD, 100, 100);
			else if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			break;

		case 6:	// hands damaged, HR-2, shield putdown
			to_char = "ослабило натиск $N1";
			to_vict = "сломало Вам руку";
			if (GET_EQ(victim, WEAR_SHIELD))
				unequip_pos = WEAR_SHIELD;
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			break;
		case 7:	// armor damaged, hand damaged if no armour
			if (GET_EQ(victim, WEAR_ARMS))
				alt_equip(victim, WEAR_ARMS, 100, 100);
			else
				alt_equip(victim, WEAR_HANDS, 100, 100);
			if (!GET_EQ(victim, WEAR_ARMS) && !GET_EQ(victim, WEAR_HANDS))
				dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 7);
			to_char = "ослабило атаку $N1";
			to_vict = "повредило Вам руку";
			break;
		case 8:	// shield damaged, hands damaged, waits 1
			alt_equip(victim, WEAR_SHIELD, 100, 100);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 7);
			to_char = "придержало $N3";
			to_vict = "повредило Вам руку";
			break;
		case 9:	// weapon putdown, hands damaged, waits 1d4
			WAIT_STATE(victim, number(2, 4) * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 6);
			to_char = "придержало $N3";
			to_vict = "повредило Вам руку";
			break;
		case 10:	// hand damaged, no attack this
			if (!AFF_FLAGGED(victim, AFF_STOPRIGHT)) {
				to_char = "ослабило атаку $N1";
				to_vict = "изуродовало Вам правую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_STOPRIGHT;
				af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			} else if (!AFF_FLAGGED(victim, AFF_STOPLEFT)) {
				to_char = "ослабило атаку $N1";
				to_vict = "изуродовало Вам левую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_STOPLEFT;
				af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			} else {
				to_char = "вывело $N3 из строя";
				to_vict = "вывело Вас из строя";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_STOPFIGHT;
				af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			}
			haemorragia(victim, 20);
			break;
		default:	// no hand attack, no speed, dam*2 if >= 13
			if (!AFF_FLAGGED(victim, AFF_STOPRIGHT)) {
				to_char = "ослабило натиск $N1";
				to_vict = "изуродовало Вам правую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_STOPRIGHT;
				af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			} else if (!AFF_FLAGGED(victim, AFF_STOPLEFT)) {
				to_char = "ослабило натиск $N1";
				to_vict = "изуродовало Вам левую руку";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_STOPLEFT;
				af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			} else {
				to_char = "вывело $N3 из строя";
				to_vict = "вывело Вас из строя";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = AFF_STOPFIGHT;
				af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			}
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim, 30);
			if (dam_critic >= 13)
				dam *= GET_SKILL (ch, SKILL_PUNCTUAL) / 5;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	default:		// HEAD
		switch (dam_critic) {
		case 1:
		case 2:
		case 3:
			// nothing
			return dam;
		case 4:	// waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			to_char = "помутило $N2 сознание";
			to_vict = "помутило Ваше сознание";
			break;

		case 5:	// head damaged, cap putdown, waits 1, HR-2 if no cap
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_HEAD))
				unequip_pos = WEAR_HEAD;
			else {
				af[0].type = SPELL_BATTLE;
				af[0].location = APPLY_HITROLL;
				af[0].modifier = -2;
			}
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 4);
			to_char = "повредило $N2 голову";
			to_vict = "повредило Вам голову";
			break;
		case 6:	// head damaged
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 4);
			to_char = "повредило $N2 голову";
			to_vict = "повредило Вам голову";
			break;
		case 7:	// cap damaged, waits 1d6, speed/2, HR-4
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			alt_equip(victim, WEAR_HEAD, 100, 100);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -4;
			af[0].bitvector = AFF_NOFLEE;
			to_char = "ранило $N3 в голову";
			to_vict = "ранило Вас в голову";
			break;
		case 8:	// cap damaged, hits 0
			WAIT_STATE(victim, 4 * PULSE_VIOLENCE);
			alt_equip(victim, WEAR_HEAD, 100, 100);
		//dam = GET_HIT(victim);
			dam *= GET_SKILL (ch, SKILL_PUNCTUAL);
			to_char = "отбило у $N1 сознание";
			to_vict = "отбило у Вас сознание";
			haemorragia(victim, 20);
			break;
		case 9:	// head damaged, no speed, no attack
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration = pc_duration(victim, 1, 0, 0, 0, 0);
			haemorragia(victim, 30);
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 3);
			to_char = "повергло $N3 в оцепенение";
			to_vict = "повергло Вас в оцепенение";
			break;
		case 10:	// head damaged, -1 INT/WIS/CHA
			dam *= (GET_SKILL (ch, SKILL_PUNCTUAL) / 2);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -1;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -1;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -1;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			af[3].type = SPELL_BATTLE;
			af[3].bitvector = AFF_STOPFIGHT;
			af[3].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			haemorragia(victim, 50);
			to_char = "сорвало у $N1 крышу";
			to_vict = "сорвало у Вас крышу";
			break;
		case 11:	// hits 0, WIS/2, INT/2, CHA/2
			dam *= GET_SKILL (ch, SKILL_PUNCTUAL);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -GET_INT(victim) / 2;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -GET_WIS(victim) / 2;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -GET_CHA(victim) / 2;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			haemorragia(victim, 60);
			to_char = "сорвало у $N1 крышу";
			to_vict = "сорвало у Вас крышу";
			break;
		default:	// killed
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -GET_INT(victim) / 2;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -GET_WIS(victim) / 2;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -GET_CHA(victim) / 2;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			dam *= GET_SKILL (ch, SKILL_PUNCTUAL);
			to_char = "размозжило $N2 голову";
			to_vict = "размозжило Вам голову";
			haemorragia(victim, 90);
			break;
		}
		break;
	}

	for (i = 0; i < 4; i++)
		if (af[i].type)
			affect_join(victim, af + i, TRUE, FALSE, TRUE, FALSE);
	if (unequip_pos && GET_EQ(victim, unequip_pos)) {
		obj = unequip_char(victim, unequip_pos);
		if (!IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
			obj_to_char(obj, victim);
		else
			obj_to_room(obj, IN_ROOM(victim));
		obj_decay(obj);
	}
	if (to_char) {
		sprintf(buf, "%sВаше точное попадание %s.%s", CCIGRN(ch, C_NRM), to_char, CCNRM(ch, C_NRM));
		act(buf, FALSE, ch, 0, victim, TO_CHAR);
		sprintf(buf, "Точное попадание $n1 %s.", to_char);
		act(buf, TRUE, ch, 0, victim, TO_NOTVICT);
	}
	if (to_vict) {
		sprintf(buf, "%sМеткое попадание $n1 %s.%s", CCIRED(victim, C_NRM), to_vict, CCNRM(victim, C_NRM));
		act(buf, FALSE, ch, 0, victim, TO_VICT);
	}
	if (!IS_NPC(victim)){
	    dam /= 5;
	    return calculate_resistance_coeff(victim, VITALITY_RESISTANCE +
							    GET_LEVEL(victim) + GET_REMORT(victim), dam);
	} else
	    return calculate_resistance_coeff(victim, VITALITY_RESISTANCE, dam);
}

void poison_victim(CHAR_DATA * ch, CHAR_DATA * vict, int modifier)
{
	AFFECT_DATA af[4];
	int i;

	/* change strength */
	af[0].type = SPELL_POISON;
	af[0].location = APPLY_STR;
	af[0].duration = pc_duration(vict, 0, MAX(2, GET_LEVEL(ch) - GET_LEVEL(vict)), 2, 0, 1);
	af[0].modifier = -MIN(2, (modifier + 29) / 40);
	af[0].bitvector = AFF_POISON;
	af[0].battleflag = 0;
	/* change damroll */
	af[1].type = SPELL_POISON;
	af[1].location = APPLY_DAMROLL;
	af[1].duration = af[0].duration;
	af[1].modifier = -MIN(2, (modifier + 29) / 30);
	af[1].bitvector = AFF_POISON;
	af[1].battleflag = 0;
	/* change hitroll */
	af[2].type = SPELL_POISON;
	af[2].location = APPLY_HITROLL;
	af[2].duration = af[0].duration;
	af[2].modifier = -MIN(2, (modifier + 19) / 20);
	af[2].bitvector = AFF_POISON;
	af[2].battleflag = 0;
	/* change poison level */
	af[3].type = SPELL_POISON;
	af[3].location = APPLY_POISON;
	af[3].duration = af[0].duration;
	af[3].modifier = GET_LEVEL(ch);
	af[3].bitvector = AFF_POISON;
	af[3].battleflag = 0;

	for (i = 0; i < 4; i++)
		affect_join(vict, af + i, FALSE, FALSE, FALSE, FALSE);
	vict->Poisoner = GET_ID(ch);
	act("Вы отравили $N3.", FALSE, ch, 0, vict, TO_CHAR);
	act("$n отравил$g Вас.", FALSE, ch, 0, vict, TO_VICT);
}

int extdamage(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int attacktype, OBJ_DATA * wielded, int mayflee)
{
	int prob, percent = 0, lag = 0, i, mem_dam = dam;
	AFFECT_DATA af;

	if (!victim) {
		return (0);
	}

	if (dam < 0)
		dam = 0;

	// MIGHT_HIT
	if (attacktype == TYPE_HIT && GET_AF_BATTLE(ch, EAF_MIGHTHIT) && GET_WAIT(ch) <= 0) {
		CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
		if (IS_NPC(ch) ||
		    IS_IMMORTAL(ch) ||
		    !(GET_EQ(ch, WEAR_BOTHS) || GET_EQ(ch, WEAR_WIELD) ||
		      GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_LIGHT) ||
		      GET_EQ(ch, WEAR_SHIELD) || GET_AF_BATTLE(ch, EAF_TOUCH))) {
			percent = number(1, skill_info[SKILL_MIGHTHIT].max_percent);
			prob = train_skill(ch, SKILL_MIGHTHIT, skill_info[SKILL_MIGHTHIT].max_percent, victim);
			if (GET_MOB_HOLD(victim))
				prob = MAX(prob, percent);
			if (IS_IMMORTAL(victim))
				prob = 0;
			if (prob * 100 / percent < 100 || dam == 0) {
				sprintf(buf, "%sВаш богатырский удар пропал впустую.%s\r\n",
					CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				lag = 3;
				dam = 0;
			} else if (prob * 100 / percent < 150) {
				sprintf(buf, "%sВаш богатырский удар задел %s.%s\r\n",
					CCBLU(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				lag = 3;
				dam += (dam / 2);
			} else if (prob * 100 / percent < 400) {
				sprintf(buf, "%sВаш богатырский удар пошатнул %s.%s\r\n",
					CCGRN(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				lag = 2;
				dam += (dam / 1);
				WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
				af.type = SPELL_BATTLE;
				af.bitvector = AFF_STOPFIGHT;
				af.location = 0;
				af.modifier = 0;
				af.duration = pc_duration(victim, 2, 0, 0, 0, 0);
				af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
				affect_join(victim, &af, TRUE, FALSE, TRUE, FALSE);
				sprintf(buf,
					"%sВаше сознание затуманилось после удара %s.%s\r\n",
					CCIRED(victim, C_NRM), PERS(ch, victim, 1), CCNRM(victim, C_NRM));
				send_to_char(buf, victim);
				act("$N пошатнул$U от богатырского удара $n1.", TRUE, ch, 0, victim, TO_NOTVICT);
			} else {
				sprintf(buf, "%sВаш богатырский удар сотряс %s.%s\r\n",
					CCIGRN(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				lag = 2;
				dam *= 4;
				WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
				af.type = SPELL_BATTLE;
				af.bitvector = AFF_STOPFIGHT;
				af.location = 0;
				af.modifier = 0;
				af.duration = pc_duration(victim, 3, 0, 0, 0, 0);
				af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
				affect_join(victim, &af, TRUE, FALSE, TRUE, FALSE);
				sprintf(buf, "%sВаше сознание померкло после удара %s.%s\r\n",
					CCIRED(victim, C_NRM), PERS(ch, victim, 1), CCNRM(victim, C_NRM));
				send_to_char(buf, victim);
				act("$N зашатал$U от богатырского удара $n1.", TRUE, ch, 0, victim, TO_NOTVICT);
			}
			if (!WAITLESS(ch))
				WAIT_STATE(ch, lag * PULSE_VIOLENCE);
		}
	}
	// STUPOR
	else if (GET_AF_BATTLE(ch, EAF_STUPOR) && GET_WAIT(ch) <= 0) {
		CLR_AF_BATTLE(ch, EAF_STUPOR);
		if (IS_NPC(ch) ||
		    IS_IMMORTAL(ch) ||
		    (wielded &&
		     GET_OBJ_WEIGHT(wielded) > 18 &&
		     GET_OBJ_SKILL(wielded) != SKILL_BOWS &&
		     !GET_AF_BATTLE(ch, EAF_PARRY) && !GET_AF_BATTLE(ch, EAF_MULTYPARRY))) {
			percent = number(1, skill_info[SKILL_STUPOR].max_percent);
			prob = train_skill(ch, SKILL_STUPOR, skill_info[SKILL_STUPOR].max_percent, victim);
			if (GET_MOB_HOLD(victim))
				prob = MAX(prob, percent * 150 / 100 + 1);
			if (IS_IMMORTAL(victim))
				prob = 0;
			if (prob * 100 / percent < 120 || dam == 0 || MOB_FLAGGED(victim, MOB_NOSTUPOR)) {
				sprintf(buf,
					"%sВы попытались оглушить %s, но не смогли.%s\r\n",
					CCCYN(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				lag = 3;
				dam = 0;
			} else if (prob * 100 / percent < 300) {
				sprintf(buf, "%sВаша мощная атака оглушила %s.%s\r\n",
					CCBLU(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				lag = 2;
				WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
				sprintf(buf,
					"%sВаше сознание помутилось после удара %s.%s\r\n",
					CCIRED(victim, C_NRM), PERS(ch, victim, 1), CCNRM(victim, C_NRM));
				send_to_char(buf, victim);
				act("$n оглушил$a $N3.", TRUE, ch, 0, victim, TO_NOTVICT);
			} else {
				if (MOB_FLAGGED(victim, MOB_NOBASH))
					sprintf(buf, "%sВаш мощнейший удар оглушил %s.%s\r\n",
						CCIGRN(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				else
					sprintf(buf, "%sВаш мощнейший удар сбил %s с ног.%s\r\n",
						CCIGRN(ch, C_NRM), PERS(victim, ch, 3), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				if (MOB_FLAGGED(victim, MOB_NOBASH))
					act("$n мощным ударом оглушил$a $N3.", TRUE, ch, 0, victim, TO_NOTVICT);
				else
					act("$n своим оглушающим ударом сбил$a $N3 с ног.", TRUE, ch,
					    0, victim, TO_NOTVICT);
				lag = 2;
				WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
				if (GET_POS(victim) > POS_SITTING && !MOB_FLAGGED(victim, MOB_NOBASH)) {
					GET_POS(victim) = POS_SITTING;
					sprintf(buf, "%sОглушающий удар %s сбил Вас с ног.%s\r\n",
						CCIRED(victim, C_NRM), PERS(ch, victim, 1), CCNRM(victim, C_NRM));
					send_to_char(buf, victim);
				} else {
					sprintf(buf,
						"%sВаше сознание помутилось после удара %s.%s\r\n",
						CCIRED(victim, C_NRM), PERS(ch, victim, 1), CCNRM(victim, C_NRM));
					send_to_char(buf, victim);
				}
			}
			if (!WAITLESS(ch))
				WAIT_STATE(ch, lag * PULSE_VIOLENCE);
		}
	}
	// Calculate poisoned weapon
	else if (dam && wielded && timed_by_skill(ch, SKILL_POISONED)) {
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
			if (wielded->affected[i].location == APPLY_POISON)
				break;
		if (i < MAX_OBJ_AFFECT &&
		    wielded->affected[i].modifier > 0 && !AFF_FLAGGED(victim, AFF_POISON) && !WAITLESS(victim)) {
			percent = number(1, skill_info[SKILL_POISONED].max_percent);
			prob = calculate_skill(ch, SKILL_POISONED, skill_info[SKILL_POISONED].max_percent, victim);
			if (prob >= percent
			    && !general_savingthrow(victim, SAVING_CRITICAL,
						    con_app[GET_REAL_CON(victim)].poison_saving, 0)) {
				improove_skill(ch, SKILL_POISONED, TRUE, victim);
				poison_victim(ch, victim, prob - percent);
				wielded->affected[i].modifier--;
			}
		}
	}
	// Calculate mob-poisoner
	else if (dam &&
		 IS_NPC(ch) &&
		 NPC_FLAGGED(ch, NPC_POISON) &&
		 !AFF_FLAGGED(ch, AFF_CHARM) &&
		 GET_WAIT(ch) <= 0 &&
		 !AFF_FLAGGED(victim, AFF_POISON) && number(0, 100) < GET_LIKES(ch) + GET_LEVEL(ch) - GET_LEVEL(victim)
		 && !general_savingthrow(victim, SAVING_CRITICAL, con_app[GET_REAL_CON(victim)].poison_saving, 0))
		poison_victim(ch, victim, MAX(1, GET_LEVEL(ch) - GET_LEVEL(victim)) * 10);

	// Если удар парирован, необходимо все равно ввязаться в драку.
	// Вызывается damage с отрицательным уроном
	return damage(ch, victim, mem_dam >= 0 ? dam : -1, attacktype, mayflee);

}



/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim  died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */

void char_dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim, int attacktype, int mayflee)
{
	if (IN_ROOM(ch) == NOWHERE)
		return;
	switch (GET_POS(victim)) {
	case POS_MORTALLYW:
		act("$n смертельно ранен$a и умрет, если $m не помогут.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("Вы смертельно ранены и умрете, если Вам не помогут.\r\n", victim);
		break;
	case POS_INCAP:
		act("$n без сознания и медленно умирает. Помогите же $m.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("Вы без сознания и медленно умираете, брошенные без помощи.\r\n", victim);
		break;
	case POS_STUNNED:
		act("$n без сознания, но возможно $e еще повоюет (попозже :).", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("Сознание покинуло Вас. В битве от Вас пока проку мало.\r\n", victim);
		break;
	case POS_DEAD:
		if (IS_NPC(victim) && (MOB_FLAGGED(victim, MOB_CORPSE))) {
			act("$n вспыхнул$g и рассыпал$u в прах.", FALSE, victim, 0, 0, TO_ROOM);
			send_to_char("Похоже Вас убили и даже тела не оставили !\r\n", victim);
		} else {
			act("$n мертв$g, $s душа медленно подымается в небеса.", FALSE, victim, 0, 0, TO_ROOM);
			send_to_char("Вы мертвы!  Hам очень жаль...\r\n", victim);
		}
		break;
	default:		/* >= POSITION SLEEPING */
		if (dam > (GET_REAL_MAX_HIT(victim) / 4))
			send_to_char("Это действительно БОЛЬНО !\r\n", victim);

		if (dam > 0 && GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4)) {
			sprintf(buf2,
				"%s Вы желаете, чтобы Ваши раны не кровоточили так сильно ! %s\r\n",
				CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
			send_to_char(buf2, victim);
		}
		if (ch != victim &&
		    IS_NPC(victim) &&
		    GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4) &&
		    MOB_FLAGGED(victim, MOB_WIMPY) && mayflee && GET_POS(victim) > POS_SITTING)
			do_flee(victim, NULL, 0, 0);

		if (ch != victim &&
		    !IS_NPC(victim) &&
		    HERE(victim) &&
		    GET_WIMP_LEV(victim) &&
		    GET_HIT(victim) < GET_WIMP_LEV(victim) && mayflee && GET_POS(victim) > POS_SITTING) {
			send_to_char("Вы запаниковали и попытались убежать !\r\n", victim);
			do_flee(victim, NULL, 0, 0);
		}
		break;
	}
}

// обработка щитов, зб, поглощения, сообщения для огн. щита НЕ ЗДЕСЬ
// возвращает сделанный дамаг
int damage(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int attacktype, int mayflee)
{
	int FS_damage = 0;
	ACMD(do_get);
//  long local_gold = 0;
//  char local_corpse[256];

	if (!ch || !victim) {
		return (0);
	}


	if (IN_ROOM(victim) == NOWHERE || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) != IN_ROOM(victim)) {
		log("SYSERR: Attempt to damage '%s' in room NOWHERE by '%s'.", GET_NAME(victim), GET_NAME(ch));
		return 0;
	}

	if (GET_POS(victim) <= POS_DEAD) {
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		    GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
		die(victim, NULL);
		return 0;	/* -je, 7/7/92 */
	}
	//
	if (dam >= 0 && damage_mtrigger(ch, victim))
		return 0;

	// Shopkeeper protection
	if (!ok_damage_shopkeeper(ch, victim))
		return 0;

	// No fight mobiles
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT)) {
		return 0;
	}

	if (IS_NPC(victim) && MOB_FLAGGED(ch, MOB_NOFIGHT)) {
		act("Боги предотвратили Ваше нападение на $N3.", FALSE, ch, 0, victim, TO_CHAR);
		return 0;
	}

	if (dam > 0) {
		// You can't damage an immortal!
		if (IS_GOD(victim))
			dam = 0;
		else if (IS_IMMORTAL(victim) || GET_GOD_FLAG(victim, GF_GODSLIKE))
			dam /= 4;
		else if (GET_GOD_FLAG(victim, GF_GODSCURSE))
			dam *= 2;
	}

// запоминание.
// чара-обидчика моб помнит всегда, если он его бьет непосредственно.
// если бьют клоны (проверка на MOB_CLONE), тоже помнит всегда. 
// если бьют храны или чармис (все остальные под чармом), то только если
// моб может видеть их хозяина.

// первое -- бьют моба, он запоминает обидчика
	if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY)) {
		if (!IS_NPC(ch))
			remember(victim, ch);
		else if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master)) {
			if (MOB_FLAGGED(ch, MOB_CLONE))
				remember(victim, ch->master);
			else if (IN_ROOM(ch->master) == IN_ROOM(victim) && CAN_SEE(victim, ch->master))
				remember(victim, ch->master);
		}
	}

// второе -- бьет сам моб и запоминает, кого потом добивать :)
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY)) {
		if (!IS_NPC(victim))
			remember(ch, victim);
		else if (AFF_FLAGGED(victim, AFF_CHARM) && victim->master && !IS_NPC(victim->master)) {
			if (MOB_FLAGGED(victim, MOB_CLONE))
				remember(ch, victim->master);
			else if (IN_ROOM(victim->master) == IN_ROOM(ch) && CAN_SEE(ch, victim->master))
				remember(ch, victim->master);
		}
	}

	//*************** If the attacker is invisible, he becomes visible
	appear(ch);

	// shapirus
	//*************** Если жертва невидима, с нее тоже инвиз убираем для
	//*************** предотвращения невидимости жертвы в случае бегства
	appear(victim);

	//**************** If you attack a pet, it hates your guts

	if (!same_group(ch, victim))
		check_agro_follower(ch, victim);


	if (victim != ch) {	//**************** Start the attacker fighting the victim
		if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL)) {
			pk_agro_action(ch, victim);
			set_fighting(ch, victim);
			npc_groupbattle(ch);
		}
		//***************** Start the victim fighting the attacker
		if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
			set_fighting(victim, ch);
			npc_groupbattle(victim);
		}
	}
	//*************** If negative damage - return
	if (dam < 0 || IN_ROOM(ch) == NOWHERE || IN_ROOM(victim) == NOWHERE || IN_ROOM(ch) != IN_ROOM(victim))
		return (0);

	// Сюда при отрицательном уроне не пройдет, и сообщений видно не будет

//  check_killer(ch, victim);

	if (victim != ch) {
		if (dam && AFF_FLAGGED(victim, AFF_SHIELD)) {
			if (attacktype == SKILL_BASH + TYPE_HIT)
				skill_message(dam, ch, victim, attacktype);
			act("Магический кокон полностью поглотил удар $N1.", FALSE, victim, 0, ch, TO_CHAR);
			act("Магический кокон вокруг $N1 полностью поглотил Ваш удар.", FALSE, ch, 0, victim, TO_CHAR);
			act("Магический кокон вокруг $N1 полностью поглотил удар $n1.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			return (0);
		}

		if ((dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_FIRESHIELD))
		    && (attacktype != (TYPE_HIT + SKILL_BACKSTAB))) {
			FS_damage = dam * 20 / 100;
			dam -= (dam * number(10, 30) / 100);
		}

		if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_ICESHIELD)) {
			act("Ледяной щит принял часть удара на себя.", FALSE, ch, 0, victim, TO_VICT);
			act("Ледяной щит вокруг $N1 смягчил Ваш удар.", FALSE, ch, 0, victim, TO_CHAR);
			act("Ледяной щит вокруг $N1 смягчил удар $n1.", TRUE, ch, 0, victim, TO_NOTVICT);
			dam -= (dam * number(30, 50) / 100);
		}

		if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_AIRSHIELD)) {
			act("Воздушный щит смягчил удар $n1.", FALSE, ch, 0, victim, TO_VICT);
			act("Воздушный щит вокруг $N1 ослабил Ваш удар.", FALSE, ch, 0, victim, TO_CHAR);
			act("Воздушный щит вокруг $N1 ослабил удар $n1.", TRUE, ch, 0, victim, TO_NOTVICT);
			dam -= (dam * number(30, 50) / 100);
		}

		if (dam && (IS_WEAPON(attacktype)
			    || attacktype == (SKILL_KICK + TYPE_HIT))) {
			alt_equip(victim, NOWHERE, dam, 50);
			if (!was_critic) {
				int decrease = MIN(25,
						   (GET_ABSORBE(victim) + 1) / 2) + GET_ARMOUR(victim);
				if (decrease >= number(dam, dam * 50)) {
					act("Ваши доспехи полностью поглотили удар $n1.", FALSE,
					    ch, 0, victim, TO_VICT);
					act("Доспехи $N1 полностью поглотили Ваш удар.", FALSE, ch, 0, victim, TO_CHAR);
					act("Доспехи $N1 полностью поглотили удар $n1.", TRUE, ch,
					    0, victim, TO_NOTVICT);
					return (0);
				}
				dam -= (dam * MIN(50, decrease) / 100);
			}
		}
	} else if (MOB_FLAGGED(victim, MOB_PROTECT)) {
		return (0);
	}
	//*************** Set the maximum damage per round and subtract the hit points
	if (MOB_FLAGGED(victim, MOB_PROTECT)) {
		act("$n находится под защитой Богов.", FALSE, victim, 0, 0, TO_ROOM);
		return (0);
	}
	// log("[DAMAGE] Compute critic...");
	dam = MAX(dam, 0);
	if (dam && was_critic) {
		FS_damage = 0;
		dam = compute_critical(ch, victim, dam);
	}

	if (attacktype == SPELL_FIRE_SHIELD) {
		if ((GET_HIT(victim) -= dam) < 1)
			GET_HIT(victim) = 1;
	} else
		GET_HIT(victim) -= dam;

	//*************** Gain exp for the hit
	//Battle exp gain for mobs is DISABLED
	if (ch != victim &&
	    OK_GAIN_EXP(ch, victim) &&
	    !AFF_FLAGGED(victim, AFF_CHARM) && !MOB_FLAGGED(victim, MOB_ANGEL) && !IS_NPC(ch))
		gain_exp(ch, (GET_LEVEL(victim) * dam + 4) / 5);
	// gain_exp(ch, IS_NPC(ch) ? GET_LEVEL(victim) * dam : (GET_LEVEL(victim) * dam + 4) / 5);
	// log("[DAMAGE] Updating pos...");
	update_pos(victim);


	// * skill_message sends a message from the messages file in lib/misc.
	//  * dam_message just sends a generic "You hit $n extremely hard.".
	// * skill_message is preferable to dam_message because it is more
	// * descriptive.
	// *
	// * If we are _not_ attacking with a weapon (i.e. a spell), always use
	// * skill_message. If we are attacking with a weapon: If this is a miss or a
	// * death blow, send a skill_message if one exists; if not, default to a
	// * dam_message. Otherwise, always send a dam_message.
	// log("[DAMAGE] Attack message...");

	if (!IS_WEAPON(attacktype))
		skill_message(dam, ch, victim, attacktype);
	else {
		if (GET_POS(victim) == POS_DEAD || dam == 0) {
			if (!skill_message(dam, ch, victim, attacktype))
				dam_message(dam, ch, victim, attacktype);
		} else {
			dam_message(dam, ch, victim, attacktype);
		}
	}

	// log("[DAMAGE] Victim message...");
	//******** Use send_to_char -- act() doesn't send message if you are DEAD.
	char_dam_message(dam, ch, victim, attacktype, mayflee);
	// log("[DAMAGE] Flee etc...");

	// Проверить, что жертва все еще тут. Может уже сбежала по трусости.
	// Думаю, простой проверки достаточно.
	// Примечание, если сбежал в FIRESHIELD, 
	// то обратного повреждения по атакующему не будет
	if (IN_ROOM(ch) != IN_ROOM(victim))
		return dam;

	// *********** Help out poor linkless people who are attacked */
	if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {	/*
		do_flee(victim, NULL, 0, 0);
		if (!FIGHTING(victim)) {
			act("$n был$g спасен$a Богами.", FALSE, victim, 0, 0, TO_ROOM);
			GET_WAS_IN(victim) = IN_ROOM(victim);
			char_from_room(victim);
			char_to_room(victim, STRANGE_ROOM);
		}
											 */
	}
	// *********** Stop someone from fighting if they're stunned or worse
	if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL)) {
		stop_fighting(victim, GET_POS(victim) <= POS_DEAD);
	}
	// *********** Uh oh.  Victim died.
	if (GET_POS(victim) == POS_DEAD) {
		CHAR_DATA *killer = NULL;

		if (IS_NPC(victim) || victim->desc) {
			if (victim == ch && IN_ROOM(victim) != NOWHERE) {
				if (attacktype == SPELL_POISON) {
					CHAR_DATA *poisoner;
					for (poisoner = world[IN_ROOM(victim)]->people; poisoner;
					     poisoner = poisoner->next_in_room)
						if (poisoner != victim && GET_ID(poisoner) == victim->Poisoner)
							killer = poisoner;
				} else if (attacktype == TYPE_SUFFERING) {
					CHAR_DATA *attacker;
					for (attacker = world[IN_ROOM(victim)]->people; attacker;
					     attacker = attacker->next_in_room)
						if (FIGHTING(attacker) == victim)
							killer = attacker;
				}
			}
			if (ch != victim)
				killer = ch;
		}

		if (killer) {
			if (AFF_FLAGGED(killer, AFF_GROUP))
// т.к. помечен флагом AFF_GROUP - точно PC
				group_gain(killer, victim);
			else if ((AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL)) && killer->master)
// killer - зачармленный NPC с хозяином
			{
				if (IN_ROOM(killer) == IN_ROOM(killer->master)) {
// Чармис и хозяин в одной комнате
					if (!IS_NPC(killer->master)
					    && AFF_FLAGGED(killer->master, AFF_GROUP))
// Хозяин - PC в группе => опыт группе
						group_gain(killer->master, victim);
					else
						// Опыт хозяину
					{
						perform_group_gain(killer->master, victim, 1, 100);
						//solo_gain(killer->master, victim);
						//solo_gain(killer,victim);
					}
				}
				// else
				// А хозяина то рядом не оказалось, все чармису - убрано
// нефиг абьюзить чарм  perform_group_gain( killer, victim, 1, 100 );
			} else
				// Просто NPC или PC сам по себе
				perform_group_gain(killer, victim, 1, 100);
		}
		if (!IS_NPC(victim)) {
			sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim),
				GET_NAME(ch), IN_ROOM(victim) != NOWHERE ? world[IN_ROOM(victim)]->name : "NOWHERE");
			mudlog(buf2, BRF, LVL_IMPL, SYSLOG, TRUE);
			if (IS_NPC(ch) &&
			    (AFF_FLAGGED(ch, AFF_CHARM) || IS_HORSE(ch)) && ch->master && !IS_NPC(ch->master)) {
				sprintf(buf2, "%s подчиняется %s.", GET_NAME(ch), GET_PAD(ch->master, 2));
				mudlog(buf2, BRF, LVL_IMPL, SYSLOG, TRUE);
			}
			if (MOB_FLAGGED(ch, MOB_MEMORY))
				forget(ch, victim);
		}
		/* Есть ли в будующем трупе куны...? */
//      if (IS_NPC (victim))
//      local_gold = GET_GOLD (victim);
		die(victim, ch);
		/* Автограбеж */
//      sprintf (local_corpse, "труп.%s", GET_PAD (victim, 1));
//      if (IS_NPC (victim) && !IS_NPC (ch) && PRF_FLAGGED (ch, PRF_AUTOLOOT)
//        && get_obj_in_list_vis (ch, local_corpse,
//                                world[ch->in_room]->contents))
//      {
//        sprintf (local_corpse, "все труп.%s земля", GET_PAD (victim, 1));
//        do_get (ch, local_corpse, 0, 0);
//      }
//      else
//      {
//        /* Брать куны */
//        if (IS_NPC (victim) && !IS_NPC (ch)
//            && PRF_FLAGGED (ch, PRF_AUTOMONEY) && (local_gold > 0)
//            && get_obj_in_list_vis (ch, local_corpse,
//                                    world[ch->in_room]->contents))
//          {
//            sprintf (local_corpse, "все.кун труп.%s земля",
//                     GET_PAD (victim, 1));
//            do_get (ch, local_corpse, 0, 0);
//          }
//      }
		return (-1);
	}
	if (FS_damage && FIGHTING(victim) && GET_POS(victim) > POS_STUNNED && IN_ROOM(victim) != NOWHERE)
		damage(victim, ch, FS_damage, SPELL_FIRE_SHIELD, FALSE);
	return (dam);
}

/**** This function realize second shot for bows *******/
void exthit(CHAR_DATA * ch, int type, int weapon)
{
	OBJ_DATA *wielded = NULL;
	int percent, prob;
	CHAR_DATA *tch;


	if (IS_NPC(ch)) {
		if (MOB_FLAGGED(ch, MOB_EADECREASE) && weapon > 1) {
			if (ch->mob_specials.ExtraAttack * GET_HIT(ch) * 2 < weapon * GET_REAL_MAX_HIT(ch))
				return;
		}
		if (MOB_FLAGGED(ch, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH |
				     MOB_ACIDBREATH | MOB_LIGHTBREATH))) {
			for (prob = percent = 0; prob <= 4; prob++)
				if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
					percent++;
			percent = weapon % percent;
			for (prob = 0; prob <= 4; prob++)
				if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob)))) {
					if (percent)
						percent--;
					else
						break;
				}
			if (MOB_FLAGGED(ch, MOB_AREA_ATTACK)) {
				for (tch = world[IN_ROOM(ch)]->people; tch; tch = tch->next_in_room) {
					if (IS_IMMORTAL(tch))	/* immortal    */
						continue;
					if (IN_ROOM(ch) == NOWHERE ||	/* Something killed in process ... */
					    IN_ROOM(tch) == NOWHERE)
						continue;
					if (tch != ch && !same_group(ch, tch))
						mag_damage(GET_LEVEL(ch), ch, tch,
							   SPELL_FIRE_BREATH + MIN(prob, 4), SAVING_CRITICAL);
				}
			} else
				mag_damage(GET_LEVEL(ch), ch, FIGHTING(ch),
					   SPELL_FIRE_BREATH + MIN(prob, 4), SAVING_CRITICAL);
			return;
		}
	}

	if (weapon == 1) {
		if (!(wielded = GET_EQ(ch, WEAR_WIELD)))
			wielded = GET_EQ(ch, WEAR_BOTHS);
	} else if (weapon == 2)
		wielded = GET_EQ(ch, WEAR_HOLD);
	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	int div = 0;
	if (wielded && !GET_EQ(ch, WEAR_SHIELD) &&
	    GET_OBJ_SKILL(wielded) == SKILL_BOWS &&
	    GET_EQ(ch, WEAR_BOTHS) &&
	    ((prob =
	      train_skill(ch, SKILL_ADDSHOT, skill_info[SKILL_ADDSHOT].max_percent,
			  FIGHTING(ch))) >= percent || WAITLESS(ch))) {
		hit(ch, FIGHTING(ch), type, weapon);
// Изменено Стрибогом     
//      if (prob / percent > 4 && FIGHTING(ch))
		// вероятность 66%
		percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
		if (prob * 2 > percent * 3 && FIGHTING(ch))
			hit(ch, FIGHTING(ch), type, weapon);

		// при 5 мортах 40%, при меньше -- меньше
		percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
		div = 5 * (6 - MIN(5, GET_REMORT(ch)));
		if (prob * 2 > percent * div && FIGHTING(ch))
			hit(ch, FIGHTING(ch), type, weapon);

		// при 8 мортах 20%, при меньше -- меньше
		percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
		div = 5 * (9 - MIN(8, GET_REMORT(ch)));
		if (prob > percent * div && FIGHTING(ch))
			hit(ch, FIGHTING(ch), type, weapon);
	}
	hit(ch, FIGHTING(ch), type, weapon);
}

// бонусы/штрафы классам за юзание определенных видов оружия
void apply_weapon_bonus(int ch_class, int skill, int *damroll, int *hitroll)
{
	int dam = *damroll;
	int calc_thaco = *hitroll;

	switch (ch_class) {
	case CLASS_CLERIC:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 0; dam += 1; break;
			case SKILL_AXES:	calc_thaco -= 0; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_NONSTANDART:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_PICK:	calc_thaco -= 0; dam += 0; break;
			case SKILL_SPADES:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= 0; dam += 0; break;
		}
		break;
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_AXES:	calc_thaco -= 0; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_NONSTANDART:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_PICK:	calc_thaco -= 0; dam += 0; break;
			case SKILL_SPADES:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= 0; dam += 0; break;
		}
		break;
	case CLASS_WARRIOR:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 2; dam += 0; break;
			case SKILL_AXES:	calc_thaco -= 1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= 1; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= -2; dam += 0; break;
			case SKILL_NONSTANDART:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= 0; dam += 2; break;
			case SKILL_PICK:	calc_thaco -= -2; dam += 0; break;
			case SKILL_SPADES:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= 0; dam += 0; break;
		}
		break;
	case CLASS_RANGER:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 1; dam += 0; break;
			case SKILL_AXES:	calc_thaco -= 1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco += 1; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_NONSTANDART:	calc_thaco -= 1; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco += 1; dam += 0; break;
			case SKILL_PICK:	calc_thaco += 1; dam += 0; break;
			case SKILL_SPADES:	calc_thaco -= 1; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= 2; dam += 1; break;
		}
		break;
		case CLASS_GUARD:
	case CLASS_PALADINE:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 1; dam += 0; break;
			case SKILL_AXES:	calc_thaco -= 1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= 1; dam += 2; break;
			case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_NONSTANDART:	calc_thaco -= 1; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= 1; dam += 3; break;
			case SKILL_PICK:	calc_thaco -= 0; dam += 0; break;
			case SKILL_SPADES:	calc_thaco -= 1; dam += 1; break;
			case SKILL_BOWS:	calc_thaco -= 1; dam += 0; break;
		}
		break;
	case CLASS_THIEF:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_AXES:	calc_thaco -= -1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 0; dam += 3; break;
			case SKILL_NONSTANDART:	calc_thaco -= -1; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_PICK:	calc_thaco -= 0; dam += 3; break;
			case SKILL_SPADES:	calc_thaco -= -1; dam += 1; break;
			case SKILL_BOWS:	calc_thaco -= -1; dam += 0; break;
		}
		break;
	case CLASS_ASSASINE:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_AXES:	calc_thaco -= -1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= 1; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 2; dam += 7; break;
			case SKILL_NONSTANDART:	calc_thaco -= -1; dam += 4; break;
			case SKILL_BOTHHANDS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_PICK:	calc_thaco -= 2; dam += 7; break;
			case SKILL_SPADES:	calc_thaco -= -1; dam += 4; break;
			case SKILL_BOWS:	calc_thaco -= -1; dam += 0; break;
		}
		break;
	case CLASS_SMITH:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 1; dam += 1; break;
			case SKILL_AXES:	calc_thaco -= 1; dam += 1; break;
			case SKILL_LONGS:	calc_thaco -= 1; dam += 1; break;
			case SKILL_SHORTS:	calc_thaco -= -1; dam += -1; break;
			case SKILL_NONSTANDART:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_PICK:	calc_thaco -= -1; dam += -1; break;
			case SKILL_SPADES:	calc_thaco -= 0; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= -1; dam += -1; break;
		}
		break;
	case CLASS_MERCHANT:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 1; dam += 1; break;
			case SKILL_AXES:	calc_thaco -= -1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 1; dam += 1; break;
			case SKILL_NONSTANDART:	calc_thaco -= 1; dam += 1; break;
			case SKILL_BOTHHANDS:	calc_thaco -= -1; dam += -1; break;
			case SKILL_PICK:	calc_thaco -= 1; dam += 1; break;
			case SKILL_SPADES:	calc_thaco -= -1; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= -1; dam += 0; break;
		}
		break;
	case CLASS_DRUID:
		switch (skill) {
			case SKILL_CLUBS:	calc_thaco -= 1; dam += 2; break;
			case SKILL_AXES:	calc_thaco -= -1; dam += 0; break;
			case SKILL_LONGS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
			case SKILL_NONSTANDART:	calc_thaco -= 1; dam += 0; break;
			case SKILL_BOTHHANDS:	calc_thaco -= -1; dam += 0; break;
			case SKILL_PICK:	calc_thaco -= -1; dam += 0; break;
			case SKILL_SPADES:	calc_thaco -= -1; dam += 0; break;
			case SKILL_BOWS:	calc_thaco -= -1; dam += 0; break;
		}
		break;
	}

	*damroll = dam;
	*hitroll = calc_thaco;
}

inline int do_punctual(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wielded)
{
	int dam_critic = 0, skill, wapp;

	if (wielded)
		wapp = GET_OBJ_WEIGHT(wielded);
	else
		wapp = 0;

		if (wapp < 10)
			dam_critic = dice (1, 6);
		else
		if (wapp < 19)
			dam_critic = dice (2, 5);
		else
		if (wapp < 27)
			dam_critic = dice (3, 4);
		else
		if (wapp < 36)
			dam_critic = dice (3, 5);
		else
		if (wapp < 44)
			dam_critic = dice (3, 6);
		else
			dam_critic = dice (4, 5);
		skill = 1 + GET_SKILL(ch,SKILL_PUNCTUAL) / 6;
		dam_critic = MIN (number (1,skill),dam_critic);

	return dam_critic;
}

// обработка ударов оружием, санка, призма, стили, итд.
void hit(CHAR_DATA * ch, CHAR_DATA * victim, int type, int weapon)
{
	OBJ_DATA *wielded = NULL;
	CHAR_DATA *vict;
//  int victim_old_ac;
	int w_type = 0, victim_ac, calc_thaco, dam, diceroll, prob, range, skill =
	    0, weapon_pos = WEAR_WIELD, percent, is_shit = (weapon == 2) ? 1 : 0, modi = 0, skill_is = 0;

	if (!victim)
		return;

	/* check if the character has a fight trigger */
//  fight_mtrigger(ch);

	/* Do some sanity checking, in case someone flees, etc. */
	if (IN_ROOM(ch) != IN_ROOM(victim) || IN_ROOM(ch) == NOWHERE) {
		if (FIGHTING(ch) && FIGHTING(ch) == victim)
			stop_fighting(ch, TRUE);
		return;
	}

	/* Stand awarness mobs */
	if (CAN_SEE(victim, ch) &&
	    !FIGHTING(victim) &&
	    ((IS_NPC(victim) &&
	      (GET_HIT(victim) < GET_MAX_HIT(victim) ||
	       MOB_FLAGGED(victim, MOB_AWARE))) ||
	     AFF_FLAGGED(victim, AFF_AWARNESS)) && !GET_MOB_HOLD(victim) && GET_WAIT(victim) <= 0)
		set_battle_pos(victim);

	/* Check protections */
	if (GET_AF_BATTLE(ch, EAF_PROTECT))
		return;

	/* Find weapon for attack number weapon */
	if (weapon == 1) {
		if (!(wielded = GET_EQ(ch, WEAR_WIELD))) {
			wielded = GET_EQ(ch, WEAR_BOTHS);
			weapon_pos = WEAR_BOTHS;
		}
	} else if (weapon == 2) {
		wielded = GET_EQ(ch, WEAR_HOLD);
		weapon_pos = WEAR_HOLD;
	}

	calc_thaco = 0;
	victim_ac = 0;
	dam = 0;

	/* Обработка SKILL_NOPARRYHIT */
	if (type == TYPE_UNDEFINED && GET_SKILL(ch, SKILL_NOPARRYHIT)) {
		if ((train_skill
		     (ch, SKILL_NOPARRYHIT, skill_info[SKILL_NOPARRYHIT].max_percent,
		      FIGHTING(ch)) >= number(1, skill_info[SKILL_NOPARRYHIT].max_percent)) || WAITLESS(ch)) {
			type = TYPE_NOPARRY;
		}
	}

	/* Find the weapon type (for display purposes only) */
	if (type == SKILL_THROW) {
		diceroll = 100;
		weapon = 100;
		skill = SKILL_THROW;
		w_type = type + TYPE_HIT;
	} else if (type == SKILL_BACKSTAB) {
		diceroll = 100;
		weapon = 100;
		skill = SKILL_BACKSTAB;
		w_type = type + TYPE_HIT;
	} else if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
		skill = GET_OBJ_SKILL(wielded);
		skill_is = train_skill(ch, skill, skill_info[skill].max_percent, victim);

		if (!IS_NPC(ch)) {	// Two-handed attack - decrease TWO HANDS

/*  shapirus: тут сделаны штрафы для дуала и для левой руки,
              но сейчас закомментированы, потому что все равно
	      они никогда не работали.

	  if (weapon == 1 &&
	      GET_EQ (ch, WEAR_HOLD) &&
	      GET_OBJ_TYPE (GET_EQ (ch, WEAR_HOLD)) == ITEM_WEAPON)
	    {
	      if (weapon < EXPERT_WEAPON)
		calc_thaco += 2;
	      else
		calc_thaco += 0;
	    }
	  else
	    if (weapon == 2 && GET_EQ (ch, WEAR_WIELD) &&
		GET_OBJ_TYPE (GET_EQ (ch, WEAR_WIELD)) == ITEM_WEAPON)
	    {
	      if (weapon < EXPERT_WEAPON)
		calc_thaco += 4;
	      else
		calc_thaco += 2;
	    }
*/
			// Apply HR for light weapon
			percent = 0;
			switch (weapon_pos) {
			case WEAR_WIELD:
				percent = (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			case WEAR_HOLD:
				percent = (str_app[STRENGTH_APPLY_INDEX(ch)].hold_w - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			case WEAR_BOTHS:
				percent = (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w +
					   str_app[STRENGTH_APPLY_INDEX(ch)].hold_w - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			}
			calc_thaco -= MIN(3, MAX(percent, 0));

			// Penalty for unknown weapon type
// shapirus: старый штраф нифига не работает, тем более, что unknown_weapon_fault
// нигде не определяется. сделан новый на базе инты чара. плюс сделан штраф на дамролл.
// если скилл есть, то штраф не даем, а применяем бонусы/штрафы по профам
			if (GET_SKILL(ch, skill) == 0) {
				calc_thaco += (50 - MIN(50, GET_REAL_INT(ch))) / 3;
				dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			} else {
				apply_weapon_bonus(GET_CLASS(ch), skill, &dam, &calc_thaco);
			}

			// Bonus for expert weapon
			if (weapon >= EXPERT_WEAPON)
				calc_thaco -= 1;
			// Bonus for leadership
			if (calc_leadership(ch))
				calc_thaco -= 2;
		}
		w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
	} else {
		skill = SKILL_PUNCH;
		weapon_pos = 0;
//      diceroll = number (0, skill_info[skill].max_percent);
		skill_is = train_skill(ch, skill, skill_info[skill].max_percent, victim);
//      skill_is = MIN (GET_SKILL (ch, skill), skill_info[skill].max_percent);

		if (!IS_NPC(ch)) {
			// кулаками у нас полагается бить только богатырям :)
			if (GET_CLASS(ch) != CLASS_WARRIOR)
				calc_thaco += 4;
			else	// а богатырям положен бонус за отсутствие оружия
				calc_thaco -= 3;

			// штраф в размере 1 хитролла за каждые
			// недокачанные 10% скилла "удар левой рукой"
			if (is_shit)
				calc_thaco += (skill_info[SKILL_SHIT].max_percent -
						train_skill(ch, SKILL_SHIT,
						skill_info[SKILL_SHIT].max_percent, victim)) / 10;
			// Bonus for expert PUNCH
			if (weapon >= EXPERT_WEAPON)
				calc_thaco -= 1;
			// Bonus for leadership
			if (calc_leadership(ch))
				calc_thaco -= 2;
		}

		if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
			w_type = ch->mob_specials.attack_type + TYPE_HIT;
		else
			w_type += TYPE_HIT;
	}

	// courage
	if (affected_by_spell(ch, SPELL_COURAGE)) {
		range = number(1, skill_info[SKILL_COURAGE].max_percent + GET_REAL_MAX_HIT(ch) - GET_HIT(ch));
		prob = train_skill(ch, SKILL_COURAGE, skill_info[SKILL_COURAGE].max_percent, victim);
		if (prob > range) {
			dam += ((GET_SKILL(ch, SKILL_COURAGE) + 19) / 20);
			calc_thaco += ((GET_SKILL(ch, SKILL_COURAGE) + 9) / 20);
		}
	}
//Adept: увеличение шанса попасть при нанесении удара богатырским молотом
	if (skill == SKILL_STUPOR || skill == SKILL_MIGHTHIT) 
		calc_thaco -= (int) MAX(0, (GET_SKILL(ch, skill) - 70) / 8);
	
	//    AWAKE style - decrease hitroll
	if (GET_AF_BATTLE(ch, EAF_AWAKE) &&
	    (IS_NPC(ch) || GET_CLASS(ch) != CLASS_ASSASINE) && skill != SKILL_THROW && skill != SKILL_BACKSTAB) {
		calc_thaco += ((GET_SKILL(ch, SKILL_AWAKE) + 9) / 10) + 2;
		if (GET_SKILL(ch, SKILL_AWAKE) > 50 && !IS_NPC(ch))
			dam = dam / (GET_SKILL(ch, SKILL_AWAKE) / 50);
	}

	if (!IS_NPC(ch) && skill != SKILL_THROW && skill != SKILL_BACKSTAB) {	// PUNCTUAL style - decrease PC damage
		if (GET_AF_BATTLE(ch, EAF_PUNCTUAL)) {
			calc_thaco += 0;
			dam -= 0;
		}
		// Casters use weather, int and wisdom
		if (IS_CASTER(ch)) {
/*	  calc_thaco +=
	    (10 -
	     complex_skill_modifier (ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS,
				     10));
*/
			calc_thaco -= (int) ((GET_REAL_INT(ch) - 13) / GET_LEVEL(ch));
			calc_thaco -= (int) ((GET_REAL_WIS(ch) - 13) / GET_LEVEL(ch));
		}
		// Horse modifier for attacker
		if (on_horse(ch)) {
			prob = train_skill(ch, SKILL_HORSE, skill_info[SKILL_HORSE].max_percent, victim);
			dam += ((prob + 19) / 10);
			range = number(1, skill_info[SKILL_HORSE].max_percent);
			if (range > prob)
				calc_thaco += ((range - prob) + 19 / 20);
			else
				calc_thaco -= ((prob - range) + 19 / 20);
		}
		// Skill level increase damage
		if (GET_SKILL(ch, skill) >= 60)
			dam += ((GET_SKILL(ch, skill) - 50) / 10);
	}
	// not can see (blind, dark, etc)
	if (!CAN_SEE(ch, victim))
		calc_thaco += 6;
	if (!CAN_SEE(victim, ch))
		calc_thaco -= 6;

	// bless
	if (AFF_FLAGGED(ch, AFF_BLESS)) {
		calc_thaco -= 4;
	}
	// curse
	if (AFF_FLAGGED(ch, AFF_CURSE)) {
		calc_thaco += 6;
		dam -= 5;
	}
	// some protects
	if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch))
		calc_thaco += 3;
	if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))
		calc_thaco += 3;

	// "Dirty" methods for battle
	if (skill != SKILL_THROW && skill != SKILL_BACKSTAB) {
		prob = (GET_SKILL(ch, skill) + cha_app[GET_REAL_CHA(ch)].illusive) -
		    (GET_SKILL(victim, skill) + int_app[GET_REAL_INT(victim)].observation);
		if (prob >= 30 && !GET_AF_BATTLE(victim, EAF_AWAKE)
		    && (IS_NPC(ch) || !GET_AF_BATTLE(ch, EAF_PUNCTUAL))) {
			calc_thaco -= (GET_SKILL(ch, skill) - GET_SKILL(victim, skill) > 60 ? 2 : 1);
			if (!IS_NPC(victim))
				dam += (prob >= 70 ? 3 : (prob >= 50 ? 2 : 1));
		}
	}
	// AWAKE style for victim
	if (GET_AF_BATTLE(victim, EAF_AWAKE) &&
	    !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
	    !AFF_FLAGGED(victim, AFF_MAGICSTOPFIGHT) &&
	    !GET_MOB_HOLD(victim) &&
	    train_skill(victim, SKILL_AWAKE, skill_info[SKILL_AWAKE].max_percent,
			ch) >= number(1, skill_info[SKILL_AWAKE].max_percent)) {
		dam -= IS_NPC(ch) ? 5 : 5;
		calc_thaco += IS_NPC(ch) ? 4 : 2;
	}
	// Calculate the THAC0 of the attacker
	if (!IS_NPC(ch))
		calc_thaco += thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	else			// штраф мобам по рекомендации Триглава
		calc_thaco += (25 - GET_LEVEL(ch) / 3);

	calc_thaco -= (str_app[STRENGTH_APPLY_INDEX(ch)].tohit + GET_REAL_HR(ch));

	if ((skill == SKILL_THROW || skill == SKILL_BACKSTAB) && wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
		skill_is = calculate_skill(ch, GET_OBJ_SKILL(wielded),
					   skill_info[GET_OBJ_SKILL(wielded)].max_percent, victim);
		if (skill == SKILL_BACKSTAB)
			calc_thaco -= MAX(0, (GET_SKILL(ch,SKILL_SNEAK) + GET_SKILL(ch,SKILL_HIDE)- 100)/30);
	} else
// тюнинг оверности делается тут :)
		calc_thaco += 4;

	if (skill_is <= 80)
		calc_thaco -= skill_is / 20;
	else if (skill_is <= 110)
		calc_thaco -= 4 + (skill_is - 80) / 10;
	else
		calc_thaco -= 4 + 3 + (skill_is - 110) / 5;


	//  log("Attacker : %s", GET_NAME(ch));
	//  log("THAC0    : %d ", calc_thaco);

	// Calculate the raw armor including magic armor.  Lower AC is better.

	victim_ac += compute_armor_class(victim);
	victim_ac /= 10;
	if (GET_POS(victim) < POS_FIGHTING)
		victim_ac += 4;
	if (GET_POS(victim) < POS_RESTING)
		victim_ac += 3;

// нефиг тут аффекты от несуществующих заклинаний учитывать
//  if (AFF_FLAGGED (victim, AFF_STAIRS) && victim_ac < 10)
//    victim_ac = (victim_ac + 10) >> 1;

	if (AFF_FLAGGED(victim, AFF_HOLD))
		victim_ac += 4;

	if (AFF_FLAGGED(victim, AFF_CRYING))
		victim_ac += 4;

	//  log("Target : %s", GET_NAME(victim));
	//  log("AC     : %d ", victim_ac);

	// roll the dice and take your chances...
	diceroll = number(1, 20);

//log("Attacker: %s, THAC0: %d, victim_ac: %d, diceroll: %d, skill_is: %d, skill: %d", GET_NAME(ch), calc_thaco, victim_ac, diceroll, skill_is, skill);

	// log("THAC0 - %d  AC - %d Diceroll - %d",calc_thaco,victim_ac, diceroll);
	// sprintf(buf,"THAC0 - %d  Diceroll - %d    AC - %d(%d)\r\n", calc_thaco, diceroll,victim_ac,compute_armor_class(victim));
	// send_to_char(buf,ch);
	//  send_to_char(buf,victim);

	// decide whether this is a hit or a miss
	// всегда есть 5% вероятность попасть или промазать,
	// какой бы AC у противника ни был
	if (((diceroll < 20) && AWAKE(victim)) && ((diceroll == 1) || (calc_thaco - diceroll > victim_ac))) {	/* the attacker missed the victim */
		extdamage(ch, victim, 0, w_type, wielded, TRUE);
	} else {		// blink
		if ((AFF_FLAGGED(victim, AFF_BLINK)
		     || (GET_CLASS(victim) == CLASS_THIEF))
		    && !GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		    && !GET_AF_BATTLE(ch, EAF_STUPOR)
		    && (!(type == SKILL_BACKSTAB && GET_CLASS(ch) == CLASS_THIEF))
		    && number(1, 100) <= 20) {
			sprintf(buf,
				"%sНа мгновение Вы исчезли из поля зрения противника.%s\r\n",
				CCINRM(victim, C_NRM), CCNRM(victim, C_NRM));
			send_to_char(buf, victim);
			extdamage(ch, victim, 0, w_type, wielded, TRUE);
			return;
		}
		// okay, we know the guy has been hit.  now calculate damage.

		// Start with the damage bonuses: the damroll and strength apply

		dam += GET_REAL_DR(ch);
		dam = dam > 0 ? number(1, (dam * 2)) : dam;
		dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;

		if (GET_EQ(ch, WEAR_BOTHS) && skill != SKILL_BOWS)
			dam *= 2;

		if (IS_NPC(ch)) {
			dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
		}

		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {	// Add weapon-based damage if a weapon is being wielded
			percent = dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
			if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM)
			    && !MOB_FLAGGED(ch, MOB_ANGEL)) {
				percent *= MOB_DAMAGE_MULT;
			} else {
				percent = MIN(percent, percent * GET_OBJ_CUR(wielded) / MAX(1, GET_OBJ_MAX(wielded)));
			}
			dam += MAX(1, percent);
		} else {	// If no weapon, add bare hand damage instead
			if (AFF_FLAGGED(ch, AFF_STONEHAND)) {
				if (GET_CLASS(ch) == CLASS_WARRIOR)
					dam += number(5, 10 + GET_LEVEL(ch) / 5);
				else
					dam += number(5, 10);
			} else if (!IS_NPC(ch)) {
				if (GET_CLASS(ch) == CLASS_WARRIOR)
					dam += number(1, 3 + GET_LEVEL(ch) / 5);
				else
					dam += number(1, 3);
			}
			// Мультипликатор повреждений без оружия и в перчатках (линейная интерполяция)
			// <вес перчаток> <увеличение>
			// 0  50%
			// 5 100%
			// 10 150%
			// 15 200%
			// НА МОЛОТ НЕ ВЛИЯЕТ
			if (!GET_AF_BATTLE(ch, EAF_MIGHTHIT)) {
				modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)) : 0));
				if (IS_NPC(ch) || IS_WARRIOR(ch))
					modi = MAX(100, modi);
				dam = modi * dam / 100;
			}
		}

		// Change victim, if protector present
		victim = try_protect(victim, ch, weapon);

		// Include a damage multiplier if victim isn't ready to fight:
		// Position sitting  1.5 x normal
		// Position resting  2.0 x normal
		// Position sleeping 2.5 x normal
		// Position stunned  3.0 x normal
		// Position incap    3.5 x normal
		// Position mortally 4.0 x normal
		//
		// Note, this is a hack because it depends on the particular
		// values of the POSITION_XXX constants.
		//
		if (GET_POS(ch) < POS_FIGHTING)
			dam -= (dam * (POS_FIGHTING - GET_POS(ch)) / 4);

		if (GET_POS(victim) == POS_SITTING &&
		    (AFF_FLAGGED(victim, AFF_AIRSHIELD) ||
		     AFF_FLAGGED(victim, AFF_FIRESHIELD) || AFF_FLAGGED(victim, AFF_ICESHIELD))) {
			// жертва сидит в щите, повреждения не меняются
		} else if (GET_POS(victim) < POS_FIGHTING)
			dam += (dam * (POS_FIGHTING - GET_POS(victim)) / 3);

		if (GET_MOB_HOLD(victim))
			dam += (dam >> 1);

		// Cut damage in half if victim has sanct, to a minimum 1
		if (AFF_FLAGGED(victim, AFF_PRISMATICAURA))
			dam *= 2;
		if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
			dam /= 2;
		if (GET_SKILL(ch, SKILL_NOPARRYHIT))
			dam += (int) ((GET_LEVEL(ch) / 3 + GET_REMORT(ch) * 3) *
				GET_SKILL(ch, SKILL_NOPARRYHIT) / 
				skill_info[SKILL_NOPARRYHIT].max_percent);

		//dzMUDiST Обработка !исступления!
		if (affected_by_spell(ch, SPELL_BERSERK)) {
			range =
			    train_skill(ch, SKILL_BERSERK, skill_info[SKILL_BERSERK].max_percent, victim);
			if (AFF_FLAGGED(ch, AFF_BERSERK)) {
				dam = (dam * MAX (150, 100 + range + GET_LEVEL(ch) + dice (0, GET_REMORT(ch)) * 2)) / 100;
				calc_thaco -= (12 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
			}
		}

		// at least 1 hp damage min per hit
		dam = MAX(1, dam);
		if (weapon_pos)
			alt_equip(ch, weapon_pos, dam, 10);
 		dam_critic = 0;
		was_critic = 19;
		if (GET_CLASS(ch) == CLASS_THIEF) 
			was_critic -= (int) (GET_SKILL(ch,SKILL_BACKSTAB) / 60);
		if (GET_CLASS(ch) == CLASS_PALADINE) 
			was_critic -= (int) (GET_SKILL(ch,SKILL_PUNCTUAL) / 80);
		if (GET_CLASS(ch) == CLASS_ASSASINE) 
			was_critic -= (int) (GET_SKILL(ch,SKILL_NOPARRYHIT) / 100);

		//critical hit ignore magic_shields and armour
		if (diceroll > was_critic)
			was_critic = TRUE;
		else
			was_critic = FALSE;

		if (type == SKILL_BACKSTAB) {
			dam *= (GET_COMMSTATE(ch) ? 25 : backstab_mult(GET_LEVEL(ch)));
			/* если критбакстаб, то дамаж равен 95% хитов жертвы 
			   вероятность критстабба - стабб/20+ловкость-20 (кард) */
			/*+скр.удар/20 */
			if (IS_NPC(victim) && (number(1, 100) <
				(GET_SKILL(ch, SKILL_BACKSTAB) / 20 + GET_REAL_DEX(ch) - 20)))
	  			if (!general_savingthrow(victim,SAVING_REFLEX,
					MAX (0, GET_SKILL(ch,SKILL_BACKSTAB) -
						skill_info[SKILL_BACKSTAB].max_percent +
						dex_app[GET_REAL_DEX(ch)].reaction),0))	{
					dam *= MAX(2, (GET_SKILL(ch,SKILL_BACKSTAB) - 40) / 8);
					send_to_char("&GПрямо в сердце!&n\r\n", ch);
				}				 

//Adept: учитываем резисты от крит. повреждений
			dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, dam);
			extdamage(ch, victim, dam, w_type, 0, TRUE);
			return;
		} else if (type == SKILL_THROW) {
			dam *=
			    (GET_COMMSTATE(ch) ? 10
			     : (calculate_skill
				(ch, SKILL_THROW, skill_info[SKILL_THROW].max_percent, victim) + 10) / 10);
			if (IS_NPC(ch))
				dam = MIN(300, dam);
			dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, dam);
			extdamage(ch, victim, dam, w_type, 0, TRUE);
			return;
		} else {	// Critical hits
			if (GET_AF_BATTLE(ch, EAF_PUNCTUAL) &&
			    GET_PUNCTUAL_WAIT(ch) <= 0 &&
			    GET_WAIT(ch) <= 0 &&
			    (diceroll >= 18 - GET_MOB_HOLD(victim)) && !MOB_FLAGGED(victim, MOB_NOTKILLPUNCTUAL)) {
				percent =
				    train_skill(ch, SKILL_PUNCTUAL, skill_info[SKILL_PUNCTUAL].max_percent, victim);
				if (!PUNCTUAL_WAITLESS(ch))
					PUNCTUAL_WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				if (percent >= number(1, skill_info[SKILL_PUNCTUAL].max_percent)
				    && (calc_thaco - diceroll < victim_ac - 5
					|| percent >= skill_info[SKILL_PUNCTUAL].max_percent)) {

					was_critic = TRUE;
					dam_critic = do_punctual(ch, victim, wielded);

					if (!PUNCTUAL_WAITLESS(ch))
						PUNCTUAL_WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
				}
			}
		/* обнуляем флаги, если у нападающего есть лаг */
		if ((GET_AF_BATTLE (ch, EAF_STUPOR) ||
			GET_AF_BATTLE (ch, EAF_MIGHTHIT)) &&
			GET_WAIT (ch) > 0) {
		    CLR_AF_BATTLE (ch, EAF_STUPOR);
		    CLR_AF_BATTLE (ch, EAF_MIGHTHIT);
		    }
			
	/**** обработаем ситуацию ЗАХВАТ */
			for (vict = world[IN_ROOM(ch)]->people;
			     vict && dam >= 0 && type != TYPE_NOPARRY &&
			     !GET_AF_BATTLE(ch, EAF_MIGHTHIT) &&
			     !GET_AF_BATTLE(ch, EAF_STUPOR); vict = vict->next_in_room) {
				if (TOUCHING(vict) == ch &&
				    !AFF_FLAGGED(vict, AFF_STOPFIGHT) &&
				    !AFF_FLAGGED(vict, AFF_MAGICSTOPFIGHT) &&
				    !AFF_FLAGGED(vict, AFF_STOPRIGHT) &&
				    GET_WAIT(vict) <= 0 &&
				    !GET_MOB_HOLD(vict) &&
				    (IS_IMMORTAL(vict) ||
				     IS_NPC(vict) ||
				     GET_GOD_FLAG(vict, GF_GODSLIKE) ||
				     !(GET_EQ(vict, WEAR_WIELD) || GET_EQ(vict, WEAR_BOTHS)))
				    && GET_POS(vict) > POS_SLEEPING) {
					percent = number(1, skill_info[SKILL_TOUCH].max_percent);
					prob = train_skill(vict, SKILL_TOUCH, skill_info[SKILL_TOUCH].max_percent, ch);
					if (IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE))
						percent = prob;
					if (GET_GOD_FLAG(vict, GF_GODSCURSE))
						percent = 0;
					CLR_AF_BATTLE(vict, EAF_TOUCH);
					SET_AF_BATTLE(vict, EAF_USEDRIGHT);
					TOUCHING(vict) = NULL;
					if (prob < percent) {
						act("Вы не смогли перехватить атаку $N1.", FALSE, vict, 0, ch, TO_CHAR);
						act("$N не смог$Q перехватить Вашу атаку.", FALSE, ch,
						    0, vict, TO_CHAR);
						act("$n не смог$q перехватить атаку $N1.", TRUE, vict,
						    0, ch, TO_NOTVICT);
						prob = 2;
					} else {
						act("Вы перехватили атаку $N1.", FALSE, vict, 0, ch, TO_CHAR);
						act("$N перехватил$G Вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n перехватил$g атаку $N1.", TRUE, vict, 0, ch, TO_NOTVICT);
						dam = -1;
						prob = 1;
					}
					if (!WAITLESS(vict))
						WAIT_STATE(vict, prob * PULSE_VIOLENCE);
				}
			}

	/**** Обработаем команду   УКЛОНИТЬСЯ */
			if (dam > 0 && type != TYPE_NOPARRY &&
			    !GET_AF_BATTLE(ch, EAF_MIGHTHIT) &&
			    !GET_AF_BATTLE(ch, EAF_STUPOR) &&
			    GET_AF_BATTLE(victim, EAF_DEVIATE) &&
			    GET_WAIT(victim) <= 0 &&
			    !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
			    !AFF_FLAGGED(victim, AFF_MAGICSTOPFIGHT) &&
			    GET_MOB_HOLD(victim) == 0 && BATTLECNTR(victim) <= (GET_LEVEL(victim) + 7 / 8)) {
				range = number(1, skill_info[SKILL_DEVIATE].max_percent);
				prob = train_skill(victim, SKILL_DEVIATE, skill_info[SKILL_DEVIATE].max_percent, ch);
				if (GET_GOD_FLAG(victim, GF_GODSCURSE))
					prob = 0;
				prob = (int) (prob * 100 / range);
				if (prob < 60) {
					act("Вы не смогли уклонитьcя от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
					act("$N не сумел$G уклониться от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
					act("$n не сумел$g уклониться от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
				} else if (prob < 100) {
					act("Вы немного уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
					act("$N немного уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
					act("$n немного уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
					dam = (int) (dam / 1.5);
				} else if (prob < 200) {
					act("Вы частично уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
					act("$N частично уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
					act("$n частично уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
					dam = (int) (dam / 2);
				} else {
					act("Вы уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
					act("$N уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
					act("$n уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
					dam = -1;
				}
				BATTLECNTR(victim)++;
			} else
	/**** обработаем команду  ПАРИРОВАТЬ */
			if (dam > 0 && type != TYPE_NOPARRY &&
				    !GET_AF_BATTLE(ch, EAF_MIGHTHIT) &&
				    !GET_AF_BATTLE(ch, EAF_STUPOR) &&
				    GET_AF_BATTLE(victim, EAF_PARRY) &&
				    !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
				    !AFF_FLAGGED(victim, AFF_MAGICSTOPFIGHT) &&
				    !AFF_FLAGGED(victim, AFF_STOPRIGHT) &&
				    !AFF_FLAGGED(victim, AFF_STOPLEFT) &&
				    GET_WAIT(victim) <= 0 && GET_MOB_HOLD(victim) == 0) {
				if (!((GET_EQ(victim, WEAR_WIELD)
				       && GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) ==
				       ITEM_WEAPON && GET_EQ(victim, WEAR_HOLD)
				       && GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) ==
				       ITEM_WEAPON) || IS_NPC(victim) || IS_IMMORTAL(victim)
				      || GET_GOD_FLAG(victim, GF_GODSLIKE))) {
					send_to_char("У Вас нечем отклонить атаку противника\r\n", victim);
					CLR_AF_BATTLE(victim, EAF_PARRY);
				} else {
					range = number(1, skill_info[SKILL_PARRY].max_percent);
					prob =
					    train_skill(victim, SKILL_PARRY, skill_info[SKILL_PARRY].max_percent, ch);
					prob = (int) (prob * 100 / range);

					if (prob < 70 || ((skill == SKILL_BOWS || w_type == TYPE_MAUL)
							  && !IS_IMMORTAL(victim))) {
						act("Вы не смогли отбить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N не сумел$G отбить Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n не сумел$g отбить атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
						prob = 2;
						SET_AF_BATTLE(victim, EAF_USEDLEFT);
					} else if (prob < 100) {
						act("Вы немного отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N немного отклонил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n немного отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
						alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, dam, 10);
						prob = 1;
						dam = (int) (dam / 1.5);
						SET_AF_BATTLE(victim, EAF_USEDLEFT);
					} else if (prob < 170) {
						act("Вы частично отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N частично отклонил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n частично отклонил$g атаку $N1", TRUE, victim,
						    0, ch, TO_NOTVICT);
						alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, dam, 15);
						prob = 0;
						dam = (int) (dam / 2);
						SET_AF_BATTLE(victim, EAF_USEDLEFT);
					} else {
						act("Вы полностью отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N полностью отклонил$G Вашу атаку", FALSE, ch, 0,
						    victim, TO_CHAR);
						act("$n полностью отклонил$g атаку $N1", TRUE, victim,
						    0, ch, TO_NOTVICT);
						alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, dam, 25);
						prob = 0;
						dam = -1;
					}
					if (!WAITLESS(ch) && prob)
						WAIT_STATE(victim, PULSE_VIOLENCE * prob);
					CLR_AF_BATTLE(victim, EAF_PARRY);
				}
			} else
	/**** обработаем команду  ВЕЕРНАЯ ЗАЩИТА */
			if (dam > 0 && type != TYPE_NOPARRY &&
				    !GET_AF_BATTLE(ch, EAF_MIGHTHIT) &&
				    !GET_AF_BATTLE(ch, EAF_STUPOR) &&
				    GET_AF_BATTLE(victim, EAF_MULTYPARRY) &&
				    !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
				    !AFF_FLAGGED(victim, AFF_MAGICSTOPFIGHT) &&
				    !AFF_FLAGGED(victim, AFF_STOPRIGHT) &&
				    !AFF_FLAGGED(victim, AFF_STOPLEFT) &&
				    BATTLECNTR(victim) < (GET_LEVEL(victim) + 4) / 5 &&
				    GET_WAIT(victim) <= 0 && GET_MOB_HOLD(victim) == 0) {
				if (!((GET_EQ(victim, WEAR_WIELD)
				       && GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) ==
				       ITEM_WEAPON && GET_EQ(victim, WEAR_HOLD)
				       && GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) ==
				       ITEM_WEAPON) || IS_NPC(victim) || IS_IMMORTAL(victim)
				      || GET_GOD_FLAG(victim, GF_GODSLIKE)))
					send_to_char("У Вас нечем отклонять атаки противников\r\n", victim);
				else {
					range =
					    number(1,
						   skill_info[SKILL_MULTYPARRY].max_percent) + 15 * BATTLECNTR(victim);
					prob =
					    train_skill(victim, SKILL_MULTYPARRY,
							skill_info[SKILL_MULTYPARRY].max_percent +
							BATTLECNTR(ch) * 15, ch);
					prob = (int) (prob * 100 / range);
					if ((skill == SKILL_BOWS || w_type == TYPE_MAUL)
					    && !IS_IMMORTAL(victim))
						prob = 0;
					else
						BATTLECNTR(victim)++;

					if (prob < 50) {
						act("Вы не смогли отбить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N не сумел$G отбить Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n не сумел$g отбить атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
					} else if (prob < 90) {
						act("Вы немного отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N немного отклонил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n немного отклонил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
						alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, dam, 10);
						dam = (int) (dam / 1.5);
					} else if (prob < 180) {
						act("Вы частично отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N частично отклонил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n частично отклонил$g атаку $N1", TRUE, victim,
						    0, ch, TO_NOTVICT);
						alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, dam, 15);
						dam = (int) (dam / 2);
					} else {
						act("Вы полностью отклонили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N полностью отклонил$G Вашу атаку", FALSE, ch, 0,
						    victim, TO_CHAR);
						act("$n полностью отклонил$g атаку $N1", TRUE, victim,
						    0, ch, TO_NOTVICT);
						alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, dam, 25);
						dam = -1;
					}
				}
			} else
	/**** Обработаем команду   БЛОКИРОВАТЬ */
			if (dam > 0 && type != TYPE_NOPARRY &&
				    !GET_AF_BATTLE(ch, EAF_MIGHTHIT) &&
				    !GET_AF_BATTLE(ch, EAF_STUPOR) &&
				    GET_AF_BATTLE(victim, EAF_BLOCK) &&
				    !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
				    !AFF_FLAGGED(victim, AFF_MAGICSTOPFIGHT) &&
				    !AFF_FLAGGED(victim, AFF_STOPLEFT) &&
				    GET_WAIT(victim) <= 0 &&
				    GET_MOB_HOLD(victim) == 0 && BATTLECNTR(victim) < (GET_LEVEL(victim) + 8) / 9) {
				if (!(GET_EQ(victim, WEAR_SHIELD) ||
				      IS_NPC(victim) || IS_IMMORTAL(victim) || GET_GOD_FLAG(victim, GF_GODSLIKE)))
					send_to_char("У Вас нечем отразить атаку противника\r\n", victim);
				else {
					range = number(1, skill_info[SKILL_BLOCK].max_percent);
					prob =
					    train_skill(victim, SKILL_BLOCK, skill_info[SKILL_BLOCK].max_percent, ch);
					prob = (int) (prob * 100 / range);
					BATTLECNTR(victim)++;
					if (prob < 100) {
						act("Вы не смогли отразить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N не сумел$G отразить Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n не сумел$g отразить атаку $N1", TRUE, victim,
						    0, ch, TO_NOTVICT);
					} else if (prob < 150) {
						act("Вы немного отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N немного отразил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n немного отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
						alt_equip(victim, WEAR_SHIELD, dam, 10);
						dam = (int) (dam / 1.5);
					} else if (prob < 250) {
						act("Вы частично отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N частично отразил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n частично отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
						alt_equip(victim, WEAR_SHIELD, dam, 15);
						dam = (int) (dam / 2);
					} else {
						act("Вы полностью отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
						act("$N полностью отразил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
						act("$n полностью отразил$g атаку $N1", TRUE, victim,
						    0, ch, TO_NOTVICT);
						alt_equip(victim, WEAR_SHIELD, dam, 25);
						dam = -1;
					}
				}
			};

			extdamage(ch, victim, dam, w_type, wielded, TRUE);
			was_critic = FALSE;
			dam_critic = 0;
		}
	}

	/* check if the victim has a hitprcnt trigger */
	hitprcnt_mtrigger(victim);

	/* Вписываем всех в группе кто с автопомощью у атакующего */
//  if (AFF_FLAGGED (ch, AFF_GROUP))
//      go_autoassist (ch);
	/* Вписываем всех в группе кто с автопомощью у цели */
//  if (AFF_FLAGGED (victim, AFF_GROUP))
//      go_autoassist (victim);
}


int GET_MAXDAMAGE(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_HOLD))
		return 0;
	else
		return GET_DAMAGE(ch);
}

int GET_MAXCASTER(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_HOLD) || AFF_FLAGGED(ch, AFF_SIELENCE)
	    || GET_WAIT(ch) > 0)
		return 0;
	else
		return IS_IMMORTAL(ch) ? 1 : GET_CASTER(ch);
}

#define GET_HP_PERC(ch) ((int)(GET_HIT(ch) * 100 / GET_MAX_HIT(ch)))
#define POOR_DAMAGE  15
#define POOR_CASTER  5
#define MAX_PROBES   0
#define SpINFO       spell_info[i]

int in_same_battle(CHAR_DATA * npc, CHAR_DATA * pc, int opponent)
{
	int ch_friend_npc, ch_friend_pc, vict_friend_npc, vict_friend_pc;
	CHAR_DATA *ch, *vict, *npc_master, *pc_master, *ch_master, *vict_master;

	if (npc == pc)
		return (!opponent);
	if (FIGHTING(npc) == pc)	// NPC fight PC - opponent
		return (opponent);
	if (FIGHTING(pc) == npc)	// PC fight NPC - opponent
		return (opponent);
	if (FIGHTING(npc) && FIGHTING(npc) == FIGHTING(pc))
		return (!opponent);	// Fight same victim - friend
	if (AFF_FLAGGED(pc, AFF_HORSE) || AFF_FLAGGED(pc, AFF_CHARM))
		return (opponent);

	npc_master = npc->master ? npc->master : npc;
	pc_master = pc->master ? pc->master : pc;

	for (ch = world[IN_ROOM(npc)]->people; ch; ch = ch->next) {
		if (!FIGHTING(ch))
			continue;
		ch_master = ch->master ? ch->master : ch;
		ch_friend_npc = (ch_master == npc_master) ||
		    (IS_NPC(ch) && IS_NPC(npc) &&
		     !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
		     !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE));
		ch_friend_pc = (ch_master == pc_master) ||
		    (IS_NPC(ch) && IS_NPC(pc) &&
		     !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
		     !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE));
		if (FIGHTING(ch) == pc && ch_friend_npc)	// Friend NPC fight PC - opponent
			return (opponent);
		if (FIGHTING(pc) == ch && ch_friend_npc)	// PC fight friend NPC - opponent
			return (opponent);
		if (FIGHTING(npc) == ch && ch_friend_pc)	// NPC fight friend PC - opponent
			return (opponent);
		if (FIGHTING(ch) == npc && ch_friend_pc)	// Friend PC fight NPC - opponent
			return (opponent);
		vict = FIGHTING(ch);
		vict_master = vict->master ? vict->master : vict;
		vict_friend_npc = (vict_master == npc_master) ||
		    (IS_NPC(vict) && IS_NPC(npc) &&
		     !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
		     !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE));
		vict_friend_pc = (vict_master == pc_master) ||
		    (IS_NPC(vict) && IS_NPC(pc) &&
		     !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
		     !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE));
		if (ch_friend_npc && vict_friend_pc)
			return (opponent);	// Friend NPC fight friend PC - opponent
		if (ch_friend_pc && vict_friend_npc)
			return (opponent);	// Friend PC fight friend NPC - opponent
	}

	return (!opponent);
}

CHAR_DATA *find_friend_cure(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, AFF_USED = 0;
	switch (spellnum) {
	case SPELL_CURE_LIGHT:
		AFF_USED = 80;
		break;
	case SPELL_CURE_SERIOUS:
		AFF_USED = 70;
		break;
	case SPELL_EXTRA_HITS:
	case SPELL_CURE_CRITIC:
		AFF_USED = 50;
		break;
	case SPELL_HEAL:
	case SPELL_GROUP_HEAL:
		AFF_USED = 30;
		break;
	}

	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL))
	    && AFF_FLAGGED(caster, AFF_HELPER)) {
		if (GET_HP_PERC(caster) < AFF_USED)
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
			 CAN_SEE(caster, caster->master) &&
			 IN_ROOM(caster->master) == IN_ROOM(caster) &&
			 FIGHTING(caster->master) && GET_HP_PERC(caster->master) < AFF_USED)
			return (caster->master);
		return (NULL);
	}

	for (vict = world[IN_ROOM(caster)]->people; AFF_USED && vict; vict = vict->next_in_room) {
		if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
								      && (vict->master && !IS_NPC(vict->master)))
		    || !CAN_SEE(caster, vict))
			continue;
		if (!FIGHTING(vict) && !MOB_FLAGGED(vict, MOB_HELPER))
			continue;
		if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict))) {
			victim = vict;
			vict_val = GET_HP_PERC(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
				break;
		}
	}
	return (victim);
}

CHAR_DATA *find_friend(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, AFF_USED = 0, spellreal = -1;
	switch (spellnum) {
	case SPELL_CURE_BLIND:
		SET_BIT(AFF_USED, AFF_BLIND);
		break;
	case SPELL_REMOVE_POISON:
		SET_BIT(AFF_USED, AFF_POISON);
		break;
	case SPELL_REMOVE_HOLD:
		SET_BIT(AFF_USED, AFF_HOLD);
		break;
	case SPELL_REMOVE_CURSE:
		SET_BIT(AFF_USED, AFF_CURSE);
		break;
	case SPELL_REMOVE_SIELENCE:
		SET_BIT(AFF_USED, AFF_SIELENCE);
		break;
	case SPELL_CURE_PLAQUE:
		spellreal = SPELL_PLAQUE;
		break;

	}
	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)) && AFF_FLAGGED(caster, AFF_HELPER)) {
		if (AFF_FLAGGED(caster, AFF_USED) || affected_by_spell(caster, spellreal))
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
			 CAN_SEE(caster, caster->master) && IN_ROOM(caster->master) == IN_ROOM(caster) &&
//         FIGHTING(caster->master)                   &&
			 (AFF_FLAGGED(caster->master, AFF_USED) || affected_by_spell(caster->master, spellreal)))
			return (caster->master);
		return (NULL);
	}

	for (vict = world[IN_ROOM(caster)]->people; AFF_USED && vict; vict = vict->next_in_room) {
		if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
								      && (vict->master && !IS_NPC(vict->master)))
		    || !CAN_SEE(caster, vict))
			continue;
		if (!AFF_FLAGGED(vict, AFF_USED))
			continue;
		if (!FIGHTING(vict) && !MOB_FLAGGED(vict, MOB_HELPER))
			continue;
		if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
			victim = vict;
			vict_val = GET_MAXDAMAGE(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
				break;
		}
	}
	return (victim);
}

CHAR_DATA *find_caster(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict = NULL, *victim = NULL;
	int vict_val = 0, AFF_USED, spellreal = -1;
	AFF_USED = 0;
	switch (spellnum) {
	case SPELL_CURE_BLIND:
		SET_BIT(AFF_USED, AFF_BLIND);
		break;
	case SPELL_REMOVE_POISON:
		SET_BIT(AFF_USED, AFF_POISON);
		break;
	case SPELL_REMOVE_HOLD:
		SET_BIT(AFF_USED, AFF_HOLD);
		break;
	case SPELL_REMOVE_CURSE:
		SET_BIT(AFF_USED, AFF_CURSE);
		break;
	case SPELL_REMOVE_SIELENCE:
		SET_BIT(AFF_USED, AFF_SIELENCE);
		break;
	case SPELL_CURE_PLAQUE:
		spellreal = SPELL_PLAQUE;
		break;
	}

	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)) && AFF_FLAGGED(caster, AFF_HELPER)) {
		if (AFF_FLAGGED(caster, AFF_USED) || affected_by_spell(caster, spellreal))
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
			 CAN_SEE(caster, caster->master) && IN_ROOM(caster->master) == IN_ROOM(caster) &&
//         FIGHTING(caster->master)                   &&
			 (AFF_FLAGGED(caster->master, AFF_USED) || affected_by_spell(caster->master, spellreal)))
			return (caster->master);
		return (NULL);
	}

	for (vict = world[IN_ROOM(caster)]->people; AFF_USED && vict; vict = vict->next_in_room) {
		if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
								      && (vict->master && !IS_NPC(vict->master)))
		    || !CAN_SEE(caster, vict))
			continue;
		if (!AFF_FLAGGED(vict, AFF_USED))
			continue;
		if (!FIGHTING(vict) && !MOB_FLAGGED(vict, MOB_HELPER))
			continue;
		if (!victim || vict_val < GET_MAXCASTER(vict)) {
			victim = vict;
			vict_val = GET_MAXCASTER(vict);
			if (GET_REAL_INT(caster) < number(10, 20))
				break;
		}
	}
	return (victim);
}


CHAR_DATA *find_affectee(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == SPELL_GROUP_ARMOR)
		spellreal = SPELL_ARMOR;
	else if (spellreal == SPELL_GROUP_STRENGTH)
		spellreal = SPELL_STRENGTH;
	else if (spellreal == SPELL_GROUP_BLESS)
		spellreal = SPELL_BLESS;
	else if (spellreal == SPELL_GROUP_HASTE)
		spellreal = SPELL_HASTE;
	else if (spellreal == SPELL_GROUP_SANCTUARY)
		spellreal = SPELL_SANCTUARY;
	else if (spellreal == SPELL_GROUP_PRISMATICAURA)
		spellreal = SPELL_PRISMATICAURA;

	if ((AFF_FLAGGED(caster, AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)) && AFF_FLAGGED(caster, AFF_HELPER)) {
		if (!affected_by_spell(caster, spellreal))
			return (caster);
		else if (caster->master &&
//         !IS_NPC(caster->master)                    &&
			 CAN_SEE(caster, caster->master) &&
			 IN_ROOM(caster->master) == IN_ROOM(caster) &&
			 FIGHTING(caster->master) && !affected_by_spell(caster->master, spellreal))
			return (caster->master);
		return (NULL);
	}

	if (GET_REAL_INT(caster) > number(5, 15))
		for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room) {
			if (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || (MOB_FLAGGED(vict, MOB_ANGEL)
									      && (vict->master
										  && !IS_NPC(vict->master)))
			    || !CAN_SEE(caster, vict))
				continue;
			if (!FIGHTING(vict) || AFF_FLAGGED(vict, AFF_HOLD) || affected_by_spell(vict, spellreal))
				continue;
			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	if (!victim && !affected_by_spell(caster, spellreal))
		victim = caster;

	return (victim);
}

CHAR_DATA *find_opp_affectee(CHAR_DATA * caster, int spellnum)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0, spellreal = spellnum;

	if (spellreal == SPELL_POWER_HOLD || spellreal == SPELL_MASS_HOLD)
		spellreal = SPELL_HOLD;
	else if (spellreal == SPELL_POWER_BLINDNESS || spellreal == SPELL_MASS_BLINDNESS)
		spellreal = SPELL_BLINDNESS;
	else if (spellreal == SPELL_POWER_SIELENCE || spellreal == SPELL_MASS_SIELENCE)
		spellreal = SPELL_SIELENCE;
	else if (spellreal == SPELL_MASS_CURSE)
		spellreal = SPELL_CURSE;
	else if (spellreal == SPELL_MASS_SLOW)
		spellreal = SPELL_SLOW;

	if (GET_REAL_INT(caster) > number(10, 20))
		for (vict = world[caster->in_room]->people; vict; vict = vict->next_in_room) {
			if ((IS_NPC(vict) && !((MOB_FLAGGED(vict, MOB_ANGEL)
						|| AFF_FLAGGED(vict, AFF_CHARM)) && (vict->master
										     && !IS_NPC(vict->master))))
			    || !CAN_SEE(caster, vict))
				continue;
			if ((!FIGHTING(vict)
			     && (GET_REAL_INT(caster) < number(20, 27)
				 || !in_same_battle(caster, vict, TRUE)))
			    || AFF_FLAGGED(vict, AFF_HOLD)
			    || affected_by_spell(vict, spellreal))
				continue;
			if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}

	if (!victim && FIGHTING(caster)
	    && !affected_by_spell(FIGHTING(caster), spellreal))
		victim = FIGHTING(caster);
	return (victim);
}

CHAR_DATA *find_opp_caster(CHAR_DATA * caster)
{
	CHAR_DATA *vict = NULL, *victim = NULL;
	int vict_val = 0;

	for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room) {
		if (IS_NPC(vict) &&
//         !AFF_FLAGGED(vict,AFF_CHARM) && 
		    !(MOB_FLAGGED(vict, MOB_ANGEL)
		      && (vict->master && !IS_NPC(vict->master))))
			continue;
		if ((!FIGHTING(vict)
		     && (GET_REAL_INT(caster) < number(15, 25)
			 || !in_same_battle(caster, vict, TRUE)))
		    || AFF_FLAGGED(vict, AFF_HOLD) || AFF_FLAGGED(vict, AFF_SIELENCE)
		    || (!CAN_SEE(caster, vict) && FIGHTING(caster) != vict))
			continue;
		if (vict_val < GET_MAXCASTER(vict)) {
			victim = vict;
			vict_val = GET_MAXCASTER(vict);
		}
	}
	return (victim);
}

CHAR_DATA *find_damagee(CHAR_DATA * caster)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20))
		for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room) {
			if ((IS_NPC(vict) && !((MOB_FLAGGED(vict, MOB_ANGEL)
						|| AFF_FLAGGED(vict, AFF_CHARM)) && (vict->master
										     && !IS_NPC(vict->master))))
			    || !CAN_SEE(caster, vict))
				continue;
			if ((!FIGHTING(vict)
			     && (GET_REAL_INT(caster) < number(20, 27)
				 || !in_same_battle(caster, vict, TRUE)))
			    || AFF_FLAGGED(vict, AFF_HOLD))
				continue;
			if (GET_REAL_INT(caster) >= number(25, 30)) {
				if (!victim || vict_val < GET_MAXCASTER(vict)) {
					victim = vict;
					vict_val = GET_MAXCASTER(vict);
				}
			} else if (!victim || vict_val < GET_MAXDAMAGE(vict)) {
				victim = vict;
				vict_val = GET_MAXDAMAGE(vict);
			}
		}
	if (!victim)
		victim = FIGHTING(caster);

	return (victim);
}

CHAR_DATA *find_minhp(CHAR_DATA * caster)
{
	CHAR_DATA *vict, *victim = NULL;
	int vict_val = 0;

	if (GET_REAL_INT(caster) > number(10, 20))
		for (vict = world[IN_ROOM(caster)]->people; vict; vict = vict->next_in_room) {
			if ((IS_NPC(vict) && !((MOB_FLAGGED(vict, MOB_ANGEL)
						|| AFF_FLAGGED(vict, AFF_CHARM)) && (vict->master
										     && !IS_NPC(vict->master))))
			    || !CAN_SEE(caster, vict))
				continue;
			if (!FIGHTING(vict) && (GET_REAL_INT(caster) < number(20, 27)
						|| !in_same_battle(caster, vict, TRUE)))
				continue;
			if (!victim || vict_val > GET_HIT(vict)) {
				victim = vict;
				vict_val = GET_HIT(vict);
			}
		}
	if (!victim)
		victim = FIGHTING(caster);

	return (victim);
}

CHAR_DATA *find_cure(CHAR_DATA * caster, CHAR_DATA * patient, int *spellnum)
{
	if (GET_HP_PERC(patient) <= number(20, 33)) {
		if (GET_SPELL_MEM(caster, SPELL_EXTRA_HITS))
			*spellnum = SPELL_EXTRA_HITS;
		else if (GET_SPELL_MEM(caster, SPELL_HEAL))
			*spellnum = SPELL_HEAL;
		else if (GET_SPELL_MEM(caster, SPELL_CURE_CRITIC))
			*spellnum = SPELL_CURE_CRITIC;
		else if (GET_SPELL_MEM(caster, SPELL_GROUP_HEAL))
			*spellnum = SPELL_GROUP_HEAL;
	} else if (GET_HP_PERC(patient) <= number(50, 65)) {
		if (GET_SPELL_MEM(caster, SPELL_CURE_CRITIC))
			*spellnum = SPELL_CURE_CRITIC;
		else if (GET_SPELL_MEM(caster, SPELL_CURE_SERIOUS))
			*spellnum = SPELL_CURE_SERIOUS;
		else if (GET_SPELL_MEM(caster, SPELL_CURE_LIGHT))
			*spellnum = SPELL_CURE_LIGHT;
	}
	if (*spellnum)
		return (patient);
	else
		return (NULL);
}

void mob_casting(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	int battle_spells[MAX_STRING_LENGTH];
	int lag = GET_WAIT(ch), i, spellnum, spells, sp_num;
	OBJ_DATA *item;

	if (AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HOLD) || AFF_FLAGGED(ch, AFF_SIELENCE) || lag > 0)
		return;

	memset(&battle_spells, 0, sizeof(battle_spells));
	for (i = 1, spells = 0; i <= MAX_SPELLS; i++)
		if (GET_SPELL_MEM(ch, i) && IS_SET(SpINFO.routines, NPC_CALCULATE))
			battle_spells[spells++] = i;

	for (item = ch->carrying;
	     spells < MAX_STRING_LENGTH &&
	     item &&
	     GET_CLASS(ch) != CLASS_ANIMAL &&
	     !MOB_FLAGGED(ch, MOB_ANGEL) && !AFF_FLAGGED(ch, AFF_CHARM); item = item->next_content)
		switch (GET_OBJ_TYPE(item)) {
		case ITEM_WAND:
		case ITEM_STAFF:
			if (GET_OBJ_VAL(item, 2) > 0 &&
			    IS_SET(spell_info[GET_OBJ_VAL(item, 3)].routines, NPC_CALCULATE))
				battle_spells[spells++] = GET_OBJ_VAL(item, 3);
			break;
		case ITEM_POTION:
			for (i = 1; i <= 3; i++)
				if (IS_SET
				    (spell_info[GET_OBJ_VAL(item, i)].routines,
				     NPC_AFFECT_NPC | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER))
					battle_spells[spells++] = GET_OBJ_VAL(item, i);
			break;
		case ITEM_SCROLL:
			for (i = 1; i <= 3; i++)
				if (IS_SET(spell_info[GET_OBJ_VAL(item, i)].routines, NPC_CALCULATE))
					battle_spells[spells++] = GET_OBJ_VAL(item, i);
			break;
		}

	// перво-наперво  -  лечим себя
	spellnum = 0;
	victim = find_cure(ch, ch, &spellnum);
	// Ищем рандомную заклинашку и цель для нее
	for (i = 0; !victim && spells && i < GET_REAL_INT(ch) / 5; i++)
		if (!spellnum && (spellnum = battle_spells[(sp_num = number(0, spells - 1))])
		    && spellnum > 0 && spellnum <= MAX_SPELLS) {	// sprintf(buf,"$n using spell '%s', %d from %d",
			//         spell_name(spellnum), sp_num, spells);
			// act(buf,FALSE,ch,0,FIGHTING(ch),TO_VICT);
			if (spell_info[spellnum].routines & NPC_DAMAGE_PC_MINHP) {
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_minhp(ch);
			} else if (spell_info[spellnum].routines & NPC_DAMAGE_PC) {
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_damagee(ch);
			} else if (spell_info[spellnum].routines & NPC_AFFECT_PC_CASTER) {
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_opp_caster(ch);
			} else if (spell_info[spellnum].routines & NPC_AFFECT_PC) {
				if (!AFF_FLAGGED(ch, AFF_CHARM))
					victim = find_opp_affectee(ch, spellnum);
			} else if (spell_info[spellnum].routines & NPC_AFFECT_NPC)
				victim = find_affectee(ch, spellnum);
			else if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC_CASTER)
				victim = find_caster(ch, spellnum);
			else if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC)
				victim = find_friend(ch, spellnum);
			else if (spell_info[spellnum].routines & NPC_DUMMY)
				victim = find_friend_cure(ch, spellnum);
			else
				spellnum = 0;
		}
	if (spellnum && victim) {	// Is this object spell ?
		for (item = ch->carrying;
		     !AFF_FLAGGED(ch, AFF_CHARM) &&
		     !MOB_FLAGGED(ch, MOB_ANGEL) && item && GET_CLASS(ch) != CLASS_ANIMAL; item = item->next_content)
			switch (GET_OBJ_TYPE(item)) {
			case ITEM_WAND:
			case ITEM_STAFF:
				if (GET_OBJ_VAL(item, 2) > 0 && GET_OBJ_VAL(item, 3) == spellnum) {
					mag_objectmagic(ch, item, GET_NAME(victim));
					return;
				}
				break;
			case ITEM_POTION:
				for (i = 1; i <= 3; i++)
					if (GET_OBJ_VAL(item, i) == spellnum) {
						if (ch != victim) {
							obj_from_char(item);
							act("$n передал$g $o3 $N2.", FALSE, ch, item, victim, TO_ROOM);
							obj_to_char(item, victim);
						} else
							victim = ch;
						mag_objectmagic(victim, item, GET_NAME(victim));
						return;
					}
				break;
			case ITEM_SCROLL:
				for (i = 1; i <= 3; i++)
					if (GET_OBJ_VAL(item, i) == spellnum) {
						mag_objectmagic(ch, item, GET_NAME(victim));
						return;
					}
				break;
			}

		cast_spell(ch, victim, 0, NULL, spellnum);
	}
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HELPER)) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)

#define	MAY_ACT(ch)	(!(AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT) || GET_MOB_HOLD(ch) || GET_WAIT(ch)))

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
	CHAR_DATA *ch, *vict, *caster = NULL, *damager = NULL;
	int i, do_this, initiative, max_init = 0, min_init = 100, sk_use = 0, sk_num = 0;
	struct helper_data_type *helpee;
	struct follow_type *k, *k_next;

	// Step 0.0 Summons mob helpers

	for (ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;
		// Extract battler if no opponent
		if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)) || IN_ROOM(ch) == NOWHERE) {
			stop_fighting(ch, TRUE);
			continue;
		}
		if (GET_MOB_HOLD(ch) ||
		    !IS_NPC(ch) ||
		    GET_WAIT(ch) > 0 ||
		    GET_POS(ch) < POS_FIGHTING ||
		    AFF_FLAGGED(ch, AFF_CHARM) ||
		    AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT) ||
		    AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_SIELENCE))
			continue;

		if (!PRF_FLAGGED(FIGHTING(ch), PRF_NOHASSLE))
			for (sk_use = 0, helpee = GET_HELPER(ch); helpee; helpee = helpee->next_helper)
				for (vict = character_list; vict; vict = vict->next) {
					if (!IS_NPC(vict) ||
					    GET_MOB_VNUM(vict) != helpee->mob_vnum ||
					    AFF_FLAGGED(ch, AFF_CHARM) ||
					    AFF_FLAGGED(vict, AFF_HOLD) ||
					    AFF_FLAGGED(vict, AFF_CHARM) ||
					    AFF_FLAGGED(vict, AFF_BLIND) ||
					    GET_WAIT(vict) > 0 ||
					    GET_POS(vict) < POS_STANDING || IN_ROOM(vict) == NOWHERE || FIGHTING(vict))
						continue;
					if (!sk_use &&
					    !(GET_CLASS(ch) == CLASS_ANIMAL || GET_CLASS(ch) == CLASS_BASIC_NPC))
						act("$n воззвал$g : \"На помощь, мои верные соратники !\"",
						    FALSE, ch, 0, 0, TO_ROOM);
					if (IN_ROOM(vict) != IN_ROOM(ch)) {
						char_from_room(vict);
						char_to_room(vict, IN_ROOM(ch));
						act("$n прибыл$g на зов и вступил$g на помощь $N2.", FALSE,
						    vict, 0, ch, TO_ROOM);
					} else
						act("$n вступил$g в битву на стороне $N1.", FALSE, vict, 0,
						    ch, TO_ROOM);
					set_fighting(vict, FIGHTING(ch));
				};
	}


	// Step 1. Define initiative, mob casting and mob flag skills
	for (ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;
		// Initialize initiative
		INITIATIVE(ch) = 0;
		BATTLECNTR(ch) = 0;
		SET_AF_BATTLE(ch, EAF_STAND);
		if (affected_by_spell(ch, SPELL_SLEEP))
			SET_AF_BATTLE(ch, EAF_SLEEP);
		if (IN_ROOM(ch) == NOWHERE)
			continue;

		if (GET_MOB_HOLD(ch) || AFF_FLAGGED(ch, AFF_STOPFIGHT)
		    || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT))
//ужжжас
		{
			for (k = ch->followers; k; k = k_next) {
				k_next = k->next;
				if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
				    MOB_FLAGGED(k->follower, MOB_ANGEL) &&
				    !FIGHTING(k->follower) &&
				    IN_ROOM(k->follower) == IN_ROOM(ch) &&
				    CAN_SEE(k->follower, ch) && AWAKE(k->follower) &&
				    MAY_ACT(k->follower) && GET_POS(k->follower) >= POS_FIGHTING) {
					for (vict = world[IN_ROOM(ch)]->people; vict; vict = vict->next_in_room)
						if (FIGHTING(vict) == ch && vict != ch && vict != k->follower)
							break;
					if (vict && GET_SKILL(k->follower, SKILL_RESCUE)) {	//if(GET_MOB_VNUM(k->follower)==108) 
//       act("TRYING RESC for STOPFIGHT", TRUE, ch, 0, 0, TO_CHAR); 
						go_rescue(k->follower, ch, vict);
					}
				}
			}
			continue;
		}
		// Mobs stand up 
		if (IS_NPC(ch) &&
		    GET_POS(ch) < POS_FIGHTING &&
		    GET_POS(ch) > POS_STUNNED &&
		    GET_WAIT(ch) <= 0 && !GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, AFF_SLEEP)) {
			GET_POS(ch) = POS_FIGHTING;
			act("$n встал$g на ноги.", TRUE, ch, 0, 0, TO_ROOM);
		}
		// For NPC without lags and charms make it likes
		if (IS_NPC(ch) && MAY_LIKES(ch)) {	// Get weapon from room
			if (!AFF_FLAGGED(ch, AFF_CHARM) && npc_battle_scavenge(ch)) {
				npc_wield(ch);
				npc_armor(ch);
			}
			//dzMUDiST. Выполнение последнего переданого в бою за время лага приказа
			if (ch->last_comm != NULL) {
				command_interpreter(ch, ch->last_comm);
				free(ch->last_comm);
				ch->last_comm = NULL;
			}
			// Set some flag-skills
			// 1) parry
			do_this = number(0, 100);
			sk_use = FALSE;
			if (!sk_use && do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_PARRY)) {
				SET_AF_BATTLE(ch, EAF_PARRY);
				sk_use = TRUE;
			}
			// 2) blocking
			do_this = number(0, 100);
			if (!sk_use && do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_BLOCK)) {
				SET_AF_BATTLE(ch, EAF_BLOCK);
				sk_use = TRUE;
			}
			// 3) multyparry
			do_this = number(0, 100);
			if (!sk_use && do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_MULTYPARRY)) {
				SET_AF_BATTLE(ch, EAF_MULTYPARRY);
				sk_use = TRUE;
			}

			// 4) deviate
			do_this = number(0, 100);
			if (!sk_use && do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_DEVIATE)) {
				SET_AF_BATTLE(ch, EAF_DEVIATE);
				sk_use = TRUE;
			}
			// 5) stupor
			do_this = number(0, 100);
			if (!sk_use && do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_STUPOR)) {
				SET_AF_BATTLE(ch, EAF_STUPOR);
				sk_use = TRUE;
			}
			// 6) mighthit
			do_this = number(0, 100);
			if (!sk_use && do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_STUPOR)) {
				SET_AF_BATTLE(ch, EAF_MIGHTHIT);
				sk_use = TRUE;
			}
			// 7) styles
			do_this = number(0, 100);
			if (do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_AWAKE) > number(1, 101))
				SET_AF_BATTLE(ch, EAF_AWAKE);
			else
				CLR_AF_BATTLE(ch, EAF_AWAKE);
			do_this = number(0, 100);
			if (do_this <= GET_LIKES(ch) && GET_SKILL(ch, SKILL_PUNCTUAL) > number(1, 101))
				SET_AF_BATTLE(ch, EAF_PUNCTUAL);
			else
				CLR_AF_BATTLE(ch, EAF_PUNCTUAL);
		}

		initiative = size_app[GET_POS_SIZE(ch)].initiative;
		if ((i = number(1, 10)) == 10)
			initiative -= 1;
		else
			initiative += i;

		initiative += GET_INITIATIVE(ch);
		if (!IS_NPC(ch))
			switch (IS_CARRYING_W(ch) * 10 / MAX(1, CAN_CARRY_W(ch))) {
			case 10:
			case 9:
			case 8:
				initiative -= 2;
				break;
			case 7:
			case 6:
			case 5:
				initiative -= 1;
				break;
			}

		if (GET_AF_BATTLE(ch, EAF_AWAKE))
			initiative -= 2;
		if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
			initiative -= 1;
		if (AFF_FLAGGED(ch, AFF_SLOW))
			initiative -= 10;
		if (AFF_FLAGGED(ch, AFF_HASTE))
			initiative += 10;
		if (GET_WAIT(ch) > 0)
			initiative -= 1;
		if (calc_leadership(ch))
			initiative += 5;
		if (GET_AF_BATTLE(ch, EAF_SLOW))
			initiative = 1;

		initiative = MAX(initiative, 1);
		INITIATIVE(ch) = initiative;
		SET_AF_BATTLE(ch, EAF_FIRST);
		max_init = MAX(max_init, initiative);
		min_init = MIN(min_init, initiative);
	}

	/* Process fighting           */
	for (initiative = max_init; initiative >= min_init; initiative--)
		for (ch = combat_list; ch; ch = next_combat_list) {
			next_combat_list = ch->next_fighting;
			if (INITIATIVE(ch) != initiative || IN_ROOM(ch) == NOWHERE)
				continue;
			// If mob cast 'hold' when initiative setted
			if (AFF_FLAGGED(ch, AFF_HOLD) ||
			    AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT) || !AWAKE(ch))
				continue;
			// If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
			if (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
				continue;

			if (IS_NPC(ch)) {	// Select extra_attack type
				// if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
				//    continue;

				// Вызываем триггер перед началом боевых моба (магических или физических)
				fight_mtrigger(ch);

				// переключение
				if (MAY_LIKES(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && GET_REAL_INT(ch) > number(15, 25))
					perform_mob_switch(ch);

				// Cast spells
				if (MAY_LIKES(ch))
					mob_casting(ch);
				if (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)) || AFF_FLAGGED(ch, AFF_HOLD) ||	// mob_casting мог от зеркала отразиться
				    AFF_FLAGGED(ch, AFF_STOPFIGHT) || !AWAKE(ch) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT))
					continue;

				if ((AFF_FLAGGED(ch, AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL)) && AFF_FLAGGED(ch, AFF_HELPER) && ch->master &&	// !IS_NPC(ch->master)  &&
				    CAN_SEE(ch, ch->master) &&
				    IN_ROOM(ch) == IN_ROOM(ch->master) && AWAKE(ch) &&
				    MAY_ACT(ch) && GET_POS(ch) >= POS_FIGHTING) {
					for (vict = world[IN_ROOM(ch)]->people; vict; vict = vict->next_in_room)
						if (FIGHTING(vict) == ch->master && vict != ch && vict != ch->master)
							break;
					if (vict && (GET_SKILL(ch, SKILL_RESCUE)	// бред какой-то || GET_REAL_INT(ch) < number(0,100)
					    )) {	//if(GET_MOB_VNUM(ch)==108 && ch->master) 
//                       act("TRYING to RESCUE in FIGHT", TRUE, ch->master, 0, 0, TO_CHAR); 
						go_rescue(ch, ch->master, vict);
					} else if (vict && GET_SKILL(ch, SKILL_PROTECT))
						go_protect(ch, ch->master);
				} else if (!AFF_FLAGGED(ch, AFF_CHARM))
					for (sk_num = 0, sk_use = GET_REAL_INT(ch)
					     /*,sprintf(buf,"{%d}-{%d}\r\n",sk_use,GET_WAIT(ch)) */
					     /*,send_to_char(buf,FIGHTING(ch)) */ ;
					     MAY_LIKES(ch) && sk_use > 0; sk_use--) {
						do_this = number(0, 100);
						if (do_this > GET_LIKES(ch))
							continue;
						do_this = number(0, 100);
						//sprintf(buf,"<%d>\r\n",do_this);
						//send_to_char(buf,FIGHTING(ch));
						if (do_this < 10)
							sk_num = SKILL_BASH;
						else if (do_this < 20)
							sk_num = SKILL_DISARM;
						else if (do_this < 30)
							sk_num = SKILL_KICK;
						else if (do_this < 40)
							sk_num = SKILL_PROTECT;
						else if (do_this < 50)
							sk_num = SKILL_RESCUE;
						else if (do_this < 60 && !TOUCHING(ch))
							sk_num = SKILL_TOUCH;
						else if (do_this < 70)
							sk_num = SKILL_CHOPOFF;
						else
							sk_num = SKILL_BASH;
						if (GET_SKILL(ch, sk_num) <= 0)
							sk_num = 0;
						if (!sk_num)
							continue;
						//else
						//   act("Victim prepare to skill '$F'.",FALSE,FIGHTING(ch),0,skill_name(sk_num),TO_CHAR);
						/* Если умеет метать и вооружен метательным, то должен метнуть */
						if (GET_EQ(ch, WEAR_WIELD))
							if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD), ITEM_THROWING))
								if (GET_REAL_INT(ch) > number(1, 36))
									sk_num = SKILL_THROW;

						if (sk_num == SKILL_TOUCH) {
							sk_use = 0;
							go_touch(ch, FIGHTING(ch));
						}

						if (sk_num == SKILL_THROW) {
							sk_use = 0;
							/* Цель выбираем по рандому */
							for (vict = world[IN_ROOM(ch)]->people, i = 0; vict;
							     vict = vict->next_in_room) {
								if (!IS_NPC(vict))
									i++;
							}
							if (i > 0) {
								caster = NULL;
								i = number(1, i);
								for (vict = world[IN_ROOM(ch)]->people; i;
								     vict = vict->next_in_room) {
									if (!IS_NPC(vict)) {
										i--;
										caster = vict;
									}
								}
							}
							/* Метаем */
							if (caster)
								go_throw(ch, caster);
						}
// проверим на всякий случай, является ли моб ангелом, хотя вроде бы этого делать не надо                   
						if (!(MOB_FLAGGED(ch, MOB_ANGEL) && ch->master)
						    && (sk_num == SKILL_RESCUE || sk_num == SKILL_PROTECT)) {
							CHAR_DATA *attacker;
							int dumb_mob;
							caster = NULL;
							damager = NULL;
							dumb_mob = (int) (GET_REAL_INT(ch) < number(5, 20));
							for (attacker = world[IN_ROOM(ch)]->people;
							     attacker; attacker = attacker->next_in_room) {
								vict = FIGHTING(attacker);	// выяснение жертвы
								if (!vict ||	// жертвы нет
								    (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || AFF_FLAGGED(vict, AFF_HELPER)) ||	// жертва - не моб
								    (IS_NPC(attacker) &&
								     !(AFF_FLAGGED(attacker, AFF_CHARM)
								       && attacker->master && !IS_NPC(attacker->master))
								     && !(MOB_FLAGGED(attacker, MOB_ANGEL)
									  && attacker->master
									  && !IS_NPC(attacker->master))
								     //не совсем понятно, зачем это было && !AFF_FLAGGED(attacker,AFF_HELPER)
								    ) ||	// свои атакуют (мобы)
								    !CAN_SEE(ch, vict) ||	// не видно, кого нужно спасать
								    ch == vict	// себя спасать не нужно
								    )
									continue;

								// Буду спасать vict от attacker
								if (!caster ||	// еще пока никого не спасаю
								    (GET_HIT(vict) < GET_HIT(caster))	// этому мобу хуже
								    ) {
									caster = vict;
									damager = attacker;
									if (dumb_mob)
										break;	// тупой моб спасает первого
								}
							}

							if (sk_num == SKILL_RESCUE && caster && damager) {
								sk_use = 0;
								go_rescue(ch, caster, damager);
							}
							if (sk_num == SKILL_PROTECT && caster) {
								sk_use = 0;
								go_protect(ch, caster);
							}
						}

						if (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF
						    || sk_num == SKILL_DISARM) {
							caster = NULL;
							damager = NULL;
							if (GET_REAL_INT(ch) < number(15, 25)) {
								caster = FIGHTING(ch);
								damager = FIGHTING(ch);
							} else {
								for (vict = world[IN_ROOM(ch)]->people; vict;
								     vict = vict->next_in_room) {
									if ((IS_NPC(vict)
									     && !AFF_FLAGGED(vict, AFF_CHARM))
									    || !FIGHTING(vict))
										continue;
									if ((AFF_FLAGGED(vict, AFF_HOLD)
									     && GET_POS(vict) < POS_FIGHTING)
									    || (IS_CASTER(vict)
										&& (AFF_FLAGGED(vict, AFF_HOLD)
										    || AFF_FLAGGED(vict, AFF_SIELENCE)
										    || GET_WAIT(vict) > 0)))
										continue;
									if (!caster || (IS_CASTER(vict)
											&& GET_CASTER(vict) >
											GET_CASTER(caster)))
										caster = vict;
									if (!damager
									    || GET_DAMAGE(vict) > GET_DAMAGE(damager))
										damager = vict;
								}
							}
							if (caster &&
							    (CAN_SEE(ch, caster) || FIGHTING(ch) == caster) &&
							    GET_CASTER(caster) > POOR_CASTER &&
							    (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF)) {
								if (sk_num == SKILL_BASH) {
									if (GET_POS(caster) >= POS_FIGHTING ||
									    calculate_skill(ch, SKILL_BASH, 200,
											    caster) > number(50, 80)) {
										sk_use = 0;
										go_bash(ch, caster);
									}
								} else {
									if (GET_POS(caster) >= POS_FIGHTING ||
									    calculate_skill(ch, SKILL_CHOPOFF, 200,
											    caster) > number(50, 80)) {
										sk_use = 0;
										go_chopoff(ch, caster);
									}
								}
							}
							if (sk_use &&
							    damager &&
							    (CAN_SEE(ch, damager) || FIGHTING(ch) == damager)) {
								if (sk_num == SKILL_BASH) {
									if (on_horse(damager)) {
										sk_use = 0;
										go_bash(ch, get_horse(damager));
									} else
									    if (GET_POS(damager) >= POS_FIGHTING ||
										calculate_skill(ch, SKILL_BASH, 200,
												damager) > number(50,
														  80)) {
										sk_use = 0;
										go_bash(ch, damager);
									}
								} else if (sk_num == SKILL_CHOPOFF) {
									if (on_horse(damager)) {
										sk_use = 0;
										go_chopoff(ch, get_horse(damager));
									} else
									    if (GET_POS(damager) >= POS_FIGHTING ||
										calculate_skill(ch, SKILL_CHOPOFF, 200,
												damager) > number(50,
														  80)) {
										sk_use = 0;
										go_chopoff(ch, damager);
									}
								} else
								    if (sk_num == SKILL_DISARM &&
									(GET_EQ(damager, WEAR_WIELD) ||
									 GET_EQ(damager, WEAR_BOTHS) ||
									 (GET_EQ(damager, WEAR_HOLD)
/* shapirus: проверку на вепон убираем, т.к. все есть в do_disarm() и go_disarm()
                                  &&
				  GET_OBJ_TYPE (GET_EQ (damager, WEAR_HOLD))
				  == ITEM_WEAPON*/
									 ))) {
									sk_use = 0;
									go_disarm(ch, damager);
								}
							}
						}

						if (sk_num == SKILL_KICK && !on_horse(FIGHTING(ch))) {
							sk_use = 0;
							go_kick(ch, FIGHTING(ch));
						}
					}

				if (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
					continue;

	       /***** удар основным оружием или рукой */
				if (!AFF_FLAGGED(ch, AFF_STOPRIGHT))
					exthit(ch, TYPE_UNDEFINED, 1);

	       /***** экстраатаки */
				for (i = 1; i <= ch->mob_specials.ExtraAttack; i++) {
					if (AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
					    AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT) ||
					    (i == 1 && AFF_FLAGGED(ch, AFF_STOPLEFT)))
						continue;
					exthit(ch, TYPE_UNDEFINED, i + 1);
				}
			} else {	/* PLAYERS - only one hit per round */

				if (GET_POS(ch) > POS_STUNNED &&
				    GET_POS(ch) < POS_FIGHTING && GET_AF_BATTLE(ch, EAF_STAND)) {
					sprintf(buf, "%sВам лучше встать на ноги !%s\r\n",
						CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
					send_to_char(buf, ch);
					CLR_AF_BATTLE(ch, EAF_STAND);
				}

				if (GET_CAST_SPELL(ch) && GET_WAIT(ch) <= 0) {
					if (AFF_FLAGGED(ch, AFF_SIELENCE))
						send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
					else {
						cast_spell(ch, GET_CAST_CHAR(ch), GET_CAST_OBJ(ch), 0, GET_CAST_SPELL(ch));
						if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)
						      || CHECK_WAIT(ch)))
							WAIT_STATE(ch, PULSE_VIOLENCE);
						SET_CAST(ch, 0, NULL, NULL, NULL);
					}
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}

				if (GET_AF_BATTLE(ch, EAF_MULTYPARRY))
					continue;

				if (GET_EXTRA_SKILL(ch) == SKILL_THROW && GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0) {
					go_throw(ch, GET_EXTRA_VICTIM(ch));
					SET_EXTRA(ch, 0, NULL);
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}


				if (GET_EXTRA_SKILL(ch) == SKILL_BASH && GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0) {
					go_bash(ch, GET_EXTRA_VICTIM(ch));
					SET_EXTRA(ch, 0, NULL);
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}

				if (GET_EXTRA_SKILL(ch) == SKILL_KICK && GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0) {
					go_kick(ch, GET_EXTRA_VICTIM(ch));
					SET_EXTRA(ch, 0, NULL);
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}

				if (GET_EXTRA_SKILL(ch) == SKILL_CHOPOFF && GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0) {
					go_chopoff(ch, GET_EXTRA_VICTIM(ch));
					SET_EXTRA(ch, 0, NULL);
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}

				if (GET_EXTRA_SKILL(ch) == SKILL_DISARM && GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0) {
					go_disarm(ch, GET_EXTRA_VICTIM(ch));
					SET_EXTRA(ch, 0, NULL);
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}

				if (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
					continue;
	       /***** удар основным оружием или рукой */
				if (GET_AF_BATTLE(ch, EAF_FIRST)) {
					if (!AFF_FLAGGED(ch, AFF_STOPRIGHT) &&
					    (IS_IMMORTAL(ch) ||
					     GET_GOD_FLAG(ch, GF_GODSLIKE) || !GET_AF_BATTLE(ch, EAF_USEDRIGHT)))
						exthit(ch, TYPE_UNDEFINED, 1);
					CLR_AF_BATTLE(ch, EAF_FIRST);
					SET_AF_BATTLE(ch, EAF_SECOND);
					if (INITIATIVE(ch) > min_init) {
						INITIATIVE(ch)--;
						continue;
					}
				}

	       /***** удар вторым оружием если оно есть и умение позволяет */
				if (GET_EQ(ch, WEAR_HOLD) &&
				    GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON &&
				    GET_AF_BATTLE(ch, EAF_SECOND) &&
				    !AFF_FLAGGED(ch, AFF_STOPLEFT) &&
				    (IS_IMMORTAL(ch) ||
				     GET_GOD_FLAG(ch, GF_GODSLIKE) || GET_SKILL(ch, SKILL_SATTACK) > number(1, 101))) {
					if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) ||
					    !GET_AF_BATTLE(ch, EAF_USEDLEFT))
						exthit(ch, TYPE_UNDEFINED, 2);
					CLR_AF_BATTLE(ch, EAF_SECOND);
				} else
	       /***** удар второй рукой если она свободна и умение позволяет */
				if (!GET_EQ(ch, WEAR_HOLD) && !GET_EQ(ch, WEAR_LIGHT) &&
					    !GET_EQ(ch, WEAR_SHIELD) && !GET_EQ(ch, WEAR_BOTHS) &&
					    !AFF_FLAGGED(ch, AFF_STOPLEFT) &&
					    GET_AF_BATTLE(ch, EAF_SECOND) && GET_SKILL(ch, SKILL_SHIT)) {
					if (IS_IMMORTAL(ch) || !GET_AF_BATTLE(ch, EAF_USEDLEFT))
						exthit(ch, TYPE_UNDEFINED, 2);
					CLR_AF_BATTLE(ch, EAF_SECOND);
				}

// немного коряво, т.к. зависит от инициативы кастера
// check if angel is in fight, and go_rescue if it is not
				for (k = ch->followers; k; k = k_next) {
					k_next = k->next;
					if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
					    MOB_FLAGGED(k->follower, MOB_ANGEL) &&
					    !FIGHTING(k->follower) &&
					    IN_ROOM(k->follower) == IN_ROOM(ch) &&
					    CAN_SEE(k->follower, ch) && AWAKE(k->follower) &&
					    MAY_ACT(k->follower) && GET_POS(k->follower) >= POS_FIGHTING) {
						for (vict = world[IN_ROOM(ch)]->people; vict; vict = vict->next_in_room)
							if (FIGHTING(vict) == ch && vict != ch && vict != k->follower)
								break;
						if (vict && GET_SKILL(k->follower, SKILL_RESCUE)) {
//                if(GET_MOB_VNUM(k->follower)==108) 
//                  act("TRYING to RESCUE without FIGHTING", TRUE, ch, 0, 0, TO_CHAR);
							go_rescue(k->follower, ch, vict);
						}
					}
				}




			}
		}

	/* Decrement mobs lag */
	for (ch = combat_list; ch; ch = ch->next_fighting) {
		if (IN_ROOM(ch) == NOWHERE)
			continue;

		CLR_AF_BATTLE(ch, EAF_FIRST);
		CLR_AF_BATTLE(ch, EAF_SECOND);
		CLR_AF_BATTLE(ch, EAF_USEDLEFT);
		CLR_AF_BATTLE(ch, EAF_USEDRIGHT);
		CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
		if (GET_AF_BATTLE(ch, EAF_SLEEP))
			affect_from_char(ch, SPELL_SLEEP);
		if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
			CLR_AF_BATTLE(ch, EAF_BLOCK);
			if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
		if (GET_AF_BATTLE(ch, EAF_DEVIATE)) {
			CLR_AF_BATTLE(ch, EAF_DEVIATE);
			if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
		battle_affect_update(ch);
	}
}

// returns 1 if only ch was outcasted
// returns 2 if only victim was outcasted
// returns 4 if both were outcasted
// returns 0 if none was outcasted  
int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim)
{
	CHAR_DATA *cleader, *vleader;
	int return_value = 0;
	if (ch == victim)
		return return_value;
// translating pointers from charimces to their leaders
	if (IS_NPC(ch) && ch->master && (AFF_FLAGGED(ch, AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL) || IS_HORSE(ch)))
		ch = ch->master;
	if (IS_NPC(victim) && victim->master &&
	    (AFF_FLAGGED(victim, AFF_CHARM) || MOB_FLAGGED(victim, MOB_ANGEL) || IS_HORSE(victim)))
		victim = victim->master;
	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->master) {
		if (IS_NPC(cleader) &&
		    !AFF_FLAGGED(cleader, AFF_CHARM) && !MOB_FLAGGED(cleader, MOB_ANGEL) && !IS_HORSE(cleader))
			break;
		cleader = cleader->master;
	}
	while (vleader->master) {
		if (IS_NPC(vleader) &&
		    !AFF_FLAGGED(vleader, AFF_CHARM) && !MOB_FLAGGED(vleader, MOB_ANGEL) && !IS_HORSE(vleader))
			break;
		vleader = vleader->master;
	}
	if (cleader != vleader)
		return return_value;


// finding closest to the leader nongrouped agressor
// it cannot be a charmice
	while (ch->master && ch->master->master) {
		if (!AFF_FLAGGED(ch->master, AFF_GROUP) && !IS_NPC(ch->master)) {
			ch = ch->master;
			continue;
		} else if (IS_NPC(ch->master)
			   && !AFF_FLAGGED(ch->master->master, AFF_GROUP)
			   && !IS_NPC(ch->master->master) && ch->master->master->master) {
			ch = ch->master->master;
			continue;
		} else
			break;
	}

// finding closest to the leader nongrouped victim
// it cannot be a charmice
	while (victim->master && victim->master->master) {
		if (!AFF_FLAGGED(victim->master, AFF_GROUP)
		    && !IS_NPC(victim->master)) {
			victim = victim->master;
			continue;
		} else if (IS_NPC(victim->master)
			   && !AFF_FLAGGED(victim->master->master, AFF_GROUP)
			   && !IS_NPC(victim->master->master)
			   && victim->master->master->master) {
			victim = victim->master->master;
			continue;
		} else
			break;
	}
	if (!AFF_FLAGGED(ch, AFF_GROUP) || cleader == victim) {
		stop_follower(ch, SF_EMPTY);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, AFF_GROUP) || vleader == ch) {
		stop_follower(victim, SF_EMPTY);
		return_value |= 2;
	}
	return return_value;
}
