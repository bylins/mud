/**
 \file service_zones.cpp - a part of the Bylins engine.
 \brief Реестр служебных и начальных зон, читается из cfg/mechanics/service_zones.xml.
*/

#include "service_zones.h"
#include "service_zones_loaders.h"

#include "utils/logger.h"
#include "utils/utils_parse.h"

#include <set>
#include <utility>
#include <vector>

namespace service_zones {

namespace {

// Одиночные зоны и диапазоны храним раздельно, чтобы конфиг читался как список намерений
// билдера, а не как развернутая простыня внумов.
struct ZoneSet {
	std::set<ZoneVnum> single;
	std::vector<std::pair<ZoneVnum, ZoneVnum>> ranges;

	void clear() {
		single.clear();
		ranges.clear();
	}

	[[nodiscard]] bool contains(ZoneVnum vnum) const {
		if (single.count(vnum) > 0) {
			return true;
		}
		for (const auto &[from, to] : ranges) {
			if (vnum >= from && vnum <= to) {
				return true;
			}
		}
		return false;
	}

	[[nodiscard]] std::size_t size() const { return single.size() + ranges.size(); }
};

ZoneSet g_service;
ZoneSet g_starting;

// Читает <zone vnum="N"/> и <zone from="A" to="B"/> внутри указанного раздела.
void LoadSection(parser_wrapper::DataNode data, const char *section, ZoneSet &out) {
	out.clear();
	if (!data.GoToChild(section)) {
		log("SYSERROR: service_zones: нет раздела <%s>", section);
		return;
	}

	for (auto &node : data.Children("zone")) {
		const char *vnum = node.GetValue("vnum");
		if (vnum && *vnum) {
			try {
				out.single.insert(parse::ReadAsInt(vnum));
			} catch (const std::exception &) {
				log("SYSERROR: service_zones: в <%s> не разобран vnum '%s'", section, vnum);
			}
			continue;
		}

		const char *from = node.GetValue("from");
		const char *to = node.GetValue("to");
		if (from && *from && to && *to) {
			try {
				const ZoneVnum a = parse::ReadAsInt(from);
				const ZoneVnum b = parse::ReadAsInt(to);
				if (a <= b) {
					out.ranges.emplace_back(a, b);
				} else {
					log("SYSERROR: service_zones: в <%s> диапазон %d-%d задан наоборот", section, a, b);
				}
			} catch (const std::exception &) {
				log("SYSERROR: service_zones: в <%s> не разобран диапазон '%s'-'%s'", section, from, to);
			}
			continue;
		}

		log("SYSERROR: service_zones: в <%s> у <zone> нет ни vnum, ни пары from/to", section);
	}
	data.GoToParent();
}

void LoadFrom(parser_wrapper::DataNode data) {
	LoadSection(data, "service", g_service);
	LoadSection(data, "starting", g_starting);
	log("Service zones: служебных записей %zu, начальных %zu.", g_service.size(), g_starting.size());
}

}  // namespace

bool IsServiceZone(ZoneVnum zone_vnum) { return g_service.contains(zone_vnum); }
bool IsStartingZone(ZoneVnum zone_vnum) { return g_starting.contains(zone_vnum); }
bool IsNoLootZone(ZoneVnum zone_vnum) { return IsServiceZone(zone_vnum) || IsStartingZone(zone_vnum); }
bool IsNoLootZoneByVnum(int entity_vnum) { return IsNoLootZone(entity_vnum / 100); }

void ServiceZonesLoader::Load(parser_wrapper::DataNode data) { LoadFrom(data); }
void ServiceZonesLoader::Reload(parser_wrapper::DataNode data) { LoadFrom(data); }

}  // namespace service_zones

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
