#include "do_spells.h"

#include "color.h"
#include "entities/char_data.h"
#include "game_classes/classes_spell_slots.h"
#include "game_magic/magic_temp_spells.h"
#include "game_magic/spells_info.h"
#include "structs/global_objects.h"

void DoSpells(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;
	skip_spaces(&argument);
	if (utils::IsAbbrev(argument, "все") || utils::IsAbbrev(argument, "all"))
		DisplaySpells(ch, ch, true);
	else
		DisplaySpells(ch, ch, false);
}

const char *GetSpellColor(ESpell spell_id) {
	switch (MUD::Spell(spell_id).GetElement()) {
		case EElement::kAir: return "&W";
		case EElement::kFire: return "&R";
		case EElement::kWater: return "&C";
		case EElement::kEarth: return "&y";
		case EElement::kLight: return "&Y";
		case EElement::kDark: return "&K";
		case EElement::kMind: return "&M";
		case EElement::kLife: return "&G";
		default: return "&n";
	}
}

/* Параметр all введен для того чтобы предметные кастеры
   смогли посмотреть заклинания которые они могут колдовать
   на своем уровне, но на которые у них нет необходимых предметов
   при параметре true */
void DisplaySpells(CharData *ch, CharData *vict, bool all) {
	using classes::CalcCircleSlotsAmount;

	char names[kMaxMemoryCircle][kMaxStringLength];
	std::string time_str;
	int slots[kMaxMemoryCircle], i, max_slot = 0, slot_num, gcount = 0;
	auto can_cast{true};
	auto is_full{false};
	max_slot = 0;
	for (i = 0; i < kMaxMemoryCircle; i++) {
		*names[i] = '\0';
		slots[i] = 0;
	}

	for (const auto &spl_info : MUD::Spells()) {
		auto spell_id = spl_info.GetId();
		const auto &class_spell = MUD::Class(ch->GetClass()).spells[spell_id];
		if (!GET_SPELL_TYPE(ch, spell_id) && !all)
			continue;
		if (!IS_MANA_CASTER(ch) && !IS_GOD(ch) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
			if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp) && !all)
				continue;
		}
		if ((CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
			class_spell.GetMinRemort() > GET_REAL_REMORT(ch) ||
			CalcCircleSlotsAmount(ch, class_spell.GetCircle()) <= 0) &&
			all && !GET_SPELL_TYPE(ch, spell_id)) {
			continue;
		}

		if (MUD::Spell(spell_id).IsInvalid())
			continue;

		if ((GET_SPELL_TYPE(ch, spell_id) & 0xFF) == ESpellType::kRunes &&
			!CheckRecipeItems(ch, spell_id, ESpellType::kRunes, false)) {
			if (all) {
				can_cast = false;
			} else {
				continue;
			}
		} else {
			can_cast = true;
		}

		if (class_spell.GetMinRemort() > GET_REAL_REMORT(ch)) {
			slot_num = kMaxMemoryCircle - 1;
		} else {
			slot_num = class_spell.GetCircle() - 1;
		}
		max_slot = std::max(slot_num + 1, max_slot);
		if (IS_MANA_CASTER(ch)) {
			if (CalcSpellManacost(ch, spell_id) > GET_MAX_MANA(ch))
				continue;
			if (can_cast) {
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|<...%4d.> %s%-30s&n|",
										   slots[slot_num] % 114 <
											   10 ? "\r\n" : "  ",
										   CalcSpellManacost(ch, spell_id), GetSpellColor(spell_id), MUD::Spell(spell_id).GetCName());
			} else {
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|+--------+ %s%-30s&n|",
										   slots[slot_num] % 114 <
											   10 ? "\r\n" : "  ", GetSpellColor(spell_id), MUD::Spell(spell_id).GetCName());
			}
		} else {
			time_str.clear();
			if (IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp)) {
				time_str.append("[");
				time_str.append(std::to_string(MAX(1,
						static_cast<int>(std::ceil(static_cast<double>(temporary_spells::GetSpellLeftTime(ch, spell_id))
						/ kSecsPerMudHour)))));
				time_str.append("]");
			}
			if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) && IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow)) {
				sprintf(buf1, "%d", CalcMinSpellLvl(ch, spell_id) - GetRealLevel(ch));
			}
			else {
				sprintf(buf1, "%s", "K");
			}
			slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
					"%s|<%s%c%c%c%c%c%c%c>%s%s%-30s %-7s&n|",
					slots[slot_num] % 116 < 10 ? "\r\n" : "  ",
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow) ? buf1 : ".",
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp) ? 'T' : '.',
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kPotionCast) ? 'P' : '.',
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kWandCast) ? 'W' : '.',
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kScrollCast) ? 'S' : '.',
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kItemCast) ? 'I' : '.',
					IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kRunes) ? 'R' : '.',
					 '.',
					(CalcMinSpellLvl(ch, spell_id) - GetRealLevel(ch) < 10) ? "  " : " ",
					GetSpellColor(spell_id),
					MUD::Spell(spell_id).GetCName(),
					time_str.c_str());
		}
		is_full = true;
	};
	gcount = sprintf(buf2 + gcount, "  %sВам доступна следующая магия :%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	if (is_full) {
		for (i = 0; i < max_slot; i++) {
			if (slots[i] != 0) {
				if (!IS_MANA_CASTER(ch))
					gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
			}
			if (slots[i])
				gcount += sprintf(buf2 + gcount, "%s", names[i]);
			//else
			//gcount += sprintf(buf2+gcount,"\n\rПусто.");
		}
	} else
		gcount += sprintf(buf2 + gcount, "\r\nВ настоящее время магия вам недоступна!");
	gcount += sprintf(buf2 + gcount, "\r\n");
	//page_string(ch->desc, buf2, 1);
	SendMsgToChar(buf2, vict);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
