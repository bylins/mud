/**
\file religion.cpp - a part of the Bylins engine.
\brief Those a character prays or sacrifices to (issue #2689), from cfg/mechanics/religion.xml.
*/

#include "gameplay/mechanics/religion.h"

#include "engine/db/global_objects.h"
#include "engine/entities/entities_constants.h"
#include "utils/logger.h"
#include "utils/utils_parse.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <cstring>

namespace religion {

namespace {

// The religion attribute: who prays to this recipient. -1 if `value` names no religion.
int ParseReligion(const char *value) {
	if (value && std::strcmp(value, "kPoly") == 0) {
		return kReligionPoly;
	}
	if (value && std::strcmp(value, "kMono") == 0) {
		return kReligionMono;
	}
	return -1;
}

} // namespace

void Recipients::Build(parser_wrapper::DataNode &root) {
	recipients_.clear();
	for (auto node : root.Children("recipient")) {
		Recipient recipient;
		const char *name = node.GetValue("name");
		if (!name || !*name) {
			err_log("religion.xml: у получателя молитвы нет имени (name), он пропущен.");
			continue;
		}
		recipient.name = name;
		recipient.religion = ParseReligion(node.GetValue("religion"));
		if (recipient.religion < 0) {
			err_log("religion.xml: у '%s' неверная религия (нужно kPoly или kMono), он пропущен.", name);
			continue;
		}
		try {
			recipient.spell = parse::ReadAsConstant<ESpell>(node.GetValue("spell"));
		} catch (const std::exception &) {
			recipient.spell = ESpell::kUndefined;
		}
		if (recipient.spell <= ESpell::kUndefined) {
			err_log("religion.xml: у '%s' неверное заклинание (spell), он пропущен.", name);
			continue;
		}
		const auto same_name = [&recipient](const Recipient &other) { return other.name == recipient.name; };
		if (std::any_of(recipients_.begin(), recipients_.end(), same_name)) {
			err_log("religion.xml: '%s' указан дважды, повтор пропущен.", name);
			continue;
		}
		recipients_.push_back(std::move(recipient));
	}
}

const Recipient *Recipients::Find(const std::string &abbr) const {
	if (abbr.empty()) {
		return nullptr;
	}
	for (const auto &recipient : recipients_) {
		if (utils::IsAbbr(abbr, recipient.name)) {
			return &recipient;
		}
	}
	return nullptr;
}

Recipients &RecipientsRegistry() {
	static Recipients recipients;
	return recipients;
}

namespace {

void LoadRecipients(parser_wrapper::DataNode &data) {
	RecipientsRegistry().Build(data);
	for (const auto &recipient : RecipientsRegistry().All()) {
		if (!MUD::Spells().IsKnown(recipient.spell)) {
			err_log("religion.xml: заклинания %s у '%s' нет в spells.xml, молитва не подействует.",
					NAME_BY_ITEM<ESpell>(recipient.spell).c_str(), recipient.name.c_str());
		}
	}
}

} // namespace

void RecipientsLoader::Load(parser_wrapper::DataNode data) {
	LoadRecipients(data);
}

void RecipientsLoader::Reload(parser_wrapper::DataNode data) {
	LoadRecipients(data);
}

} // namespace religion

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
