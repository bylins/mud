/* ************************************************************************
*   File: limits.cpp                                      Part of Bylins    *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "gameplay/core/game_limits.h"
#include "gameplay/core/experience.h"
#include "gameplay/affects/affect_data.h"   // issue.mob-flag-affect-materialization: restore re-materialize
#include "administration/privilege.h"
#include "gameplay/mechanics/condition.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/mount.h"

#include "engine/core/char_equip_flags.h"
#include "engine/core/char_handler.h"
#include "engine/core/char_movement.h"
#include "engine/core/obj_handler.h"
#include "engine/db/obj_save.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/ui/color.h"
#include "administration/dupe_check.h"
#include "gameplay/clans/house.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/mechanics/deathtrap.h"
#include "administration/ban.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/mechanics/glory.h"
#include "engine/entities/char_player.h"
#include "gameplay/fight/fight.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/observability/helpers.h"
#include "engine/observability/metrics.h"
#include "utils/tracing/trace_manager.h"
#include "utils/utils_time.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "administration/proxy.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/statistics/zone_exp.h"
#include "gameplay/mechanics/illumination.h"
#include "gameplay/communication/parcel.h"
#include "gameplay/mechanics/damage.h"
#include "gameplay/mechanics/bonus.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/core/remort.h"

#include <fmt/format.h>

#include <random>

const int kRecallSpellsInterval = 28;

extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
int average_day_temp();

namespace {

void CollectCharmiceBoxesForIdleExtract(CharData *master) {
	if (!master || master->IsNpc()) {
		return;
	}

	const auto followers = master->get_followers_list();
	for (const auto follower : followers) {
		if (!follower || !IsCharmice(follower) || follower->get_master() != master) {
			continue;
		}

		mob_ai::extract_charmice(follower, false);
	}
}

} // namespace

// local functions
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void UpdateCharObjects(CharData *ch);    // handler.cpp
// Delete this, if you delete overflow fix in beat_points_update below.
// When age < 20 return the value p0 //
// When age in 20..29 calculate the line between p1 & p2 //
// When age in 30..34 calculate the line between p2 & p3 //
// When age in 35..49 calculate the line between p3 & p4 //
// When age in 50..74 calculate the line between p4 & p5 //
// When age >= 75 return the value p6 //
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6) {

	if (age < 20)
		return (p0);    // < 20   //
	else if (age <= 29)
		return (int) (p1 + (((age - 20) * (p2 - p1)) / 10));    // 20..29 //
	else if (age <= 34)
		return (int) (p2 + (((age - 30) * (p3 - p2)) / 5));    // 30..34 //
	else if (age <= 49)
		return (int) (p3 + (((age - 35) * (p4 - p3)) / 15));    // 35..59 //
	else if (age <= 74)
		return (int) (p4 + (((age - 50) * (p5 - p4)) / 25));    // 50..74 //
	else
		return (p6);    // >= 80 //
}

void handle_recall_spells(CharData *ch) {
	Affect<EApply>::shared_ptr aff;

	for (const auto &af : ch->affected) {
		if (af->affect_type == EAffect::kMemorizeSpells) {
			aff = af;
			break;
		}
	}

	if (!aff) {
		return;
	}
	//максимальный доступный чару круг
	auto max_circle = MUD::Class(ch->GetClass()).GetMaxCircle();
	//обрабатываем только каждые RECALL_SPELLS_INTERVAL секунд
	int secs_left = (kSecsPerPlayerAffect * aff->duration) / kSecsPerMudHour - kSecsPerPlayerAffect;
	if (secs_left/kRecallSpellsInterval < max_circle - aff->modifier || secs_left <= 2) {
		int slot_to_restore = aff->modifier++;

		bool found_spells = false;
		struct SpellMemQueueItem *next = nullptr, *prev = nullptr, *i = ch->mem_queue.queue;
		while (i) {
			next = i->next;
			if (MUD::Class(ch->GetClass()).spells[i->spell_id].GetCircle() == slot_to_restore) {
				if (!found_spells) {
					SendMsgToChar("Ваша голова прояснилась, в памяти всплыло несколько новых заклинаний.\r\n", ch);
					found_spells = true;
				}
				if (prev) prev->next = next;
				if (i == ch->mem_queue.queue) {
					ch->mem_queue.queue = next;
					ch->mem_queue.stored = 0;
				}
				ch->mem_queue.total = std::max(0, ch->mem_queue.total - CalcSpellManacost(ch, i->spell_id));
				sprintf(buf, "Вы вспомнили заклинание \"%s%s%s\".\r\n",
						kColorBoldCyn, MUD::Spell(i->spell_id).GetCName(), kColorNrm);
				SendMsgToChar(buf, ch);
				GET_SPELL_MEM(ch, i->spell_id)++;
				free(i);
			} else prev = i;
			i = next;
		}
	}
}
void handle_recall_spells(CharData::shared_ptr &ch) { handle_recall_spells(ch.get()); }

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

// manapoint gain pr. game hour
int CalcManaGain(const CharData *ch) {
	auto gain{0};
	auto percent{100};
	if (ch->IsNpc()) {
		gain = GetRealLevel(ch);
	} else {
		if (!ch->desc || ch->desc->state != EConState::kPlaying) {
			return 0;
		}

		if (!IS_MANA_CASTER(ch)) {
			auto restore = IntApp(GetRealInt(ch)).mana_per_tic;
			gain = graf(CalcCharAge(ch)->year, restore - 8, restore - 4, restore,
						restore + 5, restore, restore - 4, restore - 8);
		} else {
			gain = IntApp(GetRealInt(ch)).mana_gain;
		}

		if (LIKE_ROOM(ch)) {
			percent += 25;
		}

		if (average_day_temp() < -20) {
			percent -= 10;
		} else if (average_day_temp() < -10) {
			percent -= 5;
		}
	}

	if (world[ch->in_room]->fires) {
		percent += std::max(100, 10 + (world[ch->in_room]->fires * 5) * 2);
	}

	if (AFF_FLAGGED(ch, EAffect::kDeafness)) {
		percent += 15;
	}

	auto stopmem{false};
	if (ch->GetEnemy()) {
		if (IS_MANA_CASTER(ch)) {
			percent -= 50;
		} else {
			percent -= 90;
		}
	} else
		switch (ch->GetPosition()) {
			case EPosition::kSleep:
				if (IS_MANA_CASTER(ch)) {
					percent += 80;
				} else {
					stopmem = true;
					percent = 0;
				}
				break;
			case EPosition::kRest: percent += 45;
				break;
			case EPosition::kSit: percent += 30;
				break;
			case EPosition::kStand: break;
			default: stopmem = true;
				percent = 0;
				break;
		}

	if (!IS_MANA_CASTER(ch) &&
		(AFF_FLAGGED(ch, EAffect::kHold) ||
		AFF_FLAGGED(ch, EAffect::kBlind) ||
		AFF_FLAGGED(ch, EAffect::kSleep) ||
		((ch->in_room != kNowhere) && is_dark(ch->in_room) && !CanUseFeat(ch, EFeat::kDarkReading)))) {
		stopmem = true;
		percent = 0;
	}

	if (!IS_MANA_CASTER(ch)) {
		percent += GET_MANAREG(ch);
	}
	if (AFF_FLAGGED(ch, EAffect::kPoisoned) && percent > 0) {
		percent /= 4;
	}
	if (!ch->IsNpc()) {
		percent *= condition::GetCondPenalty(ch, condition::kMemGain);
	}
	percent = std::clamp(percent, 0, 1000);
	gain = gain * percent / 100;
	return (stopmem ? 0 : gain);
}

int CalcManaGain(const CharData::shared_ptr &ch) { return CalcManaGain(ch.get()); }

// Hitpoint gain pr. game hour
int hit_gain(CharData *ch) {
	int gain = 0, restore = std::max(10, GetRealCon(ch) * 3 / 2), percent = 100;

	if (ch->IsNpc())
		gain = GetRealLevel(ch) + GetRealCon(ch);
	else {
		if (!ch->desc || ch->desc->state != EConState::kPlaying)
			return (0);

		if (!AFF_FLAGGED(ch, EAffect::kNoobRegen)) {
			gain = graf(CalcCharAge(ch)->year, restore - 3, restore, restore, restore - 2,
						restore - 3, restore - 5, restore - 7);
		} else {
			const double base_hp = std::max(1, PlayerSystem::con_total_hp(ch));
			const double rest_time = 80 + 10 * GetRealLevel(ch);
			gain = base_hp / rest_time * 60;
		}

		// Room specification    //
		if (LIKE_ROOM(ch))
			percent += 25;
		// Weather specification //
		if (average_day_temp() < -20)
			percent -= 15;
		else if (average_day_temp() < -10)
			percent -= 10;
	}

	if (world[ch->in_room]->fires)
		percent += std::max(100, 10 + (world[ch->in_room]->fires * 5) * 2);

	// Skill/Spell calculations //

	// Position calculations    //
	switch (ch->GetPosition()) {
		case EPosition::kSleep: percent += 25;
			break;
		case EPosition::kRest: percent += 15;
			break;
		case EPosition::kSit: percent += 10;
			break;
		default:
			break;
	}

	percent += ch->get_hitreg();

	// TODO: перевоткнуть на apply_аффект
	if (AFF_FLAGGED(ch, EAffect::kPoisoned) && percent > 0)
		percent /= 4;

	if (!ch->IsNpc())
		percent *= condition::GetCondPenalty(ch, condition::kHitGain);
	percent = std::max(0, std::min(250, percent));
	gain = gain * percent / 100;
	if (!ch->IsNpc()) {
		if (ch->GetPosition() == EPosition::kIncap || ch->GetPosition() == EPosition::kPerish)
			gain = 0;
	}
	return (gain);
}
int hit_gain(const CharData::shared_ptr &ch) { return hit_gain(ch.get()); }

// move gain pr. game hour //
int move_gain(CharData *ch) {
	int gain = 0, restore = GetRealCon(ch) / 2, percent = 100;

	if (ch->IsNpc())
		gain = GetRealLevel(ch);
	else {
		if (!ch->desc || ch->desc->state != EConState::kPlaying)
			return (0);
		gain =
			graf(CalcCharAge(ch)->year, 15 + restore, 20 + restore, 25 + restore,
				 20 + restore, 16 + restore, 12 + restore, 8 + restore);
		// Room specification    //
		if (LIKE_ROOM(ch))
			percent += 25;
		// Weather specification //
		if (average_day_temp() < -20)
			percent -= 10;
		else if (average_day_temp() < -10)
			percent -= 5;
	}

	if (world[ch->in_room]->fires)
		percent += std::max(50, 10 + world[ch->in_room]->fires * 5);

	// Class/Level calculations //

	// Skill/Spell calculations //


	// Position calculations    //
	switch (ch->GetPosition()) {
		case EPosition::kSleep: percent += 25;
			break;
		case EPosition::kRest: percent += 15;
			break;
		case EPosition::kSit: percent += 10;
			break;
		default:
			break;
	}

	percent += ch->get_movereg();
	if (AFF_FLAGGED(ch, EAffect::kPoisoned) && percent > 0)
		percent /= 4;

	if (!ch->IsNpc())
		percent *= condition::GetCondPenalty(ch, condition::kHitGain);
	percent = std::max(0, std::min(250, percent));
	gain = gain * percent / 100;
	return (gain);
}
int move_gain(const CharData::shared_ptr &ch) { return move_gain(ch.get()); }

#define MINUTE            60
#define UPDATE_PC_ON_BEAT true

int interpolate(int min_value, int pulse) {
	int sign = 1, int_p, frac_p, i, carry, x, y;

	if (min_value < 0) {
		sign = -1;
		min_value = -min_value;
	}
	int_p = min_value / MINUTE;
	frac_p = min_value % MINUTE;
	if (!frac_p)
		return (sign * int_p);
	pulse = time(nullptr) % MINUTE + 1;
	x = MINUTE;
	y = 0;
	for (i = 0, carry = 0; i <= pulse; i++) {
		y += frac_p;
		if (y >= x) {
			x += MINUTE;
			carry = 1;
		} else
			carry = 0;
	}
	return (sign * (int_p + carry));
}
void beat_punish(const CharData::shared_ptr &i) {
	int restore;
	// Проверяем на выпуск чара из кутузки
	if (i->IsFlagged(EPlrFlag::kHelled) && punishments::Get(i, punishments::EType::kHell).duration && punishments::Get(i, punishments::EType::kHell).duration <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kHelled);
		punishments::Get(i, punishments::EType::kHell).reason.clear();
		punishments::Get(i, punishments::EType::kHell).level = 0;
		punishments::Get(i, punishments::EType::kHell).godid = 0;
		punishments::Get(i, punishments::EType::kHell).duration = 0;
		SendMsgToChar("Вас выпустили из темницы.\r\n", i.get());
		if ((restore = GET_LOADROOM(i)) == kNowhere)
			restore = calc_loadroom(i.get());
		restore = GetRoomRnum(restore);
		if (restore == kNowhere) {
			if (GetRealLevel(i) >= kLvlImmortal)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		sight::look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	if (i->IsFlagged(EPlrFlag::kNameDenied)
		&& punishments::Get(i, punishments::EType::kName).duration
		&& punishments::Get(i, punishments::EType::kName).duration <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kNameDenied);
		punishments::Get(i, punishments::EType::kName).reason.clear();
		punishments::Get(i, punishments::EType::kName).level = 0;
		punishments::Get(i, punishments::EType::kName).godid = 0;
		punishments::Get(i, punishments::EType::kName).duration = 0;
		SendMsgToChar("Вас выпустили из КОМНАТЫ ИМЕНИ.\r\n", i.get());

		if ((restore = GET_LOADROOM(i)) == kNowhere) {
			restore = calc_loadroom(i.get());
		}

		restore = GetRoomRnum(restore);

		if (restore == kNowhere) {
			if (GetRealLevel(i) >= kLvlImmortal) {
				restore = r_immort_start_room;
			} else {
				restore = r_mortal_start_room;
			}
		}

		char_from_room(i);
		char_to_room(i, restore);
		sight::look_at_room(i.get(), restore);
		act("С ревом \"Имья, сестра, имья...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	if (i->IsFlagged(EPlrFlag::kMuted)
		&& punishments::Get(i, punishments::EType::kMute).duration != 0
		&& punishments::Get(i, punishments::EType::kMute).duration <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kMuted);
		punishments::Get(i, punishments::EType::kMute).reason.clear();
		punishments::Get(i, punishments::EType::kMute).level = 0;
		punishments::Get(i, punishments::EType::kMute).godid = 0;
		punishments::Get(i, punishments::EType::kMute).duration = 0;
		SendMsgToChar("Вы можете орать.\r\n", i.get());
	}

	if (i->IsFlagged(EPlrFlag::kDumbed)
		&& punishments::Get(i, punishments::EType::kDumb).duration != 0
		&& punishments::Get(i, punishments::EType::kDumb).duration <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kDumbed);
		punishments::Get(i, punishments::EType::kDumb).reason.clear();
		punishments::Get(i, punishments::EType::kDumb).level = 0;
		punishments::Get(i, punishments::EType::kDumb).godid = 0;
		punishments::Get(i, punishments::EType::kDumb).duration = 0;
		SendMsgToChar("Вы можете говорить.\r\n", i.get());
	}

	if (!i->IsFlagged(EPlrFlag::kRegistred)
		&& punishments::Get(i, punishments::EType::kUnreg).duration != 0
		&& punishments::Get(i, punishments::EType::kUnreg).duration <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kRegistred);
		punishments::Get(i, punishments::EType::kUnreg).reason.clear();
		punishments::Get(i, punishments::EType::kUnreg).level = 0;
		punishments::Get(i, punishments::EType::kUnreg).godid = 0;
		punishments::Get(i, punishments::EType::kUnreg).duration = 0;
		SendMsgToChar("Ваша регистрация восстановлена.\r\n", i.get());

		if (i->in_room == r_unreg_start_room) {
			if ((restore = GET_LOADROOM(i)) == kNowhere) {
				restore = calc_loadroom(i.get());
			}

			restore = GetRoomRnum(restore);

			if (restore == kNowhere) {
				if (GetRealLevel(i) >= kLvlImmortal) {
					restore = r_immort_start_room;
				} else {
					restore = r_mortal_start_room;
				}
			}

			char_from_room(i);
			char_to_room(i, restore);
			sight::look_at_room(i.get(), restore);

			act("$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!",
				false, i.get(), nullptr, nullptr, kToRoom);
		};

	}

	if (GET_GOD_FLAG(i, EGf::kGodsLike)
		&& punishments::Get(i, punishments::EType::kGcurse).duration != 0
		&& punishments::Get(i, punishments::EType::kGcurse).duration <= time(nullptr)) {
		REMOVE_BIT(i->player_specials->saved.GodsLike, EGf::kGodsLike);
		SendMsgToChar("Вы более не под защитой Богов.\r\n", i.get());
	}

	if (GET_GOD_FLAG(i, EGf::kGodscurse)
		&& punishments::Get(i, punishments::EType::kGcurse).duration != 0
		&& punishments::Get(i, punishments::EType::kGcurse).duration <= time(nullptr)) {
		REMOVE_BIT(i->player_specials->saved.GodsLike, EGf::kGodscurse);
		SendMsgToChar("Боги более не в обиде на вас.\r\n", i.get());
	}

	if (i->IsFlagged(EPlrFlag::kFrozen)
		&& punishments::Get(i, punishments::EType::kFreeze).duration != 0
		&& punishments::Get(i, punishments::EType::kFreeze).duration <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kFrozen);
		punishments::Get(i, punishments::EType::kFreeze).reason.clear();
		punishments::Get(i, punishments::EType::kFreeze).level = 0;
		punishments::Get(i, punishments::EType::kFreeze).godid = 0;
		punishments::Get(i, punishments::EType::kFreeze).duration = 0;
		SendMsgToChar("Вы оттаяли.\r\n", i.get());
		Glory::remove_freeze(i->get_uid());
		if ((restore = GET_LOADROOM(i)) == kNowhere) {
			restore = calc_loadroom(i.get());
		}
		restore = GetRoomRnum(restore);
		if (restore == kNowhere) {
			if (GetRealLevel(i) >= kLvlImmortal)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		sight::look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	// Проверяем а там ли мы где должны быть по флагам.
	if (i->in_room == kStrangeRoom) {
		restore = i->get_was_in_room();
	} else {
		restore = i->in_room;
	}

	if (i->IsFlagged(EPlrFlag::kHelled)) {
		if (restore != r_helled_start_room) {
			if (i->in_room == kStrangeRoom) {
				i->set_was_in_room(r_helled_start_room);
			} else {
				SendMsgToChar("Чья-то злая воля вернула вас в темницу.\r\n", i.get());
				act("$n возвращен$a в темницу.",
					false, i.get(), nullptr, nullptr, kToRoom);

				char_from_room(i);
				char_to_room(i, r_helled_start_room);
				sight::look_at_room(i.get(), r_helled_start_room);

				i->set_was_in_room(kNowhere);
			}
		}
	} else if (i->IsFlagged(EPlrFlag::kNameDenied)) {
		if (restore != r_named_start_room) {
			if (i->in_room == kStrangeRoom) {
				i->set_was_in_room(r_named_start_room);
			} else {
				SendMsgToChar("Чья-то злая воля вернула вас в комнату имени.\r\n", i.get());
				act("$n возвращен$a в комнату имени.",
					false, i.get(), nullptr, nullptr, kToRoom);
				char_from_room(i);
				char_to_room(i, r_named_start_room);
				sight::look_at_room(i.get(), r_named_start_room);

				i->set_was_in_room(kNowhere);
			};
		};
	} else if (!RegisterSystem::IsRegistered(i.get()) && i->desc && i->desc->state == EConState::kPlaying) {
		if (restore != r_unreg_start_room
			&& !NORENTABLE(i)
			&& !deathtrap::IsSlowDeathtrap(i->in_room)
			&& !check_dupes_host(i->desc, true)) {
			if (i->in_room == kStrangeRoom) {
				i->set_was_in_room(r_unreg_start_room);
			} else {
				act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.\r\n",
					false, i.get(), nullptr, nullptr, kToRoom);

				char_from_room(i);
				char_to_room(i, r_unreg_start_room);
				sight::look_at_room(i.get(), r_unreg_start_room);

				i->set_was_in_room(kNowhere);
			};
		} else if (restore == r_unreg_start_room && check_dupes_host(i->desc, true) && !privilege::IsImmortal(i.get())) {
			SendMsgToChar("Неведомая вытолкнула вас из комнаты для незарегистрированных игроков.\r\n", i.get());
			act("$n появил$u в центре комнаты, правда без штампика регистрации...\r\n",
				false, i.get(), nullptr, nullptr, kToRoom);
			restore = i->get_was_in_room();
			if (restore == kNowhere
				|| restore == r_unreg_start_room) {
				restore = GET_LOADROOM(i);
				if (restore == kNowhere) {
					restore = calc_loadroom(i.get());
				}
				restore = GetRoomRnum(restore);
			}

			char_from_room(i);
			char_to_room(i, restore);
			sight::look_at_room(i.get(), restore);

			i->set_was_in_room(kNowhere);
		}
	}
}

class BeatPointsMetrics {
public:
	BeatPointsMetrics()
		: m_span(tracing::TraceManager::Instance().StartSpan("Beat Points Update"))
		, m_duration("player.beat_update.duration")
		, m_online(0)
		, m_in_combat(0)
	{}

	void track(const CharData* ch) {
		++m_online;
		if (ch->GetEnemy()) {
			++m_in_combat;
		}
		std::string key = "level_" + std::to_string(ch->GetLevel())
			+ "_remort_" + std::to_string(ch->get_remort());
		++m_by_level_remort[key];
	}

	void send() {
		m_span->SetAttribute("player_count", static_cast<int64_t>(m_online));
		observability::OtelMetrics::RecordGauge("players.online.count", m_online);
		observability::OtelMetrics::RecordGauge("players.in_combat.count", m_in_combat);
		for (const auto& [key, count] : m_by_level_remort) {
			size_t level_end = key.find("_remort_");
			observability::OtelMetrics::RecordGauge("players.by_level_remort.count", count, {
				{"level",  key.substr(6, level_end - 6)},
				{"remort", key.substr(level_end + 8)}
			});
		}
	}

private:
	std::unique_ptr<tracing::ISpan> m_span;
	observability::ScopedMetric m_duration;
	int m_online;
	int m_in_combat;
	std::map<std::string, int> m_by_level_remort;
};

void beat_points_update(int pulse) {
	BeatPointsMetrics metrics;
	int restore;

	if (!UPDATE_PC_ON_BEAT)
		return;

	// only for PC's
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state != EConState::kPlaying)
			continue;
//	character_list.foreach_on_copy([&](const auto &i) {
//		if (d->character.get()->IsNpc())
//			return;

		if (d->character->in_room == kNowhere) {
			log("SYSERR: Pulse character in kNowhere.");
			continue;
		}
		
		metrics.track(d->character.get());

		if (NORENTABLE(d->character.get()) <= time(nullptr)) {
			d->character->player_specials->may_rent = 0;
			AGRESSOR(d->character.get()) = 0;
			AGRO(d->character.get()) = 0;
			d->character->agrobd = false;
		}

		if (AGRO(d->character.get()) < time(nullptr)) {
			AGRO(d->character.get()) = 0;
		}
		beat_punish(d->character);

		// This line is used only to control all situations when someone is
		// dead (EPosition::kDead). You can comment it to minimize heartbeat function
		// working time, if you're sure, that you control these situations
		// everywhere. To the time of this code revision I've fix some of them
		// and haven't seen any other.
		//             if (i->GetPosition() == EPosition::kDead)
		//                     die(i, NULL);

		if (d->character->GetPosition() < EPosition::kStun) {
			continue;
		}

		// Restore hitpoints
		restore = hit_gain(d->character.get());
		restore = interpolate(restore, pulse);

		if (AFF_FLAGGED(d->character.get(), EAffect::kBandage)) {
			for (const auto &aff : d->character->affected) {
				if (aff->affect_type == EAffect::kBandage) {
					restore += std::min(d->character.get()->get_real_max_hit() / 10, aff->modifier);
					break;
				}
			}
		}
		if (AFF_FLAGGED(d->character.get(), EAffect::kFrenzy)) {
			for (const auto &aff : d->character->affected) {
				// issue.affect-migration: identify the frenzy affect by its EAffect, not the casting spell.
				if (aff->affect_type == EAffect::kFrenzy && aff->location == EApply::kHpRegen) {
					restore += aff->modifier;
					break;
				}
			}
		}

		if (d->character.get()->get_hit() < d->character.get()->get_real_max_hit()) {
			d->character.get()->set_hit(std::min(d->character.get()->get_hit() + restore, d->character.get()->get_real_max_hit()));
		}

		// Restore PC caster mem
		if (!IS_MANA_CASTER(d->character.get()) && !d->character->mem_queue.Empty()) {
			restore = CalcManaGain(d->character.get());
			restore = interpolate(restore, pulse);
			d->character->mem_queue.stored += restore;

			if (AFF_FLAGGED(d->character.get(), EAffect::kMemorizeSpells)) {
				handle_recall_spells(d->character.get());
			}

			while (d->character->mem_queue.stored > (d->character->mem_queue.Empty() ? 0 : CalcSpellManacost(d->character.get(), d->character->mem_queue.queue->spell_id)) && !d->character->mem_queue.Empty()) {
				auto spell_id = MemQ_learn(d->character.get());
				++GET_SPELL_MEM(d->character, spell_id);
				d->character->caster_level += MUD::Spell(spell_id).GetDanger();
			}

			if (d->character->mem_queue.Empty()) {
				if (GET_RELIGION(d->character.get()) == kReligionMono) {
					SendMsgToChar("Наконец ваши занятия окончены. Вы с улыбкой захлопнули свой часослов.\r\n", d->character.get());
					act("Окончив занятия, $n с улыбкой захлопнул$g часослов.",
						false, d->character.get(), nullptr, nullptr, kToRoom);
				} else {
					SendMsgToChar("Наконец ваши занятия окончены. Вы с улыбкой убрали свои резы.\r\n", d->character.get());
					act("Окончив занятия, $n с улыбкой убрал$g резы.", false, d->character.get(), nullptr, nullptr, kToRoom);
				}
			}
		}

		if (!IS_MANA_CASTER(d->character.get()) && d->character->mem_queue.Empty()) {
			d->character->mem_queue.total = 0;
			d->character->mem_queue.stored = 0;
		}

		// Гейн маны у волхвов
		if (IS_MANA_CASTER(d->character.get()) && d->character->mem_queue.stored < Mana(GetRealWis(d->character.get()))) {
			d->character->mem_queue.stored += CalcManaGain(d->character.get());
			if (d->character->mem_queue.stored >= Mana(GetRealWis(d->character.get()))) {
				d->character->mem_queue.stored = Mana(GetRealWis(d->character.get()));
				SendMsgToChar("Ваша магическая энергия полностью восстановилась\r\n", d->character.get());
			}
		}

		if (IS_MANA_CASTER(d->character.get()) && d->character->mem_queue.stored > Mana(GetRealWis(d->character.get()))) {
			d->character->mem_queue.stored = Mana(GetRealWis(d->character.get()));
		}

		// Restore moves
		restore = move_gain(d->character.get());
		restore = interpolate(restore, pulse);

		//MZ.overflow_fix
		if (d->character.get()->get_move() < d->character.get()->get_real_max_move()) {
			d->character.get()->set_move(std::min(d->character.get()->get_move() + restore, d->character.get()->get_real_max_move()));
		}
		//-MZ.overflow_fix
	}
	
	metrics.send();
}


void gain_condition(CharData *ch, unsigned condition, int value) {
	int cond_state = GET_COND(ch, condition);

	if (condition >= ch->player_specials->saved.conditions.size()) {
		log("SYSERROR : condition=%d (%s:%d)", condition, __FILE__, __LINE__);
		return;
	}

	if (ch->IsNpc() || GET_COND(ch, condition) == -1) {
		return;
	}

	if (privilege::IsGod(ch) && condition != condition::kDrunk) {
		GET_COND(ch, condition) = -1;
		return;
	}

	GET_COND(ch, condition) += value;
	GET_COND(ch, condition) = std::max(0, GET_COND(ch, condition));

	// обработка после увеличения
	switch (condition) {
		case condition::kDrunk: GET_COND(ch, condition) = std::min(kMortallyDrunked + 1, GET_COND(ch, condition));
			break;

		default: GET_COND(ch, condition) = std::min(kMaxCondition, GET_COND(ch, condition));
			break;
	}

	if (cond_state >= kDrunked && GET_COND(ch, condition::kDrunk) < kDrunked) {
		GET_DRUNK_STATE(ch) = 0;
	}

	if (ch->IsFlagged(EPlrFlag::kWriting))
		return;

	int cond_value = GET_COND(ch, condition);
	switch (condition) {
		case condition::kFull:
			if (!condition::GetCondAboveNorm(ch, condition)) {
				return;
			}

			if (cond_value < 30) {
				SendMsgToChar("Вы голодны.\r\n", ch);
			} else if (cond_value < 40) {
				SendMsgToChar("Вы очень голодны.\r\n", ch);
			} else {
				SendMsgToChar("Вы готовы сожрать быка.\r\n", ch);
				//сюда оповещение можно вставить что бы люди видели что чар страдает
			}
			return;

		case condition::kThirst:
			if (!condition::GetCondAboveNorm(ch, condition)) {
				return;
			}

			if (cond_value < 30) {
				SendMsgToChar("Вас мучает жажда.\r\n", ch);
			} else if (cond_value < 40) {
				SendMsgToChar("Вас сильно мучает жажда.\r\n", ch);
			} else {
				SendMsgToChar("Вам хочется выпить озеро.\r\n", ch);
				//сюда оповещение можно вставить что бы люди видели что чар страдает
			}
			return;

		case condition::kDrunk:
			//Если чара прекратило штормить, шлем сообщение
			if (cond_state >= kMortallyDrunked && GET_COND(ch, condition::kDrunk) < kMortallyDrunked) {
				SendMsgToChar("Наконец-то вы протрезвели.\r\n", ch);
			}
			return;

		default: break;
	}
}

void underwater_check() {
	DescriptorData *d;
	for (d = descriptor_list; d; d = d->next) {
		if (d->character
			&& SECT(d->character->in_room) == ESector::kUnderwater
			&& !privilege::IsGod(d->character.get())
			&& !AFF_FLAGGED(d->character, EAffect::kWaterBreath)) {
			sprintf(buf, "Player %s died under water (room %d)",
					GET_NAME(d->character), GET_ROOM_VNUM(d->character->in_room));

			Damage dmg(SimpleDmg(fight::EDamageSource::kUnderwaterDeathTrap), std::max(1, d->character->get_real_max_hit() >> 2), fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);

			if (dmg.Process(d->character.get(), d->character.get()) < 0) {
				log("%s", buf);
			}
		}
	}
}

void check_idling(CharData *ch) {
	if (!NORENTABLE(ch)) {
		if (++(ch->char_specials.timer) > idle_void) {
			ch->set_motion(false);

			if (ch->get_was_in_room() == kNowhere && ch->in_room != kNowhere) {
				// Профилировка пути ухода в пустоту (#3440): логируем разбивку,
				// т.к. он редкий, а в обычном профайлере point_update всё в idle.
				utils::CExecutionTimer tmr;
				double d_save = 0.0, d_crash = 0.0, d_move = 0.0, d_aff = 0.0;
				if (ch->desc) {
					ch->save_char();
				}
				d_save = tmr.delta().count();
				ch->set_was_in_room(ch->in_room);
				if (ch->GetEnemy()) {
					stop_fighting(ch->GetEnemy(), false);
					stop_fighting(ch, true);
				}
				act("$n растворил$u в пустоте.", true, ch, nullptr, nullptr, kToRoom);
				SendMsgToChar("Вы пропали в пустоте этого мира.\r\n", ch);

				tmr.restart();
				Crash_crashsave(ch);
				d_crash = tmr.delta().count();
				tmr.restart();
				RemoveCharFromRoom(ch);
				PlaceCharToRoom(ch, kStrangeRoom);
				d_move = tmr.delta().count();
				tmr.restart();
				room_spells::RemoveSingleAffectFromWorld(ch, room_spells::ERoomAffect::kRuneLabel);
				d_aff = tmr.delta().count();
				log("idle-void %s: save_char=%.4f crashsave=%.4f move=%.4f rune_aff=%.4f",
					GET_NAME(ch), d_save, d_crash, d_move, d_aff);
			} else if (ch->char_specials.timer > idle_rent_time) {
				// Профилировка пути форс-ренты (#3440): см. комментарий выше.
				utils::CExecutionTimer tmr;
				double d_remove = 0.0, d_place = 0.0, d_charmice = 0.0,
					   d_save = 0.0, d_depot = 0.0, d_clan = 0.0;
				if (ch->in_room != kNowhere)
					RemoveCharFromRoom(ch);
				d_remove = tmr.delta().count();
				tmr.restart();
				PlaceCharToRoom(ch, kStrangeRoom);
				d_place = tmr.delta().count();
				tmr.restart();
				CollectCharmiceBoxesForIdleExtract(ch);
				d_charmice = tmr.delta().count();
				tmr.restart();
				Crash_idlesave(ch);
				d_save = tmr.delta().count();
				tmr.restart();
				Depot::exit_char(ch);
				d_depot = tmr.delta().count();
				tmr.restart();
				Clan::clan_invoice(ch, false);
				d_clan = tmr.delta().count();
				log("idle-rent %s: remove=%.4f place=%.4f charmice=%.4f save=%.4f depot=%.4f clan=%.4f",
					GET_NAME(ch), d_remove, d_place, d_charmice, d_save, d_depot, d_clan);
				sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
				mudlog(buf, NRM, kLvlGod, SYSLOG, true);
				character_list.AddToExtractedList(ch);
				// чара в лд уже посейвило при обрыве коннекта
				if (ch->desc) {
					ch->desc->state = EConState::kDisconnect;
					/*
					* For the 'if (d->character)' test in close_socket().
					* -gg 3/1/98 (Happy anniversary.)
					*/
					ch->desc->character = nullptr;
					ch->desc = nullptr;
				}

			}
		}
	}
}

