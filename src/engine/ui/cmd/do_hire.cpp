#include "do_hire.h"

#include "do_follow.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/dungeons.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <cmath>

constexpr short kMaxHireTime = 10080 / 2; // Неделя в минутах пополам
constexpr long kMaxHirePrice = LONG_MAX / (kMaxHireTime + 1);

//Функции для модифицированного чарма
float get_damage_per_round(CharData *victim) {
	float dam_per_attack = GET_DR(victim) + str_bonus(victim->get_str(), STR_TO_DAM)
		+ victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2.0
		+ (AFF_FLAGGED(victim, EAffect::kCloudOfArrows) ? 14 : 0);
	int num_attacks = 1 + victim->mob_specials.extra_attack
		+ (victim->GetSkill(ESkill::kAddshot) ? 2 : 0);

	float dam_per_round = dam_per_attack * num_attacks;

	//Если дыхание - то дамаг умножается
	if (victim->IsFlagged(EMobFlag::kFireBreath) ||
		victim->IsFlagged(EMobFlag::kGasBreath) ||
		victim->IsFlagged(EMobFlag::kFrostBreath) ||
		victim->IsFlagged(EMobFlag::kAcidBreath) ||
		victim->IsFlagged(EMobFlag::kLightingBreath)) {
		dam_per_round *= 1.3f;
	}

	return dam_per_round;
}

float calc_cha_for_hire(CharData *victim) {
	int i;
	float reformed_hp = 0.0, needed_cha = 0.0;
	for (i = 0; i < 50; i++) {
		reformed_hp = victim->get_max_hit() + get_damage_per_round(victim) * cha_app[i].dam_to_hit_rate;
		if (cha_app[i].charms >= reformed_hp)
			break;
	}
	i = POSI(i);
	needed_cha = i - 1 + (reformed_hp - cha_app[i - 1].charms) / (cha_app[i].charms - cha_app[i - 1].charms);
	return VPOSI<float>(needed_cha, 1.0, 50.0);
}

long calc_hire_price(CharData *ch, CharData *victim) {
	float price = 0; // стоимость найма
	int m_str = victim->get_str() * 20;
	int m_int = victim->get_int() * 20;
	int m_wis = victim->get_wis() * 20;
	int m_dex = victim->get_dex() * 20;
	int m_con = victim->get_con() * 20;
	int m_cha = victim->get_cha() * 10;
	ch->send_to_TC(true, true, true, "Базовые статы найма: Str:%d Int:%d Wis:%d Dex:%d Con:%d Cha:%d\r\n",
				   m_str, m_int, m_wis, m_dex, m_con, m_cha);
	price += m_str + m_int + m_wis + m_dex + m_con + m_cha;

	float m_hit = victim->get_max_hit() * 2;
	float m_lvl = victim->GetLevel() * 50;
	float m_ac = GET_AC(victim) * (-5);
	float m_hr = GET_HR(victim) * 50;
	float m_armor = GET_ARMOUR(victim) * 25;
	float m_absorb = GET_ABSORBE(victim) * 4;
	ch->send_to_TC(true,
				   true,
				   true,
				   "Статы живучести: HP:%.4lf LVL:%.4lf AC:%.4lf HR:%.4lf ARMOR:%.4lf ABSORB:%.4lf\r\n",
				   m_hit,
				   m_lvl,
				   m_ac,
				   m_hr,
				   m_armor,
				   m_absorb);

	price += m_hit + m_lvl + m_ac + m_hr + m_armor + m_absorb;

	int m_stab = GetSave(victim, ESaving::kStability) * (-4);
	int m_ref = GetSave(victim, ESaving::kReflex) * (-4);
	int m_crit = GetSave(victim, ESaving::kCritical) * (-4);
	int m_wil = GetSave(victim, ESaving::kWill) * (-4);
	ch->send_to_TC(true, true, true, "Сейвы: STAB:%d REF:%d CRIT:%d WILL:%d\r\n",
				   m_stab, m_ref, m_crit, m_wil);
	price += m_stab + m_ref + m_crit + m_wil;
	// магические резисты
	int m_fire = GET_RESIST(victim, EResist::kFire) * 4;
	int m_air = GET_RESIST(victim, EResist::kAir) * 4;
	int m_water = GET_RESIST(victim, EResist::kWater) * 4;
	int m_earth = GET_RESIST(victim, EResist::kEarth) * 4;
	int m_vita = GET_RESIST(victim, EResist::kVitality) * 4;
	int m_mind = GET_RESIST(victim, EResist::kMind) * 4;
	int m_immu = GET_RESIST(victim, EResist::kImmunity) * 4;
	int m_dark = GET_RESIST(victim, EResist::kDark) * 4;
	ch->send_to_TC(true, true, true,
				   "Маг.резисты: Fire:%d Air:%d Water:%d Earth:%d Vita:%d Mind:%d Immu:%d Dark:%d\r\n",
				   m_fire, m_air, m_water, m_earth, m_vita, m_mind, m_immu, m_dark);
	price += m_fire + m_air + m_water + m_earth + m_vita + m_mind + m_immu + m_dark;
	// удача и инициатива
	int m_luck = victim->calc_morale() * 10;
	int m_ini = GET_INITIATIVE(victim) * 10;
	// сопротивления
	int m_ar = GET_AR(victim) * 50;
	int m_mr = GET_MR(victim) * 50;
	int m_pr = GET_PR(victim) * 50;
	// дамаг
	int m_dr = ((victim->mob_specials.damnodice * victim->mob_specials.damsizedice + victim->mob_specials.damnodice) / 2
		+ GET_DR(victim)) * 10;
	float extraAttack = victim->mob_specials.extra_attack * m_dr;

	ch->send_to_TC(true, true, true, "Остальные статы: Luck:%d Ini:%d AR:%d MR:%d PR:%d DR:%d ExAttack:%.4lf\r\n",
				   m_luck, m_ini, m_ar, m_mr, m_pr, m_dr, extraAttack);

	price += m_luck + m_ini + m_ar + m_mr + m_pr + m_dr + extraAttack;
	// сколько персонаж может
	float hirePoints = 0;
	float rem_hirePoints = GetRealRemort(ch) * 1.8;
	float int_hirePoints = GetRealInt(ch) * 1.8;
	float cha_hirePoints = GetRealCha(ch) * 1.8;
	hirePoints += rem_hirePoints + int_hirePoints + cha_hirePoints;
	hirePoints = hirePoints * 5 * GetRealLevel(ch);

	int min_price = MAX((m_dr / 300 * GetRealLevel(victim)), (GetRealLevel(victim) * 5));
	min_price = MAX(min_price, mob_proto[GET_MOB_RNUM(victim)].get_gold());
	long finalPrice = MAX(min_price, (int) ceil(price - hirePoints));

	ch->send_to_TC(true, true, true,
				   "Параметры персонажа: RMRT: %.4lf, CHA: %.4lf, INT: %.4lf, TOTAL: %.4lf. Цена чармиса:  %.4lf. Итоговая цена: %d \r\n",
				   rem_hirePoints, cha_hirePoints, int_hirePoints, hirePoints, price, finalPrice);
	return std::min(finalPrice, kMaxHirePrice);
}

