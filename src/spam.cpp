// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <list>
#include "spam.hpp"
#include "db.h"
#include "utils.h"
#include "char.hpp"
#include "comm.h"
#include "screen.h"

using namespace SpamSystem;

namespace
{

// макс сообщений подряд от одного чара
const int MAX_MESSAGE_RUNNING = 10;
// х последних сообщений, обрабатываемых для проверок
const unsigned MESSAGE_LIST_SIZE = 31;
// макс сообщений от одного чара за последние MESSAGE_LIST_SIZE сообщений
const int MAX_MESSAGE_TOTAL = 15;
// секунд между мессагами от одного чара (вместо тупо лага)
const int MIN_MESSAGE_TIME = 2;

typedef std::list<std::pair<long, time_t> > UidListType;

UidListType offtop_list;

void add_to_list(UidListType &list, long uid)
{
	list.push_front(std::make_pair(uid, time(0)));
	if (list.size() > MESSAGE_LIST_SIZE)
	{
		list.pop_back();
	}
};

bool check_list(const UidListType &list, long uid)
{
	int total = 0, running = 0;
	for (UidListType::const_iterator i = offtop_list.begin(); i != offtop_list.end(); ++i)
	{
		if (uid != i->first)
		{
			running = -1;
		}
		else
		{
			if (running != -1)
			{
				++running;
				if (running >= MAX_MESSAGE_RUNNING)
				{
					return false;
				}
			}
			if ((time(0) - i->second) < MIN_MESSAGE_TIME)
			{
					return false;
			}
			++total;
		}
	}
	if (total >= MAX_MESSAGE_TOTAL)
	{
		return false;
	}
	return true;
};

/**
* \return - true = чар может говорить
*/
bool add_message(int mode, long uid)
{
	switch(mode)
	{
	case OFFTOP_MODE:
		if (check_list(offtop_list, uid))
		{
			add_to_list(offtop_list, uid);
		}
		else
		{
			return false;
		}
		break;
	default:
		sprintf(buf, "SYSERROR: мы не должны были сюда попасть (%s %s %d)", __FILE__, __func__, __LINE__);
		return false;
	};

	return true;
}

} // namespace

namespace SpamSystem
{

bool check(CHAR_DATA *ch, int mode)
{
	if (!add_message(mode, GET_UNIQUE(ch)))
	{
		send_to_char(ch, "Сообщение заблокировано спам-контролем.\r\n");
		return false;
	}
	return true;
}

void check_new_channels(CHAR_DATA *ch)
{
	if (MIN_OFFTOP_LVL == GET_LEVEL(ch) && !GET_REMORT(ch))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_OFFTOP_MODE), PRF_OFFTOP_MODE);
		send_to_char(ch, "\r\n%sТеперь Вы можете пользоваться каналом [оффтоп]!\r\n"
				"Рекомендуем преварительно ознакомиться со справкой (справка оффтоп).%s\r\n",
				CCIGRN(ch, C_SPR), CCNRM(ch, C_SPR));
	}
}

} // SpamSystem
