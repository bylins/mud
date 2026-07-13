#include "do_godtest.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/target_resolver.h"          // generic_find / EFind
#include "engine/core/utils_char_obj.inl"
#include "gameplay/affects/obj_affects.h"        // obj_affects::Impose / EObjAffect::kDartTrap

// It is a test command for different testings.
// issue.obj-affects: `godtest <obj-keyword>` arms the named carried/equipped/room item with the
// kDartTrap obj affect (2 charges) -- a stand-in until a real trap-placing mechanic exists. Picking or
// opening the item then fires the trap (blocks the action, casts kColdWind + kPoison on the actor).
void do_godtest(CharData *ch, char *argument, int /* cmd */, int /* subcmd */) {
	char arg[kMaxInputLength];
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Формат: godtest <предмет> "
			"-- установить ловушку "
			"на предмет.\r\n", ch);
		return;
	}
	ObjData *obj = nullptr;
	CharData *tmp = nullptr;
	generic_find(arg, EFind::kObjInventory | EFind::kObjEquip | EFind::kObjRoom, ch, &tmp, &obj);
	if (!obj) {
		SendMsgToChar(ch, "У вас нет '%s'.\r\n", arg);
		return;
	}
	obj_affects::Impose(obj, obj_affects::EObjAffect::kDartTrap, -1, 0, ch->get_uid(), 0.0f, 2);
	SendMsgToChar(ch, "На %s установлена "
		"ловушка с дротиками "
		"(2 заряда).\r\n", obj->get_PName(grammar::ECase::kAcc).c_str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
