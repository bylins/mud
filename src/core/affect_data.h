#ifndef BYLINS_AFFECT_DATA_H
#define BYLINS_AFFECT_DATA_H

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

#include "structs.h"

// An affect structure. //
class IAffectHandler;

template<typename TLocation>
class AFFECT_DATA {
 public:
  using shared_ptr = std::shared_ptr<AFFECT_DATA<TLocation>>;

  AFFECT_DATA() : type(0), duration(0), modifier(0), location(static_cast<TLocation>(0)),
                  battleflag(0), bitvector(0), caster_id(0), must_handled(0),
                  apply_time(0) {};
  bool removable() const;

  sh_int type;        // The type of spell that caused this      //
  int duration;    // For how long its effects will last      //
  int modifier;        // This is added to appropriate ability     //
  TLocation location;        // Tells which ability to change(APPLY_XXX) //
  long battleflag;       //*** SUCH AS HOLD,SIELENCE etc
  FLAG_DATA aff;
  uint32_t bitvector;        // Tells which bits to set (AFF_XXX) //
  long caster_id; //Unique caster ID //
  bool must_handled; // Указывает муду что для аффекта должен быть вызван обработчик (пока только для комнат) //
  sh_int apply_time; // Указывает сколько аффект висит (пока используется только в комнатах) //
  std::shared_ptr<IAffectHandler> handler; //обработчик аффектов
};

template<>
bool AFFECT_DATA<EApplyLocation>::removable() const;

void pulse_affect_update(CHAR_DATA *ch);
void player_affect_update();
void battle_affect_update(CHAR_DATA *ch);
void mobile_affect_update();

void affect_total(CHAR_DATA *ch);
void affect_modify(CHAR_DATA *ch, byte loc, int mod, const EAffectFlag bitv, bool add);
void affect_to_char(CHAR_DATA *ch, const AFFECT_DATA<EApplyLocation> &af);
void affect_from_char(CHAR_DATA *ch, int type);
bool affected_by_spell(CHAR_DATA *ch, int type);
void affect_join_fspell(CHAR_DATA *ch, const AFFECT_DATA<EApplyLocation> &af);
void affect_join(CHAR_DATA *ch,
                 AFFECT_DATA<EApplyLocation> &af,
                 bool add_dur,
                 bool avg_dur,
                 bool add_mod,
                 bool avg_mod);
void reset_affects(CHAR_DATA *ch);
bool no_bad_affects(OBJ_DATA *obj);

#endif //BYLINS_AFFECT_DATA_H
