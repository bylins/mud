//
// Created by Sventovit on 07.09.2024.
//

#include "engine/entities/char_data.h"
#include "gameplay/affects/affect_messages.h"
#include "administration/privilege.h"
#include "utils/grammar/declensions.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/affects/affect_data.h"

#include <algorithm>
#include <string>
#include <vector>

std::array<EAffect, 3> hiding = {EAffect::kSneak, EAffect::kHide, EAffect::kDisguise};

namespace {

// issue.affects-squash: a permanent (duration -1) or pulse-decayed affect is bucketed into a
// remaining-time label. Factored out so the squashed (mortal) and full (immortal / "аффекты все")
// renderers agree on the arithmetic.
int AffectDisplayMod(const Affect<EApply>::shared_ptr &aff) {
	// Pulse-decayed affects count down in ~1/25.5s pulses; convert to the tick scale used below.
	if (aff->battleflag.get_plane(0) == static_cast<Bitvector>(kAfPulsedec)) {
		return aff->duration / 51;
	}
	return aff->duration;
}

void FormatAffectDuration(int mod, char *out, size_t out_sz) {
	// A permanent affect (mod < 0) reads as unlimited, not "less than an hour".
	if (mod < 0) {
		snprintf(out, out_sz, "(постоянно)");
	} else if ((mod + 1) / kSecsPerMudHour) {
		snprintf(out, out_sz, "(%d %s)",
				 (mod + 1) / kSecsPerMudHour + 1,
				 grammar::GetDeclensionInNumber((mod + 1) / kSecsPerMudHour + 1, grammar::EWhat::kHour));
	} else {
		snprintf(out, out_sz, "(менее часа)");
	}
}

// issue.affects-squash: one displayed row, aggregating every affect that renders under the same name.
struct SquashRow {
	std::string name;      // affect short-desc (identity as the player sees it)
	int count = 0;         // number of sources (stack-equivalents) collapsed into this row
	int best_mod = 0;      // longest remaining time among timed sources
	bool has_timed = false;
	bool permanent = false;
};

}  // namespace

