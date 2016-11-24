#include <config.hpp>

#include <gtest/gtest.h>

void mag_assign_spells(void);	// defined in "spell_parser.cpp"

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	// Initialize some MUD structures
	mag_assign_spells();
	runtime_config.disable_logging();

	return RUN_ALL_TESTS();
}
