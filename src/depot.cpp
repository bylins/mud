// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include <map>
#include <list>
#include <sstream>
#include <cmath>
#include <bitset>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include "depot.hpp"
#include "db.h"
#include "handler.h"
#include "utils.h"
#include "comm.h"
#include "auction.h"
#include "exchange.h"
#include "interpreter.h"
#include "screen.h"

extern SPECIAL(bank);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern int can_take_obj(CHAR_DATA * ch, OBJ_DATA * obj);
extern std::list<FILE *> opened_files;
extern void write_time(FILE *file);

namespace Depot {

enum {PURGE_CHEST = 0, PERS_CHEST, SHARE_CHEST};
// кол-во хранилищ (для битсета) + 1 для состояния пуржа шмоток при нехватке бабла
const int DEPOT_NUM = 3;
const int PERS_CHEST_VNUM = 331;
const int SHARE_CHEST_VNUM = 332;
// рнумы пересчитываются при необходимости в renumber_obj_rnum
int PERS_CHEST_RNUM = -1;
int SHARE_CHEST_RNUM = -1;
// максимальное кол-во шмоток в персональном|общем хранилищах
const unsigned int MAX_PERS_SLOTS = 25;
const unsigned int MAX_SHARE_SLOTS = 1000;
// расширенное логирование в отдельный файл на время тестов и изменений
const bool debug_mode = 1;

class OfflineNode
{
	public:
	OfflineNode() : vnum(0), timer(0), rent_cost(0), uid(0) {};
	int vnum; // внум
	int timer; // таймер
	int rent_cost; // цена ренты в день
	int uid; // глобальный уид
};

// Это избыточная структура, но ограничивать размер списка объединенных
// хранилищ не хочется, а возможность спама в этот список исключать нельзя, поэтому
// хочется как можно меньше зависеть от его размера и не гонять лишний раз.
class AllowedChar
{
	public:
	AllowedChar(const std::string &name_, bool mutual_) : name(name_), mutual(mutual_) {};
	std::string name; // имя чара на объединение
	bool mutual; // true - есть взаимная ссылка, false - ждет ответного подтверждения
};

typedef std::list<OBJ_DATA *> ObjListType; // имя, шмотка
typedef std::list<OfflineNode> TimerListType; // список шмота, находящегося в оффлайне
typedef std::map<long, AllowedChar> AllowedCharType; // уид, инфа чара

class CharNode
{
	public:
	CharNode() : ch(0), money(0), money_spend(0), buffer_cost(0), state(0), cost_per_day(0) {};
	// онлайн
	ObjListType pers_online; // шмотки в хранилище онлайн
	ObjListType share_online; // шмотки в шкафчике онлайн
	CHAR_DATA *ch; // чар (онлайн бабло)
	// оффлайн
	TimerListType offline_list; // шмотки в обоих хранилищах оффлайн
	long money; // бабло руки+банк оффлайн
	long money_spend; // сколько было потрачено за время чара в ренте
	// общие поля
	double buffer_cost; // буффер для точного снятия ренты
	// список чаров, которым расшарено хранилище (или ожидающих подтверждения)
	AllowedCharType allowed_chars;
	// список уидов чаров, чьи хранилища будут подгружены при след.апдейте
	std::vector<long> waiting_allowed_chars;
	std::string name; // имя чара, чтобы не гонять всех через уиды при обработке
	// состояние для сохранения шмоток (номера больше нуля из enum типов хранилищ)
	// 0 - нужно спуржить все шмотки при входе чара в игру,
	// 1 - сохранить только персональный список,
	// 2 - сохранить только общий список
	std::bitset<DEPOT_NUM> state;

	ObjListType & get_depot_type(int type);
	void take_item(CHAR_DATA *vict, char *arg, int howmany, int type);
	void remove_item(CHAR_DATA *vict, ObjListType::iterator &obj_it, ObjListType &cont, int type);
	void update_online_item(ObjListType &cont, int type);
	void update_offline_item();
	void removal_period_cost();
	void load_online_objs(int file_type, bool reload = 0);
	void add_item(OBJ_DATA *obj, int type);
	void load_item(OBJ_DATA*obj, int type);
	void online_to_offline(ObjListType &cont, int file_type);
	bool any_other_share();
	bool obj_from_obj_list(char *name, int type, CHAR_DATA *vict);
	void reset();
	void save_online_objs();

	void add_cost_per_day(OBJ_DATA *obj);
	void add_cost_per_day(int amount);
	void remove_cost_per_day(OBJ_DATA *obj);
	void remove_cost_per_day(int amount);
	int get_cost_per_day() { return cost_per_day; };
	void reset_cost_per_day() { cost_per_day = 0; } ;

	private:
	// общая стоимость ренты шмота в день (исключая персональное хранилище онлайн),
	// чтобы не иметь проблем с округлением и лишними пересчетами
	int cost_per_day;
};

typedef std::map<long, CharNode> DepotListType; // уид чара, инфа
DepotListType depot_list; // список личных хранилищ

/**
* Капитально расширенная версия сислога для хранилищ.
*/
void depot_log(const char *format, ...)
{
	if (!debug_mode) return;

	const char *filename = "../log/depot.log";
	static FILE *file = 0;
	if (!file) {
		file = fopen(filename, "a");
		if (!file) {
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	} else if (!format)
		format = "SYSERR: depot_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
}

/**
* Берется минимальная цена ренты шмотки, не важно, одетая она будет или снятая.
*/
int get_object_low_rent(OBJ_DATA *obj)
{
	int rent = GET_OBJ_RENT(obj) > GET_OBJ_RENTEQ(obj) ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj);
	depot_log("get_object_low_rent: %s %i", GET_OBJ_PNAME(obj, 0), rent);
	return rent;
}

/**
* Добавление цены ренты с проверкой на валидность.
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
	depot_log("add_cost_per_day: %i, cost_per_day: %i", amount, cost_per_day);
}

/**
* Добавление цены ренты с порверкой на валидность и переполнение.
*/
void CharNode::add_cost_per_day(OBJ_DATA *obj)
{
	add_cost_per_day(get_object_low_rent(obj));
}

/**
* Вычитание цены ренты с проверкой на валидность для защиты от изменений цены на шмотках.
*/
void CharNode::remove_cost_per_day(int amount)
{
	depot_log("remove_cost_per_day: %i", amount);
	cost_per_day -= amount;
	if (cost_per_day < 0)
		cost_per_day = 0;
	depot_log("remove_per_day: %i", cost_per_day);
}

/**
* Вычитание цены ренты с проверкой на валидность для защиты от изменений цены на шмотках.
*/
void CharNode::remove_cost_per_day(OBJ_DATA *obj)
{
	remove_cost_per_day(get_object_low_rent(obj));
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
			OBJ_DATA *share_chest = read_object(SHARE_CHEST_VNUM, VIRTUAL);
			if (!pers_chest || !share_chest)
				return;
			obj_to_room(share_chest, ch->in_room);
			obj_to_room(pers_chest, ch->in_room);
		}
	}
}