int GetReformedCharmiceHp(CharData *ch, CharData *victim, ESpell spell_id) {
	auto r_hp{0.0};
	auto eff_cha{0.0};
	auto stat_cap{0.0};

	if (spell_id == ESpell::kResurrection || spell_id == ESpell::kAnimateDead) {
		eff_cha = CalcEffectiveWis(ch, spell_id);
		stat_cap = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kWis);
	} else {
		stat_cap = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kCha);
		eff_cha = get_effective_cha(ch);
	}

	if (spell_id != ESpell::kCharm) {
		eff_cha = std::min(stat_cap, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
	}

	// Интерполяция между значениями для целых значений обаяния
	if (eff_cha < stat_cap) {
		r_hp = victim->get_max_hit() + get_damage_per_round(victim) *
			((1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].dam_to_hit_rate +
				(eff_cha - (int) eff_cha) * cha_app[(int) eff_cha + 1].dam_to_hit_rate);
	} else {
		r_hp = victim->get_max_hit() + get_damage_per_round(victim) *
			((1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].dam_to_hit_rate);
	}

	if (ch->IsFlagged(EPrf::kTester))
		SendMsgToChar(ch, "&Gget_reformed_charmice_hp Расчет чарма r_hp = %f \r\n&n", r_hp);
	return (int) r_hp;
}

