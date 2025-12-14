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
    ParseBaseStats(info, node);
    ParseSavings(info, node);
    ParseResistances(info, node);
    ParseCombatStats(info, node);
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

    // ----- Base stats -----

void MobClassInfoBuilder::ParseBaseStats(ItemPtr &info, DataNode &node) {
    if (!node.GoToChild("base_stats")) {
        return;
    }

    ParseBaseStatsData(info, node);
    node.GoToParent();
}

    void MobClassInfoBuilder::ParseBaseStatsData(ItemPtr &info, DataNode &node) {
    auto stat_data = MobClassInfo::ParametersData();

    for (const auto &stat_node : node.Children("stat")) {
        EBaseStat id;
        try {
            id = parse::ReadAsConstant<EBaseStat>(stat_node.GetValue("id"));
            stat_data.base = parse::ReadAsInt(stat_node.GetValue("base"));
            stat_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            stat_data.deviation = parse::ReadAsInt(stat_node.GetValue("deviation"));
            stat_data.min_lvl = parse::ReadAsInt(stat_node.GetValue("min_lvl"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect base stat format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        info->base_stats_map[id] = stat_data;
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
            saving_data.min_lvl = parse::ReadAsInt(stat_node.GetValue("min_lvl"));
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
    if (!node.GoToChild("resistances")) {
        return;
    }

    ParseResistanceData(info, node);
    node.GoToParent();
}

void MobClassInfoBuilder::ParseResistanceData(ItemPtr &info, DataNode &node) {
    auto resistance_data = MobClassInfo::ParametersData();

    for (const auto &stat_node : node.Children("resistance")) {
        const char *id_cstr = stat_node.GetValue("id");
        const std::string id_str{id_cstr};

        try {
            resistance_data.base = parse::ReadAsInt(stat_node.GetValue("base"));
            resistance_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            resistance_data.deviation = parse::ReadAsInt(stat_node.GetValue("deviation"));
            resistance_data.min_lvl = parse::ReadAsInt(stat_node.GetValue("min_lvl"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect resistance format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        // Спец-резисты: MR / PR / AR
        if (id_str == "kMagicResist") {
            info->magic_resist = resistance_data;
            info->has_magic_resist = true;
            continue;
        } else if (id_str == "kPhysicResist") {
            info->physical_resist = resistance_data;
            info->has_physical_resist = true;
            continue;
        } else if (id_str == "kAffectResist") {
            info->affect_resist = resistance_data;
            info->has_affect_resist = true;
            continue;
        }

        // Обычный EResist
        EResist id;
        try {
            id = parse::ReadAsConstant<EResist>(id_cstr);
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect resistance id (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        info->resists_map[id] = resistance_data;
    }
}



// ----- Combat stats (armour / absorb) -----

void MobClassInfoBuilder::ParseCombatStats(ItemPtr &info, DataNode &node) {
    if (!node.GoToChild("combat_stats")) {
        return;
    }

    ParseCombatStatsData(info, node);
    node.GoToParent();
}

void MobClassInfoBuilder::ParseCombatStatsData(ItemPtr &info, DataNode &node) {
    MobClassInfo::ParametersData data;

    for (const auto &param_node : node.Children("param")) {
        const char *id_cstr = param_node.GetValue("id");
        const std::string id_str{id_cstr};

        try {
            data.base = parse::ReadAsInt(param_node.GetValue("base"));
            data.increment = parse::ReadAsFloat(param_node.GetValue("increment"));
            data.deviation = parse::ReadAsInt(param_node.GetValue("deviation"));
            data.min_lvl = parse::ReadAsInt(param_node.GetValue("min_lvl"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect combat stat format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        if (id_str == "kArmour") {
            info->armour = data;
            info->has_armour = true;
        } else if (id_str == "kAbsorb") {
            info->absorb = data;
            info->has_absorb = true;
        } else if (id_str == "kDamNoDice") {
            info->dam_n_dice = data;
            info->has_dam_n_dice = true;
        } else if (id_str == "kDamSizeDice") {
            info->dam_s_dice = data;
            info->has_dam_s_dice = true;
        } else if (id_str == "kHitroll") {
            info->hitroll = data;
            info->has_hitroll = true;
        } else if (id_str == "kMorale") {
            info->morale = data;
            info->has_morale = true;
        } else if (id_str == "kCastSuccess") {
            info->cast_success = data;
            info->has_cast_success = true;
        } else if (id_str == "kHitPoints") {
            info->hit_points = data;
            info->has_hit_points = true;
        } else if (id_str == "kSize") {
            info->size = data;
            info->has_size = true;
        } else if (id_str == "kExp") {
            info->exp = data;
            info->has_exp = true;
        } else if (id_str == "kLikesWork") {
            info->likes_work = data;
            info->has_likes_work = true;
        } else if (id_str == "kPhysDamage") {
            info->phys_damage = data;
            info->has_phys_damage = true;
        } else if (id_str == "kSpellPower") {
            info->spell_power = data;
            info->has_spell_power = true;
        } else {
            std::ostringstream out;
            out << "Incorrect combat stat id (wrong value: " << id_str << ").";
            throw std::runtime_error(out.str());
        }
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
            mob_skills_data.min_lvl = parse::ReadAsInt(stat_node.GetValue("min_lvl"));
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

    MobClassInfo::ParametersData spell_data;

    for (const auto &spell_node : node.Children("spell")) {
        ESpell id{};

        try {
            id = parse::ReadAsConstant<ESpell>(spell_node.GetValue("id"));
            spell_data.base = parse::ReadAsInt(spell_node.GetValue("base"));
            spell_data.increment = parse::ReadAsFloat(spell_node.GetValue("increment"));
            spell_data.deviation = parse::ReadAsInt(spell_node.GetValue("deviation"));
            spell_data.min_lvl = parse::ReadAsInt(spell_node.GetValue("min_lvl"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect spell format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        info->mob_spells_map[id] = spell_data;
    }

    node.GoToParent();
}




// ----- Print -----

void MobClassInfo::PrintBaseStatsTable(CharData *ch, std::ostringstream &buffer) const {
    if (base_stats_map.empty()) {
        return;
    }

    buffer << "\r\n" << kColorGrn << " Base stats:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Stat" << "MinLvl" << "Base" << "Increment" << "Deviation" << table_wrapper::kEndRow;

    for (const auto &stat_pair : base_stats_map) {
        const auto &data = stat_pair.second;
        table << NAME_BY_ITEM<EBaseStat>(stat_pair.first)
              << data.min_lvl
              << data.base
              << data.increment
              << data.deviation
              << table_wrapper::kEndRow;
    }

    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}

void MobClassInfo::PrintSavingsTable(CharData *ch, std::ostringstream &buffer) const {
    buffer << "\r\n" << kColorGrn << " Savings:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Saving" << "MinLvl" << "Base" << "Increment" << "Deviation" << table_wrapper::kEndRow;
    for (const auto &saving : savings_map) {
        table << NAME_BY_ITEM<ESaving>(saving.first)
              << saving.second.min_lvl
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
          << "Resistance" << "MinLvl" << "Base" << "Increment" << "Deviation" << table_wrapper::kEndRow;

    // Обычные резисты
    for (const auto &resistance : resists_map) {
        table << NAME_BY_ITEM<EResist>(resistance.first)
              << resistance.second.min_lvl
              << resistance.second.base
              << resistance.second.increment
              << resistance.second.deviation
              << table_wrapper::kEndRow;
    }

    // Доп. резисты
    if (has_magic_resist) {
        table << "MagicResist"
              << magic_resist.min_lvl
              << magic_resist.base
              << magic_resist.increment
              << magic_resist.deviation
              << table_wrapper::kEndRow;
    }
    if (has_physical_resist) {
        table << "PhysicResist"
              << physical_resist.min_lvl
              << physical_resist.base
              << physical_resist.increment
              << physical_resist.deviation
              << table_wrapper::kEndRow;
    }
    if (has_affect_resist) {
        table << "AffectResist"
              << affect_resist.min_lvl
              << affect_resist.base
              << affect_resist.increment
              << affect_resist.deviation
              << table_wrapper::kEndRow;
    }

    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}



void MobClassInfo::PrintCombatStatsTable(CharData *ch, std::ostringstream &buffer) const {
    if (!has_armour
        && !has_absorb
        && !has_dam_n_dice
        && !has_dam_s_dice
        && !has_hitroll
        && !has_morale
        && !has_cast_success
        && !has_hit_points
        && !has_size
        && !has_exp
        && !has_likes_work
        && !has_phys_damage
        && !has_spell_power) {
        return;
        }

    buffer << "\r\n" << kColorGrn << " Combat stats:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Param" << "MinLvl" << "Base" << "Increment" << "Deviation" << table_wrapper::kEndRow;

    if (has_armour) {
        table << "Armour"
              << armour.min_lvl
              << armour.base
              << armour.increment
              << armour.deviation
              << table_wrapper::kEndRow;
    }

    if (has_absorb) {
        table << "Absorb"
              << absorb.min_lvl
              << absorb.base
              << absorb.increment
              << absorb.deviation
              << table_wrapper::kEndRow;
    }

    if (has_dam_n_dice) {
        table << "DamNoDice"
              << dam_n_dice.min_lvl
              << dam_n_dice.base
              << dam_n_dice.increment
              << dam_n_dice.deviation
              << table_wrapper::kEndRow;
    }

    if (has_dam_s_dice) {
        table << "DamSizeDice"
              << dam_s_dice.min_lvl
              << dam_s_dice.base
              << dam_s_dice.increment
              << dam_s_dice.deviation
              << table_wrapper::kEndRow;
    }

    if (has_hitroll) {
        table << "Hitroll"
              << hitroll.min_lvl
              << hitroll.base
              << hitroll.increment
              << hitroll.deviation
              << table_wrapper::kEndRow;
    }

    if (has_morale) {
        table << "Morale"
              << morale.min_lvl
              << morale.base
              << morale.increment
              << morale.deviation
              << table_wrapper::kEndRow;
    }

    if (has_cast_success) {
        table << "CastSuccess"
              << cast_success.min_lvl
              << cast_success.base
              << cast_success.increment
              << cast_success.deviation
              << table_wrapper::kEndRow;
    }

    if (has_hit_points) {
        table << "HitPoints"
              << hit_points.min_lvl
              << hit_points.base
              << hit_points.increment
              << hit_points.deviation
              << table_wrapper::kEndRow;
    }

    if (has_size) {
        table << "Size"
              << size.min_lvl
              << size.base
              << size.increment
              << size.deviation
              << table_wrapper::kEndRow;
    }

    if (has_exp) {
        table << "Exp"
              << exp.min_lvl
              << exp.base
              << exp.increment
              << exp.deviation
              << table_wrapper::kEndRow;
    }

    if (has_likes_work) {
        table << "LikesWork"
              << likes_work.min_lvl
              << likes_work.base
              << likes_work.increment
              << likes_work.deviation
              << table_wrapper::kEndRow;
    }

    if (has_phys_damage) {
        table << "+PhysDamage%"
              << phys_damage.min_lvl
              << phys_damage.base
              << phys_damage.increment
              << phys_damage.deviation
              << table_wrapper::kEndRow;
    }

    if (has_spell_power) {
        table << "+SpellPower%"
              << spell_power.min_lvl
              << spell_power.base
              << spell_power.increment
              << spell_power.deviation
              << table_wrapper::kEndRow;
    }

    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}


void MobClassInfo::PrintMobSkillsTable(CharData *ch, std::ostringstream &buffer) const {
    buffer << "\r\n" << kColorGrn << " Skills:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Skill" << "MinLvl" << "Base" << "Increment" << "Deviation" << "Low-Lvl-Increment" << table_wrapper::kEndRow;
    for (const auto &mob_skill : mob_skills_map) {
        table << NAME_BY_ITEM<ESkill>(mob_skill.first)
              << mob_skill.second.min_lvl
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
    if (mob_spells_map.empty()) {
        return;
    }

    buffer << "\r\n" << kColorGrn << " Spells:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Spell" << "MinLvl" << "Charges" << "Increment" << "Deviation" << table_wrapper::kEndRow;

    for (const auto &spell_pair : mob_spells_map) {
        const auto &data = spell_pair.second;
        table << NAME_BY_ITEM<ESpell>(spell_pair.first)
              << data.min_lvl
              << data.base
              << data.increment
              << data.deviation
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
    PrintBaseStatsTable(ch, buffer);
    PrintSavingsTable(ch, buffer);
    PrintResistancesTable(ch, buffer);
    PrintCombatStatsTable(ch, buffer);
    PrintMobSkillsTable(ch, buffer);
    PrintMobSpellsTable(ch, buffer);
}

} // namespace mob_classes
