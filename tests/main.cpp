#include <gtest/gtest.h>

void mag_assign_spells(void);	// defined in "spell_parser.cpp"

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	// Initialize some MUD structures
	mag_assign_spells();

	return RUN_ALL_TESTS();
}
