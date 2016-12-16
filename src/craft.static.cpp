#include "craft.static.hpp"

#include "craft.hpp"
#include "craft.commands.hpp"

namespace craft
{
	CCraftModel model;
	std::shared_ptr<cmd::CommandsHandler> commands_handler = cmd::CommandsHandler::create();
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
