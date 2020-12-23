//
// Created by ubuntu on 23.12.2020.
//

#ifndef BYLINS_GRP_REQUESTMANAGER_H
#define BYLINS_GRP_REQUESTMANAGER_H

#include "grp.request.h"
#include "chars/char.hpp"

enum RQ_R {RQ_R_OK, RQ_R_NO_GROUP, RQ_R_OVERFLOW, RQ_REFRESH};
enum INV_R {INV_R_OK, INV_R_NO_PERSON, INV_R_BUSY, INV_R_REFRESH};

class RequestManager{
private:
    std::list<Request> _requestList;
    std::tuple<INV_R, CHAR_DATA *> tryMakeInvite(Group* grp, char* member);
    std::tuple<RQ_R, Group *> tryAddRequest(CHAR_DATA* author, char* targetGroup);
public:
    RequestManager();
// методы работы с заявками
    void printRequestList(CHAR_DATA* ch);
    void makeInvite(CHAR_DATA* leader, char* targetPerson);
    void makeRequest(CHAR_DATA* author, char* targetGroup);
    void revokeRequest(CHAR_DATA* author, char* targetGroup);
};

#endif //BYLINS_GRP_REQUESTMANAGER_H
