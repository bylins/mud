#ifndef OBJ_DECAY_MANAGER_H_
#define OBJ_DECAY_MANAGER_H_

#include <cstdint>
#include <list>
#include <set>
#include <unordered_map>
#include <unordered_set>

class ObjData;

struct DecayEntry {
	uint64_t deadline;
	ObjData *obj;

	bool operator<(const DecayEntry &o) const {
		if (deadline != o.deadline) {
			return deadline < o.deadline;
		}
		return obj < o.obj;
	}
};

struct TickResult {
	std::list<ObjData *> env_destroy;
	std::list<ObjData *> decay_timer;
};

class ObjDecayManager {
 public:
	void insert(ObjData *obj);
	void remove(ObjData *obj);
	void on_timer_changed(ObjData *obj);
	void add_env_check(ObjData *obj);
	void remove_env_check(ObjData *obj);
	void add_timed_spell_obj(ObjData *obj);
	void remove_timed_spell_obj(ObjData *obj);
	TickResult process_tick();
	uint64_t current_mud_hour() const { return m_counter; }
	size_t size() const { return m_obj_to_deadline.size(); }
	size_t timed_spell_size() const { return m_timed_spell_objs.size(); }
	bool contains(const ObjData *obj) const { return m_obj_to_deadline.count(const_cast<ObjData *>(obj)) > 0; }
	uint64_t get_deadline(const ObjData *obj) const;

 private:
	void update_queue_entry(ObjData *obj, uint64_t new_deadline);
	void remove_queue_entry(ObjData *obj);

	uint64_t m_counter = 0;
	std::set<DecayEntry> m_queue;
	std::unordered_map<ObjData *, uint64_t> m_obj_to_deadline;
	std::unordered_set<ObjData *> m_env_check_objs;
	std::unordered_set<ObjData *> m_timed_spell_objs;
};

extern ObjDecayManager obj_decay_manager;

#endif // OBJ_DECAY_MANAGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
