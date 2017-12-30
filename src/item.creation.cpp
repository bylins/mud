/*************************************************************************
*   File: item.creation.cpp                            Part of Bylins    *
*   Item creation from magic ingidients                                  *
*                           *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */
#include "item.creation.hpp"

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "obj.hpp"
#include "screen.h"
#include "spells.h"
#include "skills.h"
#include "constants.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "olc.h"
#include "im.h"
#include "features.hpp"
#include "char.hpp"
#include "modify.h"
#include "room.hpp"
#include "fight.h"
#include "structs.h"
#include "logger.hpp"
#include "utils.h"
#include "sysdep.h"
#include "conf.h"

#define SpINFO   spell_info[spellnum]
extern int material_value[];
int slot_for_char(CHAR_DATA * ch, int i);
void die(CHAR_DATA * ch, CHAR_DATA * killer);

constexpr auto WEAR_TAKE = to_underlying(EWearFlag::ITEM_WEAR_TAKE);
constexpr auto WEAR_TAKE_BOTHS_WIELD = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_BOTHS) | to_underlying(EWearFlag::ITEM_WEAR_WIELD);
constexpr auto WEAR_TAKE_BODY = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_BODY);
constexpr auto WEAR_TAKE_ARMS= WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_ARMS);
constexpr auto WEAR_TAKE_LEGS = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_LEGS);
constexpr auto WEAR_TAKE_HEAD = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_HEAD);
constexpr auto WEAR_TAKE_BOTHS = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_BOTHS);
constexpr auto WEAR_TAKE_DUAL = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_HOLD) | to_underlying(EWearFlag::ITEM_WEAR_WIELD);
constexpr auto WEAR_TAKE_HOLD = WEAR_TAKE | to_underlying(EWearFlag::ITEM_WEAR_HOLD);
struct create_item_type created_item[] =
{
	{300, 0x7E, 15, 40, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_BOTHS_WIELD},
	{301, 0x7E, 12, 40, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_BOTHS_WIELD },
	{302, 0x7E, 8, 25, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_DUAL },
	{303, 0x7E, 5, 13, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_HOLD },
	{304, 0x7E, 10, 35, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_BOTHS_WIELD },
	{305, 0, 8, 15, {{0, 0, 0}}, SKILL_INVALID, WEAR_TAKE_BOTHS_WIELD },
	{306, 0, 8, 20, {{0, 0, 0}}, SKILL_INVALID, WEAR_TAKE_BOTHS_WIELD },
	{307, 0x3A, 10, 20, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_BODY},
	{308, 0x3A, 4, 10, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_ARMS},
	{309, 0x3A, 6, 12, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_LEGS},
	{310, 0x3A, 4, 10, {{COAL_PROTO, 0, 0}}, SKILL_TRANSFORMWEAPON, WEAR_TAKE_HEAD},
	{312, 0, 4, 40, {{WOOD_PROTO, TETIVA_PROTO, 0}}, SKILL_CREATEBOW, WEAR_TAKE_BOTHS}
};
const char *create_item_name[] = { "шелепуга",
								   "меч",
								   "сабля",
								   "нож",
								   "топор",
								   "плеть",
								   "дубина",
								   "кольчуга",
								   "наручи",
								   "поножи",
								   "шлем",
								   "лук",
								   "\n"
								 };
const struct make_skill_type make_skills[] =
{
//  { "смастерить посох","посохи", SKILL_MAKE_STAFF },
	{"смастерить лук", "луки", SKILL_MAKE_BOW},
	{"выковать оружие", "оружие", SKILL_MAKE_WEAPON},
	{"выковать доспех", "доспех", SKILL_MAKE_ARMOR},
	{"сшить одежду", "одежда", SKILL_MAKE_WEAR},
	{"смастерить диковину", "артеф.", SKILL_MAKE_JEWEL},
	{"смастерить оберег", "оберег", SKILL_MAKE_AMULET},
//  { "сварить отвар","варево", SKILL_MAKE_POTION },
	{"\n", 0, SKILL_INVALID}		// Терминатор
};
const char *create_weapon_quality[] = { "RESERVED",
										"RESERVED",
										"RESERVED",
										"RESERVED",
										"RESERVED",
										"невообразимо ужасного качества",	// <3 //
										"ужасного качества",	// 3 //
										"более чем жуткого качества",
										"жуткого качества",	// 4 //
										"более чем отвратительного качества",
										"отвратительного качества",	// 5 //
										"более чем плохого качества",
										"плохого качества",	// 6 //
										"хуже чем среднего качества",
										"среднего качества",	// 7 //
										"лучше чем среднего качества",
										"неплохого качества",	// 8 //
										"более чем неплохого качества",
										"хорошего качества",	// 9 //
										"более чем хорошего качества",
										"превосходного качества",	// 10 //
										"более чем превосходного качества",
										"великолепного качества",	// 11 //
										"более чем великолепного качества",
										"качества, достойного учеников мастера",	// 12 //
										"лучшего качества, чем делают ученики мастера",
										"качества, достойного мастера",	// 13 //
										"лучшего качества, чем делают мастера",
										"качества, достойного великого мастера",	// 14 //
										"лучшего качества, чем делают великие мастера",
										"качества, достойного руки сына боярского",	// 15 //
										"качества, более чем достойного руки сына боярского",
										"качества, достойного руки боярина",	// 16 //
										"качества, более чем достойного руки боярина",
										"качества, достойного руки сына воеводы",	// 17 //
										"качества, более чем достойного руки сына воеводы",
										"качества, достойного руки воеводы",	// 18 //
										"качества, более чем достойного руки воеводы",
										"качества, достойного руки княжича",	// 19 //
										"качества, более чем достойного руки княжича",
										"качества, достойного руки князя",	// 20 //
										"качества, более чем достойного руки князя",
										"качества, достойного руки героя",	// 21 //
										"качества, более чем достойного руки героя",
										"качества, достойного руки великого героя",	// 22 //
										"качества, более чем достойного руки великого героя",
										"качества, которое знали только древние титаны",	// 23 //
										"качества, лучшего чем то, которое знали древние титаны",
										"качества, достойного младших богов",	// 24 //
										"качества, более чем достойного младших богов",
										"качества, достойного богов",	// 25 //
										"качества, более чем достойного богов",
										"качества, которого достигают немногие", // 26
										"качества, которого достигают немногие",
										"качества, которого достигают немногие", // 27
										"качества, которого достигают немногие",
										"качества, достойного старших богов", // >= 28
										"\n"
									  };
MakeReceptList make_recepts;
// Функция вывода в поток //
CHAR_DATA *& operator<<(CHAR_DATA * &ch, string p)
{
	send_to_char(p.c_str(), ch);
	return ch;
}
void init_make_items()
{
	char tmpbuf[MAX_INPUT_LENGTH];
	sprintf(tmpbuf, "Loading making recepts.");
	mudlog(tmpbuf, LGH, LVL_IMMORT, SYSLOG, TRUE);
	make_recepts.load();
}
// Парсим ввод пользователя в меню правки рецепта
void mredit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	string sagr = string(arg);
	char tmpbuf[MAX_INPUT_LENGTH];
	string tmpstr;
	MakeRecept *trec = OLC_MREC(d);
	int i;
	switch (OLC_MODE(d))
	{
	case MREDIT_MAIN_MENU:
		// Ввод главного меню.
		if (sagr == "1")
		{
			send_to_char("Введите VNUM изготавливаемого предмета : ", d->character.get());
			OLC_MODE(d) = MREDIT_OBJ_PROTO;
			return;
		}

		if (sagr == "2")
		{
			// Выводить список умений ... или давать вводить ручками.
			tmpstr = "\r\nСписок доступных умений:\r\n";
			i = 0;
			while (make_skills[i].num != 0)
			{
				sprintf(tmpbuf, "%s%d%s) %s.\r\n", grn, i + 1, nrm, make_skills[i].name);
				tmpstr += string(tmpbuf);
				i++;
			}
			tmpstr += "Введите номер умения : ";
			send_to_char(tmpstr.c_str(), d->character.get());
			OLC_MODE(d) = MREDIT_SKILL;
			return;
		}

		if (sagr == "3")
		{
			send_to_char("Блокировать рецепт? (y/n): ", d->character.get());
			OLC_MODE(d) = MREDIT_LOCK;
			return;
		}

		for (i = 0; i < MAX_PARTS; i++)
		{
			if (atoi(sagr.c_str()) - 4 == i)
			{
				OLC_NUM(d) = i;
				mredit_disp_ingr_menu(d);
				return;
			}
		}

		if (sagr == "d")
		{
			send_to_char("Удалить рецепт? (y/n):", d->character.get());
			OLC_MODE(d) = MREDIT_DEL;
			return;
		}

		if (sagr == "s")
		{
			// Сохраняем рецепты в файл
			make_recepts.save();
			send_to_char("Рецепты сохранены.\r\n", d->character.get());
			mredit_disp_menu(d);
			OLC_VAL(d) = 0;
			return;
		}

		if (sagr == "q")
		{
			// Проверяем не производилось ли изменение
			if (OLC_VAL(d))
			{
				send_to_char("Вы желаете сохранить изменения в рецепте? (y/n) : ", d->character.get());
				OLC_MODE(d) = MREDIT_CONFIRM_SAVE;
				return;
			}
			else
			{
				// Загружаем рецепты из файла
				// Это восстановит текущее состояние дел.
				make_recepts.load();
				// Очищаем структуры OLC выходим в нормальный режим работы
				cleanup_olc(d, CLEANUP_ALL);
				return;
			}
		}

		send_to_char("Неверный ввод.\r\n", d->character.get());
		mredit_disp_menu(d);
		break;

	case MREDIT_OBJ_PROTO:
		i = atoi(sagr.c_str());
		if (real_object(i) < 0)
		{
			send_to_char("Прототип выбранного вами объекта не существует.\r\n", d->character.get());
		}
		else
		{
			trec->obj_proto = i;
			OLC_VAL(d) = 1;
		}
		mredit_disp_menu(d);
		break;

	case MREDIT_SKILL:
		int skill_num;
		skill_num = atoi(sagr.c_str());
		i = 0;
		while (make_skills[i].num != 0)
		{
			if (skill_num == i + 1)
			{
				trec->skill = make_skills[i].num;
				OLC_VAL(d) = 1;
				mredit_disp_menu(d);
				return;
			}
			i++;
		}
		send_to_char("Выбрано некорректное умение.\r\n", d->character.get());
		mredit_disp_menu(d);
		break;

	case MREDIT_DEL:
		if (sagr == "Y" || sagr == "y")
		{
			send_to_char("Рецепт удален. Рецепты сохранены.\r\n", d->character.get());
			make_recepts.del(trec);
			make_recepts.save();
			make_recepts.load();
			// Очищаем структуры OLC выходим в нормальный режим работы
			cleanup_olc(d, CLEANUP_ALL);
			return;
		}
		else if (sagr == "N" || sagr == "n")
		{
			send_to_char("Рецепт не удален.\r\n", d->character.get());
		}
		else
		{
			send_to_char("Неверный ввод.\r\n", d->character.get());
		}
		mredit_disp_menu(d);
		break;

	case MREDIT_LOCK:
		if (sagr == "Y" || sagr == "y")
		{
			send_to_char("Рецепт заблокирован от использования.\r\n", d->character.get());
			trec->locked = true;
			OLC_VAL(d) = 1;
		}
		else if (sagr == "N" || sagr == "n")
		{
			send_to_char("Рецепт разблокирован и может использоваться.\r\n", d->character.get());
			trec->locked = false;
			OLC_VAL(d) = 1;
		}
		else
		{
			send_to_char("Неверный ввод.\r\n", d->character.get());
		}
		mredit_disp_menu(d);
		break;

	case MREDIT_INGR_MENU:
		// Ввод меню ингридиентов.
		if (sagr == "1")
		{
			send_to_char("Введите VNUM ингредиента : ", d->character.get());
			OLC_MODE(d) = MREDIT_INGR_PROTO;
			return;
		}

		if (sagr == "2")
		{
			send_to_char("Введите мин.вес ингредиента : ", d->character.get());
			OLC_MODE(d) = MREDIT_INGR_WEIGHT;
			return;
		}

		if (sagr == "3")
		{
			send_to_char("Введите мин.силу ингредиента : ", d->character.get());
			OLC_MODE(d) = MREDIT_INGR_POWER;
			return;
		}

		if (sagr == "q")
		{
			mredit_disp_menu(d);
			return;
		}

		send_to_char("Неверный ввод.\r\n", d->character.get());
		mredit_disp_ingr_menu(d);
		break;

	case MREDIT_INGR_PROTO:
		i = atoi(sagr.c_str());
		if (i == 0)
		{
			if (trec->parts[OLC_NUM(d)].proto != i)
				OLC_VAL(d) = 1;
			trec->parts[OLC_NUM(d)].proto = 0;
			trec->parts[OLC_NUM(d)].min_weight = 0;
			trec->parts[OLC_NUM(d)].min_power = 0;
		}
		else if (real_object(i) < 0)
		{
			send_to_char("Прототип выбранного вами ингредиента не существует.\r\n", d->character.get());
		}
		else
		{
			trec->parts[OLC_NUM(d)].proto = i;
			OLC_VAL(d) = 1;
		}
		mredit_disp_ingr_menu(d);
		break;

	case MREDIT_INGR_WEIGHT:
		i = atoi(sagr.c_str());
		trec->parts[OLC_NUM(d)].min_weight = i;
		OLC_VAL(d) = 1;
		mredit_disp_ingr_menu(d);
		break;

	case MREDIT_INGR_POWER:
		i = atoi(sagr.c_str());
		trec->parts[OLC_NUM(d)].min_power = i;
		OLC_VAL(d) = 1;
		mredit_disp_ingr_menu(d);
		break;

	case MREDIT_CONFIRM_SAVE:
		if (sagr == "Y" || sagr == "y")
		{
			send_to_char("Рецепты сохранены.\r\n", d->character.get());
			make_recepts.save();
			make_recepts.load();
			// Очищаем структуры OLC выходим в нормальный режим работы
			cleanup_olc(d, CLEANUP_ALL);
			return;
		}
		else if (sagr == "N" || sagr == "n")
		{
			send_to_char("Рецепт не был сохранен.\r\n", d->character.get());
			cleanup_olc(d, CLEANUP_ALL);
			return;
		}
		else
		{
			send_to_char("Неверный ввод.\r\n", d->character.get());
		}
		mredit_disp_menu(d);
		break;
	}
}

