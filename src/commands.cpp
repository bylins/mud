#include "commands.hpp"

#include "char.hpp"

#include <sstream>
#include <iomanip>

namespace commands
{
	namespace utils
	{
		void CommandEmbranchment::execute(CHAR_DATA* character, const arguments_t& arguments)
		{
			if (arguments.empty())
			{
				execution_without_arguments(character);
				return;
			}

			const auto& subcommand = arguments.front();
			const auto handler = m_branches.find(subcommand);
			if (handler == m_branches.end())
			{
				unexpected_branch(character, arguments);
			}
			else
			{
				handler->second->execute(character, arguments);
			}
		}

		void CommandEmbranchment::show_commands_list(CHAR_DATA* character) const
		{
			if (m_commands_list.empty())
			{
				build_commands_list();
			}

			send_to_char(m_commands_list, character);
		}

		void CommandEmbranchment::build_commands_list() const
		{
			constexpr int COMMAND_LENGTH = 12;

			std::stringstream help;
			help << std::endl;

			if (!m_name.empty())
			{
				help << "Аргументы команды " << m_name << ":" << std::endl;
			}

			for (const auto& branch : m_branches)
			{
				help << "    " << std::setw(COMMAND_LENGTH) << branch.first
					<< branch.second->get_help_line() << std::endl;
			}
			help << std::endl;

			m_commands_list = help.str();
		}

		void CommandEmbranchment::execution_without_arguments(CHAR_DATA* character) const
		{
			if (m_noargs_handler)
			{
				m_noargs_handler->execute(character);
			}
			else
			{
				show_commands_list(character);
			}
		}

		void CommandEmbranchment::unexpected_branch(CHAR_DATA* character, const arguments_t& arguments) const
		{
			if (m_unexpected_command_handler)
			{
				m_unexpected_command_handler->execute(character, arguments);
				return;
			}

			const auto& subcommand = arguments.front();
			if ("" == subcommand)
			{
				show_commands_list(character);
			}
			else
			{
				make_suggestion(character, subcommand);
			}
		}

		void CommandEmbranchment::make_suggestion(CHAR_DATA* character, const std::string& wrong_subcommand) const
		{
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
