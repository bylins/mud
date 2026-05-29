#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "gameplay/abilities/feats.h"
#include "engine/entities/entities_constants.h"
#include "spells.h"
#include "gameplay/skills/skills.h"
#include "gameplay/abilities/talents_actions.h"

class CharData;
struct RoomData;
class ObjData;

/**
 * Проверяется что первая строка является эквивалентом второй, например
 * строка 'ог ша' является эквивалентом строки 'огненный шар'.
 */
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
int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);

// True if `caster` bypasses the spell's room-level <blocking> (kNoMagic etc.).
// Greater gods always bypass; uncharmed NPCs (other than tutelars) bypass too -- they
// ignore the room block when ticking their own affects. (issue.room-affects: renamed
// from MayCastInNomagic; the kMagWarcry exception was removed because warcry spells
// simply do not list kNoMagic as a blocking flag.)
bool MayCastInForbiddenRoom(CharData *caster);

// True if `room` carries any of the room flags listed in the spell's
// <blocking><room_flags val="..."/></blocking>. Used in CallMagic and
// CallMagicToRoom to fizzle casts whose XML config forbids the room.
bool IsRoomBlocked(RoomData *room, const talents_actions::FlagCondition &cond);

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);
int MagusCastRequiredLevel(const CharData *ch, ESpell spell_id);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Resistance calculate //
int ApplyResist(CharData *ch, EResist resist_type, int value);
EResist GetResisTypeWithElement(EElement element);
EResist GetResistType(ESpell spell_id);

#endif // SPELL_PARSER_HPP_
