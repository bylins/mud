//
// Created by Sventovit on 07.09.2024.
//

#include "entities/char_data.h"
#include "game_skills/morph.hpp"
#include "color.h"
#include "structs/global_objects.h"

std::array<EAffect, 3> hiding = {EAffect::kSneak,
								 EAffect::kHide,
								 EAffect::kDisguise};


void do_affects(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	char sp_name[kMaxStringLength];

	// Show the bitset without "hiding" etc.
	auto aff_copy = ch->char_specials.saved.affected_by;
	for (auto j : hiding) {
		aff_copy.unset(j);
	}

	aff_copy.sprintbits(affected_bits, buf2, ",");
	snprintf(buf, kMaxStringLength, "Аффекты: %s%s%s\r\n", CCIYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);

	// Routine to show what spells a char is affected by
	if (!ch->affected.empty()) {
		for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i) {
			const auto aff = *affect_i;

			if (aff->type == ESpell::kSolobonus) {
				continue;
			}

			*buf2 = '\0';
			strcpy(sp_name, MUD::Spell(aff->type).GetCName());
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
					  GetDeclensionInNumber((mod + 1) / kSecsPerMudHour + 1, EWhat::kHour))
			: sprintf(buf2, "(менее часа)");
			snprintf(buf, kMaxStringLength, "%s%s%-21s %-12s%s ",
					 *sp_name == '!' ? "Состояние  : " : "Заклинание : ",
					 CCICYN(ch, C_NRM), sp_name, buf2, CCNRM(ch, C_NRM));
			*buf2 = '\0';
			if (!IS_IMMORTAL(ch)) {
				auto next_affect_i = affect_i;
				++next_affect_i;
				if (next_affect_i != ch->affected.end()) {
					const auto &next_affect = *next_affect_i;
					if (aff->type == next_affect->type) {
						continue;
					}
				}
			} else {
				if (aff->modifier) {
					sprintf(buf2, "%-3d к параметру: %s", aff->modifier, apply_types[(int) aff->location]);
					strcat(buf, buf2);
				}
				if (aff->bitvector) {
					if (*buf2) {
						strcat(buf, ", устанавливает ");
					} else {
						strcat(buf, "устанавливает ");
					}
					strcat(buf, CCIRED(ch, C_NRM));
					sprintbit(aff->bitvector, affected_bits, buf2);
					strcat(buf, buf2);
					strcat(buf, CCNRM(ch, C_NRM));
				}
			}
			SendMsgToChar(strcat(buf, "\r\n"), ch);
		}
// отображение наград
		for (const auto &aff : ch->affected) {
			if (aff->type == ESpell::kSolobonus) {
				int mod;
				if (aff->battleflag == kAfPulsedec) {
					mod = aff->duration / 51; //если в пульсах приводим к тикам	25.5 в сек 2 минуты
				} else {
					mod = aff->duration;
				}
				(mod + 1) / kSecsPerMudHour
				? sprintf(buf2,
						  "(%d %s)",
						  (mod + 1) / kSecsPerMudHour + 1,
						  GetDeclensionInNumber((mod + 1) / kSecsPerMudHour + 1, EWhat::kHour))
				: sprintf(buf2, "(менее часа)");
				snprintf(buf,
						 kMaxStringLength,
						 "Заклинание : %s%-21s %-12s%s ",
						 CCICYN(ch, C_NRM),
						 "награда",
						 buf2,
						 CCNRM(ch, C_NRM));
				*buf2 = '\0';
				if (aff->modifier) {
					sprintf(buf2, "%s%-3d к параметру: %s%s%s", (aff->modifier > 0) ? "+" : "",
							aff->modifier, CCIRED(ch, C_NRM), apply_types[(int) aff->location], CCNRM(ch, C_NRM));
					strcat(buf, buf2);
				}
				SendMsgToChar(strcat(buf, "\r\n"), ch);
			}
		}
	}

	if (ch->is_morphed()) {
		*buf2 = '\0';
		SendMsgToChar("Автоаффекты звериной формы: ", ch);
		const IMorph::affects_list_t &affs = ch->GetMorphAffects();
		for (auto it = affs.cbegin(); it != affs.cend();) {
			sprintbit(to_underlying(*it), affected_bits, buf2);
			SendMsgToChar(std::string(CCIYEL(ch, C_NRM)) + std::string(buf2) + std::string(CCNRM(ch, C_NRM)), ch);
			if (++it != affs.end()) {
				SendMsgToChar(", ", ch);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
