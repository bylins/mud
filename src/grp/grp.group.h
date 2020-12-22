#ifndef BYLINS_GRP_GROUP_H
#define BYLINS_GRP_GROUP_H

#include "chars/char.hpp"
#include "grp.roster.h"

void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_group2(CHAR_DATA *ch, char *argument, int, int);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
int max_group_size(CHAR_DATA *ch);


class Group {
private:
    struct char_info {
        int memberUID;
        char_info(int memberUid, CHAR_DATA *member, const std::string &memberName) : memberUID(memberUid),
                                                                                     member(member),
                                                                                     memberName(memberName) {}
        CHAR_DATA * member;
        std::string memberName;
    };
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
    std::map<int, char_info *> _memberList;
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
    void addMember(CHAR_DATA *member);
    void removeMember(CHAR_DATA *member);
    bool checkMember(int uid); // проверки валидности персонажа

    void printGroup(CHAR_DATA *requestor);
    void listMembers(CHAR_DATA *requestor);
    bool isActive(); // проверка, что в группе все персонажи онлайн

    // обработчик команд группы
    void processGroupCommands(CHAR_DATA *ch, char *argument);
    // методы работы с контентом группы
    bool approveRequest(char *applicant);
    bool denyRequest(char *applicant);
    bool expellMember(char *applicant);
    bool promote(char *applicant);
    bool leave(CHAR_DATA *ch);

};

#endif //BYLINS_GRP_GROUP_H
