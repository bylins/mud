/* ************************************************************************
*   File: act.informative.cpp                           Part of Bylins    *
*  Usage: Player-level commands of an informative nature                  *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"

#include "spells.h"
#include "skills.h"
#include "fight.h"

#include "screen.h"
#include "constants.h"
#include "pk.h"
#include "dg_scripts.h"
#include "privileges.hpp"
#include "mail.h"

#include <string>
using std::string;

/* extern variables */
extern int
 top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *character_list;
extern OBJ_DATA *object_list;
extern OBJ_DATA *obj_proto;
extern int
 top_of_socialk;
extern char *credits;
extern char *news;
extern char *godnews;
extern char *info;
extern char *motd;
extern char *rules;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *class_abbrevs[];
extern char *kin_abbrevs[];
extern INDEX_DATA *obj_index;
extern INDEX_DATA *mob_index;
extern PrivList *priv;

extern const char *material_name[];

/* extern functions */
long find_class_bitvector(char arg);
int level_exp(CHAR_DATA * ch, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(CHAR_DATA * ch);
char *str_str(char *cs, char *ct);
int low_charm(CHAR_DATA * ch);
int pk_count(CHAR_DATA * ch);
/* local functions */
void print_object_location(int num, OBJ_DATA * obj, CHAR_DATA * ch, int recur);
void show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how);
void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show);
char *diag_obj_to_char(CHAR_DATA * i, OBJ_DATA * obj, int mode);
char *diag_timer_to_char(OBJ_DATA * obj);

ACMD(do_affects);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_who_new);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(CHAR_DATA * ch, char *arg);
void perform_immort_where(CHAR_DATA * ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
ACMD(do_looking);
ACMD(do_hearing);
ACMD(do_sides);
void diag_char_to_char(CHAR_DATA * i, CHAR_DATA * ch);
void look_at_char(CHAR_DATA * i, CHAR_DATA * ch);
void list_one_char(CHAR_DATA * i, CHAR_DATA * ch, int skill_mode);
void list_char_to_char(CHAR_DATA * list, CHAR_DATA * ch);
void do_auto_exits(CHAR_DATA * ch);
ACMD(do_exits);
void look_in_direction(CHAR_DATA * ch, int dir, int info_is);
void look_in_obj(CHAR_DATA * ch, char *arg);
char *find_exdesc(char *word, EXTRA_DESCR_DATA * list);
void look_at_target(CHAR_DATA * ch, char *arg, int subcmd);
void gods_day_now(CHAR_DATA * ch);
#define EXIT_SHOW_WALL    (1 << 0)
#define EXIT_SHOW_LOOKING (1 << 1)
/*
 * This function screams bitvector... -gg 6/45/98
 */

int param_sort = 0;

const char *Dirs[NUM_OF_DIRS + 1] = { "Север",
	"Восток",
	"Юг",
	"Запад",
	"Верх",
	"Низ",
	"\n"
};

const char *ObjState[8][2] = { {"рассыпается", "рассыпается"},
{"плачевно", "в плачевном состоянии"},
{"плохо", "в плохом состоянии"},
{"неплохо", "в неплохом состоянии"},
{"средне", "в рабочем состоянии"},
{"хорошо", "в хорошем состоянии"},
{"очень хорошо", "в очень хорошем состоянии"},
{"великолепно", "в великолепном состоянии"}
};

char *diag_obj_to_char(CHAR_DATA * i, OBJ_DATA * obj, int mode)
{
	static char out_str[80] = "\0";
	const char *color;
	int percent;

	if (GET_OBJ_MAX(obj) > 0)
		percent = 100 * GET_OBJ_CUR(obj) / GET_OBJ_MAX(obj);
	else
		percent = -1;

	if (percent >= 100) {
		percent = 7;
		color = CCWHT(i, C_NRM);
	} else if (percent >= 90) {
		percent = 6;
		color = CCIGRN(i, C_NRM);
	} else if (percent >= 75) {
		percent = 5;
		color = CCGRN(i, C_NRM);
	} else if (percent >= 50) {
		percent = 4;
		color = CCIYEL(i, C_NRM);
	} else if (percent >= 30) {
		percent = 3;
		color = CCIRED(i, C_NRM);
	} else if (percent >= 15) {
		percent = 2;
		color = CCRED(i, C_NRM);
	} else if (percent > 0) {
		percent = 1;
		color = CCNRM(i, C_NRM);
	} else {
		percent = 0;
		color = CCINRM(i, C_NRM);
	}

	if (mode == 1)
		sprintf(out_str, " %s<%s>%s", color, ObjState[percent][0], CCNRM(i, C_NRM));
	else if (mode == 2)
		strcpy(out_str, ObjState[percent][1]);
	return out_str;
}


const char *weapon_class[] = { "луки",
	"короткие лезвия",
	"длинные лезвия",
	"секиры",
	"палицы и дубины",
	"иное оружие",
	"двуручники",
	"проникающее оружие",
	"копья и рогатины"
};

char *diag_weapon_to_char(OBJ_DATA * obj, int show_wear)
{
	static char out_str[MAX_STRING_LENGTH];
	int skill = 0;

	*out_str = '\0';
	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_WEAPON:
		switch (GET_OBJ_SKILL(obj)) {
		case SKILL_BOWS:
			skill = 1;
			break;
		case SKILL_SHORTS:
			skill = 2;
			break;
		case SKILL_LONGS:
			skill = 3;
			break;
		case SKILL_AXES:
			skill = 4;
			break;
		case SKILL_CLUBS:
			skill = 5;
			break;
		case SKILL_NONSTANDART:
			skill = 6;
			break;
		case SKILL_BOTHHANDS:
			skill = 7;
			break;
		case SKILL_PICK:
			skill = 8;
			break;
		case SKILL_SPADES:
			skill = 9;
			break;
		default:
			sprintf(out_str, "!! Не принадлежит к известным типам оружия - сообщите Богам !!\r\n");
		}
		if (skill)
			sprintf(out_str, "Принадлежит к классу \"%s\".\r\n", weapon_class[skill - 1]);
	default:
		if (show_wear) {
			if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
				sprintf(out_str + strlen(out_str), "Можно одеть на палец.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_NECK))
				sprintf(out_str + strlen(out_str), "Можно одеть на шею.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_BODY))
				sprintf(out_str + strlen(out_str), "Можно одеть на туловище.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
				sprintf(out_str + strlen(out_str), "Можно одеть на голову.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
				sprintf(out_str + strlen(out_str), "Можно одеть на ноги.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_FEET))
				sprintf(out_str + strlen(out_str), "Можно обуть.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
				sprintf(out_str + strlen(out_str), "Можно одеть на кисти.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
				sprintf(out_str + strlen(out_str), "Можно одеть на руки.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
				sprintf(out_str + strlen(out_str), "Можно использовать как щит.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
				sprintf(out_str + strlen(out_str), "Можно одеть на плечи.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
				sprintf(out_str + strlen(out_str), "Можно одеть на пояс.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
				sprintf(out_str + strlen(out_str), "Можно одеть на запястья.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
				sprintf(out_str + strlen(out_str), "Можно взять в правую руку.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
				sprintf(out_str + strlen(out_str), "Можно взять в левую руку.\r\n");
			if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
				sprintf(out_str + strlen(out_str), "Можно взять в обе руки.\r\n");
		}
	}
	return (out_str);
}

char *diag_timer_to_char(OBJ_DATA * obj)
{
	static char out_str[MAX_STRING_LENGTH];

	*out_str = 0;
	if (GET_OBJ_RNUM(obj) != NOTHING) {
		int tm = (GET_OBJ_TIMER(obj) * 100 / GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(obj)));
		if (tm < 20)
			sprintf(out_str, "Состояние: ужасно.\r\n");
		else if (tm < 40)
			sprintf(out_str, "Состояние: скоро испортится.\r\n");
		else if (tm < 60)
			sprintf(out_str, "Состояние: плоховато.\r\n");
		else if (tm < 80)
			sprintf(out_str, "Состояние: средне.\r\n");
		else
			sprintf(out_str, "Состояние: идеально.\r\n");
	}
	return (out_str);
}


char *diag_uses_to_char(OBJ_DATA * obj, CHAR_DATA * ch)
{
	static char out_str[MAX_STRING_LENGTH];

	*out_str = 0;
	if (GET_OBJ_TYPE(obj) == ITEM_INGRADIENT && IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES)
	    && GET_CLASS(ch) == CLASS_DRUID) {
		sprintf(out_str, "Оcталось применений: %s%d&n.\r\n",
			GET_OBJ_VAL(obj, 2) > 100 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}


void show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how)
{
	*buf = '\0';
	if ((mode < 5) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
		sprintf(buf, "[%5d] ", GET_OBJ_VNUM(object));

	if ((mode == 0) && object->description)
		strcat(buf, object->description);
	else if (object->short_description && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4)))
		strcat(buf, object->short_description);
	else if (mode == 5) {
		if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
			if (object->action_description) {
				strcpy(buf, "Вы прочитали следующее :\r\n\r\n");
				strcat(buf, object->action_description);
				page_string(ch->desc, buf, 1);
			} else
				send_to_char("Чисто.\r\n", ch);
			return;
		} else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
			strcpy(buf, "Вы не видите ничего необычного.");
		} else		/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
			strcpy(buf, "Это емкость для жидкости.");
	}

	if (show_state) {
		*buf2 = '\0';
		if (mode == 1 && how <= 1) {
			if (GET_OBJ_TYPE(object) == ITEM_LIGHT) {
				if (GET_OBJ_VAL(object, 2) == -1)
					strcpy(buf2, " (вечный свет)");
				else if (GET_OBJ_VAL(object, 2) == 0)
					sprintf(buf2, " (погас%s)", GET_OBJ_SUF_4(object));
				else
					sprintf(buf2, " (%d %s)",
						GET_OBJ_VAL(object, 2), desc_count(GET_OBJ_VAL(object, 2), WHAT_HOUR));
			} else
				// if (GET_OBJ_CUR(object) < GET_OBJ_MAX(object))
				sprintf(buf2, " %s", diag_obj_to_char(ch, object, 1));
			if (GET_OBJ_TYPE(object) == ITEM_CONTAINER) {
				if (object->contains)
					strcat(buf2, " (есть содержимое)");
				else
					sprintf(buf2 + strlen(buf2), " (пуст%s)", GET_OBJ_SUF_6(object));
			}
		} else if (mode >= 2 && how <= 1) {
			std::string obj_name = OBJN(object, ch, 0);
			obj_name[0] = UPPER(obj_name[0]);
			if (GET_OBJ_TYPE(object) == ITEM_LIGHT) {
				if (GET_OBJ_VAL(object, 2) == -1)
					sprintf(buf2, "\r\n%s дает вечный свет.", obj_name.c_str());
				else if (GET_OBJ_VAL(object, 2) == 0)
					sprintf(buf2, "\r\n%s погас%s.", obj_name.c_str(), GET_OBJ_SUF_4(object));
				else
					sprintf(buf2, "\r\n%s будет светить %d %s.", obj_name.c_str(), GET_OBJ_VAL(object, 2),
						desc_count(GET_OBJ_VAL(object, 2), WHAT_HOUR));
			} else if (GET_OBJ_CUR(object) < GET_OBJ_MAX(object))
				sprintf(buf2, "\r\n%s %s.", obj_name.c_str(), diag_obj_to_char(ch, object, 2));
		}
		strcat(buf, buf2);
	}
	if (how > 1)
		sprintf(buf + strlen(buf), " [%d]", how);
	if (mode != 3 && how <= 1) {
		if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
			sprintf(buf2, " (невидим%s)", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (IS_OBJ_STAT(object, ITEM_BLESS)
		    && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
			strcat(buf, " ..голубая аура !");
		if (IS_OBJ_STAT(object, ITEM_MAGIC)
		    && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
			strcat(buf, " ..желтая аура !");
		if (IS_OBJ_STAT(object, ITEM_POISONED)
		    && AFF_FLAGGED(ch, AFF_DETECT_POISON)) {
			sprintf(buf2, "..отравлен%s !", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (IS_OBJ_STAT(object, ITEM_GLOW))
			strcat(buf, " ..блестит !");
		if (IS_OBJ_STAT(object, ITEM_HUM) && !AFF_FLAGGED(ch, AFF_SIELENCE))
			strcat(buf, " ..шумит !");
		if (IS_OBJ_STAT(object, ITEM_FIRE))
			strcat(buf, " ..горит !");
	}
	strcat(buf, "\r\n");
	if (mode >= 5) {
		strcat(buf, diag_weapon_to_char(object, TRUE));
		strcat(buf, diag_timer_to_char(object));
		strcat(buf, diag_uses_to_char(object, ch));
	}
	page_string(ch->desc, buf, TRUE);
}


void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show)
{
	OBJ_DATA *i, *push = NULL;
	bool found = FALSE;
	int push_count = 0;

	for (i = list; i; i = i->next_content) {
		if (CAN_SEE_OBJ(ch, i)) {
			if (!push) {
				push = i;
				push_count = 1;
			} else
			    if (GET_OBJ_VNUM(i) != GET_OBJ_VNUM(push) ||
				GET_OBJ_TYPE(i) == ITEM_MING ||
				(GET_OBJ_TYPE(i) == ITEM_DRINKCON &&
				 GET_OBJ_VAL(i, 2) != GET_OBJ_VAL(push, 2)) ||
				(GET_OBJ_TYPE(i) == ITEM_CONTAINER &&
				 i->contains && !push->contains) || GET_OBJ_VNUM(push) == -1) {
				show_obj_to_char(push, ch, mode, show, push_count);
				push = i;
				push_count = 1;
			} else
				push_count++;
			found = TRUE;
		}
	}
	if (push && push_count)
		show_obj_to_char(push, ch, mode, show, push_count);
	if (!found && show) {
		if (show == 1)
			send_to_char(" Внутри ничего нет.\r\n", ch);
		else
			send_to_char(" Вы ничего не несете.\r\n", ch);
	}
}


void diag_char_to_char(CHAR_DATA * i, CHAR_DATA * ch)
{
	int percent;

	if (GET_REAL_MAX_HIT(i) > 0)
		percent = (100 * GET_HIT(i)) / GET_REAL_MAX_HIT(i);
	else
		percent = -1;	/* How could MAX_HIT be < 1?? */

	if (percent >= 100)
		return;

	strcpy(buf, PERS(i, ch, 0));
	CAP(buf);

	if (percent >= 90) {
		sprintf(buf2, " слегка поцарапан%s.", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 75) {
		sprintf(buf2, " легко ранен%s.", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 50) {
		sprintf(buf2, " ранен%s.", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 30) {
		sprintf(buf2, " тяжело ранен%s.", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 15) {
		sprintf(buf2, " смертельно ранен%s.", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 0)
		strcat(buf, " в ужасном состоянии.");
	else
		strcat(buf, " умирает.");

	if (AFF_FLAGGED(ch, AFF_DETECT_POISON))
		if (AFF_FLAGGED(i, AFF_POISON)) {
			sprintf(buf2, " (отравлен%s)", GET_CH_SUF_6(i));
			strcat(buf, buf2);
		}
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}


void look_at_char(CHAR_DATA * i, CHAR_DATA * ch)
{
	int j, found, push_count = 0;
	OBJ_DATA *tmp_obj, *push = NULL;

	if (!ch->desc)
		return;

	if (i->player.description && *i->player.description) {
		send_to_char(" * ", ch);
		send_to_char(i->player.description, ch);
	} else if (!IS_NPC(i)) {
		strcpy(buf, "\r\nЭто");
		if (IS_FEMALE(i)) {
			if (GET_HEIGHT(i) <= 151) {
				if (GET_WEIGHT(i) >= 140)
					strcat(buf, " маленькая плотная дамочка.\r\n");
				else if (GET_WEIGHT(i) >= 125)
					strcat(buf, " маленькая женщина.\r\n");
				else
					strcat(buf, " миниатюрная дамочка.\r\n");
			} else if (GET_HEIGHT(i) <= 159) {
				if (GET_WEIGHT(i) >= 145)
					strcat(buf, " невысокая плотная мадам.\r\n");
				else if (GET_WEIGHT(i) >= 130)
					strcat(buf, " невысокая женщина.\r\n");
				else
					strcat(buf, " изящная леди.\r\n");
			} else if (GET_HEIGHT(i) <= 165) {
				if (GET_WEIGHT(i) >= 145)
					strcat(buf, " среднего роста женщина.\r\n");
				else
					strcat(buf, " среднего роста изящная красавица.\r\n");
			} else if (GET_HEIGHT(i) <= 175) {
				if (GET_WEIGHT(i) >= 150)
					strcat(buf, " высокая дородная баба.\r\n");
				else if (GET_WEIGHT(i) >= 135)
					strcat(buf, " высокая стройная женщина.\r\n");
				else
					strcat(buf, " высокая изящная женщина.\r\n");
			} else {
				if (GET_WEIGHT(i) >= 155)
					strcat(buf, " очень высокая крупная дама.\r\n");
				else if (GET_WEIGHT(i) >= 140)
					strcat(buf, " очень высокая стройная женщина.\r\n");
				else
					strcat(buf, " очень высокая худощавая");
			}
		} else {
			if (GET_HEIGHT(i) <= 165) {
				if (GET_WEIGHT(i) >= 170)
					strcat(buf, " маленький, похожий на колобок, мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 150)
					strcat(buf, " маленький плотный мужчина.\r\n");
				else
					strcat(buf, " маленький плюгавенький мужичонка.\r\n");
			} else if (GET_HEIGHT(i) <= 175) {
				if (GET_WEIGHT(i) >= 175)
					strcat(buf, " невысокий коренастый крепыш.\r\n");
				else if (GET_WEIGHT(i) >= 160)
					strcat(buf, " невысокий крепкий мужчина.\r\n");
				else
					strcat(buf, " невысокий худощавый мужчина.\r\n");
			} else if (GET_HEIGHT(i) <= 185) {
				if (GET_WEIGHT(i) >= 180)
					strcat(buf, " среднего роста коренастый мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 165)
					strcat(buf, " среднего роста крепкий мужчина.\r\n");
				else
					strcat(buf, " среднего роста худощавый мужчина.\r\n");
			} else if (GET_HEIGHT(i) <= 195) {
				if (GET_WEIGHT(i) >= 185)
					strcat(buf, " высокий крупный мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 170)
					strcat(buf, " высокий стройный мужчина.\r\n");
				else
					strcat(buf, " длинный, худощавый мужчина.\r\n");
			} else {
				if (GET_WEIGHT(i) >= 190)
					strcat(buf, " огромный мужик.\r\n");
				else if (GET_WEIGHT(i) >= 180)
					strcat(buf, " очень высокий, крупный амбал.");
				else
					strcat(buf, " длиннющий, похожий на жердь мужчина.\r\n");
			}
		}
		send_to_char(buf, ch);
	} else
		act("\r\nНичего необычного в $n5 Вы не заметили.", FALSE, i, 0, ch, TO_VICT);

	if (AFF_FLAGGED(i, AFF_CHARM) && i->master == ch && low_charm(i))
		act("$n скоро перестанет следовать за Вами.", FALSE, i, 0, ch, TO_VICT);

	if (IS_HORSE(i) && i->master == ch) {
		strcpy(buf, "\r\nЭто Ваш скакун. Он ");
		if (GET_HORSESTATE(i) <= 0)
			strcat(buf, "загнан.");
		else if (GET_HORSESTATE(i) <= 20)
			strcat(buf, "весь в мыле.");
		else if (GET_HORSESTATE(i) <= 80)
			strcat(buf, "в хорошем состоянии.");
		else
			strcat(buf, "выглядит совсем свежим.");
		send_to_char(buf, ch);
	};

	diag_char_to_char(i, ch);

	found = FALSE;
	for (j = 0; !found && j < NUM_WEARS; j++)
		if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
			found = TRUE;

	if (found) {
		send_to_char("\r\n", ch);
		act("$n одет$a :", FALSE, i, 0, ch, TO_VICT);
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
				send_to_char(where[j], ch);
				if (i->master && IS_NPC(i))
					show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i->master, 1);
				else
					show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i, 1);
			}
	}

	if (ch != i && (GET_SKILL(ch, SKILL_LOOK_HIDE) || IS_IMMORTAL(ch))) {
		found = FALSE;
		act("\r\nВы попытались заглянуть в $s ношу:", FALSE, i, 0, ch, TO_VICT);
		for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
			if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 30) < GET_LEVEL(ch))) {
				if (!push) {
					push = tmp_obj;
					push_count = 1;
				} else if (GET_OBJ_VNUM(push) != GET_OBJ_VNUM(tmp_obj)
					   || GET_OBJ_VNUM(push) == -1) {
					show_obj_to_char(push, ch, 1, ch == i, push_count);
					push = tmp_obj;
					push_count = 1;
				} else
					push_count++;
				found = TRUE;
			}
		}
		if (push && push_count)
			show_obj_to_char(push, ch, 1, ch == i, push_count);
		if (!found)
			send_to_char("...и ничего не обнаружили.\r\n", ch);
	}
}


void list_one_char(CHAR_DATA * i, CHAR_DATA * ch, int skill_mode)
{
	int sector = SECT_CITY;
	int n;
	char aura_txt[200];
	const char *positions[] = { "лежит здесь, мертвый. ",
		"лежит здесь, при смерти. ",
		"лежит здесь, без сознания. ",
		"лежит здесь, в обмороке. ",
		"спит здесь. ",
		"отдыхает здесь. ",
		"сидит здесь. ",
		"СРАЖАЕТСЯ! ",
		"стоит здесь. "
	};

	if (IS_HORSE(i) && on_horse(i->master)) {
		if (ch == i->master) {
			act("$N несет Вас на своей спине.", FALSE, ch, 0, i, TO_CHAR);
		}
		return;
	}

	if (skill_mode == SKILL_LOOKING) {
		if (HERE(i) && INVIS_OK(ch, i) && GET_REAL_LEVEL(ch) >= (IS_NPC(i) ? 0 : GET_INVIS_LEV(i))) {
			sprintf(buf, "Вы разглядели %s.\r\n", GET_PAD(i, 3));
			send_to_char(buf, ch);
		}
		return;
	}

	if (!CAN_SEE(ch, i)) {
		skill_mode =
		    check_awake(i, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
		*buf = 0;
		if (IS_SET(skill_mode, ACHECK_AFFECTS)) {
			REMOVE_BIT(skill_mode, ACHECK_AFFECTS);
			sprintf(buf + strlen(buf), "магический ореол%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_LIGHT)) {
			REMOVE_BIT(skill_mode, ACHECK_LIGHT);
			sprintf(buf + strlen(buf), "яркий свет%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_GLOWING)
		    && IS_SET(skill_mode, ACHECK_HUMMING)
		    && !AFF_FLAGGED(ch, AFF_SIELENCE)) {
			REMOVE_BIT(skill_mode, ACHECK_GLOWING);
			REMOVE_BIT(skill_mode, ACHECK_HUMMING);
			sprintf(buf + strlen(buf), "шум и блеск экипировки%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_GLOWING)) {
			REMOVE_BIT(skill_mode, ACHECK_GLOWING);
			sprintf(buf + strlen(buf), "блеск экипировки%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_HUMMING)
		    && !AFF_FLAGGED(ch, AFF_SIELENCE)) {
			REMOVE_BIT(skill_mode, ACHECK_HUMMING);
			sprintf(buf + strlen(buf), "шум экипировки%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_WEIGHT)
		    && !AFF_FLAGGED(ch, AFF_SIELENCE)) {
			REMOVE_BIT(skill_mode, ACHECK_WEIGHT);
			sprintf(buf + strlen(buf), "бряцание металла%s", skill_mode ? ", " : " ");
		}
		strcat(buf, "выдает чье-то присутствие.\r\n");
		send_to_char(CAP(buf), ch);
		return;
	}

	if (IS_NPC(i) &&
	    i->player.long_descr &&
	    GET_POS(i) == GET_DEFAULT_POS(i) &&
	    IN_ROOM(ch) == IN_ROOM(i) && !AFF_FLAGGED(i, AFF_CHARM) && !IS_HORSE(i)) {
		*buf = '\0';
		if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
			sprintf(buf, "[%5d] ", GET_MOB_VNUM(i));

		if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
		    && !AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
			if (AFF_FLAGGED(i, AFF_EVILESS))
				strcat(buf, "(черная аура) ");
		}
		if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
			if (IS_NPC(i)) {

				if (NPC_FLAGGED(i, NPC_AIRCREATURE))
					sprintf(buf + strlen(buf), "%s(аура воздуха)%s ",
						CCIBLU(ch, C_CMP), CCIRED(ch, C_CMP));
				else if (NPC_FLAGGED(i, NPC_WATERCREATURE))
					sprintf(buf + strlen(buf), "%s(аура воды)%s ",
						CCICYN(ch, C_CMP), CCIRED(ch, C_CMP));
				else if (NPC_FLAGGED(i, NPC_FIRECREATURE))
					sprintf(buf + strlen(buf), "%s(аура огня)%s ",
						CCIMAG(ch, C_CMP), CCIRED(ch, C_CMP));
				else if (NPC_FLAGGED(i, NPC_EARTHCREATURE))
					sprintf(buf + strlen(buf), "%s(аура земли)%s ",
						CCIGRN(ch, C_CMP), CCIRED(ch, C_CMP));
			}
		}
		if (AFF_FLAGGED(i, AFF_INVISIBLE))
			sprintf(buf + strlen(buf), "(невидим%s) ", GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, AFF_HIDE))
			sprintf(buf + strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, AFF_CAMOUFLAGE))
			sprintf(buf + strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, AFF_FLY))
			strcat(buf, "(летит) ");
		if (AFF_FLAGGED(i, AFF_HORSE))
			strcat(buf, "(под седлом) ");

		strcat(buf, i->player.long_descr);
		send_to_char(buf, ch);

		*aura_txt = '\0';
		if (AFF_FLAGGED(i, AFF_SHIELD)) {
			strcat(aura_txt, "...окутан");
			strcat(aura_txt, GET_CH_SUF_6(i));
			strcat(aura_txt, " сверкающим коконом ");
		}
		if (AFF_FLAGGED(i, AFF_SANCTUARY))
			strcat(aura_txt, "...светится ярким сиянием ");
		else if (AFF_FLAGGED(i, AFF_PRISMATICAURA))
			strcat(aura_txt, "...переливается всеми цветами ");
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, "...окружен");
		strcat(aura_txt, GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, AFF_AIRSHIELD)) {
			strcat(aura_txt, " воздушным");
			n++;
		}
		if (AFF_FLAGGED(i, AFF_FIRESHIELD)) {
			if (n > 0)
				strcat(aura_txt, ", огненным");
			else
				strcat(aura_txt, " огненным");
			n++;
		}
		if (AFF_FLAGGED(i, AFF_ICESHIELD)) {
			if (n > 0)
				strcat(aura_txt, ", ледяным");
			else
				strcat(aura_txt, " ледяным");
			n++;
		}
		if (n == 1)
			strcat(aura_txt, " щитом ");
		else if (n > 1)
			strcat(aura_txt, " щитами ");
		if (n > 0)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
			*aura_txt = '\0';
			n = 0;
			strcat(aura_txt, "...");
			if (AFF_FLAGGED(i, AFF_AIRAURA)) {
				strcat(aura_txt, "воздушная");
				n++;
			}
			if (AFF_FLAGGED(i, AFF_FIREAURA)) {
				if (n > 0)
					strcat(aura_txt, ", огненная");
				else
					strcat(aura_txt, "огненная");
				n++;
			}
			if (AFF_FLAGGED(i, AFF_ICEAURA)) {
				if (n > 0)
					strcat(aura_txt, ", ледяная");
				else
					strcat(aura_txt, "ледяная");
				n++;
			}
			if (AFF_FLAGGED(i, AFF_MAGICGLASS)) {
				if (n > 0)
					strcat(aura_txt, ", серебристая");
				else
					strcat(aura_txt, "серебристая");
				n++;
			}
			if (AFF_FLAGGED(i, AFF_EVILESS)) {
				if (n > 0)
					strcat(aura_txt, ", черная");
				else
					strcat(aura_txt, "черная");
				n++;
			}
			if (n == 1)
				strcat(aura_txt, " аура ");
			else if (n > 1)
				strcat(aura_txt, " ауры ");

			if (n > 0)
				act(aura_txt, FALSE, i, 0, ch, TO_VICT);
		}
		*aura_txt = '\0';
		if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
			if (AFF_FLAGGED(i, AFF_HOLD))
				strcat(aura_txt, "....парализован$a");
			if (AFF_FLAGGED(i, AFF_SIELENCE))
				strcat(aura_txt, "....нем$a");
		}
		if (AFF_FLAGGED(i, AFF_BLIND))
			strcat(aura_txt, "....слеп$a");
		if (AFF_FLAGGED(i, AFF_DEAFNESS))
			strcat(aura_txt, "....глух$a");
		if (*aura_txt)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		return;
	}

	if (IS_NPC(i)) {
		strcpy(buf1, i->player.short_descr);
		strcat(buf1, " ");
		if (AFF_FLAGGED(i, AFF_HORSE))
			strcat(buf1, "(под седлом) ");
		CAP(buf1);
	} else if (IS_NPC(i)) {
		strcpy(buf1, GET_NAME(i));
		strcat(buf1, " ");
		CAP(buf1);
	} else {
		sprintf(buf1, "%s%s ", race_or_title(i), PLR_FLAGGED(i, PLR_KILLER) ? " <ДУШЕГУБ>" : "");
	}

	sprintf(buf, "%s%s", AFF_FLAGGED(i, AFF_CHARM) ? "*" : "", buf1);
	if (AFF_FLAGGED(i, AFF_INVISIBLE))
		sprintf(buf + strlen(buf), "(невидим%s) ", GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, AFF_HIDE))
		sprintf(buf + strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
	if (AFF_FLAGGED(i, AFF_CAMOUFLAGE))
		sprintf(buf + strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
	if (!IS_NPC(i) && !i->desc)
		sprintf(buf + strlen(buf), "(потерял%s связь) ", GET_CH_SUF_1(i));
	if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
		strcat(buf, "(пишет) ");

	if (GET_POS(i) != POS_FIGHTING) {
		if (on_horse(i))
			sprintf(buf + strlen(buf), "сидит здесь верхом на %s. ", PERS(get_horse(i), ch, 5));
		else if (IS_HORSE(i) && AFF_FLAGGED(i, AFF_TETHERED))
			sprintf(buf + strlen(buf), "привязан%s здесь. ", GET_CH_SUF_6(i));
		else if ((sector = real_sector(IN_ROOM(i))) == SECT_FLYING)
			strcat(buf, "летает здесь. ");
		else if (sector == SECT_UNDERWATER)
			strcat(buf, "плавает здесь. ");
		else if (GET_POS(i) > POS_SLEEPING && AFF_FLAGGED(i, AFF_FLY))
			strcat(buf, "летает здесь. ");
		else if (sector == SECT_WATER_SWIM || sector == SECT_WATER_NOSWIM)
			strcat(buf, "плавает здесь. ");
		else
			strcat(buf, positions[(int) GET_POS(i)]);
	} else {
		if (FIGHTING(i)) {
			strcat(buf, "сражается c ");
			if (i->in_room != FIGHTING(i)->in_room)
				strcat(buf, "чьей-то тенью ");
			else if (FIGHTING(i) == ch)
				strcat(buf, "ВАМИ ");
			else {
				strcat(buf, GET_PAD(FIGHTING(i), 4));
				strcat(buf, " ");
			}
			strcat(buf, "! ");
		} else		/* NIL fighting pointer */
			strcat(buf, "колотит по воздуху. ");
	}

	if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
	    && !AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
		if (AFF_FLAGGED(i, AFF_EVILESS))
			strcat(buf, "(черная аура) ");
	}
	if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
		if (IS_NPC(i)) {
			if (IS_EVIL(i)) {
				if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
				    && AFF_FLAGGED(i, AFF_EVILESS))
					strcat(buf, "(иссиня-черная аура) ");
				else
					strcat(buf, "(темная аура) ");
			} else if (IS_GOOD(i)) {
				if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
				    && AFF_FLAGGED(i, AFF_EVILESS))
					strcat(buf, "(серая аура) ");
				else
					strcat(buf, "(светлая аура) ");
			} else {
				if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
				    && AFF_FLAGGED(i, AFF_EVILESS))
					strcat(buf, "(черная аура) ");
			}
		} else {
			aura(ch, C_CMP, i, aura_txt);
			strcat(buf, aura_txt);
			strcat(buf, " ");
		}
	}
	if (AFF_FLAGGED(ch, AFF_DETECT_POISON))
		if (AFF_FLAGGED(i, AFF_POISON))
			sprintf(buf + strlen(buf), "(отравлен%s) ", GET_CH_SUF_6(i));

	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	*aura_txt = '\0';
	if (AFF_FLAGGED(i, AFF_SHIELD)) {
		strcat(aura_txt, "...окутан");
		strcat(aura_txt, GET_CH_SUF_6(i));
		strcat(aura_txt, " сверкающим коконом ");
	}
	if (AFF_FLAGGED(i, AFF_SANCTUARY))
		strcat(aura_txt, "...светится ярким сиянием ");
	else if (AFF_FLAGGED(i, AFF_PRISMATICAURA))
		strcat(aura_txt, "...переливается всеми цветами ");
	act(aura_txt, FALSE, i, 0, ch, TO_VICT);

	*aura_txt = '\0';
	n = 0;
	strcat(aura_txt, "...окружен");
	strcat(aura_txt, GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, AFF_AIRSHIELD)) {
		strcat(aura_txt, " воздушным");
		n++;
	}
	if (AFF_FLAGGED(i, AFF_FIRESHIELD)) {
		if (n > 0)
			strcat(aura_txt, ", огненным");
		else
			strcat(aura_txt, " огненным");
		n++;
	}
	if (AFF_FLAGGED(i, AFF_ICESHIELD)) {
		if (n > 0)
			strcat(aura_txt, ", ледяным");
		else
			strcat(aura_txt, " ледяным");
		n++;
	}
	if (n == 1)
		strcat(aura_txt, " щитом ");
	else if (n > 1)
		strcat(aura_txt, " щитами ");
	if (n > 0)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);

	if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, " ..");
		if (AFF_FLAGGED(i, AFF_AIRAURA)) {
			strcat(aura_txt, "воздушная");
			n++;
		}
		if (AFF_FLAGGED(i, AFF_FIREAURA)) {
			if (n > 0)
				strcat(aura_txt, ", огненная");
			else
				strcat(aura_txt, "огненная");
			n++;
		}
		if (AFF_FLAGGED(i, AFF_ICEAURA)) {
			if (n > 0)
				strcat(aura_txt, ", ледяная");
			else
				strcat(aura_txt, "ледяная");
			n++;
		}
		if (AFF_FLAGGED(i, AFF_MAGICGLASS)) {
			if (n > 0)
				strcat(aura_txt, ", серебристая");
			else
				strcat(aura_txt, "серебристая");
			n++;
		}
		if (n == 1)
			strcat(aura_txt, " аура ");
		else if (n > 1)
			strcat(aura_txt, " ауры ");

		if (n > 0)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);
	}
	*aura_txt = '\0';
	if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
		if (AFF_FLAGGED(i, AFF_HOLD))
			strcat(aura_txt, " ...парализован$a");
		if (AFF_FLAGGED(i, AFF_SIELENCE))
			strcat(aura_txt, " ...нем$a");
	}
	if (AFF_FLAGGED(i, AFF_BLIND))
		strcat(aura_txt, " ...слеп$a");
	if (AFF_FLAGGED(i, AFF_DEAFNESS))
		strcat(aura_txt, " ...глух$a");
	if (*aura_txt)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);
}

