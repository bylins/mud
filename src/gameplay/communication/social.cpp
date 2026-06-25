/* ************************************************************************
*   File: social.cpp                                    Part of Bylins    *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  issue.socials: socials are now vnum-keyed data-driven entities loaded  *
*  from cfg/messages/<locale>/social_msg.xml via cfg_manager (see         *
*  SocialsLoader / MUD::Socials()), replacing the old lib/misc/socials    *
*  flat file and the DB_BOOT_SOCIAL boot path.                            *
************************************************************************ */

#include "social.h"

#include "engine/entities/char_data.h"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "gameplay/communication/ignores.h"
#include "utils/logger.h"
#include "utils/mud_string.h"
#include "utils/utils_parse.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <cstring>

// ---------------------------------------------------------------------------
// ESocialMsg meta-enum (parsing + Vedun registry)
// ---------------------------------------------------------------------------
namespace {
const std::map<ESocialMsg, std::string> kSocialMsgNames{
		{ESocialMsg::kUndefined, "kUndefined"},
		{ESocialMsg::kCharNoArg, "kCharNoArg"},
		{ESocialMsg::kOthersNoArg, "kOthersNoArg"},
		{ESocialMsg::kCharFound, "kCharFound"},
		{ESocialMsg::kOthersFound, "kOthersFound"},
		{ESocialMsg::kVictFound, "kVictFound"},
		{ESocialMsg::kNotFound, "kNotFound"},
	};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<ESocialMsg>(const ESocialMsg item) {
	return kSocialMsgNames.at(item);
}

template<>
const std::map<ESocialMsg, std::string> &NAMES_OF<ESocialMsg>() {
	return kSocialMsgNames;
}

template<>
ESocialMsg ITEM_BY_NAME<ESocialMsg>(const std::string &name) {
	static std::map<std::string, ESocialMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kSocialMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace communication::social {

const std::string &SocialInfo::GetMsg(ESocialMsg type) const {
	static const std::string kEmpty;
	const auto it = messages_.find(type);
	return it == messages_.end() ? kEmpty : it->second;
}

SocialInfoBuilder::ItemPtr SocialInfoBuilder::Build(parser_wrapper::DataNode &node) {
	int vnum;
	try {
		vnum = parse::ReadAsInt(node.GetValue("vnum"));
	} catch (const std::exception &e) {
		err_log("SocialInfoBuilder: bad or missing 'vnum' (%s).", e.what());
		return nullptr;
	}

	// keyword synonyms (pipe-separated)
	std::vector<std::string> keywords;
	if (const char *kw = node.GetValue("keywords"); kw && *kw) {
		const std::string s(kw);
		std::size_t start = 0;
		while (start <= s.size()) {
			const std::size_t pos = s.find('|', start);
			std::string tok = (pos == std::string::npos) ? s.substr(start) : s.substr(start, pos - start);
			if (!tok.empty()) {
				keywords.push_back(std::move(tok));
			}
			if (pos == std::string::npos) {
				break;
			}
			start = pos + 1;
		}
	}
	std::string text_id = keywords.empty() ? std::string("!social!") : keywords.front();
	auto info = std::make_shared<SocialInfo>(vnum, text_id, EItemMode::kEnabled);
	info->keywords_ = std::move(keywords);

	const auto read_pos = [&node](const char *attr, EPosition def) -> EPosition {
		const char *v = node.GetValue(attr);
		if (!v || !*v) {
			return def;
		}
		try {
			return parse::ReadAsConstant<EPosition>(v);
		} catch (const std::exception &) {
			return def;
		}
	};
	info->ch_min_pos_ = read_pos("ch_min", EPosition::kStand);
	info->ch_max_pos_ = read_pos("ch_max", EPosition::kStand);
	info->vict_min_pos_ = read_pos("vict_min", EPosition::kStand);
	info->vict_max_pos_ = read_pos("vict_max", EPosition::kStand);

	for (auto &m : node.Children("message")) {
		const char *ts = m.GetValue("type");
		try {
			const ESocialMsg t = parse::ReadAsConstant<ESocialMsg>(ts);
			const char *val = m.GetValue("val");
			// Preserve the legacy &K...&n colour wrapping (was applied at load time).
			info->messages_[t] = std::string("&K") + (val ? val : "") + "&n";
		} catch (const std::exception &) {
			err_log("SocialInfoBuilder: social vnum %d has unknown message type '%s'.", vnum, ts ? ts : "(missing)");
		}
	}
	return info;
}

} // namespace communication::social

// ---------------------------------------------------------------------------
// Keyword -> vnum search index (multi-synonym, prefix-matched). Rebuilt on (re)load.
// ---------------------------------------------------------------------------
namespace {
std::vector<std::pair<std::string, int>> g_social_index;   // (keyword, vnum), sorted by keyword

void RebuildSocialIndex() {
	g_social_index.clear();
	for (const auto &soc : MUD::Socials()) {
		if (soc.GetId() < 0) {
			continue;
		}
		for (const auto &kw : soc.GetKeywords()) {
			g_social_index.emplace_back(kw, soc.GetId());
		}
	}
	std::sort(g_social_index.begin(), g_social_index.end(),
			  [](const auto &a, const auto &b) { return a.first < b.first; });
}
}  // namespace

namespace communication::social {

void SocialsLoader::Load(parser_wrapper::DataNode data) {
	MUD::Socials().Init(data.Children());
	RebuildSocialIndex();
}

void SocialsLoader::Reload(parser_wrapper::DataNode data) {
	MUD::Socials().Reload(data.Children());
	RebuildSocialIndex();
}

std::string SocialsLoader::EditableWhat() const {
	return "social";
}

std::vector<cfg_manager::EditableElement> SocialsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &soc : MUD::Socials()) {
		if (soc.GetId() < 0) {
			continue;
		}
		std::string label;
		for (const auto &kw : soc.GetKeywords()) {
			if (!label.empty()) {
				label += " ";
			}
			label += kw;
		}
		out.push_back({std::to_string(soc.GetId()), label});
	}
	return out;
}

cfg_manager::ValidationResult SocialsLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::Socials().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Social data failed to parse (see syslog for the offending <social>)."};
}

parser_wrapper::DataNode SocialsLoader::FindElementNode(parser_wrapper::DataNode root,
													   const std::string &id) const {
	// A <social> is keyed by its integer `vnum` (no `id` attribute). Iterate un-keyed so the
	// returned node carries no Children(key) filter (see the guild editor fix).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "social" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string SocialsLoader::CanonicalElementId(const std::string &id) const {
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode SocialsLoader::CreateElementNode(parser_wrapper::DataNode root,
														 const std::string &id) const {
	auto node = root.AddChild("social");
	node.SetValue("vnum", id);
	node.SetValue("keywords", "новый");
	node.SetValue("ch_min", "kStand");
	node.SetValue("ch_max", "kStand");
	node.SetValue("vict_min", "kStand");
	node.SetValue("vict_max", "kStand");
	return node;
}

} // namespace communication::social

// ---------------------------------------------------------------------------
// Command dispatch (public interface, unchanged signatures)
// ---------------------------------------------------------------------------
int find_action(char *cmd) {
	if (!cmd || !*cmd) {
		return -1;
	}
	const std::size_t len = std::strlen(cmd);
	for (const auto &entry : g_social_index) {
		if (strn_cmp(cmd, entry.first.c_str(), len) == 0) {
			return entry.second;
		}
	}
	return -1;
}

const char *deaf_social = "&K$n попытал$u очень эмоционально выразить свою мысль.&n";

int do_social(CharData *ch, char *argument) {
	char social[kMaxInputLength];
	char target[kMaxInputLength];

	if (!argument || !*argument) {
		return false;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Боги наказали вас и вы не можете выражать эмоции!\r\n", ch);
		return false;
	}

	argument = one_argument(argument, social);

	const int vnum = find_action(social);
	if (vnum < 0) {
		return false;
	}

	const auto &action = MUD::Socials()[vnum];
	if (ch->GetPosition() < action.GetChMinPos() || ch->GetPosition() > action.GetChMaxPos()) {
		SendMsgToChar("Вам крайне неудобно это сделать.\r\n", ch);
		return true;
	}

	if (action.HasMsg(ESocialMsg::kCharFound) && argument) {
		one_argument(argument, target);
	} else {
		*target = '\0';
	}

	if (!*target) {
		SendMsgToChar(action.GetMsg(ESocialMsg::kCharNoArg).c_str(), ch);
		SendMsgToChar("\r\n", ch);
		for (const auto to : world[ch->in_room]->people) {
			if (to == ch || ignores(to, ch, EIgnore::kEmote)) {
				continue;
			}
			act(action.GetMsg(ESocialMsg::kOthersNoArg).c_str(), false, ch, nullptr, to, kToVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, to, kToVict | kToDeaf);
		}
		return true;
	}

	CharData *vict = target_resolver::FindCharInRoom(ch, target);
	if (!vict) {
		const char *message = action.HasMsg(ESocialMsg::kNotFound)
							  ? action.GetMsg(ESocialMsg::kNotFound).c_str()
							  : "Поищите кого-нибудь более доступного для этих целей.\r\n";
		SendMsgToChar(message, ch);
		SendMsgToChar("\r\n", ch);
	} else if (vict == ch) {
		SendMsgToChar(action.GetMsg(ESocialMsg::kCharNoArg).c_str(), ch);
		SendMsgToChar("\r\n", ch);
		for (const auto to : world[ch->in_room]->people) {
			if (to == ch || ignores(to, ch, EIgnore::kEmote)) {
				continue;
			}
			act(action.GetMsg(ESocialMsg::kOthersNoArg).c_str(), false, ch, nullptr, to, kToVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, to, kToVict | kToDeaf);
		}
	} else {
		if (vict->GetPosition() < action.GetVictMinPos() || vict->GetPosition() > action.GetVictMaxPos()) {
			act("$N2 сейчас, похоже, не до вас.", false, ch, nullptr, vict, kToChar | kToSleep);
		} else {
			act(action.GetMsg(ESocialMsg::kCharFound).c_str(), false, ch, nullptr, vict, kToChar | kToSleep);
			act(action.GetMsg(ESocialMsg::kOthersFound).c_str(), false, ch, nullptr, vict, kToNotVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, nullptr, kToRoom | kToDeaf);
			if (!ignores(vict, ch, EIgnore::kEmote)) {
				act(action.GetMsg(ESocialMsg::kVictFound).c_str(), false, ch, nullptr, vict, kToVict | kToNotDeaf);
			}
		}
	}
	return true;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
