#ifndef __WORLD_OBJECTS_HPP__
#define __WORLD_OBJECTS_HPP__

#include "id.hpp"
#include "obj.hpp"

#include <list>
#include <functional>

class WorldObjects
{
private:
	class WO_VNumChangeObserver : public VNumChangeObserver
	{
	public:
		WO_VNumChangeObserver(WorldObjects& parent) : m_parent(parent) {}

		virtual void notify(CObjectPrototype& object, const obj_vnum old_vnum) override;

	private:
		WorldObjects& m_parent;
	};

	class WO_RNumChangeObserver : public ObjectRNum_ChangeObserver
	{
	public:
		WO_RNumChangeObserver(WorldObjects& parent) : m_parent(parent) {}

		virtual void notify(CObjectPrototype& object, const obj_rnum old_rnum) override;

	private:
		WorldObjects& m_parent;
	};

	class WO_IDChangeObserver : public IDChangeObserver
	{
	public:
		WO_IDChangeObserver(WorldObjects& parent) : m_parent(parent) {}

		virtual void notify(OBJ_DATA& object, const object_id_t old_vnum) override;

	private:
		WorldObjects& m_parent;
	};

public:
	using list_t = std::list<OBJ_DATA::shared_ptr>;
	using foreach_f = std::function<void(const OBJ_DATA::shared_ptr&)>;
	using foreach_while_f = std::function<bool(const OBJ_DATA::shared_ptr&)>;
	using predicate_f = std::function<bool(const OBJ_DATA::shared_ptr&)>;

	WorldObjects();
	WorldObjects(const WorldObjects&) = delete;
	WorldObjects& operator=(const WorldObjects&) = delete;

	/**
	* Creates an object, and add it to the object list
	*
	* \param alias - строка алиасов объекта (нужна уже здесь, т.к.
	* сразу идет добавление в ObjectAlias). На данный момент актуально
	* для трупов, остальное вроде не особо и надо видеть.
	*/
	OBJ_DATA::shared_ptr create_blank(const std::string& alias);
	OBJ_DATA::shared_ptr create_blank() { return create_blank(""); }

	// create a new object from a prototype
	OBJ_DATA::shared_ptr create_from_prototype_by_vnum(obj_vnum vnum);

	// create a new object from a prototype
	OBJ_DATA::shared_ptr create_from_prototype_by_rnum(obj_rnum rnum);

	OBJ_DATA::shared_ptr create_raw_from_prototype_by_rnum(obj_rnum rnum);

	void add(const OBJ_DATA::shared_ptr& object);
	void remove(OBJ_DATA* object);
	void remove(const OBJ_DATA::shared_ptr& object) { remove(object.get()); }
	const list_t& get_list() const { return m_objects_list; }
	void foreach(const foreach_f& function) const;
	void foreach_on_copy(const foreach_f& function) const;
	void foreach_on_copy_while(const foreach_while_f& function) const;
	void foreach_with_vnum(const obj_vnum vnum, const foreach_f& function) const;
	void foreach_with_rnum(const obj_rnum rnum, const foreach_f& function) const;
	void foreach_with_id(const object_id_t id, const foreach_f& function) const;
	OBJ_DATA::shared_ptr find_if(const predicate_f& predicate) const;
	OBJ_DATA::shared_ptr find_if(const predicate_f& predicate, unsigned number) const;
	OBJ_DATA::shared_ptr find_if_and_dec_number(const predicate_f& predicate, unsigned& number) const;
	OBJ_DATA::shared_ptr find_by_name(const char* name) const;
	OBJ_DATA::shared_ptr find_by_id(const object_id_t id, unsigned number) const;
	OBJ_DATA::shared_ptr find_first_by_id(const object_id_t id) const { return find_by_id(id, 0); }
	OBJ_DATA::shared_ptr find_by_vnum(const obj_vnum vnum, unsigned number) const;
	OBJ_DATA::shared_ptr find_by_vnum_and_dec_number(const obj_vnum vnum, unsigned& number) const;
	OBJ_DATA::shared_ptr find_by_vnum_and_dec_number(const obj_vnum vnum, unsigned& number, const object_id_set_t& except) const;
	OBJ_DATA::shared_ptr find_first_by_vnum(const obj_vnum vnum) const { return find_by_vnum(vnum, 0); }
	OBJ_DATA::shared_ptr find_by_rnum(const obj_rnum rnum, unsigned number) const;
	OBJ_DATA::shared_ptr find_first_by_rnum(const obj_rnum rnum) const { return find_by_rnum(rnum, 0); }
	OBJ_DATA::shared_ptr get_by_raw_ptr(OBJ_DATA* object) const;
	auto size() const { return m_objects_list.size(); }
	void purge() { m_purge_list.clear(); }

private:
	using objects_set_t = std::unordered_set<OBJ_DATA::shared_ptr>;
	using object_raw_ptr_to_object_ptr_t = std::unordered_map<void*, list_t::iterator>;
	using vnum_to_object_ptr_t = std::unordered_map<obj_vnum, objects_set_t>;
	using rnum_to_object_ptr_t = std::unordered_map<obj_rnum, objects_set_t>;
	using id_to_object_ptr_t = std::unordered_map<object_id_t, objects_set_t>;

	void add_to_index(const list_t::iterator& object_i);

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

extern WorldObjects& world_objects;

#endif // __WORLD_OBJECTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
