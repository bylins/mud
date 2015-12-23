#ifndef __CRAFT_HPP__
#define __CRAFT_HPP__

#include <set>
#include <map>
#include <list>
#include <string>

namespace craft
{
    typedef int id_t;                           ///< Common type for IDs.
    
    /**
     * Describes single receipt.
     */
    class CReceipt
    {
        private:
            id_t m_id;                          ///< Receipt ID.
    };

    /**
     * Describes crafting skill.
     */
    class CSkillBase
    {
        private:
            id_t m_id;                          ///< Skill ID.
            std::string m_name;                 ///< Skill Name.
            std::set<id_t> m_crafts;            ///< Set of crafts which this skill belongs to.
            int m_threshold;                    ///< Threshold to increase skill.
    };

    /**
     * Describes single craft model.
     */
    class CCraft
    {
        private:
            id_t m_id;                              ///< Craft ID.
            std::string m_name;                     ///< Craft name.
            std::set<id_t> m_skills;                ///< List of required skills for this craft.
            int m_slots;                            ///< Number of slots for additional skills.
            std::set<id_t> m_receipts;              ///< List of available receipts for this craft.
            std::set<id_t> m_material;              ///< List of meterials used by this craft.
    };

    /**
     * Class to manage and store crafting model.
     */
    class CCraftModel
    {
        public:
            /**
             * Loads data from XML data files.
             */
            bool load();

        private:
            std::list<CCraft> m_crafts;         ///< List of crafts defined for the game.
            std::list<CSkillBase> m_skills;     ///< List of skills defined for the game.
            std::list<CReceipt> m_receipts;     ///< List of receipts defined for the game.
    };
}

#endif // __CRAFT_HPP__

/* vim: set ts=4 sw=4 tw=0 et syntax=cpp :*/
