/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "parse.hpp"
#include "skills.h"
#include "comm.h"
#include "db.h"
#include "pugixml.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/detail/util.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <string>
#include <sstream>

namespace craft
{
	const char* suffix(const size_t number)
	{
		return 1 == number % 10
			? "st"
			: (2 == number % 10
				? "nd"
				: (3 == number % 10
					? "rd"
					: "th"));
	}

	CCraftModel model;

	bool start()
	{
		const bool load_result = model.load();
		if (!load_result)
		{
			log("ERROR: Failed to load craft model.\n");
			return false;
		}

		return model.merge();
	}

	class CHelper
	{
	private:
		template <typename EnumType>
		static void set_bit(FLAG_DATA& flags, const EnumType flag) { flags.set(flag); }
		template <typename EnumType>
		static void set_bit(uint32_t& flags, const EnumType flag) { SET_BIT(flags, flag); }

	public:
		enum ELoadFlagResult
		{
			ELFR_SUCCESS,
			ELFR_NO_VALUE,
			ELFR_FAIL
		};

		template <class TFlags, typename TSuccessHandler, typename TFailHandler, typename TFlagsStorage>
		static void load_flags(TFlagsStorage& flags, const pugi::xml_node& root, const char* node_name, const char* node_flag,
			TSuccessHandler success_handler, TFailHandler fail_handler);

		template <class TFlag, typename TSuccessHandler, typename TFailHandler, typename TNoValueHandler>
		static ELoadFlagResult load_flag(const pugi::xml_node& root, const char* node_name,
			TSuccessHandler success_handler, TFailHandler fail_handler, TNoValueHandler no_value_handler);

		template <typename TCatchHandler>
		static void load_integer(const char* input, int& output, TCatchHandler catch_handler);

		template <typename TSetHandler, typename TCatchHandler>
		static void load_integer(const char* input, TSetHandler set_handler, TCatchHandler catch_handler);

		template <typename KeyType,
			typename TNoKeyHandler,
			typename TKeyConverter,
			typename TConverFailHandler,
			typename TNoValueHandler,
			typename TSetHandler>
		static void load_pairs_list(
			const pugi::xml_node* node,
			const char* group_name,
			const char* entry_name,
			const char* key_name,
			const char* value_name,
			TNoKeyHandler no_key_handler,
			TKeyConverter key_converter,
			TConverFailHandler convert_fail_handler,
			TNoValueHandler no_value_handler,
			TSetHandler set_handler);

		template <typename TFailHandler>
		static void save_string(pugi::xml_node& node, const char* node_name, const char* value, TFailHandler fail_handler);
	};

	template <class TFlags, typename TSuccessHandler, typename TFailHandler, typename TFlagsStorage>
	void CHelper::load_flags(TFlagsStorage& flags, const pugi::xml_node& root, const char* node_name,
		const char* node_flag, TSuccessHandler success_handler, TFailHandler fail_handler)
	{
		const auto node = root.child(node_name);
		if (node)
		{
			for (const auto flag : node.children(node_flag))
			{
				const char* flag_value = flag.child_value();
				try
				{
					auto value = ITEM_BY_NAME<TFlags>(flag_value);
					set_bit(flags, value);
					success_handler(value);
				}
				catch (...)
				{
					fail_handler(flag_value);
				}
			}
		}
	}

	template <class TFlag, typename TSuccessHandler, typename TFailHandler, typename TNoValueHandler>
	CHelper::ELoadFlagResult CHelper::load_flag(const pugi::xml_node& root, const char* node_name,
		TSuccessHandler success_handler, TFailHandler fail_handler, TNoValueHandler no_value_handler)
	{
		const auto node = root.child(node_name);
		if (node)
		{
			const char* value = node.child_value();
			try
			{
				const TFlag type = ITEM_BY_NAME<TFlag>(value);
				success_handler(type);
			}
			catch (...)
			{
				fail_handler(value);
				return ELFR_FAIL;
			}
		}
		else
		{
			no_value_handler();
			return ELFR_NO_VALUE;
		}

		return ELFR_SUCCESS;
	}

	template <typename TCatchHandler>
	void CHelper::load_integer(const char* input, int& output, TCatchHandler catch_handler)
	{
		try
		{
			output = std::stoi(input);
		}
		catch (...)
		{
			catch_handler();
		}
	}

	template <typename TSetHandler, typename TCatchHandler>
	void CHelper::load_integer(const char* input, TSetHandler set_handler, TCatchHandler catch_handler)
	{
		try
		{
			set_handler(std::stoi(input));
		}
		catch (...)
		{
			catch_handler();
		}
	}

	template <typename KeyType,
		typename TNoKeyHandler,
		typename TKeyConverter,
		typename TConverFailHandler,
		typename TNoValueHandler,
		typename TSetHandler>
	void CHelper::load_pairs_list(const pugi::xml_node* node, const char* group_name, const char* entry_name, const char* key_name, const char* value_name, TNoKeyHandler no_key_handler, TKeyConverter key_converter, TConverFailHandler convert_fail_handler, TNoValueHandler no_value_handler, TSetHandler set_handler)
	{
		const auto group_node = node->child(group_name);
		if (!group_node)
		{
			return;
		}

		size_t number = 0;
		for (const auto entry_node : group_node.children(entry_name))
		{
			++number;
			const auto key_node = entry_node.child(key_name);
			if (!key_node)
			{
				no_key_handler(number);
				continue;
			}

			KeyType key;
			try
			{
				key = key_converter(key_node.child_value());
			}
			catch (...)
			{
				convert_fail_handler(key_node.child_value());
				continue;
			}

			const auto value_node = entry_node.child(value_name);
			if (!value_node)
			{
				no_value_handler(key_node.child_value());
				continue;
			}

			set_handler(key, value_node.child_value());
		}
	}

	template <typename TFailHandler>
	void CHelper::save_string(pugi::xml_node& node, const char* node_name, const char* value, TFailHandler fail_handler)
	{
		auto new_node = node.append_child(node_name);
		if (!new_node)
		{
			log("Failed to create node \"%s\".\n", node_name);
			fail_handler();
		}

		auto cdate_node = new_node.append_child(pugi::node_pcdata);
		if (!cdate_node)
		{
			log("WARNING: Could not add PCDATA child node to node \"%s\".\n", node_name);
			fail_handler();
		}

		if (!cdate_node.set_value(value))
		{
			log("WARNING: Failed to set value to node \"%s\".\n", node_name);
			fail_handler();
		}
	}

