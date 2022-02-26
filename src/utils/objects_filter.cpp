/**
\authors Created by Sventovit
\date 28.01.2022.
\brief Фильтр поиска предметов на базаре, в хранилищах и так далее.
\details Фильтр поиска предметов на базаре, в хранилищах и так далее.
*/

#include "objects_filter.h"

#include "game_economics/exchange.h"
#include "house.h"
#include "obj_prototypes.h"
#include "structs/global_objects.h"

bool ParseFilter::init_type(const char *str) {
	if (utils::IsAbbrev(str, "свет")
		|| utils::IsAbbrev(str, "light")) {
		type = ObjData::ITEM_LIGHT;
	} else if (utils::IsAbbrev(str, "свиток")
		|| utils::IsAbbrev(str, "scroll")) {
		type = ObjData::ITEM_SCROLL;
	} else if (utils::IsAbbrev(str, "палочка")
		|| utils::IsAbbrev(str, "wand")) {
		type = ObjData::ITEM_WAND;
	} else if (utils::IsAbbrev(str, "посох")
		|| utils::IsAbbrev(str, "staff")) {
		type = ObjData::ITEM_STAFF;
	} else if (utils::IsAbbrev(str, "оружие")
		|| utils::IsAbbrev(str, "weapon")) {
		type = ObjData::ITEM_WEAPON;
	} else if (utils::IsAbbrev(str, "броня")
		|| utils::IsAbbrev(str, "armor")) {
		type = ObjData::ITEM_ARMOR;
	} else if (utils::IsAbbrev(str, "напиток")
		|| utils::IsAbbrev(str, "potion")) {
		type = ObjData::ITEM_POTION;
	} else if (utils::IsAbbrev(str, "прочее")
		|| utils::IsAbbrev(str, "другое")
		|| utils::IsAbbrev(str, "other")) {
		type = ObjData::ITEM_OTHER;
	} else if (utils::IsAbbrev(str, "контейнер")
		|| utils::IsAbbrev(str, "container")) {
		type = ObjData::ITEM_CONTAINER;
	} else if (utils::IsAbbrev(str, "материал")
		|| utils::IsAbbrev(str, "material")) {
		type = ObjData::ITEM_MATERIAL;
	} else if (utils::IsAbbrev(str, "зачарованный")
		|| utils::IsAbbrev(str, "enchant")) {
		type = ObjData::ITEM_ENCHANT;
	} else if (utils::IsAbbrev(str, "емкость")
		|| utils::IsAbbrev(str, "tank")) {
		type = ObjData::ITEM_DRINKCON;
	} else if (utils::IsAbbrev(str, "книга")
		|| utils::IsAbbrev(str, "book")) {
		type = ObjData::ITEM_BOOK;
	} else if (utils::IsAbbrev(str, "руна")
		|| utils::IsAbbrev(str, "rune")) {
		type = ObjData::ITEM_INGREDIENT;
	} else if (utils::IsAbbrev(str, "ингредиент")
		|| utils::IsAbbrev(str, "ingradient")) {
		type = ObjData::ITEM_MING;
	} else if (utils::IsAbbrev(str, "легкие")
		|| utils::IsAbbrev(str, "легкая")) {
		type = ObjData::ITEM_ARMOR_LIGHT;
	} else if (utils::IsAbbrev(str, "средние")
		|| utils::IsAbbrev(str, "средняя")) {
		type = ObjData::ITEM_ARMOR_MEDIAN;
	} else if (utils::IsAbbrev(str, "тяжелые")
		|| utils::IsAbbrev(str, "тяжелая")) {
		type = ObjData::ITEM_ARMOR_HEAVY;
	} else {
		return false;
	}

	return true;
}

bool ParseFilter::init_state(const char *str) {
	if (utils::IsAbbrev(str, "ужасно"))
		state = 0;
	else if (utils::IsAbbrev(str, "скоро сломается"))
		state = 20;
	else if (utils::IsAbbrev(str, "плоховато"))
		state = 40;
	else if (utils::IsAbbrev(str, "средне"))
		state = 60;
	else if (utils::IsAbbrev(str, "идеально"))
		state = 80;
	else if (utils::IsAbbrev(str, "нерушимо"))
		state = 1000;
	else return false;

	return true;
}

