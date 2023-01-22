#include "inspect.h"

#include "house.h"
#include "entities/char_player.h"
#include "structs/global_objects.h"
#include "color.h"
#include "modify.h"

enum class EInspect {
	kIp = 1,
	kMail = 2,
	kChar = 3
};

struct InspectRequest {
	EInspect search_for{EInspect::kIp};	// тип запроса
	int unique{0};						// UID
	bool fullsearch{false};				// полный поиск или нет
	int found{0};						// сколько всего найдено
	char *req{nullptr};					// собственно сам запрос
	char *mail{nullptr};				// мыло
	int pos{0};							// позиция в таблице
	std::vector<Logon> ip_log;			// айпи адреса по которым идет поиск
	struct timeval start;				// время когда запустили запрос для отладки
	std::string out;					// буфер в который накапливается вывод
	bool sendmail{false};				// отправлять ли на мыло список чаров
};

extern bool need_warn;
//InspReqListType &inspect_list = MUD::inspect_list();

void PrintPunishment(CharData *vict, char *output);
char *PrintPunishmentTime(int time);
void SendListChar(const std::string &list_char, const std::string &email);

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_pfilepos() < 0)
		return;

	auto it = MUD::inspect_list().find(ch->get_uid());
	auto it_setall = setall_inspect_list.find(ch->get_uid());
	// Навсякий случай разрешаем только одну команду такого типа, либо сетол, либо инспект
	if (it != MUD::inspect_list().end() && it_setall != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}
	argument = two_arguments(argument, buf, buf2);
	if (!*buf || !*buf2 || !a_isascii(*buf2)) {
		SendMsgToChar("Usage: inspect { mail | ip | char } <argument> [all|все|sendmail]\r\n", ch);
		return;
	}
	if (!isname(buf, "mail ip char")) {
		SendMsgToChar("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return;
	}
	if (strlen(buf2) < 3) {
		SendMsgToChar("Слишком короткий запрос\r\n", ch);
		return;
	}
	if (strlen(buf2) > 65) {
		SendMsgToChar("Слишком длинный запрос\r\n", ch);
		return;
	}
	if (utils::IsAbbr(buf, "char") && (GetUniqueByName(buf2) <= 0)) {
		SendMsgToChar(ch, "Некорректное имя персонажа (%s) Inspecting char.\r\n", buf2);
		return;
	}
	InspReqPtr request(new InspectRequest);
	request->mail = nullptr;
	request->fullsearch = false;
	request->req = str_dup(buf2);
	request->sendmail = false;
	buf2[0] = '\0';

	if (argument) {
		if (isname(argument, "все all"))
			if (IS_GRGOD(ch) || PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				need_warn = false;
				request->fullsearch = true;
			}
		if (isname(argument, "sendmail"))
			request->sendmail = true;
	}

	int player_index;
	if (utils::IsAbbr(buf, "mail")) {
		request->search_for = EInspect::kMail;
	} else if (utils::IsAbbr(buf, "ip")) {
		request->search_for = EInspect::kIp;
		if (request->fullsearch) {
			const Logon logon = {str_dup(request->req), 0, 0, false};
			request->ip_log.push_back(logon);
		}
	} else if (utils::IsAbbr(buf, "char")) {
		request->search_for = EInspect::kChar;
		request->unique = static_cast<int>(GetUniqueByName(request->req));
		player_index = static_cast<int>(get_ptable_by_unique(request->unique));
		if ((request->unique <= 0)//Перс не существует
			|| (player_table[player_index].level >= kLvlImmortal && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[player_index].level > GetRealLevel(ch) && !IS_IMPL(ch)
				&& !PRF_FLAGGED(ch, EPrf::kCoderinfo)))//если левел больше то облом
		{
			SendMsgToChar(ch, "Некорректное имя персонажа (%s) Inspecting char.\r\n", request->req);
			request.reset();
			return;
		}

		DescriptorData *d_vict = DescByUID(request->unique);
		request->mail = str_dup(player_table[player_index].mail);
		time_t tmp_time = player_table[player_index].last_logon;

		sprintf(buf,
				"Персонаж: %s%s%s e-mail: %s&S%s&s%s Last: %s%s%s from IP: %s%s%s\r\n",
				(d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
				player_table[player_index].name(),
				CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR),
				request->mail,
				CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR),
				rustime(localtime(&tmp_time)),
				CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR),
				player_table[player_index].last_ip,
				CCNRM(ch, C_SPR));
		Player vict;
		char clanstatus[kMaxInputLength];
		sprintf(clanstatus, "%s", "нет");
		if ((load_char(player_table[player_index].name(), &vict)) > -1) {
			Clan::SetClanData(&vict);
			if (CLAN(&vict)) {
				sprintf(clanstatus, "%s", (&vict)->player_specials->clan->GetAbbrev());
			}
			PrintPunishment(&vict, buf2);
		}
		strcpy(smallBuf, MUD::Class(player_table[player_index].plr_class).GetCName());
		time_t mytime = player_table[player_index].last_logon;
		sprintf(buf1, "Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
				rustime(localtime(&mytime)),
				player_table[player_index].level, player_table[player_index].remorts, smallBuf, clanstatus);
		strcat(buf, buf1);

		if (request->fullsearch) {
			CharData::shared_ptr target;
			if (d_vict) {
				target = d_vict->character;
			} else {
				target.reset(new Player);
				if (load_char(request->req, target.get()) < 0) {
					SendMsgToChar(ch, "Некорректное имя персонажа (%s) Inspecting char.\r\n", request->req);
					return;
				}
			}

			if (target && !LOGON_LIST(target).empty()) {
#ifdef TEST_BUILD
				log("filling logon list");
#endif
				for (const auto &cur_log : LOGON_LIST(target)) {
					const Logon logon = {str_dup(cur_log.ip), cur_log.count, cur_log.lasttime, false};
					request->ip_log.push_back(logon);
				}
			}
		} else {
			const Logon logon = {str_dup(player_table[player_index].last_ip), 0, player_table[player_index].last_logon, false};
			request->ip_log.push_back(logon);
		}
	}

	if (request->search_for < EInspect::kChar) {
		sprintf(buf, "%s: %s&S%s&s%s\r\n", (request->search_for == EInspect::kIp ? "IP" : "e-mail"),
				CCWHT(ch, C_SPR), request->req, CCNRM(ch, C_SPR));
	}
	request->pos = 0;
	request->found = 0;
	request->out += buf;
	request->out += buf2;

	gettimeofday(&request->start, nullptr);
	MUD::inspect_list()[ch->get_pfilepos()] = request;
}

