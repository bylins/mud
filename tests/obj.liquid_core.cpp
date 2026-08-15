#include <gtest/gtest.h>

#include "engine/entities/obj_data.h"

namespace {

constexpr ObjVnum kJarVnum = 100700;

// issue.magic-items-hotfix: у сосуда и фонтана val[0..2] переехали в ключи ObjVal
// (kLiquidCapacity / kLiquidCurrent / kLiquidType), а сами val[] перестали быть
// источником правды.
ObjData::shared_ptr MakeDrinkContainer(int capacity, int current) {
	auto prototype = std::make_shared<CObjectPrototype>(kJarVnum);
	auto jar = std::make_shared<ObjData>(*prototype);
	jar->set_type(EObjType::kLiquidContainer);
	jar->set_val(0, capacity);
	jar->set_val(1, current);
	return jar;
}

TEST(ObjLiquidCore, ValuesRoundTripThroughObjVal) {
	const auto jar = MakeDrinkContainer(500, 144);

	EXPECT_EQ(500, GET_OBJ_VAL(jar, 0));
	EXPECT_EQ(144, GET_OBJ_VAL(jar, 1));
}

// issue #3727: ObjVal::set удаляет запись при отрицательном значении, а get_val на
// пропавшем ключе молча откатывается на сырой val[]. Значит уход объема в минус НЕ
// сохраняется, и прочитать оттуда недостачу нельзя -- на этом и погорел перелив,
// который заливал приемник под горлышко и правил недолив отрицательным остатком.
// Любой код, считающий объем жидкости, обязан клампить сам.
TEST(ObjLiquidCore, SubtractingBelowZeroDoesNotStoreTheDeficit) {
	const auto jar = MakeDrinkContainer(500, 144);

	jar->sub_val(1, 500);

	EXPECT_GE(GET_OBJ_VAL(jar, 1), 0);
	EXPECT_NE(GET_OBJ_VAL(jar, 1), -356);
}

// Перелив теперь считает объем через min() и в минус не уходит -- проверяем ту самую
// арифметику: из ведра на 144 глотка в пустую бочку на 500 переливается ровно 144.
TEST(ObjLiquidCore, PourTransfersNoMoreThanTheSourceHolds) {
	const auto bucket = MakeDrinkContainer(144, 144);
	const auto barrel = MakeDrinkContainer(500, 0);

	const int free_space = GET_OBJ_VAL(barrel, 0) - GET_OBJ_VAL(barrel, 1);
	const int amount = std::min(free_space, GET_OBJ_VAL(bucket, 1));
	bucket->sub_val(1, amount);
	barrel->add_val(1, amount);

	EXPECT_EQ(144, amount);
	EXPECT_EQ(144, GET_OBJ_VAL(barrel, 1));
	EXPECT_EQ(0, GET_OBJ_VAL(bucket, 1));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
