#ifndef __HEARTBEAT_HPP__
#define __HEARTBEAT_HPP__

class Heartbeat
{
public:
	Heartbeat();
	~Heartbeat();

	void operator()(const int missed_pulses);

	auto pulse_number() const { return m_pulse_number; }
	auto global_pulse_number() const { return m_global_pulse_number; }
	void reset_last_rent_check();

private:
	void pulse(const int missed_pulses);
	void check_external_reboot_trigget();

	int m_mins_since_crashsave;
	int m_pulse_number;
	unsigned long m_global_pulse_number;	// number of pulses since game start
	long m_last_rent_check;	// at what time checked rented time
	class ExternalTriggerChecker* m_external_trigger_checker;	// "class" to avoid inclusion of external.trigger.hpp
};

extern Heartbeat& heartbeat;

#endif // __HEARTBEAT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
