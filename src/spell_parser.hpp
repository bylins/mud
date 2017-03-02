#ifndef __SPELL_PARSER_HPP__
#define __SPELL_PARSER_HPP__

#include "skills.h"

ESkill find_skill_num(const char *name);
ESkill fix_name_and_find_skill_num(char *name);
ESkill fix_name_and_find_skill_num(std::string& name);
int fix_name_and_find_spell_num(char* name);
int fix_name_and_find_spell_num(std::string& name);

#endif // __SPELL_PARSER_HPP__