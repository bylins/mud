//
// Created by Sventovit on 04.09.2024.
//

#include "mob_races.h"

#include "engine/boot/boot_constants.h"
#include "engine/entities/char_data.h"

namespace mob_races {

const char *MOBRACE_FILE = LIB_CFG "mob_races.xml";

MobRaceListType mobraces_list;

//загрузка рас из файла
void LoadMobraces() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOBRACE_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node node_list = doc.child("mob_races");

	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...mob races read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	for (auto &race : node_list.children("mob_race")) {
		MobRacePtr tmp_mobrace(new MobRace);
		auto race_num = race.attribute("id").as_int();
		tmp_mobrace->race_name = race.attribute("name").value();

		pugi::xml_node imList = race.child("imlist");

		for (pugi::xml_node im = imList.child("im"); im; im = im.next_sibling("im")) {
			struct ingredient tmp_ingr;
			tmp_ingr.imtype = im.attribute("type").as_int();
			tmp_ingr.imname = std::string(im.attribute("name").value());
			utils::Trim(tmp_ingr.imname);
			int cur_lvl = 1;
			int prob_value = 1;
			for (pugi::xml_node prob = im.child("prob"); prob; prob = prob.next_sibling("prob")) {
				int next_lvl = prob.attribute("lvl").as_int();
				if (next_lvl > 0) {
					for (int lvl = cur_lvl; lvl < next_lvl; lvl++)
						tmp_ingr.prob[lvl - 1] = prob_value;
				} else {
					log("SYSERROR: Неверный уровень lvl=%d для ингредиента %s расы %s",
						next_lvl,
						tmp_ingr.imname.c_str(),
						tmp_mobrace->race_name.c_str());
					return;
				}
				prob_value = atoi(prob.child_value());
				cur_lvl = next_lvl;
			}
			for (int lvl = cur_lvl; lvl <= kMaxMobLevel; lvl++)
				tmp_ingr.prob[lvl - 1] = prob_value;

			tmp_mobrace->ingrlist.push_back(tmp_ingr);
		}
		mobraces_list[race_num] = tmp_mobrace;
	}
}

MobRace::MobRace() {
	ingrlist.clear();
}

MobRace::~MobRace() {
	ingrlist.clear();
}

} // namespace mob_races

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
