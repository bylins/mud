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
#include <vector>
#include <string>
#include <algorithm>

namespace {

// Всё, что скрывает персонажа и что снимает "появиться": невидимость плюс три скрытности. В
// движке скрытностью (char_stealth_aff) зовут только последние три, невидимость в этот набор не
// входит, поэтому здесь свой список и своё имя.
const EAffect kConcealingAffects[] =
	{EAffect::kInvisible, EAffect::kHide, EAffect::kSneak, EAffect::kDisguise};

// Волшебство надетой вещи команде не подчиняется: узел с kAfFromEquipment "появиться" не снимает
// (иначе бой убивал бы его насовсем и вещь приходилось бы перенадевать), а снятый флаг вернётся на
// ближайшем пересчёте. Поэтому, если незаметность идёт с вещей, честнее назвать их, а не писать
// "вы теперь полностью видны": снять её можно только сняв предметы. Называем все сразу -- иначе
// игрок снимает по одной, каждый раз переспрашивая командой.
std::vector<ObjData *> FindWornConcealingItems(CharData *ch) {
	std::vector<ObjData *> items;
	for (const auto affect_type : kConcealingAffects) {
		if (!AFF_FLAGGED(ch, affect_type)) {
			continue;
		}
		for (auto *obj : FindEquipmentAffectSources(ch, affect_type)) {
			if (std::find(items.begin(), items.end(), obj) == items.end()) {
				items.push_back(obj);
			}
		}
	}
	return items;
}

// "плаща теней" / "плаща теней и кольца тьмы" -- родительный падеж после "волшебство".
std::string ConcealingItemsPhrase(const std::vector<ObjData *> &items) {
	std::string phrase;
	for (std::size_t i = 0; i < items.size(); ++i) {
		if (i > 0) {
			phrase += (i + 1 == items.size()) ? " и " : ", ";
		}
		phrase += items[i]->get_PName(grammar::ECase::kGen);
	}
	return phrase;
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
		const auto worn = FindWornConcealingItems(ch);
		if (!worn.empty()) {
			SendMsgToChar(ch, "Вас делает незаметным волшебство %s -- %s\r\n",
						  ConcealingItemsPhrase(worn).c_str(),
						  worn.size() > 1 ? "снимите эти вещи." : "снимите вещь.");
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

	const auto worn = FindWornConcealingItems(ch);
	// Уровень невидимости снимается всегда -- это своя, божеская незаметность, и она команде
	// подчиняется. А волшебство надетой вещи -- нет, см. ветку для смертного.
	SET_INVIS_LEV(ch, 0);
	if (!worn.empty()) {
		SendMsgToChar(ch, "Уровень невидимости снят. Незаметным вас делает волшебство %s -- %s\r\n",
					  ConcealingItemsPhrase(worn).c_str(),
					  worn.size() > 1 ? "снимите эти вещи." : "снимите вещь.");
		return;
	}
	sight::Appear(ch, false);
	SendMsgToChar("Вы теперь полностью видны.\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
