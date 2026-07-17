// issue.dot-death-exp: regression tests for crediting a self-inflicted DoT/poison kill.
//
// When a damage-over-time affect ticks, the bearer damages ITSELF (target=kTarFightSelf), so the
// death is attributed via the Damage's author_uid -- the affect's stored caster_id (a UID, not a
// live pointer). SelectSelfInflictedKiller is the attribution ProcessDeath uses; it decides who (if
// anyone) gets the experience. The bug was that the general <damage> DoT path never fed author_uid,
// so kBurning-style DoTs credited nobody. These tests lock the attribution contract, including the
// case the user flagged: a DoT can still tick after its author has died.

#include "gameplay/mechanics/damage.h"

#include "char.utilities.hpp"   // test_utils::CharacterBuilder

#include <gtest/gtest.h>

#include <list>

namespace {

CharData *make_char(test_utils::CharacterBuilder &b, int uid) {
	b.create_new();
	CharData *c = b.get().get();
	c->set_uid(uid);
	return c;
}

}  // namespace

// A self-inflicted DoT kill credits the author when they are present in the room -- looked up by the
// affect's stored uid, exactly like a weapon/spell kill credits the attacker.
TEST(DotDeathCredit, AuthorInRoomIsCredited) {
	test_utils::CharacterBuilder vb, ab, bystb;
	CharData *victim = make_char(vb, 101);
	CharData *author = make_char(ab, 202);
	CharData *bystander = make_char(bystb, 303);

	std::list<CharData *> people{victim, bystander, author};

	EXPECT_EQ(author, SelectSelfInflictedKiller(people, victim, /*author_uid*/ 202,
												fight::EDamageSource::kUndefined));
}

// The user's constraint: a DoT can still deal damage after the character that applied it has died.
// The author is then no longer in the room, so no live char matches the stored uid -- nobody is
// credited, and (crucially) no stale pointer is dereferenced.
TEST(DotDeathCredit, DeadAuthorCreditsNobody) {
	test_utils::CharacterBuilder vb, bystb;
	CharData *victim = make_char(vb, 101);
	CharData *bystander = make_char(bystb, 303);

	std::list<CharData *> people{victim, bystander};   // author (uid 202) already extracted

	EXPECT_EQ(nullptr, SelectSelfInflictedKiller(people, victim, /*author_uid*/ 202,
												 fight::EDamageSource::kUndefined));
}

// No author (a self-poison with caster_id 0, e.g. eating poisoned food): the victim is blamed for
// their own death -- credit nobody.
TEST(DotDeathCredit, NoAuthorCreditsNobody) {
	test_utils::CharacterBuilder vb;
	CharData *victim = make_char(vb, 101);

	std::list<CharData *> people{victim};

	EXPECT_EQ(nullptr, SelectSelfInflictedKiller(people, victim, /*author_uid*/ 0,
												 fight::EDamageSource::kUndefined));
}

// A stale author_uid equal to the victim's OWN uid must not credit the victim for killing itself.
TEST(DotDeathCredit, SelfAuthorCreditsNobody) {
	test_utils::CharacterBuilder vb;
	CharData *victim = make_char(vb, 101);

	std::list<CharData *> people{victim};

	EXPECT_EQ(nullptr, SelectSelfInflictedKiller(people, victim, /*author_uid*/ 101,
												 fight::EDamageSource::kUndefined));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
