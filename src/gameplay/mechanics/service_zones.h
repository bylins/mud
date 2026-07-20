/**
 \file service_zones.h - a part of the Bylins engine.
 \brief Реестр зон, которых для игровой добычи как бы нет.

 Таких зон два вида, и путать их не стоит:
   служебные (1-39, а когда места перестало хватать -- еще 2000-2030) -- системные,
	 тестовые, склады образцов, витрины минисетов; игрок туда обычным путем не попадает;
   начальные (40-99) -- зоны для новичков; они настоящие, просто сеты в них не падают.

 До появления реестра каждое место решало вопрос само и по-своему: sets_drop считал
 неигровыми зоны ниже 100, shop_ext -- ниже 40, stable_objs держал собственный список
 диапазонов внумов предметов. Список жил в памяти того, кто его завел, и разъезжался.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_SERVICE_ZONES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_SERVICE_ZONES_H_

#include "engine/structs/structs.h"

namespace service_zones {

// Служебная зона: системная, тестовая, склад образцов и тому подобное.
[[nodiscard]] bool IsServiceZone(ZoneVnum zone_vnum);

// Начальная зона: настоящая зона для новичков, но добыча в ней не работает.
[[nodiscard]] bool IsStartingZone(ZoneVnum zone_vnum);

// Служебная или начальная -- то, что нужно почти всем механикам добычи.
[[nodiscard]] bool IsNoLootZone(ZoneVnum zone_vnum);

// То же самое, но по внуму комнаты, моба или предмета.
[[nodiscard]] bool IsNoLootZoneByVnum(int entity_vnum);

}  // namespace service_zones

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_SERVICE_ZONES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
