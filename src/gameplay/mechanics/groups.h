/**
\file groups.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Код групп игроков и мобов.
\detail Группы, вступление, покидание, дележка опыта - должно быть тут.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_GROUPS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_GROUPS_H_

class CharData;
namespace group {
bool same_group(CharData *ch, CharData *tch);
int perform_group(CharData *ch, CharData *vict);
int max_group_size(CharData *ch);
void GoGroup(CharData *ch, char *argument);
void GoUngroup(CharData *ch, char *argument);
void print_list_group(CharData *ch);
void print_group(CharData *ch);
void print_one_line(CharData *ch, CharData *k, int leader, int header);
void do_report(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, int currency);
}  // namespace group
#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_GROUPS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
