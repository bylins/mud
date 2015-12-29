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
		std::cerr << "Loading material with ID " << m_id << std::endl;

		return true;
	}

	bool CRecipe::load(const pugi::xml_node* node)
	{
		std::cerr << "Loading recipe with ID " << m_id << std::endl;

		return true;
	}

	bool CSkillBase::load(const pugi::xml_node* node)
	{
		std::cerr << "Loading skill with ID " << m_id << std::endl;

		return true;
	}

	bool CCraft::load(const pugi::xml_node* node)
	{
		std::cerr << "Loading craft with ID " << m_id << std::endl;

		return true;
	}

	bool CCraftModel::load()
	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(FILE_NAME.c_str());

		if (!result)
		{
			std::cerr << "Craft load error: " << result.description() << std::endl
				<< " at offset " << result.offset << std::endl;
			return false;
		}
		
		pugi::xml_node model = doc.child("craftmodel");
		if (!model)
		{
			std::cerr << "Craft model is not defined in XML file " << FILE_NAME << std::endl;
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
					std::cerr << "Material tag does not have ID attribute. Will be skipped." << std::endl;
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
