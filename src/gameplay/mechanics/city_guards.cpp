//
// Created by Sventovit on 03.09.2024.
//
#include "city_guards.h"

#include "utils/parser_wrapper.h"   // issue.guards: ParserWrapper вместо прямого pugixml
#include "utils/parse.h"
#include "engine/boot/boot_constants.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/clans/house.h"
#include "utils/logger.h"


namespace city_guards {

using parser_wrapper::DataNode;

GuardianRosterType guardian_roster;

namespace {
// issue.guards: целочисленный атрибут через ParserWrapper (parse::ReadAsInt бросает на
// пустой строке), с дефолтом при отсутствии/ошибке.
int AttrInt(const DataNode &node, const char *key, int def = 0) {
	const char *v = node.GetValue(key);
	if (!v || !*v) {
		return def;
	}
	try {
		return parse::ReadAsInt(v);
	} catch (const std::exception &) {
		return def;
	}
}
} // namespace

// issue.guards: data = корневой элемент <guardians wars="N"> (передаётся CfgManager через
// ParserWrapper). Механика стражников не менялась; формат: глобальный wars стал атрибутом
// корня, а зоны режима agressor="list" - атрибутами <zone vnum="N"/> (раньше текст тегов).
void CityGuardsLoader::Load(DataNode data) {
	guardian_roster.clear();
	int num_wars_global = AttrInt(data, "wars");
	struct CityGuardian tmp_guard;
	for (auto &guard_node : data.Children("guard")) {
		int guard_vnum = AttrInt(guard_node, "vnum");

		if (guard_vnum <= 0) {
			log("ERROR: Ошибка загрузки файла cfg/mechanics/guards.xml - некорректное значение VNUM: %d", guard_vnum);
			continue;
		}
		//значения по умолчанию
		tmp_guard.max_wars_allow = num_wars_global;
		tmp_guard.agro_all_agressors = false;
		tmp_guard.agro_argressors_in_zones.clear();
		tmp_guard.agro_killers = true;

		int num_wars = AttrInt(guard_node, "wars");

		if (num_wars && (num_wars != num_wars_global)) {
			tmp_guard.max_wars_allow = num_wars;
		}

		const char *killer = guard_node.GetValue("killer");
		if (killer && !strcmp(killer, "no")) {
			tmp_guard.agro_killers = false;
		}

		const char *agressor = guard_node.GetValue("agressor");
		if (agressor && !strcmp(agressor, "yes")) {
			tmp_guard.agro_all_agressors = true;
		}

		if (agressor && !strcmp(agressor, "list")) {
			for (auto &zone_node : guard_node.Children("zone")) {
				tmp_guard.agro_argressors_in_zones.push_back(AttrInt(zone_node, "vnum"));
			}
		}
		guardian_roster[guard_vnum] = tmp_guard;
	}
}

void CityGuardsLoader::Reload(DataNode data) {
	Load(std::move(data));
}

bool MustGuardianAttack(CharData *ch, CharData *vict) {
	if (!ch->IsNpc() || !vict || !ch->IsFlagged(EMobFlag::kCityGuardian)) {
		return false;
	}

	if (!guardian_roster.contains(GET_MOB_VNUM(ch))) {
		return false;
	}

	const auto &tmp_guard = guardian_roster[GET_MOB_VNUM(ch)];
	if ((tmp_guard.agro_all_agressors && AGRESSOR(vict)) ||
		(tmp_guard.agro_killers && vict->IsFlagged(EPlrFlag::kKiller))) {
		return true;
	}

	if (CLAN(vict)) {
		auto num_wars_vict = Clan::GetClanWars(vict);
		int clan_town_vnum = CLAN(vict)->GetOutRent() / 100; //Polud подскажите мне другой способ определить vnum зоны
		int mob_town_vnum = GET_MOB_VNUM(ch) / 100;          //по vnum комнаты, не перебирая все комнаты и зоны мира
		return (num_wars_vict && num_wars_vict > tmp_guard.max_wars_allow && clan_town_vnum != mob_town_vnum);
	}

	if (AGRESSOR(vict)) {
		for (const int agro_argressors_in_zone : tmp_guard.agro_argressors_in_zones) {
			return (agro_argressors_in_zone == AGRESSOR(vict) / 100);
		}
	}

	return false;
}

} // namespace city_guards

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
