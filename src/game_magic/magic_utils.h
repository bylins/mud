#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "entities/entities_constants.h"
#include "game_magic/spells.h"
#include "game_skills/skills.h"

class CharData;
struct RoomData;
class ObjData;

ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillNum(char *name);
ESkill FixNameFndFindSkillNum(std::string &name);

ESpell FindSpellNum(const char *name);
ESpell FixNameAndFindSpellNum(char *name);
ESpell FixNameAndFindSpellNum(std::string &name);

int FindCastTarget(int spellnum, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom);
void SaySpell(CharData *ch, int spellnum, CharData *tch, ObjData *tobj);
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, int spellnum, int level);
int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, int spellnum);
int CalcRequiredLevel(const CharData *ch, int spellnum);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, int spellnum, int spell_subst);

#endif // SPELL_PARSER_HPP_
