/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "pc_classes_info.h"

#include "engine/ui/color.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "engine/db/global_objects.h"
//#include "utils/table_wrapper.h"
#include "engine/entities/entities_constants.h"

namespace classes {

using DataNode = parser_wrapper::DataNode;
using ItemPtr = CharClassInfoBuilder::ItemPtr;

void ClassesLoader::Load(DataNode data) {
	MUD::Classes().Init(data.Children());
}

void ClassesLoader::Reload(DataNode data) {
	MUD::Classes().Reload(data.Children());
}

ItemPtr CharClassInfoBuilder::Build(DataNode &node) {
	try {
		auto class_node = SelectDataNode(node);
		return ParseClass(class_node);
	} catch (std::exception &e) {
		err_log("Class parsing error: '%s'", e.what());
		return nullptr;
	}
}

DataNode CharClassInfoBuilder::SelectDataNode(DataNode &node) {
	auto file_name = GetCfgFileName(node);
	if (file_name && std::filesystem::exists(file_name.value())) {
		auto class_node = parser_wrapper::DataNode(file_name.value());
		if (class_node.IsNotEmpty()) {
			return class_node;
		}
	}
	return node;
}

std::optional<std::string> CharClassInfoBuilder::GetCfgFileName(DataNode &node) {
	const char *file_name;
	try {
		file_name = parse::ReadAsStr(node.GetValue("file"));
	} catch (std::exception &) {
		return std::nullopt;
	}
	std::optional<std::string> full_file_name{file_name};
	return full_file_name;
}

ItemPtr CharClassInfoBuilder::ParseClass(DataNode &node) {
	auto id{ECharClass::kUndefined};
	try {
		id = parse::ReadAsConstant<ECharClass>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect class id (%s).", e.what());
		return nullptr;
	}
	auto mode = CharClassInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	auto info = std::make_shared<CharClassInfo>(id, mode);

	if (node.GoToChild("name")) {
		ParseName(info, node);
	}

	if (node.GoToSibling("stats")) {
		ParseStats(info, node);
	}

	if (node.GoToSibling("skills")) {
		ParseSkills(info, node);
	}

	if (node.GoToSibling("spells")) {
		ParseSpells(info, node);
	}

	if (node.GoToSibling("feats")) {
		ParseFeats(info, node);
	}

	TemporarySetStat(info);	// Временное проставление параметроа не из файла, а вручную

	return info;
}

void CharClassInfoBuilder::ParseStats(ItemPtr &info, DataNode &node) {
	node.GoToChild("base_stats");
	ParseBaseStats(info, node);

	node.GoToParent();
}

void CharClassInfoBuilder::ParseBaseStats(ItemPtr &info, DataNode &node) {
	auto base_stat_limits = CharClassInfo::BaseStatLimits();
	for (const auto &stat_node : node.Children("stat")) {
		EBaseStat id;
		try {
			id = parse::ReadAsConstant<EBaseStat>(stat_node.GetValue("id"));
			base_stat_limits.gen_min = parse::ReadAsInt(stat_node.GetValue("gen_min"));
			base_stat_limits.gen_max = parse::ReadAsInt(stat_node.GetValue("gen_max"));
			base_stat_limits.gen_auto = parse::ReadAsInt(stat_node.GetValue("auto"));
			base_stat_limits.cap = parse::ReadAsInt(stat_node.GetValue("cap"));
		} catch (std::exception &e) {
			std::ostringstream out;
			out << "Incorrect base stat limits format (wrong value: " << e.what() << ").";
			throw std::runtime_error(out.str());
		}
		info->base_stats[id] = base_stat_limits;
	}
}

void CharClassInfo::PrintBaseStatsTable(CharData *ch, std::ostringstream &buffer) const {
	buffer << "\r\n" << kColorGrn << " Base stats limits:" << kColorNrm << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader
		  << "Stat" << "Gen Min" << "Gen Max" << "Gen Auto" << "Cap" << table_wrapper::kEndRow;
	for (const auto &base_stat : base_stats) {
		table << NAME_BY_ITEM<EBaseStat>(base_stat.first)
			  << base_stat.second.gen_min
			  << base_stat.second.gen_max
			  << base_stat.second.gen_auto
			  << base_stat.second.cap
			  << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
}

void CharClassInfoBuilder::ParseName(ItemPtr &info, DataNode &node) {
	try {
		info->abbr = parse::ReadAsStr(node.GetValue("abbr"));
	} catch (std::exception &) {
		info->abbr = "--";
	}
	info->names = grammar::ItemName::Build(node);
}

int ParseLevelDecrement(DataNode &node) {
	try {
		return std::max(kMinTalentLevelDecrement, parse::ReadAsInt(node.GetValue("level_decrement")));
	} catch (std::exception &e) {
		return kMinTalentLevelDecrement;
	}
}

// Для парсинга талантов всегда используем Reload (строгий парсинг), потому что класс с неверно прописанными
// талантами не должен быть загружен, иначе при некорректном файле у игроков постираются выученные спеллы/скиллы.

void CharClassInfoBuilder::ParseSkills(ItemPtr &info, DataNode &node) {
	info->skill_level_decrement_ = ParseLevelDecrement(node);
	info->skills.Reload(node.Children());
}

void CharClassInfoBuilder::ParseSpells(ItemPtr &info, DataNode &node) {
	info->spell_level_decrement_ = ParseLevelDecrement(node);
	info->spells.Reload(node.Children());
}

void CharClassInfoBuilder::ParseFeats(ItemPtr &info, DataNode &node) {
	try {
		info->remorts_for_feat_slot_ =
			std::clamp(parse::ReadAsInt(node.GetValue("remorts_for_slot")), 1, kMaxRemort) ;
	} catch (std::exception &e) {
		info->remorts_for_feat_slot_ = kMaxRemort;
	}
	info->feats.Reload(node.Children());
}

void CharClassInfo::PrintHeader(std::ostringstream &buffer) const {
	buffer << "Print class:" << "\n"
		   << " Id: " << kColorGrn << NAME_BY_ITEM<ECharClass>(GetId()) << kColorNrm << "\r\n"
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n"
		   << " Abbr: " << kColorGrn << GetAbbr() << kColorNrm << "\r\n"
		   << " Name: " << kColorGrn << GetName()
		   << "/" << names->GetSingular(ECase::kGen)
		   << "/" << names->GetSingular(ECase::kDat)
		   << "/" << names->GetSingular(ECase::kAcc)
		   << "/" << names->GetSingular(ECase::kIns)
		   << "/" << names->GetSingular(ECase::kPre) << kColorNrm << "\r\n";
}

void CharClassInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	PrintHeader(buffer);
	PrintBaseStatsTable(ch, buffer);
	PrintSkillsTable(ch, buffer);
	PrintSpellsTable(ch, buffer);
	PrintFeatsTable(ch, buffer);
	buffer << "\r\n";
}

const std::string &CharClassInfo::GetAbbr() const {
	return abbr;
};

const std::string &CharClassInfo::GetName(ECase name_case) const {
	return names->GetSingular(name_case);
}

const std::string &CharClassInfo::GetPluralName(ECase name_case) const {
	return names->GetPlural(name_case);
}

const char *CharClassInfo::GetCName(ECase name_case) const {
	return names->GetSingular(name_case).c_str();
}

const char *CharClassInfo::GetPluralCName(ECase name_case) const {
	return names->GetPlural(name_case).c_str();
}

int CharClassInfo::GetMaxCircle() const {
/*	auto it = std::max_element(spells.begin(), spells.end(), [](auto &a, auto &b){
		return (a.GetCircle() < b.GetCircle());
	});*/
	//return it->GetCircle();

	// Это наглый костыль, который поставлен потому, что иначе надо всем классам в конфиге прописывать
	// undefined спелл. Ибо нефиг выделываться, используя внутри инфо_контейнер его же.
	// \todo не забыть убрать
	int circle{0};
	for (const auto &spell : spells) {
		if (spell.IsValid() && spell.GetCircle() > circle) {
			circle = spell.GetCircle();
		}
	}
	return circle;
}

void CharClassInfo::PrintSkillsTable(CharData *ch, std::ostringstream &buffer) const {
	buffer << "\r\n"
		   << kColorGrn << " Available skills (level decrement " << GetSkillLvlDecrement() << "):" << kColorNrm << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader
		<< "Id" << "Skill" << "Lvl" << "Rem" << "Improve" << "Mode" << table_wrapper::kEndRow;
	for (const auto &skill : skills) {
		if (skill.IsInvalid()) {
			continue;
		}
		table << NAME_BY_ITEM<ESkill>(skill.GetId())
				<< MUD::Skill(skill.GetId()).name
				<< skill.GetMinLevel()
				<< skill.GetMinRemort()
				<< skill.GetImprove()
				<< NAME_BY_ITEM<EItemMode>(skill.GetMode()) << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
}

CharClassInfo::SkillInfoBuilder::ItemPtr CharClassInfo::SkillInfoBuilder::Build(DataNode &node) {
	auto skill_id{ESkill::kUndefined};
	int min_lvl, min_remort;
	long improve;
	try {
		skill_id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
		min_lvl = parse::ReadAsInt(node.GetValue("level"));
		min_remort = parse::ReadAsInt(node.GetValue("remort"));
		improve = parse::ReadAsInt(node.GetValue("improve"));
	} catch (std::exception &e) {
		std::ostringstream out;
		out << "Incorrect skill format (wrong value: " << e.what() << ").";
		throw std::runtime_error(out.str());
	}

	auto skill_mode = SkillInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	return std::make_shared<CharClassInfo::SkillInfo>(skill_id, min_lvl, min_remort, improve, skill_mode);
}

void CharClassInfo::PrintSpellsTable(CharData *ch, std::ostringstream &buffer) const {
	buffer << "\r\n"
		   << kColorGrn << " Available spells (level decrement " << GetSpellLvlDecrement() << "):" << kColorNrm << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader
		<< "Id" << "Spell" << "Lvl" << "Rem" << "Circle" << "Mem" << "Cast" << "Mode" << table_wrapper::kEndRow;
	for (const auto &spell : spells) {
		table << NAME_BY_ITEM<ESpell>(spell.GetId())
			<< MUD::Spell(spell.GetId()).GetName()
			<< spell.GetMinLevel()
			<< spell.GetMinRemort()
			<< spell.GetCircle()
			<< spell.GetMemMod()
			<< spell.GetCastMod()
			<< NAME_BY_ITEM<EItemMode>(spell.GetMode()) << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
}

CharClassInfo::SpellInfoBuilder::ItemPtr CharClassInfo::SpellInfoBuilder::Build(DataNode &node) {
	auto spell_id{ESpell::kUndefined};
	int min_lvl, min_remort, circle, mem;
	double cast;
	const int kMinMemMod = -99;
	const int kMaxMemMod = 99;
	try {
		spell_id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
		min_lvl = std::clamp(parse::ReadAsInt(node.GetValue("level")), kMinCharLevel, kMaxPlayerLevel);
		min_remort = std::clamp(parse::ReadAsInt(node.GetValue("remort")), kMinRemort, kMaxRemort);
		circle = std::clamp(parse::ReadAsInt(node.GetValue("circle")), kMinMemoryCircle, kMaxMemoryCircle);
		mem = std::clamp(parse::ReadAsInt(node.GetValue("mem")), kMinMemMod, kMaxMemMod);
		cast = parse::ReadAsDouble(node.GetValue("cast"));
	} catch (std::exception &e) {
		std::ostringstream out;
		out << "Incorrect spell format (wrong value: " << e.what() << ").";
		throw std::runtime_error(out.str());
	}

	auto spell_mode = SpellInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	return std::make_shared<CharClassInfo::SpellInfo>(spell_id, min_lvl, min_remort, circle, mem, cast, spell_mode);
}

void CharClassInfo::PrintFeatsTable(CharData *ch, std::ostringstream &buffer) const {
	buffer << "\r\n" << kColorGrn << " Available feats (new slot every "
		   << GetRemortsNumForFeatSlot() << " remort(s)):" << kColorNrm << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader
		  << "Id" << "Feature" << "Lvl" << "Rem" << "Slot" << "Inborn" << "Mode" << table_wrapper::kEndRow;
	for (const auto &feat : feats) {
		table << NAME_BY_ITEM<EFeat>(feat.GetId())
			<< MUD::Feat(feat.GetId()).GetName()
			<< feat.GetMinLevel()
			<< feat.GetMinRemort()
			<< feat.GetSlot()
			<< (feat.IsInborn() ? "yes" : "no")
			<< NAME_BY_ITEM<EItemMode>(feat.GetMode()) << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
}

CharClassInfo::FeatInfoBuilder::ItemPtr CharClassInfo::FeatInfoBuilder::Build(DataNode &node) {
	auto feat_id{EFeat::kUndefined};
	int min_lvl, min_remort, slot;
	bool inborn{false};
	try {
		feat_id = parse::ReadAsConstant<EFeat>(node.GetValue("id"));
		min_lvl = std::clamp(parse::ReadAsInt(node.GetValue("level")), kMinCharLevel, kMaxPlayerLevel);
		min_remort = std::clamp(parse::ReadAsInt(node.GetValue("remort")), kMinRemort, kMaxRemort);
		slot = std::clamp(parse::ReadAsInt(node.GetValue("slot")), 0, to_underlying(EFeat::kLast));
		inborn = parse::ReadAsBool(node.GetValue("inborn"));
	} catch (std::exception &e) {
		std::ostringstream out;
		out << "Incorrect feat format (wrong value: " << e.what() << ").";
		throw std::runtime_error(out.str());
	}

	auto feat_mode = FeatInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	return std::make_shared<CharClassInfo::FeatInfo>(feat_id, min_lvl, min_remort, slot, inborn, feat_mode);
}

/*
 *  Ниже следует функцияя, проставляющая некоторые параметры класса
 *  На самом деле, почти все проставляемые тут параметры должны быть прочитаны из конфига,
 *  но пока сделано так. Постепенно нужно все это вынести в конфиг.
 */
void CharClassInfoBuilder::TemporarySetStat(ItemPtr &info) {
	switch (info->GetId()) {
		case ECharClass::kSorcerer: {
			info->applies = {40, 10, 12, 50};
		}
			break;
		case ECharClass::kConjurer: {
			info->applies = {35, 10, 10, 50};
		}
			break;
		case ECharClass::kThief: {
			info->applies = {55, 10, 14, 50};
		}
			break;
		case ECharClass::kWarrior: {
			info->applies = {105, 10, 22, 50};
		}
			break;
		case ECharClass::kAssasine: {
			info->applies = {50, 10, 14, 50};
		}
			break;
		case ECharClass::kGuard: {
			info->applies = {105, 10, 17, 50};
		}
			break;
		case ECharClass::kCharmer: {
			info->applies = {35, 10, 10, 50};
		}
			break;
		case ECharClass::kWizard: {
			info->applies = {35, 10, 10, 50};
		}
			break;
		case ECharClass::kNecromancer: {
			info->applies = {35, 10, 11, 50};
		}
			break;
		case ECharClass::kPaladine: {
			info->applies = {100, 10, 14, 50};
		}
			break;
		case ECharClass::kRanger: {
			info->applies = {100, 10, 14, 50};
		}
			break;
		case ECharClass::kVigilant: {
			info->applies = {100, 10, 14, 50};
		}
			break;
		case ECharClass::kMerchant: {
			info->applies = {50, 10, 14, 50};
		}
			break;
		case ECharClass::kMagus: {
			info->applies = {40, 10, 12, 50};
		}
			break;
		default: break;
	}
}

} // namespace clases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
