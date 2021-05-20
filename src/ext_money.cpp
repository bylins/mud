// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "ext_money.hpp"

#include "screen.h"
#include "pugixml.hpp"
#include "parse.hpp"
#include "zone.table.hpp"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <sstream>

using namespace ExtMoney;
using namespace Remort;

namespace
{

void message_low_torc(CHAR_DATA *ch, unsigned type, int amount, const char *add_text);

} // namespace

namespace Remort
{

const char *CONFIG_FILE = LIB_MISC"remort.xml";
std::string WHERE_TO_REMORT_STR;

void init();
bool can_remort_now(CHAR_DATA *ch);
void show_config(CHAR_DATA *ch);
int calc_torc_daily(int rmrt);
bool need_torc(CHAR_DATA *ch);

} // namespace Remort

namespace ExtMoney
{

// на все эти переменные смотреть init()
int TORC_EXCH_RATE = 999;
std::map<std::string, std::string> plural_name_currency_map = {
	{ "куны" , "денег" },
	{ "слава" , "славы" },
	{ "лед" , "льда" },
	{ "гривны", "гривен" }
};

std::string name_currency_plural(std::string name)
{	
	auto it = plural_name_currency_map.find(name);
	if (it != plural_name_currency_map.end())
	{
		return (*it).second;
	}
	return "неизвестной валюты";
}

struct type_node
{
	type_node() : MORT_REQ(99), MORT_REQ_ADD_PER_MORT(99), MORT_NUM(99),
		DROP_LVL(99), DROP_AMOUNT(0), DROP_AMOUNT_ADD_PER_LVL(0), MINIMUM_DAYS(99),
		DESC_MESSAGE_NUM(0), DESC_MESSAGE_U_NUM(0) {};
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
    int DESC_MESSAGE_NUM;
    int DESC_MESSAGE_U_NUM;
};

// список типов гривен со всеми их параметрами
std::array<type_node, TOTAL_TYPES> type_list;

struct TorcReq
{
	TorcReq(int rmrt);
	// тип гривн
	unsigned type;
	// кол-во
	int amount;
};

TorcReq::TorcReq(int rmrt)
{
	// type
	if (rmrt >= type_list[TORC_GOLD].MORT_NUM)
	{
		type = TORC_GOLD;
	}
	else if (rmrt >= type_list[TORC_SILVER].MORT_NUM)
	{
		type = TORC_SILVER;
	}
	else if (rmrt >= type_list[TORC_BRONZE].MORT_NUM)
	{
		type = TORC_BRONZE;
	}
	else
	{
		type = TOTAL_TYPES;
	}
	// amount
	if (type != TOTAL_TYPES)
	{
		amount = type_list[type].MORT_REQ +
			(rmrt - type_list[type].MORT_NUM) * type_list[type].MORT_REQ_ADD_PER_MORT;
	}
	else
	{
		amount = 0;
	}
}

// обмен гривн
void torc_exch_menu(CHAR_DATA *ch);
void parse_inc_exch(CHAR_DATA *ch, int amount, int num);
void parse_dec_exch(CHAR_DATA *ch, int amount, int num);
int check_input_amount(CHAR_DATA *ch, int num1, int num2);
bool check_equal_exch(CHAR_DATA *ch);
void torc_exch_parse(CHAR_DATA *ch, const char *arg);
// дроп гривн
std::string create_message(CHAR_DATA *ch, int gold, int silver, int bronze);
bool has_connected_bosses(CHAR_DATA *ch);
unsigned calc_type_by_zone_lvl(int zone_lvl);
int calc_drop_torc(int zone_lvl, int members);
int check_daily_limit(CHAR_DATA *ch, int drop);
void gain_torc(CHAR_DATA *ch, int drop);
void drop_torc(CHAR_DATA *mob);

} // namespace ExtMoney

