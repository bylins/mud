#include "commands.hpp"

#include "utils.h"
#include "chars/character.h"
#include "levenshtein.hpp"
#include "compact.trie.hpp"
#include "utils.string.hpp"

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
			static constexpr int SUGGESTIONS_COUNT = 4;

			using branch_t = CommandWithHelp::shared_ptr;
			using shared_ptr = std::shared_ptr<CommandEmbranchmentImplementation>;

			CommandEmbranchmentImplementation() {}

			virtual CommandEmbranchment& add_command(const std::string& branch, const branch_t& command) override;
			virtual CommandEmbranchment& set_noargs_handler(AbstractCommand::shared_ptr command) override;
			virtual CommandEmbranchment& set_unexpected_command_handler(AbstractCommand::shared_ptr command) override;
			virtual CommandEmbranchment& rebuild_help() override;

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) override;

			virtual branch_t get_command(const std::string& command) const override { return std::move(get_handler(command)); }

			void show_commands_list(const CommandContext::shared_ptr& context) const;

		private:
			using handlers_t = std::unordered_map<std::string, branch_t>;
			class Branches
			{
			public:
				Branches();

				void add_command(const std::string& branch, const branch_t& command);

				const auto& trie() const { return m_trie; }
				const auto& handlers() const { return m_handlers; }
				auto max_length() const { return m_max_length; }

				auto begin() const { return std::move(m_trie.begin()); }
				auto end() const { return std::move(m_trie.end()); }

			private:
				CompactTrie m_trie;
				handlers_t m_handlers;
				std::size_t m_max_length;
			};

			void build_commands_list() const;
			void execution_without_arguments(const CommandContext::shared_ptr& context, const arguments_t& path) const;
			void unexpected_branch(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) const;
			branch_t get_handler(const std::string& prefix) const;
			void make_suggestion(const CommandContext::shared_ptr& context, const std::string& wrong_subcommand) const;
			bool replyable_context(const CommandContext::shared_ptr& context) const;
			void reply(const CommandContext::shared_ptr& context, const std::string& message) const;
			void print_branches_list(std::stringstream& ss) const;

			Branches m_branches;

			AbstractCommand::shared_ptr m_noargs_handler;
			AbstractCommand::shared_ptr m_unexpected_command_handler;

			mutable std::string m_commands_list;
		};

		CommandEmbranchmentImplementation::Branches::Branches() : m_max_length(0)
		{
		}

		void CommandEmbranchmentImplementation::Branches::add_command(const std::string& branch, const branch_t& command)
		{
			m_trie.add_string(branch);
			m_handlers.emplace(branch, command);

			if (m_max_length < branch.size())
			{
				m_max_length = branch.size();
			}
		}

		CommandEmbranchment& CommandEmbranchmentImplementation::add_command(const std::string& branch, const branch_t& command)
		{
			m_branches.add_command(branch, command);
			return *this;
		}

		CommandEmbranchment& CommandEmbranchmentImplementation::set_noargs_handler(AbstractCommand::shared_ptr command)
		{
			m_noargs_handler = command;
			return *this;
		}

		CommandEmbranchment& CommandEmbranchmentImplementation::set_unexpected_command_handler(AbstractCommand::shared_ptr command)
		{
			m_unexpected_command_handler = command;
			return *this;
		}

		CommandEmbranchment& CommandEmbranchmentImplementation::rebuild_help()
		{
			std::stringstream ss;
			print_branches_list(ss);
			set_help(ss.str());

			return *this;
		}

		void CommandEmbranchmentImplementation::execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments)
		{
			if (arguments.empty())
			{
				execution_without_arguments(context, path);
				return;
			}

			const auto& subcommand = arguments.front();

			const auto handler = get_handler(subcommand);
			if (handler)
			{
				const arguments_t new_arguments(++arguments.begin(), arguments.end());
				arguments_t new_path = path;
				new_path.push_back(subcommand);

				handler->execute(context, new_path, new_arguments);
			}
			else
			{
				unexpected_branch(context, path, arguments);
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
			std::stringstream help;
			print_branches_list(help);
			m_commands_list = help.str();
		}

		void CommandEmbranchmentImplementation::execution_without_arguments(const CommandContext::shared_ptr& context, const arguments_t& path) const
		{
			if (m_noargs_handler)
			{
				m_noargs_handler->execute(context, path);
			}
			else
			{
				show_commands_list(context);
			}
		}

		void CommandEmbranchmentImplementation::unexpected_branch(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) const
		{
			if (m_unexpected_command_handler)
			{
				m_unexpected_command_handler->execute(context, path, arguments);
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
			const auto range = m_branches.trie().find_by_prefix(prefix);
			if (!range.empty()
				&& ++range.begin() == range.end())
			{
				const auto prefix = range.begin()->prefix();
				return m_branches.handlers().at(prefix);
			}

			return nullptr;
		}

		void CommandEmbranchmentImplementation::make_suggestion(const CommandContext::shared_ptr& context, const std::string& wrong_subcommand) const
		{
			if (!replyable_context(context))
			{
				return;
			}

			std::list<std::string> suggestions;

			using distances_t = std::multimap<int, std::string>;
			distances_t command_distances;
			for (const auto& branch : m_branches)
			{
				const auto distance = levenshtein(wrong_subcommand, branch.prefix(), 0, 2, 1, 3);
				distances_t::value_type command(distance, branch.prefix());
				command_distances.insert(command);
			}

			auto i = command_distances.begin();
			while (i != command_distances.end()
				&& suggestions.size() < SUGGESTIONS_COUNT)
			{
				suggestions.push_back(i->second);
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
			return bool(std::dynamic_pointer_cast<AbstractReplyableContext>(context));
		}

		void CommandEmbranchmentImplementation::reply(const CommandContext::shared_ptr& context, const std::string& message) const
		{
			const auto caller = std::dynamic_pointer_cast<AbstractReplyableContext>(context);
			if (!caller)
			{
				return;
			}

			caller->reply(message);
		}

		void CommandEmbranchmentImplementation::print_branches_list(std::stringstream& ss) const
		{
			ss << std::endl;
			for (const auto& branch : m_branches.trie())
			{
				const std::string& prefix = branch.prefix();
				ss << "    " << std::setw(m_branches.max_length()) << prefix
					<< " - " << m_branches.handlers().at(prefix)->get_help_line() << std::endl;
			}
			ss << std::endl;
		}

		CommandEmbranchment::shared_ptr CommandEmbranchment::create(const std::string& help_line/* = ""*/)
		{
			const auto result = std::make_shared<CommandEmbranchmentImplementation>();
			result->set_help_line(help_line);
			return result;
		}

		ParentalHelp::ParentalHelp(const shared_ptr& parent) : CommandWithHelp("Показать справку."), m_parent(parent)
		{
		}

		void ParentalHelp::execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments)
		{
			const auto parent = std::dynamic_pointer_cast<CommandWithHelp>(m_parent);
			if (!parent)
			{
				return;
			}

			const auto replyable_context = std::dynamic_pointer_cast<AbstractReplyableContext>(context);
			if (!replyable_context)
			{
				return;
			}

			if (!arguments.empty())
			{
				std::stringstream ss;
				auto embranchment = std::dynamic_pointer_cast<CommandEmbranchment>(m_parent);
				for (arguments_t::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument)
				{
					const char* COMMANDS_DELIMITER = " ";

					const auto handler = embranchment->get_command(*argument);
					if (!handler)
					{
						ss << "Invalid argument '" << *argument << "' for command '"
							<< JoinRange<arguments_t>(path, COMMANDS_DELIMITER)
							<< (path.empty() || argument == arguments.begin() ? "" : COMMANDS_DELIMITER)
							<< JoinRange<arguments_t>(arguments.begin(), argument, COMMANDS_DELIMITER) << "'.";
						break;
					}

					embranchment = std::dynamic_pointer_cast<CommandEmbranchment>(handler);
					if (!embranchment)
					{
						auto next = argument;
						if (++next == arguments.end())
						{
							ss << handler->get_help();
						}
						else
						{
							ss << "Command '"
								<< JoinRange<arguments_t>(path, COMMANDS_DELIMITER)
								<< (path.empty() || argument == arguments.begin() ? "" : COMMANDS_DELIMITER)
								<< JoinRange<arguments_t>(arguments.begin(), argument, COMMANDS_DELIMITER) << "'."
								<< "' doesn't have subcommand '" << *argument << "'";
						}
						break;
					}
				}

				if (embranchment
					&& embranchment.get() != parent.get())
				{
					replyable_context->reply(embranchment->get_help());
				}
				else
				{
					replyable_context->reply(ss.str());
				}

				return;
			}

			replyable_context->reply(parent->get_help());
		}

		void ReplyableContext::reply(const std::string& message) const
		{
			send_to_char(message, m_character);
		}

		namespace
		{
			/**
			* Returns pointer to the first argument, moves command_line to the beginning of the next argument.
			*
			* \note This routine changes a passed buffer: puts '\0' character after the first argument.
			*/
			const char* first_argument(char*& command_line)
			{
				// skip leading spaces
				while (*command_line && ' ' == *command_line)
				{
					++command_line;
				}

				// getting subcommand
				const char* result = command_line;
				while (*command_line && ' ' != *command_line)
				{
					++command_line;
				}

				// anchor subcommand and move argument pointer to the next one
				if (*command_line)
				{
					*(command_line++) = '\0';
				}

				return result;
			}
		}

		AbstractCommand::Arguments::Arguments(char* arguments)
		{
			while (*arguments)
			{
				auto next_argument = first_argument(arguments);
				if (*next_argument)
				{
					push_back(next_argument);
				}
			}
		}

		void CommonCommand::send(const CommandContext::shared_ptr& context, const std::string& message) const
		{
			const auto rcontext = std::dynamic_pointer_cast<AbstractReplyableContext>(context);
			if (rcontext)
			{
				rcontext->reply(message);
			}
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
