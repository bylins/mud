#include "cmd.generic.h"
#include "house.h"
#include "grp/grp.main.h"

void do_whoami(CHAR_DATA *ch, char *, int, int) {
    sprintf(buf, "Персонаж : %s\r\n", GET_NAME(ch));
    sprintf(buf + strlen(buf),
            "Падежи : &W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n\r\n",
            ch->get_name().c_str(), GET_PAD(ch, 1), GET_PAD(ch, 2),
            GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5));

    sprintf(buf + strlen(buf), "Ваш e-mail : &S%s&s\r\n", GET_EMAIL(ch));
    time_t birt = ch->player_data.time.birth;
    sprintf(buf + strlen(buf), "Дата вашего рождения : %s\r\n", rustime(localtime(&birt)));
    sprintf(buf + strlen(buf), "Ваш IP-адрес : %s\r\n", ch->desc ? ch->desc->host : "Unknown");
    send_to_char(buf, ch);
    if (!NAME_GOD(ch))
    {
        sprintf(buf, "Имя никем не одобрено!\r\n");
        send_to_char(buf, ch);
    }
    else
    {
        const int god_level = NAME_GOD(ch) > 1000 ? NAME_GOD(ch) - 1000 : NAME_GOD(ch);
        sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(ch)));
        *buf1 = UPPER(*buf1);

        static const char *by_rank_god = "Богом";
        static const char *by_rank_privileged = "привилегированным игроком";
        const char * by_rank = god_level < LVL_IMMORT ?  by_rank_privileged : by_rank_god;

        if (NAME_GOD(ch) < 1000)
            sprintf(buf, "&RИмя запрещено %s %s&n\r\n", by_rank, buf1);
        else
            sprintf(buf, "&WИмя одобрено %s %s&n\r\n", by_rank, buf1);
        send_to_char(buf, ch);
    }
    sprintf(buf, "Перевоплощений: %d\r\n", GET_REMORT(ch));
    send_to_char(buf, ch);
    Clan::CheckPkList(ch);
    if (ch->player_specials->saved.telegram_id != 0) { //тут прямое обращение, ибо базовый класс, а не наследник
        send_to_char(ch, "Подключен Телеграм, chat_id: %lu\r\n", ch->player_specials->saved.telegram_id);
    }
    if (ch->personGroup)
        send_to_char(ch, "Состоит в группе #%d лидера %s\r\n", ch->personGroup->getUid(), ch->personGroup->getLeaderName().c_str());
}
