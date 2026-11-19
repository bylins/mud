// Строка триггера, которая целиком состоит из вызова поля с аргументом ("%actor.wait(2)%",
// "%world.loadobj(557, 200)%"), исполняется ради действия этого поля. Значение, которое поле
// при этом вернуло, остаётся единственным текстом в строке, и до правки уезжало в интерпретатор
// команд: в комнатном триггере это давало "Unknown world cmd: '0'" на каждом неудачном броске
// world.loadobj. Тест держит границы правила -- что считается таким вызовом, а что остаётся
// обычной командой.

#include <gtest/gtest.h>

#include "engine/scripting/dg_scripts.h"

namespace dg_field_call_statement_test {

// Вызов поля с аргументом -- действие. Значение не команда.
TEST(DgFieldCallStatement, FieldCallWithArgumentIsStatement) {
	EXPECT_TRUE(IsFieldCallStatement("%world.loadobj(557, 200)%"));
	EXPECT_TRUE(IsFieldCallStatement("%actor.wait(2)%"));
	EXPECT_TRUE(IsFieldCallStatement("%actor.gold(+400)%"));
	EXPECT_TRUE(IsFieldCallStatement("%actor.questbodrich(15)%"));
}

// Отступы и хвостовые пробелы ничего не меняют: в мире строки идут с отступом в теле if.
TEST(DgFieldCallStatement, IndentationDoesNotMatter) {
	EXPECT_TRUE(IsFieldCallStatement("      %world.loadobj(100109, 200)%"));
	EXPECT_TRUE(IsFieldCallStatement("  %actor.wait(5)%   "));
}

// Вложенная подстановка внутри скобок: закрывающий процент -- тот, что вне скобок.
TEST(DgFieldCallStatement, NestedSubstitutionInsideParens) {
	EXPECT_TRUE(IsFieldCallStatement("%world.loadobj(%vnum%, 200)%"));
	EXPECT_TRUE(IsFieldCallStatement("%i.position(%pos%)%"));
}

// Простая переменная -- её значение и есть команда: "%dir%" ведёт моба на север,
// "%purge%" и "%echo%" -- допущение для билдеров на имя команды.
TEST(DgFieldCallStatement, PlainVariableStaysCommand) {
	EXPECT_FALSE(IsFieldCallStatement("%dir%"));
	EXPECT_FALSE(IsFieldCallStatement("%direction%"));
	EXPECT_FALSE(IsFieldCallStatement("%purge%"));
	EXPECT_FALSE(IsFieldCallStatement("%echo%"));
}

// Поле без скобок тоже остаётся командой: "%cmd.mudcommand%" подставляет имя команды
// и ровно за этим и зовётся.
TEST(DgFieldCallStatement, FieldWithoutParensStaysCommand) {
	EXPECT_FALSE(IsFieldCallStatement("%cmd.mudcommand%"));
	EXPECT_FALSE(IsFieldCallStatement("%char.dispel%"));
	EXPECT_FALSE(IsFieldCallStatement("%prize.trim%"));
}

// Строка, где выражение лишь часть текста, -- обычная команда.
TEST(DgFieldCallStatement, ExpressionInsideLineStaysCommand) {
	EXPECT_FALSE(IsFieldCallStatement("%echo% текст %actor.name%"));
	EXPECT_FALSE(IsFieldCallStatement("%send% %actor% привет"));
	EXPECT_FALSE(IsFieldCallStatement("дать %obj.name(0)% %actor%"));
	EXPECT_FALSE(IsFieldCallStatement("mload obj 100"));
}

// Экранированный процент и обрывки не обманывают правило.
TEST(DgFieldCallStatement, MalformedLinesAreNotStatements) {
	EXPECT_FALSE(IsFieldCallStatement("%%"));
	EXPECT_FALSE(IsFieldCallStatement("%%actor.wait(2)%"));
	EXPECT_FALSE(IsFieldCallStatement("%actor.wait(2)"));
	EXPECT_FALSE(IsFieldCallStatement("%actor.wait(2%"));
	EXPECT_FALSE(IsFieldCallStatement(""));
	EXPECT_FALSE(IsFieldCallStatement("   "));
}

}  // namespace dg_field_call_statement_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
