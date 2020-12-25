//
// Created by ubuntu on 23.12.2020.
//
#include <utility>

#include "grp.main.h"

char_info::char_info(int memberUid, CHAR_DATA *member, std::string memberName) : memberUID(
        memberUid), member(member), memberName(std::move(memberName)) {
}

char_info::~char_info() {
    mudlog("[~char_info]", BRF, LVL_IMMORT, SYSLOG, TRUE);
}

MemberManager::MemberManager(CHAR_DATA* leader, grp_ptr group) {
    this->_group = std::move(group);
    this->addMember(leader);
}

MemberManager::~MemberManager() {
    mudlog("~MemberManager", BRF, LVL_IMMORT, SYSLOG, TRUE);
}


void MemberManager::addMember(CHAR_DATA *member) {
    if (IS_NPC(member))
        return;
    if (member->personGroup == this->_group)
        return;
    auto it = this->_memberList.find(member->get_uid());
    if (it == this->_memberList.end()) {
        auto ci = std::shared_ptr<char_info>(new char_info(member->get_uid(), member, member->get_pc_name()));
        this->_memberList.emplace(member->get_uid(), ci);
    }
    else {
        sprintf(buf, "MemberManager::addMember: группа id=%lu, попытка повторного добавления персонажа с тем же uid", this->_group->getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    member->personGroup = this->_group;
    this->_group->getLeader()->send_to_TC(true, false, false, "Размер списка: %lu\r\n", this->_memberList.size());
}

bool MemberManager::restoreMember(CHAR_DATA *member) {
    return false;r
}

bool MemberManager::removeMember(CHAR_DATA *member) {
    if (member == nullptr || member->purged())
        return false;
    if (this->_memberList.empty()) {
        sprintf(buf, "MemberManager::removeMember: попытка удалить из группы при текущем индексе 0, id=%lu", this->_group->getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    auto it = this->_memberList.find(member->get_uid());
    if (it == this->_memberList.end()){
        sprintf(buf, "MemberManager::removeMember: персонаж не найден, id=%lu", this->_group->getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    this->_memberList.erase(it);
    member->personGroup = nullptr;
    return true;
}

bool MemberManager::findMember(int uid) {
    auto it = this->_memberList.find(uid);
    if (it == this->_memberList.end() )
        return false;
    return true;
}

void MemberManager::clear(bool silentMode) {
    sprintf(buf, "[MemberManager::clear()]: _memberList: %lu", _memberList.size());
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