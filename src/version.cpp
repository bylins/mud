// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"

/**
* Все файл - исключительно как попытка автоматической вставки в код нормальной даты сборки.
*/

void show_code_date(CHAR_DATA *ch)
{
	send_to_char(ch, "МПМ Былины, версия от %s\r\n", __DATE__);
}
