/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "comm.h"
#include "db.h"
#include "pugixml.hpp"

#include <boost/filesystem.hpp>

#include <iostream>

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

	void CLogger::operator()(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		// Use the following line to redirect craft log into syslog:
		// ::log(format, args);
		// instead of output just onto console:
		// FROM HERE...
		const size_t BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		char* p = buffer;
		const size_t length = vsnprintf(p, BUFFER_SIZE, format, args);
		va_end(args);

		if (BUFFER_SIZE <= length)
		{
			std::cerr << "TRUNCATED: ";
			p[BUFFER_SIZE - 1] = '\0';
		}

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
				log("ERROR: could not find case '%s'.\n", node_name.c_str());
				return false;
			}
			m_cases[c] = case_node.value();
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
		return true;
	}

	bool CMaterialClass::load(const pugi::xml_node* node)
	{
		log("Loading class with ID '%s'.\n", m_id.c_str());

		const pugi::xml_node desc_node = node->child("description");
		if (!desc_node)
		{
			log("ERROR: material class with ID '%s' does not contain required \"description\" tag.\n",
					m_id.c_str());
			return false;
		}

		const pugi::xml_node short_desc = desc_node.child("short");
		if (!short_desc)
		{
			log("ERROR: material class with ID '%s' does not contain required \"description/short\" tag.\n",
					m_id.c_str());
			return false;
		}
		m_short_desc = short_desc.value();

		const pugi::xml_node long_desc = desc_node.child("long");
		if (!long_desc)
		{
			log("ERROR: material class with ID '%s' does not contain required \"description/long\" tag.\n",
					m_id.c_str());
			return false;
		}
		m_long_desc = long_desc.value();

		const pugi::xml_node item = node->child("item");
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

		const pugi::xml_node adjectives = node->child("adjectives");
		if (!adjectives)
		{
			log("ERROR: material class with ID '%s' does not contain required \"adjectives\" tag.\n", m_id.c_str());
			return false;
		}

		const pugi::xml_node male = adjectives.child("male");
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

		const pugi::xml_node female = adjectives.child("female");
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

		const pugi::xml_node neuter = adjectives.child("neuter");
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
		const pugi::xml_node extraflags = node->child("extraflags");
		if (extraflags)
		{
			for (const pugi::xml_node extraflag : extraflags.children())
			{
				const char* flag = extraflag.child_value("extraflag");
				try
				{
					EExtraFlag value = ITEM_BY_NAME<EExtraFlag>(flag);
					m_extraflags.set(value);
					log("Setting extra flag '%s' for class ID %s.\n", NAME_BY_ITEM(value).c_str(), m_id.c_str());
				}
				catch (const std::out_of_range&)
				{
					log("Skipping extra flag '%s' of class with ID %s, because this value is not valid.\n", flag, m_id.c_str());
				}
			}
		}

		// load extra flags
		const pugi::xml_node affects = node->child("affects");
		if (affects)
		{
			for (const pugi::xml_node affect : affects.children("affect"))
			{
				const char* flag = affect.child_value();
				try
				{
					EAffectFlag value = ITEM_BY_NAME<EAffectFlag>(flag);
					m_affect_flags.set(value);
					log("Setting affect flag '%s' for class ID %s.\n", NAME_BY_ITEM(value).c_str(), m_id.c_str());
				}
				catch (const std::out_of_range&)
				{
					log("Skipping affect flag '%s' of class with ID %s, because this value is not valid.\n", flag, m_id.c_str());
				}
			}
		}

		return true;
	}

	bool CMaterial::load(const pugi::xml_node* node)
	{
		log("Begin loading material with ID %s\n", m_id.c_str());

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

		log("End of loading material with ID '%s'.\n", m_id.c_str());
		return true;
	}

	bool CRecipe::load(const pugi::xml_node* /*node*/)
	{
		log("Loading recipe with ID %s\n", m_id.c_str());

		return true;
	}

	bool CSkillBase::load(const pugi::xml_node* /*node*/)
	{
		log("Loading skill with ID %s\n", m_id.c_str());

		return true;
	}

	bool CCraft::load(const pugi::xml_node* /*node*/)
	{
		log("Loading craft with ID %s\n", m_id.c_str());

		return true;
	}

	bool CCraftModel::load()
	{
		log("Loading craft model from file '%s'.\n",
				FILE_NAME.c_str());
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
				log("WARNING: skipping %d-%s prototype with VNUM %d.\n",
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
				log("WARNING: skipping %d-%s prototype with VNUM %d.\n",
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
				log("WARNING: skipping material with ID '%s'.\n", id.c_str());
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
				log("WARNING: skipping material with ID '%s'.\n",
					id.c_str());
				return false;
			}
		}

		return true;
	}

}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
