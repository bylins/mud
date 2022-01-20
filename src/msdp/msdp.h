#ifndef __MSDP_HPP__
#define __MSDP_HPP__

#include "comm.h"

#include <ostream>

#include "structs/descriptor_data.h"

namespace msdp {
size_t handle_conversation(DescriptorData *t, const char *pos, const size_t length);
void debug(const bool on);

class Report {
 public:
	void operator()(DescriptorData *d, const std::string &type);
};

extern Report report;
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
