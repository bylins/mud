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
#include "game_fight/fight.h"
#include "handler.h"
#include "house.h"
#include "liquid.h"
#include "game_mechanics/named_stuff.h"
#include "game_fight/pk.h"
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
void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot);
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode);
void RemoveEquipment(CharData *ch, int pos);
int invalid_anti_class(CharData *ch, const ObjData *obj);
void feed_charmice(CharData *ch, char *local_arg);

int invalid_unique(CharData *ch, const ObjData *obj);

// from class.cpp
int invalid_no_class(CharData *ch, const ObjData *obj);

void do_eat(CharData *ch, char *argument, int cmd, int subcmd);
void do_wear(CharData *ch, char *argument, int cmd, int subcmd);
void do_wield(CharData *ch, char *argument, int cmd, int subcmd);
void do_refill(CharData *ch, char *argument, int cmd, int subcmd);

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

	SendMsgToChar("С таким успехом можно пополнять соседние камни, для разговоров по ним.\r\n", ch);
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
