// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "noob.h"

#include "entities/char_data.h"
#include "utils/pugixml/pugixml.h"
#include "utils/parse.h"
#include "handler.h"

int find_eq_pos(CharData *ch, ObjData *obj, char *arg);

namespace Noob {

const char *CONFIG_FILE = LIB_MISC"noob_help.xml";
// макс уровень чара, который считается нубом (is_noob в коде и тригах, из CONFIG_FILE)
int MAX_LEVEL = 0;
// список классов (по id) со списками шмоток (vnum) в каждом (из CONFIG_FILE)
std::array<std::vector<int>, kNumPlayerClasses> class_list;

///
/// чтение конфига из misc/noob_help.xml (CONFIG_FILE)
///
void init() {
	// для релоада на случай ошибок при чтении
	std::array<std::vector<int>, kNumPlayerClasses> tmp_class_list;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	const pugi::xml_node root_node = parse::GetChild(doc, "noob_help");
	if (!root_node) return;

	// <noob max_lvl="" max_money="" wait_period="" />
	pugi::xml_node cur_node = parse::GetChild(root_node, "noob");
	if (cur_node) {
		MAX_LEVEL = parse::ReadAttrAsInt(cur_node, "max_lvl");
	}

	// <all_classes>
	cur_node = parse::GetChild(root_node, "all_classes");
	if (!cur_node) return;

	for (pugi::xml_node obj_node = cur_node.child("obj");
		 obj_node; obj_node = obj_node.next_sibling("obj")) {
		int vnum = parse::ReadAttrAsInt(obj_node, "vnum");
		if (parse::IsValidObjVnum(vnum)) {
			for (auto & i : tmp_class_list) {
				i.push_back(vnum);
			}
		}
	}

	// <class id="">
	for (cur_node = root_node.child("class");
		 cur_node; cur_node = cur_node.next_sibling("class")) {
		auto id{ECharClass::kUndefined};
		std::string id_str = parse::ReadAattrAsStr(cur_node, "id");
		try {
			id = parse::ReadAsConstant<ECharClass>(id_str.c_str());
		} catch (std::exception &) {
			snprintf(buf, kMaxStringLength, "...<class id='%s'> convert fail", id_str.c_str());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}

		for (pugi::xml_node obj_node = cur_node.child("obj");
			 obj_node; obj_node = obj_node.next_sibling("obj")) {
			int vnum = parse::ReadAttrAsInt(obj_node, "vnum");
			if (parse::IsValidObjVnum(vnum)) {
				tmp_class_list[to_underlying(id)].push_back(vnum);
			}
		}
	}

	class_list = tmp_class_list;
}

///
/// \return true - если ch в коде считается нубом и соотв-но претендует на помощь
///
bool is_noob(const CharData *ch) {
	if (GetRealLevel(ch) > MAX_LEVEL || GET_REAL_REMORT(ch) > 0) {
		return false;
	}
	return true;
}

///
/// Пустой спешиал, чтобы не морочить голову с перебором тригов в карте
///
int outfit(CharData * /*ch*/, void * /*me*/, int/* cmd*/, char * /*argument*/) {
	return 0;
}

///
/// \return строка с внумами стартовых предметов персонажа
/// нужно для тригов (%actor.noob_outfit%)
///
std::string print_start_outfit(CharData *ch) {
	std::stringstream out;
	std::vector<int> tmp(get_start_outfit(ch));
	for (const auto &item : tmp) {
		out << item << " ";
	}
	return out.str();
}

///
/// \return список внумов стартовых шмоток из noob_help.xml
/// + шмоток, завясящих от местонахождения чара из birthplaces.xml
///
std::vector<int> get_start_outfit(CharData *ch) {
	// стаф из noob_help.xml
	std::vector<int> out_list;
	const int ch_class = to_underlying(ch->get_class());
	if (ch_class < kNumPlayerClasses) {
		out_list.insert(out_list.end(),
						class_list.at(ch_class).begin(), class_list.at(ch_class).end());
	}
	// стаф из birthplaces.xml (карты родовых)
	int birth_id = Birthplaces::GetIdByRoom(GET_ROOM_VNUM(ch->in_room));
	if (birth_id >= 0) {
		std::vector<int> tmp = Birthplaces::GetItemList(birth_id);
		out_list.insert(out_list.end(), tmp.begin(), tmp.end());
	}
	return out_list;
}

///
/// \return указатель на моба-рентера в данной комнате или 0
///
CharData *find_renter(int room_rnum) {
	for (const auto tch : world[room_rnum]->people) {
		if (GET_MOB_SPEC(tch) == receptionist) {
			return tch;
		}
	}

	return nullptr;
}

///
/// Проверка при входе в игру чара на ренте, при необходимости выдача
/// сообщения о возможности получить стартовую экипу у кладовщика.
/// Сообщение берется из birthplaces.xml или дефолтное из birthplaces::GetRentHelp
///
void check_help_message(CharData *ch) {
	if (Noob::is_noob(ch)
		&& GET_HIT(ch) <= 1
		&& IS_CARRYING_N(ch) <= 0
		&& IS_CARRYING_W(ch) <= 0) {
		int birth_id = Birthplaces::GetIdByRoom(GET_ROOM_VNUM(ch->in_room));
		if (birth_id >= 0) {
			CharData *renter = find_renter(ch->in_room);
			std::string text = Birthplaces::GetRentHelp(birth_id);
			if (renter && !text.empty()) {
				act("\n\\u$n оглядел$g вас с головы до пят.", true, renter, nullptr, ch, kToVict);
				act("$n посмотрел$g на $N3.", true, renter, nullptr, ch, kToNotVict);
				tell_to_char(renter, ch, text.c_str());
			}
		}
	}
}

///
/// Автоматическое надевание и вооружение только что созданного чара при входе
/// в игру со стартовой экипой. При вооржении пушки чару ставится ее скилл.
/// Богатырям при надевании перчаток сетится кулачный бой.
///
void equip_start_outfit(CharData *ch, ObjData *obj) {
	if (GET_OBJ_TYPE(obj) == EObjType::kArmor) {
		int where = find_eq_pos(ch, obj, nullptr);
		if (where >= 0) {
			EquipObj(ch, obj, where, CharEquipFlags());
			// богатырям в перчатках сетим кулачный бой вместо пушек
			if (where == EEquipPos::kHands && GET_CLASS(ch) == ECharClass::kWarrior) {
				ch->set_skill(ESkill::kPunch, 10);
			}
		}
	} else if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		if (CAN_WEAR(obj, EWearFlag::kWield)
			&& !GET_EQ(ch, EEquipPos::kWield)) {
			EquipObj(ch, obj, EEquipPos::kWield, CharEquipFlags());
			ch->set_skill(static_cast<ESkill>(GET_OBJ_SKILL(obj)), 10);
		} else if (CAN_WEAR(obj, EWearFlag::kBoth)
			&& !GET_EQ(ch, EEquipPos::kBoths)) {
			EquipObj(ch, obj, EEquipPos::kBoths, CharEquipFlags());
			ch->set_skill(static_cast<ESkill>(GET_OBJ_SKILL(obj)), 10);
		} else if (CAN_WEAR(obj, EWearFlag::kHold)
			&& !GET_EQ(ch, EEquipPos::kHold)) {
			EquipObj(ch, obj, EEquipPos::kHold, CharEquipFlags());
			ch->set_skill(static_cast<ESkill>(GET_OBJ_SKILL(obj)), 10);
		}
	}
}

} // namespace Noob

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
