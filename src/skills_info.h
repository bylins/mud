#ifndef __SKILLS_INFO__
#define __SKILLS_INFO__

#include "skills.h"
#include "classes/class_constants.h"

struct SkillInfoType {
	byte min_position;
	int min_remort[NUM_PLAYER_CLASSES][kNumKins];
	int min_level[NUM_PLAYER_CLASSES][kNumKins];
	int level_decrement[NUM_PLAYER_CLASSES][kNumKins];
	long int k_improve[NUM_PLAYER_CLASSES][kNumKins];
	int classknow[NUM_PLAYER_CLASSES][kNumKins];
	int difficulty;
	int save_type;
	int cap;
	const char *name;
	const char *shortName;
};

extern struct SkillInfoType skill_info[];

const char *skill_name(int num);

#endif //__SKILLS_INFO__
