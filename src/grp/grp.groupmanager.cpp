//
// Created by ubuntu on 23.12.2020.
//
#include "grp.main.h"

grp_ptr GroupManager::addGroup(CHAR_DATA * leader) {
    ++this->_currentGroupIndex;
    auto group = grp_ptr (new Group(leader, _currentGroupIndex)) ;
    this->_groupList.emplace(_currentGroupIndex, group);
    leader->send_to_TC(true, false, false, "addGroup: Размер списка: %lu\r\n", this->_groupList.size());
    return group;
}

void GroupManager::removeGroup(u_long uid) {
    auto grp = _groupList.find(uid);
    if (grp != _groupList.end()) {
        grp->second->disband(false);
        _groupList.erase(uid);
        return;
    }
}

grp_ptr GroupManager::findGroup(CHAR_DATA *ch) {
    if (ch == nullptr || ch->purged()){
        sprintf(buf, "GroupManager::findGroup: вызов для пустого или пурженого персонажа\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList){
        auto mgr = it.second->getMemberManager();
        if (mgr->findMember(ch->get_uid()))
            return it.second;
        }
    return nullptr;
}

grp_ptr GroupManager::findGroup(char *leaderName) {
    if (leaderName == nullptr){
        sprintf(buf, "GroupManager::findGroup: leaderName is null\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList) {
        if (str_cmp(it.second->getLeaderName().c_str(), leaderName))
            return it.second;
    }
    return nullptr;
}


void GroupManager::printList(CHAR_DATA *ch) {
    size_t cnt = this->_groupList.size();
    send_to_char(ch, "Текущее количество групп в мире: %lu\r\n", cnt);
    for (auto & it : this->_groupList) {
        send_to_char(ch, "Группа лидера %s, кол-во участников: %hu\r\n",
                                it.second->getLeaderName().c_str(),
                                it.second->getCurrentMemberCount());
    }
}