// Входим в режим редактирования рецептов для предметов.
void do_edit_make(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	string tmpstr;
	DESCRIPTOR_DATA *d;
	char tmpbuf[MAX_INPUT_LENGTH];
	MakeRecept *trec;

	// Проверяем не правит ли кто-то рецепты для исключения конфликтов
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->olc && STATE(d) == CON_MREDIT)
		{
			sprintf(tmpbuf, "Рецепты в настоящий момент редактируются %s.\r\n", GET_PAD(d->character, 4));
			send_to_char(tmpbuf, ch);
			return;
		}
	}

	argument = one_argument(argument, tmpbuf);
	if (!*tmpbuf)
	{
		// Номер не задан создаем новый рецепт.
		trec = new(MakeRecept);
		// Запихиваем рецепт но помним что он заблокирован.
		make_recepts.add(trec);
		ch->desc->olc = new olc_data;
		// входим в состояние правки рецепта.
		STATE(ch->desc) = CON_MREDIT;
		OLC_MREC(ch->desc) = trec;
		OLC_VAL(ch->desc) = 0;
		mredit_disp_menu(ch->desc);
		return;
	}

	size_t i = atoi(tmpbuf);
	if ((i > make_recepts.size()) || (i <= 0))
	{
		send_to_char("Выбранного рецепта не существует.", ch);
		return;
	}

	i -= 1;
	ch->desc->olc = new olc_data;
	STATE(ch->desc) = CON_MREDIT;
	OLC_MREC(ch->desc) = make_recepts[i];
	mredit_disp_menu(ch->desc);
	return;
}

// Отображение меню параметров ингридиента.
void mredit_disp_ingr_menu(DESCRIPTOR_DATA * d)
{
	// Рисуем меню ...
	MakeRecept *trec;
	string objname, ingrname, tmpstr;
	char tmpbuf[MAX_INPUT_LENGTH];
	int index = OLC_NUM(d);
	trec = OLC_MREC(d);
	get_char_cols(d->character.get());
	auto tobj = get_object_prototype(trec->obj_proto);
	if (trec->obj_proto && tobj)
	{
		objname = tobj->get_PName(0);
	}
	else
	{
		objname = "Нет";
	}
	tobj = get_object_prototype(trec->parts[index].proto);
	if (trec->parts[index].proto && tobj)
	{
		ingrname = tobj->get_PName(0);
	}
	else
	{
		ingrname = "Нет";
	}
	sprintf(tmpbuf,
#if defined(CLEAR_SCREEN)
			"[H[J"
#endif
			"\r\n\r\n"
			"-- Рецепт - %s%s%s (%d) \r\n"
			"%s1%s) Ингредиент : %s%s (%d)\r\n"
			"%s2%s) Мин. вес   : %s%d\r\n"
			"%s3%s) Мин. сила  : %s%d\r\n",
			grn, objname.c_str(), nrm, trec->obj_proto,
			grn, nrm, yel, ingrname.c_str(), trec->parts[index].proto,
			grn, nrm, yel, trec->parts[index].min_weight, grn, nrm, yel, trec->parts[index].min_power);
	tmpstr = string(tmpbuf);
	tmpstr += string(grn) + "q" + string(nrm) + ") Выход\r\n";
	tmpstr += "Ваш выбор : ";
	send_to_char(tmpstr.c_str(), d->character.get());
	OLC_MODE(d) = MREDIT_INGR_MENU;
}

// Отображение главного меню.
void mredit_disp_menu(DESCRIPTOR_DATA * d)
{
	// Рисуем меню ...
	MakeRecept *trec;
	char tmpbuf[MAX_INPUT_LENGTH];
	string tmpstr, objname, skillname;
	trec = OLC_MREC(d);
	get_char_cols(d->character.get());
	auto tobj = get_object_prototype(trec->obj_proto);
	if (trec->obj_proto && tobj)
	{
		objname = tobj->get_PName(0);
	}
	else
	{
		objname = "Нет";
	}
	int i = 0;
	//
	skillname = "Нет";
	while (make_skills[i].num != 0)
	{
		if (make_skills[i].num == trec->skill)
		{
			skillname = string(make_skills[i].name);
			break;
		}
		i++;
	}
	sprintf(tmpbuf,
#if defined(CLEAR_SCREEN)
			"[H[J"
#endif
			"\r\n\r\n"
			"-- Рецепт --\r\n"
			"%s1%s) Предмет    : %s%s (%d)\r\n"
			"%s2%s) Умение     : %s%s (%d)\r\n"
			"%s3%s) Блокирован : %s%s \r\n",
			grn, nrm, yel, objname.c_str(), trec->obj_proto,
			grn, nrm, yel, skillname.c_str(), trec->skill, grn, nrm, yel, (trec->locked ? "Да" : "Нет"));
	tmpstr = string(tmpbuf);
	for (int i = 0; i < MAX_PARTS; i++)
	{
		tobj = get_object_prototype(trec->parts[i].proto);
		if (trec->parts[i].proto && tobj)
		{
			objname = tobj->get_PName(0);
		}
		else
		{
			objname = "Нет";
		}
		sprintf(tmpbuf, "%s%d%s) Компонент %d: %s%s (%d)\r\n",
			grn, i + 4, nrm, i + 1, yel, objname.c_str(), trec->parts[i].proto);
		tmpstr += string(tmpbuf);
	};
	tmpstr += string(grn) + "d" + string(nrm) + ") Удалить\r\n";
	tmpstr += string(grn) + "s" + string(nrm) + ") Сохранить\r\n";
	tmpstr += string(grn) + "q" + string(nrm) + ") Выход\r\n";
	tmpstr += "Ваш выбор : ";
	send_to_char(tmpstr.c_str(), d->character.get());
	OLC_MODE(d) = MREDIT_MAIN_MENU;
}

