// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "noob.h"

#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/cities.h"
#include "utils/parser_wrapper.h"
#include "utils/parse.h"
#include "engine/core/handler.h"
#include "gameplay/communication/talk.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/core/remort.h"

int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);

namespace Noob {

// макс уровень чара, который считается нубом (is_noob в коде и тригах, из конфига)
int MAX_LEVEL = 0;
// список классов (по id) со списками шмоток (vnum) в каждом (из конфига)
std::array<std::vector<int>, kNumPlayerClasses> class_list;

// Целочисленный атрибут DataNode; def при отсутствии/некорректном значении.
using parse::AttrInt;

///
/// Чтение конфига cfg/mechanics/noob.xml через cfg_manager (boot + reload noobhelp).
///
void NoobLoader::Load(parser_wrapper::DataNode data) {
	// собираем во временный список, чтобы при ошибке не повредить рабочий
	std::array<std::vector<int>, kNumPlayerClasses> tmp_class_list;
	int tmp_max_level = MAX_LEVEL;

	// <noob max_lvl="" />
	{
		auto node = data;
		if (node.GoToChild("noob")) {
			tmp_max_level = AttrInt(node, "max_lvl", 0);
		}
	}

	// <all_classes> -- стаф для всех профессий
	{
		auto node = data;
		if (node.GoToChild("all_classes")) {
			for (auto &obj_node : node.Children("obj")) {
				const int vnum = AttrInt(obj_node, "vnum", 0);
				if (parse::IsValidObjVnum(vnum)) {
					for (auto &i : tmp_class_list) {
						i.push_back(vnum);
					}
				}
			}
		}
	}

	// <class id=""> -- стаф конкретной профессии
	for (auto &class_node : data.Children("class")) {
		const char *id_str = class_node.GetValue("id");
		ECharClass id;
		try {
			id = parse::ReadAsConstant<ECharClass>(id_str);
		} catch (const std::exception &) {
			snprintf(buf, kMaxStringLength, "...<class id='%s'> convert fail", id_str ? id_str : "");
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;   // откат: рабочий class_list/MAX_LEVEL не трогаем
		}
		for (auto &obj_node : class_node.Children("obj")) {
			const int vnum = AttrInt(obj_node, "vnum", 0);
			if (parse::IsValidObjVnum(vnum)) {
				tmp_class_list[to_underlying(id)].push_back(vnum);
			}
		}
	}

	class_list = tmp_class_list;
	MAX_LEVEL = tmp_max_level;
}

void NoobLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

///
/// \return true - если ch в коде считается нубом и соотв-но претендует на помощь
///
bool is_noob(const CharData *ch) {
	if (GetRealLevel(ch) > MAX_LEVEL || remort::GetRealRemort(ch) > 0) {
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
/// \return список внумов стартовых шмоток из noob_help.xml (зависит только от класса).
/// Городской стартовый стаф выдается отдельно в give_city_start_outfit() уже ПОСЛЕ того,
/// как чар помещен в комнату (иначе город еще не известен).
///
std::vector<int> get_start_outfit(CharData *ch) {
	// стаф из noob_help.xml
	std::vector<int> out_list;
	const int ch_class = to_underlying(ch->GetClass());
	if (ch_class < kNumPlayerClasses) {
		out_list.insert(out_list.end(),
						class_list.at(ch_class).begin(), class_list.at(ch_class).end());
	}
	return out_list;
}

/// Выдает (и надевает) стартовый стаф города из cities.xml (<start_item>), исходя из города,
/// в котором РЕАЛЬНО появился чар. Вызывать только после char_to_room() -- до помещения в
/// комнату ch->in_room еще не указывает на стартовый город.
void give_city_start_outfit(CharData *ch) {
	for (const int vnum : cities::StartItemsForRoom(GET_ROOM_VNUM(ch->in_room))) {
		const ObjData::shared_ptr obj = world_objects.create_from_prototype_by_vnum(vnum);
		if (!obj) {
			continue;
		}
		obj->set_extra_flag(EObjFlag::kNosell);
		obj->set_extra_flag(EObjFlag::kDecay);
		obj->set_cost(0);
		obj->set_rent_off(0);
		obj->set_rent_on(0);
		PlaceObjToInventory(obj.get(), ch);
		equip_start_outfit(ch, obj.get());
	}
}

///
/// \return указатель на моба-рентера в данной комнате или 0
///
CharData *find_renter(int room_rnum) {
	for (const auto tch : world[room_rnum]->people) {
		if (specials::IsMobSpecial(GET_MOB_VNUM(tch), specials::ESpecial::kRent)) {
			return tch;
		}
	}

	return nullptr;
}

///
/// Проверка при входе в игру чара на ренте, при необходимости выдача
/// сообщения о возможности получить стартовую экипу у кладовщика.
/// Сообщение выдается мобом-рентером (кладовщиком) нубу в стартовом городе.
///
void check_help_message(CharData *ch) {
	static const char *kRentHelp = "Попроси нашего кладовщика помочь тебе с экипировкой и припасами.";
	if (Noob::is_noob(ch)
		&& ch->get_hit() <= 1
		&& ch->GetCarryingQuantity() <= 0
		&& ch->GetCarryingWeight() <= 0
		&& cities::IsCharInCity(ch)) {
		CharData *renter = find_renter(ch->in_room);
		if (renter) {
			act("\n\\u$n оглядел$g вас с головы до пят.", true, renter, nullptr, ch, kToVict);
			act("$n посмотрел$g на $N3.", true, renter, nullptr, ch, kToNotVict);
			tell_to_char(renter, ch, kRentHelp);
		}
	}
}

///
/// Автоматическое надевание и вооружение только что созданного чара при входе
/// в игру со стартовой экипой. При вооржении пушки чару ставится ее скилл.
/// Богатырям при надевании перчаток сетится кулачный бой.
///
void equip_start_outfit(CharData *ch, ObjData *obj) {
	if (obj->get_type() == EObjType::kArmor) {
		int where = find_eq_pos(ch, obj, nullptr);
		if (where >= 0) {
			EquipObj(ch, obj, where, CharEquipFlags());
			// богатырям в перчатках сетим кулачный бой вместо пушек
			if (where == EEquipPos::kHands && ch->GetClass() == ECharClass::kWarrior) {
				SetSkill(ch, ESkill::kPunch, 10);
			}
		}
	} else if (obj->get_type() == EObjType::kWeapon) {
		if (CAN_WEAR(obj, EWearFlag::kWield)
			&& !GET_EQ(ch, EEquipPos::kWield)) {
			EquipObj(ch, obj, EEquipPos::kWield, CharEquipFlags());
			SetSkill(ch, static_cast<ESkill>(obj->get_spec_param()), 10);
		} else if (CAN_WEAR(obj, EWearFlag::kBoth)
			&& !GET_EQ(ch, EEquipPos::kBoths)) {
			EquipObj(ch, obj, EEquipPos::kBoths, CharEquipFlags());
			SetSkill(ch, static_cast<ESkill>(obj->get_spec_param()), 10);
		} else if (CAN_WEAR(obj, EWearFlag::kHold)
			&& !GET_EQ(ch, EEquipPos::kHold)) {
			EquipObj(ch, obj, EEquipPos::kHold, CharEquipFlags());
			SetSkill(ch, static_cast<ESkill>(obj->get_spec_param()), 10);
		}
	}
}

} // namespace Noob

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
