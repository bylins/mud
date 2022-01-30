#include "dg_domination_helper.h"
#include "dg_scripts.h"

#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "structs/structs.h"
#include "utils/utils.h"

#include <vector>
#include <utility>
#include <string>

// значения переменых из текущего контекста
struct DominationData {
	// дополнительные сообщения в лог
	bool debug_mode = false;

	// список мобов и список комнат
	std::vector<std::pair<std::vector<MobVnum>, std::vector<RoomVnum>>> mob_list_to_load;

	// текущий раунд
	int current_round;

	// список количества загружаемых мобов по раундам
	std::vector<short> mob_counter_per_round;
};

static bool read_local_variables(DominationData &dd, Script *sc, Trigger *trig, char *cmd)
{
	static const std::vector<std::pair<std::string, std::string>> var_name_mob_list = {
		{"hares", "hares_rooms"},
		{"falcons", "falcons_rooms"},
		{"bears", "bears_rooms"},
		{"lions", "lions_rooms"},
		{"wolves", "wolves_rooms"},
		{"lynxes", "lynxes_rooms"},
		{"bulls", "bulls_rooms"},
		{"snakes", "snakes_rooms"}
	};
	static const std::string var_name_round = "round";
	static const std::string var_name_counter = "mobs_count";
	static const std::string var_name_debug = "debug_messages";
	static const short expected_round_count = 9;

	char local_buf[kMaxTrglineLength];

	auto arg = one_argument(cmd, local_buf);
	if (!*arg) {
		trig_log(trig, "process_arena_round вызван без аргументов");
		return false;
	}

	// дебажные сообщения в лог
	TriggerVar *vd_debug = find_var_cntx(&GET_TRIG_VARS(trig), var_name_debug.c_str(), sc->context);
	dd.debug_mode = false;
	if (vd_debug && vd_debug->value) {
		dd.debug_mode = atoi(vd_debug->value);
	}

	dd.current_round = atoi(arg);
	if (dd.current_round < 1 || dd.current_round > expected_round_count) {
		snprintf(buf2, kMaxStringLength, "Неверное значение переменной round: %d", dd.current_round);
		trig_log(trig, buf2);
		return false;
	}
	if (dd.debug_mode) {
		snprintf(buf2, kMaxStringLength, "номер раунда: %d", atoi(arg));
		trig_log(trig, buf2);
	}

	// количество мобов
	TriggerVar *vd_mob_count = find_var_cntx(&GET_TRIG_VARS(trig), var_name_counter.c_str(), sc->context);
	if (!vd_mob_count) {
		snprintf(buf2, kMaxStringLength, "local var '%s' not found", var_name_counter.c_str());
		trig_log(trig, buf2);
		return false;
	}
	// вычитываем список количества мобов по раундам
	array_argument(vd_mob_count->value, dd.mob_counter_per_round);
	if (dd.mob_counter_per_round.size() != expected_round_count) {
		snprintf(buf2, kMaxStringLength, "Неверное количество элементов в переменной mobs_count: %zu", dd.mob_counter_per_round.size());
		trig_log(trig, buf2);
		return false;
	}
	// валидация количества мобов по раундам
	const int mob_count_minimum = 0;
	const int mob_count_mamimum = 30;
	for (const auto &counter : dd.mob_counter_per_round) {
		if (counter < mob_count_minimum || counter > mob_count_mamimum) {
			snprintf(buf2, kMaxStringLength, "Неверный счетчик мобов в переменной mobs_count: %d", counter);
			trig_log(trig, buf2);
			return false;
		}
	}

	for (const auto var_name_mob : var_name_mob_list) {
		TriggerVar *vd_mob = find_var_cntx(&GET_TRIG_VARS(trig), var_name_mob.first.c_str(), sc->context);
		if (!vd_mob) {
			snprintf(buf2, kMaxStringLength, "local var '%s' not found", var_name_mob.first.c_str());
			trig_log(trig, buf2);
			return false;
		}
		std::vector<MobVnum> mob_vnum_list;
		array_argument(vd_mob->value, mob_vnum_list);
		// т.к. мобы грузятся по раундам - количество мобов >= количествj раундов
		if (mob_vnum_list.size() < expected_round_count) {
			snprintf(buf2, kMaxStringLength, "Неверное количество мобов в группе %s: %zu", var_name_mob.first.c_str(), mob_vnum_list.size());
			trig_log(trig, buf2);
			return false;
		}

		TriggerVar *vd_room = find_var_cntx(&GET_TRIG_VARS(trig), var_name_mob.second.c_str(), sc->context);
		if (!vd_room) {
			snprintf(buf2, kMaxStringLength, "local var '%s' not found", var_name_mob.second.c_str());
			trig_log(trig, buf2);
			return false;
		}
		std::vector<MobVnum> room_vnum_list;
		array_argument(vd_room->value, room_vnum_list);

		dd.mob_list_to_load.emplace_back(mob_vnum_list, room_vnum_list);
	}

	return true;
}

void process_arena_round(Script *sc, Trigger *trig, char *cmd)
{
	DominationData dd;
	const bool load_status = read_local_variables(dd, sc, trig, cmd);
	if (!load_status) {
		return;
	}

	// загрузка мобов по комнатам
	const int mob_counter = dd.mob_counter_per_round.at(dd.current_round - 1);
	if (dd.debug_mode) {
		snprintf(buf2, kMaxStringLength, "Количество загружаемых мобов: %d", mob_counter);
		trig_log(trig, buf2);
	}
	for (auto i = 0; i < mob_counter; i++) {
		for (const auto &mob_to_load : dd.mob_list_to_load) {
			const auto &mob_vnum_list = mob_to_load.first;
			const auto &room_vnum_list = mob_to_load.second;
			if (room_vnum_list.empty() || mob_vnum_list.empty()) {
				continue;
			}
			const auto random_room_index = number(0, room_vnum_list.size() - 1);
			const auto mob_index = dd.current_round - 1;

			const auto random_room = real_room(room_vnum_list.at(random_room_index));
			if (random_room < 0) {
				snprintf(buf2, kMaxStringLength, "Не могу найти комнату: %d", room_vnum_list.at(random_room_index));
				trig_log(trig, buf2);
				continue;
			}

			CharacterData *random_mob = read_mobile(mob_vnum_list.at(mob_index), VIRTUAL);
			if (!random_mob) {
				snprintf(buf2, kMaxStringLength, "Не могу найти моба: %d", mob_vnum_list.at(mob_index));
				trig_log(trig, buf2);
				continue;
			}

			if (dd.debug_mode) {
				snprintf(buf2, kMaxStringLength, "load mob: %d to room: %d", mob_vnum_list.at(mob_index), room_vnum_list.at(random_room_index));
				trig_log(trig, buf2);
			}
			char_to_room(random_mob, random_room);
			load_mtrigger(random_mob);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
