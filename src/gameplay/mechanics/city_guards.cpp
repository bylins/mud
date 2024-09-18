//
// Created by Sventovit on 03.09.2024.
//
#include "city_guards.h"

#include "engine/boot/boot_constants.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/clans/house.h"
#include "utils/logger.h"

#include <third_party_libs/fmt/include/fmt/format.h>

namespace city_guards {

GuardianRosterType guardian_roster;

void LoadGuardians() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"guards.xml");
	if (!result) {
		const auto msg = fmt::format("...{}", result.description());
		mudlog(msg, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node xMainNode = doc.child("guardians");
	if (!xMainNode) {
		mudlog("...guards.xml read fail", CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	guardian_roster.clear();
	int num_wars_global = atoi(xMainNode.child_value("wars"));
	struct CityGuardian tmp_guard;
	for (pugi::xml_node xNodeGuard = xMainNode.child("guard"); xNodeGuard;
		 xNodeGuard = xNodeGuard.next_sibling("guard")) {
		int guard_vnum = xNodeGuard.attribute("vnum").as_int();

		if (guard_vnum <= 0) {
			log("ERROR: Ошибка загрузки файла %s - некорректное значение VNUM: %d", LIB_MISC"guards.xml", guard_vnum);
			continue;
		}
		//значения по умолчанию
		tmp_guard.max_wars_allow = num_wars_global;
		tmp_guard.agro_all_agressors = false;
		tmp_guard.agro_argressors_in_zones.clear();
		tmp_guard.agro_killers = true;

		int num_wars = xNodeGuard.attribute("wars").as_int();

		if (num_wars && (num_wars != num_wars_global)) {
			tmp_guard.max_wars_allow = num_wars;
		}

		if (!strcmp(xNodeGuard.attribute("killer").value(), "no")) {
			tmp_guard.agro_killers = false;
		}

		if (!strcmp(xNodeGuard.attribute("agressor").value(), "yes")) {
			tmp_guard.agro_all_agressors = true;
		}

		if (!strcmp(xNodeGuard.attribute("agressor").value(), "list")) {
			for (pugi::xml_node xNodeZone = xNodeGuard.child("zone"); xNodeZone;
				 xNodeZone = xNodeZone.next_sibling("zone")) {
				tmp_guard.agro_argressors_in_zones.push_back(atoi(xNodeZone.child_value()));
			}
		}
		guardian_roster[guard_vnum] = tmp_guard;
	}

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
