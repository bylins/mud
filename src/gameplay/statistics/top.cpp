#include "top.h"

#include "gameplay/classes/pc_classes.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/glory_const.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "engine/ui/table_wrapper.h"
#include "utils/utils_time.h"


PlayerChart TopPlayer::chart_(kNumPlayerClasses);

// отдельное удаление из списка (для ренеймов, делетов и т.п.)
// данная функция работает в том числе и с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void TopPlayer::Remove(CharData *short_ch) {
	auto &tmp_list = TopPlayer::chart_[short_ch->GetClass()];

	auto it = std::find_if(tmp_list.begin(), tmp_list.end(), [&short_ch](const TopPlayer &p) {
		return p.unique_ == short_ch->get_uid();
	});
	if (it != tmp_list.end())
		tmp_list.erase(it);
}

// проверяем надо-ли добавлять в топ и добавляем/обновляем при случае. reboot по дефолту 0 (1 для ребута)
// данная функция работает в том числе и с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void TopPlayer::Refresh(CharData *short_ch, bool reboot) {
	if (short_ch->IsNpc()
		|| short_ch->IsFlagged(EPlrFlag::kFrozen)
		|| short_ch->IsFlagged(EPlrFlag::kDeleted)
		|| IS_IMMORTAL(short_ch)) {
		return;
	}
	if (!reboot) {
		TopPlayer::Remove(short_ch);
	}

	std::list<TopPlayer>::iterator it_exp;
	for (it_exp = TopPlayer::chart_[short_ch->GetClass()].begin();
		 it_exp != TopPlayer::chart_[short_ch->GetClass()].end(); ++it_exp) {
		if (it_exp->remort_ < GetRealRemort(short_ch)
			|| (it_exp->remort_ == GetRealRemort(short_ch) && it_exp->exp_ < short_ch->get_exp())) {
			break;
		}
	}

	if (short_ch->get_name().empty()) {
		return; // у нас все может быть
	}
	TopPlayer temp_player(short_ch->get_uid(), GET_NAME(short_ch), short_ch->get_exp(), GetRealRemort(short_ch), 0);

	if (it_exp != TopPlayer::chart_[short_ch->GetClass()].end()) {
		TopPlayer::chart_[short_ch->GetClass()].insert(it_exp, temp_player);
	} else {
		TopPlayer::chart_[short_ch->GetClass()].push_back(temp_player);
	}
}

const PlayerChart &TopPlayer::Chart() {
	return chart_;
}

void TopPlayer::PrintPlayersChart(CharData *ch) {
	SendMsgToChar(" Лучшие персонажи игроков:\r\n", ch);

	table_wrapper::Table table;
	for (const auto &it: TopPlayer::Chart()) {
		table
			<< it.second.begin()->name_
			<< it.second.begin()->remort_
			<< GetDeclensionInNumber(it.second.begin()->remort_, EWhat::kRemort)
			<< MUD::Class(it.first).GetName() << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToChar(ch, table);

}

void TopPlayer::PrintClassChart(CharData *ch, ECharClass id) {
	int count = 1;

	std::ostringstream out;
	out << kColorWht << " Лучшие " << MUD::Class(id).GetPluralName() << ":" << kColorNrm << "\r\n";

	for (auto &it: TopPlayer::chart_[id]) {
		if (it.remort_ == kMaxRemort)
			continue;
		it.number_ = count++;
	}
	table_wrapper::Table table;
	for (const auto &it: TopPlayer::chart_[id]) {
		if (it.remort_ == kMaxRemort)
			continue;
		table << it.number_
			<< it.name_
			<< it.remort_
			<< GetDeclensionInNumber(it.remort_, EWhat::kRemort) << table_wrapper::kEndRow;
		if (table.row_count() >= kPlayerChartSize) {
			break;
		}
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(out, table);
	out << "\r\n";
	// если игрок участвует в данном топе - покажем ему, какой он неудачник
	count = 1;
	int deep = 0;
	std::list<TopPlayer> upper;
	for (const auto &it: reverse(TopPlayer::chart_[id])) {
		if (deep == 0) {
			if (it.unique_ == ch->get_uid()) {
				out  << " Перед вами:\r\n";
				deep = 1;
				continue;
			}
		} else {
			if (deep++ == 4)
				break;
			upper.push_front(it);
		}
	}
	table_wrapper::Table table3;
	for (auto &it : upper) {
		table3 << it.number_ << it.name_ << it.remort_ << GetDeclensionInNumber(it.remort_, EWhat::kRemort) << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table3);
	table_wrapper::PrintTableToStream(out, table3);

	deep = 0;
	table_wrapper::Table table4;
	for (const auto &it: TopPlayer::chart_[id]) {
		if (deep == 0) {
			if (it.unique_ == ch->get_uid()) {
				out << "\r\n" << " Ваш текущий рейтинг: " << it.number_ << " - " << it.remort_ << " " << GetDeclensionInNumber(it.remort_, EWhat::kRemort) << "\r\n";
				out  << "\r\n После вас:\r\n";
				deep = 1;
				continue;
			}
		} else {
			if (deep++ == 4)
				break;
			table4 << it.number_ << it.name_ << it.remort_ << GetDeclensionInNumber(it.remort_, EWhat::kRemort) << table_wrapper::kEndRow;
		}
	}
	table_wrapper::DecorateNoBorderTable(ch, table4);
	table_wrapper::PrintTableToStream(out, table4);
	table_wrapper::Table table2;
	upper.clear();
	for (const auto &it: TopPlayer::chart_[id]) {
		if (it.remort_ != kMaxRemort) 
			continue;
		upper.push_back(it);
	}
	if (upper.size() > 0) {
		out << kColorWht << "\r\n Достигшие максимум перевоплощений: " << kColorNrm << "\r\n";
		for (auto &it : upper) {
			table2 << it.name_
				<< it.remort_
				<< GetDeclensionInNumber(it.remort_, EWhat::kRemort) << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table2);
		table_wrapper::PrintTableToStream(out, table2);
	}
	SendMsgToChar(out.str(), ch);
}

void TopPlayer::PrintHelp(CharData *ch) {
	SendMsgToChar(" Лучшими могут быть:\n", ch);

	table_wrapper::Table table;
	const int columns_num{2};
	int count = 1;
	for (const auto &it: MUD::Classes()) {
		if (it.IsAvailable()) {
			table << it.GetPluralName();
			if (count % columns_num == 0) {
				table << table_wrapper::kEndRow;
			}
			++count;
		}
	}
	for (const auto &str: {"игроки", "прославленные"}) {
		table << str;
		if (count % columns_num == 0) {
			table << table_wrapper::kEndRow;
		}
		++count;
	}

	table_wrapper::DecorateSimpleTable(ch, table);
	table_wrapper::PrintTableToChar(ch, table);
}

void Rating::DoBest(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}

	std::string buffer = argument;
	utils::Trim(buffer);

	if (CompareParam(buffer, "прославленные")) {
		GloryConst::PrintGloryChart(ch);
		return;
	}

	if (CompareParam(buffer, "игроки")) {
		TopPlayer::PrintPlayersChart(ch);
		return;
	}

	auto class_id = FindAvailableCharClassId(buffer);
	if (class_id != ECharClass::kUndefined) {
		TopPlayer::PrintClassChart(ch, class_id);
		return;
	}

	TopPlayer::PrintHelp(ch);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
