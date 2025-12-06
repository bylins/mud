// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "glory.h"

#include "administration/karma.h"
#include "engine/core/conf.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/db/db.h"
#include "engine/core/comm.h"
#include "administration/password.h"
#include "gameplay/core/genchar.h"
#include "engine/ui/color.h"
#include "gameplay/statistics/top.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/ui/modify.h"
#include "glory_misc.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <sstream>

extern void check_max_hp(CharData *ch);

namespace Glory {

// таймеры вложенной славы
struct glory_time {
	glory_time() : glory(0), timer(0), stat(0) {};
	int glory; // кол-во славы
	int timer; // сколько тиков осталось
	int stat; // в какой стат вложена (номер G_STR ... G_CHA)
};

typedef std::shared_ptr<struct glory_time> GloryTimePtr;
typedef std::list<GloryTimePtr> GloryTimeType;
class GloryNode;
typedef std::shared_ptr<GloryNode> GloryNodePtr;
typedef std::map<long, GloryNodePtr> GloryListType; // first - уид

class GloryNode {
 public:
	GloryNode() : free_glory(0), spend_glory(0), denial(0), hide(false), freeze(false) {};

	GloryNode &operator=(const GloryNode &);
	void copy_glory(const GloryNodePtr& k);

	int free_glory;			// свободная слава на руках
	int spend_glory;		// потраченная слава в статах
	GloryTimeType timers;	// таймеры вложенной славы
	int denial;				// таймер ограничения на перекладывание вложенной славы
	std::string name;		// для топа
	bool hide;				// показывать или нет в топе прославленных
	bool freeze;			// состояние фриза (таймеры не тикают)