bool ParseFilter::init_wear(const char *str) {
	if (utils::IsAbbrev(str, "палец")) {
		wear = EWearFlag::ITEM_WEAR_FINGER;
		wear_message = 1;
	} else if (utils::IsAbbrev(str, "шея") || utils::IsAbbrev(str, "грудь")) {
		wear = EWearFlag::ITEM_WEAR_NECK;
		wear_message = 2;
	} else if (utils::IsAbbrev(str, "тело")) {
		wear = EWearFlag::ITEM_WEAR_BODY;
		wear_message = 3;
	} else if (utils::IsAbbrev(str, "голова")) {
		wear = EWearFlag::ITEM_WEAR_HEAD;
		wear_message = 4;
	} else if (utils::IsAbbrev(str, "ноги")) {
		wear = EWearFlag::ITEM_WEAR_LEGS;
		wear_message = 5;
	} else if (utils::IsAbbrev(str, "ступни")) {
		wear = EWearFlag::ITEM_WEAR_FEET;
		wear_message = 6;
	} else if (utils::IsAbbrev(str, "кисти")) {
		wear = EWearFlag::ITEM_WEAR_HANDS;
		wear_message = 7;
	} else if (utils::IsAbbrev(str, "руки")) {
		wear = EWearFlag::ITEM_WEAR_ARMS;
		wear_message = 8;
	} else if (utils::IsAbbrev(str, "щит")) {
		wear = EWearFlag::ITEM_WEAR_SHIELD;
		wear_message = 9;
	} else if (utils::IsAbbrev(str, "плечи")) {
		wear = EWearFlag::ITEM_WEAR_ABOUT;
		wear_message = 10;
	} else if (utils::IsAbbrev(str, "пояс")) {
		wear = EWearFlag::ITEM_WEAR_WAIST;
		wear_message = 11;
	} else if (utils::IsAbbrev(str, "запястья")) {
		wear = EWearFlag::ITEM_WEAR_WRIST;
		wear_message = 12;
	} else if (utils::IsAbbrev(str, "правая")) {
		wear = EWearFlag::ITEM_WEAR_WIELD;
		wear_message = 13;
	} else if (utils::IsAbbrev(str, "левая")) {
		wear = EWearFlag::ITEM_WEAR_HOLD;
		wear_message = 14;
	} else if (utils::IsAbbrev(str, "обе")) {
		wear = EWearFlag::ITEM_WEAR_BOTHS;
		wear_message = 15;
	} else if (utils::IsAbbrev(str, "колчан")) {
		wear = EWearFlag::ITEM_WEAR_QUIVER;
		wear_message = 16;
	} else {
		return false;
	}

	return true;
}

bool ParseFilter::init_cost(const char *str) {
	if (sscanf(str, "%d%[-+]", &cost, &cost_sign) != 2) {
		return false;
	}
	if (cost_sign == '-') {
		cost = -cost;
	}

	return true;
}

bool ParseFilter::init_rent(const char *str) {
	if (sscanf(str, "%d%[-+]", &rent, &rent_sign) != 2) {
		return false;
	}
	if (rent_sign == '-') {
		rent = -rent;
	}

	return true;
}

bool ParseFilter::init_remorts(const char *str) {
	filter_remorts_count++;
	if (filter_remorts_count>1) {
		filter_remorts_count = 1;
	}
	if (sscanf(str, "%d%[-+]", &remorts[filter_remorts_count], &remorts_sign[filter_remorts_count]) != 2) {
		return false;
	}
	if (remorts_sign[filter_remorts_count] == '-') {
		remorts[filter_remorts_count] = -remorts[filter_remorts_count];
	}

	return true;
}


