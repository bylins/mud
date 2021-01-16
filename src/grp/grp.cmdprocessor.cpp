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
#include "grp.main.h"


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

    }
    return (TRUE);
}

int max_group_size(CHAR_DATA *ch)
{
    int bonus_commander = 0;
    if (AFF_FLAGGED(ch, EAffectFlag::AFF_COMMANDER)) bonus_commander = VPOSI((ch->get_skill(SKILL_LEADERSHIP) - 120) / 10, 0 , 8);

    return MAX_GROUPED_FOLLOWERS + (int) VPOSI((ch->get_skill(SKILL_LEADERSHIP) - 80) / 5, 0, 4) + bonus_commander;
}

void do_group2(CHAR_DATA *ch, char *argument, int, int){
    groupRoster.processGroupCommands(ch, argument);
}

void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
    groupRoster.processGroupCommands(ch, argument);
}


void do_gsay(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA* gc;

    if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
        || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
        send_to_char("Вы немы, как рыба об лед.\r\n", ch);
        return;
    }

    if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB)) {
        send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
        return;
    }

    skip_spaces(&argument);

    if (!IN_GROUP(ch)) {
        send_to_char("Вы не являетесь членом группы!\r\n", ch);
        return;
    }
    if (!*argument) {
        send_to_char("О чем вы хотите сообщить своей группе?\r\n", ch);
        return;
    }

    auto group = ch->personGroup;
    sprintf(smallBuf, "$n сообщил$g группе : '%s'", argument);

    for (auto it : *group) {
        gc = it.second->member;
        if (gc == nullptr || it.second->type == GM_CHARMEE || ignores(gc, ch, IGNORE_GROUP))
            continue;
        if (gc == ch) {
            if (!PRF_FLAGGED(ch, PRF_NOREPEAT)) {
                sprintf(buf, "Вы сообщили группе : '%s'\r\n", argument);
                send_to_char(buf, ch);
                // added by WorM  групптелы 2010.10.13
                ch->remember_add(buf, Remember::ALL);
                ch->remember_add(buf, Remember::GROUP);
            } else {
                send_to_char(OK, ch);
            }
            continue;

        }
        act(smallBuf, FALSE, ch, 0, gc, TO_VICT | TO_SLEEP | CHECK_DEAF);
        if (!AFF_FLAGGED(gc, EAffectFlag::AFF_DEAFNESS)
            && GET_POS(gc) > POS_DEAD)
        {
            sprintf(buf, "%s сообщил%s группе : '%s'\r\n", tell_can_see(ch, gc) ? GET_NAME(ch) : "Кто-то", GET_CH_VIS_SUF_1(ch, gc), argument);
            gc->remember_add(buf, Remember::ALL);
            gc->remember_add(buf, Remember::GROUP);
        }
    }

}


void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA *k;
    struct follow_type *f;

    if (!IN_GROUP(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
        send_to_char("И перед кем вы отчитываетесь?\r\n", ch);
        return;
    }
    // готовим строку
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
        for (const auto& aff : ch->affected) {
            if (aff->type == SPELL_CHARM) {
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
    if (IN_GROUP(ch))
        ch->personGroup->actToGroup(nullptr, nullptr, grpActMode(GC_LEADER | GC_REST), buf);
    else {//чармис докладывает мастеру
        k = ch->has_master() ? ch->get_master() : ch;
        for (f = k->followers; f; f = f->next) {
            if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_DEAFNESS) && f->follower != ch) {
                send_to_char(buf, f->follower);
            }
        }
        if (k != ch && !AFF_FLAGGED(k, EAffectFlag::AFF_DEAFNESS)) {
            send_to_char(buf, k);
        }
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
                && SAME_ROOM(f->follower, ch)) {
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

grpActMode::grpActMode(grpActMode::GRP_COMM val) {
    this->set();

}
