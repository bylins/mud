#include "stat.h"

#include "ban.h"
#include "chars/char_player.hpp"
#include "chars/player_races.hpp"
#include "char_obj_utils.inl"
#include "description.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/pk.h"
#include "olc/olc.h"
#include "glory.hpp"
#include "glory_const.hpp"
#include "handler.h"
#include "house.h"
#include "liquid.hpp"
#include "object.prototypes.hpp"
#include "screen.h"
#include "mob_stat.hpp"
#include "modify.h"
#include "utils.h"
#include "zone.table.hpp"
#include "skills_info.h"
#include "spells.info.h"

void do_statip(CHAR_DATA * ch, CHAR_DATA * k)
{
    log("Start logon list stat");

    // Отображаем список ip-адресов с которых персонаж входил
    if (!LOGON_LIST(k).empty())
    {
        // update: логон-лист может быть капитально большим, поэтому пишем это в свой дин.буфер, а не в buf2
        // заодно будет постраничный вывод ип, чтобы имма не посылало на йух с **OVERFLOW**
        std::ostringstream out("Персонаж заходил с IP-адресов:\r\n");
        for(const auto& logon : LOGON_LIST(k))
        {
            sprintf(buf1, "%16s %5ld %20s%s\r\n", logon.ip, logon.count, rustime(localtime(&logon.lasttime)), logon.is_first ? " (создание)" : "");

            out << buf1;
        }
        page_string(ch->desc, out.str());
    }

    log("End logon list stat");
}

