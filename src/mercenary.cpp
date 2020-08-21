//
// Created by ubuntu on 19/08/20.
//

#include "mercenary.h"
#include "char_player.hpp"

namespace MERC {
    CHAR_DATA * findMercboss(int room_rnum)
    {
        for (const auto tch : world[room_rnum]->people)
            if (GET_MOB_SPEC(tch) == mercenary)
                return tch;
        return nullptr;
    }
    void doList(CHAR_DATA *ch){
        std::map<int, MERCDATA> *m;
        m = ch->getMercList();
        CHAR_DATA * boss = findMercboss(ch->in_room);
        if (m->empty()) {
            tell_to_char(boss, ch, "Иди, поначалу, заведи знакомства, потом ко мне приходи");
            return;
        }
        std::map<int, MERCDATA>::iterator it = m->begin();
        for (; it != m->end(); ++it) {
            sprintf(buf, "");
            tell_to_char(boss, ch, buf);
        }
    };
    void doStat(CHAR_DATA *ch){
    };
    void doBring(int vnum) {

        char_to_room();
        return;
    };
    void doForget(int vnum) {
    return;
    };
}

int mercenary(CHAR_DATA *ch, void* /*me*/, int cmd, char* argument)
{
    if (CMD_IS("наемник") || CMD_IS("mercenary") )
    {
        one_argument(argument, arg);
        if (is_abbrev(arg, "стат") || is_abbrev(arg, "stat")) {
            return (1);
        }
        else if (is_abbrev(arg, "список") || is_abbrev(arg, "list")) {
            send_to_char("Чтобы привести кого-то ненужного, надо сперва нанять кого-то ненужного!\r\n", ch);
            return (1);
        }
        else if (is_abbrev(arg, "привести") || is_abbrev(arg, "bring")) {
            send_to_char("Братва попряталась по шконарям, извиняйте...\r\n", ch);
            return (1);
        }
        else if (is_abbrev(arg, "забыть") || is_abbrev(arg, "forget")) {
            send_to_char("Никто не забыт, и ничто не забыто.\r\n", ch);
            return (1);
        }

    }
    else
        return (0);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

