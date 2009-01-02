// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <iomanip>
#include "parcel.hpp"
#include "db.h"
#include "interpreter.h"
#include "comm.h"
#include "char.hpp"
#include "handler.h"
#include "auction.h"
#include "screen.h"
#include "char_player.hpp"
#include "mail.h"

extern CHAR_DATA *get_player_of_name(const char *name);
extern int get_buf_line(char **source, char *target);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern void write_one_object(char **data, OBJ_DATA * object, int location);

namespace Parcel
{

const int KEEP_TIMER = 60 * 24 * 3; // 3 суток ждет на почте (в минутах)
const int SEND_COST = 100; // в любом случае снимается за посылку шмотки
const int RESERVED_COST_COEFF = 3; // цена ренты за 3 дня
const int MAX_SLOTS = 25; // сколько шмоток может находиться в отправке от одного игрока
const int RETURNED_TIMER = -1; // при развороте посылки идет двойной таймер шмоток без капания ренты
const char * FILE_NAME = LIB_DEPOT"parcel.db";

class Node
{
public:
	Node (int money, OBJ_DATA *obj) : money_(money), timer_(0), obj_(obj) {};
	Node () : money_(0), timer_(0), obj_(0) {};
	int money_; // резервированные средства
	int timer_; // сколько минут шмотка уже ждет получателя (при значении выще KEEP_TIMER возвращается отправителю)
	OBJ_DATA *obj_; // шмотка (здесь же берется таймер, при уходе в ноль - пурж и возврат оставшегося резерва)
};

typedef std::map<long /* уид отправителя */, std::list<Node> > SenderListType;
typedef std::map<long /* уид получателя */,  SenderListType> ParcelListType;

ParcelListType parcel_list;

void parcel_log(const char *format, ...)
{
	const char *filename = "../log/parcel.log";
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
		format = "SYSERR: // parcel_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
	fflush(file);
}

void invoice(long uid)
{
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
	{
		if (!has_parcel(d->character))
		{
			send_to_char(d->character, "%sВам пришла посылка, зайдите на почту и распишитесь!%s\r\n",
					CCWHT(d->character, C_NRM), CCNRM(d->character, C_NRM));
		}
	}
}

void add_parcel(long target, long sender, const Node &tmp_node)
{
	invoice(target);
	ParcelListType::iterator it = parcel_list.find(target);
	if (it != parcel_list.end())
	{
		SenderListType::iterator it2 = it->second.find(sender);
		if (it2 != it->second.end())
		{
			it2->second.push_back(tmp_node);
		}
		else
		{
			std::list<Node> tmp_list;
			tmp_list.push_back(tmp_node);
			it->second.insert(std::make_pair(sender, tmp_list));
		}
	}
	else
	{
		std::list<Node> tmp_list;
		tmp_list.push_back(tmp_node);
		SenderListType tmp_map;
		tmp_map.insert(std::make_pair(sender, tmp_list));
		parcel_list.insert(std::make_pair(target, tmp_map));
	}
}

int total_sended(CHAR_DATA *ch)
{
	int sended = 0;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		SenderListType::const_iterator it2 = it->second.find(GET_UNIQUE(ch));
		if (it2 != it->second.end())
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				++sended;
			}
		}
	}
	return sended;
}

