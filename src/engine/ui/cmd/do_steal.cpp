/**
\file do_steal.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/handler.h"
#include "gameplay/skills/skills.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/fight/fight_constants.h"
#include "gameplay/fight/fight_hit.h"
#include "do_get.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/ai/spec_procs.h"

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

	// NO, NO With Imp's and Shopkeepers, and if player thieving is not allowed
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
