#ifndef __CRAFT_STATIC_HPP__
#define __CRAFT_STATIC_HPP__

#include <memory>

namespace craft
{
	class CCraftModel;
	namespace cmd
	{
		class CommandsHandler;
	}

	extern CCraftModel model;
	extern std::shared_ptr<cmd::CommandsHandler> commands_handler;
}

#endif // __CRAFT_STATIC_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
