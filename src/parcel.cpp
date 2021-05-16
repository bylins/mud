// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "parcel.hpp"

#include "world.objects.hpp"
#include "logger.hpp"
#include "obj.hpp"
#include "char_obj_utils.inl"
#include "db.h"
#include "interpreter.h"
#include "comm.h"
#include "chars/character.h"
#include "handler.h"
#include "auction.h"
#include "screen.h"
#include "chars/char_player.hpp"
#include "mail.h"
#include "name_list.hpp"
#include "room.hpp"
#include "objsave.h"

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

extern CHAR_DATA *get_player_of_name(const char *name);
extern int get_buf_line(char **source, char *target);
extern void olc_update_object(int robj_num, OBJ_DATA *obj, OBJ_DATA *olc_proto);
extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);

namespace Parcel
{
void parcel_log(const char *format, ...) __attribute__((format(printf,1,2)));

const int KEEP_TIMER = 60 * 24 * 3; // 3 суток ждет на почте (в минутах)
const int SEND_COST = 100; // в любом случае снимается за посылку шмотки
const int RESERVED_COST_COEFF = 3; // цена ренты за 3 дня
const int MAX_SLOTS = 25; // сколько шмоток может находиться в отправке от одного игрока
const int RETURNED_TIMER = -1; // при развороте посылки идет двойной таймер шмоток без капания ренты
const char *FILE_NAME = LIB_DEPOT"parcel.db";

// для возврата посылки отправителю
const bool RETURN_WITH_MONEY = 1;
const bool RETURN_NO_MONEY = 0;

// доставленные с ребута посылки
static int was_sended = 0;

// для групповой отсылки шмоток (чтобы не спамить на каждую)
static std::string send_buffer;
static int send_cost_buffer = 0;
static int send_reserved_buffer = 0;

class Node
{
public:
	Node (int money, const OBJ_DATA::shared_ptr& obj) : money_(money), timer_(0), obj_(obj) {};
	Node () : money_(0), timer_(0), obj_(nullptr) {};
	int money_; // резервированные средства
	int timer_; // сколько минут шмотка уже ждет получателя (при значении выще KEEP_TIMER возвращается отправителю)
	OBJ_DATA::shared_ptr obj_; // шмотка (здесь же берется таймер, при уходе в ноль - пурж и возврат оставшегося резерва)
};

class LoadNode
{
public:
	Node obj_node;
	long sender;
	long target;
};

typedef std::map<long /* уид отправителя */, std::list<Node> > SenderListType;
typedef std::map<long /* уид получателя */,  SenderListType> ParcelListType;

ParcelListType parcel_list; // список посылок
SenderListType return_list; // временный список на возврат

// * Отдельный лог для отладки.
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

// * Уведомление чара (если он онлайна) о новой посылке.
void invoice(long uid)
{
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
	{
		if (!has_parcel(d->character.get()))
		{
			send_to_char(d->character.get(), "%sВам пришла посылка, зайдите на почту и распишитесь!%s\r\n",
				CCWHT(d->character, C_NRM), CCNRM(d->character, C_NRM));
		}
	}
}

// * Добавление шмотки в список посылок.
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

// * Сколько всего предметов уже посылается данным персонажем (для ограничения).
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

// * Проверка возможности отправить шмотку почтой.
bool can_send(CHAR_DATA *ch, CHAR_DATA *mailman, OBJ_DATA *obj, long vict_uid)
{
	if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP)
		|| obj->get_extra_flag(EExtraFlag::ITEM_NORENT)
		|| obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY)
		|| obj->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY)
		|| obj->get_extra_flag(EExtraFlag::ITEM_DECAY)
		|| obj->get_extra_flag(EExtraFlag::ITEM_NORENT)
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_KEY
		|| GET_OBJ_RENT(obj) < 0
		|| GET_OBJ_RNUM(obj) <= NOTHING
		|| GET_OBJ_OWNER(obj))
	{
		snprintf(buf, MAX_STRING_LENGTH, "$n сказал$g вам : '%s - мы не отправляем такие вещи!'\r\n",
			obj->get_PName(0).c_str());
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return 0;
	}
	else if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER
		&& obj->get_contains())
	{
		snprintf(buf, MAX_STRING_LENGTH, "$n сказал$g вам : 'В %s что-то лежит.'\r\n", obj->get_PName(5).c_str());
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return 0;
	}
	else if (SetSystem::is_big_set(obj))
	{
		snprintf(buf, MAX_STRING_LENGTH, "$n сказал$g вам : '%s является частью большого набора предметов.'\r\n",
			obj->get_PName(0).c_str());
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return 0;
	}
	Player t_vict;
	if (load_char(GetNameByUnique(vict_uid).c_str(), &t_vict) < 0)
	{
		return 0;
	}
	if (invalid_anti_class(&t_vict, obj))
	{
			switch (GET_SEX(&t_vict))
			{
			case ESex::SEX_MALE:
				act("$n сказал$g вам : 'Знаю я такого добра молодца - эта вещь явно на него не налезет.'\r\n",
					FALSE, mailman, 0, ch, TO_VICT);
				break;

			case ESex::SEX_FEMALE:
				act("$n сказал$g вам : 'Знаю я такую красну девицу - эта вещь явно на нее не налезет.'\r\n",
					FALSE, mailman, 0, ch, TO_VICT);
				break;

			default:
				act("$n сказал$g вам : 'Знаю я сие чудо бесполое - эта вещь явно на него не налезет.'\r\n",
					FALSE, mailman, 0, ch, TO_VICT);
		}
		return 0;
	}
	return 1;
}

