#ifndef __MSDP_CONSTANTS_HPP_
#define __MSDP_CONSTANTS_HPP_

#include <string>

namespace msdp
{
	namespace constants
	{
		extern const std::string ROOM;
		extern const std::string EXPERIENCE;
		extern const std::string GOLD;
		extern const std::string LEVEL;
		extern const std::string MAX_HIT;
		extern const std::string MAX_MOVE;
		extern const std::string STATE;
		extern const std::string GROUP;

		constexpr char TELOPT_MSDP = 69;
	}
}

#endif // __MSDP_CONSTANTS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
