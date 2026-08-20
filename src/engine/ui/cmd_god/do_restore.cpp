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
#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_messages.h"

#include <vector>

namespace {

// Что restore считает отрицательным аффектом и снимает.
bool IsHarmfulAffect(const Affect<EApply>::shared_ptr &aff) {
	const auto kind = affects::AffectBuffKind(aff->affect_type);
	if (kind == affects::EBuff::kYes) {
		return false;
	}
	if (kind == affects::EBuff::kNo) {
		return true;
	}
	// buff в affects.xml не указан вовсе (kAmbiguous) -- у 46 аффектов из 139, в том числе у
	// обычной параплегии. Из них вредными считаем те, что лечит первая помощь (kAfCurable) и
	// которые не помечены категорией баффов: так снимаются параплегии, кровотечение и рваные
	// раны, но остаются увеличение/уменьшение, скрытность, чармис, езда и прочее полезное.
	return IS_SET(aff->battleflag, kAfCurable)
		&& !IS_SET(aff->battleflag, kAfBoon)
		&& !IS_SET(aff->battleflag, kAfWarding)
		&& !IS_SET(aff->battleflag, kAfAegis);
}

}  // namespace

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
		vict->timed_skill.clear();
		vict->timed_feat.clear();
		if (subcmd == kScmdRestoreGod) {
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			act("Вы были полностью восстановлены $N4!",
				false, vict, nullptr, ch, kToChar);
		}
		vict->setGloryRespecTime(0);
		// restore -- это "полностью восстановить", но раньше он чинил только хиты, движение и мем.
		// Оцепенение и параплегия его переживали, и восстановленный игрок не мог даже двинуться с
		// места: interpreter.cpp режет любую команду по kHold/kStopFight/kMagicStopFight. Снимаем
		// все отрицательные аффекты. Аффекты от надетых вещей не трогаем -- их все равно вернет
		// affect_total, а проклятую вещь надо снимать, а не ресторить.
		std::vector<EAffect> harmful;
		for (const auto &aff : vict->affected) {
			if (aff && IsHarmfulAffect(aff)) {
				harmful.push_back(aff->affect_type);
			}
		}
		for (const auto affect_type : harmful) {
			RemoveAffectFromCharExceptEquipment(vict, affect_type);
		}
		affect_total(vict);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
