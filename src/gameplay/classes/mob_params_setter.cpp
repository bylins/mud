/**
* \file mob_params_setter.cpp - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 25.11.2025.
 * \brief Brief description.
 * \details Detailed description.
 */

#include "mob_params_setter.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include "utils/utils.h"

namespace mob_classes {

namespace {
inline void SysLog(const char* fmt, ...) {
    char local_buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(local_buf, sizeof(local_buf), fmt, args);
    va_end(args);
    mudlog(local_buf, BRF, kLvlImmortal, SYSLOG, true);
}
inline int clamp_nonneg(int v) { return v < 0 ? 0 : v; }
} // namespace

int MobParamsSetter::CalcValue(const MobClassInfo::ParametersData& p,
                               int level,
                               int low_skill_lvl,
                               bool apply_low_inc) {
    if (level < 1) level = 1;
    const int steps = level - 1;
    const bool use_low = apply_low_inc && p.low_increment > 0.0f && level <= low_skill_lvl;
    const double inc = use_low ? static_cast<double>(p.low_increment)
                               : static_cast<double>(p.increment);
    const double raw = static_cast<double>(p.base) + inc * static_cast<double>(steps);
    return static_cast<int>(std::llround(raw));
}

int MobParamsSetter::ApplyDeviation(int v, int deviation, bool enabled) {
    if (!enabled || deviation <= 0) return v;
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(-deviation, deviation);
    return v + dist(rng);
}

const MobClassInfo* MobParamsSetter::FindMobClassInfo(EMobClass id) {
    for (const auto& it : MUD::MobClasses()) {
        if (it.GetId() == id && it.IsAvailable())
            return &it;
    }
    return nullptr;
}

MobParamsSetter::Result MobParamsSetter::Apply() {
    Result res{};

    // Проверяем, что это моб (без nullptr ? мы держим ссылку)
    if (!is_mob_(&ch_)) {
        SysLog("SYSINFO: MobParams skipped (not NPC).");
        return res;
    }

    res.level = std::max(1, get_level_(&ch_));
    res.roles = enum_roles_(&ch_);
    SysLog("SYSINFO: MobParams start (lvl=%d, roles=%zu).", res.level, res.roles.size());

    // Фолбэк, если ролей нет
    if (res.roles.empty() && use_fallback_) {
        if (const MobClassInfo* fb = FindMobClassInfo(fallback_role_)) {
            res.roles.push_back(fallback_role_);
            SysLog("SYSINFO: MobParams no roles, using fallback %s.",
                   NAME_BY_ITEM<EMobClass>(fallback_role_).c_str());
        } else {
            SysLog("SYSINFO: MobParams no roles, fallback %s not found.",
                   NAME_BY_ITEM<EMobClass>(fallback_role_).c_str());
        }
    }

    if (res.roles.empty()) {
        SysLog("SYSINFO: MobParams nothing to apply.");
        return res;
    }

    // Обработка всех ролей
    for (EMobClass role : res.roles) {
        const MobClassInfo* info = FindMobClassInfo(role);
        if (!info) {
            SysLog("SYSINFO: MobParams role %s not found/disabled.",
                   NAME_BY_ITEM<EMobClass>(role).c_str());
            continue;
        }
        SysLog("SYSINFO: MobParams applying role %s.", NAME_BY_ITEM<EMobClass>(role).c_str());

        // Savings
        for (const auto& kv : info->savings_map) {
            ESaving id = kv.first;
            const auto& p = kv.second;
            int v = CalcValue(p, res.level, info->low_skill_lvl, false);
            v = ApplyDeviation(v, p.deviation, use_deviation_);
            v = clamp_nonneg(v);
            auto it = res.savings.find(id);
            if (it == res.savings.end()) res.savings[id] = v;
            else it->second = comb_sav_(id, it->second, v);
            SysLog("SYSINFO: Saving %s = %d.", NAME_BY_ITEM<ESaving>(id).c_str(), v);
        }

        // Resists
        for (const auto& kv : info->resists_map) {
            EResist id = kv.first;
            const auto& p = kv.second;
            int v = CalcValue(p, res.level, info->low_skill_lvl, false);
            v = ApplyDeviation(v, p.deviation, use_deviation_);
            v = clamp_nonneg(v);
            auto it = res.resists.find(id);
            if (it == res.resists.end()) res.resists[id] = v;
            else it->second = comb_res_(id, it->second, v);
            SysLog("SYSINFO: Resist %s = %d.", NAME_BY_ITEM<EResist>(id).c_str(), v);
        }

        // Skills
        for (const auto& kv : info->mob_skills_map) {
            ESkill id = kv.first;
            const auto& p = kv.second;
            int v = CalcValue(p, res.level, info->low_skill_lvl, true);
            v = ApplyDeviation(v, p.deviation, use_deviation_);
            v = clamp_nonneg(v);
            auto it = res.skills.find(id);
            if (it == res.skills.end()) res.skills[id] = v;
            else it->second = comb_skl_(id, it->second, v);
            SysLog("SYSINFO: Skill %s = %d.", NAME_BY_ITEM<ESkill>(id).c_str(), v);
        }
    }

    // Проставление в движок
    if (set_saving_) for (auto& [id, val] : res.savings) set_saving_(&ch_, id, val);
    if (set_resist_) for (auto& [id, val] : res.resists) set_resist_(&ch_, id, val);
    if (set_skill_)  for (auto& [id, val] : res.skills)  set_skill_ (&ch_, id, val);

    res.applied = true;
    SysLog("SYSINFO: MobParams applied successfully (savings=%zu, resists=%zu, skills=%zu).",
           res.savings.size(), res.resists.size(), res.skills.size());
    return res;
}

// Свободный хелпер: сюда можно смело передавать CharData*, null обработаем тут.
MobParamsSetter::Result ApplyMobParams(
    CharData* mob,
    MobParamsSetter::SavingSetter setSaving,
    MobParamsSetter::ResistSetter setResist,
    MobParamsSetter::SkillSetter  setSkill
) {
    if (!mob) {
        SysLog("SYSINFO: MobParams skipped (null CharData passed to helper).");
        return {};
    }
    MobParamsSetter setter(*mob);
    if (setSaving) setter.OnSetSaving(std::move(setSaving));
    if (setResist) setter.OnSetResist(std::move(setResist));
    if (setSkill)  setter.OnSetSkill (std::move(setSkill));
    return setter.Apply();
}

} // namespace mob_classes
