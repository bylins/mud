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

#include "engine/core/utils_char_obj.inl"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/entities/entities_constants.h"
#include "engine/ui/cmd/follow.h"
#include "engine/ui/cmd/do_features.h"
#include "engine/core/comm.h"
#include "engine/core/conf.h"
#include "gameplay/fight/fight.h"
#include "gameplay/core/constants.h"
#include "engine/db/db.h"
#include "gameplay/mechanics/depot.h"
#include "engine/scripting/dg_scripts.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/pk.h"
#include "gameplay/mechanics/mem_queue.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "engine/ui/interpreter.h"
#include "utils/logger.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_rooms.h"
#include "engine/network/msdp/msdp_constants.h"
#include "gameplay/mechanics/noob.h"
#include "engine/entities/obj_data.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/obj_save.h"
#include "administration/privilege.h"
#include "utils/random.h"
#include "gameplay/communication/remember.h"
#include "engine/entities/room_data.h"
#include "engine/ui/color.h"
#include "gameplay/economics/shop_ext.h"
#include "gameplay/skills/skills.h"
#include "gameplay/magic/spells.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/db/world_objects.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/global_objects.h"
#include "engine/ui/table_wrapper.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/affects/affect_data.h"

// extern variables
extern int nameserver_is_slow;
//void appear(CharacterData *ch);
void write_aliases(CharData *ch);
void perform_immort_vis(CharData *ch);
void do_gen_comm(CharData *ch, char *argument, int cmd, int subcmd);
extern void split_or_clan_tax(CharData *ch, long amount);
extern bool IsWearingLight(CharData *ch);
// local functions
void do_antigods(CharData *ch, char *argument, int cmd, int subcmd);
void do_quit(CharData *ch, char *argument, int /* cmd */, int subcmd);
void do_save(CharData *ch, char *argument, int cmd, int subcmd);
void do_not_here(CharData *ch, char *argument, int cmd, int subcmd);
void do_sneak(CharData *ch, char *argument, int cmd, int subcmd);
void do_visible(CharData *ch, char *argument, int cmd, int subcmd);
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
		mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
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
			if (d->character && (GET_UID(d->character) == GET_UID(ch)))
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
	SendMsgToChar(kColorRed, vict);
	sprintf(buf, "\007\007 $n вызывает вас!");
	act(buf, false, ch, nullptr, vict, kToVict | kToSleep);
	SendMsgToChar(kColorNrm, vict);

	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar(OK, ch);
	else {
		SendMsgToChar(kColorRed, ch);
		sprintf(buf, "Вы вызвали $N3.");
		act(buf, false, ch, nullptr, vict, kToChar | kToSleep);
		SendMsgToChar(kColorNrm, ch);
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
