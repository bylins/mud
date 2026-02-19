#include "affect_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/poison.h"
#include "gameplay/skills/death_rage.h"
#include "engine/db/global_objects.h"
#include "engine/core/handler.h"
#include "utils/utils_time.h"
#include "gameplay/core/genchar.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/glory_const.h"
#include "engine/ui/cmd/do_equip.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/fight/fight.h"
#include "utils/backtrace.h"

std::unordered_set<CharData *> affected_mobs;

bool no_bad_affects(ObjData *obj) {
	static std::list<EWeaponAffect> bad_waffects =
		{
			EWeaponAffect::kHold,
			EWeaponAffect::kSanctuary,
			EWeaponAffect::kPrismaticAura,
			EWeaponAffect::kPoison,
			EWeaponAffect::kSilence,
			EWeaponAffect::kDeafness,
			EWeaponAffect::kHaemorrhage,
			EWeaponAffect::kBlindness,
			EWeaponAffect::kSleep,
			EWeaponAffect::kHolyDark
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
		case EEquipPos::kBody:factor = 3;
			break;        // 30% //
		case EEquipPos::kHead:
		case EEquipPos::kLegs:factor = 2;
			break;        // 20% //
		default:factor = 1;
			break;        // all others 10% //
	}

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		factor *= kMobAcMult;

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
		case EEquipPos::kBody: factor = 3;
			break;        // 30% //
		case EEquipPos::kHead:
		case EEquipPos::kLegs: factor = 2;
			break;        // 20% //
		default: factor = 1;
			break;        // all others 10% //
	}

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		factor *= kMobArmourMult;

	// чтобы не плюсовать левую броню на стафе с текущей прочностью выше максимальной
	int cur_dur = MIN(obj->get_maximum_durability(), obj->get_current_durability());
	return (factor * GET_OBJ_VAL(obj, 1) * cur_dur / MAX(1, obj->get_maximum_durability()));
}

///
/// Сет чару аффектов, которые должны висеть постоянно (через affect_total)
///
// Была ошибка, у нубов реген хитов был всегда 50, хотя с 26 по 30, должен быть 60.
// Теперь аффект регенерация новичка держится 3 реморта, с каждыи ремортом все слабее и слабее
void apply_natural_affects(CharData *ch) {
	if (GetRealRemort(ch) <= 3 && !IS_IMMORTAL(ch)) {
		affect_modify(ch, EApply::kHpRegen, 60 - (GetRealRemort(ch) * 10), EAffect::kNoobRegen, true);
		affect_modify(ch, EApply::kMoveRegen, 100, EAffect::kNoobRegen, true);
		affect_modify(ch, EApply::kManaRegen, 100 - (GetRealRemort(ch) * 20), EAffect::kNoobRegen, true);
	}
}

std::array<EAffect, 2> char_saved_aff =
	{
		EAffect::kGroup,
		EAffect::kHorse
	};

std::array<EAffect, 3> char_stealth_aff =
	{
		EAffect::kHide,
		EAffect::kSneak,
		EAffect::kDisguise
	};

