#include "heartbeat.hpp"

#include "auction.h"
#include "deathtrap.hpp"
#include "parcel.hpp"
#include "pk.h"
#include "celebrates.hpp"
#include "fight.h"
#include "help.hpp"
#include "bonus.h"
#include "temp_spells.hpp"
#include "external.trigger.hpp"
#include "house.h"
#include "ban.hpp"
#include "exchange.h"
#include "title.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "file_crc.hpp"
#include "sets_drop.hpp"
#include "mail.h"
#include "mob_stat.hpp"
#include "weather.hpp"
#include "magic.h"
#include "objsave.h"
#include "limits.hpp"
#include "mobact.hpp"
#include "dg_event.h"
#include "corpse.hpp"
#include "char.hpp"
#include "shutdown.parameters.hpp"
#include "logger.hpp"
#include "time_utils.hpp"
#include "structs.h"
#include "global.objects.hpp"

#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>

constexpr bool FRAC_SAVE = true;

void check_idle_passwords(void)
{
	DESCRIPTOR_DATA *d, *next_d;

	for (d = descriptor_list; d; d = next_d)
	{
		next_d = d->next;
		if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME && STATE(d) != CON_GET_KEYTABLE)
			continue;
		if (!d->idle_tics)
		{
			d->idle_tics++;
			continue;
		}
		else
		{
			SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
			STATE(d) = CON_CLOSE;
		}
	}
}

void record_usage(void)
{
	int sockets_connected = 0, sockets_playing = 0;
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d; d = d->next)
	{
		sockets_connected++;
		if (STATE(d) == CON_PLAYING)
			sockets_playing++;
	}

	log("nusage: %-3d sockets connected, %-3d sockets playing", sockets_connected, sockets_playing);

#ifdef RUSAGE			// Not RUSAGE_SELF because it doesn't guarantee prototype.
	{
		struct rusage ru;

		getrusage(RUSAGE_SELF, &ru);
		log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
			ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
	}
#endif
}

// pulse steps
namespace
{
	void process_speedwalks()
	{
		for (auto &sw : GlobalObjects::speedwalks())
		{
			if (sw.wait > sw.route[sw.cur_state].wait)
			{
				for (CHAR_DATA *ch : sw.mobs) {
					if (ch && !ch->purged())
					{
						std::string direction = sw.route[sw.cur_state].direction;
						int dir = 1;
						if (boost::starts_with(direction, "север"))
							dir = SCMD_NORTH;
						if (boost::starts_with(direction, "восток"))
							dir = SCMD_EAST;
						if (boost::starts_with(direction, "юг"))
							dir = SCMD_SOUTH;
						if (boost::starts_with(direction, "запад"))
							dir = SCMD_WEST;
						if (boost::starts_with(direction, "вверх"))
							dir = SCMD_UP;
						if (boost::starts_with(direction, "вниз"))
							dir = SCMD_DOWN;
						perform_move(ch, dir - 1, 0, TRUE, 0);
					}
				}
				sw.wait = 0;
				sw.cur_state = (sw.cur_state >= static_cast<decltype(sw.cur_state)>(sw.route.size()) - 1) ? 0 : sw.cur_state + 1;
			}
			else
			{
				sw.wait += 1;
			}
		}
	}

	class SimpleCall : public AbstractPulseAction
	{
	public:
		using call_t = std::function<void()>;

		SimpleCall(call_t call): m_call(call) {}

		virtual void perform(int, int) override { m_call(); }

	private:
		call_t m_call;
	};

	class MobActCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int pulse_number, int) override { mobile_activity(pulse_number, 10); }
	};

	class InspectCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int missed_pulses) override;
	};

	void InspectCall::perform(int, int missed_pulses)
	{
		if (0 == missed_pulses
			&& 0 < inspect_list.size())
		{
			inspecting();
		}
	}

	class SetAllInspectCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int missed_pulses) override;
	};

	void SetAllInspectCall::perform(int, int missed_pulses)
	{
		if (0 == missed_pulses
			&& 0 < setall_inspect_list.size())
		{
			setall_inspect();
		}
	}

	class CheckScheduledRebootCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override;
	};

	void CheckScheduledRebootCall::perform(int, int)
	{
		const auto boot_time = GlobalObjects::shutdown_parameters().get_boot_time();
		const auto uptime_minutes = ((time(NULL) - boot_time) / 60);

		if (uptime_minutes >= (shutdown_parameters.get_reboot_uptime() - 30)
			&& shutdown_parameters.get_shutdown_timeout() == 0)
		{
			//reboot after 30 minutes minimum. Auto reboot cannot run earlier.
			send_to_all("АВТОМАТИЧЕСКАЯ ПЕРЕЗАГРУЗКА ЧЕРЕЗ 30 МИНУТ.\r\n");
			shutdown_parameters.reboot(1800);
		}
	}

	class CheckTriggeredRebootCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override;

	private:
		std::unique_ptr<ExternalTriggerChecker> m_external_trigger_checker;
	};

	void CheckTriggeredRebootCall::perform(int, int)
	{
		if (!m_external_trigger_checker)
		{
			m_external_trigger_checker = std::make_unique<ExternalTriggerChecker>(runtime_config.external_reboot_trigger_file_name());
		}

		if (m_external_trigger_checker
			&& m_external_trigger_checker->check())
		{
			mudlog("Сработал внешний триггер перезагрузки.", DEF, LVL_IMPL, SYSLOG, true);
			shutdown_parameters.reboot();
		}
	}

	class BeatPointsUpdateCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int pulse_number, int) override { beat_points_update(pulse_number / PASSES_PER_SEC); }
	};

	class CrashFracSaveCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int pulse_number, int) override;
	};

	void CrashFracSaveCall::perform(int pulse_number, int)
	{
		if (FRAC_SAVE && AUTO_SAVE)
		{
			Crash_frac_save_all((pulse_number / PASSES_PER_SEC) % PLAYER_SAVE_ACTIVITY);
			Crash_frac_rent_time((pulse_number / PASSES_PER_SEC) % OBJECT_SAVE_ACTIVITY);
		}
	}

	class ExchangeDatabaseSaveCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override;
	};

	void ExchangeDatabaseSaveCall::perform(int, int)
	{
		if (EXCHANGE_AUTOSAVETIME && AUTO_SAVE)
		{
			exchange_database_save();
		}
	}

	class ExchangeDatabaseBackupSaveCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override;
	};

	void ExchangeDatabaseBackupSaveCall::perform(int, int)
	{
		if (EXCHANGE_AUTOSAVEBACKUPTIME)
		{
			exchange_database_save(true);
		}
	}

	class GlobalSaveUIDCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override;
	};

	void GlobalSaveUIDCall::perform(int, int)
	{
		if (AUTO_SAVE)
		{
			SaveGlobalUID();
		}
	}

	class CrashSaveCall : public AbstractPulseAction
	{
	public:
		CrashSaveCall();

		virtual void perform(int, int) override;

	private:
		int m_mins_since_crashsave;
		long m_last_rent_check;	// at what time checked rented time
	};

	CrashSaveCall::CrashSaveCall() :
		m_mins_since_crashsave(0),
		m_last_rent_check(time(NULL))
	{
	}

	void CrashSaveCall::perform(int, int)
	{
		if (FRAC_SAVE || !AUTO_SAVE)
		{
			return;
		}

		if (++m_mins_since_crashsave < AUTOSAVE_TIME)
		{
			return;
		}

		m_mins_since_crashsave = 0;
		Crash_save_all();
		const auto check_at = time(NULL);

		if (m_last_rent_check > check_at)
		{
			m_last_rent_check = check_at;
		}

		if (((check_at - m_last_rent_check) / 60))
		{
			Crash_rent_time((check_at - m_last_rent_check) / 60);
			m_last_rent_check = time(NULL) - (check_at - m_last_rent_check) % 60;
		}
	}

	class UpdateClanExpCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override;
	};

	void UpdateClanExpCall::perform(int, int)
	{
		update_clan_exp();
		save_clan_exp();
	}

	class SpellUsageCall : public AbstractPulseAction
	{
	public:
		virtual void perform(int, int) override
		{
			if (!SpellUsage::isActive)
			{
				return;
			}

			time_t tmp_time = time(0);
			if ((tmp_time - SpellUsage::start) >= (60 * 60 * 24))
			{
				SpellUsage::save();
				SpellUsage::clear();
			}
		}
	};

	Heartbeat::steps_t& pulse_steps()
	{
		static Heartbeat::steps_t pulse_steps_storage = {
			Heartbeat::PulseStep("Speed walks processing", PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(process_speedwalks)),
			Heartbeat::PulseStep("Global drop: tables reloading", PASSES_PER_SEC * 120 * 60, 0, std::make_shared<SimpleCall>(GlobalDrop::reload_tables)),
			Heartbeat::PulseStep("Events processing", 1, 0, std::make_shared<SimpleCall>(process_events)),
			Heartbeat::PulseStep("Triggers check", PULSE_DG_SCRIPT, 1, std::make_shared<SimpleCall>(script_trigger_check)),
			Heartbeat::PulseStep("Sanity check", 60 * PASSES_PER_SEC, 2, std::make_shared<SimpleCall>(sanity_check)),
			Heartbeat::PulseStep("Check idle passwords", 40 * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(check_idle_passwords)),
			Heartbeat::PulseStep("Mobile activity", 10, 0, std::make_shared<MobActCall>()),
			Heartbeat::PulseStep("Inspecting", 1, 0, std::make_shared<InspectCall>()),
			Heartbeat::PulseStep("Set all inspecting", 1, 0, std::make_shared<SetAllInspectCall>()),
			Heartbeat::PulseStep("Death trap activity", 2 * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(DeathTrap::activity)),
			Heartbeat::PulseStep("Underwater check", 2 * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(underwater_check)),
			Heartbeat::PulseStep("Clan system: check player in house", 2 * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(ClanSystem::check_player_in_house)),
			Heartbeat::PulseStep("Violence performing", PULSE_VIOLENCE, 3, std::make_shared<SimpleCall>(perform_violence)),
			Heartbeat::PulseStep("Scheduled reboot checking", 30 * PASSES_PER_SEC, 0, std::make_shared<CheckScheduledRebootCall>()),
			Heartbeat::PulseStep("Check of reboot trigger", PASSES_PER_SEC, 0, std::make_shared<CheckTriggeredRebootCall>()),
			Heartbeat::PulseStep("Auction update", AUCTION_PULSES * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(tact_auction)),
			Heartbeat::PulseStep("Room affect update", SECS_PER_ROOM_AFFECT * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(RoomSpells::room_affect_update)),
			Heartbeat::PulseStep("Player affect update", SECS_PER_PLAYER_AFFECT * PASSES_PER_SEC, 0, std::make_shared<SimpleCall>(player_affect_update)),
			Heartbeat::PulseStep("Hour update", TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC, PASSES_PER_SEC - 4, std::make_shared<SimpleCall>(hour_update)),
			Heartbeat::PulseStep("Timer bonus", TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC, PASSES_PER_SEC - 3, std::make_shared<SimpleCall>(Bonus::timer_bonus)),
			Heartbeat::PulseStep("Weather and time", TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC, PASSES_PER_SEC - 2, std::make_shared<SimpleCall>([]() { weather_and_time(1); })),
			Heartbeat::PulseStep("Paste mobiles", TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC, PASSES_PER_SEC - 1, std::make_shared<SimpleCall>(paste_mobiles)),
			Heartbeat::PulseStep("Zone update", PULSE_ZONE, 5, std::make_shared<SimpleCall>(zone_update)),
			Heartbeat::PulseStep("Money drop stat: print log", 60 * 60 * PASSES_PER_SEC, 49, std::make_shared<SimpleCall>(MoneyDropStat::print_log)),
			Heartbeat::PulseStep("Zone exp stat: print log", 60 * 60 * PASSES_PER_SEC, 49, std::make_shared<SimpleCall>(ZoneExpStat::print_log)),
			Heartbeat::PulseStep("Print rune log", 60 * 60 * PASSES_PER_SEC, 49, std::make_shared<SimpleCall>(print_rune_log)),
			Heartbeat::PulseStep("Mob stats saving", 60 * mob_stat::SAVE_PERIOD * PASSES_PER_SEC, 57, std::make_shared<SimpleCall>(mob_stat::save)),
			Heartbeat::PulseStep("Sets drop table saving", 60 * SetsDrop::SAVE_PERIOD * PASSES_PER_SEC, 52, std::make_shared<SimpleCall>(SetsDrop::save_drop_table)),
			Heartbeat::PulseStep("Clan system: chest log saving", 60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC, 50, std::make_shared<SimpleCall>(ClanSystem::save_chest_log)),
			Heartbeat::PulseStep("Clan system: ingredients chests saving", 60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC, 48, std::make_shared<SimpleCall>(ClanSystem::save_ingr_chests)),
			Heartbeat::PulseStep("Global drop: saving", 60 * GlobalDrop::SAVE_PERIOD * PASSES_PER_SEC, 47, std::make_shared<SimpleCall>(GlobalDrop::save)),
			Heartbeat::PulseStep("Clan: chest update", 60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC, 46, std::make_shared<SimpleCall>(Clan::ChestUpdate)),
			Heartbeat::PulseStep("Clan: save chest all", 60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC, 44, std::make_shared<SimpleCall>(Clan::SaveChestAll)),
			Heartbeat::PulseStep("Clan: clan save", 60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC, 40, std::make_shared<SimpleCall>(Clan::ClanSave)),
			Heartbeat::PulseStep("Celebrates: sanitize", Celebrates::CLEAN_PERIOD * 60 * PASSES_PER_SEC, 39, std::make_shared<SimpleCall>(Celebrates::sanitize)),
			Heartbeat::PulseStep("Record usage", 5 * 60 * PASSES_PER_SEC, 37, std::make_shared<SimpleCall>(record_usage)),
			Heartbeat::PulseStep("Reload proxy ban", 5 * 60 * PASSES_PER_SEC, 36, std::make_shared<SimpleCall>([]() { ban->reload_proxy_ban(ban->RELOAD_MODE_TMPFILE); })),
			Heartbeat::PulseStep("God work invoice", 5 * 60 * PASSES_PER_SEC, 35, std::make_shared<SimpleCall>(god_work_invoice)),
			Heartbeat::PulseStep("Title system: title list saving", 5 * 60 * PASSES_PER_SEC, 34, std::make_shared<SimpleCall>(TitleSystem::save_title_list)),
			Heartbeat::PulseStep("Register system: save", 5 * 60 * PASSES_PER_SEC, 33, std::make_shared<SimpleCall>(RegisterSystem::save)),
			Heartbeat::PulseStep("Mail: save", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 32, std::make_shared<SimpleCall>(mail::save)),
			Heartbeat::PulseStep("Help system: update dynamic checking", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 31, std::make_shared<SimpleCall>(HelpSystem::check_update_dynamic)),
			Heartbeat::PulseStep("Sets drop: reload by timer", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 30, std::make_shared<SimpleCall>(SetsDrop::reload_by_timer)),
			Heartbeat::PulseStep("Clan: save PK log", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 29, std::make_shared<SimpleCall>(Clan::save_pk_log)),
			Heartbeat::PulseStep("Characters purging", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 28, std::make_shared<SimpleCall>([]() { character_list.purge(); })),
			Heartbeat::PulseStep("Objects purging", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 27, std::make_shared<SimpleCall>([]() { world_objects.purge(); })),
			Heartbeat::PulseStep("Depot: timers updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 25, std::make_shared<SimpleCall>(Depot::update_timers)),
			Heartbeat::PulseStep("Parcel: timers updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 24, std::make_shared<SimpleCall>(Parcel::update_timers)),
			Heartbeat::PulseStep("Glory: timers updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 23, std::make_shared<SimpleCall>(Glory::timers_update)),
			Heartbeat::PulseStep("Glory: saving", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 22, std::make_shared<SimpleCall>(Glory::save_glory)),
			Heartbeat::PulseStep("Depot: saving all online objects", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 21, std::make_shared<SimpleCall>(Depot::save_all_online_objs)),
			Heartbeat::PulseStep("Depot: time data saving", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 17, std::make_shared<SimpleCall>(Depot::save_timedata)),
			Heartbeat::PulseStep("Mobile affects updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 16, std::make_shared<SimpleCall>(mobile_affect_update)),
			Heartbeat::PulseStep("Objects point updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 11, std::make_shared<SimpleCall>(obj_point_update)),
			Heartbeat::PulseStep("Bloody: updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 10, std::make_shared<SimpleCall>(bloody::update)),
			Heartbeat::PulseStep("Room point updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 6, std::make_shared<SimpleCall>(room_point_update)),
			Heartbeat::PulseStep("Temporary spells: times updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 5, std::make_shared<SimpleCall>(Temporary_Spells::update_times)),
			Heartbeat::PulseStep("Exchange point updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 2, std::make_shared<SimpleCall>(exchange_point_update)),
			Heartbeat::PulseStep("Players index flushing", SECS_PER_MUD_HOUR * PASSES_PER_SEC, 1, std::make_shared<SimpleCall>(flush_player_index)),
			Heartbeat::PulseStep("Point updating", SECS_PER_MUD_HOUR * PASSES_PER_SEC, PASSES_PER_SEC - 5, std::make_shared<SimpleCall>(point_update)),
			Heartbeat::PulseStep("Beat points updating", PASSES_PER_SEC, 0, std::make_shared<BeatPointsUpdateCall>()),
#if defined WITH_SCRIPTING
			Heartbeat::PulseStep("Scripting: heartbeat", scripting::HEARTBEAT_PASSES, 0, std::make_shared<SimpleCall>(scripting::heartbeat)),
#endif
			Heartbeat::PulseStep("Crash frac save", PASSES_PER_SEC, 7, std::make_shared<CrashFracSaveCall>()),
			Heartbeat::PulseStep("Exchange database save", EXCHANGE_AUTOSAVETIME * PASSES_PER_SEC, 9, std::make_shared<ExchangeDatabaseSaveCall>()),
			Heartbeat::PulseStep("Exchange database backup save", EXCHANGE_AUTOSAVETIME * PASSES_PER_SEC, 9, std::make_shared<ExchangeDatabaseBackupSaveCall>()),
			Heartbeat::PulseStep("Global UID saving", 60 * PASSES_PER_SEC, 9, std::make_shared<GlobalSaveUIDCall>()),
			Heartbeat::PulseStep("Crash save", 60 * PASSES_PER_SEC, 11, std::make_shared<CrashSaveCall>()),
			Heartbeat::PulseStep("Clan experience updating", 60 * CLAN_EXP_UPDATE_PERIOD * PASSES_PER_SEC, 14, std::make_shared<UpdateClanExpCall>()),
			Heartbeat::PulseStep("Clan: chest invoice", 60 * CHEST_INVOICE_PERIOD * PASSES_PER_SEC, 15, std::make_shared<SimpleCall>(Clan::ChestInvoice)),
			Heartbeat::PulseStep("Gifts", 60 * PASSES_PER_SEC, 18, std::make_shared<SimpleCall>(gifts)),
			Heartbeat::PulseStep("File CRC: saving", PASSES_PER_SEC, 23, std::make_shared<SimpleCall>([]() { FileCRC::save(false); })),
			Heartbeat::PulseStep("Spells usage saving", 60 * 60 * PASSES_PER_SEC, 0, std::make_shared<SpellUsageCall>())
		};

		return pulse_steps_storage;
	}
}

long long NOD(long long a, long long b)
{
	if (a > b)
	{
		std::swap(a, b);
	}

	for (;0 != a; std::swap(a, b))
	{
		b = b % a;
	}

	return b;
}

long long NOK(long long a, long long b)
{
	return a * b / NOD(a, b);
}

Heartbeat::Heartbeat() :
	m_steps(pulse_steps()),
	m_pulse_number(0),
	m_global_pulse_number(0)
{
}

constexpr long long Heartbeat::ROLL_OVER_AFTER;

void Heartbeat::operator()(const int missed_pulses)
{
	pulse_label_t label;

	utils::CExecutionTimer timer;
	pulse(missed_pulses, label);
	const auto execution_time = timer.delta();
	if (PASSES_PER_SEC / 60.0 < execution_time.count())
	{
		log("SYSERR: Long-running tick #%d worked for %.4f seconds (missed pulses argument has value %d)",
			pulse_number(), execution_time.count(), missed_pulses);
	}
	m_measurements.add(label, pulse_number(), execution_time.count());
	if (GlobalObjects::stats_sender().ready())
	{
		influxdb::Record record("heartbeat");
		record.add_tag("pulse", pulse_number());
		record.add_field("duration", execution_time.count());
		GlobalObjects::stats_sender().send(record);
	}
}

long long Heartbeat::period() const
{
	long long period = 1;
	for (const auto& step : m_steps)
	{
		if (step.on())
		{
			period = NOK(period, step.modulo());
		}
	}

	return period;
}

void Heartbeat::advance_pulse_numbers()
{
	m_pulse_number++;
	m_global_pulse_number++;

	// Roll pulse over after 10 hours
	if (m_pulse_number >= ROLL_OVER_AFTER)
	{
		m_pulse_number = 0;
	}
}

void Heartbeat::pulse(const int missed_pulses, pulse_label_t& label)
{
	label.clear();

	advance_pulse_numbers();

	for (std::size_t i = 0; i != m_steps.size(); ++i)
	{
		auto& step = m_steps[i];

		if (step.off())
		{
			continue;
		}

		if (0 == (m_pulse_number + step.offset()) % step.modulo())
		{
			utils::CExecutionTimer timer;

			step.action()->perform(pulse_number(), missed_pulses);
			const auto execution_time = timer.delta().count();

			label.emplace(i, execution_time);
			m_executed_steps.insert(i);
			step.add_measurement(i, pulse_number(), execution_time);
		}
	}
}

Heartbeat::PulseStep::PulseStep(const std::string& name, const int modulo, const int offset, const pulse_action_t& action) :
	m_name(name),
	m_modulo(modulo),
	m_offset(offset),
	m_action(action),
	m_off(false)
{
}

void Heartbeat::PulseStep::add_measurement(const std::size_t index, const pulse_t pulse, const BasePulseMeasurements::value_t value)
{
	m_measurements.add(index, pulse, value);
}

BasePulseMeasurements::BasePulseMeasurements():
	m_sum(0.0),
	m_global_sum(0.0),
	m_global_count(0)
{
}

void BasePulseMeasurements::add(const measurement_t& measurement)
{
	add_measurement(measurement);
	squeeze();
}

void BasePulseMeasurements::add_measurement(const measurement_t& measurement)
{
	m_measurements.push_front(measurement);
	m_sum += measurement.second;
	m_global_sum += measurement.second;
	++m_global_count;

	m_min.insert(measurement);
	m_max.insert(measurement);
}

void BasePulseMeasurements::squeeze()
{
	while (m_measurements.size() > window_size())
	{
		const auto& last_value = m_measurements.back();

		remove_handler(last_value.first);

		const auto min_i = m_min.find(last_value);
		m_min.erase(min_i);

		const auto max_i = m_max.find(last_value);
		m_max.erase(max_i);

		m_sum -= last_value.second;

		m_measurements.pop_back();
	}
}

constexpr std::size_t BasePulseMeasurements::WINDOW_SIZE;
constexpr BasePulseMeasurements::measurement_t BasePulseMeasurements::NO_VALUE;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
