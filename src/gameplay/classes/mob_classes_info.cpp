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
            if (CompareParam(mob_class_name, it.GetName()) || CompareParam(mob_class_name, it.GetAbbr())) {
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

namespace mob_classes {

using DataNode = parser_wrapper::DataNode;
using MobItemPtr = MobClassInfoBuilder::ItemPtr;

namespace {

    // Сколько уровней добавляет 1 единица difficulty
    int g_lvl_per_difficulty = 0;

    // Сколько уровней дополнительно получает моб с ролью kBoss
    int g_boss_add_lvl = 0;

} // namespace

int GetLvlPerDifficulty() {
    return g_lvl_per_difficulty;
}

int GetBossAddLvl() {
    return g_boss_add_lvl;
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

    // Читаем атрибуты global_vars, если их нет/битые ? оставляем текущие значения
    try {
        g_lvl_per_difficulty = parse::ReadAsInt(data.GetValue("lvl_per_difficulty"));
    } catch (...) {
        err_log("mob_classes.xml: некорректный lvl_per_difficulty, оставляю прежнее значение (%d).",
                g_lvl_per_difficulty);
    }

    try {
        g_boss_add_lvl = parse::ReadAsInt(data.GetValue("boss_add_lvl"));
    } catch (...) {
        err_log("mob_classes.xml: некорректный boss_add_lvl, оставляю прежнее значение (%d).",
                g_boss_add_lvl);
    }

    if (g_lvl_per_difficulty < 0) g_lvl_per_difficulty = 0;
    if (g_boss_add_lvl < 0) g_boss_add_lvl = 0;

    data.GoToParent();
}

// ---------- Builder ----------

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
    ParseMobFeats(info, node);
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
            stat_data.low_increment = parse::ReadAsFloat(stat_node.GetValue("low_increment"));
            stat_data.threshold_mort = parse::ReadAsInt(stat_node.GetValue("threshold_mort"));
            stat_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            stat_data.deviation_type = parse::ReadAsInt(stat_node.GetValue("deviation_type"));
            stat_data.deviation = parse::ReadAsFloat(stat_node.GetValue("deviation"));
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
            saving_data.low_increment = parse::ReadAsFloat(stat_node.GetValue("low_increment"));
            saving_data.threshold_mort = parse::ReadAsInt(stat_node.GetValue("threshold_mort"));
            saving_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            saving_data.deviation_type = parse::ReadAsInt(stat_node.GetValue("deviation_type"));
            saving_data.deviation = parse::ReadAsFloat(stat_node.GetValue("deviation"));
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
            resistance_data.low_increment = parse::ReadAsFloat(stat_node.GetValue("low_increment"));
            resistance_data.threshold_mort = parse::ReadAsInt(stat_node.GetValue("threshold_mort"));
            resistance_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            resistance_data.deviation_type = parse::ReadAsInt(stat_node.GetValue("deviation_type"));
            resistance_data.deviation = parse::ReadAsFloat(stat_node.GetValue("deviation"));
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



// ----- Combat stats -----
void MobClassInfoBuilder::ParseCombatStats(ItemPtr &info, DataNode &node) {
    if (!node.GoToChild("combat_stats")) {
        return;
    }

    ParseCombatStatsData(info, node);
    node.GoToParent();
}

void MobClassInfoBuilder::ParseCombatStatsData(ItemPtr &info, DataNode &node) {
    MobClassInfo::ParametersData data;

    for (const auto &stat_node : node.Children("param")) {
        const char *id_cstr = stat_node.GetValue("id");
        const std::string id_str{id_cstr};

        try {
            data.base = parse::ReadAsInt(stat_node.GetValue("base"));
            data.low_increment = parse::ReadAsFloat(stat_node.GetValue("low_increment"));
            data.threshold_mort = parse::ReadAsInt(stat_node.GetValue("threshold_mort"));
            data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            data.deviation_type = parse::ReadAsInt(stat_node.GetValue("deviation_type"));
            data.deviation = parse::ReadAsFloat(stat_node.GetValue("deviation"));
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
        } else if (id_str == "kDamroll") {
            info->damroll = data;
            info->has_damroll = true;
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
        } else if (id_str == "kInitiative") {
            info->initiative = data;
            info->has_initiative = true;
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
            mob_skills_data.low_increment = parse::ReadAsFloat(stat_node.GetValue("low_increment"));
            mob_skills_data.threshold_mort = parse::ReadAsInt(stat_node.GetValue("threshold_mort"));
            mob_skills_data.increment = parse::ReadAsFloat(stat_node.GetValue("increment"));
            mob_skills_data.deviation_type = parse::ReadAsInt(stat_node.GetValue("deviation_type"));
            mob_skills_data.deviation = parse::ReadAsFloat(stat_node.GetValue("deviation"));
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
            spell_data.low_increment = parse::ReadAsFloat(spell_node.GetValue("low_increment"));
            spell_data.threshold_mort = parse::ReadAsInt(spell_node.GetValue("threshold_mort"));
            spell_data.increment = parse::ReadAsFloat(spell_node.GetValue("increment"));
            spell_data.deviation_type = parse::ReadAsInt(spell_node.GetValue("deviation_type"));
            spell_data.deviation = parse::ReadAsFloat(spell_node.GetValue("deviation"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect spell format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        info->mob_spells_map[id] = spell_data;
    }

    node.GoToParent();
}

void MobClassInfoBuilder::ParseMobFeats(ItemPtr &info, DataNode &node) {
    if (!node.GoToChild("feats")) {
        return;
    }
    ParseMobFeatsData(info, node);
    node.GoToParent();
}

    void MobClassInfoBuilder::ParseMobFeatsData(ItemPtr &info, DataNode &node) {
    for (const auto &feat_node : node.Children("feat")) {
        EFeat feat_id{};
        int threshold_mort = 1;

        try {
            feat_id = parse::ReadAsConstant<EFeat>(feat_node.GetValue("id"));
            threshold_mort = parse::ReadAsInt(feat_node.GetValue("threshold_mort"));
        } catch (const std::exception &e) {
            std::ostringstream out;
            out << "Incorrect feat format (wrong value: " << e.what() << ").";
            throw std::runtime_error(out.str());
        }

        if (threshold_mort < 1) {
            threshold_mort = 1;
        }

        info->mob_feats_map[feat_id] = threshold_mort;
    }
}


static const char *DeviationTypeToString(int deviation_type) {
    // 0 - additive, 1 - multiplicative (percent)
    return (deviation_type == 0) ? "add" : "mul%";
}

// ----- Print -----

void MobClassInfo::PrintBaseStatsTable(CharData *ch, std::ostringstream &buffer) const {
    if (base_stats_map.empty()) {
        return;
    }

    buffer << "\r\n" << kColorGrn << " Base stats:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Stat" << "Base" << "LowInc" << "IncThLvl" << "Inc" << "Grad" << "Deviation"
          << table_wrapper::kEndRow;


    for (const auto &stat_pair : base_stats_map) {
        const auto &data = stat_pair.second;
        table << NAME_BY_ITEM<EBaseStat>(stat_pair.first)
              << data.base
              << data.low_increment
              << data.threshold_mort
              << data.increment
              << DeviationTypeToString(data.deviation_type)
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
    << "Saving" << "Base" << "LowInc" << "IncThLvl" << "Inc" << "Grad" << "Deviation"
    << table_wrapper::kEndRow;
    for (const auto &saving : savings_map) {
        table << NAME_BY_ITEM<ESaving>(saving.first)
              << saving.second.base
              << saving.second.low_increment
              << saving.second.threshold_mort
              << saving.second.increment
              << DeviationTypeToString(saving.second.deviation_type)
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
          << "Resistance" << "Base" << "LowInc" << "IncThLvl" << "Inc" << "Grad" << "Deviation" << table_wrapper::kEndRow;

    // Обычные резисты
    for (const auto &resistance : resists_map) {
        table << NAME_BY_ITEM<EResist>(resistance.first)
              << resistance.second.base
              << resistance.second.low_increment
              << resistance.second.threshold_mort
              << resistance.second.increment
              << DeviationTypeToString(resistance.second.deviation_type)
              << resistance.second.deviation
              << table_wrapper::kEndRow;
    }

    // Доп. резисты
    if (has_magic_resist) {
        table << "MagicResist"
              << magic_resist.base
              << magic_resist.low_increment
              << magic_resist.threshold_mort
              << magic_resist.increment
              << DeviationTypeToString(magic_resist.deviation_type)
              << magic_resist.deviation
              << table_wrapper::kEndRow;
    }
    if (has_physical_resist) {
        table << "PhysicResist"
              << physical_resist.base
              << physical_resist.low_increment
              << physical_resist.threshold_mort
              << physical_resist.increment
              << DeviationTypeToString(physical_resist.deviation_type)
              << physical_resist.deviation
              << table_wrapper::kEndRow;
    }
    if (has_affect_resist) {
        table << "AffectResist"
              << affect_resist.base
              << affect_resist.low_increment
              << affect_resist.threshold_mort
              << affect_resist.increment
              << DeviationTypeToString(affect_resist.deviation_type)
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
        && !has_spell_power
        && !has_initiative) {
        return;
        }

    buffer << "\r\n" << kColorGrn << " Combat stats:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Param" << "Base" << "LowInc" << "IncThLvl" << "Inc" << "Grad" << "Deviation" << table_wrapper::kEndRow;

    if (has_armour) {
        table << "Armour"
              << armour.base
              << armour.low_increment
              << armour.threshold_mort
              << armour.increment
              << DeviationTypeToString(armour.deviation_type)
              << armour.deviation
              << table_wrapper::kEndRow;
    }

    if (has_absorb) {
        table << "Absorb"
              << absorb.base
              << absorb.low_increment
              << absorb.threshold_mort
              << absorb.increment
              << DeviationTypeToString(absorb.deviation_type)
              << absorb.deviation
              << table_wrapper::kEndRow;
    }

    if (has_dam_n_dice) {
        table << "DamNoDice"
              << dam_n_dice.base
              << dam_n_dice.low_increment
              << dam_n_dice.threshold_mort
              << dam_n_dice.increment
              << DeviationTypeToString(dam_n_dice.deviation_type)
              << dam_n_dice.deviation
              << table_wrapper::kEndRow;
    }

    if (has_dam_s_dice) {
        table << "DamSizeDice"
              << dam_s_dice.base
              << dam_s_dice.low_increment
              << dam_s_dice.threshold_mort
              << dam_s_dice.increment
              << DeviationTypeToString(dam_s_dice.deviation_type)
              << dam_s_dice.deviation
              << table_wrapper::kEndRow;
    }

    if (has_hitroll) {
        table << "Hitroll"
              << hitroll.base
              << hitroll.low_increment
              << hitroll.threshold_mort
              << hitroll.increment
              << DeviationTypeToString(hitroll.deviation_type)
              << hitroll.deviation
              << table_wrapper::kEndRow;
    }

    if (has_damroll) {
        table << "Damroll"
              << damroll.base
              << damroll.low_increment
              << damroll.threshold_mort
              << damroll.increment
              << DeviationTypeToString(damroll.deviation_type)
              << damroll.deviation
              << table_wrapper::kEndRow;
    }

    if (has_morale) {
        table << "Morale"
              << morale.base
              << morale.low_increment
              << morale.threshold_mort
              << morale.increment
              << DeviationTypeToString(morale.deviation_type)
              << morale.deviation
              << table_wrapper::kEndRow;
    }

    if (has_initiative) {
        table << "Initiative"
              << initiative.base
              << initiative.low_increment
              << initiative.threshold_mort
              << initiative.increment
              << DeviationTypeToString(initiative.deviation_type)
              << initiative.deviation
              << table_wrapper::kEndRow;
    }

    if (has_cast_success) {
        table << "CastSuccess"
              << cast_success.base
              << cast_success.low_increment
              << cast_success.threshold_mort
              << cast_success.increment
              << DeviationTypeToString(cast_success.deviation_type)
              << cast_success.deviation
              << table_wrapper::kEndRow;
    }

    if (has_hit_points) {
        table << "HitPoints"
              << hit_points.base
              << hit_points.low_increment
              << hit_points.threshold_mort
              << hit_points.increment
              << DeviationTypeToString(hit_points.deviation_type)
              << hit_points.deviation
              << table_wrapper::kEndRow;
    }

    if (has_size) {
        table << "Size"
              << size.base
              << size.low_increment
              << size.threshold_mort
              << size.increment
              << DeviationTypeToString(size.deviation_type)
              << size.deviation
              << table_wrapper::kEndRow;
    }

    if (has_exp) {
        table << "Exp"
              << exp.base
              << exp.low_increment
              << exp.threshold_mort
              << exp.increment
              << DeviationTypeToString(exp.deviation_type)
              << exp.deviation
              << table_wrapper::kEndRow;
    }

    if (has_likes_work) {
        table << "LikesWork"
              << likes_work.base
              << likes_work.low_increment
              << likes_work.threshold_mort
              << likes_work.increment
              << DeviationTypeToString(likes_work.deviation_type)
              << likes_work.deviation
              << table_wrapper::kEndRow;
    }

    if (has_phys_damage) {
        table << "+PhysDamage%"
              << phys_damage.base
              << phys_damage.low_increment
              << phys_damage.threshold_mort
              << phys_damage.increment
              << DeviationTypeToString(phys_damage.deviation_type)
              << phys_damage.deviation
              << table_wrapper::kEndRow;
    }

    if (has_spell_power) {
        table << "+SpellPower%"
              << spell_power.base
              << spell_power.low_increment
              << spell_power.threshold_mort
              << spell_power.increment
              << DeviationTypeToString(spell_power.deviation_type)
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
          << "Skill" << "Base" << "LowInc" << "IncThLvl" << "Inc" << "Grad" << "Deviation" << table_wrapper::kEndRow;
    for (const auto &mob_skill : mob_skills_map) {
        table << NAME_BY_ITEM<ESkill>(mob_skill.first)
              << mob_skill.second.base
              << mob_skill.second.low_increment
              << mob_skill.second.threshold_mort
              << mob_skill.second.increment
              << DeviationTypeToString(mob_skill.second.deviation_type)
              << mob_skill.second.deviation
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
          << "Spell" << "Charges" << "LowInc" << "IncThLvl" << "Inc" << "Grad" << "Deviation" << table_wrapper::kEndRow;

    for (const auto &spell_pair : mob_spells_map) {
        const auto &data = spell_pair.second;
        table << NAME_BY_ITEM<ESpell>(spell_pair.first)
              << data.base
              << data.low_increment
              << data.threshold_mort
              << data.increment
              << DeviationTypeToString(data.deviation_type)
              << data.deviation
              << table_wrapper::kEndRow;
    }

    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}

void MobClassInfo::PrintMobFeatsTable(CharData *ch, std::ostringstream &buffer) const {
    if (mob_feats_map.empty()) {
        return;
    }

    buffer << "\r\n" << kColorGrn << " Feats:" << kColorNrm << "\r\n";

    table_wrapper::Table table;
    table << table_wrapper::kHeader
          << "Feat" << "ThresholdMort"
          << table_wrapper::kEndRow;

    for (const auto &feat_pair : mob_feats_map) {
        table << NAME_BY_ITEM<EFeat>(feat_pair.first)
              << feat_pair.second
              << table_wrapper::kEndRow;
    }

    table_wrapper::DecorateNoBorderTable(ch, table);
    table_wrapper::PrintTableToStream(buffer, table);
}


void MobClassInfo::Print(CharData *ch, std::ostringstream &buffer) const {
    buffer << "Print mob class:\r\n"
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
    PrintMobFeatsTable(ch, buffer);
}

} // namespace mob_classes

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :