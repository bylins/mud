// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "game_skills/poison.h"

#include "entities/obj_data.h"
#include "entities/char_data.h"
#include "liquid.h"
#include "color.h"
#include "fightsystem/fight.h"
#include "structs/global_objects.h"

extern int interpolate(int min_value, int pulse);

namespace {
// * Наложение ядов с пушек, аффект стакается до трех раз.
bool poison_affect_join(CharData *ch, Affect<EApplyLocation> &af) {
	bool found = false;

	for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end() && af.location; ++affect_i) {
		const auto affect = *affect_i;
		if ((affect->type == kSpellAconitumPoison
			|| affect->type == kSpellScopolaPoison
			|| affect->type == kSpellBelenaPoison
			|| affect->type == kSpellDaturaPoison)
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
bool weap_poison_vict(CharData *ch, CharData *vict, int spell_num) {
	if (GET_AF_BATTLE(ch, kEafPoisoned))
		return false;

	if (spell_num == kSpellAconitumPoison) {
		// урон 5 + левел/2, от 5 до 20 за стак
		Affect<EApplyLocation> af;
		af.type = kSpellAconitumPoison;
		af.location = APPLY_ACONITUM_POISON;
		af.duration = 7;
		af.modifier = GetRealLevel(ch) / 2 + 5;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = kAfSameTime;
		if (poison_affect_join(vict, af)) {
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	} else if (spell_num == kSpellScopolaPoison) {
		// по +5% дамаги по целе за стак (кроме стаба)
		Affect<EApplyLocation> af;
		af.type = kSpellScopolaPoison;
		af.location = APPLY_SCOPOLIA_POISON;
		af.duration = 7;
		af.modifier = 5;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON) | to_underlying(EAffectFlag::AFF_SCOPOLIA_POISON);
		af.battleflag = kAfSameTime;
		if (poison_affect_join(vict, af)) {
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	} else if (spell_num == kSpellBelenaPoison) {
		// не переключается (моб)
		// -хитролы/хп-рег/дамаг-физ.атак/скилы

		Affect<EApplyLocation> af[3];
		// скилл * 0.05 на чаров и + 5 на мобов. 4-10% и 9-15% (80-200 скила)
		int percent = 0;
		if (ch->get_skill(ESkill::kPoisoning) >= 80) {
			percent = (ch->get_skill(ESkill::kPoisoning) * 5 / 100) + (vict->is_npc() ? 5 : 0);
		}
		// -дамаг физ.атак и скиллы
		af[0].location = APPLY_BELENA_POISON;
		af[0].modifier = percent;

		// скилл * 0.05 + 5 на чаров и + 10 на мобов. 5.5-15% и 10.5-20% (10-200 скила)
		percent = (ch->get_skill(ESkill::kPoisoning) * 5 / 100) + (vict->is_npc() ? 10 : 5);
		// -хитролы
		int remove_hit = GET_REAL_HR(vict) * (percent / 100);
		af[1].location = APPLY_HITROLL;
		af[1].modifier = -remove_hit;
		// -хп-рег
		int remove_hp = GET_HITREG(vict) * (percent / 100);
		af[2].location = APPLY_HITREG;
		af[2].modifier = -remove_hp;

		bool was_poisoned = true;
		for (auto & i : af) {
			i.type = kSpellBelenaPoison;
			i.duration = 7;
			i.bitvector = to_underlying(EAffectFlag::AFF_POISON)
				| to_underlying(EAffectFlag::AFF_BELENA_POISON)
				| to_underlying(EAffectFlag::AFF_SKILLS_REDUCE)
				| to_underlying(EAffectFlag::AFF_NOT_SWITCH);
			i.battleflag = kAfSameTime;

			if (!poison_affect_join(vict, i)) {
				was_poisoned = false;
			}
		}

		if (was_poisoned) {
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	} else if (spell_num == kSpellDaturaPoison) {
		// не переключается (моб)
		// -каст/мем-рег/дамаг-заклов/скилы
		// AFF_DATURA_POISON - флаг на снижение дамага с заклов

		Affect<EApplyLocation> af[3];
		// скилл * 0.05 на чаров и + 5 на мобов. 4-10% и 9-15% (80-200 скила)
		int percent = 0;
		if (ch->get_skill(ESkill::kPoisoning) >= 80)
			percent = (ch->get_skill(ESkill::kPoisoning) * 5 / 100) + (vict->is_npc() ? 5 : 0);
		// -дамаг заклов и скиллы
		af[0].location = APPLY_DATURA_POISON;
		af[0].modifier = percent;

		// скилл * 0.05 + 5 на чаров и + 10 на мобов. 5.5-15% и 10.5-20% (10-200 скила)
		percent = (ch->get_skill(ESkill::kPoisoning) * 5 / 100) + (vict->is_npc() ? 10 : 5);
		// -каст
		int remove_cast = GET_CAST_SUCCESS(vict) * (percent / 100);
		af[1].location = APPLY_CAST_SUCCESS;
		af[1].modifier = -remove_cast;
		// -мем
		int remove_mem = GET_MANAREG(vict) * (percent / 100);
		af[2].location = APPLY_MANAREG;
		af[2].modifier = -remove_mem;

		bool was_poisoned = true;
		for (auto & i : af) {
			i.type = kSpellDaturaPoison;
			i.duration = 7;
			i.bitvector = to_underlying(EAffectFlag::AFF_POISON)
				| to_underlying(EAffectFlag::AFF_DATURA_POISON)
				| to_underlying(EAffectFlag::AFF_SKILLS_REDUCE)
				| to_underlying(EAffectFlag::AFF_NOT_SWITCH);
			i.battleflag = kAfSameTime;

			if (!poison_affect_join(vict, i)) {
				was_poisoned = false;
			}
		}

		if (was_poisoned) {
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, kEafPoisoned);
			return true;
		}
	}
	return false;
}

// * Крит при отравлении с пушек.
void weap_crit_poison(CharData *ch, CharData *vict, int/* spell_num*/) {
	Affect<EApplyLocation> af;
	int percent = number(1, MUD::Skills()[ESkill::kPoisoning].difficulty * 3);
	int prob = CalcCurrentSkill(ch, ESkill::kPoisoning, vict);
	if (prob >= percent) {
		switch (number(1, 5)) {
			case 1:
				// аналог баша с лагом
				if (GET_POS(vict) >= EPosition::kFight) {
					if (vict->ahorse()) {
						send_to_char(ch, "%sОт действия вашего яда у %s закружилась голова!%s\r\n",
									 CCGRN(ch, C_NRM), PERS(vict, ch, 1), CCNRM(ch, C_NRM));
						send_to_char(vict, "Вы почувствовали сильное головокружение и не смогли усидеть на %s!\r\n",
									 GET_PAD(vict->get_horse(), 5));
						act("$n0 зашатал$u и не смог$q усидеть на $N5.",
							true, vict, nullptr, vict->get_horse(), kToNotVict);
						vict->drop_from_horse();
					} else {
						send_to_char(ch, "%sОт действия вашего яда у %s подкосились ноги!%s\r\n",
									 CCGRN(ch, C_NRM), PERS(vict, ch, 1), CCNRM(ch, C_NRM));
						send_to_char(vict, "Вы почувствовали сильное головокружение и не смогли устоять на ногах!\r\n");
						act("$N0 зашатал$U и не смог$Q устоять на ногах.",
							true, ch, nullptr, vict, kToNotVict);
						GET_POS(vict) = EPosition::kSit;
						WAIT_STATE(vict, 3 * kPulseViolence);
					}
					break;
				}
				break;
			case 2: {
				// минус статы (1..5)
				af.type = kSpellPoison;
				af.duration = 30;
				af.modifier = -GetRealLevel(ch) / 6;
				af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
				af.battleflag = kAfSameTime;

				for (int i = APPLY_STR; i <= APPLY_CHA; i++) {
					af.location = static_cast<EApplyLocation>(i);
					affect_join(vict, af, false, false, false, false);
				}

				send_to_char(ch, "%sОт действия вашего яда %s побледнел%s!%s\r\n",
							 CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				send_to_char(vict, "Вы почувствовали слабость во всем теле!\r\n");
				act("$N0 побледнел$G на ваших глазах.", true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 2
			case 3: {
				// минус реакция (1..5)
				af.type = kSpellPoison;
				af.duration = 30;
				af.location = APPLY_SAVING_REFLEX;
				af.modifier = GetRealLevel(ch) / 6; //Polud с плюсом, поскольку здесь чем больше - тем хуже
				af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
				af.battleflag = kAfSameTime;
				affect_join(vict, af, false, false, false, false);
				send_to_char(ch, "%sОт действия вашего яда %s стал%s хуже реагировать на движения противников!%s\r\n",
							 CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				send_to_char(vict, "Вам стало труднее реагировать на движения противников!\r\n");
				act("$N0 стал$G хуже реагировать на ваши движения!", true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 3
			case 4: {
				// минус инициатива (1..5)
				af.type = kSpellPoison;
				af.duration = 30;
				af.location = APPLY_INITIATIVE;
				af.modifier = -GetRealLevel(ch) / 6;
				af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
				af.battleflag = kAfSameTime;
				affect_join(vict, af, false, false, false, false);
				send_to_char(ch, "%sОт действия вашего яда %s стал%s заметно медленнее двигаться!%s\r\n",
							 CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				send_to_char(vict, "Вы стали заметно медленнее двигаться!\r\n");
				act("$N0 стал$G заметно медленнее двигаться!",
					true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 4
			case 5: {
				// минус живучесть (1..5)
				af.type = kSpellPoison;
				af.duration = 30;
				af.location = APPLY_RESIST_VITALITY;
				af.modifier = -GetRealLevel(ch) / 6;
				af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
				af.battleflag = kAfSameTime;
				affect_join(vict, af, false, false, false, false);
				send_to_char(ch, "%sОт действия вашего яда %s стал%s хуже переносить повреждения!%s\r\n",
							 CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
				send_to_char(vict, "Вы стали хуже переносить повреждения!\r\n");
				act("$N0 стал$G хуже переносить повреждения!", true, ch, nullptr, vict, kToNotVict);
				break;
			} // case 5
		} // switch

	}
}

} // namespace

// * Отравление с заклинания 'яд'.
void poison_victim(CharData *ch, CharData *vict, int modifier) {
	Affect<EApplyLocation> af[4];

	// change strength
	af[0].type = kSpellPoison;
	af[0].location = APPLY_STR;
	af[0].duration = CalcDuration(vict, 0, std::max(2, GetRealLevel(ch) - GetRealLevel(vict)), 2, 0, 1);
	af[0].modifier = -std::min(2, (modifier + 29) / 40);
	af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[0].battleflag = kAfSameTime;
	// change damroll
	af[1].type = kSpellPoison;
	af[1].location = APPLY_DAMROLL;
	af[1].duration = af[0].duration;
	af[1].modifier = -std::min(2, (modifier + 29) / 30);
	af[1].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[1].battleflag = kAfSameTime;
	// change hitroll
	af[2].type = kSpellPoison;
	af[2].location = APPLY_HITROLL;
	af[2].duration = af[0].duration;
	af[2].modifier = -std::min(2, (modifier + 19) / 20);
	af[2].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[2].battleflag = kAfSameTime;
	// change poison level
	af[3].type = kSpellPoison;
	af[3].location = APPLY_POISON;
	af[3].duration = af[0].duration;
	af[3].modifier = GetRealLevel(ch);
	af[3].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[3].battleflag = kAfSameTime;

	for (auto & i : af) {
		affect_join(vict, i, false, false, false, false);
	}
	vict->Poisoner = GET_ID(ch);

	snprintf(buf, sizeof(buf), "%sВы отравили $N3.%s", CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));
	act(buf, false, ch, nullptr, vict, kToChar);
	snprintf(buf, sizeof(buf), "%s$n отравил$g вас.%s", CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	act(buf, false, ch, nullptr, vict, kToVict);
}

// * Попытка травануть с пушки при ударе.
void try_weap_poison(CharData *ch, CharData *vict, int spell_num) {
	if (spell_num < 0) {
		return;
	}

	if (number(1, 200) <= 25
		|| (!GET_AF_BATTLE(vict, kEafFirstPoison) && !AFF_FLAGGED(vict, EAffectFlag::AFF_POISON))) {
		ImproveSkill(ch, ESkill::kPoisoning, true, vict);
		if (weap_poison_vict(ch, vict, spell_num)) {
			if (spell_num == kSpellAconitumPoison) {
				send_to_char(ch, "Кровоточащие язвы покрыли тело %s.\r\n",
							 PERS(vict, ch, 1));
			} else if (spell_num == kSpellScopolaPoison) {
				strcpy(buf1, PERS(vict, ch, 0));
				CAP(buf1);
				send_to_char(ch, "%s скрючил%s от нестерпимой боли.\r\n",
							 buf1, GET_CH_VIS_SUF_2(vict, ch));
				SET_AF_BATTLE(vict, kEafFirstPoison);
			} else if (spell_num == kSpellBelenaPoison) {
				strcpy(buf1, PERS(vict, ch, 3));
				CAP(buf1);
				send_to_char(ch, "%s перестали слушаться руки.\r\n", buf1);
				SET_AF_BATTLE(vict, kEafFirstPoison);
			} else if (spell_num == kSpellDaturaPoison) {
				strcpy(buf1, PERS(vict, ch, 2));
				CAP(buf1);
				send_to_char(ch, "%s стало труднее плести заклинания.\r\n", buf1);
				SET_AF_BATTLE(vict, kEafFirstPoison);
			} else {
				send_to_char(ch, "Вы отравили %s.\r\n", PERS(ch, vict, 3));
			}
			send_to_char(vict, "%s%s отравил%s вас.%s\r\n",
						 CCIRED(ch, C_NRM), PERS(ch, vict, 0),
						 GET_CH_VIS_SUF_1(ch, vict), CCNRM(ch, C_NRM));
			weap_crit_poison(ch, vict, spell_num);
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
		weapon->add_timed_spell(kSpellAconitumPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_SCOPOLIA)
		weapon->add_timed_spell(kSpellScopolaPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_BELENA)
		weapon->add_timed_spell(kSpellBelenaPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_DATURA)
		weapon->add_timed_spell(kSpellDaturaPoison, poison_timer);
	else
		log("SYSERROR: liquid_num == %d (%s %s %d)", liquid_num, __FILE__, __func__, __LINE__);
}

// * Вывод имени яда по номеру его заклинания (для осмотра пушек).
std::string get_poison_by_spell(int spell) {
	switch (spell) {
		case kSpellAconitumPoison: return drinknames[LIQ_POISON_ACONITUM];
		case kSpellScopolaPoison: return drinknames[LIQ_POISON_SCOPOLIA];
		case kSpellBelenaPoison: return drinknames[LIQ_POISON_BELENA];
		case kSpellDaturaPoison: return drinknames[LIQ_POISON_DATURA];
		default: return "";
	}
	//return "";
}

// * Проверка, является ли заклинание ядом.
bool check_poison(int spell) {
	switch (spell) {
		case kSpellAconitumPoison:
		case kSpellScopolaPoison:
		case kSpellBelenaPoison:
		case kSpellDaturaPoison: return true;
		default: return false;
	}
	//return false;
}

/**
* Пока с AF_SAME_TIME получилось следующее (в бою/вне боя):
* APPLY_POISON - у плеера раз в 2 секунды везде, у моба раз в минуту везде.
* Остальные аффекты - у плеера раз в 2 секунды везде, у моба в бою раз в 2 секунды, вне боя - раз в минуту.
*/
int processPoisonDamage(CharData *ch, const Affect<EApplyLocation>::shared_ptr &af) {
	int result = 0;
	if (af->location == APPLY_POISON) {
		int poison_dmg = GET_POISON(ch) * (ch->is_npc() ? 4 : 5);
		// мобов яд ядит на тике, а чаров каждый батл тик соответсвенно если это не моб надо делить на 30 или тип того
		if (!ch->is_npc())
			poison_dmg = poison_dmg / 30;
		//poison_dmg = interpolate(poison_dmg, 2); // И как оно должно работать чото нифига не понял, понял только что оно не работает
		Damage dmg(SpellDmg(kSpellPoison), poison_dmg, fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		result = dmg.Process(ch, ch);
	} else if (af->location == APPLY_ACONITUM_POISON) {
		Damage dmg(SpellDmg(kSpellPoison), GET_POISON(ch), fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		result = dmg.Process(ch, ch);
	}
	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