	/// Contains handlers of craft subcommands
	namespace cmd
	{
		/**
		 * Returns pointer to the first argument, moves command_line to the beginning of the next argument.
		 * 
		 * \note This routine changes a passed buffer: puts '\0' chrater after the first argument.
		 */
		const char* first_argument(char*& command_line)
		{
			// skip leading spaces
			while (*command_line && ' ' == *command_line)
			{
				++command_line;
			}

			// getting subcommand
			const char* result = command_line;
			while (*command_line && ' ' != *command_line)
			{
				++command_line;
			}

			// anchor subcommand and move argument pointer to the next one
			if (*command_line)
			{
				*(command_line++) = '\0';
			}

			return result;
		}

		void list_skills(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Listing craft skills...\nArguments: '%s'\nCount:%d\n", arguments, model.skills().size());

			size_t counter = 0;
			for (const auto& s : model.skills())
			{
				++counter;
				send_to_char(ch, "%2d. %s\n", counter, s.id().c_str());
			}
		}

		void list_recipes(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Listing craft recipes...\nArguments: '%s'\nCount:%d\n", arguments, model.recipes().size());

			size_t counter = 0;
			for (const auto& r : model.recipes())
			{
				++counter;
				send_to_char(ch, "%2d. %s\n", counter, r.id().c_str());
			}
		}

		void list_prototypes(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Listing craft prototypes...\nArguments: '%s'\nCount: %d\n", arguments, model.prototypes().size());

			size_t counter = 0;
			for (const auto& p : model.prototypes())
			{
				++counter;
				send_to_char(ch, "%2d. %s\n", counter, p.short_desc().c_str());
			}
		}

		void list_properties(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Craft properties...\nArguments: '%s'\n", arguments);
			send_to_char(ch, "Base count:              &W%4d&n crafts\n", model.base_count());
			send_to_char(ch, "Remorts for count bonus: &W%4d&n remorts\n", model.remort_for_count_bonus());
			send_to_char(ch, "Base top:                &W%4d&n percents\n", model.base_top());
			send_to_char(ch, "Remorts bonus:           &W%4d&n percent\n", model.remorts_bonus());
		}

		using subcommand_handler_t = void(*)(CHAR_DATA *, char*, void*);
		using subcommands_t = std::map<std::string, subcommand_handler_t>;

		class CSubcommands
		{
		public:
			enum EProcessResult
			{
				EPR_PROCESSED,
				EPR_UNKNOWN,
				EPR_NO_SUBCOMMAND,
				EPR_WRONG_ARGUMENT
			};

			static EProcessResult process(const subcommands_t& subcommands_table, CHAR_DATA* ch, char*arguments, void* data);
		};

		CSubcommands::EProcessResult CSubcommands::process(const subcommands_t& subcommands_table, CHAR_DATA* ch, char*arguments, void* data)
		{
			if (!arguments)
			{
				log("SYSERROR: argument passed into %s:%d is NULL.", __FILE__, __LINE__);
				send_to_char(ch, "Something went wrong... Send bug report, please. :(\n");

				return EPR_WRONG_ARGUMENT;
			}

			const char* subcommand = first_argument(arguments);
			if (!*subcommand)
			{
				return EPR_NO_SUBCOMMAND;
			}

			subcommands_t::const_iterator i = subcommands_table.lower_bound(subcommand);

			if (i != subcommands_table.end()
				&& i->first.c_str() == strstr(i->first.c_str(), subcommand))
			{
				// found subcommand handler
				i->second(ch, arguments, nullptr);
				return EPR_PROCESSED;
			}

			send_to_char(ch, "Unknown subcommand '%s'.\n", subcommand);
			return EPR_UNKNOWN;
		}

		void do_craft_list(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			static subcommands_t subcommands =
			{
				{ "properties", cmd::list_properties },
				{ "prototypes", cmd::list_prototypes },
				{ "recipes", cmd::list_recipes },
				{ "skills", cmd::list_skills }
			};

			send_to_char(ch, "Listing crafts...\nArguments: '%s'\nCount:%d\n", arguments, model.crafts().size());

			const auto result = CSubcommands::process(subcommands, ch, arguments, nullptr);
			if (CSubcommands::EPR_NO_SUBCOMMAND == result)
			{
				size_t counter = 0;
				for (const auto& c : model.crafts())
				{
					++counter;
					send_to_char(ch, "%2d. %s\n", counter, c.id().c_str());
				}
			}
		}

		void do_craft_export_prototype(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			const auto vnum_str = first_argument(arguments);
			obj_vnum vnum = 0;
			try
			{
				CHelper::load_integer(vnum_str, vnum,
					[&]() { throw std::runtime_error("wrong VNUM value"); });
			}
			catch (...)
			{
				send_to_char(ch, "Could not convert prototype VNUM value '%s' to integer number.\n", vnum_str);
				return;
			}

			const auto filename = first_argument(arguments);
			if (!*filename)
			{
				send_to_char(ch, "File name to export is not specified.");
				return;
			}

			if (!model.export_object(vnum, filename))
			{
				send_to_char(ch, "Failed to export prototype.");
			}
			else
			{
				send_to_char(ch, "Prototype with VNUM %d successfully exported into file '%s'.", vnum, filename);
			}
		}

		void do_craft_export(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			static subcommands_t subcommands =
			{
				{ "prototype", do_craft_export_prototype }
			};

			const auto result = CSubcommands::process(subcommands, ch, arguments, nullptr);
		}

		/**
		* "craft" command handler.
		*
		* Syntax:
		* craft list								- list of crafts
		* craft list prototypes						- list of prototypes loaded by craft system.
		* craft list properties						- list of properties loaded by craft system.
		* craft list recipes						- list of recipes loaded by craft system.
		* craft list skills							- list of skills loaded by craft system.
		* craft export prototype <vnum> <filename>	- exports prototype with <vnum> into file <filename>
		*/
		void do_craft(CHAR_DATA* ch, char* arguments, int /*cmd*/, int/* subcmd*/)
		{
			static subcommands_t subcommands =
			{
				{ "list", do_craft_list },
				{ "export", do_craft_export }
			};

			const auto result = CSubcommands::process(subcommands, ch, arguments, nullptr);

			if (CSubcommands::EPR_NO_SUBCOMMAND == result)
			{
				send_to_char(ch, "Crafting...\n");
				return;
			}
		}

	}

	const char* BODY_PREFIX = "| ";
	const char* END_PREFIX = "> ";

	void CLogger::operator()(const char* format, ...)
	{
		const size_t BUFFER_SIZE = 4096;

		va_list args;
		va_start(args, format);
		char buffer[BUFFER_SIZE];
		char* p = buffer;
		size_t free_space = BUFFER_SIZE;

		std::string prefix;
		for (const auto& part : m_prefix)
		{
			prefix += part;
		}
		const size_t plength = std::min(BUFFER_SIZE, prefix.length());
		strncpy(p, prefix.c_str(), plength);
		free_space -= plength;
		p += plength;

		const size_t length = vsnprintf(p, free_space, format, args);
		va_end(args);

		if (free_space <= length)
		{
			const char truncated[] = " ...<TRUNCATED>\n";
			strncpy(buffer + BUFFER_SIZE - sizeof(truncated), truncated, sizeof(truncated));
		}

		// Use the following line to redirect craft log into syslog:
		// ::log("%s", buffer);
		// instead of output just onto console:
		// FROM HERE...
		if (syslog_converter)
		{
			syslog_converter(buffer, static_cast<int>(length));
		}

		std::cerr << buffer;
		// ...TO HERE
	}

