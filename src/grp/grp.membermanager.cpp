//
// Created by ubuntu on 23.12.2020.
//

#include "grp.membermanager.h"

void MemberManager::addMember(CHAR_DATA *member) {
    if (IS_NPC(member))
        return;
    if (member->personGroup == this->_group)
        return;
    auto it = this->_memberList.find(member->get_uid());
    if (it == this->_memberList.end())
        this->_memberList.emplace(member->get_uid(), new char_info(member->get_uid(),
                                                                   member,
                                                                   member->get_pc_name()) );
    else {
        sprintf(buf, "ROSTER: группа id=%lu, попытка повторного добавления персонажа с тем же uid", this->_group->getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    member->personGroup = this->_group;
}

bool MemberManager::restoreMember(CHAR_DATA *member) {
    return false;
}
