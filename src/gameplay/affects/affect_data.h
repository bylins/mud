#ifndef BYLINS_AFFECT_DATA_H
#define BYLINS_AFFECT_DATA_H

#include "affect_contants.h"
#include "gameplay/skills/skills.h"
#include "engine/structs/flag_data.h"
#include "engine/structs/structs.h"

#include <vector>
#include <utility>
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

	Affect() : duration(0), modifier(0), location(static_cast<TLocation>(0)),
			   battleflag(0), caster_id(0),
			   apply_time(0) {};
	[[nodiscard]] bool removable() const;

	int duration;    // For how long its effects will last      //
	int modifier;        // This is added to appropriate ability     //
	TLocation location;        // Tells which ability to change(APPLY_XXX) //
	Bitvector battleflag;       //*** SUCH AS HOLD,SIELENCE etc
	// The single flag this affect sets while active (AFF_XXX). Its enum type
	// follows the affect's location kind via AffectFlagType (EAffect for chars,
	// ERoomAffect for rooms). kUndefined/0 means the affect sets no flag.
	typename AffectFlagType<TLocation>::type affect_type{};
	long caster_id; //Unique caster ID //
	// (Бывшее поле `must_handled` мигрировало в `battleflag & kAfMustBeHandled`; занимало
	// одно и то же место в семантике и теперь не дублирует battleflag. См. EAffFlag.)
	sh_int apply_time; // Указывает сколько аффект висит (пока используется только в комнатах) //
	// Сила наложенного эффекта (потенция) на момент наложения: для заклинаний это dice +
	// skill_coeff + stat_coeff из potency_roll. Сравнивается с потенцией снимающего
	// заклинания при развеивании (CastUnaffects). У аффектов НЕ от заклинаний (умения,
	// вещи, врождённые, загруженные из файла) потенция пока 0 -- не потому, что у них нет
	// силы, а потому, что общего механизма расчёта потенции для не-заклинаний ещё нет
	// (появится в будущем); до тех пор такие аффекты снимаются легко.
	float potency{0.0f};
	// true, если аффект наложен "злым" (violent) заклинанием, т.е. это дебафф; false -- бафф.
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
	// issue.room-affect-trigger-improve: remaining trigger CHARGES for a triggered (room/door) affect.
	// -1 = unlimited (the default -- char affects and untriggered affects never touch this). Set on
	// impose from the spell's <affects><charges max=N>; each time a trigger executes the affect's action
	// the dispatcher decrements it, and at 0 the affect is removed. Runtime-only (not persisted).
	int charges{-1};
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

// issue.affect-migration: "same affect" by IDENTITY (affect_type) rather than by casting spell -- for
// callers that dedup or look up affects (do_affects display, remove_random_affects).
[[nodiscard]] bool SameAffectIdentity(const Affect<EApply>::shared_ptr &a, const Affect<EApply>::shared_ptr &b);

void UpdateAffectOnPulse(CharData *ch, int count);
void player_timed_update();
void player_affect_update();
void battle_affect_update(CharData *ch);
void mobile_affect_update();
// issue.character-affect-triggers: run one char affect's pulse/battle-pulse <actions> on its bearer
// (kPulse always, kBattlePulse only while fighting). Defined in magic.cpp. Returns true if any fired.
bool RunCharAffectTick(CharData *ch, const Affect<EApply>::shared_ptr &aff);

// issue #3610: кому засчитывать урон тика повреждающего аффекта. Узлов одного типа на персонаже
// может быть несколько, с разными авторами или вовсе без автора (отравленная еда, dg-триггер,
// врожденный флаг моба), а тик достается наложенному последним. Если у него автора нет, берем его
// у любого другого узла того же типа -- иначе отравитель терял опыт за смерть жертвы от яда.
// Вынесено отдельно, чтобы правило проверялось тестами (tests/dot.death.credit.cpp).
long SelectAffectAuthorUid(const std::list<Affect<EApply>::shared_ptr> &affects,
						   const Affect<EApply>::shared_ptr &ticking);

// issue.character-affect-triggers: run an affect TYPE's <actions> matching `trig` on the bearer `ch`,
// with event.actor = `actor` (nullptr for actor-less triggers). Used for kExpired (from the affect-update
// loops, actor=null) and kDispell (from RemoveAffectAndAnnounce, actor=dispeller). Defined in magic.cpp;
// recursion is bounded by the shared trigger-action depth guard. Returns true if any action fired.
namespace talents_actions { enum class EActionTrigger; }
// event_amount/event_category feed the EventContext (kPoints: the restored amount + which category, so
// an <action base="tag"> can scale off it and a <trigger category=> action is filtered to that category;
// event_category = -1 means no category, matching every action). Defaults keep kExpired/kDispell callers
// unchanged.
bool RunCharAffectTrigger(CharData *ch, EAffect affect_type, talents_actions::EActionTrigger trig,
						  CharData *actor = nullptr, int event_amount = 0, int event_category = -1);

