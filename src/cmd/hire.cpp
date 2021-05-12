#include "hire.h"

#include "cmd/follow.h"
#include "handler.h"
#include "screen.h"

#include <algorithm>

constexpr short MAX_HIRE_TIME = 10080 / 2;
constexpr long MAX_HIRE_PRICE = LONG_MAX / (MAX_HIRE_TIME + 1);

//Функции для модифицированного чарма
float get_damage_per_round(CHAR_DATA *victim) {
  float dam_per_attack = GET_DR(victim) + str_bonus(victim->get_str(), STR_TO_DAM)
      + victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2.0
      + (AFF_FLAGGED(victim, EAffectFlag::AFF_CLOUD_OF_ARROWS) ? 14 : 0);
  int num_attacks = 1 + victim->mob_specials.ExtraAttack
      + (victim->get_skill(SKILL_ADDSHOT) ? 2 : 0);

  float dam_per_round = dam_per_attack * num_attacks;

  //Если дыхание - то дамаг умножается
  if (MOB_FLAGGED(victim, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH | MOB_ACIDBREATH | MOB_LIGHTBREATH))) {
    dam_per_round *= 1.3f;
  }

  return dam_per_round;
}

float calc_cha_for_hire(CHAR_DATA *victim) {
  int i;
  float reformed_hp = 0.0, needed_cha = 0.0;
  for (i = 0; i < 50; i++) {
    reformed_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) * cha_app[i].dam_to_hit_rate;
    if (cha_app[i].charms >= reformed_hp)
      break;
  }
  i = POSI(i);
  needed_cha = i - 1 + (reformed_hp - cha_app[i - 1].charms) / (cha_app[i].charms - cha_app[i - 1].charms);
  return VPOSI<float>(needed_cha, 1.0, 50.0);
}

