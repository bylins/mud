//
// Created by ubuntu on 19/12/20.
//

#include "grp/grp.main.h"
#include "fightsystem/fight.h"
#include "handler.h"

void perform_drop_gold(CHAR_DATA * ch, int amount);

// Called when stop following persons, or stopping charm //
// This will NOT do if a character quits/dies!!          //
// При возврате 1 использовать ch нельзя, т.к. прошли через extract_char
// TODO: по всем вызовам не проходил, может еще где-то коряво вызывается, кроме передачи скакунов -- Krodo
bool stop_follower(CHAR_DATA * ch, int mode)
{
    struct follow_type *j, *k;
    int i;

    //log("[Stop follower] Start function(%s->%s)",ch ? GET_NAME(ch) : "none",
    //      ch->master ? GET_NAME(ch->master) : "none");

    if (!ch->has_master())
    {
        log("SYSERR: stop_follower(%s) without master", GET_NAME(ch));
        return (FALSE);
    }

    // для смены лидера без лишнего спама
    if (!IS_SET(mode, SF_SILENCE))
    {
        act("Вы прекратили следовать за $N4.", FALSE, ch, 0, ch->get_master(), TO_CHAR);
        act("$n прекратил$g следовать за $N4.", TRUE, ch, 0, ch->get_master(), TO_NOTVICT | TO_ARENA_LISTEN);
    }

    //log("[Stop follower] Stop horse");
    if (ch->get_master()->get_horse() == ch && ch->get_master()->ahorse())
    {
        ch->drop_from_horse();
    }
    else
    {
        act("$n прекратил$g следовать за вами.", TRUE, ch, 0, ch->get_master(), TO_VICT);
    }

    //log("[Stop follower] Remove from followers list");
    if (!ch->get_master()->followers) {
        log("[Stop follower] SYSERR: Followers absent for %s (master %s).", GET_NAME(ch), GET_NAME(ch->get_master()));
    } else if (ch->get_master()->followers->follower == ch) {
        k = ch->get_master()->followers;
        ch->get_master()->followers = k->next;

        if (!ch->get_master()->followers && !ch->get_master()->has_master()) {
            ch->get_master()->removeGroupFlags();
        }
        free(k);
    }
    else {   		// locate follower who is not head of list
        for (k = ch->get_master()->followers; k->next && k->next->follower != ch; k = k->next);
            if (!k->next) {
                log("[Stop follower] SYSERR: Undefined %s in %s followers list.", GET_NAME(ch), GET_NAME(ch->get_master()));
            }
            else {
                j = k->next;
                k->next = j->next;
                free(j);
            }
    }

    ch->set_master(nullptr);
    ch->removeGroupFlags();

    if (IS_CHARMICE(ch)|| IS_SET(mode, SF_CHARMLOST)) {
        if (affected_by_spell(ch, SPELL_CHARM)) {
            affect_from_char(ch, SPELL_CHARM);
        }
        EXTRACT_TIMER(ch) = 5;

        AFF_FLAGS(ch).unset(EAffectFlag::AFF_CHARM);

        if (ch->get_fighting()) {
            stop_fighting(ch, TRUE);
        }

        if (IS_NPC(ch)) {
            if (MOB_FLAGGED(ch, MOB_PLAYER_SUMMON)) {
                act("Налетевший ветер развеял $n3, не оставив и следа.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
                GET_LASTROOM(ch) = GET_ROOM_VNUM(ch->in_room);
                perform_drop_gold(ch, ch->get_gold());
                ch->set_gold(0);
                extract_char(ch, FALSE);
                return (TRUE);
            }
            else if (AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)) {
                AFF_FLAGS(ch).unset(EAffectFlag::AFF_HELPER);
            }
        }
    }

    if (IS_NPC(ch)
        && !MOB_FLAGGED(ch, MOB_PLAYER_SUMMON)	//Не ресетим флаги, если моб призван игроком
        && (i = GET_MOB_RNUM(ch)) >= 0)
    {
        MOB_FLAGS(ch) = MOB_FLAGS(mob_proto + i);
    }

    return (FALSE);
}

// * Called when a character that follows/is followed dies
bool die_follower(CHAR_DATA * ch)
{
    struct follow_type *j, *k = ch->followers;

    if (ch->has_master() && stop_follower(ch, SF_FOLLOWERDIE))
    {
        //  чармиса спуржили в stop_follower
        return true;
    }

    if (ch->ahorse()) {
        AFF_FLAGS(ch).unset(EAffectFlag::AFF_HORSE);
    }

    for (k = ch->followers; k; k = j)
    {
        j = k->next;
        stop_follower(k->follower, SF_MASTERDIE);
    }
    return false;
}


// Check if making CH follow VICTIM will create an illegal //
// Follow "Loop/circle"                                    //
bool circle_follow(CHAR_DATA * ch, CHAR_DATA * victim)
{
    for (auto k = victim; k; k = k->get_master())
    {
        if (k->get_master() == k)
        {
            k->set_master(nullptr);
            return false;
        }

        if (k == ch)
        {
            return true;
        }
    }

    return false;
}

// возвращает true, если последователь - чармис или ему похожая тварь
// при указании второго параметра - проверяет, что эта тварь персональная
bool isGroupedFollower(CHAR_DATA* master, CHAR_DATA* vict) {
    if (master == nullptr || vict == nullptr)
        return false;
    if IS_NPC(master)
        return false;
    // проверяем флаги, нпц-проверки внутре
    if ( IS_HORSE(vict) // конь
         || IS_CHARMICE(vict) // почармлен
         || IS_HIRED(vict)) // нанят
        return true;
    return false;
}