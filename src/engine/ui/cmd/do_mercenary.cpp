//#include "engine/ui/cmd/mercenary.h"
#include "administration/privilege.h"

#include "engine/core/char_handler.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "gameplay/economics/currencies.h"
#include "utils/grammar/declensions.h"
#include "engine/entities/char_player.h"
#include "engine/ui/modify.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/communication/talk.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/ai/special_messages.h"
#include "gameplay/core/remort.h"

#include <fmt/format.h>

int do_social(CharData *ch, char *argument);

namespace MERC {
const int BASE_COST = 1000;
CharData *findMercboss(int room_rnum) {
	for (const auto tch : world[room_rnum]->people)
		if (specials::IsMobSpecial(GET_MOB_VNUM(tch), specials::ESpecial::kMercenary))
			return tch;
	sprintf(buf1, "[ERROR] MERC::doList - вызвана команда, не найден моб, room %d", room_rnum);
	mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
	return nullptr;
};

void doList(CharData *ch, CharData *boss, bool isFavList) {
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	if (m->empty()) {
		if (CanUseFeat(ch, EFeat::kEmployer))
			tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kEmptyEmployer).c_str());
		else if (IS_SPELL_SET(ch, ESpell::kCharm, ESpellType::kKnow))
			tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kEmptyCharmer).c_str());
		else if (privilege::IsImmortal(ch))
			tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kEmptyImmortal).c_str());
		return;
	}
	if (privilege::IsImmortal(ch)) {
		tell_to_char(boss, ch, specials::MercMsg(isFavList ? specials::EMercMsg::kListImmortalShort : specials::EMercMsg::kListImmortalFull).c_str());
	} else if (CanUseFeat(ch, EFeat::kEmployer)) {
		tell_to_char(boss, ch, specials::MercMsg(isFavList ? specials::EMercMsg::kListEmployerShort : specials::EMercMsg::kListEmployerFull).c_str());
	} else if (IS_SPELL_SET(ch, ESpell::kCharm, ESpellType::kKnow)) {
		tell_to_char(boss, ch, specials::MercMsg(isFavList ? specials::EMercMsg::kListCharmerShort : specials::EMercMsg::kListCharmerFull).c_str());
	}
	SendMsgToChar(specials::MercMsg(specials::EMercMsg::kTableHeader) + "\r\n"
				  "------------------------------------------------------------\r\n", ch);
	auto it = m->begin();
	std::stringstream out;
	std::string mobname;
	for (int num = 1; it != m->end(); ++it, ++num) {
		if (it->second.currRemortAvail == 0)
			continue;
		if (isFavList && it->second.isFavorite == 0)
			continue;
		mobname = mob_stat::PrintMobName(it->first, 54);
		out << fmt::format("{:<3})  {:<54.54}\r\n", num, mobname);
	}
	page_string(ch->desc, out.str());
	SendMsgToChar(ch, "------------------------------------------------------------\r\n");
	tell_to_char(boss, ch, fmt::format(fmt::runtime(specials::MercMsg(specials::EMercMsg::kListTotal)),
			fmt::arg("amount", 1000 * (remort::GetRealRemort(ch) + 1))).c_str());
	snprintf(buf, kMaxInputLength, "ухмы %s", GET_NAME(ch));
	do_social(boss, buf);
};

void doStat(CharData *ch) {
	if (!ch) return;
};

