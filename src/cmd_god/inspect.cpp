#include "inspect.h"

#include <memory>
#include <iostream>

#include "house.h"
#include "entities/char_player.h"
#include "structs/global_objects.h"
#include "color.h"
#include "modify.h"
#include "fmt/format.h"

enum class EInspect {
  kIp = 1,
  kMail = 2,
  kChar = 3
};

const int kMaxRequestLength{65};
const int kMinRequestLength{3};
const int kMinArgsNumber{2};
const int kRequestTypePos{0};
const int kRequestTextPos{1};

extern bool need_warn;

void PrintPunishment(CharData *vict, std::ostringstream &out);
char *PrintPunishmentTime(int time);
void SendListChar(const std::ostringstream &list_char, const std::string &email);

class InspectRequest {
 public:
  explicit InspectRequest(CharData *author, std::vector<std::string> &args);
  virtual ~InspectRequest() = default;
  void Inspect();
  bool IsFinished() const { return finished_; };

  int found_{0};                        // сколько всего найдено
  bool send_mail_{false};                // отправлять ли на мыло список чаров
  std::ostringstream output_;            // буфер в который накапливается вывод
  struct timeval start_{};                // время когда запустили запрос для отладки

 protected:
  bool finished_{false};
  int vict_uid_{0};
  std::string request_text_;
  virtual void SendInspectResult();
  void PageOutputToAuthor() const;
  void PrintFindStatToOutput();

 private:
  int author_uid_{0};
  std::size_t player_table_pos_{0};            // текущая позиция поиска

  virtual void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) = 0;
};

InspectRequest::InspectRequest(CharData *author, std::vector<std::string> &args) {
	author_uid_ = author->get_uid();
	request_text_ = args[kRequestTextPos];
	if (args.size() > kMinArgsNumber) {
		if (isname(args.back(), "send_mail")) {
			send_mail_ = true;
		}
	}
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

	timeval start{}, stop{}, result{};
	need_warn = false;
	gettimeofday(&start, nullptr);
	for (; player_table_pos_ < player_table.size(); ++player_table_pos_) {
		gettimeofday(&stop, nullptr);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= kOptUsec) {
			return;
		}
#ifdef TEST_BUILD
		log("inspecting %d/%lu", 1 + player_table_pos_, player_table.size());
#endif
		const auto &inspected_index = player_table[player_table_pos_];
		//(request->search_for == EInspect::kChar && request->vict_uid == inspected_index.unique)
		if ((inspected_index.level >= kLvlImmortal && !IS_GRGOD(author))
			|| (inspected_index.level > GetRealLevel(author) && !IS_IMPL(author))) {
			continue;
		}
		InspectIndex(author, inspected_index);
	}

	if (player_table_pos_ == player_table.size()) {
		SendInspectResult();
		finished_ = true;
	}
}

void InspectRequest::SendInspectResult() {
	PrintFindStatToOutput();
	PageOutputToAuthor();
}

void InspectRequest::PageOutputToAuthor() const {
	DescriptorData *author_descriptor = DescriptorByUid(author_uid_);
	if (author_descriptor && (author_descriptor->connected == CON_PLAYING)) {
		page_string(author_descriptor, output_.str());
	}
}

void InspectRequest::PrintFindStatToOutput() {
	need_warn = true;
	timeval stop{}, result{};
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &start_);
	output_ << fmt::format("Всего найдено: {} за {}сек.\r\n", found_, result.tv_sec);
}

// =======================================  INSPECT IP  ============================================

class InspectRequestIp : public InspectRequest {
 public:
  explicit InspectRequestIp(CharData *author, std::vector<std::string> &args);

 private:
  std::vector<Logon> ip_log_;
  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
};

InspectRequestIp::InspectRequestIp(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	output_ << fmt::format("IP: {}&S{}&s{}\r\n", CCWHT(author, C_SPR), args[kRequestTextPos], CCNRM(author, C_SPR));
}

void InspectRequestIp::InspectIndex(std::shared_ptr<CharData> &/* author */, const PlayerIndexElement &index) {
	if (index.last_ip) {
		if (strstr(index.last_ip, request_text_.c_str())
		|| (!ip_log_.empty() && !str_cmp(index.last_ip, ip_log_.at(0).ip))) {
			output_ << fmt::format(" IP:{}{:<16}{}\r\n", KBLU, index.last_ip, KNRM);
		}
	}
}

// =======================================  INSPECT MAIL  ============================================

class InspectRequestMail : public InspectRequest {
 public:
  explicit InspectRequestMail(CharData *author, std::vector<std::string> &args);

 private:
  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
  void SendInspectResult() final;
  void SendEmail();
};

