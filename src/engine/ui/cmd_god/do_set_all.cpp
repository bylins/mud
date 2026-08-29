/**
\file set_all.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd_god/do_set_all.h"
#include "utils/native_text.h"
#include "engine/db/player_index.h"
#include <fmt/format.h>
#include "utils/mud_string.h"

#include "administration/karma.h"
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

void setall_inspect() {
	if (setall_inspect_list.empty()) {
		return;
	}
	auto it = setall_inspect_list.begin();
	CharData *ch = nullptr;
	DescriptorData *d_vict = nullptr;

	DescriptorData *imm_d = DescriptorByUid(player_table[it->first].uid());
	if (!imm_d
		|| (imm_d->state != EConState::kPlaying)
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
		is_online = 0;
		d_vict = DescriptorByUid(player_table[it->second->pos].uid());
		if (d_vict)
			is_online = 1;
		if (!player_table[it->second->pos].mail.empty())
			if (player_table[it->second->pos].mail.find(it->second->mail) != std::string::npos) {
				it->second->found++;
				if (it->second->type_req == kSetallFreeze) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						}
						punishments::SetFreeze(imm_d->character.get(),
												d_vict->character.get(),
												it->second->reason,
												it->second->freeze_time);
					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name().c_str(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							delete vict;
							it->second->out += fmt::format("Ошибка загрузки персонажа: {}.\r\n",
														   player_table[it->second->pos].name());
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
															   player_table[it->second->pos].name());
								delete vict;
								continue;
							}
							punishments::SetFreeze(imm_d->character.get(),
													vict,
													it->second->reason,
													it->second->freeze_time);
							vict->save_char();
						}
					}
				} else if (it->second->type_req == kSetallEmail) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						}
						strncpy(GET_EMAIL(d_vict->character), it->second->newmail, 127);
						*(GET_EMAIL(d_vict->character) + 127) = '\0';
						const std::string mail_note =
							fmt::format("Смена e-mail адреса персонажа {} с {} на {}.\r\n",
										player_table[it->second->pos].name(),
										player_table[it->second->pos].mail,
										it->second->newmail);
						AddKarma(d_vict->character.get(), mail_note.c_str(), GET_NAME(imm_d->character));
						it->second->out += mail_note;

					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name().c_str(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							it->second->out += fmt::format("Ошибка загрузки персонажа: {}.\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								// Раньше сюда уходил глобальный buf1, который в этой ветке никто не
								// заполнял: бог получал пустоту вместо причины пропуска (issue #3807).
								it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
															   player_table[it->second->pos].name());
								delete vict;
								continue;
							}
							strncpy(GET_EMAIL(vict), it->second->newmail, 127);
							*(GET_EMAIL(vict) + 127) = '\0';
							const std::string mail_note =
								fmt::format("Смена e-mail адреса персонажа {} с {} на {}.\r\n",
											player_table[it->second->pos].name(),
											player_table[it->second->pos].mail,
											it->second->newmail);
							it->second->out += mail_note;
							AddKarma(vict, mail_note.c_str(), GET_NAME(imm_d->character));
							vict->save_char();
						}
					}
				} else if (it->second->type_req == kSetallPwd) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						}
						Password::set_password(d_vict->character.get(), std::string(it->second->pwd));
						const std::string pwd_note =
							fmt::format("У персонажа {} изменен пароль (setall).", player_table[it->second->pos].name());
						it->second->out += pwd_note;
						it->second->out += "\r\n";
						AddKarma(d_vict->character.get(), pwd_note.c_str(), GET_NAME(imm_d->character));
					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name().c_str(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							it->second->out += fmt::format("Ошибка загрузки персонажа: {}.\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						}
						if (GetRealLevel(vict) >= kLvlGod) {
							it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						}
						Password::set_password(vict, std::string(it->second->pwd));
						const std::string pwd_note =
							fmt::format("У персонажа {} изменен пароль (setall).", player_table[it->second->pos].name());
						it->second->out += pwd_note;
						it->second->out += "\r\n";
						AddKarma(vict, pwd_note.c_str(), GET_NAME(imm_d->character));
						vict->save_char();
					}
				} else if (it->second->type_req == kSetallHell) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
														   player_table[it->second->pos].name());
							delete vict;
							continue;
						}
						punishments::SetHell(imm_d->character.get(),
												d_vict->character.get(),
												it->second->reason,
												it->second->freeze_time);
					} else {
						if (LoadPlayerCharacter(player_table[it->second->pos].name().c_str(), vict,
												ELoadCharFlags::kFindId | ELoadCharFlags::kNoCrcCheck) < 0) {
							delete vict;
							it->second->out += fmt::format("Ошибка загрузки персонажа: {}.\r\n",
														   player_table[it->second->pos].name());
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								it->second->out += fmt::format("Персонаж {} бессмертный!\r\n",
															   player_table[it->second->pos].name());
								delete vict;
								continue;
							}
							punishments::SetHell(imm_d->character.get(),
													vict,
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
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &it->second->start);
	it->second->out += fmt::format("Всего найдено: {}.\r\n", it->second->found);
	page_string(ch->desc, it->second->out);
	setall_inspect_list.erase(it->first);
}

void do_setall(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int type_request = 0;
	int times = 0;
	if (ch->get_pfilepos() < 0)
		return;

	auto it = setall_inspect_list.find(ch->get_uid());
	// На всякий случай разрешаем только одну команду такого типа - либо setall, либо inspect
	if (MUD::InspectRequests().IsBusy(ch) && it != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}

	// Разбор идёт по своим строкам, а не по глобальным buf/buf1/buf2 (issue #3807).
	auto [mail, after_mail] = ChopWord(argument ? argument : "");
	auto [action, after_action] = ChopWord(after_mail);
	auto [param, reason] = ChopWord(after_action);

	SetAllInspReqPtr req(new setall_inspect_request);
	req->newmail = nullptr;
	req->mail = nullptr;
	req->reason = nullptr;
	req->pwd = nullptr;

	if (mail.empty()) {
		SendMsgToChar("Usage: setall <e-mail> <email|passwd|frozen|hell> <arguments>\r\n", ch);
		return;
	}

	if (!IsValidEmail(mail.c_str())) {
		SendMsgToChar("Некорректный e-mail!\r\n", ch);
		return;
	}

	if (!isname(action, "frozen email passwd hell")) {
		SendMsgToChar("Данное действие совершить нельзя.\r\n", ch);
		return;
	}
	if (utils::IsAbbr(action.c_str(), "frozen")) {
		if (reason.empty()) {
			SendMsgToChar("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (!param.empty()) times = atol(param.c_str());
		type_request = kSetallFreeze;
		req->freeze_time = times;
		req->reason = strdup(reason.c_str());
	} else if (utils::IsAbbr(action.c_str(), "email")) {
		if (param.empty()) {
			SendMsgToChar("Укажите новый e-mail!\r\n", ch);
			return;
		}
		if (!IsValidEmail(param.c_str())) {
			SendMsgToChar("Новый e-mail некорректен!\r\n", ch);
			return;
		}
		req->newmail = strdup(param.c_str());
		type_request = kSetallEmail;
	} else if (utils::IsAbbr(action.c_str(), "passwd")) {
		if (param.empty()) {
			SendMsgToChar("Укажите новый пароль!\r\n", ch);
			return;
		}
		req->pwd = strdup(param.c_str());
		type_request = kSetallPwd;
	} else if (utils::IsAbbr(action.c_str(), "hell")) {
		if (reason.empty()) {
			SendMsgToChar("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (!param.empty()) times = atol(param.c_str());
		type_request = kSetallHell;
		req->freeze_time = times;
		req->reason = strdup(reason.c_str());
	} else {
		SendMsgToChar("Какой-то баг. Вы эту надпись видеть не должны.\r\n", ch);
		return;
	}

	req->type_req = type_request;
	req->mail = str_dup(mail.c_str());
	req->pos = 0;
	req->found = 0;
	req->out = "";
	setall_inspect_list[ch->get_pfilepos()] = req;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
