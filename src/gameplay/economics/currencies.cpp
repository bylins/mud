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
#include <cctype>

#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "engine/core/comm.h"
#include "utils/utils_string.h"
#include "engine/db/db.h"
#include "engine/entities/zone.h"
#include "gameplay/statistics/money_drop.h"
#include "engine/network/msdp/msdp_constants.h"
#include "gameplay/mechanics/glory_const.h"
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

std::string CurrenciesLoader::EditableWhat() const {
	return "currency";
}

std::vector<cfg_manager::EditableElement> CurrenciesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &cur : MUD::Currencies()) {
		if (cur.GetId() < 0 || cur.IsLocked()) {  // hide the kUndefined sentinel and locked (kuna/glory) currencies
			continue;
		}
		out.push_back({std::to_string(cur.GetId()), cur.GetTextId() + " " + cur.GetName()});
	}
	return out;
}

cfg_manager::ValidationResult CurrenciesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::Currencies().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Currency data failed to parse (see syslog for the offending currency)."};
}

parser_wrapper::DataNode CurrenciesLoader::FindElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// A <currency> is keyed by its integer vnum (it carries no `id` attribute).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "currency" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string CurrenciesLoader::CanonicalElementId(const std::string &id) const {
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	// Locked currencies (kuna vnum 0, glory vnum 1) are not editable/creatable.
	const auto &existing = MUD::Currency(atoi(id.c_str()));
	if (existing.GetId() >= 0 && existing.IsLocked()) {
		return "";
	}
	return id;
}

parser_wrapper::DataNode CurrenciesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	auto node = root.AddChild("currency");
	node.SetValue("vnum", id);
	node.SetValue("text_id", "kUndefined");
	node.SetValue("mode", "kEnabled");
	return node;
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
	g_default_obj_names.clear();
	for (auto &sheaf : data.Children("msg_sheaf")) {
		const char *id = sheaf.GetValue("id");
		if (!id || !*id) {
			continue;
		}
		// The shared "kDefault" sheaf carries the fallback size-tier templates for every currency.
		if (std::string(id) == "kDefault") {
			ParseObjNames(sheaf, g_default_obj_names);
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
}

void CurrencyNamesLoader::Load(DataNode data) { LoadCurrencyNames(data); }
void CurrencyNamesLoader::Reload(DataNode data) { LoadCurrencyNames(data); }

std::string CurrencyNamesLoader::EditableWhat() const {
	return "currencyname";
}

std::vector<cfg_manager::EditableElement> CurrencyNamesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &[id, cn] : g_currency_names) {
		out.push_back({id, id + " (" + cn.search + ")"});
	}
	return out;
}

cfg_manager::ValidationResult CurrencyNamesLoader::Validate(parser_wrapper::DataNode & /*doc*/) const {
	return {true, ""};  // names are free-form text; a malformed reload is reported in syslog
}

parser_wrapper::DataNode CurrencyNamesLoader::FindElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "msg_sheaf" && id == child.GetValue("id")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string CurrencyNamesLoader::CanonicalElementId(const std::string &id) const {
	return id.empty() ? "" : id;  // any currency text_id is a valid names key (create allowed)
}

