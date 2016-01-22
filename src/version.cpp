// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"

const char* revision = "$Revision$";

// * Весь файл - исключительно как попытка автоматической вставки в код нормальной даты сборки.

void show_code_date(CHAR_DATA *ch)
{
	send_to_char(ch, "МПМ Былины, версия от %s %s\r\n", __DATE__, __TIME__);
}

void log_code_date()
{
	log("Code version %s %s", __DATE__, __TIME__);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
