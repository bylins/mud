/**
\file do_unfreeze.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "administration/punishments.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/global_objects.h"

#include <sstream>
#include <vector>
#include <iostream>

void DoUnfreeze(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	/*Формат файл unfreeze.lst
	Первая строка email
	Вторая строка причина по которой разфриз
	Все остальные строки полные имена чаров*/
	//char email[50], reason[50];
	Player t_vict;
	CharData *vict;
	char *reason_c; // для функции set_punish, она не умеет принимать тип string :(
	std::string email;
	std::string reason;
	std::string name_buffer;
	// issue.misc-migrate: StateManager owns the file I/O. Flatten to whitespace tokens to keep the
	// previous stream `>>` semantics: token 0 = email, token 1 = reason, the rest = char names.
	std::vector<std::string> tokens;
	for (const auto &line : MUD::StateManager().LoadLines(state::EStateFile::kUnfreeze)) {
		std::istringstream iss(line);
		std::string tok;
		while (iss >> tok) {
			tokens.push_back(tok);
		}
	}
	if (tokens.size() < 2) {
		SendMsgToChar("Файл unfreeze.lst отсутствует!\r\n", ch);
		return;
	}
	email = tokens[0];
	reason = tokens[1];
	SendMsgToChar(fmt::format("Начинаем масс.разфриз\r\nEmail:{}\r\nПричина:{}\r\n", email, reason), ch);
	reason_c = new char[reason.length() + 1];
	strcpy(reason_c, reason.c_str());

	for (std::size_t i = 2; i < tokens.size(); ++i) {
		name_buffer = tokens[i];
		if (LoadPlayerCharacter(name_buffer.c_str(), &t_vict, ELoadCharFlags::kFindId) < 0) {
			SendMsgToChar(fmt::format("Чара с именем {} не существует !\r\n", name_buffer), ch);
			continue;
		}
		vict = &t_vict;
		if (GET_EMAIL(vict) != email) {
			SendMsgToChar(fmt::format("У чара {} другой емайл.\r\n", name_buffer), ch);
			continue;
		}
		punishments::SetFreeze(ch, vict, reason_c, 0);
		vict->save_char();
		SendMsgToChar(fmt::format("Чар {} разморожен.\r\n", name_buffer), ch);
	}

	delete[] reason_c;

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
