// Тесты вёрстки вывода команды "где" (where_format::FormatWhere).
// Проверяется весь блок вывода целиком: строка моба + строки предметов +
// строка-продолжение для предмета внутри контейнера.
//
// Кодировка KOI8-R: кириллица здесь однобайтовая, поэтому байтовая длина
// строки == ширине колонки, и индексная арифметика ниже корректна.

#include "engine/ui/cmd/do_where.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

using where_format::ERowKind;
using where_format::FormatWhere;
using where_format::WhereRow;

std::vector<std::string> SplitLines(const std::string &s) {
	std::vector<std::string> out;
	std::size_t pos = 0;
	std::size_t nl;
	while ((nl = s.find("\r\n", pos)) != std::string::npos) {
		out.push_back(s.substr(pos, nl - pos));
		pos = nl + 2;
	}
	if (pos < s.size()) {
		out.push_back(s.substr(pos));
	}
	return out;
}

WhereRow Mob(int num, int vnum, std::string name, std::string loc) {
	WhereRow r;
	r.num = num;
	r.kind = ERowKind::kMob;
	r.vnum = vnum;
	r.name = std::move(name);
	r.location_lines.push_back(std::move(loc));
	return r;
}

WhereRow Obj(int num, int vnum, std::string name, std::vector<std::string> locs) {
	WhereRow r;
	r.num = num;
	r.kind = ERowKind::kObject;
	r.vnum = vnum;
	r.name = std::move(name);
	r.location_lines = std::move(locs);
	return r;
}

// Сценарий из бага: моб с длинным именем + предметы, среди них предмет
// в контейнере (даёт строку-продолжение с локацией контейнера).
std::vector<WhereRow> SampleRows() {
	std::vector<WhereRow> rows;
	rows.push_back(Mob(1, 85106, "князь Всеволод Большое Гнездо",
			"[  85132] Княжьи терема. Зона: 'Ростов Великий'"));
	rows.push_back(Obj(2, 28602, "гнездо сороки", {"[  28675] На ели"}));
	rows.push_back(Obj(4, 6505, "хрупкое гнездо с птенцами",
			{"лежит в [6503] сухом дереве, который находится", "[   6510] На краю оврага"}));
	rows.push_back(Obj(10, 8507, "гнездо", {"[   8592] На дереве"}));
	return rows;
}

// Индексы строк в выводе SampleRows():
//   0 - моб (имя 29 симв., длиннее поля)
//   1 - "гнездо сороки"
//   2 - предмет в контейнере (имя ровно 25 симв.)
//   3 - строка-продолжение (локация контейнера)
//   4 - "гнездо"

} // namespace

// Каждая строка завершается \r\n и не имеет хвостового пробела (регрессия:
// раньше строка "...который находится \r\n" несла лишний пробел).
TEST(WhereFormat, NoTrailingSpace) {
	const auto out = FormatWhere(SampleRows());
	ASSERT_NE(out.find("\r\n"), std::string::npos);
	for (const auto &line : SplitLines(out)) {
		ASSERT_FALSE(line.empty());
		EXPECT_NE(line.back(), ' ') << "хвостовой пробел в строке: [" << line << "]";
	}
}

// Разделитель " - " (а значит и колонка локации) совпадает у всех строк
// с именем <= 25 символов и у строки-продолжения контейнера.
// Регрессия: продолжение печаталось на 1 колонку левее.
TEST(WhereFormat, LocationColumnAligned) {
	const auto lines = SplitLines(FormatWhere(SampleRows()));
	ASSERT_GE(lines.size(), 5u);

	const auto ref = lines[1].rfind(" - ");
	ASSERT_NE(ref, std::string::npos);
	EXPECT_EQ(lines[2].rfind(" - "), ref) << "предмет с именем ровно 25 симв.";
	EXPECT_EQ(lines[3].rfind(" - "), ref) << "строка-продолжение контейнера";
	EXPECT_EQ(lines[4].rfind(" - "), ref) << "короткое имя";

	// Имя моба (29 симв.) длиннее поля в 25 -> разделитель уезжает вправо на 4.
	EXPECT_EQ(lines[0].rfind(" - "), ref + 4);
}

// Предмет в контейнере даёт ровно две строки: первая оканчивается на
// "находится" (без рекурсии и без хвостового пробела), вторая - локация контейнера.
TEST(WhereFormat, ContainerYieldsContinuation) {
	const auto lines = SplitLines(FormatWhere(SampleRows()));
	ASSERT_GE(lines.size(), 5u);

	EXPECT_NE(lines[2].find("лежит в [6503] сухом дереве, который находится"), std::string::npos);
	const std::string tail = "находится";
	ASSERT_GE(lines[2].size(), tail.size());
	EXPECT_EQ(lines[2].compare(lines[2].size() - tail.size(), tail.size(), tail), 0);

	EXPECT_NE(lines[3].find("[   6510] На краю оврага"), std::string::npos);
}

// Номера выровнены в одну колонку (точка после номера на одной позиции
// у однозначных и двузначных номеров). Регрессия: у моба номер был шире.
TEST(WhereFormat, NumberColumnAligned) {
	const auto lines = SplitLines(FormatWhere(SampleRows()));
	ASSERT_GE(lines.size(), 5u);
	for (int i : {0, 1, 2, 4}) {
		ASSERT_GT(lines[i].size(), 3u);
		EXPECT_EQ(lines[i][2], '.') << "номер не выровнен в строке " << i;
	}
}

// Колонка типа: моб/предмет подписаны, у моба в хвосте зона.
TEST(WhereFormat, RowKindLabels) {
	const auto lines = SplitLines(FormatWhere(SampleRows()));
	ASSERT_GE(lines.size(), 2u);
	EXPECT_NE(lines[0].find("Моб"), std::string::npos);
	EXPECT_NE(lines[0].find("Зона: '"), std::string::npos);
	EXPECT_NE(lines[1].find("Вещь"), std::string::npos);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