// Update PCs, NPCs, and objects
bool NO_DESTROY(const ObjData *obj) {
	return (obj->get_carried_by()
		|| obj->get_worn_by()
		|| obj->get_in_obj()
//		|| (obj->get_script()->has_triggers())
		|| obj->get_type() == EObjType::kFountain
		|| obj->get_in_room() == kNowhere
		|| obj->has_flag(EObjFlag::kNodecay));
}

bool NO_TIMER(const ObjData *obj) {
	if (obj->get_type() == EObjType::kFountain
			|| !obj->has_flag(EObjFlag::kTicktimer)
			|| (obj->get_in_room() != kNowhere && zone_table[world[obj->get_in_room()]->zone_rn].under_construction))
		return true;
	else
		return false;
}

int up_obj_where(ObjData *obj) {
	if (obj->get_in_obj()) {
		return up_obj_where(obj->get_in_obj());
	} else {
		return (obj->get_worn_by() ? obj->get_worn_by()->in_room :
				(obj->get_carried_by() ? obj->get_carried_by()->in_room : obj->get_in_room()));
	}
}

void hour_update() {
	DescriptorData *i;

	for (i = descriptor_list; i; i = i->next) {
		if  (i->state != EConState::kPlaying || i->character == nullptr || i->character->IsFlagged(EPlrFlag::kWriting))
			continue;
		sprintf(buf, "%sМинул час.%s\r\n", kColorBoldRed, kColorNrm);
		iosystem::write_to_output(buf, i);
	}
}

