/**
 * \file Contains definition of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#ifndef __CRAFT_HPP__
#define __CRAFT_HPP__

#include "utils.h"
#include "obj.hpp"
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
	bool start();

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

	using id_t = std::string;	///< Common type for IDs.

	class CCases
	{
		const static int CASES_COUNT = OBJ_DATA::NUM_PADS;

		using cases_t = std::array<std::string, CASES_COUNT>;

	public:
		CCases() {}

		bool load(const pugi::xml_node* node);

		const std::string& aliases() const { return m_joined_aliases; }
		OBJ_DATA::pnames_t build_pnames() const;

	private:
		cases_t m_cases;
		std::list<std::string> m_aliases;
		std::string m_joined_aliases;

		friend class CMaterialClass;
		friend class CPrototype;
	};

	class CPrototypeBase
	{
	public:
		using type_t = obj_flag_data::EObjectType;

		void set_type(const type_t _) { m_type = _; }
		type_t get_type() const { return m_type; }

		void set_weight(const int _) { m_weight = _; }
		auto get_weight() const { return m_weight; }

		void set_timer(const int _) { m_timer = _; }
		auto get_timer() const { return m_timer; }

	protected:
		CPrototypeBase():
			m_type(obj_flag_data::DEFAULT_TYPE),
			m_weight(obj_flag_data::DEFAULT_WEIGHT),
			m_timer(OBJ_DATA::DEFAULT_TIMER)
		{
		}

	private:
		type_t m_type;

		int m_weight;
		int m_timer;
	};

	class CPrototype: public CPrototypeBase
	{
	public:
		CPrototype(const obj_vnum vnum) :
			m_vnum(vnum),
			m_cost(OBJ_DATA::DEFAULT_COST),
			m_rent_on(OBJ_DATA::DEFAULT_RENT_ON),
			m_rent_off(OBJ_DATA::DEFAULT_RENT_OFF),
			m_global_maximum(OBJ_DATA::DEFAULT_GLOBAL_MAXIMUM),
			m_minimum_remorts(OBJ_DATA::DEFAULT_MINIMUM_REMORTS),
			m_maximum_durability(obj_flag_data::DEFAULT_MAXIMUM_DURABILITY),
			m_current_durability(obj_flag_data::DEFAULT_CURRENT_DURABILITY),
			m_sex(DEFAULT_SEX),
			m_level(obj_flag_data::DEFAULT_LEVEL),
			m_item_params(0),
			m_material(obj_flag_data::DEFAULT_MATERIAL),
			m_wear_flags(0),
			m_vals({0, 0, 0, 0})
		{
		}

		bool load(const pugi::xml_node* node);

		obj_vnum vnum() const { return m_vnum; }
		const std::string& short_desc() const { return m_short_desc; }

		/**
		 * Builds OBJ_DATA instance suitable for add it into the list of objects prototypes.
		 *
		 * Allocates memory for OBJ_DATA instance and fill this memory by appropriate values.
		 *
		 * \return Pointer to created instance.
		 */
		OBJ_DATA* build_object() const;

	private:
		constexpr static int VALS_COUNT = 4;

		typedef std::map<ESkill, int> skills_t;
		typedef std::array<int, VALS_COUNT> vals_t;

		bool load_item_parameters(const pugi::xml_node* node);
		void load_skills(const pugi::xml_node* node);
		bool check_prototype_consistency() const;

		const std::string& aliases() const { return m_cases.aliases(); }

		obj_vnum m_vnum;

		std::string m_short_desc;
		std::string m_long_desc;
		std::string m_action_desc;
		std::string m_keyword;
		std::string m_extended_desc;

		CCases m_cases;

		int m_cost;
		int m_rent_on;
		int m_rent_off;
		int m_global_maximum;
		int m_minimum_remorts;

		int m_maximum_durability;
		int m_current_durability;

		ESex m_sex;

		int m_level;

		uint32_t m_item_params;

		obj_flag_data::EObjectMaterial m_material;
		ESpell m_spell;

		FLAG_DATA m_extraflags;
		FLAG_DATA m_affect_flags;
		FLAG_DATA m_anti_flags;
		FLAG_DATA m_no_flags;

		std::underlying_type<EWearFlag>::type m_wear_flags;

		skills_t m_skills;

		vals_t m_vals;

		friend class CCraftModel;
	};

	/**
	 * Describes one material class.
	 */
	class CMaterialClass
	{
	public:
		CMaterialClass(const std::string& id) : m_id(id) {}

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
		CMaterial(const std::string& id) : m_id(id) {}

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
	public:
		const auto& id() const { return m_id; }

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
		CSkillBase() : m_threshold(0) {}

		const auto& id() const { return m_id; }

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
		CCraft() : m_slots(0) {}

		const auto& id() const { return m_id; }

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
		using id_set_t = std::set<id_t>;
		using crafts_t = std::list<CCraft>;
		using skills_t = std::list<CSkillBase>;
		using recipes_t = std::list<CRecipe>;
		using prototypes_t = std::list<CPrototype>;

		const static std::string FILE_NAME;
		constexpr static int DEFAULT_BASE_COUNT = 1;
		constexpr static int DEFAULT_REMORT_FOR_COUNT_BONUS = 1;
		constexpr static int DEFAULT_BASE_TOP = 75;
		constexpr static int DEFAULT_REMORTS_BONUS = 12;

		CCraftModel(): m_base_count(DEFAULT_BASE_COUNT),
			m_remort_for_count_bonus(DEFAULT_REMORT_FOR_COUNT_BONUS),
			m_base_top(DEFAULT_BASE_TOP),
			m_remorts_bonus(DEFAULT_REMORTS_BONUS)
		{
		}

		/**
		 * Loads data from XML data files.
		 *
		 * \return true if all data loaded successfully.
		 *         false otherwise.
		 */
		bool load();

		/**
		 * Merges loaded craft model into MUD world.
		 */
		bool merge();

		/**
		** For debug purposes. Should be removed when feature will be done.
		*/
		void create_item() const;

		const crafts_t& crafts() const { return m_crafts; }
		const skills_t& skills() const { return m_skills; }
		const recipes_t& recipes() const { return m_recipes; }
		const prototypes_t& prototypes() const { return m_prototypes; }

		const auto base_count() const { return m_base_count; }
		const auto remort_for_count_bonus() const { return m_remort_for_count_bonus; }
		const auto base_top() const { return m_base_top; }
		const auto remorts_bonus() const { return m_remorts_bonus; }

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

		crafts_t m_crafts;         ///< List of crafts defined for the game.
		skills_t m_skills;     ///< List of skills defined for the game.
		recipes_t m_recipes;     	///< List of recipes defined for the game.
		prototypes_t m_prototypes;	///< List of objects defined by the craft system.

		// Properties
		int m_base_count;						///< Base count of crafts (per character?).
		int m_remort_for_count_bonus;			///< Required remorts to take one more craft.
		int m_base_top;							///< Base top of skill level.
		int m_remorts_bonus;					///< Bonus to top skill level based on remorts count (how?)

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
