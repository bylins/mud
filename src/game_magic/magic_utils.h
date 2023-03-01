#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "feats.h"
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
bool IsEquivalent(const std::string_view &first_str, const std::string_view &second_str);

EAffect FindAffectId(const std::string_view &name);
EAffect FixNameAndFindAffectId(std::string &name);

ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillId(char *name);
ESkill FixNameFndFindSkillId(std::string &name);

ESpell FindSpellId(const std::string &name);
ESpell FindSpellId(const char *name);
ESpell FixNameAndFindSpellId(std::string &name);

EFeat FindFeatId(const char *name);
EFeat FixNameAndFindFeatId(const std::string &name);

abilities::EAbility FindAbilityId(const std::string &name);
abilities::EAbility FixNameAndFindAbilityId(const std::string &name);

ESpell FindSpellIdWithName(const std::string &name);

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom);
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj);
int CallMagic(CharData *caster, CharData *char_vict, ObjData *obj_vict, RoomData *room_vict, ESpell spell_id, int level);
int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);
int MagusCastRequiredLevel(const CharData *ch, ESpell spell_id);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Resistance calculate //
int ApplyResist(CharData *ch, EResist resist_type, int value);
EResist GetResisTypeWithElement(EElement element);
EResist GetResistType(ESpell spell_id);

#endif // SPELL_PARSER_HPP_
