#include "heartbeat.commands.hpp"

#include "heartbeat.hpp"
#include "logger.hpp"
#include "utils.h"
#include "global.objects.hpp"

#include <sstream>
#include <iomanip>

namespace heartbeat
{
	namespace cmd
	{
		namespace
		{
			using namespace commands::utils;

			class HeartbeatCommandContext : public ReplyableContext
			{
			public:
				HeartbeatCommandContext(CHAR_DATA* character, Heartbeat& heartbeat) : ReplyableContext(character), m_heartbeat(heartbeat) {}

				Heartbeat& operator()() const { return m_heartbeat; }

			private:
				Heartbeat & m_heartbeat;
			};

			class ShowGeneralPulseStats : public commands::utils::CommonCommand
			{
			public:
				ShowGeneralPulseStats() { set_help_line("Shows general pulse stats."); }

				virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) override;
			};

			void ShowGeneralPulseStats::execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments)
			{
				const auto heartbeat_context = std::dynamic_pointer_cast<HeartbeatCommandContext>(context);
				if (!heartbeat_context)
				{
					log("SYSERR: context of heartbeat command '%s' is not an instance of class 'HeartbeatCommandContext'.",
						JoinRange<arguments_t>(path).as_string().c_str());
					return;
				}

				const auto& heartbeat = (*heartbeat_context)();
				const auto& steps = heartbeat.steps();

				std::stringstream ss;

				ss << "General pulse stats: coming soon." << std::endl;

				send(context, ss.str());
			}

			class ShowGeneralHeartbeatStats : public commands::utils::CommonCommand
			{
			public:
				ShowGeneralHeartbeatStats() { set_help_line("Shows general heartbeat stats."); }

				virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments) override;

			private:
				static void print_steps(std::ostream& os, const Heartbeat& heartbeat, const Heartbeat::pulse_label_t& label);
			};

			void ShowGeneralHeartbeatStats::execute(const CommandContext::shared_ptr& context, const arguments_t& path, const arguments_t& arguments)
			{
				const auto heartbeat_context = std::dynamic_pointer_cast<HeartbeatCommandContext>(context);
				if (!heartbeat_context)
				{
					log("SYSERR: context of heartbeat command '%s' is not an instance of class 'HeartbeatCommandContext'.",
						JoinRange<arguments_t>(path).as_string().c_str());
					return;
				}

				const auto& heartbeat = (*heartbeat_context)();
				const auto& stats = heartbeat.stats();
				std::stringstream ss;
				ss << std::fixed << std::setprecision(6);
				ss << "Heartbeat stats:" << std::endl
					<< "\tCurrent pulse number &Y" << heartbeat.pulse_number() << "&n"
					<< ", rolls over after &W" << Heartbeat::ROLL_OVER_AFTER << "&n pulses" << std::endl
					<< "\tCurrent global pulse number: &Y" << heartbeat.global_pulse_number() << "&n" << std::endl
					<< "\tNumber of different steps: &Y" << heartbeat.steps().size() << "&n" << std::endl
					<< "\tSteps period: &Y" << heartbeat.period() << "&n pulses." << std::endl
					<< "\tAverage pulse duration: ";
				if (0 == stats.count())
				{
					ss << "&W<no measurements>&n";
				}
				else
				{
					ss << "&Y" << (stats.sum() / stats.count()) << "&n seconds.";
				}
				ss << std::endl;

				{
					const auto& longest_pulse = stats.global_max();
					ss << "\tLongest pulse: ";
					if (longest_pulse.second != BasePulseMeasurements::NO_VALUE)
					{
						ss << "&Y" << longest_pulse.second.first
							<< "&n; duration: &Y" << longest_pulse.second.second
							<< "&n; steps:" << std::endl;

						print_steps(ss, heartbeat, longest_pulse.first);
					}
					else
					{
						ss << "&W<no measurements>&n" << std::endl;
					}
				}

				{
					const auto& shortest_pulse = stats.global_min();
					ss << "\tShortest pulse: ";
					if (shortest_pulse.second != BasePulseMeasurements::NO_VALUE)
					{
						ss << "&Y" << shortest_pulse.second.first
							<< "&n; duration: &Y" << shortest_pulse.second.second
							<< "&n; steps:" << std::endl;

						print_steps(ss, heartbeat, shortest_pulse.first);
					}
					else
					{
						ss << "&W<no measurements>&n" << std::endl;
					}
				}

				send(context, ss.str());
			}

			void ShowGeneralHeartbeatStats::print_steps(std::ostream& os, const Heartbeat& heartbeat, const Heartbeat::pulse_label_t& label)
			{
				std::set<std::size_t> sorted_indexes;
				for (const auto index : label)
				{
					sorted_indexes.insert(index);
				}

				for (const auto index : sorted_indexes)
				{
					os << "\t\t&G" << (1 + index) << "&n. &W";
					heartbeat.print_step_name(os, index);
					os << "&n" << std::endl;
				}
			}

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

				const auto show_general_pulse_stats_command = CommandEmbranchment::create("Allows to get access to pulse stats.");
				show_general_pulse_stats_command->add_command("show", std::make_shared<ShowGeneralPulseStats>())
					.add_command("help", std::make_shared<ParentalHelp>(show_general_pulse_stats_command))
					.rebuild_help();

				const auto show_general_heartbeat_stats = std::make_shared<ShowGeneralHeartbeatStats>();
				m_command->set_noargs_handler(show_general_heartbeat_stats)
					.add_command("stats", show_general_pulse_stats_command)
					.add_command("help", std::make_shared<ParentalHelp>(m_command))
					.rebuild_help();
			}

			void CommandsHandler::process(CHAR_DATA* character, char* arguments)
			{
				AbstractCommand::arguments_t arguments_list(arguments);
				const auto context = std::make_shared<HeartbeatCommandContext>(character, GlobalObjects::heartbeat());
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
