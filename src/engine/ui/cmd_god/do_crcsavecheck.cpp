/**
 \file do_crcsavecheck.cpp - a part of the Bylins engine.
 \brief Сброс снимков контрольных сумм файлов игроков.
*/

#include "do_crcsavecheck.h"

#include "engine/core/comm.h"
#include "engine/entities/char_data.h"
#include "utils/file_crc.h"
#include "utils/logger.h"
#include "utils/utils.h"

#include <fmt/format.h>

#include <string>

namespace {

// Команда ищется по началу слова, поэтому одного "crc" хватило бы, чтобы ее запустить.
// Поэтому требуем слово подтверждения: случайно набранное не сделает ничего.
const char *kConfirmWord = "подтверждаю";

}  // namespace

void do_crcsavecheck(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	std::string arg = argument ? argument : "";
	utils::Trim(arg);

	if (arg != kConfirmWord) {
		SendMsgToChar(ch,
					  "Эта команда заставляет забыть все контрольные суммы файлов игроков:\r\n"
					  "следующий прочитанный файл каждого игрока станет новым эталоном.\r\n"
					  "\r\n"
					  "Нужна она после планового восстановления из архива. Файлы игроков\r\n"
					  "откатываются назад, снимки остаются от более позднего состояния, и\r\n"
					  "сверка потом несколько дней ругается на каждого входящего.\r\n"
					  "\r\n"
					  "Пока суммы забыты, подмену файла мимо сервера заметить нельзя.\r\n"
					  "Если вы уверены: crcsavecheck %s\r\n",
					  kConfirmWord);
		return;
	}

	const auto forgotten = FileCRC::forget_all();

	SendMsgToChar(ch, "Забыто контрольных сумм: %zu. Файл перезаписан.\r\n", forgotten);
	mudlog(fmt::format("{} сбросил контрольные суммы файлов игроков (забыто записей: {})",
					   GET_NAME(ch), forgotten), BRF, kLvlImplementator, SYSLOG, true);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
