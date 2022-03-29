/* ************************************************************************
*   File: act.item.cpp                                  Part of Bylins    *
*  Usage: object handling routines -- get/drop and container handling     *
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

#include "cmd/hire.h"
#include "obj_prototypes.h"
#include "entities/char_data.h"
#include "depot.h"
#include "fightsystem/fight.h"
#include "handler.h"
#include "house.h"
#include "liquid.h"
#include "game_mechanics/named_stuff.h"
#include "fightsystem/pk.h"
#include "game_skills/poison.h"
#include "meat_maker.h"
#include "utils/utils_char_obj.inl"
#include "structs/global_objects.h"

// extern variables
extern CharData *mob_proto;
extern struct house_control_rec house_control[];
extern std::array<int, kMaxMobLevel / 11 + 1> animals_levels;
// from act.informative.cpp
char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);

// local functions
int can_take_obj(CharData *ch, ObjData *obj);
void get_check_money(CharData *ch, ObjData *obj, ObjData *cont);
int perform_get_from_room(CharData *ch, ObjData *obj);
void get_from_room(CharData *ch, char *arg, int amount);
void perform_give_gold(CharData *ch, CharData *vict, int amount);
void perform_give(CharData *ch, CharData *vict, ObjData *obj);
void perform_drop(CharData *ch, ObjData *obj);
void perform_drop_gold(CharData *ch, int amount);
CharData *give_find_vict(CharData *ch, char *arg);
void weight_change_object(ObjData *obj, int weight);
int perform_put(CharData *ch, ObjData::shared_ptr obj, ObjData *cont);
void get_from_container(CharData *ch, ObjData *cont, char *arg, int mode, int amount, bool autoloot);
void perform_wear(CharData *ch, ObjData *obj, int where);
int find_eq_pos(CharData *ch, ObjData *obj, char *arg);
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode);
void RemoveEquipment(CharData *ch, int pos);
int invalid_anti_class(CharData *ch, const ObjData *obj);
void feed_charmice(CharData *ch, char *arg);
int get_player_charms(CharData *ch, int spellnum);
ObjData *create_skin(CharData *mob);
int invalid_unique(CharData *ch, const ObjData *obj);
bool unique_stuff(const CharData *ch, const ObjData *obj);

// from class.cpp
int invalid_no_class(CharData *ch, const ObjData *obj);

void do_split(CharData *ch, char *argument, int cmd, int subcmd);
void do_split(CharData *ch, char *argument, int cmd, int subcmd, int currency);
void do_remove(CharData *ch, char *argument, int cmd, int subcmd);
void do_put(CharData *ch, char *argument, int cmd, int subcmd);
void do_get(CharData *ch, char *argument, int cmd, int subcmd);
void do_drop(CharData *ch, char *argument, int cmd, int subcmd);
void do_give(CharData *ch, char *argument, int cmd, int subcmd);
void do_drink(CharData *ch, char *argument, int cmd, int subcmd);
void do_eat(CharData *ch, char *argument, int cmd, int subcmd);
void do_drunkoff(CharData *ch, char *argument, int cmd, int subcmd);
void do_pour(CharData *ch, char *argument, int cmd, int subcmd);
void do_wear(CharData *ch, char *argument, int cmd, int subcmd);
void do_wield(CharData *ch, char *argument, int cmd, int subcmd);
void do_grab(CharData *ch, char *argument, int cmd, int subcmd);
void do_upgrade(CharData *ch, char *argument, int cmd, int subcmd);
void do_fry(CharData *ch, char *argument, int/* cmd*/);
void do_refill(CharData *ch, char *argument, int cmd, int subcmd);

// чтобы словить невозможность положить в клан-сундук,
// иначе при пол все сун будет спам на каждый предмет, мол низя
// 0 - все ок, 1 - нельзя положить и дальше не обрабатывать (для кланов), 2 - нельзя положить и идти дальше
int perform_put(CharData *ch, ObjData::shared_ptr obj, ObjData *cont) {
	if (!bloody::handle_transfer(ch, nullptr, obj.get(), cont)) {
		return 2;
	}

	if (!drop_otrigger(obj.get(), ch)) {
		return 2;
	}

	if (!put_otrigger(obj.get(), ch, cont)) {
		return 2;
	}

	// если кладем в клановый сундук
	if (Clan::is_clan_chest(cont)) {
		if (!Clan::PutChest(ch, obj.get(), cont)) {
			return 1;
		}
		return 0;
	}

	// клан-хранилище под ингры
	if (ClanSystem::is_ingr_chest(cont)) {
		if (!Clan::put_ingr_chest(ch, obj.get(), cont)) {
			return 1;
		}
		return 0;
	}

	// персональный сундук
	if (Depot::is_depot(cont)) {
		if (!Depot::put_depot(ch, obj)) {
			return 1;
		}
		return 0;
	}

	if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0)) {
		act("$O : $o не помещается туда.", false, ch, obj.get(), cont, kToChar);
	} 
	else if (GET_OBJ_TYPE(obj) == ObjData::ITEM_CONTAINER && obj->get_contains()) {
		send_to_char(ch, "В %s что-то лежит.\r\n", obj->get_PName(5).c_str());
	} 
	else if (obj->has_flag(EObjFlag::kNodrop)) {
		act("Неведомая сила помешала положить $o3 в $O3.", false, ch, obj.get(), cont, kToChar);
	} 
	else if (obj->has_flag(EObjFlag::kZonedacay) || obj->get_type() == ObjData::ITEM_KEY) {
		act("Неведомая сила помешала положить $o3 в $O3.", false, ch, obj.get(), cont, kToChar);
	} 
	else {
		obj_from_char(obj.get());
		// чтобы там по 1 куне гор не было, чару тож возвращается на счет, а не в инвентарь кучкой
		if (obj->get_type() == ObjData::ITEM_MONEY && obj->get_rnum() == 0) {
			ObjData *temp, *obj_next;
			for (temp = cont->get_contains(); temp; temp = obj_next) {
				obj_next = temp->get_next_content();
				if (GET_OBJ_TYPE(temp) == ObjData::ITEM_MONEY) {
					// тут можно просто в поле прибавить, но там описание для кун разное от кол-ва
					int money = GET_OBJ_VAL(temp, 0);
					money += GET_OBJ_VAL(obj, 0);
					obj_from_obj(temp);
					extract_obj(temp);
					obj_from_obj(obj.get());
					extract_obj(obj.get());
					obj = create_money(money);
					if (!obj) {
						return 0;
					}
					break;
				}
			}
		}

		obj_to_obj(obj.get(), cont);

		act("$n положил$g $o3 в $O3.", true, ch, obj.get(), cont, kToRoom | kToArenaListen);

		// Yes, I realize this is strange until we have auto-equip on rent. -gg
		if (obj->has_flag(EObjFlag::kNodrop) && !cont->has_flag(EObjFlag::kNodrop)) {
			cont->set_extra_flag(EObjFlag::kNodrop);
			act("Вы почувствовали что-то странное, когда положили $o3 в $O3.",
				false, ch, obj.get(), cont, kToChar);
		} else
			act("Вы положили $o3 в $O3.", false, ch, obj.get(), cont, kToChar);
		return 0;
	}
	return 2;
}
const int effects_l[5][40][2]{
	{{0, 0}},
	{{0, 26}, // количество строк
	 {APPLY_ABSORBE, 5},
	 {APPLY_C1, 3},
	 {APPLY_C2, 3},
	 {APPLY_C3, 2},
	 {APPLY_C4, 2},
	 {APPLY_C5, 1},
	 {APPLY_C6, 1},
	 {APPLY_CAST_SUCCESS, 3},
	 {APPLY_HIT, 20},
	 {APPLY_HITREG, 35},
	 {APPLY_INITIATIVE, 5},
	 {APPLY_MANAREG, 15},
	 {APPLY_MORALE, 5},
	 {APPLY_MOVE, 35},
	 {APPLY_RESIST_AIR, 15},
	 {APPLY_RESIST_EARTH, 15},
	 {APPLY_RESIST_FIRE, 15},
	 {APPLY_RESIST_IMMUNITY, 5},
	 {APPLY_RESIST_MIND, 5},
	 {APPLY_RESIST_VITALITY, 5},
	 {APPLY_RESIST_WATER, 15},
	 {APPLY_SAVING_CRITICAL, -5},
	 {APPLY_SAVING_REFLEX, -5},
	 {APPLY_SAVING_STABILITY, -5},
	 {APPLY_SAVING_WILL, -5},
	 {APPLY_SIZE, 10},
	 {APPLY_RESIST_DARK, 15},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0}},
	{{0, 37},
	 {APPLY_ABSORBE, 10},
	 {APPLY_C1, 3},
	 {APPLY_C2, 3},
	 {APPLY_C3, 3},
	 {APPLY_C4, 3},
	 {APPLY_C5, 2},
	 {APPLY_C6, 2},
	 {APPLY_C7, 2},
	 {APPLY_C8, 1},
	 {APPLY_C9, 1},
	 {APPLY_CAST_SUCCESS, 5},
	 {APPLY_CHA, 1},
	 {APPLY_CON, 1},
	 {APPLY_DAMROLL, 2},
	 {APPLY_DEX, 1},
	 {APPLY_HIT, 30},
	 {APPLY_HITREG, 55},
	 {APPLY_HITROLL, 2},
	 {APPLY_INITIATIVE, 10},
	 {APPLY_INT, 1},
	 {APPLY_MANAREG, 30},
	 {APPLY_MORALE, 7},
	 {APPLY_MOVE, 55},
	 {APPLY_RESIST_AIR, 25},
	 {APPLY_RESIST_EARTH, 25},
	 {APPLY_RESIST_FIRE, 25},
	 {APPLY_RESIST_IMMUNITY, 10},
	 {APPLY_RESIST_MIND, 10},
	 {APPLY_RESIST_VITALITY, 10},
	 {APPLY_RESIST_WATER, 25},
	 {APPLY_SAVING_CRITICAL, -10},
	 {APPLY_SAVING_REFLEX, -10},
	 {APPLY_SAVING_STABILITY, -10},
	 {APPLY_SAVING_WILL, -10},
	 {APPLY_SIZE, 15},
	 {APPLY_STR, 1},
	 {APPLY_WIS, 1},
	 {0, 0},
	 {0, 0}},
	{{0, 23},
	 {APPLY_ABSORBE, 15},
	 {APPLY_C8, 2},
	 {APPLY_C9, 2},
	 {APPLY_CAST_SUCCESS, 7},
	 {APPLY_CHA, 2},
	 {APPLY_CON, 2},
	 {APPLY_DAMROLL, 3},
	 {APPLY_DEX, 2},
	 {APPLY_HIT, 45},
	 {APPLY_HITROLL, 3},
	 {APPLY_INITIATIVE, 15},
	 {APPLY_INT, 2},
	 {APPLY_MORALE, 9},
	 {APPLY_RESIST_IMMUNITY, 15},
	 {APPLY_RESIST_MIND, 15},
	 {APPLY_RESIST_VITALITY, 15},
	 {APPLY_SAVING_CRITICAL, -15},
	 {APPLY_SAVING_REFLEX, -15},
	 {APPLY_SAVING_STABILITY, -15},
	 {APPLY_SAVING_WILL, -15},
	 {APPLY_SIZE, 20},
	 {APPLY_STR, 2},
	 {APPLY_WIS, 2},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0}},
	{{0, 21},
	 {APPLY_ABSORBE, 20},
	 {APPLY_CAST_SUCCESS, 10},
	 {APPLY_CHA, 2},
	 {APPLY_CON, 2},
	 {APPLY_DAMROLL, 4},
	 {APPLY_DEX, 2},
	 {APPLY_HIT, 60},
	 {APPLY_HITROLL, 4},
	 {APPLY_INITIATIVE, 20},
	 {APPLY_INT, 2},
	 {APPLY_MORALE, 12},
	 {APPLY_MR, 3},
	 {APPLY_RESIST_IMMUNITY, 20},
	 {APPLY_RESIST_MIND, 20},
	 {APPLY_RESIST_VITALITY, 20},
	 {APPLY_SAVING_CRITICAL, -20},
	 {APPLY_SAVING_REFLEX, -20},
	 {APPLY_SAVING_STABILITY, -20},
	 {APPLY_SAVING_WILL, -20},
	 {APPLY_STR, 2},
	 {APPLY_WIS, 2},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0}}

};

ObjData *create_skin(CharData *mob, CharData *ch) {
	int vnum, i, k = 0, num, effect;
	bool concidence;
	const int vnum_skin_prototype = 1660;

	vnum = vnum_skin_prototype + MIN((int) (GetRealLevel(mob) / 5), 9);
	const auto skin = world_objects.create_from_prototype_by_vnum(vnum);
	if (!skin) {
		mudlog("Неверно задан номер прототипа для освежевания в act.item.cpp::create_skin!",
			   NRM, kLvlGreatGod, ERRLOG, true);
		return nullptr;
	}

	skin->set_val(3, int(GetRealLevel(mob) / 11)); // установим уровень шкуры, топовая 44+
	skin->set_parent(GET_MOB_VNUM(mob));
	trans_obj_name(skin.get(), mob); // переносим падежи
	for (i = 1; i <= GET_OBJ_VAL(skin, 3); i++) // топовая шкура до 4х афектов
	{
		if ((k == 1) && (number(1, 100) >= 35)) {
			continue;
		}
		if ((k == 2) && (number(1, 100) >= 20)) {
			continue;
		}
		if ((k == 3) && (number(1, 100) >= 10)) {
			continue;
		}

		{
			concidence = true;
			while (concidence) {
				num = number(1, effects_l[GET_OBJ_VAL(skin, 3)][0][1]);
				concidence = false;
				for (int n = 0; n <= k && i > 1; n++) {
					if (effects_l[GET_OBJ_VAL(skin, 3)][num][0] == (skin)->get_affected(n).location) {
						concidence = true;
					}
				}
			}
			auto location = effects_l[GET_OBJ_VAL(skin, 3)][num][0];
			effect = effects_l[GET_OBJ_VAL(skin, 3)][num][1];
			if (number(0, 1000)
				<= (250 / (GET_OBJ_VAL(skin, 3) + 1))) //  чем круче шкура тем реже  отрицательный аффект
			{
				effect *= -1;
			}
			skin->set_affected(k, static_cast<EApplyLocation>(location), effect);
			k++;
		}
	}

	skin->set_cost(GetRealLevel(mob) * number(2, MAX(3, 3 * k)));
	skin->set_val(2, 95); //оставил 5% фейла переноса аффектов на создаваемую шмотку

	act("$n умело срезал$g $o3.", false, ch, skin.get(), 0, kToRoom | kToArenaListen);
	act("Вы умело срезали $o3.", false, ch, skin.get(), 0, kToChar);

	//ставим флажок "не зависит от прототипа"
	skin->set_extra_flag(EObjFlag::kTransformed);
	return skin.get();
}

