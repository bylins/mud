#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include <string>
#include <list>
#include <memory>
#include <map>

namespace commands
{
	namespace utils
	{
		class CommandContext
		{
		public:
			using shared_ptr = std::shared_ptr<CommandContext>;

			virtual ~CommandContext() {}
		};

		class ReplyableContext: public CommandContext
		{
		public:
			virtual void reply(const std::string& message) const = 0;
		};

		class AbstractCommand
		{
		public:
			using shared_ptr = std::shared_ptr<AbstractCommand>;

			using argument_t = std::string;
			using arguments_t = std::list<argument_t>;

			virtual ~AbstractCommand() {}

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& arguments) = 0;
			virtual void execute(const CommandContext::shared_ptr& context) { return execute(context, arguments_t()); }
		};

		class Help
		{
		public:
			Help() {}
			Help(const std::string& help_line) : m_help_line(help_line) {}
			Help(const std::string& help_line, const std::string& help) : m_help_line(help_line), m_help(help) {}
			~Help() {}

			virtual const std::string& get_help_line() const { return m_help_line; }
			virtual const std::string& get_help() const { return m_help; }

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

		class CommandEmbranchment : public CommandWithHelp
		{
		public:
			static constexpr int SUGGESTIONS_COUNT = 2;

			using branch_t = CommandWithHelp::shared_ptr;
			using branches_t = std::map<std::string, branch_t>;
			using shared_ptr = std::shared_ptr<CommandEmbranchment>;

			CommandEmbranchment() {}

			virtual void add_command(const std::string& branch, const branch_t& command) = 0;

			virtual void set_noargs_handler(AbstractCommand::shared_ptr command) = 0;
			virtual void set_unexpected_command_handler(AbstractCommand::shared_ptr command) = 0;
			virtual void set_name(const std::string& name) = 0;

			static shared_ptr create();
		};
	}
}

#endif // __COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
