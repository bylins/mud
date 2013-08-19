// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <sstream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "ext_money.hpp"
#include "screen.h"
#include "db.h"
#include "utils.h"
#include "interpreter.h"
#include "pugixml.hpp"
#include "room.hpp"

namespace
{

int get_message_num(unsigned type);
int get_message_num_u(unsigned type);
int calc_torc_req(int rmrt);

} // namespace

namespace ExtMoney
{

// на все эти переменные смотреть init()
int TORC_EXCH_RATE = 999;
// сколько гривен требуется на соответствующее право морта
int BRONZE_MORT_REQ = 999;
int SILVER_MORT_REQ = 999;
int GOLD_MORT_REQ = 999;
// сколько добавлять к требованиям за каждый морт сверху
int BRONZE_MORT_REQ_ADD_PER_MORT = 999;
int SILVER_MORT_REQ_ADD_PER_MORT = 999;
int GOLD_MORT_REQ_ADD_PER_MORT = 999;
// с какого морта требуются какие гривны
int BRONZE_MORT_NUM = 999;
int SILVER_MORT_NUM = 999;
int GOLD_MORT_NUM = 999;
// дроп гривн с боссов
int GOLD_DROP_LVL = 999;
int GOLD_DROP_AMOUNT = 0;
int GOLD_DROP_AMOUNT_ADD_PER_LVL = 0;
int SILVER_DROP_LVL = 999;
int SILVER_DROP_AMOUNT = 0;
int SILVER_DROP_AMOUNT_ADD_PER_LVL = 0;
int BRONZE_DROP_LVL = 999;
int BRONZE_DROP_AMOUNT = 0;
int BRONZE_DROP_AMOUNT_ADD_PER_LVL = 0;

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

int check_input_amount(CHAR_DATA *ch, int num1, int num2)
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

void torc_exch_parse(CHAR_DATA *ch, const char *arg)
{
	if (!*arg || !isdigit(*arg))
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
		num1 = boost::lexical_cast<int>(param1);
		if (!param2.empty())
		{
			num2 = boost::lexical_cast<int>(param2);
		}
	}
	catch (boost::bad_lexical_cast &)
	{
		log("SYSERROR: bad_lexical_cast arg=%s (%s %s %d)",
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

std::string create_message(CHAR_DATA *ch, int gold, int silver, int bronze)
{
	std::stringstream out;
	int cnt = 0;

	if (gold > 0)
	{
		out << CCIYEL(ch, C_NRM) << gold << " " << desc_count(gold, get_message_num_u(TORC_GOLD));
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
				out << ", " << CCWHT(ch, C_NRM) << silver << " " << desc_count(silver, get_message_num_u(TORC_SILVER))
					<< CCNRM(ch, C_NRM) << " и ";
			}
			else
			{
				out << " и " << CCWHT(ch, C_NRM) << silver << " " << desc_count(silver, get_message_num_u(TORC_SILVER))
					<< " " << desc_count(silver, WHAT_TORCu)
					<< CCNRM(ch, C_NRM);
			}
		}
		else
		{
			if (bronze > 0)
			{
				out << CCWHT(ch, C_NRM) << silver << " " << desc_count(silver, get_message_num_u(TORC_SILVER))
					<< CCNRM(ch, C_NRM) << " и ";
			}
			else
			{
				out << CCWHT(ch, C_NRM) << silver << " " << desc_count(silver, get_message_num_u(TORC_SILVER))
					<< " " << desc_count(silver, WHAT_TORCu)
					<< CCNRM(ch, C_NRM);
			}
		}
	}
	if (bronze > 0)
	{
		out << CCYEL(ch, C_NRM) << bronze << " " << desc_count(bronze, get_message_num_u(TORC_BRONZE))
			<< " " << desc_count(bronze, WHAT_TORCu)
			<< CCNRM(ch, C_NRM);
	}

	return out.str();
}

void gain_torc_process(CHAR_DATA *ch, unsigned type, int drop, int members)
{
	// пересчитываем дроп к минимальному типу
	if (type == TORC_GOLD)
	{
		drop = drop * TORC_EXCH_RATE * TORC_EXCH_RATE;
	}
	else if (type == TORC_SILVER)
	{
		drop = drop * TORC_EXCH_RATE;
	}
	// после этого уже применяем штрафы
	if (members > 1)
	{
		drop = (drop / members) + (drop / 10);
	}

	int gold = 0, silver = 0, bronze = drop;
	// и разносим что осталось обратно по типам
	if (bronze >= TORC_EXCH_RATE * TORC_EXCH_RATE)
	{
		gold += bronze / (TORC_EXCH_RATE * TORC_EXCH_RATE);
		bronze -= gold * TORC_EXCH_RATE * TORC_EXCH_RATE;
	}
	if (bronze >= TORC_EXCH_RATE)
	{
		silver += bronze / TORC_EXCH_RATE;
		bronze -= silver * TORC_EXCH_RATE;
	}

	std::string out = create_message(ch, gold, silver, bronze);
	send_to_char(ch, "В награду за свершенный подвиг вы получили от Богов %s.\r\n", out.c_str());

	ch->set_ext_money(TORC_GOLD, gold + ch->get_ext_money(TORC_GOLD));
	ch->set_ext_money(TORC_SILVER, silver + ch->get_ext_money(TORC_SILVER));
	ch->set_ext_money(TORC_BRONZE, bronze + ch->get_ext_money(TORC_BRONZE));
}

// проверка на случай нескольких физических боссов,
// которые логически являются одной группой, предотвращающая лишний дроп гривен
bool has_connected_bosses(CHAR_DATA *ch)
{
	// если в комнате есть другие живые боссы
	for (CHAR_DATA *i = world[IN_ROOM(ch)]->people;
		i; i = i->next_in_room)
	{
		if (i != ch && IS_NPC(i) && !IS_CHARMICE(i) && i->get_role(MOB_ROLE_BOSS))
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
			&& i->follower->master == ch
			&& i->follower->get_role(MOB_ROLE_BOSS))
		{
			return true;
		}
	}
	// если он сам следует за каким-то боссом
	if (ch->master && ch->master->get_role(MOB_ROLE_BOSS))
	{
		return true;
	}

