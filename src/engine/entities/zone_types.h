/**
 * \brief Zone type registry, loaded from lib/cfg/zone_types.xml.
 * \details Mirrors the guilds.h pattern: int-keyed InfoContainer partial
 *          specialization where the configuration author owns vnum uniqueness.
 *          A zone's `type` field (parsed from its .zon header) is the vnum into
 *          this registry; carryover from the legacy lib/misc/ztypes.lst format
 *          requires preserving the historical 0-based ordering.
 *          (issue.ztypes-migrate)
 */

#ifndef BYLINS_SRC_ENGINE_ENTITIES_ZONE_TYPES_H_
#define BYLINS_SRC_ENGINE_ENTITIES_ZONE_TYPES_H_

#include <string>
#include <vector>

#include "engine/boot/cfg_manager.h"
#include "engine/structs/info_container.h"
#include "utils/parser_wrapper.h"

namespace zone_types {

using DataNode = parser_wrapper::DataNode;

class ZoneTypesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

/**
 * One <type> entry from cfg/zone_types.xml. Holds the display name and an
 * optional list of alchemical ingredient type ids. An absent or empty
 * <ingredients/> child leaves the vector empty -- ImForageRoom treats an
 * empty list as "no foraging in this zone type", matching the legacy
 * `zone_types[i].ingr_qty == 0` short-circuit.
 */
class ZoneTypeInfo : public info_container::BaseItem<int> {
	friend class ZoneTypeInfoBuilder;

	std::string name_;
	std::vector<int> ingredients_;

 public:
	ZoneTypeInfo() = default;
	ZoneTypeInfo(int id, std::string &text_id, std::string &name, EItemMode mode)
		: BaseItem<int>(id, text_id, mode), name_{name} {};

	[[nodiscard]] const std::string &GetName() const { return name_; }
	[[nodiscard]] const std::vector<int> &GetIngredients() const { return ingredients_; }
};

class ZoneTypeInfoBuilder : public info_container::IItemBuilder<ZoneTypeInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseType(parser_wrapper::DataNode node);
	static void ParseIngredients(ItemPtr &info, parser_wrapper::DataNode &node);
};

using ZoneTypesInfo = info_container::InfoContainer<int, ZoneTypeInfo, ZoneTypeInfoBuilder>;

}  // namespace zone_types

#endif  // BYLINS_SRC_ENGINE_ENTITIES_ZONE_TYPES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
