#include "top.h"

#include "game_classes/classes.h"
#include "color.h"
#include "game_mechanics/glory_const.h"
#include "entities/char_data.h"
#include "structs/global_objects.h"
#include "utils/table_wrapper.h"

PlayerChart TopPlayer::chart_(kNumPlayerClasses);

// отдельное удаление из списка (для ренеймов, делетов и т.п.)
// данная функция работает в том числе и с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void TopPlayer::Remove(CharData *short_ch) {
	auto &tmp_list = TopPlayer::chart_[short_ch->get_class()];

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
	if (short_ch->is_npc()
		|| PLR_FLAGS(short_ch).get(PLR_FROZEN)
		|| PLR_FLAGS(short_ch).get(PLR_DELETED)
		|| IS_IMMORTAL(short_ch)) {
		return;
	}

	if (!reboot) {
		TopPlayer::Remove(short_ch);
	}

	std::list<TopPlayer>::iterator it_exp;
	for (it_exp = TopPlayer::chart_[short_ch->get_class()].begin();
		 it_exp != TopPlayer::chart_[short_ch->get_class()].end(); ++it_exp) {
		if (it_exp->remort_ < GET_REAL_REMORT(short_ch)
			|| (it_exp->remort_ == GET_REAL_REMORT(short_ch) && it_exp->exp_ < GET_EXP(short_ch))) {
			break;
		}
	}

	if (short_ch->get_name().empty()) {
		return; // у нас все может быть
	}
	TopPlayer temp_player(GET_UNIQUE(short_ch), GET_NAME(short_ch), GET_EXP(short_ch), GET_REAL_REMORT(short_ch));

	if (it_exp != TopPlayer::chart_[short_ch->get_class()].end()) {
		TopPlayer::chart_[short_ch->get_class()].insert(it_exp, temp_player);
	} else {
		TopPlayer::chart_[short_ch->get_class()].push_back(temp_player);
	}
}

const PlayerChart &TopPlayer::Chart() {
	return chart_;
};

void TopPlayer::PrintPlayersChart(CharData *ch) {
	send_to_char(" Лучшие персонажи игроков:\r\n", ch);

	fort::char_table table;
	for (const auto &it: TopPlayer::Chart()) {
		table
			<< it.second.begin()->name_
			<< it.second.begin()->remort_
			<< desc_count(it.second.begin()->remort_, WHAT_REMORT)
			<< MUD::Classes()[it.first].GetName() << fort::endr;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToChar(ch, table);
}

void TopPlayer::PrintClassChart(CharData *ch, ECharClass id) {
	std::ostringstream out;
	out << KWHT << " Лучшие " << MUD::Classes()[id].GetPluralName() << ":" << KNRM << std::endl;

	fort::char_table table;
	for (const auto &it: TopPlayer::chart_[id]) {
		table
			<< it.name_
			<< it.remort_
			<< desc_count(it.remort_, WHAT_REMORT) << fort::endr;

	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(out, table);

	// если игрок участвует в данном топе - покажем ему, какой он неудачник
	int count = 1;
	for (const auto &it: TopPlayer::chart_[id]) {
		if (it.unique_ == ch->get_uid()) {
			out.clear();
			out << std::endl << "  Ваш текущий рейтинг: " << count << std::endl;
			break;
		}
		++count;
	}
	send_to_char(out.str(), ch);
}

void TopPlayer::PrintHelp(CharData *ch) {
	send_to_char(" Лучшими могут быть:\n", ch);

	fort::char_table table;
	const int columns_num{2};
	int count = 1;
	for (const auto &it: MUD::Classes()) {
		if (it.IsAvailable()) {
			table << it.GetPluralName();
			if (count % columns_num == 0) {
				table << fort::endr;
			}
			++count;
		}
	}
	for (const auto &str: {"игроки", "прославленные"}) {
		table << str;
		if (count % columns_num == 0) {
			table << fort::endr;
		}
		++count;
	}

	table_wrapper::DecorateSimpleTable(ch, table);
	table_wrapper::PrintTableToChar(ch, table);
}

void DoBest(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc()) {
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
