#include "telegram.h"
#include "chars/char_player.hpp"
#include "chars/world.characters.hpp"

#include <obj.hpp>

void do_telegram(CHAR_DATA *ch, char *argument, int, int) {
#if defined(HAVE_TG)
  unsigned long int tgId = 0;
  bool found = false;
  char playerName[24];
  char output[MAX_INPUT_LENGTH];
  char utfBuf[MAX_RAW_INPUT_LENGTH];

  half_chop(argument, playerName, output);
  if (!*playerName) {
    send_to_char(ch, "Не указано имя игрока.\r\n");
    return;
  }
  if (strlen(output) < 10) {
    send_to_char(ch, "Коротковато сообщеньице!\r\n");
    return;
  }
  for (const auto &character : character_list) {
    const auto i = character.get();
    if (isname(playerName, i->get_pc_name())) {
      found = true;
      tgId = i->player_specials->saved.telegram_id;
      break;
    }
  }
  Player p_vict;
  if (!found) {
//        send_to_char(ch, "Не нашли онлайн, ищем в файле.\r\n");
    if (load_char(playerName, &p_vict) == -1) {
      send_to_char("Ошибочка вышла..\r\n", ch);
      return;
    }
    tgId = p_vict.getTelegramId();
  }

  if (tgId == 0) {
    send_to_char(ch, "Звыняйте, барин, нэмае у той телеги колесьев...\r\n");
    return;
  }

  sprintf(smallBuf, "Поступила телега от %s, сообщают следующее:\r\n%s", GET_NAME(ch), output);
  koi_to_utf8(const_cast<char *>(smallBuf), utfBuf);
  if (strlen(utfBuf) < 10) {
    send_to_char("Ошибочка вышла..\r\n", ch);
    return;
  }
  if (!system_obj::bot->sendMessage(tgId, utfBuf)) {
    send_to_char("Ошибочка вышла..\r\n", ch);
    return;
  };
  send_to_char("Ваша телега успешно уехала в адрес.\r\n", ch);
#else
  send_to_char("Звыняйте, телегу украли цыгане.\r\n", ch);
#endif
}

size_t noop_cb(void *, size_t size, size_t nmemb, void *) {
  return size * nmemb;
}

bool TelegramBot::sendMessage(unsigned long chatId, const std::string &msg) {
  bool result = true;
#if defined(HAVE_TG)
  char msgBuf[1024];
  CURLcode res;
  curl = curl_easy_init();
  if (!curl)
    return false;
  if (chatId < 1 || msg.empty())
    return false;
  curl_easy_setopt(curl, CURLOPT_URL, this->uri.c_str());
  sprintf(msgBuf, "%s%lu%s%s",
          this->chatIdParam.c_str(), chatId,
          this->textParam.c_str(), curl_easy_escape(curl, msg.c_str(), msg.length()));
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
