// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "depot.h"

#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/core/handler.h"
#include "gameplay/economics/auction.h"
#include "engine/ui/color.h"
#include "engine/entities/char_player.h"
#include "engine/ui/modify.h"
#include "gameplay/clans/house.h"
#include "engine/core/utils_char_obj.inl"
#include "named_stuff.h"
#include "stable_objs.h"
#include "weather.h"
#include "gameplay/mechanics/dungeons.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <cmath>

extern int bank(CharData *, void *, int, char *);
extern void olc_update_object(int robj_num, ObjData *obj, ObjData *olc_proto);
namespace Depot {

// максимальное кол-во шмоток в персональном хранилище (волхвам * 2)
inline unsigned int MAX_PERS_SLOTS(CharData *ch) {return GetRealLevel(ch) + GetRealRemort(ch) * 3;};
//const unsigned int MAX_PERS_INGR_SLOTS = 50;

// * Для оффлайнового списка шмоток в хранилище.
class OfflineNode {
 public:
	OfflineNode() : vnum(0), timer(0), rent_cost(0), uid(0) {};
	int vnum; // внум
	int timer; // таймер
	int rent_cost; // цена ренты в день
	unsigned int uid; // глобальный уид
};

typedef std::list<ObjData::shared_ptr> ObjListType; // имя, шмотка
typedef std::list<OfflineNode> TimerListType; // список шмота, находящегося в оффлайне

class CharNode {
 public:
	CharNode() : ch(0), money(0), money_spend(0), buffer_cost(0), need_save(0), cost_per_day(0) {};

// онлайн
	// шмотки в персональном хранилище онлайн
	ObjListType pers_online;
	// чар (онлайн бабло и флаг наличия онлайна)
	CharData *ch;
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

	void take_item(CharData *vict, char *arg, int howmany);
	void remove_item(ObjListType::iterator &obj_it, ObjListType &cont, CharData *vict);
	bool obj_from_obj_list(char *name, CharData *vict);
	void load_online_objs(int file_type, bool reload = 0);
	void online_to_offline(ObjListType &cont);

