// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef FILE_CRC_HPP_INCLUDED
#define FILE_CRC_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

namespace FileCRC {

// Тип файла игрока, для которого считается/сверяется CRC. Глагол (запись,
// сверка, сброс) определяется именем функции, поэтому достаточно типа файла.
enum EType { kPlayer, kTextObjs, kTimeObjs };

void load();
void save(bool force_save = false);
void show(CharData *ch);
// Записывает CRC из готового буфера в памяти (при сохранении), без чтения файла.
void update_from_content(long uid, EType file, const char *data, std::size_t len);
// Сбрасывает CRC указанного файла игрока в 0 (файл удалён).
void reset(long uid, EType file);
// Сверяет CRC из готового буфера со снимком (при загрузке, вместо чтения файла).
// true -- совпало или снимка не было; false + сообщение имму -- при расхождении.
bool verify_from_content(long uid, EType file, const char *data, std::size_t len);
// Забывает все снимки: каждый следующий прочитанный файл станет новым эталоном. Нужно после
// планового восстановления из архива, когда файлы игроков откатились, а снимки остались от
// более позднего состояния и сверка ругается на каждого входящего. Возвращает число забытых.
std::size_t forget_all();

} // namespace FileCRC

#endif // FILE_CRC_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
