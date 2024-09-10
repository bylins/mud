// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "dps.h"

#include "color.h"
#include "entities/char_data.h"
#include "handler.h"

namespace DpsSystem {

// кол-во учитываемых чармисов с каждого чара
const unsigned MAX_DPS_CHARMICE = 5;

// * Добавление эффективной дамаги и овер-дамаги.
void DpsNode::add_dmg(int dmg, int over_dmg) {
	if (dmg >= 0 && over_dmg >= 0) {
		dmg_ += dmg;
		over_dmg_ += over_dmg;
		buf_dmg_ += dmg;
	}
}

/**
* Имя для вывода в статистике при отсутствии онлайн.
* Пока осталось актуальным только для чармисов.
*/
void DpsNode::set_name(const char *name) {
	if (name && *name) {
		name_ = name;
		if (name_.size() > 25) {
			name_ = name_.substr(0, 25);
		}
	}
}

// * Расчет дамаги в раунд.
int DpsNode::get_stat() const {
	if (rounds_) {
		return dmg_ / rounds_;
	}
	return 0;
}

unsigned DpsNode::get_dmg() const {
	return dmg_;
}

unsigned DpsNode::get_over_dmg() const {
	return over_dmg_;
}

const std::string &DpsNode::get_name() const {
	return name_;
}

// * Используется для идентификации в списках как чармисов, так и чаров.
long DpsNode::get_id() const {
	return id_;
}

void DpsNode::end_round() {
	if (round_dmg_ < buf_dmg_) {
		round_dmg_ = buf_dmg_;
	}
	buf_dmg_ = 0;
	++rounds_;
}

unsigned DpsNode::get_round_dmg() const {
	return round_dmg_;
}

////////////////////////////////////////////////////////////////////////////////
// Dps

void Dps::AddDmg(int type, CharData *ch, int dmg, int over_dmg) {
	switch (type) {
		case PERS_DPS: pers_dps_.add_dmg(dmg, over_dmg);
			break;
		case PERS_CHARM_DPS: pers_dps_.add_charm_dmg(ch, dmg, over_dmg);
			break;
		case GROUP_DPS: AddGroupDmg(ch, dmg, over_dmg);
			break;
		case GROUP_CHARM_DPS:
			if (ch && ch->has_master()) {
				AddGroupCharmDmg(ch, dmg, over_dmg);
			}
			break;
		default: log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
			return;
	}
}

void Dps::EndRound(int type, CharData *ch) {
	switch (type) {
		case PERS_DPS: pers_dps_.end_round();
			break;
		case PERS_CHARM_DPS: pers_dps_.end_charm_round(ch);
			break;
		case GROUP_DPS: EndGroupRound(ch);
			break;
		case GROUP_CHARM_DPS: EndGroupCharmRound(ch);
			break;
		default: log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
			return;
	}
}

// * Очистка себя или группы.
void Dps::Clear(int type) {
	switch (type) {
		case PERS_DPS: {
			PlayerDpsNode empty_dps;
			pers_dps_ = empty_dps;
			exp_ = 0;
			battle_exp_ = 0;
			lost_exp_ = 0;
			break;
		}
		case PERS_CHARM_DPS: break;
		case GROUP_DPS: group_dps_.clear();
			break;
		case GROUP_CHARM_DPS: break;
		default: log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
			return;
	}
}

// * Для сортировки вывода по нанесенной дамаге.
struct sort_node {
	sort_node(const std::string &in_name, int in_dps, unsigned in_round_dmg, unsigned in_over_dmg)
		: dps(in_dps), round_dmg(in_round_dmg), over_dmg(in_over_dmg) {
		name = in_name.substr(0, 25);
	};

