// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "privilege.h"

#include "utils/logger.h"
#include "engine/entities/char_data.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/db/player_index.h"
#include "privilege_db.h"

#include <cstdio>

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

// ===================== modern membership privilege system (issue.privilege-rework P2) =====================
// When !kLegacyPrivilege, decisions come from the cfg/privilege.xml membership DB (privilege_db.h), keyed
// by name+uid; character level grants nothing. "default", "default_demigod" and "arena" stay hardcoded
// here (point 7): default/default_demigod are auto-applied by tier; arena commands work only on arena tiles.
namespace {

const char *kDefaultCommands =
	"wizhelp|гбогам|wiznet|register|имя|титул|title|holylight|uptime|date|invis|rules|nohassle|show (punishment stats)";
const char *kDefaultDemigodCommands =
	"wizhelp|гбогам|wiznet|имя|титул|title|rules|date|uptime|сдемигодам|set (палач)";
const char *kArenaCommands = "purge|restore|arenarestore|goto|teleport";

struct CmdSet { std::set<std::string> cmds, set_subs, show_subs; };
CmdSet ParseSet(const std::string &raw) { CmdSet s; ParseCommandList(raw, s.cmds, s.set_subs, s.show_subs); return s; }
const CmdSet &DefaultSet() { static const CmdSet s = ParseSet(kDefaultCommands); return s; }
const CmdSet &DefaultDemigodSet() { static const CmdSet s = ParseSet(kDefaultDemigodCommands); return s; }
const CmdSet &ArenaSet() { static const CmdSet s = ParseSet(kArenaCommands); return s; }

int TierRank(EGodTier tier) {
	switch (tier) {
		case EGodTier::kOwner: return 0;
		case EGodTier::kImplementator: return 1;
		case EGodTier::kGreatGod: return 2;
		case EGodTier::kGod: return 3;
		case EGodTier::kImmortal: return 4;
		case EGodTier::kDemigod: return 5;
		default: return 99;
	}
}

bool OwnerByName(const CharData *ch) {
	return ch && !ch->IsNpc() && CompareParam(std::string("Стрибог"), GET_NAME(ch), true);
}

const GodEntry *FindGod(const CharData *ch) {
	if (!ch || ch->IsNpc()) return nullptr;
	const auto *e = GetDb().FindByUid(ch->get_uid());
	if (e && CompareParam(e->name, GET_NAME(ch), true)) return e;
	return nullptr;
}

bool AtLeastTier(const CharData *ch, EGodTier min) {
#ifdef TEST_BUILD
	int need = (min == EGodTier::kImplementator) ? kLvlImplementator
			 : (min == EGodTier::kGreatGod) ? kLvlGreatGod
			 : (min == EGodTier::kGod) ? kLvlGod : kLvlImmortal;
	return ch && !ch->IsNpc() && ch->GetLevel() >= need;
#else
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	return e && TierRank(e->tier) <= TierRank(min);
#endif
}

CmdSet EffectiveSet(const GodEntry *e) {
	CmdSet s; s.cmds = e->commands; s.set_subs = e->set_subs; s.show_subs = e->show_subs;
	for (const auto &gid : e->groups) {
		const auto &gm = GetDb().groups();
		auto it = gm.find(gid);
		if (it != gm.end()) ParseCommandList(it->second, s.cmds, s.set_subs, s.show_subs);
	}
	const CmdSet *def = (e->tier == EGodTier::kDemigod) ? &DefaultDemigodSet()
					 : (TierRank(e->tier) <= TierRank(EGodTier::kImmortal)) ? &DefaultSet() : nullptr;
	if (def) {
		s.cmds.insert(def->cmds.begin(), def->cmds.end());
		s.set_subs.insert(def->set_subs.begin(), def->set_subs.end());
		s.show_subs.insert(def->show_subs.begin(), def->show_subs.end());
	}
	return s;
}

std::string FlagToken(int flag) {
	switch (flag) {
		case kBoards: return "boards";
		case kUseSkills: return "skills";
		case kArenaMaster: return "arena";
		case kKroder: return "kroder";
		case kFullzedit: return "fullzedit";
		case kTitle: return "title";
		case kMisprint: return "misprint";
		case kSuggest: return "suggest";
		default: return "";
	}
}

bool ModernIsOwner(const CharData *ch) { return OwnerByName(ch); }

bool ModernCanEditVedun(const CharData *ch, const std::string &what) {
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	if (!e) return false;
	if (e->flags.count("FullAccess")) return true;
	if (e->vedun.count("*") || e->vedun.count("all")) return true;  // wildcard = any data set
	return e->vedun.count(what) > 0;
}

bool ModernHasPrivilege(CharData *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level) {
	if (check_level && !mode && cmd_info[cmd_number].minimum_level < kLvlImmortal
		&& GetRealLevel(ch) >= cmd_info[cmd_number].minimum_level) {
		return true;
	}
	if (ch->IsNpc()) return false;
#ifdef TEST_BUILD
	return true;
#endif
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	if (!e) {
		if (cmd_info[cmd_number].minimum_level >= kLvlImmortal && GetRealLevel(ch) >= kLvlImmortal) {
			char log_buf[256];
			snprintf(log_buf, sizeof(log_buf),
				"PRIVILEGE: %s (level %d) tried privileged command '%s' but is not in privilege.xml.",
				GET_NAME(ch), GetRealLevel(ch), cmd_name.c_str());
			mudlog(log_buf, DEF, kLvlGod, SYSLOG, true);
		}
		return false;
	}
	if (e->flags.count("FullAccess")) return true;
	const CmdSet eff = EffectiveSet(e);
	switch (mode) {
		case 1: return eff.set_subs.count(cmd_name) > 0;
		case 2: return eff.show_subs.count(cmd_name) > 0;
		default:
			if (eff.cmds.count(cmd_name)) return true;
			if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) && ArenaSet().cmds.count(cmd_name)) return true;
			return false;
	}
}

