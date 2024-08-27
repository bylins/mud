#include "inspect.h"

#include <memory>
#include <iostream>

#include "house.h"
#include "entities/char_player.h"
#include "structs/global_objects.h"
#include "color.h"
#include "modify.h"
#include "fmt/format.h"

const int kMaxRequestLength{65};
const int kMinRequestLength{3};
const int kMinArgsNumber{2};
const int kRequestTypePos{0};
const int kRequestTextPos{1};

extern bool need_warn;

char *PrintPunishmentTime(time_t time);
void SendListChar(const std::ostringstream &list_char, const std::string &email);
bool IsTimeout(timeval &start_time);
bool IsAuthorLevelPoor(CharData *author, const PlayerIndexElement &index);
bool IsRequestAlreadySent(CharData *ch);
bool IsArgsIncorrect(CharData *ch, std::vector<std::string> &args);
void CreateRequest(CharData *ch, std::vector<std::string> &args);

class InspectRequest {
 public:
  explicit InspectRequest(CharData *author, std::vector<std::string> &args);
  virtual ~InspectRequest() = default;
  void Inspect();
  bool IsFinished() const { return finished_; };

 protected:
  bool finished_{false};
  int found_{0};                        // сколько всего найдено
    std::string request_text_;
  std::ostringstream output_;            // буфер в который накапливается вывод
  struct timeval start_{};                // время когда запустили запрос

  virtual void SendInspectResult(DescriptorData *author_descriptor);
  void PageOutputToAuthor(DescriptorData *author_descriptor) const;
  void PrintFindStatToOutput();
  void PrintCharInfoToOutput(const PlayerIndexElement &index);
  bool IsPlayerFound(std::shared_ptr<CharData> &author, const PlayerIndexElement &index);
  virtual bool IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) = 0;

 private:
  int author_uid_{0};
  std::size_t player_table_pos_{0};            // текущая позиция поиска

  virtual void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) = 0;
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
	if (finished_) {
		return;
	}

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

	need_warn = false;
	timeval start_time{};
	gettimeofday(&start_time, nullptr);
	for (; player_table_pos_ < player_table.size(); ++player_table_pos_) {
#ifdef TEST_BUILD
		log("inspecting %d/%lu", 1 + player_table_pos_, player_table.size());
#endif
		if (IsTimeout(start_time)) {
			return;
		}
		const auto &player_index = player_table[player_table_pos_];
		if (IsAuthorLevelPoor(author.get(), player_index)) {
			continue;
		}
		InspectIndex(author, player_index);
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

bool IsAuthorLevelPoor(CharData *author, const PlayerIndexElement &index) {
	return ((index.level >= kLvlImmortal && !IS_GRGOD(author))
		|| (index.level > GetRealLevel(author) && !IS_IMPL(author)));
}

void InspectRequest::SendInspectResult(DescriptorData *author_descriptor) {
	PrintFindStatToOutput();
	PageOutputToAuthor(author_descriptor);
}

void InspectRequest::PrintFindStatToOutput() {
	need_warn = true;
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
	if (load_char(index.name(), player.get()) > -1) {
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
		PrintSinglePunishmentInfoToOutput("FREEZE", player->player_specials->pfreeze);
	}
	if (PLR_FLAGGED(player, EPlrFlag::kMuted)) {
		PrintSinglePunishmentInfoToOutput("MUTE", player->player_specials->pmute);
	}
	if (PLR_FLAGGED(player, EPlrFlag::kDumbed)) {
		PrintSinglePunishmentInfoToOutput("DUMB", player->player_specials->pdumb);
	}
	if (PLR_FLAGGED(player, EPlrFlag::kHelled)) {
		PrintSinglePunishmentInfoToOutput("HELL", player->player_specials->phell);
	}
	if (!PLR_FLAGGED(player, EPlrFlag::kRegistred)) {
		PrintSinglePunishmentInfoToOutput("UNREG", player->player_specials->punreg);
	}
}

void InspectRequest::PrintSinglePunishmentInfoToOutput(std::string_view name, const Punish &punish) {
	if (punish.duration) {
		const std::string_view output_format("{:<6}: {} [{}].\r\n");
		output_ << fmt::format(fmt::runtime(output_format),
							   name,
							   PrintPunishmentTime(punish.duration - time(nullptr)),
							   punish.reason ? punish.reason : "-");
	}
}

char *PrintPunishmentTime(time_t time) {
	static char time_buf[16];
	time_buf[0] = '\0';
	if (time < 3600) {
		snprintf(time_buf, sizeof(time_buf), "%d m", (int) time / 60);
	} else if (time < 3600 * 24) {
		snprintf(time_buf, sizeof(time_buf), "%d h", (int) time / 3600);
	} else if (time < 3600 * 24 * 30) {
		snprintf(time_buf, sizeof(time_buf), "%d D", (int) time / (3600 * 24));
	} else if (time < 3600 * 24 * 365) {
		snprintf(time_buf, sizeof(time_buf), "%d M", (int) time / (3600 * 24 * 30));
	} else {
		snprintf(time_buf, sizeof(time_buf), "%d Y", (int) time / (3600 * 24 * 365));
	}
	return time_buf;
}

bool InspectRequest::IsPlayerFound(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	auto result = IsMatchedRequest(author, index);
	found_ += result;
	return result;
}

// =======================================  INSPECT IP  ============================================

class InspectRequestIp : public InspectRequest {
 public:
  explicit InspectRequestIp(CharData *author, std::vector<std::string> &args);

 private:
  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
  bool IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
};

InspectRequestIp::InspectRequestIp(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	output_ << fmt::format("Inspecting IP: {}{}{}\r\n", KWHT, args[kRequestTextPos], KNRM);
}

void InspectRequestIp::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (IsPlayerFound(author, index)) {
		PrintCharInfoToOutput(index);
	}
}