void room_point_update() {
	int mana;
	for (int count = kFirstRoom; count <= top_of_world; count++) {
		if (world[count]->fires) {
			switch (get_room_sky(count)) {
				case kSkyCloudy:
				case kSkyCloudless: mana = number(1, 2);
					break;
				case kSkyRaining: mana = 2;
					break;
				default: mana = 1;
			}
			world[count]->fires -= std::min(mana, int(world[count]->fires));
			if (world[count]->fires <= 0) {
				if (world[count]->first_character()) {   // #3427: не спамить act() в пустую комнату
					act("Костер затух.",
						false, world[count]->first_character(), nullptr, nullptr, kToRoom);
					act("Костер затух.",
						false, world[count]->first_character(), nullptr, nullptr, kToChar);
				}
				world[count]->fires = 0;
			}
		}
/*
		if (world[count]->portal_time) {
			world[count]->portal_time--;
			if (!world[count]->portal_time) {
				OneWayPortal::remove(world[count]);
				// (issue.affect-flags: pkPenterUnique field retired; this commented-out
				// block was a write to it. PK-uid now lives on the affect via pk_unique.)
				act("Пентаграмма медленно растаяла.",
					false, world[count]->first_character(), nullptr, nullptr, kToRoom);
				act("Пентаграмма медленно растаяла.",
					false, world[count]->first_character(), nullptr, nullptr, kToChar);
			}
		}
*/
		if (world[count]->holes) {
			world[count]->holes--;
			if ((!world[count]->holes || round_up(world[count]->holes) == world[count]->holes)
				&& world[count]->first_character()) {   // #3427: не спамить act() в пустую комнату
				act("Ямку присыпало землей...",
					false, world[count]->first_character(), nullptr, nullptr, kToRoom);
				act("Ямку присыпало землей...",
					false, world[count]->first_character(), nullptr, nullptr, kToChar);
			}
		}
		if (world[count]->ices)
			if (!--world[count]->ices) {
				world[count]->unset_flag(ERoomFlag::kIceTrap);
				deathtrap::remove(world[count]);
			}


		struct TrackData *track, *next_track, *temp;
		auto time_decrease{2};
		for (track = world[count]->track, temp = nullptr; track; track = next_track) {
			next_track = track->next;
			switch (real_sector(count)) {
				case ESector::kOnlyFlying:
				case ESector::kUnderwater:
				case ESector::kSecret:
				case ESector::kWaterSwim:
				case ESector::kWaterNoswim: time_decrease = 31;
					break;
				case ESector::kThickIce:
				case ESector::kNormalIce:
				case ESector::kThinIce: time_decrease = 16;
					break;
				case ESector::kCity: time_decrease = 4;
					break;
				case ESector::kField:
				case ESector::kFieldRain: time_decrease = 2;
					break;
				case ESector::kFieldSnow: time_decrease = 1;
					break;
				case ESector::kForest:
				case ESector::kForestRain: time_decrease = 2;
					break;
				case ESector::kForestSnow: time_decrease = 1;
					break;
				case ESector::kHills:
				case ESector::kHillsRain: time_decrease = 3;
					break;
				case ESector::kHillsSnow: time_decrease = 1;
					break;
				case ESector::kMountain: time_decrease = 4;
					break;
				case ESector::kMountainSnow: time_decrease = 1;
					break;
				default: time_decrease = 2;
			}

			int restore;
			for (mana = 0, restore = false; mana < EDirection::kMaxDirNum; mana++) {
				if ((track->time_income[mana] <<= time_decrease)) {
					restore = true;
				}
				if ((track->time_outgone[mana] <<= time_decrease)) {
					restore = true;
				}
			}
			if (!restore) {
				if (temp) {
					temp->next = next_track;
				} else {
					world[count]->track = next_track;
				}
				free(track);
			} else {
				temp = track;
			}
		}

		deathtrap::CheckIceDeathTrap(count, nullptr);
	}
}

