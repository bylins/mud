/*
 \authors Created by Sventovit
 \date 17.02.2022.
 \brief Модуль игровых валют.
 \details Модуль игровых валют - кун, гривен, новогоднего льда и прочего, что придет в голову ввести.
 */

#include "currencies.h"

#include "engine/entities/char_data.h"
#include "engine/structs/structs.h"

#include "utils/grammar/declensions.h"

#include <unordered_map>
#include <algorithm>

#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
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

// issue.thing-names: currency display names, keyed by text_id, loaded from currency_msg.xml.
static std::unordered_map<std::string, CurrencyName> g_currency_names;

// issue.currencies: default GetObjName size-tier templates; a currency's own rows merge on top.
static ObjNameMap g_default_obj_names;

static const std::unordered_map<std::string, EObjNameDesc> kObjNameDescByName = {
	{"kTinyAmountDesc", EObjNameDesc::kTinyAmountDesc},
	{"kVerySmallAmountDesc", EObjNameDesc::kVerySmallAmountDesc},
	{"kSmallAmountDesc", EObjNameDesc::kSmallAmountDesc},
	{"kModestAmountDesc", EObjNameDesc::kModestAmountDesc},
	{"kBelowAverageAmountDesc", EObjNameDesc::kBelowAverageAmountDesc},
	{"kAverageAmountDesc", EObjNameDesc::kAverageAmountDesc},
	{"kAboveAverageAmountDesc", EObjNameDesc::kAboveAverageAmountDesc},
	{"kBigAmountDesc", EObjNameDesc::kBigAmountDesc},
	{"kLargeAmountDesc", EObjNameDesc::kLargeAmountDesc},
	{"kVeryLargeAmountDesc", EObjNameDesc::kVeryLargeAmountDesc},
	{"kHugeAmountDesc", EObjNameDesc::kHugeAmountDesc},
	{"kEnormousAmountDesc", EObjNameDesc::kEnormousAmountDesc},
	{"kGiganticAmountDesc", EObjNameDesc::kGiganticAmountDesc},
	{"kColossalAmountDesc", EObjNameDesc::kColossalAmountDesc},
	{"kImmenseAmountDesc", EObjNameDesc::kImmenseAmountDesc}
};

// Read a node's <obj_names><obj_name id= from= val=/> rows into `out`, keyed by tier.
static void ParseObjNames(DataNode node, ObjNameMap &out) {
	out.clear();
	for (auto &group : node.Children("obj_names")) {
		for (auto &row : group.Children("obj_name")) {
			const char *id = row.GetValue("id");
			if (!id || !*id) {
				continue;
			}
			const auto kit = kObjNameDescByName.find(id);
			if (kit == kObjNameDescByName.end()) {
				err_log("currency obj_name: unknown tier id '%s'.", id);
				continue;
			}
			ObjNamePattern pattern;
			try {
				pattern.from = parse::ReadAsInt(row.GetValue("from"));
			} catch (...) {
				pattern.from = 0;
			}
			const char *val = row.GetValue("val");
			pattern.tpl = (val && *val) ? val : "";
			if (!pattern.tpl.empty()) {
				out[kit->second] = std::move(pattern);
			}
		}
	}
}

static void LoadCurrencyNames(DataNode data) {
	g_currency_names.clear();
	for (auto &sheaf : data.Children("msg_sheaf")) {
		const char *id = sheaf.GetValue("id");
		if (!id || !*id) {
			continue;
		}
		CurrencyName cn;
		if (sheaf.GoToChild("name")) {
			try {
				cn.gender = parse::ReadAsConstant<EGender>(sheaf.GetValue("gender"));
			} catch (std::exception &) {}
			const char *search = sheaf.GetValue("search");
			cn.search = (search && *search) ? search : id;
			if (auto built = grammar::ItemName::Build(sheaf)) {
				cn.cases = std::move(*built);
			}
			sheaf.GoToParent();
		}
		ParseObjNames(sheaf, cn.obj_names);
		g_currency_names[id] = std::move(cn);
	}

	ParseObjNames(data, g_default_obj_names);
}