 private:
	void copy_stat(const GloryTimePtr& k);
};

class spend_glory {
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

const int MAX_STATS_BY_GLORY = 10; // сколько статов можно прокинуть славой
const int MAX_GLORY_TIMER = 267840; // таймер вложенной славы (в минутах)
const int DISPLACE_TIMER = 20160; // таймер на переклдывание вложенной славы между статами (в минутах)
GloryListType glory_list; // общий список славы на чарах, в том числе влитой в статы

// * Копирование всех полей, чтобы не морочить голову с копированием указателей.
GloryNode &GloryNode::operator=(const GloryNode &t) {
	free_glory = t.free_glory;
	spend_glory = t.spend_glory;
	denial = t.denial;
	name = t.name;
	hide = t.hide;
	freeze = t.freeze;
	timers.clear();
	for (const auto & timer : t.timers) {
		GloryTimePtr temp_timer(new glory_time);
		temp_timer->stat = timer->stat;
		temp_timer->glory = timer->glory;
		temp_timer->timer = timer->timer;
		timers.push_back(temp_timer);
	}
	return *this;
}

// * Копирование ноды с влитым статом от k с проверкой на макс_стат.
void GloryNode::copy_stat(const GloryTimePtr& k) {
	if (spend_glory < MAX_STATS_BY_GLORY) {
		GloryTimePtr tmp_node(new glory_time);
		*tmp_node = *k;
		tmp_node->glory = MIN(k->glory, MAX_STATS_BY_GLORY - spend_glory);
		spend_glory += tmp_node->glory;
		timers.push_back(tmp_node);
	}
}

// * Копирование свободной и влитой (по возможности) славы от k.
void GloryNode::copy_glory(const GloryNodePtr& k) {
	free_glory += k->free_glory;
	for (auto & timer : k->timers) {
		copy_stat(timer);
	}
}

// * Загрузка общего списка славы, проверка на валидность чара, славы, сверка мд5 файла.
void load_glory() {
	const char *glory_file = LIB_PLRSTUFF"glory.lst";
	std::ifstream file(glory_file);
	if (!file.is_open()) {
		log("Glory: не удалось открыть файл на чтение: %s", glory_file);
		return;
	}

	long all_sum = 0;
	std::string buffer;
	bool checksum = false;
	while (file >> buffer) {
		if (buffer == "<Node>") {
			GloryNodePtr temp_node(new GloryNode);
			long uid = 0;
			int free_glory = 0, denial = 0;
			bool hide = false, freeze = false;

			if (!(file >> uid >> free_glory >> denial >> hide >> freeze)) {
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
			if (buffer != "<Glory>") {
				log("Glory: ошибка чтения секции <Glory>");
				break;
			}

			while (file >> buffer) {
				if (buffer == "</Glory>") break;

				int glory = 0, stat = 0, timer = 0;

				try {
					glory = std::stoi(buffer, nullptr, 10);
				}
				catch (const std::invalid_argument &) {
					log("Glory: ошибка чтения glory (%s)", buffer.c_str());
					break;
				}

				all_sum += glory;

				if (!(file >> stat >> timer)) {
					log("Glory: ошибка чтения: %d, %d", stat, timer);
					break;
				}

				GloryTimePtr temp_glory_timers(new glory_time);
				temp_glory_timers->glory = glory;
				temp_glory_timers->timer = timer;
				temp_glory_timers->stat = stat;
				// даже если править файл славы руками - учитываем только первые 10 вложенных статов
				// а если там одним куском не получается обрезать - то и фик с ним, это всеравно был чит
				if (temp_node->spend_glory + glory <= MAX_STATS_BY_GLORY) {
					temp_node->timers.push_back(temp_glory_timers);
					temp_node->spend_glory += glory;
				} else {
					log("Glory: некорректное кол-во вложенных статов, %d %d %d пропущено (uid: %ld).",
						glory,
						stat,
						timer,
						uid);
				}

				all_sum += stat + timer;
			}
			if (buffer != "</Glory>") {
				log("Glory: ошибка чтения секции </Glory>.");
				break;
			}

			file >> buffer;
			if (buffer != "</Node>") {
				log("Glory: ошибка чтения </Node>: %s", buffer.c_str());
				break;
			}

			// проверяем есть ли еще такой чар вообще
			std::string name = GetNameByUnique(uid);
			if (name.empty()) {
				log("Glory: UID %ld - персонажа не существует.", uid);
				continue;
			}
			temp_node->name = name;

			if (!temp_node->free_glory && !temp_node->spend_glory) {
				log("Glory: UID %ld - не осталось вложенной или свободной славы.", uid);
				continue;
			}

			// добавляем в список
			glory_list[uid] = temp_node;
		} else if (buffer == "<End>") {
			// сверочка мд5
			file >> buffer;
			int result = Password::compare_password(buffer, fmt::format("{}", all_sum));
			checksum = true;
			if (!result) {
				// FIXME тут надо другое оповещение, но потом
				log("Glory: несовпадение общей контрольной суммы (%s).", buffer.c_str());
			}
		} else {
			log("Glory: неизвестный блок при чтении файла: %s", buffer.c_str());
			break;
		}
	}
	if (!checksum) {
		// FIXME тут надо другое оповещение, но потом
		log("Glory: проверка общей контрольной суммы не производилась.");
	}
}

// * Сохранение общего списка славы, запись мд5 файла.
void save_glory() {
	long all_sum = 0;
	std::stringstream out;

	for (const auto & it : glory_list) {
		out << "<Node>\n" << it.first << " " << it.second->free_glory << " " << it.second->denial << " "
			<< it.second->hide << " " << it.second->freeze << "\n<Glory>\n";
		all_sum += it.first + it.second->free_glory + it.second->denial + it.second->hide + it.second->freeze;
		for (const auto & timer : it.second->timers) {
			out << timer->glory << " " << timer->stat << " " << timer->timer << "\n";
			all_sum += timer->stat + timer->glory + timer->timer;
		}
		out << "</Glory>\n</Node>\n";
	}
	out << "<End>\n";
	// TODO: в file_crc систему это надо
	out << Password::generate_md5_hash(fmt::format("{}", all_sum)) << "\n";

	const char *glory_file = LIB_PLRSTUFF"glory.lst";
	std::ofstream file(glory_file);
	if (!file.is_open()) {
		log("Glory: не удалось открыть файл на запись: %s", glory_file);
		return;
	}
	file << out.rdbuf();
	file.close();
}

// * Аналог бывшего макроса GET_GLORY().
int get_glory(long uid) {
	int glory = 0;
	auto it = glory_list.find(uid);
	if (it != glory_list.end())
		glory = it->second->free_glory;
	return glory;
}

// * Добавление славы чару, создание новой записи при необходимости, уведомление, если чар онлайн.
void add_glory(long uid, int amount) {
	if (uid <= 0 || amount <= 0) return;

	auto it = glory_list.find(uid);
	if (it != glory_list.end()) {
		it->second->free_glory += amount;
	} else {
		GloryNodePtr temp_node(new GloryNode);
		temp_node->free_glory = amount;
		glory_list[uid] = temp_node;
	}
	save_glory();
	DescriptorData *d = DescriptorByUid(uid);
	if (d)
		SendMsgToChar(d->character.get(), "Вы заслужили %d %s славы.\r\n",
					  amount, GetDeclensionInNumber(amount, EWhat::kPoint));
}

/**
* Удаление славы у чара (свободной), ниже нуля быть не может.
* \return 0 - ничего не списано, число > 0 - сколько реально списали
*/
int remove_glory(long uid, int amount) {
	if (uid <= 0 || amount <= 0) return 0;
	int real_removed = amount;

	auto it = glory_list.find(uid);
	if (it != glory_list.end()) {
		// ниже нуля ес-сно не отнимаем
		if (it->second->free_glory >= amount)
			it->second->free_glory -= amount;
		else {
			real_removed = it->second->free_glory;
			it->second->free_glory = 0;
		}
		// смысла пустую запись оставлять до ребута нет
		if (!it->second->free_glory && !it->second->spend_glory)
			glory_list.erase(it);

		save_glory();
	} else {
		real_removed = 0;
	}
	return real_removed;
}

// * Просто, чтобы не дублировать в разных местах одно и тоже.
void print_denial_message(CharData *ch, int denial) {
	SendMsgToChar(ch, "Вы не сможете изменить уже вложенные очки славы (%s).\r\n", FormatTimeToStr(denial).c_str());
}

/**
* Проверка на запрет перекладывания статов при редактировании в олц.
* \return 0 - запрещено, 1 - разрешено
*/
bool parse_denial_check(CharData *ch, int stat) {
	// при включенном таймере статы ниже уже вложенных не опускаются
	if (ch->desc->glory->olc_node->denial) {
		bool stop = false;
		switch (stat) {
			case G_STR:
				if (ch->get_str() == ch->desc->glory->olc_str)
					stop = true;
				break;
			case G_DEX:
				if (ch->get_dex() == ch->desc->glory->olc_dex)
					stop = true;
				break;
			case G_INT:
				if (ch->get_int() == ch->desc->glory->olc_int)
					stop = true;
				break;
			case G_WIS:
				if (ch->get_wis() == ch->desc->glory->olc_wis)
					stop = true;
				break;
			case G_CON:
				if (ch->get_con() == ch->desc->glory->olc_con)
					stop = true;
				break;
			case G_CHA:
				if (ch->get_cha() == ch->desc->glory->olc_cha)
					stop = true;
				break;
			default: log("Glory: невалидный номер стат: %d (uid: %ld, name: %s)", stat, ch->get_uid(), GET_NAME(ch));
				stop = true;
		}
		if (stop) {
			print_denial_message(ch, ch->desc->glory->olc_node->denial);
			return false;
		}
	}
	return true;
}

/**
* Состояний тут три:
* stat = -1 - перекладывание статов, таймер сохранен, стат ждет своего вложения
* timer = 0 - новый вложенный стат при редактировании
* timer > 0 - вложенный стат, который никто не трогал при редактировании
*/
bool parse_remove_stat(CharData *ch, int stat) {
	if (!parse_denial_check(ch, stat)) {
		return false;
	}

	for (auto it = ch->desc->glory->olc_node->timers.begin();
		 it != ch->desc->glory->olc_node->timers.end(); ++it) {
		if ((*it)->stat == stat) {
			(*it)->glory -= 1;

			if ((*it)->glory > 0) {
				// если мы убрали только часть статов, то надо сохранять таймер этой части
				// да, все это тупняк и надо было разбирать еще в момент входа/выхода олц
				GloryTimePtr temp_timer(new glory_time);
				temp_timer->stat = -1;
				temp_timer->timer = (*it)->timer;
				// в начало списка, чтобы при вложении он ушел первым
				ch->desc->glory->olc_node->timers.push_front(temp_timer);

			} else if ((*it)->glory == 0 && (*it)->timer != 0) {
				// если в стате не осталось плюсов (при перекладывании), то освобождаем поле стата,
				// но оставляем таймер и перемещаем запись в начало списка, чтобы обрабатывать его первым при вложении
				(*it)->stat = -1;
				ch->desc->glory->olc_node->timers.push_front(*it);
				ch->desc->glory->olc_node->timers.erase(it);
			} else if ((*it)->glory == 0 && (*it)->timer == 0) {
				// держать нулевой в принципе смысла никакого нет
				ch->desc->glory->olc_node->timers.erase(it);
			}
			ch->desc->glory->olc_add_spend_glory -= 1;
			ch->desc->glory->olc_node->free_glory += 1000;
			return true;
		}
	}
	log("Glory: не найден стат при вычитании в олц (stat: %d, uid: %ld, name: %s)", stat, ch->get_uid(), GET_NAME(ch));
	return true;
}

// * Добавление стата в олц, проверка на свободные записи с включенным таймером, чтобы учесть перекладывание первым.
void parse_add_stat(CharData *ch, int stat) {
	// в любом случае стат добавится
	ch->desc->glory->olc_add_spend_glory += 1;
	ch->desc->glory->olc_node->free_glory -= 1000;

	for (auto it = ch->desc->glory->olc_node->timers.begin();
		 it != ch->desc->glory->olc_node->timers.end(); ++it) {
		// если есть какой-то невложенный стат (переливание), то в первую очередь вливаем его,
		// чтобы учесть его таймеры. после перекладки уже идет вливание новой славы
		if ((*it)->stat == -1) {
			// это полный идиотизм, но чет я тогда не учел эту тему, а щас уже подпираю побыстрому
			// ищем уже вложенный такой же стат с тем же таймером, чтобы не плодить разные записи
			for (auto tmp_it = ch->desc->glory->olc_node->timers.cbegin();
				 tmp_it != ch->desc->glory->olc_node->timers.cend(); ++tmp_it) {
				if ((*tmp_it)->stat == stat && (*tmp_it)->timer == (*it)->timer) {
					(*tmp_it)->glory += 1;
					ch->desc->glory->olc_node->timers.erase(it);
					return;
				}
			}
			// если статов с тем же таймером нет - пишем его как и было отдельной записью
			(*it)->stat = stat;
			(*it)->glory += 1;
			return;
		}

		// если уже есть вложения в тот же стат в текущей сессии редактирования
		// - прибавляем в тоже поле, потому что таймеры будут одинаковые
		if ((*it)->stat == stat && (*it)->timer == 0) {
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

// * Парс олц меню 'слава'.
bool parse_spend_glory_menu(CharData *ch, const char *arg) {
	switch (*arg) {
		case 'А':
		case 'а':
			if (ch->desc->glory->olc_add_str >= 1) {
				if (!parse_remove_stat(ch, G_STR)) break;
				ch->desc->glory->olc_str -= 1;
				ch->desc->glory->olc_add_str -= 1;
			}
			break;
		case 'Б':
		case 'б':
			if (ch->desc->glory->olc_add_dex >= 1) {
				if (!parse_remove_stat(ch, G_DEX)) break;
				ch->desc->glory->olc_dex -= 1;
				ch->desc->glory->olc_add_dex -= 1;
			}
			break;
		case 'Г':
		case 'г':
			if (ch->desc->glory->olc_add_int >= 1) {
				if (!parse_remove_stat(ch, G_INT)) break;
				ch->desc->glory->olc_int -= 1;
				ch->desc->glory->olc_add_int -= 1;
			}
			break;
		case 'Д':
		case 'д':
			if (ch->desc->glory->olc_add_wis >= 1) {
				if (!parse_remove_stat(ch, G_WIS)) break;
				ch->desc->glory->olc_wis -= 1;
				ch->desc->glory->olc_add_wis -= 1;
			}
			break;
		case 'Е':
		case 'е':
			if (ch->desc->glory->olc_add_con >= 1) {
				if (!parse_remove_stat(ch, G_CON)) break;
				ch->desc->glory->olc_con -= 1;
				ch->desc->glory->olc_add_con -= 1;
			}
			break;
		case 'Ж':
		case 'ж':
			if (ch->desc->glory->olc_add_cha >= 1) {
				if (!parse_remove_stat(ch, G_CHA)) break;
				ch->desc->glory->olc_cha -= 1;
				ch->desc->glory->olc_add_cha -= 1;
			}
			break;
		case 'З':
		case 'з':
			if (ch->desc->glory->olc_node->free_glory >= 1000
				&& ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY) {
				parse_add_stat(ch, G_STR);
				ch->desc->glory->olc_str += 1;
				ch->desc->glory->olc_add_str += 1;
			}
			break;
		case 'И':
		case 'и':
			if (ch->desc->glory->olc_node->free_glory >= 1000
				&& ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY) {
				parse_add_stat(ch, G_DEX);
				ch->desc->glory->olc_dex += 1;
				ch->desc->glory->olc_add_dex += 1;
			}
			break;
		case 'К':
		case 'к':
			if (ch->desc->glory->olc_node->free_glory >= 1000
				&& ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY) {
				parse_add_stat(ch, G_INT);
				ch->desc->glory->olc_int += 1;
				ch->desc->glory->olc_add_int += 1;
			}
			break;
		case 'Л':
		case 'л':
			if (ch->desc->glory->olc_node->free_glory >= 1000
				&& ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY) {
				parse_add_stat(ch, G_WIS);
				ch->desc->glory->olc_wis += 1;
				ch->desc->glory->olc_add_wis += 1;
			}
			break;
		case 'М':
		case 'м':
			if (ch->desc->glory->olc_node->free_glory >= 1000
				&& ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY) {
				parse_add_stat(ch, G_CON);
				ch->desc->glory->olc_con += 1;
				ch->desc->glory->olc_add_con += 1;
			}
			break;
		case 'Н':
		case 'н':
			if (ch->desc->glory->olc_node->free_glory >= 1000
				&& ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY) {
				parse_add_stat(ch, G_CHA);
				ch->desc->glory->olc_cha += 1;
				ch->desc->glory->olc_add_cha += 1;
			}
			break;
		case 'В':
		case 'в': {
			// проверка, чтобы не записывать зря, а только при изменения
			// и чтобы нельзя было из стата славу вытащить
			if ((ch->desc->glory->olc_str == ch->get_str()
				&& ch->desc->glory->olc_dex == ch->get_dex()
				&& ch->desc->glory->olc_int == ch->get_int()
				&& ch->desc->glory->olc_wis == ch->get_wis()
				&& ch->desc->glory->olc_con == ch->get_con()
				&& ch->desc->glory->olc_cha == ch->get_cha())
				|| ch->desc->glory->olc_add_spend_glory < ch->desc->glory->olc_node->spend_glory) {
				return false;
			}

			auto it = glory_list.find(ch->get_uid());
			if (it == glory_list.end()
				|| ch->desc->glory->check_spend_glory != it->second->spend_glory
				|| ch->desc->glory->check_free_glory != it->second->free_glory
				|| ch->desc->glory->olc_node->hide != it->second->hide
				|| ch->desc->glory->olc_node->freeze != it->second->freeze) {
				// пустое может получиться например если с него в этот момент трансфернули славу
				// остальное - за время сидения в олц что-то изменилось, дали славу, просрочились статы и т.п.
				ch->desc->glory.reset();
				ch->desc->state = EConState::kPlaying;
				SendMsgToChar("Редактирование отменено из-за внешних изменений.\r\n", ch);
				return true;
			}

			// включаем таймер, если было переливание статов
			if (ch->get_str() < ch->desc->glory->olc_str
				|| ch->get_dex() < ch->desc->glory->olc_dex
				|| ch->get_int() < ch->desc->glory->olc_int
				|| ch->get_wis() < ch->desc->glory->olc_wis
				|| ch->get_con() < ch->desc->glory->olc_con
				|| ch->get_cha() < ch->desc->glory->olc_cha) {
				ch->desc->glory->olc_node->denial = DISPLACE_TIMER;
			}

			ch->set_str(ch->desc->glory->olc_str);
			ch->set_dex(ch->desc->glory->olc_dex);
			ch->set_con(ch->desc->glory->olc_con);
			ch->set_wis(ch->desc->glory->olc_wis);
			ch->set_int(ch->desc->glory->olc_int);
			ch->set_cha(ch->desc->glory->olc_cha);

			// проставляем таймеры, потому что в олц удобнее иметь нулевые для новых статов
			for (const auto & timer : ch->desc->glory->olc_node->timers) {
				if (timer->timer == 0)
					timer->timer = MAX_GLORY_TIMER;
			}

			ch->desc->glory->olc_node->spend_glory = ch->desc->glory->olc_add_spend_glory;
			*(it->second) = *(ch->desc->glory->olc_node);
			save_glory();

			ch->desc->glory.reset();
			ch->desc->state = EConState::kPlaying;
			check_max_hp(ch);
			SendMsgToChar("Ваши изменения сохранены.\r\n", ch);
			return true;
		}
		case 'Х':
		case 'х': ch->desc->glory.reset();
			ch->desc->state = EConState::kPlaying;
			SendMsgToChar("Редактирование прервано.\r\n", ch);
			return true;
		default: break;
	}
	return false;
}

// * Распечатка олц меню.
void spend_glory_menu(CharData *ch) {
	std::ostringstream out;
	out << "\r\n                      -        +\r\n"
		<< "  Сила     : ("
		<< kColorBoldGrn << "А" << kColorNrm
		<< ") " << ch->desc->glory->olc_str << " ("
		<< kColorBoldGrn << "З" << kColorNrm
		<< ")";
	if (ch->desc->glory->olc_add_str)
		out << "    (" << (ch->desc->glory->olc_add_str > 0 ? "+" : "") << ch->desc->glory->olc_add_str << ")";
	out << "\r\n"
		<< "  Ловкость : ("
		<< kColorBoldGrn << "Б" << kColorNrm
		<< ") " << ch->desc->glory->olc_dex << " ("
		<< kColorBoldGrn << "И" << kColorNrm
		<< ")";
	if (ch->desc->glory->olc_add_dex)
		out << "    (" << (ch->desc->glory->olc_add_dex > 0 ? "+" : "") << ch->desc->glory->olc_add_dex << ")";
	out << "\r\n"
		<< "  Ум       : ("
		<< kColorBoldGrn << "Г" << kColorNrm
		<< ") " << ch->desc->glory->olc_int << " ("
		<< kColorBoldGrn << "К" << kColorNrm
		<< ")";
	if (ch->desc->glory->olc_add_int)
		out << "    (" << (ch->desc->glory->olc_add_int > 0 ? "+" : "") << ch->desc->glory->olc_add_int << ")";
	out << "\r\n"
		<< "  Мудрость : ("
		<< kColorBoldGrn << "Д" << kColorNrm
		<< ") " << ch->desc->glory->olc_wis << " ("
		<< kColorBoldGrn << "Л" << kColorNrm
		<< ")";
	if (ch->desc->glory->olc_add_wis)
		out << "    (" << (ch->desc->glory->olc_add_wis > 0 ? "+" : "") << ch->desc->glory->olc_add_wis << ")";
	out << "\r\n"
		<< "  Здоровье : ("
		<< kColorBoldGrn << "Е" << kColorNrm
		<< ") " << ch->desc->glory->olc_con << " ("
		<< kColorBoldGrn << "М" << kColorNrm
		<< ")";
	if (ch->desc->glory->olc_add_con)
		out << "    (" << (ch->desc->glory->olc_add_con > 0 ? "+" : "") << ch->desc->glory->olc_add_con << ")";
	out << "\r\n"
		<< "  Обаяние  : ("
		<< kColorBoldGrn << "Ж" << kColorNrm
		<< ") " << ch->desc->glory->olc_cha << " ("
		<< kColorBoldGrn << "Н" << kColorNrm
		<< ")";
	if (ch->desc->glory->olc_add_cha)
		out << "    (" << (ch->desc->glory->olc_add_cha > 0 ? "+" : "") << ch->desc->glory->olc_add_cha << ")";
	out << "\r\n\r\n"
		<< "  Можно добавить: "
		<< MIN((MAX_STATS_BY_GLORY - ch->desc->glory->olc_add_spend_glory),
			   ch->desc->glory->olc_node->free_glory / 1000)
		<< "\r\n\r\n";

	int diff = ch->desc->glory->olc_node->spend_glory - ch->desc->glory->olc_add_spend_glory;
	if (diff > 0) {
		out << "  Вы должны распределить вложенные ранее " << diff << " " << GetDeclensionInNumber(diff, EWhat::kPoint) << "\r\n";
	} else if (ch->desc->glory->olc_add_spend_glory > ch->desc->glory->olc_node->spend_glory
		|| ch->desc->glory->olc_str != ch->get_str()
		|| ch->desc->glory->olc_dex != ch->get_dex()
		|| ch->desc->glory->olc_int != ch->get_int()
		|| ch->desc->glory->olc_wis != ch->get_wis()
		|| ch->desc->glory->olc_con != ch->get_con()
		|| ch->desc->glory->olc_cha != ch->get_cha()) {
		out << "  "
			<< kColorBoldGrn << "В" << kColorNrm
			<< ") Сохранить результаты\r\n";
	}

	out << "  "
		<< kColorBoldGrn << "Х" << kColorNrm
		<< ") Выйти без сохранения\r\n"
		<< " Ваш выбор: ";
	SendMsgToChar(out.str(), ch);
}

// * Распечатка 'слава информация'.
void print_glory(CharData *ch, GloryListType::iterator &it) {
	std::ostringstream out;
	for (auto tm_it = it->second->timers.cbegin(); tm_it != it->second->timers.cend(); ++tm_it) {
		switch ((*tm_it)->stat) {
			case G_STR: out << "Сила : +" << (*tm_it)->glory << " (" << FormatTimeToStr((*tm_it)->timer) << ")\r\n";
				break;
			case G_DEX: out << "Подв : +" << (*tm_it)->glory << " (" << FormatTimeToStr((*tm_it)->timer) << ")\r\n";
				break;
			case G_INT: out << "Ум   : +" << (*tm_it)->glory << " (" << FormatTimeToStr((*tm_it)->timer) << ")\r\n";
				break;
			case G_WIS: out << "Мудр : +" << (*tm_it)->glory << " (" << FormatTimeToStr((*tm_it)->timer) << ")\r\n";
				break;
			case G_CON: out << "Тело : +" << (*tm_it)->glory << " (" << FormatTimeToStr((*tm_it)->timer) << ")\r\n";
				break;
			case G_CHA: out << "Обаян: +" << (*tm_it)->glory << " (" << FormatTimeToStr((*tm_it)->timer) << ")\r\n";
				break;
			default: log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
		}
	}
	out << "Свободных очков: " << it->second->free_glory << "\r\n";
	SendMsgToChar(out.str(), ch);
}

// * Команда 'слава' - вложение имеющейся у игрока славы в статы без участия иммов.
void do_spend_glory(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	auto it = glory_list.find(ch->get_uid());
	if (it == glory_list.end() || IS_IMMORTAL(ch)) {
		SendMsgToChar("Вам это не нужно...\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	if (CompareParam(buffer2, "информация")) {
		SendMsgToChar("Информация о вложенных вами очках славы:\r\n", ch);
		print_glory(ch, it);
		return;
	}

	if (it->second->free_glory < 1000) {
		if (!it->second->spend_glory) {
			SendMsgToChar("У вас недостаточно очков славы для использования этой команды.\r\n", ch);
			return;
		} else if (it->second->denial) {
			print_denial_message(ch, it->second->denial);
			return;
		}
	}
	if (it->second->spend_glory >= MAX_STATS_BY_GLORY && it->second->denial) {
		SendMsgToChar("У вас уже вложено максимально допустимое количество славы.\r\n", ch);
		print_denial_message(ch, it->second->denial);
		return;
	}

	std::shared_ptr<class spend_glory> temp_glory(new spend_glory);
	temp_glory->olc_str = ch->get_str();
	temp_glory->olc_dex = ch->get_dex();
	temp_glory->olc_int = ch->get_int();
	temp_glory->olc_wis = ch->get_wis();
	temp_glory->olc_con = ch->get_con();
	temp_glory->olc_cha = ch->get_cha();

	// я не помню уже, как там этот шаред-птр ведет себя при дефолтном копировании
	// а т.к. внутри есть список таймеров на них - скопирую вручную и пофигу на все
	GloryNodePtr olc_node(new GloryNode);
	*olc_node = *(it->second);
	temp_glory->olc_node = olc_node;
	temp_glory->olc_add_spend_glory = it->second->spend_glory;
	// поля для контрольных сверок до и после
	temp_glory->check_spend_glory = it->second->spend_glory;
	temp_glory->check_free_glory = it->second->free_glory;

	for (auto tm_it = it->second->timers.cbegin(); tm_it != it->second->timers.cend(); ++tm_it) {
		switch ((*tm_it)->stat) {
			case G_STR: temp_glory->olc_add_str += (*tm_it)->glory;
				break;
			case G_DEX: temp_glory->olc_add_dex += (*tm_it)->glory;
				break;
			case G_INT: temp_glory->olc_add_int += (*tm_it)->glory;
				break;
			case G_WIS: temp_glory->olc_add_wis += (*tm_it)->glory;
				break;
			case G_CON: temp_glory->olc_add_con += (*tm_it)->glory;
				break;
			case G_CHA: temp_glory->olc_add_cha += (*tm_it)->glory;
				break;
			default: log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
		}
	}

	ch->desc->glory = temp_glory;
	ch->desc->state = EConState::kSpendGlory;
	spend_glory_menu(ch);
}

// * Удаление статов у чара, если он онлайн.
void remove_stat_online(long uid, int stat, int glory) {
	DescriptorData *d = DescriptorByUid(uid);
	if (d) {
		switch (stat) {
			case G_STR: d->character->inc_str(-glory);
				break;
			case G_DEX: d->character->inc_dex(-glory);
				break;
			case G_INT: d->character->inc_int(-glory);
				break;
			case G_WIS: d->character->inc_wis(-glory);
				break;
			case G_CON: d->character->inc_con(-glory);
				break;
			case G_CHA: d->character->inc_cha(-glory);
				break;
			default: log("Glory: некорректный номер стата %d (uid: %ld)", stat, uid);
		}
	}
}

// * Тикание таймеров славы и запрета перекладывания, снятие просроченной славы с чара онлайн.
void timers_update() {
	for (auto & it : glory_list) {
		if (it.second->freeze) {
			continue;
		}
		bool removed = false; // флажок для сообщения о пропадании славы (чтобы не спамить на каждый стат)
		for (auto tm_it = it.second->timers.begin(); tm_it != it.second->timers.end();) {
			if ((*tm_it)->timer > 0) {
				(*tm_it)->timer -= 1;
			}
			if ((*tm_it)->timer <= 0) {
				removed = true;
				remove_stat_online(it.first, (*tm_it)->stat, (*tm_it)->glory);
				it.second->spend_glory -= (*tm_it)->glory;
				it.second->timers.erase(tm_it++);
			} else {
				++tm_it;
			}
		}
		// тут же тикаем ограничением на переливания
		if (it.second->denial > 0) {
			it.second->denial -= 1;
		}

		if (removed) {
			DescriptorData *d = DescriptorByUid(it.first);
			if (d) {
				SendMsgToChar("Вы долго не совершали достойных деяний и слава вас покинула...\r\n",
							 d->character.get());
			}
		}

	}

	// тут еще есть момент, что в меню таймеры не идут, в случае последующей записи изменений
	// в принципе фигня канеш, но тем не менее - хорошо бы учесть
	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->state != EConState::kSpendGlory || !d->glory) continue;
		for (auto d_it = d->glory->olc_node->timers.begin();
			 d_it != d->glory->olc_node->timers.end(); ++d_it) {
			// здесь мы не тикаем denial и не удаляем просроченные статы, а просто вываливаем из олц
			if ((*d_it)->timer > 0) {
				// обрабатываем мы только таймеры > 0, потому что вложенные во время редактирования
				// статы имеют нулевой таймер, а нас интересуют только нулевые после снятия с единицы
				(*d_it)->timer -= 1;
				if ((*d_it)->timer <= 0) {
					d->glory.reset();
					d->state = EConState::kPlaying;
					SendMsgToChar("Редактирование отменено из-за внешних изменений.\r\n", d->character.get());
					return;
				}
			}
		}
	}
}

/**
* Удаление иммом у чара славы, уже вложенной в статы (glory remove).
* \return 0 - ничего не снято, 1 - снято сколько просили
*/
bool remove_stats(CharData *ch, CharData *god, int amount) {
	auto it = glory_list.find(ch->get_uid());
	if (it == glory_list.end()) {
		SendMsgToChar(god, "У %s нет вложенной славы.\r\n", GET_PAD(ch, 1));
		return false;
	}
	if (amount > it->second->spend_glory) {
		SendMsgToChar(god, "У %s нет столько вложенной славы.\r\n", GET_PAD(ch, 1));
		return false;
	}
	if (amount <= 0) {
		SendMsgToChar(god, "Некорректное количество статов (%d).\r\n", amount);
		return false;
	}

	int removed = 0;
	// собсна не заморачиваясь просто режем первые попавшиеся статы
	for (auto tm_it = it->second->timers.begin(); tm_it != it->second->timers.end();) {
		if (amount > 0) {
			if ((*tm_it)->glory > amount) {
				// если в поле за раз вложено больше, чем снимаемая слава
				(*tm_it)->glory -= amount;
				it->second->spend_glory -= amount;
				removed += amount;
				// если чар онлайн - снимаем с него статы
				remove_stat_online(it->first, (*tm_it)->stat, amount);
				break;
			} else {
				// если вложено меньше - снимаем со скольких нужно
				amount -= (*tm_it)->glory;
				it->second->spend_glory -= (*tm_it)->glory;
				removed += (*tm_it)->glory;
				// если чар онлайн - снимаем с него статы
				remove_stat_online(it->first, (*tm_it)->stat, (*tm_it)->glory);
				it->second->timers.erase(tm_it++);
			}
		} else
			break;
	}
	imm_log("(GC) %s sets -%d stats to %s.", GET_NAME(god), removed, GET_NAME(ch));
	SendMsgToChar(god,
				  "С %s снято %d %s вложенной ранее славы.\r\n",
				  GET_PAD(ch, 1),
				  removed,
				  GetDeclensionInNumber(removed, EWhat::kPoint));
	// надо пересчитать хп на случай снятия с тела
	check_max_hp(ch);
	save_glory();
	return true;
}

/**
* Трансфер 'вчистую', т.е. у принимаюего чара все обнуляем (кроме свободной славы) и сетим все,
* что было у передающего (свободная слава плюсуется).
*/
void transfer_stats(CharData *ch, CharData *god, const std::string& name, char *reason) {
	if (IS_IMMORTAL(ch)) {
		SendMsgToChar(god, "Трансфер славы с бессмертных на других персонажей запрещен.\r\n");
		return;
	}

	auto it = glory_list.find(ch->get_uid());
	if (it == glory_list.end()) {
		SendMsgToChar(god, "У %s нет славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	if (it->second->denial) {
		SendMsgToChar(god, "У %s активен таймер, запрещающий переброску вложенных очков славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	long vict_uid = GetUniqueByName(name);
	if (!vict_uid) {
		SendMsgToChar(god, "Некорректное имя персонажа (%s), принимающего славу.\r\n", name.c_str());
		return;
	}

	DescriptorData *d_vict = DescriptorByUid(vict_uid);
	CharData::shared_ptr vict;
	if (d_vict) {
		vict = d_vict->character;
	} else {
		// принимающий оффлайн
		CharData::shared_ptr t_vict(new Player); // TODO: переделать на стек
		if (LoadPlayerCharacter(name.c_str(), t_vict.get(), ELoadCharFlags::kFindId) < 0) {
			SendMsgToChar(god, "Некорректное имя персонажа (%s), принимающего славу.\r\n", name.c_str());
			return;
		}

		vict = t_vict;
	}

	// дальше у нас принимающий vict в любом случае
	if (IS_IMMORTAL(vict)) {
		SendMsgToChar(god, "Трансфер славы на бессмертного - это глупо.\r\n");
		return;
	}

	if (str_cmp(GET_EMAIL(ch), GET_EMAIL(vict))) {
		SendMsgToChar(god, "Персонажи имеют разные email адреса.\r\n");
		return;
	}

	// ищем запись принимающего, если ее нет - создаем
	auto vict_it = glory_list.find(vict_uid);
	if (vict_it == glory_list.end()) {
		GloryNodePtr temp_node(new GloryNode);
		glory_list[vict_uid] = temp_node;
		vict_it = glory_list.find(vict_uid);
	}
	// vict_it сейчас валидный итератор на принимающего
	int was_stats = vict_it->second->spend_glory;
	vict_it->second->copy_glory(it->second);
	vict_it->second->denial = DISPLACE_TIMER;

	snprintf(buf, kMaxStringLength,
			 "%s: перекинуто (%s -> %s) славы: %d, статов: %d",
			 GET_NAME(god), GET_NAME(ch), GET_NAME(vict), it->second->free_glory,
			 vict_it->second->spend_glory - was_stats);
	imm_log("%s", buf);
	mudlog(buf, DEF, kLvlImmortal, SYSLOG, true);
	AddKarma(ch, buf, reason);
	GloryMisc::add_log(GloryMisc::TRANSFER_GLORY, 0, buf, std::string(reason), vict.get());

	// если принимающий чар онлайн - сетим сразу ему статы
	if (d_vict) {
		// стартовые статы полюбому должны быть валидными, раз он уже в игре
		GloryMisc::recalculate_stats(vict.get());
		for (auto tm_it = vict_it->second->timers.begin(); tm_it != vict_it->second->timers.end(); ++tm_it) {
			if ((*tm_it)->timer > 0) {
				switch ((*tm_it)->stat) {
					case G_STR: vict->inc_str((*tm_it)->glory);
						break;
					case G_DEX: vict->inc_dex((*tm_it)->glory);
						break;
					case G_INT: vict->inc_int((*tm_it)->glory);
						break;
					case G_WIS: vict->inc_wis((*tm_it)->glory);
						break;
					case G_CON: vict->inc_con((*tm_it)->glory);
						break;
					case G_CHA: vict->inc_cha((*tm_it)->glory);
						break;
					default:
						log("Glory: некорректный номер стата %d (uid: %ld)",
							(*tm_it)->stat, it->first);
				}
			}
		}
	}
	AddKarma(vict.get(), buf, reason);
	vict->save_char();

	// удаляем запись чара, с которого перекидывали
	glory_list.erase(it);
	// и выставляем ему новые статы (он то полюбому уже загружен канеш,
	// но тут стройная картина через дескриптор везде) и если он был оффлайн - обнулится при входе
	DescriptorData *k = DescriptorByUid(ch->get_uid());
	if (k) {
		GloryMisc::recalculate_stats(k->character.get());
	}

	save_glory();
}

// * Показ свободной и вложенной славы у чара (glory имя).
void show_glory(CharData *ch, CharData *god) {
	auto it = glory_list.find(ch->get_uid());
	if (it == glory_list.end()) {
		SendMsgToChar(god, "У %s совсем не славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	SendMsgToChar(god, "Информация об очках славы %s:\r\n", GET_PAD(ch, 1));
	print_glory(god, it);
}

// * Вывод инфы в show stats.
void show_stats(CharData *ch) {
	int free_glory = 0, spend_glory = 0;
	for (auto & it : glory_list) {
		free_glory += it.second->free_glory;
		spend_glory += it.second->spend_glory * 1000;
	}
	SendMsgToChar(ch, "  Слава: вложено %d, свободно %d, всего %d\r\n",
				  spend_glory, free_glory, free_glory + spend_glory);
}

// * Вкдючение/выключение показа чара в топе славы (glory hide on|off).
void hide_char(CharData *vict, CharData *god, char const *const mode) {
	if (!mode || !*mode) {
		return;
	}
	bool ok = true;
	auto it = glory_list.find(vict->get_uid());
	if (it != glory_list.end()) {
		if (!str_cmp(mode, "on") || !str_cmp(mode, "off")) {
			it->second->hide = true;
		} else {
			ok = false;
		}
	}
	if (ok) {
		std::string text = it->second->hide ? "исключен из топа" : "присутствует в топе";
		SendMsgToChar(god, "%s теперь %s прославленных персонажей\r\n", GET_NAME(vict), text.c_str());
		save_glory();
	} else {
		SendMsgToChar(god, "Некорректный параметр %s, hide может быть только on или off.\r\n", mode);
	}
}

// * Остановка таймеров на вложенной славе.
void set_freeze(long uid) {
	auto it = glory_list.find(uid);
	if (it != glory_list.end())
		it->second->freeze = true;
	save_glory();
}

// * Включение таймеров на вложенной славе.
void remove_freeze(long uid) {
	auto it = glory_list.find(uid);
	if (it != glory_list.end())
		it->second->freeze = false;
	save_glory();
}

// * Во избежание разного рода недоразумений с фризом таймеров - проверяем эту тему при входе в игру.
void check_freeze(CharData *ch) {
	auto it = glory_list.find(ch->get_uid());
	if (it != glory_list.end())
		it->second->freeze = ch->IsFlagged(EPlrFlag::kFrozen) ? true : false;
}

void set_stats(CharData *ch) {
	auto it = glory_list.find(ch->get_uid());
	if (it == glory_list.end()) {
		return;
	}

	for (auto tm_it = it->second->timers.begin(); tm_it != it->second->timers.end(); ++tm_it) {
		if ((*tm_it)->timer > 0) {
			switch ((*tm_it)->stat) {
				case G_STR: ch->inc_str((*tm_it)->glory);
					break;
				case G_DEX: ch->inc_dex((*tm_it)->glory);
					break;
				case G_INT: ch->inc_int((*tm_it)->glory);
					break;
				case G_WIS: ch->inc_wis((*tm_it)->glory);
					break;
				case G_CON: ch->inc_con((*tm_it)->glory);
					break;
				case G_CHA: ch->inc_cha((*tm_it)->glory);
					break;
				default: log("Glory: некорректный номер стата %d (uid: %ld)", (*tm_it)->stat, it->first);
			}
		}
	}
}

int get_spend_glory(CharData *ch) {
	auto i = glory_list.find(ch->get_uid());
	if (i != glory_list.end()) {
		return i->second->spend_glory;
	}
	return 0;
}

} // namespace Glory

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
