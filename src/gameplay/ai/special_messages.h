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
	kCantBank,           // "Эту валюту нельзя хранить в банке."
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

// Horse keeper (horse_msg.xml). All act() lines ($n/$N codes, to char/room). kForSale uses
// {amount} {currency}; the buy/sell delivery lines use {horse} (padded name) and {pronoun} (HSHR).
// kSellTaken serves both the to-char and to-room delivery lines (identical text).
enum class EHorseMsg {
	kGreeting,
	kAlreadyHave, kForSale,
	kBuyHaveAlready, kBuyNoMoney, kBuyNoHorse, kBuyGiveChar, kBuyGiveRoom,
	kSellNoHorse, kSellOnHorse, kSellWrongHorse, kSellHorseAway, kSellTaken,
};

// Torc / гривна exchange (torc_msg.xml). Opened by `менять` as a stateful menu (no resolver).
// Parse responses + the menu screen pieces. {amount}/{from}/{to}/{rate} are fmt args; kRow is the
// menu-line template (named args incl. {from:<17} alignment); kLabel* are the gривна names.
enum class ETorcMsg {
	kNotEnough, kReducedTo, kExchanged, kInvalidChoice, kMinAmount, kCancelled, kTechError, kConfirmed,
	kRatesHeader, kRateBronze, kRateSilver, kBalance, kRow,
	kLabelBronze, kLabelSilver, kLabelGold,
	kInstrMin, kInstrAmount, kOptCancel, kOptConfirm, kPrompt,
};

// Mercenary boss (mercenary_msg.xml). tell_to_char + act + SendMsgToChar lines. List headers are
// stored as full sentences (short/full x role) for clean localization. {amount}/{currency}/{boss}/
// {mob} are fmt args; kSummon*/act lines keep $n act-codes.
enum class EMercMsg {
	kNoAccess, kUnknownCmd, kEmptyEmployer, kEmptyCharmer, kEmptyImmortal, kListImmortalShort, kListImmortalFull, kListEmployerShort, kListEmployerFull, kListCharmerShort, kListCharmerFull, kTableHeader, kListTotal, kUnknownChar, kTooExpensive, kCantFind, kSummonHideCharm, kSummonBackCharm, kSummonHide, kSummonBack, kRefuseMale, kRefuseFemale, kFavAdded, kFavRemoved,
};

// Exchange / базар (exchange_msg.xml). Discrete command replies, guards, act confirmations and
// the message_exchange broadcasts (kBcast*). {level}/{amount}/{currency}/{item}/{lot}/{suf}/{filter}/
// {color}/{nocolor} are fmt args; kBought/kWithdrawn/kExhibited keep $O3. The help screen, item-detail
// assembly, lot tables and MortShowObjValues stay in code as layout/diagnostics.
enum class EExchMsg {
	kSilenced, kDumbed, kLevelTooLow, kNoRentable, kImmortalNoEconomy, kNoLotNum, kBadLotNum, kFormat, kNoItem, kDontHave, kPriceTooHigh, kNotForExchange, kNoTaxMoney, kEmptyFirst, kBigSet, kMaxItems, kBazaarFull, kExhibited, kBcastNew, kCostFormat, kNotYourLot, kSameCost, kBadCost, kCostSet, kBcastNewCost, kWithdrawn, kBcastWithdrawnGod, kBcastWithdrawnOwner, kIdentImmortal, kIdentNoMoney, kIdentCharged, kOwnLot, kBankNoMoney, kBought, kBcastSold, kSellerSold, kFilterTooLong, kFilterEmpty, kFilterCurrent, kFilterCleared, kFilterBadFormat, kFilterCurrentShort, kFilterYours, kBadFilterString, kBazaarEmpty, kNotHuman, kNeedTrader,
};

