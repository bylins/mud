/*
* part of bylins
* 2018 (c) bodrich
*/
#include "accounts.hpp"
#include "db.h"
#include "comm.h"
#include "password.hpp"
#include "utils.h"

#include <boost/algorithm/string.hpp>

std::unordered_map<std::string, std::shared_ptr<Account>> accounts;
extern std::string GetNameByUnique(long unique, bool god);
extern bool CompareParam(const std::string & buffer, const char *arg, bool full);

#if defined(NOCRYPT)
#define CRYPT(a,b) ((char *) (a))
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

const std::shared_ptr<Account> Account::get_account(const std::string& email)
{
	const auto search_element = accounts.find(email);
	if(search_element != accounts.end())
	{
		return search_element->second;
	}

	return nullptr;
}

void Account::complete_quest(int id)
{
	for (auto &x : this->dquests)
	{
		if (x.id == id)
		{
			x.count += 1;
			x.time = time(0);
			return;
		}
	}
	DQuest dq_tmp;
	dq_tmp.id = id;
	dq_tmp.count = 1;
	dq_tmp.time = time(0);
	this->dquests.push_back(dq_tmp);
}

void Account::show_list_players(DESCRIPTOR_DATA *d)
{
	SEND_TO_Q("Ваши персонажи:\r\n", d);
	int count = 1;
	for (auto &x : this->players_list)
	{
		SEND_TO_Q((std::to_string(count) + ") ").c_str(), d);
		SEND_TO_Q(GetNameByUnique(x, false).c_str(), d);
		SEND_TO_Q("\r\n", d);
		count++;
	}
	SEND_TO_Q(MENU, d);
}

void Account::save_to_file()
{
	std::ofstream out;
	out.open(LIB_ACCOUNTS + this->email);
	if (out.is_open())
	{
		for (const auto &x : this->dquests)
		{
			out << "DaiQ: " << x.id << " " << x.count << " " << x.time << "\n";
		}
		for (const auto &x : this->history_logins)
		{
			out << "hl: " << x.first << " " << x.second.count << " " << x.second.last_login << "\n";
		}
		for (const auto &x : this->players_list)
		{
			out << "p: " << x << "\n";
		}
		out << "Pwd: " << this->hash_password << "\n";
		out << "ll: " << this->last_login << "\n";
	}
	out.close();
}

void Account::read_from_file()
{
	std::string line;
	std::ifstream in(LIB_ACCOUNTS + this->email);
	std::vector<std::string> tmp;
	if (in.is_open())
	{
		while (getline(in, line))
		{
			if (boost::starts_with(line, "DaiQ: "))
			{
				boost::split(tmp, line, boost::is_any_of(" "));
				DQuest tmp_quest;
				tmp_quest.id = atoi(tmp[1].c_str());
				tmp_quest.count = atoi(tmp[2].c_str());
				tmp_quest.time = atoi(tmp[3].c_str());
				this->dquests.push_back(tmp_quest);
			}
			if (boost::starts_with(line, "hl: "))
			{
				boost::split(tmp, line, boost::is_any_of(" "));
				login_index tmp_li;
					tmp_li.count = atoi(tmp[2].c_str());
				tmp_li.last_login = atoi(tmp[3].c_str());
				this->history_logins.insert(std::pair<std::string, login_index>(tmp[1].c_str(), tmp_li));
			}
			if (boost::starts_with(line, "p: "))
			{
				boost::split(tmp, line, boost::is_any_of(" "));
				this->add_player(atoi(tmp[1].c_str()));					
			}
			if (boost::starts_with(line, "Pwd: "))
			{
				boost::split(tmp, line, boost::is_any_of(" "));
				this->hash_password = tmp[1];
			}
			if (boost::starts_with(line, "ll: "))
			{
				boost::split(tmp, line, boost::is_any_of(" "));
				this->last_login = atoi(tmp[1].c_str());
			}
		}
		in.close();
	}
}

std::string Account::get_email()
{
	return this->email;
}

void Account::add_player(int uid)
{
	// если уже есть, то не добавляем
	for (auto &x : this->players_list)
		if (x == uid)
			return;
	this->players_list.push_back(uid);
}

void Account::remove_player(int uid)
{
	for (size_t i = 0; i < this->players_list.size(); i++)
	{
		if (this->players_list[i] == uid)
		{
			this->players_list.erase(this->players_list.begin() + i);
			return;
		}
	}
	//mudlog("Функция Account::remove_player, uid  %d не был найден", uid);
}

time_t Account::get_last_login()
{
	return this->last_login;
}

void Account::set_last_login()
{
	this->last_login = time(0);
}

Account::Account(const std::string& email)
{
	this->email = email;
	this->read_from_file();
	this->hash_password = "";
	this->last_login = 0;
}

void Account::add_login(const std::string& ip_addr)
{
	if (this->history_logins.count(ip_addr))
	{
		this->history_logins[ip_addr].last_login = time(0);
		this->history_logins[ip_addr].count += 1;
		return;
	}
	login_index tmp;
	tmp.last_login = time(0);
	tmp.count = 1;
	this->history_logins.insert(std::pair<std::string, login_index>(ip_addr, tmp));
}

/* Показ хистори логинов */
void Account::show_history_logins(DESCRIPTOR_DATA* d)
{
	char temp_buf[256] = "\0";
	for (auto &x : this->history_logins)
	{
		sprintf(temp_buf, "IP: %s, count: %d, time: %s\r\n", x.first.c_str(), x.second.count, rustime(localtime(&x.second.last_login)));
		SEND_TO_Q(temp_buf, d);
	}
}

void Account::set_password(const std::string& password)
{
	this->hash_password = Password::generate_md5_hash(password);
}

bool Account::compare_password(const std::string& password)
{
	return CompareParam(this->hash_password, CRYPT(password.c_str(), this->hash_password.c_str()), true);
}

bool Account::quest_is_available(int id)
{
	for (const auto &x : this->dquests)
	{
		if (x.id == id)
		{
			if (x.time != 0 && (time(0) - x.time) < 82800)
			{
				return false;
			}
			return true;
		}
	}
	return true;
}