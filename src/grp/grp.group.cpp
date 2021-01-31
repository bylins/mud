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

CHAR_DATA* Group::getLeader() const{
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
    _memberCap = IS_NPC(leader)? 255 : grp::max_group_size(leader) + 1; // функция возвращает кол-во ПОСЛЕДОВАТЕЛЕЙ
}

bool Group::_isActive() {
    return false;
}

u_long Group::getUid() const {
    return _uid;
}

int Group::_getMemberCap() const {
    if (_leader != nullptr)
        return grp::max_group_size(_leader) + 1;
    return _memberCap;
}

const std::string &Group::getLeaderName() const {
    return _leaderName;
}

bool Group::_isFull() {
    if (this->_pcCount >= _getMemberCap())
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
        member->personGroup->_removeMember(member);

    auto it = this->find(_calcUID(member));
    if (it == this->end()) {
        auto ci = char_info(_calcUID(member), getType(member), member, member->get_pc_name());
        this->emplace(_calcUID(member), std::make_shared<char_info>(ci));
        if (!IS_NPC(member))
            _pcCount++;
        if (!silent) {
            actToGroup(nullptr, member, GC_LEADER, "$N принят$A в члены вашего кружка (тьфу-ты, группы :).");
            actToGroup(member, _leader, GC_CHAR , "Вы приняты в группу $N1.");
            actToGroup(nullptr, member, GC_REST , "$N принят$A в группу.");
        }
    } else {
        sprintf(buf, "Group::addMember: группа id=%lu, попытка повторного добавления персонажа с тем же uid",
                getUid());
        mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
        return;
    }
    member->personGroup = this;
    if (!IS_NPC(member) && _maxPlayerLevel < GET_LEVEL(member))
        _maxPlayerLevel = GET_LEVEL(member);

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
    _updateMemberCap();
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
    if (member != nullptr) {
        send_to_char(member, "Вы покинули группу.\r\n");
        member->personGroup = nullptr;
    }
    actToGroup(nullptr, it->second->member, GC_LEADER | GC_REST , "$N покинул$G группу.");
    if (it->second->type == GM_CHAR)
        _pcCount--;
    this->erase(memberUID); // finally remove it
    // пересчитываем макс.уровень
    {
        _maxPlayerLevel = 1;
        for (const auto& m: *this) {
            if (m.second->member == nullptr || m.second->type == GM_CHARMEE)
                continue;
            if (GET_LEVEL(m.second->member) > _maxPlayerLevel)
                _maxPlayerLevel = GET_LEVEL(m.second->member);
        }
    }

    // а также удаляем чармисов персонажа из групповых
    if (member != nullptr && member->followers != nullptr) {
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

    CHAR_DATA* nxtLdr = getLeader();
    int nxtLdrGrpSize = 0;
    // если ушел лидер - ищем нового, но после ухода предыдущего
    if (_leaderUID == memberUID) {
        nxtLdr = nullptr;
        for (const auto& mit : *this){
            auto m = mit.second->member;
            if ( m != nullptr && !m->purged() && grp::max_group_size(m) > nxtLdrGrpSize ) {
                nxtLdr = m; // нашелся!!
                nxtLdrGrpSize = grp::max_group_size(m);
            }
        }
    }
    // нового лидера не нашлось, распускаем группу
    if (nxtLdr == nullptr) {
        groupRoster.removeGroup(_uid);
        return false;
    }
    _promote(nxtLdr);
    _updateMemberCap();
    return true;
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
    sprintf(buf, "[Group::clear()]: this: %lu", this->size());
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
        actToGroup(_leader, nullptr, GC_LEADER, "Нет такого персонажа.");
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
        actToGroup(_leader, nullptr, GC_LEADER, "Нет такого персонажа.");
        return;
    }
    if (vict != nullptr && vict->member != nullptr) {
        act("$N исключен$A из состава вашей группы.", FALSE, _leader, nullptr, vict->member, TO_CHAR);
        act("Вы исключены из группы $n1!", FALSE, _leader, nullptr, vict->member, TO_VICT);
        act("$N был$G исключен$A из группы $n1!", FALSE, _leader, nullptr, vict->member, TO_NOTVICT | TO_ARENA_LISTEN);
    } else {
        actToGroup(nullptr, nullptr, GC_ROOM, "%s был исключен из группы.'\r\n", vict->memberName.c_str());
    }
    _removeMember(vict->memberUID);
}

