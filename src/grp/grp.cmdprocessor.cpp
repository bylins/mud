/*
 * Функции, подключаемые в движок процессора команд игрока
 */

#include "global.objects.hpp"
#include "handler.h"
#include "house.h"
#include "screen.h"
#include "msdp.constants.hpp"
#include "magic.h"
#include "remember.hpp"

extern GroupRoster& groupRoster;

void do_grequest(CHAR_DATA *ch, char *argument, int, int){
    char subcmd[MAX_INPUT_LENGTH], target[MAX_INPUT_LENGTH];
    // гзаявка список
    // гзаявка создать Верий - отправляет заявку в группу
    // гзаявка отменить Верий - отменяет заявку в группу

    two_arguments(argument, subcmd, target);
    /*печать перечня заявок*/
    if (!*subcmd) {
        send_to_char(ch, "Формат команды:\r\n");
        send_to_char(ch, "гзаявка список - для получения списка заявок\r\n");
        send_to_char(ch, "гзаявка создать Верий - отправляет заявку в группу лидера Верий\r\n");
        send_to_char(ch, "гзаявка отменить Верий - отменяет заявку в группу лидера Верий\r\n");
        send_to_char(ch, "гзаявка принять Верий - принимает заявку от лидера группы Верий\r\n");
        return;
    }
    else if (isname(subcmd, "список list")) {
        groupRoster.printRequestList(ch);
        return;
    }
        /*создание заявки*/
    else if (isname(subcmd, "создать отправить make send")) {
        groupRoster.makeRequest(ch, target);
        return;
    }
        /*отзыв заявки*/
    else if (isname(subcmd, "отменить отозвать cancel revoke")){
        groupRoster.revokeRequest(ch, target);
        return;
    }
    else if (isname(subcmd, "принять accept")){
        groupRoster.acceptInvite(ch, target);
        return;
    }else {
        send_to_char("Уточните команду.\r\n", ch);
        return;
    }

}

bool is_group_member(CHAR_DATA *ch, CHAR_DATA *vict)
{
    if (IS_NPC(vict) || !AFF_FLAGGED(vict, EAffectFlag::AFF_GROUP) || vict->get_master() != ch)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void print_list_group(CHAR_DATA *ch)
{
    CHAR_DATA *k;
    struct follow_type *f;
    int count = 1;
    k = (ch->has_master() ? ch->get_master() : ch);
    if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
    {
        send_to_char("Ваша группа состоит из:\r\n", ch);
        if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP))
        {
            sprintf(buf1, "Лидер: %s\r\n", GET_NAME(k));
            send_to_char(buf1, ch);
        }

        for (f = k->followers; f; f = f->next)
        {
            if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
            {
                continue;
            }
            sprintf(buf1, "%d. Согруппник: %s\r\n", count, GET_NAME(f->follower));
            send_to_char(buf1, ch);
            count++;
        }
    }
    else
    {
        send_to_char("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
    }
}

