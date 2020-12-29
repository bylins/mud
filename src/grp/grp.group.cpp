
#include <memory>

#include "comm.h"
#include "handler.h"
#include "msdp.constants.hpp"
#include "grp.main.h"
#include "screen.h"
#include "magic.h"

extern GroupRoster& groupRoster;

const char *WORD_STATE[] = { "При смерти",
                             "Оч.тяж.ран",
                             "Оч.тяж.ран",
                             " Тяж.ранен",
                             " Тяж.ранен",
                             "  Ранен   ",
                             "  Ранен   ",
                             "  Ранен   ",
                             "Лег.ранен ",
                             "Лег.ранен ",
                             "Слег.ранен",
                             " Невредим "
};
const char *MOVE_STATE[] = { "Истощен",
                             "Истощен",
                             "О.устал",
                             " Устал ",
                             " Устал ",
                             "Л.устал",
                             "Л.устал",
                             "Хорошо ",
                             "Хорошо ",
                             "Хорошо ",
                             "Отдохн.",
                             " Полон "
};
const char *POS_STATE[] = { "Умер",
                            "Истекает кровью",
                            "При смерти",
                            "Без сознания",
                            "Спит",
                            "Отдыхает",
                            "Сидит",
                            "Сражается",
                            "Стоит"
};


Group::Group(CHAR_DATA *leader, u_long uid){
    mudlog("Group", BRF, LVL_IMMORT, SYSLOG, TRUE);
    _leaderUID = 0; // чтобы не ругался clion =)
    _memberCap = 0;
    _uid = uid;
    _memberList = new std::map<int, std::shared_ptr<char_info *>>;
    addMember(leader);
    _setLeader(leader);
}

CHAR_DATA *Group::getLeader() const{
    return _leader;
}

int Group::_findMember(char *memberName) {
    for (auto & it : *_memberList) {
        if (isname((*it.second)->memberName, memberName))
            return it.first;
    }
    return 0;
}

CHAR_DATA* Group::_findMember(int uid) {
    for (auto & it : *_memberList) {
        if ((*it.second)->memberUID == uid)
            return (*it.second)->member;
    }
    return nullptr;
}

const char* Group::_getMemberName(int uid) {
    for (auto & it : *_memberList) {
        if ((*it.second)->memberUID == uid)
            return (*it.second)->memberName.c_str();
    }
    return nullptr;
}

void Group::_setLeader(CHAR_DATA *leader) {
    _leaderUID = leader->get_uid();
    _leader = leader;
    _leaderName.assign(leader->get_pc_name());
    _memberCap = IS_NPC(leader)? 255 : max_group_size(leader);
}

bool Group::_isActive() {
    return false;
}

u_long Group::getUid() const {
    return _uid;
}

u_short Group::getCurrentMemberCount() const {
    return (u_short)_memberList->size();
}

int Group::_getMemberCap() const {
    return _memberCap;
}

const std::string &Group::getLeaderName() const {
    return _leaderName;
}

bool Group::_isFull() {
    if ((u_short)_memberList->size() >= (u_short)_memberCap)
        return true;
    return false;
}

bool Group::_sameGroup(CHAR_DATA *ch, CHAR_DATA *vict) {
    if (ch == nullptr || vict == nullptr)
        return false;
    if (ch->personGroup == vict->personGroup)
        return true;
    return false;
}


Group::~Group() {
    this->_clear(false);
    mudlog("~Group", BRF, LVL_IMMORT, SYSLOG, TRUE);
}

char_info::char_info(int uid, CHAR_DATA *m, std::string name) {
    memberUID = uid;
    member = m;
    memberName.assign(name);
}

char_info::~char_info() {
    mudlog("[~char_info]", BRF, LVL_IMMORT, SYSLOG, TRUE);
}

