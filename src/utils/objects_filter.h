/**
\authors Created by Sventovit
\date 28.01.2022.
\brief Фильтр поиска предметов на базаре, в хранилищах и так далее.
\details Фильтр поиска предметов на базаре, в хранилищах и так далее. Нужно отметить, что данный код
 частично дублируется в коде магазинов, хранилищ и возможно где-то еще. Необходимо все свести в данный
 модуль и удалить это примечание.
*/

#ifndef BYLINS_SRC_UTILS_OBJECTS_FILTER_H_
#define BYLINS_SRC_UTILS_OBJECTS_FILTER_H_

#include "entities/entities_constants.h"
#include "game_skills/skills.h"

#include <string>
#include <vector>

class CharData;
struct ExchangeItem;
class ObjData;


// для парса строки с фильтрами в клан-хранах и базаре
struct ParseFilter {
	enum { CLAN, EXCHANGE };
  
	ParseFilter(int type) : remorts(-1),remorts_sign('\0'), type(-1), state(-1), wear(EWearFlag::kUndefined), wear_message(-1),
			weap_class{}, weap_message(-1), cost(-1), cost_sign('\0'), rent(-1), rent_sign('\0'),
			new_timesign('\0'), new_timedown(time(nullptr)), new_timeup(time(nullptr)),
			filter_type(type), skill_id{ESkill::kUndefined} {};

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
	bool check(ObjData *obj, CharData *ch);
	bool check(ExchangeItem *exch_obj);
	bool parse_filter(CharData *ch, ParseFilter &filter, char *argument);
	bool init_skill(const char*);

	std::string print() const;
	std::string name;      // имя предмета
	std::string owner;     // имя продавца (базар)
	std::string show_obj_aff(ObjData *obj);

 private:
	std::vector<int> affect_apply; // аффекты apply_types
	std::vector<int> affect_weap;  // аффекты weapon_affects
	std::vector<int> affect_extra; // аффекты extra_bits
  
	int remorts; //для количества ремортов
	char remorts_sign; // знак ремортов
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
	char new_timesign;       // знак времени < > =
	time_t new_timedown;   // нижняя граница времени
	time_t new_timeup;       // верхняя граница времени
	int filter_type;       // CLAN/EXCHANGE
	ESkill skill_id;
	bool check_name(ObjData *obj, CharData *ch = nullptr) const;
	bool check_type(ObjData *obj) const;
	bool check_state(ObjData *obj) const;
	bool check_wear(ObjData *obj) const;
	bool check_weap_class(ObjData *obj) const;
	bool check_cost(int obj_price) const;
	bool check_rent(int obj_price) const;
	bool check_remorts(ObjData *obj) const;
	bool check_affect_weap(ObjData *obj) const;
	bool check_affect_apply(ObjData *obj) const;
	bool check_affect_extra(ObjData *obj) const;
	bool check_owner(ExchangeItem *exch_obj) const;
	bool check_realtime(ExchangeItem *exch_obj) const;
	bool check_skill(ObjData *obj) const;
};

#endif //BYLINS_SRC_UTILS_OBJECTS_FILTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
