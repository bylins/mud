#include "boot.constants.hpp"

const std::string& FilesPrefixes::operator()(const EBootType mode) const
{
	const auto result = find(mode);
	return result == end() ? s_empty_prefix : result->second;
}

FilesPrefixes::FilesPrefixes()
{
	(*this)[DB_BOOT_TRG] = TRG_PREFIX;
	(*this)[DB_BOOT_WLD] = WLD_PREFIX;
	(*this)[DB_BOOT_MOB] = MOB_PREFIX;
	(*this)[DB_BOOT_OBJ] = OBJ_PREFIX;
	(*this)[DB_BOOT_ZON] = ZON_PREFIX;
	(*this)[DB_BOOT_HLP] = HLP_PREFIX;
	(*this)[DB_BOOT_SOCIAL] = SOC_PREFIX;
}

std::string FilesPrefixes::s_empty_prefix;
FilesPrefixes prefixes;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