namespace ExtMoney
{

// распечатка меню обмена гривен ('менять' у глашатая)
void torc_exch_menu(CHAR_DATA *ch)
{
	boost::format menu("   %s%d) %s%-17s%s -> %s%-17s%s [%d -> %d]\r\n");
	std::stringstream out;

	out << "\r\n"
		"   Курсы обмена гривен:\r\n"
		"   " << TORC_EXCH_RATE << "  бронзовых <-> 1 серебряная\r\n"
		"   " << TORC_EXCH_RATE << " серебряных <-> 1 золотая\r\n\r\n";

	out << "   Текущий баланс: "
		<< CCIYEL(ch, C_NRM) << ch->desc->ext_money[TORC_GOLD] << "з "
		<< CCWHT(ch, C_NRM) << ch->desc->ext_money[TORC_SILVER] << "с "
		<< CCYEL(ch, C_NRM) << ch->desc->ext_money[TORC_BRONZE] << "б\r\n\r\n";

	out << menu
		% CCGRN(ch, C_NRM) % 1
		% CCYEL(ch, C_NRM) % "Бронзовые гривны" % CCNRM(ch, C_NRM)
		% CCWHT(ch, C_NRM) % "Серебряные гривны" % CCNRM(ch, C_NRM)
		% TORC_EXCH_RATE % 1;
	out << menu
		% CCGRN(ch, C_NRM) % 2
		% CCWHT(ch, C_NRM) % "Серебряные гривны" % CCNRM(ch, C_NRM)
		% CCIYEL(ch, C_NRM) % "Золотые гривны" % CCNRM(ch, C_NRM)
		% TORC_EXCH_RATE % 1;
	out << "\r\n"
		<< menu
		% CCGRN(ch, C_NRM) % 3
		% CCIYEL(ch, C_NRM) % "Золотые гривны" % CCNRM(ch, C_NRM)
		% CCWHT(ch, C_NRM) % "Серебряные гривны" % CCNRM(ch, C_NRM)
		% 1 % TORC_EXCH_RATE;
	out << menu
		% CCGRN(ch, C_NRM) % 4
		% CCWHT(ch, C_NRM) % "Серебряные гривны" % CCNRM(ch, C_NRM)
		% CCYEL(ch, C_NRM) % "Бронзовые гривны" % CCNRM(ch, C_NRM)
		% 1 % TORC_EXCH_RATE;

	out << "\r\n"
		"   <номер действия> - один минимальный обмен указанного вида\r\n"
		"   <номер действия> <число х> - обмен х имеющихся гривен\r\n\r\n";

	out << CCGRN(ch, C_NRM) << "   5)"
		<< CCNRM(ch, C_NRM) << " Отменить обмен и выйти\r\n"
		<< CCGRN(ch, C_NRM) << "   6)"
		<< CCNRM(ch, C_NRM) << " Подтвердить обмен и выйти\r\n\r\n"
		<< "   Ваш выбор:";

	send_to_char(out.str(), ch);
}

// обмен в сторону больших гривен
void parse_inc_exch(CHAR_DATA *ch, int amount, int num)
{
	int torc_from = TORC_BRONZE;
	int torc_to = TORC_SILVER;
	int torc_rate = TORC_EXCH_RATE;

	if (num == 2)
	{
		torc_from = TORC_SILVER;
		torc_to = TORC_GOLD;
	}

	if (ch->desc->ext_money[torc_from] < amount)
	{
		if (ch->desc->ext_money[torc_from] < torc_rate)
		{
			send_to_char("Нет необходимого количества гривен!\r\n", ch);
		}
		else
		{
			amount = ch->desc->ext_money[torc_from] / torc_rate * torc_rate;
			send_to_char(ch, "Количество меняемых гривен уменьшено до %d.\r\n", amount);
			ch->desc->ext_money[torc_from] -= amount;
			ch->desc->ext_money[torc_to] += amount / torc_rate;
			send_to_char(ch, "Произведен обмен: %d -> %d.\r\n", amount, amount / torc_rate);
		}
	}
	else
	{
		int real_amount = amount / torc_rate * torc_rate;
		if (real_amount != amount)
		{
			send_to_char(ch, "Количество меняемых гривен уменьшено до %d.\r\n", real_amount);
		}
		ch->desc->ext_money[torc_from] -= real_amount;
		ch->desc->ext_money[torc_to] += real_amount / torc_rate;
		send_to_char(ch, "Произведен обмен: %d -> %d.\r\n", real_amount, real_amount / torc_rate);
	}
}

// обмен в сторону меньших гривен
void parse_dec_exch(CHAR_DATA *ch, int amount, int num)
{
	int torc_from = TORC_GOLD;
	int torc_to = TORC_SILVER;
	int torc_rate = TORC_EXCH_RATE;

	if (num == 4)
	{
		torc_from = TORC_SILVER;
		torc_to = TORC_BRONZE;
	}

	if (ch->desc->ext_money[torc_from] < amount)
	{
		if (ch->desc->ext_money[torc_from] < 1)
		{
			send_to_char("Нет необходимого количества гривен!\r\n", ch);
		}
		else
		{
			amount = ch->desc->ext_money[torc_from];
			send_to_char(ch, "Количество меняемых гривен уменьшено до %d.\r\n", amount);

			ch->desc->ext_money[torc_from] -= amount;
			ch->desc->ext_money[torc_to] += amount * torc_rate;
			send_to_char(ch, "Произведен обмен: %d -> %d.\r\n", amount, amount * torc_rate);
		}
	}
	else
	{
		ch->desc->ext_money[torc_from] -= amount;
		ch->desc->ext_money[torc_to] += amount * torc_rate;
		send_to_char(ch, "Произведен обмен: %d -> %d.\r\n", amount, amount * torc_rate);
	}
}

// кол-во меняемых гривен
int check_input_amount(CHAR_DATA* /*ch*/, int num1, int num2)
{
	if ((num1 == 1 || num1 == 2) && num2 < TORC_EXCH_RATE)
	{
		return TORC_EXCH_RATE;
	}
	else if ((num1 == 3 || num1 == 4) && num2 < 1)
	{
		return 1;
	}
	return 0;
}

// проверка после обмена, что ничего лишнего не сгенерили случайно
bool check_equal_exch(CHAR_DATA *ch)
{
	int before = 0, after = 0;
	for (unsigned i = 0; i < TOTAL_TYPES; ++i)
	{
		if (i == TORC_BRONZE)
		{
			before += ch->get_ext_money(i);
			after += ch->desc->ext_money[i];
		}
		if (i == TORC_SILVER)
		{
			before += ch->get_ext_money(i) * TORC_EXCH_RATE;
			after += ch->desc->ext_money[i] * TORC_EXCH_RATE;
		}
		else if (i == TORC_GOLD)
		{
			before += ch->get_ext_money(i) * TORC_EXCH_RATE * TORC_EXCH_RATE;
			after += ch->desc->ext_money[i] * TORC_EXCH_RATE * TORC_EXCH_RATE;
		}
	}
	if (before != after)
	{
		sprintf(buf, "SYSERROR: Torc exch by %s not equal: %d -> %d",
			GET_NAME(ch), before, after);
		mudlog(buf, DEF, LVL_IMMORT, SYSLOG, TRUE);
		return false;
	}
	return true;
}

// парс ввода при обмене гривен
void torc_exch_parse(CHAR_DATA *ch, const char *arg)
{
	if (!*arg || !a_isdigit(*arg))
	{
		send_to_char("Неверный выбор!\r\n", ch);
		torc_exch_menu(ch);
		return;
	}

	std::string param2(arg), param1;
	GetOneParam(param2, param1);
	boost::trim(param2);

	int num1 = 0, num2 = 0;

	try
	{
		num1 = std::stoi(param1, nullptr, 10);
		if (!param2.empty())
		{
			num2 = std::stoi(param2, nullptr, 10);
		}
	}
	catch (const std::invalid_argument&)
	{
		log("SYSERROR: invalid_argument arg=%s (%s %s %d)",
			arg, __FILE__, __func__, __LINE__);

		send_to_char("Неверный выбор!\r\n", ch);
		torc_exch_menu(ch);
		return;
	}

	int amount = num2;
	if (!param2.empty())
	{
		// ввели два числа - проверка пользовательского ввода кол-ва гривен
		int min_amount = check_input_amount(ch, num1, amount);
		if (min_amount > 0)
		{
			send_to_char(ch, "Минимальное количество гривен для данного обмена: %d.", min_amount);
			torc_exch_menu(ch);
			return;
		}
	}
	else
	{
		// ввели одно число - проставляем минимальный обмен
		if (num1 == 1 || num1 == 2)
		{
			amount = TORC_EXCH_RATE;
		}
		else if (num1 == 3 || num1 == 4)
		{
			amount = 1;
		}
	}
	// собсна обмен
	if (num1 == 1 || num1 == 2)
	{
		parse_inc_exch(ch, amount, num1);
	}
	else if (num1 == 3 || num1 == 4)
	{
		parse_dec_exch(ch, amount, num1);
	}
	else if (num1 == 5)
	{
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("Обмен отменен.\r\n", ch);
		return;
	}
	else if (num1 == 6)
	{
		if (!check_equal_exch(ch))
		{
			send_to_char("Обмен отменен по техническим причинам, обратитесь к Богам.\r\n", ch);
		}
		else
		{
			for (unsigned i = 0; i < TOTAL_TYPES; ++i)
			{
				ch->set_ext_money(i, ch->desc->ext_money[i]);
			}
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("Обмен произведен.\r\n", ch);
		}
		return;
	}
	else
	{
		send_to_char("Неверный выбор!\r\n", ch);
	}
	torc_exch_menu(ch);
}

// формирование сообщения о награде гривнами при смерти босса
// пишет в одну строку о нескольких видах гривен, если таковые были
std::string create_message(CHAR_DATA *ch, int gold, int silver, int bronze)
{
	std::stringstream out;
	int cnt = 0;

	if (gold > 0)
	{
		out << CCIYEL(ch, C_NRM) << gold << " "
			<< desc_count(gold, type_list[TORC_GOLD].DESC_MESSAGE_U_NUM);
		if (silver <= 0 && bronze <= 0)
		{
			out << " " << desc_count(gold, WHAT_TORCu);
		}
		out << CCNRM(ch, C_NRM);
		++cnt;
	}
	if (silver > 0)
	{
		if (cnt > 0)
		{
			if (bronze > 0)
			{
				out << ", " << CCWHT(ch, C_NRM) << silver << " "
					<< desc_count(silver, type_list[TORC_SILVER].DESC_MESSAGE_U_NUM)
					<< CCNRM(ch, C_NRM) << " и ";
			}
			else
			{
				out << " и " << CCWHT(ch, C_NRM) << silver << " "
					<< desc_count(silver, type_list[TORC_SILVER].DESC_MESSAGE_U_NUM)
					<< " " << desc_count(silver, WHAT_TORCu)
					<< CCNRM(ch, C_NRM);
			}
		}
		else
		{
			out << CCWHT(ch, C_NRM) << silver << " "
				<< desc_count(silver, type_list[TORC_SILVER].DESC_MESSAGE_U_NUM);
			if (bronze > 0)
			{
				out << CCNRM(ch, C_NRM) << " и ";
			}
			else
			{
				out << " " << desc_count(silver, WHAT_TORCu) << CCNRM(ch, C_NRM);
			}
		}
	}
	if (bronze > 0)
	{
		out << CCYEL(ch, C_NRM) << bronze << " "
			<< desc_count(bronze, type_list[TORC_BRONZE].DESC_MESSAGE_U_NUM)
			<< " " << desc_count(bronze, WHAT_TORCu)
			<< CCNRM(ch, C_NRM);
	}

	return out.str();
}

// проверка на случай нескольких физических боссов,
// которые логически являются одной группой, предотвращающая лишний дроп гривен
bool has_connected_bosses(CHAR_DATA *ch)
{
	// если в комнате есть другие живые боссы
	for (const auto i : world[ch->in_room]->people)
	{
		if (i != ch
			&& IS_NPC(i)
			&& !IS_CHARMICE(i)
			&& i->get_role(MOB_ROLE_BOSS))
		{
			return true;
		}
	}
	// если у данного моба есть живые последователи-боссы
	for (follow_type *i = ch->followers; i; i = i->next)
	{
		if (i->follower != ch
			&& IS_NPC(i->follower)
			&& !IS_CHARMICE(i->follower)
			&& i->follower->get_master() == ch
			&& i->follower->get_role(MOB_ROLE_BOSS))
		{
			return true;
		}
	}
	// если он сам следует за каким-то боссом
	if (ch->has_master() && ch->get_master()->get_role(MOB_ROLE_BOSS))
	{
		return true;
	}

	return false;
}

// выясняет какой тип гривен дропать с зоны
unsigned calc_type_by_zone_lvl(int zone_lvl)
{
	if (zone_lvl >= type_list[TORC_GOLD].DROP_LVL)
	{
		return TORC_GOLD;
	}
	else if (zone_lvl >= type_list[TORC_SILVER].DROP_LVL)
	{
		return TORC_SILVER;
	}
	else if (zone_lvl >= type_list[TORC_BRONZE].DROP_LVL)
	{
		return TORC_BRONZE;
	}
	return TOTAL_TYPES;
}

// возвращает кол-во гривен с босса зоны уровня zone_lvl, поделенных
// с учетом группы members и пересчитанных в бронзу
int calc_drop_torc(int zone_lvl, int members)
{
	const unsigned type = calc_type_by_zone_lvl(zone_lvl);
	if (type >= TOTAL_TYPES)
	{
		return 0;
	}
	const int add = zone_lvl - type_list[type].DROP_LVL;
	int drop = type_list[type].DROP_AMOUNT + add * type_list[type].DROP_AMOUNT_ADD_PER_LVL;

	// пересчитываем дроп к минимальному типу
	if (type == TORC_GOLD)
	{
		drop = drop * TORC_EXCH_RATE * TORC_EXCH_RATE;
	}
	else if (type == TORC_SILVER)
	{
		drop = drop * TORC_EXCH_RATE;
	}

	// есть ли вообще что дропать
	if (drop < members)
	{
		return 0;
	}

	// после этого уже применяем делитель группы
	if (members > 1)
	{
		drop = drop / members;
	}

	return drop;
}

// по дефолту отрисовка * за каждую 1/5 от суточного лимита гривен
// если imm_stat == true, то вместо звездочек конкретные цифры тек/макс
std::string draw_daily_limit(CHAR_DATA *ch, bool imm_stat)
{
	const int today_torc = ch->get_today_torc();
	const int torc_req_daily = calc_torc_daily(GET_REMORT(ch));

	TorcReq torc_req(GET_REMORT(ch));
	if (torc_req.type >= TOTAL_TYPES)
	{
		torc_req.type = TORC_BRONZE;
	}
	const int daily_torc_limit = torc_req_daily / type_list[torc_req.type].MINIMUM_DAYS;

	std::string out("[");
	if (!imm_stat)
	{
		for (int i = 1; i <= 5; ++i)
		{
			if (daily_torc_limit / 5 * i <= today_torc)
			{
				out += "*";
			}
			else
			{
				out += ".";
			}
		}
	}
	else
	{
		out += boost::str(boost::format("%d/%d") % today_torc % daily_torc_limit);
	}
	out += "]";

	return out;
}

// проверка дропа гривен на суточный замакс
int check_daily_limit(CHAR_DATA *ch, int drop)
{
	const int today_torc = ch->get_today_torc();
	const int torc_req_daily = calc_torc_daily(GET_REMORT(ch));

	// из calc_torc_daily в любом случае взялось какое-то число бронзы
	// даже если чар не имеет мортов для требования гривен
	TorcReq torc_req(GET_REMORT(ch));
	if (torc_req.type >= TOTAL_TYPES)
	{
		torc_req.type = TORC_BRONZE;
	}
	const int daily_torc_limit = torc_req_daily / type_list[torc_req.type].MINIMUM_DAYS;

	if (today_torc + drop > daily_torc_limit)
	{
		int add = daily_torc_limit - today_torc;
		if (add > 0)
		{
			ch->add_today_torc(add);
			return add;
		}
		else
		{
			return 0;
		}
	}

	ch->add_today_torc(drop);
	return drop;
}

// процесс дропа гривен конкретному чару
void gain_torc(CHAR_DATA *ch, int drop)
{
	// проверка на индивидуальный суточный замакс гривн
	int bronze = check_daily_limit(ch, drop);
	if (bronze <= 0)
	{
		return;
	}
	int gold = 0, silver = 0;
	// и разносим что осталось обратно по типам
	if (bronze >= TORC_EXCH_RATE * TORC_EXCH_RATE)
	{
		gold += bronze / (TORC_EXCH_RATE * TORC_EXCH_RATE);
		bronze -= gold * TORC_EXCH_RATE * TORC_EXCH_RATE;
		ch->set_ext_money(TORC_GOLD, gold + ch->get_ext_money(TORC_GOLD));
	}
	if (bronze >= TORC_EXCH_RATE)
	{
		silver += bronze / TORC_EXCH_RATE;
		bronze -= silver * TORC_EXCH_RATE;
		ch->set_ext_money(TORC_SILVER, silver + ch->get_ext_money(TORC_SILVER));
	}
	ch->set_ext_money(TORC_BRONZE, bronze + ch->get_ext_money(TORC_BRONZE));

	std::string out = create_message(ch, gold, silver, bronze);
	send_to_char(ch, "В награду за свершенный подвиг вы получили от Богов %s.\r\n", out.c_str());

}

// дергается из экстракт_чар, у босса берется макс дамагер, находящийся
// в той же комнате, группе готорого и раскидываются гривны, если есть
// кому раскидывать (флаг GF_REMORT, проверка на делимость гривен, проверка на
// то, что чар находился в комнате с мобом не менее половины раундов дамагера)
void drop_torc(CHAR_DATA *mob)
{
	if (!mob->get_role(MOB_ROLE_BOSS)
		|| has_connected_bosses(mob))
	{
		return;
	}

	log("[Extract char] Checking %s for ExtMoney.", mob->get_name().c_str());

	std::pair<int /* uid */, int /* rounds */> damager = mob->get_max_damager_in_room();
	DESCRIPTOR_DATA *d = 0;
	if (damager.first > 0)
	{
		d = DescByUID(damager.first);
	}
	if (!d)
	{
		return;
	}

	CHAR_DATA *leader = (d->character->has_master() && AFF_FLAGGED(d->character, EAffectFlag::AFF_GROUP))
		? d->character->get_master()
		: d->character.get();

	int members = 1;
	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(mob)
			&& !IS_NPC(f->follower))
		{
			++members;
		}
	}

	const int zone_lvl = zone_table[mob_index[GET_MOB_RNUM(mob)].zone].mob_level;
	const int drop = calc_drop_torc(zone_lvl, members);
	if (drop <= 0)
	{
		return;
	}

	if (IN_ROOM(leader) == IN_ROOM(mob)
		&& GET_GOD_FLAG(leader, GF_REMORT)
		&& (GET_UNIQUE(leader) == damager.first
			|| mob->get_attacker(leader, ATTACKER_ROUNDS) >= damager.second / 2))
	{
		gain_torc(leader, drop);
	}

	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(mob)
			&& !IS_NPC(f->follower)
			&& GET_GOD_FLAG(f->follower, GF_REMORT)
			&& mob->get_attacker(f->follower, ATTACKER_ROUNDS) >= damager.second / 2)
		{
			gain_torc(f->follower, drop);
		}
	}
}