/**
* Инициализация рнумов сундуков, лоад их в банках.
* Лоад файла с оффлайн информацией по предметам.
*/
void init_depot()
{
	depot_log("init_depot: start");
	PERS_CHEST_RNUM = real_object(PERS_CHEST_VNUM);
	SHARE_CHEST_RNUM = real_object(SHARE_CHEST_VNUM);

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
			catch(boost::bad_lexical_cast &)
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
			// проверяем существование прототипа предмета
			int rnum = real_object(tmp_obj.vnum);
			if (rnum >= 0)
			{
				obj_index[rnum].number++;
				tmp_node.add_cost_per_day(tmp_obj.rent_cost);
				tmp_node.offline_list.push_back(tmp_obj);
			}
		}
		if (buffer != "</Objects>")
		{
			log("Хранилище: ошибка чтения </Objects>.");
			break;
		}
		// чтение доверенных чаров
		file >> buffer;
		if (buffer != "<Allowed>")
		{
			log("Хранилище: ошибка чтения <Allowed>.");
			break;
		}
		while (file >> buffer)
		{
			if (buffer == "</Allowed>") break;

			bool mutual;
			long allowed_uid;
			try
			{
				allowed_uid = boost::lexical_cast<long>(buffer);
			}
			catch(boost::bad_lexical_cast &)
			{
				log("Хранилище: ошибка чтения allowed_uid (%s)", buffer.c_str());
				break;
			}
			if (!(file >> mutual))
			{
				log("Хранилище: ошибка чтения mutual (allowed_uid: %ld).", allowed_uid);
				break;
			}
			// случай делета доверенного персонажа
			std::string allowed_name = GetNameByUnique(allowed_uid);
			if (allowed_name.empty())
			{
				log("Хранилище: UID %ld - доверенного персонажа не существует.", allowed_uid);
				continue;
			}
			name_convert(allowed_name);
			// все ок, пишем его
			AllowedChar tmp_allowed(allowed_name, mutual);
			tmp_node.allowed_chars.insert(std::make_pair(allowed_uid, tmp_allowed));
		}
		if (buffer != "</Allowed>")
		{
			log("Хранилище: ошибка чтения </Allowed>.");
			break;
		}

		// проверка корректной дочитанности
		file >> buffer;
		if (buffer != "</Node>")
		{
			log("Хранилище: ошибка чтения </Node>.");
			break;
		}

		// проверяем есть ли еще такой чар вообще
		tmp_node.name = GetNameByUnique(uid);
		if (tmp_node.name.empty())
		{
			log("Хранилище: UID %ld - персонажа не существует.", uid);
			continue;
		}
		name_convert(tmp_node.name);

		depot_list[uid] = tmp_node;
	}
	depot_log("init_depot: end");
}

/**
* Проверка, является ли obj персональным или общим хранилищем.
* \return 0 - не явялется, > 0 - тип хранилища (из enum).
*/
int is_depot(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (obj->item_number == PERS_CHEST_RNUM)
		return PERS_CHEST;
	if (obj->item_number == SHARE_CHEST_RNUM)
		return SHARE_CHEST;
	return 0;
}

/**
* В случае переполнения денег на счете - кладем сколько можем, остальное возвращаем чару на руки.
* На руках при возврате переполняться уже некуда, т.к. вложение идет с этих самых рук.
*/
void put_gold_chest(CHAR_DATA *ch, OBJ_DATA *obj)
{
	depot_log("put_gold_chest: %s, %s", GET_NAME(ch), GET_OBJ_PNAME(obj, 0));
	if (GET_OBJ_TYPE(obj) != ITEM_MONEY) return;

	long gold = GET_OBJ_VAL(obj, 0);
	if ((GET_BANK_GOLD(ch) + gold) < 0)
	{
		long over = std::numeric_limits<long>::max() - GET_BANK_GOLD(ch);
		GET_BANK_GOLD(ch) += over;
		gold -= over;
		GET_GOLD(ch) += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "Вы удалось вложить только %ld %s.\r\n",
			over, desc_count(over, WHAT_MONEYu));
	}
	else
	{
		GET_BANK_GOLD(ch) += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "Вы вложили %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
	}
	depot_log("put_gold_chest: end");
}

