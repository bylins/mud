/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "горное дело" - заголовок.
*/

#ifndef BYLINS_SRC_CRAFTS_MINING_H_
#define BYLINS_SRC_CRAFTS_MINING_H_

class CHAR_DATA;

// Значения по умолчанию могут быть изменены при чтении файла
struct skillvariables_dig {
	int hole_max_deep = 10;
	int instr_crash_chance = 2;
	int treasure_chance = 30000;
	int pandora_chance = 80000;
	int mob_chance = 300;
	int trash_chance = 100;
	int lag = 4;
	int prob_divide = 3;
	int glass_chance = 3;
	int need_moves = 15;

	int stone1_skill = 15;
	int stone2_skill = 25;
	int stone3_skill = 35;
	int stone4_skill = 50;
	int stone5_skill = 70;
	int stone6_skill = 80;
	int stone7_skill = 90;
	int stone8_skill = 95;
	int stone9_skill = 99;

	int stone1_vnum = 900;
	int last_stone_vnum = 917;
	int trash_vnum_start = 920;
	int trash_vnum_end = 922;
	int mob_vnum_start = 100;
	int mob_vnum_end = 103;
	int pandora_vnum = 919;
	int glass_vnum = 1919;
};

// Перенести в глобальные объекты
extern skillvariables_dig dig_vars;

void InitMiningVars();
void do_dig(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CRAFTS_MINING_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
