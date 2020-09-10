#include "assist.h"

#include "handler.h"
#include "pk.h"
#include "start.fight.h"

void do_assist(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
    if (ch->get_fighting()) {
        send_to_char("Невозможно. Вы сражаетесь сами.\r\n", ch);
        return;
    }

    one_argument(argument, arg);
    CHAR_DATA *helpee, *opponent;
    if (!*arg) {
        helpee = nullptr;
        for (const auto i : world[ch->in_room]->people) {
            if (i->get_fighting() && i->get_fighting() != ch
                && ((ch->has_master() && ch->get_master() == i->get_master())
                    || ch->get_master() == i || i->get_master() == ch
                    || (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->get_master() && ch->get_master()->get_master()
                        && (ch->get_master()->get_master() == i || ch->get_master()->get_master() == i->get_master())))) {
                helpee = i;
                break;
            }
        }

        if (!helpee) {
            send_to_char("Кому вы хотите помочь?\r\n", ch);
            return;
        }
    } else {
        if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
            send_to_char(NOPERSON, ch);
            return;
        }
    }
    if (helpee == ch) {
        send_to_char("Вам могут помочь только Боги!\r\n", ch);
        return;
    }

    if (helpee->get_fighting()) {
        opponent = helpee->get_fighting();
    } else {
        opponent = nullptr;
        for (const auto i : world[ch->in_room]->people) {
            if (i->get_fighting() == helpee) {
                opponent = i;
                break;
            }
        }
    }

    if (!opponent)
        act("Но никто не сражается с $N4!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
        act("Вы не видите противника $N1!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (opponent == ch)
        act("Дык $E сражается с ВАМИ!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!may_kill_here(ch, opponent, NoArgument))
        return;
    else if (need_full_alias(ch, opponent))
        act("Используйте команду 'атаковать' для нападения на $N1.", FALSE, ch, 0, opponent, TO_CHAR);
    else if (set_hit(ch, opponent)) {
        act("Вы присоединились к битве, помогая $N2!", FALSE, ch, 0, helpee, TO_CHAR);
        act("$N решил$G помочь вам в битве!", 0, helpee, 0, ch, TO_CHAR);
        act("$n вступил$g в бой на стороне $N1.", FALSE, ch, 0, helpee, TO_NOTVICT | TO_ARENA_LISTEN);
    }
}
