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
#include "engine/structs/info_container.h"
#include "utils/grammar/cases.h"

#include <map>
#include <string>

// Старый неймспейс со старыми идами валют
// Его необходимо удалить после доделывания системы валют
namespace currency {
// Legacy currency vnums (this enum is being retired in the currency rework).
// vnums 2..5 are the copper/bronze/silver/gold grivna denominations (data-only,
// no enum members); kMagicIce/kNogata moved to 6/7.
enum { GOLD, GLORY, TORC, ICE = 6, NOGATA };
}

class CharData;

namespace currencies {

// issue.currency-storage: unified read/write API over a character's currency container.
// `id` is a currency text_id; vnum overloads resolve via the registry. Bank routing to the
// account container is added later (account-shared currencies).
enum class EPurse { kHand, kBank };

std::string TextIdByVnum(int vnum);

[[nodiscard]] long GetAmount(const CharData &ch, const std::string &id, EPurse purse);
[[nodiscard]] long GetTotal(const CharData &ch, const std::string &id);
void SetAmount(CharData &ch, const std::string &id, EPurse purse, long amount);
long AddAmount(CharData &ch, const std::string &id, EPurse purse, long amount);     // returns amount added
long RemoveAmount(CharData &ch, const std::string &id, EPurse purse, long amount);  // returns shortfall not removed

[[nodiscard]] long GetAmount(const CharData &ch, int vnum, EPurse purse);
[[nodiscard]] long GetTotal(const CharData &ch, int vnum);
void SetAmount(CharData &ch, int vnum, EPurse purse, long amount);
long AddAmount(CharData &ch, int vnum, EPurse purse, long amount);
long RemoveAmount(CharData &ch, int vnum, EPurse purse, long amount);


/**
 *  Данные валюты слишком глубоко "прошиты" в коде и являются базовыми.
 *  Поэтому, хотя они описаны в конфиге валют, их внумы доподнительно прописаны в коде.
 */
const int kKunaVnum = 0;
const int GloryVnum = 1;
const int kCopperGrivnaVnum = 2;
const int kNogataVnum = 7;
const int kSnowflakeVnum = 9;

class CurrenciesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
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