int perform_group(CHAR_DATA * ch, CHAR_DATA * vict)
{
    if (AFF_FLAGGED(vict, EAffectFlag::AFF_GROUP)
        || AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
        || MOB_FLAGGED(vict, MOB_ANGEL)
        || MOB_FLAGGED(vict, MOB_GHOST)
        || IS_HORSE(vict))
    {
        return (FALSE);
    }

    AFF_FLAGS(vict).set(EAffectFlag::AFF_GROUP);
    if (ch != vict)
    {
        act("$N принят$A в члены вашего кружка (тьфу-ты, группы :).", FALSE, ch, 0, vict, TO_CHAR);
        act("Вы приняты в группу $n1.", FALSE, ch, 0, vict, TO_VICT);
        act("$N принят$A в группу $n1.", FALSE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    }
    return (TRUE);
}

int max_group_size(CHAR_DATA *ch)
{
    int bonus_commander = 0;
    if (AFF_FLAGGED(ch, EAffectFlag::AFF_COMMANDER)) bonus_commander = VPOSI((ch->get_skill(SKILL_LEADERSHIP) - 120) / 10, 0 , 8);

    return MAX_GROUPED_FOLLOWERS + (int) VPOSI((ch->get_skill(SKILL_LEADERSHIP) - 80) / 5, 0, 4) + bonus_commander;
}

/**
* Смена лидера группы на персонажа с макс лидеркой.
* Сам лидер при этом остается в группе, если он живой.
* \param vict - новый лидер, если смена происходит по команде 'гр лидер имя',
* старый лидер соответственно группится обратно, если нулевой, то считаем, что
* произошла смерть старого лидера и новый выбирается по наибольшей лидерке.
*/
void change_leader(CHAR_DATA *ch, CHAR_DATA *vict)
{
    if (IS_NPC(ch)
        || ch->has_master()
        || !ch->followers)
    {
        return;
    }

    CHAR_DATA *leader = vict;
    if (!leader)
    {
        // лидер умер, ищем согрупника с максимальным скиллом лидерки
        for (struct follow_type *l = ch->followers; l; l = l->next)
        {
            if (!is_group_member(ch, l->follower))
                continue;
            if (!leader)
                leader = l->follower;
            else if (l->follower->get_skill(SKILL_LEADERSHIP) > leader->get_skill(SKILL_LEADERSHIP))
                leader = l->follower;
        }
    }

    if (!leader)
    {
        return;
    }

    // для реследования используем стандартные функции
    std::vector<CHAR_DATA *> temp_list;
    for (struct follow_type *n = 0, *l = ch->followers; l; l = n)
    {
        n = l->next;
        if (!is_group_member(ch, l->follower))
        {
            continue;
        }
        else
        {
            CHAR_DATA *temp_vict = l->follower;
            if (temp_vict->has_master()
                && stop_follower(temp_vict, SF_SILENCE))
            {
                continue;
            }

            if (temp_vict != leader)
            {
                temp_list.push_back(temp_vict);
            }
        }
    }

    // вся эта фигня только для того, чтобы при реследовании пройтись по списку в обратном
    // направлении и сохранить относительный порядок следования в группе
    if (!temp_list.empty())
    {
        for (std::vector<CHAR_DATA *>::reverse_iterator it = temp_list.rbegin(); it != temp_list.rend(); ++it)
        {
            leader->add_follower_silently(*it);
        }
    }

    // бывшего лидера последним закидываем обратно в группу, если он живой
    if (vict)
    {
        // флаг группы надо снять, иначе при регрупе не будет писаться о старом лидере
        //AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
        ch->removeGroupFlags();
        leader->add_follower_silently(ch);
    }

    if (!leader->followers)
    {
        return;
    }

    ch->dps_copy(leader);
    perform_group(leader, leader);
    int followers = 0;
    for (struct follow_type *f = leader->followers; f; f = f->next)
    {
        if (followers < max_group_size(leader))
        {
            if (perform_group(leader, f->follower))
                ++followers;
        }
        else
        {
            send_to_char("Вы больше никого не можете принять в группу.\r\n", ch);
            return;
        }
    }
}

void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA *vict;
    struct follow_type *f;
    int found, f_number;

    argument = one_argument(argument, buf);

    if (!*buf)
    {
//        print_group(ch);
        return;
    }

    if (isname(buf, "список"))
    {
        print_list_group(ch);
        return;
    }

    if (GET_POS(ch) < POS_RESTING)
    {
        send_to_char("Трудно управлять группой в таком состоянии.\r\n", ch);
        return;
    }

    if (ch->has_master())
    {
        act("Вы не можете управлять группой. Вы еще не ведущий.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!ch->followers)
    {
        send_to_char("За вами никто не следует.\r\n", ch);
        return;
    }



// вычисляем количество последователей
    for (f_number = 0, f = ch->followers; f; f = f->next)
    {
        if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
        {
            f_number++;
        }
    }

    if (!str_cmp(buf, "all")
        || !str_cmp(buf, "все"))
    {
        perform_group(ch, ch);
        for (found = 0, f = ch->followers; f; f = f->next)
        {
            if ((f_number + found) >= max_group_size(ch))
            {
                send_to_char("Вы больше никого не можете принять в группу.\r\n", ch);
                return;
            }
            found += perform_group(ch, f->follower);
        }

        if (!found)
        {
            send_to_char("Все, кто за вами следуют, уже включены в вашу группу.\r\n", ch);
        }

        return;
    }
    else if (!str_cmp(buf, "leader") || !str_cmp(buf, "лидер"))
    {
        vict = get_player_vis(ch, argument, FIND_CHAR_WORLD);
        // added by WorM (Видолюб) Если найден клон и его хозяин персонаж
        // а то чото как-то глючно Двойник %1 не является членом вашей группы.
        if (vict
            && IS_NPC(vict)
            && MOB_FLAGGED(vict, MOB_CLONE)
            && AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
            && vict->has_master()
            && !IS_NPC(vict->get_master()))
        {
            if (CAN_SEE(ch, vict->get_master()))
            {
                vict = vict->get_master();
            }
            else
            {
                vict = NULL;
            }
        }

        // end by WorM
        if (!vict)
        {
            send_to_char("Нет такого персонажа.\r\n", ch);
            return;
        }
        else if (vict == ch)
        {
            send_to_char("Вы и так лидер группы...\r\n", ch);
            return;
        }
        else if (!AFF_FLAGGED(vict, EAffectFlag::AFF_GROUP)
                 || vict->get_master() != ch)
        {
            send_to_char(ch, "%s не является членом вашей группы.\r\n", GET_NAME(vict));
            return;
        }
        change_leader(ch, vict);
        return;
    }

    if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
    {
        send_to_char(NOPERSON, ch);
    }
    else if ((vict->get_master() != ch) && (vict != ch))
    {
        act("$N2 нужно следовать за вами, чтобы стать членом вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
    }
    else
    {
        if (!AFF_FLAGGED(vict, EAffectFlag::AFF_GROUP))
        {
            if (AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(vict, MOB_ANGEL) || MOB_FLAGGED(vict, MOB_GHOST) || IS_HORSE(vict))
            {
                send_to_char("Только равноправные персонажи могут быть включены в группу.\r\n", ch);
                send_to_char("Только равноправные персонажи могут быть включены в группу.\r\n", vict);
            };
            if (f_number >= max_group_size(ch))
            {
                send_to_char("Вы больше никого не можете принять в группу.\r\n", ch);
                return;
            }
            perform_group(ch, ch);
            perform_group(ch, vict);
        }
        else if (ch != vict)
        {
            act("$N исключен$A из состава вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
            act("Вы исключены из группы $n1!", FALSE, ch, 0, vict, TO_VICT);
            act("$N был$G исключен$A из группы $n1!", FALSE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
            //AFF_FLAGS(vict).unset(EAffectFlag::AFF_GROUP);
            vict->removeGroupFlags();
        }
    }
}

void do_group2(CHAR_DATA *ch, char *argument, int, int){
    groupRoster.processGroupCommands(ch, argument);
}

void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
    struct follow_type *f, *next_fol;
    CHAR_DATA *tch;

    one_argument(argument, buf);

    if (ch->has_master()
        || !(AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)))
    {
        send_to_char("Вы же не лидер группы!\r\n", ch);
        return;
    }

    if (!*buf)
    {
        sprintf(buf2, "Вы исключены из группы %s.\r\n", GET_PAD(ch, 1));
        for (f = ch->followers; f; f = next_fol)
        {
            next_fol = f->next;
            if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
            {
                //AFF_FLAGS(f->follower).unset(EAffectFlag::AFF_GROUP);
                f->follower->removeGroupFlags();
                send_to_char(buf2, f->follower);
                if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
                    && !(IS_NPC(f->follower)
                         && AFF_FLAGGED(f->follower, EAffectFlag::AFF_HORSE)))
                {
                    stop_follower(f->follower, SF_EMPTY);
                }
            }
        }
        AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
        ch->removeGroupFlags();
        send_to_char("Вы распустили группу.\r\n", ch);
        return;
    }
    for (f = ch->followers; f; f = next_fol)
    {
        next_fol = f->next;
        tch = f->follower;
        if (isname(buf, tch->get_pc_name())
            && !AFF_FLAGGED(tch, EAffectFlag::AFF_CHARM)
            && !IS_HORSE(tch))
        {
            //AFF_FLAGS(tch).unset(EAffectFlag::AFF_GROUP);
            tch->removeGroupFlags();
            act("$N более не член вашей группы.", FALSE, ch, 0, tch, TO_CHAR);
            act("Вы исключены из группы $n1!", FALSE, ch, 0, tch, TO_VICT);
            act("$N был$G изгнан$A из группы $n1!", FALSE, ch, 0, tch, TO_NOTVICT | TO_ARENA_LISTEN);
            stop_follower(tch, SF_EMPTY);
            return;
        }
    }
    send_to_char("Этот игрок не входит в состав вашей группы.\r\n", ch);
    return;
}

void do_gsay(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA *k;
    struct follow_type *f;

    if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
        || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
    {
        send_to_char("Вы немы, как рыба об лед.\r\n", ch);
        return;
    }

    if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
    {
        send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
        return;
    }

    skip_spaces(&argument);

    if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
    {
        send_to_char("Вы не являетесь членом группы!\r\n", ch);
        return;
    }

    if (!*argument)
    {
        send_to_char("О чем вы хотите сообщить своей группе?\r\n", ch);
    }
    else
    {
        if (ch->has_master())
        {
            k = ch->get_master();
        }
        else
        {
            k = ch;
        }

        sprintf(buf, "$n сообщил$g группе : '%s'", argument);

        if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP)
            && k != ch
            && !ignores(k, ch, IGNORE_GROUP))
        {
            act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP | CHECK_DEAF);
            // added by WorM  групптелы 2010.10.13
            if(!AFF_FLAGGED(k, EAffectFlag::AFF_DEAFNESS)
               && GET_POS(k) > POS_DEAD)
            {
                sprintf(buf1, "%s сообщил%s группе : '%s'\r\n", tell_can_see(ch, k) ? GET_NAME(ch) : "Кто-то", GET_CH_VIS_SUF_1(ch, k), argument);
                k->remember_add(buf1, Remember::ALL);
                k->remember_add(buf1, Remember::GROUP);
            }
            //end by WorM
        }
        for (f = k->followers; f; f = f->next)
        {
            if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
                && (f->follower != ch)
                && !ignores(f->follower, ch, IGNORE_GROUP))
            {
                act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP | CHECK_DEAF);
                // added by WorM  групптелы 2010.10.13
                if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_DEAFNESS)
                    && GET_POS(f->follower) > POS_DEAD)
                {
                    sprintf(buf1, "%s сообщил%s группе : '%s'\r\n", tell_can_see(ch, f->follower) ? GET_NAME(ch) : "Кто-то", GET_CH_VIS_SUF_1(ch, f->follower), argument);
                    f->follower->remember_add(buf1, Remember::ALL);
                    f->follower->remember_add(buf1, Remember::GROUP);
                }
                //end by WorM
            }
        }

        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(OK, ch);
        else
        {
            sprintf(buf, "Вы сообщили группе : '%s'\r\n", argument);
            send_to_char(buf, ch);
            // added by WorM  групптелы 2010.10.13
            ch->remember_add(buf, Remember::ALL);
            ch->remember_add(buf, Remember::GROUP);
            //end by WorM
        }
    }
}