void do_list_make(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	string tmpstr, skill_name, obj_name;
	char tmpbuf[MAX_INPUT_LENGTH];
	MakeRecept *trec;
	if (make_recepts.size() == 0)
	{
		send_to_char("Рецепты в этом мире не определены.", ch);
		return;
	}
	// Выдаем список рецептов всех рецептов как в магазине.
	tmpstr = "###  Б  Умение  Предмет             Составляющие                         \r\n";
	tmpstr += "------------------------------------------------------------------------------\r\n";
	for (size_t i = 0; i < make_recepts.size(); i++)
	{
		int j = 0;
		skill_name = "Нет";
		obj_name = "Нет";
		trec = make_recepts[i];
		auto obj = get_object_prototype(trec->obj_proto);
		if (obj)
		{
			obj_name = obj->get_PName(0).substr(0, 11);
		}
		while (make_skills[j].num != 0)
		{
			if (make_skills[j].num == trec->skill)
			{
				skill_name = string(make_skills[j].short_name);
				break;
			}
			j++;
		}
		sprintf(tmpbuf, "%3zd  %-1s  %-6s  %-12s(%5d) :",
			i + 1, (trec->locked ? "*" : " "), skill_name.c_str(), obj_name.c_str(), trec->obj_proto);
		tmpstr += string(tmpbuf);
		for (int j = 0; j < MAX_PARTS; j++)
		{
			if (trec->parts[j].proto != 0)
			{
				obj = get_object_prototype(trec->parts[j].proto);
				if (obj)
				{
					obj_name = obj->get_PName(0).substr(0, 11);
				}
				else
				{
					obj_name = "Нет";
				}
				sprintf(tmpbuf, " %-12s(%5d)", obj_name.c_str(), trec->parts[j].proto);
				if (j > 0)
				{
					tmpstr += ",";
					if (j % 2 == 0)
					{
						// разбиваем строчки если ингров больше 2;
						tmpstr += "\r\n";
						tmpstr += "                                    :";
					}
				};
				tmpstr += string(tmpbuf);
			}
			else
			{
				break;
			}
		}
		tmpstr += "\r\n";
	}
	page_string(ch->desc, tmpstr);
	return;
}
// Создание любого предмета из компонента.
void do_make_item(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	// Тут творим предмет.
	// Если прислали без параметра то выводим список всех рецептов
	// доступных для изготовления персонажу из его ингров
	// Мастерить можно лук, посох , и диковину(аналог артефакта)
	// суб команда make
	// Выковать можно клинок и доспех (щит) умения разные. название одно
	// Сварить отвар
	// Сшить одежду 
	if ((subcmd == MAKE_WEAR) && (!ch->get_skill(SKILL_MAKE_WEAR))) {
		send_to_char("Вас этому никто не научил.\r\n", ch);
		return;
	}
	string tmpstr;
	MakeReceptList *canlist;
	MakeRecept *trec;
	char tmpbuf[MAX_INPUT_LENGTH];
	//int used_skill = subcmd;
	argument = one_argument(argument, tmpbuf);
	canlist = new MakeReceptList;
	// Разбираем в зависимости от того что набрали ... список объектов
	switch (subcmd)
	{
	case(MAKE_POTION):
		// Варим отвар.
		tmpstr = "Вы можете сварить:\r\n";
		make_recepts.can_make(ch, canlist, SKILL_MAKE_POTION);
		break;
	case(MAKE_WEAR):
		// Шьем одежку.
		tmpstr = "Вы можете сшить:\r\n";
		make_recepts.can_make(ch, canlist, SKILL_MAKE_WEAR);
		break;
	case(MAKE_METALL):
		tmpstr = "Вы можете выковать:\r\n";
		make_recepts.can_make(ch, canlist, SKILL_MAKE_WEAPON);
		make_recepts.can_make(ch, canlist, SKILL_MAKE_ARMOR);
		break;
	case(MAKE_CRAFT):
		tmpstr = "Вы можете смастерить:\r\n";
		make_recepts.can_make(ch, canlist, SKILL_MAKE_STAFF);
		make_recepts.can_make(ch, canlist, SKILL_MAKE_BOW);
		make_recepts.can_make(ch, canlist, SKILL_MAKE_JEWEL);
		make_recepts.can_make(ch, canlist, SKILL_MAKE_AMULET);
		break;
	}
	if (canlist->size() == 0)
	{
		// Чар не может сделать ничего.
		send_to_char("Вы ничего не можете сделать.\r\n", ch);
		return;
	}
	if (!*tmpbuf)
	{
		// Выводим тут список предметов которые можем сделать.
		for (size_t i = 0; i < canlist->size(); i++)
		{
			auto tobj = get_object_prototype((*canlist)[i]->obj_proto);
			if (!tobj)
				return;
			sprintf(tmpbuf, "%zd) %s\r\n", i + 1, tobj->get_PName(0).c_str());
			tmpstr += string(tmpbuf);
		};
		send_to_char(tmpstr.c_str(), ch);
		return;
	}
	// Адресуемся по списку либо по номеру, либо по названию с номером.
	tmpstr = string(tmpbuf);
	size_t i = atoi(tmpbuf);
	if ((i > 0) && (i <= canlist->size())
			&& (tmpstr.find(".") > tmpstr.size()))
	{
		trec = (*canlist)[i - 1];
	}
	else
	{
		trec = canlist->get_by_name(tmpstr);
		if (trec == NULL)
		{
			tmpstr = "Похоже у вас творческий кризис.\r\n";
			send_to_char(tmpstr.c_str(), ch);
			delete canlist;
			return;
		}
	};
	trec->make(ch);
	delete canlist;
	return;
}
void go_create_weapon(CHAR_DATA * ch, OBJ_DATA * obj, int obj_type, ESkill skill)
{
	char txtbuff[100];
	const char *to_char = NULL, *to_room = NULL;
	int prob, percent, ndice, sdice, weight;
	float average;
	if (obj_type == 5 || obj_type == 6)
	{
		weight = number(created_item[obj_type].min_weight, created_item[obj_type].max_weight);
		percent = 100;
		prob = 100;
	}
	else
	{
		if (!obj)
		{
			return;
		}
		skill = created_item[obj_type].skill;
		percent = number(1, skill_info[skill].max_percent);
		prob = train_skill(ch, skill, skill_info[skill].max_percent, 0);
		weight = MIN(GET_OBJ_WEIGHT(obj) - 2, GET_OBJ_WEIGHT(obj) * prob / percent);
	}
	if (weight < created_item[obj_type].min_weight)
	{
		send_to_char("У вас не хватило материала.\r\n", ch);
	}
	else if (prob * 5 < percent)
	{
		send_to_char("У вас ничего не получилось.\r\n", ch);
	}
	else
	{
		const auto tobj = world_objects.create_from_prototype_by_vnum(created_item[obj_type].obj_vnum);
		if (!tobj)
		{
			send_to_char("Образец был невозвратимо утерян.\r\n", ch);
		}
		else
		{
			tobj->set_weight(MIN(weight, created_item[obj_type].max_weight));
			tobj->set_cost(2 * GET_OBJ_COST(obj) / 3);
			tobj->set_owner(GET_UNIQUE(ch));
			tobj->set_extra_flag(EExtraFlag::ITEM_TRANSFORMED);
			// ковка объектов со слотами.
			// для 5+ мортов имеем шанс сковать стаф с 3 слотами: базовый 2% и по 0.5% за морт
			// для 2 слотов базовый шанс 5%, 1% за каждый морт
			// для 1 слота базово 20% и 4% за каждый морт
			// Карачун. Поправлено. Расчет не через морты а через скил.
			if (skill == SKILL_TRANSFORMWEAPON)
			{
				if (ch->get_skill(skill) >= 105 && number(1, 100) <= 2 + (ch->get_skill(skill) - 105) / 10)
				{
					tobj->set_extra_flag(EExtraFlag::ITEM_WITH3SLOTS);
				}
				else if (number(1, 100) <= 5 + MAX((ch->get_skill(skill) - 80), 0) / 5)
				{
					tobj->set_extra_flag(EExtraFlag::ITEM_WITH2SLOTS);
				}
				else if (number(1, 100) <= 20 + MAX((ch->get_skill(skill) - 80), 0) / 5 * 4)
				{
					tobj->set_extra_flag(EExtraFlag::ITEM_WITH1SLOT);
				}
			}
			switch (obj_type)
			{
			case 0:	// smith weapon
			case 1:
			case 2:
			case 3:
			case 4:
			case 11:
				{
					// Карачун. Таймер должен зависить от таймера прототипа.
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// В минимуме один день реала, в максимуме таймер из прототипа
					const int timer_value = tobj->get_timer() / 100 * ch->get_skill(skill) - number(0, tobj->get_timer() / 100 * 25);
					const int timer = MAX(OBJ_DATA::ONE_DAY, timer_value);
					tobj->set_timer(timer);
					sprintf(buf, "Ваше изделие продержится примерно %d дней\n", tobj->get_timer() / 24 / 60);
					act(buf, FALSE, ch, tobj.get(), 0, TO_CHAR);
					tobj->set_material(GET_OBJ_MATER(obj));
					// Карачун. Так логичнее.
					// было GET_OBJ_MAX(tobj) = MAX(50, MIN(300, 300 * prob / percent));
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// при расчете числа умножены на 100, перед приравниванием делятся на 100. Для не потерять десятые.
					tobj->set_maximum_durability(MAX(20000, 35000 / 100 * ch->get_skill(skill) - number(0, 35000 / 100 * 25)) / 100);
					tobj->set_current_durability(GET_OBJ_MAX(tobj));
					percent = number(1, skill_info[skill].max_percent);
					prob = calculate_skill(ch, skill, 0);
					ndice = MAX(2, MIN(4, prob / percent));
					ndice += GET_OBJ_WEIGHT(tobj) / 10;
					percent = number(1, skill_info[skill].max_percent);
					prob = calculate_skill(ch, skill, 0);
					sdice = MAX(2, MIN(5, prob / percent));
					sdice += GET_OBJ_WEIGHT(tobj) / 10;
					tobj->set_val(1, ndice);
					tobj->set_val(2, sdice);
					//			tobj->set_wear_flags(created_item[obj_type].wear); пусть wear флаги будут из прототипа
					if (skill != SKILL_CREATEBOW)
					{
						if (GET_OBJ_WEIGHT(tobj) < 14 && percent * 4 > prob)
						{
							tobj->set_wear_flag(EWearFlag::ITEM_WEAR_HOLD);
						}
						to_room = "$n выковал$g $o3.";
						average = (((float)sdice + 1) * (float)ndice / 2.0);
						if (average < 3.0)
						{
							sprintf(txtbuff, "Вы выковали $o3 %s.", create_weapon_quality[(int)(2.5 * 2)]);
						}
						else if (average <= 27.5)
						{
							sprintf(txtbuff, "Вы выковали $o3 %s.", create_weapon_quality[(int)(average * 2)]);
						}
						else
						{
							sprintf(txtbuff, "Вы выковали $o3 %s!", create_weapon_quality[56]);
						}
						to_char = (char *)txtbuff;
					}
					else
					{
						to_room = "$n смастерил$g $o3.";
						to_char = "Вы смастерили $o3.";
					}
					break;
				}
			case 5:	// mages weapon
			case 6:
				tobj->set_timer(OBJ_DATA::ONE_DAY);
				tobj->set_maximum_durability(50);
				tobj->set_current_durability(50);
				ndice = MAX(2, MIN(4, GET_LEVEL(ch) / number(6, 8)));
				ndice += (GET_OBJ_WEIGHT(tobj) / 10);
				sdice = MAX(2, MIN(5, GET_LEVEL(ch) / number(4, 5)));
				sdice += (GET_OBJ_WEIGHT(tobj) / 10);
				tobj->set_val(1, ndice);
				tobj->set_val(2, sdice);
				tobj->set_extra_flag(EExtraFlag::ITEM_NORENT);
				tobj->set_extra_flag(EExtraFlag::ITEM_DECAY);
				tobj->set_extra_flag(EExtraFlag::ITEM_NOSELL);
				tobj->set_wear_flags(created_item[obj_type].wear);
				to_room = "$n создал$g $o3.";
				to_char = "Вы создали $o3.";
				break;
			case 7:	// smith armor
			case 8:
			case 9:
			case 10:
				{
					// Карачун. Таймер должен зависить от таймера прототипа.
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// В минимуме один день реала, в максимуме таймер из прототипа
					const int timer_value = tobj->get_timer() / 100 * ch->get_skill(skill) - number(0, tobj->get_timer() / 100 * 25);
					const int timer = MAX(OBJ_DATA::ONE_DAY, timer_value);
					tobj->set_timer(timer);
					sprintf(buf, "Ваше изделие продержится примерно %d дней\n", tobj->get_timer() / 24 / 60);
					act(buf, FALSE, ch, tobj.get(), 0, TO_CHAR);
					tobj->set_material(GET_OBJ_MATER(obj));
					// Карачун. Так логичнее.
					// было GET_OBJ_MAX(tobj) = MAX(50, MIN(300, 300 * prob / percent));
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// при расчете числа умножены на 100, перед приравниванием делятся на 100. Для не потерять десятые.
					tobj->set_maximum_durability(MAX(20000, 10000 / 100 * ch->get_skill(skill) - number(0, 15000 / 100 * 25)) / 100);
					tobj->set_current_durability(GET_OBJ_MAX(tobj));
					percent = number(1, skill_info[skill].max_percent);
					prob = calculate_skill(ch, skill, 0);
					ndice = MAX(2, MIN((105 - material_value[GET_OBJ_MATER(tobj)]) / 10, prob / percent));
					percent = number(1, skill_info[skill].max_percent);
					prob = calculate_skill(ch, skill, 0);
					sdice = MAX(1, MIN((105 - material_value[GET_OBJ_MATER(tobj)]) / 15, prob / percent));
					tobj->set_val(0, ndice);
					tobj->set_val(1, sdice);
					tobj->set_wear_flags(created_item[obj_type].wear);
					to_room = "$n выковал$g $o3.";
					to_char = "Вы выковали $o3.";
					break;
				}
			} // switch

			if (to_char)
			{
				act(to_char, FALSE, ch, tobj.get(), 0, TO_CHAR);
			}

			if (to_room)
			{
				act(to_room, FALSE, ch, tobj.get(), 0, TO_ROOM);
			}

			if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			{
				send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
				obj_to_room(tobj.get(), ch->in_room);
				obj_decay(tobj.get());
			}
			else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch))
			{
				send_to_char("Вы не сможете унести такой вес.\r\n", ch);
				obj_to_room(tobj.get(), ch->in_room);
				obj_decay(tobj.get());
			}
			else
			{
				obj_to_char(tobj.get(), ch);
			}
		}
	}

	if (obj)
	{
		obj_from_char(obj);
		extract_obj(obj);
	}
}
void do_transform_weapon(CHAR_DATA* ch, char *argument, int/* cmd*/, int subcmd)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *obj = NULL, *coal, *proto[MAX_PROTO];
	int obj_type, i, found, rnum;

	if (IS_NPC(ch)
		|| !ch->get_skill(static_cast<ESkill>(subcmd)))
	{
		send_to_char("Вас этому никто не научил.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	if (!*arg1)
	{
		switch (subcmd)
		{
		case SKILL_TRANSFORMWEAPON:
			send_to_char("Во что вы хотите перековать?\r\n", ch);
			break;
		case SKILL_CREATEBOW:
			send_to_char("Что вы хотите смастерить?\r\n", ch);
			break;
		}
		return;
	}
	obj_type = search_block(arg1, create_item_name, FALSE);
	if (-1 == obj_type)
	{
		switch (subcmd)
		{
		case SKILL_TRANSFORMWEAPON:
			send_to_char("Перековать можно в :\r\n", ch);
			break;
		case SKILL_CREATEBOW:
			send_to_char("Смастерить можно :\r\n", ch);
			break;
		}
		for (obj_type = 0; *create_item_name[obj_type] != '\n'; obj_type++)
		{
			if (created_item[obj_type].skill == subcmd)
			{
				sprintf(buf, "- %s\r\n", create_item_name[obj_type]);
				send_to_char(buf, ch);
			}
		}
		return;
	}
	if (created_item[obj_type].skill != subcmd)
	{
		switch (subcmd)
		{
		case SKILL_TRANSFORMWEAPON:
			send_to_char("Данный предмет выковать нельзя.\r\n", ch);
			break;
		case SKILL_CREATEBOW:
			send_to_char("Данный предмет смастерить нельзя.\r\n", ch);
			break;
		}
		return;
	}
	for (i = 0; i < MAX_PROTO; proto[i++] = NULL);
	argument = one_argument(argument, arg2);
	if (!*arg2)
	{
		send_to_char("Вам нечего перековывать.\r\n", ch);
		return;
	}
	if (!(obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
	{
		sprintf(buf, "У Вас нет '%s'.\r\n", arg2);
		send_to_char(buf, ch);
		return;
	}
	if (obj->get_contains())
	{
		act("В $o5 что-то лежит.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_INGREDIENT)
	{
		for (i = 0, found = FALSE; i < MAX_PROTO; i++)
		{
			if (GET_OBJ_VAL(obj, 1) == created_item[obj_type].proto[i])
			{
				if (proto[i])
				{
					found = TRUE;
				}
				else
				{
					proto[i] = obj;
					found = FALSE;
					break;
				}
			}
		}
		if (i >= MAX_PROTO)
		{
			if (found)
			{
				act("Похоже, вы уже что-то используете вместо $o1.", FALSE, ch, obj, 0, TO_CHAR);
			}
			else
			{
				act("Похоже, $o не подойдет для этого.", FALSE, ch, obj, 0, TO_CHAR);
			}
			return;
		}
	}
	else
	{
		if (created_item[obj_type].material_bits
			&& !IS_SET(created_item[obj_type].material_bits, (1 << GET_OBJ_MATER(obj))))
		{
			act("$o сделан$G из неподходящего материала.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
	}
	switch (subcmd)
	{
	case SKILL_TRANSFORMWEAPON:
		// Проверяем повторно из чего сделан объект
		// Чтобы не было абъюза с перековкой из угля.
		if (created_item[obj_type].material_bits &&
				!IS_SET(created_item[obj_type].material_bits, (1 << GET_OBJ_MATER(obj))))
		{
			act("$o сделан$G из неподходящего материала.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
		if (!IS_IMMORTAL(ch))
		{
			if (!ROOM_FLAGGED(ch->in_room, ROOM_SMITH))
			{
				send_to_char("Вам нужно попасть в кузницу для этого.\r\n", ch);
				return;
			}
			for (coal = ch->carrying; coal; coal = coal->get_next_content())
			{
				if (GET_OBJ_TYPE(coal) == OBJ_DATA::ITEM_INGREDIENT)
				{
					for (i = 0; i < MAX_PROTO; i++)
					{
						if (proto[i] == coal)
						{
							break;
						}
						else if (!proto[i]
							&& GET_OBJ_VAL(coal, 1) == created_item[obj_type].proto[i])
						{
							proto[i] = coal;
							break;
						}
					}
				}
			}
			for (i = 0, found = TRUE; i < MAX_PROTO; i++)
			{
				if (created_item[obj_type].proto[i]
					&& !proto[i])
				{
					rnum = real_object(created_item[obj_type].proto[i]);
					if (rnum < 0)
					{
						act("У вас нет необходимого ингредиента.", FALSE, ch, 0, 0, TO_CHAR);
					}
					else
					{
						const OBJ_DATA obj(*obj_proto[rnum]);
						act("У вас не хватает $o1 для этого.", FALSE, ch, &obj, 0, TO_CHAR);
					}
					found = FALSE;
				}
			}
			if (!found)
			{
				return;
			}
		}
		for (i = 0; i < MAX_PROTO; i++)
		{
			if (proto[i] && proto[i] != obj)
			{
				obj->set_cost(GET_OBJ_COST(obj) + GET_OBJ_COST(proto[i]));
				extract_obj(proto[i]);
			}
		}
		go_create_weapon(ch, obj, obj_type, SKILL_TRANSFORMWEAPON);
		break;
	case SKILL_CREATEBOW:
		for (coal = ch->carrying; coal; coal = coal->get_next_content())
		{
			if (GET_OBJ_TYPE(coal) == OBJ_DATA::ITEM_INGREDIENT)
			{
				for (i = 0; i < MAX_PROTO; i++)
				{
					if (proto[i] == coal)
					{
						break;
					}
					else if (!proto[i]
						&& GET_OBJ_VAL(coal, 1) == created_item[obj_type].proto[i])
					{
						proto[i] = coal;
						break;
					}
				}
			}
		}
		for (i = 0, found = TRUE; i < MAX_PROTO; i++)
		{
			if (created_item[obj_type].proto[i]
				&& !proto[i])
			{
				rnum = real_object(created_item[obj_type].proto[i]);
				if (rnum < 0)
				{
					act("У вас нет необходимого ингредиента.", FALSE, ch, 0, 0, TO_CHAR);
				}
				else
				{
					const OBJ_DATA obj(*obj_proto[rnum]);
					act("У вас не хватает $o1 для этого.", FALSE, ch, &obj, 0, TO_CHAR);
				}
				found = FALSE;
			}
		}
		if (!found)
		{
			return;
		}
		for (i = 1; i < MAX_PROTO; i++)
		{
			if (proto[i])
			{
				proto[0]->add_weight(GET_OBJ_WEIGHT(proto[i]));
				proto[0]->set_cost(GET_OBJ_COST(proto[0]) + GET_OBJ_COST(proto[i]));
				extract_obj(proto[i]);
			}
		}
		go_create_weapon(ch, proto[0], obj_type, SKILL_CREATEBOW);
		break;
	}
}
// *****************************
MakeReceptList::MakeReceptList()
{
	// Инициализация класса листа .
}
MakeReceptList::~MakeReceptList()
{
	// Разрушение списка.
	clear();
}
int
MakeReceptList::load()
{
	// Читаем тут файл с рецептом.
	// НАДО ДОБАВИТЬ СОРТИРОВКУ !!!
	char tmpbuf[MAX_INPUT_LENGTH];
	string tmpstr;
	// чистим список рецептов от старых данных.
	clear();
	MakeRecept *trec;
	ifstream bifs(LIB_MISC "makeitems.lst");
	if (!bifs)
	{
		sprintf(tmpbuf, "MakeReceptList:: Unable open input file !!!");
		mudlog(tmpbuf, LGH, LVL_IMMORT, SYSLOG, TRUE);
		return (FALSE);
	}
	while (!bifs.eof())
	{
		bifs.getline(tmpbuf, MAX_INPUT_LENGTH, '\n');
		tmpstr = string(tmpbuf);
		// пропускаем закомменированные строчки.
		if (tmpstr.substr(0, 2) == "//")
			continue;
//    cout << "Get str from file :" << tmpstr << endl;
		if (tmpstr.size() == 0)
			continue;
		trec = new(MakeRecept);
		if (trec->load_from_str(tmpstr))
		{
			recepts.push_back(trec);
		}
		else
		{
			delete trec;
			// ошибка вытаскивания из строки написать
			sprintf(tmpbuf, "MakeReceptList:: Fail get recept from line.");
			mudlog(tmpbuf, LGH, LVL_IMMORT, SYSLOG, TRUE);
		}
	}
	return TRUE;
}
int MakeReceptList::save()
{
	// Пишем тут файл с рецептом.
	// Очищаем список
	string tmpstr;
	char tmpbuf[MAX_INPUT_LENGTH];
	list < MakeRecept * >::iterator p;
	ofstream bofs(LIB_MISC "makeitems.lst");
	if (!bofs)
	{
		// cout << "Unable input stream to create !!!" << endl;
		sprintf(tmpbuf, "MakeReceptList:: Unable create output file !!!");
		mudlog(tmpbuf, LGH, LVL_IMMORT, SYSLOG, TRUE);
		return (FALSE);
	}
	sort();
	p = recepts.begin();
	while (p != recepts.end())
	{
		if ((*p)->save_to_str(tmpstr))
			bofs << tmpstr << endl;
		p++;
	}
	bofs.close();
	return 0;
}
void MakeReceptList::add(MakeRecept * recept)
{
	recepts.push_back(recept);
	return;
}
void MakeReceptList::del(MakeRecept * recept)
{
	recepts.remove(recept);
	return;
}
bool by_skill(MakeRecept * const &lhs, MakeRecept * const &rhs)
{
	return ((lhs->skill) > (rhs->skill));
}
void MakeReceptList::sort()
{
	// Сделать сортировку по умениям.
	recepts.sort(by_skill);
	return;
}
size_t MakeReceptList::size()
{
	return recepts.size();
}
void MakeReceptList::clear()
{
	// Очищаем список
	list < MakeRecept * >::iterator p;
	p = recepts.begin();
	while (p != recepts.end())
	{
		delete(*p);
		p++;
	}
	recepts.erase(recepts.begin(), recepts.end());
	return;
}
MakeRecept *MakeReceptList::operator[](size_t i)
{
	list < MakeRecept * >::iterator p = recepts.begin();
	size_t j = 0;
	while (p != recepts.end())
	{
		if (i == j)
		{
			return (*p);
		}
		j++;
		p++;
	}
	return (NULL);
}
MakeRecept *MakeReceptList::get_by_name(string & rname)
{
	// Ищем по списку рецепт с таким именем.
	// Ищем по списку рецепты которые чар может изготовить.
	list<MakeRecept*>::iterator p = recepts.begin();
	int k = 1;	// count
	// split string by '.' character and convert first part into number (store to k)
	size_t i = rname.find(".");
	if (std::string::npos != i)	// TODO: Check me.
	{
		// Строка не найдена.
		if (0 < i)
		{
			k = atoi(rname.substr(0, i).c_str());
			if (k <= 0)
			{
				return NULL;	// Если ввели -3.xxx
			}
		}
		rname = rname.substr(i + 1);
	}
	int j = 0;
	while (p != recepts.end())
	{
		auto tobj = get_object_prototype((*p)->obj_proto);
		if (!tobj)
		{
			return 0;
		}
		std::string tmpstr = tobj->get_aliases() + " ";
		while (int(tmpstr.find(' ')) > 0)
		{
			if ((tmpstr.substr(0, tmpstr.find(' '))).find(rname) == 0)
			{
				if ((k - 1) == j)
				{
					return *p;
				}
				j++;
				break;
			}
			tmpstr = tmpstr.substr(tmpstr.find(' ') + 1);
		}
		p++;
	}
	return NULL;
}
MakeReceptList *MakeReceptList::can_make(CHAR_DATA * ch, MakeReceptList * canlist, int use_skill)
{
	// Ищем по списку рецепты которые чар может изготовить.
	list < MakeRecept * >::iterator p;
	p = recepts.begin();
	while (p != recepts.end())
	{
		if (((*p)->skill == use_skill) && ((*p)->can_make(ch)))
		{
			MakeRecept *trec = new MakeRecept;
			*trec = **p;
			canlist->add(trec);
		}
		p++;
	}
	return (canlist);
}
OBJ_DATA *get_obj_in_list_ingr(int num, OBJ_DATA * list) //Ингридиентом является или сам прототип с VNUM или альтернатива с VALUE 1 равным внум прототипа
{
    OBJ_DATA *i;
	for (i = list; i; i = i->get_next_content())
	{
		if (GET_OBJ_VNUM(i) == num)
		{
			return i;
		}
		if ((GET_OBJ_VAL(i, 1) == num)
			&& (GET_OBJ_TYPE(i) == OBJ_DATA::ITEM_INGREDIENT
				|| GET_OBJ_TYPE(i) == OBJ_DATA::ITEM_MING
				|| GET_OBJ_TYPE(i) == OBJ_DATA::ITEM_MATERIAL))
		{
			return i;
		}
	}
    return NULL;
}
MakeRecept::MakeRecept(): skill(SKILL_INVALID)
{
	locked = true;		// по умолчанию рецепт залочен.
	obj_proto = 0;
	for (int i = 0; i < MAX_PARTS; i++)
	{
		parts[i].proto = 0;
		parts[i].min_weight = 0;
		parts[i].min_power = 0;
	}
}
int MakeRecept::can_make(CHAR_DATA * ch)
{
	int i, spellnum;
	OBJ_DATA *ingrobj = NULL;
	// char tmpbuf[MAX_INPUT_LENGTH];
	// Сделать проверку на поле locked
	if (!ch)
		return (FALSE);
	if (locked)
		return (FALSE);
	// Сделать проверку наличия скилла у игрока.
	if (IS_NPC(ch) || !ch->get_skill(skill))
	{
		return (FALSE);
	}
	// Делаем проверку может ли чар сделать посох такого типа
	if (skill == SKILL_MAKE_STAFF)
	{
		auto tobj = get_object_prototype(obj_proto);
		if (!tobj)
		{
			return 0;
		}
		spellnum = GET_OBJ_VAL(tobj, 3);
//   if (!((GET_OBJ_TYPE(tobj) == ITEM_WAND )||(GET_OBJ_TYPE(tobj) == ITEM_WAND )))
		// Хотим делать посох проверяем есть ли заряжаемый закл у игрока.
		if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP | SPELL_KNOW) && !IS_IMMORTAL(ch))
		{
			if (GET_LEVEL(ch) < SpINFO.min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
				|| slot_for_char(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0)
			{
				//send_to_char("Рано еще Вам бросаться такими словами!\r\n", ch);
				return (FALSE);
			}
			else
			{
				// send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
				return (FALSE);
			}
		}
	}
	for (i = 0; i < MAX_PARTS; i++)
	{
		if (parts[i].proto == 0)
		{
			break;
		}
		if (real_object(parts[i].proto) < 0)
			return (FALSE);
//      send_to_char("Образец был невозвратимо утерян.\r\n",ch); //леший знает чего тут надо писать
		if (!(ingrobj = get_obj_in_list_ingr(parts[i].proto, ch->carrying)))
		{
//       sprintf(tmpbuf,"Для '%d' у вас нет '%d'.\r\n",obj_proto,parts[i].proto);
//       send_to_char(tmpbuf,ch);
			return (FALSE);
		}
		int ingr_lev = get_ingr_lev(ingrobj);
		// Если чар ниже уровня ингридиента то он не может делать рецепты с его
		// участием.
		if (!IS_IMPL(ch) && (ingr_lev > (GET_LEVEL(ch) + 2 * GET_REMORT(ch))))
    		{
			send_to_char("Вы слишком малого уровня и вам что-то не подходит для шитья.\r\n", ch);
			return (FALSE);
		}
	}
	return (TRUE);
}

int MakeRecept::get_ingr_lev(OBJ_DATA * ingrobj)
{
	// Получаем уровень ингредиента ...
	if (GET_OBJ_TYPE(ingrobj) == OBJ_DATA::ITEM_INGREDIENT)
	{
		// Получаем уровень игредиента до 128
		return (GET_OBJ_VAL(ingrobj, 0) >> 8);
	}
	else if (GET_OBJ_TYPE(ingrobj) == OBJ_DATA::ITEM_MING)
	{
		// У ингров типа 26 совпадает уровень и сила.
		return GET_OBJ_VAL(ingrobj, IM_POWER_SLOT);
	}
	else if (GET_OBJ_TYPE(ingrobj) == OBJ_DATA::ITEM_MATERIAL)
	{
		return GET_OBJ_VAL(ingrobj, 0);
	}
	else
	{
		return -1;
	}
}

int MakeRecept::get_ingr_pow(OBJ_DATA * ingrobj)
{
	// Получаем силу ингредиента ...
	if (GET_OBJ_TYPE(ingrobj) == OBJ_DATA::ITEM_INGREDIENT
		|| GET_OBJ_TYPE(ingrobj) == OBJ_DATA::ITEM_MATERIAL)
	{
		return GET_OBJ_VAL(ingrobj, 2);
	}
	else if (GET_OBJ_TYPE(ingrobj) == OBJ_DATA::ITEM_MING)
	{
		return GET_OBJ_VAL(ingrobj, IM_POWER_SLOT);
	}
	else
	{
		return -1;
	}
}
void MakeRecept::make_value_wear(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *ingrs[MAX_PARTS])
{
	int wearkoeff = 50;
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY)) // 1.1
	{
		wearkoeff = 110;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD)) //0.8
	{
		wearkoeff = 80;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS)) //0.5
	{
		wearkoeff = 50;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET)) //0.6
	{
		wearkoeff = 60;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS)) //0.5
	{
		wearkoeff = 50;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))//0.9
	{
		wearkoeff = 90;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))//0.45
	{
		wearkoeff = 45;
	}
	obj->set_val(0, ((GET_REAL_INT(ch) * GET_REAL_INT(ch) / 10 + ch->get_skill(SKILL_MAKE_WEAR)) / 100 + (GET_OBJ_VAL(ingrs[0], 3) + 1)) * wearkoeff / 100); //АС=((инта*инта/10+умелка)/100+левл.шкуры)*коэф.части тела
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY)) //0.9
	{
		wearkoeff = 90;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD)) //0.7
	{
		wearkoeff = 70;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS)) //0.4
	{
		wearkoeff = 40;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET)) //0.5
	{
		wearkoeff = 50;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS)) //0.4
	{
		wearkoeff = 40;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))//0.8
	{
		wearkoeff = 80;
	}
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))//0.31
	{
		wearkoeff = 31;
	}
	obj->set_val(1, (ch->get_skill(SKILL_MAKE_WEAR) / 25 + (GET_OBJ_VAL(ingrs[0], 3) + 1)) * wearkoeff / 100); //броня=(%умелки/25+левл.шкуры)*коэф.части тела
}
float MakeRecept::count_mort_requred(OBJ_DATA * obj)
{
    
	float result = 0.0;
	const float SQRT_MOD = 1.7095f;
	const int AFF_SHIELD_MOD = 30;
	const int AFF_MAGICGLASS_MOD = 10;
	const int AFF_BLINK_MOD = 10;

	result = 0.0;
	
	float total_weight = 0.0;

	// аффекты APPLY_x
	for (int k = 0; k < MAX_OBJ_AFFECT; k++)
	{
		if (obj->get_affected(k).location == 0) continue;

		// случай, если один аффект прописан в нескольких полях
		for (int kk = 0; kk < MAX_OBJ_AFFECT; kk++)
		{
			if (obj->get_affected(k).location == obj->get_affected(kk).location
				&& k != kk)
			{
				log("SYSERROR: double affect=%d, obj_vnum=%d",
					obj->get_affected(k).location, GET_OBJ_VNUM(obj));
				return 1000000;
			}
		}
		if ((obj->get_affected(k).modifier > 0)&&((obj->get_affected(k).location != APPLY_AC)&&
                        (obj->get_affected(k).location != APPLY_SAVING_WILL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_CRITICAL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_STABILITY)&&
                        (obj->get_affected(k).location != APPLY_SAVING_REFLEX)))
		{
                    float weight = count_affect_weight(obj->get_affected(k).location, obj->get_affected(k).modifier);
        	    log("SYSERROR: negative weight=%f, obj_vnum=%d",
					weight, GET_OBJ_VNUM(obj));
                    total_weight += pow(weight, SQRT_MOD);
		}
                // савесы которые с минусом должны тогда понижать вес если в +
 		else if ((obj->get_affected(k).modifier > 0)&&((obj->get_affected(k).location == APPLY_AC)||
                        (obj->get_affected(k).location == APPLY_SAVING_WILL)||
                        (obj->get_affected(k).location == APPLY_SAVING_CRITICAL)||
                        (obj->get_affected(k).location == APPLY_SAVING_STABILITY)||
                        (obj->get_affected(k).location == APPLY_SAVING_REFLEX)))
		{
                    float weight = count_affect_weight(obj->get_affected(k).location, 0-obj->get_affected(k).modifier);
                    total_weight -= pow(weight, -SQRT_MOD);
		}
               //Добавленый кусок учет савесов с - значениями
                else if ((obj->get_affected(k).modifier < 0)
                        &&((obj->get_affected(k).location == APPLY_AC)||
                        (obj->get_affected(k).location == APPLY_SAVING_WILL)||
                        (obj->get_affected(k).location == APPLY_SAVING_CRITICAL)||
                        (obj->get_affected(k).location == APPLY_SAVING_STABILITY)||
                        (obj->get_affected(k).location == APPLY_SAVING_REFLEX)))
                {
                    float weight = count_affect_weight(obj->get_affected(k).location, obj->get_affected(k).modifier);
                    total_weight += pow(weight, SQRT_MOD);
                }
               //Добавленый кусок учет отрицательного значения но не савесов
                else if ((obj->get_affected(k).modifier < 0)
                        &&((obj->get_affected(k).location != APPLY_AC)&&
                        (obj->get_affected(k).location != APPLY_SAVING_WILL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_CRITICAL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_STABILITY)&&
                        (obj->get_affected(k).location != APPLY_SAVING_REFLEX)))
                {
                    float weight = count_affect_weight(obj->get_affected(k).location, 0-obj->get_affected(k).modifier);
                    total_weight -= pow(weight, -SQRT_MOD);
                }
	}
	// аффекты AFF_x через weapon_affect
	for (const auto& m : weapon_affect)
	{
		if (IS_OBJ_AFF(obj, m.aff_pos))
		{
			if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_AIRSHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_FIRESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_ICESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_MAGICGLASS)
			{
				total_weight += pow(AFF_MAGICGLASS_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_BLINK)
			{
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			}
		}
	}
        if (total_weight < 1) return result;
	
        result = ceil(pow(total_weight, 1/SQRT_MOD));

	return result;
    
}

