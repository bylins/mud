#ifndef BYLINS_GRP_MEMBER_H
#define BYLINS_GRP_MEMBER_H

#include "chars/char.hpp"

class groupMember {
public:
    groupMember();

private:
    CHAR_DATA *ch;
    bool isLeader;
};

#endif //BYLINS_GRP_MEMBER_H