void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA *k;
    struct follow_type *f;

    if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
    {
        send_to_char("И перед кем вы отчитываетесь?\r\n", ch);
        return;
    }
    if (ch->is_druid())
    {
        sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %d(%d)M\r\n",
                GET_NAME(ch), GET_CH_SUF_1(ch),
                GET_HIT(ch), GET_REAL_MAX_HIT(ch),
                GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
                GET_MANA_STORED(ch), GET_MAX_MANA(ch));
    }
    else if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
    {
        int loyalty = 0;
        for (const auto& aff : ch->affected)
        {
            if (aff->type == SPELL_CHARM)
            {
                loyalty = aff->duration / 2;
                break;
            }
        }
        sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %dL\r\n",
                GET_NAME(ch), GET_CH_SUF_1(ch),
                GET_HIT(ch), GET_REAL_MAX_HIT(ch),
                GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
                loyalty);
    }
    else
    {
        sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V\r\n",
                GET_NAME(ch), GET_CH_SUF_1(ch),
                GET_HIT(ch), GET_REAL_MAX_HIT(ch),
                GET_MOVE(ch), GET_REAL_MAX_MOVE(ch));
    }
    CAP(buf);
    k = ch->has_master() ? ch->get_master() : ch;
    for (f = k->followers; f; f = f->next)
    {
        if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
            && f->follower != ch
            && !AFF_FLAGGED(f->follower, EAffectFlag::AFF_DEAFNESS))
        {
            send_to_char(buf, f->follower);
        }
    }

    if (k != ch && !AFF_FLAGGED(k, EAffectFlag::AFF_DEAFNESS))
    {
        send_to_char(buf, k);
    }
    send_to_char("Вы доложили о состоянии всем членам вашей группы.\r\n", ch);
}

