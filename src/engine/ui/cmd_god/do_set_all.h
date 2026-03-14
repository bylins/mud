/**
\file set_all.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_SET_ALL_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_SET_ALL_H_

#include <memory>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#ifdef __MINGW32__
#include <sys/time.h>
#endif
#else
#include <sys/time.h>
#endif
#include <map>

struct setall_inspect_request {
  int unique; // UID
  int found; //сколько найдено
  int type_req; // тип запроса: фриз, смена мыла, пароля или ад (0, 1, 2 или 3)
  int freeze_time; // время, на которое фризим
  char *mail; // мыло игрока
  char *pwd; // пароль
  char *newmail; // новое мыло
  char *reason; // причина для фриза
  int pos; // позиция в таблице
  struct timeval start;        //время когда запустили запрос для отладки
  std::string out;        //буфер в который накапливается вывод
};

class CharData;
using SetAllInspReqPtr = std::shared_ptr<setall_inspect_request>;
using SetAllInspReqListType = std::map<long, SetAllInspReqPtr>;
extern SetAllInspReqListType &setall_inspect_list;
void setall_inspect();
void do_setall(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_SET_ALL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
