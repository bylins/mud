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

const std::map<specials::EMailMsg, std::string> kMailMsgNames{
		{specials::EMailMsg::kGreeting, "kGreeting"},
		{specials::EMailMsg::kMailWaiting, "kMailWaiting"},
		{specials::EMailMsg::kParcelWaiting, "kParcelWaiting"},
		{specials::EMailMsg::kNothingToday, "kNothingToday"},
		{specials::EMailMsg::kNoLetters, "kNoLetters"},
		{specials::EMailMsg::kLevelTooLow, "kLevelTooLow"},
		{specials::EMailMsg::kTooManyMessages, "kTooManyMessages"},
		{specials::EMailMsg::kNoRecipient, "kNoRecipient"},
		{specials::EMailMsg::kNotRegistered, "kNotRegistered"},
		{specials::EMailMsg::kNoParcelHere, "kNoParcelHere"},
		{specials::EMailMsg::kParcelError, "kParcelError"},
		{specials::EMailMsg::kCantAffordCost, "kCantAffordCost"},
		{specials::EMailMsg::kCantAffordNoMoney, "kCantAffordNoMoney"},
		{specials::EMailMsg::kStartWriting, "kStartWriting"},
		{specials::EMailMsg::kFreePostage, "kFreePostage"},
		{specials::EMailMsg::kPostageCharged, "kPostageCharged"},
		{specials::EMailMsg::kCanWrite, "kCanWrite"},
		{specials::EMailMsg::kLetterGiven, "kLetterGiven"},
		{specials::EMailMsg::kLetterGivenRoom, "kLetterGivenRoom"},
		{specials::EMailMsg::kUndeliveredHeader, "kUndeliveredHeader"},
		{specials::EMailMsg::kUndeliveredCount, "kUndeliveredCount"},
		{specials::EMailMsg::kUndeliveredNames, "kUndeliveredNames"},
	};

const std::map<specials::EHorseMsg, std::string> kHorseMsgNames{
		{specials::EHorseMsg::kGreeting, "kGreeting"},
		{specials::EHorseMsg::kAlreadyHave, "kAlreadyHave"},
		{specials::EHorseMsg::kForSale, "kForSale"},
		{specials::EHorseMsg::kBuyHaveAlready, "kBuyHaveAlready"},
		{specials::EHorseMsg::kBuyNoMoney, "kBuyNoMoney"},
		{specials::EHorseMsg::kBuyNoHorse, "kBuyNoHorse"},
		{specials::EHorseMsg::kBuyGiveChar, "kBuyGiveChar"},
		{specials::EHorseMsg::kBuyGiveRoom, "kBuyGiveRoom"},
		{specials::EHorseMsg::kSellNoHorse, "kSellNoHorse"},
		{specials::EHorseMsg::kSellOnHorse, "kSellOnHorse"},
		{specials::EHorseMsg::kSellWrongHorse, "kSellWrongHorse"},
		{specials::EHorseMsg::kSellHorseAway, "kSellHorseAway"},
		{specials::EHorseMsg::kSellTaken, "kSellTaken"},
	};

const std::map<specials::ETorcMsg, std::string> kTorcMsgNames{
		{specials::ETorcMsg::kNotEnough, "kNotEnough"},
		{specials::ETorcMsg::kReducedTo, "kReducedTo"},
		{specials::ETorcMsg::kExchanged, "kExchanged"},
		{specials::ETorcMsg::kInvalidChoice, "kInvalidChoice"},
		{specials::ETorcMsg::kMinAmount, "kMinAmount"},
		{specials::ETorcMsg::kCancelled, "kCancelled"},
		{specials::ETorcMsg::kTechError, "kTechError"},
		{specials::ETorcMsg::kConfirmed, "kConfirmed"},
		{specials::ETorcMsg::kRatesHeader, "kRatesHeader"},
		{specials::ETorcMsg::kRateBronze, "kRateBronze"},
		{specials::ETorcMsg::kRateSilver, "kRateSilver"},
		{specials::ETorcMsg::kBalance, "kBalance"},
		{specials::ETorcMsg::kRow, "kRow"},
		{specials::ETorcMsg::kLabelBronze, "kLabelBronze"},
		{specials::ETorcMsg::kLabelSilver, "kLabelSilver"},
		{specials::ETorcMsg::kLabelGold, "kLabelGold"},
		{specials::ETorcMsg::kInstrMin, "kInstrMin"},
		{specials::ETorcMsg::kInstrAmount, "kInstrAmount"},
		{specials::ETorcMsg::kOptCancel, "kOptCancel"},
		{specials::ETorcMsg::kOptConfirm, "kOptConfirm"},
		{specials::ETorcMsg::kPrompt, "kPrompt"},
	};

