/**
 \file spell_trace.h - a part of the Bylins engine.
 \brief issue.spell-tc-info: one place to emit a tester/coder diagnostic line for a spell effect.
 \details Effect functions (damage / saving / affect / points / dispel) call spell_trace::Line()
          to report inputs -> modifiers -> result, so a tester can see why a spell produced a given
          number. Line() routes through CharData::send_to_TC(false, true, true, ...), which already
          gates on EPrf::kTester / kCoderinfo (charmice -> master, NPCs skipped), so a non-tester
          pays nothing. Active() is the same cheap predicate, exposed so callers can guard an
          expensive multi-line / per-element breakdown before building it.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_TRACE_H_
#define BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_TRACE_H_

class CharData;

namespace spell_trace {

// True if the caster or (a distinct PC) victim would receive a trace line (tester/coder, with the
// charmice -> master redirect). Cheap flag check -- use it to guard building a detailed breakdown.
[[nodiscard]] bool Active(const CharData *caster, const CharData *victim);

// Format one diagnostic line and send it to the caster and, if distinct, the victim. Self-gating
// (no-op for non-testers, so a bare call is cheap); pass victim == nullptr for a caster-only line.
void Line(CharData *caster, CharData *victim, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

} // namespace spell_trace

#endif // BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_TRACE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
