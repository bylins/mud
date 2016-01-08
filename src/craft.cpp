/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "db.h"
#include "pugixml.hpp"

#include <iostream>

namespace craft
{
	bool load()
	{
		CCraftModel model;

		return model.load();
	}

	const std::string CCraftModel::FILE_NAME = LIB_MISC_CRAFT "craft.xml";

	bool CMaterial::load(const pugi::xml_node* node)
	{
		log("Loading material with ID %s\n", m_id.c_str());

		return true;
	}

	bool CRecipe::load(const pugi::xml_node* node)
	{
		log("Loading recipe with ID %s\n", m_id.c_str());

		return true;
	}

	bool CSkillBase::load(const pugi::xml_node* node)
	{
		log("Loading skill with ID %s\n", m_id.c_str());

		return true;
	}

	bool CCraft::load(const pugi::xml_node* node)
	{
		log("Loading craft with ID %s\n", m_id.c_str());

		return true;
	}

	bool CCraftModel::load()
	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(FILE_NAME.c_str());

		if (!result)
		{
			log("Craft load error: %s at offset %zu\n",
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
		pugi::xml_node materials = model.child("materials");
		if (materials)
		{
			for (pugi::xml_node material = materials.child("material"); material; material = material.next_sibling("material"))
			{
				if (material.attribute("id").empty())
				{
					log("Material tag does not have ID attribute. Will be skipped.\n");
					continue;
				}
				id_t id = material.attribute("id").as_string();
				CMaterial m(id);
				m.load(&material);
			}
		}

		// Load recipes.
		// TODO: load it.

		// Load skills.
		// TODO: load it.

		// Load crafts.
		// TODO: load it.

		return true;
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
