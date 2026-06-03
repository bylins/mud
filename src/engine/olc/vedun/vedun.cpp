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

#include <fmt/format.h>

#include <cctype>
#include <filesystem>
#include <system_error>

namespace vedun {

namespace {

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

// Show an enum's members as a tooltip when entering an enum value (point 4). For a single enum the
// list is numbered (pick by number); for a flag-set it is a plain reference list.
void ShowEnumChoices(CharData *ch, const std::string &enum_type, bool numbered) {
	const auto *members = EnumRegistry::Instance().Members(enum_type);
	if (members == nullptr || members->empty()) {
		return;
	}
	if (members->size() > 160) {
		SendMsgToChar(fmt::format("(&c{}&n has {} values -- type the token name)\r\n", enum_type, members->size()), ch);
		return;
	}
	SendMsgToChar(fmt::format("&cValid {} values:&n\r\n", enum_type), ch);
	table_wrapper::Table table;
	std::size_t col = 0;
	for (std::size_t i = 0; i < members->size(); ++i) {
		table << (numbered ? fmt::format("{}) {}", i + 1, (*members)[i].name) : (*members)[i].name);
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
	table_wrapper::PrintTableToChar(ch, table);
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
		default:
			break;
	}
	return "";
}

void Render(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	parser_wrapper::DataNode cur = s.path.back();   // copy: keep the stored cursor stable
	const TagDef *tagdef = s.scheme.FindTag(cur.GetName());

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
	const auto attrs = cur.Attributes();
	if (!attrs.empty()) {
		table_wrapper::Table table;
		for (const auto &[name, value] : attrs) {
			++index;
			const AttrDef *ad = tagdef ? tagdef->FindAttr(name) : nullptr;
			table << fmt::format("{})", index) << name << value << AttrHint(ad, value)
				  << (ad ? ad->desc : "") << table_wrapper::kEndRow;
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
			const TagDef *kdef = s.scheme.FindTag(kids[i].GetName());
			table << fmt::format("{})", index)
				  << fmt::format("<{}>", kids[i].GetName())
				  << ChildHint(kids[i])
				  << (kdef ? kdef->desc : "") << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		StyleColumns(table);
		table_wrapper::PrintTableToChar(ch, table);
	}
	SendMsgToChar("&WSelect&n a number -- attribute = edit, <tag> = enter.\r\n", ch);
	SendMsgToChar("&Ws&n) save   &Wu&n) up   &Wq&n) quit\r\n", ch);
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

} // namespace

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
	auto session = std::make_shared<Session>();
	session->what = entry->what;
	session->file = entry->file;
	session->loader = entry->loader;
	session->doc = std::move(doc);
	session->path.push_back(found);
	session->scheme = std::move(scheme);
	d->vedun_session = std::move(session);
	d->state = EConState::kVedun;
	Render(d);
}

void vedun_parse(DescriptorData *d, char *arg) {
	if (!d->vedun_session) {
		d->state = EConState::kPlaying;
		return;
	}
	auto &s = *d->vedun_session;
	auto *ch = d->character.get();

	// Confirm-on-quit with unsaved edits (point 5).
	if (s.mode == Mode::kConfirmQuit) {
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
		return;
	}

	// Awaiting a new attribute value (case preserved). For an enum, a bare number picks a member.
	if (s.mode == Mode::kEditAttr) {
		while (*arg == ' ') {
			++arg;
		}
		std::string value = arg;
		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
			value.pop_back();
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
			// Enforce the scheme: reject out-of-range numbers / unknown enum tokens and re-prompt,
			// keeping the edit open (mode/edit_attr/edit_enum preserved) so the user can retry.
			const TagDef *td = s.scheme.FindTag(s.path.back().GetName());
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
		return;
	}

	// Flag-set toggle editor: numbers toggle membership; d) applies, c) cancels.
	if (s.mode == Mode::kEditFlagset) {
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
		return;
	}

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
	const int n = atoi(arg);
	if (n >= 1) {
		const auto attrs = s.path.back().Attributes();
		const int num_attrs = static_cast<int>(attrs.size());
		if (n <= num_attrs) {
			const std::string &name = attrs[n - 1].first;
			const TagDef *td = s.scheme.FindTag(s.path.back().GetName());
			const AttrDef *ad = td ? td->FindAttr(name) : nullptr;
			if (ad && ad->readonly) {
				SendMsgToChar(fmt::format("Vedun: '{}' is read-only.\r\n", name), ch);
				Render(d);
				return;
			}
			// Flag-set attribute: open the [*] toggle editor.
			if (ad && ad->type == FieldType::kFlagset && EnumRegistry::Instance().Known(ad->enum_type)) {
				s.mode = Mode::kEditFlagset;
				s.edit_attr = name;
				s.edit_enum = ad->enum_type;
				s.flag_set = ParsePipeSet(attrs[n - 1].second);
				RenderFlagEditor(d);
				return;
			}
			// Scalar / single-enum attribute.
			s.mode = Mode::kEditAttr;
			s.edit_attr = name;
			s.edit_enum.clear();
			if (ad && ad->type == FieldType::kEnum && EnumRegistry::Instance().Known(ad->enum_type)) {
				ShowEnumChoices(ch, ad->enum_type, true);
				s.edit_enum = ad->enum_type;
			} else if (ad && ad->type == FieldType::kList && EnumRegistry::Instance().Known(ad->enum_type)) {
				ShowEnumChoices(ch, ad->enum_type, false);   // reference list ('|'-separated; '*' = any)
			}
			SendMsgToChar(fmt::format("New value for &g{}&n [{}] ('.' or blank cancels):\r\n",
				name, attrs[n - 1].second), ch);
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

void vedun_cleanup(DescriptorData *d) {
	d->vedun_session.reset();
	d->state = EConState::kPlaying;
	if (d->character) {
		SendMsgToChar("Vedun closed.\r\n", d->character.get());
	}
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
