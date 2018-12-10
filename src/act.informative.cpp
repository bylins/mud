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

#include "world.objects.hpp"
#include "world.characters.hpp"
#include "object.prototypes.hpp"
#include "logger.hpp"
#include "shutdown.parameters.hpp"
#include "obj.hpp"
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
#include "mail.h"
#include "parcel.hpp"
#include "features.hpp"
#include "im.h"
#include "house.h"
#include "description.h"
#include "privilege.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "random.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "parcel.hpp"
#include "liquid.hpp"
#include "modify.h"
#include "room.hpp"
#include "glory_const.hpp"
#include "player_races.hpp"
#include "corpse.hpp"
#include "sets_drop.hpp"
#include "help.hpp"
#include "map.hpp"
#include "ext_money.hpp"
#include "mob_stat.hpp"
#include "char_obj_utils.inl"
#include "class.hpp"
#include "zone.table.hpp"
#include "structs.h"
#include "sysdep.h"
#include "bonus.h"
#include "conf.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <string>
#include <sstream>
#include <vector>

using std::string;

// extern variables
extern DESCRIPTOR_DATA *descriptor_list;
extern int number_of_social_commands;
extern char *credits;
extern char *info;
extern char *motd;
extern char *rules;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char const *class_abbrevs[];
extern char const *kin_abbrevs[];
extern const char *material_name[];
extern im_type *imtypes;
extern int top_imtypes;
extern void show_code_date(CHAR_DATA *ch);
extern int nameserver_is_slow; //config.cpp
extern std::vector<City> cities;
// extern functions
long find_class_bitvector(char arg);
int level_exp(CHAR_DATA * ch, int level);
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(CHAR_DATA * ch);
int pk_count(CHAR_DATA * ch);
// local functions
const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how);
void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show);
char *diag_obj_to_char(CHAR_DATA * i, OBJ_DATA * obj, int mode);
const char *diag_obj_timer(const OBJ_DATA * obj);
char *diag_timer_to_char(const OBJ_DATA* obj);
int get_pick_chance(int skill_pick, int lock_complexity);
int thaco(int class_num, int level);

