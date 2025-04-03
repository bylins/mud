/*
* part of bylins
* 2018 (c) bodrich
*/
#include "accounts.h"
#include "password.h"
#include "engine/entities/zone.h"
#include <sstream>
std::unordered_map<std::string, std::shared_ptr<Account>> accounts;

#if defined(NOCRYPT)
#define CRYPT(a,b) ((char *) (a))
#else
#define CRYPT(a, b) ((char *) crypt((a),(b)))
#endif

DQuest::DQuest()
	: id(0)
	, count(0)
	, time(0)
{
}


DQuest::DQuest(int id, int count, time_t time)
	: id(id)
	, count(count)
	, time(time)
{
}

LoginIndex::LoginIndex()
	: count(0)
	, last_login(time(0))
{
}

LoginIndex::LoginIndex(int count, time_t last_login)
	: count(count)
	, last_login(last_login)
{
}

std::shared_ptr<Account> Account::get_account(const std::string &email) {
	if (accounts.contains(email)) {
		return accounts[email];
	}
	return nullptr;
}

const std::string Account::config_parameter_daily_quest_ = "DaiQ: ";
const std::string Account::config_parameter_password_ = "Pwd: ";
const std::string Account::config_parameter_last_login_ = "ll: ";
const std::string Account::config_parameter_history_login_ = "hl: ";

Account::Account(const std::string &email)
	: email_(email)
	, last_login_(0)
{
	read_from_file();
}

// TODO логика никак не относится к аккаунту
// убрать процессинг к валютам, когда они будут готовы
int Account::zero_hryvn(CharData *ch, int val) {
	const int zone_lvl = zone_table[world[ch->in_room]->zone_rn].mob_level;
	const auto &chars_in_account = all_chars_in_account();
	for (auto &plr : chars_in_account) {
		if (!plr.name()) {
			continue;
		}

		if (zone_lvl <= 12 && (plr.level + plr.remorts / 5 >= 20)) {
			if (ch->IsFlagged(EPrf::kTester)) {
				SendMsgToChar(ch,
							  "У чара %s в расчете %d гривен, тут будет 0, левел %d морты %d обнуляем!!!\r\n",
							  plr.name(),
							  val,
							  plr.level,
							  plr.remorts);
			}
			val = 0;
		}
	}
	return val;
}

void Account::complete_quest(int id) {
	for (auto &x : dquests_) {
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
	dquests_.push_back(dq_tmp);
}

std::vector<PlayerIndexElement> Account::all_chars_in_account() const
{
	std::vector<PlayerIndexElement> result_list;

	const auto &player_table = GlobalObjects::player_table();
	std::copy_if(player_table.begin(), player_table.end(), std::back_inserter(result_list),
				 [this](const PlayerIndexElement &pie) {
		if (!pie.mail) {
			return false;
		}

		return email_.compare(pie.mail) == 0;
	});

	return result_list;
}

void Account::show_players(CharData *ch) {
	int count = 1;
	std::stringstream ss;
	const auto &chars_in_account = all_chars_in_account();
	ss << "Данные аккаунта: " << email_ << "\r\n";
	for (auto &x : chars_in_account) {
		if (!x.name()) {
			continue;
		}
		std::string name(x.name());
		name[0] = UPPER(name[0]);
		ss << count << ") " << name << "\r\n";
		count++;
	}
	SendMsgToChar(ss.str(), ch);
}

void Account::list_players(DescriptorData *d) {
	int count = 1;
	std::stringstream ss;
	const auto &chars_in_account = all_chars_in_account();
	ss << "Данные аккаунта: " << email_ << "\r\n";
	iosystem::write_to_output(ss.str().c_str(), d);
	for (auto &x : chars_in_account) {
		if (!x.name()) {
			continue;
		}
		std::string name(x.name());
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
	out.open(LIB_ACCOUNTS + email_);
	if (out.is_open()) {
		for (const auto &x : dquests_) {
			out << config_parameter_daily_quest_ << x.id << " " << x.count << " " << x.time << "\n";
		}
		for (const auto &x : history_logins_) {
			out << config_parameter_history_login_ << x.first << " " << x.second.count << " " << x.second.last_login << "\n";
		}
		if (hash_password_.has_value()) {
			// пароля на аккаунте может и не быть, поэтому проверяем перед сохранением
			out << config_parameter_password_ << hash_password_.value() << "\n";
		}
		out << config_parameter_last_login_ << last_login_ << "\n";
	}
	out.close();
}

void Account::read_from_file() {
	std::string line;
	std::ifstream in(LIB_ACCOUNTS + email_);
	std::vector<std::string> tmp;

	if (in.is_open()) {
		while (getline(in, line)) {
			const std::vector<std::string> splitted_line = utils::Split(line);
			if (line.starts_with(config_parameter_daily_quest_)) {
				const auto &read_id = atoi(tmp[1].c_str());
				const auto &read_count = atoi(tmp[2].c_str());
				const auto &read_time = atoi(tmp[3].c_str());
				dquests_.emplace_back(read_id, read_count, read_time);
			} else if (line.starts_with(config_parameter_password_)) {
				hash_password_ = line.substr(config_parameter_password_.length());
				if (hash_password_->empty()) {
					hash_password_ = std::nullopt;
				}
			}
		}
		in.close();
	}
}

time_t Account::get_last_login() const {
	return last_login_;
}

void Account::set_last_login() {
	last_login_ = time(nullptr);
}

void Account::add_login(const std::string &ip_addr) {
	if (history_logins_.count(ip_addr)) {
		history_logins_[ip_addr].last_login = time(nullptr);
		history_logins_[ip_addr].count += 1;
		return;
	}

	const auto current_last_login = time(nullptr);
	const int current_count = 1;
	const LoginIndex ll{current_count, current_last_login};
	history_logins_.insert(std::pair<std::string, LoginIndex>(ip_addr, ll));
}

/* Показ хистори логинов */
void Account::show_history_logins(CharData *ch) {
	std::stringstream ss;
	for (auto &x : history_logins_) {
		ss << "IP: " << x.first
				<< " count: " << x.second.count 
				<< " time: " << rustime(localtime(&x.second.last_login)) 
				<< "\r\n";
	}
	SendMsgToChar(ss.str(), ch);
}

void Account::set_password(const std::string &password) {
	hash_password_ = Password::generate_md5_hash(password);
}

bool Account::compare_password(const std::string &password) {
	return hash_password_.has_value() ? (hash_password_.value(), CRYPT(password.c_str(), hash_password_->c_str()), true) : false;
}

bool Account::quest_is_available(int id) {
	for (const auto &x : dquests_) {
		if (x.id == id) {
			if (x.time != 0 && (time(0) - x.time) < 82800) {
				return false;
			}
			return true;
		}
	}
	return true;
}