template<>
bool Affect<EApply>::removable() const {
	return type == ESpell::kSleep
		|| type == ESpell::kPoison
		|| type == ESpell::kWeaknes
		|| type == ESpell::kCurse
		|| type == ESpell::kFever
		|| type == ESpell::kSilence
		|| type == ESpell::kPowerSilence
		|| type == ESpell::kBlindness
		|| type == ESpell::kPowerBlindness
		|| type == ESpell::kHaemorrhage
		|| type == ESpell::kHold
		|| type == ESpell::kPowerHold
		|| type == ESpell::kPeaceful
		|| type == ESpell::kColdWind
		|| type == ESpell::kDeafness
		|| type == ESpell::kPowerDeafness
		|| type == ESpell::kBattle;
}
// 
// для мобов раз в 10 пульсов
void UpdateAffectOnPulse(CharData *ch, int count) {
	bool pulse_aff = false;

	if (ch->GetEnemy()) {
		return;
	}

	if (ch->affected.empty()) {
		return;
	}

	auto affect_i = ch->affected.begin();
	while (affect_i != ch->affected.end()) {
		const auto &affect = *affect_i;

		if (!IS_SET(affect->battleflag, kAfPulsedec)) {
			++affect_i;
			continue;
		}
		pulse_aff = true;

		if (affect->duration == 0) { //-1 вечный таймер по задумке
			affect_i = ch->AffectRemove(affect_i);
		} else {
			if (affect->duration > 0) {
				affect->duration -= count;
				affect->duration = std::max(0, affect->duration);
			}
			++affect_i;
		}
	}
	if (pulse_aff) {
		affect_total(ch);
	}
}
// игроки раз в 2 секунды
void player_affect_update() {
	utils::CExecutionTimer timer;
	int count = 0;
//	character_list.foreach_on_copy([&count](const CharData::shared_ptr &i) {
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state != EConState::kPlaying)
			continue;
		const auto i = d->get_character();
					
		// на всякий случай проверка на пурж, в целом пурж чармисов внутри
		// такого цикла сейчас выглядит безопасным, чармисы если и есть, то они
		// добавлялись в чар-лист в начало списка и идут до самого чара
		if (i->purged() || deathtrap::tunnel_damage(i.get())) {
			return;
		}
		count++;
		bool was_purged = false;
		bool set_abstinent = false;
		auto affect_i = i->affected.begin();

		while (affect_i != i->affected.end()) {
			const auto &affect = *affect_i;

			// нечего тикать аффектам вне раунда боя
			if (IS_SET(affect->battleflag, kAfBattledec) && i->GetEnemy()) {
				++affect_i;
				continue;
			}
			if (affect->duration == 0) {
				if (affect->type >= ESpell::kFirst && affect->type <= ESpell::kLast) {
					auto next_affect_i = affect_i;

					++next_affect_i;
					if (next_affect_i == i->affected.end()	//костыль на спадение 1 закла накладывающего несколько аффектов
							|| (*next_affect_i)->type != affect->type
							|| (*next_affect_i)->duration > 0) {
						//чтобы не выдавалось, "что теперь вы можете сражаться",
						//хотя на самом деле не можете :)
						if (!(affect->type == ESpell::kMagicBattle
								&& AFF_FLAGGED(i, EAffect::kStopFight))) {
							if (!(affect->type == ESpell::kBattle
									&& AFF_FLAGGED(i, EAffect::kMagicStopFight))) {
								ShowAffExpiredMsg(affect->type, i.get());
							}
						}
					}
				}
				if (affect->type == ESpell::kDrunked) {
					set_abstinent = true;
				}
				affect_i = i->AffectRemove(affect_i);
			} else {
				if (affect->duration > 0) {
					if (IS_SET(affect->battleflag, kAfSameTime) && !i->GetEnemy()) {
						// здесь плеера могут спуржить
						if (ProcessPoisonDmg(i.get(), affect) == -1) {
							was_purged = true;
							break;
						}
					}
//					sprintf(buf, "ЧАР: Спелл %s висит на %s длительносить %d\r\n", MUD::Spell(affect->type).GetCName(), GET_PAD(i, 5), affect->duration);
//					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					if (ROOM_FLAGGED(i->in_room, ERoomFlag::kDominationArena)) {
						for (int count = MAX_FIRSTAID_REMOVE - 1; count >= 0; count--) {
							if (affect->type == GetRemovableSpellId(count)) {
								affect->duration -= 15;
								break;
							}
						}
						if (IS_SET(affect->battleflag, kAfPulsedec))
							affect->duration -= MIN(affect->duration, kSecsPerPlayerAffect * kPassesPerSec);
						else
							affect->duration--;
					} else {
						if (IS_SET(affect->battleflag, kAfPulsedec))
							affect->duration -= MIN(affect->duration, kSecsPerPlayerAffect * kPassesPerSec);
						else
							affect->duration--;
					}
					affect->duration = std::max(0, affect->duration);
				}
				++affect_i;
			}
		}
		if (set_abstinent) {
			i->set_abstinent();
		}
		if (!was_purged) {
			MemQ_slots(i.get());    // сколько каких слотов занято (с коррекцией)
			affect_total(i.get());
		}
	}
	log("player affect update: timer %f, num players %d", timer.delta().count(), count);
}

// This file update battle affects only
void battle_affect_update(CharData *ch) {
	if (ch->purged()) {
		char tmpbuf[256];
		sprintf(tmpbuf,"WARNING: battle_affect_update ch purged. Name %s vnum %d", GET_NAME(ch), GET_MOB_VNUM(ch));
		mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
		return;
	}
	if (ch->affected.empty()) {
		return;
	}
	auto affect_i = ch->affected.begin();
	while (affect_i != ch->affected.end()) {
		const auto &affect = *affect_i;

		if (!IS_SET((*affect_i)->battleflag, kAfBattledec) && !IS_SET((*affect_i)->battleflag, kAfSameTime)) {
			++affect_i;
			continue;
		}
		if (ch->IsNpc() && (*affect_i)->location == EApply::kPoison) {
			++affect_i;
			continue;
		}
		if (affect->duration == 0) {
			if (affect->type >= ESpell::kFirst && affect->type <= ESpell::kLast) {
				auto next_affect_i = affect_i;

				++next_affect_i;
				if (next_affect_i == ch->affected.end()
						|| (*next_affect_i)->type != (*affect_i)->type
						|| (*next_affect_i)->duration > 0) {
					ShowAffExpiredMsg(affect->type, ch);
				}
			}
			affect_i = ch->AffectRemove(affect_i);
		} else {
			if (affect->duration > 0) {
				if (IS_SET(affect->battleflag, kAfSameTime)) {
					if (ProcessPoisonDmg(ch, affect) == -1) {// жертва умерла
						return;
					}
					if (ch->purged()) {
						mudlog("Некому обновлять аффект, чар уже спуржен.",   BRF, kLvlImplementator, SYSLOG, true);
						return;
					}
				}
				if (ch->IsNpc())
					--affect->duration;
				else {
					affect->duration -= std::min(affect->duration, kSecsPerMudHour / kSecsPerPlayerAffect);
				}
				affect->duration = std::max(0, affect->duration);
			}
			++affect_i;
		}
	}
	affect_total(ch);
}

