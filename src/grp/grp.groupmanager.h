//
// Created by ubuntu on 23.12.2020.
//

#ifndef BYLINS_GRP_GROUPMANAGER_H
#define BYLINS_GRP_GROUPMANAGER_H

class GroupRosterRecord {
public:
    explicit GroupRosterRecord(Group * group);
    Group *member;
};


class GroupManager{
private:
    u_long _currentGroupIndex = 0;
    std::map<u_long, GroupRosterRecord*> _groupList;
public:
    Group* addGroup(CHAR_DATA* leader);
    void removeGroup(u_long uid);
    Group* findGroup(CHAR_DATA* ch);
    Group* findGroup(char* leaderName);
};

#endif //BYLINS_GRP_GROUPMANAGER_H
