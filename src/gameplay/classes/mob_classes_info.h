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

EMobClass FindAvailableMobClassId(const CharData *ch, const std::string &mob_class_name);

namespace mob_classes {

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
        float increment{0.f};
        int   deviation{0};
        float low_increment{0.f};
    };

    int low_skill_lvl{0};

    std::unordered_map<ESaving, ParametersData>  savings_map;
    void PrintSavingsTable(CharData *ch, std::ostringstream &buffer) const;

    std::unordered_map<EResist, ParametersData>  resists_map;
    void PrintResistancesTable(CharData *ch, std::ostringstream &buffer) const;

    std::unordered_map<ESkill, ParametersData>   mob_skills_map;
    void PrintMobSkillsTable(CharData *ch, std::ostringstream &buffer) const;

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
};

class MobClassInfoBuilder : public info_container::IItemBuilder<MobClassInfo> {
public:
    ItemPtr Build(parser_wrapper::DataNode &node) final;

    // дефолт из <mob_classes><global_vars .../>
    static void SetDefaultLowSkillLvl(int v);

private:
    static ItemPtr ParseMobClass(parser_wrapper::DataNode node);
    static ItemPtr ParseHeader(parser_wrapper::DataNode &node);
    static void ParseName(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseSavingsData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseSavings(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseResistanceData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseResistances(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobSkillsData(ItemPtr &info, parser_wrapper::DataNode &node);
    static void ParseMobSkills(ItemPtr &info, parser_wrapper::DataNode &node);

    static int s_default_low_skill_lvl;
};

using MobClassesInfo = info_container::InfoContainer<EMobClass, MobClassInfo, MobClassInfoBuilder>;

}
#endif //BYLINS_MOB_CLASSES_INFO_H