//
// Created by ubuntu on 23.12.2020.
//

#ifndef BYLINS_GRP_MEMBERMANAGER_H
#define BYLINS_GRP_MEMBERMANAGER_H

#include "chars/char.hpp"
#include "grp.roster.h"

class MemberManager {
private:
    struct char_info {
        int memberUID;
        char_info(int memberUid, CHAR_DATA *member, const std::string &memberName) : memberUID(memberUid),
                                                                                     member(member),
                                                                                     memberName(memberName) {}
        CHAR_DATA * member;
        std::string memberName;
    };
    std::map<int, char_info *> _memberList;
    Group * _group;
public:
    MemberManager(const std::map<int, char_info *> &memberList);
    void addMember(CHAR_DATA *member);
    void removeMember(CHAR_DATA *member);
    bool findMember(int uid);
    bool restoreMember(CHAR_DATA *member);
};

#endif //BYLINS_GRP_MEMBERMANAGER_H
