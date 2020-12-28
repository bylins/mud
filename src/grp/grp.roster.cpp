//
// Created by ubuntu on 17/12/20.
//

#include "global.objects.hpp"
#include "handler.h"
#include "grp.main.h"


GroupRoster& groupRoster = GlobalObjects::groupRoster();

GroupRoster::GroupRoster() {

}

void GroupRoster::restorePlayerGroup(CHAR_DATA *ch) {
    auto grp = this->findGroup(ch);
    if (grp == nullptr)
        return;
    if (!grp->restoreMember(ch))
        return;
    grp->sendToGroup(GRP_COMM_ALL, "Игрок %s заново присоединился к группе.\r\n", ch->get_pc_name().c_str());
    ch->send_to_TC(true, false, true, "Membership in group: %d restored\r\n", ch->personGroup->getUid());
}

void GroupRoster::processGroupCommands(CHAR_DATA *ch, char *argument) {

    const std::string strHELP = "справка помощь";
    const std::string strMAKE = "создать make";
    const std::string strLIST = "список list";
    const std::string strINVITE = "пригласить invite";
    const std::string strTAKE = "принять approve";
    const std::string strREJECT = "отклонить reject";
    const std::string strEXPELL = "выгнать expell";
    const std::string strLEADER = "лидер leader";
    const std::string strLEAVE = "покинуть leave";
    const std::string strDISBAND = "распустить clear";
    const std::string strALL = "все all";
    const std::string strTEST = "тест test";
    char subcmd[MAX_INPUT_LENGTH],  target[MAX_INPUT_LENGTH];

    if (!ch || IS_NPC(ch))
        return;

    two_arguments(argument, subcmd, target);
    if (GET_POS(ch) < POS_RESTING) {
        send_to_char("Трудно управлять группой в таком состоянии.\r\n", ch);
        return;
    }
    auto grp = ch->personGroup;

    // выбираем режим
    if (!*subcmd) {
        grp->printGroup(ch); return;}
    else if (isname(subcmd, strALL.c_str())) {
        if (IS_IMMORTAL(ch))
            groupRoster.printList(ch);
        return;
    }
    else if (isname(subcmd, strHELP.c_str())) {
        send_to_char(ch, "Команды: %s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
                     strHELP.c_str(), strMAKE.c_str(), strLIST.c_str(),strINVITE.c_str(), strTAKE.c_str(),
                     strREJECT.c_str(), strEXPELL.c_str(), strLEADER.c_str(), strLEAVE.c_str(), strDISBAND.c_str());
        return;
    }
    else if (isname(subcmd, strMAKE.c_str())) {
        if (grp != nullptr && grp->getLeader() == ch){
            send_to_char(ch, "Только великим правителям, навроде Цесаря Иулия, было дозволено водить много легионов!\r\n");
            return;
        }
        if (grp != nullptr) // в группе - покидаем
            grp->_removeMember(ch);
        groupRoster.addGroup(ch);
        send_to_char(ch, "Вы создали группу c максимальным числом последователей %d.\r\n", ch->personGroup ? ch->personGroup->_getMemberCap() : 0);
         return;
    }
    else if (isname(subcmd, strLIST.c_str())) {
        grp->listMembers(ch);
        return;
    }
    else if (isname(subcmd, strLEAVE.c_str())){
        grp->leaveGroup();
        return;
    }


    if (grp != nullptr && grp->getLeader() != ch ) {
        send_to_char(ch, "Необходимо быть лидером группы для этого.\r\n");
        return;
    }

    if (isname(subcmd, strINVITE.c_str())) {
        if (grp->_isFull()){
            send_to_char(ch, "Командир, но в твоей группе больше нет места!\r\n");
            return;
        }
        groupRoster.makeInvite(ch, target);
        return;
        }
    else if (isname(subcmd, strTAKE.c_str())) {
        grp->approveRequest(target);
    }
    else if (isname(subcmd, strREJECT.c_str())){
        grp->rejectRequest(target);
        return;
    }
    else if (isname(subcmd, strEXPELL.c_str())){
        grp->expellMember(target);
        return;
    }
    else if (isname(subcmd, strLEADER.c_str())) {
        grp->promote(target);
        return;
    }
    else if (isname(subcmd, strDISBAND.c_str())) {
        groupRoster.removeGroup(grp->getUid());
        grp = nullptr;
        return;
    }
    else {
        send_to_char("Уточните команду.\r\n", ch);
        return;
    }

}


