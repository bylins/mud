#include "structs.h"

#include "utils.h"
#include "char.hpp"
#include "msdp.hpp"

void DESCRIPTOR_DATA::msdp_support(bool on)
{
	log("INFO: MSDP support enabled for client %s.\n", host);
	m_msdp_support = on;
}

void DESCRIPTOR_DATA::msdp_report(const std::string& name)
{
	msdp::msdp_report(this, name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
