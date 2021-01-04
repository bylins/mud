#include "leveling.h"

#include "chars/char_player.hpp"
#include "class.hpp"
#include "exchange.h"
#include "ext_money.hpp"
#include "house_exp.hpp"
#include "mob_stat.hpp"
#include "screen.h"
#include "spam.hpp"
#include "top.h"
#include "utils.h"
#include "zone.table.hpp"

extern double exp_coefficients[];
const char* Remort::CONFIG_FILE = LIB_MISC"remort.xml";

int get_remort_mobmax(CHAR_DATA * ch)
{
    int remort = GET_REMORT(ch);
    if (remort >= 18)
        return 15;
    if (remort >= 14)
        return 7;
    if (remort >= 9)
        return 4;
    return 0;
}


// * Обработка событий при левел-апе.
void levelup_events(CHAR_DATA *ch)
{
    if (SpamSystem::MIN_OFFTOP_LVL == GET_LEVEL(ch)
        && !ch->get_disposable_flag(DIS_OFFTOP_MESSAGE))
    {
        PRF_FLAGS(ch).set(PRF_OFFTOP_MODE);
        ch->set_disposable_flag(DIS_OFFTOP_MESSAGE);
        send_to_char(ch,
                     "%sТеперь вы можете пользоваться каналом оффтоп ('справка оффтоп').%s\r\n",
                     CCIGRN(ch, C_SPR), CCNRM(ch, C_SPR));
    }
    if (EXCHANGE_MIN_CHAR_LEV == GET_LEVEL(ch)
        && !ch->get_disposable_flag(DIS_EXCHANGE_MESSAGE))
    {
        // по умолчанию базар у всех включен, поэтому не спамим даже однократно
        if (GET_REMORT(ch) <= 0)
        {
            send_to_char(ch,
                         "%sТеперь вы можете покупать и продавать вещи на базаре ('справка базар!').%s\r\n",
                         CCIGRN(ch, C_SPR), CCNRM(ch, C_SPR));
        }
        ch->set_disposable_flag(DIS_EXCHANGE_MESSAGE);
    }
}

void decrease_level(CHAR_DATA * ch)
{
    int add_move = 0;

    switch (GET_CLASS(ch))
    {
        case CLASS_BATTLEMAGE:
        case CLASS_DEFENDERMAGE:
        case CLASS_CHARMMAGE:
        case CLASS_NECROMANCER:
            add_move = 2;
            break;

        case CLASS_CLERIC:
        case CLASS_DRUID:
            add_move = 2;
            break;

        case CLASS_THIEF:
        case CLASS_ASSASINE:
        case CLASS_MERCHANT:
            add_move = ch->get_inborn_dex() / 5 + 1;
            break;

        case CLASS_WARRIOR:
        case CLASS_GUARD:
        case CLASS_PALADINE:
        case CLASS_RANGER:
        case CLASS_SMITH:
            add_move = ch->get_inborn_dex() / 5 + 1;
            break;
    }

    check_max_hp(ch);
    ch->points.max_move -= MIN(ch->points.max_move, MAX(1, add_move));

    GET_WIMP_LEV(ch) = MAX(0, MIN(GET_WIMP_LEV(ch), GET_REAL_MAX_HIT(ch) / 2));
    if (!IS_IMMORTAL(ch))
    {
        PRF_FLAGS(ch).unset(PRF_HOLYLIGHT);
    }

    TopPlayer::Refresh(ch);
    ch->save_char();
}


