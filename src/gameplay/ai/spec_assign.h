#ifndef BYLINS_SRC_GAMEPLAY_AI_SPEC_ASSIGN_H_
#define BYLINS_SRC_GAMEPLAY_AI_SPEC_ASSIGN_H_

#include "engine/boot/cfg_manager.h"

#include <string>
#include <vector>

void AssignMobiles();
void AssignObjects();
void AssignRooms();
void ReloadSpecProcs();

// issue.specials: data-driven spec-proc assignment from cfg/specials.xml (replaces the
// lib/misc/specials.lst flat file + InitSpecProcs). Specials are grouped by mob:
//   <mob vnum="4001"><special handler="bank"/><special handler="rent"/></mob>
// A mob may carry several specials (banker + innkeeper, trainer + merchant). The file is
// Vedun-editable: `vedun special <vnum>` opens a <mob> element; add/remove its <special> children.
class SpecialsLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;

	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] parser_wrapper::DataNode FindElementNode(
			parser_wrapper::DataNode root, const std::string &id) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(
			parser_wrapper::DataNode root, const std::string &id) const final;

 private:
	// The <mob> entries loaded from the file (vnum -> joined handler names), for ListElements.
	std::vector<cfg_manager::EditableElement> elements_;
};

#endif // BYLINS_SRC_GAMEPLAY_AI_SPEC_ASSIGN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
