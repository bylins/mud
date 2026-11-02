/**
\file do_rent_search.cpp - a part of the Bylins engine.
\authors Created by Dedal.
\date 02.11.2026.
\brief Поиск вещи по внуму среди тех, кто сейчас в ренте.
\detail По каждому отрентованному игроку в памяти лежит снимок таймеров
(SaveInfo в player_table), по нему и ищем. У игрока, который в игре, этот снимок
вычищен, а его вещи живые -- их ищет "где". В снимке есть только внум и таймер,
поэтому подробностей по самой вещи тут не будет.
*/

#include "do_rent_search.h"

#include <algorithm>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "engine/db/obj_prototypes.h"
#include "engine/db/obj_save.h"
#include "engine/db/player_index.h"
#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/weather.h"
#include "utils/native_text.h"
#include "utils/utils_string.h"

namespace {

// Ширина столбца с именем в знаках: fmt считает байты, а имена у нас в UTF-8,
// и на кириллице столбцы разъезжаются (как в issue #3681).
std::string PadName(const std::string &name, std::size_t width) {
	const auto len = native_text::char_count(name);
	return len < width ? std::string(width - len, ' ') : std::string();
}

} // namespace

void DoRentSearch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	one_argument(argument, arg);
	if (!*arg || !a_isdigit(*arg)) {
		SendMsgToChar("Формат: рента <внум>\r\n", ch);
		return;
	}
	const int vnum = std::atoi(arg);
	const auto rnum = GetObjRnum(vnum);
	// у вещи с бесконечным таймером или с флагом "не тикает в ренте" остаток не считаем
	const bool frozen = rnum >= 0
		&& (stable_objs::IsTimerUnlimited(obj_proto[rnum].get())
			|| obj_proto[rnum]->has_flag(EObjFlag::kNoRentTimer));

	std::vector<std::pair<std::string, std::string>> rows;
	int total_items = 0;
	const auto now = time(nullptr);
	for (std::size_t i = 0; i < player_table.size(); ++i) {
		const auto *si = SAVEINFO(i);
		if (!si || player_table[i].uid() == -1) {
			continue;
		}
		const auto rented = now - si->rent.time;
		const auto ticks = static_cast<int>((rented / kSecsPerMudHour) + (rented % kSecsPerMudHour ? 1 : 0));
		const auto items = std::min(static_cast<std::size_t>(si->rent.n_items), si->time.size());
		int count = 0;
		int left = 0;
		for (std::size_t k = 0; k < items; ++k) {
			if (si->time[k].vnum != vnum || si->time[k].timer < 0) {
				continue;
			}
			const auto timer = frozen ? si->time[k].timer : std::max(-1, si->time[k].timer - ticks);
			if (count == 0 || timer < left) {
				left = timer;
			}
			++count;
		}
		if (count == 0) {
			continue;
		}
		total_items += count;
		auto name = utils::CAP(player_table[i].name());
		const auto timer_str = frozen ? std::string("не тикает")
									  : (left < 0 ? std::string("истек") : std::to_string(left));
		rows.emplace_back(name, fmt::format("  {}{}  {:>3} шт.  таймер {:>9}  в ренте {} тиков\r\n",
											name, PadName(name, 16), count, timer_str, ticks));
	}
	std::sort(rows.begin(), rows.end());

	std::string out = rnum >= 0
					  ? fmt::format("Вещь: {} #{}\r\n", obj_proto[rnum]->get_short_description(), vnum)
					  : fmt::format("Вещь #{} (прототипа нет)\r\n", vnum);
	if (rows.empty()) {
		out += "В ренте такой вещи ни у кого нет.\r\n";
	} else {
		for (const auto &row : rows) {
			out += row.second;
		}
		out += fmt::format("Игроков: {}, вещей: {}.\r\n", rows.size(), total_items);
	}
	out += "Считаются только те, кто сейчас в ренте. Вещи игроков в игре ищет \"где\".\r\n";
	page_string(ch->desc, out);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
