/*
 \authors Created by Sventovit
 \date 26.02.2022.
 \brief Команды "счет"
 \details Команды "счет" - основная статистика персонажа игрока. Включает краткий счет, счет все и вариант для
 слабовидящих.
 */
#ifndef BYLINS_SRC_CMD_SCORE_H_
#define BYLINS_SRC_CMD_SCORE_H_

class CharData;

void DoScore(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_SCORE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
