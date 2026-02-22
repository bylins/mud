#include "engine/core/config.h"

#include <gtest/gtest.h>

#ifdef ENABLE_GCOV_FLUSH
// GCC gcov: flush coverage data before _Exit() bypasses atexit handlers
extern "C" void __gcov_dump();
#endif

class BylinsEnvironment: public ::testing::Environment
{
public:
	void SetUp() override;
};

void BylinsEnvironment::SetUp()
{
	runtime_config.disable_logging();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new BylinsEnvironment());

	const auto result = RUN_ALL_TESTS();

#ifdef ENABLE_GCOV_FLUSH
	// Flush gcov coverage data before _Exit() (which bypasses atexit handlers)
	__gcov_dump();
#endif

	// Use _Exit() to skip all cleanup (atexit, global destructors, etc.)
	// exit() still causes heap corruption on Windows, _Exit() bypasses everything
	_Exit(result);
}