long calc_hire_price(CHAR_DATA *ch, CHAR_DATA *victim) {
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
  float m_lvl = victim->get_level() * 50;
  float m_ac = GET_AC(victim) * (-5);
  float m_hr = GET_HR(victim) * 50;
  float m_armor = GET_ARMOUR(victim) * 25;
  float m_absorb = GET_ABSORBE(victim) * 4;
  ch->send_to_TC(true, true, true, "Статы живучести: HP:%.4lf LVL:%.4lf AC:%.4lf HR:%.4lf ARMOR:%.4lf ABSORB:%.4lf\r\n",
                 m_hit, m_lvl, m_ac, m_hr, m_armor, m_absorb);

  price += m_hit + m_lvl + m_ac + m_hr + m_armor + m_absorb;

  int m_stab = GET_SAVE(victim, SAVING_STABILITY) * (-4);
  int m_ref = GET_SAVE(victim, SAVING_REFLEX) * (-4);
  int m_crit = GET_SAVE(victim, SAVING_CRITICAL) * (-4);
  int m_wil = GET_SAVE(victim, SAVING_WILL) * (-4);
  ch->send_to_TC(true, true, true, "Сейвы: STAB:%d REF:%d CRIT:%d WILL:%d\r\n",
                 m_stab, m_ref, m_crit, m_wil);
  price += m_stab + m_ref + m_crit + m_wil;
  // магические резисты
  int m_fire = GET_RESIST(victim, FIRE_RESISTANCE) * 4;
  int m_air = GET_RESIST(victim, AIR_RESISTANCE) * 4;
  int m_water = GET_RESIST(victim, WATER_RESISTANCE) * 4;
  int m_earth = GET_RESIST(victim, EARTH_RESISTANCE) * 4;
  int m_vita = GET_RESIST(victim, VITALITY_RESISTANCE) * 4;
  int m_mind = GET_RESIST(victim, MIND_RESISTANCE) * 4;
  int m_immu = GET_RESIST(victim, IMMUNITY_RESISTANCE) * 4;
  int m_dark = GET_RESIST(victim, DARK_RESISTANCE) * 4;
  ch->send_to_TC(true, true, true, "Маг.резисты: Fire:%d Air:%d Water:%d Earth:%d Vita:%d Mind:%d Immu:%d Dark:%d\r\n",
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
  float extraAttack = victim->mob_specials.ExtraAttack * m_dr;

  ch->send_to_TC(true, true, true, "Остальные статы: Luck:%d Ini:%d AR:%d MR:%d PR:%d DR:%d ExAttack:%.4lf\r\n",
                 m_luck, m_ini, m_ar, m_mr, m_pr, m_dr, extraAttack);

  price += m_luck + m_ini + m_ar + m_mr + m_pr + m_dr + extraAttack;
  // сколько персонаж может
  float hirePoints = 0;
  float rem_hirePoints = GET_REMORT(ch) * 1.8;
  float int_hirePoints = GET_REAL_INT(ch) * 1.8;
  float cha_hirePoints = GET_REAL_CHA(ch) * 1.8;
  hirePoints += rem_hirePoints + int_hirePoints + cha_hirePoints;
  hirePoints = hirePoints * 5 * GET_LEVEL(ch);

  int min_price = MAX((m_dr / 300 * GET_LEVEL(victim)), (GET_LEVEL(victim) * 5));
  long finalPrice = MAX(min_price, (int) ceil(price - hirePoints));

  ch->send_to_TC(true,
                 true,
                 true,
                 "Параметры персонажа: RMRT: %.4lf, CHA: %.4lf, INT: %.4lf, TOTAL: %.4lf. Цена чармиса:  %.4lf. Итоговая цена: %d \r\n",
                 rem_hirePoints,
                 cha_hirePoints,
                 int_hirePoints,
                 hirePoints,
                 price,
                 finalPrice);
  return std::min(finalPrice, MAX_HIRE_PRICE);
}

int get_reformed_charmice_hp(CHAR_DATA *ch, CHAR_DATA *victim, int spellnum) {
  float r_hp = 0;
  float eff_cha = 0.0;
  float max_cha;

  if (spellnum == SPELL_RESSURECTION || spellnum == SPELL_ANIMATE_DEAD) {
    eff_cha = get_effective_wis(ch, spellnum);
    max_cha = class_stats_limit[ch->get_class()][3];
  } else {
    max_cha = class_stats_limit[ch->get_class()][5];
    eff_cha = get_effective_cha(ch);
  }

  if (spellnum != SPELL_CHARM) {
    eff_cha = MMIN(max_cha, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
  }

  // Интерполяция между значениями для целых значений обаяния
  if (eff_cha < max_cha) {
    r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
        ((1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].dam_to_hit_rate +
            (eff_cha - (int) eff_cha) * cha_app[(int) eff_cha + 1].dam_to_hit_rate);
  } else {
    r_hp = GET_MAX_HIT(victim) + get_damage_per_round(victim) *
        ((1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].dam_to_hit_rate);
  }

  if (PRF_FLAGGED(ch, PRF_TESTER))
    send_to_char(ch, "&Gget_reformed_charmice_hp Расчет чарма r_hp = %f \r\n&n", r_hp);
  return (int) r_hp;
}

void do_findhelpee(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch)
      || (!WAITLESS(ch) && !can_use_feat(ch, EMPLOYER_FEAT))) {
    send_to_char("Вам недоступно это!\r\n", ch);
    return;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    follow_type *k;
    for (k = ch->followers; k; k = k->next) {
      if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER) && AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)) {
        break;
      }
    }

    if (k) {
      act("Вашим наемником является $N.", FALSE, ch, 0, k->follower, TO_CHAR);
    } else {
      act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
    }
    return;
  }

  const auto helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  if (!helpee) {
    send_to_char("Вы не видите никого похожего.\r\n", ch);
    return;
  }

  follow_type *k;
  for (k = ch->followers; k; k = k->next) {
    if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER) && AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)) {
      break;
    }
  }

  if (helpee == ch)
    send_to_char("И как вы это представляете - нанять самого себя?\r\n", ch);
  else if (!IS_NPC(helpee))
    send_to_char("Вы не можете нанять реального игрока!\r\n", ch);
  else if (!NPC_FLAGGED(helpee, NPC_HELPED))
    act("$N не нанимается!", FALSE, ch, 0, helpee, TO_CHAR);
  else if (AFF_FLAGGED(helpee, EAffectFlag::AFF_CHARM) && (!k || (k && helpee != k->follower)))
    act("$N под чьим-то контролем.", FALSE, ch, 0, helpee, TO_CHAR);
  else if (AFF_FLAGGED(helpee, EAffectFlag::AFF_DEAFNESS))
    act("$N не слышит вас.", FALSE, ch, 0, helpee, TO_CHAR);
  else if (IS_HORSE(helpee))
    send_to_char("Это боевой скакун, а не хухры-мухры.\r\n", ch);
  else if (helpee->get_fighting() || GET_POS(helpee) < POS_RESTING)
    act("$M сейчас, похоже, не до вас.", FALSE, ch, 0, helpee, TO_CHAR);
  else if (circle_follow(helpee, ch))
    send_to_char("Следование по кругу запрещено.\r\n", ch);
  else {
    // Вы издеваетесь? Блок else на три экрана, реально?
    // Svent TODO: Вынести проверку на корректность чармиса в отдельную функицю.
    char isbank[MAX_STRING_LENGTH];
    two_arguments(argument, arg, isbank);

    unsigned int times = 0;
    times = atoi(arg);
    if (!*arg || times == 0) {
      const auto cost = calc_hire_price(ch, helpee);
      sprintf(buf, "$n сказал$g вам : \"Один час моих услуг стоит %ld %s\".\r\n",
              cost, desc_count(cost, WHAT_MONEYu));
      act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
      return;
    }

    if (k && helpee != k->follower) {
      act("Вы уже наняли $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
      return;
    }

    if (times > MAX_HIRE_TIME) {
      send_to_char("Пожизненный найм запрещен!\r\n", ch);
      times = MAX_HIRE_TIME;
    }

    auto hire_price = calc_hire_price(ch, helpee);
    long cost = std::abs(times * hire_price);

    if ((!isname(isbank, "банк bank") && cost > ch->get_gold()) ||
        (isname(isbank, "банк bank") && cost > ch->get_bank())) {
      sprintf(buf,
              "$n сказал$g вам : \" Мои услуги за %d %s стоят %ld %s - это тебе не по карману.\"",
              times, desc_count(times, WHAT_HOUR), cost, desc_count(cost, WHAT_MONEYu));
      act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
      return;
    }

    if (helpee->has_master() && helpee->get_master() != ch) {
      if (stop_follower(helpee, SF_MASTERDIE)) {
        return;
      }
    }
    if (MOB_FLAGGED(helpee, MOB_NOGROUP))
      MOB_FLAGS(helpee).unset(MOB_NOGROUP);

    AFFECT_DATA<EApplyLocation> af;
    if (!(k && k->follower == helpee)) {
      ch->add_follower(helpee);
      af.duration = pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
    } else {
      auto aff = k->follower->affected.begin();
      for (; aff != k->follower->affected.end(); ++aff) {
        if ((*aff)->type == SPELL_CHARM) {
          break;
        }
      }

      if (aff != k->follower->affected.end()) {
        af.duration = (*aff)->duration + pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
      }
    }

    affect_from_char(helpee, SPELL_CHARM);

    if (!WAITLESS(ch)) {
      if (isname(isbank, "банк bank")) {
        ch->remove_bank(cost);
        helpee->mob_specials.hire_price = -hire_price;
      } else {
        ch->remove_gold(cost);
        helpee->mob_specials.hire_price = hire_price;
      }
    }

    af.type = SPELL_CHARM;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
    af.battleflag = 0;
    affect_to_char(helpee, af);

    af.type = SPELL_CHARM;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = to_underlying(EAffectFlag::AFF_HELPER);
    af.battleflag = 0;
    affect_to_char(helpee, af);

    sprintf(buf, "$n сказал$g вам : \"Приказывай, %s!\"", IS_FEMALE(ch) ? "хозяйка" : "хозяин");
    act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);

    if (IS_NPC(helpee)) {
      for (auto i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(helpee, i)) {
          if (!remove_otrigger(GET_EQ(helpee, i), helpee))
            continue;

          act("Вы прекратили использовать $o3.", FALSE, helpee, GET_EQ(helpee, i), 0, TO_CHAR);
          act("$n прекратил$g использовать $o3.", TRUE, helpee, GET_EQ(helpee, i), 0, TO_ROOM);
          obj_to_char(unequip_char(helpee, i | 0x40), helpee);
        }
      }

      MOB_FLAGS(helpee).unset(MOB_AGGRESSIVE);
      MOB_FLAGS(helpee).unset(MOB_SPEC);
      PRF_FLAGS(helpee).unset(PRF_PUNCTUAL);
      MOB_FLAGS(helpee).set(MOB_NOTRAIN);
      helpee->set_skill(SKILL_PUNCTUAL, 0);
      ch->updateCharmee(GET_MOB_VNUM(helpee), cost);

      Crash_crashsave(ch);
      ch->save_char();
    }
  }
}

