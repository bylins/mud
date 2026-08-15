// issue.playerdata-migration: account persistence lands under the new
// userdata/accounts/ location and round-trips the fields read_from_file()
// actually parses (daily quests + currencies). The account file is named by
// email; save writes LIB_ACCOUNTS + email, load reads the same path.
//
// NOTE: the account system is knowingly incomplete (password / players_list /
// login history are written but not read back yet -- to be reworked later), so
// this test only asserts what the current read path supports.

#include "administration/accounts.h"
#include "gameplay/economics/currency_storage.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {
// An email nothing else uses; the file is created and removed by the test.
const std::string kEmail = "test-account@bylins.invalid";
const std::string kPath = std::string("userdata/accounts/") + kEmail;
}  // namespace

TEST(AccountsStorage, RoundTripsToUserdataAccounts) {
	// The engine creates this at boot; tests run without that step, so make it.
	std::filesystem::create_directories("userdata/accounts");
	std::filesystem::remove(kPath);

	// A fresh account has nothing persisted yet.
	{
		Account acc(kEmail);
		EXPECT_TRUE(acc.quest_is_available(7));    // never completed -> available
		acc.complete_quest(7);                     // just completed now
		EXPECT_FALSE(acc.quest_is_available(7));   // inside the cooldown window
		acc.currency_storage().SetHand("gold", 123);
		acc.currency_storage().SetBank("gold", 456);
		acc.save_to_file();
	}

	// The save must land at the NEW userdata/accounts/ path (the migration point).
	EXPECT_TRUE(std::filesystem::exists(kPath));

	// A reload parses DaiQ + Cur back out of the file.
	{
		Account reloaded(kEmail);
		EXPECT_FALSE(reloaded.quest_is_available(7));                 // dquest survived
		EXPECT_EQ(reloaded.currency_storage().GetHand("gold"), 123);  // currency survived
		EXPECT_EQ(reloaded.currency_storage().GetBank("gold"), 456);
	}

	std::filesystem::remove(kPath);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
