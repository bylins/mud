/* ************************************************************************
*   File: act.other.cpp                                 Part of Bylins    *
*  Usage: Miscellaneous player-level commands                             *
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

#include "act_other.h"

#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <iterator>
#include <set>
#include <utility>
#include <iomanip>

#include <third_party_libs/fmt/include/fmt/format.h>

#include "utils/utils_char_obj.inl"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "entities/entities_constants.h"
#include "cmd/follow.h"
#include "cmd/do_features.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "depot.h"
#include "dg_script/dg_scripts.h"
#include "feats.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "game_fight/pk.h"
#include "game_mechanics/mem_queue.h"
#include "handler.h"
#include "house.h"
#include "interpreter.h"
#include "utils/logger.h"
#include "game_magic/magic.h"
#include "game_magic/magic_rooms.h"
#include "msdp/msdp_constants.h"
#include "noob.h"
#include "entities/obj_data.h"
#include "obj_prototypes.h"
#include "obj_save.h"
#include "administration/privilege.h"
#include "utils/random.h"
#include "communication/remember.h"
#include "entities/room_data.h"
#include "color.h"
#include "game_economics/shop_ext.h"
#include "game_skills/skills.h"
#include "game_magic/spells.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "entities/world_objects.h"
#include "game_skills/skills_info.h"
#include "game_mechanics/weather.h"
#include "structs/global_objects.h"
#include "utils/table_wrapper.h"

// extern variables
extern int nameserver_is_slow;
//void appear(CharacterData *ch);
void write_aliases(CharData *ch);
void perform_immort_vis(CharData *ch);
void do_gen_comm(CharData *ch, char *argument, int cmd, int subcmd);
extern char *color_value(CharData *ch, int real, int max);
//int posi_value(int real, int max);
extern void split_or_clan_tax(CharData *ch, long amount);
extern bool IsWearingLight(CharData *ch);
// local functions
void do_antigods(CharData *ch, char *argument, int cmd, int subcmd);
void do_quit(CharData *ch, char *argument, int /* cmd */, int subcmd);
void do_save(CharData *ch, char *argument, int cmd, int subcmd);
void do_not_here(CharData *ch, char *argument, int cmd, int subcmd);
void do_sneak(CharData *ch, char *argument, int cmd, int subcmd);
void do_hide(CharData *ch, char *argument, int cmd, int subcmd);
void do_steal(CharData *ch, char *argument, int cmd, int subcmd);
void do_visible(CharData *ch, char *argument, int cmd, int subcmd);
void print_group(CharData *ch);
void do_group(CharData *ch, char *argument, int cmd, int subcmd);
void do_ungroup(CharData *ch, char *argument, int cmd, int subcmd);
void do_report(CharData *ch, char *argument, int cmd, int subcmd);
void do_split(CharData *ch, char *argument, int cmd, int subcmd);
void do_split(CharData *ch, char *argument, int cmd, int subcmd, int currency);
void do_wimpy(CharData *ch, char *argument, int cmd, int subcmd);
void do_display(CharData *ch, char *argument, int cmd, int subcmd);
void do_gen_tog(CharData *ch, char *argument, int cmd, int subcmd);
void do_courage(CharData *ch, char *argument, int cmd, int subcmd);
void do_toggle(CharData *ch, char *argument, int cmd, int subcmd);
void do_recall(CharData *ch, char *argument, int cmd, int subcmd);
void do_dig(CharData *ch, char *argument, int cmd, int subcmd);

void do_antigods(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_IMMORTAL(ch)) {
		SendMsgToChar("Оно вам надо?\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kGodsShield)) {
		if (IsAffectedBySpell(ch, ESpell::kGodsShield)) {
			RemoveAffectFromChar(ch, ESpell::kGodsShield);
		}
		AFF_FLAGS(ch).unset(EAffect::kGodsShield);
		SendMsgToChar("Голубой кокон вокруг вашего тела угас.\r\n", ch);
		act("&W$n отринул$g защиту, дарованную богами.&n", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
	} else
		SendMsgToChar("Вы и так не под защитой богов.\r\n", ch);
}

void do_quit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	DescriptorData *d, *next_d;

	if (ch->IsNpc() || !ch->desc)
		return;

	if (subcmd != SCMD_QUIT)
		SendMsgToChar("Вам стоит набрать эту команду полностью во избежание недоразумений!\r\n", ch);
	else if (ch->GetPosition() == EPosition::kFight)
		SendMsgToChar("Угу! Щаз-з-з! Вы, батенька, деретесь!\r\n", ch);
	else if (ch->GetPosition() < EPosition::kStun) {
		SendMsgToChar("Вас пригласила к себе владелица косы...\r\n", ch);
		die(ch, nullptr);
	}
	else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		if (GET_SEX(ch) == EGender::kMale)
			SendMsgToChar("Сдался салага? Не выйдет...", ch);
		else
			SendMsgToChar("Сдалась салага? Не выйдет...", ch);
		return;
	} else if (AFF_FLAGGED(ch, EAffect::kSleep)) {
		return;
	} else if (*argument) {
		SendMsgToChar("Если вы хотите выйти из игры с потерей всех вещей, то просто наберите 'конец'.\r\n", ch);
	} else {
//		int loadroom = ch->in_room;
		if (NORENTABLE(ch)) {
			SendMsgToChar("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
			return;
		}
		if (!GET_INVIS_LEV(ch))
			act("$n покинул$g игру.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		sprintf(buf, "%s quit the game.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		SendMsgToChar("До свидания, странник... Мы ждем тебя снова!\r\n", ch);

		long depot_cost = static_cast<long>(Depot::get_total_cost_per_day(ch));
		if (depot_cost) {
			SendMsgToChar(ch,
						  "За вещи в хранилище придется заплатить %ld %s в день.\r\n",
						  depot_cost,
						  GetDeclensionInNumber(depot_cost, EWhat::kMoneyU));
			long deadline = ((ch->get_gold() + ch->get_bank()) / depot_cost);
			SendMsgToChar(ch, "Твоих денег хватит на %ld %s.\r\n", deadline,
						  GetDeclensionInNumber(deadline, EWhat::kDay));
		}
		Depot::exit_char(ch);
		Clan::clan_invoice(ch, false);

		/*
		 * kill off all sockets connected to the same player as the one who is
		 * trying to quit.  Helps to maintain sanity as well as prevent duping.
		 */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (d == ch->desc)
				continue;
			if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
				STATE(d) = CON_DISCONNECT;
		}
		ExtractCharFromWorld(ch, false);
	}
}

void do_summon(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *horse = nullptr;
	horse = ch->get_horse();
	if (!horse) {
		SendMsgToChar("У вас нет скакуна!\r\n", ch);
		return;
	}

	if (ch->in_room == horse->in_room) {
		SendMsgToChar("Но ваш cкакун рядом с вами!\r\n", ch);
		return;
	}

	SendMsgToChar("Ваш скакун появился перед вами.\r\n", ch);
	act("$n исчез$q в голубом пламени.", true, horse, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(horse);
	PlaceCharToRoom(horse, ch->in_room);
	look_at_room(horse, 0);
	act("$n появил$u из голубого пламени!", true, horse, nullptr, nullptr, kToRoom);
}

void do_save(CharData *ch, char * /*argument*/, int cmd, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->desc) {
		return;
	}

	// Only tell the char we're saving if they actually typed "save"
	if (cmd) {
		SendMsgToChar("Сохраняю игрока, синонимы и вещи.\r\n", ch);
		SetWaitState(ch, 3 * kBattleRound);
	}
	write_aliases(ch);
	ch->save_char();
	Crash_crashsave(ch);
}

// generic function for commands which are normally overridden by
// special procedures - i.e., shop commands, mail commands, etc.
void do_not_here(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("Эта команда недоступна в этом месте!\r\n", ch);
}

