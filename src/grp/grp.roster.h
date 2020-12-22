//
// Created by ubuntu on 17/12/20.
//

#ifndef BYLINS_GRP_ROSTER_H
#define BYLINS_GRP_ROSTER_H

#include "chars/char.hpp"
#include "grp.group.h"

enum RQ_RESULT {OK_SUCCESS, NO_GROUP, GROUP_OVERFLOW, RQ_REFRESH};
enum RQ_TYPE {RQ_GROUP, RQ_PERSON};

void do_grequest(CHAR_DATA *ch, char *argument, int, int);

class GroupRosterRecord {
public:
    explicit GroupRosterRecord(Group * group);
    Group *member;
};

class Request {
public:
    CHAR_DATA *_applicant;
    std::string _applicantName;
    int _applicantUID;
    Group *_group;
    RQ_TYPE _type;
    Request(CHAR_DATA* author, Group* group);
    std::chrono::time_point<std::chrono::system_clock> time;
};

class GroupRoster {
private:
    u_long _currentGroupIndex = 0;
    std::map<u_long, GroupRosterRecord*> _groupList;
    std::list<Request> _requestList;
    RQ_RESULT tryAddRequest(CHAR_DATA* author, char* targetGroup);
public:
    GroupRoster();
    u_long getSize() {return _groupList.size();}
    void restorePlayerGroup(CHAR_DATA* ch);
    void recalculateRoster();

    // манипуляция группой
    Group* addGroup(CHAR_DATA* leader);
    void removeGroup(u_long uid);

    // методы работы с заявками
    void printRequestList(CHAR_DATA* ch);
    void makeRequest(char* targetPerson);
    void makeRequest(CHAR_DATA* author, char* targetGroup);
    void revokeRequest(CHAR_DATA* author, char* targetGroup);
};

#endif //BYLINS_GRP_ROSTER_H
