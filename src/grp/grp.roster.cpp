//
// Created by ubuntu on 17/12/20.
//

#include "global.objects.hpp"
#include "handler.h"
#include "grp.main.h"


GroupRoster& groupRoster = GlobalObjects::groupRoster();
extern const char *class_name[];

GroupRoster::GroupRoster() {
    initPenaltyTable();
}

void GroupRoster::restorePlayerGroup(CHAR_DATA *ch) {
    auto grp = this->findGroup(ch->get_uid());
    if (grp == nullptr)
        return;
    if (!grp->_restoreMember(ch))
        return;
    grp->actToGroup(nullptr, ch, GC_LEADER | GC_REST, "$N заново присоединил$U к вашей группе.");
}

void GroupRoster::processGroupScmds(CHAR_DATA *ch, char *argument, GRP_SUBCMD subcmd) {
    switch (subcmd) {
        case GRP_SUBCMD::GCMD_DISBAND: {
            auto grp = ch->personGroup;
            if (grp == nullptr) {
                send_to_char(ch, "Дабы выгнать кого, сперва надобно в ватаге состояти.\r\n");
                return;
            }
            if (ch != grp->getLeader()) {
                send_to_char(ch, "Негоже простому ратнику ватагой командовати.\r\n");
                return;
            }
            if (!*argument)
                groupRoster.removeGroup(grp->getUid());
            else
                grp->expellMember(argument);
            return;
        }
        default:{
            sprintf(buf, "GroupRoster::processGroupScmds: вызов не поддерживаемой команды.\r\n");
            mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
            send_to_char(ch, "Не поддерживается!\r\n");
            return;
        }
    }
}

void GroupRoster::processGroupCommands(CHAR_DATA *ch, char *argument) {

    const std::string strHELP = "справка помощь";
    const std::string strMAKE = "создать make";
    const std::string strLIST = "список list"; // краткий список членов группы
    const std::string strINVITE = "пригласить invite";
    const std::string strTAKE = "принять approve";
    const std::string strREJECT = "отклонить reject";
    const std::string strEXPELL = "выгнать expell";
    const std::string strLEADER = "лидер leader";
    const std::string strLEAVE = "покинуть выйти leave";
    const std::string strDISBAND = "распустить clear";
    const std::string strWORLD = "мир world";
    const std::string strALL = "все all";// старый режим, создание группы и добавление всех последователей.
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
        grp->printGroup(ch);
        return;
    }  else if (isname(subcmd, strHELP.c_str())) {
        send_to_char(ch, "Команды: %s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
                     strALL.c_str(),
                     strHELP.c_str(), strMAKE.c_str(), strLIST.c_str(),strINVITE.c_str(), strTAKE.c_str(),
                     strREJECT.c_str(), strEXPELL.c_str(), strLEADER.c_str(), strLEAVE.c_str(), strDISBAND.c_str());
        return;
    } else if (isname(subcmd, strMAKE.c_str())) {
        if (grp != nullptr && grp->getLeader() == ch){
            send_to_char(ch, "Только великим правителям, навроде Цесаря Иулия, было под силу водить много легионов!\r\n");
            return;
        }
        if (grp != nullptr) // в группе - покидаем
            grp->_removeMember(ch);
        groupRoster.addGroup(ch);
         return;
    } else if (isname(subcmd, strLIST.c_str())) {
        grp->listMembers(ch);
        return;
    } else if (isname(subcmd, strALL.c_str())) {
        // если не в группе, создаем и добавляем всех последователей
        if (grp == nullptr) {
            grp = groupRoster.addGroup(ch).get();
            if (!grp->addFollowers(ch))
                send_to_char(ch, "А окромя вас, в группу то добавить и некого...\r\n");
            return;
        }
        // если не лидер и в группе, печатаем полный список.
        if (grp->getLeader() != ch){
            grp->printGroup(ch);
            return;
        }
        // сюда приходим лидером и добавляем всех, кто следует.
        if (!grp->addFollowers(ch))
            send_to_char(ch, "Все, кто за вами следует, уже в группе.\r\n");
        return;
    } else if (isname(subcmd, strLEAVE.c_str())){
        grp->leaveGroup(ch);
        return;
    } else if (isname(subcmd, strWORLD.c_str())) {
        if (IS_IMMORTAL(ch))
            groupRoster.printList(ch);
        return;
    } else if (isname(subcmd, strTEST.c_str())) {
            groupRoster.runTests(ch);
        return;
    }

    if (grp == nullptr ||  grp->getLeader() != ch ) {
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
    send_to_char(leader, "Вы создали группу c максимальным числом соратников %d.\r\n", leader->personGroup ? leader->personGroup->_getMemberCap() : 0);
    return group;
}