/**
* Проверка возможности положить шмотку в хранилище.
* FIXME с кланами копипаст.
*/
bool can_put_chest(CHAR_DATA *ch, OBJ_DATA *obj)
{
	depot_log("can_put_chest: %s, %s", GET_NAME(ch), GET_OBJ_PNAME(obj, 0));
	if (IS_OBJ_STAT(obj, ITEM_NODROP)
			|| OBJ_FLAGGED(obj, ITEM_ZONEDECAY)
			|| GET_OBJ_TYPE(obj) == ITEM_KEY
			|| IS_OBJ_STAT(obj, ITEM_NORENT)
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
* Просто для читаемости put_depot - добавление шмотки в список персонального или общего хранилища.
* Рента считается только если кладем в общее хранилище. У персонального она плюсанется при переводе
* в оффлайн. \param type - здесь и далее тип хранилища (из enum).
*/
void CharNode::add_item(OBJ_DATA *obj, int type)
{
	depot_log("add_item: %s, %i", GET_OBJ_PNAME(obj, 0), type);
	if (type == PERS_CHEST)
		pers_online.push_front(obj);
	else
	{
		share_online.push_front(obj);
		add_cost_per_day(obj);
	}
	state.set(type);
	depot_log("add_item: state.set(%i)", type);
}

/**
* Аналог add_item, только тут у нас рента должна вычитаться, если вещь идет в персональный
* онлайновый список. Используется при лоаде файлов шмоток чара при входе в игру, чтобы корректно
* учесть цену ренты.
*/
void CharNode::load_item(OBJ_DATA*obj, int type)
{
	depot_log("load_item: %s, %i", GET_OBJ_PNAME(obj, 0), type);
	if (type == PERS_CHEST)
	{
		pers_online.push_front(obj);
		remove_cost_per_day(obj);
	}
	else
		share_online.push_front(obj);
	state.set(type);
	depot_log("load_item: state.set(%i)", type);
}

/**
* Поиск итератора хранилища или создание нового, чтобы не париться.
* \param ch - проставляет в зависимости от наличия чара онлайн, иначе будет нулевым.
*/
DepotListType::iterator create_depot(long uid, CHAR_DATA *ch = 0)
{
	depot_log("create_depot: %i, %s", uid, ch ? GET_NAME(ch) : "ch = 0");
	DepotListType::iterator it = depot_list.find(uid);
	if (it == depot_list.end())
	{
		CharNode tmp_node;
		// в случае отсутствии чара (пока только при релоаде) ch останется нулевым,
		// а имя возьмется через уид
		tmp_node.ch = ch;
		tmp_node.name = GetNameByUnique(uid);
		depot_list[uid] = tmp_node;
		it = depot_list.find(uid);
		depot_log("create_depot: new_node for %s", tmp_node.name.c_str());
	}
	depot_log("create_depot: end");
	return it;
}

/**
* Кладем шмотку в хранилище (мобов посылаем лесом), деньги автоматом на счет в банке.
*/
bool put_depot(CHAR_DATA *ch, OBJ_DATA *obj, int type)
{
	depot_log("put_depot: %s, %s, %i", GET_NAME(ch), GET_OBJ_PNAME(obj, 0), type);
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

	if (type == PERS_CHEST && it->second.pers_online.size() >= MAX_PERS_SLOTS)
	{
		send_to_char("В вашем хранилище совсем не осталось места :(.\r\n" , ch);
		return 0;
	}
	else if (type == SHARE_CHEST && it->second.share_online.size() >= MAX_SHARE_SLOTS)
	{
		send_to_char("В вашем хранилище совсем не осталось места :(.\r\n" , ch);
		return 0;
	}

	if (!GET_BANK_GOLD(ch) && !GET_GOLD(ch))
	{
		send_to_char(ch,
			"У Вас ведь совсем нет денег, чем Вы собираетесь расплачиваться за хранение вещей?\r\n",
			OBJ_PAD(obj, 5));
		return 0;
	}

	it->second.add_item(obj, type);

	std::string chest_name;
	if (type == PERS_CHEST)
		chest_name = "персональное";
	else
		chest_name = "общее";
	sprintf(buf, "Вы положили $o3 в %s хранилище.", chest_name.c_str());
	sprintf(buf1, "$n положил$g $o3 в %s хранилище.", chest_name.c_str());
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	act(buf1, TRUE, ch, obj, 0, TO_ROOM);

	obj_from_char(obj);
	check_auction(NULL, obj);
	OBJ_DATA *temp;
	REMOVE_FROM_LIST(obj, object_list, next);

	depot_log("put_depot: done");
	return 1;
}

void print_obj(std::stringstream &out, OBJ_DATA *obj, int count)
{
	out << obj->short_description;
	if (count > 1)
		out << " [" << count << "]";
	out << " [" << get_object_low_rent(obj) << " "
		<< desc_count(get_object_low_rent(obj), WHAT_MONEYa) << "]\r\n";
}

/**
* Для читаемости show_depot - вывод списка предметов персонажу.
* Сортировка по алиасам и группировка одинаковых предметов.
* В данном случае ch и money идут раздельно, чтобы можно было сразу выводить и чужие хранилища.
*/
std::string print_obj_list(CHAR_DATA * ch, ObjListType &cont, const std::string &chest_name, int money)
{
	depot_log("print_obj_list: start");
	int rent_per_day = 0;
	std::stringstream out;

	cont.sort(
		boost::bind(std::less<char *>(),
			boost::bind(&obj_data::name, _1),
			boost::bind(&obj_data::name, _2)));

	ObjListType::const_iterator prev_obj_it = cont.end();
	int count = 0;
	bool found = 0;
	for(ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
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
		<< " (вещей: " << cont.size()
		<< ", в день: " << rent_per_day << " " << desc_count(rent_per_day, WHAT_MONEYa);
	if (expired)
		head << ", денег на: " << expired << " " << desc_count(expired, WHAT_DAY);
	else
		head << ", денег на: очень много дней";
	head << CCNRM(ch, C_NRM) << ")\r\n";

	depot_log("print_obj_list: end");
	return (head.str() + out.str());
}

/**
* Выводим состояние хранилища вместо просмотра контейнера.
* В случае общего хранилища идем по всем прицепленным хранилищам.
*/
void show_depot(CHAR_DATA * ch, OBJ_DATA * obj, int type)
{
	depot_log("show_depot: %s, %s, %i", GET_NAME(ch), GET_OBJ_PNAME(obj, 0), type);
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

	std::string out;
	if (type == PERS_CHEST)
	{
		out = print_obj_list(ch, it->second.pers_online, "Ваше персональное хранилище",
			(GET_GOLD(ch) + GET_BANK_GOLD(ch)));
	}
	else
	{
		out = CCWHT(ch, C_NRM);
		out += "Для работы с общим хранилищем наберите команду 'хранилище'.\r\n";
		out += CCNRM(ch, C_NRM);
		out += print_obj_list(ch, it->second.share_online, "Ваше общее хранилище",
			(GET_GOLD(ch) + GET_BANK_GOLD(ch)));
		out += "\r\n";
		// ищем общие хранилища других персонажей и выводим их тоже
		for (AllowedCharType::const_iterator al_it = it->second.allowed_chars.begin();
				al_it != it->second.allowed_chars.end(); ++al_it)
		{
			if (al_it->second.mutual)
			{
				DepotListType::iterator vict_it = depot_list.find(al_it->first);
				if (vict_it != depot_list.end())
				{
				 // тут надо учесть кол-во денег у чара при том, что он может быть и оффлайн
				 DESCRIPTOR_DATA *d = DescByUID(al_it->first);
				 int money = d ? (GET_GOLD(d->character) + GET_BANK_GOLD(d->character))
						: vict_it->second.money;
				 out += print_obj_list(ch, vict_it->second.share_online,
						al_it->second.name, money);
				 // если хранилище еще не подгрузилось - надо так и сказать,
				 // а то будут думать, что не работает
				 if ((std::find(it->second.waiting_allowed_chars.begin(),
								it->second.waiting_allowed_chars.end(), al_it->first)
							!= it->second.waiting_allowed_chars.end())
							&& vict_it->second.share_online.empty())
				 {
						out += "Хранилище будет доступно в течение пары минут.\r\n\r\n";
				 }
				 else
						out += "\r\n";
				}
			}
		}
	}
	page_string(ch->desc, out, 1);
	depot_log("show_depot: end");
}

/**
* Поиск шмотки в контейнере (со всякими точками), удаляем ее тут же.
* Общие контейнеры смотрятся друг за другом до совпадения.
*/
bool CharNode::obj_from_obj_list(char *name, int type, CHAR_DATA *vict)
{
	depot_log("obj_from_obj_list: %s, %i, %s", name ? name : "<none>", type, GET_NAME(vict));
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	strcpy(tmp, name);

	ObjListType &cont = get_depot_type(type);

	int j = 0, number;
	if (!(number = get_number(&tmp)))
	{
		depot_log("obj_from_obj_list: fasle");
		return false;
	}

	for (ObjListType::iterator obj_it = cont.begin(); obj_it != cont.end() && (j <= number); ++obj_it)
	{
		if (isname(tmp, (*obj_it)->name) && ++j == number)
		{
			remove_item(vict, obj_it, cont, type);
			return true;
		}
	}

	if (type == SHARE_CHEST)
	{
		for (AllowedCharType::const_iterator al_it = allowed_chars.begin();
				al_it != allowed_chars.end(); ++al_it)
		{
			if (al_it->second.mutual)
			{
				DepotListType::iterator vict_it = depot_list.find(al_it->first);
				if (vict_it != depot_list.end())
				{
				 for (ObjListType::iterator vict_obj_it = vict_it->second.share_online.begin();
							vict_obj_it != vict_it->second.share_online.end() && (j <= number);
							++vict_obj_it)
				 {
						if (isname(tmp, (*vict_obj_it)->name) && ++j == number)
						{
							remove_item(vict, vict_obj_it, vict_it->second.share_online, type);
							return true;
						}
				 }
				}
			}
		}
	}
	depot_log("obj_from_obj_list: fasle");
	return false;
}

/**
* Берем шмотку из хранилища.
* \param vict - тут берется не из поля хранилища на случай, если берем из чужого.
*/
void CharNode::remove_item(CHAR_DATA *vict, ObjListType::iterator &obj_it, ObjListType &cont, int type)
{
	depot_log("remove_item: %s, %s, %i", GET_NAME(vict), GET_OBJ_PNAME(*obj_it, 0), type);
	(*obj_it)->next = object_list;
	object_list = *obj_it;
	obj_to_char(*obj_it, vict);

	std::string chest_name;
	if (type == PERS_CHEST)
		chest_name = "персонального";
	else
		chest_name = "общего";
	sprintf(buf, "Вы взяли $o3 из %s хранилища.", chest_name.c_str());
	sprintf(buf1, "$n взял$g $o3 из %s хранилища.", chest_name.c_str());
	act(buf, FALSE, vict, *obj_it, 0, TO_CHAR);
	act(buf1, TRUE, vict, *obj_it, 0, TO_ROOM);

	if (type != PERS_CHEST)
		remove_cost_per_day(*obj_it);
	cont.erase(obj_it++);
	state.set(type);
	depot_log("remove_item: state.set(%i)", type);
}

/**
* Чтобы не морочить голову с передачей контейнера помимо типа в функции.
*/
ObjListType & CharNode::get_depot_type(int type)
{
	depot_log("get_depot_type: %i", type);
	if (type == PERS_CHEST)
		return pers_online;
	else
		return share_online;
}

/**
* Поиск шмотки в хранилище и ее взятие, в том числе и из общих хранилищ.
*/
void CharNode::take_item(CHAR_DATA *vict, char *arg, int howmany, int type)
{
	depot_log("take_item: %s, %s, %i, %i", GET_NAME(vict), arg ? arg : "<none>", howmany, type);
	ObjListType &cont = get_depot_type(type);

	int obj_dotmode = find_all_dots(arg);
	if (obj_dotmode == FIND_INDIV)
	{
		bool result = obj_from_obj_list(arg, type, vict);
		if (!result)
		{
			send_to_char(vict, "Вы не видите '%s' в хранилище.\r\n", arg);
			return;
		}
		while (result && --howmany)
			result = obj_from_obj_list(arg, type, vict);
	}
	else
	{
		if (obj_dotmode == FIND_ALLDOT && !*arg)
		{
			send_to_char("Взять что \"все\" ?\r\n", vict);
			return;
		}
		bool found = 0;
		for (ObjListType::iterator obj_list_it = cont.begin(); obj_list_it != cont.end(); )
		{
			if (obj_dotmode == FIND_ALL || isname(arg, (*obj_list_it)->name))
			{
				// чтобы нельзя было разом собрать со шкафчика неск.тыс шмоток
				if (!can_take_obj(vict, *obj_list_it)) return;
				found = 1;
				remove_item(vict, obj_list_it, cont, type);
			}
			else
				++obj_list_it;
		}
		// в случае общего хранилища идем по всем расшаренным
		// FIXME: пока копи-паст с obj_from_obj_list и убрать вложенность
		if (type == SHARE_CHEST)
		{
			for (AllowedCharType::const_iterator al_it = allowed_chars.begin();
				 al_it != allowed_chars.end(); ++al_it)
			{
				if (al_it->second.mutual)
				{
				 DepotListType::iterator vict_it = depot_list.find(al_it->first);
				 if (vict_it != depot_list.end())
				 {
						for (ObjListType::iterator vict_obj_it = vict_it->second.share_online.begin();
								vict_obj_it != vict_it->second.share_online.end(); )
						{
							if (obj_dotmode == FIND_ALL || isname(arg, (*vict_obj_it)->name))
							{
								// чтобы нельзя было разом собрать со шкафчика неск.тыс шмоток
								if (!can_take_obj(vict, *vict_obj_it)) return;
								found = 1;
								remove_item(vict, vict_obj_it, vict_it->second.share_online, type);
							}
							else
								++vict_obj_it;
						}
				 }
				}
			}
		}

		if (!found)
		{
			send_to_char(vict, "Вы не видите ничего похожего на '%s' в хранилище.\r\n", arg);
			return;
		}
	}
}

/**
* Взятие чего-то из хранилищ.
*/
void take_depot(CHAR_DATA *vict, char *arg, int howmany, int type)
{
	depot_log("take_depot: %s, %s, %i, %i", GET_NAME(vict), arg ? arg : "<none>", howmany, type);
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

	if (type == PERS_CHEST)
		it->second.take_item(vict, arg, howmany, PERS_CHEST);
	else
		it->second.take_item(vict, arg, howmany, SHARE_CHEST);
	depot_log("take_depot: done");
}

/**
* Очищение всех списков шмота и полей бабла с флагами.
*/
void CharNode::reset()
{
	depot_log("reset: %s", name.c_str());
	for (ObjListType::iterator obj_it = pers_online.begin(); obj_it != pers_online.end(); ++obj_it)
		extract_obj(*obj_it);
	for (ObjListType::iterator obj_it = share_online.begin(); obj_it != share_online.end(); ++obj_it)
		extract_obj(*obj_it);
	pers_online.clear();
	share_online.clear();

	// тут нужно ручками перекинуть удаляемый шмот в лоад и потом уже грохнуть все разом
	for (TimerListType::iterator obj = offline_list.begin(); obj != offline_list.end(); ++obj)
	{
		int rnum = real_object(obj->vnum);
		if (rnum >= 0)
			obj_index[rnum].number--;
	}
	offline_list.clear();

	reset_cost_per_day();
	buffer_cost = 0;
	money = 0;
	money_spend = 0;

	state.reset();
	state.set(PURGE_CHEST);
	depot_log("reset: %s done", name.c_str());
}

/**
* Запись файла со шмотом (полная версия записи).
*/
void write_obj_file(const std::string &name, int file_type, const ObjListType &cont)
{
	depot_log("write_obj_file: %s, %i", name.c_str(), file_type);
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
		remove(filename);
		depot_log("write_obj_file: %s deleted", filename);
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
		char databuf[MAX_STRING_LENGTH];
		char *data = databuf;
		write_one_object(&data, *obj_it, 0);
		file << databuf;
	}
	file << "\n$\n$\n";
	file.close();
	depot_log("write_obj_file: %s done", filename);
}

/**
* Снятие ренты за период в онлайне и оффлайне с удалением шмота при нехватке денег.
* FIXME в кланах аналогичная фигня + вообще эти операции надо выносить в методы у чара
*/
void CharNode::removal_period_cost()
{
	depot_log("removal_period_cost: %s", name.c_str());
	double i;
	buffer_cost += (static_cast<double>(cost_per_day) / SECS_PER_MUD_DAY);
	modf(buffer_cost, &i);
	if (i)
	{
		int diff = static_cast<int> (i);
		depot_log("removal_period_cost: diff = %i", diff);
		if (ch)
			GET_BANK_GOLD(ch) -= diff;
		else
		{
			money -= diff;
			money_spend += diff;
		}
		buffer_cost -= i;
	}

	bool purge = 0;
	// онлайн/оффлайн
	if (ch && GET_BANK_GOLD(ch) < 0)
	{
		GET_GOLD(ch) += GET_BANK_GOLD(ch);
		GET_BANK_GOLD(ch) = 0;
		if (GET_GOLD(ch) < 0)
		{
			GET_GOLD(ch) = 0;
			purge = 1;
		}
	}
	else
	{
		if (money < 0)
			purge = 1;
	}

	if (purge)
	{
		depot_log("removal_period_cost: purge");
		reset();
		write_obj_file(name, PERS_DEPOT_FILE, pers_online);
		write_obj_file(name, SHARE_DEPOT_FILE, share_online);
	}
	depot_log("removal_period_cost: %s done", name.c_str());
}

/**
* Апдейт таймеров в онлайн списках с оповещением о пурже, если чар онлайн и расчетом общей ренты.
*/
void CharNode::update_online_item(ObjListType &cont, int type)
{
	depot_log("update_online_item: %s, %i", name.c_str(), type);
	std::string chest_name;
	if (type == PERS_CHEST)
		chest_name = "Персональное";
	else
		chest_name = "Общее";

	for (ObjListType::iterator obj_it = cont.begin(); obj_it != cont.end(); )
	{
		GET_OBJ_TIMER(*obj_it)--;
		if (GET_OBJ_TIMER(*obj_it) <= 0)
		{
			depot_log("update_online_item: %s destroyed (timer = %i)", GET_OBJ_PNAME(*obj_it, 0), GET_OBJ_TIMER(*obj_it));
			if (ch)
				send_to_char(ch, "[%s хранилище]: %s'%s рассыпал%s в прах'%s\r\n",
				 chest_name.c_str(), CCIRED(ch, C_NRM), (*obj_it)->short_description,
				 GET_OBJ_SUF_2((*obj_it)), CCNRM(ch, C_NRM));
			// вычитать ренту из cost_per_day здесь не надо, потому что она уже обнулена
			extract_obj(*obj_it);
			cont.erase(obj_it++);
			state.set(type);
			depot_log("update_online_item: state.set(%i)", type);
		}
		else
		{
			add_cost_per_day(*obj_it);
			++obj_it;
		}
	}
	depot_log("update_online_item: %s done", name.c_str());
}

/**
* Апдейт таймеров в оффлайн списках с расчетом общей ренты.
*/
void CharNode::update_offline_item()
{
	depot_log("update_offline_item: %s", name.c_str());
	for (TimerListType::iterator obj_it = offline_list.begin(); obj_it != offline_list.end(); )
	{
		--(obj_it->timer);
		if (obj_it->timer <= 0)
		{
			depot_log("update_offline_item: uid = %li, vnum = %i destroyed (timer = %i)", obj_it->uid, obj_it->vnum, obj_it->timer);
			// шмотка уходит в лоад
			int rnum = real_object(obj_it->vnum);
			if (rnum >= 0)
			obj_index[rnum].number--;
			// вычитать ренту из cost_per_day здесь не надо, потому что она уже обнулена
			offline_list.erase(obj_it++);
		}
		else
		{
			add_cost_per_day(obj_it->rent_cost);
			++obj_it;
		}
	}
	depot_log("update_offline_item: %s done", name.c_str());
}

/**
* Апдейт таймеров на всех шмотках, удаление при нулевых таймерах (с оповещением),
* удаление записи при пустом хранилище.
*/
void update_timers()
{
	depot_log("update_timers: start");
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); )
	{
		class CharNode & node = it->second;
		// обнуляем ренту и пересчитываем ее по ходу обновления таймеров
		node.reset_cost_per_day();
		// онлайновые списки
		if (!node.pers_online.empty())
			node.update_online_item(node.pers_online, PERS_CHEST);
		if (!node.share_online.empty())
			node.update_online_item(node.share_online, SHARE_CHEST);
		// оффлайновый список
		if (!node.offline_list.empty())
			node.update_offline_item();

		// снятие денег и пурж шмота, если денег уже не хватает
		if (node.get_cost_per_day())
			node.removal_period_cost();
		// пустые хранилища здесь спокойно удаляем, т.к. все уже спуржено и делать ничего больше не надо
		if (node.offline_list.empty()
				&& node.allowed_chars.empty()
				&& node.state.none()
				&& node.pers_online.empty()
				&& node.share_online.empty())
		{
			depot_log("update_timers: purge empty depot %s", it->second.name.c_str());
			depot_list.erase(it++);
		}
		else
			++it;
	}
	depot_log("update_timers: end");
}

