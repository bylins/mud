//
// Created by Sventovit on 07.09.2024.
//

#include "stable_objs.h"

#include "engine/entities/entities_constants.h"
#include "engine/entities/obj_data.h"
#include "gameplay/affects/affect_contants.h"
#include "utils/utils_parse.h"
#include "utils/parser_wrapper.h"
#include "utils/utils.h"

#include <map>
#include <string>
#include <vector>

namespace stable_objs {

struct UndecayableCriterions {
  std::map<EApply, double> params;          // вес параметра предмета (EApply)
  std::map<EEquipmentAffect, double> affects;  // вес аффекта предмета (EEquipmentAffect)
};

// массив для критерии, каждый элемент массива это отдельный слот (индекс = степень двойки EWearFlag)
UndecayableCriterions undecayable_criterions[17]; // = new Item_struct[16];

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

// Суммарный вес положительных параметров предмета по критериям слота item_wear.
double SumParamWeights(const CObjectPrototype *obj, int item_wear) {
	double sum = 0.0;
	const auto &params = undecayable_criterions[item_wear].params;
	for (int i = 0; i < kMaxObjAffect; i++) {
		const auto &af = obj->get_affected(i);
		if (af.modifier > 0) {
			const auto it = params.find(af.location);
			if (it != params.end()) {
				sum += it->second * af.modifier;
			}
		}
	}
	return sum;
}

// Суммарный вес аффектов предмета по критериям слота item_wear.
double SumAffectWeights(const CObjectPrototype *obj, int item_wear) {
	double sum = 0.0;
	for (const auto &[equipment_affect, weight] : undecayable_criterions[item_wear].affects) {
		if (obj->GetEEquipmentAffect(equipment_affect)) {
			sum += weight;
		}
	}
	return sum;
}

double CountUnlimitedTimerBonus(const CObjectPrototype *obj, int item_wear) {
	return SumParamWeights(obj, item_wear) + SumAffectWeights(obj, item_wear);
}

bool IsTimerUnlimited(const CObjectPrototype *obj) {
	// куда надевается наш предмет.
	int item_wear = -1;
	bool type_item = false;
	if (obj->get_type() == EObjType::kArmor
		|| obj->get_type() == EObjType::kStaff
		|| obj->get_type() == EObjType::kWorm
		|| obj->get_type() == EObjType::kWeapon) {
		type_item = true;
	}
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
	if (obj->has_flag(EObjFlag::kSetItem)) {
		return false;
	}
	// !нерушима
	if (obj->has_flag(EObjFlag::kLimitedTimer)) {
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
	// сумма весов параметров и аффектов предмета по критериям слота
	const double sum = SumParamWeights(obj, item_wear);
	const double sum_aff = SumAffectWeights(obj, item_wear);

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

// Лояльный разбор числа, как было у pugixml as_double(): пустое/некорректное значение -> 0.
double ParseCriterionValue(const char *value) {
	if (!value || !*value) {
		return 0.0;
	}
	try {
		return parse::ReadAsDouble(value);
	} catch (const std::exception &) {
		return 0.0;
	}
}

// Загружает критерии одной <position> в слот index: <apply id=EApply>, <affect id=EEquipmentAffect>.
void LoadPosition(parser_wrapper::DataNode position, int index) {
	if (position.GoToChild("params")) {
		for (auto &apply : position.Children("apply")) {
			try {
				const EApply id = parse::ReadAsConstant<EApply>(apply.GetValue("id"));
				undecayable_criterions[index].params[id] = ParseCriterionValue(apply.GetValue("value"));
			} catch (const std::exception &) {
				err_log("stable_objs: unknown <apply id> '%s'", apply.GetValue("id"));
			}
		}
		position.GoToParent();
	}
	if (position.GoToChild("affects")) {
		for (auto &affect : position.Children("affect")) {
			try {
				const EEquipmentAffect id = parse::ReadAsConstant<EEquipmentAffect>(affect.GetValue("id"));
				undecayable_criterions[index].affects[id] = ParseCriterionValue(affect.GetValue("value"));
			} catch (const std::exception &) {
				err_log("stable_objs: unknown <affect id> '%s'", affect.GetValue("id"));
			}
		}
		position.GoToParent();
	}
}

void StableObjsLoader::Load(parser_wrapper::DataNode data) {
	// перезагрузка должна быть идемпотентной -- чистим накопленные критерии
	for (auto &slot : undecayable_criterions) {
		slot.params.clear();
		slot.affects.clear();
	}

	// <equipment_positions> -> набор <position val="EEquipPos|...">. Каждая позиция из списка val
	// (разбор -- общий ParseWearPositions) ложится в свой слот критериев (EWearFlag-индекс).
	for (auto &position : data.Children("position")) {
		for (const EWearFlag wear : ParseWearPositions(position.GetValue("val"))) {
			LoadPosition(position, DeterminePowerOfTwoForEnum(wear));
		}
	}
}

void StableObjsLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

} // namespace stable_obj

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
