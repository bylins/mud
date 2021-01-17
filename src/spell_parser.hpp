#ifndef __SPELL_PARSER_HPP__
#define __SPELL_PARSER_HPP__

#include "skills/skills.h"

ESkill find_skill_num(const char *name);
ESkill fix_name_and_find_skill_num(char *name);
ESkill fix_name_and_find_skill_num(std::string& name);
int find_spell_num(const char *name);
int fix_name_and_find_spell_num(char* name);
int fix_name_and_find_spell_num(std::string& name);
int mag_manacost(const CHAR_DATA* ch, int spellnum);

class MaxClassSlot
{
public:
	MaxClassSlot();;

	void init(int chclass, int kin, int slot);
	int get(int chclass, int kin) const;
	int get(const CHAR_DATA* ch) const;

private:
	int max_class_slot_[NUM_PLAYER_CLASSES][NUM_KIN];
};

extern MaxClassSlot max_slots;

#endif // __SPELL_PARSER_HPP__