const char *HELP_FORMAT =
	"Формат: хранилище - справка по команде и список имен с указанием статуса подтверждения.\r\n"
	"        хранилище добавить имя - добавить персонажа в список объединенных хранилищ.\r\n"
	"        хранилище удалить имя - удалить персонажа из списка объединенных хранилищ.\r\n"
	"Работа с хранилищами аналогична любым другим контейнерам, например:\r\n"
	"осмотреть хранилище, положить меч хранилище, взять меч хранилище, смотреть шкафчик, взять все шкафчик и т.д.\r\n"
	"Персональное хранилище - Ваше личное хранилище (до 25 предметов).\r\n"
	"Шкафчик для передачи вещей - хранилище для одновременного использования несколькими персонажами (до 1к предметов).\r\n"
	"- за предметы в вашем персональном хранилище плата снимается только когда вы не в игре.\r\n"
	"- за предметы в шкафчике плата снимается постоянно (только за вашу часть предметов).\r\n"
	"- рента снимается равными долями каждые несколько минут с вашего счета или с денег на руках, если счет пустой.\r\n"
	"- при пустом счете все предметы в обоих хранилищах уничтожаются (в общем хранилище касается только ваших вещей).\r\n";

/**
* Команда 'хранилище' при нахождении в банке.
*/
SPECIAL(Special)
{
	if (IS_NPC(ch) || !CMD_IS("хранилище")) return false;

	depot_log("Special: %s", GET_NAME(ch));
	if (IS_IMMORTAL(ch))
	{
		send_to_char("И без хранилища обойдешься...\r\n" , ch);
		return true;
	}

	DepotListType::iterator ch_it = create_depot(GET_UNIQUE(ch), ch);

	std::string buffer = argument, command;
	GetOneParam(buffer, command);

	if (CompareParam(command, "добавить"))
	{
		std::string name;
		GetOneParam(buffer, name);
		long uid = GetUniqueByName(name);
		if (uid <= 0)
			send_to_char("Такого персонажа не существует.\r\n", ch);
		else if (uid == GET_UNIQUE(ch))
			send_to_char("Выглядит довольно глупо...\r\n", ch);
		else if (get_level_by_unique(uid) >= LVL_IMMORT)
			send_to_char("Ему хранилище ни к чему.\r\n", ch);
		else
		{
			AllowedCharType::const_iterator tmp_vict = ch_it->second.allowed_chars.find(uid);
			if (tmp_vict != ch_it->second.allowed_chars.end())
			{
				send_to_char("Он уже и так добавлен в Ваш доверительный список.\r\n", ch);
				return true;
			}

			depot_log("Special: %s add %s (uid: %li)", GET_NAME(ch), name.c_str(), uid);
			// сначала смотрим добавляемого чара на предмет ответной записи
			// если она есть - то втыкаем ему флаг обоюдной ссылки
			bool mutual = false;
			DepotListType::iterator vict_it = depot_list.find(uid);
			if (vict_it != depot_list.end())
			{
				AllowedCharType::iterator al_it = vict_it->second.allowed_chars.find(GET_UNIQUE(ch));
				if (al_it != vict_it->second.allowed_chars.end())
					al_it->second.mutual = mutual = true;
			}
			// а потом уже пишем его к себе с указанием, есть или нет ответная ссылка
			name_convert(name);
			AllowedChar tmp_char(name, mutual);
			ch_it->second.allowed_chars.insert(std::make_pair(uid, tmp_char));

			send_to_char(ch, "Персонаж добавлен в доверительный список.\r\n");
			if (mutual)
			{
				depot_log("Special: %s add mutual = 1");
				ch_it->second.waiting_allowed_chars.push_back(uid);
				// добавляемому тоже впишем себя в очередь на объединение, чтобы верно выводить
				// ему статус по команде хранилище
				vict_it->second.waiting_allowed_chars.push_back(GET_UNIQUE(ch));
				send_to_char(ch,
				 "Найдена ответная запись, хранилища будут объединены в течении пары минут.\r\n");
			}
			else
			{
				depot_log("Special: %s add mutual = 0");
				send_to_char(ch,
				 "Ответной записи не найдено, ожидается подтверждение указанного персонажа.\r\n");
			}
		}
		return true;
	}
	else if (CompareParam(command, "удалить"))
	{
		std::string name;
		GetOneParam(buffer, name);
		name_convert(name);
		long uid = -1;
		// удаляем персонажа из своего доверительного списка
		for (AllowedCharType::iterator al_it = ch_it->second.allowed_chars.begin();
				al_it != ch_it->second.allowed_chars.end(); ++al_it)
		{
			if (al_it->second.name == name)
			{
				uid = al_it->first;
				ch_it->second.allowed_chars.erase(al_it);
				break;
			}
		}
		// удаляем у него флаг взаимной ссылки на нас, если таковая была
		if (uid != -1)
		{
			DepotListType::iterator vict_it = depot_list.find(uid);
			if (vict_it != depot_list.end())
			{
				AllowedCharType::iterator al_it = vict_it->second.allowed_chars.find(GET_UNIQUE(ch));
				if (al_it != vict_it->second.allowed_chars.end())
					al_it->second.mutual = false;
				// если на это хранилище больше нет ссылающихся чаров онлайн - переводим его в оффлайн
				if (!vict_it->second.any_other_share())
					vict_it->second.online_to_offline(vict_it->second.share_online, SHARE_DEPOT_FILE);
			}
			depot_log("Special: %s remove %s (uid: %li)", GET_NAME(ch), name.c_str(), uid);
			send_to_char(ch, "Персонаж удален из Вашего доверительного списка.\r\n");
		}
		else
		{
			depot_log("Special: %s remove %s fail", GET_NAME(ch), name.c_str());
			send_to_char(ch, "Указанный персонаж не найден.\r\n");
		}
	}
	else
	{
		depot_log("Special: %s list");
		std::string out = "Список персонажей на объединение хранилищ:\r\n";
		out += CCWHT(ch, C_NRM);
		for (AllowedCharType::const_iterator al_it = ch_it->second.allowed_chars.begin();
				al_it != ch_it->second.allowed_chars.end(); ++al_it)
		{
			out += al_it->second.name + " ";
			if (al_it->second.mutual)
			{
				if (std::find(ch_it->second.waiting_allowed_chars.begin(),
						ch_it->second.waiting_allowed_chars.end(), al_it->first)
						!= ch_it->second.waiting_allowed_chars.end())
				{
				 out += "(подтвержден, объединение произойдет в течение пары минут)\r\n";
				}
				else
				 out += "(хранилища объединены)\r\n";
			}
			else
				out += "(ожидается подтверждение)\r\n";
		}
		if (ch_it->second.allowed_chars.empty())
			out += "в данный момент нет записей\r\n";
		out += CCNRM(ch, C_NRM);
		out += "\r\n";
		out += HELP_FORMAT;
		page_string(ch->desc, out, 1);
	}
	return true;
	depot_log("Special: %s done");
}