// issue.character-affect-triggers: fire the dying char's kDeath actions (from die(), before raw_kill;
// killer = the credited killer, may be null). Returns true if an affect PREVENTED the death (a kDeath
// action resolved a <trigger return="0"/> "block"). A preventing affect must heal the char itself (a
// <points><heal>); otherwise it is still at <=0 HP and dies again on the next tick. Defined in magic.cpp.
bool RunCharDeathTriggers(CharData *ch, CharData *killer);

void affect_total(CharData *ch);
void affect_modify(CharData *ch, EApply loc, int mod, EAffect bitv, bool add);
std::pair<EApply, int> GetApplyByWeaponAffect(EWeaponAffect element, CharData *ch);
void affect_to_char(CharData *ch, const Affect<EApply> &af, Bitvector extra_battleflag = 0);
void RemoveAffectFromChar(CharData *ch, EAffect affect_type);
void RemoveAffectFromCharAndRecalculate(CharData *ch, EAffect affect_type);
// issue.affect-migration: un-charm -- remove the whole charm package (affects flagged kAfCharmBond) and,
// for an NPC, schedule extraction + clear hire price. Replaces RemoveAffectFromChar(ESpell::kCharm).
void RemoveCharmBond(CharData *ch, bool recalculate = false);
void RemoveCurableAffects(CharData *ch);
// True if `vict` carries a real (non-failed) affect of this affect_type cast by `ch`.
bool IsAffectedWithCasterId(CharData *ch, CharData *vict, EAffect affect_type);
// True if `ch` carries a real (non-failed) affect of this affect_type -- the affect STRUCTURE
// itself, unlike AFF_FLAGGED which is ALSO set by worn equipment (EWeaponAffect) and a mob's
// innate proto flags. EAffect-keyed (decoupled from the casting spell); the successor to
// IsAffectedBySpell for "does ch currently have this effect" queries.
bool IsAffected(CharData *ch, EAffect affect_type);
// Like IsAffected but ALSO counts failed-attempt markers (affects carrying kAfFailed): true if ch
// has ANY affect of this affect_type -- a working effect OR a lingering failed attempt. Use for the
// "already attempting" anti-spam checks (hide/camouflage): a failed try must still block a retry,
// and the player must not be able to tell a real effect from a failed one.
bool IsAffectedOrAttempting(CharData *ch, EAffect affect_type);
// issue.affect-migration: true if ch has any REAL affect (failed-attempt markers excluded) carrying the
// given battleflag bit. For affect-CATEGORY queries -- e.g. IsAffectedWithFlag(ch, kAfPoison) tests "is
// poisoned by anything" with one flag instead of enumerating every poison spell/affect.
bool IsAffectedWithFlag(CharData *ch, EAffFlag flag);
void ImposeAffect(CharData *ch, const Affect<EApply> &af);
void ImposeAffect(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod);
void ImposeAffectNoRecalc(CharData *ch, const Affect<EApply> &af);
void ImposeAffectNoRecalc(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod);
void reset_affects(CharData *ch);
void reset_affects_no_recalc(CharData *ch);

// issue.mob-flag-affect-materialization: for a flag-only NPC, realize each kAfMaterialize buff FLAG it
// carries (with no matching Affect yet) as a real duration=-1 (permanent, dispellable) affect, so the
// data-driven affect system works for that mob. One affect_total() at the end if anything was added.
void MaterializeMobFlagAffects(CharData *mob);
// issue.autoaffects-hotfix: shield/ward affects from PC equipment are flag-only in this branch (not
// materialized like NPC buffs), so ward handlers that iterate ch->affected miss them. Returns the
// affect types active for ward purposes: real affects (with potency) + flagged kAfMaterialize (potency 0).
// TEMPORARY: a stopgap for the `unstable` line only. On unstable.next equipment affects are fully
// materialized as real Affects (like NPC buffs), so ch->affected already holds them and every ward
// handler can iterate ch->affected directly again -- at that point delete this helper and revert the
// call sites (damage.cpp shield paths, magic.cpp RunAttackWards) back to `for (aff : victim->affected)`.
std::vector<std::pair<EAffect, float>> VictimWardAffects(CharData *ch);
// Materialize every NPC in the given zone (walks the zone's room rnum range).
void MaterializeZoneMobAffects(ZoneRnum zrn);
// (MarkZoneUsed -- the zone-wake hook that calls the above -- is declared in engine/entities/zone.h so
//  the world/movement call sites can use it without pulling in this heavy affect header.)
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
				 unsigned base, unsigned skill_divisor, int min, int max, int skill_override = -1, bool raw_rounds = false);

#endif //BYLINS_AFFECT_DATA_H
