// issue #2689: selectable spell actions (<action choice="...">) and the prayer/sacrifice spells built
// on them (kOffering, kPrayer).

#include "gameplay/abilities/talents_actions.h"
#include "gameplay/magic/cast_args.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {

using talents_actions::Actions;

const char *kActionsSrc = "action_choice_test.xml";

Actions ParseActions(const std::string &actions_xml) {
	{
		std::ofstream f(kActionsSrc);
		f << "<talent_actions>" << actions_xml << "</talent_actions>";
	}
	parser_wrapper::DataNode doc(kActionsSrc);
	Actions result;
	result.Build(doc);
	std::remove(kActionsSrc);
	return result;
}

const char *kGods =
	R"(<action><affects><affect id="kPietas"/></affects></action>)"
	R"(<action choice="Перуну|Перун"><affects><affect id="kPerunFavor"/></affects></action>)"
	R"(<action choice="Пресвятой Богородице|Богородице"><affects><affect id="kTheotokosGrace"/></affects></action>)"
	R"(<action choice="Велесу|Велес"><affects><affect id="kVelesFavor"/></affects></action>)";

// The spell node with the given id in the shipped spells.xml (the node is left on <spell>).
bool FindShippedSpell(parser_wrapper::DataNode &spells, const char *id) {
	for (auto spell : spells.Children("spell")) {
		if (std::strcmp(spell.GetValue("id"), id) == 0) {
			spells = spell;
			return true;
		}
	}
	return false;
}

} // namespace

TEST(ActionChoice, ParsesChoiceWords) {
	const auto actions = ParseActions(kGods);
	ASSERT_EQ(actions.list().size(), 4u);
	EXPECT_FALSE(actions.list()[0].IsChoice());
	EXPECT_TRUE(actions.list()[0].GetChoiceName().empty());
	ASSERT_EQ(actions.list()[1].GetChoices().size(), 2u);
	EXPECT_EQ(actions.list()[1].GetChoiceName(), "Перуну");
	EXPECT_EQ(actions.list()[1].GetChoices()[1], "Перун");
	EXPECT_EQ(actions.list()[2].GetChoiceName(), "Пресвятой Богородице");
	EXPECT_TRUE(actions.HasChoices());
	EXPECT_EQ(actions.ChoiceNames(), (std::vector<std::string>{"Перуну", "Пресвятой Богородице", "Велесу"}));
}

TEST(ActionChoice, NoChoicesByDefault) {
	const auto actions = ParseActions(R"(<action><affects><affect id="kPietas"/></affects></action>)");
	EXPECT_FALSE(actions.HasChoices());
	EXPECT_TRUE(actions.ChoiceNames().empty());
	EXPECT_EQ(actions.FindChoice("Перуну"), nullptr);
}

TEST(ActionChoice, FindsByWordAbbreviationAndCase) {
	const auto actions = ParseActions(kGods);
	const auto *perun = &actions.list()[1];
	const auto *theotokos = &actions.list()[2];
	EXPECT_EQ(actions.FindChoice("Перуну"), perun);
	EXPECT_EQ(actions.FindChoice("перун"), perun);
	EXPECT_EQ(actions.FindChoice("ПЕР"), perun);
	EXPECT_EQ(actions.FindChoice("богородице"), theotokos);
	EXPECT_EQ(actions.FindChoice("пресвятой богородице"), theotokos);
	EXPECT_EQ(actions.FindChoice("пресв"), theotokos);
}

TEST(ActionChoice, ExactWordBeatsEarlierAbbreviation) {
	// "Вел" is an abbreviation of the first action's word, but the exact word of the second one wins.
	const auto actions = ParseActions(
		R"(<action choice="Великому"><affects><affect id="kPietas"/></affects></action>)"
		R"(<action choice="Вел"><affects><affect id="kPietas"/></affects></action>)");
	EXPECT_EQ(actions.FindChoice("вел"), &actions.list()[1]);
	EXPECT_EQ(actions.FindChoice("вели"), &actions.list()[0]);
}

TEST(ActionChoice, FindsNothing) {
	const auto actions = ParseActions(kGods);
	EXPECT_EQ(actions.FindChoice(""), nullptr);
	EXPECT_EQ(actions.FindChoice("Яриле"), nullptr);
	EXPECT_EQ(actions.FindChoice("Перуну-Громовержцу"), nullptr);
	EXPECT_EQ(actions.FindChoice("уну"), nullptr);   // a substring is not an abbreviation
}

TEST(ActionChoice, FreeFindChoiceSkipsUnconditionalActions) {
	const auto actions = ParseActions(kGods);
	std::vector<const talents_actions::Action *> chain;
	for (const auto &action : actions.list()) {
		chain.push_back(&action);
	}
	EXPECT_EQ(talents_actions::FindChoice(chain, "велес"), &actions.list()[3]);
	EXPECT_EQ(talents_actions::FindChoice({chain[0]}, "велес"), nullptr);
}