int check_awake(CharData *ch, int what) {
	int i, retval = 0, wgt = 0;

	if (!IS_GOD(ch)) {
		if (IS_SET(what, kAcheckAffects)
			&& (AFF_FLAGGED(ch, EAffect::kStairs) || AFF_FLAGGED(ch, EAffect::kSanctuary)))
			SET_BIT(retval, kAcheckAffects);

		if (IS_SET(what, kAcheckLight) &&
			IS_DEFAULTDARK(ch->in_room)
			&& (AFF_FLAGGED(ch, EAffect::kSingleLight) || AFF_FLAGGED(ch, EAffect::kHolyLight)))
			SET_BIT(retval, kAcheckLight);

		for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (!GET_EQ(ch, i))
				continue;

			if (IS_SET(what, kAcheckHumming) && GET_EQ(ch, i)->has_flag(EObjFlag::kHum))
				SET_BIT(retval, kAcheckHumming);

			if (IS_SET(what, kAcheckGlowing) && GET_EQ(ch, i)->has_flag(EObjFlag::kGlow))
				SET_BIT(retval, kAcheckGlowing);

			if (IS_SET(what, kAcheckLight)
				&& IS_DEFAULTDARK(ch->in_room)
				&& GET_OBJ_TYPE(GET_EQ(ch, i)) == EObjType::kLightSource
				&& GET_OBJ_VAL(GET_EQ(ch, i), 2)) {
				SET_BIT(retval, kAcheckLight);
			}

			if (ObjSystem::is_armor_type(GET_EQ(ch, i))
				&& GET_OBJ_MATER(GET_EQ(ch, i)) <= EObjMaterial::kPreciousMetel) {
				wgt += GET_OBJ_WEIGHT(GET_EQ(ch, i));
			}
		}

		if (IS_SET(what, kAcheckWeight) && wgt > GetRealStr(ch) * 2)
			SET_BIT(retval, kAcheckWeight);
	}
	return (retval);
}

int awake_hide(CharData *ch) {
	if (IS_GOD(ch))
		return (false);
	return check_awake(ch, kAcheckAffects | kAcheckLight | kAcheckHumming
		| kAcheckGlowing | kAcheckWeight);
}

int awake_sneak(CharData *ch) {
	return awake_hide(ch);
}

int awake_invis(CharData *ch) {
	if (IS_GOD(ch))
		return (false);
	return check_awake(ch, kAcheckAffects | kAcheckLight | kAcheckHumming
		| kAcheckGlowing);
}

int awake_camouflage(CharData *ch) {
	return awake_invis(ch);
}

int awaking(CharData *ch, int mode) {
	if (IS_GOD(ch))
		return (false);
	if (IS_SET(mode, kAwHide) && awake_hide(ch))
		return (true);
	if (IS_SET(mode, kAwInvis) && awake_invis(ch))
		return (true);
	if (IS_SET(mode, kAwCamouflage) && awake_camouflage(ch))
		return (true);
	if (IS_SET(mode, kAwSneak) && awake_sneak(ch))
		return (true);
	return (false);
}

int char_humming(CharData *ch) {
	int i;

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		return (false);

	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i) && GET_EQ(ch, i)->has_flag(EObjFlag::kHum))
			return (true);
	}
	return (false);
}

void do_sneak(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int prob, percent;

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kSneak)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		act("Вам стоит подумать о мягкой обуви для $N1", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	if (IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		SendMsgToChar("Вы бесшумно крадетесь, отбрасывая тысячи солнечных зайчиков...\r\n", ch);
		return;
	}
	RemoveAffectFromChar(ch, ESpell::kSneak);
	SendMsgToChar("Хорошо, вы попытаетесь двигаться бесшумно.\r\n", ch);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILSNEAK);
	percent = number(1, MUD::Skill(ESkill::kSneak).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kSneak, nullptr);

	Affect<EApply> af;
	af.type = ESpell::kSneak;
	af.duration = CalcDuration(ch, 0, GetRealLevel(ch), 8, 0, 1);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;
	if (percent > prob) {
		af.bitvector = 0;
	} else {
		af.bitvector = to_underlying(EAffect::kSneak);
	}
	affect_to_char(ch, af);
}

void do_camouflage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedSkill timed;
	int prob, percent;

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kDisguise)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (ch->IsOnHorse()) {
		SendMsgToChar("Вы замаскировались под статую Юрия Долгорукого.\r\n", ch);
		return;
	}

	if (IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		SendMsgToChar("Вы замаскировались под золотую рыбку.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kDisguise)) {
		SendMsgToChar("У вас пока не хватает фантазии. Побудьте немного самим собой.\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch)) {
		RemoveAffectFromChar(ch, ESpell::kCamouflage);
	}

	if (IsAffectedBySpell(ch, ESpell::kCamouflage)) {
		SendMsgToChar("Вы уже маскируетесь.\r\n", ch);
		return;
	}

	SendMsgToChar("Вы начали усиленно маскироваться.\r\n", ch);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILCAMOUFLAGE);
	percent = number(1, MUD::Skill(ESkill::kDisguise).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kDisguise, nullptr);

	Affect<EApply> af;
	af.type = ESpell::kCamouflage;
	af.duration = CalcDuration(ch, 0, GetRealLevel(ch), 6, 0, 2);
	af.modifier = world[ch->in_room]->zone_rn;
	af.location = EApply::kNone;
	af.battleflag = 0;

	if (percent > prob) {
		af.bitvector = 0;
	} else {
		af.bitvector = to_underlying(EAffect::kDisguise);
	}

	affect_to_char(ch, af);
	if (!IS_IMMORTAL(ch)) {
		timed.skill = ESkill::kDisguise;
		timed.time = 2;
		ImposeTimedSkill(ch, &timed);
	}
}

void do_hide(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int prob, percent;

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kHide)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (ch->IsOnHorse()) {
		act("А куда вы хотите спрятать $N3?", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}

	RemoveAffectFromChar(ch, ESpell::kHide);

	if (IsAffectedBySpell(ch, ESpell::kHide)) {
		SendMsgToChar("Вы уже пытаетесь спрятаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы и не видите куда прятаться.\r\n", ch);
		return;
	}

	if (IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		SendMsgToChar("Спрятаться?! Да вы сверкаете как корчма во время гулянки!.\r\n", ch);
		return;
	}

	SendMsgToChar("Хорошо, вы попытаетесь спрятаться.\r\n", ch);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	percent = number(1, MUD::Skill(ESkill::kHide).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kHide, nullptr);

	Affect<EApply> af;
	af.type = ESpell::kHide;
	af.duration = CalcDuration(ch, 0, GetRealLevel(ch), 8, 0, 1);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;

	if (percent > prob) {
		af.bitvector = 0;
	} else {
		af.bitvector = to_underlying(EAffect::kHide);
	}

	affect_to_char(ch, af);
}

