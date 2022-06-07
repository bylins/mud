#include "depot.h"
#include "entities/char_data.h"
#include "entities/world_objects.h"
#include "game_economics/currencies.h"
#include "game_fight/pk.h"
#include "house.h"
#include "utils/utils_char_obj.inl"
#include "structs/global_objects.h"

ObjData::shared_ptr CreateCurrencyObj(long quantity);

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
					obj = CreateCurrencyObj(money);
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

					const auto obj = CreateCurrencyObj(howmany);

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

ObjData::shared_ptr CreateCurrencyObj(long quantity) {
	char buf[200];

	if (quantity <= 0) {
		log("SYSERR: Try to create negative or 0 money. (%ld)", quantity);
		return (nullptr);
	}
	auto obj = world_objects.create_blank();
	ExtraDescription::shared_ptr new_descr(new ExtraDescription());

	if (quantity == 1) {
		sprintf(buf, "coin gold кун деньги денег монет %s",
				MUD::Currency(currencies::kKunaVnum).GetObjCName(quantity, ECase::kNom));
		obj->set_aliases(buf);
		obj->set_short_description("куна");
		obj->set_description("Одна куна лежит здесь.");
		new_descr->keyword = str_dup("coin gold монет кун денег");
		new_descr->description = str_dup("Всего лишь одна куна.");
		for (int i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
			obj->set_PName(i,
						   MUD::Currency(currencies::kKunaVnum).GetObjCName(quantity, static_cast<ECase>(i)));
		}
	} else {
		sprintf(buf, "coins gold кун денег %s",
				MUD::Currency(currencies::kKunaVnum).GetObjCName(quantity, ECase::kNom));
		obj->set_aliases(buf);
		obj->set_short_description(MUD::Currency(currencies::kKunaVnum).GetObjCName(quantity, ECase::kNom));
		for (int i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
			obj->set_PName(i, MUD::Currency(currencies::kKunaVnum).GetObjCName(quantity, static_cast<ECase>(i)));
		}

		sprintf(buf, "Здесь лежит %s.",
				MUD::Currency(currencies::kKunaVnum).GetObjCName(quantity, ECase::kNom));
		obj->set_description(CAP(buf));

		new_descr->keyword = str_dup("coins gold кун денег");
	}

	new_descr->next = nullptr;
	obj->set_ex_description(new_descr);

	obj->set_type(EObjType::kMoney);
	obj->set_wear_flags(to_underlying(EWearFlag::kTake));
	obj->set_sex(EGender::kFemale);
	obj->set_val(0, quantity);
	obj->set_cost(quantity);
	obj->set_maximum_durability(ObjData::DEFAULT_MAXIMUM_DURABILITY);
	obj->set_current_durability(ObjData::DEFAULT_CURRENT_DURABILITY);
	obj->set_timer(24 * 60 * 7);
	obj->set_weight(1);
	obj->set_extra_flag(EObjFlag::kNodonate);
	obj->set_extra_flag(EObjFlag::kNosell);

	return obj;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
