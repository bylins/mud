// Дамролл подчинённого в автоатаке -- issue #3958 (разбор в #3670).
//
// Кап от умения оружия (weapon_skill / 2) рассчитан на игрока, который умение качает.
// У подчинённого дамролл приходит из прототипа, подъёма нежити и баффов, поэтому кап
// срезал и его собственное повреждение, и бонус «сил зла»: у костяка велета с кулачным
// боем 80 и дамроллом 50 выходило 40 что без сил зла, что под ними.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "gameplay/fight/fight_hit.h"
#include "gameplay/mechanics/minions.h"

namespace {

// Подчинённый: NPC под чарами. Нежить вдобавок носит флаг kCorpse и хозяина -- по ним
// её отличает IsMortifier, и предел дамролла у неё выше.
CharData::shared_ptr MakeCharmice(int damroll, bool undead, CharData::shared_ptr master)
{
	// Моб собирается как моб: игрока в NPC на ходу не превратить -- у него остаются
	// player_specials, и разрушение такого объекта падает.
	auto mob = std::make_shared<CharData>();
	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;
	mob->set_rnum(-1);   // прототипа в памяти нет
	mob->SetFlag(EMobFlag::kNpc);
	AFF_FLAGS(mob.get()).set(EAffect::kCharmed);
	GET_DR(mob) = damroll;
	if (undead) {
		mob->SetFlag(EMobFlag::kCorpse);
		mob->set_master(master.get());
	}
	return mob;
}

CharData::shared_ptr MakePlayer(int damroll)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto player = builder.get();
	GET_DR(player) = damroll;
	return player;
}

}  // namespace

TEST(Charmice_Damroll, UndeadKeepsItsOwnDamroll)
{
	// Костяк велета: 50 из прототипа при кулачном бое 80. Было 40 -- кап срезал десятку.
	auto master = MakePlayer(0);
	auto undead = MakeCharmice(50, true, master);
	EXPECT_EQ(GetAutoattackDamroll(undead.get(), 80), 50);
}

TEST(Charmice_Damroll, UndeadGetsTheForcesOfEvilBonus)
{
	// Те же 50 плюс 19 от сил зла. Было всё те же 40, то есть бафф не давал ничего.
	auto master = MakePlayer(0);
	auto undead = MakeCharmice(50 + 19, true, master);
	EXPECT_EQ(GetAutoattackDamroll(undead.get(), 80), 69);
}

TEST(Charmice_Damroll, UndeadIsStillCappedAtHundred)
{
	// Предел для нежити -- тот же, что в GetRealDamroll.
	auto master = MakePlayer(0);
	auto undead = MakeCharmice(250, true, master);
	EXPECT_EQ(GetAutoattackDamroll(undead.get(), 80), 100);
}

TEST(Charmice_Damroll, OrdinaryCharmiceIsCappedAtFifty)
{
	// Обычный подчинённый -- не нежить, предел ниже.
	auto charmice = MakeCharmice(250, false, nullptr);
	EXPECT_EQ(GetAutoattackDamroll(charmice.get(), 80), 50);
}

TEST(Charmice_Damroll, PlayerCapStaysOnTheWeaponSkill)
{
	// Для игрока правило не менялось: половина умения оружия.
	auto player = MakePlayer(100);
	EXPECT_EQ(GetAutoattackDamroll(player.get(), 80), 40);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
