// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include "privilege.h"

#include "utils/logger.h"
#include "engine/entities/char_data.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/db/player_index.h"
#include "privilege_db.h"

#include <cstdio>

// issue.misc-migrate: the legacy level-based / misc/privilege.lst system has been removed.
// Privileges now come solely from the membership DB (cfg/privilege.xml -> privilege_db.*);
// the public API below dispatches to the Modern* helpers in the anonymous namespace.

namespace privilege {

const int kBoards = 0;
const int kUseSkills = 1;
const int kArenaMaster = 2;
const int kKroder = 3;
const int kFullzedit = 4;
const int kTitle = 5;
// чтение/удаление доски опечаток, ставился по наличию группы olc
const int kMisprint = 6;
// чтение/удаление доски придумок
const int kSuggest = 7;

// ===================== modern membership privilege system (issue.privilege-rework P2) =====================
// Decisions come from the cfg/privilege.xml membership DB (privilege_db.h), keyed
// by name+uid; character level grants nothing. "default", "default_demigod" and "arena" stay hardcoded
// here (point 7): default/default_demigod are auto-applied by tier; arena commands work only on arena tiles.
namespace {

const char *kDefaultCommands =
	"wizhelp|гбогам|wiznet|register|имя|титул|title|holylight|uptime|date|invis|rules|nohassle|show (punishment stats)";
const char *kDefaultDemigodCommands =
	"wizhelp|гбогам|wiznet|имя|титул|title|rules|date|uptime|сдемигодам|set (палач)";
const char *kArenaCommands = "purge|restore|arenarestore|goto|teleport";

struct CmdSet { std::set<std::string> cmds, set_subs, show_subs; };
CmdSet ParseSet(const std::string &raw) { CmdSet s; ParseCommandList(raw, s.cmds, s.set_subs, s.show_subs); return s; }
const CmdSet &DefaultSet() { static const CmdSet s = ParseSet(kDefaultCommands); return s; }
const CmdSet &DefaultDemigodSet() { static const CmdSet s = ParseSet(kDefaultDemigodCommands); return s; }
const CmdSet &ArenaSet() { static const CmdSet s = ParseSet(kArenaCommands); return s; }

int TierRank(EGodTier tier) {
	switch (tier) {
		case EGodTier::kOwner: return 0;
		case EGodTier::kImplementator: return 1;
		case EGodTier::kGreatGod: return 2;
		case EGodTier::kGod: return 3;
		case EGodTier::kImmortal: return 4;
		case EGodTier::kDemigod: return 5;
		default: return 99;
	}
}

bool OwnerByName(const CharData *ch) {
	return ch && !ch->IsNpc() && CompareParam(std::string("Стрибог"), GET_NAME(ch), true);
}

const GodEntry *FindGod(const CharData *ch) {
	if (!ch || ch->IsNpc()) return nullptr;
	const auto *e = GetDb().FindByUid(ch->get_uid());
	if (e && CompareParam(e->name, GET_NAME(ch), true)) return e;
	return nullptr;
}

bool AtLeastTier(const CharData *ch, EGodTier min) {
#ifdef TEST_BUILD
	int need = (min == EGodTier::kImplementator) ? kLvlImplementator
			 : (min == EGodTier::kGreatGod) ? kLvlGreatGod
			 : (min == EGodTier::kGod) ? kLvlGod : kLvlImmortal;
	return ch && !ch->IsNpc() && ch->GetLevel() >= need;
#else
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	return e && TierRank(e->tier) <= TierRank(min);
#endif
}

CmdSet EffectiveSet(const GodEntry *e) {
	CmdSet s; s.cmds = e->commands; s.set_subs = e->set_subs; s.show_subs = e->show_subs;
	for (const auto &gid : e->groups) {
		const auto &gm = GetDb().groups();
		auto it = gm.find(gid);
		if (it != gm.end()) ParseCommandList(it->second, s.cmds, s.set_subs, s.show_subs);
	}
	const CmdSet *def = (e->tier == EGodTier::kDemigod) ? &DefaultDemigodSet()
					 : (TierRank(e->tier) <= TierRank(EGodTier::kImmortal)) ? &DefaultSet() : nullptr;
	if (def) {
		s.cmds.insert(def->cmds.begin(), def->cmds.end());
		s.set_subs.insert(def->set_subs.begin(), def->set_subs.end());
		s.show_subs.insert(def->show_subs.begin(), def->show_subs.end());
	}
	return s;
}

std::string FlagToken(int flag) {
	switch (flag) {
		case kBoards: return "boards";
		case kUseSkills: return "skills";
		case kArenaMaster: return "arena";
		case kKroder: return "kroder";
		case kFullzedit: return "fullzedit";
		case kTitle: return "title";
		case kMisprint: return "misprint";
		case kSuggest: return "suggest";
		default: return "";
	}
}

bool ModernIsOwner(const CharData *ch) { return OwnerByName(ch); }

bool ModernCanEditVedun(const CharData *ch, const std::string &what) {
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	if (!e) return false;
	if (e->flags.count("FullAccess")) return true;
	if (e->vedun.count("*") || e->vedun.count("all")) return true;  // wildcard = any data set
	return e->vedun.count(what) > 0;
}

bool ModernHasPrivilege(CharData *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level) {
	if (check_level && !mode && cmd_info[cmd_number].minimum_level < kLvlImmortal
		&& GetRealLevel(ch) >= cmd_info[cmd_number].minimum_level) {
		return true;
	}
	if (ch->IsNpc()) return false;
#ifdef TEST_BUILD
	return true;
#endif
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	if (!e) {
		if (cmd_info[cmd_number].minimum_level >= kLvlImmortal && GetRealLevel(ch) >= kLvlImmortal) {
			char log_buf[256];
			snprintf(log_buf, sizeof(log_buf),
				"PRIVILEGE: %s (level %d) tried privileged command '%s' but is not in privilege.xml.",
				GET_NAME(ch), GetRealLevel(ch), cmd_name.c_str());
			mudlog(log_buf, DEF, kLvlGod, SYSLOG, true);
		}
		return false;
	}
	if (e->flags.count("FullAccess")) return true;
	const CmdSet eff = EffectiveSet(e);
	switch (mode) {
		case 1: return eff.set_subs.count(cmd_name) > 0;
		case 2: return eff.show_subs.count(cmd_name) > 0;
		default:
			if (eff.cmds.count(cmd_name)) return true;
			if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) && ArenaSet().cmds.count(cmd_name)) return true;
			return false;
	}
}