void do_put(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	char arg4[kMaxInputLength];
	ObjData *next_obj, *cont;
	CharData *tmp_char;
	int obj_dotmode, cont_dotmode, found = 0, howmany = 1, money_mode = false;
	char *theobj, *thecont, *theplace;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;

	argument = two_arguments(argument, arg1, arg2);
	argument = two_arguments(argument, arg3, arg4);

	if (is_number(arg1)) {
		howmany = atoi(arg1);
		theobj = arg2;
		thecont = arg3;
		theplace = arg4;
	} else {
		theobj = arg1;
		thecont = arg2;
		theplace = arg3;
	}

	if (isname(theplace, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(theplace, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(theplace, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	if (theobj && (!strn_cmp("coin", theobj, 4) || !strn_cmp("кун", theobj, 3))) {
		money_mode = true;
		if (howmany <= 0) {
			send_to_char("Следует указать чиста конкретную сумму.\r\n", ch);
			return;
		}
		if (ch->get_gold() < howmany) {
			send_to_char("Нет у вас такой суммы.\r\n", ch);
			return;
		}
		obj_dotmode = FIND_INDIV;
	} else
		obj_dotmode = find_all_dots(theobj);

	cont_dotmode = find_all_dots(thecont);

	if (!*theobj)
		send_to_char("Положить что и куда?\r\n", ch);
	else if (cont_dotmode != FIND_INDIV)
		send_to_char("Вы можете положить вещь только в один контейнер.\r\n", ch);
	else if (!*thecont) {
		sprintf(buf, "Куда вы хотите положить '%s'?\r\n", theobj);
		send_to_char(buf, ch);
	} else {
		generic_find(thecont, where_bits, ch, &tmp_char, &cont);
		if (!cont) {
			sprintf(buf, "Вы не видите здесь '%s'.\r\n", thecont);
			send_to_char(buf, ch);
		} else if (GET_OBJ_TYPE(cont) != ObjData::ITEM_CONTAINER) {
			act("В $o3 нельзя ничего положить.", false, ch, cont, 0, kToChar);
		} else if (OBJVAL_FLAGGED(cont, CONT_CLOSED)) {
			act("$o0 закрыт$A!", false, ch, cont, 0, kToChar);
		} else {
			if (obj_dotmode == FIND_INDIV)    // put <obj> <container>
			{
				if (money_mode) {
					if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM)) {
						act("Неведомая сила помешала вам сделать это!!", false,
							ch, 0, 0, kToChar);
						return;
					}

					const auto obj = create_money(howmany);

					if (!obj) {
						return;
					}

					obj_to_char(obj.get(), ch);
					ch->remove_gold(howmany);

					// если положить не удалось - возвращаем все взад
					if (perform_put(ch, obj, cont)) {
						obj_from_char(obj.get());
						extract_obj(obj.get());
						ch->add_gold(howmany);
						return;
					}
				} else {
					auto obj = get_obj_in_list_vis(ch, theobj, ch->carrying);
					if (!obj) {
						sprintf(buf, "У вас нет '%s'.\r\n", theobj);
						send_to_char(buf, ch);
					} else if (obj == cont) {
						send_to_char("Вам будет трудно запихнуть вещь саму в себя.\r\n", ch);
					} else {
						ObjData *next_obj;
						while (obj && howmany--) {
							next_obj = obj->get_next_content();
							const auto object_ptr = world_objects.get_by_raw_ptr(obj);
							if (perform_put(ch, object_ptr, cont) == 1) {
								return;
							}
							obj = get_obj_in_list_vis(ch, theobj, next_obj);
						}
					}
				}
			} else {
				for (auto obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->get_next_content();
					if (obj != cont
						&& CAN_SEE_OBJ(ch, obj)
						&& (obj_dotmode == FIND_ALL
							|| isname(theobj, obj->get_aliases())
							|| CHECK_CUSTOM_LABEL(theobj, obj, ch))) {
						found = 1;
						const auto object_ptr = world_objects.get_by_raw_ptr(obj);
						if (perform_put(ch, object_ptr, cont) == 1) {
							return;
						}
					}
				}

				if (!found) {
					if (obj_dotmode == FIND_ALL)
						send_to_char
							("Чтобы положить что-то ненужное нужно купить что-то ненужное.\r\n",
							 ch);
					else {
						sprintf(buf, "Вы не видите ничего похожего на '%s'.\r\n", theobj);
						send_to_char(buf, ch);
					}
				}
			}
		}
	}
}

//переложить стрелы из пучка стрел
//в колчан
void do_refill(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *from_obj = nullptr, *to_obj = nullptr;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1)    // No arguments //
	{
		send_to_char("Откуда брать стрелы?\r\n", ch);
		return;
	}
	if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		send_to_char("У вас нет этого!\r\n", ch);
		return;
	}
	if (GET_OBJ_TYPE(from_obj) != ObjData::ITEM_MAGIC_ARROW) {
		send_to_char("И как вы себе это представляете?\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("Пусто.", false, ch, from_obj, 0, kToChar);
		return;
	}
	if (!*arg2) {
		send_to_char("Куда вы хотите их засунуть?\r\n", ch);
		return;
	}
	if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		send_to_char("Вы не можете этого найти!\r\n", ch);
		return;
	}
	if (!((GET_OBJ_TYPE(to_obj) == ObjData::ITEM_MAGIC_CONTAINER)
		|| GET_OBJ_TYPE(to_obj) == ObjData::ITEM_MAGIC_ARROW)) {
		send_to_char("Вы не сможете в это сложить стрелы.\r\n", ch);
		return;
	}

	if (to_obj == from_obj) {
		send_to_char("Нечем заняться? На печи ездить еще не научились?\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 2) >= GET_OBJ_VAL(to_obj, 1)) {
		send_to_char("Там нет места.\r\n", ch);
		return;
	} else //вроде прошли все проверки. начинаем перекладывать
	{
		if (GET_OBJ_VAL(from_obj, 0) != GET_OBJ_VAL(to_obj, 0)) {
			send_to_char("Хамово ремесло еще не известно на руси.\r\n", ch);
			return;
		}
		int t1 = GET_OBJ_VAL(from_obj, 3);  // количество зарядов
		int t2 = GET_OBJ_VAL(to_obj, 3);
		int delta = (GET_OBJ_VAL(to_obj, 2) - GET_OBJ_VAL(to_obj, 3));
		if (delta >= t1) //объем колчана больше пучка
		{
			to_obj->add_val(2, t1);
			send_to_char("Вы аккуратно сложили стрелы в колчан.\r\n", ch);
			extract_obj(from_obj);
			return;
		} else {
			to_obj->add_val(2, (t2 - GET_OBJ_VAL(to_obj, 2)));
			send_to_char("Вы аккуратно переложили несколько стрел в колчан.\r\n", ch);
			from_obj->add_val(2, (GET_OBJ_VAL(to_obj, 2) - t2));
			return;
		}
	}

	send_to_char("С таким успехом надо пополнять соседние камни, для разговоров по ним.\r\n", ch);
	return;

}

int can_take_obj(CharData *ch, ObjData *obj) {
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)
		&& GET_OBJ_TYPE(obj) != ObjData::ITEM_MONEY) {
		act("$p: Вы не могете нести столько вещей.", false, ch, obj, 0, kToChar);
		return (0);
	} else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)
		&& GET_OBJ_TYPE(obj) != ObjData::ITEM_MONEY) {
		act("$p: Вы не в состоянии нести еще и $S.", false, ch, obj, 0, kToChar);
		return (0);
	} else if (!(CAN_WEAR(obj, EWearFlag::kTake))) {
		act("$p: Вы не можете взять $S.", false, ch, obj, 0, kToChar);
		return (0);
	} else if (invalid_anti_class(ch, obj)) {
		act("$p: Эта вещь не предназначена для вас!", false, ch, obj, 0, kToChar);
		return (0);
	} else if (NamedStuff::check_named(ch, obj, 0)) {
		if (!NamedStuff::wear_msg(ch, obj))
			act("$p: Эта вещь не предназначена для вас!", false, ch, obj, 0, kToChar);
		return (0);
	}
	return (1);
}

/// считаем сколько у ch в группе еще игроков (не мобов)
int other_pc_in_group(CharData *ch) {
	int num = 0;
	CharData *k = ch->has_master() ? ch->get_master() : ch;
	for (Follower *f = k->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->ch, EAffectFlag::AFF_GROUP)
			&& !f->ch->is_npc()
			&& IN_ROOM(f->ch) == ch->in_room) {
			++num;
		}
	}
	return num;
}

void split_or_clan_tax(CharData *ch, long amount) {
	if (IS_AFFECTED(ch, AFF_GROUP) && (other_pc_in_group(ch) > 0) && PRF_FLAGGED(ch, EPrf::kAutosplit)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%ld", amount);
		do_split(ch, buf_, 0, 0);
	} else {
		long tax = ClanSystem::do_gold_tax(ch, amount);
		ch->remove_gold(tax);
	}
}

void get_check_money(CharData *ch, ObjData *obj, ObjData *cont) {

	if (system_obj::is_purse(obj) && GET_OBJ_VAL(obj, 3) == ch->get_uid()) {
		system_obj::process_open_purse(ch, obj);
		return;
	}

	const int value = GET_OBJ_VAL(obj, 0);
	const int curr_type = GET_OBJ_VAL(obj, 1);

	if (GET_OBJ_TYPE(obj) != ObjData::ITEM_MONEY
		|| value <= 0) {
		return;
	}

	if (curr_type == currency::ICE) {
		sprintf(buf, "Это составило %d %s.\r\n", value, GetDeclensionInNumber(value, EWhat::kIceU));
		send_to_char(buf, ch);
		ch->add_ice_currency(value);
		//Делить лед ВСЕГДА!
		if (IS_AFFECTED(ch, AFF_GROUP) && other_pc_in_group(ch) > 0) {
			char local_buf[256];
			sprintf(local_buf, "%d", value);
			do_split(ch, local_buf, 0, 0, curr_type);
		}
		extract_obj(obj);
		return;
	}

// Все что неизвестно - куны (для совместимости)
/*	if (curr_type != currency::GOLD) {
		//Вот тут неопознанная валюта
		return;
	}
*/
	sprintf(buf, "Это составило %d %s.\r\n", value, GetDeclensionInNumber(value, EWhat::kMoneyU));
	send_to_char(buf, ch);

	// все, что делится на группу - идет через налог (из кошельков не делится)
	if (IS_AFFECTED(ch, AFF_GROUP) && other_pc_in_group(ch) > 0
		&& PRF_FLAGGED(ch, EPrf::kAutosplit)
		&& (!cont || !system_obj::is_purse(cont))) {
		// добавляем бабло, пишем в лог, клан-налог снимаем
		// только по факту деления на группу в do_split()
		ch->add_gold(value);
		sprintf(buf,
				"<%s> {%d} заработал %d %s в группе.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				value,
				GetDeclensionInNumber(value, EWhat::kMoneyU));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		char local_buf[256];
		sprintf(local_buf, "%d", value);
		do_split(ch, local_buf, 0, 0);
	} else if (cont && system_obj::is_purse(cont)) {
		// лут кошелька с баблом
		// налогом не облагается, т.к. уже все уплочено
		// на данном этапе cont уже не содержит владельца
		sprintf(buf, "%s взял деньги из кошелька: %d  %s.", ch->get_name().c_str(), value,
				GetDeclensionInNumber(value, EWhat::kMoneyU));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		ch->add_gold(value);
	} else if ((cont && IS_MOB_CORPSE(cont)) || GET_OBJ_VNUM(obj) != -1) {
		// лут из трупа моба или из предметов-денег с внумом
		// (предметы-награды в зонах) - снимаем клан-налог
		sprintf(buf, "%s заработал %d  %s.", ch->get_name().c_str(), value,
				GetDeclensionInNumber(value, EWhat::kMoneyU));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		ch->add_gold(value, true, true);
	} else {
		sprintf(buf,
				"<%s> {%d} как-то получил %d  %s.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				value,
				GetDeclensionInNumber(value, EWhat::kMoneyU));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		ch->add_gold(value);
	}

	obj_from_char(obj);
	extract_obj(obj);
}

// return 0 - чтобы словить невозможность взять из клан-сундука,
// иначе при вз все сун будет спам на каждый предмет, мол низя
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode) {
	if (!bloody::handle_transfer(nullptr, ch, obj))
		return false;
	if ((mode == FIND_OBJ_INV || mode == FIND_OBJ_ROOM || mode == FIND_OBJ_EQUIP) && can_take_obj(ch, obj)
		&& get_otrigger(obj, ch)) {
		// если берем из клан-сундука
		if (Clan::is_clan_chest(cont)) {
			if (!Clan::TakeChest(ch, obj, cont)) {
				return 0;
			}
			return 1;
		}
		// клан-хранилище ингров
		if (ClanSystem::is_ingr_chest(cont)) {
			if (!Clan::take_ingr_chest(ch, obj, cont)) {
				return 0;
			}
			return 1;
		}
		obj_from_obj(obj);
		obj_to_char(obj, ch);
		if (obj->get_carried_by() == ch) {
			if (bloody::is_bloody(obj)) {
				act("Вы взяли $o3 из $O1, испачкав свои руки кровью!", false, ch, obj, cont, kToChar);
				act("$n взял$g $o3 из $O1, испачкав руки кровью.", true, ch, obj, cont, kToRoom | kToArenaListen);
			} else {
				act("Вы взяли $o3 из $O1.", false, ch, obj, cont, kToChar);
				act("$n взял$g $o3 из $O1.", true, ch, obj, cont, kToRoom | kToArenaListen);
			}
			get_check_money(ch, obj, cont);
		}
	}
	return 1;
}

