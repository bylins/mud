/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_TRAINERS_H_
#define BYLINS_SRC_GAME_MECHANICS_TRAINERS_H_

class CharData;

void init_guilds();
int guild_mono(CharData *ch, void *me, int cmd, char *argument);
int guild_poly(CharData *ch, void *me, int cmd, char *argument);

#endif //BYLINS_SRC_GAME_MECHANICS_TRAINERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
