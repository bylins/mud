#include "hire.h"

#include "handler.h"
#include <boost/lexical_cast.hpp>

int max_stats2[][6] =
        // Str Dex Int Wis Con Cha //
        { {14, 13, 24, 25, 15, 10},	// Лекарь //
          {14, 12, 25, 23, 13, 16},	// Колдун //
          {19, 25, 12, 12, 17, 16},	// Вор //
          {25, 11, 15, 15, 25, 10},	// Богатырь //
          {22, 24, 14, 14, 17, 12},	// Наемник //
          {23, 17, 14, 14, 23, 12},	// Дружинник //
          {14, 12, 25, 23, 13, 16},	// Кудесник //
          {14, 12, 25, 23, 13, 16},	// Волшебник //
          {15, 13, 25, 23, 14, 12},	// Чернокнижник //
          {22, 13, 16, 19, 18, 17},	// Витязь //
          {25, 21, 16, 16, 18, 16},	// Охотник //
          {25, 17, 13, 15, 20, 16},	// Кузнец //
          {21, 17, 14, 13, 20, 17},	// Купец //
          {18, 12, 24, 18, 15, 12}	// Волхв //
        };

int min_stats2[][6] =
        // Str Dex Int Wis Con Cha //
        { {11, 10, 19, 20, 12, 10},	// Лекарь //
          {10,  9, 20, 18, 10, 13},	// Колдун //
          {16, 22,  9,  9, 14, 13},	// Вор //
          {21,  8, 11, 11, 22, 10},	// Богатырь //
          {17, 19, 11, 11, 14, 12},	// Наемник //
          {20, 14, 10, 10, 17, 12},	// Дружинник //
          {10,  9, 20, 18, 10, 13},	// Кудесник //
          {10,  9, 20, 18, 10, 13},	// Волшебник //
          { 9,  9, 20, 20, 11, 10},	// Чернокнижник //
          {19, 10, 12, 15, 14, 13},	// Витязь //
          {19, 15, 11, 11, 14, 11},	// Охотник //
          {20, 14, 10, 11, 14, 12},	// Кузнец //
          {18, 14, 10, 10, 14, 13},	// Купец //
          {15, 10, 19, 15, 12, 12}	// Волхв //
        };

//Функции для модифицированного чарма
float get_damage_per_round(CHAR_DATA * victim)
{
    float dam_per_attack = GET_DR(victim) + str_bonus(victim->get_str(), STR_TO_DAM)
                           + victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2.0
                           + (AFF_FLAGGED(victim, EAffectFlag::AFF_CLOUD_OF_ARROWS) ? 14 : 0);
    int num_attacks = 1 + victim->mob_specials.ExtraAttack
                      + (victim->get_skill(SKILL_ADDSHOT) ? 2 : 0);

    float dam_per_round = dam_per_attack * num_attacks;

    //Если дыхание - то дамаг умножается
    if (MOB_FLAGGED(victim, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH | MOB_ACIDBREATH | MOB_LIGHTBREATH)))
    {
        dam_per_round *= 1.3f;
    }

    return dam_per_round;
}

float calc_cha_for_hire(CHAR_DATA * victim)
{
    int i;
    float reformed_hp = 0.0, needed_cha = 0.0;
    for (i = 0; i < 50; i++)
    {
        reformed_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) * cha_app[i].dam_to_hit_rate;
        if (cha_app[i].charms >= reformed_hp)
            break;
    }
    i = POSI(i);
    needed_cha = i - 1 + (reformed_hp - cha_app[i - 1].charms) / (cha_app[i].charms - cha_app[i - 1].charms);
//sprintf(buf,"check: charms = %d   rhp = %f\r\n",cha_app[i].charms,reformed_hp);
//act(buf,FALSE,victim,0,0,TO_ROOM);
    return VPOSI<float>(needed_cha, 1.0, 50.0);
}

int calc_hire_price(CHAR_DATA * ch, CHAR_DATA * victim) {
    float needed_cha = calc_cha_for_hire(victim), dpr = 0.0;
    float e_cha = get_effective_cha(ch), e_int = get_effective_int(ch);
    float stat_overlimit = VPOSI<float>(e_cha + e_int - 1.0 -
                                        min_stats2[(int) GET_CLASS(ch)][5] - 1 -
                                        min_stats2[(int) GET_CLASS(ch)][2], 0, 100);

    if (stat_overlimit >= 100.0) stat_overlimit = 99.99;

    float price = 0;
    float real_cha = 1.0 + GET_LEVEL(ch) / 2.0 + stat_overlimit / 2.0;
    float difference = needed_cha - real_cha;

    dpr = get_damage_per_round(victim);

    log("MERCHANT: hero (%s) mob (%s [%5d] ) charm (%f) dpr (%f)",GET_NAME(ch),GET_NAME(victim),GET_MOB_VNUM(victim),needed_cha,dpr);



    if (difference <= 0)
        price = dpr * (1.00 - 0.01 * stat_overlimit);
    else
        price = MMIN((dpr * pow(2.0F, difference)), MAXPRICE);

    if (price <= 0.0 || (difference >= 25 && (int) dpr))
        price = MAXPRICE;

    if(WAITLESS(ch)) {
        price = 0;
    }
    return (int) ceil(price);
}