parser_wrapper::DataNode CurrencyNamesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	auto sheaf = root.AddChild("msg_sheaf");
	sheaf.SetValue("id", id);
	auto name = sheaf.AddChild("name");
	name.SetValue("gender", "kFemale");
	name.SetValue("search", id);
	name.AddChild("singular");
	name.AddChild("plural");
	return sheaf;
}

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
			if (const char *v = node.GetValue("logged"); v && *v) currency_info->logged_ = parse::ReadAsBool(v);
			if (const char *v = node.GetValue("msdp"); v && *v) currency_info->reports_msdp_ = parse::ReadAsBool(v);
			if (const char *v = node.GetValue("money_stat"); v && *v) currency_info->money_stat_ = parse::ReadAsBool(v);
			if (const char *v = node.GetValue("force_split"); v && *v) currency_info->force_split_ = parse::ReadAsBool(v);
			if (const char *v = node.GetValue("arena_only"); v && *v) currency_info->arena_only_ = parse::ReadAsBool(v);
			if (const char *v = node.GetValue("merchant_payout"); v && *v) currency_info->merchant_payout_ = parse::ReadAsBool(v);
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

	if (node.GoToChild("limits")) {
		try {
			currency_info->max_amount_ = std::clamp(static_cast<long>(parse::ReadAsInt(node.GetValue("max"))), 0L, kMaxMoneyKept);
		} catch (std::runtime_error &e) {
			err_log("incorrect limits (%s) in currency '%s'.", e.what(), currency_info->name_.c_str());
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

// EPurse is an internal hand/bank selector; the public API is GetHand/GetBank/etc.
enum class EPurse { kHand, kBank };

std::string TextIdByVnum(int vnum) {
	return MUD::Currency(vnum).GetTextId();
}

long GetAmount(const CharData &ch, const std::string &id, EPurse purse = EPurse::kHand) {
	// P4: const glory is not stored in the container; it delegates to the GloryConst store.
	if (id == kGlory) {
		return GloryConst::get_glory(ch.get_uid());
	}
	const auto &cs = ch.currency_storage();
	return purse == EPurse::kHand ? cs.GetHand(id) : cs.GetBank(id);
}

long GetTotal(const CharData &ch, const std::string &id) {
	if (id == kGlory) {
		return GloryConst::get_glory(ch.get_uid());
	}
	const auto &cs = ch.currency_storage();
	return cs.GetHand(id) + cs.GetBank(id);
}

void SetAmount(CharData &ch, const std::string &id, long amount, EPurse purse = EPurse::kHand, bool with_log = true, bool immortal = false) {
	if (id == kGlory) {
		// Const glory lives in the GloryConst store; raising it is an immortal-only operation.
		amount = std::max(0L, amount);
		const long current = GloryConst::get_glory(ch.get_uid());
		if (amount > current) {
			if (!immortal) {
				log("SYSERR: refused to grant glory to %s via the currency API without immortal rights", ch.get_name().c_str());
				return;
			}
			GloryConst::add_glory(ch.get_uid(), amount - current);
		} else if (amount < current) {
			GloryConst::remove_glory(ch.get_uid(), current - amount);
		}
		return;
	}
	const auto &info = MUD::Currencies().FindAvailableItem(id);
	const long before = GetAmount(ch, id, purse);
	amount = std::clamp(amount, 0L, info.GetMaxAmount());
	if (amount == before) {
		return;
	}
	auto &cs = ch.currency_storage();
	if (purse == EPurse::kHand) {
		cs.SetHand(id, amount);
	} else {
		cs.SetBank(id, amount);
	}
	const long change = amount - before;
	if (with_log && !ch.IsNpc()) {
		if (info.IsLogged()) {
			log("Gold: %s %s %ld", ch.get_name().c_str(), change > 0 ? "add" : "remove", change > 0 ? change : -change);
		}
		if (info.TracksMoneyDrop() && purse == EPurse::kHand && ch.in_room > 0) {
			MoneyDropStat::add(zone_table[world[ch.in_room]->zone_rn].vnum, change);
		}
	}
	if (info.ReportsMsdp()) {
		ch.msdp_report(msdp::constants::GOLD);
	}
}

long AddAmount(CharData &ch, const std::string &id, long amount, EPurse purse = EPurse::kHand, bool notify = false, bool with_log = true, bool immortal = false) {
	amount = std::max(0L, amount);
	const long before = GetAmount(ch, id, purse);
	SetAmount(ch, id, before + amount, purse, with_log, immortal);
	const long added = GetAmount(ch, id, purse) - before;
	if (notify && added > 0 && ch.desc) {
		const auto &info = MUD::Currencies().FindAvailableItem(id);
		if (added < amount) {
			SendMsgToChar(&ch, "Вы получили только %ld %s, больше не помещается.\r\n",
				added, info.GetNameWithAmount(added, grammar::ECase::kAcc).c_str());
		} else {
			SendMsgToChar(&ch, "Вы получили %ld %s.\r\n",
				added, info.GetNameWithAmount(added, grammar::ECase::kAcc).c_str());
		}
	}
	return added;
}

long RemoveAmount(CharData &ch, const std::string &id, long amount, EPurse purse = EPurse::kHand, bool with_log = true) {
	amount = std::max(0L, amount);
	const long have = GetAmount(ch, id, purse);
	if (have >= amount) {
		SetAmount(ch, id, have - amount, purse, with_log);
		return 0;
	}
	SetAmount(ch, id, 0, purse, with_log);
	return amount - have;
}

long RemoveTotal(CharData &ch, const std::string &id, long amount, bool with_log) {
	const long rest = RemoveAmount(ch, id, amount, EPurse::kBank, with_log);
	return RemoveAmount(ch, id, rest, EPurse::kHand, with_log);
}

long GetTotal(const CharData &ch, int vnum) { return GetTotal(ch, TextIdByVnum(vnum)); }
long RemoveTotal(CharData &ch, int vnum, long amount, bool with_log) { return RemoveTotal(ch, TextIdByVnum(vnum), amount, with_log); }

// ---- Separate hand/bank public API (EPurse stays an internal detail) ----
long GetHand(const CharData &ch, const std::string &id) { return GetAmount(ch, id, EPurse::kHand); }
long GetBank(const CharData &ch, const std::string &id) { return GetAmount(ch, id, EPurse::kBank); }
void SetHand(CharData &ch, const std::string &id, long amount, bool with_log, bool immortal) { SetAmount(ch, id, amount, EPurse::kHand, with_log, immortal); }
void SetBank(CharData &ch, const std::string &id, long amount, bool with_log, bool immortal) { SetAmount(ch, id, amount, EPurse::kBank, with_log, immortal); }
long AddHand(CharData &ch, const std::string &id, long amount, bool notify, bool with_log, bool immortal) { return AddAmount(ch, id, amount, EPurse::kHand, notify, with_log, immortal); }
long AddBank(CharData &ch, const std::string &id, long amount, bool notify, bool with_log, bool immortal) { return AddAmount(ch, id, amount, EPurse::kBank, notify, with_log, immortal); }
long RemoveHand(CharData &ch, const std::string &id, long amount, bool with_log) { return RemoveAmount(ch, id, amount, EPurse::kHand, with_log); }
long RemoveBank(CharData &ch, const std::string &id, long amount, bool with_log) { return RemoveAmount(ch, id, amount, EPurse::kBank, with_log); }

long GetHand(const CharData &ch, int vnum) { return GetHand(ch, TextIdByVnum(vnum)); }
long GetBank(const CharData &ch, int vnum) { return GetBank(ch, TextIdByVnum(vnum)); }
void SetHand(CharData &ch, int vnum, long amount, bool with_log, bool immortal) { SetHand(ch, TextIdByVnum(vnum), amount, with_log, immortal); }
void SetBank(CharData &ch, int vnum, long amount, bool with_log, bool immortal) { SetBank(ch, TextIdByVnum(vnum), amount, with_log, immortal); }
long AddHand(CharData &ch, int vnum, long amount, bool notify, bool with_log, bool immortal) { return AddHand(ch, TextIdByVnum(vnum), amount, notify, with_log, immortal); }
long AddBank(CharData &ch, int vnum, long amount, bool notify, bool with_log, bool immortal) { return AddBank(ch, TextIdByVnum(vnum), amount, notify, with_log, immortal); }
long RemoveHand(CharData &ch, int vnum, long amount, bool with_log) { return RemoveHand(ch, TextIdByVnum(vnum), amount, with_log); }
long RemoveBank(CharData &ch, int vnum, long amount, bool with_log) { return RemoveBank(ch, TextIdByVnum(vnum), amount, with_log); }


const CurrencyInfo *FindBySearch(const std::string &word) {
	if (word.empty()) {
		return nullptr;
	}
	for (const auto &cur : MUD::Currencies()) {
		if (cur.GetId() < 0) {
			continue;
		}
		const CurrencyName *cn = FindCurrencyName(cur.GetTextId());
		// Same matcher as spells/skills (FixDot + ordered abbreviation of each word), so "сереб.грив"
		// matches "серебряная гривна" and a single "лед" matches "волшебный лед".
		if (cn && utils::IsEquivalent(word, cn->search)) {
			return &cur;
		}
	}
	return nullptr;
}

const CurrencyInfo &FindByTextIdNoCase(const std::string &text_id) {
	// Fast path: exact match. DG scripts lowercase the whole command line, so a camelCase text_id
	// like "kSilverGrivna" arrives as "ksilvergrivna" -- fall back to a case-insensitive scan.
	const auto &exact = MUD::Currencies().FindAvailableItem(text_id);
	if (exact.GetId() >= 0) {
		return exact;
	}
	auto lower = [](std::string v) {
		std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return v;
	};
	const std::string needle = lower(text_id);
	for (const auto &cur : MUD::Currencies()) {
		if (cur.GetId() >= 0 && lower(cur.GetTextId()) == needle) {
			return cur;
		}
	}
	return exact;  // the kUndefined sentinel
}
} // namespace currencies

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