// *\param autoloot - true только при взятии шмоток из трупа в режиме автограбежа
void get_from_container(CharData *ch, ObjData *cont, char *arg, int mode, int howmany, bool autoloot) {
	if (Depot::is_depot(cont)) {
		Depot::take_depot(ch, arg, howmany);
		return;
	}

	ObjData *obj, *next_obj;
	int obj_dotmode, found = 0;

	obj_dotmode = find_all_dots(arg);
	if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
		act("$o закрыт$A.", false, ch, cont, 0, kToChar);
	else if (obj_dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, cont->get_contains()))) {
			sprintf(buf, "Вы не видите '%s' в $o5.", arg);
			act(buf, false, ch, cont, 0, kToChar);
		} else {
			ObjData *obj_next;
			while (obj && howmany--) {
				obj_next = obj->get_next_content();
				if (!perform_get_from_container(ch, obj, cont, mode))
					return;
				obj = get_obj_in_list_vis(ch, arg, obj_next);
			}
		}
	} else {
		if (obj_dotmode == FIND_ALLDOT && !*arg) {
			send_to_char("Взять что \"все\"?\r\n", ch);
			return;
		}
		for (obj = cont->get_contains(); obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (obj_dotmode == FIND_ALL
					|| isname(arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(arg, obj, ch))) {
				if (autoloot
					&& (GET_OBJ_TYPE(obj) == ObjData::ITEM_INGREDIENT
						|| GET_OBJ_TYPE(obj) == ObjData::ITEM_MING)
					&& PRF_FLAGGED(ch, EPrf::kNoIngrLoot)) {
					continue;
				}
				found = 1;
				if (!perform_get_from_container(ch, obj, cont, mode)) {
					return;
				}
			}
		}
		if (!found) {
			if (obj_dotmode == FIND_ALL)
				act("$o пуст$A.", false, ch, cont, 0, kToChar);
			else {
				sprintf(buf, "Вы не видите ничего похожего на '%s' в $o5.", arg);
				act(buf, false, ch, cont, 0, kToChar);
			}
		}
	}
}

int perform_get_from_room(CharData *ch, ObjData *obj) {
	if (can_take_obj(ch, obj) && get_otrigger(obj, ch) && bloody::handle_transfer(nullptr, ch, obj)) {
		obj_from_room(obj);
		obj_to_char(obj, ch);
		if (obj->get_carried_by() == ch) {
			if (bloody::is_bloody(obj)) {
				act("Вы подняли $o3, испачкав свои руки кровью!", false, ch, obj, 0, kToChar);
				act("$n поднял$g $o3, испачкав руки кровью.", true, ch, obj, 0, kToRoom | kToArenaListen);
			} else {
				act("Вы подняли $o3.", false, ch, obj, 0, kToChar);
				act("$n поднял$g $o3.", true, ch, obj, 0, kToRoom | kToArenaListen);
			}
			get_check_money(ch, obj, 0);
			return (1);
		}
	}
	return (0);
}

void get_from_room(CharData *ch, char *arg, int howmany) {
	ObjData *obj, *next_obj;
	int dotmode, found = 0;

	// Are they trying to take something in a room extra description?
	if (find_exdesc(arg, world[ch->in_room]->ex_description) != nullptr) {
		send_to_char("Вы не можете это взять.\r\n", ch);
		return;
	}

	dotmode = find_all_dots(arg);

	if (dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents))) {
			sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			ObjData *obj_next;
			while (obj && howmany--) {
				obj_next = obj->get_next_content();
				perform_get_from_room(ch, obj);
				obj = get_obj_in_list_vis(ch, arg, obj_next);
			}
		}
	} else {
		if (dotmode == FIND_ALLDOT && !*arg) {
			send_to_char("Взять что \"все\"?\r\n", ch);
			return;
		}
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (dotmode == FIND_ALL
					|| isname(arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(arg, obj, ch))) {
				found = 1;
				perform_get_from_room(ch, obj);
			}
		}
		if (!found) {
			if (dotmode == FIND_ALL) {
				send_to_char("Похоже, здесь ничего нет.\r\n", ch);
			} else {
				sprintf(buf, "Вы не нашли здесь '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
		}
	}
}

void do_mark(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	int cont_dotmode, found = 0;
	ObjData *cont;
	CharData *tmp_char;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char("Что вы хотите маркировать?\r\n", ch);
	} else if (!*arg2 || !is_number(arg2)) {
		send_to_char("Не указан или неверный маркер.\r\n", ch);
	} else {
		cont_dotmode = find_all_dots(arg1);
		if (cont_dotmode == FIND_INDIV) {
			generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
			if (!cont) {
				sprintf(buf, "У вас нет '%s'.\r\n", arg1);
				send_to_char(buf, ch);
				return;
			}
			cont->set_owner(atoi(arg2));
			act("Вы пометили $o3.", false, ch, cont, 0, kToChar);
		} else {
			if (cont_dotmode == FIND_ALLDOT && !*arg1) {
				send_to_char("Пометить что \"все\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont; cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(arg1, cont->get_aliases()))) {
					cont->set_owner(atoi(arg2));
					act("Вы пометили $o3.", false, ch, cont, 0, kToChar);
					found = true;
				}
			}
			for (cont = world[ch->in_room]->contents; cont; cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(arg2, cont->get_aliases()))) {
					cont->set_owner(atoi(arg2));
					act("Вы пометили $o3.", false, ch, cont, 0, kToChar);
					found = true;
				}
			}
			if (!found) {
				if (cont_dotmode == FIND_ALL) {
					send_to_char("Вы не смогли найти ничего для маркировки.\r\n", ch);
				} else {
					sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", arg1);
					send_to_char(buf, ch);
				}
			}
		}
	}
}

void do_get(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	char arg4[kMaxInputLength];
	char *theobj, *thecont, *theplace;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;

	int cont_dotmode, found = 0, mode, amount = 1;
	ObjData *cont;
	CharData *tmp_char;

	argument = two_arguments(argument, arg1, arg2);
	argument = two_arguments(argument, arg3, arg4);

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		send_to_char("У вас заняты руки!\r\n", ch);
	else if (!*arg1)
		send_to_char("Что вы хотите взять?\r\n", ch);
	else if (!*arg2 || isname(arg2, "земля комната ground room"))
		get_from_room(ch, arg1, 1);
	else if (is_number(arg1) && (!*arg3 || isname(arg3, "земля комната ground room")))
		get_from_room(ch, arg2, atoi(arg1));
	else if ((!*arg3 && isname(arg2, "инвентарь экипировка inventory equipment")) ||
		(is_number(arg1) && !*arg4 && isname(arg3, "инвентарь экипировка inventory equipment")))
		send_to_char("Вы уже подобрали этот предмет!\r\n", ch);
	else {
		if (is_number(arg1)) {
			amount = atoi(arg1);
			theobj = arg2;
			thecont = arg3;
			theplace = arg4;
		} else {
			theobj = arg1;
			thecont = arg2;
			theplace = arg3;
		}

		if (isname(theplace, "земля комната room ground"))
			where_bits = FIND_OBJ_ROOM;
		else if (isname(theplace, "инвентарь inventory"))
			where_bits = FIND_OBJ_INV;
		else if (isname(theplace, "экипировка equipment"))
			where_bits = FIND_OBJ_EQUIP;

		cont_dotmode = find_all_dots(thecont);
		if (cont_dotmode == FIND_INDIV) {
			mode = generic_find(thecont, where_bits, ch, &tmp_char, &cont);
			if (!cont) {
				sprintf(buf, "Вы не видите '%s'.\r\n", arg2);
				send_to_char(buf, ch);
			} else if (GET_OBJ_TYPE(cont) != ObjData::ITEM_CONTAINER) {
				act("$o - не контейнер.", false, ch, cont, 0, kToChar);
			} else {
				get_from_container(ch, cont, theobj, mode, amount, false);
			}
		} else {
			if (cont_dotmode == FIND_ALLDOT
				&& !*thecont) {
				send_to_char("Взять из чего \"всего\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont && IS_SET(where_bits, FIND_OBJ_INV); cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch))) {
					if (GET_OBJ_TYPE(cont) == ObjData::ITEM_CONTAINER) {
						found = 1;
						get_from_container(ch, cont, theobj, FIND_OBJ_INV, amount, false);
					} else if (cont_dotmode == FIND_ALLDOT) {
						found = 1;
						act("$o - не контейнер.", false, ch, cont, 0, kToChar);
					}
				}
			}
			for (cont = world[ch->in_room]->contents; cont && IS_SET(where_bits, FIND_OBJ_ROOM);
				 cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch))) {
					if (GET_OBJ_TYPE(cont) == ObjData::ITEM_CONTAINER) {
						get_from_container(ch, cont, theobj, FIND_OBJ_ROOM, amount, false);
						found = 1;
					} else if (cont_dotmode == FIND_ALLDOT) {
						act("$o - не контейнер.", false, ch, cont, 0, kToChar);
						found = 1;
					}
				}
			}
			if (!found) {
				if (cont_dotmode == FIND_ALL) {
					send_to_char("Вы не смогли найти ни одного контейнера.\r\n", ch);
				} else {
					sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", thecont);
					send_to_char(buf, ch);
				}
			}
		}
	}
}

void perform_drop_gold(CharData *ch, int amount) {
	if (amount <= 0) {
		send_to_char("Да, похоже вы слишком переиграли сегодня.\r\n", ch);
	} else if (ch->get_gold() < amount) {
		send_to_char("У вас нет такой суммы!\r\n", ch);
	} else {
		WAIT_STATE(ch, kPulseViolence);    // to prevent coin-bombing
		if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM)) {
			act("Неведомая сила помешала вам сделать это!", false, ch, 0, 0, kToChar);
			return;
		}
		//Находим сначала кучку в комнате
		int additional_amount = 0;
		ObjData *next_obj;
		for (ObjData *existing_obj = world[ch->in_room]->contents; existing_obj; existing_obj = next_obj) {
			next_obj = existing_obj->get_next_content();
			if (GET_OBJ_TYPE(existing_obj) == ObjData::ITEM_MONEY && GET_OBJ_VAL(existing_obj, 1) == currency::GOLD) {
				//Запоминаем стоимость существующей кучки и удаляем ее
				additional_amount = GET_OBJ_VAL(existing_obj, 0);
				obj_from_room(existing_obj);
				extract_obj(existing_obj);
			}
		}

		const auto obj = create_money(amount + additional_amount);
		int result = drop_wtrigger(obj.get(), ch);

		if (!result) {
			extract_obj(obj.get());
			return;
		}

		// Если этот моб трупа не оставит, то не выводить сообщение иначе ужасно коряво смотрится в бою и в тригах
		if (!ch->is_npc() || !MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
			send_to_char(ch, "Вы бросили %d %s на землю.\r\n",
						 amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
			sprintf(buf,
					"<%s> {%d} выбросил %d %s на землю.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					GetDeclensionInNumber(amount, EWhat::kMoneyU));
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			sprintf(buf, "$n бросил$g %s на землю.", money_desc(amount, 3));
			act(buf, true, ch, 0, 0, kToRoom | kToArenaListen);
		}
		obj_to_room(obj.get(), ch->in_room);

		ch->remove_gold(amount);
	}
}

const char *drop_op[3] =
	{
		"бросить", "бросили", "бросил"
	};

void perform_drop(CharData *ch, ObjData *obj) {
	if (!drop_otrigger(obj, ch))
		return;
	if (!bloody::handle_transfer(ch, nullptr, obj))
		return;
	if (!drop_wtrigger(obj, ch))
		return;

	if (obj->has_flag(EObjFlag::kNodrop)) {
		sprintf(buf, "Вы не можете %s $o3!", drop_op[0]);
		act(buf, false, ch, obj, 0, kToChar);
		return;
	}
	sprintf(buf, "Вы %s $o3.", drop_op[1]);
	act(buf, false, ch, obj, 0, kToChar);
	sprintf(buf, "$n %s$g $o3.", drop_op[2]);
	act(buf, true, ch, obj, 0, kToRoom | kToArenaListen);
	obj_from_char(obj);

	obj_to_room(obj, ch->in_room);
	obj_decay(obj);
}

void do_drop(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	ObjData *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg) {
		sprintf(buf, "Что вы хотите %s?\r\n", drop_op[0]);
		send_to_char(buf, ch);
		return;
	} else if (is_number(arg)) {
		int multi = atoi(arg);
		one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg) || !str_cmp("кун", arg) || !str_cmp("денег", arg))
			perform_drop_gold(ch, multi);
		else if (multi <= 0)
			send_to_char("Не имеет смысла.\r\n", ch);
		else if (!*arg) {
			sprintf(buf, "%s %d чего?\r\n", drop_op[0], multi);
			send_to_char(buf, ch);
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
			snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на %s.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			do {
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				perform_drop(ch, obj);
				obj = next_obj;
			} while (obj && --multi);
		}
	} else {
		const auto dotmode = find_all_dots(arg);
		// Can't junk or donate all
		if (dotmode == FIND_ALL) {
			if (!ch->carrying)
				send_to_char("А у вас ничего и нет.\r\n", ch);
			else
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->get_next_content();
					perform_drop(ch, obj);
				}
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg) {
				sprintf(buf, "%s \"все\" какого типа предметов?\r\n", drop_op[0]);
				send_to_char(buf, ch);
				return;
			}
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
			while (obj) {
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				perform_drop(ch, obj);
				obj = next_obj;
			}
		} else {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
				send_to_char(buf, ch);
			} else
				perform_drop(ch, obj);
		}
	}
}