int get_reformed_charmice_hp(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum)
{
    float r_hp = 0;
    float eff_cha = 0.0;
    float max_cha;

    if (spellnum == SPELL_RESSURECTION || spellnum == SPELL_ANIMATE_DEAD)
    {
        eff_cha = get_effective_wis(ch, spellnum);
        max_cha = class_stats_limit[ch->get_class()][3];
    }
    else
    {
        max_cha = class_stats_limit[ch->get_class()][5];
        eff_cha = get_effective_cha(ch);
    }

    if (spellnum != SPELL_CHARM)
    {
        eff_cha = MMIN(max_cha, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
    }

    // Интерполяция между значениями для целых значений обаяния
    if (eff_cha < max_cha)
    {
        r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
                                     ((1 - eff_cha + (int)eff_cha) * cha_app[(int)eff_cha].dam_to_hit_rate +
                                      (eff_cha - (int)eff_cha) * cha_app[(int)eff_cha + 1].dam_to_hit_rate);
    }
    else
    {
        r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
                                     ((1 - eff_cha + (int)eff_cha) * cha_app[(int)eff_cha].dam_to_hit_rate);
    }

    if (PRF_FLAGGED(ch, PRF_TESTER))
        send_to_char(ch, "&Gget_reformed_charmice_hp Расчет чарма r_hp = %f \r\n&n", r_hp);
    return (int) r_hp;
}

void do_findhelpee(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
    if (IS_NPC(ch)
        || (!WAITLESS(ch) && !can_use_feat(ch, EMPLOYER_FEAT)))
    {
        send_to_char("Вам недоступно это!\r\n", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (!*arg) {
        follow_type* k;
        for (k = ch->followers; k; k = k->next) {
            if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER) && AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)) {
                break;
            }
        }

        if (k) {
            act("Вашим наемником является $N.", FALSE, ch, 0, k->follower, TO_CHAR);
        }
        else {
            act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
        }
        return;
    }

    const auto helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM);
    if (!helpee) {
        send_to_char("Вы не видите никого похожего.\r\n", ch);
        return;
    }

    follow_type* k;
    for (k = ch->followers; k; k = k->next) {
        if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER) && AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)) {
            break;
        }
    }

    if (helpee == ch)
        send_to_char("И как вы это представляете - нанять самого себя?\r\n", ch);
    else if (!IS_NPC(helpee))
        send_to_char("Вы не можете нанять реального игрока!\r\n", ch);
    else if (!NPC_FLAGGED(helpee, NPC_HELPED))
        act("$N не нанимается!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (AFF_FLAGGED(helpee, EAffectFlag::AFF_CHARM) && (!k  || (k && helpee != k->follower)))
        act("$N под чьим-то контролем.", FALSE, ch, 0, helpee, TO_CHAR);
    else if (AFF_FLAGGED(helpee, EAffectFlag::AFF_DEAFNESS))
        act("$N не слышит вас.", FALSE, ch, 0, helpee, TO_CHAR);
    else if (IS_HORSE(helpee))
        send_to_char("Это боевой скакун, а не хухры-мухры.\r\n", ch);
    else if (helpee->get_fighting() || GET_POS(helpee) < POS_RESTING)
        act("$M сейчас, похоже, не до вас.", FALSE, ch, 0, helpee, TO_CHAR);
    else if (circle_follow(helpee, ch))
        send_to_char("Следование по кругу запрещено.\r\n", ch);
    else
    {
        char isbank[MAX_STRING_LENGTH];
        two_arguments(argument, arg, isbank);

        int times = -1;
        try
        {
            times = boost::lexical_cast<int>(arg);
        }
        catch (boost::bad_lexical_cast&) {}

        if(!*arg || times < 0)
        {
            const auto cost = calc_hire_price(ch, helpee);
            sprintf(buf,
                    "$n сказал$g вам : \"Один час моих услуг стоит %d %s\".\r\n",
                    cost, desc_count(cost, WHAT_MONEYu));
            act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
            return;
        }

        if (k && helpee != k->follower) {
            act("Вы уже наняли $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
            return;
        }

        auto hire_price = calc_hire_price(ch, helpee);

        const auto cost = times * hire_price;

        if ((!isname(isbank, "банк bank") && cost > ch->get_gold()) ||
            (isname(isbank, "банк bank") && cost > ch->get_bank()))
        {
            sprintf(buf,
                    "$n сказал$g вам : \" Мои услуги за %d %s стоят %d %s - это тебе не по карману.\"",
                    times, desc_count(times, WHAT_HOUR), cost, desc_count(cost, WHAT_MONEYu));
            act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
            return;
        }

        if (helpee->has_master()
            && helpee->get_master() != ch)
        {
            if (stop_follower(helpee, SF_MASTERDIE))
            {
                return;
            }
        }

        AFFECT_DATA<EApplyLocation> af;
        if (!(k && k->follower == helpee)) {
            ch->add_follower(helpee);
            af.duration = pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
        }
        else {
            auto aff = k->follower->affected.begin();
            for (; aff != k->follower->affected.end(); ++aff)
            {
                if ((*aff)->type == SPELL_CHARM)
                {
                    break;
                }
            }

            if (aff != k->follower->affected.end())
            {
                af.duration = (*aff)->duration + pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
            }
        }

        affect_from_char(helpee, SPELL_CHARM);

        if (!WAITLESS(ch))
        {
            if (isname(isbank, "банк bank"))
            {
                ch->remove_bank(cost);
                helpee->mob_specials.hire_price = -hire_price;
            }
            else
            {
                ch->remove_gold(cost);
                helpee->mob_specials.hire_price = hire_price;
            }
        }

        af.type = SPELL_CHARM;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
        af.battleflag = 0;
        affect_to_char(helpee, af);

        af.type = SPELL_CHARM;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = to_underlying(EAffectFlag::AFF_HELPER);
        af.battleflag = 0;
        affect_to_char(helpee, af);

        sprintf(buf, "$n сказал$g вам : \"Приказывай, %s!\"", IS_FEMALE(ch) ? "хозяйка" : "хозяин");
        act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);

        if (IS_NPC(helpee))
        {
            for (auto i = 0; i < NUM_WEARS; i++)
            {
                if (GET_EQ(helpee, i))
                {
                    if (!remove_otrigger(GET_EQ(helpee, i), helpee))
                        continue;

                    act("Вы прекратили использовать $o3.", FALSE, helpee, GET_EQ(helpee, i), 0, TO_CHAR);
                    act("$n прекратил$g использовать $o3.", TRUE, helpee, GET_EQ(helpee, i), 0, TO_ROOM);
                    obj_to_char(unequip_char(helpee, i | 0x40), helpee);
                }
            }

            MOB_FLAGS(helpee).unset(MOB_AGGRESSIVE);
            MOB_FLAGS(helpee).unset(MOB_SPEC);
            PRF_FLAGS(helpee).unset(PRF_PUNCTUAL);
            MOB_FLAGS(helpee).set(MOB_NOTRAIN);
            helpee->set_skill(SKILL_PUNCTUAL, 0);
            ch->updateCharmee(GET_MOB_VNUM(helpee), cost);

            Crash_crashsave(ch);
            ch->save_char();
        }
    }
}

