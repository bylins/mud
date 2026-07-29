#include "engine/db/global_objects.h"
#include "utils/grammar/gender.h"
#include "skill_messages.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"

void DoRepair(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!GetSkill(ch, ESkill::kRepair)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kRepair, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
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
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kRepair, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}

	ObjData *obj;
	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	};

	if (obj->get_maximum_durability() <= obj->get_current_durability()) {
		act("$o в ремонте не нуждается.", false, ch, obj, nullptr, kToChar);
		return;
	}
	if ((obj->get_type() != EObjType::kWeapon) && !ObjSystem::is_armor_type(obj)) {
		SendMsgToChar("Вы можете отремонтировать только оружие или броню.\r\n", ch);
		return;
	}

	auto prob = number(1, MUD::Skills()[ESkill::kRepair].difficulty);
	auto percent = CalcCurrentSkill(ch, ESkill::kRepair, nullptr);
	TrainSkill(ch, ESkill::kRepair, prob <= percent, nullptr);
	if (prob > percent) {
		if (!percent) {
			percent = GetSkill(ch, ESkill::kRepair) / 10;
		}
		obj->set_current_durability(std::max(0, obj->get_current_durability() * percent / prob));
		if (obj->get_current_durability()) {
			act("Вы попытались починить $o3, но сломали $S еще больше.",
				false, ch, obj, nullptr, kToChar);
			act("$n попытал$u починить $o3, но сломал$g $S еще больше.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			auto decay = (obj->get_maximum_durability() - obj->get_current_durability()) / 10;
			decay = std::clamp(decay, 1, std::max(1, obj->get_maximum_durability() / 20));   // issue #3631: hi>=lo
			if (obj->get_maximum_durability() > decay) {
				obj->set_maximum_durability(obj->get_maximum_durability() - decay);
			} else {
				obj->set_maximum_durability(1);
			}
			// issue #3618: максимальная прочность назад не поднимается -- вещь навсегда хуже
			// прототипа, помечаем, чтобы правка прототипа в olc не вернула ее в исходный вид.
			obj->set_extra_flag(EObjFlag::kTransformed);
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
		auto modif = GetSkill(ch, ESkill::kRepair) / 7 + number(1, 5);
		timed.time = std::max(1, 25 - modif);
		ImposeTimedSkill(ch, &timed);
		obj->set_current_durability(std::min(obj->get_maximum_durability(), obj->get_current_durability() * percent / prob + 1));
		SendMsgToChar(ch, "Теперь %s выгляд%s лучше.\r\n",
					  obj->get_PName(grammar::ECase::kNom).c_str(), grammar::ObjPluralVerbEnding((obj)->get_sex()));
		act("$n умело починил$g $o3.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
