/**
 \file vedun.h - a part of the Bylins engine.
 \brief Vedun -- a data-driven in-game editor for cfg-loaded entities (issue.vedun-editor).
 \details Phase 1: the `vedun [what] [element]` command (implementor-only) and a read-only viewer
          that reflects an entity's on-disk XML DOM into a navigable tree. Edits XML (not the parsed
          struct); later phases validate via the loader and apply via reload. See OUTPUT/
          issue.vedun-editor-plan.md.
*/

#ifndef BYLINS_SRC_ENGINE_OLC_VEDUN_VEDUN_H_
#define BYLINS_SRC_ENGINE_OLC_VEDUN_VEDUN_H_

#include "engine/boot/cfg_manager.h"
#include "utils/parser_wrapper.h"

#include <filesystem>
#include <string>
#include <vector>

class CharData;
class DescriptorData;

namespace vedun {

// Per-descriptor editing session. Phase 1 is a read-only viewer: it holds the whole-file DOM and a
// cursor stack (path) from the element's root node to the node currently shown.
struct Session {
	std::string what;                              // e.g. "spell"
	std::filesystem::path file;                    // the cfg source file
	cfg_manager::IEditableCfgLoader *loader{nullptr};
	parser_wrapper::DataNode doc;                  // the loaded whole-file DOM (kept alive here)
	std::vector<parser_wrapper::DataNode> path;    // cursor stack; path.front() = the element root
};

// The `vedun [what] [element]` command (implementor-only). The listing forms (no args / what only)
// print and return; the full form opens the read-only viewer (EConState::kVedun).
void do_vedun(CharData *ch, char *argument, int cmd, int subcmd);

// Input dispatch while a descriptor is in EConState::kVedun.
void vedun_parse(DescriptorData *d, char *arg);

// Tear down the session and return the descriptor to play.
void vedun_cleanup(DescriptorData *d);

} // namespace vedun

#endif // BYLINS_SRC_ENGINE_OLC_VEDUN_VEDUN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
