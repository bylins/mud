#include "inspect.h"

#include <memory>

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

struct InspectRequest {
	EInspect search_for{EInspect::kIp};	// тип запроса
	int vict_uid{0};
	int found{0};						// сколько всего найдено
  	int player_table_pos{0};			// текущая позиция поиска
  	bool full_search{false};
  	bool send_mail{false};				// отправлять ли на мыло список чаров
  	bool valid{false};					// был ли зпрос корректно сформирован
  	std::string request_text;
	std::string mail;
  	std::string out;					// буфер в который накапливается вывод
	std::vector<Logon> ip_log;			// айпи адреса по которым идет поиск
	struct timeval start;				// время когда запустили запрос для отладки
};

extern bool need_warn;
//InspReqListType &inspect_list = MUD::inspect_list();

void PrintPunishment(CharData *vict, char *output);
char *PrintPunishmentTime(int time);
void SendListChar(const std::string &list_char, const std::string &email);
void ComposeMailInspectRequest(InspReqPtr &request_ptr);
void ComposeIpInspectRequest(InspReqPtr &request_ptr);
void ComposeCharacterInspectRequest(CharData *ch, InspReqPtr &request_ptr);

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
	argument = two_arguments(argument, buf, buf2);
	if (!*buf || !*buf2 || !a_isascii(*buf2)) {
		SendMsgToChar("Usage: inspect { mail | ip | char } <argument> [all|все|send_mail]\r\n", ch);
		return;
	}
	if (!isname(buf, "mail ip char")) {
		SendMsgToChar("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return;
	}
	if (strlen(buf2) < kMinRequestLength) {
		SendMsgToChar("Слишком короткий запрос\r\n", ch);
		return;
	}
	if (strlen(buf2) > kMaxRequestLength) {
		SendMsgToChar("Слишком длинный запрос\r\n", ch);
		return;
	}
	if (utils::IsAbbr(buf, "char") && (GetUniqueByName(buf2) <= 0)) {
		SendMsgToChar(ch, "Некорректное имя персонажа (%s) Inspecting char.\r\n", buf2);
		return;
	}
	InspReqPtr request = std::make_shared<InspectRequest>();
	request->request_text = str_dup(buf2);
	buf2[0] = '\0';

	if (argument) {
		if (isname(argument, "все all"))
			if (IS_GRGOD(ch) || PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				need_warn = false;
				request->full_search = true;
			}
		if (isname(argument, "send_mail"))
			request->send_mail = true;
	}

	if (utils::IsAbbr(buf, "mail")) {
	  	ComposeMailInspectRequest(request);
	} else if (utils::IsAbbr(buf, "ip")) {
	  	ComposeIpInspectRequest(request);
	} else if (utils::IsAbbr(buf, "char")) {
	  	ComposeCharacterInspectRequest(ch, request);
	}

  	if (request->valid) {
	  if (request->search_for < EInspect::kChar) {
		sprintf(buf, "%s: %s&S%s&s%s\r\n", (request->search_for == EInspect::kIp ? "IP" : "e-mail"),
				CCWHT(ch, C_SPR), request->request_text.c_str(), CCNRM(ch, C_SPR));
	  }

	  request->out += buf;
	  request->out += buf2;

	  gettimeofday(&request->start, nullptr);
	  MUD::inspect_list()[ch->get_pfilepos()] = request;
	}
}

void ComposeMailInspectRequest(InspReqPtr &request_ptr) {
  request_ptr->search_for = EInspect::kMail;
  request_ptr->valid = true;
}

void ComposeIpInspectRequest(InspReqPtr &request_ptr) {
  request_ptr->search_for = EInspect::kIp;
  if (request_ptr->full_search) {
	const Logon logon = {str_dup(request_ptr->request_text.c_str()), 0, 0, false};
	request_ptr->ip_log.push_back(logon);
  }
  request_ptr->valid = true;
}

