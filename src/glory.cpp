// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "conf.h"
#include "glory.hpp"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "password.hpp"
#include "genchar.h"
#include "screen.h"
#include "top.h"
#include "char.hpp"

extern void add_karma(CHAR_DATA * ch, char const * punish , char * reason);
extern void check_max_hp(CHAR_DATA *ch);

namespace Glory
{

// таймеры вложенной славы
struct glory_time
{
	glory_time() : glory(0), timer(0), stat(0) {};
	int glory; // кол-во славы
	int timer; // сколько тиков осталось
	int stat; // в какой стат вложена (номер G_STR ... G_CHA)
};

typedef boost::shared_ptr<struct glory_time> GloryTimePtr;
typedef std::list<GloryTimePtr> GloryTimeType;

class GloryNode
{
public:
	GloryNode() : free_glory(0), spend_glory(0), denial(0), hide(0), freeze(0) {};
	GloryNode &operator= (const GloryNode&);

	int free_glory; // свободная слава на руках
	int spend_glory; // потраченная слава в статах
	GloryTimeType timers; // таймеры вложенной славы
	int denial; // таймер ограничения на перекладывание вложенной славы
	std::string name; // для топа
	bool hide; // показывать или нет в топе прославленных
	bool freeze; // состояние фриза (таймеры не тикают)
};

typedef boost::shared_ptr<GloryNode> GloryNodePtr;
typedef std::map<long, GloryNodePtr> GloryListType; // first - уид

class spend_glory
{
public:
	spend_glory() : olc_str(0), olc_dex(0), olc_int(0), olc_wis(0), olc_con(0),
			olc_cha(0), olc_add_str(0), olc_add_dex(0), olc_add_int(0), olc_add_wis(0),
			olc_add_con(0), olc_add_cha(0), olc_add_spend_glory(0) {};

	int olc_str; // статы
	int olc_dex;
	int olc_int;
	int olc_wis;
	int olc_con;
	int olc_cha; // --//--
	int olc_add_str; // вложенные статы просто для удобства вывода при редактировании в олц
	int olc_add_dex; // это разница между изначальными статами и текущим вложением, а не фул статы
	int olc_add_int;
	int olc_add_wis;
	int olc_add_con;
	int olc_add_cha; // --//--
	GloryNodePtr olc_node; // копия с общего списка
	int olc_add_spend_glory; // потраченная слава с учетом текущих изменений в олц
	// все изменения статов в олц меняют именно ее, а не spend_glory в olc_node
	// сравнивается с изначальным значением spend_glory в olc_node, чтобы понять чего изменилось

