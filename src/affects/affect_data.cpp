#include <game_mechanics/glory_const.h>

#include "affect_data.h"
#include "entities/char_player.h"
#include "entities/world_characters.h"
#include "fightsystem/fight_hit.h"
#include "game_classes/classes.h"
#include "cmd/follow.h"
#include "game_mechanics/deathtrap.h"
#include "game_magic/magic.h"
#include "game_skills/poison.h"

bool no_bad_affects(ObjData *obj) {
	static std::list<EWeaponAffectFlag> bad_waffects =
		{
			EWeaponAffectFlag::WAFF_HOLD,
			EWeaponAffectFlag::WAFF_SANCTUARY,
			EWeaponAffectFlag::WAFF_PRISMATIC_AURA,
			EWeaponAffectFlag::WAFF_POISON,
			EWeaponAffectFlag::WAFF_SILENCE,
			EWeaponAffectFlag::WAFF_DEAFNESS,
			EWeaponAffectFlag::WAFF_HAEMORRAGIA,
			EWeaponAffectFlag::WAFF_BLINDNESS,
			EWeaponAffectFlag::WAFF_SLEEP,
			EWeaponAffectFlag::WAFF_HOLY_DARK
		};
	for (const auto wa : bad_waffects) {
		if (OBJ_AFFECT(obj, wa)) {
			return false;
		}
	}
	return true;
}