// раз в минуту
void mobile_affect_update() {
	utils::CExecutionTimer timer;
	int count = 0;

	for (auto it = affected_mobs.begin(); it != affected_mobs.end();) {
		const auto &ch = *it;
		int was_charmed = false, charmed_msg = false;
		bool was_purged = false;
		count++;
//		if (!ch->in_used_zone()) {
//			return;
//		}
		auto affect_i = ch->affected.begin();

		if (ch->affected.empty()) {
			mudlog(fmt::format("ERROR!!! Проверка счетчика аффектов у очищенного моба {} #{}", ch->get_name(), GET_MOB_VNUM(ch)));
			it = affected_mobs.erase(it);
			continue;
		}
		while (affect_i != ch->affected.end()) {
			const auto &affect = *affect_i;
			
			if (affect->duration == 0) {
				if (affect->type >= ESpell::kFirst && affect->type <= ESpell::kLast) {
					if (affect->type == ESpell::kCharm || affect->bitvector == to_underlying(EAffect::kCharmed)) {
						was_charmed = true;
					}
					auto next_affect_i = affect_i;

					++next_affect_i;
					if (next_affect_i == ch->affected.end()
							|| (*next_affect_i)->type != affect->type
							|| (*next_affect_i)->duration > 0) {
						ShowAffExpiredMsg(affect->type, ch);
					}
				}
				affect_i = ch->AffectRemove(affect_i);
			} else {
				if (affect->duration > 0) {
					if (IS_SET(affect->battleflag, kAfSameTime)
						&& (!ch->GetEnemy() || affect->location == EApply::kPoison)) {
						// здесь плеера могут спуржить
						if (ProcessPoisonDmg(ch, affect) == -1) {
							was_purged = true;
							break;
						}
					}
					affect->duration--;
					if (affect->type == ESpell::kCharm && !charmed_msg && affect->duration <= 1) {
						act("$n начал$g растерянно оглядываться по сторонам.",
								false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
					charmed_msg = true;
					}
				}
				++affect_i;
			}
		}
		if (!was_purged) {
			affect_total(ch);
// обработка таймеров скилов фитов игрока
			decltype(ch->timed) timed_skill;
			for (auto timed = ch->timed; timed; timed = timed_skill) {
				timed_skill = timed->next;
				if (timed->time >= 1) {
					timed->time--;
				} else {
					ExpireTimedSkill(ch, timed);
				}
			}
			decltype(ch->timed_feat) timed_feat;
			for (auto timed = ch->timed_feat; timed; timed = timed_feat) {
				timed_feat = timed->next;
				if (timed->time >= 1) {
					timed->time--;
				} else {
					ExpireTimedFeat(ch, timed);
				}
			}
			if (deathtrap::check_death_trap(ch)) {
				return;
			}
			if (was_charmed) {
				stop_follower(ch, kSfCharmlost);
			}
		}
		if (ch->affected.empty()) {
			it = affected_mobs.erase(it);
		} else
			++it;

	}
	log("mobile affect update: timer %f, num mobs %d", timer.delta().count(), count);
}

void RemoveAffectFromCharAndRecalculate(CharData *ch, ESpell spell_id) {
	RemoveAffectFromChar(ch, spell_id);
	affect_total(ch);
}

// Call affect_remove with every spell of spelltype "skill"
void RemoveAffectFromChar(CharData *ch, ESpell spell_id) {
	std::list<std::shared_ptr<Affect<EApply>>>::iterator it  = ch->affected.begin();

	while (it != ch->affected.end()) {
		Affect<EApply>::shared_ptr affect = *it;
		if (affect->type == spell_id) {
			it = ch->AffectRemove(it);
		}
		else {
			++it;
		}
	}
	if (ch->IsNpc()) {
		if (spell_id == ESpell::kCharm) {
			ch->extract_timer = 5;
			ch->mob_specials.hire_price = 0;// added by WorM (Видолюб) 2010.06.04 Сбрасываем цену найма
		}
	}
}

std::pair<EApply, int>  GetApplyByWeaponAffect(EWeaponAffect element, CharData *ch) {
	int value;
	if (ch) //чтоб не было варнинга, ch передаю на будущее
		value = 2;
	switch (element) {
		case EWeaponAffect::kFireAura:
			return std::pair<EApply, int>(EApply::kResistFire, value);
			break;
		case EWeaponAffect::kAirAura:
			return std::pair<EApply, int>(EApply::kResistAir, value);
			break;
		case EWeaponAffect::kIceAura:
			return std::pair<EApply, int>(EApply::kResistWater, value);
			break;
		case EWeaponAffect::kEarthAura:
			return std::pair<EApply, int>(EApply::kResistEarth, value);
			break;
		case EWeaponAffect::kProtectFromDark:
			return std::pair<EApply, int>(EApply::kResistDark, value);
			break;
		case EWeaponAffect::kProtectFromMind:
			return std::pair<EApply, int>(EApply::kResistMind, value);
			break;
		default: 
			return std::pair<EApply, int>(EApply::kNone, 0);
			break;
	}
}

// This updates a character by subtracting everything he is affected by
// restoring original abilities, and then affecting all again
void affect_total(CharData *ch) {
	if (ch->purged()) {
		return;
	}
	bool domination = false;

	if (!ch->IsNpc() && ch->in_room != kNowhere && ch->in_room >= 0
			&& static_cast<size_t>(ch->in_room) < world.size()
			&& ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
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
		if (ch->IsNpc()) {
			ch->char_specials.saved.affected_by = mob_proto[GET_MOB_RNUM(ch)].char_specials.saved.affected_by;
		} else {
			ch->char_specials.saved.affected_by = clear_flags;
		}
		for (const auto &i : char_saved_aff) {
			if (saved.get(i)) {
				AFF_FLAGS(ch).set(i);
			}
		}
	}
	if (domination) {
		ch->set_remort_add(24 - ch->get_remort());
		ch->set_level_add(30 - ch->GetLevel());
		ch->set_str_add(ch->get_start_stat(G_STR) + 24 - ch->get_str());
		ch->set_dex_add(ch->get_start_stat(G_DEX) + 24 - ch->get_dex());
		ch->set_con_add(ch->get_start_stat(G_CON) + 24 - ch->get_con());
		ch->set_int_add(ch->get_start_stat(G_INT) + 24 - ch->get_int());
		ch->set_wis_add(ch->get_start_stat(G_WIS) + 24 - ch->get_wis());
		ch->set_cha_add(ch->get_start_stat(G_CHA) + 24 - ch->get_cha());
		double add_hp_per_level = MUD::Class(ch->GetClass()).applies.base_con
				+ (ClampBaseStat(ch, EBaseStat::kCon, GetRealCon(ch)) - MUD::Class(ch->GetClass()).applies.base_con)
				* MUD::Class(ch->GetClass()).applies.koef_con / 100.0 + 3;
		double hiton30lvl = 10 + add_hp_per_level * 30;
		ch->set_hit_add(hiton30lvl - ch->get_max_hit());
//		SendMsgToChar(ch, "max_hit: %d, add per level: %f, hitadd: %d, start_con: %d, level: %d\r\n", 
//			GET_REAL_MAX_HIT(ch), add_hp_per_level, GET_HIT_ADD(ch), ch->get_start_stat(G_CON), GetRealLevel(ch));
	}
	if (ch->IsNpc()) {
		(ch)->add_abils = (&mob_proto[GET_MOB_RNUM(ch)])->add_abils;
	}
	// move object modifiers
	for (int i = EEquipPos::kFirstEquipPos; i < EEquipPos::kNumEquipPos; i++) {
		if ((obj = GET_EQ(ch, i))) {
			if (ObjSystem::is_armor_type(obj)) {
				GET_AC_ADD(ch) -= apply_ac(ch, i);
				GET_ARMOUR(ch) += apply_armour(ch, i);
			}
			// Update weapon applies
			for (int j = 0; j < kMaxObjAffect; j++) {
				affect_modify(ch, GET_EQ(ch, i)->get_affected(j).location,
							  GET_EQ(ch, i)->get_affected(j).modifier, static_cast<EAffect>(0), true);
			}
			// Update weapon bitvectors
			for (const auto &j : weapon_affect) {
				// То же самое, но переформулировал
				if (j.aff_bitvector == 0 || !obj->GetEWeaponAffect(j.aff_pos)) {
					continue;
				}
				affect_modify(ch, GetApplyByWeaponAffect(j.aff_pos, ch).first, GetApplyByWeaponAffect(j.aff_pos, ch).second, static_cast<EAffect>(j.aff_bitvector), true);
			}
		}
	}
	ch->obj_bonus().apply_affects(ch);

	for (const auto &feat : MUD::Feats()) {
		if (CanUseFeat(ch, feat.GetId())) {
			feat.effects.ImposeApplies(ch);
			feat.effects.ImposeSkillsMods(ch);
		}
	}

	if (!ch->IsNpc()) {
		if (!domination) // мы не на новой арене и не ПК
			GloryConst::apply_modifiers(ch);
		apply_natural_affects(ch);
	}

	// move affect modifiers
	for (const auto &af : ch->affected) {
		affect_modify(ch, af->location, af->modifier, static_cast<EAffect>(af->bitvector), true);
	}

	// move race and class modifiers
	if (!ch->IsNpc()) {
		const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
		if (wdex != 0) {
			ch->set_dex_add(ch->get_dex_add() - wdex);
		}
			GET_DR_ADD(ch) += GetExtraDamroll(ch->GetClass(), GetRealLevel(ch));
		if (!AFF_FLAGGED(ch, EAffect::kNoobRegen)) {
			ch->set_hitreg(ch->get_hitreg() + (GetRealLevel(ch) + 4) / 5 * 10);
		}
		if (CanUseFeat(ch, EFeat::kRegenOfDarkness)) {
			ch->set_hitreg(ch->get_hitreg() + ch->get_hitreg() * 0.2);
		}
		if (GET_CON_ADD(ch)) {
			ch->set_hit_add(ch->get_hit_add() + PlayerSystem::con_add_hp(ch));
			int i = ch->get_real_max_hit();
			if (i < 1) {
				ch->set_hit_add(ch->get_hit_add() - (i - 1));
			}
		}
		if (!IS_IMMORTAL(ch) && ch->IsOnHorse()) {
			AFF_FLAGS(ch).unset(EAffect::kHide);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
			AFF_FLAGS(ch).unset(EAffect::kDisguise);
			AFF_FLAGS(ch).unset(EAffect::kInvisible);
		}
	}

	// correctize all weapon
	if (!IS_IMMORTAL(ch)) {
		if ((obj = GET_EQ(ch, EEquipPos::kBoths)) && !CanBeTakenInBothHands(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 в обоих руках!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_BOTH_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlags()), ch);
			return;
		}
		if ((obj = GET_EQ(ch, EEquipPos::kWield)) && !CanBeTakenInMajorHand(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 в правой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_WIELD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kWield, CharEquipFlags()), ch);
			// если пушку можно вооружить в обе руки и эти руки свободны
			if (CAN_WEAR(obj, EWearFlag::kBoth)
				&& CanBeTakenInBothHands(ch, obj)
				&& !GET_EQ(ch, EEquipPos::kHold)
				&& !GET_EQ(ch, EEquipPos::kLight)
				&& !GET_EQ(ch, EEquipPos::kShield)
				&& !GET_EQ(ch, EEquipPos::kWield)
				&& !GET_EQ(ch, EEquipPos::kBoths)) {
				EquipObj(ch, obj, EEquipPos::kBoths, CharEquipFlag::show_msg);
			}
			return;
		}
		if ((obj = GET_EQ(ch, EEquipPos::kHold)) && !CanBeTakenInMinorHand(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 в левой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_HOLD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlags()), ch);
			return;
		}
		if ((obj = GET_EQ(ch, EEquipPos::kShield)) && !CanBeWearedAsShield(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 на левой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_SHIELD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kShield, CharEquipFlags()), ch);
			return;
		}
		if (!ch->IsNpc() && (obj = GET_EQ(ch, EEquipPos::kQuiver)) && !GET_EQ(ch, EEquipPos::kBoths)) {
			SendMsgToChar("Нет лука, нет и стрел.\r\n", ch);
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kQuiver, CharEquipFlags()), ch);
			return;
		}
	}

	// calculate DAMAGE value
	// походу для выбора цели переключения в бою
	ch->damage_level = (str_bonus(GetRealStr(ch), STR_TO_DAM) + GetRealDamroll(ch)) * 2;
	if ((obj = GET_EQ(ch, EEquipPos::kBoths))
		&& obj->get_type() == EObjType::kWeapon) {
		ch->damage_level += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1)))
			>> 1; // правильный расчет среднего у оружия
	} else {
		if ((obj = GET_EQ(ch, EEquipPos::kWield))
			&& obj->get_type() == EObjType::kWeapon) {
			ch->damage_level += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
		if ((obj = GET_EQ(ch, EEquipPos::kHold))
			&& obj->get_type() == EObjType::kWeapon) {
			ch->damage_level += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
	}

	// бонусы от морта
	if (GetRealRemort(ch) >= 20) {
		ch->add_abils.mresist += GetRealRemort(ch) - 19;
		ch->add_abils.presist += GetRealRemort(ch) - 19;
	}

	//капы
	ch->add_abils.mresist = std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, ch->add_abils.mresist);
	ch->add_abils.presist = std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, ch->add_abils.presist);

	{
		// Calculate CASTER value
		auto spell_id{ESpell::kFirst};
		for (ch->caster_level = 0; !ch->IsNpc() && spell_id <= ESpell::kLast; ++spell_id) {
			if (IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
				ch->caster_level += MUD::Spell(spell_id).GetDanger()*GET_SPELL_MEM(ch, spell_id);
			}
		}
	}

	{
		// Check steal affects
		for (const auto &i : char_stealth_aff) {
			if (saved.get(i)
				&& !AFF_FLAGS(ch).get(i)) {
				ch->check_aggressive = true;
			}
		}
	}
	if (!ch->IsNpc())
		CheckDeathRage(ch);
	if (ch->GetEnemy() || IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		AFF_FLAGS(ch).unset(EAffect::kHide);
		AFF_FLAGS(ch).unset(EAffect::kSneak);
		AFF_FLAGS(ch).unset(EAffect::kDisguise);
		AFF_FLAGS(ch).unset(EAffect::kInvisible);
	}
	update_pos(ch);
	int was_lgt = AFF_FLAGGED(ch, EAffect::kSingleLight) ? kLightYes : kLightNo;
	long was_hlgt = AFF_FLAGGED(ch, EAffect::kHolyLight) ? kLightYes : kLightNo;
	long was_hdrk = AFF_FLAGGED(ch, EAffect::kHolyDark) ? kLightYes : kLightNo;

	CheckLight(ch, kLightUndef, was_lgt, was_hlgt, was_hdrk, 1);
}

