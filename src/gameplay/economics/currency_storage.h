// issue.currency-storage: a per-owner currency container.
//
// Replaces the old per-currency member variables (gold_/bank_gold_/hryvn/nogata/
// ice_currency) with one map keyed by a currency's text_id, holding the amount in
// hand and in the bank. Keyed by text_id (not vnum) because a vnum may be reused by
// a different currency, whereas reusing a text_id is an intentional rename.
//
// This is a lightweight container only. The operations (flag checks, account routing,
// logging) live in the currency module; the container is reached through a single
// CharData getter so that data-corruption sources stay easy to find.

#ifndef BYLINS_SRC_GAMEPLAY_ECONOMICS_CURRENCY_STORAGE_H_
#define BYLINS_SRC_GAMEPLAY_ECONOMICS_CURRENCY_STORAGE_H_

#include <map>
#include <string>

namespace currencies {

// Canonical text_id keys for the built-in currencies.
inline const std::string kGold = "kKuna";
inline const std::string kGlory = "kGlory";
inline const std::string kCopperGrivnaId = "kCopperGrivna";

// The amount of one currency the owner holds: in hand and in the bank/casket.
struct OwnerCurrencyInfo {
	long hand{0};
	long bank{0};
};

class CurrencyStorage {
 public:
	using Map = std::map<std::string, OwnerCurrencyInfo>;

	[[nodiscard]] long GetHand(const std::string &id) const {
		const auto it = data_.find(id);
		return it == data_.end() ? 0 : it->second.hand;
	}
	[[nodiscard]] long GetBank(const std::string &id) const {
		const auto it = data_.find(id);
		return it == data_.end() ? 0 : it->second.bank;
	}
	void SetHand(const std::string &id, long value) { data_[id].hand = value; }
	void SetBank(const std::string &id, long value) { data_[id].bank = value; }

	[[nodiscard]] const Map &data() const { return data_; }
	[[nodiscard]] Map &data() { return data_; }

 private:
	Map data_;
};

}  // namespace currencies

#endif  // BYLINS_SRC_GAMEPLAY_ECONOMICS_CURRENCY_STORAGE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
