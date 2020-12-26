
#include <memory>

#include "comm.h"
#include "handler.h"
#include "msdp.constants.hpp"
//#include "global.objects.hpp"
#include "grp.main.h"


extern GroupRoster& groupRoster;

Group::Group(CHAR_DATA *leader, u_long uid){
    _leaderUID = 0; // чтобы не ругался clion =)
    _memberCap = 0;
    _uid = uid;
    addMember(leader);
    _setLeader(leader);
}

CHAR_DATA *Group::getLeader() {
    return _leader;
}

void Group::_setLeader(CHAR_DATA *leader) {
    _leaderUID = leader->get_uid();
    _leader = leader;
    _leaderName.assign(leader->get_pc_name());
    _memberCap = IS_NPC(leader)? 255 : max_group_size(leader);
}

bool Group::_isActive() {
    return false;
}

u_long Group::getUid() const {
    return _uid;
}

u_short Group::getCurrentMemberCount() const {
    return (u_short)_memberList.size();
}

int Group::_getMemberCap() const {
    return _memberCap;
}

void Group::printGroup(CHAR_DATA *requestor) {

}

const std::string &Group::getLeaderName() const {
    return _leaderName;
}

bool Group::_isFull() {
    if (_memberList.size() >= _memberCap)
        return true;
    return false;
}

void Group::sendToGroup(GRP_COMM mode, const char *msg, ...) {


}

Group::~Group() {
    mudlog("~Group", BRF, LVL_IMMORT, SYSLOG, TRUE);
}

char_info::char_info(int uid, CHAR_DATA *m, std::string name) {
    memberUID = uid;
    member = m;
    memberName.assign(name);
}

char_info::~char_info() {
    mudlog("[~char_info]", BRF, LVL_IMMORT, SYSLOG, TRUE);
}

void Group::addMember(CHAR_DATA *member) {
    if (IS_NPC(member))
        return;
    if (member->personGroup == this)
        return;
    auto it = _memberList.find(member->get_uid());
    if (it == _memberList.end()) {
        auto ci = std::make_shared<char_info>(member->get_uid(), member, member->get_pc_name());
        _memberList.emplace(member->get_uid(), ci);
    }
    else {
        sprintf(buf, "Group::addMember: группа id=%lu, попытка повторного добавления персонажа с тем же uid", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    member->personGroup = this;
//    getLeader()->send_to_TC(true, false, false, "Размер списка: %lu\r\n", _memberList.size());

}

bool Group::restoreMember(CHAR_DATA *member) {
    return false;
}

bool Group::removeMember(CHAR_DATA *member) {
    if (member == nullptr || member->purged())
        return false;
    if (_memberList.empty()) {
        sprintf(buf, "Group::removeMember: попытка удалить из группы при текущем индексе 0, id=%lu", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    auto it = _memberList.find(member->get_uid());
    if (it == _memberList.end()){
        sprintf(buf, "Group::removeMember: персонаж не найден, id=%lu", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    _memberList.erase(it);
    member->personGroup = nullptr;
    return true;
}

bool Group::_isMember(int uid) {
    auto it = _memberList.find(uid);
    if (it == _memberList.end() )
        return false;
    return true;
}

void Group::clear(bool silentMode) {
    sprintf(buf, "[Group::clear()]: _memberList: %lu", _memberList.size());
    mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
    CHAR_DATA* ch;
    for (auto & it : _memberList) {
        ch = it.second->member;
        if (ch == nullptr || ch->purged())
            continue;
        if (!silentMode)
            send_to_char(ch, "Ваша группа распущена.\r\n");
        ch->personGroup = nullptr;
    }
    _memberList.clear();
}

void Group::promote(char *applicant) {
    int memberId = _findMember(applicant);
    if (memberId == 0) {
        sendToGroup(GRP_COMM_LEADER, "Нет такого персонажа.");
        return;
    }
    _setLeader(_memberList.at(memberId)->member);
    sendToGroup(GRP_COMM_ALL, "Изменился лидер группы на %.", _memberList.at(memberId)->memberName.c_str());
    if (u_short diff = _memberCap - (int)_memberList.size() >0){
        u_short list[diff]; u_short i = 0;
        for (auto it = _memberList.rend(); it != _memberList.rbegin(); --it){
            ++i;
            if (it->second->member == _leader )
                continue;
            list[i-1] = it->second->memberUID;
            if (diff == i)
                break;
        }
        for (i = 0; i <= diff; i++)
            removeMember(_memberList.find(list[i])->second->member);
    }
}

int Group::_findMember(char *memberName) {
    for (auto & it : _memberList) {
        if (!str_cmp(it.second->memberName, memberName))
            return it.first;
    }
    return 0;
}

void Group::approveRequest(char *applicant) {

}
