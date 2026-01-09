/**
* \file mob_classes_info.h - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 25.11.2025.
 * \brief Brief description.
 * \details Detailed description.
 */

#ifndef BYLINS_MOB_CLASSES_INFO_H
#define BYLINS_MOB_CLASSES_INFO_H

#include "engine/boot/cfg_manager.h"
#include "classes_constants.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/magic/spells.h"
#include "gameplay/skills/skills.h"
#include "engine/structs/info_container.h"
#include "utils/grammar/cases.h"

enum class EBaseStat;

EMobClass FindAvailableMobClassId(const CharData *ch, const std::string &mob_class_name);

namespace mob_classes {

// Глобальные параметры перерасчёта сложности/уровня мобов, читаются из mob_classes.xml
int GetLvlPerDifficulty();
int GetBossAddLvl();

class MobClassesLoader : virtual public cfg_manager::ICfgLoader {
public:
    void Load(parser_wrapper::DataNode data) final;
    void Reload(parser_wrapper::DataNode data) final;
private:
    void ParseGlobalVarsFromRoot(parser_wrapper::DataNode &data);
};

class MobClassInfo : public info_container::BaseItem<EMobClass> {
public:
    MobClassInfo() = default;
    MobClassInfo(EMobClass id, EItemMode mode) : BaseItem<EMobClass>(id, mode) {}

    std::string name{"!undefined!"};
    std::string eng_name{"!error"};

    [[nodiscard]] const char *GetName() const { return name.c_str(); }
    [[nodiscard]] const char *GetAbbr() const { return eng_name.c_str(); }
    void Print(CharData *ch, std::ostringstream &buffer) const;

    struct ParametersData {
        int   base{0};
        float low_increment{0.f};
        int   threshold_mort{1};
        float increment{0.f};
        // это показывает какой тип отклонения использовать
        // 0 - прибавление/вычитание, 1 - умножение
        int deviation_type{0};
        // на сколько отклоняемся
        float deviation{0.f};
    };

    // Base stats (Str/Dex/Con/Wis/Int/Cha)
    std::unordered_map<EBaseStat, ParametersData> base_stats_map;
    void PrintBaseStatsTable(CharData *ch, std::ostringstream &buffer) const;

    std::unordered_map<ESaving, ParametersData>  savings_map;
    void PrintSavingsTable(CharData *ch, std::ostringstream &buffer) const;

    std::unordered_map<EResist, ParametersData>  resists_map;
    void PrintResistancesTable(CharData *ch, std::ostringstream &buffer) const;

    // Combat stats (armour / absorb)
    ParametersData armour;
    ParametersData absorb;
    bool has_armour{false};
    bool has_absorb{false};

    // Damage dice (ndd / sdd)
    ParametersData dam_n_dice;
    ParametersData dam_s_dice;
    bool has_dam_n_dice{false};
    bool has_dam_s_dice{false};

    // Hitroll / luck(morale) / cast success
    ParametersData hitroll;
    ParametersData morale;
    ParametersData initiative;
    ParametersData cast_success;
    bool has_hitroll{false};
    bool has_morale{false};
    bool has_initiative{false};
    bool has_cast_success{false};


    // HP / size / exp / likes_work
    ParametersData hit_points;
    ParametersData size;
    ParametersData exp;
    ParametersData likes_work;
    bool has_hit_points{false};
    bool has_size{false};
    bool has_exp{false};
    bool has_likes_work{false};

    // Percent damage bonuses
    ParametersData phys_damage;
    ParametersData spell_power;
    bool has_phys_damage{false};
    bool has_spell_power{false};

    void PrintCombatStatsTable(CharData *ch, std::ostringstream &buffer) const;

    // Extra resists: MR / PR / AR
    ParametersData magic_resist;
    ParametersData physical_resist;
    ParametersData affect_resist;
    bool has_magic_resist{false};
    bool has_physical_resist{false};
    bool has_affect_resist{false};

    std::unordered_map<ESkill, ParametersData>   mob_skills_map;
    void PrintMobSkillsTable(CharData *ch, std::ostringstream &buffer) const;

    std::unordered_map<ESpell, ParametersData> mob_spells_map;
    void PrintMobSpellsTable(CharData *ch, std::ostringstream &buffer) const;
    inline bool HasMobSpell(ESpell id) const {
        return mob_spells_map.find(id) != mob_spells_map.end();
    }

    // Feats (binary: has / not has)
    std::unordered_map<EFeat, int> mob_feats_map;
    void PrintMobFeatsTable(CharData *ch, std::ostringstream &buffer) const;

    auto GetBaseStatBase(EBaseStat id) const { return base_stats_map.at(id).base; }
    auto GetBaseStatIncrement(EBaseStat id) const { return base_stats_map.at(id).increment; }
    auto GetBaseStatDeviation(EBaseStat id) const { return base_stats_map.at(id).deviation; }

    auto GetSavingBase(ESaving id) const { return savings_map.at(id).base; }
    auto GetSavingIncrement(ESaving id) const { return savings_map.at(id).increment; }
    auto GetSavingDeviation(ESaving id) const { return savings_map.at(id).deviation; }

    auto GetResistBase(EResist id) const { return resists_map.at(id).base; }
    auto GetResistIncrement(EResist id) const { return resists_map.at(id).increment; }
    auto GetResistDeviation(EResist id) const { return resists_map.at(id).deviation; }

    auto GetMobSkillBase(ESkill id) const { return mob_skills_map.at(id).base; }
    auto GetMobSkillIncrement(ESkill id) const { return mob_skills_map.at(id).increment; }
    auto GetMobSkillDeviation(ESkill id) const { return mob_skills_map.at(id).deviation; }
    auto GetMobLowSkillIncrement(ESkill id) const { return mob_skills_map.at(id).low_increment; }

    auto GetMobSpellBase(ESpell id) const { return mob_spells_map.at(id).base; }
    auto GetMobSpellIncrement(ESpell id) const { return mob_spells_map.at(id).increment; }
    auto GetMobSpellDeviation(ESpell id) const { return mob_spells_map.at(id).deviation; }

    inline bool IsFeatConfigured(EFeat feat_id) const {
        return mob_feats_map.find(feat_id) != mob_feats_map.end();
    }


};

class MobClassInfoBuilder : public info_container::IItemBuilder<MobClassInfo> {
public:
    ItemPtr Build(parser_wrapper::DataNode &node) final;

private:
    static ItemPtr ParseMobClass(parser_wrapper::DataNode node);
    static ItemPtr ParseHeader(parser_wrapper::DataNode &node);
    static void ParseName(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseBaseStatsData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseBaseStats(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseSavingsData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseSavings(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseResistanceData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseResistances(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseCombatStatsData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseCombatStats(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobSkillsData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobSkills(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobSpells(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobFeats(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobFeatsData(ItemPtr &info, parser_wrapper::DataNode &node);
    };

using MobClassesInfo = info_container::InfoContainer<EMobClass, MobClassInfo, MobClassInfoBuilder>;

} // namespace mob_classes

#endif //BYLINS_MOB_CLASSES_INFO_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :