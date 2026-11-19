/**
 \file alter_enchant_weapon.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterEnchantWeapon alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/magic_internal.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/mechanics/equipment.h"

namespace handlers {

// issue #4173: знак зачарования не ингредиент, а предмет глобального дропа (kOther с внумом
// 1930/1931/1932), поэтому через <components><material> он не ищется и трату знака держим
// здесь, где знак и берется -- из руки. Заряд в val(2): списываем один, при исчерпании знак
// уничтожается. Отвязка от слота перед удалением -- как в ConsumeMatComponent: освобождение
// отложенное (AddToExtractedList), чтобы предмет не висел битой ссылкой до конца каста.
static void ConsumeEnchantSign(CharData *ch, ObjData *sign, ESpell spell_id) {
	const std::string &use = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kComponentUse);
	if (!use.empty()) {
		act(use.c_str(), false, ch, sign, nullptr, kToChar);
	}
	sign->dec_val(2);
	if (GET_OBJ_VAL(sign, 2) >= 1) {
		return;
	}
	const std::string &exhausted =
			MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kComponentExhausted);
	if (!exhausted.empty()) {
		act(exhausted.c_str(), false, ch, sign, nullptr, kToChar);
	}
	for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		if (GET_EQ(ch, i) == sign) {
			UnequipChar(ch, i, CharEquipFlags{});
			break;
		}
	}
	world_objects.AddToExtractedList(sign);
}

static const char *EnchantWeapon(CharData *ch, ObjData *obj, ESpell spell_id) {
	// Either already enchanted or not a weapon.
	if (obj->get_type() != EObjType::kWeapon) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantNotWeapon).c_str();
	}
	if (obj->has_flag(EObjFlag::kMagic)) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantMagic).c_str();
	}
	if (obj->has_flag(EObjFlag::kSetItem)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantSetItem) + "\r\n", ch);
		return nullptr;
	}

	auto reagobj = GET_EQ(ch, EEquipPos::kHold);
	if (reagobj
		&& (GetObjByVnumInContent(GlobalDrop::MAGIC1_ENCHANT_VNUM, reagobj)
			|| GetObjByVnumInContent(GlobalDrop::MAGIC2_ENCHANT_VNUM, reagobj)
			|| GetObjByVnumInContent(GlobalDrop::MAGIC3_ENCHANT_VNUM, reagobj))) {
		// у нас имеется доп символ для зачарования
		obj->set_enchant(GetSkill(ch, ESkill::kLightMagic), reagobj);
		ConsumeEnchantSign(ch, reagobj, spell_id);
	} else {
		obj->set_enchant(GetSkill(ch, ESkill::kLightMagic));
	}
	if (GET_RELIGION(ch) == kReligionMono) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantMono).c_str();
	}
	if (GET_RELIGION(ch) == kReligionPoly) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantPoly).c_str();
	}
	return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantOther).c_str();
}

EStageResult AlterEnchantWeapon(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	const char *msg = EnchantWeapon(ch, ctx.ovict, ctx.spell_id());
	if (msg == nullptr) {
		return EStageResult::kFail;
	}
	act(msg, true, ch, ctx.ovict, nullptr, kToChar);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