void ComposeCharacterInspectRequest(CharData *ch, InspReqPtr &request_ptr) {
  request_ptr->search_for = EInspect::kChar;
  request_ptr->vict_uid = GetUniqueByName(request_ptr->request_text);
  auto player_index = GetPtableByUnique(request_ptr->vict_uid);
  if ((request_ptr->vict_uid <= 0)
	  || (player_table[player_index].level >= kLvlImmortal && !IS_GRGOD(ch))
	  || (player_table[player_index].level > GetRealLevel(ch) && !IS_IMPL(ch)
		  && !PRF_FLAGGED(ch, EPrf::kCoderinfo)))
  {
	SendMsgToChar(ch, "Некорректное имя персонажа (%s) Inspecting char.\r\n", request_ptr->request_text.c_str());
	return;
  }

  DescriptorData *d_vict = DescriptorByUid(request_ptr->vict_uid);
  request_ptr->mail = str_dup(player_table[player_index].mail);
  time_t tmp_time = player_table[player_index].last_logon;

  sprintf(buf,
		  "Персонаж: %s%s%s e-mail: %s&S%s&s%s Last: %s%s%s from IP: %s%s%s\r\n",
		  (d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
		  player_table[player_index].name(),
		  CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR),
		  request_ptr->mail.c_str(),
		  CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR),
		  rustime(localtime(&tmp_time)),
		  CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR),
		  player_table[player_index].last_ip,
		  CCNRM(ch, C_SPR));
  Player vict;
  char clan_status[kMaxInputLength];
  sprintf(clan_status, "%s", "нет");
  if ((load_char(player_table[player_index].name(), &vict)) > -1) {
	Clan::SetClanData(&vict);
	if (CLAN(&vict)) {
	  sprintf(clan_status, "%s", (&vict)->player_specials->clan->GetAbbrev());
	}
	PrintPunishment(&vict, buf2);
  }
  strcpy(smallBuf, MUD::Class(player_table[player_index].plr_class).GetCName());
  time_t mytime = player_table[player_index].last_logon;
  sprintf(buf1, "Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
		  rustime(localtime(&mytime)),
		  player_table[player_index].level, player_table[player_index].remorts, smallBuf, clan_status);
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
  request_ptr->valid = true;
}

void Inspecting() {
	if (MUD::inspect_list().empty()) {
		return;
	}

	auto it = MUD::inspect_list().begin();
	DescriptorData *inspector_descriptor = DescriptorByUid(player_table[it->first].unique);
  	if (!inspector_descriptor || (inspector_descriptor->connected != CON_PLAYING)) {
	  	MUD::inspect_list().erase(it);
	  	return;
	}

  	CharData *ch = inspector_descriptor->character.get();
	if (!ch) {
		MUD::inspect_list().erase(it);
		return;
	}

	auto request = it->second;
	timeval start{}, stop{}, result{};
	time_t mytime;
	int mail_found = 0;
	need_warn = false;

	gettimeofday(&start, nullptr);
	for (; request->player_table_pos < static_cast<int>(player_table.size()); request->player_table_pos++) {
		gettimeofday(&stop, nullptr);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= kOptUsec) {
			return;
		}

#ifdef TEST_BUILD
		log("inspecting %d/%lu", 1 + request->pos, player_table.size());
#endif

		if (request->request_text.empty()) {
			SendMsgToChar(ch, "Ошибка: пустой параметр для поиска");
			break;
		}

		const auto &inspected_index = player_table[request->player_table_pos];
		if ((request->search_for == EInspect::kChar && request->vict_uid == inspected_index.unique)
			|| (inspected_index.level >= kLvlImmortal && !IS_GRGOD(ch))
			|| (inspected_index.level > GetRealLevel(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))) {
			continue;
		}

		buf1[0] = '\0';
		buf2[0] = '\0';
		CharData::shared_ptr vict;
		DescriptorData *d_vict = DescriptorByUid(inspected_index.unique);
		if (request->search_for != EInspect::kMail && request->full_search) {
			if (d_vict) {
				vict = d_vict->character;
			} else {
				vict = std::make_shared<Player>();
				if (load_char(inspected_index.name(), vict.get()) < 0) {
					auto msg = fmt::format("Некорректное имя персонажа ({}) Inspecting {}: {}.\r\n",
										   inspected_index.name(),
										   (request->search_for == EInspect::kMail ? "mail" :
											(request->search_for == EInspect::kIp ? "ip" : "char")),
										   request->request_text);
					SendMsgToChar(msg, ch);
					continue;
				}
			}
//			show_pun(vict.get(), buf2); ниже общий
		}

		if (request->search_for == EInspect::kMail || request->search_for == EInspect::kChar) {
			mail_found = 0;
			if (inspected_index.mail) {
				if ((request->search_for == EInspect::kMail
					&& strstr(inspected_index.mail, request->request_text.c_str()))
					|| (request->search_for == EInspect::kChar
					&& !strcmp(inspected_index.mail, request->mail.c_str()))) {
					mail_found = 1;
				}
			}
		}

		if (request->search_for == EInspect::kIp || request->search_for == EInspect::kChar) {
			if (!request->full_search) {
				if (inspected_index.last_ip) {
					if ((request->search_for == EInspect::kIp
						&& strstr(inspected_index.last_ip, request->request_text.c_str()))
						|| (!request->ip_log.empty()
							&& !str_cmp(inspected_index.last_ip, request->ip_log.at(0).ip))) {
						sprintf(buf1 + strlen(buf1),
								" IP:%s%-16s%s\r\n",
								(request->search_for == EInspect::kChar ? CCBLU(ch, C_SPR) : ""),
								inspected_index.last_ip,
								(request->search_for == EInspect::kChar ? CCNRM(ch, C_SPR) : ""));
					}
				}
			} else if (vict && !LOGON_LIST(vict).empty()) {
				for (const auto &cur_log : LOGON_LIST(vict)) {
					for (const auto &ch_log : request->ip_log) {
						if (!ch_log.ip) {
							SendMsgToChar(ch, "Ошибка: пустой ip\r\n");
							break;
						}
						if (str_cmp(cur_log.ip, "135.181.219.76")) { // игнорим bylins.online
							if (!str_cmp(cur_log.ip, ch_log.ip)) {
								sprintf(buf1 + strlen(buf1),
										" IP:%s%-16s%s Количество входов с него:%5ld Последний раз: %-30s\r\n",
										CCBLU(ch, C_SPR),
										cur_log.ip,
										CCNRM(ch, C_SPR),
										cur_log.count,
										rustime(localtime(&cur_log.lasttime)));
								/*							if (request->sfor == ICHAR)
															{
																sprintf(buf1 + strlen(buf1), "-> Count:%5ld Last : %s\r\n",
																	ch_log.count, rustime(localtime(&ch_log.lasttime)));
															}*/
							}
						}
					}
				}
			}
		}

		if (*buf1 || mail_found) {
			strcpy(smallBuf, MUD::Class(inspected_index.plr_class).GetCName());
			mytime = inspected_index.last_logon;
			Player target;
			char clanstatus[kMaxInputLength];
			sprintf(clanstatus, "%s", "нет");
			if ((load_char(inspected_index.name(), &target)) > -1) {
				Clan::SetClanData(&target);
				if (CLAN(&target))
					sprintf(clanstatus, "%s", (&target)->player_specials->clan->GetAbbrev());
				PrintPunishment(&target, buf2);
			}
			sprintf(buf,
					"--------------------\r\nИмя: %s%-12s%s e-mail: %s&S%-30s&s%s Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
					(d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
					inspected_index.name(),
					CCNRM(ch, C_SPR),
					(mail_found && request->search_for != EInspect::kMail ? CCBLU(ch, C_SPR) : ""),
					inspected_index.mail,
					(mail_found ? CCNRM(ch, C_SPR) : ""),
					rustime(localtime(&mytime)),
					inspected_index.level,
					inspected_index.remorts,
					smallBuf, clanstatus);
			request->out += buf;
			request->out += buf2;
			request->out += buf1;
			request->found++;
		}
	}

	need_warn = true;
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &request->start);
	sprintf(buf1, "Всего найдено: %d за %ldсек.\r\n", request->found, result.tv_sec);
	request->out += buf1;
	if (request->send_mail) {
		if (request->found > 1 && request->search_for == EInspect::kMail) {
			request->out += "Данный список отправлен игроку на емайл\r\n";
			SendListChar(request->out, request->request_text);
		}
	}

	page_string(ch->desc, request->out);
	MUD::inspect_list().erase(it);
}

