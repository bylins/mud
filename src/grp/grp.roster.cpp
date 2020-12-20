//
// Created by ubuntu on 17/12/20.
//

#include "grp.roster.h"
#include "global.objects.hpp"

GroupRoster& groupRoster = GlobalObjects::groupRoster();

GroupRoster::GroupRoster() {
    this->_currentGroupIndex = 0;
}

Group* GroupRoster::addGroup(CHAR_DATA * leader) {
    ++this->_currentGroupIndex;
    auto *group = new Group(leader, _currentGroupIndex);
    this->_groupList.emplace(_currentGroupIndex, new GroupRosterRecord(group));
    leader->personGroup = group; // прописываем сразу обратную ссылку персонажа на группу
    return group;
}

void GroupRoster::removeGroup(u_long uid) {
    auto grp = _groupList.find(uid);
    delete grp->second->member;
    _groupList.erase(uid);
}

void GroupRoster::recalculateRoster() {
    return;
}

void GroupRoster::restorePlayerGroup(CHAR_DATA *ch) {
    for (auto & it : this->_groupList){
        if(it.second->member->checkMember(ch->get_uid())){
            ch->personGroup = it.second->member;
            ch->send_to_TC(true, false, true, "Restore group: %d", ch->personGroup->getUid());
            break;
        }
    }
}


GroupRosterRecord::GroupRosterRecord(Group *group) {
    this->member = group;
    this->needRecalc = false;
}
