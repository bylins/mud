/*************************************************************************
*   File: item.creation.cpp                            Part of Bylins    *
*   Item creation from magic ingidients                                  *
*                           *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */
#include "item_creation.h"

#include "engine/db/obj_prototypes.h"
#include "engine/core/handler.h"
#include "engine/olc/olc.h"
#include "engine/ui/modify.h"
#include "gameplay/fight/fight.h"
#include "engine/entities/entities_constants.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/core/constants.h"
#include "engine/observability/metrics.h"

#include <cmath>

char *format_act(const char *orig, CharData *ch, ObjData *obj, const void *vict_obj);

constexpr auto WEAR_TAKE = to_underlying(EWearFlag::kTake);
constexpr auto WEAR_TAKE_BOTHS_WIELD = WEAR_TAKE | to_underlying(EWearFlag::kBoth) | to_underlying(EWearFlag::kWield);
constexpr auto WEAR_TAKE_BODY = WEAR_TAKE | to_underlying(EWearFlag::kBody);
constexpr auto WEAR_TAKE_ARMS = WEAR_TAKE | to_underlying(EWearFlag::kArms);
constexpr auto WEAR_TAKE_LEGS = WEAR_TAKE | to_underlying(EWearFlag::kLegs);
constexpr auto WEAR_TAKE_HEAD = WEAR_TAKE | to_underlying(EWearFlag::kHead);
constexpr auto WEAR_TAKE_BOTHS = WEAR_TAKE | to_underlying(EWearFlag::kBoth);
constexpr auto WEAR_TAKE_DUAL = WEAR_TAKE | to_underlying(EWearFlag::kHold) | to_underlying(EWearFlag::kWield);
constexpr auto WEAR_TAKE_HOLD = WEAR_TAKE | to_underlying(EWearFlag::kHold);
struct create_item_type created_item[] =
	{
		{300, 0x7E, 15, 40, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_BOTHS_WIELD},
		{301, 0x7E, 12, 40, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_BOTHS_WIELD},
		{302, 0x7E, 8, 25, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_DUAL},
		{303, 0x7E, 5, 13, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_HOLD},
		{304, 0x7E, 10, 35, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_BOTHS_WIELD},
		{305, 0, 8, 15, {{0, 0, 0}}, ESkill::kUndefined, WEAR_TAKE_BOTHS_WIELD},
		{306, 0, 8, 20, {{0, 0, 0}}, ESkill::kUndefined, WEAR_TAKE_BOTHS_WIELD},
		{307, 0x3A, 10, 20, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_BODY},
		{308, 0x3A, 4, 10, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_ARMS},
		{309, 0x3A, 6, 12, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_LEGS},
		{310, 0x3A, 4, 10, {{COAL_PROTO, 0, 0}}, ESkill::kReforging, WEAR_TAKE_HEAD},
		{312, 0, 4, 40, {{WOOD_PROTO, TETIVA_PROTO, 0}}, ESkill::kCreateBow, WEAR_TAKE_BOTHS}
	};
const char *create_item_name[] = {"шелепуга",
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
		{"смастерить предмет", "предметы", ESkill::kMakeStaff},
		{"смастерить лук", "луки", ESkill::kMakeBow},
		{"выковать оружие", "оружие", ESkill::kMakeWeapon},
		{"выковать доспех", "доспех", ESkill::kMakeArmor},
		{"сшить одежду", "одежда", ESkill::kMakeWear},
		{"смастерить диковину", "артеф.", ESkill::kMakeJewel},
		{"смастерить оберег", "оберег", ESkill::kMakeAmulet},
//  { "сварить отвар","варево", ESkill::kMakePotion },
		{"\n", "\n", ESkill::kUndefined}        // Терминатор
	};