void perform_give(CharData *ch, CharData *vict, ObjData *obj) {
	if (!bloody::handle_transfer(ch, vict, obj))
		return;
	if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM) && !IS_GOD(ch)) {
		act("Неведомая сила помешала вам сделать это!", false, ch, 0, 0, kToChar);
		return;
	}
	if (NPC_FLAGGED(vict, NPC_NOTAKEITEMS)) {
		act("$N не нуждается в ваших подачках, своего барахла навалом.", false, ch, 0, vict, kToChar);
		return;
	}
	if (obj->has_flag(EObjFlag::kNodrop)) {
		act("Вы не можете передать $o3!", false, ch, obj, 0, kToChar);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
		act("У $N1 заняты руки.", false, ch, 0, vict, kToChar);
		return;
	}
	if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
		act("$E не может нести такой вес.", false, ch, 0, vict, kToChar);
		return;
	}
	if (!give_otrigger(obj, ch, vict)) {
		act("$E не хочет иметь дело с этой вещью.", false, ch, 0, vict, kToChar);
		return;
	}

	if (!receive_mtrigger(vict, ch, obj)) {
		act("$E не хочет иметь дело с этой вещью.", false, ch, 0, vict, kToChar);
		return;
	}

	act("Вы дали $o3 $N2.", false, ch, obj, vict, kToChar);
	act("$n дал$g вам $o3.", false, ch, obj, vict, kToVict);
	act("$n дал$g $o3 $N2.", true, ch, obj, vict, kToNotVict | kToArenaListen);

	if (!world_objects.get_by_raw_ptr(obj)) {
		return;    // object has been removed from world during script execution.
	}

	obj_from_char(obj);
	obj_to_char(obj, vict);

	// передача объектов-денег и кошельков
	get_check_money(vict, obj, 0);

	if (!ch->is_npc() && !vict->is_npc()) {
		ObjSaveSync::add(ch->get_uid(), vict->get_uid(), ObjSaveSync::CHAR_SAVE);
	}
}

// utility function for give
CharData *give_find_vict(CharData *ch, char *arg) {
	CharData *vict;

	if (!*arg) {
		send_to_char("Кому?\r\n", ch);
		return (nullptr);
	} else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char(NOPERSON, ch);
		return (nullptr);
	} else if (vict == ch) {
		send_to_char("Вы переложили ЭТО из одного кармана в другой.\r\n", ch);
		return (nullptr);
	} else
		return (vict);
}

void perform_give_gold(CharData *ch, CharData *vict, int amount) {
	if (amount <= 0) {
		send_to_char("Ха-ха-ха (3 раза)...\r\n", ch);
		return;
	}
	if (ch->get_gold() < amount && (ch->is_npc() || !IS_IMPL(ch))) {
		send_to_char("И откуда вы их взять собираетесь?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM) && !IS_GOD(ch)) {
		act("Неведомая сила помешала вам сделать это!", false, ch, 0, 0, kToChar);
		return;
	}
	send_to_char(OK, ch);
	sprintf(buf, "$n дал$g вам %d %s.", amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
	act(buf, false, ch, 0, vict, kToVict);
	sprintf(buf, "$n дал$g %s $N2.", money_desc(amount, 3));
	act(buf, true, ch, 0, vict, kToNotVict | kToArenaListen);
	if (!(ch->is_npc() || vict->is_npc())) {
		sprintf(buf,
				"<%s> {%d} передал %d кун при личной встрече c %s.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				amount,
				GET_PAD(vict, 4));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
	}
	if (ch->is_npc() || !IS_IMPL(ch)) {
		ch->remove_gold(amount);
	}
	// если денег дает моб - снимаем клан-налог
	if (ch->is_npc() && !IS_CHARMICE(ch)) {
		vict->add_gold(amount);
		split_or_clan_tax(vict, amount);
	} else {
		vict->add_gold(amount);
	}
	bribe_mtrigger(vict, ch, amount);
}

void perform_give_nogat(CharData *ch, CharData *vict, int amount) {
	if (amount <= 0) {
		send_to_char("Ха-ха-ха (3 раза)...\r\n", ch);
		return;
	}
	if (ch->get_nogata() < amount && (ch->is_npc() || !IS_IMPL(ch))) {
		send_to_char("И откуда ты их взять собирался?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM) && !IS_GOD(ch)) {
		act("Неведомая сила помешала вам сделать это!", false, ch, 0, 0, kToChar);
		return;
	}
	send_to_char(OK, ch);
	sprintf(buf, "$n дал$g вам %d %s.", amount, GetDeclensionInNumber(amount, EWhat::kNogataU));
	act(buf, false, ch, 0, vict, kToVict);
	if (amount > 4)
		sprintf(buf, "$n дал$g много %s $N2.", GetDeclensionInNumber(amount, EWhat::kNogataU));
	else
		sprintf(buf, "$n дал$g %s $N2.", GetDeclensionInNumber(amount, EWhat::kNogataU));
	act(buf, true, ch, 0, vict, kToNotVict | kToArenaListen);
	if (ch->is_npc() || !IS_IMPL(ch)) {
		ch->sub_nogata(amount);
	}
	vict->add_nogata(amount);
}

void do_give(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int amount, dotmode;
	CharData *vict;
	ObjData *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("Дать что и кому?\r\n", ch);
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (!strn_cmp("coin", arg, 4) || !strn_cmp("кун", arg, 3) || !str_cmp("денег", arg)) {
			one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)) != nullptr)
				perform_give_gold(ch, vict, amount);
			return;
		}
		if (!strn_cmp("nogat", arg, 5) || !strn_cmp("ногат", arg, 5)) {
			one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)) != nullptr)
				perform_give_nogat(ch, vict, amount);
			return;
		}
		if (!*arg) {
			sprintf(buf, "Чего %d вы хотите дать?\r\n", amount);
			send_to_char(buf, ch);
		} else if (!(vict = give_find_vict(ch, argument))) {
			return;
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
			snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			while (obj && amount--) {
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				perform_give(ch, vict, obj);
				obj = next_obj;
			}
		}
	} else {
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1)))
			return;
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_INDIV) {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
				send_to_char(buf, ch);
			} else
				perform_give(ch, vict, obj);
		} else {
			if (dotmode == FIND_ALLDOT && !*arg) {
				send_to_char("Дать \"все\" какого типа предметов?\r\n", ch);
				return;
			}
			if (!ch->carrying)
				send_to_char("У вас ведь ничего нет.\r\n", ch);
			else {
				bool has_items = false;
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->get_next_content();
					if (CAN_SEE_OBJ(ch, obj)
						&& (dotmode == FIND_ALL
							|| isname(arg, obj->get_aliases())
							|| CHECK_CUSTOM_LABEL(arg, obj, ch))) {
						perform_give(ch, vict, obj);
						has_items = true;
					}
				}
				if (!has_items) {
					send_to_char(ch, "У вас нет '%s'.\r\n", arg);
				}
			}
		}
	}
}

void weight_change_object(ObjData *obj, int weight) {
	ObjData *tmp_obj;
	CharData *tmp_ch;

	if (obj->get_in_room() != kNowhere) {
		obj->set_weight(MAX(1, GET_OBJ_WEIGHT(obj) + weight));
	} else if ((tmp_ch = obj->get_carried_by())) {
		obj_from_char(obj);
		obj->set_weight(MAX(1, GET_OBJ_WEIGHT(obj) + weight));
		obj_to_char(obj, tmp_ch);
	} else if ((tmp_obj = obj->get_in_obj())) {
		obj_from_obj(obj);
		obj->set_weight(MAX(1, GET_OBJ_WEIGHT(obj) + weight));
		obj_to_obj(obj, tmp_obj);
	} else {
		log("SYSERR: Unknown attempt to subtract weight from an object.");
	}
}

