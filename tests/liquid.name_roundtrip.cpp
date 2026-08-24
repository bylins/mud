// Название ёмкости: залил жидкость -- вылил -- имя вернулось прежним (issue #3681).
//
// Разделитель " с " между названием ёмкости и названием жидкости срезался жёсткой
// тройкой байт -- столько он занимает в KOI8-R. В UTF-8 буква "с" двухбайтовая,
// разделитель занимает четыре байта, срезалось три, и первый пробел оставался
// в названии: "древний череп  с синим колдовским зельем". При следующей заливке
// к нему снова клеился разделитель, и пробелов становилось два.

#include "gameplay/mechanics/liquid.h"
#include "engine/entities/obj_data.h"
#include "utils/grammar/cases.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace {

constexpr ObjVnum kRoundTripJarVnum = 100702;
constexpr int kRoundTripLiquid = 0;   // первая жидкость из drinknames[]

ObjData::shared_ptr MakeJar(const std::string &name) {
	auto prototype = std::make_shared<CObjectPrototype>(kRoundTripJarVnum);
	auto jar = std::make_shared<ObjData>(*prototype);
	jar->set_type(EObjType::kLiquidContainer);
	jar->set_short_description(name);
	jar->set_aliases(name);
	for (int c = grammar::ECase::kFirstCase; c <= grammar::ECase::kLastCase; ++c) {
		jar->set_PName(static_cast<grammar::ECase>(c), name);
	}
	return jar;
}

}  // namespace

TEST(LiquidNameRoundTrip, FillAndEmptyRestoresTheName) {
	// "древний череп" -- название с кириллицей, на нём баг и был виден.
	const std::string original = "\xD0\xB4\xD1\x80\xD0\xB5\xD0\xB2\xD0\xBD\xD0\xB8\xD0\xB9 "
								 "\xD1\x87\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBF";
	auto jar = MakeJar(original);

	name_to_drinkcon(jar.get(), kRoundTripLiquid);
	ASSERT_NE(jar->get_short_description(), original) << "жидкость должна попасть в название";

	name_from_drinkcon(jar.get());
	EXPECT_EQ(jar->get_short_description(), original)
		<< "после опустошения название обязано вернуться байт в байт, без хвостового пробела";
	EXPECT_EQ(jar->get_PName(grammar::ECase::kNom), original) << "и в падежах тоже";
}

TEST(LiquidNameRoundTrip, RepeatedFillsDoNotAccumulateSpaces) {
	// Именно так лишние пробелы и копились: остаток разделителя оставался в названии,
	// и следующая заливка приклеивала к нему ещё один.
	const std::string original = "\xD1\x81\xD0\xBE\xD1\x81\xD1\x83\xD0\xB4";   // "сосуд"
	auto jar = MakeJar(original);

	for (int i = 0; i < 3; ++i) {
		name_to_drinkcon(jar.get(), kRoundTripLiquid);
		name_from_drinkcon(jar.get());
	}

	EXPECT_EQ(jar->get_short_description(), original);
	EXPECT_EQ(jar->get_short_description().find("  "), std::string::npos)
		<< "двойных пробелов в названии быть не должно";
}

TEST(LiquidNameRoundTrip, StoredExtraSpaceHealsOnReload) {
	// Имена, испорченные прежней обрезкой, уже лежат в файлах вещей: "огромная дубовая бочка"
	// с хвостовым пробелом перед разделителем. Правка разделителя такое имя не чинила -- снимала
	// ровно " с ", получала имя с хвостовым пробелом и приклеивала разделитель обратно, так что
	// два пробела всплывали снова при каждой загрузке. Загрузка вещи гоняет ту же пару
	// name_from_drinkcon + name_to_drinkcon, поэтому чиниться такое имя должно само.
	const std::string original = "огромная дубовая бочка";
	const std::string liquid = drinknames[kRoundTripLiquid];
	auto jar = MakeJar(original + "  с " + liquid);
	jar->set_aliases(original + " " + liquid);

	name_from_drinkcon(jar.get());
	EXPECT_EQ(jar->get_short_description(), original) << "хвостовой пробел обязан уйти вместе с жидкостью";

	name_to_drinkcon(jar.get(), kRoundTripLiquid);
	EXPECT_EQ(jar->get_short_description(), original + " с " + liquid);
	EXPECT_EQ(jar->get_PName(grammar::ECase::kNom), original + " с " + liquid) << "и в падежах тоже";
	EXPECT_EQ(jar->get_aliases().find("  "), std::string::npos) << "в синонимах двойных пробелов тоже быть не должно";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
