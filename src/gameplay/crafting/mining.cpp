/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "горное дело" - код.
*/

#include "mining.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/illumination.h"
#include "utils/parse.h"
#include "utils/utils_string.h"

#include <algorithm>

extern void split_or_clan_tax(CharData *ch, long amount);

namespace mining {

DiggingCfg dig_cfg;

namespace {

// Целочисленный атрибут DataNode; def при отсутствии/некорректном значении.
int AttrInt(parser_wrapper::DataNode &node, const char *key, int def) {
	const char *v = node.GetValue(key);
	if (!v || !*v) {
		return def;
	}
	try {
		return parse::ReadAsInt(v);
	} catch (const std::exception &) {
		return def;
	}
}

// Разбор списка vnum'ов вида "900|901|902" в вектор.
std::vector<int> ParseVnumList(const char *value) {
	std::vector<int> result;
	if (!value || !*value) {
		return result;
	}
	for (const auto &token : utils::Split(value, '|')) {
		try {
			result.push_back(std::stoi(token));
		} catch (...) {
			err_log("digging: vnum '%s' cannot be converted into num.", token.c_str());
		}
	}
	return result;
}

} // namespace

void DiggingLoader::Load(parser_wrapper::DataNode data) {
	DiggingCfg tmp;   // собираем во временный, чтобы при ошибке не повредить рабочий

	// <stones><stone rank stone_vnum glass_vnum skill/></stones>
	{
		auto node = data;
		if (node.GoToChild("stones")) {
			for (auto &stone_node : node.Children("stone")) {
				DigStone s;
				s.rank = AttrInt(stone_node, "rank", 0);
				s.stone_vnum = AttrInt(stone_node, "stone_vnum", 0);
				s.glass_vnum = AttrInt(stone_node, "glass_vnum", 0);
				s.skill = AttrInt(stone_node, "skill", 0);
				tmp.stones.push_back(s);
			}
		}
	}
	// упорядочиваем по возрастанию навыка (на случай произвольного порядка в файле)
	std::sort(tmp.stones.begin(), tmp.stones.end(),
			  [](const DigStone &a, const DigStone &b) { return a.skill < b.skill; });

	auto scalar = [&](const char *tag, int def) -> int {
		auto node = data;
		if (node.GoToChild(tag)) {
			return AttrInt(node, "val", def);
		}
		return def;
	};

	tmp.hole_max_deep = scalar("hole_max_deep", tmp.hole_max_deep);
	tmp.tool_crash_chance = scalar("tool_crash_chance", tmp.tool_crash_chance);
	tmp.treasure_chance = scalar("treasure_chance", tmp.treasure_chance);
	tmp.lag = scalar("lag", tmp.lag);
	tmp.skill_divisor = scalar("skill_divisor", tmp.skill_divisor);
	tmp.glass_chance = scalar("glass_chance", tmp.glass_chance);
	tmp.moves_expense = scalar("moves_expense", tmp.moves_expense);

	// <jackpot vnum chance/>
	{
		auto node = data;
		if (node.GoToChild("jackpot")) {
			tmp.jackpot_vnum = AttrInt(node, "vnum", tmp.jackpot_vnum);
			tmp.jackpot_chance = AttrInt(node, "chance", tmp.jackpot_chance);
		}
	}
	// <mob vnum="a|b|..." chance/>
	{
		auto node = data;
		if (node.GoToChild("mob")) {
			tmp.mob_vnums = ParseVnumList(node.GetValue("vnum"));
			tmp.mob_chance = AttrInt(node, "chance", tmp.mob_chance);
		}
	}
	// <trash vnum="a|b|..." chance/>
	{
		auto node = data;
		if (node.GoToChild("trash")) {
			tmp.trash_vnums = ParseVnumList(node.GetValue("vnum"));
			tmp.trash_chance = AttrInt(node, "chance", tmp.trash_chance);
		}
	}

	dig_cfg = std::move(tmp);
}

void DiggingLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

} // namespace mining

using mining::dig_cfg;

int make_hole(CharData *ch) {
	if (round_up(world[ch->in_room]->holes / kHolesTime) >= dig_cfg.hole_max_deep) {
		SendMsgToChar("Тут и так все перекопано.\r\n", ch);
		return 0;
	}

	return 1;
}

