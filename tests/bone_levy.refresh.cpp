// issue #4032: костяная подать -- нежить чернокнижника слабеет за живую костяную свиту. Подать
// берётся со свиты один раз: сумма долей живых скелетов делится между всей нежитью хозяина.
// Тут закреплён сам пересчёт (RefreshLevy): сколько получается при разном составе свиты и что
// подать снимается, когда скелетов не осталось.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "gameplay/handlers/bone_commander.h"
#include "gameplay/mechanics/animate_dead.h"
#include "gameplay/affects/affect_data.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

namespace bone_levy_refresh_test {

// Внумы берём свои, не мировые: тест ставит собственную лестницу и не зависит от того, что
// сейчас лежит в animate_dead.xml.
constexpr int kUndeadVnum = 9001;
constexpr int kFierceVnum = 9011;
constexpr int kNimbleVnum = 9012;

const char *kCfgSrc = "bone_levy_refresh_src.xml";

// Лестница из одной нежити и двух видов скелетов. Доли те же, что на боевом: 6.3 и 4.9.
void LoadTiers() {
	std::ofstream f(kCfgSrc);
	f << R"(<animate_dead><control skill="kDarkMagic" budget_cap="75" />)"
	     R"(<creature vnum="0" id="kNecrobreather" proto_vnum="9001" weight="25">)"
	     R"(<cost corpse_max_level="127" min_rating="0" /></creature>)"
	     R"(<creature vnum="1" id="kFierceSkeleton" spell="kAnimateSkeleton" damage_share="6.3")"
	     R"( proto_vnum="9011" weight="0"><cost corpse_min_level="26" corpse_max_level="127" /></creature>)"
	     R"(<creature vnum="2" id="kNimbleSkeleton" spell="kAnimateSkeleton" damage_share="4.9")"
	     R"( proto_vnum="9012" weight="0"><cost corpse_min_level="26" corpse_max_level="127" /></creature>)"
	     R"(</animate_dead>)";
	f.close();
	parser_wrapper::DataNode doc(kCfgSrc);
	MUD::AnimateDead().Load(doc);
	std::remove(kCfgSrc);
}

// Мобы живут не сами по себе: GET_MOB_VNUM читает внум из mob_index по rnum, а affect_total у NPC
// берёт прототипные бонусы из mob_proto. Поэтому тест подставляет оба массива и убирает их за собой.
class MobIndexFixture {
	IndexData *saved_index_ = mob_index;
	CharData *saved_proto_ = mob_proto;
	std::vector<IndexData> index_;
	std::unique_ptr<CharData[]> proto_;

