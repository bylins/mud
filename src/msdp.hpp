#ifndef __MSDP_HPP__
#define __MSDP_HPP__

struct DESCRIPTOR_DATA;	// to avoid inclusion of "structs.h"

namespace msdp
{
	extern size_t handle_conversation(DESCRIPTOR_DATA* t, const char* pos, const size_t length);
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