void break_inst(CharData *ch) {
	int i;
	char buf[300];

	for (i = EEquipPos::kWield; i <= EEquipPos::kBoths; i++) {
		if (GET_EQ(ch, i)
			&& (strstr(GET_EQ(ch, i)->get_aliases().c_str(), "лопата")
				|| strstr(GET_EQ(ch, i)->get_aliases().c_str(), "кирка"))) {
			if (GET_EQ(ch, i)->get_current_durability() > 1) {
				if (number(1, dig_cfg.tool_crash_chance) == 1) {
					const auto current = GET_EQ(ch, i)->get_current_durability();
					GET_EQ(ch, i)->set_current_durability(current - 1);
				}
			} else {
				GET_EQ(ch, i)->set_timer(0);
			}
			if (GET_EQ(ch, i)->get_current_durability() <= 1 && number(1, 3) == 1) {
				sprintf(buf, "Ваша %s трескается!\r\n", GET_EQ(ch, i)->get_short_description().c_str());
				SendMsgToChar(buf, ch);
			}
		}
	}

}

int check_for_dig(CharData *ch) {
	int i;

	for (i = EEquipPos::kWield; i <= EEquipPos::kBoths; i++) {
		if (GET_EQ(ch, i)
			&& (strstr(GET_EQ(ch, i)->get_aliases().c_str(), "лопата")
				|| strstr(GET_EQ(ch, i)->get_aliases().c_str(), "кирка"))) {
			return 1;
		}
	}
	return 0;
}

void dig_obj(CharData *ch, ObjData *obj) {
	char textbuf[300];

	if (GetObjMIW(obj->get_rnum()) >= obj_proto.actual_count(obj->get_rnum())
		|| GetObjMIW(obj->get_rnum()) == ObjData::UNLIMITED_GLOBAL_MAXIMUM) {
		sprintf(textbuf, "Вы нашли %s!\r\n", obj->get_PName(grammar::ECase::kAcc).c_str());
		SendMsgToChar(textbuf, ch);
		sprintf(textbuf, "$n выкопал$g %s!\r\n", obj->get_PName(grammar::ECase::kAcc).c_str());
		act(textbuf, false, ch, nullptr, nullptr, kToRoom);
		if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
			SendMsgToChar("Вы не смогли унести столько предметов.\r\n", ch);
			PlaceObjToRoom(obj, ch->in_room);
		} else if (ch->GetCarryingWeight() + obj->get_weight() > CAN_CARRY_W(ch)) {
			SendMsgToChar("Вы не смогли унести такой веc.\r\n", ch);
			PlaceObjToRoom(obj, ch->in_room);
		} else {
			PlaceObjToInventory(obj, ch);
		}
	}
}

