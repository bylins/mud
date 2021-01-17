/* ************************************************************************
*   File: grp.main.h                                    Part of Bylins    *
*   Заголовок всех функций и классов работы с группами                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
************************************************************************ */

#ifndef BYLINS_GRP_MAIN_H
#define BYLINS_GRP_MAIN_H

#include <utility>
#include "chars/char.hpp"
#include "structs.h"
#include "dps.hpp"

enum GM_TYPE {GM_CHAR, GM_CHARMEE};
enum RQ_TYPE {RQ_GROUP, RQ_PERSON, RQ_ANY};


enum GRP_COMM : short {
    GC_LEADER = 1, // только лидеру
    GC_CHAR = 2, // только персонажу
    GC_REST = 4, // всем остальным
    GC_ROOM = 8 // эхо в комнату
};

enum RQ_R {RQ_R_OK, RQ_R_NO_GROUP, RQ_R_OVERFLOW, RQ_REFRESH};
enum INV_R {INV_R_OK, INV_R_NO_PERSON, INV_R_BUSY, INV_R_REFRESH};

void do_grequest(CHAR_DATA *ch, char *argument, int, int);
void do_group2(CHAR_DATA *ch, char *argument, int, int);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
int max_group_size(CHAR_DATA *ch);
bool isGroupedFollower(CHAR_DATA* master, CHAR_DATA* vict);
bool circle_follow(CHAR_DATA * ch, CHAR_DATA * victim);
bool die_follower(CHAR_DATA * ch);
bool stop_follower(CHAR_DATA * ch, int mode);
// возвращает true, если рядом с персонажем лидер группы и прокнул скилл
bool calc_leadership(CHAR_DATA * ch);

class Request;

using namespace std::chrono;

using sclock_t = time_point<std::chrono::steady_clock>;

struct char_info {
    char_info(int memberUid, GM_TYPE type, CHAR_DATA *member, const std::string& memberName);
    virtual ~char_info();
    int memberUID; // часть ключа
    GM_TYPE type; // тип персонажа
    CHAR_DATA * member; // ссылка на персонажа, может быть невалидна!
    std::string memberName; // бэкап имени, если персонаж оффлайн
    sclock_t expiryTime; // время, когда запись автоматом удаляется после проверок.
};

struct PenaltyCalcData {
    int penalty;
};

using grp_mt = std::map<int, std::shared_ptr<char_info>>;
using grp_ptr = std::shared_ptr<Group>;
using rq_ptr = std::shared_ptr<Request>;
using cd_v = std::vector<CHAR_DATA*>;
using npc_r = std::unordered_set<CHAR_DATA *> *;

const duration DEF_EXPIRY_TIME = 600s;

inline bool IN_GROUP(CHAR_DATA* ch) {return ch != nullptr && ch->personGroup != nullptr;}
inline bool IN_SAME_GROUP(CHAR_DATA* p1, CHAR_DATA* p2) {return IN_GROUP(p1) && IN_GROUP(p2) && p1->personGroup == p2->personGroup;}

// класс, хранящий обвязку штрафов по экспе для группы
// наркоман писал, не иначе
class GroupPenalties
{
public:
    using class_penalties_t = std::array<int, MAX_REMORT + 1>;
    using penalties_t = std::array<class_penalties_t, NUM_PLAYER_CLASSES>;

    int init();
    const auto& operator[](const size_t player_class) const { return m_grouping[player_class]; }
private:
    penalties_t m_grouping;
};


// класс-коллекция персонажей обоих типов что ли..
class Group : public grp_mt {
private:
    constexpr static int DEFAULT_100 = 100;
    // ид группы в ростере
    u_long _uid = 0;
    //макс.количество игроков
    int _memberCap = 0;
    // ид лидера
    int _leaderUID;
    // имя лидера
    std::string _leaderName;
    // ссылка на персонажа, АХТУНГ! Может меняться и быть невалидным
    CHAR_DATA* _leader = nullptr;
    // макс.уровень игрока в группе, для расчета штрафа
    u_short _maxPlayerLevel = 1;
    // количество игроков
    u_short _pcCount = 0;
public:
    u_long getUid() const;
    const std::string &getLeaderName() const;
    CHAR_DATA *getLeader() const;
    void _clear(bool silent);
    Group(CHAR_DATA *leader, u_long uid);
    ~Group();
    void _setLeader(CHAR_DATA *leader);
    int _getMemberCap() const;
    bool _isFull();
    bool _isActive(); // проверка, что в группе все персонажи онлайн
    bool _isMember(int uid);
    const char* _getMemberName(int uid);
    char_info* _findMember(char* memberName);
    bool _removeMember(CHAR_DATA* ch);
    void _promote(CHAR_DATA* ch);
    void charDataPurged(CHAR_DATA* ch);
    u_short get_size(rnum_t room_rnum = 0);
private:
    bool _removeMember(int uid);
    GM_TYPE getType(CHAR_DATA* ch) {
        if (IS_NPC(ch))
            return  GM_CHARMEE;
        else
            return GM_CHAR;
    }
    int _calcUID(CHAR_DATA* ch);

