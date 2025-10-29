//
// Created by Sventovit on 08.09.2024.
//

#ifndef BYLINS_SRC_CMD_WHO_H_
#define BYLINS_SRC_CMD_WHO_H_

class CharData;

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
