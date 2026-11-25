/**
\file religion.h - a part of the Bylins engine.
\brief Those a character prays or sacrifices to (issue #2689), from cfg/mechanics/religion.xml.
\details Each recipient has the name the player types (in the dative: "молиться Николе-Угоднику"),
		 the religion it belongs to and the service spell the prayer or the sacrifice casts. Heathens
		 sacrifice to their gods, Christians pray to their saints; what a recipient grants is the spell's
		 affect, not part of this table.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_RELIGION_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_RELIGION_H_

#include "engine/boot/cfg_manager.h"
#include "gameplay/magic/spells_constants.h"

#include <string>
#include <vector>

namespace religion {

struct Recipient {
	std::string name;    // as typed and shown, in the dative case: "Перуну"
	int religion{0};     // kReligionPoly or kReligionMono
	ESpell spell{ESpell::kUndefined};
};

class Recipients {
 public:
	// Reads the <recipient> children of the <religion> node `root` points at, replacing what was there.
	// A recipient with a missing name, an unknown religion or a spell that is not a valid spell is
	// logged and dropped.
	void Build(parser_wrapper::DataNode &root);

	// The first recipient whose name starts with `abbr` (case-insensitive), or nullptr.
	[[nodiscard]] const Recipient *Find(const std::string &abbr) const;
	[[nodiscard]] const std::vector<Recipient> &All() const { return recipients_; }

 private:
	std::vector<Recipient> recipients_;
};

// The recipients of the loaded religion.xml.
Recipients &RecipientsRegistry();

class RecipientsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace religion

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_RELIGION_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
