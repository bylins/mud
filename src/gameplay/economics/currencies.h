/*
 \authors Created by Sventovit
 \date 17.02.2022.
 \brief Модуль игровых валют.
 \details Модуль игровых валют - кун, гривен, новогоднего льда и прочего, что придет в голову ввести.
 */

#ifndef BYLINS_SRC_GAME_ECONOMICS_CURRENCIES_H_
#define BYLINS_SRC_GAME_ECONOMICS_CURRENCIES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"
#include <vector>
#include "engine/structs/info_container.h"
#include "utils/grammar/cases.h"

#include <map>
#include <string>


class CharData;

namespace currencies {

// issue.currency-storage: unified read/write API over a character's currency container.
// `id` is a currency text_id; vnum overloads resolve via the registry. Bank routing to the
// account container is added later (account-shared currencies).

std::string TextIdByVnum(int vnum);

[[nodiscard]] long GetTotal(const CharData &ch, const std::string &id);
long RemoveTotal(CharData &ch, const std::string &id, long amount, bool with_log = true);  // bank then hand

[[nodiscard]] long GetTotal(const CharData &ch, int vnum);
long RemoveTotal(CharData &ch, int vnum, long amount, bool with_log = true);

// Separate hand/bank accessors - the readable public surface (only two purses exist).
[[nodiscard]] long GetHand(const CharData &ch, const std::string &id);
[[nodiscard]] long GetBank(const CharData &ch, const std::string &id);
void SetHand(CharData &ch, const std::string &id, long amount, bool with_log = true, bool immortal = false);
void SetBank(CharData &ch, const std::string &id, long amount, bool with_log = true, bool immortal = false);
long AddHand(CharData &ch, const std::string &id, long amount, bool notify = false, bool with_log = true, bool immortal = false);
long AddBank(CharData &ch, const std::string &id, long amount, bool notify = false, bool with_log = true, bool immortal = false);
long RemoveHand(CharData &ch, const std::string &id, long amount, bool with_log = true);
long RemoveBank(CharData &ch, const std::string &id, long amount, bool with_log = true);

[[nodiscard]] long GetHand(const CharData &ch, int vnum);
[[nodiscard]] long GetBank(const CharData &ch, int vnum);
void SetHand(CharData &ch, int vnum, long amount, bool with_log = true, bool immortal = false);
void SetBank(CharData &ch, int vnum, long amount, bool with_log = true, bool immortal = false);
long AddHand(CharData &ch, int vnum, long amount, bool notify = false, bool with_log = true, bool immortal = false);
long AddBank(CharData &ch, int vnum, long amount, bool notify = false, bool with_log = true, bool immortal = false);
long RemoveHand(CharData &ch, int vnum, long amount, bool with_log = true);
long RemoveBank(CharData &ch, int vnum, long amount, bool with_log = true);


/**
 *  Данные валюты слишком глубоко "прошиты" в коде и являются базовыми.
 *  Поэтому, хотя они описаны в конфиге валют, их внумы доподнительно прописаны в коде.
 */
const int kGoldVnum = 0;
const int kGloryVnum = 1;

class CurrenciesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] parser_wrapper::DataNode FindElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

// issue.currencies: size-tier keys for a money object's name (GetObjName), from a
// "tiny handful" (kTinyAmountDesc) up to a "huge mountain" (kImmenseAmountDesc). The
// default tier templates live in currency_msg.xml; a currency may override any tier.
enum class EObjNameDesc {
	kTinyAmountDesc,
	kVerySmallAmountDesc,
	kSmallAmountDesc,
	kModestAmountDesc,
	kBelowAverageAmountDesc,
	kAverageAmountDesc,
	kAboveAverageAmountDesc,
	kBigAmountDesc,
	kLargeAmountDesc,
	kVeryLargeAmountDesc,
	kHugeAmountDesc,
	kEnormousAmountDesc,
	kGiganticAmountDesc,
	kColossalAmountDesc,
	kImmenseAmountDesc
};

// A GetObjName size-phrase template (placeholders: {adjective_ending}, {noun_ending},
// {noun_ending_hard}, {currency_name}) and the smallest amount it applies from.
struct ObjNamePattern {
	long from{0};
	std::string tpl;
};
using ObjNameMap = std::map<EObjNameDesc, ObjNamePattern>;

