// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "privilege.h"

#include "utils/logger.h"
#include "entities/char_data.h"
#include "boards/boards.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

/**
* Система привилегий иммов и демигодов, совмещенная с бывшим god.lst.
* Убрано редактирование из мада и запись уида автоматом -> все изменения производятся прямой правкой файла на сервере.
* Добавлены группы команд и флаги для более удобного распределения привилегий.
* Пример файла privilege.lst:
* # предустановленные группы (названия не менять): default, default_demigod, boards, arena, skills
* # default - команды, доступные всем иммам по умолчанию (демигоды не в счет)
* # default_demigod - команды всем демигодам по дефолту
* # boards - полный доступ на доски новостей, ошибок, кодеров и какие там еще будут
* # arena - команды, которые доступны только на арене, без set и show (+ спеллы переход и призвать)
* # skills - флаг, позволяющий имму пользоваться заклинаниями, умениями, рунами, взламывать двери (34е по дефолту)
* # fullzedit - возможность открытия/закрытия зон для записи (zed lock/unlock) (34е по дефолту)
* # title - одобрение/запрет чужих титулов
* # остальные группы вписываете сколько хотите
* # иммы: имя уид (0 не канает, теперь надо сразу писать уид) команды/группы
* <groups>
* default = wizhelp wiznet register имя титул title holylight uptime date set (title name) rules show nohassle ; show (punishment stats player)
* default_demigod = wizhelp wiznet имя rules
* arena = purge
* olc = oedit zedit redit olc trigedit
* goto = goto прыжок poofin poofout
* </groups>
* <gods>
* Йцук 595336650 groups (olc) hell mute dumb ban delete set (bank)
* Фыва 803863739 groups (arena goto olc boards) hell mute dumb ban delete set (bank)
* </gods>
* Формат файла временный, zone.ru там грозится своим форматом на lua, а xml в очередной раз решено не воротить, хотя и хотелось...
*/
namespace privilege {

const int kBoards = 0;
const int kUseSkills = 1;
const int kArenaMaster = 2;
const int kKroder = 3;
const int kFullzedit = 4;
const int kTitle = 5;
// чтение/удаление доски опечаток, ставился по наличию группы olc
const int kMisprint = 6;
// чтение/удаление доски придумок
const int kSuggest = 7;
// количество флагов
const int FLAGS_NUM = 8;

typedef std::set<std::string> PrivListType;

class GodListNode {
 public:
	std::string name; // имя
	PrivListType set; // доступные подкоманды set
	PrivListType show; // доступные подкоманды show
	PrivListType other; // доступные команды
	PrivListType arena; // доступные команды на арене
	std::bitset<FLAGS_NUM> flags; // флаги
	void clear() {
		name.clear();
		set.clear();
		show.clear();
		other.clear();
		arena.clear();
		flags.reset();
	}
};

const char *PRIVILEGE_FILE = LIB_MISC"privilege.lst";
typedef std::map<long, GodListNode> GodListType;
GodListType god_list; // основной список иммов и привилегий
std::map<std::string, std::string> group_list; // имя группы, строка команд (для парса при лоаде файла)
GodListNode tmp_god; // так удобнее
void parse_command_line(const std::string &command, int other_flags = 0); // прототип

/**
* Группы и флаги идут одним полем (причем флаг может быть и группой одновременно),
* поэтому флаги обрабатываем до парса по группам здесь.
* \param command - имя флага
*/
void parse_flags(const std::string &command) {
	if (command == "boards")
		tmp_god.flags.set(kBoards);
	else if (command == "skills")
		tmp_god.flags.set(kUseSkills);
	else if (command == "arena")
		tmp_god.flags.set(kArenaMaster);
	else if (command == "kroder")
		tmp_god.flags.set(kKroder);
	else if (command == "fullzedit")
		tmp_god.flags.set(kFullzedit);
	else if (command == "title")
		tmp_god.flags.set(kTitle);
	else if (command == "olc")
		tmp_god.flags.set(kMisprint);
	else if (command == "suggest")
		tmp_god.flags.set(kSuggest);
}

/**
* Рассовываем команды по группам (общие, set, show), из группы arena команды идут в отдельный список
* \param command - имя команды, fill_mode - в какой список добавлять, other_flags - для групп вроде арены со своим списком команд
*/
void insert_command(const std::string &command, int fill_mode, int other_flags) {
	if (other_flags == 1) {
		// в арену пишется только аналог общего списка, я не знаю зачем на арене set или show
		if (!fill_mode)
			tmp_god.arena.insert(command);
		return;
	}

	switch (fill_mode) {
		case 0: tmp_god.other.insert(command);
			break;
		case 1: tmp_god.set.insert(command);
			break;
		case 2: tmp_god.show.insert(command);
			break;
		case 3: {
			const auto it = group_list.find(command);
			if (it != group_list.end()) {
				if (command == "arena")
					parse_command_line(it->second, 1);
				else
					parse_command_line(it->second);
			}
			break;
		}
		default: break;
	}
}

// * Добавление иммам и демигодам списка команд по умолчанию из групп default и default_demigod.
void insert_default_command(long uid) {
	std::map<std::string, std::string>::const_iterator it;
	if (get_level_by_unique(uid) < kLvlImmortal)
		it = group_list.find("default_demigod");
	else
		it = group_list.find("default");
	if (it != group_list.end())
		parse_command_line(it->second);
}

/**
* Парс строки нагло дернут у Пелена, ибо креативить свой было влом
* \param other_flags - по дефолту 0 (добавление идет в основной список команд), 1 - добавление в список arena
* \param commands - строка со списком команд для парса
*/
void parse_command_line(const std::string &commands, int other_flags) {
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ", "()");
	tokenizer::iterator tok_iter, tmp_tok_iter;
	tokenizer tokens(commands, sep);

	int fill_mode = 0;
	tokens.assign(commands);
	if (tokens.begin() == tokens.end()) return;
	for (tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
		if ((*tok_iter) == "(") {
			if ((*tmp_tok_iter) == "set") {
				fill_mode = 1;
				continue;
			} else if ((*tmp_tok_iter) == "show") {
				fill_mode = 2;
				continue;
			} else if ((*tmp_tok_iter) == "groups") {
				fill_mode = 3;
				continue;
			}
		} else if ((*tok_iter) == ")") {
			fill_mode = 0;
			continue;
		}
		parse_flags(*tok_iter);
		insert_command(*tok_iter, fill_mode, other_flags);
		tmp_tok_iter = tok_iter;
	}
}

// * Лоад и релоад файла привилегий (reload privilege) с последующим проставлением блокнотов иммам.
void Load() {
	std::ifstream file(PRIVILEGE_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", PRIVILEGE_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	god_list.clear(); // для релоада

	std::string name, commands, temp;
	long uid;

	while (file >> name) {
		if (name == "#") {
			ReadEndString(file);
			continue;
		} else if (name == "<groups>") {

			while (file >> name) {
				if (name == "#") {
					ReadEndString(file);
					continue;
				}
				if (name == "</groups>")
					break;

				file >> temp; // "="
				std::getline(file, commands);
				boost::trim(commands);
				group_list[name] = commands;
			}
			continue;
		} else if (name == "<gods>") {
			while (file >> name) {
				if (name == "#") {
					ReadEndString(file);
					continue;
				}
				if (name == "</gods>")
					break;
				file >> uid;
				name_convert(name);
				tmp_god.name = name;
				std::getline(file, commands);
				boost::trim(commands);
				parse_command_line(commands);
				insert_default_command(uid);
				god_list[uid] = tmp_god;
				tmp_god.clear();
			}
		}
	}
	// генерим блокноты
	LoadGodBoards();
	group_list.clear();
}

/**
* Ищет имма в списке по уиду и полному совпадению имени. Вариант с CharacterData на входе убран за ненадобностью.
* Имм вне списка ниче из wiz команд не сможет, при сборке через make test или под студией поиск в этом списке не производится.
* Напоминание: в этом списке не только иммы, но и демигоды...
* \param name - имя имма, unique - его уид
* \return 0 - не нашли, 1 - нашли
*/

bool IsContainedInGodsList(const std::string &name, long unique) {
#ifdef TEST_BUILD
	return true;
#endif
	auto it = god_list.find(unique);
	if (it != god_list.end())
		if (it->second.name == name)
			return true;
	return false;
}

// * Создание и лоад/релоад блокнотов иммам.
void LoadGodBoards() {
	Boards::Static::clear_god_boards();
	for (auto & god : god_list) {
		int level = get_level_by_unique(god.first);
		if (level < kLvlImmortal) continue;
		Boards::Static::init_god_board(god.first, god.second.name);
	}
}

/**
* Проверка на возможность использования команды (для команд с левелом 31+). 34е используют без ограничений.
* При сборке через make test или под студией поиск по привилегиям не производится.
* \param mode 0 - общие команды, 1 - подкоманды set, 2 - подкоманды show
* \return 0 - нельзя, 1 - можно
*/
bool IsAbleToDoPrivilege(CharData *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level) {
	if (check_level && !mode && cmd_info[cmd_number].minimum_level < kLvlImmortal
		&& GetRealLevel(ch) >= cmd_info[cmd_number].minimum_level)
		return true;
	if (ch->IsNpc()) return false;
#ifdef TEST_BUILD
	if (IS_IMMORTAL(ch))
		return true;
#endif
	const auto it = god_list.find(GET_UNIQUE(ch));
	if (it != god_list.end() && CompareParam(it->second.name, GET_NAME(ch), true)) {
		if (GetRealLevel(ch) == kLvlImplementator)
			return true;
		switch (mode) {
			case 0:
				if (it->second.other.find(cmd_name) != it->second.other.end())
					return true;
				break;
			case 1:
				if (it->second.set.find(cmd_name) != it->second.set.end())
					return true;
				break;
			case 2:
				if (it->second.show.find(cmd_name) != it->second.show.end())
					return true;
				break;
			default: break;
		}
		// на арене доступны команды из группы arena_master
		if (!mode && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) && it->second.arena.find(cmd_name) != it->second.arena.end())
			return true;
	}
	return false;
}

/**
* Проверка флагов. 34м автоматически присваивается группа skills
* для более удобного вызова, например при использовании рун.
* \param flag - список флагов в начале файла, кол-во FLAGS_NUM
* \return 0 - не нашли, 1 - нашли
*/
bool CheckFlag(const CharData *ch, int flag) {
	if (flag >= FLAGS_NUM || flag < 0) return false;
	bool result = false;
	const auto it = god_list.find(GET_UNIQUE(ch));
	if (it != god_list.end() && CompareParam(it->second.name, GET_NAME(ch), true))
		if (it->second.flags[flag])
			result = true;
	if (flag == kUseSkills && (IS_IMPL(ch)))
		result = true;
	return result;
}

/**
* Проверка на возможность каста заклинания иммом.
* Группа skills без ограничений. Группа arena только призыв, пента и слово возврата и только на клетках арены.
* У морталов и 34х проверка не производится.
*/
bool IsSpellPermit(const CharData *ch, ESpell spell_id) {
	if (!IS_IMMORTAL(ch) || IS_IMPL(ch) || CheckFlag(ch, kUseSkills)) {
		return true;
	}
	if (spell_id == kSpellPortal || spell_id == kSpellSummon || spell_id == kSpellWorldOfRecall) {
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) && CheckFlag(ch, kArenaMaster)) {
			return true;
		}
	}
	return false;
}

/**
* Проверка на возможность использования скилла. Вызов через get_skill.
* У морталов, мобов и 34х проверка не производится.
* \return 0 - не может использовать скиллы, 1 - может
*/
bool CheckSkills(const CharData *ch) {
	if ((GetRealLevel(ch) > kLvlGod) || !IS_IMMORTAL(ch) || CheckFlag(ch, kUseSkills))
//	if (!IS_IMMORTAL(ch) || IS_IMPL(ch) || check_flag(ch, USE_SKILLS))
		return true;
	return false;
}

} // namespace Privilege

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
