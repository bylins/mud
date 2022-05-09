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
#include "game_fight/fight.h"
#include "handler.h"
#include "house.h"
#include "liquid.h"
#include "game_mechanics/named_stuff.h"
#include "game_fight/pk.h"
#include "game_skills/poison.h"
#include "meat_maker.h"
#include "utils/utils_char_obj.inl"
#include "structs/global_objects.h"

// from act.informative.cpp
char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);

// local functions
int can_take_obj(CharData *ch, ObjData *obj);
void get_check_money(CharData *ch, ObjData *obj, ObjData *cont);
int perform_get_from_room(CharData *ch, ObjData *obj);
void perform_give_gold(CharData *ch, CharData *vict, int amount);
void perform_give(CharData *ch, CharData *vict, ObjData *obj);
void PerformDrop(CharData *ch, ObjData *obj);
CharData *give_find_vict(CharData *ch, char *local_arg);
void weight_change_object(ObjData *obj, int weight);
int perform_put(CharData *ch, ObjData::shared_ptr obj, ObjData *cont);
void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot);
void perform_wear(CharData *ch, ObjData *obj, int equip_pos);
int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode);
void RemoveEquipment(CharData *ch, int pos);
int invalid_anti_class(CharData *ch, const ObjData *obj);
void feed_charmice(CharData *ch, char *local_arg);

int invalid_unique(CharData *ch, const ObjData *obj);
bool unique_stuff(const CharData *ch, const ObjData *obj);

// from class.cpp
int invalid_no_class(CharData *ch, const ObjData *obj);

void do_split(CharData *ch, char *argument, int cmd, int subcmd);
void do_split(CharData *ch, char *argument, int cmd, int subcmd, int currency);
void do_remove(CharData *ch, char *argument, int cmd, int subcmd);
void do_put(CharData *ch, char *argument, int cmd, int subcmd);
void do_get(CharData *ch, char *argument, int cmd, int subcmd);
void DoDrop(CharData *ch, char *argument, int, int);
void do_eat(CharData *ch, char *argument, int cmd, int subcmd);
void do_wear(CharData *ch, char *argument, int cmd, int subcmd);
void do_wield(CharData *ch, char *argument, int cmd, int subcmd);
void do_grab(CharData *ch, char *argument, int cmd, int subcmd);
void DoSharpening(CharData *ch, char *argument, int, int);
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
	else if (GET_OBJ_TYPE(obj) == EObjType::kContainer && obj->get_contains()) {
		SendMsgToChar(ch, "В %s что-то лежит.\r\n", obj->get_PName(5).c_str());
	} 
	else if (obj->has_flag(EObjFlag::kNodrop)) {
		act("Неведомая сила помешала положить $o3 в $O3.", false, ch, obj.get(), cont, kToChar);
	} 
	else if (obj->has_flag(EObjFlag::kZonedacay) || obj->get_type() == EObjType::kKey) {
		act("Неведомая сила помешала положить $o3 в $O3.", false, ch, obj.get(), cont, kToChar);
	} 
	else {
		ExtractObjFromChar(obj.get());
		// чтобы там по 1 куне гор не было, чару тож возвращается на счет, а не в инвентарь кучкой
		if (obj->get_type() == EObjType::kMoney && obj->get_rnum() == 0) {
			ObjData *temp, *obj_next;
			for (temp = cont->get_contains(); temp; temp = obj_next) {
				obj_next = temp->get_next_content();
				if (GET_OBJ_TYPE(temp) == EObjType::kMoney) {
					// тут можно просто в поле прибавить, но там описание для кун разное от кол-ва
					int money = GET_OBJ_VAL(temp, 0);
					money += GET_OBJ_VAL(obj, 0);
					ExtractObjFromObj(temp);
					ExtractObjFromWorld(temp);
					ExtractObjFromObj(obj.get());
					ExtractObjFromWorld(obj.get());
					obj = create_money(money);
					if (!obj) {
						return 0;
					}
					break;
				}
			}
		}

		PlaceObjIntoObj(obj.get(), cont);

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

void do_put(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	char arg4[kMaxInputLength];
	ObjData *next_obj, *cont;
	CharData *tmp_char;
	int obj_dotmode, cont_dotmode, found = 0, howmany = 1, money_mode = false;
	char *theobj, *thecont, *theplace;
	int where_bits = EFind::kObjInventory | EFind::kObjEquip | EFind::kObjRoom;

	argument = two_arguments(argument, arg1, arg2);
	argument = two_arguments(argument, arg3, arg4);

	if (is_number(arg1)) {
		howmany = std::stoi(arg1);
		theobj = arg2;
		thecont = arg3;
		theplace = arg4;
	} else {
		theobj = arg1;
		thecont = arg2;
		theplace = arg3;
	}

	if (isname(theplace, "земля комната room ground"))
		where_bits = EFind::kObjRoom;
	else if (isname(theplace, "инвентарь inventory"))
		where_bits = EFind::kObjInventory;
	else if (isname(theplace, "экипировка equipment"))
		where_bits = EFind::kObjEquip;

	if (theobj && (!strn_cmp("coin", theobj, 4) || !strn_cmp("кун", theobj, 3))) {
		money_mode = true;
		if (howmany <= 0) {
			SendMsgToChar("Следует указать чиста конкретную сумму.\r\n", ch);
			return;
		}
		if (ch->get_gold() < howmany) {
			SendMsgToChar("Нет у вас такой суммы.\r\n", ch);
			return;
		}
		obj_dotmode = kFindIndiv;
	} else
		obj_dotmode = find_all_dots(theobj);

	cont_dotmode = find_all_dots(thecont);

	if (!*theobj)
		SendMsgToChar("Положить что и куда?\r\n", ch);
	else if (cont_dotmode != kFindIndiv)
		SendMsgToChar("Вы можете положить вещь только в один контейнер.\r\n", ch);
	else if (!*thecont) {
		sprintf(buf, "Куда вы хотите положить '%s'?\r\n", theobj);
		SendMsgToChar(buf, ch);
	} else {
		generic_find(thecont, where_bits, ch, &tmp_char, &cont);
		if (!cont) {
			sprintf(buf, "Вы не видите здесь '%s'.\r\n", thecont);
			SendMsgToChar(buf, ch);
		} else if (GET_OBJ_TYPE(cont) != EObjType::kContainer) {
			act("В $o3 нельзя ничего положить.", false, ch, cont, nullptr, kToChar);
		} else if (OBJVAL_FLAGGED(cont, EContainerFlag::kShutted)) {
			act("$o0 закрыт$A!", false, ch, cont, nullptr, kToChar);
		} else {
			if (obj_dotmode == kFindIndiv)    // put <obj> <container>
			{
				if (money_mode) {
					if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem)) {
						act("Неведомая сила помешала вам сделать это!!", false,
							ch, nullptr, nullptr, kToChar);
						return;
					}

					const auto obj = create_money(howmany);

					if (!obj) {
						return;
					}

					PlaceObjToInventory(obj.get(), ch);
					ch->remove_gold(howmany);

					// если положить не удалось - возвращаем все взад
					if (perform_put(ch, obj, cont)) {
						ExtractObjFromChar(obj.get());
						ExtractObjFromWorld(obj.get());
						ch->add_gold(howmany);
						return;
					}
				} else {
					auto obj = get_obj_in_list_vis(ch, theobj, ch->carrying);
					if (!obj) {
						sprintf(buf, "У вас нет '%s'.\r\n", theobj);
						SendMsgToChar(buf, ch);
					} else if (obj == cont) {
						SendMsgToChar("Вам будет трудно запихнуть вещь саму в себя.\r\n", ch);
					} else {
						while (obj && howmany--) {
							auto next_item = obj->get_next_content();
							const auto object_ptr = world_objects.get_by_raw_ptr(obj);
							if (perform_put(ch, object_ptr, cont) == 1) {
								return;
							}
							obj = get_obj_in_list_vis(ch, theobj, next_item);
						}
					}
				}
			} else {
				for (auto obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->get_next_content();
					if (obj != cont
						&& CAN_SEE_OBJ(ch, obj)
						&& (obj_dotmode == kFindAll
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
					if (obj_dotmode == kFindAll)
						SendMsgToChar("Чтобы положить что-то ненужное нужно купить что-то ненужное.\r\n", ch);
					else {
						sprintf(buf, "Вы не видите ничего похожего на '%s'.\r\n", theobj);
						SendMsgToChar(buf, ch);
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
		SendMsgToChar("Откуда брать стрелы?\r\n", ch);
		return;
	}
	if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		SendMsgToChar("У вас нет этого!\r\n", ch);
		return;
	}
	if (GET_OBJ_TYPE(from_obj) != EObjType::kMagicArrow) {
		SendMsgToChar("И как вы себе это представляете?\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("Пусто.", false, ch, from_obj, nullptr, kToChar);
		return;
	}
	if (!*arg2) {
		SendMsgToChar("Куда вы хотите их засунуть?\r\n", ch);
		return;
	}
	if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		SendMsgToChar("Вы не можете этого найти!\r\n", ch);
		return;
	}
	if (!((GET_OBJ_TYPE(to_obj) == EObjType::kMagicContaner)
		|| GET_OBJ_TYPE(to_obj) == EObjType::kMagicArrow)) {
		SendMsgToChar("Вы не сможете в это сложить стрелы.\r\n", ch);
		return;
	}

	if (to_obj == from_obj) {
		SendMsgToChar("Нечем заняться? На печи ездить еще не научились?\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 2) >= GET_OBJ_VAL(to_obj, 1)) {
		SendMsgToChar("Там нет места.\r\n", ch);
		return;
	} else //вроде прошли все проверки. начинаем перекладывать
	{
		if (GET_OBJ_VAL(from_obj, 0) != GET_OBJ_VAL(to_obj, 0)) {
			SendMsgToChar("Хамово ремесло еще не известно на руси.\r\n", ch);
			return;
		}
		int t1 = GET_OBJ_VAL(from_obj, 3);  // количество зарядов
		int t2 = GET_OBJ_VAL(to_obj, 3);
		int delta = (GET_OBJ_VAL(to_obj, 2) - GET_OBJ_VAL(to_obj, 3));
		if (delta >= t1) //объем колчана больше пучка
		{
			to_obj->add_val(2, t1);
			SendMsgToChar("Вы аккуратно сложили стрелы в колчан.\r\n", ch);
			ExtractObjFromWorld(from_obj);
			return;
		} else {
			to_obj->add_val(2, (t2 - GET_OBJ_VAL(to_obj, 2)));
			SendMsgToChar("Вы аккуратно переложили несколько стрел в колчан.\r\n", ch);
			from_obj->add_val(2, (GET_OBJ_VAL(to_obj, 2) - t2));
			return;
		}
	}

	SendMsgToChar("С таким успехом надо пополнять соседние камни, для разговоров по ним.\r\n", ch);
}

int can_take_obj(CharData *ch, ObjData *obj) {
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)
		&& GET_OBJ_TYPE(obj) != EObjType::kMoney) {
		act("$p: Вы не могете нести столько вещей.", false, ch, obj, nullptr, kToChar);
		return (0);
	} else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)
		&& GET_OBJ_TYPE(obj) != EObjType::kMoney) {
		act("$p: Вы не в состоянии нести еще и $S.", false, ch, obj, nullptr, kToChar);
		return (0);
	} else if (!(CAN_WEAR(obj, EWearFlag::kTake))) {
		act("$p: Вы не можете взять $S.", false, ch, obj, nullptr, kToChar);
		return (0);
	} else if (invalid_anti_class(ch, obj)) {
		act("$p: Эта вещь не предназначена для вас!", false, ch, obj, nullptr, kToChar);
		return (0);
	} else if (NamedStuff::check_named(ch, obj, false)) {
		if (!NamedStuff::wear_msg(ch, obj))
			act("$p: Эта вещь не предназначена для вас!", false, ch, obj, nullptr, kToChar);
		return (0);
	}
	return (1);
}

/// считаем сколько у ch в группе еще игроков (не мобов)
int other_pc_in_group(CharData *ch) {
	int num = 0;
	CharData *k = ch->has_master() ? ch->get_master() : ch;
	for (Follower *f = k->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->ch, EAffect::kGroup)
			&& !f->ch->IsNpc()
			&& IN_ROOM(f->ch) == ch->in_room) {
			++num;
		}
	}
	return num;
}

