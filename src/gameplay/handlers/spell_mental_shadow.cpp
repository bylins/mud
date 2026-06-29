/**
 \file spell_mental_shadow.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellMentalShadow manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/follow.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/minions.h"
#include "engine/core/char_handler.h"

namespace handlers {

EStageResult SpellMentalShadow(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}

	// подготовка контейнера для создания заклинания ментальная тень
	// все предложения пишем мад почтой

	MobVnum mob_num = kMobMentalShadow;

	CharData *mob = nullptr;
	auto followers_copy = ch->followers;
	for (auto *k : followers_copy) {
		if (k->IsFlagged(EMobFlag::kMentalShadow)) {
			follow::StopFollower(k, false);
		}
	}
	auto eff_int = get_effective_int(ch);
	int hp = 100;
	int hp_per_int = 15;
	float base_ac = 100;
	float additional_ac = -1.5;
	if (eff_int < 26 && !privilege::IsImmortal(ch)) {
		// low-Int rejection on kMentalShadow's sheaf as kCustomMsgOne.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kMentalShadow, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
		return EStageResult::kSuccess;
	};

	if (!(mob = ReadMobile(mob_num, kVirtual))) {
		// kSummonNoProto kDefault already carries this exact text -- the sheaf lookup
		// returns it without any per-spell override needed.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kMentalShadow, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	Affect<EApply> af;
	af.duration = CalcDuration(mob, mob, ESkill::kUndefined, 5 + (int) VPOSI<float>((get_effective_int(ch) - 16.0) / 2, 0, 50), 0, 0, 0);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.affect_type = EAffect::kHelper;
	af.battleflag = kAfCharmBond;
	affect_to_char(mob, af);
	
	mob->set_max_hit(floorf(hp + hp_per_int * (eff_int - 20) + ch->get_hit()/4));
	mob->set_hit(mob->get_max_hit());
	GET_AC(mob) = floorf(base_ac + additional_ac * eff_int);
	// Добавление заклов и аффектов в зависимости от интелекта кудеса
	if (eff_int >= 28 && eff_int < 32) {
     	SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 1);
	} else if (eff_int >= 32 && eff_int < 38) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 1);
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(mob, af);
		mob->mob_specials.have_spell = true;
	} else if(eff_int >= 38 && eff_int < 44) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 2);
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(mob, af);
		mob->mob_specials.have_spell = true;
	
	} else if(eff_int >= 44) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 3);
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(mob, af);
		af.affect_type = EAffect::kBrokenChains;
		affect_to_char(mob, af);
		mob->mob_specials.have_spell = true;
	}
	if (GetSkill(mob, ESkill::kAwake)) {
		mob->SetFlag(EPrf::kAwake);
	}
	mob->set_level(GetRealLevel(ch));
	mob->SetFlag(EMobFlag::kCorpse);
	mob->SetFlag(EMobFlag::kMentalShadow);
	mob->SetFlag(EMobFlag::kSummoned);	// true conjuration (banishable)
	mob->SetFlag(EMobFlag::kCompanion);	// any NPC ally
	PlaceCharToRoom(mob, ch->in_room);
	follow::AddFollower(ch, mob);
	mob->set_protecting(ch);
	
	// kMentalShadow overrides kSummonToRoom (whose kDefault sheaf carries 9
	// random-failure variants used by kClone-style spells) with its single
	// success line.
	const auto &shadow_msg = MUD::SpellMessages().GetMessage(
			ESpell::kMentalShadow, ESpellMsg::kSummonToRoom);
	act(shadow_msg.c_str(), true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