void do_findhelpee(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()
		|| (!IS_IMMORTAL(ch) && !CanUseFeat(ch, EFeat::kEmployer))) {
		SendMsgToChar("Вам недоступно это!\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg);

	if (!*arg) {
		FollowerType *k;
		for (k = ch->followers; k; k = k->next) {
			if (AFF_FLAGGED(k->follower, EAffect::kHelper) && AFF_FLAGGED(k->follower, EAffect::kCharmed)) {
				break;
			}
		}

		if (k) {
			act("Вашим наемником является $N.", false, ch, 0, k->follower, kToChar);
		} else {
			act("У вас нет наемников!", false, ch, 0, 0, kToChar);
		}
		return;
	}

	const auto helpee = get_char_vis(ch, arg, EFind::kCharInRoom);
	if (!helpee) {
		SendMsgToChar("Вы не видите никого похожего.\r\n", ch);
		return;
	}

	FollowerType *k;
	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->follower, EAffect::kHelper) && AFF_FLAGGED(k->follower, EAffect::kCharmed)) {
			break;
		}
	}

	if (helpee == ch)
		SendMsgToChar("И как вы это представляете - нанять самого себя?\r\n", ch);
	else if (!helpee->IsNpc())
		SendMsgToChar("Вы не можете нанять реального игрока!\r\n", ch);
	else if (!NPC_FLAGGED(helpee, ENpcFlag::kHelped))
		act("$N не нанимается!", false, ch, 0, helpee, kToChar);
	else if (AFF_FLAGGED(helpee, EAffect::kCharmed) && (!k || (k && helpee != k->follower)))
		act("$N под чьим-то контролем.", false, ch, 0, helpee, kToChar);
	else if (AFF_FLAGGED(helpee, EAffect::kDeafness))
		act("$N не слышит вас.", false, ch, 0, helpee, kToChar);
	else if (IS_HORSE(helpee))
		SendMsgToChar("Это боевой скакун, а не хухры-мухры.\r\n", ch);
	else if (helpee->GetEnemy() || helpee->GetPosition() < EPosition::kRest)
		act("$M сейчас, похоже, не до вас.", false, ch, 0, helpee, kToChar);
	else if (circle_follow(helpee, ch))
		SendMsgToChar("Следование по кругу запрещено.\r\n", ch);
	else if (GetRealRemort(ch) < GetRealRemort(helpee))
		act("$N сказал вам: \"Ты слишком слаб, чтобы нанять меня\".", false, ch, 0, helpee, kToChar);
	else {
		// Вы издеваетесь? Блок else на три экрана, реально?
		// Svent TODO: Вынести проверку на корректность чармиса в отдельную функицю.
		char isbank[kMaxStringLength];
		two_arguments(argument, arg, isbank);

		unsigned int times = 0;
		times = atoi(arg);
		if (!*arg || times == 0) {
			const auto cost = calc_hire_price(ch, helpee);
			sprintf(buf, "$n сказал$g вам : \"Один час моих услуг стоит %ld %s\".\r\n",
					cost, GetDeclensionInNumber(cost, EWhat::kMoneyU));
			act(buf, false, helpee, 0, ch, kToVict | kToNotDeaf);
			return;
		}
		if (k && helpee != k->follower) {
//		if (k && helpee) {
			act("Вы уже наняли $N3.", false, ch, 0, k->follower, kToChar);
			return;
		}

		if (times > kMaxHireTime) {
			SendMsgToChar("Пожизненный найм запрещен!\r\n", ch);
			times = kMaxHireTime;
		}

		auto hire_price = calc_hire_price(ch, helpee);
		long cost = times * hire_price;

		if ((!isname(isbank, "банк bank") && cost > ch->get_gold()) ||
			(isname(isbank, "банк bank") && cost > ch->get_bank())) {
			sprintf(buf,
					"$n сказал$g вам : \" Мои услуги за %d %s стоят %ld %s - это тебе не по карману.\"",
					times,
					GetDeclensionInNumber(times, EWhat::kHour), cost, GetDeclensionInNumber(cost, EWhat::kMoneyU));
			act(buf, false, helpee, 0, ch, kToVict | kToNotDeaf);
			return;
		}

		if (helpee->has_master() && helpee->get_master() != ch) {
			if (stop_follower(helpee, kSfMasterdie)) {
				return;
			}
		}
		if (helpee->IsFlagged(EMobFlag::kNoGroup))
			helpee->UnsetFlag(EMobFlag::kNoGroup);

		Affect<EApply> af;
		if (!(k && k->follower == helpee)) {
			ch->add_follower(helpee);
			af.duration = CalcDuration(helpee, times * kTimeKoeff, 0, 0, 0, 0);
		} else {
			auto aff = k->follower->affected.begin();
			for (; aff != k->follower->affected.end(); ++aff) {
				if ((*aff)->type == ESpell::kCharm) {
					break;
				}
			}
			if (aff != k->follower->affected.end()) {
				long oldcost = MAX(0, (int) (((*aff)->duration - 1) / 2) * (int) abs(k->follower->mob_specials.hire_price));
				if (oldcost > 0) {
					if (k->follower->mob_specials.hire_price < 0) {
						ch->add_bank(oldcost);
					} else {
						ch->add_gold(oldcost);
					}
					SendMsgToChar(ch, "Вам вернули нерастраченный задаток в %ld %s.\r\n",
								  oldcost, GetDeclensionInNumber(cost, EWhat::kMoneyA));
				}
				af.duration = CalcDuration(helpee, times * kTimeKoeff, 0, 0, 0, 0);
			}
		}
		RemoveAffectFromChar(helpee, ESpell::kCharm);
		if (!IS_IMMORTAL(ch)) {
			if (isname(isbank, "банк bank")) {
				ch->remove_bank(cost);
				helpee->mob_specials.hire_price = -hire_price;
			} else {
				ch->remove_gold(cost);
				helpee->mob_specials.hire_price = hire_price;
			}
		}

		af.type = ESpell::kCharm;
		af.modifier = 0;
		af.location = EApply::kNone;
		af.bitvector = to_underlying(EAffect::kCharmed);
		af.battleflag = 0;
		affect_to_char(helpee, af);

		af.type = ESpell::kCharm;
		af.modifier = 0;
		af.location = EApply::kNone;
		af.bitvector = to_underlying(EAffect::kHelper);
		af.battleflag = 0;
		affect_to_char(helpee, af);

		sprintf(buf, "$n сказал$g вам : \"Приказывай, %s!\"", IS_FEMALE(ch) ? "хозяйка" : "хозяин");
		act(buf, false, helpee, 0, ch, kToVict | kToNotDeaf);

		if (helpee->IsNpc()) {
			for (auto i = 0; i < EEquipPos::kNumEquipPos; i++) {
				if (GET_EQ(helpee, i)) {
					if (!remove_otrigger(GET_EQ(helpee, i), helpee))
						continue;

					act("Вы прекратили использовать $o3.", false, helpee, GET_EQ(helpee, i), 0, kToChar);
					act("$n прекратил$g использовать $o3.", true, helpee, GET_EQ(helpee, i), 0, kToRoom);
					PlaceObjToInventory(UnequipChar(helpee, i, CharEquipFlag::show_msg), helpee);
				}
			}

			helpee->UnsetFlag(EMobFlag::kAgressive);
			helpee->UnsetFlag(EMobFlag::kSpec);
			helpee->UnsetFlag(EPrf::kPunctual);
			helpee->SetFlag(EMobFlag::kNoSkillTrain);
			helpee->set_skill(ESkill::kPunctual, 0);
			if (!NPC_FLAGGED(ch, ENpcFlag::kNoMercList)) {
				MobVnum mvn = GET_MOB_VNUM(helpee);

				if (mvn / 100 >=  dungeons::kZoneStartDungeons) {
					mvn = zone_table[GetZoneRnum(mvn / 100)].copy_from_zone * 100 + mvn % 100;
				}
				ch->updateCharmee(mvn, cost);
			}
			Crash_crashsave(ch);
			ch->save_char();
		}
	}
}

