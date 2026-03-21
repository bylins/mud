// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

//#include "remember.h"

//#include "utils/logger.h"
#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "gameplay/clans/house.h"

#include <third_party_libs/fmt/include/fmt/format.h>

namespace Remember {

// болтать/орать
RememberListType gossip;
// оффтоп
RememberListType offtop;
// воззвания (иммам)
RememberListType imm_pray;
// гбогам/wiznet
RememberWiznetListType wiznet_;

std::string time_format() {
	char time_buf[9];
	time_t tmp_time = time(nullptr);
	strftime(time_buf, sizeof(time_buf), "[%H:%M] ", localtime(&tmp_time));
	return time_buf;
}

// * Анти-копипаст для CharRemember::add_str.
void add_to_cont(RememberListType &cont, const std::string &text) {
	cont.push_back(text);
	if (cont.size() > MAX_REMEMBER_NUM) {
		cont.erase(cont.begin());
	}
}

// * Анти-копипаст для CharRemember::get_text.
std::string get_from_cont(const RememberListType &cont, unsigned int num_str) {
	std::string text;
	auto it = cont.cbegin();
	if (cont.size() > num_str) {
		std::advance(it, cont.size() - num_str);
	}
	for (; it != cont.cend(); ++it) {
		text += *it;
	}
	return text;
}

void add_to_flaged_cont(RememberWiznetListType &cont, const std::string &text, const int level) {
	RememberMsgPtr temp_msg_ptr(new RememberMsg);
	temp_msg_ptr->Msg = time_format() + text;
	temp_msg_ptr->level = level;
	cont.push_back(temp_msg_ptr);
	if (cont.size() > MAX_REMEMBER_NUM) {
		cont.erase(cont.begin());
	}
}

std::string get_from_flaged_cont(const RememberWiznetListType &cont, unsigned int num_str, const int level) {
	std::string text;
	RememberWiznetListType::const_iterator it = cont.begin();
	if (cont.size() > num_str) {
		std::advance(it, cont.size() - num_str);
	}
	for (; it != cont.end(); ++it) {
		if (level >= (*it)->level)
			text += (*it)->Msg;
	}
	if (text.empty()) {
		text = "Вам нечего вспомнить.\r\n";
	}
	return text;
}

} // namespace Remember

using namespace Remember;

// * Добавление строки в список (flag).
void CharRemember::add_str(std::string text, int flag) {
	std::string buffer = time_format();
	buffer += text;

	switch (flag) {
		case ALL: add_to_cont(all_, buffer);
			break;
		case PERSONAL: add_to_cont(personal_, buffer);
			break;
		case Remember::GROUP:// added by WorM  групптелы 2010.10.13
			add_to_cont(group_, buffer);
			break;
		case GOSSIP: add_to_cont(gossip, buffer);
			break;
		case OFFTOP: add_to_cont(offtop, buffer);
			break;
		case PRAY: add_to_cont(imm_pray, buffer);
			break;
		case PRAY_PERSONAL: add_to_cont(pray_, buffer);
			add_to_cont(all_, buffer);
			break;
		default:
			log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
			return;
	}
}

// * Вывод списка (flag), ограниченного числом из режима вспомнить.
std::string CharRemember::get_text(int flag) const {
	std::string buffer;

	switch (flag) {
		case ALL: buffer = get_from_cont(all_, num_str_);
			break;
		case PERSONAL: buffer = get_from_cont(personal_, num_str_);
			break;
		case Remember::GROUP:// added by WorM  групптелы 2010.10.13
			buffer = get_from_cont(group_, num_str_);
			break;
		case GOSSIP: buffer = get_from_cont(gossip, num_str_);
			break;
		case OFFTOP: buffer = get_from_cont(offtop, num_str_);
			break;
		case PRAY: buffer = get_from_cont(imm_pray, num_str_);
			break;
		case PRAY_PERSONAL: buffer = get_from_cont(pray_, num_str_);
			break;
		default:
			log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
			break;
	}
	if (buffer.empty()) {
		buffer = "Вам нечего вспомнить.\r\n";
	}
	return buffer;
}

void CharRemember::reset() {
	all_.clear();
	personal_.clear();
	pray_.clear();
	group_.clear();// added by WorM  групптелы 2010.10.13
}

bool CharRemember::set_num_str(unsigned int num) {
	if (num >= 1 && num <= MAX_REMEMBER_NUM) {
		num_str_ = num;
		return true;
	}
	return false;
}

unsigned int CharRemember::get_num_str() const {
	return num_str_;
}

void do_remember_char(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];

	if (ch->IsNpc())
		return;

	// Если без аргумента - выдает личные теллы
	if (!*argument) {
		SendMsgToChar(ch->remember_get(Remember::PERSONAL), ch);
		return;
	}

	argument = one_argument(argument, arg);

	if (utils::IsAbbr(arg, "воззвать")) {
		if (IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo)) {
			SendMsgToChar(ch->remember_get(Remember::PRAY), ch);
		} else {
			SendMsgToChar(ch->remember_get(Remember::PRAY_PERSONAL), ch);
		}
	} else if ((GetRealLevel(ch) < kLvlImmortal || IS_IMPL(ch)) && utils::IsAbbr(arg, "оффтоп")) {
		if (!ch->IsFlagged(EPrf::kStopOfftop)) {
			SendMsgToChar(ch->remember_get(Remember::OFFTOP), ch);
		} else {
			SendMsgToChar(ch, "Вам нечего вспомнить.\r\n");
		}
	} else if (utils::IsAbbr(arg, "болтать") || utils::IsAbbr(arg, "орать")) {
		SendMsgToChar(ch->remember_get(Remember::GOSSIP), ch);
	} else if (utils::IsAbbr(arg, "группа") || utils::IsAbbr(arg, "ггруппа"))
	{
		SendMsgToChar(ch->remember_get(Remember::GROUP), ch);
	} else if (utils::IsAbbr(arg, "клан") || utils::IsAbbr(arg, "гдругам")) {
		if (CLAN(ch)) {
			SendMsgToChar(CLAN(ch)->get_remember(ch->remember_get_num(), Remember::CLAN), ch);
		} else {
			SendMsgToChar(ch, "Вам нечего вспомнить.\r\n");
		}
		return;
	} else if (utils::IsAbbr(arg, "союзники") || utils::IsAbbr(arg, "альянс") || utils::IsAbbr(arg, "гсоюзникам")) {
		if (CLAN(ch)) {
			SendMsgToChar(CLAN(ch)->get_remember(ch->remember_get_num(), Remember::ALLY), ch);
		} else {
			SendMsgToChar(ch, "Вам нечего вспомнить.\r\n");
		}
		return;
	} else if (utils::IsAbbr(arg, "гбогам") && IS_IMMORTAL(ch)) {
		SendMsgToChar(get_from_flaged_cont(wiznet_, ch->remember_get_num(), GetRealLevel(ch)), ch);
		return;
	} else if (utils::IsAbbr(arg, "все")) {
		SendMsgToChar(ch->remember_get(Remember::ALL), ch);
		return;
	} else {
		if (IS_IMMORTAL(ch) && !IS_IMPL(ch))
			SendMsgToChar("Формат команды: вспомнить [без параметров|болтать|воззвать|гг|гд|гс|гб|все]\r\n", ch);
		else
			SendMsgToChar("Формат команды: вспомнить [без параметров|болтать|оффтоп|гг|гд|гс|все]\r\n", ch);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