// Return the effect of a piece of armor in position eq_pos
int apply_ac(CharData *ch, int eq_pos) {
	int factor = 1;

	if (GET_EQ(ch, eq_pos) == nullptr) {
		log("SYSERR: apply_ac(%s,%d) when no equip...", GET_NAME(ch), eq_pos);
		// core_dump();
		return (0);
	}

	if (!ObjSystem::is_armor_type(GET_EQ(ch, eq_pos))) {
		return (0);
	}

	switch (eq_pos) {
		case WEAR_BODY:factor = 3;
			break;        // 30% //
		case WEAR_HEAD:
		case WEAR_LEGS:factor = 2;
			break;        // 20% //
		default:factor = 1;
			break;        // all others 10% //
	}

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		factor *= MOB_AC_MULT;

	return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int apply_armour(CharData *ch, int eq_pos) {
	int factor = 1;
	ObjData *obj = GET_EQ(ch, eq_pos);

	if (!obj) {
		log("SYSERR: apply_armor(%s,%d) when no equip...", GET_NAME(ch), eq_pos);
		// core_dump();
		return (0);
	}

	if (!ObjSystem::is_armor_type(obj))
		return (0);

	switch (eq_pos) {
		case WEAR_BODY:factor = 3;
			break;        // 30% //
		case WEAR_HEAD:
		case WEAR_LEGS:factor = 2;
			break;        // 20% //
		default:factor = 1;
			break;        // all others 10% //
	}

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		factor *= MOB_ARMOUR_MULT;

	// чтобы не плюсовать левую броню на стафе с текущей прочностью выше максимальной
	int cur_dur = MIN(GET_OBJ_MAX(obj), GET_OBJ_CUR(obj));
	return (factor * GET_OBJ_VAL(obj, 1) * cur_dur / MAX(1, GET_OBJ_MAX(obj)));
}

///
/// Сет чару аффектов, которые должны висеть постоянно (через affect_total)
///
// Была ошибка, у нубов реген хитов был всегда 50, хотя с 26 по 30, должен быть 60.
// Теперь аффект регенерация новичка держится 3 реморта, с каждыи ремортом все слабее и слабее
void apply_natural_affects(CharData *ch) {
	if (GET_REAL_REMORT(ch) <= 3 && !IS_IMMORTAL(ch)) {
		affect_modify(ch, APPLY_HITREG, 60 - (GET_REAL_REMORT(ch) * 10), EAffectFlag::AFF_NOOB_REGEN, true);
		affect_modify(ch, APPLY_MOVEREG, 100, EAffectFlag::AFF_NOOB_REGEN, true);
		affect_modify(ch, APPLY_MANAREG, 100 - (GET_REAL_REMORT(ch) * 20), EAffectFlag::AFF_NOOB_REGEN, true);
	}
}

std::array<EAffectFlag, 2> char_saved_aff =
	{
		EAffectFlag::AFF_GROUP,
		EAffectFlag::AFF_HORSE
	};

std::array<EAffectFlag, 3> char_stealth_aff =
	{
		EAffectFlag::AFF_HIDE,
		EAffectFlag::AFF_SNEAK,
		EAffectFlag::AFF_CAMOUFLAGE
	};

template<>
bool Affect<EApplyLocation>::removable() const {
	return !spell_info[type].name
		|| *spell_info[type].name == '!'
		|| type == kSpellSleep
		|| type == kSpellPoison
		|| type == kSpellWeaknes
		|| type == kSpellCurse
		|| type == kSpellFever
		|| type == kSpellSllence
		|| type == kSpellPowerSilence
		|| type == kSpellBlindness
		|| type == kSpellPowerBlindness
		|| type == kSpellHaemorragis
		|| type == kSpellHold
		|| type == kSpellPowerHold
		|| type == kSpellPeaceful
		|| type == kSpellColdWind
		|| type == kSpellDeafness
		|| type == kSpellPowerDeafness
		|| type == kSpellBattle;
}

// This file update pulse affects only
void pulse_affect_update(CharData *ch) {
	bool pulse_aff = false;

	if (ch->get_fighting()) {
		return;
	}

	auto next_affect_i = ch->affected.begin();
	for (auto affect_i = next_affect_i; affect_i != ch->affected.end(); affect_i = next_affect_i) {
		++next_affect_i;
		const auto &affect = *affect_i;

		if (!IS_SET(affect->battleflag, kAfPulsedec)) {
			continue;
		}

		pulse_aff = true;
		if (affect->duration >= 1) {
			if (IS_NPC(ch)) {
				affect->duration--;
			} else {
				affect->duration -= MIN(affect->duration, SECS_PER_PLAYER_AFFECT * kPassesPerSec);
			}
		} else if (affect->duration == -1)    // No action //
		{
			affect->duration = -1;    // GODs only! unlimited //
		} else {
			if ((affect->type > 0) && (affect->type <= kSpellCount)) {
				if (next_affect_i == ch->affected.end()
					|| (*next_affect_i)->type != affect->type
					|| (*next_affect_i)->duration > 0) {
					if (affect->type > 0
						&& affect->type <= kSpellCount) {
						show_spell_off(affect->type, ch);
					}
				}
			}

			ch->affect_remove(affect_i);
		}
	}

	if (pulse_aff) {
		affect_total(ch);
	}
}

void player_affect_update() {
	character_list.foreach_on_copy([](const CharData::shared_ptr &i) {
		// на всякий случай проверка на пурж, в целом пурж чармисов внутри
		// такого цикла сейчас выглядит безопасным, чармисы если и есть, то они
		// добавлялись в чар-лист в начало списка и идут до самого чара
		if (i->purged()
			|| IS_NPC(i)
			|| DeathTrap::tunnel_damage(i.get())) {
			return;
		}

		pulse_affect_update(i.get());

		bool was_purged = false;
		auto next_affect_i = i->affected.begin();
		for (auto affect_i = next_affect_i; affect_i != i->affected.end(); affect_i = next_affect_i) {
			++next_affect_i;
			const auto &affect = *affect_i;

			if (affect->duration >= 1) {
				if (IS_SET(affect->battleflag, kAfSameTime) && !i->get_fighting()) {
					// здесь плеера могут спуржить
					if (processPoisonDamage(i.get(), affect) == -1) {
						was_purged = true;
						break;
					}
				}
				affect->duration--;
			} else if (affect->duration != -1) {
				if ((affect->type > 0) && (affect->type <= kSpellCount)) {
					if (next_affect_i == i->affected.end()
						|| (*next_affect_i)->type != affect->type
						|| (*next_affect_i)->duration > 0) {
						if (affect->type > 0
							&& affect->type <= kSpellCount) {
							//чтобы не выдавалось, "что теперь вы можете сражаться",
							//хотя на самом деле не можете :)
							if (!(affect->type == kSpellMagicBattle
								&& AFF_FLAGGED(i, EAffectFlag::AFF_STOPFIGHT))) {
								if (!(affect->type == kSpellBattle
									&& AFF_FLAGGED(i, EAffectFlag::AFF_MAGICSTOPFIGHT))) {
									show_spell_off(affect->type, i.get());
								}
							}
						}
					}
				}

				i->affect_remove(affect_i);
			}
		}

		if (!was_purged) {
			MemQ_slots(i.get());    // сколько каких слотов занято (с коррекцией)

			affect_total(i.get());
		}
	});
}

// This file update battle affects only
void battle_affect_update(CharData *ch) {
	auto next_affect_i = ch->affected.begin();
	for (auto affect_i = next_affect_i; affect_i != ch->affected.end(); affect_i = next_affect_i) {
		++next_affect_i;
		const auto &affect = *affect_i;

		if (!IS_SET(affect->battleflag, kAfBattledec) && !IS_SET(affect->battleflag, kAfSameTime))
			continue;

		if (IS_NPC(ch) && affect->location == APPLY_POISON)
			continue;

		if (affect->duration >= 1) {
			if (IS_SET(affect->battleflag, kAfSameTime)) {
				if (processPoisonDamage(ch, affect) == -1) // жертва умерла
					return;
				if (ch->purged()) {
					mudlog("Некому обновлять аффект, чар уже спуржен.", BRF, kLvlImplementator, SYSLOG, true);
					return;
				}
				affect->duration--;
			} else {
				if (IS_NPC(ch))
					affect->duration--;
				else
					affect->duration -= MIN(affect->duration, SECS_PER_MUD_HOUR / SECS_PER_PLAYER_AFFECT);
			}
		} else if (affect->duration != -1) {
			if (affect->type > 0 && affect->type <= kSpellCount) {
				if (next_affect_i == ch->affected.end()
					|| (*next_affect_i)->type != affect->type
					|| (*next_affect_i)->duration > 0) {
					if (affect->type > 0 && affect->type <= kSpellCount)
						show_spell_off(affect->type, ch);
				}
			}

			ch->affect_remove(affect_i);
		}
	}

	affect_total(ch);
}

void mobile_affect_update() {
	character_list.foreach_on_copy([](const CharData::shared_ptr &i) {
		int was_charmed = false, charmed_msg = false;
		bool was_purged = false;

		if (IS_NPC(i)) {
			auto next_affect_i = i->affected.begin();
			for (auto affect_i = next_affect_i; affect_i != i->affected.end(); affect_i = next_affect_i) {
				++next_affect_i;
				const auto &affect = *affect_i;

				if (affect->duration >= 1) {
					if (IS_SET(affect->battleflag, kAfSameTime)
						&& (!i->get_fighting() || affect->location == APPLY_POISON)) {
						// здесь плеера могут спуржить
						if (processPoisonDamage(i.get(), affect) == -1) {
							was_purged = true;

							break;
						}
					}

					affect->duration--;
					if (affect->type == kSpellCharm && !charmed_msg && affect->duration <= 1) {
						act("$n начал$g растерянно оглядываться по сторонам.",
							false,
							i.get(),
							nullptr,
							nullptr,
							kToRoom | kToArenaListen);
						charmed_msg = true;
					}
				} else if (affect->duration == -1) {
					affect->duration = -1;    // GODS - unlimited
				} else {
					if (affect->type > 0
						&& affect->type <= kSpellCount) {
						if (next_affect_i == i->affected.end()
							|| (*next_affect_i)->type != affect->type
							|| (*next_affect_i)->duration > 0) {
							if (affect->type > 0
								&& affect->type <= kSpellCount) {
								show_spell_off(affect->type, i.get());
								if (affect->type == kSpellCharm
									|| affect->bitvector == to_underlying(EAffectFlag::AFF_CHARM)) {
									was_charmed = true;
								}
							}
						}
					}

					i->affect_remove(affect_i);
				}
			}
		}

		if (!was_purged) {
			affect_total(i.get());

			decltype(i->timed) timed_skill;
			for (auto timed = i->timed; timed; timed = timed_skill) {
				timed_skill = timed->next;
				if (timed->time >= 1) {
					timed->time--;
				} else {
					timed_from_char(i.get(), timed);
				}
			}

			decltype(i->timed_feat) timed_feat;
			for (auto timed = i->timed_feat; timed; timed = timed_feat) {
				timed_feat = timed->next;
				if (timed->time >= 1) {
					timed->time--;
				} else {
					ExpireTimedFeat(i.get(), timed);
				}
			}

			if (DeathTrap::check_death_trap(i.get())) {
				return;
			}

			if (was_charmed) {
				stop_follower(i.get(), SF_CHARMLOST);
			}
		}
	});
}

// Call affect_remove with every spell of spelltype "skill"
void affect_from_char(CharData *ch, int type) {
	auto next_affect_i = ch->affected.begin();
	for (auto affect_i = next_affect_i; affect_i != ch->affected.end(); affect_i = next_affect_i) {
		++next_affect_i;
		const auto affect = *affect_i;
		if (affect->type == type) {
			ch->affect_remove(affect_i);
		}
	}

	if (IS_NPC(ch) && type == kSpellCharm) {
		EXTRACT_TIMER(ch) = 5;
		ch->mob_specials.hire_price = 0;// added by WorM (Видолюб) 2010.06.04 Сбрасываем цену найма
	}
}

// This updates a character by subtracting everything he is affected by
// restoring original abilities, and then affecting all again
void affect_total(CharData *ch) {
	if (ch->purged()) {
		// we don't care of affects of removed character.
		return;
	}
	bool domination = false;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DOMINATION)) {
		domination = true;
	}
	ObjData *obj;

	FlagData saved;

	// Init struct
	saved.clear();
	ch->clear_add_apply_affects();
	// PC's clear all affects, because recalc one
	{
		saved = ch->char_specials.saved.affected_by;
		if (IS_NPC(ch))
			ch->char_specials.saved.affected_by = mob_proto[GET_MOB_RNUM(ch)].char_specials.saved.affected_by;
		else
			ch->char_specials.saved.affected_by = clear_flags;
		for (const auto &i : char_saved_aff) {
			if (saved.get(i)) {
				AFF_FLAGS(ch).set(i);
			}
		}
	}
	if (domination) {
		ch->set_remort_add(20 - ch->get_remort());
		ch->set_level_add(30 - ch->get_level());
		ch->set_str_add(ch->get_remort_add());
		ch->set_dex_add(ch->get_remort_add());
		ch->set_con_add(ch->get_remort_add()); 
		ch->set_int_add(ch->get_remort_add());
		ch->set_wis_add(ch->get_remort_add());
		ch->set_cha_add(ch->get_remort_add());
		double add_hp_per_level = class_app[GET_CLASS(ch)].base_con
				+ (VPOSI_MOB(ch, EBaseStat::kCon, ch->get_con()) - class_app[GET_CLASS(ch)].base_con)
				* class_app[GET_CLASS(ch)].koef_con / 100.0 + 3;
	 	GET_HIT_ADD(ch) = static_cast<int>(add_hp_per_level * (30 - ch->get_level()));
//		send_to_char(ch, "add per level %f hitadd %d  level %d\r\n", add_hp_per_level, GET_HIT_ADD(ch), ch->get_level());
	}

	// бонусы от морта
	if (GET_REAL_REMORT(ch) >= 20) {
		ch->add_abils.mresist += GET_REAL_REMORT(ch) - 19;
		ch->add_abils.presist += GET_REAL_REMORT(ch) - 19;
	}
	// Restore values for NPC - added by Adept
	if (IS_NPC(ch)) {
		(ch)->add_abils = (&mob_proto[GET_MOB_RNUM(ch)])->add_abils;
	}
	// move object modifiers
	for (int i = 0; i < NUM_WEARS; i++) {
		if ((obj = GET_EQ(ch, i))) {
			if (ObjSystem::is_armor_type(obj)) {
				GET_AC_ADD(ch) -= apply_ac(ch, i);
				GET_ARMOUR(ch) += apply_armour(ch, i);
			}
			// Update weapon applies
			for (int j = 0; j < kMaxObjAffect; j++) {
				affect_modify(ch, GET_EQ(ch, i)->get_affected(j).location, GET_EQ(ch, i)->get_affected(j).modifier, static_cast<EAffectFlag>(0), true);
			}
			// Update weapon bitvectors
			for (const auto &j : weapon_affect) {
				// То же самое, но переформулировал
				if (j.aff_bitvector == 0 || !IS_OBJ_AFF(obj, j.aff_pos)) {
					continue;
				}
				affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(j.aff_bitvector), true);
			}
		}
	}
	ch->obj_bonus().apply_affects(ch);

