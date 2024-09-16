/**
\file do_ban.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Команда бан. Должна использовать административную механику из ban.h
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_BAN_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_BAN_H_

class CharData;
void do_ban(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_unban(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_BAN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