void do_freehelpee(CHAR_DATA *ch, char * /* argument*/, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch)
      || (!WAITLESS(ch) && !can_use_feat(ch, EMPLOYER_FEAT))) {
    send_to_char("Вам недоступно это!\r\n", ch);
    return;
  }

  follow_type *k;
  for (k = ch->followers; k; k = k->next) {
    if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
        && AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)) {
      break;
    }
  }

  if (!k) {
    act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (ch->in_room != IN_ROOM(k->follower)) {
    act("Вам следует встретиться с $N4 для этого.", FALSE, ch, 0, k->follower, TO_CHAR);
    return;
  }

  if (GET_POS(k->follower) < POS_STANDING) {
    act("$N2 сейчас, похоже, не до вас.", FALSE, ch, 0, k->follower, TO_CHAR);
    return;
  }

  if (!IS_IMMORTAL(ch)) {
    for (const auto &aff : k->follower->affected) {
      if (aff->type == SPELL_CHARM) {
        const auto cost = MAX(0, (int) ((aff->duration - 1) / 2) * (int) abs(k->follower->mob_specials.hire_price));
        if (cost > 0) {
          if (k->follower->mob_specials.hire_price < 0) {
            ch->add_bank(cost);
          } else {
            ch->add_gold(cost);
          }
        }

        break;
      }
    }
  }

  act("Вы рассчитали $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
  affect_from_char(k->follower, SPELL_CHARM);
  stop_follower(k->follower, SF_CHARMLOST);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

