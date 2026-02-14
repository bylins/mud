/*
* part of bylins
* 2018 (c) bodrich
*/
#include "accounts.h"
#include "password.h"
#include "engine/entities/zone.h"
#include <sstream>
#include "engine/db/player_index.h"

std::unordered_map<std::string, std::shared_ptr<Account>> accounts;

#if defined(NOCRYPT)
#define CRYPT(a,b) ((char *) (a))
#else
#include <crypt.h>
#define CRYPT(a, b) ((char *) crypt((a),(b)))
#endif

std::shared_ptr<Account> Account::get_account(const std::string &email) {
	if (accounts.contains(email)) {
		return accounts[email];
	}
	return nullptr;
}
int Account::zero_hryvn(CharData *ch, int val) {
	const int zone_lvl = zone_table[world[ch->in_room]->zone_rn].mob_level;
	for (auto &plr : this->players_list) {
		std::string name = GetNameByUnique(plr);
		if (name.empty()) {
			continue;
		}

		const auto &player = player_table[GetPtableByUnique(plr)];
		if (zone_lvl <= 12 && (player.level + player.remorts / 5 >= 20)) {
			if (ch->IsFlagged(EPrf::kTester)) {
				SendMsgToChar(ch,
							  "У чара %s в расчете %d гривен, тут будет 0, левел %d морты %d обнуляем!!!\r\n",
							  player.name().c_str(),
							  val,
							  player.level,
							  player.remorts);
			}
			val = 0;
		}
	}
	return val;
}

void Account::complete_quest(int id) {
	for (auto &x : this->dquests) {
		if (x.id == id) {
			x.count += 1;
			x.time = time(nullptr);
			return;
		}
	}
	DQuest dq_tmp{};
	dq_tmp.id = id;
	dq_tmp.count = 1;
	dq_tmp.time = time(nullptr);
	this->dquests.push_back(dq_tmp);
}

void Account::purge_erased() {
	std::string uid;
	for (size_t i = 0; i < this->players_list.size(); i++) {
		uid = GetNameByUnique(this->players_list[i]);
		if (uid.empty()) {
			this->players_list.erase(this->players_list.begin() + i);
		}
	}
	save_to_file();
}

void Account::show_players(CharData *ch) {
	int count = 1;
	std::stringstream ss;
	purge_erased();
	ss << "Данные аккаунта: " << this->email << "\r\n";
	for (auto &x : this->players_list) {
		std::string name = GetNameByUnique(x);
		name[0] = UPPER(name[0]);
		ss << count << ") " << name << "\r\n";
		count++;
	}
	SendMsgToChar(ss.str(), ch);
}

void Account::list_players(DescriptorData *d) {
	int count = 1;
	std::stringstream ss;
	purge_erased();
	ss << "Данные аккаунта: " << this->email << "\r\n";
	iosystem::write_to_output(ss.str().c_str(), d);
	for (auto &x : this->players_list) {
		std::string name = GetNameByUnique(x);
		iosystem::write_to_output((std::to_string(count) + ") ").c_str(), d);
		name[0] = UPPER(name[0]);
		iosystem::write_to_output(name.c_str(), d);
		iosystem::write_to_output("\r\n", d);
		count++;
	}
	iosystem::write_to_output(MENU, d);
}

void Account::save_to_file() {
	std::ofstream out;
	out.open(LIB_ACCOUNTS + this->email);
	if (out.is_open()) {
		for (const auto &x : this->dquests) {
			out << "DaiQ: " << x.id << " " << x.count << " " << x.time << "\n";
		}
		for (const auto &x : this->history_logins) {
			out << "hl: " << x.first << " " << x.second.count << " " << x.second.last_login << "\n";
		}
		for (const auto &x : this->players_list) {
			out << "p: " << x << "\n";
		}
		out << "Pwd: " << this->hash_password << "\n";
		out << "ll: " << this->last_login << "\n";
	}
	out.close();
}

void Account::read_from_file() {
	std::string line;
	std::ifstream in(LIB_ACCOUNTS + this->email);
	std::vector<std::string> tmp;

	if (in.is_open()) {
		while (getline(in, line)) {
			tmp = utils::Split(line);
			if (line.starts_with("DaiQ: ")) {
				DQuest tmp_quest{};
				tmp_quest.id = atoi(tmp[1].c_str());
				tmp_quest.count = atoi(tmp[2].c_str());
				tmp_quest.time = atoi(tmp[3].c_str());
				this->dquests.push_back(tmp_quest);
			}
/*			if (line.starts_with("hl: ")) {
				utils::Split(tmp, line);
				login_index tmp_li;
				tmp_li.count = atoi(tmp[2].c_str());
				tmp_li.last_login = atoi(tmp[3].c_str());
				this->history_logins.insert(std::pair<std::string, login_index>(tmp[1].c_str(), tmp_li));
			}
			if (line.starts_with("p: ")) {
				utils::Split(tmp, line);
				this->add_player(atoi(tmp[1].c_str()));
			}
			if (line.starts_with("Pwd: ")) {
				utils::Split(tmp, line);
				this->hash_password = tmp[1];
			}
			if (line.starts_with("ll: ")) {
				utils::Split(tmp, line);
				this->last_login = atoi(tmp[1].c_str());
			}
*/
		}
		in.close();
	}
}

std::string Account::get_email() {
	return this->email;
}
\
void Account::add_player(long uid) {
	// если уже есть, то не добавляем
	for (auto &x : this->players_list) {
		if (x == uid) {
			return;
		}
	}
	this->players_list.push_back(uid);
}

void Account::remove_player(long uid) {
	for (size_t i = 0; i < this->players_list.size(); i++) {
		if (this->players_list[i] == uid) {
			this->players_list.erase(this->players_list.begin() + i);
			return;
		}
	}
	//mudlog("Функция Account::remove_player, uid  %d не был найден", uid);
}

time_t Account::get_last_login() const {
	return this->last_login;
}

void Account::set_last_login() {
	this->last_login = time(nullptr);
}

Account::Account(const std::string &email) {
	this->email = email;
	this->read_from_file();
	this->hash_password = "";
	this->last_login = 0;
}

void Account::add_login(const std::string &ip_addr) {
	if (this->history_logins.count(ip_addr)) {
		this->history_logins[ip_addr].last_login = time(nullptr);
		this->history_logins[ip_addr].count += 1;
		return;
	}
	login_index tmp{};
	tmp.last_login = time(nullptr);
	tmp.count = 1;
	this->history_logins.insert(std::pair<std::string, login_index>(ip_addr, tmp));
}
/* Показ хистори логинов */
void Account::show_history_logins(CharData *ch) {
	std::stringstream ss;
	for (auto &x : this->history_logins) {
		ss << "IP: " << x.first
				<< " count: " << x.second.count 
				<< " time: " << rustime(localtime(&x.second.last_login)) 
				<< "\r\n";
	}
	SendMsgToChar(ss.str(), ch);
}

void Account::set_password(const std::string &password) {
	this->hash_password = Password::generate_md5_hash(password);
}

bool Account::compare_password(const std::string &password) {
	return CompareParam(this->hash_password, CRYPT(password.c_str(), this->hash_password.c_str()), true);
}

bool Account::quest_is_available(int id) {
	for (const auto &x : this->dquests) {
		if (x.id == id) {
			if (x.time != 0 && (time(0) - x.time) < 82800) {
				return false;
			}
			return true;
		}
	}
	return true;
}