void GroupRoster::removeGroup(u_long uid) {
    auto grp = _groupList.find(uid);
    if (grp != _groupList.end()) {
        _groupList.erase(uid);
        return;
    }
}

grp_ptr GroupRoster::findGroup(int personUID) {
    if (personUID == 0){
        sprintf(buf, "GroupRoster::findGroup: вызов для пустого или пурженого персонажа\r\n");
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return nullptr;
    }
    for (auto & it : this->_groupList){
        if (it.second->_isMember(personUID))
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
        if (isname(leaderName, it.second->getLeaderName().c_str()))
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
        send_to_char(ch, "Группа лидера %s, кол-во участников: %lu\r\n",
                     it.second->getLeaderName().c_str(),
                     it.second->size());
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
            it->_expiryTime = steady_clock::now() + DEF_EXPIRY_TIME;
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
            notFound = false;
        }
        if (ch->personGroup != nullptr && ch->personGroup->getUid() == it->_group->getUid()) {
            notFound = false;
            if (it->_type== RQ_TYPE::RQ_GROUP) {
                sprintf(smallBuf, "Приглашение игроку: %s\r\n", it->_applicantName.c_str() );
            }
            else {
                sprintf(smallBuf, "Заявка от игрока: %s\r\n", it->_applicantName.c_str() );
            }
        }
        if (!notFound)
            send_to_char(ch, "%s", smallBuf);
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
            grp->actToGroup(nullptr, author, GC_LEADER, "Получена заявка от $N1 на вступление в группу.\r\n");
            break;
        case RQ_R::RQ_REFRESH:
            send_to_char("Заявка успешно продлена.\r\n", author);
            break;
    }
}