void exchange_point_update() {
	ExchangeItem *exch_item, *next_exch_item;
	for (exch_item = exchange_item_list; exch_item; exch_item = next_exch_item) {
		next_exch_item = exch_item->next;

		if (GET_EXCHANGE_ITEM(exch_item)->get_timer() == 0) {
			std::string cap = GET_EXCHANGE_ITEM(exch_item)->get_PName(grammar::ECase::kNom);
			cap[0] = UPPER(cap[0]);
			sprintf(buf, "Exchange: - %s рассыпал%s от длительного использования.\r\n",
					cap.c_str(), grammar::ObjSexEnding((GET_EXCHANGE_ITEM(exch_item))->get_sex(), 2));
			log("%s", buf);
			extract_exchange_item(exch_item);
		}
	}

}

// * Оповещение о дикее шмотки из храна в клан-канал.
void clan_chest_invoice(ObjData *j) {
	const int room = GET_ROOM_VNUM(j->get_in_obj()->get_in_room());

	if (room <= 0) {
		snprintf(buf, sizeof(buf), "clan_chest_invoice: room=%d, ObjVnum=%d",
				 room, GET_OBJ_VNUM(j));
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character
			&& d->state == EConState::kPlaying
			&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
			&& d->character->IsFlagged(EPrf::kDecayMode)
			&& CLAN(d->character)
			&& CLAN(d->character)->GetRent() == room) {
			SendMsgToChar(d->character.get(), "[Хранилище]: %s'%s%s рассыпал%s в прах'%s\r\n",
						  kColorBoldRed,
						  j->get_short_description().c_str(),
						  clan_get_custom_label(j, CLAN(d->character)).c_str(),
						  grammar::ObjSexEnding((j)->get_sex(), 2),
						  kColorNrm);
		}
	}

	for (const auto &i : Clan::ClanList) {
		if (i->GetRent() == room) {
			std::string log_text = fmt::format("{}{} рассыпал{} в прах\r\n",
												  j->get_short_description(),
												  clan_get_custom_label(j, i),
												  grammar::ObjSexEnding((j)->get_sex(), 2));
			i->chest_log.add(log_text);
			return;
		}
	}
}

