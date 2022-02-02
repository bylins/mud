/**
\authors Created by Sventovit
\date 28.01.2022.
\brief Фильтр поиска предметов на базаре, в хранилищах и так далее.
\details Фильтр поиска предметов на базаре, в хранилищах и так далее.
*/

#ifndef BYLINS_SRC_UTILS_OBJECTS_FILTER_H_
#define BYLINS_SRC_UTILS_OBJECTS_FILTER_H_

#include "entities/entity_constants.h"
#include "skills.h"

#include <string>
#include <vector>

class CharacterData;
struct ExchangeItem;
class ObjectData;

// для парса строки с фильтрами в клан-хранах и базаре
struct ParseFilter {
	enum { CLAN, EXCHANGE };

	ParseFilter(int type) : type(-1), state(-1), wear(EWearFlag::ITEM_WEAR_UNDEFINED), wear_message(-1),
							weap_class{}, weap_message(-1), cost(-1), cost_sign('\0'), rent(-1), rent_sign('\0'),
							new_timesign('\0'), new_timedown(time(nullptr)), new_timeup(time(nullptr)),
							filter_type(type) {};

	bool init_type(const char *str);
	bool init_state(const char *str);
	bool init_wear(const char *str);
	bool init_cost(const char *str);
	bool init_rent(const char *str);
	bool init_remorts(const char *str);
	bool init_weap_class(const char *str);
	bool init_affect(char *str, size_t str_len);
	bool init_realtime(const char *str);
	size_t affects_cnt() const;
	bool check(ObjectData *obj, CharacterData *ch);
	bool check(ExchangeItem *exch_obj);
	std::string print() const;

	std::string name;      // имя предмета
	std::string owner;     // имя продавца (базар)
	int type;              // тип оружия
	int state;             // состояние
	EWearFlag wear;              // куда одевается
	int wear_message;      // для названия куда одеть
	ESkill weap_class;        // класс оружие
	int weap_message;      // для названия оружия
	int cost;              // для цены
	char cost_sign;        // знак цены +/-
	int rent;             // для стоимости ренты
	char rent_sign;        // знак ренты +/-
	int filter_remorts_count = -1;
	int remorts[2] = {-1,-1}; //для количества ремортов
	char remorts_sign[2] = "\0"; // знак ремортов
	char new_timesign;       // знак времени < > =
	time_t new_timedown;   // нижняя граница времени
	time_t new_timeup;       // верхняя граница времени
	int filter_type;       // CLAN/EXCHANGE

	std::vector<int> affect_apply; // аффекты apply_types
	std::vector<int> affect_weap;  // аффекты weapon_affects
	std::vector<int> affect_extra; // аффекты extra_bits

	std::string show_obj_aff(ObjectData *obj);

 private:
	bool check_name(ObjectData *obj, CharacterData *ch = nullptr) const;
	bool check_type(ObjectData *obj) const;
	bool check_state(ObjectData *obj) const;
	bool check_wear(ObjectData *obj) const;
	bool check_weap_class(ObjectData *obj) const;
	bool check_cost(int obj_price) const;
	bool check_rent(int obj_price) const;
	bool check_remorts(ObjectData *obj) const;
	bool check_affect_weap(ObjectData *obj) const;
	bool check_affect_apply(ObjectData *obj) const;
	bool check_affect_extra(ObjectData *obj) const;
	bool check_owner(ExchangeItem *exch_obj) const;
	bool check_realtime(ExchangeItem *exch_obj) const;
};

#endif //BYLINS_SRC_UTILS_OBJECTS_FILTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
