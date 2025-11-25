// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "ext_money.h"

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "engine/entities/zone.h"

#include <third_party_libs/fmt/include/fmt/format.h>

using namespace ExtMoney;
using namespace Remort;

namespace {

void message_low_torc(CharData *ch, unsigned type, int amount, const char *add_text);

} // namespace

namespace Remort {

const char *CONFIG_FILE = LIB_MISC"remort.xml";
std::string WHERE_TO_REMORT_STR;

int calc_torc_daily(int rmrt);

} // namespace Remort

namespace ExtMoney {

// на все эти переменные смотреть init()
int TORC_EXCH_RATE = 999;
std::map<std::string, std::string> plural_name_currency_map = {
	{"куны", "денег"},
	{"слава", "славы"},
	{"лед", "льда"},
	{"гривны", "гривен"},
	{"ногаты", "ногат"}
};

std::string name_currency_plural(const std::string& name) {
	auto it = plural_name_currency_map.find(name);
	if (it != plural_name_currency_map.end()) {
		return (*it).second;
	}
	return "неизвестной валюты";
}

struct type_node {
	type_node() : MORT_REQ(99), MORT_REQ_ADD_PER_MORT(99), MORT_NUM(99),
				  DROP_LVL(99), DROP_AMOUNT(0), DROP_AMOUNT_ADD_PER_LVL(0), MINIMUM_DAYS(99),
				  DESC_MESSAGE_NUM(EWhat::kDay), DESC_MESSAGE_U_NUM(EWhat::kDay) {};
	// сколько гривен требуется на соответствующее право морта
	int MORT_REQ;
	// сколько добавлять к требованиям за каждый морт сверху
	int MORT_REQ_ADD_PER_MORT;
	// с какого морта требуются какие гривны
	int MORT_NUM;
	// с боссов зон какого мин. среднего уровня
	int DROP_LVL;
	// сколько дропать с базового ср. уровня
	int DROP_AMOUNT;
	// сколько накидывать за каждый уровень выше базового
	int DROP_AMOUNT_ADD_PER_LVL;
	// делитель требования гривен, определяет сколько дней минимум потребуется
	// для набора необходимого на морт кол-ва гривен. например если делитель
	// равен 7, а гривен для след. морта нужно 70 золотых, то за сутки персонажу
	// дропнется не более 10 золотых гривен или их эквивалента
	int MINIMUM_DAYS;
	// для сообщений через desc_count
	EWhat DESC_MESSAGE_NUM;
	EWhat DESC_MESSAGE_U_NUM;
};

// список типов гривен со всеми их параметрами
std::array<type_node, kTotalTypes> type_list;

struct TorcReq {
	TorcReq(int rmrt);
	// тип гривн
	unsigned type;
	// кол-во
	int amount;
};

TorcReq::TorcReq(int rmrt) {
	// type
	if (rmrt >= type_list[kTorcGold].MORT_NUM) {
		type = kTorcGold;
	} else if (rmrt >= type_list[kTorcSilver].MORT_NUM) {
		type = kTorcSilver;
	} else if (rmrt >= type_list[kTorcBronze].MORT_NUM) {
		type = kTorcBronze;
	} else {
		type = kTotalTypes;
	}
	// amount
	if (type != kTotalTypes) {
		amount = type_list[type].MORT_REQ +
			(rmrt - type_list[type].MORT_NUM) * type_list[type].MORT_REQ_ADD_PER_MORT;
	} else {
		amount = 0;
	}
}

// обмен гривн
void torc_exch_menu(CharData *ch);
void parse_inc_exch(CharData *ch, int amount, int num);
void parse_dec_exch(CharData *ch, int amount, int num);
int check_input_amount(CharData *ch, int num1, int num2);
bool check_equal_exch(CharData *ch);
void torc_exch_parse(CharData *ch, const char *arg);
// дроп гривн
std::string create_message(int gold, int silver, int bronze);
bool has_connected_bosses(CharData *ch);
unsigned calc_type_by_zone_lvl(int zone_lvl);
int calc_drop_torc(int zone_lvl, int members);
int check_daily_limit(CharData *ch, int drop);
void gain_torc(CharData *ch, int drop);
void drop_torc(CharData *mob);

} // namespace ExtMoney