void do_affects(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_look(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_examine(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gold(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_score(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_inventory(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_equipment(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_time(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_weather(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_who(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_users(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gen_ps(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void perform_mortal_where(CHAR_DATA * ch, char *arg);
void perform_immort_where(CHAR_DATA * ch, char *arg);
void do_where(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_levels(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_consider(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_diagnose(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_toggle(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void sort_commands(void);
void do_commands(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_looking(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hearing(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sides(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_quest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_check(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_cities(CHAR_DATA *ch, char*, int, int);
void diag_char_to_char(CHAR_DATA * i, CHAR_DATA * ch);
void look_at_char(CHAR_DATA * i, CHAR_DATA * ch);
void list_one_char(CHAR_DATA * i, CHAR_DATA * ch, int skill_mode);
void list_char_to_char(const ROOM_DATA::people_t& list, CHAR_DATA* ch);
void do_auto_exits(CHAR_DATA * ch);
void do_exits(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void look_in_direction(CHAR_DATA * ch, int dir, int info_is);
void look_in_obj(CHAR_DATA * ch, char *arg);
char *find_exdesc(char *word, const EXTRA_DESCR_DATA::shared_ptr& list);
bool look_at_target(CHAR_DATA * ch, char *arg, int subcmd);
void gods_day_now(CHAR_DATA * ch);
void do_blind_exits(CHAR_DATA *ch);
const char *diag_liquid_timer(const OBJ_DATA * obj);
#define EXIT_SHOW_WALL    (1 << 0)
#define EXIT_SHOW_LOOKING (1 << 1)

void do_quest(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	
	send_to_char("У Вас нет никаких ежедневных поручений.\r\nЧтобы взять новые, наберите &Wпоручения получить&n.\r\n", ch);
}

/*
 * This function screams bitvector... -gg 6/45/98
 */

int param_sort = 0;

const char *Dirs[NUM_OF_DIRS + 1] = { "Север",
									  "Восток",
									  "Юг",
									  "Запад",
									  "Вверх",
									  "Вниз",
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

const char *Locks[4][2] =
{
	{"%s Вы в жизни не видели подобного замка.%s\r\n", KIRED},
	{"%s Замок очень сложный.%s\r\n", KIYEL},
	{"%s Сложный замок. Как бы не сломать.%s\r\n", KIGRN},
	{"%s Простой замок. Эка невидаль.%s\r\n", KGRN}
};

void do_check(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
    if (!login_change_invoice(ch))
		send_to_char("Проверка показала: новых сообщений нет.\r\n", ch);
}

char *diag_obj_to_char(CHAR_DATA* i, OBJ_DATA* obj, int mode)
{
	static char out_str[80] = "\0";
	const char *color;
	int percent;

	if (GET_OBJ_MAX(obj) > 0)
		percent = 100 * GET_OBJ_CUR(obj) / GET_OBJ_MAX(obj);
	else
		percent = -1;

	if (percent >= 100)
	{
		percent = 7;
		color = CCWHT(i, C_NRM);
	}
	else if (percent >= 90)
	{
		percent = 6;
		color = CCIGRN(i, C_NRM);
	}
	else if (percent >= 75)
	{
		percent = 5;
		color = CCGRN(i, C_NRM);
	}
	else if (percent >= 50)
	{
		percent = 4;
		color = CCIYEL(i, C_NRM);
	}
	else if (percent >= 30)
	{
		percent = 3;
		color = CCIRED(i, C_NRM);
	}
	else if (percent >= 15)
	{
		percent = 2;
		color = CCIRED(i, C_NRM);
	}
	else if (percent > 0)
	{
		percent = 1;
		color = CCRED(i, C_NRM);
	}
	else
	{
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

char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear)
{
	static char out_str[MAX_STRING_LENGTH];
	int skill = 0;
	int need_str = 0;

	*out_str = '\0';
	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_WEAPON:
		switch (GET_OBJ_SKILL(obj))
		{
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
		{
			sprintf(out_str, "Принадлежит к классу \"%s\".\r\n", weapon_class[skill - 1]);
		}

	default:
		if (show_wear)
		{
			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на палец.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на шею.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на туловище.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на голову.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на ноги.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
			{
				sprintf(out_str + strlen(out_str), "Можно обуть.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на кисти.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на руки.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на плечи.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на пояс.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_QUIVER))
			{
				sprintf(out_str + strlen(out_str), "Можно использовать как колчан.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
			{
				sprintf(out_str + strlen(out_str), "Можно надеть на запястья.\r\n");
			}

			if (show_wear > 1)
			{
				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
				{
					need_str = MAX(0, calc_str_req((GET_OBJ_WEIGHT(obj)+1)/2, STR_HOLD_W));
					sprintf(out_str + strlen(out_str), "Можно использовать как щит (требуется %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
				{
					need_str = MAX(0, calc_str_req(GET_OBJ_WEIGHT(obj), STR_WIELD_W));
					sprintf(out_str + strlen(out_str), "Можно взять в правую руку (требуется %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
				{
					need_str = MAX(0, calc_str_req(GET_OBJ_WEIGHT(obj), STR_HOLD_W));
					sprintf(out_str + strlen(out_str), "Можно взять в левую руку (требуется %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
				{
					need_str = MAX(0, calc_str_req(GET_OBJ_WEIGHT(obj), STR_BOTH_W));
					sprintf(out_str + strlen(out_str), "Можно взять в обе руки (требуется %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}
			}
			else
			{
				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
				{
					sprintf(out_str + strlen(out_str), "Можно использовать как щит.\r\n");
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
				{
					sprintf(out_str + strlen(out_str), "Можно взять в правую руку.\r\n");
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
				{
					sprintf(out_str + strlen(out_str), "Можно взять в левую руку.\r\n");
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
				{
					sprintf(out_str + strlen(out_str), "Можно взять в обе руки.\r\n");
				}
			}
		}
	}
	return (out_str);
}

// Чтобы можно было получить только строку состяния
const char *diag_obj_timer(const OBJ_DATA* obj)
{
	int prot_timer;
	if (GET_OBJ_RNUM(obj) != NOTHING)
	{
		if (check_unlimited_timer(obj))
		{
			return "нерушимо";
		}

		if (GET_OBJ_CRAFTIMER(obj) > 0)
		{
			prot_timer = GET_OBJ_CRAFTIMER(obj);// если вещь скрафчена, смотрим ее таймер а не у прототипа
		}
		else 
		{
			prot_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		}

		if (!prot_timer)
		{
			return "Прототип предмета имеет нулевой таймер!\r\n";
		}

		const int tm = (obj->get_timer() * 100 / prot_timer); // если вещь скрафчена, смотрим ее таймер а не у прототипа
		return print_obj_state(tm);
	}
	return "";
}



char *diag_timer_to_char(const OBJ_DATA* obj)
{
	static char out_str[MAX_STRING_LENGTH];
	*out_str = 0;
	sprintf(out_str, "Состояние: %s.\r\n", diag_obj_timer(obj));
	return (out_str);
}

char *diag_uses_to_char(OBJ_DATA * obj, CHAR_DATA * ch)
{
	static char out_str[MAX_STRING_LENGTH];

	*out_str = 0;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_INGREDIENT
		&& IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES)
		&& GET_CLASS(ch) == CLASS_DRUID)
	{
		int i = -1;
		if ((i = real_object(GET_OBJ_VAL(obj, 1))) >= 0)
		{
			sprintf(out_str, "Прототип: %s%s%s.\r\n",
				CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
		}
		sprintf(out_str + strlen(out_str), "Осталось применений: %s%d&n.\r\n",
			GET_OBJ_VAL(obj, 2) > 100 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}

char *diag_shot_to_char(OBJ_DATA * obj, CHAR_DATA * ch)
{
	static char out_str[MAX_STRING_LENGTH];

	*out_str = 0;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MAGIC_CONTAINER
		&& (GET_CLASS(ch) == CLASS_RANGER||GET_CLASS(ch) == CLASS_CHARMMAGE||GET_CLASS(ch) == CLASS_DRUID))
	{
		sprintf(out_str + strlen(out_str), "Осталось стрел: %s%d&n.\r\n",
			GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}

/**
* При чтении писем и осмотре чара в его описании подставляем в начало каждой строки пробел
* (для дурных тригов), пользуясь случаем передаю привет проне!
*/
std::string space_before_string(char const *text)
{
	if (text)
	{
		std::string tmp(" ");
		tmp += text;
		boost::replace_all(tmp, "\n", "\n ");
		boost::trim_right_if(tmp, boost::is_any_of(std::string(" ")));
		return tmp;
	}
	return "";
}

std::string space_before_string(std::string text)
{
	if (text != "")
	{
		std::string tmp(" ");
		tmp += text;
		boost::replace_all(tmp, "\n", "\n ");
		boost::trim_right_if(tmp, boost::is_any_of(std::string(" ")));
		return tmp;
	}
	return "";
}


namespace
{

std::string diag_armor_type_to_char(const OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_LIGHT)
	{
		return "Легкий тип доспехов.\r\n";
	}
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_MEDIAN)
	{
		return "Средний тип доспехов.\r\n";
	}
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_HEAVY)
	{
		return "Тяжелый тип доспехов.\r\n";
	}
	return "";
}

} // namespace

// для использования с чарами:
// возвращает метки предмета, если они есть и смотрящий является их автором или является членом соотв. клана
std::string char_get_custom_label(OBJ_DATA *obj, CHAR_DATA *ch)
{
	const char *delim_l = NULL;
	const char *delim_r = NULL;

	// разные скобки для клановых и личных
	if (obj->get_custom_label() && (ch->player_specials->clan && obj->get_custom_label()->clan != NULL &&
	    !strcmp(obj->get_custom_label()->clan, ch->player_specials->clan->GetAbbrev())))
	{
		delim_l = " *";
		delim_r = "*";
	} else {
		delim_l = " (";
		delim_r = ")";
	}

	if (AUTH_CUSTOM_LABEL(obj, ch))
	{
		return boost::str(boost::format("%s%s%s") % delim_l % obj->get_custom_label()->label_text % delim_r);
	}

	return "";
}

// mode 1 show_state 3 для хранилище (4 - хранилище ингров)
const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how)
{
	*buf = '\0';
	if ((mode < 5) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
		sprintf(buf, "[%5d] ", GET_OBJ_VNUM(object));

	if (mode == 0
		&& !object->get_description().empty())
	{
		strcat(buf, object->get_description().c_str());
		strcat(buf, char_get_custom_label(object, ch).c_str());
	}
	else if (!object->get_short_description().empty() && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4)))
	{
		strcat(buf, object->get_short_description().c_str());
		strcat(buf, char_get_custom_label(object, ch).c_str());
	}
	else if (mode == 5)
	{
		if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_NOTE)
		{
			if (!object->get_action_description().empty())
			{
				strcpy(buf, "Вы прочитали следующее :\r\n\r\n");
				strcat(buf, space_before_string(object->get_action_description().c_str()).c_str());
				page_string(ch->desc, buf, 1);
			}
			else
			{
				send_to_char("Чисто.\r\n", ch);
			}
			return 0;
		}
		else if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_BANDAGE)
		{
			strcpy(buf, "Бинты для перевязки ран ('перевязать').\r\n");
			snprintf(buf2, MAX_STRING_LENGTH, "Осталось применений: %d, восстановление: %d",
				GET_OBJ_WEIGHT(object), GET_OBJ_VAL(object, 0) * 10);
			strcat(buf, buf2);
		}
		else if (GET_OBJ_TYPE(object) != OBJ_DATA::ITEM_DRINKCON)
		{
			strcpy(buf, "Вы не видите ничего необычного.");
		}
		else		// ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN
		{
			strcpy(buf, "Это емкость для жидкости.");
		}
	}

	if (show_state && show_state != 3 && show_state != 4)
	{
		*buf2 = '\0';
		if (mode == 1 && how <= 1)
		{
			if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_LIGHT)
			{
				if (GET_OBJ_VAL(object, 2) == -1)
					strcpy(buf2, " (вечный свет)");
				else if (GET_OBJ_VAL(object, 2) == 0)
					sprintf(buf2, " (погас%s)", GET_OBJ_SUF_4(object));
				else
					sprintf(buf2, " (%d %s)",
							GET_OBJ_VAL(object, 2), desc_count(GET_OBJ_VAL(object, 2), WHAT_HOUR));
			}
			else
			{
				if (object->timed_spell().is_spell_poisoned() != -1)
				{
					sprintf(buf2, " %s*%s%s", CCGRN(ch, C_NRM),
						CCNRM(ch, C_NRM), diag_obj_to_char(ch, object, 1));
				}
				else
				{
					sprintf(buf2, " %s", diag_obj_to_char(ch, object, 1));
				}
			}
			if ((GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_CONTAINER) && !OBJVAL_FLAGGED(object, CONT_CLOSED)) // если закрыто, содержимое не показываем
			{
				if (object->get_contains())
				{
					strcat(buf2, " (есть содержимое)");
				}
				else
				{
					if (GET_OBJ_VAL(object, 3) < 1) // есть ключ для открытия, пустоту не показываем2
						sprintf(buf2 + strlen(buf2), " (пуст%s)", GET_OBJ_SUF_6(object));
				}
			}
		}
		else if (mode >= 2 && how <= 1)
		{
			std::string obj_name = OBJN(object, ch, 0);
			obj_name[0] = UPPER(obj_name[0]);
			if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_LIGHT)
			{
				if (GET_OBJ_VAL(object, 2) == -1)
				{
					sprintf(buf2, "\r\n%s дает вечный свет.", obj_name.c_str());
				}
				else if (GET_OBJ_VAL(object, 2) == 0)
				{
					sprintf(buf2, "\r\n%s погас%s.", obj_name.c_str(), GET_OBJ_SUF_4(object));
				}
				else
				{
					sprintf(buf2, "\r\n%s будет светить %d %s.", obj_name.c_str(), GET_OBJ_VAL(object, 2),
						desc_count(GET_OBJ_VAL(object, 2), WHAT_HOUR));
				}
			}
			else if (GET_OBJ_CUR(object) < GET_OBJ_MAX(object))
			{
				sprintf(buf2, "\r\n%s %s.", obj_name.c_str(), diag_obj_to_char(ch, object, 2));
			}
		}
		strcat(buf, buf2);
	}
	if (how > 1)
	{
		sprintf(buf + strlen(buf), " [%d]", how);
	}
	if (mode != 3 && how <= 1)
	{
		if (object->get_extra_flag(EExtraFlag::ITEM_INVISIBLE))
		{
			sprintf(buf2, " (невидим%s)", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (object->get_extra_flag(EExtraFlag::ITEM_BLESS)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
			strcat(buf, " ..голубая аура!");
		if (object->get_extra_flag(EExtraFlag::ITEM_MAGIC)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
			strcat(buf, " ..желтая аура!");
		if (object->get_extra_flag(EExtraFlag::ITEM_POISONED)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON))
		{
			sprintf(buf2, "..отравлен%s!", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (object->get_extra_flag(EExtraFlag::ITEM_GLOW))
			strcat(buf, " ..блестит!");
		if (object->get_extra_flag(EExtraFlag::ITEM_HUM) && !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
			strcat(buf, " ..шумит!");
		if (object->get_extra_flag(EExtraFlag::ITEM_FIRE))
			strcat(buf, " ..горит!");
		if (object->get_extra_flag(EExtraFlag::ITEM_BLOODY))
		{
			sprintf(buf2, " %s..покрыт%s кровью!%s", CCIRED(ch, C_NRM), GET_OBJ_SUF_6(object), CCNRM(ch, C_NRM));
			strcat(buf, buf2);
		}
	}

	if (mode == 1)
	{
		// клан-сундук, выводим список разом постранично
		if (show_state == 3)
		{
			sprintf(buf + strlen(buf), " [%d %s]\r\n",
					GET_OBJ_RENTEQ(object) * CLAN_STOREHOUSE_COEFF / 100,
					desc_count(GET_OBJ_RENTEQ(object) * CLAN_STOREHOUSE_COEFF / 100, WHAT_MONEYa));
			return buf;
		}
		// ингры
		else if (show_state == 4)
		{
			sprintf(buf + strlen(buf), " [%d %s]\r\n", GET_OBJ_RENT(object),
					desc_count(GET_OBJ_RENT(object), WHAT_MONEYa));
			return buf;
		}
	}

	strcat(buf, "\r\n");
	if (mode >= 5)
	{
		strcat(buf, diag_weapon_to_char(object, TRUE));
		strcat(buf, diag_armor_type_to_char(object).c_str());
		strcat(buf, diag_timer_to_char(object));
		//strcat(buf, diag_uses_to_char(object, ch)); // commented by WorM перенес в obj_info чтобы заряды рун было видно на базаре/ауке
		strcat(buf, object->diag_ts_to_char(ch).c_str());
	}
	page_string(ch->desc, buf, TRUE);
	return 0;
}

void do_cities(CHAR_DATA *ch, char*, int, int)
{
	send_to_char("Города на Руси:\r\n", ch);
	for (unsigned int i = 0; i < cities.size(); i++)
	{
		sprintf(buf, "%3d.", i + 1);
		if (IS_IMMORTAL(ch))
		{
			sprintf(buf1, " [VNUM: %d]", cities[i].rent_vnum);
			strcat(buf, buf1);
		}
		sprintf(buf1, " %s: %s\r\n", cities[i].name.c_str(), (ch->check_city(i) ? "&gВы были там.&n" : "&rВы еще не были там.&n"));
		strcat(buf, buf1);
		send_to_char(buf, ch);
	}
}


bool quest_item(OBJ_DATA *obj)
{
	if ((OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODECAY)) && (!(CAN_WEAR(obj, EWearFlag::ITEM_WEAR_TAKE))))
	{
		return true;
	}
	return false;
}

void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show)
{
	OBJ_DATA *i, *push = NULL;
	bool found = FALSE;
	int push_count = 0;
	std::ostringstream buffer;
	long count = 0, cost = 0;

	bool clan_chest = false;
	if (mode == 1 && (show == 3 || show == 4))
	{
		clan_chest = true;
	}

	for (i = list; i; i = i->get_next_content())
	{
		if (CAN_SEE_OBJ(ch, i))
		{
			if (!push)
			{
				push = i;
				push_count = 1;
			}
			else if ((!equal_obj(i, push)) 
				|| (quest_item(i)))
			{
				if (clan_chest)
				{
					buffer << show_obj_to_char(push, ch, mode, show, push_count);
					count += push_count;
					cost += GET_OBJ_RENTEQ(push) * push_count;
				}
				else
					show_obj_to_char(push, ch, mode, show, push_count);
				push = i;
				push_count = 1;
			}
			else
				push_count++;
			found = TRUE;
		}
	}
	if (push && push_count)
	{
		if (clan_chest)
		{
			buffer << show_obj_to_char(push, ch, mode, show, push_count);
			count += push_count;
			cost += GET_OBJ_RENTEQ(push) * push_count;
		}
		else
			show_obj_to_char(push, ch, mode, show, push_count);
	}
	if (!found && show)
	{
		if (show == 1)
			send_to_char(" Внутри ничего нет.\r\n", ch);
		else if (show == 2)
			send_to_char(" Вы ничего не несете.\r\n", ch);
		else if (show == 3)
		{
			send_to_char(" Пусто...\r\n", ch);
			return;
		}
	}
	if (clan_chest)
		page_string(ch->desc, buffer.str());
}

void diag_char_to_char(CHAR_DATA * i, CHAR_DATA * ch)
{
	int percent;

	if (GET_REAL_MAX_HIT(i) > 0)
		percent = (100 * GET_HIT(i)) / GET_REAL_MAX_HIT(i);
	else
		percent = -1;	// How could MAX_HIT be < 1??

	strcpy(buf, PERS(i, ch, 0));
	CAP(buf);

	if (percent >= 100)
	{
		sprintf(buf2, " невредим%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 90)
	{
		sprintf(buf2, " слегка поцарапан%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 75)
	{
		sprintf(buf2, " легко ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 50)
	{
		sprintf(buf2, " ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 30)
	{
		sprintf(buf2, " тяжело ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 15)
	{
		sprintf(buf2, " смертельно ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 0)
		strcat(buf, " в ужасном состоянии");
	else
		strcat(buf, " умирает");

	if (!on_horse(i))
		switch (GET_POS(i))
			{
			case POS_MORTALLYW:
				strcat(buf, ".");
			break;
			case POS_INCAP:
				strcat(buf, IS_POLY(i) ? ", лежат без сознания." : ", лежит без сознания.");
			break;
			case POS_STUNNED:
				strcat(buf, IS_POLY(i) ? ", лежат в обмороке." : ", лежит в обмороке.");
			break;
			case POS_SLEEPING:
				strcat(buf, IS_POLY(i) ? ", спят." : ", спит.");
			break;
			case POS_RESTING:
				strcat(buf, IS_POLY(i) ? ", отдыхают." : ", отдыхает.");
			break;
			case POS_SITTING:
				strcat(buf, IS_POLY(i) ? ", сидят." : ", сидит.");
			break;
			case POS_STANDING:
				strcat(buf, IS_POLY(i) ? ", стоят." : ", стоит.");
			break;
			case POS_FIGHTING:
				if (i->get_fighting())
					strcat(buf, IS_POLY(i) ? ", сражаются." : ", сражается.");
				else
					strcat(buf, IS_POLY(i) ? ", махают кулаками." : ", махает кулаками.");
			break;
			default:
				return;
			break;
		}
	else
		strcat(buf, IS_POLY(i) ? ", сидят верхом." : ", сидит верхом.");

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON))
		if (AFF_FLAGGED(i, EAffectFlag::AFF_POISON))
		{
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

	if (i->player_data.description != "")
	{
		if (IS_NPC(i))
			send_to_char(ch, " * %s", i->player_data.description.c_str());
		else
			send_to_char(ch, "*\r\n%s*\r\n", space_before_string(i->player_data.description).c_str());
	}
	else if (!IS_NPC(i))
	{
		strcpy(buf, "\r\nЭто");
		if (i->is_morphed())
			strcat(buf, string(" "+i->get_morph_desc()+".\r\n").c_str());
		else
		if (IS_FEMALE(i))
		{
			if (GET_HEIGHT(i) <= 151)
			{
				if (GET_WEIGHT(i) >= 140)
					strcat(buf, " маленькая плотная дамочка.\r\n");
				else if (GET_WEIGHT(i) >= 125)
					strcat(buf, " маленькая женщина.\r\n");
				else
					strcat(buf, " миниатюрная дамочка.\r\n");
			}
			else if (GET_HEIGHT(i) <= 159)
			{
				if (GET_WEIGHT(i) >= 145)
					strcat(buf, " невысокая плотная мадам.\r\n");
				else if (GET_WEIGHT(i) >= 130)
					strcat(buf, " невысокая женщина.\r\n");
				else
					strcat(buf, " изящная леди.\r\n");
			}
			else if (GET_HEIGHT(i) <= 165)
			{
				if (GET_WEIGHT(i) >= 145)
					strcat(buf, " среднего роста женщина.\r\n");
				else
					strcat(buf, " среднего роста изящная красавица.\r\n");
			}
			else if (GET_HEIGHT(i) <= 175)
			{
				if (GET_WEIGHT(i) >= 150)
					strcat(buf, " высокая дородная баба.\r\n");
				else if (GET_WEIGHT(i) >= 135)
					strcat(buf, " высокая стройная женщина.\r\n");
				else
					strcat(buf, " высокая изящная женщина.\r\n");
			}
			else
			{
				if (GET_WEIGHT(i) >= 155)
					strcat(buf, " очень высокая крупная дама.\r\n");
				else if (GET_WEIGHT(i) >= 140)
					strcat(buf, " очень высокая стройная женщина.\r\n");
				else
					strcat(buf, " очень высокая худощавая женщина.\r\n");
			}
		}
		else
		{
			if (GET_HEIGHT(i) <= 165)
			{
				if (GET_WEIGHT(i) >= 170)
					strcat(buf, " маленький, похожий на колобок, мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 150)
					strcat(buf, " маленький плотный мужчина.\r\n");
				else
					strcat(buf, " маленький плюгавенький мужичонка.\r\n");
			}
			else if (GET_HEIGHT(i) <= 175)
			{
				if (GET_WEIGHT(i) >= 175)
					strcat(buf, " невысокий коренастый крепыш.\r\n");
				else if (GET_WEIGHT(i) >= 160)
					strcat(buf, " невысокий крепкий мужчина.\r\n");
				else
					strcat(buf, " невысокий худощавый мужчина.\r\n");
			}
			else if (GET_HEIGHT(i) <= 185)
			{
				if (GET_WEIGHT(i) >= 180)
					strcat(buf, " среднего роста коренастый мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 165)
					strcat(buf, " среднего роста крепкий мужчина.\r\n");
				else
					strcat(buf, " среднего роста худощавый мужчина.\r\n");
			}
			else if (GET_HEIGHT(i) <= 195)
			{
				if (GET_WEIGHT(i) >= 185)
					strcat(buf, " высокий крупный мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 170)
					strcat(buf, " высокий стройный мужчина.\r\n");
				else
					strcat(buf, " длинный, худощавый мужчина.\r\n");
			}
			else
			{
				if (GET_WEIGHT(i) >= 190)
					strcat(buf, " огромный мужик.\r\n");
				else if (GET_WEIGHT(i) >= 180)
					strcat(buf, " очень высокий, крупный амбал.\r\n");
				else
					strcat(buf, " длиннющий, похожий на жердь мужчина.\r\n");
			}
		}
		send_to_char(buf, ch);
	}
	else
		act("\r\nНичего необычного в $n5 вы не заметили.", FALSE, i, 0, ch, TO_VICT);

	if (AFF_FLAGGED(i, EAffectFlag::AFF_CHARM)
		&& i->get_master() == ch)
	{
		if (i->low_charm())
		{
			act("$n скоро перестанет следовать за вами.", FALSE, i, 0, ch, TO_VICT);
		}
		else
		{
			for (const auto& aff : i->affected)
			{
				if (aff->type == SPELL_CHARM)
				{
					sprintf(buf, IS_POLY(i) ? "$n будут слушаться вас еще %d %s." : "$n будет слушаться вас еще %d %s.", aff->duration / 2, desc_count(aff->duration / 2, 1));
					act(buf, FALSE, i, 0, ch, TO_VICT);
					break;
				}
			}
		}
	}

	if (IS_HORSE(i)
		&& i->get_master() == ch)
	{
		strcpy(buf, "\r\nЭто ваш скакун. Он ");
		if (GET_HORSESTATE(i) <= 0)
			strcat(buf, "загнан.\r\n");
		else if (GET_HORSESTATE(i) <= 20)
			strcat(buf, "весь в мыле.\r\n");
		else if (GET_HORSESTATE(i) <= 80)
			strcat(buf, "в хорошем состоянии.\r\n");
		else
			strcat(buf, "выглядит совсем свежим.\r\n");
		send_to_char(buf, ch);
	};

	diag_char_to_char(i, ch);

	if (i->is_morphed())
	{
		send_to_char("\r\n", ch);
		std::string coverDesc = "$n покрыт$a " + i->get_cover_desc()+".";
		act(coverDesc.c_str(), FALSE, i, 0, ch, TO_VICT);
		send_to_char("\r\n", ch);
	}
	else
	{
		found = FALSE;
		for (j = 0; !found && j < NUM_WEARS; j++)
			if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
				found = TRUE;

		if (found)
		{
			send_to_char("\r\n", ch);
			act("$n одет$a :", FALSE, i, 0, ch, TO_VICT);
			for (j = 0; j < NUM_WEARS; j++)
			{
				if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
				{
					send_to_char(where[j], ch);
					if (i->has_master()
						&& IS_NPC(i))
					{
						show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i->get_master(), 1);
					}
					else
					{
						show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i, 1);
					}
				}
			}
		}
	}

	if (ch != i && (ch->get_skill(SKILL_LOOK_HIDE) || IS_IMMORTAL(ch)))
	{
		found = FALSE;
		act("\r\nВы попытались заглянуть в $s ношу:", FALSE, i, 0, ch, TO_VICT);
		for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->get_next_content())
		{
			if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 30) < GET_LEVEL(ch)))
			{
				if (!push)
				{
					push = tmp_obj;
					push_count = 1;
				}
				else if (GET_OBJ_VNUM(push) != GET_OBJ_VNUM(tmp_obj)
						 || GET_OBJ_VNUM(push) == -1)
				{
					show_obj_to_char(push, ch, 1, ch == i, push_count);
					push = tmp_obj;
					push_count = 1;
				}
				else
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
	const char *positions[] =
	{
		"лежит здесь, мертвый. ",
		"лежит здесь, при смерти. ",
		"лежит здесь, без сознания. ",
		"лежит здесь, в обмороке. ",
		"спит здесь. ",
		"отдыхает здесь. ",
		"сидит здесь. ",
		"СРАЖАЕТСЯ! ",
		"стоит здесь. "
	};

	// Здесь и далее при использовании IS_POLY() - патч для отображения позиций мобов типа "они" -- Ковшегуб
	const char *poly_positions[] =
	{
		"лежат здесь, мертвые. ",
		"лежат здесь, при смерти. ",
		"лежат здесь, без сознания. ",
		"лежат здесь, в обмороке. ",
		"спят здесь. ",
		"отдыхают здесь. ",
		"сидят здесь. ",
		"СРАЖАЮТСЯ! ",
		"стоят здесь. "
	};

	if (IS_HORSE(i) && on_horse(i->get_master()))
	{
		if (ch == i->get_master())
		{
			if (!IS_POLY(i))
			{
				act("$N несет вас на своей спине.", FALSE, ch, 0, i, TO_CHAR);
			}
			else
			{
				act("$N несут вас на своей спине.", FALSE, ch, 0, i, TO_CHAR);
			}
		}

		return;
	}

	if (skill_mode == SKILL_LOOKING)
	{
		if (HERE(i) && INVIS_OK(ch, i) && GET_REAL_LEVEL(ch) >= (IS_NPC(i) ? 0 : GET_INVIS_LEV(i)))
		{
			if (GET_RACE(i)==NPC_RACE_THING && IS_IMMORTAL(ch)) {
				sprintf(buf, "Вы разглядели %s.(предмет)\r\n", GET_PAD(i, 3));
			} else {
				sprintf(buf, "Вы разглядели %s.\r\n", GET_PAD(i, 3));
			}
			send_to_char(buf, ch);
		}
		return;
	}

	if (!CAN_SEE(ch, i))
	{
		skill_mode =
			check_awake(i, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
		*buf = 0;
		if (IS_SET(skill_mode, ACHECK_AFFECTS))
		{
			REMOVE_BIT(skill_mode, ACHECK_AFFECTS);
			sprintf(buf + strlen(buf), "магический ореол%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_LIGHT))
		{
			REMOVE_BIT(skill_mode, ACHECK_LIGHT);
			sprintf(buf + strlen(buf), "яркий свет%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_GLOWING)
				&& IS_SET(skill_mode, ACHECK_HUMMING)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
		{
			REMOVE_BIT(skill_mode, ACHECK_GLOWING);
			REMOVE_BIT(skill_mode, ACHECK_HUMMING);
			sprintf(buf + strlen(buf), "шум и блеск экипировки%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_GLOWING))
		{
			REMOVE_BIT(skill_mode, ACHECK_GLOWING);
			sprintf(buf + strlen(buf), "блеск экипировки%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_HUMMING)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
		{
			REMOVE_BIT(skill_mode, ACHECK_HUMMING);
			sprintf(buf + strlen(buf), "шум экипировки%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_WEIGHT)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
		{
			REMOVE_BIT(skill_mode, ACHECK_WEIGHT);
			sprintf(buf + strlen(buf), "бряцание металла%s", skill_mode ? ", " : " ");
		}
		strcat(buf, "выдает чье-то присутствие.\r\n");
		send_to_char(CAP(buf), ch);
		return;
	}

	if (IS_NPC(i)
		&& i->player_data.long_descr != ""
		&& GET_POS(i) == GET_DEFAULT_POS(i)
		&& ch->in_room == i->in_room
		&& !AFF_FLAGGED(i, EAffectFlag::AFF_CHARM)
		&& !IS_HORSE(i))
	{
		*buf = '\0';
		if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
		{
			sprintf(buf, "[%5d] ", GET_MOB_VNUM(i));
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
		{
			if (AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
			{
				strcat(buf, "(черная аура) ");
			}
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
		{
			if (IS_NPC(i))
			{
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
		if (AFF_FLAGGED(i, EAffectFlag::AFF_INVISIBLE))
			sprintf(buf + strlen(buf), "(невидим%s) ", GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HIDE))
			sprintf(buf + strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_CAMOUFLAGE))
			sprintf(buf + strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_FLY))
			strcat(buf, IS_POLY(i) ? "(летят) " : "(летит) ");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HORSE))
			strcat(buf, "(под седлом) ");

		strcat(buf, i->player_data.long_descr.c_str());
		send_to_char(buf, ch);

		*aura_txt = '\0';
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SHIELD))
		{
			strcat(aura_txt, "...окутан");
			strcat(aura_txt, GET_CH_SUF_6(i));
			strcat(aura_txt, " сверкающим коконом ");
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SANCTUARY))
			strcat(aura_txt, IS_POLY(i) ? "...светятся ярким сиянием " : "...светится ярким сиянием ");
		else if (AFF_FLAGGED(i, EAffectFlag::AFF_PRISMATICAURA))
			strcat(aura_txt, IS_POLY(i) ? "...переливаются всеми цветами " : "...переливается всеми цветами ");
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, "...окружен");
		strcat(aura_txt, GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRSHIELD))
		{
			strcat(aura_txt, " воздушным");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_FIRESHIELD))
		{
			if (n > 0)
				strcat(aura_txt, ", огненным");
			else
				strcat(aura_txt, " огненным");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_ICESHIELD))
		{
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

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
		{
			*aura_txt = '\0';
			n = 0;
			strcat(aura_txt, "...");
			if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRAURA))
			{
				strcat(aura_txt, "воздушная");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_FIREAURA))
			{
				if (n > 0)
					strcat(aura_txt, ", огненная");
				else
					strcat(aura_txt, "огненная");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_ICEAURA))
			{
				if (n > 0)
					strcat(aura_txt, ", ледяная");
				else
					strcat(aura_txt, "ледяная");
				n++;
                        }
			if (AFF_FLAGGED(i, EAffectFlag::AFF_EARTHAURA))
			{
				if (n > 0)
					strcat(aura_txt, ", коричневая");
				else
					strcat(aura_txt, "коричневая");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_MAGICGLASS))
			{
				if (n > 0)
					strcat(aura_txt, ", серебристая");
				else
					strcat(aura_txt, "серебристая");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_BROKEN_CHAINS))
			{
				if (n > 0)
					strcat(aura_txt, ", ярко-синяя");
				else
					strcat(aura_txt, "ярко-синяя");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
			{
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
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
		{
			if (AFF_FLAGGED(i, EAffectFlag::AFF_HOLD))
				strcat(aura_txt, "...парализован$a");
			if (AFF_FLAGGED(i, EAffectFlag::AFF_SILENCE))
				strcat(aura_txt, "...нем$a");
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_BLIND))
			strcat(aura_txt, "...слеп$a");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS))
			strcat(aura_txt, "...глух$a");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_STRANGLED))
			strcat(aura_txt, "...задыхается.");

		if (*aura_txt)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		return;
	}

	if (IS_NPC(i))
	{
		strcpy(buf1, i->get_npc_name().c_str());
		strcat(buf1, " ");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HORSE))
			strcat(buf1, "(под седлом) ");
		CAP(buf1);
	}
	else
	{
		sprintf(buf1, "%s%s ", i->get_morphed_title().c_str(), PLR_FLAGGED(i, PLR_KILLER) ? " <ДУШЕГУБ>" : "");
	}

	sprintf(buf, "%s%s", AFF_FLAGGED(i, EAffectFlag::AFF_CHARM) ? "*" : "", buf1);
	if (AFF_FLAGGED(i, EAffectFlag::AFF_INVISIBLE))
		sprintf(buf + strlen(buf), "(невидим%s) ", GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, EAffectFlag::AFF_HIDE))
		sprintf(buf + strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
	if (AFF_FLAGGED(i, EAffectFlag::AFF_CAMOUFLAGE))
		sprintf(buf + strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
	if (!IS_NPC(i) && !i->desc)
		sprintf(buf + strlen(buf), "(потерял%s связь) ", GET_CH_SUF_1(i));
	if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
		strcat(buf, "(пишет) ");

	if (GET_POS(i) != POS_FIGHTING)
	{
		if (on_horse(i))
		{
			CHAR_DATA *horse = get_horse(i);
			if (horse)
			{
				const char *msg =
					AFF_FLAGGED(horse, EAffectFlag::AFF_FLY) ? "летает" : "сидит";
				sprintf(buf + strlen(buf), "%s здесь верхом на %s. ",
					msg, PERS(horse, ch, 5));
			}
		}
		else if (IS_HORSE(i) && AFF_FLAGGED(i, EAffectFlag::AFF_TETHERED))
			sprintf(buf + strlen(buf), "привязан%s здесь. ", GET_CH_SUF_6(i));
		else if ((sector = real_sector(i->in_room)) == SECT_FLYING)
			strcat(buf, IS_POLY(i) ? "летают здесь. " : "летает здесь. ");
		else if (sector == SECT_UNDERWATER)
			strcat(buf, IS_POLY(i) ? "плавают здесь. " : "плавает здесь. ");
		else if (GET_POS(i) > POS_SLEEPING && AFF_FLAGGED(i, EAffectFlag::AFF_FLY))
			strcat(buf, IS_POLY(i) ? "летают здесь. " : "летает здесь. ");
		else if (sector == SECT_WATER_SWIM || sector == SECT_WATER_NOSWIM)
			strcat(buf, IS_POLY(i) ? "плавают здесь. " : "плавает здесь. ");
		else
			strcat(buf, IS_POLY(i) ? poly_positions[static_cast<int>(GET_POS(i))] : positions[static_cast<int>(GET_POS(i))]);
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC) && IS_NPC(i) && affected_by_spell(i, SPELL_CAPABLE))
			sprintf(buf + strlen(buf), "(аура магии) ");
	}
	else
	{
		if (i->get_fighting())
		{
			strcat(buf, IS_POLY(i) ? "сражаются с " : "сражается с ");
			if (i->in_room != i->get_fighting()->in_room)
				strcat(buf, "чьей-то тенью");
			else if (i->get_fighting() == ch)
				strcat(buf, "ВАМИ");
			else
				strcat(buf, GET_PAD(i->get_fighting(), 4));
			if (on_horse(i))
				sprintf(buf + strlen(buf), ", сидя верхом на %s! ", PERS(get_horse(i), ch, 5));
			else
				strcat(buf, "! ");
		}
		else		// NIL fighting pointer
		{
			strcat(buf, IS_POLY(i) ? "колотят по воздуху" : "колотит по воздуху");
			if (on_horse(i))
				sprintf(buf + strlen(buf), ", сидя верхом на %s. ", PERS(get_horse(i), ch, 5));
			else
				strcat(buf, ". ");
		}
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
	{
		if (AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
			strcat(buf, "(черная аура) ");
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
	{
		if (IS_NPC(i))
		{
			if (IS_EVIL(i))
			{
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
						&& AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
					strcat(buf, "(иссиня-черная аура) ");
				else
					strcat(buf, "(темная аура) ");
			}
			else if (IS_GOOD(i))
			{
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
						&& AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
					strcat(buf, "(серая аура) ");
				else
					strcat(buf, "(светлая аура) ");
			}
			else
			{
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
						&& AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
					strcat(buf, "(черная аура) ");
			}
		}
		else
		{
			aura(ch, C_CMP, i, aura_txt);
			strcat(buf, aura_txt);
			strcat(buf, " ");
		}
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON))
		if (AFF_FLAGGED(i, EAffectFlag::AFF_POISON))
			sprintf(buf + strlen(buf), "(отравлен%s) ", GET_CH_SUF_6(i));

	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	*aura_txt = '\0';
	if (AFF_FLAGGED(i, EAffectFlag::AFF_SHIELD))
	{
		strcat(aura_txt, "...окутан");
		strcat(aura_txt, GET_CH_SUF_6(i));
		strcat(aura_txt, " сверкающим коконом ");
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_SANCTUARY))
		strcat(aura_txt, IS_POLY(i) ? "...светятся ярким сиянием " : "...светится ярким сиянием ");
	else if (AFF_FLAGGED(i, EAffectFlag::AFF_PRISMATICAURA))
		strcat(aura_txt, IS_POLY(i) ? "...переливаются всеми цветами " : "...переливается всеми цветами ");
	act(aura_txt, FALSE, i, 0, ch, TO_VICT);

	*aura_txt = '\0';
	n = 0;
	strcat(aura_txt, "...окружен");
	strcat(aura_txt, GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRSHIELD))
	{
		strcat(aura_txt, " воздушным");
		n++;
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_FIRESHIELD))
	{
		if (n > 0)
			strcat(aura_txt, ", огненным");
		else
			strcat(aura_txt, " огненным");
		n++;
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_ICESHIELD))
	{
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
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
	{
	*aura_txt = '\0';
	if (AFF_FLAGGED(i, EAffectFlag::AFF_COMMANDER))
		strcat(aura_txt, "... реет стяг над головой ");
	if (*aura_txt)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
	{
		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, " ..");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRAURA))
		{
			strcat(aura_txt, "воздушная");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_FIREAURA))
		{
			if (n > 0)
				strcat(aura_txt, ", огненная");
			else
				strcat(aura_txt, "огненная");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_ICEAURA))
		{
			if (n > 0)
				strcat(aura_txt, ", ледяная");
			else
				strcat(aura_txt, "ледяная");
			n++;
                }
		if (AFF_FLAGGED(i, EAffectFlag::AFF_EARTHAURA))
		{
			if (n > 0)
				strcat(aura_txt, ", коричневая");
			else
				strcat(aura_txt, "коричневая");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_MAGICGLASS))
		{
			if (n > 0)
				strcat(aura_txt, ", серебристая");
			else
				strcat(aura_txt, "серебристая");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_BROKEN_CHAINS))
		{
			if (n > 0)
				strcat(aura_txt, ", ярко-синяя");
			else
				strcat(aura_txt, "ярко-синяя");
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
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
	{
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HOLD))
			strcat(aura_txt, " ...парализован$a");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SILENCE))
			strcat(aura_txt, " ...нем$a");
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_BLIND))
		strcat(aura_txt, " ...слеп$a");
	if (AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS))
		strcat(aura_txt, " ...глух$a");
	if (AFF_FLAGGED(i, EAffectFlag::AFF_STRANGLED))
		strcat(aura_txt, " ...задыхается");
	if (*aura_txt)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);
}

void list_char_to_char(const ROOM_DATA::people_t& list, CHAR_DATA* ch)
{
	for (const auto i : list)
	{
		if (ch != i)
		{
			if (HERE(i) && (GET_RACE(i) != NPC_RACE_THING)
				&& (CAN_SEE(ch, i)
					|| awaking(i, AW_HIDE | AW_INVIS | AW_CAMOUFLAGE)))
			{
				list_one_char(i, ch, 0);
			}
			else if (IS_DARK(i->in_room)
				&& i->in_room == ch->in_room
				&& !CAN_SEE_IN_DARK(ch)
				&& AFF_FLAGGED(i, EAffectFlag::AFF_INFRAVISION))
			{
				send_to_char("Пара светящихся глаз смотрит на вас.\r\n", ch);
			}
		}
	}
}
void list_char_to_char_thing(const ROOM_DATA::people_t& list, CHAR_DATA* ch)   //мобы рассы предмет будем выводить вместе с предметами желтеньким
{
	for (const auto i : list)
	{
		if (ch != i)
		{
			if (GET_RACE(i) == NPC_RACE_THING)
			{
				list_one_char(i, ch, 0);
			}
		}
	}
}

void do_auto_exits(CHAR_DATA * ch)
{
	int door, slen = 0;

	*buf = '\0';

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		// Наконец-то добавлена отрисовка в автовыходах закрытых дверей
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != NOWHERE)
		{
			if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
			{
				slen += sprintf(buf + slen, "(%c) ", LOWER(*dirs[door]));
			}
			else if (!EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
			{
				if (world[EXIT(ch, door)->to_room()]->zone == world[ch->in_room]->zone)
				{
					slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));
				}
				else
				{
					slen += sprintf(buf + slen, "%c ", UPPER(*dirs[door]));
				}
			}
		}
	}
	sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM), *buf ? buf : "None! ", CCNRM(ch, C_NRM));

	send_to_char(buf2, ch);
}

void do_exits(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int door;

	*buf = '\0';

	if (PRF_FLAGGED(ch, PRF_BLIND))
	{
		do_blind_exits(ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы слепы, как котенок!\r\n", ch);
		return;
	}
	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != NOWHERE && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
		{
			if (IS_GOD(ch))
				sprintf(buf2, "%-5s - [%5d] %s\r\n", Dirs[door],
						GET_ROOM_VNUM(EXIT(ch, door)->to_room()), world[EXIT(ch, door)->to_room()]->name);
			else
			{
				sprintf(buf2, "%-5s - ", Dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room()) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "слишком темно\r\n");
				else
				{
					strcat(buf2, world[EXIT(ch, door)->to_room()]->name);
					strcat(buf2, "\r\n");
				}
			}
			strcat(buf, CAP(buf2));
		}
	send_to_char("Видимые выходы:\r\n", ch);
	if (*buf)
		send_to_char(buf, ch);
	else
		send_to_char(" Замуровали, ДЕМОНЫ!\r\n", ch);
}
void do_blind_exits(CHAR_DATA *ch)
{
	int door;

	*buf = '\0';

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы слепы, как котенок!\r\n", ch);
		return;
	}
	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != NOWHERE && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
		{
			if (IS_GOD(ch))
				sprintf(buf2, "&W%-5s - [%5d] %s ", Dirs[door],
						GET_ROOM_VNUM(EXIT(ch, door)->to_room()), world[EXIT(ch, door)->to_room()]->name);
			else
			{
				sprintf(buf2, "&W%-5s - ", Dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room()) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "слишком темно");
				else
				{
					strcat(buf2, world[EXIT(ch, door)->to_room()]->name);
					strcat(buf2, "");
				}
			}
			strcat(buf, CAP(buf2));
		}
	send_to_char("Видимые выходы:\r\n", ch);
	if (*buf)
		send_to_char(ch, "%s&n\r\n", buf);
	else
		send_to_char("&W Замуровали, ДЕМОНЫ!&n\r\n", ch);
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

int paste_description(char *string, const char *tag, int need)
{
	if (!*string || !*tag)
	{
		return (FALSE);
	}

	const char *pos = str_str(string, tag);
	if (!pos)
	{
		return FALSE;
	}

	if (!need)
	{
		const size_t offset = pos - string;
		string[offset] = '\0';
		pos = str_str(pos + 1, tag);
		if (pos)
		{
			auto to_pos = string + offset;
			auto from_pos = pos + strlen(tag);
			while ((*(to_pos++) = *(from_pos++)));
		}
		return FALSE;
	}

	for (; *pos && *pos != '>'; pos++);

	if (*pos)
	{
		pos++;
	}

	if (*pos == 'R')
	{
		pos++;
		buf[0] = '\0';
	}

	strcat(buf, pos);
	pos = str_str(buf, tag);
	if (pos)
	{
		const size_t offset = pos - buf;
		buf[offset] = '\0';
	}

	return (TRUE);
}


void show_extend_room(const char * const description, CHAR_DATA * ch)
{
	int found = FALSE;
	char string[MAX_STRING_LENGTH], *pos;

	if (!description || !*description)
		return;

	strcpy(string, description);
	if ((pos = strchr(string, '<')))
		* pos = '\0';
	strcpy(buf, string);
	if (pos)
		*pos = '<';

	found = found || paste_description(string, TAG_WINTERNIGHT,
									   (weather_info.season == SEASON_WINTER
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_WINTERDAY,
									   (weather_info.season == SEASON_WINTER
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_SPRINGNIGHT,
									   (weather_info.season == SEASON_SPRING
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_SPRINGDAY,
									   (weather_info.season == SEASON_SPRING
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_SUMMERNIGHT,
									   (weather_info.season == SEASON_SUMMER
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_SUMMERDAY,
									   (weather_info.season == SEASON_SUMMER
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_AUTUMNNIGHT,
									   (weather_info.season == SEASON_AUTUMN
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_AUTUMNDAY,
									   (weather_info.season == SEASON_AUTUMN
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_NIGHT,
									   (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK));
	found = found || paste_description(string, TAG_DAY,
									   (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT));

	// Trim any LF/CRLF at the end of description
	pos = buf + strlen(buf);
	while (pos > buf && *--pos == '\n')
	{
		*pos = '\0';
		if (pos > buf && *(pos - 1) == '\r')
			*--pos = '\0';
	}	

	send_to_char(buf, ch);
	send_to_char("\r\n", ch);
}

bool put_delim(std::stringstream &out, bool delim)
{
	if (!delim)
	{
		out << " (";
	}
	else
	{
		out << ", ";
	}
	return true;
}

void print_zone_info(CHAR_DATA *ch)
{
	ZoneData *zone = &zone_table[world[ch->in_room]->zone];
	std::stringstream out;
	out << "\r\n" << zone->name;

	bool delim = false;
	if (!zone->is_town)
	{
		delim = put_delim(out, delim);
		out << "средний уровень: " << zone->mob_level;
	}
	if (zone->group > 1)
	{
		delim = put_delim(out, delim);
		out << "групповая на " << zone->group
			<< " " << desc_count(zone->group, WHAT_PEOPLE);
	}
	if (delim)
	{
		out << ")";
	}
	out << ".\r\n";

	send_to_char(out.str(), ch);
}

void show_glow_objs(CHAR_DATA *ch)
{
	unsigned cnt = 0;
	for (OBJ_DATA *obj = world[ch->in_room]->contents;
		obj; obj = obj->get_next_content())
	{
		if (obj->get_extra_flag(EExtraFlag::ITEM_GLOW))
		{
			++cnt;
			if (cnt > 1)
			{
				break;
			}
		}
	}
	if (!cnt) return;

	const char *str = cnt > 1 ?
		"Вы видите очертания каких-то блестящих предметов.\r\n" :
		"Вы видите очертания какого-то блестящего предмета.\r\n";
	send_to_char(str, ch);
}

void show_room_affects(CHAR_DATA* ch, const char* name_affects[], const char* name_self_affects[])
{
	bitvector_t bitvector = 0;
	std::ostringstream buffer;

	for (const auto& af : world[ch->in_room]->affected)
	{
		switch (af->bitvector)
		{
		case AFF_ROOM_LIGHT:					// 1 << 0
			if (!IS_SET(bitvector, AFF_ROOM_LIGHT))
			{
				if (af->caster_id == ch->id && *name_self_affects[0] != '\0')
				{
					buffer << name_self_affects[0] << "\r\n";
				}
				else if(*name_affects[0] != '\0')
				{
					buffer << name_affects[0] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_LIGHT);
			}
			break;
		case AFF_ROOM_FOG:						// 1 << 1
			if (!IS_SET(bitvector, AFF_ROOM_FOG))
			{
				if (af->caster_id == ch->id && *name_self_affects[1] != '\0')
				{
					buffer << name_self_affects[1] << "\r\n";
				}
				else if (*name_affects[1] != '\0')
				{
					buffer << name_affects[1] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_FOG);
			}
			break;
		case AFF_ROOM_RUNE_LABEL:				// 1 << 2
			if (af->caster_id == ch->id && *name_self_affects[2] != '\0')
			{
				buffer << name_self_affects[2] << "\r\n";
			}
			else if (*name_affects[2] != '\0')
			{
				buffer << name_affects[2] << "\r\n";
			}
			break;
		case AFF_ROOM_FORBIDDEN:				// 1 << 3
			if (!IS_SET(bitvector, AFF_ROOM_FORBIDDEN))
			{
				if (af->caster_id == ch->id && *name_self_affects[3] != '\0')
				{
					buffer << name_self_affects[3] << "\r\n";
				}
				else if (*name_affects[3] != '\0')
				{
					buffer << name_affects[3] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_FORBIDDEN);
			}
			break;
		case AFF_ROOM_HYPNOTIC_PATTERN:			// 1 << 4
			if (!IS_SET(bitvector, AFF_ROOM_HYPNOTIC_PATTERN))
			{
				if (af->caster_id == ch->id && *name_self_affects[4] != '\0')
				{
					buffer << name_self_affects[4] << "\r\n";
				}
				else if (*name_affects[4] != '\0')
				{
					buffer << name_affects[4] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_HYPNOTIC_PATTERN);
			}
			break;
		case AFF_ROOM_EVARDS_BLACK_TENTACLES:	// 1 << 5
			if (!IS_SET(bitvector, AFF_ROOM_EVARDS_BLACK_TENTACLES))
			{
				if (af->caster_id == ch->id && *name_self_affects[5] != '\0')
				{
					buffer << name_self_affects[5] << "\r\n";
				}
				else if (*name_affects[5] != '\0')
				{
					buffer << name_affects[5] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_EVARDS_BLACK_TENTACLES);
			}
			break;
		case AFF_ROOM_METEORSTORM:				// 1 << 6
			if (!IS_SET(bitvector, AFF_ROOM_METEORSTORM))
			{
				if (af->caster_id == ch->id && *name_self_affects[6] != '\0')
				{
					buffer << name_self_affects[6] << "\r\n";
				}
				else if (*name_affects[6] != '\0')
				{
					buffer << name_affects[6] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_METEORSTORM);
			}
			break;
		case AFF_ROOM_THUNDERSTORM:				// 1 << 7
			if (!IS_SET(bitvector, AFF_ROOM_THUNDERSTORM))
			{
				if (af->caster_id == ch->id && *name_self_affects[7] != '\0')
				{
					buffer << name_self_affects[7] << "\r\n";
				}
				else if (*name_affects[7] != '\0')
				{
					buffer << name_affects[7] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_THUNDERSTORM);
			}
			break;
		default:
			log("SYSERR: Unknown room affect: %d", af->type);
		}
	}

	auto affects = buffer.str();
	if (!affects.empty())
	{
		affects.append("\r\n");
		send_to_char(affects.c_str(), ch);
	}
}

void look_at_room(CHAR_DATA * ch, int ignore_brief)
{
	if (!ch->desc)
		return;

	if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !can_use_feat(ch, DARK_READING_FEAT))
	{
		send_to_char("Слишком темно...\r\n", ch);
		show_glow_objs(ch);
		return;
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы все еще слепы...\r\n", ch);
		return;
	}
	else if (GET_POS(ch) < POS_SLEEPING)
	{
		return;
	}

	if (PRF_FLAGGED(ch, PRF_DRAW_MAP) && !PRF_FLAGGED(ch, PRF_BLIND))
	{
		MapSystem::print_map(ch);
	}
	else if (ch->desc->snoop_by
		&& ch->desc->snoop_by->snoop_with_map
		&& ch->desc->snoop_by->character)
	{
		ch->map_print_to_snooper(ch->desc->snoop_by->character.get());
	}

	send_to_char(CCICYN(ch, C_NRM), ch);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
	{
		// иммам рандомная * во флагах ломает мапер грят
		const bool has_flag = ROOM_FLAGGED(ch->in_room, ROOM_BFS_MARK) ? true : false;
		GET_ROOM(ch->in_room)->unset_flag(ROOM_BFS_MARK);

		GET_ROOM(ch->in_room)->flags_sprint(buf, ";");
		sprintf(buf2, "[%5d] %s [%s]", GET_ROOM_VNUM(ch->in_room), world[ch->in_room]->name, buf);
		send_to_char(buf2, ch);

		if (has_flag)
		{
			GET_ROOM(ch->in_room)->set_flag(ROOM_BFS_MARK);
		}
	}
	else
	{
		if (PRF_FLAGGED(ch, PRF_MAPPER))
		{
			sprintf(buf2, "%s [%d]", world[ch->in_room]->name, GET_ROOM_VNUM(ch->in_room));
			send_to_char(buf2, ch);
		}
		else    
			send_to_char(world[ch->in_room]->name, ch);
	}

	send_to_char(CCNRM(ch, C_NRM), ch);
	send_to_char("\r\n", ch);

	if (IS_DARK(ch->in_room) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
	{
		send_to_char("Слишком темно...\r\n", ch);
	}
	else if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief || ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
	{
		show_extend_room(RoomDescription::show_desc(world[ch->in_room]->description_num).c_str(), ch);
	}

	// autoexits
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
	{
		do_auto_exits(ch);
	}

	// Отображаем аффекты комнаты. После автовыходов чтобы не ломать популярный маппер.
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC) || IS_IMMORTAL(ch))
	{
		show_room_affects(ch, room_aff_invis_bits, room_self_aff_invis_bits);
	}
	else
	{
		show_room_affects(ch, room_aff_visib_bits, room_aff_visib_bits);
	}

	// now list characters & objects
	if (world[ch->in_room]->fires)
	{
		sprintf(buf, "%sВ центре %s.%s\r\n",
				CCRED(ch, C_NRM), Fires[MIN(world[ch->in_room]->fires, MAX_FIRES - 1)], CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (world[ch->in_room]->portal_time)
	{
		if (world[ch->in_room]->pkPenterUnique)
		{
			sprintf(buf, "%sЛазурная пентаграмма %sс кровавым отблеском%s ярко сверкает здесь.%s\r\n",
				CCIBLU(ch, C_NRM), CCIRED(ch, C_NRM), CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		else
		{
			sprintf(buf, "%sЛазурная пентаграмма ярко сверкает здесь.%s\r\n",
				CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		}

		send_to_char(buf, ch);
	}

	if (world[ch->in_room]->holes)
	{
		const int ar = roundup(world[ch->in_room]->holes / HOLES_TIME);
		sprintf(buf, "%sЗдесь выкопана ямка глубиной примерно в %i аршин%s.%s\r\n",
			CCYEL(ch, C_NRM), ar, (ar == 1 ? "" : (ar < 5 ? "а" : "ов")), (CCNRM(ch, C_NRM)));
		send_to_char(buf, ch);
	}

	if (ch->in_room != NOWHERE && !ROOM_FLAGGED(ch->in_room, ROOM_NOWEATHER))
	{
		*buf = '\0';
		switch (real_sector(ch->in_room))
		{
		case SECT_FIELD_SNOW:
		case SECT_FOREST_SNOW:
		case SECT_HILLS_SNOW:
		case SECT_MOUNTAIN_SNOW:
			sprintf(buf, "%sСнежный ковер лежит у вас под ногами.%s\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_FIELD_RAIN:
		case SECT_FOREST_RAIN:
		case SECT_HILLS_RAIN:
			sprintf(buf, "%sВы просто увязаете в грязи.%s\r\n", CCIWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_THICK_ICE:
			sprintf(buf, "%sУ вас под ногами толстый лед.%s\r\n", CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_NORMAL_ICE:
			sprintf(buf, "%sУ вас под ногами достаточно толстый лед.%s\r\n",
				CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_THIN_ICE:
			sprintf(buf, "%sТоненький ледок вот-вот проломится под вами.%s\r\n",
				CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		};
		if (*buf)
		{
			send_to_char(buf, ch);
		}
	}

	send_to_char("&Y&q", ch);
//  if (IS_SET(GET_SPELL_TYPE(ch, SPELL_TOWNPORTAL),SPELL_KNOW))
	if (ch->get_skill(SKILL_TOWNPORTAL))
	{
		if (find_portal_by_vnum(GET_ROOM_VNUM(ch->in_room)))
		{
			send_to_char("Рунный камень с изображением пентаграммы немного выступает из земли.\r\n", ch);
		}
	}
	list_obj_to_char(world[ch->in_room]->contents, ch, 0, FALSE);
	list_char_to_char_thing(world[ch->in_room]->people, ch);  //добавим отдельный вызов если моб типа предмет выводим желтым
	send_to_char("&R&q", ch);
	list_char_to_char(world[ch->in_room]->people, ch);
	send_to_char("&Q&n", ch);
	
	// вход в новую зону
	if (zone_table[world[ch->get_from_room()]->zone].number != zone_table[world[ch->in_room]->zone].number)
	{
		if (PRF_FLAGGED(ch, PRF_ENTER_ZONE))
			print_zone_info(ch);
		++zone_table[world[ch->in_room]->zone].traffic;
		
	}
}

int get_pick_chance(int skill_pick, int lock_complexity)
{
	return (MIN(5, MAX(-5, skill_pick - lock_complexity)) + 5);
}

void look_in_direction(CHAR_DATA * ch, int dir, int info_is)
{
	int count = 0, probe, percent;
	ROOM_DATA::exit_data_ptr rdata;

	if (CAN_GO(ch, dir)
		|| (EXIT(ch, dir)
			&& EXIT(ch, dir)->to_room() != NOWHERE))
	{
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", CCYEL(ch, C_NRM), Dirs[dir], CCNRM(ch, C_NRM));
		if (EXIT_FLAGGED(rdata, EX_CLOSED))
		{
			if (rdata->keyword)
			{
				count += sprintf(buf + count, " закрыто (%s).\r\n", rdata->keyword);
			}
			else
			{
				count += sprintf(buf + count, " закрыто (вероятно дверь).\r\n");
			}

			const int skill_pick = ch->get_skill(SKILL_PICK_LOCK) ;
			if (EXIT_FLAGGED(rdata, EX_LOCKED) && skill_pick)
			{
				if (EXIT_FLAGGED(rdata, EX_PICKPROOF))
				{
					count += sprintf(buf+count-2, "%s вы никогда не сможете ЭТО взломать!%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
				}
				else if (EXIT_FLAGGED(rdata, EX_BROKEN))
				{
					count += sprintf(buf+count-2, "%s Замок сломан... %s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
				}
				else
				{
					const int chance = get_pick_chance(skill_pick, rdata->lock_complexity);
					const int index = chance ? chance/5 + 1 : 0;

					std::string color = Locks[index][1];
					if (abs(skill_pick - rdata->lock_complexity)>10)
					{
						color = KIDRK;
					}
					if (COLOR_LEV(ch)>C_NRM)
					{
						count += sprintf(buf + count - 2, Locks[index][0], color.c_str(), KNRM);
					}
					else
					{
						count += sprintf(buf + count - 2, Locks[index][0], KNUL, KNUL);
					}
				}
			}

			send_to_char(buf, ch);
			return;
		}

		if (IS_TIMEDARK(rdata->to_room()))
		{
			count += sprintf(buf + count, " слишком темно.\r\n");
			send_to_char(buf, ch);
			if (info_is & EXIT_SHOW_LOOKING)
			{
				send_to_char("&R&q", ch);
				count = 0;
				for (const auto tch : world[rdata->to_room()]->people)
				{
					percent = number(1, skill_info[SKILL_LOOKING].max_percent);
					probe =
						train_skill(ch, SKILL_LOOKING, skill_info[SKILL_LOOKING].max_percent, tch);
					if (HERE(tch) && INVIS_OK(ch, tch) && probe >= percent
							&& (percent < 100 || IS_IMMORTAL(ch)))
					{
						// Если моб не вещь и смотрящий не им
						if ( GET_RACE(tch) != NPC_RACE_THING || IS_IMMORTAL(ch) ) {
							list_one_char(tch, ch, SKILL_LOOKING);
							count++;
						}
					}
				}

				if (!count)
				{
					send_to_char("Вы ничего не смогли разглядеть!\r\n", ch);
				}
				send_to_char("&Q&n", ch);
			}
		}
		else
		{
			if (!rdata->general_description.empty())
			{
				count += sprintf(buf + count, "%s\r\n", rdata->general_description.c_str());
			}
			else
			{
				count += sprintf(buf + count, "%s\r\n", world[rdata->to_room()]->name);
			}
			send_to_char(buf, ch);
			send_to_char("&R&q", ch);
			list_char_to_char(world[rdata->to_room()]->people, ch);
			send_to_char("&Q&n", ch);
		}
	}
	else if (info_is & EXIT_SHOW_WALL)
		send_to_char("И что вы там мечтаете увидеть?\r\n", ch);
}

void hear_in_direction(CHAR_DATA * ch, int dir, int info_is)
{
	int count = 0, percent = 0, probe = 0;
	ROOM_DATA::exit_data_ptr rdata;
	int fight_count = 0;
	string tmpstr = "";

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
	{
		send_to_char("Вы забыли, что вы глухи?\r\n", ch);
		return;
	}
	if (CAN_GO(ch, dir)
		|| (EXIT(ch, dir)
		&& EXIT(ch, dir)->to_room() != NOWHERE))
	{
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", CCYEL(ch, C_NRM), Dirs[dir], CCNRM(ch, C_NRM));
		count += sprintf(buf + count, "\r\n%s", CCGRN(ch, C_NRM));
		send_to_char(buf, ch);
		count = 0;
		for (const auto tch : world[rdata->to_room()]->people)
		{
			percent = number(1, skill_info[SKILL_HEARING].max_percent);
			probe = train_skill(ch, SKILL_HEARING, skill_info[SKILL_HEARING].max_percent, tch);
			// Если сражаются то слышем только борьбу.
			if (tch->get_fighting())
			{
				if (IS_NPC(tch))
				{
					tmpstr += " Вы слышите шум чьей-то борьбы.\r\n";
				}
				else
				{
					tmpstr += " Вы слышите звуки чьих-то ударов.\r\n";
				}
				fight_count++;
				continue;
			}

			if ((probe >= percent || ((!AFF_FLAGGED(tch, EAffectFlag::AFF_SNEAK) || !AFF_FLAGGED(tch, EAffectFlag::AFF_HIDE)) && (probe > percent * 2)))
					&& (percent < 100 || IS_IMMORTAL(ch))
					&& !fight_count)
			{
				if (IS_NPC(tch))
				{
					if (GET_RACE(tch)==NPC_RACE_THING) {
						if (GET_LEVEL(tch) < 5)
							tmpstr += " Вы слышите чье-то тихое поскрипывание.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " Вы слышите чей-то скрип.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " Вы слышите чей-то громкий скрип.\r\n";
						else
							tmpstr += " Вы слышите чей-то грозный скрип.\r\n";
					} 
					else if (real_sector(ch->in_room) != SECT_UNDERWATER)
					{
						if (GET_LEVEL(tch) < 5)
							tmpstr += " Вы слышите чью-то тихую возню.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " Вы слышите чье-то сопение.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " Вы слышите чье-то громкое дыхание.\r\n";
						else
							tmpstr += " Вы слышите чье-то грозное дыхание.\r\n";
					}
					else
					{
						if (GET_LEVEL(tch) < 5)
							tmpstr += " Вы слышите тихое бульканье.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " Вы слышите бульканье.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " Вы слышите громкое бульканье.\r\n";
						else
							tmpstr += " Вы слышите грозное пузырение.\r\n";
					}
				}
				else
				{
					tmpstr += " Вы слышите чье-то присутствие.\r\n";
				}
				count++;
			}
		}

		if ((!count) && (!fight_count))
		{
			send_to_char(" Тишина и покой.\r\n", ch);
		}
		else
		{
			send_to_char(tmpstr.c_str(), ch);
		}

		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	else
	{
		if (info_is & EXIT_SHOW_WALL)
		{
			send_to_char("И что вы там хотите услышать?\r\n", ch);
		}
	}
}

void look_in_obj(CHAR_DATA * ch, char *arg)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *dummy = NULL;
	char whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
	int amt, bits;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

	if (!*arg)
		send_to_char("Смотреть во что?\r\n", ch);
	else
		half_chop(arg, whatp, where);

	if (isname(where, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(where, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	bits = generic_find(arg, where_bits, ch, &dummy, &obj);

	if ((obj == NULL) || !bits)
	{
		sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg);
		send_to_char(buf, ch);
	}
	else if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_DRINKCON
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_FOUNTAIN
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_CONTAINER)
	{
		send_to_char("Ничего в нем нет!\r\n", ch);
	}
	else
	{
		if (Clan::ChestShow(obj, ch))
		{
			return;
		}
		if (ClanSystem::show_ingr_chest(obj, ch))
		{
			return;
		}
		if (Depot::is_depot(obj))
		{
			Depot::show_depot(ch);
			return;
		}

		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER)
		{
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
			{
				act("Закрыт$A.", FALSE, ch, obj, 0, TO_CHAR);
				const int skill_pick = ch->get_skill(SKILL_PICK_LOCK) ;
				int count = sprintf(buf, "Заперт%s.", GET_OBJ_SUF_6(obj));
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED) && skill_pick)
				{
					if (OBJVAL_FLAGGED(obj, CONT_PICKPROOF))
						count += sprintf(buf+count, "%s Вы никогда не сможете ЭТО взломать!%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
					else if (OBJVAL_FLAGGED(obj, CONT_BROKEN))
						count += sprintf(buf+count, "%s Замок сломан... %s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
					else
					{
						const int chance = get_pick_chance(skill_pick, GET_OBJ_VAL(obj, 3));
						const int index = chance ? chance/5 + 1 : 0;
						std::string color = Locks[index][1];
						if (abs(skill_pick - GET_OBJ_VAL(obj, 3))>10)
							color = KIDRK;

						if (COLOR_LEV(ch)>C_NRM)
							count += sprintf(buf + count, Locks[index][0], color.c_str(), KNRM);
						else
							count += sprintf(buf + count, Locks[index][0], KNUL, KNUL);
					}
					send_to_char(buf, ch);
				}
			}
			else
			{
				send_to_char(OBJN(obj, ch, 0), ch);
				switch (bits)
				{
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
				if (!obj->get_contains())
					send_to_char(" Внутри ничего нет.\r\n", ch);
				else
				{
					if (GET_OBJ_VAL(obj, 0) > 0 && bits != FIND_OBJ_ROOM) {
						/* amt - индекс массива из 6 элементов (0..5) с описанием наполненности
						   с помощью нехитрых мат. преобразований мы получаем соотношение веса и максимального объема контейнера,
						   выраженные числами от 0 до 5. (причем 5 будет лишь при полностью полном контейнере)
						*/
						amt = MAX(0, MIN(5, (GET_OBJ_WEIGHT(obj) * 100) / (GET_OBJ_VAL(obj, 0) *  20)));
						//sprintf(buf, "DEBUG 1: %d 2: %d 3: %d.\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_VAL(obj, 0), amt);
						//send_to_char(buf, ch);
						sprintf(buf, "Заполнен%s содержимым %s:\r\n", GET_OBJ_SUF_6(obj), fullness[amt]);
						send_to_char(buf, ch);
					}
					list_obj_to_char(obj->get_contains(), ch, 1, bits != FIND_OBJ_ROOM);
				}
			}
		}
		else  	// item must be a fountain or drink container
		{
			if (GET_OBJ_VAL(obj, 1) <= 0)
				send_to_char("Пусто.\r\n", ch);
			else
			{
				if (GET_OBJ_VAL(obj, 0) <= 0 || GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0))
				{
					sprintf(buf, "Заполнен%s вакуумом?!\r\n", GET_OBJ_SUF_6(obj));	// BUG
				}
				else
				{
					const char* msg = AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON)
						&& obj->get_val(3) == 1 ? "(отравленной)" : "";
					amt = (GET_OBJ_VAL(obj, 1) * 5) / GET_OBJ_VAL(obj, 0);
					sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
					sprintf(buf, "Наполнен%s %s%s%s жидкостью.\r\n", GET_OBJ_SUF_6(obj), fullness[amt], buf2, msg);
				}
				send_to_char(buf, ch);
			}
		}
	}
}

char *find_exdesc(char *word, const EXTRA_DESCR_DATA::shared_ptr& list)
{
	for (auto i = list; i; i = i->next)
	{
		if (isname(word, i->keyword))
		{
			return i->description;
		}
	}

	return nullptr;
}
const char *diag_liquid_timer(const OBJ_DATA* obj)
{	int tm;
	if (GET_OBJ_VAL(obj, 3) == 1)
		return "испортилось!";
	if (GET_OBJ_VAL(obj, 3) == 0)
		return "идеальное.";
	tm = (GET_OBJ_VAL(obj, 3));
	if (tm < 1440) // сутки
		return "скоро испортится!";
	else if (tm < 10080) //неделя
		return "сомнительное.";
	else if (tm < 20160) // 2 недели
		return "выглядит свежим.";
	else if (tm < 30240) // 3 недели
		return "свежее.";
	return "идеальное.";
}

//ф-ция вывода доп инфы об объекте
//buf это буфер в который дописывать инфу, в нем уже может быть что-то иначе надо перед вызовом присвоить *buf='\0'
void obj_info(CHAR_DATA * ch, OBJ_DATA *obj, char buf[MAX_STRING_LENGTH])
{
	int j;
		if (can_use_feat(ch, SKILLED_TRADER_FEAT) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)|| ch->get_skill(SKILL_INSERTGEM))
		{
			sprintf(buf+strlen(buf), "Материал : %s", CCCYN(ch, C_NRM));
			sprinttype(obj->get_material(), material_name, buf+strlen(buf));
			sprintf(buf+strlen(buf), "\r\n%s", CCNRM(ch, C_NRM));
		}

		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MING
			&& (can_use_feat(ch, BREW_POTION_FEAT)
				|| PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
		{
			for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;)
			{
				j++;
			}
			sprintf(buf+strlen(buf), "Это ингредиент вида '%s'.\r\n", imtypes[j].name);
			const int imquality = GET_OBJ_VAL(obj, IM_POWER_SLOT);
			if (GET_LEVEL(ch) >= imquality)
			{
				sprintf(buf+strlen(buf), "Качество ингредиента ");
				if (imquality > 25)
					strcat(buf+strlen(buf), "наилучшее.\r\n");
				else if (imquality > 20)
					strcat(buf+strlen(buf), "отличное.\r\n");
				else if (imquality > 15)
					strcat(buf+strlen(buf), "очень хорошее.\r\n");
				else if (imquality > 10)
					strcat(buf+strlen(buf), "выше среднего.\r\n");
				else if (imquality > 5)
					strcat(buf+strlen(buf), "весьма посредственное.\r\n");
				else
					strcat(buf+strlen(buf), "хуже не бывает.\r\n");
			}
			else
			{
				strcat(buf+strlen(buf), "Вы не в состоянии определить качество этого ингредиента.\r\n");
			}
		}

 		//|| PRF_FLAGGED(ch, PRF_HOLYLIGHT)
		if (can_use_feat(ch, MASTER_JEWELER_FEAT))
		{
			sprintf(buf+strlen(buf), "Слоты : %s", CCCYN(ch, C_NRM));
			if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WITH3SLOTS))
			{
				strcat(buf, "доступно 3 слота\r\n");
			}
			else if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WITH2SLOTS))
			{
				strcat(buf, "доступно 2 слота\r\n");
			}
			else if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WITH1SLOT))
			{
				strcat(buf, "доступен 1 слот\r\n");
			}
			else
			{
				strcat(buf, "нет слотов\r\n");
			}
			sprintf(buf+strlen(buf), "\r\n%s", CCNRM(ch, C_NRM));
		}
		if (AUTH_CUSTOM_LABEL(obj, ch) && obj->get_custom_label()->label_text)
		{
			if (obj->get_custom_label()->clan)
			{
				strcat(buf, "Метки дружины: ");
			}
			else
			{
				strcat(buf, "Ваши метки: ");
			}
			sprintf(buf + strlen(buf), "%s\r\n", obj->get_custom_label()->label_text);
		}
		sprintf(buf+strlen(buf), "%s", diag_uses_to_char(obj, ch));
		sprintf(buf+strlen(buf), "%s", diag_shot_to_char(obj, ch));
		if (GET_OBJ_VNUM(obj) >= DUPLICATE_MINI_SET_VNUM)
		{
			sprintf(buf + strlen(buf), "Светится белым сиянием.\r\n");
		}

		if (((GET_OBJ_TYPE(obj) == CObjectPrototype::ITEM_DRINKCON)
			&& (GET_OBJ_VAL(obj, 1) > 0))
			|| (GET_OBJ_TYPE(obj) == CObjectPrototype::ITEM_FOOD))
		{
			sprintf(buf1, "Качество: %s\r\n", diag_liquid_timer(obj));
			strcat(buf, buf1);
		}
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 * \return флаг если смотрим в клан-сундук, чтобы после осмотра не смотреть второй раз по look_in_obj
 */
bool look_at_target(CHAR_DATA * ch, char *arg, int subcmd)
{
	int bits, found = FALSE, fnum, i = 0, cn = 0;
	struct portals_list_type *port;
	CHAR_DATA *found_char = NULL;
	OBJ_DATA *found_obj = NULL;
	struct char_portal_type *tmp;
	char *desc, *what, whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_OBJ_EXDESC;

	if (!ch->desc)
	{
		return false;
	}

	if (!*arg)
	{
		send_to_char("На что вы так мечтаете посмотреть?\r\n", ch);
		return false;
	}

	half_chop(arg, whatp, where);
	what = whatp;

	if (isname(where, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
	else if (isname(where, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	// для townportal
	if (isname(whatp, "камень") &&
//       IS_SET(GET_SPELL_TYPE(ch, SPELL_TOWNPORTAL), SPELL_KNOW) &&
			ch->get_skill(SKILL_TOWNPORTAL) &&
			(port = get_portal(GET_ROOM_VNUM(ch->in_room), NULL)) != NULL && IS_SET(where_bits, FIND_OBJ_ROOM))
	{
		if (GET_LEVEL(ch) < MAX(1, port->level - GET_REMORT(ch) / 2))
		{
			send_to_char("На камне что-то написано огненными буквами.\r\n", ch);
			send_to_char("Но вы еще недостаточно искусны, чтобы разобрать слово.\r\n", ch);
			return false;
		}
		else
		{
			for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next)
			{
				cn++;
			}
			if (cn >= MAX_PORTALS(ch))
			{
				send_to_char
				("Все доступные вам камни уже запомнены, удалите и попробуйте еще.\r\n", ch);
				return false;
			}
			send_to_char("На камне огненными буквами написано слово '&R", ch);
			send_to_char(port->wrd, ch);
			send_to_char("&n'.\r\n", ch);
			// теперь добавляем в память чара
			add_portal_to_char(ch, GET_ROOM_VNUM(ch->in_room));
			check_portals(ch);
			return false;
		}
	}

	// заглянуть в пентаграмму
	if (isname(whatp, "пентаграмма") && world[ch->in_room]->portal_time && IS_SET(where_bits, FIND_OBJ_ROOM))
	{
		const auto r = ch->in_room;
		const auto to_room = world[r]->portal_room;
		send_to_char("Приблизившись к пентаграмме, вы осторожно заглянули в нее.\r\n\r\n", ch);
		act("$n0 осторожно заглянул$g в пентаграмму.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		if (world[to_room]->portal_time && (r == world[to_room]->portal_room))
		{
			send_to_char
			("Яркий свет, идущий с противоположного конца прохода, застилает вам глаза.\r\n\r\n", ch);
			return false;
		}
		ch->in_room = world[ch->in_room]->portal_room;
		look_at_room(ch, 1);
		ch->in_room = r;
		return false;
	}

	bits = generic_find(what, where_bits, ch, &found_char, &found_obj);
	// Is the target a character?
	if (found_char != NULL)
	{
		if (subcmd == SCMD_LOOK_HIDE && !check_moves(ch, LOOKHIDE_MOVES))
			return false;
		look_at_char(found_char, ch);
		if (ch != found_char)
		{
			if (subcmd == SCMD_LOOK_HIDE && ch->get_skill(SKILL_LOOK_HIDE) > 0)
			{
				fnum = number(1, skill_info[SKILL_LOOK_HIDE].max_percent);
				found =
					train_skill(ch, SKILL_LOOK_HIDE,
								skill_info[SKILL_LOOK_HIDE].max_percent, found_char);
				if (!WAITLESS(ch))
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				if (found >= fnum && (fnum < 100 || IS_IMMORTAL(ch)) && !IS_IMMORTAL(found_char))
					return false;
			}
			if (CAN_SEE(found_char, ch))
				act("$n оглядел$g вас с головы до пят.", TRUE, ch, 0, found_char, TO_VICT);
			act("$n посмотрел$g на $N3.", TRUE, ch, 0, found_char, TO_NOTVICT);
		}
		return false;
	}

	// Strip off "number." from 2.foo and friends.
	if (!(fnum = get_number(&what)))
	{
		send_to_char("Что осматриваем?\r\n", ch);
		return false;
	}

	// Does the argument match an extra desc in the room?
	if ((desc = find_exdesc(what, world[ch->in_room]->ex_description)) != NULL && ++i == fnum)
	{
		page_string(ch->desc, desc, FALSE);
		return false;
	}

	// If an object was found back in generic_find
	if (bits && (found_obj != NULL))
	{

		if (Clan::ChestShow(found_obj, ch))
		{
			return true;
		}
		if (ClanSystem::show_ingr_chest(found_obj, ch))
		{
			return true;
		}
		if (Depot::is_depot(found_obj))
		{
			Depot::show_depot(ch);
			return true;
		}

		// Собственно изменение. Вместо проверки "if (!found)" юзается проверка
		// наличия описания у объекта, найденного функцией "generic_find"
		if (!(desc = find_exdesc(what, found_obj->get_ex_description())))
		{
			show_obj_to_char(found_obj, ch, 5, TRUE, 1);	// Show no-description
		}
		else
		{
			send_to_char(desc, ch);
			show_obj_to_char(found_obj, ch, 6, TRUE, 1);	// Find hum, glow etc
		}

		*buf = '\0';
		obj_info(ch, found_obj, buf);
		send_to_char(buf, ch);
	}
	else
		send_to_char("Похоже, этого здесь нет!\r\n", ch);

	return false;
}


void skip_hide_on_look(CHAR_DATA * ch)
{

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE) &&
			((!ch->get_skill(SKILL_LOOK_HIDE) ||
			  ((number(1, 100) -
				calculate_skill(ch, SKILL_LOOK_HIDE, 0) - 2 * (ch->get_wis() - 9)) > 0))))
	{
		affect_from_char(ch, SPELL_HIDE);
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE))
		{
			send_to_char("Вы прекратили прятаться.\r\n", ch);
			act("$n прекратил$g прятаться.", FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	return;
}

void do_look(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	char arg2[MAX_INPUT_LENGTH];
	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
	{
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы ослеплены!\r\n", ch);
	}
	else if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch))
	{
		if (GET_LEVEL(ch) > 30)
		{
			sprintf(buf,
				"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
				"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
				CCNRM(ch, C_NRM), CCINRM(ch, C_NRM), ch->in_room,
				CCRED(ch, C_NRM), CCIRED(ch, C_NRM), world[ch->in_room]->light,
				CCGRN(ch, C_NRM), CCIGRN(ch, C_NRM), world[ch->in_room]->glight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[ch->in_room]->fires,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[ch->in_room]->ices,
				CCBLU(ch, C_NRM), CCIBLU(ch, C_NRM), world[ch->in_room]->gdark,
				CCMAG(ch, C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
				CCWHT(ch, C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		skip_hide_on_look(ch);

		send_to_char("Слишком темно...\r\n", ch);
		list_char_to_char(world[ch->in_room]->people, ch);	// glowing red eyes
		show_glow_objs(ch);
	}
	else
	{
		half_chop(argument, arg, arg2);

		skip_hide_on_look(ch);

		if (subcmd == SCMD_READ)
		{
			if (!*arg)
				send_to_char("Что вы хотите прочитать?\r\n", ch);
			else
				look_at_target(ch, arg, subcmd);
			return;
		}
		if (!*arg)	// "look" alone, without an argument at all
		{
			if (ch->desc)
			{
				ch->desc->msdp_report("ROOM");
			}
			look_at_room(ch, 1);
		}
		else if (is_abbrev(arg, "in") || is_abbrev(arg, "внутрь"))
			look_in_obj(ch, arg2);
		// did the char type 'look <direction>?'
		else if (((look_type = search_block(arg, dirs, FALSE)) >= 0) ||
				 ((look_type = search_block(arg, Dirs, FALSE)) >= 0))
			look_in_direction(ch, look_type, EXIT_SHOW_WALL);
		else if (is_abbrev(arg, "at") || is_abbrev(arg, "на"))
			look_at_target(ch, arg2, subcmd);
		else
			look_at_target(ch, argument, subcmd);
	}
}

void do_sides(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (!ch->desc)
		return;

	if (GET_POS(ch) <= POS_SLEEPING)
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		send_to_char("Вы ослеплены!\r\n", ch);
	else
	{
		skip_hide_on_look(ch);
		send_to_char("Вы посмотрели по сторонам.\r\n", ch);
		for (i = 0; i < NUM_OF_DIRS; i++)
		{
			look_in_direction(ch, i, 0);
		}
	}
}

void do_looking(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("Белый Ангел возник перед вами, маняще помахивая крыльями.\r\n", ch);
	if (GET_POS(ch) == POS_SLEEPING)
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		send_to_char("Вы ослеплены!\r\n", ch);
	else if (ch->get_skill(SKILL_LOOKING))
	{
		if (check_moves(ch, LOOKING_MOVES))
		{
			send_to_char("Вы напрягли зрение и начали присматриваться по сторонам.\r\n", ch);
			for (i = 0; i < NUM_OF_DIRS; i++)
				look_in_direction(ch, i, EXIT_SHOW_LOOKING);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
	}
	else
		send_to_char("Вам явно не хватает этого умения.\r\n", ch);
}

void do_hearing(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (!ch->desc)
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
	{
		send_to_char("Вы глухи и все равно ничего не услышите.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("Вам начали слышаться голоса предков, зовущие вас к себе.\r\n", ch);
	if (GET_POS(ch) == POS_SLEEPING)
		send_to_char("Морфей медленно задумчиво провел рукой по струнам и заиграл колыбельную.\r\n", ch);
	else if (ch->get_skill(SKILL_HEARING))
	{
		if (check_moves(ch, HEARING_MOVES))
		{
			send_to_char("Вы начали сосредоточенно прислушиваться.\r\n", ch);
			for (i = 0; i < NUM_OF_DIRS; i++)
				hear_in_direction(ch, i, 0);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
	}
	else
		send_to_char("Выучите сначала как это следует делать.\r\n", ch);
}

void do_examine(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *tmp_char;
	OBJ_DATA *tmp_object;
	char where[MAX_INPUT_LENGTH];
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_OBJ_EXDESC;


	if (GET_POS(ch) < POS_SLEEPING)
	{
		send_to_char("Виделся часто сон беспокойный...\r\n", ch);
		return;
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы ослеплены!\r\n", ch);
		return;
	}

	two_arguments(argument, arg, where);

	if (!*arg)
	{
		send_to_char("Что вы желаете осмотреть?\r\n", ch);
		return;
	}

	if (isname(where, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
	else if (isname(where, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	skip_hide_on_look(ch);

	if (look_at_target(ch, argument, subcmd))
		return;

	if (isname(arg, "пентаграмма") && world[ch->in_room]->portal_time && IS_SET(where_bits, FIND_OBJ_ROOM))
		return;

	if (isname(arg, "камень") &&
			ch->get_skill(SKILL_TOWNPORTAL) &&
			(get_portal(GET_ROOM_VNUM(ch->in_room), NULL)) != NULL && IS_SET(where_bits, FIND_OBJ_ROOM))
		return;

	generic_find(arg, where_bits, ch, &tmp_char, &tmp_object);
	if (tmp_object)
	{
		if (GET_OBJ_TYPE(tmp_object) == OBJ_DATA::ITEM_DRINKCON
			|| GET_OBJ_TYPE(tmp_object) == OBJ_DATA::ITEM_FOUNTAIN
			|| GET_OBJ_TYPE(tmp_object) == OBJ_DATA::ITEM_CONTAINER)
		{
			look_in_obj(ch, argument);
		}
	}
}

void do_gold(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int count = 0;
	if (ch->get_gold() == 0)
		send_to_char("Вы разорены!\r\n", ch);
	else if (ch->get_gold() == 1)
		send_to_char("У вас есть всего лишь одна куна.\r\n", ch);
	else
	{
		count += sprintf(buf, "У Вас есть %ld %s.\r\n", ch->get_gold(), desc_count(ch->get_gold(), WHAT_MONEYa));
		send_to_char(buf, ch);
	}
}

/// см pc_class_name
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


const char *ac_text[] =
{
	"&WВы защищены как БОГ",	//  -30
	"&WВы защищены как БОГ",	//  -29
	"&WВы защищены как БОГ",	//  -28
	"&gВы защищены почти как БОГ",	//  -27
	"&gВы защищены почти как БОГ",	//  -26
	"&gВы защищены почти как БОГ",	//  -25
	"&gНаилучшая защита",	//  -24
	"&gНаилучшая защита",	//  -23
	"&gНаилучшая защита",	//  -22
	"&gВеликолепная защита",	//  -21
	"&gВеликолепная защита",	//  -20
	"&gВеликолепная защита",	//  -19
	"&gОтличная защита",	//  -18
	"&gОтличная защита",	//  -17
	"&gОтличная защита",	//  -16
	"&GОчень хорошая защита",	//  -15
	"&GОчень хорошая защита",	//  -14
	"&GОчень хорошая защита",	//  -13
	"&GВесьма хорошая защита",	//  -12
	"&GВесьма хорошая защита",	//  -11
	"&GВесьма хорошая защита",	//  -10
	"&GХорошая защита",	//   -9
	"&GХорошая защита",	//   -8
	"&GХорошая защита",	//   -7
	"&GНеплохая защита",	//   -6
	"&GНеплохая защита",	//   -5
	"&GНеплохая защита",	//   -4
	"&YЗащита чуть выше среднего",	//   -3
	"&YЗащита чуть выше среднего",	//   -2
	"&YЗащита чуть выше среднего",	//   -1
	"&YСредняя защита",	//    0
	"&YЗащита чуть ниже среднего",
	"&YСлабая защита",
	"&RСлабая защита",
	"&RОчень слабая защита",
	"&RВы немного защищены",	// 5
	"&RВы совсем немного защищены",
	"&rВы чуть-чуть защищены",
	"&rВы легко уязвимы",
	"&rВы почти полностью уязвимы",
	"&rВы полностью уязвимы",	// 10
};

void print_do_score_all(CHAR_DATA *ch)
{
	int ac, max_dam = 0, hr = 0, modi = 0;
	ESkill skill = SKILL_BOTHHANDS;

	std::string sum = string("Вы ") + string(ch->get_name()) + string(", ")
		+ string(class_name[static_cast<int>(GET_CLASS(ch)) + 14 * GET_KIN(ch)]) + string(".");

	sprintf(buf,
			" %s-------------------------------------------------------------------------------------\r\n"
			" || %s%-80s%s||\r\n"
			" -------------------------------------------------------------------------------------\r\n",
			CCCYN(ch, C_NRM),
			CCNRM(ch, C_NRM), sum.substr(0, 80).c_str(), CCCYN(ch, C_NRM));

	sprintf(buf + strlen(buf),
			" || %sПлемя: %-11s %s|"
			" %sРост:        %-3d(%-3d) %s|"
			" %sБроня:       %4d %s|"
			" %sСопротивление: %s||\r\n",
			CCNRM(ch, C_NRM),
			string(PlayerRace::GetKinNameByNum(GET_KIN(ch),GET_SEX(ch))).substr(0, 14).c_str(),
			CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), GET_ARMOUR(ch), CCCYN(ch, C_NRM),
			CCIYEL(ch, C_NRM), CCCYN(ch, C_NRM));

	ac = compute_armor_class(ch) / 10;
	if (ac < 5) {
		const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
		ac = ac + mod > 5 ? 5 : ac + mod;
	}

	int resist = MIN(GET_RESIST(ch, FIRE_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
			" || %sРод: %-13s %s|"
			" %sВес:         %3d(%3d) %s|"
			" %sЗащита:       %3d %s|"
			" %sОгню:      %3d %s||\r\n",
			CCNRM(ch, C_NRM),
            string(PlayerRace::GetRaceNameByNum(GET_KIN(ch),GET_RACE(ch),GET_SEX(ch))).substr(0, 14).c_str(),
			CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), ac, CCCYN(ch, C_NRM),
			CCIRED(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, AIR_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
			" || %sВера: %-13s%s|"
			" %sРазмер:      %3d(%3d) %s|"
			" %sПоглощение:   %3d %s|"
			" %sВоздуху:   %3d %s||\r\n",
			CCNRM(ch, C_NRM),
			string(religion_name[GET_RELIGION(ch)][static_cast<int>(GET_SEX(ch))]).substr(0, 13).c_str(),
			CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), GET_SIZE(ch), GET_REAL_SIZE(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), GET_ABSORBE(ch), CCCYN(ch, C_NRM),
			CCWHT(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	if (can_use_feat(ch, SHOT_FINESSE_FEAT)) //ловкий выстрел дамы от ловки
		max_dam = GET_REAL_DR(ch) + str_bonus(GET_REAL_DEX(ch), STR_TO_DAM);
	else
		max_dam = GET_REAL_DR(ch) + str_bonus(GET_REAL_STR(ch), STR_TO_DAM);

	if (can_use_feat(ch, BULLY_FEAT))
	{
		modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? MIN(GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)), 18) : 0));
		//modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)) : 0));
		modi = MAX(100, modi);
		max_dam += modi * max_dam / 50;
		max_dam += MAX(0, GET_REAL_STR(ch) - 25);
	}
	else
	{
	    max_dam += 6 + 2 * GET_LEVEL(ch) / 3;
	}

	OBJ_DATA* weapon = GET_EQ(ch, WEAR_BOTHS);
	if (weapon)
	{
		if (GET_OBJ_TYPE(weapon) == OBJ_DATA::ITEM_WEAPON)
		{
			max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1);
			skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
			if (ch->get_skill(skill) == SKILL_INVALID)
			{
				hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
				max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			}
			else
			{
			    apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
			}
		}
	}
	else
	{
		weapon = GET_EQ(ch, WEAR_WIELD);
		if (weapon)
		{
			if (GET_OBJ_TYPE(weapon) == OBJ_DATA::ITEM_WEAPON)
			{
				max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1) / 2;
				skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
				if (ch->get_skill(skill) == SKILL_INVALID)
				{
					hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
					max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
				}
				else
				{
				    apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
				}
			}
		}

		weapon = GET_EQ(ch, WEAR_HOLD);
		if (weapon)
		{
			if (GET_OBJ_TYPE(weapon) == OBJ_DATA::ITEM_WEAPON)
			{
				max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1) / 2;
				skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
				if (ch->get_skill(skill) == SKILL_INVALID)
				{
					hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
					max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
				}
				else
				{
				    apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
				}
			}
		}
	}

	if (can_use_feat(ch, WEAPON_FINESSE_FEAT))
	{
		if (weapon && GET_OBJ_WEIGHT(weapon) > 20)
		{
			hr += str_bonus(GET_REAL_STR(ch), STR_TO_HIT);
		}
		else
		{
			hr += str_bonus(GET_REAL_DEX(ch), STR_TO_HIT);
		}
	}
	else
	{
		hr += str_bonus(GET_REAL_STR(ch), STR_TO_HIT);
	}
	hr += GET_REAL_HR(ch) - thaco(static_cast<int>(GET_CLASS(ch)), static_cast<int>(GET_LEVEL(ch)));
	if (PRF_FLAGGED(ch, PRF_POWERATTACK)) {
		hr -= 2;
		max_dam += 5;
	}
	if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK)) {
		hr -= 4;
		max_dam += 10;
	}
	if (PRF_FLAGGED(ch, PRF_AIMINGATTACK)) {
		hr += 2;
		max_dam -= 5;
	}
	if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK)) {
		hr += 4;
		max_dam -= 10;
	}

	max_dam += ch->obj_bonus().calc_phys_dmg(max_dam);
	max_dam = MAX(0, max_dam);
	max_dam *= ch->get_cond_penalty(P_DAMROLL);

	hr *= ch->get_cond_penalty(P_HITROLL);

	resist = MIN(GET_RESIST(ch, WATER_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
			" || %sУровень: %s%-2d        %s|"
			" %sСила:          %2d(%2d) %s|"
			" %sАтака:        %3d %s|"
			" %sВоде:      %3d %s||\r\n",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_LEVEL(ch), CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), ch->get_str(), GET_REAL_STR(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), hr - (on_horse(ch) ? (10 - GET_SKILL(ch, SKILL_HORSE) / 20) : 0) , CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, EARTH_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
			" || %sПеревоплощений: %s%-2d %s|"
			" %sЛовкость:      %2d(%2d) %s|"
			" %sУрон:        %4d %s|"
			" %sЗемле:     %3d %s||\r\n",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_REMORT(ch), CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), ch->get_dex(), GET_REAL_DEX(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), int (max_dam * (on_horse(ch) ? ((GET_SKILL(ch, SKILL_HORSE) > 100) ? (1 + (GET_SKILL(ch, SKILL_HORSE) - 100) / 500.0) : 1 ) : 1)), CCCYN(ch, C_NRM),
			CCYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = GET_RESIST(ch, DARK_RESISTANCE);
	sprintf(buf + strlen(buf),
			" || %sВозраст: %s%-3d       %s|"
			" %sТелосложение:  %2d(%2d) %s|-------------------| &KТьме:      %3d&c ||\r\n",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_AGE(ch), CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), ch->get_con(), GET_REAL_CON(ch), CCCYN(ch, C_NRM),
			resist);
	resist = MIN(GET_RESIST(ch, VITALITY_RESISTANCE), 75);
	const int rcast = GET_CAST_SUCCESS(ch) * ch->get_cond_penalty(P_CAST);
	sprintf(buf + strlen(buf),
			" || %sОпыт: %s%-10ld   %s|"
			" %sМудрость:      %2d(%2d) %s|"
			" %sКолдовство:   %3d %s|"
			"&c----------------||\r\n",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_EXP(ch), CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), ch->get_wis(), GET_REAL_WIS(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), rcast, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, VITALITY_RESISTANCE), 75);

	if (IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), " || %sДСУ: %s1%s             |",
				CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf + strlen(buf),
				" || %sДСУ: %s%-10ld    %s|",
				CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch), CCCYN(ch, C_NRM));
	int itmp =  GET_MANAREG(ch);
	itmp *= ch->get_cond_penalty(P_CAST);
	sprintf(buf + strlen(buf),
			" %sУм:            %2d(%2d) %s|"
			" %sЗапоминание: %4d %s|"
			" %sЖивучесть: %3d %s||\r\n",

			CCICYN(ch, C_NRM), ch->get_int(), GET_REAL_INT(ch), CCCYN(ch, C_NRM),
			CCIGRN(ch, C_NRM), itmp , CCCYN(ch, C_NRM),
			CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));
	resist = MIN(GET_RESIST(ch, MIND_RESISTANCE), 75);

	sprintf(buf + strlen(buf),
			" || %sДенег: %s%-8ld    %s|"
			" %sОбаяние:       %2d(%2d) %s|-------------------|"
			" %sРазум:     %3d %s||\r\n",

			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), ch->get_gold(), CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), ch->get_cha(), GET_REAL_CHA(ch), CCCYN(ch, C_NRM),
			CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));
	resist = MIN(GET_RESIST(ch, IMMUNITY_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
			" || %sНа счету: %s%-8ld %s|"
			" %sЖизнь:     %4d(%4d) %s|"
			" %sВоля:         %3d%s |"
			" %sИммунитет: %3d %s||\r\n",

			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), ch->get_bank(), CCCYN(ch, C_NRM),
			CCICYN(ch, C_NRM), GET_HIT(ch), GET_REAL_MAX_HIT(ch), CCCYN(ch, C_NRM),
			CCGRN(ch, C_NRM), GET_REAL_SAVING_WILL(ch), CCCYN(ch, C_NRM),
			CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	if (!on_horse(ch))
		switch (GET_POS(ch))
		{
		case POS_DEAD:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCIRED(ch, C_NRM), string("Вы МЕРТВЫ!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_MORTALLYW:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCIRED(ch, C_NRM), string("Вы умираете!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_INCAP:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCRED(ch, C_NRM), string("Вы без сознания.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_STUNNED:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCIYEL(ch, C_NRM), string("Вы в обмороке!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_SLEEPING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCIGRN(ch, C_NRM), string("Вы спите.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_RESTING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCGRN(ch, C_NRM), string("Вы отдыхаете.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_SITTING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCIGRN(ch, C_NRM), string("Вы сидите.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_FIGHTING:
			if (ch->get_fighting())
				sprintf(buf + strlen(buf), " || %s%-19s%s|",
						CCIRED(ch, C_NRM), string("Вы сражаетесь!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			else
				sprintf(buf + strlen(buf), " || %s%-19s%s|",
						CCRED(ch, C_NRM), string("Вы машете кулаками.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_STANDING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCNRM(ch, C_NRM), string("Вы стоите.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		default:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCNRM(ch, C_NRM), string("You are floating..").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		}
	else
		sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCNRM(ch, C_NRM), string("Вы сидите верхом.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));

	sprintf(buf + strlen(buf),
			" %sВыносл.:     %3d(%3d) %s|"
			" %sЗдоровье:     %3d %s|"
			"----------------||\r\n",

			CCICYN(ch, C_NRM), GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), CCCYN(ch, C_NRM),
			CCGRN(ch, C_NRM), GET_REAL_SAVING_CRITICAL(ch), CCCYN(ch, C_NRM));

	if (GET_COND(ch, FULL) > NORM_COND_VALUE)
		sprintf(buf + strlen(buf), " || %sГолоден: %sугу :(%s    |", CCNRM(ch, C_NRM), CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf + strlen(buf), " || %sГолоден: %sнет%s       |", CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (IS_MANA_CASTER(ch))
		sprintf(buf + strlen(buf),
				" %sМаг. сила: %4d(%4d) %s|",
				CCICYN(ch, C_NRM), GET_MANA_STORED(ch), GET_MAX_MANA(ch), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                       |");

	sprintf(buf + strlen(buf),
			" %sСтойкость:    %3d %s|"
			" &rВосст. жизни:  &c||\r\n",
			CCGRN(ch, C_NRM), GET_REAL_SAVING_STABILITY(ch), CCCYN(ch, C_NRM));

	if (GET_COND_M(ch, THIRST))
		sprintf(buf + strlen(buf),
				" || %sЖажда: %sналивай!%s    |",
				CCNRM(ch, C_NRM), CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf + strlen(buf),
				" || %sЖажда: %sнет%s         |",
				CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (IS_MANA_CASTER(ch))
		sprintf(buf + strlen(buf),
				" %sВосстан.:    %3d сек. %s|",
				CCICYN(ch, C_NRM), mana_gain(ch), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                       |");

	sprintf(buf + strlen(buf),
			" %sРеакция:      %3d %s|"
			" %s  %+4d%% (%+4d) %s||\r\n",
			CCGRN(ch, C_NRM), GET_REAL_SAVING_REFLEX(ch), CCCYN(ch, C_NRM),
			CCRED(ch, C_NRM), GET_HITREG(ch), hit_gain(ch), CCCYN(ch, C_NRM));

	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
	{
		if (affected_by_spell(ch, SPELL_ABSTINENT))
			sprintf(buf + strlen(buf),
					" || %sПохмелье.          %s|                       |",
					CCIYEL(ch, C_NRM), CCCYN(ch, C_NRM));
		else
			sprintf(buf + strlen(buf),
					" || %sВы пьяны.          %s|                       |",
					CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));
	}
	else
	{
		strcat(buf, " ||                    |                       |");
	}
	sprintf(buf + strlen(buf),
			" %sУдача:       %4d %s|"
			" &rВосст. сил:    &c||\r\n",
			CCGRN(ch, C_NRM), ch->calc_morale(), CCCYN(ch, C_NRM));

	const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
	if (wdex == 0)
	{
		strcat(buf, " ||                    |                       |");
	}
	else
	{
		sprintf(buf + strlen(buf),
			" || %sПерегруз!%s          |                       |",
			wdex == 1 ? CCIYEL(ch, C_NRM) : CCIRED(ch, C_NRM),
			CCCYN(ch, C_NRM));
	}
	sprintf(buf + strlen(buf),
		" %sИнициатива:  %4d %s|"
		" %s  %+4d%% (%+4d) %s||\r\n"
		" -------------------------------------------------------------------------------------\r\n",
		CCGRN(ch, C_NRM), calc_initiative(ch, false), CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), GET_MOVEREG(ch), move_gain(ch), CCCYN(ch, C_NRM));

	if (has_horse(ch, FALSE))
	{
		if (on_horse(ch))
			sprintf(buf + strlen(buf),
					" %s|| %sВы верхом на %-67s%s||\r\n"
					" -------------------------------------------------------------------------------------\r\n",
					CCCYN(ch, C_NRM), CCIGRN(ch, C_NRM),
					(string(GET_PAD(get_horse(ch), 5)) + string(".")).substr(0, 67).c_str(), CCCYN(ch, C_NRM));
		else
			sprintf(buf + strlen(buf),
					" %s|| %sУ вас есть %-69s%s||\r\n"
					" -------------------------------------------------------------------------------------\r\n",
					CCCYN(ch, C_NRM), CCIGRN(ch, C_NRM),
					(string(GET_NAME(get_horse(ch))) + string(".")).substr(0, 69).c_str(), CCCYN(ch, C_NRM));
	}

	//Напоминаем о метке, если она есть.
    ROOM_DATA *label_room = RoomSpells::find_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
	if (label_room)
	{
		const int timer_room_label = RoomSpells::timer_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
		sprintf(buf + strlen(buf),
				" %s|| &G&qВы поставили рунную метку в комнате %s%s||\r\n",
				CCCYN(ch, C_NRM),
				colored_name(string(string("'")+label_room->name+string("&n&Q'.")).c_str(), 44),
				CCCYN(ch, C_NRM));
		if (timer_room_label > 0)
		{
			*buf2 = '\0';
			(timer_room_label + 1) / SECS_PER_MUD_HOUR ? sprintf(buf2, "%d %s.", (timer_room_label + 1) / SECS_PER_MUD_HOUR + 1, desc_count((timer_room_label + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR)) : sprintf(buf2, "менее часа.");			
			sprintf(buf + strlen(buf),
					" || Метка продержится еще %-58s||\r\n",buf2);
			*buf2 = '\0';
		}
	}

	int glory = Glory::get_glory(GET_UNIQUE(ch));
/*	if (glory)
		sprintf(buf + strlen(buf),
				" %s|| %sВы заслужили %5d %-61s%s||\r\n",
				CCCYN(ch, C_NRM), CCWHT(ch, C_NRM), glory,
				(string(desc_count(glory, WHAT_POINT)) + string(" славы для временного улучшения характеристик.")).substr(0, 61).c_str(),
				CCCYN(ch, C_NRM));
*/
	glory = GloryConst::get_glory(GET_UNIQUE(ch));
	if (glory)
		sprintf(buf + strlen(buf),
				" %s|| %sВы заслужили %5d %-61s%s||\r\n",
				CCCYN(ch, C_NRM), CCWHT(ch, C_NRM), glory,
				(string(desc_count(glory, WHAT_POINT)) + string(" постоянной славы.")).substr(0, 61).c_str(),
				CCCYN(ch, C_NRM));

	if (GET_GOD_FLAG(ch, GF_REMORT) && CLAN(ch))
	{
		sprintf(buf + strlen(buf),
			" || Вы самоотверженно отдаете весь получаемый опыт своей дружине.                   ||\r\n");
	}

	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		sprintf(buf + strlen(buf),
				" || Вы можете быть призваны.                                                        ||\r\n");
	else
		sprintf(buf + strlen(buf),
				" || Вы защищены от призыва.                                                         ||\r\n");
	if (PRF_FLAGGED(ch, PRF_BLIND))
		sprintf(buf + strlen(buf),
				" || Режим слепого игрока включен.                                                   ||\r\n");
	if (Bonus::is_bonus(0))
		sprintf(buf + strlen(buf),
			" || %-79s ||\r\n || %-79s ||\r\n", Bonus::str_type_bonus().c_str(), Bonus::bonus_end().c_str());
	if (!NAME_GOD(ch) && GET_LEVEL(ch) <= NAME_LEVEL)
	{
		sprintf(buf + strlen(buf),
				" &c|| &RВНИМАНИЕ!&n ваше имя не одобрил никто из богов!&c                                   ||\r\n");
		sprintf(buf + strlen(buf),
				" || &nCкоро вы прекратите получать опыт, обратитесь к богам для одобрения имени.      &c||\r\n");
	}
	else if (NAME_BAD(ch))
	{
		sprintf(buf + strlen(buf),
				" || &RВНИМАНИЕ!&n ваше имя запрещено богами. Очень скоро вы прекратите получать опыт.   &c||\r\n");
	}
	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
				" || %sВы можете вступить в группу с максимальной разницей                             %s||\r\n"
				" || %sв %2d %-75s%s||\r\n",
				CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
				grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REMORT(ch))],
				(string(desc_count(grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REMORT(ch))], WHAT_LEVEL))
				 + string(" без потерь для опыта.")).substr(0, 76).c_str(), CCCYN(ch, C_NRM));

	if (RENTABLE(ch))
	{
		const time_t rent_time = RENTABLE(ch) - time(0);
		const int minutes = rent_time > 60 ? rent_time / 60 : 0;
		sprintf(buf + strlen(buf),
				" || %sВ связи с боевыми действиями вы не можете уйти на постой еще %-18s%s ||\r\n",
				CCIRED(ch, C_NRM),
				minutes ? (boost::lexical_cast<std::string>(minutes) + string(" ") + string(desc_count(minutes, WHAT_MINu)) + string(".")).substr(0, 18).c_str()
						: (boost::lexical_cast<std::string>(rent_time) + string(" ") + string(desc_count(rent_time, WHAT_SEC)) + string(".")).substr(0, 18).c_str(),
				CCCYN(ch, C_NRM));
	}
	else if ((ch->in_room != NOWHERE) && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER))
		sprintf(buf + strlen(buf),
				" || %sТут вы чувствуете себя в безопасности.                                          %s||\r\n",
				CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMITH) && (ch->get_skill(SKILL_INSERTGEM) || ch->get_skill(SKILL_REPAIR) || ch->get_skill(SKILL_TRANSFORMWEAPON)))
		sprintf(buf + strlen(buf),
				" || %sЭто место отлично подходит для занятий кузнечным делом.                         %s||\r\n",
				CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (mail::has_mail(ch->get_uid()))
		sprintf(buf + strlen(buf),
				" || %sВас ожидает новое письмо, зайдите на почту.                                     %s||\r\n",
				CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (Parcel::has_parcel(ch))
		sprintf(buf + strlen(buf),
				" || %sВас ожидает посылка, зайдите на почту.                                          %s||\r\n",
				CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (ch->get_protecting())
		sprintf(buf + strlen(buf),
				" || %sВы прикрываете %-65s%s||\r\n",
				CCIGRN(ch, C_NRM), string(GET_PAD(ch->get_protecting(),3)+string(" от нападения.")).substr(0,65).c_str(),
				CCCYN(ch, C_NRM));

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch))
	{
		const int hrs = (GCURSE_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((GCURSE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
				" || %sВы прокляты Богами на %3d %-5s %2d %-45s%s||\r\n",
				CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
				mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 45).c_str(), CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(NULL))
	{
		const int hrs = (HELL_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
				" || %sВам предстоит провести в темнице еще %6d %-5s %2d %-27s%s||\r\n"
				" || %s[%-79s%s||\r\n",
				CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
				mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 27).c_str(),
				CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
				(string(HELL_REASON(ch) ? HELL_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
				CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(NULL))
	{
		const int hrs = (MUTE_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
				" || %sВы не сможете кричать еще %6d %-5s %2d %-38s%s||\r\n"
				" || %s[%-79s%s||\r\n",
				CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
				mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 38).c_str(),
				CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
				(string(MUTE_REASON(ch) ? MUTE_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
				CCCYN(ch, C_NRM));
	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(NULL))
	{
		const int hrs = (UNREG_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((UNREG_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
				" || %sВы не сможете входить с одного IP еще %6d %-5s %2d %-26s%s||\r\n"
				" || %s[%-79s%s||\r\n",
				CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
				mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 38).c_str(),
				CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
				(string(UNREG_REASON(ch) ? UNREG_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
				CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(NULL))
	{
		const int hrs = (DUMB_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
				" || %sВы будете молчать еще %6d %-5s %2d %-42s%s||\r\n"
				" || %s[%-79s%s||\r\n",
				CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
				mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 42).c_str(),
				CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
				(string(DUMB_REASON(ch) ? DUMB_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
				CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_FROZEN) && FREEZE_DURATION(ch) != 0 && FREEZE_DURATION(ch) > time(NULL))
	{
		const int hrs = (FREEZE_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((FREEZE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
				" || %sВы будете заморожены еще %6d %-5s %2d %-39s%s||\r\n"
				" || %s[%-79s%s||\r\n",
				CCRED(ch, C_NRM), hrs, string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
				mins, (string(desc_count(mins, WHAT_MINu)) + string(".")).substr(0, 42).c_str(),
				CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
				(string(FREEZE_REASON(ch) ? FREEZE_REASON(ch) : "-") + string("].")).substr(0, 79).c_str(),
				CCCYN(ch, C_NRM));
	}

	if (ch->is_morphed())
	{
		sprintf(buf + strlen(buf),
			" || %sВы находитесь в звериной форме - %-47s%s||\r\n",
			CCYEL(ch, C_NRM),
			ch->get_morph_desc().substr(0, 47).c_str(),
			CCCYN(ch, C_NRM));
	}
	strcat(buf, " ||                                                                                 ||\r\n");
	strcat(buf, " -------------------------------------------------------------------------------------\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
//	test_self_hitroll(ch);
}

void do_score(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	TIME_INFO_DATA playing_time;
	int ac, ac_t;

	skip_spaces(&argument);

	if (IS_NPC(ch))
		return;

	//Обработка команды "счет все", добавил Adept. Ширина таблицы - 85 символов + пробел.
	if (is_abbrev(argument, "все") || is_abbrev(argument, "all"))
	{
		print_do_score_all(ch);
		return;
	}

	sprintf(buf, "Вы %s (%s, %s, %s, %s %d уровня).\r\n",
		ch->only_title().c_str(),
		string(PlayerRace::GetKinNameByNum(GET_KIN(ch), GET_SEX(ch))).c_str(),
		string(PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), GET_SEX(ch))).c_str(),
		religion_name[GET_RELIGION(ch)][static_cast<int>(GET_SEX(ch))],
		class_name[static_cast<int>(GET_CLASS(ch)) + 14 * GET_KIN(ch)], GET_LEVEL(ch));

	if (!NAME_GOD(ch) && GET_LEVEL(ch) <= NAME_LEVEL)
	{
		sprintf(buf + strlen(buf), "\r\n&RВНИМАНИЕ!&n Ваше имя не одобрил никто из богов!\r\n");
		sprintf(buf + strlen(buf), "Очень скоро вы прекратите получать опыт,\r\n");
		sprintf(buf + strlen(buf), "обратитесь к богам для одобрения имени.\r\n\r\n");
	}
	else if (NAME_BAD(ch))
	{
		sprintf(buf + strlen(buf), "\r\n&RВНИМАНИЕ!&n Ваше имя запрещено богами.\r\n");
		sprintf(buf + strlen(buf), "Очень скоро вы прекратите получать опыт.\r\n\r\n");
	}

	sprintf(buf + strlen(buf), "Сейчас вам %d %s. ", GET_REAL_AGE(ch), desc_count(GET_REAL_AGE(ch), WHAT_YEAR));

	if (age(ch)->month == 0 && age(ch)->day == 0)
	{
		sprintf(buf2, "%sУ вас сегодня День Варенья!%s\r\n", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	}
	else
		strcat(buf, "\r\n");

	sprintf(buf + strlen(buf),
			"Вы можете выдержать %d(%d) %s повреждения, и пройти %d(%d) %s по ровной местности.\r\n",
			GET_HIT(ch), GET_REAL_MAX_HIT(ch), desc_count(GET_HIT(ch),
					WHAT_ONEu),
			GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), desc_count(GET_MOVE(ch), WHAT_MOVEu));

	if (IS_MANA_CASTER(ch))
	{
		sprintf(buf + strlen(buf),
				"Ваша магическая энергия %d(%d) и вы восстанавливаете %d в сек.\r\n",
				GET_MANA_STORED(ch), GET_MAX_MANA(ch), mana_gain(ch));
	}

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
			CCICYN(ch, C_NRM), ch->get_str(), GET_REAL_STR(ch),
			ch->get_dex(), GET_REAL_DEX(ch),
			ch->get_con(), GET_REAL_CON(ch),
			ch->get_wis(), GET_REAL_WIS(ch),
			ch->get_int(), GET_REAL_INT(ch),
			ch->get_cha(), GET_REAL_CHA(ch),
			GET_SIZE(ch), GET_REAL_SIZE(ch),
			GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCNRM(ch, C_NRM));

	if (IS_IMMORTAL(ch))
	{
		sprintf(buf + strlen(buf),
				"%sВаши боевые качества :\r\n"
				"  AC   : %4d(%4d)"
				"  DR   : %4d(%4d)%s\r\n",
				CCIGRN(ch, C_NRM), GET_AC(ch), compute_armor_class(ch),
				GET_DR(ch), GET_REAL_DR(ch), CCNRM(ch, C_NRM));
	}
	else
	{
		ac = compute_armor_class(ch) / 10;

		if (ac < 5)
		{
			const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
			ac = ac + mod > 5 ? 5 : ac + mod;
		}

		ac_t = MAX(MIN(ac + 30, 40), 0);
		sprintf(buf + strlen(buf), "&GВаши боевые качества :\r\n"
				"  Защита  (AC)     : %4d - %s&G\r\n"
				"  Броня/Поглощение : %4d/%d&n\r\n",
				ac, ac_text[ac_t], GET_ARMOUR(ch), GET_ABSORBE(ch));
	}
	sprintf(buf + strlen(buf), "Ваш опыт - %ld %s. ", GET_EXP(ch), desc_count(GET_EXP(ch), WHAT_POINT));
	if (GET_LEVEL(ch) < LVL_IMMORT)
	{
		if (PRF_FLAGGED(ch, PRF_BLIND))
		{
			sprintf(buf + strlen(buf), "\r\n");
		}
		sprintf(buf + strlen(buf),
			"Вам осталось набрать %ld %s до следующего уровня.\r\n",
			level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch),
			desc_count(level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch), WHAT_POINT));
	}
	else
		sprintf(buf + strlen(buf), "\r\n");

	sprintf(buf + strlen(buf), "У вас на руках %ld %s и %d %s",
		ch->get_gold(), desc_count(ch->get_gold(), WHAT_MONEYa), ch->get_hryvn(), desc_count(ch->get_hryvn(), WHAT_TORC));
	if (ch->get_bank() > 0)
		sprintf(buf + strlen(buf), " (и еще %ld %s припрятано в лежне).\r\n",
			ch->get_bank(), desc_count(ch->get_bank(), WHAT_MONEYa));
	else
		strcat(buf, ".\r\n");


	if (GET_LEVEL(ch) < LVL_IMMORT)
	{
		sprintf(buf + strlen(buf),
				"Вы можете вступить в группу с максимальной разницей в %d %s без потерь для опыта.\r\n",
				grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REMORT(ch))],
				desc_count(grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REMORT(ch))], WHAT_LEVEL));
	}

	//Напоминаем о метке, если она есть.
    ROOM_DATA *label_room = RoomSpells::find_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
    if (label_room)
	{
        sprintf(buf + strlen(buf),
                "&G&qВы поставили рунную метку в комнате '%s'.&Q&n\r\n",
                string(label_room->name).c_str());
	}

	int glory = Glory::get_glory(GET_UNIQUE(ch));
	if (glory)
	{
		sprintf(buf + strlen(buf), "Вы заслужили %d %s славы.\r\n",
				glory, desc_count(glory, WHAT_POINT));
	}
	glory = GloryConst::get_glory(GET_UNIQUE(ch));
	if (glory)
	{
		sprintf(buf + strlen(buf), "Вы заслужили %d %s постоянной славы.\r\n",
				glory, desc_count(glory, WHAT_POINT));
	}

	playing_time = *real_time_passed((time(0) - ch->player_data.time.logon) + ch->player_data.time.played, 0);
	sprintf(buf + strlen(buf), "Вы играете %d %s %d %s реального времени.\r\n",
			playing_time.day, desc_count(playing_time.day, WHAT_DAY),
			playing_time.hours, desc_count(playing_time.hours, WHAT_HOUR));

	if (!on_horse(ch))
		switch (GET_POS(ch))
		{
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
			if (ch->get_fighting())
				sprintf(buf + strlen(buf), "Вы сражаетесь с %s.\r\n", GET_PAD(ch->get_fighting(), 4));
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
	const auto value_drunked = GET_COND(ch, DRUNK);
	if (value_drunked >= CHAR_DRUNKED)
	{
		if (affected_by_spell(ch, SPELL_ABSTINENT))
			strcat(buf, "Привет с большого бодуна!\r\n");
		else
		{
			if (value_drunked >= CHAR_MORTALLY_DRUNKED)
				strcat(buf, "Вы так пьяны, что ваши ноги не хотят слушаться вас...\r\n");
			else if (value_drunked >= 10)
				strcat(buf, "Вы так пьяны, что вам хочется петь песни.\r\n");
			else if (value_drunked >= 5)
				strcat(buf, "Вы пьяны.\r\n");
			else
				strcat(buf, "Вы немного пьяны.\r\n");
		}
			
	}
	if (GET_COND_M(ch, FULL))
		strcat(buf, "Вы голодны.\r\n");
	if (GET_COND_M(ch, THIRST))
		strcat(buf, "Вас мучает жажда.\r\n");
	/*
	   strcat(buf, CCICYN(ch, C_NRM));
	   strcat(buf,"Аффекты :\r\n");
	   (ch)->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n");
	   strcat(buf,buf2);
	 */
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		strcat(buf, "Вы можете быть призваны.\r\n");

	if (has_horse(ch, FALSE))
	{
		if (on_horse(ch))
			sprintf(buf + strlen(buf), "Вы верхом на %s.\r\n", GET_PAD(get_horse(ch), 5));
		else
			sprintf(buf + strlen(buf), "У вас есть %s.\r\n", GET_NAME(get_horse(ch)));
	}
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	if (RENTABLE(ch))
	{
		sprintf(buf,
				"%sВ связи с боевыми действиями вы не можете уйти на постой.%s\r\n",
				CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
	else if ((ch->in_room != NOWHERE) && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER))
	{
		sprintf(buf, "%sТут вы чувствуете себя в безопасности.%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMITH) && (ch->get_skill(SKILL_INSERTGEM) || ch->get_skill(SKILL_REPAIR) || ch->get_skill(SKILL_TRANSFORMWEAPON)))
	{
		sprintf(buf, "%sЭто место отлично подходит для занятий кузнечным делом.%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (mail::has_mail(ch->get_uid()))
	{
		sprintf(buf, "%sВас ожидает новое письмо, зайдите на почту!%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (Parcel::has_parcel(ch))
	{
		sprintf(buf, "%sВас ожидает посылка, зайдите на почту!%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(NULL))
	{
		const int hrs = (HELL_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf,
				"Вам предстоит провести в темнице еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins,
						WHAT_MINu),
				HELL_REASON(ch) ? HELL_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(NULL))
	{
		const int hrs = (MUTE_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете кричать еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), MUTE_REASON(ch) ? MUTE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(NULL))
	{
		const int hrs = (DUMB_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете молчать еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), DUMB_REASON(ch) ? DUMB_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_FROZEN) && FREEZE_DURATION(ch) != 0 && FREEZE_DURATION(ch) > time(NULL))
	{
		const int hrs = (FREEZE_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((FREEZE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете заморожены еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), FREEZE_REASON(ch) ? FREEZE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(NULL))
	{
		const int hrs = (UNREG_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((UNREG_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете заходить с одного IP еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), UNREG_REASON(ch) ? UNREG_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch))
	{
		const int hrs = (GCURSE_DURATION(ch) - time(NULL)) / 3600;
		const int mins = ((GCURSE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "Вы прокляты Богами на %d %s %d %s.\r\n",
				hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins, WHAT_MINu));
		send_to_char(buf, ch);
	}

	if (ch->is_morphed())
	{
		sprintf(buf, "Вы находитесь в звериной форме - %s.\r\n", ch->get_morph_desc().c_str());
		send_to_char(buf, ch);
	}
	if (can_use_feat(ch, COLLECTORSOULS_FEAT))
	{
		const int souls = ch->get_souls();
		if (souls == 0)
		{
			sprintf(buf, "Вы не имеете чужих душ.\r\n");
			send_to_char(buf, ch);
		}
		else
		{
			if (souls == 1)
			{
				sprintf(buf, "Вы имеете всего одну душу в запасе.\r\n");
				send_to_char(buf, ch);
			}
			if (souls > 1 && souls < 5)
			{
				sprintf(buf, "Вы имеете %d души в запасе.\r\n", souls);
				send_to_char(buf, ch);
			}
			if (souls >= 5)
			{
				sprintf(buf, "Вы имеете %d чужих душ в запасе.\r\n", souls);
				send_to_char(buf, ch);
			}
		}
	}
	if (ch->get_ice_currency() > 0)
	{
		if (ch->get_ice_currency() == 1)
		{
			sprintf(buf, "У вас в наличии есть одна жалкая искристая снежинка.\r\n");
			send_to_char(buf, ch);
		}
		else if (ch->get_ice_currency() < 5)
		{
			sprintf(buf, "У вас в наличии есть жалкие %d искристые снежинки.\r\n", ch->get_ice_currency());
			send_to_char(buf, ch);
		}
		else
		{
			sprintf(buf, "У вас в наличии есть %d искристых снежинок.\r\n", ch->get_ice_currency());
			send_to_char(buf, ch);
		}
	}
}

//29.11.09 Отображение количества рипов (с) Василиса
// edited by WorM 2011.05.21
void do_mystat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	skip_spaces(&argument);
	if (is_abbrev(argument, "очистить") || is_abbrev(argument, "clear"))
	{
		GET_RIP_MOBTHIS(ch) = GET_EXP_MOBTHIS(ch) = GET_RIP_MOB(ch) = GET_EXP_MOB(ch) =
		GET_RIP_PKTHIS(ch) = GET_EXP_PKTHIS(ch) = GET_RIP_PK(ch) = GET_EXP_PK(ch) =
		GET_RIP_DTTHIS(ch) = GET_EXP_DTTHIS (ch) = GET_RIP_DT(ch) = GET_EXP_DT(ch) =
		GET_RIP_OTHERTHIS(ch) = GET_EXP_OTHERTHIS(ch) = GET_RIP_OTHER(ch) = GET_EXP_OTHER(ch) =
		GET_WIN_ARENA(ch) = GET_RIP_ARENA(ch) = GET_EXP_ARENA(ch) = 0;
		send_to_char("Статистика очищена.\r\n", ch);
	}
	else
	{
		sprintf(buf,    " &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n   Статистика ваших смертей   &C|&n         &WТекущее&n         &C|&n                         &C||&n\r\n"
				" &C||&n (количество, потеряно опыта) &C|&n      &Wперевоплощение&n     &C|&n           &KВсего&n         &C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n    В неравном бою с тварями: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C||&n    В неравном бою с врагами: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C||&n             В гиблых местах: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C||&n   По стечению обстоятельств: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n                       &yИТОГО:&n &C|&n &W%4d (%16llu)&n &C| &K%4d (%16llu)&n &n&C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n &WНа арене (всего):                                                                &n&C||&n\r\n"
				" &C||&n   &wУбито игроков:&n&r%4d&n     &wСмертей:&n&r%4d&n           &wПотеряно опыта:&n &r%16llu&n &C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				,
				GET_RIP_MOBTHIS(ch),GET_EXP_MOBTHIS(ch), GET_RIP_MOB(ch), GET_EXP_MOB(ch),
				GET_RIP_PKTHIS(ch),GET_EXP_PKTHIS(ch), GET_RIP_PK(ch), GET_EXP_PK(ch),
				GET_RIP_DTTHIS(ch),GET_EXP_DTTHIS (ch), GET_RIP_DT(ch), GET_EXP_DT(ch),
				GET_RIP_OTHERTHIS(ch),GET_EXP_OTHERTHIS(ch), GET_RIP_OTHER(ch), GET_EXP_OTHER(ch),
				GET_RIP_MOBTHIS(ch)+GET_RIP_PKTHIS(ch)+GET_RIP_DTTHIS(ch)+GET_RIP_OTHERTHIS(ch),
				GET_EXP_MOBTHIS(ch)+GET_EXP_PKTHIS(ch)+GET_EXP_DTTHIS(ch)+GET_EXP_OTHERTHIS(ch)+GET_EXP_ARENA(ch),
				GET_RIP_MOB(ch)+GET_RIP_PK(ch)+GET_RIP_DT(ch)+GET_RIP_OTHER(ch),
				GET_EXP_MOB(ch)+GET_EXP_PK(ch)+GET_EXP_DT(ch)+GET_EXP_OTHER(ch)+GET_EXP_ARENA(ch),
				GET_WIN_ARENA(ch),GET_RIP_ARENA(ch), GET_EXP_ARENA(ch));
		send_to_char(buf, ch);
	}
}
// end by WorM
// конец правки (с) Василиса

void do_inventory(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	send_to_char("Вы несете:\r\n", ch);
	list_obj_to_char(ch->carrying, ch, 1, 2);
}

void do_equipment(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int i, found = 0;
	skip_spaces(&argument);

	send_to_char("На вас надето:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
			{
				send_to_char(where[i], ch);
				show_obj_to_char(GET_EQ(ch, i), ch, 1, TRUE, 1);
				found = TRUE;
			}
			else
			{
				send_to_char(where[i], ch);
				send_to_char("что-то.\r\n", ch);
				found = TRUE;
			}
		}
		else		// added by Pereplut
		{
			if (is_abbrev(argument, "все") || is_abbrev(argument, "all"))
			{
			    if (GET_EQ(ch, 18))
				if ((i==16) || (i==17))
				    continue;
                            if ((i==19)&&(GET_EQ(ch, WEAR_BOTHS)))
                            {
                                if (!(((GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS))) == OBJ_DATA::ITEM_WEAPON) && (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS )))
                                    continue;
                            }
                            else if (i==19)
				continue;
 			    if (GET_EQ(ch, 16) || GET_EQ(ch, 17))
				if (i==18)
				    continue;
			    if (GET_EQ(ch, 11))
				{
					if ((i==17) || (i==18))
						continue;
				}
				send_to_char(where[i], ch);
				sprintf(buf, "%s[ Ничего ]%s\r\n", CCINRM(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				found = TRUE;
			}
		}
	}
	if (!found)
	{
		if (IS_FEMALE(ch))
			send_to_char("Костюм Евы вам очень идет :)\r\n", ch);
		else
			send_to_char(" Вы голы, аки сокол.\r\n", ch);
	}
}

void do_time(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int day, month, days_go;
	if (IS_NPC(ch))
		return;
	sprintf(buf, "Сейчас ");
	switch (time_info.hours % 24)
	{
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
	switch (weather_info.sunlight)
	{
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

	day = time_info.day + 1;	// day in [1..30]
	*buf = '\0';
	if (GET_RELIGION(ch) == RELIGION_POLY || IS_IMMORTAL(ch))
	{
		days_go = time_info.month * DAYS_PER_MONTH + time_info.day;
		month = days_go / 40;
		days_go = (days_go % 40) + 1;
		sprintf(buf + strlen(buf), "%s, %dй День, Год %d%s",
				month_name_poly[month], days_go, time_info.year, IS_IMMORTAL(ch) ? ".\r\n" : "");
	}
	if (GET_RELIGION(ch) == RELIGION_MONO || IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), "%s, %dй День, Год %d",
				month_name[static_cast<int>(time_info.month)], day, time_info.year);
	if (IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), "\r\n%d.%d.%d, дней с начала года: %d", day, time_info.month+1, time_info.year, (time_info.month *DAYS_PER_MONTH) + day);
	switch (weather_info.season)
	{
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

void do_weather(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
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

	if (OUTSIDE(ch))
	{
		*buf = '\0';
		if (world[ch->in_room]->weather.duration > 0)
		{
			sky = world[ch->in_room]->weather.sky;
			weather_type = world[ch->in_room]->weather.weather_type;
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
	}
	else
		send_to_char("Вы ничего не можете сказать о погоде сегодня.\r\n", ch);
	if (IS_GOD(ch))
	{
		sprintf(buf, "День: %d Месяц: %s Час: %d Такт = %d\r\n"
				"Температура =%-5d, за день = %-8d, за неделю = %-8d\r\n"
				"Давление    =%-5d, за день = %-8d, за неделю = %-8d\r\n"
				"Выпало дождя = %d(%d), снега = %d(%d). Лед = %d(%d). Погода = %08x(%08x).\r\n",
				time_info.day, month_name[time_info.month], time_info.hours,
				weather_info.hours_go, weather_info.temperature,
				weather_info.temp_last_day, weather_info.temp_last_week,
				weather_info.pressure, weather_info.press_last_day,
				weather_info.press_last_week, weather_info.rainlevel,
				world[ch->in_room]->weather.rainlevel, weather_info.snowlevel,
				world[ch->in_room]->weather.snowlevel, weather_info.icelevel,
				world[ch->in_room]->weather.icelevel,
				weather_info.weather_type, world[ch->in_room]->weather.weather_type);
		send_to_char(buf, ch);
	}
}

namespace
{

const char* IMM_WHO_FORMAT =
"Формат: кто [минуров[-максуров]] [-n имя] [-c профлист] [-s] [-r] [-z] [-h] [-b|-и]\r\n";

const char* MORT_WHO_FORMAT = "Формат: кто [имя] [-?]\r\n";

} // namespace

void do_who(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char name_search[MAX_INPUT_LENGTH];
	name_search[0] = '\0';

	// Флаги для опций
	int low = 0, high = LVL_IMPL;
	int showclass = 0, num_can_see = 0;
	int imms_num = 0, morts_num = 0, demigods_num = 0;
	bool localwho = false, short_list = false;
	bool who_room = false, showname = false;

	skip_spaces(&argument);
	strcpy(buf, argument);

	// Проверка аргументов команды "кто"
	while (*buf)
	{
		half_chop(buf, arg, buf1);
		if (!str_cmp(arg, "боги") && strlen(arg) == 4)
		{
			low = LVL_IMMORT;
			high = LVL_IMPL;
			strcpy(buf, buf1);
		}
		else if (a_isdigit(*arg))
		{
			if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
				sscanf(arg, "%d-%d", &low, &high);
			strcpy(buf, buf1);
		}
		else if (*arg == '-')
		{
			const char mode = *(arg + 1);	// just in case; we destroy arg in the switch
			switch (mode)
			{
			case 'b':
			case 'и':
				if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_DEMIGOD) || PRF_FLAGGED(ch, PRF_CODERINFO))
					showname = true;
				strcpy(buf, buf1);
				break;
			case 'z':
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					localwho = true;
				strcpy(buf, buf1);
				break;
			case 's':
				if (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					short_list = true;
				strcpy(buf, buf1);
				break;
			case 'l':
				half_chop(buf1, arg, buf);
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				half_chop(buf1, name_search, buf);
				break;
			case 'r':
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					who_room = true;
				strcpy(buf, buf1);
				break;
			case 'c':
				half_chop(buf1, arg, buf);
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
				{
					const size_t len = strlen(arg);
					for (size_t i = 0; i < len; i++)
					{
						showclass |= find_class_bitvector(arg[i]);
					}
				}
				break;
			case 'h':
			case '?':
			default:
				if (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					send_to_char(IMM_WHO_FORMAT, ch);
				else
					send_to_char(MORT_WHO_FORMAT, ch);
				return;
			}	// end of switch
		}
		else  	// endif
		{
			strcpy(name_search, arg);
			strcpy(buf, buf1);

		}
	}			// end while (parser)

	if (who_spamcontrol(ch, strlen(name_search) ? WHO_LISTNAME : WHO_LISTALL))
		return;

	// Строки содержащие имена
	sprintf(buf, "%sБОГИ%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	std::string imms(buf);

	sprintf(buf, "%sПривилегированные%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	std::string demigods(buf);

	sprintf(buf, "%sИгроки%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	std::string morts(buf);

	int all = 0;

	for (const auto tch: character_list)
	{
		if (IS_NPC(tch))
			continue;

		if (!HERE(tch))
			continue;

		if (!*argument && GET_LEVEL(tch) < LVL_IMMORT)
			++all;

		if (*name_search && !(isname(name_search, GET_NAME(tch))))
			continue;

		if (!CAN_SEE_CHAR(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
			continue;
		if (localwho && world[ch->in_room]->zone != world[tch->in_room]->zone)
			continue;
		if (who_room && (tch->in_room != ch->in_room))
			continue;
		if (showclass && !(showclass & (1 << GET_CLASS(tch))))
			continue;
		if (showname && !(!NAME_GOD(tch) && GET_LEVEL(tch) <= NAME_LEVEL))
			continue;
		if (PLR_FLAGGED(tch, PLR_NAMED) && NAME_DURATION(tch) && !IS_IMMORTAL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO) && ch != tch.get())
			continue;

		*buf = '\0';
		num_can_see++;

		if (short_list)
		{
			char tmp[MAX_INPUT_LENGTH];
			snprintf(tmp, sizeof(tmp), "%s%s%s", CCPK(ch, C_NRM, tch), GET_NAME(tch), CCNRM(ch, C_NRM));
			if (IS_IMPL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				sprintf(buf, "%s[%2d %s %s] %-30s%s",
					IS_GOD(tch) ? CCWHT(ch, C_SPR) : "",
					GET_LEVEL(tch), KIN_ABBR(tch), CLASS_ABBR(tch),
					tmp, IS_GOD(tch) ? CCNRM(ch, C_SPR) : "");
			}
			else
			{
				sprintf(buf, "%s%-30s%s",
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
					tmp, IS_IMMORTAL(tch) ? CCNRM(ch, C_SPR) : "");
			}
		}
		else
		{
			if (IS_IMPL(ch)
				|| PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				sprintf(buf, "%s[%2d %2d %s(%5d)] %s%s%s%s",
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
					GET_LEVEL(tch),
					GET_REMORT(tch),
					CLASS_ABBR(tch),
					tch->get_pfilepos(),
					CCPK(ch, C_NRM, tch),
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", tch->race_or_title().c_str(), CCNRM(ch, C_NRM));
			}
			else
			{
				sprintf(buf, "%s %s%s%s",
					CCPK(ch, C_NRM, tch),
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", tch->race_or_title().c_str(), CCNRM(ch, C_NRM));
			}

			if (GET_INVIS_LEV(tch))
				sprintf(buf + strlen(buf), " (i%d)", GET_INVIS_LEV(tch));
			else if (AFF_FLAGGED(tch, EAffectFlag::AFF_INVISIBLE))
				sprintf(buf + strlen(buf), " (невидим%s)", GET_CH_SUF_6(tch));
			if (AFF_FLAGGED(tch, EAffectFlag::AFF_HIDE))
				strcat(buf, " (прячется)");
			if (AFF_FLAGGED(tch, EAffectFlag::AFF_CAMOUFLAGE))
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
			if ( (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_DEMIGOD)) &&  !NAME_GOD(tch)
					&& GET_LEVEL(tch) <= NAME_LEVEL)
			{
				sprintf(buf + strlen(buf), " &W!НЕ ОДОБРЕНО!&n");
				if (showname)
				{
					sprintf(buf + strlen(buf),
							"\r\nПадежи: %s/%s/%s/%s/%s/%s Email: &S%s&s Пол: %s",
							GET_PAD(tch, 0), GET_PAD(tch, 1), GET_PAD(tch, 2),
							GET_PAD(tch, 3), GET_PAD(tch, 4), GET_PAD(tch, 5), 
							GET_GOD_FLAG(ch, GF_DEMIGOD) ? "скрыто" : GET_EMAIL(tch),
							genders[static_cast<int>(GET_SEX(tch))]);
				}
			}
			if ((GET_LEVEL(ch) == LVL_IMPL) && (RENTABLE(tch)))
			    sprintf(buf + strlen(buf), " &R(В КРОВИ)&n");
			else if ((IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO)) && NAME_BAD(tch))
			{
				sprintf(buf + strlen(buf), " &Wзапрет %s!&n", get_name_by_id(NAME_ID_GOD(tch)));
			}
			if (IS_GOD(ch) && (GET_GOD_FLAG(tch, GF_TESTER) || PRF_FLAGGED(tch, PRF_TESTER)))
				sprintf(buf + strlen(buf), " &G(ТЕСТЕР!)&n");
			if (IS_GOD(ch) && (PLR_FLAGGED(tch, PLR_AUTOBOT)))
				sprintf(buf + strlen(buf), " &G(БОТ!)&n");
			if (IS_IMMORTAL(tch))
				strcat(buf, CCNRM(ch, C_SPR));
		}		// endif shortlist

		if (IS_IMMORTAL(tch))
		{
			imms_num++;
			imms += buf;
			if (!short_list || !(imms_num % 4))
			{
				imms += "\r\n";
			}
		}
		else if (GET_GOD_FLAG(tch, GF_DEMIGOD)
			&& (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO) || GET_GOD_FLAG(tch, GF_DEMIGOD)))
		{
			demigods_num++;
			demigods += buf;
			if (!short_list || !(demigods_num % 4))
			{
				demigods += "\r\n";
			}
		}
		else
		{
			morts_num++;
			morts += buf;
			if (!short_list || !(morts_num % 4))
				morts += "\r\n";
		}
	}			// end of for

	if (morts_num + imms_num + demigods_num == 0)
	{
		send_to_char("\r\nВы никого не видите.\r\n", ch);
		// !!!
		return;
	}

	std::string out;

	if (imms_num > 0)
	{
		out += imms;
	}
	if (demigods_num > 0)
	{
		if (short_list)
		{
			out += "\r\n";
		}
		out += demigods;
	}
	if (morts_num > 0)
	{
		if (short_list)
		{
			out += "\r\n";
		}
		out += morts;
	}

	out += "\r\nВсего:";
	if (imms_num)
	{
		sprintf(buf, " бессмертных %d", imms_num);
		out += buf;
	}
	if (demigods_num)
	{
		sprintf(buf, " привилегированных %d", demigods_num);
		out += buf;
	}
	if (all && morts_num)
	{
		sprintf(buf, " смертных %d (видимых %d)", all, morts_num);
		out += buf;
	}
	else if (morts_num)
	{
		sprintf(buf, " смертных %d", morts_num);
		out += buf;
	}

	out += ".\r\n";
	page_string(ch->desc, out);
}

std::string print_server_uptime()
{
	const auto boot_time = shutdown_parameters.get_boot_time();
	const time_t diff = time(0) - boot_time;
	const int d = diff / 86400;
	const int h = (diff / 3600) % 24;
	const int m = (diff / 60) % 60;
	const int s = diff % 60;
	return boost::str(boost::format("Времени с перезагрузки: %dд %02d:%02d:%02d\r\n") % d % h % m % s);
}

void do_statistic(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int proff[NUM_PLAYER_CLASSES][2];
	int ptot[NUM_PLAYER_CLASSES];
	int i, clan = 0, noclan = 0, hilvl = 0, lowlvl = 0, all = 0, rem = 0, norem = 0, pk = 0, nopk = 0;

	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
	{
		proff[i][0] = 0;
		proff[i][1] = 0;
		ptot[i] = 0;
	}

	for (const auto tch : character_list)
	{
		if (IS_NPC(tch) || GET_LEVEL(tch) >= LVL_IMMORT || !HERE(tch))
			continue;

		if (CLAN(tch))
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
		if (pk_count(tch.get()) >= 1)
		{
			pk++;
		}
		else
		{
			nopk++;
		}

		if (GET_LEVEL(tch) >= 25)
			proff[static_cast<int>(GET_CLASS(tch))][0]++;
		else
			proff[static_cast<int>(GET_CLASS(tch))][1]++;
		ptot[static_cast<int>(GET_CLASS(tch))]++;
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
	sprintf(buf + strlen(buf), "Всего игроков %s[%s%*d%s]%s\r\n\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, all, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	char buf_[MAX_INPUT_LENGTH];
	std::string out;

	out += print_server_uptime();
	snprintf(buf_, sizeof(buf_),
		"Героев (без ПК) | Тварей убито  %s[%s%3d%s|%s %2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), char_stat::pkilled,
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), char_stat::mkilled,
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	out += buf_;
	out += char_stat::print_class_exp(ch);

	send_to_char(out, ch);
}


#define USERS_FORMAT \
"Формат: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"
#define MAX_LIST_LEN 200
void do_users(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
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
	int showremorts = 0, showemail = 0, locating = 0;
	char sorting = '!';
	DESCRIPTOR_DATA *d;
	int low = 0, high = LVL_IMPL, num_can_see = 0;
	int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

	host_search[0] = name_search[0] = '\0';

	strcpy(buf, argument);
	while (*buf)
	{
		half_chop(buf, arg, buf1);
		if (*arg == '-')
		{
			mode = *(arg + 1);	// just in case; we destroy arg in the switch
			switch (mode)
			{
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
			{
				playing = 1;
				half_chop(buf1, arg, buf);
				const size_t len = strlen(arg);
				for (size_t i = 0; i < len; i++)
				{
					showclass |= find_class_bitvector(arg[i]);
				}
				break;
			}
			case 'e':
				showemail = 1;
				strcpy(buf, buf1);
				break;
			case 'r':
				showremorts = 1;
				strcpy(buf, buf1);
				break;

			case 's':
				//sorting = 'i';
				sorting = *(arg + 2);
				strcpy(buf, buf1);
				break;
			default:
				send_to_char(USERS_FORMAT, ch);
				return;
			}	// end of switch

		}
		else  	// endif
		{
			strcpy(name_search, arg);
			strcpy(buf, buf1);
		}
	}			// end while (parser)
	if (showemail)
	{
		strcpy(line, "Ном Професс       Имя         Состояние       Idl Логин    Сайт       E-mail\r\n");
	}
	else
	{
		strcpy(line, "Ном Професс       Имя         Состояние       Idl Логин    Сайт\r\n");
	}
	strcat(line, "--- ---------- ------------ ----------------- --- -------- ----------------------------\r\n");
	send_to_char(line, ch);

	one_argument(argument, arg);

// Хорс
	if (strlen(host_by_name) != 0)
	{
		strcpy(host_search, "!");
	}

	for (d = descriptor_list, count_pl = 0; d && count_pl < MAX_LIST_LEN; d = d->next, count_pl++)
	{
		list_players[count_pl] = d;

		const auto character = d->get_character();
		if (!character)
		{
			continue;
		}

		if (isname(host_by_name, GET_NAME(character)))
		{
			strcpy(host_search, d->host);
		}
	}

	if (sorting != '!')
	{
		is = 1;
		while (is)
		{
			is = 0;
			for (cycle_i = 1; cycle_i < count_pl; cycle_i++)
			{
				flag_change = 0;
				d = list_players[cycle_i - 1];

				const auto t = d->get_character();

				d_tmp = list_players[cycle_i];

				const auto t_tmp = d_tmp->get_character();

				switch (sorting)
				{
				case 'n':
					if (0 < strcoll(t ? t->get_pc_name().c_str() : "", t_tmp ? t_tmp->get_pc_name().c_str() : ""))
					{
						flag_change = 1;
					}
					break;

				case 'e':
					if (strcoll(t ? GET_EMAIL(t) : "", t_tmp ? GET_EMAIL(t_tmp) : "") > 0)
						flag_change = 1;
					break;

				default:
					a1 = get_ip(const_cast<char*>(d->host));
					a2 = get_ip(const_cast<char*>(d_tmp->host));
					if (a1 > a2)
						flag_change = 1;
				}
				if (flag_change)
				{
					list_players[cycle_i - 1] = d_tmp;
					list_players[cycle_i] = d;
					is = 1;
				}
			}
		}
	}

	for (cycle_i = 0; cycle_i < count_pl; cycle_i++)
	{
		d = list_players[cycle_i];
// ---
		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (STATE(d) == CON_PLAYING)
		{
			const auto character = d->get_character();
			if (!character)
			{
				continue;
			}

			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && !isname(name_search, GET_NAME(character)))
				continue;
			if (!CAN_SEE(ch, character) || GET_LEVEL(character) < low || GET_LEVEL(character) > high)
				continue;
			if (outlaws && !PLR_FLAGGED((ch), PLR_KILLER))
				continue;
			if (showclass && !(showclass & (1 << GET_CLASS(character))))
				continue;
			if (GET_INVIS_LEV(character) > GET_LEVEL(ch))
				continue;

			if (d->original)
				if (showremorts)
					sprintf(classname, "[%2d %2d %s %s]", GET_LEVEL(d->original), GET_REMORT(d->original), KIN_ABBR(d->original), CLASS_ABBR(d->original));
				else
					sprintf(classname, "[%2d %s %s]   ", GET_LEVEL(d->original), KIN_ABBR(d->original), CLASS_ABBR(d->original));
			else
				if (showremorts)
					sprintf(classname, "[%2d %2d %s %s]", GET_LEVEL(d->character), GET_REMORT(d->character), KIN_ABBR(d->character), CLASS_ABBR(d->character));
				else
					sprintf(classname, "[%2d %s %s]   ", GET_LEVEL(d->character), KIN_ABBR(d->character), CLASS_ABBR(d->character));
		}
		else
		{
			strcpy(classname, "      -      ");
		}

		if (GET_LEVEL(ch) < LVL_IMPL && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			strcpy(classname, "      -      ");
		}

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CON_PLAYING && d->original)
			strcpy(state, "Switched");
		else
			sprinttype(STATE(d), connected_types, state);

		if (d->character
			&& STATE(d) == CON_PLAYING
			&& !IS_GOD(d->character))
		{
			sprintf(idletime, "%3d", d->character->char_specials.timer *
				SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		}
		else
		{
			strcpy(idletime, "");
		}

		if (d->character
			&& d->character->get_pc_name().c_str())
		{
			if (d->original)
			{
				sprintf(line, format, d->desc_num, classname, d->original->get_pc_name().c_str(), state, idletime, timeptr);
			}
			else
			{
				sprintf(line, format, d->desc_num, classname, d->character->get_pc_name().c_str(), state, idletime, timeptr);
			}
		}
		else
		{
			sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED", state, idletime, timeptr);
		}

// Хорс
		if (d && *d->host)
		{
			sprintf(line2, "[%s]", d->host);
			strcat(line, line2);
		}
		else
		{
			strcat(line, "[Неизвестный хост]");
		}

		if (showemail)
		{
			sprintf(line2, "[&S%s&s]",
					d->original ? GET_EMAIL(d->original) : d->character ? GET_EMAIL(d->character) : "");
			strcat(line, line2);
		}

		if (locating && (*name_search || *host_by_name))
		{
			if (STATE(d) == CON_PLAYING)
			{
				const auto ci = d->get_character();
				if (ci
					&& CAN_SEE(ch, ci)
					&& ci->in_room != NOWHERE)
				{
					if (d->original && d->character)
					{
						sprintf(line2, " [%5d] %s (in %s)",
							GET_ROOM_VNUM(IN_ROOM(d->character)),
							world[d->character->in_room]->name, GET_NAME(d->character));
					}
					else
					{
						sprintf(line2, " [%5d] %s",
							GET_ROOM_VNUM(IN_ROOM(ci)), world[ci->in_room]->name);
					}
				}

				strcat(line, line2);
			}
		}

//--
		strcat(line, "\r\n");
		if (STATE(d) != CON_PLAYING)
		{
			sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}

		if (STATE(d) != CON_PLAYING || (STATE(d) == CON_PLAYING && d->character && CAN_SEE(ch, d->character)))
		{
			send_to_char(line, ch);
			num_can_see++;
		}
	}

	sprintf(line, "\r\n%d видимых соединений.\r\n", num_can_see);
	page_string(ch->desc, line, TRUE);
}

// Generic page_string function for displaying text
void do_gen_ps(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int subcmd)
{
	//DESCRIPTOR_DATA *d;
	switch (subcmd)
	{
	case SCMD_CREDITS:
		page_string(ch->desc, credits, 0);
		break;
	case SCMD_INFO:
		page_string(ch->desc, info, 0);
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
		show_code_date(ch);
		break;
	case SCMD_WHOAMI:
	{
		//Изменения. Фиопий.
		sprintf(buf, "Персонаж : %s\r\n", GET_NAME(ch));
		sprintf(buf + strlen(buf),
				"Падежи : &W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n\r\n",
				ch->get_name().c_str(), GET_PAD(ch, 1), GET_PAD(ch, 2),
				GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5));

		sprintf(buf + strlen(buf), "Ваш e-mail : &S%s&s\r\n", GET_EMAIL(ch));
		time_t birt = ch->player_data.time.birth;
		sprintf(buf + strlen(buf), "Дата вашего рождения : %s\r\n", rustime(localtime(&birt)));
		sprintf(buf + strlen(buf), "Ваш IP-адрес : %s\r\n", ch->desc ? ch->desc->host : "Unknown");
//               GET_LASTIP (ch));
		send_to_char(buf, ch);
		if (!NAME_GOD(ch))
		{
			sprintf(buf, "Имя никем не одобрено!\r\n");
			send_to_char(buf, ch);
		}
		else
		{
			const int god_level = NAME_GOD(ch) > 1000 ? NAME_GOD(ch) - 1000 : NAME_GOD(ch);
			sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(ch)));
			*buf1 = UPPER(*buf1);

			static const char *by_rank_god = "Богом";
			static const char *by_rank_privileged = "привилегированным игроком";
			const char * by_rank = god_level < LVL_IMMORT ?  by_rank_privileged : by_rank_god;

			if (NAME_GOD(ch) < 1000)
				sprintf(buf, "&RИмя запрещено %s %s&n\r\n", by_rank, buf1);
			else
				sprintf(buf, "&WИмя одобрено %s %s&n\r\n", by_rank, buf1);
			send_to_char(buf, ch);
		}
		sprintf(buf, "Перевоплощений: %d\r\n", GET_REMORT(ch));
		send_to_char(buf, ch);
		//Конец изменений. Фиопий.
		Clan::CheckPkList(ch);
		break;
	}
	default:
		log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
		return;
	}
}

void perform_mortal_where(CHAR_DATA * ch, char *arg)
{
	DESCRIPTOR_DATA *d;

	send_to_char("Кто много знает, тот плохо спит.\r\n", ch);
	return;

	if (!*arg)
	{
		send_to_char("Игроки, находящиеся в зоне\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
		{
			if (STATE(d) != CON_PLAYING
				|| d->character.get() == ch)
			{
				continue;
			}

			const auto i = d->get_character();
			if (!i)
			{
				continue;
			}

			if (i->in_room == NOWHERE
				|| !CAN_SEE(ch, i))
			{
				continue;
			}

			if (world[ch->in_room]->zone != world[i->in_room]->zone)
			{
				continue;
			}

			sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			send_to_char(buf, ch);
		}
	}
	else  		// print only FIRST char, not all.
	{
		for (const auto i : character_list)
		{
			if (i->in_room == NOWHERE
				|| i.get() == ch)
			{
				continue;
			}

			if (!CAN_SEE(ch, i)
				|| world[i->in_room]->zone != world[ch->in_room]->zone)
			{
				continue;
			}

			if (!isname(arg, i->get_pc_name()))
			{
				continue;
			}

			sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			send_to_char(buf, ch);
			return;
		}
		send_to_char("Никого похожего с этим именем нет.\r\n", ch);
	}
}

void print_object_location(int num, const OBJ_DATA * obj, CHAR_DATA * ch)
{
	if (num > 0)
	{
		sprintf(buf, "%2d. ", num);
		if(IS_GRGOD(ch))
		{
			sprintf(buf2, "[%6d] %-25s - ", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
			strcat(buf, buf2);
		}
		else
		{
			sprintf(buf2, "%-34s - ", obj->get_short_description().c_str());
			strcat(buf, buf2);
		}
	}
	else
	{
		sprintf(buf, "%41s", " - ");
	}

	if (obj->get_in_room() > NOWHERE)
	{
		sprintf(buf + strlen(buf), "[%5d] %s", GET_ROOM_VNUM(obj->get_in_room()), world[obj->get_in_room()]->name);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else if (obj->get_carried_by())
	{
		sprintf(buf + strlen(buf), "затарено %s[%d] в комнате [%d]",
			PERS(obj->get_carried_by(), ch, 4),
			GET_MOB_VNUM(obj->get_carried_by()),
			world[obj->get_carried_by()->in_room]->number);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else if (obj->get_worn_by())
	{
		sprintf(buf + strlen(buf), "надет на %s[%d] в комнате [%d]",
			PERS(obj->get_worn_by(), ch, 3),
			GET_MOB_VNUM(obj->get_worn_by()),
			world[obj->get_worn_by()->in_room]->number);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else if (obj->get_in_obj())
	{
		if (Clan::is_clan_chest(obj->get_in_obj()))// || Clan::is_ingr_chest(obj->get_in_obj())) сделать отдельный поиск
		{
			return; // шоб не забивало локейт на мобах/плеерах - по кланам проходим ниже отдельно
		}
		else
		{
			sprintf(buf + strlen(buf), "лежит в [%d]%s, который находится \r\n",
				GET_OBJ_VNUM(obj->get_in_obj()), obj->get_in_obj()->get_PName(5).c_str());
			send_to_char(buf, ch);
			print_object_location(0, obj->get_in_obj(), ch);
		}
	}
	else
	{
		sprintf(buf + strlen(buf), "находится где-то там, далеко-далеко.");
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
}

/**
* Иммский поиск шмоток по 'где' с проходом как по глобальному списку, так
* и по спискам хранилищ и почты.
*/
bool print_imm_where_obj(CHAR_DATA *ch, char *arg, int num)
{
	bool found = false;

	world_objects.foreach([&](const OBJ_DATA::shared_ptr object)	/* maybe it is possible to create some index instead of linear search */
	{
		if (isname(arg, object->get_aliases()))
		{
			found = true;
			print_object_location(num++, object.get(), ch);
		}
	});

	int tmp_num = num;
	if (IS_GOD(ch)
		|| PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		tmp_num = Clan::print_spell_locate_object(ch, tmp_num, arg);
		tmp_num = Depot::print_imm_where_obj(ch, arg, tmp_num);
		tmp_num = Parcel::print_imm_where_obj(ch, arg, tmp_num);
	}

	if (!found
		&& tmp_num == num)
	{
		return false;
	}
	else
	{
		num = tmp_num;
		return true;
	}
}

void perform_immort_where(CHAR_DATA * ch, char *arg)
{
	DESCRIPTOR_DATA *d;
	int num = 1, found = 0;

	if (!*arg)
	{
		if (GET_LEVEL(ch) < LVL_IMPL && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Где КТО конкретно?", ch);
		}
		else
		{
			send_to_char("ИГРОКИ\r\n------\r\n", ch);
			for (d = descriptor_list; d; d = d->next)
			{
				if (STATE(d) == CON_PLAYING)
				{
					const auto i = d->get_character();
					if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE))
					{
						if (d->original)
						{
							sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
								GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(d->character)),
								world[d->character->in_room]->name,
								GET_NAME(d->character));
						}
						else
						{
							sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(i)), world[i->in_room]->name);
						}
						send_to_char(buf, ch);
					}
				}
			}
		}
	}
	else
	{
		for (const auto i : character_list)
		{
			if (CAN_SEE(ch, i)
				&& i->in_room != NOWHERE
				&& isname(arg, i->get_pc_name()))
			{
			    ZoneData *zone = &zone_table[world[i->in_room]->zone];
				found = 1;
				sprintf(buf, "%s%3d. %-25s - [%5d] %s. Название зоны: '%s'\r\n", IS_NPC(i)? "Моб:  ":"Игрок:", num++, GET_NAME(i),
						GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)]->name, zone->name);
				send_to_char(buf, ch);
			}
		}

		if (!print_imm_where_obj(ch, arg, num)
			&& !found)
		{
			send_to_char("Нет ничего похожего.\r\n", ch);
		}
	}
}

void do_where(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	one_argument(argument, arg);

	if (IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		perform_immort_where(ch, arg);
	else
		perform_mortal_where(ch, arg);
}

void do_levels(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;
	char *ptr = &buf[0];

	if (IS_NPC(ch))
	{
		send_to_char("Боги уже придумали ваш уровень.\r\n", ch);
		return;
	}
	*ptr = '\0';

	ptr += sprintf(ptr, "Уровень          Опыт            Макс на урв.\r\n");
	for (i = 1; i < LVL_IMMORT; i++)
	{	
		ptr += sprintf(ptr, "%s[%2d] %13s-%-13s %-13s%s\r\n", (ch->get_level() == i) ? CCICYN(ch, C_NRM) : "", i,
			thousands_sep(level_exp(ch, i)).c_str(),
			thousands_sep(level_exp(ch, i + 1) - 1).c_str(),
			thousands_sep((int) (level_exp(ch, i + 1) - level_exp(ch, i)) / (10 + GET_REMORT(ch))).c_str(),
			(ch->get_level() == i) ? CCNRM(ch, C_NRM) : "");
	}

	ptr += sprintf(ptr, "%s[%2d] %13s               (БЕССМЕРТИЕ)%s\r\n",
		(ch->get_level() >= LVL_IMMORT) ? CCICYN(ch, C_NRM) : "", LVL_IMMORT,
		thousands_sep(level_exp(ch, LVL_IMMORT)).c_str(),
		(ch->get_level() >= LVL_IMMORT) ? CCNRM(ch, C_NRM) : "");
	page_string(ch->desc, buf, 1);
}

void do_consider(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	int diff;

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
	{
		send_to_char("Кого вы хотите оценить?\r\n", ch);
		return;
	}
	if (victim == ch)
	{
		send_to_char("Легко! Выберите параметр <Удалить персонаж>!\r\n", ch);
		return;
	}
	if (!IS_NPC(victim))
	{
		send_to_char("Оценивайте игроков сами - тут я не советчик.\r\n", ch);
		return;
	}
	diff = (GET_LEVEL(victim) - GET_LEVEL(ch) - GET_REMORT(ch));

	if (diff <= -10)
		send_to_char("Ути-пути, моя рыбонька.\r\n", ch);
	else if (diff <= -5)
		send_to_char("\"Сделаем без шуму и пыли!\"\r\n", ch);
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
		send_to_char("Удача и хорошее снаряжение вам сильно пригодятся!\r\n", ch);
	else if (diff <= 5)
		send_to_char("Вы берете на себя слишком много.\r\n", ch);
	else if (diff <= 10)
		send_to_char("Ладно, войдете еще раз.\r\n", ch);
	else if (diff <= 100)
		send_to_char("Срочно к психиатру - вы страдаете манией величия!\r\n", ch);

}

void do_diagnose(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;

	one_argument(argument, buf);

	if (*buf)
	{
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
			send_to_char(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	}
	else
	{
		if (ch->get_fighting())
			diag_char_to_char(ch->get_fighting(), ch);
		else
			send_to_char("На кого вы хотите взглянуть?\r\n", ch);
	}
}

const char *ctypes[] = { "выключен", "простой", "обычный", "полный", "\n" };

void do_toggle(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
		return;
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "нет");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

	if (GET_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		sprintf(buf,
				" Нет агров     : %-3s     "
				" Супервидение  : %-3s     "
				" Флаги комнат  : %-3s \r\n"
				" Частный режим : %-3s     "
				" Замедление    : %-3s     "
				" Кодер         : %-3s \r\n"
				" Опечатки      : %-3s \r\n",
				ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
				ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
				ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)),
				ONOFF(PRF_FLAGGED(ch, PRF_NOWIZ)),
				ONOFF(nameserver_is_slow),
				ONOFF(PRF_FLAGGED(ch, PRF_CODERINFO)),
				ONOFF(PRF_FLAGGED(ch, PRF_MISPRINT)));
		send_to_char(buf, ch);
	}

	sprintf(buf,
			" Автовыходы    : %-3s     "
			" Краткий режим : %-3s     "
			" Сжатый режим  : %-3s \r\n"
			" Повтор команд : %-3s     "
			" Обращения     : %-3s     "
			" Цвет          : %-8s\r\n"
			" Кто-то        : %-6s  "
			" Болтать       : %-3s     "
			" Орать         : %-3s \r\n"
			" Аукцион       : %-3s     "
			" Базар         : %-3s     "
			" Автозаучивание: %-3s \r\n"
			" Призыв        : %-3s     "
			" Автозавершение: %-3s     "
			" Группа (вид)  : %-7s \r\n"
			" Без двойников : %-3s     "
			" Автодележ     : %-3s     "
			" Автограбеж    : %-7s \r\n"
			" Брать куны    : %-3s     "
			" Арена         : %-3s     "
			" Трусость      : %-3s \r\n"
			" Ширина экрана : %-3d     "
			" Высота экрана : %-3d     "
			" Сжатие        : %-6s \r\n"
			" Новости (вид) : %-5s   "
			" Доски         : %-3s     "
			" Хранилище     : %-10s\r\n"
			" Пклист        : %-3s     "
			" Политика      : %-3s     "
			" Пкформат      : %-10s\r\n"
			" Соклановцы    : %-3s     "
			" Оффтоп        : %-3s     "
			" Потеря связи  : %-3s \r\n"
			" Ингредиенты   : %-3s     "
			" Вспомнить     : %-3u     ",
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
			ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
			ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
			YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOTELL)),
			ctypes[COLOR_LEV(ch)],
			PRF_FLAGGED(ch, PRF_NOINVISTELL) ? "нельзя" : "можно",
			ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOHOLLER)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOEXCHANGE)),
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),
			ONOFF(PRF_FLAGGED(ch, PRF_SUMMONABLE)),
			ONOFF(PRF_FLAGGED(ch, PRF_GOAHEAD)),
			PRF_FLAGGED(ch, PRF_SHOWGROUP) ? "полный" : "краткий",
			ONOFF(PRF_FLAGGED(ch, PRF_NOCLONES)),
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
			PRF_FLAGGED(ch, PRF_AUTOLOOT) ? PRF_FLAGGED(ch, PRF_NOINGR_LOOT) ? "NO-INGR" : "ALL    " : "OFF    ",
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOMONEY)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOARENA)),
			buf2,
			STRING_LENGTH(ch),
			STRING_WIDTH(ch),
#if defined(HAVE_ZLIB)
			ch->desc->deflate == NULL ? "нет" : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"),
#else
			"N/A",
#endif
			PRF_FLAGGED(ch, PRF_NEWS_MODE) ? "доска" : "лента",
			ONOFF(PRF_FLAGGED(ch, PRF_BOARD_MODE)),
			GetChestMode(ch).c_str(),
			ONOFF(PRF_FLAGGED(ch, PRF_PKL_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_POLIT_MODE)),
			PRF_FLAGGED(ch, PRF_PKFORMAT_MODE) ? "краткий" : "полный",
			ONOFF(PRF_FLAGGED(ch, PRF_WORKMATE_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_OFFTOP_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_ANTIDC_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_NOINGR_MODE)),
			ch->remember_get_num());
	send_to_char(buf, ch);
	if (NOTIFY_EXCH_PRICE(ch) > 0)
	{
		sprintf(buf,  " Уведомления   : %-3ld \r\n", NOTIFY_EXCH_PRICE(ch));
	}
	else
	{
		sprintf(buf,  " Уведомления   : %-3s \r\n", "Нет");
	}
	send_to_char(buf, ch);

	sprintf(buf,
		" Карта         : %-3s     "
		" Вход в зону   : %-3s     "
		" Магщиты (вид) : %s\r\n"
		" Автопризыв    : %-3s     "
		" Маппер        : %-3s     "
		" Контроль IP   : %-3s",
		ONOFF(PRF_FLAGGED(ch, PRF_DRAW_MAP)),
		ONOFF(PRF_FLAGGED(ch, PRF_ENTER_ZONE)),
		(PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS) ? "краткий" : "полный"),
		ONOFF(PRF_FLAGGED(ch, PRF_AUTO_NOSUMMON)),
		ONOFF(PRF_FLAGGED(ch, PRF_MAPPER)),
		ONOFF(PRF_FLAGGED(ch, PRF_IPCONTROL)));
	send_to_char(buf, ch);
	if (GET_GOD_FLAG(ch, GF_TESTER))
		sprintf(buf, " Тестер        : %-3s\r\n", ONOFF(PRF_FLAGGED(ch, PRF_TESTER)));
	else
		sprintf(buf, "\r\n");
	send_to_char(buf, ch);
}

void do_zone(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (ch->desc
		&& !(IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !can_use_feat(ch, DARK_READING_FEAT))
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		MapSystem::print_map(ch);
	}

	print_zone_info(ch);

	if ((IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		&& zone_table[world[ch->in_room]->zone].comment)
	{
		send_to_char(ch, "Комментарий: %s.\r\n",
			zone_table[world[ch->in_room]->zone].comment);
	}
}


struct sort_struct
{
	int sort_pos;
	byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
	int a, b, tmp;

	num_of_cmds = 0;

	 // first, count commands (num_of_commands is actually one greater than the
	 // number of commands; it inclues the '\n'.

	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	// create data array
	CREATE(cmd_sort_info, num_of_cmds);

	// initialize it
	for (a = 1; a < num_of_cmds; a++)
	{
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social = FALSE;
	}

	// the infernal special case
	cmd_sort_info[find_command("insult")].is_social = TRUE;

	// Sort.  'a' starts at 1, not 0, to remove 'RESERVED'
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
					   cmd_info[cmd_sort_info[b].sort_pos].command) > 0)
			{
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}

void do_commands(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	int no, i, cmd_num, num_of;
	int wizhelp = 0, socials = 0;
	CHAR_DATA *vict = ch;

	one_argument(argument, arg);

	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;

	sprintf(buf, "Следующие %s%s доступны %s:\r\n",
			wizhelp ? "привилегированные " : "",
			socials ? "социалы" : "команды", vict == ch ? "вам" : GET_PAD(vict, 2));

	if (socials)
		num_of = number_of_social_commands;
	else
		num_of = num_of_cmds - 1;

	// cmd_num starts at 1, not 0, to remove 'RESERVED'
	for (no = 1, cmd_num = socials ? 0 : 1; cmd_num < num_of; cmd_num++)
		if (socials)
		{
			sprintf(buf + strlen(buf), "%-19s", soc_keys_list[cmd_num].keyword);
			if (!(no % 4))
				strcat(buf, "\r\n");
			no++;
		}
		else
		{
			i = cmd_sort_info[cmd_num].sort_pos;
			if (cmd_info[i].minimum_level >= 0
					&& (Privilege::can_do_priv(vict, std::string(cmd_info[i].command), i, 0))
					&& (cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp
					&& (wizhelp || socials == cmd_sort_info[i].is_social))
			{
				sprintf(buf + strlen(buf), "%-15s", cmd_info[i].command);
				if (!(no % 5))
					strcat(buf, "\r\n");
				no++;
			}
		}

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}

std::array<EAffectFlag, 3> hiding = { EAffectFlag::AFF_SNEAK, EAffectFlag::AFF_HIDE, EAffectFlag::AFF_CAMOUFLAGE };

void do_affects(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	char sp_name[MAX_STRING_LENGTH];

	// Show the bitset without "hiding" etc.
	auto aff_copy = ch->char_specials.saved.affected_by;
	for (auto j : hiding)
	{
		aff_copy.unset(j);
	}

	aff_copy.sprintbits(affected_bits, buf2, ",");
	snprintf(buf, MAX_STRING_LENGTH, "Аффекты: %s%s%s\r\n", CCIYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	// Routine to show what spells a char is affected by
	if (!ch->affected.empty())
	{
		for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i)
		{
			const auto aff = *affect_i;

			if (aff->type == SPELL_SOLOBONUS)
			{
				continue;
			}

			*buf2 = '\0';
			strcpy(sp_name, spell_name(aff->type));
			int mod = 0;
			if (aff->battleflag == AF_PULSEDEC)
			{
				mod = aff->duration / 51; //если в пульсах приводим к тикам 25.5 в сек 2 минуты
			}
			else
			{
				mod = aff->duration;
			}
			(mod + 1) / SECS_PER_MUD_HOUR
				? sprintf(buf2, "(%d %s)", (mod + 1) / SECS_PER_MUD_HOUR + 1, desc_count((mod + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR))
				: sprintf(buf2, "(менее часа)");
			sprintf(buf, "%s%s%-21s %-12s%s ",
					*sp_name == '!' ? "Состояние  : " : "Заклинание : ",
					CCICYN(ch, C_NRM), sp_name, buf2, CCNRM(ch, C_NRM));
			*buf2 = '\0';
			if (!IS_IMMORTAL(ch))
			{
				auto next_affect_i = affect_i;
				++next_affect_i;
				if (next_affect_i != ch->affected.end())
				{
					const auto& next_affect = *next_affect_i;
					if (aff->type == next_affect->type)
					{
						continue;
					}
				}
			}
			else
			{
				if (aff->modifier)
				{
					sprintf(buf2, "%-3d к параметру: %s", aff->modifier, apply_types[(int) aff->location]);
					strcat(buf, buf2);
				}
				if (aff->bitvector)
				{
					if (*buf2)
					{
						strcat(buf, ", устанавливает ");
					}
					else
					{
						strcat(buf, "устанавливает ");
					}
					strcat(buf, CCIRED(ch, C_NRM));
					sprintbit(aff->bitvector, affected_bits, buf2);
					strcat(buf, buf2);
					strcat(buf, CCNRM(ch, C_NRM));
				}
			}
			send_to_char(strcat(buf, "\r\n"), ch);
		}
// отображение наград
		for (const auto& aff : ch->affected)
		{
		    if (aff->type == SPELL_SOLOBONUS)
		    {
				int mod;
				if (aff->battleflag == AF_PULSEDEC)
				{
					mod = aff->duration / 51; //если в пульсах приводим к тикам	25.5 в сек 2 минуты
				}
				else
				{
					mod = aff->duration;
				}
				(mod + 1) / SECS_PER_MUD_HOUR
					? sprintf(buf2, "(%d %s)", (mod + 1) / SECS_PER_MUD_HOUR + 1, desc_count((mod + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR))
					: sprintf(buf2, "(менее часа)");
			    sprintf(buf, "Заклинание : %s%-21s %-12s%s ", CCICYN(ch, C_NRM),  "награда",  buf2, CCNRM(ch, C_NRM));
			    *buf2 = '\0';
			    if (aff->modifier)
			    {	
				    sprintf(buf2, "%s%-3d к параметру: %s%s%s",(aff->modifier > 0)? "+": "",  aff->modifier, CCIRED(ch, C_NRM), apply_types[(int) aff->location], CCNRM(ch, C_NRM));
				    strcat(buf, buf2);
			    }
			    send_to_char(strcat(buf, "\r\n"), ch);
		    }
		}
	}

	if (ch->is_morphed())
	{
		*buf2 = '\0';
		send_to_char("Автоаффекты звериной формы: " , ch);
		const IMorph::affects_list_t& affs = ch->GetMorphAffects();
		for (IMorph::affects_list_t::const_iterator it = affs.begin(); it != affs.end();)
		{
			sprintbit(to_underlying(*it), affected_bits, buf2);
			send_to_char(string(CCIYEL(ch, C_NRM))+ string(buf2)+ string(CCNRM(ch, C_NRM)), ch);
			if (++it != affs.end())
			{
				send_to_char(", ", ch);
			}
		}
	}
}

// Create web-page with users list
void make_who2html(void)
{
	FILE *opf;
	DESCRIPTOR_DATA *d;

	int imms_num = 0, morts_num = 0;

	char *imms = NULL;
	char *morts = NULL;
	char *buffer = NULL;

	if ((opf = fopen(WHOLIST_FILE, "w")) == 0)
		return;		// or log it ? *shrug*
	fprintf(opf, "<HTML><HEAD><TITLE>Кто сейчас в Былинах?</TITLE></HEAD>\n");
	fprintf(opf, "<BODY><H1>Кто сейчас живет в Былинах?</H1><HR>\n");

	sprintf(buf, "БОГИ <BR> \r\n");
	imms = str_add(imms, buf);

	sprintf(buf, "<BR>Игроки<BR> \r\n  ");
	morts = str_add(morts, buf);

	for (d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) == CON_PLAYING
			&& GET_INVIS_LEV(d->character) < 31)
		{
			const auto ch = d->character;
			sprintf(buf, "%s <BR> \r\n ", ch->race_or_title().c_str());

			if (IS_IMMORTAL(ch))
			{
				imms_num++;
				imms = str_add(imms, buf);
			}
			else
			{
				morts_num++;
				morts = str_add(morts, buf);
			}
		}
	}

	if (morts_num + imms_num == 0)
	{
		sprintf(buf, "Все ушли на фронт! <BR>");
		buffer = str_add(buffer, buf);
	}
	else
	{
		if (imms_num > 0)
			buffer = str_add(buffer, imms);
		if (morts_num > 0)
			buffer = str_add(buffer, morts);
		buffer = str_add(buffer, " <BR> \r\n Всего :");
		if (imms_num)
		{
			// sprintf(buf+strlen(buf)," бессмертных %d",imms_num);
			sprintf(buf, " бессмертных %d", imms_num);
			buffer = str_add(buffer, buf);
		}
		if (morts_num)
		{
			// sprintf(buf+strlen(buf)," смертных %d",morts_num);
			sprintf(buf, " смертных %d", morts_num);
			buffer = str_add(buffer, buf);
		}

		buffer = str_add(buffer, ".\n");
	}

	fprintf(opf, "%s", buffer);

	free(buffer);
	free(imms);
	free(morts);


	fprintf(opf, "<HR></BODY></HTML>\n");
	fclose(opf);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