void Inspecting() {
	if (MUD::inspect_list().empty()) {
		return;
	}

	auto it = MUD::inspect_list().begin();

	CharData *ch;
	DescriptorData *d_vict;
	if (!(d_vict = DescByUID(player_table[it->first].unique))
		|| (STATE(d_vict) != CON_PLAYING)
		|| !(ch = d_vict->character.get())) {
		MUD::inspect_list().erase(it->first);
		return;
	}

	timeval start{}, stop{}, result{};
	time_t mytime;
	int mail_found = 0;
	int is_online;
	need_warn = false;

	gettimeofday(&start, nullptr);
	for (; it->second->pos < static_cast<int>(player_table.size()); it->second->pos++) {
		gettimeofday(&stop, nullptr);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= kOptUsec) {
			return;
		}

#ifdef TEST_BUILD
		log("inspecting %d/%lu", 1 + it->second->pos, player_table.size());
#endif

		if (!*it->second->req) {
			SendMsgToChar(ch, "Ошибка: пустой параметр для поиска");
			break;
		}

		if ((it->second->search_for == EInspect::kChar
			&& it->second->unique == player_table[it->second->pos].unique)//Это тот же перс которого мы статим
			|| (player_table[it->second->pos].level >= kLvlImmortal && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[it->second->pos].level > GetRealLevel(ch) && !IS_IMPL(ch)
				&& !PRF_FLAGGED(ch, EPrf::kCoderinfo)))//если левел больше то облом
		{
			continue;
		}

		buf1[0] = '\0';
		buf2[0] = '\0';
		is_online = 0;

		CharData::shared_ptr vict;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict) {
			is_online = 1;
		}

		if (it->second->search_for != EInspect::kMail && it->second->fullsearch) {
			if (d_vict) {
				vict = d_vict->character;
			} else {
				vict.reset(new Player);
				if (load_char(player_table[it->second->pos].name(), vict.get()) < 0) {
					SendMsgToChar(ch,
								  "Некорректное имя персонажа (%s) Inspecting %s: %s.\r\n",
								  player_table[it->second->pos].name(),
								  (it->second->search_for == EInspect::kMail ? "mail" :
								   (it->second->search_for == EInspect::kIp ? "ip" : "char")),
								  it->second->req);
					continue;
				}
			}
//			show_pun(vict.get(), buf2); ниже общий
		}

		if (it->second->search_for == EInspect::kMail || it->second->search_for == EInspect::kChar) {
			mail_found = 0;
			if (player_table[it->second->pos].mail) {
				if ((it->second->search_for == EInspect::kMail
					&& strstr(player_table[it->second->pos].mail, it->second->req))
					|| (it->second->search_for == EInspect::kChar
					&& !strcmp(player_table[it->second->pos].mail, it->second->mail))) {
					mail_found = 1;
				}
			}
		}

		if (it->second->search_for == EInspect::kIp
			|| it->second->search_for == EInspect::kChar) {
			if (!it->second->fullsearch) {
				if (player_table[it->second->pos].last_ip) {
					if ((it->second->search_for == EInspect::kIp
						&& strstr(player_table[it->second->pos].last_ip, it->second->req))
						|| (!it->second->ip_log.empty()
							&& !str_cmp(player_table[it->second->pos].last_ip, it->second->ip_log.at(0).ip))) {
						sprintf(buf1 + strlen(buf1),
								" IP:%s%-16s%s\r\n",
								(it->second->search_for == EInspect::kChar ? CCBLU(ch, C_SPR) : ""),
								player_table[it->second->pos].last_ip,
								(it->second->search_for == EInspect::kChar ? CCNRM(ch, C_SPR) : ""));
					}
				}
			} else if (vict && !LOGON_LIST(vict).empty()) {
				for (const auto &cur_log : LOGON_LIST(vict)) {
					for (const auto &ch_log : it->second->ip_log) {
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
								/*							if (it->second->sfor == ICHAR)
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
			const auto &player = player_table[it->second->pos];
			strcpy(smallBuf, MUD::Class(player.plr_class).GetCName());
			mytime = player_table[it->second->pos].last_logon;
			Player target;
			char clanstatus[kMaxInputLength];
			sprintf(clanstatus, "%s", "нет");
			if ((load_char(player.name(), &target)) > -1) {
				Clan::SetClanData(&target);
				if (CLAN(&target))
					sprintf(clanstatus, "%s", (&target)->player_specials->clan->GetAbbrev());
				PrintPunishment(&target, buf2);
			}
			sprintf(buf,
					"--------------------\r\nИмя: %s%-12s%s e-mail: %s&S%-30s&s%s Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
					(is_online ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
					player.name(),
					CCNRM(ch, C_SPR),
					(mail_found && it->second->search_for != EInspect::kMail ? CCBLU(ch, C_SPR) : ""),
					player.mail,
					(mail_found ? CCNRM(ch, C_SPR) : ""),
					rustime(localtime(&mytime)),
					player.level,
					player.remorts,
					smallBuf, clanstatus);
			it->second->out += buf;
			it->second->out += buf2;
			it->second->out += buf1;
			it->second->found++;
		}
	}

	need_warn = true;
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d за %ldсек.\r\n", it->second->found, result.tv_sec);
	it->second->out += buf1;
	if (it->second->sendmail)
		if (it->second->found > 1 && it->second->search_for == EInspect::kMail) {
			it->second->out += "Данный список отправлен игроку на емайл\r\n";
			SendListChar(it->second->out, it->second->req);
		}
	if (it->second->mail)
		free(it->second->mail);

	page_string(ch->desc, it->second->out);
	free(it->second->req);
	MUD::inspect_list().erase(it->first);
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