	return false;
}

// вычисление сколько и каких гривен давать за босса
void gain_torc(CHAR_DATA *ch, CHAR_DATA *victim, int members)
{
	const int zone_lvl = zone_table[mob_index[GET_MOB_RNUM(victim)].zone].mob_level;

	if (zone_lvl >= GOLD_DROP_LVL)
	{
		const int add = zone_lvl - GOLD_DROP_LVL;
		const int drop = GOLD_DROP_AMOUNT + add * GOLD_DROP_AMOUNT_ADD_PER_LVL;
		gain_torc_process(ch, TORC_GOLD, drop, members);
	}
	else if (zone_lvl >= SILVER_DROP_LVL)
	{
		const int add = zone_lvl - SILVER_DROP_LVL;
		const int drop = SILVER_DROP_AMOUNT + add * SILVER_DROP_AMOUNT_ADD_PER_LVL;
		gain_torc_process(ch, TORC_SILVER, drop, members);
	}
	else if (zone_lvl >= BRONZE_DROP_LVL)
	{
		const int add = zone_lvl - BRONZE_DROP_LVL;
		const int drop = BRONZE_DROP_AMOUNT + add * BRONZE_DROP_AMOUNT_ADD_PER_LVL;
		gain_torc_process(ch, TORC_BRONZE, drop, members);
	}
}

void drop_torc(CHAR_DATA *mob)
{
	if (!mob->get_role(MOB_ROLE_BOSS)
		|| has_connected_bosses(mob))
	{
		return;
	}

	log("[Extract char] Checking for torc drop.");

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

	CHAR_DATA *leader = (d->character->master && AFF_FLAGGED(d->character, AFF_GROUP))
		? d->character->master : d->character;

	int members = 1;
	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(mob)
			&& !IS_NPC(f->follower))
		{
			++members;
		}
	}

	if (IN_ROOM(leader) == IN_ROOM(mob)
		&& GET_GOD_FLAG(leader, GF_REMORT)
		&& (GET_UNIQUE(leader) == damager.first
			|| mob->get_attacker(leader, ATTACKER_ROUNDS) >= damager.second / 2))
	{
		gain_torc(leader, mob, members);
	}

	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(mob)
			&& !IS_NPC(f->follower)
			&& GET_GOD_FLAG(f->follower, GF_REMORT)
			&& mob->get_attacker(f->follower, ATTACKER_ROUNDS) >= damager.second / 2)
		{
			gain_torc(f->follower, mob, members);
		}
	}
}

} // namespace ExtMoney