void ImposeAffect(CharData *ch, const Affect<EApply> &af) {
	for (const auto &affect : ch->affected) {
		const bool same_affect = (af.location == EApply::kNone) && (affect->bitvector == af.bitvector);
		const bool same_type = (af.location != EApply::kNone) && (affect->type == af.type) && (affect->location == af.location);
		if (same_affect || same_type) {
			if (affect->modifier < af.modifier) {
				affect->modifier = af.modifier;
			}
			if (affect->duration < af.duration) {
				affect->duration = af.duration;
			}
			affect_total(ch);
			return;
		}
	}
	affect_to_char(ch, af);
}

void ImposeAffect(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod) {
	if (af.location) {
		auto it = ch->affected.begin();

		while (it != ch->affected.end()) {
			const auto &affect = *it;

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
				ch->AffectRemove(it);
				affect_to_char(ch, af);
				return;
			} else {
				++it;
			}
		}
	}
	affect_to_char(ch, af);
}

/* Insert an affect_type in a char_data structure
   Automatically sets appropriate bits and apply's */
void affect_to_char(CharData *ch, const Affect<EApply> &af) {
	Affect<EApply>::shared_ptr affected_alloc(new Affect<EApply>(af));

	if (ch->IsNpc()) {
		affected_mobs.insert(ch);
	}
	ch->affected.push_front(affected_alloc);

	AFF_FLAGS(ch) += af.aff;
	if (af.bitvector)
		affect_modify(ch, af.location, af.modifier, static_cast<EAffect>(af.bitvector), true);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
	affect_total(ch);
}

