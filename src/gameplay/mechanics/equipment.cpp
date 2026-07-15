/**
\file equipment.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "equipment.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "utils/random.h"
#include "engine/core/char_equip_flags.h"
#include "engine/core/obj_handler.h"
#include "gameplay/mechanics/illumination.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/db/world_objects.h"
#include "gameplay/core/constants.h"
#include "gameplay/mechanics/obj_sets.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/magic/magic.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/fight/common.h"
#include "gameplay/mechanics/named_stuff.h"
#include "administration/privilege.h"
#include "gameplay/core/remort.h"
#include "utils/grammar/declensions.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/magic/magic_utils.h"

// issue.handler-cleaning: invalid_unique has no header decl (was fwd-declared in handler.cpp)
int invalid_unique(CharData *ch, const ObjData *obj);

void DamageEquipment(CharData *ch, int pos, int dam, int chance) {
	// calculate drop_chance if
	if (pos == kNowhere) {
		pos = number(0, 100);
		if (pos < 3)
			pos = EEquipPos::kFingerR + number(0, 1);
		else if (pos < 6)
			pos = EEquipPos::kNeck + number(0, 1);
		else if (pos < 20)
			pos = EEquipPos::kBody;
		else if (pos < 30)
			pos = EEquipPos::kHead;
		else if (pos < 45)
			pos = EEquipPos::kLegs;
		else if (pos < 50)
			pos = EEquipPos::kFeet;
		else if (pos < 58)
			pos = EEquipPos::kHands;
		else if (pos < 66)
			pos = EEquipPos::kArms;
		else if (pos < 76)
			pos = EEquipPos::kShield;
		else if (pos < 86)
			pos = EEquipPos::kShoulders;
		else if (pos < 90)
			pos = EEquipPos::kWaist;
		else if (pos < 94)
			pos = EEquipPos::kWristR + number(0, 1);
		else
			pos = EEquipPos::kHold;
	}

	if (pos <= 0 || pos > EEquipPos::kBoths || !GET_EQ(ch, pos) || dam < 0 || AFF_FLAGGED(ch, EAffect::kGodsShield)) {
		return;
	}
	DamageObj(GET_EQ(ch, pos), dam, chance);
}

// * Alterate equipment
// По-хорошему, нужна отдельная механика прочности предметов и эту функцию нужно перенести туда
void DamageObj(ObjData *obj, int dam, int chance) {
	if (!obj)
		return;
	dam = number(0, dam * (material_value[obj->get_material()] + 30) /
		std::max(1, obj->get_maximum_durability() *
			(obj->has_flag(EObjFlag::kNodrop) ? 5 :
			 obj->has_flag(EObjFlag::kBless) ? 15 : 10)
			* (static_cast<ESkill>(obj->get_spec_param()) == ESkill::kBows ? 3 : 1)));

	if (dam > 0 && chance >= number(1, 100)) {
		if (dam > 1 && obj->get_worn_by() && GET_EQ(obj->get_worn_by(), EEquipPos::kShield) == obj) {
			dam /= 2;
		}

		obj->sub_current(dam);
		if (obj->get_current_durability() <= 0) {
			if (obj->get_worn_by()) {
				snprintf(buf, kMaxStringLength, "$o%s рассыпал$U, не выдержав повреждений.",
						 char_get_custom_label(obj, obj->get_worn_by()).c_str());
				act(buf, false, obj->get_worn_by(), obj, nullptr, kToChar);
			} else if (obj->get_carried_by()) {
				snprintf(buf, kMaxStringLength, "$o%s рассыпал$U, не выдержав повреждений.",
						 char_get_custom_label(obj, obj->get_carried_by()).c_str());
				act(buf, false, obj->get_carried_by(), obj, nullptr, kToChar);
			}
			// issue #3563: логируем пропажу до отложенного удаления, пока владелец известен.
			LogPlayerObjLoss(obj, "рассыпалась по прочности");
			// issue.obj-casting: deferred extraction (freed at heartbeat end) so a live ctx.ovict
			// (acid corrode etc.) is left flagged purged() rather than dangling.
			world_objects.AddToExtractedList(obj);
		}
	}
}


// issue.handler-cleaning (Bucket 1): equip/unequip machinery moved from handler.cpp.
// ActivateStuff/DeactivateStuff are slated to move to obj_sets in Bucket 3.



void DisplayWearMsg(CharData *ch, ObjData *obj, int position) {
	const char *wear_messages[][2] =
		{
			{"$n засветил$g $o3 и взял$g во вторую руку.",
			 "Вы зажгли $o3 и взяли во вторую руку."},

			{"$n0 надел$g $o3 на правый указательный палец.",
			 "Вы надели $o3 на правый указательный палец."},

			{"$n0 надел$g $o3 на левый указательный палец.",
			 "Вы надели $o3 на левый указательный палец."},

			{"$n0 надел$g $o3 вокруг шеи.",
			 "Вы надели $o3 вокруг шеи."},

			{"$n0 надел$g $o3 на грудь.",
			 "Вы надели $o3 на грудь."},

			{"$n0 надел$g $o3 на туловище.",
			 "Вы надели $o3 на туловище.",},

			{"$n0 водрузил$g $o3 на голову.",
			 "Вы водрузили $o3 себе на голову."},

			{"$n0 надел$g $o3 на ноги.",
			 "Вы надели $o3 на ноги."},

			{"$n0 обул$g $o3.",
			 "Вы обули $o3."},

			{"$n0 надел$g $o3 на кисти.",
			 "Вы надели $o3 на кисти."},

			{"$n0 надел$g $o3 на руки.",
			 "Вы надели $o3 на руки."},

			{"$n0 начал$g использовать $o3 как щит.",
			 "Вы начали использовать $o3 как щит."},

			{"$n0 облачил$u в $o3.",
			 "Вы облачились в $o3."},

			{"$n0 надел$g $o3 вокруг пояса.",
			 "Вы надели $o3 вокруг пояса."},

			{"$n0 надел$g $o3 вокруг правого запястья.",
			 "Вы надели $o3 вокруг правого запястья."},

			{"$n0 надел$g $o3 вокруг левого запястья.",
			 "Вы надели $o3 вокруг левого запястья."},

			{"$n0 взял$g в правую руку $o3.",
			 "Вы вооружились $o4."},

			{"$n0 взял$g $o3 в левую руку.",
			 "Вы взяли $o3 в левую руку."},

			{"$n0 взял$g $o3 в обе руки.",
			 "Вы взяли $o3 в обе руки."},

			{"$n0 начал$g использовать $o3 как колчан.",
			 "Вы начали использовать $o3 как колчан."}
		};

	act(wear_messages[position][1], false, ch, obj, nullptr, kToChar);
	act(wear_messages[position][0],
		ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed) ? false : true,
		ch, obj, nullptr, kToRoom | kToArenaListen);
}



void EquipObj(CharData *ch, ObjData *obj, int pos, const CharEquipFlags& equip_flags) {
	int was_lgt = AFF_FLAGGED(ch, EAffect::kSingleLight) ? kLightYes : kLightNo,
		was_hlgt = AFF_FLAGGED(ch, EAffect::kHolyLight) ? kLightYes : kLightNo,
		was_hdrk = AFF_FLAGGED(ch, EAffect::kHolyDark) ? kLightYes : kLightNo,
		was_lamp = false;

	const bool no_cast = equip_flags.test(CharEquipFlag::no_cast);
	const bool skip_total = equip_flags.test(CharEquipFlag::skip_total);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);

	if (pos < 0 || pos >= EEquipPos::kNumEquipPos) {
		log("SYSERR: equip_char(%s,%d) in unknown pos...", GET_NAME(ch), pos);
		return;
	}

	if (GET_EQ(ch, pos)) {
		log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->get_short_description().c_str());
		return;
	}
	//if (obj->carried_by) {
	//	log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj, ch, ECase::kNom));
	//	return;
	//}
	if (obj->get_in_room() != kNowhere) {
		log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj, ch, grammar::ECase::kNom));
		return;
	}

	if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj)) {
		act("Вас обожгло при попытке использовать $o3.", false, ch, obj, nullptr, kToChar);
		act("$n попытал$u использовать $o3 - и чудом не обгорел$g.", false, ch, obj, nullptr, kToRoom);
		if (obj->get_carried_by()) {
			RemoveObjFromChar(obj);
		}
		PlaceObjToRoom(obj, ch->in_room);
		CheckObjDecay(obj);
		return;
	} else if ((!ch->IsNpc() || IsCharmice(ch)) && obj->has_flag(EObjFlag::kNamed)
		&& NamedStuff::check_named(ch, obj, true)) {
		if (!NamedStuff::wear_msg(ch, obj))
			SendMsgToChar("Просьба не трогать! Частная собственность!\r\n", ch);
		if (!obj->get_carried_by()) {
			PlaceObjToInventory(obj, ch);
		}
		return;
	}

	if ((!ch->IsNpc() && HaveIncompatibleAlign(ch, obj)) || invalid_no_class(ch, obj) ||
		(AFF_FLAGGED(ch, EAffect::kCharmed) && (obj->has_flag(EObjFlag::kSharpen) ||
		obj->has_flag(EObjFlag::kArmored)))) {
		act("$o0 явно не предназначен$A для вас.", false, ch, obj, nullptr, kToChar);
		act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", false, ch, obj, nullptr, kToRoom);
		if (!obj->get_carried_by()) {
			PlaceObjToInventory(obj, ch);
		}
		return;
	}

	if (!ch->IsNpc() || IsCharmice(ch)) {
		CharData *master = IsCharmice(ch) && ch->has_master() ? ch->get_master() : ch;
		if ((obj->get_auto_mort_req() >= 0) && (obj->get_auto_mort_req() > remort::GetRealRemort(master))
			&& !privilege::IsImmortal(master)) {
			SendMsgToChar(master, "Для использования %s требуется %d %s.\r\n",
						  obj->get_PName(grammar::ECase::kGen).c_str(),
						  obj->get_auto_mort_req(),
						  grammar::GetDeclensionInNumber(obj->get_auto_mort_req(), grammar::EWhat::kRemort));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", false, ch, obj, nullptr, kToRoom);
			if (!obj->get_carried_by()) {
				PlaceObjToInventory(obj, ch);
			}
			return;
		} else if ((obj->get_auto_mort_req() < -1) && (abs(obj->get_auto_mort_req()) < remort::GetRealRemort(master))
			&& !privilege::IsImmortal(master)) {
			SendMsgToChar(master, "Максимально количество перевоплощений для использования %s равно %d.\r\n",
						  obj->get_PName(grammar::ECase::kGen).c_str(),
						  abs(obj->get_auto_mort_req()));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
				false, ch, obj, nullptr, kToRoom);
			if (!obj->get_carried_by()) {
				PlaceObjToInventory(obj, ch);
			}
			return;
		}

	}

	if (obj->get_carried_by()) {
		RemoveObjFromChar(obj);
	}

	was_lamp = IsWearingLight(ch);
	GET_EQ(ch, pos) = obj;
	obj->set_worn_by(ch);
	obj->set_worn_on(pos);
	obj->set_next_content(nullptr);
	ch->check_aggressive = true;

	if (show_msg) {
		DisplayWearMsg(ch, obj, pos);
		if (obj->has_flag(EObjFlag::kNamed)) {
			NamedStuff::wear_msg(ch, obj);
		}
	}

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::kSetItem)) {
		for (; it != ObjData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				obj_sets::ActivateStuff(ch, obj, it, 0,
							  (show_msg ? CharEquipFlag::show_msg : CharEquipFlags())
								  | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()), 0);
				break;
			}
		}
	}

	if (!obj->has_flag(EObjFlag::kSetItem) || it == ObjData::set_table.end()) {
		for (int j = 0; j < kMaxObjAffect; j++) {
			affect_modify(ch,
						  obj->get_affected(j).location,
						  obj->get_affected(j).modifier,
						  static_cast<EAffect>(0),
						  true);
		}

		if (ch->in_room != kNowhere) {
			for (const auto &j : weapon_affect) {
				if (j.aff_spell == ESpell::kUndefined || !obj->GetEWeaponAffect(j.aff_pos)) {
					continue;
				}
				if (!no_cast) {
					if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic)) {
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							false, ch, obj, nullptr, kToRoom);
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							false, ch, obj, nullptr, kToChar);
					} else {
						CastWeaponAffect(ch, j.aff_spell);
					}
				} else {
					affect_modify(ch, GetApplyByWeaponAffect(j.aff_pos, ch).first,
								  GetApplyByWeaponAffect(j.aff_pos, ch).second,
								  static_cast<EAffect>(j.aff_bitvector), true);
				}
			}
		}
	}

	if (!skip_total) {
		if (obj_sets::is_set_item(obj)) {
			ch->obj_bonus().update(ch);
		}
		affect_total(ch);
		CheckLight(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	// Раз показываем сообщение, значит, предмет надевает сам персонаж
	// А вообще эта порнография из-за того, что одна функция используется с кучей флагов в разных вариантах
	if (show_msg && ch->GetEnemy() && (obj->get_type() == EObjType::kWeapon || pos == kShield)) {
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	}
}

ObjData *UnequipChar(CharData *ch, int pos, const CharEquipFlags& equip_flags) {
	int was_lgt = AFF_FLAGGED(ch, EAffect::kSingleLight) ? kLightYes : kLightNo,
		was_hlgt = AFF_FLAGGED(ch, EAffect::kHolyLight) ? kLightYes : kLightNo,
		was_hdrk = AFF_FLAGGED(ch, EAffect::kHolyDark) ? kLightYes : kLightNo, was_lamp = false;

	const bool skip_total = equip_flags.test(CharEquipFlag::skip_total);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);

	if (pos < 0 || pos >= EEquipPos::kNumEquipPos) {
		log("SYSERR: UnequipChar(%s,%d) - unused pos...", GET_NAME(ch), pos);
		return nullptr;
	}

	ObjData *obj = GET_EQ(ch, pos);
	if (nullptr == obj) {
		log("SYSERR: UnequipChar(%s,%d) - no equip...", GET_NAME(ch), pos);
		return nullptr;
	}

	was_lamp = IsWearingLight(ch);



	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::kSetItem))
		for (; it != ObjData::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				obj_sets::DeactivateStuff(ch, obj, it, 0, (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()), 0);
				break;
			}

	if (!obj->has_flag(EObjFlag::kSetItem) || it == ObjData::set_table.end()) {
		for (int j = 0; j < kMaxObjAffect; j++) {
			affect_modify(ch,
						  obj->get_affected(j).location,
						  obj->get_affected(j).modifier,
						  static_cast<EAffect>(0),
						  false);
		}

		if (ch->in_room != kNowhere) {
			for (const auto &j : weapon_affect) {
				if (j.aff_bitvector == 0 || !obj->GetEWeaponAffect(j.aff_pos)) {
					continue;
				}
				if (ch->IsNpc()
					&& AFF_FLAGGED(&mob_proto[ch->get_rnum()], static_cast<EAffect>(j.aff_bitvector))) {
					continue;
				}
				affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(j.aff_bitvector), false);
			}
		}

		if ((obj->has_flag(EObjFlag::kSetItem)) && (SetSystem::is_big_set(obj)))
			obj->deactivate_obj(activation());
	}

	GET_EQ(ch, pos) = nullptr;
	obj->set_worn_by(nullptr);
	obj->set_worn_on(kNowhere);
	obj->set_next_content(nullptr);

	if (!skip_total) {
		if (obj_sets::is_set_item(obj)) {
			if (obj->get_activator().first) {
				obj_sets::print_off_msg(ch, obj);
			}
			ch->obj_bonus().update(ch);
		}
		obj->set_activator(false, 0);
		obj->remove_set_bonus();

		affect_total(ch);
		CheckLight(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	return (obj);
}



// issue.handler-cleaning: equip restriction/state queries (moved from handler).
bool HaveIncompatibleAlign(CharData *ch, ObjData *obj) {
	if (ch->IsNpc() || privilege::IsImmortal(ch)) {
		return false;
	}
	if (obj->has_anti_flag(EAntiFlag::kMono) && GET_RELIGION(ch) == kReligionMono) {
		return true;
	}
	if (obj->has_anti_flag(EAntiFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly) {
		return true;
	}
	return false;
}

int IsEquipInMetall(CharData *ch) {
	int i, wgt = 0;

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		return (false);
	if (privilege::IsGod(ch))
		return (false);

	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)
			&& ObjSystem::is_armor_type(GET_EQ(ch, i))
			&& GET_EQ(ch, i)->get_material() <= EObjMaterial::kPreciousMetel) {
			wgt += GET_EQ(ch, i)->get_weight();
		}
	}

	if (wgt > GetRealStr(ch))
		return (true);

	return (false);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