bool ParseFilter::init_weap_class(const char *str) {
	if (utils::IsAbbrev(str, "луки")) {
		weap_class = ESkill::kBows;
		weap_message = 0;
	} else if (utils::IsAbbrev(str, "короткие")) {
		weap_class = ESkill::kShortBlades;
		weap_message = 1;
	} else if (utils::IsAbbrev(str, "длинные")) {
		weap_class = ESkill::kLongBlades;
		weap_message = 2;
	} else if (utils::IsAbbrev(str, "секиры")) {
		weap_class = ESkill::kAxes;
		weap_message = 3;
	} else if (utils::IsAbbrev(str, "палицы")) {
		weap_class = ESkill::kClubs;
		weap_message = 4;
	} else if (utils::IsAbbrev(str, "иное")) {
		weap_class = ESkill::kNonstandart;
		weap_message = 5;
	} else if (utils::IsAbbrev(str, "двуручники")) {
		weap_class = ESkill::kTwohands;
		weap_message = 6;
	} else if (utils::IsAbbrev(str, "проникающее")) {
		weap_class = ESkill::kPicks;
		weap_message = 7;
	} else if (utils::IsAbbrev(str, "копья")) {
		weap_class = ESkill::kSpades;
		weap_message = 8;
	} else {
		return false;
	}

	type = ObjData::ITEM_WEAPON;

	return true;
}

bool ParseFilter::init_realtime(const char *str) {
	tm trynewtimeup;
	tm trynewtimedown;
	int day, month, year;
	std::string tmp_string;

	if (strlen(str) != 11) {
		return false;
	}

	if ((str[2] != '.')
		|| (str[5] != '.')
		|| (str[10] != '<'
			&& str[10] != '>'
			&& str[10] != '=')) {
		return false;
	}

	if (!isdigit(static_cast<unsigned int>(str[0]))
		|| !isdigit(static_cast<unsigned int>(str[1]))
		|| !isdigit(static_cast<unsigned int>(str[3]))
		|| !isdigit(static_cast<unsigned int>(str[4]))
		|| !isdigit(static_cast<unsigned int>(str[6]))
		|| !isdigit(static_cast<unsigned int>(str[7]))
		|| !isdigit(static_cast<unsigned int>(str[8]))
		|| !isdigit(static_cast<unsigned int>(str[9]))) {
		return false;
	}

	tmp_string = "";
	tmp_string.push_back(str[0]);
	tmp_string.push_back(str[1]);
	day = std::stoi(tmp_string);
	tmp_string = "";
	tmp_string.push_back(str[3]);
	tmp_string.push_back(str[4]);
	month = std::stoi(tmp_string);
	tmp_string = "";
	tmp_string.push_back(str[6]);
	tmp_string.push_back(str[7]);
	tmp_string.push_back(str[8]);
	tmp_string.push_back(str[9]);
	year = std::stoi(tmp_string);

	if (year <= 1900) {
		return false;
	}
	if (month > 12) {
		return false;
	}
	if (year % 4 == 0 && month == 2 && day > 29) {
		return false;
	} else if (month == 1 && day > 31) {
		return false;
	} else if (year % 4 != 0 && month == 2 && day > 28) {
		return false;
	} else if (month == 3 && day > 31) {
		return false;
	} else if (month == 4 && day > 30) {
		return false;
	} else if (month == 5 && day > 31) {
		return false;
	} else if (month == 6 && day > 30) {
		return false;
	} else if (month == 7 && day > 31) {
		return false;
	} else if (month == 8 && day > 31) {
		return false;
	} else if (month == 9 && day > 30) {
		return false;
	} else if (month == 10 && day > 31) {
		return false;
	} else if (month == 11 && day > 30) {
		return false;
	} else if (month == 12 && day > 31) {
		return false;
	}

	trynewtimedown.tm_sec = 0;
	trynewtimedown.tm_hour = 0;
	trynewtimedown.tm_min = 0;
	trynewtimedown.tm_mday = day;
	trynewtimedown.tm_mon = month - 1;
	trynewtimedown.tm_year = year - 1900;
	new_timedown = mktime(&trynewtimedown);

	trynewtimeup.tm_sec = 59;
	trynewtimeup.tm_hour = 23;
	trynewtimeup.tm_min = 59;
	trynewtimeup.tm_mday = day;
	trynewtimeup.tm_mon = month - 1;
	trynewtimeup.tm_year = year - 1900;
	new_timeup = mktime(&trynewtimeup);

	new_timesign = str[10];

	return true;
}

