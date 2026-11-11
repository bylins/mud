// kGreatHelper ("помогает издали") расширяет радиус поиска драки с одной клетки до трёх:
// mobile_activity зовёт FindHelpDirection(ch, 3) вместо FindHelpDirection(ch, 1). Тест строит
// коридор из четырёх клеток, ставит драку в дальнем конце и проверяет, что обычный помощник её
// не видит, а великий -- видит и получает направление первого шага.
//
// Важно: сам флаг kGreatHelper ничего не включает, весь блок помощи входит только при kHelper --
// это проверяется отдельно, через условие, которое стоит в mobile_activity.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "engine/entities/char_data.h"
#include "engine/entities/room_data.h"
#include "engine/db/world_characters.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/alignment.h"
#include "utils/utils.h"

namespace mob_ai {
bool WorthHelping(CharData *helper, CharData *fighter);
bool HoldsRoomForHelp(CharData *helper, RoomRnum room);
bool CanRunToHelp(CharData *ch);
int FindHelpDirection(CharData *helper, int range);
}

namespace great_helper_test {

// Коридор: клетки 1 -- 2 -- 3 -- 4, связаны восток/запад. Драка ставится в 1, помощник в 4,
// то есть ровно три шага, предельная дальность kGreatHelper.
class CorridorFixture {
	std::size_t saved_world_size_ = world.size();

 public:
	static constexpr int kRooms = 5;    // 0 -- пустышка, рабочие клетки 1..4

	CorridorFixture() {
		while (world.size() < static_cast<std::size_t>(kRooms)) {
			auto *room = new RoomData();
			room->vnum = 100 + static_cast<int>(world.size());
			world.push_back(room);
		}
		for (RoomRnum rn = 1; rn + 1 < kRooms; ++rn) {
			Link(rn, rn + 1);
		}
		for (RoomRnum rn = 1; rn < kRooms; ++rn) {
			world[rn]->people.clear();
		}
	}

	~CorridorFixture() {
		for (RoomRnum rn = 1; rn < kRooms && rn < static_cast<RoomRnum>(world.size()); ++rn) {
			world[rn]->people.clear();
		}
		while (world.size() > saved_world_size_) {
			delete world.back();
			world.pop_back();
		}
	}

 private:
	static void Link(RoomRnum from, RoomRnum to) {
		auto east = std::make_shared<ExitData>();
		east->to_room(to);
		world[from]->dir_option[EDirection::kEast] = east;
		auto west = std::make_shared<ExitData>();
		west->to_room(from);
		world[to]->dir_option[EDirection::kWest] = west;
	}
};

CharData::shared_ptr MakeNpc(const char *aliases) {
	auto mob = std::make_shared<CharData>();
	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;
	mob->set_npc_name(aliases);
	mob->SetCharAliases(aliases);
	mob->SetPosition(EPosition::kStand);
	return mob;
}

CharData::shared_ptr MakePlayer(const char *name) {
	auto ch = std::make_shared<CharData>();
	ch->SetNpcAttribute(false);
	ch->set_name(name);
	ch->SetCharAliases(name);
	ch->SetPosition(EPosition::kStand);
	return ch;
}

void PlaceInRoom(const CharData::shared_ptr &ch, RoomRnum rn) {
	ch->in_room = rn;
	world[rn]->people.push_back(ch.get());
}

// Драка в клетке rn: моб бьёт игрока. Помощнику важен именно НПЦ со врагом.
struct Fight {
	CharData::shared_ptr mob;
	CharData::shared_ptr victim;
};

Fight StartFight(RoomRnum rn) {
	Fight f{MakeNpc("агрессор"), MakePlayer("Ратибор")};
	PlaceInRoom(f.mob, rn);
	PlaceInRoom(f.victim, rn);
	f.mob->SetEnemy(f.victim.get());
	return f;
}

TEST(MobActGreatHelper, FightOfAnNpcIsWorthHelping) {
	CorridorFixture corridor;
	auto fight = StartFight(1);
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 1);

