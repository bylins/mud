//#include "heartbeat.h"

#include <utility>

#include "administration/proxy.h"
#include "gameplay/economics/auction.h"
#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/fight/fight.h"
#include "engine/db/help.h"
#include "gameplay/mechanics/bonus.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "external_trigger.h"
#include "gameplay/clans/house.h"
#include "administration/ban.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/mechanics/title.h"
#include "gameplay/mechanics/glory.h"
#include "utils/file_crc.h"
#include "gameplay/mechanics/sets_drop.h"
#include "gameplay/communication/mail.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/magic/magic.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/ai/mobact.h"
#include "engine/scripting/dg_event.h"
#include "gameplay/mechanics/corpse.h"
#include "engine/db/global_objects.h"
#include "engine/ui/cmd_god/do_set_all.h"
#include "gameplay/statistics/money_drop.h"
#include "gameplay/mechanics/weather.h"
#include "utils/utils_time.h"
#include "gameplay/statistics/zone_exp.h"
#include "gameplay/communication/check_invoice.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/statistics/spell_usage.h"

#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif

extern std::pair<int, int> TotalMemUse();

constexpr bool FRAC_SAVE = true;

void check_idle_passwords() {
	DescriptorData *d, *next_d;

	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (d->state != EConState::kPassword && d->state != EConState::kGetName && d->state != EConState::kGetKeytable)
			continue;
		if (!d->idle_tics) {
			d->idle_tics++;
			continue;
		} else {
			iosystem::write_to_output("\r\nTimed out... goodbye.\r\n", d);
			d->state = EConState::kClose;
		}
	}
}

void record_usage() {
	int sockets_connected = 0, sockets_playing = 0;
	DescriptorData *d;

	for (d = descriptor_list; d; d = d->next) {
		sockets_connected++;
		if (d->state == EConState::kPlaying)
			sockets_playing++;
	}

	log("nusage: %-3d sockets connected, %-3d sockets playing", sockets_connected, sockets_playing);

#ifdef RUSAGE            // Not RUSAGE_SELF because it doesn't guarantee prototype.
	{
		struct rusage ru;

		getrusage(RUSAGE_SELF, &ru);
		log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
			ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
	}
#endif
}

// pulse steps
namespace {
class SimpleCall : public AbstractPulseAction {
 public:
	using call_t = std::function<void()>;

	explicit SimpleCall(call_t call) : m_call(std::move(call)) {}

	void perform(int, int) override { m_call(); }

 private:
	call_t m_call;
};

class MobActCall : public AbstractPulseAction {
 public:
	void perform(int pulse_number, int) override { mob_ai::mobile_activity(pulse_number, 10); }
};

class InspectCall : public AbstractPulseAction {
 public:
	void perform(int, int missed_pulses) override;
};

void InspectCall::perform(int, int missed_pulses) {
	if (0 == missed_pulses) {
		MUD::InspectRequests().Inspecting();
	}
}

class SetAllInspectCall : public AbstractPulseAction {
 public:
	void perform(int, int missed_pulses) override;
};

void SetAllInspectCall::perform(int, int missed_pulses) {
	if (0 == missed_pulses && !setall_inspect_list.empty()) {
		setall_inspect();
	}
}

class CheckScheduledRebootCall : public AbstractPulseAction {
 public:
	void perform(int, int) override;
};

void CheckScheduledRebootCall::perform(int, int) {
	const auto boot_time = GlobalObjects::shutdown_parameters().get_boot_time();
	const auto uptime_minutes = ((time(nullptr) - boot_time) / 60);

	if (uptime_minutes >= (shutdown_parameters.get_reboot_uptime() - 60)
		&& shutdown_parameters.get_shutdown_timeout() == 0) {
		//reboot after 60 minutes minimum. Auto reboot cannot run earlier.
		SendMsgToAll("АВТОМАТИЧЕСКАЯ ПЕРЕЗАГРУЗКА ЧЕРЕЗ 60 МИНУТ.\r\n");
		shutdown_parameters.reboot(3600);
	}
}

class CheckTriggeredRebootCall : public AbstractPulseAction {
 public:
	void perform(int, int) override;

 private:
	std::unique_ptr<ExternalTriggerChecker> m_external_trigger_checker;
};

void CheckTriggeredRebootCall::perform(int, int) {
	if (!m_external_trigger_checker) {
		m_external_trigger_checker =
			std::make_unique<ExternalTriggerChecker>(runtime_config.external_reboot_trigger_file_name());
	}

	if (m_external_trigger_checker
		&& m_external_trigger_checker->check()) {
		mudlog("Сработал внешний триггер перезагрузки.", DEF, kLvlImplementator, SYSLOG, true);
		shutdown_parameters.reboot();
	}
}

class BeatPointsUpdateCall : public AbstractPulseAction {
 public:
	void perform(int pulse_number, int) override { beat_points_update(pulse_number / kPassesPerSec); }
};

class CrashFracSaveCall : public AbstractPulseAction {
 public:
	void perform(int pulse_number, int) override;
};

void CrashFracSaveCall::perform(int pulse_number, int) {
	if (FRAC_SAVE && AUTO_SAVE) {
		Crash_frac_save_all((pulse_number / kPassesPerSec) % kPlayerSaveActivity);
		Crash_frac_rent_time((pulse_number / kPassesPerSec) % kObjectSaveActivity);
	}
}

class ExchangeDatabaseSaveCall : public AbstractPulseAction {
 public:
	void perform(int, int) override;
};

void ExchangeDatabaseSaveCall::perform(int, int) {
	if (EXCHANGE_AUTOSAVETIME && AUTO_SAVE) {
		exchange_database_save();
	}
}

class ExchangeDatabaseBackupSaveCall : public AbstractPulseAction {
 public:
	void perform(int, int) override;
};

void ExchangeDatabaseBackupSaveCall::perform(int, int) {
	if (EXCHANGE_AUTOSAVEBACKUPTIME) {
		exchange_database_save(true);
	}
}

class GlobalSaveUIDCall : public AbstractPulseAction {
 public:
	void perform(int, int) override;
};

void GlobalSaveUIDCall::perform(int, int) {
	if (AUTO_SAVE) {
		SaveGlobalUID();
	}
}

class CrashSaveCall : public AbstractPulseAction {
 public:
	CrashSaveCall();

	void perform(int, int) override;

 private:
	int m_mins_since_crashsave;
	long m_last_rent_check;    // at what time checked rented time
};

CrashSaveCall::CrashSaveCall() :
	m_mins_since_crashsave(0),
	m_last_rent_check(time(nullptr)) {
}

void CrashSaveCall::perform(int, int) {
	if (FRAC_SAVE || !AUTO_SAVE) {
		return;
	}

	if (++m_mins_since_crashsave < AUTOSAVE_TIME) {
		return;
	}

	m_mins_since_crashsave = 0;
	Crash_save_all();
	const auto check_at = time(nullptr);

	if (m_last_rent_check > check_at) {
		m_last_rent_check = check_at;
	}

	if (((check_at - m_last_rent_check) / 60)) {
		Crash_rent_time((check_at - m_last_rent_check) / 60);
		m_last_rent_check = time(nullptr) - (check_at - m_last_rent_check) % 60;
	}
}

class UpdateClanExpCall : public AbstractPulseAction {
 public:
	void perform(int, int) override;
};

void UpdateClanExpCall::perform(int, int) {
	update_clan_exp();
	save_clan_exp();
}

class SpellUsageCall : public AbstractPulseAction {
 public:
	void perform(int, int) override {
		if (!SpellUsage::is_active) {
			return;
		}

		time_t tmp_time = time(nullptr);
		if ((tmp_time - SpellUsage::start) >= (60 * 60 * 24)) {
			SpellUsage::Save();
			SpellUsage::Clear();
		}
	}
};

Heartbeat::steps_t &pulse_steps() {
	static Heartbeat::steps_t pulse_steps_storage = {
		Heartbeat::PulseStep("Global drop: tables reloading",
							 kPassesPerSec * 120 * 60,
							 0,
							 std::make_shared<SimpleCall>(GlobalDrop::reload_tables)),
		Heartbeat::PulseStep("Events processing", 1, 0, std::make_shared<SimpleCall>(process_events)),
		Heartbeat::PulseStep("Triggers check mobile", 
							PULSE_DG_SCRIPT, 
							1, 
							std::make_shared<SimpleCall>([]() { script_trigger_check(MOB_TRIGGER); })),
		Heartbeat::PulseStep("Triggers check object", 
							PULSE_DG_SCRIPT, 
							5, 
							std::make_shared<SimpleCall>([]() { script_trigger_check(OBJ_TRIGGER); })),
		Heartbeat::PulseStep("Triggers check rooms", 
							PULSE_DG_SCRIPT, 
							10, 
							std::make_shared<SimpleCall>([]() { script_trigger_check(WLD_TRIGGER); })),
		Heartbeat::PulseStep("Sanity check", 60 * kPassesPerSec, 2, std::make_shared<SimpleCall>(sanity_check)),
		Heartbeat::PulseStep("Check idle passwords",
							 40 * kPassesPerSec,
							 0,
							 std::make_shared<SimpleCall>(check_idle_passwords)),
		Heartbeat::PulseStep("Mobile activity", 10, 0, std::make_shared<MobActCall>()),
		Heartbeat::PulseStep("Inspecting", 1, 0, std::make_shared<InspectCall>()),
		Heartbeat::PulseStep("Set all Inspecting", 1, 0, std::make_shared<SetAllInspectCall>()),
		Heartbeat::PulseStep("Death trap activity",
							 2 * kPassesPerSec,
							 0,
							 std::make_shared<SimpleCall>(deathtrap::activity)),
		Heartbeat::PulseStep("Underwater check", 2 * kPassesPerSec, 0, std::make_shared<SimpleCall>(underwater_check)),
		Heartbeat::PulseStep("Clan system: check player in house",
							 2 * kPassesPerSec,
							 0,
							 std::make_shared<SimpleCall>(ClanSystem::check_player_in_house)),
		Heartbeat::PulseStep("Violence performing", kBattleRound, 3, std::make_shared<SimpleCall>(perform_violence)),
		Heartbeat::PulseStep("Char timer update", 1, 4, std::make_shared<SimpleCall>(CharTimerUpdate)),
		Heartbeat::PulseStep("Scheduled reboot checking",
							 30 * kPassesPerSec,
							 0,
							 std::make_shared<CheckScheduledRebootCall>()),
		Heartbeat::PulseStep("Check of reboot trigger",
							 kPassesPerSec,
							 0,
							 std::make_shared<CheckTriggeredRebootCall>()),
		Heartbeat::PulseStep("Auction update",
							 kAuctionPulses * kPassesPerSec,
							 0,
							 std::make_shared<SimpleCall>(tact_auction)),
		Heartbeat::PulseStep("Room affect update",
							 room_spells::kSecsPerRoomAffect * kPassesPerSec,
							 0,
							 std::make_shared<SimpleCall>(room_spells::UpdateRoomsAffects)),
		Heartbeat::PulseStep("Player affect update",
							 kSecsPerPlayerAffect * kPassesPerSec,
							 kPassesPerSec - 3,
							 std::make_shared<SimpleCall>(player_affect_update)),
		Heartbeat::PulseStep("Hour update",
							 kTimeKoeff * kSecsPerMudHour * kPassesPerSec,
							 kPassesPerSec - 4,
							 std::make_shared<SimpleCall>(hour_update)),
		Heartbeat::PulseStep("Timer bonus",
							 kTimeKoeff * kSecsPerMudHour * kPassesPerSec,
							 kPassesPerSec - 3,
							 std::make_shared<SimpleCall>(Bonus::timer_bonus)),
		Heartbeat::PulseStep("Weather and time",
							 kTimeKoeff * kSecsPerMudHour * kPassesPerSec,
							 kPassesPerSec - 2,
							 std::make_shared<SimpleCall>([]() { weather_and_time(1); })),
		Heartbeat::PulseStep("Paste mobiles",
							 kTimeKoeff * kSecsPerMudHour * kPassesPerSec,
							 kPassesPerSec - 1,
							 std::make_shared<SimpleCall>(PasteMobiles)),
		Heartbeat::PulseStep("Zone update", kPulseZone, 5, std::make_shared<SimpleCall>(ZoneUpdate)),
		Heartbeat::PulseStep("Money drop stat: print log",
							 60 * 60 * kPassesPerSec,
							 45,
							 std::make_shared<SimpleCall>(MoneyDropStat::print_log)),
		Heartbeat::PulseStep("Zone exp stat: print log",
							 60 * 60 * kPassesPerSec,
							 49,
							 std::make_shared<SimpleCall>(ZoneExpStat::print_log)),
		Heartbeat::PulseStep("Print rune log",
							 60 * 60 * kPassesPerSec,
							 47,
							 std::make_shared<SimpleCall>(print_rune_log)),
		Heartbeat::PulseStep("Mob stats saving",
							 60 * mob_stat::kSavePeriod * kPassesPerSec,
							 57,
							 std::make_shared<SimpleCall>(mob_stat::Save)),
		Heartbeat::PulseStep("Sets drop table saving",
							 60 * SetsDrop::SAVE_PERIOD * kPassesPerSec,
							 52,
							 std::make_shared<SimpleCall>(SetsDrop::save_drop_table)),
		Heartbeat::PulseStep("Clan system: chest log saving",
							 60 * kChestUpdatePeriod * kPassesPerSec,
							 50,
							 std::make_shared<SimpleCall>(ClanSystem::save_chest_log)),
		Heartbeat::PulseStep("Clan system: ingredients chests saving",
							 60 * kChestUpdatePeriod * kPassesPerSec,
							 48,
							 std::make_shared<SimpleCall>(ClanSystem::save_ingr_chests)),
		Heartbeat::PulseStep("Global drop: saving",
							 60 * GlobalDrop::SAVE_PERIOD * kPassesPerSec,
							 47,
							 std::make_shared<SimpleCall>(GlobalDrop::save)),
		Heartbeat::PulseStep("Clan: chest update",
							 60 * kChestUpdatePeriod * kPassesPerSec,
							 46,
							 std::make_shared<SimpleCall>(Clan::ChestUpdate)),
		Heartbeat::PulseStep("Clan: save chest all",
							 60 * kChestUpdatePeriod * kPassesPerSec,
							 44,
							 std::make_shared<SimpleCall>(Clan::SaveChestAll)),
		Heartbeat::PulseStep("Clan: clan save",
							 60 * kChestUpdatePeriod * kPassesPerSec,
							 40,
							 std::make_shared<SimpleCall>(Clan::ClanSave)),
		Heartbeat::PulseStep("Celebrates: Sanitize",
							 celebrates::kCleanPeriod * 60 * kPassesPerSec,
							 39,
							 std::make_shared<SimpleCall>(celebrates::Sanitize)),
		Heartbeat::PulseStep("Record usage", 5 * 60 * kPassesPerSec, 37, std::make_shared<SimpleCall>(record_usage)),
		Heartbeat::PulseStep("Reload proxy ban",
							 5 * 60 * kPassesPerSec,
							 36,
							 std::make_shared<SimpleCall>([]() { ban->ReloadProxyBan(); })),
		Heartbeat::PulseStep("God work invoice",
							 5 * 60 * kPassesPerSec,
							 35,
							 std::make_shared<SimpleCall>(god_work_invoice)),
		Heartbeat::PulseStep("Title system: title list saving",
							 5 * 60 * kPassesPerSec,
							 34,
							 std::make_shared<SimpleCall>(TitleSystem::save_title_list)),
		Heartbeat::PulseStep("Register system: save",
							 5 * 60 * kPassesPerSec,
							 33,
							 std::make_shared<SimpleCall>(RegisterSystem::save)),
		Heartbeat::PulseStep("Mail: save",
							 kSecsPerMudHour * kPassesPerSec,
							 32,
							 std::make_shared<SimpleCall>(mail::save)),
		Heartbeat::PulseStep("Help system: update dynamic checking",
							 kSecsPerMudHour * kPassesPerSec,
							 31,
							 std::make_shared<SimpleCall>(HelpSystem::check_update_dynamic)),
		Heartbeat::PulseStep("Sets drop: reload by timer",
							 kSecsPerMudHour * kPassesPerSec,
							 30,
							 std::make_shared<SimpleCall>(SetsDrop::reload_by_timer)),
		Heartbeat::PulseStep("Clan: save PK log",
							 kSecsPerMudHour * kPassesPerSec,
							 29,
							 std::make_shared<SimpleCall>(Clan::save_pk_log)),
		Heartbeat::PulseStep("Characters purging",
							 kSecsPerMudHour * kPassesPerSec,
							 28,
							 std::make_shared<SimpleCall>([]() { character_list.purge(); })),
		Heartbeat::PulseStep("Objects purging",
							 kSecsPerMudHour * kPassesPerSec,
							 27,
							 std::make_shared<SimpleCall>([]() { world_objects.purge(); })),
		Heartbeat::PulseStep("Glory: timers updating",
							 kSecsPerMudHour * kPassesPerSec,
							 23,
							 std::make_shared<SimpleCall>(Glory::timers_update)),
		Heartbeat::PulseStep("Glory: saving",
							 kSecsPerMudHour * kPassesPerSec,
							 22,
							 std::make_shared<SimpleCall>(Glory::save_glory)),
		Heartbeat::PulseStep("Depot: saving all online objects",
							 kSecsPerMudHour * kPassesPerSec,
							 21,
							 std::make_shared<SimpleCall>(Depot::save_all_online_objs)),
		Heartbeat::PulseStep("Depot: time data saving",
							 kSecsPerMudHour * kPassesPerSec,
							 17,
							 std::make_shared<SimpleCall>(Depot::save_timedata)),
		Heartbeat::PulseStep("Mobile affects updating",
							 kSecsPerMudHour * kPassesPerSec,
							 16,
							 std::make_shared<SimpleCall>(mobile_affect_update)),
		Heartbeat::PulseStep("Objects point updating",
							 kSecsPerMudHour * kPassesPerSec,
							 11,
							 std::make_shared<SimpleCall>(obj_point_update)),
		Heartbeat::PulseStep("Bloody: updating",
							 kSecsPerMudHour * kPassesPerSec,
							 10,
							 std::make_shared<SimpleCall>(bloody::update)),
		Heartbeat::PulseStep("Room point updating",
							 kSecsPerMudHour * kPassesPerSec,
							 6,
							 std::make_shared<SimpleCall>(room_point_update)),
		Heartbeat::PulseStep("Temporary spells: times updating",
							 kSecsPerMudHour * kPassesPerSec,
							 5,
							 std::make_shared<SimpleCall>(temporary_spells::update_times)),
		Heartbeat::PulseStep("Players index flushing",
							 kSecsPerMudHour * kPassesPerSec,
							 1,
							 std::make_shared<SimpleCall>(FlushPlayerIndex)),
		Heartbeat::PulseStep("Point updating",
							 kSecsPerMudHour * kPassesPerSec,
							 kPassesPerSec - 5,
							 std::make_shared<SimpleCall>(point_update)),
		Heartbeat::PulseStep("Beat points updating", kPassesPerSec, 0, std::make_shared<BeatPointsUpdateCall>()),
#if defined WITH_SCRIPTING
		Heartbeat::PulseStep("Scripting: heartbeat", scripting::HEARTBEAT_PASSES, 0, std::make_shared<SimpleCall>(scripting::heartbeat)),
#endif
		Heartbeat::PulseStep("Crash frac save", kPassesPerSec, 7, std::make_shared<CrashFracSaveCall>()),
		Heartbeat::PulseStep("Exchange database save",
							 EXCHANGE_AUTOSAVETIME * kPassesPerSec,
							 9,
							 std::make_shared<ExchangeDatabaseSaveCall>()),
		Heartbeat::PulseStep("Exchange database backup save",
							 EXCHANGE_AUTOSAVETIME * kPassesPerSec,
							 9,
							 std::make_shared<ExchangeDatabaseBackupSaveCall>()),
		Heartbeat::PulseStep("Global UID saving", 60 * kPassesPerSec, 9, std::make_shared<GlobalSaveUIDCall>()),
		Heartbeat::PulseStep("Crash save", 60 * kPassesPerSec, 11, std::make_shared<CrashSaveCall>()),
		Heartbeat::PulseStep("Clan experience updating",
							 60 * CLAN_EXP_UPDATE_PERIOD * kPassesPerSec,
							 14,
							 std::make_shared<UpdateClanExpCall>()),
		Heartbeat::PulseStep("Clan: chest invoice",
							 60 * kChestInvoicePeriod * kPassesPerSec,
							 15,
							 std::make_shared<SimpleCall>(Clan::ChestInvoice)),
//			Heartbeat::PulseStep("Gifts", 60 * 60 * kPassesPerSec, 18, std::make_shared<SimpleCall>(gifts)),
		Heartbeat::PulseStep("File CRC: saving",
							 kPassesPerSec,
							 23,
							 std::make_shared<SimpleCall>([]() { FileCRC::save(false); })),
		Heartbeat::PulseStep("Spells usage saving", 60 * 60 * kPassesPerSec, 0, std::make_shared<SpellUsageCall>()),
		Heartbeat::PulseStep("Zone traffic statistic saving", 30 * 60 * kPassesPerSec, 37, 
							std::make_shared<SimpleCall>(ZoneTrafficSave))
	};

	return pulse_steps_storage;
}
}

long long NOD(long long a, long long b) {
	if (a > b) {
		std::swap(a, b);
	}

	for (; 0 != a; std::swap(a, b)) {
		b = b % a;
	}

	return b;
}

long long NOK(long long a, long long b) {
	return a * b / NOD(a, b);
}

Heartbeat::Heartbeat() :
	m_steps(pulse_steps()),
	m_pulse_number(0),
	m_global_pulse_number(0) {
}

void Heartbeat::operator()(const int missed_pulses) {
	pulse_label_t label;

	utils::CExecutionTimer timer;
	pulse(missed_pulses, label);
	const auto execution_time = timer.delta();
	if (kPassesPerSec / 25.0 < execution_time.count()) {
//		log("SYSERR: Long-running tick #%d worked for %.4f seconds (missed pulses argument has value %d)",
//			pulse_number(), execution_time.count(), missed_pulses);
		char tmpbuf[256];
		sprintf(tmpbuf, "WARNING: Long-running tick #%d worked for %.4f seconds (missed pulses argument has value %d), last trig %d",
			pulse_number(), execution_time.count(), missed_pulses, last_trig_vnum);
		mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
	}
	m_measurements.add(label, pulse_number(), execution_time.count());
	if (GlobalObjects::stats_sender().ready()) {
		influxdb::Record record("heartbeat");
		record.add_tag("pulse", pulse_number());
		record.add_field("duration", execution_time.count());
		GlobalObjects::stats_sender().send(record);
	}
}

long long Heartbeat::period() const {
	long long period = 1;
	for (const auto &step : m_steps) {
		if (step.on()) {
			period = NOK(period, step.modulo());
		}
	}

	return period;
}

void Heartbeat::clear_stats() {
	for (auto &step : m_steps) {
		if (step.on()) {
			step.clear_measurements();
		}
	}
	m_measurements.clear();
}

void Heartbeat::advance_pulse_numbers() {
	m_pulse_number++;
	m_global_pulse_number++;

	// Roll pulse over after 10 hours
	if (m_pulse_number >= ROLL_OVER_AFTER) {
		m_pulse_number = 0;
	}
}

void Heartbeat::pulse(const int missed_pulses, pulse_label_t &label) {
	static int last_pmem_used = 0;

	label.clear();
	advance_pulse_numbers();
	log("Heartbeat pulse");
	character_list.PurgeExtractedList();
	world_objects.PurgeExtractedList();
	for (std::size_t i = 0; i != m_steps.size(); ++i) {
		auto &step = m_steps[i];
		auto get_mem = TotalMemUse();
		int vmem_used = get_mem.first;
		int pmem_used = get_mem.second;
		if (step.off()) {
			continue;
		}

		if (0 == (m_pulse_number + step.offset()) % step.modulo()) {
			utils::CExecutionTimer timer;

			step.action()->perform(pulse_number(), missed_pulses);
			const auto execution_time = timer.delta().count();
			if (step.modulo() >= kSecsPerMudHour * kPassesPerSec) {
				log("Heartbeat step: %s", step.name().c_str());
			}
			if (pmem_used != last_pmem_used) {
//				char buf [128];
				last_pmem_used = pmem_used;
				log("HeartBeat memory resize, step:(%s), memory used: virt (%d kB) phys (%d kB)", step.name().c_str(), vmem_used, pmem_used);
//				mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			}
			label.emplace(i, execution_time);
			m_executed_steps.insert(i);
			step.add_measurement(i, pulse_number(), execution_time);
		}
	}
}

Heartbeat::PulseStep::PulseStep(std::string name,
								const int modulo,
								const int offset,
								pulse_action_t action) :
	m_name(std::move(name)),
	m_modulo(modulo),
	m_offset(offset),
	m_action(std::move(action)),
	m_off(false) {
}

void Heartbeat::PulseStep::add_measurement(const std::size_t index,
										   const pulse_t pulse,
										   const BasePulseMeasurements::value_t value) {
	m_measurements.add(index, pulse, value);
}

void Heartbeat::PulseStep::clear_measurements() {
	m_measurements.clear();
}

BasePulseMeasurements::BasePulseMeasurements() :
	m_sum(0.0),
	m_global_sum(0.0),
	m_global_count(0) {
}

void BasePulseMeasurements::add(const measurement_t &measurement) {
	add_measurement(measurement);
	squeeze();
}

void BasePulseMeasurements::clear() {
	m_measurements.clear();
	m_sum = 0.0;
	m_global_sum = 0.0;
	m_global_count = 0;
}

void BasePulseMeasurements::add_measurement(const measurement_t &measurement) {
	m_measurements.push_front(measurement);
	m_sum += measurement.second.first;
	m_global_sum += measurement.second.first;
	++m_global_count;

	m_min.insert(measurement);
	m_max.insert(measurement);
}

void BasePulseMeasurements::squeeze() {
	while (m_measurements.size() > window_size()) {
		const auto &last_value = m_measurements.back();

		remove_handler(last_value.first);

		const auto min_i = m_min.find(last_value);
		m_min.erase(min_i);

		const auto max_i = m_max.find(last_value);
		m_max.erase(max_i);

		m_sum -= last_value.second.first;

		m_measurements.pop_back();
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