void do_freehelpee(CharData *ch, char * /* argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()
		|| (!IS_IMMORTAL(ch) && !CanUseFeat(ch, EFeat::kEmployer))) {
		SendMsgToChar("Вам недоступно это!\r\n", ch);
		return;
	}

	FollowerType *k;
	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->follower, EAffect::kHelper)
			&& AFF_FLAGGED(k->follower, EAffect::kCharmed)) {
			break;
		}
	}

	if (!k) {
		act("У вас нет наемников!", false, ch, 0, 0, kToChar);
		return;
	}

	if (ch->in_room != k->follower->in_room) {
		act("Вам следует встретиться с $N4 для этого.", false, ch, 0, k->follower, kToChar);
		return;
	}

	if (k->follower->GetPosition() < EPosition::kStand) {
		act("$N2 сейчас, похоже, не до вас.", false, ch, 0, k->follower, kToChar);
		return;
	}

	if (!IS_IMMORTAL(ch)) {
		for (const auto &aff : k->follower->affected) {
			if (aff->type == ESpell::kCharm) {
				long cost = MAX(0, (int) ((aff->duration - 1) / 2) * (int) abs(k->follower->mob_specials.hire_price));
				if (cost > 0) {
					if (k->follower->mob_specials.hire_price < 0) {
						ch->add_bank(cost);
					} else {
						ch->add_gold(cost);
					}
					SendMsgToChar(ch, "Вам вернули нерастраченный задаток в %ld %s.\r\n", cost, GetDeclensionInNumber(cost, EWhat::kMoneyA));
				}
				break;
			}
		}
	}

	act("Вы рассчитали $N3.", false, ch, 0, k->follower, kToChar);
	RemoveAffectFromCharAndRecalculate(k->follower, ESpell::kCharm);
	stop_follower(k->follower, kSfCharmlost);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

