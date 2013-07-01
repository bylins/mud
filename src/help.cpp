// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include <iterator>
#include <sstream>
#include <iomanip>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include "help.hpp"
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "modify.h"
#include "house.h"
#include "sets_drop.hpp"
#include "handler.h"

extern char *help;
void index_boot(int mode);

////////////////////////////////////////////////////////////////////////////////

namespace HelpSystem
{

struct help_node
{
	help_node() : min_level(0), sets_drop_page(false) {};

	// ключ для поиска
	std::string keyword;
	// текст справки
	std::string entry;
	// требуемый уровень для чтения (демигоды могут читать LVL_IMMORT)
	int min_level;
	// для сгенерированных страниц дропа сетов
	// не спамят в иммлог при чтении, выводят перед страницей таймер
	bool sets_drop_page;
};

// справка, подгружаемая из файлов на старте (STATIC)
std::vector<help_node> static_help;
// справка для всего, что нужно часто релоадить
// сеты, сайты дружин, групповые зоны (DYNAMIC)
std::vector<help_node> dynamic_help;
// флаг для проверки необходимости обновления dynamic_help по таймеру раз в минуту
bool need_update = false;

class UserSearch
{
public:
	UserSearch(CHAR_DATA *in_ch)
		: strong(false), stop(false), diff_keys(false), level(0), topic_num(0), curr_topic_num(0)
	{ ch = in_ch; };

	// ищущий чар
	CHAR_DATA *ch;
	// строгий поиск (! на конце)
    bool strong;
    // флаг остановки прохода по спискам справок
    bool stop;
    // флаг наличия двух и более разных топиков в key_list
    // если топик 1 с несколькими дублями ключей - печатается просто этот топик
    // если топиков два и более - печатается список всех ключей
    bool diff_keys;
    // уровень справки для просмотра данным чаром
    int level;
    // номер из х.поисковая_фраза
    int topic_num;
    // счетчик поиска при topic_num != 0
    int curr_topic_num;
    // поисковая фраза
    std::string arg_str;
    // формирующийся список подходящих топиков
    std::vector<std::vector<help_node>::const_iterator> key_list;

