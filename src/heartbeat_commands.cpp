#include "heartbeat_commands.h"

#include "heartbeat.h"
#include "structs/global_objects.h"
#include "utils/utils_string.h"

#include <iomanip>

namespace heartbeat {
namespace cmd {
namespace {
using namespace commands::utils;

class HeartbeatCommandContext : public ReplyableContext {
 public:
	HeartbeatCommandContext(CharData *character, Heartbeat &heartbeat)
		: ReplyableContext(character), m_heartbeat(heartbeat) {}

	Heartbeat &operator()() const { return m_heartbeat; }

 private:
	Heartbeat &m_heartbeat;
};

class DefaultFloatsFormat {};
std::ostream &operator<<(std::ostream &os, const DefaultFloatsFormat &) {
	return os << std::fixed << std::setprecision(6);
}

class ClearStats : public commands::utils::CommonCommand {
 public:
	ClearStats() { set_help_line("Clear heartbeat stats."); }

	virtual void execute(const CommandContext::shared_ptr &context,
						 const arguments_t &path,
						 const arguments_t &arguments) override;
};

void ClearStats::execute(const CommandContext::shared_ptr &context,
						 const arguments_t &path,
						 const arguments_t &/*arguments*/) {
	const auto heartbeat_context = std::dynamic_pointer_cast<HeartbeatCommandContext>(context);
	if (!heartbeat_context) {
		log("SYSERR: context of heartbeat command '%s' is not an instance of class 'HeartbeatCommandContext'.",
			JoinRange<arguments_t>(path).as_string().c_str());
		return;
	}

	(*heartbeat_context)().clear_stats();
	send(context, "Статистика очищена");
}

class ShowStepStats : public commands::utils::CommonCommand {
 public:
	ShowStepStats() { set_help_line("Shows step stats."); }

	virtual void execute(const CommandContext::shared_ptr &context,
						 const arguments_t &path,
						 const arguments_t &arguments) override;

 private:
	std::ostream &output_step_stats(std::ostream &os, const Heartbeat::PulseStep &step, std::size_t index) {
		const auto &stats = step.stats();

		os << DefaultFloatsFormat();
		os << "Step: &W" << step.name() << "&n; Number: &W" << (1 + index) << "&n" << "\r\n";
		os << "\tModulo: &W" << step.modulo() << "&n; offset: &W" << step.offset() << "&n" << "\r\n";
		os << "\tCurrently &Y" << (step.on() ? "on" : "off") << "&n" << "\r\n";

		os << "\tExecuted &Y" << stats.global_count() << "&n times.";
		if (0 < stats.global_count()) {
			os << " Average execution time is &Y" << (stats.global_sum() / stats.global_count()) << "&n seconds.";
		}
		os << "\r\n";

		os << "\tWindow size is &W" << stats.window_size() << "&n" << "\r\n";

		if (stats.global_count() > stats.window_size()) {
			os << "\tAverage in window: &Y" << (stats.window_sum() / stats.current_window_size()) << "&n seconds."
			   << "\r\n";
		}

		if (stats.global_max().second != BasePulseMeasurements::NO_VALUE) {
			os << "\tLongest execution time was on pulse &Y" << stats.global_max().second.first << "&n: "
			   << "&Y" << stats.global_max().second.second.first << "&n seconds." << "\r\n";
		}

		if (stats.global_min().second != BasePulseMeasurements::NO_VALUE) {
			os << "\tFastest execution time was on pulse &Y" << stats.global_min().second.first << "&n: "
			   << "&Y" << stats.global_min().second.second.first << "&n seconds." << "\r\n";
		}

		return os;
	}
};

void ShowStepStats::execute(const CommandContext::shared_ptr &context,
							const arguments_t &path,
							const arguments_t &arguments) {
	const auto heartbeat_context = std::dynamic_pointer_cast<HeartbeatCommandContext>(context);
	if (!heartbeat_context) {
		log("SYSERR: context of heartbeat command '%s' is not an instance of class 'HeartbeatCommandContext'.",
			JoinRange<arguments_t>(path).as_string().c_str());
		return;
	}

	if (1 != arguments.size()) {
		std::stringstream ss;
		ss << "Error: not enough or too much arguments. Must be exactly one." << "\r\n";
		send(context, ss.str());

		usage(context);

		return;
	}

	const auto number = std::strtoull(arguments.front().c_str(), nullptr, 10);
	if (0 == number) {
		std::stringstream ss;
		ss << "Error: &R" << arguments.front() << "&n is not a valid natural number (1, 2, 3, ...)."
		   << "\r\n";
		send(context, ss.str());

		usage(context);

		return;
	}

	const auto &heartbeat = (*heartbeat_context)();
	const auto &steps = heartbeat.steps();

	if (number > steps.size()) {
		std::stringstream ss;
		ss << "Error: number &R" << arguments.front() << "&n of the step is too big. Maximal is &W"
		   << steps.size() << "&n." << "\r\n";
		send(context, ss.str());

		usage(context);

		return;
	}

	const std::size_t index = number - 1;
	std::stringstream ss;
	output_step_stats(ss, steps[index], index);

	send(context, ss.str());
}

class ShowStepsStats : public commands::utils::CommonCommand {
 public:
	ShowStepsStats() { set_help_line("Shows overall steps stats."); }

