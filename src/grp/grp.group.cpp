
#include "comm.h"
#include "handler.h"
#include "magic.h"
#include "msdp.constants.hpp"
#include "screen.h"
#include "global.objects.hpp"
#include "grp.main.h"


extern GroupRoster& groupRoster;

Group::Group(CHAR_DATA *leader, u_long uid){
    this->_memberCap = IS_NPC(leader)? 255 : max_group_size(leader);
    this->_uid = uid;
    this->setLeader(leader);
    this->_memberManager =  mm_ptr (new MemberManager(leader, grp_ptr(this)));
    _memberManager->addMember(leader);

}

CHAR_DATA *Group::getLeader() {
    return _leader;
}

void Group::setLeader(CHAR_DATA *leader) {
    this->_leaderUID = leader->get_uid();
    _leader = leader;
    _leaderName = leader->get_pc_name();
}

bool Group::isActive() {
    return false;
}

u_long Group::getUid() const {
    return _uid;
}

u_short Group::getCurrentMemberCount() const {
    return _currentMemberCount;
}

int Group::getMemberCap() const {
    return _memberCap;
}

void Group::printGroup(CHAR_DATA *requestor) {

}


const std::string &Group::getLeaderName() const {
    return _leaderName;
}

bool Group::isFull() {
    if (this->_memberManager->getMemberCount() == this->_memberCap)
        return true;
    return false;
}

void Group::sendToGroup(GRP_COMM mode, const char *msg, ...) {


}

void Group::disband(bool silentMode) {
    _memberManager->clear(silentMode);
};

Group::~Group() {
    mudlog("[~Group]", BRF, LVL_IMMORT, SYSLOG, TRUE);
}