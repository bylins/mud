/*
 \authors Created by Sventovit
 \date 18.02.2022.
 \brief Обертка-помощник модуля таблиц.
 \details Обертка модуля таблиц. Цель в том, чтобы собрать в одном месте процедуры оформления таблиц: не стоит
 в каждом случае формировать окраску, отступы и цвета вручную, достаточно определить несколько основных типов: магазины,
 "красивые" таблицы, вроде "счет все" и "моястатистика", просто статистика, типа списка клана, и  служебные, для вывода
 информации иммморталам. Вторая задача - избежать повторения в формировании каждой таблицы кода выбора режима
 оформления для слабовидящих игроков. Наконец, третья задача - облегчить смену модуля таблиц при возникновении такой
 необходимости.

 Следует иметь в виду, что стиль таблицы FT_EMPTY2_STYLE в используемой сейчас библиотеке libfort слегка изменен,
 чтобы соответствовать нуждам незрячих игроков. Окнкретно - добавлен разделитель столбцов и заголовков столбцов
 в виде двоточия. При обновлении библиотеки стиль следует снова поменять (к сожалению, добавить соверщенно новый стиль
 в библиотеку несколько сложнее, а настройку оформления стиля она не поддерживает).

 Также нуэно учитывать, что в настоязий момент библиотека не подерживает и escape-последовательности цветов telnet.
 Чтобы цвета все-таки отображались, при9лось вручную заменть массив последовательностей. Что опять-таки приводит
 к необходимости повторить эту работу при обновлении библиотеки (при условии, что в новых версияъ не будет добавлена
 поддержка telnet, разумеется).
 */

#ifndef BYLINS_SRC_UTILS_TABLE_WRAPPER_H_
#define BYLINS_SRC_UTILS_TABLE_WRAPPER_H_

#include "utils/libfort/fort.hpp"

class CharData;

namespace table_wrapper {

[[maybe_unused]] const auto kDefaultColor	= fort::color::default_color;
[[maybe_unused]] const auto kBlack			= fort::color::black;
[[maybe_unused]] const auto kRed			= fort::color::red;
[[maybe_unused]] const auto kGreen			= fort::color::green;
[[maybe_unused]] const auto kYellow			= fort::color::yellow;
[[maybe_unused]] const auto kBlue			= fort::color::blue;
[[maybe_unused]] const auto kMagenta		= fort::color::magenta;
[[maybe_unused]] const auto kCyan			= fort::color::cyan;
[[maybe_unused]] const auto kLightGray		= fort::color::light_gray;
[[maybe_unused]] const auto kDarkGray		= fort::color::dark_gray;
[[maybe_unused]] const auto kLightRed		= fort::color::light_red;
[[maybe_unused]] const auto kLightGreen		= fort::color::light_green;
[[maybe_unused]] const auto kLightYellow	= fort::color::light_yellow;
[[maybe_unused]] const auto kLightBlue		= fort::color::light_blue;
[[maybe_unused]] const auto kLightMagenta	= fort::color::light_magenta;
[[maybe_unused]] const auto kLightCyan		= fort::color::light_cyan;
[[maybe_unused]] const auto kLightWhite		= fort::color::light_whyte;

/**
 * Оформить простую таблицу, с рамками, но без всяких цветов.
 * @param ch - персонаж, для которого формируется таблица.
 * @param table - оформляемая таблица.
 */
void DecorateSimpleTable(CharData *ch, fort::char_table &table);

/**
 * Оформить простейшую таблицу без рамок (для слабовидящих сохраняется сепаратор).
 * @param ch - персонаж, для которого формируется таблица.
 * @param table - оформляемая таблица.
 */
void DecorateNoBorderTable(CharData *ch, fort::char_table &table);

/**
 * Оформить сервисную таблицу - вывод различной статистики иммморталам.
 * @param ch - персонаж, для которого формируется таблица.
 * @param table - оформляемая таблица.
 */
void DecorateServiceTable(CharData *ch, fort::char_table &table);

/**
 * Оформить "красивую" таблицу - счет все и т.п.
 * @param ch - персонаж, для которого формируется таблица.
 * @param table - оформляемая таблица.
 */
void DecorateCuteTable(CharData *ch, fort::char_table &table);

/**
 * Оформить таблицу в стиле "зебры".
 * Каждая вторая строка, не считая заголовка, имеет фон указаного цвета.
 * @param ch - персонаж, для которого формируется таблица.
 * @param table - оформляемая таблица.
 * @param color - фоновый цвет строки.
 */
void DecorateZebraTable(CharData *ch, fort::char_table &table, fort::color color);

/**
 * Оформить таблицу в стиле текстовой "зебры".
 * Каждая вторая строка, не считая заголовка, имеет текст указанного цвета.
 * @param ch - персонаж, для которого формируется таблица.
 * @param table - оформляемая таблица.
 * @param color - фоновый цвет строки.
 */
void DecorateZebraTextTable(CharData *ch, fort::char_table &table, fort::color color);

/**
 *  Вывести таблицу персонажу. Возможное исключение перехватывается и обрабатывается.
 *  @param ch - персонаж, которому выводится таблица.
 *  @param table - распечатываемая таблица.
 */
void PrintTableToChar(CharData *ch, fort::char_table &table);

/**
 *  Вывести таблицу в указанный поток. Возможное исключение перехватывается и обрабатывается.
 *  @param ch - персонаж, которому выводится таблица.
 *  @param table - распечатываемая таблица.
 *  @param out - выводной поток.
 */
void PrintTableToStream(std::ostringstream &out, fort::char_table &table);

} // table_wrapper

#endif //BYLINS_SRC_UTILS_TABLE_WRAPPER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
