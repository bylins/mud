/**
 \file vedun.cpp - a part of the Bylins engine.
 \brief Vedun data editor -- Phase 1: command + read-only DOM viewer (issue.vedun-editor).
*/

#include "vedun.h"

#include "engine/db/global_objects.h"        // MUD::CfgManager
#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/core/comm.h"                 // SendMsgToChar
#include "engine/ui/table_wrapper.h"          // table_wrapper::Table
#include "engine/ui/modify.h"                 // page_string (pager)
#include "engine/olc/vedun/enum_registry.h"   // EnumRegistry / RegisterEditorEnums
#include "utils/mud_string.h"                 // two_arguments
#include "utils/utils_string.h"               // str_cmp (case-insensitive)
#include "utils/logger.h"                      // log (boot-time scheme lint)
#include "gameplay/magic/spells_info.h"        // SpellInfo::Print -- the `l` spellinfo view
#include "utils/parse.h"                        // parse::ReadAsConstant<ESpell>

#include <fmt/format.h>

#include <cctype>
#include <filesystem>
#include <system_error>
#include <map>
#include <sstream>

namespace vedun {

namespace {

// issue: prevent two implementors from clobbering the same data file. A Vedun session loads and
// rewrites the WHOLE file (any element's edit re-saves the entire file), so concurrent editing of
// any element of the same file races -- last save wins. We therefore lock at file granularity:
// one active editor per data file. The lock is released by Session's destructor (RAII), so a quit,
// a disconnect, or a session replacement all free it. Maps file path -> current editor's name.
std::map<std::string, std::string> &EditLocks() {
	static std::map<std::string, std::string> locks;
	return locks;
}

std::vector<parser_wrapper::DataNode> ChildrenOf(parser_wrapper::DataNode node);   // defined below

// The parent tag name of the deepest path node (empty at the element root). Used so the scheme
// can disambiguate repeated tag names (e.g. <flags> = EMagic at spell level, EAffFlag in <affects>).
std::string ParentTagName(const Session &s) {
	return s.path.size() >= 2 ? std::string(s.path[s.path.size() - 2].GetName()) : std::string();
}

// The scheme def for the current node, honouring its parent context.
const TagDef *CurrentTagDef(const Session &s) {
	return s.scheme.FindTag(s.path.back().GetName(), ParentTagName(s));
}

// The children the current node can still gain: repeatable ones always; singletons only while absent.
// (So the add-child menu hides a single-use tag once it is present.)
std::vector<const ChildDef *> AddableChildren(const Session &s) {
	const TagDef *td = CurrentTagDef(s);
	std::vector<const ChildDef *> out;
	if (td == nullptr) {
		return out;
	}
	std::set<std::string> have;
	for (auto &c : ChildrenOf(s.path.back())) {
		have.insert(c.GetName());
	}
	for (const auto &cd : td->children) {
		if (cd.multiple || have.find(cd.tag) == have.end()) {
			out.push_back(&cd);
		}
	}
	return out;
}

// One editable attribute row: present on the node, or scheme-declared but not yet set (offered so it
// can be added). `def` is the scheme def (may be null for an untyped present attr).
struct AttrRow {
	std::string name;
	std::string value;
	bool present{false};
	const AttrDef *def{nullptr};
};

// The current node's editable attributes: those present (document order), then any scheme-declared
// attributes still absent (so a missing field like reposition/stop_fight or material/any_of can be
// added). Render and the number dispatch share this so the numbering matches.
std::vector<AttrRow> AttrRows(const Session &s) {
	const TagDef *td = CurrentTagDef(s);
	// Snapshot the node's present attributes once (name -> value, document order).
	std::vector<std::pair<std::string, std::string>> present;
	for (const auto &[name, value] : s.path.back().Attributes()) {
		present.emplace_back(name, value);
	}
	const auto find_present = [&present](const std::string &n) -> const std::string * {
		for (const auto &p : present) {
			if (p.first == n) {
				return &p.second;
			}
		}
		return nullptr;
	};
	std::vector<AttrRow> rows;
	std::set<std::string> seen;
	// Scheme-declared attributes first, in declared order, so a row keeps its position whether or
	// not the attribute is set yet -- setting a value no longer makes its row jump to the end, so
	// the editor view stays stable across edits (the previous "present first, then absent" order
	// reshuffled rows the moment an unset attribute was given a value).
	if (td != nullptr) {
		for (const auto &ad : td->attrs) {
			const std::string *val = find_present(ad.name);
			rows.push_back({ad.name, val ? *val : "", val != nullptr, &ad});
			seen.insert(ad.name);
		}
	}
	// Any present-but-undeclared (untyped) attributes follow, in document order.
	for (const auto &p : present) {
		if (seen.find(p.first) == seen.end()) {
			rows.push_back({p.first, p.second, true, nullptr});
		}
	}
	return rows;
}

// Collect a node's child tags into an indexable vector (the menu order shown to the user).
std::vector<parser_wrapper::DataNode> ChildrenOf(parser_wrapper::DataNode node) {
	std::vector<parser_wrapper::DataNode> kids;
	for (auto &child : node.Children()) {
		kids.push_back(child);
	}
	return kids;
}

// A short inline hint for a child tag: its id/val attribute if present, so the menu is scannable
// without descending.
std::string ChildHint(const parser_wrapper::DataNode &child) {
	const std::string id = child.GetValue("id");
	if (!id.empty()) {
		return fmt::format("id={}", id);
	}
	const std::string type = child.GetValue("type");
	if (!type.empty()) {
		return fmt::format("type={}", type);
	}
	const std::string val = child.GetValue("val");
	if (!val.empty()) {
		return fmt::format("val={}", val);
	}
	return "";
}

// Build a compact type/validity hint for an attribute from its scheme def (empty if untyped).
std::string AttrHint(const AttrDef *ad, const std::string &value) {
	if (ad == nullptr) {
		return "";
	}
	std::string hint;
	switch (ad->type) {
		case FieldType::kEnum:
		case FieldType::kFlagset:
		case FieldType::kList:
			hint = fmt::format("{}<{}>", FieldTypeName(ad->type), ad->enum_type);
			if (ad->type == FieldType::kEnum) {
				const auto &reg = EnumRegistry::Instance();
				if (reg.Known(ad->enum_type) && !reg.ValueOf(ad->enum_type, value).has_value()) {
					hint += " (?)";   // current value is not a known member
				}
			}
			break;
		case FieldType::kInt:
		case FieldType::kDouble:
			hint = FieldTypeName(ad->type);
			if (ad->min || ad->max) {
				hint += fmt::format("[{}..{}]", ad->min ? std::to_string(*ad->min) : "",
					ad->max ? std::to_string(*ad->max) : "");
			}
			break;
		default:
			hint = FieldTypeName(ad->type);
			break;
	}
	if (ad->readonly) {
		hint += " ro";
	}
	return hint;
}

// Colour + space the editor tables: dark-green numbering, dark-cyan name, white value (point 3).
void StyleColumns(table_wrapper::Table &table) {
	table.set_cell_right_padding(2);
	table.column(0).set_cell_content_fg_color(table_wrapper::color::kGreen);
	table.column(1).set_cell_content_fg_color(table_wrapper::color::kCyan);
	table.column(2).set_cell_content_fg_color(table_wrapper::color::kLightWhite);
}

// Enums up to this size are listed inline when editing; larger ones (ESpell ~268, ESkill ~96)
// would flood the screen, so the prompt points at "?" for a paged list instead.
constexpr std::size_t kInlineEnumLimit = 100;

// Fill a 4-column grid with an enum's members (numbered for a single-pick enum) and decorate it.
void FillEnumGrid(CharData *ch, const std::vector<EnumMember> &members, bool numbered,
				  table_wrapper::Table &table) {
	std::size_t col = 0;
	for (std::size_t i = 0; i < members.size(); ++i) {
		table << (numbered ? fmt::format("{}) {}", i + 1, members[i].name) : members[i].name);
		if (++col % 4 == 0) {
			table << table_wrapper::kEndRow;
		}
	}
	if (col % 4 != 0) {
		for (std::size_t j = col % 4; j < 4; ++j) {
			table << "";
		}
		table << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
}

// Show an enum's members as a tooltip when entering an enum/list value. A single enum is numbered
// (pick by number); a list is a plain reference. Small enums are shown inline; large ones (ESpell,
// ESkill) print a hint to type "?" for the full paged list (handled in vedun_parse).
void ShowEnumChoices(CharData *ch, const std::string &enum_type, bool numbered) {
	const auto *members = EnumRegistry::Instance().Members(enum_type);
	if (members == nullptr || members->empty()) {
		return;
	}
	if (members->size() > kInlineEnumLimit) {
		SendMsgToChar(fmt::format("(&c{}&n has {} values -- type &W?&n for the full list, or type the token name)\r\n",
			enum_type, members->size()), ch);
		return;
	}
	SendMsgToChar(fmt::format("&cValid {} values{}:&n\r\n", enum_type, numbered ? " (pick by number)" : ""), ch);
	table_wrapper::Table table;
	FillEnumGrid(ch, *members, numbered, table);
	table_wrapper::PrintTableToChar(ch, table);
}

// Page the full member list of an enum (the "?" help at an edit prompt), regardless of size. Uses
// the pager (page_string works in the editor state -- comm.cpp routes paged input before vedun_parse).
void PageEnumList(DescriptorData *d, const std::string &enum_type, bool numbered) {
	auto *ch = d->character.get();
	const auto *members = EnumRegistry::Instance().Members(enum_type);
	if (members == nullptr || members->empty()) {
		SendMsgToChar(fmt::format("(&c{}&n has no listable values)\r\n", enum_type), ch);
		return;
	}
	table_wrapper::Table table;
	FillEnumGrid(ch, *members, numbered, table);
	page_string(d, fmt::format("&cValid {} values{}:&n\r\n{}",
		enum_type, numbered ? " (pick by number)" : " ('|'-separated; '*' = any)", table.to_string()));
}

// Split a pipe-separated flag value ("a|b|c") into a set of (trimmed) tokens.
std::set<std::string> ParsePipeSet(const std::string &value) {
	std::set<std::string> out;
	std::size_t start = 0;
	while (true) {
		const auto pos = value.find('|', start);
		std::string token = value.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
		while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
			token.erase(token.begin());
		}
		while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
			token.pop_back();
		}
		if (!token.empty()) {
			out.insert(token);
		}
		if (pos == std::string::npos) {
			break;
		}
		start = pos + 1;
	}
	return out;
}

// The flag-set toggle editor (point 5): every member listed with a bright-green [*] marker on the
// selected ones; the user toggles by number, then applies.
void RenderFlagEditor(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	const auto *members = EnumRegistry::Instance().Members(s.edit_enum);
	SendMsgToChar(fmt::format("&WFlag-set&n &g{}&n &c<{}>&n -- toggle members by number:\r\n",
		s.edit_attr, s.edit_enum), ch);
	std::string current;
	if (members) {
		table_wrapper::Table table;
		for (std::size_t i = 0; i < members->size(); ++i) {
			const bool on = s.flag_set.count((*members)[i].name) > 0;
			table << fmt::format("{})", i + 1) << (on ? "[*]" : "") << (*members)[i].name
				  << table_wrapper::kEndRow;
			if (on) {
				current += (current.empty() ? "" : "|") + (*members)[i].name;
			}
		}
		table.set_cell_right_padding(2);
		table.column(0).set_cell_content_fg_color(table_wrapper::color::kGreen);
		table.column(1).set_cell_content_fg_color(table_wrapper::color::kLightGreen);  // [*] bright green
		table.column(2).set_cell_content_fg_color(table_wrapper::color::kLightWhite);
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToChar(ch, table);
	}
	SendMsgToChar(fmt::format("Current: &g{}&n\r\n", current.empty() ? "(none)" : current), ch);
	SendMsgToChar("&Wd&n) done (apply)   &Wc&n) cancel\r\n", ch);
}

// The add-child menu: only the tags the node can still gain (single-use children already present are
// hidden). A number or the exact tag name adds it.
void RenderAddChild(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	const std::string parent = s.path.back().GetName();
	const auto addable = AddableChildren(s);
	SendMsgToChar(fmt::format("&WAdd child to&n &c<{}>&n:\r\n", parent), ch);
	if (addable.empty()) {
		SendMsgToChar("(nothing can be added here)  &Wc&n) cancel\r\n", ch);
		return;
	}
	table_wrapper::Table table;
	for (std::size_t i = 0; i < addable.size(); ++i) {
		const TagDef *kd = s.scheme.FindTag(addable[i]->tag, parent);
		std::string note = kd ? kd->desc : "";
		if (addable[i]->multiple) {
			note += "  (repeatable)";
		}
		table << fmt::format("{})", i + 1) << fmt::format("<{}>", addable[i]->tag)
			  << note << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	StyleColumns(table);
	table_wrapper::PrintTableToChar(ch, table);
	SendMsgToChar("\r\nType a number (or the tag name) to add it.  &Wc&n) cancel\r\n", ch);
}

// The delete-child menu: the current node's children, numbered. A number removes that child.
void RenderDeleteChild(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto kids = ChildrenOf(d->vedun_session->path.back());
	SendMsgToChar("&WDelete which child?&n\r\n", ch);
	if (kids.empty()) {
		SendMsgToChar("(no children to delete)  &Wc&n) cancel\r\n", ch);
		return;
	}
	table_wrapper::Table table;
	for (std::size_t i = 0; i < kids.size(); ++i) {
		table << fmt::format("{})", i + 1) << fmt::format("<{}>", kids[i].GetName())
			  << ChildHint(kids[i]) << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	StyleColumns(table);
	table_wrapper::PrintTableToChar(ch, table);
	SendMsgToChar("\r\nType a number to remove it.  &Wc&n) cancel\r\n", ch);
}

// The move-child menu: the current node's children numbered, with the picked one marked ">>". Before
// a child is picked a number selects it; after, u/d nudge it and an empty line finishes.
void RenderMoveChild(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	const auto kids = ChildrenOf(s.path.back());
	const bool picked = s.move_node.IsNotEmpty();
	SendMsgToChar(picked ? "&WMove&n -- reposition the picked child:\r\n" : "&WMove which child?&n\r\n", ch);
	if (kids.empty()) {
		SendMsgToChar("(no children)  &Wc&n) cancel\r\n", ch);
		return;
	}
	table_wrapper::Table table;
	for (std::size_t i = 0; i < kids.size(); ++i) {
		const bool sel = picked && kids[i] == s.move_node;
		table << fmt::format("{})", i + 1) << (sel ? ">>" : "") << fmt::format("<{}>", kids[i].GetName())
			  << ChildHint(kids[i]) << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	StyleColumns(table);
	table_wrapper::PrintTableToChar(ch, table);
	if (picked) {
		SendMsgToChar("\r\n&Wu&n) up   &Wd&n) down   &W<enter>&n) finish   &Wc&n) cancel\r\n", ch);
	} else {
		SendMsgToChar("\r\nType a child number to pick it up.  &Wc&n) cancel\r\n", ch);
	}
}

// Page the current node's children as a flat "number) key: value" list -- the full overview of a
// sheaf's messages (or any node's children) without descending into each one. The number shown is the
// `l` on a spell: show the live parsed spell -- the in-game `show spellinfo` view -- for the element
// being edited (MUD::Spell(id).Print). Reflects the currently-loaded spell, so unsaved DOM edits show
// only after a save+reload. Read-only.
void ShowSpellInfoForCurrent(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	const std::string id_str = s.path.front().GetValue("id");
	ESpell id = ESpell::kUndefined;
	try {
		id = parse::ReadAsConstant<ESpell>(id_str.c_str());
	} catch (const std::exception &) {
		SendMsgToChar(fmt::format("Vedun: cannot resolve spell id '{}'.\r\n", id_str), ch);
		return;
	}
	std::ostringstream out;
	MUD::Spell(id).Print(ch, out);
	page_string(d, out.str());
}

// browse selection number, so the user can type it directly afterwards. Read-only.
void ListChildrenFull(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	const auto kids = ChildrenOf(s.path.back());
	if (kids.empty()) {
		SendMsgToChar("(no entries to list)\r\n", ch);
		return;
	}
	const int base = static_cast<int>(AttrRows(s).size());   // children are numbered after the attrs
	std::string out = fmt::format("&WVedun&n [{}] &c<{}>&n -- {} entries:\r\n",
		s.what, s.path.back().GetName(), kids.size());
	for (std::size_t i = 0; i < kids.size(); ++i) {
		std::string key = kids[i].GetValue("type");
		if (key.empty()) {
			key = kids[i].GetValue("id");
		}
		if (key.empty()) {
			key = kids[i].GetName();
		}
		out += fmt::format("&g{}&n) &c{}&n: {}\r\n", base + static_cast<int>(i) + 1, key, kids[i].GetValue("val"));
	}
	page_string(d, out);
}

// Page the whole element being edited (e.g. the entire spell) as XML, regardless of how deep the
// cursor sits in its tree -- it always serialises path.front() (the element root). Read-only. '&' is
// doubled so the pager's colour processor renders colour / act codes literally instead of eating them.
void ShowElement(DescriptorData *d) {
	const auto &s = *d->vedun_session;
	const std::string xml = s.path.front().ToXmlString();
	std::string out = fmt::format("&WVedun&n [{}] id=&c{}&n -- full element:\r\n",
		s.what, s.path.front().GetValue("id"));
	out.reserve(out.size() + xml.size() + 32);
	for (const char c : xml) {
		out += c;
		if (c == '&') {
			out += '&';   // pager: && renders one literal '&' (see color.cpp)
		}
	}
	page_string(d, out);
}

// Validate a typed value against its scheme def before it is written. Returns an error message (shown
// to the user) when the value violates the field's type / numeric range / enum membership, or empty
// when acceptable. Untyped fields (no scheme def) accept anything -- the loader still checks structure
// on save. This is the editor's first line of defence: bad values never reach the DOM.
std::string ValidateValue(const AttrDef *ad, const std::string &value) {
	if (ad == nullptr) {
		return "";
	}
	const auto &reg = EnumRegistry::Instance();
	switch (ad->type) {
		case FieldType::kInt: {
			long n = 0;
			try {
				std::size_t used = 0;
				n = std::stol(value, &used);
				if (used != value.size()) {
					return "expected a whole number";
				}
			} catch (...) {
				return "expected a whole number";
			}
			if (ad->min && n < *ad->min) {
				return fmt::format("must be at least {}", *ad->min);
			}
			if (ad->max && n > *ad->max) {
				return fmt::format("must be at most {}", *ad->max);
			}
			break;
		}
		case FieldType::kDouble: {
			double v = 0.0;
			try {
				std::size_t used = 0;
				v = std::stod(value, &used);
				if (used != value.size()) {
					return "expected a number";
				}
			} catch (...) {
				return "expected a number";
			}
			if (ad->min && v < static_cast<double>(*ad->min)) {
				return fmt::format("must be at least {}", *ad->min);
			}
			if (ad->max && v > static_cast<double>(*ad->max)) {
				return fmt::format("must be at most {}", *ad->max);
			}
			break;
		}
		case FieldType::kEnum: {
			if (reg.Known(ad->enum_type) && !reg.ValueOf(ad->enum_type, value).has_value()) {
				return fmt::format("'{}' is not a valid {} value", value, ad->enum_type);
			}
			break;
		}
		case FieldType::kIntList: {
			for (const auto &token : utils::Split(value, '|')) {
				if (token.empty()) {
					continue;
				}
				long n = 0;
				try {
					std::size_t used = 0;
					n = std::stol(token, &used);
					if (used != token.size()) {
						return fmt::format("'{}' is not a whole number", token);
					}
				} catch (...) {
					return fmt::format("'{}' is not a whole number", token);
				}
				if (ad->min && n < *ad->min) {
					return fmt::format("each value must be at least {}", *ad->min);
				}
				if (ad->max && n > *ad->max) {
					return fmt::format("each value must be at most {}", *ad->max);
				}
			}
			break;
		}
		case FieldType::kFlagset:
		case FieldType::kList: {
			if (reg.Known(ad->enum_type)) {
				for (const auto &token : utils::Split(value, '|')) {
					if (token.empty()) {
						continue;
					}
					if (ad->type == FieldType::kList && token == "*") {
						continue;   // the "any eligible" wildcard
					}
					if (!reg.ValueOf(ad->enum_type, token).has_value()) {
						return fmt::format("'{}' is not a valid {} member", token, ad->enum_type);
					}
				}
			}
			break;
		}
		case FieldType::kBool: {
			static const std::set<std::string> ok = {"true", "false", "Y", "N", "y", "n", "1", "0"};
			if (ok.find(value) == ok.end()) {
				return "expected true/false (or Y/N)";
			}
			break;
		}
		default:
			break;
	}
	return "";
}

void Render(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	parser_wrapper::DataNode cur = s.path.back();   // copy: keep the stored cursor stable
	const TagDef *tagdef = CurrentTagDef(s);

	std::string crumb;
	for (std::size_t i = 0; i < s.path.size(); ++i) {
		crumb += (i ? "/" : "");
		crumb += s.path[i].GetName();
	}
	const std::string tag_desc = tagdef ? tagdef->desc : "";
	SendMsgToChar(fmt::format("&WVedun&n [{}]  &c{}&n{}{}\r\n",
		s.what, crumb, s.dirty ? "  &Y*modified*&n" : "",
		tag_desc.empty() ? std::string{} : "  -- " + tag_desc), ch);

	std::size_t index = 0;
	const auto rows = AttrRows(s);
	if (!rows.empty()) {
		table_wrapper::Table table;
		for (const auto &row : rows) {
			++index;
			table << fmt::format("{})", index) << row.name
				  << (row.present ? row.value : "(unset)")
				  << AttrHint(row.def, row.value)
				  << (row.def ? row.def->desc : "") << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		StyleColumns(table);
		table_wrapper::PrintTableToChar(ch, table);
	}

	const std::string text = cur.GetValue("");
	if (text.find_first_not_of(" \t\r\n") != std::string::npos) {
		SendMsgToChar(fmt::format("&g(text)&n {}\r\n", text), ch);
	}

	const auto kids = ChildrenOf(cur);
	if (!kids.empty()) {
		table_wrapper::Table table;
		for (std::size_t i = 0; i < kids.size(); ++i) {
			++index;
			const TagDef *kdef = s.scheme.FindTag(kids[i].GetName(), cur.GetName());
			table << fmt::format("{})", index)
				  << fmt::format("<{}>", kids[i].GetName())
				  << ChildHint(kids[i])
				  << (kdef ? kdef->desc : "") << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		StyleColumns(table);
		table_wrapper::PrintTableToChar(ch, table);
	}
	SendMsgToChar("\r\n&WSelect&n a number -- attribute = edit, <tag> = enter.\r\n", ch);
	SendMsgToChar("&Wl&n) list   &Wp&n) print   &Wa&n) add child   &Wd&n) delete child   &Wm&n) move child   &Ws&n) save   &Wu&n) up   &Wq&n) quit\r\n", ch);
}

// The safe commit: validate the edited DOM (dry-parse, no swap), then atomically rewrite the file
// (.new -> rename) and reload the container. The original file is never at risk.
void SaveSession(DescriptorData *d) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	if (!s.dirty) {
		SendMsgToChar("Vedun: nothing to save.\r\n", ch);
		return;
	}
	s.doc.GoToRadix();
	const auto result = s.loader->Validate(s.doc);
	if (!result.ok) {
		SendMsgToChar(fmt::format("&RVedun: save refused -- {}&n\r\nThe file was not changed.\r\n",
			result.error), ch);
		return;
	}
	std::filesystem::path tmp = s.file;
	tmp += ".new";
	if (!s.doc.Save(tmp)) {
		SendMsgToChar("&RVedun: save failed (could not write the file).&n\r\n", ch);
		return;
	}
	std::error_code ec;
	std::filesystem::rename(tmp, s.file, ec);
	if (ec) {
		std::filesystem::remove(tmp, ec);
		SendMsgToChar("&RVedun: save failed (rename).&n\r\n", ch);
		return;
	}
	s.loader->Reload(parser_wrapper::DataNode(s.file));
	s.dirty = false;
	SendMsgToChar("&GVedun: saved and reloaded.&n\r\n", ch);
}

// Quit, or (if there are unsaved edits) prompt for save/abandon/cancel (point 5).
void QuitOrConfirm(DescriptorData *d) {
	auto &s = *d->vedun_session;
	if (!s.dirty) {
		vedun_cleanup(d);
		return;
	}
	s.mode = Mode::kConfirmQuit;
	SendMsgToChar("&YUnsaved changes.&n  &Ws&n) save & exit   &Wa&n) abandon (no save)   &Wc&n) cancel\r\n",
		d->character.get());
}

// Skip leading spaces, then return the argument with trailing whitespace trimmed (the editor's
// menus take a single token / number per line).
std::string TrimmedToken(const char *arg) {
	while (*arg == ' ') {
		++arg;
	}
	std::string token = arg;
	while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
		token.pop_back();
	}
	return token;
}

// Confirm-on-quit with unsaved edits: s) save & exit, a) abandon, anything else cancels.
void ParseConfirmQuit(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	while (*arg == ' ') {
		++arg;
	}
	if (*arg == 's' || *arg == 'S') {
		SaveSession(d);
		if (!s.dirty) {
			vedun_cleanup(d);
		} else {
			s.mode = Mode::kBrowse;  // save refused; stay open
			Render(d);
		}
		return;
	}
	if (*arg == 'a' || *arg == 'A') {
		vedun_cleanup(d);
		return;
	}
	s.mode = Mode::kBrowse;
	SendMsgToChar("Cancelled.\r\n", ch);
	Render(d);
}

// Awaiting a new attribute value (case preserved). "?" pages the value list; a bare number picks an
// enum member; the value is validated against the scheme before it is written (creating the attr).
void ParseEditAttr(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	std::string value = TrimmedToken(arg);
	if (value == "?") {
		const TagDef *td = CurrentTagDef(s);
		const AttrDef *ad = td ? td->FindAttr(s.edit_attr) : nullptr;
		if (ad && (ad->type == FieldType::kEnum || ad->type == FieldType::kList)
				&& EnumRegistry::Instance().Known(ad->enum_type)) {
			PageEnumList(d, ad->enum_type, ad->type == FieldType::kEnum);
		} else {
			SendMsgToChar("(no value list for this field)\r\n", ch);
		}
		return;
	}
	if (value.empty() || value == ".") {
		SendMsgToChar("Cancelled.\r\n", ch);
	} else {
		if (!s.edit_enum.empty()) {
			const auto *members = EnumRegistry::Instance().Members(s.edit_enum);
			if (members && value.find_first_not_of("0123456789") == std::string::npos) {
				const std::size_t pick = static_cast<std::size_t>(atoi(value.c_str()));
				if (pick >= 1 && pick <= members->size()) {
					value = (*members)[pick - 1].name;
				}
			}
		}
		// Enforce the scheme: reject out-of-range numbers / unknown enum tokens and re-prompt, keeping
		// the edit open (mode/edit_attr/edit_enum preserved) so the user can retry.
		const TagDef *td = CurrentTagDef(s);
		const AttrDef *ad = td ? td->FindAttr(s.edit_attr) : nullptr;
		const std::string err = ValidateValue(ad, value);
		if (!err.empty()) {
			SendMsgToChar(fmt::format("&RRejected:&n {}. Try again ('.' or blank cancels):\r\n", err), ch);
			return;
		}
		s.path.back().SetValue(s.edit_attr, value);
		s.dirty = true;
	}
	s.mode = Mode::kBrowse;
	s.edit_attr.clear();
	s.edit_enum.clear();
	Render(d);
}

// Flag-set toggle editor: a number toggles a member; d) applies (joins selected in enum order); c)/. cancels.
void ParseEditFlagset(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	while (*arg == ' ') {
		++arg;
	}
	if (*arg == 'c' || *arg == 'C' || *arg == '.') {
		s.mode = Mode::kBrowse;
		s.flag_set.clear();
		s.edit_attr.clear();
		s.edit_enum.clear();
		SendMsgToChar("Cancelled.\r\n", ch);
		Render(d);
		return;
	}
	if (*arg == 'd' || *arg == 'D') {
		const auto *members = EnumRegistry::Instance().Members(s.edit_enum);
		std::string joined;
		if (members) {
			for (const auto &m : *members) {
				if (s.flag_set.count(m.name)) {
					joined += (joined.empty() ? "" : "|") + m.name;
				}
			}
		}
		s.path.back().SetValue(s.edit_attr, joined);
		s.dirty = true;
		s.mode = Mode::kBrowse;
		s.flag_set.clear();
		s.edit_attr.clear();
		s.edit_enum.clear();
		Render(d);
		return;
	}
	const int pick = atoi(arg);
	const auto *members = EnumRegistry::Instance().Members(s.edit_enum);
	if (members && pick >= 1 && pick <= static_cast<int>(members->size())) {
		const std::string &nm = (*members)[pick - 1].name;
		if (s.flag_set.count(nm)) {
			s.flag_set.erase(nm);
		} else {
			s.flag_set.insert(nm);
		}
	}
	RenderFlagEditor(d);
}

// Add-child menu: resolve a still-addable child (by number or exact name) and append it.
void ParseAddChild(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	const std::string token = TrimmedToken(arg);
	if (token.empty() || token == "c" || token == "C" || token == ".") {
		s.mode = Mode::kBrowse;
		SendMsgToChar("Cancelled.\r\n", ch);
		Render(d);
		return;
	}
	// Resolve against the addable list (single-use children already present are excluded), so a
	// maxed-out tag simply isn't offered.
	const auto addable = AddableChildren(s);
	if (addable.empty()) {
		s.mode = Mode::kBrowse;
		SendMsgToChar("&RNothing can be added here.&n\r\n", ch);
		Render(d);
		return;
	}
	const ChildDef *picked = nullptr;
	if (token.find_first_not_of("0123456789") == std::string::npos) {
		const std::size_t pick = static_cast<std::size_t>(atoi(token.c_str()));
		if (pick >= 1 && pick <= addable.size()) {
			picked = addable[pick - 1];
		}
	} else {
		for (const auto *cd : addable) {
			if (cd->tag == token) {
				picked = cd;
				break;
			}
		}
	}
	if (picked == nullptr) {
		const TagDef *td = CurrentTagDef(s);
		if (td && td->FindChild(token)) {
			SendMsgToChar(fmt::format("&R<{}> is already present (only one allowed here).&n\r\n", token), ch);
		} else {
			SendMsgToChar(fmt::format("&R'{}' is not an allowed child here.&n Pick from the list.\r\n", token), ch);
		}
		RenderAddChild(d);
		return;
	}
	s.path.back().AddChild(picked->tag);
	s.dirty = true;
	s.mode = Mode::kBrowse;
	SendMsgToChar(fmt::format("&GAdded&n <{}> (not saved yet).\r\n", picked->tag), ch);
	Render(d);
}

// Delete-child menu: a number removes that child of the current node.
void ParseDeleteChild(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	while (*arg == ' ') {
		++arg;
	}
	if (!*arg || *arg == 'c' || *arg == 'C' || *arg == '.') {
		s.mode = Mode::kBrowse;
		SendMsgToChar("Cancelled.\r\n", ch);
		Render(d);
		return;
	}
	const auto kids = ChildrenOf(s.path.back());
	const int n = atoi(arg);
	if (n >= 1 && n <= static_cast<int>(kids.size())) {
		const std::string name = kids[n - 1].GetName();
		s.path.back().RemoveChild(kids[n - 1]);
		s.dirty = true;
		s.mode = Mode::kBrowse;
		SendMsgToChar(fmt::format("&GDeleted&n <{}> (not saved yet).\r\n", name), ch);
		Render(d);
	} else {
		SendMsgToChar("No such child number.\r\n", ch);
		RenderDeleteChild(d);
	}
}

// Move-child menu: first a number picks the child; then u/d nudge it, an empty line (or c) finishes.
void ParseMoveChild(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	const std::string tok = TrimmedToken(arg);
	if (s.move_node.IsEmpty()) {
		if (tok.empty() || tok == "c" || tok == "C" || tok == ".") {
			s.mode = Mode::kBrowse;
			Render(d);
			return;
		}
		const auto kids = ChildrenOf(s.path.back());
		const int n = atoi(tok.c_str());
		if (n >= 1 && n <= static_cast<int>(kids.size())) {
			s.move_node = kids[n - 1];
		} else {
			SendMsgToChar("No such child number.\r\n", ch);
		}
		RenderMoveChild(d);
		return;
	}
	// A child is picked: u/d reposition it, an empty line (or c) finishes.
	if (tok.empty() || tok == "c" || tok == "C" || tok == "." || tok == "q") {
		s.move_node = parser_wrapper::DataNode{};
		s.mode = Mode::kBrowse;
		Render(d);
		return;
	}
	if (tok == "u" || tok == "U") {
		if (s.path.back().MoveChildUp(s.move_node)) {
			s.dirty = true;
		}
	} else if (tok == "d" || tok == "D") {
		if (s.path.back().MoveChildDown(s.move_node)) {
			s.dirty = true;
		}
	}
	RenderMoveChild(d);
}

// Browse mode: top-level keys (q/s/u/a/d/m), else a number selects (attribute = edit, child = descend).
void ParseBrowse(DescriptorData *d, char *arg) {
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();
	while (*arg && std::isspace(static_cast<unsigned char>(*arg))) {
		++arg;
	}
	if (*arg == 'q' || *arg == 'Q') {
		QuitOrConfirm(d);
		return;
	}
	if (*arg == 's' || *arg == 'S') {
		SaveSession(d);
		Render(d);
		return;
	}
	if (*arg == 'u' || *arg == 'U' || *arg == '0') {
		if (s.path.size() > 1) {
			s.path.pop_back();
			Render(d);
		} else {
			QuitOrConfirm(d);
		}
		return;
	}
	if (*arg == 'a' || *arg == 'A') {
		s.mode = Mode::kAddChild;
		RenderAddChild(d);
		return;
	}
	if (*arg == 'd' || *arg == 'D') {
		s.mode = Mode::kDeleteChild;
		RenderDeleteChild(d);
		return;
	}
	if (*arg == 'm' || *arg == 'M') {
		s.mode = Mode::kMoveChild;
		s.move_node = parser_wrapper::DataNode{};
		RenderMoveChild(d);
		return;
	}
	if (*arg == 'l' || *arg == 'L') {
		if (s.what == "spell") {
			ShowSpellInfoForCurrent(d);   // `l` = the `show spellinfo` view for the edited spell
		} else {
			ListChildrenFull(d);
		}
		return;
	}
	if (*arg == 'p' || *arg == 'P') {
		ShowElement(d);
		return;
	}
	const int n = atoi(arg);
	if (n >= 1) {
		const auto rows = AttrRows(s);
		const int num_attrs = static_cast<int>(rows.size());
		if (n <= num_attrs) {
			const AttrRow &row = rows[n - 1];
			const std::string &name = row.name;
			const AttrDef *ad = row.def;
			if (ad && ad->readonly) {
				SendMsgToChar(fmt::format("Vedun: '{}' is read-only.\r\n", name), ch);
				Render(d);
				return;
			}
			// Flag-set attribute: open the [*] toggle editor (empty start when the attr is unset).
			if (ad && ad->type == FieldType::kFlagset && EnumRegistry::Instance().Known(ad->enum_type)) {
				s.mode = Mode::kEditFlagset;
				s.edit_attr = name;
				s.edit_enum = ad->enum_type;
				s.flag_set = ParsePipeSet(row.value);
				RenderFlagEditor(d);
				return;
			}
			// Scalar / single-enum / list attribute. Entering a value creates the attribute if unset.
			s.mode = Mode::kEditAttr;
			s.edit_attr = name;
			s.edit_enum.clear();
			if (ad && ad->type == FieldType::kEnum && EnumRegistry::Instance().Known(ad->enum_type)) {
				ShowEnumChoices(ch, ad->enum_type, true);
				s.edit_enum = ad->enum_type;
			} else if (ad && ad->type == FieldType::kList && EnumRegistry::Instance().Known(ad->enum_type)) {
				ShowEnumChoices(ch, ad->enum_type, false);   // reference list ('|'-separated; '*' = any)
			} else if (ad && ad->type == FieldType::kBool) {
				SendMsgToChar("&c(boolean: true / false)&n\r\n", ch);
			}
			SendMsgToChar(fmt::format("New value for &g{}&n [{}] ('.' or blank cancels):\r\n",
				name, row.present ? row.value : "unset"), ch);
			return;
		}
		const auto kids = ChildrenOf(s.path.back());
		const int child_index = n - num_attrs;
		if (child_index >= 1 && child_index <= static_cast<int>(kids.size())) {
			s.path.push_back(kids[child_index - 1]);
		}
	}
	Render(d);
}

} // namespace

// RAII release of the per-file edit lock (see EditLocks): runs on quit, disconnect, or replacement.
Session::~Session() {
	if (!lock_key.empty()) {
		EditLocks().erase(lock_key);
	}
}

void do_vedun(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	auto *d = ch->desc;
	if (!d) {
		return;
	}
	static const bool kEnumsReady = [] { RegisterEditorEnums(); return true; }();  // once
	(void) kEnumsReady;
	char what[kMaxInputLength], element[kMaxInputLength];
	two_arguments(argument, what, element);

	// `vedun` -- list the editable data sets, four per row (paged).
	if (!*what) {
		std::vector<std::string> whats;
		for (const auto &entry : MUD::CfgManager().EditableEntries()) {
			whats.push_back(entry.what);
		}
		table_wrapper::Table table;
		for (std::size_t i = 0; i < whats.size(); ++i) {
			table << whats[i];
			if ((i + 1) % 4 == 0) {
				table << table_wrapper::kEndRow;
			}
		}
		if (const auto rem = whats.size() % 4; rem != 0) {  // pad the last partial row
			for (std::size_t j = rem; j < 4; ++j) {
				table << "";
			}
			table << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		page_string(d, fmt::format("&WVedun&n -- editable data sets:\r\n{}Usage: vedun <what> [element]\r\n",
			table.to_string()));
		return;
	}

	const auto entry = MUD::CfgManager().FindEditable(what);
	if (!entry) {
		SendMsgToChar(fmt::format("Vedun: unknown data set '{}'.\r\n", what), ch);
		return;
	}

	// `vedun <what>` -- list the editable elements (paged).
	if (!*element) {
		table_wrapper::Table table;
		for (const auto &el : entry->loader->ListElements()) {
			table << el.id << el.label << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		page_string(d, fmt::format("&WVedun&n [{}] -- elements:\r\n{}", entry->what, table.to_string()));
		return;
	}

	// `vedun <what> <element>` -- resolve the element by id (case-insensitive; the interpreter
	// lowercases input) or by its label (e.g. the Russian name), then open the read-only viewer.
	std::string resolved_id;
	for (const auto &el : entry->loader->ListElements()) {
		if (str_cmp(el.id, element) == 0 || str_cmp(el.label, element) == 0) {
			resolved_id = el.id;
			break;
		}
	}
	if (resolved_id.empty()) {
		// No exact id/label hit: fall back to a case-insensitive substring search over id and label.
		// A single hit opens directly; several hits are listed so the user can disambiguate.
		std::vector<cfg_manager::EditableElement> matches;
		for (const auto &el : entry->loader->ListElements()) {
			if (str_str(el.id, element) != std::string::npos
					|| str_str(el.label, element) != std::string::npos) {
				matches.push_back(el);
			}
		}
		if (matches.size() == 1) {
			resolved_id = matches.front().id;
		} else if (matches.size() > 1) {
			table_wrapper::Table table;
			for (const auto &m : matches) {
				table << m.id << m.label << table_wrapper::kEndRow;
			}
			table_wrapper::DecorateNoBorderTable(ch, table);
			page_string(d, fmt::format("&WVedun&n: '{}' matches {} elements in '{}' -- be more specific:\r\n{}",
				element, matches.size(), entry->what, table.to_string()));
			return;
		}
	}
	if (resolved_id.empty()) {
		SendMsgToChar(fmt::format("Vedun: element '{}' not found in '{}'.\r\n", element, entry->what), ch);
		return;
	}
	Scheme scheme = LoadScheme(SchemePathFor(entry->file));
	if (scheme.IsProhibited(resolved_id)) {
		SendMsgToChar(fmt::format("Vedun: '{}' is not editable (prohibited by its scheme).\r\n", resolved_id), ch);
		return;
	}
	// Locate the element's DOM node via the loader (it owns its file layout / id-attribute). The
	// returned node shares `doc`'s document, so edits through the session are persisted with the file.
	parser_wrapper::DataNode doc(entry->file);
	parser_wrapper::DataNode found = entry->loader->FindElementNode(doc, resolved_id);
	if (found.IsEmpty()) {
		SendMsgToChar(fmt::format("Vedun: '{}' has no node in {} (data/file out of sync?).\r\n",
			resolved_id, entry->what), ch);
		return;
	}
	// Refuse if another implementor already holds this file's edit lock (whole-file save races).
	const std::string lock_key = entry->file.string();
	if (const auto it = EditLocks().find(lock_key); it != EditLocks().end()) {
		SendMsgToChar(fmt::format("Vedun: '{}' is being edited by {} right now -- try again later.\r\n",
			entry->what, it->second), ch);
		return;
	}
	auto session = std::make_shared<Session>();
	session->what = entry->what;
	session->lock_key = lock_key;
	session->file = entry->file;
	session->loader = entry->loader;
	session->doc = std::move(doc);
	session->path.push_back(found);
	session->scheme = std::move(scheme);
	d->vedun_session = std::move(session);
	EditLocks()[lock_key] = GET_NAME(ch);
	d->state = EConState::kVedun;
	Render(d);
}

void vedun_parse(DescriptorData *d, char *arg) {
	if (!d->vedun_session) {
		d->state = EConState::kPlaying;
		return;
	}
	switch (d->vedun_session->mode) {
		case Mode::kConfirmQuit: ParseConfirmQuit(d, arg); return;
		case Mode::kEditAttr:    ParseEditAttr(d, arg);    return;
		case Mode::kEditFlagset: ParseEditFlagset(d, arg); return;
		case Mode::kAddChild:    ParseAddChild(d, arg);    return;
		case Mode::kDeleteChild: ParseDeleteChild(d, arg); return;
		case Mode::kMoveChild:   ParseMoveChild(d, arg);   return;
		case Mode::kBrowse:      ParseBrowse(d, arg);      return;
	}
}

namespace {

// Recursively collect, for the lint, tag names present in the data but undeclared by the scheme, and
// (for declared tags) attributes present in the data but not declared on that tag.
void CollectCoverage(const Scheme &scheme, parser_wrapper::DataNode node, const std::string &parent,
					 std::set<std::string> &undeclared_tags, std::set<std::string> &undeclared_attrs) {
	const std::string name = node.GetName();
	const TagDef *td = scheme.FindTag(name, parent);
	if (td == nullptr) {
		undeclared_tags.insert(parent.empty() ? name : parent + "/" + name);
	} else {
		for (const auto &[an, av] : node.Attributes()) {
			(void) av;
			if (td->FindAttr(an) == nullptr) {
				undeclared_attrs.insert(name + "." + an);
			}
		}
	}
	for (auto &child : node.Children()) {
		CollectCoverage(scheme, child, name, undeclared_tags, undeclared_attrs);
	}
}

// A short, capped sample of a set (so the log line can't be flooded).
std::string SampleOf(const std::set<std::string> &items) {
	std::string out;
	int n = 0;
	for (const auto &x : items) {
		if (n++ >= 15) {
			out += " ...";
			break;
		}
		out += " " + x;
	}
	return out;
}

} // namespace

void LintSchemes() {
	RegisterEditorEnums();
	const auto &reg = EnumRegistry::Instance();
	for (const auto &entry : MUD::CfgManager().EditableEntries()) {
		Scheme scheme = LoadScheme(SchemePathFor(entry.file));
		if (scheme.Empty()) {
			continue;
		}
		int enum_errors = 0;
		for (const auto &[key, tag] : scheme.Tags()) {
			(void) key;
			for (const auto &attr : tag.attrs) {
				const bool enumish = attr.type == FieldType::kEnum || attr.type == FieldType::kFlagset
					|| attr.type == FieldType::kList;
				if (enumish && !attr.enum_type.empty() && !reg.Known(attr.enum_type)) {
					log("Vedun scheme lint [%s]: <%s %s> references unregistered enum '%s'.",
						entry.what.c_str(), tag.name.c_str(), attr.name.c_str(), attr.enum_type.c_str());
					++enum_errors;
				}
			}
		}
		std::set<std::string> undeclared_tags;
		std::set<std::string> undeclared_attrs;
		parser_wrapper::DataNode doc(entry.file);
		const std::string root = doc.GetName();
		for (auto &el : doc.Children()) {
			CollectCoverage(scheme, el, root, undeclared_tags, undeclared_attrs);
		}
		log("Vedun scheme lint [%s]: %d enum error(s), %zu undeclared tag type(s), %zu undeclared attribute(s).",
			entry.what.c_str(), enum_errors, undeclared_tags.size(), undeclared_attrs.size());
		if (!undeclared_tags.empty()) {
			log("Vedun scheme lint [%s]: undeclared tags:%s", entry.what.c_str(), SampleOf(undeclared_tags).c_str());
		}
		if (!undeclared_attrs.empty()) {
			log("Vedun scheme lint [%s]: undeclared attrs:%s", entry.what.c_str(), SampleOf(undeclared_attrs).c_str());
		}
	}
}

void vedun_cleanup(DescriptorData *d) {
	d->vedun_session.reset();
	d->state = EConState::kPlaying;
	if (d->character) {
		SendMsgToChar("Vedun closed.\r\n", d->character.get());
	}
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
