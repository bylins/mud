// Вёрстка короткого списка команды "кто" (who_format::FormatShortCell).
//
// Ячейки короткого списка склеиваются по четыре в строку, без разделителей, поэтому вся
// вёрстка держится на том, что ячейка всегда одной ширины. Название класса в префиксе
// не добиралось до общей ширины, и разница между "татью" (4 буквы) и "чернокнижником"
// (12) уезжала в отступ следующей колонки: список съезжал ступеньками.

#include "engine/ui/cmd/do_who.h"
#include "utils/native_text.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

using who_format::FormatShortCell;
using who_format::kNameWidth;
using who_format::PrefixWidth;

// Самое длинное название класса в конфиге -- "чернокнижник".
constexpr std::size_t kClassWidth = 12;

// Ожидаемая видимая ширина ячейки: префикс, пробел, колонка имени.
constexpr std::size_t kCellWidth = PrefixWidth(kClassWidth) + 1 + kNameWidth;

std::string Cell(int level, const std::string &char_class, const std::string &name) {
	return FormatShortCell(level, char_class, kClassWidth, name, "", "");
}

}  // namespace

TEST(WhoShortFormat, CellWidthDoesNotDependOnClassName) {
	// Классы разной длины: короткое имя класса раньше давало короткую ячейку.
	for (const auto *char_class : {"тать", "колдун", "богатырь", "чернокнижник"}) {
		const std::string cell = Cell(29, char_class, "Рогоза");
		EXPECT_EQ(native_text::char_count(cell), kCellWidth)
			<< "класс \"" << char_class << "\" даёт ячейку другой ширины";
	}
}

TEST(WhoShortFormat, CellWidthDoesNotDependOnName) {
	// Имя добирается до kNameWidth, самое длинное (kMaxNameLength) обязано влезать.
	for (const auto *name : {"Ян", "Рогоза", "Елизавета", "Твердиславополозий"}) {
		const std::string cell = Cell(30, "колдун", name);
		EXPECT_EQ(native_text::char_count(cell), kCellWidth)
			<< "имя \"" << name << "\" даёт ячейку другой ширины";
	}
}

TEST(WhoShortFormat, CellWidthIsMeasuredInCharactersNotBytes) {
	// Кириллица в UTF-8 двухбайтовая: байтовая ширина ячейки с русским именем больше
	// видимой, и колонка, отмеренная в байтах, оказалась бы короче нужной.
	const std::string cyrillic = Cell(29, "колдун", "Рогоза");
	const std::string latin = Cell(29, "conjurer", "Rogoza");
	EXPECT_EQ(native_text::char_count(cyrillic), native_text::char_count(latin));
	EXPECT_GT(cyrillic.size(), latin.size()) << "иначе тест не про UTF-8";
}

TEST(WhoShortFormat, NamesStartAtTheSameColumn) {
	// Главное свойство для читаемости списка: имена выстраиваются в колонку независимо
	// от класса. Проверяется по позиции первой буквы имени.
	std::vector<std::size_t> positions;
	for (const auto *char_class : {"тать", "колдун", "чернокнижник"}) {
		const std::string cell = Cell(29, char_class, "Рогоза");
		const auto at = cell.find("Рогоза");
		ASSERT_NE(at, std::string::npos);
		positions.push_back(native_text::char_count(cell.substr(0, at)));
	}
	EXPECT_EQ(positions[0], positions[1]);
	EXPECT_EQ(positions[1], positions[2]);
}

TEST(WhoShortFormat, ColorCodesDoNotEatTheWidth) {
	// Цветовой код -- невидимые символы, и ширина добирается внутри них: ячейка с цветом
	// отличается от бесцветной ровно на длину кодов, а не на добитые пробелы.
	const std::string colour = "\x1B[1;32m";
	const std::string reset = "\x1B[0;37m";
	const std::string plain = Cell(29, "колдун", "Рогоза");
	const std::string colored =
		FormatShortCell(29, "колдун", kClassWidth, "Рогоза", colour, reset);
	EXPECT_EQ(colored.size(), plain.size() + colour.size() + reset.size());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
