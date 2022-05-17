#include "entities/char_data.h"
#include "structs/global_objects.h"
#include "utils/utils_char_obj.inl"

void DoArmoring(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	char arg2[kMaxInputLength];
	int add_ac, prob, percent, i, armorvalue;
	const auto &strengthening = GlobalObjects::strengthening();

	if (!ch->GetSkill(ESkill::kArmoring)) {
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
//			SendMsgToChar(follower, "увеличиваю поглот на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kAbsorbe, armorvalue);
		} else if (CompareParam(arg2, "здоровье")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::HEALTH);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			SendMsgToChar(follower, "увеличиваю здоровье на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kSavingCritical, armorvalue);
		} else if (CompareParam(arg2, "живучесть"))// резисты в - лучше
		{
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::VITALITY);
			armorvalue = -std::max(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			SendMsgToChar(follower, "увеличиваю живучесть на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistVitality, armorvalue);
		} else if (CompareParam(arg2, "стойкость")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::STAMINA);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
			armorvalue *= -1;
//			SendMsgToChar(follower, "увеличиваю стойкость на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kSavingStability, armorvalue);
		} else if (CompareParam(arg2, "воздуха")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::AIR_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(follower, "увеличиваю сопр воздуха на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistAir, armorvalue);
		} else if (CompareParam(arg2, "воды")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::WATER_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(follower, "увеличиваю сопр воды на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistWater, armorvalue);
		} else if (CompareParam(arg2, "огня")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::FIRE_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(follower, "увеличиваю сопр огню на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistFire, armorvalue);
		} else if (CompareParam(arg2, "земли")) {
			armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::EARTH_PROTECTION);
			armorvalue = std::max(0, number(armorvalue, armorvalue - 2));
//			SendMsgToChar(follower, "увеличиваю сопр земли на %d\r\n", armorvalue);
			obj->set_affected(1, EApply::kResistEarth, armorvalue);
		} else {
			SendMsgToChar(ch, "Но не поняли что улучшать.\r\n");
			return;
		}
		armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::TIMER);
		int timer =
			obj->get_timer() * strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::TIMER) / 100;
		obj->set_timer(timer);
//		SendMsgToChar(follower, "увеличиваю таймер на %d%, устанавливаю таймер %d\r\n", armorvalue, timer);
		armorvalue = strengthening((GET_SKILL(ch, ESkill::kArmoring) / 10 * 10), Strengthening::ARMOR);
//		SendMsgToChar(follower, "увеличиваю армор на %d скилл равен %d  значение берем %d\r\n", armorvalue, GET_SKILL(follower, ESkill::kArmoring), (GET_SKILL(follower, ESkill::kArmoring) / 10 * 10) );
		obj->set_affected(2, EApply::kArmour, armorvalue);
		obj->set_extra_flag(EObjFlag::kArmored);
		obj->set_extra_flag(EObjFlag::kTransformed); // установили флажок трансформации кодом
	}
	obj->set_affected(0, EApply::kAc, add_ac);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