using namespace ExtMoney;

namespace Remort
{

const char *CONFIG_FILE = LIB_MISC"remort.xml";
std::string WHERE_TO_REMORT_STR;

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

	WHERE_TO_REMORT_STR = xmlparse_child_value_str(main_node, "WHERE_TO_REMORT_STR");

	BRONZE_MORT_NUM = xmlparse_child_value_int(main_node, "BRONZE_MORT_NUM");
	SILVER_MORT_NUM = xmlparse_child_value_int(main_node, "SILVER_MORT_NUM");
	GOLD_MORT_NUM = xmlparse_child_value_int(main_node, "GOLD_MORT_NUM");

	BRONZE_MORT_REQ = xmlparse_child_value_int(main_node, "BRONZE_MORT_REQ");
	SILVER_MORT_REQ = xmlparse_child_value_int(main_node, "SILVER_MORT_REQ");
	GOLD_MORT_REQ = xmlparse_child_value_int(main_node, "GOLD_MORT_REQ");

	BRONZE_MORT_REQ_ADD_PER_MORT = xmlparse_child_value_int(main_node, "BRONZE_MORT_REQ_ADD_PER_MORT");
	SILVER_MORT_REQ_ADD_PER_MORT = xmlparse_child_value_int(main_node, "SILVER_MORT_REQ_ADD_PER_MORT");
	GOLD_MORT_REQ_ADD_PER_MORT = xmlparse_child_value_int(main_node, "GOLD_MORT_REQ_ADD_PER_MORT");

	TORC_EXCH_RATE = xmlparse_child_value_int(main_node, "TORC_EXCH_RATE");

	BRONZE_DROP_LVL = xmlparse_child_value_int(main_node, "BRONZE_DROP_LVL");
	BRONZE_DROP_AMOUNT = xmlparse_child_value_int(main_node, "BRONZE_DROP_AMOUNT");
	BRONZE_DROP_AMOUNT_ADD_PER_LVL = xmlparse_child_value_int(main_node, "BRONZE_DROP_AMOUNT_ADD_PER_LVL");

	SILVER_DROP_LVL = xmlparse_child_value_int(main_node, "SILVER_DROP_LVL");
	SILVER_DROP_AMOUNT = xmlparse_child_value_int(main_node, "SILVER_DROP_AMOUNT");
	SILVER_DROP_AMOUNT_ADD_PER_LVL = xmlparse_child_value_int(main_node, "SILVER_DROP_AMOUNT_ADD_PER_LVL");

	GOLD_DROP_LVL = xmlparse_child_value_int(main_node, "GOLD_DROP_LVL");
	GOLD_DROP_AMOUNT = xmlparse_child_value_int(main_node, "GOLD_DROP_AMOUNT");
	GOLD_DROP_AMOUNT_ADD_PER_LVL = xmlparse_child_value_int(main_node, "GOLD_DROP_AMOUNT_ADD_PER_LVL");
}

bool can_remort_now(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_CAN_REMORT)
		|| calc_torc_req(GET_REMORT(ch)) <= 0)
	{
		return true;
	}
	return false;
}

} // namespace Remort

using namespace Remort;