	std::string name;
	int dps;
	unsigned round_dmg;
	unsigned over_dmg;
};

typedef std::multimap<unsigned /* dmg */, sort_node> SortGroupType;
SortGroupType tmp_group_list;
// суммарный дамаг группы при распечатке статистики
unsigned tmp_total_dmg = 0;

void Dps::AddTmpGroupList(CharData *ch) {
	auto it = group_dps_.find(GET_ID(ch));
	if (it != group_dps_.end()) {
		sort_node tmp_node(it->second.get_name(), it->second.get_stat(),
						   it->second.get_round_dmg(), it->second.get_over_dmg());
		tmp_group_list.insert(std::make_pair(it->second.get_dmg(), tmp_node));
		tmp_total_dmg += it->second.get_dmg();
		it->second.print_group_charm_stats(ch);
	}
}

/**
* Распечатка персональной статистики игрока и его чармисов.
* \param ch - игрок, которому идет распечатка.
* \param coder - случай вывода статистики другому персонажу, по умолчанию = 0
*/
void Dps::PrintStats(CharData *ch, CharData *coder) {
	if (!coder) {
		coder = ch;
	}

	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		tmp_total_dmg = 0;
		CharData *leader = ch->has_master() ? ch->get_master() : ch;
		leader->dps_print_group_stats(ch, coder);
	}

	std::ostringstream out;
	PrintPersonalDpsStat(ch, out);
	out << "\r\n";
	PrintPersonalExpStat(out);

	SendMsgToChar(out.str(), coder);
}

/**
 * Вывести в поток персональную статистику по урону.
 * @param ch - персонаж, для которого формируется статистика.
 * @param out - поток для вывода.
 */
void Dps::PrintPersonalDpsStat(CharData *ch, std::ostringstream &out) {
	out << " Персональная статистика урона:" << "\r\n";
	table_wrapper::Table table;
	table << table_wrapper::kHeader	<<
		"Имя" << "Урон" << "В раунд" << "Макс." << "Лишний урон" << table_wrapper::kEndRow;
	table << ch->get_name()
		  << pers_dps_.get_dmg()
		  << pers_dps_.get_stat()
		  << pers_dps_.get_round_dmg()
		  << pers_dps_.get_over_dmg() << table_wrapper::kEndRow;
	pers_dps_.print_charm_stats(table);

	table_wrapper::DecorateZebraTable(ch, table, table_wrapper::color::kBlue);
	table_wrapper::PrintTableToStream(out, table);
}

/**
 * Вывести в поток персональную статистику по полученному и потерянному опыту.
 * @param out - выводной поток.
 */
void Dps::PrintPersonalExpStat(std::ostringstream &out) const {
	out << " Персональная статистика по опыту:" << "\r\n";

	double percent = exp_ ? battle_exp_ * 100.0 / exp_ : 0.0;
	int balance = exp_ + lost_exp_;

	out << KIBLU << " *" << KNRM << " Всего получено опыта: " << PrintNumberByDigits(exp_)
		<< " Из него за удары: " << PrintNumberByDigits(battle_exp_)
		<< " (" << std::setprecision(2) << percent << "%)" << "\r\n"
		<< KIBLU << " *" << KNRM << " Потеряно опыта: " << PrintNumberByDigits(abs(lost_exp_));

	if (balance != 0) {
		out << " Баланс: " << (balance > 0 ? "+" : "-") << PrintNumberByDigits(abs(balance)) << "\r\n";
	}
}

/**
* Распечатка групповой статистики, находящейся у лидера группы.
* \param ch - игрок, которому идет распечатка.
* \param coder - случай вывода статистики другому персонажу, по умолчанию = nullptr
*/
void Dps::PrintGroupStats(CharData *ch, CharData *coder) {
	if (!coder) {
		coder = ch;
	}

	CharData *leader = ch->has_master() ? ch->get_master() : ch;
	for (FollowerType *f = leader->followers; f; f = f->next) {
		if (f->follower
			&& !f->follower->IsNpc()
			&& AFF_FLAGGED(f->follower, EAffect::kGroup)) {
			AddTmpGroupList(f->follower);
		}
	}
	AddTmpGroupList(leader);

	if (tmp_group_list.empty()) {
		return;
	}

	std::ostringstream out;
	out << " Статистика урона вашей группы:" << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader <<
		"Имя" << "Урон" << "(%)" << "В раунд" << "Макс." << "Лишний урон" << table_wrapper::kEndRow;
	for (auto it = tmp_group_list.rbegin(); it != tmp_group_list.rend(); ++it) {
		double percent = tmp_total_dmg ? it->first * 100.0 / tmp_total_dmg : 0.0;
		table << it->second.name
			<< (unsigned) it->first
			<< std::setprecision(2) << (unsigned) percent
			<< (unsigned) it->second.dps
			<< (unsigned) it->second.round_dmg
			<< (unsigned) it->second.over_dmg << table_wrapper::kEndRow;
	}
	tmp_group_list.clear();

	table_wrapper::DecorateZebraTable(ch, table, table_wrapper::color::kBlue);
	out << table.to_string() << "\r\n";
	SendMsgToChar(out.str(), coder);
}

void Dps::AddGroupDmg(CharData *ch, int dmg, int over_dmg) {
	auto it = group_dps_.find(GET_ID(ch));
	if (it != group_dps_.end()) {
		it->second.add_dmg(dmg, over_dmg);
	} else {
		PlayerDpsNode tmp_node;
		tmp_node.set_name(GET_NAME(ch));
		tmp_node.add_dmg(dmg, over_dmg);
		group_dps_.insert(std::make_pair(GET_ID(ch), tmp_node));
	}
}

void Dps::EndGroupRound(CharData *ch) {
	auto it = group_dps_.find(GET_ID(ch));
	if (it != group_dps_.end()) {
		it->second.end_round();
	} else {
		PlayerDpsNode tmp_node;
		tmp_node.set_name(GET_NAME(ch));
		tmp_node.end_round();
		group_dps_.insert(std::make_pair(GET_ID(ch), tmp_node));
	}
}

void Dps::AddGroupCharmDmg(CharData *ch, int dmg, int over_dmg) {
	auto it = group_dps_.find(GET_ID(ch->get_master()));
	if (it != group_dps_.end()) {
		it->second.add_charm_dmg(ch, dmg, over_dmg);
	} else {
		PlayerDpsNode tmp_node;
		tmp_node.set_name(GET_NAME(ch->get_master()));
		tmp_node.add_charm_dmg(ch, dmg, over_dmg);
		group_dps_.insert(std::make_pair(GET_ID(ch->get_master()), tmp_node));
	}
}

void Dps::EndGroupCharmRound(CharData *ch) {
	auto it = group_dps_.find(GET_ID(ch->get_master()));
	if (it != group_dps_.end()) {
		it->second.end_charm_round(ch);
	} else {
		PlayerDpsNode tmp_node;
		tmp_node.set_name(GET_NAME(ch->get_master()));
		tmp_node.end_charm_round(ch);
		group_dps_.insert(std::make_pair(GET_ID(ch->get_master()), tmp_node));
	}
}

// * Чтобы не морочить голову в dps_copy, заменяем только груп.статистику.
Dps &Dps::operator=(const Dps &copy) {
	if (this != &copy) {
		group_dps_ = copy.group_dps_;
	}
	return *this;
}

void Dps::AddExp(int exp) {
	if (exp >= 0) {
		exp_ += exp;
	} else {
		lost_exp_ += exp;
	}
}

void Dps::AddBattleExp(int exp) {
	battle_exp_ += exp;
}

// Dps
////////////////////////////////////////////////////////////////////////////////
// PlayerDpsNode

CharmListType::iterator PlayerDpsNode::find_charmice(CharData *ch) {
	auto i = std::find_if(charm_list_.begin(), charm_list_.end(),
											 [&](const DpsNode &x) {
												 return x.get_id() == GET_ID(ch);
											 });

	if (i != charm_list_.end()) {
		return i;
	}

	DpsNode tmp_node(GET_ID(ch));
	tmp_node.set_name(GET_NAME(ch));

	charm_list_.push_front(tmp_node);
	if (charm_list_.size() > MAX_DPS_CHARMICE) {
		charm_list_.pop_back();
	}
	return charm_list_.begin();
}

void PlayerDpsNode::add_charm_dmg(CharData *ch, int dmg, int over_dmg) {
	auto i = find_charmice(ch);
	if (i != charm_list_.end()) {
		i->add_dmg(dmg, over_dmg);
	} else {
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
	}
}

void PlayerDpsNode::end_charm_round(CharData *ch) {
	auto it = find_charmice(ch);
	if (it != charm_list_.end()) {
		it->end_round();
	} else {
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
	}
}

// * Чармисы в персональной статистике выводятся без сортировки.
void PlayerDpsNode::print_charm_stats(table_wrapper::Table &table) const {
	for (const auto & it : charm_list_) {
		if (it.get_dmg() > 0) {
			table
				<< it.get_name()
				<< it.get_dmg()
				<< it.get_stat()
				<< it.get_round_dmg()
				<< it.get_over_dmg() << table_wrapper::kEndRow;
		}
	}
}

// * Распечатка групповой статистики живых чармисов по данному игроку.
void PlayerDpsNode::print_group_charm_stats(CharData *ch) const {
	for (FollowerType *f = ch->followers; f; f = f->next) {
		if (!IS_CHARMICE(f->follower)) {
			continue;
		}
		const auto it = std::find_if(charm_list_.begin(), charm_list_.end(),
														[&](const DpsNode &dps_node) {
															return dps_node.get_id() == GET_ID(f->follower);
														});

		if (it != charm_list_.end() && it->get_dmg() > 0) {
			sort_node tmp_node(it->get_name(), it->get_stat(), it->get_round_dmg(), it->get_over_dmg());
			tmp_group_list.insert(std::make_pair(it->get_dmg(), tmp_node));
			tmp_total_dmg += it->get_dmg();
		}
	}
}

// PlayerDpsNode
////////////////////////////////////////////////////////////////////////////////

// * Подсчет дамаги за предыдущий раунд, дергается в начале раунда и по окончанию боя.
void check_round(CharData *ch) {
	if (!ch->IsNpc()) {
		ch->dps_end_round(DpsSystem::PERS_DPS);
		if (AFF_FLAGGED(ch, EAffect::kGroup)) {
			CharData *leader = ch->has_master() ? ch->get_master() : ch;
			leader->dps_end_round(DpsSystem::GROUP_DPS, ch);
		}
	} else if (IS_CHARMICE(ch) && ch->has_master()) {
		ch->get_master()->dps_end_round(DpsSystem::PERS_CHARM_DPS, ch);
		if (AFF_FLAGGED(ch->get_master(), EAffect::kGroup)) {
			CharData *leader = ch->get_master()->has_master() ? ch->get_master()->get_master() : ch->get_master();
			leader->dps_end_round(DpsSystem::GROUP_CHARM_DPS, ch);
		}
	}
}

} // namespace DpsSystem

namespace {

const char *DMETR_FORMAT =
	"Формат команды:\r\n"
	"дметр - вывод всей статистики\r\n"
	"дметр очистить - очистка персональной статистики\r\n"
	"дметр очистить группа - очистка групповой статистики (только лидер)\r\n";

} // namespace

/**
* 'дметр' - вывод своей статистики и групповой, если в группе.
* 'дметр очистить' - очистка своей статистики.
* 'дметр очистить группа' - очистка групповой статистики (только лидер).
*/
void do_dmeter(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}

	char name[kMaxInputLength];
	two_arguments(argument, arg, name);

	if (!*arg) {
		ch->dps_print_stats();
	} else if (isname(arg, "очистить")) {
		if (!*name) {
			ch->dps_clear(DpsSystem::PERS_DPS);
			SendMsgToChar("Персональная статистика очищена.\r\n", ch);
		} else if (isname(name, "группа")) {
			if (!AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar("Вы не состоите в группе.\r\n", ch);
				return;
			}
			if (ch->has_master()) {
				SendMsgToChar("Вы не являетесь лидером группы.\r\n", ch);
				return;
			}
			ch->dps_clear(DpsSystem::GROUP_DPS);
		}
	} else if (ch->IsFlagged(EPrf::kCoderinfo)) {
		// распечатка статистики указанного персонажа
		CharData *vict = get_player_vis(ch, arg, EFind::kCharInWorld);
		if (vict) {
			vict->dps_print_stats(ch);
		} else {
			SendMsgToChar("Нет такого персонажа.\r\n", ch);
		}
	} else {
		SendMsgToChar(DMETR_FORMAT, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
