//
// Created by Sventovit on 07.09.2024.
//

#include "stable_objs.h"

#include "engine/boot/boot_constants.h"
#include "engine/entities/obj_data.h"
#include "gameplay/core/constants.h"
#include "utils/utils.h"

extern pugi::xml_node XmlLoad(const char *PathToFile,
							  const char *MainTag,
							  const char *ErrorStr,
							  pugi::xml_document &Doc);

namespace stable_objs {

#define CRITERION_FILE "criterion.xml"

struct UndecayableCriterions {
  std::map<std::string, double> params;
  std::map<std::string, double> affects;
};

// массив для критерии, каждый элемент массива это отдельный слот
UndecayableCriterions undecayable_criterions[17]; // = new Item_struct[16];

void LoadCriterion(pugi::xml_node criterion, EWearFlag type);
double CountUnlimitedTimerBonus(const CObjectPrototype *obj, int item_wear);

// определение степени двойки
int DeterminePowerOfTwo(int number) {
	int count = 0;
	while (true) {
		const int tmp = 1 << count;
		if (number < tmp) {
			return -1;
		}
		if (number == tmp) {
			return count;
		}
		count++;
	}
}

template<typename EnumType>
inline int DeterminePowerOfTwoForEnum(EnumType number) {
	return DeterminePowerOfTwo(to_underlying(number));
}

double CountUnlimitedTimer(const CObjectPrototype *obj) {
	bool type_item = false;
	if (obj->get_type() == EObjType::kArmor
		|| obj->get_type() == EObjType::kStaff
		|| obj->get_type() == EObjType::kWorm
		|| obj->get_type() == EObjType::kWeapon) {
		type_item = true;
	}
	// сумма для статов

	double result{0.0};
	if (CAN_WEAR(obj, EWearFlag::kFinger)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kFinger));
	}

	if (CAN_WEAR(obj, EWearFlag::kNeck)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kNeck));
	}

	if (CAN_WEAR(obj, EWearFlag::kBody)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kBody));
	}

	if (CAN_WEAR(obj, EWearFlag::kHead)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kHead));
	}

	if (CAN_WEAR(obj, EWearFlag::kLegs)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kLegs));
	}

	if (CAN_WEAR(obj, EWearFlag::kFeet)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kFeet));
	}

	if (CAN_WEAR(obj, EWearFlag::kHands)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kHands));
	}

	if (CAN_WEAR(obj, EWearFlag::kArms)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kArms));
	}

	if (CAN_WEAR(obj, EWearFlag::kShield)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kShield));
	}

	if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kShoulders));
	}

	if (CAN_WEAR(obj, EWearFlag::kWaist)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kWaist));
	}

	if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kQuiver));
	}

	if (CAN_WEAR(obj, EWearFlag::kWrist)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kWrist));
	}

	if (CAN_WEAR(obj, EWearFlag::kWield)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kWield));
	}

	if (CAN_WEAR(obj, EWearFlag::kHold)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kHold));
	}

	if (CAN_WEAR(obj, EWearFlag::kBoth)) {
		result += CountUnlimitedTimerBonus(obj, DeterminePowerOfTwoForEnum(EWearFlag::kBoth));
	}

	if (!type_item) {
		return 0.0;
	}

	return result;
}

