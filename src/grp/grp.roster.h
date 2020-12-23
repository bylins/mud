//
// Created by ubuntu on 17/12/20.
//

#ifndef BYLINS_GRP_ROSTER_H
#define BYLINS_GRP_ROSTER_H

#include "chars/char.hpp"
#include "grp.group.h"
#include "grp.requestmanager.h"
#include "grp.groupmanager.h"


void do_grequest(CHAR_DATA *ch, char *argument, int, int);

class GroupRoster {
private: // fields

private: // methods

// properties
private:
    RequestManager* _requestManager;
    GroupManager* _groupManager;
public:
    GroupManager* getGroupManager() const {return _groupManager;}
    RequestManager* getRequestManager() const {return _requestManager;}
public:
    GroupRoster();
    void restorePlayerGroup(CHAR_DATA* ch); // возвращает игрока в группу после смерти
};

#endif //BYLINS_GRP_ROSTER_H
