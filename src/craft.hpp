/**
 * \file Contains definition of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#ifndef __CRAFT_HPP__
#define __CRAFT_HPP__

#include <stdarg.h>

#include <set>
#include <map>
#include <list>
#include <string>

namespace pugi
{
	class xml_node;
}

namespace craft
{
	bool load();
	class CLogger
	{
		public:
			void operator()(const char* format, ...) __attribute__((format(printf, 2, 3)));
	};

	extern CLogger log;

	typedef std::string id_t;						///< Common type for IDs.

	/**
	 * Describes one material class.
	 */
	class CMaterialClass
	{
		public:
			CMaterialClass(const std::string& name): m_name(name) {}

		private:
			const std::string m_name;
	};

	/**
	 * Defines material type.
	 */
	class CMaterial
	{
		public:
			CMaterial(const std::string& id): m_id(id) {}

		private:
			bool load(const pugi::xml_node* node);

			const id_t m_id;						///< Meterial ID.
			std::string m_name;						///< Material name.
			std::list<CMaterialClass> m_classes;	///< List of material classes for this material.

			friend class CCraftModel;
	};

	/**
	 * Describes single recipe.
	 */
	class CRecipe
	{
		private:
			bool load(const pugi::xml_node* node);

			id_t m_id;                          ///< Recipe ID.
			std::string m_name;					///< Recipe name.

			friend class CCraftModel;
	};

	/**
	 * Describes crafting skill.
	 */
	class CSkillBase
	{
		public:
			CSkillBase(): m_threshold(0) {}

		private:
			bool load(const pugi::xml_node* node);

			id_t m_id;                          ///< Skill ID.
			std::string m_name;                 ///< Skill Name.
			int m_threshold;                    ///< Threshold to increase skill.

			friend class CCraftModel;
	};

	/**
	 * Describes single craft model.
	 */
	class CCraft
	{
		public:
			CCraft(): m_slots(0) {}

		private:
			bool load(const pugi::xml_node* node);

			id_t m_id;                              ///< Craft ID.
			std::string m_name;                     ///< Craft name.
			std::set<id_t> m_skills;                ///< List of required skills for this craft.
			std::set<id_t> m_recipes;               ///< List of available recipes for this craft.
			std::set<id_t> m_material;              ///< List of meterials used by this craft.
			int m_slots;                            ///< Number of slots for additional skills.

			friend class CCraftModel;
	};

	/**
	 * Class to manage and store crafting model.
	 */
	class CCraftModel
	{
		public:
			typedef std::set<id_t> id_set_t;

			const static std::string FILE_NAME;

			/**
			 * Loads data from XML data files.
			 *
			 * \return true if all data loaded successfully.
			 *         false otherwise.
			 */
			bool load();

		private:
			std::list<CCraft> m_crafts;         ///< List of crafts defined for the game.
			std::list<CSkillBase> m_skills;     ///< List of skills defined for the game.
			std::list<CRecipe> m_recipes;     	///< List of recipes defined for the game.

			// Properties
			int m_base_count;						///< Base count of crafts (per character?).
			int m_remort_for_count_bonus;			///< Required remorts to take one more craft.
			unsigned char m_base_top;				///< Base top of skill level.
			unsigned char m_remorts_bonus;			///< Bonus to top skill level based on remorts count (how?)
			
			std::list<std::string> m_skill_files;		///< List of files with skill definitions.
			std::list<std::string> m_craft_files;		///< List of files with craft definitions.
			std::list<std::string> m_material_files;	///< List of files with material definitions.
			std::list<std::string> m_recipe_files;		///< List of files with recipe definitions.

			// Helpers
			std::map<id_t, id_set_t> m_skill2crafts;		///< Maps skill ID to set of crafts which this skill belongs to.
			std::map<id_t, const CCraft*> m_id2craft;		///< Maps craft ID to pointer to craft descriptor.
			std::map<id_t, const CSkillBase*> m_id2skill;	///< Maps skill ID to pointer to skill descriptor.
			std::map<id_t, const CRecipe*> m_id2recipe;		///< Maps recipe ID to pointer to recipe descriptor.
	};
}

#endif // __CRAFT_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