void do_fry(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	ObjData *meet;
	one_argument(argument, arg);
	if (!*arg) {
		send_to_char("Что вы собрались поджарить?\r\n", ch);
		return;
	}
	if (ch->get_fighting()) {
		send_to_char("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}
	if (!(meet = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxStringLength, "У вас нет '%s'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}
	if (!world[ch->in_room]->fires) {
		send_to_char(ch, "На чем вы собрались жарить, огня то нет!\r\n");
		return;
	}
	if (world[ch->in_room]->fires > 2) {
		send_to_char(ch, "Костер слишком силен, сгорит!\r\n");
		return;
	}

	const auto meet_vnum = GET_OBJ_VNUM(meet);
	if (!meat_mapping.has(meet_vnum)) // не нашлось в массиве
	{
		send_to_char(ch, "%s не подходит для жарки.\r\n", GET_OBJ_PNAME(meet, 0).c_str());
		return;
	}

	act("Вы нанизали на веточку и поджарили $o3.", false, ch, meet, 0, kToChar);
	act("$n нанизал$g на веточку и поджарил$g $o3.", true, ch, meet, 0, kToRoom | kToArenaListen);
	const auto tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get(meet_vnum));
	if (tobj) {
		can_carry_obj(ch, tobj.get());
		extract_obj(meet);
		WAIT_STATE(ch, 1 * kPulseViolence);
	} else {
		mudlog("Не возможно загрузить жаренное мясо в act.item.cpp::do_fry!", NRM, kLvlGreatGod, ERRLOG, true);
	}
}

void do_eat(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *food;
	int amount;

	one_argument(argument, arg);

	if (subcmd == SCMD_DEVOUR) {
		if (MOB_FLAGGED(ch, EMobFlag::kResurrected)
			&& can_use_feat(ch->get_master(), ZOMBIE_DROVER_FEAT)) {
			feed_charmice(ch, arg);
			return;
		}
	}
	if (!ch->is_npc()
		&& subcmd == SCMD_DEVOUR) {
		send_to_char("Вы же не зверь какой, пожирать трупы!\r\n", ch);
		return;
	}

	if (ch->is_npc())        // Cannot use GET_COND() on mobs.
		return;

	if (!*arg) {
		send_to_char("Чем вы собрались закусить?\r\n", ch);
		return;
	}
	if (ch->get_fighting()) {
		send_to_char("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}

	if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}
	if (subcmd == SCMD_TASTE
		&& ((GET_OBJ_TYPE(food) == ObjData::ITEM_DRINKCON)
			|| (GET_OBJ_TYPE(food) == ObjData::ITEM_FOUNTAIN))) {
		do_drink(ch, argument, 0, SCMD_SIP);
		return;
	}

	if (!IS_GOD(ch)) {
		if (GET_OBJ_TYPE(food) == ObjData::ITEM_MING) //Сообщение на случай попытки проглотить ингры
		{
			send_to_char("Не можешь приготовить - покупай готовое!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(food) != ObjData::ITEM_FOOD
			&& GET_OBJ_TYPE(food) != ObjData::ITEM_NOTE) {
			send_to_char("Это несъедобно!\r\n", ch);
			return;
		}
	}
	if (GET_COND(ch, FULL) == 0
		&& GET_OBJ_TYPE(food) != ObjData::ITEM_NOTE)    // Stomach full
	{
		send_to_char("Вы слишком сыты для этого!\r\n", ch);
		return;
	}
	if (subcmd == SCMD_EAT
		|| (subcmd == SCMD_TASTE
			&& GET_OBJ_TYPE(food) == ObjData::ITEM_NOTE)) {
		act("Вы съели $o3.", false, ch, food, 0, kToChar);
		act("$n съел$g $o3.", true, ch, food, 0, kToRoom | kToArenaListen);
	} else {
		act("Вы откусили маленький кусочек от $o1.", false, ch, food, 0, kToChar);
		act("$n попробовал$g $o3 на вкус.", true, ch, food, 0, kToRoom | kToArenaListen);
	}

	amount = ((subcmd == SCMD_EAT && GET_OBJ_TYPE(food) != ObjData::ITEM_NOTE)
			  ? GET_OBJ_VAL(food, 0)
			  : 1);

	gain_condition(ch, FULL, -2 * amount);

	if (GET_COND(ch, FULL) == 0) {
		send_to_char("Вы наелись.\r\n", ch);
	}

	for (int i = 0; i < kMaxObjAffect; i++) {
		if (food->get_affected(i).modifier) {
			Affect<EApplyLocation> af;
			af.location = food->get_affected(i).location;
			af.modifier = food->get_affected(i).modifier;
			af.bitvector = 0;
			af.type = kSpellFullFeed;
//			af.battleflag = 0;
			af.duration = CalcDuration(ch, 10 * 2, 0, 0, 0, 0);
			affect_join_fspell(ch, af);
		}

	}

	if ((GET_OBJ_VAL(food, 3) == 1) && !IS_IMMORTAL(ch))    // The shit was poisoned !
	{
		send_to_char("Однако, какой странный вкус!\r\n", ch);
		act("$n закашлял$u и начал$g отплевываться.", false, ch, 0, 0, kToRoom | kToArenaListen);

		Affect<EApplyLocation> af;
		af.type = kSpellPoison;
		af.duration = CalcDuration(ch, amount == 1 ? amount : amount * 2, 0, 0, 0, 0);
		af.modifier = 0;
		af.location = APPLY_STR;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = kAfSameTime;
		affect_join(ch, af, false, false, false, false);
		af.type = kSpellPoison;
		af.duration = CalcDuration(ch, amount == 1 ? amount : amount * 2, 0, 0, 0, 0);
		af.modifier = amount * 3;
		af.location = APPLY_POISON;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = kAfSameTime;
		affect_join(ch, af, false, false, false, false);
		ch->Poisoner = 0;
	}
	if (subcmd == SCMD_EAT
		|| (subcmd == SCMD_TASTE
			&& GET_OBJ_TYPE(food) == ObjData::ITEM_NOTE)) {
		extract_obj(food);
	} else {
		food->set_val(0, food->get_val(0) - 1);
		if (!food->get_val(0)) {
			send_to_char("Вы доели все!\r\n", ch);
			extract_obj(food);
		}
	}
}

void perform_wear(CharData *ch, ObjData *obj, int where) {
	/*
	   * kTake is used for objects that do not require special bits
	   * to be put into that position (e.g. you can hold any object, not just
	   * an object with a HOLD bit.)
	   */

	const EWearFlag wear_bitvectors[] =
		{
			EWearFlag::kTake,
			EWearFlag::kFinger,
			EWearFlag::kFinger,
			EWearFlag::kNeck,
			EWearFlag::kNeck,
			EWearFlag::kBody,
			EWearFlag::kHead,
			EWearFlag::kLegs,
			EWearFlag::kFeet,
			EWearFlag::kHands,
			EWearFlag::kArms,
			EWearFlag::kShield,
			EWearFlag::kShoulders,
			EWearFlag::kWaist,
			EWearFlag::kWrist,
			EWearFlag::kWrist,
			EWearFlag::kWield,
			EWearFlag::kTake,
			EWearFlag::kBoth,
			EWearFlag::kQuiver
		};

	const std::array<const char *, sizeof(wear_bitvectors)> already_wearing =
		{
			"Вы уже используете свет.\r\n",
			"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
			"У вас уже что-то надето на пальцах.\r\n",
			"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
			"У вас уже что-то надето на шею.\r\n",
			"У вас уже что-то надето на туловище.\r\n",
			"У вас уже что-то надето на голову.\r\n",
			"У вас уже что-то надето на ноги.\r\n",
			"У вас уже что-то надето на ступни.\r\n",
			"У вас уже что-то надето на кисти.\r\n",
			"У вас уже что-то надето на руки.\r\n",
			"Вы уже используете щит.\r\n",
			"Вы уже облачены во что-то.\r\n",
			"У вас уже что-то надето на пояс.\r\n",
			"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
			"У вас уже что-то надето на запястья.\r\n",
			"Вы уже что-то держите в правой руке.\r\n",
			"Вы уже что-то держите в левой руке.\r\n",
			"Вы уже держите оружие в обеих руках.\r\n"
			"Вы уже используете колчан.\r\n"
		};

	// first, make sure that the wear position is valid.
	if (!CAN_WEAR(obj, wear_bitvectors[where])) {
		act("Вы не можете надеть $o3 на эту часть тела.", false, ch, obj, 0, kToChar);
		return;
	}
	if (unique_stuff(ch, obj) && obj->has_flag(EObjFlag::kUnique)) {
		send_to_char("Вы не можете использовать более одной такой вещи.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kGlobalCooldown)) {
		if (ch->get_fighting() && (where == EEquipPos::kShield || GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON)) {
			send_to_char("Вам нужно набраться сил.\r\n", ch);
			return;
		}
	};

	// for neck, finger, and wrist, try pos 2 if pos 1 is already full
	if (   // не может держать если есть свет или двуручник
		(where == EEquipPos::kHold && (GET_EQ(ch, EEquipPos::kBoths) || GET_EQ(ch, EEquipPos::kLight)
			|| GET_EQ(ch, EEquipPos::kShield))) ||
			// не может вооружиться если есть двуручник
			(where == EEquipPos::kWield && GET_EQ(ch, EEquipPos::kBoths)) ||
			// не может держать щит если что-то держит или двуручник
			(where == EEquipPos::kShield && (GET_EQ(ch, EEquipPos::kHold) || GET_EQ(ch, EEquipPos::kBoths))) ||
			// не может двуручник если есть щит, свет, вооружен или держит
			(where == EEquipPos::kBoths && (GET_EQ(ch, EEquipPos::kHold) || GET_EQ(ch, EEquipPos::kLight)
				|| GET_EQ(ch, EEquipPos::kShield) || GET_EQ(ch, EEquipPos::kWield))) ||
			// не может держать свет если двуручник или держит
			(where == EEquipPos::kLight && (GET_EQ(ch, EEquipPos::kHold) || GET_EQ(ch, EEquipPos::kBoths)))) {
		send_to_char("У вас заняты руки.\r\n", ch);
		return;
	}
	if (   // не может одеть колчан если одет не лук
		(where == EEquipPos::kQuiver &&
			!(GET_EQ(ch, EEquipPos::kBoths) &&
				(((GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kBoths))) == ObjData::ITEM_WEAPON)
					&& (static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, EEquipPos::kBoths)) == ESkill::kBows))))) {
		send_to_char("А стрелять чем будете?\r\n", ch);
		return;
	}
	// нельзя надеть щит, если недостаточно силы
	if (!IS_IMMORTAL(ch) && (where == EEquipPos::kShield) && !OK_SHIELD(ch, obj)) {
	}

	if ((where == EEquipPos::kFingerR) || (where == EEquipPos::kNeck) || (where == EEquipPos::kWristR))
		if (GET_EQ(ch, where))
			where++;

	if (GET_EQ(ch, where)) {
		send_to_char(already_wearing[where], ch);
		return;
	}
	if (!wear_otrigger(obj, ch, where))
		return;

	//obj_from_char(obj);
	equip_char(ch, obj, where, CharEquipFlag::show_msg);
}

int find_eq_pos(CharData *ch, ObjData *obj, char *arg) {
	int where = -1;

	// \r to prevent explicit wearing. Don't use \n, it's end-of-array marker.
	const char *keywords[] =
		{
			"\r!RESERVED!",
			"палецправый",
			"палецлевый",
			"шея",
			"грудь",
			"тело",
			"голова",
			"ноги",
			"ступни",
			"кисти",
			"руки",
			"щит",
			"плечи",
			"пояс",
			"запястья",
			"\r!RESERVED!",
			"\r!RESERVED!",
			"\r!RESERVED!",
			"\n"
		};

	if (!arg || !*arg) {
		int tmp_where = -1;
		if (CAN_WEAR(obj, EWearFlag::kFinger)) {
			if (!GET_EQ(ch, EEquipPos::kFingerR)) {
				where = EEquipPos::kFingerR;
			} else if (!GET_EQ(ch, EEquipPos::kFingerL)) {
				where = EEquipPos::kFingerL;
			} else {
				tmp_where = EEquipPos::kFingerR;
			}
		}
		if (CAN_WEAR(obj, EWearFlag::kNeck)) {
			if (!GET_EQ(ch, EEquipPos::kNeck)) {
				where = EEquipPos::kNeck;
			} else if (!GET_EQ(ch, EEquipPos::kChest)) {
				where = EEquipPos::kChest;
			} else {
				tmp_where = EEquipPos::kNeck;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kBody)) {
			if (!GET_EQ(ch, EEquipPos::kBody)) {
				where = EEquipPos::kBody;
			} else {
				tmp_where = EEquipPos::kBody;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kHead)) {
			if (!GET_EQ(ch, EEquipPos::kHead)) {
				where = EEquipPos::kHead;
			} else {
				tmp_where = EEquipPos::kHead;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kLegs)) {
			if (!GET_EQ(ch, EEquipPos::kLegs)) {
				where = EEquipPos::kLegs;
			} else {
				tmp_where = EEquipPos::kLegs;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kFeet)) {
			if (!GET_EQ(ch, EEquipPos::kFeet)) {
				where = EEquipPos::kFeet;
			} else {
				tmp_where = EEquipPos::kFeet;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kHands)) {
			if (!GET_EQ(ch, EEquipPos::kHands)) {
				where = EEquipPos::kHands;
			} else {
				tmp_where = EEquipPos::kHands;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kArms)) {
			if (!GET_EQ(ch, EEquipPos::kArms)) {
				where = EEquipPos::kArms;
			} else {
				tmp_where = EEquipPos::kArms;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kShield)) {
			if (!GET_EQ(ch, EEquipPos::kShield)) {
				where = EEquipPos::kShield;
			} else {
				tmp_where = EEquipPos::kShield;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
			if (!GET_EQ(ch, EEquipPos::kShoulders)) {
				where = EEquipPos::kShoulders;
			} else {
				tmp_where = EEquipPos::kShoulders;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kWaist)) {
			if (!GET_EQ(ch, EEquipPos::kWaist)) {
				where = EEquipPos::kWaist;
			} else {
				tmp_where = EEquipPos::kWaist;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
			if (!GET_EQ(ch, EEquipPos::kQuiver)) {
				where = EEquipPos::kQuiver;
			} else {
				tmp_where = EEquipPos::kQuiver;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kWrist)) {
			if (!GET_EQ(ch, EEquipPos::kWristR)) {
				where = EEquipPos::kWristR;
			} else if (!GET_EQ(ch, EEquipPos::kWristL)) {
				where = EEquipPos::kWristL;
			} else {
				tmp_where = EEquipPos::kWristR;
			}
		}

		if (where == -1) {
			where = tmp_where;
		}
	} else {
		where = search_block(arg, keywords, false);
		if (where < 0
			|| *arg == '!') {
			sprintf(buf, "'%s'? Странная анатомия у этих русских!\r\n", arg);
			send_to_char(buf, ch);
			return -1;
		}
	}

	return where;
}

void do_wear(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *obj, *next_obj;
	int where, dotmode, items_worn = 0;

	two_arguments(argument, arg1, arg2);

	if (ch->is_npc()
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& (!NPC_FLAGGED(ch, NPC_ARMORING)
			|| MOB_FLAGGED(ch, EMobFlag::kResurrected))) {
		return;
	}

	if (!*arg1) {
		send_to_char("Что вы собрались надеть?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg1);

	if (*arg2 && (dotmode != FIND_INDIV)) {
		send_to_char("И на какую часть тела вы желаете это надеть?!\r\n", ch);
		return;
	}
	if (dotmode == FIND_ALL) {
		for (obj = ch->carrying; obj && !GET_MOB_HOLD(ch) && GET_POS(ch) > EPosition::kSleep; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (where = find_eq_pos(ch, obj, 0)) >= 0) {
				items_worn++;
				perform_wear(ch, obj, where);
			}
		}
		if (!items_worn) {
			send_to_char("Увы, но надеть вам нечего.\r\n", ch);
		}
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg1) {
			send_to_char("Надеть \"все\" чего?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			sprintf(buf, "У вас нет ничего похожего на '%s'.\r\n", arg1);
			send_to_char(buf, ch);
		} else
			while (obj && !GET_MOB_HOLD(ch) && GET_POS(ch) > EPosition::kSleep) {
				next_obj = get_obj_in_list_vis(ch, arg1, obj->get_next_content());
				if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
					perform_wear(ch, obj, where);
				} else {
					act("Вы не можете надеть $o3.", false, ch, obj, 0, kToChar);
				}
				obj = next_obj;
			}
	} else {
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			sprintf(buf, "У вас нет ничего похожего на '%s'.\r\n", arg1);
			send_to_char(buf, ch);
		} else {
			if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
				perform_wear(ch, obj, where);
			else if (!*arg2)
				act("Вы не можете надеть $o3.", false, ch, obj, 0, kToChar);
		}
	}
}

void do_wield(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int wear;

	if (ch->is_npc() && (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& (!NPC_FLAGGED(ch, NPC_WIELDING) || MOB_FLAGGED(ch, EMobFlag::kResurrected))))
		return;

	if (ch->is_morphed()) {
		send_to_char("Лапами неудобно держать оружие.\r\n", ch);
		return;
	}
	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("Вооружиться чем?\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "Вы не видите ничего похожего на \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
	} else {
		if (!CAN_WEAR(obj, EWearFlag::kWield)
			&& !CAN_WEAR(obj, EWearFlag::kBoth)) {
			send_to_char("Вы не можете вооружиться этим.\r\n", ch);
		} else if (GET_OBJ_TYPE(obj) != ObjData::ITEM_WEAPON) {
			send_to_char("Это не оружие.\r\n", ch);
		} else if (ch->is_npc()
			&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
			send_to_char("Ожившие трупы не могут вооружаться.\r\n", ch);
		} else {
			one_argument(argument, arg);
			if (!str_cmp(arg, "обе")
				&& CAN_WEAR(obj, EWearFlag::kBoth)) {
				// иногда бывает надо
				if (!IS_IMMORTAL(ch) && !OK_BOTH(ch, obj)) {
					act("Вам слишком тяжело держать $o3 двумя руками.", false, ch, obj, 0, kToChar);
					message_str_need(ch, obj, STR_BOTH_W);
					return;
				};
				perform_wear(ch, obj, EEquipPos::kBoths);
				return;
			}

			if (CAN_WEAR(obj, EWearFlag::kWield)) {
				wear = EEquipPos::kWield;
			} else {
				wear = EEquipPos::kBoths;
			}

			if (wear == EEquipPos::kWield && !IS_IMMORTAL(ch) && !OK_WIELD(ch, obj)) {
				act("Вам слишком тяжело держать $o3 в правой руке.", false, ch, obj, 0, kToChar);
				message_str_need(ch, obj, STR_WIELD_W);

				if (CAN_WEAR(obj, EWearFlag::kBoth)) {
					wear = EEquipPos::kBoths;
				} else {
					return;
				}
			}

			if (wear == EEquipPos::kBoths && !IS_IMMORTAL(ch) && !OK_BOTH(ch, obj)) {
				act("Вам слишком тяжело держать $o3 двумя руками.", false, ch, obj, 0, kToChar);
				message_str_need(ch, obj, STR_BOTH_W);
				return;
			};
			perform_wear(ch, obj, wear);
		}
	}
}

std::string readFile1(const std::string &fileName) {
	std::ifstream f(fileName);
	f.seekg(0, std::ios::end);
	size_t size = f.tellg();
	std::string s(size, ' ');
	f.seekg(0);
	f.read(&s[0], size); // по стандарту можно в C++11, по факту работает и на старых компиляторах
	return s;
}

void do_grab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int where = EEquipPos::kHold;
	ObjData *obj;
	one_argument(argument, arg);

	if (ch->is_npc() && !NPC_FLAGGED(ch, NPC_WIELDING))
		return;

	if (ch->is_morphed()) {
		send_to_char("Лапами неудобно это держать.\r\n", ch);
		return;
	}

	if (!*arg)
		send_to_char("Вы заорали : 'Держи его!!! Хватай его!!!'\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на '%s'.\r\n", arg);
		send_to_char(buf, ch);
	} else {
		if (GET_OBJ_TYPE(obj) == ObjData::ITEM_LIGHT) {
			perform_wear(ch, obj, EEquipPos::kLight);
		} else {
			if (!CAN_WEAR(obj, EWearFlag::kHold)
				&& GET_OBJ_TYPE(obj) != ObjData::ITEM_WAND
				&& GET_OBJ_TYPE(obj) != ObjData::ITEM_STAFF
				&& GET_OBJ_TYPE(obj) != ObjData::ITEM_SCROLL
				&& GET_OBJ_TYPE(obj) != ObjData::ITEM_POTION) {
				send_to_char("Вы не можете это держать.\r\n", ch);
				return;
			}

			if (GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
				if (static_cast<ESkill>GET_OBJ_SKILL(obj) == ESkill::kTwohands
					|| static_cast<ESkill>GET_OBJ_SKILL(obj) == ESkill::kBows) {
					send_to_char("Данный тип оружия держать невозможно.", ch);
					return;
				}
			}

			if (ch->is_npc()
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				&& MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
				send_to_char("Ожившие трупы не могут вооружаться.\r\n", ch);
				return;
			}
			if (!IS_IMMORTAL(ch)
				&& !OK_HELD(ch, obj)) {
				act("Вам слишком тяжело держать $o3 в левой руке.", false, ch, obj, 0, kToChar);
				message_str_need(ch, obj, STR_HOLD_W);

				if (CAN_WEAR(obj, EWearFlag::kBoth)) {
					if (!OK_BOTH(ch, obj)) {
						act("Вам слишком тяжело держать $o3 двумя руками.", false, ch, obj, 0, kToChar);
						message_str_need(ch, obj, STR_BOTH_W);
						return;
					} else {
						where = EEquipPos::kBoths;
					}
				} else {
					return;
				}
			}
			perform_wear(ch, obj, where);
		}
	}
}

void RemoveEquipment(CharData *ch, int pos) {
	ObjData *obj;

	if (!(obj = GET_EQ(ch, pos))) {
		log("SYSERR: RemoveEquipment: bad pos %d passed.", pos);
	} else {
		/*
			   if (IS_OBJ_STAT(obj, ITEM_NODROP))
			   act("Вы не можете снять $o3!", false, ch, obj, 0, TO_CHAR);
			   else
			 */
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
			act("$p: Вы не можете нести столько вещей!", false, ch, obj, 0, kToChar);
		} else {
			if (!remove_otrigger(obj, ch)) {
				return;
			}
			if (ch->get_fighting() && (GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON || pos == EEquipPos::kShield)) {
				ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
			}
			act("Вы прекратили использовать $o3.", false, ch, obj, 0, kToChar);
			act("$n прекратил$g использовать $o3.", true, ch, obj, 0, kToRoom | kToArenaListen);
			obj_to_char(unequip_char(ch, pos, CharEquipFlag::show_msg), ch);
		}
	}
}

