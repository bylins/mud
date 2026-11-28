// Кап сопротивления берётся по тому, кто сопротивляется, -- issue #3644.
//
// Бросок MR (заклинание, дыхание, щиты от MR) резал MR жертвы капом атакующего: у игрока-
// кастера это kMaxPcResist = 75, и моб с MR 100, которого автор зоны задумал иммунным к магии,
// всё равно получал четверть ударов. Кап принадлежит жертве: игрок -- 75, моб -- 100.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "gameplay/mechanics/resist.h"
#include "gameplay/affects/affect_data.h"
#include "engine/entities/room_data.h"

#include <vector>

namespace {

CharData::shared_ptr MakeMob()
{
	auto mob = std::make_shared<CharData>();
	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;
	mob->set_rnum(-1);   // прототипа в памяти нет
	mob->SetFlag(EMobFlag::kNpc);
	return mob;
}

CharData::shared_ptr MakePlayer()
{
	test_utils::CharacterBuilder builder;
	builder.create_new();
	return builder.get();
}

TEST(ResistCap, MobKeepsFullRange)
{
	const auto mob = MakeMob();
	EXPECT_EQ(CapResist(mob.get(), 100), 100);   // иммунитет
	EXPECT_EQ(CapResist(mob.get(), 95), 95);
	EXPECT_EQ(CapResist(mob.get(), 150), kMaxNpcResist);
}

// Пересчёт выходит сразу, если персонаж в kNowhere (это нулевая комната), поэтому тест кладёт
// комнату-пустоту и работает во второй, а за собой мир убирает.
class RoomFixture {
	std::size_t saved_world_size_ = world.size();

 public:
	RoomRnum room_rn{0};

	RoomFixture() {
		while (world.size() < 2) {
			auto *room = new RoomData();
			room->vnum = kRoomVnum + static_cast<int>(world.size());
			world.push_back(room);
		}
		room_rn = static_cast<RoomRnum>(world.size() - 1);
	}

	~RoomFixture() {
		while (world.size() > saved_world_size_) {
			delete world.back();
			world.pop_back();
		}
	}

	static constexpr int kRoomVnum = 970200;
};

// AR капается при пересчёте, как MR и PR: игрок, набравший AR 100 с вещей и аффектов, не
// становится неуязвим для дебаффов.
TEST(ResistCap, PlayerAffectResistCappedOnRecalc)
{
	RoomFixture rooms;
	const auto player = MakePlayer();
	player->in_room = rooms.room_rn;

	Affect<EApply> af;
	af.affect_type = EAffect::kUndefined;
	af.location = EApply::kAffectResist;
	af.modifier = 100;
	af.duration = -1;
	affect_to_char(player.get(), af);   // пересчитывает сам

	EXPECT_EQ(GET_AR(player), kMaxPcResist);
	player->in_room = kNowhere;
}

TEST(ResistCap, PlayerCappedAtPcMaximum)
{
	const auto player = MakePlayer();
	EXPECT_EQ(CapResist(player.get(), 100), kMaxPcResist);
	EXPECT_EQ(CapResist(player.get(), 50), 50);
}

} // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
