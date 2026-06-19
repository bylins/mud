/**
\file feat_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.thing-names).
\brief In-game message container for feats: loader + EFeatMsg name registration.
*/

#include "feat_messages.h"
#include "utils/utils_string.h"   // str_cmp

#include <map>
#include <vector>

#include "engine/db/global_objects.h"

namespace {
// File-scope so NAMES_OF can expose it to the Vedun editor (mirrors skill_messages).
const std::map<EFeatMsg, std::string> kEFeatMsgNames{
		{EFeatMsg::kUndefined, "kUndefined"},
	};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<EFeatMsg>(const EFeatMsg item) {
	return kEFeatMsgNames.at(item);
}

template<>
const std::map<EFeatMsg, std::string> &NAMES_OF<EFeatMsg>() {
	return kEFeatMsgNames;
}

template<>
EFeatMsg ITEM_BY_NAME<EFeatMsg>(const std::string &name) {
	static const std::map<std::string, EFeatMsg> kMap{
		{"kUndefined", EFeatMsg::kUndefined},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

namespace feats {

void FeatMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::FeatMessages().Init(data.Children());
}

void FeatMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::FeatMessages().Reload(data.Children());
}

// issue.vedun-msg-editor: editor discovery + safe-commit validation.
std::string FeatMessagesLoader::EditableWhat() const {
	return "featmsg";
}

std::vector<cfg_manager::EditableElement> FeatMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::FeatMessages()) {
		const EFeat id = sheaf.GetId();
		const std::string id_str = (id == EFeat::kUndefined) ? "kDefault" : NAME_BY_ITEM<EFeat>(id);
		std::string label;
		if (id != EFeat::kUndefined && MUD::Feats().IsKnown(id)) {
			label = MUD::Feats()[id].GetName();
		}
		out.push_back({id_str, label});
	}
	return out;
}

cfg_manager::ValidationResult FeatMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::FeatMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Feat-message data failed to parse (see syslog for the offending sheaf/message)."};
}

std::string FeatMessagesLoader::CanonicalElementId(const std::string &id) const {
	// EFeat has no NAMES_OF specialisation, so round-trip through ITEM_BY_NAME/NAME_BY_ITEM to
	// recover the canonical token (or "" if `id` names no feat). ITEM_BY_NAME throws on unknown.
	try {
		const EFeat feat = ITEM_BY_NAME<EFeat>(id);
		if (feat != EFeat::kUndefined) {
			return NAME_BY_ITEM<EFeat>(feat);
		}
	} catch (const std::exception &) {
	}
	return "";
}

parser_wrapper::DataNode FeatMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// An empty sheaf for `id`; the editor then adds <message> children.
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace feats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
