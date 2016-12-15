#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include <string>
#include <list>
#include <memory>
#include <map>

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

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

			virtual void execute(CHAR_DATA* character, const arguments_t& arguments) = 0;
			virtual void execute(CHAR_DATA* character) { return execute(character, arguments_t()); }
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

		class CommandWithHelp : public AbstractCommand, public Help
		{
		public:
			using shared_ptr = std::shared_ptr<CommandWithHelp>;

			CommandWithHelp() {}
			CommandWithHelp(const std::string& help_line) : Help(help_line) {}
			CommandWithHelp(const std::string& help_line, const std::string& help) : Help(help_line, help) {}
		};

		class CommandEmbranchment : public AbstractCommand
		{
		public:
			using branches_t = std::map<std::string, CommandWithHelp::shared_ptr>;

			CommandEmbranchment() {}

			void add_command(const std::string& branch, CommandWithHelp::shared_ptr command) { m_branches.emplace(branch, command); }

			void set_noargs_handler(AbstractCommand::shared_ptr command) { m_noargs_handler = command; }
			void set_unexpected_command_handler(AbstractCommand::shared_ptr command) { m_unexpected_command_handler = command; }
			void set_name(const std::string& name) { m_name = name; }

			virtual void execute(CHAR_DATA* character, const arguments_t& arguments) override;

			void show_commands_list(CHAR_DATA* character) const;

		private:
			void build_commands_list() const;
			void execution_without_arguments(CHAR_DATA* character) const;
			void unexpected_branch(CHAR_DATA* character, const arguments_t& arguments) const;
			void make_suggestion(CHAR_DATA* character, const std::string& wrong_subcommand) const;

			branches_t m_branches;

			AbstractCommand::shared_ptr m_noargs_handler;
			AbstractCommand::shared_ptr m_unexpected_command_handler;

			std::string m_name;
			mutable std::string m_commands_list;
		};
	}
}

#endif // __COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
