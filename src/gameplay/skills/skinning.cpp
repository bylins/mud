#include "gameplay/mechanics/dead_load.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/meat_maker.h"

extern std::array<int, kMaxMobLevel / 11 + 1> animals_levels;

const int effects_l[5][40][2]{
	{{0, 0}},
	{{0, 26}, // количество строк
	 {EApply::kAbsorbe, 5},
	 {EApply::kFirstCircle, 3},
	 {EApply::kSecondCircle, 3},
	 {EApply::kThirdCircle, 2},
	 {EApply::kFourthCircle, 2},
	 {EApply::kFifthCircle, 1},
	 {EApply::kSixthCircle, 1},
	 {EApply::kCastSuccess, 3},
	 {EApply::kHp, 20},
	 {EApply::kHpRegen, 35},
	 {EApply::kInitiative, 5},
	 {EApply::kManaRegen, 15},
	 {EApply::kMorale, 5},
	 {EApply::kMove, 35},
	 {EApply::kResistAir, 15},
	 {EApply::kResistEarth, 15},
	 {EApply::kResistFire, 15},
	 {EApply::kResistImmunity, 5},
	 {EApply::kResistMind, 5},
	 {EApply::kResistVitality, 5},
	 {EApply::kResistWater, 15},
	 {EApply::kSavingCritical, -5},
	 {EApply::kSavingReflex, -5},
	 {EApply::kSavingStability, -5},
	 {EApply::kSavingWill, -5},
	 {EApply::kSize, 10},
	 {EApply::kResistDark, 15},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0}},
	{{0, 37},
	 {EApply::kAbsorbe, 10},
	 {EApply::kFirstCircle, 3},
	 {EApply::kSecondCircle, 3},
	 {EApply::kThirdCircle, 3},
	 {EApply::kFourthCircle, 3},
	 {EApply::kFifthCircle, 2},
	 {EApply::kSixthCircle, 2},
	 {EApply::kSeventhCircle, 2},
	 {EApply::kEighthCircle, 1},
	 {EApply::kNinthCircle, 1},
	 {EApply::kCastSuccess, 5},
	 {EApply::kCha, 1},
	 {EApply::kCon, 1},
	 {EApply::kDamroll, 2},
	 {EApply::kDex, 1},
	 {EApply::kHp, 30},
	 {EApply::kHpRegen, 55},
	 {EApply::kHitroll, 2},
	 {EApply::kInitiative, 10},
	 {EApply::kInt, 1},
	 {EApply::kManaRegen, 30},
	 {EApply::kMorale, 7},
	 {EApply::kMove, 55},
	 {EApply::kResistAir, 25},
	 {EApply::kResistEarth, 25},
	 {EApply::kResistFire, 25},
	 {EApply::kResistImmunity, 10},
	 {EApply::kResistMind, 10},
	 {EApply::kResistVitality, 10},
	 {EApply::kResistWater, 25},
	 {EApply::kSavingCritical, -10},
	 {EApply::kSavingReflex, -10},
	 {EApply::kSavingStability, -10},
	 {EApply::kSavingWill, -10},
	 {EApply::kSize, 15},
	 {EApply::kStr, 1},
	 {EApply::kWis, 1},
	 {0, 0},
	 {0, 0}},
	{{0, 23},
	 {EApply::kAbsorbe, 15},
	 {EApply::kEighthCircle, 2},
	 {EApply::kNinthCircle, 2},
	 {EApply::kCastSuccess, 7},
	 {EApply::kCha, 2},
	 {EApply::kCon, 2},
	 {EApply::kDamroll, 3},
	 {EApply::kDex, 2},
	 {EApply::kHp, 45},
	 {EApply::kHitroll, 3},
	 {EApply::kInitiative, 15},
	 {EApply::kInt, 2},
	 {EApply::kMorale, 9},
	 {EApply::kResistImmunity, 15},
	 {EApply::kResistMind, 15},
	 {EApply::kResistVitality, 15},
	 {EApply::kSavingCritical, -15},
	 {EApply::kSavingReflex, -15},
	 {EApply::kSavingStability, -15},
	 {EApply::kSavingWill, -15},
	 {EApply::kSize, 20},
	 {EApply::kStr, 2},
	 {EApply::kWis, 2},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0}},
	{{0, 21},
	 {EApply::kAbsorbe, 20},
	 {EApply::kCastSuccess, 10},
	 {EApply::kCha, 2},
	 {EApply::kCon, 2},
	 {EApply::kDamroll, 4},
	 {EApply::kDex, 2},
	 {EApply::kHp, 60},
	 {EApply::kHitroll, 4},
	 {EApply::kInitiative, 20},
	 {EApply::kInt, 2},
	 {EApply::kMorale, 12},
	 {EApply::kMagicResist, 3},
	 {EApply::kResistImmunity, 20},
	 {EApply::kResistMind, 20},
	 {EApply::kResistVitality, 20},
	 {EApply::kSavingCritical, -20},
	 {EApply::kSavingReflex, -20},
	 {EApply::kSavingStability, -20},
	 {EApply::kSavingWill, -20},
	 {EApply::kStr, 2},
	 {EApply::kWis, 2},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0},
	 {0, 0}}

};