void do_remove(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int i, dotmode, found;
	ObjData *obj;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Снять что?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg);

	if (dotmode == FIND_ALL) {
		found = 0;
		for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (GET_EQ(ch, i)) {
				RemoveEquipment(ch, i);
				found = 1;
			}
		}
		if (!found) {
			send_to_char("На вас не надето предметов этого типа.\r\n", ch);
			return;
		}
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg) {
			send_to_char("Снять все вещи какого типа?\r\n", ch);
			return;
		} else {
			found = 0;
			for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
				if (GET_EQ(ch, i)
					&& CAN_SEE_OBJ(ch, GET_EQ(ch, i))
					&& (isname(arg, GET_EQ(ch, i)->get_aliases())
						|| CHECK_CUSTOM_LABEL(arg, GET_EQ(ch, i), ch))) {
					RemoveEquipment(ch, i);
					found = 1;
				}
			}
			if (!found) {
				snprintf(buf, kMaxStringLength, "Вы не используете ни одного '%s'.\r\n", arg);
				send_to_char(buf, ch);
				return;
			}
		}
	} else        // Returns object pointer but we don't need it, just true/false.
	{
		if (!get_object_in_equip_vis(ch, arg, ch->equipment, &i)) {
			// если предмет не найден, то возможно игрок ввел "левая" или "правая"
			if (!str_cmp("правая", arg)) {
				if (!GET_EQ(ch, EEquipPos::kWield)) {
					send_to_char("В правой руке ничего нет.\r\n", ch);
				} else {
					RemoveEquipment(ch, EEquipPos::kWield);
				}
			} else if (!str_cmp("левая", arg)) {
				if (!GET_EQ(ch, EEquipPos::kHold))
					send_to_char("В левой руке ничего нет.\r\n", ch);
				else
					RemoveEquipment(ch, EEquipPos::kHold);
			} else {
				snprintf(buf, kMaxInputLength, "Вы не используете '%s'.\r\n", arg);
				send_to_char(buf, ch);
				return;
			}
		} else {
			RemoveEquipment(ch, i);
		}
	}
	//мы что-то да снимали. значит проверю я доп слот
	if ((obj = GET_EQ(ch, EEquipPos::kQuiver)) && !GET_EQ(ch, EEquipPos::kBoths)) {
		send_to_char("Нету лука, нет и стрел.\r\n", ch);
		act("$n прекратил$g использовать $o3.", false, ch, obj, 0, kToRoom);
		obj_to_char(unequip_char(ch, EEquipPos::kQuiver, CharEquipFlags()), ch);
		return;
	}
}

void do_upgrade(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int weight, add_hr, add_dr, prob, percent, min_mod, max_mod, i;
	bool oldstate;
	if (!ch->get_skill(ESkill::kSharpening)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Что вы хотите заточить?\r\n", ch);
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	};

	if (GET_OBJ_TYPE(obj) != ObjData::ITEM_WEAPON) {
		send_to_char("Вы можете заточить только оружие.\r\n", ch);
		return;
	}

	if (static_cast<ESkill>GET_OBJ_SKILL(obj) == ESkill::kBows) {
		send_to_char("Невозможно заточить этот тип оружия.\r\n", ch);
		return;
	}

	if (obj->has_flag(EObjFlag::kMagic)) {
		send_to_char("Вы не можете заточить заколдованный предмет.\r\n", ch);
		return;
	}

	// Make sure no other (than hitroll & damroll) affections.
	for (i = 0; i < kMaxObjAffect; i++) {
		if ((obj->get_affected(i).location != APPLY_NONE)
			&& (obj->get_affected(i).location != APPLY_HITROLL)
			&& (obj->get_affected(i).location != APPLY_DAMROLL)) {
			send_to_char("Этот предмет не может быть заточен.\r\n", ch);
			return;
		}
	}

	switch (obj->get_material()) {
		case ObjData::MAT_BRONZE:
		case ObjData::MAT_BULAT:
		case ObjData::MAT_IRON:
		case ObjData::MAT_STEEL:
		case ObjData::MAT_SWORDSSTEEL:
		case ObjData::MAT_COLOR:
		case ObjData::MAT_BONE: act("Вы взялись точить $o3.", false, ch, obj, 0, kToChar);
			act("$n взял$u точить $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
			weight = -1;
			break;

		case ObjData::MAT_WOOD:
		case ObjData::MAT_SUPERWOOD: act("Вы взялись стругать $o3.", false, ch, obj, 0, kToChar);
			act("$n взял$u стругать $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
			weight = -1;
			break;

		case ObjData::MAT_SKIN: act("Вы взялись проклепывать $o3.", false, ch, obj, 0, kToChar);
			act("$n взял$u проклепывать $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
			weight = +1;
			break;

		default: sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n", OBJN(obj, ch, 0));
			send_to_char(buf, ch);
			return;
	}
	bool change_weight = true;
	//Заточить повторно можно, но это уменьшает таймер шмотки на 16%
	if (obj->has_flag(EObjFlag::kSharpen)) {
		int timer = obj->get_timer()
			- MAX(1000, obj->get_timer() / 6); // абуз, таймер меньше 6 вычитается 0 бесконечная прокачка умелки
		obj->set_timer(timer);
		change_weight = false;
	} else {
		obj->set_extra_flag(EObjFlag::kSharpen);
		obj->set_extra_flag(EObjFlag::kTransformed); // установили флажок трансформации кодом
	}

	percent = number(1, MUD::Skills()[ESkill::kSharpening].difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kSharpening, nullptr);
	TrainSkill(ch, ESkill::kSharpening, percent <= prob, nullptr);
	if (obj->get_timer() == 0) // не ждем рассыпания на тике
	{
		act("$o не выдержал$G издевательств и рассыпал$U в мелкую пыль...", false, ch, obj, 0, kToChar);
		extract_obj(obj);
		return;
	}
	//При 200% заточки шмотка будет точиться на 4-5 хитролов и 4-5 дамролов
	min_mod = ch->get_trained_skill(ESkill::kSharpening) / 50;
	//С мортами все меньший уровень требуется для макс. заточки
	max_mod = MAX(1, MIN(5, (GetRealLevel(ch) + 5 + GET_REAL_REMORT(ch) / 4) / 6));
	oldstate = check_unlimited_timer(obj); // запомним какая шмотка была до заточки
	if (IS_IMMORTAL(ch)) {
		add_dr = add_hr = 10;
	} else {
		add_dr = add_hr = (max_mod <= min_mod) ? min_mod : number(min_mod, max_mod);
	}
	if (percent > prob || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		act("Но только загубили $S.", false, ch, obj, 0, kToChar);
		add_hr = -add_hr;
		add_dr = -add_dr;
	} else {
		act("И вроде бы неплохо в итоге получилось.", false, ch, obj, 0, kToChar);
	}

	obj->set_affected(0, APPLY_HITROLL, add_hr);
	obj->set_affected(1, APPLY_DAMROLL, add_dr);

	// если шмотка перестала быть нерушимой ставим таймер из прототипа
	if (oldstate && !check_unlimited_timer(obj)) {
		obj->set_timer(obj_proto.at(GET_OBJ_RNUM(obj))->get_timer());
	}
	//Вес меняется только если шмотка еще не была заточена
	//Также вес НЕ меняется если он уже нулевой и должен снизиться
	const auto curent_weight = obj->get_weight();
	if (change_weight && !(curent_weight == 0 && weight < 0)) {
		obj->set_weight(curent_weight + weight);
		IS_CARRYING_W(ch) += weight;
	}
}

void do_armored(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	char arg2[kMaxInputLength];
	int add_ac, prob, percent, i, armorvalue;
	const auto &strengthening = GlobalObjects::strengthening();

	if (!ch->get_skill(ESkill::kArmoring)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	}

	two_arguments(argument, arg, arg2);

	if (!*arg)
		send_to_char("Что вы хотите укрепить?\r\n", ch);

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}

	if (!ObjSystem::is_armor_type(obj)) {
		send_to_char("Вы можете укрепить только доспех.\r\n", ch);
		return;
	}

	if (obj->has_flag(EObjFlag::kMagic) || obj->has_flag(EObjFlag::kArmored)) {
		send_to_char("Вы не можете укрепить этот предмет.\r\n", ch);
		return;
	}

	// Make sure no other affections.
	for (i = 0; i < kMaxObjAffect; i++)
		if (obj->get_affected(i).location != APPLY_NONE) {
			send_to_char("Этот предмет не может быть укреплен.\r\n", ch);
			return;
		}

	if (!OBJWEAR_FLAGGED(obj, (to_underlying(EWearFlag::kBody)
		| to_underlying(EWearFlag::kShoulders)
		| to_underlying(EWearFlag::kHead)
		| to_underlying(EWearFlag::kArms)
		| to_underlying(EWearFlag::kLegs)
		| to_underlying(EWearFlag::kFeet)))) {
		act("$o3 невозможно укрепить.", false, ch, obj, 0, kToChar);
		return;
	}
	if (obj->get_owner() != GET_UNIQUE(ch)) {
		send_to_char(ch, "Укрепить можно только лично сделанный предмет.\r\n");
		return;
	}
	if (!*arg2 && (GET_SKILL(ch, ESkill::kArmoring) >= 100)) {
		send_to_char(ch,
					 "Укажите параметр для улучшения: поглощение, здоровье, живучесть (сопротивление), стойкость (сопротивление), огня (сопротивление), воздуха (сопротивление), воды (сопротивление), земли (сопротивление)\r\n");
		return;
	}
	switch (obj->get_material()) {
		case ObjData::MAT_IRON:
		case ObjData::MAT_STEEL:
		case ObjData::MAT_BULAT: act("Вы принялись закалять $o3.", false, ch, obj, 0, kToChar);
			act("$n принял$u закалять $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
			break;

		case ObjData::MAT_WOOD:
		case ObjData::MAT_SUPERWOOD: act("Вы принялись обшивать $o3 железом.", false, ch, obj, 0, kToChar);
			act("$n принял$u обшивать $o3 железом.", false, ch, obj, 0, kToRoom | kToArenaListen);
			break;

		case ObjData::MAT_SKIN: act("Вы принялись проклепывать $o3.", false, ch, obj, 0, kToChar);
			act("$n принял$u проклепывать $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
			break;

		default: sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n", OBJN(obj, ch, 0));
			send_to_char(buf, ch);
			return;
	}

	percent = number(1, MUD::Skills()[ESkill::kArmoring].difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kArmoring, nullptr);
	TrainSkill(ch, ESkill::kArmoring, percent <= prob, nullptr);
	add_ac = IS_IMMORTAL(ch) ? -20 : -number(1, (GetRealLevel(ch) + 4) / 5);
	if (percent > prob
		|| GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		act("Но только испортили $S.", false, ch, obj, nullptr, kToChar);
		add_ac = -add_ac;
	} else if (GET_SKILL(ch, ESkill::kArmoring) >= 100) {
		if (CompareParam(arg2, "поглощение")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::ABSORBTION);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
//			send_to_char(ch, "увеличиваю поглот на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_ABSORBE, armorvalue);
		} else if (CompareParam(arg2, "здоровье")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::HEALTH);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			send_to_char(ch, "увеличиваю здоровье на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_SAVING_CRITICAL, armorvalue);
		} else if (CompareParam(arg2, "живучесть"))// резисты в - лучше
		{
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::VITALITY);
			armorvalue = -MAX(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			send_to_char(ch, "увеличиваю живучесть на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_RESIST_VITALITY, armorvalue);
		} else if (CompareParam(arg2, "стойкость")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::STAMINA);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			send_to_char(ch, "увеличиваю стойкость на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_SAVING_STABILITY, armorvalue);
		} else if (CompareParam(arg2, "воздуха")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::AIR_PROTECTION);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
//			send_to_char(ch, "увеличиваю сопр воздуха на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_RESIST_AIR, armorvalue);
		} else if (CompareParam(arg2, "воды")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::WATER_PROTECTION);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
//			send_to_char(ch, "увеличиваю сопр воды на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_RESIST_WATER, armorvalue);
		} else if (CompareParam(arg2, "огня")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::FIRE_PROTECTION);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
//			send_to_char(ch, "увеличиваю сопр огню на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_RESIST_FIRE, armorvalue);
		} else if (CompareParam(arg2, "земли")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::EARTH_PROTECTION);
			armorvalue = MAX(0, number(armorvalue, armorvalue - 2));
//			send_to_char(ch, "увеличиваю сопр земли на %d\r\n", armorvalue);
			obj->set_affected(1, APPLY_RESIST_EARTH, armorvalue);
		} else {
			send_to_char(ch, "Но не поняли что улучшать.\r\n");
			return;
		}
		armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::TIMER);
		int timer =
			obj->get_timer() * strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::TIMER) / 100;
		obj->set_timer(timer);