void do_stat_character(CHAR_DATA * ch, CHAR_DATA * k, const int virt = 0)
{
	int i, i2, found = 0;
	OBJ_DATA *j;
	struct follow_type *fol;
	char tmpbuf[128];
	buf[0] = 0;
	int god_level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);
	int k_room = -1;
	if (!virt && (god_level == LVL_IMPL || (god_level == LVL_GRGOD && !IS_NPC(k)))) {
		k_room = GET_ROOM_VNUM(IN_ROOM(k));
	}
	// пишем пол  (мужчина)
	sprinttype(to_underlying(GET_SEX(k)), genders, tmpbuf);
	// пишем расу (Человек)
	if (IS_NPC(k)) {
		sprinttype(GET_RACE(k) - NPC_RACE_BASIC, npc_race_types, smallBuf);
		sprintf(buf, "%s %s ", tmpbuf, smallBuf);
	}
	sprintf(buf2, "%s '%s' IDNum: [%ld] В комнате [%d] Текущий ID:[%ld]", (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")), GET_NAME(k), GET_IDNUM(k), k_room, GET_ID(k));
	send_to_char(strcat(buf, buf2), ch);
	send_to_char(ch, " ЛАГ: [%d]\r\n", k->get_wait());
	if (IS_MOB(k)) {
		sprintf(buf, "Синонимы: &S%s&s, VNum: [%5d], RNum: [%5d]\r\n", k->get_pc_name().c_str(), GET_MOB_VNUM(k), GET_MOB_RNUM(k));
		send_to_char(buf, ch);
	}

    sprintf(buf, "Падежи: %s/%s/%s/%s/%s/%s ", GET_PAD(k, 0), GET_PAD(k, 1), GET_PAD(k, 2), GET_PAD(k, 3), GET_PAD(k, 4), GET_PAD(k, 5));
    send_to_char(buf, ch);


    if (!IS_NPC(k))
    {

        if (!NAME_GOD(k))
        {
            sprintf(buf, "Имя никем не одобрено!\r\n");
            send_to_char(buf, ch);
        }
        else if (NAME_GOD(k) < 1000)
        {
            sprintf(buf, "Имя запрещено! - %s\r\n", get_name_by_id(NAME_ID_GOD(k)));
            send_to_char(buf, ch);
        }
        else
        {
            sprintf(buf, "Имя одобрено! - %s\r\n", get_name_by_id(NAME_ID_GOD(k)));
            send_to_char(buf, ch);
        }

        sprintf(buf, "Вероисповедание: %s\r\n", religion_name[(int)GET_RELIGION(k)][(int)GET_SEX(k)]);
        send_to_char(buf, ch);

        std::string file_name = GET_NAME(k);
        CreateFileName(file_name);
        sprintf(buf, "E-mail: &S%s&s Unique: %d File: %s\r\n", GET_EMAIL(k), GET_UNIQUE(k), file_name.c_str());
        send_to_char(buf, ch);

        std::string text = RegisterSystem::show_comment(GET_EMAIL(k));
        if (!text.empty())
            send_to_char(ch, "Registered by email from %s\r\n", text.c_str());

        if (k->player_specials->saved.telegram_id != 0)
            send_to_char(ch, "Подключен Телеграм, chat_id: %lu\r\n", k->player_specials->saved.telegram_id);


        if (PLR_FLAGGED(k, PLR_FROZEN) && FREEZE_DURATION(k))
        {
            sprintf(buf, "Заморожен : %ld час [%s].\r\n",
                    static_cast<long>((FREEZE_DURATION(k) - time(NULL)) / 3600),
                    FREEZE_REASON(k) ? FREEZE_REASON(k) : "-");
            send_to_char(buf, ch);
        }
        if (PLR_FLAGGED(k, PLR_HELLED) && HELL_DURATION(k))
        {
            sprintf(buf, "Находится в темнице : %ld час [%s].\r\n",
                    static_cast<long>((HELL_DURATION(k) - time(NULL)) / 3600),
                    HELL_REASON(k) ? HELL_REASON(k) : "-");
            send_to_char(buf, ch);
        }
        if (PLR_FLAGGED(k, PLR_NAMED) && NAME_DURATION(k))
        {
            sprintf(buf, "Находится в комнате имени : %ld час.\r\n",
                    static_cast<long>((NAME_DURATION(k) - time(NULL)) / 3600));
            send_to_char(buf, ch);
        }
        if (PLR_FLAGGED(k, PLR_MUTE) && MUTE_DURATION(k))
        {
            sprintf(buf, "Будет молчать : %ld час [%s].\r\n",
                    static_cast<long>((MUTE_DURATION(k) - time(NULL)) / 3600),
                    MUTE_REASON(k) ? MUTE_REASON(k) : "-");
            send_to_char(buf, ch);
        }
        if (PLR_FLAGGED(k, PLR_DUMB) && DUMB_DURATION(k))
        {
            sprintf(buf, "Будет нем : %ld мин [%s].\r\n",
                    static_cast<long>((DUMB_DURATION(k) - time(NULL)) / 60),
                    DUMB_REASON(k) ? DUMB_REASON(k) : "-");
            send_to_char(buf, ch);
        }
        if (!PLR_FLAGGED(k, PLR_REGISTERED) && UNREG_DURATION(k))
        {
            sprintf(buf, "Не будет зарегистрирован : %ld час [%s].\r\n",
                    static_cast<long>((UNREG_DURATION(k) - time(NULL)) / 3600),
                    UNREG_REASON(k) ? UNREG_REASON(k) : "-");
            send_to_char(buf, ch);
        }

        if (GET_GOD_FLAG(k, GF_GODSLIKE) && GCURSE_DURATION(k))
        {
            sprintf(buf, "Под защитой Богов : %ld час.\r\n",
                    static_cast<long>((GCURSE_DURATION(k) - time(NULL)) / 3600));
            send_to_char(buf, ch);
        }
        if (GET_GOD_FLAG(k, GF_GODSCURSE) && GCURSE_DURATION(k))
        {
            sprintf(buf, "Проклят Богами : %ld час.\r\n",
                    static_cast<long>((GCURSE_DURATION(k) - time(NULL)) / 3600));
            send_to_char(buf, ch);
        }
    }

    sprintf(buf, "Титул: %s\r\n", (k->player_data.title != "" ? k->player_data.title.c_str() : "<Нет>"));
    send_to_char(buf, ch);
    if (IS_NPC(k))
        sprintf(buf, "L-Des: %s", (k->player_data.long_descr != "" ? k->player_data.long_descr.c_str() : "<Нет>\r\n"));
    else
        sprintf(buf, "L-Des: %s", (k->player_data.description != "" ? k->player_data.description.c_str() : "<Нет>\r\n"));
    send_to_char(buf, ch);

    if (!IS_NPC(k))
    {
        sprinttype(k->get_class(), pc_class_types, smallBuf);
        sprintf(buf, "Племя: %s, Род: %s, Профессия: %s",
                PlayerRace::GetKinNameByNum(GET_KIN(k), GET_SEX(k)).c_str(),
                k->get_race_name().c_str(),
                smallBuf);
        send_to_char(buf, ch);
    }
    else
    {
        std::string str;
        if (k->get_role_bits().any())
        {
            print_bitset(k->get_role_bits(), npc_role_types, ",", str);
        }
        else
        {
            str += "нет";
        }
        send_to_char(ch, "Роли NPC: %s%s%s", CCCYN(ch, C_NRM), str.c_str(), CCNRM(ch, C_NRM));
    }

    char tmp_buf[256];
    if (k->get_zone_group() > 1)
    {
        snprintf(tmp_buf, sizeof(tmp_buf), " : групповой %ldx%d",
                 GET_EXP(k) / k->get_zone_group(), k->get_zone_group());
    }
    else
    {
        tmp_buf[0] = '\0';
    }

    sprintf(buf, ", Уровень: [%s%2d%s], Опыт: [%s%10ld%s]%s, Наклонности: [%4d]\r\n",
            CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_EXP(k), CCNRM(ch, C_NRM), tmp_buf, GET_ALIGNMENT(k));

    send_to_char(buf, ch);

    if (!IS_NPC(k))
    {
        if (CLAN(k))
        {
            send_to_char(ch, "Статус дружины: %s\r\n", GET_CLAN_STATUS(k));
        }

        //added by WorM когда статишь файл собсно показывалось текущее время а не время последнего входа
        time_t ltime = get_lastlogon_by_unique(GET_UNIQUE(k));
        char t1[11];
        char t2[11];
        strftime(t1, sizeof(t1), "%d-%m-%Y", localtime(&(k->player_data.time.birth)));
        strftime(t2, sizeof(t2), "%d-%m-%Y", localtime(&ltime));
        t1[10] = t2[10] = '\0';

        sprintf(buf,
                "Создан: [%s] Последний вход: [%s] Играл: [%dh %dm] Возраст: [%d]\r\n",
                t1, t2, k->player_data.time.played / 3600, ((k->player_data.time.played % 3600) / 60), age(k)->year);
        send_to_char(buf, ch);

        k->add_today_torc(0);
        sprintf(buf, "Рента: [%d], Денег: [%9ld], В банке: [%9ld] (Всего: %ld), Гривны: %d/%d/%d %d",
                GET_LOADROOM(k), k->get_gold(), k->get_bank(), k->get_total_gold(),
                k->get_ext_money(ExtMoney::TORC_GOLD),
                k->get_ext_money(ExtMoney::TORC_SILVER),
                k->get_ext_money(ExtMoney::TORC_BRONZE),
                k->get_hryvn());

        //. Display OLC zone for immorts .
        if (GET_LEVEL(ch) >= LVL_IMMORT)
        {
            sprintf(buf1, ", %sOLC[%d]%s", CCGRN(ch, C_NRM), GET_OLC_ZONE(k), CCNRM(ch, C_NRM));
            strcat(buf, buf1);
        }
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
    }
    else
    {
        sprintf(buf, "Сейчас в мире : %d. ", GET_MOB_RNUM(k) >= 0 ? mob_index[GET_MOB_RNUM(k)].number - (virt ? 1 : 0) : -1);
        send_to_char(buf, ch);
        std::string stats;
        mob_stat::last_kill_mob(k, stats);
        sprintf(buf, "Последний раз убит: %s", stats.c_str());
        send_to_char(buf, ch);
    }
    sprintf(buf,
            "Сила: [%s%d/%d%s]  Инт : [%s%d/%d%s]  Мудр : [%s%d/%d%s] \r\n"
            "Ловк: [%s%d/%d%s]  Тело:[%s%d/%d%s]  Обаян:[%s%d/%d%s] Размер: [%s%d/%d%s]\r\n",
            CCCYN(ch, C_NRM), k->get_inborn_str(), GET_REAL_STR(k), CCNRM(ch,
                                                                          C_NRM),
            CCCYN(ch, C_NRM), k->get_inborn_int(), GET_REAL_INT(k), CCNRM(ch,
                                                                          C_NRM),
            CCCYN(ch, C_NRM), k->get_inborn_wis(), GET_REAL_WIS(k), CCNRM(ch,
                                                                          C_NRM),
            CCCYN(ch, C_NRM), k->get_inborn_dex(), GET_REAL_DEX(k), CCNRM(ch,
                                                                          C_NRM),
            CCCYN(ch, C_NRM), k->get_inborn_con(), GET_REAL_CON(k), CCNRM(ch,
                                                                          C_NRM),
            CCCYN(ch, C_NRM), k->get_inborn_cha(), GET_REAL_CHA(k), CCNRM(ch,
                                                                          C_NRM),
            CCCYN(ch, C_NRM), GET_SIZE(k), GET_REAL_SIZE(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf, "Жизни :[%s%d/%d+%d%s]  Энергии :[%s%d/%d+%d%s]",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_REAL_MAX_HIT(k), hit_gain(k),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k), GET_REAL_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    if (IS_MANA_CASTER(k))
    {
        sprintf(buf, " Мана :[%s%d/%d+%d%s]\r\n",
                CCGRN(ch, C_NRM), GET_MANA_STORED(k), GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM));
    }
    else
    {
        sprintf(buf, "\r\n");
    }
    send_to_char(buf, ch);

    sprintf(buf,
            "Glory: [%d], ConstGlory: [%d], AC: [%d/%d(%d)], Броня: [%d], Попадания: [%2d/%2d/%d], Повреждения: [%2d/%2d/%d]\r\n",
            Glory::get_glory(GET_UNIQUE(k)), GloryConst::get_glory(GET_UNIQUE(k)), GET_AC(k), GET_REAL_AC(k),
            compute_armor_class(k), GET_ARMOUR(k), GET_HR(k),
            GET_REAL_HR(k), GET_REAL_HR(k) + str_bonus(GET_REAL_STR(k), STR_TO_HIT),
            GET_DR(k), GET_REAL_DR(k), GET_REAL_DR(k) + str_bonus(GET_REAL_STR(k), STR_TO_DAM));
    send_to_char(buf, ch);
    sprintf(buf,
            "Защитн.аффекты: [Para:%d/Breath:%d/Spell:%d/Basic:%d], Поглощ: [%d], Стойк: [%d], Реакц: [%d], Воля: [%d]\r\n",
            GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3),
            GET_ABSORBE(k), GET_REAL_SAVING_STABILITY(k), GET_REAL_SAVING_REFLEX(k), GET_REAL_SAVING_WILL(k));
    send_to_char(buf, ch);
    sprintf(buf,
            "Резисты: [Огонь:%d/Воздух:%d/Вода:%d/Земля:%d/Жизнь:%d/Разум:%d/Иммунитет:%d/Тьма:%d]\r\n",
            GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3),
            GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6), GET_RESIST(k, 7));
    send_to_char(buf, ch);
    sprintf(buf,
            "Защита от маг. аффектов : [%d], Защита от маг. урона : [%d], Защита от физ. урона : [%d]\r\n", GET_AR(k), GET_MR(k), GET_PR(k));
    send_to_char(buf, ch);

    sprintf(buf, "Запом: [%d], УспехКолд: [%d], ВоссЖиз: [%d], ВоссСил: [%d], Поглощ: [%d], Удача: [%d], Иниц: [%d]\r\n",
            GET_MANAREG(k), GET_CAST_SUCCESS(k), GET_HITREG(k), GET_MOVEREG(k), GET_ABSORBE(k), k->calc_morale(), GET_INITIATIVE(k));
    send_to_char(buf, ch);

    sprinttype(GET_POS(k), position_types, smallBuf);
    sprintf(buf, "Положение: %s, Сражается: %s, Экипирован в металл: %s",
            smallBuf, (k->get_fighting() ? GET_NAME(k->get_fighting()) : "Нет"), (equip_in_metall(k) ? "Да" : "Нет"));

    if (IS_NPC(k))
    {
        strcat(buf, ", Тип атаки: ");
        strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
    }
    if (k->desc)
    {
        sprinttype(STATE(k->desc), connected_types, buf2);
        strcat(buf, ", Соединение: ");
        strcat(buf, buf2);
    }
    send_to_char(strcat(buf, "\r\n"), ch);

    strcpy(buf, "Позиция по умолчанию: ");
    sprinttype((k->mob_specials.default_pos), position_types, buf2);
    strcat(buf, buf2);

    sprintf(buf2, ", Таймер отсоединения (тиков) [%d]\r\n", k->char_specials.timer);
    strcat(buf, buf2);
    send_to_char(buf, ch);

    if (IS_NPC(k))
    {
        k->char_specials.saved.act.sprintbits(action_bits, smallBuf, ",", 4);
        sprintf(buf, "MOB флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
        send_to_char(buf, ch);
        k->mob_specials.npc_flags.sprintbits(function_bits, smallBuf, ",", 4);
        sprintf(buf, "NPC флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
        send_to_char(buf, ch);
        send_to_char(ch, "Количество атак: %s%d%s. ", CCCYN(ch, C_NRM), k->mob_specials.ExtraAttack + 1, CCNRM(ch, C_NRM));
        send_to_char(ch, "Вероятность использования умений: %s%d%%%s. ", CCCYN(ch, C_NRM), k->mob_specials.LikeWork, CCNRM(ch, C_NRM));
        send_to_char(ch, "Убить до начала замакса: %s%d%s\r\n", CCCYN(ch, C_NRM), k->mob_specials.MaxFactor, CCNRM(ch, C_NRM));
        send_to_char(ch, "Умения:&c");
        for (const auto counter : AVAILABLE_SKILLS)
        {
            if (*skill_info[counter].name != '!' && k->get_skill(counter))
            {
                send_to_char(ch, " %s:[%3d]", skill_info[counter].name, k->get_skill(counter));
            }
        }
        send_to_char(ch, "&n\r\n");
    }
    else
    {
        k->char_specials.saved.act.sprintbits(player_bits, smallBuf, ",", 4);
        sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
        send_to_char(buf, ch);

        k->player_specials->saved.pref.sprintbits(preference_bits, smallBuf, ",", 4);
        sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
        send_to_char(buf, ch);

        if (IS_IMPL(ch))
        {
            sprintbitwd(k->player_specials->saved.GodsLike, godslike_bits, smallBuf, ",");
            sprintf(buf, "GFL: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
            send_to_char(buf, ch);
        }
    }

    if (IS_MOB(k))
    {
        sprintf(buf, "Mob СпецПроц: %s, NPC сила удара: %dd%d\r\n",
                (mob_index[GET_MOB_RNUM(k)].func ? "Есть" : "Нет"),
                k->mob_specials.damnodice, k->mob_specials.damsizedice);
        send_to_char(buf, ch);
    }
    sprintf(buf, "Несет - вес %d, предметов %d; ", IS_CARRYING_W(k), IS_CARRYING_N(k));

    for (i = 0, j = k->carrying; j; j = j->get_next_content(), i++) { ; }
    sprintf(buf + strlen(buf), "(в инвентаре) : %d, ", i);

    for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    {
        if (GET_EQ(k, i))
        {
            i2++;
        }
    }
    sprintf(buf2, "(надето): %d\r\n", i2);
    strcat(buf, buf2);
    send_to_char(buf, ch);

    if (!IS_NPC(k))
    {
        sprintf(buf, "Голод: %d, Жажда: %d, Опьянение: %d\r\n",
                GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
        send_to_char(buf, ch);
    }

    if (god_level >= LVL_GRGOD)
    {
        sprintf(buf, "Ведущий: %s, Ведомые:", (k->has_master() ? GET_NAME(k->get_master()) : "<нет>"));

        for (fol = k->followers; fol; fol = fol->next)
        {
            sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch, 0));
            strcat(buf, buf2);
            if (strlen(buf) >= 62)
            {
                if (fol->next)
                    send_to_char(strcat(buf, ",\r\n"), ch);
                else
                    send_to_char(strcat(buf, "\r\n"), ch);
                *buf = found = 0;
            }
        }

        if (*buf)
            send_to_char(strcat(buf, "\r\n"), ch);
    }
    // Showing the bitvector
    k->char_specials.saved.affected_by.sprintbits(affected_bits, smallBuf, ",", 4);
    sprintf(buf, "Аффекты: %s%s%s\r\n", CCYEL(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintf(buf, "&GПеревоплощений: %d\r\n&n", GET_REMORT(k));
    send_to_char(buf, ch);
    // Routine to show what spells a char is affected by
    if (!k->affected.empty())
    {
        for (const auto aff : k->affected)
        {
            *buf2 = '\0';
            sprintf(buf, "Заклинания: (%3d%s|%s) %s%-21s%s ", aff->duration + 1,
                    (aff->battleflag & AF_PULSEDEC) || (aff->battleflag & AF_SAME_TIME) ? "плс" : "мин",
                    (aff->battleflag & AF_BATTLEDEC) || (aff->battleflag & AF_SAME_TIME) ? "рнд" : "мин",
                    CCCYN(ch, C_NRM), spell_name(aff->type), CCNRM(ch, C_NRM));
            if (aff->modifier)
            {
                sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int)aff->location]);
                strcat(buf, buf2);
            }
            if (aff->bitvector)
            {
                if (*buf2)
                {
                    strcat(buf, ", sets ");
                }
                else
                {
                    strcat(buf, "sets ");
                }
                sprintbit(aff->bitvector, affected_bits, buf2);
                strcat(buf, buf2);
            }
            send_to_char(strcat(buf, "\r\n"), ch);
        }
    }

    // check mobiles for a script
    if (IS_NPC(k) && god_level >= LVL_BUILDER)
    {
        do_sstat_character(ch, k);
        if (MEMORY(k))
        {
            struct memory_rec_struct *memchar;
            send_to_char("Помнит:\r\n", ch);
            for (memchar = MEMORY(k); memchar; memchar = memchar->next)
            {
                sprintf(buf, "%10ld - %10ld\r\n",
                        static_cast<long>(memchar->id),
                        static_cast<long>(memchar->time - time(NULL)));
                send_to_char(buf, ch);
            }
        }
    }
    else  		// this is a PC, display their global variables
    {
        if (SCRIPT(k)->global_vars)
        {
            struct trig_var_data *tv;
            char name[MAX_INPUT_LENGTH];
            void find_uid_name(char *uid, char *name);
            send_to_char("Глобальные переменные:\r\n", ch);
            // currently, variable context for players is always 0, so it is
            // not displayed here. in the future, this might change
            for (tv = k->script->global_vars; tv; tv = tv->next)
            {
                if (*(tv->value) == UID_CHAR)
                {
                    find_uid_name(tv->value, name);
                    sprintf(buf, "    %10s:  [CharUID]: %s\r\n", tv->name, name);
                }
                else if (*(tv->value) == UID_OBJ)
                {
                    find_uid_name(tv->value, name);
                    sprintf(buf, "    %10s:  [ObjUID]: %s\r\n", tv->name, name);
                }
                else if (*(tv->value) == UID_ROOM)
                {
                    find_uid_name(tv->value, name);
                    sprintf(buf, "    %10s:  [RoomUID]: %s\r\n", tv->name, name);
                }
                else
                    sprintf(buf, "    %10s:  %s\r\n", tv->name, tv->value);
                send_to_char(buf, ch);
            }
        }

        std::string quested(k->quested_print());
        if (!quested.empty())
            send_to_char(ch, "Выполнил квесты:\r\n%s\r\n", quested.c_str());

        if (RENTABLE(k))
        {
            sprintf(buf, "Не может уйти на постой %ld\r\n", static_cast<long int>(RENTABLE(k) - time(0)));
            send_to_char(buf, ch);
        }
        if (AGRO(k))
        {
            sprintf(buf, "Агрессор %ld\r\n", static_cast<long int>(AGRO(k) - time(NULL)));
            send_to_char(buf, ch);
        }
        pk_list_sprintf(k, buf);
        send_to_char(buf, ch);
        // Отображаем карму.
        if (KARMA(k))
        {
            sprintf(buf, "Карма:\r\n%s", KARMA(k));
            send_to_char(buf, ch);
        }

    }
}

