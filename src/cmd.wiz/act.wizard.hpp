#ifndef __ACT_WIZARD_HPP__
#define __ACT_WIZARD_HPP__

#include "sysdep.h"
#include "structs.h"

#include <memory>
#include <map>

struct inspect_request
{
	int sfor;			//тип запроса
	int unique;			//UID
	int fullsearch;			//полный поиск или нет
	int found;			//сколько всего найдено
	char *req;			//собственно сам запрос
	char *mail;			//мыло
	int pos;			//позиция в таблице
	std::vector<logon_data> ip_log;	//айпи адреса по которым идет поиск
	struct timeval start;		//время когда запустили запрос для отладки
	std::string out;		//буфер в который накапливается вывод
	bool sendmail; // отправлять ли на мыло список чаров
};

using InspReqPtr = std::shared_ptr<inspect_request>;
using InspReqListType = std::map<int /* filepos, позиция в player_table перса который делает запрос */, InspReqPtr /* сам запрос */>;

extern InspReqListType& inspect_list;

struct setall_inspect_request
{
	int unique; // UID
	int found; //сколько найдено
	int type_req; // тип запроса: фриз, смена мыла, пароля или ад (0, 1, 2 или 3)
	int freeze_time; // время, на которое фризим
	char *mail; // мыло игрока
	char *pwd; // пароль
	char *newmail; // новое мыло
	char *reason; // причина для фриза
	int pos; // позиция в таблице
	struct timeval start;		//время когда запустили запрос для отладки
	std::string out;		//буфер в который накапливается вывод
};

using SetAllInspReqPtr = std::shared_ptr<setall_inspect_request>;
using SetAllInspReqListType = std::map<int, SetAllInspReqPtr>;

extern SetAllInspReqListType& setall_inspect_list;

void inspecting();
void setall_inspect();

#endif // __ACT_WIZARD_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