namespace ExpCalc {
    void advance_level(CHAR_DATA * ch)
    {
        int add_move = 0, i;

        switch (GET_CLASS(ch))
        {
            case CLASS_BATTLEMAGE:
            case CLASS_DEFENDERMAGE:
            case CLASS_CHARMMAGE:
            case CLASS_NECROMANCER:
                add_move = 2;
                break;

            case CLASS_CLERIC:
            case CLASS_DRUID:
                add_move = 2;
                break;

            case CLASS_THIEF:
            case CLASS_ASSASINE:
            case CLASS_MERCHANT:
                add_move = number(ch->get_inborn_dex() / 6 + 1, ch->get_inborn_dex() / 5 + 1);
                break;

            case CLASS_WARRIOR:
                add_move = number(ch->get_inborn_dex() / 6 + 1, ch->get_inborn_dex() / 5 + 1);
                break;

            case CLASS_GUARD:
            case CLASS_RANGER:
            case CLASS_PALADINE:
            case CLASS_SMITH:
                add_move = number(ch->get_inborn_dex() / 6 + 1, ch->get_inborn_dex() / 5 + 1);
                break;
        }

        check_max_hp(ch);
        ch->points.max_move += MAX(1, add_move);

        setAllInbornFeatures(ch);

        if (IS_IMMORTAL(ch))
        {
            for (i = 0; i < 3; i++)
                GET_COND(ch, i) = (char) - 1;
            PRF_FLAGS(ch).set(PRF_HOLYLIGHT);
        }

        TopPlayer::Refresh(ch);
        levelup_events(ch);
        ch->save_char();
    }

