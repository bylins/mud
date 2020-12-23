//
// Created by ubuntu on 23.12.2020.
//

#include "grp.groupmanager.h"

Group* GroupManager::addGroup(CHAR_DATA * leader) {
    ++this->_currentGroupIndex;
    auto *group = new Group(leader, _currentGroupIndex);
    this->_groupList.emplace(_currentGroupIndex, new GroupRosterRecord(group));
    return group;
}

void GroupManager::removeGroup(u_long uid) {
    auto grp = _groupList.find(uid);
    if (grp != _groupList.end())
    {
        delete grp->second->member;
        _groupList.erase(uid);
    }
}

Group *GroupManager::findGroup(CHAR_DATA *ch) {
    if (ch == nullptr || ch->purged()){
        sprintf(buf, "GroupManager::findGroup: вызов для пустого или пурженого персонажа\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList){
        auto mgr = it.second->member->getMemberManager();
        if (mgr->findMember(ch->get_uid()))
            return it.second->member;
        }
    return nullptr;
}

Group *GroupManager::findGroup(char *leaderName) {
    if (leaderName == nullptr){
        sprintf(buf, "GroupManager::findGroup: leaderName is null\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList) {
        if (str_cmp(it.second->member->getLeaderName().c_str(), leaderName))
            return it.second->member;
    }
    return nullptr;
}