void go_steal(CharData *ch, CharData *vict, char *obj_name) {
	int percent, gold, eq_pos, ohoh = 0, success = 0, prob;
	ObjData *obj;

	if (!vict) {
		return;
	}

	if (!IS_IMMORTAL(ch) && vict->GetEnemy()) {
		act("$N слишком быстро перемещается.", false, ch, nullptr, vict, kToChar);
		return;
	}

	if (!IS_IMMORTAL(ch) && ROOM_FLAGGED(vict->in_room, ERoomFlag::kArena)) {
		SendMsgToChar("Воровство при поединке недопустимо.\r\n", ch);
		return;
	}

	// 101% is a complete failure
	percent = number(1, MUD::Skill(ESkill::kSteal).difficulty);

	if (IS_IMMORTAL(ch) || (vict->GetPosition() <= EPosition::kSleep && !AFF_FLAGGED(vict, EAffect::kSleep)))
		success = 1;    // ALWAYS SUCCESS, unless heavy object.

	if (!AWAKE(vict))    // Easier to steal from sleeping people.
		percent = MAX(percent - 50, 0);

	// NO NO With Imp's and Shopkeepers, and if player thieving is not allowed
	if ((IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, EGf::kGodsLike) || GET_MOB_SPEC(vict) == shop_ext)
		&& !IS_IMPL(ch)) {
		SendMsgToChar("Вы постеснялись красть у такого хорошего человека.\r\n", ch);
		return;
	}

	if (str_cmp(obj_name, "coins")
		&& str_cmp(obj_name, "gold")
		&& str_cmp(obj_name, "кун")
		&& str_cmp(obj_name, "деньги")) {
		if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying))) {
			for (eq_pos = 0; eq_pos < EEquipPos::kNumEquipPos; eq_pos++) {
				if (GET_EQ(vict, eq_pos)
					&& (isname(obj_name, GET_EQ(vict, eq_pos)->get_aliases()))
					&& CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
					obj = GET_EQ(vict, eq_pos);
					break;
				}
			}
			if (!obj) {
				act("А у н$S этого и нет - ха-ха-ха (2 раза)...", false, ch, nullptr, vict, kToChar);
				return;
			} else    // It is equipment
			{
				if (!success) {
					SendMsgToChar("Украсть? Из экипировки? Щаз-з-з!\r\n", ch);
					return;
				} else if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
					SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
					return;
				} else if (ch->GetCarryingWeight() + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
					SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
					return;
				} else if (obj->has_flag(EObjFlag::kBloody)) {
					SendMsgToChar(
						"\"Мокрухой пахнет!\" - пронеслось у вас в голове, и вы вовремя успели отдернуть руку, не испачкавшись в крови.\r\n",
						ch);
					return;
				} else {
					act("Вы раздели $N3 и взяли $o3.", false, ch, obj, vict, kToChar);
					act("$n украл$g $o3 у $N1.", false, ch, obj, vict, kToNotVict | kToArenaListen);
					PlaceObjToInventory(UnequipChar(vict, eq_pos, CharEquipFlags()), ch);
				}
			}
		} else    // obj found in inventory
		{
			if (obj->has_flag(EObjFlag::kBloody)) {
				SendMsgToChar(
					"\"Мокрухой пахнет!\" - пронеслось у вас в голове, и вы вовремя успели отдернуть руку, не испачкавшись в крови.\r\n",
					ch);
				return;
			}
			percent += GET_OBJ_WEIGHT(obj);    // Make heavy harder
			prob = CalcCurrentSkill(ch, ESkill::kSteal, vict);

			if (AFF_FLAGGED(ch, EAffect::kHide))
				prob += 5;
			if (!IS_IMMORTAL(ch) && AFF_FLAGGED(vict, EAffect::kSleep))
				prob = 0;
			if (percent > prob && !success) {
				ohoh = true;
				if (AFF_FLAGGED(ch, EAffect::kHide)) {
					RemoveAffectFromChar(ch, ESpell::kHide);
					AFF_FLAGS(ch).unset(EAffect::kHide);
					SendMsgToChar("Вы прекратили прятаться.\r\n", ch);
					act("$n прекратил$g прятаться.", false, ch, nullptr, nullptr, kToRoom);
				};
				SendMsgToChar("Атас.. Дружина на конях!\r\n", ch);
				act("$n пытал$u обокрасть вас!", false, ch, nullptr, vict, kToVict);
				act("$n пытал$u украсть нечто у $N1.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
			} else    // Steal the item
			{
				if (ch->GetCarryingQuantity() + 1 < CAN_CARRY_N(ch)) {
					if (ch->GetCarryingWeight() + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
						RemoveObjFromChar(obj);
						PlaceObjToInventory(obj, ch);
						act("Вы украли $o3 у $N1!", false, ch, obj, vict, kToChar);
					}
				} else {
					SendMsgToChar("Вы не можете столько нести.\r\n", ch);
					return;
				}
			}
			if (CAN_SEE(vict, ch) && AWAKE(vict))
				ImproveSkill(ch, ESkill::kSteal, 0, vict);
		}
	} else        // Steal some coins
	{
		prob = CalcCurrentSkill(ch, ESkill::kSteal, vict);
		if (AFF_FLAGGED(ch, EAffect::kHide))
			prob += 5;
		if (!IS_IMMORTAL(ch) && AFF_FLAGGED(vict, EAffect::kSleep))
			prob = 0;
		if (percent > prob && !success) {
			ohoh = true;
			if (AFF_FLAGGED(ch, EAffect::kHide)) {
				RemoveAffectFromChar(ch, ESpell::kHide);
				AFF_FLAGS(ch).unset(EAffect::kHide);
				SendMsgToChar("Вы прекратили прятаться.\r\n", ch);
				act("$n прекратил$g прятаться.", false, ch, nullptr, nullptr, kToRoom);
			};
			SendMsgToChar("Вы влипли... Вас посодют... А вы не воруйте..\r\n", ch);
			act("Вы обнаружили руку $n1 в своем кармане.", false, ch, nullptr, vict, kToVict);
			act("$n пытал$u спионерить деньги у $N1.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		} else    // Steal some gold coins
		{
			if (!vict->get_gold()) {
				act("$E богат$A, как амбарная мышь :)", false, ch, nullptr, vict, kToChar);
				return;
			} else {
				// Считаем вероятность крит-воровства (воровства всех денег)
				if ((number(1, 100) - ch->GetSkill(ESkill::kSteal) -
					ch->get_dex() + vict->get_wis() + vict->get_gold() / 500) < 0) {
					act("Тугой кошелек $N1 перекочевал к вам.", true, ch, nullptr, vict, kToChar);
					gold = vict->get_gold();
				} else
					gold = (int) ((vict->get_gold() * number(1, 75)) / 100);

				if (gold > 0) {
					if (gold > 1) {
						sprintf(buf, "УР-Р-Р-А! Вы таки сперли %d %s.\r\n",
								gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));
						SendMsgToChar(buf, ch);
					} else {
						SendMsgToChar("УРА-А-А ! Вы сперли :) 1 (одну) куну :(.\r\n", ch);
					}
					ch->add_gold(gold);
					sprintf(buf,
							"<%s> {%d} нагло спер %d кун у %s.",
							ch->get_name().c_str(),
							GET_ROOM_VNUM(ch->in_room),
							gold,
							GET_PAD(vict, 0));
					mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
					split_or_clan_tax(ch, gold);
					vict->remove_gold(gold);
				} else
					SendMsgToChar("Вы ничего не сумели украсть...\r\n", ch);
			}
		}
		if (CAN_SEE(vict, ch) && AWAKE(vict))
			ImproveSkill(ch, ESkill::kSteal, 0, vict);
	}
	if (!IS_IMMORTAL(ch) && ohoh)
		SetWaitState(ch, 3 * kBattleRound);
	pk_thiefs_action(ch, vict);
	if (ohoh && vict->IsNpc() && AWAKE(vict) && CAN_SEE(vict, ch) && MAY_ATTACK(vict))
		hit(vict, ch, ESkill::kUndefined, fight::kMainHand);
}

void do_steal(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	char vict_name[kMaxInputLength], obj_name[kMaxInputLength];

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kSteal)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch) && ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}
	two_arguments(argument, obj_name, vict_name);
	if (!(vict = get_char_vis(ch, vict_name, EFind::kCharInRoom))) {
		SendMsgToChar("Украсть у кого?\r\n", ch);
		return;
	} else if (vict == ch) {
		SendMsgToChar("Попробуйте набрать \"бросить <n> кун\".\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful) && !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("Здесь слишком мирно. Вам не хочется нарушать сию благодать...\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouse) && !vict->IsNpc()) {
		SendMsgToChar("Воровать у своих? Это мерзко...\r\n", ch);
		return;
	}
	if (vict->IsNpc() && (vict->IsFlagged(EMobFlag::kNoFight) || AFF_FLAGGED(vict, EAffect::kGodsShield)
		|| vict->IsFlagged(EMobFlag::kProtect))
		&& !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("А ежели поймают? Посодют ведь!\r\nПодумав так, вы отказались от сего намеренья.\r\n", ch);
		return;
	}
	go_steal(ch, vict, obj_name);
}

void do_visible(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_IMMORTAL(ch)) {
		perform_immort_vis(ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kInvisible)
		|| AFF_FLAGGED(ch, EAffect::kDisguise)
		|| AFF_FLAGGED(ch, EAffect::kHide)
		|| AFF_FLAGGED(ch, EAffect::kSneak)) {
		appear(ch);
		SendMsgToChar("Вы перестали быть невидимым.\r\n", ch);
	} else
		SendMsgToChar("Вы и так видимы.\r\n", ch);
}

