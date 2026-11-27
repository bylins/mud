// issue #2648: a fighter whose enemy has left the room turns to someone there who is still fighting it,
// instead of the whole fight falling apart; with nobody left it stops fighting as before.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "engine/db/db.h"
#include "gameplay/fight/fight.h"

#include <list>
#include <memory>
#include <vector>

extern std::list<combat_list_element> combat_list;

namespace fight_switch_on_leave_test {

// Two rooms besides kNowhere (room 0) and a mob prototype for whatever looks a mob up by rnum.
class WorldFixture {
	IndexData *saved_index_ = mob_index;
	CharData *saved_proto_ = mob_proto;
	std::size_t saved_world_size_ = world.size();
	std::vector<IndexData> index_;
	std::unique_ptr<CharData[]> proto_;

 public:
	RoomRnum here{0};
	RoomRnum elsewhere{0};

	WorldFixture() : index_{IndexData(kProtoVnum)}, proto_(new CharData[1]) {
		proto_[0].SetNpcAttribute(true);
		proto_[0].player_specials = player_special_data::s_for_mobiles;
		mob_index = index_.data();
		mob_proto = proto_.get();
		while (world.size() < saved_world_size_ + 3 || world.size() < 3) {
			auto *room = new RoomData();
			room->vnum = kRoomVnum + static_cast<int>(world.size());
			world.push_back(room);
		}
		here = static_cast<RoomRnum>(world.size() - 2);
		elsewhere = static_cast<RoomRnum>(world.size() - 1);
	}

	~WorldFixture() {
		combat_list.clear();
		while (world.size() > saved_world_size_) {
			delete world.back();
			world.pop_back();
		}
		mob_index = saved_index_;
		mob_proto = saved_proto_;
	}

	// A mob standing in the room. The wait keeps check_mob_helpers from calling for help.
	CharData::shared_ptr MakeMob(RoomRnum room) {
		auto mob = std::make_shared<CharData>();
		mob->SetNpcAttribute(true);
		mob->player_specials = player_special_data::s_for_mobiles;
		mob->set_rnum(0);
		mob->SetFlag(EMobFlag::kNpc);
		mob->SetPosition(EPosition::kStand);
		mob->set_wait(10u);
		Place(mob.get(), room);
		return mob;
	}

	static void Place(CharData *ch, RoomRnum room) {
		if (ch->in_room != kNowhere) {
			world[ch->in_room]->people.remove(ch);
		}
		ch->in_room = room;
		world[room]->people.push_back(ch);
	}

	static constexpr int kProtoVnum = 9702;
	static constexpr int kRoomVnum = 970200;
};

// The fighter switched to the woodpecker; the woodpecker flies away; the wolf is still biting him.
TEST(Fight_SwitchOnLeave, TurnsToTheRemainingAttacker) {
	WorldFixture fixture;
	auto fighter = fixture.MakeMob(fixture.here);
	auto wolf = fixture.MakeMob(fixture.here);
	auto woodpecker = fixture.MakeMob(fixture.here);
	SetFighting(wolf.get(), fighter.get());
	SetFighting(fighter.get(), woodpecker.get());
	SetFighting(woodpecker.get(), fighter.get());
	stop_fighting(woodpecker.get(), false);
	WorldFixture::Place(woodpecker.get(), fixture.elsewhere);

	check_mob_helpers();

	EXPECT_EQ(fighter->GetEnemy(), wolf.get());
	EXPECT_EQ(wolf->GetEnemy(), fighter.get());
}

// Nobody else is fighting him: the fight stops as before.
TEST(Fight_SwitchOnLeave, StopsWithNobodyLeft) {
	WorldFixture fixture;
	auto fighter = fixture.MakeMob(fixture.here);
	auto bystander = fixture.MakeMob(fixture.here);
	auto woodpecker = fixture.MakeMob(fixture.here);
	SetFighting(fighter.get(), woodpecker.get());
	WorldFixture::Place(woodpecker.get(), fixture.elsewhere);

	check_mob_helpers();

	EXPECT_EQ(fighter->GetEnemy(), nullptr);
	EXPECT_EQ(bystander->GetEnemy(), nullptr);
}

// An attacker who is in another room does not count.
TEST(Fight_SwitchOnLeave, IgnoresAttackersElsewhere) {
	WorldFixture fixture;
	auto fighter = fixture.MakeMob(fixture.here);
	auto far_attacker = fixture.MakeMob(fixture.elsewhere);
	auto woodpecker = fixture.MakeMob(fixture.here);
	far_attacker->SetEnemy(fighter.get());
	SetFighting(fighter.get(), woodpecker.get());
	WorldFixture::Place(woodpecker.get(), fixture.elsewhere);

	check_mob_helpers();

	EXPECT_EQ(fighter->GetEnemy(), nullptr);
	far_attacker->SetEnemy(nullptr);
}

} // namespace fight_switch_on_leave_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