// Rent receptionist (rent_msg.xml). Greeting + gen_receptionist/Crash_* dialogue (act + printf
// SendMsgToChar). {amount}/{currency}/{day}/{item}/{recep}/{max}/{count}/{perday} are fmt args; act
// lines keep $n/$N codes. The assembled per-item rent listing (Crash_report_rent_item) and the
// general do_quit messages stay in code.
enum class ERentMsg {
	kGreeting, kCanLiveForever, kDeadlineIntro, kDepotCost, kRentCost, kMoneyLasts, kUnrentSet, kUnrent, kRentIntroEquip, kRentIntroStore, kNothingToRent, kTooManyItems1, kTooManyItems2, kTipForBeer, kTotalCost, kNoMoneyEver, kHalfPrice, kRecepAsleep, kCantSee, kEnemyZone, kNoRentableWar, kFreeRent, kDailyCost, kDailyCostCryo, kCantAfford, kLockedAway, kCryoGhost, kCryoLostTouch, kKickedToBench, kHelpedToSleep, kOfferStay, kSettleForced, kSettleOffer, kSettleWelcome,
};

// Shop keeper (shop_msg.xml). Greeting + process_buy / process_cmd / process_ident / do_shop_cmd.
// tell_to_char (keeper says to char), printf SendMsgToChar (code adds the trailing \r\n) and one act
// line (kRepaired keeps $n/$g/$o3). Named fmt args: {amount}/{currency} (money), {count} (quantity),
// {item} (object name in the proper case), {name} (player), {type}/{material}/{filter}/{cmd}. Gender
// splits into *Male/*Female keys (tell text has no act-codes). The shop list/filter tables keep their
// column layout + ASCII separator in code; only the localized header line is a message. The
// item-detail diagnostics (diag_weapon/diag_timer/MortShowObjValues) stay in code.
enum class EShopMsg {
	kGreeting, kError,
	kBuyWhat, kNoSuchItem, kTooGreedy, kWrongClass, kCantAfford, kSwearing, kDrinkEmoteMale, kDrinkEmoteFemale,
	kHandsFull1, kHandsFull2, kCheaterMale, kCheaterFemale, kCarryOnly, kLiftOnly, kSellOnly,
	kSellNothing, kHryvnReset, kPrice, kHappyOwnerMale, kHappyOwnerFemale,
	kListHeader, kFilterHeader, kFilterSelection, kBulk,
	kOwnSuppliers, kCmdWhat, kNoItem,
	kIdentWhat, kInspectIntro, kMaterial, kInspectExdesc, kCantCarry, kNoMoney, kIdentCost, kIdentResult,
	kWontDeal, kNoUsedItems, kNoPigInPoke, kBloodyValue, kWontBuy, kValueOffer, kBloodySell, kSellPaid,
	kBloodyRepair, kNoRepairNeeded, kWontRepair, kRepairCost, kCantAffordRepair, kRepaired,
};

// Notice boards (board_msg.xml). DoBoard / message_no_* / do_list / report_on_board / login + new-post
// notifications. All SendMsgToChar (code appends the trailing \r\n); no act-codes. Named fmt args:
// {level} {board} {alias} {subject} {text} {count} {desc} {author} {color} {nocolor}. Multi-line
// originals are split into numbered slots (kSpecialUsage/Alias, kNameCollision1/2, kListIntro1/2,
// kReportSaved1/2). The board-stats table (DoBoardList) + per-message formatters stay in code as
// layout, and board names/descriptions + server-posted message bodies stay in code as content.
enum class EBoardMsg {
	kNoWriteLevel, kNoWriteAccess, kNoReadAccess, kSpecialUsage, kSpecialAlias,
	kBlind, kNoBoard, kNoUnread, kNoSuchMessage, kSpammer, kClanWriteBan, kOverflow,
	kNeedAuthorName, kSubjectTruncated, kWritingPrompt,
	kRemoveNeedNumber, kRemoveNoAccess, kRemoved, kBadCommand,
	kNameCollision1, kNameCollision2,
	kListIntro1, kListIntro2, kListEmpty, kListTotal,
	kLoginHeader, kLoginRow, kNewMessage,
	kReportEmpty, kReportNoBoard, kReportSaved1, kReportSaved2, kReportSaved3,
};

