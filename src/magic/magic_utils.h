#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "entities/entity_constants.h"
#include "skills.h"

class CharacterData;
class RoomData;
class ObjectData;

ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillNum(char *name);
ESkill FixNameFndFindSkillNum(std::string &name);
int FindSpellNum(const char *name);
int FixNameAndFindSpellNum(char *name);
int FixNameAndFindSpellNum(std::string &name);
int FindCastTarget(int spellnum, const char *t, CharacterData *ch, CharacterData **tch, ObjectData **tobj, RoomData **troom);
void SaySpell(CharacterData *ch, int spellnum, CharacterData *tch, ObjectData *tobj);
int CallMagic(CharacterData *caster, CharacterData *cvict, ObjectData *ovict, RoomData *rvict, int spellnum, int level);
int CalcCastSuccess(CharacterData *ch, CharacterData *victim, ESaving saving, int spellnum);
int CalcRequiredLevel(const CharacterData *ch, int spellnum);
int CastSpell(CharacterData *ch, CharacterData *tch, ObjectData *tobj, RoomData *troom, int spellnum, int spell_subst);

#endif // SPELL_PARSER_HPP_
