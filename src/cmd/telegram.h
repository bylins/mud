#ifndef BYLINS_TELEGRAM_H
#define BYLINS_TELEGRAM_H

#if defined(HAVE_TG)
#include <tgbot/tgbot.h>
#endif
class CHAR_DATA;

void do_telegram(CHAR_DATA *ch, char *argument, int, int);

class TelegramBot {
private:
#if defined(HAVE_TG)
    TgBot::Bot* _bot;
#endif
    const std::string token = "1330963555:AAHvh-gXBRxJHVKOmjsl8E73TJr0cO2eC50";
    const unsigned long debugChatId = 358708535;
public:
    TelegramBot();
    bool sendMessage(unsigned long chatId, const std::string& msg);
};

#endif //BYLINS_TELEGRAM_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