enum class ECharmeeObjPos {
	kHandsOrEquip = 0,
	kInventory,
	kContainer
};

// симуляция телла чармиса при дикее стафа по таймеру
void charmee_obj_decay_tell(CharData *charmee, ObjData *obj, ECharmeeObjPos obj_pos) {
	char local_buf[kMaxStringLength];
	char local_buf1[128]; // ну не лепить же сюда malloc

	if (!charmee->has_master()) {
		return;
	}

	if (obj_pos == ECharmeeObjPos::kHandsOrEquip) {
		sprintf(local_buf1, "в ваших руках");
	} else if (obj_pos == ECharmeeObjPos::kInventory) {
		sprintf(local_buf1, "прямо на вас");
	} else if (obj_pos == ECharmeeObjPos::kContainer && obj->get_in_obj()) {
		snprintf(local_buf1, 128, "в %s%s",
				 obj->get_in_obj()->get_PName(grammar::ECase::kPre).c_str(),
				 char_get_custom_label(obj->get_in_obj(), charmee->get_master()).c_str());
	} else {
		sprintf(local_buf1, "непонятно где"); // для дебага -- сюда выполнение доходить не должно
	}

	/*
	   реализация телла не самая красивая, но ничего более подходящего придумать не удается:
	   через do_tell чармисы телять не могут, а если это будет выглядеть не как телл,
	   то в игре будет выглядеть неестественно.
	   короче, рефакторинг приветствуется, если кто-нибудь придумает лучше.
	*/
	std::string cap = obj->get_PName(grammar::ECase::kNom);
	cap[0] = UPPER(cap[0]);
	snprintf(local_buf, kMaxStringLength, "%s сказал%s вам : '%s%s рассыпал%s %s...'",
			 GET_NAME(charmee),
			 grammar::SexEnding((charmee)->get_sex(), 1),
			 cap.c_str(),
			 char_get_custom_label(obj, charmee->get_master()).c_str(),
			 grammar::ObjSexEnding((obj)->get_sex(), 2),
			 local_buf1);
	SendMsgToChar(charmee->get_master(),
				  "%s%s%s\r\n",
				  kColorBoldCyn,
				  utils::CAP(local_buf),
				  kColorNrm);
}