bool InspectRequestIp::IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (index.last_ip && !IsAuthorLevelPoor(author.get(), index)) {
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

  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
  bool IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
  void SendInspectResult(DescriptorData *author_descriptor) final;
  void SendEmail();
};

InspectRequestMail::InspectRequestMail(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	output_ << fmt::format("Inspecting e-mail: {}{}{}\r\n", KWHT, args[kRequestTextPos], KNRM);
	if (args.size() > kMinArgsNumber) {
		if (isname(args.back(), "send_mail")) {
			send_mail_ = true;
		}
	}
}

void InspectRequestMail::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (IsPlayerFound(author, index)) {
		PrintCharInfoToOutput(index);
	}
}

bool InspectRequestMail::IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (index.mail && !IsAuthorLevelPoor(author.get(), index)) {
		return (strstr(index.mail, request_text_.c_str()));
	}
	return false;
}

void InspectRequestMail::SendInspectResult(DescriptorData *author_descriptor) {
	PrintFindStatToOutput();
	SendEmail();
	PageOutputToAuthor(author_descriptor);
}

void InspectRequestMail::SendEmail() {
	if (send_mail_ && found_ > 1) {
		SendListChar(output_, request_text_);
		output_ << "Данный список отправлен игроку на емайл\r\n";
	}
}

