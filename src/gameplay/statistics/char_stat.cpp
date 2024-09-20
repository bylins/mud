/**
\file char_stat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "gameplay/statistics/char_stat.h"

#include "engine/entities/char_data.h"
#include "engine/ui/table_wrapper.h"

#include <third_party_libs/fmt/include/fmt/format.h>

void CharStat::Increase(ECategory category, ullong increment) {
	try {
		statistics_.at(category) += increment;
	} catch (std::out_of_range &) {
		log("SYSERROR: access to unknown statistic %d in CharStat.", category);
	}
}

ullong CharStat::GetValue(ECategory category) const {
	try {
		return statistics_.at(category);
	} catch (std::out_of_range &) {
		log("SYSERROR: access to unknown statistic %d in CharStat.", category);
		return 0LL;
	}
}

void CharStat::Clear() {
	statistics_.fill(0LL);
}

void CharStat::ClearThisRemort() {
	auto it = statistics_.begin();
	std::advance(it, ECategory::CategoryRemortFirst);
	std::fill(it, statistics_.end(), 0LL);
}

void CharStat::Print(CharData *ch) {
	table_wrapper::Table table;
	std::size_t row{0};
	std::size_t col{0};

	table << table_wrapper::kHeader;
	table[row][col] = "Статистика ваших смертей\r\n(количество, потерянного опыта)";
	table[row][++col] = "Текущее\r\nперевоплощение:";
	table[row][++col] = "\r\nВсего:";

	col = 0;
	table[++row][col] = "В неравном бою с тварями:";
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(MobRemortRip), ch->GetStatistic(MobRemortExpLost));
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(MobRip), ch->GetStatistic(MobExpLost));

	col = 0;
	table[++row][col] = "В неравном бою с врагами:";
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(PkRemortRip), ch->GetStatistic(PkRemortExpLost));
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(PkRip), ch->GetStatistic(PkExpLost));

	col = 0;
	table[++row][col] = "В гиблых местах:";
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(DtRemortRip), ch->GetStatistic(DtRemortExpLost));
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(DtRip), ch->GetStatistic(DtExpLost));

	col = 0;
	table[++row][col] = "По стечению обстоятельств:";
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(OtherRemortRip), ch->GetStatistic(OtherRemortExpLost));
	table[row][++col] = fmt::format("{} ({})", ch->GetStatistic(OtherRip), ch->GetStatistic(OtherExpLost));

	col = 0;
	table[++row][col] = "ИТОГО:";
	auto rip_remort_num = ch->GetStatistic(MobRemortRip) + ch->GetStatistic(PkRemortRip) + ch->GetStatistic(DtRemortRip)
		+ ch->GetStatistic(OtherRemortRip);
	auto exp_remort_amount =
		ch->GetStatistic(MobRemortExpLost) + ch->GetStatistic(PkRemortExpLost) + ch->GetStatistic(DtRemortExpLost)
			+ ch->GetStatistic(OtherRemortExpLost) + ch->GetStatistic(ArenaRemortExpLost);
	table[row][++col] = fmt::format("{} ({})", rip_remort_num, exp_remort_amount);

	auto rip_num =
		ch->GetStatistic(MobRip) + ch->GetStatistic(PkRip) + ch->GetStatistic(DtRip) + ch->GetStatistic(OtherRip);
	auto exp_amount = ch->GetStatistic(MobExpLost) + ch->GetStatistic(PkExpLost) + ch->GetStatistic(DtExpLost)
		+ ch->GetStatistic(OtherExpLost) + ch->GetStatistic(ArenaExpLost);
	table[row][++col] = fmt::format("{} ({})", rip_num, exp_amount);

	table << table_wrapper::kEndRow << table_wrapper::kSeparator << table_wrapper::kEndRow;

	col = 0;
	table[++row][col] = "На арене (в текущем перевоплощении):";
	table[row][++col] = " ";
	table[row][++col] = " ";

	col = 0;
	table[++row][col] = fmt::format("Побед: {} ({})", ch->GetStatistic(ArenaWin), ch->GetStatistic(ArenaRemortWin));
	table[row][++col] = fmt::format("Поражений: {} ({})", ch->GetStatistic(ArenaRip), ch->GetStatistic(ArenaRemortRip));
	table[row][++col] =
		fmt::format("Потеряно опыта: {} ({})", ch->GetStatistic(ArenaExpLost), ch->GetStatistic(ArenaRemortExpLost));
	table << table_wrapper::kEndRow << table_wrapper::kSeparator << table_wrapper::kEndRow;

	col = 0;
	table[++row][col] = "Арена доминирования:";
	table[row][++col] =
		fmt::format("Побед: {} ({})", ch->GetStatistic(ArenaDomWin), ch->GetStatistic(ArenaDomRemortWin));
	table[row][++col] =
		fmt::format("Поражений: {} ({})", ch->GetStatistic(ArenaDomRip), ch->GetStatistic(ArenaDomRemortRip));

	table_wrapper::DecorateZebraTextTable(ch, table, table_wrapper::color::kLightCyan);
	table_wrapper::PrintTableToChar(ch, table);
}

/*!
 * Любое обновление статистики должно происходить либо при загрузке персонажа, либо через вызов этой функции.
 * @param ch - персонаж, для которого обновляется статистика.
 * @param killer - убийца, если он есть.
 * @param dec_exp - потерянный опыт.
 */
