/*
 \authors Created by Sventovit
 \date 17.02.2022.
 \brief Модуль игровых валют.
 \details Модуль игровых валют - кун, гривен, новогоднего льда и прочего, что придет в голову ввести.
 */

#include "currencies.h"

#include "color.h"
#include "structs/global_objects.h"
//#include "utils/parse.h"

namespace currencies {

using DataNode = parser_wrapper::DataNode;
using ItemPtr = CurrencyInfoBuilder::ItemPtr;

void CurrenciesLoader::Load(DataNode data) {
	MUD::Currencies().Init(data.Children());
}

void CurrenciesLoader::Reload(DataNode data) {
	MUD::Currencies().Reload(data.Children());
}

ItemPtr CurrencyInfoBuilder::Build(DataNode &node) {
	try {
		return ParseCurrency(node);
	} catch (std::exception &e) {
		err_log("Currency parsing error: '%s'", e.what());
		return nullptr;
	}
}

ItemPtr CurrencyInfoBuilder::ParseCurrency(DataNode node) {
	auto vnum = std::clamp(parse::ReadAsInt(node.GetValue("vnum")), 0, kMaxProtoNumber);
	auto mode = SkillInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	std::string text_id{"kUndefined"};
	std::string name{"undefined"};
	try {
		text_id = parse::ReadAsStr(node.GetValue("text_id"));
		name = parse::ReadAsStr(node.GetValue("name"));
	} catch (...) {}

	auto currency_info = std::make_shared<CurrencyInfo>(vnum, text_id, name, mode);

	if (node.GoToChild("flags")) {
		try {
			currency_info->locked_ = parse::ReadAsBool(node.GetValue("locked"));
			currency_info->account_shared_ = parse::ReadAsBool(node.GetValue("account_shared"));
		} catch (std::runtime_error &e) {
			err_log("incorrect flags (%s) in currency '%s'.", e.what(), currency_info->name_.c_str());
		}
		node.GoToParent();
	}

	if (node.GoToChild("permits")) {
		try {
			currency_info->giveable_ = parse::ReadAsBool(node.GetValue("give"));
			currency_info->objectable_ = parse::ReadAsBool(node.GetValue("obj"));
			currency_info->bankable_ = parse::ReadAsBool(node.GetValue("bank"));
			currency_info->transferable_ = parse::ReadAsBool(node.GetValue("transfer"));
			currency_info->transferable_to_other_ = parse::ReadAsBool(node.GetValue("transfer_other"));
			currency_info->transfer_tax_ = std::clamp(parse::ReadAsInt(node.GetValue("transfer_tax")), 0, 99);
			currency_info->drop_on_death_ = std::clamp(parse::ReadAsInt(node.GetValue("drop")), 0, 100);
			currency_info->max_clan_tax_ = std::clamp(parse::ReadAsInt(node.GetValue("clan_tax")), 0, 100);

		} catch (std::runtime_error &e) {
			err_log("incorrect permits (%s) in currency '%s'.", e.what(), currency_info->name_.c_str());
		}
		node.GoToParent();
	}

	if (node.GoToChild("name")) {
		try {
			currency_info->gender_ = parse::ReadAsConstant<EGender>(node.GetValue("gender"));
		} catch (std::exception &e) {}
		currency_info->names_ = base_structs::ItemName::Build(node);
	}

	return currency_info;
}

void CurrencyInfo::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << "Print currency:" << "\n"
		<< " Vnum: " << KGRN << GetId() << KNRM << std::endl
		<< " TextId: " << KGRN << GetTextId() << KNRM << std::endl
		<< " Name: " << KGRN << name_ << KNRM << std::endl
		<< " Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(GetMode()) << KNRM << std::endl
		<< " Can be given: " << KGRN << (giveable_ ? "Y" : "N") << KNRM << std::endl
		<< " Can be objected: " << KGRN << (objectable_ ? "Y" : "N") << KNRM << std::endl
		<< " Can be stored in bank: " << KGRN << (bankable_ ? "Y" : "N") << KNRM << std::endl
		<< " Can be transfered: " << KGRN << (transferable_ ? "Y" : "N") << KNRM << std::endl
		<< " Can be transfered to other account: " << KGRN << (transferable_to_other_ ? "Y" : "N") << KNRM << std::endl
		<< " Transfer tax: " << KGRN << transfer_tax_ << KNRM << std::endl
		<< " Drop on death: " << KGRN << drop_on_death_ << KNRM << std::endl
		<< " Max clan tax: " << KGRN << max_clan_tax_ << KNRM << std::endl
		<< std::endl;
}