const char *create_weapon_quality[] = {"RESERVED",
									   "RESERVED",
									   "RESERVED",
									   "RESERVED",
									   "RESERVED",
									   "невообразимо ужасного качества",    // <3 //
									   "ужасного качества",    // 3 //
									   "более чем жуткого качества",
									   "жуткого качества",    // 4 //
									   "более чем отвратительного качества",
									   "отвратительного качества",    // 5 //
									   "более чем плохого качества",
									   "плохого качества",    // 6 //
									   "хуже чем среднего качества",
									   "среднего качества",    // 7 //
									   "лучше чем среднего качества",
									   "неплохого качества",    // 8 //
									   "более чем неплохого качества",
									   "хорошего качества",    // 9 //
									   "более чем хорошего качества",
									   "превосходного качества",    // 10 //
									   "более чем превосходного качества",
									   "великолепного качества",    // 11 //
									   "более чем великолепного качества",
									   "качества, достойного учеников мастера",    // 12 //
									   "лучшего качества, чем делают ученики мастера",
									   "качества, достойного мастера",    // 13 //
									   "лучшего качества, чем делают мастера",
									   "качества, достойного великого мастера",    // 14 //
									   "лучшего качества, чем делают великие мастера",
									   "качества, достойного руки сына боярского",    // 15 //
									   "качества, более чем достойного руки сына боярского",
									   "качества, достойного руки боярина",    // 16 //
									   "качества, более чем достойного руки боярина",
									   "качества, достойного руки сына воеводы",    // 17 //
									   "качества, более чем достойного руки сына воеводы",
									   "качества, достойного руки воеводы",    // 18 //
									   "качества, более чем достойного руки воеводы",
									   "качества, достойного руки княжича",    // 19 //
									   "качества, более чем достойного руки княжича",
									   "качества, достойного руки князя",    // 20 //
									   "качества, более чем достойного руки князя",
									   "качества, достойного руки героя",    // 21 //
									   "качества, более чем достойного руки героя",
									   "качества, достойного руки великого героя",    // 22 //
									   "качества, более чем достойного руки великого героя",
									   "качества, которое знали только древние титаны",    // 23 //
									   "качества, лучшего чем то, которое знали древние титаны",
									   "качества, достойного младших богов",    // 24 //
									   "качества, более чем достойного младших богов",
									   "качества, достойного богов",    // 25 //
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
CharData *&operator<<(CharData *&ch, const string &p) {
	SendMsgToChar(p, ch);
	return ch;
}

void init_make_items() {
	char tmpbuf[kMaxInputLength];
	sprintf(tmpbuf, "Loading making recepts.");
	mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
	make_recepts.load();
}
// Парсим ввод пользователя в меню правки рецепта
void mredit_parse(DescriptorData *d, char *arg) {
	string sagr = string(arg);
	char tmpbuf[kMaxInputLength];
	string tmpstr;
	MakeRecept *trec = OLC_MREC(d);
	int i;
	switch (OLC_MODE(d)) {
		case MREDIT_MAIN_MENU:
			// Ввод главного меню.
			if (sagr == "1") {
				SendMsgToChar("Введите VNUM изготавливаемого предмета : ", d->character.get());
				OLC_MODE(d) = MREDIT_OBJ_PROTO;
				return;
			}

			if (sagr == "2") {
				// Выводить список умений ... или давать вводить ручками.
				tmpstr = "\r\nСписок доступных умений:\r\n";
				i = 0;
				while (make_skills[i].num != ESkill::kUndefined) {
					sprintf(tmpbuf, "%s%d%s) %s.\r\n", grn, i + 1, nrm, make_skills[i].name);
					tmpstr += string(tmpbuf);
					i++;
				}
				tmpstr += "Введите номер умения : ";
				SendMsgToChar(tmpstr.c_str(), d->character.get());
				OLC_MODE(d) = MREDIT_SKILL;
				return;
			}

			if (sagr == "3") {
				SendMsgToChar("Блокировать рецепт? (y/n): ", d->character.get());
				OLC_MODE(d) = MREDIT_LOCK;
				return;
			}

			for (i = 0; i < MAX_PARTS; i++) {
				if (atoi(sagr.c_str()) - 4 == i) {
					OLC_NUM(d) = i;
					mredit_disp_ingr_menu(d);
					return;
				}
			}

			if (sagr == "d") {
				SendMsgToChar("Удалить рецепт? (y/n):", d->character.get());
				OLC_MODE(d) = MREDIT_DEL;
				return;
			}

			if (sagr == "s") {
				// Сохраняем рецепты в файл
				make_recepts.save();
				SendMsgToChar("Рецепты сохранены.\r\n", d->character.get());
				mredit_disp_menu(d);
				OLC_VAL(d) = 0;
				return;
			}

			if (sagr == "q") {
				// Проверяем не производилось ли изменение
				if (OLC_VAL(d)) {
					SendMsgToChar("Вы желаете сохранить изменения в рецепте? (y/n) : ", d->character.get());
					OLC_MODE(d) = MREDIT_CONFIRM_SAVE;
					return;
				} else {
					// Загружаем рецепты из файла
					// Это восстановит текущее состояние дел.
					make_recepts.load();
					// Очищаем структуры OLC выходим в нормальный режим работы
					cleanup_olc(d, CLEANUP_ALL);
					return;
				}
			}

			SendMsgToChar("Неверный ввод.\r\n", d->character.get());
			mredit_disp_menu(d);
			break;

		case MREDIT_OBJ_PROTO: i = atoi(sagr.c_str());
			if (GetObjRnum(i) < 0) {
				SendMsgToChar("Прототип выбранного вами объекта не существует.\r\n", d->character.get());
			} else {
				trec->obj_proto = i;
				OLC_VAL(d) = 1;
			}
			mredit_disp_menu(d);
			break;

		case MREDIT_SKILL: {
			auto skill_num = atoi(sagr.c_str());
			i = 0;
			while (make_skills[i].num != ESkill::kUndefined) {
				if (skill_num == i + 1) {
					trec->skill = make_skills[i].num;
					OLC_VAL(d) = 1;
					mredit_disp_menu(d);
					return;
				}
				i++;
			}
			SendMsgToChar("Выбрано некорректное умение.\r\n", d->character.get());
			mredit_disp_menu(d);
			break;
		}
		case MREDIT_DEL: {
			if (sagr == "Y" || sagr == "y") {
				SendMsgToChar("Рецепт удален. Рецепты сохранены.\r\n", d->character.get());
				make_recepts.del(trec);
				make_recepts.save();
				make_recepts.load();
				// Очищаем структуры OLC выходим в нормальный режим работы
				cleanup_olc(d, CLEANUP_ALL);
				return;
			} else if (sagr == "N" || sagr == "n") {
				SendMsgToChar("Рецепт не удален.\r\n", d->character.get());
			} else {
				SendMsgToChar("Неверный ввод.\r\n", d->character.get());
			}
			mredit_disp_menu(d);
			break;
		}
		case MREDIT_LOCK: {
			if (sagr == "Y" || sagr == "y") {
				SendMsgToChar("Рецепт заблокирован от использования.\r\n", d->character.get());
				trec->locked = true;
				OLC_VAL(d) = 1;
			} else if (sagr == "N" || sagr == "n") {
				SendMsgToChar("Рецепт разблокирован и может использоваться.\r\n", d->character.get());
				trec->locked = false;
				OLC_VAL(d) = 1;
			} else {
				SendMsgToChar("Неверный ввод.\r\n", d->character.get());
			}
			mredit_disp_menu(d);
			break;
		}
		case MREDIT_INGR_MENU: {
			// Ввод меню ингридиентов.
			if (sagr == "1") {
				SendMsgToChar("Введите VNUM ингредиента : ", d->character.get());
				OLC_MODE(d) = MREDIT_INGR_PROTO;
				return;
			}

			if (sagr == "2") {
				SendMsgToChar("Введите мин.вес ингредиента : ", d->character.get());
				OLC_MODE(d) = MREDIT_INGR_WEIGHT;
				return;
			}

			if (sagr == "3") {
				SendMsgToChar("Введите мин.силу ингредиента : ", d->character.get());
				OLC_MODE(d) = MREDIT_INGR_POWER;
				return;
			}

			if (sagr == "q") {
				mredit_disp_menu(d);
				return;
			}

			SendMsgToChar("Неверный ввод.\r\n", d->character.get());
			mredit_disp_ingr_menu(d);
			break;
		}
		case MREDIT_INGR_PROTO: {
			i = atoi(sagr.c_str());
			if (i == 0) {
				if (trec->parts[OLC_NUM(d)].proto != i)
					OLC_VAL(d) = 1;
				trec->parts[OLC_NUM(d)].proto = 0;
				trec->parts[OLC_NUM(d)].min_weight = 0;
				trec->parts[OLC_NUM(d)].min_power = 0;
			} else if (GetObjRnum(i) < 0) {
				SendMsgToChar("Прототип выбранного вами ингредиента не существует.\r\n", d->character.get());
			} else {
				trec->parts[OLC_NUM(d)].proto = i;
				OLC_VAL(d) = 1;
			}
			mredit_disp_ingr_menu(d);
			break;
		}
		case MREDIT_INGR_WEIGHT: {
			i = atoi(sagr.c_str());
			trec->parts[OLC_NUM(d)].min_weight = i;
			OLC_VAL(d) = 1;
			mredit_disp_ingr_menu(d);
			break;
		}
		case MREDIT_INGR_POWER: {
			i = atoi(sagr.c_str());
			trec->parts[OLC_NUM(d)].min_power = i;
			OLC_VAL(d) = 1;
			mredit_disp_ingr_menu(d);
			break;
		}
		case MREDIT_CONFIRM_SAVE: {
			if (sagr == "Y" || sagr == "y") {
				SendMsgToChar("Рецепты сохранены.\r\n", d->character.get());
				make_recepts.save();
				make_recepts.load();
				// Очищаем структуры OLC выходим в нормальный режим работы
				cleanup_olc(d, CLEANUP_ALL);
				return;
			} else if (sagr == "N" || sagr == "n") {
				SendMsgToChar("Рецепт не был сохранен.\r\n", d->character.get());
				cleanup_olc(d, CLEANUP_ALL);
				return;
			} else {
				SendMsgToChar("Неверный ввод.\r\n", d->character.get());
			}
			mredit_disp_menu(d);
			break;
		}
	}
}

// Входим в режим редактирования рецептов для предметов.
void do_edit_make(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	string tmpstr;
	DescriptorData *d;
	char tmpbuf[kMaxInputLength];
	MakeRecept *trec;

	// Проверяем не правит ли кто-то рецепты для исключения конфликтов
	for (d = descriptor_list; d; d = d->next) {
		if (d->olc && d->state == EConState::kMredit) {
			sprintf(tmpbuf, "Рецепты в настоящий момент редактируются %s.\r\n", GET_PAD(d->character, 4));
			SendMsgToChar(tmpbuf, ch);
			return;
		}
	}

	argument = one_argument(argument, tmpbuf);
	if (!*tmpbuf) {
		// Номер не задан создаем новый рецепт.
		trec = new(MakeRecept);
		// Запихиваем рецепт но помним что он заблокирован.
		make_recepts.add(trec);
		ch->desc->olc = new olc_data;
		// входим в состояние правки рецепта.
		ch->desc->state = EConState::kMredit;
		OLC_MREC(ch->desc) = trec;
		OLC_VAL(ch->desc) = 0;
		mredit_disp_menu(ch->desc);
		return;
	}

	size_t i = atoi(tmpbuf);
	if ((i > make_recepts.size()) || (i <= 0)) {
		SendMsgToChar("Выбранного рецепта не существует.", ch);
		return;
	}

	i -= 1;
	ch->desc->olc = new olc_data;
	ch->desc->state = EConState::kMredit;
	OLC_MREC(ch->desc) = make_recepts[i];
	mredit_disp_menu(ch->desc);
	return;
}

// Отображение меню параметров ингридиента.
void mredit_disp_ingr_menu(DescriptorData *d) {
	// Рисуем меню ...
	MakeRecept *trec;
	string objname, ingrname, tmpstr;
	char tmpbuf[kMaxInputLength];
	int index = OLC_NUM(d);
	trec = OLC_MREC(d);
	auto tobj = GetObjectPrototype(trec->obj_proto);
	if (trec->obj_proto && tobj) {
		objname = tobj->get_PName(ECase::kNom);
	} else {
		objname = "Нет";
	}
	tobj = GetObjectPrototype(trec->parts[index].proto);
	if (trec->parts[index].proto && tobj) {
		ingrname = tobj->get_PName(ECase::kNom);
	} else {
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
	SendMsgToChar(tmpstr.c_str(), d->character.get());
	OLC_MODE(d) = MREDIT_INGR_MENU;
}

// Отображение главного меню.
void mredit_disp_menu(DescriptorData *d) {
	// Рисуем меню ...
	MakeRecept *trec;
	char tmpbuf[kMaxInputLength];
	string tmpstr, objname, skillname;
	trec = OLC_MREC(d);
	auto tobj = GetObjectPrototype(trec->obj_proto);
	if (trec->obj_proto && tobj) {
		objname = tobj->get_PName(ECase::kNom);
	} else {
		objname = "Нет";
	}
	int i = 0;
	//
	skillname = "Нет";
	while (make_skills[i].num != ESkill::kUndefined) {
		if (make_skills[i].num == trec->skill) {
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
			grn, nrm, yel, skillname.c_str(), to_underlying(trec->skill),
			grn, nrm, yel, (trec->locked ? "Да" : "Нет"));
	tmpstr = string(tmpbuf);
	for (int i = 0; i < MAX_PARTS; i++) {
		tobj = GetObjectPrototype(trec->parts[i].proto);
		if (trec->parts[i].proto && tobj) {
			objname = tobj->get_PName(ECase::kNom);
		} else {
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
	SendMsgToChar(tmpstr.c_str(), d->character.get());
	OLC_MODE(d) = MREDIT_MAIN_MENU;
}

void do_list_make(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	std::string tmpstr, skill_name, obj_name;
	char tmpbuf[kMaxInputLength];
	MakeRecept *trec;
	if (make_recepts.size() == 0) {
		SendMsgToChar("Рецепты в этом мире не определены.", ch);
		return;
	}
	// Выдаем список рецептов всех рецептов как в магазине.
	tmpstr = "###  Б  Умение  Предмет                                 Составляющие:                         \r\n";
	tmpstr +=
		"-------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
	for (size_t i = 0; i < make_recepts.size(); i++) {
		int j = 0;
		skill_name = "Нет";
		obj_name = "Нет";
		trec = make_recepts[i];
		auto obj = GetObjectPrototype(trec->obj_proto);
		if (obj) {
			obj_name = utils::RemoveColors(obj->get_PName(ECase::kNom).substr(0, 39));
		}
		while (make_skills[j].num != ESkill::kUndefined) {
			if (make_skills[j].num == trec->skill) {
				skill_name = string(make_skills[j].short_name);
				break;
			}
			j++;
		}
		sprintf(tmpbuf, "%3zd  %-1s  %-6s  %-40s(%5d) :",
				i + 1, (trec->locked ? "*" : " "), skill_name.c_str(), obj_name.c_str(), trec->obj_proto);
		tmpstr += string(tmpbuf);
		for (int j = 0; j < MAX_PARTS; j++) {
			if (trec->parts[j].proto != 0) {
				obj = GetObjectPrototype(trec->parts[j].proto);
				if (obj) {
					obj_name = utils::RemoveColors(obj->get_PName(ECase::kNom).substr(0, 34));
				} else {
					obj_name = "Нет";
				}
				sprintf(tmpbuf, " %-35s(%5d)", obj_name.c_str(), trec->parts[j].proto);
				if (j > 0) {
					if (j % 2 == 0) {
						// разбиваем строчки если ингров больше 2;
						tmpstr += "\r\n";
						tmpstr += "                                                               ->";
					}
				}
				tmpstr += string(tmpbuf);
			} else {
				break;
			}
		}
		tmpstr += "\r\n";
	}
	page_string(ch->desc, tmpstr);
	return;
}
// Создание любого предмета из компонента.
void do_make_item(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	// Тут творим предмет.
	// Если прислали без параметра то выводим список всех рецептов
	// доступных для изготовления персонажу из его ингров
	// Мастерить можно лук, посох , и диковину(аналог артефакта)
	// суб команда make
	// Выковать можно клинок и доспех (щит) умения разные. название одно
	// Сварить отвар
	// Сшить одежду
	if ((subcmd == MAKE_WEAR) && (!ch->GetSkill(ESkill::kMakeWear))) {
		SendMsgToChar("Вас этому никто не научил.\r\n", ch);
		return;
	}
	string tmpstr;
	MakeReceptList canlist;
	MakeRecept *trec;
	char tmpbuf[kMaxInputLength];
	//int used_skill = subcmd;
	argument = one_argument(argument, tmpbuf);
	// Разбираем в зависимости от того что набрали ... список объектов
	switch (subcmd) {
		case (MAKE_POTION):
			// Варим отвар.
			tmpstr = "Вы можете сварить:\r\n";
			make_recepts.can_make(ch, &canlist, ESkill::kMakePotion);
			break;
		case (MAKE_WEAR):
			// Шьем одежку.
			tmpstr = "Вы можете сшить:\r\n";
			make_recepts.can_make(ch, &canlist, ESkill::kMakeWear);
			break;
		case (MAKE_METALL): tmpstr = "Вы можете выковать:\r\n";
			make_recepts.can_make(ch, &canlist, ESkill::kMakeWeapon);
			make_recepts.can_make(ch, &canlist, ESkill::kMakeArmor);
			break;
		case (MAKE_CRAFT): tmpstr = "Вы можете смастерить:\r\n";
			make_recepts.can_make(ch, &canlist, ESkill::kMakeStaff);
			make_recepts.can_make(ch, &canlist, ESkill::kMakeBow);
			make_recepts.can_make(ch, &canlist, ESkill::kMakeJewel);
			make_recepts.can_make(ch, &canlist, ESkill::kMakeAmulet);
			break;
	}
	if (canlist.size() == 0) {
		// Чар не может сделать ничего.
		SendMsgToChar("Вы ничего не можете сделать.\r\n", ch);
		return;
	}
	if (!*tmpbuf) {
		// Выводим тут список предметов которые можем сделать.
		for (size_t i = 0; i < canlist.size(); i++) {
			auto tobj = GetObjectPrototype(canlist[i]->obj_proto);
			if (!tobj)
				return;
			sprintf(tmpbuf, "%zd) %s\r\n", i + 1, tobj->get_PName(ECase::kNom).c_str());
			tmpstr += string(tmpbuf);
		};
		SendMsgToChar(tmpstr.c_str(), ch);
		return;
	}
	// Адресуемся по списку либо по номеру, либо по названию с номером.
	tmpstr = string(tmpbuf);
	size_t i = atoi(tmpbuf);
	if ((i > 0) && (i <= canlist.size())
		&& (tmpstr.find(".") > tmpstr.size())) {
		trec = canlist[i - 1];
	} else {
		trec = canlist.get_by_name(tmpstr);
		if (trec == nullptr) {
			tmpstr = "Похоже, у вас творческий кризис.\r\n";
			SendMsgToChar(tmpstr.c_str(), ch);
			return;
		}
	};
	trec->make(ch);
	return;
}
void go_create_weapon(CharData *ch, ObjData *obj, int obj_type, ESkill skill) {
	char txtbuff[100];
	const char *to_char = nullptr, *to_room = nullptr;
	int prob, percent, ndice, sdice, weight;
	float average;
	if (obj_type == 5 || obj_type == 6) {
		weight = number(created_item[obj_type].min_weight, created_item[obj_type].max_weight);
		percent = 100;
		prob = 100;
	} else {
		if (!obj) {
			return;
		}
		skill = created_item[obj_type].skill;
		percent = number(1, MUD::Skill(skill).difficulty);
		prob = CalcCurrentSkill(ch, skill, nullptr);
		TrainSkill(ch, skill, true, nullptr);
		weight = MIN(obj->get_weight() - 2, obj->get_weight() * prob / percent);
	}
	if (weight < created_item[obj_type].min_weight) {
		SendMsgToChar("У вас не хватило материала.\r\n", ch);
	} else if (prob * 5 < percent) {
		SendMsgToChar("У вас ничего не получилось.\r\n", ch);
	} else {
		const auto tobj = world_objects.create_from_prototype_by_vnum(created_item[obj_type].obj_vnum);
		if (!tobj) {
			SendMsgToChar("Образец был невозвратимо утерян.\r\n", ch);
		} else {
			tobj->set_weight(MIN(weight, created_item[obj_type].max_weight));
			tobj->set_cost(2 * obj->get_cost() / 3);
			tobj->set_owner(ch->get_uid());
			tobj->set_extra_flag(EObjFlag::kTransformed);
			// ковка объектов со слотами.
			// для 5+ мортов имеем шанс сковать стаф с 3 слотами: базовый 2% и по 0.5% за морт
			// для 2 слотов базовый шанс 5%, 1% за каждый морт
			// для 1 слота базово 20% и 4% за каждый морт
			// Карачун. Поправлено. Расчет не через морты а через скил.
			if (skill == ESkill::kReforging) {
				if (ch->GetSkill(skill) >= 105 && number(1, 100) <= 2 + (ch->GetSkill(skill) - 105) / 10) {
					tobj->set_extra_flag(EObjFlag::kHasThreeSlots);
				} else if (number(1, 100) <= 5 + MAX((ch->GetSkill(skill) - 80), 0) / 5) {
					tobj->set_extra_flag(EObjFlag::kHasTwoSlots);
				} else if (number(1, 100) <= 20 + MAX((ch->GetSkill(skill) - 80), 0) / 5 * 4) {
					tobj->set_extra_flag(EObjFlag::kHasOneSlot);
				}
			}
			switch (obj_type) {
				case 0:    // smith weapon
				case 1:
				case 2:
				case 3:
				case 4:
				case 11: {
					// Карачун. Таймер должен зависить от таймера прототипа.
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// В минимуме один день реала, в максимуме таймер из прототипа
					const int
						timer_value =
						tobj->get_timer() / 100 * ch->GetSkill(skill) - number(0, tobj->get_timer() / 100 * 25);
					const int timer = MAX(ObjData::ONE_DAY, timer_value);
					tobj->set_timer(timer);
					sprintf(buf, "Ваше изделие продержится примерно %d дней\n", tobj->get_timer() / 24 / 60);
					act(buf, false, ch, tobj.get(), 0, kToChar);
					tobj->set_material(obj->get_material());
					// Карачун. Так логичнее.
					// было tobj->get_maximum_durability() = MAX(50, MIN(300, 300 * prob / percent));
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// при расчете числа умножены на 100, перед приравниванием делятся на 100. Для не потерять десятые.
					tobj->set_maximum_durability(
						MAX(20000, 35000 / 100 * ch->GetSkill(skill) - number(0, 35000 / 100 * 25)) / 100);
					tobj->set_current_durability(tobj->get_maximum_durability());
					percent = number(1, MUD::Skill(skill).difficulty);
					prob = CalcCurrentSkill(ch, skill, nullptr);
					ndice = MAX(2, MIN(4, prob / percent));
					ndice += tobj->get_weight() / 10;
					percent = number(1, MUD::Skill(skill).difficulty);
					prob = CalcCurrentSkill(ch, skill, nullptr);
					sdice = MAX(2, MIN(5, prob / percent));
					sdice += tobj->get_weight() / 10;
					tobj->set_val(1, ndice);
					tobj->set_val(2, sdice);
					//			tobj->set_wear_flags(created_item[obj_type].wear); пусть wear флаги будут из прототипа
					if (skill != ESkill::kCreateBow) {
						if (tobj->get_weight() < 14 && percent * 4 > prob) {
							tobj->set_wear_flag(EWearFlag::kHold);
						}
						to_room = "$n выковал$g $o3.";
						average = (((float) sdice + 1) * (float) ndice / 2.0);
						if (average < 3.0) {
							sprintf(txtbuff, "Вы выковали $o3 %s.", create_weapon_quality[(int) (2.5 * 2)]);
						} else if (average <= 27.5) {
							sprintf(txtbuff, "Вы выковали $o3 %s.", create_weapon_quality[(int) (average * 2)]);
						} else {
							sprintf(txtbuff, "Вы выковали $o3 %s!", create_weapon_quality[56]);
						}
						to_char = (char *) txtbuff;
					} else {
						to_room = "$n смастерил$g $o3.";
						to_char = "Вы смастерили $o3.";
					}
					break;
				}
				case 5:    // mages weapon
				case 6: tobj->set_timer(ObjData::ONE_DAY);
					tobj->set_maximum_durability(50);
					tobj->set_current_durability(50);
					ndice = MAX(2, MIN(4, GetRealLevel(ch) / number(6, 8)));
					ndice += (tobj->get_weight() / 10);
					sdice = MAX(2, MIN(5, GetRealLevel(ch) / number(4, 5)));
					sdice += (tobj->get_weight() / 10);
					tobj->set_val(1, ndice);
					tobj->set_val(2, sdice);
					tobj->set_extra_flag(EObjFlag::kNorent);
					tobj->set_extra_flag(EObjFlag::kDecay);
					tobj->set_extra_flag(EObjFlag::kNosell);
					tobj->set_wear_flags(created_item[obj_type].wear);
					to_room = "$n создал$g $o3.";
					to_char = "Вы создали $o3.";
					break;
				case 7:    // smith armor
				case 8:
				case 9:
				case 10: {
					// Карачун. Таймер должен зависить от таймера прототипа.
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// В минимуме один день реала, в максимуме таймер из прототипа
					const int
						timer_value =
						tobj->get_timer() / 100 * ch->GetSkill(skill) - number(0, tobj->get_timer() / 100 * 25);
					const int timer = MAX(ObjData::ONE_DAY, timer_value);
					tobj->set_timer(timer);
					sprintf(buf, "Ваше изделие продержится примерно %d дней\n", tobj->get_timer() / 24 / 60);
					act(buf, false, ch, tobj.get(), 0, kToChar);
					tobj->set_material(obj->get_material());
					// Карачун. Так логичнее.
					// было tobj->get_maximum_durability() = MAX(50, MIN(300, 300 * prob / percent));
					// Формула MAX(<минимум>, <максимум>/100*<процент скила>-<рандом от 0 до 25% максимума>)
					// при расчете числа умножены на 100, перед приравниванием делятся на 100. Для не потерять десятые.
					tobj->set_maximum_durability(
						MAX(20000, 10000 / 100 * ch->GetSkill(skill) - number(0, 15000 / 100 * 25)) / 100);
					tobj->set_current_durability(tobj->get_maximum_durability());
					percent = number(1, MUD::Skill(skill).difficulty);
					prob = CalcCurrentSkill(ch, skill, nullptr);
					ndice = MAX(2, MIN((105 - material_value[tobj->get_material()]) / 10, prob / percent));
					percent = number(1, MUD::Skill(skill).difficulty);
					prob = CalcCurrentSkill(ch, skill, nullptr);
					sdice = MAX(1, MIN((105 - material_value[tobj->get_material()]) / 15, prob / percent));
					tobj->set_val(0, ndice);
					tobj->set_val(1, sdice);
					tobj->set_wear_flags(created_item[obj_type].wear);
					to_room = "$n выковал$g $o3.";
					to_char = "Вы выковали $o3.";
					break;
				}
			} // switch

			if (to_char) {
				act(to_char, false, ch, tobj.get(), 0, kToChar);
			}

			if (to_room) {
				act(to_room, false, ch, tobj.get(), 0, kToRoom);
			}

			if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
				SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
				PlaceObjToRoom(tobj.get(), ch->in_room);
				CheckObjDecay(tobj.get());
			} else if (ch->GetCarryingWeight() + tobj->get_weight() > CAN_CARRY_W(ch)) {
				SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
				PlaceObjToRoom(tobj.get(), ch->in_room);
				CheckObjDecay(tobj.get());
			} else {
				PlaceObjToInventory(tobj.get(), ch);
			}
		}
	}

	if (obj) {
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}
void do_transform_weapon(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *obj = nullptr, *coal, *proto[MAX_PROTO];
	int obj_type, i, found, rnum;

	ESkill skill_id{ESkill::kUndefined};
	switch (subcmd) {
		case SCMD_TRANSFORMWEAPON: skill_id = ESkill::kReforging;
			break;
		case SCMD_CREATEBOW: skill_id = ESkill::kCreateBow;
			break;
		default: break;
	}

	if (ch->IsNpc() || !ch->GetSkill(skill_id)) {
		SendMsgToChar("Вас этому никто не научил.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	if (!*arg1) {
		switch (skill_id) {
			case ESkill::kReforging: SendMsgToChar("Во что вы хотите перековать?\r\n", ch);
				break;
			case ESkill::kCreateBow: SendMsgToChar("Что вы хотите смастерить?\r\n", ch);
				break;
			default: break;
		}
		return;
	}
	obj_type = search_block(arg1, create_item_name, false);
	if (-1 == obj_type) {
		switch (skill_id) {
			case ESkill::kReforging: SendMsgToChar("Перековать можно в :\r\n", ch);
				break;
			case ESkill::kCreateBow: SendMsgToChar("Смастерить можно :\r\n", ch);
				break;
			default: break;
		}
		for (obj_type = 0; *create_item_name[obj_type] != '\n'; obj_type++) {
			if (created_item[obj_type].skill == skill_id) {
				sprintf(buf, "- %s\r\n", create_item_name[obj_type]);
				SendMsgToChar(buf, ch);
			}
		}
		return;
	}
	if (created_item[obj_type].skill != skill_id) {
		switch (skill_id) {
			case ESkill::kReforging: SendMsgToChar("Данный предмет выковать нельзя.\r\n", ch);
				break;
			case ESkill::kCreateBow: SendMsgToChar("Данный предмет смастерить нельзя.\r\n", ch);
				break;
			default: break;
		}
		return;
	}
	for (i = 0; i < MAX_PROTO; proto[i++] = nullptr);
	argument = one_argument(argument, arg2);
	if (!*arg2) {
		SendMsgToChar("Вам нечего перековывать.\r\n", ch);
		return;
	}
	if (!(obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		sprintf(buf, "У Вас нет '%s'.\r\n", arg2);
		SendMsgToChar(buf, ch);
		return;
	}
	if (obj->get_contains()) {
		act("В $o5 что-то лежит.", false, ch, obj, 0, kToChar);
		return;
	}
	if (obj->get_type() == EObjType::kMagicIngredient) {
		for (i = 0, found = false; i < MAX_PROTO; i++) {
			if (GET_OBJ_VAL(obj, 1) == created_item[obj_type].proto[i]) {
				if (proto[i]) {
					found = true;
				} else {
					proto[i] = obj;
					found = false;
					break;
				}
			}
		}
		if (i >= MAX_PROTO) {
			if (found) {
				act("Похоже, вы уже что-то используете вместо $o1.", false, ch, obj, 0, kToChar);
			} else {
				act("Похоже, $o не подойдет для этого.", false, ch, obj, 0, kToChar);
			}
			return;
		}
	} else {
		if (created_item[obj_type].material_bits
			&& !IS_SET(created_item[obj_type].material_bits, (1 << obj->get_material()))) {
			act("$o сделан$G из неподходящего материала.", false, ch, obj, 0, kToChar);
			return;
		}
	}
	switch (skill_id) {
		case ESkill::kReforging:
			// Проверяем повторно из чего сделан объект
			// Чтобы не было абъюза с перековкой из угля.
			if (created_item[obj_type].material_bits &&
				!IS_SET(created_item[obj_type].material_bits, (1 << obj->get_material()))) {
				act("$o сделан$G из неподходящего материала.", false, ch, obj, 0, kToChar);
				return;
			}
			if (!ch->IsImmortal()) {
				if (!ROOM_FLAGGED(ch->in_room, ERoomFlag::kForge)) {
					SendMsgToChar("Вам нужно попасть в кузницу для этого.\r\n", ch);
					return;
				}
				for (coal = ch->carrying; coal; coal = coal->get_next_content()) {
					if (coal->get_type() == EObjType::kMagicIngredient) {
						for (i = 0; i < MAX_PROTO; i++) {
							if (proto[i] == coal) {
								break;
							} else if (!proto[i]
								&& GET_OBJ_VAL(coal, 1) == created_item[obj_type].proto[i]) {
								proto[i] = coal;
								break;
							}
						}
					}
				}
				for (i = 0, found = true; i < MAX_PROTO; i++) {
					if (created_item[obj_type].proto[i]
						&& !proto[i]) {
						rnum = GetObjRnum(created_item[obj_type].proto[i]);
						if (rnum < 0) {
							act("У вас нет необходимого ингредиента.", false, ch, 0, 0, kToChar);
						} else {
							const ObjData obj(*obj_proto[rnum]);
							act("У вас не хватает $o1 для этого.", false, ch, &obj, 0, kToChar);
						}
						found = false;
					}
				}
				if (!found) {
					return;
				}
			}
			for (i = 0; i < MAX_PROTO; i++) {
				if (proto[i] && proto[i] != obj) {
					obj->set_cost(obj->get_cost() + proto[i]->get_cost());
					ExtractObjFromWorld(proto[i]);
				}
			}
			go_create_weapon(ch, obj, obj_type, ESkill::kReforging);
			break;
		case ESkill::kCreateBow:
			for (coal = ch->carrying; coal; coal = coal->get_next_content()) {
				if (coal->get_type() == EObjType::kMagicIngredient) {
					for (i = 0; i < MAX_PROTO; i++) {
						if (proto[i] == coal) {
							break;
						} else if (!proto[i]
							&& GET_OBJ_VAL(coal, 1) == created_item[obj_type].proto[i]) {
							proto[i] = coal;
							break;
						}
					}
				}
			}
			for (i = 0, found = true; i < MAX_PROTO; i++) {
				if (created_item[obj_type].proto[i]
					&& !proto[i]) {
					rnum = GetObjRnum(created_item[obj_type].proto[i]);
					if (rnum < 0) {
						act("У вас нет необходимого ингредиента.", false, ch, 0, 0, kToChar);
					} else {
						const ObjData obj(*obj_proto[rnum]);
						act("У вас не хватает $o1 для этого.", false, ch, &obj, 0, kToChar);
					}
					found = false;
				}
			}
			if (!found) {
				return;
			}
			for (i = 1; i < MAX_PROTO; i++) {
				if (proto[i]) {
					proto[0]->add_weight(proto[i]->get_weight());
					proto[0]->set_cost(proto[0]->get_cost() + proto[i]->get_cost());
					ExtractObjFromWorld(proto[i]);
				}
			}
			go_create_weapon(ch, proto[0], obj_type, ESkill::kCreateBow);
			break;
		default: break;
	}
}
// *****************************
MakeReceptList::MakeReceptList() {
	// Инициализация класса листа .
}
MakeReceptList::~MakeReceptList() {
	// Разрушение списка.
	clear();
}
int
MakeReceptList::load() {
	// Читаем тут файл с рецептом.
	// НАДО ДОБАВИТЬ СОРТИРОВКУ !!!
	char tmpbuf[kMaxInputLength];
	string tmpstr;
	// чистим список рецептов от старых данных.
	clear();
	MakeRecept *trec;
	ifstream bifs(LIB_MISC "makeitems.lst");
	if (!bifs) {
		sprintf(tmpbuf, "MakeReceptList:: Unable open input file !!!");
		mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
		return (false);
	}
	while (!bifs.eof()) {
		bifs.getline(tmpbuf, kMaxInputLength, '\n');
		tmpstr = string(tmpbuf);
		// пропускаем закомменированные строчки.
		if (tmpstr.substr(0, 2) == "//")
			continue;
//    cout << "Get str from file :" << tmpstr << endl;
		if (tmpstr.size() == 0)
			continue;
		trec = new(MakeRecept);
		if (trec->load_from_str(tmpstr)) {
			recepts.push_back(trec);
		} else {
			delete trec;
			// ошибка вытаскивания из строки написать
			sprintf(tmpbuf, "MakeReceptList:: Fail get recept from line.");
			mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
		}
	}
	return true;
}
int MakeReceptList::save() {
	// Пишем тут файл с рецептом.
	// Очищаем список
	string tmpstr;
	char tmpbuf[kMaxInputLength];
	list<MakeRecept *>::iterator p;
	ofstream bofs(LIB_MISC "makeitems.lst");
	if (!bofs) {
		// cout << "Unable input stream to create !!!" << endl;
		sprintf(tmpbuf, "MakeReceptList:: Unable create output file !!!");
		mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
		return (false);
	}
	sort();
	p = recepts.begin();
	while (p != recepts.end()) {
		if ((*p)->save_to_str(tmpstr))
			bofs << tmpstr << endl;
		p++;
	}
	bofs.close();
	return 0;
}
void MakeReceptList::add(MakeRecept *recept) {
	recepts.push_back(recept);
	return;
}
void MakeReceptList::del(MakeRecept *recept) {
	recepts.remove(recept);
	return;
}
bool by_skill(MakeRecept *const &lhs, MakeRecept *const &rhs) {
	return ((lhs->skill) > (rhs->skill));
}
void MakeReceptList::sort() {
	// Сделать сортировку по умениям.
	recepts.sort(by_skill);
	return;
}
size_t MakeReceptList::size() {
	return recepts.size();
}
void MakeReceptList::clear() {
	// Очищаем список
	list<MakeRecept *>::iterator p;
	p = recepts.begin();
	while (p != recepts.end()) {
		delete (*p);
		p++;
	}
	recepts.erase(recepts.begin(), recepts.end());
	return;
}
MakeRecept *MakeReceptList::operator[](size_t i) {
	list<MakeRecept *>::iterator p = recepts.begin();
	size_t j = 0;
	while (p != recepts.end()) {
		if (i == j) {
			return (*p);
		}
		j++;
		p++;
	}
	return (nullptr);
}
MakeRecept *MakeReceptList::get_by_name(string &rname) {
	// Ищем по списку рецепт с таким именем.
	// Ищем по списку рецепты которые чар может изготовить.
	list<MakeRecept *>::iterator p = recepts.begin();
	int k = 1;    // count
	// split string by '.' character and convert first part into number (store to k)
	size_t i = rname.find(".");
	if (std::string::npos != i)    // TODO: Check me.
	{
		// Строка не найдена.
		if (0 < i) {
			k = atoi(rname.substr(0, i).c_str());
			if (k <= 0) {
				return nullptr;    // Если ввели -3.xxx
			}
		}
		rname = rname.substr(i + 1);
	}
	int j = 0;
	while (p != recepts.end()) {
		auto tobj = GetObjectPrototype((*p)->obj_proto);
		if (!tobj) {
			return 0;
		}
		std::string tmpstr = tobj->get_aliases() + " ";
		while (int(tmpstr.find(' ')) > 0) {
			if ((tmpstr.substr(0, tmpstr.find(' '))).find(rname) == 0) {
				if ((k - 1) == j) {
					return *p;
				}
				j++;
				break;
			}
			tmpstr = tmpstr.substr(tmpstr.find(' ') + 1);
		}
		p++;
	}
	return nullptr;
}
MakeReceptList *MakeReceptList::can_make(CharData *ch, MakeReceptList *canlist, ESkill use_skill) {
	// Ищем по списку рецепты которые чар может изготовить.
	list<MakeRecept *>::iterator p;
	p = recepts.begin();
	while (p != recepts.end()) {
		if (((*p)->skill == use_skill) && ((*p)->can_make(ch))) {
			MakeRecept *trec = new MakeRecept;
			*trec = **p;
			canlist->add(trec);
		}
		p++;
	}
	return (canlist);
}
ObjData *get_obj_in_list_ingr(int num,
							  ObjData *list) //Ингридиентом является или сам прототип с VNUM или альтернатива с VALUE 1 равным внум прототипа
{
	ObjData *i;
	for (i = list; i; i = i->get_next_content()) {
		if (GET_OBJ_VNUM(i) == num) {
			return i;
		}
		if ((GET_OBJ_VAL(i, 1) == num)
			&& (i->get_type() == EObjType::kMagicIngredient
				|| i->get_type() == EObjType::kMagicComponent
				|| i->get_type() == EObjType::kCraftMaterial)) {
			return i;
		}
	}
	return nullptr;
}
MakeRecept::MakeRecept() : skill(ESkill::kUndefined) {
	locked = true;        // по умолчанию рецепт залочен.
	obj_proto = 0;
	for (int i = 0; i < MAX_PARTS; i++) {
		parts[i].proto = 0;
		parts[i].min_weight = 0;
		parts[i].min_power = 0;
	}
}
int MakeRecept::can_make(CharData *ch) {
	int i;
	ObjData *ingrobj = nullptr;
	// char tmpbuf[kMaxInputLength];
	// Сделать проверку на поле locked
	if (!ch)
		return (false);
	if (locked)
		return (false);
	// Сделать проверку наличия скилла у игрока.
	if (ch->IsNpc() || !ch->GetSkill(skill)) {
		return (false);
	}
	// Делаем проверку может ли чар сделать предмет такого типа
	if (skill == ESkill::kMakeStaff) {
		return 0;
	}
	for (i = 0; i < MAX_PARTS; i++) {
		if (parts[i].proto == 0) {
			break;
		}
		if (GetObjRnum(parts[i].proto) < 0)
			return (false);
		//SendMsgToChar("Образец был невозвратимо утерян.\r\n",ch); //леший знает чего тут надо писать
		if (!(ingrobj = get_obj_in_list_ingr(parts[i].proto, ch->carrying))) {
			//sprintf(tmpbuf,"Для '%d' у вас нет '%d'.\r\n",obj_proto,parts[i].proto);
			//SendMsgToChar(tmpbuf,ch);
			return (false);
		}
		int ingr_lev = get_ingr_lev(ingrobj);
		// Если чар ниже уровня ингридиента то он не может делать рецепты с его
		// участием.
		if (!ch->IsImpl() && (ingr_lev > (GetRealLevel(ch) + 2 * GetRealRemort(ch)))) {
			SendMsgToChar("Вы слишком малого уровня и вам что-то не подходит для шитья.\r\n", ch);
			return (false);
		}
	}
	return (true);
}

int MakeRecept::get_ingr_lev(ObjData *ingrobj) {
	// Получаем уровень ингредиента ...
	if (ingrobj->get_type() == EObjType::kMagicIngredient) {
		// Получаем уровень игредиента до 128
		return (GET_OBJ_VAL(ingrobj, 0) >> 8);
	} else if (ingrobj->get_type() == EObjType::kMagicComponent) {
		// У ингров типа 26 совпадает уровень и сила.
		return GET_OBJ_VAL(ingrobj, IM_POWER_SLOT);
	} else if (ingrobj->get_type() == EObjType::kCraftMaterial) {
		return GET_OBJ_VAL(ingrobj, 0);
	} else {
		return -1;
	}
}

int MakeRecept::get_ingr_pow(ObjData *ingrobj) {
	// Получаем силу ингредиента ...
	if (ingrobj->get_type() == EObjType::kMagicIngredient
		|| ingrobj->get_type() == EObjType::kCraftMaterial) {
		return GET_OBJ_VAL(ingrobj, 2);
	} else if (ingrobj->get_type() == EObjType::kMagicComponent) {
		return GET_OBJ_VAL(ingrobj, IM_POWER_SLOT);
	} else {
		return -1;
	}
}
void MakeRecept::make_value_wear(CharData *ch, ObjData *obj, ObjData *ingrs[MAX_PARTS]) {
	int wearkoeff = 50;
	if (CAN_WEAR(obj, EWearFlag::kBody)) // 1.1
	{
		wearkoeff = 110;
	}
	if (CAN_WEAR(obj, EWearFlag::kHead)) //0.8
	{
		wearkoeff = 80;
	}
	if (CAN_WEAR(obj, EWearFlag::kLegs)) //0.5
	{
		wearkoeff = 50;
	}
	if (CAN_WEAR(obj, EWearFlag::kFeet)) //0.6
	{
		wearkoeff = 60;
	}
	if (CAN_WEAR(obj, EWearFlag::kArms)) //0.5
	{
		wearkoeff = 50;
	}
	if (CAN_WEAR(obj, EWearFlag::kShoulders))//0.9
	{
		wearkoeff = 90;
	}
	if (CAN_WEAR(obj, EWearFlag::kHands))//0.45
	{
		wearkoeff = 45;
	}
	obj->set_val(0,
				 ((GetRealInt(ch) * GetRealInt(ch) / 10 + ch->GetSkill(ESkill::kMakeWear)) / 100
					 + (GET_OBJ_VAL(ingrs[0], 3) + 1)) * wearkoeff
					 / 100); //АС=((инта*инта/10+умелка)/100+левл.шкуры)*коэф.части тела
	if (CAN_WEAR(obj, EWearFlag::kBody)) //0.9
	{
		wearkoeff = 90;
	}
	if (CAN_WEAR(obj, EWearFlag::kHead)) //0.7
	{
		wearkoeff = 70;
	}
	if (CAN_WEAR(obj, EWearFlag::kLegs)) //0.4
	{
		wearkoeff = 40;
	}
	if (CAN_WEAR(obj, EWearFlag::kFeet)) //0.5
	{
		wearkoeff = 50;
	}
	if (CAN_WEAR(obj, EWearFlag::kArms)) //0.4
	{
		wearkoeff = 40;
	}
	if (CAN_WEAR(obj, EWearFlag::kShoulders))//0.8
	{
		wearkoeff = 80;
	}
	if (CAN_WEAR(obj, EWearFlag::kHands))//0.31
	{
		wearkoeff = 31;
	}
	obj->set_val(1,
				 (ch->GetSkill(ESkill::kMakeWear) / 25 + (GET_OBJ_VAL(ingrs[0], 3) + 1)) * wearkoeff
					 / 100); //броня=(%умелки/25+левл.шкуры)*коэф.части тела
}
float MakeRecept::count_mort_requred(ObjData *obj) {
	float result = 0.0;
	const float SQRT_MOD = 1.7095f;
	const int AFF_SHIELD_MOD = 30;
	const int AFF_MAGICGLASS_MOD = 10;
	const int AFF_BLINK_MOD = 10;
	const int AFF_CLOUDLY_MOD = 10;

	result = 0.0;

	float total_weight = 0.0;

	// аффекты APPLY_x
	for (int k = 0; k < kMaxObjAffect; k++) {
		if (obj->get_affected(k).location == 0) continue;

		// случай, если один аффект прописан в нескольких полях
		for (int kk = 0; kk < kMaxObjAffect; kk++) {
			if (obj->get_affected(k).location == obj->get_affected(kk).location
				&& k != kk) {
				log("SYSERROR: double affect=%d, ObjVnum=%d",
					obj->get_affected(k).location, GET_OBJ_VNUM(obj));
				return 1000000;
			}
		}
		if ((obj->get_affected(k).modifier > 0) && ((obj->get_affected(k).location != EApply::kAc) &&
			(obj->get_affected(k).location != EApply::kSavingWill) &&
			(obj->get_affected(k).location != EApply::kSavingCritical) &&
			(obj->get_affected(k).location != EApply::kSavingStability) &&
			(obj->get_affected(k).location != EApply::kSavingReflex))) {
			float weight = count_affect_weight(obj->get_affected(k).location, obj->get_affected(k).modifier);
			log("SYSERROR: negative weight=%f, ObjVnum=%d",
				weight, GET_OBJ_VNUM(obj));
			total_weight += pow(weight, SQRT_MOD);
		}
			// савесы которые с минусом должны тогда понижать вес если в +
		else if ((obj->get_affected(k).modifier > 0) && ((obj->get_affected(k).location == EApply::kAc) ||
			(obj->get_affected(k).location == EApply::kSavingWill) ||
			(obj->get_affected(k).location == EApply::kSavingCritical) ||
			(obj->get_affected(k).location == EApply::kSavingStability) ||
			(obj->get_affected(k).location == EApply::kSavingReflex))) {
			float weight = count_affect_weight(obj->get_affected(k).location, 0 - obj->get_affected(k).modifier);
			total_weight -= pow(weight, -SQRT_MOD);
		}
			//Добавленый кусок учет савесов с - значениями
		else if ((obj->get_affected(k).modifier < 0)
			&& ((obj->get_affected(k).location == EApply::kAc) ||
				(obj->get_affected(k).location == EApply::kSavingWill) ||
				(obj->get_affected(k).location == EApply::kSavingCritical) ||
				(obj->get_affected(k).location == EApply::kSavingStability) ||
				(obj->get_affected(k).location == EApply::kSavingReflex))) {
			float weight = count_affect_weight(obj->get_affected(k).location, obj->get_affected(k).modifier);
			total_weight += pow(weight, SQRT_MOD);
		}
			//Добавленый кусок учет отрицательного значения но не савесов
		else if ((obj->get_affected(k).modifier < 0)
			&& ((obj->get_affected(k).location != EApply::kAc) &&
				(obj->get_affected(k).location != EApply::kSavingWill) &&
				(obj->get_affected(k).location != EApply::kSavingCritical) &&
				(obj->get_affected(k).location != EApply::kSavingStability) &&
				(obj->get_affected(k).location != EApply::kSavingReflex))) {
			float weight = count_affect_weight(obj->get_affected(k).location, 0 - obj->get_affected(k).modifier);
			total_weight -= pow(weight, -SQRT_MOD);
		}
	}
	// аффекты AFF_x через weapon_affect
	for (const auto &m : weapon_affect) {
		if (obj->GetEWeaponAffect(m.aff_pos)) {
			if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kAirShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kFireShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kIceShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kMagicGlass) {
				total_weight += pow(AFF_MAGICGLASS_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kBlink) {
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kCloudly) {
				total_weight += pow(AFF_CLOUDLY_MOD, SQRT_MOD);
			}
		}
	}
	if (total_weight < 1) return result;

	result = ceil(pow(total_weight, 1 / SQRT_MOD));

	return result;
}

float MakeRecept::count_affect_weight(int num, int mod) {
	float weight = 0;

	switch (num) {
		case EApply::kStr: weight = mod * 7.5;
			break;
		case EApply::kDex: weight = mod * 10.0;
			break;
		case EApply::kInt: weight = mod * 10.0;
			break;
		case EApply::kWis: weight = mod * 10.0;
			break;
		case EApply::kCon: weight = mod * 10.0;
			break;
		case EApply::kCha: weight = mod * 10.0;
			break;
		case EApply::kHp: weight = mod * 0.3;
			break;
		case EApply::kAc: weight = mod * -0.5;
			break;
		case EApply::kHitroll: weight = mod * 2.3;
			break;
		case EApply::kDamroll: weight = mod * 3.3;
			break;
		case EApply::kSavingWill: weight = mod * -0.5;
			break;
		case EApply::kSavingCritical: weight = mod * -0.5;
			break;
		case EApply::kSavingStability: weight = mod * -0.5;
			break;
		case EApply::kSavingReflex: weight = mod * -0.5;
			break;
		case EApply::kCastSuccess: weight = mod * 1.5;
			break;
		case EApply::kManaRegen: weight = mod * 0.2;
			break;
		case EApply::kMorale: weight = mod * 1.0;
			break;
		case EApply::kInitiative: weight = mod * 2.0;
			break;
		case EApply::kAbsorbe: weight = mod * 1.0;
			break;
		case EApply::kAffectResist: weight = mod * 1.5;
			break;
		case EApply::kMagicResist: weight = mod * 1.5;
			break;
	}

	return weight;
}
void MakeRecept::make_object(CharData *ch, ObjData *obj, ObjData *ingrs[MAX_PARTS], int ingr_cnt) {
	int i, j;
	//ставим именительные именительные падежи в алиасы
	sprintf(buf, "%s %s %s %s",
			obj->get_PName(ECase::kNom).c_str(),
			ingrs[0]->get_PName(ECase::kGen).c_str(),
			ingrs[1]->get_PName(ECase::kIns).c_str(),
			ingrs[2]->get_PName(ECase::kIns).c_str());
	obj->set_aliases(buf);
	for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) // ставим падежи в имя с учетов ингров
	{
		auto name_case = static_cast<ECase>(i);
		sprintf(buf, "%s", obj->get_PName(name_case).c_str());
		strcat(buf, " из ");
		strcat(buf, ingrs[0]->get_PName(ECase::kGen).c_str());
		strcat(buf, " с ");
		strcat(buf, ingrs[1]->get_PName(ECase::kIns).c_str());
		strcat(buf, " и ");
		strcat(buf, ingrs[2]->get_PName(ECase::kIns).c_str());
		obj->set_PName(name_case, buf);
		if (i == 0) // именительный падеж
		{
			obj->set_short_description(buf);
			if (GET_OBJ_SEX(obj) == EGender::kMale) {
				snprintf(buf2, kMaxStringLength, "Брошенный %s лежит тут.", buf);
			} else if (GET_OBJ_SEX(obj) == EGender::kFemale) {
				snprintf(buf2, kMaxStringLength, "Брошенная %s лежит тут.", buf);
			} else if (GET_OBJ_SEX(obj) == EGender::kPoly) {
				snprintf(buf2, kMaxStringLength, "Брошенные %s лежат тут.", buf);
			}
			obj->set_description(buf2); // описание на земле
		}
	}
	obj->set_is_rename(true); // ставим флаг что объект переименован


	auto temp_flags = obj->get_affect_flags();
	add_flags(ch, &temp_flags, &ingrs[0]->get_affect_flags(), get_ingr_pow(ingrs[0]));
	obj->SetWeaponAffectFlags(temp_flags);
	// перносим эффекты ... с ингров на прототип, 0 объект шкура переносим все, с остальных 1 рандом
	temp_flags = obj->get_extra_flags();
	add_flags(ch, &temp_flags, &ingrs[0]->get_extra_flags(), get_ingr_pow(ingrs[0]));
	obj->set_extra_flags(temp_flags);
	auto temp_affected = obj->get_all_affected();
	add_affects(ch, temp_affected, ingrs[0]->get_all_affected(), get_ingr_pow(ingrs[0]));
	obj->set_all_affected(temp_affected);
	add_rnd_skills(ch, ingrs[0], obj); //переносим случайную умелку со шкуры
	obj->set_extra_flag(EObjFlag::kNoalter);  // нельзя сфрешить черным свитком
	obj->set_timer((GET_OBJ_VAL(ingrs[0], 3) + 1) * 1000
					   + ch->GetSkill(ESkill::kMakeWear) / 2 * number(160, 220)); // таймер зависит в основном от умелки
	obj->set_craft_timer(obj->get_timer()); // запомним таймер созданной вещи для правильного отображения при осм для ее сост.
	for (j = 1; j < ingr_cnt; j++) {
		int i, raffect = 0;
		for (i = 0; i < kMaxObjAffect; i++) // посмотрим скока аффектов
		{
			if (ingrs[j]->get_affected(i).location == EApply::kNone) {
				break;
			}
		}
		if (i > 0) // если > 0 переносим случайный
		{
			raffect = number(0, i - 1);
			for (int i = 0; i < kMaxObjAffect; i++) {
				const auto &ra = ingrs[j]->get_affected(raffect);
				if (obj->get_affected(i).location
					== ra.location) // если аффект такой уже висит и он меньше, переставим значение
				{
					if (obj->get_affected(i).modifier < ra.modifier) {
						obj->set_affected(i, obj->get_affected(i).location, ra.modifier);
					}
					break;
				}
				if (obj->get_affected(i).location == EApply::kNone) // добавляем афф на свободное место
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
		obj->SetWeaponAffectFlags(temp_flags);
		// перносим эффекты ... с ингров на прототип.
		temp_flags = obj->get_extra_flags();
		add_flags(ch, &temp_flags, &ingrs[j]->get_extra_flags(), get_ingr_pow(ingrs[j]));
		obj->set_extra_flags(temp_flags);
		// переносим 1 рандом аффект
		add_rnd_skills(ch, ingrs[j], obj); //переноси случайную умелку с ингров
	}
}
static std::string craft_recipe_name(int recipe_id) {
	const auto proto = GetObjectPrototype(recipe_id);
	if (proto) {
		return proto->get_PName(ECase::kNom); // KOI8-R; auto-converted by OtelMetrics
	}
	return std::to_string(recipe_id);
}

static void record_craft_failure(int recipe_id, ESkill skill) {
	observability::OtelMetrics::RecordCounter("craft.failures.total", 1, {
		{"recipe_name",    craft_recipe_name(recipe_id)},
		{"skill",          MUD::Skill(skill).GetName()}, // KOI8-R; auto-converted by OtelMetrics
		{"failure_reason", "craft_failed"}
	});
}

static void record_craft_success(int recipe_id, ESkill skill) {
	observability::OtelMetrics::RecordCounter("craft.completed.total", 1, {
		{"recipe_name", craft_recipe_name(recipe_id)},
		{"skill",       MUD::Skill(skill).GetName()} // KOI8-R; auto-converted by OtelMetrics
	});
}

// создать предмет по рецепту
int MakeRecept::make(CharData *ch) {
	char tmpbuf[kMaxStringLength];//, tmpbuf2[kMaxStringLength];
	ObjData *ingrs[MAX_PARTS];
	string tmpstr, charwork, roomwork, charfail, roomfail, charsucc, roomsucc, chardam, roomdam, tagging, itemtag;
	int dam = 0;
	bool make_fail;
	// 1. Проверить есть ли скилл у чара
	if (ch->IsNpc() || !ch->GetSkill(skill)) {
		SendMsgToChar("Странно что вам вообще пришло в голову cделать это.\r\n", ch);
		return (false);
	}
	// 2. Проверить есть ли ингры у чара
	if (!can_make(ch)) {
		SendMsgToChar("У вас нет составляющих для этого.\r\n", ch);
		return (false);
	}
	if (ch->get_move() < MIN_MAKE_MOVE) {
		SendMsgToChar("Вы слишком устали и вам ничего не хочется делать.\r\n", ch);
		return (false);
	}
	auto tobj = GetObjectPrototype(obj_proto);
	if (!tobj) {
		return 0;
	}
	// Проверяем возможность создания предмета
	if (!ch->IsImmortal() && (skill == ESkill::kMakeStaff)) {
		const ObjData obj(*tobj);
		act("Вы не готовы к тому чтобы сделать $o3.", false, ch, &obj, 0, kToChar);
		return (false);
	}
	// Прогружаем в массив реальные ингры
	// 3. Проверить уровни ингров и чара
	int ingr_cnt = 0, ingr_lev, i, craft_weight, ingr_pow;
	for (i = 0; i < MAX_PARTS; i++) {
		if (parts[i].proto == 0)
			break;
		ingrs[i] = get_obj_in_list_ingr(parts[i].proto, ch->carrying);
		ingr_lev = get_ingr_lev(ingrs[i]);
		if (!ch->IsImpl() && (ingr_lev > (GetRealLevel(ch) + 2 * GetRealRemort(ch)))) {
			tmpstr = "Вы побоялись испортить " + ingrs[i]->get_PName(ECase::kAcc)
				+ "\r\n и прекратили работу над " + tobj->get_PName(ECase::kIns) + ".\r\n";
			SendMsgToChar(tmpstr.c_str(), ch);
			return (false);
		};
		ingr_pow = get_ingr_pow(ingrs[i]);
		if (ingr_pow < parts[i].min_power) {
			tmpstr = "$o не подходит для изготовления " + tobj->get_PName(ECase::kGen) + ".";
			act(tmpstr.c_str(), false, ch, ingrs[i], 0, kToChar);
			return (false);
		}
		ingr_cnt++;
	}
	//int stat_bonus;
	// Делаем всякие доп проверки для различных умений.
	switch (skill) {
		case ESkill::kMakeWeapon:
		case ESkill::kMakeArmor:
			// Проверяем есть ли тут наковальня или комната кузня.
			if ((!ROOM_FLAGGED(ch->in_room, ERoomFlag::kForge)) && (!ch->IsImmortal())) {
				SendMsgToChar("Вам нужно попасть в кузницу для этого.\r\n", ch);
				return (false);
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
			//stat_bonus = number(0, GetRealStr(ch));
			break;
		case ESkill::kMakeBow: charwork = "Вы начали мастерить $o3.";
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
			//stat_bonus = number(0, GetRealDex(ch));
			dam = 40;
			break;
		case ESkill::kMakeWear: charwork = "Вы взяли в руку иголку и начали шить $o3.";
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
			//stat_bonus = number(0, GetRealCon(ch));
			dam = 30;
			break;
		case ESkill::kMakeAmulet: charwork = "Вы взяли в руки необходимые материалы и начали мастерить $o3.";
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
		case ESkill::kMakeJewel: charwork = "Вы начали мастерить $o3.";
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
			//stat_bonus = number(0, GetRealCha(ch));
			dam = 30;
			break;
		case ESkill::kMakeStaff: charwork = "Вы начали мастерить $o3.";
			roomwork = "$n начал мастерить что-то, посылая всех к чертям.";
			charfail = "$o3 осветил комнату магическим светом и истаял.";
			roomfail = "Предмет в руках $n1 вспыхнул, озарив комнату магическим светом и истаял.";
			charsucc =
				"Тайные знаки нанесенные на $o3 вспыхнули и погасли.\r\nДа, $E хорошо послужит своему хозяину.";
			roomsucc = "$n смастерил$g $o3. Вы почувствовали скрытую силу спрятанную в этом предмете.";
			chardam = "$o взорвался в ваших руках. Вас сильно обожгло.";
			roomdam = "$o взорвался в руках $n1, опалив его.\r\nВокруг приятно запахло жаренным мясом.";
			tagging = "Вы начертили на $o2 свое имя.";
			itemtag = "Среди рунных знаков видна надпись 'Создано $n4'.";
			// Бонус ум.
			//stat_bonus = number(0, GetRealInt(ch));
			dam = 70;
			break;
		case ESkill::kMakePotion: charwork = "Вы достали небольшой горшочек и развели под ним огонь, начав варить $o3.";
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
			//stat_bonus = number(0, GetRealWis(ch));
			dam = 40;
			break;
		default: break;
	}
	const ObjData object(*tobj);
	act(charwork.c_str(), false, ch, &object, 0, kToChar);
	act(roomwork.c_str(), false, ch, &object, 0, kToRoom);
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
	for (i = 0; i < ingr_cnt; i++) {
		ingr_lev = get_ingr_lev(ingrs[i]);
		if (ingr_lev < 0) {
			used_non_ingrs++;
		} else {
			created_lev += ingr_lev;
		}
		// Шанс испортить не ингредиент всетаки есть.
		if ((number(0, 30) < (5 + ingr_lev - GetRealLevel(ch) - 2 * GetRealRemort(ch))) && !ch->IsImpl()) {
			tmpstr = "Вы испортили " + ingrs[i]->get_PName(ECase::kAcc) + ".\r\n";
			SendMsgToChar(tmpstr.c_str(), ch);
			//extract_obj(ingrs[i]); //заменим на обнуление веса
			//чтобы не крешило дальше в обработке фейла (Купала)
			IS_CARRYING_W(ch) -= ingrs[i]->get_weight();
			ingrs[i]->set_weight(0);
			make_fail = true;
		}
	};
	created_lev = created_lev / MAX(1, (ingr_cnt - used_non_ingrs));
	int j;
	int craft_move = MIN_MAKE_MOVE + (created_lev / 2) - 1;
	// Снимаем мувы за умение
	if (ch->get_move() < craft_move) {
		ch->set_move(0);
		// Вам не хватило сил доделать.
		tmpstr = "Вам не хватило сил доделать " + tobj->get_PName(ECase::kAcc) + ".\r\n";
		SendMsgToChar(tmpstr.c_str(), ch);
		make_fail = true;
	} else {
		if (!ch->IsImpl()) {
			ch->set_move(ch->get_move() - craft_move);
		}
	}

	TrainSkill(ch, skill, !make_fail, nullptr);
	// 4. Считаем сколько материала треба.
	if (!make_fail) {
		for (i = 0; i < ingr_cnt; i++) {
			if (skill == ESkill::kMakeWear && i == 0) //для шитья всегда раскраиваем шкуру
			{
				IS_CARRYING_W(ch) -= ingrs[0]->get_weight();
				ingrs[0]->set_weight(0);  // шкуру дикеим полностью
				tmpstr = "Вы раскроили полностью " + ingrs[0]->get_PName(ECase::kAcc) + ".\r\n";
				SendMsgToChar(tmpstr.c_str(), ch);
				continue;
			}
			//
			// нужный материал = мин.материал +
			// random(100) - skill
			// если она < 20 то мин.вес + rand(мин.вес/3)
			// если она < 50 то мин.вес*rand(1,2) + rand(мин.вес/3)
			// если она > 50    мин.вес*rand(2,5) + rand(мин.вес/3)
			if (get_ingr_lev(ingrs[i]) == -1)
				continue;    // Компонент не ингр. пропускаем.
			craft_weight = parts[i].min_weight + number(0, (parts[i].min_weight / 3) + 1);
			j = number(0, 100) - CalcCurrentSkill(ch, skill, 0);
			if ((j >= 20) && (j < 50))
				craft_weight += parts[i].min_weight * number(1, 2);
			else if (j > 50)
				craft_weight += parts[i].min_weight * number(2, 5);
			// 5. Делаем проверку есть ли столько материала.
			// если не хватает то удаляем игридиент и фейлим.
			int state = craft_weight;
			// Обсчет веса ингров в цикле, если не хватило веса берем следующий ингр в инве, если не хватает, делаем фэйл (make_fail) и брекаем внешний цикл, смысл дальше ингры смотреть?
			//SendMsgToChar(ch, "Требуется вес %d вес ингра %d требуемое кол ингров %d\r\n", state, ingrs[i]->get_weight(), ingr_cnt);
			int obj_vnum_tmp = GET_OBJ_VNUM(ingrs[i]);
			while (state > 0) {
				//Переделаем слегка логику итераций
				//Сперва проверяем сколько нам нужно. Если вес ингра больше, чем требуется, то вычитаем вес и останавливаем итерацию.
				if (ingrs[i]->get_weight() > state) {
					ingrs[i]->sub_weight(state);
					SendMsgToChar(ch, "Вы использовали %s.\r\n", ingrs[i]->get_PName(ECase::kAcc).c_str());
					IS_CARRYING_W(ch) -= state;
					break;
				}
					//Если вес ингра ровно столько, сколько требуется, вычитаем вес, ломаем ингр и останавливаем итерацию.
				else if (ingrs[i]->get_weight() == state) {
					IS_CARRYING_W(ch) -= ingrs[i]->get_weight();
					ingrs[i]->set_weight(0);
					SendMsgToChar(ch, "Вы полностью использовали %s.\r\n", ingrs[i]->get_PName(ECase::kAcc).c_str());
					//extract_obj(ingrs[i]);
					break;
				}
					//Если вес ингра меньше, чем требуется, то вычтем этот вес из того, сколько требуется.
				else {
					state = state - ingrs[i]->get_weight();
					SendMsgToChar(ch,
								  "Вы полностью использовали %s и начали искать следующий ингредиент.\r\n",
								  ingrs[i]->get_PName(ECase::kAcc).c_str());
					std::string tmpname = std::string(ingrs[i]->get_PName(ECase::kGen).c_str());
					IS_CARRYING_W(ch) -= ingrs[i]->get_weight();
					ingrs[i]->set_weight(0);
					ExtractObjFromWorld(ingrs[i]);
					ingrs[i] = nullptr;
					//Если некст ингра в инве нет, то сообщаем об этом и идем в фэйл. Некст ингры все равно проверяем
					if (!get_obj_in_list_ingr(obj_vnum_tmp, ch->carrying)) {
						SendMsgToChar(ch, "У вас в инвентаре больше нет %s.\r\n", tmpname.c_str());
						make_fail = true;
						break;
					}
						//Подцепляем некст ингр и идем в нашу проверку заново
					else {
						ingrs[i] = get_obj_in_list_ingr(obj_vnum_tmp, ch->carrying);
					}
				}
			}
		}
	}
	if (make_fail) {
		// Считаем крит фейл или нет.
		int crit_fail = number(0, 100);
		if (crit_fail > 2) {
			const ObjData obj(*tobj);
			act(charfail.c_str(), false, ch, &obj, 0, kToChar);
			act(roomfail.c_str(), false, ch, &obj, 0, kToRoom);
		} else {
			const ObjData obj(*tobj);
			act(chardam.c_str(), false, ch, &obj, 0, kToChar);
			act(roomdam.c_str(), false, ch, &obj, 0, kToRoom);
			dam = number(0, dam);
			// Наносим дамаг.
			if (GetRealLevel(ch) >= kLvlImmortal && dam > 0) {
				SendMsgToChar("Будучи бессмертным, вы избежали повреждения...", ch);
				return (false);
			}
			ch->set_hit(ch->get_hit() - dam);
			update_pos(ch);
			char_dam_message(dam, ch, ch, 0);
			if (ch->GetPosition() == EPosition::kDead) {
				// Убился веником.
				if (!ch->IsNpc()) {
					sprintf(tmpbuf, "%s killed by a crafting at %s",
							GET_NAME(ch),
							ch->in_room == kNowhere ? "kNowhere" : world[ch->in_room]->name);
					mudlog(tmpbuf, BRF, kLvlBuilder, SYSLOG, true);
				}
				die(ch, nullptr);
			}
		}
		for (i = 0; i < ingr_cnt; i++) {
			if (ingrs[i] && ingrs[i]->get_weight() <= 0) {
				ExtractObjFromWorld(ingrs[i]);
			}
		}

		record_craft_failure(obj_proto, skill);
		return (false);
	}
	// Лоадим предмет игроку
	const auto obj = world_objects.create_from_prototype_by_vnum(obj_proto);
	act(charsucc.c_str(), false, ch, obj.get(), 0, kToChar);
	act(roomsucc.c_str(), false, ch, obj.get(), 0, kToRoom);
	// 6. Считаем базовые статсы предмета и таймер
	//  формула для каждого умения отдельная
	// Для числовых х-к:  х-ка+(skill - random(100))/20;
	// Для флагов ???: random(200) - skill > 0 то флаг переноситься.
	// Т.к. сделать мы можем практически любой предмет.
	// Модифицируем вес предмета и его таймер.
	// Для маг предметов надо в сторону облегчения.
//	i = obj->get_weight();
	switch (skill) {
		case ESkill::kMakeBow:;
		case ESkill::kMakeWear: obj->set_extra_flag(EObjFlag::kTransformed);
			obj->set_extra_flag(EObjFlag::KLimitedTimer);
			break;
		default: break;
	}
	int sign = -1;
	if (obj->get_type() == EObjType::kWeapon
		|| obj->get_type() == EObjType::kMagicIngredient) {
		sign = 1;
	}
	obj->set_weight(stat_modify(ch, obj->get_weight(), 20 * sign));

	i = obj->get_timer();
	obj->set_timer(stat_modify(ch, obj->get_timer(), 1));
	// Модифицируем уникальные для предметов х-ки.
	// Балансировка варьируется изменением делителя (!20!)
	// при делителе 20 и умении 100 и макс везении будет +5 к параметру
	// Можно посчитать бонусы от времени суток.
	// Считаем среднюю силу ингров .
	switch (obj->get_type()) {
		case EObjType::kLightSource:
			// Считаем длительность свечения.
			if (GET_OBJ_VAL(obj, 2) != -1) {
				obj->set_val(2, stat_modify(ch, GET_OBJ_VAL(obj, 2), 1));
			}
			break;
		case EObjType::kWand:
		case EObjType::kStaff:
			// Считаем уровень закла
			obj->set_val(0, GetRealLevel(ch));
			break;
		case EObjType::kWeapon:
			// Считаем число xdy
			// модифицируем XdY
			if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 2)) {
				obj->set_val(1, stat_modify(ch, GET_OBJ_VAL(obj, 1), 1));
			} else {
				obj->set_val(2, stat_modify(ch, GET_OBJ_VAL(obj, 2) - 1, 1) + 1);
			}
			break;
		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor:
			// Считаем + АС
			obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
			// Считаем поглощение.
			obj->set_val(1, stat_modify(ch, GET_OBJ_VAL(obj, 1), 1));
			break;
		case EObjType::kPotion:
			// Считаем уровень итоговый напитка
			obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
			break;
		case EObjType::kContainer:
			// Считаем объем контейнера.
			obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
			break;
		case EObjType::kLiquidContainer:
			// Считаем объем контейнера.
			obj->set_val(0, stat_modify(ch, GET_OBJ_VAL(obj, 0), 1));
			break;
		case EObjType::kMagicIngredient:
			// Для ингров ничего не трогаем ... ибо опасно. :)
			break;
		default: break;
	}
	// 7. Считаем доп. статсы предмета.
	// х-ка прототипа +
	// если (random(100) - сила ингра ) < 1 то переноситься весь параметр.
	// если от 1 до 25 то переноситься 1/2
	// если от 25 до 50 то переноситься 1/3
	// больше переноситься 0
	// переносим доп аффекты ...+мудра +ловка и т.п.
	if (skill == ESkill::kMakeWear) {
		make_object(ch, obj.get(), ingrs, ingr_cnt);
		make_value_wear(ch, obj.get(), ingrs);
	} else // если не шитье то никаких махинаций с падежами и копированием рандом аффекта
	{
		for (j = 0; j < ingr_cnt; j++) {
			ingr_pow = get_ingr_pow(ingrs[j]);
			if (ingr_pow < 0) {
				ingr_pow = 20;
			}
			// переносим аффекты ... c ингров на прототип.
			auto temp_flags = obj->get_affect_flags();
			add_flags(ch, &temp_flags, &ingrs[j]->get_affect_flags(), ingr_pow);
			obj->SetWeaponAffectFlags(temp_flags);
			temp_flags = obj->get_extra_flags();
			// перносим эффекты ... с ингров на прототип.
			add_flags(ch, &temp_flags, &ingrs[j]->get_extra_flags(), ingr_pow);
			obj->set_extra_flags(temp_flags);
			auto temp_affected = obj->get_all_affected();
			add_affects(ch, temp_affected, ingrs[j]->get_all_affected(), ingr_pow);
			obj->set_all_affected(temp_affected);
		}
	}
	// Мочим истраченные ингры.
	for (i = 0; i < ingr_cnt; i++) {
		if (ingrs[i]->get_weight() <= 0) {
			ExtractObjFromWorld(ingrs[i]);
		}
	}
	// 8. Проверяем мах. инворлд.
	// Считаем по формуле (31 - ср. уровень предмета) * 5 -
	// овер шмота в мире не 30 лева не больше 5 штук
	// Т.к. ср. уровень ингров будет определять
	// число шмоток в мире то шмотки по хуже будут вытеснять
	// шмотки по лучше (в целом это не так страшно).
	// Ставим метку если все хорошо.
	if ((obj->get_type() != EObjType::kMagicIngredient
		&& obj->get_type() != EObjType::kMagicComponent)
		&& (number(1, 100) - CalcCurrentSkill(ch, skill, 0) < 0)) {
		act(tagging.c_str(), false, ch, obj.get(), 0, kToChar);
		// Прибавляем в экстра описание строчку.
		char *tagchar = format_act(itemtag.c_str(), ch, obj.get(), 0);
		obj->set_tag(tagchar);
		free(tagchar);
	};
	// простановка мортов при шитье
	auto total_weight = count_mort_requred(obj.get()) * 7 / 10;

	if (total_weight > 35) {
		obj->set_minimum_remorts(12);
	} else if (total_weight > 33) {
		obj->set_minimum_remorts(11);
	} else if (total_weight > 30) {
		obj->set_minimum_remorts(9);
	} else if (total_weight > 25) {
		obj->set_minimum_remorts(6);
	} else if (total_weight > 20) {
		obj->set_minimum_remorts(3);
	} else {
		obj->set_minimum_remorts(0);
	}

	// Пишем производителя в поле.
	obj->set_crafter_uid(ch->get_uid());
	// 9. Проверяем минимум 2
	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
		PlaceObjToRoom(obj.get(), ch->in_room);
	} else if (ch->GetCarryingWeight() + obj->get_weight() > CAN_CARRY_W(ch)) {
		SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
		PlaceObjToRoom(obj.get(), ch->in_room);
	} else {
		PlaceObjToInventory(obj.get(), ch);
	}

	record_craft_success(obj_proto, skill);
	return (true);
}
// вытащить рецепт из строки.
int MakeRecept::load_from_str(string &rstr) {
	// Разбираем строку.
	char tmpbuf[kMaxInputLength];
	// Проверяем рецепт на блокировку .
	if (rstr.substr(0, 1) == string("*")) {
		rstr = rstr.substr(1);
		locked = true;
	} else {
		locked = false;
	}
	skill = static_cast<ESkill>(atoi(rstr.substr(0, rstr.find(" ")).c_str()));
	rstr = rstr.substr(rstr.find(" ") + 1);
	obj_proto = atoi((rstr.substr(0, rstr.find(" "))).c_str());
	rstr = rstr.substr(rstr.find(" ") + 1);

	if (GetObjRnum(obj_proto) < 0) {
		// Обнаружен несуществующий прототип объекта.
		sprintf(tmpbuf, "MakeRecept::Unfound object proto %d", obj_proto);
		mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
		// блокируем рецепты без ингров.
		locked = true;
	}

	for (int i = 0; i < MAX_PARTS; i++) {
		// считали номер прототипа компонента
		parts[i].proto = atoi((rstr.substr(0, rstr.find(" "))).c_str());
		rstr = rstr.substr(rstr.find(" ") + 1);
		// Проверяем на конец компонентов.
		if (parts[i].proto == 0) {
			break;
		}
		if (GetObjRnum(parts[i].proto) < 0) {
			// Обнаружен несуществующий прототип компонента.
			sprintf(tmpbuf, "MakeRecept::Unfound item part %d for %d", obj_proto, parts[i].proto);
			mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
			// блокируем рецепты без ингров.
			locked = true;
		}
		parts[i].min_weight = atoi(rstr.substr(0, rstr.find(" ")).c_str());
		rstr = rstr.substr(rstr.find(" ") + 1);
		parts[i].min_power = atoi(rstr.substr(0, rstr.find(" ")).c_str());
		rstr = rstr.substr(rstr.find(" ") + 1);
	}
	return (true);
}
// сохранить рецепт в строку.
int MakeRecept::save_to_str(string &rstr) {
	char tmpstr[kMaxInputLength];
	if (obj_proto == 0) {
		return (false);
	}
	if (locked) {
		rstr = "*";
	} else {
		rstr = "";
	}
	sprintf(tmpstr, "%d %d", to_underlying(skill), obj_proto);
	rstr += string(tmpstr);
	for (int i = 0; i < MAX_PARTS; i++) {
		sprintf(tmpstr, " %d %d %d", parts[i].proto, parts[i].min_weight, parts[i].min_power);
		rstr += string(tmpstr);
	}
	return (true);
}
// Модификатор базовых значений.
int MakeRecept::stat_modify(CharData *ch, int value, float devider) {
	// Для числовых х-к:  х-ка+(skill - random(100))/20;
	// Для флагов ???: random(200) - skill > 0 то флаг переноситься.
	int res = value;
	float delta = 0;
	int skill_prc = 0;
	if (devider <= 0) {
		return res;
	}
	skill_prc = CalcCurrentSkill(ch, skill, nullptr);
	delta = (int) ((float) (skill_prc - number(0, MUD::Skill(skill).difficulty)));
	if (delta > 0) {
		delta = (value / 2) * delta / MUD::Skill(skill).difficulty / devider;
	} else {
		delta = (value / 4) * delta / MUD::Skill(skill).difficulty / devider;
	}
	res += (int) delta;
	// Если параметр завалили то возвращаем 1;
	if (res < 0) {
		return 1;
	}
	return res;
}
void MakeRecept::add_rnd_skills(CharData * /*ch*/, ObjData *obj_from, ObjData *obj_to) {
	if (obj_from->has_skills()) {
		int rskill;
		int z = 0;
		int percent;
		CObjectPrototype::skills_t skills;
		obj_from->get_skills(skills);
		int i = static_cast<int>(skills.size()); // сколько добавлено умелок
		rskill = number(0, i); // берем рандом
		for (const auto &it : skills) {
			if (z == rskill) // ставим рандомную умелку
			{
				auto skill_num = it.first;
				percent = it.second;
				// TODO: такого не должно быть?
				if (percent == 0) {
					continue;
				}
				obj_to->set_skill(skill_num, percent);
			}
			z++;
		}
	}
}
int MakeRecept::add_flags(CharData *ch, FlagData *base_flag, const FlagData *add_flag, int/* delta*/) {
// БЕз вариантов вообще :(
	int tmpprob, skl = CalcCurrentSkill(ch, skill, 0);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 32; j++) {
			tmpprob = number(0, 200) - skl;
			if ((add_flag->get_plane(i) & (1 << j)) && (tmpprob < 0)) {
				base_flag->set_flag(i, 1 << j);
			}
		}
	}
	return (true);
}
int MakeRecept::add_affects(CharData *ch,
							std::array<obj_affected_type, kMaxObjAffect> &base,
							const std::array<obj_affected_type, kMaxObjAffect> &add,
							int delta) {
	bool found = false;
	int i, j;
	for (i = 0; i < kMaxObjAffect; i++) {
		found = false;
		if (add[i].location == EApply::kNone)
			continue;
		for (j = 0; j < kMaxObjAffect; j++) {
			if (base[j].location == EApply::kNone)
				continue;
			if (add[i].location == base[j].location) {
				// Аффекты совпали.
				found = true;
				if (number(0, 100) > delta)
					break;
				base[j].modifier += stat_modify(ch, add[i].modifier, 1);
				break;
			}
		}
		if (!found) {
			// Ищем первый свободный аффект и втыкаем туда новый.
			for (int j = 0; j < kMaxObjAffect; j++) {
				if (base[j].location == EApply::kNone) {
					if (number(0, 100) > delta)
						break;
					base[j].location = add[i].location;
					base[j].modifier += add[i].modifier;
					//cout << "add affect " << int(base[j].location) <<" - " << int(base[j].modifier) << endl;
					break;
				}
			}
		}
	}
	return (true);
}

// Форматирование вывода в соответствии с форматом act-a
// output act format//
char *format_act(const char *orig, CharData *ch, ObjData *obj, const void *vict_obj) {
	const char *i = nullptr;
	char *buf, *lbuf;
	ubyte padis;
	int stopbyte;
//	CharacterData *dg_victim = nullptr;

	buf = (char *) malloc(kMaxStringLength);
	lbuf = buf;

	for (stopbyte = 0; stopbyte < kMaxStringLength; stopbyte++) {
		if (*orig == '$') {
			switch (*(++orig)) {
				case 'n':
					if (*(orig + 1) < '0' || *(orig + 1) > '5')
						i = ch->get_name().c_str();
					else {
						padis = *(++orig) - '0';
						i = GET_PAD(ch, padis);
					}
					break;
				case 'N':
					if (*(orig + 1) < '0' || *(orig + 1) > '5') {
						CHECK_NULL(vict_obj, GET_PAD((const CharData *) vict_obj, 0));
					} else {
						padis = *(++orig) - '0';
						CHECK_NULL(vict_obj, GET_PAD((const CharData *) vict_obj, padis));
					}
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'm': i = HMHR(ch);
					break;
				case 'M':
					if (vict_obj)
						i = HMHR((const CharData *) vict_obj);
					else CHECK_NULL(obj, OMHR(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 's': i = HSHR(ch);
					break;
				case 'S':
					if (vict_obj)
						i = HSHR((const CharData *) vict_obj);
					else CHECK_NULL(obj, OSHR(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'e': i = HSSH(ch);
					break;
				case 'E':
					if (vict_obj)
						i = HSSH((const CharData *) vict_obj);
					else CHECK_NULL(obj, OSSH(obj));
					break;

				case 'o':
					if (*(orig + 1) < '0' || *(orig + 1) > '5') {
						CHECK_NULL(obj, obj->get_PName(ECase::kNom).c_str());
					} else {
						padis = *(++orig) - '0';
						CHECK_NULL(obj, obj->get_PName(padis > ECase::kLastCase ?
						ECase::kFirstCase : static_cast<ECase>(padis)).c_str());
					}
					break;
				case 'O':
					if (*(orig + 1) < '0' || *(orig + 1) > '5') {
						CHECK_NULL(vict_obj, ((const ObjData *) vict_obj)->get_PName(ECase::kNom).c_str());
					} else {
						padis = *(++orig) - '0';
						CHECK_NULL(vict_obj, ((const ObjData *) vict_obj)->get_PName(padis > ECase::kLastCase ?
						ECase::kFirstCase : static_cast<ECase>(padis)).c_str());
					}
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 't': CHECK_NULL(obj, (const char *) obj);
					break;

				case 'T': CHECK_NULL(vict_obj, (const char *) vict_obj);
					break;

				case 'F': CHECK_NULL(vict_obj, (const char *) vict_obj);
					break;

				case '$': i = "$";
					break;

				case 'a': i = GET_CH_SUF_6(ch);
					break;
				case 'A':
					if (vict_obj)
						i = GET_CH_SUF_6((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_6(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'g': i = GET_CH_SUF_1(ch);
					break;
				case 'G':
					if (vict_obj)
						i = GET_CH_SUF_1((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_1(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'y': i = GET_CH_SUF_5(ch);
					break;
				case 'Y':
					if (vict_obj)
						i = GET_CH_SUF_5((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_5(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'u': i = GET_CH_SUF_2(ch);
					break;
				case 'U':
					if (vict_obj)
						i = GET_CH_SUF_2((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_2(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'w': i = GET_CH_SUF_3(ch);
					break;
				case 'W':
					if (vict_obj)
						i = GET_CH_SUF_3((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_3(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'q': i = GET_CH_SUF_4(ch);
					break;
				case 'Q':
					if (vict_obj)
						i = GET_CH_SUF_4((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_4(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;
				case 'z':
					if (obj)
						i = OYOU(obj);
					else CHECK_NULL(obj, OYOU(obj));
					break;
				case 'Z':
					if (vict_obj)
						i = HYOU((const CharData *) vict_obj);
					else CHECK_NULL(vict_obj, HYOU((const CharData *) vict_obj));
					break;
				default: log("SYSERR: Illegal $-code to act(): %c", *orig);
					log("SYSERR: %s", orig);
					i = "";
					break;
			}
			while ((*buf = *(i++)))
				buf++;
			orig++;
		} else if (*orig == '\\') {
			if (*(orig + 1) == 'r') {
				*(buf++) = '\r';
				orig += 2;
			} else if (*(orig + 1) == 'n') {
				*(buf++) = '\n';
				orig += 2;
			} else
				*(buf++) = *(orig++);
		} else if (!(*(buf++) = *(orig++)))
			break;
	}

	*(--buf) = '\r';
	*(++buf) = '\n';
	*(++buf) = '\0';
	return (lbuf);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
