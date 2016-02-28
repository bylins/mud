/**
 * \file Contains definition of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#ifndef __CRAFT_HPP__
#define __CRAFT_HPP__

#include "structs.h"
#include "sysdep.h"

#include <stdarg.h>

#include <set>
#include <map>
#include <list>
#include <array>
#include <string>

namespace pugi
{
	class xml_node;
}

namespace craft
{
	/**
	** \brief Loads Craft model.
	**/
	bool load();

	// Subcommands for the "craft" command
	const int SCMD_NOTHING = 0;

	/// Defines for the "craft" command (base craft command)
	namespace base_cmd
	{
		/// Minimal position for base craft command
		const int MINIMAL_POSITION = POS_SITTING;

		// Minimal level for base craft command
		const int MINIMAL_LEVEL = 0;

		// Probability to stop hide when using base craft command
		const int UNHIDE_PROBABILITY = 0;	// -1 - always, 0 - never
	}

	extern void do_craft(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

	class CLogger
	{
	public:
		class CPrefix
		{
		public:
			CPrefix(CLogger& logger, const char* prefix) : m_logger(logger) { logger.push_prefix(prefix); }
			~CPrefix() { m_logger.pop_prefix(); }
			void change_prefix(const char* new_prefix) { m_logger.change_prefix(new_prefix); }

		private:
			CLogger& m_logger;
		};

		void operator()(const char* format, ...) __attribute__((format(printf, 2, 3)));

	private:
		void push_prefix(const char* prefix) { m_prefix.push_back(prefix); }
		void change_prefix(const char* new_prefix);
		void pop_prefix();

		std::list<std::string> m_prefix;
	};

	inline void CLogger::change_prefix(const char* new_prefix)
	{
		if (!m_prefix.empty())
		{
			m_prefix.back() = new_prefix;
		}
	}

	inline void CLogger::pop_prefix()
	{
		if (!m_prefix.empty())
		{
			m_prefix.pop_back();
		}
	}

	extern CLogger log;

	typedef std::string id_t;	///< Common type for IDs.
	typedef int vnum_t;			///< Common type for VNUM. Actually this type should be declared at the project level.

	class CCases
	{
		const static int CASES_COUNT = 6;

		typedef std::array<std::string, CASES_COUNT> cases_t;

	public:
		CCases() {}

		bool load(const pugi::xml_node* node);

	private:
		cases_t m_cases;
		std::list<std::string> m_aliases;

		friend class CMaterialClass;
		friend class CPrototype;
	};

	class CPrototype
	{
	public:
		CPrototype(const vnum_t vnum): m_vnum(vnum) {}

		bool load(const pugi::xml_node* node);

	private:
		vnum_t m_vnum;

		std::string m_short_desc;
		std::string m_long_desc;
		std::string m_keyword;
		std::string m_extended_desc;

		CCases m_cases;

		friend class CCraftModel;
	};

	/**
	 * Describes one material class.
	 */
	class CMaterialClass
	{
		public:
			CMaterialClass(const std::string& id): m_id(id) {}

		private:
			bool load(const pugi::xml_node* node);

			const std::string m_id;
			std::string m_short_desc;	///< Short description
			std::string m_long_desc;	///<Long description

			CCases m_item_cases;		///< Item cases (including aliases)
			CCases m_name_cases;		///< Name cases (including aliases)
			CCases m_male_adjectives;	///< Male adjective cases
			CCases m_female_adjectives;	///< Female adjective cases
			CCases m_neuter_adjectives;	///< Neuter adjective cases

			FLAG_DATA m_extraflags;
			FLAG_DATA m_affect_flags;

			friend class CMaterial;
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

			/**
			** For debug purposes. Should be removed when feature will be done.
			*/
			void create_item() const;

		private:
			/**
			** Loads N-th prototype from specified XML node and returns true
			** if it was successful and false otherwise.
			*/
			bool load_prototype(const pugi::xml_node* prototype, const size_t number);

			/**
			** Loads N-th material from specified XML node and returns true
			** if it was successful and false otherwise.
			*/
			bool load_material(const pugi::xml_node* material, const size_t number);

			std::list<CCraft> m_crafts;         ///< List of crafts defined for the game.
			std::list<CSkillBase> m_skills;     ///< List of skills defined for the game.
			std::list<CRecipe> m_recipes;     	///< List of recipes defined for the game.
			std::list<CPrototype> m_prototypes;	///< List of objects defined by the craft system.

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
