/**
\file cases.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "cases.h"

#include "utils/logger.h"
#include "utils/parser_wrapper.h"
#include "utils/parse.h"

namespace grammar {

void ParseValueToNameCase(const char *value, std::string &name_case) {
	try {
		name_case = parse::ReadAsStr(value);
	} catch (std::exception &e) {
		err_log("invalid name case (value: %s).", e.what());
	}
}

void ParseNodeToNameCases(parser_wrapper::DataNode &node, ItemName::NameCases &name_cases) {
	ParseValueToNameCase(node.GetValue("nom"), name_cases[ECase::kNom]);
	ParseValueToNameCase(node.GetValue("gen"), name_cases[ECase::kGen]);
	ParseValueToNameCase(node.GetValue("dat"), name_cases[ECase::kDat]);
	ParseValueToNameCase(node.GetValue("acc"), name_cases[ECase::kAcc]);
	ParseValueToNameCase(node.GetValue("ins"), name_cases[ECase::kIns]);
	ParseValueToNameCase(node.GetValue("pre"), name_cases[ECase::kPre]);
}

ItemName::ItemName() {
	std::fill(plural_names_.begin(), plural_names_.end(), "UNDEF");
	std::fill(singular_names_.begin(), singular_names_.end(), "UNDEF");
}

ItemName::Ptr ItemName::Build(parser_wrapper::DataNode &node) {
	Ptr ptr(new ItemName());
	node.GoToChild("singular");
	ParseNodeToNameCases(node, ptr->singular_names_);
	node.GoToSibling("plural");
	ParseNodeToNameCases(node, ptr->plural_names_);
	node.GoToParent();
	return ptr;
}

ItemName &ItemName::operator=(ItemName &&i) noexcept {
	if (&i == this) {
		return *this;
	}
	singular_names_ = std::move(i.singular_names_);
	plural_names_ = std::move(i.plural_names_);
	return *this;
}

ItemName::ItemName(ItemName &&i) noexcept {
	singular_names_ = std::move(i.singular_names_);
	plural_names_ = std::move(i.plural_names_);
};

const std::string &ItemName::GetSingular(ECase name_case) const {
	return singular_names_.at(name_case);
}

const std::string &ItemName::GetPlural(ECase name_case) const {
	return plural_names_.at(name_case);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
