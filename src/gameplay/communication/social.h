/**
 \authors Created by Sventovit
 \date 20.01.2021.
 \brief Заголовок модуля социалов.
*/

#ifndef BYLINS_SRC_COMMUNICATION_SOCIAL_H_
#define BYLINS_SRC_COMMUNICATION_SOCIAL_H_

#include "engine/entities/entities_constants.h"

struct SocialMessages {
	EPosition ch_min_pos = EPosition::kDead;
	EPosition ch_max_pos = EPosition::kDead;
	EPosition vict_min_pos = EPosition::kDead;
	EPosition vict_max_pos = EPosition::kDead;
	char *char_no_arg = nullptr;
	char *others_no_arg = nullptr;

	// An argument was there, and a victim was found //
	char *char_found = nullptr;    // if NULL, read no further, ignore args //
	char *others_found = nullptr;
	char *vict_found = nullptr;

	// An argument was there, but no victim was found //
	char *not_found = nullptr;
};

struct SocialKeyword {
	char *keyword = nullptr;
	int social_message = 0;
};

extern struct SocialMessages *soc_mess_list;
extern struct SocialKeyword *soc_keys_list;

extern int number_of_social_commands;
extern int number_of_social_messages;

int find_action(char *cmd);
int do_social(CharData *ch, char *argument);
void GoBootSocials();

#endif //BYLINS_SRC_COMMUNICATION_SOCIAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
