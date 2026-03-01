#include "do_telegram.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"

void do_telegram([[maybe_unused]] CharData *ch, [[maybe_unused]] char *argument, int, int) {
#if defined(HAVE_TG)
	unsigned long int tgId = 0;
	bool found = false;
	char playerName[24];
	char output[kMaxInputLength];
	char utfBuf[kMaxRawInputLength];

	half_chop(argument, playerName, output);
	if (!*playerName) {
		SendMsgToChar(ch, "Не указано имя игрока.\r\n");
		return;
	}
	if (strlen(output) < 10) {
		SendMsgToChar(ch, "Коротковато сообщеньице!\r\n");
		return;
	}
	for (const auto &character : character_list) {
		const auto i = character.get();
		if (isname(playerName, i->GetCharAliases())) {
			found = true;
			tgId = i->player_specials->saved.telegram_id;
			break;
		}
	}
	Player p_vict;
	if (!found) {
//        SendMsgToChar(ch, "Не нашли онлайн, ищем в файле.\r\n");
		if (LoadPlayerCharacter(playerName, &p_vict, ELoadCharFlags::kFindId) == -1) {
			SendMsgToChar("Ошибочка вышла..\r\n", ch);
			return;
		}
		tgId = p_vict.getTelegramId();
	}

	if (tgId == 0) {
		SendMsgToChar(ch, "Звыняйте, барин, нэмае у той телеги колесьев...\r\n");
		return;
	}

	snprintf(smallBuf, kMaxInputLength, "Поступила телега от %s, сообщают следующее:\r\n%s", GET_NAME(ch), output);
	koi_to_utf8(const_cast<char *>(smallBuf), utfBuf);
	if (strlen(utfBuf) < 10) {
		SendMsgToChar("Ошибочка вышла..\r\n", ch);
		return;
	}
	if (!system_obj::bot->sendMessage(tgId, utfBuf)) {
		SendMsgToChar("Ошибочка вышла..\r\n", ch);
		return;
	};
	SendMsgToChar("Ваша телега успешно уехала в адрес.\r\n", ch);
#else
	SendMsgToChar("Звыняйте, телегу украли цыгане.\r\n", ch);
#endif
}

size_t noop_cb(void *, size_t size, size_t nmemb, void *) {
	return size * nmemb;
}

bool TelegramBot::sendMessage([[maybe_unused]] unsigned long chatId, [[maybe_unused]] const std::string &msg) {
	bool result = true;
#if defined(HAVE_TG)
	char msgBuf[kMaxStringLength];
	CURLcode res;
	curl = curl_easy_init();
	if (!curl)
		return false;
	if (chatId < 1 || msg.empty())
		return false;
	curl_easy_setopt(curl, CURLOPT_URL, this->uri.c_str());
	char *escaped_msg = curl_easy_escape(curl, msg.c_str(), msg.length());
	snprintf(msgBuf, kMaxStringLength,
			this->chatIdParam.c_str(), chatId,
			this->textParam.c_str(), escaped_msg);
	curl_free(escaped_msg);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msgBuf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop_cb);

	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK)
		result = false;
	curl_easy_cleanup(curl);
#endif
	return result;
}

TelegramBot::TelegramBot() {
#if defined(HAVE_TG)
	curl_global_init(CURL_GLOBAL_ALL);
	this->sendMessage(debugChatId, "Chat-bot init successful.");
#endif
}

TelegramBot::~TelegramBot() {
#if defined(HAVE_TG)
	curl_global_cleanup();
#endif

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
