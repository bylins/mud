/**
 \authors Created by Sventovit
 \date 20.01.2021.
 \brief Заголовок модуля социалов.
*/

#ifndef BYLINS_SRC_COMMUNICATION_SOCIAL_H_
#define BYLINS_SRC_COMMUNICATION_SOCIAL_H_

class CHAR_DATE;

struct SocialMessages {
	int ch_min_pos = 0;
	int ch_max_pos = 0;
	int vict_min_pos = 0;
	int vict_max_pos = 0;
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

int find_action(char *cmd);
int do_social(CharacterData *ch, char *argument);

#endif //BYLINS_SRC_COMMUNICATION_SOCIAL_H_
