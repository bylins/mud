// Цена, по которой магазин отдаёт вещь покупателю -- issue #3953.
//
// На одном узле магазина (один vnum) лежат экземпляры с разной стоимостью: свежёванные шкуры
// с одного и того же моба стоят от 90 до 540. Скупка платила по экземпляру, а продажа брала
// цену узла, и шкура за 405 уходила обратно за 90 -- деньги из воздуха.

#include <gtest/gtest.h>

#include "gameplay/economics/shops_implementation.h"

namespace {

constexpr ObjVnum kSkinVnum = 1669;
constexpr long kNodePrice = 90;

auto MakeItem(const long cost)
{
	auto item = std::make_shared<CObjectPrototype>(kSkinVnum);
	item->set_cost(cost);
	return item;
}

}  // namespace

TEST(Shop_SalePrice, EmptyNodeKeepsItsOwnPrice)
{
	// Товар со склада прототипов: своих экземпляров нет, цена узла из shops.xml и нужна.
	const ShopExt::ItemNode node(kSkinVnum, kNodePrice);
	EXPECT_EQ(ShopExt::CalcSalePrice(node, nullptr), kNodePrice);

	const auto item = MakeItem(405);
	EXPECT_EQ(ShopExt::CalcSalePrice(node, item.get()), kNodePrice);
}

TEST(Shop_SalePrice, ShelfItemSetsThePrice)
{
	// Сданная игроком вещь лежит на полке -- цена её собственная, а не узла.
	const ShopExt::ItemNode node(kSkinVnum, kNodePrice, 42);
	const auto expensive = MakeItem(405);
	EXPECT_EQ(ShopExt::CalcSalePrice(node, expensive.get()), 405);

	const auto cheap = MakeItem(90);
	EXPECT_EQ(ShopExt::CalcSalePrice(node, cheap.get()), 90);
}

TEST(Shop_SalePrice, MissingShelfItemFallsBackToNodePrice)
{
	// Узел не пуст, а вещь по uid не нашлась: продавать бесплатно нельзя.
	const ShopExt::ItemNode node(kSkinVnum, kNodePrice, 42);
	EXPECT_EQ(ShopExt::CalcSalePrice(node, nullptr), kNodePrice);
}

TEST(Shop_SalePrice, CheaperShelfItemIsSoldCheaper)
{
	// Обратный случай: узел завёлся с дорогой вещи, а лежит дешёвая. Покупатель платит
	// за то, что получает, иначе дешёвые вещи уходили по цене первой сданной.
	const ShopExt::ItemNode node(kSkinVnum, 405, 42);
	const auto cheap = MakeItem(90);
	EXPECT_EQ(ShopExt::CalcSalePrice(node, cheap.get()), 90);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