namespace ExtMoney {

// распечатка меню обмена гривен ('менять' у глашатая)
void torc_exch_menu(CharData *ch) {
	std::string_view menu{"   {}{}) {}{:<17}{} -> {}{:<17}{} [{} -> {}]\r\n"};
	std::stringstream out;

	out << "\r\n"
		   "   Курсы обмена гривен:\r\n"
		   "   " << TORC_EXCH_RATE << "  бронзовых <-> 1 серебряная\r\n"
									  "   " << TORC_EXCH_RATE << " серебряных <-> 1 золотая\r\n\r\n";

	out << "   Текущий баланс: "
		<< kColorBoldYel << ch->desc->ext_money[kTorcGold] << "з "
		<< kColorWht << ch->desc->ext_money[kTorcSilver] << "с "
		<< kColorYel << ch->desc->ext_money[kTorcBronze] << "б\r\n\r\n";

	out << fmt::format(fmt::runtime(menu),
					   kColorGrn, 1,
					   kColorYel, "Бронзовые гривны", kColorNrm,
					   kColorWht, "Серебряные гривны", kColorNrm,
					   TORC_EXCH_RATE, 1);
	out << fmt::format(fmt::runtime(menu),
					   kColorGrn, 2,
					   kColorWht, "Серебряные гривны", kColorNrm,
					   kColorBoldYel, "Золотые гривны", kColorNrm,
					   TORC_EXCH_RATE, 1);
	out << "\r\n"
		<< fmt::format(fmt::runtime(menu),
					   kColorGrn, 3,
					   kColorBoldYel, "Золотые гривны", kColorNrm,
					   kColorWht, "Серебряные гривны", kColorNrm,
					   1, TORC_EXCH_RATE);
	out << fmt::format(fmt::runtime(menu),
					   kColorGrn, 4,
					   kColorWht, "Серебряные гривны", kColorNrm,
					   kColorYel, "Бронзовые гривны", kColorNrm,
					   1, TORC_EXCH_RATE);

	out << "\r\n"
		   "   <номер действия> - один минимальный обмен указанного вида\r\n"
		   "   <номер действия> <число х> - обмен х имеющихся гривен\r\n\r\n";

	out << kColorGrn << "   5)"
		<< kColorNrm << " Отменить обмен и выйти\r\n"
		<< kColorGrn << "   6)"
		<< kColorNrm << " Подтвердить обмен и выйти\r\n\r\n"
		<< "   Ваш выбор:";

	SendMsgToChar(out.str(), ch);
}

// обмен в сторону больших гривен
void parse_inc_exch(CharData *ch, int amount, int num) {
	int torc_from = kTorcBronze;
	int torc_to = kTorcSilver;
	int torc_rate = TORC_EXCH_RATE;

	if (num == 2) {
		torc_from = kTorcSilver;
		torc_to = kTorcGold;
	}

	if (ch->desc->ext_money[torc_from] < amount) {
		if (ch->desc->ext_money[torc_from] < torc_rate) {
			SendMsgToChar("Нет необходимого количества гривен!\r\n", ch);
		} else {
			amount = ch->desc->ext_money[torc_from] / torc_rate * torc_rate;
			SendMsgToChar(ch, "Количество меняемых гривен уменьшено до %d.\r\n", amount);
			ch->desc->ext_money[torc_from] -= amount;
			ch->desc->ext_money[torc_to] += amount / torc_rate;
			SendMsgToChar(ch, "Произведен обмен: %d -> %d.\r\n", amount, amount / torc_rate);
		}
	} else {
		int real_amount = amount / torc_rate * torc_rate;
		if (real_amount != amount) {
			SendMsgToChar(ch, "Количество меняемых гривен уменьшено до %d.\r\n", real_amount);
		}
		ch->desc->ext_money[torc_from] -= real_amount;
		ch->desc->ext_money[torc_to] += real_amount / torc_rate;
		SendMsgToChar(ch, "Произведен обмен: %d -> %d.\r\n", real_amount, real_amount / torc_rate);
	}
}

// обмен в сторону меньших гривен
void parse_dec_exch(CharData *ch, int amount, int num) {
	int torc_from = kTorcGold;
	int torc_to = kTorcSilver;
	int torc_rate = TORC_EXCH_RATE;

	if (num == 4) {
		torc_from = kTorcSilver;
		torc_to = kTorcBronze;
	}

	if (ch->desc->ext_money[torc_from] < amount) {
		if (ch->desc->ext_money[torc_from] < 1) {
			SendMsgToChar("Нет необходимого количества гривен!\r\n", ch);
		} else {
			amount = ch->desc->ext_money[torc_from];
			SendMsgToChar(ch, "Количество меняемых гривен уменьшено до %d.\r\n", amount);

			ch->desc->ext_money[torc_from] -= amount;
			ch->desc->ext_money[torc_to] += amount * torc_rate;
			SendMsgToChar(ch, "Произведен обмен: %d -> %d.\r\n", amount, amount * torc_rate);
		}
	} else {
		ch->desc->ext_money[torc_from] -= amount;
		ch->desc->ext_money[torc_to] += amount * torc_rate;
		SendMsgToChar(ch, "Произведен обмен: %d -> %d.\r\n", amount, amount * torc_rate);
	}
}

// кол-во меняемых гривен
int check_input_amount(CharData * /*ch*/, int num1, int num2) {
	if ((num1 == 1 || num1 == 2) && num2 < TORC_EXCH_RATE) {
		return TORC_EXCH_RATE;
	} else if ((num1 == 3 || num1 == 4) && num2 < 1) {
		return 1;
	}
	return 0;
}

// проверка после обмена, что ничего лишнего не сгенерили случайно
bool check_equal_exch(CharData *ch) {
	int before = 0, after = 0;
	for (unsigned i = 0; i < kTotalTypes; ++i) {
		if (i == kTorcBronze) {
			before += ch->get_ext_money(i);
			after += ch->desc->ext_money[i];
		}
		if (i == kTorcSilver) {
			before += ch->get_ext_money(i) * TORC_EXCH_RATE;
			after += ch->desc->ext_money[i] * TORC_EXCH_RATE;
		} else if (i == kTorcGold) {
			before += ch->get_ext_money(i) * TORC_EXCH_RATE * TORC_EXCH_RATE;
			after += ch->desc->ext_money[i] * TORC_EXCH_RATE * TORC_EXCH_RATE;
		}
	}
	if (before != after) {
		sprintf(buf, "SYSERROR: Torc exch by %s not equal: %d -> %d",
				GET_NAME(ch), before, after);
		mudlog(buf, DEF, kLvlImmortal, SYSLOG, true);
		return false;
	}
	return true;
}

// парс ввода при обмене гривен
void torc_exch_parse(CharData *ch, const char *arg) {
	if (!*arg || !a_isdigit(*arg)) {
		SendMsgToChar("Неверный выбор!\r\n", ch);
		torc_exch_menu(ch);
		return;
	}

	std::string param2(arg), param1;
	GetOneParam(param2, param1);
	utils::Trim(param2);

	int num1 = 0, num2 = 0;

	try {
		num1 = std::stoi(param1, nullptr, 10);
		if (!param2.empty()) {
			num2 = std::stoi(param2, nullptr, 10);
		}
	}
	catch (const std::invalid_argument &) {
		log("SYSERROR: invalid_argument arg=%s (%s %s %d)",
			arg, __FILE__, __func__, __LINE__);

		SendMsgToChar("Неверный выбор!\r\n", ch);
		torc_exch_menu(ch);
		return;
	}

	int amount = num2;
	if (!param2.empty()) {
		// ввели два числа - проверка пользовательского ввода кол-ва гривен
		int min_amount = check_input_amount(ch, num1, amount);
		if (min_amount > 0) {
			SendMsgToChar(ch, "Минимальное количество гривен для данного обмена: %d.", min_amount);
			torc_exch_menu(ch);
			return;
		}
	} else {
		// ввели одно число - проставляем минимальный обмен
		if (num1 == 1 || num1 == 2) {
			amount = TORC_EXCH_RATE;
		} else if (num1 == 3 || num1 == 4) {
			amount = 1;
		}
	}
	// собсна обмен
	if (num1 == 1 || num1 == 2) {
		parse_inc_exch(ch, amount, num1);
	} else if (num1 == 3 || num1 == 4) {
		parse_dec_exch(ch, amount, num1);
	} else if (num1 == 5) {
		ch->desc->state = EConState::kPlaying;
		SendMsgToChar("Обмен отменен.\r\n", ch);
		return;
	} else if (num1 == 6) {
		if (!check_equal_exch(ch)) {
			SendMsgToChar("Обмен отменен по техническим причинам, обратитесь к Богам.\r\n", ch);
		} else {
			for (unsigned i = 0; i < kTotalTypes; ++i) {
				ch->set_ext_money(i, ch->desc->ext_money[i]);
			}
			ch->desc->state = EConState::kPlaying;
			SendMsgToChar("Обмен произведен.\r\n", ch);
		}
		return;
	} else {
		SendMsgToChar("Неверный выбор!\r\n", ch);
	}
	torc_exch_menu(ch);
}

// формирование сообщения о награде гривнами при смерти босса
// пишет в одну строку о нескольких видах гривен, если таковые были
std::string create_message(int gold, int silver, int bronze) {
	std::stringstream out;
	int cnt = 0;

	if (gold > 0) {
		out << kColorBoldYel << gold << " "
			<< GetDeclensionInNumber(gold, type_list[kTorcGold].DESC_MESSAGE_U_NUM);
		if (silver <= 0 && bronze <= 0) {
			out << " " << GetDeclensionInNumber(gold, EWhat::kTorcU);
		}
		out << kColorNrm;
		++cnt;
	}
	if (silver > 0) {
		if (cnt > 0) {
			if (bronze > 0) {
				out << ", " << kColorWht << silver << " "
					<< GetDeclensionInNumber(silver, type_list[kTorcSilver].DESC_MESSAGE_U_NUM)
					<< kColorNrm << " и ";
			} else {
				out << " и " << kColorWht << silver << " "
					<< GetDeclensionInNumber(silver, type_list[kTorcSilver].DESC_MESSAGE_U_NUM)
					<< " " << GetDeclensionInNumber(silver, EWhat::kTorcU)
					<< kColorNrm;
			}
		} else {
			out << kColorWht << silver << " "
				<< GetDeclensionInNumber(silver, type_list[kTorcSilver].DESC_MESSAGE_U_NUM);
			if (bronze > 0) {
				out << kColorNrm << " и ";
			} else {
				out << " " << GetDeclensionInNumber(silver, EWhat::kTorcU) << kColorNrm;
			}
		}
	}
	if (bronze > 0) {
		out << kColorYel << bronze << " "
			<< GetDeclensionInNumber(bronze, type_list[kTorcBronze].DESC_MESSAGE_U_NUM)
			<< " " << GetDeclensionInNumber(bronze, EWhat::kTorcU)
			<< kColorNrm;
	}

	return out.str();
}

// проверка на случай нескольких физических боссов,
// которые логически являются одной группой, предотвращающая лишний дроп гривен
bool has_connected_bosses(CharData *ch) {
	// если в комнате есть другие живые боссы
	for (const auto i : world[ch->in_room]->people) {
		if (i != ch
			&& i->IsNpc()
			&& !IS_CHARMICE(i)
			&& i->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
			return true;
		}
	}
	// если у данного моба есть живые последователи-боссы
	for (FollowerType *i = ch->followers; i; i = i->next) {
		if (i->follower != ch
			&& i->follower->IsNpc()
			&& !IS_CHARMICE(i->follower)
			&& i->follower->get_master() == ch
			&& i->follower->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
			return true;
		}
	}
	// если он сам следует за каким-то боссом
	if (ch->has_master() && ch->get_master()->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
		return true;
	}

	return false;
}

// выясняет какой тип гривен дропать с зоны
unsigned calc_type_by_zone_lvl(int zone_lvl) {
	if (zone_lvl >= type_list[kTorcGold].DROP_LVL) {
		return kTorcGold;
	} else if (zone_lvl >= type_list[kTorcSilver].DROP_LVL) {
		return kTorcSilver;
	} else if (zone_lvl >= type_list[kTorcBronze].DROP_LVL) {
		return kTorcBronze;
	}
	return kTotalTypes;
}

// возвращает кол-во гривен с босса зоны уровня zone_lvl, поделенных
// с учетом группы members и пересчитанных в бронзу
int calc_drop_torc(int zone_lvl, int members) {
	const unsigned type = calc_type_by_zone_lvl(zone_lvl);
	if (type >= kTotalTypes) {
		return 0;
	}
	const int add = zone_lvl - type_list[type].DROP_LVL;
	int drop = type_list[type].DROP_AMOUNT + add * type_list[type].DROP_AMOUNT_ADD_PER_LVL;

	// пересчитываем дроп к минимальному типу
	if (type == kTorcGold) {
		drop = drop * TORC_EXCH_RATE * TORC_EXCH_RATE;
	} else if (type == kTorcSilver) {
		drop = drop * TORC_EXCH_RATE;
	}

	// есть ли вообще что дропать
	if (drop < members) {
		return 0;
	}

	// после этого уже применяем делитель группы
	if (members > 1) {
		drop = drop / members;
	}

	return drop;
}

// по дефолту отрисовка * за каждую 1/5 от суточного лимита гривен
// если imm_stat == true, то вместо звездочек конкретные цифры тек/макс
std::string draw_daily_limit(CharData *ch, bool imm_stat) {
	const int today_torc = ch->get_today_torc();
	const int torc_req_daily = calc_torc_daily(GetRealRemort(ch));

	TorcReq torc_req(GetRealRemort(ch));
	if (torc_req.type >= kTotalTypes) {
		torc_req.type = kTorcBronze;
	}
	const int daily_torc_limit = torc_req_daily / type_list[torc_req.type].MINIMUM_DAYS;

	std::string out("[");
	if (!imm_stat) {
		for (int i = 1; i <= 5; ++i) {
			if (daily_torc_limit / 5 * i <= today_torc) {
				out += "*";
			} else {
				out += ".";
			}
		}
	} else {
		out += fmt::format("{}/{}", today_torc, daily_torc_limit);
	}
	out += "]";

	return out;
}

// проверка дропа гривен на суточный замакс
int check_daily_limit(CharData *ch, int drop) {
	const int today_torc = ch->get_today_torc();
	const int torc_req_daily = calc_torc_daily(GetRealRemort(ch));

	// из calc_torc_daily в любом случае взялось какое-то число бронзы
	// даже если чар не имеет мортов для требования гривен
	TorcReq torc_req(GetRealRemort(ch));
	if (torc_req.type >= kTotalTypes) {
		torc_req.type = kTorcBronze;
	}
	const int daily_torc_limit = torc_req_daily / type_list[torc_req.type].MINIMUM_DAYS;

	if (today_torc + drop > daily_torc_limit) {
		int add = daily_torc_limit - today_torc;
		if (add > 0) {
			ch->add_today_torc(add);
			return add;
		} else {
			return 0;
		}
	}

	ch->add_today_torc(drop);
	return drop;
}

// процесс дропа гривен конкретному чару
void gain_torc(CharData *ch, int drop) {
	// проверка на индивидуальный суточный замакс гривн
	int bronze = check_daily_limit(ch, drop);
	if (bronze <= 0) {
		return;
	}
	int gold = 0, silver = 0;
	// и разносим что осталось обратно по типам
	if (bronze >= TORC_EXCH_RATE * TORC_EXCH_RATE) {
		gold += bronze / (TORC_EXCH_RATE * TORC_EXCH_RATE);
		bronze -= gold * TORC_EXCH_RATE * TORC_EXCH_RATE;
		ch->set_ext_money(kTorcGold, gold + ch->get_ext_money(kTorcGold));
	}
	if (bronze >= TORC_EXCH_RATE) {
		silver += bronze / TORC_EXCH_RATE;
		bronze -= silver * TORC_EXCH_RATE;
		ch->set_ext_money(kTorcSilver, silver + ch->get_ext_money(kTorcSilver));
	}
	ch->set_ext_money(kTorcBronze, bronze + ch->get_ext_money(kTorcBronze));

	std::string out = create_message(gold, silver, bronze);
	SendMsgToChar(ch, "В награду за свершенный подвиг вы получили от Богов %s.\r\n", out.c_str());

}

// дергается из экстракт_чар, у босса берется макс дамагер, находящийся
// в той же комнате, группе готорого и раскидываются гривны, если есть
// кому раскидывать (флаг EGodFlag::REMORT, проверка на делимость гривен, проверка на
// то, что чар находился в комнате с мобом не менее половины раундов дамагера)
void drop_torc(CharData *mob) {
	if (!mob->get_role(static_cast<unsigned>(EMobClass::kBoss))
		|| has_connected_bosses(mob)) {
		return;
	}

	log("[Extract char] Checking %s for ExtMoney.", mob->get_name().c_str());

	std::pair<int /* uid */, int /* rounds */> damager = mob->get_max_damager_in_room();
	DescriptorData *d = nullptr;
	if (damager.first > 0) {
		d = DescriptorByUid(damager.first);
	}
	if (!d) {
		return;
	}

	CharData *leader = (d->character->has_master() && AFF_FLAGGED(d->character, EAffect::kGroup))
						? d->character->get_master()
						: d->character.get();

	int members = 1;
	for (FollowerType *f = leader->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)
			&& f->follower->in_room == mob->in_room
			&& !f->follower->IsNpc()) {
			++members;
		}
	}

	const int zone_lvl = zone_table[mob_index[GET_MOB_RNUM(mob)].zone].mob_level;
	const int drop = calc_drop_torc(zone_lvl, members);
	if (drop <= 0) {
		return;
	}

	if (leader->in_room == mob->in_room
		&& GET_GOD_FLAG(leader, EGf::kRemort)
		&& (leader->get_uid() == damager.first
			|| mob->get_attacker(leader, ATTACKER_ROUNDS) >= damager.second / 2)) {
		gain_torc(leader, drop);
	}

	for (FollowerType *f = leader->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)
			&& f->follower->in_room == mob->in_room
			&& !f->follower->IsNpc()
			&& GET_GOD_FLAG(f->follower, EGf::kRemort)
			&& mob->get_attacker(f->follower, ATTACKER_ROUNDS) >= damager.second / 2) {
			gain_torc(f->follower, drop);
		}
	}
}

