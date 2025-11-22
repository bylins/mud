/**
\file player_index.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 22.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "player_index.h"

#include "administration/accounts.h"
#include "global_objects.h"
#include "utils/backtrace.h"
#include "engine/entities/char_player.h"
#include "gameplay/statistics/top.h"

PlayersIndex &player_table = GlobalObjects::player_table();

bool MustBeDeleted(CharData *short_ch);

PlayerIndexElement::PlayerIndexElement(const char *name) {
	set_name(name);
}

void PlayerIndexElement::set_name(const char *name) {
	delete[] m_name;
	char *new_name = new char[strlen(name) + 1];
	for (int i = 0; (new_name[i] = LOWER(name[i])); i++);
	m_name = new_name;
}

// ===============================================================================

const std::size_t PlayersIndex::NOT_FOUND = ~static_cast<std::size_t>(0);

PlayersIndex::~PlayersIndex() {
	log("~PlayersIndex()");
}

std::size_t PlayersIndex::Append(const PlayerIndexElement &element) {
	const auto index = size();

	push_back(element);
	m_id_to_index.emplace(element.uid(), index);
	AddNameToIndex(element.name(), index);

	return index;
}

std::size_t PlayersIndex::GetIndexByName(const char *name) const {
	const auto i = m_name_to_index.find(name);
	if (i != m_name_to_index.end()) {
		return i->second;
	}

	return NOT_FOUND;
}

void PlayersIndex::SetName(const std::size_t index, const char *name) {
	const auto i = m_name_to_index.find(operator[](index).name());
	m_name_to_index.erase(i);
	operator[](index).set_name(name);
	AddNameToIndex(name, index);
}

void PlayersIndex::AddNameToIndex(const char *name, const std::size_t index) {
	if (m_name_to_index.find(name) != m_name_to_index.end()) {
		log("SYSERR: Detected attempt to create player with duplicate name.");
		abort();
	}

	m_name_to_index.emplace(name, index);
}

std::size_t PlayersIndex::hasher::operator()(const std::string &value) const {
	// FNV-1a implementation
	static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");

	const std::size_t FNV_offset_basis = 14695981039346656037ULL;
	const std::size_t FNV_prime = 1099511628211ULL;

	const auto count = value.size();
	std::size_t result = FNV_offset_basis;
	for (std::size_t i = 0; i < count; ++i) {
		result ^= (std::size_t) LOWER(value[i]);
		result *= FNV_prime;
	}

	return result;
}

bool PlayersIndex::equal_to::operator()(const std::string &left, const std::string &right) const {
	if (left.size() != right.size()) {
		return false;
	}

	for (std::size_t i = 0; i < left.size(); ++i) {
		if (LOWER(left[i]) != LOWER(right[i])) {
			return false;
		}
	}

	return true;
}

/// ==================== TOOLS ================================

bool IsPlayerExists(const long id) { return player_table.IsPlayerExists(id); }

long CmpPtableByName(char *name, int len) {
	len = std::min(len, static_cast<int>(strlen(name)));
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): I am not sure but I guess that linear search is not the best solution here.
	 * TODO: make map helper (MAPHELPER). */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		const char *pname = player_table[i].name();
		if (!strn_cmp(pname, arg, std::min(len, static_cast<int>(strlen(pname))))) {
			return static_cast<long>(i);
		}
	}
	return -1;
}

long GetPlayerTablePosByName(const char *name) {
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		const char *pname = player_table[i].name();
		if (!str_cmp(pname, arg)) {
			return static_cast<long>(i);
		}
	}
	std::stringstream buffer;
	buffer << "Char " << name << " (" << arg << ") not found !!! Сброшен стек в сислог";
	debug::backtrace(runtime_config.logs(SYSLOG).handle());
//	sprintf(buf, "Char %s(%s) not found !!!", name, arg);
	mudlog(buffer.str().c_str(), CMP, kLvlImmortal, SYSLOG, false);
	return (-1);
}

long GetPtableByUnique(long unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].uid() == unique) {
			return static_cast<long>(i);
		}
	}
	return 0;
}

long GetPlayerIdByName(char *name) {
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (const auto &i : player_table) {
		if (!str_cmp(i.name(), arg)) {
			return (i.uid());
		}
	}

	return (-1);
}

int GetPlayerUidByName(int id) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.uid() == id) {
			return i.uid();
		}
	}
	return -1;
}

const char *GetNameById(long id) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (const auto &i : player_table) {
		if (i.uid() == id) {
			return i.name();
		}
	}
	return "";
}

const char *GetPlayerNameByUnique(int unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.uid() == unique) {
			return i.name();
		}
	}
	return nullptr;
}

int GetLevelByUnique(long unique) {
	int level = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.uid() == unique) {
			level = i.level;
		}
	}
	return level;
}

long GetLastlogonByUnique(long unique) {
	long time = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.uid() == unique) {
			time = i.last_logon;
		}
	}
	return time;
}

int IsCorrectUnique(int unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.uid() == unique) {
			return true;
		}
	}

	return false;
}

