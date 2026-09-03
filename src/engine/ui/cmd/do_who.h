//
// Created by Sventovit on 08.09.2024.
//

#ifndef BYLINS_SRC_CMD_WHO_H_
#define BYLINS_SRC_CMD_WHO_H_

#include <cstddef>
#include <string>

class CharData;

// Вёрстка короткого списка ("кто -с"). Ячейки просто склеиваются по четыре в строку, поэтому
// ширина ячейки обязана быть постоянной: иначе разница в длине названия класса ("тать" против
// "чернокнижника" -- восемь знаков) уезжает в отступ следующей колонки, и список съезжает.
namespace who_format {

// Колонка имени: kMaxNameLength (20) плюс пробел-разделитель.
inline constexpr std::size_t kNameWidth = 21;

// Ширина префикса "[<уровень> <класс>]": скобки, два знака уровня и пробел.
[[nodiscard]] constexpr std::size_t PrefixWidth(std::size_t class_width) {
	return class_width + 5;
}

// Ячейка короткого списка. Цвет вставляется ВОКРУГ уже добитого пробелами имени: цветовой код --
// невидимые символы, и добирать ширину поверх них нельзя. class_width задаётся снаружи (ширина
// самого длинного названия класса в конфиге), чтобы вёрстку можно было проверить без конфига.
[[nodiscard]] std::string FormatShortCell(int level, const std::string &class_name,
										  std::size_t class_width, const std::string &name,
										  const std::string &name_color,
										  const std::string &color_end);

} // namespace who_format

// константы для спам-контроля команды кто
// если кто захочет и сможет вынести их во внешний конфиг, то почет ему и слава

// максимум маны
inline constexpr int kWhoManaMax{6000};
// расход на одно выполнение с выводом полного списка
inline constexpr int kWhoCost{180};
// расход на одно выполнение с поиском по имени
inline constexpr int kWhoCostName{30};
// расход на вывод списка по кланам
inline constexpr int kWhoCostClan{120};
// скорость восстановления
inline constexpr int kWhoManaRestPerSecond{9};
// режимы выполнения
inline constexpr int kWhoListall{0};
inline constexpr int kWhoListname{1};
inline constexpr int kWhoListclan{2};

void DoWho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
bool PerformWhoSpamcontrol(CharData *ch, unsigned short int mode = kWhoListall);

#endif //BYLINS_SRC_CMD_WHO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
