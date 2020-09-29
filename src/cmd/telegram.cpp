#include "telegram.h"
#include "chars/char_player.hpp"

#include <obj.hpp>

extern CHAR_DATA *get_player_of_name(const char *name);

void do_telegram(CHAR_DATA *ch, char *argument, int, int){
#if defined(HAVE_TG)
    char playerName[24];
    char output[MAX_INPUT_LENGTH];

    half_chop(argument, playerName, smallBuf);
    if (!*playerName){
        send_to_char(ch, "Не указано имя игрока.\r\n");
        return;
    }
    if (strlen(smallBuf) < 10){
        send_to_char(ch, "Коротковато сообщеньице!\r\n");
        return;
    }
    CHAR_DATA * victim = get_player_of_name(playerName);
    Player p_vict;
    if (!victim) {
        if (load_char(playerName, &p_vict) == -1) {
            send_to_char("Ошибочка вышла..\r\n", ch);
            return;
        }
    } else
        p_vict = *(dynamic_cast<Player*>(victim));

    if (p_vict.getTelegramId() == 0) {
        send_to_char(ch, "Звыняйте, барин, нэмае у той телеги колесьев...\r\n");
        return;
    }
    koi_to_utf8(const_cast<char *>(smallBuf), output);
    if (strlen(output) < 10){
        send_to_char("Ошибочка вышла..\r\n", ch);
        return;
    }
    if (!system_obj::bot->sendMessage(p_vict.getTelegramId(), output)) {
        send_to_char("Ошибочка вышла..\r\n", ch);
        return;
    };
    send_to_char("Ваша телега успешно уехала в адрес.\r\n", ch);
#else
    send_to_char("Звыняйте, телегу украли цыгане.\r\n", ch);
#endif
}

bool TelegramBot::sendMessage(unsigned long chatId, const std::string& msg) {
    if (chatId < 1 || msg.empty())
        return false;
    #if defined(HAVE_TG)
    _bot->getApi().sendMessage(chatId, msg);
    #endif
    return true;
}

TelegramBot::TelegramBot() {
    #if defined(HAVE_TG)
    _bot = new TgBot::Bot(token);
    this->sendMessage(debugChatId, "Chat-bot init successful.");
    #endif
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
