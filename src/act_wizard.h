#ifndef __ACT_WIZARD_HPP__
#define __ACT_WIZARD_HPP__

#include "sysdep.h"
#include "structs/structs.h"

#include <memory>
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

using SetAllInspReqPtr = std::shared_ptr<setall_inspect_request>;
using SetAllInspReqListType = std::map<int, SetAllInspReqPtr>;
extern SetAllInspReqListType &setall_inspect_list;
void setall_inspect();
ZoneRnum ZoneCopy(ZoneVnum zvn_from);

#endif // __ACT_WIZARD_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