void player_drop_log(CHAR_DATA *ch, unsigned type, int diff)
{
	int total_bronze = ch->get_ext_money(TORC_BRONZE);
	total_bronze += ch->get_ext_money(TORC_SILVER) * TORC_EXCH_RATE;
	total_bronze += ch->get_ext_money(TORC_GOLD) * TORC_EXCH_RATE * TORC_EXCH_RATE;

	log("ExtMoney: %s%s%d%s, sum=%d",
		ch->get_name().c_str(),
		(diff > 0 ? " +" : " "),
		diff,
		((type == TORC_GOLD) ? "g" : (type == TORC_SILVER) ? "s" : "b"),
		total_bronze);
}

} // namespace ExtMoney

namespace Remort
{

// релоадится через 'reload remort.xml'
void init()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

    pugi::xml_node main_node = doc.child("remort");
    if (!main_node)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<remort> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }

	WHERE_TO_REMORT_STR = Parse::child_value_str(main_node, "WHERE_TO_REMORT_STR");
	TORC_EXCH_RATE = Parse::child_value_int(main_node, "TORC_EXCH_RATE");

	type_list[TORC_GOLD].MORT_NUM = Parse::child_value_int(main_node, "GOLD_MORT_NUM");
	type_list[TORC_GOLD].MORT_REQ = Parse::child_value_int(main_node, "GOLD_MORT_REQ");
	type_list[TORC_GOLD].MORT_REQ_ADD_PER_MORT = Parse::child_value_int(main_node, "GOLD_MORT_REQ_ADD_PER_MORT");
	type_list[TORC_GOLD].DROP_LVL = Parse::child_value_int(main_node, "GOLD_DROP_LVL");
	type_list[TORC_GOLD].DROP_AMOUNT = Parse::child_value_int(main_node, "GOLD_DROP_AMOUNT");
	type_list[TORC_GOLD].DROP_AMOUNT_ADD_PER_LVL = Parse::child_value_int(main_node, "GOLD_DROP_AMOUNT_ADD_PER_LVL");
	type_list[TORC_GOLD].MINIMUM_DAYS = Parse::child_value_int(main_node, "GOLD_MINIMUM_DAYS");

	type_list[TORC_SILVER].MORT_NUM = Parse::child_value_int(main_node, "SILVER_MORT_NUM");
	type_list[TORC_SILVER].MORT_REQ = Parse::child_value_int(main_node, "SILVER_MORT_REQ");
	type_list[TORC_SILVER].MORT_REQ_ADD_PER_MORT = Parse::child_value_int(main_node, "SILVER_MORT_REQ_ADD_PER_MORT");
	type_list[TORC_SILVER].DROP_LVL = Parse::child_value_int(main_node, "SILVER_DROP_LVL");
	type_list[TORC_SILVER].DROP_AMOUNT = Parse::child_value_int(main_node, "SILVER_DROP_AMOUNT");
	type_list[TORC_SILVER].DROP_AMOUNT_ADD_PER_LVL = Parse::child_value_int(main_node, "SILVER_DROP_AMOUNT_ADD_PER_LVL");
	type_list[TORC_SILVER].MINIMUM_DAYS = Parse::child_value_int(main_node, "SILVER_MINIMUM_DAYS");

	type_list[TORC_BRONZE].MORT_NUM = Parse::child_value_int(main_node, "BRONZE_MORT_NUM");
	type_list[TORC_BRONZE].MORT_REQ = Parse::child_value_int(main_node, "BRONZE_MORT_REQ");
	type_list[TORC_BRONZE].MORT_REQ_ADD_PER_MORT = Parse::child_value_int(main_node, "BRONZE_MORT_REQ_ADD_PER_MORT");
	type_list[TORC_BRONZE].DROP_LVL = Parse::child_value_int(main_node, "BRONZE_DROP_LVL");
	type_list[TORC_BRONZE].DROP_AMOUNT = Parse::child_value_int(main_node, "BRONZE_DROP_AMOUNT");
	type_list[TORC_BRONZE].DROP_AMOUNT_ADD_PER_LVL = Parse::child_value_int(main_node, "BRONZE_DROP_AMOUNT_ADD_PER_LVL");
	type_list[TORC_BRONZE].MINIMUM_DAYS = Parse::child_value_int(main_node, "BRONZE_MINIMUM_DAYS");

	// не из конфига, но инится заодно со всеми
	type_list[TORC_GOLD].DESC_MESSAGE_NUM = WHAT_TGOLD;
	type_list[TORC_SILVER].DESC_MESSAGE_NUM = WHAT_TSILVER;
	type_list[TORC_BRONZE].DESC_MESSAGE_NUM = WHAT_TBRONZE;
	type_list[TORC_GOLD].DESC_MESSAGE_U_NUM = WHAT_TGOLDu;
	type_list[TORC_SILVER].DESC_MESSAGE_U_NUM = WHAT_TSILVERu;
	type_list[TORC_BRONZE].DESC_MESSAGE_U_NUM = WHAT_TBRONZEu;
}

