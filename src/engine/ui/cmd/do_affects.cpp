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
#include "gameplay/affects/affect_data.h"   // issue.affect-migration: SameAffectIdentity dedup by affect_type

std::array<EAffect, 3> hiding = {EAffect::kSneak, EAffect::kHide, EAffect::kDisguise};


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

	// Routine to show what spells a char is affected by
	if (!ch->affected.empty()) {
		for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i) {
			const auto aff = *affect_i;


			*buf2 = '\0';
			// issue.affect-migration: name the affect by its own identity (affect_type kShortDesc);
			// fall back to the casting spell's name for affects not yet migrated off Affect::type.
			snprintf(sp_name, sizeof(sp_name), "%s",
					affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc).c_str());
			int mod = 0;
			if (aff->battleflag == kAfPulsedec) {
				mod = aff->duration / 51; //если в пульсах приводим к тикам 25.5 в сек 2 минуты
			} else {
				mod = aff->duration;
			}
			(mod + 1) / kSecsPerMudHour
			? sprintf(buf2,
					  "(%d %s)",
					  (mod + 1) / kSecsPerMudHour + 1,
					  grammar::GetDeclensionInNumber((mod + 1) / kSecsPerMudHour + 1, grammar::EWhat::kHour))
			: sprintf(buf2, "(менее часа)");
			snprintf(buf, kMaxStringLength, "%s%s%-21s %-12s%s ",
					 *sp_name == '!' ? "Состояние  : " : "Заклинание : ",
					 kColorBoldCyn, sp_name, buf2, kColorNrm);
			*buf2 = '\0';
			if (!privilege::IsImmortal(ch)) {
				auto next_affect_i = affect_i;
				++next_affect_i;
				if (next_affect_i != ch->affected.end()) {
					const auto &next_affect = *next_affect_i;
					// issue.affect-migration: collapse adjacent slots of the SAME affect (one effect
					// shown once) by identity, not by casting spell -- migrated affects share
					// Affect::type == kUndefined and must stay distinct here.
					if (SameAffectIdentity(aff, next_affect)) {
						continue;
					}
				}
			} else {
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
			if (privilege::IsImmortal(ch) || ch->IsFlagged(EPrf::kTester)) {
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " [p: %.1f]", aff->potency);
			}
			SendMsgToChar(strcat(buf, "\r\n"), ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
