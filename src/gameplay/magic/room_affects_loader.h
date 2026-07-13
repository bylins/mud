/**
 \brief issue.affect-migration: cfg loader for the room-affect registry (cfg/room_affects.xml).
 \details Mirrors affects::AffectsLoader for char affects. The room-affect identity is the
		  room_spells::ERoomAffect enumerator (kept in C++); this file validates that every
		  <room_affect id> in the data resolves to a known enumerator and that every enumerator
		  has a row. It is the data home meant to grow per-room-affect handler/action data later.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MAGIC_ROOM_AFFECTS_LOADER_H_
#define BYLINS_SRC_GAMEPLAY_MAGIC_ROOM_AFFECTS_LOADER_H_

#include "engine/boot/cfg_manager.h"

namespace room_spells {

class RoomAffectsLoader : public cfg_manager::IEditableCfgLoader {  // cfg id "room_affects" -> cfg/room_affects.xml
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	// issue.vedun-editor: in-game editing of cfg/room_affects.xml (`vedun room_affects`). Keyed by the
	// <room_affect id=> (ERoomAffect token) -> default FindElementNode (child-by-id) is reused.
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
};

}  // namespace room_spells

#endif  // BYLINS_SRC_GAMEPLAY_MAGIC_ROOM_AFFECTS_LOADER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