void GroupRoster::revokeRequest(CHAR_DATA *ch, char* target) {
    if (!ch)
        return;
    for (auto it = this->_requestList.begin(); it != this->_requestList.end(); ++it)
        if ( it->get()->_applicant == ch  && isname(target, it->get()->_group->getLeaderName().c_str())){
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
            it->_expiryTime = std::chrono::steady_clock::now() + DEF_EXPIRY_TIME;
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
        if (it->_type == (type == RQ_ANY ? it->_type : type)  && // осподи, какой я пиздец пишу)
            isname(group, it->_group->getLeaderName().c_str()) &&
            isname(targetPerson, it->_applicantName))
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
    if (!*author) {
        send_to_char(who, "Соберись! Чье приглашение принимаем?!\r\n");
        return;
    }

    auto r = findRequest(who->get_pc_name().c_str(), author, RQ_TYPE::RQ_GROUP);
    if (r == nullptr) {
        send_to_char(who, "Соберись! Чье приглашение принимаем?!\r\n");
        return;
    }
    send_to_char(r->_applicant, "Вы приняли приглашение.\r\n");
    r->_group->addMember(r->_applicant); // и удалит заявку, если есть
}

void GroupRoster::runTests(CHAR_DATA *) {

}

// удаление заявок при пурже персонажа
void GroupRoster::charDataPurged(CHAR_DATA *ch) {
    if (IS_NPC(ch))
        return;
    for (auto r = _requestList.begin(); r != _requestList.end();){
        if (r->get()->_applicant == ch) {
            r = _requestList.erase(r);
        } else {
            ++r;
        }
    }
}

void GroupRoster::processGarbageCollection() {
    for (const auto& it: _groupList){
        it.second->processGarbageCollection();
    }
    for (auto rq = _requestList.begin(); rq != _requestList.end();){
        if ((*rq)->_expiryTime <= steady_clock::now()) {
            rq = _requestList.erase(rq);
        } else {
            ++rq;
        }
    }
}


// expell timed out members
void grp::gc() {
    groupRoster.processGarbageCollection();
}

int GroupRoster::initPenaltyTable()
{
    int clss = 0, remorts = 0, rows_assigned = 0, levels = 0, pos = 0, max_rows = MAX_REMORT+1;

    // пре-инициализация
    for (remorts = 0; remorts < max_rows; remorts++) //Строк в массиве должно быть на 1 больше, чем макс. морт
    {
        for (clss = 0; clss < NUM_PLAYER_CLASSES; clss++) //Столбцов в массиве должно быть ровно столько же, сколько есть классов
        {
            m_grouping[clss][remorts] = -1;
        }
    }

    FILE* f = fopen(LIB_MISC "grouping", "r");
    if (!f)
    {
        log("Невозможно открыть файл %s", LIB_MISC "grouping");
        return 1;
    }

    while (get_line(f, buf))
    {
        if (!buf[0] || buf[0] == ';' || buf[0] == '\n') //Строка пустая или строка-коммент
        {
            continue;
        }
        clss = 0;
        pos = 0;
        while (sscanf(&buf[pos], "%d", &levels) == 1)
        {
            while (buf[pos] == ' ' || buf[pos] == '\t')
            {
                pos++;
            }
            if (clss == 0) //Первый проход цикла по строке
            {
                remorts = levels; //Номера строк
                if (m_grouping[0][remorts] != -1)
                {
                    log("Ошибка при чтении файла %s: дублирование параметров для %d ремортов",
                        LIB_MISC "grouping", remorts);
                    return 2;
                }
                if (remorts > MAX_REMORT || remorts < 0)
                {
                    log("Ошибка при чтении файла %s: неверное значение количества ремортов: %d, "
                        "должно быть в промежутке от 0 до %d",
                        LIB_MISC "grouping", remorts, MAX_REMORT);
                    return 3;
                }
            }
            else
            {
                m_grouping[clss - 1][remorts] = levels; // -1 потому что в массиве нет столбца с кол-вом мортов
            }
            clss++; //+Номер столбца массива
            while (buf[pos] != ' ' && buf[pos] != '\t' && buf[pos] != 0)
            {
                pos++; //Ищем следующее число в строке конфига
            }
        }
        if (clss != NUM_PLAYER_CLASSES+1)
        {
            log("Ошибка при чтении файла %s: неверный формат строки '%s', должно быть %d "
                "целых чисел, прочитали %d", LIB_MISC "grouping", buf, NUM_PLAYER_CLASSES+1, clss);
            return 4;
        }
        rows_assigned++;
    }

    if (rows_assigned < max_rows)
    {
        for (levels = remorts; levels < max_rows; levels++) //Берем свободную переменную
        {
            for (clss = 0; clss < NUM_PLAYER_CLASSES; clss++)
            {
                m_grouping[clss][levels] = m_grouping[clss][remorts]; //Копируем последнюю строку на все морты, для которых нет строк
            }
        }
    }
    fclose(f);
    return 0;
}

void GroupRoster::show(CHAR_DATA *ch, char* arg) {
    send_to_char(ch, "Параметры группы:\r\n");
    for (int i = 0; i< NUM_PLAYER_CLASSES; i++) {
        if (*arg && !isname(arg, class_name[i]))
            continue;
        send_to_char(ch, "Класс %s, реморт:уровень:\r\n", class_name[i]);
        for (int j = 0; j < MAX_REMORT; j++) {
            send_to_char(ch, "%d:%d ", j, m_grouping[i][j]);
        }
        send_to_char(ch, "\r\n");
    }
}

short GroupRoster::getPenalty(const CHAR_DATA *ch) {
    if (ch == nullptr || ch->purged())
        return 0;
    return m_grouping[GET_CLASS(ch)][GET_REMORT(ch)];
}
