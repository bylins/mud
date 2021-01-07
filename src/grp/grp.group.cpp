/* ************************************************************************
*   File: grp.main.h                                    Part of Bylins    *
*   Методы и функции работы с группами персонажей                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
************************************************************************ */

#include <memory>

#include "grp.main.h"

#include "bonus.h"
#include "comm.h"
#include "msdp.constants.hpp"
#include "screen.h"
#include "core/leveling.h"
#include "magic.h"
#include "mob_stat.hpp"
#include "top.h"
#include "zone.table.hpp"

using namespace ExpCalc;

extern GroupRoster& groupRoster;
extern int max_exp_gain_npc;


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
    addMember(leader, true);
    _setLeader(leader);
}

CHAR_DATA *Group::getLeader() const{
    return _leader;
}

char_info*  Group::_findMember(char *memberName) {
    for (auto & it : *this) {
        if (isname(memberName, it.second->memberName))
            return it.second.get();
    }
    return nullptr;
}

const char* Group::_getMemberName(int uid) {
    for (auto & it : *this) {
        if (it.second->memberUID == uid)
            return it.second->memberName.c_str();
    }
    return nullptr;
}

void Group::_setLeader(CHAR_DATA *leader) {
    _leaderUID = _calcUID(leader);
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

int Group::_getMemberCap() const {
    return _memberCap;
}

const std::string &Group::getLeaderName() const {
    return _leaderName;
}

bool Group::_isFull() {
    if ((u_short)this->size() >= (u_short)_memberCap)
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

char_info::char_info(int uid, GM_TYPE type, CHAR_DATA *m, const std::string& name) {
    this->memberUID = uid;
    this->type = type;
    this->member = m;
    this->memberName.assign(name);
    this->expiryTime = steady_clock::now() + DEF_EXPIRY_TIME;
}

char_info::~char_info() {
    mudlog("[~char_info]", BRF, LVL_IMMORT, SYSLOG, TRUE);
}

void Group::addMember(CHAR_DATA *member, bool silent) {
    if (member->personGroup == this)
        return;
    // в другой группе, вышвыриваем
    if (member->personGroup != nullptr)
        _removeMember(member);

    auto it = this->find(_calcUID(member));
    if (it == this->end()) {
        // рисуем сообщение до добавления, чтобы самому персонажу не показывать
        if (!silent) {
            actToGroup(member, GRP_COMM_ALL, "$N принят$A в группу.");
            send_to_char(member, "Вас приняли в группу.\r\n");
        }
        auto ci = char_info(_calcUID(member), getType(member), member, member->get_pc_name());
        this->emplace(_calcUID(member), std::make_shared<char_info>(ci));
    } else {
        sprintf(buf, "Group::addMember: группа id=%lu, попытка повторного добавления персонажа с тем же uid",
                getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    member->personGroup = this;
    auto r = groupRoster.findRequest(member->get_pc_name().c_str(), getLeaderName().c_str(), RQ_ANY);
    if (r)
        groupRoster.deleteRequest(r);
    // добавляем чармисов группу
    for (auto f = member->followers; f; f = f->next) {
        auto ff = f->follower;
        if (IS_CHARMICE(ff)) {
            this->addMember(ff, true); // рекурсия!
        }
    }
}

bool Group::_removeMember(int memberUID) {
    if (this->empty()) {
        sprintf(buf, "Group::_removeMember: попытка удалить из группы при текущем индексе 0, id=%lu", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    // удаляем персонажа из группы и ссылку на группу
    auto it = this->find(memberUID);
    if (it == this->end()) {
        sprintf(buf, "Group::_removeMember: персонаж не найден, id=%lu", getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return false;
    }
    auto member = it->second->member;
    if (member != nullptr)
        member->personGroup = nullptr;
    this->erase(memberUID); // finally remove it

    // а также удаляем чармисов персонажа из групповых
    if (member->followers != nullptr) {
        for (auto f = member->followers; f; f = f->next) {
            auto ff = f->follower;
            if (IS_CHARMICE(ff)) {
                auto c = this->find(_calcUID(ff));
                if (c != this->end()) {
                    this->erase(c);
                    ff->personGroup = nullptr;
                }
            }
        }
    }

    CHAR_DATA* nxtLdr = _leader;
    int nxtLdrGrpSize = 0;
    // если ушел лидер - ищем нового, но после ухода предыдущего
    if ( member == _leader) {
        nxtLdr = nullptr;
        for (const auto& mit : *this){
            auto m = mit.second->member;
            if ( m != nullptr && !m->purged() && max_group_size(m) > nxtLdrGrpSize ) {
                nxtLdr = m; // нашелся!!
                nxtLdrGrpSize = max_group_size(m);
            }
        }
    }
    // нового лидера не нашлось, распускаем группу
    if (nxtLdr == nullptr) {
        groupRoster.removeGroup(_uid);
        return false;
    }
    _promote(nxtLdr);
    return true;
}

bool Group::_removeMember(char *name) {
    int uid = 0;
    for (auto &it: *this){
        if (str_cmp(name, it.second->memberName) == 0) {
            uid = it.first;
            break;
        }
    }
    if (uid != 0)
       return _removeMember(uid);
    return false;
}

bool Group::_removeMember(CHAR_DATA *member) {
    int memberUID = _calcUID(member);
    return _removeMember(memberUID);
}

bool Group::_restoreMember(CHAR_DATA *member) {
    if (!member)
        return false;
    for (auto &it: *this){
        if (it.first == _calcUID(member)) {
            it.second->member = member;
            member->personGroup = this;
            return true;
        }
    }
    return false;
}

bool Group::_isMember(int uid) {
    auto it = this->find(uid);
    if (it == this->end() )
        return false;
    return true;
}

void Group::_clear(bool silentMode) {
    sprintf(buf, "[Group::clear()]: this: %hu", this->size());
    mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
    CHAR_DATA* ch;
    // чистим игроков
    for (auto & it : *this) {
        ch = it.second->member;
        if (ch == nullptr || ch->purged())
            continue;
        if (!silentMode)
            send_to_char(ch, "Ваша группа распущена.\r\n");
        ch->personGroup = nullptr;
    }
    this->clear();
}

void Group::promote(char *applicant) {
    auto member = _findMember(applicant);
    if (member == nullptr || member->member == nullptr) {
        sendToGroup(GRP_COMM_LEADER, "Нет такого персонажа.");
        return;
    }
    _promote(member->member);
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
    addMember(r->_applicant); //и удаление заявки
}

void Group::expellMember(char *memberName) {
    auto vict = _findMember(memberName);
    if (vict == nullptr) {
        sendToGroup(GRP_COMM_LEADER, "Нет такого персонажа.");
        return;
    }
    _removeMember(vict->memberUID);
    if (vict != nullptr && vict->member != nullptr) {
        act("$N исключен$A из состава вашей группы.", FALSE, _leader, nullptr, vict, TO_CHAR);
        act("Вы исключены из группы $n1!", FALSE, _leader, nullptr, vict, TO_VICT);
        act("$N был$G исключен$A из группы $n1!", FALSE, _leader, nullptr, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    } else {
        sendToGroup(GRP_COMM_LEADER, "%s был исключен из группы.'\r\n", vict->memberName.c_str());
    }
}

void Group::listMembers(CHAR_DATA *ch) {
    CHAR_DATA *leader;
    struct follow_type *f;
    int count = 1;

    if (ch->personGroup)
    {
        leader = ch->personGroup->getLeader();
        for (auto & it : *this )
        {
            if (it.second->member == leader)
                continue;
            count++;
            if (count == 2){
                send_to_char("Ваша группа состоит из:\r\n", ch);
                sprintf(smallBuf, "Лидер: %s\r\n", ch->personGroup->getLeaderName().c_str() );
                send_to_char(smallBuf, ch);
            }
            sprintf(smallBuf, "%d. Согруппник: %s\r\n", count, it.second->memberName.c_str());
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
    _removeMember(_calcUID(ch));
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

    auto ok = SAME_ROOM(ch, npc);
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

    ok = SAME_ROOM(ch, pc);
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

    if (ch->personGroup)
        leader = ch->personGroup->getLeader();
    else
        leader = ch;

    if (!IS_NPC(ch))
        ch->desc->msdp_report(msdp::constants::GROUP);

    // печатаем группу
    if (ch->personGroup != nullptr ) {
        send_to_char("Ваша группа состоит из:\r\n", ch);
        for (auto &it : *this ){
            if (it.second->type == GM_CHARMEE)
                continue; // чармисов скипуем и показываем ниже
            if (it.second->member == nullptr || it.second->member->purged())
                _printDeadLine(ch, it.second->memberName.c_str(), gfound++);
            else
                _printPCLine(ch, it.second->member, gfound++);
        }
    }

    // допечатываем чармисов, которые прямые последователи
    for (auto f = ch->followers; f; f = f->next)
    {
        if (IS_CHARMICE(f->follower)) {
            if (!cfound)
                send_to_char("Ваши последователи:\r\n", ch);
            _printNPCLine(ch, f->follower, cfound++);
        }
    }

    if (!gfound && !cfound) {
        send_to_char("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
        return;
    }

    // печатаем чармисов членов группы
    if (PRF_FLAGGED(ch, PRF_SHOWGROUP)) {
        for (auto &it : *this ) {
            auto npc = it.second->member;
            // на всякий пожарный
            if (npc == nullptr) continue; //
            // только чармисов
            if (it.second->type == GM_CHAR) continue;
            // клоны отключены
            if (PRF_FLAGGED(ch, PRF_NOCLONES) && (MOB_FLAGGED(npc, MOB_CLONE) || GET_MOB_VNUM(npc) == MOB_KEEPER))
                    continue;
            // чармисов игрока вывели ранее
            if (ch == npc->get_master()) continue;
            if (!cfound)
                send_to_char("Последователи членов вашей группы:\r\n", ch);
            _printNPCLine(ch, npc, cfound++);
        }
    }
}

void Group::charDataPurged(CHAR_DATA *ch) {
    for (auto &it : *this) {
        if (ch == it.second->member) {
            it.second->member = nullptr;
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
        for (auto & it : *this ) {
            send_to_char(it.second->member, "%s%s", smallBuf, "\r\n");
        }
    else if (mode == GRP_COMM_LEADER) {
        send_to_char(_leader, "%s%s", smallBuf, "\r\n");
    }
}

// надстройка над act, пока только про персонажей
void Group::actToGroup(CHAR_DATA* vict, GRP_COMM mode, const char *msg, ...) {
    if (vict == nullptr)
        return;
    va_list args;
    va_start(args, msg);
    vsnprintf(smallBuf, sizeof(smallBuf), msg, args);
    va_end(args);
    CHAR_DATA* to;
    if (mode == GRP_COMM_LEADER) {
        act(smallBuf, FALSE, _leader, nullptr, vict, TO_CHAR);
        return;
    }
    for (auto &it : *this) {
        to = it.second->member;
        if ( to == nullptr || to->purged()) {
            continue;
        }
        act(smallBuf, FALSE, to, nullptr, vict, TO_CHAR);
    }
}

void Group::addFollowers(CHAR_DATA *leader) {
    for (auto f = leader->followers; f; f = f->next) {
        if (IS_NPC(f->follower))
            continue;
        if ((u_short)this->size() < (u_short)_memberCap)
            this->addMember(f->follower);
    }
}

// метод может вернуть мусор :(
CHAR_DATA* Group::get_random_pc_group() {
    u_short rnd = number(0, (u_short)this->size() - 1);
    int i = 0;
    for (const auto& it : *this){
        if (i == rnd) {
            return it.second->member; //
        }
        i++;
    }
    return nullptr;
}

u_short Group::size(rnum_t room_rnum) {
    u_short  retval = 0;
    for (const auto& it : *this){
        auto c = it.second->member;
        if (room_rnum == (room_rnum == 0 ? room_rnum : c->in_room)  && !c->purged() && retval < 255)
            retval++;
    }
    return retval;
}

bool Group::has_clan_members_in_group(CHAR_DATA *victim) {
    for (const auto& it : *this) {
        auto gm = it.second->member;
        if (gm == nullptr || it.second->type == GM_CHARMEE)
            continue;
        if (CLAN(gm) && SAME_ROOM(gm, victim))
            return true;
    }
    return false;
}

int Group::_calcUID(CHAR_DATA *ch) {
    if (IS_NPC(ch))
        return ch->id;
    else
        return ch->get_uid();

}

void Group::_promote(CHAR_DATA *member) {
    _setLeader(member);
    sendToGroup(GRP_COMM_ALL, "Изменился лидер группы на %.", _leaderName.c_str() );
    auto diff = (u_short)_memberCap - (u_short)this->size();
    if (diff < 0) diff = 0;
    if (diff > 0){
        u_short i = 0;
        int expellList[diff];
        for (auto it = this->begin(); it!= this->end() || i > diff; it++){
            if (it->second->member != _leader ) {
                expellList[i] = _calcUID(it->second->member);
                ++i;
            }
        }
        for (i=0; i < diff; i++){
            _removeMember(expellList[i]);
        }
    }

}


bool same_group(CHAR_DATA * ch, CHAR_DATA * tch)
{
    if (!ch || !tch)
        return false;
    if (ch->personGroup == tch->personGroup)
        return true;
    return false;
}


/*++
   Функция начисления опыта
      ch - кому опыт начислять
           Вызов этой функции для NPC смысла не имеет, но все равно
           какие-то проверки внутри зачем то делаются
--*/
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef)
{


// Странно, но для NPC эта функция тоже должна работать
//  if (IS_NPC(ch) || !OK_GAIN_EXP(ch,victim))
    if (!OK_GAIN_EXP(ch, victim))
    {
        send_to_char("Ваше деяние никто не оценил.\r\n", ch);
        return;
    }

    // 1. Опыт делится поровну на всех
    int exp = GET_EXP(victim) / MAX(members, 1);

    if(victim->get_zone_group() > 1 && members < victim->get_zone_group())
    {
        // в случае груп-зоны своего рода планка на мин кол-во человек в группе
        exp = GET_EXP(victim) / victim->get_zone_group();
    }

    // 2. Учитывается коэффициент (лидерство, разность уровней)
    //    На мой взгляд его правильней использовать тут а не в конце процедуры,
    //    хотя в большинстве случаев это все равно
    exp = exp * koef / 100;

    // 3. Вычисление опыта для PC и NPC
    if (IS_NPC(ch))
    {
        exp = MIN(max_exp_gain_npc, exp);
        exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
    }
    else
        exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim));
    // 4. Последняя проверка
    exp = MAX(1, exp);
    if (exp > 1)
    {
        if (Bonus::is_bonus(Bonus::BONUS_EXP))
        {
            exp *= Bonus::get_mult_bonus();
        }

        if (!IS_NPC(ch) && !ch->affected.empty()) {
            for (const auto& aff : ch->affected) {
                if (aff->location == APPLY_BONUS_EXP) {
                    // скушал свиток с эксп бонусом
                    exp *= MIN(3, aff->modifier); // бонус макс тройной
                }
            }
        }

        int long_live_bonus = floor(ExpCalc::get_npc_long_live_exp_bounus( GET_MOB_VNUM(victim)));
        if (long_live_bonus>1)	{
            std::string mess;
            switch (long_live_bonus) {
                case 2:
                    mess = "Редкая удача! Опыт повышен!\r\n";
                    break;
                case 3:
                    mess = "Очень редкая удача! Опыт повышен!\r\n";
                    break;
                case 4:
                    mess = "Очень-очень редкая удача! Опыт повышен!\r\n";
                    break;
                case 5:
                    mess = "Вы везунчик! Опыт повышен!\r\n";
                    break;
                case 6:
                    mess = "Ваша удача велика! Опыт повышен!\r\n";
                    break;
                case 7:
                    mess = "Ваша удача достигла небес! Опыт повышен!\r\n";
                    break;
                case 8:
                    mess = "Ваша удача коснулась луны! Опыт повышен!\r\n";
                    break;
                case 9:
                    mess = "Ваша удача затмевает солнце! Опыт повышен!\r\n";
                    break;
                case 10:
                    mess = "Ваша удача выше звезд! Опыт повышен!\r\n";
                    break;
                default:
                    mess = "Ваша удача выше звезд! Опыт повышен!\r\n";
                    break;
            }
            send_to_char(mess.c_str(), ch);
        }

        exp = MIN(max_exp_gain_pc(ch), exp);
        send_to_char(ch, "Ваш опыт повысился на %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
    }
    else if (exp == 1) {
        send_to_char("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);
    }
    gain_exp(ch, exp);
    GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
    TopPlayer::Refresh(ch);

    if (!EXTRA_FLAGGED(victim, EXTRA_GRP_KILL_COUNT)
        && !IS_NPC(ch)
        && !IS_IMMORTAL(ch)
        && IS_NPC(victim)
        && !IS_CHARMICE(victim)
        && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
    {
        mob_stat::add_mob(victim, members);
        EXTRA_FLAGS(victim).set(EXTRA_GRP_KILL_COUNT);
    }
    else if (IS_NPC(ch) && !IS_NPC(victim)
             && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
    {
        mob_stat::add_mob(ch, 0);
    }
}

/*++
   Функция расчитывает всякие бонусы для группы при получении опыта,
 после чего вызывает функцию получения опыта для всех членов группы
 Т.к. членом группы может быть только PC, то эта функция раздаст опыт только PC

   ch - обязательно член группы, из чего следует:
            1. Это не NPC
            2. Он находится в группе лидера (или сам лидер)

   Просто для PC-последователей эта функция не вызывается

--*/

void group_gain(CHAR_DATA * killer, CHAR_DATA * victim)
{
    int inroom_members, koef = 100, maxlevel;
    struct follow_type *f;
    int partner_count = 0;
    int total_group_members = 1;
    bool use_partner_exp = false;

    // если наем лидер, то тоже режем экспу
    if (can_use_feat(killer, CYNIC_FEAT)) {
        maxlevel = 300;
    }
    else {
        maxlevel = GET_LEVEL(killer);
    }

    auto leader = killer->get_master();
    if (nullptr == leader) {
        leader = killer;
    }

    // k - подозрение на лидера группы
    const bool leader_inroom = AFF_FLAGGED(leader, EAffectFlag::AFF_GROUP)
                               && SAME_ROOM(leader, killer);

    // Количество согрупников в комнате
    if (leader_inroom) {
        inroom_members = 1;
        maxlevel = GET_LEVEL(leader);
    }
    else {
        inroom_members = 0;
    }

    // Вычисляем максимальный уровень в группе
    for (f = leader->followers; f; f = f->next)
    {
        if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)) ++total_group_members;
        if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
            && SAME_ROOM(f->follower, killer))
        {
            // если в группе наем, то режим опыт всей группе
            // дабы наема не выгодно было бы брать в группу
            // ставим 300, чтобы вообще под ноль резало
            if (can_use_feat(f->follower, CYNIC_FEAT))
            {
                maxlevel = 300;
            }
            // просмотр членов группы в той же комнате
            // член группы => PC автоматически
            ++inroom_members;
            maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
            if (!IS_NPC(f->follower))
            {
                partner_count++;
            }
        }
    }

    GroupPenaltyCalculator group_penalty(killer, leader, maxlevel, groupRoster.grouping);
    koef -= group_penalty.get();

    koef = MAX(0, koef);

    // Лидерство используется, если в комнате лидер и есть еще хоть кто-то
    // из группы из PC (последователи типа лошади или чармисов не считаются)
    if (koef >= 100 && leader_inroom && (inroom_members > 1) && calc_leadership(leader))
    {
        koef += 20;
    }

    // Раздача опыта

    // если групповой уровень зоны равняется единице
    if (zone_table[world[killer->in_room]->zone].group < 2)
    {
        // чтобы не абьюзили на суммонах, когда в группе на самом деле больше
        // двух мемберов, но лишних реколят перед непосредственным рипом
        use_partner_exp = total_group_members == 2;
    }

    // если лидер группы в комнате
    if (leader_inroom)
    {
        // если у лидера группы есть способность напарник
        if (can_use_feat(leader, PARTNER_FEAT) && use_partner_exp)
        {
            // если в группе всего двое человек
            // k - лидер, и один последователь
            if (partner_count == 1)
            {
                // и если кожф. больше или равен 100
                if (koef >= 100)
                {
                    if (leader->get_zone_group() < 2)
                    {
                        koef += 100;
                    }
                }
            }
        }
        perform_group_gain(leader, victim, inroom_members, koef);
    }

    for (f = leader->followers; f; f = f->next)
    {
        if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
            && SAME_ROOM(f->follower, killer))
        {
            perform_group_gain(f->follower, victim, inroom_members, koef);
        }
    }
}


int calc_leadership(CHAR_DATA * ch)
{
    int prob, percent;

    if (IS_NPC(ch) || ch->personGroup == nullptr) {
        return FALSE;
    }

    CHAR_DATA* leader = ch->personGroup->getLeader();

    // если лидер умер или нет в комнате - фиг вам, а не бонусы
    if (leader == nullptr || IN_ROOM(ch) != IN_ROOM(leader)) {
        return FALSE;
    }

    if (!leader->get_skill(SKILL_LEADERSHIP))	{
        return (FALSE);
    }

    percent = number(1, 101);
    prob = calculate_skill(leader, SKILL_LEADERSHIP, 0);
    if (percent > prob)  {
        return (FALSE);
    }
    else {
        return (TRUE);
    }
}