/**
* Сохранение хранилищ конкретного плеера в файлы.
* Чар может быть и оффлайн в случае загруженного общего хранилища
*/
void CharNode::save_online_objs()
{
	depot_log("save_online_objs: %s", name.c_str());
	if (state[PERS_CHEST])
	{
		write_obj_file(name, PERS_DEPOT_FILE, pers_online);
		state.reset(PERS_CHEST);
		depot_log("save_online_objs: %s pers_depot saved (state.reset(%i))", name.c_str(), PERS_CHEST);
	}
	if (state[SHARE_CHEST])
	{
		write_obj_file(name, SHARE_DEPOT_FILE, share_online);
		state.reset(SHARE_CHEST);
		depot_log("save_online_objs: %s share_depot saved (state.reset(%i))", name.c_str(), SHARE_CHEST);
	}
	depot_log("save_online_objs: %s done", name.c_str());
}

/**
* Сохранение всех онлайновых списков предметов в файлы.
*/
void save_all_online_objs()
{
	depot_log("save_all_online_objs: start");
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
		it->second.save_online_objs();
	depot_log("save_all_online_objs: end");
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
* Сохранение всех списков в общий файл с тайм-инфой шмоток.
*/
void save_timedata()
{
	depot_log("save_timedata: start");
	const char *depot_file = LIB_DEPOT"depot.db";
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
			file << GET_BANK_GOLD(it->second.ch) + GET_GOLD(it->second.ch) << " 0 ";
		else
			file << it->second.money << " " << it->second.money_spend << " ";
		file << it->second.buffer_cost << "\n";

		depot_log("save_timedata: %s (uid: %li) pers: %i, share: %i, offline: %i",
			it->second.name.c_str(), it->first,
			it->second.pers_online.size(),
			it->second.share_online.size(),
			it->second.offline_list.size());
		// собсна какие списки не пустые - те и пишем, особо не мудрствуя
		file << "<Objects>\n";
		write_objlist_timedata(it->second.pers_online, file);
		write_objlist_timedata(it->second.share_online, file);
		for (TimerListType::const_iterator obj_it = it->second.offline_list.begin();
				obj_it != it->second.offline_list.end(); ++obj_it)
		{
			file << obj_it->vnum << " " << obj_it->timer << " " << obj_it->rent_cost
				<< " " << obj_it->uid << "\n";
		}
		file << "</Objects>\n";

		file << "<Allowed>\n";
		for (AllowedCharType::const_iterator al_it = it->second.allowed_chars.begin();
				al_it != it->second.allowed_chars.end(); ++al_it)
		{
			file << al_it->first << " " << al_it->second.mutual << "\n";
		}
		file << "</Allowed>\n";
		file << "</Node>\n";
	}
	depot_log("save_timedata: end");
}

