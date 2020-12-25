#ifndef BYLINS_GRP_MAIN_H
#define BYLINS_GRP_MAIN_H

#include <utility>
#include "chars/char.hpp"

enum RQ_TYPE {RQ_GROUP, RQ_PERSON};
enum GRP_COMM {GRP_COMM_LEADER, GRP_COMM_ALL};
enum RQ_R {RQ_R_OK, RQ_R_NO_GROUP, RQ_R_OVERFLOW, RQ_REFRESH};
enum INV_R {INV_R_OK, INV_R_NO_PERSON, INV_R_BUSY, INV_R_REFRESH};

void do_grequest(CHAR_DATA *ch, char *argument, int, int);
void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_group2(CHAR_DATA *ch, char *argument, int, int);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
int max_group_size(CHAR_DATA *ch);
void do_grequest(CHAR_DATA *ch, char *argument, int, int);


class Group;
class MemberManager;
class RequestManager;
class GroupManager;

using mm_ptr = std::shared_ptr<MemberManager>;
using rm_ptr = std::shared_ptr<RequestManager>;
using gm_ptr = std::shared_ptr<GroupManager>;
using grp_ptr = std::shared_ptr<Group>;

struct char_info {
    char_info(int memberUid, CHAR_DATA *member, std::string memberName);
    virtual ~char_info();
    int memberUID;
    CHAR_DATA * member;
    std::string memberName;
};

class MemberManager {
private:
    std::map<int, std::shared_ptr<char_info>> _memberList;
    grp_ptr _group;
public:
    MemberManager(CHAR_DATA* leader, grp_ptr group);
    ~MemberManager();
    void addMember(CHAR_DATA *member);
    bool removeMember(CHAR_DATA *member);
    bool findMember(int uid);
    bool restoreMember(CHAR_DATA *member);
    u_short getMemberCount() {return (u_short)this->_memberList.size();}
    void clear(bool silentMode); // вызывается при расформировании группы
};

class Group {
private:
    // ид группы в ростере
    u_long _uid = 0;
    // текущее количество игроков
    u_short _currentMemberCount = 0;
    //макс.количество игроков
    int _memberCap = 0;
    // ид лидера
    int _leaderUID;
    // имя лидера
    std::string _leaderName;
    // ссылка на персонажа, АХТУНГ! Может меняться и быть невалидным
    CHAR_DATA* _leader;
    // состав группы, ключом UID персонажа. Дохлые/вышедшие проверяем отдельным методом
    mm_ptr _memberManager;
    void clear();
public:
    mm_ptr getMemberManager() {return _memberManager;}
    u_long getUid() const;
    u_short getCurrentMemberCount() const;
    const std::string &getLeaderName() const;
    CHAR_DATA *getLeader();
    void disband(bool silent);

    ~Group();

    void setLeader(CHAR_DATA *leader);
    int getMemberCap() const;
    bool isFull();
    Group(CHAR_DATA *leader, u_long uid);
    void printGroup(CHAR_DATA *requestor);
    void listMembers(CHAR_DATA *requestor);
    bool isActive(); // проверка, что в группе все персонажи онлайн

    bool promote(char *applicant);
    void sendToGroup(GRP_COMM mode, const char *msg, ...);
};

class Request {
public:
    std::chrono::time_point<std::chrono::system_clock> _time;
    CHAR_DATA *_applicant;
    grp_ptr _group;
    std::string _applicantName;
    int _applicantUID;
    RQ_TYPE _type;
    Request(CHAR_DATA* subject, grp_ptr group, RQ_TYPE type);
};

class GroupManager{
private:
    u_long _currentGroupIndex = 0;
    std::map<u_long, grp_ptr> _groupList;
public:
    grp_ptr addGroup(CHAR_DATA* leader);
    void removeGroup(u_long uid);
    grp_ptr findGroup(CHAR_DATA* ch);
    grp_ptr findGroup(char* leaderName);
    void printList(CHAR_DATA *ch);
};

class RequestManager{
private:
    std::list<Request> _requestList;
    std::tuple<INV_R, CHAR_DATA *> tryMakeInvite(grp_ptr grp, char* member);
    std::tuple<RQ_R, grp_ptr> tryAddRequest(CHAR_DATA* author, char* targetGroup);
public:
    RequestManager();
// методы работы с заявками
    void printRequestList(CHAR_DATA* ch);
    void makeInvite(CHAR_DATA* leader, char* targetPerson);
    void makeRequest(CHAR_DATA* author, char* targetGroup);
    void revokeRequest(CHAR_DATA* author, char* targetGroup);
};

class GroupRoster {
// properties
private:
    rm_ptr _requestManager;
    gm_ptr _groupManager;
public:
    gm_ptr getGroupManager() const {return _groupManager;}
    rm_ptr getRequestManager() const {return _requestManager;}
    GroupRoster();
    void restorePlayerGroup(CHAR_DATA* ch); // возвращает игрока в группу после смерти
    void processGroupCommands(CHAR_DATA *ch, char *argument);
    void printList(CHAR_DATA *ch);
};

#endif //BYLINS_GRP_MAIN_H
