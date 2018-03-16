#ifndef __HEARTBEAT_HPP__
#define __HEARTBEAT_HPP__

#include <functional>
#include <queue>
#include <list>
#include <memory>

class AbstractPulseAction
{
public:
	using shared_ptr = std::shared_ptr<AbstractPulseAction>;

	virtual ~AbstractPulseAction() {}

	virtual void perform(int pulse_number, int missed_pulses) = 0;
};

class Heartbeat
{
public:
	using pulse_action_t = AbstractPulseAction::shared_ptr;

	struct PulseStep
	{
		PulseStep(const std::string& name, const int modulo, const int offset, const pulse_action_t& action);

		std::string name;
		int modulo;
		int offset;
		pulse_action_t action;
	};

	using steps_t = std::list<PulseStep>;

	Heartbeat();

	void operator()(const int missed_pulses);

	auto pulse_number() const { return m_pulse_number; }
	auto global_pulse_number() const { return m_global_pulse_number; }

private:
	using pulse_t = int;

	void advance_pulses_number();
	void pulse(const int missed_pulses);

	steps_t m_steps;
	pulse_t m_pulse_number;
	unsigned long m_global_pulse_number;	// number of pulses since game start
};

extern Heartbeat& heartbeat;

#endif // __HEARTBEAT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