const std::string &CurrencyInfo::GetName(ECase name_case) const {
	return names_->GetSingular(name_case);
}

const std::string &CurrencyInfo::GetPluralName(ECase name_case) const {
	return names_->GetPlural(name_case);
}

const char *CurrencyInfo::GetCName(ECase name_case) const {
	return names_->GetSingular(name_case).c_str();
}

const char *CurrencyInfo::GetPluralCName(ECase name_case) const {
	return names_->GetPlural(name_case).c_str();
}

const std::string &CurrencyInfo::GetNameWithQuantity(long quantity) const {
	auto remainder = quantity % 20;
	if ((remainder >= 5 && remainder <= 19) || remainder == 0) {
		return GetPluralName(ECase::kGen);
	} else if (remainder >= 2 && remainder <= 4) {
		return GetPluralName(ECase::kAcc);
	} else {
		return GetName();
	}
}

std::string CurrencyInfo::GetObjName(long quantity, ECase gram_case) const {
	const char *plural[6][3] =
		{
			{
				"ая", "а", "а"}, {
				"ой", "и", "ы"}, {
				"ой", "е", "е"}, {
				"ую", "у", "у"}, {
				"ой", "ой", "ой"}, {
				"ой", "е", "е"}
		};

	using Cases = std::unordered_map<ECase, std::string>;
	using Suffixes = std::unordered_map<EGender, Cases>;

	static const Suffixes kNumeralSuffixes {
		{EGender::kMale, {
			{ECase::kNom, "ин"},
			{ECase::kGen, "ного"},
			{ECase::kDat, "ному"},
			{ECase::kAcc, "нин"},
			{ECase::kIns, "ним"},
			{ECase::kPre, "ном"}
			}
		},
		{EGender::kFemale,{
			{ECase::kNom, "на"},
			{ECase::kGen, "ной"},
			{ECase::kDat, "ной"},
			{ECase::kAcc, "ну"},
			{ECase::kIns, "ной"},
			{ECase::kPre, "ной"}
			}
		},
		{EGender::kNeutral, {
			{ECase::kNom, "но"},
			{ECase::kGen, "ного"},
			{ECase::kDat, "ному"},
			{ECase::kAcc, "но"},
			{ECase::kIns, "ним"},
			{ECase::kPre, "ном"}
			}
		},
		{EGender::kPoly, {
			{ECase::kNom, "ни"},
			{ECase::kGen, "них"},
			{ECase::kDat, "ним"},
			{ECase::kAcc, "ни"},
			{ECase::kIns, "ними"},
			{ECase::kPre, "них"}
			}
		}
	};


	if (quantity <= 0) {
		log("SYSERR: Try to create negative or 0 money (%ld).", quantity);
		return {};
	}

	std::ostringstream out;
	if (quantity == 1) {
		out << "од" << kNumeralSuffixes.at(GetGender()).at(gram_case) << " " << GetName(gram_case);
	} else if (quantity <= 20) {
		out << "малюсеньк" << plural[gram_case][0] << " горстк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 50) {
		out << "маленьк" << plural[gram_case][0] << " горстк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 150) {
		out << "небольш" << plural[gram_case][0] << " горстк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 300) {
		out << "маленьк" << plural[gram_case][0] << " кучк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 1000) {
		out << "небольш" << plural[gram_case][0] << " кучк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 5000) {
		out << "кучк" << plural[gram_case][1] << " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 20000) {
		out << "больш" << plural[gram_case][0] << " кучк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 50000) {
		out << "небольш" << plural[gram_case][0] << " горк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 75000) {
		out << "горк" << plural[gram_case][1] << " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 100000) {
		out << "больш" << plural[gram_case][0] << " горк" << plural[gram_case][1]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 150000) {
		out << "груд" << plural[gram_case][2] << " " << GetPluralCName(ECase::kGen);
	} else if (quantity <= 250000) {
		out << "больш" << plural[gram_case][0] << " груд" << plural[gram_case][2]
			<< " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 500000) {
		out << "гор" << plural[gram_case][1] << " " << GetPluralName(ECase::kGen);
	} else if (quantity <= 1000000) {
		out << "больш" << plural[gram_case][0] << " гор" << plural[gram_case][2]
			<< " " << GetPluralName(ECase::kGen);
	} else  {
		out << "огромн" << plural[gram_case][0] << " гор" << plural[gram_case][2]
			<< " " << GetPluralName(ECase::kGen);
	}

	return out.str();
}
const char *CurrencyInfo::GetObjCName(long quantity, ECase gram_case) const {
	static char buf[128];
	sprintf(buf, "%s", GetObjName(quantity, gram_case).c_str());
	return buf;
}

} // namespace currencies

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
