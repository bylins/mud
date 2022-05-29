#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "entities/entities_constants.h"
#include "game_magic/spells.h"
#include "game_skills/skills.h"

class CharData;
struct RoomData;
class ObjData;

/**
 * Проверяется что первая строка является эквивалентом второй, например
 * строка 'ог ша' является эквивалентом строки 'огненный шар'.
 */
bool IsEquivalent(const char *first_str, const char *second_str);
bool IsEquivalent(const std::string &first_str, const std::string &second_str);

ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillId(char *name);
ESkill FixNameFndFindSkillId(std::string &name);

ESpell FindSpellId(const std::string &name);
ESpell FindSpellId(const char *name);
ESpell FixNameAndFindSpellId(std::string &name);

ESpell FindSpellIdWithName(const std::string &name);

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom);
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj);
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);
int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);
int CalcRequiredLevel(const CharData *ch, ESpell spell_id);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Resistance calculate //
int ApplyResist(CharData *ch, int resist_type, int effect);
EResist GetResisTypeWithElement(EElement element);
EResist GetResistType(ESpell spell_id);

#endif // SPELL_PARSER_HPP_
