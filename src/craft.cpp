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
	const std::string CCraftModel::FILE_NAME = LIB_MISC_CRAFT "craft.xml";

	bool CCraftModel::load()
	{
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
