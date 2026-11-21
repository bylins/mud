#include "feat_categories.h"

#include "utils/logger.h"
#include "utils/utils_parse.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <unordered_set>

namespace feats {

namespace {

int ReadInt(parser_wrapper::DataNode &node, const char *key, int def) {
	const char *value = node.GetValue(key);
	if (!value || !*value) {
		return def;
	}
	try {
		return parse::ReadAsInt(value);
	} catch (const std::exception &) {
		throw std::runtime_error(std::string("bad ") + key + " " + value);
	}
}

FeatCategories::Category ParseCategory(parser_wrapper::DataNode &node) {
	if (strcmp(node.GetName(), "category") != 0) {
		throw std::runtime_error(std::string("unknown tag <") + node.GetName() + ">");
	}
	FeatCategories::Category category;
	category.id = parse::AttrStr(node, "id");
	if (category.id.empty()) {
		throw std::runtime_error("<category> without id");
	}
	if (!node.GetValue("base") || !*node.GetValue("base")) {
		throw std::runtime_error("no base");
	}
	category.base = ReadInt(node, "base", 0);
	category.remort_divisor = ReadInt(node, "remort_divisor", 0);
	category.cap = ReadInt(node, "cap", FeatCategories::kNoCap);
	if (category.base < 0) {
		throw std::runtime_error("base must not be negative");
	}
	if (category.remort_divisor < 0) {
		throw std::runtime_error("remort_divisor must not be negative");
	}
	if (category.cap != FeatCategories::kNoCap && category.cap < category.base) {
		throw std::runtime_error("cap is below base");
	}
	return category;
}

} // namespace

int FeatCategories::Category::CalcLimit(int remorts) const {
	int limit = base;
	if (remort_divisor > 0) {
		limit += std::max(0, remorts) / remort_divisor;
	}
	if (cap != kNoCap) {
		limit = std::min(limit, cap);
	}
	return limit;
}

std::string FeatCategories::Category::DescribeLimit() const {
	std::string text = std::to_string(base);
	if (remort_divisor > 0) {
		text += " + remorts / " + std::to_string(remort_divisor);
	}
	if (cap != kNoCap) {
		text += ", at most " + std::to_string(cap);
	}
	return text;
}

void FeatCategories::Build(parser_wrapper::DataNode &root) {
	categories_.clear();
	if (!root.GoToChild("feat_categories")) {
		return;
	}
	std::unordered_set<std::string> duplicates;
	for (auto &node : root.Children()) {
		try {
			auto category = ParseCategory(node);
			if (!categories_.emplace(category.id, category).second) {
				duplicates.insert(category.id);
			}
		} catch (const std::exception &e) {
			err_log("Feat category [%s] dropped (%s).", node.GetValue("id"), e.what());
		}
	}
	root.GoToParent();

	// Which copy was meant is unknown, so all of them go and the category fails closed.
	for (const auto &id : duplicates) {
		err_log("Feat category [%s] is defined more than once, dropped.", id.c_str());
		categories_.erase(id);
	}
}

const FeatCategories::Category *FeatCategories::Find(const std::string &id) const {
	const auto it = categories_.find(id);
	return it == categories_.end() ? nullptr : &it->second;
}

FeatCategories &FeatCategoriesRegistry() {
	static FeatCategories categories;
	return categories;
}

} // namespace feats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
