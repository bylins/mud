/**
* \file mob_classes_info.cpp - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 25.11.2025.
 * \brief Brief description.
 * \details Detailed description.
 */

#include "gameplay/classes/mob_classes_info.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "engine/entities/entities_constants.h"

EMobClass FindAvailableMobClassId(const CharData *ch, const std::string &mob_class_name) {
    for (const auto &it: MUD::MobClasses()) {
        if (it.IsAvailable()) {
            SendMsgToChar(ch, "Нашёл доступный класс! ID класса: %d!\r\n", int(it.GetId()));
            SendMsgToChar(ch, "Имя класса: %s!\r\n", it.GetName());
            if (CompareParam(mob_class_name, it.GetName()) || CompareParam(mob_class_name, it.GetAbbr())) {
                SendMsgToChar("Название класса совпадает!\r\n", ch);
                return it.GetId();
            }
        } else {
            SendMsgToChar(ch, "Недоступен класс с названием: %s!\r\n", it.GetName());
        }
    }
    if (MUD::MobClasses().begin() == MUD::MobClasses().end()) {
        SendMsgToChar("Список классов пуст!\r\n", ch);
    }
    return EMobClass::kUndefined;
}

namespace {

// глобальная настройка: сколько уровней моба добавлять за один реморт игрока
int g_mob_lvl_per_mort = 0;

} // namespace