void RecreateSaveinfo(size_t number) {
	delete player_table[number].timer;
	NEWCREATE(player_table[number].timer);
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void ActualizePlayersIndex(char *name) {
	int deleted;
	char filename[kMaxStringLength];

	for (int i = 0; (name[i] = LOWER(name[i])); i++);
	if (get_filename(name, filename, kPlayersFile)) {
		Player t_short_ch;
		Player *short_ch = &t_short_ch;
		deleted = 1;
		// персонаж загружается неполностью
		if (LoadPlayerCharacter(name, short_ch, ELoadCharFlags::kReboot) > -1) {
			// если чар удален или им долго не входили, то не создаем для него запись
			if (!MustBeDeleted(short_ch)) {
				deleted = 0;

				PlayerIndexElement element(GET_NAME(short_ch));

				CREATE(element.mail, strlen(GET_EMAIL(short_ch)) + 1);
				for (int i = 0; (element.mail[i] = LOWER(GET_EMAIL(short_ch)[i])); i++);

				CREATE(element.last_ip, strlen(GET_LASTIP(short_ch)) + 1);
				for (int i = 0; (element.last_ip[i] = GET_LASTIP(short_ch)[i]); i++);

				element.set_uid(short_ch->get_uid());
				element.level = GetRealLevel(short_ch);
				element.remorts = short_ch->get_remort();
				element.timer = nullptr;
				element.plr_class = short_ch->GetClass();
				if (short_ch->IsFlagged(EPlrFlag::kDeleted)) {
					element.last_logon = -1;
					element.activity = -1;
				} else {
					element.last_logon = LAST_LOGON(short_ch);
					element.activity = number(0, kObjectSaveActivity - 1);
				}

#ifdef TEST_BUILD
				log("entry: char:%s level:%d mail:%s ip:%s", element.name(), element.level, element.mail, element.last_ip);
#endif

				top_idnum = std::max(top_idnum, short_ch->get_uid());
				TopPlayer::Refresh(short_ch, true);

				log("Adding new player %s", element.name());
				player_table.Append(element);
			} else {
				log("Delete %s from account email: %s",
					GET_NAME(short_ch),
					short_ch->get_account()->get_email().c_str());
				short_ch->get_account()->remove_player(short_ch->get_uid());
			}
		} else {
			log("SYSERR: Failed to load player %s.", name);
		}

		// если чар уже удален, то стираем с диска его файл
		if (deleted) {
			log("Player %s already deleted - kill player file", name);
			remove(filename);
			// 2) Remove all other files
			get_filename(name, filename, kAliasFile);
			remove(filename);
			get_filename(name, filename, kScriptVarsFile);
			remove(filename);
			get_filename(name, filename, kPersDepotFile);
			remove(filename);
			get_filename(name, filename, kShareDepotFile);
			remove(filename);
			get_filename(name, filename, kPurgeDepotFile);
			remove(filename);
			get_filename(name, filename, kTextCrashFile);
			remove(filename);
			get_filename(name, filename, kTimeCrashFile);
			remove(filename);
		}
	}
}

void BuildPlayerIndexNew() {
	FILE *players;
	char name[kMaxInputLength], playername[kMaxInputLength];

	if (!(players = fopen(LIB_PLRS "players.lst", "r"))) {
		log("Players list empty...");
		return;
	}

	while (get_line(players, name)) {
		if (!*name || *name == ';')
			continue;
		if (sscanf(name, "%s ", playername) == 0)
			continue;

		if (!player_table.IsPlayerExists(playername)) {
			ActualizePlayersIndex(playername);
		}
	}

	fclose(players);

	player_table.GetNameAdviser().init();
}

void FlushPlayerIndex() {
	FILE *players;
	char name[kMaxStringLength];

	if (!(players = fopen(LIB_PLRS "players.lst", "w+"))) {
		log("Can't save players list...");
		return;
	}

	std::size_t saved = 0;
	for (auto &i : player_table) {
		if (!i.name() || !*i.name()) {
			continue;
		}

		++saved;
		sprintf(name, "%s %ld %d %d\n",
				i.name(),
				i.uid(), i.level, i.last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Сохранено индексов %zd (считано при загрузке %zd)", saved, player_table.size());
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
bool MustBeDeleted(CharData *short_ch) {
	int ci, timeout;

	if (short_ch->IsFlagged(EPlrFlag::kNoDelete)) {
		return false;
	}

	if (short_ch->IsFlagged(EPlrFlag::kDeleted)) {
		return true;
	}

	if (short_ch->get_remort()) {
		return false;
	}

	timeout = -1;
	for (ci = 0; ci == 0 || pclean_criteria[ci].level > pclean_criteria[ci - 1].level; ci++) {
		if (short_ch->GetLevel() <= pclean_criteria[ci].level) {
			timeout = pclean_criteria[ci].days;
			break;
		}
	}
	if (timeout >= 0) {
		timeout *= kSecsPerRealDay;
		if ((time(nullptr) - LAST_LOGON(short_ch)) > timeout) {
			return true;
		}
	}

	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
