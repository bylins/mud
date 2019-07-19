#ifndef __HEARTBEAT_HPP__
#define __HEARTBEAT_HPP__

#include "structs.h"

#include <functional>
#include <queue>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>

class AbstractPulseAction
{
public:
	using shared_ptr = std::shared_ptr<AbstractPulseAction>;

	virtual ~AbstractPulseAction() {}

	virtual void perform(int pulse_number, int missed_pulses) = 0;
};

using pulse_t = int;

class BasePulseMeasurements
{
public:
	using value_t = double;
	using measurement_t = std::pair<pulse_t, value_t>;

	static constexpr measurement_t NO_VALUE = measurement_t(-1, -1);

	BasePulseMeasurements();
	virtual ~BasePulseMeasurements() {}

	const auto window_size() const { return WINDOW_SIZE; }

	const auto window_sum() const { return m_sum; }
	const auto current_window_size() const { return m_measurements.size(); }

	const auto global_sum() const { return m_global_sum; }
	const auto global_count() const { return m_global_count; }

	const auto& min() const { return m_min; }
	const auto& max() const { return m_max; }

	void add(const measurement_t& measurement);

protected:
	virtual void remove_handler(const pulse_t) { /* do nothing by default */ }

private:
	using measurements_t = std::list<measurement_t>;

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

	using min_t = std::multiset<measurement_t, Min>;
	using max_t = std::multiset<measurement_t, Max>;

	static constexpr std::size_t WINDOW_SIZE = 100;

	void add_measurement(const measurement_t& measurement);
	void squeeze();

	measurements_t m_measurements;

	min_t m_min;
	max_t m_max;

	value_t m_sum;

	value_t m_global_sum;
	std::size_t m_global_count;
};

template <typename Label>
class LabelledMeasurements: public BasePulseMeasurements
{
public:
	using single_value_t = std::pair<Label, measurement_t>;

	LabelledMeasurements() :
		m_global_min(Label(), NO_VALUE),
		m_global_max(Label(), NO_VALUE)
	{
	}

	void add(const Label& label, const pulse_t pulse, const value_t value) { add(label, measurement_t(pulse, value)); }
	void add(const Label& label, const measurement_t measurement);

	const auto& global_min() const { return m_global_min; }
	const auto& global_max() const { return m_global_max; }

protected:
	using BasePulseMeasurements::add;

private:
	using pulse_to_label_t = std::unordered_map<pulse_t, Label>;

	virtual void remove_handler(const pulse_t pulse) override;

	pulse_to_label_t m_labels;

	single_value_t m_global_min;
	single_value_t m_global_max;
};

template <typename Label>
void LabelledMeasurements<Label>::add(const Label& label, const measurement_t measurement)
{
	BasePulseMeasurements::add(measurement);
	m_labels.emplace(measurement.first, label);

	if (m_global_min.second.second > measurement.second
		|| m_global_min.second == NO_VALUE)
	{
		m_global_min = std::move(single_value_t(label, measurement));
	}

	if (m_global_max.second.second < measurement.second
		|| m_global_max.second == NO_VALUE)
	{
		m_global_max = std::move(single_value_t(label, measurement));
	}
}

template <typename Label>
void LabelledMeasurements<Label>::remove_handler(const pulse_t pulse)
{
    if (!m_labels.empty())
    {
        m_labels.erase(pulse);
    }
}

class Heartbeat
{
public:
	static constexpr long long ROLL_OVER_AFTER = 600 * 60 * PASSES_PER_SEC;

	using pulse_action_t = AbstractPulseAction::shared_ptr;

	class PulseStep
	{
	public:
		using StepMeasurement = LabelledMeasurements<std::size_t>;	// Label is the index in steps array

		PulseStep(const std::string& name, const int modulo, const int offset, const pulse_action_t& action);

		const auto& name() const { return m_name; }
		const auto modulo() const { return m_modulo; }
		const auto offset() const { return m_offset; }
		const auto& action() const { return m_action; }
		const auto off() const { return m_off; }
		const auto on() const { return !off(); }

		void turn_on() { m_off = false; }
		void turn_off() { m_off = true; }

		void add_measurement(const std::size_t index, const pulse_t pulse, const StepMeasurement::value_t value);

		const auto& stats() const { return m_measurements; }

	private:
		std::string m_name;
		int m_modulo;
		int m_offset;
		pulse_action_t m_action;

		bool m_off;
		StepMeasurement m_measurements;
	};

	using steps_t = std::vector<PulseStep>;
	using pulse_label_t = std::unordered_map<std::size_t, double>;
	using PulseMeasurement = LabelledMeasurements<pulse_label_t>;

	Heartbeat();

	void operator()(const int missed_pulses);

	auto pulse_number() const { return m_pulse_number; }
	auto global_pulse_number() const { return m_global_pulse_number; }
	long long period() const;

	const auto& stats() const { return m_measurements; }
	const auto& steps() const { return m_steps; }
	const auto& executed_steps() const { return m_executed_steps; }

	const std::string& step_name(const std::size_t index) const { return m_steps[index].name(); }

private:
	using steps_set_t = std::unordered_set<std::size_t>;

	void advance_pulse_numbers();
	void pulse(const int missed_pulses, pulse_label_t& label);

	steps_t m_steps;
	pulse_t m_pulse_number;
	unsigned long m_global_pulse_number;	// number of pulses since game start

	steps_set_t m_executed_steps;	// set of step indexes ever executed
	PulseMeasurement m_measurements;
};

#endif // __HEARTBEAT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