void list_char_to_char(CHAR_DATA * list, CHAR_DATA * ch)
{
	CHAR_DATA *i;

	for (i = list; i; i = i->next_in_room)
		if (ch != i) {
			if (HERE(i) && (CAN_SEE(ch, i)
					|| awaking(i, AW_HIDE | AW_INVIS | AW_CAMOUFLAGE)))
				list_one_char(i, ch, 0);
			else if (IS_DARK(i->in_room) &&
				 IN_ROOM(i) == IN_ROOM(ch) && !CAN_SEE_IN_DARK(ch) && AFF_FLAGGED(i, AFF_INFRAVISION))
				send_to_char("Пара светящихся глаз смотрит на Вас.\r\n", ch);
		}
}

void do_auto_exits(CHAR_DATA * ch)
{
	int door, slen = 0;

	*buf = '\0';

	for (door = 0; door < NUM_OF_DIRS; door++)
	/* Наконец-то добавлена отрисовка в автовыходах закрытых дверей */
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE) {
			if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
				slen += sprintf(buf + slen, "(%c) ", LOWER(*dirs[door]));
			else if (!EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
				slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));
		}
	sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM), *buf ? buf : "None! ", CCNRM(ch, C_NRM));

	send_to_char(buf2, ch);
}


ACMD(do_exits)
{
	int door;

	*buf = '\0';

	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("Вы слепы, как котенок !\r\n", ch);
		return;
	}
	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
			if (IS_GOD(ch))
				sprintf(buf2, "%-5s - [%5d] %s\r\n", Dirs[door],
					GET_ROOM_VNUM(EXIT(ch, door)->to_room), world[EXIT(ch, door)->to_room]->name);
			else {
				sprintf(buf2, "%-5s - ", Dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "слишком темно\r\n");
				else {
					strcat(buf2, world[EXIT(ch, door)->to_room]->name);
					strcat(buf2, "\r\n");
				}
			}
			strcat(buf, CAP(buf2));
		}
	send_to_char("Видимые выходы :\r\n", ch);
	if (*buf)
		send_to_char(buf, ch);
	else
		send_to_char(" Замуровали, ДЕМОНЫ !\r\n", ch);
}


#define MAX_FIRES 6
const char *Fires[MAX_FIRES] = { "тлеет небольшая кучка угольков",
	"тлеет небольшая кучка угольков",
	"еле-еле теплится огонек",
	"догорает небольшой костер",
	"весело трещит костер",
	"ярко пылает костер"
};

#define TAG_NIGHT       "<night>"
#define TAG_DAY         "<day>"
#define TAG_WINTERNIGHT "<winternight>"
#define TAG_WINTERDAY   "<winterday>"
#define TAG_SPRINGNIGHT "<springnight>"
#define TAG_SPRINGDAY   "<springday>"
#define TAG_SUMMERNIGHT "<summernight>"
#define TAG_SUMMERDAY   "<summerday>"
#define TAG_AUTUMNNIGHT "<autumnnight>"
#define TAG_AUTUMNDAY   "<autumnday>"

int paste_description(char *string, char *tag, int need)
{
	char *pos;
	if (!*string || !*tag)
		return (FALSE);
	if ((pos = str_str(string, tag))) {
		if (need) {
			for (; *pos && *pos != '>'; pos++);
			if (*pos)
				pos++;
			if (*pos == 'R') {
				pos++;
				buf[0] = '\0';
			}
			strcat(buf, pos);
			if ((pos = str_str(buf, tag)))
				*pos = '\0';
			return (TRUE);
		} else {
			*pos = '\0';
			if ((pos = str_str(pos + 1, tag)))
				strcat(string, pos + strlen(tag));
		}
	}
	return (FALSE);
}