const std::map<specials::EMercMsg, std::string> kMercMsgNames{
		{specials::EMercMsg::kNoAccess, "kNoAccess"},
		{specials::EMercMsg::kUnknownCmd, "kUnknownCmd"},
		{specials::EMercMsg::kEmptyEmployer, "kEmptyEmployer"},
		{specials::EMercMsg::kEmptyCharmer, "kEmptyCharmer"},
		{specials::EMercMsg::kEmptyImmortal, "kEmptyImmortal"},
		{specials::EMercMsg::kListImmortalShort, "kListImmortalShort"},
		{specials::EMercMsg::kListImmortalFull, "kListImmortalFull"},
		{specials::EMercMsg::kListEmployerShort, "kListEmployerShort"},
		{specials::EMercMsg::kListEmployerFull, "kListEmployerFull"},
		{specials::EMercMsg::kListCharmerShort, "kListCharmerShort"},
		{specials::EMercMsg::kListCharmerFull, "kListCharmerFull"},
		{specials::EMercMsg::kTableHeader, "kTableHeader"},
		{specials::EMercMsg::kListTotal, "kListTotal"},
		{specials::EMercMsg::kUnknownChar, "kUnknownChar"},
		{specials::EMercMsg::kTooExpensive, "kTooExpensive"},
		{specials::EMercMsg::kCantFind, "kCantFind"},
		{specials::EMercMsg::kSummonHideCharm, "kSummonHideCharm"},
		{specials::EMercMsg::kSummonBackCharm, "kSummonBackCharm"},
		{specials::EMercMsg::kSummonHide, "kSummonHide"},
		{specials::EMercMsg::kSummonBack, "kSummonBack"},
		{specials::EMercMsg::kRefuseMale, "kRefuseMale"},
		{specials::EMercMsg::kRefuseFemale, "kRefuseFemale"},
		{specials::EMercMsg::kFavAdded, "kFavAdded"},
		{specials::EMercMsg::kFavRemoved, "kFavRemoved"},
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

template<>
const std::string &NAME_BY_ITEM<specials::EMailMsg>(const specials::EMailMsg item) {
	return kMailMsgNames.at(item);
}
template<>
const std::map<specials::EMailMsg, std::string> &NAMES_OF<specials::EMailMsg>() {
	return kMailMsgNames;
}
template<>
specials::EMailMsg ITEM_BY_NAME<specials::EMailMsg>(const std::string &name) {
	static std::map<std::string, specials::EMailMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kMailMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<specials::EHorseMsg>(const specials::EHorseMsg item) {
	return kHorseMsgNames.at(item);
}
template<>
const std::map<specials::EHorseMsg, std::string> &NAMES_OF<specials::EHorseMsg>() {
	return kHorseMsgNames;
}
template<>
specials::EHorseMsg ITEM_BY_NAME<specials::EHorseMsg>(const std::string &name) {
	static std::map<std::string, specials::EHorseMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kHorseMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}


template<>
const std::string &NAME_BY_ITEM<specials::ETorcMsg>(const specials::ETorcMsg item) {
	return kTorcMsgNames.at(item);
}
template<>
const std::map<specials::ETorcMsg, std::string> &NAMES_OF<specials::ETorcMsg>() {
	return kTorcMsgNames;
}
template<>
specials::ETorcMsg ITEM_BY_NAME<specials::ETorcMsg>(const std::string &name) {
	static std::map<std::string, specials::ETorcMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kTorcMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}


template<>
const std::string &NAME_BY_ITEM<specials::EMercMsg>(const specials::EMercMsg item) {
	return kMercMsgNames.at(item);
}
template<>
const std::map<specials::EMercMsg, std::string> &NAMES_OF<specials::EMercMsg>() {
	return kMercMsgNames;
}
template<>
specials::EMercMsg ITEM_BY_NAME<specials::EMercMsg>(const std::string &name) {
	static std::map<std::string, specials::EMercMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kMercMsgNames) {
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

const std::string &MailMsg(EMailMsg id) {
	return MUD::MailMessages().GetMessage(info_container::kUndefinedVnum, id);
}

const std::string &HorseMsg(EHorseMsg id) {
	return MUD::HorseMessages().GetMessage(info_container::kUndefinedVnum, id);
}

const std::string &TorcMsg(ETorcMsg id) {
	return MUD::TorcMessages().GetMessage(info_container::kUndefinedVnum, id);
}

const std::string &MercMsg(EMercMsg id) {
	return MUD::MercMessages().GetMessage(info_container::kUndefinedVnum, id);
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

// --- mail / post office messages (mail_msg.xml) -----------------------------------------------------
void MailMessagesLoader::Load(parser_wrapper::DataNode data) { MUD::MailMessages().Init(data.Children()); }
void MailMessagesLoader::Reload(parser_wrapper::DataNode data) { MUD::MailMessages().Reload(data.Children()); }
std::string MailMessagesLoader::EditableWhat() const { return "mailmsg"; }
std::vector<cfg_manager::EditableElement> MailMessagesLoader::ListElements() const {
	return DefaultSheafElements("mail / post office messages (shared)", 1);
}
cfg_manager::ValidationResult MailMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::MailMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Mail-message data failed to parse (see syslog for the offending message)."};
}
std::string MailMessagesLoader::CanonicalElementId(const std::string &id) const { return CanonicalSheafId(id); }
parser_wrapper::DataNode MailMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	return CreateSheaf(root, id);
}

// --- horse keeper messages (horse_msg.xml) ----------------------------------------------------------
void HorseMessagesLoader::Load(parser_wrapper::DataNode data) { MUD::HorseMessages().Init(data.Children()); }
void HorseMessagesLoader::Reload(parser_wrapper::DataNode data) { MUD::HorseMessages().Reload(data.Children()); }
std::string HorseMessagesLoader::EditableWhat() const { return "horsemsg"; }
std::vector<cfg_manager::EditableElement> HorseMessagesLoader::ListElements() const {
	return DefaultSheafElements("horse keeper messages (shared)", 1);
}
cfg_manager::ValidationResult HorseMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::HorseMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Horse-message data failed to parse (see syslog for the offending message)."};
}
std::string HorseMessagesLoader::CanonicalElementId(const std::string &id) const { return CanonicalSheafId(id); }
parser_wrapper::DataNode HorseMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	return CreateSheaf(root, id);
}