	// инициация поиска через search в нужном массиве
    void process(int flag);
    // собственно сам поиск справки в конкретном массиве
    void search(const std::vector<help_node> &cont);
    // распечатка чару когда ничего не нашлось
    void print_not_found() const;
    // распечатка чару конкретного топика справки
    void print_curr_topic(const help_node &node) const;
    // распечатка чару топика или списка кеев
    // в зависимости от состояния key_list и diff_keys
    void print_key_list() const;
};

void add(const std::string key_str, const std::string entry_str, int min_level, Flags add_flag)
{
	if (key_str.empty() || entry_str.empty())
	{
		log("SYSERROR: empty str '%s' -> '%s' (%s %s %d)",
				key_str.c_str(), entry_str.c_str(), __FILE__, __func__, __LINE__ );
		return;
	}

	help_node tmp_node;
	tmp_node.keyword = key_str;
	lower_convert(tmp_node.keyword);
	tmp_node.entry = entry_str;
	tmp_node.min_level = min_level;

	switch(add_flag)
	{
	case STATIC:
		static_help.push_back(tmp_node);
		break;
	case DYNAMIC:
		dynamic_help.push_back(tmp_node);
		break;
	default:
		log("SYSERROR: wrong add_flag = %d (%s %s %d)",
				add_flag, __FILE__, __func__, __LINE__ );
	};
}

void add_sets(const std::string key_str, const std::string entry_str)
{
	if (key_str.empty() || entry_str.empty())
	{
		log("SYSERROR: empty str '%s' -> '%s' (%s %s %d)",
				key_str.c_str(), entry_str.c_str(), __FILE__, __func__, __LINE__ );
		return;
	}

	help_node tmp_node;
	tmp_node.keyword = key_str;
	lower_convert(tmp_node.keyword);
	tmp_node.entry = entry_str;
	tmp_node.sets_drop_page = true;

	dynamic_help.push_back(tmp_node);
}

void init_group_zones()
{
	std::stringstream out;
	for (int rnum = 0, i = 1; rnum <= top_of_zone_table; ++rnum)
	{
		const int group = zone_table[rnum].group;
		if (group > 1)
		{
			out << boost::format("  %2d - %s (гр. %d+).\r\n") % i % zone_table[rnum].name % group;
			++i;
		}
	}
	add("групповыезоны", out.str(), 0, DYNAMIC);
}

void check_update_dynamic()
{
	if (need_update)
	{
		need_update = false;
		reload(DYNAMIC);
	}
}

void reload(Flags flag)
{
	switch(flag)
	{
	case STATIC:
		static_help.clear();
		index_boot(DB_BOOT_HLP);
		// итоговая сортировка массива через дефолтное < для строковых ключей
		std::sort(static_help.begin(), static_help.end(),
			boost::bind(std::less<std::string>(),
				boost::bind(&help_node::keyword, _1),
				boost::bind(&help_node::keyword, _2)));
		break;
	case DYNAMIC:
		dynamic_help.clear();
		SetsDrop::init_xhelp();
		SetsDrop::init_xhelp_full();
		ClanSystem::init_xhelp();
		init_group_zones();
		std::sort(dynamic_help.begin(), dynamic_help.end(),
			boost::bind(std::less<std::string>(),
				boost::bind(&help_node::keyword, _1),
				boost::bind(&help_node::keyword, _2)));
		break;
	default:
		log("SYSERROR: wrong flag = %d (%s %s %d)", flag, __FILE__, __func__, __LINE__ );
	};
}

void reload_all()
{
	reload(STATIC);
	reload(DYNAMIC);
}

bool help_compare(const std::string &arg, const std::string &text, bool strong)
{
	if (strong)
	{
		return arg == text;
	}

	return isname(arg, text.c_str());
}

void UserSearch::process(int flag)
{
	switch(flag)
	{
	case STATIC:
		search(static_help);
		break;
	case DYNAMIC:
		search(dynamic_help);
		break;
	default:
		log("SYSERROR: wrong flag = %d (%s %s %d)", flag, __FILE__, __func__, __LINE__ );
	};
}

void UserSearch::print_not_found() const
{
	snprintf(buf, sizeof(buf), "%s uses command HELP: %s (not found)", GET_NAME(ch), arg_str.c_str());
	mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
	snprintf(buf, sizeof(buf),
			"&WПо вашему запросу '&w%s&W' ничего не было найдено.&n\r\n"
			"\r\n&cИнформация:&n\r\nЕсли применять команду \"справка\" без параметров, будут отображены основные команды,\r\nособенно необходимые новичкам. Кроме того полезно ознакомиться с разделом &CНОВИЧОК&n.\r\n\r\nСправочная система позволяет использовать в запросе индексацию разделов и использовать строгий поиск.\r\n\r\n&cПримеры:&n\r\n\t\"справка 3.защита\"\r\n\t\"справка 4.защита\"\r\n\t\"справка защитаоттьмы\"\r\n\t\"справка защита!\"\r\n\t\"справка 3.защита!\"\r\n\r\nСм. также: &CИСПОЛЬЗОВАНИЕСПРАВКИ&n\r\n",
			arg_str.c_str());
	send_to_char(buf, ch);
}

void UserSearch::print_curr_topic(const help_node &node) const
{
	if (node.sets_drop_page)
	{
		// распечатка таймера до следующего релоада таблицы дропа сетов
		SetsDrop::print_timer_str(ch);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%s uses command HELP: %s (read)", GET_NAME(ch), arg_str.c_str());
		mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
	}
	page_string(ch->desc, node.entry);
}

void UserSearch::print_key_list() const
{
	// конкретный раздел справки
	// печатается если нашлось всего одно вхождение
	// или все вхождения были дублями одного топика с разными ключами
	if (key_list.size() > 0 && (!diff_keys || key_list.size() == 1))
	{
		print_curr_topic(*(key_list[0]));
		return;
	}
	// список найденных топиков
	std::stringstream out;
	out << "&WПо вашему запросу '&w" << arg_str << "&W' найдены следующие разделы справки:&n\r\n\r\n";
	for (unsigned i = 0, count = 1; i < key_list.size(); ++i, ++count)
	{
		out << boost::format("|&C %-23.23s &n|") % key_list[i]->keyword;
		if ((count % 3) == 0)
		{
			out << "\r\n";
		}
	}
	out << "\r\n";

	snprintf(buf, sizeof(buf), "%s uses command HELP: %s (list)", GET_NAME(ch), arg_str.c_str());
	mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
	page_string(ch->desc, out.str());
}

void UserSearch::search(const std::vector<help_node> &cont)
{
	// поиск в сортированном по ключам массиве через lower_bound
	std::vector<help_node>::const_iterator i =
		std::lower_bound(cont.begin(), cont.end(), arg_str,
			boost::bind(std::less<std::string>(),
				boost::bind(&help_node::keyword, _1), _2));

	while (i != cont.end())
	{
		// проверка текущего кея с учетом флага строгости
		if (!help_compare(arg_str, i->keyword, strong))
		{
			return;
		}
		// уровень топика (актуально для статик справки)
		if (level < i->min_level)
		{
			++i;
			continue;
		}
		// key_list заполняется в любом случае, если поиск
		// идет без индекса topic_num, уникальность содержимого
		// для последующих проверок отражается через diff_keys
		for (unsigned k = 0; k < key_list.size(); ++k)
		{
			if (key_list[k]->entry != i->entry)
			{
				diff_keys = true;
				break;
			}
		}

		if (!topic_num)
		{
			key_list.push_back(i);
		}
		else
		{
			++curr_topic_num;
			// нашли запрос вида х.строка
			if (curr_topic_num == topic_num)
			{
				print_curr_topic(*i);
				stop = true;
				return;
			}
		}
		++i;
	}
}

} // namespace HelpSystem

