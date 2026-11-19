// Appear() -- раскрытие персонажа при действии (подножка, удар, "появиться"). Волшебство надетой
// вещи раскрытием не убивается: его подавляет только бой, а вне боя флаг должен стоять заново по
// живому источнику. Здесь закреплено, кому Appear флаг возвращает, а кому нет: вещевому аффекту
// (kAfFromEquipment) -- да, накастованному или умению -- нет; в бою -- никому, там флаг подавлен
// до конца боя; и явный отказ keep_equipment_stealth=false оставляет чара видимым.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/db.h"

#include <memory>
#include <vector>

namespace appear_equipment_stealth_test {

// Непись живёт не сам по себе: affect_total берёт у прототипа флаги аффектов и add_abils по rnum,
// а комната нужна потому, что из kNowhere пересчёт выходит сразу. Оба массива и мир тест
// подставляет свои и убирает за собой.
class WorldFixture {
	IndexData *saved_index_ = mob_index;
	CharData *saved_proto_ = mob_proto;
	std::size_t saved_world_size_ = world.size();
	std::vector<IndexData> index_;
	std::unique_ptr<CharData[]> proto_;

 public:
	RoomRnum room_rn{0};

	WorldFixture() : index_{IndexData(kProtoVnum)}, proto_(new CharData[1]) {
		proto_[0].SetNpcAttribute(true);
		proto_[0].player_specials = player_special_data::s_for_mobiles;
		mob_index = index_.data();
		mob_proto = proto_.get();

		// kNowhere == 0, то есть нулевая комната -- это "нигде", и пересчёт из неё выходит сразу.
		// В тесте мир пуст, поэтому сперва кладём комнату-пустоту, а работаем во второй.
		while (world.size() < 2) {
			auto *filler = new RoomData();
			filler->vnum = kRoomVnum + static_cast<int>(world.size());
			world.push_back(filler);
		}
		room_rn = static_cast<RoomRnum>(world.size() - 1);
	}

	~WorldFixture() {
		while (world.size() > saved_world_size_) {
			delete world.back();
			world.pop_back();
		}
		mob_index = saved_index_;
		mob_proto = saved_proto_;
	}

	static constexpr int kProtoVnum = 9701;
	static constexpr int kRoomVnum = 970100;
};

CharData::shared_ptr MakeMobInRoom(RoomRnum room) {
	auto mob = std::make_shared<CharData>();
	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;
	mob->set_rnum(0);
	mob->SetFlag(EMobFlag::kNpc);
	mob->in_room = room;
	return mob;
}

// Невидимость одним узлом: вечная (срок -1), как её заводит надевание вещи. from_equipment
// решает, несёт ли узел пометку источника.
void ImposeInvisibility(CharData *ch, bool from_equipment) {
	Affect<EApply> af;
	af.affect_type = EAffect::kInvisible;
	af.location = EApply::kNone;
	af.modifier = 0;
	af.duration = -1;
	if (from_equipment) {
		af.battleflag = {EAffFlag::kAfFromEquipment};
	} else {
		af.battleflag.clear();
	}
	affect_to_char(ch, af);
	AFF_FLAGS(ch).set(EAffect::kInvisible);
}

int CountInvisibilityAffects(CharData *ch) {
	int count = 0;
	for (const auto &af : ch->affected) {
		if (af->affect_type == EAffect::kInvisible) {
			++count;
		}
	}
	return count;
}

TEST(Appear_EquipmentStealth, OutOfCombatItemInvisibilityComesBack) {
	WorldFixture world_fixture;
	auto mob = MakeMobInRoom(world_fixture.room_rn);
	ImposeInvisibility(mob.get(), true);

	sight::Appear(mob.get());

	// Узел цел, и флаг поднят заново тем же пересчётом -- раскрытия не вышло.
	EXPECT_EQ(1, CountInvisibilityAffects(mob.get()));
	EXPECT_TRUE(AFF_FLAGGED(mob.get(), EAffect::kInvisible));
}

TEST(Appear_EquipmentStealth, OutOfCombatCastInvisibilityIsGone) {
	WorldFixture world_fixture;
	auto mob = MakeMobInRoom(world_fixture.room_rn);
	ImposeInvisibility(mob.get(), false);

	sight::Appear(mob.get());

	// Накастованному возвращаться неоткуда: узел снят, возвращать пересчёту нечего.
	EXPECT_EQ(0, CountInvisibilityAffects(mob.get()));
	EXPECT_FALSE(AFF_FLAGGED(mob.get(), EAffect::kInvisible));
}

TEST(Appear_EquipmentStealth, InCombatItemInvisibilityStaysSuppressed) {
	WorldFixture world_fixture;
	auto mob = MakeMobInRoom(world_fixture.room_rn);
	auto enemy = MakeMobInRoom(world_fixture.room_rn);
	ImposeInvisibility(mob.get(), true);
	mob->SetEnemy(enemy.get());

	sight::Appear(mob.get());

	// В бою флаг подавлен до конца боя (его гасит и сам affect_total по GET_ENEMY), а узел цел:
	// вернуть флаг -- дело защёлки kEafInvisible в stop_fighting.
	EXPECT_EQ(1, CountInvisibilityAffects(mob.get()));
	EXPECT_FALSE(AFF_FLAGGED(mob.get(), EAffect::kInvisible));

	mob->SetEnemy(nullptr);
}

TEST(Appear_EquipmentStealth, DeliberateRevealDropsItemInvisibility) {
	WorldFixture world_fixture;
	auto mob = MakeMobInRoom(world_fixture.room_rn);
	ImposeInvisibility(mob.get(), true);

	sight::Appear(mob.get(), false);

	// Так зовёт "появиться": просили раскрыться -- раскрываем, хотя узел вещи и остаётся.
	EXPECT_EQ(1, CountInvisibilityAffects(mob.get()));
	EXPECT_FALSE(AFF_FLAGGED(mob.get(), EAffect::kInvisible));
}

} // namespace appear_equipment_stealth_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
