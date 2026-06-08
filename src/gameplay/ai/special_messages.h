/**
\file special_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.specials, Phase 2).
\brief In-game message containers for special procedures (bank, mail, horse, ...).
\details Phase 2 of issue.specials moves the player-facing text out of the spec-proc code into
		 per-locale XML, mirroring the guild/skill/spell message systems. Each special procedure has
		 its own message enum and its own data file (cfg/messages/ru/<name>_msg.xml); the messages
		 SHARED by the dispatch framework (the resolver's "unknown sub-command" / ambiguity replies and
		 RunSpecCommand's wrong-place reply) live in special_msg.xml under specials::ESpecialMsg.

		 Every container is a vnum-keyed msg_container::MsgContainer<int, EXxxMsg> with a single shared
		 sheaf (XML id "kDefault" => info_container::kUndefinedVnum); the int key leaves room for future
		 per-mob overrides. Containers are exposed via MUD::SpecialMessages() / MUD::BankMessages().
*/

#ifndef BYLINS_SRC_GAMEPLAY_AI_SPECIAL_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_AI_SPECIAL_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"

#include <map>
#include <string>
#include <vector>

namespace specials {

// Shared dispatch messages (special_msg.xml). kAmbiguous is a prefix; the dynamic tooltip and the
// trailing newline are appended by the resolver. kUnknownCommand/kNotHere are sent as whole lines.
enum class ESpecialMsg {
	kAmbiguous,        // "Уточните, что именно:" + (tooltip)
	kUnknownCommand,   // "Вам явно в какое-то другое заведение."
	kNotHere,          // "Это нельзя сделать здесь." (RunSpecCommand: no carrier in the room)
};

// Bank + clan-treasury (казна) messages (bank_msg.xml). Money amounts use {amount}/{currency} named
// fmt args; the transfer lines add {color}/{nocolor}/{recipient}/{sender}; kFinancialOp is an act()
// line ($n act-codes). Several bank slots are reused by the казна handler (Clan::BankManage).
enum class EBankMsg {
	kGreeting,           // resolver greeting: "Да-да, тут банк, чего изволите?"
	kBalance,            // "У вас на счету {amount} {currency}."
	kNoMoney,            // "У вас нет денег."
	kDepositHowMuch,     // "Сколько вы хотите вложить?"
	kCantAfford,         // "О такой сумме вы можете только мечтать!"
	kDeposited,          // "Вы вложили {amount} {currency}."
	kWithdrawHowMuch,    // "Уточните количество денег, которые вы хотите получить?"
	kNeverHadThatMuch,   // "Да вы отродясь столько денег не видели!"
	kWithdrawn,          // "Вы сняли {amount} {currency}."
	kFinancialOp,        // act to room: "$n произвел$g финансовую операцию."
	kImmCant,            // "Почитить захотелось?"
	kTransferToWhom,     // "Уточните кому вы хотите перевести?"
	kNoTaxMoney,         // "У вас не хватит денег на налоги!"
	kTransferSent,       // "{color}Вы перевели {amount} кун {recipient}{nocolor}."
	kTransferReceived,   // "{color}Вы получили {amount} кун банковским переводом от {sender}{nocolor}."
	kNoSuchPlayer,       // "Такого персонажа не существует."
	kNotInClan,          // "Вы не состоите в дружине."
	// казна (Clan::BankManage)
	kClanBalance,        // "На счету вашей дружины ровно {amount} {currency}."
	kClanDepositFormat,  // "Формат команды казна вложить <число>"
	kClanDepositPartial, // "Вам удалось вложить в казну дружины только {amount} {currency}."
	kClanNoWithdraw,     // "К сожалению, у вас нет возможности транжирить средства дружины."
	kClanTooPoor,        // "К сожалению, ваша дружина не так богата."
	kClanWithdrawPartial,// "Вам удалось снять только {amount} {currency}."
	kClanFormat,         // "Формат команды: казна вложить|получить|баланс сумма."
};

// Mail / post office (mail_msg.xml). Mostly act() lines ($n/$N codes, to vict/room). kLevelTooLow
// uses {level}; kCantAffordCost/kPostageCharged use {amount} {currency}. Two-sentence originals are
// split into separate slots so no message carries an embedded newline. kUndelivered* form the
// reboot-recovery status (code joins with newlines; kUndeliveredCount/Names use {count}/{names}).
enum class EMailMsg {
	kGreeting,
	kMailWaiting, kParcelWaiting, kNothingToday,
	kNoLetters,
	kLevelTooLow, kTooManyMessages, kNoRecipient, kNotRegistered, kNoParcelHere, kParcelError,
	kCantAffordCost, kCantAffordNoMoney,
	kStartWriting, kFreePostage, kPostageCharged, kCanWrite,
	kLetterGiven, kLetterGivenRoom,
	kUndeliveredHeader, kUndeliveredCount, kUndeliveredNames,
};

using SpecialMessages = msg_container::MsgContainer<int, ESpecialMsg>;
using BankMessages = msg_container::MsgContainer<int, EBankMsg>;
using MailMessages = msg_container::MsgContainer<int, EMailMsg>;

// Convenience accessors for the shared (default-sheaf) message of each container.
[[nodiscard]] const std::string &SpecialMsg(ESpecialMsg id);
[[nodiscard]] const std::string &BankMsg(EBankMsg id);
[[nodiscard]] const std::string &MailMsg(EMailMsg id);

// One loader per data file (cfg_manager id in the comment). All are Vedun-editable (msg-sheaf model).
class SpecialMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "special_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class BankMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "bank_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class MailMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "mail_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace specials

template<>
const std::string &NAME_BY_ITEM<specials::ESpecialMsg>(specials::ESpecialMsg item);
template<>
specials::ESpecialMsg ITEM_BY_NAME<specials::ESpecialMsg>(const std::string &name);
template<>
const std::map<specials::ESpecialMsg, std::string> &NAMES_OF<specials::ESpecialMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::EBankMsg>(specials::EBankMsg item);
template<>
specials::EBankMsg ITEM_BY_NAME<specials::EBankMsg>(const std::string &name);
template<>
const std::map<specials::EBankMsg, std::string> &NAMES_OF<specials::EBankMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::EMailMsg>(specials::EMailMsg item);
template<>
specials::EMailMsg ITEM_BY_NAME<specials::EMailMsg>(const std::string &name);
template<>
const std::map<specials::EMailMsg, std::string> &NAMES_OF<specials::EMailMsg>();

#endif // BYLINS_SRC_GAMEPLAY_AI_SPECIAL_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