// --- torc / гривна exchange messages (torc_msg.xml) -------------------------------------------------
void TorcMessagesLoader::Load(parser_wrapper::DataNode data) { MUD::TorcMessages().Init(data.Children()); }
void TorcMessagesLoader::Reload(parser_wrapper::DataNode data) { MUD::TorcMessages().Reload(data.Children()); }
std::string TorcMessagesLoader::EditableWhat() const { return "torcmsg"; }
std::vector<cfg_manager::EditableElement> TorcMessagesLoader::ListElements() const {
	return DefaultSheafElements("torc exchange messages (shared)", 1);
}
cfg_manager::ValidationResult TorcMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::TorcMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Torc-message data failed to parse (see syslog for the offending message)."};
}
std::string TorcMessagesLoader::CanonicalElementId(const std::string &id) const { return CanonicalSheafId(id); }
parser_wrapper::DataNode TorcMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	return CreateSheaf(root, id);
}

// --- mercenary boss messages (mercenary_msg.xml) ----------------------------------------------------
void MercMessagesLoader::Load(parser_wrapper::DataNode data) { MUD::MercMessages().Init(data.Children()); }
void MercMessagesLoader::Reload(parser_wrapper::DataNode data) { MUD::MercMessages().Reload(data.Children()); }
std::string MercMessagesLoader::EditableWhat() const { return "mercmsg"; }
std::vector<cfg_manager::EditableElement> MercMessagesLoader::ListElements() const {
	return DefaultSheafElements("mercenary boss messages (shared)", 1);
}
cfg_manager::ValidationResult MercMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::MercMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Mercenary-message data failed to parse (see syslog for the offending message)."};
}
std::string MercMessagesLoader::CanonicalElementId(const std::string &id) const { return CanonicalSheafId(id); }
parser_wrapper::DataNode MercMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	return CreateSheaf(root, id);
}

} // namespace specials

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
