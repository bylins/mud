/* ************************************************************************
*   File: ban.hpp                                       Part of Bylins    *
*  Usage: banning/unbanning/checking sites and player names functions     *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef __BAN_HPP__
#define __BAN_HPP__

#include "conf.h"
#include "diskio.h"

#include <string>
#include <list>
#include <algorithm>
#include <sys/types.h>

////////////////////////////////////////////////////////////////////////////////


struct BanNode
{
	std::string BannedIp;
	std::string BanReason;
	std::string BannerName;
	time_t BanDate;
	time_t UnbanDate;
	int BanType;
};

struct ProxyBanNode
{
	std::string BannedIp;
	std::string BannerName;
};

typedef
std::shared_ptr < BanNode > BanNodePtr;
typedef
std::shared_ptr < ProxyBanNode > ProxyBanNodePtr;

class BanList
{
public:
	static const char *const
	ban_filename;
	static const char *const
	proxy_ban_filename;
	static const char *const
	proxy_ban_filename_tmp;
	static const int
	BAN_NO = 0;
	static const int
	BAN_NEW = 1;
	static const int
	BAN_SELECT = 2;
	static const int
	BAN_ALL = 3;
	static const int
	BAN_ERROR = 4;
	static const char *ban_types[];
	static const int
	SORT_UNDEFINED = -1;
	static const int
	SORT_BY_NAME = 1;
	static const int
	SORT_BY_DATE = 2;
	static const int
	SORT_BY_BANNER = 3;
	static const int
	BAN_MAX_TIME = 0x7fffffff;
// compare modes for ban_compare
	static const int
	BAN_IP_COMPARE = 1;
	static const int
	BAN_TIME_COMPARE = 2;
	static const int
	BAN_LENGTH_COMPARE = 3;

	static const int
	MAX_STRLEN = 8192;

	static const int
	RELOAD_MODE_MAIN = 0;
	static const int
	RELOAD_MODE_TMPFILE = 1;

	BanList();
//methods
	time_t getBanDate(std::string Ip);
	bool add_ban(std::string BannedIp, std::string BanReason, std::string BannerName, int UnbanDate, int BanType);
	bool add_proxy_ban(std::string BannedIp, std::string BannerName);
	bool unban(std::string Ip, CHAR_DATA *ch);
	bool unban_ip(std::string Ip, CHAR_DATA *ch);
	bool unban_proxy(std::string ProxyIp, CHAR_DATA *ch);
	int
	is_banned(std::string Ip);
	bool reload_ban();
	bool reload_proxy_ban(int mode);
	bool save_ip();
	bool save_proxy();
	void
	sort_ip(int sort_algorithm);
	void
	sort_proxy(int sort_algorithm);
	bool empty_ip();
	bool empty_proxy();
	void
	clear_all();
	void
	purge_obsolete();
	void
	disconnectBannedIp(std::string Ip);
//////////////////////////////////////////////////////////////////////////////
	void
	ShowBannedIp(int sort_mode, CHAR_DATA *ch);
	void
	ShowBannedProxy(int sort_mode, CHAR_DATA *ch);
	void
	ShowBannedIpByMask(int sort_mode, CHAR_DATA *ch, const char *mask);
//////////////////////////////////////////////////////////////////////////////
private:
	std::list < BanNodePtr > Ban_List;
	std::list < ProxyBanNodePtr > Proxy_Ban_List;
	int
	current_sort_algorithm;
	int
	current_proxy_sort_algorithm;
	bool ban_compare(BanNodePtr nodePtr, int mode, const void *op2);
	bool proxy_ban_compare(ProxyBanNodePtr nodePtr, int mode, const void *op2);
	bool ban_sort_func(const BanNodePtr & lft, const BanNodePtr & rght, int sort_algorithm);
	bool proxy_ban_sort_func(const ProxyBanNodePtr & lft, const ProxyBanNodePtr & rght, int sort_algorithm);
};

////////////////////////////////////////////////////////////////////////////////

namespace RegisterSystem
{

void add(CHAR_DATA* ch, const char* text, const char* reason);
void remove(CHAR_DATA* ch);
bool is_registered(CHAR_DATA* ch);
bool is_registered_email(const std::string& email);
const std::string show_comment(const std::string& email);
void load();
void save();

} // namespace RegisterSystem

extern BanList*& ban;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