namespace
{

int get_message_num(unsigned type)
{
	int message_num = (type == TORC_GOLD)
		? WHAT_TGOLD : (type == TORC_SILVER)
			? WHAT_TSILVER : WHAT_TBRONZE;
	return message_num;
}

int get_message_num_u(unsigned type)
{
	int message_num = (type == TORC_GOLD)
		? WHAT_TGOLDu : (type == TORC_SILVER)
			? WHAT_TSILVERu : WHAT_TBRONZEu;
	return message_num;
}

void donat_torc(CHAR_DATA *ch, unsigned type, int amount)
{
	const int balance = ch->get_ext_money(type) - amount;
	ch->set_ext_money(type, balance);

	const int message_num = get_message_num(type);

	send_to_char(ch, "Вы пожертвовали %d %s %s.\r\n",
		amount,
		desc_count(amount, message_num),
		desc_count(amount, WHAT_TORC));
}

void message_low_torc(CHAR_DATA *ch, unsigned type, int amount, const char *add_text)
{
	send_to_char(ch,
		"Для подтверждения права на перевоплощение вы должны пожертвовать %d %s %s.\r\n"
		"У вас в данный момент %d %s %s%s\r\n",
		amount,
		desc_count(amount, get_message_num_u(type)),
		desc_count(amount, WHAT_TORC),
		ch->get_ext_money(type),
		desc_count(ch->get_ext_money(type), get_message_num(type)),
		desc_count(ch->get_ext_money(type), WHAT_TORC),
		add_text);
}

int calc_torc_req(int rmrt)
{
	int total = 0;
	if (rmrt >= GOLD_MORT_NUM)
	{
		total = GOLD_MORT_REQ + (rmrt - GOLD_MORT_NUM) * GOLD_MORT_REQ_ADD_PER_MORT;
	}
	else if (rmrt >= SILVER_MORT_NUM)
	{
		total = SILVER_MORT_REQ + (rmrt - SILVER_MORT_NUM) * SILVER_MORT_REQ_ADD_PER_MORT;
	}
	return total;
}

} // namespace

SPECIAL(torc)
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
		else
		{
			// чар еще не жертвовал, от него нужны золотые гривны
			if (GET_REMORT(ch) >= GOLD_MORT_NUM)
			{
				message_low_torc(ch, TORC_GOLD, calc_torc_req(GET_REMORT(ch)), " (команда 'жертвовать').");
				return 1;
			}
			// чар еще не жертвовал, от него нужны серебряные гривны
			if (GET_REMORT(ch) >= SILVER_MORT_NUM)
			{
				message_low_torc(ch, TORC_SILVER,  calc_torc_req(GET_REMORT(ch)), " (команда 'жертвовать').");
				return 1;
			}
		}
	}
	if (CMD_IS("жертвовать"))
	{
		// от чара ничего не требуется
		if (calc_torc_req(GET_REMORT(ch)) <= 0)
		{
			send_to_char(
				"Вам не нужно подтверждать свое право на перевоплощение, просто наберите 'перевоплотиться'.\r\n", ch);
			return 1;
		}
		// чар на этом морте уже жертвовал необходимое кол-во гривен
		if (PRF_FLAGGED(ch, PRF_CAN_REMORT))
		{
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
			return 1;
		}

		bool result = false;
		if (GET_REMORT(ch) >= GOLD_MORT_NUM)
		{
			const int total_req = calc_torc_req(GET_REMORT(ch));
			if (ch->get_ext_money(TORC_GOLD) >= total_req)
			{
				result = true;
				donat_torc(ch, TORC_GOLD, total_req);
			}
			else
			{
				message_low_torc(ch, TORC_GOLD, total_req, ". Попробуйте позже.");
			}
		}
		else if (GET_REMORT(ch) >= SILVER_MORT_NUM)
		{
			const int total_req = calc_torc_req(GET_REMORT(ch));
			if (ch->get_ext_money(TORC_SILVER) >= total_req)
			{
				result = true;
				donat_torc(ch, TORC_SILVER, total_req);
			}
			else
			{
				message_low_torc(ch, TORC_SILVER, total_req, ". Попробуйте позже.");
			}
		}

		if (result)
		{
			CHAR_DATA *mob = reinterpret_cast<CHAR_DATA *>(me);
			std::string name = mob->get_name_str();
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
			SET_BIT(PRF_FLAGS(ch, PRF_CAN_REMORT), PRF_CAN_REMORT);
		}
		return 1;
	}

	return 0;
}