/**
* Лоад списка шмоток в онлайн список. FIXME: копи-паст чтения предметов с двух мест.
* \param reload - для рестора шмоток чара, по умолчанию ноль
* (определяет сверяемся мы с оффлайн списком при лоаде или нет).
*/
void CharNode::load_online_objs(int file_type, bool reload)
{
	depot_log("load_online_objs: %s, %i, %i", name.c_str(), file_type, reload);
	if (offline_list.empty()) return;

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
		if (!reload)
		{
			// собсна сверимся со списком таймеров и проставим изменившиеся данные
			TimerListType::iterator obj_it = std::find_if(offline_list.begin(), offline_list.end(),
				boost::bind(std::equal_to<long>(),
				 boost::bind(&OfflineNode::uid, _1), GET_OBJ_UID(obj)));
			if (obj_it != offline_list.end() && obj_it->vnum == GET_OBJ_VNUM(obj))
			{
				GET_OBJ_TIMER(obj) = obj_it->timer;
				// убирать из списка оффлайного надо, т.к. просто очистить потом список - это вата
				// при подгрузке только общего хранилища
				offline_list.erase(obj_it);
			}
			else
			{
				extract_obj(obj);
				continue;
			}
		}
		// при релоаде мы ничего не сверяем, а лоадим все, что есть
		if (file_type == PERS_DEPOT_FILE)
			load_item(obj, PERS_CHEST);
		else
			load_item(obj, SHARE_CHEST);
		// убираем его из глобального листа, в который он добавился еще на стадии чтения из файла
		OBJ_DATA *temp;
		REMOVE_FROM_LIST(obj, object_list, next);
	}
	delete [] databuf;
	depot_log("load_online_objs: done");
}

