/**
\file spell_usage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "spell_usage.h"

#include "engine/boot/boot_data_files.h"
#include "engine/db/global_objects.h"

namespace SpellUsage {
bool is_active = false;
std::map<ECharClass, SpellCountType> usage;
const char *SPELL_STAT_FILE = LIB_STAT"spellstat.txt";
time_t start;
}

void SpellUsage::Clear() {
	for (auto & it : usage) {
		it.second.clear();
	}
	usage.clear();
	start = time(nullptr);
}

std::string SpellUsage::StatToPrint() {
	std::stringstream out;
	time_t now = time(nullptr);
	char *end_time = str_dup(rustime(localtime(&now)));
	out << rustime(localtime(&SpellUsage::start)) << " - " << end_time << "\n";
	for (auto & it : SpellUsage::usage) {
		out << std::setw(35) << MUD::Class(it.first).GetName() << "\r\n";
		for (auto & itt : it.second) {
			out << std::setw(25) << MUD::Spell(itt.first).GetName() << " : " << itt.second << "\r\n";
		}
	}
	return out.str();
}

void SpellUsage::Save() {
	if (!is_active)
		return;

	std::ofstream file(SPELL_STAT_FILE, std::ios_base::app | std::ios_base::out);

	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", SPELL_STAT_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	file << StatToPrint();
	file.close();
}

void SpellUsage::AddSpellStat(ECharClass char_class, ESpell spell_id) {
	if (!is_active) {
		return;
	}
	if (MUD::Classes().IsUnavailable(char_class) || spell_id > ESpell::kLast) {
		return;
	}
	++usage[char_class][spell_id];
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
