//
// Created by ubuntu on 23.12.2020.
//

#include "handler.h"
#include "global.objects.hpp"
#include "grp/grp.main.h"

extern GroupRoster& groupRoster;

void RequestManager::makeInvite(CHAR_DATA *leader, char *targetPerson) {
    if (AFF_FLAGGED(leader, EAffectFlag::AFF_CHARM))
        return;
    if (AFF_FLAGGED(leader, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(leader, EAffectFlag::AFF_STRANGLED)) {
        send_to_char(leader, "Вы немы, как рыба об лед.\r\n");
        return;
    }
    if (leader->personGroup->getLeader() != leader){
        send_to_char(leader, "Чтобы пригласить в группу, надо быть лидером.\r\n");
        return;
    }
    auto [res, vict] = this->tryMakeInvite(leader->personGroup, targetPerson);
    switch (res) {
        case INV_R_BUSY:
            send_to_char(leader, "Извините, но указанный персонаж уже в группе.\r\n");
            return;
        case INV_R_NO_PERSON:
            send_to_char(leader, "Протри глаза, нет такого игрока!\r\n");
            return;
        case INV_R_REFRESH:
            send_to_char(leader, "Вы продлили приглашение.\r\n");
            return;
        case INV_R_OK:
            send_to_char(leader, "Вы направили приглашение на вступление в группу.\r\n");
            send_to_char(vict, "Лидер группы %s направил вам приглашение в группу.\r\n", leader->get_pc_name().c_str());
            return;
    }
}

std::tuple<INV_R, CHAR_DATA *> RequestManager::tryMakeInvite(grp_ptr grp, char *member) {
    CHAR_DATA *vict = get_player_vis(grp->getLeader(), member, FIND_CHAR_WORLD);
    if (!vict) return std::make_tuple(INV_R_NO_PERSON, nullptr);
    if (vict->personGroup != nullptr ) return std::make_tuple(INV_R::INV_R_BUSY, nullptr);
    for (auto & it : this->_requestList){
        if (vict == it._applicant && grp == it._group){
            it._time = std::chrono::system_clock::now();
            return std::make_tuple(INV_R::INV_R_REFRESH, vict);
        }
    }
    // не нашли, создаём
    this->_requestList.emplace(_requestList.end(), Request(vict, grp, RQ_GROUP));
    return std::make_tuple(INV_R::INV_R_OK, vict);
}

void RequestManager::printRequestList(CHAR_DATA *ch) {
    bool notFound = true;
    if (!ch || ch->purged())
        return;
    send_to_char(ch, "Активные заявки и приглашения:\r\n");
    for (const auto & it : this->_requestList){
        if (ch->get_uid() == it._applicantUID){
            sprintf(smallBuf, "%s%s\r\n",
                    it._type== RQ_TYPE::RQ_PERSON? "Заявка в группу лидера ": "Приглашение от лидера группы ",
                    it._group->getLeaderName().c_str());
        }
        if (ch->personGroup != nullptr && ch->personGroup->getUid() == it._group->getUid()) {
            if (it._type== RQ_TYPE::RQ_GROUP) {
                sprintf(smallBuf, "Приглашение игроку: %s\r\n", it._applicantName.c_str() );
            }
            else {
                sprintf(smallBuf, "Заявка от игрока: %s\r\n", it._applicantName.c_str() );
            }
        }
        send_to_char(ch, "%s", smallBuf);
        notFound = false;
    }
    if (notFound)
        send_to_char(ch, "Заявок нет.\r\n");
}

void RequestManager::makeRequest(CHAR_DATA *author, char* target) {
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
    std::shared_ptr<RequestManager> mgr = groupRoster.getRequestManager();
    auto [res, grp] = mgr->tryAddRequest(author, target);
    switch (res) {
        case RQ_R::RQ_R_OVERFLOW:
            send_to_char("В группе нет свободных мест, свяжитесь с лидером.\r\n", author);
            return;
        case RQ_R::RQ_R_NO_GROUP:
            send_to_char("Протри глаза, не такой группы!\r\n", author);
            return;
        case RQ_R::RQ_R_OK:
            send_to_char("Заявка на вступление в группу отправлена.\r\n", author);
            break;
        case RQ_R::RQ_REFRESH:
            send_to_char("Заявка успешно продлена.\r\n", author);
            break;
    }
    grp->sendToGroup(GRP_COMM_LEADER, "Поступила заявка на вступление в группу. \r\n");
}

void RequestManager::revokeRequest(CHAR_DATA *ch, char* target) {
    for (auto it = this->_requestList.begin(); it != this->_requestList.end(); ++it)
        if (it->_applicant == ch  && str_cmp(target, it->_group->getLeaderName().c_str())){
            send_to_char(ch, "заявка найдена и удалена\r\n");
            _requestList.erase(it);
            return;
        }
    send_to_char(ch, "заявка не найдена\r\n");
}

std::tuple<RQ_R, grp_ptr> RequestManager::tryAddRequest(CHAR_DATA *author, char *targetGroup) {
    if (!author || author->purged()){
        sprintf(buf, "RequestManager::tryAddRequest: author is null or purged\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return std::make_tuple(RQ_R::RQ_R_NO_GROUP, nullptr);
    }
    if (targetGroup == nullptr){
        return std::make_tuple(RQ_R::RQ_R_NO_GROUP, nullptr);
    }
    grp_ptr grp = groupRoster.getGroupManager()->findGroup(targetGroup) ;
    if (grp == nullptr)
        return std::make_tuple(RQ_R::RQ_R_NO_GROUP, nullptr);
    if (grp->isFull())
        return std::make_tuple(RQ_R::RQ_R_OVERFLOW, nullptr);

    for (auto & it : this->_requestList){
        if (author == it._applicant && grp == it._group){
            it._time = std::chrono::system_clock::now();
            return std::make_tuple(RQ_REFRESH, grp);
        }
    }
    // не нашли, создаём
    this->_requestList.emplace(_requestList.end(), Request(author, grp, RQ_PERSON));
    return std::make_tuple(RQ_R::RQ_R_OK, grp);
}

RequestManager::RequestManager() = default;
