/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "ювелир" - код.
*/

#include "jewelry.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "mining.h"
#include "utils/random.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"
#include "engine/db/global_objects.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/core/constants.h"
#include "gameplay/skills/skills.h"
#include "gameplay/mechanics/illumination.h"

#include <algorithm>
#include <bit>

namespace jewelry {

JewelryCfg cfg;

namespace {

// Целочисленный атрибут DataNode; def при отсутствии/некорректном значении.
using parse::AttrInt;

// Игровое название эффекта (то, что игрок видит и набирает) по его типу и id.
// type: 1 apply (apply_types), 2 affect (affect_msg kShortDesc), 3 флаг (extra_bits),
// 4 умение (название умения). Возвращает "" если название не найдено.
std::string EffectName(int type, int id) {
	switch (type) {
		case 1:   // apply
			if (id > 0 && id < EApply::kNumberApplies) {
				return apply_types[id];
			}
			return "";
		case 2:   // affect
			return affects::AffectMsg(static_cast<EAffect>(id), affects::EAffectMsgType::kShortDesc);
		case 3: {   // extra-flag: extra_bits хранится планами, разделенными "\n"
			const auto value = static_cast<Bitvector>(id);
			const Bitvector low = value & 0x3FFFFFFFu;
			if (low == 0) {
				return "";
			}
			const int plane = static_cast<int>(value >> 30);
			const int bit = std::countr_zero(low);
			int cur_plane = 0;
			int cur_bit = 0;
			for (int i = 0; i < 256; ++i) {
				if (*extra_bits[i] == '\n') {
					if (cur_plane == plane) {
						return "";   // искомый бит вне списка имен этого плана
					}
					++cur_plane;
					cur_bit = 0;
					continue;
				}
				if (cur_plane == plane && cur_bit == bit) {
					return extra_bits[i];
				}
				++cur_bit;
			}
			return "";
		}
		case 4:   // skill
			return MUD::Skill(static_cast<ESkill>(id)).GetName();
		default:
			return "";
	}
}

// Значение скалярного тега <tag val="..."/>.
int ScalarVal(parser_wrapper::DataNode root, const char *tag, int def) {
	auto node = root;
	if (node.GoToChild(tag)) {
		return AttrInt(node, "val", def);
	}
	return def;
}

} // namespace

bool JewelryCfg::IsGem(int vnum) const {
	return gems.find(vnum) != gems.end();
}

const GemEffect *JewelryCfg::Find(int vnum, const std::string &key) const {
	const auto it = gems.find(vnum);
	if (it == gems.end()) {
		return nullptr;
	}
	for (const auto &eff : it->second) {
		if (!str_cmp(eff.key.c_str(), key.c_str())) {   // регистронезависимо
			return &eff;
		}
	}
	return nullptr;
}

std::string JewelryCfg::RandomKey(int vnum) const {
	const auto it = gems.find(vnum);
	if (it == gems.end() || it->second.empty()) {
		return "";
	}
	return it->second[number(0, static_cast<int>(it->second.size()) - 1)].key;
}

void JewelryCfg::Show(CharData *ch, int vnum) const {
	const auto it = gems.find(vnum);
	if (it == gems.end()) {
		return;
	}
	SendMsgToChar("Будучи искусным ювелиром, вы можете выбрать, какого эффекта вы желаете добиться: \r\n", ch);
	for (const auto &eff : it->second) {
		char buf[kMaxInputLength];
		sprintf(buf, " %s\r\n", eff.key.c_str());
		SendMsgToChar(buf, ch);
	}
}

void JewelryLoader::Load(parser_wrapper::DataNode data) {
	JewelryCfg tmp;   // собираем во временный, чтобы при ошибке не повредить рабочий

	tmp.lag = ScalarVal(data, "lag", tmp.lag);
	tmp.skill_divisor = ScalarVal(data, "skill_divisor", tmp.skill_divisor);
	tmp.skill_penalty_for_affect = ScalarVal(data, "skill_penalty_for_affect", tmp.skill_penalty_for_affect);
	tmp.target_obj_crash_percent = ScalarVal(data, "target_obj_crash_percent", tmp.target_obj_crash_percent);
	tmp.target_obj_timer_increment_percent =
		ScalarVal(data, "target_obj_timer_increment_percent", tmp.target_obj_timer_increment_percent);
	tmp.target_obj_timer_decrement_percent =
		ScalarVal(data, "target_obj_timer_decrement_percent", tmp.target_obj_timer_decrement_percent);
	tmp.desired_min_skill = ScalarVal(data, "desired_min_skill", tmp.desired_min_skill);
	tmp.desired_foreign_min_skill = ScalarVal(data, "desired_foreign_min_skill", tmp.desired_foreign_min_skill);
	tmp.desired_success_offset = ScalarVal(data, "desired_success_offset", tmp.desired_success_offset);

	// <gems><gem vnum=""><apply|affect|flag|skill .../></gem></gems>
	auto gems_node = data;
	if (gems_node.GoToChild("gems")) {
		for (auto &gem_node : gems_node.Children("gem")) {
			const int vnum = AttrInt(gem_node, "vnum", 0);
			if (!parse::IsValidObjVnum(vnum)) {
				continue;
			}
			std::vector<GemEffect> effects;
			for (auto &eff_node : gem_node.Children()) {
				const std::string kind = eff_node.GetName();
				const char *id_str = eff_node.GetValue("id");
				GemEffect eff;
				try {
					if (kind == "apply") {
						eff.type = 1;
						eff.id = static_cast<int>(parse::ReadAsConstant<EApply>(id_str));
						eff.val = AttrInt(eff_node, "val", 0);
					} else if (kind == "affect") {
						eff.type = 2;
						eff.id = static_cast<int>(parse::ReadAsConstant<EAffect>(id_str));
					} else if (kind == "flag") {
						eff.type = 3;
						eff.id = static_cast<int>(parse::ReadAsConstant<EObjFlag>(id_str));
					} else if (kind == "skill") {
						eff.type = 4;
						eff.id = static_cast<int>(parse::ReadAsConstant<ESkill>(id_str));
						eff.val = AttrInt(eff_node, "val", 0);
					} else {
						continue;   // неизвестный тег внутри <gem>
					}
				} catch (const std::exception &) {
					snprintf(buf, kMaxStringLength, "jewelry: gem %d: bad <%s id='%s'>",
							 vnum, kind.c_str(), id_str ? id_str : "");
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}
				const char *alias = eff_node.GetValue("alias");
				if (alias && *alias) {
					eff.key = alias;
				} else {
					eff.key = EffectName(eff.type, eff.id);   // игровое название эффекта
					if (eff.key.empty()) {
						eff.key = id_str ? id_str : "";   // запасной вариант: id-токен
						snprintf(buf, kMaxStringLength,
								 "jewelry: gem %d: no display name for <%s id='%s'>, using id token as key",
								 vnum, kind.c_str(), id_str ? id_str : "");
						mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					}
				}
				effects.push_back(eff);
			}
			if (!effects.empty()) {
				tmp.gems[vnum] = std::move(effects);
			}
		}
	}

	cfg = std::move(tmp);
}

void JewelryLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

} // namespace jewelry

