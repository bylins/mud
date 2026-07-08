#ifndef BYLINS_AFFECT_DATA_H
#define BYLINS_AFFECT_DATA_H

#include "affect_contants.h"
#include "gameplay/skills/skills.h"
#include "engine/structs/flag_data.h"
#include "engine/structs/structs.h"

#include <vector>
#include <list>
#include <bitset>
#include <string>
#include <fstream>
#include <iterator>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <set>

extern std::unordered_set<CharData *> affected_mobs;

// An affect structure. //
class IAffectHandler;

// Maps an affect's location enum to the flag enum stored in Affect::affect_type.
// Character affects (EApply) use EAffect by default; room affects specialize this
// (see magic_rooms.h) to room_spells::ERoomAffect.
template<typename TLocation>
struct AffectFlagType {
	using type = EAffect;
};

template<typename TLocation>
class Affect {
 public:
	using shared_ptr = std::shared_ptr<Affect<TLocation>>;

	Affect() : type(ESpell::kUndefined), duration(0), modifier(0), location(static_cast<TLocation>(0)),
			   battleflag(0), caster_id(0),
			   apply_time(0) {};
	[[nodiscard]] bool removable() const;

	ESpell type;        // The type of spell that caused this      //
	int duration;    // For how long its effects will last      //
	int modifier;        // This is added to appropriate ability     //
	TLocation location;        // Tells which ability to change(APPLY_XXX) //
	Bitvector battleflag;       //*** SUCH AS HOLD,SIELENCE etc
	// The single flag this affect sets while active (AFF_XXX). Its enum type
	// follows the affect's location kind via AffectFlagType (EAffect for chars,
	// ERoomAffect for rooms). kUndefined/0 means the affect sets no flag.
	typename AffectFlagType<TLocation>::type affect_type{};
	FlagData aff;
	long caster_id; //Unique caster ID //
	// (Бывшее поле `must_handled` мигрировало в `battleflag & kAfMustBeHandled`; занимало
	// одно и то же место в семантике и теперь не дублирует battleflag. См. EAffFlag.)
	sh_int apply_time; // Указывает сколько аффект висит (пока используется только в комнатах) //
	std::shared_ptr<IAffectHandler> handler; //обработчик аффектов
	// Сила наложенного заклинания (потенция) на момент наложения: dice + skill_coeff +
	// stat_coeff из potency_roll. Сравнивается с потенцией снимающего заклинания при
	// развеивании (CastUnaffects). 0 для аффектов не от заклинаний (вещи, врождённые,
	// загруженные из файла) -- такие снимаются легко.
	float potency{0.0f};
	// true, если аффект наложен "злым" (violent) заклинанием, т.е. это дебафф; false -- бафф.
	bool debuff{false};
	// Число стаков этого аффекта (issue.affect-stacks). 1 по умолчанию. Повторное наложение
	// до предела <modifier stack=N> увеличивает счётчик и суммирует модификатор; успешное
	// снятие при stacks > 1 уменьшает счётчик (и модификатор пропорционально) вместо удаления.
	int stacks{1};
	// PK-метка: для room-affect'ов kPortalTimer хранит caster_id игрока, поставившего пенту
	// в режиме мести/боя (pkPortal). Заменил поле RoomData::pkPenterUnique: связан с
	// конкретным аффектом, а не с комнатой, поэтому корректно работает с несколькими
	// пентами в одной комнате и автоматически уходит вместе с аффектом по таймеру.
	// Для char-аффектов и room-аффектов не-пента типа всегда 0; в save-файлы не пишется
	// (room-аффекты не персистентны, char-аффекты этим полем не пользуются).
	long pk_unique{0};
};

template<>
bool Affect<EApply>::removable() const;

// Возможно эту структуру следует перенести в отдельный модуль для обкаста предметов.
struct obj_affected_type {
	EApply location;    // Which ability to change (APPLY_XXX) //
	int modifier;                // How much it changes by              //

	obj_affected_type() : location(EApply::kNone), modifier(0) {}

	obj_affected_type(EApply __location, int __modifier)
		: location(__location), modifier(__modifier) {}

	// для сравнения в sedit
	bool operator!=(const obj_affected_type &r) const {
		return (location != r.location || modifier != r.modifier);
	}
	bool operator==(const obj_affected_type &r) const {
		return !(*this != r);
	}
};

void UpdateAffectOnPulse(CharData *ch, int count);
void player_timed_update();
void player_affect_update();
void battle_affect_update(CharData *ch);
void mobile_affect_update();

void affect_total(CharData *ch);
void affect_modify(CharData *ch, EApply loc, int mod, EAffect bitv, bool add);
std::pair<EApply, int> GetApplyByWeaponAffect(EWeaponAffect element, CharData *ch);
void affect_to_char(CharData *ch, const Affect<EApply> &af);
void RemoveAffectFromChar(CharData *ch, ESpell spell_id);
void RemoveAffectFromCharAndRecalculate(CharData *ch, ESpell spell_id);
bool IsAffectedBySpell(CharData *ch, ESpell type);
bool IsAffectedBySpellWithCasterId(CharData *ch, CharData *vict, ESpell type);
void ImposeAffect(CharData *ch, const Affect<EApply> &af);
void ImposeAffect(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod);
void ImposeAffectNoRecalc(CharData *ch, const Affect<EApply> &af);
void ImposeAffectNoRecalc(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod);
void reset_affects(CharData *ch);
void reset_affects_no_recalc(CharData *ch);
bool no_bad_affects(ObjData *obj);
bool IsNegativeApply(EApply location);
bool GetAffectNumByName(const std::string &affName, EAffect &result);
// Skill-based duration (issue.calc-duration): `caster` provides the skill (via CalcNoviceSkillBonus,
// capped at kNoviceSkillThreshold so the bonus can't grow without bound on high-level monsters);
// `victim` decides the unit (PC -> hour-to-tick conversion, NPC -> raw). skill_id == kUndefined
// skips the skill bonus entirely (flat duration). min/max keep OLD-style semantics: a 0 means
// "no clamp on that side", a positive value clamps the bonus accordingly. The previous level-based
// overload was removed once every caller migrated to this one.
int CalcDuration(CharData *caster, CharData *victim, ESkill skill_id,
				 unsigned base, unsigned skill_divisor, int min, int max, int skill_override = -1);

#endif //BYLINS_AFFECT_DATA_H
