#include "feat_timer.h"

#include "utils/logger.h"
#include "utils/utils_parse.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace feats {

namespace {

// Reads the optional integer attribute `key`; returns false if there is none.
bool ReadTimerAttr(parser_wrapper::DataNode &node, const char *key, int &out) {
	const char *value = node.GetValue(key);
	if (!value || !*value) {
		return false;
	}
	try {
		out = parse::ReadAsInt(value);
	} catch (const std::exception &) {
		throw std::runtime_error(std::string("bad ") + key + " " + value);
	}
	return true;
}

FeatTimer ParseTimer(parser_wrapper::DataNode &node) {
	FeatTimer timer;
	timer.has_base = ReadTimerAttr(node, "base", timer.base);
	const bool has_min = ReadTimerAttr(node, "min", timer.min);
	const bool has_divisor = ReadTimerAttr(node, "remort_divisor", timer.remort_divisor);
	if (!timer.has_base) {
		if (has_min || has_divisor) {
			throw std::runtime_error("min and remort_divisor need base");
		}
		return timer;
	}
	if (timer.base < 1 || timer.base > FeatTimer::kMaxDelay) {
		throw std::runtime_error("base must be within 1.." + std::to_string(FeatTimer::kMaxDelay));
	}
	if (timer.min < 1) {
		throw std::runtime_error("min must be at least 1");
	}
	if (timer.min > timer.base) {
		throw std::runtime_error("min is above base");
	}
	if (timer.remort_divisor < 0) {
		throw std::runtime_error("remort_divisor must not be negative");
	}
	return timer;
}

} // namespace

int FeatTimer::CalcDelay(int remorts) const {
	int delay = base;
	if (remort_divisor > 0) {
		delay -= std::max(0, remorts) / remort_divisor;
	}
	return std::max(min, delay);
}

std::string FeatTimer::Describe() const {
	if (!has_base) {
		return "set in code";
	}
	std::string text = std::to_string(base) + " h";
	if (remort_divisor > 0) {
		text += ", -1 h per " + std::to_string(remort_divisor) + " remorts, at least " + std::to_string(min) + " h";
	}
	return text;
}

std::optional<FeatTimer> FeatTimer::Parse(parser_wrapper::DataNode &node, EFeat owner) {
	if (!node.GoToChild("timed")) {
		return std::nullopt;
	}
	std::optional<FeatTimer> result;
	try {
		result = ParseTimer(node);
	} catch (const std::exception &e) {
		err_log("Feat [%s]: invalid <timed> dropped (%s).", NAME_BY_ITEM<EFeat>(owner).c_str(), e.what());
	}
	node.GoToParent();
	return result;
}

} // namespace feats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
