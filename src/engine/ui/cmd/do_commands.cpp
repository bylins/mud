/**
\file do_commands.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "gameplay/communication/social.h"

void do_commands(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int no, i, cmd_num, num_of;
	int wizhelp = 0, socials = 0;
	CharData *vict = ch;

	one_argument(argument, arg);

	if (subcmd == kScmdSocials)
		socials = 1;
	else if (subcmd == kScmdWizhelp)
		wizhelp = 1;

	sprintf(buf, "Следующие %s%s доступны %s:\r\n",
			wizhelp ? "привилегированные " : "",
			socials ? "социалы" : "команды", vict == ch ? "вам" : GET_PAD(vict, 2));

	if (socials)
		num_of = number_of_social_commands;
	else
		num_of = num_of_cmds - 1;

	// cmd_num starts at 1, not 0, to remove 'RESERVED'
	for (no = 1, cmd_num = socials ? 0 : 1; cmd_num < num_of; cmd_num++)
		if (socials) {
			sprintf(buf + strlen(buf), "%-19s", soc_keys_list[cmd_num].keyword);
			if (!(no % 4))
				strcat(buf, "\r\n");
			no++;
		} else {
			i = cmd_sort_info[cmd_num].sort_pos;
			if (wizhelp) {
				if (privilege::HasPrivilege(vict, std::string(cmd_info[i].command), 0, 0, 0)) {
					sprintf(buf + strlen(buf), "%-15s", cmd_info[i].command);
					if (!(no % 5))
						strcat(buf, "\r\n");
					no++;
				}
			} else if (cmd_info[i].minimum_level >= 0 && (static_cast<bool>(socials) == cmd_sort_info[i].is_social)) {
				sprintf(buf + strlen(buf), "%-15s", cmd_info[i].command);
				if (!(no % 5))
					strcat(buf, "\r\n");
				no++;
			}
		}

	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
