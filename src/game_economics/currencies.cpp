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

	try {
		currency_info->locked_ = parse::ReadAsBool(node.GetValue("locked"));
		currency_info->account_shared_ = parse::ReadAsBool(node.GetValue("account_shared"));
	} catch (...) {}

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
			err_log("permits (%s) in currency '%s'.", e.what(), currency_info->name_.c_str());
		}
		node.GoToParent();
	}

	if (node.GoToChild("name")) {
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

const std::string &CurrencyInfo::GetNameWithQuantity(uint64_t quantity) const {
	auto remainder = quantity % 20;
	if ((remainder >= 5 && remainder <= 19) || remainder == 0) {
		return GetPluralName(ECase::kGen);
	} else if (remainder >= 2 && remainder <= 4) {
		return GetPluralName(ECase::kAcc);
	} else {
		return GetName();
	}
}

/*std::string CurrenciesInfo::GetCurrencyObjDescription(Vnum currency_id, long quantity, ECase name_case) {
	using Cases = std::unordered_map<ECase, std::string>;
	using CurrencyObjDescription = std::unordered_map<ENumber, Cases>;
	static const std::vector<CurrencyObjDescription> descriptions {	{
			{ENumber::kSingular, {
				{ECase::kNom, ""},
				{ECase::kGen, ""},
				{ECase::kDat, ""},
				{ECase::kAcc, ""},
				{ECase::kIns, ""},
				{ECase::kPre, ""},
			}
			},
			{ENumber::kPlural, {
				{ECase::kNom, ""},
				{ECase::kGen, ""},
				{ECase::kDat, ""},
				{ECase::kAcc, ""},
				{ECase::kIns, ""},
				{ECase::kPre, ""},
			}
			},
		}
		};

	return std::string();
}*/

char *GetCurrencyObjDescription(Vnum currency_id, long quantity, ECase gram_case) {
	static char buf[128];
	const char *single[6][3] = {{"на", "ин", "о"},
								{"ной", "ого", "ого"},
								{"ной", "ому", "ому"},
								{"ну", "ого", "о"},
								{"ной", "ним", "им"},
								{"ной", "ном", "ом"}
	}, *plural[6][3] =
		{
			{
				"ая", "а", "а"}, {
				"ой", "и", "ы"}, {
				"ой", "е", "е"}, {
				"ую", "у", "у"}, {
				"ой", "ой", "ой"}, {
				"ой", "е", "е"}
		};


	if (quantity <= 0) {
		log("SYSERR: Try to create negative or 0 money (%ld).", quantity);
		return (nullptr);
	}
	if (quantity == 1) {
		//sprintf(buf, "одн%s кун%s", single[padis][0], single[padis][1]);
		sprintf(buf, "од%s %s",
				single[gram_case][0], MUD::Currencies(currency_id).GetCName(gram_case));
	} else if (quantity <= 10)
		sprintf(buf, "малюсеньк%s горстк%s %s",
				plural[gram_case][0], plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 20)
		sprintf(buf, "маленьк%s горстк%s %s",
				plural[gram_case][0], plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 75)
		sprintf(buf, "небольш%s горстк%s %s",
				plural[gram_case][0], plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 200)
		sprintf(buf, "маленьк%s кучк%s %s",
				plural[gram_case][0], plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 1000)
		sprintf(buf, "небольш%s кучк%s %s",
				plural[gram_case][0], plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 5000)
		sprintf(buf, "кучк%s %s",
				plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 10000)
		sprintf(buf, "больш%s кучк%s %s",
				plural[gram_case][0], plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 20000)
		sprintf(buf, "груд%s %s",
				plural[gram_case][2],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 75000)
		sprintf(buf, "больш%s груд%s %s",
				plural[gram_case][0], plural[gram_case][2],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 150000)
		sprintf(buf, "горк%s %s",
				plural[gram_case][1],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else if (quantity <= 250000)
		sprintf(buf, "гор%s %s",
				plural[gram_case][2],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));
	else
		sprintf(buf, "огромн%s гор%s %s",
				plural[gram_case][0], plural[gram_case][2],
				MUD::Currencies(currency_id).GetPluralCName(ECase::kGen));

	return (buf);
}

ObjData::shared_ptr CreateCurrencyObj(long quantity) {
	char buf[200];

	if (quantity <= 0) {
		log("SYSERR: Try to create negative or 0 money. (%ld)", quantity);
		return (nullptr);
	}
	auto obj = world_objects.create_blank();
	ExtraDescription::shared_ptr new_descr(new ExtraDescription());

	if (quantity == 1) {
		sprintf(buf, "coin gold кун деньги денег монет %s", GetCurrencyObjDescription(KunaVnum, quantity, ECase::kNom));
		obj->set_aliases(buf);
		obj->set_short_description("куна");
		obj->set_description("Одна куна лежит здесь.");
		new_descr->keyword = str_dup("coin gold монет кун денег");
		new_descr->description = str_dup("Всего лишь одна куна.");
		for (int i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
			obj->set_PName(i, GetCurrencyObjDescription(KunaVnum, quantity, static_cast<ECase>(i)));
		}
	} else {
		sprintf(buf, "coins gold кун денег %s", GetCurrencyObjDescription(KunaVnum, quantity, ECase::kNom));
		obj->set_aliases(buf);
		obj->set_short_description(GetCurrencyObjDescription(KunaVnum, quantity, ECase::kNom));
		for (int i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
			obj->set_PName(i, GetCurrencyObjDescription(KunaVnum, quantity, static_cast<ECase>(i)));
		}

		sprintf(buf, "Здесь лежит %s.", GetCurrencyObjDescription(KunaVnum, quantity, ECase::kNom));
		obj->set_description(CAP(buf));

		new_descr->keyword = str_dup("coins gold кун денег");
	}

	new_descr->next = nullptr;
	obj->set_ex_description(new_descr);

	obj->set_type(EObjType::kMoney);
	obj->set_wear_flags(to_underlying(EWearFlag::kTake));
	obj->set_sex(ESex::kFemale);
	obj->set_val(0, quantity);
	obj->set_cost(quantity);
	obj->set_maximum_durability(ObjData::DEFAULT_MAXIMUM_DURABILITY);
	obj->set_current_durability(ObjData::DEFAULT_CURRENT_DURABILITY);
	obj->set_timer(24 * 60 * 7);
	obj->set_weight(1);
	obj->set_extra_flag(EObjFlag::kNodonate);
	obj->set_extra_flag(EObjFlag::kNosell);

	return obj;
}

} // namespace currencies

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
