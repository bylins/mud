// issue.3678-affect-timer: unit coverage for ApplyOfflineTimerAging -- the login-time deduction that
// ages a character's affect and cooldown timers by the offline interval (now - last live-state save).

#include "char.utilities.hpp"
#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_contants.h"

#include <gtest/gtest.h>
#include <ctime>
#include <memory>

namespace {

Affect<EApply>::shared_ptr make_affect(EAffect type, int duration, Bitvector flags) {
	auto a = std::make_shared<Affect<EApply>>();
	a->affect_type = type;
	a->location = EApply::kNone;
	a->modifier = 0;
	a->duration = duration;
	a->battleflag.set_plane(0, flags);
	return a;
}

test_utils::CharacterBuilder::result_t new_char() {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	return builder.get();
}

}  // namespace

// A normal (non-pulsedec) affect ticks once per kSecsPerPlayerAffect out of combat, so the offline
// interval removes elapsed / kSecsPerPlayerAffect ticks.
TEST(OfflineTimer, NormalAffectAgesByElapsed) {
	auto ch = new_char();
	ch->affected.push_front(make_affect(EAffect::kInvisible, 1000, 0));
	const time_t now = time(nullptr);
	ch->set_last_state_save(now - 200);   // 200 seconds offline
	ApplyOfflineTimerAging(ch.get(), now);
	ASSERT_EQ(1u, ch->affected.size());
	EXPECT_EQ(1000 - 200 / kSecsPerPlayerAffect, ch->affected.front()->duration);
}

// Pulsedec affects tick sub-second, so any real offline interval fully decays them.
TEST(OfflineTimer, PulsedecAffectFullyDecays) {
	auto ch = new_char();
	ch->affected.push_front(make_affect(EAffect::kInvisible, 500, kAfPulsedec));
	const time_t now = time(nullptr);
	ch->set_last_state_save(now - 10);
	ApplyOfflineTimerAging(ch.get(), now);
	ASSERT_EQ(1u, ch->affected.size());
	EXPECT_EQ(0, ch->affected.front()->duration);
}

// Permanent affects (duration -1, e.g. materialized equipment buffs) are never aged.
TEST(OfflineTimer, PermanentAffectUntouched) {
	auto ch = new_char();
	ch->affected.push_front(make_affect(EAffect::kInvisible, -1, 0));
	const time_t now = time(nullptr);
	ch->set_last_state_save(now - 100000);
	ApplyOfflineTimerAging(ch.get(), now);
	ASSERT_EQ(1u, ch->affected.size());
	EXPECT_EQ(-1, ch->affected.front()->duration);
}

// A long offline interval clamps the duration to 0 (never negative), so the normal update reaps it.
TEST(OfflineTimer, ExpiredClampsToZeroNotNegative) {
	auto ch = new_char();
	ch->affected.push_front(make_affect(EAffect::kInvisible, 10, 0));
	const time_t now = time(nullptr);
	ch->set_last_state_save(now - 100000);
	ApplyOfflineTimerAging(ch.get(), now);
	ASSERT_EQ(1u, ch->affected.size());
	EXPECT_EQ(0, ch->affected.front()->duration);
}

// No recorded save time (0) means no known offline interval -> pause (unchanged). Covers old files.
TEST(OfflineTimer, NoSaveTimeNoChange) {
	auto ch = new_char();
	ch->affected.push_front(make_affect(EAffect::kInvisible, 1000, 0));
	ch->set_last_state_save(0);
	ApplyOfflineTimerAging(ch.get(), time(nullptr));
	ASSERT_EQ(1u, ch->affected.size());
	EXPECT_EQ(1000, ch->affected.front()->duration);
}

// Clock moved backwards (save time in the future) -> no negative aging.
TEST(OfflineTimer, ClockBackwardsNoChange) {
	auto ch = new_char();
	ch->affected.push_front(make_affect(EAffect::kInvisible, 1000, 0));
	const time_t now = time(nullptr);
	ch->set_last_state_save(now + 500);
	ApplyOfflineTimerAging(ch.get(), now);
	ASSERT_EQ(1u, ch->affected.size());
	EXPECT_EQ(1000, ch->affected.front()->duration);
}

// Cooldowns hold an absolute expiry; the offline interval shifts them back so they age too.
TEST(OfflineTimer, CooldownAgesByElapsed) {
	auto ch = new_char();
	const time_t now = time(nullptr);
	const auto skill = static_cast<ESkill>(1);
	ch->timed_skill[skill] = now + 100;
	ch->set_last_state_save(now - 60);
	ApplyOfflineTimerAging(ch.get(), now);
	EXPECT_EQ(now + 100 - 60, ch->timed_skill[skill]);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
