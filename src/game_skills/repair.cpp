#include "structs/global_objects.h"
#include "handler.h"

void DoRepair(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_skill(ESkill::kRepair)) {
		SendMsgToChar("Вы не умеете этого.\r\n", ch);
		return;
	}
	if (IsTimedBySkill(ch, ESkill::kRepair)) {
		SendMsgToChar("У вас недостаточно сил для ремонта.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (ch->GetEnemy()) {
		SendMsgToChar("Вы не можете сделать это в бою!\r\n", ch);
		return;
	}

	if (!*arg) {
		SendMsgToChar("Что вы хотите ремонтировать?\r\n", ch);
		return;
	}

	ObjData *obj;
	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	};

	if (GET_OBJ_MAX(obj) <= GET_OBJ_CUR(obj)) {
		act("$o в ремонте не нуждается.", false, ch, obj, nullptr, kToChar);
		return;
	}
	if ((GET_OBJ_TYPE(obj) != EObjType::kWeapon) && !ObjSystem::is_armor_type(obj)) {
		SendMsgToChar("Вы можете отремонтировать только оружие или броню.\r\n", ch);
		return;
	}

	auto prob = number(1, MUD::Skills()[ESkill::kRepair].difficulty);
	auto percent = CalcCurrentSkill(ch, ESkill::kRepair, nullptr);
	TrainSkill(ch, ESkill::kRepair, prob <= percent, nullptr);
	if (prob > percent) {
		if (!percent) {
			percent = ch->get_skill(ESkill::kRepair) / 10;
		}
		obj->set_current_durability(std::max(0, obj->get_current_durability() * percent / prob));
		if (obj->get_current_durability()) {
			act("Вы попытались починить $o3, но сломали $S еще больше.",
				false, ch, obj, nullptr, kToChar);
			act("$n попытал$u починить $o3, но сломал$g $S еще больше.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			auto decay = (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 10;
			decay = std::clamp(decay, 1, GET_OBJ_MAX(obj)/20);
			if (GET_OBJ_MAX(obj) > decay) {
				obj->set_maximum_durability(obj->get_maximum_durability() - decay);
			} else {
				obj->set_maximum_durability(1);
			}
		} else {
			act("Вы окончательно доломали $o3.",
				false, ch, obj, nullptr, kToChar);
			act("$n окончательно доломал$g $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			ExtractObjFromWorld(obj);
		}
	} else {
		TimedSkill timed;
		timed.skill = ESkill::kRepair;
		auto modif = ch->get_skill(ESkill::kRepair) / 7 + number(1, 5);
		timed.time = std::max(1, 25 - modif);
		ImposeTimedSkill(ch, &timed);
		obj->set_current_durability(std::min(GET_OBJ_MAX(obj), GET_OBJ_CUR(obj) * percent / prob + 1));
		SendMsgToChar(ch, "Теперь %s выгляд%s лучше.\r\n",
					  obj->get_PName(0).c_str(), GET_OBJ_POLY_1(ch, obj));
		act("$n умело починил$g $o3.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
