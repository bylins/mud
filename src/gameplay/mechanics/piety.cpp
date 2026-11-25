/**
\file piety.cpp - a part of the Bylins engine.
\brief Piety: offering rules and growth thresholds from cfg/mechanics/piety.xml (issue #2689).
*/

#include "gameplay/mechanics/piety.h"

#include "engine/db/obj_prototypes.h"
#include "engine/entities/obj_data.h"
#include "gameplay/mechanics/stable_objs.h"
#include "utils/logger.h"
#include "utils/parser_wrapper.h"
#include "utils/utils_string.h"

#include <fmt/format.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace piety {

namespace {

// CalcRemortRequirements marks an item with a stat listed twice this way.
constexpr double kBrokenItemLevel{1000000.0};

Rules rules;

bool HasAttr(const parser_wrapper::DataNode &node, const char *key) {
	const char *value = node.GetValue(key);
	return value && *value;
}

double ReadDouble(const parser_wrapper::DataNode &node, const char *key) {
	const char *value = node.GetValue(key);
	char *end = nullptr;
	const double result = std::strtod(value, &end);
	if (end == value || *end != '\0') {
		throw std::runtime_error(fmt::format("{}=\"{}\" is not a number", key, value));
	}
	return result;
}

std::optional<double> ReadOptionalDouble(const parser_wrapper::DataNode &node, const char *key) {
	if (!HasAttr(node, key)) {
		return std::nullopt;
	}
	return ReadDouble(node, key);
}

EMeasure ReadMeasure(const char *value) {
	if (std::strcmp(value, "none") == 0) {
		return EMeasure::kNone;
	}
	if (std::strcmp(value, "ilvl") == 0) {
		return EMeasure::kIlvl;
	}
	if (std::strcmp(value, "cost") == 0) {
		return EMeasure::kCost;
	}
	throw std::runtime_error(fmt::format("unknown value=\"{}\" (none, ilvl or cost)", value));
}

OfferingRules ReadOfferings(parser_wrapper::DataNode node, const char *what) {
	OfferingRules result;
	if (HasAttr(node, "default_currency")) {
		result.default_currency = node.GetValue("default_currency");
		result.default_amount = static_cast<long>(ReadDouble(node, "default_amount"));
	}
	for (auto items : node.Children("items")) {
		const auto measure = ReadMeasure(items.GetValue("value"));
		for (const auto &type_name : utils::Split(items.GetValue("types"), '|')) {
			try {
				result.items[ITEM_BY_NAME<EObjType>(type_name)] = measure;
			} catch (const std::out_of_range &) {
				err_log("piety.xml, <%s>: unknown item type '%s'.", what, type_name.c_str());
			}
		}
	}
	for (auto currency : node.Children("currency")) {
		const long min = HasAttr(currency, "min") ? static_cast<long>(ReadDouble(currency, "min")) : 1L;
		result.currencies[currency.GetValue("id")] = std::max(1L, min);
	}
	if (!result.default_currency.empty() && !result.CurrencyMinimum(result.default_currency)) {
		err_log("piety.xml, <%s>: default_currency '%s' is not among the accepted currencies.",
				what, result.default_currency.c_str());
	}
	return result;
}

} // namespace

std::optional<EMeasure> OfferingRules::ItemMeasure(EObjType type) const {
	const auto it = items.find(type);
	if (it == items.end()) {
		return std::nullopt;
	}
	return it->second;
}

std::optional<long> OfferingRules::CurrencyMinimum(const std::string &text_id) const {
	const auto it = currencies.find(text_id);
	if (it == currencies.end()) {
		return std::nullopt;
	}
	return it->second;
}

std::optional<double> GrowthTier::Threshold(const Gift &gift) const {
	switch (gift.measure) {
		case EMeasure::kIlvl: return ilvl;
		case EMeasure::kCost: return cost;
		case EMeasure::kCurrency: {
			const auto it = currencies.find(gift.currency);
			if (it == currencies.end()) {
				return std::nullopt;
			}
			return it->second;
		}
		case EMeasure::kNone:
		default: return std::nullopt;
	}
}

void Rules::Load(parser_wrapper::DataNode data) {
	Rules loaded;
	auto node = data;
	if (node.GoToChild("sacrifice")) {
		loaded.sacrifice_ = ReadOfferings(node, "sacrifice");
	}
	node = data;
	if (node.GoToChild("prayer")) {
		loaded.prayer_ = ReadOfferings(node, "prayer");
	}
	node = data;
	if (node.GoToChild("growth")) {
		if (HasAttr(node, "fresh_timer")) {
			loaded.fresh_timer_share_ = std::clamp(ReadDouble(node, "fresh_timer") / 100.0, 0.01, 1.0);
		}
		for (auto tier_node : node.Children("tier")) {
			GrowthTier tier;
			tier.piety = static_cast<int>(ReadDouble(tier_node, "piety"));
			tier.ilvl = ReadOptionalDouble(tier_node, "ilvl");
			tier.cost = ReadOptionalDouble(tier_node, "cost");
			for (auto currency : tier_node.Children("currency")) {
				tier.currencies[currency.GetValue("id")] = ReadDouble(currency, "min");
			}
			loaded.tiers_.push_back(std::move(tier));
		}
	}
	std::stable_sort(loaded.tiers_.begin(), loaded.tiers_.end(),
		[](const GrowthTier &a, const GrowthTier &b) { return a.piety < b.piety; });
	*this = std::move(loaded);
}

const GrowthTier *Rules::TierFor(int piety) const {
	const GrowthTier *result = nullptr;
	for (const auto &tier : tiers_) {
		if (tier.piety > piety) {
			break;
		}
		result = &tier;
	}
	return result;
}

bool Rules::IsWorthy(int piety, const Gift &gift) const {
	const auto *tier = TierFor(piety);
	if (!tier) {
		return false;
	}
	const auto threshold = tier->Threshold(gift);
	return threshold && gift.value >= *threshold;
}

const Rules &GetRules() {
	return rules;
}

double TimerFactor(int timer, int proto_timer, double fresh_share) {
	if (proto_timer <= 0 || proto_timer >= ObjData::UNLIMITED_TIMER || timer >= ObjData::UNLIMITED_TIMER) {
		return 1.0;
	}
	const double share = std::max(0, timer) / static_cast<double>(proto_timer);
	if (share >= fresh_share) {
		return 1.0;
	}
	return share / fresh_share;
}

std::optional<double> ItemValue(const ObjData *obj, EMeasure measure, double fresh_share) {
	double value{0.0};
	switch (measure) {
		case EMeasure::kIlvl:
			value = CalcRemortRequirements(obj);
			if (value >= kBrokenItemLevel) {
				return std::nullopt;
			}
			break;
		case EMeasure::kCost:
			value = obj->get_cost();
			break;
		case EMeasure::kNone:
		case EMeasure::kCurrency:
		default:
			return 0.0;
	}
	if (stable_objs::IsTimerUnlimited(obj) || obj->get_rnum() < 0) {
		return value;
	}
	return value * TimerFactor(obj->get_timer(), obj_proto[obj->get_rnum()]->get_timer(), fresh_share);
}

void PietyLoader::Load(parser_wrapper::DataNode data) {
	try {
		rules.Load(std::move(data));
	} catch (const std::exception &e) {
		err_log("piety.xml: %s; piety will not grow from offerings.", e.what());
	}
}

void PietyLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

} // namespace piety

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