void SendListChar(const std::ostringstream &list_char, const std::string &email) {
	std::string cmd_line = "python3 SendListChar.py " + email + " " + list_char.str() + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// =======================================  INSPECT CHAR  ============================================

class InspectRequestChar : public InspectRequest {
 public:
  explicit InspectRequestChar(CharData *author, std::vector<std::string> &args);

 private:
  int vict_uid_{0};
  std::string mail_;
  std::string last_ip_;

  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
  bool IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
};

InspectRequestChar::InspectRequestChar(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	const auto victim_name = args[kRequestTextPos];
	vict_uid_ = GetUniqueByName(victim_name);
	if (vict_uid_ <= 0) {
		SendMsgToChar(author, "Неизвестное имя персонажа (%s) Inspecting char.\r\n", victim_name.c_str());
		finished_ = true;
	} else {
		auto player_pos = GetPtableByUnique(vict_uid_);
		const auto &player_index = player_table[player_pos];
		if (IsAuthorLevelPoor(author, player_index)) {
			SendMsgToChar("А ежели он вам молнией по тыковке?.\r\n", author);
			finished_ = true;
		} else {
			mail_ = player_index.mail;
			last_ip_ = player_index.last_ip;
			output_ << fmt::format("Incpecting character: {}{}{}. E-mail: {} Last IP: {}\r\n",
								   KWHT, args[kRequestTextPos], KNRM, mail_, last_ip_);
			PrintCharInfoToOutput(player_index);
		}
	}
}

void InspectRequestChar::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (IsPlayerFound(author, index)) {
		PrintCharInfoToOutput(index);
	}
}

bool InspectRequestChar::IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (vict_uid_ == index.unique || IsAuthorLevelPoor(author.get(), index)) {
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
  std::vector<Logon> ip_log_;            // айпи адреса по которым идет поиск

  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
  bool IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
};

InspectRequestAll::InspectRequestAll(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	output_ << fmt::format("All: {}&S{}&s{}\r\n", CCWHT(author, C_SPR), args[kRequestTextPos], CCNRM(author, C_SPR));
	ip_log_.emplace_back(Logon{str_dup(args[kRequestTextPos].c_str()), 0, 0, false});
}

void InspectRequestAll::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (IsPlayerFound(author, index)) {
		PrintCharInfoToOutput(index);
	}
}

bool InspectRequestAll::IsMatchedRequest(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	return false;
}

// ========================================================================

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_pfilepos() < 0) {
		return;
	}

	if (IsRequestAlreadySent(ch)) {
		return;
	}

	std::vector<std::string> args;
	array_argument(argument, args);
	if (IsArgsIncorrect(ch, args)) {
		return;
	}

	CreateRequest(ch, args);
}

bool IsRequestAlreadySent(CharData *ch) {
	auto it_inspect = MUD::inspect_list().find(ch->get_uid());
	auto it_setall = setall_inspect_list.find(ch->get_uid());
	if (it_inspect != MUD::inspect_list().end() || it_setall != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return true;
	}
	return false;
}

bool IsArgsIncorrect(CharData *ch, std::vector<std::string> &args) {
	if (args.size() < kMinArgsNumber) {
		SendMsgToChar("Usage: inspect { mail | ip | char | all } <argument> [send_mail]\r\n", ch);
		return true;
	}
	if (!isname(args[kRequestTypePos], "mail ip char all")) {
		SendMsgToChar("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return true;
	}

	auto &request_text = args[kRequestTextPos];
	if (request_text.length() < kMinRequestLength) {
		SendMsgToChar("Слишком короткий запрос.\r\n", ch);
		return true;
	}
	if (request_text.length() > kMaxRequestLength) {
		SendMsgToChar("Слишком длинный запрос.\r\n", ch);
		return true;
	}
	if (utils::IsAbbr(request_text.c_str(), "char") && (GetUniqueByName(request_text) <= 0)) {
		auto msg = fmt::format("Некорректное имя персонажа ({}) Inspecting char.\r\n", request_text);
		SendMsgToChar(msg, ch);
		return true;
	}
	return false;
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
		if (!IS_GRGOD(ch)) {
			SendMsgToChar("Вы не столь божественны, как вам кажется.\r\n", ch);
			return;
		}
		MUD::inspect_list().emplace(ch->get_pfilepos(), std::make_shared<InspectRequestAll>(ch, args));
	}
	SendMsgToChar("Запрос создан, ожидайте результата...\r\n", ch);
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
