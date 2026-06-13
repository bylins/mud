// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SHOP_EXT_HPP_INCLUDED
#define SHOP_EXT_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/ui/interpreter.h"
#include "dictionary.h"
#include "engine/boot/cfg_manager.h"

#include <memory>
#include <string>
#include <vector>


namespace ShopExt {

// issue.shops-ext: a shop item-set (catalog entry), loaded from cfg/economics/shop_item_sets.xml
// and referenced from shops by id. The catalog now lives apart from the shop list.
struct item_set_node {
	long item_vnum{0};
	int item_price{0};
};
struct item_set {
	std::string _id;
	std::string comment;   // free-text note for the game designer (informational)
	std::vector<item_set_node> item_list;
};
using ItemSetPtr = std::shared_ptr<item_set>;
using ItemSetListType = std::vector<ItemSetPtr>;

// The loaded item-set catalog (populated by ShopItemSetsLoader, consumed by ShopExt::load).
const ItemSetListType &item_sets();

// Loads/edits cfg/economics/shop_item_sets.xml. Vedun-editable ("vedun shop_item_set").
class ShopItemSetsLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
};

void do_shops_list(CharData *ch);
void DoStoreShop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void load(bool reload);
int get_spent_today();
void update_timers();

} // namespace ShopExt

void town_shop_keepers();
void fill_shop_dictionary(DictionaryType &dic);

#endif // SHOP_EXT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