/*	if (ch->add_abils.absorb > 0) {
		ch->add_abils.mresist += MIN(ch->add_abils.absorb / 2, 25); //поглота
	}
*/
	// move features modifiers - added by Gorrah
	for (int i = 1; i < kMaxFeats; i++) {
		if (can_use_feat(ch, i) && (feat_info[i].type == AFFECT_FTYPE)) {
			for (int j = 0; j < kMaxFeatAffect; j++) {
				affect_modify(ch,
							  feat_info[i].affected[j].location,
							  feat_info[i].affected[j].modifier,
							  static_cast<EAffectFlag>(0),
							  true);
			}
		}
	}

	// IMPREGNABLE_FEAT учитывается дважды: выше начисляем единичку за 0 мортов, а теперь по 1 за каждый морт
	if (can_use_feat(ch, IMPREGNABLE_FEAT)) {
		for (int j = 0; j < kMaxFeatAffect; j++) {
			affect_modify(ch,
						  feat_info[IMPREGNABLE_FEAT].affected[j].location,
						  MIN(9, feat_info[IMPREGNABLE_FEAT].affected[j].modifier * GET_REAL_REMORT(ch)),
						  static_cast<EAffectFlag>(0),
						  true);
		}
	}

	// Обработка изворотливости (с) Числобог
	if (can_use_feat(ch, DODGER_FEAT)) {
		affect_modify(ch, APPLY_SAVING_REFLEX, -(GET_REAL_REMORT(ch) + GetRealLevel(ch)), static_cast<EAffectFlag>(0), true);
		affect_modify(ch, APPLY_SAVING_WILL, -(GET_REAL_REMORT(ch) + GetRealLevel(ch)), static_cast<EAffectFlag>(0), true);
		affect_modify(ch, APPLY_SAVING_STABILITY, -(GET_REAL_REMORT(ch) + GetRealLevel(ch)), static_cast<EAffectFlag>(0), true);
		affect_modify(ch, APPLY_SAVING_CRITICAL, -(GET_REAL_REMORT(ch) + GetRealLevel(ch)), static_cast<EAffectFlag>(0), true);
	}

	// Обработка "выносливости" и "богатырского здоровья
	// Знаю, что кривовато, придумаете, как лучше - делайте
	if (!IS_NPC(ch)) {
		if (can_use_feat(ch, ENDURANCE_FEAT))
			affect_modify(ch, APPLY_MOVE, GetRealLevel(ch) * 2, static_cast<EAffectFlag>(0), true);
		if (can_use_feat(ch, SPLENDID_HEALTH_FEAT))
			affect_modify(ch, APPLY_HIT, GetRealLevel(ch) * 2, static_cast<EAffectFlag>(0), true);
		if (NORENTABLE(ch) == 0 && !domination) // мы не на новой арене и не ПК
			GloryConst::apply_modifiers(ch);
		apply_natural_affects(ch);
	}

	// move affect modifiers
	for (const auto &af : ch->affected) {
		affect_modify(ch, af->location, af->modifier, static_cast<EAffectFlag>(af->bitvector), true);
	}

	// move race and class modifiers
	if (!IS_NPC(ch)) {
		if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < kNumPlayerClasses) {
			for (auto i : *class_app[(int) GET_CLASS(ch)].extra_affects) {
				affect_modify(ch, APPLY_NONE, 0, i.affect, i.set_or_clear);
			}
		}

		// Apply other PC modifiers
		const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
		if (wdex != 0) {
			ch->set_dex_add(ch->get_dex_add() - wdex);
		}
		GET_DR_ADD(ch) += extra_damroll((int) GET_CLASS(ch), GetRealLevel(ch));
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_NOOB_REGEN)) {
			GET_HITREG(ch) += (GetRealLevel(ch) + 4) / 5 * 10;
		}
		if (can_use_feat(ch, DARKREGEN_FEAT)) {
			GET_HITREG(ch) += GET_HITREG(ch) * 0.2;
		}
		if (GET_CON_ADD(ch)) {
			GET_HIT_ADD(ch) += PlayerSystem::con_add_hp(ch);
			int i = GET_MAX_HIT(ch) + GET_HIT_ADD(ch);
			if (i < 1) {
				GET_HIT_ADD(ch) -= (i - 1);
			}
		}

		if (!WAITLESS(ch) && ch->ahorse()) {
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
		}
	}

	// correctize all weapon
	if (!IS_IMMORTAL(ch)) {
		if ((obj = GET_EQ(ch, WEAR_BOTHS)) && !OK_BOTH(ch, obj)) {
			if (!IS_NPC(ch)) {
				act("Вам слишком тяжело держать $o3 в обоих руках!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_BOTH_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_BOTHS, CharEquipFlags()), ch);
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_WIELD)) && !OK_WIELD(ch, obj)) {
			if (!IS_NPC(ch)) {
				act("Вам слишком тяжело держать $o3 в правой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_WIELD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_WIELD, CharEquipFlags()), ch);
			// если пушку можно вооружить в обе руки и эти руки свободны
			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS)
				&& OK_BOTH(ch, obj)
				&& !GET_EQ(ch, WEAR_HOLD)
				&& !GET_EQ(ch, WEAR_LIGHT)
				&& !GET_EQ(ch, WEAR_SHIELD)
				&& !GET_EQ(ch, WEAR_WIELD)
				&& !GET_EQ(ch, WEAR_BOTHS)) {
				equip_char(ch, obj, WEAR_BOTHS, CharEquipFlag::show_msg);
			}
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_HOLD)) && !OK_HELD(ch, obj)) {
			if (!IS_NPC(ch)) {
				act("Вам слишком тяжело держать $o3 в левой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_HOLD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_HOLD, CharEquipFlags()), ch);
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_SHIELD)) && !OK_SHIELD(ch, obj)) {
			if (!IS_NPC(ch)) {
				act("Вам слишком тяжело держать $o3 на левой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_SHIELD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_SHIELD, CharEquipFlags()), ch);
			return;
		}
		if ((obj = GET_EQ(ch, WEAR_QUIVER)) && !GET_EQ(ch, WEAR_BOTHS)) {
			send_to_char("Нету лука, нет и стрел.\r\n", ch);
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_QUIVER, CharEquipFlags()), ch);
			return;
		}
	}

	// calculate DAMAGE value
	GET_DAMAGE(ch) = (str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GetRealDamroll(ch)) * 2;
	if ((obj = GET_EQ(ch, WEAR_BOTHS))
		&& GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
		GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1)))
			>> 1; // правильный расчет среднего у оружия
	} else {
		if ((obj = GET_EQ(ch, WEAR_WIELD))
			&& GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
			GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
		if ((obj = GET_EQ(ch, WEAR_HOLD))
			&& GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
			GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
	}

	{
		// Calculate CASTER value
		int i = 1;
		for (GET_CASTER(ch) = 0; !IS_NPC(ch) && i <= kSpellCount; i++) {
			if (IS_SET(GET_SPELL_TYPE(ch, i), kSpellKnow | kSpellTemp)) {
				GET_CASTER(ch) += (spell_info[i].danger * GET_SPELL_MEM(ch, i));
			}
		}
	}

	{
		// Check steal affects
		for (const auto &i : char_stealth_aff) {
			if (saved.get(i)
				&& !AFF_FLAGS(ch).get(i)) {
				CHECK_AGRO(ch) = true;
			}
		}
	}
	CheckBerserk(ch);
	if (ch->get_fighting() || affected_by_spell(ch, kSpellGlitterDust)) {
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);
		AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
	}
}

