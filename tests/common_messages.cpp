// issue #3467: the shutdown countdown and the reboot flavor pool in cfg/messages/ru/common_msg.xml.

#include "engine/core/common_messages.h"
#include "engine/core/config.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace {

void LoadShippedCommonMessages() {
	parser_wrapper::DataNode doc(std::string("cfg/messages/ru/common_msg.xml"));
	CommonMessagesLoader loader;
	loader.Reload(doc);
}

} // namespace

TEST(CommonMessages, NamesRoundTrip) {
	for (const auto msg : {ECommonMsg::kRebootCountdown, ECommonMsg::kShutdownCountdown,
						   ECommonMsg::kRebootFlavor}) {
		EXPECT_EQ(msg, ITEM_BY_NAME<ECommonMsg>(NAME_BY_ITEM<ECommonMsg>(msg)));
	}
}

// Both countdown lines take the time left.
TEST(CommonMessages, ShippedCountdown) {
	LoadShippedCommonMessages();
	for (const auto msg : {ECommonMsg::kRebootCountdown, ECommonMsg::kShutdownCountdown}) {
		EXPECT_NE(CommonMsg(msg).find("{left}"), std::string::npos) << NAME_BY_ITEM<ECommonMsg>(msg);
	}
}

// The flavor is a pool: every line of it comes up, one at a time.
TEST(CommonMessages, ShippedRebootFlavorPool) {
	LoadShippedCommonMessages();
	std::set<std::string> seen;
	for (int i = 0; i < 1000; ++i) {
		const auto &line = CommonMsg(ECommonMsg::kRebootFlavor);
		ASSERT_FALSE(line.empty());
		seen.insert(line);
	}
	EXPECT_EQ(seen.size(), 12u);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