void do_affects(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char sp_name[kMaxStringLength];
	const size_t agr_length = strlen(argument);

	if (*argument && !strn_cmp(argument, "краткий", agr_length)) {
		if (!ch->get_master()) {
			group::print_one_line(ch, ch, true, 0);
		} else {
			group::print_one_line(ch, ch, false, 0);
		}
		return;
	}

	// issue.affects-squash: "аффекты все" forces the full, un-collapsed per-source list. Mortals
	// otherwise get one row per distinct effect (duplicate sources collapsed, longest duration shown);
	// immortals always see every slot (with modifier/potency detail).
	const bool show_all = (*argument && !strn_cmp(argument, "все", agr_length));
	const bool squash = !privilege::IsImmortal(ch) && !show_all;

	// Show the bitset without "hiding" etc.
	auto aff_copy = ch->char_specials.saved.affected_by;
	for (auto j : hiding) {
		aff_copy.unset(j);
	}

	snprintf(buf2, sizeof(buf2), "%s", affects::DescribeActive(aff_copy, ", ").c_str());
	std::vector<std::string> out_str = utils::Split(buf2, ',');
	// "Аффекты: " передаём префиксом: учитывается в ширине строки, но не
	// склеивается через ", " (иначе после метки была бы лишняя запятая).
	snprintf(buf, kMaxStringLength, "%s%s%s\r\n", kColorYel,
			 utils::OutWordsList(out_str, ch->player_specials->saved.stringLength, ", ", "Аффекты: ").c_str(),
			 kColorNrm);
	SendMsgToChar(buf, ch);

	if (ch->affected.empty()) {
		return;
	}

	// --- Squashed view (mortals): one row per distinct effect, longest remaining duration, [xN]. ---
	if (squash) {
		std::vector<SquashRow> rows;
		for (const auto &aff : ch->affected) {
			std::string name = affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc).c_str();
			const int mod = AffectDisplayMod(aff);
			auto it = std::find_if(rows.begin(), rows.end(),
								   [&name](const SquashRow &r) { return r.name == name; });
			if (it == rows.end()) {
				rows.push_back(SquashRow{name, 0, 0, false, false});
				it = std::prev(rows.end());
			}
			// Count stack-equivalents so a single multi-stack affect still reads [xN] as before,
			// and several items granting the same effect collapse into one [xN] row.
			it->count += (aff->stacks > 1 ? aff->stacks : 1);
			if (mod < 0) {
				it->permanent = true;
			} else if (!it->has_timed || mod > it->best_mod) {
				it->best_mod = mod;
				it->has_timed = true;
			}
		}
		for (const auto &r : rows) {
			// A permanent source wins the label; otherwise show the longest remaining time.
			FormatAffectDuration(r.permanent ? -1 : r.best_mod, buf2, sizeof(buf2));
			snprintf(buf, kMaxStringLength, "%s%s%-21s %-12s%s",
					 (!r.name.empty() && r.name[0] == '!') ? "Состояние  : " : "Заклинание : ",
					 kColorBoldCyn, r.name.c_str(), buf2, kColorNrm);
			if (r.count > 1) {
				snprintf(buf + strlen(buf), kMaxStringLength - strlen(buf), " [x%d]", r.count);
			}
			SendMsgToChar(strcat(buf, "\r\n"), ch);
		}
		return;
	}

	// --- Full view (immortals, or "аффекты все"): one line per affect slot, no collapsing. ---
	const bool immortal = privilege::IsImmortal(ch);
	for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i) {
		const auto aff = *affect_i;

		*buf2 = '\0';
		// issue.affect-migration: name the affect by its own identity (affect_type kShortDesc);
		// fall back to the casting spell's name for affects not yet migrated off Affect::type.
		snprintf(sp_name, sizeof(sp_name), "%s",
				affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc).c_str());
		FormatAffectDuration(AffectDisplayMod(aff), buf2, sizeof(buf2));
		snprintf(buf, kMaxStringLength, "%s%s%-21s %-12s%s ",
				 *sp_name == '!' ? "Состояние  : " : "Заклинание : ",
				 kColorBoldCyn, sp_name, buf2, kColorNrm);
		*buf2 = '\0';
		if (immortal) {
			if (aff->modifier) {
				sprintf(buf2, "%-3d к параметру: %s", aff->modifier, apply_types[(int) aff->location]);
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", buf2);
			}
			// Show the affect's short-desc; an anonymous affect (kDefault/kUndefined) resolves
			// via the shared kDefault sheaf fallback to "странное ощущение".
			if (!affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc).empty()) {
				if (*buf2) {
					strncat(buf, ", устанавливает ", sizeof(buf) - strlen(buf) - 1);
				} else {
					strncat(buf, "устанавливает ", sizeof(buf) - strlen(buf) - 1);
				}
				strncat(buf, kColorBoldRed, sizeof(buf) - strlen(buf) - 1);
				snprintf(buf2, sizeof(buf2), "%s", affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc).c_str());
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", buf2);
				strncat(buf, kColorNrm, sizeof(buf) - strlen(buf) - 1);
			}
		}
		// Stack count (issue.affect-stacks): show [xN] for a multi-stack affect.
		if (aff->stacks > 1) {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " [x%d]", aff->stacks);
		}
		// Potency for immortals / testers: the cast-roll strength (dice+skill+stat)
		// recorded on the affect at impose time; drives the dispel comparison in
		// CastUnaffects::DispelSucceeds. 0 means "not recorded" (charms, name-tied
		// affects, etc.).
		if (immortal || ch->IsFlagged(EPrf::kTester)) {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " [p: %.1f]", aff->potency);
		}
		SendMsgToChar(strcat(buf, "\r\n"), ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