void PrintPunishment(CharData *vict, char *output) {
	if (PLR_FLAGGED(vict, EPlrFlag::kFrozen)
		&& FREEZE_DURATION(vict))
		sprintf(output + strlen(output), "FREEZE : %s [%s].\r\n",
				PrintPunishmentTime(FREEZE_DURATION(vict) - time(nullptr)),
				FREEZE_REASON(vict) ? FREEZE_REASON(vict)
									: "-");

	if (PLR_FLAGGED(vict, EPlrFlag::kMuted)
		&& MUTE_DURATION(vict))
		sprintf(output + strlen(output), "MUTE   : %s [%s].\r\n",
				PrintPunishmentTime(MUTE_DURATION(vict) - time(nullptr)),
				MUTE_REASON(vict) ? MUTE_REASON(vict) : "-");

	if (PLR_FLAGGED(vict, EPlrFlag::kDumbed)
		&& DUMB_DURATION(vict))
		sprintf(output + strlen(output), "DUMB   : %s [%s].\r\n",
				PrintPunishmentTime(DUMB_DURATION(vict) - time(nullptr)),
				DUMB_REASON(vict) ? DUMB_REASON(vict) : "-");

	if (PLR_FLAGGED(vict, EPlrFlag::kHelled)
		&& HELL_DURATION(vict))
		sprintf(output + strlen(output), "HELL   : %s [%s].\r\n",
				PrintPunishmentTime(HELL_DURATION(vict) - time(nullptr)),
				HELL_REASON(vict) ? HELL_REASON(vict) : "-");

	if (!PLR_FLAGGED(vict, EPlrFlag::kRegistred)
		&& UNREG_DURATION(vict))
		sprintf(output + strlen(output), "UNREG  : %s [%s].\r\n",
				PrintPunishmentTime(UNREG_DURATION(vict) - time(nullptr)),
				UNREG_REASON(vict) ? UNREG_REASON(vict) : "-");
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

void SendListChar(const std::string &list_char, const std::string &email) {
	std::string cmd_line = "python3 SendListChar.py " + email + " " + list_char + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :