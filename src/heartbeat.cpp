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

struct InternalStructures
{
};

Heartbeat::Heartbeat() :
	m_mins_since_crashsave(0),
	m_pulse_number(0),
	m_global_pulse_number(0),
	m_last_rent_check(0),
	m_external_trigger_checker(nullptr)
{
}

Heartbeat::~Heartbeat()
{
	delete m_external_trigger_checker;
}

void Heartbeat::operator()(const int missed_pulses)
{
	utils::CExecutionTimer timer;
	pulse(missed_pulses);
	const auto execution_time = timer.delta();
	if (PASSES_PER_SEC / 60.0 < execution_time.count())
	{
		log("SYSERR: Long-running tick #%d worked for %.4f seconds (missed pulses argument has value %d)",
			pulse_number(), execution_time.count(), missed_pulses);
	}
}

void Heartbeat::reset_last_rent_check()
{
	m_last_rent_check = time(NULL);
}

void Heartbeat::pulse(const int missed_pulses)
{
	int uptime_minutes = 0;
	long check_at = 0;

	m_pulse_number++;
	m_global_pulse_number++;

	// Roll pulse over after 10 hours
	if (m_pulse_number >= (600 * 60 * PASSES_PER_SEC))
	{
		m_pulse_number = 0;
	}

	if (!(m_pulse_number % PASSES_PER_SEC))
	{
		const auto boot_time = GlobalObjects::shutdown_parameters().get_boot_time();
		uptime_minutes = ((time(NULL) - boot_time) / 60);
	}

	if ((m_pulse_number % (PASSES_PER_SEC)) == 0)
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

	// таблица меняется каждые два часа
	if ((m_pulse_number % (PASSES_PER_SEC * 120 * 60)) == 0)
	{
		GlobalDrop::reload_tables();
	}

	process_events();

	if (!((m_pulse_number + 1) % PULSE_DG_SCRIPT))  	//log("Triggers check...");
	{
		script_trigger_check();
	}

	if (!((m_pulse_number + 2) % (60 * PASSES_PER_SEC)))
	{
		sanity_check();
	}

	if (!(m_pulse_number % (40 * PASSES_PER_SEC)))
	{
		check_idle_passwords();
	}

	// экономим процессор. mobile_activity() дергать каждый пульс совсем не обязательно.
	// второй аргумент -- период вызова в пульсах
	// желательно, чтобы этот период был таким, чтобы различные задержки,
	// выраженные в количестве пульсов, были ему кратны.
	if (!(m_pulse_number % 10))
	{
		mobile_activity(m_pulse_number, 10);
	}

	if ((missed_pulses == 0) && (inspect_list.size() > 0))
	{
		inspecting();
	}

	if ((missed_pulses == 0) && (setall_inspect_list.size() > 0))
	{
		setall_inspect();
	}

	if (!(m_pulse_number % (2 * PASSES_PER_SEC)))
	{
		DeathTrap::activity();
		underwater_check();
		ClanSystem::check_player_in_house();
	}

	if (!((m_pulse_number + 3) % PULSE_VIOLENCE))
	{
		perform_violence();
	}

	if (!(m_pulse_number % (30 * PASSES_PER_SEC)))
	{
		if (uptime_minutes >= (shutdown_parameters.get_reboot_uptime() - 30)
			&& shutdown_parameters.get_shutdown_timeout() == 0)
		{
			//reboot after 30 minutes minimum. Auto reboot cannot run earlier.
			send_to_all("АВТОМАТИЧЕСКАЯ ПЕРЕЗАГРУЗКА ЧЕРЕЗ 30 МИНУТ.\r\n");
			shutdown_parameters.reboot(1800);
		}
	}

	if (!(m_pulse_number % PASSES_PER_SEC))
	{
		check_external_reboot_trigget();
	}

	if (!(m_pulse_number % (AUCTION_PULSES * PASSES_PER_SEC)))  	//log("Auction update...");
	{
		tact_auction();
	}

	if (!(m_pulse_number % (SECS_PER_ROOM_AFFECT * PASSES_PER_SEC)))
	{
		RoomSpells::room_affect_update();
	}

	if (!(m_pulse_number % (SECS_PER_PLAYER_AFFECT * PASSES_PER_SEC)))
	{
		player_affect_update();
	}

	if (!((m_pulse_number + PASSES_PER_SEC - 1) % (TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		hour_update();
		Bonus::timer_bonus();
		weather_and_time(1);
		paste_mobiles();
	}

	if (!((m_pulse_number + 5) % PULSE_ZONE))
	{
		zone_update();
	}

	if (!((m_pulse_number + 49) % (60 * 60 * PASSES_PER_SEC)))
	{
		MoneyDropStat::print_log();
		ZoneExpStat::print_log();
		print_rune_log();
	}

	if (!((m_pulse_number + 57) % (60 * mob_stat::SAVE_PERIOD * PASSES_PER_SEC)))
	{
		mob_stat::save();
	}

	if (!((m_pulse_number + 52) % (60 * SetsDrop::SAVE_PERIOD * PASSES_PER_SEC)))
	{
		SetsDrop::save_drop_table();
	}

	// раз в 10 минут >> ///////////////////////////////////////////////////////////

	// если здесь прибавляется больше 25 пульсов - это фигня, потому что
	// перекрываться с другими тайм-фреймами они так или иначе будут в любом случае
	// все на разные пульсы не раскидаешь внутри одной секунды, а многие функции
	// сами по себе работают намного дольше 1го пульса
	// здесь важно то, что они не перекрываются друг другом в момент тика

	// сохранение лога клан-хранов
	if (!((m_pulse_number + 50) % (60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC)))
	{
		ClanSystem::save_chest_log();
	}

	// сохранение клан-хранов для ингров
	if (!((m_pulse_number + 48) % (60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC)))
	{
		ClanSystem::save_ingr_chests();
	}

	// убитые мобы для глобал-дропа
	if (!((m_pulse_number + 47) % (60 * GlobalDrop::SAVE_PERIOD * PASSES_PER_SEC)))
	{
		GlobalDrop::save();
	}

	// снятие денег за шмот в клановых сундуках
	if (!((m_pulse_number + 46) % (60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC)))
	{
		Clan::ChestUpdate();
	}

	// сохранение клан-хранов
	if (!((m_pulse_number + 44) % (60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC)))
	{
		Clan::SaveChestAll();
	}

	// и самих кланов
	if (!((m_pulse_number + 40) % (60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC)))
	{
		Clan::ClanSave();
	}

	//Polud организуем зачистку после праздника
	if (!((m_pulse_number + 39) % (Celebrates::CLEAN_PERIOD * 60 * PASSES_PER_SEC)))
	{
		Celebrates::sanitize();
	}

	// раз в 5 минут >> ////////////////////////////////////////////////////////////

	if (!((m_pulse_number + 37) % (5 * 60 * PASSES_PER_SEC)))
	{
		record_usage();
	}

	if (!((m_pulse_number + 36) % (5 * 60 * PASSES_PER_SEC)))
	{
		ban->reload_proxy_ban(ban->RELOAD_MODE_TMPFILE);
	}

	// вывод иммам о неодобренных именах и титулах
	if (!((m_pulse_number + 35) % (5 * 60 * PASSES_PER_SEC)))
	{
		god_work_invoice();
	}

	// сейв титулов, ждущих одобрения
	if (!((m_pulse_number + 34) % (5 * 60 * PASSES_PER_SEC)))
	{
		TitleSystem::save_title_list();
	}

	// сейв зареганных мыл
	if (!((m_pulse_number + 33) % (5 * 60 * PASSES_PER_SEC)))
	{
		RegisterSystem::save();
	}

	// раз в минуту >> /////////////////////////////////////////////////////////////

	// сохранение почты (при наличии изменений)
	if (!((m_pulse_number + 32) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		mail::save();
	}

	// проверка необходимости обновления динамической справки
	if (!((m_pulse_number + 31) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		HelpSystem::check_update_dynamic();
	}

	// обновление таблицы дропа сетов
	if (!((m_pulse_number + 30) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		SetsDrop::reload_by_timer();
	}

	// клан-пк
	if (!((m_pulse_number + 29) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Clan::save_pk_log();
	}

	// очистка спурженных char_data и obj_data
	if (!((m_pulse_number + 28) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		character_list.purge();
		world_objects.purge();
	}

	// апдейт таймеров в личных хранах + пурж чего надо
	if (!((m_pulse_number + 25) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Depot::update_timers();
	}
	// апдейт таймеров на почте + разворот посылок/пурж
	if (!((m_pulse_number + 24) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Parcel::update_timers();
	}

	// апдейт таймеров славы
	if (!((m_pulse_number + 23) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Glory::timers_update();
	}

	// сохранение файла славы
	if (!((m_pulse_number + 22) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Glory::save_glory();
	}

	// сохранение онлайновых списков шмота
	if (!((m_pulse_number + 21) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Depot::save_all_online_objs();
	}

	// сохранение таймер-инфы всех шмоток в общий файл
	if (!((m_pulse_number + 17) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Depot::save_timedata();
	}

	if (!((m_pulse_number + 16) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		mobile_affect_update();
	}

	if (!((m_pulse_number + 11) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		obj_point_update();
		bloody::update();
	}

	if (!((m_pulse_number + 6) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		room_point_update();
	}

	if (!((m_pulse_number + 5) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		Temporary_Spells::update_times();
	}

	if (!((m_pulse_number + 2) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		exchange_point_update();
	}

	if (!((m_pulse_number + 1) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		flush_player_index();
	}

	if (!((m_pulse_number + PASSES_PER_SEC - 2) % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
	{
		point_update();
	}

	// << раз в минуту /////////////////////////////////////////////////////////////

	if (!(m_pulse_number % PASSES_PER_SEC))
	{
		beat_points_update(m_pulse_number / PASSES_PER_SEC);
	}

#if defined WITH_SCRIPTING
	if (!(m_pulse_number % scripting::HEARTBEAT_PASSES))
	{
		scripting::heartbeat();
	}
#endif

	if (FRAC_SAVE && AUTO_SAVE && !((m_pulse_number + 7) % PASSES_PER_SEC))  	// 1 game second
	{
		Crash_frac_save_all((m_pulse_number / PASSES_PER_SEC) % PLAYER_SAVE_ACTIVITY);
		Crash_frac_rent_time((m_pulse_number / PASSES_PER_SEC) % OBJECT_SAVE_ACTIVITY);
	}

	if (EXCHANGE_AUTOSAVETIME && AUTO_SAVE && !((m_pulse_number + 9) % (EXCHANGE_AUTOSAVETIME * PASSES_PER_SEC)))
	{
		exchange_database_save();
	}

	if (EXCHANGE_AUTOSAVEBACKUPTIME && !((m_pulse_number + 9) % (EXCHANGE_AUTOSAVEBACKUPTIME * PASSES_PER_SEC)))
	{
		exchange_database_save(true);
	}

	if (AUTO_SAVE && !((m_pulse_number + 9) % (60 * PASSES_PER_SEC)))
	{
		SaveGlobalUID();
	}

	if (!FRAC_SAVE && AUTO_SAVE && !((m_pulse_number + 11) % (60 * PASSES_PER_SEC)))  	// 1 minute
	{
		if (++m_mins_since_crashsave >= AUTOSAVE_TIME)
		{
			m_mins_since_crashsave = 0;
			Crash_save_all();
			check_at = time(NULL);

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
	}

	// обновление и сохранение клановой экспы
	if (!((m_pulse_number + 14) % (60 * CLAN_EXP_UPDATE_PERIOD * PASSES_PER_SEC)))
	{
		update_clan_exp();
		save_clan_exp();
	}

	// оповещение о скорой кончине денег в дружине
	if (!((m_pulse_number + 15) % (60 * CHEST_INVOICE_PERIOD * PASSES_PER_SEC)))
	{
		Clan::ChestInvoice();
	}

	// обновление статов экспы в топе кланов для тех, кто вырубил показ на лету
	if (!((m_pulse_number + 16) % (60 * CLAN_TOP_REFRESH_PERIOD * PASSES_PER_SEC)))
	{
		Clan::SyncTopExp();
	}

	// сохранение файла чексумм, если в нем были изменения
	if (!((m_pulse_number + 23) % (PASSES_PER_SEC)))
	{
		FileCRC::save();
	}

	//Polud раз в час проверяем не пришло ли время сохранить статистику
	if (SpellUsage::isActive && (!(m_pulse_number % (60 * 60 * PASSES_PER_SEC))))
	{
		time_t tmp_time = time(0);
		if ((tmp_time - SpellUsage::start) >= (60 * 60 * 24))
		{
			SpellUsage::save();
			SpellUsage::clear();
		}
	}
}

void Heartbeat::check_external_reboot_trigget()
{
	try
	{
		if (!m_external_trigger_checker)
		{
			m_external_trigger_checker = new ExternalTriggerChecker(runtime_config.external_reboot_trigger_file_name());
		}
	}
	catch (const std::bad_alloc&)
	{
		log("SYSERR: Couldn't allocate memory for external trigger checker.");
		return;
	}

	if (m_external_trigger_checker->check())
	{
		mudlog("Сработал внешний триггер перезагрузки.", DEF, LVL_IMPL, SYSLOG, true);
		shutdown_parameters.reboot();
	}
}

Heartbeat& heartbeat = GlobalObjects::heartbeat();

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
