#ifndef __HEARTBEAT_HPP__
#define __HEARTBEAT_HPP__

#include <functional>
#include <queue>
#include <list>
#include <memory>
#include <set>

class AbstractPulseAction
{
public:
	using shared_ptr = std::shared_ptr<AbstractPulseAction>;

	virtual ~AbstractPulseAction() {}

	virtual void perform(int pulse_number, int missed_pulses) = 0;
};

using pulse_t = int;

class PulseMeasurements
{
public:
	using value_t = double;
	using measurement_t = std::pair<pulse_t, value_t>;	// pulse -> longtitude

	PulseMeasurements();

	void add(const measurement_t& measurement);

private:
	class Min
	{
	public:
		bool operator()(const measurement_t& a, const measurement_t& b) const { return a.second < b.second; }
	};

	class Max
	{
	public:
		bool operator()(const measurement_t& a, const measurement_t& b) const { return a.second > b.second; }
	};

	using measurements_t = std::list<measurement_t>;
	using min_t = std::multiset<measurement_t, Min>;
	using max_t = std::multiset<measurement_t, Max>;

	static constexpr std::size_t WINDOW_SIZE = 100;
	static constexpr measurement_t NO_VALUE = measurement_t(-1, -1);

	measurements_t m_measurements;
	double m_sum;
	min_t m_min;
	max_t m_max;
	measurement_t m_global_min;
	measurement_t m_global_max;
};

class Heartbeat
{
public:
	using pulse_action_t = AbstractPulseAction::shared_ptr;

	class PulseStep
	{
	public:
		PulseStep(const std::string& name, const int modulo, const int offset, const pulse_action_t& action);

		const auto& name() const { return m_name; }
		const auto modulo() const { return m_modulo; }
		const auto offset() const { return m_offset; }
		const auto& action() const { return m_action; }
		const auto off() const { return m_off; }
		const auto on() const { return !off(); }

		void turn_on() { m_off = false; }
		void turn_off() { m_off = true; }

		void add_measurement(const pulse_t pulse, const PulseMeasurements::value_t value);

	private:
		std::string m_name;
		int m_modulo;
		int m_offset;
		pulse_action_t m_action;

		bool m_off;
		PulseMeasurements m_measurements;
	};

	using steps_t = std::list<PulseStep>;

	Heartbeat();

	void operator()(const int missed_pulses);

	auto pulse_number() const { return m_pulse_number; }
	auto global_pulse_number() const { return m_global_pulse_number; }
	long long period() const;

private:
	void advance_pulse_numbers();
	void pulse(const int missed_pulses);

	steps_t m_steps;
	pulse_t m_pulse_number;
	unsigned long m_global_pulse_number;	// number of pulses since game start
};

extern Heartbeat& heartbeat;

#endif // __HEARTBEAT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