	int check_spend_glory; // это аналог spend_glory в olc_node, но что-бы не смущать себя же
	int check_free_glory; // в последствии, для задачи контрольной суммы до и после олц имеем
	// отдельное поле + поле check_free_glory для free_glory, т.к. в olc_node оно меняется
	// поля нужны, чтобы узнать о факте внешних изменений за время редактирования в олц
};

enum { ADD_GLORY = 1, REMOVE_GLORY, REMOVE_STAT, TRANSFER_GLORY, HIDE_GLORY };

class GloryLog
{
public:
	GloryLog() : type(0), num(0) {};
	int type; // тип записи (1 - плюс слава, 2 - минус слава, 3 - убирание стата, 4 - трансфер, 5 - hide)
	int num; // кол-во славы при +- славы
	std::string karma; // что было записано в карму
};

typedef boost::shared_ptr<GloryLog> GloryLogPtr;
typedef std::multimap<time_t, GloryLogPtr> GloryLogType; // first - время

const int MAX_STATS_BY_GLORY = 10; // сколько статов можно прокинуть славой
const int MAX_GLORY_TIMER = 267840; // таймер вложенной славы (в минутах)
const int DISPLACE_TIMER = 20160; // таймер на переклдывание вложенной славы между статами (в минутах)
GloryListType glory_list; // общий список славы на чарах, в том числе влитой в статы
GloryLogType glory_log; // лог манипуляций со славой

/**
* Копирование всех полей, чтобы не морочить голову с копированием указателей.
*/
GloryNode & GloryNode::operator= (const GloryNode &t)
{
	free_glory = t.free_glory;
	spend_glory = t.spend_glory;
	denial = t.denial;
	name = t.name;
	hide = t.hide;
	freeze = t.freeze;
	timers.clear();
	for (GloryTimeType::const_iterator tm_it = t.timers.begin(); tm_it != t.timers.end(); ++tm_it)
	{
		GloryTimePtr temp_timer(new glory_time);
		temp_timer->stat = (*tm_it)->stat;
		temp_timer->glory = (*tm_it)->glory;
		temp_timer->timer = (*tm_it)->timer;
		timers.push_back(temp_timer);
	}
	return *this;
}

/**
* Загрузка общего списка славы, проверка на валидность чара, славы, сверка мд5 файла.
*/
void load_glory()
{
	const char *glory_file = LIB_PLRSTUFF"glory.lst";
	std::ifstream file(glory_file);
	if (!file.is_open())
	{
		log("Glory: не удалось открыть файл на чтение: %s", glory_file);
		return;
	}

	long all_sum = 0;
	std::string buffer;
	bool checksum = 0;
	while (file >> buffer)
	{
		if (buffer == "<Node>")
		{
			GloryNodePtr temp_node(new GloryNode);
			long uid = 0;
			int free_glory = 0, denial = 0;
			bool hide = 0, freeze = 0;

			if (!(file >> uid >> free_glory >> denial >> hide >> freeze))
			{
				log("Glory: ошибка чтения uid: %ld, free_glory: %d, denial: %d, hide: %d, freeze: %d",
					uid, free_glory, denial, hide, freeze);
				break;
			}
			temp_node->free_glory = free_glory;
			temp_node->denial = denial;
			temp_node->hide = hide;
			temp_node->freeze = freeze;
			all_sum += uid + free_glory + denial + hide + freeze;

			file >> buffer;
			if (buffer != "<Glory>")
			{
				log("Glory: ошибка чтения секции <Glory>");
				break;
			}


			while (file >> buffer)
			{
				if (buffer == "</Glory>") break;

				int glory = 0, stat = 0, timer = 0;

				try
				{
					glory = boost::lexical_cast<int>(buffer);
				}
				catch (boost::bad_lexical_cast &)
				{
					log("Glory: ошибка чтения glory (%s)", buffer.c_str());
					break;
				}
				all_sum += glory;

				if (!(file >> stat >> timer))
				{
					log("Glory: ошибка чтения: %d, %d", stat, timer);
					break;
				}

				GloryTimePtr temp_glory_timers(new glory_time);
				temp_glory_timers->glory = glory;
				temp_glory_timers->timer = timer;
				temp_glory_timers->stat = stat;
				// даже если править файл славы руками - учитываем только первые 10 вложенных статов
				// а если там одним куском не получается обрезать - то и фик с ним, это всеравно был чит
				if (temp_node->spend_glory + glory <= MAX_STATS_BY_GLORY)
				{
					temp_node->timers.push_back(temp_glory_timers);
					temp_node->spend_glory += glory;
				}
				else
					log("Glory: некорректное кол-во вложенных статов, %d %d %d пропущено (uid: %ld).", glory, stat, timer, uid);

				all_sum += stat + timer;
			}
			if (buffer != "</Glory>")
			{
				log("Glory: ошибка чтения секции </Glory>.");
				break;
			}

			file >> buffer;
			if (buffer != "</Node>")
			{
				log("Glory: ошибка чтения </Node>: %s", buffer.c_str());
				break;
			}

			// проверяем есть ли еще такой чар вообще
			std::string name = GetNameByUnique(uid);
			if (name.empty())
			{
				log("Glory: UID %ld - персонажа не существует.", uid);
				continue;
			}
			temp_node->name = name;

			if (!temp_node->free_glory && !temp_node->spend_glory)
			{
				log("Glory: UID %ld - не осталось вложенной или свободной славы.", uid);
				continue;
			}

			// добавляем в список
			glory_list[uid] = temp_node;
		}
		else if (buffer == "<End>")
		{
			// сверочка мд5
			file >> buffer;
			int result = Password::compare_password(buffer, boost::lexical_cast<std::string>(all_sum));
			checksum = 1;
			if (!result)
			{
				// FIXME тут надо другое оповещение, но потом
				log("Glory: несовпадение общей контрольной суммы (%s).", buffer.c_str());
			}
		}
		else
		{
			log("Glory: неизвестный блок при чтении файла: %s", buffer.c_str());
			break;
		}
	}
	if (!checksum)
	{
		// FIXME тут надо другое оповещение, но потом
		log("Glory: проверка общей контрольной суммы не производилась.");
	}
}

/**
* Сохранение общего списка славы, запись мд5 файла.
*/
void save_glory()
{
	const char *glory_file = LIB_PLRSTUFF"glory.backup";
	std::ofstream file(glory_file);
	if (!file.is_open())
	{
		log("Glory: не удалось открыть файл на запись: %s", glory_file);
		return;
	}

	long all_sum = 0;
	for (GloryListType::const_iterator it = glory_list.begin(); it != glory_list.end(); ++it)
	{
		file << "<Node>\n" << it->first << " " << it->second->free_glory << " " << it->second->denial << " " << it->second->hide << " " << it->second->freeze << "\n<Glory>\n";
		all_sum += it->first + it->second->free_glory + it->second->denial + it->second->hide + it->second->freeze;
		for (GloryTimeType::const_iterator gl_it = it->second->timers.begin(); gl_it != it->second->timers.end(); ++gl_it)
		{
			file << (*gl_it)->glory << " " << (*gl_it)->stat << " " << (*gl_it)->timer << "\n";
			all_sum += (*gl_it)->stat + (*gl_it)->glory + (*gl_it)->timer;
		}
		file << "</Glory>\n</Node>\n";
	}
	file << "<End>\n";
	file << Password::generate_md5_hash(boost::lexical_cast<std::string>(all_sum)) << "\n";
	file.close();

	std::string buffer("cp "LIB_PLRSTUFF"glory.backup "LIB_PLRSTUFF"glory.lst");
	system(buffer.c_str());
}

/**
* Аналог бывшего макроса GET_GLORY().
*/
int get_glory(long uid)
{
	int glory = 0;
	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
		glory = it->second->free_glory;
	return glory;
}

/**
* Добавление славы чару, создание новой записи при необходимости, уведомление, если чар онлайн.
*/
void add_glory(long uid, int amount)
{
	if (uid <= 0 || amount <= 0) return;

	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
	{
		it->second->free_glory += amount;
	}
	else
	{
		GloryNodePtr temp_node(new GloryNode);
		temp_node->free_glory = amount;
		glory_list[uid] = temp_node;
	}
	save_glory();
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
		send_to_char(d->character, "Вы заслужили %d %s славы.\r\n",
					 amount, desc_count(amount, WHAT_POINT));
}

/**
* Удаление славы у чара (свободной), ниже нуля быть не может.
* \return 0 - ничего не списано, число > 0 - сколько реально списали
*/
int remove_glory(long uid, int amount)
{
	if (uid <= 0 || amount <= 0) return 0;
	int real_removed = amount;

	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
	{
		// ниже нуля ес-сно не отнимаем
		if (it->second->free_glory >= amount)
			it->second->free_glory -= amount;
		else
		{
			real_removed = it->second->free_glory;
			it->second->free_glory = 0;
		}
		// смысла пустую запись оставлять до ребута нет
		if (!it->second->free_glory && !it->second->spend_glory)
			glory_list.erase(it);

		save_glory();
	}
	return real_removed;
}

/**
* Просто, чтобы не дублировать в разных местах одно и тоже.
*/
void print_denial_message(CHAR_DATA *ch, int denial)
{
	send_to_char(ch, "Вы не сможете изменить уже вложенные очки славы (%s).\r\n", time_format(denial).c_str());
}

/**
* Проверка на запрет перекладывания статов при редактировании в олц.
* \return 0 - запрещено, 1 - разрешено
*/
bool parse_denial_check(CHAR_DATA *ch, int stat)
{
	// при включенном таймере статы ниже уже вложенных не опускаются
	if (ch->desc->glory->olc_node->denial)
	{
		bool stop = 0;
		switch (stat)
		{
		case G_STR:
			if (GET_STR(ch) == ch->desc->glory->olc_str)
				stop = 1;
			break;
		case G_DEX:
			if (GET_DEX(ch) == ch->desc->glory->olc_dex)
				stop = 1;
			break;
		case G_INT:
			if (GET_INT(ch) == ch->desc->glory->olc_int)
				stop = 1;
			break;
		case G_WIS:
			if (GET_WIS(ch) == ch->desc->glory->olc_wis)
				stop = 1;
			break;
		case G_CON:
			if (GET_CON(ch) == ch->desc->glory->olc_con)
				stop = 1;
			break;
		case G_CHA:
			if (GET_CHA(ch) == ch->desc->glory->olc_cha)
				stop = 1;
			break;
		default:
			log("Glory: невалидный номер стат: %d (uid: %d, name: %s)", stat, GET_UNIQUE(ch), GET_NAME(ch));
			stop = 1;
		}
		if (stop)
		{
			print_denial_message(ch, ch->desc->glory->olc_node->denial);
			return 0;
		}
	}
	return 1;
}

/**
* Состояний тут три:
* stat = -1 - перекладывание статов, таймер сохранен, стат ждет своего вложения
* timer = 0 - новый вложенный стат при редактировании
* timer > 0 - вложенный стат, который никто не трогал при редактировании
*/
bool parse_remove_stat(CHAR_DATA *ch, int stat)
{
	if (!parse_denial_check(ch, stat)) return 0;

	for (GloryTimeType::iterator it = ch->desc->glory->olc_node->timers.begin(); it != ch->desc->glory->olc_node->timers.end(); ++it)
	{
		if ((*it)->stat == stat)
		{
			(*it)->glory -= 1;
			// если в стате не осталось плюсов (при перекладывании), то освобождаем поле стата,
			// но оставляем таймер и перемещаем запись в начало списка, чтобы обрабатывать его первым при вложении
			if ((*it)->glory == 0 && (*it)->timer != 0)
			{
				(*it)->stat = -1;
				ch->desc->glory->olc_node->timers.push_front(*it);
				ch->desc->glory->olc_node->timers.erase(it);
			}
			else if ((*it)->glory == 0 && (*it)->timer == 0)
			{
				// держать нулевой в принципе смысла никакого нет
				ch->desc->glory->olc_node->timers.erase(it);
			}
			ch->desc->glory->olc_add_spend_glory -= 1;
			ch->desc->glory->olc_node->free_glory += 1000;
			return 1;
		}
	}
	log("Glory: не найден стат при вычитании в олц (stat: %d, uid: %d, name: %s)", stat, GET_UNIQUE(ch), GET_NAME(ch));
	return 1;
}

/**
* Добавление стата в олц, проверка на свободные записи с включенным таймером, чтобы учесть перекладывание первым.
*/
void parse_add_stat(CHAR_DATA *ch, int stat)
{
	// в любом случае стат добавится
	ch->desc->glory->olc_add_spend_glory += 1;
	ch->desc->glory->olc_node->free_glory -= 1000;

	for (GloryTimeType::const_iterator it = ch->desc->glory->olc_node->timers.begin(); it != ch->desc->glory->olc_node->timers.end(); ++it)
	{
		// если есть какой-то невложенный стат (переливание), то в первую очередь вливаем его,
		// чтобы учесть его таймеры. после перекладки уже идет вливание новой славы
		if ((*it)->stat == -1)
		{
			(*it)->stat = stat;
			(*it)->glory += 1;
			return;
		}

		// если уже есть вложения в тот же стат в текущей сессии редактирования
		// - прибавляем в тоже поле, потому что таймеры будут одинаковые
		if ((*it)->stat == stat && (*it)->timer == 0)
		{
			(*it)->glory += 1;
			return;
		}
	}
	// если стат не найден - делаем новую запись
	GloryTimePtr temp_timer(new glory_time);
	temp_timer->stat = stat;
	temp_timer->glory = 1;
	// вставляем в начало списка, чтобы при вычитании удалять первыми вложенные в текущей сессии статы
	ch->desc->glory->olc_node->timers.push_front(temp_timer);
}

/**
* Парс олц меню 'слава'.
*/
bool parse_spend_glory_menu(CHAR_DATA *ch, char *arg)
{
	switch (*arg)
	{
	case 'А':
	case 'а':
		if (ch->desc->glory->olc_add_str >= 1)
		{
			if (!parse_remove_stat(ch, G_STR)) break;
			ch->desc->glory->olc_str -= 1;
			ch->desc->glory->olc_add_str -= 1;
		}
		break;
	case 'Б':
	case 'б':
		if (ch->desc->glory->olc_add_dex >= 1)
		{
			if (!parse_remove_stat(ch, G_DEX)) break;
			ch->desc->glory->olc_dex -= 1;
			ch->desc->glory->olc_add_dex -= 1;
		}
		break;
	case 'Г':
	case 'г':
		if (ch->desc->glory->olc_add_int >= 1)
		{
			if (!parse_remove_stat(ch, G_INT)) break;
			ch->desc->glory->olc_int -= 1;
			ch->desc->glory->olc_add_int -= 1;
		}
		break;
	case 'Д':
	case 'д':
		if (ch->desc->glory->olc_add_wis >= 1)
		{
			if (!parse_remove_stat(ch, G_WIS)) break;
			ch->desc->glory->olc_wis -= 1;
			ch->desc->glory->olc_add_wis -= 1;
		}
		break;
	case 'Е':
	case 'е':
		if (ch->desc->glory->olc_add_con >= 1)
		{
			if (!parse_remove_stat(ch, G_CON)) break;
			ch->desc->glory->olc_con -= 1;
			ch->desc->glory->olc_add_con -= 1;
		}
		break;
	case 'Ж':
	case 'ж':
		if (ch->desc->glory->olc_add_cha >= 1)
		{
			if (!parse_remove_stat(ch, G_CHA)) break;
			ch->desc->glory->olc_cha -= 1;
			ch->desc->glory->olc_add_cha -= 1;
		}
		break;
	case 'З':
	case 'з':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_STR);
			ch->desc->glory->olc_str += 1;
			ch->desc->glory->olc_add_str += 1;
		}
		break;
	case 'И':
	case 'и':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_DEX);
			ch->desc->glory->olc_dex += 1;
			ch->desc->glory->olc_add_dex += 1;
		}
		break;
	case 'К':
	case 'к':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_INT);
			ch->desc->glory->olc_int += 1;
			ch->desc->glory->olc_add_int += 1;
		}
		break;
	case 'Л':
	case 'л':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_WIS);
			ch->desc->glory->olc_wis += 1;
			ch->desc->glory->olc_add_wis += 1;
		}
		break;
	case 'М':
	case 'м':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_CON);
			ch->desc->glory->olc_con += 1;
			ch->desc->glory->olc_add_con += 1;
		}
		break;
	case 'Н':
	case 'н':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_CHA);
			ch->desc->glory->olc_cha += 1;
			ch->desc->glory->olc_add_cha += 1;
		}
		break;
	case 'В':
	case 'в':
	{
		// проверка, чтобы не записывать зря, а только при изменения
		// и чтобы нельзя было из стата славу вытащить
		if ((ch->desc->glory->olc_str == GET_STR(ch)
				&& ch->desc->glory->olc_dex == GET_DEX(ch)
				&& ch->desc->glory->olc_int == GET_INT(ch)
				&& ch->desc->glory->olc_wis == GET_WIS(ch)
				&& ch->desc->glory->olc_con == GET_CON(ch)
				&& ch->desc->glory->olc_cha == GET_CHA(ch))
				|| ch->desc->glory->olc_add_spend_glory < ch->desc->glory->olc_node->spend_glory)
		{
			return 0;
		}

		GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
		if (it == glory_list.end()
				|| ch->desc->glory->check_spend_glory != it->second->spend_glory
				|| ch->desc->glory->check_free_glory != it->second->free_glory
				|| ch->desc->glory->olc_node->hide != it->second->hide
				|| ch->desc->glory->olc_node->freeze != it->second->freeze)
		{
			// пустое может получиться например если с него в этот момент трансфернули славу
			// остальное - за время сидения в олц что-то изменилось, дали славу, просрочились статы и т.п.
			ch->desc->glory.reset();
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("Редактирование отменено из-за внешних изменений.\r\n", ch);
			return 1;
		}

		// включаем таймер, если было переливание статов
		if (GET_STR(ch) < ch->desc->glory->olc_str
				|| GET_DEX(ch) < ch->desc->glory->olc_dex
				|| GET_INT(ch) < ch->desc->glory->olc_int
				|| GET_WIS(ch) < ch->desc->glory->olc_wis
				|| GET_CON(ch) < ch->desc->glory->olc_con
				|| GET_CHA(ch) < ch->desc->glory->olc_cha)
		{
			ch->desc->glory->olc_node->denial = DISPLACE_TIMER;
		}

		GET_STR(ch) = ch->desc->glory->olc_str;
		GET_DEX(ch) = ch->desc->glory->olc_dex;
		GET_INT(ch) = ch->desc->glory->olc_int;
		GET_WIS(ch) = ch->desc->glory->olc_wis;
		GET_CON(ch) = ch->desc->glory->olc_con;
		GET_CHA(ch) = ch->desc->glory->olc_cha;

		// проставляем таймеры, потому что в олц удобнее иметь нулевые для новых статов
		for (GloryTimeType::const_iterator it = ch->desc->glory->olc_node->timers.begin(); it != ch->desc->glory->olc_node->timers.end(); ++it)
		{
			if ((*it)->timer == 0)
				(*it)->timer = MAX_GLORY_TIMER;
		}
		ch->desc->glory->olc_node->spend_glory = ch->desc->glory->olc_add_spend_glory;
		*(it->second) = *(ch->desc->glory->olc_node);
		save_glory();

		ch->desc->glory.reset();
		STATE(ch->desc) = CON_PLAYING;
		check_max_hp(ch);
		send_to_char("Ваши изменения сохранены.\r\n", ch);
		return 1;
	}
	case 'Х':
	case 'х':
		ch->desc->glory.reset();
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("Редактирование прервано.\r\n", ch);
		return 1;
	default:
		break;
	}
	return 0;
}

/**
* Распечатка олц меню.
*/
void spend_glory_menu(CHAR_DATA *ch)
{
	std::ostringstream out;
	out << "\r\n              -      +\r\n"
	<< "  Сила     : ("
	<< CCIGRN(ch, C_SPR) << "А" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_str << " ("
	<< CCIGRN(ch, C_SPR) << "З" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_str)
		out << "    (" << (ch->desc->glory->olc_add_str > 0 ? "+" : "") << ch->desc->glory->olc_add_str << ")";
	out << "\r\n"
	<< "  Ловкость : ("
	<< CCIGRN(ch, C_SPR) << "Б" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_dex << " ("
	<< CCIGRN(ch, C_SPR) << "И" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_dex)
		out << "    (" << (ch->desc->glory->olc_add_dex > 0 ? "+" : "") << ch->desc->glory->olc_add_dex << ")";
	out << "\r\n"
	<< "  Ум       : ("
	<< CCIGRN(ch, C_SPR) << "Г" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_int << " ("
	<< CCIGRN(ch, C_SPR) << "К" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_int)
		out << "    (" << (ch->desc->glory->olc_add_int > 0 ? "+" : "") << ch->desc->glory->olc_add_int << ")";
	out << "\r\n"
	<< "  Мудрость : ("
	<< CCIGRN(ch, C_SPR) << "Д" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_wis << " ("
	<< CCIGRN(ch, C_SPR) << "Л" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_wis)
		out << "    (" << (ch->desc->glory->olc_add_wis > 0 ? "+" : "") << ch->desc->glory->olc_add_wis << ")";
	out << "\r\n"
	<< "  Здоровье : ("
	<< CCIGRN(ch, C_SPR) << "Е" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_con << " ("
	<< CCIGRN(ch, C_SPR) << "М" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_con)
		out << "    (" << (ch->desc->glory->olc_add_con > 0 ? "+" : "") << ch->desc->glory->olc_add_con << ")";
	out << "\r\n"
	<< "  Обаяние  : ("
	<< CCIGRN(ch, C_SPR) << "Ж" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_cha << " ("
	<< CCIGRN(ch, C_SPR) << "Н" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_cha)
		out << "    (" << (ch->desc->glory->olc_add_cha > 0 ? "+" : "") << ch->desc->glory->olc_add_cha << ")";
	out << "\r\n\r\n"
	<< "  Можно добавить: "
	<< MIN((MAX_STATS_BY_GLORY - ch->desc->glory->olc_add_spend_glory), ch->desc->glory->olc_node->free_glory / 1000)
	<< "\r\n\r\n";

	int diff = ch->desc->glory->olc_node->spend_glory - ch->desc->glory->olc_add_spend_glory;
	if (diff > 0)
	{
		out << "  Вы должны распределить вложенные ранее " << diff << " " << desc_count(diff, WHAT_POINT) << "\r\n";
	}
	else if (ch->desc->glory->olc_add_spend_glory > ch->desc->glory->olc_node->spend_glory
			 || ch->desc->glory->olc_str != GET_STR(ch)
			 || ch->desc->glory->olc_dex != GET_DEX(ch)
			 || ch->desc->glory->olc_int != GET_INT(ch)
			 || ch->desc->glory->olc_wis != GET_WIS(ch)
			 || ch->desc->glory->olc_con != GET_CON(ch)
			 || ch->desc->glory->olc_cha != GET_CHA(ch))
	{
		out << "  "
		<< CCIGRN(ch, C_SPR) << "В" << CCNRM(ch, C_SPR)
		<< ") Сохранить результаты\r\n";
	}

	out << "  "
	<< CCIGRN(ch, C_SPR) << "Х" << CCNRM(ch, C_SPR)
	<< ") Выйти без сохранения\r\n"
	<< " Ваш выбор: ";
	send_to_char(out.str(), ch);
}