bool skill_to_skin(CharData *mob, CharData *ch) {
	int num;
	switch (GetRealLevel(mob) / 11) {
		case 0: num = 15 * animals_levels[0] / 2201; // приводим пропорцией к количеству зверья на 15.11.2015 в мире
			if (number(1, 100) <= num)
				return true;
			break;
		case 1:
			if (ch->GetSkill(ESkill::kSkinning) >= 40) {
				num = 20 * animals_levels[1] / 701;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				SendMsgToChar(buf, ch);
				return false;
			}

			break;
		case 2:
			if (ch->GetSkill(ESkill::kSkinning) >= 80) {
				num = 10 * animals_levels[2] / 594;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				SendMsgToChar(buf, ch);
				return false;
			}
			break;

		case 3:
			if (ch->GetSkill(ESkill::kSkinning) >= 120) {
				num = 8 * animals_levels[3] / 209;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				SendMsgToChar(buf, ch);
				return false;
			}
			break;

		case 4:
			if (ch->GetSkill(ESkill::kSkinning) >= 160) {
				num = 25 * animals_levels[4] / 20;
				if (number(1, 100) <= num)
					return true;
			} else {
				sprintf(buf, "Ваше умение слишком низкое, чтобы содрать шкуру %s.\r\n", GET_PAD(mob, 1));
				SendMsgToChar(buf, ch);
				return false;
			}
			break;
			//TODO: Добавить для мобов выше 54 уровня
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		default: return false;
	}
	return false;
}

ObjData *create_skin(CharData *mob, CharData *ch) {
	int vnum, i, k = 0, num, effect;
	bool concidence;
	const int vnum_skin_prototype = 1660;

	vnum = vnum_skin_prototype + std::min(GetRealLevel(mob) / 5, 9);
	const auto skin = world_objects.create_from_prototype_by_vnum(vnum);
	if (!skin) {
		mudlog("Неверно задан номер прототипа для освежевания в act.item.cpp::create_skin!",
			   NRM, kLvlGreatGod, ERRLOG, true);
		return nullptr;
	}

	skin->set_val(3, int(GetRealLevel(mob) / 11)); // установим уровень шкуры, топовая 44+
	dead_load::ResolveTagsInObjName(skin.get(), mob); // переносим падежи
	for (i = 1; i <= GET_OBJ_VAL(skin, 3); i++) // топовая шкура до 4х афектов
	{
		if ((k == 1) && (number(1, 100) >= 35)) {
			continue;
		}
		if ((k == 2) && (number(1, 100) >= 20)) {
			continue;
		}
		if ((k == 3) && (number(1, 100) >= 10)) {
			continue;
		}

		{
			concidence = true;
			while (concidence) {
				num = number(1, effects_l[GET_OBJ_VAL(skin, 3)][0][1]);
				concidence = false;
				for (int n = 0; n <= k && i > 1; n++) {
					if (effects_l[GET_OBJ_VAL(skin, 3)][num][0] == (skin)->get_affected(n).location) {
						concidence = true;
					}
				}
			}
			auto location = effects_l[GET_OBJ_VAL(skin, 3)][num][0];
			effect = effects_l[GET_OBJ_VAL(skin, 3)][num][1];
			if (number(0, 1000)
				<= (250 / (GET_OBJ_VAL(skin, 3) + 1))) //  чем круче шкура тем реже  отрицательный аффект
			{
				effect *= -1;
			}
			skin->set_affected(k, static_cast<EApply>(location), effect);
			k++;
		}
	}

	skin->set_cost(GetRealLevel(mob) * number(2, std::max(3, 3 * k)));
	skin->set_val(2, 95); //оставил 5% фейла переноса аффектов на создаваемую шмотку

	act("$n умело срезал$g $o3.", false, ch, skin.get(), nullptr, kToRoom | kToArenaListen);
	act("Вы умело срезали $o3.", false, ch, skin.get(), nullptr, kToChar);

	//ставим флажок "не зависит от прототипа"
	skin->set_extra_flag(EObjFlag::kTransformed);
	return skin.get();
}

