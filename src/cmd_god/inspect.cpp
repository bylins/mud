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
#include "utils/utils_time.h"

const int kMaxRequestLength{65};
const int kMinRequestLength{3};
const int kMinArgsNumber{2};
const int kRequestTypePos{0};
const int kRequestTextPos{1};

const std::set<std::string> kIgnoredIpChecklist = {"135.181.219.76"};

// =======================================  REQUEST RESULT  ============================================

class RequestResult {
 public:
  RequestResult() = default;

  void AddToOutput(const std::string &str) { output_ << str; };
  void MailOutputTo(const std::string &email);
  void PageOutputToAuthor(DescriptorData *author_descriptor) const;
 private:
  std::ostringstream output_;
};

void RequestResult::MailOutputTo(const std::string &email) {
	std::string cmd_line = "python3 MailOutputTo.py " + email + " " + output_.str() + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

void RequestResult::PageOutputToAuthor(DescriptorData *author_descriptor) const {
	page_string(author_descriptor, output_.str());
}

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
  RequestResult result_;

  explicit InspectRequest(const CharData *author, std::vector<std::string> &args);
  void ProcessMatchedIndex(const PlayerIndexElement &index);
  void PrintSearchStatisticsToOutput();
  void PrintCharInfoToOutput(const PlayerIndexElement &index);
  bool GetMatchCount() const { return match_count_; };
  bool IsAuthorPrivilegedForInspect(const PlayerIndexElement &index) const;
  const std::string &GetRequestText() const { return request_text_; };
  static CharData::shared_ptr GetCharPtr(const PlayerIndexElement &index);

  virtual void OutputResult(DescriptorData *author_descriptor);
  virtual void PrintOtherInspectInfoToOutput() {}
  virtual bool IsIndexMatched(const PlayerIndexElement &index) = 0;

 private:
  int author_uid_{0};
  int author_level_{0};
  int match_count_{0};
  std::size_t current_player_table_pos_{0};
  std::string request_text_;
  utils::CExecutionTimer request_timer_;
  utils::CExecutionTimer inspecting_timer_;

  void InspectPlayerTable();
  void InspectIndex(const PlayerIndexElement &index);
  void PrintBaseCharInfoToOutput(const PlayerIndexElement &index);
  void PrintExtraCharInfoToOutput(const PlayerIndexElement &index);
  void PrintPunishmentsInfoToOutput(std::shared_ptr<CharData> &player);
  void PrintSinglePunishmentInfoToOutput(std::string_view punish_text, const Punish &punish);
  void PrintClanAbbrevToOutput(const std::shared_ptr<CharData> &player);
  bool IsTimeExpired();
  Status CheckStatus(DescriptorData *author);
};

InspectRequest::InspectRequest(const CharData *author, std::vector<std::string> &args)
	: author_uid_(author->get_uid()),
	  author_level_(author->GetLevel()),
	  request_text_(args[kRequestTextPos]) {}

InspectRequest::Status InspectRequest::Inspect() {
	DescriptorData *author_descriptor = DescriptorByUid(author_uid_);
	if (!author_descriptor || (author_descriptor->connected != CON_PLAYING)) {
		return Status::kFinished;
	}

	InspectPlayerTable();
	return CheckStatus(author_descriptor);
}

void InspectRequest::InspectPlayerTable() {
	inspecting_timer_.restart();
	for (; current_player_table_pos_ < player_table.size(); ++current_player_table_pos_) {
		if (IsTimeExpired()) {
			break;
		}
		const auto &player_index = player_table[current_player_table_pos_];
		if (IsAuthorPrivilegedForInspect(player_index)) {
			InspectIndex(player_index);
		}
	}
}

bool InspectRequest::IsTimeExpired() {
	auto delta = std::chrono::duration_cast<std::chrono::microseconds>(inspecting_timer_.delta()).count();
	return (delta > kOptUsec);
}

bool InspectRequest::IsAuthorPrivilegedForInspect(const PlayerIndexElement &index) const {
	return ((author_level_ >= kLvlImplementator) || ((index.level < author_level_) && (index.level < kLvlImmortal)));
}

void InspectRequest::InspectIndex(const PlayerIndexElement &index) {
	if (IsIndexMatched(index)) {
		ProcessMatchedIndex(index);
	}
}

void InspectRequest::ProcessMatchedIndex(const PlayerIndexElement &index) {
	++match_count_;
	PrintCharInfoToOutput(index);
	PrintOtherInspectInfoToOutput();
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
	result_.PageOutputToAuthor(author_descriptor);
}

void InspectRequest::PrintSearchStatisticsToOutput() {
	using namespace std::chrono_literals;
	result_.AddToOutput(fmt::format("Всего найдено: {} за {:.3f} сек.\r\n", match_count_, request_timer_.delta()/1.0s));
}

void InspectRequest::PrintCharInfoToOutput(const PlayerIndexElement &index) {
	PrintBaseCharInfoToOutput(index);
	PrintExtraCharInfoToOutput(index);
}

