#ifndef __MSDP_HPP__
#define __MSDP_HPP__

#include <string>

struct DESCRIPTOR_DATA;	// to avoid inclusion of "structs.h"

const char TELOPT_MSDP = 69;

namespace msdp
{
	extern size_t handle_conversation(DESCRIPTOR_DATA* t, const char* pos, const size_t length);
	void msdp_report(DESCRIPTOR_DATA* d, const std::string& name);
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