void affect_join(CharData *ch,
				 Affect<EApplyLocation> &af,
				 bool add_dur,
				 bool max_dur,
				 bool add_mod,
				 bool max_mod) {
	bool found = false;

	if (af.location) {
		for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i) {
			const auto &affect = *affect_i;
			if (affect->type == af.type
				&& affect->location == af.location) {

				if (add_dur) {
					af.duration += affect->duration;
				} else if (max_dur) {
					af.duration = std::max(af.duration, affect->duration);
				}

				if (add_mod) {
					af.modifier += affect->modifier;
				} else if (max_mod) {
					af.modifier = std::max(af.modifier, affect->modifier);
				}

				ch->affect_remove(affect_i);
				affect_to_char(ch, af);

				found = true;
				break;
			}
		}
	}

	if (!found) {
		affect_to_char(ch, af);
	}
}

/* Insert an affect_type in a char_data structure
   Automatically sets appropriate bits and apply's */
void affect_to_char(CharData *ch, const Affect<EApplyLocation> &af) {
	long was_lgt = AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO;
	long was_hlgt = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO;
	long was_hdrk = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;

	Affect<EApplyLocation>::shared_ptr affected_alloc(new Affect<EApplyLocation>(af));

	ch->affected.push_front(affected_alloc);

	AFF_FLAGS(ch) += af.aff;
	if (af.bitvector)
		affect_modify(ch, af.location, af.modifier, static_cast<EAffectFlag>(af.bitvector), true);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
	affect_total(ch);
	check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}

