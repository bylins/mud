#ifndef __SPELL_PARSER_HPP__
#define __SPELL_PARSER_HPP__

#include "skills.h"

class CHAR_DATA;
class ROOM_DATA;
class OBJ_DATA;

ESkill FindSkillNum(const char *name);
ESkill FixNameAndFindSkillNum(char *name);
ESkill FixNameFndFindSkillNum(std::string &name);
int FindSpellNum(const char *name);
int FixNameAndFindSpellNum(char *name);
int FixNameAndFindSpellNum(std::string &name);
int FindCastTarget(int spellnum, const char *t, CHAR_DATA *ch, CHAR_DATA **tch, OBJ_DATA **tobj, ROOM_DATA **troom);
void SaySpell(CHAR_DATA *ch, int spellnum, CHAR_DATA *tch, OBJ_DATA *tobj);
int CallMagic(CHAR_DATA *caster, CHAR_DATA *cvict, OBJ_DATA *ovict, ROOM_DATA *rvict, int spellnum, int level);
int CalculateCastSuccess(CHAR_DATA *ch, CHAR_DATA *victim, int casting_type, int spellnum);
int CalculateRequiredLevel(const CHAR_DATA *ch, int spellnum);
int CastSpell(CHAR_DATA *ch, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom, int spellnum, int spell_subst);

#endif // __SPELL_PARSER_HPP__
