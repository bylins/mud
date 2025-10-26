// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "administration/karma.h"
#include "genchar.h"
#include "engine/ui/color.h"
#include "engine/entities/char_data.h"

#include <third_party_libs/fmt/include/fmt/format.h>

extern bool ValidateStats(DescriptorData *d);
extern int check_dupes_email(DescriptorData *d);
extern void do_entergame(DescriptorData *d);

namespace stats_reset {

const char *CONFIG_FILE = LIB_MISC"reset_stats.xml";

// для списка reset_prices
struct price_node {
	int base_price;
	int add_price;
	int max_price;
	// название сбрасываемой характеристики для сислога/кармы
	std::string log_text;
};

// список цен для разных типов резета характеристик чара (из CONFIG_FILE)
std::array<price_node, Type::TOTAL_NUM> reset_prices =
	{{
		 {100000000, 1000000, 1000000000, "main stats"},
		 {110000000, 1100000, 1100000000, "race"},
		 {120000000, 1200000, 1200000000, "feats"},
		 {400000, 200000, 1000000, "religion"}
	 }};

///
/// Парс отдельной записи в CONFIG_FILE из init()
///
void parse_prices(const pugi::xml_node &cur_node, Type type) {
	if (cur_node) {
		reset_prices.at(type).base_price = parse::ReadAttrAsInt(cur_node, "price");
		reset_prices.at(type).add_price = parse::ReadAttrAsInt(cur_node, "price_add");
		reset_prices.at(type).max_price = parse::ReadAttrAsInt(cur_node, "max_price");
	}
}

///
/// Лоад/релоад CONFIG_FILE, релоадится через 'reload <имя файла>'
///
void init() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node main_node = doc.child("reset_stats");
	if (!main_node) {
		snprintf(buf, kMaxStringLength, "...<reset_stats> read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node cur_node = parse::GetChild(main_node, "main_stats");
	parse_prices(cur_node, Type::MAIN_STATS);

	cur_node = parse::GetChild(main_node, "race");
	parse_prices(cur_node, Type::RACE);

	cur_node = parse::GetChild(main_node, "feats");
	parse_prices(cur_node, Type::FEATS);
	cur_node = parse::GetChild(main_node, "religion");
	parse_prices(cur_node, Type::RELIGION);
}

///
/// \return стоимость очередного сброса характеристик type через меню
///
int calc_price(CharData *ch, Type type) {
	int price = reset_prices[type].base_price
		+ ch->get_reset_stats_cnt(type) * reset_prices[type].add_price;
	return std::min(price, reset_prices[type].max_price);
}

///
/// Подготовка чара на резет характеристик type:
/// снятие денег, логирование, запись в карму
///
void reset_stats(CharData *ch, Type type) {
	switch (type) {
		case Type::MAIN_STATS: ch->set_start_stat(G_STR, 0);
			break;
		case Type::RACE: UnsetRaceFeats(ch);
			// выводим на ValidateStats
			ch->set_race(99);
			break;
		case Type::FEATS: ch->real_abils.Feats.reset();
			SetInbornAndRaceFeats(ch);
			break;
		case Type::RELIGION: ch->player_data.Religion = 2; //kReligionMono + 1
			break;
		default: mudlog("SYSERROR: reset_stats() switch", NRM, kLvlImmortal, SYSLOG, true);
			return;
	}

	ch->inc_reset_stats_cnt(type);
	ch->save_char();
}

///
/// Распечатка меню сброса характеристик
///
void print_menu(DescriptorData *d) {
	const int stats_price = calc_price(d->character.get(), Type::MAIN_STATS);
	const int race_price = calc_price(d->character.get(), Type::RACE);
	const int feats_price = calc_price(d->character.get(), Type::FEATS);
	const int religion_price = calc_price(d->character.get(), Type::RELIGION);

	std::string str = fmt::format(
		"{}В случае потери связи процедуру можно будет продолжить при следующем входе в игру.{}\r\n\r\n"
		"1) оплатить {} {} и начать перераспределение стартовых характеристик.\r\n"
		"2) оплатить {} {} и перейти к выбору рода.\r\n"
		"3) оплатить {} {} и сбросить способности (кроме врожденных).\r\n"
		"4) оплатить {} {} и перейти к выбору вероисповедания.\r\n"
		"5) отменить и вернуться в главное меню\r\n"
		"\r\nВаш выбор:",
		kColorBoldGrn, kColorNrm,
		stats_price, GetDeclensionInNumber(stats_price, EWhat::kMoneyA),
		race_price, GetDeclensionInNumber(race_price, EWhat::kMoneyA),
		feats_price, GetDeclensionInNumber(feats_price, EWhat::kMoneyA),
		religion_price, GetDeclensionInNumber(religion_price, EWhat::kMoneyA));
iosystem::write_to_output(str.c_str(), d);
}

///
/// Обработка конкретного типа сброса характеристик из меню
///
void process(DescriptorData *d, Type type) {
	const auto &ch = d->character;
	const int price = calc_price(ch.get(), type);

	if (ch->get_total_gold() < price) {
	iosystem::write_to_output("\r\nУ вас нет такой суммы!\r\n", d);
	iosystem::write_to_output(MENU, d);
		STATE(d) = CON_MENU;
	} else {
		char buf_[kMaxInputLength];
		reset_stats(ch.get(), type);

		if ((type == Type::MAIN_STATS || type == Type::RACE || type == Type::RELIGION)
			&& ValidateStats(d)) {
			// если мы попали сюда, значит чара не вывело на переброс статов
			// после проверки в ValidateStats()
		iosystem::write_to_output("Произошла какая-то ошибка, сообщите богам!\r\n", d);
		iosystem::write_to_output(MENU, d);
			STATE(d) = CON_MENU;
			snprintf(buf_, sizeof(buf_), "%s failed to change %s",
					 d->character->get_name().c_str(), reset_prices.at(type).log_text.c_str());
			mudlog(buf_, NRM, kLvlImmortal, SYSLOG, true);
		} else {
			// в любом другом случае изменения можно считать состояшимися
			snprintf(buf_, sizeof(buf_), "changed %s, price=%d",
					 reset_prices.at(type).log_text.c_str(), price);
			AddKarma(ch.get(), buf_, "auto");

			ch->remove_both_gold(price);
			ch->save_char();

			snprintf(buf_, sizeof(buf_), "%s changed %s, price=%d",
					 ch->get_name().c_str(), reset_prices.at(type).log_text.c_str(), price);
			mudlog(buf_, NRM, kLvlBuilder, SYSLOG, true);
		}

		// при сбросе не через ValidateStats нужно отправлять чара дальше в игру
		if (type == Type::FEATS) {
			const char *message = "\r\nИзменение характеристик выполнено!\r\n";
			if (!check_dupes_email(d)) {
			iosystem::write_to_output(message, d);
				STATE(d) = CON_CLOSE;
				return;
			}
			do_entergame(d);
		iosystem::write_to_output(message, d);
		}
	}
}

///
/// Обработка нажатий в меню сброса характеристик для выбора типа
///
void parse_menu(DescriptorData *d, const char *arg) {
	bool result = false;

	if (arg && a_isdigit(*arg)) {
		const int num = atoi(arg);

		switch (num) {
			case 1: process(d, Type::MAIN_STATS);
				result = true;
				break;
			case 2: process(d, Type::RACE);
				result = true;
				break;
			case 3: process(d, Type::FEATS);
				result = true;
				break;
			case 4: process(d, Type::RELIGION);
				result = true;
				break;
		}
	}

	if (!result) {
	iosystem::write_to_output("Изменение параметров персонажа было отменено.\r\n", d);
	iosystem::write_to_output(MENU, d);
		STATE(d) = CON_MENU;
	}
}

} // namespace ResetStats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
