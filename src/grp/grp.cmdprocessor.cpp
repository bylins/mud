/*
 * Функции, подключаемые в движок процессора команд игрока
 */

#include "grp.cmdprocessor.h"
#include "grp.requestmanager.h"

extern GroupRoster& groupRoster;

void do_grequest(CHAR_DATA *ch, char *argument, int, int){
    char subcmd[MAX_INPUT_LENGTH], target[MAX_INPUT_LENGTH];
    // гзаявка список
    // гзаявка создать Верий - отправляет заявку в группу
    // гзаявка отменить Верий - отменяет заявку в группу

    RequestManager rm = groupRoster.getRequestManager();
    two_arguments(argument, subcmd, target);
    /*печать перечня заявок*/
    if (!*subcmd) {
        send_to_char(ch, "Формат команды:\r\n");
        send_to_char(ch, "гзаявка список - для получения списка заявок\r\n");
        send_to_char(ch, "гзаявка создать Верий - отправляет заявку в группу лидера Верий\r\n");
        send_to_char(ch, "гзаявка отменить Верий - отменяет заявку в группу лидера Верий\r\n");
        return;
    }
    else if (isname(subcmd, "список list")) {
        rm.printRequestList(ch);
        return;
    }
        /*создание заявки*/
    else if (isname(subcmd, "создать отправить make send")) {
        rm.makeRequest(ch, target);
        return;
    }
        /*отзыв заявки*/
    else if (isname(subcmd, "отменить отозвать cancel revoke")){
        rm.revokeRequest(ch, target);
        return;
    }
    else {
        send_to_char("Уточните команду.\r\n", ch);
        return;
    }

}
