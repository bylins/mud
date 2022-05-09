#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "entities/entities_constants.h"
#include "game_magic/spells.h"
#include "game_skills/skills.h"

class CharData;
struct RoomData;
class ObjData;

ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillId(char *name);
ESkill FixNameFndFindSkillId(std::string &name);

ESpell FindSpellNum(const char *name);
ESpell FixNameAndFindSpellId(char *name);
ESpell FixNameAndFindSpellId(std::string &name);

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom);
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj);
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);
int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);
int CalcRequiredLevel(const CharData *ch, ESpell spell_id);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

#endif // SPELL_PARSER_HPP_
