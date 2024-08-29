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

// =======================================  INSPECT REQUEST INTERFACE  ============================================

class InspectRequest {
 public:
  enum class Status { kProcessing, kFinished };

  InspectRequest() = delete;
  InspectRequest(const InspectRequest &) = delete;
  InspectRequest(InspectRequest &&) = delete;
  InspectRequest &operator=(const InspectRequest &) = delete;
  InspectRequest &operator=(InspectRequest &&) = delete;
  virtual ~InspectRequest() = default;
  int GetAuthorUid() const { return author_uid_; }
  Status Inspect();

 protected:
  explicit InspectRequest(const CharData *author, std::vector<std::string> &args);
  void PageOutputToAuthor(DescriptorData *author_descriptor) const;
  void PrintSearchStatisticsToOutput();
  void PrintCharInfoToOutput(const PlayerIndexElement &index);
  void AddToOutput(const std::string &str) { output_ << str; };
  void MailOutputTo(const std::string &email);
  bool IsPlayerMathed(const PlayerIndexElement &index);
  bool GetMatchCount() const { return match_count_; };
  bool IsAuthorPrivilegedForInspect(const PlayerIndexElement &index) const;
  const std::string &GetRequestText() const { return request_text_; };

  virtual void OutputResult(DescriptorData *author_descriptor);
  virtual void PrintOtherInspectInfoToOutput() {}
  virtual bool IsIndexMatched(const PlayerIndexElement &index) = 0;

 private:
  int author_uid_{0};
  int author_level_{0};
  int match_count_{0};
  std::size_t current_player_table_pos_{0};
  struct timeval request_creation_time_{};
  std::string request_text_;
  std::ostringstream output_;

  void InspectPlayerTable();
  void InspectIndex(const PlayerIndexElement &index);
  void PrintBaseCharInfoToOutput(const PlayerIndexElement &index);
  void PrintExtraCharInfoToOutput(const PlayerIndexElement &index);
  void PrintPunishmentsInfoToOutput(std::shared_ptr<Player> &player);
  void PrintSinglePunishmentInfoToOutput(std::string_view punish_text, const Punish &punish);
  void PrintClanAbbrevToOutput(const std::shared_ptr<Player> &player);
  Status CheckStatus(DescriptorData *author);

  static bool IsTimeExpired(timeval &start_time);
};

InspectRequest::InspectRequest(const CharData *author, std::vector<std::string> &args) {
	author_uid_ = author->get_uid();
	author_level_ = author->GetLevel();
	request_text_ = args[kRequestTextPos];
	gettimeofday(&request_creation_time_, nullptr);
}

InspectRequest::Status InspectRequest::Inspect() {
	DescriptorData *author_descriptor = DescriptorByUid(author_uid_);
	if (!author_descriptor || (author_descriptor->connected != CON_PLAYING)) {
		return Status::kFinished;
	}

	InspectPlayerTable();
	return CheckStatus(author_descriptor);
}

void InspectRequest::InspectPlayerTable() {
	timeval start_inspect_time{};
	gettimeofday(&start_inspect_time, nullptr);
	for (; current_player_table_pos_ < player_table.size(); ++current_player_table_pos_) {
		if (IsTimeExpired(start_inspect_time)) {
			break;
		}
		const auto &player_index = player_table[current_player_table_pos_];
		if (IsAuthorPrivilegedForInspect(player_index)) {
			InspectIndex(player_index);
		}
	}
}

bool InspectRequest::IsTimeExpired(timeval &start_time) {
	timeval current_time{}, delta_time{};
	gettimeofday(&current_time, nullptr);
	timediff(&delta_time, &current_time, &start_time);
	return (delta_time.tv_sec > 0 || delta_time.tv_usec > kOptUsec);
}

bool InspectRequest::IsAuthorPrivilegedForInspect(const PlayerIndexElement &index) const {
	return ((author_level_ >= kLvlImplementator) || ((index.level < author_level_) && (index.level < kLvlImmortal)));
}

void InspectRequest::InspectIndex(const PlayerIndexElement &index) {
	if (IsPlayerMathed(index)) {
		PrintCharInfoToOutput(index);
		PrintOtherInspectInfoToOutput();
	}
}