void obj_point_update() {
	utils::CExecutionTimer timer;

	auto tick_result = world_objects.decay_manager().process_tick();

	for (auto it : tick_result.env_destroy) {
		ExtractObjFromWorld(it);
	}

	Parcel::update_timers();
	Depot::update_timers();
	exchange_point_update();
	for (auto j : tick_result.decay_timer) {
		if (j->get_in_obj() && Clan::is_clan_chest(j->get_in_obj())) {
			clan_chest_invoice(j);
		}
		if (IS_CORPSE(j)) {
				ObjData *jj, *next_thing2;
				for (jj = j->get_contains(); jj; jj = next_thing2) {
					next_thing2 = jj->get_next_content();    // Next in inventory
					RemoveObjFromObj(jj);
					if (j->get_in_obj()) {
						PlaceObjIntoObj(jj, j->get_in_obj());
					} else if (j->get_carried_by()) {
						PlaceObjToInventory(jj, j->get_carried_by());
					} else if (j->get_in_room() != kNowhere) {
						PlaceObjToRoom(jj, j->get_in_room());
					} else {
						log("SYSERR: extract %s from %s to kNothing !!!",
							jj->get_PName(grammar::ECase::kNom).c_str(), j->get_PName(grammar::ECase::kNom).c_str());
						// core_dump();
						ExtractObjFromWorld(jj);
					}
				}
				if (j->get_carried_by()) {
					act("$p рассыпал$U в ваших руках.",
						false, j->get_carried_by(), j, nullptr, kToChar);
					RemoveObjFromChar(j);
				} else if (j->get_in_room() != kNowhere) {
					if (!world[j->get_in_room()]->people.empty()) {
						act("Черви полностью сожрали $o3.",
							true, world[j->get_in_room()]->first_character(), j, nullptr, kToRoom);
						act("Черви не оставили от $o1 и следа.",
							true, world[j->get_in_room()]->first_character(), j, nullptr, kToChar);
					}
					RemoveObjFromRoom(j);
				} else if (j->get_in_obj()) {
					RemoveObjFromObj(j);
				}
				ExtractObjFromWorld(j);
		} else {
			if (j->get_timer() == 0 && CheckSript(j, OTRIG_TIMER)) {
					timer_otrigger(j);
					continue;
			}
			// *** рассыпание объекта
			ObjData *jj, *next_thing2;
			for (jj = j->get_contains(); jj; jj = next_thing2) {
				next_thing2 = jj->get_next_content();
				RemoveObjFromObj(jj);
				if (j->get_in_obj()) {
					PlaceObjIntoObj(jj, j->get_in_obj());
				} else if (j->get_worn_by()) {
					PlaceObjToInventory(jj, j->get_worn_by());
				} else if (j->get_carried_by()) {
					PlaceObjToInventory(jj, j->get_carried_by());
				} else if (j->get_in_room() != kNowhere) {
					PlaceObjToRoom(jj, j->get_in_room());
				} else {
					log("SYSERR: extract %s from %s to kNothing !!!",
						jj->get_PName(grammar::ECase::kNom).c_str(),
						j->get_PName(grammar::ECase::kNom).c_str());
					// core_dump();
					ExtractObjFromWorld(jj);
				}
			}
			// Конец Ладник
			if (j->get_worn_by()) {
				switch (j->get_worn_on()) {
					case EEquipPos::kLight:
					case EEquipPos::kShield:
					case EEquipPos::kWield:
					case EEquipPos::kHold:
					case EEquipPos::kBoths:
						if (IsCharmice(j->get_worn_by())) {
							charmee_obj_decay_tell(j->get_worn_by(), j, ECharmeeObjPos::kHandsOrEquip);
						} else {
							snprintf(buf, kMaxStringLength, "$o%s рассыпал$U в ваших руках...",
									 char_get_custom_label(j, j->get_worn_by()).c_str());
							act(buf, false, j->get_worn_by(), j, nullptr, kToChar);
						}
						break;
						default:
						if (IsCharmice(j->get_worn_by())) {
							charmee_obj_decay_tell(j->get_worn_by(), j, ECharmeeObjPos::kInventory);
						} else {
							snprintf(buf, kMaxStringLength, "$o%s рассыпал$U прямо на вас...",
									 char_get_custom_label(j, j->get_worn_by()).c_str());
							act(buf, false, j->get_worn_by(), j, nullptr, kToChar);
						}
						break;
				}
				UnequipChar(j->get_worn_by(), j->get_worn_on(), CharEquipFlags());
			} else if (j->get_carried_by()) {
				if (IsCharmice(j->get_carried_by())) {
					charmee_obj_decay_tell(j->get_carried_by(), j, ECharmeeObjPos::kHandsOrEquip);
				} else {
					snprintf(buf, kMaxStringLength, "$o%s рассыпал$U в ваших руках...",
							 char_get_custom_label(j, j->get_carried_by()).c_str());
					act(buf, false, j->get_carried_by(), j, nullptr, kToChar);
				}
				RemoveObjFromChar(j);
			} else if (j->get_in_room() != kNowhere) {
				if (!world[j->get_in_room()]->people.empty()) {
					act("$o рассыпал$U в прах, который был развеян ветром...",
						false, world[j->get_in_room()]->first_character(), j, nullptr, kToChar);
					act("$o рассыпал$U в прах, который был развеян ветром...",
						false, world[j->get_in_room()]->first_character(), j, nullptr, kToRoom);
				}
			} else if (j->get_in_obj()) {
				// если сыпется в находящемся у чара или чармиса контейнере, то об этом тоже сообщаем
				CharData *cont_owner = nullptr;
				if (j->get_in_obj()->get_carried_by()) {
					cont_owner = j->get_in_obj()->get_carried_by();
				} else if (j->get_in_obj()->get_worn_by()) {
					cont_owner = j->get_in_obj()->get_worn_by();
				}
				if (cont_owner) {
					if (IsCharmice(cont_owner)) {
						charmee_obj_decay_tell(cont_owner, j, ECharmeeObjPos::kContainer);
					} else {
							char buf[kMaxStringLength];
							snprintf(buf, kMaxStringLength, "$o%s рассыпал$U в %s%s...",
									 char_get_custom_label(j, cont_owner).c_str(),
									 j->get_in_obj()->get_PName(grammar::ECase::kPre).c_str(),
									 char_get_custom_label(j->get_in_obj(), cont_owner).c_str());
							act(buf, false, cont_owner, j, nullptr, kToChar);
					}
				}
				RemoveObjFromObj(j);
			}
			ExtractObjFromWorld(j);
		}
	}
	log("obj_point_update stop, size %ld,  delta %f", world_objects.decay_manager().size(), timer.delta().count());

}