void show_extend_room(char *description, CHAR_DATA * ch)
{
	int found = FALSE, i;
	char string[MAX_STRING_LENGTH], *pos;

	if (!description || !*description)
		return;

	strcpy(string, description);
	if ((pos = strchr(description, '<')))
		*pos = '\0';
	strcpy(buf, description);
	if (pos)
		*pos = '<';

	found = found || paste_description(string, TAG_WINTERNIGHT,
					   (weather_info.season == SEASON_WINTER &&
					    (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found
	    || paste_description(string, TAG_WINTERDAY,
				 (weather_info.season == SEASON_WINTER
				  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found
	    || paste_description(string, TAG_SPRINGNIGHT,
				 (weather_info.season == SEASON_SPRING
				  && (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found
	    || paste_description(string, TAG_SPRINGDAY,
				 (weather_info.season == SEASON_SPRING
				  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found
	    || paste_description(string, TAG_SUMMERNIGHT,
				 (weather_info.season == SEASON_SUMMER
				  && (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found
	    || paste_description(string, TAG_SUMMERDAY,
				 (weather_info.season == SEASON_SUMMER
				  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found
	    || paste_description(string, TAG_AUTUMNNIGHT,
				 (weather_info.season == SEASON_AUTUMN
				  && (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found
	    || paste_description(string, TAG_AUTUMNDAY,
				 (weather_info.season == SEASON_AUTUMN
				  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found
	    || paste_description(string, TAG_NIGHT,
				 (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK));
	found = found
	    || paste_description(string, TAG_DAY,
				 (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT));
	for (i = strlen(buf); i > 0 && *(buf + i) == '\n'; i--) {
		*(buf + i) = '\0';
		if (i > 0 && *(buf + i) == '\r')
			*(buf + --i) = '\0';
	}

	send_to_char(buf, ch);
	send_to_char("\r\n", ch);
}

void look_at_room(CHAR_DATA * ch, int ignore_brief)
{
	if (!ch->desc)
		return;

	if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
		send_to_char("Слишком темно...\r\n", ch);
		return;
	} else if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("Вы все еще слепы...\r\n", ch);
		return;
	} else if (GET_POS(ch) < POS_SLEEPING)
		return;
	send_to_char(CCICYN(ch, C_NRM), ch);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
		sprintbits(world[ch->in_room]->room_flags, room_bits, buf, ";");
		sprintf(buf2, "[%5d] %s [%s]", GET_ROOM_VNUM(IN_ROOM(ch)), world[ch->in_room]->name, buf);
		send_to_char(buf2, ch);
	} else
		send_to_char(world[ch->in_room]->name, ch);

	send_to_char(CCNRM(ch, C_NRM), ch);
	send_to_char("\r\n", ch);

	if (IS_DARK(IN_ROOM(ch)) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
		send_to_char("Слишком темно...\r\n", ch);
	else if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief || ROOM_FLAGGED(ch->in_room, ROOM_DEATH)) {
		show_extend_room(world[ch->in_room]->description, ch);
	}

	/* Отображаем аффекты комнаты */
	if AFF_FLAGGED(ch, AFF_DETECT_MAGIC) 
		sprintbits(world[ch->in_room]->affected_by, room_aff_invis_bits, buf2, "\n");
	else 
		sprintbits(world[ch->in_room]->affected_by, room_aff_visib_bits, buf2, "\n");

	// подавляем если нет аффектов
	if (strcmp(buf2,"ничего"))
	{
		sprintf(buf, "%s\r\n\r\n",buf2);
		send_to_char(buf, ch);
	}

	/* autoexits */
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
		do_auto_exits(ch);

	/* now list characters & objects */
	if (world[IN_ROOM(ch)]->fires) {
		sprintf(buf, "%sВ центре %s.%s\r\n",
			CCRED(ch, C_NRM), Fires[MIN(world[IN_ROOM(ch)]->fires, MAX_FIRES - 1)], CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (world[IN_ROOM(ch)]->portal_time) {
		sprintf(buf,
			"%sЛазурная пентаграмма переливается ярко сверкает здесь.%s\r\n",
			CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (world[IN_ROOM(ch)]->holes) {
		int ar = (int) roundup(world[IN_ROOM(ch)]->holes / HOLES_TIME);
		sprintf(buf, "%sЗдесь выкопана ямка глубиной примерно в %i аршин%s.%s\r\n",
			CCYEL(ch, C_NRM), ar, (ar == 1 ? "" : (ar < 5 ? "а" : "ов")), (CCNRM(ch, C_NRM)));
		send_to_char(buf, ch);
	}

	if (IN_ROOM(ch) != NOWHERE && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOWEATHER)) {
		*buf = '\0';
		switch (real_sector(IN_ROOM(ch))) {
		case SECT_FIELD_SNOW:
		case SECT_FOREST_SNOW:
		case SECT_HILLS_SNOW:
		case SECT_MOUNTAIN_SNOW:
			sprintf(buf, "%sСнежный ковер лежит у Вас под ногами.%s\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_FIELD_RAIN:
		case SECT_FOREST_RAIN:
		case SECT_HILLS_RAIN:
			sprintf(buf, "%sВы просто увязаете в грязи.%s\r\n", CCIWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_THICK_ICE:
			sprintf(buf, "%sУ Вас под ногами толстый лед.%s\r\n", CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_NORMAL_ICE:
			sprintf(buf, "%sУ Вас под ногами достаточно толстый лед.%s\r\n",
				CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_THIN_ICE:
			sprintf(buf,
				"%sТоненький ледок вот-вот проломится под Вами.%s\r\n",
				CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		};
		if (*buf)
			send_to_char(buf, ch);
	}

	send_to_char(CCIYEL(ch, C_NRM), ch);
//  if (IS_SET(GET_SPELL_TYPE(ch, SPELL_TOWNPORTAL),SPELL_KNOW))
	if (GET_SKILL(ch, SKILL_TOWNPORTAL))
		if (find_portal_by_vnum(GET_ROOM_VNUM(ch->in_room)))
			send_to_char("Рунный камень с изображением пентаграммы немного выступает из земли.\r\n", ch);
	list_obj_to_char(world[ch->in_room]->contents, ch, 0, FALSE);
	send_to_char(CCIRED(ch, C_NRM), ch);
	list_char_to_char(world[ch->in_room]->people, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
}


void look_in_direction(CHAR_DATA * ch, int dir, int info_is)
{
	int count = 0, probe, percent;
	EXIT_DATA *rdata = NULL;
	CHAR_DATA *tch;
	if (CAN_GO(ch, dir)
	    || (EXIT(ch, dir) && EXIT(ch, dir)->to_room != NOWHERE)) {
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", CCYEL(ch, C_NRM), Dirs[dir], CCNRM(ch, C_NRM));
		if (EXIT_FLAGGED(rdata, EX_CLOSED)) {
			if (rdata->keyword)
				count += sprintf(buf + count, " закрыто (%s).\r\n", rdata->keyword);
			else
				count += sprintf(buf + count, " закрыто (вероятно дверь).\r\n");
			send_to_char(buf, ch);
			return;
		};
		if (IS_TIMEDARK(rdata->to_room)) {
			count += sprintf(buf + count, " слишком темно.\r\n");
			send_to_char(buf, ch);
			if (info_is & EXIT_SHOW_LOOKING) {
				send_to_char(CCIRED(ch, C_NRM), ch);
				for (count = 0, tch = world[rdata->to_room]->people; tch; tch = tch->next_in_room) {
					percent = number(1, skill_info[SKILL_LOOKING].max_percent);
					probe =
					    train_skill(ch, SKILL_LOOKING, skill_info[SKILL_LOOKING].max_percent, tch);
					if (HERE(tch) && INVIS_OK(ch, tch) && probe >= percent
					    && (percent < 100 || IS_IMMORTAL(ch))) {
						list_one_char(tch, ch, SKILL_LOOKING);
						count++;
					}
				}
				if (!count)
					send_to_char("Вы ничего не смогли разглядеть!\r\n", ch);
				send_to_char(CCNRM(ch, C_NRM), ch);
			}
		} else {
			if (rdata->general_description)
				count += sprintf(buf + count, "%s\r\n", rdata->general_description);
			else
				count += sprintf(buf + count, "%s\r\n", world[rdata->to_room]->name);
			send_to_char(buf, ch);
			send_to_char(CCIRED(ch, C_NRM), ch);
			list_char_to_char(world[rdata->to_room]->people, ch);
			send_to_char(CCNRM(ch, C_NRM), ch);
		}
	} else if (info_is & EXIT_SHOW_WALL)
		send_to_char("И что Вы там мечтаете увидеть ?\r\n", ch);
}

void hear_in_direction(CHAR_DATA * ch, int dir, int info_is)
{
	int count = 0, percent = 0, probe = 0;
	EXIT_DATA *rdata;
	CHAR_DATA *tch;
	int fight_count = 0;
	string tmpstr = "";

	if (AFF_FLAGGED(ch, AFF_DEAFNESS)) {
		send_to_char("Вы забыли, что вы глухи ?\r\n", ch);
		return;
	}
	if (CAN_GO(ch, dir)) {
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", CCYEL(ch, C_NRM), Dirs[dir], CCNRM(ch, C_NRM));

		if (EXIT_FLAGGED(rdata, EX_CLOSED) && rdata->keyword) {
			count += sprintf(buf + count, " закрыто (%s).\r\n", fname(rdata->keyword));
			send_to_char(buf, ch);
			return;
		};
		count += sprintf(buf + count, "\r\n%s", CCGRN(ch, C_NRM));
		send_to_char(buf, ch);
		for (count = 0, tch = world[rdata->to_room]->people; tch; tch = tch->next_in_room) {
			percent = number(1, skill_info[SKILL_HEARING].max_percent);
			probe = train_skill(ch, SKILL_HEARING, skill_info[SKILL_HEARING].max_percent, tch);
			// Если сражаются то слышем только борьбу.
			if (FIGHTING(tch)) {
				if (IS_NPC(tch))
					tmpstr += " Вы слышите шум чьей-то борьбы.\r\n";
				else
					tmpstr += " Вы слышите звуки чьих-то ударов.\r\n";
				fight_count++;
				continue;
			}
			if ((probe >= percent ||
			     ((!AFF_FLAGGED(tch, AFF_SNEAK) ||
			       !AFF_FLAGGED(tch, AFF_HIDE)) &&
			      (probe > percent * 2)) && (percent < 100 || IS_IMMORTAL(ch)) && !fight_count)) {
				if (IS_NPC(tch)) {
					if (real_sector(IN_ROOM(ch)) != SECT_UNDERWATER) {
						if (GET_LEVEL(tch) < 5)
							tmpstr += " Вы слышите чью-то тихую возню.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " Вы слышите чье-то сопение.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " Вы слышите чье-то громкое дыхание.\r\n";
						else
							tmpstr += " Вы слышите чье-то грозное дыхание.\r\n";
					} else {
						if (GET_LEVEL(tch) < 5)
							tmpstr += " Вы слышите тихое бульканье.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " Вы слышите бульканье.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " Вы слышите громкое бульканье.\r\n";
						else
							tmpstr += " Вы слышите грозное пузырение.\r\n";
					}
				} else
					tmpstr += " Вы слышите чье-то присутствие.\r\n";
				count++;
			}
		}
		if ((!count) && (!fight_count))
			send_to_char(" Тишина и покой.\r\n", ch);
		else
			send_to_char(tmpstr.c_str(), ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
	} else {
		if (info_is & EXIT_SHOW_WALL)
			send_to_char("И что вы там хотите услышать ?.\r\n", ch);
	}
}



void look_in_obj(CHAR_DATA * ch, char *arg)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *dummy = NULL;
	char *what, whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
	int amt, bits;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

	if (!*arg)
		send_to_char("Смотреть во что?\r\n", ch);
	else
		half_chop(arg, whatp, where);
	what = whatp;

	if (isname(where, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(where, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	bits = generic_find(arg, where_bits, ch, &dummy, &obj);

	if ((obj == NULL) || !bits) {
		sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg);
		send_to_char(buf, ch);
	} else
	    if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
		(GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
		send_to_char("Ничего в нем нет !\r\n", ch);
	else {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				act("Закрыт$g.", FALSE, ch, obj, 0, TO_CHAR);
			else {
				send_to_char(OBJN(obj, ch, 0), ch);
				switch (bits) {
				case FIND_OBJ_INV:
					send_to_char("(в руках)\r\n", ch);
					break;
				case FIND_OBJ_ROOM:
					send_to_char("(на земле)\r\n", ch);
					break;
				case FIND_OBJ_EQUIP:
					send_to_char("(в амуниции)\r\n", ch);
					break;
				}
				if (!obj->contains)
					send_to_char(" Внутри ничего нет.\r\n", ch);
				else
					list_obj_to_char(obj->contains, ch, 1, bits != FIND_OBJ_ROOM);
			}
		} else {	/* item must be a fountain or drink container */
			if (GET_OBJ_VAL(obj, 1) <= 0)
				send_to_char("Пусто.\r\n", ch);
			else {
				if (GET_OBJ_VAL(obj, 0) <= 0 || GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0)) {
					sprintf(buf, "Заполнен%s вакуумом ?!.\r\n", GET_OBJ_SUF_6(obj));	/* BUG */
				} else {
					amt = (GET_OBJ_VAL(obj, 1) * 5) / GET_OBJ_VAL(obj, 0);
					sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
					sprintf(buf, "Наполнен%s %s%s%s жидкостью.\r\n",
						GET_OBJ_SUF_6(obj), fullness[amt],
						buf2,
						(AFF_FLAGGED(ch, AFF_DETECT_POISON) &&
						 GET_OBJ_VAL(obj, 3) > 0 ? "(отравленной)" : ""));
				}
				send_to_char(buf, ch);
			}
		}
	}
}



char *find_exdesc(char *word, EXTRA_DESCR_DATA * list)
{
	EXTRA_DESCR_DATA *i;

	for (i = list; i; i = i->next)
		if (isname(word, i->keyword))
			return (i->description);

	return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(CHAR_DATA * ch, char *arg, int subcmd)
{
	int bits, found = FALSE, fnum, i = 0, cn = 0;
	struct portals_list_type *port;
	CHAR_DATA *found_char = NULL;
	OBJ_DATA *found_obj = NULL;
	struct char_portal_type *tmp;
	char *desc, *what, whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM;

	if (!ch->desc)
		return;

	if (!*arg) {
		send_to_char("На что Вы так мечтаете посмотреть ?\r\n", ch);
		return;
	}

	half_chop(arg, whatp, where);
	what = whatp;

	if (isname(where, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
	else if (isname(where, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	/* для townportal */
	if (isname(whatp, "камень") &&
//       IS_SET(GET_SPELL_TYPE(ch, SPELL_TOWNPORTAL), SPELL_KNOW) &&
	    GET_SKILL(ch, SKILL_TOWNPORTAL) &&
	    (port = get_portal(GET_ROOM_VNUM(ch->in_room), NULL)) != NULL && IS_SET(where_bits, FIND_OBJ_ROOM)) {
		if (GET_LEVEL(ch) < port->level) {
			send_to_char("На камне что-то написано огненными буквами.\r\n", ch);
			send_to_char("Но вы еще недостаточно искусны, чтобы разобрать слово.\r\n", ch);
			return;
		} else {
			for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
				cn++;
			}
			if (cn >= MAX_PORTALS(ch)) {
				send_to_char
				    ("Все доступные вам камни уже запомнены, удалите и попробуйте еще.\r\n", ch);
				return;
			}
			send_to_char("На камне огненными буквами напиcано слово '&R", ch);
			send_to_char(port->wrd, ch);
			send_to_char("&n'.\r\n", ch);
			/* теперь добавляем в память чара */
			add_portal_to_char(ch, GET_ROOM_VNUM(ch->in_room));
			check_portals(ch);
			return;
		}
	}

	/* заглянуть в пентаграмму */
	if (isname(whatp, "пентаграмма") && world[IN_ROOM(ch)]->portal_time && IS_SET(where_bits, FIND_OBJ_ROOM)) {
		int r = IN_ROOM(ch), to_room;
		to_room = world[r]->portal_room;
		send_to_char("Приблизившись к пентаграмме, Вы осторожно заглянули в нее.\r\n\r\n", ch);
		act("$n0 осторожно заглянул$g в пентаграмму.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		if (world[to_room]->portal_time && (r == world[to_room]->portal_room)) {
			send_to_char
			    ("Яркий свет, идущий с противоположного конца прохода, застилает Вам глаза.\r\n\r\n", ch);
			return;
		}
		IN_ROOM(ch) = world[IN_ROOM(ch)]->portal_room;
		look_at_room(ch, 1);
		IN_ROOM(ch) = r;
		return;
	}

	bits = generic_find(what, where_bits, ch, &found_char, &found_obj);
	/* Is the target a character? */
	if (found_char != NULL) {
		if (subcmd == SCMD_LOOK_HIDE && !check_moves(ch, LOOKHIDE_MOVES))
			return;
		look_at_char(found_char, ch);
		if (ch != found_char) {
			if (subcmd == SCMD_LOOK_HIDE && GET_SKILL(ch, SKILL_LOOK_HIDE) > 0) {
				fnum = number(1, skill_info[SKILL_LOOK_HIDE].max_percent);
				found =
				    train_skill(ch, SKILL_LOOK_HIDE,
						skill_info[SKILL_LOOK_HIDE].max_percent, found_char);
				if (!WAITLESS(ch))
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				if (found >= fnum && (fnum < 100 || IS_IMMORTAL(ch)) && !IS_IMMORTAL(found_char))
					return;
			}
			if (CAN_SEE(found_char, ch))
				act("$n оглядел$g Вас с головы до пят.", TRUE, ch, 0, found_char, TO_VICT);
			act("$n посмотрел$g на $N3.", TRUE, ch, 0, found_char, TO_NOTVICT);
		}
		return;
	}

	/* Strip off "number." from 2.foo and friends. */
	if (!(fnum = get_number(&what))) {
		send_to_char("Что осматриваем ?\r\n", ch);
		return;
	}

	/* Does the argument match an extra desc in the room? */
	if ((desc = find_exdesc(what, world[ch->in_room]->ex_description)) != NULL && ++i == fnum) {
		page_string(ch->desc, desc, FALSE);
		return;
	}

/* Начало изменений.
   (с) Дмитрий ака dzMUDiST ака Кудояр */

// Закомментированый ниже код IMHO не нужен, т.к. повторяет сделанное функцией
//    bits = generic_find (what, where_bits, ch, &found_char, &found_obj);
// и вдобавок изменяет указатель found_obj, что приводит к отображению 
// описаний не тех предметов.


	/* Does the argument match an extra desc in the char's equipment? */
//  for (j = 0;
//       j < NUM_WEARS && !found && IS_SET (where_bits, FIND_OBJ_EQUIP); j++)
//    if (GET_EQ (ch, j) &&
//      CAN_SEE_OBJ (ch, GET_EQ (ch, j)) &&
//      (desc = find_exdesc (what, GET_EQ (ch, j)->ex_description)) &&
//      ++i == fnum)
//      {
//      send_to_char (desc, ch);
//      found_obj = GET_EQ (ch, j);
//      found = TRUE;
//      in_eq = TRUE;
//      }

	/* Does the argument match an extra desc in the char's inventory? */
//  for (obj = ch->carrying;
//       obj && !found && IS_SET (where_bits, FIND_OBJ_INV);
//       obj = obj->next_content)
//    {
//      if (CAN_SEE_OBJ (ch, obj) &&
//        (desc = find_exdesc (what, obj->ex_description)) && ++i == fnum)
//      {
//        send_to_char (desc, ch);
//        found_obj = obj;
//        found = TRUE;
//        in_eq = TRUE;
//      }
//    }

	/* Does the argument match an extra desc of an object in the room? */
//  for (obj = world[ch->in_room]->contents;
//       obj && !found && IS_SET (where_bits, FIND_OBJ_ROOM);
//       obj = obj->next_content)
//    if (CAN_SEE_OBJ (ch, obj) &&
//      (desc = find_exdesc (what, obj->ex_description)) && ++i == fnum)
//      {
//      send_to_char (desc, ch);
//      found_obj = obj;
//      found = TRUE;
//      }


	/* If an object was found back in generic_find */
	if (bits && (found_obj != NULL)) {

		// Собственно изменение. Вместо проверки "if (!found)" юзается проверка
		// наличия описания у объекта, найденного функцией "generic_find"

		if (!(desc = find_exdesc(what, found_obj->ex_description)))
			show_obj_to_char(found_obj, ch, 5, TRUE, 1);	/* Show no-description */
		else {
			send_to_char(desc, ch);
			show_obj_to_char(found_obj, ch, 6, TRUE, 1);	/* Find hum, glow etc */
		}

/* Конец изменений.
   (с) Дмитрий ака dzMUDiST ака Кудояр */

		if (GET_CLASS(ch) == CLASS_MERCHANT && GET_LEVEL(ch) >= 20) {
			send_to_char("Материал : ", ch);
			send_to_char(CCCYN(ch, C_NRM), ch);
			sprinttype(found_obj->obj_flags.Obj_mater, material_name, buf);
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
			send_to_char(CCNRM(ch, C_NRM), ch);
		}

		if ((CAN_WEAR(found_obj, ITEM_WEAR_BODY) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_HEAD) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_LEGS) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_ARMS) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_SHIELD) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_WIELD) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_HOLD) ||
		     CAN_WEAR(found_obj, ITEM_WEAR_BOTHS)) &&
		    (GET_CLASS(ch) == CLASS_SMITH && GET_SKILL(ch, SKILL_INSERTGEM) >= 60)) {
			send_to_char("Слоты : ", ch);
			send_to_char(CCCYN(ch, C_NRM), ch);
			if (OBJ_FLAGGED(found_obj, ITEM_WITH3SLOTS))
				strcat(buf, "доступно 3 слота\r\n");
			else if (OBJ_FLAGGED(found_obj, ITEM_WITH2SLOTS))
				strcat(buf, "доступно 2 слота\r\n");
			else if (OBJ_FLAGGED(found_obj, ITEM_WITH1SLOT))
				strcat(buf, "доступен 1 слот\r\n");
			else
				strcat(buf, "нет слотов\r\n");
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
			send_to_char(CCNRM(ch, C_NRM), ch);
		}

	} else
		send_to_char("Похоже, этого здесь нет !\r\n", ch);
}


void skip_hide_on_look(CHAR_DATA * ch)
{

	if (AFF_FLAGGED(ch, AFF_HIDE) &&
	    ((!GET_SKILL(ch, SKILL_LOOK_HIDE) ||
	      ((number(1, 100) -
		calculate_skill(ch, SKILL_LOOK_HIDE,
				skill_info[SKILL_LOOK_HIDE].max_percent, 0) - 2 * (GET_WIS(ch) - 9)) > 0)))) {
		affect_from_char(ch, SPELL_HIDE);
		if (!AFF_FLAGGED(ch, AFF_HIDE)) {
			send_to_char("Вы прекратили прятаться.\r\n", ch);
			act("$n прекратил$g прятаться.", FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	return;
}


ACMD(do_look)
{
	char arg2[MAX_INPUT_LENGTH];
	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char("Вы ослеплены !\r\n", ch);
	else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
		skip_hide_on_look(ch);

		send_to_char("Слишком темно...\r\n", ch);
		list_char_to_char(world[ch->in_room]->people, ch);	/* glowing red eyes */
	} else {
		half_chop(argument, arg, arg2);

		skip_hide_on_look(ch);

		if (subcmd == SCMD_READ) {
			if (!*arg)
				send_to_char("Что вы хотите прочитать ?\r\n", ch);
			else
				look_at_target(ch, arg, subcmd);
			return;
		}
		if (!*arg)	/* "look" alone, without an argument at all */
			look_at_room(ch, 1);
		else if (is_abbrev(arg, "in") || is_abbrev(arg, "внутрь"))
			look_in_obj(ch, arg2);
		/* did the char type 'look <direction>?' */
		else if (((look_type = search_block(arg, dirs, FALSE)) >= 0) ||
			 ((look_type = search_block(arg, Dirs, FALSE)) >= 0))
			look_in_direction(ch, look_type, EXIT_SHOW_WALL);
		else if (is_abbrev(arg, "at") || is_abbrev(arg, "на"))
			look_at_target(ch, arg2, subcmd);
		else
			look_at_target(ch, argument, subcmd);
	}
}

ACMD(do_sides)
{
	int i;

	if (!ch->desc)
		return;

	if (GET_POS(ch) <= POS_SLEEPING)
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char("Вы ослеплены !\r\n", ch);
	else {
		skip_hide_on_look(ch);
		send_to_char("Вы посмотрели по сторонам.\r\n", ch);
		for (i = 0; i < NUM_OF_DIRS; i++) {
//         log("Look sides from %d to %d",world[IN_ROOM(ch)]->number, i);
			look_in_direction(ch, i, 0);
//           log("Look Ok !");
		}
	}
}


ACMD(do_looking)
{
	int i;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("Белый Ангел возник перед Вами, маняще помахивая крыльями.\r\n", ch);
	if (GET_POS(ch) == POS_SLEEPING)
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char("Вы ослеплены !\r\n", ch);
	else if (GET_SKILL(ch, SKILL_LOOKING)) {
		if (check_moves(ch, LOOKING_MOVES)) {
			send_to_char("Вы напрягли зрение и начали присматриваться по сторонам.\r\n", ch);
			for (i = 0; i < NUM_OF_DIRS; i++)
				look_in_direction(ch, i, EXIT_SHOW_LOOKING);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
	} else
		send_to_char("Вам явно не хватает этого умения.\r\n", ch);
}

ACMD(do_hearing)
{
	int i;

	if (!ch->desc)
		return;

	if (AFF_FLAGGED(ch, AFF_SIELENCE)) {
		send_to_char("Вы глухи и все равно ничего не услышите.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("Вам начали слышаться голоса предков, зовущие Вас к себе.\r\n", ch);
	if (GET_POS(ch) == POS_SLEEPING)
		send_to_char("мОрфей медленно задумчиво провел рукой по струнам и заиграл колыбельную.\r\n", ch);
	else if (GET_SKILL(ch, SKILL_HEARING)) {
		if (check_moves(ch, HEARING_MOVES)) {
			send_to_char("Вы начали сосредоточенно прислушиваться.\r\n", ch);
			for (i = 0; i < NUM_OF_DIRS; i++)
				hear_in_direction(ch, i, 0);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
	} else
		send_to_char("Выучите сначала как это следует делать.\r\n", ch);
}



ACMD(do_examine)
{
	CHAR_DATA *tmp_char;
	OBJ_DATA *tmp_object;
	char where[MAX_INPUT_LENGTH];
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM;


	if (GET_POS(ch) < POS_SLEEPING) {
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
		return;
	} else if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("Вы ослеплены !\r\n", ch);
		return;
	}

	two_arguments(argument, arg, where);

	if (!*arg) {
		send_to_char("Что Вы желаете осмотреть ?\r\n", ch);
		return;
	}

	if (isname(where, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
	else if (isname(where, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	skip_hide_on_look(ch);

	look_at_target(ch, argument, subcmd);

	if (isname(arg, "пентаграмма") && world[IN_ROOM(ch)]->portal_time && IS_SET(where_bits, FIND_OBJ_ROOM))
		return;

	if (isname(arg, "камень") &&
	    GET_SKILL(ch, SKILL_TOWNPORTAL) &&
	    (get_portal(GET_ROOM_VNUM(ch->in_room), NULL)) != NULL && IS_SET(where_bits, FIND_OBJ_ROOM))
		return;

	generic_find(arg, where_bits, ch, &tmp_char, &tmp_object);
	if (tmp_object) {
		if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
		    (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) || (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER))
			look_in_obj(ch, argument);
	}
}

ACMD(do_gold)
{
	int count = 0;
	if (GET_GOLD(ch) == 0)
		send_to_char("Вы разорены !\r\n", ch);
	else if (GET_GOLD(ch) == 1)
		send_to_char("У Вас есть всего лишь одна куна.\r\n", ch);
	else {
		count += sprintf(buf, "У Вас есть %d %s.\r\n", GET_GOLD(ch), desc_count(GET_GOLD(ch), WHAT_MONEYa));
		send_to_char(buf, ch);
	}
}

const char *class_name[] = { "лекарь",
	"колдун",
	"тать",
	"богатырь",
	"наемник",
	"дружинник",
	"кудесник",
	"волшебник",
	"чернокнижник",
	"витязь",
	"охотник",
	"кузнец",
	"купец",
	"волхв",
	"жрец",
	"нойда",
	"тиуве",
	"берсерк",
	"наемник",
	"хирдман",
	"заарин",
	"босоркун",
	"равк",
	"кампе",
	"лучник",
	"аргун",
	"кепмен",
	"скальд",
	"знахарь",
	"бакша",
	"карак",
	"батыр",
	"тургауд",
	"нуке",
	"капнобатай",
	"акшаман",
	"карашаман",
	"чериг",
	"шикорхо",
	"дархан",
	"сатучы",
	"сеид"
};


const char *ac_text[] = {
	"&WВы защищены как БОГ",	/*  -30  */
	"&WВы защищены как БОГ",	/*  -29  */
	"&WВы защищены как БОГ",	/*  -28  */
	"&gВы защищены почти как БОГ",	/*  -27  */
	"&gВы защищены почти как БОГ",	/*  -26  */
	"&gВы защищены почти как БОГ",	/*  -25  */
	"&gНаилучшая защита",	/*  -24  */
	"&gНаилучшая защита",	/*  -23  */
	"&gНаилучшая защита",	/*  -22  */
	"&gВеликолепная защита",	/*  -21  */
	"&gВеликолепная защита",	/*  -20  */
	"&gВеликолепная защита",	/*  -19  */
	"&gОтличная защита",	/*  -18  */
	"&gОтличная защита",	/*  -17  */
	"&gОтличная защита",	/*  -16  */
	"&GОчень хорошая защита",	/*  -15  */
	"&GОчень хорошая защита",	/*  -14  */
	"&GОчень хорошая защита",	/*  -13  */
	"&GВесьма хорошая защита",	/*  -12  */
	"&GВесьма хорошая защита",	/*  -11  */
	"&GВесьма хорошая защита",	/*  -10  */
	"&GХорошая защита",	/*   -9  */
	"&GХорошая защита",	/*   -8  */
	"&GХорошая защита",	/*   -7  */
	"&GНеплохая защита",	/*   -6  */
	"&GНеплохая защита",	/*   -5  */
	"&GНеплохая защита",	/*   -4  */
	"&YЗащита чуть выше среднего",	/*   -3  */
	"&YЗащита чуть выше среднего",	/*   -2  */
	"&YЗащита чуть выше среднего",	/*   -1  */
	"&YСредняя защита",	/*    0  */
	"&YЗащита чуть ниже среднего",
	"&YСлабая защита",
	"&RСлабая защита",
	"&RОчень слабая защита",
	"&RВы немного защищены",	/* 5 */
	"&RВы совсем немного защищены",
	"&rВы чуть-чуть защищены",
	"&rВы легко уязвимы",
	"&rВы почти полностью уязвимы",
	"&rВы полностью уязвимы",	/* 10 */
};

ACMD(do_score)
{
	TIME_INFO_DATA playing_time;
	OBJ_DATA *weapon = NULL;
	int ac, ac_t, max_dam = 0, hr = 0, resist, modi = 0, skill = SKILL_BOTHHANDS;
	string sum;

	skip_spaces(&argument);

	if (IS_NPC(ch))
		return;

//Обработка команды "счет все", добавил Adept. Ширина таблицы - 85 символов + пробел.
	if (is_abbrev(argument, "все") || is_abbrev(argument, "all")) {

	if (GET_LEVEL(ch) < 6) {
		send_to_char(ch, "Вам следует достичь хотя бы шестого уровня, чтобы воспользоваться этой командой!");
		return;
	}

	sum = string ("Вы ") + string(GET_PAD(ch, 0)) + string(", ")
			+ string(class_name[(int) GET_CLASS (ch)+14*GET_KIN (ch)]) + string(".");

	sprintf(buf,
		" %s-------------------------------------------------------------------------------------\r\n"
		" || %s%-80s%s||\r\n"
		" -------------------------------------------------------------------------------------\r\n",
		CCCYN(ch, C_NRM),
		CCNRM(ch, C_NRM), sum.substr(0, 80).c_str(), CCCYN(ch, C_NRM));

	sprintf(buf+ strlen(buf),
		" || %sПлемя: %-12s%s|"
		" %sРост: %3d(%3d)       %s|"
		" %sБроня: %4d      %s|"
		" %sСопротивление:   %s||\r\n",
		CCNRM(ch, C_NRM),
		string(kin_name[GET_KIN(ch)][(int) GET_SEX(ch)]).substr(0, 14).c_str(),
		CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), GET_ARMOUR(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), CCCYN(ch, C_NRM));

	ac = compute_armor_class(ch) / 10;
	resist = MIN(GET_RESIST(ch, FIRE_RESISTANCE), 75);
	sprintf(buf+ strlen(buf),
		" || %sРод: %-14s%s|"
		" %sВес: %3d(%3d)        %s|"
		" %sЗащита: %3d%s      |"
		" %sОгню:      %3d   %s||\r\n",
		CCNRM(ch, C_NRM), 
		string(race_name[GET_RACE(ch)][(int)GET_SEX(ch)]).substr(0, 14).c_str(),
		CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), ac, CCCYN(ch, C_NRM),
		CCIRED(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, AIR_RESISTANCE), 75);
	sprintf(buf+ strlen(buf),
		" || %sВера: %-13s%s|"
		" %sРазмер: %3d(%3d)     %s|"
		" %sПоглощение: %3d%s  |"
		" %sВоздуху:   %3d   %s||\r\n",
		CCNRM(ch, C_NRM), 
		string(religion_name[GET_RELIGION(ch)][(int) GET_SEX(ch)]).substr(0, 13).c_str(), 
		CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_SIZE(ch), GET_REAL_SIZE(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), GET_ABSORBE(ch), CCCYN(ch, C_NRM),
		CCWHT(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	hr += str_app[STRENGTH_APPLY_INDEX(ch)].tohit + GET_REAL_HR(ch)
					- thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	max_dam = GET_REAL_DR(ch) + str_app[GET_REAL_STR (ch)].todam;
	
        if (IS_WARRIOR(ch)) {
	        modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)) : 0));
	        modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)) : 0));
	        modi = MAX(100, modi);
	        max_dam += modi * max_dam / 50;
        } else
	        max_dam += 6 + 2 * GET_LEVEL(ch) / 3;			    	

	weapon = GET_EQ(ch, WEAR_BOTHS);
	if (weapon) {
		if (GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
                        max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1);		
			skill = GET_OBJ_SKILL(weapon);
			if (GET_SKILL(ch, skill) == 0) {
                                hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
                                max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			} else 
				apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
			
		}
	} else {
		weapon = GET_EQ(ch, WEAR_WIELD);
		if (weapon) {
                    if (GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
			max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1) / 2;
			skill = GET_OBJ_SKILL(weapon);
			if (GET_SKILL(ch, skill) == 0) {
			    hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
			    max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			} else
			    apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
		    }				
		} 

		weapon = GET_EQ(ch, WEAR_HOLD);
		if (weapon) {
                    if (GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
			max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1) / 2;
			skill = GET_OBJ_SKILL(weapon);
			if (GET_SKILL(ch, skill) == 0) {
       	                    hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
              	            max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			} else 
			    apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
		    }			    
		}
	}
	max_dam = MAX(0, max_dam);
	resist = MIN(GET_RESIST(ch, WATER_RESISTANCE), 75);
	sprintf(buf+ strlen(buf),
		" || %sУровень: %s%2d%s        |"
		" %sСила: %2d(%2d)         %s|"
		" %sАтака: %3d%s       |"
		" %sВоде:      %3d%s   ||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_LEVEL(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_STR(ch), GET_REAL_STR(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), hr, CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, EARTH_RESISTANCE), 75);
	sprintf(buf+ strlen(buf),
		" || %sПеревоплощений: %s%2d%s |"
		" %sЛовкость: %2d(%2d)     %s|"
		" %sУрон: %4d%s       |"
		" %sЗемле:     %3d%s   ||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_REMORT(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_DEX(ch), GET_REAL_DEX(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), max_dam, CCCYN(ch, C_NRM),
		CCYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	sprintf(buf+ strlen(buf),
		" || %sВозраст: %s%3d%s       |"
		" %sТелосложение: %2d(%2d) %s|------------------|------------------||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_AGE(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_CON(ch), GET_REAL_CON(ch), CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, VITALITY_RESISTANCE), 75);
	sprintf(buf+ strlen(buf),
		" || %sОпыт: %s%10ld%s   |"
		" %sМудрость: %2d(%2d)     %s|"
		" %sКолдовство: %3d%s  |"
		" %sЖивучесть: %3d%s   ||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_EXP(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_WIS(ch), GET_REAL_WIS(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), GET_CAST_SUCCESS(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, MIND_RESISTANCE), 75);

        if (IS_IMMORTAL(ch)) 
		sprintf(buf+ strlen(buf), " || %sДСУ: %s1%s             |",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf+ strlen(buf),
			" || %sДСУ: %s%10ld%s    |",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch), CCCYN(ch, C_NRM));

	sprintf(buf+ strlen(buf),
		" %sУм: %2d(%2d)           %s|"
		" %sЗапоминание: %4d%s|"
		" %sРазум:     %3d%s   ||\r\n",
		CCICYN(ch, C_NRM), GET_INT(ch), GET_REAL_INT(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), GET_MANAREG(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, IMMUNITY_RESISTANCE), 75);
	sprintf(buf+ strlen(buf),
		" || %sДенег: %s%8d%s    |"
		" %sОбаяние: %2d(%2d)      %s|------------------|"
		" %sИммунитет: %3d%s   ||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_GOLD(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_CHA(ch), GET_REAL_CHA(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	sprintf(buf+ strlen(buf),
	        " || %sНа счету: %s%8ld%s |"
		" %sЖизнь: %4d(%4d)    %s|"
		" %sВоля:      %3d%s   |------------------||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_BANK_GOLD(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_HIT(ch), GET_REAL_MAX_HIT(ch), CCCYN(ch, C_NRM),
		CCGRN(ch, C_NRM), - GET_SAVE(ch, SAVING_WILL) - wis_app[GET_REAL_WIS(ch)].char_savings, CCCYN(ch, C_NRM)
		);

	if (!on_horse(ch))
	switch (GET_POS(ch)) {
	case POS_DEAD:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCIRED(ch, C_NRM), string("Вы МЕРТВЫ!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_MORTALLYW:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCIRED(ch, C_NRM), string("Вы умираете!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_INCAP:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCRED(ch, C_NRM), string("Вы без сознания.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_STUNNED:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCIYEL(ch, C_NRM), string("Вы в обмороке!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_SLEEPING:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCIGRN(ch, C_NRM), string("Вы спите.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_RESTING:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCGRN(ch, C_NRM), string("Вы отдыхаете.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_SITTING:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
			CCIGRN(ch, C_NRM), string("Вы сидите.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_FIGHTING:
		if (FIGHTING(ch))
			sprintf(buf+ strlen(buf), " || %s%-19s%s|",
				CCIRED(ch, C_NRM), string("Вы сражаетесь!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		else
			sprintf(buf+ strlen(buf), " || %s%-19s%s|",
				CCRED(ch, C_NRM), string("Вы машете кулаками.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	case POS_STANDING:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
				CCNRM(ch, C_NRM), string("Вы стоите.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	default:
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
				CCNRM(ch, C_NRM), string("You are floating..").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
		break;
	} else
		sprintf(buf+ strlen(buf), " || %s%-19s%s|",
				CCNRM(ch, C_NRM), string("Вы сидите верхом.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));

	sprintf(buf+ strlen(buf),
		" %sВыносл.: %3d(%3d)    %s|"
		" %sЗдоровье: %3d    %s|"
		" %sВосст. жизни:    %s||\r\n",
		CCICYN(ch, C_NRM), GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), CCCYN(ch, C_NRM),
		CCGRN(ch, C_NRM), - GET_SAVE(ch, SAVING_CRITICAL) - con_app[GET_REAL_CON(ch)].critic_saving, CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), CCCYN(ch, C_NRM));

	if (GET_COND(ch, FULL) == 0)
		sprintf(buf+ strlen(buf), " || %sГолоден: &Rугу :(%s    |", CCNRM(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf+ strlen(buf), " || %sГолоден: &gнет%s       |", CCNRM(ch, C_NRM), CCCYN(ch, C_NRM));

	if (IS_MANA_CASTER(ch)) 
		sprintf(buf + strlen(buf),
			" %sМагическая сила:%s     |", CCICYN(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                      |");
	

	sprintf(buf+ strlen(buf),
		" %sСтойкость: %3d   %s|"
		" %s %4d            %s||\r\n",
		CCGRN(ch, C_NRM), - GET_SAVE(ch, SAVING_STABILITY) - con_app[GET_REAL_CON(ch)].affect_saving,
		CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), GET_HITREG(ch), CCCYN(ch, C_NRM)
		);

	if (GET_COND(ch, THIRST) == 0)
		sprintf(buf+ strlen(buf), " || %sЖажда: &Rналивай!%s    |", CCNRM(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf+ strlen(buf), " || %sЖажда: &gнет%s         |", CCNRM(ch, C_NRM), CCCYN(ch, C_NRM));

	if (IS_MANA_CASTER(ch)) 
		sprintf(buf + strlen(buf),
			" %s%4d(%4d)           %s|",
			CCICYN(ch, C_NRM), GET_MANA_STORED(ch), GET_MAX_MANA(ch), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                      |");

	sprintf(buf+ strlen(buf),
		" %sРеакция:   %3d   %s|"
		" %sВосст. сил:      %s||\r\n",
		CCGRN(ch, C_NRM), - GET_SAVE(ch, SAVING_REFLEX) + dex_app[GET_REAL_DEX(ch)].reaction, CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), CCCYN(ch, C_NRM)
		);

	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED) {
		if (affected_by_spell(ch, SPELL_ABSTINENT))
			sprintf(buf+ strlen(buf), " || %sПохмелье.          %s|", CCIYEL(ch, C_NRM), CCCYN(ch, C_NRM));
		else
			sprintf(buf+ strlen(buf), " || %sВы пьяны.          %s|", CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));
	} else
		strcat(buf, " ||                    |"); 

	if (IS_MANA_CASTER(ch)) 
		sprintf(buf + strlen(buf),
			" %sВосстан.: %3d сек.   %s|",
			CCICYN(ch, C_NRM), mana_gain(ch), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                      |");

	sprintf(buf+ strlen(buf),
		" %sУдача:   %4d    %s|"	
		" %s %4d            %s||\r\n"
		" -------------------------------------------------------------------------------------\r\n",
		CCGRN(ch, C_NRM), GET_MORALE(ch), CCCYN(ch, C_NRM),		
		CCRED(ch, C_NRM), GET_MOVEREG(ch), CCCYN(ch, C_NRM));

	if (has_horse(ch, FALSE)) {
		if (on_horse(ch))
			sprintf(buf + strlen(buf),
			" %s|| %sВы верхом на %-67s%s||\r\n"
			" -------------------------------------------------------------------------------------\r\n",
			CCCYN(ch, C_NRM), CCIGRN(ch, C_NRM),
			(string(GET_PAD(get_horse(ch), 5))+ string(".")).substr(0, 67).c_str(), CCCYN(ch, C_NRM));
		else
			sprintf(buf + strlen(buf),
			" %s|| %sУ вас есть %-69s%s||\r\n"
			" -------------------------------------------------------------------------------------\r\n",
			CCCYN(ch, C_NRM), CCIGRN(ch, C_NRM),
			(string(GET_NAME(get_horse(ch))) + string(".")).substr(0, 69).c_str(), CCCYN(ch, C_NRM));
	}

	if (GET_GLORY(ch))		
		sprintf(buf + strlen(buf),
			" %s|| %sВы заслужили %5d %-61s%s||\r\n",
			CCCYN(ch, C_NRM), CCWHT(ch, C_NRM), GET_GLORY(ch),
			(string(desc_count(GET_GLORY(ch), WHAT_POINT)) + string(" славы.")).substr(0, 61).c_str(),
			CCCYN(ch, C_NRM));

	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		sprintf(buf + strlen(buf),
		        " || Вы можете быть призваны.                                                        ||\r\n");
	else
		sprintf(buf + strlen(buf),
		        " || Вы защищены от призыва.                                                         ||\r\n");
	
	if (!NAME_GOD(ch) && GET_LEVEL(ch) <= NAME_LEVEL) {
		sprintf(buf + strlen(buf),
			" &c|| &RВНИМАНИЕ!&n Ваше имя не одобрил никто из богов!&c                                   ||\r\n");
		sprintf(buf + strlen(buf),
			" || &nCкоро вы прекратите получать опыт, обратитесь к богам для одобрения имени.      &c||\r\n");
	} else if (NAME_BAD(ch)) {
		sprintf(buf + strlen(buf),
			" || &RВНИМАНИЕ!&n Ваше имя запрещено богами. Очень скоро вы прекратите получать опыт.   &c||\r\n");
	}

	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
			" || %sВы можете вступить в группу с максимальной разницей                             %s||\r\n"
			" || %sв %d %-76s%s||\r\n",
			CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
			grouping[(int)GET_CLASS(ch)][MIN(14, (int)GET_REMORT(ch))],
			(string(desc_count(grouping[(int)GET_CLASS(ch)][MIN(14, (int)GET_REMORT(ch))], WHAT_LEVEL))
			+ string(" без потерь для опыта.")).substr(0, 76).c_str(), CCCYN(ch, C_NRM));

	if (RENTABLE(ch)) 
		sprintf(buf + strlen(buf),
			" || %sВ связи с боевыми действиями Вы не можете уйти на постой.                       %s||\r\n",
			CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	else if ((IN_ROOM(ch) != NOWHERE) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER)) 
		sprintf(buf + strlen(buf),
			" || %sТут Вы чувствуете себя в безопасности.                                          %s||\r\n",
			CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (has_mail(GET_IDNUM(ch)))
		sprintf(buf + strlen(buf),
			" || %sВас ожидает новое письмо, зайдите на почту                                      %s||\r\n",
			CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch)) {
		int hrs = (GCURSE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((GCURSE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %sВы прокляты Богами на %3d %-5s %2d %-45s%s||\r\n",
			CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 45).c_str(), CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(NULL)) {
		int hrs = (HELL_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %sВам предстоит провести в темнице еще %6d %-5s %2d %-27s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 27).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(string(HELL_REASON(ch) ? HELL_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
 	}

	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(NULL)) {
		int hrs = (MUTE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %sВы не сможете кричать еще %6d %-5s %2d %-38s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 38).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(string(MUTE_REASON(ch) ? MUTE_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
 	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(NULL)) {
		int hrs = (UNREG_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((UNREG_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %sВы не сможете входить с одного IP еще %6d %-5s %2d %-38s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 38).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(string(UNREG_REASON(ch) ? UNREG_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
 	}

	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(NULL)) {
		int hrs = (DUMB_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %sВы будете молчать еще %6d %-5s %2d %-42s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 42).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(string(DUMB_REASON(ch) ? DUMB_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
 	}

	strcat(buf, " ||                                                                                 ||\r\n");	
	strcat(buf, " -------------------------------------------------------------------------------------\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	return;
  	}

/**********************************************/

	sprintf(buf, "Вы %s (%s, %s, %s, %s %d уровня).\r\n",
		only_title(ch),
		kin_name[GET_KIN (ch)][(int) GET_SEX (ch)],
		race_name[GET_RACE(ch)][(int) GET_SEX(ch)],
		religion_name[GET_RELIGION(ch)][(int) GET_SEX(ch)], 
		class_name[(int) GET_CLASS (ch)+14*GET_KIN (ch)],GET_LEVEL (ch));

	if (!NAME_GOD(ch) && GET_LEVEL(ch) <= NAME_LEVEL) {
		sprintf(buf + strlen(buf), "\r\n&RВНИМАНИЕ!&n Ваше имя не одобрил никто из богов!\r\n");
		sprintf(buf + strlen(buf), "Очень скоро вы прекратите получать опыт,\r\n");
		sprintf(buf + strlen(buf), "обратитесь к богам для одобрения имени.\r\n\r\n");
	} else if (NAME_BAD(ch)) {
		sprintf(buf + strlen(buf), "\r\n&RВНИМАНИЕ!&n Ваше имя запрещено богами.\r\n");
		sprintf(buf + strlen(buf), "Очень скоро вы прекратите получать опыт.\r\n\r\n");
	}

	sprintf(buf + strlen(buf), "Сейчас Вам %d %s. ", GET_REAL_AGE(ch), desc_count(GET_REAL_AGE(ch), WHAT_YEAR));

	if (age(ch)->month == 0 && age(ch)->day == 0) {
		sprintf(buf2, "%sУ Вас сегодня День Варенья !%s\r\n", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	} else
		strcat(buf, "\r\n");

	sprintf(buf + strlen(buf),
		"Вы можете выдержать %d(%d) %s повреждения, и пройти %d(%d) %s по ровной местности.\r\n",
		GET_HIT(ch), GET_REAL_MAX_HIT(ch), desc_count(GET_HIT(ch),
							      WHAT_ONEu),
		GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), desc_count(GET_MOVE(ch), WHAT_MOVEu));

	if (IS_MANA_CASTER(ch)) {
		sprintf(buf + strlen(buf),
			"Ваша магическая энергия %d(%d) и Вы восстанавливаете %d в сек.\r\n",
			GET_MANA_STORED(ch), GET_MAX_MANA(ch), mana_gain(ch));
	}

	if (GET_LEVEL(ch) > 4) {
		sprintf(buf + strlen(buf),
			"%sВаши характеристики :\r\n"
			"  Сила : %2d(%2d)"
			"  Подв : %2d(%2d)"
			"  Тело : %2d(%2d)"
			"  Мудр : %2d(%2d)"
			"  Ум   : %2d(%2d)"
			"  Обаян: %2d(%2d)\r\n"
			"  Размер %3d(%3d)"
			"  Рост   %3d(%3d)"
			"  Вес    %3d(%3d)%s\r\n",
			CCICYN(ch, C_NRM), GET_STR(ch), GET_REAL_STR(ch),
			GET_DEX(ch), GET_REAL_DEX(ch),
			GET_CON(ch), GET_REAL_CON(ch),
			GET_WIS(ch), GET_REAL_WIS(ch),
			GET_INT(ch), GET_REAL_INT(ch),
			GET_CHA(ch), GET_REAL_CHA(ch),
			GET_SIZE(ch), GET_REAL_SIZE(ch),
			GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCNRM(ch, C_NRM));
	}
	if (IS_IMMORTAL(ch)) {
		sprintf(buf + strlen(buf),
			"%sВаши боевые качества :\r\n"
			"  AC   : %4d(%4d)"
			"  DR   : %4d(%4d)%s\r\n",
			CCIGRN(ch, C_NRM), GET_AC(ch), compute_armor_class(ch),
			GET_DR(ch), GET_REAL_DR(ch), CCNRM(ch, C_NRM));
	} else {
		if (GET_LEVEL(ch) > 4) {
			ac = compute_armor_class(ch) / 10;
			ac_t = MAX(MIN(ac + 30, 40), 0);
			sprintf(buf + strlen(buf), "&GВаши боевые качества :\r\n"
				"  Защита  (AC)     : %4d - %s&G\r\n"
				"  Броня/Поглощение : %4d/%d&n\r\n",
				ac, ac_text[ac_t], GET_ARMOUR(ch), GET_ABSORBE(ch));
		}
	}
/*  if (charm_points(ch) > 0) {
     sprintf(buf + strlen(buf),  " Пунктов лояльности: %4d\r\n",
             charm_points(ch));
     sprintf(buf + strlen(buf),  " Из них свободно   : %4d\r\n",
             charm_points(ch) - used_charm_points(ch));
  } */
	sprintf(buf + strlen(buf), "Ваш опыт - %ld %s, у Вас на руках %d %s",
		GET_EXP(ch), desc_count(GET_EXP(ch), WHAT_POINT), GET_GOLD(ch), desc_count(GET_GOLD(ch), WHAT_MONEYa));
	if (GET_BANK_GOLD(ch) > 0)
		sprintf(buf + strlen(buf), "(и еще %ld %s припрятано в лежне).\r\n",
			GET_BANK_GOLD(ch), desc_count(GET_BANK_GOLD(ch), WHAT_MONEYa));
	else
		strcat(buf, ".\r\n");

	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
			"Вам осталось набрать %ld %s до следующего уровня.\r\n",
			level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch),
			desc_count(level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch), WHAT_POINT));
	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
			"Вы можете вступить в группу с максимальной разницей в %d %s без потерь для опыта.\r\n",
			grouping[(int)GET_CLASS(ch)][MIN(14, (int)GET_REMORT(ch))],
			desc_count(grouping[(int)GET_CLASS(ch)][MIN(14, (int)GET_REMORT(ch))], WHAT_LEVEL));
		
//if (GET_GOD_FLAG(ch, GF_REMORT))
//   sprintf(buf+strlen(buf),"Вы имеете право на перевоплощение.\r\n");
	if (GET_GLORY(ch))
		sprintf(buf + strlen(buf), "Вы заслужили %d %s славы.\r\n",
			GET_GLORY(ch), desc_count(GET_GLORY(ch), WHAT_POINT));
	playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
	sprintf(buf + strlen(buf), "Вы играете %d %s %d %s реального времени\r\n",
		playing_time.day, desc_count(playing_time.day, WHAT_DAY),
		playing_time.hours, desc_count(playing_time.hours, WHAT_HOUR));

	if (!on_horse(ch))
		switch (GET_POS(ch)) {
		case POS_DEAD:
			strcat(buf, "Вы МЕРТВЫ!\r\n");
			break;
		case POS_MORTALLYW:
			strcat(buf, "Вы смертельно ранены и нуждаетесь в помощи!\r\n");
			break;
		case POS_INCAP:
			strcat(buf, "Вы без сознания и медленно умираете...\r\n");
			break;
		case POS_STUNNED:
			strcat(buf, "Вы в обмороке!\r\n");
			break;
		case POS_SLEEPING:
			strcat(buf, "Вы спите.\r\n");
			break;
		case POS_RESTING:
			strcat(buf, "Вы отдыхаете.\r\n");
			break;
		case POS_SITTING:
			strcat(buf, "Вы сидите.\r\n");
			break;
		case POS_FIGHTING:
			if (FIGHTING(ch))
				sprintf(buf + strlen(buf), "Вы сражаетесь с %s.\r\n", GET_PAD(FIGHTING(ch), 4));
			else
				strcat(buf, "Вы машете кулаками по воздуху.\r\n");
			break;
		case POS_STANDING:
			strcat(buf, "Вы стоите.\r\n");
			break;
		default:
			strcat(buf, "You are floating.\r\n");
			break;
		}
	send_to_char(buf, ch);

	strcpy(buf, CCIGRN(ch, C_NRM));
	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED) {
		if (affected_by_spell(ch, SPELL_ABSTINENT))
			strcat(buf, "Привет с большого бодуна !\r\n");
		else
			strcat(buf, "Вы пьяны.\r\n");
	}
	if (GET_COND(ch, FULL) == 0)
		strcat(buf, "Вы голодны.\r\n");
	if (GET_COND(ch, THIRST) == 0)
		strcat(buf, "Вас мучает жажда.\r\n");
	/*
	   strcat(buf, CCICYN(ch, C_NRM));
	   strcat(buf,"Аффекты :\r\n");
	   sprintbits((ch)->char_specials.saved.affected_by, affected_bits, buf2, "\r\n");
	   strcat(buf,buf2);
	 */
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		strcat(buf, "Вы можете быть призваны.\r\n");

	if (has_horse(ch, FALSE)) {
		if (on_horse(ch))
			sprintf(buf + strlen(buf), "Вы верхом на %s.\r\n", GET_PAD(get_horse(ch), 5));
		else
			sprintf(buf + strlen(buf), "У Вас есть %s.\r\n", GET_NAME(get_horse(ch)));
	}
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	if (RENTABLE(ch)) {
		sprintf(buf,
			"%sВ связи с боевыми действиями Вы не можете уйти на постой.%s\r\n",
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	} else if ((IN_ROOM(ch) != NOWHERE) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER)) {
		sprintf(buf, "%sТут Вы чувствуете себя в безопасности.%s\r\n", CCIGRN(ch, C_NRM), CCINRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (has_mail(GET_IDNUM(ch))) {
		sprintf(buf, "%sВас ожидает новое письмо, зайдите на почту!%s\r\n", CCIGRN(ch, C_NRM), CCINRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(NULL)) {
		int hrs = (HELL_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf,
			"Вам предстоит провести в темнице еще %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins,
									  WHAT_MINu),
			HELL_REASON(ch) ? HELL_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(NULL)) {
		int hrs = (MUTE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете кричать еще %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), MUTE_REASON(ch) ? MUTE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(NULL)) {
		int hrs = (DUMB_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете молчать еще %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), DUMB_REASON(ch) ? DUMB_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(NULL)) {
		int hrs = (UNREG_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((UNREG_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете заходить с одного IP еще %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), UNREG_REASON(ch) ? UNREG_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch)) {
		int hrs = (GCURSE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((GCURSE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы прокляты Богами на %d %s %d %s.\r\n",
			hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins, WHAT_MINu));
		send_to_char(buf, ch);
	}
}


ACMD(do_inventory)
{
	send_to_char("Вы несете:\r\n", ch);
	list_obj_to_char(ch->carrying, ch, 1, 2);
}


ACMD(do_equipment)
{
	int i, found = 0;
	skip_spaces(&argument);

	send_to_char("На Вас надето:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)) {
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
				send_to_char(where[i], ch);
				show_obj_to_char(GET_EQ(ch, i), ch, 1, TRUE, 1);
				found = TRUE;
			} else {
				send_to_char(where[i], ch);
				send_to_char("что-то.\r\n", ch);
				found = TRUE;
			}
		} else		// added by Pereplut
		{
			if (is_abbrev(argument, "все") || is_abbrev(argument, "all")) {
				send_to_char(where[i], ch);
//           send_to_char("&K[ Ничего ]&n\r\n", ch);
				sprintf(buf, "%s[ Ничего ]%s\r\n", CCINRM(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				found = TRUE;
			}
		}
	}
	if (!found) {
		if (IS_FEMALE(ch))
			send_to_char("Костюм Евы Вам очень идет :)\r\n", ch);
		else
			send_to_char(" Вы голы, аки сокол.\r\n", ch);
	}
}


ACMD(do_time)
{
	int day, month, days_go;
	if (IS_NPC(ch))
		return;
	sprintf(buf, "Сейчас ");
	switch (time_info.hours % 24) {
	case 0:
		sprintf(buf + strlen(buf), "полночь, ");
		break;
	case 1:
		sprintf(buf + strlen(buf), "1 час ночи, ");
		break;
	case 2:
	case 3:
	case 4:
		sprintf(buf + strlen(buf), "%d часа ночи, ", time_info.hours);
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		sprintf(buf + strlen(buf), "%d часов утра, ", time_info.hours);
		break;
	case 12:
		sprintf(buf + strlen(buf), "полдень, ");
		break;
	case 13:
		sprintf(buf + strlen(buf), "1 час пополудни, ");
		break;
	case 14:
	case 15:
	case 16:
		sprintf(buf + strlen(buf), "%d часа пополудни, ", time_info.hours - 12);
		break;
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
		sprintf(buf + strlen(buf), "%d часов вечера, ", time_info.hours - 12);
		break;
	}

	if (GET_RELIGION(ch) == RELIGION_POLY)
		strcat(buf, weekdays_poly[weather_info.week_day_poly]);
	else
		strcat(buf, weekdays[weather_info.week_day_mono]);
	switch (weather_info.sunlight) {
	case SUN_DARK:
		strcat(buf, ", ночь");
		break;
	case SUN_SET:
		strcat(buf, ", закат");
		break;
	case SUN_LIGHT:
		strcat(buf, ", день");
		break;
	case SUN_RISE:
		strcat(buf, ", рассвет");
		break;
	}
	strcat(buf, ".\r\n");
	send_to_char(buf, ch);

	day = time_info.day + 1;	/* day in [1..35] */
	*buf = '\0';
	if (GET_RELIGION(ch) == RELIGION_POLY || IS_IMMORTAL(ch)) {
		days_go = time_info.month * DAYS_PER_MONTH + time_info.day;
		month = days_go / 40;
		days_go = (days_go % 40) + 1;
		sprintf(buf + strlen(buf), "%s, %dй День, Год %d%s",
			month_name_poly[month], days_go, time_info.year, IS_IMMORTAL(ch) ? ".\r\n" : "");
	}
	if (GET_RELIGION(ch) == RELIGION_MONO || IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), "%s, %dй День, Год %d",
			month_name[(int) time_info.month], day, time_info.year);
	switch (weather_info.season) {
	case SEASON_WINTER:
		strcat(buf, ", зима");
		break;
	case SEASON_SPRING:
		strcat(buf, ", весна");
		break;
	case SEASON_SUMMER:
		strcat(buf, ", лето");
		break;
	case SEASON_AUTUMN:
		strcat(buf, ", осень");
		break;
	}
	strcat(buf, ".\r\n");
	send_to_char(buf, ch);
	gods_day_now(ch);
}

int get_moon(int sky)
{
	if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT || sky == SKY_RAINING)
		return (0);
	else if (weather_info.moon_day <= NEWMOONSTOP || weather_info.moon_day >= NEWMOONSTART)
		return (1);
	else if (weather_info.moon_day < HALFMOONSTART)
		return (2);
	else if (weather_info.moon_day < FULLMOONSTART)
		return (3);
	else if (weather_info.moon_day <= FULLMOONSTOP)
		return (4);
	else if (weather_info.moon_day < LASTHALFMOONSTART)
		return (5);
	else
		return (6);
	return (0);
}



ACMD(do_weather)
{
	int sky = weather_info.sky, weather_type = weather_info.weather_type;
	const char *sky_look[] = { "облачное",
		"пасмурное",
		"покрыто тяжелыми тучами",
		"ясное"
	};
	const char *moon_look[] = { "Новолуние.",
		"Растущий серп луны.",
		"Растущая луна.",
		"Полнолуние.",
		"Убывающая луна.",
		"Убывающий серп луны."
	};

	if (OUTSIDE(ch)) {
		*buf = '\0';
		if (world[IN_ROOM(ch)]->weather.duration > 0) {
			sky = world[IN_ROOM(ch)]->weather.sky;
			weather_type = world[IN_ROOM(ch)]->weather.weather_type;
		}
		sprintf(buf + strlen(buf),
			"Небо %s. %s\r\n%s\r\n", sky_look[sky],
			get_moon(sky) ? moon_look[get_moon(sky) - 1] : "",
			(weather_info.change >=
			 0 ? "Атмосферное давление повышается." : "Атмосферное давление понижается."));
		sprintf(buf + strlen(buf), "На дворе %d %s.\r\n",
			weather_info.temperature, desc_count(weather_info.temperature, WHAT_DEGREE));

		if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND))
			strcat(buf, "Сильный ветер.\r\n");
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMWIND))
			strcat(buf, "Умеренный ветер.\r\n");
		else if (IS_SET(weather_info.weather_type, WEATHER_LIGHTWIND))
			strcat(buf, "Легкий ветерок.\r\n");

		if (IS_SET(weather_type, WEATHER_BIGSNOW))
			strcat(buf, "Валит снег.\r\n");
		else if (IS_SET(weather_type, WEATHER_MEDIUMSNOW))
			strcat(buf, "Снегопад.\r\n");
		else if (IS_SET(weather_type, WEATHER_LIGHTSNOW))
			strcat(buf, "Легкий снежок.\r\n");

		if (IS_SET(weather_type, WEATHER_GRAD))
			strcat(buf, "Дождь с градом.\r\n");
		else if (IS_SET(weather_type, WEATHER_BIGRAIN))
			strcat(buf, "Льет, как из ведра.\r\n");
		else if (IS_SET(weather_type, WEATHER_MEDIUMRAIN))
			strcat(buf, "Идет дождь.\r\n");
		else if (IS_SET(weather_type, WEATHER_LIGHTRAIN))
			strcat(buf, "Моросит дождик.\r\n");

		send_to_char(buf, ch);
	} else
		send_to_char("Вы ничего не можете сказать о погоде сегодня.\r\n", ch);
	if (IS_GOD(ch)) {
		sprintf(buf, "День: %d Месяц: %s Час: %d Такт = %d\r\n"
			"Температура =%-5d, за день = %-8d, за неделю = %-8d\r\n"
			"Давление    =%-5d, за день = %-8d, за неделю = %-8d\r\n"
			"Выпало дождя = %d(%d), снега = %d(%d). Лед = %d(%d). Погода = %08x(%08x).\r\n",
			time_info.day, month_name[time_info.month], time_info.hours,
			weather_info.hours_go, weather_info.temperature,
			weather_info.temp_last_day, weather_info.temp_last_week,
			weather_info.pressure, weather_info.press_last_day,
			weather_info.press_last_week, weather_info.rainlevel,
			world[IN_ROOM(ch)]->weather.rainlevel, weather_info.snowlevel,
			world[IN_ROOM(ch)]->weather.snowlevel, weather_info.icelevel,
			world[IN_ROOM(ch)]->weather.icelevel,
			weather_info.weather_type, world[IN_ROOM(ch)]->weather.weather_type);
		send_to_char(buf, ch);
	}
}


ACMD(do_index)
{
	int i;
	int row = 0;
	int minlen;
	int trust_level = 0;
	if (!ch->desc)
		return;

	*buf = '\0';

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char("Использование: разделы <буква|фраза>\r\n", ch);
		return;
	}

	minlen = strlen(argument);

	/* Персонажам с флагом gf_demigod позволяется
	   читать справку до уровня LVL_IMMORT включительно  */
	if (GET_GOD_FLAG(ch, GF_DEMIGOD))
		trust_level = LVL_IMMORT;
	else
		trust_level = GET_LEVEL(ch);;

	for (i = 0; i < top_of_helpt; i++) {
		if (!strn_cmp(argument, help_table[i].keyword, minlen) && trust_level >= help_table[i].min_level) {
			row++;
			sprintf(buf + strlen(buf), "|%-23.23s |", help_table[i].keyword);
			if ((row % 3) == 0)
				strcat(buf, "\r\n");
		}
	}
	if (row > 0) {
		send_to_char("Найдены следующие разделы справки:\r\n", ch);
		if ((row % 3) != 0)
			strcat(buf, "\r\n");
	} else
		send_to_char("Разделы не найдены.\r\n", ch);

	if (ch->desc)
		page_string(ch->desc, buf, 1);

}

ACMD(do_help)
{
	int chk, bot, top, mid, minlen, strong = FALSE;
	int topic_num = 0;	// для организации поиска раздела с указанием индекса
	int trust_level = 0;

	if (!ch->desc)
		return;

	skip_spaces(&argument);

	if (!*argument) {
		page_string(ch->desc, help, 0);
		return;
	}
	if (!help_table) {
		send_to_char("Помощь недоступна.\r\n", ch);
		return;
	}

	sprintf(buf, "%s uses command HELP: %s", GET_NAME(ch), argument);
	mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);

	bot = 0;
	top = top_of_helpt;

	/* Персонажам с флагом gf_demigod позволяется
	   читать справку до уровня LVL_IMMORT включительно  */
	if (GET_GOD_FLAG(ch, GF_DEMIGOD))
		trust_level = LVL_IMMORT;
	else
		trust_level = GET_LEVEL(ch);;

	sscanf(argument, "%d.%s", &topic_num, argument);	// включаем возможность индексации топика

	if (strlen(argument) > 1 && *(argument + strlen(argument) - 1) == '!') {
		strong = TRUE;
		*(argument + strlen(argument) - 1) = '\0';
	}
	minlen = strlen(argument);
	for (;;) {
		mid = (bot + top) / 2;

		if (bot > top) {
			send_to_char("Нет справки по выбранной теме.\r\n", ch);
			sprintf(buf, "%s не нашел справку по команде: %s.", GET_NAME(ch), argument);
			mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
			return;
		} else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen)) &&
			   trust_level >= help_table[mid].min_level) {
			/* Находим начальный раздел, соответствующий нашему шаблону */
			while (mid > 0) {
				if (!strn_cmp(argument, help_table[mid - 1].keyword, minlen))
					mid--;
				else
					break;
			}

			/* Если найденный топик не для нашего уровня - ищем следующий подходящий */
			while (trust_level < help_table[mid].min_level)
				mid++;

			if (strong) {	/* если поиск с указанной длинной строки */
				while (mid < top
				       && trust_level > help_table[mid + 1].min_level
				       && !strn_cmp(argument, help_table[mid + 1].keyword, minlen)
				       && *(help_table[mid].keyword + minlen)) {
					mid++;
				}
			} else {
				/* если мы указали индекс раздела */
				while (topic_num > 1) {
					if (!strn_cmp(argument, help_table[mid + 1].keyword, minlen)) {
						if (trust_level >= help_table[mid + 1].min_level)
							topic_num--;
						mid++;
					} else
						topic_num = 0;
				}
				while (trust_level < help_table[mid].min_level)
					mid--;
			}

			page_string(ch->desc, help_table[mid].entry, 0);
			return;
		} else {
			if (chk > 0)
				bot = mid + 1;
			else
				top = mid - 1;
		}
	}
}


#define IMM_WHO_FORMAT \
"Формат: кто [минуров[-максуров]] [-n имя] [-c профлист] [-s] [-r] [-z] [-h] [-b|-и]\r\n"

#define MORT_WHO_FORMAT \
"Формат: кто [имя] [-?]\r\n"

ACMD(do_who)
{
//  DESCRIPTOR_DATA *d;
	CHAR_DATA *tch;
	char name_search[MAX_INPUT_LENGTH] = "\0";

	/* Строки содержащие имена  */
	char *imms = NULL;
	char *morts = NULL;
	char *demigods = NULL;
	char *buffer = NULL;

	char mode;
	size_t i;
	/* Флаги для опций  */
	int low = 0, high = LVL_IMPL, localwho = 0;
	int showclass = 0, short_list = 0, num_can_see = 0;
	int who_room = 0, imms_num = 0, morts_num = 0, demigods_num = 0;
	int showname = 0;

// Добавлено Дажьбогом
//  int    count_pk=0;
	char name_who[MAX_STRING_LENGTH] = "\0";
//

	skip_spaces(&argument);
	strcpy(buf, argument);
	name_search[0] = '\0';

	/* Проверка аргументов команды "кто" */
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (!str_cmp(arg, "боги") && strlen(arg) == 4) {
			low = LVL_IMMORT;
			high = LVL_IMPL;
			strcpy(buf, buf1);
		} else if (a_isdigit(*arg)) {
			if (IS_GOD(ch))
				sscanf(arg, "%d-%d", &low, &high);
			strcpy(buf, buf1);
		} else if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
			switch (mode) {
			case 'b':
			case 'и':
				if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_DEMIGOD))
					showname = 1;
				strcpy(buf, buf1);
				break;
			case 'z':
				if (IS_GOD(ch))
					localwho = 1;
				strcpy(buf, buf1);
				break;
			case 's':
				short_list = 1;
				strcpy(buf, buf1);
				break;
			case 'l':
				half_chop(buf1, arg, buf);
				if (IS_GOD(ch))
					sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				half_chop(buf1, name_search, buf);
				break;
			case 'r':
				if (IS_GOD(ch))
					who_room = 1;
				strcpy(buf, buf1);
				break;
			case 'c':

				half_chop(buf1, arg, buf);
				if (IS_GOD(ch))
					for (i = 0; i < strlen(arg); i++)
						showclass |= find_class_bitvector(arg[i]);
				break;
			case 'h':
			case '?':
			default:
				if (IS_IMMORTAL(ch))
					send_to_char(IMM_WHO_FORMAT, ch);
				else
					send_to_char(MORT_WHO_FORMAT, ch);
				return;
			}	/* end of switch */
		} else {	/* endif */
			strcpy(name_search, arg);
			strcpy(buf, buf1);

		}
	}			/* end while (parser) */

	/* Первоначальное заполнение строк imms, morts, demigods  */
	sprintf(buf, "%sБОГИ%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	imms = str_add(imms, buf);
	sprintf(buf, "%sПривилегированные%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	demigods = str_add(demigods, buf);
	sprintf(buf, "%sИгроки%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	morts = str_add(morts, buf);


	for (tch = character_list; tch; tch = tch->next) {
		if (IS_NPC(tch))
			continue;

		if (!HERE(tch))
			continue;

		if (*name_search &&
		    !(isname(name_search, GET_NAME(tch)) ||
		      (only_title(tch) && strstr(only_title(tch), name_search))))

			continue;
		if (!CAN_SEE_CHAR(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
			continue;
// Изменено Дажьбогом
//       if (outlaws && !IS_KILLER(tch))
//          continue;
		if (localwho && world[ch->in_room]->zone != world[tch->in_room]->zone)
			continue;
		if (who_room && (tch->in_room != ch->in_room))
			continue;
		if (showclass && !(showclass & (1 << GET_CLASS(tch))))
			continue;
		if (showname && !(!NAME_GOD(tch) && GET_LEVEL(tch) <= NAME_LEVEL))
			continue;
		*buf = '\0';
		num_can_see++;

		*name_who = '\0';
		if (!short_list)
			sprintf(name_who, "%s%s%s", CCPK(ch, C_NRM, tch), race_or_title(tch), CCNRM(ch, C_NRM));
		else
			sprintf(name_who, "%s%s%s", CCPK(ch, C_NRM, tch), GET_NAME(tch), CCNRM(ch, C_NRM));

		if (short_list) {
			if (IS_IMPL(ch))
				sprintf(buf, "%s[%2d %s %s] %-20.15s%s",
					IS_GOD(tch) ? CCWHT(ch, C_SPR) : "",
					GET_LEVEL(tch), KIN_ABBR (tch), CLASS_ABBR(tch), name_who, IS_GOD(tch) ? CCNRM(ch, C_SPR) : "");
			else
				sprintf(buf, "%s      %-20.15s%s",
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
					name_who, IS_IMMORTAL(tch) ? CCNRM(ch, C_SPR) : "");
		} else {
			if (IS_IMPL(ch))
				sprintf(buf, "%s[%2d %s %s(%3d)] %s%s%s%s",
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
					GET_LEVEL(tch),
					KIN_ABBR (tch),
					CLASS_ABBR(tch),
					GET_PFILEPOS(tch),
					CCPK(ch, C_NRM, tch),
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", race_or_title(tch), CCNRM(ch, C_NRM));
			else
				sprintf(buf, "%s %s%s%s",
					CCPK(ch, C_NRM, tch),
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", race_or_title(tch), CCNRM(ch, C_NRM));

			if (GET_INVIS_LEV(tch))
				sprintf(buf + strlen(buf), " (i%d)", GET_INVIS_LEV(tch));
			else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
				sprintf(buf + strlen(buf), " (невидим%s)", GET_CH_SUF_6(tch));
			if (AFF_FLAGGED(tch, AFF_HIDE))
				strcat(buf, " (прячется)");
			if (AFF_FLAGGED(tch, AFF_CAMOUFLAGE))
				strcat(buf, " (маскируется)");

			if (PLR_FLAGGED(tch, PLR_MAILING))
				strcat(buf, " (отправляет письмо)");
			else if (PLR_FLAGGED(tch, PLR_WRITING))
				strcat(buf, " (пишет)");

			if (PRF_FLAGGED(tch, PRF_NOHOLLER))
				sprintf(buf + strlen(buf), " (глух%s)", GET_CH_SUF_1(tch));
			if (PRF_FLAGGED(tch, PRF_NOTELL))
				sprintf(buf + strlen(buf), " (занят%s)", GET_CH_SUF_6(tch));
			if (PLR_FLAGGED(tch, PLR_MUTE))
				sprintf(buf + strlen(buf), " (молчит)");
			if (PLR_FLAGGED(tch, PLR_DUMB))
				sprintf(buf + strlen(buf), " (нем%s)", GET_CH_SUF_6(tch));
			if (PLR_FLAGGED(tch, PLR_KILLER) == PLR_KILLER)
				sprintf(buf + strlen(buf), "&R (ДУШЕГУБ)&n");
			if (IS_IMMORTAL(ch) && !NAME_GOD(tch)
			    && GET_LEVEL(tch) <= NAME_LEVEL) {
				sprintf(buf + strlen(buf), " &W!НЕ ОДОБРЕНО!&n");
				if (showname) {
					sprintf(buf + strlen(buf),
						"\r\nПадежи: %s/%s/%s/%s/%s/%s Email: %s Пол: %s",
						GET_PAD(tch, 0), GET_PAD(tch, 1), GET_PAD(tch, 2),
						GET_PAD(tch, 3), GET_PAD(tch, 4), GET_PAD(tch, 5), GET_EMAIL(tch),
						genders[(int)GET_SEX(tch)]);
				}
			} else if (IS_IMMORTAL(ch) && NAME_BAD(tch)) {
				sprintf(buf + strlen(buf), " &Wзапрет %s!&n", get_name_by_id(NAME_ID_GOD(tch)));
			}
			if (IS_IMMORTAL(tch))
				strcat(buf, CCNRM(ch, C_SPR));
		}		/* endif shortlist */

		if (IS_IMMORTAL(tch)) {
			imms_num++;
			imms = str_add(imms, buf);
			if (!short_list || !(imms_num % 4))
				imms = str_add(imms, "\r\n");
		} else if (GET_GOD_FLAG(tch, GF_DEMIGOD) && IS_IMMORTAL(ch)) {
			demigods_num++;
			demigods = str_add(demigods, buf);
			if (!short_list || !(demigods_num % 4))
				demigods = str_add(demigods, "\r\n");
		} else {
			morts_num++;
			morts = str_add(morts, buf);
			if (!short_list || !(morts_num % 4))
				morts = str_add(morts, "\r\n");
		}
	}			/* end of for */

	if (morts_num + imms_num + demigods_num == 0) {
		send_to_char("\r\nВы никого не видите.\r\n", ch);
		// !!!
		return;
	}

	if (short_list) {
		if (imms_num > 0)
			buffer = str_add(buffer, imms);
		if (demigods_num > 0) {
			buffer = str_add(buffer, "\r\n");
			buffer = str_add(buffer, demigods);
		}
		if (morts_num > 0) {
			buffer = str_add(buffer, "\r\n");
			buffer = str_add(buffer, morts);
		}
	} else {
		if (imms_num > 0)
			buffer = str_add(buffer, imms);
		if (demigods_num > 0)
			buffer = str_add(buffer, demigods);
		if (morts_num > 0)
			buffer = str_add(buffer, morts);
	}

	buffer = str_add(buffer, "\r\nВсего видимых:");

	if (imms_num) {
		sprintf(buf, " бессмертных %d", imms_num);
		buffer = str_add(buffer, buf);
	}
	if (demigods_num) {
		sprintf(buf, " привилегированных %d", demigods_num);
		buffer = str_add(buffer, buf);
	}
	if (morts_num) {
		sprintf(buf, " смертных %d", morts_num);
		buffer = str_add(buffer, buf);
	}

	buffer = str_add(buffer, ".\r\n");
	page_string(ch->desc, buffer, 1);
	free(buffer);
	free(imms);
	free(demigods);
	free(morts);
}

ACMD(do_who_new)
{
//  if (!GET_GOD_FLAG(ch,GF_DEMIGOD) && !IS_IMMORTAL(ch))
//  {    
//    send_to_char("Чаво ? \n",ch);
//    return;
//  }
	CHAR_DATA *tch;

	char name_search[MAX_INPUT_LENGTH] = "\0";
	//imms[MAX_STRING_LENGTH],
	//morts[MAX_STRING_LENGTH];
	char *imms = NULL;
	char *morts = NULL;
	char *buffer = NULL;

	int low = 0, high = LVL_IMPL;
	int num_can_see = 0;
	int imms_num = 0, morts_num = 0;
// Добавлено Дажьбогом
	char name_who[MAX_STRING_LENGTH] = "\0";

	skip_spaces(&argument);
	strcpy(buf, argument);
	name_search[0] = '\0';

	half_chop(buf, arg, buf1);
	strcpy(buf, buf1);
	sprintf(buf, "%sБОГИ%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	imms = str_add(imms, buf);
	sprintf(buf, "%sИгроки%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	morts = str_add(morts, buf);

	for (tch = character_list; tch; tch = tch->next) {
		if (IS_NPC(tch))
			continue;

		if (!HERE(tch))
			continue;

		if (!CAN_SEE_CHAR(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
			continue;
		if (!(!NAME_GOD(tch) && GET_LEVEL(tch) <= NAME_LEVEL))
			continue;
		*buf = '\0';
		num_can_see++;

// Добавлено Дажьбогом
		*name_who = '\0';
		sprintf(name_who, "%s%s%s", CCPK(ch, C_NRM, tch), race_or_title(tch), CCNRM(ch, C_NRM));


//      {
		if (IS_IMPL(ch))
			sprintf(buf, "%s[%2d %s %s(%3d)] %s%s%s%s",
				IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
				GET_LEVEL(tch),
				KIN_ABBR (tch),
				CLASS_ABBR(tch),
				GET_PFILEPOS(tch),
				CCPK(ch, C_NRM, tch),
				IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", race_or_title(tch), CCNRM(ch, C_NRM));
		else
			sprintf(buf, "%s %s%s%s",
				CCPK(ch, C_NRM, tch),
				IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", race_or_title(tch), CCNRM(ch, C_NRM));

		if (GET_INVIS_LEV(tch))
			sprintf(buf + strlen(buf), " (i%d)", GET_INVIS_LEV(tch));
		else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
			sprintf(buf + strlen(buf), " (невидим%s)", GET_CH_SUF_6(tch));
		if (AFF_FLAGGED(tch, AFF_HIDE))
			strcat(buf, " (прячется)");
		if (AFF_FLAGGED(tch, AFF_CAMOUFLAGE))
			strcat(buf, " (маскируется)");

		if (PLR_FLAGGED(tch, PLR_MAILING))
			strcat(buf, " (отправляет письмо)");
		else if (PLR_FLAGGED(tch, PLR_WRITING))
			strcat(buf, " (пишет)");

		if (PRF_FLAGGED(tch, PRF_NOHOLLER))
			sprintf(buf + strlen(buf), " (глух%s)", GET_CH_SUF_1(tch));
		if (PRF_FLAGGED(tch, PRF_NOTELL))
			sprintf(buf + strlen(buf), " (занят%s)", GET_CH_SUF_6(tch));
		if (PLR_FLAGGED(tch, PLR_MUTE))
			sprintf(buf + strlen(buf), " (молчит)");
		if (PLR_FLAGGED(tch, PLR_DUMB))
			sprintf(buf + strlen(buf), " (нем%s)", GET_CH_SUF_6(tch));
		if (PLR_FLAGGED(tch, PLR_KILLER) == PLR_KILLER)
			sprintf(buf + strlen(buf), "&R (ДУШЕГУБ)&n");
		if (!NAME_GOD(tch)
		    && GET_LEVEL(tch) <= NAME_LEVEL) {
			sprintf(buf + strlen(buf), " &W!НЕ ОДОБРЕНО!&n");
			sprintf(buf + strlen(buf),
				"\r\nПадежи: %s/%s/%s/%s/%s/%s  E-mail: %s\r\n",
				GET_PAD(tch, 0), GET_PAD(tch, 1), GET_PAD(tch,
									  2),
				GET_PAD(tch, 3), GET_PAD(tch, 4), GET_PAD(tch, 5), GET_EMAIL(tch));
		}
		if (IS_IMMORTAL(tch))
			strcat(buf, CCNRM(ch, C_SPR));
		//}                     /* endif shortlist */

		if (IS_IMMORTAL(tch)) {
			imms_num++;
			imms = str_add(imms, buf);
			//if (!short_list || !(imms_num % 4)) //Ann: ne ponyatno poka
			//imms = str_add (imms, "\r\n");
		} else {
			morts_num++;
			morts = str_add(morts, buf);
			//if (!short_list || !(morts_num % 4)) //Ann: .
			//morts = str_add (morts, "\r\n");
		}
	}			/* end of for */

	//send_to_char ("do_who_new end for", ch);
	if (morts_num + imms_num == 0) {
		send_to_char("\r\nВы никого не видите.\r\n", ch);
		// !!!
		return;
	}


	if (imms_num > 0)
		buffer = str_add(buffer, imms);
	if (morts_num > 0)
		buffer = str_add(buffer, morts);

	buffer = str_add(buffer, "\r\nВсего видимых:");

	if (imms_num) {
		// sprintf(buf+strlen(buf)," бессмертных %d",imms_num);
		sprintf(buf, " бессмертных %d", imms_num);
		buffer = str_add(buffer, buf);
	}
	if (morts_num) {
		// sprintf(buf+strlen(buf)," смертных %d",morts_num);
		sprintf(buf, " смертных %d", morts_num);
		buffer = str_add(buffer, buf);
	}

	buffer = str_add(buffer, ".\r\n");
	page_string(ch->desc, buffer, 1);
	free(buffer);
	free(imms);
	free(morts);
}


ACMD(do_statistic)
{
	CHAR_DATA *tch;
	int proff[NUM_CLASSES][2], ptot[NUM_CLASSES], i, clan = 0, noclan = 0, hilvl = 0, lowlvl = 0,
	    all = 0, rem = 0, norem = 0, pk = 0, nopk = 0;

	for (i = 0; i < NUM_CLASSES; i++) {
		proff[i][0] = 0;
		proff[i][1] = 0;
		ptot[i] = 0;
	}

	for (tch = character_list; tch; tch = tch->next) {
		if (IS_NPC(tch) || GET_LEVEL(tch) >= LVL_IMMORT || !HERE(tch))
			continue;

		if (GET_HOUSE_UID(tch))
			clan++;
		else
			noclan++;
		if (GET_LEVEL(tch) >= 25)
			hilvl++;
		else
			lowlvl++;
		if (GET_REMORT(tch) >= 1)
			rem++;
		else
			norem++;
		all++;
		if (pk_count(tch) >= 1)
			pk++;
		else
			nopk++;
	
		if (GET_LEVEL(tch) >= 25)
			proff[(int)GET_CLASS(tch)][0]++;
		else
			proff[(int)GET_CLASS(tch)][1]++;
		ptot[(int)GET_CLASS(tch)]++;
	}
	sprintf(buf, "%sСтатистика по игрокам, находящимся в игре (всего / 25 и выше / ниже 25):%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Лекари        %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_CLERIC], proff[CLASS_CLERIC][0], proff[CLASS_CLERIC][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Колдуны     %s[%s%2d/%2d/%2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_BATTLEMAGE], proff[CLASS_BATTLEMAGE][0],
		proff[CLASS_BATTLEMAGE][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Тати          %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_THIEF], proff[CLASS_THIEF][0], proff[CLASS_THIEF][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Богатыри    %s[%s%2d/%2d/%2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_WARRIOR], proff[CLASS_WARRIOR][0], proff[CLASS_WARRIOR][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Наемники      %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_ASSASINE], proff[CLASS_ASSASINE][0],
		proff[CLASS_ASSASINE][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Дружинники  %s[%s%2d/%2d/%2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_GUARD], proff[CLASS_GUARD][0], proff[CLASS_GUARD][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Кудесники     %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_CHARMMAGE], proff[CLASS_CHARMMAGE][0],
		proff[CLASS_CHARMMAGE][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Волшебники  %s[%s%2d/%2d/%2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM),
		ptot[CLASS_DEFENDERMAGE], proff[CLASS_DEFENDERMAGE][0], proff[CLASS_DEFENDERMAGE][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Чернокнижники %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_NECROMANCER], proff[CLASS_NECROMANCER][0],
		proff[CLASS_NECROMANCER][1], CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Витязи      %s[%s%2d/%2d/%2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_PALADINE], proff[CLASS_PALADINE][0],
		proff[CLASS_PALADINE][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Охотники      %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_RANGER], proff[CLASS_RANGER][0], proff[CLASS_RANGER][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Кузнецы     %s[%s%2d/%2d/%2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_SMITH], proff[CLASS_SMITH][0], proff[CLASS_SMITH][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Купцы         %s[%s%2d/%2d/%2d%s]%s       ",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_MERCHANT], proff[CLASS_MERCHANT][0],
		proff[CLASS_MERCHANT][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Волхвы      %s[%s%2d/%2d/%2d%s]%s\r\n\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_DRUID], proff[CLASS_DRUID][0], proff[CLASS_DRUID][1],
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
		"Игроков выше|ниже 25 уровня     %s[%s%*d%s|%s%*d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, hilvl, CCIRED(ch,
								       C_NRM),
		CCICYN(ch, C_NRM), 3, lowlvl, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
		"Игроков с перевоплощениями|без  %s[%s%*d%s|%s%*d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, rem, CCIRED(ch, C_NRM),
		CCICYN(ch, C_NRM), 3, norem, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
		"Клановых|внеклановых игроков    %s[%s%*d%s|%s%*d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, clan, CCIRED(ch,
								      C_NRM),
		CCICYN(ch, C_NRM), 3, noclan, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
		"Игроков с флагами ПК|без ПК     %s[%s%*d%s|%s%*d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, pk, CCIRED(ch,
								    C_NRM),
		CCICYN(ch, C_NRM), 3, nopk, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "Всего игроков %s[%s%*d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, all, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
}


#define USERS_FORMAT \
"Формат: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"
#define MAX_LIST_LEN 200
ACMD(do_users)
{
	const char *format = "%3d %-7s %-12s %-14s %-3s %-8s ";
	char line[200], line2[220], idletime[10], classname[20];
	char state[30] = "\0", *timeptr, mode;
	char name_search[MAX_INPUT_LENGTH] = "\0", host_search[MAX_INPUT_LENGTH];
// Хорс
	char host_by_name[MAX_INPUT_LENGTH] = "\0";
	DESCRIPTOR_DATA *list_players[MAX_LIST_LEN];
	DESCRIPTOR_DATA *d_tmp;
	int count_pl;
	int cycle_i, is, flag_change;
	unsigned long a1, a2;
	int showemail = 0, locating = 0;
	char sorting = '!';
	register CHAR_DATA *ci;
// ---
	CHAR_DATA *tch, *t, *t_tmp;
	DESCRIPTOR_DATA *d;
	int i;
	int low = 0, high = LVL_IMPL, num_can_see = 0;
	int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

	host_search[0] = name_search[0] = '\0';

	strcpy(buf, argument);
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
			switch (mode) {
			case 'o':
			case 'k':
				outlaws = 1;
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'p':
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'd':
				deadweight = 1;
				strcpy(buf, buf1);
				break;
			case 'l':
				if (!IS_GOD(ch))
					return;
				playing = 1;
				half_chop(buf1, arg, buf);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				playing = 1;
				half_chop(buf1, name_search, buf);
				break;
			case 'h':
				playing = 1;
				half_chop(buf1, host_search, buf);
				break;
			case 'u':
				playing = 1;
				half_chop(buf1, host_by_name, buf);
				break;
			case 'w':
				if (!IS_GRGOD(ch))
					return;
				playing = 1;
				locating = 1;
				strcpy(buf, buf1);
				break;
			case 'c':
				playing = 1;
				half_chop(buf1, arg, buf);
				for (i = 0; i < (int) strlen(arg); i++)
					showclass |= find_class_bitvector(arg[i]);
				break;
			case 'e':
				showemail = 1;
				strcpy(buf, buf1);
				break;
			case 's':
				sorting = 'i';
				sorting = *(arg + 2);
				strcpy(buf, buf1);
				break;
			default:
				send_to_char(USERS_FORMAT, ch);
				return;
			}	/* end of switch */

		} else {	/* endif */
			strcpy(name_search, arg);
			strcpy(buf, buf1);
		}
	}			/* end while (parser) */
	if (showemail) {
		strcpy(line, "Ном Професс    Имя         Состояние    Idl Логин    E-mail\r\n");
	} else {
		strcpy(line, "Ном Професс    Имя         Состояние    Idl Логин    Сайт\r\n");
	}
	strcat(line, "--- ------- ------------ -------------- --- -------- ------------------------\r\n");
	send_to_char(line, ch);

	one_argument(argument, arg);

// Хорс
	if (strlen(host_by_name) != 0)
		strcpy(host_search, "!");
	for (d = descriptor_list, count_pl = 0; d && count_pl < MAX_LIST_LEN; d = d->next, count_pl++) {
		list_players[count_pl] = d;
		if (d->original)
			tch = d->original;
		else if (!(tch = d->character))
			continue;
		if (host_by_name != 0)
			if (isname(host_by_name, GET_NAME(tch)))
				strcpy(host_search, d->host);
	}
	if (sorting != '!') {
		is = 1;
		while (is) {
			is = 0;
			for (cycle_i = 1; cycle_i < count_pl; cycle_i++) {
				flag_change = 0;
				d = list_players[cycle_i - 1];
				if (d->original)
					t = d->original;
				else
					t = d->character;
				d_tmp = list_players[cycle_i];
				if (d_tmp->original)
					t_tmp = d_tmp->original;
				else
					t_tmp = d_tmp->character;
				switch (sorting) {
				case 'n':
					if (strcoll(t ? t->player.name : "", t_tmp ? t_tmp->player.name : "") > 0)
						flag_change = 1;
					break;
				case 'e':
					if (strcoll(t ? GET_EMAIL(t) : "", t_tmp ? GET_EMAIL(t_tmp) : "") > 0)
						flag_change = 1;
					break;
				default:
					a1 = get_ip((const char *) d->host);
					a2 = get_ip((const char *) d_tmp->host);
					if (a1 > a2)
						flag_change = 1;
				}
				if (flag_change) {
					list_players[cycle_i - 1] = d_tmp;
					list_players[cycle_i] = d;
					is = 1;
				}
			}
		}
	}

	for (cycle_i = 0; cycle_i < count_pl; cycle_i++) {
		d = (DESCRIPTOR_DATA *) list_players[cycle_i];
// ---
		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (STATE(d) == CON_PLAYING) {
			if (d->original)
				tch = d->original;
			else if (!(tch = d->character))
				continue;

			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && !isname(name_search, GET_NAME(tch)))
				continue;
			if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
				continue;
			if (outlaws && !PLR_FLAGGED((ch), PLR_KILLER))
				continue;
			if (showclass && !(showclass & (1 << GET_CLASS(tch))))
				continue;
			if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
				continue;

			if (d->original)
				sprintf(classname, "[%2d %s %s]", GET_LEVEL(d->original), KIN_ABBR(d->original), CLASS_ABBR(d->original));
			else
				sprintf(classname, "[%2d %s %s]", GET_LEVEL(d->character), KIN_ABBR(d->character), CLASS_ABBR(d->character));
		} else
			strcpy(classname, "   -   ");
		if (GET_LEVEL(ch) < LVL_IMPL)
			strcpy(classname, "   -   ");
		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CON_PLAYING && d->original)
			strcpy(state, "Switched");
		else
			strcpy(state, connected_types[STATE(d)]);

		if (d->character && STATE(d) == CON_PLAYING && !IS_GOD(d->character))
			sprintf(idletime, "%3d", d->character->char_specials.timer *
				SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		else
			strcpy(idletime, "");

		if (d->character && d->character->player.name) {
			if (d->original)
				sprintf(line, format, d->desc_num, classname,
					d->original->player.name, state, idletime, timeptr);
			else
				sprintf(line, format, d->desc_num, classname,
					d->character->player.name, state, idletime, timeptr);
		} else
			sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED", state, idletime, timeptr);
// Хорс
		if (showemail) {
			sprintf(line2, "[%s]",
				d->original ? GET_EMAIL(d->original) : d->character ? GET_EMAIL(d->character) : "");
			strcat(line, line2);
		} else if (d->host && *d->host) {
			sprintf(line2, "[%s]", d->host);
			strcat(line, line2);
		} else
			strcat(line, "[Неизвестный хост]");

		if (locating && (*name_search || *host_by_name))
			if ((STATE(d) == CON_PLAYING)) {
				ci = (d->original ? d->original : d->character);
				if (ci && CAN_SEE(ch, ci) && (ci->in_room != NOWHERE)) {
					if (d->original)
						sprintf(line2, " [%5d] %s (in %s)",
							GET_ROOM_VNUM(IN_ROOM(d->character)),
							world[d->character->in_room]->name, GET_NAME(d->character));
					else
						sprintf(line2, " [%5d] %s",
							GET_ROOM_VNUM(IN_ROOM(ci)), world[ci->in_room]->name);
				}
				strcat(line, line2);
			}
//--  
		strcat(line, "\r\n");
		if (STATE(d) != CON_PLAYING) {
			sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}
		if (STATE(d) != CON_PLAYING || (STATE(d) == CON_PLAYING && d->character && CAN_SEE(ch, d->character))) {
			send_to_char(line, ch);
			num_can_see++;
		}
	}

	sprintf(line, "\r\n%d видимых соединений.\r\n", num_can_see);
//  send_to_char(line, ch);
	page_string(ch->desc, line, TRUE);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
	//DESCRIPTOR_DATA *d;
	switch (subcmd) {
	case SCMD_CREDITS:
		page_string(ch->desc, credits, 0);
		break;
	case SCMD_NEWS:
		page_string(ch->desc, news, 0);
		break;
	case SCMD_INFO:
		page_string(ch->desc, info, 0);
		break;
	case SCMD_WIZLIST:
		page_string(ch->desc, wizlist, 0);
		break;
	case SCMD_IMMLIST:
		page_string(ch->desc, immlist, 0);
		break;
	case SCMD_HANDBOOK:
		page_string(ch->desc, handbook, 0);
		break;
	case SCMD_POLICIES:
		page_string(ch->desc, policies, 0);
		break;
	case SCMD_MOTD:
		page_string(ch->desc, motd, 0);
		break;
	case SCMD_RULES:
		page_string(ch->desc, rules, 0);
		break;
	case SCMD_CLEAR:
		send_to_char("\033[H\033[J", ch);
		break;
	case SCMD_VERSION:
		/*
		   send_to_char(strcat(strcpy(buf, circlemud_version), "\r\n"), ch);
		   send_to_char(strcat(strcpy(buf, DG_SCRIPT_VERSION), "\r\n"), ch);
		 */
		sprintf(buf, "МПМ Былины, версия 0.91 от %s\r\n", __DATE__);
		send_to_char(buf, ch);
		break;
	case SCMD_WHOAMI:
		{
			//Изменения. Фиопий.
			sprintf(buf, "Персонаж : %s\r\n", GET_NAME(ch));
			sprintf(buf + strlen(buf),
				"Падежи : &W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n\r\n",
				GET_PAD(ch, 0), GET_PAD(ch, 1), GET_PAD(ch, 2),
				GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5));

			sprintf(buf + strlen(buf), "Ваш e-mail : %s\r\n", GET_EMAIL(ch));
			time_t birt = ch->player.time.birth;
			struct tm birthday;
			localtime_r(&birt, &birthday);
			sprintf(buf + strlen(buf), "Дата Вашего рождения : %s\r\n", rustime(&birthday));
			sprintf(buf + strlen(buf), "Ваш IP-адрес : %s\r\n", ch->desc ? ch->desc->host : "Unknown");
//               GET_LASTIP (ch));
			send_to_char(buf, ch);
			if (!NAME_GOD(ch)) {
				sprintf(buf, "Имя никем не одобрено!\r\n");
				send_to_char(buf, ch);
			} else if (NAME_GOD(ch) < 1000) {
				/* подправлено Переплутом */
				sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(ch)));
				*buf1 = UPPER(*buf1);
				sprintf(buf, "&RИмя запрещено богом %s&n\r\n", buf1);
				send_to_char(buf, ch);
			} else {
				/* подправлено Переплутом */
				sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(ch)));
				*buf1 = UPPER(*buf1);
				sprintf(buf, "&WИмя одобрено богом %s&n\r\n", buf1);
				send_to_char(buf, ch);
			}
			sprintf(buf, "Перевоплощений: %d\r\n", GET_REMORT(ch));
			send_to_char(buf, ch);
			//Конец изменений. Фиопий.

			break;
		}
	case SCMD_GODNEWS:
		page_string(ch->desc, godnews, 0);
		break;
	default:
		log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
		return;
	}
}


void perform_mortal_where(CHAR_DATA * ch, char *arg)
{
	register CHAR_DATA *i;
	register DESCRIPTOR_DATA *d;

	send_to_char("Кто много знает, тот плохо спит.\r\n", ch);
	return;

	if (!*arg) {
		send_to_char("Игроки, находящиеся в зоне\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) != CON_PLAYING || d->character == ch)
				continue;
			if ((i = (d->original ? d->original : d->character)) == NULL)
				continue;
			if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
				continue;
			if (world[ch->in_room]->zone != world[i->in_room]->zone)
				continue;
			sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			send_to_char(buf, ch);
		}
	} else {		/* print only FIRST char, not all. */
		for (i = character_list; i; i = i->next) {
			if (i->in_room == NOWHERE || i == ch)
				continue;
			if (!CAN_SEE(ch, i)
			    || world[i->in_room]->zone != world[ch->in_room]->zone)
				continue;
			if (!isname(arg, i->player.name))
				continue;
			sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			send_to_char(buf, ch);
			return;
		}
		send_to_char("Никого похожего с этим именем нет.\r\n", ch);
	}
}


void print_object_location(int num, OBJ_DATA * obj, CHAR_DATA * ch, int recur)
{
	if (num > 0)
		sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
	else
		sprintf(buf, "%33s", " - ");

	if (obj->in_room > NOWHERE) {
		sprintf(buf + strlen(buf), "[%5d] %s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), world[obj->in_room]->name);
		send_to_char(buf, ch);
	} else if (obj->carried_by) {
		sprintf(buf + strlen(buf), "затарено %s\r\n", PERS(obj->carried_by, ch, 4));
		send_to_char(buf, ch);
	} else if (obj->worn_by) {
		sprintf(buf + strlen(buf), "одет на %s\r\n", PERS(obj->worn_by, ch, 1));
		send_to_char(buf, ch);
	} else if (obj->in_obj) {
		sprintf(buf + strlen(buf), "лежит в %s%s\r\n",
			obj->in_obj->short_description, (recur ? ", который находится " : " "));
		send_to_char(buf, ch);
		if (recur)
			print_object_location(0, obj->in_obj, ch, recur);
	} else {
		sprintf(buf + strlen(buf), "находится где-то там, далеко-далеко.\r\n");
		send_to_char(buf, ch);
	}
}



void perform_immort_where(CHAR_DATA * ch, char *arg)
{
	register CHAR_DATA *i;
	register OBJ_DATA *k;
	DESCRIPTOR_DATA *d;
	int num = 0, found = 0;

	if (!*arg) {
		if (GET_LEVEL(ch) < LVL_IMPL)
			send_to_char("Где КТО конкретно?", ch);
		else {
			send_to_char("ИГРОКИ\r\n------\r\n", ch);
			for (d = descriptor_list; d; d = d->next)
				if (STATE(d) == CON_PLAYING) {
					i = (d->original ? d->original : d->character);
					if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
						if (d->original)
							sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
								GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(d->character)),
								world[d->character->in_room]->name,
								GET_NAME(d->character));
						else
							sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(i)), world[i->in_room]->name);
						send_to_char(buf, ch);
					}
				}
		}
	} else {
		for (i = character_list; i; i = i->next)
			if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
				found = 1;
				sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
					GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)]->name);
				send_to_char(buf, ch);
			}
		if (GET_LEVEL(ch) > LVL_GOD) {
			for (num = 0, k = object_list; k; k = k->next)
				if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
					found = 1;
					print_object_location(++num, k, ch, TRUE);
				}
			if (!found)
				send_to_char("Нет ничего похожего.\r\n", ch);
		}
	}
}



ACMD(do_where)
{
	one_argument(argument, arg);

	if (IS_GRGOD(ch))
		perform_immort_where(ch, arg);
	else
		perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
	int i;

	if (IS_NPC(ch)) {
		send_to_char("Боги уже придумали Ваш уровень.\r\n", ch);
		return;
	}
	*buf = '\0';

	for (i = 1; i < LVL_IMMORT; i++) {
		sprintf(buf + strlen(buf), "[%2d] %8d-%-8d\r\n", i, level_exp(ch, i), level_exp(ch, i + 1) - 1);
		/* switch (GET_SEX(ch))
		   {case SEX_MALE:
		   case SEX_NEUTRAL:
		   strcat(buf, title_male(GET_CLASS(ch), i));
		   break;
		   case SEX_FEMALE:
		   strcat(buf, title_female(GET_CLASS(ch), i));
		   break;
		   default:
		   send_to_char("Атас, бесполые в игре.\r\n", ch);
		   break;
		   }
		   strcat(buf, "\r\n");
		 */
	}
	sprintf(buf + strlen(buf), "[%2d] %8d          (БЕССМЕРТИЕ)\r\n", LVL_IMMORT, level_exp(ch, LVL_IMMORT));
	page_string(ch->desc, buf, 1);
}



ACMD(do_consider)
{
	CHAR_DATA *victim;
	int diff;

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		send_to_char("Кого Вы хотите оценить ?\r\n", ch);
		return;
	}
	if (victim == ch) {
		send_to_char("Легко!  Выберите параметр <Удалить персонаж> !\r\n", ch);
		return;
	}
	if (!IS_NPC(victim)) {
		send_to_char("Оценивайте игроков сами - тут я не советчик.\r\n", ch);
		return;
	}
	diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

	if (diff <= -10)
		send_to_char("Ути-пути, моя рыбонька.\r\n", ch);
	else if (diff <= -5)
		send_to_char("\"Сделаем без шуму и пыли !\"\r\n", ch);
	else if (diff <= -2)
		send_to_char("Легко.\r\n", ch);
	else if (diff <= -1)
		send_to_char("Сравнительно легко.\r\n", ch);
	else if (diff == 0)
		send_to_char("Равный поединок!\r\n", ch);
	else if (diff <= 1)
		send_to_char("Вам понадобится немного удачи!\r\n", ch);
	else if (diff <= 2)
		send_to_char("Вам потребуется везение!\r\n", ch);
	else if (diff <= 3)
		send_to_char("Удача и хорошее снаряжение Вам сильно пригодятся!\r\n", ch);
	else if (diff <= 5)
		send_to_char("Вы берете на себя слишком много.\r\n", ch);
	else if (diff <= 10)
		send_to_char("Ладно, войдете еще раз.\r\n", ch);
	else if (diff <= 100)
		send_to_char("Срочно к психиатру - Вы страдаете манией величия!\r\n", ch);

}



ACMD(do_diagnose)
{
	CHAR_DATA *vict;

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
			send_to_char(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	} else {
		if (FIGHTING(ch))
			diag_char_to_char(FIGHTING(ch), ch);
		else
			send_to_char("На кого вы хотите взглянуть ?\r\n", ch);
	}
}


const char *ctypes[] = { "выключен", "простой", "обычный", "полный", "\n"
};

ACMD(do_color)
{
	int tp;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		sprintf(buf, "%s %sцветовой%s режим.\r\n", ctypes[COLOR_LEV(ch)], CCRED(ch, C_SPR), CCNRM(ch, C_OFF));
		send_to_char(CAP(buf), ch);
		return;
	}
	if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
		send_to_char("Формат: [режим] цвет { выкл | простой | обычный | полный }\r\n", ch);
		return;
	}
	REMOVE_BIT(PRF_FLAGS(ch, PRF_COLOR_1), PRF_COLOR_1);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_COLOR_2), PRF_COLOR_2);

	SET_BIT(PRF_FLAGS(ch, PRF_COLOR_1), (PRF_COLOR_1 * (tp & 1)));
	SET_BIT(PRF_FLAGS(ch, PRF_COLOR_1), (PRF_COLOR_2 * (tp & 2) >> 1));

	sprintf(buf, "%s %sцветовой%s режим.\r\n", ctypes[tp], CCRED(ch, C_SPR), CCNRM(ch, C_OFF));
	send_to_char(CAP(buf), ch);
}


ACMD(do_toggle)
{
	if (IS_NPC(ch))
		return;
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "нет");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		sprintf(buf,
			" Не выследить  : %-3s     "
			" Вечный свет   : %-3s     "
			" Флаги комнат  : %-3s \r\n",
			ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
			ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)), ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)));
		send_to_char(buf, ch);
	}

	sprintf(buf,
		" Уровень жизни : %-3s     "
		" Обращения     : %-3s     " " Краткий режим : %-3s \r\n" " Энергия       : %-3s     "
// shapirus
		" Кто-то        : %-6s  "
		" Сжатый режим  : %-3s \r\n"
		" Опыт          : %-3s     "
		" Орать         : %-3s     "
		" Повтор команд : %-3s \r\n"
		" Заучивание    : %-3s     "
		" Болтать       : %-3s     "
		" Автовыходы    : %-3s \r\n"
		" Деньги        : %-3s     "
		" Аукцион       : %-3s     "
		" Цвет          : %-8s\r\n"
		" Выходы        : %-3s     "
		" Себя в бою    : %-3s     "
		" Сжатие        : %-3s \r\n"
		" Брать куны    : %-3s     "
		" Задание       : %-3s     "
		" Трусость      : %-3s \r\n"
		" Автозауч.     : %-3s     "
		" Призыв        : %-3s     "
		" Автопомощь    : %-3s \r\n"
		" IAC GA        : %-3s     "
		" Показ группы  : %-7s "
		" Автодележ     : %-3s \r\n"
		" Автограбеж    : %-3s     " " Без двойников : %-3s     " " Арена         : %-3s \r\n"
//F@N|
		" Базар         : %-3s \r\n",
		ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOTELL)),
		ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)), ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
// shapirus
		PRF_FLAGGED(ch, PRF_NOINVISTELL) ? "нельзя" : "можно",
		ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
		ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOHOLLER)),
		YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),
		ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
		ONOFF(PRF_FLAGGED(ch, PRF_DISPGOLD)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
		ctypes[COLOR_LEV(ch)], ONOFF(PRF_FLAGGED(ch, PRF_DISPEXITS)), ONOFF(!PRF_FLAGGED(ch, PRF_DISPFIGHT)),
#if defined(HAVE_ZLIB)
		ch->desc->deflate == NULL ? "нет" : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"),
#else
		"N/A",
#endif
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOMONEY)),
		YESNO(PRF_FLAGGED(ch, PRF_QUEST)),
		buf2,
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),
		ONOFF(PRF_FLAGGED(ch, PRF_SUMMONABLE)),
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
		ONOFF(PRF_FLAGGED(ch, PRF_GOAHEAD)),
		PRF_FLAGGED(ch, PRF_SHOWGROUP) ? "полный" : "краткий",
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
		ONOFF(PRF_FLAGGED(ch, PRF_NOCLONES)), ONOFF(!PRF_FLAGGED(ch, PRF_NOARENA)),
//F@N|
		ONOFF(!PRF_FLAGGED(ch, PRF_NOEXCHANGE)));


	send_to_char(buf, ch);
}


struct sort_struct {
	int sort_pos;
	byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
	int a, b, tmp;

	num_of_cmds = 0;

	/*
	 * first, count commands (num_of_commands is actually one greater than the
	 * number of commands; it inclues the '\n'.
	 */
	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	/* create data array */
	CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

	/* initialize it */
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social = FALSE;
	}

	/* the infernal special case */
	cmd_sort_info[find_command("insult")].is_social = TRUE;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
				   cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}



ACMD(do_commands)
{
	int no, i, cmd_num, num_of;
	int wizhelp = 0, socials = 0;
	CHAR_DATA *vict;

	one_argument(argument, arg);

/*  if (*arg)
     {if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) || IS_NPC(vict))
         {send_to_char("Кто это ?\r\n", ch);
          return;
         }
      if (GET_LEVEL(ch) < GET_LEVEL(vict) && !GET_COMMSTATE(ch))
         {send_to_char("Вы не можете узнать команды для персонажа выше Вас уровнем.\r\n", ch);
          return;
         }
     }
  else */

	vict = ch;

	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;

	sprintf(buf, "Следующие %s%s доступны %s:\r\n",
		wizhelp ? "привилегированные " : "",
		socials ? "социалы" : "команды", vict == ch ? "Вам" : GET_PAD(vict, 2));

	if (socials)
		num_of = top_of_socialk + 1;
	else
		num_of = num_of_cmds - 1;

	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = socials ? 0 : 1; cmd_num < num_of; cmd_num++)
		if (socials) {
			sprintf(buf + strlen(buf), "%-19s", soc_keys_list[cmd_num].keyword);
			if (!(no % 4))
				strcat(buf, "\r\n");
			no++;
		} else {
			i = cmd_sort_info[cmd_num].sort_pos;
			if (cmd_info[i].minimum_level >= 0 &&
//           (GET_LEVEL(vict) >= cmd_info[i].minimum_level || GET_COMMSTATE(vict)) &&
			    (priv->
			     enough_cmd_priv(std::string(GET_NAME(vict)), GET_LEVEL(vict),
					     std::string(cmd_info[i].command), i)
			     || GET_COMMSTATE(vict))
			    && (cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp
			    && (wizhelp || socials == cmd_sort_info[i].is_social)) {
				sprintf(buf + strlen(buf), "%-15s", cmd_info[i].command);
				if (!(no % 5))
					strcat(buf, "\r\n");
				no++;
			}
		}

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}

int hiding[] = { AFF_SNEAK,
	AFF_HIDE,
	AFF_CAMOUFLAGE,
	0
};

ACMD(do_affects)
{
	AFFECT_DATA *aff;
	FLAG_DATA saved;
	int i, j;
	char sp_name[MAX_STRING_LENGTH];

	/* Showing the bitvector */
	saved = ch->char_specials.saved.affected_by;
	for (i = 0; (j = hiding[i]); i++) {
		if (IS_SET(GET_FLAG(saved, j), j))
			SET_BIT(GET_FLAG(saved, j), j);
		REMOVE_BIT(AFF_FLAGS(ch, j), j);
	}
	sprintbits(ch->char_specials.saved.affected_by, affected_bits, buf2, ",");
	sprintf(buf, "Аффекты: %s%s%s\r\n", CCIYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	for (i = 0; (j = hiding[i]); i++)
		if (IS_SET(GET_FLAG(saved, j), j))
			SET_BIT(AFF_FLAGS(ch, j), j);

	/* Routine to show what spells a char is affected by */
	if (ch->affected) {
		for (aff = ch->affected; aff; aff = aff->next) {
			*buf2 = '\0';
			strcpy(sp_name, spell_name(aff->type));
			sprintf(buf, "%s%s%-21s%s ",
				*sp_name == '!' ? "Состояние  : " : "Заклинание : ",
				CCICYN(ch, C_NRM), sp_name, CCNRM(ch, C_NRM));
			if (!IS_IMMORTAL(ch)) {
				if (aff->next && aff->type == aff->next->type)
					continue;
			} else {
				if (aff->modifier) {
					sprintf(buf2, "%+d к %s", aff->modifier, apply_types[(int) aff->location]);
					strcat(buf, buf2);
				}
				if (aff->bitvector) {
					if (*buf2)
						strcat(buf, ", устанавливает ");
					else
						strcat(buf, "устанавливает ");
					strcat(buf, CCIRED(ch, C_NRM));
					sprintbit(aff->bitvector, affected_bits, buf2);
					strcat(buf, buf2);
					strcat(buf, CCNRM(ch, C_NRM));
				}
			}
			send_to_char(strcat(buf, "\r\n"), ch);
		}
	}
}

// Create web-page with users list
void make_who2html(void)
{

	FILE *opf;

	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;

	int imms_num = 0, morts_num = 0;

	char *imms = NULL;
	char *morts = NULL;
	char *buffer = NULL;

	if ((opf = fopen(WHOLIST_FILE, "w")) == 0)
		return;		/* or log it ? *shrug* */

	fprintf(opf, "<HTML><HEAD><TITLE>Кто сейчас в Былинах?</TITLE></HEAD>\n");
	fprintf(opf, "<BODY><H1>Кто сейчас живет в Былинах?</H1><HR>\n");

	sprintf(buf, "БОГИ <BR> \r\n");
	imms = str_add(imms, buf);

	sprintf(buf, "<BR>Игроки<BR> \r\n  ");
	morts = str_add(morts, buf);

	for (d = descriptor_list; d; d = d->next)
		if (STATE(d) == CON_PLAYING && GET_INVIS_LEV(d->character) < 31) {
			ch = d->character;
			sprintf(buf, "%s <BR> \r\n ", race_or_title(ch));

			if (IS_IMMORTAL(ch)) {
				imms_num++;
				imms = str_add(imms, buf);
			} else {
				morts_num++;
				morts = str_add(morts, buf);
			}
		}

	if (morts_num + imms_num == 0) {
		sprintf(buf, "Все ушли на фронт! <BR>");
		buffer = str_add(buffer, buf);
	} else {
		if (imms_num > 0)
			buffer = str_add(buffer, imms);
		if (morts_num > 0)
			buffer = str_add(buffer, morts);

		buffer = str_add(buffer, " <BR> \r\n Всего :");

		if (imms_num) {
			// sprintf(buf+strlen(buf)," бессмертных %d",imms_num);
			sprintf(buf, " бессмертных %d", imms_num);
			buffer = str_add(buffer, buf);
		}
		if (morts_num) {
			// sprintf(buf+strlen(buf)," смертных %d",morts_num);
			sprintf(buf, " смертных %d", morts_num);
			buffer = str_add(buffer, buf);
		}

		buffer = str_add(buffer, ".\n");
	}

	fprintf(opf, buffer);

	free(buffer);
	free(imms);
	free(morts);


	fprintf(opf, "<HR></BODY></HTML>\n");
	fclose(opf);
}