void affect_modify(CharData *ch, byte loc, int mod, const EAffectFlag bitv, bool add) {
	if (add) {
		AFF_FLAGS(ch).set(bitv);
	} else {
		AFF_FLAGS(ch).unset(bitv);
		mod = -mod;
	}
	switch (loc) {
		case APPLY_NONE:break;
		case APPLY_STR:ch->set_str_add(ch->get_str_add() + mod);
			break;
		case APPLY_DEX:ch->set_dex_add(ch->get_dex_add() + mod);
			break;
		case APPLY_INT:ch->set_int_add(ch->get_int_add() + mod);
			break;
		case APPLY_WIS:ch->set_wis_add(ch->get_wis_add() + mod);
			break;
		case APPLY_CON:ch->set_con_add(ch->get_con_add() + mod);
			break;
		case APPLY_CHA:ch->set_cha_add(ch->get_cha_add() + mod);
			break;
		case APPLY_CLASS:
		case APPLY_LEVEL:break;
		case APPLY_AGE:GET_AGE_ADD(ch) += mod;
			break;
		case APPLY_CHAR_WEIGHT:GET_WEIGHT_ADD(ch) += mod;
			break;
		case APPLY_CHAR_HEIGHT:GET_HEIGHT_ADD(ch) += mod;
			break;
		case APPLY_MANAREG:GET_MANAREG(ch) += mod;
			break;
		case APPLY_HIT:GET_HIT_ADD(ch) += mod;
			break;
		case APPLY_MOVE:GET_MOVE_ADD(ch) += mod;
			break;
		case APPLY_GOLD:
		case APPLY_EXP:break;
		case APPLY_AC:GET_AC_ADD(ch) += mod;
			break;
		case APPLY_HITROLL:GET_HR_ADD(ch) += mod;
			break;
		case APPLY_DAMROLL:GET_DR_ADD(ch) += mod;
			break;
		case APPLY_RESIST_FIRE:GET_RESIST(ch, FIRE_RESISTANCE) += mod;
			break;
		case APPLY_RESIST_AIR:GET_RESIST(ch, AIR_RESISTANCE) += mod;
			break;
		case APPLY_RESIST_DARK:GET_RESIST(ch, DARK_RESISTANCE) += mod;
			break;
		case APPLY_SAVING_WILL:SET_SAVE(ch, ESaving::kWill, GET_SAVE(ch, ESaving::kWill) +  mod);
			break;
		case APPLY_SAVING_CRITICAL:SET_SAVE(ch, ESaving::kCritical, GET_SAVE(ch, ESaving::kCritical) +  mod);
			break;
		case APPLY_SAVING_STABILITY:SET_SAVE(ch, ESaving::kStability, GET_SAVE(ch, ESaving::kStability) +  mod);
			break;
		case APPLY_SAVING_REFLEX:SET_SAVE(ch, ESaving::kReflex, GET_SAVE(ch, ESaving::kReflex) +  mod);
			break;
		case APPLY_HITREG:GET_HITREG(ch) += mod;
			break;
		case APPLY_MOVEREG:GET_MOVEREG(ch) += mod;
			break;
		case APPLY_C1:
		case APPLY_C2:
		case APPLY_C3:
		case APPLY_C4:
		case APPLY_C5:
		case APPLY_C6:
		case APPLY_C7:
		case APPLY_C8:
		case APPLY_C9:ch->add_obj_slot(loc - APPLY_C1, mod);
			break;
		case APPLY_SIZE:GET_SIZE_ADD(ch) += mod;
			break;
		case APPLY_ARMOUR:GET_ARMOUR(ch) += mod;
			break;
		case APPLY_POISON:GET_POISON(ch) += mod;
			break;
		case APPLY_CAST_SUCCESS:GET_CAST_SUCCESS(ch) += mod;
			break;
		case APPLY_MORALE:GET_MORALE(ch) += mod;
			break;
		case APPLY_INITIATIVE:GET_INITIATIVE(ch) += mod;
			break;
		case APPLY_RELIGION:
			if (add)
				GET_PRAY(ch) |= mod;
			else
				GET_PRAY(ch) &= mod;
			break;
		case APPLY_ABSORBE:GET_ABSORBE(ch) += mod;
			break;
		case APPLY_LIKES:GET_LIKES(ch) += mod;
			break;
		case APPLY_RESIST_WATER:GET_RESIST(ch, WATER_RESISTANCE) += mod;
			break;
		case APPLY_RESIST_EARTH:GET_RESIST(ch, EARTH_RESISTANCE) += mod;
			break;
		case APPLY_RESIST_VITALITY:GET_RESIST(ch, VITALITY_RESISTANCE) += mod;
			break;
		case APPLY_RESIST_MIND:GET_RESIST(ch, MIND_RESISTANCE) += mod;
			break;
		case APPLY_RESIST_IMMUNITY:GET_RESIST(ch, IMMUNITY_RESISTANCE) += mod;
			break;
		case APPLY_AR:GET_AR(ch) += mod;
			break;
		case APPLY_MR:GET_MR(ch) += mod;
			break;
		case APPLY_ACONITUM_POISON:
		case APPLY_SCOPOLIA_POISON:
		case APPLY_BELENA_POISON:
		case APPLY_DATURA_POISON:GET_POISON(ch) += mod;
			break;
		case APPLY_PR:GET_PR(ch) += mod; //скиллрезист
			break;
		case APPLY_PERCENT_PHYSDAM:ch->add_abils.percent_physdam_add += mod;
			break;
		case APPLY_PERCENT_MAGDAM:ch->add_abils.percent_magdam_add += mod;
			break;
		case APPLY_PERCENT_EXP:ch->add_abils.percent_exp_add += mod;
			break;
		case APPLY_SPELL_BLINK:ch->add_abils.percent_spell_blink += mod;
			break;
		case APPLY_BONUS_SKILLS: {
			ch->set_skill_bonus(ch->get_skill_bonus() + mod);
		}
		default:break;
	}            // switch
}

// * Снятие аффектов с чара при смерти/уходе в дт.
void reset_affects(CharData *ch) {
	auto naf = ch->affected.begin();

	for (auto af = naf; af != ch->affected.end(); af = naf) {
		++naf;
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) 
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_DOMINATION))
			continue;
		const auto &affect = *af;
		if (!IS_SET(affect->battleflag, kAfDeadkeep)) {
			ch->affect_remove(af);
		}
	}

	GET_COND(ch, DRUNK) = 0; // Чтобы не шатало без аффекта "под мухой"
	affect_total(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
