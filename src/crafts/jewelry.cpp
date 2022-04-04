/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "ювелир" - код.
*/

#include "jewelry.h"

#include "boot/boot_constants.h"
#include "entities/char_data.h"
#include "handler.h"
#include "mining.h"
#include "utils/random.h"
#include "structs/global_objects.h"

skillvariables_insgem insgem_vars;
insert_wanted_gem iwg;

void InitJewelryVars() {
	char line[256];
	FILE *cfg_file;

	if (!(cfg_file = fopen(LIB_MISC "skillvariables.lst", "r"))) {
		log("Cann't open skillvariables list file...");
		graceful_exit(1);
	}

	while (get_line(cfg_file, line)) {
		if (!line[0] || line[0] == ';')
			continue;

		sscanf(line, "insgem_lag %d", &insgem_vars.lag);
		sscanf(line, "insgem_minus_for_affect %d", &insgem_vars.minus_for_affect);
		sscanf(line, "insgem_prob_divide %d", &insgem_vars.prob_divide);
		sscanf(line, "insgem_dikey_percent %d", &insgem_vars.dikey_percent);
		sscanf(line, "insgem_timer_plus_percent %d", &insgem_vars.timer_plus_percent);
		sscanf(line, "insgem_timer_minus_percent %d", &insgem_vars.timer_minus_percent);

		line[0] = '\0';
	}
	fclose(cfg_file);
}

bool is_dig_stone(ObjData *obj) {
	if ((obj->get_vnum() >= dig_vars.stone1_vnum
		&& obj->get_vnum() <= dig_vars.last_stone_vnum)
		|| obj->get_vnum() == dig_vars.glass_vnum
		|| iwg.is_gem(obj->get_vnum())) {
		return true;
	}

	return false;
}

void do_insertgem(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	int percent, prob;
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	char buf[300];
	char *gem, *item;
	ObjData *gemobj, *itemobj;

	argument = two_arguments(argument, arg1, arg2);

	if (ch->is_npc() || !ch->get_skill(ESkill::kJewelry)) {
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch)) {
		if (!ROOM_FLAGGED(ch->in_room, ERoomFlag::kForge)) {
			send_to_char("Вам нужно попасть в кузницу для этого.\r\n", ch);
			return;
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		send_to_char("Вы слепы!\r\n", ch);
		return;
	}

	if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !IS_IMMORTAL(ch)) {
		send_to_char("Да тут темно хоть глаза выколи...\r\n", ch);
		return;
	}

	if (!WAITLESS(ch) && ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (!*arg1) {
		send_to_char("Вплавить что?\r\n", ch);
		return;
	} else
		gem = arg1;

	if (!(gemobj = get_obj_in_list_vis(ch, gem, ch->carrying))) {
		sprintf(buf, "У вас нет '%s'.\r\n", gem);
		send_to_char(buf, ch);
		return;
	}

	if (!is_dig_stone(gemobj)) {
		sprintf(buf, "Вы не умеете вплавлять %s.\r\n", gemobj->get_PName(3).c_str());
		send_to_char(buf, ch);
		return;
	}

	if (!*arg2) {
		send_to_char("Вплавить во что?\r\n", ch);
		return;
	} else
		item = arg2;

	if (!(itemobj = get_obj_in_list_vis(ch, item, ch->carrying))) {
		sprintf(buf, "У вас нет '%s'.\r\n", item);
		send_to_char(buf, ch);
		return;
	}
	if (GET_OBJ_MATER(itemobj) == EObjMaterial::kMaterialUndefined || (GET_OBJ_MATER(itemobj) > EObjMaterial::kPreciousMetel)) {
		if (!(GET_OBJ_MATER(itemobj) == EObjMaterial::kBone || GET_OBJ_MATER(itemobj) == EObjMaterial::kStone)) {
			sprintf(buf, "%s состоит из неподходящего материала.\r\n", itemobj->get_PName(0).c_str());
			send_to_char(buf, ch);
			return;
		}
	}
	if (!itemobj->has_flag(EObjFlag::kHasOneSlot)
		&& !itemobj->has_flag(EObjFlag::kHasTwoSlots)
		&& !itemobj->has_flag(EObjFlag::kHasThreeSlots)) {
		send_to_char("Вы не видите куда здесь можно вплавить камень.\r\n", ch);
		return;
	}

	percent = number(1, MUD::Skills()[ESkill::kJewelry].difficulty);
	prob = ch->get_skill(ESkill::kJewelry);

	WAIT_STATE(ch, kPulseViolence);

	for (int i = 0; i < kMaxObjAffect; i++) {
		if (itemobj->get_affected(i).location == EApply::kNone) {
			prob -= i * insgem_vars.minus_for_affect;
			break;
		}
	}

	for (const auto &i : weapon_affect) {
		if (IS_OBJ_AFF(itemobj, i.aff_pos)) {
			prob -= insgem_vars.minus_for_affect;
		}
	}

	argument = one_argument(argument, arg3);

	if (!*arg3) {
		ImproveSkill(ch, ESkill::kJewelry, 0, nullptr);

		if (percent > prob / insgem_vars.prob_divide) {
			sprintf(buf, "Вы неудачно попытались вплавить %s в %s, испортив камень...\r\n",
					gemobj->get_short_description().c_str(),
					itemobj->get_PName(3).c_str());
			send_to_char(buf, ch);
			sprintf(buf, "$n испортил$g %s, вплавляя его в %s!\r\n",
					gemobj->get_PName(3).c_str(),
					itemobj->get_PName(3).c_str());
			act(buf, false, ch, nullptr, nullptr, kToRoom);
			extract_obj(gemobj);
			if (number(1, 100) <= insgem_vars.dikey_percent) {
				sprintf(buf, "...и испортив хорошую вещь!\r\n");
				send_to_char(buf, ch);
				sprintf(buf, "$n испортил$g %s!\r\n", itemobj->get_PName(3).c_str());
				act(buf, false, ch, nullptr, nullptr, kToRoom);
				extract_obj(itemobj);
			}
			return;
		}
	} else {
		if (ch->get_skill(ESkill::kJewelry) < 80) {
			sprintf(buf, "Вы должны достигнуть мастерства в умении ювелир, чтобы вплавлять желаемые аффекты!\r\n");
			send_to_char(buf, ch);
			return;

		}
		if (GET_OBJ_OWNER(itemobj) != GET_UNIQUE(ch) && (ch->get_skill(ESkill::kJewelry) < 130)) {
			sprintf(buf, "Вы недостаточно искусны и можете вплавлять желаемые аффекты только в перековку!\r\n");
			send_to_char(buf, ch);
			return;
		}

		std::string str(arg3);
		if (!iwg.exist(GET_OBJ_VNUM(gemobj), str)) {
			iwg.show(ch, GET_OBJ_VNUM(gemobj));
			return;
		}

		ImproveSkill(ch, ESkill::kJewelry, 0, nullptr);

		//успех или фэйл? при 80% скила успех 30% при 100% скила 50% при 200% скила успех 75%
		if (number(1, ch->get_skill(ESkill::kJewelry)) > (ch->get_skill(ESkill::kJewelry) - 50)) {
			sprintf(buf, "Вы неудачно попытались вплавить %s в %s, испортив камень...\r\n",
					gemobj->get_short_description().c_str(),
					itemobj->get_PName(3).c_str());
			send_to_char(buf, ch);
			sprintf(buf, "$n испортил$g %s, вплавляя его в %s!\r\n",
					gemobj->get_PName(3).c_str(),
					itemobj->get_PName(3).c_str());
			act(buf, false, ch, nullptr, nullptr, kToRoom);
			extract_obj(gemobj);
			return;
		}
	}

	sprintf(buf, "Вы вплавили %s в %s!\r\n", gemobj->get_PName(3).c_str(), itemobj->get_PName(3).c_str());
	send_to_char(buf, ch);
	sprintf(buf, "$n вплавил$g %s в %s.\r\n", gemobj->get_PName(3).c_str(), itemobj->get_PName(3).c_str());
	act(buf, false, ch, nullptr, nullptr, kToRoom);

	if (GET_OBJ_OWNER(itemobj) == GET_UNIQUE(ch)) {
		int timer = itemobj->get_timer() + itemobj->get_timer() / 100 * insgem_vars.timer_plus_percent;
		itemobj->set_timer(timer);
	} else {
		int timer = itemobj->get_timer() - itemobj->get_timer() / 100 * insgem_vars.timer_minus_percent;
		itemobj->set_timer(timer);
	}

	if (GET_OBJ_MATER(gemobj) == EObjMaterial::kDiamond) {
		std::string effect;
		if (!*arg3) {
			int gem_vnum = GET_OBJ_VNUM(gemobj);
			effect = iwg.get_random_str_for(gem_vnum);
		} else {
			effect = arg3;
		}

		int tmp_type, tmp_qty;
		int tmp_bit = iwg.get_bit(GET_OBJ_VNUM(gemobj), effect);
		tmp_qty = iwg.get_qty(GET_OBJ_VNUM(gemobj), effect);
		tmp_type = iwg.get_type(GET_OBJ_VNUM(gemobj), effect);
		switch (tmp_type) {
			case 1: 
				set_obj_eff(itemobj, static_cast<EApply>(tmp_bit), tmp_qty);
				break;
			case 2: 
				set_obj_aff(itemobj, static_cast<EAffect>(tmp_bit));
				break;
			case 3: 
				itemobj->set_extra_flag(static_cast<EObjFlag>(tmp_bit));
				break;
			case 4:
				itemobj->set_skill(static_cast<ESkill>(tmp_bit), tmp_qty);
				break;
			default: 
				break;
		}
	}

	if (itemobj->has_flag(EObjFlag::kHasThreeSlots)) {
		itemobj->unset_extraflag(EObjFlag::kHasThreeSlots);
		itemobj->set_extra_flag(EObjFlag::kHasTwoSlots);
	} else if (itemobj->has_flag(EObjFlag::kHasTwoSlots)) {
		itemobj->unset_extraflag(EObjFlag::kHasTwoSlots);
		itemobj->set_extra_flag(EObjFlag::kHasOneSlot);
	} else if (itemobj->has_flag(EObjFlag::kHasOneSlot)) {
		itemobj->unset_extraflag(EObjFlag::kHasOneSlot);
	}

	if (!itemobj->has_flag(EObjFlag::kTransformed)) {
		itemobj->set_extra_flag(EObjFlag::kTransformed);
	}
	extract_obj(gemobj);
}

