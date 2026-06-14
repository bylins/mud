//
// Created by Sventovit on 07.09.2024. Reworked for issue.cities (InfoContainer + cfg_manager).
//

#include "cities.h"

#include "cities_messages.h"
#include "administration/privilege.h"
#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include "utils/parse.h"
#include "utils/utils.h"

using parser_wrapper::DataNode;

namespace cities {

CityInfoBuilder::ItemPtr CityInfoBuilder::Build(DataNode &node) {
	try {
		return ParseCity(node);
	} catch (std::exception &e) {
		err_log("City parsing error (incorrect value '%s').", e.what());
		return nullptr;
	}
}

CityInfoBuilder::ItemPtr CityInfoBuilder::ParseCity(DataNode node) {
	const int vnum = parse::ReadAsInt(node.GetValue("vnum"));
	auto mode = CityInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	std::string text_id = parse::ReadAsStr(node.GetValue("id"));
	std::string name = MUD::CityMessages().GetName(vnum);

	auto city = std::make_shared<CityInfo>(vnum, text_id, name, mode);
	if (node.GoToChild("rent")) {
		parse::ReadAsIntSet(city->rent_vnums_, node.GetValue("vnums"));
		node.GoToParent();
	}

	if (node.GoToChild("zones")) {
		for (auto &zone : node.Children("zone")) {
			CityZone cz;
			cz.vnum = parse::ReadAsInt(zone.GetValue("vnum"));
			const char *suburb = zone.GetValue("suburb");
			cz.suburb = suburb && (std::string(suburb) == "true" || std::string(suburb) == "1");
			city->zones_.push_back(cz);
		}
		node.GoToParent();
	}

	for (auto &item : node.Children("start_item")) {
		city->start_items_.push_back(parse::ReadAsInt(item.GetValue("vnum")));
	}
	return city;
}

void CitiesLoader::Load(DataNode data) {
	MUD::Cities().Init(data.Children());
}

void CitiesLoader::Reload(DataNode data) {
	MUD::Cities().Reload(data.Children());
}

std::string CitiesLoader::EditableWhat() const {
	return "city";
}

std::vector<cfg_manager::EditableElement> CitiesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &city : MUD::Cities()) {
		if (city.GetId() < 0) {   // skip the kUndefined sentinel
			continue;
		}
		out.push_back({std::to_string(city.GetId()), city.GetTextId() + " " + city.GetName()});
	}
	return out;
}

cfg_manager::ValidationResult CitiesLoader::Validate(DataNode &doc) const {
	if (MUD::Cities().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "City data failed to parse (see syslog for the offending city)."};
}

parser_wrapper::DataNode CitiesLoader::FindElementNode(DataNode root, const std::string &id) const {
	// A <city> is keyed by its integer vnum; iterate all children with a name check (a node copied
	// out of a keyed range would otherwise carry that range's filter -- see GuildsLoader).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "city" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string CitiesLoader::CanonicalElementId(const std::string &id) const {
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode CitiesLoader::CreateElementNode(DataNode root, const std::string &id) const {
	auto node = root.AddChild("city");
	node.SetValue("vnum", id);
	node.SetValue("id", "Undefined");
	node.AddChild("rent");
	node.AddChild("zones");
	return node;
}

void CheckCityVisit(CharData *ch, RoomRnum room_rnum) {
	for (const auto &city : MUD::Cities()) {
		if (city.GetId() < 0) {
			continue;
		}
		if (city.HasRentVnum(GET_ROOM_VNUM(room_rnum))) {
			ch->mark_city(city.GetTextId());
			return;
		}
	}
}

bool IsCharInCity(CharData *ch) {
	const int zone_vnum = GetZoneVnumByCharPlace(ch);
	for (const auto &city : MUD::Cities()) {
		if (city.GetId() < 0) {
			continue;
		}
		if (city.HasZone(zone_vnum)) {
			return true;
		}
	}
	return false;
}

std::vector<int> StartItemsForRoom(int room_vnum) {
	const int zone_vnum = room_vnum / 100;   // зона = внум комнаты / 100 (как в старых birthplaces)
	for (const auto &city : MUD::Cities()) {
		if (city.GetId() < 0) {
			continue;
		}
		if (city.HasZone(zone_vnum)) {
			return city.GetStartItems();
		}
	}
	return {};
}

void DoCities(CharData *ch, char *, int, int) {
	SendMsgToChar("Города на Руси:\r\n", ch);
	int n = 0;
	for (const auto &city : MUD::Cities()) {
		if (city.GetId() < 0) {
			continue;
		}
		++n;
		sprintf(buf, "%3d.", n);
		if (privilege::IsImmortal(ch)) {
			std::string rents;
			for (const int rent_vnum : city.GetRentVnums()) {
				if (!rents.empty()) {
					rents += "|";
				}
				rents += std::to_string(rent_vnum);
			}
			sprintf(buf1, " [vnum: %d, rent: %s]", city.GetId(), rents.c_str());
			strcat(buf, buf1);
		}
		sprintf(buf1,
				" %s: %s\r\n",
				city.GetName().c_str(),
				(ch->check_city(city.GetTextId()) ? "&gВы были там.&n" : "&rВы еще не были там.&n"));
		strcat(buf, buf1);
		SendMsgToChar(buf, ch);
	}
}

} // namespace cities

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