void player_drop_log(CharData *ch, unsigned type, int diff) {
	int total_bronze = ch->get_ext_money(kTorcBronze);
	total_bronze += ch->get_ext_money(kTorcSilver) * TORC_EXCH_RATE;
	total_bronze += ch->get_ext_money(kTorcGold) * TORC_EXCH_RATE * TORC_EXCH_RATE;

	log("ExtMoney: %s%s%d%s, sum=%d",
		ch->get_name().c_str(),
		(diff > 0 ? " +" : " "),
		diff,
		((type == kTorcGold) ? "g" : (type == kTorcSilver) ? "s" : "b"),
		total_bronze);
}

} // namespace ExtMoney

namespace Remort {

// релоадится через 'reload remort.xml'
void init() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node main_node = doc.child("remort");
	if (!main_node) {
		snprintf(buf, kMaxStringLength, "...<remort> read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	WHERE_TO_REMORT_STR = parse::ReadChildValueAsStr(main_node, "WHERE_TO_REMORT_STR");
	TORC_EXCH_RATE = parse::ReadChildValueAsInt(main_node, "TORC_EXCH_RATE");

	type_list[kTorcGold].MORT_NUM = parse::ReadChildValueAsInt(main_node, "GOLD_MORT_NUM");
	type_list[kTorcGold].MORT_REQ = parse::ReadChildValueAsInt(main_node, "GOLD_MORT_REQ");
	type_list[kTorcGold].MORT_REQ_ADD_PER_MORT = parse::ReadChildValueAsInt(main_node, "GOLD_MORT_REQ_ADD_PER_MORT");
	type_list[kTorcGold].DROP_LVL = parse::ReadChildValueAsInt(main_node, "GOLD_DROP_LVL");
	type_list[kTorcGold].DROP_AMOUNT = parse::ReadChildValueAsInt(main_node, "GOLD_DROP_AMOUNT");
	type_list[kTorcGold].DROP_AMOUNT_ADD_PER_LVL = parse::ReadChildValueAsInt(main_node, "GOLD_DROP_AMOUNT_ADD_PER_LVL");
	type_list[kTorcGold].MINIMUM_DAYS = parse::ReadChildValueAsInt(main_node, "GOLD_MINIMUM_DAYS");

	type_list[kTorcSilver].MORT_NUM = parse::ReadChildValueAsInt(main_node, "SILVER_MORT_NUM");
	type_list[kTorcSilver].MORT_REQ = parse::ReadChildValueAsInt(main_node, "SILVER_MORT_REQ");
	type_list[kTorcSilver].MORT_REQ_ADD_PER_MORT = parse::ReadChildValueAsInt(main_node, "SILVER_MORT_REQ_ADD_PER_MORT");
	type_list[kTorcSilver].DROP_LVL = parse::ReadChildValueAsInt(main_node, "SILVER_DROP_LVL");
	type_list[kTorcSilver].DROP_AMOUNT = parse::ReadChildValueAsInt(main_node, "SILVER_DROP_AMOUNT");
	type_list[kTorcSilver].DROP_AMOUNT_ADD_PER_LVL =
		parse::ReadChildValueAsInt(main_node, "SILVER_DROP_AMOUNT_ADD_PER_LVL");
	type_list[kTorcSilver].MINIMUM_DAYS = parse::ReadChildValueAsInt(main_node, "SILVER_MINIMUM_DAYS");

	type_list[kTorcBronze].MORT_NUM = parse::ReadChildValueAsInt(main_node, "BRONZE_MORT_NUM");
	type_list[kTorcBronze].MORT_REQ = parse::ReadChildValueAsInt(main_node, "BRONZE_MORT_REQ");
	type_list[kTorcBronze].MORT_REQ_ADD_PER_MORT = parse::ReadChildValueAsInt(main_node, "BRONZE_MORT_REQ_ADD_PER_MORT");
	type_list[kTorcBronze].DROP_LVL = parse::ReadChildValueAsInt(main_node, "BRONZE_DROP_LVL");
	type_list[kTorcBronze].DROP_AMOUNT = parse::ReadChildValueAsInt(main_node, "BRONZE_DROP_AMOUNT");
	type_list[kTorcBronze].DROP_AMOUNT_ADD_PER_LVL =
		parse::ReadChildValueAsInt(main_node, "BRONZE_DROP_AMOUNT_ADD_PER_LVL");
	type_list[kTorcBronze].MINIMUM_DAYS = parse::ReadChildValueAsInt(main_node, "BRONZE_MINIMUM_DAYS");

	// не из конфига, но инится заодно со всеми
	type_list[kTorcGold].DESC_MESSAGE_NUM = EWhat::kGoldTorc;
	type_list[kTorcSilver].DESC_MESSAGE_NUM = EWhat::kSilverTorc;
	type_list[kTorcBronze].DESC_MESSAGE_NUM = EWhat::kBronzeTorc;
	type_list[kTorcGold].DESC_MESSAGE_U_NUM = EWhat::kGoldTorcU;
	type_list[kTorcSilver].DESC_MESSAGE_U_NUM = EWhat::kSilverTorcU;
	type_list[kTorcBronze].DESC_MESSAGE_U_NUM = EWhat::kBronzeTorcU;
}

// проверка, мешает ли что-то чару уйти в реморт
bool can_remort_now(CharData *ch) {
	if (ch->IsFlagged(EPrf::kCanRemort) || !need_torc(ch)) {
		return true;
	}
	return false;
}

// распечатка переменных из конфига
void show_config(CharData *ch) {
	std::stringstream out;
	out << "&SТекущие значения основных параметров:\r\n"
		<< "WHERE_TO_REMORT_STR = " << WHERE_TO_REMORT_STR << "\r\n"
		<< "TORC_EXCH_RATE = " << TORC_EXCH_RATE << "\r\n"

		<< "GOLD_MORT_NUM = " << type_list[kTorcGold].MORT_NUM << "\r\n"
		<< "GOLD_MORT_REQ = " << type_list[kTorcGold].MORT_REQ << "\r\n"
		<< "GOLD_MORT_REQ_ADD_PER_MORT = " << type_list[kTorcGold].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "GOLD_DROP_LVL = " << type_list[kTorcGold].DROP_LVL << "\r\n"
		<< "GOLD_DROP_AMOUNT = " << type_list[kTorcGold].DROP_AMOUNT << "\r\n"
		<< "GOLD_DROP_AMOUNT_ADD_PER_LVL = " << type_list[kTorcGold].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "GOLD_MINIMUM_DAYS = " << type_list[kTorcGold].MINIMUM_DAYS << "\r\n"

		<< "SILVER_MORT_NUM = " << type_list[kTorcSilver].MORT_NUM << "\r\n"
		<< "SILVER_MORT_REQ = " << type_list[kTorcSilver].MORT_REQ << "\r\n"
		<< "SILVER_MORT_REQ_ADD_PER_MORT = " << type_list[kTorcSilver].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "SILVER_DROP_LVL = " << type_list[kTorcSilver].DROP_LVL << "\r\n"
		<< "SILVER_DROP_AMOUNT = " << type_list[kTorcSilver].DROP_AMOUNT << "\r\n"
		<< "SILVER_DROP_AMOUNT_ADD_PER_LVL = " << type_list[kTorcSilver].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "SILVER_MINIMUM_DAYS = " << type_list[kTorcSilver].MINIMUM_DAYS << "\r\n"

		<< "BRONZE_MORT_NUM = " << type_list[kTorcBronze].MORT_NUM << "\r\n"
		<< "BRONZE_MORT_REQ = " << type_list[kTorcBronze].MORT_REQ << "\r\n"
		<< "BRONZE_MORT_REQ_ADD_PER_MORT = " << type_list[kTorcBronze].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "BRONZE_DROP_LVL = " << type_list[kTorcBronze].DROP_LVL << "\r\n"
		<< "BRONZE_DROP_AMOUNT = " << type_list[kTorcBronze].DROP_AMOUNT << "\r\n"
		<< "BRONZE_DROP_AMOUNT_ADD_PER_LVL = " << type_list[kTorcBronze].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "BRONZE_MINIMUM_DAYS = " << type_list[kTorcBronze].MINIMUM_DAYS << "\r\n";

	SendMsgToChar(out.str(), ch);
}

// возвращает требование гривен на морт в пересчете на бронзу
// для лоу-мортов берется требование бронзы базового уровня
int calc_torc_daily(int rmrt) {
	TorcReq torc_req(rmrt);
	int num = 0;

	if (torc_req.type < kTotalTypes) {
		num = type_list[torc_req.type].MORT_REQ;

		if (torc_req.type == kTorcGold) {
			num = num * TORC_EXCH_RATE * TORC_EXCH_RATE;
		} else if (torc_req.type == kTorcSilver) {
			num = num * TORC_EXCH_RATE;
		}
	} else {
		num = type_list[kTorcBronze].MORT_REQ;
	}

	return num;
}

// проверка, требуется ли от чара жертвовать для реморта
bool need_torc(CharData *ch) {
	TorcReq torc_req(GetRealRemort(ch));

	if (torc_req.type < kTotalTypes && torc_req.amount > 0) {
		return true;
	}

	return false;
}

} // namespace Remort

namespace {

// жертвование гривен
void donat_torc(CharData *ch, const std::string &mob_name, unsigned type, int amount) {
	const int balance = ch->get_ext_money(type) - amount;
	ch->set_ext_money(type, balance);
	ch->SetFlag(EPrf::kCanRemort);

	SendMsgToChar(ch, "Вы пожертвовали %d %s %s.\r\n",
				  amount, GetDeclensionInNumber(amount, type_list[type].DESC_MESSAGE_NUM),
				  GetDeclensionInNumber(amount, EWhat::kTorc));

	std::string name = mob_name;
	name_convert(name);

	SendMsgToChar(ch,
				  "%s оценил ваши заслуги перед князем и народом земли русской и вознес вам хвалу.\r\n"
				  "Вы почувствовали себя значительно опытней.\r\n", name.c_str());

	if (GET_GOD_FLAG(ch, EGf::kRemort)) {
		SendMsgToChar(ch,
					  "%sПоздравляем, вы получили право на перевоплощение!%s\r\n",
					  kColorBoldGrn, kColorNrm);
	} else {
		SendMsgToChar(ch,
					  "Вы подтвердили свое право на следующее перевоплощение,\r\n"
					  "для его совершения вам нужно набрать максимальное количество опыта.\r\n");
	}
}

// дергается как при нехватке гривен, так и при попытке реморта без пожертвований
void message_low_torc(CharData *ch, unsigned type, int amount, const char *add_text) {
	if (type < kTotalTypes) {
		const int money = ch->get_ext_money(type);
		SendMsgToChar(ch,
					  "Для подтверждения права на перевоплощение вы должны пожертвовать %d %s %s.\r\n"
					  "У вас в данный момент %d %s %s%s\r\n",
					  amount,
					  GetDeclensionInNumber(amount, type_list[type].DESC_MESSAGE_U_NUM),
					  GetDeclensionInNumber(amount, EWhat::kTorc),
					  money,
					  GetDeclensionInNumber(money, type_list[type].DESC_MESSAGE_NUM),
					  GetDeclensionInNumber(money, EWhat::kTorc),
					  add_text);
	}
}

} // namespace

// глашатаи
int torc(CharData *ch, void *me, int cmd, char * /*argument*/) {
	if (!ch->desc || ch->IsNpc()) {
		return 0;
	}
	if (CMD_IS("менять") || CMD_IS("обмен") || CMD_IS("обменять")) {
		// олц для обмена гривен в обе стороны
		ch->desc->state = EConState::kTorcExch;
		for (unsigned i = 0; i < kTotalTypes; ++i) {
			ch->desc->ext_money[i] = ch->get_ext_money(i);
		}
		torc_exch_menu(ch);
		return 1;
	}
	if (CMD_IS("перевоплотитьс") || CMD_IS("перевоплотиться")) {
		if (can_remort_now(ch)) {
			// чар уже жертвовал или от него и не требовалось
			return 0;
		} else if (need_torc(ch)) {
			TorcReq torc_req(GetRealRemort(ch));
			// чар еще не жертвовал и от него что-то требуется
			message_low_torc(ch, torc_req.type, torc_req.amount, " (команда 'жертвовать').");
			return 1;
		}
	}
	if (CMD_IS("жертвовать")) {
		if (!need_torc(ch)) {
			// от чара для реморта ничего не требуется
			SendMsgToChar(
				"Вам не нужно подтверждать свое право на перевоплощение, просто наберите 'перевоплотиться'.\r\n", ch);
		} else if (ch->IsFlagged(EPrf::kCanRemort)) {
			// чар на этом морте уже жертвовал необходимое кол-во гривен
			if (GET_GOD_FLAG(ch, EGf::kRemort)) {
				SendMsgToChar(
					"Вы уже подтвердили свое право на перевоплощение, просто наберите 'перевоплотиться'.\r\n", ch);
			} else {
				SendMsgToChar(
					"Вы уже пожертвовали достаточное количество гривен.\r\n"
					"Для совершения перевоплощения вам нужно набрать максимальное количество опыта.\r\n", ch);
			}
		} else {
			// пробуем пожертвовать
			TorcReq torc_req(GetRealRemort(ch));
			if (ch->get_ext_money(torc_req.type) >= torc_req.amount) {
				const CharData *mob = reinterpret_cast<CharData *>(me);
				donat_torc(ch, mob->get_name_str(), torc_req.type, torc_req.amount);
			} else {
				message_low_torc(ch, torc_req.type, torc_req.amount, ". Попробуйте позже.");
			}
		}
		return 1;
	}
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