size_t ParseFilter::affects_cnt() const {
	return affect_weap.size() + affect_apply.size() + affect_extra.size();
}

bool ParseFilter::init_affect(char *str, size_t str_len) {
	// Аимя!
	bool strong = false;
	if (str_len > 1 && str[str_len - 1] == '!') {
		strong = true;
		str[str_len - 1] = '\0';
	}
	// А1, А2, А3
	if (str_len == 1) {
		switch (*str) {
			case '1': sprintf(str, "можно вплавить 1 камень");
				break;
			case '2': sprintf(str, "можно вплавить 2 камня");
				break;
			case '3': sprintf(str, "можно вплавить 3 камня");
				break;
		}
	}

	utils::ConvertToLow(str);
	str_len = strlen(str);

	for (int num = 0; *apply_types[num] != '\n'; ++num) {
		if (strong && !strcmp(str, apply_types[num])) {
			affect_apply.push_back(num);
			return true;
		} else if (!strong && isname(str, apply_types[num])) {
			affect_apply.push_back(num);
			return true;
		}
	}

	int num = 0;
	for (int flag = 0; flag < 4; ++flag) {
		for (/* тут ничего не надо */; *weapon_affects[num] != '\n'; ++num) {
			if (strong && !strcmp(str, weapon_affects[num])) {
				affect_weap.push_back(num);
				return true;
			} else if (!strong && isname(str, weapon_affects[num])) {
				affect_weap.push_back(num);
				return true;
			}
		}
		++num;
	}

	num = 0;
	for (int flag = 0; flag < 4; ++flag) {
		for (/* тут ничего не надо */; *extra_bits[num] != '\n'; ++num) {
			if (strong && !strcmp(str, extra_bits[num])) {
				affect_extra.push_back(num);
				return true;
			} else if (!strong && isname(str, extra_bits[num])) {
				affect_extra.push_back(num);
				return true;
			}
		}
		num++;
	}

	return false;
}

/// имя, метка для клан-хранов
bool ParseFilter::check_name(ObjData *obj, CharData *ch) const {
	bool result = false;
	char name_obj[kMaxStringLength];
	strcpy(name_obj, GET_OBJ_PNAME(obj, 0).c_str());
	utils::remove_colors(name_obj);
	if (name.empty()
		|| isname(name, name_obj)) {
		result = true;
	} else if ((GET_OBJ_TYPE(obj) == ObjData::ITEM_MING
		|| GET_OBJ_TYPE(obj) == ObjData::ITEM_INGREDIENT)
		&& GET_OBJ_RNUM(obj) >= 0
		&& isname(name, obj_proto[GET_OBJ_RNUM(obj)]->get_aliases().c_str())) {
		result = true;
	} else if (ch
		&& filter_type == CLAN
		&& CHECK_CUSTOM_LABEL(name, obj, ch)) {
		result = true;
	}

	return result;
}

bool ParseFilter::check_type(ObjData *obj) const {
	if (type < 0
		|| type == GET_OBJ_TYPE(obj)) {
		return true;
	}

	return false;
}

bool ParseFilter::check_state(ObjData *obj) const {
	bool result = false;
	if (state < 0) {
		result = true;
	} else if (GET_OBJ_RNUM(obj) >= 0) {
		int proto_tm = obj_proto.at(GET_OBJ_RNUM(obj))->get_timer();
		if (proto_tm <= 0) {
			char buf_[kMaxInputLength];
			snprintf(buf_, sizeof(buf_), "SYSERROR: wrong obj-proto timer %d, vnum=%d (%s %s:%d)",
					 proto_tm, obj_proto.at(GET_OBJ_RNUM(obj))->get_rnum(), __func__, __FILE__, __LINE__);
			mudlog(buf_, CMP, kLvlImmortal, SYSLOG, true);
		} else {
			int tm_pct;
			if (check_unlimited_timer(obj))  // если шмотка нерушима, физически проставляем текст нерушимо
			{
				tm_pct = 1000;
			} else {
				tm_pct = obj->get_timer() * 100 / proto_tm;
			}

			if (filter_type == CLAN
				&& tm_pct >= state
				&& tm_pct < state + 20) {
				result = true;
			} else if (filter_type == EXCHANGE && tm_pct >= state) {
				result = true;
			}
		}
	}
	return result;
}

