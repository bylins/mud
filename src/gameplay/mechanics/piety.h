/**
\file piety.h - a part of the Bylins engine.
\brief Piety (kReligion): what a prayer or a sacrifice may offer and which gift raises piety (issue #2689).
\details Every offering lays the favour of the god or saint, at the strength current piety gives. Piety
		 itself grows only from a gift worth at least the threshold for the current piety level
		 (cfg/mechanics/piety.xml): equipment is valued by its item level (CalcRemortRequirements),
		 treasure by its cost, a currency by the amount; an item worn down below the fresh share of its
		 timer is worth proportionally less.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_PIETY_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_PIETY_H_

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

class ObjData;

namespace piety {

// true: piety grows only from a gift worth at least the threshold in piety.xml (+1 per such gift),
// and an offering may be equipment, treasure or a currency. false: the old way -- a heathen offers
// food or treasure, a Christian lights a candle, and every offering may raise piety by the usual
// skill roll, but not above kLegacyGrowthLimit.
constexpr bool kValuedOfferings{true};

// The old way stops piety here: C = 0.35 + (Cha - 10) * 0.03 = 0.47 at charisma 14, which gives
// +1 Str, +2 Hit, +5 morale and +47% regen -- about the old fixed favour table.
constexpr int kLegacyGrowthLimit{35};

// How a gift is valued.
enum class EMeasure {
	kNone,      // accepted, but never raises piety (food)
	kIlvl,      // the item level of equipment
	kCost,      // the item's cost (treasure)
	kCurrency   // the amount of a currency
};

struct Gift {
	EMeasure measure{EMeasure::kNone};
	std::string currency;   // the currency's text id, for kCurrency
	double value{0.0};
};

// What one command (sacrifice or prayer) accepts.
struct OfferingRules {
	std::map<EObjType, EMeasure> items;
	std::map<std::string, long> currencies;   // text id -> the least amount accepted
	// An offering without a gift costs this (a Christian's candle); an empty currency means a gift is
	// required.
	std::string default_currency;
	long default_amount{0};

	[[nodiscard]] std::optional<EMeasure> ItemMeasure(EObjType type) const;
	[[nodiscard]] std::optional<long> CurrencyMinimum(const std::string &text_id) const;
};

// The least gift that raises piety from `piety` upwards. A measure without a threshold never raises
// piety on this tier.
struct GrowthTier {
	int piety{0};
	std::optional<double> ilvl;
	std::optional<double> cost;
	std::map<std::string, double> currencies;

	[[nodiscard]] std::optional<double> Threshold(const Gift &gift) const;
};

class Rules {
 public:
	void Load(parser_wrapper::DataNode data);

	[[nodiscard]] const OfferingRules &Sacrifice() const { return sacrifice_; }
	[[nodiscard]] const OfferingRules &Prayer() const { return prayer_; }
	[[nodiscard]] double FreshTimerShare() const { return fresh_timer_share_; }
	[[nodiscard]] const std::vector<GrowthTier> &Tiers() const { return tiers_; }
	// The tier for this piety: the last one whose piety is not above it; nullptr if there is none.
	[[nodiscard]] const GrowthTier *TierFor(int piety) const;
	// The gift reaches the threshold of the tier for this piety.
	[[nodiscard]] bool IsWorthy(int piety, const Gift &gift) const;

 private:
	OfferingRules sacrifice_;
	OfferingRules prayer_;
	double fresh_timer_share_{0.8};
	std::vector<GrowthTier> tiers_;   // sorted by piety
};

const Rules &GetRules();

// The share of an item's value its timer leaves: 1 while the timer is at least fresh_share of the
// prototype's, then falling in proportion down to 0. An item without a finite timer keeps its value.
double TimerFactor(int timer, int proto_timer, double fresh_share);

// The value of an item offered with this measure, the timer counted. std::nullopt: the item cannot be
// valued at all (its stats are broken, see CalcRemortRequirements).
std::optional<double> ItemValue(const ObjData *obj, EMeasure measure, double fresh_share);

class PietyLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace piety

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_PIETY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