void affect_modify(CharData *ch, EApply loc, int mod, const EAffect bitv, bool add) {
	if (add) {
		AFF_FLAGS(ch).set(bitv);
	} else {
		AFF_FLAGS(ch).unset(bitv);
		mod = -mod;
	}
	switch (loc) {
		case EApply::kNone: break;
		case EApply::kStr: ch->set_str_add(ch->get_str_add() + mod);
			break;
		case EApply::kDex: ch->set_dex_add(ch->get_dex_add() + mod);
			break;
		case EApply::kInt: ch->set_int_add(ch->get_int_add() + mod);
			break;
		case EApply::kWis: ch->set_wis_add(ch->get_wis_add() + mod);
			break;
		case EApply::kCon: ch->set_con_add(ch->get_con_add() + mod);
			break;
		case EApply::kCha: ch->set_cha_add(ch->get_cha_add() + mod);
			break;
		case EApply::kClass:
		case EApply::kLvl: break;
		case EApply::kAge: GET_AGE_ADD(ch) += mod;
			break;
		case EApply::kWeight: GET_WEIGHT_ADD(ch) += mod;
			break;
		case EApply::kHeight: GET_HEIGHT_ADD(ch) += mod;
			break;
		case EApply::kManaRegen: GET_MANAREG(ch) += mod;
			break;
		case EApply::kHp: ch->set_hit_add(ch->get_hit_add() + mod);
			break;
		case EApply::kMove: ch->set_move_add(ch->get_move_add() + mod);
			break;
		case EApply::kGold:
		case EApply::kExp: break;
		case EApply::kAc: GET_AC_ADD(ch) += mod;
			break;
		case EApply::kHitroll: GET_HR_ADD(ch) += mod;
			break;
		case EApply::kDamroll: GET_DR_ADD(ch) += mod;
			break;
		case EApply::kResistFire: GET_RESIST(ch, EResist::kFire) += mod;
			break;
		case EApply::kResistAir: GET_RESIST(ch, EResist::kAir) += mod;
			break;
		case EApply::kResistDark: GET_RESIST(ch, EResist::kDark) += mod;
			break;
		case EApply::kSavingWill: SetSave(ch, ESaving::kWill, GetSave(ch, ESaving::kWill) +  mod);
			break;
		case EApply::kSavingCritical: SetSave(ch, ESaving::kCritical, GetSave(ch, ESaving::kCritical) +  mod);
			break;
		case EApply::kSavingStability: SetSave(ch, ESaving::kStability, GetSave(ch, ESaving::kStability) +  mod);
			break;
		case EApply::kSavingReflex: SetSave(ch, ESaving::kReflex, GetSave(ch, ESaving::kReflex) +  mod);
			break;
		case EApply::kHpRegen:  ch->set_hitreg(ch->get_hitreg() + mod);
			break;
		case EApply::kMoveRegen: ch->set_movereg(ch->get_movereg() + mod);
			break;
		case EApply::kFirstCircle:
		case EApply::kSecondCircle:
		case EApply::kThirdCircle:
		case EApply::kFourthCircle:
		case EApply::kFifthCircle:
		case EApply::kSixthCircle:
		case EApply::kSeventhCircle:
		case EApply::kEighthCircle:
		case EApply::kNinthCircle: ch->add_obj_slot(loc - EApply::kFirstCircle, mod);
			break;
		case EApply::kSize: GET_SIZE_ADD(ch) += mod;
			break;
		case EApply::kArmour: GET_ARMOUR(ch) += mod;
			break;
		case EApply::kPoison: GET_POISON(ch) += mod;
			break;
		case EApply::kCastSuccess: GET_CAST_SUCCESS(ch) += mod;
			break;
		case EApply::kMorale: GET_MORALE(ch) += mod;
			break;
		case EApply::kInitiative: GET_INITIATIVE(ch) += mod;
			break;
		case EApply::kReligion:
			if (add) {
				GET_PRAY(ch) |= mod;
			} else {
				GET_PRAY(ch) &= mod;
			}
			break;
		case EApply::kAbsorbe: GET_ABSORBE(ch) += mod;
			break;
		case EApply::kLikes: GET_LIKES(ch) += mod;
			break;
		case EApply::kResistWater: GET_RESIST(ch, EResist::kWater) += mod;
			break;
		case EApply::kResistEarth: GET_RESIST(ch, EResist::kEarth) += mod;
			break;
		case EApply::kResistVitality: GET_RESIST(ch, EResist::kVitality) += mod;
			break;
		case EApply::kResistMind: GET_RESIST(ch, EResist::kMind) += mod;
			break;
		case EApply::kResistImmunity: GET_RESIST(ch, EResist::kImmunity) += mod;
			break;
		case EApply::kAffectResist: GET_AR(ch) += mod;
			break;
		case EApply::kMagicResist: GET_MR(ch) += mod;
			break;
		case EApply::kAconitumPoison: GET_POISON(ch) += mod;
			break;
		case EApply::kBelenaPoison: GET_SKILL_REDUCE(ch) += mod;
			break;
		case EApply::kDaturaPoison: GET_SKILL_REDUCE(ch) += mod;
			break;
		case EApply::kPhysicResist: GET_PR(ch) += mod; //скиллрезист
			break;
		case EApply::kPhysicDamagePercent: ch->add_abils.percent_physdam_add += mod;
			break;
		case EApply::kMagicDamagePercent: 
			ch->add_abils.percent_spellpower_add += mod;
			break;
		case EApply::kExpPercent: ch->add_abils.percent_exp_add += mod;
			break;
		case EApply::kSpelledBlinkPhys: ch->add_abils.percent_spell_blink_phys += mod;
			break;
			case EApply::kSpelledBlinkMag: ch->add_abils.percent_spell_blink_mag += mod;
				break;
		case EApply::kSkillsBonus: {
			ch->set_skill_bonus(ch->get_skill_bonus() + mod);
		}
		default:break;
	}            // switch
}

