// issue #2689: those a character prays or sacrifices to (cfg/mechanics/religion.xml), and the shipped
// prayer/sacrifice spells and favours they point to.

#include "gameplay/mechanics/religion.h"
#include "engine/entities/entities_constants.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

namespace {

using religion::Recipients;

const char *kRecipientsSrc = "religion_test.xml";

Recipients ParseRecipients(const std::string &recipients_xml) {
	{
		std::ofstream f(kRecipientsSrc);
		f << "<religion>" << recipients_xml << "</religion>";
	}
	parser_wrapper::DataNode doc(kRecipientsSrc);
	Recipients result;
	result.Build(doc);
	std::remove(kRecipientsSrc);
	return result;
}

const char *kTwoRecipients =
	R"(<recipient name="Перуну" religion="kPoly" spell="kSacrificePerun"/>)"
	R"(<recipient name="Николе-Угоднику" religion="kMono" spell="kPrayerNicholas"/>)";

} // namespace

TEST(ReligionRecipients, ParsesValidRecipients) {
	const auto recipients = ParseRecipients(kTwoRecipients);
	ASSERT_EQ(recipients.All().size(), 2u);
	EXPECT_EQ(recipients.All()[0].name, "Перуну");
	EXPECT_EQ(recipients.All()[0].religion, kReligionPoly);
	EXPECT_EQ(recipients.All()[0].spell, ESpell::kSacrificePerun);
	EXPECT_EQ(recipients.All()[1].religion, kReligionMono);
	EXPECT_EQ(recipients.All()[1].spell, ESpell::kPrayerNicholas);
}

TEST(ReligionRecipients, DropsInvalidRecipients) {
	const auto recipients = ParseRecipients(
		R"(<recipient religion="kPoly" spell="kSacrificePerun"/>)"
		R"(<recipient name="Перуну" religion="kAtheist" spell="kSacrificePerun"/>)"
		R"(<recipient name="Перуну" religion="kPoly" spell="kNoSuchSpell"/>)"
		R"(<recipient name="Перуну" religion="kPoly"/>)"
		R"(<recipient name="Велесу" religion="kPoly" spell="kSacrificeVeles"/>)");
	ASSERT_EQ(recipients.All().size(), 1u);
	EXPECT_EQ(recipients.All()[0].name, "Велесу");
}

TEST(ReligionRecipients, DropsDuplicateName) {
	const auto recipients = ParseRecipients(
		R"(<recipient name="Перуну" religion="kPoly" spell="kSacrificePerun"/>)"
		R"(<recipient name="Перуну" religion="kPoly" spell="kSacrificeVeles"/>)");
	ASSERT_EQ(recipients.All().size(), 1u);
	EXPECT_EQ(recipients.All()[0].spell, ESpell::kSacrificePerun);
}

TEST(ReligionRecipients, FindsByAbbreviationIgnoringCase) {
	const auto recipients = ParseRecipients(kTwoRecipients);
	ASSERT_NE(recipients.Find("перуну"), nullptr);
	EXPECT_EQ(recipients.Find("пер")->spell, ESpell::kSacrificePerun);
	EXPECT_EQ(recipients.Find("НИК")->spell, ESpell::kPrayerNicholas);
	EXPECT_EQ(recipients.Find("Николе-Угоднику")->spell, ESpell::kPrayerNicholas);
}

TEST(ReligionRecipients, FindsNothing) {
	const auto recipients = ParseRecipients(kTwoRecipients);
	EXPECT_EQ(recipients.Find(""), nullptr);
	EXPECT_EQ(recipients.Find("Велесу"), nullptr);
	EXPECT_EQ(recipients.Find("Перуну-Громовержцу"), nullptr);
}

// The shipped religion.xml: four gods for heathens, four saints for Christians, each with its own spell.
TEST(ReligionRecipients, ShippedRecipients) {
	parser_wrapper::DataNode doc(std::string("cfg/mechanics/religion.xml"));
	Recipients recipients;
	recipients.Build(doc);
	ASSERT_EQ(recipients.All().size(), 8u);
	std::map<int, int> per_religion;
	std::map<ESpell, int> per_spell;
	for (const auto &recipient : recipients.All()) {
		++per_religion[recipient.religion];
		++per_spell[recipient.spell];
		EXPECT_EQ(recipient.name.find(' '), std::string::npos) << recipient.name;
	}
	EXPECT_EQ(per_religion[kReligionPoly], 4);
	EXPECT_EQ(per_religion[kReligionMono], 4);
	EXPECT_EQ(per_spell.size(), 8u);
}

// Every shipped recipient's spell is a service spell that applies the right marker (sacrifice --
// kPietas, prayer -- kPrayerful) and takes its strength from piety.
TEST(ReligionRecipients, ShippedSpells) {
	parser_wrapper::DataNode recipients_doc(std::string("cfg/mechanics/religion.xml"));
	Recipients recipients;
	recipients.Build(recipients_doc);

	for (const auto &recipient : recipients.All()) {
		const std::string spell_id = NAME_BY_ITEM<ESpell>(recipient.spell);
		parser_wrapper::DataNode spells(std::string("cfg/spells.xml"));
		bool found = false;
		for (auto spell : spells.Children("spell")) {
			if (spell_id != spell.GetValue("id")) {
				continue;
			}
			found = true;
			EXPECT_STREQ(spell.GetValue("mode"), "kService") << spell_id;
			ASSERT_TRUE(spell.GoToChild("potency_roll")) << spell_id;
			ASSERT_TRUE(spell.GoToChild("base_skill")) << spell_id;
			EXPECT_STREQ(spell.GetValue("id"), "kReligion") << spell_id;
			spell.GoToParent();
			spell.GoToParent();
			ASSERT_TRUE(spell.GoToChild("talent_actions")) << spell_id;
			ASSERT_TRUE(spell.GoToChild("action")) << spell_id;
			ASSERT_TRUE(spell.GoToChild("affects")) << spell_id;
			const char *marker = (recipient.religion == kReligionPoly) ? "kPietas" : "kPrayerful";
			bool has_marker = false;
			for (auto affect : spell.Children("affect")) {
				if (std::strcmp(affect.GetValue("id"), marker) == 0) {
					has_marker = true;
				}
			}
			EXPECT_TRUE(has_marker) << spell_id;
		}
		EXPECT_TRUE(found) << spell_id;
	}
}

// The favours cannot be dispelled.
TEST(ReligionRecipients, ShippedFavoursUndispellable) {
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
