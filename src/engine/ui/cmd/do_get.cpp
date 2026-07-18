#include "gameplay/mechanics/depot.h"
#include "gameplay/mechanics/sight.h"
#include "utils/grammar/declensions.h"
#include "engine/entities/char_data.h"
#include "gameplay/fight/pk.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/clans/house.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/economics/currencies.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/groups.h"
#include "engine/db/world_objects.h"

#include <fmt/format.h>

extern bool CanTakeObj(CharData *ch, ObjData *obj);
extern const char *sight::find_exdesc(const char *word, const std::vector<ExtraDescription> &list);

/// считаем сколько у ch в группе еще игроков (не мобов)
int other_pc_in_group(CharData *ch) {
	int num = 0;
	CharData *k = ch->has_master() ? ch->get_master() : ch;
	for (auto *f : k->followers) {
		if (AFF_FLAGGED(f, EAffect::kGroup)
			&& !f->IsNpc()
			&& f->in_room == ch->in_room) {
			++num;
		}
	}
	return num;
}

void split_or_clan_tax(CharData *ch, long amount) {
	if (AFF_FLAGGED(ch, EAffect::kGroup) && (other_pc_in_group(ch) > 0) &&
		ch->IsFlagged(EPrf::kAutosplit)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%ld", amount);
		group::do_split(ch, buf_, 0, 0);
	} else {
		long tax = ClanSystem::do_gold_tax(ch, amount);
		currencies::RemoveHand(*ch, currencies::kGold, tax);
	}
}

void get_check_money(CharData *ch, ObjData *obj, ObjData *cont) {

	if (system_obj::is_purse(obj) && GET_OBJ_VAL(obj, 3) == ch->get_uid()) {
		system_obj::process_open_purse(ch, obj);
		return;
	}

	const int value = GET_OBJ_VAL(obj, 0);
	const int curr_type = GET_OBJ_VAL(obj, 1);

	if (obj->get_type() != EObjType::kMoney || value <= 0) {
		return;
	}

	const auto &money_cur = MUD::Currency(curr_type);
	if (money_cur.GetId() >= 0 && money_cur.GetTextId() != currencies::kGold) {
		// Не-золотая валюта (напр. событийная): зачисляем; force_split - делим всегда.
		sprintf(buf, "Это составило %d %s.\r\n", value, money_cur.GetNameWithAmount(value, grammar::ECase::kAcc).c_str());
		SendMsgToChar(buf, ch);
		currencies::AddHand(*ch, curr_type, value);
		if (money_cur.ForceSplit() && AFF_FLAGGED(ch, EAffect::kGroup) && other_pc_in_group(ch) > 0) {
			char local_buf[256];
			sprintf(local_buf, "%d", value);
			group::do_split(ch, local_buf, 0, 0, curr_type);
		}
		ExtractObjFromWorld(obj);
		return;
	}

	sprintf(buf, "Это составило %d %s.\r\n", value, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(value, grammar::ECase::kAcc).c_str());
	SendMsgToChar(buf, ch);
	if (InTestZone(ch)) {
		ExtractObjFromWorld(obj);
		return;
	}
	// все, что делится на группу - идет через налог (из кошельков не делится)
	if (AFF_FLAGGED(ch, EAffect::kGroup) && other_pc_in_group(ch) > 0 &&
		ch->IsFlagged(EPrf::kAutosplit) && (!cont || !system_obj::is_purse(cont))) {
		// добавляем бабло, пишем в лог, клан-налог снимаем
		// только по факту деления на группу в do_split()
		currencies::AddHand(*ch, currencies::kGold, value);
		sprintf(buf,
				"<%s> {%d} заработал %d %s в группе.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				value,
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(value, grammar::ECase::kNom).c_str());
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		char local_buf[256];
		sprintf(local_buf, "%d", value);
		group::do_split(ch, local_buf, 0, 0);
	} else if (cont && system_obj::is_purse(cont)) {
		// лут кошелька с баблом
		// налогом не облагается, т.к. уже все уплочено
		// на данном этапе cont уже не содержит владельца
		sprintf(buf, "%s взял деньги из кошелька: %d  %s.", ch->get_name().c_str(), value,
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(value, grammar::ECase::kNom).c_str());
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		currencies::AddHand(*ch, currencies::kGold, value);
	} else if ((cont && IS_MOB_CORPSE(cont)) || GET_OBJ_VNUM(obj) != -1) {
		// лут из трупа моба или из предметов-денег с внумом
		// (предметы-награды в зонах) - снимаем клан-налог
		sprintf(buf, "%s заработал %d  %s.", ch->get_name().c_str(), value,
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(value, grammar::ECase::kNom).c_str());
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		currencies::AddHand(*ch, currencies::kGold, value - ClanSystem::do_gold_tax(ch, value), false, true);
	} else {
		sprintf(buf,
				"<%s> {%d} как-то получил %d  %s.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				value,
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(value, grammar::ECase::kNom).c_str());
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		currencies::AddHand(*ch, currencies::kGold, value);
	}
	RemoveObjFromChar(obj);
	ExtractObjFromWorld(obj);
}

// return 0 - чтобы словить невозможность взять из клан-сундука,
// иначе при вз все сун будет спам на каждый предмет, мол низя
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode) {
	if (!bloody::handle_transfer(nullptr, ch, obj))
		return false;
	if ((mode == EFind::kObjInventory || mode == EFind::kObjRoom || mode == EFind::kObjEquip) && CanTakeObj(ch, obj)
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
		RemoveObjFromObj(obj);
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
	if (IS_SET(GET_OBJ_VAL((cont), 1), (EContainerFlag::kShutted)))
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
			if (sight::CanSeeObj(ch, obj)
				&& (obj_dotmode == kFindAll
					|| isname(local_arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(local_arg, obj, ch))) {
				if (autoloot
					&& (obj->get_type() == EObjType::kMagicIngredient
						|| obj->get_type() == EObjType::kMagicComponent)
					&& ch->IsFlagged(EPrf::kNoIngrLoot)) {
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
	if (CanTakeObj(ch, obj) && get_otrigger(obj, ch) && bloody::handle_transfer(nullptr, ch, obj)) {
		RemoveObjFromRoom(obj);
		if (ch->IsNpc() && !obj->has_flag(EObjFlag::kTicktimer)) { 
			world_objects.decay_manager().remove(obj);
		}
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
	ObjData *obj;
	int dotmode;
	bool found = false;

	// Are they trying to take something in a room extra description?
	if (sight::find_exdesc(local_arg, world[ch->in_room]->ex_description) != nullptr) {
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
		for (auto it = world[ch->in_room]->contents.begin(); it != world[ch->in_room]->contents.end(); ) {
			auto obj = *it; ++it;
			if (sight::CanSeeObj(ch, obj)
				&& (dotmode == kFindAll
					|| isname(local_arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(local_arg, obj, ch))) {
				if (perform_get_from_room(ch, obj) == 1) {
					found = true;
				}
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

	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch))
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
			} else if (cont->get_type() != EObjType::kContainer) {
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
				if (sight::CanSeeObj(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch))) {
					if (cont->get_type() == EObjType::kContainer) {
						found = 1;
						get_from_container(ch, cont, theobj, EFind::kObjInventory, amount, false);
					} else if (cont_dotmode == kFindAlldot) {
						found = 1;
						act("$o - не контейнер.", false, ch, cont, nullptr, kToChar);
					}
				}
			}
			if (IS_SET(where_bits, EFind::kObjRoom)) for (auto cont : world[ch->in_room]->contents) {
				if (sight::CanSeeObj(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch))) {
					if (cont->get_type() == EObjType::kContainer) {
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