grp_ptr GroupRoster::addGroup(CHAR_DATA * leader) {
    if (leader == nullptr || leader->purged())
        return nullptr;
    ++this->_currentGroupIndex;
    auto group = std::make_shared<Group>(leader, _currentGroupIndex) ;
    this->_groupList.emplace(_currentGroupIndex, group);
    leader->send_to_TC(true, false, false, "addGroup: Размер списка: %lu\r\n", this->_groupList.size());
    return group;
}

void GroupRoster::removeGroup(u_long uid) {
    auto grp = _groupList.find(uid);
    if (grp != _groupList.end()) {
        _groupList.erase(uid);
        return;
    }
}

grp_ptr GroupRoster::findGroup(CHAR_DATA *ch) {
    if (ch == nullptr || ch->purged()){
        sprintf(buf, "GroupRoster::findGroup: вызов для пустого или пурженого персонажа\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList){
        if (it.second->_isMember(ch->get_uid()))
            return it.second;
    }
    return nullptr;
}

grp_ptr GroupRoster::findGroup(char *leaderName) {
    if (leaderName == nullptr){
        sprintf(buf, "GroupRoster::findGroup: leaderName is null\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList) {
        if (str_cmp(it.second->getLeaderName().c_str(), leaderName))
            return it.second;
    }
    return nullptr;
}


void GroupRoster::printList(CHAR_DATA *ch) {
    if (ch == nullptr)
        return;
    size_t cnt = this->_groupList.size();
    send_to_char(ch, "Текущее количество групп в мире: %lu\r\n", cnt);
    for (auto & it : this->_groupList) {
        send_to_char(ch, "Группа лидера %s, кол-во участников: %hu\r\n",
                     it.second->getLeaderName().c_str(),
                     it.second->getCurrentMemberCount());
    }
}

void GroupRoster::makeInvite(CHAR_DATA *leader, char *targetPerson) {
    if (!leader || AFF_FLAGGED(leader, EAffectFlag::AFF_CHARM))
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

std::tuple<INV_R, CHAR_DATA *> GroupRoster::tryMakeInvite(Group* grp, char *member) {
    // проверки
    CHAR_DATA *vict = get_player_vis(grp->getLeader(), member, FIND_CHAR_WORLD);
    if (!vict) return std::make_tuple(INV_R_NO_PERSON, nullptr);
    if (vict->personGroup != nullptr ) return std::make_tuple(INV_R::INV_R_BUSY, nullptr);
    // ищем и продлеваем
    for (auto & it : this->_requestList){
        if (vict == it->_applicant && grp == it->_group){
            it->_time = std::chrono::system_clock::now();
            return std::make_tuple(INV_R::INV_R_REFRESH, vict);
        }
    }
    // не нашли, создаём
    rq_ptr inv = std::make_shared<Request>(vict, grp, RQ_GROUP);
    this->_requestList.emplace(_requestList.end(), inv);
    return std::make_tuple(INV_R::INV_R_OK, vict);
}

void GroupRoster::printRequestList(CHAR_DATA *ch) {
    bool notFound = true;
    if (!ch)
        return;
    send_to_char(ch, "Активные заявки и приглашения:\r\n");
    for (const auto & it : this->_requestList){
        if (ch->get_uid() == it->_applicantUID){
            sprintf(smallBuf, "%s%s\r\n",
                    it->_type== RQ_TYPE::RQ_PERSON? "Заявка в группу лидера ": "Приглашение от лидера группы ",
                    it->_group->getLeaderName().c_str());
        }
        if (ch->personGroup != nullptr && ch->personGroup->getUid() == it->_group->getUid()) {
            if (it->_type== RQ_TYPE::RQ_GROUP) {
                sprintf(smallBuf, "Приглашение игроку: %s\r\n", it->_applicantName.c_str() );
            }
            else {
                sprintf(smallBuf, "Заявка от игрока: %s\r\n", it->_applicantName.c_str() );
            }
        }
        send_to_char(ch, "%s", smallBuf);
        notFound = false;
    }
    if (notFound)
        send_to_char(ch, "Заявок нет.\r\n");
}

void GroupRoster::makeRequest(CHAR_DATA *author, char* target) {
    if (!author || AFF_FLAGGED(author, EAffectFlag::AFF_CHARM))
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
    auto [res, grp] = groupRoster.tryAddRequest(author, target);
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

void GroupRoster::revokeRequest(CHAR_DATA *ch, char* target) {
    if (!ch)
        return;
    for (auto it = this->_requestList.begin(); it != this->_requestList.end(); ++it)
        if ( it->get()->_applicant == ch  && str_cmp(target, it->get()->_group->getLeaderName().c_str())){
            send_to_char(ch, "Вы отменили заявку на вступление в группу.\r\n");
            this->_requestList.erase(it);
            return;
        }
    send_to_char(ch, "Заявка не найдена.\r\n");
}

std::tuple<RQ_R, grp_ptr> GroupRoster::tryAddRequest(CHAR_DATA *author, char *targetGroup) {
    // проверки
    if (!author || author->purged()){
        sprintf(buf, "GroupRoster::tryAddRequest: author is null or purged\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return std::make_tuple(RQ_R::RQ_R_NO_GROUP, nullptr);
    }
    if (targetGroup == nullptr){
        return std::make_tuple(RQ_R::RQ_R_NO_GROUP, nullptr);
    }
    // ищем группу и проверяем на всякое
    grp_ptr grp = groupRoster.findGroup(targetGroup) ;
    if (grp == nullptr)
        return std::make_tuple(RQ_R::RQ_R_NO_GROUP, nullptr);
    if (grp->_isFull())
        return std::make_tuple(RQ_R::RQ_R_OVERFLOW, nullptr);
    // продлеваем
    for (auto & it : this->_requestList){
        if (author == it->_applicant && grp.get() == it->_group){
            it->_time = std::chrono::system_clock::now();
            return std::make_tuple(RQ_REFRESH, grp);
        }
    }
    // не нашли, создаём
    rq_ptr rq = std::make_shared<Request>(author, grp.get(), RQ_PERSON);
    this->_requestList.emplace(_requestList.end(), rq);
    return std::make_tuple(RQ_R::RQ_R_OK, grp);
}

Request *GroupRoster::findRequest(const char* targetPerson, const char* group, RQ_TYPE type) {
    for (auto & it : this->_requestList){
        if (type == it->_type &&
            !str_cmp(group, it->_group->getLeaderName().c_str()) &&
            !str_cmp(targetPerson, it->_applicantName))
            return it.get();
    }
    return nullptr;
}

void GroupRoster::deleteRequest(Request *r) {
    for (auto it = this->_requestList.begin(); it != this->_requestList.end(); ++it)
        if ( it->get() == r){
            this->_requestList.erase(it);
            return;
        }

}
// who - игрок,
// author - имя лидера (может быть офлайн)
void GroupRoster::acceptInvite(CHAR_DATA* who, char* author) {
    if (!who)
        return;
    auto r = findRequest(who->get_pc_name().c_str(), author, RQ_TYPE::RQ_GROUP);
    r->_group->addMember(r->_applicant);
    send_to_char(r->_applicant, "Вы одобрили заявку от лидера %s\r\n", r->_group->getLeaderName().c_str());
    deleteRequest(r);
}
