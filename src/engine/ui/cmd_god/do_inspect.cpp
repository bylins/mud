#include "do_inspect.h"

#include <memory>
#include <iostream>

#include "gameplay/clans/house.h"
#include "engine/entities/char_player.h"
#include "engine/db/global_objects.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "fmt/format.h"
#include "fmt/chrono.h"
#include "utils/utils_time.h"
#include "engine/db/player_index.h"

const int kMaxRequestLength{65};
const int kMinRequestLength{3};
const int kMinArgsNumber{2};
const int kRequestKindPos{0};
const int kRequestTextPos{1};

const char *kUndefined{"undefined"};
const std::set<std::string_view> kIgnoredIpChecklist = {"135.181.219.76"};

const PlayerIndexElement &GetCharIndex(std::string_view char_name) {
	std::string name {char_name};

	auto vict_uid = GetUniqueByName(name);
	if (vict_uid <= 0) {
		throw std::runtime_error(fmt::format("Неизвестное имя персонажа '{}'.\r\n", char_name));
	} else {
		auto player_pos = GetPtableByUnique(vict_uid);
		return player_table[player_pos];
	}
}

CharData::shared_ptr GetCharPtr(const PlayerIndexElement &index) {
	auto d_vict = DescriptorByUid(index.uid());
	if (d_vict) {
		return d_vict->character;
	} else {
		auto player_ptr = std::make_shared<Player>();
		if (LoadPlayerCharacter(index.name().c_str(), player_ptr.get(), ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) > -1) {
			return player_ptr;
		}
	}
	auto char_name = index.name();
	const auto error_msg = fmt::format("Не удалось загрузить файл персонажа '{}'.\r\n",
				(char_name.empty() ? "unknown" : char_name));
	mudlog(error_msg, DEF, kLvlImplementator, ERRLOG, true);
	return {};
}

void ClearBuffer(std::ostringstream &stream) {
	stream.str("");
	stream.clear();
}