// * Снятие аффектов с чара при смерти/уходе в дт.
void reset_affects(CharData *ch) {
	auto af = ch->affected.begin();

	while (af != ch->affected.end()) {
		const auto &affect = *af;

		if (!IS_SET(affect->battleflag, kAfDeadkeep)) {
			af = ch->AffectRemove(af);
		} else {
			++af;
		}
	}
	GET_COND(ch, DRUNK) = 0; // Чтобы не шатало без аффекта "под мухой"
	affect_total(ch);
}

bool IsAffectedBySpell(CharData *ch, ESpell type) {
	if (type == ESpell::kPowerHold) {
		type = ESpell::kHold;
	} else if (type == ESpell::kPowerSilence) {
		type = ESpell::kSilence;
	} else if (type == ESpell::kPowerBlindness) {
		type = ESpell::kBlindness;
	}

	for (const auto &affect : ch->affected) {
		if (affect->type == type) {
			return true;
		}
	}

	return false;
}

bool IsAffectedBySpellWithCasterId(CharData *ch, CharData *vict, ESpell type) {
	if (type == ESpell::kPowerHold) {
		type = ESpell::kHold;
	} else if (type == ESpell::kPowerSilence) {
		type = ESpell::kSilence;
	} else if (type == ESpell::kPowerBlindness) {
		type = ESpell::kBlindness;
	}

	for (const auto &affect : vict->affected) {
		if (affect->type == type && affect->caster_id == ch->get_uid()) {
			return true;
		}
	}

	return false;
}