InspectRequest::Status InspectRequest::CheckStatus(DescriptorData *author) {
	if (current_player_table_pos_ >= player_table.size()) {
		OutputResult(author);
		return Status::kFinished;
	}
	return Status::kProcessing;
}

void InspectRequest::OutputResult(DescriptorData *author_descriptor) {
	PrintSearchStatisticsToOutput();
	PageOutputToAuthor(author_descriptor);
}

void InspectRequest::PrintSearchStatisticsToOutput() {
	timeval stop{}, result{};
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &request_creation_time_);
	output_ << fmt::format("Всего найдено: {} за {} сек.\r\n", match_count_, result.tv_sec);
}

void InspectRequest::PageOutputToAuthor(DescriptorData *author_descriptor) const {
	page_string(author_descriptor, output_.str());
}

void InspectRequest::PrintCharInfoToOutput(const PlayerIndexElement &index) {
	PrintBaseCharInfoToOutput(index);
	PrintExtraCharInfoToOutput(index);
}

void InspectRequest::PrintBaseCharInfoToOutput(const PlayerIndexElement &index) {
	DescriptorData *d_vict = DescriptorByUid(index.unique);
	time_t last_logon = index.last_logon;
	output_ << fmt::format("{:-^121}\r\n", "*");
	output_ << fmt::format("Name: {}{:<12}{} E-mail: {:<30} Last: {} from IP: {}, Level {}, Remort {}, Class: {}",
						   (d_vict ? KIGRN : KIWHT), index.name(), KNRM,
						   index.mail, rustime(localtime(&last_logon)), index.last_ip,
						   index.level, index.remorts, MUD::Class(index.plr_class).GetName());
}

