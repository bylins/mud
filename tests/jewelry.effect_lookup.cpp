// Выбор вплавляемого эффекта по названию (JewelryCfg::Find) -- то, чем "вплавить <камень>
// <вещь> <эффект>" превращает написанное игроком имя в эффект.
//
// Названия эффектов многословные, а одно из них ("бонус физических повреждений, %") содержит
// запятую и процент. Прежнее сравнение требовало совпадения целиком, и такое название набрать
// было нельзя вовсе: команда молча перепечатывала список (issue #4138). Звездочка в начале
// имени -- пометка таблиц для билдеров, в ключ она попадать не должна.
//
// Конфиг собран руками, а не прочитан из XML: загрузчик проверяет внум камня по прототипам
// мира, которого у тестов нет.

#include "gameplay/crafting/jewelry.h"
#include "gameplay/affects/affect_contants.h"

#include <gtest/gtest.h>

namespace {

constexpr int kJewelryGem = 907;    // белый камень
constexpr int kJewelryNoGem = 906;  // камня нет в конфиге

void SetUpJewelryGem() {
	jewelry::cfg.gems.clear();
	auto add = [](EApply apply, const char *name) {
		jewelry::GemEffect eff;
		eff.type = 1;
		eff.id = static_cast<int>(apply);
		eff.val = 5;
		eff.key = jewelry::EffectKey(name);
		jewelry::cfg.gems[kJewelryGem].push_back(eff);
	};
	// ровно те названия, что игрок видит в списке камня
	add(EApply::kCastSuccess, "успех колдовства");
	add(EApply::kManaRegen, "запоминание");
	add(EApply::kPhysicDamagePercent, "бонус физических повреждений, %");
	add(EApply::kExpPercent, "*дополнительный бонус опыта");
}

int ResolveGemEffect(const char *name) {
	const auto *eff = jewelry::cfg.Find(kJewelryGem, name);
	return eff ? eff->id : -1;
}

}  // namespace

TEST(JewelryEffectLookup, BuilderMarkIsNotPartOfTheName) {
	EXPECT_EQ(jewelry::EffectKey("*дополнительный бонус опыта"), "дополнительный бонус опыта");
	EXPECT_EQ(jewelry::EffectKey("успех колдовства"), "успех колдовства");
	EXPECT_EQ(jewelry::EffectKey(""), "");
}

TEST(JewelryEffectLookup, FullNameWithPunctuation) {
	SetUpJewelryGem();
	EXPECT_EQ(ResolveGemEffect("бонус физических повреждений, %"),
			  static_cast<int>(EApply::kPhysicDamagePercent));
}

TEST(JewelryEffectLookup, DotsInsteadOfSpaces) {
	SetUpJewelryGem();
	// то, что набирал игрок в issue #4138: запятую и процент в аргумент не поставить
	EXPECT_EQ(ResolveGemEffect("бонус.физических.повреждений"),
			  static_cast<int>(EApply::kPhysicDamagePercent));
	EXPECT_EQ(ResolveGemEffect("успех.колдовства"), static_cast<int>(EApply::kCastSuccess));
	EXPECT_EQ(ResolveGemEffect("доп.бонус.опыта"), static_cast<int>(EApply::kExpPercent));
}

TEST(JewelryEffectLookup, AbbreviatedWords) {
	SetUpJewelryGem();
	EXPECT_EQ(ResolveGemEffect("бон.физ.повр"), static_cast<int>(EApply::kPhysicDamagePercent));
	EXPECT_EQ(ResolveGemEffect("усп.колд"), static_cast<int>(EApply::kCastSuccess));
	EXPECT_EQ(ResolveGemEffect("запом"), static_cast<int>(EApply::kManaRegen));
}

TEST(JewelryEffectLookup, UnderscoresAndCaseIgnored) {
	SetUpJewelryGem();
	EXPECT_EQ(ResolveGemEffect("Бонус_Физических_Повреждений"),
			  static_cast<int>(EApply::kPhysicDamagePercent));
}

TEST(JewelryEffectLookup, NoSilentMatchOnEmptyOrForeignName) {
	SetUpJewelryGem();
	EXPECT_EQ(ResolveGemEffect(""), -1);
	EXPECT_EQ(ResolveGemEffect("сила.заклинаний"), -1);   // этого эффекта у камня нет
	EXPECT_EQ(jewelry::cfg.Find(kJewelryNoGem, "успех.колдовства"), nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
