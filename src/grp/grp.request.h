#ifndef BYLINS_GRP_REQUEST_H
#define BYLINS_GRP_REQUEST_H

#include "chars/char.hpp"

enum RQ_TYPE {RQ_GROUP, RQ_PERSON};

class Request {
public:
    std::chrono::time_point<std::chrono::system_clock> _time;
    CHAR_DATA *_applicant;
    Group *_group;
    std::string _applicantName;
    int _applicantUID;
    RQ_TYPE _type;
    Request(CHAR_DATA* subject, Group* group, RQ_TYPE type);
};


#endif //BYLINS_GRP_REQUEST_H