void do_split(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/,int currency)
{
    int amount, num, share, rest;
    CHAR_DATA *k;
    struct follow_type *f;

    if (IS_NPC(ch))
        return;

    one_argument(argument, buf);

    int what_currency;

    switch (currency) {
        case currency::ICE :
            what_currency = WHAT_ICEu;
            break;
        default :
            what_currency = WHAT_MONEYu;
            break;
    }

    if (is_number(buf)) {
        amount = atoi(buf);
        if (amount <= 0)
        {
            send_to_char("И как вы это планируете сделать?\r\n", ch);
            return;
        }

        if (amount > ch->get_gold() && currency == currency::GOLD)
        {
            send_to_char("И где бы взять вам столько денег?.\r\n", ch);
            return;
        }
        k = ch->has_master() ? ch->get_master() : ch;

        if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP)
            && (SAME_ROOM(k, ch)))
        {
            num = 1;
        }
        else
        {
            num = 0;
        }

        for (f = k->followers; f; f = f->next)
        {
            if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
                && !IS_NPC(f->follower)
                && SAME_ROOM(f->follower) == ch->in_room)
            {
                num++;
            }
        }

        if (num && AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
        {
            share = amount / num;
            rest = amount % num;
        }
        else
        {
            send_to_char("С кем вы хотите разделить это добро?\r\n", ch);
            return;
        }
        //MONEY_HACK

        switch (currency) {
            case currency::ICE :
                ch->sub_ice_currency(share* (num - 1));
                break;
            case currency::GOLD :
                ch->remove_gold(share * (num - 1));
                break;
        }

        sprintf(buf, "%s разделил%s %d %s; вам досталось %d.\r\n",
                GET_NAME(ch), GET_CH_SUF_1(ch), amount, desc_count(amount, what_currency), share);
        if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP) && IN_ROOM(k) == ch->in_room && !IS_NPC(k) && k != ch)
        {
            send_to_char(buf, k);
            switch (currency)
            {
                case currency::ICE :
                {
                    k->add_ice_currency(share);
                    break;
                }
                case currency::GOLD :
                {
                    k->add_gold(share, true, true);
                    break;
                }
            }
        }
        for (f = k->followers; f; f = f->next)
        {
            if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
                && !IS_NPC(f->follower)
                && IN_ROOM(f->follower) == ch->in_room
                && f->follower != ch)
            {
                send_to_char(buf, f->follower);
                switch (currency) {
                    case currency::ICE :
                        f->follower->add_ice_currency(share);
                        break;
                    case currency::GOLD :
                        f->follower->add_gold(share, true, true);
                        break;
                }
            }
        }
        sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
                amount, desc_count(amount, what_currency), num, share);
        if (rest)
        {
            sprintf(buf + strlen(buf),
                    "Как истинный еврей вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
                    rest, desc_count(rest, what_currency));
        }

        send_to_char(buf, ch);
        // клан-налог лутера с той части, которая пошла каждому в группе
        if (currency == currency::GOLD) {
            const long clan_tax = ClanSystem::do_gold_tax(ch, share);
            ch->remove_gold(clan_tax);
        }
    }
    else
    {
        send_to_char("Сколько и чего вы хотите разделить?\r\n", ch);
        return;
    }
}

void do_split(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
    do_split(ch,argument,0,0,0);
}
