#include "obj_decay_manager.h"

#include "engine/entities/obj_data.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/stable_objs.h"

void ObjDecayManager::insert(ObjData *obj) {
	if (!obj) {
		return;
	}
	if (m_obj_to_deadline.count(obj)) {
		on_timer_changed(obj);
		return;
	}

	if (obj->has_flag(EObjFlag::kZonedecay)) {
		m_zonedecay_objs.insert(obj);
	}

	compute_and_store_deadline(obj);
}

void ObjDecayManager::remove(ObjData *obj) {
	if (!obj) {
		return;
	}
	remove_queue_entry(obj);
	m_obj_to_deadline.erase(obj);
	m_env_check_objs.erase(obj);
	m_timed_spell_objs.erase(obj);
	m_zonedecay_objs.erase(obj);
}

void ObjDecayManager::on_timer_changed(ObjData *obj) {
	if (!obj || !m_obj_to_deadline.count(obj)) {
		return;
	}

	remove_queue_entry(obj);

	if (obj->has_flag(EObjFlag::kZonedecay)) {
		m_zonedecay_objs.insert(obj);
	}

	compute_and_store_deadline(obj);
}

void ObjDecayManager::compute_and_store_deadline(ObjData *obj) {
	// Use CObjectPrototype::get_timer() to read raw m_timer value, bypassing
	// ObjData::get_timer() which is computed from the deadline we are about
	// to set.
	const auto timer = obj->CObjectPrototype::get_timer();

	// kZonedecay без явного таймера живёт бесконечно и рассыпается только
	// вне родной зоны (проверяется в section 4).
	if (timer <= 0 && obj->has_flag(EObjFlag::kZonedecay)) {
		m_obj_to_deadline[obj] = UINT64_MAX;
		return;
	}

	if (timer >= CObjectPrototype::UNLIMITED_TIMER
		|| stable_objs::IsTimerUnlimited(obj)) {
		m_obj_to_deadline[obj] = UINT64_MAX;
		return;
	}

	// Таймер "заморожен" до тех пор, пока с предметом не повзаимодействовал
	// игрок (флаг kTicktimer выставляется в PlaceObjToInventory). Лежащий в
	// комнате предмет должен убираться через destroyer/zone-reset, а не
	// через истечение собственного таймера.
	if (!obj->has_flag(EObjFlag::kTicktimer)) {
		m_obj_to_deadline[obj] = UINT64_MAX;
		return;
	}

	if (timer <= 0) {
		m_obj_to_deadline[obj] = m_counter;
		m_queue.insert({m_counter, obj});
		return;
	}

	const uint64_t deadline = m_counter + static_cast<uint64_t>(timer);
	m_queue.insert({deadline, obj});
	m_obj_to_deadline[obj] = deadline;
}

void ObjDecayManager::add_env_check(ObjData *obj) {
	if (obj) {
		m_env_check_objs.insert(obj);
	}
}

void ObjDecayManager::remove_env_check(ObjData *obj) {
	m_env_check_objs.erase(obj);
}

void ObjDecayManager::add_timed_spell_obj(ObjData *obj) {
	if (obj) {
		m_timed_spell_objs.insert(obj);
	}
}

void ObjDecayManager::remove_timed_spell_obj(ObjData *obj) {
	m_timed_spell_objs.erase(obj);
}

void ObjDecayManager::drop_obj_from_indices(ObjData *obj) {
	m_env_check_objs.erase(obj);
	m_timed_spell_objs.erase(obj);
	m_zonedecay_objs.erase(obj);
	remove_queue_entry(obj);
	m_obj_to_deadline.erase(obj);
}

bool ObjDecayManager::is_outside_home_zone(ObjData *obj) const {
	if (!obj->has_flag(EObjFlag::kZonedecay)) {
		return false;
	}
	if (!obj->get_vnum_zone_from()) {
		return false;
	}
	const int room = up_obj_where(obj);
	if (room == kNowhere) {
		return false;
	}
	return obj->get_vnum_zone_from()
		!= zone_table[world[room]->zone_rn].vnum;
}

TickResult ObjDecayManager::process_tick() {
	++m_counter;
	TickResult result;

	// 1. Process expired timer entries
	while (!m_queue.empty()) {
		auto it = m_queue.begin();
		if (it->deadline > m_counter) {
			break;
		}
		auto *obj = it->obj;
		m_queue.erase(it);
		m_obj_to_deadline.erase(obj);

		if (obj->get_where_obj() == EWhereObj::kSeller) {
			continue;
		}

		m_env_check_objs.erase(obj);
		m_timed_spell_objs.erase(obj);
		m_zonedecay_objs.erase(obj);
		result.decay_timer.push_back(obj);
	}

	// 2. Environmental checks + destroyer for room objects.
	// Зональная проверка для kZonedecay вынесена в section 4 - она работает
	// для предметов в любом месте (комната/инвентарь/контейнер), а не только
	// для лежащих в комнате.
	{
		auto env_copy = m_env_check_objs;
		for (auto *obj : env_copy) {
			if (!m_obj_to_deadline.count(obj)) {
				m_env_check_objs.erase(obj);
				continue;
			}
			if (obj->get_where_obj() == EWhereObj::kSeller) {
				continue;
			}
			if (CheckObjDecay(obj, false)) {
				drop_obj_from_indices(obj);
				result.env_destroy.push_back(obj);
				continue;
			}
			if (obj->get_destroyer() > 0 && !NO_DESTROY(obj)) {
				obj->dec_destroyer();
				if (obj->get_destroyer() == 0) {
					drop_obj_from_indices(obj);
					result.decay_timer.push_back(obj);
				}
			}
		}
	}

	// 3. Process timed spell and food/liquid periodic effects
	{
		auto ts_copy = m_timed_spell_objs;
		for (auto *obj : ts_copy) {
			if (!m_obj_to_deadline.count(obj)) {
				m_timed_spell_objs.erase(obj);
				continue;
			}
			obj->process_periodic_effects();
			if (!obj->has_timed_spell()) {
				m_timed_spell_objs.erase(obj);
			}
		}
	}

	// 4. kZonedecay: предмет рассыпается, если оказался вне родной зоны.
	// Работает по up_obj_where, поэтому ловит и комнатные, и носимые,
	// и надетые, и вложенные в контейнер предметы.
	{
		auto zd_copy = m_zonedecay_objs;
		for (auto *obj : zd_copy) {
			if (!m_obj_to_deadline.count(obj)) {
				m_zonedecay_objs.erase(obj);
				continue;
			}
			if (obj->get_where_obj() == EWhereObj::kSeller) {
				continue;
			}
			if (!is_outside_home_zone(obj)) {
				continue;
			}
			drop_obj_from_indices(obj);
			result.decay_timer.push_back(obj);
		}
	}

	return result;
}

uint64_t ObjDecayManager::get_deadline(const ObjData *obj) const {
	auto it = m_obj_to_deadline.find(const_cast<ObjData *>(obj));
	if (it != m_obj_to_deadline.end()) {
		return it->second;
	}
	return UINT64_MAX;
}

void ObjDecayManager::update_queue_entry(ObjData *obj, uint64_t new_deadline) {
	remove_queue_entry(obj);
	m_queue.insert({new_deadline, obj});
	m_obj_to_deadline[obj] = new_deadline;
}

void ObjDecayManager::remove_queue_entry(ObjData *obj) {
	auto it_dl = m_obj_to_deadline.find(obj);
	if (it_dl != m_obj_to_deadline.end() && it_dl->second != UINT64_MAX) {
		m_queue.erase({it_dl->second, obj});
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
