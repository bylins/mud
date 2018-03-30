#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include <string>
#include <list>
#include <memory>
#include <map>

class CHAR_DATA;	// to avoid inclusion of char.hpp

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

		class AbstractReplyableContext: public CommandContext
		{
		public:
			virtual void reply(const std::string& message) const = 0;
		};

		class ReplyableContext : public AbstractReplyableContext
		{
		public:
			using shared_ptr = std::shared_ptr<ReplyableContext>;

			ReplyableContext(CHAR_DATA* character) : m_character(character) {}

			virtual void reply(const std::string& message) const override;

		private:
			CHAR_DATA * m_character;
		};

		class AbstractCommand
		{
		public:
			using shared_ptr = std::shared_ptr<AbstractCommand>;

			using argument_t = std::string;

			class Arguments: public std::list<argument_t>
			{
			public:
				using parent_t = std::list<argument_t>;

				Arguments() {}
				Arguments(const_iterator& _begin, const_iterator _end) : parent_t(_begin, _end) {}

				/**
				* This constructor splits value passed by arguments into tokens.
				* '\0' character after each token will be added.
				*
				* \note This constructor changes (!) passed argument.
				*/
				Arguments(char* arguments);
			};

			using arguments_t = Arguments;

			virtual ~AbstractCommand() {}

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) = 0;
			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path) final { return execute(context, path, arguments_t()); }
			virtual void execute(const CommandContext::shared_ptr& context) final { return execute(context, arguments_t(), arguments_t()); }
		};

		class Help
		{
		public:
			Help() {}
			Help(const std::string& help_line) : m_help_line(help_line) {}
			Help(const std::string& help_line, const std::string& help) : m_help_line(help_line), m_help(help) {}
			virtual ~Help() {}

			virtual const std::string& get_help_line() const { return m_help_line; }
			virtual const std::string& get_help() const { return m_help; }

		protected:
			void set_help(const std::string& help) { m_help = help; }
			void set_help_line(const std::string& help_line) { m_help_line = help_line; }

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

		class CommonCommand : public CommandWithHelp
		{
		protected:
			void send(const CommandContext::shared_ptr& context, const std::string& message) const;
			void usage(const CommandContext::shared_ptr& context) const { send(context, get_help()); }
		};

		class CommandEmbranchment : public CommandWithHelp
		{
		public:
			static constexpr int SUGGESTIONS_COUNT = 2;

			using branch_t = CommandWithHelp::shared_ptr;
			using shared_ptr = std::shared_ptr<CommandEmbranchment>;

			CommandEmbranchment() {}

			virtual CommandEmbranchment& add_command(const std::string& branch, const branch_t& command) = 0;
			virtual CommandEmbranchment& set_noargs_handler(AbstractCommand::shared_ptr command) = 0;
			virtual CommandEmbranchment& set_unexpected_command_handler(AbstractCommand::shared_ptr command) = 0;
			virtual CommandEmbranchment& rebuild_help() = 0;

			virtual branch_t get_command(const std::string& command) const = 0;

			static shared_ptr create(const std::string& help_line = "");
		};

		class ParentalHelp : public CommandWithHelp
		{
		public:
			ParentalHelp(const shared_ptr& parent);

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) override;

		private:
			shared_ptr m_parent;
		};
	}

	class AbstractCommandsHanler
	{
	public:
		using shared_ptr = std::shared_ptr<AbstractCommandsHanler>;

		virtual void initialize() = 0;
		virtual void process(CHAR_DATA* character, char* arguments) = 0;
	};

	template <class T>
	class HandlerInitializer
	{
	public:
		HandlerInitializer();

		auto& operator()() const { return m_handler; }

	private:
		commands::AbstractCommandsHanler::shared_ptr m_handler;
	};

	template <class T>
	commands::HandlerInitializer<T>::HandlerInitializer():
		m_handler(std::make_shared<T>())
	{
		m_handler->initialize();
	}
}

#endif // __COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
