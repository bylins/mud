/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "db.h"
#include "pugixml.hpp"

#include <boost/filesystem.hpp>

#include <iostream>

namespace craft
{
	bool load()
	{
		CCraftModel model;

		return model.load();
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

	bool CMaterial::load(const pugi::xml_node* node)
	{
		log("Begin loading material with ID %s\n", m_id.c_str());

		// load material name
		const pugi::xml_node node_name = node->child("name");
		if (!node_name)
		{
			log("WARNING: could not find required node 'name' for material with ID '%s'. Material will be skipped\n", m_id.c_str());
			return false;
		}
		const std::string name = node_name.value();

		// load meterial classes
		for (const pugi::xml_node node_class: node->children("class"))
		{
			if (node_class.attribute("id").empty())
			{
				log("WARNING: class tag of material with ID '%s' does not contain ID of class. Class will be skipped.\n", m_id.c_str());
				continue;
			}
			const std::string class_name = node_class.attribute("id").value();
			log("Loading class with ID '%s'\n", class_name.c_str());
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
		pugi::xml_parse_result result = doc.load_file(FILE_NAME.c_str());

		if (!result)
		{
			log("Craft load error: '%s' at offset %zu\n",
					result.description(),
					result.offset);
			return false;
		}
		
		pugi::xml_node model = doc.child("craftmodel");
		if (!model)
		{
			log("Craft model is not defined in XML file %s\n", FILE_NAME.c_str());
			return false;
		}
		// Load model properties.
		// TODO: load it.

		// Load materials.
		const pugi::xml_node materials = model.child("materials");
		if (materials)
		{
			for (const pugi::xml_node material: materials.children("material"))
			{
				if (material.attribute("id").empty())
				{
					log("Material tag does not have ID attribute. Will be skipped.\n");
					continue;
				}
				id_t id = material.attribute("id").as_string();
				CMaterial m(id);
				if (material.attribute("filename").empty())
				{
					if (!m.load(&material))
					{
						log("WARNING: skipping material with ID '%s'.\n",
								id.c_str());
					}
				}
				else
				{
					using boost::filesystem::path;
					const std::string filename = (path(FILE_NAME).parent_path() / material.attribute("filename").value()).string();
					pugi::xml_document mdoc;
					pugi::xml_parse_result mresult = mdoc.load_file(filename.c_str());
					if (!mresult)
					{
						log("WARNING: could not load external file '%s' with material '%s': '%s' "
								"at offset %zu. Material will be skipped.\n",
								filename.c_str(),
								id.c_str(),
								mresult.description(),
								mresult.offset);
						continue;
					}
					const pugi::xml_node mroot = mdoc.child("material");
					if (!mroot)
					{
						log("WARNING: could not find root \"material\" tag for material with ID "
								"'%s' in the external file '%s'. Material will be skipped.\n",
								id.c_str(),
								filename.c_str());
						continue;
					}
					log("Using external file '%s' for material with ID '%s'.\n",
							filename.c_str(),
							id.c_str());
					if (!m.load(&mroot))
					{
						log("WARNING: skipping material with ID '%s'.\n",
								id.c_str());
					}
				}
			}
		}

		// Load recipes.
		// TODO: load it.

		// Load skills.
		// TODO: load it.

		// Load crafts.
		// TODO: load it.

		log("End of loading craft model.\n");
		// TODO: print statistics of the model (i. e. count of materials, recipes, crafts, missed entries and so on).

		return true;
	}

	CLogger log;
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