	int get_cost_per_day() const {
		return cost_per_day;
	}
	void reset_cost_per_day() {
		cost_per_day = 0;
	}
	void add_cost_per_day(ObjData *obj);
	void add_cost_per_day(int amount);
};

typedef std::map<long, CharNode> DepotListType; // уид чара, инфа
DepotListType depot_list; // список личных хранилищ

// * Капитально расширенная версия сислога для хранилищ.
void depot_log(const char *format, ...) {
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
typedef std::map<long/*уид игрока*/, time_t /*время первой записи в файле*/ > PurgedListType;
PurgedListType purged_list;

bool need_save_purged_list = false;

void save_purged_list() {
	if (!need_save_purged_list) return;

	const char *filename = LIB_DEPOT"purge.db";
	std::ofstream file(filename);
	if (!file.is_open()) {
		log("Хранилище: error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}
	for (const auto &[uid, time] : purged_list)
		file << uid << " " << time << "\n";

	need_save_purged_list = false;
}

void load_purge_list() {
	const char *filename = LIB_DEPOT"purge.db";
	std::ifstream file(filename);
	if (!file.is_open()) {
		log("Хранилище: error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}

	long uid;
	time_t date;

	while (file >> uid >> date)
		purged_list[uid] = date;
}

std::string generate_purged_filename(long uid) {
	std::string name = GetNameByUnique(uid);
	if (name.empty())
		return "";

	char filename[kMaxStringLength];
	if (!get_filename(name.c_str(), filename, kPurgeDepotFile))
		return "";

	return filename;
}

std::string generate_purged_text(long uid, int obj_vnum, unsigned int obj_uid) {
	std::ostringstream out;
	out << "Ошибка при удалении предмета: vnum " << obj_vnum << ", uid " << obj_uid << "\r\n";

	std::string name = GetNameByUnique(uid);
	if (name.empty())
		return out.str();

	Player t_ch;
	Player *ch = &t_ch;
	if (LoadPlayerCharacter(name.c_str(), ch, ELoadCharFlags::kFindId) < 0)
		return out.str();

	char filename[kMaxStringLength];
	if (!get_filename(name.c_str(), filename, kPersDepotFile)) {
		log("Хранилище: не удалось сгенерировать имя файла (name: %s, filename: %s) (%s %s %d).",
			name.c_str(), filename, __FILE__, __func__, __LINE__);
		return out.str();
	}

	FILE *fl = fopen(filename, "r+b");
	if (!fl) return out.str();

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
	if (!fsize) {
		fclose(fl);
		log("Хранилище: пустой файл предметов (%s).", filename);
		return out.str();
	}

	const std::shared_ptr<char> databuf(new char[fsize + 1], std::default_delete<char[]>());

	fseek(fl, 0L, SEEK_SET);
	if (!databuf
		|| !fread(databuf.get(), fsize, 1, fl)
		|| ferror(fl)) {
		fclose(fl);
		log("Хранилище: ошибка чтения файла предметов (%s).", filename);
		return out.str();
	}
	fclose(fl);

	char *data = databuf.get();
	data[fsize] = '\0';
	int error = 0;

	for (fsize = 0; *data && *data != '$'; fsize++) {
		const auto obj = read_one_object_new(&data, &error);
		if (!obj) {
			if (error) {
				log("Хранилище: ошибка чтения предмета (%s, error: %d).", filename, error);
			}
			continue;
		}

		if (obj->get_unique_id() == obj_uid
			&& obj->get_vnum() == obj_vnum) {
			std::ostringstream text;
			text << "[Персональное хранилище]: " << kColorBoldRed << "'"
				 << obj->get_short_description() << char_get_custom_label(obj.get(), ch)
				 << " рассыпал" << GET_OBJ_SUF_2(obj.get())
				 << " в прах'" << kColorNrm << "\r\n";
			ExtractObjFromWorld(obj.get());
			return text.str();
		}
		ExtractObjFromWorld(obj.get());
	}

	return out.str();
}

void add_purged_message(long uid, int obj_vnum, unsigned int obj_uid) {
	std::string name = generate_purged_filename(uid);
	if (name.empty()) {
		log("Хранилище: add_purge_message пустое имя файла %ld.", uid);
		return;
	}

	if (purged_list.find(uid) == purged_list.end()) {
		purged_list[uid] = time(0);
		need_save_purged_list = true;
	}

	std::ofstream file(name.c_str(), std::ios_base::app);
	if (!file.is_open()) {
		log("Хранилище: error open file: %s! (%s %s %d)", name.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	file << generate_purged_text(uid, obj_vnum, obj_uid);
}

void delete_purged_entry(long uid) {
	auto it = purged_list.find(uid);
	if (it != purged_list.end()) {
		std::string name = generate_purged_filename(uid);
		if (name.empty()) {
			log("Хранилище: delete_purge_entry пустое имя файла %ld.", uid);
			return;
		}
		remove(name.c_str());
		purged_list.erase(it);
		need_save_purged_list = true;
	}
}

bool show_purged_message(CharData *ch) {
	auto it = purged_list.find(ch->get_uid());
	if (it != purged_list.end()) {
		// имя у нас канеш и так есть, но че зря код дублировать
		std::string name = generate_purged_filename(ch->get_uid());
		if (name.empty()) {
			log("Хранилище: show_purged_message пустое имя файла %ld.", ch->get_uid());
			return true;
		}
		std::ifstream file(name.c_str(), std::ios::binary);
		if (!file.is_open()) {
			log("Хранилище: error open file: %s! (%s %s %d)", name.c_str(), __FILE__, __func__, __LINE__);
			return true;
		}
		std::ostringstream out;
		out << "\r\n" << file.rdbuf();
		SendMsgToChar(out.str(), ch);
		remove(name.c_str());
		purged_list.erase(it);
		need_save_purged_list = true;
		return true;
	}

	return false;
}

void clear_old_purged_entry() {
	time_t today = time(0);
	for (auto it = purged_list.begin(); it != purged_list.end(); /* пусто */) {
		time_t diff = today - it->second;
		// месяц в секундах
		if (diff >= 60 * 60 * 24 * 31) {
			std::string name = generate_purged_filename(it->first);
			if (!name.empty())
				remove(name.c_str());
			purged_list.erase(it++);
			need_save_purged_list = true;
		} else
			++it;
	}
}

void init_purged_list() {
	load_purge_list();
	clear_old_purged_entry();
	need_save_purged_list = true;
	save_purged_list();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// * Удаление файла персонального хранилища (шмотки отдельного чара).
void remove_pers_file(const std::string &name) {
	if (name.empty()) return;

	char filename[kMaxStringLength];
	if (get_filename(name.c_str(), filename, kPersDepotFile))
		remove(filename);
}

/**
* Удаление шмоток и файлов шмоток, если они были.
* Делается попытся снятия с чара долга за ренту.
*/
void remove_char_entry(long uid, CharNode &node) {
	depot_log("remove_char_entry: %ld", uid);
	// если чар был что-то должен, надо попытаться с него это снять
	if (!node.name.empty() && (node.money_spend || node.buffer_cost)) {
		Player t_victim;
		Player *victim = &t_victim;
		if (LoadPlayerCharacter(node.name.c_str(), victim, ELoadCharFlags::kFindId) > -1 && victim->get_uid() == uid) {
			int total_pay = node.money_spend + static_cast<int>(node.buffer_cost);
			victim->remove_both_gold(total_pay);
			victim->save_char();
		}
	}
	node.reset();
	remove_pers_file(node.name);
}

// * Инициализация рнумов сундуков. Лоад файла с оффлайн информацией по предметам.
void init_depot() {
	depot_log("init_depot start");

	init_purged_list();

	const char *depot_file = LIB_DEPOT"depot.db";
	std::ifstream file(depot_file);
	if (!file.is_open()) {
		log("Хранилище: error open file: %s! (%s %s %d)", depot_file, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	while (file >> buffer) {
		if (buffer != "<Node>") {
			log("Хранилище: ошибка чтения <Node>");
			break;
		}
		CharNode tmp_node;
		long uid;
		// общие поля
		if (!(file >> uid >> tmp_node.money >> tmp_node.money_spend >> tmp_node.buffer_cost)) {
			log("Хранилище: ошибка чтения uid(%ld), money(%ld), money_spend(%ld), buffer_cost(%f).",
				uid, tmp_node.money, tmp_node.money_spend, tmp_node.buffer_cost);
			break;
		}
		// чтение предметов
		file >> buffer;
		if (buffer != "<Objects>") {
			log("Хранилище: ошибка чтения <Objects>.");
			break;
		}
		while (file >> buffer) {
			if (buffer == "</Objects>") break;

			OfflineNode tmp_obj;
			try {
				tmp_obj.vnum = std::stoi(buffer, nullptr, 10);
			}
			catch (const std::invalid_argument &) {
				log("Хранилище: ошибка чтения vnum (%s)", buffer.c_str());
				break;
			}
			if (!(file >> tmp_obj.timer >> tmp_obj.rent_cost >> tmp_obj.uid)) {
				log("Хранилище: ошибка чтения timer(%d) rent_cost(%d) uid(%d) (obj vnum: %d).",
					tmp_obj.timer, tmp_obj.rent_cost, tmp_obj.uid, tmp_obj.vnum);
				break;
			}
			// проверяем существование прототипа предмета и суем его в маск в мире на постое
			int rnum = GetObjRnum(tmp_obj.vnum);
			if (rnum >= 0) {
				obj_proto.inc_stored(rnum);
				tmp_node.add_cost_per_day(tmp_obj.rent_cost);
				tmp_node.offline_list.push_back(tmp_obj);
			}
		}
		if (buffer != "</Objects>") {
			log("Хранилище: ошибка чтения </Objects>.");
			break;
		}
		// проверка корректной дочитанности
		file >> buffer;
		if (buffer != "</Node>") {
			log("Хранилище: ошибка чтения </Node>.");
			break;
		}

		// Удаление записи из списка хранилищ делается только здесь и при пурже на нехватке денег,
		// причем нужно не забывать, что выше при лоаде шмоток они уже попали в список макс в мире
		// и удаление записи не через remove_char_entry - это будет касяк

		// проверяем есть ли еще такой чар вообще
		tmp_node.name = GetNameByUnique(uid);
		if (tmp_node.name.empty()) {
			log("Хранилище: UID %ld - персонажа не существует.", uid);
			remove_char_entry(uid, tmp_node);
			continue;
		}
		// пустые хранилища удаляются только здесь
		if (tmp_node.offline_list.empty() && tmp_node.pers_online.empty()) {
			log("Хранилище: UID %ld - пустое хранилище.", uid);
			remove_char_entry(uid, tmp_node);
			continue;
		}

		name_convert(tmp_node.name);
		depot_list[uid] = tmp_node;
	}

	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		depot_log("char: %ld", it->first);
		for (TimerListType::iterator obj_it = it->second.offline_list.begin(); obj_it != it->second.offline_list.end();
			 ++obj_it)
			depot_log("%d %d %d %d", obj_it->vnum, obj_it->uid, obj_it->timer, obj_it->rent_cost);
	}
	depot_log("init_depot end");
}

// * Загрузка самих хранилищ в банки делается после инита хранилищ и резета зон, потому как мобов надо.
void load_chests() {
	for (const auto &ch : character_list) {
		const auto rnum = ch->get_rnum();
		if (rnum > 0
			&& rnum <= top_of_mobt
			&& mob_index[rnum].func == bank) {
			const auto pers_chest = world_objects.create_from_prototype_by_rnum(system_obj::PERS_CHEST_RNUM);
			if (!pers_chest) {
				return;
			}
			if (world[ch->in_room]->vnum % 100 == 99) {
				PlaceObjToRoom(pers_chest.get(), GET_LASTROOM(ch));
			} else {
				PlaceObjToRoom(pers_chest.get(), ch->in_room);
			}
		}
	}
}

// * Добавление цены ренты с порверкой на валидность и переполнение.
void CharNode::add_cost_per_day(int amount) {
	if (amount < 0 || amount > 50000) {
		log("Хранилище: невалидная стоимость ренты %i", amount);
		return;
	}
	int over = std::numeric_limits<int>::max() - cost_per_day;
	if (amount > over)
		cost_per_day = std::numeric_limits<int>::max();
	else
		cost_per_day += amount;
}

// * Версия add_cost_per_day(int amount) для принятия *ObjectData.
void CharNode::add_cost_per_day(ObjData *obj) {
	add_cost_per_day(get_object_low_rent(obj));
}

// * Для удобства в save_timedata(), запись строки с параметрами шмотки.
void write_objlist_timedata(const ObjListType &cont, std::stringstream &out) {
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it) {
		out << GET_OBJ_VNUM(obj_it->get()) << " " << (*obj_it)->get_timer() << " "
			<< get_object_low_rent(obj_it->get()) << " " << (*obj_it)->get_unique_id() << "\n";
	}
}

// * Сохранение всех списков в общий файл с тайм-инфой шмоток (в том числе таймеров из онлайн списка).
void save_timedata() {
	// на случай падения мада во время прохода по списку
	std::stringstream out;

	for (DepotListType::const_iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		out << "<Node>\n" << it->first << " ";
		// чар онлайн - пишем бабло с его счета, иначе берем из оффлайновых полей инфу
		if (it->second.ch)
			out << it->second.ch->get_total_gold() << " 0 ";
		else
			out << it->second.money << " " << it->second.money_spend << " ";
		out << it->second.buffer_cost << "\n";

		// собсна какие списки не пустые - те и пишем, особо не мудрствуя
		out << "<Objects>\n";
		write_objlist_timedata(it->second.pers_online, out);
		for (TimerListType::const_iterator obj_it = it->second.offline_list.begin();
			 obj_it != it->second.offline_list.end(); ++obj_it) {
			out << obj_it->vnum << " " << obj_it->timer << " " << obj_it->rent_cost
				<< " " << obj_it->uid << "\n";
		}
		out << "</Objects>\n</Node>\n";
	}

	// канеш тут может файл не открыться и мы зря все считали, но это клиника
	const char *depot_file = LIB_DEPOT"depot.db";
	std::ofstream file(depot_file);
	if (!file.is_open()) {
		log("Хранилище: error open file: %s! (%s %s %d)", depot_file, __FILE__, __func__, __LINE__);
		return;
	}
	file << out.rdbuf();
	file.close();
}

// * Запись файла со шмотом (полная версия записи).
void write_obj_file(const std::string &name, int file_type, const ObjListType &cont) {
	// генерим имя файла
	depot_log("write_obj_file: %s", name.c_str());
	char filename[kMaxStringLength];
	if (!get_filename(name.c_str(), filename, file_type)) {
		log("Хранилище: не удалось сгенерировать имя файла (name: %s, filename: %s) (%s %s %d).",
			name.c_str(), filename, __FILE__, __func__, __LINE__);
		return;
	}

	// при пустом списке просто удаляем файл, чтобы не плодить пустышек в дире
	if (cont.empty()) {
		depot_log("empty cont");
		remove(filename);
		return;
	}

	// пишем в буфер для верности на время цикла
	std::stringstream out;
	out << "* Items file\n";
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it) {
		depot_log("save: %s %d %d",
				  (*obj_it)->get_short_description().c_str(),
				  (*obj_it)->get_unique_id(),
				  GET_OBJ_VNUM(obj_it->get()));
		write_one_object(out, obj_it->get(), 0);
	}
	out << "\n$\n$\n";

	// скидываем в файл
	std::ofstream file(filename);
	if (!file.is_open()) {
		log("Хранилище: error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}
	file << out.rdbuf();
	file.close();
}

// * Сохранение персонального хранилища конкретного плеера в файл. Обнуление флага сейва.
void CharNode::save_online_objs() {
	if (need_save) {
		log("Save obj: %s depot", ch->get_name().c_str());
		ObjSaveSync::check(ch->get_uid(), ObjSaveSync::PERS_CHEST_SAVE);

		write_obj_file(name, kPersDepotFile, pers_online);
		need_save = false;
	}
}

// * Сохранение всех ждущих этого онлайновых списков предметов в файлы.
void save_all_online_objs() {
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
		it->second.save_online_objs();
}

// * Сохранение файла хранилища из системы синхронизации.
void save_char_by_uid(int uid) {
	DepotListType::iterator it = depot_list.find(uid);
	if (it != depot_list.end()) {
		it->second.save_online_objs();
	}
}

// * Апдейт таймеров в онлайн списках с оповещением о пурже, если чар онлайн и расчетом общей ренты.
void CharNode::update_online_item() {
	for (ObjListType::iterator obj_it = pers_online.begin(); obj_it != pers_online.end();) {
		if ((*obj_it)->get_timer() == 0) {
			if (ch) {
				// если чар в лд или еще чего - лучше записать и выдать это ему при след
				// входе в игру, чтобы уж точно увидел
				if (ch->desc && ch->desc->state == EConState::kPlaying) {
					SendMsgToChar(ch, "[Персональное хранилище]: %s'%s%s рассыпал%s в прах'%s\r\n",
								  kColorBoldRed, (*obj_it)->get_short_description().c_str(),
								  char_get_custom_label(obj_it->get(), ch).c_str(),
								  GET_OBJ_SUF_2((*obj_it)), kColorNrm);
				} else {
					add_purged_message(ch->get_uid(), GET_OBJ_VNUM(obj_it->get()), (*obj_it)->get_unique_id());
				}
			}

			// вычитать ренту из cost_per_day здесь не надо, потому что она уже обнулена
			depot_log("zero timer, online extract: %s %d %d",
					  (*obj_it)->get_short_description().c_str(),
					  (*obj_it)->get_unique_id(),
					  GET_OBJ_VNUM(obj_it->get()));
			pers_online.erase(obj_it++);
			need_save = true;
		} else {
			++obj_it;
		}
	}
	save_purged_list();
}

int delete_obj(int vnum) {
	int num = 0;
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end();
			 ++obj_it) {
			if (GET_OBJ_VNUM(obj_it->get()) == vnum) {
				(*obj_it)->set_timer(0);
				num++;
			}
		}
	}
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {

		for (TimerListType::iterator k = it->second.offline_list.begin(),
				 kend = it->second.offline_list.end(); k != kend; ++k) {
			if (vnum == k->vnum) {
				k->timer = 0;
				num++;
			}
		}
	}
	return num;
}

/**
* Снятие ренты за период в оффлайне с удалением шмота при нехватке денег.
* \return true - нужно удалять запись в списки хранилищ
*/
bool CharNode::removal_period_cost() {
	double i;
	buffer_cost += static_cast<double>(cost_per_day) / kSecsPerMudDay;
	modf(buffer_cost, &i);
	if (i >= 1.0f) {
		unsigned diff = static_cast<unsigned>(i);
		money -= diff;
		money_spend += diff;
		buffer_cost -= i;
	}
	return (money < 0) ? true : false;
}

// * Апдейт таймеров на всех шмотках, удаление при нулевых таймерах (с оповещением).
void update_timers() {
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end();) {
		class CharNode &node = it->second;
		// обнуляем ренту и пересчитываем ее по ходу обновления оффлайн таймеров
		node.reset_cost_per_day();
		// онлайновый/оффлайновый списки
		if (node.ch)
			node.update_online_item();
		else
			node.update_offline_item(it->first);
		// снятие денег и пурж шмота, если денег уже не хватает
		if (node.get_cost_per_day() && node.removal_period_cost()) {
			depot_log("update_timers: no money %ld %s", it->first, node.name.c_str());
			log("Хранилище: UID %ld (%s) - запись удалена из-за нехватки денег на ренту.",
				it->first, node.name.c_str());
			remove_char_entry(it->first, it->second);
			depot_list.erase(it++);
		} else
			++it;
	}
}

// * Апдейт таймеров в оффлайн списках с расчетом общей ренты.
void CharNode::update_offline_item(long uid) {
	for (TimerListType::iterator obj_it = offline_list.begin(); obj_it != offline_list.end();) {
		int rnum = GetObjRnum(obj_it->vnum);
		if (rnum < 0) {
			depot_log("Что-то не так с объектом : %d", obj_it->vnum);
			continue;
		}
		const auto obj = obj_proto[rnum];
		if (!stable_objs::IsTimerUnlimited(obj.get())) {
			--(obj_it->timer);
		}
		if (obj_it->timer <= 0) {
			depot_log("update_offline_item %s: zero timer %d %d", name.c_str(), obj_it->vnum, obj_it->uid);
			add_purged_message(uid, obj_it->vnum, obj_it->uid);
			// шмотка уходит в лоад			
			if (rnum >= 0) {
				obj_proto.dec_stored(rnum);
			}
			// вычитать ренту из cost_per_day здесь не надо, потому что она уже обнулена
			offline_list.erase(obj_it++);
		} else {
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
void CharNode::reset() {
	for (ObjListType::iterator obj_it = pers_online.begin(); obj_it != pers_online.end(); ++obj_it) {
		depot_log("reset %s: online extract %s %d %d",
				  name.c_str(),
				  (*obj_it)->get_short_description().c_str(),
				  (*obj_it)->get_unique_id(),
				  GET_OBJ_VNUM(obj_it->get()));
		ExtractObjFromWorld(obj_it->get());
	}
	pers_online.clear();

	// тут нужно ручками перекинуть удаляемый шмот в лоад и потом уже грохнуть все разом
	for (TimerListType::iterator obj = offline_list.begin(); obj != offline_list.end(); ++obj) {
		depot_log("reset_%s: offline erase %d %d", name.c_str(), obj->vnum, obj->uid);
		int rnum = GetObjRnum(obj->vnum);
		if (rnum >= 0) {
			obj_proto.dec_stored(rnum);
		}
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
bool is_depot(ObjData *obj) {
	if (system_obj::PERS_CHEST_RNUM < 0
		|| obj->get_rnum() != system_obj::PERS_CHEST_RNUM) {
		return false;
	} else {
		return true;
	}
}

// * Распечатка отдельного предмета при осмотре хранилища.
void print_obj(std::stringstream &i_out, std::stringstream &s_out,
			   ObjData *obj, int count, CharData *ch) {
	const bool output_to_i = obj->get_type() == EObjType::kMagicIngredient
		|| obj->get_type() == EObjType::kCraftMaterial;
	std::stringstream &out = output_to_i ? i_out : s_out;

	out << obj->get_short_description();
	out << char_get_custom_label(obj, ch);
	if (count > 1) {
		out << " [" << count << "]";
	}
	out << " [" << get_object_low_rent(obj) << " "
		<< GetDeclensionInNumber(get_object_low_rent(obj), EWhat::kMoneyA) << "]\r\n";
}

// * Расчет кол-ва слотов под шмотки в персональном хранилище с учетом профы чара.
unsigned int get_max_pers_slots(CharData *ch) {
	if (CanUseFeat(ch, EFeat::kThrifty))
		return MAX_PERS_SLOTS(ch) * 2;
	else
		return MAX_PERS_SLOTS(ch);
}

/**
* Для читаемости show_depot - вывод списка предметов персонажу.
* Сортировка по алиасам и группировка одинаковых предметов.
*/
std::string print_obj_list(CharData *ch, ObjListType &cont) {
	cont.sort([](ObjData::shared_ptr lrs, ObjData::shared_ptr rhs) {
		return lrs->get_aliases() < rhs->get_aliases();
	});

	// чтобы сначала вывести шмотки, а потом ингры
	std::stringstream s_out, i_out;
	int rent_per_day = 0;
	int count = 0, s_cnt = 0, i_cnt = 0;
	bool found = 0;

	auto prev_obj_it = cont.cend();
	for (auto obj_it = cont.cbegin(); obj_it != cont.cend(); ++obj_it) {
		if ((*obj_it)->get_type() == EObjType::kMagicIngredient
			|| (*obj_it)->get_type() == EObjType::kCraftMaterial) {
			++i_cnt;
		} else {
			++s_cnt;
		}

		if (prev_obj_it == cont.cend()) {
			prev_obj_it = obj_it;
			count = 1;
		} else if (!IsObjsStackable(obj_it->get(), prev_obj_it->get())) {
			print_obj(i_out, s_out, prev_obj_it->get(), count, ch);
			prev_obj_it = obj_it;
			count = 1;
		} else {
			count++;
		}
		rent_per_day += get_object_low_rent(obj_it->get());
		found = true;
	}
	if (prev_obj_it != cont.cend() && count) {
		print_obj(i_out, s_out, prev_obj_it->get(), count, ch);
	}

	const long money = ch->get_gold() + ch->get_bank();
	const long expired = rent_per_day ? (money / rent_per_day) : 0;

	std::stringstream head;
	head << kColorWht
		 << "Ваше персональное хранилище. Рента в день: "
		 << rent_per_day << " " << GetDeclensionInNumber(rent_per_day, EWhat::kMoneyA);
	if (rent_per_day) {
		head << ", денег хватит на " << expired
			 << " " << GetDeclensionInNumber(expired, EWhat::kDay);
	}
	head << ".\r\n"
		 << "Заполненность отделения для вещей: "
		 << s_cnt << "/" << get_max_pers_slots(ch)
		 << ", отделения для ингредиентов: "
		 << i_cnt << "/" << MAX_PERS_SLOTS(ch) * 2
		 << kColorNrm << "\r\n\r\n";

	if (!found) {
		head << "В данный момент хранилище абсолютно пусто.\r\n";
	}
	std::string s_str = s_out.str();
	std::string i_str = i_out.str();
	if (!s_str.empty() && !i_str.empty()) {
		s_str += "-----------------------------------\r\n";
	}

	return (head.str() + s_str + i_str);
}

/**
* Поиск итератора хранилища или создание нового, чтобы не париться.
* Возврат итератора за списком означает, что чара в плеер-таблице мы не нашли (что странно).
* \param ch - проставляет в зависимости от наличия чара онлайн, иначе будет нулевым.
*/
DepotListType::iterator create_depot(long uid, CharData *ch = 0) {
	DepotListType::iterator it = depot_list.find(uid);
	if (it == depot_list.end()) {
		CharNode tmp_node;
		// в случае отсутствия чара (пока только при релоаде) ch останется нулевым,
		// а имя возьмется через уид
		tmp_node.ch = ch;
		tmp_node.name = GetNameByUnique(uid);
		if (!tmp_node.name.empty()) {
			depot_list[uid] = tmp_node;
			it = depot_list.find(uid);
		}
	}
	return it;
}

// * Выводим персональное хранилище вместо просмотра контейнера.
void show_depot(CharData *ch) {
	if (ch->IsNpc()) return;

#ifndef TEST_BUILD
	if (IS_IMMORTAL(ch) && !IS_IMPL(ch)) {
		SendMsgToChar("И без хранилища обойдешься...\r\n", ch);
		return;
	}
#endif

	if (NORENTABLE(ch)) {
		SendMsgToChar(ch, "%sХранилище недоступно в связи с боевыми действиями.%s\r\n",
					  kColorBoldRed, kColorNrm);
		return;
	}

	DepotListType::iterator it = create_depot(ch->get_uid(), ch);
	if (it == depot_list.end()) {
		SendMsgToChar("Ошибочка, сообщие богам...\r\n", ch);
		log("Хранилище: UID %ld, name: %s - возвращен некорректный уид персонажа.", ch->get_uid(), GET_NAME(ch));
		return;
	}

	std::string out;
	out = print_obj_list(ch, it->second.pers_online);
	page_string(ch->desc, out);
}

/**
* В случае переполнения денег на счете - кладем сколько можем, остальное возвращаем чару на руки.
* На руках при возврате переполняться уже некуда, т.к. вложение идет с этих самых рук.
*/
void put_gold_chest(CharData *ch, const ObjData::shared_ptr &obj) {
	if (obj->get_type() != EObjType::kMoney) {
		return;
	}
	long gold = GET_OBJ_VAL(obj, 0);
	ch->add_bank(gold);
	RemoveObjFromChar(obj.get());
	ExtractObjFromWorld(obj.get());
	SendMsgToChar(ch, "Вы вложили %ld %s.\r\n", gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));
}

/**
* Проверка возможности положить шмотку в хранилище.
* FIXME с кланами копипаст.
*/
bool can_put_chest(CharData *ch, ObjData *obj) {
	// depot_log("can_put_chest: %s, %s", GET_NAME(ch), obj->get_PName(ECase::kNom));
	if (obj->is_unrentable()
		|| obj->has_flag(EObjFlag::kDecay)
		|| NamedStuff::check_named(ch, obj, 0)) {
		SendMsgToChar(ch, "Неведомая сила помешала положить %s в хранилище.\r\n", obj->get_PName(ECase::kAcc).c_str());
		return 0;
	} else if (obj->get_type() == EObjType::kContainer
		&& obj->get_contains()) {
		SendMsgToChar(ch, "В %s что-то лежит.\r\n", obj->get_PName(ECase::kPre).c_str());
		return 0;
	} else if (SetSystem::is_norent_set(ch, obj)) {
		snprintf(buf, kMaxStringLength, "%s - требуется две и более вещи из набора.\r\n", obj->get_PName(ECase::kNom).c_str());
		SendMsgToChar(utils::CAP(buf), ch);
		return 0;
	}
	return 1;
}

unsigned count_inrg(const ObjListType &cont) {
	unsigned ingr_cnt = 0;
	for (auto obj_it = cont.cbegin(); obj_it != cont.cend(); ++obj_it) {
		if ((*obj_it)->get_type() == EObjType::kMagicIngredient
			|| (*obj_it)->get_type() == EObjType::kCraftMaterial) {
			++ingr_cnt;
		}
	}
	return ingr_cnt;
}

// * Кладем шмотку в хранилище (мобов посылаем лесом), деньги автоматом на счет в банке.
bool put_depot(CharData *ch, ObjData::shared_ptr &obj) {
	if (ch->IsNpc()) return 0;

#ifndef TEST_BUILD
	if (IS_IMMORTAL(ch) && !IS_IMPL(ch)) {
		SendMsgToChar("И без хранилища обойдешься...\r\n", ch);
		return 0;
	}
#endif

	if (NORENTABLE(ch)) {
		SendMsgToChar(ch,
					  "%sХранилище недоступно в связи с боевыми действиями.%s\r\n",
					  kColorBoldRed, kColorNrm);
		return 0;
	}

	if (obj->get_type() == EObjType::kMoney) {
		put_gold_chest(ch, obj);
		return 1;
	}
	// в данном случае если мы не можем сунуть какую-то
	// конкретную шмотку - это не стопит процесс
	if (!can_put_chest(ch, obj.get())) {
		return 1;
	}

	DepotListType::iterator it = create_depot(ch->get_uid(), ch);
	if (it == depot_list.end()) {
		SendMsgToChar("Ошибочка, сообщие богам...\r\n", ch);
		log("Хранилище: UID %ld, name: %s - возвращен некорректный уид персонажа.", ch->get_uid(), GET_NAME(ch));
		return 0;
	}

	const size_t ingr_cnt = count_inrg(it->second.pers_online);
	const size_t staff_cnt = it->second.pers_online.size() - ingr_cnt;
	const bool is_ingr = obj->get_type() == EObjType::kMagicIngredient
		|| obj->get_type() == EObjType::kCraftMaterial;

	if (is_ingr && ingr_cnt >= (MAX_PERS_SLOTS(ch) * 2)) {
		SendMsgToChar("В вашем хранилище совсем не осталось места для ингредиентов :(.\r\n", ch);
		return 0;
	} else if (!is_ingr && staff_cnt >= get_max_pers_slots(ch)) {
		SendMsgToChar("В вашем хранилище совсем не осталось места для вещей :(.\r\n", ch);
		return 0;
	}

	if (!ch->get_bank() && !ch->get_gold()) {
		SendMsgToChar(ch, "У вас ведь совсем нет денег, чем вы собираетесь расплачиваться за хранение вещей?\r\n");
		return 0;
	}
	obj = world_objects.get_by_raw_ptr(dungeons::SwapOriginalObject(obj.get()));
	depot_log("put_depot %s %ld: %s %d %d",
			  GET_NAME(ch), ch->get_uid(), obj->get_short_description().c_str(),
			  obj->get_unique_id(), GET_OBJ_VNUM(obj.get()));
	it->second.pers_online.push_front(obj);
	it->second.need_save = true;

	act("Вы положили $o3 в персональное хранилище.", false, ch, obj.get(), 0, kToChar);
	act("$n положил$g $o3 в персональное хранилище.", true, ch, obj.get(), 0, kToRoom);
	RemoveObjFromChar(obj.get());
	check_auction(nullptr, obj.get());
	ObjSaveSync::add(ch->get_uid(), ch->get_uid(), ObjSaveSync::PERS_CHEST_SAVE);

	return 1;
}

// * Взятие чего-то из персонального хранилища.
void take_depot(CharData *vict, char *arg, int howmany) {
	if (vict->IsNpc()) return;

#ifndef TEST_BUILD
	if (IS_IMMORTAL(vict) && !IS_IMPL(vict)) {
		SendMsgToChar("И без хранилища обойдешься...\r\n", vict);
		return;
	}
#endif

	if (NORENTABLE(vict)) {
		SendMsgToChar(vict, "%sХранилище недоступно в связи с боевыми действиями.%s\r\n",
					  kColorBoldRed, kColorNrm);
		return;
	}

	DepotListType::iterator it = depot_list.find(vict->get_uid());
	if (it == depot_list.end()) {
		SendMsgToChar("В данный момент ваше хранилище абсолютно пусто.\r\n", vict);
		return;
	}

	it->second.take_item(vict, arg, howmany);
}

// * Берем шмотку из хранилища.
void CharNode::remove_item(ObjListType::iterator &obj_it, ObjListType &cont, CharData *vict) {
	depot_log("remove_item %s: %s %d %d",
			  name.c_str(),
			  (*obj_it)->get_short_description().c_str(),
			  (*obj_it)->get_unique_id(),
			  GET_OBJ_VNUM(obj_it->get()));
	PlaceObjToInventory(obj_it->get(), vict);
	act("Вы взяли $o3 из персонального хранилища.", false, vict, obj_it->get(), 0, kToChar);
	act("$n взял$g $o3 из персонального хранилища.", true, vict, obj_it->get(), 0, kToRoom);
	cont.erase(obj_it++);
	need_save = true;
	ObjSaveSync::add(ch->get_uid(), ch->get_uid(), ObjSaveSync::PERS_CHEST_SAVE);
}

// * Поиск шмотки в контейнере (со всякими точками), удаляем ее тут же.
bool CharNode::obj_from_obj_list(char *name, CharData *vict) {
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;
	strcpy(tmp, name);

	ObjListType &cont = pers_online;

	int j = 0, number;
	if (!(number = get_number(&tmp))) {
		return false;
	}

	for (ObjListType::iterator obj_it = cont.begin(); obj_it != cont.end() && (j <= number); ++obj_it) {
		if ((isname(tmp, (*obj_it)->get_aliases()) || CHECK_CUSTOM_LABEL(tmp, obj_it->get(), vict)) && ++j == number) {
			remove_item(obj_it, cont, vict);
			return true;
		}
	}
	return false;
}

// * Поиск шмотки в хранилище и ее взятие.
void CharNode::take_item(CharData *vict, char *arg, int howmany) {
	ObjListType &cont = pers_online;

	int obj_dotmode = find_all_dots(arg);
	if (obj_dotmode == kFindIndiv) {
		bool result = obj_from_obj_list(arg, vict);
		if (!result) {
			SendMsgToChar(vict, "Вы не видите '%s' в хранилище.\r\n", arg);
			return;
		}
		while (result && --howmany) {
			result = obj_from_obj_list(arg, vict);
		}
	} else {
		if (obj_dotmode == kFindAlldot && !*arg) {
			SendMsgToChar("Взять что \"все\"?\r\n", vict);
			return;
		}
		bool found = 0;
		for (ObjListType::iterator obj_list_it = cont.begin(); obj_list_it != cont.end();) {
			if (obj_dotmode == kFindAll
				|| isname(arg, (*obj_list_it)->get_aliases())
				|| CHECK_CUSTOM_LABEL(arg, obj_list_it->get(), vict)) {
				// чтобы нельзя было разом собрать со шкафчика неск.тыс шмоток
				if (!CanTakeObj(vict, obj_list_it->get())) {
					return;
				}
				found = 1;
				remove_item(obj_list_it, cont, vict);
			} else {
				++obj_list_it;
			}
		}

		if (!found) {
			SendMsgToChar(vict, "Вы не видите ничего похожего на '%s' в хранилище.\r\n", arg);
			return;
		}
	}
}

/**
* Рента за шмот в день, но с учетом персонального хранилища онлайн, для вывода перед рентой.
* Эта функция зовется ДО того, как поизройдет перекидывание шиота в оффлайн список.
*/
int get_total_cost_per_day(CharData *ch) {
	DepotListType::iterator it = depot_list.find(ch->get_uid());
	if (it != depot_list.end()) {
		int cost = 0;
		if (!it->second.pers_online.empty())
			for (ObjListType::const_iterator obj_it = it->second.pers_online.begin();
				 obj_it != it->second.pers_online.end(); ++obj_it) {
				cost += get_object_low_rent(obj_it->get());
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
void show_stats(CharData *ch) {
	std::stringstream out;
	size_t pers_count = 0;
	size_t offline_count = 0;
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		pers_count += it->second.pers_online.size();
		offline_count += it->second.offline_list.size();
	}

	out << "  Хранилищ: " << depot_list.size() << ", "
		<< "в персональных: " << pers_count << ", "
		<< "в оффлайне: " << offline_count << "\r\n";
	SendMsgToChar(out.str().c_str(), ch);
}

/**
* Лоад списка шмоток в онлайн список. FIXME: копи-паст чтения предметов с двух мест.
* \param reload - для рестора шмоток чара, по умолчанию ноль
* (определяет сверяемся мы с оффлайн списком при лоаде или нет).
*/
void CharNode::load_online_objs(int file_type, bool reload) {
	depot_log("load_online_objs %s start", name.c_str());
	if (!reload && offline_list.empty()) {
		depot_log("empty offline_list and !reload");
		return;
	}

	char filename[kMaxStringLength];
	if (!get_filename(name.c_str(), filename, file_type)) {
		log("Хранилище: не удалось сгенерировать имя файла (name: %s, filename: %s) (%s %s %d).",
			name.c_str(), filename, __FILE__, __func__, __LINE__);
		return;
	}

	FILE *fl = fopen(filename, "r+b");
	if (!fl) return;

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
	if (!fsize) {
		fclose(fl);
		log("Хранилище: пустой файл предметов (%s).", filename);
		return;
	}
	char *databuf = new char[fsize + 1];

	fseek(fl, 0L, SEEK_SET);
	if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf) {
		fclose(fl);
		delete[] databuf;
		log("Хранилище: ошибка чтения файла предметов (%s).", filename);
		return;
	}
	fclose(fl);

	depot_log("file ok");

	char *data = databuf;
	*(data + fsize) = '\0';
	int error = 0;

	ObjData::shared_ptr obj;
	for (fsize = 0; *data && *data != '$'; fsize++) {
		obj = read_one_object_new(&data, &error);
		if (!obj) {
			depot_log("reading object error %d", error);
			if (error)
				log("Хранилище: ошибка чтения предмета (%s, error: %d).", filename, error);
			continue;
		}
		if (!reload) {
			// собсна сверимся со списком таймеров и проставим изменившиеся данные
			TimerListType::iterator obj_it = std::find_if(offline_list.begin(), offline_list.end(),
														  [&](const OfflineNode &x) {
															  return x.uid == obj->get_unique_id();
														  });

			if (obj_it != offline_list.end() && obj_it->vnum == obj->get_vnum()) {
				depot_log("load object %s %d %d",
						  obj->get_short_description().c_str(),
						  obj->get_unique_id(),
						  obj->get_vnum());
				obj->set_timer(obj_it->timer);
				int temp_timer = obj_proto[obj->get_rnum()]->get_timer();
				if (obj->get_timer() > temp_timer)
					obj->set_timer(temp_timer);
				// надо уменьшать макс в мире на постое, макс в мире шмотки в игре
				// увеличивается в read_one_object_new через read_object
				int rnum = GetObjRnum(obj->get_vnum());
				if (rnum >= 0) {
					obj_proto.dec_stored(rnum);
				}
			} else {
				depot_log("extract object %s %d %d",
						  obj->get_short_description().c_str(),
						  obj->get_unique_id(),
						  obj->get_vnum());
				ExtractObjFromWorld(obj.get());
				continue;
			}
		}
		// при релоаде мы ничего не сверяем, а лоадим все, что есть,
		// макс в мире и так увеличивается при чтении шмотки, а на постое ее и не было

		pers_online.push_front(obj);
	}
	delete[] databuf;
	offline_list.clear();
	reset_cost_per_day();
}

// * Вход чара в игру - снятие за хранилище, пурж шмоток, оповещение о кол-ве снятых денег, перевод списков в онлайн.
void enter_char(CharData *ch) {
	DepotListType::iterator it = depot_list.find(ch->get_uid());
	if (it != depot_list.end()) {
		// снимаем бабло, если что-то было потрачено на ренту
		if (it->second.money_spend > 0) {
			SendMsgToChar(ch, "%sХранилище: за время вашего отсутствия удержано %ld %s.%s\r\n\r\n",
						  kColorWht, it->second.money_spend,
						  GetDeclensionInNumber(it->second.money_spend, EWhat::kMoneyA), kColorNrm);

			long rest = ch->remove_both_gold(it->second.money_spend);
			if (rest > 0) {
				// есть вариант, что денег не хватит, потому что помимо хранилищ еще капает за
				// одежду и инвентарь, а учитывать еще и их при расчетах уже как-то мутно
				// поэтому мы просто готовы, если что, все технично спуржить при входе
				depot_log("no money %s %ld: reset depot", GET_NAME(ch), ch->get_uid());
				it->second.reset();
				// файл убьется позже при ребуте на пустом хране,
				// даже если не будет никаких перезаписей по ходу игры
				SendMsgToChar(ch, "%sХранилище: у вас не хватило денег на постой.%s\r\n\r\n",
							  kColorWht, kColorNrm);
			}
		}
		// грузим хранилище, сохранять его тут вроде как смысла нет
		it->second.load_online_objs(kPersDepotFile);
		// обнуление оффлайновых полей и проставление ch для снятия бабла онлайн
		it->second.money = 0;
		it->second.money_spend = 0;
		it->second.offline_list.clear();
		it->second.ch = ch;
	}
}

// * Перевод онлайн списка в оффлайн.
void CharNode::online_to_offline(ObjListType &cont) {
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it) {
		depot_log("online_to_offline %s: %s %d %d",
				  name.c_str(),
				  (*obj_it)->get_short_description().c_str(),
				  (*obj_it)->get_unique_id(),
				  (*obj_it)->get_vnum());
		OfflineNode tmp_obj;
		tmp_obj.vnum = (*obj_it)->get_vnum();
		tmp_obj.timer = (*obj_it)->get_timer();
		tmp_obj.rent_cost = get_object_low_rent(obj_it->get());
		tmp_obj.uid = (*obj_it)->get_unique_id();
		offline_list.push_back(tmp_obj);
		ExtractObjFromWorld(obj_it->get());
		// плюсуем персональное хранилище к общей ренте
		add_cost_per_day(tmp_obj.rent_cost);
		// из макс.в мире в игре она уходит в ренту
		int rnum = GetObjRnum(tmp_obj.vnum);
		if (rnum >= 0) {
			obj_proto.inc_stored(rnum);
		}
	}
	cont.clear();
}

/**
* Выход чара из игры - перевод хранилищ в оффлайн (уход в ренту/дт/смерть и последующий выход через меню,
* форс-рента при лд, 'конец'). TODO: когда буду втыкать так же ренту чара - надо учитывать, что пока мы
* не вышли из меню (при ренте/смерти) - в данный момент мы считаем этого чара все еще онлайн.
* TODO: так же мы сейчас дергаем эту функцию для пустых чаров, у которых уид заведомо нулевой.
*/
void exit_char(CharData *ch) {
	DepotListType::iterator it = depot_list.find(ch->get_uid());
	if (it != depot_list.end()) {
		// тут лучше дернуть сейв руками до тика, т.к. есть вариант зайти и тут же выйти
		// в итоге к моменту сейва онлайн список будет пустой и все похерится
		it->second.save_online_objs();

		it->second.online_to_offline(it->second.pers_online);
		it->second.ch = 0;
		it->second.money = ch->get_bank() + ch->get_gold();
		it->second.money_spend = 0;
	}
}

// * Релоад шмоток чара на случай отката.
void reload_char(long uid, CharData *ch) {
	DepotListType::iterator it = create_depot(uid);
	if (it == depot_list.end()) {
		log("Хранилище: UID %ld - возвращен некорректный уид персонажа.", uid);
		return;
	}
	// сначала обнуляем все, что надо
	it->second.reset();

	// после чего надо записать ему бабла, иначе при входе все спуржится (бабло проставит в exit_char)
	CharData::shared_ptr vict;
	DescriptorData *d = DescriptorByUid(uid);
	if (d) {
		vict = d->character; // чар онлайн
	} else {
		// чар соответственно оффлайн
		const CharData::shared_ptr t_vict(new Player);
		if (LoadPlayerCharacter(it->second.name.c_str(), t_vict.get(), ELoadCharFlags::kFindId) < 0) {
			// вообще эт нереальная ситуация после проверки в DoReboot
			SendMsgToChar(ch, "Некорректное имя персонажа (%s).\r\n", it->second.name.c_str());
		}
		vict = t_vict;
	}

	if (vict) {
		// вобщем тут мысль такая: кодить доп. обработку для релоада оффлайн чара мне стало лень,
		// поэтому мы штатно грузим ему сначала онлайн список, после чего отправляем его в оффлайн
		it->second.load_online_objs(kPersDepotFile, 1);
		// если чара грузили с оффлайна
		if (!d) {
			exit_char(vict.get());
		}
	}

	snprintf(buf, kMaxStringLength, "Depot: %s reload items for %s.", GET_NAME(ch), it->second.name.c_str());
	mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
	imm_log("%s", buf);
}

/**
* Учет локейт в персональных хранилищах.
* \param count - оставшееся кол-во возможных показываемых шмоток после прохода по основному обж-списку.
*/
std::string PrintSpellLocateObject(CharData *ch, ObjData *obj) {
	for (auto it : depot_list) {
		for (auto obj_it : it.second.pers_online) {
			if (!IS_GOD(ch)) {
				if (number(1, 100) > (40 + std::max((GetRealInt(ch) - 25) * 2, 0))) {
					continue;
				}
				if (obj_it->has_flag(EObjFlag::kNolocate)
					&& !IS_GOD(ch)) {
					continue;
				}
			}
			if (obj->get_id() == obj_it->get_id()) {
				return fmt::format("{} наход{}ся у кого-то в персональном хранилище.\r\n", obj_it->get_short_description().c_str(), GET_OBJ_POLY_1(ch, obj_it));
			}
		}
	}
	return {};
}

std::string print_imm_where_obj(const ObjData *obj) {
	std::string str;
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end();
			 ++obj_it) {
			if (obj->get_id() ==  (*obj_it)->get_id()) {
				str = fmt::format("наход{}ся в персональном хранилище ({}).\r\n",
						GET_OBJ_POLY_1(ch, (*obj_it)),
						it->second.name.c_str());
				return str;
			}
		}
	}
	return str;
}

ObjData *find_obj_from_depot_and_dec_number(char *arg, int &number) {
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end();
			 ++obj_it) {
			if (isname(arg, (*obj_it)->get_aliases())) {
				if (--number == 0) {
					return (*obj_it).get();
				}
			}
		}
	}
	return nullptr;
}

// * Обновление полей объектов при изменении их прототипа через олц.
void olc_update_from_proto(int robj_num, ObjData *olc_proto) {
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it) {
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end();
			 ++obj_it) {
			if ((*obj_it)->get_rnum() == robj_num) {
				olc_update_object(robj_num, obj_it->get(), olc_proto);
			}
		}
	}
}

// * Обработка ренейма персонажа.
void rename_char(CharData *ch) {
	DepotListType::iterator it = depot_list.find(ch->get_uid());
	if (it != depot_list.end()) {
		it->second.name = GET_NAME(ch);
	}
}

// * Поиск цели для каста локейта.
ObjData *locate_object(const char *str) {
	for (DepotListType::const_iterator i = depot_list.begin(); i != depot_list.end(); ++i) {
		for (ObjListType::const_iterator k = i->second.pers_online.begin(); k != i->second.pers_online.end(); ++k) {
			if (isname(str, (*k)->get_aliases())) {
				return k->get();
			}
		}
	}
	return 0;
}

// * Добавление денег чару, находящемуся оффлайн при переводе кун (типа временное решение).
void add_offline_money(long uid, int money) {
	DepotListType::iterator it = depot_list.find(uid);
	if (it != depot_list.end()) {
		it->second.money += money;
	}
}

// * Поиск в хранилище внума из списка vnum_list.
bool find_set_item(CharData *ch, const std::set<int> &vnum_list) {
	DepotListType::iterator it = depot_list.find(ch->get_uid());
	if (it != depot_list.end()) {
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(),
				 obj_it_end = it->second.pers_online.end(); obj_it != obj_it_end; ++obj_it) {
			if (vnum_list.find(obj_sets::normalize_vnum((*obj_it)->get_vnum())) != vnum_list.end()) {
				return true;
			}
		}
	}
	return false;
}

// * Запрет на ренту при наличии единственной сетины в хранилище с сообщением от рент-кипера.
int report_unrentables(CharData *ch, CharData *recep) {
	DepotListType::iterator it = depot_list.find(ch->get_uid());
	if (it != depot_list.end()) {
		for (ObjListType::iterator obj_it = it->second.pers_online.begin(),
				 obj_it_end = it->second.pers_online.end(); obj_it != obj_it_end; ++obj_it) {
			if (SetSystem::is_norent_set(ch, obj_it->get())) {
				snprintf(buf, kMaxStringLength,
						 "$n сказал$g вам : \"Я не приму на постой %s - требуется две и более вещи из набора.\"",
						 OBJN(obj_it->get(), ch, ECase::kAcc));
				act(buf, false, recep, 0, ch, kToVict);
				return 1;
			}
		}
	}
	return 0;
}

// * Проверка при ребуте на сныканые одинокие фулл-сетины.
void check_rented(int uid) {
	DepotListType::iterator it = depot_list.find(uid);
	if (it != depot_list.end()) {
		for (TimerListType::iterator k = it->second.offline_list.begin(),
				 kend = it->second.offline_list.end(); k != kend; ++k) {
			SetSystem::check_item(k->vnum);
		}
	}
}

/**
 * Обнуление таймера указанной шмотки в оффлайн хранилище (при ребуте).
 * Макс.в.мире учтется при последующем обновлении таймеров хранилища.
 */
void delete_set_item(int uid, int vnum) {
	DepotListType::iterator i = depot_list.find(uid);
	if (i != depot_list.end()) {
		TimerListType::iterator k = std::find_if(
			i->second.offline_list.begin(),
			i->second.offline_list.end(),
			[&](const OfflineNode &x) {
				return x.vnum == vnum;
			});

		if (k != i->second.offline_list.end()) {
			log("[TO] Player %s depot : set-item %d deleted",
				i->second.name.c_str(), k->vnum);
			k->timer = -1;
		}
	}
}

} // namespace Depot

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