void Group::listMembers(CHAR_DATA *ch) {
    CHAR_DATA *leader;
    int count = 1;
    u_short inRoomCount = 0;

    if (ch->personGroup)
    {
        leader = ch->personGroup->getLeader();
        for (const auto rm : world[ch->in_room]->people) {
            if (IN_SAME_GROUP(ch, rm))
                inRoomCount++;
        }
        for (auto & it : *this ) {
            if (it.second->member == leader)
                continue;
            count++;
            if (count == 2){
                send_to_char( ch, "Ваша группа [%d/%d/%d]:\r\n", _pcCount, _getMemberCap(), inRoomCount );
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

    if (!IS_NPC(ch))
        ch->desc->msdp_report(msdp::constants::GROUP);

    // печатаем группу
    if (ch->personGroup != nullptr ) {
        send_to_char("Ваша группа состоит из:\r\n", ch);
        for (auto &it : *this ){
            if (it.second->type == GM_CHARMEE)
                continue; // чармисов скипуем и показываем ниже
            if (it.second->member == nullptr || it.second->member->purged() || it.second->member->desc == NULL)
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
    if (PRF_FLAGGED(ch, PRF_SHOWGROUP) && ch->personGroup != nullptr) { //
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
            it.second->expiryTime = steady_clock::now() + DEF_EXPIRY_TIME;
            return;
        }
    }
}

/*
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
*/

// надстройка над act, регулируется набором флагов grpActMode + GRP_COMM
// ch указывается для указания цели в режиме GC_CHAR и GC_ROOM
// vict передается в аргумент функции act
void Group::actToGroup(CHAR_DATA* ch, CHAR_DATA* vict, int mode, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vsnprintf(smallBuf, sizeof(smallBuf), msg, args);
    va_end(args);
    CHAR_DATA* to;
    for (auto &it : *this) {
        to = it.second->member;
        if ( to == nullptr || to->purged()) {
            continue;
        }
        if ( (mode & GC_LEADER && to == _leader) ||
            (mode & GC_CHAR && to == ch) ||
            (mode & GC_REST && to != vict && to != _leader))
            act(smallBuf, TRUE, to, nullptr, vict, TO_CHAR);
    }
    if (mode & GC_ROOM)
        act(smallBuf, TRUE, ch, nullptr, vict, TO_ROOM | TO_ARENA_LISTEN);
}

bool Group::addFollowers(CHAR_DATA *leader) {
    bool result = false;
    for (auto f = leader->followers; f; f = f->next) {
        if (IS_NPC(f->follower))
            continue;
        if ((u_short)this->get_size() < (u_short)_getMemberCap()) {
            this->addMember(f->follower);
            result = true;
        }
    }
    return result;
}

// метод может вернуть мусор :(
CHAR_DATA* Group::get_random_pc_group() {
    u_short rnd = number(0, (u_short)this->get_size() - 1);
    int i = 0;
    for (const auto& it : *this){
        if (i == rnd) {
            return it.second->member; //
        }
        i++;
    }
    return nullptr;
}

u_short Group::get_size(rnum_t room_rnum) {
    u_short  retval = 0;
    for (const auto& it : *this){
        auto c = it.second->member;
        if (c == nullptr)
            continue;
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
    if (_leader == member)
        return;
    _setLeader(member);
    actToGroup(nullptr, nullptr, GC_LEADER | GC_REST, "Изменился лидер группы на %s.", _leaderName.c_str());
    auto diff = (u_short)_getMemberCap() - (u_short)this->size();
    if (diff < 0) diff = abs(diff); else diff = 0;
    if (diff > 0){
        u_short i = 0;
        int expellList[diff];
        for (auto it = this->begin(); it != this->end() && i < diff; it++){
            if (it->second->member != _leader ) {
                expellList[i] = _calcUID(it->second->member);
                actToGroup(nullptr, nullptr, GC_LEADER | GC_REST, "В группе не хватает места, нас покинул %s!", it->second->memberName.c_str());
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
    if (!ch || !tch) return false;
    if (ch == tch) return true;
    // нпц по-умолчанию в группе, если одного алайнмента
    if (IS_NPC(ch) && IS_NPC(tch) && SAME_ALIGN(ch, tch)) return true;
    if (ch->personGroup != nullptr && tch->personGroup != nullptr && ch->personGroup == tch->personGroup)
        return true;
    return false;
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

void Group::gainExp(CHAR_DATA * victim) {
    int inroom_members = 0;
    u_short expMultiplicator = 0;
    u_short expDivider = 1;
    CHAR_DATA* ch;

    // на всякий пожарный проверим
    if (!victim)
        return;

    // есть ли в комнате живой лидер
    const bool leader_inroom = SAME_ROOM(_leader, victim);

    // прежде чем раздать опыт, считаем количество персонажей в клетке
    for (auto it: *this) {
        ch = it.second->member;
        if (ch != nullptr && SAME_ROOM(ch, victim) && it.second->type == GM_CHAR) {
            ++inroom_members;
        }
    }
    // есть партнерка и персонажей двое и зона негрупповая
    if (leader_inroom and inroom_members == 2 and can_use_feat(_leader, PARTNER_FEAT) and _leader->get_zone_group() <2)
        expDivider = 1;
    else
        expDivider = inroom_members;

    // погнали раздавать опыт
    for (auto m: *this) {
        ch = m.second->member;
        if (ch == nullptr || ch->purged() || m.second->type == GM_CHARMEE || !SAME_ROOM(ch, victim) ) {
            continue;
        }
        expMultiplicator = calcExpMultiplicator(ch); // цифра процента
        // если прокнула лидерка, то добавляем еще 20
        if (leader_inroom && calc_leadership(_leader) and inroom_members > 1)
            expMultiplicator += 20;
        increaseExperience(ch, victim, expDivider, expMultiplicator);
    }
}

bool calc_leadership(CHAR_DATA * ch)
{
    int prob, percent;

    if (IS_NPC(ch) || ch->personGroup == nullptr) {
        return false;
    }

    CHAR_DATA* leader = ch->personGroup->getLeader();

    // если лидер умер или нет в комнате - фиг вам, а не бонусы
    if (!SAME_ROOM(ch, leader)) {
        return false;
    }

    if (!leader->get_skill(SKILL_LEADERSHIP))	{
        return false;
    }

    percent = number(1, 101);
    prob = calculate_skill(leader, SKILL_LEADERSHIP, 0);
    if (percent > prob)  {
        return false;
    }
    else {
        return true;
    }
}


u_short Group::calcExpMultiplicator(const CHAR_DATA* player)
{
    const int player_remorts = static_cast<int>(GET_REMORT(player));
    const int player_class = static_cast<int>(GET_CLASS(player));
    const int player_level = GET_LEVEL(player);
    short result = DEFAULT_100;

    if (IS_NPC(player))
    {
        log("LOGIC ERROR: try to get penalty for NPC [%s], VNum: %d\n",
            player->get_name().c_str(),
            GET_MOB_VNUM(player));
        return result;
    }

    if (0 > player_class || player_class > NUM_PLAYER_CLASSES) {
        log("LOGIC ERROR: wrong player class: %d for player [%s]",
            player_class,
            player->get_name().c_str());
        return result;

    }

    if (0 > player_remorts || player_remorts > MAX_REMORT){
        log("LOGIC ERROR: wrong number of remorts: %d for player [%s]",
            player_remorts,
            player->get_name().c_str());
        return result;
    }

    if (_maxPlayerLevel - player_level > groupRoster.getPenalty(player)) {
        return MAX(1, DEFAULT_100 - 3 * (_maxPlayerLevel - player_level));
    }
    return DEFAULT_100;
}

void Group::_updateMemberCap() {
    if (_leader == nullptr || _leader->purged())
        return;
    _memberCap = _getMemberCap();
}

void Group::processGarbageCollection() {
    std::vector<int> expellV; // массивчик персонажей, которых надо удалить из группы
    for (const auto& it : *this) {
        auto m = it.second;
        if (m->expiryTime <= steady_clock::now() && m->member == nullptr) {
            expellV.push_back(m->memberUID);
        }
    }
    if (!expellV.empty())
        for (auto it : expellV) {
            _removeMember(it);
        }
}

