/* ************************************************************************
*   File: ban.hpp                                       Part of Bylins    *
*  Usage: banning/unbanning/checking sites and player names functions     *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef BAN_HPP_
#define BAN_HPP_

#include "engine/core/conf.h"
#include "utils/diskio.h"

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <sys/types.h>

class CharData;

struct BanNode {
  std::string BannedIp;
  std::string BanReason;
  std::string BannerName;
  time_t BanDate;
  time_t UnbanDate;
  int BanType;
};

struct ProxyBanNode {
  std::string BannedIp;
  std::string BannerName;
};

using BanNodePtr = std::shared_ptr<BanNode>;
using ProxyBanNodePtr = std::shared_ptr<ProxyBanNode>;

class BanList {
 public:
  static const char *const ban_filename;
  static const char *const proxy_ban_filename;
  static const int BAN_NO = 0;
  static const int BAN_NEW = 1;
  static const int BAN_SELECT = 2;
  static const int BAN_ALL = 3;
  static const int BAN_ERROR = 4;
  static const char *ban_types[];
  static const int SORT_UNDEFINED = -1;
  static const int SORT_BY_NAME = 1;
  static const int SORT_BY_DATE = 2;
  static const int SORT_BY_BANNER = 3;
  static const int BAN_MAX_TIME = 0x7fffffff;
// compare modes for ban_compare
  static const int BAN_IP_COMPARE = 1;
  static const int BAN_TIME_COMPARE = 2;
  static const int BAN_LENGTH_COMPARE = 3;
  static const int MAX_STRLEN = 8192;
  static const int RELOAD_MODE_MAIN = 0;
  static const int RELOAD_MODE_TMPFILE = 1;

  BanList();
  time_t GetBanDate(std::string Ip);
  bool AddBan(std::string BannedIp,
			  const std::string &BanReason,
			  const std::string &BannerName,
			  int UnbanDate,
			  int BanType);
  bool AddProxyBan(std::string BannedIp, const std::string &BannerName);
  bool Unban(const std::string &Ip, CharData *ch);
  bool UnbanIp(std::string Ip, CharData *ch);
  bool UnbanProxy(std::string ProxyIp, CharData *ch);
  int IsBanned(std::string Ip);
  bool ReloadBan();
  bool ReloadProxyBan();
  bool SaveIp();
  bool SaveProxy();
  void SortIp(int sort_algorithm);
  void SortProxy(int sort_algorithm);
  bool EmptyIp();
  bool EmptyProxy();
  void ClearAll();
  void PurgeObsolete();
  static void DisconnectBannedIp(const std::string &Ip);
  void ShowBannedIp(int sort_mode, CharData *ch);
  void ShowBannedProxy(int sort_mode, CharData *ch);
  void ShowBannedIpByMask(int sort_mode, CharData *ch, const char *mask);

 private:
  std::list<BanNodePtr> ban_list_;
  std::list<ProxyBanNodePtr> proxy_ban_list_;
  int current_sort_algorithm_;
  int current_proxy_sort_algorithm_;
  static bool BanCompare(const BanNodePtr &nodePtr, int mode, const void *op2);
  static bool ProxyBanCompare(const ProxyBanNodePtr &nodePtr, int mode, const void *op2);
  static bool BanSortFunc(const BanNodePtr &lft, const BanNodePtr &rght, int sort_algorithm);
  static bool ProxyBanSortFunc(const ProxyBanNodePtr &lft, const ProxyBanNodePtr &rght, int sort_algorithm);
};

extern BanList *&ban;

#endif // BAN_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