// получаем все объекты, которые были отправлены чару
std::vector<int> get_objs(long char_uid)
{
	std::vector<int> buf_vector;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		SenderListType::const_iterator it2 = it->second.find(char_uid);
		if (it2 != it->second.end())
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				buf_vector.push_back((*it3).obj_->get_vnum());
			}
		}
	}
	return buf_vector;
}

// * Отправка предмета (снятие/резервирование денег, вывод из списка предметов).
void send_object(CHAR_DATA *ch, CHAR_DATA *mailman, long vict_uid, OBJ_DATA* obj)
{
	if (!ch || !mailman || !vict_uid || !obj)
	{
		log("Parcel: нулевой входной параметр: %d, %d, %d, %d (%s %s %d)",
				ch ? 1 : 0, mailman ? 1 : 0, vict_uid ? 1 : 0, obj ? 1 : 0, __FILE__, __func__, __LINE__);
		return;
	}

	if (!can_send(ch, mailman, obj, vict_uid)) return;

	const int reserved_cost = get_object_low_rent(obj) * RESERVED_COST_COEFF;
	const int total_cost = reserved_cost + SEND_COST;

	if (ch->get_total_gold() < total_cost)
	{
		act("$n сказал$g вам : 'Да у тебя ведь нет столько денег!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if (total_sended(ch) >= MAX_SLOTS)
	{
		act("$n сказал$g вам : 'Ты уже и так отправил кучу вещей! Подожди, пока их получат адресаты!'",
				FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	std::string name = GetNameByUnique(vict_uid);
	if (name.empty())
	{
		act("$n сказал$g вам : 'Ошибка в имени получателя, сообщите Богам!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if (SetSystem::is_norent_set(ch, obj)
		&& SetSystem::is_norent_set(GET_OBJ_VNUM(obj), get_objs(GET_UNIQUE(ch))))
	{
		snprintf(buf, MAX_STRING_LENGTH, "%s - требуется две и более вещи из набора.\r\n", obj->get_PName(0).c_str());
		send_to_char(CAP(buf), ch);
		return;
	}
	name_convert(name);

	if (send_buffer.empty())
		send_buffer += "Адресат: " + name + ", отправлено:\r\n";

	snprintf(buf, sizeof(buf), "%s%s%s\r\n", CCWHT(ch, C_NRM), GET_OBJ_PNAME(obj, 0).c_str(), CCNRM(ch, C_NRM));
	send_buffer += buf;

	const auto object_ptr = world_objects.get_by_raw_ptr(obj);
	Node tmp_node(reserved_cost, object_ptr);
	add_parcel(vict_uid, GET_UNIQUE(ch), tmp_node);

	send_reserved_buffer += reserved_cost;
	send_cost_buffer += SEND_COST;

	ch->remove_both_gold(total_cost);
	obj_from_char(obj);
	ObjSaveSync::add(ch->get_uid(), ch->get_uid(), ObjSaveSync::PARCEL_SAVE);

	check_auction(NULL, obj);
	world_objects.remove(obj);
}

// * Отправка предмета, дергается из спешиала почты ('отправить имя предмет)'.
void send(CHAR_DATA *ch, CHAR_DATA *mailman, long vict_uid, char *arg)
{
	if (IS_NPC(ch)) return;

	if (GET_UNIQUE(ch) == vict_uid)
	{
		act("$n сказал$g вам : 'Не загружай понапрасну почту!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if (RENTABLE(ch))
	{
		act("$n сказал$g вам : 'Да у тебя руки по локоть в крови, проваливай!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}


	OBJ_DATA *obj, *next_obj;
	char tmp_arg[MAX_INPUT_LENGTH];
	char tmp_arg2[MAX_INPUT_LENGTH];

	two_arguments(arg, tmp_arg, tmp_arg2);

	if (is_number(tmp_arg))
	{
		int amount = atoi(tmp_arg);
		if (!strn_cmp("coin", tmp_arg2, 4) || !strn_cmp("кун", tmp_arg2, 5) || !str_cmp("денег", tmp_arg2))
		{
			act("$n сказал$g вам : 'Для перевода денег воспользуйтесь услугами банка.'", FALSE, mailman, 0, ch, TO_VICT);
			return;
		}
		else if (!str_cmp("все", tmp_arg2) || !str_cmp("all", tmp_arg2))
		{
			if (!ch->carrying)
			{
				send_to_char("У вас ведь ничего нет.\r\n", ch);
				return;
			}
			for (obj = ch->carrying; obj && amount; obj = next_obj)
			{
				--amount;
				next_obj = obj->get_next_content();
				send_object(ch, mailman, vict_uid, obj);
			}
		}
		else if (!*tmp_arg2)
		{
			send_to_char(ch, "Чего %d вы хотите отправить?\r\n", amount);
		}
		else if (!(obj = get_obj_in_list_vis(ch, tmp_arg2, ch->carrying)))
		{
			send_to_char(ch, "У вас нет '%s'.\r\n", tmp_arg2);
		}
		else
		{
			while (obj && amount--)
			{
				next_obj = get_obj_in_list_vis(ch, tmp_arg2, obj->get_next_content());
				send_object(ch, mailman, vict_uid, obj);
				obj = next_obj;
			}
		}
	}
	else
	{
		int dotmode = find_all_dots(tmp_arg);
		if (dotmode == FIND_INDIV)
		{
			if (!(obj = get_obj_in_list_vis(ch, tmp_arg, ch->carrying)))
			{
				send_to_char(ch, "У вас нет '%s'.\r\n", tmp_arg);
				return;
			}
			send_object(ch, mailman, vict_uid, obj);
		}
		else
		{
			if (dotmode == FIND_ALLDOT && !*tmp_arg)
			{
				send_to_char("Отправить \"все\" какого типа предметов?\r\n", ch);
				return;
			}
			if (!ch->carrying)
			{
				send_to_char("У вас ведь ничего нет.\r\n", ch);
			}
			else
			{
				bool has_items = false;
				for (obj = ch->carrying; obj; obj = next_obj)
				{
					next_obj = obj->get_next_content();
					if (CAN_SEE_OBJ(ch, obj)
						&& ((dotmode == FIND_ALL
							|| isname(tmp_arg, obj->get_aliases()))))
					{
						send_object(ch, mailman, vict_uid, obj);
						has_items = true;
					}
				}
				if (!has_items)
					send_to_char(ch, "У вас нет '%s'.\r\n", tmp_arg);
			}
		}
	}

	if (!send_buffer.empty())
	{
		snprintf(buf, sizeof(buf), "с вас удержано %d %s и еще %d %s зарезервировано на 3 дня хранения.\r\n",
			send_cost_buffer, desc_count(send_cost_buffer, WHAT_MONEYa),
			send_reserved_buffer, desc_count(send_reserved_buffer, WHAT_MONEYa));
		send_buffer += buf;
		send_to_char(send_buffer.c_str(), ch);

		send_buffer = "";
		send_cost_buffer = 0;
		send_reserved_buffer = 0;
	}
}

// * Дергается из спешиала почты ('почта'). Распечатка отправленных посылок, которые еще не доставлены.
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

// * Для учитывания предметов на почте в локейте.
int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name)
{
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (!IS_GOD(ch))
				{
					if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
					{
						continue;
					}					
				}

				if (it3->obj_->get_extra_flag(EExtraFlag::ITEM_NOLOCATE)
					&& !IS_GOD(ch))
				{
					continue;
				}

				if (!isname(name.c_str(), it3->obj_->get_aliases()))
				{
					continue;
				}

				snprintf(buf, MAX_STRING_LENGTH, "%s наход%sся у почтового голубя в инвентаре.\r\n",
					it3->obj_->get_short_description().c_str(), GET_OBJ_POLY_1(ch, it3->obj_));
//				CAP(buf); issue #59
				send_to_char(buf, ch);

				if (--count <= 0)
				{
					return count;
				}
			}
		}
	}
	return count;
}

// * Есть ли на чара какие-нить посылки.
bool has_parcel(CHAR_DATA *ch)
{
	ParcelListType::const_iterator it = parcel_list.find(GET_UNIQUE(ch));
	if (it != parcel_list.end())
		return true;
	else
		return false;
}

// * Возврат зарезервированных денег отправителю.
void return_money(std::string const &name, int money, bool add)
{
	if (money <= 0)
	{
		log("WARNING: money=%d (%s %s %d)", money, __FILE__, __func__, __LINE__);
		return;
	}

	CHAR_DATA *vict = 0;
	if ((vict = get_player_of_name(name.c_str())))
	{
		if (add)
		{
			vict->add_bank(money);
			send_to_char(vict, "%sВы получили %d %s банковским переводом от почтовой службы%s.\r\n",
					CCWHT(vict, C_NRM), money, desc_count(money, WHAT_MONEYu), CCNRM(vict, C_NRM));
		}
	}
	else
	{
		vict = new Player; // TODO: переделать на стек
		if (load_char(name.c_str(), vict) < 0)
		{
			delete vict;
			return;
		}
		vict->add_bank(money);
		vict->save_char();
		delete vict;
	}
}

// * Экстра-описание на самой посылке при получении.
void fill_ex_desc(CHAR_DATA *ch, OBJ_DATA *obj, std::string sender)
{
	size_t size = std::max(strlen(GET_NAME(ch)), sender.size());
	std::stringstream out;
	out.setf(std::ios_base::left);

	out << "   Разваливающийся на глазах ящик с явными признаками\r\n"
			"неуклюжего взлома - царская служба безопасности бдит...\r\n"
			"На табличке сбоку видны надписи:\r\n\r\n";
	out << std::setw(size + 16) << std::setfill('-') << " " << std::setfill(' ') << "\r\n";
	out << "| Отправитель: " << std::setw(size) << sender
		<< " |\r\n|  Получатель: " << std::setw(size) << GET_NAME(ch) << " |\r\n";
	out << std::setw(size + 16) << std::setfill('-') << " " << std::setfill(' ') << "\r\n";

	obj->set_ex_description("посылка бандероль пакет ящик parcel box case chest", out.str().c_str());
}

// * Расчет стоимости ренты за предмет, пока он лежал на почте.
int calculate_timer_cost(std::list<Node>::iterator const &it)
{
	return static_cast<int>((get_object_low_rent(it->obj_.get()) / (24.0 * 60.0)) * it->timer_);
}

// * Генерим сам контейнер посылку.
OBJ_DATA * create_parcel()
{
	const auto obj = world_objects.create_blank();

	obj->set_aliases("посылка бандероль пакет ящик parcel box case chest");
	obj->set_short_description("посылка");
	obj->set_description("Кто-то забыл здесь свою посылку.");
	obj->set_PName(0, "посылка");
	obj->set_PName(1, "посылки");
	obj->set_PName(2, "посылке");
	obj->set_PName(3, "посылку");
	obj->set_PName(4, "посылкой");
	obj->set_PName(5, "посылке");
	obj->set_sex(ESex::SEX_FEMALE);
	obj->set_type(OBJ_DATA::ITEM_CONTAINER);
	obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));
	obj->set_weight(1);
	obj->set_cost(1);
	obj->set_rent_off(1);
	obj->set_rent_on(1);
	obj->set_timer(24 * 60);
	obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);
	obj->set_extra_flag(EExtraFlag::ITEM_DECAY);

	return obj.get();
}

// * Получение посылки на почте, дергается из спешиала почты. ('получить').
void receive(CHAR_DATA *ch, CHAR_DATA *mailman)
{
	if (((ch->in_room == r_helled_start_room) ||
		(ch->in_room == r_named_start_room) ||
		(ch->in_room == r_unreg_start_room)) &&
		has_parcel(ch))
	{
		act("$n сказал$g вам : 'А посылку-то не получишь, сюда не доставляем.'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	ParcelListType::iterator it = parcel_list.find(GET_UNIQUE(ch));
	if (it != parcel_list.end())
	{
		for (SenderListType::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			std::string name = GetNameByUnique(it2->first);
			name_convert(name);

			OBJ_DATA *obj = create_parcel();
			fill_ex_desc(ch, obj, name);

			int money = 0;
			for (std::list<Node>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				money += it3->money_ - calculate_timer_cost(it3);
				// добавляем в глоб.список и кладем в посылку
				world_objects.add(it3->obj_);
				obj_to_obj(it3->obj_.get(), obj);
			}
			return_money(name, money, RETURN_WITH_MONEY);

			obj_to_char(obj, ch);
			snprintf(buf, MAX_STRING_LENGTH, "$n дал$g вам посылку (отправитель %s).", name.c_str());
			act(buf, FALSE, mailman, 0, ch, TO_VICT);
			act("$N дал$G $n2 посылку.", FALSE, ch, 0, mailman, TO_ROOM);
			++was_sended;
		}
		ObjSaveSync::add(ch->get_uid(), ch->get_uid(), ObjSaveSync::PARCEL_SAVE);
		parcel_list.erase(it);
	}
}

// * Отправка сообщения через письмо, с уведомлением чару, если тот онлайн.
void create_mail(int to_uid, int from_uid, char *text)
{
	mail::add(to_uid, from_uid, text);
	const DESCRIPTOR_DATA* i = DescByUID(to_uid);
	if (i)
	{
		send_to_char(i->character.get(), "%sВам пришло письмо, зайдите на почту и распишитесь!%s\r\n",
			CCWHT(i->character, C_NRM), CCNRM(i->character, C_NRM));
	}
}

// * Формирование временного списка возвращенных предметов (из основного удалены).
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

// * Возврат предметов из временного списка (перекидывание их в основной список посылок).
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

// * Дикей предмета на почте и уведомление об этом отправителя и получателя через письма.
void extract_parcel(int sender_uid, int target_uid, const std::list<Node>::iterator &it)
{
	snprintf(buf, MAX_STRING_LENGTH, "С прискорбием сообщаем вам: %s рассыпал%s в прах.\r\n",
		it->obj_->get_short_description().c_str(),
		GET_OBJ_SUF_2(it->obj_));

	char *tmp = str_dup(buf);
	// -1 в качестве ид отправителя при получении подставит в имя почтовую службу
	create_mail(sender_uid, -1, tmp);
	create_mail(target_uid, -1, tmp);
	free(tmp);

	// возврат оставшихся зарезервированных кун отправителю (у развернутых уже ноль)
	if (it->money_ && it->timer_ != RETURNED_TIMER)
	{
		int money_return = it->money_ - calculate_timer_cost(it);
		std::string name = GetNameByUnique(sender_uid);
		return_money(name, money_return, RETURN_WITH_MONEY);
	}

	extract_obj(it->obj_.get());
}

// * Генерация письма о возврате посылки.
void return_invoice(int uid, OBJ_DATA *obj)
{
	snprintf(buf, MAX_STRING_LENGTH, "Посылка возвращена отправителю: %s.\r\n",
		obj->get_short_description().c_str());
	char *tmp = str_dup(buf);
	create_mail(uid, -1, tmp);
	free(tmp);
}

// * Чтение записи, включающей инфу почты и сам предмет после нее.
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
	if ((tmp_node.target = atol(buffer)) <= 0)
		return tmp_node;

	*error = 4;
	// получатель
	if (!get_buf_line(data, buffer))
		return tmp_node;
	*error = 5;
	if ((tmp_node.sender = atol(buffer)) <= 0)
		return tmp_node;

	*error = 6;
	// зарезервированная сумма
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

// * Загрузка предметов и тех.информации (при ребуте).
void load()
{
	FILE *fl;
	if (!(fl = fopen(FILE_NAME, "r")))
	{
		log("SYSERR: Error opening parcel database.");
		return;
	}

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);

	char *data, *readdata;
	CREATE(readdata, fsize + 1);
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
		world_objects.remove(node.obj_node.obj_);
	}

	free(readdata);
}

// * Сохранение предметов и тех.информации (при апдейте таймеров).
void save()
{
	log("Save obj: parcel");
	ObjSaveSync::check(0, ObjSaveSync::PARCEL_SAVE);

	std::stringstream out;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				out << "#" << it->first << "\n" << it2->first << "\n" << it3->money_ << "\n" << it3->timer_ << "\n\n";
				write_one_object(out, it3->obj_.get(), 0);
				out << "\n";
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

// * Обновление таймеров у предметов + таймеров ожидания на почте. Пурж/возврат по надобности.
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

				it3->obj_->dec_timer();
				if (it3->obj_->get_timer() <= 0)
				{
					extract_parcel(it2->first, it->first, it3);
					it2->second.erase(it3);
				}
				else
				{
					if (it3->timer_ == RETURNED_TIMER)
					{
						// шмотка уже развернута отправителю, рента не капает, но таймер идет два раза
						it3->obj_->dec_timer();
						if (it3->obj_->get_timer() <= 0)
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
							return_invoice(it->first, it3->obj_.get());
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

// * Иммский 'show stats' интересу и статистики ради.
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
	send_to_char(ch, "  Почта: предметов в ожидании %d, доставлено с ребута %d\r\n", objs, was_sended);
}

int delete_obj(int vnum)
{
	int num = 0;
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (it3->obj_->get_vnum() == vnum)
				{
					it3->obj_->set_timer(0);
					num++;
				}
			}
		}
	}
	return num;
}

// * Иммское 'где' для учета предметов на почте.
int print_imm_where_obj(CHAR_DATA *ch, char *arg, int num)
{
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (isname(arg, it3->obj_->get_aliases()))
				{
					std::string target = GetNameByUnique(it->first);
					std::string sender = GetNameByUnique(it2->first);

					send_to_char(ch, "O%3d. %-25s - наход%sся на почте (отправитель: %s, получатель: %s).\r\n",
						num++,
						it3->obj_->get_short_description().c_str(),
						GET_OBJ_POLY_1(ch, it3->obj_),
						sender.c_str(),
						target.c_str());
				}
			}
		}
	}
	return num;
}