InspectRequestMail::InspectRequestMail(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	output_ << fmt::format("E-mail: {}&S{}&s{}\r\n", CCWHT(author, C_SPR), args[kRequestTextPos], CCNRM(author, C_SPR));
	request_text_ = args[kRequestTextPos];
}

void InspectRequestMail::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	if (index.mail) {
		if (strstr(index.mail, request_text_.c_str())) {
			++found_;
			DescriptorData *d_vict = DescriptorByUid(index.unique);
			time_t mytime = index.last_logon;
			output_ << fmt::format("{:*^80}\r\n", "*");
			output_ << fmt::format("Name: {}{:<12}{} E-mail: {:<30} Last: {}. Level {}, Remort {}, Class: {}",
				(d_vict ? CCGRN(author, C_SPR) : CCWHT(author, C_SPR)), index.name(), CCNRM(author, C_SPR),
				index.mail, rustime(localtime(&mytime)), index.level,
				index.remorts, MUD::Class(index.plr_class).GetName());

			Player target;
			if ((load_char(index.name(), &target)) > -1) {
				Clan::SetClanData(&target);
				if (CLAN(&target)) {
					output_ << fmt::format(", Clan: {}.\r\n", (&target)->player_specials->clan->GetAbbrev());
				} else {
					output_ << ", Clan: нет.\r\n";
				}
				PrintPunishment(&target, output_);
			}  else {
				output_ << ", Clan: unknown.\r\n";
			}
		}
	}
}

void InspectRequestMail::SendInspectResult() {
	PrintFindStatToOutput();
	SendEmail();
	PageOutputToAuthor();
}

void InspectRequestMail::SendEmail() {
	if (send_mail_ && found_ > 1) {
		output_ << "Данный список отправлен игроку на емайл\r\n";
		SendListChar(output_, request_text_);
	}
}

// =======================================  INSPECT CHAR  ============================================

class InspectRequestChar : public InspectRequest {
 public:
  explicit InspectRequestChar(CharData *author, std::vector<std::string> &args);

 private:
  std::string mail_;
  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
};

InspectRequestChar::InspectRequestChar(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	const auto victim_name = args[kRequestTextPos];
	vict_uid_ = GetUniqueByName(victim_name);
	auto player_pos = GetPtableByUnique(vict_uid_);
	const auto &player_index = player_table[player_pos];
	if ((vict_uid_ <= 0)
		|| (player_index.level >= kLvlImmortal && !IS_GRGOD(author))
		|| (player_index.level > GetRealLevel(author) && !IS_IMPL(author))) {
		SendMsgToChar(author, "Некорректное имя персонажа (%s) Inspecting char.\r\n", victim_name.c_str());
		finished_ = true;
	} else {
		output_
			<< fmt::format("Char: {}&S{}&s{}\r\n", CCWHT(author, C_SPR), args[kRequestTextPos], CCNRM(author, C_SPR));
		mail_ = player_index.mail;
	}
}

void InspectRequestChar::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
	finished_ = true;
}

// =======================================  INSPECT ALL  ============================================

class InspectRequestAll : public InspectRequest {
 public:
  explicit InspectRequestAll(CharData *author, std::vector<std::string> &args);

 private:
  std::vector<Logon> ip_log_;            // айпи адреса по которым идет поиск

  void InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) final;
};

InspectRequestAll::InspectRequestAll(CharData *author, std::vector<std::string> &args)
	: InspectRequest(author, args) {
	output_ << fmt::format("All: {}&S{}&s{}\r\n", CCWHT(author, C_SPR), args[kRequestTextPos], CCNRM(author, C_SPR));
	ip_log_.emplace_back(Logon{str_dup(args[kRequestTextPos].c_str()), 0, 0, false});
}

void InspectRequestAll::InspectIndex(std::shared_ptr<CharData> &author, const PlayerIndexElement &index) {
//	if (request->search_for != EInspect::kMail && request->full_search) {
//		if (d_vict) {
//			vict = d_vict->character;
//		} else {
//			vict = std::make_shared<Player>();
//			if (load_char(inspected_index.name(), vict.get()) < 0) {
//				auto msg = fmt::format("Некорректное имя персонажа ({}) Inspecting {}: {}.\r\n",
//									   inspected_index.name(),
//									   (request->search_for == EInspect::kMail ? "mail" :
//										(request->search_for == EInspect::kIp ? "ip" : "char")),
//									   request->request_text);
//				SendMsgToChar(msg, ch);
//				continue;
//			}
//		}
//	}
	finished_ = true;
}