/**
* Вход чара в игру - снятие за хранилище, пурж шмоток, оповещение о кол-ве снятых денег,
* перевод списков в онлайн, подгрузка общих хранилищ.
*/
void enter_char(CHAR_DATA *ch)
{
	depot_log("enter_char: %s", GET_NAME(ch));
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		// попадалово в виде нехватки денег
		if (it->second.state[PURGE_CHEST])
		{
			send_to_char(ch,
				"%sХранилище: у Вас не хватило денег на оплату вещей и все они были нами пропиты!%s\r\n\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			GET_BANK_GOLD(ch) = 0;
			GET_GOLD(ch) = 0;
			depot_log("enter_char: %s purge_chest", GET_NAME(ch));
		}
		// снимаем бабло, если что-то было потрачено на ренту
		if (it->second.money_spend > 0)
		{
			GET_BANK_GOLD(ch) -= it->second.money_spend;
			if (GET_BANK_GOLD(ch) < 0)
			{
				GET_GOLD(ch) += GET_BANK_GOLD(ch);
				GET_BANK_GOLD(ch) = 0;
				if (GET_GOLD(ch) < 0)
				 GET_GOLD(ch) = 0;
			}

			send_to_char(ch, "%sХранилище: за время Вашего отсутствия удержано %d %s.%s\r\n\r\n",
				CCWHT(ch, C_NRM), it->second.money_spend, desc_count(it->second.money_spend,
				WHAT_MONEYa), CCNRM(ch, C_NRM));

		}
		// грузим оба хранилища и выставляем их на сейв, чтобы отразить изменения
		// за оффлайн тикание. общее могло быть уже подгружено
		it->second.load_online_objs(PERS_DEPOT_FILE);
		it->second.state.set(PERS_CHEST);
		if (it->second.share_online.empty())
		{
			it->second.load_online_objs(SHARE_DEPOT_FILE);
			it->second.state.set(SHARE_CHEST);
		}
		// обнуление оффлайновых полей и проставление ch для снятия бабла онлайн
		it->second.money = 0;
		it->second.money_spend = 0;
		it->second.offline_list.clear();
		it->second.ch = ch;
		// заявка на подгрузку расшаренных хранилищ (так же со след.апдейтом)
		for (AllowedCharType::const_iterator al_it = it->second.allowed_chars.begin();
				al_it != it->second.allowed_chars.end(); ++al_it)
		{
			if (al_it->second.mutual)
				it->second.waiting_allowed_chars.push_back(al_it->first);
		}
		// если хранилище уже было подгружено - это пофигу, просто ничего не произойдет
		// поэтому его общее хранилище мы тоже не убираем из возможных заявок на подгрузку
	}
	depot_log("enter_char: %s done", GET_NAME(ch));
}

/**
* Перевод онлайн списка в оффлайн.
*/
void CharNode::online_to_offline(ObjListType &cont, int file_type)
{
	depot_log("online_to_offline: %s, %i", name.c_str(), file_type);
	if (file_type == PERS_DEPOT_FILE && state[PERS_CHEST])
	{
		write_obj_file(name, file_type, cont);
		state.reset(PERS_CHEST);
		depot_log("online_to_offline: pers_depot saved");
	}
	if (file_type == SHARE_DEPOT_FILE && state[SHARE_CHEST])
	{
		write_obj_file(name, file_type, cont);
		state.reset(SHARE_CHEST);
		depot_log("online_to_offline: share_depot saved");
	}

	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		depot_log("online_to_offline: %s (uid = %li)", GET_OBJ_PNAME(*obj_it, 0), GET_OBJ_UID(*obj_it));
		OfflineNode tmp_obj;
		tmp_obj.vnum = GET_OBJ_VNUM(*obj_it);
		tmp_obj.timer = GET_OBJ_TIMER(*obj_it);
		tmp_obj.rent_cost = get_object_low_rent(*obj_it);
		tmp_obj.uid = GET_OBJ_UID(*obj_it);
		offline_list.push_back(tmp_obj);
		extract_obj(*obj_it);
		// из макс.в мире он никуда не уходит
		int rnum = real_object(tmp_obj.vnum);
		if (rnum >= 0)
			obj_index[rnum].number++;
	}
	cont.clear();
	depot_log("online_to_offline: %s done", name.c_str());
}

/**
* Проверка, расшарено ли кому-нить это хранилище.
*/
bool CharNode::any_other_share()
{
	depot_log("any_other_share: %s", name.c_str());
	for (AllowedCharType::const_iterator al_it = allowed_chars.begin();
			al_it != allowed_chars.end(); ++al_it)
	{
		if (al_it->second.mutual)
		{
			DESCRIPTOR_DATA *d = DescByUID(al_it->first);
			if (d)
			{
				depot_log("any_other_share: %s true (%s)", name.c_str(), GET_NAME(d->character));
				return true;
			}
		}
	}
	depot_log("any_other_share: %s fasle", name.c_str());
	return false;
}

/**
* Выход чара из игры - перевод хранилищ в оффлайн.
*/
void exit_char(CHAR_DATA *ch)
{
	depot_log("exit_char: %s", GET_NAME(ch));
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		// персональное хранилище в любом случае уходит в оффлайн
		// даже пустое нужно перезаписать, потому что если взяли все с персонального,
		// сунули в общее и тут же вышли - файл персонального останется и при входе он первым
		// обработается, шмотки залоадятся опять в персональном, а общее будет без них
		it->second.online_to_offline(it->second.pers_online, PERS_DEPOT_FILE);
		// общее хранилище уходит только если в онлайне не осталось никого из доверенных чаров
		// в любом случае оно сохраняется если не сейчас, то на тике
		if (!it->second.any_other_share())
			it->second.online_to_offline(it->second.share_online, SHARE_DEPOT_FILE);

		it->second.ch = 0;
		it->second.money = GET_BANK_GOLD(ch) + GET_GOLD(ch);
		it->second.money_spend = 0;
	}
	depot_log("exit_char: %s done", GET_NAME(ch));
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
* Подгрузка общих хранилищ, ожидающих объединения и перевод в оффлайн ненужных.
*/
void load_share_depots()
{
	depot_log("load_share_depots: start");
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		if (!it->second.waiting_allowed_chars.empty())
		{
			for(std::vector<long>::iterator w_it = it->second.waiting_allowed_chars.begin();
				 w_it != it->second.waiting_allowed_chars.end(); ++w_it)
			{
				// ищем чара, чье хранилище нам нужно и грузим только если оно не было уже загружено
				DepotListType::iterator vict_it = depot_list.find(*w_it);
				if (vict_it != depot_list.end() && vict_it->second.share_online.empty())
					vict_it->second.load_online_objs(SHARE_DEPOT_FILE);
			}
			it->second.waiting_allowed_chars.clear();
		}

		// перевод в оффлайн расшаренных хранилищ, оставшихся без надобности...
		if (!it->second.ch && !it->second.share_online.empty() && !it->second.any_other_share())
			it->second.online_to_offline(it->second.share_online, SHARE_DEPOT_FILE);
	}
	depot_log("load_share_depots: end");
}

/**
* Обработка ренейма чара - чтобы не выводить в общих хранилищах левые имена.
*/
void rename_char(long uid)
{
	depot_log("rename_char: %li", uid);
	std::string new_name = GetNameByUnique(uid);
	if (new_name.empty())
	{
		log("Хранилище: невалидный uid (%li)", uid);
		return;
	}
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		AllowedCharType::iterator al_it = it->second.allowed_chars.find(uid);
		if (al_it != it->second.allowed_chars.end())
		{
			al_it->second.name = new_name;
			continue;
		}
	}
	depot_log("rename_char: done");
}

/**
* Релоад шмоток чара на случай отката.
*/
void reload_char(long uid)
{
	depot_log("reload_char: %li", uid);
	DepotListType::iterator it = create_depot(uid);
	// сначала обнуляем все, что надо
	it->second.reset();
	// потом читаем шмот
	it->second.load_online_objs(PERS_DEPOT_FILE, 1);
	it->second.load_online_objs(SHARE_DEPOT_FILE, 1);
	depot_log("reload_char: %li done", uid);
}

/**
* Вывод инфы в show stats.
* TODO: добавить вывод расходов, может еще процент наполненности персональных хранилищ.
*/
void show_stats(CHAR_DATA *ch)
{
	std::stringstream out;
	int pers_count = 0, share_count = 0, offline_count = 0;
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		pers_count += it->second.pers_online.size();
		share_count += it->second.share_online.size();
		offline_count += it->second.offline_list.size();
	}

	out << "  Всего хранилищ: " << depot_list.size() << "\r\n"
		<< "  Предметов в персональных хранилищах: " << pers_count << "\r\n"
		<< "  Предметов в общих хранилищах: " << share_count << "\r\n"
		<< "  Предметов в оффлайне: " << offline_count << "\r\n";
	send_to_char(out.str().c_str(), ch);
}

} // namespace Depot