void do_dig(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *mob;
	char textbuf[300];
	int percent, prob;
	int random_stone;
	int vnum;
	int old_int;

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kDigging)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!check_for_dig(ch) && !privilege::IsImmortal(ch)) {
		SendMsgToChar("Вам бы лопату взять в руки... Или кирку...\r\n", ch);
		return;
	}

	if (world[ch->in_room]->sector_type != ESector::kMountain &&
		world[ch->in_room]->sector_type != ESector::kHills && !privilege::IsImmortal(ch)) {
		SendMsgToChar("Полезные минералы водятся только в гористой местности!\r\n", ch);
		return;
	}

	if (!privilege::IsImmortal(ch) && mount::IsOnHorse(ch)) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind) && !privilege::IsImmortal(ch)) {
		SendMsgToChar("Вы слепы и не видите где копать.\r\n", ch);
		return;
	}

	if (is_dark(ch->in_room) && !sight::CanSeeInDark(ch) && !privilege::IsImmortal(ch)) {
		SendMsgToChar("Куда копать? Чего копать? Ничего не видно...\r\n", ch);
		return;
	}

	if (!make_hole(ch) && !privilege::IsImmortal(ch))
		return;

	if (!check_moves(ch, dig_cfg.moves_expense))
		return;

	world[ch->in_room]->holes += kHolesTime;

	SendMsgToChar("Вы стали усердно ковырять каменистую почву...\r\n", ch);
	act("$n стал$g усердно ковырять каменистую почву...", false, ch, nullptr, nullptr, kToRoom);

	break_inst(ch);

	// копнули клад
	if (number(1, dig_cfg.treasure_chance) == 1) {
		int gold = number(40000, 60000);
		SendMsgToChar("Вы нашли клад!\r\n", ch);
		act("$n выкопал$g клад!", false, ch, nullptr, nullptr, kToRoom);
		sprintf(textbuf, "Вы насчитали %i монет.\r\n", gold);
		SendMsgToChar(textbuf, ch);
		currencies::AddHand(*ch, currencies::kGold, gold);
		sprintf(buf, "<%s> {%d} нарыл %d кун.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), gold);
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		split_or_clan_tax(ch, gold);
		return;
	}
	// копнули мертвяка
	if (number(1, dig_cfg.mob_chance) == 1 && !dig_cfg.mob_vnums.empty()) {
		vnum = dig_cfg.mob_vnums[number(0, static_cast<int>(dig_cfg.mob_vnums.size()) - 1)];
		mob = ReadMobile(GetMobRnum(vnum), kReal);
		if (mob) {
			if (GetRealLevel(mob) <= GetRealLevel(ch)) {
				mob->SetFlag(EMobFlag::kAgressive);
				sprintf(textbuf, "Вы выкопали %s!\r\n", mob->player_data.PNames[grammar::ECase::kAcc].c_str());
				SendMsgToChar(textbuf, ch);
				sprintf(textbuf, "$n выкопал$g %s!\r\n", mob->player_data.PNames[grammar::ECase::kAcc].c_str());
				act(textbuf, false, ch, nullptr, nullptr, kToRoom);
				PlaceCharToRoom(mob, ch->in_room);
				return;
			}
		} else
			SendMsgToChar("Не найден прототип обжекта!", ch);
	}
	// копнули шкатулку пандоры
	ObjData::shared_ptr obj;
	if (number(1, dig_cfg.jackpot_chance) == 1) {
		vnum = dig_cfg.jackpot_vnum;

		obj = world_objects.create_from_prototype_by_vnum(vnum);
		if (obj) {
			dig_obj(ch, obj.get());
		} else {
			SendMsgToChar("Не найден прототип обжекта!", ch);
		}

		return;
	}
	// копнули мусор
	if (number(1, dig_cfg.trash_chance) == 1 && !dig_cfg.trash_vnums.empty()) {
		vnum = dig_cfg.trash_vnums[number(0, static_cast<int>(dig_cfg.trash_vnums.size()) - 1)];
		obj = world_objects.create_from_prototype_by_vnum(vnum);

		if (obj) {
			dig_obj(ch, obj.get());
		} else {
			SendMsgToChar("Не найден прототип обжекта!", ch);
		}

		return;
	}

	percent = number(1, MUD::Skill(ESkill::kDigging).difficulty);
	prob = GetSkill(ch, ESkill::kDigging);
	old_int = ch->get_int();
	ch->set_int(13);
	ch->set_int_add(0);
	ImproveSkill(ch, ESkill::kDigging, 0, nullptr);
	ch->set_int(old_int);
	SetBattleLag(ch, dig_cfg.lag);
	if (percent > prob / dig_cfg.skill_divisor) {
		SendMsgToChar("Вы только зря расковыряли землю и раскидали камни.\r\n", ch);
		act("$n отрыл$g смешную ямку.", false, ch, nullptr, nullptr, kToRoom);
		return;
	}
	// возможность копать мощные камни зависит от навыка: берется рандом от 1 до
	// уровня скилла, и среди подходящих по навыку камней выбирается самый "крутой"
	random_stone = number(1, MIN(prob, 100));
	const mining::DigStone *chosen = nullptr;
	for (const auto &stone : dig_cfg.stones) {   // камни идут по возрастанию навыка
		if (random_stone >= stone.skill) {
			chosen = &stone;
		}
	}

	if (!chosen) {
		SendMsgToChar("Вы долго копали, но так и не нашли ничего полезного.\r\n", ch);
		act("$n долго копал$g землю, но все без толку.", false, ch, nullptr, nullptr, kToRoom);
		return;
	}

	vnum = chosen->stone_vnum;
	obj = world_objects.create_from_prototype_by_vnum(vnum);
	if (obj) {
		if (number(1, dig_cfg.glass_chance) != 1) {
			obj->set_material(EObjMaterial::kGlass);
		} else {
			obj->set_material(EObjMaterial::kDiamond);
		}

		dig_obj(ch, obj.get());
	} else {
		SendMsgToChar("Не найден прототип обжекта!", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
