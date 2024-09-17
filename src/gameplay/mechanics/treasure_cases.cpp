//
// Created by Sventovit on 07.09.2024.
//

#include "treasure_cases.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/db/world_objects.h"
#include "engine/core/handler.h"
#include "engine/boot/boot_constants.h"

#include <third_party_libs/fmt/include/fmt/format.h>
#include "third_party_libs/pugixml/pugixml.h"

extern pugi::xml_node XmlLoad(const char *PathToFile,
							  const char *MainTag,
							  const char *ErrorStr,
							  pugi::xml_document &Doc);

namespace treasure_cases {

#define CASES_FILE "cases.xml"

struct TreasureCase {
  ObjVnum vnum{};
  int drop_chance{};
  std::vector<ObjVnum> vnum_objs;
};

std::vector<TreasureCase> cases;

void LoadTreasureCases() {
	pugi::xml_document doc_cases;
	pugi::xml_node case_, object_, file_case;
	file_case = XmlLoad(LIB_MISC CASES_FILE, "cases", "Error loading cases file: cases.xml", doc_cases);
	for (case_ = file_case.child("casef"); case_; case_ = case_.next_sibling("casef")) {
		TreasureCase treasure_case;
		treasure_case.vnum = case_.attribute("vnum").as_int();
		treasure_case.drop_chance = case_.attribute("drop_chance").as_int();
		for (object_ = case_.child("object"); object_; object_ = object_.next_sibling("object")) {
			treasure_case.vnum_objs.push_back(object_.attribute("vnum").as_int());
		}
		cases.push_back(treasure_case);
	}
}

void UnlockTreasureCase(CharData *ch, ObjData *obj) {
	for (auto &i : cases) {
		if (GET_OBJ_VNUM(obj) == i.vnum) {
			if (!AFF_FLAGGED(ch, EAffect::kDeafness)) {
				SendMsgToChar("&GГде-то далеко наверху раздалась звонкая музыка.&n\r\n", ch);
			}
			// drop_chance = cases[i].drop_chance;
			// drop_chance пока что не учитывается, просто падает одна рандомная стафина из всего этого
			const int maximal_chance = static_cast<int>(i.vnum_objs.size() - 1);
			const int random_number = number(0, maximal_chance);
			auto vnum = i.vnum_objs[random_number];
			ObjRnum r_num;
			if ((r_num = GetObjRnum(vnum)) < 0) {
				act("$o исчез$Y с яркой вспышкой. Случилось неладное, поняли вы..",
					false,
					ch,
					obj,
					nullptr,
					kToRoom);
				const auto msg = fmt::format(
					"[ERROR] treasure cases: ошибка при открытии контейнера {}, неизвестное содержимое!",
					obj->get_vnum());
				mudlog(msg, LogMode::CMP, kLvlGreatGod, MONEY_LOG, true);
				return;
			}
			// сначала удалим ключ из инвентаря
			int vnum_key = GET_OBJ_VAL(obj, 2);
			// первый предмет в инвентаре
			ObjData *obj_inv = ch->carrying;
			for (auto p_obj_data = obj_inv; p_obj_data; p_obj_data = p_obj_data->get_next_content()) {
				if (GET_OBJ_VNUM(p_obj_data) == vnum_key) {
					ExtractObjFromWorld(p_obj_data);
					break;
				}
			}
			ExtractObjFromWorld(obj);
			obj = world_objects.create_from_prototype_by_rnum(r_num).get();
			obj->set_crafter_uid(GET_UID(ch));
			PlaceObjToInventory(obj, ch);
			act("$n завизжал$g от радости.", false, ch, nullptr, nullptr, kToRoom);
			load_otrigger(obj);
			CheckObjDecay(obj);
			olc_log("%s load obj %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), vnum);
		}
	}
}

} // namespace treasure_cases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
