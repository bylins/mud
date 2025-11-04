#include "engine/entities/char_data.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/global_objects.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"

void DoSharpening(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int weight, add_hr, add_dr, prob, percent, min_mod, max_mod, i;
	bool oldstate;
	if (!ch->GetSkill(ESkill::kSharpening)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar("Что вы хотите заточить?\r\n", ch);
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		snprintf(buf, kMaxInputLength, "У вас нет \'%s\'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	};

	if (obj->get_type() != EObjType::kWeapon) {
		SendMsgToChar("Вы можете заточить только оружие.\r\n", ch);
		return;
	}

	if (static_cast<ESkill>(obj->get_spec_param()) == ESkill::kBows) {
		SendMsgToChar("Невозможно заточить этот тип оружия.\r\n", ch);
		return;
	}

	if (obj->has_flag(EObjFlag::kMagic)) {
		SendMsgToChar("Вы не можете заточить заколдованный предмет.\r\n", ch);
		return;
	}

	// Make sure no other (than hitroll & damroll) affections.
	for (i = 0; i < kMaxObjAffect; i++) {
		if ((obj->get_affected(i).location != EApply::kNone)
			&& (obj->get_affected(i).location != EApply::kHitroll)
			&& (obj->get_affected(i).location != EApply::kDamroll)) {
			SendMsgToChar("Этот предмет не может быть заточен.\r\n", ch);
			return;
		}
	}

	switch (obj->get_material()) {
		case EObjMaterial::kBronze:
		case EObjMaterial::kBulat:
		case EObjMaterial::kIron:
		case EObjMaterial::kSteel:
		case EObjMaterial::kForgedSteel:
		case EObjMaterial::kPreciousMetel:
		case EObjMaterial::kBone: act("Вы взялись точить $o3.",
									  false, ch, obj, nullptr, kToChar);
			act("$n взял$u точить $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			weight = -1;
			break;

		case EObjMaterial::kWood:
		case EObjMaterial::kHardWood: act("Вы взялись стругать $o3.",
										  false, ch, obj, nullptr, kToChar);
			act("$n взял$u стругать $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			weight = -1;
			break;

		case EObjMaterial::kSkin: act("Вы взялись проклепывать $o3.",
									  false, ch, obj, nullptr, kToChar);
			act("$n взял$u проклепывать $o3.",
				false, ch, obj, nullptr, kToRoom | kToArenaListen);
			weight = +1;
			break;

		default: sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n", OBJN(obj, ch, ECase::kNom));
			SendMsgToChar(buf, ch);
			return;
	}
	bool change_weight = true;
	//Заточить повторно можно, но это уменьшает таймер шмотки на 16%
	if (obj->has_flag(EObjFlag::kSharpen)) {
		int timer = obj->get_timer()
			- std::max(1000, obj->get_timer() / 6); // абуз, таймер меньше 6 вычитается 0 бесконечная прокачка умелки
		obj->set_timer(timer);
		change_weight = false;
	} else {
		obj->set_extra_flag(EObjFlag::kSharpen);
		obj->set_extra_flag(EObjFlag::kTransformed); // установили флажок трансформации кодом
	}

	percent = number(1, MUD::Skills()[ESkill::kSharpening].difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kSharpening, nullptr);
	TrainSkill(ch, ESkill::kSharpening, percent <= prob, nullptr);
	if (obj->get_timer() == 0) // не ждем рассыпания на тике
	{
		act("$o не выдержал$G издевательств и рассыпал$U в мелкую пыль...",
			false, ch, obj, nullptr, kToChar);
		ExtractObjFromWorld(obj);
		return;
	}
	//При 200% заточки шмотка будет точиться на 4-5 хитролов и 4-5 дамролов
	min_mod = ch->GetMorphSkill(ESkill::kSharpening) / 50;
	//С мортами все меньший уровень требуется для макс. заточки
	max_mod = std::clamp((GetRealLevel(ch) + 5 + GetRealRemort(ch)/4)/6, 1, 5);
	oldstate = stable_objs::IsTimerUnlimited(obj); // запомним какая шмотка была до заточки
	if (IS_IMMORTAL(ch)) {
		add_dr = add_hr = 10;
	} else {
		add_dr = add_hr = (max_mod <= min_mod) ? min_mod : number(min_mod, max_mod);
	}
	if (percent > prob || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		act("Но только загубили $S.", false, ch, obj, nullptr, kToChar);
		add_hr = -add_hr;
		add_dr = -add_dr;
	} else {
		act("И вроде бы неплохо в итоге получилось.", false, ch, obj, nullptr, kToChar);
	}

	obj->set_affected(0, EApply::kHitroll, add_hr);
	obj->set_affected(1, EApply::kDamroll, add_dr);

	// если шмотка перестала быть нерушимой ставим таймер из прототипа
	if (oldstate && !stable_objs::IsTimerUnlimited(obj)) {
		obj->set_timer(obj_proto.at(obj->get_rnum())->get_timer());
	}
	//Вес меняется только если шмотка еще не была заточена
	//Также вес НЕ меняется если он уже нулевой и должен снизиться
	const auto curent_weight = obj->get_weight();
	if (change_weight && !(curent_weight == 0 && weight < 0)) {
		obj->set_weight(curent_weight + weight);
		IS_CARRYING_W(ch) += weight;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
