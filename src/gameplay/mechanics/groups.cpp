/**
\file groups.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Код групп игроков и мобов.
\detail Группы, вступление, покидание, дележка опыта - должно быть тут.
*/

#include "engine/entities/char_data.h"

bool same_group(CharData *ch, CharData *tch) {
	if (!ch || !tch)
		return false;

	// Добавлены проверки чтобы не любой заследовавшийся моб считался согруппником (Купала)
	if (ch->IsNpc()
		&& ch->has_master()
		&& !ch->get_master()->IsNpc()
		&& (IS_HORSE(ch)
			|| AFF_FLAGGED(ch, EAffect::kCharmed)
			|| ch->IsFlagged(EMobFlag::kTutelar)
			|| ch->IsFlagged(EMobFlag::kMentalShadow))) {
		ch = ch->get_master();
	}

	if (tch->IsNpc()
		&& tch->has_master()
		&& !tch->get_master()->IsNpc()
		&& (IS_HORSE(tch)
			|| AFF_FLAGGED(tch, EAffect::kCharmed)
			|| tch->IsFlagged(EMobFlag::kTutelar)
			|| tch->IsFlagged(EMobFlag::kMentalShadow))) {
		tch = tch->get_master();
	}

	// NPC's always in same group
	if ((ch->IsNpc() && tch->IsNpc())
		|| ch == tch) {
		return true;
	}

	if (!AFF_FLAGGED(ch, EAffect::kGroup)
		|| !AFF_FLAGGED(tch, EAffect::kGroup)) {
		return false;
	}

	if (ch->get_master() == tch
		|| tch->get_master() == ch
		|| (ch->has_master()
			&& ch->get_master() == tch->get_master())) {
		return true;
	}

	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
