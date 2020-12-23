#ifndef BYLINS_GRP_GROUP_H
#define BYLINS_GRP_GROUP_H

#include "chars/char.hpp"
#include "grp.membermanager.h"

void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_group2(CHAR_DATA *ch, char *argument, int, int);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
int max_group_size(CHAR_DATA *ch);

enum GRP_COMM {GRP_COMM_LEADER, GRP_COMM_ALL};


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
    MemberManager * _memberManager;
public:
    MemberManager *getMemberManager() const {return _memberManager;}
public:
    u_long getUid() const;
    u_short getCurrentMemberCount() const;
    const std::string &getLeaderName() const;
    CHAR_DATA *getLeader() const;
    void setLeader(CHAR_DATA *leader);
    int getMemberCap() const;
    virtual ~Group();
    bool isFull();
    Group(CHAR_DATA *leader, u_long uid);
    void printGroup(CHAR_DATA *requestor);
    void listMembers(CHAR_DATA *requestor);
    bool isActive(); // проверка, что в группе все персонажи онлайн

    // обработчик команд группы
    void processGroupCommands(CHAR_DATA *ch, char *argument);

    bool promote(char *applicant);
    void sendToGroup(GRP_COMM mode, const char *msg, ...);
};


#endif //BYLINS_GRP_GROUP_H
