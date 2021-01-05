/* ************************************************************************
*   File: GroupPenalties.cpp                            Part of Bylins    *
*   Класс расчета всяких штрафов при вступлении в группу                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
************************************************************************ */

#include "grp.main.h"

#include "chars/char_player.hpp"
#include "db.h"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

extern GroupRoster& groupRoster;

/*
	Берет misc/grouping, первый столбик цифр считает номерами мортов,
	остальные столбики - значение макс. разрыва в уровнях для конкретного
	класса. На момент написания этого в конфиге присутствует 26 строк, макс.
	морт равен 50 - строки с мортами с 26 по 50 копируются с 25-мортовой строки.
*/

int GroupPenalties::init()
{
    int clss = 0, remorts = 0, rows_assigned = 0, levels = 0, pos = 0, max_rows = MAX_REMORT+1;

    // пре-инициализация
    for (remorts = 0; remorts < max_rows; remorts++) //Строк в массиве должно быть на 1 больше, чем макс. морт
    {
        for (clss = 0; clss < NUM_PLAYER_CLASSES; clss++) //Столбцов в массиве должно быть ровно столько же, сколько есть классов
        {
            m_grouping[clss][remorts] = -1;
        }
    }

    FILE* f = fopen(LIB_MISC "grouping", "r");
    if (!f)
    {
        log("Невозможно открыть файл %s", LIB_MISC "grouping");
        return 1;
    }

    while (get_line(f, buf))
    {
        if (!buf[0] || buf[0] == ';' || buf[0] == '\n') //Строка пустая или строка-коммент
        {
            continue;
        }
        clss = 0;
        pos = 0;
        while (sscanf(&buf[pos], "%d", &levels) == 1)
        {
            while (buf[pos] == ' ' || buf[pos] == '\t')
            {
                pos++;
            }
            if (clss == 0) //Первый проход цикла по строке
            {
                remorts = levels; //Номера строк
                if (m_grouping[0][remorts] != -1)
                {
                    log("Ошибка при чтении файла %s: дублирование параметров для %d ремортов",
                        LIB_MISC "grouping", remorts);
                    return 2;
                }
                if (remorts > MAX_REMORT || remorts < 0)
                {
                    log("Ошибка при чтении файла %s: неверное значение количества ремортов: %d, "
                        "должно быть в промежутке от 0 до %d",
                        LIB_MISC "grouping", remorts, MAX_REMORT);
                    return 3;
                }
            }
            else
            {
                m_grouping[clss - 1][remorts] = levels; // -1 потому что в массиве нет столбца с кол-вом мортов
            }
            clss++; //+Номер столбца массива
            while (buf[pos] != ' ' && buf[pos] != '\t' && buf[pos] != 0)
            {
                pos++; //Ищем следующее число в строке конфига
            }
        }
        if (clss != NUM_PLAYER_CLASSES+1)
        {
            log("Ошибка при чтении файла %s: неверный формат строки '%s', должно быть %d "
                "целых чисел, прочитали %d", LIB_MISC "grouping", buf, NUM_PLAYER_CLASSES+1, clss);
            return 4;
        }
        rows_assigned++;
    }

    if (rows_assigned < max_rows)
    {
        for (levels = remorts; levels < max_rows; levels++) //Берем свободную переменную
        {
            for (clss = 0; clss < NUM_PLAYER_CLASSES; clss++)
            {
                m_grouping[clss][levels] = m_grouping[clss][remorts]; //Копируем последнюю строку на все морты, для которых нет строк
            }
        }
    }
    fclose(f);
    return 0;
}

int grouping_koef(int player_class, int player_remort)
{
    if ((player_class >= NUM_PLAYER_CLASSES) || (player_class < 0))
        return 1;
    return groupRoster.grouping[player_class][player_remort];

}