bool ParseFilter::check_wear(ObjData *obj) const {
	if (wear == EWearFlag::ITEM_WEAR_UNDEFINED
		|| CAN_WEAR(obj, wear)) {
		return true;
	}
	return false;
}

bool ParseFilter::check_weap_class(ObjData *obj) const {
	if (MUD::Skills().IsInvalid(weap_class) || weap_class == static_cast<ESkill>(GET_OBJ_SKILL(obj))) {
		return true;
	}
	return false;
}

bool ParseFilter::check_cost(int obj_price) const {
	bool result = false;

	if (cost_sign == '\0') {
		result = true;
	} else if (cost >= 0 && obj_price >= cost) {
		result = true;
	} else if (cost < 0 && obj_price <= -cost) {
		result = true;
	}
	return result;
}

bool ParseFilter::check_rent(int obj_price) const {
	bool result = false;

	if (rent_sign == '\0') {
		result = true;
	} else if (rent >= 0 && obj_price >= rent) {
		result = true;
	} else if (rent < 0 && obj_price <= -rent) {
		result = true;
	}
	return result;
}

bool ParseFilter::check_remorts(ObjData *obj) const {
	int result;
	int obj_remorts = obj->get_auto_mort_req();

	for (int i=0;i<=filter_remorts_count;i++) {
		result = 0;
		if (remorts_sign[i] == '\0') {
			result = 1;
		} else if (remorts[i] >= 0 && obj_remorts >= remorts[i]) {
			result = 1;
		} else if (remorts[i] <= 0 && obj_remorts <= -remorts[i]) {
			result = 1;
		}
		if (result==0) {
			return false;
		}
	}
	return true;
}

bool ParseFilter::check_affect_weap(ObjData *obj) const {
	if (!affect_weap.empty()) {
		for (auto it = affect_weap.begin(); it != affect_weap.end(); ++it) {
			if (!CompareBits(obj->get_affect_flags(), weapon_affects, *it)) {
				return false;
			}
		}
	}
	return true;
}

std::string ParseFilter::show_obj_aff(ObjData *obj) {
	if (!affect_apply.empty()) {
		for (auto it = affect_apply.begin(); it != affect_apply.end(); ++it) {
			for (int i = 0; i < kMaxObjAffect; ++i) {
				if (obj->get_affected(i).location == *it) {
					int mod = obj->get_affected(i).modifier;
					char buf_[kMaxInputLength];
					sprinttype(obj->get_affected(i).location, apply_types, buf_);
					for (int j = 0; *apply_negative[j] != '\n'; j++) {
						if (!str_cmp(buf_, apply_negative[j])) {
							mod = -mod;
						}
					}
					std::string return_str(buf_);
					if (mod > 0)
						return_str = return_str + " +" + std::to_string(mod);
					else
						return_str = return_str + " " + std::to_string(mod);
					return "(" + return_str + ")";
				}

			}
		}
	}
	return " ";
}

bool ParseFilter::check_affect_apply(ObjData *obj) const {
	bool result = true;
	if (!affect_apply.empty()) {
		for (auto it = affect_apply.begin(); it != affect_apply.end() && result; ++it) {
			result = false;
			for (int i = 0; i < kMaxObjAffect; ++i) {
				if (obj->get_affected(i).location == *it) {
					int mod = obj->get_affected(i).modifier;
					char buf_[kMaxInputLength];
					sprinttype(obj->get_affected(i).location, apply_types, buf_);
					for (int j = 0; *apply_negative[j] != '\n'; j++) {
						if (!str_cmp(buf_, apply_negative[j])) {
							mod = -mod;
							break;
						}
					}
					result = true;
					break;
				}
			}
		}
	}
	return result;
}

