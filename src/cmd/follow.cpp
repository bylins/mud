#include "cmd/cmd.generic.h"

#include "fightsystem/fight.h"
#include "grp/grp.main.h"
#include "handler.h"

void do_follow(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA *leader;
    struct follow_type *f;
    one_argument(argument, smallBuf);

    if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->get_fighting())
        return;
    if (*smallBuf)
    {
        if (!str_cmp(smallBuf, "я") || !str_cmp(smallBuf, "self") || !str_cmp(smallBuf, "me"))
        {
            if (!ch->has_master())
            {
                send_to_char("Но вы ведь ни за кем не следуете...\r\n", ch);
            }
            else
            {
                stop_follower(ch, SF_EMPTY);
            }
            return;
        }
        if (!(leader = get_char_vis(ch, smallBuf, FIND_CHAR_ROOM)))
        {
            send_to_char(NOPERSON, ch);
            return;
        }
    }
    else
    {
        send_to_char("За кем вы хотите следовать?\r\n", ch);
        return;
    }

    if (ch->get_master() == leader)
    {
        act("Вы уже следуете за $N4.", FALSE, ch, 0, leader, TO_CHAR);
        return;
    }

    if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
        && ch->has_master())
    {
        act("Но вы можете следовать только за $N4!", FALSE, ch, 0, ch->get_master(), TO_CHAR);
    }
    else  		// Not Charmed follow person
    {
        if (leader == ch)
        {
            if (!ch->has_master())
            {
                send_to_char("Вы уже следуете за собой.\r\n", ch);
                return;
            }
            stop_follower(ch, SF_EMPTY);
        }
        else
        {
            if (circle_follow(ch, leader))
            {
                send_to_char("Так у вас целый хоровод получится.\r\n", ch);
                return;
            }

            if (ch->has_master())
            {
                stop_follower(ch, SF_EMPTY);
            }
            //AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
            ch->removeGroupFlags();
            for (f = ch->followers; f; f = f->next)
            {
                //AFF_FLAGS(f->follower).unset(EAffectFlag::AFF_GROUP);
                f->follower->removeGroupFlags();
            }

            leader->add_follower(ch);
        }
    }
}
