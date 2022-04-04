/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "горное дело" - код.
*/

#include "mining.h"

#include "entities/char_data.h"
#include "handler.h"
#include "obj_prototypes.h"
#include "structs/global_objects.h"

skillvariables_dig dig_vars;
extern void split_or_clan_tax(CharData *ch, long amount);

void InitMiningVars() {
	char line[256];
	FILE *cfg_file;

	if (!(cfg_file = fopen(LIB_MISC "skillvariables.lst", "r"))) {
		log("Cann't open skillvariables list file...");
		graceful_exit(1);
	}

	while (get_line(cfg_file, line)) {
		if (!line[0] || line[0] == ';')
			continue;

		sscanf(line, "dig_hole_max_deep %d", &dig_vars.hole_max_deep);
		sscanf(line, "dig_instr_crash_chance %d", &dig_vars.instr_crash_chance);
		sscanf(line, "dig_treasure_chance %d", &dig_vars.treasure_chance);
		sscanf(line, "dig_pandora_chance %d", &dig_vars.pandora_chance);
		sscanf(line, "dig_mob_chance %d", &dig_vars.mob_chance);
		sscanf(line, "dig_trash_chance %d", &dig_vars.trash_chance);
		sscanf(line, "dig_lag %d", &dig_vars.lag);
		sscanf(line, "dig_prob_divide %d", &dig_vars.prob_divide);
		sscanf(line, "dig_glass_chance %d", &dig_vars.glass_chance);
		sscanf(line, "dig_need_moves %d", &dig_vars.need_moves);

		sscanf(line, "dig_stone1_skill %d", &dig_vars.stone1_skill);
		sscanf(line, "dig_stone2_skill %d", &dig_vars.stone2_skill);
		sscanf(line, "dig_stone3_skill %d", &dig_vars.stone3_skill);
		sscanf(line, "dig_stone4_skill %d", &dig_vars.stone4_skill);
		sscanf(line, "dig_stone5_skill %d", &dig_vars.stone5_skill);
		sscanf(line, "dig_stone6_skill %d", &dig_vars.stone6_skill);
		sscanf(line, "dig_stone7_skill %d", &dig_vars.stone7_skill);
		sscanf(line, "dig_stone8_skill %d", &dig_vars.stone8_skill);
		sscanf(line, "dig_stone9_skill %d", &dig_vars.stone9_skill);

		sscanf(line, "dig_stone1_vnum %d", &dig_vars.stone1_vnum);
		sscanf(line, "dig_trash_vnum_start %d", &dig_vars.trash_vnum_start);
		sscanf(line, "dig_trash_vnum_end %d", &dig_vars.trash_vnum_end);
		sscanf(line, "dig_mob_vnum_start %d", &dig_vars.mob_vnum_start);
		sscanf(line, "dig_mob_vnum_end %d", &dig_vars.mob_vnum_end);
		sscanf(line, "dig_pandora_vnum %d", &dig_vars.pandora_vnum);

		line[0] = '\0';
	}
	fclose(cfg_file);
}