void CharStat::UpdateOnKill(CharData *ch, CharData *killer, ullong dec_exp) {
	//настоящий убийца мастер чармиса/коня/ангела
	CharData *rkiller = killer;

	if (rkiller
		&& rkiller->IsNpc()
		&& (IS_CHARMICE(rkiller)
			|| IS_HORSE(rkiller)
			|| killer->IsFlagged(EMobFlag::kTutelar)
			|| killer->IsFlagged(EMobFlag::kMentalShadow))) {
		if (rkiller->has_master()) {
			rkiller = rkiller->get_master();
		} else {
			snprintf(buf, kMaxStringLength,
					 "die: %s killed by %s (without master)",
					 GET_PAD(ch, 0), GET_PAD(rkiller, 0));
			mudlog(buf, LGH, kLvlImmortal, SYSLOG, true);
			rkiller = nullptr;
		}
	}
	auto char_room = world[ch->in_room];
	if (!ch->IsNpc()) {
		if (rkiller && rkiller != ch) {
			if (char_room->get_flag(ERoomFlag::kArena)) {
				if (char_room->get_flag(ERoomFlag::kDominationArena)) {
					ch->IncreaseStatistic(ArenaDomRip, 1);
					ch->IncreaseStatistic(ArenaDomRemortRip, 1);
					rkiller->IncreaseStatistic(ArenaDomWin, 1);
					rkiller->IncreaseStatistic(ArenaDomRemortWin, 1);
				} else {
					ch->IncreaseStatistic(ArenaRip, 1);
					ch->IncreaseStatistic(ArenaRemortRip, 1);
					killer->IncreaseStatistic(ArenaWin, 1);
					killer->IncreaseStatistic(ArenaRemortWin, 1);
					if (dec_exp) {
						ch->IncreaseStatistic(ArenaExpLost, dec_exp);
						ch->IncreaseStatistic(ArenaRemortExpLost, dec_exp);
					}
				}
			} else if (rkiller->IsNpc()) {
				ch->IncreaseStatistic(MobRip, 1);
				ch->IncreaseStatistic(MobRemortRip, 1);
				if (dec_exp) {
					ch->IncreaseStatistic(MobExpLost, dec_exp);
					ch->IncreaseStatistic(MobExpLost, dec_exp);
				}
			} else if (!rkiller->IsNpc()) {
				ch->IncreaseStatistic(PkRip, 1);
				ch->IncreaseStatistic(PkRemortRip, 1);
				if (dec_exp) {
					ch->IncreaseStatistic(PkExpLost, dec_exp);
					ch->IncreaseStatistic(PkRemortExpLost, dec_exp);
				}
			}
		} else if ((!rkiller || (rkiller && rkiller == ch)) &&
			(char_room->get_flag(ERoomFlag::kDeathTrap) ||
				char_room->get_flag(ERoomFlag::kSlowDeathTrap) ||
				char_room->get_flag(ERoomFlag::kIceTrap))) {
			ch->IncreaseStatistic(DtRip, 1);
			ch->IncreaseStatistic(DtRemortRip, 1);
			if (dec_exp) {
				ch->IncreaseStatistic(DtExpLost, dec_exp);
				ch->IncreaseStatistic(DtRemortExpLost, dec_exp);
			}
		} else {
			ch->IncreaseStatistic(OtherRip, 1);
			ch->IncreaseStatistic(OtherRemortRip, 1);
			if (dec_exp) {
				ch->IncreaseStatistic(OtherExpLost, dec_exp);
				ch->IncreaseStatistic(OtherRemortExpLost, dec_exp);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
