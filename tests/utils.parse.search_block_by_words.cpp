// issue #4113: поиск по таблице имён, сверяющий имя по словам.
// Имена таблиц движка многословные и записаны через точку ("макс.жизнь"), а запрос приходит
// и через точку, и через пробел, и через подчёркивание. Прежний search_block сверял строку
// посимвольно и спотыкался на первом же несовпавшем разделителе.

#include "utils/utils_parse.h"

#include <gtest/gtest.h>

namespace {

// Хвостовой "\n" -- признак конца таблицы, как у apply_types и прочих списков движка.
const char *kNames[] = {"ничего",
						"макс.жизнь",
						"защита.от.магии.тьмы",
						"бонус.ко.всем.умениям",
						"\n"};

}  // namespace

TEST(Utils_Parse, SearchBlockByWords_ExactName_Found) {
	EXPECT_EQ(1, search_block_by_words("макс.жизнь", kNames));
}

TEST(Utils_Parse, SearchBlockByWords_SeparatorDoesNotMatter) {
	// Ровно то, из-за чего "бонус_ко_всем" не находил своё имя: разделители разные.
	EXPECT_EQ(3, search_block_by_words("бонус_ко_всем", kNames));
	EXPECT_EQ(3, search_block_by_words("бонус ко всем", kNames));
	EXPECT_EQ(3, search_block_by_words("бонус.ко.всем", kNames));
}

TEST(Utils_Parse, SearchBlockByWords_AbbreviatedWords_Found) {
	// Каждое слово запроса -- сокращение своего слова имени, как в триггерах ("макс.жиз").
	EXPECT_EQ(1, search_block_by_words("макс.жиз", kNames));
	EXPECT_EQ(1, search_block_by_words("макс", kNames));
	EXPECT_EQ(2, search_block_by_words("защита.от.маг", kNames));
}

TEST(Utils_Parse, SearchBlockByWords_WordsMustStartFromTheBeginning) {
	// Пропускать слова имени нельзя -- иначе запрос цеплял бы чужое свойство.
	EXPECT_EQ(-1, search_block_by_words("жизнь", kNames));
	EXPECT_EQ(-1, search_block_by_words("умениям", kNames));
}

TEST(Utils_Parse, SearchBlockByWords_UnknownName_ReturnsMinusOne) {
	EXPECT_EQ(-1, search_block_by_words("несуществующее", kNames));
	EXPECT_EQ(-1, search_block_by_words("", kNames));
}

TEST(Utils_Parse, SearchBlockByWords_LongerThanName_NotFound) {
	// Лишнее слово в запросе -- не совпадение.
	EXPECT_EQ(-1, search_block_by_words("макс.жизнь.персонажа", kNames));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
