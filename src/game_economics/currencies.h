/*
 \authors Created by Sventovit
 \date 17.02.2022.
 \brief Модуль игровых валют.
 \details Модуль игровых валют - кун, гривен, новогоднего льда и прочего, что придет в голову ввести.
 */

#ifndef BYLINS_SRC_GAME_ECONOMICS_CURRENCIES_H_
#define BYLINS_SRC_GAME_ECONOMICS_CURRENCIES_H_

#include "boot/cfg_manager.h"
#include "structs/info_container.h"

namespace currencies {

class CurrenciesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class CurrencyInfo : public info_container::BaseItem<int> {
	friend class CurrencyInfoBuilder;

	std::string name_{"!undefined!"};
	std::unique_ptr<base_structs::ItemName> names_;

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
	[[nodiscard]] bool IsGiveable() const { return giveable_; };
	[[nodiscard]] bool IsObjectable() const { return objectable_; };
	[[nodiscard]] bool IsStorable() const { return bankable_; };
	[[nodiscard]] bool IsTransferable() const { return transferable_; };
	[[nodiscard]] bool IsTransferableToOther() const { return transferable_to_other_; };

	[[nodiscard]] const std::string &GetName(ECase name_case = ECase::kNom) const;
	[[nodiscard]] const std::string &GetPluralName(ECase name_case = ECase::kNom) const;
	[[nodiscard]] const char *GetCName(ECase name_case) const;
	[[nodiscard]] const char *GetPluralCName(ECase name_case) const;
	[[nodiscard]] const std::string &GetNameWithAmount(uint64_t amount) const;

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
