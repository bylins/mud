#include "inspect.h"

#include <memory>
#include <iostream>

#include "house.h"
#include "entities/char_player.h"
#include "structs/global_objects.h"
#include "color.h"
#include "modify.h"
#include "fmt/format.h"
#include "fmt/chrono.h"

const int kMaxRequestLength{65};
const int kMinRequestLength{3};
const int kMinArgsNumber{2};
const int kRequestTypePos{0};
const int kRequestTextPos{1};

const std::set<std::string> kIgnoredIpChecklist = {"135.181.219.76"};

bool IsTimeout(timeval &start_time);
bool IsAuthorLevelAcceptable(CharData *author, const PlayerIndexElement &index);
void CheckSentRequests(CharData *ch);
void ValidateArgs(std::vector<std::string> &args);
void CreateRequest(CharData *ch, std::vector<std::string> &args);

class InspectRequest {
 public:
  explicit InspectRequest(CharData *author, std::vector<std::string> &args);
  virtual ~InspectRequest() = default;
  void Inspect();
  bool IsFinished() const { return finished_; };

 protected:
  std::string request_text_;

  virtual void SendInspectResult(DescriptorData *author_descriptor);
  void PageOutputToAuthor(DescriptorData *author_descriptor) const;
  void PrintFindStatToOutput();
  void PrintCharInfoToOutput(const PlayerIndexElement &index);
  bool IsPlayerFound(std::shared_ptr<CharData> &author, const PlayerIndexElement &index);
  void AddToOutput(const std::string &str) { output_ << str; };
  void MailOutputTo(const std::string &email);
  bool GetNumFound() const { return found_; };
  virtual bool IsMatchedRequest(const PlayerIndexElement &index) = 0;
  virtual void PrintOtherInfoToOutput() { }

 private:
  bool finished_{false};
  int author_uid_{0};
  int found_{0};                    // сколько всего найдено
  std::size_t player_table_pos_{0};	// текущая позиция поиска
  struct timeval start_{};          // время когда запустили запрос
  std::ostringstream output_;       // буфер в который накапливается вывод

  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index);
  void PrintPunishmentsInfoToOutput(std::shared_ptr<Player> &player);
  void PrintSinglePunishmentInfoToOutput(std::string_view name, const Punish &punish);
  void PrintClanAbbrevToOutput(std::shared_ptr<Player> &player);
};

InspectRequest::InspectRequest(CharData *author, std::vector<std::string> &args) {
	author_uid_ = author->get_uid();
	request_text_ = args[kRequestTextPos];
	gettimeofday(&start_, nullptr);
}

void InspectRequest::Inspect() {
	DescriptorData *author_descriptor = DescriptorByUid(author_uid_);
	if (!author_descriptor || (author_descriptor->connected != CON_PLAYING)) {
		finished_ = true;
		return;
	}

	auto author = author_descriptor->character;
	if (!author) {
		finished_ = true;
		return;
	}

	timeval start_time{};
	gettimeofday(&start_time, nullptr);
	for (; player_table_pos_ < player_table.size(); ++player_table_pos_) {
		if (IsTimeout(start_time)) {
			return;
		}
#ifdef TEST_BUILD
		log("inspecting %d/%lu", 1 + player_table_pos_, player_table.size());
#endif
		const auto &player_index = player_table[player_table_pos_];
		if (IsAuthorLevelAcceptable(author.get(), player_index)) {
			InspectIndex(author, player_index);
		}
	}

	if (player_table_pos_ == player_table.size()) {
		SendInspectResult(author_descriptor);
		finished_ = true;
	}
}

bool IsTimeout(timeval &start_time) {
	timeval current_time{}, delta_time{};
	gettimeofday(&current_time, nullptr);
	timediff(&delta_time, &current_time, &start_time);
	return (delta_time.tv_sec > 0 || delta_time.tv_usec >= kOptUsec);
}

void InspectRequest::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (IsPlayerFound(author, index)) {
		PrintCharInfoToOutput(index);
		PrintOtherInfoToOutput();
	}
}

bool IsAuthorLevelAcceptable(CharData *author, const PlayerIndexElement &index) {
	return ((index.level < kLvlImmortal || IS_GRGOD(author))
		&& (index.level <= GetRealLevel(author)));
}