	virtual void execute(const CommandContext::shared_ptr &context,
						 const arguments_t &path,
						 const arguments_t &arguments) override;
};

void ShowStepsStats::execute(const CommandContext::shared_ptr &context, const arguments_t &, const arguments_t &) {
	send(context, "Coming soon...");
}

class ShowHeartbeatStats : public commands::utils::CommonCommand {
 public:
	ShowHeartbeatStats() { set_help_line("Shows general heartbeat stats."); }

	virtual void execute(const CommandContext::shared_ptr &context,
						 const arguments_t &path,
						 const arguments_t &arguments) override;

 private:
	static void print_steps(std::ostream &os, const Heartbeat &heartbeat, const Heartbeat::pulse_label_t &label);
};

void ShowHeartbeatStats::execute(const CommandContext::shared_ptr &context,
								 const arguments_t &path,
								 const arguments_t &) {
	const auto heartbeat_context = std::dynamic_pointer_cast<HeartbeatCommandContext>(context);
	if (!heartbeat_context) {
		log("SYSERR: context of heartbeat command '%s' is not an instance of class 'HeartbeatCommandContext'.",
			JoinRange<arguments_t>(path).as_string().c_str());
		return;
	}

	const auto &heartbeat = (*heartbeat_context)();
	const auto &stats = heartbeat.stats();
	std::stringstream ss;
	ss << DefaultFloatsFormat();
	ss << "Heartbeat stats:" << "\r\n"
	   << "\tCurrent pulse number &Y" << heartbeat.pulse_number() << "&n"
	   << ", rolls over after &W" << Heartbeat::ROLL_OVER_AFTER << "&n pulses" << "\r\n"
	   << "\tCurrent global pulse number: &Y" << heartbeat.global_pulse_number() << "&n" << "\r\n"
	   << "\tNumber of different steps: &W" << heartbeat.steps().size() << "&n" << "\r\n"
	   << "\tNumber of different steps executed ever: &Y" << heartbeat.executed_steps().size() << "&n" << "\r\n"
	   << "\tSteps period: &Y" << heartbeat.period() << "&n pulses." << "\r\n";

	if (0 < stats.global_count()) {
		ss << "\tAverage: &Y" << (stats.global_sum() / stats.global_count()) << "&n seconds." << "\r\n";
	}

	ss << "\tWindow size is &W" << stats.window_size() << "&n." << "\r\n";
	if (stats.global_count() > stats.current_window_size()) {
		ss << "\tAverage in window: &Y" << (stats.window_sum() / stats.current_window_size()) << "&n seconds."
		   << "\r\n";
	}

	{
		const auto &longest_pulse = stats.global_max();
		ss << "\tLongest pulse: ";
		if (longest_pulse.second != BasePulseMeasurements::NO_VALUE) {
			ss << "&Y" << longest_pulse.second.first
			   << "&n; duration: &Y" << longest_pulse.second.second.first
			   << "&n seconds, time &Y" << asctime(localtime(&longest_pulse.second.second.second));
			ss.seekp(-1, std::ios_base::end);
			ss << "&n; steps:" << "\r\n";
			print_steps(ss, heartbeat, longest_pulse.first);
		} else {
			ss << "&W<no value>&n" << "\r\n";
		}
	}

	{
		const auto &shortest_pulse = stats.global_min();
		ss << "\tShortest pulse: ";
		if (shortest_pulse.second != BasePulseMeasurements::NO_VALUE) {
			ss << "&Y" << shortest_pulse.second.first
			   << "&n; duration: &Y" << shortest_pulse.second.second.first
			   << "&n seconds, time &Y" << asctime(localtime(&shortest_pulse.second.second.second));
			ss.seekp(-1, std::ios_base::end);
			ss << "&n; steps:" << "\r\n";
			print_steps(ss, heartbeat, shortest_pulse.first);
		} else {
			ss << "&W<no value>&n" << "\r\n";
		}
	}

	send(context, ss.str());
}

void ShowHeartbeatStats::print_steps(std::ostream &os,
									 const Heartbeat &heartbeat,
									 const Heartbeat::pulse_label_t &label) {
	std::size_t max_width = 0;
	std::size_t max_index = 0;
	std::multimap<double, std::size_t, std::greater<double>> reverse_sorted_label;
	for (const auto &step : label) {
		reverse_sorted_label.emplace(step.second, step.first);
		max_width = std::max(max_width, heartbeat.step_name(step.first).size());
		max_index = std::max(max_index, step.first);
	}

	std::size_t max_index_length = 0;
	while (0 < max_index) {
		max_index /= 10;
		++max_index_length;
	}

	int counter = 0;
	for (const auto &step : reverse_sorted_label) {
		const auto &name = heartbeat.step_name(step.second);
		os << "\t\t&G" << std::setw(max_index_length) << (1 + step.second) << "&n. &W"
		   << name << "&n"
		   << utils::SpacedPadding(name, max_width, (0 == ++counter % 2 ? '.' : ' '))
		   << "&Y" << step.first << "&n seconds."
		   << "\r\n";
	}
}

class CommandsHandler : public commands::AbstractCommandsHanler {
 public:
	virtual void initialize() override;
	virtual void process(CharData *character, char *arguments) override;