// проверка, мешает ли что-то чару уйти в реморт
bool can_remort_now(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_CAN_REMORT) || !need_torc(ch))
	{
		return true;
	}
	return false;
}

// распечатка переменных из конфига
void show_config(CHAR_DATA *ch)
{
	std::stringstream out;
	out << "&SТекущие значения основных параметров:\r\n"
		<< "WHERE_TO_REMORT_STR = " << WHERE_TO_REMORT_STR << "\r\n"
		<< "TORC_EXCH_RATE = " << TORC_EXCH_RATE << "\r\n"

		<< "GOLD_MORT_NUM = " << type_list[TORC_GOLD].MORT_NUM << "\r\n"
		<< "GOLD_MORT_REQ = " << type_list[TORC_GOLD].MORT_REQ << "\r\n"
		<< "GOLD_MORT_REQ_ADD_PER_MORT = " << type_list[TORC_GOLD].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "GOLD_DROP_LVL = " << type_list[TORC_GOLD].DROP_LVL << "\r\n"
		<< "GOLD_DROP_AMOUNT = " << type_list[TORC_GOLD].DROP_AMOUNT << "\r\n"
		<< "GOLD_DROP_AMOUNT_ADD_PER_LVL = " << type_list[TORC_GOLD].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "GOLD_MINIMUM_DAYS = " << type_list[TORC_GOLD].MINIMUM_DAYS << "\r\n"

		<< "SILVER_MORT_NUM = " << type_list[TORC_SILVER].MORT_NUM << "\r\n"
		<< "SILVER_MORT_REQ = " << type_list[TORC_SILVER].MORT_REQ << "\r\n"
		<< "SILVER_MORT_REQ_ADD_PER_MORT = " << type_list[TORC_SILVER].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "SILVER_DROP_LVL = " << type_list[TORC_SILVER].DROP_LVL << "\r\n"
		<< "SILVER_DROP_AMOUNT = " << type_list[TORC_SILVER].DROP_AMOUNT << "\r\n"
		<< "SILVER_DROP_AMOUNT_ADD_PER_LVL = " << type_list[TORC_SILVER].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "SILVER_MINIMUM_DAYS = " << type_list[TORC_SILVER].MINIMUM_DAYS << "\r\n"

		<< "BRONZE_MORT_NUM = " << type_list[TORC_BRONZE].MORT_NUM << "\r\n"
		<< "BRONZE_MORT_REQ = " << type_list[TORC_BRONZE].MORT_REQ << "\r\n"
		<< "BRONZE_MORT_REQ_ADD_PER_MORT = " << type_list[TORC_BRONZE].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "BRONZE_DROP_LVL = " << type_list[TORC_BRONZE].DROP_LVL << "\r\n"
		<< "BRONZE_DROP_AMOUNT = " << type_list[TORC_BRONZE].DROP_AMOUNT << "\r\n"
		<< "BRONZE_DROP_AMOUNT_ADD_PER_LVL = " << type_list[TORC_BRONZE].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "BRONZE_MINIMUM_DAYS = " << type_list[TORC_BRONZE].MINIMUM_DAYS << "\r\n";

	send_to_char(out.str(), ch);
}