void do_courage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int prob, dur;
	struct TimedSkill timed;
	int i;
	if (ch->IsNpc())        // Cannot use GET_COND() on mobs.
		return;

	if (!ch->GetSkill(ESkill::kCourage)) {
		SendMsgToChar("Вам это не по силам.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kCourage)) {
		SendMsgToChar("Вы не можете слишком часто впадать в ярость.\r\n", ch);
		return;
	}

	timed.skill = ESkill::kCourage;
	timed.time = 6;
	ImposeTimedSkill(ch, &timed);
	prob = CalcCurrentSkill(ch, ESkill::kCourage, nullptr) / 20;
	dur = 1 + MIN(5, ch->GetSkill(ESkill::kCourage) / 40);
	Affect<EApply> af[4];
	af[0].type = ESpell::kCourage;
	af[0].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[0].modifier = 40;
	af[0].location = EApply::kAc;
	af[0].bitvector = to_underlying(EAffect::kNoFlee);
	af[0].battleflag = 0;
	af[1].type = ESpell::kCourage;
	af[1].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[1].modifier = MAX(1, prob);
	af[1].location = EApply::kDamroll;
	af[1].bitvector = to_underlying(EAffect::kCourage);
	af[1].battleflag = 0;
	af[2].type = ESpell::kCourage;
	af[2].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[2].modifier = MAX(1, prob * 7);
	af[2].location = EApply::kAbsorbe;
	af[2].bitvector = to_underlying(EAffect::kCourage);
	af[2].battleflag = 0;
	af[3].type = ESpell::kCourage;
	af[3].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[3].modifier = 50;
	af[3].location = EApply::kHpRegen;
	af[3].bitvector = to_underlying(EAffect::kCourage);
	af[3].battleflag = 0;

	for (i = 0; i < 4; i++) {
		ImposeAffect(ch, af[i], false, false, false, false);
	}

	SendMsgToChar("Вы пришли в ярость.\r\n", ch);

	if ((obj = GET_EQ(ch, EEquipPos::kWield)) || (obj = GET_EQ(ch, EEquipPos::kBoths)))
		strcpy(buf, "Глаза $n1 налились кровью и $e яростно сжал$g в руках $o3.");
	else
		strcpy(buf, "Глаза $n1 налились кровью.");
	act(buf, false, ch, obj, nullptr, kToRoom | kToArenaListen);
}

int max_group_size(CharData *ch) {
	int bonus_commander = 0;
//	if (AFF_FLAGGED(ch, EAffectFlag::AFF_COMMANDER))
//		bonus_commander = VPOSI((ch->get_skill(ESkill::kLeadership) - 120) / 10, 0, 8);
	bonus_commander = VPOSI((ch->GetSkill(ESkill::kLeadership) - 200) / 8, 0, 8);
	return kMaxGroupedFollowers + (int) VPOSI((ch->GetSkill(ESkill::kLeadership) - 80) / 5, 0, 4) + bonus_commander;
}

bool is_group_member(CharData *ch, CharData *vict) {
	if (vict->IsNpc()
		|| !AFF_FLAGGED(vict, EAffect::kGroup)
		|| vict->get_master() != ch) {
		return false;
	} else {
		return true;
	}
}

int perform_group(CharData *ch, CharData *vict) {
	if (AFF_FLAGGED(vict, EAffect::kGroup)
		|| AFF_FLAGGED(vict, EAffect::kCharmed)
		|| vict->IsFlagged(EMobFlag::kTutelar)
		|| vict->IsFlagged(EMobFlag::kMentalShadow)
		|| IS_HORSE(vict)) {
		return (false);
	}

	AFF_FLAGS(vict).set(EAffect::kGroup);
	if (ch != vict) {
		act("$N принят$A в члены вашего кружка (тьфу-ты, группы :).", false, ch, nullptr, vict, kToChar);
		act("Вы приняты в группу $n1.", false, ch, nullptr, vict, kToVict);
		act("$N принят$A в группу $n1.", false, ch, nullptr, vict, kToNotVict | kToArenaListen);
	}
	return (true);
}

/**
* Смена лидера группы на персонажа с макс лидеркой.
* Сам лидер при этом остается в группе, если он живой.
* \param vict - новый лидер, если смена происходит по команде 'гр лидер имя',
* старый лидер соответственно группится обратно, если нулевой, то считаем, что
* произошла смерть старого лидера и новый выбирается по наибольшей лидерке.
*/
void change_leader(CharData *ch, CharData *vict) {
	if (ch->IsNpc()
		|| ch->has_master()
		|| !ch->followers) {
		return;
	}

	CharData *leader = vict;
	if (!leader) {
		// лидер умер, ищем согрупника с максимальным скиллом лидерки
		for (struct FollowerType *l = ch->followers; l; l = l->next) {
			if (!is_group_member(ch, l->follower))
				continue;
			if (!leader)
				leader = l->follower;
			else if (l->follower->GetSkill(ESkill::kLeadership) > leader->GetSkill(ESkill::kLeadership))
				leader = l->follower;
		}
	}

	if (!leader) {
		return;
	}

	// для реследования используем стандартные функции
	std::vector<CharData *> temp_list;
	for (struct FollowerType *n = nullptr, *l = ch->followers; l; l = n) {
		n = l->next;
		if (!is_group_member(ch, l->follower)) {
			continue;
		} else {
			CharData *temp_vict = l->follower;
			if (temp_vict->has_master()
				&& stop_follower(temp_vict, kSfSilence)) {
				continue;
			}

			if (temp_vict != leader) {
				temp_list.push_back(temp_vict);
			}
		}
	}

	// вся эта фигня только для того, чтобы при реследовании пройтись по списку в обратном
	// направлении и сохранить относительный порядок следования в группе
	if (!temp_list.empty()) {
		for (auto it = temp_list.rbegin(); it != temp_list.rend(); ++it) {
			leader->add_follower_silently(*it);
		}
	}

	// бывшего лидера последним закидываем обратно в группу, если он живой
	if (vict) {
		// флаг группы надо снять, иначе при регрупе не будет писаться о старом лидере
		//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
		ch->removeGroupFlags();
		leader->add_follower_silently(ch);
	}

	if (!leader->followers) {
		return;
	}

	ch->dps_copy(leader);
	perform_group(leader, leader);
	int followers = 0;
	for (struct FollowerType *f = leader->followers; f; f = f->next) {
		if (followers < max_group_size(leader)) {
			if (perform_group(leader, f->follower))
				++followers;
		} else {
			SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
			return;
		}
	}
}

