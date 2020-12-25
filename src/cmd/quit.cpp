#include "cmd.generic.h"
#include "fightsystem/fight_stuff.hpp"
#include "depot.hpp"
#include "handler.h"
#include "objsave.h"
#include "house.h"
#include "global.objects.hpp"

extern int free_rent;
extern GroupRoster& groupRoster;

void do_quit(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
    DESCRIPTOR_DATA *d, *next_d;

    if (IS_NPC(ch) || !ch->desc)
        return;

    if (subcmd != SCMD_QUIT)
        send_to_char("Вам стоит набрать эту команду полностью во избежание недоразумений!\r\n", ch);
    else if (GET_POS(ch) == POS_FIGHTING)
        send_to_char("Угу! Щаз-з-з! Вы, батенька, деретесь!\r\n", ch);
    else if (GET_POS(ch) < POS_STUNNED)
    {
        send_to_char("Вас пригласила к себе владелица косы...\r\n", ch);
        die(ch, nullptr);
    }
    else if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
    {
        return;
    }
    else if (*argument)
    {
        send_to_char("Если вы хотите выйти из игры с потерей всех вещей, то просто наберите 'конец'.\r\n", ch);
    }
    else
    {
//		int loadroom = ch->in_room;
        if (RENTABLE(ch))
        {
            send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
            return;
        }
        if (!GET_INVIS_LEV(ch))
            act("$n покинул$g игру.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
        sprintf(buf, "%s quit the game.", GET_NAME(ch));
        mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
        send_to_char("До свидания, странник... Мы ждем тебя снова!\r\n", ch);

        long depot_cost = static_cast<long>(Depot::get_total_cost_per_day(ch));
        if (depot_cost)
        {
            send_to_char(ch, "За вещи в хранилище придется заплатить %ld %s в день.\r\n", depot_cost, desc_count(depot_cost, WHAT_MONEYu));
            long deadline = ((ch->get_gold() + ch->get_bank()) / depot_cost);
            send_to_char(ch, "Твоих денег хватит на %ld %s.\r\n", deadline, desc_count(deadline, WHAT_DAY));
        }
        Depot::exit_char(ch);
        Clan::clan_invoice(ch, false);
        if (ch->personGroup)
            ch->personGroup->getMemberManager()->removeMember(ch);

        /*
         * kill off all sockets connected to the same player as the one who is
         * trying to quit.  Helps to maintain sanity as well as prevent duping.
         */
        for (d = descriptor_list; d; d = next_d)
        {
            next_d = d->next;
            if (d == ch->desc)
                continue;
            if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
                STATE(d) = CON_DISCONNECT;
        }

        if (free_rent || IS_GOD(ch))
        {
            Crash_rentsave(ch, 0);
        }
        extract_char(ch, FALSE);
    }
}
