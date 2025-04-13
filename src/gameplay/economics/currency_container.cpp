#include "currency_container.h"

#include "engine/db/global_objects.h"
#include "gameplay/economics/currencies.h"
#include "utils/logger.h"
#include "utils/utils_string.h"

#include <sstream>

namespace currencies {

CurrencyContainer::CurrencyContainer(bool account_shared)
	: account_shared_(account_shared)
{
}

void CurrencyContainer::SetCurrencyAmount(Vnum currency_vnum, int value)
{
	currency_storage_[currency_vnum] = std::clamp(value, 0, currency_amount_cap_);
	Validate();
}

void CurrencyContainer::ModifyCurrencyAmount(Vnum currency_vnum, int value)
{
	if (!currency_storage_.contains(currency_vnum)) {
		currency_storage_[currency_vnum] = value;
	} else {
		currency_storage_[currency_vnum] += value;
	}
	currency_storage_[currency_vnum] = std::clamp(currency_storage_[currency_vnum], 0, currency_amount_cap_);
	Validate();
}

std::optional<int> CurrencyContainer::GetCurrencyAmount(Vnum currency_vnum) const
{
	if (!currency_storage_.contains(currency_vnum)) {
		return std::nullopt;
	}

	return currency_storage_.at(currency_vnum);
}

void CurrencyContainer::Validate()
{
	std::erase_if(currency_storage_, [this](const auto &element) {
		const bool wrong_amount = element.second <= 0;
		const auto currency = FindCurrencyByVnum(element.first);
		const bool wrong_account_shared = currency.has_value() ? currency->get().IsAccountShared() != account_shared_ : true;

		const bool result = wrong_amount || wrong_account_shared;
		if (result) {
			err_log("CurrencyContainer::Validate. Currency text_id: %s, Wrong amount: %d, Wrong account shared: %d",
					currency->get().GetTextId().c_str(), wrong_amount, wrong_account_shared);
		}

		return result;
	});
}

std::optional<std::string> CurrencyContainer::serialize()
{
	Validate();

	// формат:
	// currency_text_id=value currency_text_id_2=value ...
	if (currency_storage_.empty()) {
		return std::nullopt;
	}

	std::ostringstream ss;

	for (const auto & [currency_vnum, currency_amount] : currency_storage_) {
		const auto currency = FindCurrencyByVnum(currency_vnum);
		if (!currency.has_value() || currency->get().GetId() == info_container::kUndefinedVnum) {
			err_log("CurrencyContainer::serialize: No text_id for currency vnum: %d", currency_vnum);
			continue;
		}

		if (!ss.str().empty()) {
			ss << " ";
		}

		ss << currency->get().GetTextId() << "=" << currency_amount;
	}

	return ss.str();
}

void CurrencyContainer::deserealize(const std::string& data)
{
	if (data.empty()) {
		return;
	}

	for (const auto &data_list : utils::Split(data, ' ')) {
		const auto currency_data = utils::Split(data_list, '=');
		if (currency_data.size() != 2) {
			err_log("CurrencyContainer::deserealize: Wrong input data: %s", data_list.c_str());
			continue;
		}
		const std::string &in_currency_text_id = currency_data[0];
		const std::string &in_currency_amount = currency_data[1];

		const auto currency = FindCurrencyByTextId(in_currency_text_id);
		if (!currency.has_value() || currency->get().GetId() == info_container::kUndefinedVnum) {
			err_log("CurrencyContainer::deserealize: No such currency: %s", data_list.c_str());
			continue;
		}

		int currency_amount = 0;
		try {
			currency_amount = std::stoi(in_currency_amount);
		} catch (const std::invalid_argument& e) {
			err_log("CurrencyContainer::deserealize: exception: %s", e.what());
			currency_amount = 0;
		} catch (const std::out_of_range& e) {
			err_log("CurrencyContainer::deserealize: exception: %s", e.what());
			currency_amount = 0;
		}
		if (currency_amount == 0) {
			err_log("CurrencyContainer::deserealize: Wrong currency amount: %s", data_list.c_str());
			continue;
		}

		currency_storage_[currency->get().GetId()] = currency_amount;
	}

	Validate();
}

std::optional<std::reference_wrapper<const CurrencyInfo>> CurrencyContainer::FindCurrencyByVnum(Vnum vnum)
{
	if (vnum == info_container::kUndefinedVnum) {
		return std::nullopt;
	}

	for (const auto &currency : MUD::Currencies()) {
		if (currency.GetId() == vnum) {
			return std::cref(currency);
		}
	}

	return std::nullopt;
}

std::optional<std::reference_wrapper<const CurrencyInfo>> CurrencyContainer::FindCurrencyByTextId(const std::string &text_id)
{
	for (const auto &currency : MUD::Currencies()) {
		if (currency.GetTextId() == text_id) {
			return std::cref(currency);
		}
	}

	return std::nullopt;
}

} // namespace currencies