void do_stat_object(CHAR_DATA * ch, OBJ_DATA * j, const int virt = 0)
{
    int i, found;
    obj_vnum rnum, vnum;
    OBJ_DATA *j2;
    long int li;
    bool is_grgod = (IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO)) ? true : false;

    vnum = GET_OBJ_VNUM(j);
    rnum = GET_OBJ_RNUM(j);
    sprintf(buf, "Название: '%s%s%s',\r\nСинонимы: '&c%s&n',",
            CCYEL(ch, C_NRM),
            (!j->get_short_description().empty() ? j->get_short_description().c_str() : "<None>"),
            CCNRM(ch, C_NRM),
            j->get_aliases().c_str());
    send_to_char(buf, ch);
    if (j->get_custom_label() && j->get_custom_label()->label_text)
    {
        sprintf(buf, " нацарапано: '&c%s&n',", j->get_custom_label()->label_text);
        send_to_char(buf, ch);
    }
    sprintf(buf, "\r\n");
    send_to_char(buf, ch);
    sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
    if (rnum >= 0)
    {
        strcpy(buf2, (obj_proto.func(j->get_rnum()) ? "Есть" : "Нет"));
    }
    else
    {
        strcpy(buf2, "None");
    }

    send_to_char(ch, "VNum: [%s%5d%s], RNum: [%5d], UID: [%d], ID: [%ld]\r\n",
                 CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), GET_OBJ_UID(j), j->get_id());

    send_to_char(ch, "Расчет критерия: %f, мортов: (%f) \r\n", j->show_koef_obj(), j->show_mort_req());
    send_to_char(ch, "Тип: %s, СпецПроцедура: %s", buf1, buf2);

    if (GET_OBJ_OWNER(j))
    {
        send_to_char(ch, ", Владелец : %s", get_name_by_unique(GET_OBJ_OWNER(j)));
    }
    if (GET_OBJ_ZONE(j))
        send_to_char(ch, ", Принадлежит зоне VNUM : %d", zone_table[GET_OBJ_ZONE(j)].number);
    if (GET_OBJ_MAKER(j))
    {
        send_to_char(ch, ", Создатель : %s", get_name_by_unique(GET_OBJ_MAKER(j)));
    }
    if (GET_OBJ_PARENT(j))
    {
        send_to_char(ch, ", Родитель(VNum) : [%d]", GET_OBJ_PARENT(j));
    }
    if (GET_OBJ_CRAFTIMER(j) > 0)
    {
        send_to_char(ch, ", &Yскрафчена с таймером : [%d]&n", GET_OBJ_CRAFTIMER(j));
    }
    if (j->get_is_rename()) // изменены падежи
    {
        send_to_char(ch, ", &Gпадежи отличны от прототипа&n");
    }
    sprintf(buf, "\r\nL-Des: %s\r\n%s",
            !j->get_description().empty() ? j->get_description().c_str() : "Нет",
            CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    if (j->get_ex_description())
    {
        sprintf(buf, "Экстра описание:%s", CCCYN(ch, C_NRM));
        for (auto desc = j->get_ex_description(); desc; desc = desc->next)
        {
            strcat(buf, " ");
            strcat(buf, desc->keyword);
        }
        strcat(buf, CCNRM(ch, C_NRM));
        send_to_char(strcat(buf, "\r\n"), ch);
    }
    send_to_char("Может быть надет : ", ch);
    sprintbit(j->get_wear_flags(), wear_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    sprinttype(j->get_material(), material_name, buf2);
	snprintf(buf, MAX_STRING_LENGTH, "Материал : %s, макс.прочность : %d, тек.прочность : %d\r\n", 
		    buf2, j->get_maximum_durability(), j->get_current_durability());
	send_to_char(buf, ch);

    send_to_char("Неудобства : ", ch);
    j->get_no_flags().sprintbits(no_bits, buf, ",", 4);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    send_to_char("Запреты : ", ch);
    j->get_anti_flags().sprintbits(anti_bits, buf, ",", 4);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    send_to_char("Устанавливает аффекты : ", ch);
    j->get_affect_flags().sprintbits(weapon_affects, buf, ",", 4);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    send_to_char("Дополнительные флаги  : ", ch);
    GET_OBJ_EXTRA(j).sprintbits(extra_bits, buf, ",", 4);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "Вес: %d, Цена: %d, Рента(eq): %d, Рента(inv): %d, ",
            GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENTEQ(j), GET_OBJ_RENT(j));
    send_to_char(buf, ch);
    if (check_unlimited_timer(j))
        sprintf(buf, "Таймер: нерушимо\r\n");
    else
        sprintf(buf, "Таймер: %d\r\n", j->get_timer());
    send_to_char(buf, ch);

    auto room = get_room_where_obj(j);
    strcpy(buf, "Находится в комнате : ");
    if (room == NOWHERE || !is_grgod)
    {
        strcat(buf, "нигде");
    }
    else
    {
        sprintf(buf2, "%d", room);
        strcat(buf, buf2);
    }

    strcat(buf, ", В контейнере: ");
    if (j->get_in_obj() && is_grgod)
    {
        sprintf(buf2, "[%d] %s", GET_OBJ_VNUM(j->get_in_obj()), j->get_in_obj()->get_short_description().c_str());
        strcat(buf, buf2);
    }
    else
    {
        strcat(buf, "Нет");
    }

    strcat(buf, ", В инвентаре: ");
    if (j->get_carried_by() && is_grgod)
    {
        strcat(buf, GET_NAME(j->get_carried_by()));
    }
    else if (j->get_in_obj() && j->get_in_obj()->get_carried_by() && is_grgod)
    {
        strcat(buf, GET_NAME(j->get_in_obj()->get_carried_by()));
    }
    else
    {
        strcat(buf, "Нет");
    }

    strcat(buf, ", Надет: ");
    if (j->get_worn_by() && is_grgod)
    {
        strcat(buf, GET_NAME(j->get_worn_by()));
    }
    else if (j->get_in_obj() && j->get_in_obj()->get_worn_by() && is_grgod)
    {
        strcat(buf, GET_NAME(j->get_in_obj()->get_worn_by()));
    }
    else
    {
        strcat(buf, "Нет");
    }

    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    switch (GET_OBJ_TYPE(j))
    {
        case OBJ_DATA::ITEM_BOOK:

            switch (GET_OBJ_VAL(j, 0))
            {
                case BOOK_SPELL:
                    if (GET_OBJ_VAL(j, 1) >= 1 && GET_OBJ_VAL(j, 1) < MAX_SPELLS)
                    {
                        sprintf(buf, "содержит заклинание        : \"%s\"", spell_info[GET_OBJ_VAL(j, 1)].name);
                    }
                    else
                        sprintf(buf, "неверный номер заклинания");
                    break;
                case BOOK_SKILL:
                    if (GET_OBJ_VAL(j, 1) >= 1 && GET_OBJ_VAL(j, 1) < MAX_SKILL_NUM)
                    {
                        sprintf(buf, "содержит секрет умения     : \"%s\"", skill_info[GET_OBJ_VAL(j, 1)].name);
                    }
                    else
                        sprintf(buf, "неверный номер умения");
                    break;
                case BOOK_UPGRD:
                {
                    const auto skill_num = GET_OBJ_VAL(j, 1);
                    if (skill_num >= 1 && skill_num < MAX_SKILL_NUM)
                    {
                        if (GET_OBJ_VAL(j, 3) > 0)
                        {
                            sprintf(buf, "повышает умение \"%s\" (максимум %d)", skill_info[skill_num].name, GET_OBJ_VAL(j, 3));
                        }
                        else
                        {
                            sprintf(buf, "повышает умение \"%s\" (не больше максимума текущего перевоплощения)", skill_info[skill_num].name);
                        }
                    }
                    else
                        sprintf(buf, "неверный номер повышаемоего умения");
                }
                    break;
                case BOOK_FEAT:
                    if (GET_OBJ_VAL(j, 1) >= 1 && GET_OBJ_VAL(j, 1) < MAX_FEATS)
                    {
                        sprintf(buf, "содержит секрет способности : \"%s\"", feat_info[GET_OBJ_VAL(j, 1)].name);
                    }
                    else
                        sprintf(buf, "неверный номер способности");
                    break;
                case BOOK_RECPT:
                {
                    const auto recipe = im_get_recipe(GET_OBJ_VAL(j, 1));
                    if (recipe >= 0)
                    {
                        const auto recipelevel = MAX(GET_OBJ_VAL(j, 2), imrecipes[recipe].level);
                        const auto recipemort = imrecipes[recipe].remort;
                        if ((recipelevel >= 0) &&  (recipemort >= 0))
                        {
                            sprintf(buf, "содержит рецепт отвара     : \"%s\", уровень изучения: %d, количество ремортов: %d", imrecipes[recipe].name, recipelevel, recipemort);
                        }
                        else
                        {
                            sprintf(buf, "Некорректная запись рецепта (нет уровня или реморта)");
                        }
                    }
                    else
                        sprintf(buf, "Некорректная запись рецепта");
                }
                    break;
                default:
                    sprintf(buf, "НЕВЕРНО УКАЗАН ТИП КНИГИ");
                    break;
            }
            break;
        case OBJ_DATA::ITEM_LIGHT:
            if (GET_OBJ_VAL(j, 2) < 0)
            {
                strcpy(buf, "Вечный свет!");
            }
            else
            {
                sprintf(buf, "Осталось светить: [%d]", GET_OBJ_VAL(j, 2));
            }
            break;

        case OBJ_DATA::ITEM_SCROLL:
        case OBJ_DATA::ITEM_POTION:
            sprintf(buf, "Заклинания: (Уровень %d) %s, %s, %s",
                    GET_OBJ_VAL(j, 0),
                    spell_name(GET_OBJ_VAL(j, 1)),
                    spell_name(GET_OBJ_VAL(j, 2)),
                    spell_name(GET_OBJ_VAL(j, 3)));
            break;

        case OBJ_DATA::ITEM_WAND:
        case OBJ_DATA::ITEM_STAFF:
            sprintf(buf, "Заклинание: %s уровень %d, %d (из %d) зарядов осталось",
                    spell_name(GET_OBJ_VAL(j, 3)),
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 1));
            break;

        case OBJ_DATA::ITEM_WEAPON:
            sprintf(buf, "Повреждения: %dd%d, Тип повреждения: %d",
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3));
            break;

        case OBJ_DATA::ITEM_ARMOR:
        case OBJ_DATA::ITEM_ARMOR_LIGHT:
        case OBJ_DATA::ITEM_ARMOR_MEDIAN:
        case OBJ_DATA::ITEM_ARMOR_HEAVY:
            sprintf(buf, "AC: [%d]  Броня: [%d]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
            break;

        case OBJ_DATA::ITEM_TRAP:
            sprintf(buf, "Spell: %d, - Hitpoints: %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
            break;

        case OBJ_DATA::ITEM_CONTAINER:
            sprintbit(GET_OBJ_VAL(j, 1), container_bits, smallBuf);
            //sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Труп: %s",
            //	GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2), YESNO(GET_OBJ_VAL(j, 3)));
            if (IS_CORPSE(j))
            {
                sprintf(buf, "Объем: %d, Тип ключа: %s, VNUM моба: %d, Труп: да",
                        GET_OBJ_VAL(j, 0), smallBuf, GET_OBJ_VAL(j, 2));
            }
            else
            {
                sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Сложность замка: %d",
                        GET_OBJ_VAL(j, 0), smallBuf, GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
            }
            break;

        case OBJ_DATA::ITEM_DRINKCON:
        case OBJ_DATA::ITEM_FOUNTAIN:
            sprinttype(GET_OBJ_VAL(j, 2), drinks, smallBuf);
            {
                std::string spells = drinkcon::print_spells(ch, j);
                boost::trim(spells);
                sprintf(buf, "Обьем: %d, Содержит: %d, Таймер (если 1 отравлено): %d, Жидкость: %s\r\n%s",
                        GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3), smallBuf, spells.c_str());
            }
            break;

        case OBJ_DATA::ITEM_NOTE:
            sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
            break;

        case OBJ_DATA::ITEM_KEY:
            strcpy(buf, "");
            break;

        case OBJ_DATA::ITEM_FOOD:
            sprintf(buf, "Насыщает(час): %d, Таймер (если 1 отравлено): %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 3));
            break;

        case OBJ_DATA::ITEM_MONEY:
            sprintf(buf, "Сумма: %d\r\nВалюта: %s", GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1) == currency::GOLD ? "куны" :
                    GET_OBJ_VAL(j, 1) == currency::ICE ? "искристые снежинки" :
                    "что-то другое"
            );
            break;

        case OBJ_DATA::ITEM_INGREDIENT:
            sprintbit(GET_OBJ_SKILL(j), ingradient_bits, smallBuf);
            sprintf(buf, "ingr bits %s", smallBuf);

            if (IS_SET(GET_OBJ_SKILL(j), ITEM_CHECK_USES))
            {
                sprintf(buf + strlen(buf), "\r\nможно применить %d раз", GET_OBJ_VAL(j, 2));
            }

            if (IS_SET(GET_OBJ_SKILL(j), ITEM_CHECK_LAG))
            {
                sprintf(buf + strlen(buf), "\r\nможно применить 1 раз в %d сек", (i = GET_OBJ_VAL(j, 0) & 0xFF));
                if (GET_OBJ_VAL(j, 3) == 0 || GET_OBJ_VAL(j, 3) + i < time(NULL))
                    sprintf(buf + strlen(buf), "(можно применять).");
                else
                {
                    li = GET_OBJ_VAL(j, 3) + i - time(NULL);
                    sprintf(buf + strlen(buf), "(осталось %ld сек).", li);
                }
            }

            if (IS_SET(GET_OBJ_SKILL(j), ITEM_CHECK_LEVEL))
            {
                sprintf(buf+ strlen(buf), "\r\nможно применить с %d уровня.", (GET_OBJ_VAL(j, 0) >> 8) & 0x1F);
            }

            if ((i = real_object(GET_OBJ_VAL(j, 1))) >= 0)
            {
                sprintf(buf + strlen(buf), "\r\nпрототип %s%s%s.",
                        CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
            }
            break;
        case OBJ_DATA::ITEM_MAGIC_CONTAINER:
        case OBJ_DATA::ITEM_MAGIC_ARROW:
            sprintf(buf, "Заклинание: [%s]. Объем [%d]. Осталось стрел[%d].",
                    spell_name(GET_OBJ_VAL(j, 0)), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
            break;

        default:
            sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
                    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
            break;
    }
    send_to_char(strcat(buf, "\r\n"), ch);

    // * I deleted the "equipment status" code from here because it seemed
    // * more or less useless and just takes up valuable screen space.

    if (j->get_contains())
    {
        sprintf(buf, "\r\nСодержит:%s", CCGRN(ch, C_NRM));
        for (found = 0, j2 = j->get_contains(); j2; j2 = j2->get_next_content())
        {
            sprintf(buf2, "%s %s", found++ ? "," : "", j2->get_short_description().c_str());
            strcat(buf, buf2);
            if (strlen(buf) >= 62)
            {
                if (j2->get_next_content())
                {
                    send_to_char(strcat(buf, ",\r\n"), ch);
                }
                else
                {
                    send_to_char(strcat(buf, "\r\n"), ch);
                }
                *buf = found = 0;
            }
        }

        if (*buf)
        {
            send_to_char(strcat(buf, "\r\n"), ch);
        }
        send_to_char(CCNRM(ch, C_NRM), ch);
    }
    found = 0;
    send_to_char("Аффекты:", ch);
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
        if (j->get_affected(i).modifier)
        {
            sprinttype(j->get_affected(i).location, apply_types, smallBuf);
            sprintf(buf, "%s %+d to %s", found++ ? "," : "", j->get_affected(i).modifier, smallBuf);
            send_to_char(buf, ch);
        }
    }
    if (!found)
    {
        send_to_char(" Нет", ch);
    }

    if (j->has_skills())
    {
        CObjectPrototype::skills_t skills;
        j->get_skills(skills);
        int skill_num;
        int percent;

        send_to_char("\r\nУмения :", ch);
        for (const auto& it : skills)
        {
            skill_num = it.first;
            percent = it.second;

            if (percent == 0) // TODO: такого не должно быть?
            {
                continue;
            }

            sprintf(buf, " %+d%% to %s", percent, skill_info[skill_num].name);
            send_to_char(buf, ch);
        }
    }
    send_to_char("\r\n", ch);

    if (j->get_ilevel() > 0)
    {
        send_to_char(ch, "Уровень (ilvl): %f\r\n", j->get_ilevel());
    }

    if (j->get_minimum_remorts() != 0)
    {
        send_to_char(ch, "Проставлено поле перевоплощений: %d\r\n", j->get_minimum_remorts());
    }
    else if (j->get_auto_mort_req() > 0)
    {
        send_to_char(ch, "Вычислено поле минимальных перевоплощений: %d\r\n", j->get_auto_mort_req());
    }

    if (is_grgod)
    {
        sprintf(buf, "Сейчас в мире : %d. На постое : %d. Макс в мире: %d\r\n",
                rnum >= 0 ? obj_proto.number(rnum) - (virt ? 1 : 0) : -1, rnum >= 0 ? obj_proto.stored(rnum) : -1, GET_OBJ_MIW(j));
        send_to_char(buf, ch);
        // check the object for a script
        do_sstat_object(ch, j);
    }
}

