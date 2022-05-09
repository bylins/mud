#include "entities/char_data.h"
#include "handler.h"
#include "meat_maker.h"
#include "utils/utils_char_obj.inl"
#include "world_objects.h"

void do_fry(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	ObjData *meet;
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Что вы собрались поджарить?\r\n", ch);
		return;
	}
	if (ch->GetEnemy()) {
		SendMsgToChar("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}
	if (!(meet = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxStringLength, "У вас нет '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}
	if (!world[ch->in_room]->fires) {
		SendMsgToChar(ch, "На чем вы собрались жарить, огня то нет!\r\n");
		return;
	}
	if (world[ch->in_room]->fires > 2) {
		SendMsgToChar(ch, "Костер слишком силен, сгорит!\r\n");
		return;
	}

	const auto meet_vnum = GET_OBJ_VNUM(meet);
	if (!meat_mapping.has(meet_vnum)) // не нашлось в массиве
	{
		SendMsgToChar(ch, "%s не подходит для жарки.\r\n", GET_OBJ_PNAME(meet, 0).c_str());
		return;
	}

	act("Вы нанизали на веточку и поджарили $o3.", false, ch, meet, nullptr, kToChar);
	act("$n нанизал$g на веточку и поджарил$g $o3.",
		true, ch, meet, nullptr, kToRoom | kToArenaListen);
	const auto tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get(meet_vnum));
	if (tobj) {
		can_carry_obj(ch, tobj.get());
		ExtractObjFromWorld(meet);
		SetWaitState(ch, 1 * kPulseViolence);
	} else {
		mudlog("Невозможно загрузить жаренное мясо в do_fry!", NRM, kLvlGreatGod, ERRLOG, true);
	}
}

