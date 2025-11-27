/**
* \file mob_classes_info.h - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 25.11.2025.
 * \brief Brief description.
 * \details Detailed description.
 */

#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <random>
#include <cmath>

#include "gameplay/classes/mob_classes_info.h"
#include "engine/db/global_objects.h"
#include "classes_constants.h"

class CharData;

namespace mob_classes {

class MobParamsSetter {
public:
    struct Result {
        std::unordered_map<ESaving, int>  savings;
        std::unordered_map<EResist, int>  resists;
        std::unordered_map<ESkill,  int>  skills;
        int level{1};
        std::vector<EMobClass> roles;
        bool applied{false};
    };

    explicit MobParamsSetter(CharData& mob) : ch_(mob) {}

    MobParamsSetter& UseDeviation(bool v) { use_deviation_ = v; return *this; }
    MobParamsSetter& UseFallbackIfNoRoles(bool v) { use_fallback_ = v; return *this; }
    MobParamsSetter& WithFallbackRole(EMobClass r) { fallback_role_ = r; return *this; }

    MobParamsSetter& WithIsMob   (std::function<bool(const CharData*)> f) { is_mob_ = std::move(f); return *this; }
    MobParamsSetter& WithGetLevel(std::function<int (const CharData*)> f) { get_level_ = std::move(f); return *this; }

    using RolesEnumerator = std::function<std::vector<EMobClass>(const CharData*)>;
    MobParamsSetter& WithEnumerateRoles(RolesEnumerator f) { enum_roles_ = std::move(f); return *this; }

    using CombineSaving = std::function<int(ESaving, int current, int candidate)>;
    using CombineResist = std::function<int(EResist, int current, int candidate)>;
    using CombineSkill  = std::function<int(ESkill,  int current, int candidate)>;
    MobParamsSetter& WithCombineSaving(CombineSaving f) { comb_sav_ = std::move(f); return *this; }
    MobParamsSetter& WithCombineResist(CombineResist f) { comb_res_ = std::move(f); return *this; }
    MobParamsSetter& WithCombineSkill (CombineSkill  f) { comb_skl_ = std::move(f); return *this; }

    using SavingSetter = std::function<void(CharData*, ESaving, int)>;
    using ResistSetter = std::function<void(CharData*, EResist, int)>;
    using SkillSetter  = std::function<void(CharData*, ESkill,  int)>;
    MobParamsSetter& OnSetSaving(SavingSetter f) { set_saving_ = std::move(f); return *this; }
    MobParamsSetter& OnSetResist(ResistSetter f) { set_resist_ = std::move(f); return *this; }
    MobParamsSetter& OnSetSkill (SkillSetter  f) { set_skill_  = std::move(f); return *this; }

    Result Apply();

private:
    static int CalcValue(const MobClassInfo::ParametersData& p,
                         int level, int low_skill_lvl, bool apply_low_inc);
    static int ApplyDeviation(int v, int deviation, bool enabled);
    static const MobClassInfo* FindMobClassInfo(EMobClass id);

private:
    CharData& ch_;

    bool use_deviation_{true};
    bool use_fallback_{true};
    EMobClass fallback_role_{EMobClass::kUndefined};

    std::function<bool(const CharData*)> is_mob_ = [](const CharData* ch)->bool {
        return ch && ch->IsNpc();
    };
    std::function<int(const CharData*)> get_level_ = [](const CharData* ch)->int {
        return ch ? GetRealLevel(ch) : 1;
    };

    RolesEnumerator enum_roles_ = [](const CharData* ch)->std::vector<EMobClass> {
        std::vector<EMobClass> out;
        if (!ch) return out;
        const unsigned base = static_cast<unsigned>(EMobClass::kBoss);
        const unsigned total = static_cast<unsigned>(EMobClass::kTotal);
        for (unsigned u = base; u < total; ++u) {
            unsigned idx = u - base;
            if (ch->get_role(idx))
                out.push_back(static_cast<EMobClass>(u));
        }
        return out;
    };

    CombineSaving comb_sav_ = [](ESaving, int cur, int cand){ return std::max(cur, cand); };
    CombineResist comb_res_ = [](EResist, int cur, int cand){ return std::max(cur, cand); };
    CombineSkill  comb_skl_ = [](ESkill,  int cur, int cand){ return std::max(cur, cand); };

    SavingSetter set_saving_ = nullptr;
    ResistSetter set_resist_ = nullptr;
    SkillSetter  set_skill_  = nullptr;
};

// Удобный хелпер: принимает CharData*, логирует nullptr и вызывает Apply()
MobParamsSetter::Result ApplyMobParams(
    CharData* mob,
    MobParamsSetter::SavingSetter setSaving = nullptr,
    MobParamsSetter::ResistSetter setResist = nullptr,
    MobParamsSetter::SkillSetter  setSkill  = nullptr
);

} // namespace mob_classes