/**
* Распечатка 'слава информация'.
*/
void print_glory(CHAR_DATA *ch, GloryListType::iterator &it)
{
	std::ostringstream out;
	for (GloryTimeType::const_iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end(); ++tm_it)
	{
		switch ((*tm_it)->stat)
		{
		case G_STR:
			out << "Сила : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_DEX:
			out << "Подв : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_INT:
			out << "Ум   : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_WIS:
			out << "Мудр : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_CON:
			out << "Тело : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_CHA:
			out << "Обаян: +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		default:
			log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
		}
	}
	out << "Свободных очков: " << it->second->free_glory << "\r\n";
	send_to_char(out.str(), ch);
	return;
}

/**
* Команда 'слава' - вложение имеющейся у игрока славы в статы без участия иммов.
*/
ACMD(do_spend_glory)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end() || IS_IMMORTAL(ch))
	{
		send_to_char("Вам это не нужно...\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	if (CompareParam(buffer2, "информация"))
	{
		send_to_char("Информация о вложенных вами очках славы:\r\n", ch);
		print_glory(ch, it);
		return;
	}

	if (it->second->free_glory < 1000)
	{
		if (!it->second->spend_glory)
		{
			send_to_char("У вас недостаточно очков славы для использования этой команды.\r\n", ch);
			return;
		}
		else if (it->second->denial)
		{
			print_denial_message(ch, it->second->denial);
			return;
		}
	}
	if (it->second->spend_glory >= MAX_STATS_BY_GLORY && it->second->denial)
	{
		send_to_char("У вас уже вложено максимально допустимое количество славы.\r\n", ch);
		print_denial_message(ch, it->second->denial);
		return;
	}

	boost::shared_ptr<class spend_glory> temp_glory(new spend_glory);
	temp_glory->olc_str = GET_STR(ch);
	temp_glory->olc_dex = GET_DEX(ch);
	temp_glory->olc_int = GET_INT(ch);
	temp_glory->olc_wis = GET_WIS(ch);
	temp_glory->olc_con = GET_CON(ch);
	temp_glory->olc_cha = GET_CHA(ch);

	// я не помню уже, как там этот шаред-птр ведет себя при дефолтном копировании
	// а т.к. внутри есть список таймеров на них - скопирую вручную и пофигу на все
	GloryNodePtr olc_node(new GloryNode);
	*olc_node = *(it->second);
	temp_glory->olc_node = olc_node;
	temp_glory->olc_add_spend_glory = it->second->spend_glory;
	// поля для контрольных сверок до и после
	temp_glory->check_spend_glory = it->second->spend_glory;
	temp_glory->check_free_glory = it->second->free_glory;

	for (GloryTimeType::const_iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end(); ++tm_it)
	{
		switch ((*tm_it)->stat)
		{
		case G_STR:
			temp_glory->olc_add_str += (*tm_it)->glory;
			break;
		case G_DEX:
			temp_glory->olc_add_dex += (*tm_it)->glory;
			break;
		case G_INT:
			temp_glory->olc_add_int += (*tm_it)->glory;
			break;
		case G_WIS:
			temp_glory->olc_add_wis += (*tm_it)->glory;
			break;
		case G_CON:
			temp_glory->olc_add_con += (*tm_it)->glory;
			break;
		case G_CHA:
			temp_glory->olc_add_cha += (*tm_it)->glory;
			break;
		default:
			log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
		}
	}

	ch->desc->glory = temp_glory;
	STATE(ch->desc) = CON_SPEND_GLORY;
	spend_glory_menu(ch);
}

/**
* Удаление статов у чара, если он онлайн.
*/
void remove_stat_online(long uid, int stat, int glory)
{
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
	{
		switch (stat)
		{
		case G_STR:
			GET_STR(d->character) -= glory;
			break;
		case G_DEX:
			GET_DEX(d->character) -= glory;
			break;
		case G_INT:
			GET_INT(d->character) -= glory;
			break;
		case G_WIS:
			GET_WIS(d->character) -= glory;
			break;
		case G_CON:
			GET_CON(d->character) -= glory;
			break;
		case G_CHA:
			GET_CHA(d->character) -= glory;
			break;
		default:
			log("Glory: некорректный номер стата %d (uid: %ld)", stat, uid);
		}
	}
}

/**
* Тикание таймеров славы и запрета перекладывания, снятие просроченной славы с чара онлайн.
*/
void timers_update()
{
	for (GloryListType::iterator it = glory_list.begin(); it != glory_list.end(); ++it)
	{
		if (it->second->freeze) continue; // во фризе никакие таймеры не тикают
		bool removed = 0; // флажок для сообщения о пропадании славы (чтобы не спамить на каждый стат)
		for (GloryTimeType::iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end();)
		{
			if ((*tm_it)->timer > 0)
				(*tm_it)->timer -= 1;
			if ((*tm_it)->timer <= 0)
			{
				removed = 1;
				// если чар онлайн - снимаем с него стат
				remove_stat_online(it->first, (*tm_it)->stat, (*tm_it)->glory);
				it->second->spend_glory -= (*tm_it)->glory;
				it->second->timers.erase(tm_it++);
			}
			else
				++tm_it;
		}
		// тут же тикаем ограничением на переливания
		if (it->second->denial > 0)
			it->second->denial -= 1;

		if (removed)
		{
			DESCRIPTOR_DATA *d = DescByUID(it->first);
			if (d) send_to_char("Вы долго не совершали достойных деяний и слава вас покинула...\r\n", d->character);
		}

	}
	save_glory();

	// тут еще есть момент, что в меню таймеры не идут, в случае последующей записи изменений
	// в принципе фигня канеш, но тем не менее - хорошо бы учесть
	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) != CON_SPEND_GLORY || !d->glory) continue;
		for (GloryTimeType::iterator d_it = d->glory->olc_node->timers.begin(); d_it != d->glory->olc_node->timers.end(); ++d_it)
		{
			// здесь мы не тикаем denial и не удаляем просроченные статы, а просто вываливаем из олц
			if ((*d_it)->timer > 0)
			{
				// обрабатываем мы только таймеры > 0, потому что вложенные во время редактирования
				// статы имеют нулевой таймер, а нас интересуют только нулевые после снятия с единицы
				(*d_it)->timer -= 1;
				if ((*d_it)->timer <= 0)
				{
					d->glory.reset();
					STATE(d) = CON_PLAYING;
					send_to_char("Редактирование отменено из-за внешних изменений.\r\n", d->character);
					return;
				}
			}
		}
	}
}