void DoSkinning(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kSkinning)) {
		SendMsgToChar("Вы не умеете этого.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Что вы хотите освежевать?\r\n", ch);
		return;
	}

//	auto obj = get_obj_in_list_vis(ch, arg, ch->carrying);
//	if (!obj) {
	auto obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents);
	if (!obj) {
		snprintf(buf, kMaxInputLength, "Вы не видите здесь '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}
//	}

	const auto mobn = GET_OBJ_VAL(obj, 2);
	if (!IS_CORPSE(obj) || mobn < 0) {
		act("Вы не сможете освежевать $o3.", false, ch, obj, nullptr, kToChar);
		return;
	}

	const auto mob = (mob_proto + GetMobRnum(mobn));
	if (!IS_IMMORTAL(ch)
		&& GET_RACE(mob) != ENpcRace::kAnimal
		&& GET_RACE(mob) != ENpcRace::kReptile
		&& GET_RACE(mob) != ENpcRace::kFish
		&& GET_RACE(mob) != ENpcRace::kBird
		&& GET_RACE(mob) != ENpcRace::kBeastman) {
		SendMsgToChar("Этот труп невозможно освежевать.\r\n", ch);
		return;
	}

	if (GET_WEIGHT(mob) < 11) {
		SendMsgToChar("Этот труп слишком маленький, ничего не получится.\r\n", ch);
		return;
	}

	const auto prob = number(1, MUD::Skills()[ESkill::kSkinning].difficulty);
	const auto percent = CalcCurrentSkill(ch, ESkill::kSkinning, mob)
		+ number(1, GetRealDex(ch)) + number(1, GetRealStr(ch));
	TrainSkill(ch, ESkill::kSkinning, percent <= prob, mob);

	ObjData::shared_ptr tobj;
	if (ch->GetSkill(ESkill::kSkinning) > 150 && number(1, 200) == 1) // артефакт
	{
		tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get_artefact_key());
	} else {
		tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.random_key());
	}

	if (prob > percent || !tobj) {
		act("Вы не сумели освежевать $o3.", false, ch, obj, nullptr, kToChar);
		act("$n попытал$u освежевать $o3, но неудачно.",
			false, ch, obj, nullptr, kToRoom | kToArenaListen);
	} else {
		act("$n умело освежевал$g $o3.",
			false, ch, obj, nullptr, kToRoom | kToArenaListen);
		act("Вы умело освежевали $o3.",
			false, ch, obj, nullptr, kToChar);

		dead_load::LoadObjFromDeadLoad(obj, mob, ch, dead_load::kSkin);

		std::vector<ObjData *> entrails;
		entrails.push_back(tobj.get());

		if (GET_RACE(mob) == ENpcRace::kAnimal) // шкуры только с животных
		{
			if (IS_IMMORTAL(ch) || skill_to_skin(mob, ch)) {
				entrails.push_back(create_skin(mob, ch));
			}
		}

		entrails.push_back(try_make_ingr(mob, 1000 - ch->GetSkill(ESkill::kSkinning) * 2));  // ингры со всех

		for (const auto &it : entrails) {
			if (it) {
//				if (obj->get_carried_by() == ch) {
					can_carry_obj(ch, it);
//				} else {
//					PlaceObjToRoom(it, ch->in_room);
//				}
			}
		}
	}

	ExtractObjFromWorld(obj);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