bool IsNegativeApply(EApply location) {
	for (auto elem : apply_negative) {
		if (location == elem.location)
			return true;
	}
	return false;
}

bool GetAffectNumByName(const std::string &affName, EAffect &result) {
	int base = 0, offset = 0, counter = 0;
	bool endOfArray = false;
	while (!endOfArray) {
		if (affName == std::string(affected_bits[counter])) {
			result = static_cast<EAffect>((base << 30) | (1 << offset));
			return true;
		}
		offset++;
		if (*affected_bits[counter] == '\n') {
			base++;
			offset = 0;
			if (*affected_bits[counter + 1] == '\n')
				endOfArray = true;
		}
		counter++;
	}
	return false;
}

int CalcDuration(CharData *ch, int cnst, int level, int level_divisor, int min, int max) {
	int result = 0;

	if (ch->IsNpc()) {
		result = cnst;
		if (level > 0 && level_divisor > 0)
			level = level / level_divisor;
		else
			level = 0;
		if (min > 0)
			level = std::min(level, min);
		if (max > 0)
			level = std::max(level, max);
		return (level + result);
	}
	result = cnst * kSecsPerMudHour;
	if (level > 0 && level_divisor > 0)
		level = level * kSecsPerMudHour / level_divisor;
	else
		level = 0;
	if (min > 0)
		level = std::min(level, min * kSecsPerMudHour);
	if (max > 0)
		level = std::max(level, max * kSecsPerMudHour);
	result = (level + result) / kSecsPerPlayerAffect;
	return (result);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