	const std::string CCraftModel::FILE_NAME = LIB_MISC_CRAFT "index.xml";

	bool CCases::load_from_node(const pugi::xml_node* node)
	{
		for (int c = 0; c < CASES_COUNT; ++c)
		{
			const std::string node_name = std::string("case") + std::to_string(1 + c);
			const pugi::xml_node case_node = node->child(node_name.c_str());
			if (!case_node)
			{
				log("ERROR: Could not find case '%s'.\n", node_name.c_str());
				return false;
			}
			m_cases[c] = case_node.child_value();
		}

		const pugi::xml_node aliases_node = node->child("aliases");
		if (aliases_node)
		{
			for (const pugi::xml_node alias_node : aliases_node.children("alias"))
			{
				const char* value = alias_node.child_value();
				m_joined_aliases.append(std::string(m_aliases.empty() ? "" : " ") + value);
				m_aliases.push_back(value);
			}
		}

		return true;
	}

	void CCases::load_from_object(const OBJ_DATA* object)
	{
		boost::algorithm::split(m_aliases, std::string(object->aliases), boost::algorithm::is_any_of(" "), boost::token_compress_on);
		for (size_t n = 0; n < CASES_COUNT; ++n)
		{
			m_cases[n] = object->PNames[n];
		}
	}

