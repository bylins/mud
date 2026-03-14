/**
\file equip.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Команда "надать/экипировать".
\detail Отсюда нужно вынести в механику собственно механику надевания, поиск слотов, требования к силе и все такие.
*/

#include "engine/ui/cmd/do_equip.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/core/utils_char_obj.inl"

bool unique_stuff(const CharData *ch, const ObjData *obj) {
	for (unsigned int i = 0; i < EEquipPos::kNumEquipPos; i++)
		if (GET_EQ(ch, i) && (GET_OBJ_VNUM(GET_EQ(ch, i)) == GET_OBJ_VNUM(obj))) {
			return true;
		}
	return false;
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
		if (ch->GetEnemy() && (equip_pos == EEquipPos::kShield || obj->get_type() == EObjType::kWeapon)) {
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
	if ((equip_pos == EEquipPos::kQuiver && !(GET_EQ(ch, EEquipPos::kBoths) // не может одеть колчан если одет не лук
				&& (GET_EQ(ch, EEquipPos::kBoths)->get_type() == EObjType::kWeapon)
				&& (static_cast<ESkill>(GET_EQ(ch, EEquipPos::kBoths)->get_spec_param()) == ESkill::kBows)))) {
		SendMsgToChar("А стрелять чем будете?\r\n", ch);
		return;
	}
	// нельзя надеть щит, если недостаточно силы
	if (!IS_IMMORTAL(ch) && (equip_pos == EEquipPos::kShield) && !CanBeWearedAsShield(ch, obj)) {
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

void do_wear(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	ObjData *obj, *next_obj;
	int equip_pos, dotmode, items_worn = 0;

	two_arguments(argument, arg1, arg2);

	if (ch->IsNpc()
		&& AFF_FLAGGED(ch, EAffect::kCharmed)
		&& (!NPC_FLAGGED(ch, ENpcFlag::kArmoring)
			|| ch->IsFlagged(EMobFlag::kResurrected))) {
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
			ch->GetPosition() > EPosition::kSleep; obj = next_obj) {
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
			while (obj && !AFF_FLAGGED(ch, EAffect::kHold) && ch->GetPosition() > EPosition::kSleep) {
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
		&& (!NPC_FLAGGED(ch, ENpcFlag::kWielding) || ch->IsFlagged(EMobFlag::kResurrected))))
		return;

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
		} else if (obj->get_type() != EObjType::kWeapon) {
			SendMsgToChar("Это не оружие.\r\n", ch);
		} else if (ch->IsNpc()
			&& AFF_FLAGGED(ch, EAffect::kCharmed)
			&& ch->IsFlagged(EMobFlag::kCorpse)) {
			SendMsgToChar("Ожившие трупы не могут вооружаться.\r\n", ch);
		} else {
			one_argument(argument, arg);
			if (!str_cmp(arg, "обе")
				&& CAN_WEAR(obj, EWearFlag::kBoth)) {
				// иногда бывает надо
				if (!IS_IMMORTAL(ch) && !CanBeTakenInBothHands(ch, obj)) {
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

			if (wear == EEquipPos::kWield && !IS_IMMORTAL(ch) && !CanBeTakenInMajorHand(ch, obj)) {
				act("Вам слишком тяжело держать $o3 в правой руке.",
					false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_WIELD_W);

				if (CAN_WEAR(obj, EWearFlag::kBoth)) {
					wear = EEquipPos::kBoths;
				} else {
					return;
				}
			}

			if (wear == EEquipPos::kBoths && !IS_IMMORTAL(ch) && !CanBeTakenInBothHands(ch, obj)) {
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

	if (!*arg)
		SendMsgToChar("Вы заорали : 'Держи его!!! Хватай его!!!'\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
	} else {
		if (obj->get_type() == EObjType::kLightSource) {
			perform_wear(ch, obj, EEquipPos::kLight);
		} else {
			if (!CAN_WEAR(obj, EWearFlag::kHold)
				&& obj->get_type() != EObjType::kWand
				&& obj->get_type() != EObjType::kStaff
				&& obj->get_type() != EObjType::kScroll
				&& obj->get_type() != EObjType::kPotion) {
				SendMsgToChar("Вы не можете это держать.\r\n", ch);
				return;
			}

			if (obj->get_type() == EObjType::kWeapon) {
				if (static_cast<ESkill>(obj->get_spec_param()) == ESkill::kTwohands
					|| static_cast<ESkill>(obj->get_spec_param()) == ESkill::kBows) {
					SendMsgToChar("Данный тип оружия держать невозможно.", ch);
					return;
				}
			}

			if (ch->IsNpc()
				&& AFF_FLAGGED(ch, EAffect::kCharmed)
				&& ch->IsFlagged(EMobFlag::kCorpse)) {
				SendMsgToChar("Ожившие трупы не могут вооружаться.\r\n", ch);
				return;
			}
			if (!IS_IMMORTAL(ch)
				&& !CanBeTakenInMinorHand(ch, obj)) {
				act("Вам слишком тяжело держать $o3 в левой руке.",
					false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_HOLD_W);

				if (CAN_WEAR(obj, EWearFlag::kBoth)) {
					if (!CanBeTakenInBothHands(ch, obj)) {
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

void message_str_need(CharData *ch, ObjData *obj, int type) {
	if (ch->GetPosition() == EPosition::kDead)
		return;
	int need_str = 0;
	switch (type) {
		case STR_WIELD_W: need_str = calc_str_req(obj->get_weight(), STR_WIELD_W);
			break;
		case STR_HOLD_W: need_str = calc_str_req(obj->get_weight(), STR_HOLD_W);
			break;
		case STR_BOTH_W: need_str = calc_str_req(obj->get_weight(), STR_BOTH_W);
			break;
		case STR_SHIELD_W: need_str = calc_str_req((obj->get_weight() + 1) / 2, STR_HOLD_W);
			break;
		default:
			log("SYSERROR: ch=%s, weight=%d, type=%d (%s %s %d)",
				GET_NAME(ch), obj->get_weight(), type,
				__FILE__, __func__, __LINE__);
			return;
	}
	SendMsgToChar(ch, "Для этого требуется %d %s.\r\n",
				  need_str, GetDeclensionInNumber(need_str, EWhat::kStr));
}

bool CanBeTakenInBothHands(CharData *ch, ObjData *obj) {
	return (obj->get_weight() <= str_bonus(GetRealStr(ch), STR_WIELD_W) + str_bonus(GetRealStr(ch), STR_HOLD_W));
}

bool CanBeTakenInMajorHand(CharData *ch, ObjData *obj) {
	return (obj->get_weight() <= str_bonus(GetRealStr(ch), STR_WIELD_W));
}

bool CanBeTakenInMinorHand(CharData *ch, ObjData *obj) {
	return (obj->get_weight() <= str_bonus(GetRealStr(ch), STR_HOLD_W));
}

bool CanBeWearedAsShield(CharData *ch, ObjData *obj) {
	return (obj->get_weight() <= (2 * str_bonus(GetRealStr(ch), STR_HOLD_W)));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
