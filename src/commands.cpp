#include "commands.hpp"

#include "utils.h"
#include "char.hpp"
#include "levenshtein.hpp"

#include <sstream>
#include <iomanip>
#include <cstring>

namespace commands
{
	namespace utils
	{
		class CommandEmbranchmentImplementation : public CommandEmbranchment
		{
		public:
			static constexpr int SUGGESTIONS_COUNT = 2;

			using branch_t = CommandWithHelp::shared_ptr;
			using branches_t = std::map<std::string, branch_t>;
			using shared_ptr = std::shared_ptr<CommandEmbranchmentImplementation>;

			CommandEmbranchmentImplementation() {}

			void add_command(const std::string& branch, const branch_t& command) { m_branches.emplace(branch, command); }

			void set_noargs_handler(AbstractCommand::shared_ptr command) { m_noargs_handler = command; }
			void set_unexpected_command_handler(AbstractCommand::shared_ptr command) { m_unexpected_command_handler = command; }
			void set_name(const std::string& name) { m_name = name; }

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& arguments) override;

			void show_commands_list(const CommandContext::shared_ptr& context) const;

		private:
			void build_commands_list() const;
			void execution_without_arguments(const CommandContext::shared_ptr& context) const;
			void unexpected_branch(const CommandContext::shared_ptr& context, const arguments_t& arguments) const;
			branch_t get_handler(const std::string& prefix) const;
			bool check_uniqueness(const std::string& prefix) const;
			bool check_prefix(const std::string& prefix) const;
			void make_suggestion(const CommandContext::shared_ptr& context, const std::string& wrong_subcommand) const;
			bool replyable_context(const CommandContext::shared_ptr& context) const;
			void reply(const CommandContext::shared_ptr& context, const std::string& message) const;

			branches_t m_branches;

			AbstractCommand::shared_ptr m_noargs_handler;
			AbstractCommand::shared_ptr m_unexpected_command_handler;

			std::string m_name;
			mutable std::string m_commands_list;
		};

		void CommandEmbranchmentImplementation::execute(const CommandContext::shared_ptr& context, const arguments_t& arguments)
		{
			if (arguments.empty())
			{
				execution_without_arguments(context);
				return;
			}

			const auto& subcommand = arguments.front();
			const auto handler = get_handler(subcommand);
			if (handler)
			{
				handler->execute(context, arguments);
			}
			else
			{
				unexpected_branch(context, arguments);
			}
		}

		void CommandEmbranchmentImplementation::show_commands_list(const CommandContext::shared_ptr& context) const
		{
			if (m_commands_list.empty())
			{
				build_commands_list();
			}

			reply(context, m_commands_list);
		}

		void CommandEmbranchmentImplementation::build_commands_list() const
		{
			constexpr int COMMAND_LENGTH = 12;

			std::stringstream help;
			help << std::endl << std::endl;

			if (!m_name.empty())
			{
				help << "Аргументы команды " << m_name << ":" << std::endl << std::endl;
			}

			for (const auto& branch : m_branches)
			{
				help << "    " << std::setw(COMMAND_LENGTH) << branch.first
					<< branch.second->get_help_line() << std::endl;
			}
			help << std::endl;

			m_commands_list = help.str();
		}

		void CommandEmbranchmentImplementation::execution_without_arguments(const CommandContext::shared_ptr& context) const
		{
			if (m_noargs_handler)
			{
				m_noargs_handler->execute(context);
			}
			else
			{
				show_commands_list(context);
			}
		}

		void CommandEmbranchmentImplementation::unexpected_branch(const CommandContext::shared_ptr& context, const arguments_t& arguments) const
		{
			if (m_unexpected_command_handler)
			{
				m_unexpected_command_handler->execute(context, arguments);
				return;
			}

			const auto& subcommand = arguments.front();
			if ("" == subcommand)
			{
				show_commands_list(context);
				return;
			}

			make_suggestion(context, subcommand);
		}

		CommandEmbranchmentImplementation::branch_t CommandEmbranchmentImplementation::get_handler(const std::string& prefix) const
		{
			if (check_uniqueness(prefix)
				&& check_prefix(prefix))
			{
				const auto lower = m_branches.lower_bound(prefix);
				return lower->second;
			}

			return nullptr;
		}

		bool CommandEmbranchmentImplementation::check_uniqueness(const std::string& prefix) const
		{
			const auto lower = m_branches.lower_bound(prefix + '\0');
			const auto upper = m_branches.upper_bound(prefix + '\xff');
			if (lower != m_branches.end()
				&& 1 == std::distance(lower, upper))
			{
				return true;
			}

			return false;
		}

		bool CommandEmbranchmentImplementation::check_prefix(const std::string& prefix) const
		{
			const auto lower = m_branches.lower_bound(prefix);
			if (lower != m_branches.end()
				&& 0 == lower->first.find(prefix))
			{
				return true;
			}

			return false;
		}

		void CommandEmbranchmentImplementation::make_suggestion(const CommandContext::shared_ptr& context, const std::string& wrong_subcommand) const
		{
			if (!replyable_context(context))
			{
				return;
			}

			std::list<std::string> suggestions;

			using distances_t = std::multimap<int, const std::string*>;
			distances_t command_distances;
			for (const auto& branch : m_branches)
			{
				const auto distance = levenshtein(wrong_subcommand, branch.first, 0, 2, 1, 3);
				distances_t::value_type command(distance, &branch.first);
				command_distances.insert(command);
			}

			auto i = command_distances.begin();
			while (i != command_distances.end()
				&& suggestions.size() < SUGGESTIONS_COUNT)
			{
				suggestions.push_back(*i->second);
				++i;
			}

			std::string suggestions_string;
			joinList(suggestions, suggestions_string);
			std::stringstream ss;
			ss << "Подкоманда [" << wrong_subcommand << "] не существует. Возможно, вы имели ввиду одну из следующих: "
				<< suggestions_string << "?";

			reply(context, ss.str());
		}

		bool CommandEmbranchmentImplementation::replyable_context(const CommandContext::shared_ptr& context) const
		{
			return bool(std::dynamic_pointer_cast<ReplyableContext>(context));
		}

		void CommandEmbranchmentImplementation::reply(const CommandContext::shared_ptr& context, const std::string& message) const
		{
			const auto caller = std::dynamic_pointer_cast<ReplyableContext>(context);
			if (!caller)
			{
				return;
			}

			caller->reply(message);
		}

		commands::utils::CommandEmbranchment::shared_ptr CommandEmbranchment::create()
		{
			return std::make_shared<CommandEmbranchmentImplementation>();
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