	bool CCases::save_to_node(pugi::xml_node* node) const
	{
		try
		{
			size_t number = 0;
			for (const auto& c : m_cases)
			{
				++number;
				const auto case_str = std::string("case") + std::to_string(number);
				CHelper::save_string(*node, case_str.c_str(), c.c_str(),
					[&]() { throw std::runtime_error("failed to save case value"); });
			}

			auto aliases = node->append_child("aliases");
			if (!aliases)
			{
				log("WARNING: Failed to create aliases node");
				return false;
			}

			for (const auto& a : m_aliases)
			{
				CHelper::save_string(aliases, "alias", a.c_str(),
					[&]() { throw std::runtime_error("failed to save alias value"); });
			}
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	OBJ_DATA::pnames_t CCases::build_pnames() const
	{
		OBJ_DATA::pnames_t result;
		for (size_t n = 0; n < CASES_COUNT; ++n)
		{
			result[n] = str_dup(m_cases[n].c_str());
		}
		return result;
	}

	bool CPrototype::load_from_node(const pugi::xml_node* node)
	{
		log("Loading prototype with VNUM %d...\n", m_vnum);
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		const auto description = node->child("description");
		if (description)
		{
			// these fields are optional for prototypes
			m_short_desc = description.child_value("short");
			m_long_desc = description.child_value("long");
			m_keyword = description.child_value("keyword");
			m_extended_desc = description.child_value("extended");
		}

		const auto item = node->child("item");
		if (!item)
		{
			log("ERROR: The prototype with VNUM %d does not contain required \"item\" tag.\n", m_vnum);
			return false;
		}

		if (!m_cases.load_from_node(&item))
		{
			log("ERROR: could not load item cases for the prototype with VNUM %d.\n", m_vnum);
			return false;
		}

		const auto cost = node->child("cost");
		int cost_value = -1;
		if (cost)
		{
			CHelper::load_integer(cost.child_value(), cost_value, [&]() { /* just do nothing: keep default value */});
		}
		else
		{
			log("WARNING: Could not find \"cost\" tag for the prototype with VNUM %d.\n", m_vnum);
		}

		if (0 > cost_value)
		{
			log("WARNING: Wrong \"cost\" value of the prototype with VNUM %d. Setting to the default value %d.\n",
				m_vnum, OBJ_DATA::DEFAULT_COST);
			cost_value = OBJ_DATA::DEFAULT_COST;
		}
		m_cost = cost_value;

		const auto rent = node->child("rent");
		int rent_on_value = -1;
		int rent_off_value = -1;
		if (rent)
		{
			const auto rent_on = rent.child("on");
			if (!rent_on)
			{
				log("WARNING: Could not find \"on\" tag for prototype with VNUM %d.\n", m_vnum);
			}
			else
			{
				CHelper::load_integer(rent_on.child_value(), rent_on_value,
					[&]() { log("WARNING: Wrong value \"%s\" of the \"rent\"/\"on\" tag for prototype with VNUM %d.\n",
						rent_on.child_value(), m_vnum); });
			}

			const auto rent_off = rent.child("off");
			if (!rent_off)
			{
				log("WARNING: Could not find \"off\" tag for prototype with VNUM %d.\n", m_vnum);
			}
			else
			{
				CHelper::load_integer(rent_off.child_value(), rent_off_value,
					[&]() { log("WARNING: Wrong value \"%s\" of the \"rent\"/\"off\" tag for prototype with VNUM %d.\n",
						rent_off.child_value(), m_vnum); });
			}
		}
		else
		{
			log("WARNING: Could not find \"rent\" tag for the prototype with VNUM %d.\n", m_vnum);
		}

		if (0 > rent_on_value)
		{
			log("WARNING: Wrong \"rent/on\" value of the prototype with VNUM %d. Setting to the default value %d.\n",
				m_vnum, OBJ_DATA::DEFAULT_RENT_ON);
			rent_on_value = OBJ_DATA::DEFAULT_RENT_ON;
		}
		m_rent_on = rent_on_value;

		if (0 > rent_off_value)
		{
			log("WARNING: Wrong \"rent/off\" value of the prototype with VNUM %d. Setting to the default value %d.\n",
				m_vnum, OBJ_DATA::DEFAULT_RENT_OFF);
			rent_off_value = OBJ_DATA::DEFAULT_RENT_OFF;
		}
		m_rent_off = rent_off_value;

		const auto global_maximum = node->child("global_maximum");
		if (global_maximum)
		{
			int global_maximum_value = OBJ_DATA::DEFAULT_GLOBAL_MAXIMUM;
			CHelper::load_integer(global_maximum.child_value(), global_maximum_value,
				[&]() { log("WARNING: \"global_maximum\" value of the prototype with VNUM %d is not valid integer. Setting to the default value %d.\n",
					m_vnum, global_maximum_value); });

			if (0 >= global_maximum_value)
			{
				log("WARNING: Wrong \"global_maximum\" value %d of the prototype with VNUM %d. Setting to the default value %d.\n",
					global_maximum_value, m_vnum, OBJ_DATA::DEFAULT_GLOBAL_MAXIMUM);
				global_maximum_value = OBJ_DATA::DEFAULT_GLOBAL_MAXIMUM;
			}

			m_global_maximum = global_maximum_value;
		}

		const auto minimum_remorts = node->child("minimal_remorts");
		if (minimum_remorts)
		{
			int minimum_remorts_value = OBJ_DATA::DEFAULT_MINIMUM_REMORTS; 
			CHelper::load_integer(minimum_remorts.child_value(), minimum_remorts_value,
				[&]() { log("WARNING: \"minimal_remorts\" value of the prototype with VNUM %d is not valid integer. Setting to the default value %d.\n",
					m_vnum, minimum_remorts_value); });

			if (0 > minimum_remorts_value)
			{
				log("WARNING: Wrong \"minimal_remorts\" value %d of the prototype with VNUM %d. Setting to the default value %d.\n",
					minimum_remorts_value, m_vnum, OBJ_DATA::DEFAULT_MINIMUM_REMORTS);
				minimum_remorts_value = OBJ_DATA::DEFAULT_MINIMUM_REMORTS;
			}
			m_minimum_remorts = minimum_remorts_value;
		}

		CHelper::ELoadFlagResult load_result = CHelper::load_flag<type_t>(*node, "type",
			[&](const auto type) { this->set_type(type); },
			[&](const auto name) { log("WARNING: Failed to set object type '%s' for prototype with VNUM %d. Prototype will be skipped.\n", name, m_vnum); },
			[&]() { log("WARNING: \"type\" tag not found for prototype with VNUM %d not found. Setting to default value: %s.\n", m_vnum, NAME_BY_ITEM(get_type()).c_str()); });
		if (CHelper::ELFR_FAIL == load_result)
		{
			return false;
		}

		const auto durability = node->child("durability");
		if (durability)
		{
			const auto maximum = durability.child("maximum");
			if (maximum)
			{
				CHelper::load_integer(maximum.child_value(),
					[&](const auto value) { m_maximum_durability = std::max(value, 0); },
					[&]() { log("WARNING: Wrong integer value of tag \"maximum_durability\" for prototype with VNUM %d. Leaving default value %d\n",
						m_vnum, m_maximum_durability); });
			}

			const auto current = durability.child("current");
			if (current)
			{
				CHelper::load_integer(current.child_value(),
					[&](const auto value) { m_current_durability = std::min(std::max(value, 0), m_maximum_durability); },
					[&]() {
					log("WARNING: Wrong integer value of tag \"current_durability\" for prototype with VNUM %d. Setting to value of \"maximum_durability\" %d\n",
						m_vnum, m_maximum_durability);
					m_current_durability = m_maximum_durability;
				});
			}
		}

		load_result = CHelper::load_flag<decltype(m_sex)>(*node, "sex",
			[&](const auto sex) { m_sex = sex; },
			[&](const auto name) { log("WARNING: Failed to set sex '%s' for prototype with VNUM %d. Prototype will be skipped.\n", name, m_vnum); },
			[&]() { log("WARNING: \"sex\" tag for prototype with VNUM %d not found. Setting to default value: %s.\n", m_vnum, NAME_BY_ITEM(m_sex).c_str()); });
		if (CHelper::ELFR_FAIL == load_result)
		{
			return false;
		}

		const auto level = node->child("level");
		if (level)
		{
			CHelper::load_integer(level.child_value(),
				[&](const auto value) { m_level = std::max(value, 0); },
				[&]() { log("WARNING: Wrong integer value of the \"level\" tag for prototype with VNUM %d. Leaving default value %d.\n",
					m_vnum, m_level); });
		}

		const auto weight = node->child("weight");
		if (weight)
		{
			CHelper::load_integer(weight.child_value(),
				[&](const auto value) { this->set_weight(std::max(value, 1)); },
				[&]() { log("WARNING: Wrong integer value of the \"weight\" tag for prototype with VNUM %d. Leaving default value %d.\n",
					m_vnum, this->get_weight()); });
		}

		const auto timer = node->child("timer");
		{
			const std::string timer_value = timer.child_value();
			if ("unlimited" == timer_value)
			{
				set_timer(OBJ_DATA::UNLIMITED_TIMER);
			}
			else
			{
				CHelper::load_integer(weight.child_value(),
					[&](const auto value) { this->set_timer(std::max(value, 0)); },
					[&]() { log("WARNING: Wrong integer value of the \"timer\" tag for prototype with VNUM %d. Leaving default value %d.\n",
						m_vnum, this->get_timer()); });
			}
		}

		const auto item_parameters = node->child("item_parameters");
		if (item_parameters)
		{
			const bool load_result = load_item_parameters(&item_parameters);
			if (!load_result)
			{
				return false;
			}
		}

		load_result = CHelper::load_flag<decltype(m_material)>(*node, "material",
			[&](const auto material) { m_material = material; },
			[&](const auto name) { log("WARNING: Failed to set material '%s' for prototype with VNUM %d. Prototype will be skipped.\n", name, m_vnum); },
			[&]() { log("WARNING: \"material\" tag for prototype with VNUM %d not found. Setting to default value: %s.\n", m_vnum, NAME_BY_ITEM(m_material).c_str()); });
		if (CHelper::ELFR_FAIL == load_result)
		{
			return false;
		}

		load_result = CHelper::load_flag<decltype(m_spell)>(*node, "spell",
			[&](const auto spell) { m_spell = spell; },
			[&](const auto value) { log("WARNING: Failed to set spell '%s' for prototype with VNUM %d. Spell will not be set.\n", value, m_vnum); },
			[&]() {});

		// loading of prototype extraflags
		CHelper::load_flags<EExtraFlag>(m_extraflags, *node, "extraflags", "extraflag",
			[&](const auto value) { log("Setting extra flag '%s' for prototype with VNUM %d.\n", NAME_BY_ITEM(value).c_str(), m_vnum); },
			[&](const auto flag) { log("WARNING: Skipping extra flag '%s' of prototype with VNUM %d, because this value is not valid.\n", flag, m_vnum); });

        // loading of prototype weapon affect flags
		CHelper::load_flags<EWeaponAffectFlag>(m_waffect_flags, *node, "weapon_affects", "weapon_affect",
			[&](const auto value) { log("Setting weapon affect flag '%s' for prototype with VNUM %d.\n", NAME_BY_ITEM(value).c_str(), m_vnum); },
			[&](const auto flag) { log("WARNING: Skipping weapon affect flag '%s' of prototype with VNUM %d, because this value is not valid.\n", flag, m_vnum); });
        
        // loading of prototype antiflags
		CHelper::load_flags<EAntiFlag>(m_anti_flags, *node, "antiflags", "antiflag",
			[&](const auto value) { log("Setting antiflag '%s' for prototype with VNUM %d.\n", NAME_BY_ITEM(value).c_str(), m_vnum); },
			[&](const auto flag) { log("WARNING: Skipping antiflag '%s' of prototype with VNUM %d, because this value is not valid.\n", flag, m_vnum); });

		// loading of prototype noflags
		CHelper::load_flags<ENoFlag>(m_no_flags, *node, "noflags", "noflag",
			[&](const auto value) { log("Setting noflag '%s' for prototype with VNUM %d.\n", NAME_BY_ITEM(value).c_str(), m_vnum); },
			[&](const auto flag) { log("WARNING: Skipping noflag '%s' of prototype with VNUM %d, because this value is not valid.\n", flag, m_vnum); });

		// loading of prototype wearflags
		CHelper::load_flags<EWearFlag>(m_wear_flags, *node, "wearflags", "wearflag",
			[&](const auto value) { log("Setting wearflag '%s' for prototype with VNUM %d.\n", NAME_BY_ITEM(value).c_str(), m_vnum); },
			[&](const auto flag) { log("WARNING: Skipping wearflag '%s' of prototype with VNUM %d, because this value is not valid.\n", flag, m_vnum); });

		// loading of prototype skills
		load_skills(node);

		// load prototype vals
		for (size_t i = 0; i < m_vals.size(); ++i)
		{
			std::stringstream val_name;
			val_name << "val" << i;

			const auto val_node = node->child(val_name.str().c_str());
			if (val_node)
			{
				CHelper::load_integer(val_node.child_value(), m_vals[i],
					[&]() { log("WARNING: \"%s\" tag of prototype with VNUM %d has wrong integer value. Leaving default value %d.\n",
						val_name.str().c_str(), m_vnum, m_vals[i]); });
			}
		}

		const auto triggers_node = node->child("triggers");
		for (const auto trigger_node : triggers_node.children("trigger"))
		{
			const char* vnum_str = trigger_node.child_value();
			CHelper::load_integer(vnum_str,
				[&](const auto value) { m_triggers_list.push_back(value); },
				[&]() { log("WARNING: Invalid trigger's VNUM value \"%s\" for prototype with VNUM %d. Skipping.\n",
					vnum_str, m_vnum); });
		}

		load_extended_values(node);
		load_applies(node);

		if (!check_prototype_consistency())
		{
			log("WARNING: Prototype with VNUM %d has not passed consistency check.\n",
				m_vnum);
			return false;
		}

		prefix.change_prefix(END_PREFIX);
		log("End of loading prototype with VNUM %d.\n", m_vnum);

		return true;
	}

	void CPrototype::load_from_object(const OBJ_DATA* object)
	{
		set_type(object->obj_flags.type_flag);

		m_maximum_durability = object->obj_flags.Obj_max;
		m_current_durability = object->obj_flags.Obj_cur;
		m_material = object->obj_flags.Obj_mater;
		m_sex = object->obj_flags.Obj_sex;
		set_timer(object->get_timer());
		m_item_params = object->obj_flags.Obj_skill;
		m_spell = static_cast<ESpell>(object->obj_flags.Obj_spell);
		m_level= object->obj_flags.Obj_level;
		set_weight(object->obj_flags.weight);
		m_cost = object->get_cost();
		m_rent_off = object->get_rent();
		m_rent_on = object->get_rent_eq();

		m_waffect_flags = object->obj_flags.affects;
		m_anti_flags = object->obj_flags.anti_flag;
		m_no_flags = object->obj_flags.no_flag;
		m_extraflags = object->obj_flags.extra_flags;

		m_wear_flags = object->obj_flags.wear_flags;

		m_cases.load_from_object(object);
		m_short_desc = object->short_description;
		m_long_desc = object->description;
		m_action_desc = object->action_description;

		if (nullptr != object->ex_description)
		{
			m_keyword = object->ex_description->keyword;
			m_extended_desc = object->ex_description->description;
		}

		m_global_maximum = object->max_in_world;
		m_minimum_remorts = object->get_manual_mort_req();

		for (size_t i = 0; i < m_vals.size(); ++i)
		{
			m_vals[i]= object->obj_flags.value[i];
		}

		const auto& skills = object->get_skills();
		for (const auto& s : skills)
		{
			m_skills[static_cast<ESkill>(s.first)] = s.second;
		}

		m_triggers_list = object->proto_script;
		m_extended_values = object->values;

		size_t number = 0;
		for (const auto& apply : object->affected)
		{
			m_applies.push_back(apply);
		}
	}

	bool CPrototype::save_to_node(pugi::xml_node* node) const
	{
		auto description = node->append_child("description");
		if (!description)
		{
			log("WARNIGN: Failed to create description node.\n");
			return false;
		}

		try
		{
			CHelper::save_string(description, "short", m_short_desc.c_str(),
				[&]() { throw std::runtime_error("failed to save short description"); });
			CHelper::save_string(description, "long", m_long_desc.c_str(),
				[&]() { throw std::runtime_error("failed to save long description"); });
			CHelper::save_string(description, "action", m_action_desc.c_str(),
				[&]() { throw std::runtime_error("failed to save action description"); });
			CHelper::save_string(description, "keyword", m_keyword.c_str(),
				[&]() { throw std::runtime_error("failed to save keyword"); });
			CHelper::save_string(description, "extended", m_extended_desc.c_str(),
				[&]() { throw std::runtime_error("failed to save extended description"); });

			auto item = node->append_child("item");
			if (!item)
			{
				log("Failed to create item node.\n");
				return false;
			}

			if (!m_cases.save_to_node(&item))
			{
				// Error message was output inside m_cases.save_to_node
				return false;
			}
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	OBJ_DATA* CPrototype::build_object() const
	{
		const auto result = NEWCREATE<OBJ_DATA>();
		
		result->obj_flags.type_flag = get_type();

		result->obj_flags.Obj_max = m_maximum_durability;
		result->obj_flags.Obj_cur = m_current_durability;
		result->obj_flags.Obj_mater = m_material;
		result->obj_flags.Obj_sex = m_sex;
		result->set_timer(get_timer());
		result->obj_flags.Obj_skill = m_item_params;
		result->obj_flags.Obj_spell = m_spell;
		result->obj_flags.Obj_level = m_level;
		result->obj_flags.Obj_destroyer = 60;	// I don't know why it is constant. But seems like it is the same for all prototypes.
		result->obj_flags.weight = get_weight();
		result->set_cost(m_cost);
		result->set_rent(m_rent_off);
		result->set_rent_eq(m_rent_on);

		result->obj_flags.affects = m_waffect_flags;
		result->obj_flags.anti_flag = m_anti_flags;
		result->obj_flags.no_flag = m_no_flags;
		result->obj_flags.extra_flags = m_extraflags;

		result->obj_flags.wear_flags = m_wear_flags;

		result->aliases = str_dup(aliases().c_str());
		result->short_description = str_dup(m_short_desc.c_str());
		result->description = str_dup(m_long_desc.c_str());
		result->PNames = m_cases.build_pnames();
		result->action_description = str_dup(m_action_desc.c_str());

		CREATE(result->ex_description, 1);
		result->ex_description->keyword = str_dup(m_keyword.c_str());
		result->ex_description->description = str_dup(m_extended_desc.c_str());

		result->max_in_world = m_global_maximum;
		result->set_manual_mort_req(m_minimum_remorts);

		for (size_t i = 0; i < m_vals.size(); ++i)
		{
			result->obj_flags.value[i] = m_vals[i];
		}

		for (const auto& s : m_skills)
		{
			result->set_skill(s.first, s.second);
		}

		result->proto_script = m_triggers_list;
		result->values = m_extended_values;

		size_t number = 0;
		for (const auto& apply : m_applies)
		{
			result->affected[number++] = apply;
		}

		return result;
	}

	bool CPrototype::load_item_parameters(const pugi::xml_node* node)
	{
		switch (get_type())
		{
		case obj_flag_data::ITEM_INGREDIENT:
			for (const auto flags : node->children("parameter"))
			{
				const char* flag = flags.child_value();
				try
				{
					const auto value = ITEM_BY_NAME<EIngredientFlag>(flag);
					m_item_params |= to_underlying(value);
					log("Setting ingredient flag '%s' for prototype with VNUM %d.\n",
						NAME_BY_ITEM(value).c_str(), m_vnum);
				}
				catch (const std::out_of_range&)
				{
					log("WARNING: Skipping ingredient flag '%s' of prototype with VNUM %d, because this value is not valid.\n",
						flag, m_vnum);
				}
			}
			break;

		case obj_flag_data::ITEM_WEAPON:
		{
			const char* skill_value = node->child_value("parameter");
			try
			{
				m_item_params = to_underlying(ITEM_BY_NAME<ESkill>(skill_value));
			}
			catch (const std::out_of_range&)
			{
				log("WARNING: Failed to set skill value '%s' for prototype with VNUM %d. Prototype will be skipped.\n",
					skill_value, m_vnum);
				return false;
			}
			break;
		}

		default:
			// For other item types "skills" tag should be ignored.
			break;
		}

		return true;
	}

	void CPrototype::load_skills(const pugi::xml_node* node)
	{
		CHelper::load_pairs_list<ESkill>(node, "skills", "skill", "id", "value",
			[&](const size_t number) { log("WARNING: %d-%s \"skill\" tag of \"skills\" group does not have the \"id\" tag. Prototype with VNUM %d.\n",
				number, suffix(number), m_vnum); },
			[&](const auto value) -> auto { return ITEM_BY_NAME<ESkill>(value); },
			[&](const auto key) { log("WARNING: Could not convert value \"%s\" to skill ID. Prototype with VNUM %d.\n Skipping entry.\n",
				key, m_vnum); },
			[&](const auto key) { log("WARNING: skill with key \"%s\" does not have \"value\" tag. Prototype with VNUM %d. Skipping entry.\n",
				key, m_vnum); },
			[&](const auto key, const auto value) { CHelper::load_integer(value,
				[&](const auto int_value) {
					m_skills.emplace(key, int_value);
					log("Adding skill pair (%s, %d) to prototype with VNUM %d.\n",
						NAME_BY_ITEM(key).c_str(), int_value, m_vnum);
				},
				[&]() { log("WARNIGN: Could not convert skill value of \"value\" tag to integer. Entry key value \"%s\". Prototype with VNUM %d",
					NAME_BY_ITEM(key).c_str(), m_vnum); }); });
	}

	void CPrototype::load_extended_values(const pugi::xml_node* node)
	{
		CHelper::load_pairs_list<ObjVal::EValueKey>(node, "extended_values", "entry", "key", "value",
			[&](const size_t number) { log("WARNING: %d-%s \"entry\" tag of \"extended_values\" group does not have the \"key\" tag. Prototype with VNUM %d.\n",
				number, suffix(number), m_vnum); },
			[&](const auto value) -> auto { return static_cast<ObjVal::EValueKey>(TextId::to_num(TextId::OBJ_VALS, value)); },
			[&](const auto key) { log("WARNING: Could not convert extended value \"%s\" to key value. Prototype with VNUM %d.\n Skipping entry.\n",
				key, m_vnum); },
			[&](const auto key) { log("WARNING: entry with key \"%s\" does not have \"value\" tag. Prototype with VNUM %d. Skipping entry.\n",
				key, m_vnum); },
			[&](const auto key, const auto value) { CHelper::load_integer(value,
				[&](const auto int_value) {
					m_extended_values.set(key, int_value);
					log("Adding extended values pair (%s, %d) to prototype with VNUM %d.\n",
						TextId::to_str(TextId::OBJ_VALS, to_underlying(key)).c_str(), int_value, m_vnum);
				},
				[&]() { log("WARNIGN: Could not convert extended value of \"value\" tag to integer. Entry key value \"%s\". Prototype with VNUM %d",
					TextId::to_str(TextId::OBJ_VALS, to_underlying(key)).c_str(), m_vnum); }); });
	}

	void CPrototype::load_applies(const pugi::xml_node* node)
	{
		CHelper::load_pairs_list<EApplyLocation>(node, "applies", "apply", "location", "modifier",
			[&](const size_t number) { log("WARNING: %d-%s \"apply\" tag of \"applies\" group does not have the \"location\" tag. Prototype with VNUM %d.\n",
				number, suffix(number), m_vnum); },
			[&](const auto value) -> auto { return ITEM_BY_NAME<EApplyLocation>(value); },
			[&](const auto key) { log("WARNING: Could not convert value \"%s\" to apply location. Prototype with VNUM %d.\n Skipping entry.\n",
				key, m_vnum); },
			[&](const auto key) { log("WARNING: apply with key \"%s\" does not have \"modifier\" tag. Prototype with VNUM %d. Skipping entry.\n",
				key, m_vnum); },
			[&](const auto key, const auto value) { CHelper::load_integer(value,
				[&](const auto int_value) {
					m_applies.push_back(decltype(m_applies)::value_type(key, int_value));
					log("Adding apply pair (%s, %d) to prototype with VNUM %d.\n",
						NAME_BY_ITEM(key).c_str(), int_value, m_vnum);
				},
				[&]() { log("WARNIGN: Could not convert apply value of \"modifier\" tag to integer. Entry key value \"%s\". Prototype with VNUM %d",
					NAME_BY_ITEM(key).c_str(), m_vnum); }); });
		if (m_applies.size() > MAX_OBJ_AFFECT)
		{
			std::stringstream ignored_applies;
			bool first = true;
			while (m_applies.size() > MAX_OBJ_AFFECT)
			{
				const auto& apply = m_applies.back();
				ignored_applies << (first ? "" : ", ") << NAME_BY_ITEM(apply.location);
				m_applies.pop_back();
				first = false;
			}

			log("WARNING: prototype with VNUM %d has applies over limit %d. The following applies is ignored: { %s }.\n",
				m_vnum, MAX_OBJ_AFFECT, ignored_applies.str().c_str());
		}
	}

	bool CPrototype::check_prototype_consistency() const
	{
		// perform some checks here.

		return true;
	}

	bool CMaterialClass::load(const pugi::xml_node* node)
	{
		log("Loading material class with ID '%s'...\n", m_id.c_str());
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		const auto desc_node = node->child("description");
		if (!desc_node)
		{
			log("ERROR: material class with ID '%s' does not contain required \"description\" tag.\n",
				m_id.c_str());
			return false;
		}

		const auto short_desc = desc_node.child("short");
		if (!short_desc)
		{
			log("ERROR: material class with ID '%s' does not contain required \"description/short\" tag.\n",
					m_id.c_str());
			return false;
		}
		m_short_desc = short_desc.value();

		const auto long_desc = desc_node.child("long");
		if (!long_desc)
		{
			log("ERROR: material class with ID '%s' does not contain required \"description/long\" tag.\n",
					m_id.c_str());
			return false;
		}
		m_long_desc = long_desc.value();

		const auto item = node->child("item");
		if (!item)
		{
			log("ERROR: material class with ID '%s' does not contain required \"item\" tag.\n", m_id.c_str());
			return false;
		}
		if (!m_item_cases.load_from_node(&item))
		{
			log("ERROR: could not load item cases for material class '%s'.\n", m_id.c_str());
			return false;
		}

		const auto adjectives = node->child("adjectives");
		if (!adjectives)
		{
			log("ERROR: material class with ID '%s' does not contain required \"adjectives\" tag.\n", m_id.c_str());
			return false;
		}

		const auto male = adjectives.child("male");
		if (!male)
		{
			log("ERROR: material class with ID '%s' does not contain required \"adjectives/male\" tag.\n", m_id.c_str());
			return false;
		}
		if (!m_male_adjectives.load_from_node(&male))
		{
			log("ERROR: could not load male adjective cases for material class '%s'.\n", m_id.c_str());
			return false;
		}

		const auto female = adjectives.child("female");
		if (!female)
		{
			log("ERROR: material class with ID '%s' does not contain required \"adjectives/female\" tag.\n", m_id.c_str());
			return false;
		}
		if (!m_female_adjectives.load_from_node(&female))
		{
			log("ERROR: could not load female adjective cases for material class '%s'.\n", m_id.c_str());
			return false;
		}

		const auto neuter = adjectives.child("neuter");
		if (!neuter)
		{
			log("ERROR: material class with ID '%s' does not contain required \"adjectives/neuter\" tag.\n", m_id.c_str());
			return false;
		}
		if (!m_neuter_adjectives.load_from_node(&neuter))
		{
			log("ERROR: could not load neuter adjective cases for material class '%s'.\n", m_id.c_str());
			return false;
		}

		// load extra flags
		CHelper::load_flags<EExtraFlag>(m_extraflags, *node, "extraflags", "extraflag",
			[&](const auto value) { log("Setting extra flag '%s' for class ID %s.\n", NAME_BY_ITEM(value).c_str(), m_id.c_str()); },
			[&](const auto flag) { log("WARNING: Skipping extra flag '%s' of class with ID %s, because this value is not valid.\n", flag, m_id.c_str()); });

		// load weapon affects
		CHelper::load_flags<EWeaponAffectFlag>(m_waffect_flags, *node, "weapon_affects", "weapon_affect",
			[&](const auto value) { log("Setting weapon affect flag '%s' for class ID %s.\n", NAME_BY_ITEM(value).c_str(), m_id.c_str()); },
			[&](const auto flag) { log("WARNING: Skipping weapon affect flag '%s' of class with ID %s, because this value is not valid.\n", flag, m_id.c_str()); });

		prefix.change_prefix(END_PREFIX);
		log("End of loading material class with ID '%s'.\n", m_id.c_str());

		return true;
	}

	bool CMaterial::load(const pugi::xml_node* node)
	{
		log("Loading material with ID %s...\n", m_id.c_str());
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		// load material name
		const auto node_name = node->child("name");
		if (!node_name)
		{
			log("ERROR: could not find required node 'name' for material with ID '%s'.\n", m_id.c_str());
			return false;
		}
		const std::string name = node_name.value();

		// load meterial classes
		for (const auto node_class: node->children("class"))
		{
			if (node_class.attribute("id").empty())
			{
				log("WARNING: class tag of material with ID '%s' does not contain ID of class. Class will be skipped.\n", m_id.c_str());
				continue;
			}
			const std::string class_id = node_class.attribute("id").value();
			CMaterialClass mc(class_id);
			mc.load(&node_class);
		}

		prefix.change_prefix(END_PREFIX);
		log("End of loading material with ID '%s'.\n", m_id.c_str());

		return true;
	}

	bool CRecipe::load(const pugi::xml_node* /*node*/)
	{
		log("Loading recipe with ID %s...\n", m_id.c_str());
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		prefix.change_prefix(END_PREFIX);
		log("End of loading recipe with ID %s\n", m_id.c_str());

		return true;
	}

	bool CSkillBase::load(const pugi::xml_node* /*node*/)
	{
		log("Loading skill with ID %s...\n", m_id.c_str());
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		prefix.change_prefix(END_PREFIX);
		log("End of loading skill with ID %s\n", m_id.c_str());

		return true;
	}

	bool CCraft::load(const pugi::xml_node* /*node*/)
	{
		log("Loading craft with ID %s...\n", m_id.c_str());
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		prefix.change_prefix(END_PREFIX);
		log("End of loading craft with ID %s\n", m_id.c_str());

		return true;
	}

	bool CCraftModel::load()
	{
		log("Loading craft model from file '%s'...\n",
			FILE_NAME.c_str());
		CLogger::CPrefix prefix(log, BODY_PREFIX);

		pugi::xml_document doc;
		const auto result = doc.load_file(FILE_NAME.c_str());

		if (!result)
		{
			log("Craft load error: '%s' at offset %zu\n",
				result.description(),
				result.offset);
			return false;
		}
		
		const auto model = doc.child("craftmodel");
		if (!model)
		{
			log("Craft model is not defined in XML file %s\n", FILE_NAME.c_str());
			return false;
		}

		// Load model properties.
		const auto base_count_node = model.child("base_crafts");
		if (base_count_node)
		{
			CHelper::load_integer(base_count_node.child_value(), m_base_count,
				[&]() { log("WARNING: \"base_crafts\" tag has wrong integer value. Leaving default value %d.\n", m_base_count); });
		}

		const auto remort_for_count_bonus_node = model.child("crafts_bonus");
		if (remort_for_count_bonus_node)
		{
			CHelper::load_integer(remort_for_count_bonus_node.child_value(), m_remort_for_count_bonus,
				[&]() { log("WARNING: \"crafts_bonus\" tag has wrong integer value. Leaving default value %d.\n", m_remort_for_count_bonus); });
		}

		const auto base_top_node = model.child("skills_cap");
		if (base_top_node)
		{
			CHelper::load_integer(base_top_node.child_value(), m_base_top,
				[&]() { log("WARNING: \"skills_cap\" tag has wrong integer value. Leaving default value %d.\n", m_base_top); });
		}

		const auto remorts_bonus_node = model.child("skills_bonus");
		if (remorts_bonus_node)
		{
			CHelper::load_integer(remorts_bonus_node.child_value(), m_remorts_bonus,
				[&]() { log("WARNING: \"skills_bonus\" tag has wrong integer value. Leaving default value %d.\n", m_remorts_bonus); });
		}

		// TODO: load remaining properties.

		// Load materials.
		const auto materials = model.child("materials");
		if (materials)
		{
			size_t number = 0;
			for (const auto material: materials.children("material"))
			{
				load_material(&material, ++number);
			}
		}

		// Load recipes.
		// TODO: load it.

		// Load skills.
		// TODO: load it.

		// Load crafts.
		// TODO: load it.

		// Load prototypes
		const auto prototypes = model.child("prototypes");
		if (prototypes)
		{
			size_t number = 0;
			for (const auto prototype : prototypes.children("prototype"))
			{
				load_prototype(&prototype, ++number);
			}
		}

		prefix.change_prefix(END_PREFIX);
		log("End of loading craft model.\n");
		// TODO: print statistics of the model (i. e. count of materials, recipes, crafts, missed entries and so on).

		return true;
	}

	bool CCraftModel::merge()
	{
		for (const auto& p : m_prototypes)
		{
			OBJ_DATA* object = p.build_object();
			obj_proto.add(object, p.vnum());
		}

		return true;
	}

	CLogger log;

	void CCraftModel::create_item() const
	{
	}

	bool CCraftModel::load_prototype(const pugi::xml_node* prototype, const size_t number)
	{
		if (prototype->attribute("vnum").empty())
		{
			log("%zd-%s prototype tag does not have VNUM attribute. Will be skipped.\n",
				number, suffix(number));
			return false;
		}

		obj_vnum vnum = prototype->attribute("vnum").as_int(0);
		if (0 == vnum)
		{
			log("Failed to get VNUM of the %zd-%s prototype. This prototype entry will be skipped.\n",
				number, suffix(number));
		}

		CPrototype p(vnum);
		if (prototype->attribute("filename").empty())
		{
			if (!p.load_from_node(prototype))
			{
				log("WARNING: Skipping %zd-%s prototype with VNUM %d.\n",
					number, suffix(number), vnum);
				return false;
			}
		}
		else
		{
			using boost::filesystem::path;
			const std::string filename = (path(FILE_NAME).parent_path() / prototype->attribute("filename").value()).string();
			pugi::xml_document pdoc;
			const auto presult = pdoc.load_file(filename.c_str());
			if (!presult)
			{
				log("WARNING: could not load external file '%s' with %zd-%s prototype (ID: %d): '%s' "
					"at offset %zu. Prototype will be skipped.\n",
					filename.c_str(),
					number,
					suffix(number),
					vnum,
					presult.description(),
					presult.offset);
				return false;
			}
			const auto proot = pdoc.child("prototype");
			if (!proot)
			{
				log("WARNING: could not find root \"prototype\" tag for prototype with VNUM "
					"%d in the external file '%s'. Prototype will be skipped.\n",
					vnum,
					filename.c_str());
				return false;
			}
			log("Using external file '%s' for %zd-%s prototype with VNUM %d.\n",
				filename.c_str(),
				number,
				suffix(number),
				vnum);
			if (!p.load_from_node(&proot))
			{
				log("WARNING: Skipping %zd-%s prototype with VNUM %d.\n",
					number, suffix(number), vnum);
				return false;
			}
		}

		m_prototypes.push_back(p);

		return true;
	}

	bool CCraftModel::load_material(const pugi::xml_node* material, const size_t number)
	{
		if (material->attribute("id").empty())
		{
			log("%zd-%s material tag does not have ID attribute. Will be skipped.\n",
				number, suffix(number));
			return false;
		}
		id_t id = material->attribute("id").as_string();
		CMaterial m(id);
		if (material->attribute("filename").empty())
		{
			if (!m.load(material))
			{
				log("WARNING: Skipping material with ID '%s'.\n", id.c_str());
				return false;
			}
		}
		else
		{
			using boost::filesystem::path;
			const std::string filename = (path(FILE_NAME).parent_path() / material->attribute("filename").value()).string();
			pugi::xml_document mdoc;
			const auto mresult = mdoc.load_file(filename.c_str());
			if (!mresult)
			{
				log("WARNING: could not load external file '%s' with material '%s': '%s' "
					"at offset %zu. Material will be skipped.\n",
					filename.c_str(),
					id.c_str(),
					mresult.description(),
					mresult.offset);
				return false;
			}
			const auto mroot = mdoc.child("material");
			if (!mroot)
			{
				log("WARNING: could not find root \"material\" tag for material with ID "
					"'%s' in the external file '%s'. Material will be skipped.\n",
					id.c_str(),
					filename.c_str());
				return false;
			}
			log("Using external file '%s' for material with ID '%s'.\n",
				filename.c_str(),
				id.c_str());
			if (!m.load(&mroot))
			{
				log("WARNING: Skipping material with ID '%s'.\n",
					id.c_str());
				return false;
			}
		}

		return true;
	}

	bool CCraftModel::export_object(const obj_vnum vnum, const char* filename)
	{
		const auto rnum = obj_proto.rnum(vnum);
		if (-1 == rnum)
		{
			log("WARNING: Failed to export prototype with VNUM %d", vnum);
			return false;
		}

		const auto object = obj_proto[rnum];
		pugi::xml_document document;
		pugi::xml_node root = document.append_child("prototype");
		CPrototype prototype(vnum);
		prototype.load_from_object(object);

		if (!prototype.save_to_node(&root))
		{
			log("WARNING: Could not save prototype to XML.");
			return false;
		}

		pugi::xml_node decl = document.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "koi8-r";

		return document.save_file(filename);
	}

}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
