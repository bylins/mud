/**
\file set_all.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd_god/do_set_all.h"

#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/global_objects.h"
#include "engine/ui/modify.h"
#include "administration/password.h"

SetAllInspReqListType &setall_inspect_list = MUD::setall_inspect_list();

enum ESetAllKind {
  kSetallFreeze,
  kSetallEmail,
  kSetallPwd,
  kSetallHell
};

extern void AddKarma(CharData *ch, const char *punish, const char *reason);

void setall_inspect() {
	if (setall_inspect_list.empty()) {
		return;
	}
	auto it = setall_inspect_list.begin();
	CharData *ch = nullptr;
	DescriptorData *d_vict = nullptr;

	DescriptorData *imm_d = DescriptorByUid(player_table[it->first].uid());
	if (!imm_d
		|| (STATE(imm_d) != CON_PLAYING)
		|| !(ch = imm_d->character.get())) {
		setall_inspect_list.erase(it->first);
		return;
	}

	timeval start{}, stop{}, result{};
	int is_online;
	gettimeofday(&start, nullptr);
	Player *vict;
	for (; it->second->pos < static_cast<int>(player_table.size()); it->second->pos++) {
		vict = new Player;
		gettimeofday(&stop, nullptr);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= kOptUsec) {
			delete vict;
			return;
		}
		buf1[0] = '\0';
		is_online = 0;
		d_vict = DescriptorByUid(player_table[it->second->pos].uid());
		if (d_vict)
			is_online = 1;
		if (player_table[it->second->pos].mail)
			if (strstr(player_table[it->second->pos].mail, it->second->mail)) {
				it->second->found++;
				if (it->second->type_req == kSetallFreeze) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						punishments::set_punish(imm_d->character.get(),
												d_vict->character.get(),
												SCMD_FREEZE,
												it->second->reason,
												it->second->freeze_time);
					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							delete vict;
							it->second->out += buf1;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
								it->second->out += buf1;
								continue;
							}
							punishments::set_punish(imm_d->character.get(),
													vict,
													SCMD_FREEZE,
													it->second->reason,
													it->second->freeze_time);
							vict->save_char();
						}
					}
				} else if (it->second->type_req == kSetallEmail) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						strncpy(GET_EMAIL(d_vict->character), it->second->newmail, 127);
						*(GET_EMAIL(d_vict->character) + 127) = '\0';
						sprintf(buf2,
								"Смена e-mail адреса персонажа %s с %s на %s.\r\n",
								player_table[it->second->pos].name(),
								player_table[it->second->pos].mail,
								it->second->newmail);
						AddKarma(d_vict->character.get(), buf2, GET_NAME(imm_d->character));
						it->second->out += buf2;

					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							delete vict;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								it->second->out += buf1;
								continue;
							}
							strncpy(GET_EMAIL(vict), it->second->newmail, 127);
							*(GET_EMAIL(vict) + 127) = '\0';
							sprintf(buf2,
									"Смена e-mail адреса персонажа %s с %s на %s.\r\n",
									player_table[it->second->pos].name(),
									player_table[it->second->pos].mail,
									it->second->newmail);
							it->second->out += buf2;
							AddKarma(vict, buf2, GET_NAME(imm_d->character));
							vict->save_char();
						}
					}
				} else if (it->second->type_req == kSetallPwd) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						Password::set_password(d_vict->character.get(), std::string(it->second->pwd));
						sprintf(buf2, "У персонажа %s изменен пароль (setall).", player_table[it->second->pos].name());
						it->second->out += buf2;
						sprintf(buf1, "\r\n");
						it->second->out += buf1;
						AddKarma(d_vict->character.get(), buf2, GET_NAME(imm_d->character));
					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							delete vict;
							continue;
						}
						if (GetRealLevel(vict) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						Password::set_password(vict, std::string(it->second->pwd));
						std::string str = player_table[it->second->pos].name();
						str[0] = UPPER(str[0]);
						sprintf(buf2, "У персонажа %s изменен пароль (setall).", player_table[it->second->pos].name());
						it->second->out += buf2;
						sprintf(buf1, "\r\n");
						it->second->out += buf1;
						AddKarma(vict, buf2, GET_NAME(imm_d->character));
						vict->save_char();
					}
				} else if (it->second->type_req == kSetallHell) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						punishments::set_punish(imm_d->character.get(),
												d_vict->character.get(),
												SCMD_HELL,
												it->second->reason,
												it->second->freeze_time);
					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							delete vict;
							it->second->out += buf1;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
								it->second->out += buf1;
								continue;
							}
							punishments::set_punish(imm_d->character.get(),
													vict,
													SCMD_HELL,
													it->second->reason,
													it->second->freeze_time);
							vict->save_char();
						}
					}
				}
			}
		delete vict;
	}
	if (it->second->mail && it->second->pwd)
		Password::send_password(it->second->mail, it->second->pwd);
	// освобождение памяти
	if (it->second->pwd)
		free(it->second->pwd);
	if (it->second->reason)
		free(it->second->reason);
	if (it->second->newmail)
		free(it->second->newmail);
	if (it->second->mail)
		free(it->second->mail);
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d.\r\n", it->second->found);
	it->second->out += buf1;
	page_string(ch->desc, it->second->out);
	setall_inspect_list.erase(it->first);
}

void do_setall(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int type_request = 0;
	int times = 0;
	if (ch->get_pfilepos() < 0)
		return;

	auto it = setall_inspect_list.find(GET_UID(ch));
	// На всякий случай разрешаем только одну команду такого типа - либо setall, либо inspect
	if (MUD::InspectRequests().IsBusy(ch) && it != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}

	argument = three_arguments(argument, buf, buf1, buf2);
	SetAllInspReqPtr req(new setall_inspect_request);
	req->newmail = nullptr;
	req->mail = nullptr;
	req->reason = nullptr;
	req->pwd = nullptr;

	if (!*buf) {
		SendMsgToChar("Usage: setall <e-mail> <email|passwd|frozen|hell> <arguments>\r\n", ch);
		return;
	}

	if (!IsValidEmail(buf)) {
		SendMsgToChar("Некорректный e-mail!\r\n", ch);
		return;
	}

	if (!isname(buf1, "frozen email passwd hell")) {
		SendMsgToChar("Данное действие совершить нельзя.\r\n", ch);
		return;
	}
	if (utils::IsAbbr(buf1, "frozen")) {
		skip_spaces(&argument);
		if (!argument || !*argument) {
			SendMsgToChar("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (*buf2) times = atol(buf2);
		type_request = kSetallFreeze;
		req->freeze_time = times;
		req->reason = strdup(argument);
	} else if (utils::IsAbbr(buf1, "email")) {
		if (!*buf2) {
			SendMsgToChar("Укажите новый e-mail!\r\n", ch);
			return;
		}
		if (!IsValidEmail(buf2)) {
			SendMsgToChar("Новый e-mail некорректен!\r\n", ch);
			return;
		}
		req->newmail = strdup(buf2);
		type_request = kSetallEmail;
	} else if (utils::IsAbbr(buf1, "passwd")) {
		if (!*buf2) {
			SendMsgToChar("Укажите новый пароль!\r\n", ch);
			return;
		}
		req->pwd = strdup(buf2);
		type_request = kSetallPwd;
	} else if (utils::IsAbbr(buf1, "hell")) {
		skip_spaces(&argument);
		if (!argument || !*argument) {
			SendMsgToChar("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (*buf2) times = atol(buf2);
		type_request = kSetallHell;
		req->freeze_time = times;
		req->reason = strdup(argument);
	} else {
		SendMsgToChar("Какой-то баг. Вы эту надпись видеть не должны.\r\n", ch);
		return;
	}

	req->type_req = type_request;
	req->mail = str_dup(buf);
	req->pos = 0;
	req->found = 0;
	req->out = "";
	setall_inspect_list[ch->get_pfilepos()] = req;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