int make_hole(CharData *ch) {
	if (roundup(world[ch->in_room]->holes / kHolesTime) >= dig_vars.hole_max_deep) {
		send_to_char("Тут и так все перекопано.\r\n", ch);
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
			if (GET_OBJ_CUR(GET_EQ(ch, i)) > 1) {
				if (number(1, dig_vars.instr_crash_chance) == 1) {
					const auto current = GET_EQ(ch, i)->get_current_durability();
					GET_EQ(ch, i)->set_current_durability(current - 1);
				}
			} else {
				GET_EQ(ch, i)->set_timer(0);
			}
			if (GET_OBJ_CUR(GET_EQ(ch, i)) <= 1 && number(1, 3) == 1) {
				sprintf(buf, "Ваша %s трескается!\r\n", GET_EQ(ch, i)->get_short_description().c_str());
				send_to_char(buf, ch);
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

	if (GET_OBJ_MIW(obj) >= obj_proto.actual_count(obj->get_rnum())
		|| GET_OBJ_MIW(obj) == ObjData::UNLIMITED_GLOBAL_MAXIMUM) {
		sprintf(textbuf, "Вы нашли %s!\r\n", obj->get_PName(3).c_str());
		send_to_char(textbuf, ch);
		sprintf(textbuf, "$n выкопал$g %s!\r\n", obj->get_PName(3).c_str());
		act(textbuf, false, ch, nullptr, nullptr, kToRoom);
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
			send_to_char("Вы не смогли унести столько предметов.\r\n", ch);
			PlaceObjToRoom(obj, ch->in_room);
		} else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
			send_to_char("Вы не смогли унести такой веc.\r\n", ch);
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
	int stone_num, random_stone;
	int vnum;
	int old_wis, old_int;

	if (ch->is_npc() || !ch->get_skill(ESkill::kDigging)) {
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!check_for_dig(ch) && !IS_IMMORTAL(ch)) {
		send_to_char("Вам бы лопату взять в руки... Или кирку...\r\n", ch);
		return;
	}

	if (world[ch->in_room]->sector_type != ESector::kMountain &&
		world[ch->in_room]->sector_type != ESector::kHills && !IS_IMMORTAL(ch)) {
		send_to_char("Полезные минералы водятся только в гористой местности!\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы слепы и не видите где копать.\r\n", ch);
		return;
	}

	if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !IS_IMMORTAL(ch)) {
		send_to_char("Куда копать? Чего копать? Ничего не видно...\r\n", ch);
		return;
	}

	if (!make_hole(ch) && !IS_IMMORTAL(ch))
		return;

	if (!check_moves(ch, dig_vars.need_moves))
		return;

	world[ch->in_room]->holes += kHolesTime;

	send_to_char("Вы стали усердно ковырять каменистую почву...\r\n", ch);
	act("$n стал$g усердно ковырять каменистую почву...", false, ch, nullptr, nullptr, kToRoom);

	break_inst(ch);

	// копнули клад
	if (number(1, dig_vars.treasure_chance) == 1) {
		int gold = number(40000, 60000);
		send_to_char("Вы нашли клад!\r\n", ch);
		act("$n выкопал$g клад!", false, ch, nullptr, nullptr, kToRoom);
		sprintf(textbuf, "Вы насчитали %i монет.\r\n", gold);
		send_to_char(textbuf, ch);
		ch->add_gold(gold);
		sprintf(buf, "<%s> {%d} нарыл %d кун.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), gold);
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		split_or_clan_tax(ch, gold);
		return;
	}
	// копнули мертвяка
	if (number(1, dig_vars.mob_chance) == 1) {
		vnum = number(dig_vars.mob_vnum_start, dig_vars.mob_vnum_end);
		mob = read_mobile(real_mobile(vnum), REAL);
		if (mob) {
			if (GetRealLevel(mob) <= GetRealLevel(ch)) {
				MOB_FLAGS(mob).set(EMobFlag::kAgressive);
				sprintf(textbuf, "Вы выкопали %s!\r\n", mob->player_data.PNames[3].c_str());
				send_to_char(textbuf, ch);
				sprintf(textbuf, "$n выкопал$g %s!\r\n", mob->player_data.PNames[3].c_str());
				act(textbuf, false, ch, nullptr, nullptr, kToRoom);
				PlaceCharToRoom(mob, ch->in_room);
				return;
			}
		} else
			send_to_char("Не найден прототип обжекта!", ch);
	}
	// копнули шкатулку пандоры
	ObjData::shared_ptr obj;
	if (number(1, dig_vars.pandora_chance) == 1) {
		vnum = dig_vars.pandora_vnum;

		obj = world_objects.create_from_prototype_by_vnum(vnum);
		if (obj) {
			dig_obj(ch, obj.get());
		} else {
			send_to_char("Не найден прототип обжекта!", ch);
		}

		return;
	}
	// копнули мусор
	if (number(1, dig_vars.trash_chance) == 1){
		vnum = number(dig_vars.trash_vnum_start, dig_vars.trash_vnum_end);
		obj = world_objects.create_from_prototype_by_vnum(vnum);

		if (obj) {
			dig_obj(ch, obj.get());
		} else {
			send_to_char("Не найден прототип обжекта!", ch);
		}

		return;
	}

	percent = number(1, MUD::Skills()[ESkill::kDigging].difficulty);
	prob = ch->get_skill(ESkill::kDigging);
	old_int = ch->get_int();
	old_wis = ch->get_wis();
	ch->set_int(ch->get_int() + 14 - MAX(14, GET_REAL_INT(ch)));
	ch->set_wis(ch->get_wis() + 14 - MAX(14, GET_REAL_WIS(ch)));
	ImproveSkill(ch, ESkill::kDigging, 0, nullptr);
	ch->set_int(old_int);
	ch->set_wis(old_wis);

	WAIT_STATE(ch, dig_vars.lag * kPulseViolence);

	if (percent > prob / dig_vars.prob_divide) {
		send_to_char("Вы только зря расковыряли землю и раскидали камни.\r\n", ch);
		act("$n отрыл$g смешную ямку.", false, ch, nullptr, nullptr, kToRoom);
		return;
	}

	// возможность копать мощные камни зависит от навыка

	random_stone = number(1, MIN(prob, 100));
	if (random_stone >= dig_vars.stone9_skill)
		stone_num = 9;
	else if (random_stone >= dig_vars.stone8_skill)
		stone_num = 8;
	else if (random_stone >= dig_vars.stone7_skill)
		stone_num = 7;
	else if (random_stone >= dig_vars.stone6_skill)
		stone_num = 6;
	else if (random_stone >= dig_vars.stone5_skill)
		stone_num = 5;
	else if (random_stone >= dig_vars.stone4_skill)
		stone_num = 4;
	else if (random_stone >= dig_vars.stone3_skill)
		stone_num = 3;
	else if (random_stone >= dig_vars.stone2_skill)
		stone_num = 2;
	else if (random_stone >= dig_vars.stone1_skill)
		stone_num = 1;
	else
		stone_num = 0;

	if (stone_num == 0) {
		send_to_char("Вы долго копали, но так и не нашли ничего полезного.\r\n", ch);
		act("$n долго копал$g землю, но все без толку.", false, ch, nullptr, nullptr, kToRoom);
		return;
	}

	vnum = dig_vars.stone1_vnum - 1 + stone_num;
	obj = world_objects.create_from_prototype_by_vnum(vnum);
	if (obj) {
		if (number(1, dig_vars.glass_chance) != 1) {
			obj->set_material(EObjMaterial::kGlass);
		} else {
			obj->set_material(EObjMaterial::kDiamond);
		}

		dig_obj(ch, obj.get());
	} else {
		send_to_char("Не найден прототип обжекта!", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