void print_one_line(CharData *ch, CharData *k, int leader, int header) {
	const char *WORD_STATE[] = {"При смерти",
								"Оч.тяж.ран",
								"Оч.тяж.ран",
								" Тяж.ранен",
								" Тяж.ранен",
								"  Ранен   ",
								"  Ранен   ",
								"  Ранен   ",
								"Лег.ранен ",
								"Лег.ранен ",
								"Слег.ранен",
								" Невредим "
	};
	const char *MOVE_STATE[] = {"Истощен",
								"Истощен",
								"О.устал",
								" Устал ",
								" Устал ",
								"Л.устал",
								"Л.устал",
								"Хорошо ",
								"Хорошо ",
								"Хорошо ",
								"Отдохн.",
								" Полон "
	};
	const char *POS_STATE[] = {"Умер",
							   "Истекает кровью",
							   "При смерти",
							   "Без сознания",
							   "Спит",
							   "Отдыхает",
							   "Сидит",
							   "Сражается",
							   "Стоит"
	};

	auto generate_affects_string = [](CharData *k) -> std::string {
		std::string affects;
		affects += AFF_FLAGGED(k, EAffect::kSanctuary) ? "&RО" : (AFF_FLAGGED(k, EAffect::kPrismaticAura) ? "&RП" : " ");
		affects += AFF_FLAGGED(k, EAffect::kWaterBreath) ? "&gД" : " ";
		affects += AFF_FLAGGED(k, EAffect::kInvisible) ? "&CН" : " ";
		affects += (AFF_FLAGGED(k, EAffect::kSingleLight) 
					|| AFF_FLAGGED(k, EAffect::kHolyLight)
					|| (GET_EQ(k, EEquipPos::kLight)
						&& GET_OBJ_VAL(GET_EQ(k, EEquipPos::kLight), 2))) ? "&YС" : " ";
		affects += AFF_FLAGGED(k, EAffect::kFly) ? "&BЛ" : " ";

		return affects;
	};

	auto generate_debuf_string = [](CharData *k) -> std::string {
		std::string debuf;
		debuf += AFF_FLAGGED(k, EAffect::kHold) ? "&RХ" : " ";
		debuf += AFF_FLAGGED(k, EAffect::kSleep) ? "&BС" : " ";
		debuf += AFF_FLAGGED(k, EAffect::kSilence) ? "&yМ" : " ";
		debuf += AFF_FLAGGED(k, EAffect::kDeafness) ? "&gГ" : " ";
		debuf += AFF_FLAGGED(k, EAffect::kBlind) ? "&YБ" : " ";
		debuf += AFF_FLAGGED(k, EAffect::kCurse) ? "&mП" : " ";
		debuf += IsAffectedBySpell(k, ESpell::kFever) ? "&cЛ" : " ";

		return debuf;
	};

	auto generate_mem_string = [](CharData *k) -> std::string {
		int ok, ok2, div;
		if ((!IS_MANA_CASTER(k) && !k->mem_queue.Empty()) ||
			(IS_MANA_CASTER(k) && k->mem_queue.stored < GET_MAX_MANA(k))) {
			div = CalcManaGain(k);
			if (div > 0) {
				if (!IS_MANA_CASTER(k)) {
					ok2 = std::max(0, 1 + k->mem_queue.total - k->mem_queue.stored);
					ok2 = ok2 * 60 / div;    // время мема в сек
				} else {
					ok2 = std::max(0, 1 + GET_MAX_MANA(k) - k->mem_queue.stored);
					ok2 = ok2 / div;    // время восстановления в секундах
				}
				ok = ok2 / 60;
				ok2 %= 60;
				if (ok > 99)
					return fmt::format("&g{:02}", ok);
				else
					return fmt::format("&g{:02}:{:02}", ok, ok2);
			} else {
				return "&r  -  ";				
			}
		} else
			return " ";
	};


	if (k->IsNpc()) {
		std::ostringstream buffer;
		if (!header)
			buffer << "Персонаж            | Здоровье | Рядом | Аффект |  Дебаф  | Положение\r\n";

		buffer << fmt::format("&B{:<20}&n|", k->get_name().substr(0, 20));

		buffer << fmt::format("{}", color_value(ch, GET_HIT(k), GET_REAL_MAX_HIT(k)));
		buffer << fmt::format("{:<10}&n|", WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1]);
		buffer << fmt::format(" {:^7} &n|", ch->in_room == k->in_room ? "&gДа" : "&rНет");

		// АФФЕКТЫ 
		buffer << fmt::format(" {:<5}", generate_affects_string(k));
		buffer << fmt::format("{:<1} &n|", k->low_charm() ? "&nТ" : " ");
		
		// ДЕБАФЫ
		buffer << fmt::format(" {:<7} &n|", generate_debuf_string(k));
		
		buffer << fmt::format(" {:<10}\r\n", POS_STATE[(int) k->GetPosition()]);

		SendMsgToChar(buffer.str().c_str(), ch);

	} else {
		std::ostringstream buffer;
		if (!header)
			buffer << "Персонаж            | Здоровье | Энергия | Рядом | Учить | Аффект |  Дебаф  |  Кто  | Строй | Положение \r\n";

		std::string health_color = color_value(ch, GET_HIT(k), GET_REAL_MAX_HIT(k));
		std::string move_color = color_value(ch, GET_MOVE(k), GET_REAL_MAX_MOVE(k));

		buffer << fmt::format("&B{:<20}&n|", k->get_name()); 

		buffer << fmt::format("{}", health_color);
		buffer << fmt::format("{:<10}&n|", WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1]);

		buffer << fmt::format("{}", move_color);
		buffer << fmt::format("{:^9}&n|", MOVE_STATE[posi_value(GET_MOVE(k), GET_REAL_MAX_MOVE(k)) + 1]);

		buffer << fmt::format(" {:^7} &n|", ch->in_room == k->in_room ? "&gДа" : "&rНет");

		buffer << fmt::format(" {:^5} &n|", generate_mem_string(k));
		buffer << fmt::format(" {:<5}  &n|", generate_affects_string(k));
		buffer << fmt::format(" {:<7} &n|", generate_debuf_string(k));
		
		buffer << fmt::format(" {:^5} &n|", leader ? "Лидер" : "");
		buffer << fmt::format(" {:^5} &n|", k->IsFlagged(EPrf::kSkirmisher) ? " &gДа  " : "Нет");
		buffer << fmt::format(" {:<10}\r\n", POS_STATE[(int) k->GetPosition()]);

		SendMsgToChar(buffer.str().c_str(), ch);
	}
}

void print_list_group(CharData *ch) {
	CharData *k;
	struct FollowerType *f;
	int count = 1;
	k = (ch->has_master() ? ch->get_master() : ch);
	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, EAffect::kGroup)) {
			sprintf(buf1, "Лидер: %s\r\n", GET_NAME(k));
			SendMsgToChar(buf1, ch);
		}

		for (f = k->followers; f; f = f->next) {
			if (!AFF_FLAGGED(f->follower, EAffect::kGroup)) {
				continue;
			}
			sprintf(buf1, "%d. Согруппник: %s\r\n", count, GET_NAME(f->follower));
			SendMsgToChar(buf1, ch);
			count++;
		}
	} else {
		SendMsgToChar("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
	}
}

void print_group(CharData *ch) {
	int gfound = 0, cfound = 0;
	CharData *k;
	struct FollowerType *f, *g;

	k = ch->has_master() ? ch->get_master() : ch;
	if (!ch->IsNpc())
		ch->desc->msdp_report(msdp::constants::GROUP);

	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, EAffect::kGroup)) {
			print_one_line(ch, k, true, gfound++);
		}

		for (f = k->followers; f; f = f->next) {
			if (!AFF_FLAGGED(f->follower, EAffect::kGroup)) {
				continue;
			}
			print_one_line(ch, f->follower, false, gfound++);
		}
	}

	for (f = ch->followers; f; f = f->next) {
		if (!(AFF_FLAGGED(f->follower, EAffect::kCharmed)
			|| f->follower->IsFlagged(EMobFlag::kTutelar) || f->follower->IsFlagged(EMobFlag::kMentalShadow))) {
			continue;
		}
		if (!cfound)
			SendMsgToChar("Ваши последователи:\r\n", ch);
		print_one_line(ch, f->follower, false, cfound++);
	}
	if (!gfound && !cfound) {
		SendMsgToChar("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kShowGroup)) {
		for (g = k->followers, cfound = 0; g; g = g->next) {
			for (f = g->follower->followers; f; f = f->next) {
				if (!(AFF_FLAGGED(f->follower, EAffect::kCharmed)
					|| f->follower->IsFlagged(EMobFlag::kTutelar) || f->follower->IsFlagged(EMobFlag::kMentalShadow))
					|| !AFF_FLAGGED(ch, EAffect::kGroup)) {
					continue;
				}

				if (f->follower->get_master() == ch
					|| !AFF_FLAGGED(f->follower->get_master(), EAffect::kGroup)) {
					continue;
				}

				if (ch->IsFlagged(EPrf::kNoClones)
					&& f->follower->IsNpc()
					&& (f->follower->IsFlagged(EMobFlag::kClone)
						|| GET_MOB_VNUM(f->follower) == kMobKeeper)) {
					continue;
				}

				if (!cfound) {
					SendMsgToChar("Последователи членов вашей группы:\r\n", ch);
				}
				print_one_line(ch, f->follower, false, cfound++);
			}

			if (ch->has_master()) {
				if (!(AFF_FLAGGED(g->follower, EAffect::kCharmed)
					|| g->follower->IsFlagged(EMobFlag::kTutelar) || g->follower->IsFlagged(EMobFlag::kMentalShadow))
					|| !AFF_FLAGGED(ch, EAffect::kGroup)) {
					continue;
				}

				if (ch->IsFlagged(EPrf::kNoClones)
					&& g->follower->IsNpc()
					&& (g->follower->IsFlagged(EMobFlag::kClone)
						|| GET_MOB_VNUM(g->follower) == kMobKeeper)) {
					continue;
				}

				if (!cfound) {
					SendMsgToChar("Последователи членов вашей группы:\r\n", ch);
				}
				print_one_line(ch, g->follower, false, cfound++);
			}
		}
	}
}

