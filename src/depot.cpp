// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "depot.hpp"
#include <map>
#include <list>
#include <sstream>
#include <cmath>
#include <bitset>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include "db.h"
#include "handler.h"
#include "utils.h"
#include "comm.h"
#include "auction.h"
#include "exchange.h"
#include "interpreter.h"
#include "screen.h"
#include "char.hpp"

extern SPECIAL(bank);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern int can_take_obj(CHAR_DATA * ch, OBJ_DATA * obj);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);

namespace Depot
{

// внум персонального сундука
const int PERS_CHEST_VNUM = 331;
// рнум (пересчитываются при необходимости в renumber_obj_rnum)
int PERS_CHEST_RNUM = -1;
// максимальное кол-во шмоток в персональном хранилище (волхвам * 2)
const unsigned int MAX_PERS_SLOTS = 25;

/**
* Для оффлайнового списка шмоток в хранилище.
*/
class OfflineNode
{
public:
	OfflineNode() : vnum(0), timer(0), rent_cost(0), uid(0) {};
	int vnum; // внум
	int timer; // таймер
	int rent_cost; // цена ренты в день
	int uid; // глобальный уид
};

typedef std::list<OBJ_DATA *> ObjListType; // имя, шмотка
typedef std::list<OfflineNode> TimerListType; // список шмота, находящегося в оффлайне

class CharNode
{
public:
	CharNode() : ch(0), money(0), money_spend(0), buffer_cost(0), need_save(0), cost_per_day(0) {};

// онлайн
	// шмотки в персональном хранилище онлайн
	ObjListType pers_online;
	// чар (онлайн бабло и флаг наличия онлайна)
	CHAR_DATA *ch;
// оффлайн
	// шмотки в хранилищах оффлайн
	TimerListType offline_list;
	// бабло руки+банк оффлайн (флаг для пуржа шмоток при нуле)
	long money;
	// сколько было потрачено за время чара в ренте
	long money_spend;
// общие поля
	// буффер для точного снятия ренты
	double buffer_cost;
	// имя чара, чтобы не гонять всех через уиды при обработке
	std::string name;
	// в хранилище изменились шмотки и его нужно сохранить (тикание таймеров не в счет)
	// взяли/положили шмотку, спуржили по нулевому таймеру, вход чара в игру
	bool need_save;
	// общая стоимость ренты шмота в день (исключая персональное хранилище онлайн),
	// чтобы не иметь проблем с округлением и лишними пересчетами
	int cost_per_day;

	void save_online_objs();
	void update_online_item();
	void update_offline_item(long uid);
	void reset();
	bool removal_period_cost();

	void take_item(CHAR_DATA *vict, char *arg, int howmany);
	void remove_item(ObjListType::iterator &obj_it, ObjListType &cont, CHAR_DATA *vict);
	bool obj_from_obj_list(char *name, CHAR_DATA *vict);
	void load_online_objs(int file_type, bool reload = 0);
	void online_to_offline(ObjListType &cont);

