/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "db.h"
#include "pugixml.hpp"

namespace craft
{
	bool load()
	{
		CCraftModel model;

		model.load();
	}

	const std::string CCraftModel::FILE_NAME = LIB_MISC_CRAFT "craft.xml";

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

		return true;
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