void do_group(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	struct FollowerType *f;
	int found, f_number;

	argument = one_argument(argument, buf);

	if (!*buf) {
		print_group(ch);
		return;
	}

	if (!str_cmp(buf, "список")) {
		print_list_group(ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kRest) {
		SendMsgToChar("Трудно управлять группой в таком состоянии.\r\n", ch);
		return;
	}

	if (ch->has_master()) {
		act("Вы не можете управлять группой. Вы еще не ведущий.", false, ch, nullptr, nullptr, kToChar);
		return;
	}

	if (!ch->followers) {
		SendMsgToChar("За вами никто не следует.\r\n", ch);
		return;
	}



// вычисляем количество последователей
	for (f_number = 0, f = ch->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)) {
			f_number++;
		}
	}

	if (!str_cmp(buf, "all")
		|| !str_cmp(buf, "все")) {
		perform_group(ch, ch);
		for (found = 0, f = ch->followers; f; f = f->next) {
			if ((f_number + found) >= max_group_size(ch)) {
				SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			found += perform_group(ch, f->follower);
		}

		if (!found) {
			SendMsgToChar("Все, кто за вами следуют, уже включены в вашу группу.\r\n", ch);
		}

		return;
	} else if (!str_cmp(buf, "leader") || !str_cmp(buf, "лидер")) {
		vict = get_player_vis(ch, argument, EFind::kCharInWorld);
		// added by WorM (Видолюб) Если найден клон и его хозяин персонаж
		// а то чото как-то глючно Двойник %1 не является членом вашей группы.
		if (vict
			&& vict->IsNpc()
			&& vict->IsFlagged(EMobFlag::kClone)
			&& AFF_FLAGGED(vict, EAffect::kCharmed)
			&& vict->has_master()
			&& !vict->get_master()->IsNpc()) {
			if (CAN_SEE(ch, vict->get_master())) {
				vict = vict->get_master();
			} else {
				vict = nullptr;
			}
		}

		// end by WorM
		if (!vict) {
			SendMsgToChar("Нет такого персонажа.\r\n", ch);
			return;
		} else if (vict == ch) {
			SendMsgToChar("Вы и так лидер группы...\r\n", ch);
			return;
		} else if (!AFF_FLAGGED(vict, EAffect::kGroup)
			|| vict->get_master() != ch) {
			SendMsgToChar(ch, "%s не является членом вашей группы.\r\n", GET_NAME(vict));
			return;
		}
		change_leader(ch, vict);
		return;
	}

	if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom))) {
		SendMsgToChar(NOPERSON, ch);
	} else if ((vict->get_master() != ch) && (vict != ch)) {
		act("$N2 нужно следовать за вами, чтобы стать членом вашей группы.", false, ch, nullptr, vict, kToChar);
	} else {
		if (!AFF_FLAGGED(vict, EAffect::kGroup)) {
			if (AFF_FLAGGED(vict, EAffect::kCharmed) || vict->IsFlagged(EMobFlag::kTutelar)
				|| vict->IsFlagged(EMobFlag::kMentalShadow) || IS_HORSE(vict)) {
				SendMsgToChar("Только равноправные персонажи могут быть включены в группу.\r\n", ch);
				SendMsgToChar("Только равноправные персонажи могут быть включены в группу.\r\n", vict);
			};
			if (f_number >= max_group_size(ch)) {
				SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			perform_group(ch, ch);
			perform_group(ch, vict);
		} else if (ch != vict) {
			act("$N исключен$A из состава вашей группы.", false, ch, nullptr, vict, kToChar);
			act("Вы исключены из группы $n1!", false, ch, nullptr, vict, kToVict);
			act("$N был$G исключен$A из группы $n1!", false, ch, nullptr, vict, kToNotVict | kToArenaListen);
			//AFF_FLAGS(vict).unset(EAffectFlag::AFF_GROUP);
			vict->removeGroupFlags();
		}
	}
}

void do_ungroup(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	struct FollowerType *f, *next_fol;
	CharData *tch;

	one_argument(argument, buf);

	if (ch->has_master()
		|| !(AFF_FLAGGED(ch, EAffect::kGroup))) {
		SendMsgToChar("Вы же не лидер группы!\r\n", ch);
		return;
	}

	if (!*buf) {
		sprintf(buf2, "Вы исключены из группы %s.\r\n", GET_PAD(ch, 1));
		for (f = ch->followers; f; f = next_fol) {
			next_fol = f->next;
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)) {
				//AFF_FLAGS(f->ch).unset(EAffectFlag::AFF_GROUP);
				f->follower->removeGroupFlags();
				SendMsgToChar(buf2, f->follower);
				if (!AFF_FLAGGED(f->follower, EAffect::kCharmed)
					&& !(f->follower->IsNpc()
						&& AFF_FLAGGED(f->follower, EAffect::kHorse))) {
					stop_follower(f->follower, kSfEmpty);
				}
			}
		}
		AFF_FLAGS(ch).unset(EAffect::kGroup);
		ch->removeGroupFlags();
		SendMsgToChar("Вы распустили группу.\r\n", ch);
		return;
	}
	for (f = ch->followers; f; f = next_fol) {
		next_fol = f->next;
		tch = f->follower;
		if (isname(buf, tch->GetCharAliases())
			&& !AFF_FLAGGED(tch, EAffect::kCharmed)
			&& !IS_HORSE(tch)) {
			//AFF_FLAGS(tch).unset(EAffectFlag::AFF_GROUP);
			tch->removeGroupFlags();
			act("$N более не член вашей группы.", false, ch, nullptr, tch, kToChar);
			act("Вы исключены из группы $n1!", false, ch, nullptr, tch, kToVict);
			act("$N был$G изгнан$A из группы $n1!", false, ch, nullptr, tch, kToNotVict | kToArenaListen);
			stop_follower(tch, kSfEmpty);
			return;
		}
	}
	SendMsgToChar("Этот игрок не входит в состав вашей группы.\r\n", ch);
}

void do_report(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *k;
	struct FollowerType *f;

	if (!AFF_FLAGGED(ch, EAffect::kGroup) && !AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar("И перед кем вы отчитываетесь?\r\n", ch);
		return;
	}
	if (IS_MANA_CASTER(ch)) {
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %d(%d)M\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
				ch->mem_queue.stored, GET_MAX_MANA(ch));
	} else if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		int loyalty = 0;
		for (const auto &aff : ch->affected) {
			if (aff->type == ESpell::kCharm) {
				loyalty = aff->duration / 2;
				break;
			}
		}
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %dL\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
				loyalty);
	} else {
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch));
	}
	CAP(buf);
	k = ch->has_master() ? ch->get_master() : ch;
	for (f = k->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)
			&& f->follower != ch
			&& !AFF_FLAGGED(f->follower, EAffect::kDeafness)) {
			SendMsgToChar(buf, f->follower);
		}
	}

	if (k != ch && !AFF_FLAGGED(k, EAffect::kDeafness)) {
		SendMsgToChar(buf, k);
	}
	SendMsgToChar("Вы доложили о состоянии всем членам вашей группы.\r\n", ch);
}
void do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	do_split(ch, argument, 0, 0, 0);
}

void do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, int currency) {
	int amount, num, share, rest;
	CharData *k;
	struct FollowerType *f;

	if (ch->IsNpc())
		return;

	one_argument(argument, buf);

	EWhat what_currency;

	switch (currency) {
		case currency::ICE : what_currency = EWhat::kIceU;
			break;
		default : what_currency = EWhat::kMoneyU;
			break;
	}

	if (is_number(buf)) {
		amount = atoi(buf);
		if (amount <= 0) {
			SendMsgToChar("И как вы это планируете сделать?\r\n", ch);
			return;
		}

		if (amount > ch->get_gold() && currency == currency::GOLD) {
			SendMsgToChar("И где бы взять вам столько денег?.\r\n", ch);
			return;
		}
		k = ch->has_master() ? ch->get_master() : ch;

		if (AFF_FLAGGED(k, EAffect::kGroup)
			&& (k->in_room == ch->in_room)) {
			num = 1;
		} else {
			num = 0;
		}

		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)
				&& !f->follower->IsNpc()
				&& f->follower->in_room == ch->in_room) {
				num++;
			}
		}

		if (num && AFF_FLAGGED(ch, EAffect::kGroup)) {
			share = amount / num;
			rest = amount % num;
		} else {
			SendMsgToChar("С кем вы хотите разделить это добро?\r\n", ch);
			return;
		}
		//MONEY_HACK

		switch (currency) {
			case currency::ICE : ch->sub_ice_currency(share * (num - 1));
				break;
			case currency::GOLD : ch->remove_gold(share * (num - 1));
				break;
		}

		sprintf(buf, "%s разделил%s %d %s; вам досталось %d.\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch), amount, GetDeclensionInNumber(amount, what_currency), share);
		if (AFF_FLAGGED(k, EAffect::kGroup) && k->in_room == ch->in_room && !k->IsNpc() && k != ch) {
			SendMsgToChar(buf, k);
			switch (currency) {
				case currency::ICE : {
					k->add_ice_currency(share);
					break;
				}
				case currency::GOLD : {
					k->add_gold(share, true, true);
					break;
				}
			}
		}
		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)
				&& !f->follower->IsNpc()
				&& f->follower->in_room == ch->in_room
				&& f->follower != ch) {
				SendMsgToChar(buf, f->follower);
				switch (currency) {
					case currency::ICE : f->follower->add_ice_currency(share);
						break;
					case currency::GOLD : f->follower->add_gold(share, true, true);
						break;
				}
			}
		}
		sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
				amount, GetDeclensionInNumber(amount, what_currency), num, share);
		if (rest) {
			sprintf(buf + strlen(buf),
					"Как истинный еврей вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
					rest, GetDeclensionInNumber(rest, what_currency));
		}

		SendMsgToChar(buf, ch);
		// клан-налог лутера с той части, которая пошла каждому в группе
		if (currency == currency::GOLD) {
			const long clan_tax = ClanSystem::do_gold_tax(ch, share);
			ch->remove_gold(clan_tax);
		}
	} else {
		SendMsgToChar("Сколько и чего вы хотите разделить?\r\n", ch);
		return;
	}
}

