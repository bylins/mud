#ifndef __SPELL_PARSER_HPP__
#define __SPELL_PARSER_HPP__

#include "skills.h"

class CHAR_DATA;
class ROOM_DATA;
class OBJ_DATA;

ESkill find_skill_num(const char *name);
ESkill fix_name_and_find_skill_num(char *name);
ESkill fix_name_and_find_skill_num(std::string& name);
int find_spell_num(const char *name);
int fix_name_and_find_spell_num(char* name);
int fix_name_and_find_spell_num(std::string& name);
//int mag_manacost(const CHAR_DATA* ch, int spellnum);
int find_cast_target(int spellnum, const char *t, CHAR_DATA * ch, CHAR_DATA ** tch, OBJ_DATA ** tobj, ROOM_DATA ** troom);
void say_spell(CHAR_DATA * ch, int spellnum, CHAR_DATA * tch, OBJ_DATA * tobj);
int call_magic(CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, ROOM_DATA *rvict, int spellnum, int level);
int spell_use_success(CHAR_DATA * ch, CHAR_DATA * victim, int casting_type, int spellnum);
int spell_create_level(const CHAR_DATA* ch, int spellnum);
int cast_spell(CHAR_DATA * ch, CHAR_DATA * tch, OBJ_DATA * tobj, ROOM_DATA * troom, int spellnum, int spell_subst);

#endif // __SPELL_PARSER_HPP__
