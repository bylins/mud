/*
	Константы, определяющие количество слотов под заклинания
	и прочий код для работы с ними.
*/

#include "spell.slots.hpp"
#include "spells.info.h"
#include "structs.h"

namespace PlayerClass {

const short SPELL_SLOTS_FOR_IMMORTAL = 10;

const int MAG_SLOTS[][MAX_SLOT] = { {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 1
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 2
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 3
	{3, 1, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 4
	{3, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 5
	{4, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 6
	{4, 2, 1, 0, 0, 0, 0, 0, 0, 0},	// lvl 7
	{4, 3, 1, 0, 0, 0, 0, 0, 0, 0},	// lvl 8
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// lvl 9
	{4, 3, 2, 1, 0, 0, 0, 0, 0, 0},	// lvl 10
	{5, 3, 2, 1, 0, 0, 0, 0, 0, 0},	// lvl 11
	{5, 4, 2, 2, 0, 0, 0, 0, 0, 0},	// lvl 12
	{5, 4, 3, 2, 0, 0, 0, 0, 0, 0},	// lvl 13
	{5, 4, 3, 2, 1, 0, 0, 0, 0, 0},	// lvl 14
	{5, 4, 3, 3, 1, 0, 0, 0, 0, 0},	// lvl 15
	{5, 4, 3, 3, 2, 0, 0, 0, 0, 0},	// lvl 16
	{5, 5, 4, 3, 2, 0, 0, 0, 0, 0},	// lvl 17
	{6, 5, 4, 3, 2, 1, 0, 0, 0, 0},	// lvl 18
	{6, 5, 4, 4, 3, 1, 0, 0, 0, 0},	// lvl 19
	{6, 5, 4, 4, 3, 2, 0, 0, 0, 0},	// lvl 20
	{6, 5, 5, 4, 3, 2, 0, 0, 0, 0},	// lvl 21
	{6, 6, 5, 4, 3, 2, 1, 0, 0, 0},	// lvl 22
	{6, 6, 5, 4, 4, 3, 1, 0, 0, 0},	// lvl 23
	{7, 6, 5, 5, 4, 3, 2, 0, 0, 0},	// lvl 24
	{7, 6, 6, 5, 4, 3, 2, 0, 0, 0},	// lvl 25
	{7, 7, 6, 5, 5, 3, 2, 1, 0, 0},	// lvl 26
	{7, 7, 6, 5, 5, 4, 3, 1, 0, 0},	// lvl 27
	{7, 7, 6, 6, 5, 4, 3, 2, 0, 0},	// lvl 28
	{8, 7, 6, 6, 5, 4, 3, 2, 1, 0},	// lvl 29
	{8, 8, 7, 6, 6, 5, 4, 2, 1, 0},	// lvl 30
};

const int NECROMANCER_SLOTS[][MAX_SLOT] = { {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 1
	{2, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 2
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 3
	{3, 1, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 4
	{3, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 5
	{4, 2, 0, 0, 0, 0, 0, 0, 0, 0},	// lvl 6
	{4, 2, 1, 0, 0, 0, 0, 0, 0, 0},	// lvl 7
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// lvl 8
	{4, 3, 2, 0, 0, 0, 0, 0, 0, 0},	// lvl 9
	{4, 3, 3, 1, 0, 0, 0, 0, 0, 0},	// lvl 10
	{5, 3, 3, 1, 0, 0, 0, 0, 0, 0},	// lvl 11
	{5, 4, 3, 2, 0, 0, 0, 0, 0, 0},	// lvl 12
	{5, 4, 4, 2, 0, 0, 0, 0, 0, 0},	// lvl 13
	{5, 4, 4, 2, 1, 0, 0, 0, 0, 0},	// lvl 14
	{5, 4, 4, 3, 1, 0, 0, 0, 0, 0},	// lvl 15
	{5, 4, 4, 3, 2, 0, 0, 0, 0, 0},	// lvl 16
	{5, 5, 4, 3, 2, 0, 0, 0, 0, 0},	// lvl 17
	{6, 5, 5, 3, 2, 1, 0, 0, 0, 0},	// lvl 18
	{6, 5, 5, 4, 3, 1, 0, 0, 0, 0},	// lvl 19
	{6, 5, 5, 4, 3, 2, 0, 0, 0, 0},	// lvl 20
	{6, 5, 5, 4, 3, 2, 0, 0, 0, 0},	// lvl 21
	{6, 6, 5, 4, 3, 2, 1, 0, 0, 0},	// lvl 22
	{6, 6, 6, 4, 4, 3, 1, 0, 0, 0},	// lvl 23
	{7, 6, 6, 5, 4, 3, 2, 0, 0, 0},	// lvl 24
	{7, 6, 6, 5, 4, 3, 2, 0, 0, 0},	// lvl 25
	{7, 7, 6, 5, 5, 3, 2, 1, 0, 0},	// lvl 26
	{7, 7, 6, 5, 5, 4, 3, 1, 0, 0},	// lvl 27
	{7, 7, 7, 6, 5, 4, 3, 2, 0, 0},	// lvl 28
	{8, 7, 7, 6, 5, 4, 3, 2, 1, 0},	// lvl 29
	{8, 8, 7, 6, 6, 5, 4, 2, 1, 0},	// lvl 30
};

#define MIN_CL_LEVEL 1
#define MAX_CL_LEVEL 30
#define MAX_CL_SLOT  8
#define MIN_CL_WIS   10
#define MAX_CL_WIS   35
#define CL_WIS_DIV   1

const int CLERIC_SLOTS[][MAX_CL_SLOT] = { {1, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 10
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 10
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 10
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 10
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 10
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 10
	{2, 1, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 10
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 8 wisdom 10
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 9 wisdom 10
	{3, 2, 1, 1, 0, 0, 0, 0},	// level 10 wisdom 10
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 11 wisdom 10
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 12 wisdom 10
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 13 wisdom 10
	{3, 3, 2, 2, 1, 0, 0, 0},	// level 14 wisdom 10
	{3, 3, 2, 2, 1, 0, 0, 0},	// level 15 wisdom 10
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 16 wisdom 10
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 17 wisdom 10
	{4, 3, 2, 2, 1, 1, 0, 0},	// level 18 wisdom 10
	{4, 3, 3, 2, 2, 1, 0, 0},	// level 19 wisdom 10
	{4, 3, 3, 2, 2, 1, 0, 0},	// level 20 wisdom 10
	{4, 4, 3, 3, 2, 1, 0, 0},	// level 21 wisdom 10
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 22 wisdom 10
	{5, 4, 3, 3, 2, 1, 1, 0},	// level 23 wisdom 10
	{5, 4, 3, 3, 2, 1, 1, 0},	// level 24 wisdom 10
	{5, 4, 3, 3, 2, 2, 1, 0},	// level 25 wisdom 10
	{5, 4, 3, 3, 2, 2, 1, 0},	// level 26 wisdom 10
	{5, 5, 4, 4, 3, 2, 1, 0},	// level 27 wisdom 10
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 28 wisdom 10
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 29 wisdom 10
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 30 wisdom 10
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 11
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 11
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 11
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 11
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 11
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 11
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 11
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 8 wisdom 11
	{3, 2, 1, 0, 0, 0, 0, 0},	// level 9 wisdom 11
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 11
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 11 wisdom 11
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 12 wisdom 11
	{3, 3, 2, 2, 0, 0, 0, 0},	// level 13 wisdom 11
	{3, 3, 2, 2, 1, 0, 0, 0},	// level 14 wisdom 11
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 15 wisdom 11
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 16 wisdom 11
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 17 wisdom 11
	{4, 3, 3, 2, 2, 1, 0, 0},	// level 18 wisdom 11
	{4, 4, 3, 3, 2, 1, 0, 0},	// level 19 wisdom 11
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 20 wisdom 11
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 21 wisdom 11
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 22 wisdom 11
	{5, 4, 3, 3, 2, 2, 1, 0},	// level 23 wisdom 11
	{5, 4, 4, 3, 2, 2, 1, 0},	// level 24 wisdom 11
	{5, 5, 4, 4, 3, 2, 1, 0},	// level 25 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 26 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 27 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 28 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 29 wisdom 11
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 30 wisdom 11
	{1, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 12
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 12
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 12
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 12
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 12
	{2, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 12
	{2, 2, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 12
	{3, 2, 1, 0, 0, 0, 0, 0},	// level 8 wisdom 12
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 12
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 12
	{3, 2, 2, 1, 0, 0, 0, 0},	// level 11 wisdom 12
	{3, 3, 2, 2, 0, 0, 0, 0},	// level 12 wisdom 12
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 13 wisdom 12
	{4, 3, 2, 2, 1, 0, 0, 0},	// level 14 wisdom 12
	{4, 3, 3, 2, 1, 0, 0, 0},	// level 15 wisdom 12
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 16 wisdom 12
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 17 wisdom 12
	{4, 4, 3, 3, 2, 1, 0, 0},	// level 18 wisdom 12
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 19 wisdom 12
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 20 wisdom 12
	{5, 4, 3, 3, 2, 2, 0, 0},	// level 21 wisdom 12
	{5, 4, 4, 3, 2, 2, 0, 0},	// level 22 wisdom 12
	{5, 4, 4, 3, 3, 2, 1, 0},	// level 23 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 24 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 25 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 26 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 27 wisdom 12
	{6, 5, 4, 4, 3, 2, 1, 1},	// level 28 wisdom 12
	{6, 5, 5, 4, 3, 2, 1, 1},	// level 29 wisdom 12
	{7, 6, 5, 5, 4, 3, 1, 1},	// level 30 wisdom 12
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 13
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 13
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 13
	{2, 1, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 13
	{2, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 13
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 13
	{3, 2, 1, 0, 0, 0, 0, 0},	// level 7 wisdom 13
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 13
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 13
	{3, 3, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 13
	{3, 3, 2, 2, 0, 0, 0, 0},	// level 11 wisdom 13
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 12 wisdom 13
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 13 wisdom 13
	{4, 3, 3, 2, 1, 0, 0, 0},	// level 14 wisdom 13
	{4, 3, 3, 2, 2, 0, 0, 0},	// level 15 wisdom 13
	{4, 4, 3, 2, 2, 0, 0, 0},	// level 16 wisdom 13
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 17 wisdom 13
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 18 wisdom 13
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 19 wisdom 13
	{5, 4, 4, 3, 2, 2, 0, 0},	// level 20 wisdom 13
	{5, 4, 4, 3, 2, 2, 0, 0},	// level 21 wisdom 13
	{5, 5, 4, 4, 3, 2, 0, 0},	// level 22 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 23 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 24 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 25 wisdom 13
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 26 wisdom 13
	{6, 5, 5, 4, 3, 2, 2, 0},	// level 27 wisdom 13
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 28 wisdom 13
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 29 wisdom 13
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 30 wisdom 13
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 14
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 14
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 14
	{2, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 14
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 14
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 14
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 14
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 14
	{3, 3, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 14
	{4, 3, 2, 1, 0, 0, 0, 0},	// level 10 wisdom 14
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 11 wisdom 14
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 12 wisdom 14
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 14
	{4, 3, 3, 2, 1, 0, 0, 0},	// level 14 wisdom 14
	{4, 4, 3, 2, 2, 0, 0, 0},	// level 15 wisdom 14
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 16 wisdom 14
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 17 wisdom 14
	{5, 4, 3, 3, 2, 1, 0, 0},	// level 18 wisdom 14
	{5, 4, 4, 3, 2, 1, 0, 0},	// level 19 wisdom 14
	{5, 4, 4, 3, 3, 2, 0, 0},	// level 20 wisdom 14
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 21 wisdom 14
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 22 wisdom 14
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 23 wisdom 14
	{6, 5, 4, 4, 3, 2, 1, 0},	// level 24 wisdom 14
	{6, 5, 5, 4, 3, 2, 2, 0},	// level 25 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 26 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 27 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 28 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 29 wisdom 14
	{7, 6, 5, 5, 4, 3, 2, 2},	// level 30 wisdom 14
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 15
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 15
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 15
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 15
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 15
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 15
	{3, 2, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 15
	{3, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 15
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 15
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 10 wisdom 15
	{4, 3, 2, 2, 0, 0, 0, 0},	// level 11 wisdom 15
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 15
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 15
	{5, 4, 3, 2, 2, 0, 0, 0},	// level 14 wisdom 15
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 15 wisdom 15
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 16 wisdom 15
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 17 wisdom 15
	{5, 4, 4, 3, 2, 1, 0, 0},	// level 18 wisdom 15
	{5, 5, 4, 3, 3, 2, 0, 0},	// level 19 wisdom 15
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 20 wisdom 15
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 21 wisdom 15
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 22 wisdom 15
	{6, 5, 5, 4, 3, 2, 1, 0},	// level 23 wisdom 15
	{6, 5, 5, 4, 3, 2, 2, 0},	// level 24 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 25 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 26 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 27 wisdom 15
	{7, 6, 5, 5, 4, 3, 2, 1},	// level 28 wisdom 15
	{7, 6, 6, 5, 4, 3, 2, 1},	// level 29 wisdom 15
	{8, 7, 6, 6, 5, 3, 2, 2},	// level 30 wisdom 15
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 16
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 16
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 16
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 16
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 16
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 16
	{3, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 16
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 16
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 9 wisdom 16
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 16
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 16
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 16
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 16
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 14 wisdom 16
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 15 wisdom 16
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 16 wisdom 16
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 17 wisdom 16
	{6, 5, 4, 3, 3, 1, 0, 0},	// level 18 wisdom 16
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 19 wisdom 16
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 20 wisdom 16
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 21 wisdom 16
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 22 wisdom 16
	{7, 6, 5, 4, 4, 2, 1, 0},	// level 23 wisdom 16
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 24 wisdom 16
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 25 wisdom 16
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 26 wisdom 16
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 27 wisdom 16
	{8, 7, 6, 5, 5, 3, 2, 1},	// level 28 wisdom 16
	{8, 7, 6, 6, 5, 3, 2, 2},	// level 29 wisdom 16
	{8, 7, 6, 6, 5, 4, 2, 2},	// level 30 wisdom 16
	{2, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 17
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 17
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 17
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 17
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 17
	{3, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 17
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 17
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 17
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 17
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 17
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 17
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 17
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 13 wisdom 17
	{5, 4, 3, 3, 2, 0, 0, 0},	// level 14 wisdom 17
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 17
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 16 wisdom 17
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 17 wisdom 17
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 18 wisdom 17
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 19 wisdom 17
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 20 wisdom 17
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 21 wisdom 17
	{7, 6, 5, 4, 4, 2, 0, 0},	// level 22 wisdom 17
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 23 wisdom 17
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 24 wisdom 17
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 25 wisdom 17
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 26 wisdom 17
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 27 wisdom 17
	{8, 7, 6, 6, 5, 4, 2, 1},	// level 28 wisdom 17
	{8, 7, 6, 6, 5, 4, 3, 2},	// level 29 wisdom 17
	{8, 7, 7, 6, 5, 4, 3, 2},	// level 30 wisdom 17
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 18
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 18
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 18
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 18
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 18
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 18
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 18
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 8 wisdom 18
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 18
	{4, 3, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 18
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 18
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 12 wisdom 18
	{5, 4, 3, 3, 0, 0, 0, 0},	// level 13 wisdom 18
	{5, 4, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 18
	{5, 5, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 18
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 16 wisdom 18
	{6, 5, 4, 4, 3, 0, 0, 0},	// level 17 wisdom 18
	{6, 5, 4, 4, 3, 2, 0, 0},	// level 18 wisdom 18
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 19 wisdom 18
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 20 wisdom 18
	{7, 6, 5, 4, 4, 2, 0, 0},	// level 21 wisdom 18
	{7, 6, 5, 5, 4, 3, 0, 0},	// level 22 wisdom 18
	{7, 6, 5, 5, 4, 3, 2, 0},	// level 23 wisdom 18
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 24 wisdom 18
	{8, 7, 6, 5, 4, 3, 2, 0},	// level 25 wisdom 18
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 26 wisdom 18
	{8, 7, 6, 6, 5, 4, 2, 0},	// level 27 wisdom 18
	{8, 7, 6, 6, 5, 4, 3, 1},	// level 28 wisdom 18
	{8, 7, 7, 6, 5, 4, 3, 2},	// level 29 wisdom 18
	{9, 8, 7, 7, 6, 4, 3, 2},	// level 30 wisdom 18
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 19
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 19
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 19
	{3, 2, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 19
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 19
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 19
	{4, 3, 2, 0, 0, 0, 0, 0},	// level 7 wisdom 19
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 19
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 19
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 19
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 19
	{5, 4, 3, 3, 0, 0, 0, 0},	// level 12 wisdom 19
	{5, 4, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 19
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 19
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 19
	{6, 5, 4, 4, 3, 0, 0, 0},	// level 16 wisdom 19
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 19
	{6, 5, 5, 4, 3, 2, 0, 0},	// level 18 wisdom 19
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 19 wisdom 19
	{7, 6, 5, 4, 4, 2, 0, 0},	// level 20 wisdom 19
	{7, 6, 5, 5, 4, 2, 0, 0},	// level 21 wisdom 19
	{7, 6, 6, 5, 4, 3, 0, 0},	// level 22 wisdom 19
	{7, 6, 6, 5, 4, 3, 2, 0},	// level 23 wisdom 19
	{8, 7, 6, 5, 4, 3, 2, 0},	// level 24 wisdom 19
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 25 wisdom 19
	{8, 7, 6, 6, 5, 4, 2, 0},	// level 26 wisdom 19
	{8, 7, 7, 6, 5, 4, 3, 0},	// level 27 wisdom 19
	{8, 7, 7, 6, 5, 4, 3, 1},	// level 28 wisdom 19
	{9, 8, 7, 7, 6, 4, 3, 2},	// level 29 wisdom 19
	{9, 8, 7, 7, 6, 5, 3, 2},	// level 30 wisdom 19
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 20
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 20
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 20
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 20
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 20
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 20
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 20
	{4, 3, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 20
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 20
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 20
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 11 wisdom 20
	{5, 4, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 20
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 20
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 20
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 15 wisdom 20
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 20
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 20
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 18 wisdom 20
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 19 wisdom 20
	{7, 6, 5, 5, 4, 2, 0, 0},	// level 20 wisdom 20
	{7, 6, 6, 5, 4, 3, 0, 0},	// level 21 wisdom 20
	{7, 6, 6, 5, 4, 3, 0, 0},	// level 22 wisdom 20
	{8, 7, 6, 5, 4, 3, 2, 0},	// level 23 wisdom 20
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 24 wisdom 20
	{8, 7, 7, 6, 5, 4, 2, 0},	// level 25 wisdom 20
	{8, 7, 7, 6, 5, 4, 2, 0},	// level 26 wisdom 20
	{9, 8, 7, 6, 5, 4, 3, 0},	// level 27 wisdom 20
	{9, 8, 7, 7, 6, 4, 3, 1},	// level 28 wisdom 20
	{9, 8, 7, 7, 6, 5, 3, 2},	// level 29 wisdom 20
	{9, 8, 8, 7, 6, 5, 3, 3},	// level 30 wisdom 20
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 21
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 21
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 21
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 21
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 21
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 21
	{5, 3, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 21
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 21
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 21
	{5, 4, 3, 2, 0, 0, 0, 0},	// level 10 wisdom 21
	{5, 4, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 21
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 21
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 21
	{6, 5, 4, 3, 2, 0, 0, 0},	// level 14 wisdom 21
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 21
	{6, 5, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 21
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 21
	{7, 6, 5, 4, 3, 2, 0, 0},	// level 18 wisdom 21
	{7, 6, 5, 5, 4, 2, 0, 0},	// level 19 wisdom 21
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 20 wisdom 21
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 21 wisdom 21
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 22 wisdom 21
	{8, 7, 6, 6, 5, 3, 2, 0},	// level 23 wisdom 21
	{8, 7, 7, 6, 5, 3, 2, 0},	// level 24 wisdom 21
	{8, 7, 7, 6, 5, 4, 2, 0},	// level 25 wisdom 21
	{9, 8, 7, 6, 5, 4, 3, 0},	// level 26 wisdom 21
	{9, 8, 7, 7, 6, 4, 3, 0},	// level 27 wisdom 21
	{9, 8, 8, 7, 6, 5, 3, 1},	// level 28 wisdom 21
	{9, 8, 8, 7, 6, 5, 3, 2},	// level 29 wisdom 21
	{10, 9, 8, 8, 7, 5, 4, 3},	// level 30 wisdom 21
	{3, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 22
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 22
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 22
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 22
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 22
	{5, 3, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 22
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 22
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 22
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 9 wisdom 22
	{5, 4, 4, 2, 0, 0, 0, 0},	// level 10 wisdom 22
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 22
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 22
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 13 wisdom 22
	{6, 5, 5, 4, 2, 0, 0, 0},	// level 14 wisdom 22
	{7, 5, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 22
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 22
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 17 wisdom 22
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 22
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 19 wisdom 22
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 20 wisdom 22
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 21 wisdom 22
	{8, 7, 6, 6, 5, 3, 0, 0},	// level 22 wisdom 22
	{8, 7, 7, 6, 5, 3, 2, 0},	// level 23 wisdom 22
	{9, 7, 7, 6, 5, 4, 2, 0},	// level 24 wisdom 22
	{9, 8, 7, 6, 5, 4, 3, 0},	// level 25 wisdom 22
	{9, 8, 7, 7, 6, 4, 3, 0},	// level 26 wisdom 22
	{9, 8, 8, 7, 6, 5, 3, 0},	// level 27 wisdom 22
	{9, 8, 8, 7, 6, 5, 3, 1},	// level 28 wisdom 22
	{10, 9, 8, 8, 7, 5, 4, 2},	// level 29 wisdom 22
	{10, 9, 8, 8, 7, 5, 4, 3},	// level 30 wisdom 22
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 23
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 23
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 23
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 23
	{5, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 23
	{5, 4, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 23
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 23
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 23
	{5, 4, 4, 0, 0, 0, 0, 0},	// level 9 wisdom 23
	{6, 4, 4, 3, 0, 0, 0, 0},	// level 10 wisdom 23
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 23
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 12 wisdom 23
	{6, 5, 5, 3, 0, 0, 0, 0},	// level 13 wisdom 23
	{7, 5, 5, 4, 3, 0, 0, 0},	// level 14 wisdom 23
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 23
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 16 wisdom 23
	{7, 6, 6, 5, 3, 0, 0, 0},	// level 17 wisdom 23
	{7, 6, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 23
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 19 wisdom 23
	{8, 7, 6, 5, 4, 3, 0, 0},	// level 20 wisdom 23
	{8, 7, 6, 6, 5, 3, 0, 0},	// level 21 wisdom 23
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 22 wisdom 23
	{9, 8, 7, 6, 5, 4, 2, 0},	// level 23 wisdom 23
	{9, 8, 7, 6, 5, 4, 2, 0},	// level 24 wisdom 23
	{9, 8, 7, 7, 6, 4, 3, 0},	// level 25 wisdom 23
	{9, 8, 8, 7, 6, 4, 3, 0},	// level 26 wisdom 23
	{9, 8, 8, 7, 6, 5, 3, 0},	// level 27 wisdom 23
	{10, 9, 8, 8, 7, 5, 4, 2},	// level 28 wisdom 23
	{10, 9, 8, 8, 7, 5, 4, 2},	// level 29 wisdom 23
	{10, 9, 9, 8, 7, 6, 4, 3},	// level 30 wisdom 23
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 24
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 2 wisdom 24
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 3 wisdom 24
	{4, 3, 0, 0, 0, 0, 0, 0},	// level 4 wisdom 24
	{5, 3, 0, 0, 0, 0, 0, 0},	// level 5 wisdom 24
	{5, 4, 0, 0, 0, 0, 0, 0},	// level 6 wisdom 24
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 7 wisdom 24
	{5, 4, 3, 0, 0, 0, 0, 0},	// level 8 wisdom 24
	{6, 4, 4, 0, 0, 0, 0, 0},	// level 9 wisdom 24
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 10 wisdom 24
	{6, 5, 4, 3, 0, 0, 0, 0},	// level 11 wisdom 24
	{6, 5, 5, 3, 0, 0, 0, 0},	// level 12 wisdom 24
	{7, 5, 5, 4, 0, 0, 0, 0},	// level 13 wisdom 24
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 14 wisdom 24
	{7, 6, 5, 4, 3, 0, 0, 0},	// level 15 wisdom 24
	{7, 6, 6, 4, 3, 0, 0, 0},	// level 16 wisdom 24
	{7, 6, 6, 5, 4, 0, 0, 0},	// level 17 wisdom 24
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 24
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 19 wisdom 24
	{8, 7, 7, 6, 4, 3, 0, 0},	// level 20 wisdom 24
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 21 wisdom 24
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 22 wisdom 24
	{9, 8, 7, 6, 5, 4, 2, 0},	// level 23 wisdom 24
	{9, 8, 8, 7, 6, 4, 2, 0},	// level 24 wisdom 24
	{9, 8, 8, 7, 6, 4, 3, 0},	// level 25 wisdom 24
	{10, 9, 8, 7, 6, 5, 3, 0},	// level 26 wisdom 24
	{10, 9, 8, 8, 7, 5, 3, 0},	// level 27 wisdom 24
	{10, 9, 9, 8, 7, 5, 4, 2},	// level 28 wisdom 24
	{10, 9, 9, 8, 7, 6, 4, 2},	// level 29 wisdom 24
	{10, 9, 9, 8, 7, 6, 4, 3},	// level 30 wisdom 24
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 25
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 25
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 25
	{5, 3, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 25
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 25
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 25
	{5, 4, 3, 0, 0, 0, 0, 0},	//level 7 wisdom 25
	{6, 4, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 25
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 25
	{6, 5, 4, 3, 0, 0, 0, 0},	//level 10 wisdom 25
	{6, 5, 4, 3, 0, 0, 0, 0},	//level 11 wisdom 25
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 12 wisdom 25
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 13 wisdom 25
	{7, 6, 5, 4, 3, 0, 0, 0},	//level 14 wisdom 25
	{7, 6, 5, 4, 3, 0, 0, 0},	//level 15 wisdom 25
	{8, 6, 6, 5, 3, 0, 0, 0},	// level 16 wisdom 25
	{8, 7, 6, 5, 4, 0, 0, 0},	// level 17 wisdom 25
	{8, 7, 6, 5, 4, 2, 0, 0},	// level 18 wisdom 25
	{8, 7, 7, 5, 4, 3, 0, 0},	// level 19 wisdom 25
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 20 wisdom 25
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 21 wisdom 25
	{9, 8, 7, 6, 5, 4, 0, 0},	// level 22 wisdom 25
	{9, 8, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 25
	{9, 8, 8, 7, 6, 4, 3, 0},	// level 24 wisdom 25
	{10, 9, 8, 7, 6, 5, 3, 0},	// level 25 wisdom 25
	{10, 9, 8, 8, 7, 5, 3, 0},	// level 26 wisdom 25
	{10, 9, 9, 8, 7, 5, 4, 0},	// level 27 wisdom 25
	{10, 9, 9, 8, 7, 6, 4, 2},	// level 28 wisdom 25
	{11, 10, 9, 8, 7, 6, 4, 2},	// level 29 wisdom 25
	{11, 10, 9, 9, 8, 6, 5, 3},	// level 30 wisdom 25
	{4, 0, 0, 0, 0, 0, 0, 0},	// level 1 wisdom 26
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 26
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 26
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 26
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 26
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 26
	{6, 4, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 26
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 26
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 26
	{6, 5, 4, 3, 0, 0, 0, 0},	//level 10 wisdom 26
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 11 wisdom 26
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 12 wisdom 26
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 13 wisdom 26
	{7, 6, 5, 4, 3, 0, 0, 0},	//level 14 wisdom 26
	{8, 6, 6, 4, 3, 0, 0, 0},	// level 15 wisdom 26
	{8, 7, 6, 5, 4, 0, 0, 0},	// level 16 wisdom 26
	{8, 7, 6, 5, 4, 0, 0, 0},	// level 17 wisdom 26
	{8, 7, 7, 5, 4, 2, 0, 0},	// level 18 wisdom 26
	{8, 7, 7, 6, 5, 3, 0, 0},	// level 19 wisdom 26
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 20 wisdom 26
	{9, 8, 7, 6, 5, 3, 0, 0},	// level 21 wisdom 26
	{9, 8, 8, 7, 6, 4, 0, 0},	// level 22 wisdom 26
	{9, 8, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 26
	{10, 9, 8, 7, 6, 4, 3, 0},	// level 24 wisdom 26
	{10, 9, 8, 8, 6, 5, 3, 0},	// level 25 wisdom 26
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 26 wisdom 26
	{10, 9, 9, 8, 7, 5, 4, 0},	// level 27 wisdom 26
	{11, 10, 9, 8, 7, 6, 4, 2},	// level 28 wisdom 26
	{11, 10, 9, 9, 8, 6, 4, 3},	// level 29 wisdom 26
	{11, 10, 10, 9, 8, 6, 5, 4},	// level 30 wisdom 26
	{4, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 27
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 27
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 27
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 27
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 27
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 27
	{6, 4, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 27
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 27
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 27
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 27
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 11 wisdom 27
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 12 wisdom 27
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 13 wisdom 27
	{8, 6, 6, 4, 3, 0, 0, 0},	//level 14 wisdom 27
	{8, 7, 6, 5, 3, 0, 0, 0},	//level 15 wisdom 27
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 16 wisdom 27
	{8, 7, 7, 5, 4, 0, 0, 0},	//level 17 wisdom 27
	{9, 7, 7, 6, 4, 2, 0, 0},	//level 18 wisdom 27
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 19 wisdom 27
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 20 wisdom 27
	{9, 8, 8, 7, 5, 3, 0, 0},	//level 21 wisdom 27
	{9, 8, 8, 7, 6, 4, 0, 0},	//level 22 wisdom 27
	{10, 9, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 27
	{10, 9, 8, 8, 6, 5, 3, 0},	// level 24 wisdom 27
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 25 wisdom 27
	{10, 9, 9, 8, 7, 5, 4, 0},	// level 26 wisdom 27
	{11, 10, 9, 8, 7, 6, 4, 0},	// level 27 wisdom 27
	{11, 10, 10, 9, 8, 6, 4, 2},	// level 28 wisdom 27
	{11, 10, 10, 9, 8, 6, 5, 3},	// level 29 wisdom 27
	{11, 10, 10, 9, 8, 7, 5, 4},	// level 30 wisdom 27
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 28
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 28
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 28
	{5, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 28
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 28
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 28
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 28
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 28
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 9 wisdom 28
	{7, 5, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 28
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 11 wisdom 28
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 12 wisdom 28
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 13 wisdom 28
	{8, 7, 6, 4, 3, 0, 0, 0},	//level 14 wisdom 28
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 15 wisdom 28
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 16 wisdom 28
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 17 wisdom 28
	{9, 8, 7, 6, 5, 2, 0, 0},	//level 18 wisdom 28
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 19 wisdom 28
	{9, 8, 8, 6, 5, 3, 0, 0},	//level 20 wisdom 28
	{10, 8, 8, 7, 6, 4, 0, 0},	// level 21 wisdom 28
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 22 wisdom 28
	{10, 9, 8, 7, 6, 4, 2, 0},	// level 23 wisdom 28
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 24 wisdom 28
	{11, 9, 9, 8, 7, 5, 3, 0},	// level 25 wisdom 28
	{11, 10, 9, 8, 7, 6, 4, 0},	// level 26 wisdom 28
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 27 wisdom 28
	{11, 10, 10, 9, 8, 6, 4, 2},	// level 28 wisdom 28
	{12, 10, 10, 9, 8, 7, 5, 3},	// level 29 wisdom 28
	{12, 11, 10, 10, 9, 7, 5, 4},	// level 30 wisdom 28
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 29
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 29
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 29
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 29
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 29
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 29
	{6, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 29
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 8 wisdom 29
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 29
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 29
	{7, 6, 5, 4, 0, 0, 0, 0},	//level 11 wisdom 29
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 29
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 13 wisdom 29
	{8, 7, 6, 5, 3, 0, 0, 0},	//level 14 wisdom 29
	{8, 7, 6, 5, 4, 0, 0, 0},	//level 15 wisdom 29
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 16 wisdom 29
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 17 wisdom 29
	{9, 8, 7, 6, 5, 3, 0, 0},	//level 18 wisdom 29
	{9, 8, 8, 6, 5, 3, 0, 0},	//level 19 wisdom 29
	{10, 8, 8, 7, 5, 3, 0, 0},	// level 20 wisdom 29
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 21 wisdom 29
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 22 wisdom 29
	{10, 9, 9, 8, 7, 5, 3, 0},	// level 23 wisdom 29
	{11, 9, 9, 8, 7, 5, 3, 0},	// level 24 wisdom 29
	{11, 10, 9, 8, 7, 5, 3, 0},	// level 25 wisdom 29
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 26 wisdom 29
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 27 wisdom 29
	{12, 11, 10, 9, 8, 7, 5, 2},	// level 28 wisdom 29
	{12, 11, 11, 10, 9, 7, 5, 3},	// level 29 wisdom 29
	{12, 11, 11, 10, 9, 7, 6, 4},	// level 30 wisdom 29
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 30
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 30
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 30
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 30
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 30
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 30
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 30
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 30
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 30
	{7, 6, 5, 3, 0, 0, 0, 0},	//level 10 wisdom 30
	{8, 6, 5, 4, 0, 0, 0, 0},	//level 11 wisdom 30
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 30
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 13 wisdom 30
	{8, 7, 6, 5, 3, 0, 0, 0},	//level 14 wisdom 30
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 15 wisdom 30
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 16 wisdom 30
	{9, 8, 7, 6, 5, 0, 0, 0},	//level 17 wisdom 30
	{9, 8, 8, 6, 5, 3, 0, 0},	//level 18 wisdom 30
	{10, 8, 8, 7, 5, 3, 0, 0},	// level 19 wisdom 30
	{10, 9, 8, 7, 6, 3, 0, 0},	// level 20 wisdom 30
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 21 wisdom 30
	{10, 9, 9, 8, 6, 4, 0, 0},	// level 22 wisdom 30
	{11, 9, 9, 8, 7, 5, 3, 0},	// level 23 wisdom 30
	{11, 10, 9, 8, 7, 5, 3, 0},	// level 24 wisdom 30
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 25 wisdom 30
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 26 wisdom 30
	{12, 11, 10, 9, 8, 6, 4, 0},	// level 27 wisdom 30
	{12, 11, 11, 10, 9, 7, 5, 2},	// level 28 wisdom 30
	{12, 11, 11, 10, 9, 7, 5, 3},	// level 29 wisdom 30
	{12, 11, 11, 10, 9, 8, 6, 4},	// level 30 wisdom 30
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 31
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 31
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 31
	{6, 4, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 31
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 31
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 31
	{7, 5, 4, 0, 0, 0, 0, 0},	//level 7 wisdom 31
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 31
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 31
	{8, 6, 5, 4, 0, 0, 0, 0},	//level 10 wisdom 31
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 31
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 31
	{8, 7, 6, 5, 0, 0, 0, 0},	//level 13 wisdom 31
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 31
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 15 wisdom 31
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 16 wisdom 31
	{9, 8, 7, 6, 5, 0, 0, 0},	//level 17 wisdom 31
	{10, 8, 8, 6, 5, 3, 0, 0},	// level 18 wisdom 31
	{10, 9, 8, 7, 5, 3, 0, 0},	// level 19 wisdom 31
	{10, 9, 8, 7, 6, 4, 0, 0},	// level 20 wisdom 31
	{10, 9, 9, 7, 6, 4, 0, 0},	// level 21 wisdom 31
	{11, 9, 9, 8, 7, 4, 0, 0},	// level 22 wisdom 31
	{11, 10, 9, 8, 7, 5, 3, 0},	// level 23 wisdom 31
	{11, 10, 10, 9, 7, 5, 3, 0},	// level 24 wisdom 31
	{11, 10, 10, 9, 8, 6, 4, 0},	// level 25 wisdom 31
	{12, 11, 10, 9, 8, 6, 4, 0},	// level 26 wisdom 31
	{12, 11, 11, 10, 9, 7, 5, 0},	// level 27 wisdom 31
	{12, 11, 11, 10, 9, 7, 5, 2},	// level 28 wisdom 31
	{12, 11, 11, 10, 9, 7, 6, 3},	// level 29 wisdom 31
	{13, 12, 12, 11, 10, 8, 6, 4},	// level 30 wisdom 31
	{5, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 32
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 32
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 32
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 32
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 32
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 32
	{7, 5, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 32
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 32
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 32
	{8, 6, 5, 4, 0, 0, 0, 0},	//level 10 wisdom 32
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 32
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 12 wisdom 32
	{9, 7, 6, 5, 0, 0, 0, 0},	//level 13 wisdom 32
	{9, 7, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 32
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 15 wisdom 32
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 16 wisdom 32
	{10, 8, 8, 6, 5, 0, 0, 0},	// level 17 wisdom 32
	{10, 9, 8, 7, 5, 3, 0, 0},	// level 18 wisdom 32
	{10, 9, 8, 7, 6, 3, 0, 0},	// level 19 wisdom 32
	{10, 9, 9, 7, 6, 4, 0, 0},	// level 20 wisdom 32
	{11, 9, 9, 8, 6, 4, 0, 0},	// level 21 wisdom 32
	{11, 10, 9, 8, 7, 5, 0, 0},	// level 22 wisdom 32
	{11, 10, 10, 8, 7, 5, 3, 0},	// level 23 wisdom 32
	{11, 10, 10, 9, 8, 5, 3, 0},	// level 24 wisdom 32
	{12, 11, 10, 9, 8, 6, 4, 0},	// level 25 wisdom 32
	{12, 11, 11, 10, 8, 6, 4, 0},	// level 26 wisdom 32
	{12, 11, 11, 10, 9, 7, 5, 0},	// level 27 wisdom 32
	{13, 11, 11, 10, 9, 7, 5, 2},	// level 28 wisdom 32
	{13, 12, 12, 11, 10, 8, 6, 3},	// level 29 wisdom 32
	{13, 12, 12, 11, 10, 8, 6, 5},	// level 30 wisdom 32
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 33
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 33
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 33
	{6, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 33
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 33
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 33
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 33
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 33
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 9 wisdom 33
	{8, 6, 6, 4, 0, 0, 0, 0},	//level 10 wisdom 33
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 33
	{9, 7, 6, 5, 0, 0, 0, 0},	//level 12 wisdom 33
	{9, 7, 7, 5, 0, 0, 0, 0},	//level 13 wisdom 33
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 33
	{9, 8, 7, 6, 4, 0, 0, 0},	//level 15 wisdom 33
	{10, 8, 8, 6, 5, 0, 0, 0},	// level 16 wisdom 33
	{10, 9, 8, 6, 5, 0, 0, 0},	// level 17 wisdom 33
	{10, 9, 8, 7, 5, 3, 0, 0},	// level 18 wisdom 33
	{10, 9, 9, 7, 6, 3, 0, 0},	// level 19 wisdom 33
	{11, 9, 9, 8, 6, 4, 0, 0},	// level 20 wisdom 33
	{11, 10, 9, 8, 7, 4, 0, 0},	// level 21 wisdom 33
	{11, 10, 10, 8, 7, 5, 0, 0},	// level 22 wisdom 33
	{11, 10, 10, 9, 7, 5, 3, 0},	// level 23 wisdom 33
	{12, 11, 10, 9, 8, 6, 3, 0},	// level 24 wisdom 33
	{12, 11, 11, 9, 8, 6, 4, 0},	// level 25 wisdom 33
	{12, 11, 11, 10, 9, 7, 4, 0},	// level 26 wisdom 33
	{13, 11, 11, 10, 9, 7, 5, 0},	// level 27 wisdom 33
	{13, 12, 12, 11, 10, 8, 5, 2},	// level 28 wisdom 33
	{13, 12, 12, 11, 10, 8, 6, 3},	// level 29 wisdom 33
	{13, 12, 12, 11, 10, 8, 7, 5},	// level 30 wisdom 33
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 34
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 34
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 34
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 34
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 34
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 34
	{7, 6, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 34
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 34
	{8, 6, 6, 0, 0, 0, 0, 0},	//level 9 wisdom 34
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 10 wisdom 34
	{9, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 34
	{9, 7, 7, 5, 0, 0, 0, 0},	//level 12 wisdom 34
	{9, 8, 7, 5, 0, 0, 0, 0},	//level 13 wisdom 34
	{9, 8, 7, 5, 4, 0, 0, 0},	//level 14 wisdom 34
	{10, 8, 8, 6, 4, 0, 0, 0},	// level 15 wisdom 34
	{10, 8, 8, 6, 5, 0, 0, 0},	// level 16 wisdom 34
	{10, 9, 8, 7, 5, 0, 0, 0},	// level 17 wisdom 34
	{10, 9, 9, 7, 6, 3, 0, 0},	// level 18 wisdom 34
	{11, 9, 9, 7, 6, 3, 0, 0},	// level 19 wisdom 34
	{11, 10, 9, 8, 6, 4, 0, 0},	// level 20 wisdom 34
	{11, 10, 10, 8, 7, 4, 0, 0},	// level 21 wisdom 34
	{12, 10, 10, 9, 7, 5, 0, 0},	// level 22 wisdom 34
	{12, 11, 10, 9, 8, 5, 3, 0},	// level 23 wisdom 34
	{12, 11, 11, 9, 8, 6, 3, 0},	// level 24 wisdom 34
	{12, 11, 11, 10, 9, 6, 4, 0},	// level 25 wisdom 34
	{13, 11, 11, 10, 9, 7, 5, 0},	// level 26 wisdom 34
	{13, 12, 12, 11, 9, 7, 5, 0},	// level 27 wisdom 34
	{13, 12, 12, 11, 10, 8, 6, 2},	// level 28 wisdom 34
	{13, 12, 12, 11, 10, 8, 6, 3},	// level 29 wisdom 34
	{14, 13, 13, 12, 11, 9, 7, 5},	// level 30 wisdom 34
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 1 wisdom 35
	{6, 0, 0, 0, 0, 0, 0, 0},	//level 2 wisdom 35
	{7, 0, 0, 0, 0, 0, 0, 0},	//level 3 wisdom 35
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 4 wisdom 35
	{7, 5, 0, 0, 0, 0, 0, 0},	//level 5 wisdom 35
	{7, 6, 0, 0, 0, 0, 0, 0},	//level 6 wisdom 35
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 7 wisdom 35
	{8, 6, 5, 0, 0, 0, 0, 0},	//level 8 wisdom 35
	{8, 7, 6, 0, 0, 0, 0, 0},	//level 9 wisdom 35
	{8, 7, 6, 4, 0, 0, 0, 0},	//level 10 wisdom 35
	{9, 7, 6, 4, 0, 0, 0, 0},	//level 11 wisdom 35
	{9, 7, 7, 5, 0, 0, 0, 0},	//level 12 wisdom 35
	{9, 8, 7, 5, 0, 0, 0, 0},	//level 13 wisdom 35
	{10, 8, 7, 6, 4, 0, 0, 0},	// level 14 wisdom 35
	{10, 8, 8, 6, 4, 0, 0, 0},	// level 15 wisdom 35
	{10, 9, 8, 6, 5, 0, 0, 0},	// level 16 wisdom 35
	{10, 9, 8, 7, 5, 0, 0, 0},	// level 17 wisdom 35
	{11, 9, 9, 7, 6, 3, 0, 0},	// level 18 wisdom 35
	{11, 10, 9, 8, 6, 3, 0, 0},	//level 19 wisdom 35
	{11, 10, 10, 8, 7, 4, 0, 0},	// level 20 wisdom 35
	{12, 10, 10, 8, 7, 4, 0, 0},	// level 21 wisdom 35
	{12, 11, 10, 9, 7, 5, 0, 0},	// level 22 wisdom 35
	{12, 11, 11, 9, 8, 5, 3, 0},	//level 23 wisdom 35
	{12, 11, 11, 10, 8, 6, 4, 0},	// level 24 wisdom 35
	{13, 11, 11, 10, 9, 6, 4, 0},	// level 25 wisdom 35
	{13, 12, 12, 10, 9, 7, 5, 0},	// level 26 wisdom 35
	{13, 12, 12, 11, 10, 7, 5, 0},	// level 27 wisdom 35
	{13, 12, 12, 11, 10, 8, 6, 2},	// level 28 wisdom 35
	{14, 13, 13, 12, 11, 8, 6, 3},	// level 29 wisdom 35
	{14, 13, 13, 12, 11, 9, 7, 5},	// level 30 wisdom 35
};

#define MAX_ME_SLOT 4
const int MERCHANT_SLOTS[][MAX_ME_SLOT] = { {0, 0, 0, 0},	// lvl 1
	{0, 0, 0, 0},			// lvl 2
	{0, 0, 0, 0},			// lvl 3
	{1, 0, 0, 0},			// lvl 4
	{2, 0, 0, 0},			// lvl 5
	{2, 0, 0, 0},			// lvl 6
	{3, 0, 0, 0},			// lvl 7
	{3, 0, 0, 0},			// lvl 8
	{3, 0, 0, 0},			// lvl 9
	{4, 0, 0, 0},			// lvl 10
	{4, 0, 0, 0},			// lvl 11
	{4, 1, 0, 0},			// lvl 12
	{4, 2, 0, 0},			// lvl 13
	{5, 2, 0, 0},			// lvl 14
	{5, 3, 0, 0},			// lvl 15
	{5, 3, 0, 0},			// lvl 16
	{5, 3, 0, 0},			// lvl 17
	{5, 4, 1, 0},			// lvl 18
	{6, 4, 2, 0},			// lvl 19
	{6, 4, 2, 0},			// lvl 20
	{6, 4, 3, 0},			// lvl 21
	{6, 5, 3, 0},			// lvl 22
	{6, 5, 3, 1},			// lvl 23
	{6, 5, 4, 2},			// lvl 24
	{7, 5, 4, 2},			// lvl 25
	{7, 5, 4, 3},			// lvl 26
	{7, 6, 4, 3},			// lvl 27
	{7, 6, 5, 3},			// lvl 28
	{7, 6, 5, 4},			// lvl 29
	{7, 6, 5, 4},			// lvl 30
};

#define MIN_PA_LEVEL 8
#define MAX_PA_LEVEL 30
#define MAX_PA_SLOT  4
#define MIN_PA_WIS   10
#define MAX_PA_WIS   30
#define PA_WIS_DIV   2

const int PALADINE_SLOTS[][MAX_PA_SLOT] = { {1, 0, 0, 0},	// lvl 8 wis 10,11
	{1, 0, 0, 0},			// lvl 9
	{1, 0, 0, 0},			// lvl 10
	{1, 0, 0, 0},			// lvl 11
	{1, 0, 0, 0},			// lvl 12
	{1, 0, 0, 0},			// lvl 13
	{1, 1, 0, 0},			// lvl 14
	{2, 1, 0, 0},			// lvl 15
	{2, 1, 0, 0},			// lvl 16
	{2, 1, 0, 0},			// lvl 17
	{2, 1, 1, 0},			// lvl 18
	{2, 1, 1, 0},			// lvl 19
	{2, 1, 1, 0},			// lvl 20
	{2, 2, 1, 0},			// lvl 21
	{3, 2, 1, 0},			// lvl 22
	{3, 2, 1, 1},			// lvl 23
	{3, 2, 1, 1},			// lvl 24
	{3, 2, 2, 1},			// lvl 25
	{3, 2, 2, 1},			// lvl 26
	{3, 2, 2, 1},			// lvl 27
	{3, 3, 2, 1},			// lvl 28
	{4, 3, 2, 1},			// lvl 29
	{4, 3, 2, 1},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 12,13
	{1, 0, 0, 0},			// lvl 9
	{1, 0, 0, 0},			// lvl 10
	{1, 0, 0, 0},			// lvl 11
	{1, 0, 0, 0},			// lvl 12
	{2, 0, 0, 0},			// lvl 13
	{2, 1, 0, 0},			// lvl 14
	{2, 1, 0, 0},			// lvl 15
	{2, 1, 0, 0},			// lvl 16
	{2, 1, 0, 0},			// lvl 17
	{3, 1, 1, 0},			// lvl 18
	{3, 2, 1, 0},			// lvl 19
	{3, 2, 1, 0},			// lvl 20
	{3, 2, 1, 0},			// lvl 21
	{3, 2, 1, 0},			// lvl 22
	{3, 2, 2, 1},			// lvl 23
	{4, 2, 2, 1},			// lvl 24
	{4, 3, 2, 1},			// lvl 25
	{4, 3, 2, 1},			// lvl 26
	{4, 3, 2, 1},			// lvl 27
	{4, 3, 2, 2},			// lvl 28
	{4, 3, 3, 2},			// lvl 29
	{4, 3, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 14,15
	{1, 0, 0, 0},			// lvl 9
	{1, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{2, 0, 0, 0},			// lvl 13
	{2, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 1, 0, 0},			// lvl 16
	{3, 2, 0, 0},			// lvl 17
	{3, 2, 1, 0},			// lvl 18
	{3, 2, 1, 0},			// lvl 19
	{4, 2, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{4, 3, 2, 0},			// lvl 22
	{4, 3, 2, 1},			// lvl 23
	{4, 3, 2, 1},			// lvl 24
	{4, 3, 3, 1},			// lvl 25
	{4, 3, 3, 1},			// lvl 26
	{4, 4, 3, 2},			// lvl 27
	{4, 4, 3, 2},			// lvl 28
	{4, 4, 3, 2},			// lvl 29
	{4, 4, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 16,17
	{1, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{3, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 1, 0, 0},			// lvl 16
	{4, 2, 0, 0},			// lvl 17
	{4, 2, 1, 0},			// lvl 18
	{4, 2, 1, 0},			// lvl 19
	{4, 2, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{5, 3, 2, 0},			// lvl 22
	{5, 3, 2, 1},			// lvl 23
	{5, 3, 2, 1},			// lvl 24
	{5, 3, 3, 1},			// lvl 25
	{5, 4, 3, 1},			// lvl 26
	{5, 4, 3, 2},			// lvl 27
	{5, 4, 3, 2},			// lvl 28
	{5, 4, 3, 2},			// lvl 29
	{5, 4, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 18,19
	{1, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{3, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 2, 0, 0},			// lvl 16
	{4, 2, 0, 0},			// lvl 17
	{4, 2, 1, 0},			// lvl 18
	{4, 3, 1, 0},			// lvl 19
	{4, 3, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{5, 3, 2, 0},			// lvl 22
	{5, 4, 2, 1},			// lvl 23
	{5, 4, 2, 1},			// lvl 24
	{5, 4, 3, 1},			// lvl 25
	{5, 4, 3, 1},			// lvl 26
	{5, 4, 3, 2},			// lvl 27
	{5, 4, 3, 2},			// lvl 28
	{5, 4, 3, 2},			// lvl 29
	{5, 4, 3, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 20,21
	{1, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{2, 0, 0, 0},			// lvl 11
	{2, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{3, 1, 0, 0},			// lvl 14
	{3, 1, 0, 0},			// lvl 15
	{3, 2, 0, 0},			// lvl 16
	{4, 2, 0, 0},			// lvl 17
	{4, 2, 1, 0},			// lvl 18
	{4, 3, 1, 0},			// lvl 19
	{4, 3, 1, 0},			// lvl 20
	{4, 3, 2, 0},			// lvl 21
	{5, 3, 2, 0},			// lvl 22
	{5, 4, 2, 1},			// lvl 23
	{5, 4, 2, 1},			// lvl 24
	{5, 4, 3, 1},			// lvl 25
	{5, 4, 3, 1},			// lvl 26
	{5, 4, 3, 2},			// lvl 27
	{6, 4, 3, 2},			// lvl 28
	{6, 5, 3, 2},			// lvl 29
	{6, 5, 4, 2},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 22,23
	{2, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{4, 1, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 2, 0, 0},			// lvl 16
	{4, 3, 0, 0},			// lvl 17
	{4, 3, 1, 0},			// lvl 18
	{5, 3, 1, 0},			// lvl 19
	{5, 3, 2, 0},			// lvl 20
	{5, 4, 2, 0},			// lvl 21
	{5, 4, 2, 0},			// lvl 22
	{5, 4, 3, 1},			// lvl 23
	{5, 4, 3, 1},			// lvl 24
	{5, 4, 3, 1},			// lvl 25
	{6, 4, 3, 2},			// lvl 26
	{6, 5, 3, 2},			// lvl 27
	{6, 5, 4, 2},			// lvl 28
	{6, 5, 4, 2},			// lvl 29
	{6, 5, 4, 3},			// lvl 30
	{1, 0, 0, 0},			// lvl 8 wis 24,25
	{2, 0, 0, 0},			// lvl 9
	{2, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{3, 0, 0, 0},			// lvl 13
	{4, 1, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 2, 0, 0},			// lvl 16
	{4, 3, 0, 0},			// lvl 17
	{5, 3, 1, 0},			// lvl 18
	{5, 3, 1, 0},			// lvl 19
	{5, 4, 2, 0},			// lvl 20
	{5, 4, 2, 0},			// lvl 21
	{5, 4, 2, 0},			// lvl 22
	{5, 4, 3, 1},			// lvl 23
	{6, 4, 3, 1},			// lvl 24
	{6, 4, 3, 1},			// lvl 25
	{6, 5, 3, 2},			// lvl 26
	{6, 5, 4, 2},			// lvl 27
	{6, 5, 4, 2},			// lvl 28
	{6, 5, 4, 2},			// lvl 29
	{6, 5, 4, 3},			// lvl 30
	{2, 0, 0, 0},			// lvl 8 wis 26,27
	{2, 0, 0, 0},			// lvl 9
	{3, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{4, 0, 0, 0},			// lvl 13
	{4, 2, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 3, 0, 0},			// lvl 16
	{4, 3, 0, 0},			// lvl 17
	{5, 3, 1, 0},			// lvl 18
	{5, 4, 2, 0},			// lvl 19
	{5, 4, 2, 0},			// lvl 20
	{5, 4, 2, 0},			// lvl 21
	{5, 4, 3, 0},			// lvl 22
	{5, 4, 3, 1},			// lvl 23
	{6, 4, 3, 1},			// lvl 24
	{6, 5, 3, 2},			// lvl 25
	{6, 5, 4, 2},			// lvl 26
	{6, 5, 4, 2},			// lvl 27
	{6, 5, 4, 2},			// lvl 28
	{6, 5, 4, 3},			// lvl 29
	{6, 6, 4, 3},			// lvl 30
	{2, 0, 0, 0},			// lvl 8 wis 28,29
	{2, 0, 0, 0},			// lvl 9
	{3, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{3, 0, 0, 0},			// lvl 12
	{4, 0, 0, 0},			// lvl 13
	{4, 2, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{4, 3, 0, 0},			// lvl 16
	{5, 3, 0, 0},			// lvl 17
	{5, 3, 2, 0},			// lvl 18
	{5, 4, 2, 0},			// lvl 19
	{5, 4, 2, 0},			// lvl 20
	{5, 4, 3, 0},			// lvl 21
	{5, 4, 3, 0},			// lvl 22
	{6, 4, 3, 1},			// lvl 23
	{6, 5, 3, 2},			// lvl 24
	{6, 5, 3, 2},			// lvl 25
	{6, 5, 4, 2},			// lvl 26
	{6, 5, 4, 2},			// lvl 27
	{6, 5, 4, 3},			// lvl 28
	{6, 5, 4, 3},			// lvl 29
	{7, 6, 5, 3},			// lvl 30
	{2, 0, 0, 0},			// lvl 8 wis 30
	{3, 0, 0, 0},			// lvl 9
	{3, 0, 0, 0},			// lvl 10
	{3, 0, 0, 0},			// lvl 11
	{4, 0, 0, 0},			// lvl 12
	{4, 0, 0, 0},			// lvl 13
	{4, 2, 0, 0},			// lvl 14
	{4, 2, 0, 0},			// lvl 15
	{5, 3, 0, 0},			// lvl 16
	{5, 3, 0, 0},			// lvl 17
	{5, 3, 2, 0},			// lvl 18
	{5, 4, 2, 0},			// lvl 19
	{6, 4, 3, 0},			// lvl 20
	{6, 4, 3, 0},			// lvl 21
	{6, 4, 3, 0},			// lvl 22
	{6, 5, 3, 2},			// lvl 23
	{6, 5, 3, 2},			// lvl 24
	{6, 5, 4, 2},			// lvl 25
	{6, 5, 4, 2},			// lvl 26
	{7, 5, 4, 3},			// lvl 27
	{7, 6, 4, 3},			// lvl 28
	{7, 6, 5, 3},			// lvl 29
	{7, 6, 5, 3},			// lvl 30
};

MaxClassSlot max_slots;

int slot_for_char(CHAR_DATA * ch, int slot_num) {
	int wis_is = -1, wis_line, wis_block;

	if (slot_num < 1 || slot_num > MAX_SLOT || GET_LEVEL(ch) < 1 || IS_NPC(ch)) {
		return -1;
	}

	if (IS_IMMORTAL(ch)) {
		return SPELL_SLOTS_FOR_IMMORTAL;
	}

	if (max_slots.get(GET_CLASS(ch), GET_KIN(ch)) < slot_num) {
		return 0;
	}

	slot_num--;

	switch (GET_CLASS(ch)) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
		wis_is = MAG_SLOTS[(int) GET_LEVEL(ch) - 1][slot_num];
		break;
	case CLASS_NECROMANCER:
		wis_is = NECROMANCER_SLOTS[(int) GET_LEVEL(ch) - 1][slot_num];
		break;
	case CLASS_CLERIC:
		if (GET_LEVEL(ch) >= MIN_CL_LEVEL && slot_num < MAX_CL_SLOT && GET_REAL_WIS(ch) >= MIN_CL_WIS) {
			wis_block = MIN(MAX_CL_WIS, GET_REAL_WIS(ch)) - MIN_CL_WIS;
			wis_block = wis_block / CL_WIS_DIV;
			wis_block = wis_block * (MAX_CL_LEVEL - MIN_CL_LEVEL + 1);
			wis_line = GET_LEVEL(ch) - MIN_CL_LEVEL;
			wis_is = CLERIC_SLOTS[wis_block + wis_line][slot_num];
		}
		break;
	case CLASS_PALADINE:
		if (GET_LEVEL(ch) >= MIN_PA_LEVEL && slot_num < MAX_PA_SLOT && GET_REAL_WIS(ch) >= MIN_PA_WIS) {
			wis_block = MIN(MAX_PA_WIS, GET_REAL_WIS(ch)) - MIN_PA_WIS;
			wis_block = wis_block / PA_WIS_DIV;
			wis_block = wis_block * (MAX_PA_LEVEL - MIN_PA_LEVEL + 1);
			wis_line = GET_LEVEL(ch) - MIN_PA_LEVEL;
			wis_is = PALADINE_SLOTS[wis_block + wis_line][slot_num];
		}
		break;
	case CLASS_MERCHANT:
		if (slot_num < MAX_ME_SLOT) {
			wis_is = MERCHANT_SLOTS[(int) GET_LEVEL(ch) - 1][slot_num];
		}
		break;
	}

	if (wis_is == -1) {
		return 0;
	}

	return ((wis_is || (GET_REMORT(ch) > slot_num)) ? MIN(25, wis_is + ch->get_obj_slot(slot_num) + GET_REMORT(ch)) : 0);
}

void mspell_slot(char *name, int spell, int kin , int chclass, int slot) {
	int bad = 0;

	if (spell < 0 || spell > TOP_SPELL_DEFINE) {
		log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
		return;
	}

	if (kin < 0 || kin >= NUM_KIN) {
		log("SYSERR: assigning '%s' to illegal kin %d/%d.", skill_name(spell), chclass, NUM_KIN);
		bad = 1;
	}

	if (chclass < 0 || chclass >=  NUM_PLAYER_CLASSES) {
		log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell), chclass, NUM_PLAYER_CLASSES - 1);
		bad = 1;
	}

	if (slot < 1 || slot > MAX_SLOT) {
		log("SYSERR: assigning '%s' to illegal slot %d/%d.", skill_name(spell), slot, LVL_IMPL);
		bad = 1;
	}

	if (!bad) {
		spell_info[spell].slot_forc[chclass][kin] = slot;
		max_slots.init(chclass, kin, slot);
		log("SLOT set '%s' kin '%d' classes %d value %d", name, kin, chclass, slot);
	}

}

MaxClassSlot::MaxClassSlot() {
	for (int i = 0; i < NUM_PLAYER_CLASSES; ++i) 	{
		for (int k = 0; k < NUM_KIN; ++k) 		{
			_max_class_slot[i][k] = 0;
		}
	}
}

void MaxClassSlot::init(int chclass, int kin, int slot) {
	if (_max_class_slot[chclass][kin] < slot) 	{
		_max_class_slot[chclass][kin] = slot;
	}
}

int MaxClassSlot::get(int chclass, int kin) const {
	if (kin < 0
		|| kin >= NUM_KIN
		|| chclass < 0
		|| chclass >= NUM_PLAYER_CLASSES) {
		return 0;
	}
	return _max_class_slot[chclass][kin];
}

int MaxClassSlot::get(const CHAR_DATA* ch) const
{
	return this->get(GET_CLASS(ch), GET_KIN(ch));
}

}; // namespace ClassPlayer

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
