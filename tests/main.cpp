#include "engine/core/config.h"
#include "utils/logging/log_manager.h"
#include "utils/logging/log_sender.h"

#include <gtest/gtest.h>

namespace {

class NoOpLogSender : public logging::ILogSender {
public:
	void Debug(const std::string&) override {}
	void Debug(const std::string&, const std::map<std::string, std::string>&) override {}
	void Info(const std::string&) override {}
	void Info(const std::string&, const std::map<std::string, std::string>&) override {}
	void Warn(const std::string&) override {}
	void Warn(const std::string&, const std::map<std::string, std::string>&) override {}
	void Error(const std::string&) override {}
	void Error(const std::string&, const std::map<std::string, std::string>&) override {}
};

} // namespace

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
	logging::LogManager::ClearSenders();
	logging::LogManager::AddSender(std::make_unique<NoOpLogSender>());
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