void Group::addMember(CHAR_DATA *member) {
    if (IS_NPC(member))
        return;
    if (member->personGroup == this)
        return;
    auto it = _memberList->find(member->get_uid());
    if (it == _memberList->end()) {
        auto ci = new char_info(member->get_uid(), member, member->get_pc_name());
        _memberList->emplace(member->get_uid(), std::make_shared<char_info *>(ci));
    }
    else {
        sprintf(buf, "Group::addMember: группа id=%lu, попытка повторного добавления персонажа с тем же uid", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    member->personGroup = this;
    actToGroup(member, "$N принят$A в группу.");
//    getLeader()->send_to_TC(true, false, false, "Размер списка: %lu\r\n", _memberList->size());
}

bool Group::restoreMember(CHAR_DATA *member) {
    for (auto &it: *_memberList){
        if (it.first == member->get_uid()) {
            (*it.second)->member = member;
            return true;
        }
    }
    return false;
}

bool Group::_removeMember(CHAR_DATA *member) {
    if (member == nullptr || member->purged())
        return false;
    if (_memberList->empty()) {
        sprintf(buf, "Group::_removeMember: попытка удалить из группы при текущем индексе 0, id=%lu", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    // удаляем персонажа из группы и ссылку на группу
    auto num = _memberList->erase(member->get_uid());
    member->personGroup = nullptr;
    if (num == 0){
        sprintf(buf, "Group::_removeMember: персонаж не найден, id=%lu", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }

    CHAR_DATA* nxtLdr = _leader;
    int nxtLdrGrpSize = 0;
    // если ушел лидер - ищем нового, но после ухода предыдущего
    if ( member == _leader) {
        for (const auto & mit : *_memberList){
            auto m = (*mit.second)->member;
            if ( m != nullptr && !m->purged() && max_group_size(m) > nxtLdrGrpSize ) {
                nxtLdr = m;
                nxtLdrGrpSize = max_group_size(m);
            }
        }
    }
    // никого не осталось, распускаем группу
    if (nxtLdr == nullptr) {
        groupRoster.removeGroup(_uid);
        return false;
    }
    _setLeader(nxtLdr);
    return true;
}

bool Group::_isMember(int uid) {
    auto it = _memberList->find(uid);
    if (it == _memberList->end() )
        return false;
    return true;
}

void Group::_clear(bool silentMode) {
    sprintf(buf, "[Group::clear()]: _memberList: %lu", _memberList->size());
    mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
    CHAR_DATA* ch;
    for (auto & it : *_memberList) {
        ch = (*it.second)->member;
        if (ch == nullptr || ch->purged())
            continue;
        if (!silentMode)
            send_to_char(ch, "Ваша группа распущена.\r\n");
        ch->personGroup = nullptr;
    }
    _memberList->clear();
}

void Group::promote(char *applicant) {
    int memberId = _findMember(applicant);
    if (memberId == 0) {
        sendToGroup(GRP_COMM_LEADER, "Нет такого персонажа.");
        return;
    }
    _setLeader((*_memberList->at(memberId))->member);
    sendToGroup(GRP_COMM_ALL, "Изменился лидер группы на %.", (*_memberList->at(memberId))->memberName.c_str());
    _memberCap = 1;
    auto diff = (u_short)(_memberList->size() - _memberCap);
    if (diff > 0){
        u_short i = 0;
        CHAR_DATA* expellList[diff];
        for (auto it = _memberList->begin(); it!= _memberList->end() || diff > i; it++){
            if ((*it->second)->member != _leader ) {
                expellList[i] = (*it->second)->member;
                ++i;
            }
        }
        for (i=0; i < diff; i++){
            _removeMember(expellList[i]);
        }
    }
}

void Group::rejectRequest(char *applicant) {
    if (!*applicant) {
        send_to_char(_leader, "Заявка не найдена, уточните имя.\r\n");
        return;
    }
    auto r = groupRoster.findRequest(applicant, this->getLeaderName().c_str(), RQ_PERSON);
    if (r == nullptr || r->_group != this){
        send_to_char(_leader, "Заявка не найдена, уточните имя.\r\n");
        return;
    }
    send_to_char(r->_applicant, "Ваша заявка в группу лидера %s отклонена.\r\n", r->_group->getLeaderName().c_str());
    groupRoster.deleteRequest(r);
    send_to_char(_leader, "Вы отклонили заявку.\r\n");
}


void Group::approveRequest(const char *applicant) {
    if (!*applicant) {
        send_to_char(_leader, "Заявка не найдена, уточните имя.\r\n");
        return;
    }
    auto r = groupRoster.findRequest(applicant, this->getLeaderName().c_str(), RQ_PERSON);
    if (r == nullptr || r->_group != this){
        send_to_char(_leader, "Заявка не найдена, уточните имя.\r\n");
        return;
    }
    actToGroup(r->_applicant, "$N принят$A в группу.");
    addMember(r->_applicant);
    send_to_char(r->_applicant, "Вас приняли в группу.\r\n");
    groupRoster.deleteRequest(r);
}

void Group::expellMember(char *memberName) {
    auto mId = _findMember(memberName);
    auto vict = (*_memberList->at(mId))->member;
    if (mId != 0 && vict != nullptr) {
        _removeMember(vict);
        act("$N исключен$A из состава вашей группы.", FALSE, _leader, nullptr, vict, TO_CHAR);
        act("Вы исключены из группы $n1!", FALSE, _leader, nullptr, vict, TO_VICT);
        act("$N был$G исключен$A из группы $n1!", FALSE, _leader, nullptr, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    }
}

void Group::listMembers(CHAR_DATA *ch) {
    CHAR_DATA *leader;
    struct follow_type *f;
    int count = 1;

    if (ch->personGroup)
    {
        leader = ch->personGroup->getLeader();
        for (auto & it : *_memberList )
        {
            if ((*it.second)->member == leader)
                continue;
            count++;
            if (count == 2){
                send_to_char("Ваша группа состоит из:\r\n", ch);
                sprintf(smallBuf, "Лидер: %s\r\n", ch->personGroup->getLeaderName().c_str() );
                send_to_char(smallBuf, ch);
            }
            sprintf(smallBuf, "%d. Согруппник: %s\r\n", count, (*it.second)->memberName.c_str());
            send_to_char(smallBuf, ch);
        }
        if (count == 1)
            send_to_char(ch, "Увы, но вы один, как перст!\r\n");
    }
    else
    {
        send_to_char("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
    }
}

void Group::leaveGroup(CHAR_DATA* ch) {
    if (ch->personGroup == nullptr) {
        send_to_char(ch, "Нельзя стать еще более одиноким, чем сейчас.\r\n");
        return;
    }
    _removeMember(ch);
}

void Group::_printHeader(CHAR_DATA* ch, bool npc) {
    if (ch == nullptr)
        return;
    if (npc)
        send_to_char("Персонаж            | Здоровье |Рядом| Аффект | Положение\r\n", ch);
    else
        send_to_char ("Персонаж            | Здоровье |Энергия|Рядом|Учить| Аффект | Кто | Держит строй | Положение \r\n", ch);

}

void Group::_printDeadLine(CHAR_DATA* ch, const char* playerName, int header) {
    if (ch == nullptr)
        return;
    if (header == 0)
        _printHeader(ch, false);
    send_to_char(ch, "%s%-20s| Пока нет в игре.%s\r\n",  CCINRM(ch, C_NRM), playerName, CCNRM(ch, C_NRM));
}

void Group::_printNPCLine(CHAR_DATA* ch, CHAR_DATA* npc, int header) {
    if (ch == nullptr || npc == nullptr)
        return;
    if (header == 0)
        _printHeader(ch, true);
    std::string name = GET_NAME(npc);
    name[0] = UPPER(name[0]);
    sprintf(buf, "%s%-20s%s|", CCIBLU(ch, C_NRM),
            name.substr(0, 20).c_str(), CCNRM(ch, C_NRM));
    sprintf(buf + strlen(buf), "%s%10s%s|",
            color_value(ch, GET_HIT(npc), GET_REAL_MAX_HIT(npc)),
            WORD_STATE[posi_value(GET_HIT(npc), GET_REAL_MAX_HIT(npc)) + 1], CCNRM(ch, C_NRM));

    auto ok = ch->in_room == IN_ROOM(npc);
    sprintf(buf + strlen(buf), "%s%5s%s|",
            ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));

    sprintf(buf + strlen(buf), " %s%s%s%s%s%s%s%s%s%s%s%s%s |",
            CCIRED(ch, C_NRM),
            AFF_FLAGGED(npc, EAffectFlag::AFF_SANCTUARY) ? "О" : (AFF_FLAGGED(npc, EAffectFlag::AFF_PRISMATICAURA) ? "П" : " "),
            CCGRN(ch, C_NRM),
            AFF_FLAGGED(npc, EAffectFlag::AFF_WATERBREATH) ? "Д" : " ", CCICYN(ch, C_NRM),
            AFF_FLAGGED(npc, EAffectFlag::AFF_INVISIBLE) ? "Н" : " ", CCIYEL(ch, C_NRM),
            (AFF_FLAGGED(npc, EAffectFlag::AFF_SINGLELIGHT)
             || AFF_FLAGGED(npc, EAffectFlag::AFF_HOLYLIGHT)
             || (GET_EQ(npc, WEAR_LIGHT)
                 && GET_OBJ_VAL(GET_EQ(npc, WEAR_LIGHT), 2))) ? "С" : " ",
            CCIBLU(ch, C_NRM), AFF_FLAGGED(npc, EAffectFlag::AFF_FLY) ? "Л" : " ", CCYEL(ch, C_NRM),
            npc->low_charm() ? "Т" : " ", CCNRM(ch, C_NRM));

    sprintf(buf + strlen(buf), "%-15s", POS_STATE[(int) GET_POS(npc)]);

    act(buf, FALSE, ch, nullptr, npc, TO_CHAR);
}

void Group::_printPCLine(CHAR_DATA* ch, CHAR_DATA* pc, int header) {
    int ok, ok2, div;

    bool leader = false;
    if (pc!= nullptr && pc->personGroup)
        if (pc->personGroup->getLeader() == pc)
            leader = true;
    if (header == 0)
        _printHeader(ch, false);
    std::string name = pc->get_pc_name();
    name[0] = UPPER(name[0]);
    sprintf(buf, "%s%-20s%s|", CCIBLU(ch, C_NRM), name.c_str(), CCNRM(ch, C_NRM));
    sprintf(buf + strlen(buf), "%s%10s%s|",
            color_value(ch, GET_HIT(pc), GET_REAL_MAX_HIT(pc)),
            WORD_STATE[posi_value(GET_HIT(pc), GET_REAL_MAX_HIT(pc)) + 1], CCNRM(ch, C_NRM));

    sprintf(buf + strlen(buf), "%s%7s%s|",
            color_value(ch, GET_MOVE(pc), GET_REAL_MAX_MOVE(pc)),
            MOVE_STATE[posi_value(GET_MOVE(pc), GET_REAL_MAX_MOVE(pc)) + 1], CCNRM(ch, C_NRM));

    ok = ch->in_room == IN_ROOM(pc);
    sprintf(buf + strlen(buf), "%s%5s%s|",
            ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));

    if ((!IS_MANA_CASTER(pc) && !MEMQUEUE_EMPTY(pc)) ||
        (IS_MANA_CASTER(pc) && GET_MANA_STORED(pc) < GET_MAX_MANA(pc)))
    {
        div = mana_gain(pc);
        if (div > 0) {
            if (!IS_MANA_CASTER(pc)) {
                ok2 = MAX(0, 1 + GET_MEM_TOTAL(pc) - GET_MEM_COMPLETED(pc));
                ok2 = ok2 * 60 / div;	// время мема в сек
            } else {
                ok2 = MAX(0, 1 + GET_MAX_MANA(pc) - GET_MANA_STORED(pc));
                ok2 = ok2 / div;	// время восстановления в секундах
            }
            ok = ok2 / 60;
            ok2 %= 60;
            if (ok > 99)
                sprintf(buf + strlen(buf), "&g%5d&n|", ok);
            else
                sprintf(buf + strlen(buf), "&g%2d:%02d&n|", ok, ok2);
        } else {
            sprintf(buf + strlen(buf), "&r    -&n|");
        }
    } else
        sprintf(buf + strlen(buf), "     |");

    sprintf(buf + strlen(buf), " %s%s%s%s%s%s%s%s%s%s%s%s%s |",
            CCIRED(ch, C_NRM), AFF_FLAGGED(pc, EAffectFlag::AFF_SANCTUARY) ? "О" : (AFF_FLAGGED(pc, EAffectFlag::AFF_PRISMATICAURA)
                                                                                        ? "П" : " "), CCGRN(ch,
                                                                                                            C_NRM),
            AFF_FLAGGED(pc, EAffectFlag::AFF_WATERBREATH) ? "Д" : " ", CCICYN(ch,
                                                                                  C_NRM),
            AFF_FLAGGED(pc, EAffectFlag::AFF_INVISIBLE) ? "Н" : " ", CCIYEL(ch, C_NRM), (AFF_FLAGGED(pc, EAffectFlag::AFF_SINGLELIGHT)
                                                                                             || AFF_FLAGGED(pc, EAffectFlag::AFF_HOLYLIGHT)
                                                                                             || (GET_EQ(pc, WEAR_LIGHT)
                                                                                                 &&
                                                                                                 GET_OBJ_VAL(GET_EQ
                                                                                                             (pc, WEAR_LIGHT),
                                                                                                             2))) ? "С" : " ",
            CCIBLU(ch, C_NRM), AFF_FLAGGED(pc, EAffectFlag::AFF_FLY) ? "Л" : " ", CCYEL(ch, C_NRM),
            pc->ahorse() ? "В" : " ", CCNRM(ch, C_NRM));

    sprintf(buf + strlen(buf), "%5s|", leader ? "Лидер" : "");
    ok = PRF_FLAGGED(pc, PRF_SKIRMISHER);
    sprintf(buf + strlen(buf), "%s%-14s%s|",	ok ? CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));
    sprintf(buf + strlen(buf), " %s", POS_STATE[(int) GET_POS(pc)]);
    act(buf, FALSE, ch, nullptr, pc, TO_CHAR);
}

void Group::printGroup(CHAR_DATA *ch) {
    int gfound = 0, cfound = 0;
    CHAR_DATA *leader;
    struct follow_type *f, *g;

    if (ch->personGroup)
        leader = ch->personGroup->getLeader();
    else
        leader = ch;

    if (!IS_NPC(ch))
        ch->desc->msdp_report(msdp::constants::GROUP);

    // печатаем группу
    if (ch->personGroup != nullptr )
    {
        send_to_char("Ваша группа состоит из:\r\n", ch);
        for (auto &it : *_memberList ){
            if ((*it.second)->member == nullptr || (*it.second)->member->purged())
                _printDeadLine(ch, (*it.second)->memberName.c_str(), gfound++);
            else
                _printPCLine(ch, (*it.second)->member, gfound++);
        }
    }

    // допечатываем чармисов
    for (f = ch->followers; f; f = f->next)
    {
        if (!(AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
              || MOB_FLAGGED(f->follower, MOB_ANGEL)|| MOB_FLAGGED(f->follower, MOB_GHOST))) {
            continue;
        }
        if (!cfound)
            send_to_char("Ваши последователи:\r\n", ch);
        _printNPCLine(ch, f->follower, cfound++);
    }

    if (!gfound && !cfound) {
        send_to_char("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
        return;
    }

    // печатаем чармисов членов группы
    if (PRF_FLAGGED(ch, PRF_SHOWGROUP))
    {
        for (auto &it : *_memberList ) {
            for (f = (*it.second)->member->followers; f; f = f->next) {
                if (!(AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
                      || MOB_FLAGGED(f->follower, MOB_ANGEL) || MOB_FLAGGED(f->follower, MOB_GHOST))){
                    continue;
                }
                // клоны отключены
                if (PRF_FLAGGED(ch, PRF_NOCLONES) && IS_NPC(f->follower) &&
                    (MOB_FLAGGED(f->follower, MOB_CLONE) || GET_MOB_VNUM(f->follower) == MOB_KEEPER)) {
                    continue;
                }

                if (!cfound) {
                    send_to_char("Последователи членов вашей группы:\r\n", ch);
                }
                _printNPCLine(ch, f->follower, cfound++);
            }

        }
    }
}



void Group::charDataPurged(CHAR_DATA *ch) {
    for (auto &it : *_memberList) {
        if (ch == (*(it.second))->member) {
            (*(it.second))->member = nullptr;
            return;
        }
    }
}


void Group::sendToGroup(GRP_COMM mode, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vsnprintf(smallBuf, sizeof(smallBuf), msg, args);
    va_end(args);
    if (mode == GRP_COMM::GRP_COMM_ALL)
        for (auto & it : *_memberList ) {
            send_to_char((*it.second)->member, "%s%s", smallBuf, "\r\n");
        }
    else if (mode == GRP_COMM_LEADER) {
        send_to_char(_leader, "%s%s", smallBuf, "\r\n");
    }
}

// надстройка над act, пока только про персонажей
void Group::actToGroup(CHAR_DATA* vict, const char *msg, ...) {
    if (vict == nullptr)
        return;
    va_list args;
    va_start(args, msg);
    vsnprintf(smallBuf, sizeof(smallBuf), msg, args);
    va_end(args);
    for (auto &it : *_memberList) {
        auto ch = (*(it.second))->member;
        if ( ch == nullptr || ch->purged()) {
            continue;
        }
        act(smallBuf, FALSE, ch, nullptr, vict, TO_CHAR);
    }
}

void Group::makeAddFollowers(CHAR_DATA *leader) {
    for (auto f = leader->followers; f; f = f->next) {
        if (IS_NPC(f->follower))
            continue;
        if (_memberList->size() > _memberCap)
            this->addMember(f->follower);
    }
}
