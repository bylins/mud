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
#include "utils/mud_string.h"                 // two_arguments

#include <fmt/format.h>

#include <cctype>

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

void Render(DescriptorData *d) {
	auto *ch = d->character.get();
	const auto &s = *d->vedun_session;
	parser_wrapper::DataNode cur = s.path.back();   // copy: keep the stored cursor stable

	std::string crumb;
	for (std::size_t i = 0; i < s.path.size(); ++i) {
		crumb += (i ? "/" : "");
		crumb += s.path[i].GetName();
	}
	SendMsgToChar(fmt::format("&WVedun&n [{}]  &c{}&n  (read-only)\r\n", s.what, crumb), ch);

	const auto attrs = cur.Attributes();
	if (!attrs.empty()) {
		table_wrapper::Table table;
		for (const auto &[name, value] : attrs) {
			table << name << value << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
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
			table << fmt::format("{})", i + 1)
				  << fmt::format("<{}>", kids[i].GetName())
				  << ChildHint(kids[i]) << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToChar(ch, table);
	}
	SendMsgToChar("&WSelect&n: a number to descend, &Wu&n) up, &Wq&n) quit\r\n", ch);
}

} // namespace

void do_vedun(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	auto *d = ch->desc;
	if (!d) {
		return;
	}
	char what[kMaxInputLength], element[kMaxInputLength];
	two_arguments(argument, what, element);

	// `vedun` -- list the editable data sets.
	if (!*what) {
		SendMsgToChar("&WVedun&n -- editable data sets:\r\n", ch);
		table_wrapper::Table table;
		for (const auto &entry : MUD::CfgManager().EditableEntries()) {
			table << entry.what << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToChar(ch, table);
		SendMsgToChar("Usage: vedun <what> [element]\r\n", ch);
		return;
	}

	const auto entry = MUD::CfgManager().FindEditable(what);
	if (!entry) {
		SendMsgToChar(fmt::format("Vedun: unknown data set '{}'.\r\n", what), ch);
		return;
	}

	// `vedun <what>` -- list the editable elements.
	if (!*element) {
		SendMsgToChar(fmt::format("&WVedun&n [{}] -- elements:\r\n", entry->what), ch);
		table_wrapper::Table table;
		for (const auto &el : entry->loader->ListElements()) {
			table << el.id << el.label << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToChar(ch, table);
		return;
	}

	// `vedun <what> <element>` -- open the read-only viewer on that element's DOM. The element is
	// the direct child of the file root whose `id` attribute matches (true for spells; generalised
	// later via the loader/scheme).
	parser_wrapper::DataNode doc(entry->file);
	parser_wrapper::DataNode found;
	bool ok = false;
	for (auto &child : doc.Children()) {
		if (std::string(child.GetValue("id")) == element) {
			found = child;
			ok = true;
			break;
		}
	}
	if (!ok) {
		SendMsgToChar(fmt::format("Vedun: element '{}' not found in '{}'.\r\n", element, entry->what), ch);
		return;
	}

	auto session = std::make_shared<Session>();
	session->what = entry->what;
	session->file = entry->file;
	session->loader = entry->loader;
	session->doc = std::move(doc);
	session->path.push_back(found);
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
	while (*arg && std::isspace(static_cast<unsigned char>(*arg))) {
		++arg;
	}

	if (*arg == 'q' || *arg == 'Q') {
		vedun_cleanup(d);
		return;
	}
	if (*arg == 'u' || *arg == 'U' || *arg == '0') {
		if (s.path.size() > 1) {
			s.path.pop_back();
			Render(d);
		} else {
			vedun_cleanup(d);
		}
		return;
	}
	const int n = atoi(arg);
	if (n > 0) {
		const auto kids = ChildrenOf(s.path.back());
		if (n <= static_cast<int>(kids.size())) {
			s.path.push_back(kids[n - 1]);
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