bool ParseFilter::check_affect_extra(ObjData *obj) const {
	if (!affect_extra.empty()) {
		for (auto it = affect_extra.begin(); it != affect_extra.end(); ++it) {
			if (!CompareBits(GET_OBJ_EXTRA(obj), extra_bits, *it)) {
				return false;
			}
		}
	}
	return true;
}

bool ParseFilter::check_owner(ExchangeItem *exch_obj) const {
	if (owner.empty()
		|| isname(owner, get_name_by_id(GET_EXCHANGE_ITEM_SELLERID(exch_obj)))) {
		return true;
	}
	return false;
}

bool ParseFilter::check_realtime(ExchangeItem *exch_obj) const {
	bool result = false;

	if (new_timesign == '\0')
		result = true;
	else if (new_timesign == '=' && (new_timedown <= exch_obj->time)
		&& (new_timeup >= exch_obj->time))
		result = true;
	else if (new_timesign == '<' && (new_timeup >= exch_obj->time))
		result = true;
	else if (new_timesign == '>' && (new_timedown <= exch_obj->time))
		result = true;

	return result;
}

bool ParseFilter::check(ObjData *obj, CharData *ch) {
	if (check_name(obj, ch)
		&& check_type(obj)
		&& check_state(obj)
		&& check_wear(obj)
		&& check_weap_class(obj)
		&& check_cost(GET_OBJ_COST(obj))
		&& check_rent(GET_OBJ_RENTEQ(obj) * kClanStorehouseCoeff / 100)
		&& check_affect_apply(obj)
		&& check_affect_weap(obj)
		&& check_affect_extra(obj)
		&& check_remorts(obj)) {
		return true;
	}
	return false;
}

bool ParseFilter::check(ExchangeItem *exch_obj) {
	ObjData *obj = GET_EXCHANGE_ITEM(exch_obj);
	if (check_name(obj)
		&& check_owner(exch_obj)
			//&& (owner_id == -1 || owner_id == GET_EXCHANGE_ITEM_SELLERID(exch_obj))
		&& check_type(obj)
		&& check_state(obj)
		&& check_wear(obj)
		&& check_remorts(obj)
		&& check_weap_class(obj)
		&& check_cost(GET_EXCHANGE_ITEM_COST(exch_obj))
		&& check_affect_apply(obj)
		&& check_affect_weap(obj)
		&& check_affect_extra(obj)
		&& check_realtime(exch_obj)) {
		return true;
	}
	return false;
}

std::string ParseFilter::print() const {
	std::string buffer = "Выборка: ";
	if (!name.empty()) {
		buffer += name + " ";
	}
	/*
	if (owner_id >= 0)
	{
		const char *name = get_name_by_id(owner_id);
		if (name && name[0] != '\0')
		{
			buffer += name;
			buffer += " ";
		}
	}
	*/
	if (!owner.empty()) {
		buffer += owner + " ";
	}
	if (type >= 0) {
		buffer += item_types[type];
		buffer += " ";
	}
	if (state >= 0) {
		buffer += print_obj_state(state);
		buffer += " ";
	}
	if (wear != EWearFlag::ITEM_WEAR_UNDEFINED) {
		buffer += wear_bits[wear_message];
		buffer += " ";
	}
	if (MUD::Skills().IsValid(weap_class)) {
		buffer += weapon_class[weap_message];
		buffer += " ";
	}
	if (cost >= 0) {
		sprintf(buf, "%d%c ", cost, cost_sign);
		buffer += buf;
	}
	if (rent >= 0) {
		sprintf(buf, "%d%c ", rent, rent_sign);
		buffer += buf;
	}
	/*if (remorts >= 0) {
		sprintf(buf, "%d%c ", remorts, remorts_sign);
		buffer += buf;
	}*/
	if (!affect_weap.empty()) {
		for (int it : affect_weap) {
			buffer += weapon_affects[it];
			buffer += " ";
		}
	}
	if (!affect_apply.empty()) {
		for (int it : affect_apply) {
			buffer += apply_types[it];
			buffer += " ";
		}
	}
	if (!affect_extra.empty()) {
		for (int it : affect_extra) {
			buffer += extra_bits[it];
			buffer += " ";
		}
	}
	buffer += "\r\n";

	return buffer;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
