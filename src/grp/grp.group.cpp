#include "grp.group.h"

#include "comm.h"
#include "handler.h"
#include "magic.h"
#include "msdp.constants.hpp"
#include "screen.h"

extern GroupRoster& groupRoster;

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

void print_one_line(CHAR_DATA * ch, CHAR_DATA * k, int leader, int header)
{
    int ok, ok2, div;
    const char *WORD_STATE[] = { "При смерти",
                                 "Оч.тяж.ран",
                                 "Оч.тяж.ран",
                                 " Тяж.ранен",
                                 " Тяж.ранен",
                                 "  Ранен   ",
                                 "  Ранен   ",
                                 "  Ранен   ",
                                 "Лег.ранен ",
                                 "Лег.ранен ",
                                 "Слег.ранен",
                                 " Невредим "
    };
    const char *MOVE_STATE[] = { "Истощен",
                                 "Истощен",
                                 "О.устал",
                                 " Устал ",
                                 " Устал ",
                                 "Л.устал",
                                 "Л.устал",
                                 "Хорошо ",
                                 "Хорошо ",
                                 "Хорошо ",
                                 "Отдохн.",
                                 " Полон "
    };
    const char *POS_STATE[] = { "Умер",
                                "Истекает кровью",
                                "При смерти",
                                "Без сознания",
                                "Спит",
                                "Отдыхает",
                                "Сидит",
                                "Сражается",
                                "Стоит"
    };

    if (IS_NPC(k))
    {
        if (!header)
//       send_to_char("Персонаж       | Здоровье |Рядом| Доп | Положение     | Лояльн.\r\n",ch);
            send_to_char("Персонаж            | Здоровье |Рядом| Аффект | Положение\r\n", ch);
        std::string name = GET_NAME(k);
        name[0] = UPPER(name[0]);
        sprintf(buf, "%s%-20s%s|", CCIBLU(ch, C_NRM),
                name.substr(0, 20).c_str(), CCNRM(ch, C_NRM));
        sprintf(buf + strlen(buf), "%s%10s%s|",
                color_value(ch, GET_HIT(k), GET_REAL_MAX_HIT(k)),
                WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1], CCNRM(ch, C_NRM));

        ok = ch->in_room == IN_ROOM(k);
        sprintf(buf + strlen(buf), "%s%5s%s|",
                ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));

        sprintf(buf + strlen(buf), " %s%s%s%s%s%s%s%s%s%s%s%s%s |",
                CCIRED(ch, C_NRM),
                AFF_FLAGGED(k, EAffectFlag::AFF_SANCTUARY) ? "О" : (AFF_FLAGGED(k, EAffectFlag::AFF_PRISMATICAURA) ? "П" : " "),
                CCGRN(ch, C_NRM),
                AFF_FLAGGED(k, EAffectFlag::AFF_WATERBREATH) ? "Д" : " ", CCICYN(ch, C_NRM),
                AFF_FLAGGED(k, EAffectFlag::AFF_INVISIBLE) ? "Н" : " ", CCIYEL(ch, C_NRM),
                (AFF_FLAGGED(k, EAffectFlag::AFF_SINGLELIGHT)
                 || AFF_FLAGGED(k, EAffectFlag::AFF_HOLYLIGHT)
                 || (GET_EQ(k, WEAR_LIGHT)
                     && GET_OBJ_VAL(GET_EQ(k, WEAR_LIGHT), 2))) ? "С" : " ",
                CCIBLU(ch, C_NRM), AFF_FLAGGED(k, EAffectFlag::AFF_FLY) ? "Л" : " ", CCYEL(ch, C_NRM),
                k->low_charm() ? "Т" : " ", CCNRM(ch, C_NRM));

        sprintf(buf + strlen(buf), "%-15s", POS_STATE[(int) GET_POS(k)]);

        act(buf, FALSE, ch, 0, k, TO_CHAR);
    }
    else
    {
        if (!header)
            send_to_char
                    ("Персонаж            | Здоровье |Энергия|Рядом|Учить| Аффект | Кто | Держит строй | Положение \r\n", ch);

        std::string name = GET_NAME(k);
        name[0] = UPPER(name[0]);
        sprintf(buf, "%s%-20s%s|", CCIBLU(ch, C_NRM), name.c_str(), CCNRM(ch, C_NRM));
        sprintf(buf + strlen(buf), "%s%10s%s|",
                color_value(ch, GET_HIT(k), GET_REAL_MAX_HIT(k)),
                WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1], CCNRM(ch, C_NRM));

        sprintf(buf + strlen(buf), "%s%7s%s|",
                color_value(ch, GET_MOVE(k), GET_REAL_MAX_MOVE(k)),
                MOVE_STATE[posi_value(GET_MOVE(k), GET_REAL_MAX_MOVE(k)) + 1], CCNRM(ch, C_NRM));

        ok = ch->in_room == IN_ROOM(k);
        sprintf(buf + strlen(buf), "%s%5s%s|",
                ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));

        if ((!IS_MANA_CASTER(k) && !MEMQUEUE_EMPTY(k)) ||
            (IS_MANA_CASTER(k) && GET_MANA_STORED(k) < GET_MAX_MANA(k)))
        {
            div = mana_gain(k);
            if (div > 0)
            {
                if (!IS_MANA_CASTER(k))
                {
                    ok2 = MAX(0, 1 + GET_MEM_TOTAL(k) - GET_MEM_COMPLETED(k));
                    ok2 = ok2 * 60 / div;	// время мема в сек
                }
                else
                {
                    ok2 = MAX(0, 1 + GET_MAX_MANA(k) - GET_MANA_STORED(k));
                    ok2 = ok2 / div;	// время восстановления в секундах
                }
                ok = ok2 / 60;
                ok2 %= 60;
                if (ok > 99)
                    sprintf(buf + strlen(buf), "&g%5d&n|", ok);
                else
                    sprintf(buf + strlen(buf), "&g%2d:%02d&n|", ok, ok2);
            }
            else
            {
                sprintf(buf + strlen(buf), "&r    -&n|");
            }
        }
        else
            sprintf(buf + strlen(buf), "     |");

        sprintf(buf + strlen(buf), " %s%s%s%s%s%s%s%s%s%s%s%s%s |",
                CCIRED(ch, C_NRM), AFF_FLAGGED(k, EAffectFlag::AFF_SANCTUARY) ? "О" : (AFF_FLAGGED(k, EAffectFlag::AFF_PRISMATICAURA)
                                                                                       ? "П" : " "), CCGRN(ch,
                                                                                                           C_NRM),
                AFF_FLAGGED(k, EAffectFlag::AFF_WATERBREATH) ? "Д" : " ", CCICYN(ch,
                                                                                 C_NRM),
                AFF_FLAGGED(k, EAffectFlag::AFF_INVISIBLE) ? "Н" : " ", CCIYEL(ch, C_NRM), (AFF_FLAGGED(k, EAffectFlag::AFF_SINGLELIGHT)
                                                                                            || AFF_FLAGGED(k, EAffectFlag::AFF_HOLYLIGHT)
                                                                                            || (GET_EQ(k, WEAR_LIGHT)
                                                                                                &&
                                                                                                GET_OBJ_VAL(GET_EQ
                                                                                                            (k, WEAR_LIGHT),
                                                                                                            2))) ? "С" : " ",
                CCIBLU(ch, C_NRM), AFF_FLAGGED(k, EAffectFlag::AFF_FLY) ? "Л" : " ", CCYEL(ch, C_NRM),
                k->ahorse() ? "В" : " ", CCNRM(ch, C_NRM));

        sprintf(buf + strlen(buf), "%5s|", leader ? "Лидер" : "");
        ok = PRF_FLAGGED(k, PRF_SKIRMISHER);
        sprintf(buf + strlen(buf), "%s%-14s%s|",	ok ? CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));
        sprintf(buf + strlen(buf), " %s", POS_STATE[(int) GET_POS(k)]);
        act(buf, FALSE, ch, 0, k, TO_CHAR);
    }
}

