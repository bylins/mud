/**
\file do_restore.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/condition.h"
#include "administration/privilege.h"
#include "engine/core/target_resolver.h"
#include "gameplay/fight/fight.h"

void DoRestore(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;

	one_argument(argument, buf);
	if (!*buf)
		SendMsgToChar("Кого вы хотите восстановить?\r\n", ch);
	else if (!(vict = target_resolver::FindCharInWorld(ch, buf)))
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	else {
		// имм с привилегией arena может ресторить только чаров, находящихся с ним на этой же арене
		// плюс исключается ситуация, когда они в одной зоне, но чар не в клетке арены
		if (privilege::CheckFlag(ch, privilege::kArenaMaster) && !privilege::IsImpl(ch)) {
			if (!ROOM_FLAGGED(vict->in_room, ERoomFlag::kArena) || world[ch->in_room]->zone_rn != world[vict->in_room]->zone_rn) {
				SendMsgToChar("Не положено...\r\n", ch);
				return;
			}
		}

		vict->set_hit(vict->get_real_max_hit());
		vict->set_move(vict->get_real_max_move());
		if (IS_MANA_CASTER(vict)) {
			vict->mem_queue.stored = Mana(GetRealWis(vict));
		} else {
			vict->mem_queue.stored = vict->mem_queue.total;
		}
		if (privilege::IsGrGod(ch) && privilege::IsImmortal(vict)) {
			vict->set_str(25);
			vict->set_int(25);
			vict->set_wis(25);
			vict->set_dex(25);
			vict->set_con(25);
			vict->set_cha(25);
		}
		update_pos(vict);
		RemoveAffectFromChar(vict, EAffect::kDrunked);
		GET_DRUNK_STATE(vict) = GET_COND(vict, condition::kDrunk) = 0;
		RemoveAffectFromChar(vict, EAffect::kAbstinent);

		//сброс таймеров скиллов и фитов
		ch->timed_skill.clear();
		ch->timed_feat.clear();
		if (subcmd == kScmdRestoreGod) {
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			act("Вы были полностью восстановлены $N4!",
				false, vict, nullptr, ch, kToChar);
		}
		vict->setGloryRespecTime(0);
		affect_total(vict);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
