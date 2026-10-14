/**
\file do_visible.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd/do_visible.h"
#include "administration/privilege.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/sight.h"

namespace {

// Незаметность, которую снимает "появиться".
const EAffect kStealthAffects[] =
	{EAffect::kInvisible, EAffect::kHide, EAffect::kSneak, EAffect::kDisguise};

// Волшебство надетой вещи команде не подчиняется: узел с kAfFromEquipment "появиться" не снимает
// (иначе бой убивал бы его насовсем и вещь приходилось бы перенадевать), а снятый флаг вернётся на
// ближайшем пересчёте. Поэтому, если незаметность идёт с вещи, честнее назвать вещь, а не писать
// "вы теперь полностью видны": снять её можно только сняв предмет.
ObjData *FindWornStealthSource(CharData *ch) {
	for (const auto affect_type : kStealthAffects) {
		if (!AFF_FLAGGED(ch, affect_type)) {
			continue;
		}
		if (auto *obj = FindEquipmentStealthSource(ch, affect_type)) {
			return obj;
		}
	}
	return nullptr;
}

} // namespace

void do_visible(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (privilege::IsImmortal(ch)) {
		perform_immort_vis(ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kInvisible)
		|| AFF_FLAGGED(ch, EAffect::kDisguise)
		|| AFF_FLAGGED(ch, EAffect::kHide)
		|| AFF_FLAGGED(ch, EAffect::kSneak)) {
		// Незаметность с вещи командой не снимается: трогать узел нельзя (иначе бой убивал бы
		// волшебство вещи насовсем), а снять один флаг -- значит соврать, он вернётся на
		// ближайшем пересчёте. Поэтому просто говорим, что мешает, и ничего не меняем.
		if (ObjData *worn = FindWornStealthSource(ch)) {
			act("Вас делает незаметным волшебство $o2 -- снимите вещь.", false, ch, worn, nullptr, kToChar);
			return;
		}
		sight::Appear(ch, false);
		SendMsgToChar("Вы перестали быть невидимым.\r\n", ch);
	} else
		SendMsgToChar("Вы и так видимы.\r\n", ch);
}

void perform_immort_vis(CharData *ch) {
	if (GET_INVIS_LEV(ch) == 0 &&
		!AFF_FLAGGED(ch, EAffect::kHide) && !AFF_FLAGGED(ch, EAffect::kInvisible)
		&& !AFF_FLAGGED(ch, EAffect::kDisguise)) {
		SendMsgToChar("Ну вот вас и заметили. Стало ли вам легче от этого?\r\n", ch);
		return;
	}

	ObjData *worn = FindWornStealthSource(ch);
	// Уровень невидимости снимается всегда -- это своя, божеская незаметность, и она команде
	// подчиняется. А волшебство надетой вещи -- нет, см. ветку для смертного.
	SET_INVIS_LEV(ch, 0);
	if (worn) {
		act("Уровень невидимости снят. Незаметным вас делает волшебство $o2 -- снимите вещь.",
			false, ch, worn, nullptr, kToChar);
		return;
	}
	sight::Appear(ch, false);
	SendMsgToChar("Вы теперь полностью видны.\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
