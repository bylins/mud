#ifndef BYLINS_GRP_GROUP_H
#define BYLINS_GRP_GROUP_H

#include "chars/char.hpp"
#include "grp.roster.h"

void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
int max_group_size(CHAR_DATA *ch);

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
    CHAR_DATA* _leader;
    // состав группы, ключом UID персонажа. Дохлые/вышедшие проверяем отдельным методом
    std::map<int, CHAR_DATA *> _memberList;
    // список заявок ключ UID персонажа. При принятии одобряется, при отказе удаляется.
    std::map<int, CHAR_DATA *> _requestList;
public:
    u_long getUid() const;
    u_short getCurrentMemberCount() const;
    CHAR_DATA *getLeader() const;
    void setLeader(CHAR_DATA *leader);
    int getMemberCap() const;

    virtual ~Group();

    Group(CHAR_DATA *leader, u_long uid);
    void addMember(CHAR_DATA *member);
    void removeMember(CHAR_DATA *member);
    bool checkMember(int uid); // проверки валидности персонажа

    void print(CHAR_DATA *member);
    bool isActive(); // проверка, что в группе все персонажи онлайн

    bool applyRequest(CHAR_DATA *applicant);
    bool approveRequest(CHAR_DATA *applicant);
    bool denyRequest(CHAR_DATA *applicant);
};

namespace GCmd{
    // команда роспуска группы
    void do_gabandon(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
    // команда приглашения лидером персонажа в группу
    void do_ginvite(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
    // команда выхода из группы
    void do_gleave(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
    // команда создания группы
    void do_gmake(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
    // команда переназначения лидера
    void do_gpromote(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
    // команда запроса персонажем в группу
    void do_grequest(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
}

#endif //BYLINS_GRP_GROUP_H