void doBring(CharData *ch, CharData *boss, unsigned int pos, char *bank) {
	CharData *mob;
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	const int cost = MERC::BASE_COST * (remort::GetRealRemort(ch) + 1);
	MobRnum rnum;
	//std::map<int, MERCDATA>::iterator
	auto it = m->begin();
	for (unsigned int num = 1; it != m->end(); ++it, ++num) {
		if (num != pos)
			continue;
		if (it->second.currRemortAvail == 0) {
			tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kUnknownChar).c_str());
			return;
		}

		if ((!isname(bank, "банк bank") && cost > currencies::GetHand(*ch, currencies::kGold)) ||
			(isname(bank, "банк bank") && cost > currencies::GetBank(*ch, currencies::kGold))) {
			tell_to_char(boss, ch, fmt::format(fmt::runtime(specials::MercMsg(specials::EMercMsg::kTooExpensive)),
					fmt::arg("amount", cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str())).c_str());
			return;
		}

		if ((rnum = GetMobRnum(it->first)) < 0) {
			tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kCantFind).c_str());
			sprintf(buf1, "[ERROR] MERC::doBring, не найден моб, vnum: %d", it->first);
			mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
			return;
		}
		mob = ReadMobile(rnum, kReal);
		PlaceCharToRoom(mob, ch->in_room);
		if (IS_SPELL_SET(ch, ESpell::kCharm, ESpellType::kKnow)) {
			act(specials::MercMsg(specials::EMercMsg::kSummonHideCharm), true, boss, nullptr, nullptr, kToRoom);
			act(specials::MercMsg(specials::EMercMsg::kSummonBackCharm), true, mob, nullptr, nullptr, kToRoom);
		} else {
			act(specials::MercMsg(specials::EMercMsg::kSummonHide), true, boss, nullptr, nullptr, kToRoom);
			act(fmt::format(fmt::runtime(specials::MercMsg(specials::EMercMsg::kSummonBack)),
					fmt::arg("boss", boss->get_npc_name())), true, mob, nullptr, ch, kToRoom);
		}
		if (!privilege::IsImmortal(ch)) {
			if (isname(bank, "банк bank"))
				currencies::RemoveBank(*ch, currencies::kGold, cost);
			else
				currencies::RemoveHand(*ch, currencies::kGold, cost);
		}
		if (NPC_FLAGGED(mob, ENpcFlag::kNoMercList)) {
			if (mob->get_sex() == EGender::kMale)
				SendMsgToChar(fmt::format(fmt::runtime(specials::MercMsg(specials::EMercMsg::kRefuseMale)), fmt::arg("mob", mob->get_npc_name())) + "\r\n", ch);
			else
				SendMsgToChar(fmt::format(fmt::runtime(specials::MercMsg(specials::EMercMsg::kRefuseFemale)), fmt::arg("mob", mob->get_npc_name())) + "\r\n", ch);
			ExtractCharFromWorld(mob, false);
		}
	}
};

void doForget(CharData *ch, CharData *boss, unsigned int pos) {
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	auto it = m->begin();
	for (unsigned int num = 1; it != m->end(); ++it, ++num) {
		if (num != pos)
			continue;
		if (it->second.currRemortAvail == 0) {
			tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kUnknownChar).c_str());
			return;
		}
		it->second.isFavorite = !it->second.isFavorite;
		tell_to_char(boss, ch, fmt::format(fmt::runtime(specials::MercMsg(it->second.isFavorite
				? specials::EMercMsg::kFavAdded : specials::EMercMsg::kFavRemoved)),
				fmt::arg("mob", mob_stat::PrintMobName(it->first, 54))).c_str());
		return;
	}
	sprintf(buf1, "[ERROR] MERC::doForget, не найден моб, pos: %d", pos);
	mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
};

unsigned int getPos(char *arg, CharData *ch, CharData *boss) {
	unsigned int pos = 0;
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	try {
		pos = std::stoi(arg);
	} catch (const std::invalid_argument &) {
		pos = 0;
	}
	if (pos < 1 || pos > m->size() || m->empty()) {
		tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kUnknownChar).c_str());
		return 0;
	}
	return pos;
}

}

int mercenary(CharData *ch, void * /*me*/, int /*cmd*/, char *argument) {
	if (!ch || !ch->desc || ch->IsNpc())
		return 0;

	CharData *boss = MERC::findMercboss(ch->in_room);
	if (!boss) return 0;

	if (!privilege::IsImmortal(ch) && !CanUseFeat(ch, EFeat::kEmployer) && ch->GetClass() != ECharClass::kCharmer) {
		tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kNoAccess).c_str());
		return 0;
	}

	char subCmd[kMaxInputLength];
	char cmdParam[kMaxInputLength];
	char bank[kMaxInputLength];
	unsigned int pos;

	three_arguments(argument, subCmd, cmdParam, bank);
	if (utils::IsAbbr(subCmd, "стат") || utils::IsAbbr(subCmd, "stat")) {
		return (1);
	} else if (utils::IsAbbr(subCmd, "список") || utils::IsAbbr(subCmd, "list")) {
		if (utils::IsAbbr(cmdParam, "полный") || utils::IsAbbr(cmdParam, "full"))
			MERC::doList(ch, boss, false);
		else
			MERC::doList(ch, boss, true);
		return (1);
	} else if (utils::IsAbbr(subCmd, "привести") || utils::IsAbbr(subCmd, "bring")) {
		pos = MERC::getPos(cmdParam, ch, boss);
		if (pos == 0) return (1);
		MERC::doBring(ch, boss, pos, bank);
		return (1);
	} else if (utils::IsAbbr(subCmd, "фаворит") || utils::IsAbbr(subCmd, "favorite")) {
		pos = MERC::getPos(cmdParam, ch, boss);
		if (pos == 0) return (1);
		MERC::doForget(ch, boss, pos);
		return (1);
	} else
		tell_to_char(boss, ch, specials::MercMsg(specials::EMercMsg::kUnknownCmd).c_str());
	return (1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

