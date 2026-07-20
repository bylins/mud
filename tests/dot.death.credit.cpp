// issue.dot-death-exp: regression tests for crediting a self-inflicted DoT/poison kill.
//
// When a damage-over-time affect ticks, the bearer damages ITSELF (target=kTarFightSelf), so the
// death is attributed via the Damage's author_uid -- the affect's stored caster_id (a UID, not a
// live pointer). SelectSelfInflictedKiller is the attribution ProcessDeath uses; it decides who (if
// anyone) gets the experience. The bug was that the general <damage> DoT path never fed author_uid,
// so kBurning-style DoTs credited nobody. These tests lock the attribution contract, including the
// case the user flagged: a DoT can still tick after its author has died.

#include "gameplay/mechanics/damage.h"
#include "gameplay/affects/affect_data.h"

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

Affect<EApply>::shared_ptr make_poison(long caster_id, EAffect type = EAffect::kPoisoned) {
	auto a = std::make_shared<Affect<EApply>>();
	a->affect_type = type;
	a->caster_id = caster_id;
	return a;
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

// issue #3610: несколько узлов одного аффекта -- тик достается наложенному ПОСЛЕДНИМ (список
// пополняется через push_front). Если у него есть автор, он и получает урон в зачет.
TEST(DotAuthorSelect, TickingNodeAuthorWins) {
	auto ticking = make_poison(202);
	std::list<Affect<EApply>::shared_ptr> affects{ticking, make_poison(303)};

	EXPECT_EQ(202, SelectAffectAuthorUid(affects, ticking));
}

// Тот самый баг: поверх яда отравителя лег безавторский узел того же типа (крит-яд с пушки,
// отравленная еда, dg-триггер, врожденный флаг моба) -- тик достался ему, и опыт терялся.
// Теперь автор берется у соседнего узла того же типа.
TEST(DotAuthorSelect, AuthorlessNodeFallsBackToSibling) {
	auto ticking = make_poison(0);
	std::list<Affect<EApply>::shared_ptr> affects{ticking, make_poison(202)};

	EXPECT_EQ(202, SelectAffectAuthorUid(affects, ticking));
}

// Узел другого типа автора не одалживает: горение не должно засчитываться отравителю.
TEST(DotAuthorSelect, OtherAffectTypeIsNotBorrowed) {
	auto ticking = make_poison(0, EAffect::kPoisoned);
	std::list<Affect<EApply>::shared_ptr> affects{ticking, make_poison(202, EAffect::kBurning)};

	EXPECT_EQ(0, SelectAffectAuthorUid(affects, ticking));
}

// Автора нет нигде (съел отраву сам) -- засчитывать некому, и это нормально.
TEST(DotAuthorSelect, NoAuthorAnywhere) {
	auto ticking = make_poison(0);
	std::list<Affect<EApply>::shared_ptr> affects{ticking, make_poison(0)};

	EXPECT_EQ(0, SelectAffectAuthorUid(affects, ticking));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
