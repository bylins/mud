/**
 \brief issue.ext-affects: cfg loader for affect data (cfg/affects.xml).
 \details Phase 1 (Names pilot) loads the per-affect short display names that feed sprintbits via the
		  EAffect bit, replacing the hand-maintained positionally-coupled affected_bits[] literal. The
		  EAffect enum itself stays in C++ (it is the identity, used in AFF_FLAGGED/switch everywhere);
		  this file is the data home meant to grow later (per-affect handlers/actions).
*/

#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECTS_LOADER_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECTS_LOADER_H_

#include "engine/boot/cfg_manager.h"

namespace affects {

class AffectsLoader : public cfg_manager::IEditableCfgLoader {  // cfg id "affects" -> cfg/affects.xml
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	// issue.vedun-editor: in-game editing of cfg/affects.xml (`vedun affects`). Elements are keyed by
	// the <affect id=> (EAffect token), so the default FindElementNode (child-by-id) is reused.
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
};

}  // namespace affects

#endif  // BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECTS_LOADER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