/**
* Стартовые статы при любых условиях должны соответствовать границам ролла.
* В случае старого ролла тут это всплывет из-за нулевых статов.
*/
bool bad_start_stats(CHAR_DATA *ch)
{
	int total_stats = GET_START_STAT(ch, G_STR)
					  + GET_START_STAT(ch, G_DEX)
					  + GET_START_STAT(ch, G_INT)
					  + GET_START_STAT(ch, G_WIS)
					  + GET_START_STAT(ch, G_CON)
					  + GET_START_STAT(ch, G_CHA);
	if (GET_START_STAT(ch, G_STR) > MAX_STR(ch)
			|| GET_START_STAT(ch, G_STR) < MIN_STR(ch)
			|| GET_START_STAT(ch, G_DEX) > MAX_DEX(ch)
			|| GET_START_STAT(ch, G_DEX) < MIN_DEX(ch)
			|| GET_START_STAT(ch, G_INT) > MAX_INT(ch)
			|| GET_START_STAT(ch, G_INT) < MIN_INT(ch)
			|| GET_START_STAT(ch, G_WIS) > MAX_WIS(ch)
			|| GET_START_STAT(ch, G_WIS) < MIN_WIS(ch)
			|| GET_START_STAT(ch, G_CON) > MAX_CON(ch)
			|| GET_START_STAT(ch, G_CON) < MIN_CON(ch)
			|| GET_START_STAT(ch, G_CHA) > MAX_CHA(ch)
			|| GET_START_STAT(ch, G_CHA) < MIN_CHA(ch)
			|| total_stats != SUM_ALL_STATS)
	{
		return 1;
	}
	return 0;
}

/**
* Считаем реальные статы с учетом мортов и влитой славы.
* \return 0 - все ок, любое другое число - все плохо
*/
int bad_real_stats(CHAR_DATA *ch, int check)
{
	check -= SUM_ALL_STATS; // стартовые статы у всех по 95
	check -= 6 * GET_REMORT(ch); // реморты
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it != glory_list.end())
		check -= it->second->spend_glory; // влитая слава
	return check;
}

/**
* Она нам нужна еще после перегенрации чара в интерпретере.
*/
void calculate_total_stats(CHAR_DATA *ch)
{
	// тут прибавляем морты
	if (GET_REMORT(ch))
	{
		GET_STR(ch) += GET_REMORT(ch);
		GET_CON(ch) += GET_REMORT(ch);
		GET_DEX(ch) += GET_REMORT(ch);
		GET_INT(ch) += GET_REMORT(ch);
		GET_WIS(ch) += GET_REMORT(ch);
		GET_CHA(ch) += GET_REMORT(ch);
	}

	// тут прибавляем славу
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end()) return;

	for (GloryTimeType::iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end(); ++tm_it)
	{
		if ((*tm_it)->timer > 0)
		{
			switch ((*tm_it)->stat)
			{
			case G_STR:
				GET_STR(ch) += (*tm_it)->glory;
				break;
			case G_DEX:
				GET_DEX(ch) += (*tm_it)->glory;
				break;
			case G_INT:
				GET_INT(ch) += (*tm_it)->glory;
				break;
			case G_WIS:
				GET_WIS(ch) += (*tm_it)->glory;
				break;
			case G_CON:
				GET_CON(ch) += (*tm_it)->glory;
				break;
			case G_CHA:
				GET_CHA(ch) += (*tm_it)->glory;
				break;
			default:
				log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
			}
		}
	}
}

/**
* Проверка стартовых и итоговых статов.
* Если невалидные стартовые статы - чар отправляется на реролл.
* Если невалидные только итоговые статы - идет перезапись со стартовых с учетом мортов и славы.
*/
bool check_stats(CHAR_DATA *ch)
{
	if (IS_IMMORTAL(ch)) return 1; // иммов травмировать не стоит

	int have_stats = GET_STR(ch) + GET_DEX(ch) + GET_INT(ch) + GET_WIS(ch) + GET_CON(ch) + GET_CHA(ch);

	// чар со старым роллом статов или после попыток поправить статы в файле
	if (bad_start_stats(ch))
	{
		sprintf(buf, "\r\n%sВаши параметры за вычетом перевоплощений:\r\n"
				"Сила: %d, Ловкость: %d, Ум: %d, Мудрость: %d, Телосложение: %d, Обаяние: %d\r\n"
				"Если вы долго отсутствовали в игре, то изменения, касающиеся стартовых параметров были следующие:%s\r\n"
				"\r\n"
				"\tДобавлено ограничение на максимальный класс защиты:\r\n"
				"\tВоры, наемники и дружинники - -250\r\n"
				"\tКупцы, богатыри, витязи, охотники, кузнецы - -200\r\n"
				"\tЛекари, волхвы - -150\r\n"
				"\tКудесники, чернокнижники, колдуны, волшебники - -100\r\n"
				"\r\n"
				"\tТелосложение: изменились коэффициенты профессий и максимальное родное тело (50) в расчетах при\r\n"
				"\tполучении уровня, поэтому изменены границы стартового телосложения у некоторых профессий,\r\n"
				"\tв целом это увеличивает кол-во жизней персонажа тем сильнее, чем больше у него было ремортов.\r\n"
				"\r\n",
				CCIGRN(ch, C_SPR),
				GET_STR(ch) - GET_REMORT(ch),
				GET_DEX(ch) - GET_REMORT(ch),
				GET_INT(ch) - GET_REMORT(ch),
				GET_WIS(ch) - GET_REMORT(ch),
				GET_CON(ch) - GET_REMORT(ch),
				GET_CHA(ch) - GET_REMORT(ch),
				CCNRM(ch, C_SPR));
		SEND_TO_Q(buf, ch->desc);

		// данную фигню мы делаем для того, чтобы из ролла нельзя было случайно так просто выйти
		// сразу, не раскидав статы, а то много любителей тригов и просто нажатий не глядя
		GET_STR(ch) = MIN_STR(ch);
		GET_DEX(ch) = MIN_DEX(ch);
		GET_INT(ch) = MIN_INT(ch);
		GET_WIS(ch) = MIN_WIS(ch);
		GET_CON(ch) = MIN_CON(ch);
		GET_CHA(ch) = MIN_CHA(ch);

		sprintf(buf, "%sПросим вас заново распределить основные параметры персонажа.%s\r\n",
				CCIGRN(ch, C_SPR), CCNRM(ch, C_SPR));
		SEND_TO_Q(buf, ch->desc);
		SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", ch->desc);
		STATE(ch->desc) = CON_RESET_STATS;
		return 0;
	}
	// стартовые статы в поряде, но слава не сходится (снялась по времени или иммом)
	if (bad_real_stats(ch, have_stats))
	{
		// проставляем стартовые статы
		GET_STR(ch) = GET_START_STAT(ch, G_STR);
		GET_DEX(ch) = GET_START_STAT(ch, G_DEX);
		GET_INT(ch) = GET_START_STAT(ch, G_INT);
		GET_WIS(ch) = GET_START_STAT(ch, G_WIS);
		GET_CON(ch) = GET_START_STAT(ch, G_CON);
		GET_CHA(ch) = GET_START_STAT(ch, G_CHA);
		// и все нужные плюсеги сверху
		calculate_total_stats(ch);
	}
	return 1;
}

/**
* Удаление иммом у чара славы, уже вложенной в статы (glory remove).
* \return 0 - ничего не снято, 1 - снято сколько просили
*/
bool remove_stats(CHAR_DATA *ch, CHAR_DATA *god, int amount)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "У %s нет вложенной славы.\r\n", GET_PAD(ch, 1));
		return 0;
	}
	if (amount > it->second->spend_glory)
	{
		send_to_char(god, "У %s нет столько вложенной славы.\r\n", GET_PAD(ch, 1));
		return 0;
	}
	if (amount <= 0)
	{
		send_to_char(god, "Некорректное количество статов (%d).\r\n", amount);
		return 0;
	}

	int removed = 0;
	// собсна не заморачиваясь просто режем первые попавшиеся статы
	for (GloryTimeType::iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end();)
	{
		if (amount > 0)
		{
			if ((*tm_it)->glory > amount)
			{
				// если в поле за раз вложено больше, чем снимаемая слава
				(*tm_it)->glory -= amount;
				it->second->spend_glory -= amount;
				removed += amount;
				// если чар онлайн - снимаем с него статы
				remove_stat_online(it->first, (*tm_it)->stat, amount);
				break;
			}
			else
			{
				// если вложено меньше - снимаем со скольких нужно
				amount -= (*tm_it)->glory;
				it->second->spend_glory -= (*tm_it)->glory;
				removed += (*tm_it)->glory;
				// если чар онлайн - снимаем с него статы
				remove_stat_online(it->first, (*tm_it)->stat, (*tm_it)->glory);
				it->second->timers.erase(tm_it++);
			}
		}
		else
			break;
	}
	imm_log("(GC) %s sets -%d stats to %s.", GET_NAME(god), removed, GET_NAME(ch));
	send_to_char(god, "С %s снято %d %s вложенной ранее славы.\r\n", GET_PAD(ch, 1), removed, desc_count(removed, WHAT_POINT));
	// надо пересчитать хп на случай снятия с тела
	check_max_hp(ch);
	save_glory();
	return 1;
}

