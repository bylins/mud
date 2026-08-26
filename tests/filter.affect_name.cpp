// Имя аффекта в фильтре предметов ищется вместе с ведущим "!" (issue #3774).
//
// В extra_bits лежат два разных флага -- "рассыпется" и "!рассыпется" (не рассыпается).
// Нестрогий поиск шёл через isname, а тот срезает у запроса ведущие не-буквы, поэтому
// "!рассыпется" искалось как "рассыпется" и находило противоположный флаг: какой из пары
// попадётся, решал порядок в таблице, а не запрос. Пара "невидим" / "!невидим" путалась
// в другую сторону -- там первым в таблице лежит вариант с восклицательным знаком.

#include "engine/ui/objects_filter.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace {

// Разобрать имя аффекта так же, как это делает фильтр "vnum f Аимя", и вернуть
// то, что фильтр считает разобранным (print печатает найденные флаги по именам).
std::string ParseAffect(const char *query) {
	ParseFilter filter(ParseFilter::CLAN);
	char buffer[kMaxInputLength];
	strncpy(buffer, query, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	if (!filter.init_affect(buffer, strlen(buffer))) {
		return "";
	}
	return filter.print();
}

}  // namespace

TEST(ObjectsFilter_AffectName, LeadingBangBelongsToTheName) {
	EXPECT_EQ(ParseAffect("рассыпется"), "Арассыпется");
	EXPECT_EQ(ParseAffect("!рассыпется"), "А!рассыпется");
}

TEST(ObjectsFilter_AffectName, PairWithBangIsNotConfusedInEitherDirection) {
	// Тут в таблице первым лежит "!невидим", то есть путаница шла в обратную сторону.
	EXPECT_EQ(ParseAffect("невидим"), "Аневидим");
	EXPECT_EQ(ParseAffect("!невидим"), "А!невидим");
}

TEST(ObjectsFilter_AffectName, WordSkippingAndAbbreviationsStillWork) {
	// Слова запроса ищутся по порядку с пропуском, каждое -- по префиксу.
	EXPECT_EQ(ParseAffect("защита.от.огня"), "Азащита.от.стихии.огня");
	EXPECT_EQ(ParseAffect("вплавить"), "Аможно вплавить 1 камень");
	EXPECT_EQ(ParseAffect("стойк"), "Астойкость");
}

TEST(ObjectsFilter_AffectName, UnknownNameIsNotParsed) {
	EXPECT_EQ(ParseAffect("такого.флага.нет"), "");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
