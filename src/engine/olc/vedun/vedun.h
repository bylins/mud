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
#include "engine/olc/vedun/scheme.h"
#include "utils/parser_wrapper.h"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

class CharData;
class DescriptorData;

namespace vedun {

// Per-descriptor editing session: the whole-file DOM, a cursor stack (path) from the element root
// to the node shown, the scheme, and the edit state. Editing mutates the shared DOM in place;
// saving validates then atomically rewrites the file and reloads the container.
enum class Mode { kBrowse, kEditAttr, kEditFlagset, kAddChild, kDeleteChild, kMoveChild, kConfirmQuit };
struct Session {
	std::string what;                              // e.g. "spell"
	std::filesystem::path file;                    // the cfg source file
	cfg_manager::IEditableCfgLoader *loader{nullptr};
	parser_wrapper::DataNode doc;                  // the loaded whole-file DOM (kept alive here)
	std::vector<parser_wrapper::DataNode> path;    // cursor stack; path.front() = the element root
	Scheme scheme;                                 // the data file's .scheme (empty if none)
	Mode mode{Mode::kBrowse};                      // browse vs awaiting an attribute value
	std::string edit_attr;                         // attribute being edited (kEditAttr)
	std::string edit_enum;                         // enum type of edit_attr (number-pick), else empty
	std::set<std::string> flag_set;                // selected members while editing a flag-set
	parser_wrapper::DataNode move_node;            // the child being repositioned (kMoveChild)
	bool dirty{false};                             // unsaved edits in the DOM
	std::string lock_key;                          // the held per-file edit lock (empty = none)

	Session() = default;
	// Releases the per-file edit lock (RAII): fires on quit, disconnect, or session replacement.
	~Session();
};

// The `vedun [what] [element]` command (implementor-only). The listing forms (no args / what only)
// print and return; the full form opens the read-only viewer (EConState::kVedun).
void do_vedun(CharData *ch, char *argument, int cmd, int subcmd);

// Input dispatch while a descriptor is in EConState::kVedun.
void vedun_parse(DescriptorData *d, char *arg);

// Tear down the session and return the descriptor to play.
void vedun_cleanup(DescriptorData *d);

// issue.vedun-editor: boot-time scheme lint. For every editable data set that has a .scheme, logs
// scheme/data drift -- enum types the scheme references but the editor hasn't registered, and the
// tags/attributes present in the data but not declared in the scheme (so they edit untyped). Cheap,
// log-only; call once after the cfg files are loaded.
void LintSchemes();

} // namespace vedun

#endif // BYLINS_SRC_ENGINE_OLC_VEDUN_VEDUN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