double CountUnlimitedTimerBonus(const CObjectPrototype *obj, int item_wear) {
	double sum = 0.0;
	double sum_aff = 0.0;
	char buf_temp[kMaxStringLength];
	char buf_temp1[kMaxStringLength];

	// проходим по всем характеристикам предмета
	for (int i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).modifier) {
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// проходим по нашей таблице с критериями
			for (auto &param : undecayable_criterions[item_wear].params) {
				if (strcmp(param.first.c_str(), buf_temp) == 0) {
					if (obj->get_affected(i).modifier > 0) {
						sum += param.second * obj->get_affected(i).modifier;
					}
				}

				//std::cout << it->first << " " << it->second << "\r\n";
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");

	for (auto &affect : undecayable_criterions[item_wear].affects) {
		if (strstr(buf_temp1, affect.first.c_str()) != nullptr) {
			sum_aff += affect.second;
		}
	}
	sum += sum_aff;
	return sum;
}

bool IsTimerUnlimited(const CObjectPrototype *obj) {
	char buf_temp[kMaxStringLength];
	char buf_temp1[kMaxStringLength];
	//sleep(15);
	// куда надевается наш предмет.
	int item_wear = -1;
	bool type_item = false;
	if (obj->get_type() == EObjType::kArmor
		|| obj->get_type() == EObjType::kStaff
		|| obj->get_type() == EObjType::kWorm
		|| obj->get_type() == EObjType::kWeapon) {
		type_item = true;
	}
	// сумма для статов
	double sum = 0;
	// сумма для аффектов
	double sum_aff = 0;
	// по другому чот не получилось
	if (obj->has_wear_flag(EWearFlag::kFinger)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kFinger);
	}

	if (obj->has_wear_flag(EWearFlag::kNeck)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kNeck);
	}

	if (obj->has_wear_flag(EWearFlag::kBody)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kBody);
	}

	if (obj->has_wear_flag(EWearFlag::kHead)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kHead);
	}

	if (obj->has_wear_flag(EWearFlag::kLegs)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kLegs);
	}

	if (obj->has_wear_flag(EWearFlag::kFeet)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kFeet);
	}

	if (obj->has_wear_flag(EWearFlag::kHands)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kHands);
	}

	if (obj->has_wear_flag(EWearFlag::kArms)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kArms);
	}

	if (obj->has_wear_flag(EWearFlag::kShield)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kShield);
	}

	if (obj->has_wear_flag(EWearFlag::kShoulders)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kShoulders);
	}

	if (obj->has_wear_flag(EWearFlag::kWaist)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kWaist);
	}

	if (obj->has_wear_flag(EWearFlag::kQuiver)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kQuiver);
	}

	if (obj->has_wear_flag(EWearFlag::kWrist)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kWrist);
	}

	if (obj->has_wear_flag(EWearFlag::kBoth)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kBoth);
	}

	if (obj->has_wear_flag(EWearFlag::kWield)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kWield);
	}

	if (obj->has_wear_flag(EWearFlag::kHold)) {
		item_wear = DeterminePowerOfTwoForEnum(EWearFlag::kHold);
	}

	if (!type_item) {
		return false;
	}
	// находится ли объект в системной зоне
	if (IsObjFromSystemZone(obj->get_vnum())) {
		return false;
	}
	// если объект никуда не надевается, то все, облом
	if (item_wear == -1) {
		return false;
	}
	// если шмотка магическая или энчантнута таймер обычный
	if (obj->has_flag(EObjFlag::kMagic)) {
		return false;
	}
	// если это сетовый предмет
	if (obj->has_flag(EObjFlag::KSetItem)) {
		return false;
	}
	// !нерушима
	if (obj->has_flag(EObjFlag::KLimitedTimer)) {
		return false;
	}
	// рассыпется вне зоны
	if (obj->has_flag(EObjFlag::kZonedecay)) {
		return false;
	}
	// рассыпется на репоп зоны
	if (obj->has_flag(EObjFlag::kRepopDecay)) {
		return false;
	}

	// если предмет требует реморты, то он явно овер
	if (obj->get_auto_mort_req() > 0)
		return false;
	if (obj->get_minimum_remorts() > 0)
		return false;
	// проверяем дырки в предмете
	if (obj->has_flag(EObjFlag::kHasOneSlot)
		|| obj->has_flag(EObjFlag::kHasTwoSlots)
		|| obj->has_flag(EObjFlag::kHasThreeSlots)) {
		return false;
	}
	// если у объекта таймер ноль, то облом.
	if (obj->get_timer() == 0) {
		return false;
	}
	// проходим по всем характеристикам предмета
	for (int i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).modifier) {
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// проходим по нашей таблице с критериями
			for (auto &param : undecayable_criterions[item_wear].params) {
				if (strcmp(param.first.c_str(), buf_temp) == 0) {
					if (obj->get_affected(i).modifier > 0) {
						sum += param.second * obj->get_affected(i).modifier;
					}
				}
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");

	// проходим по всем аффектам в нашей таблице
	for (auto &affect : undecayable_criterions[item_wear].affects) {
		// проверяем, есть ли наш аффект на предмете
		if (strstr(buf_temp1, affect.first.c_str()) != nullptr) {
			sum_aff += affect.second;
		}
		//std::cout << it->first << " " << it->second << "\r\n";
	}

	// если сумма больше или равна единице
	if (sum >= 1) {
		return false;
	}

	// тоже самое для аффектов на объекте
	if (sum_aff >= 1) {
		return false;
	}

	// иначе все норм
	return true;
}

bool IsObjFromSystemZone(ObjVnum vnum) {
	// ковка
	if ((vnum < 400) && (vnum > 299))
		return true;
	// сет-шмот
	if ((vnum >= 1200) && (vnum <= 1299))
		return true;
	if ((vnum >= 2300) && (vnum <= 2399))
		return true;
	// луки
	if ((vnum >= 1500) && (vnum <= 1599))
		return true;
	return false;
}

void LoadCriterionsCfg() {
	pugi::xml_document doc1;
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "finger", "Error Loading Criterion.xml: <finger>", doc1),
				  EWearFlag::kFinger);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "neck", "Error Loading Criterion.xml: <neck>", doc1),
				  EWearFlag::kNeck);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "body", "Error Loading Criterion.xml: <body>", doc1),
				  EWearFlag::kBody);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "head", "Error Loading Criterion.xml: <head>", doc1),
				  EWearFlag::kHead);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "legs", "Error Loading Criterion.xml: <legs>", doc1),
				  EWearFlag::kLegs);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "feet", "Error Loading Criterion.xml: <feet>", doc1),
				  EWearFlag::kFeet);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "hands", "Error Loading Criterion.xml: <hands>", doc1),
				  EWearFlag::kHands);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "arms", "Error Loading Criterion.xml: <arms>", doc1),
				  EWearFlag::kArms);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "shield", "Error Loading Criterion.xml: <shield>", doc1),
				  EWearFlag::kShield);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "about", "Error Loading Criterion.xml: <about>", doc1),
				  EWearFlag::kShoulders);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "waist", "Error Loading Criterion.xml: <waist>", doc1),
				  EWearFlag::kWaist);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "waist", "Error Loading Criterion.xml: <quiver>", doc1),
				  EWearFlag::kQuiver);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "wrist", "Error Loading Criterion.xml: <wrist>", doc1),
				  EWearFlag::kWrist);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "boths", "Error Loading Criterion.xml: <boths>", doc1),
				  EWearFlag::kBoth);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "wield", "Error Loading Criterion.xml: <wield>", doc1),
				  EWearFlag::kWield);
	LoadCriterion(XmlLoad(LIB_MISC CRITERION_FILE, "hold", "Error Loading Criterion.xml: <hold>", doc1),
				  EWearFlag::kHold);
};

void LoadCriterion(pugi::xml_node criterion, const EWearFlag type) {
	int index = DeterminePowerOfTwoForEnum(type);
	pugi::xml_node params, CurNode, affects;
	params = criterion.child("params");
	affects = criterion.child("affects");

	// добавляем в массив все параметры, типа силы, ловкости, каст и тд
	for (CurNode = params.child("param"); CurNode; CurNode = CurNode.next_sibling("param")) {
		undecayable_criterions[index].params.insert(std::make_pair(CurNode.attribute("name").value(),
																   CurNode.attribute("value").as_double()));
		//log("str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}

	// добавляем в массив все аффекты
	for (CurNode = affects.child("affect"); CurNode; CurNode = CurNode.next_sibling("affect")) {
		undecayable_criterions[index].affects.insert(std::make_pair(CurNode.attribute("name").value(),
																	CurNode.attribute("value").as_double()));
		//log("Affects:str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}
}

} // namespace stable_obj

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
