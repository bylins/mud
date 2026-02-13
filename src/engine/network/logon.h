/**
\file logon.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief description.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_LOGON_H_
#define BYLINS_SRC_ENGINE_NETWORK_LOGON_H_

#include <ctime>
#include <vector>

struct DescriptorData;

namespace network {

struct Logon {
  char *ip;
  long count;
  time_t lasttime;
  bool is_first;
};

using LogonRecords = std::vector<Logon>;

void add_logon_record(DescriptorData *d);

} // namespace network

#endif //BYLINS_SRC_ENGINE_NETWORK_LOGON_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
