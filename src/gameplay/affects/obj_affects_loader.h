/**
 \brief issue.obj-affects: cfg loader for the obj-affect registry (cfg/obj_affects.xml).
 \details Mirrors room_spells::RoomAffectsLoader. The obj-affect identity is the
		  obj_affects::EObjAffect enumerator (kept in C++). This validates that every <obj_affect id>
		  resolves to a known enumerator and that every enumerator has a row, then builds the runtime
		  registry (which EObjFlag each affect owns + whether it is player-dispellable) via
		  obj_affects::BuildRegistry.
*/

#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_OBJ_AFFECTS_LOADER_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_OBJ_AFFECTS_LOADER_H_

#include "engine/boot/cfg_manager.h"

namespace obj_affects {

class ObjAffectsLoader : public cfg_manager::IEditableCfgLoader {  // cfg "obj_affects" -> cfg/obj_affects.xml
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	// issue.obj-affects: in-game editing (`vedun obj_affects`). Keyed by the <obj_affect id=>
	// (EObjAffect token); the default FindElementNode (child-by-id) is reused.
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
};

}  // namespace obj_affects

#endif  // BYLINS_SRC_GAMEPLAY_AFFECTS_OBJ_AFFECTS_LOADER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
