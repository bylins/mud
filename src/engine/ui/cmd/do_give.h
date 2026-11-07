#ifndef BYLINS_SRC_CMD_GIVE_H_
#define BYLINS_SRC_CMD_GIVE_H_

#include <functional>
#include <string>

class CharData;

void do_give(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

// Поиск получателя для "дать". Игроку его ищет give_find_vict -- только того, кого игрок
// видит; триггерной передаче (do_mgive в dg_mobcmd.cpp) видимость не нужна. Возвращает
// nullptr, если получателя нет; сообщить об этом -- дело самого поисковика, он один знает,
// кому и куда писать.
using GiveVictFinder = std::function<CharData *(CharData *, const std::string &)>;

// Разбор аргументов "дать" (вещь, "все", "все.<метка>", N кун, N <валюта>) с внешним поиском
// получателя -- общая часть команды игрока и команды моба из триггера. Разбор держим в одном
// месте, чтобы триггерная передача понимала ровно то же, что игроковая.
void PerformGiveCommand(CharData *ch, char *argument, const GiveVictFinder &find_vict);

// Штатный поиск получателя для игрока: своя комната плюс проверка видимости.
CharData *give_find_vict(CharData *ch, const std::string &local_arg);

#endif //BYLINS_SRC_CMD_GIVE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
