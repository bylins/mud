/**
\file DoArenaRestore.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/fight/fight.h"

void DoArenaRestore(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	one_argument(argument, buf);
	if (!*buf)
		SendMsgToChar("Кого вы хотите восстановить?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, EFind::kCharInWorld)))
		SendMsgToChar(NOPERSON, ch);
	else {
		vict->set_hit(vict->get_real_max_hit());
		vict->set_move(vict->get_real_max_move());
		if (IS_MANA_CASTER(vict)) {
			vict->mem_queue.stored = GET_MAX_MANA(vict);
		} else {
			vict->mem_queue.stored = vict->mem_queue.total;
		}
		if (vict->GetSkill(ESkill::kWarcry) > 0) {
			struct TimedSkill wctimed;
			wctimed.skill = ESkill::kWarcry;
			wctimed.time = 0;
			ImposeTimedSkill(vict, &wctimed);
		}
		if (IS_GRGOD(ch) && IS_IMMORTAL(vict)) {
			vict->set_str(25);
			vict->set_int(25);
			vict->set_wis(25);
			vict->set_dex(25);
			vict->set_con(25);
			vict->set_cha(25);
		}
		update_pos(vict);
		RemoveAffectFromChar(vict, ESpell::kDrunked);
		GET_DRUNK_STATE(vict) = GET_COND(vict, DRUNK) = 0;
		RemoveAffectFromChar(vict, ESpell::kAbstinent);

		//сброс таймеров скиллов и фитов
		while (vict->timed)
			ExpireTimedSkill(vict, vict->timed);
		while (vict->timed_feat)
			ExpireTimedFeat(vict, vict->timed_feat);
		reset_affects(vict);
		for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (GET_EQ(vict, i)) {
				remove_otrigger(GET_EQ(vict, i), vict);
				ExtractObjFromWorld(UnequipChar(vict, i, CharEquipFlags()));
			}
		}
		ObjData *obj;
		for (obj = vict->carrying; obj; obj = vict->carrying) {
			RemoveObjFromChar(obj);
			ExtractObjFromWorld(obj);
		}
		act("Все ваши вещи были удалены и все аффекты сняты $N4!",
			false, vict, nullptr, ch, kToChar);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