//		send_to_char(ch, "увеличиваю таймер на %d%, устанавливаю таймер %d\r\n", armorvalue, timer);
		armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::ARMOR);
//		send_to_char(ch, "увеличиваю армор на %d скилл равен %d  значение берем %d\r\n", armorvalue, GET_SKILL(ch, ESkill::kArmoring), (GET_SKILL(ch, ESkill::kArmoring) / 10 * 10) );
		obj->set_affected(2, APPLY_ARMOUR, armorvalue);
		obj->set_extra_flag(EObjFlag::kArmored);
		obj->set_extra_flag(EObjFlag::kTransformed); // установили флажок трансформации кодом
	}
	obj->set_affected(0, APPLY_AC, add_ac);
}

void do_fire(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int percent, prob;
	if (!ch->get_skill(ESkill::kCampfire)) {
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (ch->ahorse()) {
		send_to_char("Верхом это будет затруднительно.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
		send_to_char("Вы ничего не видите!\r\n", ch);
		return;
	}

	if (world[ch->in_room]->fires) {
		send_to_char("Здесь уже горит огонь.\r\n", ch);
		return;
	}

	if (SECT(ch->in_room) == kSectInside ||
		SECT(ch->in_room) == kSectCity ||
		SECT(ch->in_room) == kSectWaterSwim ||
		SECT(ch->in_room) == kSectWaterNoswim ||
		SECT(ch->in_room) == kSectOnlyFlying ||
		SECT(ch->in_room) == kSectUnderwater || SECT(ch->in_room) == kSectSecret) {
		send_to_char("В этой комнате нельзя разжечь костер.\r\n", ch);
		return;
	}

	if (!check_moves(ch, FIRE_MOVES))
		return;

	percent = number(1, MUD::Skills()[ESkill::kCampfire].difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kCampfire, 0);
	if (percent > prob) {
		send_to_char("Вы попытались разжечь костер, но у вас ничего не вышло.\r\n", ch);
		return;
	} else {
		world[ch->in_room]->fires = MAX(0, (prob - percent) / 5) + 1;
		send_to_char("Вы набрали хворосту и разожгли огонь.\n\r", ch);
		act("$n развел$g огонь.", false, ch, 0, 0, kToRoom | kToArenaListen);
		ImproveSkill(ch, ESkill::kCampfire, true, 0);
	}
}

#include "game_magic/magic_rooms.h"
void do_extinguish(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *caster;
	int tp, lag = 0;
	const char *targets[] =
		{
			"костер",
			"пламя",
			"огонь",
			"fire",
			"метку",
			"надпись",
			"руны",
			"label",
			"\n"
		};

	if (ch->is_npc()) {
		return;
	}

	one_argument(argument, arg);

	if ((!*arg) || ((tp = search_block(arg, targets, false)) == -1)) {
		send_to_char("Что вы хотите затоптать?\r\n", ch);
		return;
	}
	tp >>= 2;

	switch (tp) {
		case 0:
			if (world[ch->in_room]->fires) {
				if (world[ch->in_room]->fires < 5)
					--world[ch->in_room]->fires;
				else
					world[ch->in_room]->fires = 4;
				send_to_char("Вы затоптали костер.\r\n", ch);
				act("$n затоптал$g костер.", false, ch, 0, 0, kToRoom | kToArenaListen);
				if (world[ch->in_room]->fires == 0) {
					send_to_char("Костер потух.\r\n", ch);
					act("Костер потух.", false, ch, 0, 0, kToRoom | kToArenaListen);
				}
				lag = 1;
			} else {
				send_to_char("А тут топтать и нечего :)\r\n", ch);
			}
			break;

		case 1: const auto &room = world[ch->in_room];
			auto aff_i = room->affected.end();
			auto aff_first = room->affected.end();

			//Find own rune label or first run label in room
			for (auto affect_it = room->affected.begin(); affect_it != room->affected.end(); ++affect_it) {
				if (affect_it->get()->type == kSpellRuneLabel) {
					if (affect_it->get()->caster_id == GET_ID(ch)) {
						aff_i = affect_it;
						break;
					}

					if (aff_first == room->affected.end()) {
						aff_first = affect_it;
					}
				}
			}

			if (aff_i == room->affected.end()) {
				//Own rune label not found. Use first in room
				aff_i = aff_first;
			}

			if (aff_i != room->affected.end()
				&& (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
					|| IS_IMMORTAL(ch)
					|| PRF_FLAGGED(ch, EPrf::kCoderinfo))) {
				send_to_char("Шаркнув несколько раз по земле, вы стерли светящуюся надпись.\r\n", ch);
				act("$n шаркнул$g несколько раз по светящимся рунам, полностью их уничтожив.",
					false,
					ch,
					0,
					0,
					kToRoom | kToArenaListen);

				const auto &aff = *aff_i;
				if (GET_ID(ch) != aff->caster_id) { //чел стирает не свою метку - вай, нехорошо
					//Ищем кастера по миру
					caster = find_char(aff->caster_id);
					//Если кастер онлайн - выдаем деятелю БД как за воровство
					if (caster && !same_group(ch, caster)) {
						pk_thiefs_action(ch, caster);
						sprintf(buf,
								"Послышался далекий звук лопнувшей струны, и перед вами промельнул призрачный облик %s.\r\n",
								GET_PAD(ch, 1));
						send_to_char(buf, caster);
					}
				}
				room_spells::RemoveAffect(world[ch->in_room], aff_i);
				lag = 3;
			} else {
				send_to_char("А тут топтать и нечего :)\r\n", ch);
			}
			break;
	}

	//Выдадим-ка лаг за эти дела.
	if (!WAITLESS(ch)) {
		WAIT_STATE(ch, lag * kPulseViolence);
	}
}

void do_firstaid(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int success = false, need = false, spellnum = 0;
	struct TimedSkill timed;

	if (!ch->get_skill(ESkill::kFirstAid)) {
		send_to_char("Вам следует этому научиться.\r\n", ch);
		return;
	}
	if (!IS_GOD(ch) && IsTimedBySkill(ch, ESkill::kFirstAid)) {
		send_to_char("Так много лечить нельзя - больных не останется.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	CharData *vict;
	if (!*arg) {
		vict = ch;
	} else {
		vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
		if (!vict) {
			send_to_char("Кого вы хотите подлечить?\r\n", ch);
			return;
		}
	}

	if (vict->get_fighting()) {
		act("$N сражается, $M не до ваших телячьих нежностей.", false, ch, 0, vict, kToChar);
		return;
	}
	if (vict->is_npc() && !IS_CHARMICE(vict)) {
		send_to_char("Вы не красный крест лечить всех подряд.\r\n", ch);
		return;
	}
	int percent = number(1, MUD::Skills()[ESkill::kFirstAid].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kFirstAid, vict);

	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || GET_GOD_FLAG(vict, EGf::kGodsLike)) {
		percent = prob;
	}
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = 0;
	}
	success = (prob >= percent);
	need = false;

	if ((GET_REAL_MAX_HIT(vict) > 0
		&& (GET_HIT(vict) * 100 / GET_REAL_MAX_HIT(vict)) < 31)
		|| (GET_REAL_MAX_HIT(vict) <= 0
			&& GET_HIT(vict) < GET_REAL_MAX_HIT(vict))
		|| (GET_HIT(vict) < GET_REAL_MAX_HIT(vict)
			&& can_use_feat(ch, HEALER_FEAT))) {
		need = true;
		if (success) {
			if (!PRF_FLAGGED(ch, EPrf::kTester)) {
				int dif = GET_REAL_MAX_HIT(vict) - GET_HIT(vict);
				int add = MIN(dif, (dif * (prob - percent) / 100) + 1);
				GET_HIT(vict) += add;
			} else {
				percent = CalcCurrentSkill(ch, ESkill::kFirstAid, vict);
				prob = GetRealLevel(ch) * percent * 0.5;
				send_to_char(ch, "&RУровень цели %d Отхилено %d хитов, скилл %d\r\n", GetRealLevel(vict), prob, percent);
				GET_HIT(vict) += prob;
				GET_HIT(vict) = MIN(GET_HIT(vict), GET_REAL_MAX_HIT(vict));
				update_pos(vict);
			}
		}
	}

	int count = 0;
	if (PRF_FLAGGED(ch, EPrf::kTester)) {
		count = (GET_SKILL(ch, ESkill::kFirstAid) - 20) / 30;
		send_to_char(ch, "Снимаю %d аффектов\r\n", count);

		const auto remove_count = vict->remove_random_affects(count);
		send_to_char(ch, "Снято %ld аффектов\r\n", remove_count);

		//
		need = true;
		prob = true;
	} else {
		count = MIN(MAX_FIRSTAID_REMOVE, MAX_FIRSTAID_REMOVE * prob / 100);

		for (percent = 0, prob = need; !need && percent < MAX_FIRSTAID_REMOVE && RemoveSpell(percent); percent++) {
			if (affected_by_spell(vict, RemoveSpell(percent))) {
				need = true;
				if (percent < count) {
					spellnum = RemoveSpell(percent);
					prob = true;
				}
			}
		}
	}

	if (!need) {
		act("$N в лечении не нуждается.", false, ch, 0, vict, kToChar);
	} else if (!prob) {
		act("У вас не хватит умения вылечить $N3.", false, ch, 0, vict, kToChar);
	} else {
		timed.skill = ESkill::kFirstAid;
		timed.time = IS_IMMORTAL(ch) ? 2 : IS_PALADINE(ch) ? 4 : IS_SORCERER(ch) ? 2 : 6;
		timed_to_char(ch, &timed);
		if (vict != ch) {
			ImproveSkill(ch, ESkill::kFirstAid, success, 0);
			if (success) {
				act("Вы оказали первую помощь $N2.", false, ch, 0, vict, kToChar);
				act("$N оказал$G вам первую помощь.", false, vict, 0, ch, kToChar);
				act("$n оказал$g первую помощь $N2.", true, ch, 0, vict, kToNotVict | kToArenaListen);
				if (spellnum)
					affect_from_char(vict, spellnum);
			} else {
				act("Вы безрезультатно попытались оказать первую помощь $N2.",
					false, ch, 0, vict, kToChar);
				act("$N безрезультатно попытал$U оказать вам первую помощь.",
					false, vict, 0, ch, kToChar);
				act("$n безрезультатно попытал$u оказать первую помощь $N2.",
					true, ch, 0, vict, kToNotVict | kToArenaListen);
			}
		} else {
			if (success) {
				act("Вы оказали себе первую помощь.", false, ch, 0, 0, kToChar);
				act("$n оказал$g себе первую помощь.", false, ch, 0, 0, kToRoom | kToArenaListen);
				if (spellnum)
					affect_from_char(vict, spellnum);
			} else {
				act("Вы безрезультатно попытались оказать себе первую помощь.",
					false, ch, 0, vict, kToChar);
				act("$n безрезультатно попытал$u оказать себе первую помощь.",
					false, ch, 0, vict, kToRoom | kToArenaListen);
			}
		}
	}
}

void do_poisoned(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_skill(ESkill::kPoisoning)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	}

	argument = one_argument(argument, arg);
	skip_spaces(&argument);

	if (!*arg) {
		send_to_char("Что вы хотите отравить?\r\n", ch);
		return;
	} else if (!*argument) {
		send_to_char("Из чего вы собираете взять яд?\r\n", ch);
		return;
	}

	ObjData *weapon = 0;
	CharData *dummy = 0;
	int result = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &weapon);

	if (!weapon || !result) {
		send_to_char(ch, "У вас нет \'%s\'.\r\n", arg);
		return;
	} else if (GET_OBJ_TYPE(weapon) != ObjData::ITEM_WEAPON) {
		send_to_char("Вы можете нанести яд только на оружие.\r\n", ch);
		return;
	}

	ObjData *cont = get_obj_in_list_vis(ch, argument, ch->carrying);
	if (!cont) {
		send_to_char(ch, "У вас нет \'%s\'.\r\n", argument);
		return;
	} else if (GET_OBJ_TYPE(cont) != ObjData::ITEM_DRINKCON) {
		send_to_char(ch, "%s не является емкостью.\r\n", cont->get_PName(0).c_str());
		return;
	} else if (GET_OBJ_VAL(cont, 1) <= 0) {
		send_to_char(ch, "В %s нет никакой жидкости.\r\n", cont->get_PName(5).c_str());
		return;
	} else if (!poison_in_vessel(GET_OBJ_VAL(cont, 2))) {
		send_to_char(ch, "В %s нет подходящего яда.\r\n", cont->get_PName(5).c_str());
		return;
	}

	int cost = MIN(GET_OBJ_VAL(cont, 1), GetRealLevel(ch) <= 10 ? 1 : GetRealLevel(ch) <= 20 ? 2 : 3);
	cont->set_val(1, cont->get_val(1) - cost);
	weight_change_object(cont, -cost);
	if (!GET_OBJ_VAL(cont, 1)) {
		name_from_drinkcon(cont);
	}

	set_weap_poison(weapon, cont->get_val(2));

	snprintf(buf, sizeof(buf), "Вы осторожно нанесли немного %s на $o3.", drinks[cont->get_val(2)]);
	act(buf, false, ch, weapon, 0, kToChar);
	act("$n осторожно нанес$q яд на $o3.", false, ch, weapon, 0, kToRoom | kToArenaListen);
}

