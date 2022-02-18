#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "entities/entities_constants.h"
#include "skills.h"

class CharData;
class RoomData;
class ObjData;

ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillNum(char *name);
ESkill FixNameFndFindSkillNum(std::string &name);
int FindSpellNum(const char *name);
int FixNameAndFindSpellNum(char *name);
int FixNameAndFindSpellNum(std::string &name);
int FindCastTarget(int spellnum, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom);
void SaySpell(CharData *ch, int spellnum, CharData *tch, ObjData *tobj);
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, int spellnum, int level);
int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, int spellnum);
int CalcRequiredLevel(const CharData *ch, int spellnum);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, int spellnum, int spell_subst);

#endif // SPELL_PARSER_HPP_