void CurrencyNamesLoader::Load(DataNode data) { LoadCurrencyNames(data); }
void CurrencyNamesLoader::Reload(DataNode data) { LoadCurrencyNames(data); }

const CurrencyName *FindCurrencyName(const std::string &text_id) {
	const auto it = g_currency_names.find(text_id);
	return it == g_currency_names.end() ? nullptr : &it->second;
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
	try {
		text_id = parse::ReadAsStr(node.GetValue("text_id"));
	} catch (...) {}

	// issue.thing-names: name (search string + gendered declensions) comes from currency_msg.xml.
	std::string name{"undefined"};
	EGender gender{EGender::kFemale};
	auto names = std::make_unique<grammar::ItemName>();
	ObjNameMap obj_names;
	if (const auto *cn = FindCurrencyName(text_id)) {
		name = cn->search;
		gender = cn->gender;
		names = std::make_unique<grammar::ItemName>(cn->cases);
		obj_names = cn->obj_names;
	} else {
		err_log("currency '%s' has no name in currency_msg.xml.", text_id.c_str());
	}

	auto currency_info = std::make_shared<CurrencyInfo>(vnum, text_id, name, mode);
	currency_info->gender_ = gender;
	currency_info->names_ = std::move(names);
	currency_info->obj_names_ = std::move(obj_names);

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

	return currency_info;
}

void CurrencyInfo::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << "Print currency:" << "\n"
		   << " Vnum: " << kColorGrn << GetId() << kColorNrm << "\r\n"
		   << " TextId: " << kColorGrn << GetTextId() << kColorNrm << "\r\n"
		   << " Name: " << kColorGrn << name_ << kColorNrm << "\r\n"
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n"
		   << " Can be given: " << kColorGrn << (giveable_ ? "Y" : "N") << kColorNrm << "\r\n"
		   << " Can be objected: " << kColorGrn << (objectable_ ? "Y" : "N") << kColorNrm << "\r\n"
		   << " Can be stored in bank: " << kColorGrn << (bankable_ ? "Y" : "N") << kColorNrm << "\r\n"
		   << " Can be transferred: " << kColorGrn << (transferable_ ? "Y" : "N") << kColorNrm << "\r\n"
		   << " Can be transferred to other account: " << kColorGrn << (transferable_to_other_ ? "Y" : "N") << kColorNrm << "\r\n"
		   << " Transfer tax: " << kColorGrn << transfer_tax_ << kColorNrm << "\r\n"
		   << " Drop on death: " << kColorGrn << drop_on_death_ << kColorNrm << "\r\n"
		   << " Max clan tax: " << kColorGrn << max_clan_tax_ << kColorNrm << "\r\n"
		<< "\r\n";
}

const std::string &CurrencyInfo::GetName(grammar::ECase name_case) const {
	return names_->GetSingular(name_case);
}

const std::string &CurrencyInfo::GetPluralName(grammar::ECase name_case) const {
	return names_->GetPlural(name_case);
}

const char *CurrencyInfo::GetCName(grammar::ECase name_case) const {
	return names_->GetSingular(name_case).c_str();
}

const char *CurrencyInfo::GetPluralCName(grammar::ECase name_case) const {
	return names_->GetPlural(name_case).c_str();
}

const std::string &CurrencyInfo::GetNameWithAmount(long amount, grammar::ECase one_case) const {
	// Correct Russian numeral agreement: 11-14 / 5-0 -> genitive plural; ends in 1 -> singular
	// (in `one_case`, nominative by default, accusative for "you received 1 ..." contexts);
	// ends in 2-4 -> 'few' form.
	const long n = amount < 0 ? -amount : amount;
	if ((n % 100 >= 11 && n % 100 <= 14) || n % 10 >= 5 || n % 10 == 0) {
		return GetPluralName(grammar::ECase::kGen);
	}
	if (n % 10 == 1) {
		return GetName(one_case);
	}
	return GetPluralName(grammar::ECase::kAcc);
}

