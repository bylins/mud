/**
\file do_pray.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd/do_pray.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/handler.h"

void do_pray(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int metter = -1;
	ObjData *obj = nullptr;
	struct TimedSkill timed;

	if (ch->IsNpc()) {
		return;
	}

	if (!IS_IMMORTAL(ch)
		&& ((subcmd == SCMD_DONATE
			&& GET_RELIGION(ch) != kReligionPoly)
			|| (subcmd == SCMD_PRAY
				&& GET_RELIGION(ch) != kReligionMono))) {
		SendMsgToChar("Не кощунствуйте!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_DONATE && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kForPoly)) {
		SendMsgToChar("Найдите подходящее место для вашей жертвы.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_PRAY && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kForMono)) {
		SendMsgToChar("Это место не обладает необходимой святостью.\r\n", ch);
		return;
	}

	half_chop(argument, arg, buf);

	if (!*arg || (metter = search_block(arg, pray_whom, false)) < 0) {
		if (subcmd == SCMD_DONATE) {
			SendMsgToChar("Вы можете принести жертву :\r\n", ch);
			for (metter = 0; *(pray_metter[metter]) != '\n'; metter++) {
				if (*(pray_metter[metter]) == '-') {
					SendMsgToChar(pray_metter[metter], ch);
					SendMsgToChar("\r\n", ch);
				}
			}
			SendMsgToChar("Укажите, кому и что вы хотите жертвовать.\r\n", ch);
		} else if (subcmd == SCMD_PRAY) {
			SendMsgToChar("Вы можете вознести молитву :\r\n", ch);
			for (metter = 0; *(pray_metter[metter]) != '\n'; metter++)
				if (*(pray_metter[metter]) == '*') {
					SendMsgToChar(pray_metter[metter], ch);
					SendMsgToChar("\r\n", ch);
				}
			SendMsgToChar("Укажите, кому вы хотите вознести молитву.\r\n", ch);
		}
		return;
	}

	if (subcmd == SCMD_DONATE && *(pray_metter[metter]) != '-') {
		SendMsgToChar("Приносите жертвы только своим Богам.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_PRAY && *(pray_metter[metter]) != '*') {
		SendMsgToChar("Возносите молитвы только своим Богам.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_DONATE) {
		if (!*buf || !(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
			SendMsgToChar("Вы должны пожертвовать что-то стоящее.\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(obj) != EObjType::kFood
			&& GET_OBJ_TYPE(obj) != EObjType::kTreasure) {
			SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
			return;
		}
	} else if (subcmd == SCMD_PRAY) {
		if (ch->get_gold() < 10) {
			SendMsgToChar("У вас не хватит денег на свечку.\r\n", ch);
			return;
		}
	} else
		return;

	if (!IS_IMMORTAL(ch) && (IsTimedBySkill(ch, ESkill::kReligion)
		|| IsAffectedBySpell(ch, ESpell::kReligion))) {
		SendMsgToChar("Вы не можете так часто взывать к Богам.\r\n", ch);
		return;
	}

	timed.skill = ESkill::kReligion;
	timed.time = 12;
	ImposeTimedSkill(ch, &timed);

	for (const auto &i : pray_affect) {
		if (i.metter == metter) {
			Affect<EApply> af;
			af.type = ESpell::kReligion;
			af.duration = CalcDuration(ch, 12, 0, 0, 0, 0);
			af.modifier = i.modifier;
			af.location = i.location;
			af.bitvector = i.bitvector;
			af.battleflag = i.battleflag;
			ImposeAffect(ch, af, false, false, false, false);
		}
	}

	if (subcmd == SCMD_PRAY) {
		sprintf(buf, "$n затеплил$g свечку и вознес$q молитву %s.", pray_whom[metter]);
		act(buf, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		sprintf(buf, "Вы затеплили свечку и вознесли молитву %s.", pray_whom[metter]);
		act(buf, false, ch, nullptr, nullptr, kToChar);
		ch->remove_gold(10);
	} else if (subcmd == SCMD_DONATE && obj) {
		sprintf(buf, "$n принес$q $o3 в жертву %s.", pray_whom[metter]);
		act(buf, false, ch, obj, nullptr, kToRoom | kToArenaListen);
		sprintf(buf, "Вы принесли $o3 в жертву %s.", pray_whom[metter]);
		act(buf, false, ch, obj, nullptr, kToChar);
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