void InspectRequest::PrintExtraCharInfoToOutput(const PlayerIndexElement &index) {
	auto player = std::make_shared<Player>();
	if (load_char(index.name(), player.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
		PrintClanAbbrevToOutput(player);
		PrintPunishmentsInfoToOutput(player);
	} else {
		output_ << ".\r\n";
	}
}

void InspectRequest::PrintClanAbbrevToOutput(const std::shared_ptr<Player> &player) {
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

void InspectRequest::PrintSinglePunishmentInfoToOutput(std::string_view punish_text, const Punish &punish) {
	if (punish.duration) {
		output_ << fmt::format(" {}{}{} until {:%R %d-%B-%Y} [{}].\r\n",
							   KIRED, punish_text, KNRM,
							   std::chrono::system_clock::from_time_t(punish.duration),
							   (punish.reason ? punish.reason : "-"));
	}
}

bool InspectRequest::IsPlayerMathed(const PlayerIndexElement &index) {
	auto result = IsIndexMatched(index);
	match_count_ += result;
	return result;
}

void InspectRequest::MailOutputTo(const std::string &email) {
	std::string cmd_line = "python3 MailOutputTo.py " + email + " " + output_.str() + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// =======================================  INSPECT IP  ============================================

class InspectRequestIp : public InspectRequest {
 public:
  explicit InspectRequestIp(const CharData *author, std::vector<std::string> &args);

 private:
  bool IsIndexMatched(const PlayerIndexElement &index) final;
};

InspectRequestIp::InspectRequestIp(const CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	AddToOutput(fmt::format("Inspecting IP (last logon): {}{}{}\r\n", KWHT, GetRequestText(), KNRM));
}

bool InspectRequestIp::IsIndexMatched(const PlayerIndexElement &index) {
	return (index.last_ip && strstr(index.last_ip, GetRequestText().c_str()));
}

// =======================================  INSPECT MAIL  ============================================

class InspectRequestMail : public InspectRequest {
 public:
  explicit InspectRequestMail(const CharData *author, std::vector<std::string> &args);

 private:
  bool send_mail_{false};

  bool IsIndexMatched(const PlayerIndexElement &index) final;
  void OutputResult(DescriptorData *author_descriptor) final;
};

InspectRequestMail::InspectRequestMail(const CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	AddToOutput(fmt::format("Inspecting e-mail: {}{}{}\r\n", KWHT, GetRequestText(), KNRM));
	send_mail_ = ((args.size() > kMinArgsNumber) && (isname(args.back(), "send_mail")));
}

bool InspectRequestMail::IsIndexMatched(const PlayerIndexElement &index) {
	return (index.mail && strstr(index.mail, GetRequestText().c_str()));
}

void InspectRequestMail::OutputResult(DescriptorData *author_descriptor) {
	PrintSearchStatisticsToOutput();
	if (send_mail_ && GetMatchCount() > 0) {
		MailOutputTo(GetRequestText());
		AddToOutput("Данный список отправлен игроку на емайл\r\n");
	}
	PageOutputToAuthor(author_descriptor);
}

// =======================================  INSPECT CHAR  ============================================

class InspectRequestChar : public InspectRequest {
 public:
  explicit InspectRequestChar(const CharData *author, std::vector<std::string> &args);

 private:
  std::string mail_;
  std::string last_ip_;

  void NoteVictimInfo(const PlayerIndexElement &index);
  bool IsIndexMatched(const PlayerIndexElement &index) final;
};

InspectRequestChar::InspectRequestChar(const CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	auto vict_uid = GetUniqueByName(GetRequestText());
	if (vict_uid <= 0) {
		throw std::runtime_error("Inspecting char: Неизвестное имя персонажа.\r\n");
	} else {
		auto player_pos = GetPtableByUnique(vict_uid);
		const auto &player_index = player_table[player_pos];
		if (IsAuthorPrivilegedForInspect(player_index)) {
			NoteVictimInfo(player_index);
		} else {
			throw std::runtime_error("А ежели он ваc молнией по тыковке?\r\n");
		}
	}
}

void InspectRequestChar::NoteVictimInfo(const PlayerIndexElement &index) {
	mail_ = index.mail;
	last_ip_ = index.last_ip;
	AddToOutput(fmt::format("Incpecting character (e-mail or last IP): {}{}{}. E-mail: {} Last IP: {}\r\n",
							KWHT, GetRequestText(), KNRM, mail_, last_ip_));
}

bool InspectRequestChar::IsIndexMatched(const PlayerIndexElement &index) {
	return ((index.mail && (mail_ == index.mail)) || (index.last_ip && (last_ip_ == index.last_ip)));
}

// =======================================  INSPECT ALL  ============================================

class InspectRequestAll : public InspectRequest {
 public:
  explicit InspectRequestAll(const CharData *author, std::vector<std::string> &args);

 private:
  int vict_uid_{0};
  std::set<std::string> victim_ip_log_;    // айпи адреса по которым идет поиск
  std::ostringstream logon_buffer_;        // записи о совпавших коннектах проверяемого персонажа

  void NoteVictimInfo(const std::shared_ptr<Player> &vict);
  bool IsIndexMatched(const PlayerIndexElement &index) final;
  bool IsIpMatched(const char *ip);
  bool InspectLogonList(const CharData::shared_ptr &player);
  void NoteLogonInfo(const Logon &logon);
  void PrintOtherInspectInfoToOutput() final;
};

InspectRequestAll::InspectRequestAll(const CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	if (!IS_GRGOD(author)) {
		throw std::runtime_error("Вы не столь божественны, как вам кажется.\r\n");
	}

	auto vict = std::make_shared<Player>();
	if (load_char(GetRequestText().c_str(), vict.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
		NoteVictimInfo(vict);
	} else {
		throw std::runtime_error(fmt::format("Inspecting all: не удалось загрузить файл персонажа '{}'.\r\n",
											 GetRequestText()));
	}
}

void InspectRequestAll::NoteVictimInfo(const std::shared_ptr<Player> &vict) {
	AddToOutput(fmt::format("Inspecting all (IP intersection): {}{}{}\r\n", KIWHT, GetRequestText(), KNRM));
	vict_uid_ = vict->get_uid();
	for (const auto &logon : LOGON_LIST(vict)) {
		if (logon.ip && !kIgnoredIpChecklist.contains(logon.ip)) {
			victim_ip_log_.insert(logon.ip);
		}
	}
}

void InspectRequestAll::PrintOtherInspectInfoToOutput() {
	AddToOutput(logon_buffer_.str());
	logon_buffer_.str("");
	logon_buffer_.clear();
}

bool InspectRequestAll::IsIndexMatched(const PlayerIndexElement &index) {
	if (vict_uid_ == index.unique) {
		return false;
	}

	auto d_vict = DescriptorByUid(index.unique);
	if (d_vict) {
		return InspectLogonList(d_vict->character);
	} else {
		auto player = std::make_shared<Player>();
		if (load_char(index.name(), player.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
			return InspectLogonList(player);
		}
	}
	return false;
}

bool InspectRequestAll::InspectLogonList(const CharData::shared_ptr &player) {
	bool result{false};
	for (const auto &logon : LOGON_LIST(player)) {
		if (IsIpMatched(logon.ip)) {
			result = true;
			NoteLogonInfo(logon);
		}
	}
	return result;
}

bool InspectRequestAll::IsIpMatched(const char *ip) {
	return (ip && !kIgnoredIpChecklist.contains(ip) && victim_ip_log_.contains(ip));
}

void InspectRequestAll::NoteLogonInfo(const Logon &logon) {
	logon_buffer_ << fmt::format(" IP: {}{:<16}{} Количество заходов: {}. Последний: {}.\r\n",
								 KICYN, logon.ip, KNRM, logon.count,
								 rustime(localtime(&logon.lasttime)));
}

// ================================== Inspect Request Factory ======================================

class InspectRequestFactory {
 public:
  InspectRequestFactory() = delete;
  static InspectRequestPtr Create(const CharData *ch, char *argument);

 private:
  static void ValidateArgs(std::vector<std::string> &args);
  static InspectRequestPtr CreateRequest(const CharData *ch, std::vector<std::string> &args);
};

InspectRequestPtr InspectRequestFactory::Create(const CharData *ch, char *argument) {
	std::vector<std::string> args;
	SplitArgument(argument, args);
	ValidateArgs(args);
	return CreateRequest(ch, args);
}

void InspectRequestFactory::ValidateArgs(std::vector<std::string> &args) {
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

InspectRequestPtr InspectRequestFactory::CreateRequest(const CharData *ch, std::vector<std::string> &args) {
	auto &request_type = args[kRequestTypePos];
	if (utils::IsAbbr(request_type.c_str(), "mail")) {
		return std::make_shared<InspectRequestMail>(ch, args);
	} else if (utils::IsAbbr(request_type.c_str(), "ip")) {
		return std::make_shared<InspectRequestIp>(ch, args);
	} else if (utils::IsAbbr(request_type.c_str(), "char")) {
		return std::make_shared<InspectRequestChar>(ch, args);
	} else if (utils::IsAbbr(request_type.c_str(), "all")) {
		return std::make_shared<InspectRequestAll>(ch, args);
	}
	throw std::runtime_error("Неизвестный тип запроса.\r\n");
}

// ================================= Inspect Request Deque =======================================

void InspectRequestDeque::Create(const CharData *ch, char *argument) {
	if (IsQueueAvailable(ch)) {
		emplace_back(InspectRequestFactory::Create(ch, argument));
		SendMsgToChar("Запрос создан, ожидайте результата...\r\n", ch);
	} else {
		SendMsgToChar("Обрабатывается другой запрос, подождите...\r\n", ch);
	}
}

bool InspectRequestDeque::IsQueueAvailable(const CharData *ch) {
	auto it_setall = setall_inspect_list.find(ch->get_uid());
	return (!IsBusy(ch) && (it_setall == setall_inspect_list.end()));
}

bool InspectRequestDeque::IsBusy(const CharData *ch) {
	auto predicate = [ch](const auto &p)
		{ return (ch->get_uid() == p->GetAuthorUid()); };
	return (std::find_if(begin(), end(), predicate) != end());
}

void InspectRequestDeque::Inspecting() {
	if (empty()) {
		return;
	}

	if (front()->Inspect() == InspectRequest::Status::kFinished) {
		pop_front();
	}
}

// ================================= Commands =======================================

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || ch->get_pfilepos() < 0) {
		return;
	}

	try {
		MUD::InspectRequests().Create(ch, argument);
	} catch (std::runtime_error &e) {
		SendMsgToChar(ch, "%s", e.what());
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