void do_freehelpee(CHAR_DATA* ch, char* /* argument*/, int/* cmd*/, int/* subcmd*/)
{
    if (IS_NPC(ch)
        || (!WAITLESS(ch) && !can_use_feat(ch, EMPLOYER_FEAT)))
    {
        send_to_char("Вам недоступно это!\r\n", ch);
        return;
    }

    follow_type* k;
    for (k = ch->followers; k; k = k->next)
    {
        if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
            && AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM))
        {
            break;
        }
    }

    if (!k)
    {
        act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (ch->in_room != IN_ROOM(k->follower))
    {
        act("Вам следует встретиться с $N4 для этого.", FALSE, ch, 0, k->follower, TO_CHAR);
        return;
    }

    if (GET_POS(k->follower) < POS_STANDING)
    {
        act("$N2 сейчас, похоже, не до вас.", FALSE, ch, 0, k->follower, TO_CHAR);
        return;
    }

    if (!IS_IMMORTAL(ch))
    {
        for (const auto& aff : k->follower->affected)
        {
            if (aff->type == SPELL_CHARM)
            {
                const auto cost = MAX(0, (int)((aff->duration - 1) / 2)*(int)abs(k->follower->mob_specials.hire_price));
                if (cost > 0)
                {
                    if (k->follower->mob_specials.hire_price < 0)
                    {
                        ch->add_bank(cost);
                    }
                    else
                    {
                        ch->add_gold(cost);
                    }
                }

                break;
            }
        }
    }

    act("Вы рассчитали $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
    affect_from_char(k->follower, SPELL_CHARM);
    stop_follower(k->follower, SF_CHARMLOST);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