////////////////////////////////////////////////////////////////////////////////

using namespace HelpSystem;

ACMD(do_help)
{
	if (!ch->desc)
	{
		return;
	}

	skip_spaces(&argument);

	// печатаем экран справки если нет аргументов
	if (!*argument)
	{
		page_string(ch->desc, help, 0);
		return;
	}

	UserSearch user_search(ch);
	// trust_level справки для демигодов - LVL_IMMORT
	user_search.level = GET_GOD_FLAG(ch, GF_DEMIGOD) ? LVL_IMMORT : GET_LEVEL(ch);
	// первый аргумент без пробелов, заодно в нижний регистр
	one_argument(argument, arg);
	// Получаем topic_num для индексации топика
	sscanf(arg, "%d.%s", &user_search.topic_num, arg);
	// если последний символ аргумента '!' -- включаем строгий поиск
	if (strlen(arg) > 1 && *(arg + strlen(arg) - 1) == '!')
	{
		user_search.strong = true;
		*(arg + strlen(arg) - 1) = '\0';
	}
	user_search.arg_str = arg;

	// поиск по всем массивам или до стопа по флагу
	for (int i = STATIC; i < TOTAL_NUM && !user_search.stop; ++i)
	{
		user_search.process(i);
	}
	// если вышли по флагу, то чару уже был распечатан найденный топик вида х.строка
	// иначе распечатываем чару что-то в зависимости от состояния key_list
	if (!user_search.stop)
	{
		if (user_search.key_list.empty())
		{
			user_search.print_not_found();
		}
		else
		{
			user_search.print_key_list();
		}
	}
}
