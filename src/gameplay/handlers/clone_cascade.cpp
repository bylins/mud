/**
 \file clone_cascade.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: CloneCascade kClone post-spawn handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/db/db.h"
#include "gameplay/mechanics/summon.h"
#include "utils/utils.h"
#include <cstdio>

// ApplyCloneCosmetics: single-use helper for CloneCascade (kept file-local).
static void ApplyCloneCosmetics(CharData *ch, CharData *mob) {
	sprintf(buf2, "двойник %s %s", GET_PAD(ch, 1), GET_NAME(ch));
	mob->SetCharAliases(buf2);
	sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
	mob->set_npc_name(buf2);
	mob->player_data.long_descr = "";
	sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kNom] = std::string(buf2);
	sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kGen] = std::string(buf2);
	sprintf(buf2, "двойнику %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kDat] = std::string(buf2);
	sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kAcc] = std::string(buf2);
	sprintf(buf2, "двойником %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kIns] = std::string(buf2);
	sprintf(buf2, "двойнике %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kPre] = std::string(buf2);

	mob->set_str(ch->get_str());
	mob->set_dex(ch->get_dex());
	mob->set_con(ch->get_con());
	mob->set_wis(ch->get_wis());
	mob->set_int(ch->get_int());
	mob->set_cha(ch->get_cha());

	mob->set_level(GetRealLevel(ch));
	GET_HR(mob) = -20;
	GET_AC(mob) = GET_AC(ch);
	GET_DR(mob) = GET_DR(ch);

	mob->set_max_hit(ch->get_max_hit());
	mob->set_hit(ch->get_max_hit());
	mob->mob_specials.damnodice = 0;
	mob->mob_specials.damsizedice = 0;
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
	mob->set_exp(0);

	mob->SetPosition(EPosition::kStand);
	GET_DEFAULT_POS(mob) = EPosition::kStand;
	mob->set_sex(EGender::kMale);

	mob->set_class(ch->GetClass());
	GET_WEIGHT(mob) = GET_WEIGHT(ch);
	GET_HEIGHT(mob) = GET_HEIGHT(ch);
	GET_SIZE(mob) = GET_SIZE(ch);
	mob->SetFlag(EMobFlag::kClone);
	mob->UnsetFlag(EMobFlag::kMounting);
}

// CheckCharmices is defined non-static in spells.cpp (forward-declared here, as magic.cpp does).
int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id);

namespace handlers {

void CloneCascade(CharData *ch, CharData *mob, const CastContext &ctx, int /*duration*/) {
	ApplyCloneCosmetics(ch, mob);
	int already = 0;
	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed) && k->get_master() == ch) {
			++already;
		}
	}
	int remaining = std::max(1, (GetRealLevel(ch) + 4) / 5 - 2) - already;
	while (remaining-- > 0) {
		CharData *extra = ReadMobile(-kMobDouble, kVirtual);
		if (!extra) {
			break;
		}
		if (IsSummonTargetProtected(ch, extra, ctx.spell_id())) {
			continue;
		}
		if (!CheckCharmices(ch, extra, ctx.spell_id())) {
			ExtractCharFromWorld(extra, false);
			break;
		}
		FinalizeSummonedMob(ch, extra, ctx.spell_id(), true);
		ApplyCloneCosmetics(ch, extra);
		extra->SetFlag(EMobFlag::kNoSkillTrain);
		extra->char_specials.saved.alignment = ch->char_specials.saved.alignment;
	}
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
