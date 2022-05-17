#include "entities/char_data.h"
#include "handler.h"
#include "house.h"
#include "utils/utils_char_obj.inl"

void RemoveEquipment(CharData *ch, int pos) {
	ObjData *obj;

	if (!(obj = GET_EQ(ch, pos))) {
		log("SYSERR: RemoveEquipment: bad pos %d passed.", pos);
	} else {
		/*
			   if (IS_OBJ_STAT(obj, ITEM_NODROP))
			   act("Вы не можете снять $o3!", false, follower, obj, 0, TO_CHAR);
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
