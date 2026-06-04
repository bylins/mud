/**
 \file charm.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: charmed-minion control mechanics (extracted from spells.cpp).
*/

#include "gameplay/mechanics/charm.h"

#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "gameplay/mechanics/minions.h"   // CalcCharmPoint
#include "engine/ui/cmd/do_hire.h"        // GetReformedCharmiceHp
#include "utils/logger.h"

int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id) {
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = false, living_in_group = false;

	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)
			&& k->get_master() == ch) {
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->ch);
			reformed_hp_summ += GetReformedCharmiceHp(ch, k, spell_id);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (k->IsFlagged(EMobFlag::kCorpse)) {
				undead_in_group = true;
			} else {
				living_in_group = true;
			}
		}
	}

	if (undead_in_group && living_in_group) {
		mudlog("SYSERR: Undead and living in group simultaniously", NRM, kLvlGod, ERRLOG, true);
		return (false);
	}

	if (spell_id == ESpell::kCharm && undead_in_group) {
		SendMsgToChar("Ваша жертва боится ваших последователей.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kCharm && living_in_group) {
		SendMsgToChar("Ваш последователь мешает вам произнести это заклинание.\r\n", ch);
		return (false);
	}

	if (spell_id == ESpell::kClone && cha_summ >= std::max(1, (GetRealLevel(ch) + 4) / 5 - 2)) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kClone && cha_summ >= (GetRealLevel(ch) + 9) / 10) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kClone &&
		reformed_hp_summ + GetReformedCharmiceHp(ch, victim, spell_id) >= CalcCharmPoint(ch, spell_id)) {
		SendMsgToChar("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (false);
	}
	return (true);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