void send_object(CHAR_DATA *ch, CHAR_DATA *mailman, long vict_uid, OBJ_DATA *obj)
{
	if (!ch || !mailman || !vict_uid || !obj)
	{
		log("Parcel: нулевой входной параметр: %d, %d, %d, %d (%s %s %d)",
				ch ? 1 : 0, mailman ? 1 : 0, vict_uid ? 1 : 0, obj ? 1 : 0, __FILE__, __func__, __LINE__);
		return;
	}

	const int cost = get_object_low_rent(obj) * RESERVED_COST_COEFF;
	if (get_bank_gold(ch) + get_gold(ch) < cost)
	{
		act("$n сказал$g Вам : 'Да у тебя ведь нет столько денег!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if (total_sended(ch) >= MAX_SLOTS)
	{
		act("$n сказал$g Вам : 'Ты уже и так отправил кучу вещей! Подожди, пока их получат адресаты!'",
				FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	std::string name = GetNameByUnique(vict_uid);
	if (name.empty())
	{
		act("$n сказал$g Вам : 'Ошибка в имени получателя, сообщите Богам!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	name_convert(name);
	send_to_char(ch,
			"Адресат: %s, отправлено:\r\n"
			"%s%s%s\r\n"
			"с Вас удержано %d %s и еще %d %s зарезервировано на 3 дня хранения.\r\n",
			name.c_str(), CCWHT(ch, C_NRM), GET_OBJ_PNAME(obj, 0), CCNRM(ch, C_NRM),
			SEND_COST, desc_count(SEND_COST, WHAT_MONEYa), cost, desc_count(cost, WHAT_MONEYa));

	Node tmp_node(cost, obj);
	add_parcel(vict_uid, GET_UNIQUE(ch), tmp_node);

	add_bank_gold(ch, -cost);
	add_bank_gold(ch, -SEND_COST);
	if (get_bank_gold(ch) < 0)
	{
		// выше мы убедились, что денег банк+руки как минимум не меньше, чем нужно
		add_gold(ch, get_bank_gold(ch));
		set_bank_gold(ch, 0);
	}
	save_char(ch);

	obj_from_char(obj);
	check_auction(NULL, obj);
	OBJ_DATA *temp;
	REMOVE_FROM_LIST(obj, object_list, next);
}

/**
* Проверка возможности отправить шмотку почтой.
* FIXME с кланами и перс.хранами почти копипаст.
*/
bool can_send(CHAR_DATA *ch, CHAR_DATA *mailman, OBJ_DATA *obj)
{
	if (OBJ_FLAGGED(obj, ITEM_ZONEDECAY)
			|| OBJ_FLAGGED(obj, ITEM_REPOP_DECAY)
			|| OBJ_FLAGGED(obj, ITEM_NOSELL)
			|| OBJ_FLAGGED(obj, ITEM_DECAY)
			|| OBJ_FLAGGED(obj, ITEM_NORENT)
			|| GET_OBJ_TYPE(obj) == ITEM_KEY
			|| GET_OBJ_RENT(obj) < 0
			|| GET_OBJ_RNUM(obj) <= NOTHING)
	{
		act("$n сказал$g Вам : 'Мы не отправляем такие вещи!'", FALSE, mailman, 0, ch, TO_VICT);
		return 0;
	}
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
	{
		snprintf(buf, MAX_STRING_LENGTH, "$n сказал$g Вам : 'В %s что-то лежит.'\r\n", OBJ_PAD(obj, 5));
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return 0;
	}
	return 1;
}


void send(CHAR_DATA *ch, CHAR_DATA *mailman, long vict_uid, char *arg)
{
	if (IS_NPC(ch)) return;

	if (IS_IMMORTAL(ch))
	{
		send_to_char("Не не не...\r\n" , ch);
		return;
	}
	if (GET_UNIQUE(ch) == vict_uid)
	{
		act("$n сказал$g Вам : 'Не загружай понапрасну почту!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if (RENTABLE(ch))
	{
		act("$n сказал$g Вам : 'Да у тебя руки по локоть в крови, проваливай!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	OBJ_DATA *obj;
	arg = one_argument(arg, buf);

	// TODO: все.имя, 10 имя
	if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
	{
		send_to_char(ch, "У Вас нет '%s'.\r\n", buf);
	}
	else if (can_send(ch, mailman, obj))
		send_object(ch, mailman, vict_uid, obj);
}

void print_sending_stuff(CHAR_DATA *ch)
{
	std::stringstream out;
	out << "\r\nВаши текущие посылки:";
	bool print = false;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		SenderListType::const_iterator it2 = it->second.find(GET_UNIQUE(ch));
		if (it2 != it->second.end())
		{
			print = true;
			std::string name = GetNameByUnique(it->first);
			name_convert(name);
			out << "\r\nАдресат: " << name << ", отправлено:\r\n" << CCWHT(ch, C_NRM);

			int money = 0;
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				out << GET_OBJ_PNAME(it3->obj_, 0) << "\r\n";
				money += it3->money_;
			}
			out << CCNRM(ch, C_NRM)
				<< money << " " << desc_count(money, WHAT_MONEYa) << " зарезервировано на 3 дня хранения.\r\n";
		}
	}
	if (print)
		send_to_char(out.str(), ch);
}

int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name)
{
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
					continue;
				if (!isname(name.c_str(), it3->obj_->name))
					continue;

				snprintf(buf, MAX_STRING_LENGTH, "%s находится у почтового голубя в инвентаре.\r\n", it3->obj_->short_description);
				CAP(buf);
				send_to_char(buf, ch);

				if (--count <= 0)
					return count;
			}
		}
	}
	return count;
}

bool has_parcel(CHAR_DATA *ch)
{
	ParcelListType::const_iterator it = parcel_list.find(GET_UNIQUE(ch));
	if (it != parcel_list.end())
		return true;
	else
		return false;
}

const bool RETURN_WITH_MONEY = 1;
const bool RETURN_NO_MONEY = 0;

void return_money(std::string const &name, int money, bool add)
{
	if (!money) return;

	CHAR_DATA *vict = 0;
	if ((vict = get_player_of_name(name.c_str())))
	{
		if (add)
		{
			add_bank_gold(vict, money);
			send_to_char(vict, "%sВы получили %d %s банковским переводом от почтовой службы%s.\r\n",
					CCWHT(vict, C_NRM), money, desc_count(money, WHAT_MONEYa), CCNRM(vict, C_NRM));
		}
	}
	else
	{
		vict = new CHAR_DATA; // TODO: переделать на стек
		if (load_char(name.c_str(), vict) < 0)
		{
			delete vict;
			return;
		}
		add_bank_gold(vict, money);
		save_char(vict);
		delete vict;
	}
}

void fill_ex_desc(CHAR_DATA *ch, OBJ_DATA *obj, std::string sender)
{
	CREATE(obj->ex_description, EXTRA_DESCR_DATA, 1);
	obj->ex_description->keyword = str_dup("посылка бандероль пакет ящик parcel box case chest");
	obj->ex_description->next = 0;

	int size = MAX(strlen(GET_NAME(ch)), sender.size());
	std::stringstream out;
	out.setf(std::ios_base::left);

	out << "   Разваливающийся на глазах ящик с явными признаками\r\n"
			"неуклюжего взлома - царская служба безопасности бдит...\r\n"
			"На табличке сбоку видны надписи:\r\n\r\n";
	out << std::setw(size + 16) << std::setfill('-') << " " << std::setfill(' ') << "\r\n";
	out << "| Отправитель: " << std::setw(size) << sender
		<< " |\r\n|  Получатель: " << std::setw(size) << GET_NAME(ch) << " |\r\n";
	out << std::setw(size + 16) << std::setfill('-') << " " << std::setfill(' ') << "\r\n";

	obj->ex_description->description = str_dup(out.str().c_str());
}

int calculate_timer_cost(std::list<Node>::iterator const &it)
{
	double tmp = static_cast<double>(get_object_low_rent(it->obj_))/(24*60);
	return tmp * it->timer_;
}

void receive(CHAR_DATA *ch, CHAR_DATA *mailman)
{
	ParcelListType::iterator it = parcel_list.find(GET_UNIQUE(ch));
	if (it != parcel_list.end())
	{
		for (SenderListType::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			std::string name = GetNameByUnique(it2->first);
			name_convert(name);

			OBJ_DATA *obj = create_obj();
			obj->name = str_dup("посылка бандероль пакет ящик parcel box case chest");
			obj->short_description = str_dup("посылка");
			obj->description = str_dup("Кто-то забыл здесь свою посылку.");
			obj->PNames[0] = str_dup("посылка");
			obj->PNames[1] = str_dup("посылки");
			obj->PNames[2] = str_dup("посылке");
			obj->PNames[3] = str_dup("посылку");
			obj->PNames[4] = str_dup("посылкой");
			obj->PNames[5] = str_dup("посылке");
			GET_OBJ_SEX(obj) = SEX_FEMALE;
			GET_OBJ_TYPE(obj) = ITEM_CONTAINER;
			GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
			GET_OBJ_WEIGHT(obj) = 1;
			GET_OBJ_COST(obj) = 1;
			GET_OBJ_RENT(obj) = 1;
			GET_OBJ_RENTEQ(obj) = 1;
			GET_OBJ_TIMER(obj) = 24 * 60;
			SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NOSELL), ITEM_NOSELL);
			SET_BIT(GET_OBJ_EXTRA(obj, ITEM_DECAY), ITEM_DECAY);
			fill_ex_desc(ch, obj, name);

			int money = 0;
			for (std::list<Node>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				money += it3->money_ - calculate_timer_cost(it3);
				// добавляем в глоб.список и кладем в посылку
				it3->obj_->next = object_list;
				object_list = it3->obj_;
				obj_to_obj(it3->obj_, obj);
			}
			return_money(name, money, RETURN_WITH_MONEY);

			obj_to_char(obj, ch);
			snprintf(buf, MAX_STRING_LENGTH, "$n дал$g Вам посылку (отправитель %s).", name.c_str());
			act(buf, FALSE, mailman, 0, ch, TO_VICT);
			act("$N дал$G $n2 посылку.", FALSE, ch, 0, mailman, TO_ROOM);
		}
		parcel_list.erase(it);
	}
}

void create_mail(long to, long from, char *text)
{
	store_mail(to, from, text);
	DESCRIPTOR_DATA* i = get_desc_by_id(to);
	if (i)
		send_to_char(i->character, "%sВам пришло письмо, зайдите на почту и распишитесь!%s\r\n",
				CCWHT(i->character, C_NRM), CCNRM(i->character, C_NRM));
}

SenderListType return_list;

void prepare_return(const long uid, const std::list<Node>::iterator &it)
{
	Node tmp_node(0, it->obj_);
	tmp_node.timer_ = RETURNED_TIMER;

	SenderListType::iterator it2 = return_list.find(uid);
	if (it2 != return_list.end())
	{
		it2->second.push_back(tmp_node);
	}
	else
	{
		std::list<Node> tmp_list;
		tmp_list.push_back(tmp_node);
		return_list.insert(std::make_pair(uid, tmp_list));
	}
}

void return_parcel()
{
	for (SenderListType::iterator it = return_list.begin(); it != return_list.end(); ++it)
	{
		for (std::list<Node>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			Node tmp_node(it2->money_, it2->obj_);
			tmp_node.timer_ = RETURNED_TIMER;
			add_parcel(it->first, it->first, tmp_node);
		}
	}
	return_list.clear();
}

void extract_parcel(const long sender_uid, const long target_uid, const std::list<Node>::iterator &it)
{
	long sender_id = get_id_by_uid(sender_uid);
	long target_id = get_id_by_uid(target_uid);
	snprintf(buf, MAX_STRING_LENGTH, "С прискорбием сообщаем Вам: %s рассыпал%s в прах.\r\n",
			it->obj_->short_description, GET_OBJ_SUF_2(it->obj_));

	char *tmp = str_dup(buf);
	// -1 в качестве ид отправителя при получении подставит в имя почтовую службу
	create_mail(sender_id, -1, tmp);
	create_mail(target_id, -1, tmp);
	free(tmp);

	// возврат оставшихся зарезервированных кун отправителю (у развернутых уже ноль)
	if (it->money_ && it->timer_ != RETURNED_TIMER)
	{
		int money_return = it->money_ - calculate_timer_cost(it);
		std::string name = GetNameByUnique(sender_uid);
		return_money(name, money_return, RETURN_WITH_MONEY);
	}

	extract_obj(it->obj_);
}

void return_invoice(long uid, OBJ_DATA *obj)
{
	long target_id = get_id_by_uid(uid);
	snprintf(buf, MAX_STRING_LENGTH, "Посылка возвращена отправителю: %s.\r\n", obj->short_description);

	char *tmp = str_dup(buf);
	create_mail(target_id, -1, tmp);
	free(tmp);
}

class LoadNode
{
public:
	Node obj_node;
	long sender;
	long target;
};

LoadNode parcel_read_one_object(char **data, int *error)
{
	LoadNode tmp_node;

	*error = 1;
	// Станем на начало предмета (#)
	for (; **data != '#'; (*data)++)
		if (!**data || **data == '$')
			return tmp_node;

	// Пропустим #
	(*data)++;
	char buffer[MAX_STRING_LENGTH];

	*error = 2;
	// отправитель
	if (!get_buf_line(data, buffer))
		return tmp_node;
	*error = 3;
	log("3: %s", buffer);
	if ((tmp_node.target = atol(buffer)) <= 0)
		return tmp_node;

	*error = 4;
	// получатель
	if (!get_buf_line(data, buffer))
		return tmp_node;
	*error = 5;
	log("5: %s", buffer);
	if ((tmp_node.sender = atol(buffer)) <= 0)
		return tmp_node;

	*error = 6;
	// зарезервированная сумма
	log("6: %s", buffer);
	if (!get_buf_line(data, buffer))
		return tmp_node;
	*error = 7;
	if ((tmp_node.obj_node.money_ = atoi(buffer)) < 0)
		return tmp_node;

	*error = 8;
	// таймер ожидания на почте
	if (!get_buf_line(data, buffer))
		return tmp_node;
	*error = 9;
	if ((tmp_node.obj_node.timer_ = atoi(buffer)) < -1)
		return tmp_node;

	*error = 0;
	tmp_node.obj_node.obj_ = read_one_object_new(data, error);

	if (*error)
		*error = 10;

	return tmp_node;
}


void load()
{
	FILE *fl;
	if (!(fl = fopen(LIB_DEPOT"parcel.db", "r")))
	{
		log("SYSERR: Error opening parcel database.");
		return;
	}

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);

	char *data, *readdata;
	CREATE(readdata, char, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	if (!fread(readdata, fsize, 1, fl) || ferror(fl))
	{
		fclose(fl);
		log("SYSERR: Memory error or cann't read parcel database file.");
		free(readdata);
		return;
	};
	fclose(fl);

	data = readdata;
	*(data + fsize) = '\0';

	for (fsize = 0; *data && *data != '$'; fsize++)
	{
		int error;
		LoadNode node = parcel_read_one_object(&data, &error);

		if (!node.obj_node.obj_)
		{
			log("SYSERR: Error #%d reading parcel database file.", error);
			return;
		}

		if (error)
		{
			log("SYSERR: Error #%d reading item from parcel database.", error);
			return;
		}
		add_parcel(node.target, node.sender, node.obj_node);
		// из глобального списка изымаем
		OBJ_DATA *temp;
		REMOVE_FROM_LIST(node.obj_node.obj_, object_list, next);

	}

	free(readdata);
}

void save()
{
	std::stringstream out;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				out << "#" << it->first << "\n" << it2->first << "\n" << it3->money_ << "\n" << it3->timer_ << "\n\n";
				char databuf[MAX_STRING_LENGTH];
				char *data = databuf;
				write_one_object(&data, it3->obj_, 0);
				out << databuf << "\n";
			}
		}
	}
	out << "$\n$\n";

	// скидываем в файл
	std::ofstream file(FILE_NAME);
	if (!file.is_open())
	{
		log("SYSERR: error opening file: %s! (%s %s %d)", FILE_NAME, __FILE__, __func__, __LINE__);
		return;
	}
	file << out.rdbuf();
	file.close();

	return;
}

