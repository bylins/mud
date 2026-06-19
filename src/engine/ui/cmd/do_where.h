//
// Created by Sventovit on 08.09.2024.
//

#ifndef BYLINS_SRC_CMD_WHERE_H_
#define BYLINS_SRC_CMD_WHERE_H_

#include <string>
#include <vector>

class CharData;
class ObjData;

void DoWhere(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DoFindObjByRnum(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

// Форматирование вывода команды 'где'. Вынесено сюда (без зависимостей от
// состояния мира), чтобы покрыть вёрстку юнит-тестами; см. tests/where.format.cpp.
namespace where_format {

enum class ERowKind {
	kMob,
	kPlayer,
	kObject
};

// Одна строка результата 'где' с уже разрезолвленной локацией.
// location_lines: первая строка печатается после разделителя " - ",
// остальные - выровненными строками-продолжениями (вложенные контейнеры).
struct WhereRow {
	int num = 0;
	ERowKind kind = ERowKind::kObject;
	int vnum = 0;
	std::string name;
	std::vector<std::string> location_lines;
};

// Рендерит весь блок вывода 'где' единой колоночной сеткой:
//   {num}. {Моб|Игрок|Вещь} [{vnum}] {имя} - {локация}
std::string FormatWhere(const std::vector<WhereRow> &rows);

} // namespace where_format

#endif //BYLINS_SRC_CMD_WHERE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
