#include "telegram.h"
#include "chars/char_player.hpp"

#if defined(HAVE_TG)
#include <tgbot/tgbot.h>
#endif

extern CHAR_DATA *get_player_of_name(const char *name);

void do_telegram(CHAR_DATA *ch, char *argument, int, int){
#if defined(HAVE_TG)
    char playerName[24];

    two_arguments(argument, playerName, smallBuf);
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
        send_to_char(ch, "Звыняйте, барин, нэмае у той телеги колесьев...");
        return;
    }
    TgBot::Bot bot("1330963555:AAHvh-gXBRxJHVKOmjsl8E73TJr0cO2eC50");
//    bot.getApi().sendMessage(358708535, "bot started");

    if (!bot.getApi().sendMessage(p_vict.getTelegramId(), smallBuf)) {
        send_to_char("Ошибочка вышла..\r\n", ch);
        return;
    };
    send_to_char("Ваша телега успешно уехала в адрес.\r\n", ch);
#else
    send_to_char("Звыняйте, телегу украли цыгане.\r\n", ch);
#endif
}
