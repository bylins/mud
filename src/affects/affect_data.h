#ifndef BYLINS_AFFECT_DATA_H
#define BYLINS_AFFECT_DATA_H

#include "affect_contants.h"
#include "structs/flag_data.h"
#include "structs/structs.h"

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

// An affect structure. //
class IAffectHandler;

template<typename TLocation>
class Affect {
 public:
	using shared_ptr = std::shared_ptr<Affect<TLocation>>;

	Affect() : type(0), duration(0), modifier(0), location(static_cast<TLocation>(0)),
			   battleflag(0), bitvector(0), caster_id(0), must_handled(false),
			   apply_time(0) {};
	[[nodiscard]] bool removable() const;

	sh_int type;        // The type of spell that caused this      //
	int duration;    // For how long its effects will last      //
	int modifier;        // This is added to appropriate ability     //
	TLocation location;        // Tells which ability to change(APPLY_XXX) //
	long battleflag;       //*** SUCH AS HOLD,SIELENCE etc
	FlagData aff;
	Bitvector bitvector;        // Tells which bits to set (AFF_XXX) //
	long caster_id; //Unique caster ID //
	bool must_handled; // Указывает муду что для аффекта должен быть вызван обработчик (пока только для комнат) //
	sh_int apply_time; // Указывает сколько аффект висит (пока используется только в комнатах) //
	std::shared_ptr<IAffectHandler> handler; //обработчик аффектов
};

template<>
bool Affect<EApplyLocation>::removable() const;

// Возможно эту структуру следует перенести в отдельный модуль для обкаста предметов.
struct obj_affected_type {
	EApplyLocation location;    // Which ability to change (APPLY_XXX) //
	int modifier;                // How much it changes by              //

	obj_affected_type() : location(APPLY_NONE), modifier(0) {}

	obj_affected_type(EApplyLocation __location, int __modifier)
		: location(__location), modifier(__modifier) {}

	// для сравнения в sedit
	bool operator!=(const obj_affected_type &r) const {
		return (location != r.location || modifier != r.modifier);
	}
	bool operator==(const obj_affected_type &r) const {
		return !(*this != r);
	}
};

void pulse_affect_update(CharacterData *ch);
void player_affect_update();
void battle_affect_update(CharacterData *ch);
void mobile_affect_update();

void affect_total(CharacterData *ch);
void affect_modify(CharacterData *ch, byte loc, int mod, EAffectFlag bitv, bool add);
void affect_to_char(CharacterData *ch, const Affect<EApplyLocation> &af);
void affect_from_char(CharacterData *ch, int type);
bool affected_by_spell(CharacterData *ch, int type);
void affect_join_fspell(CharacterData *ch, const Affect<EApplyLocation> &af);
void affect_join(CharacterData *ch, Affect<EApplyLocation> &af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void reset_affects(CharacterData *ch);
bool no_bad_affects(ObjectData *obj);

#endif //BYLINS_AFFECT_DATA_H
