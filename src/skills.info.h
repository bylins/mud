#ifndef __SKILLS_INFO__
#define __SKILLS_INFO__

#include "skills.h"
#include "classes/constants.hpp"

struct skillInfo_t {
	byte min_position;
	int min_remort[NUM_PLAYER_CLASSES][NUM_KIN];
	int min_level[NUM_PLAYER_CLASSES][NUM_KIN];
	int level_decrement[NUM_PLAYER_CLASSES][NUM_KIN];
	long int k_improve[NUM_PLAYER_CLASSES][NUM_KIN];
	int classknow[NUM_PLAYER_CLASSES][NUM_KIN];
	int max_percent;
	const char *name;
	const char *shortName;
};

extern struct skillInfo_t skill_info[];

#endif //__SKILLS_INFO__