std::string CurrencyInfo::GetObjName(long amount, grammar::ECase gram_case) const {
	if (amount <= 0) {
		log("SYSERR: Try to create negative or 0 money (%ld).", amount);
		return {};
	}
	if (amount == 1) {
		std::ostringstream out;
		out << "од" << grammar::OneNumeralEnding(GetGender(), gram_case) << " " << GetName(gram_case);
		return out.str();
	}
	// issue.currencies: amount >= 2 -- merge this currency's tier overrides onto the default
	// tiers, then pick the tier with the largest `from` <= amount and fill the placeholders.
	ObjNameMap tiers = g_default_obj_names;
	for (const auto &[key, pattern] : obj_names_) {
		tiers[key] = pattern;
	}
	const ObjNamePattern *chosen = nullptr;
	for (const auto &[key, pattern] : tiers) {
		if (amount >= pattern.from && (!chosen || pattern.from > chosen->from)) {
			chosen = &pattern;
		}
	}
	if (!chosen) {
		return GetPluralName(grammar::ECase::kGen);
	}
	std::string result = chosen->tpl;
	const auto fill = [&result](const char *placeholder, const std::string &value) {
		for (std::size_t pos; (pos = result.find(placeholder)) != std::string::npos; ) {
			result.replace(pos, std::char_traits<char>::length(placeholder), value);
		}
	};
	fill("{adjective_ending}", grammar::CountedFormEnding(gram_case, 0));
	fill("{noun_ending_hard}", grammar::CountedFormEnding(gram_case, 2));
	fill("{noun_ending}", grammar::CountedFormEnding(gram_case, 1));
	fill("{currency_name}", GetPluralName(grammar::ECase::kGen));
	return result;
}
const char *CurrencyInfo::GetObjCName(long amount, grammar::ECase gram_case) const {
	static char buf[128];
	sprintf(buf, "%s", GetObjName(amount, gram_case).c_str());
	return buf;
}

std::string TextIdByVnum(int vnum) {
	return MUD::Currency(vnum).GetTextId();
}

long GetAmount(const CharData &ch, const std::string &id, EPurse purse) {
	const auto &cs = ch.currency_storage();
	return purse == EPurse::kHand ? cs.GetHand(id) : cs.GetBank(id);
}

long GetTotal(const CharData &ch, const std::string &id) {
	const auto &cs = ch.currency_storage();
	return cs.GetHand(id) + cs.GetBank(id);
}

void SetAmount(CharData &ch, const std::string &id, EPurse purse, long amount) {
	amount = std::clamp(amount, 0L, kMaxMoneyKept);
	auto &cs = ch.currency_storage();
	if (purse == EPurse::kHand) {
		cs.SetHand(id, amount);
	} else {
		cs.SetBank(id, amount);
	}
}

long AddAmount(CharData &ch, const std::string &id, EPurse purse, long amount) {
	const long before = GetAmount(ch, id, purse);
	SetAmount(ch, id, purse, before + amount);
	return GetAmount(ch, id, purse) - before;
}

long RemoveAmount(CharData &ch, const std::string &id, EPurse purse, long amount) {
	const long have = GetAmount(ch, id, purse);
	if (have >= amount) {
		SetAmount(ch, id, purse, have - amount);
		return 0;
	}
	SetAmount(ch, id, purse, 0);
	return amount - have;
}

long GetAmount(const CharData &ch, int vnum, EPurse purse) { return GetAmount(ch, TextIdByVnum(vnum), purse); }
long GetTotal(const CharData &ch, int vnum) { return GetTotal(ch, TextIdByVnum(vnum)); }
void SetAmount(CharData &ch, int vnum, EPurse purse, long amount) { SetAmount(ch, TextIdByVnum(vnum), purse, amount); }
long AddAmount(CharData &ch, int vnum, EPurse purse, long amount) { return AddAmount(ch, TextIdByVnum(vnum), purse, amount); }
long RemoveAmount(CharData &ch, int vnum, EPurse purse, long amount) { return RemoveAmount(ch, TextIdByVnum(vnum), purse, amount); }

} // namespace currencies

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