void update_timers()
{
	for (ParcelListType::iterator it = parcel_list.begin(); it != parcel_list.end(); /* empty */)
	{
		for (SenderListType::iterator it2 = it->second.begin(); it2 != it->second.end(); /* empty */)
		{
			std::list<Node>::iterator tmp_it;
			for (std::list<Node>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); it3 = tmp_it)
			{
				tmp_it = it3;
				++tmp_it;

				--GET_OBJ_TIMER(it3->obj_);
				if (GET_OBJ_TIMER(it3->obj_) <= 0)
				{
					extract_parcel(it2->first, it->first, it3);
					it2->second.erase(it3);
				}
				else
				{
					if (it3->timer_ == RETURNED_TIMER)
					{
						// шмотка уже развернута отправителю, рента не капает, но таймер идет два раза
						--GET_OBJ_TIMER(it3->obj_);
						if (GET_OBJ_TIMER(it3->obj_) <= 0)
						{
							extract_parcel(it2->first, it->first, it3);
							it2->second.erase(it3);
						}
					}
					else
					{
						++it3->timer_;
						if (it3->timer_ >= KEEP_TIMER)
						{
							return_invoice(it->first, it3->obj_);
							prepare_return(it2->first, it3);
							// тут надо штатно уменьшить счетчики у плеера
							std::string name = GetNameByUnique(it2->first);
							return_money(name, it3->money_, RETURN_NO_MONEY);
							// и удалить запись (она уйдет в разворот позже в return_parcel)
							it2->second.erase(it3);
						}
					}
				}
			}
			if (it2->second.empty())
				it->second.erase(it2++);
			else
				++it2;
		}
		if (it->second.empty())
			parcel_list.erase(it++);
		else
			++it;
	}
	return_parcel();
	save();
}

void show_stats(CHAR_DATA *ch)
{
	int targets = 0, returned = 0, objs = 0, reserved_money = 0;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		++targets;
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				++objs;
				reserved_money += it3->money_;
				if (it3->timer_ == RETURNED_TIMER)
					++returned;
			}
		}
	}
	send_to_char(ch, "  Почта: получателей %d, возвращено %d, предметов %d, зарезервировано %d\r\n", targets, returned, objs, reserved_money);
}

/**
* Пересчет рнумов шмоток на почте в случае добавления новых через олц.
*/
void renumber_obj_rnum(int rnum)
{
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (GET_OBJ_RNUM(it3->obj_) >= rnum)
					GET_OBJ_RNUM(it3->obj_)++;
			}
		}
	}
}

int print_imm_where_obj(CHAR_DATA *ch, char *arg, int num)
{
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (isname(arg, it3->obj_->name))
				{
					std::string target = GetNameByUnique(it->first);
					std::string sender = GetNameByUnique(it2->first);

					send_to_char(ch, "O%3d. %-25s - находится на почте (отправитель: %s, получатель: %s).\r\n",
							num++, it3->obj_->short_description, sender.c_str(), target.c_str());
				}
			}
		}
	}
	return num;
}

} // namespace Parcel
