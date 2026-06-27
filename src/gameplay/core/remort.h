//
// Created by Sventovit on 11.06.2026.
//

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

#ifndef MUD_SRC_GAMEPLAY_CORE_REMORT_H_
#define MUD_SRC_GAMEPLAY_CORE_REMORT_H_

#include "engine/boot/cfg_manager.h"   // cfg_manager::ICfgLoader
#include "utils/parser_wrapper.h"      // parser_wrapper::DataNode

#include <memory>

class CharData;

namespace remort {

void ProcessRemort(CharData *ch, char *argument, int subcmd);

int GetRealRemort(const CharData *ch);
int GetRealRemort(const std::shared_ptr<CharData> &ch);

void SetSkillAfterRemort(CharData *ch);

// --- issue.remort-system: parameters from cfg/remort.xml ---

// false: legacy system (+1 to every base stat per remort).
// true:  proportional system (the highest start stat gains +1/remort, the others scale to keep
//        their original ratio to it). See ApplyRemortStatGains.
[[nodiscard]] bool IsPointsBuy();

// The per-character skill cap: SkillCapStart() + remort_count * SkillCapIncrement().
[[nodiscard]] int SkillCapStart();
[[nodiscard]] int SkillCapIncrement();
[[nodiscard]] int CalcSkillCap(int remort_count);

// The obligatory full skill/spell reset gate (remort.cpp): true when
// remort_count >= stop_obligatory_reset && remort_count % period == 0.
[[nodiscard]] bool IsObligatoryResetDue(int remort_count);

// Base max-HP / max-moves a character is reset to on remort (remort.cpp).
[[nodiscard]] int RemortHp();
[[nodiscard]] int RemortMoves();

// issue.remort-system: compute the base stats a character has for `remort_count` remorts from their
// six start (creation) stats. Writes out[G_STR..G_CHA]. Legacy: start + remort_count each.
// Proportional (points_buy): out[i] = floor((max_start + remort_count) * start[i] / max_start).
// Glory is added by the caller on top. `start`/`out` are indexed by G_STR..G_CHA.
void ApplyRemortStatGains(const int *start, int *out, int remort_count);

// cfg/remort.xml loader (boot via cfg_manager; `reload remort`).
class RemortLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) override;
	void Reload(parser_wrapper::DataNode data) override;
};

} // namespace remort

#endif //MUD_SRC_GAMEPLAY_CORE_REMORT_H_