	EXPECT_TRUE(mob_ai::WorthHelping(helper.get(), fight.mob.get()));
	// За игрока не вступаются: помощь -- дело НПЦ между собой.
	EXPECT_FALSE(mob_ai::WorthHelping(helper.get(), fight.victim.get()));
	// И сам себе помощник не нужен.
	EXPECT_FALSE(mob_ai::WorthHelping(helper.get(), helper.get()));
}

TEST(MobActGreatHelper, AlignmentDifferenceOverTenBreaksHelp) {
	CorridorFixture corridor;
	auto fight = StartFight(1);
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 1);

	alignment::SetAlignment(fight.mob.get(), 0);
	alignment::SetAlignment(helper.get(), 10);
	EXPECT_TRUE(mob_ai::WorthHelping(helper.get(), fight.mob.get()));

	alignment::SetAlignment(helper.get(), 15);
	EXPECT_FALSE(mob_ai::WorthHelping(helper.get(), fight.mob.get()));
}

TEST(MobActGreatHelper, OrdinaryHelperDoesNotSeeAFightThreeRoomsAway) {
	CorridorFixture corridor;
	auto fight = StartFight(1);
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 4);

	EXPECT_EQ(kBfsError, mob_ai::FindHelpDirection(helper.get(), 1));
}

TEST(MobActGreatHelper, GreatHelperSeesAFightThreeRoomsAway) {
	CorridorFixture corridor;
	auto fight = StartFight(1);
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 4);

	// Первый шаг из клетки 4 к драке в клетке 1 -- на запад.
	EXPECT_EQ(static_cast<int>(EDirection::kWest), mob_ai::FindHelpDirection(helper.get(), 3));
}

TEST(MobActGreatHelper, RangeIsCountedInRooms) {
	CorridorFixture corridor;
	auto fight = StartFight(1);
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 3);    // две клетки до драки

	EXPECT_EQ(kBfsError, mob_ai::FindHelpDirection(helper.get(), 1));
	EXPECT_EQ(static_cast<int>(EDirection::kWest), mob_ai::FindHelpDirection(helper.get(), 2));
	EXPECT_EQ(static_cast<int>(EDirection::kWest), mob_ai::FindHelpDirection(helper.get(), 3));
}

// Условия, при которых моб вообще идёт на помощь. Их же проверяет гейт mobile_activity на
// пометке "иду на помощь", чтобы помеченный моб не остался в быстром режиме навсегда.
TEST(MobActGreatHelper, PreconditionsForRunningToHelp) {
	CorridorFixture corridor;
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 1);
	helper->SetFlag(EMobFlag::kHelper);
	EXPECT_TRUE(mob_ai::CanRunToHelp(helper.get()));

	// Без самого kHelper никакой помощи нет: kGreatHelper только расширяет радиус.
	auto bystander = MakeNpc("прохожий");
	PlaceInRoom(bystander, 1);
	bystander->SetFlag(EMobFlag::kGreatHelper);
	EXPECT_FALSE(mob_ai::CanRunToHelp(bystander.get()));

	// "Не ходит" сильнее "помогает".
	helper->SetFlag(EMobFlag::kSentinel);
	EXPECT_FALSE(mob_ai::CanRunToHelp(helper.get()));
	helper->UnsetFlag(EMobFlag::kSentinel);
	EXPECT_TRUE(mob_ai::CanRunToHelp(helper.get()));

	// Сидящий или спящий остаётся на месте.
	helper->SetPosition(EPosition::kSit);
	EXPECT_FALSE(mob_ai::CanRunToHelp(helper.get()));
	helper->SetPosition(EPosition::kStand);
	EXPECT_TRUE(mob_ai::CanRunToHelp(helper.get()));
}

TEST(MobActGreatHelper, HelperInTheFightRoomHoldsItInstead) {
	CorridorFixture corridor;
	auto fight = StartFight(1);
	auto helper = MakeNpc("ворон");
	PlaceInRoom(helper, 1);

	// Драка в своей клетке: помощнику уходить некуда и незачем.
	EXPECT_TRUE(mob_ai::HoldsRoomForHelp(helper.get(), helper->in_room));
}

}  // namespace great_helper_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
