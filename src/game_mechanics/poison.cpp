// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "poison.h"

#include "entities/obj_data.h"
#include "entities/char_data.h"
#include "liquid.h"
#include "color.h"
#include "game_fight/fight.h"
#include "structs/global_objects.h"

extern int interpolate(int min_value, int pulse);

namespace {
// * Наложение ядов с пушек, аффект стакается до трех раз.
bool poison_affect_join(CharData *ch, Affect<EApply> &af) {
	bool found = false;

	for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end() && af.location; ++affect_i) {
		const auto affect = *affect_i;
		if ((affect->type == ESpell::kAconitumPoison
			|| affect->type == ESpell::kScopolaPoison
			|| affect->type == ESpell::kBelenaPoison
			|| affect->type == ESpell::kDaturaPoison)
			&& af.type != affect->type) {
			// если уже есть другой яд - борода
			return false;
		}

		if ((affect->type == af.type) && (affect->location == af.location)) {
			if (abs(affect->modifier / 3) < abs(af.modifier)) {
				af.modifier += affect->modifier;
			} else {
				af.modifier = affect->modifier;
			}

			ch->affect_remove(affect_i);
			affect_to_char(ch, *affect);
			found = true;
			break;
		}
	}

	if (!found) {
		affect_to_char(ch, af);
	}

	return true;
}