// issue.thing-names: a currency's display name -- the search string + gendered cased declensions --
// loaded from cfg/messages/ru/currency_msg.xml (before currencies.xml), keyed by currency text_id.
struct CurrencyName {
	EGender gender{EGender::kFemale};
	std::string search{"!undefined!"};
	grammar::ItemName cases;
	ObjNameMap obj_names;  // per-currency GetObjName tier overrides
};

class CurrencyNamesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

// The loaded name for a currency text_id, or nullptr if none.
const CurrencyName *FindCurrencyName(const std::string &text_id);

class CurrencyInfo : public info_container::BaseItem<int> {
	friend class CurrencyInfoBuilder;

	EGender gender_{EGender::kFemale};
	std::string name_{"!undefined!"};
	std::unique_ptr<grammar::ItemName> names_;
	ObjNameMap obj_names_;  // per-currency GetObjName tier overrides

	bool account_shared_{false};
	bool locked_{true};
	bool giveable_{false};
	bool objectable_{false};
	bool bankable_{false};
	bool transferable_{false};
	bool transferable_to_other_{false};
	int transfer_tax_{5};
	int drop_on_death_{100};
	int max_clan_tax_{25};
	long max_amount_{1000000000L};  // = kMaxMoneyKept
	bool logged_{false};       // audit-log changes
	bool reports_msdp_{false}; // push to the client via MSDP (the GOLD field)
	bool money_stat_{false};   // feed MoneyDropStat (zone money flow)
	bool force_split_{false};  // group split: always divide among members (event currencies)
	bool arena_only_{false};   // only relevant in arena context
	bool merchant_payout_{true};  // a shop may pay this currency when buying goods (else it pays gold)

 public:
	CurrencyInfo() = default;
	CurrencyInfo(int id, std::string &text_id, std::string &name, EItemMode mode)
		: BaseItem<int>(id, text_id, mode), name_{name} {};

	[[nodiscard]] bool IsLocked() const { return locked_; };
	[[nodiscard]] bool IsAccountShared() const { return account_shared_; };

	[[nodiscard]] bool IsGiveable() const { return giveable_; };
	[[nodiscard]] bool IsObjectable() const { return objectable_; };
	[[nodiscard]] bool IsStorable() const { return bankable_; };
	[[nodiscard]] bool IsTransferable() const { return transferable_; };
	[[nodiscard]] bool IsTransferableToOther() const { return transferable_to_other_; };

	[[nodiscard]] EGender GetGender() const { return gender_; };
	[[nodiscard]] long GetMaxAmount() const { return max_amount_; };
	[[nodiscard]] bool IsLogged() const { return logged_; };
	[[nodiscard]] bool ReportsMsdp() const { return reports_msdp_; };
	[[nodiscard]] bool TracksMoneyDrop() const { return money_stat_; };
	[[nodiscard]] bool ForceSplit() const { return force_split_; };
	[[nodiscard]] bool IsArenaOnly() const { return arena_only_; };
	[[nodiscard]] bool MerchantPayout() const { return merchant_payout_; };
	[[nodiscard]] const std::string &GetNameWithAmount(long amount, grammar::ECase one_case = grammar::ECase::kNom) const;
	[[nodiscard]] const std::string &GetName(grammar::ECase name_case = grammar::ECase::kNom) const;
	[[nodiscard]] const std::string &GetPluralName(grammar::ECase name_case = grammar::ECase::kNom) const;
	[[nodiscard]] const char *GetCName(grammar::ECase name_case) const;
	[[nodiscard]] const char *GetPluralCName(grammar::ECase name_case) const;
	/**
	 * Генерация названия объекта, содержащего заданное количество валюты.
	 * @param amount - количество валюты.
	 * @param gram_case - в каком падеже нужно название.
	 * @return - строка-описание объекта.
	 */
	[[nodiscard]] std::string GetObjName(long amount, grammar::ECase gram_case) const;
	[[nodiscard]] const char *GetObjCName(long amount, grammar::ECase gram_case) const;

	void Print(CharData */*ch*/, std::ostringstream &buffer) const;
};

// Resolve a player-typed word to a currency by abbrev-matching its search name (nullptr if none).
const CurrencyInfo *FindBySearch(const std::string &word);

class CurrencyInfoBuilder : public info_container::IItemBuilder<CurrencyInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseCurrency(parser_wrapper::DataNode node);
};

using CurrenciesInfo = info_container::InfoContainer<int, CurrencyInfo, CurrencyInfoBuilder>;

} //namespace currencies

#endif //BYLINS_SRC_GAME_ECONOMICS_CURRENCIES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