void point_update() {
	std::vector<ESpell> real_spell(to_underlying(ESpell::kLast) + 1);
	auto count{0};
	mob_ai::MemoryRecord *mem, *nmem, *pmem;

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		real_spell[count] = spell_id;
		++count;
	}
	std::shuffle(real_spell.begin(), real_spell.end(), std::mt19937(std::random_device()()));

	// Профайлер частей цикла: копим время по каждой части за весь цикл и выводим 1 раз после.
	utils::CExecutionTimer point_update_timer;    // весь цикл целиком
	utils::CExecutionTimer prof;                  // переиспользуем для замера частей
	double t_cond = 0.0, t_hp = 0.0, t_mob = 0.0, t_move = 0.0, t_pos = 0.0, t_idle = 0.0;
	std::size_t scanned = 0, profiled_chars = 0;

	for (auto &ch : character_list) {
		const auto i = ch.get();
		++scanned;

		if (i->purged() || (i->IsNpc() && !i->in_used_zone())) {
			continue;
	  	}

		if (i->IsNpc()) {
			// issue.mob-flag-affect-materialization: on the tick the out-of-combat restore fires,
			// re-materialize any intrinsic buffs a player dispelled during the fight.
			if (i->inc_restore_timer(kSecsPerMudHour)) {
				MaterializeMobFlagAffects(i);
			}
		}
		/* Если чар или моб попытался проснуться а на нем аффект сон,
		то он снова должен валиться в сон */
		if (AFF_FLAGGED(i, EAffect::kSleep) && i->GetPosition() > EPosition::kSleep) {
			i->SetPosition(EPosition::kSleep);
			SendMsgToChar("Вы попытались очнуться, но снова заснули и упали наземь.\r\n", i);
			act("$n попытал$u очнуться, но снова заснул$a и упал$a наземь.",
				true, i, nullptr, nullptr, kToRoom);
		}
		++profiled_chars;
		prof.restart();
		if (!i->IsNpc()) {
			gain_condition(i, condition::kDrunk, -1);
			if (average_day_temp() < -20) {
				gain_condition(i, condition::kFull, +2);
			} else if (average_day_temp() < -5) {
				gain_condition(i, condition::kFull, number(+2, +1));
			} else {
				gain_condition(i, condition::kFull, +1);
			}
			if (average_day_temp() > 25) {
				gain_condition(i, condition::kThirst, +2);
			} else if (average_day_temp() > 20) {
				gain_condition(i, condition::kThirst, number(+2, +1));
			} else {
				gain_condition(i, condition::kThirst, +1);
			}
			UpdateCharObjects(i);
		}
		t_cond += prof.delta().count();
		if (i->GetPosition() >= EPosition::kStun)    // Restore hit points
		{
			prof.restart();
			if (i->IsNpc() || !UPDATE_PC_ON_BEAT) {
				const int count = hit_gain(i);

				if (i->get_hit() < i->get_real_max_hit()) {
					i->set_hit(std::min(i->get_hit() + count, i->get_real_max_hit()));
				}
			}
			t_hp += prof.delta().count();
			prof.restart();
			// Restore mobs
			if (i->IsNpc() && !i->GetEnemy())    // Restore horse
			{
				if (mount::IsHorse(i)) {
					mount::RestoreHorseState(i);
				}
				// Forget PC's
				for (mem = MEMORY(i), pmem = nullptr; mem; mem = nmem) {
					nmem = mem->next;
					if (mem->time <= 0
						&& i->GetEnemy()) {
						pmem = mem;
						continue;
					}
					if (mem->time < time(nullptr)
						|| mem->time <= 0) {
						if (pmem) {
							pmem->next = nmem;
						} else {
							MEMORY(i) = nmem;
						}
						free(mem);
					} else {
						pmem = mem;
					}
				}
				// Remember some spells
				if (i->mob_specials.have_spell && GET_MOB_RNUM(i) >= 0) {
					const auto mob_num = GET_MOB_RNUM(i);
					auto mana{0};
					auto count{0};
					const auto max_mana = GetRealInt(i) * 10;
					while (count <= to_underlying(ESpell::kLast) && mana < max_mana) {
						const auto spell_id = real_spell[count];
						// Моб добивает память спеллов до уровня своего прототипа.
						if (GET_SPELL_MEM(mob_proto + mob_num, spell_id) > GET_SPELL_MEM(i, spell_id)) {
							GET_SPELL_MEM(i, spell_id)++;
							mana += ((MUD::Spell(spell_id).GetMaxMana() + MUD::Spell(spell_id).GetMinMana()) / 2);
							i->caster_level += (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE) ? 1 : 0);
						}
						++count;
					}
				}
			}
			t_mob += prof.delta().count();
			prof.restart();
			// Restore moves
			if (i->IsNpc()
				|| !UPDATE_PC_ON_BEAT) {
				if (i->get_move() < i->get_real_max_move()) {
					i->set_move(std::min(i->get_move() + move_gain(i), i->get_real_max_move()));
				}
			}
			t_move += prof.delta().count();
		} else if (i->GetPosition() == EPosition::kIncap) {
			i->points.hit += 1;
			act("$n, пуская слюни, забил$u в судорогах.", true, i, nullptr, nullptr, kToRoom | kToArenaListen);
		} else if (i->GetPosition() == EPosition::kPerish) {
			act("$n, пуская слюни, забил$u в судорогах.", true, i, nullptr, nullptr, kToRoom | kToArenaListen);
			i->points.hit += 2;
		}
		prof.restart();
		update_pos(i);
		t_pos += prof.delta().count();
		prof.restart();
		if (!i->IsNpc()
			&& GetRealLevel(i) < idle_max_level
			&& !i->IsFlagged(EPrf::kCoderinfo)) {
			check_idling(i);
		}
		t_idle += prof.delta().count();
	}

	const double point_update_total = point_update_timer.delta().count();
	log("Point updating (просмотрено %zu, обработано %zu, всего %.4f сек): условия=%.4f hp=%.4f мобы=%.4f move=%.4f update_pos=%.4f idle=%.4f",
		scanned, profiled_chars, point_update_total, t_cond, t_hp, t_mob, t_move, t_pos, t_idle);
}
void ExtractRepopDecayObject(ObjData *obj) {
	if (obj->get_worn_by()) {
		act("$o рассыпал$U, вспыхнув ярким светом...",
			false, obj->get_worn_by(), obj, nullptr, kToChar);
	} else if (obj->get_carried_by()) {
		act("$o рассыпал$U в ваших руках, вспыхнув ярким светом...",
				false, obj->get_carried_by(), obj, nullptr, kToChar);
	} else if (obj->get_in_room() != kNowhere) {
		if (!world[obj->get_in_room()]->people.empty()) {
			act("$o рассыпал$U, вспыхнув ярким светом...",
					false, world[obj->get_in_room()]->first_character(), obj, nullptr, kToChar);
			act("$o рассыпал$U, вспыхнув ярким светом...",
					false, world[obj->get_in_room()]->first_character(), obj, nullptr, kToRoom);
		}
	} else if (obj->get_in_obj()) {
		CharData *owner = nullptr;
		if (obj->get_in_obj()->get_carried_by()) {
			owner = obj->get_in_obj()->get_carried_by();
		} else if (obj->get_in_obj()->get_worn_by()) {
			owner = obj->get_in_obj()->get_worn_by();
			}

		if (owner) {
			const auto msg = fmt::format("$o рассыпал$U в {}...", obj->get_in_obj()->get_PName(grammar::ECase::kPre));
			act(msg, false, owner, obj, nullptr, kToChar);
		}
	}
	ExtractObjFromWorld(obj);
}

void DecayObjectsOnRepop(UniqueList<ZoneRnum> &zone_list) {
	std::list<ObjData *> extract_list;

	for (auto &zrn : zone_list) {
		const auto zone_vnum = zone_table[zrn].vnum;
		world_objects.foreach_in_zone(zone_vnum, [&extract_list](const ObjData::shared_ptr &j) {
			if (j->has_flag(EObjFlag::kRepopDecay)) {
				extract_list.push_back(j.get());
			}
		});
	}
	for (auto it : extract_list) {
		ExtractRepopDecayObject(it);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