void do_repair(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int prob, percent = 0, decay;
	struct TimedSkill timed;

	if (!ch->get_skill(ESkill::kRepair)) {
		send_to_char("Вы не умеете этого.\r\n", ch);
		return;
	}
	if (IsTimedBySkill(ch, ESkill::kRepair)) {
		send_to_char("У вас недостаточно сил для ремонта.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (ch->get_fighting()) {
		send_to_char("Вы не можете сделать это в бою!\r\n", ch);
		return;
	}

	if (!*arg) {
		send_to_char("Что вы хотите ремонтировать?\r\n", ch);
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	};

	if (GET_OBJ_MAX(obj) <= GET_OBJ_CUR(obj)) {
		act("$o в ремонте не нуждается.", false, ch, obj, 0, kToChar);
		return;
	}
	if (GET_OBJ_TYPE(obj) != ObjData::ITEM_WEAPON
		&& !ObjSystem::is_armor_type(obj)) {
		send_to_char("Вы можете отремонтировать только оружие или броню.\r\n", ch);
		return;
	}

	prob = number(1, MUD::Skills()[ESkill::kRepair].difficulty);
	percent = CalcCurrentSkill(ch, ESkill::kRepair, nullptr);
	TrainSkill(ch, ESkill::kRepair, prob <= percent, nullptr);
	if (prob > percent) {
//Polos.repair_bug
//Потому что 0 уничтожает шмотку полностью даже при скиле 100+ и
//состоянии шмотки <очень хорошо>
		if (!percent) {
			percent = ch->get_skill(ESkill::kRepair) / 10;
		}
//-Polos.repair_bug
		obj->set_current_durability(MAX(0, obj->get_current_durability() * percent / prob));
		if (obj->get_current_durability()) {
			act("Вы попытались починить $o3, но сломали $S еще больше.", false, ch, obj, 0, kToChar);
			act("$n попытал$u починить $o3, но сломал$g $S еще больше.", false, ch, obj, 0, kToRoom | kToArenaListen);
			decay = (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 10;
			decay = MAX(1, MIN(decay, GET_OBJ_MAX(obj) / 20));
			if (GET_OBJ_MAX(obj) > decay) {
				obj->set_maximum_durability(obj->get_maximum_durability() - decay);
			} else {
				obj->set_maximum_durability(1);
			}
		} else {
			act("Вы окончательно доломали $o3.", false, ch, obj, 0, kToChar);
			act("$n окончательно доломал$g $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
			extract_obj(obj);
		}
	} else {
		timed.skill = ESkill::kRepair;
		// timed.time - это unsigned char, поэтому при уходе в минус будет вынос на 255 и ниже
		int modif = ch->get_skill(ESkill::kRepair) / 7 + number(1, 5);
		timed.time = MAX(1, 25 - modif);
		timed_to_char(ch, &timed);
		obj->set_current_durability(MIN(GET_OBJ_MAX(obj), GET_OBJ_CUR(obj) * percent / prob + 1));
		send_to_char(ch, "Теперь %s выгляд%s лучше.\r\n", obj->get_PName(0).c_str(), GET_OBJ_POLY_1(ch, obj));
		act("$n умело починил$g $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
	}
}

bool skill_to_skin(CharData *mob, CharData *ch) {
	int num;
	switch (GetRealLevel(mob) / 11) {
		case 0: num = 15 * animals_levels[0] / 2201; // приводим пропорцией к количеству зверья на 15.11.2015 в мире
			if (number(1, 100) <= num)
				return true;
			break;
		case 1:
			if (ch->get_skill(ESkill::kSkinning) >= 40) {
				num = 20 * animals_levels[1] / 701;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				send_to_char(buf, ch);
				return false;
			}

			break;
		case 2:
			if (ch->get_skill(ESkill::kSkinning) >= 80) {
				num = 10 * animals_levels[2] / 594;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				send_to_char(buf, ch);
				return false;
			}
			break;

		case 3:
			if (ch->get_skill(ESkill::kSkinning) >= 120) {
				num = 8 * animals_levels[3] / 209;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				send_to_char(buf, ch);
				return false;
			}
			break;

		case 4:
			if (ch->get_skill(ESkill::kSkinning) >= 160) {
				num = 25 * animals_levels[4] / 20;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				send_to_char(buf, ch);
				return false;
			}
			break;
			//TODO: Добавить для мобов выше 54 уровня
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		default: return false;
	}
	return false;
}

void do_makefood(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_skill(ESkill::kSkinning)) {
		send_to_char("Вы не умеете этого.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		send_to_char("Что вы хотите освежевать?\r\n", ch);
		return;
	}

	auto obj = get_obj_in_list_vis(ch, arg, ch->carrying);
	if (!obj) {
		obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents);
		if (!obj) {
			snprintf(buf, kMaxInputLength, "Вы не видите здесь '%s'.\r\n", arg);
			send_to_char(buf, ch);
			return;
		}
	}

	const auto mobn = GET_OBJ_VAL(obj, 2);
	if (!IS_CORPSE(obj) || mobn < 0) {
		act("Вы не сможете освежевать $o3.", false, ch, obj, 0, kToChar);
		return;
	}

	const auto mob = (mob_proto + real_mobile(mobn));
	mob->set_normal_morph();

	if (!IS_IMMORTAL(ch)
		&& GET_RACE(mob) != ENpcRace::kAnimal
		&& GET_RACE(mob) != ENpcRace::kReptile
		&& GET_RACE(mob) != ENpcRace::kFish
		&& GET_RACE(mob) != ENpcRace::kBird
		&& GET_RACE(mob) != ENpcRace::kBeastman) {
		send_to_char("Этот труп невозможно освежевать.\r\n", ch);
		return;
	}

	if (GET_WEIGHT(mob) < 11) {
		send_to_char("Этот труп слишком маленький, ничего не получится.\r\n", ch);
		return;
	}

	const auto prob = number(1, MUD::Skills()[ESkill::kSkinning].difficulty);
	const auto percent = CalcCurrentSkill(ch, ESkill::kSkinning, mob)
		+ number(1, GET_REAL_DEX(ch)) + number(1, GET_REAL_STR(ch));
	TrainSkill(ch, ESkill::kSkinning, percent <= prob, mob);

	ObjData::shared_ptr tobj;
	if (GET_SKILL(ch, ESkill::kSkinning) > 150 && number(1, 200) == 1) // артефакт
	{
		tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get_artefact_key());
	} else {
		tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.random_key());
	}

	if (prob > percent || !tobj) {
		act("Вы не сумели освежевать $o3.", false, ch, obj, 0, kToChar);
		act("$n попытал$u освежевать $o3, но неудачно.", false, ch, obj, 0, kToRoom | kToArenaListen);
	} else {
		act("$n умело освежевал$g $o3.", false, ch, obj, 0, kToRoom | kToArenaListen);
		act("Вы умело освежевали $o3.", false, ch, obj, 0, kToChar);

		dl_load_obj(obj, mob, ch, DL_SKIN);

		std::vector<ObjData *> entrails;
		entrails.push_back(tobj.get());

		if (GET_RACE(mob) == ENpcRace::kAnimal) // шкуры только с животных
		{
			if (WAITLESS(ch) || skill_to_skin(mob, ch)) {
				entrails.push_back(create_skin(mob, ch));
			}
		}

		entrails.push_back(try_make_ingr(mob, 1000 - ch->get_skill(ESkill::kSkinning) * 2));  // ингры со всех

		for (const auto &it : entrails) {
			if (it) {
				if (obj->get_carried_by() == ch) {
					can_carry_obj(ch, it);
				} else {
					obj_to_room(it, ch->in_room);
				}
			}
		}
	}

	extract_obj(obj);
}

void feed_charmice(CharData *ch, char *arg) {
	ObjData *obj;
	int max_charm_duration = 1;
	int chance_to_eat = 0;
	struct Follower *k;
	int reformed_hp_summ = 0;

	obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents);

	if (!obj || !IS_CORPSE(obj) || !ch->has_master()) {
		return;
	}

	for (k = ch->get_master()->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->ch, EAffectFlag::AFF_CHARM)
			&& k->ch->get_master() == ch->get_master()) {
			reformed_hp_summ += get_reformed_charmice_hp(ch->get_master(), k->ch, kSpellAnimateDead);
		}
	}

	if (reformed_hp_summ >= get_player_charms(ch->get_master(), kSpellAnimateDead)) {
		send_to_char("Вы не можете управлять столькими последователями.\r\n", ch->get_master());
		extract_char(ch, false);
		return;
	}

	int mob_level = 1;
	// труп не игрока
	if (GET_OBJ_VAL(obj, 2) != -1) {
		mob_level = GetRealLevel(mob_proto + real_mobile(GET_OBJ_VAL(obj, 2)));
	}
	const int max_heal_hp = 3 * mob_level;
	chance_to_eat = (100 - 2 * mob_level) / 2;
	//Added by Ann
	if (affected_by_spell(ch->get_master(), kSpellFascination)) {
		chance_to_eat -= 30;
	}
	//end Ann
	if (number(1, 100) < chance_to_eat) {
		act("$N подавил$U и начал$G сильно кашлять.", true, ch, nullptr, ch, kToRoom | kToArenaListen);
		GET_HIT(ch) -= 3 * mob_level;
		update_pos(ch);
		// Подавился насмерть.
		if (GET_POS(ch) == EPosition::kDead) {
			die(ch, nullptr);
		}
		extract_obj(obj);
		return;
	}
	if (weather_info.moon_day < 14) {
		max_charm_duration =
			CalcDuration(ch, GET_REAL_WIS(ch->get_master()) - 6 + number(0, weather_info.moon_day % 14), 0, 0, 0, 0);
	} else {
		max_charm_duration =
			CalcDuration(ch,
						 GET_REAL_WIS(ch->get_master()) - 6 + number(0, 14 - weather_info.moon_day % 14),
						 0, 0, 0, 0);
	}

	Affect<EApplyLocation> af;
	af.type = kSpellCharm;
	af.duration = MIN(max_charm_duration, (int) (mob_level * max_charm_duration / 30));
	af.modifier = 0;
	af.location = EApplyLocation::APPLY_NONE;
	af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
	af.battleflag = 0;

	affect_join_fspell(ch, af);

	act("Громко чавкая, $N сожрал$G труп.", true, ch, obj, ch, kToRoom | kToArenaListen);
	act("Похоже, лакомство пришлось по вкусу.", true, ch, nullptr, ch->get_master(), kToVict);
	act("От омерзительного зрелища вас едва не вывернуло.",
		true, ch, nullptr, ch->get_master(), kToNotVict | kToArenaListen);

	if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
		GET_HIT(ch) = MIN(GET_HIT(ch) + MIN(max_heal_hp, GET_MAX_HIT(ch)), GET_MAX_HIT(ch));
	}

	if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
		act("$n сыто рыгнул$g и благодарно посмотрел$g на вас.", true, ch, nullptr, ch->get_master(), kToVict);
		act("$n сыто рыгнул$g и благодарно посмотрел$g на $N3.",
			true, ch, nullptr, ch->get_master(), kToNotVict | kToArenaListen);
	}

	extract_obj(obj);
}

// чтоб не абузили длину. персональные пофиг, а клановые не надо.
#define MAX_LABEL_LENGTH 32
void do_custom_label(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	char arg3[kMaxInputLength];
	char arg4[kMaxInputLength];

	ObjData *target = nullptr;
	int erase_only = 0; // 0 -- наносим новую метку, 1 -- удаляем старую
	int clan = 0; // клан режим, если единица. персональный, если не
	int no_target = 0; // красиво сделать не выйдет, будем через флаги

	char *objname = nullptr;
	char *labels = nullptr;

	if (ch->is_npc())
		return;

	half_chop(argument, arg1, arg2);

	if (!strlen(arg1))
		no_target = 1;
	else {
		if (!strcmp(arg1, "клан")) { // если в arg1 "клан", то в arg2 ищем название объекта и метки
			clan = 1;
			if (strlen(arg2)) {
				half_chop(arg2, arg3, arg4); // в arg3 получаем название объекта
				objname = str_dup(arg3);
				if (strlen(arg4))
					labels = str_dup(arg4);
				else
					erase_only = 1;
			} else {
				no_target = 1;
			}
		} else { // слова "клан" не нашли, значит, ожидаем в arg1 сразу имя объекта и метки в arg2
			if (strlen(arg1)) {
				objname = str_dup(arg1);
				if (strlen(arg2))
					labels = str_dup(arg2);
				else
					erase_only = 1;
			} else {
				no_target = 1;
			}
		}
	}

	if (no_target) {
		send_to_char("На чем царапаем?\r\n", ch);
	} else {
		if (!(target = get_obj_in_list_vis(ch, objname, ch->carrying))) {
			sprintf(buf, "У вас нет \'%s\'.\r\n", objname);
			send_to_char(buf, ch);
		} else {
			if (erase_only) {
				target->remove_custom_label();
				act("Вы затерли надписи на $o5.", false, ch, target, 0, kToChar);
			} else if (labels) {
				if (strlen(labels) > MAX_LABEL_LENGTH)
					labels[MAX_LABEL_LENGTH] = '\0';

				// убираем тильды
				for (int i = 0; labels[i] != '\0'; i++)
					if (labels[i] == '~')
						labels[i] = '-';

				std::shared_ptr<custom_label> label(new custom_label());
				label->label_text = str_dup(labels);
				label->author = ch->get_idnum();
				label->author_mail = str_dup(GET_EMAIL(ch));

				const char *msg = "Вы покрыли $o3 каракулями, которые никто кроме вас не разберет.";
				if (clan && ch->player_specials->clan) {
					label->clan = str_dup(ch->player_specials->clan->GetAbbrev());
					msg = "Вы покрыли $o3 каракулями, понятными разве что вашим соратникам.";
				}
				target->set_custom_label(label);
				act(msg, false, ch, target, 0, kToChar);
			}
		}
	}

	if (objname) {
		free(objname);
	}

	if (labels) {
		free(labels);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
