/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "skills.h"
#include "comm.h"
#include "db.h"
#include "pugixml.hpp"

#include <boost/filesystem.hpp>

#include <iostream>
#include <string>

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

	bool load()
	{
		CCraftModel model;

		return model.load();
	}
	
	void do_craft(CHAR_DATA *ch, char *argument, int cmd, int subcmd)
	{
		send_to_char("&WCrafting...&n\r\n", ch);
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

	bool CCases::load(const pugi::xml_node* node)
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
				m_aliases.push_back(value);
			}
		}

		return true;
	}

	bool CPrototype::load(const pugi::xml_node* node)
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

		if (!m_cases.load(&item))
		{
			log("ERROR: could not load item cases for the prototype with VNUM.\n", m_vnum);
			return false;
		}

		const auto cost = node->child("cost");
		int cost_value = -1;
		if (cost)
		{
			cost_value = std::atoi(cost.child_value());
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
				rent_on_value = std::atoi(rent_on.child_value());
			}

			const auto rent_off = rent.child("off");
			if (!rent_off)
			{
				log("WARNING: Could not find \"off\" tag for prototype with VNUM %d.\n", m_vnum);
			}
			else
			{
				rent_off_value = std::atoi(rent_off.child_value());
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
			int global_maximum_value = std::atoi(global_maximum.child_value());
			if (0 >= global_maximum_value)
			{
				log("WARNING: Wrong \"global_maximum\" value of the prototype with VNUM %d. Setting to the default value %d.\n",
					m_vnum, OBJ_DATA::DEFAULT_GLOBAL_MAXIMUM);
				global_maximum_value = OBJ_DATA::DEFAULT_GLOBAL_MAXIMUM;
			}
			m_global_maximum = global_maximum_value;
		}

		const auto minimum_remorts = node->child("minimal_remorts");
		if (minimum_remorts)
		{
			int minimum_remorts_value = std::atoi(minimum_remorts.child_value());
			if (0 > minimum_remorts_value)
			{
				log("WARNING: Wrong \"minimal_remorts\" value of the prototype with VNUM %d. Setting to the default value %d.\n",
					m_vnum, OBJ_DATA::DEFAULT_MINIMUM_REMORTS);
				minimum_remorts_value = OBJ_DATA::DEFAULT_MINIMUM_REMORTS;
			}
			m_minimum_remorts = minimum_remorts_value;
		}

		const auto object_type = node->child("type");
		if (object_type)
		{
			const char* name = object_type.child_value();
			try
			{
				const obj_flag_data::EObjectType type = ITEM_BY_NAME<obj_flag_data::EObjectType>(name);
				set_type(type);
			}
			catch (const std::out_of_range&)
			{
				log("WARNING: Failed to set object type '%s' for prototype with VNUM %d. Prototype will be skipped.\n",
					name, m_vnum);
				return false;
			}
		}
		else
		{
			log("WARNING: \"type\" tag not found for prototype with VNUM %d not found. Setting to default value: %s.\n",
				m_vnum, NAME_BY_ITEM(get_type()));
		}

		const auto durability = node->child("durability");
		if (durability)
		{
			const auto maximum = durability.child("maximum");
			if (maximum)
			{
				m_maximum_durability = std::max(std::atoi(maximum.child_value()), 0);
			}

			const auto current = durability.child("current");
			if (current)
			{
				m_current_durability = std::min(std::max(std::atoi(current.child_value()), 0), m_maximum_durability);
			}
		}

		const auto sex = node->child("sex");
		if (sex)
		{
			const char* sex_value = sex.child_value();
			try
			{
				m_sex = ITEM_BY_NAME<decltype(m_sex)>(sex_value);
			}
			catch (const std::out_of_range&)
			{
				log("WARNING: Failed to set sex '%s' for prototype with VNUM %d. Prototype will be skipped.\n",
					sex_value, m_vnum);
				return false;
			}
		}
		else
		{
			log("WARNING: \"sex\" tag for prototype with VNUM %d not found. Setting to default value: %s.\n",
				m_vnum, NAME_BY_ITEM(m_sex));
		}

		const auto level = node->child("level");
		if (level)
		{
			m_level = std::max(std::atoi(level.child_value()), 0);
		}

		const auto weight = node->child("weight");
		{
			const int weight_value = std::max(std::atoi(level.child_value()), 1);
			set_weight(weight_value);
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
				const int t = std::max(std::atoi(timer_value.c_str()), 0);
				set_timer(t);
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

		const auto material = node->child("material");
		if (material)
		{
			const char* material_value = material.child_value();
			try
			{
				m_material = ITEM_BY_NAME<decltype(m_material)>(material_value);
			}
			catch (const std::out_of_range&)
			{
				log("WARNING: Failed to set material '%s' for prototype with VNUM %d. Prototype will be skipped.\n",
					material_value, m_vnum);
				return false;
			}
		}
		else
		{
			log("WARNING: \"material\" tag for prototype with VNUM %d not found. Setting to default value: %s.\n",
				m_vnum, NAME_BY_ITEM(m_material));
		}

		prefix.change_prefix(END_PREFIX);
		log("End of loading prototype with VNUM %d.\n", m_vnum);

		return true;
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
		if (!m_item_cases.load(&item))
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
		if (!m_male_adjectives.load(&male))
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
		if (!m_female_adjectives.load(&female))
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
		if (!m_neuter_adjectives.load(&neuter))
		{
			log("ERROR: could not load neuter adjective cases for material class '%s'.\n", m_id.c_str());
			return false;
		}

		// load extra flags
		const auto extraflags = node->child("extraflags");
		if (extraflags)
		{
			for (const auto extraflag : extraflags.children("extraflag"))
			{
				const char* flag = extraflag.child_value();
				try
				{
					auto value = ITEM_BY_NAME<EExtraFlag>(flag);
					m_extraflags.set(value);
					log("Setting extra flag '%s' for class ID %s.\n", NAME_BY_ITEM(value).c_str(), m_id.c_str());
				}
				catch (const std::out_of_range&)
				{
					log("WARNING: Skipping extra flag '%s' of class with ID %s, because this value is not valid.\n", flag, m_id.c_str());
				}
			}
		}

		// load extra flags
		const auto affects = node->child("affects");
		if (affects)
		{
			for (const auto affect : affects.children("affect"))
			{
				const char* flag = affect.child_value();
				try
				{
					auto value = ITEM_BY_NAME<EAffectFlag>(flag);
					m_affect_flags.set(value);
					log("Setting affect flag '%s' for class ID %s.\n", NAME_BY_ITEM(value).c_str(), m_id.c_str());
				}
				catch (const std::out_of_range&)
				{
					log("WARNING: Skipping affect flag '%s' of class with ID %s, because this value is not valid.\n", flag, m_id.c_str());
				}
			}
		}

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
		// TODO: load it.

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

	CLogger log;

	void CCraftModel::create_item() const
	{

	}

	bool CCraftModel::load_prototype(const pugi::xml_node* prototype, const size_t number)
	{
		if (prototype->attribute("vnum").empty())
		{
			log("%d-%s prototype tag does not have VNUM attribute. Will be skipped.\n",
				number, suffix(number));
			return false;
		}

		vnum_t vnum = prototype->attribute("vnum").as_int(0);
		if (0 == vnum)
		{
			log("Failed to get VNUM of the %d-%s prototype. This prototype entry will be skipped.\n",
				number, suffix(number));
		}

		CPrototype p(vnum);
		if (prototype->attribute("filename").empty())
		{
			if (!p.load(prototype))
			{
				log("WARNING: Skipping %d-%s prototype with VNUM %d.\n",
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
				log("WARNING: could not load external file '%s' with %d-%s prototype (ID: %d): '%s' "
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
			log("Using external file '%s' for %d-%s prototype with VNUM %d.\n",
				filename.c_str(),
				number,
				suffix(number),
				vnum);
			if (!p.load(&proot))
			{
				log("WARNING: Skipping %d-%s prototype with VNUM %d.\n",
					number, suffix(number), vnum);
				return false;
			}
		}

		return true;
	}

	bool CCraftModel::load_material(const pugi::xml_node* material, const size_t number)
	{
		if (material->attribute("id").empty())
		{
			log("%d-%s material tag does not have ID attribute. Will be skipped.\n",
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

}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