/**
* Пересчет статов чара на основании стартовых статов, ремортов и славы.
*/
void recalculate_stats(CHAR_DATA *ch)
{
	GET_STR(ch) = GET_START_STAT(ch, G_STR);
	GET_DEX(ch) = GET_START_STAT(ch, G_DEX);
	GET_INT(ch) = GET_START_STAT(ch, G_INT);
	GET_WIS(ch) = GET_START_STAT(ch, G_WIS);
	GET_CON(ch) = GET_START_STAT(ch, G_CON);
	GET_CHA(ch) = GET_START_STAT(ch, G_CHA);
	calculate_total_stats(ch);
}

/**
* Трансфер 'вчистую', т.е. у принимаюего чара все обнуляем (кроме свободной славы) и сетим все,
* что было у передающего (свободная слава плюсуется).
*/
void transfer_stats(CHAR_DATA *ch, CHAR_DATA *god, std::string name, char *reason)
{
	if (IS_IMMORTAL(ch))
	{
		send_to_char(god, "Трансфер славы с бессмертных на других персонажей запрещен.\r\n");
		return;
	}

	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "У %s нет славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	if (it->second->denial)
	{
		send_to_char(god, "У %s активен таймер, запрещающий переброску вложенных очков славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	long vict_uid = GetUniqueByName(name);
	if (!vict_uid)
	{
		send_to_char(god, "Некорректное имя персонажа (%s), принимающего славу.\r\n", name.c_str());
		return;
	}

	DESCRIPTOR_DATA *d_vict = DescByUID(vict_uid);
	CHAR_DATA *vict = 0, *t_vict = 0;
	if (d_vict)
		vict = d_vict->character;
	else
	{
		// принимающий оффлайн
		t_vict = new CHAR_DATA; // TODO: переделать на стек
		if (load_char(name.c_str(), t_vict) < 0)
		{
			send_to_char(god, "Некорректное имя персонажа (%s), принимающего славу.\r\n", name.c_str());
			delete t_vict;
			return;
		}
		vict = t_vict;
	}

	// дальше у нас принимающий vict в любом случае
	if (IS_IMMORTAL(vict))
	{
		send_to_char(god, "Трансфер славы на бессмертного - это глупо.\r\n");
		if (t_vict) delete t_vict;
		return;
	}
	if (str_cmp(GET_EMAIL(ch), GET_EMAIL(vict)))
	{
		send_to_char(god, "Персонажи имеют разные email адреса.\r\n");
		if (t_vict) delete t_vict;
		return;
	}

	int free_glory = 0;
	// ищем запись принимающего, если таковая есть - обнуляем ее по полной программе
	GloryListType::iterator vict_it = glory_list.find(vict_uid);
	if (vict_it != glory_list.end())
	{
		free_glory = vict_it->second->free_glory;
		glory_list.erase(vict_it);
		// и выставляем ему новые статы, если он онлайн
		DESCRIPTOR_DATA *k = DescByUID(vict_uid);
		if (k)
		{
			// стартовые статы полюбому должны быть валидными, раз он уже в игре
			recalculate_stats(k->character);
		}
	}

	// создаем новую запись...
	GloryNodePtr temp_node(new GloryNode);
	glory_list[vict_uid] = temp_node;
	// и берем новый итератор
	vict_it = glory_list.find(vict_uid);
	// тут перекинутся все поля, только имя нужно вставить свое
	*(vict_it->second) = *(it->second);
	// у передающего мог стоять фриз славы, поэтому тут не помешает перепроверить и воткнуть нужное
	// тут есть вариант, когда нас не спасет даже проверка при входе в игру
	vict_it->second->freeze = PLR_FLAGGED(vict, PLR_FROZEN) ? true : false;
	name_convert(name);
	vict_it->second->name = name;
	vict_it->second->free_glory += free_glory; // если была свободная слава на принимающем - плюсуем

	std::ostringstream out;
	out << "Transfer "
	<< vict_it->second->spend_glory << " stats and "
	<< vict_it->second->free_glory - free_glory << " glory from "
	<< GET_NAME(ch) << " to " << name << " by " << GET_NAME(god);
	// в лог, обоим чарам в карму, имму в конце
	imm_log(out.str().c_str());
	add_karma(ch, out.str().c_str(), reason);
	Glory::add_glory_log(TRANSFER_GLORY, 0, out.str(), std::string(reason), vict);

	// если принимающий чар онлайн - сетим сразу ему статы
	if (d_vict)
	{
		for (GloryTimeType::iterator tm_it = vict_it->second->timers.begin(); tm_it != vict_it->second->timers.end(); ++tm_it)
		{
			if ((*tm_it)->timer > 0)
			{
				switch ((*tm_it)->stat)
				{
				case G_STR:
					GET_STR(d_vict->character) += (*tm_it)->glory;
					break;
				case G_DEX:
					GET_DEX(d_vict->character) += (*tm_it)->glory;
					break;
				case G_INT:
					GET_INT(d_vict->character) += (*tm_it)->glory;
					break;
				case G_WIS:
					GET_WIS(d_vict->character) += (*tm_it)->glory;
					break;
				case G_CON:
					GET_CON(d_vict->character) += (*tm_it)->glory;
					break;
				case G_CHA:
					GET_CHA(d_vict->character) += (*tm_it)->glory;
					break;
				default:
					log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
				}
			}
		}
	}

	add_karma(vict, out.str().c_str(), reason);
	save_char(vict, NOWHERE);

	// удаляем запись чара, с которого перекидывали
	glory_list.erase(it);
	// и выставляем ему новые статы (он то полюбому уже загружен канеш,
	// но тут стройная картина через дескриптор везде) и если он был оффлайн - обнулится при входе
	DESCRIPTOR_DATA *k = DescByUID(GET_UNIQUE(ch));
	if (k) recalculate_stats(k->character);

	out << ".\r\n";
	send_to_char(out.str(), god);
	save_glory();
	if (t_vict) delete t_vict;
}

/**
* Показ свободной и вложенной славы у чара (glory имя).
*/
void show_glory(CHAR_DATA *ch, CHAR_DATA *god)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "У %s совсем не славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	send_to_char(god, "Информация об очках славы %s:\r\n", GET_PAD(ch, 1));
	print_glory(god, it);
	return;
}

/**
* Вывод инфы в show stats.
*/
void show_stats(CHAR_DATA *ch)
{
	int free_glory = 0, spend_glory = 0;
	for (GloryListType::iterator it = glory_list.begin(); it != glory_list.end(); ++it)
	{
		free_glory += it->second->free_glory;
		spend_glory += it->second->spend_glory * 1000;
	}
	send_to_char(ch, "  Слава: вложено %d, свободно %d, всего %d\r\n", spend_glory, free_glory, free_glory + spend_glory);
}

/**
* Загрузка лога славы.
*/
void load_glory_log()
{
	const char *glory_file = "../log/glory.log";
	std::ifstream file(glory_file);
	if (!file.is_open())
	{
		log("GloryLog: не удалось открыть файл на чтение: %s", glory_file);
		return;
	}

	std::string buffer;
	while (std::getline(file, buffer))
	{
		std::string buffer2;
		GetOneParam(buffer, buffer2);
		time_t time = 0;
		try
		{
			time = boost::lexical_cast<int>(buffer2);
		}
		catch (boost::bad_lexical_cast &)
		{
			log("GloryLog: ошибка чтения time, buffer2: %s", buffer2.c_str());
			return;
		}
		GetOneParam(buffer, buffer2);
		int type = 0;
		try
		{
			type = boost::lexical_cast<int>(buffer2);
		}
		catch (boost::bad_lexical_cast &)
		{
			log("GloryLog: ошибка чтения type, buffer2: %s", buffer2.c_str());
			return;
		}
		GetOneParam(buffer, buffer2);
		int num = 0;
		try
		{
			num = boost::lexical_cast<int>(buffer2);
		}
		catch (boost::bad_lexical_cast &)
		{
			log("GloryLog: ошибка чтения num, buffer2: %s", buffer2.c_str());
			return;
		}

		boost::trim(buffer);
		GloryLogPtr temp_node(new GloryLog);
		temp_node->type = type;
		temp_node->num = num;
		temp_node->karma = buffer;
		glory_log.insert(std::make_pair(time, temp_node));
	}
}

/**
* Сохранение лога славы.
*/
void save_glory_log()
{
	const char *glory_file = "../log/glory.backup";
	std::ofstream file(glory_file);
	if (!file.is_open())
	{
		log("GloryLog: не удалось открыть файл на запись: %s", glory_file);
		return;
	}

	for (GloryLogType::const_iterator it = glory_log.begin(); it != glory_log.end(); ++it)
		file << it->first << " " << it->second->type << " " << it->second->num << " " << it->second->karma << "\n";
	file.close();

	std::string buffer("cp ""../log/glory.backup ""../log/glory.log");
	system(buffer.c_str());
}

/**
* Добавление записи в лог славы (время, тип, кол-во, строка из кармы).
*/
void add_glory_log(int type, int num, std::string punish, std::string reason, CHAR_DATA *vict)
{
	if (!vict || !GET_NAME(vict)) return;

	GloryLogPtr temp_node(new GloryLog);
	temp_node->type = type;
	temp_node->num = num;
	std::stringstream out;
	out << GET_NAME(vict) << " : " << punish << " [" << reason << "]";
	temp_node->karma = out.str();
	glory_log.insert(std::make_pair(time(0), temp_node));
	save_glory_log();
}

/**
* Показ лога славы (show glory), отсортированного по убыванию даты, с возможностью фильтрациии.
* Фильтры: show glory число|transfer|remove|hide
*/
void show_glory(CHAR_DATA *ch , char const * const value)
{
	if (glory_log.empty())
	{
		send_to_char("Пусто, слава те господи!\r\n", ch);
		return;
	}

	int type = 0;
	int num = 0;
	std::string buffer;
	if (value && *value)
		buffer = value;
	if (CompareParam(buffer, "transfer"))
		type = TRANSFER_GLORY;
	else if (CompareParam(buffer, "remove"))
		type = REMOVE_GLORY;
	else if (CompareParam(buffer, "hide"))
		type = HIDE_GLORY;
	else
	{
		try
		{
			num = boost::lexical_cast<int>(buffer);
		}
		catch (boost::bad_lexical_cast &)
		{
			type = 0;
			num = 0;
		}
	}
	std::stringstream out;
	for (GloryLogType::reverse_iterator it = glory_log.rbegin(); it != glory_log.rend(); ++it)
	{
		if (type == it->second->type || (type == 0 && num == 0) || (type == 0 && num <= it->second->num))
		{
			char time_buf[17];
			strftime(time_buf, sizeof(time_buf), "%H:%M %d-%m-%Y", localtime(&it->first));
			out << time_buf << " " << it->second->karma << "\r\n";
		}
	}
	page_string(ch->desc, out.str(), 1);
}

/**
* Распечатка топа славы. У иммов дополнительно печатается список чаров, не вошедших в топ (hide).
*/
void print_glory_top(CHAR_DATA *ch)
{
	std::stringstream out;
	boost::format class_format("\t%-20s %-2d\r\n");
	std::map<int, GloryNodePtr> temp_list;
	std::stringstream hide;

	bool print_hide = 0;
	if (IS_IMMORTAL(ch))
	{
		print_hide = 1;
		hide << "\r\nПерсонажи, исключенные из списка: ";
	}

	for (GloryListType::const_iterator it = glory_list.begin(); it != glory_list.end(); ++it)
	{
		if (!it->second->hide && !it->second->freeze)
			temp_list[it->second->free_glory + it->second->spend_glory * 1000] = it->second;
		else if (print_hide)
			hide << it->second->name << " ";
	}

	int i = 0;
	for (std::map<int, GloryNodePtr>::reverse_iterator t_it = temp_list.rbegin(); t_it != temp_list.rend() && i < MAX_TOP_CLASS; ++t_it, ++i)
		out << class_format % t_it->second->name % (t_it->second->free_glory + t_it->second->spend_glory * 1000);
	send_to_char(ch, out.str().c_str());

	if (print_hide)
	{
		hide << "\r\n";
		send_to_char(ch, hide.str().c_str());
	}
}

/**
* Вкдючение/выключение показа чара в топе славы (glory hide on|off).
*/
void hide_char(CHAR_DATA *vict, CHAR_DATA *god, char const * const mode)
{
	if (!mode || !*mode) return;
	bool ok = 1;
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(vict));
	if (it != glory_list.end())
	{
		if (!str_cmp(mode, "on"))
			it->second->hide = 1;
		else if (!str_cmp(mode, "off"))
			it->second->hide = 1;
		else
			ok = 0;
	}
	if (ok)
	{
		std::string text = it->second->hide ? "исключен из топа" : "присутствует в топе";
		send_to_char(god, "%s теперь %s прославленных персонажей\r\n", GET_NAME(vict), text.c_str());
		save_glory();
	}
	else
		send_to_char(god, "Некорректный параметр %s, hide может быть только on или off.\r\n", mode);
}

/**
* Остановка таймеров на вложенной славе.
*/
void set_freeze(long uid)
{
	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
		it->second->freeze = 1;
	save_glory();
}

/**
* Включение таймеров на вложенной славе.
*/
void remove_freeze(long uid)
{
	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
		it->second->freeze = 0;
	save_glory();
}

/**
* Во избежание разного рода недоразумений с фризом таймеров - проверяем эту тему при входе в игру.
*/
void check_freeze(CHAR_DATA *ch)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it != glory_list.end())
		it->second->freeze = PLR_FLAGGED(ch, PLR_FROZEN) ? true : false;
}

} // namespace Glory
