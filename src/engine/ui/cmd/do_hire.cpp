#include "do_hire.h"
#include "utils/logger.h"
#include "gameplay/core/remort.h"
#include "administration/privilege.h"
#include "gameplay/economics/currencies.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"

#include "do_follow.h"
#include "engine/core/char_equip_flags.h"
#include "engine/db/obj_save.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/minions.h"

#include <fmt/format.h>

#include <cmath>

constexpr short kMaxHireTime = 10080 / 2; // Неделя в минутах пополам
constexpr long kMaxHirePrice = LONG_MAX / (kMaxHireTime + 1);

//Функции для модифицированного чарма

float CalcChaForHire(CharData *victim) {
	int i;
	float reformed_hp = 0.0, needed_cha = 0.0;
	for (i = 0; i < 50; i++) {
		reformed_hp = victim->get_max_hit() + CalcDamagePerRound(victim) * ChaApp(i).dam_to_hit_rate;
		if (ChaApp(i).charms >= reformed_hp)
			break;
	}
	i = Posi(i);
	needed_cha = i - 1 + (reformed_hp - ChaApp(i - 1).charms) / (ChaApp(i).charms - ChaApp(i - 1).charms);
	return VPOSI<float>(needed_cha, 1.0, 50.0);
}

long CalcHirePrice(CharData *ch, CharData *victim) {
	float price = 0; // стоимость найма
	int m_str = victim->get_str() * 20;
	int m_int = victim->get_int() * 20;
	int m_wis = victim->get_wis() * 20;
	int m_dex = victim->get_dex() * 20;
	int m_con = victim->get_con() * 20;
	int m_cha = victim->get_cha() * 10;
	SendToTC(ch, true, true, true, "Базовые статы найма: Str:%d Int:%d Wis:%d Dex:%d Con:%d Cha:%d\r\n",
				   m_str, m_int, m_wis, m_dex, m_con, m_cha);
	price += m_str + m_int + m_wis + m_dex + m_con + m_cha;

	float m_hit = victim->get_max_hit() * 2;
	float m_lvl = victim->GetLevel() * 50;
	float m_ac = GET_AC(victim) * (-5);
	float m_hr = GET_HR(victim) * 50;
	float m_armor = GET_ARMOUR(victim) * 25;
	float m_absorb = GET_ABSORBE(victim) * 4;
	SendToTC(ch, true,
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
	SendToTC(ch, true, true, true, "Сейвы: STAB:%d REF:%d CRIT:%d WILL:%d\r\n",
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
	SendToTC(ch, true, true, true,
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

	SendToTC(ch, true, true, true, "Остальные статы: Luck:%d Ini:%d AR:%d MR:%d PR:%d DR:%d ExAttack:%.4lf\r\n",
				   m_luck, m_ini, m_ar, m_mr, m_pr, m_dr, extraAttack);

	price += m_luck + m_ini + m_ar + m_mr + m_pr + m_dr + extraAttack;
	// сколько персонаж может
	float hirePoints = 0;
	float rem_hirePoints = remort::GetRealRemort(ch) * 1.8;
	float int_hirePoints = GetRealInt(ch) * 1.8;
	float cha_hirePoints = GetRealCha(ch) * 1.8;
	hirePoints += rem_hirePoints + int_hirePoints + cha_hirePoints;
	hirePoints = hirePoints * 5 * GetRealLevel(ch);

	int min_price = MAX((m_dr / 300 * GetRealLevel(victim)), (GetRealLevel(victim) * 5));
	min_price = MAX(min_price, currencies::GetHand(mob_proto[victim->get_rnum()], currencies::kGold));
	long finalPrice = MAX(min_price, (int) ceil(price - hirePoints));

	SendToTC(ch, true, true, true,
				   "Параметры персонажа: RMRT: %.4lf, CHA: %.4lf, INT: %.4lf, TOTAL: %.4lf. Цена чармиса:  %.4lf. Итоговая цена: %ld \r\n",
				   rem_hirePoints, cha_hirePoints, int_hirePoints, hirePoints, price, finalPrice);
	return std::min(finalPrice, kMaxHirePrice);
}

void DoFindhelpee(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()
		|| (!privilege::IsImmortal(ch) && !CanUseFeat(ch, EFeat::kEmployer))) {
		SendMsgToChar("Вам недоступно это!\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg);

	if (!*arg) {
		CharData *hired = nullptr;
		for (auto *k : ch->followers) {
			if (AFF_FLAGGED(k, EAffect::kHelper) && AFF_FLAGGED(k, EAffect::kCharmed)) {
				hired = k;
				break;
			}
		}

		if (hired) {
			act("Вашим наемником является $N.", false, ch, 0, hired, kToChar);
		} else {
			act("У вас нет наемников!", false, ch, 0, 0, kToChar);
		}
		return;
	}

	CharData *helpee = nullptr;

	helpee = target_resolver::FindCharInRoom(ch, arg);
	if (!helpee) {
		SendMsgToChar("Вы не видите никого похожего.\r\n", ch);
		return;
	}

	CharData *hired = nullptr;
	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kHelper) && AFF_FLAGGED(k, EAffect::kCharmed)) {
			hired = k;
			break;
		}
	}

	if (helpee == ch)
		SendMsgToChar("И как вы это представляете - нанять самого себя?\r\n", ch);
	else if (!helpee->IsNpc())
		SendMsgToChar("Вы не можете нанять реального игрока!\r\n", ch);
	else if (!NPC_FLAGGED(helpee, ENpcFlag::kHelped))
		act("$N не нанимается!", false, ch, 0, helpee, kToChar);
	else if (AFF_FLAGGED(helpee, EAffect::kCharmed) && (!hired || (hired && helpee != hired)))
		act("$N под чьим-то контролем.", false, ch, 0, helpee, kToChar);
	else if (AFF_FLAGGED(helpee, EAffect::kDeafness))
		act("$N не слышит вас.", false, ch, 0, helpee, kToChar);
	else if (mount::IsHorse(helpee))
		SendMsgToChar("Это боевой скакун, а не хухры-мухры.\r\n", ch);
	else if (helpee->GetEnemy() || helpee->GetPosition() < EPosition::kRest)
		act("$M сейчас, похоже, не до вас.", false, ch, 0, helpee, kToChar);
	else if (follow::CircleFollow(helpee, ch))
		SendMsgToChar("Следование по кругу запрещено.\r\n", ch);
	else if (remort::GetRealRemort(ch) < remort::GetRealRemort(helpee))
		act("$N сказал вам: \"Ты слишком слаб, чтобы нанять меня\".", false, ch, 0, helpee, kToChar);
	else {
		// Вы издеваетесь? Блок else на три экрана, реально?
		// Svent TODO: Вынести проверку на корректность чармиса в отдельную функицю.
		char isbank[kMaxStringLength];
		two_arguments(argument, arg, isbank);

		unsigned int times = 0;
		times = atoi(arg);
		if (!*arg || times == 0) {
			const auto cost = CalcHirePrice(ch, helpee);
			sprintf(buf, "$n сказал$g вам : \"Один час моих услуг стоит %ld %s\".\r\n",
					cost, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str());
			act(buf, false, helpee, 0, ch, kToVict | kToNotDeaf);
			return;
		}
		if (hired && helpee != hired) {
//		if (hired && helpee) {
			act("Вы уже наняли $N3.", false, ch, 0, hired, kToChar);
			return;
		}

		if (times > kMaxHireTime) {
			SendMsgToChar("Пожизненный найм запрещен!\r\n", ch);
			times = kMaxHireTime;
		}

		auto hire_price = CalcHirePrice(ch, helpee);
		long cost = times * hire_price;

		if ((!isname(isbank, "банк bank") && cost > currencies::GetHand(*ch, currencies::kGold)) ||
			(isname(isbank, "банк bank") && cost > currencies::GetBank(*ch, currencies::kGold))) {
			sprintf(buf,
					"$n сказал$g вам : \" Мои услуги за %d %s стоят %ld %s - это тебе не по карману.\"",
					times,
					grammar::GetDeclensionInNumber(times, grammar::EWhat::kHour), cost, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str());
			act(buf, false, helpee, 0, ch, kToVict | kToNotDeaf);
			return;
		}

		if (helpee->has_master() && helpee->get_master() != ch) {
			if (follow::StopFollower(helpee, follow::kSfMasterdie)) {
				return;
			}
		}
		if (helpee->IsFlagged(EMobFlag::kNoGroup))
			helpee->UnsetFlag(EMobFlag::kNoGroup);

		Affect<EApply> af;
		if (!(hired && hired == helpee)) {
			follow::AddFollower(ch, helpee);
			af.duration = CalcDuration(helpee, helpee, ESkill::kUndefined, times * kTimeKoeff, 0, 0, 0);
		} else {
			auto aff = hired->affected.begin();
			for (; aff != hired->affected.end(); ++aff) {
				if (IS_SET((*aff)->battleflag, kAfCharmBond)) {
					break;
				}
			}
			if (aff != hired->affected.end()) {
				long oldcost = MAX(0, (int) (((*aff)->duration - 1) / 2) * (int) abs(hired->mob_specials.hire_price));
				if (oldcost > 0) {
					if (hired->mob_specials.hire_price < 0) {
						currencies::AddBank(*ch, currencies::kGold, oldcost);
					} else {
						currencies::AddHand(*ch, currencies::kGold, oldcost);
					}
					SendMsgToChar(ch, "Вам вернули нерастраченный задаток в %ld %s.\r\n",
								  oldcost, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str());
				}
				af.duration = CalcDuration(helpee, helpee, ESkill::kUndefined, times * kTimeKoeff, 0, 0, 0);
			}
		}
		RemoveCharmBond(helpee);
		if (!privilege::IsImmortal(ch)) {
			if (isname(isbank, "банк bank")) {
				currencies::RemoveBank(*ch, currencies::kGold, cost);
				helpee->mob_specials.hire_price = -hire_price;
			} else {
				currencies::RemoveHand(*ch, currencies::kGold, cost);
				helpee->mob_specials.hire_price = hire_price;
			}
		}

		af.modifier = 0;
		af.location = EApply::kNone;
		af.affect_type = EAffect::kCharmed;
		af.battleflag = kAfCharmBond;
		affect_to_char(helpee, af);
		helpee->SetFlag(EMobFlag::kCompanion);	// any NPC ally

		af.modifier = 0;
		af.location = EApply::kNone;
		af.affect_type = EAffect::kHelper;
		af.battleflag = kAfCharmBond;
		affect_to_char(helpee, af);

		sprintf(buf, "$n сказал$g вам : \"Приказывай, %s!\"", IsFemale(ch) ? "хозяйка" : "хозяин");
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
			SetSkill(helpee, ESkill::kPunctual, 0);
			if (!NPC_FLAGGED(helpee, ENpcFlag::kNoMercList)) {
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

void DoFreehelpee(CharData *ch, char * /* argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()
		|| (!privilege::IsImmortal(ch) && !CanUseFeat(ch, EFeat::kEmployer))) {
		SendMsgToChar("Вам недоступно это!\r\n", ch);
		return;
	}

	CharData *hired = nullptr;
	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kHelper)
			&& AFF_FLAGGED(k, EAffect::kCharmed)) {
			hired = k;
			break;
		}
	}

	if (!hired) {
		act("У вас нет наемников!", false, ch, 0, 0, kToChar);
		return;
	}

	if (ch->in_room != hired->in_room) {
		act("Вам следует встретиться с $N4 для этого.", false, ch, 0, hired, kToChar);
		return;
	}

	if (hired->GetPosition() < EPosition::kStand) {
		act("$N2 сейчас, похоже, не до вас.", false, ch, 0, hired, kToChar);
		return;
	}

	if (!privilege::IsImmortal(ch)) {
		for (const auto &aff : hired->affected) {
			if (IS_SET(aff->battleflag, kAfCharmBond)) {
				long cost = MAX(0, (int) ((aff->duration - 1) / 2) * (int) abs(hired->mob_specials.hire_price));
				if (cost > 0) {
					if (hired->mob_specials.hire_price < 0) {
						currencies::AddBank(*ch, currencies::kGold, cost);
					} else {
						currencies::AddHand(*ch, currencies::kGold, cost);
					}
					SendMsgToChar(ch, "Вам вернули нерастраченный задаток в %ld %s.\r\n", cost, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str());
				}
				break;
			}
		}
	}

	act("Вы рассчитали $N3.", false, ch, 0, hired, kToChar);
	RemoveCharmBond(hired, true);
	follow::StopFollower(hired, follow::kSfCharmlost);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

