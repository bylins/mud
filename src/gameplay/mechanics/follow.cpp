// issue.chardata-cleaning: the character-following mechanic (logic moved off CharData).

#include "follow.h"

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/handler.h"
#include "utils/logger.h"
#include "utils/backtrace.h"

#include <fmt/format.h>
#include <sstream>

namespace follow {

bool MakesLoop(CharData *ch, CharData *master) {
	while (master) {
		if (master == ch) {
			return true;
		}
		master = master->get_master();
	}
	return false;
}

void PrintLeadersChain(const CharData *ch, std::ostream &ss) {
	if (!ch->has_master()) {
		ss << "<пуста>";
		return;
	}

	bool first = true;
	for (auto master = ch->get_master(); master; master = master->get_master()) {
		ss << (first ? "" : " -> ") << "[" << master->get_name() << "]";
		first = false;
	}
}

void ReportLoopError(CharData *ch, CharData *master) {
	std::stringstream ss;
	ss << "Обнаружена ошибка логики: попытка сделать цикл в цепочке последователей.\nТекущая цепочка лидеров: ";
	PrintLeadersChain(master, ss);
	ss << "\nПопытка сделать персонажа [" << master->get_name() << "] лидером персонажа [" << ch->get_name() << "]";
	mudlog(ss.str().c_str(), DEF, -1, ERRLOG, true);

	std::stringstream additional_info;
	additional_info << "Потенциальный лидер: name=[" << master->get_name() << "]"
					<< "; адрес структуры: " << master << "; текущий лидер: ";
	if (master->has_master()) {
		additional_info << "name=[" << master->get_master()->get_name() << "]"
						<< "; адрес структуры: " << master->get_master() << "";
	} else {
		additional_info << "<отсутствует>";
	}
	additional_info << "\nПоследователь: name=[" << ch->get_name() << "]"
					<< "; адрес структуры: " << ch << "; текущий лидер: ";
	if (ch->has_master()) {
		additional_info << "name=[" << ch->get_master()->get_name() << "]"
						<< "; адрес структуры: " << ch->get_master() << "";
	} else {
		additional_info << "<отсутствует>";
	}
	mudlog(additional_info.str().c_str(), DEF, -1, ERRLOG, true);

	ss << "\nТекущий стек будет распечатан в SYSLOG.";
	debug::backtrace(runtime_config.logs(SYSLOG).handle());
	mudlog(ss.str().c_str(), LGH, kLvlImmortal, SYSLOG, false);
}

bool IsLeader(CharData *ch) {
	if (ch->IsNpc()) {
		return false;
	}
	if (ch->get_master() != ch) {
		return false;
	}
	for (auto *f : ch->followers) {
		if (!f->IsNpc()) {
			return true;
		}
	}
	return false;
}

void AddFollowerSilently(CharData *leader, CharData *ch) {
	if (ch->has_master()) {
		log("SYSERR: add_follower_implementation(%s->%s) when master existing(%s)...",
			GET_NAME(ch), leader->get_name().c_str(), GET_NAME(ch->get_master()));
		return;
	}

	if (ch == leader) {
		return;
	}

	ch->set_master(leader);

	leader->followers.push_front(ch);
}

void AddFollower(CharData *leader, CharData *ch) {
	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoGroup))
		return;
	if (leader->in_room != ch->in_room) {
		log(fmt::format("попытка загрупить игроков в разных комнатах, лидер {} #{} фолловер {} #{}",
				leader->get_name(), GET_MOB_VNUM(leader), ch->get_name(), GET_MOB_VNUM(ch)));
		mudlog(fmt::format("попытка загрупить игроков в разных комнатах, лидер {} #{} фолловер {} #{}",
				leader->get_name(), GET_MOB_VNUM(leader), ch->get_name(), GET_MOB_VNUM(ch)));
		return;
	}
	AddFollowerSilently(leader, ch);

	if (!mount::IsHorse(ch)) {
		act("Вы начали следовать за $N4.", false, ch, 0, leader, kToChar);
		act("$n начал$g следовать за вами.", true, ch, 0, leader, kToVict);
		act("$n начал$g следовать за $N4.", true, ch, 0, leader, kToNotVict | kToArenaListen);
	}
}

}  // namespace follow

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