void insert_wanted_gem::show(CharData *ch, int gem_vnum) {
	alias_type::iterator alias_it;
	char buf[kMaxInputLength];

	const auto it = content.find(gem_vnum);
	if (it == content.end()) return;

	send_to_char("Будучи искусным ювелиром, вы можете выбрать, какого эффекта вы желаете добиться: \r\n", ch);
	for (alias_it = it->second.begin(); alias_it != it->second.end(); ++alias_it) {
		sprintf(buf, " %s\r\n", alias_it->first.c_str());
		send_to_char(buf, ch);
	}
}

void insert_wanted_gem::init() {
	std::ifstream file;
	char dummy;
	char buf[kMaxInputLength];
	std::string str;
	int val, val2, curr_val = 0;
	std::map<int, alias_type>::iterator it;
	alias_type temp;
	alias_type::iterator alias_it;
	struct int3 arr;

	content.clear();
	temp.clear();

	file.open(LIB_MISC "insert_wanted.lst", std::fstream::in);
	if (!file.is_open()) {
		return log("failed to open insert_wanted.lst.");
	}

	file.width(kMaxInputLength);

	while (true) {
		if (!(file >> dummy)) break;

		if (dummy == '*') {
			if (!file.getline(buf, kMaxInputLength)) break;
			continue;
		}

		if (dummy == '#') {
			if (!(file >> val)) break;

			if (!temp.empty() && (curr_val != 0)) {
				content.insert(std::make_pair(curr_val, temp));
				temp.clear();
			}
			curr_val = val;

			continue;
		}

		if (dummy == '$') {
			if (curr_val == 0) break;
			if (!(file >> str)) break;
			if (str.size() > kMaxAliasLehgt - 1) break;
			if (!(file >> val)) break;

			switch (val) {
				case 1: if (!(file >> val >> val2)) break;
					arr.type = 1;
					arr.bit = val;
					arr.qty = val2;
					temp.insert(std::make_pair(str, arr));
					break;
				case 2:
				case 3: if (!(file >> val2)) break;
					arr.type = val;
					arr.bit = val2;
					arr.qty = 0;
					temp.insert(std::make_pair(str, arr));
					break;
				case 4: if (!(file >> val >> val2)) break;
					arr.type = 4;
					arr.bit = val;
					arr.qty = val2;
					temp.insert(std::make_pair(str, arr));
					break;
				default: {
					log("something goes wrong\r\nclosed insert_wanted.lst.");
					file.close();
					return;
				}
			};

		}

	}

	file.close();
	log("closed insert_wanted.lst.");

	if (!temp.empty()) {
		content.insert(std::make_pair(curr_val, temp));
	}
}

int insert_wanted_gem::get_type(int gem_vnum, const std::string &str) {
	return content[gem_vnum][str].type;
}

int insert_wanted_gem::get_bit(int gem_vnum, const std::string &str) {
	return content[gem_vnum][str].bit;
}

int insert_wanted_gem::get_qty(int gem_vnum, const std::string &str) {
	return content[gem_vnum][str].qty;
}
bool insert_wanted_gem::is_gem(int gem_vnum) {
	const auto it = content.find(gem_vnum);
	if (it == content.end()) {
		return false;
	}
	return true;
}

std::string insert_wanted_gem::get_random_str_for(int gem_vnum) {
	const auto it = content.find(gem_vnum);
	if (it == content.end()) {
		return "";
	}

	auto gem = content[gem_vnum];
	int rnd = number(0, gem.size() - 1);

	int count = 0;
	for (const auto& kv : gem) {
		if (count == rnd) {
			return kv.first;
		}
		count++;
	}

	return "";
}

int insert_wanted_gem::exist(const int gem_vnum, const std::string &str) const {
	alias_type::const_iterator alias_it;

	const auto it = content.find(gem_vnum);
	if (it == content.end()) {
		return 0;
	}

	alias_it = content.at(gem_vnum).find(str);
	if (alias_it == content.at(gem_vnum).end()) {
		return 0;
	}

	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