// * Отравление с пушек.
bool PoisonVictWithWeapon(CharData *ch, CharData *vict, ESpell spell_id) {
	if (GET_AF_BATTLE(ch, kEafPoisoned))
		return false;

	if (spell_id == ESpell::kAconitumPoison) {
		// урон 5 + левел/2, от 5 до 20 за стак
		Affect<EApply> af;
		af.type = ESpell::kAconitumPoison;
		af.location = EApply::kAconitumPoison;
		af.duration = 7;
		af.modifier = GetRealLevel(ch) / 2 + 5;
		af.bitvector = to_underlying(EAffect::kPoisoned);
		af.battleflag = kAfSameTime;
		if (poison_affect_join(vict, af)) {
			vict->poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	} else if (spell_id == ESpell::kScopolaPoison) {
		// по +5% дамаги по целе за стак (кроме стаба)
		Affect<EApply> af;
		af.type = ESpell::kScopolaPoison;
		af.location = EApply::kScopolaPoison;
		af.duration = 7;
		af.modifier = 5;
		af.bitvector = to_underlying(EAffect::kPoisoned) | to_underlying(EAffect::kScopolaPoison);
		af.battleflag = kAfSameTime;
		if (poison_affect_join(vict, af)) {
			vict->poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	} else if (spell_id == ESpell::kBelenaPoison) {
		// не переключается (моб)
		// -хитролы/хп-рег/дамаг-физ.атак/скилы

		Affect<EApply> af[3];
		// скилл * 0.05 на чаров и + 5 на мобов. 4-10% и 9-15% (80-200 скила)
		int percent = 0;
		if (ch->GetSkill(ESkill::kPoisoning) >= 80) {
			percent = (ch->GetSkill(ESkill::kPoisoning) * 5 / 100) + (vict->IsNpc() ? 5 : 0);
		}
		// -дамаг физ.атак и скиллы
		af[0].location = EApply::kBelenaPoison;
		af[0].modifier = percent;

		// скилл * 0.05 + 5 на чаров и + 10 на мобов. 5.5-15% и 10.5-20% (10-200 скила)
		percent = (ch->GetSkill(ESkill::kPoisoning) * 5 / 100) + (vict->IsNpc() ? 10 : 5);
		// -хитролы
		int remove_hit = GET_REAL_HR(vict) * (percent / 100);
		af[1].location = EApply::kHitroll;
		af[1].modifier = -remove_hit;
		// -хп-рег
		int remove_hp = GET_HITREG(vict) * (percent / 100);
		af[2].location = EApply::kHpRegen;
		af[2].modifier = -remove_hp;

		bool was_poisoned = true;
		for (auto & i : af) {
			i.type = ESpell::kBelenaPoison;
			i.duration = 7;
			i.bitvector = to_underlying(EAffect::kPoisoned)
				| to_underlying(EAffect::kBelenaPoison)
				| to_underlying(EAffect::kSkillReduce)
				| to_underlying(EAffect::kNoBattleSwitch);
			i.battleflag = kAfSameTime;

			if (!poison_affect_join(vict, i)) {
				was_poisoned = false;
			}
		}

		if (was_poisoned) {
			vict->poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	} else if (spell_id == ESpell::kDaturaPoison) {
		// не переключается (моб)
		// -каст/мем-рег/дамаг-заклов/скилы
		// AFF_DATURA_POISON - флаг на снижение дамага с заклов

		Affect<EApply> af[3];
		// скилл * 0.05 на чаров и + 5 на мобов. 4-10% и 9-15% (80-200 скила)
		int percent = 0;
		if (ch->GetSkill(ESkill::kPoisoning) >= 80)
			percent = (ch->GetSkill(ESkill::kPoisoning) * 5 / 100) + (vict->IsNpc() ? 5 : 0);
		// -дамаг заклов и скиллы
		af[0].location = EApply::kDaturaPoison;
		af[0].modifier = percent;

		// скилл * 0.05 + 5 на чаров и + 10 на мобов. 5.5-15% и 10.5-20% (10-200 скила)
		percent = (ch->GetSkill(ESkill::kPoisoning) * 5 / 100) + (vict->IsNpc() ? 10 : 5);
		// -каст
		int remove_cast = GET_CAST_SUCCESS(vict) * (percent / 100);
		af[1].location = EApply::kCastSuccess;
		af[1].modifier = -remove_cast;
		// -мем
		int remove_mem = GET_MANAREG(vict) * (percent / 100);
		af[2].location = EApply::kManaRegen;
		af[2].modifier = -remove_mem;

		bool was_poisoned = true;
		for (auto & i : af) {
			i.type = ESpell::kDaturaPoison;
			i.duration = 7;
			i.bitvector = to_underlying(EAffect::kPoisoned)
				| to_underlying(EAffect::kDaturaPoison)
				| to_underlying(EAffect::kSkillReduce)
				| to_underlying(EAffect::kNoBattleSwitch);
			i.battleflag = kAfSameTime;

			if (!poison_affect_join(vict, i)) {
				was_poisoned = false;
			}
		}

		if (was_poisoned) {
			vict->poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	}
	return false;
}

// * Крит при отравлении с пушек.
void ProcessCritWeaponPoison(CharData *ch, CharData *vict, ESpell/* spell_num*/) {
	Affect<EApply> af;
	int percent = number(1, MUD::Skill(ESkill::kPoisoning).difficulty * 3);
	int prob = CalcCurrentSkill(ch, ESkill::kPoisoning, vict);
	if (prob >= percent) {
		switch (number(1, 5)) {
			case 1:
				// аналог баша с лагом
				if (GET_POS(vict) >= EPosition::kFight) {
					if (vict->IsOnHorse()) {
						SendMsgToChar(ch, "%sОт действия вашего яда у %s закружилась голова!%s\r\n",
									  CCGRN(ch, C_NRM), PERS(vict, ch, 1), CCNRM(ch, C_NRM));
						SendMsgToChar(vict, "Вы почувствовали сильное головокружение и не смогли усидеть на %s!\r\n",
									  GET_PAD(vict->get_horse(), 5));
						act("$n0 зашатал$u и не смог$q усидеть на $N5.",
							true, vict, nullptr, vict->get_horse(), kToNotVict);
						vict->DropFromHorse();
					} else {
						SendMsgToChar(ch, "%sОт действия вашего яда у %s подкосились ноги!%s\r\n",
									  CCGRN(ch, C_NRM), PERS(vict, ch, 1), CCNRM(ch, C_NRM));
						SendMsgToChar(vict, "Вы почувствовали сильное головокружение и не смогли устоять на ногах!\r\n");
						act("$N0 зашатал$U и не смог$Q устоять на ногах.",
							true, ch, nullptr, vict, kToNotVict);
						GET_POS(vict) = EPosition::kSit;
						SetWaitState(vict, 3 * kBattleRound);
					}
					break;
				}
				break;
			case 2: {
				// минус статы (1..5)
				af.type = ESpell::kPoison;
				af.duration = 30;
				af.modifier = -GetRealLevel(ch) / 6;
				af.bitvector = to_underlying(EAffect::kPoisoned);
				af.battleflag = kAfSameTime;

				for (int i = EApply::kStr; i <= EApply::kCha; i++) {
					af.location = static_cast<EApply>(i);
					ImposeAffect(vict, af, false, false, false, false);
				}

				SendMsgToChar(ch, "%sОт действия вашего яда %s побледнел%s!%s\r\n",
							  CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				SendMsgToChar(vict, "Вы почувствовали слабость во всем теле!\r\n");
				act("$N0 побледнел$G на ваших глазах.", true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 2
			case 3: {
				// минус реакция (1..5)
				af.type = ESpell::kPoison;
				af.duration = 30;
				af.location = EApply::kSavingReflex;
				af.modifier = GetRealLevel(ch) / 6; //Polud с плюсом, поскольку здесь чем больше - тем хуже
				af.bitvector = to_underlying(EAffect::kPoisoned);
				af.battleflag = kAfSameTime;
				ImposeAffect(vict, af, false, false, false, false);
				SendMsgToChar(ch, "%sОт действия вашего яда %s стал%s хуже реагировать на движения противников!%s\r\n",
							  CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				SendMsgToChar(vict, "Вам стало труднее реагировать на движения противников!\r\n");
				act("$N0 стал$G хуже реагировать на ваши движения!", true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 3
			case 4: {
				// минус инициатива (1..5)
				af.type = ESpell::kPoison;
				af.duration = 30;
				af.location = EApply::kInitiative;
				af.modifier = -GetRealLevel(ch) / 6;
				af.bitvector = to_underlying(EAffect::kPoisoned);
				af.battleflag = kAfSameTime;
				ImposeAffect(vict, af, false, false, false, false);
				SendMsgToChar(ch, "%sОт действия вашего яда %s стал%s заметно медленнее двигаться!%s\r\n",
							  CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				SendMsgToChar(vict, "Вы стали заметно медленнее двигаться!\r\n");
				act("$N0 стал$G заметно медленнее двигаться!",
					true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 4
			case 5: {
				// минус живучесть (1..5)
				af.type = ESpell::kPoison;
				af.duration = 30;
				af.location = EApply::kResistVitality;
				af.modifier = -GetRealLevel(ch) / 6;
				af.bitvector = to_underlying(EAffect::kPoisoned);
				af.battleflag = kAfSameTime;
				ImposeAffect(vict, af, false, false, false, false);
				SendMsgToChar(ch, "%sОт действия вашего яда %s стал%s хуже переносить повреждения!%s\r\n",
							  CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				SendMsgToChar(vict, "Вы стали хуже переносить повреждения!\r\n");
				act("$N0 стал$G хуже переносить повреждения!", true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 5
		} // switch

	}
}

} // namespace

// * Отравление с заклинания 'яд'.
void poison_victim(CharData *ch, CharData *vict, int modifier) {
	Affect<EApply> af[4];

	// change strength
	af[0].type = ESpell::kPoison;
	af[0].location = EApply::kStr;
	af[0].duration = CalcDuration(vict, 0, std::max(2, GetRealLevel(ch) - GetRealLevel(vict)), 2, 0, 1);
	af[0].modifier = -std::min(2, (modifier + 29) / 40);
	af[0].bitvector = to_underlying(EAffect::kPoisoned);
	af[0].battleflag = kAfSameTime;
	// change damroll
	af[1].type = ESpell::kPoison;
	af[1].location = EApply::kDamroll;
	af[1].duration = af[0].duration;
	af[1].modifier = -std::min(2, (modifier + 29) / 30);
	af[1].bitvector = to_underlying(EAffect::kPoisoned);
	af[1].battleflag = kAfSameTime;
	// change hitroll
	af[2].type = ESpell::kPoison;
	af[2].location = EApply::kHitroll;
	af[2].duration = af[0].duration;
	af[2].modifier = -std::min(2, (modifier + 19) / 20);
	af[2].bitvector = to_underlying(EAffect::kPoisoned);
	af[2].battleflag = kAfSameTime;
	// change poison level
	af[3].type = ESpell::kPoison;
	af[3].location = EApply::kPoison;
	af[3].duration = af[0].duration;
	af[3].modifier = GetRealLevel(ch);
	af[3].bitvector = to_underlying(EAffect::kPoisoned);
	af[3].battleflag = kAfSameTime;

	for (auto & i : af) {
		ImposeAffect(vict, i, false, false, false, false);
	}
	vict->poisoner = GET_ID(ch);

	snprintf(buf, sizeof(buf), "%sВы отравили $N3.%s", CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));
	act(buf, false, ch, nullptr, vict, kToChar);
	snprintf(buf, sizeof(buf), "%s$n отравил$g вас.%s", CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	act(buf, false, ch, nullptr, vict, kToVict);
}

// * Попытка травануть с пушки при ударе.
void TryPoisonWithWeapom(CharData *ch, CharData *vict, ESpell spell_id) {
	if (spell_id < ESpell::kFirst) {
		return;
	}

	if (number(1, 200) <= 25 ||
		(!GET_AF_BATTLE(vict, kEafFirstPoison) && !AFF_FLAGGED(vict, EAffect::kPoisoned))) {
		ImproveSkill(ch, ESkill::kPoisoning, true, vict);
		if (PoisonVictWithWeapon(ch, vict, spell_id)) {
			if (spell_id == ESpell::kAconitumPoison) {
				SendMsgToChar(ch, "Кровоточащие язвы покрыли тело %s.\r\n",
							  PERS(vict, ch, 1));
			} else if (spell_id == ESpell::kScopolaPoison) {
				strcpy(buf1, PERS(vict, ch, 0));
				CAP(buf1);
				SendMsgToChar(ch, "%s скрючил%s от нестерпимой боли.\r\n",
							  buf1, GET_CH_VIS_SUF_2(vict, ch));
				SET_AF_BATTLE(vict, kEafFirstPoison);
			} else if (spell_id == ESpell::kBelenaPoison) {
				strcpy(buf1, PERS(vict, ch, 3));
				CAP(buf1);
				SendMsgToChar(ch, "%s перестали слушаться руки.\r\n", buf1);
				SET_AF_BATTLE(vict, kEafFirstPoison);
			} else if (spell_id == ESpell::kDaturaPoison) {
				strcpy(buf1, PERS(vict, ch, 2));
				CAP(buf1);
				SendMsgToChar(ch, "%s стало труднее плести заклинания.\r\n", buf1);
				SET_AF_BATTLE(vict, kEafFirstPoison);
			} else {
				SendMsgToChar(ch, "Вы отравили %s.\r\n", PERS(ch, vict, 3));
			}
			SendMsgToChar(vict, "%s%s отравил%s вас.%s\r\n",
						  CCIRED(ch, C_NRM), PERS(ch, vict, 0),
						  GET_CH_VIS_SUF_1(ch, vict), CCNRM(ch, C_NRM));
			ProcessCritWeaponPoison(ch, vict, spell_id);
		}
	}
}

// * Проверка типа жидкости на яд для нанесения на пушку.
bool poison_in_vessel(int liquid_num) {
	if (liquid_num == LIQ_POISON_ACONITUM
		|| liquid_num == LIQ_POISON_SCOPOLIA
		|| liquid_num == LIQ_POISON_BELENA
		|| liquid_num == LIQ_POISON_DATURA) {
		return true;
	}
	return false;
}

// * Сет яда на пушку в зависимости от типа жидкости.
void set_weap_poison(ObjData *weapon, int liquid_num) {
	const int poison_timer = 30;
	if (liquid_num == LIQ_POISON_ACONITUM)
		weapon->add_timed_spell(ESpell::kAconitumPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_SCOPOLIA)
		weapon->add_timed_spell(ESpell::kScopolaPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_BELENA)
		weapon->add_timed_spell(ESpell::kBelenaPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_DATURA)
		weapon->add_timed_spell(ESpell::kDaturaPoison, poison_timer);
	else
		log("SYSERROR: liquid_num == %d (%s %s %d)", liquid_num, __FILE__, __func__, __LINE__);
}

// * Вывод имени яда по номеру его заклинания (для осмотра пушек).
std::string GetPoisonName(ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kAconitumPoison: return drinknames[LIQ_POISON_ACONITUM];
		case ESpell::kScopolaPoison: return drinknames[LIQ_POISON_SCOPOLIA];
		case ESpell::kBelenaPoison: return drinknames[LIQ_POISON_BELENA];
		case ESpell::kDaturaPoison: return drinknames[LIQ_POISON_DATURA];
		default: return "";
	}
}

// * Проверка, является ли заклинание ядом.
bool IsSpellPoison(ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kAconitumPoison:
		case ESpell::kScopolaPoison:
		case ESpell::kBelenaPoison:
		case ESpell::kDaturaPoison: return true;
		default: return false;
	}
	//return false;
}

/**
* Пока с AF_SAME_TIME получилось следующее (в бою/вне боя):
* APPLY_POISON - у плеера раз в 2 секунды везде, у моба раз в минуту везде.
* Остальные аффекты - у плеера раз в 2 секунды везде, у моба в бою раз в 2 секунды, вне боя - раз в минуту.
*/
int ProcessPoisonDmg(CharData *ch, const Affect<EApply>::shared_ptr &af) {
	int result = 0;
	if (af->location == EApply::kPoison) {
		int poison_dmg = GET_POISON(ch) * (ch->IsNpc() ? 4 : 5);
		// мобов яд ядит на тике, а чаров каждый батл тик соответсвенно если это не моб надо делить на 30 или тип того
		if (!ch->IsNpc())
			poison_dmg = poison_dmg / 30;
		//poison_dmg = interpolate(poison_dmg, 2); // И как оно должно работать чото нифига не понял, понял только что оно не работает
		Damage dmg(SpellDmg(ESpell::kPoison), poison_dmg, fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		result = dmg.Process(ch, ch);
	} else if (af->location == EApply::kAconitumPoison) {
		Damage dmg(SpellDmg(ESpell::kPoison), GET_POISON(ch), fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		result = dmg.Process(ch, ch);
	}
	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
