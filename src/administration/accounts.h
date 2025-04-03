/*
* part of bylins
* 2018 (c) bodrich
*/

#ifndef __ACCOUNTS_HPP__
#define __ACCOUNTS_HPP__

#include "engine/structs/structs.h"
#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/db/global_objects.h"

#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include <unordered_map>

class DQuest {
public:
	DQuest();
	DQuest(int id, int count, time_t time);
	virtual ~DQuest() = default;

public:
	int id;
	int count;
	time_t time;
};

class LoginIndex {
public:
	LoginIndex();
	LoginIndex(int count, time_t last_login);
	virtual ~LoginIndex() = default;

public:
	// количество заходов
	int count;
	// дата последнего захода с данного ip
	time_t last_login;
};

class Account {
private:
	const std::string email_;
	std::vector<DQuest> dquests_;
	// пароль (а точнее его хеш) аккаунта
	std::optional<std::string> hash_password_;
	// дата последнего входа в аккаунт
	time_t last_login_;
	// История логинов, ключ - айпи, в структуре количество раз, с которых был произведен заход с данного айпи-адреса + дата, когда последний раз выходили с данного айпишника
	std::unordered_map<std::string, LoginIndex> history_logins_;

public:
	static std::shared_ptr<Account> get_account(const std::string &email);
	Account(const std::string &email);
	virtual ~Account() = default;

public:
	void save_to_file();
	void read_from_file();
	bool quest_is_available(int id);
	int zero_hryvn(CharData *ch, int val);
	void complete_quest(int id);
	void show_players(CharData *ch);
	void list_players(DescriptorData *d);
	time_t get_last_login() const;
	void set_last_login();
	void set_password(const std::string &password);
	bool compare_password(const std::string &password);
	void show_history_logins(CharData *ch);
	void add_login(const std::string &ip_addr);
	std::string email() const;

private:
	std::vector<PlayerIndexElement> all_chars_in_account() const;

private:
	static const std::string config_parameter_daily_quest_;
	static const std::string config_parameter_password_;
	static const std::string config_parameter_last_login_;
	static const std::string config_parameter_history_login_;
};

extern std::unordered_map<std::string, std::shared_ptr<Account>> accounts;

#endif    //__ACCOUNTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