void InspectRequest::SendInspectResult(DescriptorData *author_descriptor) {
	PrintFindStatToOutput();
	PageOutputToAuthor(author_descriptor);
}

void InspectRequest::PrintFindStatToOutput() {
	timeval stop{}, result{};
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &start_);
	output_ << fmt::format("Всего найдено: {} за {} сек.\r\n", found_, result.tv_sec);
}

void InspectRequest::PageOutputToAuthor(DescriptorData *author_descriptor) const {
	page_string(author_descriptor, output_.str());
}

void InspectRequest::PrintCharInfoToOutput(const PlayerIndexElement &index) {
	DescriptorData *d_vict = DescriptorByUid(index.unique);
	time_t last_logon = index.last_logon;
	output_ << fmt::format("{:-^121}\r\n", "*");
	output_ << fmt::format("Name: {}{:<12}{} E-mail: {:<30} Last: {} from IP: {}, Level {}, Remort {}, Class: {}",
						   (d_vict ? KIGRN : KIWHT), index.name(), KNRM,
						   index.mail, rustime(localtime(&last_logon)), index.last_ip,
						   index.level, index.remorts, MUD::Class(index.plr_class).GetName());

	auto player = std::make_shared<Player>();
	if (load_char(index.name(), player.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
		PrintClanAbbrevToOutput(player);
		PrintPunishmentsInfoToOutput(player);
	} else {
		output_ << ".\r\n";
	}
}

void InspectRequest::PrintClanAbbrevToOutput(std::shared_ptr<Player> &player) {
	Clan::SetClanData(player.get());
	if (CLAN(player)) {
		output_ << fmt::format(", Clan: {}.\r\n", (player)->player_specials->clan->GetAbbrev());
	} else {
		output_ << ", Clan: нет.\r\n";
	}
}

void InspectRequest::PrintPunishmentsInfoToOutput(std::shared_ptr<Player> &player) {
	if (PLR_FLAGGED(player, EPlrFlag::kFrozen)) {
		PrintSinglePunishmentInfoToOutput("FROZEN", player->player_specials->pfreeze);
	}
	if (PLR_FLAGGED(player, EPlrFlag::kMuted)) {
		PrintSinglePunishmentInfoToOutput("MUTED", player->player_specials->pmute);
	}
	if (PLR_FLAGGED(player, EPlrFlag::kDumbed)) {
		PrintSinglePunishmentInfoToOutput("DUMBED", player->player_specials->pdumb);
	}
	if (PLR_FLAGGED(player, EPlrFlag::kHelled)) {
		PrintSinglePunishmentInfoToOutput("HELLED", player->player_specials->phell);
	}
	if (!PLR_FLAGGED(player, EPlrFlag::kRegistred)) {
		PrintSinglePunishmentInfoToOutput("UNREGISTERED", player->player_specials->punreg);
	}
}

void InspectRequest::PrintSinglePunishmentInfoToOutput(std::string_view name, const Punish &punish) {
	if (punish.duration) {
		output_ << fmt::format("{} until {:%R %d-%B-%Y} [{}].\r\n",
							   name,
							   std::chrono::system_clock::from_time_t(punish.duration),
							   (punish.reason ? punish.reason : "-"));
	}
}

bool InspectRequest::IsPlayerFound(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (IsAuthorLevelAcceptable(author.get(), index)) {
		auto result = IsMatchedRequest(index);
		found_ += result;
		return result;
	}
	return false;
}

void InspectRequest::MailOutputTo(const std::string &email) {
	std::string cmd_line = "python3 MailOutputTo.py " + email + " " + output_.str() + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// =======================================  INSPECT IP  ============================================

class InspectRequestIp : public InspectRequest {
 public:
  explicit InspectRequestIp(CharData *author, std::vector<std::string> &args);

 private:
    bool IsMatchedRequest(const PlayerIndexElement &index) final;
};

InspectRequestIp::InspectRequestIp(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	AddToOutput(fmt::format("Inspecting IP (last logon): {}{}{}\r\n", KWHT, args[kRequestTextPos], KNRM));
}

bool InspectRequestIp::IsMatchedRequest(const PlayerIndexElement &index) {
	if (index.last_ip) {
		return (strstr(index.last_ip, request_text_.c_str()));
	}
	return false;
}

// =======================================  INSPECT MAIL  ============================================

class InspectRequestMail : public InspectRequest {
 public:
  explicit InspectRequestMail(CharData *author, std::vector<std::string> &args);

 private:
  bool send_mail_{false};                // отправлять ли на мыло список чаров

  bool IsMatchedRequest(const PlayerIndexElement &index) final;
  void SendInspectResult(DescriptorData *author_descriptor) final;
};

InspectRequestMail::InspectRequestMail(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	AddToOutput(fmt::format("Inspecting e-mail: {}{}{}\r\n", KWHT, args[kRequestTextPos], KNRM));
	if (args.size() > kMinArgsNumber) {
		if (isname(args.back(), "send_mail")) {
			send_mail_ = true;
		}
	}
}

bool InspectRequestMail::IsMatchedRequest(const PlayerIndexElement &index) {
	if (index.mail) {
		return (strstr(index.mail, request_text_.c_str()));
	}
	return false;
}

void InspectRequestMail::SendInspectResult(DescriptorData *author_descriptor) {
	PrintFindStatToOutput();
	if (send_mail_ && GetNumFound() > 0) {
		MailOutputTo(request_text_);
		AddToOutput("Данный список отправлен игроку на емайл\r\n");
	}
	PageOutputToAuthor(author_descriptor);
}

// =======================================  INSPECT CHAR  ============================================

class InspectRequestChar : public InspectRequest {
 public:
  explicit InspectRequestChar(CharData *author, std::vector<std::string> &args);

 private:
  int vict_uid_{0};
  std::string mail_;
  std::string last_ip_;

  bool IsMatchedRequest(const PlayerIndexElement &index) final;
};

InspectRequestChar::InspectRequestChar(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	const auto victim_name = args[kRequestTextPos];
	vict_uid_ = GetUniqueByName(victim_name);
	if (vict_uid_ <= 0) {
		throw std::runtime_error("Inspecting char: Неизвестное имя персонажа.\r\n");
	} else {
		auto player_pos = GetPtableByUnique(vict_uid_);
		const auto &player_index = player_table[player_pos];
		if (IsAuthorLevelAcceptable(author, player_index)) {
			mail_ = player_index.mail;
			last_ip_ = player_index.last_ip;
			AddToOutput(fmt::format("Incpecting character (e-mail or last IP): {}{}{}. E-mail: {} Last IP: {}\r\n",
								   KWHT, args[kRequestTextPos], KNRM, mail_, last_ip_));
			PrintCharInfoToOutput(player_index);
		} else {
			throw std::runtime_error("А ежели он ваc молнией по тыковке?\r\n");
		}
	}
}

bool InspectRequestChar::IsMatchedRequest(const PlayerIndexElement &index) {
	if (vict_uid_ == index.unique) {
		return false;
	}
	bool result{false};
	if (index.mail) {
		result |= (mail_ == index.mail);
	}
	if (index.last_ip) {
		result |= (last_ip_ == index.last_ip);
	}
	return result;
}

// =======================================  INSPECT ALL  ============================================

class InspectRequestAll : public InspectRequest {
 public:
  explicit InspectRequestAll(CharData *author, std::vector<std::string> &args);

 private:
  int vict_uid_{0};
  std::set<std::string> victim_ip_log_;	// айпи адреса по которым идет поиск
  std::ostringstream  logon_buffer_;	// буфер, в который накапливаются записи о коннектах проверяемого персонажа

  bool IsMatchedRequest(const PlayerIndexElement &index) final;
  void PrintOtherInfoToOutput() final;
};

InspectRequestAll::InspectRequestAll(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	if (!IS_GRGOD(author)) {
		throw std::runtime_error("Вы не столь божественны, как вам кажется.\r\n");
	}

	auto vict = std::make_shared<Player>();
	if (load_char(request_text_.c_str(), vict.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
		AddToOutput(fmt::format("Inspecting all (IP intersection): {}{}{}\r\n", KIWHT, request_text_, KNRM));
		vict_uid_ = vict->get_uid();
		auto player_pos = GetPtableByUnique(vict->get_uid());
		const auto &player_index = player_table[player_pos];
		PrintCharInfoToOutput(player_index);
		for (const auto &logon : LOGON_LIST(vict)) {
			if (logon.ip) {
				if (!kIgnoredIpChecklist.contains(logon.ip)) {
					victim_ip_log_.insert(logon.ip);
				}
			}
		}
	} else {
		throw std::runtime_error("Inspecting all: не удалось загрузить файл персонажа.\r\n");
	}
}

void InspectRequestAll::PrintOtherInfoToOutput() {
	AddToOutput(logon_buffer_.str());
	logon_buffer_.str("");
	logon_buffer_.clear();
}

bool InspectRequestAll::IsMatchedRequest(const PlayerIndexElement &index) {
	if (vict_uid_ == index.unique) {
		return false;
	}
	auto player = std::make_shared<Player>();
	bool result{false};
	if (load_char(index.name(), player.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
		for (const auto &inspected_logon : LOGON_LIST(player)) {
			if (!inspected_logon.ip) {
				continue;
			}
			if (kIgnoredIpChecklist.contains(inspected_logon.ip)) {
				continue;
			}
			if (victim_ip_log_.contains(inspected_logon.ip)) {
				result =  true;
				logon_buffer_ << fmt::format(" IP: {}{:<16}{} Количество заходов: {}. Последний: {}.\r\n",
									   KICYN, inspected_logon.ip, KNRM, inspected_logon.count,
									   rustime(localtime(&inspected_logon.lasttime)));
			}
		}
	}
	return result;
}

// ========================================================================

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_pfilepos() < 0) {
		return;
	}

	try {
		CheckSentRequests(ch);
		std::vector<std::string> args;
		array_argument(argument, args);
		ValidateArgs(args);
		CreateRequest(ch, args);
		SendMsgToChar("Запрос создан, ожидайте результата...\r\n", ch);
	} catch (std::runtime_error &e) {
		SendMsgToChar(ch, "%s", e.what());
	}
}

void CheckSentRequests(CharData *ch) {
	auto it_inspect = MUD::inspect_list().find(ch->get_uid());
	auto it_setall = setall_inspect_list.find(ch->get_uid());
	if (it_inspect != MUD::inspect_list().end() || it_setall != setall_inspect_list.end()) {
		throw std::runtime_error("Обрабатывается другой запрос, подождите...\r\n");
	}
}

void ValidateArgs(std::vector<std::string> &args) {
	if (args.size() < kMinArgsNumber) {
		throw std::runtime_error("Usage: inspect { mail | ip | char | all } <argument> [send_mail]\r\n");
	}
	if (!isname(args[kRequestTypePos], "mail ip char all")) {
		throw std::runtime_error("Нет уж. Изыщите другую цель для своих исследований.\r\n");
	}
	auto &request_text = args[kRequestTextPos];
	if (request_text.length() < kMinRequestLength) {
		throw std::runtime_error("Слишком короткий запрос.\r\n");
	}
	if (request_text.length() > kMaxRequestLength) {
		throw std::runtime_error("Слишком длинный запрос.\r\n");
	}
}

void CreateRequest(CharData *ch, std::vector<std::string> &args) {
	auto &request_type = args[kRequestTypePos];
	if (utils::IsAbbr(request_type.c_str(), "mail")) {
		MUD::inspect_list().emplace(ch->get_pfilepos(), std::make_shared<InspectRequestMail>(ch, args));
	} else if (utils::IsAbbr(request_type.c_str(), "ip")) {
		MUD::inspect_list().emplace(ch->get_pfilepos(), std::make_shared<InspectRequestIp>(ch, args));
	} else if (utils::IsAbbr(request_type.c_str(), "char")) {
		MUD::inspect_list().emplace(ch->get_pfilepos(), std::make_shared<InspectRequestChar>(ch, args));
	} else if (utils::IsAbbr(request_type.c_str(), "all")) {
		MUD::inspect_list().emplace(ch->get_pfilepos(), std::make_shared<InspectRequestAll>(ch, args));
	}
}

void Inspecting() {
	if (MUD::inspect_list().empty()) {
		return;
	}

	auto it = MUD::inspect_list().begin();
	it->second->Inspect();
	if (it->second->IsFinished()) {
		MUD::inspect_list().erase(it);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