void do_stat_room(CHAR_DATA * ch, const int rnum = 0) {
    ROOM_DATA *rm = world[ch->in_room];
    int i, found;
    OBJ_DATA *j;
    CHAR_DATA *k;
    char tmpBuf[255];

    if (rnum != 0)
    {
        rm = world[rnum];
    }

    sprintf(buf, "Комната : %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprinttype(rm->sector_type, sector_types, smallBuf);
    sprintf(buf,
            "Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип  сектора: %s\r\n",
            zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number, CCNRM(ch, C_NRM), ch->in_room, smallBuf);
    send_to_char(buf, ch);

    rm->flags_sprint(smallBuf, ",");
    sprintf(buf, "СпецПроцедура: %s, Флаги: %s\r\n", (rm->func == NULL) ? "None" : "Exists", smallBuf);
    send_to_char(buf, ch);

    send_to_char("Описание:\r\n", ch);
    send_to_char(RoomDescription::show_desc(rm->description_num), ch);

    if (rm->ex_description)
    {
        sprintf(buf, "Доп. описание:%s", CCCYN(ch, C_NRM));
        for (auto desc = rm->ex_description; desc; desc = desc->next)
        {
            strcat(buf, " ");
            strcat(buf, desc->keyword);
        }
        strcat(buf, CCNRM(ch, C_NRM));
        send_to_char(strcat(buf, "\r\n"), ch);
    }
    sprintf(buf, "Живые существа:%s", CCYEL(ch, C_NRM));
    found = 0;
    size_t counter = 0;
    for (auto k_i = rm->people.begin(); k_i != rm->people.end(); ++k_i)
    {
        const auto k = *k_i;
        ++counter;

        if (!CAN_SEE(ch, k))
        {
            continue;
        }
        sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
                (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
        strcat(buf, buf2);
        if (strlen(buf) >= 62)
        {
            if (counter != rm->people.size())
            {
                send_to_char(strcat(buf, ",\r\n"), ch);
            }
            else
            {
                send_to_char(strcat(buf, "\r\n"), ch);
            }
            *buf = found = 0;
        }
    }

    if (*buf)
    {
        send_to_char(strcat(buf, "\r\n"), ch);
    }
    send_to_char(CCNRM(ch, C_NRM), ch);

    if (rm->contents)
    {
        sprintf(buf, "Предметы:%s", CCGRN(ch, C_NRM));
        for (found = 0, j = rm->contents; j; j = j->get_next_content())
        {
            if (!CAN_SEE_OBJ(ch, j))
                continue;
            sprintf(buf2, "%s %s", found++ ? "," : "", j->get_short_description().c_str());
            strcat(buf, buf2);
            if (strlen(buf) >= 62)
            {
                if (j->get_next_content())
                {
                    send_to_char(strcat(buf, ",\r\n"), ch);
                }
                else
                {
                    send_to_char(strcat(buf, "\r\n"), ch);
                }
                *buf = found = 0;
            }
        }

        if (*buf)
        {
            send_to_char(strcat(buf, "\r\n"), ch);
        }
        send_to_char(CCNRM(ch, C_NRM), ch);
    }
    for (i = 0; i < NUM_OF_DIRS; i++)
    {
        if (rm->dir_option[i])
        {
            if (rm->dir_option[i]->to_room() == NOWHERE)
                sprintf(smallBuf, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
            else
                sprintf(smallBuf, "%s%5d%s", CCCYN(ch, C_NRM),
                        GET_ROOM_VNUM(rm->dir_option[i]->to_room()), CCNRM(ch, C_NRM));
            sprintbit(rm->dir_option[i]->exit_info, exit_bits, tmpBuf);
            sprintf(buf,
                    "Выход %s%-5s%s:  Ведет в : [%s], Ключ: [%5d], Название: %s (%s), Тип: %s\r\n",
                    CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), smallBuf,
                    rm->dir_option[i]->key,
                    rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
                    rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)", tmpBuf);
            send_to_char(buf, ch);
            if (!rm->dir_option[i]->general_description.empty())
            {
                sprintf(buf, "  %s\r\n", rm->dir_option[i]->general_description.c_str());
            }
            else
            {
                strcpy(buf, "  Нет описания выхода.\r\n");
            }
            send_to_char(buf, ch);
        }
    }

    if (!rm->affected.empty())
    {
        sprintf(buf1, " Аффекты на комнате:\r\n");
        for (const auto& aff : rm->affected)
        {
            sprintf(buf1 + strlen(buf1), "       Заклинание \"%s\" (%d) - %s.\r\n",
                    spell_name(aff->type),
                    aff->duration,
                    ((k = find_char(aff->caster_id))
                     ? GET_NAME(k)
                     : "неизвестно"));
        }
        send_to_char(buf1, ch);
    }
    // check the room for a script
    do_sstat_room(rm, ch);
}

void do_stat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
    CHAR_DATA *victim;
    OBJ_DATA *object;
    int tmp;

    half_chop(argument, buf1, buf2);

    if (!*buf1)
    {
        send_to_char("Состояние КОГО или ЧЕГО?\r\n", ch);
        return;
    }

    int level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);

    if (is_abbrev(buf1, "room") && level >= LVL_BUILDER)
    {
        int vnum, rnum = NOWHERE;
        if (*buf2 && (vnum = atoi(buf2)))
        {
            if ((rnum = real_room(vnum)) != NOWHERE)
                do_stat_room(ch, rnum);
            else
                send_to_char("Состояние какой комнаты?\r\n", ch);
        }
        if (!*buf2)
            do_stat_room(ch);
    }
    else if (is_abbrev(buf1, "mob") && level >= LVL_BUILDER)
    {
        if (!*buf2)
            send_to_char("Состояние какого создания?\r\n", ch);
        else
        {
            if ((victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)) != nullptr)
                do_stat_character(ch, victim, 0);
            else
                send_to_char("Нет такого создания в этом МАДе.\r\n", ch);
        }
    }
    else if (is_abbrev(buf1, "player"))
    {
        if (!*buf2)
        {
            send_to_char("Состояние какого игрока?\r\n", ch);
        }
        else
        {
            if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != nullptr)
                do_stat_character(ch, victim);
            else
                send_to_char("Этого персонажа сейчас нет в игре.\r\n", ch);
        }
    }
    else if (is_abbrev(buf1, "ip"))
    {
        if (!*buf2)
        {
            send_to_char("Состояние ip какого игрока?\r\n", ch);
        }
        else
        {
            if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != nullptr)
            {
                do_statip(ch, victim);
                return;
            }
            else
            {
                send_to_char("Этого персонажа сейчас нет в игре, смотрим пфайл.\r\n", ch);
            }

            Player t_vict;
            if (load_char(buf2, &t_vict) > -1)
            {
                //Clan::SetClanData(&t_vict); не понял зачем проставлять клановый статус тут?
                do_statip(ch, &t_vict);
            }
            else
            {
                send_to_char("Такого игрока нет ВООБЩЕ.\r\n", ch);
            }
        }
    }
    else if (is_abbrev(buf1, "file"))
    {
        if (!*buf2)
        {
            send_to_char("Состояние какого игрока(из файла)?\r\n", ch);
        }
        else
        {
            Player t_vict;
            if (load_char(buf2, &t_vict) > -1)
            {
                if (GET_LEVEL(&t_vict) > level)
                {
                    send_to_char("Извините, вам это еще рано.\r\n", ch);
                }
                else
                {
                    Clan::SetClanData(&t_vict);
                    do_stat_character(ch, &t_vict);
                }
            }
            else
            {
                send_to_char("Такого игрока нет ВООБЩЕ.\r\n", ch);
            }
        }
    }
    else if (is_abbrev(buf1, "object") && level >= LVL_BUILDER)
    {
        if (!*buf2)
            send_to_char("Состояние какого предмета?\r\n", ch);
        else
        {
            if ((object = get_obj_vis(ch, buf2)) != NULL)
                do_stat_object(ch, object);
            else
                send_to_char("Нет такого предмета в игре.\r\n", ch);
        }
    }
    else
    {
        if (level >= LVL_BUILDER)
        {
            if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != NULL)
                do_stat_object(ch, object);
            else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != NULL)
                do_stat_object(ch, object);
            else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
                do_stat_character(ch, victim);
            else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room]->contents)) != NULL)
                do_stat_object(ch, object);
            else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
                do_stat_character(ch, victim);
            else if ((object = get_obj_vis(ch, buf1)) != NULL)
                do_stat_object(ch, object);
            else
                send_to_char("Ничего похожего с этим именем нет.\r\n", ch);
        }
        else
        {
            if ((victim = get_player_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
                do_stat_character(ch, victim);
            else if ((victim = get_player_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
                do_stat_character(ch, victim);
            else
                send_to_char("Никого похожего с этим именем нет.\r\n", ch);
        }
    }
}
