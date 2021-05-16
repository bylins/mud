#ifndef __SKILLS_INFO__
#define __SKILLS_INFO__

#include "skills.h"
#include "classes/constants.hpp"

struct SkillInfoType {
	byte min_position;
	int min_remort[NUM_PLAYER_CLASSES][NUM_KIN];
	int min_level[NUM_PLAYER_CLASSES][NUM_KIN];
	int level_decrement[NUM_PLAYER_CLASSES][NUM_KIN];
	long int k_improve[NUM_PLAYER_CLASSES][NUM_KIN];
	int classknow[NUM_PLAYER_CLASSES][NUM_KIN];
	int save_type;
	int difficulty;
	int cap;
	const char *name;
	const char *shortName;
};

extern struct SkillInfoType skill_info[];

const char *skill_name(int num);

#endif //__SKILLS_INFO__
