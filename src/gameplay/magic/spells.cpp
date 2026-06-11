/* ************************************************************************
*   File: spells.cpp                                    Part of Bylins    *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "spells.h"
#include "engine/db/global_objects.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/remort.h"

int what_sky = kSkyCloudless;

//Определим мин уровень для изучения спелла из книги
//req_lvl - требуемый уровень из книги
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl) {
	int min_lvl = std::max(req_lvl, MUD::Class(ch->GetClass()).spells[spell_id].GetMinLevel())
		- (std::max(0, GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement()));

	return std::max(1, min_lvl);
}

int CalcMinSpellLvl(const CharData *ch, ESpell spell_id) {
	auto min_lvl = MUD::Class(ch->GetClass()).spells[spell_id].GetMinLevel()
		- GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement();

	return std::max(1, min_lvl);
}

int CalcMinRuneSpellLvl(const CharData *ch, ESpell spell_id) {
	int min_lvl;
	// Read from the new rune_spells registry.
	const auto &runes = MUD::RuneSpells();
	if (auto it = runes.find(spell_id); it != runes.end()) {
		min_lvl = it->second.min_caster_level - GetRealRemort(ch) / MUD::Class(ch->GetClass()).GetSpellLvlDecrement();
	} else {
		return 999;
	}
	return std::max(1, min_lvl);
}

bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl) {
	if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id) ||
		CalcMinSpellLvl(ch, spell_id, req_lvl) > GetRealLevel(ch) ||
		MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
		return false;
	}

	return true;
};

// Функция определяет возможность изучения спелла из книги или в гильдии
bool CanGetSpell(CharData *ch, ESpell spell_id) {
	if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id)) {
		return false;
	}

	if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
		MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
		return false;
	}

	return true;
};

void CheckAutoNosummon(CharData *ch) {
	if (ch->IsFlagged(EPrf::kAutonosummon) && ch->IsFlagged(EPrf::KSummonable)) {
		ch->UnsetFlag(EPrf::KSummonable);
		SendMsgToChar("Режим автопризыв: вы защищены от призыва.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
