//
// Created by ubuntu on 17/12/20.
//

#include "global.objects.hpp"
#include "handler.h"

GroupRoster& groupRoster = GlobalObjects::groupRoster();

GroupRoster::GroupRoster() {
    this->_requestManager = std::make_shared<RequestManager>(RequestManager());
    this->_groupManager = std::make_shared<GroupManager>(GroupManager());
}

void GroupRoster::restorePlayerGroup(CHAR_DATA *ch) {
    auto grp = this->_groupManager->findGroup(ch);
    if (grp == nullptr)
        return;
    if (!grp->getMemberManager()->restoreMember(ch))
        return;
    grp->sendToGroup(GRP_COMM_ALL, "Игрок %s заново присоединился к группе.\r\n", ch->get_pc_name().c_str());
    ch->send_to_TC(true, false, true, "Membership in group: %d restored\r\n", ch->personGroup->getUid());
}

void GroupRoster::processGroupCommands(CHAR_DATA *ch, char *argument) {
    enum SubCmd { PRINT, ALL, HELP, LIST, MAKE, INVITE, TAKE, REJECT, EXPELL, LEADER, LEAVE, DISBAND};
    SubCmd mode;

    const std::string strHELP = "справка помощь";
    const std::string strMAKE = "создать make";
    const std::string strLIST = "список list";
    const std::string strINVITE = "пригласить invite";
    const std::string strTAKE = "принять approve";
    const std::string strREJECT = "отклонить reject";
    const std::string strEXPELL = "выгнать expell";
    const std::string strLEADER = "лидер leader";
    const std::string strLEAVE = "покинуть leave";
    const std::string strDISBAND = "распустить disband";
    const std::string strALL = "все all";
    char subcmd[MAX_INPUT_LENGTH],  target[MAX_INPUT_LENGTH];

    if (!ch || IS_NPC(ch))
        return;

    two_arguments(argument, subcmd, target);

    // выбираем режим
    if (!*subcmd)
        mode = PRINT;
    else if (isname(subcmd, strALL.c_str()))
        mode = ALL;
    else if (isname(subcmd, strHELP.c_str()))
        mode = HELP;
    else if (isname(subcmd, strMAKE.c_str()))
        mode = MAKE;
    else if (isname(subcmd, strLIST.c_str()))
        mode = LIST;
    else if (isname(subcmd, strINVITE.c_str()))
        mode = INVITE;
    else if (isname(subcmd, strTAKE.c_str()))
        mode = TAKE;
    else if (isname(subcmd, strREJECT.c_str()))
        mode = REJECT;
    else if (isname(subcmd, strEXPELL.c_str()))
        mode = EXPELL;
    else if (isname(subcmd, strLEADER.c_str()))
        mode = LEADER;
    else if (isname(subcmd, strLEAVE.c_str()))
        mode = LEAVE;
    else if (isname(subcmd, strDISBAND.c_str()))
        mode = DISBAND;
    else {
        send_to_char("Уточните команду.\r\n", ch);
        return;
    }
    if (GET_POS(ch) < POS_RESTING) {
        send_to_char("Трудно управлять группой в таком состоянии.\r\n", ch);
        return;
    }
    auto grp = ch->personGroup;

    if (mode == MAKE) {
        if (grp != nullptr && grp->getLeader() == ch){
            send_to_char(ch, "Только великим правителям, навроде Цесаря Иулия, было дозволено водить много легионов!\r\n");
            return;
        }
        if (grp != nullptr) // в группе - покидаем
            grp->getMemberManager()->removeMember(ch);
        groupRoster.getGroupManager()->addGroup(ch);
        send_to_char(ch, "Вы создали группу c максимальным числом последователей %d.\r\n", ch->personGroup->getMemberCap());
        return;
    }

    if (grp != nullptr && grp->getLeader() != ch ) {
        send_to_char(ch, "Необходимо быть лидером группы для этого.\r\n");
        return;
    }


    // MAKE был раньше, т.к. он создает группу
    switch (mode) {
        case MAKE:
            break;
        case PRINT:
            grp->printGroup(ch);
            break;
        case ALL:
            if (!IS_IMMORTAL(ch))
                return;
            groupRoster.getGroupManager()->printList(ch);
            break;
        case HELP:
            break;
        case LIST:
            //grp->listMembers(ch);
            break;
        case INVITE:
            if (grp->isFull()){
                send_to_char(ch, "Командир, но в твоей группе больше нет места!\r\n");
                return;
            }
            groupRoster.getRequestManager()->makeInvite(ch, target);
            break;
        case TAKE:
            //grp->approveRequest(target);
            break;
        case REJECT:
            //grp->denyRequest(target);
            break;
        case EXPELL:
            //grp->expellMember(target);
            break;
        case LEADER:
            //grp->promote(target);
            break;
        case LEAVE:
            //grp->leave(ch);
            break;
        case DISBAND:
            groupRoster.getGroupManager()->removeGroup(grp->getUid());
            break;
    }
}