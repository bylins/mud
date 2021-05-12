#ifndef BYLINS_TELEGRAM_H
#define BYLINS_TELEGRAM_H

#if defined(HAVE_TG)
#include <curl/curl.h>
#endif

#include <string>

class CHAR_DATA;

void do_telegram(CHAR_DATA *ch, char *argument, int, int);

class TelegramBot {
 private:
#if defined(HAVE_TG)
  CURL *curl;
#endif
  const std::string msgStr = "chat_id=358708535&text=";
  const std::string chatIdParam = "chat_id=";
  const std::string textParam = "&text=";
  const std::string uri = "https://api.telegram.org/bot1330963555:AAHvh-gXBRxJHVKOmjsl8E73TJr0cO2eC50/sendMessage";
  const unsigned long debugChatId = 358708535;
 public:
  TelegramBot();
  ~TelegramBot();
  bool sendMessage(unsigned long chatId, const std::string &msg);
};

#endif //BYLINS_TELEGRAM_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
