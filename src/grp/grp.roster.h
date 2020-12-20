//
// Created by ubuntu on 17/12/20.
//

#ifndef BYLINS_GRP_ROSTER_H
#define BYLINS_GRP_ROSTER_H

#include "chars/char.hpp"
#include "grp.group.h"

class GroupRosterRecord {
public:
    explicit GroupRosterRecord(Group * group);
    Group *member;
    bool needRecalc = false;
};


class GroupRoster {
private:
    u_long _currentGroupIndex = 0;
    std::map<u_long, GroupRosterRecord *> _groupList;
public:
    GroupRoster();
    Group* addGroup(CHAR_DATA * leader);
    u_long getSize() {return _groupList.size();}
    void removeGroup(u_long uid);
    void restorePlayerGroup(CHAR_DATA *ch);
    void recalculateRoster();
};

#endif //BYLINS_GRP_ROSTER_H