    void gain_exp(CHAR_DATA * ch, int gain)
    {
        int is_altered = FALSE;
        int num_levels = 0;
        char buf[128];

        if (IS_NPC(ch))
        {
            ch->set_exp(ch->get_exp() + gain);
            return;
        }
        else
        {
            ch->dps_add_exp(gain);
            ZoneExpStat::add(zone_table[world[ch->in_room]->zone].number, gain);
        }

        if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
            return;

        if (gain > 0 && GET_LEVEL(ch) < LVL_IMMORT)
        {
            gain = MIN(gain, max_exp_gain_pc(ch));	// put a cap on the max gain per kill
            ch->set_exp(ch->get_exp() + gain);
            if (GET_EXP(ch) >= level_exp(ch, LVL_IMMORT))
            {
                if (!GET_GOD_FLAG(ch, GF_REMORT) && GET_REMORT(ch) < MAX_REMORT)
                {
                    if (Remort::can_remort_now(ch))
                    {
                        send_to_char(ch, "%sПоздравляем, вы получили право на перевоплощение!%s\r\n",
                                     CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
                    }
                    else
                    {
                        send_to_char(ch,
                                     "%sПоздравляем, вы набрали максимальное количество опыта!\r\n"
                                     "%s%s\r\n", CCIGRN(ch, C_NRM), Remort::WHERE_TO_REMORT_STR.c_str(), CCNRM(ch, C_NRM));
                    }
                    SET_GOD_FLAG(ch, GF_REMORT);
                }
            }
            ch->set_exp(MIN(GET_EXP(ch), level_exp(ch, LVL_IMMORT) - 1));
            while (GET_LEVEL(ch) < LVL_IMMORT && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
            {
                ch->set_level(ch->get_level() + 1);
                num_levels++;
                sprintf(buf, "%sВы достигли следующего уровня!%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
                send_to_char(buf, ch);
                advance_level(ch);
                is_altered = TRUE;
            }

            if (is_altered)
            {
                sprintf(buf, "%s advanced %d level%s to level %d.",
                        GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
                mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
            }
        }
        else if (gain < 0 && GET_LEVEL(ch) < LVL_IMMORT)
        {
            gain = MAX(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
            ch->set_exp(ch->get_exp() + gain);
            while (GET_LEVEL(ch) > 1 && GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch)))
            {
                ch->set_level(ch->get_level() - 1);
                num_levels++;
                sprintf(buf,
                        "%sВы потеряли уровень. Вам должно быть стыдно!%s\r\n",
                        CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
                send_to_char(buf, ch);
                decrease_level(ch);
                is_altered = TRUE;
            }
            if (is_altered)
            {
                sprintf(buf, "%s decreases %d level%s to level %d.",
                        GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
                mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
            }
        }
        if ((GET_EXP(ch) < level_exp(ch, LVL_IMMORT) - 1)
            && GET_GOD_FLAG(ch, GF_REMORT)
            && gain
            && (GET_LEVEL(ch) < LVL_IMMORT))
        {
            if (Remort::can_remort_now(ch))
            {
                send_to_char(ch, "%sВы потеряли право на перевоплощение!%s\r\n",
                             CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
            }
            CLR_GOD_FLAG(ch, GF_REMORT);
        }

        char_stat::add_class_exp(GET_CLASS(ch), gain);
        update_clan_exp(ch, gain);
    }

// юзается исключительно в act.wizards.cpp в имм командах "advance" и "set exp".
    void gain_exp_regardless(CHAR_DATA * ch, int gain)
    {
        int is_altered = FALSE;
        int num_levels = 0;

        ch->set_exp(ch->get_exp() + gain);
        if (!IS_NPC(ch))
        {
            if (gain > 0)
            {
                while (GET_LEVEL(ch) < LVL_IMPL && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
                {
                    ch->set_level(ch->get_level() + 1);
                    num_levels++;
                    sprintf(buf, "%sВы достигли следующего уровня!%s\r\n",
                            CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
                    send_to_char(buf, ch);

                    advance_level(ch);
                    is_altered = TRUE;
                }

                if (is_altered)
                {
                    sprintf(buf, "%s advanced %d level%s to level %d.",
                            GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
                    mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
                }
            }
            else if (gain < 0)
            {
                // Pereplut: глупый участок кода.
                //			gain = MAX(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
                //			GET_EXP(ch) += gain;
                //			if (GET_EXP(ch) < 0)
                //				GET_EXP(ch) = 0;
                while (GET_LEVEL(ch) > 1 && GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch)))
                {
                    ch->set_level(ch->get_level() - 1);
                    num_levels++;
                    sprintf(buf,
                            "%sВы потеряли уровень!%s\r\n",
                            CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
                    send_to_char(buf, ch);
                    decrease_level(ch);
                    is_altered = TRUE;
                }
                if (is_altered)
                {
                    sprintf(buf, "%s decreases %d level%s to level %d.",
                            GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
                    mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
                }
            }

        }
    }

    int get_extend_exp(int exp, CHAR_DATA * ch, CHAR_DATA * victim)
    {
        int base, diff;
        int koef;

        if (!IS_NPC(victim) || IS_NPC(ch))
            return (exp);
        // если моб убивается первый раз, то повышаем экспу в несколько раз
        // стимулируем изучение новых зон!
        ch->send_to_TC(false, true, false,
                       "&RУ моба еще %d убийств без замакса, экспа %d, убито %d&n\r\n",
                       mob_proto[victim->get_rnum()].mob_specials.MaxFactor,
                       exp,
                       ch->mobmax_get(GET_MOB_VNUM(victim)));
// все равно таблица корявая, учитываются только уникальные мобы и глючит
/*
	// даем увеличенную экспу за давно не убитых мобов.
	// за совсем неубитых мобов не даем, что бы новые зоны не давали x10 экспу.
	exp *= get_npc_long_live_exp_bounus(GET_MOB_VNUM(victim));
*/
/*  бонусы за непопулярных мобов круче
	if (ch->mobmax_get(GET_MOB_VNUM(victim)) == 0)	{
		// так чуть-чуть поприятней
		exp *= 1.5;
		exp /= std::max(1.0, 0.5 * (GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED));
		return (exp);
	}
*/
        exp += exp * (ch->add_abils.percent_exp_add) / 100.0;
        for (koef = 100, base = 0, diff = ch->mobmax_get(GET_MOB_VNUM(victim)) - mob_proto[victim->get_rnum()].mob_specials.MaxFactor;
             base < diff && koef > 5; base++, koef = koef * (95 - get_remort_mobmax(ch)) / 100);
        // минимальный опыт при замаксе 15% от полного опыта
        exp = exp * MAX(15, koef) / 100;

        // делим на реморты
        exp /= std::max(1.0, 0.5 * (GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED));
        return (exp);
    }

    int max_exp_gain_pc(CHAR_DATA * ch)
    {
        int result = 1;
        if (!IS_NPC(ch))
        {
            int max_per_lev =
                    level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch) + 0);
            result = max_per_lev / (10 + GET_REMORT(ch));
        }
        return result;
    }

    int max_exp_loss_pc(CHAR_DATA * ch)
    {
        return (IS_NPC(ch) ? 1 : (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch) + 0)) / 3);
    }

    float get_npc_long_live_exp_bounus(int vnum)
    {
        if (vnum == -1)	{
            return  1.0f;
        }
        int nowTime = time(0);
        int lastKillTime = mob_stat::last_time_killed_mob(vnum);
        if ( lastKillTime > 0) {
            int deltaTime = nowTime - lastKillTime;
            int delay = 60 * 60 * 24 * 30 ; // 30 days
            float timeMultiplicator = floor(deltaTime/delay);
            if (timeMultiplicator<1.0f) {
                timeMultiplicator = 1.0f;
            }
            if (timeMultiplicator>10.0f) {
                timeMultiplicator = 10.0f;
            }
            return timeMultiplicator;
        }
        return  1.0f;
    }

// Function to return the exp required for each class/level
    int level_exp(CHAR_DATA * ch, int level)
    {
        if (level > LVL_IMPL || level < 0)
        {
            log("SYSERR: Requesting exp for invalid level %d!", level);
            return 0;
        }

        /*
         * Gods have exp close to EXP_MAX.  This statement should never have to
         * changed, regardless of how many mortal or immortal levels exist.
         */
        if (level > LVL_IMMORT)
        {
            return EXP_IMPL - ((LVL_IMPL - level) * 1000);
        }

        // Exp required for normal mortals is below
        float exp_modifier;
        if (GET_REMORT(ch) < MAX_EXP_COEFFICIENTS_USED)
            exp_modifier = exp_coefficients[GET_REMORT(ch)];
        else
            exp_modifier = exp_coefficients[MAX_EXP_COEFFICIENTS_USED];

        switch (GET_CLASS(ch))
        {

            case CLASS_BATTLEMAGE:
            case CLASS_DEFENDERMAGE:
            case CLASS_CHARMMAGE:
            case CLASS_NECROMANCER:
                switch (level)
                {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return int (exp_modifier * 2500);
                    case 3:
                        return int (exp_modifier * 5000);
                    case 4:
                        return int (exp_modifier * 9000);
                    case 5:
                        return int (exp_modifier * 17000);
                    case 6:
                        return int (exp_modifier * 27000);
                    case 7:
                        return int (exp_modifier * 47000);
                    case 8:
                        return int (exp_modifier * 77000);
                    case 9:
                        return int (exp_modifier * 127000);
                    case 10:
                        return int (exp_modifier * 197000);
                    case 11:
                        return int (exp_modifier * 297000);
                    case 12:
                        return int (exp_modifier * 427000);
                    case 13:
                        return int (exp_modifier * 587000);
                    case 14:
                        return int (exp_modifier * 817000);
                    case 15:
                        return int (exp_modifier * 1107000);
                    case 16:
                        return int (exp_modifier * 1447000);
                    case 17:
                        return int (exp_modifier * 1847000);
                    case 18:
                        return int (exp_modifier * 2310000);
                    case 19:
                        return int (exp_modifier * 2830000);
                    case 20:
                        return int (exp_modifier * 3580000);
                    case 21:
                        return int (exp_modifier * 4580000);
                    case 22:
                        return int (exp_modifier * 5830000);
                    case 23:
                        return int (exp_modifier * 7330000);
                    case 24:
                        return int (exp_modifier * 9080000);
                    case 25:
                        return int (exp_modifier * 11080000);
                    case 26:
                        return int (exp_modifier * 15000000);
                    case 27:
                        return int (exp_modifier * 22000000);
                    case 28:
                        return int (exp_modifier * 33000000);
                    case 29:
                        return int (exp_modifier * 47000000);
                    case 30:
                        return int (exp_modifier * 64000000);
                        // add new levels here
                    case LVL_IMMORT:
                        return int (exp_modifier * 94000000);
                }
                break;

            case CLASS_CLERIC:
            case CLASS_DRUID:
                switch (level)
                {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return int (exp_modifier * 2500);
                    case 3:
                        return int (exp_modifier * 5000);
                    case 4:
                        return int (exp_modifier * 9000);
                    case 5:
                        return int (exp_modifier * 17000);
                    case 6:
                        return int (exp_modifier * 27000);
                    case 7:
                        return int (exp_modifier * 47000);
                    case 8:
                        return int (exp_modifier * 77000);
                    case 9:
                        return int (exp_modifier * 127000);
                    case 10:
                        return int (exp_modifier * 197000);
                    case 11:
                        return int (exp_modifier * 297000);
                    case 12:
                        return int (exp_modifier * 427000);
                    case 13:
                        return int (exp_modifier * 587000);
                    case 14:
                        return int (exp_modifier * 817000);
                    case 15:
                        return int (exp_modifier * 1107000);
                    case 16:
                        return int (exp_modifier * 1447000);
                    case 17:
                        return int (exp_modifier * 1847000);
                    case 18:
                        return int (exp_modifier * 2310000);
                    case 19:
                        return int (exp_modifier * 2830000);
                    case 20:
                        return int (exp_modifier * 3580000);
                    case 21:
                        return int (exp_modifier * 4580000);
                    case 22:
                        return int (exp_modifier * 5830000);
                    case 23:
                        return int (exp_modifier * 7330000);
                    case 24:
                        return int (exp_modifier * 9080000);
                    case 25:
                        return int (exp_modifier * 11080000);
                    case 26:
                        return int (exp_modifier * 15000000);
                    case 27:
                        return int (exp_modifier * 22000000);
                    case 28:
                        return int (exp_modifier * 33000000);
                    case 29:
                        return int (exp_modifier * 47000000);
                    case 30:
                        return int (exp_modifier * 64000000);
                        // add new levels here
                    case LVL_IMMORT:
                        return int (exp_modifier * 87000000);
                }
                break;

            case CLASS_THIEF:
                switch (level)
                {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return int (exp_modifier * 1000);
                    case 3:
                        return int (exp_modifier * 2000);
                    case 4:
                        return int (exp_modifier * 4000);
                    case 5:
                        return int (exp_modifier * 8000);
                    case 6:
                        return int (exp_modifier * 15000);
                    case 7:
                        return int (exp_modifier * 28000);
                    case 8:
                        return int (exp_modifier * 52000);
                    case 9:
                        return int (exp_modifier * 85000);
                    case 10:
                        return int (exp_modifier * 131000);
                    case 11:
                        return int (exp_modifier * 192000);
                    case 12:
                        return int (exp_modifier * 271000);
                    case 13:
                        return int (exp_modifier * 372000);
                    case 14:
                        return int (exp_modifier * 512000);
                    case 15:
                        return int (exp_modifier * 672000);
                    case 16:
                        return int (exp_modifier * 854000);
                    case 17:
                        return int (exp_modifier * 1047000);
                    case 18:
                        return int (exp_modifier * 1274000);
                    case 19:
                        return int (exp_modifier * 1534000);
                    case 20:
                        return int (exp_modifier * 1860000);
                    case 21:
                        return int (exp_modifier * 2520000);
                    case 22:
                        return int (exp_modifier * 3380000);
                    case 23:
                        return int (exp_modifier * 4374000);
                    case 24:
                        return int (exp_modifier * 5500000);
                    case 25:
                        return int (exp_modifier * 6827000);
                    case 26:
                        return int (exp_modifier * 8667000);
                    case 27:
                        return int (exp_modifier * 13334000);
                    case 28:
                        return int (exp_modifier * 20000000);
                    case 29:
                        return int (exp_modifier * 28667000);
                    case 30:
                        return int (exp_modifier * 40000000);
                        // add new levels here
                    case LVL_IMMORT:
                        return int (exp_modifier * 53000000);
                }
                break;

            case CLASS_ASSASINE:
            case CLASS_MERCHANT:
                switch (level)
                {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return int (exp_modifier * 1500);
                    case 3:
                        return int (exp_modifier * 3000);
                    case 4:
                        return int (exp_modifier * 6000);
                    case 5:
                        return int (exp_modifier * 12000);
                    case 6:
                        return int (exp_modifier * 22000);
                    case 7:
                        return int (exp_modifier * 42000);
                    case 8:
                        return int (exp_modifier * 77000);
                    case 9:
                        return int (exp_modifier * 127000);
                    case 10:
                        return int (exp_modifier * 197000);
                    case 11:
                        return int (exp_modifier * 287000);
                    case 12:
                        return int (exp_modifier * 407000);
                    case 13:
                        return int (exp_modifier * 557000);
                    case 14:
                        return int (exp_modifier * 767000);
                    case 15:
                        return int (exp_modifier * 1007000);
                    case 16:
                        return int (exp_modifier * 1280000);
                    case 17:
                        return int (exp_modifier * 1570000);
                    case 18:
                        return int (exp_modifier * 1910000);
                    case 19:
                        return int (exp_modifier * 2300000);
                    case 20:
                        return int (exp_modifier * 2790000);
                    case 21:
                        return int (exp_modifier * 3780000);
                    case 22:
                        return int (exp_modifier * 5070000);
                    case 23:
                        return int (exp_modifier * 6560000);
                    case 24:
                        return int (exp_modifier * 8250000);
                    case 25:
                        return int (exp_modifier * 10240000);
                    case 26:
                        return int (exp_modifier * 13000000);
                    case 27:
                        return int (exp_modifier * 20000000);
                    case 28:
                        return int (exp_modifier * 30000000);
                    case 29:
                        return int (exp_modifier * 43000000);
                    case 30:
                        return int (exp_modifier * 59000000);
                        // add new levels here
                    case LVL_IMMORT:
                        return int (exp_modifier * 79000000);
                }
                break;

            case CLASS_WARRIOR:
            case CLASS_GUARD:
            case CLASS_PALADINE:
            case CLASS_RANGER:
            case CLASS_SMITH:
                switch (level)
                {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return int (exp_modifier * 2000);
                    case 3:
                        return int (exp_modifier * 4000);
                    case 4:
                        return int (exp_modifier * 8000);
                    case 5:
                        return int (exp_modifier * 14000);
                    case 6:
                        return int (exp_modifier * 24000);
                    case 7:
                        return int (exp_modifier * 39000);
                    case 8:
                        return int (exp_modifier * 69000);
                    case 9:
                        return int (exp_modifier * 119000);
                    case 10:
                        return int (exp_modifier * 189000);
                    case 11:
                        return int (exp_modifier * 289000);
                    case 12:
                        return int (exp_modifier * 419000);
                    case 13:
                        return int (exp_modifier * 579000);
                    case 14:
                        return int (exp_modifier * 800000);
                    case 15:
                        return int (exp_modifier * 1070000);
                    case 16:
                        return int (exp_modifier * 1340000);
                    case 17:
                        return int (exp_modifier * 1660000);
                    case 18:
                        return int (exp_modifier * 2030000);
                    case 19:
                        return int (exp_modifier * 2450000);
                    case 20:
                        return int (exp_modifier * 2950000);
                    case 21:
                        return int (exp_modifier * 3950000);
                    case 22:
                        return int (exp_modifier * 5250000);
                    case 23:
                        return int (exp_modifier * 6750000);
                    case 24:
                        return int (exp_modifier * 8450000);
                    case 25:
                        return int (exp_modifier * 10350000);
                    case 26:
                        return int (exp_modifier * 14000000);
                    case 27:
                        return int (exp_modifier * 21000000);
                    case 28:
                        return int (exp_modifier * 31000000);
                    case 29:
                        return int (exp_modifier * 44000000);
                    case 30:
                        return int (exp_modifier * 64000000);
                        // add new levels here
                    case LVL_IMMORT:
                        return int (exp_modifier * 79000000);
                }
                break;
        }

        /*
         * This statement should never be reached if the exp tables in this function
         * are set up properly.  If you see exp of 123456 then the tables above are
         * incomplete -- so, complete them!
         */
        log("SYSERR: XP tables not set up correctly in class.c!");
        return 123456;
    }

    int calcDeathExp(CHAR_DATA* ch){
        if (!RENTABLE(ch))
            return (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch))) / (3 + MIN(3, GET_REMORT(ch) / 5)) / ch->death_player_count();
        else
            return (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch))) / (3 + MIN(3, GET_REMORT(ch) / 5));
    }
}

namespace Remort
{

// релоадится через 'reload remort.xml'

// проверка, мешает ли что-то чару уйти в реморт
    bool can_remort_now(CHAR_DATA *ch)
    {
        if (PRF_FLAGGED(ch, PRF_CAN_REMORT) || !need_torc(ch))
        {
            return true;
        }
        return false;
    }

// проверка, требуется ли от чара жертвовать для реморта
    bool need_torc(CHAR_DATA *ch)
    {
        TorcReq torc_req(GET_REMORT(ch));

        if (torc_req.type < ExtMoney::TOTAL_TYPES && torc_req.amount > 0) {
            return true;
        }
        return false;
    }

} // namespace Remort
