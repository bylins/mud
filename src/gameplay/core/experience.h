/**
\file experience.h - a part of the Bylins engine.
\brief issue.chardata-cleaning: experience-gain rules.
\detail New home for experience-related logic pulled off char_data. Experience
        tables, gain/loss formulas, etc. should accrete here over time; for now it
        holds the zone group size and the "may this kill grant exp?" predicate.
*/

#ifndef BYLINS_SRC_GAMEPLAY_CORE_EXPERIENCE_H_
#define BYLINS_SRC_GAMEPLAY_CORE_EXPERIENCE_H_

class CharData;

#include "engine/boot/cfg_manager.h"

namespace experience {

// Group size configured for the zone the mob `ch` belongs to; 1 for players/invalid.
int GetZoneGroup(const CharData *ch);

// Whether killing `victim` may grant `ch` experience (name ok, not on arena, real mob, ...).
bool OkGainExp(const CharData *ch, const CharData *victim);


// Experience needed to reach `level` (per-class/level table, scaled by remort count).
long GetExpUntilNextLvl(CharData *ch, int level);



// Apply a gained/lost character level (stats, feats, move, save) -- and the one-shot
// unlock notices fired when crossing certain levels (levelup_events).
void levelup_events(CharData *ch);
void advance_level(CharData *ch);
void decrease_level(CharData *ch);


// Grant (or remove) experience to a character, applying level changes, caps, clan exp
// and the remort-eligibility flag. gain_exp_regardless skips the per-kill caps (god cmds).
void EndowExpToChar(CharData *ch, int gain);
void gain_exp_regardless(CharData *ch, int gain);
void gain_battle_exp(CharData *ch, CharData *victim, int dam);   // battle exp from damage dealt
void update_clan_exp(CharData *ch, int gain);

// Per-kill exp caps: most a player may gain/lose in one kill/death.
int max_exp_gain_pc(CharData *ch);
int max_exp_loss_pc(CharData *ch);

// Loads cfg/experience_table.xml (remort coefficients + per-class level tables). Load-only:
// hot reload is intentionally unsupported -- exp tables change rarely and a mid-game reload
// would be unpredictable (issue.experience-table).
class ExperienceTableLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

// Number of loaded remort coefficients (0 when the table is missing/corrupt).
int RemortCoefficientCount();

// True if `ch` may gain experience: coefficients are loaded AND the char's class has a
// non-empty level table. When false the gods' scales are broken and no exp may be granted.
bool CanGainExp(const CharData *ch);

}  // namespace experience

#endif  // BYLINS_SRC_GAMEPLAY_CORE_EXPERIENCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
