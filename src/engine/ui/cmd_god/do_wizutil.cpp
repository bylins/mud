/**
\file wizutil.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "administration/punishments.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/core/genchar.h"
#include "utils/logger.h"

void do_wizutil(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	long result;
	int times = 0;
	char *reason;
	char num[kMaxInputLength];

	//  one_argument(argument, arg);
	reason = two_arguments(argument, arg, num);

	if (!*arg)
		SendMsgToChar("Для кого?\r\n", ch);
	else if (!(vict = get_player_pun(ch, arg, EFind::kCharInWorld)))
		SendMsgToChar("Нет такого игрока.\r\n", ch);
	else if (GetRealLevel(vict) > GetRealLevel(ch) && !GET_GOD_FLAG(ch, EGf::kDemigod)
		&& !ch->IsFlagged(EPrf::kCoderinfo))
		SendMsgToChar("А он ведь старше вас....\r\n", ch);
	else if (GetRealLevel(vict) >= kLvlImmortal && GET_GOD_FLAG(ch, EGf::kDemigod))
		SendMsgToChar("А он ведь старше вас....\r\n", ch);
	else {
		switch (subcmd) {
			case SCMD_REROLL: SendMsgToChar("Сбрасываю параметры...\r\n", ch);
				vict->set_start_stat(G_STR, 0);
				SendMsgToChar(vict, "&GВам сбросили парамерты персонажа, стоит перезайти в игру.\r\n&n");
/*				roll_real_abils(vict);
				log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
				imm_log("%s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
				sprintf(buf,
						"Новые параметры: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
						vict->GetInbornStr(), vict->GetInbornInt(), vict->GetInbornWis(),
						vict->GetInbornDex(), vict->GetInbornCon(), vict->GetInbornCha());
				SendMsgToChar(buf, ch);
*/
				break;
			case SCMD_NOTITLE:
				vict->IsFlagged(EPlrFlag::kNoTitle) ? vict->UnsetFlag(EPlrFlag::kNoTitle)
													: vict->SetFlag(EPlrFlag::kNoTitle);
				result = vict->IsFlagged(EPlrFlag::kNoTitle);
				sprintf(buf, "(GC) Notitle %s for %s by %s.", (result ? "ON" : "OFF"), GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("Notitle %s for %s by %s.", (result ? "ON" : "OFF"), GET_NAME(vict), GET_NAME(ch));
				strcat(buf, "\r\n");
				SendMsgToChar(buf, ch);
				break;
			case SCMD_SQUELCH: break;
			case SCMD_MUTE: if (*num) times = atol(num);
				punishments::set_punish(ch, vict, SCMD_MUTE, reason, times);
				break;
			case SCMD_DUMB: if (*num) times = atol(num);
				punishments::set_punish(ch, vict, SCMD_DUMB, reason, times);
				break;
			case SCMD_FREEZE: if (*num) times = atol(num);
				punishments::set_punish(ch, vict, SCMD_FREEZE, reason, times);
				break;
			case SCMD_HELL: if (*num) times = atol(num);
				punishments::set_punish(ch, vict, SCMD_HELL, reason, times);
				break;

			case SCMD_NAME: if (*num) times = atol(num);
				punishments::set_punish(ch, vict, SCMD_NAME, reason, times);
				break;

			case SCMD_REGISTER: punishments::set_punish(ch, vict, SCMD_REGISTER, reason, 0);
				break;

			case SCMD_UNREGISTER: punishments::set_punish(ch, vict, SCMD_UNREGISTER, reason, 0);
				break;

			case SCMD_UNAFFECT:
				if (!vict->affected.empty()) {
					vict->affected.clear();
					affect_total(vict);
					SendMsgToChar("Яркая вспышка осветила вас!\r\n"
								  "Вы почувствовали себя немного иначе.\r\n", vict);
					SendMsgToChar("Все афекты сняты.\r\n", ch);
				} else {
					SendMsgToChar("Аффектов не было изначально.\r\n", ch);
					return;
				}
				break;
			default: log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
				break;
		}
		vict->save_char();
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