void split_or_clan_tax(CharData *ch, long amount) {
	if (AFF_FLAGGED(ch, EAffect::kGroup) && (other_pc_in_group(ch) > 0) &&
		PRF_FLAGGED(ch, EPrf::kAutosplit)) {
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

	if (GET_OBJ_TYPE(obj) != EObjType::kMoney
		|| value <= 0) {
		return;
	}

	if (curr_type == currency::ICE) {
		sprintf(buf, "Это составило %d %s.\r\n", value, GetDeclensionInNumber(value, EWhat::kIceU));
		SendMsgToChar(buf, ch);
		ch->add_ice_currency(value);
		//Делить лед ВСЕГДА!
		if (AFF_FLAGGED(ch, EAffect::kGroup) && other_pc_in_group(ch) > 0) {
			char local_buf[256];
			sprintf(local_buf, "%d", value);
			do_split(ch, local_buf, 0, 0, curr_type);
		}
		ExtractObjFromWorld(obj);
		return;
	}

// Все что неизвестно - куны (для совместимости)
/*	if (curr_type != currency::GOLD) {
		//Вот тут неопознанная валюта
		return;
	}
*/
	sprintf(buf, "Это составило %d %s.\r\n", value, GetDeclensionInNumber(value, EWhat::kMoneyU));
	SendMsgToChar(buf, ch);

	// все, что делится на группу - идет через налог (из кошельков не делится)
	if (AFF_FLAGGED(ch, EAffect::kGroup) && other_pc_in_group(ch) > 0 &&
		PRF_FLAGGED(ch, EPrf::kAutosplit) && (!cont || !system_obj::is_purse(cont))) {
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

	ExtractObjFromChar(obj);
	ExtractObjFromWorld(obj);
}

// return 0 - чтобы словить невозможность взять из клан-сундука,
// иначе при вз все сун будет спам на каждый предмет, мол низя
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode) {
	if (!bloody::handle_transfer(nullptr, ch, obj))
		return false;
	if ((mode == EFind::kObjInventory || mode == EFind::kObjRoom || mode == EFind::kObjEquip) && can_take_obj(ch, obj)
		&& get_otrigger(obj, ch)) {
		// если берем из клан-сундука
		if (Clan::is_clan_chest(cont)) {
			if (!Clan::TakeChest(ch, obj, cont)) {
				return false;
			}
			return true;
		}
		// клан-хранилище ингров
		if (ClanSystem::is_ingr_chest(cont)) {
			if (!Clan::take_ingr_chest(ch, obj, cont)) {
				return false;
			}
			return true;
		}
		ExtractObjFromObj(obj);
		PlaceObjToInventory(obj, ch);
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
	return true;
}

// *\param autoloot - true только при взятии шмоток из трупа в режиме автограбежа
void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot) {
	if (Depot::is_depot(cont)) {
		Depot::take_depot(ch, local_arg, amount);
		return;
	}

	ObjData *obj, *next_obj;
	int obj_dotmode, found = 0;

	obj_dotmode = find_all_dots(local_arg);
	if (OBJVAL_FLAGGED(cont, EContainerFlag::kShutted))
		act("$o закрыт$A.", false, ch, cont, nullptr, kToChar);
	else if (obj_dotmode == kFindIndiv) {
		if (!(obj = get_obj_in_list_vis(ch, local_arg, cont->get_contains()))) {
			sprintf(buf, "Вы не видите '%s' в $o5.", local_arg);
			act(buf, false, ch, cont, nullptr, kToChar);
		} else {
			ObjData *obj_next;
			while (obj && amount--) {
				obj_next = obj->get_next_content();
				if (!perform_get_from_container(ch, obj, cont, mode))
					return;
				obj = get_obj_in_list_vis(ch, local_arg, obj_next);
			}
		}
	} else {
		if (obj_dotmode == kFindAlldot && !*local_arg) {
			SendMsgToChar("Взять что \"все\"?\r\n", ch);
			return;
		}
		for (obj = cont->get_contains(); obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (obj_dotmode == kFindAll
					|| isname(local_arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(local_arg, obj, ch))) {
				if (autoloot
					&& (GET_OBJ_TYPE(obj) == EObjType::kIngredient
						|| GET_OBJ_TYPE(obj) == EObjType::kMagicIngredient)
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
			if (obj_dotmode == kFindAll)
				act("$o пуст$A.", false, ch, cont, nullptr, kToChar);
			else {
				sprintf(buf, "Вы не видите ничего похожего на '%s' в $o5.", local_arg);
				act(buf, false, ch, cont, nullptr, kToChar);
			}
		}
	}
}

int perform_get_from_room(CharData *ch, ObjData *obj) {
	if (can_take_obj(ch, obj) && get_otrigger(obj, ch) && bloody::handle_transfer(nullptr, ch, obj)) {
		ExtractObjFromRoom(obj);
		PlaceObjToInventory(obj, ch);
		if (obj->get_carried_by() == ch) {
			if (bloody::is_bloody(obj)) {
				act("Вы подняли $o3, испачкав свои руки кровью!",
					false, ch, obj, nullptr, kToChar);
				act("$n поднял$g $o3, испачкав руки кровью.",
					true, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы подняли $o3.", false, ch, obj, nullptr, kToChar);
				act("$n поднял$g $o3.", true, ch, obj, nullptr, kToRoom | kToArenaListen);
			}
			get_check_money(ch, obj, nullptr);
			return (1);
		}
	}
	return (0);
}

void get_from_room(CharData *ch, char *local_arg, int howmany) {
	ObjData *obj, *next_obj;
	int dotmode, found = 0;

	// Are they trying to take something in a room extra description?
	if (find_exdesc(local_arg, world[ch->in_room]->ex_description) != nullptr) {
		SendMsgToChar("Вы не можете это взять.\r\n", ch);
		return;
	}

	dotmode = find_all_dots(local_arg);

	if (dotmode == kFindIndiv) {
		if (!(obj = get_obj_in_list_vis(ch, local_arg, world[ch->in_room]->contents))) {
			sprintf(buf, "Вы не видите здесь '%s'.\r\n", local_arg);
			SendMsgToChar(buf, ch);
		} else {
			ObjData *obj_next;
			while (obj && howmany--) {
				obj_next = obj->get_next_content();
				perform_get_from_room(ch, obj);
				obj = get_obj_in_list_vis(ch, local_arg, obj_next);
			}
		}
	} else {
		if (dotmode == kFindAlldot && !*local_arg) {
			SendMsgToChar("Взять что \"все\"?\r\n", ch);
			return;
		}
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (dotmode == kFindAll
					|| isname(local_arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(local_arg, obj, ch))) {
				found = 1;
				perform_get_from_room(ch, obj);
			}
		}
		if (!found) {
			if (dotmode == kFindAll) {
				SendMsgToChar("Похоже, здесь ничего нет.\r\n", ch);
			} else {
				sprintf(buf, "Вы не нашли здесь '%s'.\r\n", local_arg);
				SendMsgToChar(buf, ch);
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
		SendMsgToChar("Что вы хотите маркировать?\r\n", ch);
	} else if (!*arg2 || !is_number(arg2)) {
		SendMsgToChar("Не указан или неверный маркер.\r\n", ch);
	} else {
		cont_dotmode = find_all_dots(arg1);
		if (cont_dotmode == kFindIndiv) {
			generic_find(arg1, EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip, ch, &tmp_char, &cont);
			if (!cont) {
				sprintf(buf, "У вас нет '%s'.\r\n", arg1);
				SendMsgToChar(buf, ch);
				return;
			}
			cont->set_owner(atoi(arg2));
			act("Вы пометили $o3.", false, ch, cont, nullptr, kToChar);
		} else {
			if (cont_dotmode == kFindAlldot && !*arg1) {
				SendMsgToChar("Пометить что \"все\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont; cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(arg1, cont->get_aliases()))) {
					cont->set_owner(atoi(arg2));
					act("Вы пометили $o3.", false, ch, cont, nullptr, kToChar);
					found = true;
				}
			}
			for (cont = world[ch->in_room]->contents; cont; cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(arg2, cont->get_aliases()))) {
					cont->set_owner(atoi(arg2));
					act("Вы пометили $o3.", false, ch, cont, nullptr, kToChar);
					found = true;
				}
			}
			if (!found) {
				if (cont_dotmode == kFindAll) {
					SendMsgToChar("Вы не смогли найти ничего для маркировки.\r\n", ch);
				} else {
					sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", arg1);
					SendMsgToChar(buf, ch);
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
	int where_bits = EFind::kObjInventory | EFind::kObjEquip | EFind::kObjRoom;

	int cont_dotmode, found = 0, mode, amount = 1;
	ObjData *cont;
	CharData *tmp_char;

	argument = two_arguments(argument, arg1, arg2);
	argument = two_arguments(argument, arg3, arg4);

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		SendMsgToChar("У вас заняты руки!\r\n", ch);
	else if (!*arg1)
		SendMsgToChar("Что вы хотите взять?\r\n", ch);
	else if (!*arg2 || isname(arg2, "земля комната ground room"))
		get_from_room(ch, arg1, 1);
	else if (is_number(arg1) && (!*arg3 || isname(arg3, "земля комната ground room")))
		get_from_room(ch, arg2, atoi(arg1));
	else if ((!*arg3 && isname(arg2, "инвентарь экипировка inventory equipment")) ||
		(is_number(arg1) && !*arg4 && isname(arg3, "инвентарь экипировка inventory equipment")))
		SendMsgToChar("Вы уже подобрали этот предмет!\r\n", ch);
	else {
		if (is_number(arg1)) {
			amount = std::stoi(arg1);
			theobj = arg2;
			thecont = arg3;
			theplace = arg4;
		} else {
			theobj = arg1;
			thecont = arg2;
			theplace = arg3;
		}

		if (isname(theplace, "земля комната room ground"))
			where_bits = EFind::kObjRoom;
		else if (isname(theplace, "инвентарь inventory"))
			where_bits = EFind::kObjInventory;
		else if (isname(theplace, "экипировка equipment"))
			where_bits = EFind::kObjEquip;

		cont_dotmode = find_all_dots(thecont);
		if (cont_dotmode == kFindIndiv) {
			mode = generic_find(thecont, where_bits, ch, &tmp_char, &cont);
			if (!cont) {
				sprintf(buf, "Вы не видите '%s'.\r\n", arg2);
				SendMsgToChar(buf, ch);
			} else if (GET_OBJ_TYPE(cont) != EObjType::kContainer) {
				act("$o - не контейнер.", false, ch, cont, nullptr, kToChar);
			} else {
				get_from_container(ch, cont, theobj, mode, amount, false);
			}
		} else {
			if (cont_dotmode == kFindAlldot
				&& !*thecont) {
				SendMsgToChar("Взять из чего \"всего\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont && IS_SET(where_bits, EFind::kObjInventory); cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch))) {
					if (GET_OBJ_TYPE(cont) == EObjType::kContainer) {
						found = 1;
						get_from_container(ch, cont, theobj, EFind::kObjInventory, amount, false);
					} else if (cont_dotmode == kFindAlldot) {
						found = 1;
						act("$o - не контейнер.", false, ch, cont, nullptr, kToChar);
					}
				}
			}
			for (cont = world[ch->in_room]->contents; cont && IS_SET(where_bits, EFind::kObjRoom);
				 cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch))) {
					if (GET_OBJ_TYPE(cont) == EObjType::kContainer) {
						get_from_container(ch, cont, theobj, EFind::kObjRoom, amount, false);
						found = 1;
					} else if (cont_dotmode == kFindAlldot) {
						act("$o - не контейнер.", false, ch, cont, nullptr, kToChar);
						found = 1;
					}
				}
			}
			if (!found) {
				if (cont_dotmode == kFindAll) {
					SendMsgToChar("Вы не смогли найти ни одного контейнера.\r\n", ch);
				} else {
					sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", thecont);
					SendMsgToChar(buf, ch);
				}
			}
		}
	}
}

void weight_change_object(ObjData *obj, int weight) {
	ObjData *tmp_obj;
	CharData *tmp_ch;

	if (obj->get_in_room() != kNowhere) {
		obj->set_weight(std::max(1, GET_OBJ_WEIGHT(obj) + weight));
	} else if ((tmp_ch = obj->get_carried_by())) {
		ExtractObjFromChar(obj);
		obj->set_weight(std::max(1, GET_OBJ_WEIGHT(obj) + weight));
		PlaceObjToInventory(obj, tmp_ch);
	} else if ((tmp_obj = obj->get_in_obj())) {
		ExtractObjFromObj(obj);
		obj->set_weight(std::max(1, GET_OBJ_WEIGHT(obj) + weight));
		PlaceObjIntoObj(obj, tmp_obj);
	} else {
		log("SYSERR: Unknown attempt to subtract weight from an object.");
	}
}

void do_fry(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	ObjData *meet;
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Что вы собрались поджарить?\r\n", ch);
		return;
	}
	if (ch->GetEnemy()) {
		SendMsgToChar("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}
	if (!(meet = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxStringLength, "У вас нет '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}
	if (!world[ch->in_room]->fires) {
		SendMsgToChar(ch, "На чем вы собрались жарить, огня то нет!\r\n");
		return;
	}
	if (world[ch->in_room]->fires > 2) {
		SendMsgToChar(ch, "Костер слишком силен, сгорит!\r\n");
		return;
	}

	const auto meet_vnum = GET_OBJ_VNUM(meet);
	if (!meat_mapping.has(meet_vnum)) // не нашлось в массиве
	{
		SendMsgToChar(ch, "%s не подходит для жарки.\r\n", GET_OBJ_PNAME(meet, 0).c_str());
		return;
	}

	act("Вы нанизали на веточку и поджарили $o3.", false, ch, meet, nullptr, kToChar);
	act("$n нанизал$g на веточку и поджарил$g $o3.",
		true, ch, meet, nullptr, kToRoom | kToArenaListen);
	const auto tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get(meet_vnum));
	if (tobj) {
		can_carry_obj(ch, tobj.get());
		ExtractObjFromWorld(meet);
		SetWaitState(ch, 1 * kPulseViolence);
	} else {
		mudlog("Невозможно загрузить жаренное мясо в act.item.cpp::do_fry!", NRM, kLvlGreatGod, ERRLOG, true);
	}
}

void do_eat(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *food;
	int amount;

	one_argument(argument, arg);

	if (subcmd == SCMD_DEVOUR) {
		if (MOB_FLAGGED(ch, EMobFlag::kResurrected)
			&& IsAbleToUseFeat(ch->get_master(), EFeat::kZombieDrover)) {
			feed_charmice(ch, arg);
			return;
		}
	}
	if (!ch->IsNpc()
		&& subcmd == SCMD_DEVOUR) {
		SendMsgToChar("Вы же не зверь какой, пожирать трупы!\r\n", ch);
		return;
	}

	if (ch->IsNpc())        // Cannot use GET_COND() on mobs.
		return;

	if (!*arg) {
		SendMsgToChar("Чем вы собрались закусить?\r\n", ch);
		return;
	}
	if (ch->GetEnemy()) {
		SendMsgToChar("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}

	if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}
	if (subcmd == SCMD_TASTE
		&& ((GET_OBJ_TYPE(food) == EObjType::kLiquidContainer)
			|| (GET_OBJ_TYPE(food) == EObjType::kFountain))) {
		do_drink(ch, argument, 0, SCMD_SIP);
		return;
	}

	if (!IS_GOD(ch)) {
		if (GET_OBJ_TYPE(food) == EObjType::kMagicIngredient) //Сообщение на случай попытки проглотить ингры
		{
			SendMsgToChar("Не можешь приготовить - покупай готовое!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(food) != EObjType::kFood
			&& GET_OBJ_TYPE(food) != EObjType::kNote) {
			SendMsgToChar("Это несъедобно!\r\n", ch);
			return;
		}
	}
	if (GET_COND(ch, FULL) == 0
		&& GET_OBJ_TYPE(food) != EObjType::kNote)    // Stomach full
	{
		SendMsgToChar("Вы слишком сыты для этого!\r\n", ch);
		return;
	}
	if (subcmd == SCMD_EAT
		|| (subcmd == SCMD_TASTE
			&& GET_OBJ_TYPE(food) == EObjType::kNote)) {
		act("Вы съели $o3.", false, ch, food, nullptr, kToChar);
		act("$n съел$g $o3.", true, ch, food, nullptr, kToRoom | kToArenaListen);
	} else {
		act("Вы откусили маленький кусочек от $o1.", false, ch, food, nullptr, kToChar);
		act("$n попробовал$g $o3 на вкус.",
			true, ch, food, nullptr, kToRoom | kToArenaListen);
	}

	amount = ((subcmd == SCMD_EAT && GET_OBJ_TYPE(food) != EObjType::kNote)
			  ? GET_OBJ_VAL(food, 0)
			  : 1);

	gain_condition(ch, FULL, -2 * amount);

	if (GET_COND(ch, FULL) == 0) {
		SendMsgToChar("Вы наелись.\r\n", ch);
	}

	for (int i = 0; i < kMaxObjAffect; i++) {
		if (food->get_affected(i).modifier) {
			Affect<EApply> af;
			af.location = food->get_affected(i).location;
			af.modifier = food->get_affected(i).modifier;
			af.bitvector = 0;
			af.type = kSpellFullFeed;
//			af.battleflag = 0;
			af.duration = CalcDuration(ch, 10 * 2, 0, 0, 0, 0);
			ImposeAffect(ch, af);
		}

	}

	if ((GET_OBJ_VAL(food, 3) == 1) && !IS_IMMORTAL(ch))    // The shit was poisoned !
	{
		SendMsgToChar("Однако, какой странный вкус!\r\n", ch);
		act("$n закашлял$u и начал$g отплевываться.",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

		Affect<EApply> af;
		af.type = kSpellPoison;
		af.duration = CalcDuration(ch, amount == 1 ? amount : amount * 2, 0, 0, 0, 0);
		af.modifier = 0;
		af.location = EApply::kStr;
		af.bitvector = to_underlying(EAffect::kPoisoned);
		af.battleflag = kAfSameTime;
		ImposeAffect(ch, af, false, false, false, false);
		af.type = kSpellPoison;
		af.duration = CalcDuration(ch, amount == 1 ? amount : amount * 2, 0, 0, 0, 0);
		af.modifier = amount * 3;
		af.location = EApply::kPoison;
		af.bitvector = to_underlying(EAffect::kPoisoned);
		af.battleflag = kAfSameTime;
		ImposeAffect(ch, af, false, false, false, false);
		ch->poisoner = 0;
	}
	if (subcmd == SCMD_EAT
		|| (subcmd == SCMD_TASTE
			&& GET_OBJ_TYPE(food) == EObjType::kNote)) {
		ExtractObjFromWorld(food);
	} else {
		food->set_val(0, food->get_val(0) - 1);
		if (!food->get_val(0)) {
			SendMsgToChar("Вы доели все!\r\n", ch);
			ExtractObjFromWorld(food);
		}
	}
}

void perform_wear(CharData *ch, ObjData *obj, int equip_pos) {
	/*
	   * kTake is used for objects that do not require special bits
	   * to be put into that position (e.g. you can hold any object, not just
	   * an object with a HOLD bit.)
	   */

	const EWearFlag wear_bitvectors[] = {
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
	if (!CAN_WEAR(obj, wear_bitvectors[equip_pos])) {
		act("Вы не можете надеть $o3 на эту часть тела.", false, ch, obj, nullptr, kToChar);
		return;
	}
	if (unique_stuff(ch, obj) && obj->has_flag(EObjFlag::kUnique)) {
		SendMsgToChar("Вы не можете использовать более одной такой вещи.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		if (ch->GetEnemy() && (equip_pos == EEquipPos::kShield || GET_OBJ_TYPE(obj) == EObjType::kWeapon)) {
			SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
			return;
		}
	};

	// for neck, finger, and wrist, try pos 2 if pos 1 is already full
	if (   // не может держать если есть свет или двуручник
		(equip_pos == EEquipPos::kHold && (GET_EQ(ch, EEquipPos::kBoths) || GET_EQ(ch, EEquipPos::kLight)
			|| GET_EQ(ch, EEquipPos::kShield))) ||
			// не может вооружиться если есть двуручник
			(equip_pos == EEquipPos::kWield && GET_EQ(ch, EEquipPos::kBoths)) ||
			// не может держать щит если что-то держит или двуручник
			(equip_pos == EEquipPos::kShield && (GET_EQ(ch, EEquipPos::kHold) || GET_EQ(ch, EEquipPos::kBoths))) ||
			// не может двуручник если есть щит, свет, вооружен или держит
			(equip_pos == EEquipPos::kBoths && (GET_EQ(ch, EEquipPos::kHold) || GET_EQ(ch, EEquipPos::kLight)
				|| GET_EQ(ch, EEquipPos::kShield) || GET_EQ(ch, EEquipPos::kWield))) ||
			// не может держать свет если двуручник или держит
			(equip_pos == EEquipPos::kLight && (GET_EQ(ch, EEquipPos::kHold) || GET_EQ(ch, EEquipPos::kBoths)))) {
		SendMsgToChar("У вас заняты руки.\r\n", ch);
		return;
	}
	if (   // не может одеть колчан если одет не лук
		(equip_pos == EEquipPos::kQuiver &&
			!(GET_EQ(ch, EEquipPos::kBoths) &&
				(((GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kBoths))) == EObjType::kWeapon)
					&& (static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, EEquipPos::kBoths)) == ESkill::kBows))))) {
		SendMsgToChar("А стрелять чем будете?\r\n", ch);
		return;
	}
	// нельзя надеть щит, если недостаточно силы
	if (!IS_IMMORTAL(ch) && (equip_pos == EEquipPos::kShield) && !OK_SHIELD(ch, obj)) {
	}

	if ((equip_pos == EEquipPos::kFingerR) || (equip_pos == EEquipPos::kNeck) || (equip_pos == EEquipPos::kWristR))
		if (GET_EQ(ch, equip_pos))
			++equip_pos;

	if (GET_EQ(ch, equip_pos)) {
		SendMsgToChar(already_wearing[equip_pos], ch);
		return;
	}
	if (!wear_otrigger(obj, ch, equip_pos))
		return;

	//obj_from_char(obj);
	EquipObj(ch, obj, equip_pos, CharEquipFlag::show_msg);
}

int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg) {
	int equip_pos = -1;

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

	if (!local_arg || !*local_arg) {
		int tmp_where = -1;
		if (CAN_WEAR(obj, EWearFlag::kFinger)) {
			if (!GET_EQ(ch, EEquipPos::kFingerR)) {
				equip_pos = EEquipPos::kFingerR;
			} else if (!GET_EQ(ch, EEquipPos::kFingerL)) {
				equip_pos = EEquipPos::kFingerL;
			} else {
				tmp_where = EEquipPos::kFingerR;
			}
		}
		if (CAN_WEAR(obj, EWearFlag::kNeck)) {
			if (!GET_EQ(ch, EEquipPos::kNeck)) {
				equip_pos = EEquipPos::kNeck;
			} else if (!GET_EQ(ch, EEquipPos::kChest)) {
				equip_pos = EEquipPos::kChest;
			} else {
				tmp_where = EEquipPos::kNeck;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kBody)) {
			if (!GET_EQ(ch, EEquipPos::kBody)) {
				equip_pos = EEquipPos::kBody;
			} else {
				tmp_where = EEquipPos::kBody;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kHead)) {
			if (!GET_EQ(ch, EEquipPos::kHead)) {
				equip_pos = EEquipPos::kHead;
			} else {
				tmp_where = EEquipPos::kHead;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kLegs)) {
			if (!GET_EQ(ch, EEquipPos::kLegs)) {
				equip_pos = EEquipPos::kLegs;
			} else {
				tmp_where = EEquipPos::kLegs;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kFeet)) {
			if (!GET_EQ(ch, EEquipPos::kFeet)) {
				equip_pos = EEquipPos::kFeet;
			} else {
				tmp_where = EEquipPos::kFeet;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kHands)) {
			if (!GET_EQ(ch, EEquipPos::kHands)) {
				equip_pos = EEquipPos::kHands;
			} else {
				tmp_where = EEquipPos::kHands;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kArms)) {
			if (!GET_EQ(ch, EEquipPos::kArms)) {
				equip_pos = EEquipPos::kArms;
			} else {
				tmp_where = EEquipPos::kArms;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kShield)) {
			if (!GET_EQ(ch, EEquipPos::kShield)) {
				equip_pos = EEquipPos::kShield;
			} else {
				tmp_where = EEquipPos::kShield;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
			if (!GET_EQ(ch, EEquipPos::kShoulders)) {
				equip_pos = EEquipPos::kShoulders;
			} else {
				tmp_where = EEquipPos::kShoulders;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kWaist)) {
			if (!GET_EQ(ch, EEquipPos::kWaist)) {
				equip_pos = EEquipPos::kWaist;
			} else {
				tmp_where = EEquipPos::kWaist;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
			if (!GET_EQ(ch, EEquipPos::kQuiver)) {
				equip_pos = EEquipPos::kQuiver;
			} else {
				tmp_where = EEquipPos::kQuiver;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::kWrist)) {
			if (!GET_EQ(ch, EEquipPos::kWristR)) {
				equip_pos = EEquipPos::kWristR;
			} else if (!GET_EQ(ch, EEquipPos::kWristL)) {
				equip_pos = EEquipPos::kWristL;
			} else {
				tmp_where = EEquipPos::kWristR;
			}
		}

		if (equip_pos == -1) {
			equip_pos = tmp_where;
		}
	} else {
		equip_pos = search_block(local_arg, keywords, false);
		if (equip_pos < 0
			|| *local_arg == '!') {
			sprintf(buf, "'%s'? Странная анатомия у этих русских!\r\n", local_arg);
			SendMsgToChar(buf, ch);
			return -1;
		}
	}

	return equip_pos;
}

void do_wear(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *obj, *next_obj;
	int equip_pos, dotmode, items_worn = 0;

	two_arguments(argument, arg1, arg2);

	if (ch->IsNpc()
		&& AFF_FLAGGED(ch, EAffect::kCharmed)
		&& (!NPC_FLAGGED(ch, ENpcFlag::kArmoring)
			|| MOB_FLAGGED(ch, EMobFlag::kResurrected))) {
		return;
	}

	if (!*arg1) {
		SendMsgToChar("Что вы собрались надеть?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg1);

	if (*arg2 && (dotmode != kFindIndiv)) {
		SendMsgToChar("И на какую часть тела вы желаете это надеть?!\r\n", ch);
		return;
	}
	if (dotmode == kFindAll) {
		for (obj = ch->carrying; obj && !AFF_FLAGGED(ch, EAffect::kHold) &&
			GET_POS(ch) > EPosition::kSleep; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (equip_pos = find_eq_pos(ch, obj, nullptr)) >= 0) {
				items_worn++;
				perform_wear(ch, obj, equip_pos);
			}
		}
		if (!items_worn) {
			SendMsgToChar("Увы, но надеть вам нечего.\r\n", ch);
		}
	} else if (dotmode == kFindAlldot) {
		if (!*arg1) {
			SendMsgToChar("Надеть \"все\" чего?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			sprintf(buf, "У вас нет ничего похожего на '%s'.\r\n", arg1);
			SendMsgToChar(buf, ch);
		} else
			while (obj && !AFF_FLAGGED(ch, EAffect::kHold) && GET_POS(ch) > EPosition::kSleep) {
				next_obj = get_obj_in_list_vis(ch, arg1, obj->get_next_content());
				if ((equip_pos = find_eq_pos(ch, obj, nullptr)) >= 0) {
					perform_wear(ch, obj, equip_pos);
				} else {
					act("Вы не можете надеть $o3.", false, ch, obj, nullptr, kToChar);
				}
				obj = next_obj;
			}
	} else {
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			sprintf(buf, "У вас нет ничего похожего на '%s'.\r\n", arg1);
			SendMsgToChar(buf, ch);
		} else {
			if ((equip_pos = find_eq_pos(ch, obj, arg2)) >= 0)
				perform_wear(ch, obj, equip_pos);
			else if (!*arg2)
				act("Вы не можете надеть $o3.", false, ch, obj, nullptr, kToChar);
		}
	}
}

void do_wield(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int wear;

	if (ch->IsNpc() && (AFF_FLAGGED(ch, EAffect::kCharmed)
		&& (!NPC_FLAGGED(ch, ENpcFlag::kWielding) || MOB_FLAGGED(ch, EMobFlag::kResurrected))))
		return;

	if (ch->is_morphed()) {
		SendMsgToChar("Лапами неудобно держать оружие.\r\n", ch);
		return;
	}
	argument = one_argument(argument, arg);

	if (!*arg)
		SendMsgToChar("Вооружиться чем?\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "Вы не видите ничего похожего на \'%s\'.\r\n", arg);
		SendMsgToChar(buf, ch);
	} else {
		if (!CAN_WEAR(obj, EWearFlag::kWield)
			&& !CAN_WEAR(obj, EWearFlag::kBoth)) {
			SendMsgToChar("Вы не можете вооружиться этим.\r\n", ch);
		} else if (GET_OBJ_TYPE(obj) != EObjType::kWeapon) {
			SendMsgToChar("Это не оружие.\r\n", ch);
		} else if (ch->IsNpc()
			&& AFF_FLAGGED(ch, EAffect::kCharmed)
			&& MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
			SendMsgToChar("Ожившие трупы не могут вооружаться.\r\n", ch);
		} else {
			one_argument(argument, arg);
			if (!str_cmp(arg, "обе")
				&& CAN_WEAR(obj, EWearFlag::kBoth)) {
				// иногда бывает надо
				if (!IS_IMMORTAL(ch) && !OK_BOTH(ch, obj)) {
					act("Вам слишком тяжело держать $o3 двумя руками.",
						false, ch, obj, nullptr, kToChar);
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
				act("Вам слишком тяжело держать $o3 в правой руке.",
					false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_WIELD_W);

				if (CAN_WEAR(obj, EWearFlag::kBoth)) {
					wear = EEquipPos::kBoths;
				} else {
					return;
				}
			}

			if (wear == EEquipPos::kBoths && !IS_IMMORTAL(ch) && !OK_BOTH(ch, obj)) {
				act("Вам слишком тяжело держать $o3 двумя руками.",
					false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_BOTH_W);
				return;
			};
			perform_wear(ch, obj, wear);
		}
	}
}

void do_grab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	auto equip_pos{EEquipPos::kHold};
	ObjData *obj;
	one_argument(argument, arg);

	if (ch->IsNpc() && !NPC_FLAGGED(ch, ENpcFlag::kWielding))
		return;

	if (ch->is_morphed()) {
		SendMsgToChar("Лапами неудобно это держать.\r\n", ch);
		return;
	}

	if (!*arg)
		SendMsgToChar("Вы заорали : 'Держи его!!! Хватай его!!!'\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
	} else {
		if (GET_OBJ_TYPE(obj) == EObjType::kLightSource) {
			perform_wear(ch, obj, EEquipPos::kLight);
		} else {
			if (!CAN_WEAR(obj, EWearFlag::kHold)
				&& GET_OBJ_TYPE(obj) != EObjType::kWand
				&& GET_OBJ_TYPE(obj) != EObjType::kStaff
				&& GET_OBJ_TYPE(obj) != EObjType::kScroll
				&& GET_OBJ_TYPE(obj) != EObjType::kPotion) {
				SendMsgToChar("Вы не можете это держать.\r\n", ch);
				return;
			}

			if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
				if (static_cast<ESkill>GET_OBJ_SKILL(obj) == ESkill::kTwohands
					|| static_cast<ESkill>GET_OBJ_SKILL(obj) == ESkill::kBows) {
					SendMsgToChar("Данный тип оружия держать невозможно.", ch);
					return;
				}
			}

			if (ch->IsNpc()
				&& AFF_FLAGGED(ch, EAffect::kCharmed)
				&& MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
				SendMsgToChar("Ожившие трупы не могут вооружаться.\r\n", ch);
				return;
			}
			if (!IS_IMMORTAL(ch)
				&& !OK_HELD(ch, obj)) {
				act("Вам слишком тяжело держать $o3 в левой руке.",
					false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_HOLD_W);

				if (CAN_WEAR(obj, EWearFlag::kBoth)) {
					if (!OK_BOTH(ch, obj)) {
						act("Вам слишком тяжело держать $o3 двумя руками.",
							false, ch, obj, nullptr, kToChar);
						message_str_need(ch, obj, STR_BOTH_W);
						return;
					} else {
						equip_pos = EEquipPos::kBoths;
					}
				} else {
					return;
				}
			}
			perform_wear(ch, obj, equip_pos);
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
			act("$p: Вы не можете нести столько вещей!", false, ch, obj, nullptr, kToChar);
		} else {
			if (!remove_otrigger(obj, ch)) {
				return;
			}
			if (ch->GetEnemy() && (GET_OBJ_TYPE(obj) == EObjType::kWeapon || pos == EEquipPos::kShield)) {
				ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
			}
			act("Вы прекратили использовать $o3.", false, ch, obj, nullptr, kToChar);
			act("$n прекратил$g использовать $o3.",
				true, ch, obj, nullptr, kToRoom | kToArenaListen);
			PlaceObjToInventory(UnequipChar(ch, pos, CharEquipFlag::show_msg), ch);
		}
	}
}

void do_remove(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int i, dotmode, found;
	ObjData *obj;

	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar("Снять что?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg);

	if (dotmode == kFindAll) {
		found = 0;
		for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (GET_EQ(ch, i)) {
				RemoveEquipment(ch, i);
				found = 1;
			}
		}
		if (!found) {
			SendMsgToChar("На вас не надето предметов этого типа.\r\n", ch);
			return;
		}
	} else if (dotmode == kFindAlldot) {
		if (!*arg) {
			SendMsgToChar("Снять все вещи какого типа?\r\n", ch);
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
				SendMsgToChar(buf, ch);
				return;
			}
		}
	} else        // Returns object pointer but we don't need it, just true/false.
	{
		if (!get_object_in_equip_vis(ch, arg, ch->equipment, &i)) {
			// если предмет не найден, то возможно игрок ввел "левая" или "правая"
			if (!str_cmp("правая", arg)) {
				if (!GET_EQ(ch, EEquipPos::kWield)) {
					SendMsgToChar("В правой руке ничего нет.\r\n", ch);
				} else {
					RemoveEquipment(ch, EEquipPos::kWield);
				}
			} else if (!str_cmp("левая", arg)) {
				if (!GET_EQ(ch, EEquipPos::kHold))
					SendMsgToChar("В левой руке ничего нет.\r\n", ch);
				else
					RemoveEquipment(ch, EEquipPos::kHold);
			} else {
				snprintf(buf, kMaxInputLength, "Вы не используете '%s'.\r\n", arg);
				SendMsgToChar(buf, ch);
				return;
			}
		} else {
			RemoveEquipment(ch, i);
		}
	}
	//мы что-то да снимали. значит проверю я доп слот
	if ((obj = GET_EQ(ch, EEquipPos::kQuiver)) && !GET_EQ(ch, EEquipPos::kBoths)) {
		SendMsgToChar("Нету лука, нет и стрел.\r\n", ch);
		act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
		PlaceObjToInventory(UnequipChar(ch, EEquipPos::kQuiver, CharEquipFlags()), ch);
		return;
	}
}

void DoSharpening(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int weight, add_hr, add_dr, prob, percent, min_mod, max_mod, i;
	bool oldstate;
	if (!ch->get_skill(ESkill::kSharpening)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar("Что вы хотите заточить?\r\n", ch);
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	};

	if (GET_OBJ_TYPE(obj) != EObjType::kWeapon) {
		SendMsgToChar("Вы можете заточить только оружие.\r\n", ch);
		return;
	}

	if (static_cast<ESkill>GET_OBJ_SKILL(obj) == ESkill::kBows) {
		SendMsgToChar("Невозможно заточить этот тип оружия.\r\n", ch);
		return;
	}

	if (obj->has_flag(EObjFlag::kMagic)) {
		SendMsgToChar("Вы не можете заточить заколдованный предмет.\r\n", ch);
		return;
	}

	// Make sure no other (than hitroll & damroll) affections.
	for (i = 0; i < kMaxObjAffect; i++) {
		if ((obj->get_affected(i).location != EApply::kNone)
			&& (obj->get_affected(i).location != EApply::kHitroll)
			&& (obj->get_affected(i).location != EApply::kDamroll)) {
			SendMsgToChar("Этот предмет не может быть заточен.\r\n", ch);
			return;
		}
	}

	switch (obj->get_material()) {
		case EObjMaterial::kBronze:
		case EObjMaterial::kBulat:
		case EObjMaterial::kIron:
		case EObjMaterial::kSteel:
		case EObjMaterial::kForgedSteel:
		case EObjMaterial::kPreciousMetel:
		case EObjMaterial::kBone: act("Вы взялись точить $o3.",
									  false, ch, obj, nullptr, kToChar);
			act("$n взял$u точить $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			weight = -1;
			break;

		case EObjMaterial::kWood:
		case EObjMaterial::kHardWood: act("Вы взялись стругать $o3.",
										  false, ch, obj, nullptr, kToChar);
			act("$n взял$u стругать $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			weight = -1;
			break;

		case EObjMaterial::kSkin: act("Вы взялись проклепывать $o3.",
									  false, ch, obj, nullptr, kToChar);
			act("$n взял$u проклепывать $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			weight = +1;
			break;

		default: sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n", OBJN(obj, ch, 0));
			SendMsgToChar(buf, ch);
			return;
	}
	bool change_weight = true;
	//Заточить повторно можно, но это уменьшает таймер шмотки на 16%
	if (obj->has_flag(EObjFlag::kSharpen)) {
		int timer = obj->get_timer()
			- std::max(1000, obj->get_timer() / 6); // абуз, таймер меньше 6 вычитается 0 бесконечная прокачка умелки
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
		act("$o не выдержал$G издевательств и рассыпал$U в мелкую пыль...",
			false, ch, obj, nullptr, kToChar);
		ExtractObjFromWorld(obj);
		return;
	}
	//При 200% заточки шмотка будет точиться на 4-5 хитролов и 4-5 дамролов
	min_mod = ch->get_trained_skill(ESkill::kSharpening) / 50;
	//С мортами все меньший уровень требуется для макс. заточки
	max_mod = std::clamp((GetRealLevel(ch) + 5 + GET_REAL_REMORT(ch)/4)/6, 1, 5);
	oldstate = check_unlimited_timer(obj); // запомним какая шмотка была до заточки
	if (IS_IMMORTAL(ch)) {
		add_dr = add_hr = 10;
	} else {
		add_dr = add_hr = (max_mod <= min_mod) ? min_mod : number(min_mod, max_mod);
	}
	if (percent > prob || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		act("Но только загубили $S.", false, ch, obj, nullptr, kToChar);
		add_hr = -add_hr;
		add_dr = -add_dr;
	} else {
		act("И вроде бы неплохо в итоге получилось.", false, ch, obj, nullptr, kToChar);
	}

	obj->set_affected(0, EApply::kHitroll, add_hr);
	obj->set_affected(1, EApply::kDamroll, add_dr);

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

void DoArmoring(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	char arg2[kMaxInputLength];
	int add_ac, prob, percent, i, armorvalue;
	const auto &strengthening = GlobalObjects::strengthening();

	if (!ch->get_skill(ESkill::kArmoring)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	}

	two_arguments(argument, arg, arg2);

	if (!*arg)
		SendMsgToChar("Что вы хотите укрепить?\r\n", ch);

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}

	if (!ObjSystem::is_armor_type(obj)) {
		SendMsgToChar("Вы можете укрепить только доспех.\r\n", ch);
		return;
	}

	if (obj->has_flag(EObjFlag::kMagic) || obj->has_flag(EObjFlag::kArmored)) {
		SendMsgToChar("Вы не можете укрепить этот предмет.\r\n", ch);
		return;
	}

	// Make sure no other affections.
	for (i = 0; i < kMaxObjAffect; i++)
		if (obj->get_affected(i).location != EApply::kNone) {
			SendMsgToChar("Этот предмет не может быть укреплен.\r\n", ch);
			return;
		}

	if (!OBJWEAR_FLAGGED(obj, (to_underlying(EWearFlag::kBody)
		| to_underlying(EWearFlag::kShoulders)
		| to_underlying(EWearFlag::kHead)
		| to_underlying(EWearFlag::kArms)
		| to_underlying(EWearFlag::kLegs)
		| to_underlying(EWearFlag::kFeet)))) {
		act("$o3 невозможно укрепить.", false, ch, obj, nullptr, kToChar);
		return;
	}
	if (obj->get_owner() != GET_UNIQUE(ch)) {
		SendMsgToChar(ch, "Укрепить можно только лично сделанный предмет.\r\n");
		return;
	}
	if (!*arg2 && (GET_SKILL(ch, ESkill::kArmoring) >= 100)) {
		SendMsgToChar(ch,
					  "Укажите параметр для улучшения: поглощение, здоровье, живучесть (сопротивление),"
					  " стойкость (сопротивление), огня (сопротивление), воздуха (сопротивление), воды (сопротивление), земли (сопротивление)\r\n");
		return;
	}
	switch (obj->get_material()) {
		case EObjMaterial::kIron:
		case EObjMaterial::kSteel:
		case EObjMaterial::kBulat: act("Вы принялись закалять $o3.",
									   false, ch, obj, nullptr, kToChar);
			act("$n принял$u закалять $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			break;

		case EObjMaterial::kWood:
		case EObjMaterial::kHardWood: act("Вы принялись обшивать $o3 железом.",
										  false, ch, obj, nullptr, kToChar);
			act("$n принял$u обшивать $o3 железом.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			break;

		case EObjMaterial::kSkin: act("Вы принялись проклепывать $o3.",
									  false, ch, obj, nullptr, kToChar);
			act("$n принял$u проклепывать $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			break;

		default: sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n", OBJN(obj, ch, 0));
			SendMsgToChar(buf, ch);
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
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(ch, "увеличиваю поглот на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kAbsorbe, armorvalue);
		} else if (CompareParam(arg2, "здоровье")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::HEALTH);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			SendMsgToChar(ch, "увеличиваю здоровье на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kSavingCritical, armorvalue);
		} else if (CompareParam(arg2, "живучесть"))// резисты в - лучше
		{
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::VITALITY);
			armorvalue = -std::max(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			SendMsgToChar(ch, "увеличиваю живучесть на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistVitality, armorvalue);
		} else if (CompareParam(arg2, "стойкость")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::STAMINA);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			SendMsgToChar(ch, "увеличиваю стойкость на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kSavingStability, armorvalue);
		} else if (CompareParam(arg2, "воздуха")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::AIR_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(ch, "увеличиваю сопр воздуха на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistAir, armorvalue);
		} else if (CompareParam(arg2, "воды")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::WATER_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(ch, "увеличиваю сопр воды на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistWater, armorvalue);
		} else if (CompareParam(arg2, "огня")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::FIRE_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(ch, "увеличиваю сопр огню на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistFire, armorvalue);
		} else if (CompareParam(arg2, "земли")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::EARTH_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(ch, "увеличиваю сопр земли на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistEarth, armorvalue);
		} else {
			SendMsgToChar(ch, "Но не поняли что улучшать.\r\n");
			return;
		}
		armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::TIMER);
		int timer =
			obj->get_timer() * strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::TIMER) / 100;
		obj->set_timer(timer);
//		SendMsgToChar(ch, "увеличиваю таймер на %d%, устанавливаю таймер %d\r\n", armorvalue, timer);
		armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::ARMOR);
//		SendMsgToChar(ch, "увеличиваю армор на %d скилл равен %d  значение берем %d\r\n", armorvalue, GET_SKILL(ch, ESkill::kArmoring), (GET_SKILL(ch, ESkill::kArmoring) / 10 * 10) );
		obj->set_affected(2, EApply::kArmour, armorvalue);
		obj->set_extra_flag(EObjFlag::kArmored);
		obj->set_extra_flag(EObjFlag::kTransformed); // установили флажок трансформации кодом
	}
	obj->set_affected(0, EApply::kAc, add_ac);
}

void feed_charmice(CharData *ch, char *local_arg) {
	int max_charm_duration = 1;
	int chance_to_eat = 0;
	struct Follower *k;
	int reformed_hp_summ = 0;

	auto obj = get_obj_in_list_vis(ch, local_arg, world[ch->in_room]->contents);

	if (!obj || !IS_CORPSE(obj) || !ch->has_master()) {
		return;
	}

	for (k = ch->get_master()->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->ch, EAffect::kCharmed)
			&& k->ch->get_master() == ch->get_master()) {
			reformed_hp_summ += get_reformed_charmice_hp(ch->get_master(), k->ch, kSpellAnimateDead);
		}
	}

	if (reformed_hp_summ >= get_player_charms(ch->get_master(), kSpellAnimateDead)) {
		SendMsgToChar("Вы не можете управлять столькими последователями.\r\n", ch->get_master());
		ExtractCharFromWorld(ch, false);
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
	if (IsAffectedBySpell(ch->get_master(), kSpellFascination)) {
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
		ExtractObjFromWorld(obj);
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

	Affect<EApply> af;
	af.type = kSpellCharm;
	af.duration = std::min(max_charm_duration, (int) (mob_level * max_charm_duration / 30));
	af.modifier = 0;
	af.location = EApply::kNone;
	af.bitvector = to_underlying(EAffect::kCharmed);
	af.battleflag = 0;

	ImposeAffect(ch, af);

	act("Громко чавкая, $N сожрал$G труп.", true, ch, obj, ch, kToRoom | kToArenaListen);
	act("Похоже, лакомство пришлось по вкусу.", true, ch, nullptr, ch->get_master(), kToVict);
	act("От омерзительного зрелища вас едва не вывернуло.",
		true, ch, nullptr, ch->get_master(), kToNotVict | kToArenaListen);

	if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
		GET_HIT(ch) = std::min(GET_HIT(ch) + std::min(max_heal_hp, GET_MAX_HIT(ch)), GET_MAX_HIT(ch));
	}

	if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
		act("$n сыто рыгнул$g и благодарно посмотрел$g на вас.", true, ch, nullptr, ch->get_master(), kToVict);
		act("$n сыто рыгнул$g и благодарно посмотрел$g на $N3.",
			true, ch, nullptr, ch->get_master(), kToNotVict | kToArenaListen);
	}

	ExtractObjFromWorld(obj);
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

	if (ch->IsNpc())
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
		SendMsgToChar("На чем царапаем?\r\n", ch);
	} else {
		if (!(target = get_obj_in_list_vis(ch, objname, ch->carrying))) {
			sprintf(buf, "У вас нет \'%s\'.\r\n", objname);
			SendMsgToChar(buf, ch);
		} else {
			if (erase_only) {
				target->remove_custom_label();
				act("Вы затерли надписи на $o5.", false, ch, target, nullptr, kToChar);
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
				act(msg, false, ch, target, nullptr, kToChar);
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
