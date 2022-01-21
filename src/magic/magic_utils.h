#ifndef __SPELL_PARSER_HPP__
#define __SPELL_PARSER_HPP__

#include "skills.h"

class CharacterData;
class RoomData;
class ObjectData;

ESkill FindSkillNum(const char *name);
ESkill FixNameAndFindSkillNum(char *name);
ESkill FixNameFndFindSkillNum(std::string &name);
int FindSpellNum(const char *name);
int FixNameAndFindSpellNum(char *name);
int FixNameAndFindSpellNum(std::string &name);
int FindCastTarget(int spellnum, const char *t, CharacterData *ch, CharacterData **tch, ObjectData **tobj, RoomData **troom);
void SaySpell(CharacterData *ch, int spellnum, CharacterData *tch, ObjectData *tobj);
int CallMagic(CharacterData *caster, CharacterData *cvict, ObjectData *ovict, RoomData *rvict, int spellnum, int level);
int CalculateCastSuccess(CharacterData *ch, CharacterData *victim, int casting_type, int spellnum);
int CalculateRequiredLevel(const CharacterData *ch, int spellnum);
int CastSpell(CharacterData *ch, CharacterData *tch, ObjectData *tobj, RoomData *troom, int spellnum, int spell_subst);

#endif // __SPELL_PARSER_HPP__