    static void _printHeader(CHAR_DATA* ch, bool npc);
    static void _printDeadLine(CHAR_DATA* ch, const char* playerName, int header);
    static void _printNPCLine(CHAR_DATA* ch, CHAR_DATA* npc, int header);
    static void _printPCLine(CHAR_DATA* ch, CHAR_DATA* pc, int header);
    bool _sameGroup(CHAR_DATA * ch, CHAR_DATA * vict);
public:


    void addFollowers(CHAR_DATA* leader);
    void addMember(CHAR_DATA *member, bool silent = false);
    void expellMember(char* memberName);
    bool _restoreMember(CHAR_DATA *member);

    void printGroup(CHAR_DATA *requestor);
    void listMembers(CHAR_DATA *requestor);

    void promote(char *applicant);
    void approveRequest(const char *applicant);
    void rejectRequest(char *applicant);
    void leaveGroup(CHAR_DATA* vict);

//    void sendToGroup(GRP_COMM mode, const char *msg, ...);
    void actToGroup(CHAR_DATA* ch, CHAR_DATA* vict, int mode, const char *msg, ...);
    u_short calcExpMultiplicator(const CHAR_DATA* player);
public:
    // всякий унаследованный стафф
    CHAR_DATA* get_random_pc_group();
    // лень обвязывать, тупо переместил объект
    DpsSystem::GroupListType _group_dps;
    bool has_clan_members_in_group(CHAR_DATA *victim);
    // группа получает экспу за убивство
    void gainExp(CHAR_DATA * victim);
};

class Request {
public:
    sclock_t _time;
    CHAR_DATA *_applicant;
    Group* _group;
    std::string _applicantName;
    int _applicantUID;
    RQ_TYPE _type;
    Request(CHAR_DATA* subject, Group* group, RQ_TYPE type);
};


class GroupRoster {
// properties
public:
    GroupRoster();
    void charDataPurged(CHAR_DATA* ch); // очистка заявок при пурже персонажа
    void restorePlayerGroup(CHAR_DATA* ch); // возвращает игрока в группу после смерти
    void processGroupCommands(CHAR_DATA *ch, char *argument);
    void printList(CHAR_DATA *ch);
private:
    u_long _currentGroupIndex = 0;
    std::map<u_long, grp_ptr> _groupList;
// методы работы с группами
public:
    grp_ptr addGroup(CHAR_DATA* leader);
    void removeGroup(u_long uid);
    grp_ptr findGroup(int personUID);
    grp_ptr findGroup(char* leaderName);
    void runTests(CHAR_DATA* leader);
private:
    std::list<rq_ptr> _requestList;
    std::tuple<INV_R, CHAR_DATA *> tryMakeInvite(Group* grp, char* member);
    std::tuple<RQ_R, grp_ptr> tryAddRequest(CHAR_DATA* author, char* targetGroup);
public:
    // методы работы с заявками
    void printRequestList(CHAR_DATA* ch);
    void makeInvite(CHAR_DATA* leader, char* targetPerson);
    void makeRequest(CHAR_DATA* author, char* targetGroup);
    void revokeRequest(CHAR_DATA* author, char* targetGroup);
    void acceptInvite(CHAR_DATA* who, char* author);
    void deleteRequest(Request * r);
    Request* findRequest(const char* targetPerson, const char* group, RQ_TYPE type);
    GroupPenalties grouping;
};


//extern GroupPenalties grouping;
#endif //BYLINS_GRP_MAIN_H