bool is_dig_stone(ObjData *obj) {
	const int vnum = obj->get_vnum();
	for (const auto &stone : mining::dig_cfg.stones) {
		if (vnum == stone.stone_vnum || vnum == stone.glass_vnum) {
			return true;
		}
	}

	return jewelry::cfg.IsGem(vnum);
}

void do_insertgem(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	int percent, prob;
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	char gem[kMaxInputLength], item[kMaxInputLength];
	ObjData *gemobj, *itemobj;

	argument = two_arguments(argument, arg1, arg2);

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kJewelry)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!privilege::IsImmortal(ch)) {
		if (!ROOM_FLAGGED(ch->in_room, ERoomFlag::kForge)) {
			SendMsgToChar("Вам нужно попасть в кузницу для этого.\r\n", ch);
			return;
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы!\r\n", ch);
		return;
	}

	if (is_dark(ch->in_room) && !sight::CanSeeInDark(ch) && !privilege::IsImmortal(ch)) {
		SendMsgToChar("Да тут темно хоть глаза выколи...\r\n", ch);
		return;
	}

	if (!privilege::IsImmortal(ch) && mount::IsOnHorse(ch)) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (!*arg1) {
		SendMsgToChar("Вплавить что?\r\n", ch);
		return;
	} else
		strcpy(gem, arg1);

	if (!(gemobj = get_obj_in_list_vis(ch, gem, ch->carrying))) {
		sprintf(buf, "У вас нет '%s'.\r\n", gem);
		SendMsgToChar(buf, ch);
		return;
	}

	if (!is_dig_stone(gemobj)) {
		sprintf(buf, "Вы не умеете вплавлять %s.\r\n", gemobj->get_PName(grammar::ECase::kAcc).c_str());
		SendMsgToChar(buf, ch);
		return;
	}

	if (!*arg2) {
		SendMsgToChar("Вплавить во что?\r\n", ch);
		return;
	} else
		strcpy(item, arg2);

	if (!(itemobj = get_obj_in_list_vis(ch, item, ch->carrying))) {
		sprintf(buf, "У вас нет '%s'.\r\n", item);
		SendMsgToChar(buf, ch);
		return;
	}
	if (itemobj->get_material() == EObjMaterial::kMaterialUndefined || (itemobj->get_material() > EObjMaterial::kPreciousMetel)) {
		if (!(itemobj->get_material() == EObjMaterial::kBone || itemobj->get_material() == EObjMaterial::kStone)) {
			sprintf(buf, "%s состоит из неподходящего материала.\r\n", itemobj->get_PName(grammar::ECase::kNom).c_str());
			SendMsgToChar(buf, ch);
			return;
		}
	}
	if (!itemobj->has_flag(EObjFlag::kHasOneSlot)
		&& !itemobj->has_flag(EObjFlag::kHasTwoSlots)
		&& !itemobj->has_flag(EObjFlag::kHasThreeSlots)) {
		SendMsgToChar("Вы не видите куда здесь можно вплавить камень.\r\n", ch);
		return;
	}

	percent = number(1, MUD::Skill(ESkill::kJewelry).difficulty);
	prob = GetSkill(ch, ESkill::kJewelry);

	SetBattleLag(ch, jewelry::cfg.lag);

	for (int i = 0; i < kMaxObjAffect; i++) {
		if (itemobj->get_affected(i).location == EApply::kNone) {
			prob -= i * jewelry::cfg.skill_penalty_for_affect;
			break;
		}
	}

	for (const auto &i : weapon_affect) {
		if (itemobj->GetEWeaponAffect(i.aff_pos)) {
			prob -= jewelry::cfg.skill_penalty_for_affect;
		}
	}

	// остаток строки целиком - игровое название желаемого эффекта (может быть из нескольких слов)
	skip_spaces(&argument);
	strncpy(arg3, argument, kMaxInputLength - 1);
	arg3[kMaxInputLength - 1] = '\0';

	if (!*arg3) {
		ImproveSkill(ch, ESkill::kJewelry, 0, nullptr);

		if (percent > prob / std::max(1, jewelry::cfg.skill_divisor)) {
			sprintf(buf, "Вы неудачно попытались вплавить %s в %s, испортив камень...\r\n",
					gemobj->get_short_description().c_str(),
					itemobj->get_PName(grammar::ECase::kAcc).c_str());
			SendMsgToChar(buf, ch);
			sprintf(buf, "$n испортил$g %s, вплавляя его в %s!\r\n",
					gemobj->get_PName(grammar::ECase::kAcc).c_str(),
					itemobj->get_PName(grammar::ECase::kAcc).c_str());
			act(buf, false, ch, nullptr, nullptr, kToRoom);
			ExtractObjFromWorld(gemobj);
			if (number(1, 100) <= jewelry::cfg.target_obj_crash_percent) {
				sprintf(buf, "...и испортив хорошую вещь!\r\n");
				SendMsgToChar(buf, ch);
				sprintf(buf, "$n испортил$g %s!\r\n", itemobj->get_PName(grammar::ECase::kAcc).c_str());
				act(buf, false, ch, nullptr, nullptr, kToRoom);
				ExtractObjFromWorld(itemobj);
			}
			return;
		}
	} else {
		if (GetSkill(ch, ESkill::kJewelry) < jewelry::cfg.desired_min_skill) {
			sprintf(buf, "Вы должны достигнуть мастерства в умении ювелир, чтобы вплавлять желаемые аффекты!\r\n");
			SendMsgToChar(buf, ch);
			return;

		}
		if (itemobj->get_owner() != ch->get_uid()
			&& (GetSkill(ch, ESkill::kJewelry) < jewelry::cfg.desired_foreign_min_skill)) {
			sprintf(buf, "Вы недостаточно искусны и можете вплавлять желаемые аффекты только в перековку!\r\n");
			SendMsgToChar(buf, ch);
			return;
		}

		std::string str(arg3);
		if (!jewelry::cfg.Find(GET_OBJ_VNUM(gemobj), str)) {
			jewelry::cfg.Show(ch, GET_OBJ_VNUM(gemobj));
			return;
		}

		ImproveSkill(ch, ESkill::kJewelry, 0, nullptr);

		//успех или фэйл? при 80% скила успех 30% при 100% скила 50% при 200% скила успех 75%
		if (number(1, GetSkill(ch, ESkill::kJewelry))
			> (GetSkill(ch, ESkill::kJewelry) - jewelry::cfg.desired_success_offset)) {
			sprintf(buf, "Вы неудачно попытались вплавить %s в %s, испортив камень...\r\n",
					gemobj->get_short_description().c_str(),
					itemobj->get_PName(grammar::ECase::kAcc).c_str());
			SendMsgToChar(buf, ch);
			sprintf(buf, "$n испортил$g %s, вплавляя его в %s!\r\n",
					gemobj->get_PName(grammar::ECase::kAcc).c_str(),
					itemobj->get_PName(grammar::ECase::kAcc).c_str());
			act(buf, false, ch, nullptr, nullptr, kToRoom);
			ExtractObjFromWorld(gemobj);
			return;
		}
	}

	sprintf(buf, "Вы вплавили %s в %s!\r\n", gemobj->get_PName(grammar::ECase::kAcc).c_str(), itemobj->get_PName(grammar::ECase::kAcc).c_str());
	SendMsgToChar(buf, ch);
	sprintf(buf, "$n вплавил$g %s в %s.\r\n", gemobj->get_PName(grammar::ECase::kAcc).c_str(), itemobj->get_PName(grammar::ECase::kAcc).c_str());
	act(buf, false, ch, nullptr, nullptr, kToRoom);

	if (itemobj->get_owner() == ch->get_uid()) {
		int timer = itemobj->get_timer() + itemobj->get_timer() / 100 * jewelry::cfg.target_obj_timer_increment_percent;
		itemobj->set_timer(timer);
	} else {
		int timer = itemobj->get_timer() - itemobj->get_timer() / 100 * jewelry::cfg.target_obj_timer_decrement_percent;
		itemobj->set_timer(timer);
	}

	if (gemobj->get_material() == EObjMaterial::kDiamond) {
		std::string effect;
		if (!*arg3) {
			effect = jewelry::cfg.RandomKey(GET_OBJ_VNUM(gemobj));
		} else {
			effect = arg3;
		}

		const auto *eff = jewelry::cfg.Find(GET_OBJ_VNUM(gemobj), effect);
		if (eff) {
			switch (eff->type) {
				case 1:
					set_obj_eff(itemobj, static_cast<EApply>(eff->id), eff->val);
					break;
				case 2:
					set_obj_aff(itemobj, static_cast<EAffect>(eff->id));
					break;
				case 3:
					itemobj->set_extra_flag(static_cast<EObjFlag>(eff->id));
					break;
				case 4:
					itemobj->set_skill(static_cast<ESkill>(eff->id), eff->val);
					break;
				default:
					break;
			}
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
	itemobj->set_extra_flag(EObjFlag::kLimitedTimer);
	ExtractObjFromWorld(gemobj);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
