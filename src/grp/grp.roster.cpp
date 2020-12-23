//
// Created by ubuntu on 17/12/20.
//

#include "grp.roster.h"
#include "global.objects.hpp"
#include "handler.h"

GroupRoster& groupRoster = GlobalObjects::groupRoster();

GroupRoster::GroupRoster() {
    this->_requestManager = new RequestManager();
    this->_groupManager = new GroupManager();
}

void GroupRoster::restorePlayerGroup(CHAR_DATA *ch) {
    Group* grp = this->_groupManager->findGroup(ch);
    if (grp == nullptr)
        return;
    if (!grp->getMemberManager()->restoreMember(ch))
        return;
    grp->sendToGroup(GRP_COMM_ALL, "Игрок %s заново присоединился к группе.\r\n", ch->get_pc_name().c_str());
    ch->send_to_TC(true, false, true, "Membership in group: %d restored\r\n", ch->personGroup->getUid());
}


GroupRosterRecord::GroupRosterRecord(Group *group) {
    this->member = group;
}