 private:
	CommandEmbranchment::shared_ptr m_command;
};

void CommandsHandler::initialize() {
	/*
	* Commands:
	* heartbeat stats general - General heartbeat stats: number of different steps, fastest, slowest pulses
	* -- heartbeat stats pulses - General pulses stats: table of steps with their general stats.
	* heartbeat stats steps <step number> - Stats for each step
	*/
	m_command = CommandEmbranchment::create();

	const auto stats_command = CommandEmbranchment::create("Allows to get access to heartbeat stats.");
	stats_command->add_command("general", std::make_shared<ShowHeartbeatStats>())
		.add_command("step", std::make_shared<ShowStepStats>())
		.add_command("clear", std::make_shared<ClearStats>())
		.add_command("help", std::make_shared<ParentalHelp>(stats_command))
		.rebuild_help();
	m_command->add_command("stats", stats_command)
		.add_command("help", std::make_shared<ParentalHelp>(m_command))
		.rebuild_help();
}

void CommandsHandler::process(CharData *character, char *arguments) {
	AbstractCommand::arguments_t arguments_list(arguments);
	const auto context = std::make_shared<HeartbeatCommandContext>(character, GlobalObjects::heartbeat());
	AbstractCommand::arguments_t path;
	path.push_back(heartbeat::cmd::HEARTBEAT_COMMAND);
	m_command->execute(context, path, arguments_list);
}

const commands::AbstractCommandsHanler::shared_ptr &commands_handler() {
	static commands::HandlerInitializer<CommandsHandler> handler;

	return handler();
}
}

void do_heartbeat(CharData *ch, char *arguments, int /*cmd*/, int /*subcmd*/) {
	commands_handler()->process(ch, arguments);
}

const char *HEARTBEAT_COMMAND = "heartbeat";
}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
