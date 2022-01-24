#ifndef SKILLS_INFO_
#define SKILLS_INFO_

#include "skills.h"

#include "entities/entity_constants.h"
#include "classes/class_constants.h"

struct SkillInfoType {
	bool autosuccess = false;
	int min_remort[NUM_PLAYER_CLASSES][kNumKins];
	int min_level[NUM_PLAYER_CLASSES][kNumKins];
	int level_decrement[NUM_PLAYER_CLASSES][kNumKins];
	long int k_improve[NUM_PLAYER_CLASSES][kNumKins];
	int classknow[NUM_PLAYER_CLASSES][kNumKins];
	int difficulty;
	ESaving save_type;
	int cap;
	const char *name;
	const char *shortName;
};

extern struct SkillInfoType skill_info[];

// Всей этой красоте место в структуре скилл_инфо, но во-первых, в messages запихали и сообщения спеллов,
// и еще черта лысого в ступе, во-вторых, это надо переделывать структуру и ее парсинг. Поэтому пока так.
struct AttackMsg {
	char *attacker_msg = nullptr;	// message to attacker //
	char *victim_msg = nullptr;		// message to victim   //
	char *room_msg = nullptr;		// message to room     //
};

struct AttackMsgSet {
	AttackMsg die_msg;        // messages when death        //
	AttackMsg miss_msg;        // messages when miss         //
	AttackMsg hit_msg;        // messages when hit       //
	AttackMsg god_msg;        // messages when hit on god      //
	AttackMsgSet *next = nullptr;    // to next messages of this kind.   //
};

struct AttackMessages {
	int attack_type = 0;				// Attack type          //
	int number_of_attacks = 0;			// How many attack messages to chose from. //
	AttackMsgSet *msg_set = nullptr;	// List of messages.       //
};

const int kMaxMessages = 600; // Эту похабень надо переделать на вектор.
extern AttackMessages fight_messages[kMaxMessages];

const char *skill_name(int num);

#endif // SKILLS_INFO_
