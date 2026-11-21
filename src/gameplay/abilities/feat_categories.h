/**
\file feat_categories.h - a part of the Bylins engine.
\brief Feat categories from the <feat_categories> section of cfg/feats.xml.
\details A feat names its category with the `category` attribute. A category limits how many of its
 feats a character may hold at once: base + remorts / remort_divisor, but not more than cap.
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_CATEGORIES_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_CATEGORIES_H_

#include "utils/parser_wrapper.h"

#include <string>
#include <unordered_map>

namespace feats {

class FeatCategories {
 public:
	static constexpr int kNoCap{-1};

	struct Category {
		std::string id;
		int base{0};
		int remort_divisor{0};	// 0 -- the limit does not grow with remorts
		int cap{kNoCap};

		// How many feats of this category a character with `remorts` remorts may hold.
		[[nodiscard]] int CalcLimit(int remorts) const;
	};

	// Reads the <feat_categories> child of the <feats> node `root` points at, replacing what was there.
	// An invalid or duplicate category is logged and dropped; feats that name it cannot be learned.
	void Build(parser_wrapper::DataNode &root);

	// nullptr if there is no such category.
	[[nodiscard]] const Category *Find(const std::string &id) const;
	[[nodiscard]] std::size_t Size() const { return categories_.size(); };

 private:
	std::unordered_map<std::string, Category> categories_;
};

// The categories of the loaded feats.xml.
FeatCategories &FeatCategoriesRegistry();

} // namespace feats

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_CATEGORIES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