float MakeRecept::count_affect_weight(int num, int mod)
{
	float weight = 0;

	switch(num)
	{
	case APPLY_STR:
		weight = mod * 7.5;
		break;
	case APPLY_DEX:
		weight = mod * 10.0;
		break;
	case APPLY_INT:
		weight = mod * 10.0;
		break;
	case APPLY_WIS:
		weight = mod * 10.0;
		break;
	case APPLY_CON:
		weight = mod * 10.0;
		break;
	case APPLY_CHA:
		weight = mod * 10.0;
		break;
	case APPLY_HIT:
		weight = mod * 0.3;
		break;
	case APPLY_AC:
		weight = mod * -0.5;
		break;
	case APPLY_HITROLL:
		weight = mod * 2.3;
		break;
	case APPLY_DAMROLL:
		weight = mod * 3.3;
		break;
	case APPLY_SAVING_WILL:
		weight = mod * -0.5;
		break;
	case APPLY_SAVING_CRITICAL:
		weight = mod * -0.5;
		break;
	case APPLY_SAVING_STABILITY:
		weight = mod * -0.5;
		break;
	case APPLY_SAVING_REFLEX:
		weight = mod * -0.5;
		break;
	case APPLY_CAST_SUCCESS:
		weight = mod * 1.5;
		break;
	case APPLY_MANAREG:
		weight = mod * 0.2;
		break;
	case APPLY_MORALE:
		weight = mod * 1.0;
		break;
	case APPLY_INITIATIVE:
		weight = mod * 2.0;
		break;
	case APPLY_ABSORBE:
		weight = mod * 1.0;
		break;
	case APPLY_AR:
		weight = mod * 1.5;
		break;
	case APPLY_MR:
		weight = mod * 1.5;
		break;
	}

	return weight;
}
void MakeRecept::make_object(CHAR_DATA *ch, OBJ_DATA * obj, OBJ_DATA *ingrs[MAX_PARTS], int ingr_cnt)
{
	int i, j;
	//ставим именительные именительные падежи в алиасы 
	sprintf(buf, "%s %s %s %s",
		GET_OBJ_PNAME(obj, 0).c_str(),
		GET_OBJ_PNAME(ingrs[0], 1).c_str(),
		GET_OBJ_PNAME(ingrs[1], 4).c_str(),
		GET_OBJ_PNAME(ingrs[2], 4).c_str());
	obj->set_aliases(buf);
	for (i = 0; i < CObjectPrototype::NUM_PADS; i++) // ставим падежи в имя с учетов ингров
	{
		sprintf(buf, "%s", GET_OBJ_PNAME(obj, i).c_str());
		strcat(buf, " из ");
		strcat(buf, GET_OBJ_PNAME(ingrs[0], 1).c_str());
		strcat(buf, " с ");
		strcat(buf, GET_OBJ_PNAME(ingrs[1], 4).c_str());
		strcat(buf, " и ");
		strcat(buf, GET_OBJ_PNAME(ingrs[2], 4).c_str());
		obj->set_PName(i, buf);
		if (i == 0) // именительный падеж
		{
			obj->set_short_description(buf);
			if (GET_OBJ_SEX(obj) == ESex::SEX_MALE)
			{
				sprintf(buf2, "Брошенный %s лежит тут.", buf);
			}
			else if (GET_OBJ_SEX(obj) == ESex::SEX_FEMALE)
			{
				sprintf(buf2, "Брошенная %s лежит тут.", buf);
			}
			else if (GET_OBJ_SEX(obj) == ESex::SEX_POLY)
			{
				sprintf(buf2, "Брошенные %s лежат тут.", buf);
			}
			obj->set_description(buf2); // описание на земле
		}
	}
	obj->set_is_rename(true); // ставим флаг что объект переименован
	
	
	auto temp_flags = obj->get_affect_flags();
	add_flags(ch, &temp_flags, &ingrs[0]->get_affect_flags(), get_ingr_pow(ingrs[0]));
	obj->set_affect_flags(temp_flags);
	// перносим эффекты ... с ингров на прототип, 0 объект шкура переносим все, с остальных 1 рандом
	temp_flags = obj->get_extra_flags();
	add_flags(ch, &temp_flags, &GET_OBJ_EXTRA(ingrs[0]), get_ingr_pow(ingrs[0]));
	obj->set_extra_flags(temp_flags);
	auto temp_affected = obj->get_all_affected();
	add_affects(ch,temp_affected, ingrs[0]->get_all_affected(), get_ingr_pow(ingrs[0]));
	obj->set_all_affected(temp_affected);
	add_rnd_skills(ch, ingrs[0], obj); //переносим случайную умелку со шкуры
	obj->set_extra_flag(EExtraFlag::ITEM_NOALTER);  // нельзя сфрешить черным свитком
	obj->set_timer((GET_OBJ_VAL(ingrs[0], 3) + 1) * 1000 + ch->get_skill(SKILL_MAKE_WEAR) * number(180, 220)); // таймер зависит в основном от умелки
	obj->set_craft_timer(obj->get_timer()); // запомним таймер созданной вещи для правильного отображения при осм для ее сост.
	for (j = 1; j < ingr_cnt; j++)
	{
		int i, raffect = 0;
		for (i = 0; i < MAX_OBJ_AFFECT; i++) // посмотрим скока аффектов
		{
			if (ingrs[j]->get_affected(i).location == APPLY_NONE)
			{
				break;
			}
		}
		if (i > 0) // если > 0 переносим случайный
		{
			raffect = number(0, i - 1);
			for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				const auto& ra = ingrs[j]->get_affected(raffect);
				if (obj->get_affected(i).location == ra.location) // если аффект такой уже висит и он меньше, переставим значение
				{
					if (obj->get_affected(i).modifier < ra.modifier)
					{
						obj->set_affected(i, obj->get_affected(i).location, ra.modifier);
					}
					break;
				}
				if (obj->get_affected(i).location == APPLY_NONE) // добавляем афф на свободное место
				{
					if (number(1, 100) > GET_OBJ_VAL(ingrs[j], 2)) // проверим фейл на силу ингра
					{
						break;
					}
					obj->set_affected(i, ra);
					break;
				}
			}
		}
		// переносим аффекты ... c ингров на прототип.
		auto temp_flags = obj->get_affect_flags();
		add_flags(ch, &temp_flags, &ingrs[j]->get_affect_flags(), get_ingr_pow(ingrs[j]));
		obj->set_affect_flags(temp_flags);
		// перносим эффекты ... с ингров на прототип.
		temp_flags = obj->get_extra_flags();
		add_flags(ch, &temp_flags, &GET_OBJ_EXTRA(ingrs[j]), get_ingr_pow(ingrs[j]));
		obj->set_extra_flags(temp_flags);
		// переносим 1 рандом аффект
		add_rnd_skills(ch, ingrs[j], obj); //переноси случайную умелку с ингров
	}
}
// создать предмет по рецепту
int MakeRecept::make(CHAR_DATA * ch)
{
	char tmpbuf[MAX_STRING_LENGTH];//, tmpbuf2[MAX_STRING_LENGTH];
	OBJ_DATA *ingrs[MAX_PARTS];
	string tmpstr, charwork, roomwork, charfail, roomfail, charsucc, roomsucc, chardam, roomdam, tagging, itemtag;
	int dam = 0;
	bool make_fail;
	// 1. Проверить есть ли скилл у чара
	if (IS_NPC(ch) || !ch->get_skill(skill))
	{
		send_to_char("Странно что вам вообще пришло в голову cделать это.\r\n", ch);
		return (FALSE);
	}
	// 2. Проверить есть ли ингры у чара
	if (!can_make(ch))
	{
		send_to_char("У вас нет составляющих для этого.\r\n", ch);
		return (FALSE);
	}
	if (GET_MOVE(ch) < MIN_MAKE_MOVE)
	{
		send_to_char("Вы слишком устали и вам ничего не хочется делать.\r\n", ch);
		return (FALSE);
	}
	auto tobj = get_object_prototype(obj_proto);
	if (!tobj)
	{
		return 0;
	}
	// Проверяем замемлены ли заклы у чара на посох
	if (!IS_IMMORTAL(ch) && (skill == SKILL_MAKE_STAFF) && (GET_SPELL_MEM(ch, GET_OBJ_VAL(tobj, 3)) == 0))
	{
		const OBJ_DATA obj(*tobj);
		act("Вы не готовы к тому чтобы сделать $o3.", FALSE, ch, &obj, 0, TO_CHAR);
		return (FALSE);
	}
	// Прогружаем в массив реальные ингры
	// 3. Проверить уровни ингров и чара
	int ingr_cnt = 0, ingr_lev, i, craft_weight, ingr_pow;
	for (i = 0; i < MAX_PARTS; i++)
	{
		if (parts[i].proto == 0)
			break;
		ingrs[i] = get_obj_in_list_ingr(parts[i].proto, ch->carrying);
		ingr_lev = get_ingr_lev(ingrs[i]);
		if (!IS_IMPL(ch) && (ingr_lev > (GET_LEVEL(ch) + 2 * GET_REMORT(ch))))
		{
			tmpstr = "Вы побоялись испортить " + ingrs[i]->get_PName(3)
				+ "\r\n и прекратили работу над " + tobj->get_PName(4) + ".\r\n";
			send_to_char(tmpstr.c_str(), ch);
			return (FALSE);
		};
		ingr_pow = get_ingr_pow(ingrs[i]);
		if (ingr_pow < parts[i].min_power)
		{
			tmpstr = "$o не подходит для изготовления " + tobj->get_PName(1) + ".";
			act(tmpstr.c_str(), FALSE, ch, ingrs[i], 0, TO_CHAR);
			return (FALSE);
		}
		ingr_cnt++;
	}
	//int stat_bonus;
	// Делаем всякие доп проверки для различных умений.
	switch (skill)
	{
	case SKILL_MAKE_WEAPON:
	case SKILL_MAKE_ARMOR:
		// Проверяем есть ли тут наковальня или комната кузня.
		if ((!ROOM_FLAGGED(ch->in_room, ROOM_SMITH)) && (!IS_IMMORTAL(ch)))
		{
			send_to_char("Вам нужно попасть в кузницу для этого.\r\n", ch);
			return (FALSE);
		}
		charwork = "Вы поместили заготовку на наковальню и начали ковать $o3.";
		roomwork = "$n поместил$g закотовку на наковальню и начал$g ковать.";
		charfail = "Заготовка под вашими ударами покрылась сетью трещин и раскололась.";
		roomfail = "Под ударами молота $n1 заготовка раскололась.";
		charsucc = "Вы выковали $o3.";
		roomsucc = "$n выковал$G $o3.";
		chardam = "Заготовка выскочила из под молота и больно ударила вас.";
		roomdam = "Заготовка выскочила из под молота $n1, больно $s ударив.";
		tagging = "Вы поставили свое клеймо на $o5.";
		itemtag = "На $o5 стоит клеймо 'Выковал$g $n'.";
		dam = 70;
		// Бонус сила
		//stat_bonus = number(0, GET_REAL_STR(ch));
		break;
	case SKILL_MAKE_BOW:
		charwork = "Вы начали мастерить $o3.";
		roomwork = "$n начал$g мастерить что-то очень напоминающее $o3.";
		charfail = "С громким треском $o сломал$U в ваших неумелых руках.";
		roomfail = "С громким треском $o сломал$U в неумелых руках $n1.";
		charsucc = "Вы cмастерили $o3.";
		roomsucc = "$n смастерил$g $o3.";
		chardam = "$o3 с громким треском сломал$U оцарапав вам руки.";
		roomdam = "$o3 с громким треском сломал$U оцарапав руки $n2.";
		tagging = "Вы вырезали свое имя на $o5.";
		itemtag = "На $o5 видна метка 'Смастерил$g $n'.";
		// Бонус ловкость
		//stat_bonus = number(0, GET_REAL_DEX(ch));
		dam = 40;
		break;
	case SKILL_MAKE_WEAR:
		charwork = "Вы взяли в руку иголку и начали шить $o3.";
		roomwork = "$n взял$g в руку иголку и начал$g увлеченно шить.";
		charfail = "У вас ничего не получилось сшить.";
		roomfail = "$n пытал$u что-то сшить, но ничего не вышло.";
		charsucc = "Вы сшили $o3.";
		roomsucc = "$n сшил$g $o3.";
		chardam = "Игла глубоко вошла в вашу руку. Аккуратнее надо быть.";
		roomdam = "$n глубоко воткнул$g иглу в себе в руку. \r\nА с виду вполне нормальный человек.";
		tagging = "Вы пришили к $o2 бирку со своим именем.";
		itemtag = "На $o5 вы заметили бирку 'Сшил$g $n'.";
		// Бонус тело , не спрашивайте почему :))
		//stat_bonus = number(0, GET_REAL_CON(ch));
		dam = 30;
		break;
	case SKILL_MAKE_AMULET:
		charwork = "Вы взяли в руки необходимые материалы и начали мастерить $o3.";
		roomwork = "$n взял$g в руки что-то и начал$g увлеченно пыхтеть над чем-то.";
		charfail = "У вас ничего не получилось";
		roomfail = "$n пытал$u что-то сделать, но ничего не вышло.";
		charsucc = "Вы смастерили $o3.";
		roomsucc = "$n смастерил$g $o3.";
		chardam = "Яркая вспышка дала вам знать, что с магией необходимо быть поосторожней.";
		roomdam = "Яркая вспышка осветила все вокруг. $n2 стоит быть аккуратнее с магией! \r\n";
		tagging = "На обратной стороне $o1 вы нацарапали свое имя.";
		itemtag = "На обратной стороне $o1 вы заметили слово '$n', видимо, это имя создателя этой чудной вещицы.";
		dam = 30;
		break;
	case SKILL_MAKE_JEWEL:
		charwork = "Вы начали мастерить $o3.";
		roomwork = "$n начал мастерить какую-то диковинку.";
		charfail = "Ваше неудачное движение повредило $o5.";
		roomfail = "Неудачное движение $n, сделало $s работу бессмысленной.";
		charsucc = "Вы смастерили $o3.";
		roomsucc = "$n смастерил$g $o3.";
		chardam = "Мелкий кусочек материала отскочил и попал вам в глаз.\r\nЭто было больно!";
		roomdam = "Мелкий кусочек материала отскочил и попал в глаз $n2.";
		tagging = "Вы приладили к $o2 табличку со своим именем.";
		itemtag = "С нижней стороны $o1 укреплена табличка 'Cделано $n4'.";
		// Бонус харя
		//stat_bonus = number(0, GET_REAL_CHA(ch));
		dam = 30;
		break;
	case SKILL_MAKE_STAFF:
		charwork = "Вы начали мастерить $o3.";
		roomwork = "$n начал мастерить что-то нашептывая при этом странные слова.";
		charfail = "$o3 осветил комнату магическим светом и истаял.";
		roomfail = "Предмет в руках $n1 вспыхнул, озарив комнату магическим светом и истаял.";
		charsucc =
			"Тайные знаки нанесенные на $o3 вспыхнули и погасли.\r\nДа, $E хорошо послужит своему хозяину.";
		roomsucc = "$n смастерил$g $o3. Вы почуствовали скрытую силу спрятанную в этом предмете.";
		chardam = "$o взорвался в ваших руках. Вас сильно обожгло.";
		roomdam = "$o взорвался в руках $n1, опалив его.\r\nВокруг приятно запахло жаренным мясом.";
		tagging = "Вы начертили на $o2 свое имя.";
		itemtag = "Среди рунных знаков видна надпись 'Создано $n4'.";
		// Бонус ум.
		//stat_bonus = number(0, GET_REAL_INT(ch));
		dam = 70;
		break;
	case SKILL_MAKE_POTION:
		charwork = "Вы достали небольшой горшочек и развели под ним огонь, начав варить $o3.";
		roomwork = "$n достал горшочек и поставил его на огонь.";
		charfail = "Вы не уследили как зелье закипело и пролилось в огонь.";
		roomfail =
			"Зелье которое варил$g $n закипело и пролилось в огонь,\r\n распространив по комнате ужасную вонь.";
		charsucc = "Зелье удалось вам на славу.";
		roomsucc = "$n сварил$g $o3. Приятный аромат зелья из горшочка, так и манит вас.";
		chardam = "Вы опрокинули горшочек с зельем на себя, сильно ошпарившись.";
		roomdam = "Горшочек с $o4 опрокинулся на $n1, ошпарив $s.";
		tagging = "Вы на прикрепили к $o2 бирку со своим именем.";
		itemtag = "На $o1 вы заметили бирку 'Сварено $n4'";
		// Бонус мудра
		//stat_bonus = number(0, GET_REAL_WIS(ch));
		dam = 40;
		break;
	default:
		break;
	}
	const OBJ_DATA object(*tobj);
	act(charwork.c_str(), FALSE, ch, &object, 0, TO_CHAR);
	act(roomwork.c_str(), FALSE, ch, &object, 0, TO_ROOM);
	// Считаем вероятность испортить отдельный ингридиент
	// если уровень чара = уровню ингра то фейл 50%
	// если уровень чара > уровня ингра на 15 то фейл 0%
	// уровень чара * 2 - random(30) < 15 - фейл то пропадает весь материал
	// Выдается Вы испортили (...)
	// и выходим.
	make_fail = false;
	// Это средний уровень получившегося предмета.
	// использует при расчете макс количества предметов в мире.
	int created_lev = 0;
	int used_non_ingrs = 0;
	for (i = 0; i < ingr_cnt; i++)
	{
		ingr_lev = get_ingr_lev(ingrs[i]);
		if (ingr_lev < 0)
		{
			used_non_ingrs++;
		}
		else
		{
			created_lev += ingr_lev;
		}
		// Шанс испортить не ингредиент всетаки есть.
		if ((number(0, 30) < (5 + ingr_lev - GET_LEVEL(ch) - 2 * GET_REMORT(ch))) && !IS_IMPL(ch))
		{
			tmpstr = "Вы испортили " + ingrs[i]->get_PName(3) + ".\r\n";
			send_to_char(tmpstr.c_str(), ch);
			//extract_obj(ingrs[i]); //заменим на обнуление веса
			//чтобы не крешило дальше в обработке фейла (Купала)
			IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(ingrs[i]);
			ingrs[i]->set_weight(0);
			make_fail = true;
		}
	};
	created_lev = created_lev / MAX(1, (ingr_cnt - used_non_ingrs));
	int j;
	int craft_move = MIN_MAKE_MOVE + (created_lev / 2) - 1;
	// Снимаем мувы за умение
	if (GET_MOVE(ch) < craft_move)
	{
		GET_MOVE(ch) = 0;
		// Вам не хватило сил доделать.
		tmpstr = "Вам не хватило сил доделать " + tobj->get_PName(3) + ".\r\n";
		send_to_char(tmpstr.c_str(), ch);
		make_fail = true;
	}
	else
	{
		if (!IS_IMPL(ch))
		{
			GET_MOVE(ch) -= craft_move;
		}
	}
	// Делаем тут прокачку умения.
	// Прокачка должна зависеть от среднего уровня материала и игрока.
	// При разнице со средним уровнем до 0 никаких штрафов.
	// При разнице большей чем 1 уровней замедление в 2 раза.
	// При разнице большей чем в 2 уровней замедление в 3 раза.
	if (skill == SKILL_MAKE_STAFF)
	{
		if (number(0, GET_LEVEL(ch) - created_lev) < GET_SPELL_MEM(ch, GET_OBJ_VAL(tobj, 3)))
		{
			train_skill(ch, skill, skill_info[skill].max_percent, 0);
		}
	}
	train_skill(ch, skill, skill_info[skill].max_percent, 0);
	// 4. Считаем сколько материала треба.
	if (!make_fail)
	{
		for (i = 0; i < ingr_cnt; i++)
		{
			if (skill == SKILL_MAKE_WEAR && i == 0) //для шитья всегда раскраиваем шкуру 
			{
				IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(ingrs[0]);
				ingrs[0]->set_weight(0);  // шкуру дикеим полностью
				tmpstr = "Вы раскроили полностью " + ingrs[0]->get_PName(3) + ".\r\n";
				send_to_char(tmpstr.c_str(), ch);
				continue;
			}
			//
			// нужный материал = мин.материал +
			// random(100) - skill
			// если она < 20 то мин.вес + rand(мин.вес/3)
			// если она < 50 то мин.вес*rand(1,2) + rand(мин.вес/3)
			// если она > 50    мин.вес*rand(2,5) + rand(мин.вес/3)
			if (get_ingr_lev(ingrs[i]) == -1)
				continue;	// Компонент не ингр. пропускаем.
			craft_weight = parts[i].min_weight + number(0, (parts[i].min_weight / 3) + 1);
			j = number(0, 100) - calculate_skill(ch, skill, 0);
			if ((j >= 20) && (j < 50))
				craft_weight += parts[i].min_weight * number(1, 2);
			else if (j > 50)
				craft_weight += parts[i].min_weight * number(2, 5);
			// 5. Делаем проверку есть ли столько материала.
			// если не хватает то удаляем игридиент и фейлим.
			int state = craft_weight;
			// Обсчет веса ингров в цикле, если не хватило веса берем следующий ингр в инве, если не хватает, делаем фэйл (make_fail) и брекаем внешний цикл, смысл дальше ингры смотреть?
			//send_to_char(ch, "Требуется вес %d вес ингра %d требуемое кол ингров %d\r\n", state, GET_OBJ_WEIGHT(ingrs[i]), ingr_cnt);
			int obj_vnum_tmp = GET_OBJ_VNUM(ingrs[i]);
			while (state > 0)
			{
				//Переделаем слегка логику итераций
				//Сперва проверяем сколько нам нужно. Если вес ингра больше, чем требуется, то вычитаем вес и останавливаем итерацию.
				if (GET_OBJ_WEIGHT(ingrs[i]) > state)
				{
					ingrs[i]->sub_weight(state);
					send_to_char(ch, "Вы использовали %s.\r\n", ingrs[i]->get_PName(3).c_str());
					IS_CARRYING_W(ch) -= state;
					break;
				}
				//Если вес ингра ровно столько, сколько требуется, вычитаем вес, ломаем ингр и останавливаем итерацию.
				else if (GET_OBJ_WEIGHT(ingrs[i]) == state)
				{
					IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(ingrs[i]);
					ingrs[i]->set_weight(0);
					send_to_char(ch, "Вы полностью использовали %s.\r\n", ingrs[i]->get_PName(3).c_str());
					//extract_obj(ingrs[i]);
					break;
				}
				//Если вес ингра меньше, чем требуется, то вычтем этот вес из того, сколько требуется.
				else
				{
					state = state - GET_OBJ_WEIGHT(ingrs[i]);
					send_to_char(ch, "Вы полностью использовали %s и начали искать следующий ингредиент.\r\n", ingrs[i]->get_PName(3).c_str());
					std::string tmpname = std::string(ingrs[i]->get_PName(1).c_str());
					IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(ingrs[i]);
					ingrs[i]->set_weight(0);
					extract_obj(ingrs[i]);
					ingrs[i] = nullptr;
					//Если некст ингра в инве нет, то сообщаем об этом и идем в фэйл. Некст ингры все равно проверяем
					if (!get_obj_in_list_ingr(obj_vnum_tmp, ch->carrying))
					{
						send_to_char(ch, "У вас в инвентаре больше нет %s.\r\n", tmpname.c_str());
						make_fail = true;
						break;
					}
					//Подцепляем некст ингр и идем в нашу проверку заново
					else
					{
						ingrs[i] = get_obj_in_list_ingr(obj_vnum_tmp, ch->carrying);
					}
				}
			}
		}
	}
	if (make_fail)
	{
		// Считаем крит фейл или нет.
		int crit_fail = number(0, 100);
		if (crit_fail > 2)
		{
			const OBJ_DATA obj(*tobj);
			act(charfail.c_str(), FALSE, ch, &obj, 0, TO_CHAR);
			act(roomfail.c_str(), FALSE, ch, &obj, 0, TO_ROOM);
		}
		else
		{
			const OBJ_DATA obj(*tobj);
			act(chardam.c_str(), FALSE, ch, &obj, 0, TO_CHAR);
			act(roomdam.c_str(), FALSE, ch, &obj, 0, TO_ROOM);
			dam = number(0, dam);
			// Наносим дамаг.
			if (GET_LEVEL(ch) >= LVL_IMMORT && dam > 0)
			{
				send_to_char("Будучи бессмертным, вы избежали повреждения...", ch);
				return (FALSE);
			}
			GET_HIT(ch) -= dam;
			update_pos(ch);
			char_dam_message(dam, ch, ch, 0);
			if (GET_POS(ch) == POS_DEAD)
			{
				// Убился веником.
				if (!IS_NPC(ch))
				{
					sprintf(tmpbuf, "%s killed by a crafting at %s",
							GET_NAME(ch),
							ch->in_room == NOWHERE ? "NOWHERE" : world[ch->in_room]->name);
					mudlog(tmpbuf, BRF, LVL_BUILDER, SYSLOG, TRUE);
				}
				die(ch, NULL);
			}
		}
		for (i = 0; i < ingr_cnt; i++)
		{
			if (ingrs[i] && GET_OBJ_WEIGHT(ingrs[i]) <= 0)
			{
				extract_obj(ingrs[i]);
			}
		}
		return (FALSE);
	}
	// Лоадим предмет игроку
	const auto obj = world_objects.create_from_prototype_by_vnum(obj_proto);
	act(charsucc.c_str(), FALSE, ch, obj.get(), 0, TO_CHAR);
	act(roomsucc.c_str(), FALSE, ch, obj.get(), 0, TO_ROOM);
	// 6. Считаем базовые статсы предмета и таймер
	//  формула для каждого умения отдельная
	// Для числовых х-к:  х-ка+(skill - random(100))/20;
	// Для флагов ???: random(200) - skill > 0 то флаг переноситься.
	// Т.к. сделать мы можем практически любой предмет.
	// Модифицируем вес предмета и его таймер.
	// Для маг предметов надо в сторону облегчения.
//	i = GET_OBJ_WEIGHT(obj);
	switch(skill) {
		case SKILL_MAKE_BOW:
			obj->set_extra_flag(EExtraFlag::ITEM_TRANSFORMED);
		break;
		case SKILL_MAKE_WEAR:
			obj->set_extra_flag(EExtraFlag::ITEM_NOT_DEPEND_RPOTO);
		break;
		default:
		break;
	}
	int sign = -1;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_INGREDIENT)
	{
		sign = 1;
	}
	obj->set_weight(stat_modify(ch, GET_OBJ_WEIGHT(obj), 20 * sign));

	i = obj->get_timer();
	obj->set_timer(stat_modify(ch, obj->get_timer(), 1));
	// Модифицируем уникальные для предметов х-ки.
	// Балансировка варьируется изменением делителя (!20!)
	// при делителе 20 и умении 100 и макс везении будет +5 к параметру
	// Можно посчитать бонусы от времени суток.
	// Считаем среднюю силу ингров .
	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_LIGHT:
		// Считаем длительность свечения.
		if (GET_OBJ_VAL(obj, 2) != -1)
		{
			obj->set_val(2, stat_modify(ch, GET_OBJ_VAL(obj, 2), 1));
		}
		break;
	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		// Считаем уровень закла
		obj->set_val(0, GET_LEVEL(ch));
		break;
	case OBJ_DATA::ITEM_WEAPON:
		// Считаем число xdy
		// модифицируем XdY
		if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 2))
		{
			obj->set_val(1, stat_modify(ch, GET_OBJ_VAL(obj, 1), 1));
		}
		else
		{
			obj->set_val(2, stat_modify(ch, GET_OBJ_VAL(obj, 2) - 1, 1) + 1);
		}
		break;
	case OBJ_DATA::ITEM_ARMOR:
	case OBJ_DATA::ITEM_ARMOR_LIGHT:
	case OBJ_DATA::ITEM_ARMOR_MEDIAN:
	case OBJ_DATA::ITEM_ARMOR_HEAVY:
		// Считаем + АС
		obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
		// Считаем поглощение.
		obj->set_val(1, stat_modify(ch, GET_OBJ_VAL(obj, 1), 1));
		break;
	case OBJ_DATA::ITEM_POTION:
		// Считаем уровень итоговый напитка
		obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
		break;
	case OBJ_DATA::ITEM_CONTAINER:
		// Считаем объем контейнера.
		obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
		break;
	case OBJ_DATA::ITEM_DRINKCON:
		// Считаем объем контейнера.
		obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
		break;
	case OBJ_DATA::ITEM_INGREDIENT:
		// Для ингров ничего не трогаем ... ибо опасно. :)
		break;
	default:
		break;
	}
	// 7. Считаем доп. статсы предмета.
	// х-ка прототипа +
	// если (random(100) - сила ингра ) < 1 то переноситься весь параметр.
	// если от 1 до 25 то переноситься 1/2
	// если от 25 до 50 то переноситься 1/3
	// больше переноситься 0
	// переносим доп аффекты ...+мудра +ловка и т.п.
	if (skill == SKILL_MAKE_WEAR)
	{ 
		make_object(ch, obj.get(), ingrs, ingr_cnt );
		make_value_wear(ch, obj.get(), ingrs);
	}
	else // если не шитье то никаких махинаций с падежами и копированием рандом аффекта
	{
		for (j = 0; j < ingr_cnt; j++)
		{
			ingr_pow = get_ingr_pow(ingrs[j]);
			if (ingr_pow < 0)
			{
				ingr_pow = 20;
			}
			// переносим аффекты ... c ингров на прототип.
			auto temp_flags = obj->get_affect_flags();
			add_flags(ch, &temp_flags, &ingrs[j]->get_affect_flags(), ingr_pow);
			obj->set_affect_flags(temp_flags);
			temp_flags = obj->get_extra_flags();
			// перносим эффекты ... с ингров на прототип.
			add_flags(ch, &temp_flags, &GET_OBJ_EXTRA(ingrs[j]), ingr_pow);
			obj->set_extra_flags(temp_flags);
			auto temp_affected = obj->get_all_affected();
			add_affects(ch, temp_affected, ingrs[j]->get_all_affected(), ingr_pow);
			obj->set_all_affected(temp_affected);
		}
	}
	// Мочим истраченные ингры.
	for (i = 0; i < ingr_cnt; i++)
	{
		if (GET_OBJ_WEIGHT(ingrs[i]) <= 0)
		{
			extract_obj(ingrs[i]);
		}
	}
	// 8. Проверяем мах. инворлд.
	// Считаем по формуле (31 - ср. уровень предмета) * 5 -
	// овер шмота в мире не 30 лева не больше 5 штук
	// Т.к. ср. уровень ингров будет определять
	// число шмоток в мире то шмотки по хуже будут вытеснять
	// шмотки по лучше (в целом это не так страшно).
	// Ставим метку если все хорошо.
	if ((GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_INGREDIENT
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MING)
		&& (number(1, 100) - calculate_skill(ch, skill, 0) < 0))
	{
		act(tagging.c_str(), FALSE, ch, obj.get(), 0, TO_CHAR);
		// Прибавляем в экстра описание строчку.
		char *tagchar = format_act(itemtag.c_str(), ch, obj.get(), 0);
		obj->set_tag(tagchar);
		free(tagchar);
	};
        // простановка мортов при шитье
	float total_weight = count_mort_requred(obj.get()) * 7 / 10;
          
        if (total_weight > 35)
	{
		obj->set_minimum_remorts(12);
	}
	else if (total_weight > 33)
	{
		obj->set_minimum_remorts(11);
	}
	else if (total_weight > 30)
	{
		obj->set_minimum_remorts(9);
	}
	else if (total_weight > 25)
	{
		obj->set_minimum_remorts(6);
	}
	else if (total_weight > 20)
	{
		obj->set_minimum_remorts(3);
	}
	else if (total_weight > 20)
	{
		obj->set_minimum_remorts(0);
	}

	// Пишем производителя в поле.
	obj->set_crafter_uid(GET_UNIQUE(ch));
	// 9. Проверяем минимум 2
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
	{
		send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
		obj_to_room(obj.get(), ch->in_room);
	}
	else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
	{
		send_to_char("Вы не сможете унести такой вес.\r\n", ch);
		obj_to_room(obj.get(), ch->in_room);
	}
	else
	{
		obj_to_char(obj.get(), ch);
	}
	return (TRUE);
}
// вытащить рецепт из строки.
int MakeRecept::load_from_str(string & rstr)
{
	// Разбираем строку.
	char tmpbuf[MAX_INPUT_LENGTH];
	// Проверяем рецепт на блокировку .
	if (rstr.substr(0, 1) == string("*"))
	{
		rstr = rstr.substr(1);
		locked = true;
	}
	else
	{
		locked = false;
	}
	skill = static_cast<ESkill>(atoi(rstr.substr(0, rstr.find(" ")).c_str()));
	rstr = rstr.substr(rstr.find(" ") + 1);
	obj_proto = atoi((rstr.substr(0, rstr.find(" "))).c_str());
	rstr = rstr.substr(rstr.find(" ") + 1);

	if (real_object(obj_proto) < 0)
	{
		// Обнаружен несуществующий прототип объекта.
		sprintf(tmpbuf, "MakeRecept::Unfound object proto %d", obj_proto);
		mudlog(tmpbuf, LGH, LVL_IMMORT, SYSLOG, TRUE);
		// блокируем рецепты без ингров.
		locked = true;
	}

	for (int i = 0; i < MAX_PARTS; i++)
	{
		// считали номер прототипа компонента
		parts[i].proto = atoi((rstr.substr(0, rstr.find(" "))).c_str());
		rstr = rstr.substr(rstr.find(" ") + 1);
		// Проверяем на конец компонентов.
		if (parts[i].proto == 0)
		{
			break;
		}
		if (real_object(parts[i].proto) < 0)
		{
			// Обнаружен несуществующий прототип компонента.
			sprintf(tmpbuf, "MakeRecept::Unfound item part %d for %d", obj_proto, parts[i].proto);
			mudlog(tmpbuf, LGH, LVL_IMMORT, SYSLOG, TRUE);
			// блокируем рецепты без ингров.
			locked = true;
		}
		parts[i].min_weight = atoi(rstr.substr(0, rstr.find(" ")).c_str());
		rstr = rstr.substr(rstr.find(" ") + 1);
		parts[i].min_power = atoi(rstr.substr(0, rstr.find(" ")).c_str());
		rstr = rstr.substr(rstr.find(" ") + 1);
	}
	return (TRUE);
}
// сохранить рецепт в строку.
int MakeRecept::save_to_str(string & rstr)
{
	char tmpstr[MAX_INPUT_LENGTH];
	if (obj_proto == 0)
	{
		return (FALSE);
	}
	if (locked)
	{
		rstr = "*";
	}
	else
	{
		rstr = "";
	}
	sprintf(tmpstr, "%d %d", skill, obj_proto);
	rstr += string(tmpstr);
	for (int i = 0; i < MAX_PARTS; i++)
	{
		sprintf(tmpstr, " %d %d %d", parts[i].proto, parts[i].min_weight, parts[i].min_power);
		rstr += string(tmpstr);
	}
	return (TRUE);
}
// Модификатор базовых значений.
int MakeRecept::stat_modify(CHAR_DATA * ch, int value, float devider)
{
	// Для числовых х-к:  х-ка+(skill - random(100))/20;
	// Для флагов ???: random(200) - skill > 0 то флаг переноситься.
	int res = value;
	float delta = 0;
	int skill_prc = 0;
	if (devider <= 0)
	{
		return res;
	}
	skill_prc = calculate_skill(ch, skill, 0);
	delta = (int)((float)(skill_prc - number(0, skill_info[skill].max_percent)));
	if (delta > 0)
	{
		delta = (value / 2) * delta / skill_info[skill].max_percent / devider;
	}
	else
	{
		delta = (value / 4) * delta / skill_info[skill].max_percent / devider;
	}
	res += (int) delta;
	// Если параметр завалили то возвращаем 1;
	if (res < 0)
	{
		return 1;
	}
	return res;
}
void MakeRecept::add_rnd_skills(CHAR_DATA* /*ch*/, OBJ_DATA * obj_from, OBJ_DATA *obj_to)
{
	if (obj_from->has_skills())
	{
		int skill_num, rskill;
		int z = 0;
		int percent;
//		send_to_char("Копирую умения :\r\n", ch);
		CObjectPrototype::skills_t skills;
		obj_from->get_skills(skills);
		int i = static_cast<int>(skills.size()); // сколько добавлено умелок
		rskill = number(0, i); // берем рандом
//		sprintf(buf, "Всего умений %d копируем из них случайное под N %d.\r\n", i, rskill);
//		send_to_char(buf,  ch);
		for (const auto& it : skills)
		{	
			if (z == rskill) // ставим рандомную умелку
			{
				skill_num = it.first;
				percent = it.second;
				if (percent == 0) // TODO: такого не должно быть?
				{
					continue;
				}
//				sprintf(buf, "   %s%s%s%s%s%d%%%s\r\n",
//				CCCYN(ch, C_NRM), skill_info[skill_num].name, CCNRM(ch, C_NRM),
//				CCCYN(ch, C_NRM),
//				percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), CCNRM(ch, C_NRM));
//				send_to_char(buf, ch);
				obj_to->set_skill(skill_num, percent);// копируем скиллы
			}
			z++;
		}
	}
}
int MakeRecept::add_flags(CHAR_DATA * ch, FLAG_DATA * base_flag, const FLAG_DATA* add_flag, int/* delta*/)
{
// БЕз вариантов вообще :(
	int tmpprob;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 32; j++)
		{
			tmpprob = number(0, 200) - calculate_skill(ch, skill, 0);
			if ((add_flag->get_plane(i) & (1 << j)) && (tmpprob < 0))
			{
				base_flag->set_flag(i, 1 << j);
			}
		}
	}
	return (TRUE);
}
int MakeRecept::add_affects(CHAR_DATA * ch, std::array<obj_affected_type, MAX_OBJ_AFFECT>& base, const std::array<obj_affected_type, MAX_OBJ_AFFECT>& add, int delta)
{
	bool found = false;
	int i, j;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		found = false;
		if (add[i].location == APPLY_NONE)
			continue;
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
		{
			if (base[j].location == APPLY_NONE)
				continue;
			if (add[i].location == base[j].location)
			{
				// Аффекты совпали.
				found = true;
				if (number(0, 100) > delta)
					break;
				base[j].modifier += stat_modify(ch, add[i].modifier, 1);
				break;
			}
		}
		if (!found)
		{
			// Ищем первый свободный аффект и втыкаем туда новый.
			for (int j = 0; j < MAX_OBJ_AFFECT; j++)
			{
				if (base[j].location == APPLY_NONE)
				{
					if (number(0, 100) > delta)
						break;
					base[j].location = add[i].location;
					base[j].modifier += add[i].modifier;
//    cout << "add affect " << int(base[j].location) <<" - " << int(base[j].modifier) << endl;
					break;
				}
			}
		}
	}
	return (TRUE);
}
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