// * Обновление полей объектов при изменении их прототипа через олц.
void olc_update_from_proto(int robj_num, OBJ_DATA *olc_proto)
{
	for (ParcelListType::const_iterator it = parcel_list.begin(); it != parcel_list.end(); ++it)
	{
		for (SenderListType::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (std::list<Node>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
			{
				if (GET_OBJ_RNUM(it3->obj_) == robj_num)
				{
					olc_update_object(robj_num, it3->obj_.get(), olc_proto);
				}
			}
		}
	}
}

// * Поиск цели для каста локейта.
OBJ_DATA * locate_object(const char *str)
{
	for (ParcelListType::const_iterator i = parcel_list.begin(); i != parcel_list.end(); ++i)
	{
		for (SenderListType::const_iterator k = i->second.begin(); k != i->second.end(); ++k)
		{
			for (std::list<Node>::const_iterator o = k->second.begin(); o != k->second.end(); ++o)
			{
				if (isname(str, o->obj_->get_aliases()))
				{
					return o->obj_.get();
				}
			}
		}
	}
	return 0;
}

// * Возврат всех ждущих посылок их отправителю.
void bring_back(CHAR_DATA *ch, CHAR_DATA *mailman)
{
	int money = 0;
	bool empty = true;
	for (ParcelListType::iterator i = parcel_list.begin(); i != parcel_list.end(); /* empty */)
	{
		SenderListType::iterator k = i->second.find(GET_UNIQUE(ch));
		if (k == i->second.end())
		{
			++i;
			continue;
		}
		empty = false;
		OBJ_DATA *obj = create_parcel();
		fill_ex_desc(ch, obj, std::string("Отдел возвратов"));
		for (std::list<Node>::iterator l = k->second.begin(); l != k->second.end(); ++l)
		{
			money += l->money_ - calculate_timer_cost(l);
			world_objects.add(l->obj_);
			obj_to_obj(l->obj_.get(), obj);
		}
		obj_to_char(obj, ch);
		snprintf(buf, MAX_STRING_LENGTH, "$n дал$g вам посылку.");
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		act("$N дал$G $n2 посылку.", FALSE, ch, 0, mailman, TO_ROOM);

		i->second.erase(k);
		if (i->second.empty())
		{
			parcel_list.erase(i++);
		}
		else
		{
			++i;
		}
	}
	if (!empty && money > 0)
	{
		act("$n сказал$g вам : 'За экстренный возврат посылок с вас удержана половина зарезервированных кун.'",
				FALSE, mailman, 0, ch, TO_VICT);
		std::string name = GET_NAME(ch);
		return_money(name, money/2, RETURN_WITH_MONEY);
		ObjSaveSync::add(ch->get_uid(), ch->get_uid(), ObjSaveSync::PARCEL_SAVE);
	}
	else if (empty)
	{
		act("$n сказал$g вам : 'У нас нет ни одной вашей посылки!'", FALSE, mailman, 0, ch, TO_VICT);
	}
}

} // namespace Parcel

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
