//
// Created by Sventovit on 07.09.2024.
//

#include "engine/boot/boot_constants.h"

#include <map>
#include <vector>

#include "third_party_libs/pugixml/pugixml.h"

extern pugi::xml_node XmlLoad(const char *PathToFile,
							  const char *MainTag,
							  const char *ErrorStr,
							  pugi::xml_document &Doc);

namespace bodrich_quest {

#define QUESTBODRICH_FILE "quest_bodrich.xml"

struct QuestBodrichRewards {
  int level;
  int vnum;
  int money;
  int exp;
};

class QuestBodrich {
 public:
  QuestBodrich();

 private:
  void LoadMobs();
  void LoadObjs();
  void LoadRewards();

  // здесь храним предметы для каждого класса
  std::map<int, std::vector<int>> objs;
  // здесь храним мобов
  std::map<int, std::vector<int>> mobs;
  // а здесь награды
  std::map<int, std::vector<QuestBodrichRewards>> rewards;
};

QuestBodrich::QuestBodrich() {
	this->LoadObjs();
	this->LoadMobs();
	this->LoadRewards();
}

void QuestBodrich::LoadObjs() {
	pugi::xml_document doc_;
	pugi::xml_node class_, file_, object_;
	file_ = XmlLoad(LIB_MISC QUESTBODRICH_FILE, "objects", "Error loading obj file: quest_bodrich.xml", doc_);
	std::vector<int> tmp_array;
	for (class_ = file_.child("class"); class_; class_ = class_.next_sibling("class")) {
		tmp_array.clear();
		for (object_ = object_.child("obj"); object_; object_ = object_.next_sibling("obj")) {
			tmp_array.push_back(object_.attribute("vnum").as_int());
		}
		this->objs.insert(std::pair<int, std::vector<int>>(class_.attribute("id").as_int(), tmp_array));
	}
}

void QuestBodrich::LoadMobs() {
	pugi::xml_document doc_;
	pugi::xml_node level_, file_, mob_;
	file_ = XmlLoad(LIB_MISC QUESTBODRICH_FILE, "mobs", "Error loading mobs file: quest_bodrich.xml", doc_);
	std::vector<int> tmp_array;
	for (level_ = file_.child("level"); level_; level_ = level_.next_sibling("level")) {
		tmp_array.clear();
		for (mob_ = mob_.child("mob"); mob_; mob_ = mob_.next_sibling("mob")) {
			tmp_array.push_back(mob_.attribute("vnum").as_int());
		}
		this->mobs.insert(std::pair<int, std::vector<int>>(level_.attribute("value").as_int(), tmp_array));
	}
}

void QuestBodrich::LoadRewards() {
	pugi::xml_document doc_;
	pugi::xml_node class_, file_, object_, level_;
	file_ = XmlLoad(LIB_MISC QUESTBODRICH_FILE, "rewards", "Error loading rewards file: quest_bodrich.xml", doc_);
	std::vector<QuestBodrichRewards> tmp_array;
	for (class_ = file_.child("class"); class_; class_ = class_.next_sibling("class")) {
		tmp_array.clear();
		for (level_ = level_.child("level"); level_; level_ = level_.next_sibling("level")) {
			QuestBodrichRewards qbr{};
			qbr.level = level_.attribute("level").as_int();
			qbr.vnum = level_.attribute("ObjVnum").as_int();
			qbr.money = level_.attribute("money_value").as_int();
			qbr.exp = level_.attribute("exp_value").as_int();
			tmp_array.push_back(qbr);
		}
		//std::map<int, QuestBodrichRewards> rewards;
		this->rewards.insert(std::pair<int, std::vector<QuestBodrichRewards>>(class_.attribute("id").as_int(),
			tmp_array));
	}
}

} // namespace bodrich_quest

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
