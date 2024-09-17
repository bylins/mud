/* ************************************************************************
*   File: ban.cpp                                       Part of Bylins    *
*  Usage: banning/unbanning/checking sites and player names               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "ban.h"
#include "engine/ui/modify.h"
#include "engine/db/global_objects.h"

#include <third_party_libs/fmt/include/fmt/format.h>

const char *const BanList::ban_filename = "etc/badsites";
const char *const BanList::proxy_ban_filename = "etc/badproxy";
const char *BanList::ban_types[] = {
	"no",
	"new",
	"select",
	"all",
	"ERROR"
};

bool BanList::BanCompare(const BanNodePtr &nodePtr, int mode, const void *op2) {
	switch (mode) {
		case BAN_IP_COMPARE:
			if (nodePtr->BannedIp == *(std::string *) op2)
				return true;
			break;
		case BAN_TIME_COMPARE:
			if (nodePtr->BanDate == *(long *) op2)
				return true;
			break;
		default:;
	}

	return false;
}

bool BanList::ProxyBanCompare(const ProxyBanNodePtr &nodePtr, int mode, const void *op2) {
	switch (mode) {
		case BAN_IP_COMPARE:
			if (nodePtr->BannedIp == *(std::string *) op2)
				return true;
			break;
		default:;
	}

	return false;
}

bool BanList::BanSortFunc(const BanNodePtr &lft, const BanNodePtr &rght, int sort_algorithm) {
	switch (sort_algorithm) {
		case SORT_BY_NAME:
			return std::lexicographical_compare(lft->BannedIp.begin(),
												lft->BannedIp.end(), rght->BannedIp.begin(), rght->BannedIp.end());
		case SORT_BY_DATE: return (lft->BanDate > rght->BanDate);
		case SORT_BY_BANNER:
			return std::lexicographical_compare(lft->BannerName.begin(),
												lft->BannerName.end(),
												rght->BannerName.begin(), rght->BannerName.end());
		default: return true;
	}
}

bool BanList::ProxyBanSortFunc(const ProxyBanNodePtr &lft, const ProxyBanNodePtr &rght, int sort_algorithm) {
	switch (sort_algorithm) {
		case SORT_BY_NAME:
			return std::lexicographical_compare(lft->BannedIp.begin(),
												lft->BannedIp.end(), rght->BannedIp.begin(), rght->BannedIp.end());
		case SORT_BY_BANNER:
			return std::lexicographical_compare(lft->BannerName.begin(),
												lft->BannerName.end(),
												rght->BannerName.begin(), rght->BannerName.end());
		default: return true;
	}
}

BanList::BanList() :
	current_sort_algorithm_(0),
	current_proxy_sort_algorithm_(0) {
	ReloadBan();
	ReloadProxyBan();
	SortIp(SORT_BY_NAME);
	SortProxy(SORT_BY_NAME);
	PurgeObsolete();
}

void BanList::SortIp(int sort_algorithm) {
	if (sort_algorithm == current_sort_algorithm_)
		return;

	ban_list_.sort([&](const BanNodePtr &lhs, const BanNodePtr &rhs) {
	  return BanList::BanSortFunc(lhs, rhs, sort_algorithm);
	});

	current_sort_algorithm_ = sort_algorithm;
}

void BanList::SortProxy(int sort_algorithm) {
	if (sort_algorithm == current_proxy_sort_algorithm_)
		return;

	proxy_ban_list_.sort([&](const ProxyBanNodePtr &lhs, const ProxyBanNodePtr &rhs) {
	  return BanList::ProxyBanSortFunc(lhs, rhs, sort_algorithm);
	});

	current_proxy_sort_algorithm_ = sort_algorithm;
}

bool BanList::AddBan(std::string BannedIp,
					 const std::string &BanReason,
					 const std::string &BannerName,
					 int UnbanDate,
					 int BanType) {
	BanNodePtr temp_node_ptr(new struct BanNode);

	if (BannedIp.empty() || BannerName.empty())
		return false;

	// looking if the ip is already in the ban list
	auto i = std::find_if(ban_list_.begin(), ban_list_.end(), [&](const BanNodePtr &ptr) {
	  return BanList::BanCompare(ptr, BAN_IP_COMPARE, &BannedIp);
	});

	if (i != ban_list_.end())
		return false;

	temp_node_ptr->BannedIp = BannedIp;
	temp_node_ptr->BanReason = (BanReason.empty() ? "Unknown" : BanReason);
	temp_node_ptr->BannerName = BannerName;
	temp_node_ptr->BanDate = time(nullptr);
	temp_node_ptr->UnbanDate = (UnbanDate > 0) ? time(nullptr) + UnbanDate * 60 * 60 : BAN_MAX_TIME;
	temp_node_ptr->BanType = BanType;

	// checking all strings: replacing all whitespaces with underlines
	size_t k = temp_node_ptr->BannedIp.size();
	for (size_t j = 0; j < k; j++) {
		if (temp_node_ptr->BannedIp[j] == ' ') {
			temp_node_ptr->BannedIp[j] = '_';
		}
	}

	k = temp_node_ptr->BannerName.size();
	for (size_t j = 0; j < k; j++) {
		if (temp_node_ptr->BannerName[j] == ' ') {
			temp_node_ptr->BannerName[j] = '_';
		}
	}

	k = temp_node_ptr->BanReason.size();
	for (size_t j = 0; j < k; j++) {
		if (temp_node_ptr->BanReason[j] == ' ') {
			temp_node_ptr->BanReason[j] = '_';
		}
	}

	ban_list_.push_front(temp_node_ptr);
	current_sort_algorithm_ = SORT_UNDEFINED;
	SaveIp();
///////////////////////////////////////////////////////////////////////
	if (BanType == 3)
		DisconnectBannedIp(BannedIp);

	sprintf(buf, "%s has banned %s for %s players(%s) (%dh).",
			BannerName.c_str(), BannedIp.c_str(), ban_types[BanType], temp_node_ptr->BanReason.c_str(), UnbanDate);
	mudlog(buf, BRF, kLvlGod, SYSLOG, true);
	imm_log("%s has banned %s for %s players(%s) (%dh).", BannerName.c_str(),
			BannedIp.c_str(), ban_types[BanType], temp_node_ptr->BanReason.c_str(), UnbanDate);

///////////////////////////////////////////////////////////////////////
	return true;
}

bool BanList::AddProxyBan(std::string BannedIp, const std::string &BannerName) {
	ProxyBanNodePtr temp_node_ptr(new struct ProxyBanNode);
	if (BannedIp.empty() || BannerName.empty())
		return false;

	auto i = std::find_if(proxy_ban_list_.begin(), proxy_ban_list_.end(), [&](const ProxyBanNodePtr &ptr) {
	  return BanList::ProxyBanCompare(ptr, BAN_IP_COMPARE, &BannedIp);
	});

	if (i != proxy_ban_list_.end())
		return false;

	temp_node_ptr->BannedIp = BannedIp;
	temp_node_ptr->BannerName = BannerName;

	// checking all strings: replacing all whitespaces with underlines
	size_t k = temp_node_ptr->BannedIp.size();
	for (size_t j = 0; j < k; j++) {
		if (temp_node_ptr->BannedIp[j] == ' ') {
			temp_node_ptr->BannedIp[j] = '_';
		}
	}

	k = temp_node_ptr->BannerName.size();
	for (size_t j = 0; j < k; j++) {
		if (temp_node_ptr->BannerName[j] == ' ') {
			temp_node_ptr->BannerName[j] = '_';
		}
	}

	proxy_ban_list_.push_front(temp_node_ptr);
	current_proxy_sort_algorithm_ = SORT_UNDEFINED;
	SaveProxy();
	DisconnectBannedIp(BannedIp);
	sprintf(buf, "%s has banned proxy %s", BannerName.c_str(), BannedIp.c_str());
	mudlog(buf, BRF, kLvlGod, SYSLOG, true);
	imm_log("%s has banned proxy %s", BannerName.c_str(), BannedIp.c_str());
	return true;
}

bool BanList::ReloadBan() {
	FILE *loaded;
	ban_list_.clear();
	if ((loaded = fopen(ban_filename, "r"))) {
		std::string str_to_parse;

		while (!feof(loaded)) {
			DiskIo::read_line(loaded, str_to_parse, true);
			std::vector<std::string> ban_tokens = utils::Split(str_to_parse);

			if (ban_tokens.size() != 6) {
				log("Ban list %s error, line %s", ban_filename, str_to_parse.c_str());
				continue;
			}
			BanNodePtr ptr(new struct BanNode);
			auto tok_iter = ban_tokens.begin();

			// type
			for (int i = BAN_NO; i <= BAN_ALL; i++) {
				if (!strcmp((*tok_iter).c_str(), ban_types[i])) {
					ptr->BanType = i;
				}
			}
			// ip
			++tok_iter;
			ptr->BannedIp = (*tok_iter);
			// removing port specification i.e. 129.1.1.1:8080; :8080 is removed
			std::string::size_type at = ptr->BannedIp.find_first_of(':');
			if (at != std::string::npos) {
				ptr->BannedIp.erase(at);
			}
			//ban_date
			++tok_iter;
			ptr->BanDate = atol((*tok_iter).c_str());
			//banner_name
			++tok_iter;
			ptr->BannerName = (*tok_iter);
			//ban_length
			++tok_iter;
			ptr->UnbanDate = atol((*tok_iter).c_str());
			//ban_reason
			++tok_iter;
			ptr->BanReason = (*tok_iter);
			ban_list_.push_front(ptr);
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to open banfile");
	return false;
}

bool BanList::ReloadProxyBan() {
	FILE *loaded;

	loaded = fopen(proxy_ban_filename, "r");
	if (loaded) {
		std::string str_to_parse;

		while (DiskIo::read_line(loaded, str_to_parse, true)) {
			std::vector<std::string> ban_tokens = utils::Split(str_to_parse);

			if (ban_tokens.size() != 2) {
				log("Proxy ban list %s error, line %s", proxy_ban_filename, str_to_parse.c_str());
				continue;
			}
			auto tok_iter = ban_tokens.begin();
			ProxyBanNodePtr ptr(new struct ProxyBanNode);

			ptr->BannerName = "Undefined";
			ptr->BannedIp = (*tok_iter);
			// removing port specification i.e. 129.1.1.1:8080; :8080 is removed
			std::string::size_type at = ptr->BannedIp.find_first_of(':');
			if (at != std::string::npos) {
				ptr->BannedIp.erase(at);
			}
			++tok_iter;
			ptr->BannerName = (*tok_iter);
			proxy_ban_list_.push_front(ptr);
			DisconnectBannedIp(ptr->BannedIp);
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to open proxybanfile");
	return false;
}

bool BanList::SaveIp() {
	FILE *loaded;
	if ((loaded = fopen(ban_filename, "w"))) {
		for (auto &i : ban_list_) {
			fprintf(loaded, "%s %s %ld %s %ld %s\n",
					ban_types[i->BanType], i->BannedIp.c_str(),
					static_cast<long int>(i->BanDate), i->BannerName.c_str(),
					static_cast<long int>(i->UnbanDate), i->BanReason.c_str());
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to save banfile");
	return false;
}

bool BanList::SaveProxy() {
	FILE *loaded;
	if ((loaded = fopen(proxy_ban_filename, "w"))) {
		for (auto &i : proxy_ban_list_) {
			fprintf(loaded, "%s %s\n", i->BannedIp.c_str(), i->BannerName.c_str());
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to save proxybanfile");
	return false;
}

void BanList::ShowBannedIp(int sort_mode, CharData *ch) {
	if (ban_list_.empty()) {
		SendMsgToChar("No sites are banned.\r\n", ch);
		return;
	}

	SortIp(sort_mode);
	char format[kMaxInputLength], to_unban[kMaxInputLength], buff[kMaxInputLength], *listbuf = nullptr, *timestr;
	strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s %-8.8s\r\n");
	sprintf(buf, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By", "To Unban");

	listbuf = str_add(listbuf, buf);
	sprintf(buf, format,
			"---------------------------------",
			"---------------------------------",
			"---------------------------------",
			"---------------------------------", "---------------------------------");
	listbuf = str_add(listbuf, buf);

	for (auto &i : ban_list_) {
		timestr = asctime(localtime(&(i->BanDate)));
		*(timestr + 10) = 0;
		strcpy(to_unban, timestr);
		sprintf(buff, "%ldh", static_cast<long int>(i->UnbanDate - time(nullptr)) / 3600);
		sprintf(buf, format, i->BannedIp.c_str(), ban_types[i->BanType],
				to_unban, i->BannerName.c_str(), buff);
		listbuf = str_add(listbuf, buf);
		strcpy(buf, i->BanReason.c_str());
		strcat(buf, "\r\n");
		listbuf = str_add(listbuf, buf);
	}
	page_string(ch->desc, listbuf, 1);
	free(listbuf);
}

void BanList::ShowBannedIpByMask(int sort_mode, CharData *ch, const char *mask) {
	bool is_find = false;
	if (ban_list_.empty()) {
		SendMsgToChar("No sites are banned.\r\n", ch);
		return;
	}

	SortIp(sort_mode);
	char format[kMaxInputLength], to_unban[kMaxInputLength], buff[kMaxInputLength], *listbuf = nullptr, *timestr;
	strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s %-8.8s\r\n");
	sprintf(buf, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By", "To Unban");

	listbuf = str_add(listbuf, buf);
	sprintf(buf, format,
			"---------------------------------",
			"---------------------------------",
			"---------------------------------",
			"---------------------------------", "---------------------------------");
	listbuf = str_add(listbuf, buf);

	for (auto &i : ban_list_) {
		if (strncmp(i->BannedIp.c_str(), mask, strlen(mask)) == 0) {
			timestr = asctime(localtime(&(i->BanDate)));
			*(timestr + 10) = 0;
			strcpy(to_unban, timestr);
			sprintf(buff, "%ldh", static_cast<long int>(i->UnbanDate - time(nullptr)) / 3600);
			sprintf(buf, format, i->BannedIp.c_str(), ban_types[i->BanType],
					to_unban, i->BannerName.c_str(), buff);
			listbuf = str_add(listbuf, buf);
			strcpy(buf, i->BanReason.c_str());
			strcat(buf, "\r\n");
			listbuf = str_add(listbuf, buf);
			is_find = true;
		};

	}
	if (is_find) {
		page_string(ch->desc, listbuf, 1);
	} else {
		SendMsgToChar("No sites are banned.\r\n", ch);
	}
	free(listbuf);
}

void BanList::ShowBannedProxy(int sort_mode, CharData *ch) {
	if (proxy_ban_list_.empty()) {
		SendMsgToChar("No proxies are banned.\r\n", ch);
		return;
	}
	SortProxy(sort_mode);
	char format[kMaxInputLength], *listbuf = nullptr;
	strcpy(format, "%-25.25s  %-16.16s\r\n");
	sprintf(buf, format, "Banned Site Name", "Banned By");
	SendMsgToChar(buf, ch);
	sprintf(buf, format, "---------------------------------", "---------------------------------");
	listbuf = str_add(listbuf, buf);
	for (auto &i : proxy_ban_list_) {
		sprintf(buf, format, i->BannedIp.c_str(), i->BannerName.c_str());
		listbuf = str_add(listbuf, buf);
	}
	page_string(ch->desc, listbuf, 1);
}

int BanList::IsBanned(std::string Ip) {
	auto i = std::find_if(proxy_ban_list_.begin(), proxy_ban_list_.end(), [&](const ProxyBanNodePtr &ptr) {
	  return BanList::ProxyBanCompare(ptr, BAN_IP_COMPARE, &Ip);
	});

	if (i != proxy_ban_list_.end())
		return BAN_ALL;

	auto j = std::find_if(ban_list_.begin(), ban_list_.end(), [&](const BanNodePtr &ptr) {
	  return BanList::BanCompare(ptr, BAN_IP_COMPARE, &Ip);
	});

	if (j != ban_list_.end()) {
		if ((*j)->UnbanDate <= time(nullptr)) {
			sprintf(buf, "Site %s is unbaned (time expired).", (*j)->BannedIp.c_str());
			mudlog(buf, NRM, kLvlGod, SYSLOG, true);
			ban_list_.erase(j);
			SaveIp();
			return BAN_NO;
		}
		return (*j)->BanType;
	}
	return BAN_NO;
}

bool BanList::UnbanIp(std::string Ip, CharData *ch) {
	auto i =
		std::find_if(ban_list_.begin(), ban_list_.end(), [&](const BanNodePtr &ptr) {
		  return BanList::BanCompare(ptr, BAN_IP_COMPARE, &Ip);
		});

	if (i != ban_list_.end()) {
		SendMsgToChar("Site unbanned.\r\n", ch);
		sprintf(buf, "%s removed the %s-player ban on %s.",
				GET_NAME(ch), ban_types[(*i)->BanType], (*i)->BannedIp.c_str());
		mudlog(buf, BRF, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s removed the %s-player ban on %s.", GET_NAME(ch),
				ban_types[(*i)->BanType], (*i)->BannedIp.c_str());
		ban_list_.erase(i);
		SaveIp();
		return true;
	}
	return false;
}

bool BanList::UnbanProxy(std::string ProxyIp, CharData *ch) {
	auto i = std::find_if(proxy_ban_list_.begin(), proxy_ban_list_.end(), [&](const ProxyBanNodePtr &ptr) {
	  return BanList::ProxyBanCompare(ptr, BAN_IP_COMPARE, &ProxyIp);
	});

	if (i != proxy_ban_list_.end()) {
		SendMsgToChar("Proxy unbanned.\r\n", ch);
		sprintf(buf, "%s removed the proxy ban on %s.", GET_NAME(ch), (*i)->BannedIp.c_str());
		mudlog(buf, BRF, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s removed the proxy ban on %s.", GET_NAME(ch), (*i)->BannedIp.c_str());
		proxy_ban_list_.erase(i);
		SaveProxy();
		return true;
	}
	return false;
}

bool BanList::Unban(const std::string &Ip, CharData *ch) {
	bool flag1 = false, flag2 = false;
	flag1 = UnbanIp(Ip, ch);
	flag2 = UnbanProxy(Ip, ch);
	if (flag1 || flag2)
		return true;
	return false;
}

bool BanList::EmptyIp() {
	return ban_list_.empty();
}

bool BanList::EmptyProxy() {
	return proxy_ban_list_.empty();
}

void BanList::ClearAll() {
	ban_list_.clear();
	proxy_ban_list_.clear();
}

void BanList::PurgeObsolete() {
	bool purged = false;
	for (auto i = ban_list_.begin(); i != ban_list_.end();) {
		if ((*i)->UnbanDate <= time(nullptr)) {
			ban_list_.erase(i++);
			purged = true;
		} else
			i++;
	}
	if (purged)
		SaveIp();
}

time_t BanList::GetBanDate(std::string Ip) {
	auto i = std::find_if(proxy_ban_list_.begin(), proxy_ban_list_.end(), [&](const ProxyBanNodePtr &ptr) {
	  return BanList::ProxyBanCompare(ptr, BAN_IP_COMPARE, &Ip);
	});

	if (i != proxy_ban_list_.end())
		return -1;    //infinite ban

	auto j = std::find_if(ban_list_.begin(), ban_list_.end(), [&](const BanNodePtr &ptr) {
	  return BanList::BanCompare(ptr, BAN_IP_COMPARE, &Ip);
	});

	if (j != ban_list_.end()) {
		return (*j)->UnbanDate;
	}

	return time(nullptr);

}

void BanList::DisconnectBannedIp(const std::string &Ip) {
	DescriptorData *d;

	for (d = descriptor_list; d; d = d->next) {
		if (d->host == Ip) {
			if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
				return;
			//SendMsgToChar will crash if character has not been loaded/created yet.
			iosystem::write_to_output("Your IP has been banned, disconnecting...\r\n", d);
			if (STATE(d) == CON_PLAYING)
				STATE(d) = CON_DISCONNECT;
			else
				STATE(d) = CON_CLOSE;
		}
	}
}

// contains list of banned ip's and proxies + interface
BanList *&ban = GlobalObjects::ban();

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