using SpecialMessages = msg_container::MsgContainer<int, ESpecialMsg>;
using BankMessages = msg_container::MsgContainer<int, EBankMsg>;
using MailMessages = msg_container::MsgContainer<int, EMailMsg>;
using HorseMessages = msg_container::MsgContainer<int, EHorseMsg>;
using TorcMessages = msg_container::MsgContainer<int, ETorcMsg>;
using MercMessages = msg_container::MsgContainer<int, EMercMsg>;
using ExchMessages = msg_container::MsgContainer<int, EExchMsg>;
using RentMessages = msg_container::MsgContainer<int, ERentMsg>;
using ShopMessages = msg_container::MsgContainer<int, EShopMsg>;
using BoardMessages = msg_container::MsgContainer<int, EBoardMsg>;

// Convenience accessors for the shared (default-sheaf) message of each container.
[[nodiscard]] const std::string &SpecialMsg(ESpecialMsg id);
[[nodiscard]] const std::string &BankMsg(EBankMsg id);
[[nodiscard]] const std::string &MailMsg(EMailMsg id);
[[nodiscard]] const std::string &HorseMsg(EHorseMsg id);
[[nodiscard]] const std::string &TorcMsg(ETorcMsg id);
[[nodiscard]] const std::string &MercMsg(EMercMsg id);
[[nodiscard]] const std::string &ExchMsg(EExchMsg id);
[[nodiscard]] const std::string &RentMsg(ERentMsg id);
[[nodiscard]] const std::string &ShopMsg(EShopMsg id);
[[nodiscard]] const std::string &BoardMsg(EBoardMsg id);

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

class HorseMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "horse_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class TorcMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "torc_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class MercMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "mercenary_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class ExchMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "exchange_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class RentMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "rent_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class ShopMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "shop_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

class BoardMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {  // "board_messages"
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

template<>
const std::string &NAME_BY_ITEM<specials::EHorseMsg>(specials::EHorseMsg item);
template<>
specials::EHorseMsg ITEM_BY_NAME<specials::EHorseMsg>(const std::string &name);
template<>
const std::map<specials::EHorseMsg, std::string> &NAMES_OF<specials::EHorseMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::ETorcMsg>(specials::ETorcMsg item);
template<>
specials::ETorcMsg ITEM_BY_NAME<specials::ETorcMsg>(const std::string &name);
template<>
const std::map<specials::ETorcMsg, std::string> &NAMES_OF<specials::ETorcMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::EMercMsg>(specials::EMercMsg item);
template<>
specials::EMercMsg ITEM_BY_NAME<specials::EMercMsg>(const std::string &name);
template<>
const std::map<specials::EMercMsg, std::string> &NAMES_OF<specials::EMercMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::EExchMsg>(specials::EExchMsg item);
template<>
specials::EExchMsg ITEM_BY_NAME<specials::EExchMsg>(const std::string &name);
template<>
const std::map<specials::EExchMsg, std::string> &NAMES_OF<specials::EExchMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::ERentMsg>(specials::ERentMsg item);
template<>
specials::ERentMsg ITEM_BY_NAME<specials::ERentMsg>(const std::string &name);
template<>
const std::map<specials::ERentMsg, std::string> &NAMES_OF<specials::ERentMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::EShopMsg>(specials::EShopMsg item);
template<>
specials::EShopMsg ITEM_BY_NAME<specials::EShopMsg>(const std::string &name);
template<>
const std::map<specials::EShopMsg, std::string> &NAMES_OF<specials::EShopMsg>();

template<>
const std::string &NAME_BY_ITEM<specials::EBoardMsg>(specials::EBoardMsg item);
template<>
specials::EBoardMsg ITEM_BY_NAME<specials::EBoardMsg>(const std::string &name);
template<>
const std::map<specials::EBoardMsg, std::string> &NAMES_OF<specials::EBoardMsg>();

#endif // BYLINS_SRC_GAMEPLAY_AI_SPECIAL_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
