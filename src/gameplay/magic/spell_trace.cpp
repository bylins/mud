/**
 \file spell_trace.cpp - a part of the Bylins engine.
 \brief issue.spell-tc-info: shared tester/coder diagnostic emitter for spell effects.
*/

#include "spell_trace.h"
#include "gameplay/mechanics/minions.h"

#include "engine/entities/char_data.h"

#include <cstdarg>
#include <cstdio>

namespace spell_trace {

namespace {

// The character who would actually receive send_to_TC output: a charmice forwards to its master;
// plain NPCs (and master-less / NPC-mastered charmice) receive nothing.
const CharData *Recipient(const CharData *ch) {
	if (ch == nullptr) {
		return nullptr;
	}
	if (IsCharmice(ch)) {
		if (!ch->has_master() || ch->get_master()->IsNpc()) {
			return nullptr;
		}
		return ch->get_master();
	}
	if (ch->IsNpc()) {
		return nullptr;
	}
	return ch;
}

// Mirrors send_to_TC(false, true, true): the recipient must be a tester or a coder.
bool Wants(const CharData *ch) {
	const CharData *to = Recipient(ch);
	return to != nullptr && (to->IsFlagged(EPrf::kTester) || to->IsFlagged(EPrf::kCoderinfo));
}

} // namespace

bool Active(const CharData *caster, const CharData *victim) {
	return Wants(caster) || (victim != nullptr && victim != caster && Wants(victim));
}

void Line(CharData *caster, CharData *victim, const char *fmt, ...) {
	if (!Active(caster, victim)) {
		return;
	}
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	if (caster != nullptr) {
		caster->send_to_TC(false, true, true, "%s", buf);
	}
	if (victim != nullptr && victim != caster) {
		victim->send_to_TC(false, true, true, "%s", buf);
	}
}

} // namespace spell_trace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
