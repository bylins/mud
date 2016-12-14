#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include <string>
#include <list>
#include <memory>

namespace commands
{
	namespace utils
	{
		class AbstractCommand
		{
		public:
			using shared_ptr = std::shared_ptr<AbstractCommand>;

			using argument_t = std::string;
			using arguments_t = std::list<argument_t>;

			virtual ~AbstractCommand() {}

			virtual void execute(const arguments_t& arguments) = 0;
			virtual void execute() { return execute(arguments_t()); }
		};

		class Help
		{
		public:
			Help() {}
			Help(const std::string& help_line) : m_help_line(help_line) {}
			Help(const std::string& help_line, const std::string& help) : m_help_line(help_line), m_help(help) {}

			const std::string& get_help_line() const { return m_help_line; }
			const std::string& get_help() const { return m_help; }

		private:
			std::string m_help_line;
			std::string m_help;
		};

		class CommandWithHelp : public AbstractCommand
		{
		public:
			using shared_ptr = std::shared_ptr<CommandWithHelp>;

			CommandWithHelp() {}
			CommandWithHelp(const std::string& help_line) : m_help(help_line) {}
			CommandWithHelp(const std::string& help_line, const std::string& help) : m_help(help_line, help) {}

			virtual const std::string& get_help_line() const {}
			virtual const std::string& get_help() const {}

		private:
			Help m_help;
		};

		class CommandEmbranchment : public AbstractCommand
		{
		public:
			using branches_t = std::list<CommandWithHelp::shared_ptr>;

			CommandEmbranchment(const branches_t& branches) : m_branches(branches) {}

			virtual void execute(const arguments_t& arguments) override;

		private:
			branches_t m_branches;
		};
	}
}

#endif // __COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