	int get_cost_per_day()
	{
		return cost_per_day;
	};
	void reset_cost_per_day()
	{
		cost_per_day = 0;
	} ;
	void add_cost_per_day(OBJ_DATA *obj);
	void add_cost_per_day(int amount);
};

typedef std::map<long, CharNode> DepotListType; // уид чара, инфа
DepotListType depot_list; // список личных хранилищ

/**
* Капитально расширенная версия сислога для хранилищ.
*/
void depot_log(const char *format, ...)
{
	const char *filename = "../log/depot.log";
	static FILE *file = 0;
	if (!file)
	{
		file = fopen(filename, "a");
		if (!file)
		{
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	}
	else if (!format)
		format = "SYSERR: // depot_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
	fflush(file);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// список плееров, у которых было че-нить спуржено пока они были оффлайн.
// хранится просто уид игрока и при надобности лоадится его файл со строками о пурже.
// дата первой записи пишется для того, чтобы не плодить мертвых файлов (держим только за месяц).
typedef std::map < long/*уид игрока*/, time_t /*время первой записи в файле*/ > PurgedListType;
PurgedListType purged_list;

bool need_save_purged_list = false;

/**
*
*/
void save_purged_list()
{
	if (!need_save_purged_list) return;

	const char *filename = LIB_DEPOT"purge.db";
	std::ofstream file(filename);
	if (!file.is_open())
	{
		log("Хранилище: error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}
	for (PurgedListType::const_iterator it = purged_list.begin(); it != purged_list.end(); ++it)
		file << it->first << " " << it->second << "\n";

	need_save_purged_list = false;
}

/**
*
*/
void load_purge_list()
{
	const char *filename = LIB_DEPOT"purge.db";
	std::ifstream file(filename);
	if (!file.is_open())
	{
		log("Хранилище: error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}

	long uid;
	time_t date;

	while (file >> uid >> date)
		purged_list[uid] = date;
}

/**
*
*/
std::string generate_purged_filename(long uid)
{
	std::string name = GetNameByUnique(uid);
	if (name.empty())
		return "";

	char filename[MAX_STRING_LENGTH];
	if (!get_filename(name.c_str(), filename, PURGE_DEPOT_FILE))
		return "";

	return filename;
}

std::string generate_purged_text(long uid, int obj_vnum, unsigned int obj_uid)
{
	std::ostringstream out;
	out << "Ошибка при удалении предмета: vnum " << obj_vnum << ", uid " << obj_uid << "\r\n";

	std::string name = GetNameByUnique(uid);
	if (name.empty())
		return out.str();

	CHAR_DATA t_ch;
	CHAR_DATA *ch = &t_ch;
	if (load_char(name.c_str(), ch) < 0)
		return out.str();

	char filename[MAX_STRING_LENGTH];
	if (!get_filename(name.c_str(), filename, PERS_DEPOT_FILE))
	{
		log("Хранилище: не удалось сгенерировать имя файла (name: %s, filename: %s) (%s %s %d).",
			name.c_str(), filename, __FILE__, __func__, __LINE__);
		return out.str();
	}

	FILE *fl = fopen(filename, "r+b");
	if (!fl) return out.str();

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
	if (!fsize)
	{
		fclose(fl);
		log("Хранилище: пустой файл предметов (%s).", filename);
		return out.str();
	}
	char *databuf = new char [fsize + 1];

	fseek(fl, 0L, SEEK_SET);
	if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf)
	{
		fclose(fl);
		log("Хранилище: ошибка чтения файла предметов (%s).", filename);
		return out.str();
	}
	fclose(fl);

	char *data = databuf;
	*(data + fsize) = '\0';
	int error = 0;
	OBJ_DATA *obj;

	for (fsize = 0; *data && *data != '$'; fsize++)
	{
		if (!(obj = read_one_object_new(&data, &error)))
		{
			if (error)
				log("Хранилище: ошибка чтения предмета (%s, error: %d).", filename, error);
			continue;
		}
		if (GET_OBJ_UID(obj) == obj_uid && GET_OBJ_VNUM(obj) == obj_vnum)
		{
			std::ostringstream text;
			text << "[Персональное хранилище]: " << CCIRED(ch, C_NRM) << "'"
			<< obj->short_description << " рассыпал" << GET_OBJ_SUF_2(obj)
			<<  " в прах'" << CCNRM(ch, C_NRM) << "\r\n";
			extract_obj(obj);
			return text.str();
		}
		extract_obj(obj);
	}
	delete [] databuf;
	return out.str();
}

/**
*
*/
void add_purged_message(long uid, int obj_vnum, unsigned int obj_uid)
{
	std::string name = generate_purged_filename(uid);
	if (name.empty())
	{
		log("Хранилище: add_purge_message пустое имя файла %ld.", uid);
		return;
	}

	if (purged_list.find(uid) == purged_list.end())
	{
		purged_list[uid] = time(0);
		need_save_purged_list = true;
	}

	std::ofstream file(name.c_str(), std::ios_base::app);
	if (!file.is_open())
	{
		log("Хранилище: error open file: %s! (%s %s %d)", name.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	file << generate_purged_text(uid, obj_vnum, obj_uid);
}

/**
*
*/
void delete_purged_entry(long uid)
{
	PurgedListType::iterator it = purged_list.find(uid);
	if (it != purged_list.end())
	{
		std::string name = generate_purged_filename(uid);
		if (name.empty())
		{
			log("Хранилище: delete_purge_entry пустое имя файла %ld.", uid);
			return;
		}
		remove(name.c_str());
		purged_list.erase(it);
		need_save_purged_list = true;
	}
}

/**
*
*/
void show_purged_message(CHAR_DATA *ch)
{
	PurgedListType::iterator it = purged_list.find(GET_UNIQUE(ch));
	if (it != purged_list.end())
	{
		// имя у нас канеш и так есть, но че зря код дублировать
		std::string name = generate_purged_filename(GET_UNIQUE(ch));
		if (name.empty())
		{
			log("Хранилище: show_purged_message пустое имя файла %d.", GET_UNIQUE(ch));
			return;
		}
		std::ifstream file(name.c_str(), std::ios::binary);
		if (!file.is_open())
		{
			log("Хранилище: error open file: %s! (%s %s %d)", name.c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		std::ostringstream out;
		out << "\r\n" << file.rdbuf();
		send_to_char(out.str(), ch);
		remove(name.c_str());
		purged_list.erase(it);
		need_save_purged_list = true;
	}
}

/**
*
*/
void clear_old_purged_entry()
{
	time_t today = time(0);
	for (PurgedListType::iterator it = purged_list.begin(); it != purged_list.end(); /* пусто */)
	{
		time_t diff = today - it->second;
		// месяц в секундах
		if (diff >= 60 * 60 * 24 * 31)
		{
			std::string name = generate_purged_filename(it->first);
			if (!name.empty())
				remove(name.c_str());
			purged_list.erase(it++);
			need_save_purged_list = true;
		}
		else
			++it;
	}
}

/**
*
*/
void init_purged_list()
{
	load_purge_list();
	clear_old_purged_entry();
	need_save_purged_list = true;
	save_purged_list();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Удаление файла персонального хранилища (шмотки отдельного чара).
*/
void remove_pers_file(std::string name)
{
	if (name.empty()) return;

	char filename[MAX_STRING_LENGTH];
	if (get_filename(name.c_str(), filename, PERS_DEPOT_FILE))
		remove(filename);
}

/**
* Удаление шмоток и файлов шмоток, если они были.
* Делается попытся снятия с чара долга за ренту.
*/
void remove_char_entry(long uid, CharNode &node)
{
	depot_log("remove_char_entry: %ld", uid);
	// если чар был что-то должен, надо попытаться с него это снять
	if (!node.name.empty() && (node.money_spend || node.buffer_cost))
	{
		CHAR_DATA t_victim;
		CHAR_DATA *victim = &t_victim;
		if (load_char(node.name.c_str(), victim) > -1 && GET_UNIQUE(victim) == uid)
		{
			int total_pay = node.money_spend + static_cast<int>(node.buffer_cost);
			add_bank_gold(victim, -total_pay);
			if (get_bank_gold(victim) < 0)
			{
				add_gold(victim, get_bank_gold(victim));
				set_bank_gold(victim, 0);
				if (get_gold(victim) < 0)
					set_gold(victim, 0);
			}
			save_char(victim, NOWHERE);
		}
	}
	node.reset();
	remove_pers_file(node.name);
}

/**
* Инициализация рнумов сундуков. Лоад файла с оффлайн информацией по предметам.
*/
void init_depot()
{
	depot_log("init_depot start");

	init_purged_list();
	PERS_CHEST_RNUM = real_object(PERS_CHEST_VNUM);

	const char *depot_file = LIB_DEPOT"depot.db";
	std::ifstream file(depot_file);
	if (!file.is_open())
	{
		log("Хранилище: error open file: %s! (%s %s %d)", depot_file, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	while (file >> buffer)
	{
		if (buffer != "<Node>")
		{
			log("Хранилище: ошибка чтения <Node>");
			break;
		}
		CharNode tmp_node;
		long uid;
		// общие поля
		if (!(file >> uid >> tmp_node.money >> tmp_node.money_spend >> tmp_node.buffer_cost))
		{
			log("Хранилище: ошибка чтения uid(%ld), money(%ld), money_spend(%ld), buffer_cost(%f).",
				uid, tmp_node.money, tmp_node.money_spend, tmp_node.buffer_cost);
			break;
		}
		// чтение предметов
		file >> buffer;
		if (buffer != "<Objects>")
		{
			log("Хранилище: ошибка чтения <Objects>.");
			break;
		}
		while (file >> buffer)
		{
			if (buffer == "</Objects>") break;

			OfflineNode tmp_obj;
			try
			{
				tmp_obj.vnum = boost::lexical_cast<int>(buffer);
			}
			catch (boost::bad_lexical_cast &)
			{
				log("Хранилище: ошибка чтения vnum (%s)", buffer.c_str());
				break;
			}
			if (!(file >> tmp_obj.timer >> tmp_obj.rent_cost >> tmp_obj.uid))
			{
				log("Хранилище: ошибка чтения timer(%d) rent_cost(%d) uid(%d) (obj vnum: %d).",
					tmp_obj.timer, tmp_obj.rent_cost, tmp_obj.uid, tmp_obj.vnum);
				break;
			}
			// проверяем существование прототипа предмета и суем его в маск в мире на постое
			int rnum = real_object(tmp_obj.vnum);
			if (rnum >= 0)
			{
				obj_index[rnum].stored++;
				tmp_node.add_cost_per_day(tmp_obj.rent_cost);
				tmp_node.offline_list.push_back(tmp_obj);
			}
		}
		if (buffer != "</Objects>")
		{
			log("Хранилище: ошибка чтения </Objects>.");
			break;
		}
		// проверка корректной дочитанности
		file >> buffer;
		if (buffer != "</Node>")
		{
			log("Хранилище: ошибка чтения </Node>.");
			break;
		}

		// Удаление записи из списка хранилищ делается только здесь и при пурже на нехватке денег,
		// причем нужно не забывать, что выше при лоаде шмоток они уже попали в список макс в мире
		// и удаление записи не через remove_char_entry - это будет касяк

		// проверяем есть ли еще такой чар вообще
		tmp_node.name = GetNameByUnique(uid);
		if (tmp_node.name.empty())
		{
			log("Хранилище: UID %ld - персонажа не существует.", uid);
			remove_char_entry(uid, tmp_node);
			continue;
		}
		// пустые хранилища удаляются только здесь
		if (tmp_node.offline_list.empty() && tmp_node.pers_online.empty())
		{
			log("Хранилище: UID %ld - пустое хранилище.", uid);
			remove_char_entry(uid, tmp_node);
			continue;
		}

		name_convert(tmp_node.name);
		depot_list[uid] = tmp_node;
	}

	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		depot_log("char: %ld", it->first);
		for (TimerListType::iterator obj_it = it->second.offline_list.begin(); obj_it != it->second.offline_list.end(); ++obj_it)
			depot_log("%d %d %d %d", obj_it->vnum, obj_it->uid, obj_it->timer, obj_it->rent_cost);
	}
	depot_log("init_depot end");
}

/**
* Загрузка самих хранилищ в банки делается после инита хранилищ и резета зон, потому как мобов надо.
*/
void load_chests()
{
	for (CHAR_DATA *ch = character_list; ch; ch = ch->next)
	{
		if (ch->nr > 0 && ch->nr <= top_of_mobt && mob_index[ch->nr].func == bank)
		{
			OBJ_DATA *pers_chest = read_object(PERS_CHEST_VNUM, VIRTUAL);
			if (!pers_chest) return;
			obj_to_room(pers_chest, ch->in_room);
		}
	}
}

/**
* Берется минимальная цена ренты шмотки, не важно, одетая она будет или снятая.
*/
int get_object_low_rent(OBJ_DATA *obj)
{
	int rent = GET_OBJ_RENT(obj) > GET_OBJ_RENTEQ(obj) ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj);
	return rent;
}

/**
* Добавление цены ренты с порверкой на валидность и переполнение.
*/
void CharNode::add_cost_per_day(int amount)
{
	if (amount < 0 || amount > 50000)
	{
		log("Хранилище: невалидная стоимость ренты %i", amount);
		return;
	}
	int over = std::numeric_limits<int>::max() - cost_per_day;
	if (amount > over)
		cost_per_day = std::numeric_limits<int>::max();
	else
		cost_per_day += amount;
}

/**
* Версия add_cost_per_day(int amount) для принятия *OBJ_DATA.
*/
void CharNode::add_cost_per_day(OBJ_DATA *obj)
{
	add_cost_per_day(get_object_low_rent(obj));
}

/**
* Для удобства в save_timedata(), запись строки с параметрами шмотки.
*/
void write_objlist_timedata(const ObjListType &cont, std::ofstream &file)
{
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
		file << GET_OBJ_VNUM(*obj_it) << " " << GET_OBJ_TIMER(*obj_it) << " "
		<< get_object_low_rent(*obj_it) << " " << GET_OBJ_UID(*obj_it) << "\n";
}

/**
* Сохранение всех списков в общий файл с тайм-инфой шмоток (в том числе таймеров из онлайн списка).
*/
void save_timedata()
{
	const char *depot_file = LIB_DEPOT"depot.backup";
	std::ofstream file(depot_file);
	if (!file.is_open())
	{
		log("Хранилище: error open file: %s! (%s %s %d)", depot_file, __FILE__, __func__, __LINE__);
		return;
	}

	for (DepotListType::const_iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		file << "<Node>\n" << it->first << " ";
		// чар онлайн - пишем бабло с его счета, иначе берем из оффлайновых полей инфу
		if (it->second.ch)
			file << get_bank_gold(it->second.ch) + get_gold(it->second.ch) << " 0 ";
		else
			file << it->second.money << " " << it->second.money_spend << " ";
		file << it->second.buffer_cost << "\n";

		// собсна какие списки не пустые - те и пишем, особо не мудрствуя
		file << "<Objects>\n";
		write_objlist_timedata(it->second.pers_online, file);
		for (TimerListType::const_iterator obj_it = it->second.offline_list.begin();
				obj_it != it->second.offline_list.end(); ++obj_it)
		{
			file << obj_it->vnum << " " << obj_it->timer << " " << obj_it->rent_cost
			<< " " << obj_it->uid << "\n";
		}
		file << "</Objects>\n</Node>\n";
	}
	file.close();
	std::string buffer("cp "LIB_DEPOT"depot.backup "LIB_DEPOT"depot.db");
	system(buffer.c_str());
}

/**
* Запись файла со шмотом (полная версия записи).
*/
void write_obj_file(const std::string &name, int file_type, const ObjListType &cont)
{
	depot_log("write_obj_file: %s", name.c_str());
	char filename[MAX_STRING_LENGTH];
	if (!get_filename(name.c_str(), filename, file_type))
	{
		log("Хранилище: не удалось сгенерировать имя файла (name: %s, filename: %s) (%s %s %d).",
			name.c_str(), filename, __FILE__, __func__, __LINE__);
		return;
	}
	// при пустом списке просто удаляем файл, чтобы не плодить пустышек в дире
	if (cont.empty())
	{
		depot_log("empty cont");
		remove(filename);
		return;
	}

	std::ofstream file(filename);
	if (!file.is_open())
	{
		log("Хранилище: error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}
	file << "* Items file\n";
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		depot_log("save: %s %d %d", (*obj_it)->short_description, GET_OBJ_UID(*obj_it), GET_OBJ_VNUM(*obj_it));
		char databuf[MAX_STRING_LENGTH];
		char *data = databuf;
		write_one_object(&data, *obj_it, 0);
		file << databuf;
	}
	file << "\n$\n$\n";
	file.close();
}

/**
* Сохранение персонального хранилища конкретного плеера в файл. Обнуление флага сейва.
*/
void CharNode::save_online_objs()
{
	if (need_save)
	{
		write_obj_file(name, PERS_DEPOT_FILE, pers_online);
		need_save = false;
	}
}

/**
* Сохранение всех ждущих этого онлайновых списков предметов в файлы.
*/
void save_all_online_objs()
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
		it->second.save_online_objs();
}

/**
* Апдейт таймеров в онлайн списках с оповещением о пурже, если чар онлайн и расчетом общей ренты.
*/
void CharNode::update_online_item()
{
	for (ObjListType::iterator obj_it = pers_online.begin(); obj_it != pers_online.end();)
	{
		GET_OBJ_TIMER(*obj_it)--;
		if (GET_OBJ_TIMER(*obj_it) <= 0)
		{
			if (ch)
			{
				// если чар в лд или еще чего - лучше записать и выдать это ему при след
				// входе в игру, чтобы уж точно увидел
				if (ch->desc && STATE(ch->desc) == CON_PLAYING)
				{
					send_to_char(ch, "[Персональное хранилище]: %s'%s рассыпал%s в прах'%s\r\n",
								 CCIRED(ch, C_NRM), (*obj_it)->short_description,
								 GET_OBJ_SUF_2((*obj_it)), CCNRM(ch, C_NRM));
				}
				else
					add_purged_message(GET_UNIQUE(ch), GET_OBJ_VNUM(*obj_it), GET_OBJ_UID(*obj_it));
			}

			// вычитать ренту из cost_per_day здесь не надо, потому что она уже обнулена
			depot_log("zero timer, online extract: %s %d %d", (*obj_it)->short_description, GET_OBJ_UID(*obj_it), GET_OBJ_VNUM(*obj_it));
			extract_obj(*obj_it);
			pers_online.erase(obj_it++);
			need_save = true;
		}
		else
			++obj_it;
	}
	save_purged_list();
}

/**
* Снятие ренты за период в оффлайне с удалением шмота при нехватке денег.
* \return true - нужно удалять запись в списки хранилищ
*/
bool CharNode::removal_period_cost()
{
	double i;
	buffer_cost += (static_cast<double>(cost_per_day) / SECS_PER_MUD_DAY);
	modf(buffer_cost, &i);
	if (i)
	{
		int diff = static_cast<int>(i);
		money -= diff;
		money_spend += diff;
		buffer_cost -= i;
	}
	return (money < 0) ? true : false;
}

/**
* Апдейт таймеров на всех шмотках, удаление при нулевых таймерах (с оповещением).
*/
void update_timers()
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end();)
	{
		class CharNode & node = it->second;
		// обнуляем ренту и пересчитываем ее по ходу обновления оффлайн таймеров
		node.reset_cost_per_day();
		// онлайновый/оффлайновый списки
		if (node.ch)
			node.update_online_item();
		else
			node.update_offline_item(it->first);
		// снятие денег и пурж шмота, если денег уже не хватает
		if (node.get_cost_per_day() && node.removal_period_cost())
		{
			depot_log("update_timers: no money %ld %s", it->first, node.name.c_str());
			log("Хранилище: UID %ld (%s) - запись удалена из-за нехватки денег на ренту.",
				it->first, node.name.c_str());
			remove_char_entry(it->first, it->second);
			depot_list.erase(it++);
		}
		else
			++it;
	}
}

/**
* Апдейт таймеров в оффлайн списках с расчетом общей ренты.
*/
void CharNode::update_offline_item(long uid)
{
	for (TimerListType::iterator obj_it = offline_list.begin(); obj_it != offline_list.end();)
	{
		--(obj_it->timer);
		if (obj_it->timer <= 0)
		{
			depot_log("update_offline_item %s: zero timer %d %d", name.c_str(), obj_it->vnum, obj_it->uid);
			add_purged_message(uid, obj_it->vnum, obj_it->uid);
			// шмотка уходит в лоад
			int rnum = real_object(obj_it->vnum);
			if (rnum >= 0)
				obj_index[rnum].stored--;
			// вычитать ренту из cost_per_day здесь не надо, потому что она уже обнулена
			offline_list.erase(obj_it++);
		}
		else
		{
			add_cost_per_day(obj_it->rent_cost);
			++obj_it;
		}
	}
	save_purged_list();
}

/**
* Очищение всех списков шмота и полей бабла с флагами. Сама запись хранилища не убивается.
* Флаг для сохранения файла шмоток здесь не включается, потому что не всегда нужен (релоад).
*/
void CharNode::reset()
{
	for (ObjListType::iterator obj_it = pers_online.begin(); obj_it != pers_online.end(); ++obj_it)
	{
		depot_log("reset %s: online extract %s %d %d", name.c_str(), (*obj_it)->short_description, GET_OBJ_UID(*obj_it), GET_OBJ_VNUM(*obj_it));
		extract_obj(*obj_it);
	}
	pers_online.clear();

	// тут нужно ручками перекинуть удаляемый шмот в лоад и потом уже грохнуть все разом
	for (TimerListType::iterator obj = offline_list.begin(); obj != offline_list.end(); ++obj)
	{
		depot_log("reset_%s: offline erase %d %d", name.c_str(), obj->vnum, obj->uid);
		int rnum = real_object(obj->vnum);
		if (rnum >= 0)
			obj_index[rnum].stored--;
	}
	offline_list.clear();

	reset_cost_per_day();
	buffer_cost = 0;
	money = 0;
	money_spend = 0;
}

/**
* Проверка, является ли obj персональным хранилищем.
* \return 0 - не является, 1- является.
*/
bool is_depot(OBJ_DATA *obj)
{
	if (obj->item_number == PERS_CHEST_RNUM)
		return true;
	else
		return false;
}

/**
* Распечатка отдельного предмета при осмотре хранилища.
*/
void print_obj(std::stringstream &out, OBJ_DATA *obj, int count)
{
	out << obj->short_description;
	if (count > 1)
		out << " [" << count << "]";
	out << " [" << get_object_low_rent(obj) << " "
	<< desc_count(get_object_low_rent(obj), WHAT_MONEYa) << "]\r\n";
}

/**
* Расчет кол-ва слотов под шмотки в персональном хранилище с учетом профы чара.
*/
unsigned int get_max_pers_slots(CHAR_DATA *ch)
{
	if (GET_CLASS(ch) == CLASS_DRUID)
		return MAX_PERS_SLOTS * 2;
	else
		return MAX_PERS_SLOTS;
}

/**
* Для читаемости show_depot - вывод списка предметов персонажу.
* Сортировка по алиасам и группировка одинаковых предметов.
* В данном случае ch и money идут раздельно, чтобы можно было сразу выводить и чужие хранилища.
*/
std::string print_obj_list(CHAR_DATA * ch, ObjListType &cont, const std::string &chest_name, int money)
{
	int rent_per_day = 0;
	std::stringstream out;

	cont.sort(
		boost::bind(std::less<char *>(),
					boost::bind(&obj_data::name, _1),
					boost::bind(&obj_data::name, _2)));

	ObjListType::const_iterator prev_obj_it = cont.end();
	int count = 0;
	bool found = 0;
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		if (prev_obj_it == cont.end())
		{
			prev_obj_it = obj_it;
			count = 1;
		}
		else if (!equal_obj(*obj_it, *prev_obj_it))
		{
			print_obj(out, *prev_obj_it, count);
			prev_obj_it = obj_it;
			count = 1;
		}
		else
			count++;
		rent_per_day += get_object_low_rent(*obj_it);
		found = true;
	}
	if (prev_obj_it != cont.end() && count)
		print_obj(out, *prev_obj_it, count);
	if (!found)
		out << "В данный момент хранилище абсолютно пусто.\r\n";

	std::stringstream head;
	int expired = rent_per_day ? (money / rent_per_day) : 0;
	head << CCWHT(ch, C_NRM) << chest_name
	<< "Вещей: " << cont.size()
	<< ", свободно мест: " << get_max_pers_slots(ch) - cont.size()
	<< ", рента в день: " << rent_per_day << " " << desc_count(rent_per_day, WHAT_MONEYa);
	if (rent_per_day)
		head << ", денег на " << expired << " " << desc_count(expired, WHAT_DAY);
	else
		head << ", денег на очень много дней";
	head << CCNRM(ch, C_NRM) << ".\r\n";

	return (head.str() + out.str());
}

/**
* Поиск итератора хранилища или создание нового, чтобы не париться.
* Возврат итератора за списком означает, что чара в плеер-таблице мы не нашли (что странно).
* \param ch - проставляет в зависимости от наличия чара онлайн, иначе будет нулевым.
*/
DepotListType::iterator create_depot(long uid, CHAR_DATA *ch = 0)
{
	DepotListType::iterator it = depot_list.find(uid);
	if (it == depot_list.end())
	{
		CharNode tmp_node;
		// в случае отсутствия чара (пока только при релоаде) ch останется нулевым,
		// а имя возьмется через уид
		tmp_node.ch = ch;
		tmp_node.name = GetNameByUnique(uid);
		if (!tmp_node.name.empty())
		{
			depot_list[uid] = tmp_node;
			it = depot_list.find(uid);
		}
	}
	return it;
}

/**
* Выводим персональное хранилище вместо просмотра контейнера.
*/
void show_depot(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_NPC(ch)) return;
	if (IS_IMMORTAL(ch))
	{
		send_to_char("И без хранилища обойдешься...\r\n" , ch);
		return;
	}
	if (RENTABLE(ch))
	{
		send_to_char(ch, "%sХранилище недоступно в связи с боевыми действиями.%s\r\n",
					 CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		return;
	}

	DepotListType::iterator it = create_depot(GET_UNIQUE(ch), ch);
	if (it == depot_list.end())
	{
		send_to_char("Ошибочка, сообщие богам...\r\n" , ch);
		log("Хранилище: UID %d, name: %s - возвращен некорректный уид персонажа.", GET_UNIQUE(ch), GET_NAME(ch));
		return;
	}

	std::string out;
	out = print_obj_list(ch, it->second.pers_online, "Ваше персональное хранилище:\r\n", (get_gold(ch) + get_bank_gold(ch)));
	page_string(ch->desc, out, 1);
}

/**
* В случае переполнения денег на счете - кладем сколько можем, остальное возвращаем чару на руки.
* На руках при возврате переполняться уже некуда, т.к. вложение идет с этих самых рук.
*/
void put_gold_chest(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) != ITEM_MONEY) return;

	long gold = GET_OBJ_VAL(obj, 0);
	if ((get_bank_gold(ch) + gold) < 0)
	{
		long over = std::numeric_limits<long>::max() - get_bank_gold(ch);
		add_bank_gold(ch, over);
		gold -= over;
		add_gold(ch, gold);
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "Вы удалось вложить только %ld %s.\r\n",
					 over, desc_count(over, WHAT_MONEYu));
	}
	else
	{
		add_bank_gold(ch, gold);
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "Вы вложили %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
	}
}

/**
* Проверка возможности положить шмотку в хранилище.
* FIXME с кланами копипаст.
*/
bool can_put_chest(CHAR_DATA *ch, OBJ_DATA *obj)
{
	// depot_log("can_put_chest: %s, %s", GET_NAME(ch), GET_OBJ_PNAME(obj, 0));
	if (OBJ_FLAGGED(obj, ITEM_ZONEDECAY)
			|| OBJ_FLAGGED(obj, ITEM_REPOP_DECAY)
			|| OBJ_FLAGGED(obj, ITEM_NOSELL)
			|| OBJ_FLAGGED(obj, ITEM_DECAY)
			|| OBJ_FLAGGED(obj, ITEM_NORENT)
			|| GET_OBJ_TYPE(obj) == ITEM_KEY
			|| GET_OBJ_RENT(obj) < 0
			|| GET_OBJ_RNUM(obj) <= NOTHING)
	{
		send_to_char(ch, "Неведомая сила помешала положить %s в хранилище.\r\n", OBJ_PAD(obj, 3));
		return 0;
	}
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
	{
		send_to_char(ch, "В %s что-то лежит.\r\n", OBJ_PAD(obj, 5));
		return 0;
	}
	return 1;
}

/**
* Кладем шмотку в хранилище (мобов посылаем лесом), деньги автоматом на счет в банке.
*/
bool put_depot(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_NPC(ch)) return 0;
	if (IS_IMMORTAL(ch))
	{
		send_to_char("И без хранилища обойдешься...\r\n" , ch);
		return 0;
	}
	if (RENTABLE(ch))
	{
		send_to_char(ch, "%sХранилище недоступно в связи с боевыми действиями.%s\r\n",
					 CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		return 0;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
	{
		put_gold_chest(ch, obj);
		return 1;
	}
	// в данном случае если мы не можем сунуть какую-то конкретную шмотку - это не стопит процесс
	if (!can_put_chest(ch, obj)) return 1;

	DepotListType::iterator it = create_depot(GET_UNIQUE(ch), ch);
	if (it == depot_list.end())
	{
		send_to_char("Ошибочка, сообщие богам...\r\n" , ch);
		log("Хранилище: UID %d, name: %s - возвращен некорректный уид персонажа.", GET_UNIQUE(ch), GET_NAME(ch));
		return 0;
	}

	if (it->second.pers_online.size() >= get_max_pers_slots(ch))
	{
		send_to_char("В вашем хранилище совсем не осталось места :(.\r\n" , ch);
		return 0;
	}
	if (!get_bank_gold(ch) && !get_gold(ch))
	{
		send_to_char(ch,
					 "У Вас ведь совсем нет денег, чем Вы собираетесь расплачиваться за хранение вещей?\r\n",
					 OBJ_PAD(obj, 5));
		return 0;
	}

	depot_log("put_depot %s %ld: %s %d %d", GET_NAME(ch), GET_UNIQUE(ch), obj->short_description, GET_OBJ_UID(obj), GET_OBJ_VNUM(obj));
	it->second.pers_online.push_front(obj);
	it->second.need_save = true;

	act("Вы положили $o3 в персональное хранилище.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n положил$g $o3 в персональное хранилище.", TRUE, ch, obj, 0, TO_ROOM);

	obj_from_char(obj);
	check_auction(NULL, obj);
	OBJ_DATA *temp;
	REMOVE_FROM_LIST(obj, object_list, next);

	return 1;
}

/**
* Взятие чего-то из персонального хранилища.
*/
void take_depot(CHAR_DATA *vict, char *arg, int howmany)
{
	if (IS_NPC(vict)) return;
	if (IS_IMMORTAL(vict))
	{
		send_to_char("И без хранилища обойдешься...\r\n" , vict);
		return;
	}
	if (RENTABLE(vict))
	{
		send_to_char(vict, "%sХранилище недоступно в связи с боевыми действиями.%s\r\n",
					 CCIRED(vict, C_NRM), CCNRM(vict, C_NRM));
		return;
	}

	DepotListType::iterator it = depot_list.find(GET_UNIQUE(vict));
	if (it == depot_list.end())
	{
		send_to_char("В данный момент Ваше хранилище абсолютно пусто.\r\n", vict);
		return;
	}

	it->second.take_item(vict, arg, howmany);
}

/**
* Берем шмотку из хранилища.
*/
void CharNode::remove_item(ObjListType::iterator &obj_it, ObjListType &cont, CHAR_DATA *vict)
{
	depot_log("remove_item %s: %s %d %d", name.c_str(), (*obj_it)->short_description, GET_OBJ_UID(*obj_it), GET_OBJ_VNUM(*obj_it));
	(*obj_it)->next = object_list;
	object_list = *obj_it;
	obj_to_char(*obj_it, vict);
	act("Вы взяли $o3 из персонального хранилища.", FALSE, vict, *obj_it, 0, TO_CHAR);
	act("$n взял$g $o3 из персонального хранилища.", TRUE, vict, *obj_it, 0, TO_ROOM);
	cont.erase(obj_it++);
	need_save = true;
}

/**
* Поиск шмотки в контейнере (со всякими точками), удаляем ее тут же.
*/
bool CharNode::obj_from_obj_list(char *name, CHAR_DATA *vict)
{
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	strcpy(tmp, name);

	ObjListType &cont = pers_online;

	int j = 0, number;
	if (!(number = get_number(&tmp))) return false;

	for (ObjListType::iterator obj_it = cont.begin(); obj_it != cont.end() && (j <= number); ++obj_it)
	{
		if (isname(tmp, (*obj_it)->name) && ++j == number)
		{
			remove_item(obj_it, cont, vict);
			return true;
		}
	}
	return false;
}

/**
* Поиск шмотки в хранилище и ее взятие.
*/
void CharNode::take_item(CHAR_DATA *vict, char *arg, int howmany)
{
	ObjListType &cont = pers_online;

	int obj_dotmode = find_all_dots(arg);
	if (obj_dotmode == FIND_INDIV)
	{
		bool result = obj_from_obj_list(arg, vict);
		if (!result)
		{
			send_to_char(vict, "Вы не видите '%s' в хранилище.\r\n", arg);
			return;
		}
		while (result && --howmany)
			result = obj_from_obj_list(arg, vict);
	}
	else
	{
		if (obj_dotmode == FIND_ALLDOT && !*arg)
		{
			send_to_char("Взять что \"все\" ?\r\n", vict);
			return;
		}
		bool found = 0;
		for (ObjListType::iterator obj_list_it = cont.begin(); obj_list_it != cont.end();)
		{
			if (obj_dotmode == FIND_ALL || isname(arg, (*obj_list_it)->name))
			{
				// чтобы нельзя было разом собрать со шкафчика неск.тыс шмоток
				if (!can_take_obj(vict, *obj_list_it)) return;
				found = 1;
				remove_item(obj_list_it, cont, vict);
			}
			else
				++obj_list_it;
		}

		if (!found)
		{
			send_to_char(vict, "Вы не видите ничего похожего на '%s' в хранилище.\r\n", arg);
			return;
		}
	}
}

/**
* Рента за шмот в день, но с учетом персонального хранилища онлайн, для вывода перед рентой.
* Эта функция зовется ДО того, как поизройдет перекидывание шиота в оффлайн список.
*/
int get_total_cost_per_day(CHAR_DATA *ch)
{
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		int cost = 0;
		if (!it->second.pers_online.empty())
			for (ObjListType::const_iterator obj_it = it->second.pers_online.begin();
					obj_it != it->second.pers_online.end(); ++obj_it)
			{
				cost += get_object_low_rent(*obj_it);
			}
		cost += it->second.get_cost_per_day();
		return cost;
	}
	return 0;
}


/**
* Вывод инфы в show stats.
* TODO: добавить вывод расходов, может еще процент наполненности персональных хранилищ.
*/
void show_stats(CHAR_DATA *ch)
{
	std::stringstream out;
	int pers_count = 0, offline_count = 0;
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		pers_count += it->second.pers_online.size();
		offline_count += it->second.offline_list.size();
	}

	out << "  Хранилищ: " << depot_list.size() << ", "
	<< "в персональных: " << pers_count << ", "
	<< "в оффлайне: " << offline_count << "\r\n";
	send_to_char(out.str().c_str(), ch);
}

/**
* Лоад списка шмоток в онлайн список. FIXME: копи-паст чтения предметов с двух мест.
* \param reload - для рестора шмоток чара, по умолчанию ноль
* (определяет сверяемся мы с оффлайн списком при лоаде или нет).
*/
void CharNode::load_online_objs(int file_type, bool reload)
{
	depot_log("load_online_objs %s start", name.c_str());
	if (!reload && offline_list.empty())
	{
		depot_log("empty offline_list and !reload");
		return;
	}

	char filename[MAX_STRING_LENGTH];
	if (!get_filename(name.c_str(), filename, file_type))
	{
		log("Хранилище: не удалось сгенерировать имя файла (name: %s, filename: %s) (%s %s %d).",
			name.c_str(), filename, __FILE__, __func__, __LINE__);
		return;
	}

	FILE *fl = fopen(filename, "r+b");
	if (!fl) return;

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
	if (!fsize)
	{
		fclose(fl);
		log("Хранилище: пустой файл предметов (%s).", filename);
		return;
	}
	char *databuf = new char [fsize + 1];

	fseek(fl, 0L, SEEK_SET);
	if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf)
	{
		fclose(fl);
		log("Хранилище: ошибка чтения файла предметов (%s).", filename);
		return;
	}
	fclose(fl);

	depot_log("file ok");

	char *data = databuf;
	*(data + fsize) = '\0';
	int error = 0;
	OBJ_DATA *obj;

	for (fsize = 0; *data && *data != '$'; fsize++)
	{
		if (!(obj = read_one_object_new(&data, &error)))
		{
			depot_log("reading object error %d", error);
			if (error)
				log("Хранилище: ошибка чтения предмета (%s, error: %d).", filename, error);
			continue;
		}
		if (!reload)
		{
			// собсна сверимся со списком таймеров и проставим изменившиеся данные
			TimerListType::iterator obj_it = std::find_if(offline_list.begin(), offline_list.end(),
											 boost::bind(std::equal_to<long>(),
														 boost::bind(&OfflineNode::uid, _1), GET_OBJ_UID(obj)));
			if (obj_it != offline_list.end() && obj_it->vnum == GET_OBJ_VNUM(obj))
			{
				depot_log("load object %s %d %d", obj->short_description, GET_OBJ_UID(obj), GET_OBJ_VNUM(obj));
				GET_OBJ_TIMER(obj) = obj_it->timer;
				// надо уменьшать макс в мире на постое, макс в мире шмотки в игре
				// увеличивается в read_one_object_new через read_object
				int rnum = real_object(GET_OBJ_VNUM(obj));
				if (rnum >= 0)
					obj_index[rnum].stored--;
			}
			else
			{
				depot_log("extract object %s %d %d", obj->short_description, GET_OBJ_UID(obj), GET_OBJ_VNUM(obj));
				extract_obj(obj);
				continue;
			}
		}
		// при релоаде мы ничего не сверяем, а лоадим все, что есть,
		// макс в мире и так увеличивается при чтении шмотки, а на постое ее и не было

		pers_online.push_front(obj);
		// убираем ее из глобального листа, в который она добавилась еще на стадии чтения из файла
		OBJ_DATA *temp;
		REMOVE_FROM_LIST(obj, object_list, next);
	}
	delete [] databuf;
	offline_list.clear();
	reset_cost_per_day();
}

/**
* Вход чара в игру - снятие за хранилище, пурж шмоток, оповещение о кол-ве снятых денег, перевод списков в онлайн.
*/
void enter_char(CHAR_DATA *ch)
{
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		bool purged = false;
		// снимаем бабло, если что-то было потрачено на ренту
		if (it->second.money_spend > 0)
		{
			add_bank_gold(ch, -(it->second.money_spend));
			if (get_bank_gold(ch) < 0)
			{
				add_gold(ch, get_bank_gold(ch));
				set_bank_gold(ch, 0);
				// есть вариант, что денег не хватит, потому что помимо хранилищ еще капает за
				// одежду и инвентарь, а учитывать еще и их при расчетах уже как-то мутно
				// поэтому мы просто готовы, если что, все технично спуржить при входе
				if (get_gold(ch) < 0)
				{
					depot_log("no money %s %ld: reset depot", GET_NAME(ch), GET_UNIQUE(ch));
					set_gold(ch, 0);
					it->second.reset();
					purged = true;
					// файл убьется позже при ребуте на пустом хране,
					// даже если не будет никаких перезаписей по ходу игры
				}
			}

			send_to_char(ch, "%sХранилище: за время Вашего отсутствия удержано %d %s.%s\r\n\r\n",
						 CCWHT(ch, C_NRM), it->second.money_spend, desc_count(it->second.money_spend,
								 WHAT_MONEYa), CCNRM(ch, C_NRM));
			if (purged)
				send_to_char(ch, "%sХранилище: у вас нехватило денег на постой.%s\r\n\r\n",
							 CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		// грузим хранилище, сохранять его тут вроде как смысла нет
		it->second.load_online_objs(PERS_DEPOT_FILE);
		// обнуление оффлайновых полей и проставление ch для снятия бабла онлайн
		it->second.money = 0;
		it->second.money_spend = 0;
		it->second.offline_list.clear();
		it->second.ch = ch;
	}
}

/**
* Перевод онлайн списка в оффлайн.
*/
void CharNode::online_to_offline(ObjListType &cont)
{
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		depot_log("online_to_offline %s: %s %d %d", name.c_str(), (*obj_it)->short_description, GET_OBJ_UID(*obj_it), GET_OBJ_VNUM(*obj_it));
		OfflineNode tmp_obj;
		tmp_obj.vnum = GET_OBJ_VNUM(*obj_it);
		tmp_obj.timer = GET_OBJ_TIMER(*obj_it);
		tmp_obj.rent_cost = get_object_low_rent(*obj_it);
		tmp_obj.uid = GET_OBJ_UID(*obj_it);
		offline_list.push_back(tmp_obj);
		extract_obj(*obj_it);
		// плюсуем персональное хранилище к общей ренте
		add_cost_per_day(tmp_obj.rent_cost);
		// из макс.в мире в игре она уходит в ренту
		int rnum = real_object(tmp_obj.vnum);
		if (rnum >= 0)
			obj_index[rnum].stored++;
	}
	cont.clear();
}

/**
* Пересчет рнумов шмоток в хранилищах в случае добавления новых через олц.
*/
void renumber_obj_rnum(int rnum)
{
	depot_log("renumber_obj_rnum");
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end(); ++obj_it)
			if (GET_OBJ_RNUM(*obj_it) >= rnum)
				GET_OBJ_RNUM(*obj_it)++;
}

/**
* Выход чара из игры - перевод хранилищ в оффлайн (уход в ренту, форс-рента при лд, 'конец').
*/
void exit_char(CHAR_DATA *ch)
{
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		// тут лучше дернуть сейв руками до тика, т.к. есть вариант зайти и тут же выйти
		// в итоге к моменту сейва онлайн список будет пустой и все похерится
		it->second.save_online_objs();

		it->second.online_to_offline(it->second.pers_online);
		it->second.ch = 0;
		it->second.money = get_bank_gold(ch) + get_gold(ch);
		it->second.money_spend = 0;
	}
}

/**
* Релоад шмоток чара на случай отката.
*/
void reload_char(long uid, CHAR_DATA *ch)
{
	DepotListType::iterator it = create_depot(uid);
	if (it == depot_list.end())
	{
		log("Хранилище: UID %ld - возвращен некорректный уид персонажа.", uid);
		return;
	}
	// сначала обнуляем все, что надо
	it->second.reset();

	// после чего надо записать ему бабла, иначе при входе все спуржится (бабло проставит в exit_char)
	CHAR_DATA *vict = 0, *t_vict = 0;
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
		vict = d->character; // чар онлайн
	else
	{
		// чар соответственно оффлайн
		t_vict = new CHAR_DATA; // TODO: переделать на стек
		if (load_char(it->second.name.c_str(), t_vict) < 0)
		{
			// вообще эт нереальная ситуация после проверки в do_reboot
			send_to_char(ch, "Некорректное имя персонажа (%s).\r\n", it->second.name.c_str());
			delete t_vict;
			t_vict = 0;
		}
		vict = t_vict;
	}
	if (vict)
	{
		// вобщем тут мысль такая: кодить доп. обработку для релоада оффлайн чара мне стало лень,
		// поэтому мы штатно грузим ему сначала онлайн список, после чего отправляем его в оффлайн
		it->second.load_online_objs(PERS_DEPOT_FILE, 1);
		// если чара грузили с оффлайна
		if (!d) exit_char(vict);
	}
	if (t_vict) delete t_vict;

	sprintf(buf, "Depot: %s reload items for %s.", GET_NAME(ch), it->second.name.c_str());
	mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
	imm_log(buf);
}

/**
* Учет локейт в персональных хранилищах.
* \param count - оставшееся кол-во возможных показываемых шмоток после прохода по основному обж-списку.
*/
int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name)
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end(); ++obj_it)
		{
			if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
				continue;
			if (!isname(name.c_str(), (*obj_it)->name) || OBJ_FLAGGED((*obj_it), ITEM_NOLOCATE))
				continue;

			sprintf(buf, "%s находится у кого-то в персональном хранилище.\r\n", (*obj_it)->short_description);
			CAP(buf);
			send_to_char(buf, ch);

			if (--count <= 0)
				return count;
		}
	}
	return count;
}

} // namespace Depot