// возвращает требование гривен на морт в пересчете на бронзу
// для лоу-мортов берется требование бронзы базового уровня
int calc_torc_daily(int rmrt)
{
	TorcReq torc_req(rmrt);
	int num = 0;

	if (torc_req.type < TOTAL_TYPES)
	{
		num = type_list[torc_req.type].MORT_REQ;

		if (torc_req.type == TORC_GOLD)
		{
			num = num * TORC_EXCH_RATE * TORC_EXCH_RATE;
		}
		else if (torc_req.type == TORC_SILVER)
		{
			num = num * TORC_EXCH_RATE;
		}
	}
	else
	{
		num = type_list[TORC_BRONZE].MORT_REQ;
	}

	return num;
}

// проверка, требуется ли от чара жертвовать для реморта
bool need_torc(CHAR_DATA *ch)
{
	TorcReq torc_req(GET_REMORT(ch));

	if (torc_req.type < TOTAL_TYPES && torc_req.amount > 0)
	{
		return true;
	}

	return false;
}

} // namespace Remort

namespace
{

// жертвование гривен
void donat_torc(CHAR_DATA *ch, const std::string &mob_name, unsigned type, int amount)
{
	const int balance = ch->get_ext_money(type) - amount;
	ch->set_ext_money(type, balance);
	PRF_FLAGS(ch).set(PRF_CAN_REMORT);

	send_to_char(ch, "Вы пожертвовали %d %s %s.\r\n",
		amount, desc_count(amount, type_list[type].DESC_MESSAGE_NUM),
		desc_count(amount, WHAT_TORC));

	std::string name = mob_name;
	name_convert(name);

	send_to_char(ch,
		"%s оценил ваши заслуги перед князем и народом земли русской и вознес вам хвалу.\r\n"
		"Вы почувствовали себя значительно опытней.\r\n", name.c_str());

	if (GET_GOD_FLAG(ch, GF_REMORT))
	{
		send_to_char(ch,
			"%sПоздравляем, вы получили право на перевоплощение!%s\r\n",
			CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	}
	else
	{
		send_to_char(ch,
			"Вы подтвердили свое право на следующее перевоплощение,\r\n"
			"для его совершения вам нужно набрать максимальное количество опыта.\r\n");
	}
}

// дергается как при нехватке гривен, так и при попытке реморта без пожертвований
void message_low_torc(CHAR_DATA *ch, unsigned type, int amount, const char *add_text)
{
	if (type < TOTAL_TYPES)
	{
		const int money = ch->get_ext_money(type);
		send_to_char(ch,
			"Для подтверждения права на перевоплощение вы должны пожертвовать %d %s %s.\r\n"
			"У вас в данный момент %d %s %s%s\r\n",
			amount,
			desc_count(amount, type_list[type].DESC_MESSAGE_U_NUM),
			desc_count(amount, WHAT_TORC),
			money,
			desc_count(money, type_list[type].DESC_MESSAGE_NUM),
			desc_count(money, WHAT_TORC),
			add_text);
	}
}

} // namespace

// глашатаи
int torc(CHAR_DATA *ch, void *me, int cmd, char* /*argument*/)
{
	if (!ch->desc || IS_NPC(ch))
	{
		return 0;
	}
	if (CMD_IS("менять") || CMD_IS("обмен") || CMD_IS("обменять"))
	{
		// олц для обмена гривен в обе стороны
		STATE(ch->desc) = CON_TORC_EXCH;
		for (unsigned i = 0; i < TOTAL_TYPES; ++i)
		{
			ch->desc->ext_money[i] = ch->get_ext_money(i);
		}
		torc_exch_menu(ch);
		return 1;
	}
	if (CMD_IS("перевоплотитьс") || CMD_IS("перевоплотиться"))
	{
		if (can_remort_now(ch))
		{
			// чар уже жертвовал или от него и не требовалось
			return 0;
		}
		else if (need_torc(ch))
		{
			TorcReq torc_req(GET_REMORT(ch));
			// чар еще не жертвовал и от него что-то требуется
			message_low_torc(ch, torc_req.type, torc_req.amount, " (команда 'жертвовать').");
			return 1;
		}
	}
	if (CMD_IS("жертвовать"))
	{
		if (!need_torc(ch))
		{
			// от чара для реморта ничего не требуется
			send_to_char(
				"Вам не нужно подтверждать свое право на перевоплощение, просто наберите 'перевоплотиться'.\r\n", ch);
		}
		else if (PRF_FLAGGED(ch, PRF_CAN_REMORT))
		{
			// чар на этом морте уже жертвовал необходимое кол-во гривен
			if (GET_GOD_FLAG(ch, GF_REMORT))
			{
				send_to_char(
					"Вы уже подтвердили свое право на перевоплощение, просто наберите 'перевоплотиться'.\r\n", ch);
			}
			else
			{
				send_to_char(
					"Вы уже пожертвовали достаточное количество гривен.\r\n"
					"Для совершения перевоплощения вам нужно набрать максимальное количество опыта.\r\n", ch);
			}
		}
		else
		{
			// пробуем пожертвовать
			TorcReq torc_req(GET_REMORT(ch));
			if (ch->get_ext_money(torc_req.type) >= torc_req.amount)
			{
				const CHAR_DATA *mob = reinterpret_cast<CHAR_DATA *>(me);
				donat_torc(ch, mob->get_name_str(), torc_req.type, torc_req.amount);
			}
			else
			{
				message_low_torc(ch, torc_req.type, torc_req.amount, ". Попробуйте позже.");
			}
		}
		return 1;
	}
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