void MailTextTo(std::string_view address, std::string_view text) {
	std::string cmd_line = fmt::format("python3 MailTextTo.py {} {} &", address, text);
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// ==================================  EXTRACTED CHARACTER INFO  ========================================

class ExtractedCharacterInfo {
 public:
  void ExtractDataFromIndex(const PlayerIndexElement &index);
  void ExtractDataFromPtr(const CharData::shared_ptr &player_ptr);
  void AddExtraInfo(std::string_view str) { extra_info_ << str; };
  void PrintReport(std::ostringstream &report);
  void Clear();
  bool IsPresent() { return !name_.empty(); };

 private:
  bool online_{false};
  int level_{0};
  int remort_{0};
  int last_logon_time_{0};
  std::string clan_abbrev_;
  std::string class_name_;
  std::string name_;
  std::string_view mail_;
  std::string_view last_ip_;
  std::ostringstream punishments_;
  std::ostringstream extra_info_;

  void ExtractClanAbbrev(const CharData::shared_ptr &player_ptr);
  void ExtractPunishmenstsInfo(const CharData::shared_ptr &player_ptr);
  void ExtractPunishmentInfo(std::string_view punish_text, const punishments::Punish &punish);
  void PrintBaseInfo(std::ostringstream &report);
  void PrintPunishmentsInfo(std::ostringstream &report) const;
  void PrintExtraInfo(std::ostringstream &report) const;
  static void PrintSeparator(std::ostringstream &report);
};

void ExtractedCharacterInfo::ExtractDataFromIndex(const PlayerIndexElement &index) {
	online_ = (DescriptorByUid(index.uid()) != nullptr);
	name_ = (index.name().empty() ? kUndefined : index.name());
	mail_ = (index.mail ? index.mail : kUndefined);
	last_ip_ = (index.last_ip ? index.last_ip : kUndefined);
	class_name_ = MUD::Class(index.plr_class).GetName();
	level_ = index.level;
	remort_ = index.remorts;
	last_logon_time_ = index.last_logon;
}

void ExtractedCharacterInfo::ExtractDataFromPtr(const CharData::shared_ptr &player_ptr) {
	ExtractClanAbbrev(player_ptr);
	ExtractPunishmenstsInfo(player_ptr);
}

void ExtractedCharacterInfo::ExtractClanAbbrev(const CharData::shared_ptr &player_ptr) {
	Clan::SetClanData(player_ptr.get());
	const auto clan = player_ptr->player_specials->clan;

	if (clan) {
		clan_abbrev_ = clan->GetAbbrev();
	} else {
		clan_abbrev_ = "нет";
	}
}

void ExtractedCharacterInfo::ExtractPunishmenstsInfo(const CharData::shared_ptr &player_ptr) {
	if (player_ptr->IsFlagged(EPlrFlag::kFrozen)) {
		ExtractPunishmentInfo("FROZEN", player_ptr->player_specials->pfreeze);
	}
	if (player_ptr->IsFlagged(EPlrFlag::kMuted)) {
		ExtractPunishmentInfo("MUTED", player_ptr->player_specials->pmute);
	}
	if (player_ptr->IsFlagged(EPlrFlag::kDumbed)) {
		ExtractPunishmentInfo("DUMBED", player_ptr->player_specials->pdumb);
	}
	if (player_ptr->IsFlagged(EPlrFlag::kHelled)) {
		ExtractPunishmentInfo("HELLED", player_ptr->player_specials->phell);
	}
	if (!player_ptr->IsFlagged(EPlrFlag::kRegistred)) {
		ExtractPunishmentInfo("UNREGISTERED", player_ptr->player_specials->punreg);
	}
}

void ExtractedCharacterInfo::ExtractPunishmentInfo(std::string_view punish_text, const punishments::Punish &punish) {
	if (punish.duration) {
		punishments_ << fmt::format(" {}{}{} until {:%R %e-%b-%Y} [{}].\r\n",
									kColorBoldRed, punish_text, kColorNrm,
									std::chrono::system_clock::from_time_t(punish.duration),
									(punish.reason ? punish.reason : "-"));
	}
}

void ExtractedCharacterInfo::PrintReport(std::ostringstream &report) {
	PrintSeparator(report);
	PrintBaseInfo(report);
	PrintPunishmentsInfo(report);
	PrintExtraInfo(report);
}

void ExtractedCharacterInfo::PrintSeparator(std::ostringstream &report) {
	report << fmt::format("{:-^132}\r\n", "");
}

void ExtractedCharacterInfo::PrintBaseInfo(std::ostringstream &report) {
	report << fmt::format(
		"Name: {color_on}{name:<12}{color_off} Mail: {mail:<30} Last: {time:%e-%b-%Y} from {ip}, Lvl {level}, Rmt {remort}, Cls: {class}, Clan: {clan}.\r\n",
		fmt::arg("color_on", (online_ ? kColorBoldGrn : kColorBoldWht)),
		fmt::arg("name", name_),
		fmt::arg("color_off", kColorNrm),
		fmt::arg("mail", mail_),
		fmt::arg("time", std::chrono::system_clock::from_time_t(last_logon_time_)),
		fmt::arg("ip", last_ip_),
		fmt::arg("level", level_),
		fmt::arg("remort", remort_),
		fmt::arg("class", class_name_),
		fmt::arg("clan", clan_abbrev_));
}

void ExtractedCharacterInfo::PrintPunishmentsInfo(std::ostringstream &report) const {
	report << punishments_.str();
}

void ExtractedCharacterInfo::PrintExtraInfo(std::ostringstream &report) const {
	report << extra_info_.str();
}

void ExtractedCharacterInfo::Clear() {
	online_ = false;
	last_logon_time_ = 0;
	level_ = 0;
	remort_ = 0;
	name_.clear();
	mail_ = kUndefined;
	last_ip_ = kUndefined;
	class_name_.clear();
	clan_abbrev_.clear();
	ClearBuffer(punishments_);
	ClearBuffer(extra_info_);
}

// =======================================  REPORT GENERATOR  ============================================

class ReportGenerator {
 public:
  void SetReportHeader(std::string_view header) { report_header_ = header; };
  void SetDestinationEmail(std::string_view email) { destination_email_ = email; };
  void AddCharacterExtraInfo(std::string_view str) { current_char_info_.AddExtraInfo(str); };
  void ExtractDataFromIndex(const PlayerIndexElement &index);
  void ExtractDataFromPtr(const CharData::shared_ptr &player_ptr);
  void GenerateCharacterReport();
  void GenerateSummary(std::ostringstream &summary);
  void OutputSummary(std::ostringstream &summary, DescriptorData *descriptor);
  void ProcessLoadingCharError();

 private:
  int character_reports_count_{0};
  std::string report_header_;
  std::string destination_email_;
  std::ostringstream character_reports_;
  utils::CExecutionTimer request_timer_;
  ExtractedCharacterInfo current_char_info_;

  void PrintHeader(std::ostringstream &stream);
  void PrintCharacterReports(std::ostringstream &stream);
  void PrintSearchStatistics(std::ostringstream &stream);
};

void ReportGenerator::ExtractDataFromIndex(const PlayerIndexElement &index) {
	current_char_info_.ExtractDataFromIndex(index);
}

void ReportGenerator::ExtractDataFromPtr(const CharData::shared_ptr &player_ptr) {
	current_char_info_.ExtractDataFromPtr(player_ptr);
}

void ReportGenerator::GenerateCharacterReport() {
	if (current_char_info_.IsPresent()) {
		++character_reports_count_;
		current_char_info_.PrintReport(character_reports_);
	} else {
		mudlog("Inspecting: попытка сгенерировать отчёт для персонажа без собранной информации.",
			   DEF,
			   kLvlImplementator,
			   ERRLOG,
			   true);
	}
	current_char_info_.Clear();
}

void ReportGenerator::GenerateSummary(std::ostringstream &summary) {
	PrintHeader(summary);
	PrintCharacterReports(summary);
	PrintSearchStatistics(summary);
}

void ReportGenerator::PrintHeader(std::ostringstream &stream) {
	stream << report_header_;
}

void ReportGenerator::PrintCharacterReports(std::ostringstream &stream) {
	stream << character_reports_.str();
}

void ReportGenerator::PrintSearchStatistics(std::ostringstream &stream) {
	using namespace std::chrono_literals;
	stream << fmt::format("\r\nВсего найдено: {} за {:.3f} сек.\r\n",
						  character_reports_count_,
						  request_timer_.delta() / 1.0s);
}

void ReportGenerator::OutputSummary(std::ostringstream &summary, DescriptorData *descriptor) {
	if (!destination_email_.empty() && character_reports_count_ > 0) {
		MailTextTo(destination_email_, summary.str());
		summary << "Данный список отправлен игроку на емайл\r\n";
	}
	page_string(descriptor, summary.str());
}

void ReportGenerator::ProcessLoadingCharError() {
	current_char_info_.Clear();
}

// =======================================  INSPECT REQUEST INTERFACE  ============================================

class InspectRequest {
 public:
  InspectRequest() = delete;
  InspectRequest(const InspectRequest &) = delete;
  InspectRequest(InspectRequest &&) = delete;
  InspectRequest &operator=(const InspectRequest &) = delete;
  InspectRequest &operator=(InspectRequest &&) = delete;
  virtual ~InspectRequest() = default;
  long GetAuthorUid() const { return author_uid_; }
  bool IsActive() { return status_; };
  void Execute();

 protected:
  enum Status { kFinished = false, kProcessing = true };
  Status status_{kProcessing};
  ReportGenerator report_generator_;

  explicit InspectRequest(const CharData *author, const std::vector<std::string> &args);
  bool IsAuthorPrivilegedForInspect(const PlayerIndexElement &index) const;
  std::string_view GetRequestText() const { return request_text_; };

  virtual bool IsIndexMatched(const PlayerIndexElement &index) = 0;
  virtual void ProcessMatchedIndex(const PlayerIndexElement &index);

 private:
  long author_uid_{0};
  int author_level_{0};
  std::size_t current_player_table_pos_{0};
  std::string request_text_;
  utils::CExecutionTimer inspecting_timer_;

  void InspectPlayersTable();
  void InspectIndex(const PlayerIndexElement &index);
  void CheckCompletion(DescriptorData *author);
  bool IsTimeExpired();
};

InspectRequest::InspectRequest(const CharData *author, const std::vector<std::string> &args)
	: author_uid_(author->get_uid()),
	  author_level_(author->GetLevel()),
	  request_text_(args[kRequestTextPos]) {}

void InspectRequest::Execute() {
	DescriptorData *author_descriptor = DescriptorByUid(author_uid_);
	if (!author_descriptor || (author_descriptor->state != EConState::kPlaying)) {
		status_ = kFinished;
		return;
	}

	InspectPlayersTable();
	CheckCompletion(author_descriptor);
}

void InspectRequest::InspectPlayersTable() {
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
	report_generator_.ExtractDataFromIndex(index);
	auto player_ptr = GetCharPtr(index);
	if (player_ptr) {
		report_generator_.ExtractDataFromPtr(player_ptr);
		report_generator_.GenerateCharacterReport();
	} else {
		report_generator_.ProcessLoadingCharError();
	}
}

void InspectRequest::CheckCompletion(DescriptorData *author) {
	if (current_player_table_pos_ >= player_table.size()) {
		std::ostringstream summary;
		report_generator_.GenerateSummary(summary);
		report_generator_.OutputSummary(summary, author);
		status_ = kFinished;
	}
}

// =======================================  INSPECT IP  ============================================

class InspectRequestIp : public InspectRequest {
 public:
  explicit InspectRequestIp(const CharData *author, const std::vector<std::string> &args);

 private:
  bool IsIndexMatched(const PlayerIndexElement &index) final;
};

InspectRequestIp::InspectRequestIp(const CharData *author, const std::vector<std::string> &args)
	: InspectRequest(author, args) {
	report_generator_.SetReportHeader(fmt::format("Inspecting IP (last logon): {}{}{}\r\n",
												  kColorWht,
												  GetRequestText(),
												  kColorNrm));
}

bool InspectRequestIp::IsIndexMatched(const PlayerIndexElement &index) {
	return (index.last_ip && strstr(index.last_ip, GetRequestText().data()));
}

// =======================================  INSPECT MAIL  ============================================

class InspectRequestMail : public InspectRequest {
 public:
  explicit InspectRequestMail(const CharData *author, const std::vector<std::string> &args);

 private:
  bool IsIndexMatched(const PlayerIndexElement &index) final;
};

InspectRequestMail::InspectRequestMail(const CharData *author, const std::vector<std::string> &args)
	: InspectRequest(author, args) {
	report_generator_.SetReportHeader(fmt::format("Inspecting e-mail: {}{}{}\r\n", kColorWht, GetRequestText(), kColorNrm));
	if ((args.size() > kMinArgsNumber) && (args.back() == "send_mail")) {
		report_generator_.SetDestinationEmail(GetRequestText());
	}
}

bool InspectRequestMail::IsIndexMatched(const PlayerIndexElement &index) {
	return (index.mail && strstr(index.mail, GetRequestText().data()));
}

// =======================================  INSPECT CHAR  ============================================

class InspectRequestChar : public InspectRequest {
 public:
  explicit InspectRequestChar(const CharData *author, const std::vector<std::string> &args);

 private:
  std::string_view mail_;
  std::string_view last_ip_;

  void NoteVictimInfo(const PlayerIndexElement &index);
  bool IsIndexMatched(const PlayerIndexElement &index) final;
};

InspectRequestChar::InspectRequestChar(const CharData *author, const std::vector<std::string> &args)
	: InspectRequest(author, args) {
	const auto &player_index = GetCharIndex(GetRequestText());
	if (IsAuthorPrivilegedForInspect(player_index)) {
		NoteVictimInfo(player_index);
	} else {
		status_ = kFinished;
		SendMsgToChar("А ежели он ваc молнией по тыковке?\r\n", author);
	}
}

void InspectRequestChar::NoteVictimInfo(const PlayerIndexElement &index) {
	mail_ = (index.mail ? index.mail : kUndefined);
	last_ip_ = (index.last_ip ? index.last_ip : kUndefined);
	report_generator_.SetReportHeader(fmt::format(
		"Incpecting character (e-mail or last IP): {}{}{}. E-mail: {} Last IP: {}\r\n",
		kColorWht,
		GetRequestText(),
		kColorNrm,
		mail_,
		last_ip_));
}

bool InspectRequestChar::IsIndexMatched(const PlayerIndexElement &index) {
	return ((index.mail && (mail_ == index.mail)) || (index.last_ip && (last_ip_ == index.last_ip)));
}

// =======================================  INSPECT ALL  ============================================

class InspectRequestAll : public InspectRequest {
 public:
  explicit InspectRequestAll(const CharData *author, const std::vector<std::string> &args);

 private:
  long vict_uid_{0};
  std::set<std::string> victim_ip_log_;
  std::ostringstream current_char_intercesting_logons_;

  void NoteVictimInfo(const CharData::shared_ptr &vict);
  bool IsIpMatched(const char *ip);
  bool IsLogonsIntersect(const CharData::shared_ptr &player);
  void NoteLogonInfo(const network::Logon &logon);
  void FlushLogonsBufferToReportGenerator();

  bool IsIndexMatched(const PlayerIndexElement &index) final;
  void ProcessMatchedIndex(const PlayerIndexElement &index) final;
  void ForceGenerateReport(const PlayerIndexElement &index, const CharData::shared_ptr &vict);
};

InspectRequestAll::InspectRequestAll(const CharData *author, const std::vector<std::string> &args)
	: InspectRequest(author, args) {
	if (!IS_GRGOD(author)) {
		SendMsgToChar("Вы не столь божественны, как вам кажется.\r\n", author);
		status_ = kFinished;
		return;
	}
	const auto &vict_index = GetCharIndex(GetRequestText());
	const auto vict_ptr = GetCharPtr(vict_index);
	if (vict_ptr) {
		NoteVictimInfo(vict_ptr);
		ForceGenerateReport(vict_index, vict_ptr);
	} else {
		SendMsgToChar(fmt::format("Не удалось загрузить файл персонажа '{}'.\r\n", GetRequestText()), author);
		status_ = kFinished;
	}
}

void InspectRequestAll::NoteVictimInfo(const CharData::shared_ptr &vict) {
	report_generator_.SetReportHeader(fmt::format("Inspecting all (IP intersection): {}{}{}\r\n",
												  kColorBoldWht,
												  GetRequestText(),
												  kColorNrm));
	vict_uid_ = vict->get_uid();
	for (const auto &logon : LOGON_LIST(vict)) {
		if (logon.ip && !kIgnoredIpChecklist.contains(logon.ip)) {
			victim_ip_log_.insert(logon.ip);
		}
	}
}

void InspectRequestAll::ForceGenerateReport(const PlayerIndexElement &index, const CharData::shared_ptr &vict) {
	report_generator_.ExtractDataFromIndex(index);
	report_generator_.ExtractDataFromPtr(vict);
	report_generator_.GenerateCharacterReport();
}

void InspectRequestAll::ProcessMatchedIndex(const PlayerIndexElement &index) {
	report_generator_.ExtractDataFromIndex(index);
	FlushLogonsBufferToReportGenerator();
	report_generator_.GenerateCharacterReport();
}

void InspectRequestAll::FlushLogonsBufferToReportGenerator() {
	report_generator_.AddCharacterExtraInfo(current_char_intercesting_logons_.str());
	ClearBuffer(current_char_intercesting_logons_);
}

bool InspectRequestAll::IsIndexMatched(const PlayerIndexElement &index) {
	if (vict_uid_ == index.uid()) {
		return false;
	}

	const auto player_ptr = GetCharPtr(index);
	if (player_ptr && IsLogonsIntersect(player_ptr)) {
		report_generator_.ExtractDataFromPtr(player_ptr);
		return true;
	} else {
		report_generator_.ProcessLoadingCharError();
		return false;
	}
}

bool InspectRequestAll::IsLogonsIntersect(const CharData::shared_ptr &player) {
	bool result{false};
	for (const auto &logon : player->player_specials->logons) {
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

void InspectRequestAll::NoteLogonInfo(const network::Logon &logon) {
	current_char_intercesting_logons_
		<< fmt::format(" IP: {}{:<16}{} Logons: {}. Last: {:%R %e-%b-%Y}.\r\n",
					   kColorBoldCyn, logon.ip, kColorNrm, logon.count,
					   std::chrono::system_clock::from_time_t(logon.lasttime));
}

// ================================= IsExecuting Request Deque =======================================

void InspectRequestDeque::NewRequest(const CharData *ch, char *argument) {
	if (IsQueueAvailable(ch)) {
		std::vector<std::string> args;
		SplitArgument(argument, args);
		if (args.size() > 0 && IsArgsValid(ch, args)) {
			auto request_ptr = CreateRequest(ch, args);
			if (request_ptr) {
				push_back(request_ptr);
				SendMsgToChar("Запрос создан, ожидайте результата...\r\n", ch);
			}
		} else {
			SendMsgToChar("Usage: inspect { mail | ip | char | all } <argument>\r\n", ch);
		}
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

bool InspectRequestDeque::IsArgsValid(const CharData *ch, const std::vector<std::string> &args) {
	auto &request_text = args[kRequestTextPos];
	if (request_text.length() < kMinRequestLength) {
		SendMsgToChar("Слишком короткий запрос.\r\n", ch);
		return false;
	}
	if (request_text.length() > kMaxRequestLength) {
		SendMsgToChar("Слишком длинный запрос.\r\n", ch);
		return false;
	}
	if (args.size() < kMinArgsNumber) {
		std::string request_names;
		for (const auto &request_kind : request_kinds_) {
			request_names.append(fmt::format(" {} |", request_kind.first));
		}
		request_names.pop_back();
		SendMsgToChar(fmt::format("Usage: inspect {{{}}} <argument> [send_mail]\r\n", request_names), ch);
		return false;
	}
	if (!request_kinds_.contains(args[kRequestKindPos])) {
		SendMsgToChar("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return false;
	}

	return true;
}

InspectRequestPtr InspectRequestDeque::CreateRequest(const CharData *ch, const std::vector<std::string> &args) {
	auto request_kind = request_kinds_.at(args[kRequestKindPos]);
	switch (request_kind) {
		case kMail: return std::make_shared<InspectRequestMail>(ch, args);
		case kIp: return std::make_shared<InspectRequestIp>(ch, args);
		case kChar: return std::make_shared<InspectRequestChar>(ch, args);
		case kAll: return std::make_shared<InspectRequestAll>(ch, args);
		default:
			SendMsgToChar(ch, "Неизвестный тип запроса.\r\n");
			return {};
			break;
	}
}

void InspectRequestDeque::Inspecting() {
	if (!empty()) {
		if (front()->IsActive()) {
			front()->Execute();
		} else {
			pop_front();
		}
	}
}

// ================================= Commands =======================================

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || ch->get_pfilepos() < 0) {
		return;
	}

	try {
		MUD::InspectRequests().NewRequest(ch, argument);
	} catch (std::runtime_error &e) {
		SendMsgToChar(ch, "%s", e.what());
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
