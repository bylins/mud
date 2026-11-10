#include "obj_prototypes.h"
#include "utils/logger.h"
#include "utils/backtrace.h"

#include <fmt/format.h>

#include <unordered_set>

size_t CObjectPrototypes::add(CObjectPrototype *prototype, const ObjVnum vnum) {
	return add(CObjectPrototype::shared_ptr(prototype, [](auto ptr) { delete (ObjData *) ptr; }), vnum);
}

size_t CObjectPrototypes::add(const CObjectPrototype::shared_ptr &prototype, const ObjVnum vnum) {
	const auto index = m_index.size();
	prototype->set_rnum(static_cast<int>(index));
	m_vnum2index[vnum] = index;
	m_prototypes.push_back(prototype);
	m_index.push_back(SPrototypeIndex());
	return index;
}

int CObjectPrototypes::stored(const size_t rnum) const {
	auto orn  = obj_proto[rnum]->get_parent_rnum();

	if (!is_index_safe(rnum)) {
		return -1;
	}
	if (orn != -1) {
		return m_index[orn].stored;
	} else {
		return m_index[rnum].stored;
	}
}

int CObjectPrototypes::total_online(const size_t rnum) const {
	if (is_index_safe(rnum)) {
		ObjRnum orn = obj_proto[rnum]->get_parent_rnum();

		if (orn != -1) {
			if (!CAN_WEAR(obj_proto[rnum].get(), EWearFlag::kTake) || obj_proto[rnum].get()->has_flag(EObjFlag::kQuestItem))
				return m_index[rnum].total_online;
			else
				return m_index[orn].total_online;
		} else {
			return m_index[rnum].total_online;
		}
	}
	return -1;
}

void CObjectPrototypes::inc_number(const size_t rnum) {
	auto orn  = obj_proto[rnum]->get_parent_rnum();

	if (orn != -1) {
		if (!CAN_WEAR(obj_proto[rnum].get(), EWearFlag::kTake) || obj_proto[rnum].get()->has_flag(EObjFlag::kQuestItem)) {
			++m_index[rnum].total_online;
		} else {
			++m_index[orn].total_online;
		}
	} else {
		++m_index[rnum].total_online;
	}
}

void CObjectPrototypes::dec_number(const size_t rnum) {
	auto orn  = obj_proto[rnum]->get_parent_rnum();

	if (orn != -1) {
		if (!CAN_WEAR(obj_proto[rnum].get(), EWearFlag::kTake) || obj_proto[rnum].get()->has_flag(EObjFlag::kQuestItem)) {
			if (0 == m_index[rnum].total_online) {
				debug::backtrace(runtime_config.logs(SYSLOG).handle());
				mudlog(fmt::format("SYSERR: Attempt to decrement number of dungeon objects (vnum {}) that does not exist at all (0 == number).", obj_proto[rnum]->get_vnum()),
						CMP, kLvlGreatGod, SYSLOG, true);
				return;
			}
			--m_index[rnum].total_online;
		} else {
			if (0 == m_index[orn].total_online) {
				debug::backtrace(runtime_config.logs(SYSLOG).handle());
				mudlog(fmt::format("SYSERR: Attempt to decrement number of parent objects (vnum {}) that does not exist at all (0 == number).", obj_proto[rnum]->get_vnum()),
						CMP, kLvlGreatGod, SYSLOG, true);
				return;
			}
			--m_index[orn].total_online;
		}
	} else {
		if (0 == m_index[rnum].total_online) {
			debug::backtrace(runtime_config.logs(SYSLOG).handle());
			mudlog(fmt::format("SYSERR: Attempt to decrement number of real objects (vnum {}) that does not exist at all (0 == number).", obj_proto[rnum]->get_vnum()),
					CMP, kLvlGreatGod, SYSLOG, true);
			return;
		}
		--m_index[rnum].total_online;
	}
}

int CObjectPrototypes::get_rnum(const ObjVnum vnum) const {
	vnum2index_t::const_iterator i = m_vnum2index.find(vnum);
	return i == m_vnum2index.end() ? -1 : static_cast<int>(i->second);
}

void CObjectPrototypes::set_rnum(const size_t index, CObjectPrototype *new_value) {
	new_value->set_rnum(static_cast<int>(index));
	m_prototypes[index].reset(new_value);
}

CObjectPrototypes obj_proto;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

// Раньше здесь печаталось просто "неизвестный прототип объекта" -- ни номера, ни того,
// кто спросил, так что по логу нельзя было понять ровно ничего. Причина почти всегда
// одна: GetObjRnum() не нашел внум и вернул -1, а вызывающий это не проверил, и -1
// пришел сюда как огромный size_t. Такое бывает, когда внум из данных игрока или клана
// больше не существует -- например после переноса зоны на новый номер.
//
// Печатаем и номер, и стек -- тем же приемом, что поиск предмета (spell_locate_object).
// Стек по каждому номеру печатаем однажды: в цикле по хранилищу жалоб бывает много, и
// лог забьется.
void CObjectPrototypes::ReportBadIndex(std::size_t index, std::size_t size) {
	static std::unordered_set<std::size_t> already_printed;

	const auto as_signed = static_cast<std::ptrdiff_t>(index);
	const bool looks_like_minus_one = as_signed < 0;
	const auto first_time = already_printed.insert(index).second;

	mudlog(fmt::format("SYSERR: неизвестный прототип объекта: рнум {}{}, прототипов всего {}.{}",
					   index,
					   looks_like_minus_one
						   ? fmt::format(" (как знаковое: {} -- похоже, непроверенный GetObjRnum)", as_signed)
						   : "",
					   size,
					   first_time ? " Стек будет распечатан ниже" : ""),
		   BRF, kLvlBuilder, SYSLOG, true);

	if (first_time) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
	}
}
