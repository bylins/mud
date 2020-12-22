//
// Created by ubuntu on 17/12/20.
//

#include "grp.roster.h"
#include "global.objects.hpp"
#include "handler.h"

GroupRoster& groupRoster = GlobalObjects::groupRoster();
GroupRoster::GroupRoster() {
    this->_currentGroupIndex = 0;
}

Request::Request(CHAR_DATA *author, Group *group) {
    if (!author || !group)
        return;
    _applicant = author;
    _applicantName = author->get_pc_name();
    _applicantUID = author->get_uid();
    _group = group;
    if (author == group->getLeader())
        _type = RQ_GROUP;
    else
        _type = RQ_PERSON;
}

Group* GroupRoster::addGroup(CHAR_DATA * leader) {
    ++this->_currentGroupIndex;
    auto *group = new Group(leader, _currentGroupIndex);
    this->_groupList.emplace(_currentGroupIndex, new GroupRosterRecord(group));
    return group;
}

void GroupRoster::removeGroup(u_long uid) {
    auto grp = _groupList.find(uid);
    if (grp != _groupList.end())
    {
        delete grp->second->member;
        _groupList.erase(uid);
    }
}

void GroupRoster::recalculateRoster() {

}

void GroupRoster::restorePlayerGroup(CHAR_DATA *ch) {
    for (auto & it : this->_groupList){
        if(it.second->member->checkMember(ch->get_uid())){
            ch->personGroup = it.second->member;
            ch->send_to_TC(true, false, true, "Restore group: %d", ch->personGroup->getUid());
            break;
        }
    }
}


GroupRosterRecord::GroupRosterRecord(Group *group) {
    this->member = group;
}

void GroupRoster::printRequestList(CHAR_DATA *ch) {
    bool notFound = true;
    if (!ch || ch->purged())
        return;
    send_to_char(ch, "Активные заявки и приглашения:\r\n");
    for (const auto & it : this->_requestList){
        if (ch->get_uid() == it._applicantUID)
            sprintf(smallBuf, "%s%s\r\n",
                    it._type== RQ_TYPE::RQ_PERSON? "Заявка в группу лидера ": "Приглашение от лидера группы ",
                    it._group->getLeaderName().c_str());
        if (ch->personGroup != nullptr && ch->personGroup->getUid() == it._group->getUid())
            sprintf(smallBuf, "Приглашение игроку: %s\r\n", it._applicantName.c_str() );
        send_to_char(ch, "%s", smallBuf);
        notFound = false;
        }
        if (notFound)
            send_to_char(ch, "Заявок нет.\r\n");
}

void GroupRoster::makeRequest(CHAR_DATA *author, char* target) {
    if (AFF_FLAGGED(author, EAffectFlag::AFF_CHARM))
        return;

    if (AFF_FLAGGED(author, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(author, EAffectFlag::AFF_STRANGLED)) {
        send_to_char(author, "Вы немы, как рыба об лед.\r\n");
        return;
    }
    if (author->personGroup && author->personGroup->getLeader() == author) {
        send_to_char(author, "Негоже лидеру группы напрашиваться в чужую!\r\n");
        return;
    }

    if (!*target) {
        send_to_char("И кому же вы хотите напроситься в группу?.\r\n", author);
        return;
    }

    switch (groupRoster.tryAddRequest(author, target)) {
        case OK_SUCCESS:
            send_to_char("Заявка на вступление в группу отправлена.\r\n", author);
            return;
        case GROUP_OVERFLOW:
            send_to_char("В группе нет свободных мест, свяжитесь с лидером.\r\n", author);
            return;
        case NO_GROUP:
            send_to_char("Протри глаза, не такой группы!\r\n", author);
            return;
        case RQ_REFRESH:
            send_to_char("Заявка успешно продлена.\r\n", author);
    }
}

void GroupRoster::revokeRequest(CHAR_DATA *ch, char* target) {
    if (ch->personGroup != nullptr && ch->personGroup->getLeader() == ch) {
        // вызов от имени лидера, отменяем заявку лидера на присоединение к группе
        send_to_char(ch, "вызов от имени лидера, отменяем заявку лидера на присоединение к группе\r\n");
        return;
    }
    for (auto it = this->_requestList.begin(); it != this->_requestList.end(); ++it)
        if (it->_applicant == ch  && str_cmp(target, it->_group->getLeaderName().c_str())){
            send_to_char(ch, "заявка найдена и удалена\r\n");
            _requestList.erase(it);
            return;
        }
    send_to_char(ch, "заявка не найдена\r\n");
}

void GroupRoster::makeRequest(char *targetPerson) {

}

RQ_RESULT GroupRoster::tryAddRequest(CHAR_DATA *author, char *targetGroup) {
    Group* grp = nullptr;
    for (auto & it : this->_groupList) {
        if (str_cmp(it.second->member->getLeaderName().c_str(), targetGroup))
            grp = it.second->member;
    }
    if (grp == nullptr)
        return RQ_RESULT::NO_GROUP;
    if (grp->isFull())
        return RQ_RESULT::GROUP_OVERFLOW;

    for (auto & it : this->_requestList){
        if (author == it._applicant && grp == it._group){
            it.time = std::chrono::system_clock::now();
            return RQ_REFRESH;
        }
    }
    // не нашли, создаём
    this->_requestList.emplace(_requestList.end(), Request(author, grp));
    return RQ_RESULT::OK_SUCCESS;
}


void do_grequest(CHAR_DATA *ch, char *argument, int, int){
    char subcmd[MAX_INPUT_LENGTH], target[MAX_INPUT_LENGTH];
    enum SUBCMD {LIST, MAKE, CANCEL};
    RQ_RESULT res;
    // гзаявка - список
    // гзаявка создать Верий - отправляет заявку в группу
    // гзаявка отменить Верий - отменяет заявку в группу

    two_arguments(argument, subcmd, target);

    if (!*subcmd || isname(subcmd, "список list"))
    {
        groupRoster.printRequestList(ch);
        return;
    }
    else if (isname(subcmd, "создать отправить make send")) {
        groupRoster.makeRequest(ch, target);
        return;
    }
    else if (isname(subcmd, "отменить отозвать cancel revoke")){
        groupRoster.revokeRequest(ch, target);
        return;
    }
    else {
        send_to_char("Уточните команду.\r\n", ch);
        return;
    }

}
