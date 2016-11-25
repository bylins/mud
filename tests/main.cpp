#include <config.hpp>
#include <class.hpp>

#include <gtest/gtest.h>

void mag_assign_spells(void);	// defined in "spell_parser.cpp"

class BylinsEnvironment: public ::testing::Environment
{
public:
	virtual void SetUp();
};

void BylinsEnvironment::SetUp()
{
	mag_assign_spells();
	runtime_config.disable_logging();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new BylinsEnvironment());

	return RUN_ALL_TESTS();
}
