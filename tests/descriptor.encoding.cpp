#include "engine/network/descriptor_data.h"

#include <gtest/gtest.h>

// Regression test for crash when string_to_client_encoding receives a null pointer.
// Reproduces the crash from msdp::GroupReporter::append_char when GET_NAME(character)
// returns null (e.g. due to a corrupted/freed character still in the follower list).
TEST(DescriptorEncoding, StringToClientEncoding_NullInput_DoesNotCrash)
{
	DescriptorData d;
	char buffer[256] = {0};
	d.string_to_client_encoding(nullptr, buffer);
	EXPECT_EQ('\0', buffer[0]);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
