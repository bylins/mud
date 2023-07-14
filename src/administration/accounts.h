/*
* part of bylins
* 2018 (c) bodrich
*/

#ifndef __ACCOUNTS_HPP__
#define __ACCOUNTS_HPP__

#include "structs/structs.h"
#include "entities/char_data.h"
#include "structs/descriptor_data.h"

#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include <unordered_map>

struct DQuest {
	int id;
	int count;
	time_t time;
};

struct login_index {
	// количество заходов
	int count;
	// дата последнего захода с данного ip
	time_t last_login;
};

class Account {
 private:
	std::string email;
	std::vector<std::string> characters;
	std::vector<DQuest> dquests;
	std::string karma;
	// список чаров на мыле (только уиды)
	std::vector<long> players_list;
	// пароль (а точнее его хеш) аккаунта
	std::string hash_password;
	// дата последнего входа в аккаунт
	time_t last_login;
	// История логинов, ключ - айпи, в структуре количество раз, с которых был произведен заход с данного айпи-адреса + дата, когда последний раз выходили с данного айпишника
	std::unordered_map<std::string, login_index> history_logins;

 public:
	void purge_erased();
	Account(const std::string &name);
	void save_to_file();
	void read_from_file();
	std::string get_email();
	bool quest_is_available(int id);
	int zero_hryvn(CharData *ch, int val);
	void complete_quest(int id);
	static const std::shared_ptr<Account> get_account(const std::string &email);
	void show_players(CharData *ch);
	void list_players(DescriptorData *d);
	void add_player(long uid);
	void remove_player(long uid);
	time_t get_last_login();
	void set_last_login();
	void set_password(const std::string &password);
	bool compare_password(const std::string &password);
	void show_history_logins(CharData *ch);
	void add_login(const std::string &ip_addr);
};

extern std::unordered_map<std::string, std::shared_ptr<Account>> accounts;

#endif    //__ACCOUNTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :