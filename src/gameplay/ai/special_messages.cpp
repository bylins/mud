/**
\file special_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.specials, Phase 2).
\brief Special-procedure message containers: loaders + enum name registration.
*/

#include "special_messages.h"

#include "engine/structs/info_container.h"   // info_container::kUndefinedVnum
#include "engine/db/global_objects.h"

#include <map>
#include <string>
#include <vector>

namespace {
// File-scope so NAMES_OF can expose them to the Vedun editor (mirrors guild/skill messages).
const std::map<specials::ESpecialMsg, std::string> kSpecialMsgNames{
		{specials::ESpecialMsg::kAmbiguous, "kAmbiguous"},
		{specials::ESpecialMsg::kUnknownCommand, "kUnknownCommand"},
		{specials::ESpecialMsg::kNotHere, "kNotHere"},
	};

const std::map<specials::EBankMsg, std::string> kBankMsgNames{
		{specials::EBankMsg::kGreeting, "kGreeting"},
		{specials::EBankMsg::kBalance, "kBalance"},
		{specials::EBankMsg::kNoMoney, "kNoMoney"},
		{specials::EBankMsg::kDepositHowMuch, "kDepositHowMuch"},
		{specials::EBankMsg::kCantAfford, "kCantAfford"},
		{specials::EBankMsg::kDeposited, "kDeposited"},
		{specials::EBankMsg::kWithdrawHowMuch, "kWithdrawHowMuch"},
		{specials::EBankMsg::kNeverHadThatMuch, "kNeverHadThatMuch"},
		{specials::EBankMsg::kWithdrawn, "kWithdrawn"},
		{specials::EBankMsg::kFinancialOp, "kFinancialOp"},
		{specials::EBankMsg::kImmCant, "kImmCant"},
		{specials::EBankMsg::kTransferToWhom, "kTransferToWhom"},
		{specials::EBankMsg::kNoTaxMoney, "kNoTaxMoney"},
		{specials::EBankMsg::kTransferSent, "kTransferSent"},
		{specials::EBankMsg::kTransferReceived, "kTransferReceived"},
		{specials::EBankMsg::kNoSuchPlayer, "kNoSuchPlayer"},
		{specials::EBankMsg::kNotInClan, "kNotInClan"},
		{specials::EBankMsg::kClanBalance, "kClanBalance"},
		{specials::EBankMsg::kClanDepositFormat, "kClanDepositFormat"},
		{specials::EBankMsg::kClanDepositPartial, "kClanDepositPartial"},
		{specials::EBankMsg::kClanNoWithdraw, "kClanNoWithdraw"},
		{specials::EBankMsg::kClanTooPoor, "kClanTooPoor"},
		{specials::EBankMsg::kClanWithdrawPartial, "kClanWithdrawPartial"},
		{specials::EBankMsg::kClanFormat, "kClanFormat"},
	};

// Editable msg-sheaf loaders share the kDefault-only element model (one shared sheaf per file).
std::vector<cfg_manager::EditableElement> DefaultSheafElements(
		const std::string &label, std::size_t count) {
	std::vector<cfg_manager::EditableElement> out;
	out.reserve(count);
	out.push_back({"kDefault", label});
	return out;
}

std::string CanonicalSheafId(const std::string &id) {
	if (id == "kDefault") {
		return "kDefault";
	}
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {     // a per-mob override sheaf would be keyed by a non-negative vnum
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode CreateSheaf(parser_wrapper::DataNode root, const std::string &id) {
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}
}  // namespace

template<>
const std::string &NAME_BY_ITEM<specials::ESpecialMsg>(const specials::ESpecialMsg item) {
	return kSpecialMsgNames.at(item);
}
template<>
const std::map<specials::ESpecialMsg, std::string> &NAMES_OF<specials::ESpecialMsg>() {
	return kSpecialMsgNames;
}
template<>
specials::ESpecialMsg ITEM_BY_NAME<specials::ESpecialMsg>(const std::string &name) {
	static std::map<std::string, specials::ESpecialMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kSpecialMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<specials::EBankMsg>(const specials::EBankMsg item) {
	return kBankMsgNames.at(item);
}
template<>
const std::map<specials::EBankMsg, std::string> &NAMES_OF<specials::EBankMsg>() {
	return kBankMsgNames;
}
template<>
specials::EBankMsg ITEM_BY_NAME<specials::EBankMsg>(const std::string &name) {
	static std::map<std::string, specials::EBankMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kBankMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace specials {

const std::string &SpecialMsg(ESpecialMsg id) {
	return MUD::SpecialMessages().GetMessage(info_container::kUndefinedVnum, id);
}

const std::string &BankMsg(EBankMsg id) {
	return MUD::BankMessages().GetMessage(info_container::kUndefinedVnum, id);
}

// --- shared dispatch messages (special_msg.xml) -----------------------------------------------------
void SpecialMessagesLoader::Load(parser_wrapper::DataNode data) { MUD::SpecialMessages().Init(data.Children()); }
void SpecialMessagesLoader::Reload(parser_wrapper::DataNode data) { MUD::SpecialMessages().Reload(data.Children()); }
std::string SpecialMessagesLoader::EditableWhat() const { return "specialmsg"; }
std::vector<cfg_manager::EditableElement> SpecialMessagesLoader::ListElements() const {
	return DefaultSheafElements("shared special-procedure messages", 1);
}
cfg_manager::ValidationResult SpecialMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::SpecialMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Special-message data failed to parse (see syslog for the offending message)."};
}
std::string SpecialMessagesLoader::CanonicalElementId(const std::string &id) const { return CanonicalSheafId(id); }
parser_wrapper::DataNode SpecialMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	return CreateSheaf(root, id);
}

// --- bank + клановая казна messages (bank_msg.xml) --------------------------------------------------
void BankMessagesLoader::Load(parser_wrapper::DataNode data) { MUD::BankMessages().Init(data.Children()); }
void BankMessagesLoader::Reload(parser_wrapper::DataNode data) { MUD::BankMessages().Reload(data.Children()); }
std::string BankMessagesLoader::EditableWhat() const { return "bankmsg"; }
std::vector<cfg_manager::EditableElement> BankMessagesLoader::ListElements() const {
	return DefaultSheafElements("bank + treasury messages (shared)", 1);
}
cfg_manager::ValidationResult BankMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::BankMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Bank-message data failed to parse (see syslog for the offending message)."};
}
std::string BankMessagesLoader::CanonicalElementId(const std::string &id) const { return CanonicalSheafId(id); }
parser_wrapper::DataNode BankMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	return CreateSheaf(root, id);
}

} // namespace specials

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
