#ifndef BYLINS_AFFECT_DATA_H
#define BYLINS_AFFECT_DATA_H

#include "affect_contants.h"
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

template<typename TLocation>
class Affect {
 public:
	using shared_ptr = std::shared_ptr<Affect<TLocation>>;

	Affect() : type(ESpell::kUndefined), duration(0), modifier(0), location(static_cast<TLocation>(0)),
			   battleflag(0), bitvector(0), caster_id(0), must_handled(false),
			   apply_time(0) {};
	[[nodiscard]] bool removable() const;

	ESpell type;        // The type of spell that caused this      //
	int duration;    // For how long its effects will last      //
	int modifier;        // This is added to appropriate ability     //
	TLocation location;        // Tells which ability to change(APPLY_XXX) //
	Bitvector battleflag;       //*** SUCH AS HOLD,SIELENCE etc
	Bitvector bitvector;        // Tells which bits to set (AFF_XXX) //
	FlagData aff;
	long caster_id; //Unique caster ID //
	bool must_handled; // Указывает муду что для аффекта должен быть вызван обработчик (пока только для комнат) //
	sh_int apply_time; // Указывает сколько аффект висит (пока используется только в комнатах) //
	std::shared_ptr<IAffectHandler> handler; //обработчик аффектов
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
void affect_to_char(CharData *ch, const Affect<EApply> &af);
void RemoveAffectFromChar(CharData *ch, ESpell spell_id);
void RemoveAffectFromCharAndRecalculate(CharData *ch, ESpell spell_id);
bool IsAffectedBySpell(CharData *ch, ESpell type);
bool IsAffectedBySpellWithCasterId(CharData *ch, CharData *vict, ESpell type);
void ImposeAffect(CharData *ch, const Affect<EApply> &af);
void ImposeAffect(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod);
void reset_affects(CharData *ch);
bool no_bad_affects(ObjData *obj);
bool IsNegativeApply(EApply location);
bool GetAffectNumByName(const std::string &affName, EAffect &result);
int CalcDuration(CharData *ch, int cnst, int level, int level_divisor, int min, int max);

#endif //BYLINS_AFFECT_DATA_H
