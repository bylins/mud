/**
\file peer.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/handler.h"

void DoPeer(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int i;

	if (!ch->desc)
		return;

	if (ch->GetPosition() < EPosition::kSleep)
		SendMsgToChar("Белый Ангел возник перед вами, маняще помахивая крыльями.\r\n", ch);
	if (ch->GetPosition() == EPosition::kSleep)
		SendMsgToChar("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffect::kBlind))
		SendMsgToChar("Вы ослеплены!\r\n", ch);
	else if (ch->GetSkill(ESkill::kLooking)) {
		if (check_moves(ch, kLookingMoves)) {
			SendMsgToChar("Вы напрягли зрение и начали присматриваться по сторонам.\r\n", ch);
			for (i = 0; i < EDirection::kMaxDirNum; i++)
				look_in_direction(ch, i, EXIT_SHOW_LOOKING);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)))
				SetWaitState(ch, 1 * kBattleRound);
		}
	} else
		SendMsgToChar("Вам явно не хватает этого умения.\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
