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
	if (utils::IsAbbr(str, "свет")
		|| utils::IsAbbr(str, "light")) {
		type = EObjType::kLightSource;
	} else if (utils::IsAbbr(str, "свиток")
		|| utils::IsAbbr(str, "scroll")) {
		type = EObjType::kScroll;
	} else if (utils::IsAbbr(str, "палочка")
		|| utils::IsAbbr(str, "wand")) {
		type = EObjType::kWand;
	} else if (utils::IsAbbr(str, "посох")
		|| utils::IsAbbr(str, "staff")) {
		type = EObjType::kStaff;
	} else if (utils::IsAbbr(str, "оружие")
		|| utils::IsAbbr(str, "weapon")) {
		type = EObjType::kWeapon;
	} else if (utils::IsAbbr(str, "броня")
		|| utils::IsAbbr(str, "armor")) {
		type = EObjType::kArmor;
	} else if (utils::IsAbbr(str, "напиток")
		|| utils::IsAbbr(str, "potion")) {
		type = EObjType::kPotion;
	} else if (utils::IsAbbr(str, "прочее")
		|| utils::IsAbbr(str, "другое")
		|| utils::IsAbbr(str, "other")) {
		type = EObjType::kOther;
	} else if (utils::IsAbbr(str, "контейнер")
		|| utils::IsAbbr(str, "container")) {
		type = EObjType::kContainer;
	} else if (utils::IsAbbr(str, "материал")
		|| utils::IsAbbr(str, "material")) {
		type = EObjType::kCraftMaterial;
	} else if (utils::IsAbbr(str, "зачарованный")
		|| utils::IsAbbr(str, "enchant")) {
		type = EObjType::kEnchant;
	} else if (utils::IsAbbr(str, "емкость")
		|| utils::IsAbbr(str, "tank")) {
		type = EObjType::kLiquidContainer;
	} else if (utils::IsAbbr(str, "книга")
		|| utils::IsAbbr(str, "book")) {
		type = EObjType::kBook;
	} else if (utils::IsAbbr(str, "руна")
		|| utils::IsAbbr(str, "rune")) {
		type = EObjType::kIngredient;
	} else if (utils::IsAbbr(str, "ингредиент")
		|| utils::IsAbbr(str, "ingradient")) {
		type = EObjType::kMagicIngredient;
	} else if (utils::IsAbbr(str, "легкие")
		|| utils::IsAbbr(str, "легкая")) {
		type = EObjType::kLightArmor;
	} else if (utils::IsAbbr(str, "средние")
		|| utils::IsAbbr(str, "средняя")) {
		type = EObjType::kMediumArmor;
	} else if (utils::IsAbbr(str, "тяжелые")
		|| utils::IsAbbr(str, "тяжелая")) {
		type = EObjType::kHeavyArmor;
	} else {
		return false;
	}

	return true;
}

bool ParseFilter::init_state(const char *str) {
	if (utils::IsAbbr(str, "ужасно"))
		state = 0;
	else if (utils::IsAbbr(str, "скоро сломается"))
		state = 20;
	else if (utils::IsAbbr(str, "плоховато"))
		state = 40;
	else if (utils::IsAbbr(str, "средне"))
		state = 60;
	else if (utils::IsAbbr(str, "идеально"))
		state = 80;
	else if (utils::IsAbbr(str, "нерушимо"))
		state = 1000;
	else return false;

	return true;
}

