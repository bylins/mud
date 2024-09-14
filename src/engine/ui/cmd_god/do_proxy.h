/**
\file do_proxy.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Команда регистрации прокси. Должна использовать административную механику из proxy.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_PROXY_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_PROXY_H_

class CharData;
void do_proxy(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_PROXY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