 public:
	explicit MobIndexFixture(const std::vector<int> &vnums) : proto_(new CharData[vnums.size()]) {
		for (const int vnum : vnums) {
			index_.emplace_back(vnum);
		}
		for (size_t i = 0; i < vnums.size(); ++i) {
			proto_[i].SetNpcAttribute(true);
			proto_[i].player_specials = player_special_data::s_for_mobiles;
		}
		mob_index = index_.data();
		mob_proto = proto_.get();
	}
	~MobIndexFixture() {
		mob_index = saved_index_;
		mob_proto = saved_proto_;
	}
};

CharData::shared_ptr MakeMob(int rnum, bool bone_servant, CharData *master) {
	auto mob = std::make_shared<CharData>();
	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;
	mob->set_rnum(rnum);
	mob->SetFlag(EMobFlag::kNpc);
	mob->SetFlag(EMobFlag::kCorpse);
	AFF_FLAGS(mob.get()).set(EAffect::kCharmed);
	if (bone_servant) {
		AFF_FLAGS(mob.get()).set(EAffect::kBoneServant);
	}
	mob->set_master(master);
	master->followers.push_back(mob.get());
	return mob;
}

// Подать, которую сейчас несёт нежить (0 -- аффекта нет). Смотрим обе строки: по физическому и
// магическому урону должно стоять одно и то же число.
int LevyOf(CharData *mob) {
	int physic = 0;
	int magic = 0;
	for (const auto &af : mob->affected) {
		if (af->affect_type != EAffect::kBoneLevy) {
			continue;
		}
		if (af->location == EApply::kPhysicDamagePercent) {
			physic = -af->modifier;
		} else if (af->location == EApply::kMagicDamagePercent) {
			magic = -af->modifier;
		}
	}
	EXPECT_EQ(physic, magic);
	return physic;
}

CharData::shared_ptr MakeMaster() {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	return builder.get();
}

TEST(BoneLevy_Refresh, WholeRetinueIsPaidForOnceByOneUndead) {
	LoadTiers();
	MobIndexFixture mobs({kUndeadVnum, kFierceVnum, kNimbleVnum});
	auto master = MakeMaster();
	auto undead = MakeMob(0, false, master.get());
	auto fierce = MakeMob(1, true, master.get());
	auto nimble = MakeMob(2, true, master.get());

	bone_commander::RefreshLevy(master.get());
	EXPECT_EQ(LevyOf(undead.get()), 11);   // 6.3 + 4.9 = 11.2 -> 11
}

TEST(BoneLevy_Refresh, PriceIsSplitBetweenSeveralUndead) {
	LoadTiers();
	MobIndexFixture mobs({kUndeadVnum, kFierceVnum, kNimbleVnum});
	auto master = MakeMaster();
	auto first = MakeMob(0, false, master.get());
	auto second = MakeMob(0, false, master.get());
	auto fierce = MakeMob(1, true, master.get());
	auto nimble = MakeMob(2, true, master.get());

	bone_commander::RefreshLevy(master.get());
	EXPECT_EQ(LevyOf(first.get()), 6);    // 11.2 / 2 = 5.6 -> 6
	EXPECT_EQ(LevyOf(second.get()), 6);
}

TEST(BoneLevy_Refresh, LeavingSkeletonIsNotCounted) {
	LoadTiers();
	MobIndexFixture mobs({kUndeadVnum, kFierceVnum, kNimbleVnum});
	auto master = MakeMaster();
	auto undead = MakeMob(0, false, master.get());
	auto fierce = MakeMob(1, true, master.get());
	auto nimble = MakeMob(2, true, master.get());

	bone_commander::RefreshLevy(master.get());
	ASSERT_EQ(LevyOf(undead.get()), 11);

	// Выбывающий ещё числится в последователях, поэтому его передают отдельно.
	bone_commander::RefreshLevy(master.get(), nimble.get());
	EXPECT_EQ(LevyOf(undead.get()), 6);   // остался только свирепый: 6.3 -> 6
}

TEST(BoneLevy_Refresh, LastSkeletonGoneRemovesTheLevy) {
	LoadTiers();
	MobIndexFixture mobs({kUndeadVnum, kFierceVnum});
	auto master = MakeMaster();
	auto undead = MakeMob(0, false, master.get());
	auto fierce = MakeMob(1, true, master.get());

	bone_commander::RefreshLevy(master.get());
	ASSERT_EQ(LevyOf(undead.get()), 6);

	master->followers.remove(fierce.get());
	bone_commander::RefreshLevy(master.get());
	EXPECT_EQ(LevyOf(undead.get()), 0);
}

TEST(BoneLevy_Refresh, RetinueWithoutUndeadCostsNothing) {
	LoadTiers();
	MobIndexFixture mobs({kFierceVnum, kNimbleVnum});
	auto master = MakeMaster();
	auto fierce = MakeMob(0, true, master.get());
	auto nimble = MakeMob(1, true, master.get());

	// Платить некому -- и на самих скелетах подати быть не должно.
	bone_commander::RefreshLevy(master.get());
	EXPECT_EQ(LevyOf(fierce.get()), 0);
	EXPECT_EQ(LevyOf(nimble.get()), 0);
}

}   // namespace bone_levy_refresh_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