bool ParseFilter::init_wear(const char *str) {
	if (utils::IsAbbr(str, "палец")) {
		wear = EWearFlag::kFinger;
		wear_message = 1;
	} else if (utils::IsAbbr(str, "шея") || utils::IsAbbr(str, "грудь")) {
		wear = EWearFlag::kNeck;
		wear_message = 2;
	} else if (utils::IsAbbr(str, "тело")) {
		wear = EWearFlag::kBody;
		wear_message = 3;
	} else if (utils::IsAbbr(str, "голова")) {
		wear = EWearFlag::kHead;
		wear_message = 4;
	} else if (utils::IsAbbr(str, "ноги")) {
		wear = EWearFlag::kLegs;
		wear_message = 5;
	} else if (utils::IsAbbr(str, "ступни")) {
		wear = EWearFlag::kFeet;
		wear_message = 6;
	} else if (utils::IsAbbr(str, "кисти")) {
		wear = EWearFlag::kHands;
		wear_message = 7;
	} else if (utils::IsAbbr(str, "руки")) {
		wear = EWearFlag::kArms;
		wear_message = 8;
	} else if (utils::IsAbbr(str, "щит")) {
		wear = EWearFlag::kShield;
		wear_message = 9;
	} else if (utils::IsAbbr(str, "плечи")) {
		wear = EWearFlag::kShoulders;
		wear_message = 10;
	} else if (utils::IsAbbr(str, "пояс")) {
		wear = EWearFlag::kWaist;
		wear_message = 11;
	} else if (utils::IsAbbr(str, "запястья")) {
		wear = EWearFlag::kWrist;
		wear_message = 12;
	} else if (utils::IsAbbr(str, "правая")) {
		wear = EWearFlag::kWield;
		wear_message = 13;
	} else if (utils::IsAbbr(str, "левая")) {
		wear = EWearFlag::kHold;
		wear_message = 14;
	} else if (utils::IsAbbr(str, "обе")) {
		wear = EWearFlag::kBoth;
		wear_message = 15;
	} else if (utils::IsAbbr(str, "колчан")) {
		wear = EWearFlag::kQuiver;
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

bool ParseFilter::init_skill(const char *str) {
	for (skill_id = ESkill::kFirst; skill_id <= ESkill::kLast; ++skill_id) {
		if (utils::IsAbbr(str, MUD::Skill(skill_id).GetName())) {
			return true;
		}
	}
	return false;
}

bool ParseFilter::init_weap_class(const char *str) {
	if (utils::IsAbbr(str, "луки")) {
		weap_class = ESkill::kBows;
		weap_message = 0;
	} else if (utils::IsAbbr(str, "короткие")) {
		weap_class = ESkill::kShortBlades;
		weap_message = 1;
	} else if (utils::IsAbbr(str, "длинные")) {
		weap_class = ESkill::kLongBlades;
		weap_message = 2;
	} else if (utils::IsAbbr(str, "секиры")) {
		weap_class = ESkill::kAxes;
		weap_message = 3;
	} else if (utils::IsAbbr(str, "палицы")) {
		weap_class = ESkill::kClubs;
		weap_message = 4;
	} else if (utils::IsAbbr(str, "иное")) {
		weap_class = ESkill::kNonstandart;
		weap_message = 5;
	} else if (utils::IsAbbr(str, "двуручники")) {
		weap_class = ESkill::kTwohands;
		weap_message = 6;
	} else if (utils::IsAbbr(str, "проникающее")) {
		weap_class = ESkill::kPicks;
		weap_message = 7;
	} else if (utils::IsAbbr(str, "копья")) {
		weap_class = ESkill::kSpades;
		weap_message = 8;
	} else {
		return false;
	}

	type = EObjType::kWeapon;

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
	utils::RemoveColors(name_obj);
	if (name.empty()
		|| isname(name, name_obj)) {
		result = true;
	} else if ((GET_OBJ_TYPE(obj) == EObjType::kMagicIngredient
		|| GET_OBJ_TYPE(obj) == EObjType::kIngredient)
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

bool ParseFilter::check_skill(ObjData *obj) const {
	if (skill_id == ESkill::kUndefined)
		return true;
	if (obj->has_skills()) {
		auto it = obj->get_skills().find(skill_id);
		if (it != obj->get_skills().end()) {
			return true;
		}
	}
	return false;
}

bool ParseFilter::check_wear(ObjData *obj) const {
	if (wear == EWearFlag::kUndefined
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
					if (IsNegativeApply(obj->get_affected(i).location)) {
						mod = -mod;
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
		&& check_skill(obj)
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
		&& check_weap_class(obj)
		&& check_cost(GET_EXCHANGE_ITEM_COST(exch_obj))
		&& check_affect_apply(obj)
		&& check_affect_weap(obj)
		&& check_affect_extra(obj)
		&& check_realtime(exch_obj)
		&& check_skill(obj)
		&& check_remorts(obj)) {
		return true;
	}
	return false;
}

bool ParseFilter::parse_filter(CharData *ch, ParseFilter &filter, char *argument) {
	char buf_tmp[kMaxInputLength];

	if (!*argument) {
		std::stringstream ss;
		ss << "Фильтры аналогичны командам базара + аффекты предмета:\r\n" <<
			  "   И - Имя (название) предмета\r\n" <<
			  "   Т - Тип предмета (свет,свиток,палочка,посох,оружие,броня,напиток,прочее,\r\n" <<
			  "       контейнер,книга,руна,ингредиент)\r\n" <<
			  "   C - Состояние предмета (ужасно,скоро исп,плоховато,средне,идеально)\r\n" <<
			  "   О - Куда можно одеть предмет (палец,шея,тело,голова,ноги,ступни,кисти,руки,\r\n" <<
			  "       щит,плечи,пояс,запястья,левая,правая,обе)\r\n" <<
			  "   К - Класс оружия (луки,короткие,длинные,секиры,палицы,иное,двуручники,\r\n" <<
			  "       проникающее,копья)\r\n" <<
			  "   А - название аффекта (длинное.имя.аффекта), до трех Аххх за один запрос,\r\n" <<
			  "       для слотов под камни доступны короткие алиасы А1, А2, А3 - 1..3 слота.\r\n" <<
			  "       допускается несколько слов через . и строгий поиск (! на конце слова)\r\n" <<
			  "   Ц - цена предмета, знак '+' в конце указанной цены выведeт предметы,\r\n" <<
			  "       которые равны или дороже указанной цены. Знак '-' выведет предметы,\r\n" <<
			  "       которые равны или дешевле указанной цены.\r\n" <<
			  "   Р - стоимость ренты предмета, знак '+' в конце указанной стоимости выведeт\r\n" <<
			  "       предметы, содержание которых равно или дороже указанной цифры. Знак\r\n" <<
			  "       '-' выведет  предметы, содержание которых равно или дешевле указанной\r\n" <<
			  "       цифры.                                                                 \r\n" <<
			  "   М - количество перевоплощений, знак '+' в конце указанного количества\r\n" <<
			  "       выведет предметы, которые требует больше или равное количество        \r\n" <<
			  "       перевоплощений. Знак '-' выведет предметы, которое требует меньше или\r\n" <<
			  "       равное количество перевоплощений.                                  \r\n" <<
			  "   У - Добавляемое умение\r\n" <<
			  " Можно указать несколько фильтров, разделив их пробелом\r\n";
		SendMsgToChar(ss.str(), ch);
		return false;
	}
	while (*argument) {
		switch (*argument) {
			case 'И': argument = one_argument(++argument, buf_tmp);
				if (strlen(buf_tmp) == 0) {
					SendMsgToChar("Укажите имя предмета.\r\n", ch);
					return false;
				}
				filter.name = buf_tmp;
				break;
			case 'Т': argument = one_argument(++argument, buf_tmp);
				if (!filter.init_type(buf_tmp)) {
					SendMsgToChar("Неверный тип предмета.\r\n", ch);
					return false;
				}
				break;
			case 'С': argument = one_argument(++argument, buf_tmp);
				if (!filter.init_state(buf_tmp)) {
					SendMsgToChar("Неверное состояние предмета.\r\n", ch);
					return false;
				}
				break;
			case 'О': argument = one_argument(++argument, buf_tmp);
				if (!filter.init_wear(buf_tmp)) {
					SendMsgToChar("Неверное место одевания предмета.\r\n", ch);
					return false;
				}
				break;
			case 'Ц': argument = one_argument(++argument, buf_tmp);
				if (!filter.init_cost(buf_tmp)) {
					SendMsgToChar("Неверный формат в фильтре: Ц<цена><+->.\r\n", ch);
					return false;
				}
				break;
			case 'К': argument = one_argument(++argument, buf_tmp);
				if (!filter.init_weap_class(buf_tmp)) {
					SendMsgToChar("Неверный класс оружия.\r\n", ch);
					return false;
				}
				break;
			case 'А': {
				argument = one_argument(++argument, buf_tmp);
				size_t len = strlen(buf_tmp);
				if (len == 0) {
					SendMsgToChar("Укажите аффект предмета.\r\n", ch);
					return false;
				}
				if (filter.affects_cnt() >= 3) {
					break;
				}
				if (!filter.init_affect(buf_tmp, len)) {
					SendMsgToChar(ch, "Неверный аффект предмета: '%s'.\r\n", buf_tmp);
					return false;
				}
				break;
			} // case 'А'
			case 'Р':// стоимость ренты
				argument = one_argument(++argument, buf_tmp);
				if (!filter.init_rent(buf_tmp)) {
					SendMsgToChar("Неверный формат в фильтре: Р<стоимость><+->.\r\n", ch);
					return false;
				}
				break;
			case 'М':// количество мортов
				argument = one_argument(++argument, buf_tmp);
				if (!filter.init_remorts(buf_tmp)) {
					SendMsgToChar("Неверный формат в фильтре: М<количество мортов><+->.\r\n", ch);
					return false;
				}
				break;
			case 'У':// умения
				argument = one_argument(++argument, buf_tmp);
				if (!filter.init_skill(buf_tmp)) {
					SendMsgToChar("Неверное умение.\r\n", ch);
					return false;
				}
				break;
			default: 
				++argument;
				break;
		}
	}
	return true;
}

std::string ParseFilter::print() const {
	std::string buffer;

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
	if (wear != EWearFlag::kUndefined) {
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
	if (skill_id != ESkill::kUndefined) {
		sprintf(buf, "%s ", MUD::Skill(skill_id).GetName());
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
	return buffer;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
