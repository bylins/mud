#ifndef __SHOPS_IMPLEMENTATION_HPP__
#define __SHOPS_IMPLEMENTATION_HPP__

#include "dictionary.hpp"
#include "structs.h"
#include "obj.hpp"

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <list>

namespace ShopExt
{
	extern int spent_today;

	class GoodsStorage
	{
	public:
		using shared_ptr = std::shared_ptr<GoodsStorage>;

		GoodsStorage() : m_object_uid_change_observer(new ObjectUIDChangeObserver(*this)) {}
		~GoodsStorage() { clear(); }

		const auto begin() const { return m_activities.begin(); }
		const auto end() const { return m_activities.end(); }

		void add(OBJ_DATA* object);
		void remove(OBJ_DATA* object);
		OBJ_DATA* get_by_uid(const int uid) const;
		void clear();

	private:
		using activities_t = std::unordered_map<OBJ_DATA* /* object pointer */, int /* to last activity */>;
		using objects_by_uid_t = std::unordered_map<int /* UID */, OBJ_DATA* /* to object ponter */>;

		friend class ObjectUIDChangeObserver;

		class ObjectUIDChangeObserver : public UIDChangeObserver
		{
		public:
			ObjectUIDChangeObserver(GoodsStorage& storage) : m_parent(storage) {}

			virtual void notify(OBJ_DATA& object, const int old_uid) override;

		private:
			GoodsStorage& m_parent;
		};

		activities_t m_activities;
		objects_by_uid_t m_objects_by_uid;

		ObjectUIDChangeObserver::shared_ptr m_object_uid_change_observer;
	};

	struct item_desc_node
	{
		std::string name;
		std::string description;
		std::string short_description;
		std::array<std::string, 6> PNames;
		ESex sex;
		CObjectPrototype::triggers_list_t trigs;
	};

	class ItemNode
	{
	public:
		using shared_ptr = std::shared_ptr<ItemNode>;
		using uid_t = unsigned;
		using temporary_ids_t = std::unordered_set<uid_t>;
		using item_descriptions_t = std::unordered_map<int/*vnum продавца*/, item_desc_node>;

		const static uid_t NO_UID = ~0u;

		ItemNode(const obj_vnum vnum, const long price) : m_vnum(vnum), m_price(price) {}
		ItemNode(const obj_vnum vnum, const long price, const uid_t uid) : m_vnum(vnum), m_price(price) { add_uid(uid); }

		const std::string& get_item_name(int keeper_vnum, int pad = 0) const;

		void add_desc(const mob_vnum vnum, const item_desc_node& desc) { m_descs[vnum] = desc; }
		void replace_descs(OBJ_DATA *obj, const int vnum) const;
		bool has_desc(const obj_vnum vnum) const { return m_descs.find(vnum) != m_descs.end(); }

		bool empty() const { return m_item_uids.empty(); }
		const auto size() const { return m_item_uids.size(); }

		auto uid() const { return 0 < m_item_uids.size() ? *m_item_uids.begin() : NO_UID; }
		void add_uid(const uid_t uid) { m_item_uids.emplace(uid); }
		void remove_uid(const uid_t uid) { m_item_uids.erase(uid); }

		const auto vnum() const { return m_vnum; }

		const auto get_price() const { return m_price; }
		void set_price(const long _) { m_price = _; }

	private:
		int m_vnum;						// VNUM of the item's prototype
		long m_price;					// Price of the item

		temporary_ids_t m_item_uids;	// List of uids of this item in the shop
		item_descriptions_t m_descs;
	};

	class ItemsList
	{
	public:
		using items_list_t = std::vector<ItemNode::shared_ptr>;

		ItemsList() {}

		void add(const obj_vnum vnum, const long price);
		void add(const obj_vnum vnum, const long price, const ItemNode::uid_t uid);
		void remove(const size_t index) { m_items_list.erase(m_items_list.begin() + index); }

		const items_list_t::value_type& node(const size_t index) const;
		const auto size() const { return m_items_list.size(); }
		bool empty() const { return m_items_list.empty(); }

	private:
		items_list_t m_items_list;
	};

	class shop_node : public DictionaryItem
	{
	public:
		using shared_ptr = std::shared_ptr<shop_node>;
		using mob_vnums_t = std::list<mob_vnum>;

		shop_node() : waste_time_min(0), can_buy(true) {};

		std::string currency;
		std::string id;
		int profit;
		int waste_time_min;
		bool can_buy;

		void add_item(const obj_vnum vnum, const long price) { m_items_list.add(vnum, price); }
		void add_item(const obj_vnum vnum, const long price, const ItemNode::uid_t uid) { m_items_list.add(vnum, price, uid); }

		const auto& items_list() const { return m_items_list; }

		void add_mob_vnum(const mob_vnum vnum) { m_mob_vnums.push_back(vnum); }

		const auto& mob_vnums() const { return m_mob_vnums; }

		void process_buy(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument);
		void print_shop_list(CHAR_DATA *ch, const std::string& arg, int keeper_vnum) const;
		void filter_shop_list(CHAR_DATA *ch, const std::string& arg, int keeper_vnum);
		void process_cmd(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd);
		void process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd);	// it should be const
		void clear_store();
		bool empty() const { return m_items_list.empty(); }

	private:
		void put_to_storage(OBJ_DATA* object) { m_storage.add(object); }

		void remove_from_storage(OBJ_DATA *obj);
		OBJ_DATA* get_from_shelve(const size_t index) const;
		unsigned get_item_num(std::string &item_name, int keeper_vnum) const;
		int can_sell_count(const int item_index) const;
		void put_item_to_shop(OBJ_DATA* obj);
		void do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, std::string cmd);

		ItemsList m_items_list;
		mob_vnums_t m_mob_vnums;

		GoodsStorage m_storage;
	};

	using ShopListType = std::vector<shop_node::shared_ptr>;
}

#endif // __SHOPS_IMPLEMENTATION_HPP__