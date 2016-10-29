#ifndef __MSDP_HPP__
#define __MSDP_HPP__

#include <string>

struct DESCRIPTOR_DATA;	// to avoid inclusion of "structs.h"

const char TELOPT_MSDP = 69;

namespace msdp
{
	size_t handle_conversation(DESCRIPTOR_DATA* t, const char* pos, const size_t length);
	void report(DESCRIPTOR_DATA* d, const std::string& name);

	void debug(const bool on);
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
