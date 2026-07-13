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

namespace handlers {

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
		// kEnchantWeapon's magical-symbol consumption now flows through the
		// data-driven <components><material> path, so nothing more is needed
		// here other than the enchant being recorded above.
		// declared on kEnchantWeapon in spells.xml. Return value (kBreak when
		// the inventory ingredient is absent) is intentionally ignored at this
		// point: the enchant has already landed via set_enchant above, so the
		// "missing" message simply reports the post-hoc shortage to the player
		// without rolling back -- same UX as the bool overload's old branch
		// where the held slot was empty.
		ProcessMatComponents(ch, ch, spell_id);
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