bool ModernCheckFlag(const CharData *ch, int flag) {
	if (OwnerByName(ch)) return true;
	const auto *e = FindGod(ch);
	if (!e) return false;
	if (e->flags.count("FullAccess")) return true;
	const std::string tok = FlagToken(flag);
	if (tok.empty()) return false;
	if (e->flags.count(tok)) return true;
	if (tok == "boards" && e->tier == EGodTier::kDemigod) return true;  // default_demigod grants boards
	return false;
}

bool ModernIsContainedInGodsList(const std::string &name, long unique) {
#ifdef TEST_BUILD
	return true;
#endif
	if (CompareParam(std::string("Стрибог"), name, true)) return true;
	const auto *e = GetDb().FindByUid(unique);
	return e && CompareParam(e->name, name, true);
}

void ModernLoadGodBoards() {
	Boards::Static::clear_god_boards();
	for (const auto &pair : GetDb().entries()) {
		if (TierRank(pair.second.tier) <= TierRank(EGodTier::kImmortal))
			Boards::Static::init_god_board(pair.first, pair.second.name);
	}
}

}  // namespace

bool IsContainedInGodsList(const std::string &name, long unique) {
	return ModernIsContainedInGodsList(name, unique);
}

// * Создание и лоад/релоад блокнотов иммам.
void LoadGodBoards() {
	ModernLoadGodBoards();
}

/**
* Проверка на возможность использования команды (для команд с левелом 31+). 34е используют без ограничений.
* При сборке через make test или под студией поиск по привилегиям не производится.
* \param mode 0 - общие команды, 1 - подкоманды set, 2 - подкоманды show
* \return 0 - нельзя, 1 - можно
*/
bool HasPrivilege(CharData *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level) {
	return ModernHasPrivilege(ch, cmd_name, cmd_number, mode, check_level);
}

/**
* Проверка флагов. 34м автоматически присваивается группа skills
* для более удобного вызова, например при использовании рун.
* \param flag - один из privilege::k* флагов, объявленных в начале файла
* \return 0 - не нашли, 1 - нашли
*/
bool CheckFlag(const CharData *ch, int flag) {
	return ModernCheckFlag(ch, flag);
}

/**
* Проверка на возможность каста заклинания иммом.
* Группа skills без ограничений. Группа arena только призыв, пента и слово возврата и только на клетках арены.
* У морталов и 34х проверка не производится.
*/
bool IsSpellPermit(const CharData *ch, ESpell spell_id) {
	if (!privilege::IsImmortal(ch) || privilege::IsImpl(ch) || CheckFlag(ch, kUseSkills)) {
		return true;
	}
	if (spell_id == ESpell::kPortal || spell_id == ESpell::kSummon || spell_id == ESpell::kWorldOfRecall) {
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) && CheckFlag(ch, kArenaMaster)) {
			return true;
		}
	}
	return false;
}

/**
* Проверка на возможность использования скилла. Вызов через get_skill.
* У морталов, мобов и 34х проверка не производится.
* \return 0 - не может использовать скиллы, 1 - может
*/
bool CheckSkills(const CharData *ch) {
	if (privilege::IsGrGod(ch) || !privilege::IsImmortal(ch) || CheckFlag(ch, kUseSkills))
//	if (!privilege::IsImmortal(ch) || privilege::IsImpl(ch) || check_flag(ch, USE_SKILLS))
		return true;
	return false;
}

bool IsImmortal(const CharData *ch) { return AtLeastTier(ch, EGodTier::kImmortal); }
bool IsGod(const CharData *ch) { return AtLeastTier(ch, EGodTier::kGod); }
bool IsGrGod(const CharData *ch) { return AtLeastTier(ch, EGodTier::kGreatGod); }
bool IsImpl(const CharData *ch) { return AtLeastTier(ch, EGodTier::kImplementator); }

bool IsOwner(const CharData *ch) { return ModernIsOwner(ch); }

bool CanEditVedun(const CharData *ch, const std::string &what) {
	return ModernCanEditVedun(ch, what);
}

} // namespace Privilege

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