void InspectRequest::PrintBaseCharInfoToOutput(const PlayerIndexElement &index) {
	DescriptorData *d_vict = DescriptorByUid(index.unique);
	result_.AddToOutput(fmt::format("{:-^121}\r\n", "*"));
	result_.AddToOutput(fmt::format("Name: {}{:<12}{} Mail: {:<30} Last: {:%e-%b-%Y} from {}, Lvl {}, Rmt {}, Cls: {}",
						   (d_vict ? KIGRN : KIWHT), index.name(), KNRM,
						   index.mail, std::chrono::system_clock::from_time_t(index.last_logon), index.last_ip,
						   index.level, index.remorts, MUD::Class(index.plr_class).GetName()));
}

void InspectRequest::PrintExtraCharInfoToOutput(const PlayerIndexElement &index) {
	auto player_ptr =  GetCharPtr(index);
	if (player_ptr) {
		PrintClanAbbrevToOutput(player_ptr);
		PrintPunishmentsInfoToOutput(player_ptr);
	} else {
		result_.AddToOutput(".\r\n");
	}
}

CharData::shared_ptr InspectRequest::GetCharPtr(const PlayerIndexElement &index) {
	auto d_vict = DescriptorByUid(index.unique);
	if (d_vict) {
		return d_vict->character;
	} else {
		auto player_ptr = std::make_shared<Player>();
		if (load_char(index.name(), player_ptr.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
			return player_ptr;
		}
	}
	return {};
}

void InspectRequest::PrintClanAbbrevToOutput(const std::shared_ptr<CharData> &player) {
	Clan::SetClanData(player.get());
	if (CLAN(player)) {
		result_.AddToOutput(fmt::format(", Clan: {}.\r\n", (player)->player_specials->clan->GetAbbrev()));
	} else {
		result_.AddToOutput(", Clan: нет.\r\n");
	}
}

void InspectRequest::PrintPunishmentsInfoToOutput(std::shared_ptr<CharData> &player) {
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
		result_.AddToOutput(fmt::format(" {}{}{} until {:%R %e-%b-%Y} [{}].\r\n",
							   KIRED, punish_text, KNRM,
							   std::chrono::system_clock::from_time_t(punish.duration),
							   (punish.reason ? punish.reason : "-")));
	}
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
	result_.AddToOutput(fmt::format("Inspecting IP (last logon): {}{}{}\r\n", KWHT, GetRequestText(), KNRM));
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
	result_.AddToOutput(fmt::format("Inspecting e-mail: {}{}{}\r\n", KWHT, GetRequestText(), KNRM));
	send_mail_ = ((args.size() > kMinArgsNumber) && (args.back() == "send_mail"));
}

bool InspectRequestMail::IsIndexMatched(const PlayerIndexElement &index) {
	return (index.mail && strstr(index.mail, GetRequestText().c_str()));
}

void InspectRequestMail::OutputResult(DescriptorData *author_descriptor) {
	PrintSearchStatisticsToOutput();
	if (send_mail_ && GetMatchCount() > 0) {
		result_.MailOutputTo(GetRequestText());
		result_.AddToOutput("Данный список отправлен игроку на емайл\r\n");
	}
	result_.PageOutputToAuthor(author_descriptor);
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
	result_.AddToOutput(fmt::format("Incpecting character (e-mail or last IP): {}{}{}. E-mail: {} Last IP: {}\r\n",
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
  std::set<std::string> victim_ip_log_;
  std::ostringstream logon_buffer_;        // записи о совпавших коннектах проверяемого персонажа

  void NoteVictimInfo(const CharData::shared_ptr &vict);
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

	const auto vict_table_pos = player_table.get_by_name(GetRequestText().c_str());
	const auto &vict_index = player_table[vict_table_pos];
	auto vict_ptr = GetCharPtr(vict_index);
	if (vict_ptr) {
		NoteVictimInfo(vict_ptr);
	} else {
		throw std::runtime_error(fmt::format("Inspecting all: не удалось загрузить файл персонажа '{}'.\r\n",
											 GetRequestText()));
	}
	// Forced processing so that victim info was on the top of inpect results.
	ProcessMatchedIndex(vict_index);
}

void InspectRequestAll::NoteVictimInfo(const CharData::shared_ptr &vict) {
	result_.AddToOutput(fmt::format("Inspecting all (IP intersection): {}{}{}\r\n", KIWHT, GetRequestText(), KNRM));
	vict_uid_ = vict->get_uid();
	for (const auto &logon : LOGON_LIST(vict)) {
		if (logon.ip && !kIgnoredIpChecklist.contains(logon.ip)) {
			victim_ip_log_.insert(logon.ip);
		}
	}
}

void InspectRequestAll::PrintOtherInspectInfoToOutput() {
	result_.AddToOutput(logon_buffer_.str());
	logon_buffer_.str("");
	logon_buffer_.clear();
}

bool InspectRequestAll::IsIndexMatched(const PlayerIndexElement &index) {
	if (vict_uid_ == index.unique) {
		return false;
	}
	auto player_ptr = GetCharPtr(index);
	if (player_ptr) {
		return InspectLogonList(player_ptr);
	} else {
		return false;
	}
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
	auto predicate = [ch](const auto &p) { return (ch->get_uid() == p->GetAuthorUid()); };
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
