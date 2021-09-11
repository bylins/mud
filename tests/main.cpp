#include <config.h>
#include <magic/spells_info.h>

#include <gtest/gtest.h>

class BylinsEnvironment: public ::testing::Environment
{
public:
	void SetUp() override;
};

void BylinsEnvironment::SetUp()
{
	initSpells();
	runtime_config.disable_logging();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new BylinsEnvironment());

	const auto result = RUN_ALL_TESTS();

	return result;
}