void print_group(CHAR_DATA * ch)
{
    int gfound = 0, cfound = 0;
    CHAR_DATA *k;
    struct follow_type *f, *g;

    k = ch->has_master() ? ch->get_master() : ch;
    if (!IS_NPC(ch))
        ch->desc->msdp_report(msdp::constants::GROUP);

    if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
    {
        send_to_char("Ваша группа состоит из:\r\n", ch);
        if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP))
        {
            print_one_line(ch, k, TRUE, gfound++);
        }

        for (f = k->followers; f; f = f->next)
        {
            if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
            {
                continue;
            }
            print_one_line(ch, f->follower, FALSE, gfound++);
        }
    }

    for (f = ch->followers; f; f = f->next)
    {
        if (!(AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
              || MOB_FLAGGED(f->follower, MOB_ANGEL)|| MOB_FLAGGED(f->follower, MOB_GHOST)))
        {
            continue;
        }
        if (!cfound)
            send_to_char("Ваши последователи:\r\n", ch);
        print_one_line(ch, f->follower, FALSE, cfound++);
    }
    if (!gfound && !cfound)
    {
        send_to_char("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
        return;
    }
    if (PRF_FLAGGED(ch, PRF_SHOWGROUP))
    {
        for (g = k->followers, cfound = 0; g; g = g->next)
        {
            for (f = g->follower->followers; f; f = f->next)
            {
                if (!(AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
                      || MOB_FLAGGED(f->follower, MOB_ANGEL) || MOB_FLAGGED(f->follower, MOB_GHOST))
                    || !AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
                {
                    continue;
                }

                if (f->follower->get_master() == ch
                    || !AFF_FLAGGED(f->follower->get_master(), EAffectFlag::AFF_GROUP))
                {
                    continue;
                }

                // shapirus: при включенном режиме не показываем клонов и хранителей
                if (PRF_FLAGGED(ch, PRF_NOCLONES)
                    && IS_NPC(f->follower)
                    && (MOB_FLAGGED(f->follower, MOB_CLONE)
                        || GET_MOB_VNUM(f->follower) == MOB_KEEPER))
                {
                    continue;
                }

                if (!cfound)
                {
                    send_to_char("Последователи членов вашей группы:\r\n", ch);
                }
                print_one_line(ch, f->follower, FALSE, cfound++);
            }

            if (ch->has_master())
            {
                if (!(AFF_FLAGGED(g->follower, EAffectFlag::AFF_CHARM)
                      || MOB_FLAGGED(g->follower, MOB_ANGEL) || MOB_FLAGGED(g->follower, MOB_GHOST))
                    || !AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
                {
                    continue;
                }

                // shapirus: при включенном режиме не показываем клонов и хранителей
                if (PRF_FLAGGED(ch, PRF_NOCLONES)
                    && IS_NPC(g->follower)
                    && (MOB_FLAGGED(g->follower, MOB_CLONE)
                        || GET_MOB_VNUM(g->follower) == MOB_KEEPER))
                {
                    continue;
                }

                if (!cfound)
                {
                    send_to_char("Последователи членов вашей группы:\r\n", ch);
                }
                print_one_line(ch, g->follower, FALSE, cfound++);
            }
        }
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
    if (ch->personGroup == nullptr)
        ch->personGroup = groupRoster.addGroup(ch);
    if (vict->personGroup == nullptr)
    {
        vict->personGroup = ch->personGroup;
        vict->personGroup->addMember(ch);
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
        print_group(ch);
        return;
    }

    if (!str_cmp(buf, "список"))
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
    ch->personGroup->processGroupCommands(ch, argument);
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

Group::Group(CHAR_DATA *leader, u_long uid){
    this->_memberCap = IS_NPC(leader)? 255 : max_group_size(leader);
    this->_uid = uid;
    // заодно иничим остальные поля
    GroupManager mgr = this->getMemberManager();
    mgr->addMember(leader);
    this->setLeader(leader);
}



void Group::removeMember(CHAR_DATA *member) {
    if (this->_currentMemberCount == 0) {
        sprintf(buf, "ROSTER: попытка удалить из группы при текущем индексе 0, id=%lu", this->_uid);
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    auto it = this->_memberList.find(member->get_uid());
    if (it != this->_memberList.end())
        this->_memberList.erase(it);
    member->personGroup = nullptr;
}

CHAR_DATA *Group::getLeader() const {
    return _leader;
}

void Group::setLeader(CHAR_DATA *leader) {
    this->_leaderUID = leader->get_uid();
    _leader = leader;
    _leaderName = leader->get_pc_name();
}

bool Group::isActive() {
    return false;
}

bool Group::checkMember(int uid) {
    return false;
}

MemberManager::MemberManager(const std::map<int, char_info *> &memberList) : _memberList(memberList) {}

u_long Group::getUid() const {
    return _uid;
}

u_short Group::getCurrentMemberCount() const {
    return _currentMemberCount;
}

int Group::getMemberCap() const {
    return _memberCap;
}

Group::~Group() {
    // чистим ссылки
    for (auto & it : this->_memberList){
        if (!it.second->member || it.second->member->purged())
            continue;
        if (it.second->member != this->_leader)
            send_to_char(it.second->member, "Ваша группа была распущена.\r\n");
        it.second->member->personGroup = nullptr;
    }
}

bool Group::approveRequest(char *applicant) {
    if ((int)this->_memberList.size() == this->_memberCap){
        send_to_char(this->_leader, "Чтобы принять кого нужного, надо сперва исключить кого ненужного!\r\n");
        return false;
    }
    return true;
}

void Group::printGroup(CHAR_DATA *requestor) {

}

void Group::processGroupCommands(CHAR_DATA *ch, char *argument) {
    enum SubCmd { PRINT, HELP, LIST, MAKE, INVITE, TAKE, REJECT, EXPELL, LEADER, LEAVE, DISBAND};
    SubCmd mode;
    Group * grp;

    const std::string strHELP = "справка помощь";
    const std::string strMAKE = "создать make";
    const std::string strLIST = "список list";
    const std::string strINVITE = "пригласить invite";
    const std::string strTAKE = "принять approve";
    const std::string strREJECT = "отклонить reject";
    const std::string strEXPELL = "выгнать expell";
    const std::string strLEADER = "лидер leader";
    const std::string strLEAVE = "покинуть leave";
    const std::string strDISBAND = "распустить disband";
    char subcmd[MAX_INPUT_LENGTH],  target[MAX_INPUT_LENGTH];

    if (!ch || IS_NPC(ch))
        return;

    two_arguments(argument, subcmd, target);

    // выбираем режим
    if (!*subcmd)
        mode = PRINT;
    else if (isname(subcmd, strHELP.c_str()))
        mode = HELP;
    else if (isname(subcmd, strMAKE.c_str()))
        mode = MAKE;
    else if (isname(subcmd, strLIST.c_str()))
        mode = LIST;
    else if (isname(subcmd, strINVITE.c_str()))
        mode = INVITE;
    else if (isname(subcmd, strTAKE.c_str()))
        mode = TAKE;
    else if (isname(subcmd, strREJECT.c_str()))
        mode = REJECT;
    else if (isname(subcmd, strEXPELL.c_str()))
        mode = EXPELL;
    else if (isname(subcmd, strLEADER.c_str()))
        mode = LEADER;
    else if (isname(subcmd, strLEAVE.c_str()))
        mode = LEAVE;
    else if (isname(subcmd, strDISBAND.c_str()))
        mode = DISBAND;
    else {
        send_to_char("Уточните команду.\r\n", ch);
        return;
    }
    grp = ch->personGroup;

    if (mode == MAKE) {
        if (grp && grp->getLeader() == ch){
            send_to_char(ch, "Только древний правитель Цесарь Иулий мог водить много легионов!\r\n");
            return;
        }
        if (grp) // в группе - покидаем
            grp->removeMember(ch);
        groupRoster.addGroup(ch);
        send_to_char(ch, "Вы создали группу id:%lu, максимальное число последователей: %d\r\n",ch->personGroup->getUid(),  ch->personGroup->getMemberCap());
        return;
    }

    if (!grp || grp->getLeader() != ch ) {
        send_to_char(ch, "Необходимо быть лидером группы для этого.\r\n");
        return;
    }
    if (GET_POS(ch) < POS_RESTING) {
        send_to_char("Трудно управлять группой в таком состоянии.\r\n", ch);
        return;
    }

    // MAKE был раньше, т.к. он создает группу
    switch (mode) {
        case PRINT:
            grp->printGroup(ch);
            break;
        case HELP:
            break;
        case LIST:
            //grp->listMembers(ch);
            break;
        case INVITE:
            if (this->isFull()){
                send_to_char(ch, "Командир, но в твоей группе больше нет места!\r\n");
                return;
            }
            groupRoster.makeInvite(target);
            break;
        case TAKE:
            //grp->approveRequest(target);
            break;
        case REJECT:
            //grp->denyRequest(target);
            break;
        case EXPELL:
            //grp->expellMember(target);
            break;
        case LEADER:
            //grp->promote(target);
            break;
        case LEAVE:
            //grp->leave(ch);
            break;
        case DISBAND:
            groupRoster.removeGroup(grp->getUid());
            break;
    }
}

const std::string &Group::getLeaderName() const {
    return _leaderName;
}

bool Group::isFull() {
    if (this->_currentMemberCount == this->_memberCap)
        return true;
    return false;
}