// CastArgs: the words after the spell's name; the ones after the target are the spell's own.
TEST(CastArgs, WordsAfterTarget) {
	const CastArgs skeleton{{"труп", "яростный"}, 1};
	EXPECT_EQ(skeleton.AfterTarget(), (std::vector<std::string>{"яростный"}));
	EXPECT_EQ(skeleton.AfterTargetText(), "яростный");

	const CastArgs no_target{{"пресвятой", "богородице"}, 0};
	EXPECT_EQ(no_target.AfterTarget().size(), 2u);
	EXPECT_EQ(no_target.AfterTargetText(), "пресвятой богородице");

	const CastArgs only_target{{"труп"}, 1};
	EXPECT_TRUE(only_target.AfterTarget().empty());
	EXPECT_TRUE(only_target.AfterTargetText().empty());
	EXPECT_TRUE(CastArgs{}.AfterTargetText().empty());
}

// The text a cast's words give selects the action the same way a single name does.
TEST(CastArgs, SelectsChoiceAfterTarget) {
	const auto actions = ParseActions(kGods);
	EXPECT_EQ(actions.FindChoice(CastArgs({{"пресвятой", "богородице"}, 0}).AfterTargetText()), &actions.list()[2]);
	EXPECT_EQ(actions.FindChoice(CastArgs({{"я", "перун"}, 1}).AfterTargetText()), &actions.list()[1]);
	EXPECT_EQ(actions.FindChoice(CastArgs({{"перун"}, 1}).AfterTargetText()), nullptr);
}

// The shipped prayer and sacrifice: the first action, without a choice, lays the marker (kPietas or
// kPrayerful); then four selectable actions, one per god or saint, each with its own favour. Both
// spells are service spells whose strength comes from piety.
TEST(ActionChoice, ShippedOfferingAndPrayer) {
	struct Expected {
		const char *spell;
		const char *marker;
	};
	std::set<std::string> favours;
	std::set<std::string> names;
	for (const auto &expected : {Expected{"kOffering", "kPietas"}, Expected{"kPrayer", "kPrayerful"}}) {
		parser_wrapper::DataNode spell(std::string("cfg/spells.xml"));
		ASSERT_TRUE(FindShippedSpell(spell, expected.spell)) << expected.spell;
		EXPECT_STREQ(spell.GetValue("mode"), "kService") << expected.spell;
		ASSERT_TRUE(spell.GoToChild("potency_roll")) << expected.spell;
		ASSERT_TRUE(spell.GoToChild("base_skill")) << expected.spell;
		EXPECT_STREQ(spell.GetValue("id"), "kReligion") << expected.spell;
		spell.GoToParent();
		spell.GoToParent();
		ASSERT_TRUE(spell.GoToChild("talent_actions")) << expected.spell;
		Actions actions;
		actions.Build(spell);
		ASSERT_EQ(actions.list().size(), 5u) << expected.spell;
		EXPECT_FALSE(actions.list()[0].IsChoice()) << expected.spell;
		EXPECT_EQ(actions.ChoiceNames().size(), 4u) << expected.spell;

		size_t index = 0;
		for (auto action : spell.Children("action")) {
			ASSERT_TRUE(action.GoToChild("affects")) << expected.spell;
			ASSERT_TRUE(action.GoToChild("affect")) << expected.spell;
			const std::string affect_id = action.GetValue("id");
			if (index == 0) {
				EXPECT_EQ(affect_id, expected.marker) << expected.spell;
			} else {
				favours.insert(affect_id);
				const auto &choice = actions.list()[index];
				names.insert(choice.GetChoiceName());
				// Every word of a recipient selects that recipient and no other.
				for (const auto &word : choice.GetChoices()) {
					EXPECT_EQ(actions.FindChoice(word), &choice) << word;
				}
			}
			++index;
		}
	}
	EXPECT_EQ(favours.size(), 8u);
	EXPECT_EQ(names.size(), 8u);
}

// The favours and the markers cannot be dispelled.
TEST(ActionChoice, ShippedFavoursUndispellable) {
	const char *kFavours[] = {"kYariloFavor", "kMakoshFavor", "kPerunFavor", "kVelesFavor",
							  "kTheotokosGrace", "kNicholasGrace", "kElijahGrace", "kGeorgeGrace",
							  "kPrayerful", "kPietas"};
	parser_wrapper::DataNode affects(std::string("cfg/affects/affects.xml"));
	for (const char *favour : kFavours) {
		bool found = false;
		for (auto affect : affects.Children("affect")) {
			if (std::strcmp(affect.GetValue("id"), favour) != 0) {
				continue;
			}
			found = true;
			if (affect.GoToChild("flags")) {
				EXPECT_EQ(std::string(affect.GetValue("val")).find("kAfDispellable"), std::string::npos) << favour;
			}
		}
		EXPECT_TRUE(found) << favour;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