ObjData *get_obj_equip_or_carry(CharData *ch, const std::string &text) {
	int eq_num = 0;
	ObjData *obj = get_object_in_equip_vis(ch, text, ch->equipment, &eq_num);
	if (!obj) {
		obj = get_obj_in_list_vis(ch, text, ch->carrying);
	}
	return obj;
}

void do_wimpy(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int wimp_lev;

	// 'wimp_level' is a player_special. -gg 2/25/98
	if (ch->IsNpc())
		return;

	one_argument(argument, arg);

	if (!*arg) {
		if (GET_WIMP_LEV(ch)) {
			sprintf(buf, "Вы попытаетесь бежать при %d ХП.\r\n", GET_WIMP_LEV(ch));
			SendMsgToChar(buf, ch);
			return;
		} else {
			SendMsgToChar("Вы будете драться, драться и драться (пока не помрете, ессно...)\r\n", ch);
			return;
		}
	}
	if (a_isdigit(*arg)) {
		if ((wimp_lev = atoi(arg)) != 0) {
			if (wimp_lev < 0)
				SendMsgToChar("Да, перегрев похоже. С такими хитами вы и так помрете :)\r\n", ch);
			else if (wimp_lev > GET_REAL_MAX_HIT(ch))
				SendMsgToChar("Осталось только дожить до такого количества ХП.\r\n", ch);
			else if (wimp_lev > (GET_REAL_MAX_HIT(ch) / 2))
				SendMsgToChar("Размечтались. Сбечь то можно, но не более половины максимальных ХП.\r\n", ch);
			else {
				sprintf(buf, "Ладушки. Вы сбегите (или сбежите) по достижению %d ХП.\r\n", wimp_lev);
				SendMsgToChar(buf, ch);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		} else {
			SendMsgToChar("Вы будете сражаться до конца (скорее всего своего ;).\r\n", ch);
			GET_WIMP_LEV(ch) = 0;
		}
	} else
		SendMsgToChar("Уточните, при достижении какого количества ХП вы планируете сбежать (0 - драться до смерти)\r\n",
			 ch);
}

void set_display_bits(CharData *ch, bool flag) {
	if (flag) {
		ch->SetFlag(EPrf::kDispHp);
		ch->SetFlag(EPrf::kDispMana);
		ch->SetFlag(EPrf::kDispMove);
		ch->SetFlag(EPrf::kDispExits);
		ch->SetFlag(EPrf::kDispMoney);
		ch->SetFlag(EPrf::kDispLvl);
		ch->SetFlag(EPrf::kDispExp);
		ch->SetFlag(EPrf::kDispFight);
		ch->SetFlag(EPrf::kDispCooldowns);
		if (!IS_IMMORTAL(ch)) {
			ch->SetFlag(EPrf::kDispTimed);
		}
	} else {
		ch->UnsetFlag(EPrf::kDispHp);
		ch->UnsetFlag(EPrf::kDispMana);
		ch->UnsetFlag(EPrf::kDispMove);
		ch->UnsetFlag(EPrf::kDispExits);
		ch->UnsetFlag(EPrf::kDispMoney);
		ch->UnsetFlag(EPrf::kDispLvl);
		ch->UnsetFlag(EPrf::kDispExp);
		ch->UnsetFlag(EPrf::kDispFight);
		ch->UnsetFlag(EPrf::kDispTimed);
		ch->UnsetFlag(EPrf::kDispCooldowns);
	}
}

const char *DISPLAY_HELP =
	"Формат: статус { { Ж | Э | З | В | Д | У | О | Б | П | К } | все | нет }\r\n";

void do_display(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		SendMsgToChar("И зачем это монстру? Не юродствуйте.\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar(DISPLAY_HELP, ch);
		return;
	}

	if (!str_cmp(argument, "on") || !str_cmp(argument, "all") ||
		!str_cmp(argument, "вкл") || !str_cmp(argument, "все")) {
		set_display_bits(ch, true);
	} else if (!str_cmp(argument, "off")
		|| !str_cmp(argument, "none")
		|| !str_cmp(argument, "выкл")
		|| !str_cmp(argument, "нет")) {
		set_display_bits(ch, false);
	} else {
		set_display_bits(ch, false);

		const size_t len = strlen(argument);
		for (size_t i = 0; i < len; i++) {
			switch (LOWER(argument[i])) {
				case 'h':
				case 'ж': ch->SetFlag(EPrf::kDispHp);
					break;
				case 'w':
				case 'з': ch->SetFlag(EPrf::kDispMana);
					break;
				case 'm':
				case 'э': ch->SetFlag(EPrf::kDispMove);
					break;
				case 'e':
				case 'в': ch->SetFlag(EPrf::kDispExits);
					break;
				case 'g':
				case 'д': ch->SetFlag(EPrf::kDispMoney);
					break;
				case 'l':
				case 'у': ch->SetFlag(EPrf::kDispLvl);
					break;
				case 'x':
				case 'о': ch->SetFlag(EPrf::kDispExp);
					break;
				case 'б':
				case 'f': ch->SetFlag(EPrf::kDispFight);
					break;
				case 'п':
				case 't': ch->SetFlag(EPrf::kDispTimed);
					break;
				case 'к':
				case 'c': ch->SetFlag(EPrf::kDispCooldowns);
					break;
				case ' ': break;
				default: SendMsgToChar(DISPLAY_HELP, ch);
					return;
			}
		}
	}

	SendMsgToChar(OK, ch);
}

void do_pray(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int metter = -1;
	ObjData *obj = nullptr;
	struct TimedSkill timed;

	if (ch->IsNpc()) {
		return;
	}

	if (!IS_IMMORTAL(ch)
		&& ((subcmd == SCMD_DONATE
			&& GET_RELIGION(ch) != kReligionPoly)
			|| (subcmd == SCMD_PRAY
				&& GET_RELIGION(ch) != kReligionMono))) {
		SendMsgToChar("Не кощунствуйте!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_DONATE && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kForPoly)) {
		SendMsgToChar("Найдите подходящее место для вашей жертвы.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_PRAY && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kForMono)) {
		SendMsgToChar("Это место не обладает необходимой святостью.\r\n", ch);
		return;
	}

	half_chop(argument, arg, buf);

	if (!*arg || (metter = search_block(arg, pray_whom, false)) < 0) {
		if (subcmd == SCMD_DONATE) {
			SendMsgToChar("Вы можете принести жертву :\r\n", ch);
			for (metter = 0; *(pray_metter[metter]) != '\n'; metter++) {
				if (*(pray_metter[metter]) == '-') {
					SendMsgToChar(pray_metter[metter], ch);
					SendMsgToChar("\r\n", ch);
				}
			}
			SendMsgToChar("Укажите, кому и что вы хотите жертвовать.\r\n", ch);
		} else if (subcmd == SCMD_PRAY) {
			SendMsgToChar("Вы можете вознести молитву :\r\n", ch);
			for (metter = 0; *(pray_metter[metter]) != '\n'; metter++)
				if (*(pray_metter[metter]) == '*') {
					SendMsgToChar(pray_metter[metter], ch);
					SendMsgToChar("\r\n", ch);
				}
			SendMsgToChar("Укажите, кому вы хотите вознести молитву.\r\n", ch);
		}
		return;
	}

	if (subcmd == SCMD_DONATE && *(pray_metter[metter]) != '-') {
		SendMsgToChar("Приносите жертвы только своим Богам.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_PRAY && *(pray_metter[metter]) != '*') {
		SendMsgToChar("Возносите молитвы только своим Богам.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_DONATE) {
		if (!*buf || !(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
			SendMsgToChar("Вы должны пожертвовать что-то стоящее.\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(obj) != EObjType::kFood
			&& GET_OBJ_TYPE(obj) != EObjType::kTreasure) {
			SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
			return;
		}
	} else if (subcmd == SCMD_PRAY) {
		if (ch->get_gold() < 10) {
			SendMsgToChar("У вас не хватит денег на свечку.\r\n", ch);
			return;
		}
	} else
		return;

	if (!IS_IMMORTAL(ch) && (IsTimedBySkill(ch, ESkill::kReligion)
		|| IsAffectedBySpell(ch, ESpell::kReligion))) {
		SendMsgToChar("Вы не можете так часто взывать к Богам.\r\n", ch);
		return;
	}

	timed.skill = ESkill::kReligion;
	timed.time = 12;
	ImposeTimedSkill(ch, &timed);

	for (const auto &i : pray_affect) {
		if (i.metter == metter) {
			Affect<EApply> af;
			af.type = ESpell::kReligion;
			af.duration = CalcDuration(ch, 12, 0, 0, 0, 0);
			af.modifier = i.modifier;
			af.location = i.location;
			af.bitvector = i.bitvector;
			af.battleflag = i.battleflag;
			ImposeAffect(ch, af, false, false, false, false);
		}
	}

	if (subcmd == SCMD_PRAY) {
		sprintf(buf, "$n затеплил$g свечку и вознес$q молитву %s.", pray_whom[metter]);
		act(buf, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		sprintf(buf, "Вы затеплили свечку и вознесли молитву %s.", pray_whom[metter]);
		act(buf, false, ch, nullptr, nullptr, kToChar);
		ch->remove_gold(10);
	} else if (subcmd == SCMD_DONATE && obj) {
		sprintf(buf, "$n принес$q $o3 в жертву %s.", pray_whom[metter]);
		act(buf, false, ch, obj, nullptr, kToRoom | kToArenaListen);
		sprintf(buf, "Вы принесли $o3 в жертву %s.", pray_whom[metter]);
		act(buf, false, ch, obj, nullptr, kToChar);
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}

void do_recall(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		SendMsgToChar("Монстрам некуда возвращаться!\r\n", ch);
		return;
	}

	const int rent_room = GetRoomRnum(GET_LOADROOM(ch));
	if (rent_room == kNowhere || ch->in_room == kNowhere) {
		SendMsgToChar("Вам некуда возвращаться!\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch)
		&& (SECT(ch->in_room) == ESector::kSecret
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kDeathTrap)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kSlowDeathTrap)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kTunnel)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoRelocateIn)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTeleportIn)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kIceTrap)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kGodsRoom)
			|| !Clan::MayEnter(ch, ch->in_room, kHousePortal)
			|| !Clan::MayEnter(ch, rent_room, kHousePortal))) {
		SendMsgToChar("У вас не получилось вернуться!\r\n", ch);
		return;
	}

	SendMsgToChar("Вам очень захотелось оказаться подальше от этого места!\r\n", ch);
	if (IS_GOD(ch) || Noob::is_noob(ch)) {
		if (ch->in_room != rent_room) {
			SendMsgToChar("Вы почувствовали, как чья-то огромная рука подхватила вас и куда-то унесла!\r\n", ch);
			act("$n поднял$a глаза к небу и внезапно исчез$q!", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, rent_room);
			look_at_room(ch, 0);
			act("$n внезапно появил$u в центре комнаты!", true, ch, nullptr, nullptr, kToRoom);
		} else {
			SendMsgToChar("Но тут и так достаточно мирно...\r\n", ch);
		}
	} else {
		SendMsgToChar("Ну и хотите себе на здоровье...\r\n", ch);
	}
}

void perform_beep(CharData *ch, CharData *vict) {
	SendMsgToChar(CCRED(vict, C_NRM), vict);
	sprintf(buf, "\007\007 $n вызывает вас!");
	act(buf, false, ch, nullptr, vict, kToVict | kToSleep);
	SendMsgToChar(CCNRM(vict, C_NRM), vict);

	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar(OK, ch);
	else {
		SendMsgToChar(CCRED(ch, C_CMP), ch);
		sprintf(buf, "Вы вызвали $N3.");
		act(buf, false, ch, nullptr, vict, kToChar | kToSleep);
		SendMsgToChar(CCNRM(ch, C_CMP), ch);
	}
}

void do_beep(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;

	one_argument(argument, buf);

	if (!*buf)
		SendMsgToChar("Кого вызывать?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, EFind::kCharInWorld)) || vict->IsNpc())
		SendMsgToChar(NOPERSON, ch);
	else if (ch == vict)
		SendMsgToChar("\007\007Вы вызвали себя!\r\n", ch);
	else if (ch->IsFlagged(EPrf::kNoTell))
		SendMsgToChar("Вы не можете пищать в режиме обращения.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof))
		SendMsgToChar("Стены заглушили ваш писк.\r\n", ch);
	else if (!vict->IsNpc() && !vict->desc)    // linkless
		act("$N потерял связь.", false, ch, nullptr, vict, kToChar | kToSleep);
	else if (vict->IsFlagged(EPlrFlag::kWriting))
		act("$N пишет сейчас; Попищите позже.", false, ch, nullptr, vict, kToChar | kToSleep);
	else
		perform_beep(ch, vict);
}

extern struct IndexData *obj_index;


bool is_dark(RoomRnum room) {
	double coef = 0.0;

	// если на комнате висит флаг всегда светло, то добавляем
	// +2 к коэф
	if (room_spells::IsRoomAffected(world[room], ESpell::kLight))
		coef += 2.0;
	// если светит луна и комната !помещение и !город
	if ((SECT(room) != ESector::kInside) && (SECT(room) != ESector::kCity)
		&& (GET_ROOM_SKY(room) == kSkyLightning
			&& weather_info.moon_day >= kFullMoonStart
			&& weather_info.moon_day <= kFullMoonStop))
		coef += 1.0;

	// если ночь и мы не внутри и не в городе
	if ((SECT(room) != ESector::kInside) && (SECT(room) != ESector::kCity)
		&& ((weather_info.sunlight == kSunSet) || (weather_info.sunlight == kSunDark)))
		coef -= 1.0;
	// если на комнате флаг темно
	if (ROOM_FLAGGED(room, ERoomFlag::kDarked))
		coef -= 1.0;

	if (ROOM_FLAGGED(room, ERoomFlag::kAlwaysLit))
		coef += 200.0;

	// проверка на костер
	if (world[room]->fires)
		coef += 1.0;

	// проходим по всем чарам и смотрим на них аффекты тьма/свет/освещение

	for (const auto tmp_ch : world[room]->people) {
		// если на чаре есть освещение, например, шарик или лампа
		if (IsWearingLight(tmp_ch))
			coef += 1.0;
		// если на чаре аффект свет
		if (AFF_FLAGGED(tmp_ch, EAffect::kSingleLight))
			coef += 3.0;
		// освещение перекрывает 1 тьму!
		if (AFF_FLAGGED(tmp_ch, EAffect::kHolyLight))
			coef += 9.0;
		// Санка. Логично, что если чар светится ярким сиянием, то это сияние распространяется на комнату
		if (AFF_FLAGGED(tmp_ch, EAffect::kSanctuary))
			coef += 1.0;
		// Тьма. Сразу фигачим коэф 6.
		if (AFF_FLAGGED(tmp_ch, EAffect::kHolyDark))
			coef -= 6.0;
	}

	if (coef < 0) {
		return true;
	}
	return false;

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