bool ModernCheckFlag(const CharData *ch, int flag) {
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	if (!e) return false;
	if (e->flags.count("FullAccess")) return true;
	const std::string tok = FlagToken(flag);
	if (tok.empty()) return false;
	if (e->flags.count(tok)) return true;
	if (tok == "boards" && e->tier == EGodTier::kDemigod) return true;  // default_demigod grants boards
	return false;
}

bool ModernIsContainedInGodsList(const std::string &name, long unique) {
#ifdef TEST_BUILD
	return true;
#endif
	if (CompareParam(std::string("Стрибог"), name, true)) return true;
	const auto *e = GetDb().FindByUid(unique);
	return e && CompareParam(e->name, name, true);
}

void ModernLoadGodBoards() {
	Boards::Static::clear_god_boards();
	for (const auto &pair : GetDb().entries()) {
		if (TierRank(pair.second.tier) <= TierRank(EGodTier::kImmortal))
			Boards::Static::init_god_board(pair.first, pair.second.name);
	}
}

}  // namespace

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
	if (GetLevelByUnique(uid) < kLvlImmortal)
		it = group_list.find("default_demigod");
	else
		it = group_list.find("default");
	if (it != group_list.end())
		parse_command_line(it->second);
}


std::vector<std::string> tokenize(const std::string& str) {
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string token;

	while (iss >> token) {
		std::string new_token;

		for (char c : token) {
			if (c == '(' || c == ')') {
				if (!new_token.empty()) {
					tokens.push_back(new_token);
					new_token.clear();
				}
				tokens.push_back(std::string(1, c));
			} else {
				new_token += c;
			}
		}
		if (!new_token.empty()) {
			tokens.push_back(new_token);
		}
	}
	return tokens;
}
/**
* Парс строки нагло дернут у Пелена, ибо креативить свой было влом
* \param other_flags - по дефолту 0 (добавление идет в основной список команд), 1 - добавление в список arena
* \param commands - строка со списком команд для парса
*/
void parse_command_line(const std::string &commands, int other_flags) {
	std::vector<std::string>::iterator tok_iter, tmp_tok_iter;
	std::stringstream ss;
	int fill_mode = 0;
	auto tokens = tokenize(commands);

/*	for (auto it : tokens) {
		ss << it << "|";
	}
	ss << "\r\n";
	mudlog(ss.str(), CMP, kLvlImmortal, SYSLOG, true);
*/
	if (tokens.begin() == tokens.end()) 
		return;
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
	if constexpr (!kLegacyPrivilege) return;  // modern DB is loaded via CfgManager (PrivilegeLoader)
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
				utils::Trim(commands);
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
				utils::Trim(commands);
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
	if constexpr (!kLegacyPrivilege) return ModernIsContainedInGodsList(name, unique);
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
	if constexpr (!kLegacyPrivilege) { ModernLoadGodBoards(); return; }
	Boards::Static::clear_god_boards();
	for (auto & god : god_list) {
		int level = GetLevelByUnique(god.first);
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
bool HasPrivilege(CharData *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level) {
	if constexpr (!kLegacyPrivilege) return ModernHasPrivilege(ch, cmd_name, cmd_number, mode, check_level);
	if (check_level && !mode && cmd_info[cmd_number].minimum_level < kLvlImmortal
		&& GetRealLevel(ch) >= cmd_info[cmd_number].minimum_level) {
		return true;
	}
	if (ch->IsNpc()) {
		return false;
	}
#ifdef TEST_BUILD
	if (privilege::IsImmortal(ch))
		return true;
#endif
	const auto it = god_list.find(ch->get_uid());
	if (it != god_list.end() && CompareParam(it->second.name, ch->get_name(), true)) {
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
	if constexpr (!kLegacyPrivilege) return ModernCheckFlag(ch, flag);
	if (flag >= FLAGS_NUM || flag < 0) return false;
	bool result = false;
	const auto it = god_list.find(ch->get_uid());
	if (it != god_list.end() && CompareParam(it->second.name, GET_NAME(ch), true))
		if (it->second.flags[flag])
			result = true;
	if (flag == kUseSkills && (privilege::IsImpl(ch)))
		result = true;
	return result;
}

/**
* Проверка на возможность каста заклинания иммом.
* Группа skills без ограничений. Группа arena только призыв, пента и слово возврата и только на клетках арены.
* У морталов и 34х проверка не производится.
*/
bool IsSpellPermit(const CharData *ch, ESpell spell_id) {
	if (!privilege::IsImmortal(ch) || privilege::IsImpl(ch) || CheckFlag(ch, kUseSkills)) {
		return true;
	}
	if (spell_id == ESpell::kPortal || spell_id == ESpell::kSummon || spell_id == ESpell::kWorldOfRecall) {
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
	if (privilege::IsGrGod(ch) || !privilege::IsImmortal(ch) || CheckFlag(ch, kUseSkills))
//	if (!privilege::IsImmortal(ch) || privilege::IsImpl(ch) || check_flag(ch, USE_SKILLS))
		return true;
	return false;
}

bool IsImmortal(const CharData *ch) { if constexpr (!kLegacyPrivilege) return AtLeastTier(ch, EGodTier::kImmortal); return !ch->IsNpc() && ch->GetLevel() >= kLvlImmortal; }
bool IsGod(const CharData *ch) { if constexpr (!kLegacyPrivilege) return AtLeastTier(ch, EGodTier::kGod); return !ch->IsNpc() && ch->GetLevel() >= kLvlGod; }
bool IsGrGod(const CharData *ch) { if constexpr (!kLegacyPrivilege) return AtLeastTier(ch, EGodTier::kGreatGod); return !ch->IsNpc() && ch->GetLevel() >= kLvlGreatGod; }
bool IsImpl(const CharData *ch) { if constexpr (!kLegacyPrivilege) return AtLeastTier(ch, EGodTier::kImplementator); return !ch->IsNpc() && ch->GetLevel() >= kLvlImplementator; }

bool IsOwner(const CharData *ch) { return ModernIsOwner(ch); }

bool CanEditVedun(const CharData *ch, const std::string &what) {
	if constexpr (kLegacyPrivilege) return true;  // no per-file Vedun restriction in legacy mode
	return ModernCanEditVedun(ch, what);
}

} // namespace Privilege

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
