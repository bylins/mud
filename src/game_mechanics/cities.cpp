//
// Created by Sventovit on 07.09.2024.
//

#include "cities.h"
#include "entities/char_data.h"

#include <vector>

#include <third_party_libs/pugixml/pugixml.h>

extern pugi::xml_node XmlLoad(const char *PathToFile,
							  const char *MainTag,
							  const char *ErrorStr,
							  pugi::xml_document &Doc);

namespace cities {

#define CITIES_FILE "cities.xml"

struct City {
  std::string name; // имя города
  std::vector<int> vnums; // номера зон, которые принадлежат городу
  int rent_vnum{kNowhere}; // внум ренты города
};

std::vector<City> cities_roster;
std::string default_str_cities;

std::size_t CountCities() {
	return cities_roster.size();
}

void CheckCityVisit(CharData *ch, RoomRnum room_rnum) {
	for (std::size_t i = 0; i < cities_roster.size(); i++) {
		if (GET_ROOM_VNUM(room_rnum) == cities_roster[i].rent_vnum) {
			ch->mark_city(i);
			return;
		}
	}
}

void LoadCities() {
	default_str_cities = "";
	pugi::xml_document doc_cities;
	pugi::xml_node child_, object_, file_;
	file_ = XmlLoad(LIB_MISC CITIES_FILE, "cities", "Error loading cases file: cities.xml", doc_cities);
	for (child_ = file_.child("city"); child_; child_ = child_.next_sibling("city")) {
		City city;
		city.name = child_.child("name").attribute("value").as_string();
		city.rent_vnum = child_.child("rent_vnum").attribute("value").as_int();
		for (object_ = child_.child("ZoneVnum"); object_; object_ = object_.next_sibling("ZoneVnum")) {
			city.vnums.push_back(object_.attribute("value").as_int());
		}
		cities_roster.push_back(city);
		default_str_cities += "0";
	}
}

void DoCities(CharData *ch, char *, int, int) {
	SendMsgToChar("Города на Руси:\r\n", ch);
	for (unsigned int i = 0; i < cities_roster.size(); i++) {
		sprintf(buf, "%3d.", i + 1);
		if (IS_IMMORTAL(ch)) {
			sprintf(buf1, " [VNUM: %d]", cities_roster[i].rent_vnum);
			strcat(buf, buf1);
		}
		sprintf(buf1,
				" %s: %s\r\n",
				cities_roster[i].name.c_str(),
				(ch->check_city(i) ? "&gВы были там.&n" : "&rВы еще не были там.&n"));
		strcat(buf, buf1);
		SendMsgToChar(buf, ch);
	}
}

bool IsCharInCity(CharData *ch) {
	for (auto & city : cities_roster) {
		if (GetZoneVnumByCharPlace(ch) == city.rent_vnum / 100) {
			return true;
		}
	}
	return false;
}

} // namespace cities

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
