//
// Created by ubuntu on 19/12/20.
//

#include "test.h"
#include "comm.h"

bool handleTestInput(CHAR_DATA *ch, char *arg) {
    switch (LOWER(*arg)) {
        case '1':
        case 'z':
        case 'я':
            STATE(ch->desc) = CON_PLAYING;
            send_to_char("Режим тестирования окончен.\r\n", ch);
            return 1;
        default:
            break;

    }
}


void do_test(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
    send_to_char(ch, "Переход в режим тестирования");
    STATE(ch->desc) = CON_IMPLTEST;
}