// ========================================================================

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_pfilepos() < 0) {
		return;
	}

	auto it_inspect = MUD::inspect_list().find(ch->get_uid());
	auto it_setall = setall_inspect_list.find(ch->get_uid());
	if (it_inspect != MUD::inspect_list().end() && it_setall != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}

	std::vector<std::string> args;
	array_argument(argument, args);
	if (args.size() < kMinArgsNumber) {
		SendMsgToChar("Usage: inspect { mail | ip | char | all } <argument> [send_mail]\r\n", ch);
		return;
	}

	auto &request_type = args[kRequestTypePos];
	auto &request_text = args[kRequestTextPos];
	if (!isname(request_type, "mail ip char all")) {
		SendMsgToChar("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return;
	}
	if (request_text.length() < kMinRequestLength) {
		SendMsgToChar("Слишком короткий запрос.\r\n", ch);
		return;
	}
	if (request_text.length() > kMaxRequestLength) {
		SendMsgToChar("Слишком длинный запрос.\r\n", ch);
		return;
	}
	if (utils::IsAbbr(request_text.c_str(), "char") && (GetUniqueByName(request_text) <= 0)) {
		auto msg = fmt::format("Некорректное имя персонажа ({}) Inspecting char.\r\n", request_text);
		SendMsgToChar(msg, ch);
		return;
	}

	if (utils::IsAbbr(request_type.c_str(), "mail")) {
		InspReqPtr request = std::make_shared<InspectRequestMail>(ch, args);
		MUD::inspect_list()[ch->get_pfilepos()] = request;
	} else if (utils::IsAbbr(request_type.c_str(), "ip")) {
		InspReqPtr request = std::make_shared<InspectRequestIp>(ch, args);
		MUD::inspect_list()[ch->get_pfilepos()] = request;
	} else if (utils::IsAbbr(request_type.c_str(), "char")) {
		InspReqPtr request = std::make_shared<InspectRequestChar>(ch, args);
		MUD::inspect_list()[ch->get_pfilepos()] = request;
	} else if (utils::IsAbbr(request_type.c_str(), "all")) {
		if (!IS_GRGOD(ch)) {
			SendMsgToChar("Вы не столь божественны, как вам кажется.\r\n", ch);
			return;
		}
		InspReqPtr request = std::make_shared<InspectRequestAll>(ch, args);
		MUD::inspect_list()[ch->get_pfilepos()] = request;
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

//	for (; request->player_table_pos < static_cast<int>(player_table.size()); request->player_table_pos++) {
//		const auto &inspected_index = player_table[request->player_table_pos];
//		if ((request->search_for == EInspect::kChar && request->vict_uid == inspected_index.unique)
//			|| (inspected_index.level >= kLvlImmortal && !IS_GRGOD(ch))
//			|| (inspected_index.level > GetRealLevel(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))) {
//			continue;
//		}

//		if (request->search_for == EInspect::kIp || request->search_for == EInspect::kChar) {
//			if (!request->full_search) {
//				if (inspected_index.last_ip) {
//					if ((request->search_for == EInspect::kIp
//						&& strstr(inspected_index.last_ip, request->request_text.c_str()))
//						|| (!request->ip_log.empty()
//							&& !str_cmp(inspected_index.last_ip, request->ip_log.at(0).ip))) {
//						sprintf(buf1 + strlen(buf1),
//								" IP:%s%-16s%s\r\n",
//								(request->search_for == EInspect::kChar ? CCBLU(ch, C_SPR) : ""),
//								inspected_index.last_ip,
//								(request->search_for == EInspect::kChar ? CCNRM(ch, C_SPR) : ""));
//					}
//				}
//			} else if (vict && !LOGON_LIST(vict).empty()) {
//				for (const auto &cur_log : LOGON_LIST(vict)) {
//					for (const auto &ch_log : request->ip_log) {
//						if (!ch_log.ip) {
//							SendMsgToChar(ch, "Ошибка: пустой ip\r\n");
//							break;
//						}
//						if (str_cmp(cur_log.ip, "135.181.219.76")) { // игнорим bylins.online
//							if (!str_cmp(cur_log.ip, ch_log.ip)) {
//								sprintf(buf1 + strlen(buf1),
//										" IP:%s%-16s%s Количество входов с него:%5ld Последний раз: %-30s\r\n",
//										CCBLU(ch, C_SPR),
//										cur_log.ip,
//										CCNRM(ch, C_SPR),
//										cur_log.count,
//										rustime(localtime(&cur_log.lasttime)));
//							}
//						}
//					}
//				}
//			}
//		}
//	}
}

void PrintPunishment(CharData *vict, std::ostringstream &out) {
	if (PLR_FLAGGED(vict, EPlrFlag::kFrozen) && FREEZE_DURATION(vict)) {
		out << fmt::format("FREEZE : {} [{}].\r\n",
				PrintPunishmentTime(FREEZE_DURATION(vict) - time(nullptr)),
				FREEZE_REASON(vict) ? FREEZE_REASON(vict) : "-");
	}

	if (PLR_FLAGGED(vict, EPlrFlag::kMuted) && MUTE_DURATION(vict)) {
		out << fmt::format("MUTE   : {} [{}].\r\n",
				PrintPunishmentTime(MUTE_DURATION(vict) - time(nullptr)),
				MUTE_REASON(vict) ? MUTE_REASON(vict) : "-");
	}

	if (PLR_FLAGGED(vict, EPlrFlag::kDumbed) && DUMB_DURATION(vict)) {
		out << fmt::format("DUMB   : {} [{}].\r\n",
				PrintPunishmentTime(DUMB_DURATION(vict) - time(nullptr)),
				DUMB_REASON(vict) ? DUMB_REASON(vict) : "-");
	}

	if (PLR_FLAGGED(vict, EPlrFlag::kHelled) && HELL_DURATION(vict)) {
		out << fmt::format("HELL   : {} [{}].\r\n",
				PrintPunishmentTime(HELL_DURATION(vict) - time(nullptr)),
				HELL_REASON(vict) ? HELL_REASON(vict) : "-");
	}

	if (!PLR_FLAGGED(vict, EPlrFlag::kRegistred) && UNREG_DURATION(vict)) {
		out << fmt::format("UNREG  : {} [{}].\r\n",
				PrintPunishmentTime(UNREG_DURATION(vict) - time(nullptr)),
				UNREG_REASON(vict) ? UNREG_REASON(vict) : "-");
	}
}

char *PrintPunishmentTime(int time) {
	static char time_buf[16];
	time_buf[0] = '\0';
	if (time < 3600)
		snprintf(time_buf, sizeof(time_buf), "%d m", (int) time / 60);
	else if (time < 3600 * 24)
		snprintf(time_buf, sizeof(time_buf), "%d h", (int) time / 3600);
	else if (time < 3600 * 24 * 30)
		snprintf(time_buf, sizeof(time_buf), "%d D", (int) time / (3600 * 24));
	else if (time < 3600 * 24 * 365)
		snprintf(time_buf, sizeof(time_buf), "%d M", (int) time / (3600 * 24 * 30));
	else
		snprintf(time_buf, sizeof(time_buf), "%d Y", (int) time / (3600 * 24 * 365));
	return time_buf;
}

void SendListChar(const std::ostringstream &list_char, const std::string &email) {
	std::string cmd_line = "python3 SendListChar.py " + email + " " + list_char.str() + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

/*

 time_t tmp_time = player_table[player_index].last_logon;
   DescriptorData *d_vict = DescriptorByUid(request_ptr->vict_uid);
  std::ostringstream out;
  out << fmt::format(
		  "Персонаж: {}{}{} e-mail: {}&S{}&s{} Last: {}{}{} from IP: {}{}{}\r\n",
		  (d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)), player_table[player_index].name(), CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR), request_ptr->mail, CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR), rustime(localtime(&tmp_time)), CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR), player_table[player_index].last_ip, CCNRM(ch, C_SPR));
  Player vict;
//  char clan_status[kMaxInputLength];
  sprintf(clan_status, "%s", "нет");
  if ((load_char(player_table[player_index].name(), &vict)) > -1) {
	Clan::SetClanData(&vict);
	if (CLAN(&vict)) {
	  sprintf(clan_status, "%s", (&vict)->player_specials->clan->GetAbbrev());
	}
	PrintPunishment(&vict, buf2);
  }

  time_t mytime = player_table[player_index].last_logon;
  sprintf(buf1, "Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
		  rustime(localtime(&mytime)),
		  player_table[player_index].level, player_table[player_index].remorts,
		  MUD::Class(player_table[player_index].plr_class).GetCName(), clan_status);
  strcat(buf, buf1);

  if (request_ptr->full_search) {
	CharData::shared_ptr target;
	if (d_vict) {
	  target = d_vict->character;
	} else {
	  target = std::make_shared<Player>();
	  if (load_char(request_ptr->request_text.c_str(), target.get()) < 0) {
		auto msg = fmt::format("Некорректное имя персонажа ({}) Inspecting char.\r\n", request_ptr->request_text);
		SendMsgToChar(msg, ch);
		return;
	  }
	}

	if (target && !LOGON_LIST(target).empty()) {
#ifdef TEST_BUILD
	  log("filling logon list");
#endif
	  for (const auto &cur_log : LOGON_LIST(target)) {
		const Logon logon = {str_dup(cur_log.ip), cur_log.count, cur_log.lasttime, false};
		request_ptr->ip_log.push_back(logon);
	  }
	}
  } else {
	const Logon logon = {str_dup(player_table[player_index].last_ip), 0, player_table[player_index].last_logon, false};
	request_ptr->ip_log.push_back(logon);
  }

 */

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :