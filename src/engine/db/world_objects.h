#ifndef __WORLD_OBJECTS_HPP__
#define __WORLD_OBJECTS_HPP__

#include "id.h"
#include "engine/entities/obj_data.h"

#include <list>
#include <functional>

class WorldObjects {
 private:
	class WO_VNumChangeObserver : public VNumChangeObserver {
	 public:
		WO_VNumChangeObserver(WorldObjects &parent) : m_parent(parent) {}

		virtual void notify(CObjectPrototype &object, const ObjVnum old_vnum) override;

	 private:
		WorldObjects &m_parent;
	};

	class WO_RNumChangeObserver : public ObjectRNum_ChangeObserver {
	 public:
		WO_RNumChangeObserver(WorldObjects &parent) : m_parent(parent) {}

		virtual void notify(CObjectPrototype &object, const ObjRnum old_rnum) override;

	 private:
		WorldObjects &m_parent;
	};

	class WO_IDChangeObserver : public IDChangeObserver {
	 public:
		WO_IDChangeObserver(WorldObjects &parent) : m_parent(parent) {}

		virtual void notify(ObjData &object, const object_id_t old_vnum) override;

	 private:
		WorldObjects &m_parent;
	};

 public:
	using list_t = std::list<ObjData::shared_ptr>;
	using foreach_f = std::function<void(const ObjData::shared_ptr &)>;
	using foreach_while_f = std::function<bool(const ObjData::shared_ptr &)>;
	using predicate_f = std::function<bool(const ObjData::shared_ptr &)>;

	WorldObjects();
	WorldObjects(const WorldObjects &) = delete;
	WorldObjects &operator=(const WorldObjects &) = delete;

	/**
	* Creates an object, and add it to the object list
	*
	* \param alias - строка алиасов объекта (нужна уже здесь, т.к.
	* сразу идет добавление в ObjectAlias). На данный момент актуально
	* для трупов, остальное вроде не особо и надо видеть.
	*/
	ObjData::shared_ptr create_blank();
	// create a new object from a prototype
	ObjData::shared_ptr create_from_prototype_by_vnum(ObjVnum vnum);

	// create a new object from a prototype
	ObjData::shared_ptr create_from_prototype_by_rnum(ObjRnum rnum);

	ObjData::shared_ptr create_raw_from_prototype_by_rnum(ObjRnum rnum);

	void add(const ObjData::shared_ptr &object);
	void remove(ObjData *object);
	void remove(const ObjData::shared_ptr &object) { remove(object.get()); }
	const list_t &get_list() const { return m_objects_list; }
	void foreach(const foreach_f &function) const;
	void foreach_on_copy(const foreach_f &function) const;
	void foreach_on_copy_while(const foreach_while_f &function) const;
	void foreach_with_vnum(const ObjVnum vnum, const foreach_f &function) const;
	void foreach_with_rnum(const ObjRnum rnum, const foreach_f &function) const;
	void foreach_with_id(const object_id_t id, const foreach_f &function) const;
	ObjData::shared_ptr find_if(const predicate_f &predicate) const;
	ObjData::shared_ptr find_if(const predicate_f &predicate, int number) const;
	ObjData::shared_ptr find_if_and_dec_number(const predicate_f &predicate, int &number) const;
	ObjData::shared_ptr find_by_name(const char *name) const;
	ObjData::shared_ptr find_by_id(const object_id_t id, unsigned number) const;
	ObjData::shared_ptr find_first_by_id(const object_id_t id) const { return find_by_id(id, 0); }
	ObjData::shared_ptr find_by_vnum(const ObjVnum vnum, unsigned number) const;
	ObjData::shared_ptr find_by_vnum_and_dec_number(const ObjVnum vnum, unsigned &number) const;
	ObjData::shared_ptr find_by_vnum_and_dec_number(const ObjVnum vnum, unsigned &number, const object_id_set_t &except) const;
	ObjData::shared_ptr find_first_by_vnum(const ObjVnum vnum) const { return find_by_vnum(vnum, 0); }
	ObjData::shared_ptr find_by_rnum(const ObjRnum rnum, unsigned number) const;
	ObjData::shared_ptr find_first_by_rnum(const ObjRnum rnum) const { return find_by_rnum(rnum, 0); }
	ObjData::shared_ptr get_by_raw_ptr(ObjData *object) const;
	auto size() const { return m_objects_list.size(); }
	void purge() { m_purge_list.clear(); }

 private:
	using objects_set_t = std::unordered_set<ObjData::shared_ptr>;
	using object_raw_ptr_to_object_ptr_t = std::unordered_map<void *, list_t::iterator>;
	using vnum_to_object_ptr_t = std::unordered_map<ObjVnum, objects_set_t>;
	using rnum_to_object_ptr_t = std::unordered_map<ObjRnum, objects_set_t>;
	using id_to_object_ptr_t = std::unordered_map<object_id_t, objects_set_t>;

	void add_to_index(const list_t::iterator &object_i);

	list_t m_objects_list;
	vnum_to_object_ptr_t m_vnum_to_object_ptr;
	object_raw_ptr_to_object_ptr_t m_object_raw_ptr_to_object_ptr;
	id_to_object_ptr_t m_id_to_object_ptr;
	rnum_to_object_ptr_t m_rnum_to_object_ptr;

	WO_IDChangeObserver::shared_ptr m_id_change_observer;
	WO_RNumChangeObserver::shared_ptr m_rnum_change_observer;
	WO_VNumChangeObserver::shared_ptr m_vnum_change_observer;

	list_t m_purge_list;
};

extern WorldObjects &world_objects;

#endif // __WORLD_OBJECTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