namespace mob_classes {

using DataNode = parser_wrapper::DataNode;
using MobItemPtr = MobClassInfoBuilder::ItemPtr;

// ------- Глобальный доступ к mob_lvl_per_mort -------

int GetMobLvlPerMort() {
    return g_mob_lvl_per_mort;
}

void SetMobLvlPerMort(int v) {
    if (v < 0) {
        v = 0;
    }
    g_mob_lvl_per_mort = v;
}

// ------- Loader -------

void MobClassesLoader::Load(DataNode data) {
    ParseGlobalVarsFromRoot(data);
    MUD::MobClasses().Init(data.Children());
}

void MobClassesLoader::Reload(DataNode data) {
    ParseGlobalVarsFromRoot(data);
    MUD::MobClasses().Reload(data.Children());
}

void MobClassesLoader::ParseGlobalVarsFromRoot(DataNode &data) {
    if (!data.GoToChild("global_vars")) {
        return;
    }

    // low_skill_lvl
    try {
        const int lvl = parse::ReadAsInt(data.GetValue("low_skill_lvl"));
        MobClassInfoBuilder::SetDefaultLowSkillLvl(lvl);
    } catch (const std::exception &e) {
        err_log("Incorrect 'global_vars.low_skill_lvl' at root (value: %s).", e.what());
    }

    // Новый параметр: mob_lvl_per_mort (может быть не задан)
    try {
        const int per = parse::ReadAsInt(data.GetValue("mob_lvl_per_mort"));
        SetMobLvlPerMort(per);
    } catch (const std::exception &e) {
        // Если атрибут отсутствует или некорректен ? просто залогируем, но игру не уронем.
        err_log("Incorrect 'global_vars.mob_lvl_per_mort' at root (value: %s).", e.what());
    }

    data.GoToParent();
}

// ---------- Builder ----------

int MobClassInfoBuilder::s_default_low_skill_lvl = 0;

void MobClassInfoBuilder::SetDefaultLowSkillLvl(int v) {
    s_default_low_skill_lvl = v;
}

MobClassInfoBuilder::ItemPtr MobClassInfoBuilder::Build(DataNode &node) {
    // Быстрый фильтр: у mobclass есть атрибут id; у global_vars его нет.
    try {
        (void)node.GetValue("id"); // если нет ? бросит исключение
    } catch (...) {
        return nullptr; // не mobclass ? пропускаем
    }

    try {
        return ParseMobClass(node);
    } catch (const std::exception &e) {
        err_log("Mob class parsing error (incorrect value '%s')", e.what());
        return nullptr;
    }
}

MobClassInfoBuilder::ItemPtr MobClassInfoBuilder::ParseMobClass(DataNode node) {
    auto info = ParseHeader(node);
    ParseSavings(info, node);
    ParseResistances(info, node);
    ParseName(info, node);
    ParseMobSkills(info, node);
    ParseMobSpells(info, node);
    return info;
}

MobClassInfoBuilder::ItemPtr MobClassInfoBuilder::ParseHeader(DataNode &node) {
    auto id{EMobClass::kUndefined};
    try {
        id = parse::ReadAsConstant<EMobClass>(node.GetValue("id"));
    } catch (const std::exception &e) {
        err_log("Incorrect mob class id (%s).", e.what());
        throw;
    }
    auto mode = MobClassInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
    auto info = std::make_shared<MobClassInfo>(id, mode);

    // применяем дефолт из корня
    info->low_skill_lvl = s_default_low_skill_lvl;
    return info;
}

void MobClassInfoBuilder::ParseName(ItemPtr &info, DataNode &node) {
    if (node.GoToChild("name")) {
        try {
            info->name = parse::ReadAsStr(node.GetValue("rus"));
            info->eng_name = parse::ReadAsStr(node.GetValue("eng"));
        } catch (const std::exception &e) {
            err_log("Incorrect 'name' section (mob class: %s, value: %s).",
                    NAME_BY_ITEM(info->GetId()).c_str(), e.what());
        }
        node.GoToParent();
    }
}

// ----- Savings -----

void MobClassInfoBuilder::ParseSavings(ItemPtr &info, DataNode &node) {
    node.GoToChild("savings");
    ParseSavingsData(info, node);
    node.GoToParent();
}

void MobClassInfoBuilder::ParseSavingsData(ItemPtr &info, DataNode &node) {
    auto saving_data = MobClassInfo::ParametersData();
    for (const auto &stat_node : node.Children("saving")) {
        ESaving id;
        try {
            id = parse::ReadAsConstant<ESaving>(stat_node.GetValue("id"));
            saving_data.base = parse::ReadAsInt(stat_node.GetValue("base"));
            saving_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            saving_data.deviation = parse::ReadAsInt(stat_node.GetValue("deviation"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect base saving format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }
        info->savings_map[id] = saving_data;
    }
}

// ----- Resists -----

void MobClassInfoBuilder::ParseResistances(ItemPtr &info, DataNode &node) {
    node.GoToChild("resistances");
    ParseResistanceData(info, node);
    node.GoToParent();
}

void MobClassInfoBuilder::ParseResistanceData(ItemPtr &info, DataNode &node) {
    auto resistance_data = MobClassInfo::ParametersData();
    for (const auto &stat_node : node.Children("resistance")) {
        EResist id;
        try {
            id = parse::ReadAsConstant<EResist>(stat_node.GetValue("id"));
            resistance_data.base = parse::ReadAsInt(stat_node.GetValue("base"));
            resistance_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            resistance_data.deviation = parse::ReadAsInt(stat_node.GetValue("deviation"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect base saving format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }
        info->resists_map[id] = resistance_data;
    }
}

// ----- Skills -----

void MobClassInfoBuilder::ParseMobSkills(ItemPtr &info, DataNode &node) {
    node.GoToChild("skills");
    ParseMobSkillsData(info, node);
    node.GoToParent();
}

void MobClassInfoBuilder::ParseMobSkillsData(ItemPtr &info, DataNode &node) {
    auto mob_skills_data = MobClassInfo::ParametersData();
    for (const auto &stat_node : node.Children("skill")) {
        ESkill id;
        try {
            id = parse::ReadAsConstant<ESkill>(stat_node.GetValue("id"));
            mob_skills_data.base = parse::ReadAsInt(stat_node.GetValue("base"));
            mob_skills_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            mob_skills_data.deviation = parse::ReadAsInt(stat_node.GetValue("deviation"));
            mob_skills_data.low_increment = parse::ReadAsFloat(stat_node.GetValue("low_increment"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect skill format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }
        info->mob_skills_map[id] = mob_skills_data;
    }
}

void MobClassInfoBuilder::ParseMobSpells(ItemPtr &info, DataNode &node) {
    if (!node.GoToChild("spells")) {
        return;
    }

    // обходим всех <spell .../>
    for (const auto &spell_node : node.Children("spell")) {
        try {
            ESpell id = parse::ReadAsConstant<ESpell>(spell_node.GetValue("id"));
            info->mob_spells.push_back(id);
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect spell format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }
    }

    node.GoToParent();
}



// ----- Print -----

void MobClassInfo::PrintSavingsTable(CharData *ch, std::ostringstream &buffer) const {
    buffer << "\r\n" << kColorGrn << " Savings:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Saving" << "Base" << "Increment" << "Deviation" << table_wrapper::kEndRow;
    for (const auto &saving : savings_map) {
        table << NAME_BY_ITEM<ESaving>(saving.first)
              << saving.second.base
              << saving.second.increment
              << saving.second.deviation
              << table_wrapper::kEndRow;
    }
    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}

void MobClassInfo::PrintResistancesTable(CharData *ch, std::ostringstream &buffer) const {
    buffer << "\r\n" << kColorGrn << " Resistances:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Resistance" << "Base" << "Increment" << "Deviation" << table_wrapper::kEndRow;
    for (const auto &resistance : resists_map) {
        table << NAME_BY_ITEM<EResist>(resistance.first)
              << resistance.second.base
              << resistance.second.increment
              << resistance.second.deviation
              << table_wrapper::kEndRow;
    }
    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}

void MobClassInfo::PrintMobSkillsTable(CharData *ch, std::ostringstream &buffer) const {
    buffer << "\r\n" << kColorGrn << " Skills:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Skill" << "Base" << "Increment" << "Deviation" << "Low-Lvl-Increment" << table_wrapper::kEndRow;
    for (const auto &mob_skill : mob_skills_map) {
        table << NAME_BY_ITEM<ESkill>(mob_skill.first)
              << mob_skill.second.base
              << mob_skill.second.increment
              << mob_skill.second.deviation
              << mob_skill.second.low_increment
              << table_wrapper::kEndRow;
    }
    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}

void MobClassInfo::PrintMobSpellsTable(CharData *ch, std::ostringstream &buffer) const {
    if (mob_spells.empty()) {
        return;
    }

    buffer << "\r\n" << kColorGrn << " Spells:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Spell"
          << table_wrapper::kEndRow;

    for (std::size_t i = 0; i < mob_spells.size(); ++i) {
        table << NAME_BY_ITEM<ESpell>(mob_spells[i])
              << table_wrapper::kEndRow;
    }

    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}


void MobClassInfo::Print(CharData *ch, std::ostringstream &buffer) const {
    buffer << "Print mob class:\r\n"
           << " Low-skill-lvl: " << kColorGrn << low_skill_lvl << kColorNrm << "\r\n"
           << " Id: " << kColorGrn << NAME_BY_ITEM<EMobClass>(GetId()) << kColorNrm << "\r\n"
           << " Название: " << kColorGrn << name << kColorNrm << "\r\n"
           << " Англ.: " << kColorGrn << eng_name << kColorNrm << "\r\n"
           << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n";
    PrintSavingsTable(ch, buffer);
    PrintResistancesTable(ch, buffer);
    PrintMobSkillsTable(ch, buffer);
    PrintMobSpellsTable(ch, buffer);
}

} // namespace mob_classes
