#include "heartbeat.commands.hpp"

namespace heartbeat
{
	namespace cmd
	{
		namespace
		{
			using namespace commands::utils;

			class ShowGeneralStats : public commands::utils::CommonCommand
			{
			public:
				ShowGeneralStats() { set_help_line("Show general heartbeat stats."); }

				virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) override
				{
					send(context, "Stats will be right here.");
				}
			};

			class CommandsHandler : public commands::AbstractCommandsHanler
			{
			public:
				virtual void initialize() override;
				virtual void process(CHAR_DATA* character, char* arguments) override;

			private:
				CommandEmbranchment::shared_ptr m_command;
			};

			void CommandsHandler::initialize()
			{
				m_command = CommandEmbranchment::create();
				const auto stats_command = CommandEmbranchment::create("Allows to get access to heartbeat stats.");

				const auto show_general_stats_command = std::make_shared<ShowGeneralStats>();
				stats_command->add_command("show", std::make_shared<ShowGeneralStats>())
					.rebuild_help();

				m_command->add_command("stats", stats_command)
					.rebuild_help();
			}

			void CommandsHandler::process(CHAR_DATA* character, char* arguments)
			{
				AbstractCommand::arguments_t arguments_list(arguments);
				const auto context = ReplyableContext::create(character);
				m_command->execute(context, { heartbeat::cmd::HEARTBEAT_COMMAND }, arguments_list);
			}

			const commands::AbstractCommandsHanler::shared_ptr& commands_handler()
			{
				static commands::HandlerInitializer<CommandsHandler> handler;

				return handler();
			}
		}

		void do_heartbeat(CHAR_DATA *ch, char *arguments, int /*cmd*/, int /*subcmd*/)
		{
			commands_handler()->process(ch, arguments);
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
