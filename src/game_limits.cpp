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

#include "game_limits.h"

#include <boost/format.hpp>
#include <random>

#include "entities/world_characters.h"
#include "game_skills/townportal.h"
#include "handler.h"
#include "color.h"
#include "house.h"
#include "game_economics/exchange.h"
#include "game_mechanics/deathtrap.h"
#include "cmd_god/ban.h"
#include "depot.h"
#include "game_mechanics/glory.h"
#include "entities/char_player.h"
#include "game_fight/fight.h"
#include "game_economics/ext_money.h"
#include "statistics/mob_stat.h"
#include "game_magic/spells_info.h"
#include "liquid.h"
#include "structs/global_objects.h"

extern int check_dupes_host(DescriptorData *d, bool autocheck = false);
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int CheckProxy(DescriptorData *ch);
extern int check_death_ice(int room, CharData *ch);
void decrease_level(CharData *ch);
int max_exp_gain_pc(CharData *ch);
int max_exp_loss_pc(CharData *ch);
int average_day_temp();

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
		if (af->type == ESpell::kRecallSpells) {
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
						CCICYN(ch, C_NRM), MUD::Spell(i->spell_id).GetCName(), CCNRM(ch, C_NRM));
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
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING) {
			return 0;
		}

		if (!IS_MANA_CASTER(ch)) {
			auto restore = int_app[GetRealInt(ch)].mana_per_tic;
			gain = graf(age(ch)->year, restore - 8, restore - 4, restore,
						restore + 5, restore, restore - 4, restore - 8);
		} else {
			gain = mana_gain_cs[GetRealInt(ch)];
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
		switch (GET_POS(ch)) {
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
		percent *= ch->get_cond_penalty(P_MEM_GAIN);
	}
	percent = std::clamp(percent, 0, 1000);
	gain = gain * percent / 100;
	return (stopmem ? 0 : gain);
}

int CalcManaGain(const CharData::shared_ptr &ch) { return CalcManaGain(ch.get()); }

// Hitpoint gain pr. game hour
int hit_gain(CharData *ch) {
	int gain = 0, restore = MAX(10, GetRealCon(ch) * 3 / 2), percent = 100;

	if (ch->IsNpc())
		gain = GetRealLevel(ch) + GetRealCon(ch);
	else {
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);

		if (!AFF_FLAGGED(ch, EAffect::kNoobRegen)) {
			gain = graf(age(ch)->year, restore - 3, restore, restore, restore - 2,
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
		percent += MAX(100, 10 + (world[ch->in_room]->fires * 5) * 2);

	// Skill/Spell calculations //

	// Position calculations    //
	switch (GET_POS(ch)) {
		case EPosition::kSleep: percent += 25;
			break;
		case EPosition::kRest: percent += 15;
			break;
		case EPosition::kSit: percent += 10;
			break;
		default:
			break;
	}

	percent += GET_HITREG(ch);

	// TODO: перевоткнуть на apply_аффект
	if (AFF_FLAGGED(ch, EAffect::kPoisoned) && percent > 0)
		percent /= 4;

	if (!ch->IsNpc())
		percent *= ch->get_cond_penalty(P_HIT_GAIN);
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	if (!ch->IsNpc()) {
		if (GET_POS(ch) == EPosition::kIncap || GET_POS(ch) == EPosition::kPerish)
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
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);
		gain =
			graf(age(ch)->year, 15 + restore, 20 + restore, 25 + restore,
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
		percent += MAX(50, 10 + world[ch->in_room]->fires * 5);

	// Class/Level calculations //

	// Skill/Spell calculations //


	// Position calculations    //
	switch (GET_POS(ch)) {
		case EPosition::kSleep: percent += 25;
			break;
		case EPosition::kRest: percent += 15;
			break;
		case EPosition::kSit: percent += 10;
			break;
		default:
			break;
	}

	percent += GET_MOVEREG(ch);
	if (AFF_FLAGGED(ch, EAffect::kPoisoned) && percent > 0)
		percent /= 4;

	if (!ch->IsNpc())
		percent *= ch->get_cond_penalty(P_HIT_GAIN);
	percent = MAX(0, MIN(250, percent));
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
	if (PLR_FLAGGED(i, EPlrFlag::kHelled) && HELL_DURATION(i) && HELL_DURATION(i) <= time(nullptr)) {
		restore = PLR_TOG_CHK(i, EPlrFlag::kHelled);
		if (HELL_REASON(i))
			free(HELL_REASON(i));
		HELL_REASON(i) = nullptr;
		GET_HELL_LEV(i) = 0;
		HELL_GODID(i) = 0;
		HELL_DURATION(i) = 0;
		SendMsgToChar("Вас выпустили из темницы.\r\n", i.get());
		if ((restore = GET_LOADROOM(i)) == kNowhere)
			restore = calc_loadroom(i.get());
		restore = real_room(restore);
		if (restore == kNowhere) {
			if (GetRealLevel(i) >= kLvlImmortal)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	if (PLR_FLAGGED(i, EPlrFlag::kNameDenied)
		&& NAME_DURATION(i)
		&& NAME_DURATION(i) <= time(nullptr)) {
		restore = PLR_TOG_CHK(i, EPlrFlag::kNameDenied);
		if (NAME_REASON(i)) {
			free(NAME_REASON(i));
		}
		NAME_REASON(i) = nullptr;
		GET_NAME_LEV(i) = 0;
		NAME_GODID(i) = 0;
		NAME_DURATION(i) = 0;
		SendMsgToChar("Вас выпустили из КОМНАТЫ ИМЕНИ.\r\n", i.get());

		if ((restore = GET_LOADROOM(i)) == kNowhere) {
			restore = calc_loadroom(i.get());
		}

		restore = real_room(restore);

		if (restore == kNowhere) {
			if (GetRealLevel(i) >= kLvlImmortal) {
				restore = r_immort_start_room;
			} else {
				restore = r_mortal_start_room;
			}
		}

		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i.get(), restore);
		act("С ревом \"Имья, сестра, имья...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	if (PLR_FLAGGED(i, EPlrFlag::kMuted)
		&& MUTE_DURATION(i) != 0
		&& MUTE_DURATION(i) <= time(nullptr)) {
		restore = PLR_TOG_CHK(i, EPlrFlag::kMuted);
		if (MUTE_REASON(i))
			free(MUTE_REASON(i));
		MUTE_REASON(i) = nullptr;
		GET_MUTE_LEV(i) = 0;
		MUTE_GODID(i) = 0;
		MUTE_DURATION(i) = 0;
		SendMsgToChar("Вы можете орать.\r\n", i.get());
	}

	if (PLR_FLAGGED(i, EPlrFlag::kDumbed)
		&& DUMB_DURATION(i) != 0
		&& DUMB_DURATION(i) <= time(nullptr)) {
		restore = PLR_TOG_CHK(i, EPlrFlag::kDumbed);
		if (DUMB_REASON(i))
			free(DUMB_REASON(i));
		DUMB_REASON(i) = nullptr;
		GET_DUMB_LEV(i) = 0;
		DUMB_GODID(i) = 0;
		DUMB_DURATION(i) = 0;
		SendMsgToChar("Вы можете говорить.\r\n", i.get());
	}

	if (!PLR_FLAGGED(i, EPlrFlag::kRegistred)
		&& UNREG_DURATION(i) != 0
		&& UNREG_DURATION(i) <= time(nullptr)) {
		restore = PLR_TOG_CHK(i, EPlrFlag::kRegistred);
		if (UNREG_REASON(i))
			free(UNREG_REASON(i));
		UNREG_REASON(i) = nullptr;
		GET_UNREG_LEV(i) = 0;
		UNREG_GODID(i) = 0;
		UNREG_DURATION(i) = 0;
		SendMsgToChar("Ваша регистрация восстановлена.\r\n", i.get());

		if (IN_ROOM(i) == r_unreg_start_room) {
			if ((restore = GET_LOADROOM(i)) == kNowhere) {
				restore = calc_loadroom(i.get());
			}

			restore = real_room(restore);

			if (restore == kNowhere) {
				if (GetRealLevel(i) >= kLvlImmortal) {
					restore = r_immort_start_room;
				} else {
					restore = r_mortal_start_room;
				}
			}

			char_from_room(i);
			char_to_room(i, restore);
			look_at_room(i.get(), restore);

			act("$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!",
				false, i.get(), nullptr, nullptr, kToRoom);
		};

	}

	if (GET_GOD_FLAG(i, EGf::kGodsLike)
		&& GCURSE_DURATION(i) != 0
		&& GCURSE_DURATION(i) <= time(nullptr)) {
		CLR_GOD_FLAG(i, EGf::kGodsLike);
		SendMsgToChar("Вы более не под защитой Богов.\r\n", i.get());
	}

	if (GET_GOD_FLAG(i, EGf::kGodscurse)
		&& GCURSE_DURATION(i) != 0
		&& GCURSE_DURATION(i) <= time(nullptr)) {
		CLR_GOD_FLAG(i, EGf::kGodscurse);
		SendMsgToChar("Боги более не в обиде на вас.\r\n", i.get());
	}

	if (PLR_FLAGGED(i, EPlrFlag::kFrozen)
		&& FREEZE_DURATION(i) != 0
		&& FREEZE_DURATION(i) <= time(nullptr)) {
		restore = PLR_TOG_CHK(i, EPlrFlag::kFrozen);
		if (FREEZE_REASON(i)) {
			free(FREEZE_REASON(i));
		}
		FREEZE_REASON(i) = nullptr;
		GET_FREEZE_LEV(i) = 0;
		FREEZE_GODID(i) = 0;
		FREEZE_DURATION(i) = 0;
		SendMsgToChar("Вы оттаяли.\r\n", i.get());
		Glory::remove_freeze(GET_UNIQUE(i));
		if ((restore = GET_LOADROOM(i)) == kNowhere) {
			restore = calc_loadroom(i.get());
		}
		restore = real_room(restore);
		if (restore == kNowhere) {
			if (GetRealLevel(i) >= kLvlImmortal)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	// Проверяем а там ли мы где должны быть по флагам.
	if (IN_ROOM(i) == kStrangeRoom) {
		restore = i->get_was_in_room();
	} else {
		restore = IN_ROOM(i);
	}

	if (PLR_FLAGGED(i, EPlrFlag::kHelled)) {
		if (restore != r_helled_start_room) {
			if (IN_ROOM(i) == kStrangeRoom) {
				i->set_was_in_room(r_helled_start_room);
			} else {
				SendMsgToChar("Чья-то злая воля вернула вас в темницу.\r\n", i.get());
				act("$n возвращен$a в темницу.",
					false, i.get(), nullptr, nullptr, kToRoom);

				char_from_room(i);
				char_to_room(i, r_helled_start_room);
				look_at_room(i.get(), r_helled_start_room);

				i->set_was_in_room(kNowhere);
			}
		}
	} else if (PLR_FLAGGED(i, EPlrFlag::kNameDenied)) {
		if (restore != r_named_start_room) {
			if (IN_ROOM(i) == kStrangeRoom) {
				i->set_was_in_room(r_named_start_room);
			} else {
				SendMsgToChar("Чья-то злая воля вернула вас в комнату имени.\r\n", i.get());
				act("$n возвращен$a в комнату имени.",
					false, i.get(), nullptr, nullptr, kToRoom);
				char_from_room(i);
				char_to_room(i, r_named_start_room);
				look_at_room(i.get(), r_named_start_room);

				i->set_was_in_room(kNowhere);
			};
		};
	} else if (!RegisterSystem::is_registered(i.get()) && i->desc && STATE(i->desc) == CON_PLAYING) {
		if (restore != r_unreg_start_room
			&& !NORENTABLE(i)
			&& !deathtrap::IsSlowDeathtrap(IN_ROOM(i))
			&& !check_dupes_host(i->desc, true)) {
			if (IN_ROOM(i) == kStrangeRoom) {
				i->set_was_in_room(r_unreg_start_room);
			} else {
				act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.\r\n",
					false, i.get(), nullptr, nullptr, kToRoom);

				char_from_room(i);
				char_to_room(i, r_unreg_start_room);
				look_at_room(i.get(), r_unreg_start_room);

				i->set_was_in_room(kNowhere);
			};
		} else if (restore == r_unreg_start_room && check_dupes_host(i->desc, true) && !IS_IMMORTAL(i)) {
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
				restore = real_room(restore);
			}

			char_from_room(i);
			char_to_room(i, restore);
			look_at_room(i.get(), restore);

			i->set_was_in_room(kNowhere);
		}
	}
}

void beat_points_update(int pulse) {
	int restore;

	if (!UPDATE_PC_ON_BEAT)
		return;

	// only for PC's
	for (auto d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING) 
			continue;
//	character_list.foreach_on_copy([&](const auto &i) {
//		if (d->character.get()->IsNpc())
//			return;

		if (IN_ROOM(d->character.get()) == kNowhere) {
			log("SYSERR: Pulse character in kNowhere.");
			continue;
		}

		if (NORENTABLE(d->character.get()) <= time(nullptr)) {
			NORENTABLE(d->character.get()) = 0;
			AGRESSOR(d->character.get()) = 0;
			AGRO(d->character.get()) = 0;
			d->character.get()->agrobd = false;
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
		//             if (GET_POS(i) == EPosition::kDead)
		//                     die(i, NULL);

		if (GET_POS(d->character.get()) < EPosition::kStun) {
			continue;
		}

		// Restore hitpoints
		restore = hit_gain(d->character.get());
		restore = interpolate(restore, pulse);

		if (AFF_FLAGGED(d->character.get(), EAffect::kBandage)) {
			for (const auto &aff : d->character.get()->affected) {
				if (aff->type == ESpell::kBandage) {
					restore += std::min(GET_REAL_MAX_HIT(d->character.get()) / 10, aff->modifier);
					break;
				}
			}
		}

		if (GET_HIT(d->character.get()) < GET_REAL_MAX_HIT(d->character.get())) {
			GET_HIT(d->character.get()) = std::min(GET_HIT(d->character.get()) + restore, GET_REAL_MAX_HIT(d->character.get()));
		}

		// Restore PC caster mem
		if (!IS_MANA_CASTER(d->character.get()) && !d->character.get()->mem_queue.Empty()) {
			restore = CalcManaGain(d->character.get());
			restore = interpolate(restore, pulse);
			d->character.get()->mem_queue.stored += restore;

			if (AFF_FLAGGED(d->character.get(), EAffect::kMemorizeSpells)) {
				handle_recall_spells(d->character.get());
			}

			while (d->character.get()->mem_queue.stored > GET_MEM_CURRENT(d->character.get()) && !d->character.get()->mem_queue.Empty()) {
				auto spell_id = MemQ_learn(d->character.get());
				++GET_SPELL_MEM(d->character.get(), spell_id);
				d->character.get()->caster_level += MUD::Spell(spell_id).GetDanger();
			}

			if (d->character.get()->mem_queue.Empty()) {
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

		if (!IS_MANA_CASTER(d->character.get()) && d->character.get()->mem_queue.Empty()) {
			d->character.get()->mem_queue.total = 0;
			d->character.get()->mem_queue.stored = 0;
		}

		// Гейн маны у волхвов
		if (IS_MANA_CASTER(d->character.get()) && d->character.get()->mem_queue.stored < GET_MAX_MANA(d->character.get())) {
			d->character.get()->mem_queue.stored += CalcManaGain(d->character.get());
			if (d->character.get()->mem_queue.stored >= GET_MAX_MANA(d->character.get())) {
				d->character.get()->mem_queue.stored = GET_MAX_MANA(d->character.get());
				SendMsgToChar("Ваша магическая энергия полностью восстановилась\r\n", d->character.get());
			}
		}

		if (IS_MANA_CASTER(d->character.get()) && d->character.get()->mem_queue.stored > GET_MAX_MANA(d->character.get())) {
			d->character.get()->mem_queue.stored = GET_MAX_MANA(d->character.get());
		}

		// Restore moves
		restore = move_gain(d->character.get());
		restore = interpolate(restore, pulse);

		//MZ.overflow_fix
		if (GET_MOVE(d->character.get()) < GET_REAL_MAX_MOVE(d->character.get())) {
			GET_MOVE(d->character.get()) = MIN(GET_MOVE(d->character.get()) + restore, GET_REAL_MAX_MOVE(d->character.get()));
		}
		//-MZ.overflow_fix
	}
}

void update_clan_exp(CharData *ch, int gain) {
	if (CLAN(ch) && gain != 0) {
		// экспа для уровня клана (+ только на праве, - любой, но /5)
		if (gain < 0 || GET_GOD_FLAG(ch, EGf::kRemort)) {
			int tmp = gain > 0 ? gain : gain / 5;
			CLAN(ch)->SetClanExp(ch, tmp);
		}
		// экспа для топа кланов за месяц (учитываются все + и -)
		CLAN(ch)->last_exp.add_temp(gain);
		// экспа для топа кланов за все время (учитываются все + и -)
		CLAN(ch)->AddTopExp(ch, gain);
		// экспа для авто-очистки кланов (учитываются только +)
		if (gain > 0) {
			CLAN(ch)->exp_history.add_exp(gain);
		}
	}
}

void EndowExpToChar(CharData *ch, int gain) {
	int is_altered = false;
	int num_levels = 0;
	char local_buf[128];

	if (ch->IsNpc()) {
		ch->set_exp(ch->get_exp() + gain);
		return;
	} else {
		ch->dps_add_exp(gain);
		ZoneExpStat::add(zone_table[world[ch->in_room]->zone_rn].vnum, gain);
	}

	if (!ch->IsNpc() && ((GetRealLevel(ch) < 1 || GetRealLevel(ch) >= kLvlImmortal))) {
		return;
	}

	if (gain > 0 && GetRealLevel(ch) < kLvlImmortal) {
		gain = std::min(max_exp_gain_pc(ch), gain);    // put a cap on the max gain per kill
		ch->set_exp(ch->get_exp() + gain);
		if (ch->get_exp() >= GetExpUntilNextLvl(ch, kLvlImmortal)) {
			if (!GET_GOD_FLAG(ch, EGf::kRemort) && GetRealRemort(ch) < kMaxRemort) {
				if (Remort::can_remort_now(ch)) {
					SendMsgToChar(ch, "%sПоздравляем, вы получили право на перевоплощение!%s\r\n",
								  CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
				} else {
					SendMsgToChar(ch,
								  "%sПоздравляем, вы набрали максимальное количество опыта!\r\n"
								  "%s%s\r\n", CCIGRN(ch, C_NRM), Remort::WHERE_TO_REMORT_STR.c_str(), CCNRM(ch, C_NRM));
				}
				SET_GOD_FLAG(ch, EGf::kRemort);
			}
		}
		ch->set_exp(std::min(ch->get_exp(), GetExpUntilNextLvl(ch, kLvlImmortal) - 1));
		while (GetRealLevel(ch) < kLvlImmortal && GET_EXP(ch) >= GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
			ch->set_level(ch->GetLevel() + 1);
			num_levels++;
			sprintf(local_buf, "%sВы достигли следующего уровня!%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			SendMsgToChar(local_buf, ch);
			advance_level(ch);
			is_altered = true;
		}

		if (is_altered) {
			sprintf(local_buf, "%s advanced %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
			mudlog(local_buf, BRF, kLvlImplementator, SYSLOG, true);
		}
	} else if (gain < 0 && GetRealLevel(ch) < kLvlImmortal) {
		gain = std::max(-max_exp_loss_pc(ch), gain);    // Cap max exp lost per death
		ch->set_exp(ch->get_exp() + gain);
		while (GetRealLevel(ch) > 1 && GET_EXP(ch) < GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
			ch->set_level(ch->GetLevel() - 1);
			num_levels++;
			sprintf(local_buf,
					"%sВы потеряли уровень. Вам должно быть стыдно!%s\r\n",
					CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
			SendMsgToChar(local_buf, ch);
			decrease_level(ch);
			is_altered = true;
		}
		if (is_altered) {
			sprintf(local_buf, "%s decreases %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
			mudlog(local_buf, BRF, kLvlImplementator, SYSLOG, true);
		}
	}
	if ((GET_EXP(ch) < GetExpUntilNextLvl(ch, kLvlImmortal) - 1)
		&& GET_GOD_FLAG(ch, EGf::kRemort)
		&& gain
		&& (GetRealLevel(ch) < kLvlImmortal)) {
		if (Remort::can_remort_now(ch)) {
			SendMsgToChar(ch, "%sВы потеряли право на перевоплощение!%s\r\n",
						  CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		CLR_GOD_FLAG(ch, EGf::kRemort);
	}

	char_stat::AddClassExp(ch->GetClass(), gain);
	update_clan_exp(ch, gain);
}

// юзается исключительно в act.wizards.cpp в имм командах "advance" и "set exp".
void gain_exp_regardless(CharData *ch, int gain) {
	int is_altered = false;
	int num_levels = 0;

	ch->set_exp(ch->get_exp() + gain);
	if (!ch->IsNpc()) {
		if (gain > 0) {
			while (GetRealLevel(ch) < kLvlImplementator && GET_EXP(ch) >= GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
				ch->set_level(ch->GetLevel() + 1);
				num_levels++;
				sprintf(buf, "%sВы достигли следующего уровня!%s\r\n",
						CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
				SendMsgToChar(buf, ch);

				advance_level(ch);
				is_altered = true;
			}

			if (is_altered) {
				sprintf(buf, "%s advanced %d level%s to level %d.",
						GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
			}
		} else if (gain < 0) {
			// Pereplut: глупый участок кода.
			//			gain = MAX(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
			//			GET_EXP(ch) += gain;
			//			if (GET_EXP(ch) < 0)
			//				GET_EXP(ch) = 0;
			while (GetRealLevel(ch) > 1 && GET_EXP(ch) < GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
				ch->set_level(ch->GetLevel() - 1);
				num_levels++;
				sprintf(buf,
						"%sВы потеряли уровень!%s\r\n",
						CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
				SendMsgToChar(buf, ch);
				decrease_level(ch);
				is_altered = true;
			}
			if (is_altered) {
				sprintf(buf, "%s decreases %d level%s to level %d.",
						GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GetRealLevel(ch));
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
			}
		}

	}
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

	if (IS_GOD(ch) && condition != DRUNK) {
		GET_COND(ch, condition) = -1;
		return;
	}

	GET_COND(ch, condition) += value;
	GET_COND(ch, condition) = std::max(0, GET_COND(ch, condition));

	// обработка после увеличения
	switch (condition) {
		case DRUNK: GET_COND(ch, condition) = std::min(kMortallyDrunked + 1, GET_COND(ch, condition));
			break;

		default: GET_COND(ch, condition) = std::min(kMaxCondition, GET_COND(ch, condition));
			break;
	}

	if (cond_state >= kDrunked && GET_COND(ch, DRUNK) < kDrunked) {
		GET_DRUNK_STATE(ch) = 0;
	}

	if (PLR_FLAGGED(ch, EPlrFlag::kWriting))
		return;

	int cond_value = GET_COND(ch, condition);
	switch (condition) {
		case FULL:
			if (!GET_COND_M(ch, condition)) {
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

		case THIRST:
			if (!GET_COND_M(ch, condition)) {
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

		case DRUNK:
			//Если чара прекратило штормить, шлем сообщение
			if (cond_state >= kMortallyDrunked && GET_COND(ch, DRUNK) < kMortallyDrunked) {
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
			&& !IS_GOD(d->character)
			&& !AFF_FLAGGED(d->character, EAffect::kWaterBreath)) {
			sprintf(buf, "Player %s died under water (room %d)",
					GET_NAME(d->character), GET_ROOM_VNUM(d->character->in_room));

			Damage dmg(SimpleDmg(kTypeWaterdeath), MAX(1, GET_REAL_MAX_HIT(d->character) >> 2), fight::kUndefDmg);
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
				if (ch->desc) {
					ch->save_char();
				}
				ch->set_was_in_room(ch->in_room);
				if (ch->GetEnemy()) {
					stop_fighting(ch->GetEnemy(), false);
					stop_fighting(ch, true);
				}
				act("$n растворил$u в пустоте.", true, ch, nullptr, nullptr, kToRoom);
				SendMsgToChar("Вы пропали в пустоте этого мира.\r\n", ch);

				Crash_crashsave(ch);
				ExtractCharFromRoom(ch);
				PlaceCharToRoom(ch, kStrangeRoom);
				RemoveRuneLabelFromWorld(ch, ESpell::kRuneLabel);
			} else if (ch->char_specials.timer > idle_rent_time) {
				if (ch->in_room != kNowhere)
					ExtractCharFromRoom(ch);
				PlaceCharToRoom(ch, kStrangeRoom);
				Crash_idlesave(ch);
				Depot::exit_char(ch);
				Clan::clan_invoice(ch, false);
				sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
				mudlog(buf, NRM, kLvlGod, SYSLOG, true);
				ExtractCharFromWorld(ch, false);

				// чара в лд уже посейвило при обрыве коннекта
				if (ch->desc) {
					STATE(ch->desc) = CON_DISCONNECT;
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
inline bool NO_DESTROY(const ObjData *obj) {
	return (obj->get_carried_by()
		|| obj->get_worn_by()
		|| obj->get_in_obj()
//		|| (obj->get_script()->has_triggers())
		|| GET_OBJ_TYPE(obj) == EObjType::kFountain
		|| obj->get_in_room() == kNowhere
		|| (obj->has_flag(EObjFlag::kNodecay)
			&& !ROOM_FLAGGED(obj->get_in_room(), ERoomFlag::kDeathTrap)));
}

inline bool NO_TIMER(const ObjData *obj) {
	if (GET_OBJ_TYPE(obj) == EObjType::kFountain)
		return true;
// так как таймер всего 30 шмот из тестовой зоны в своей зоне запретим тикать на земле
// полный вариан
/*	ZoneRnum zrn = 0;
	if (GET_OBJ_VNUM_ZONE_FROM(obj) > 0) {
		for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size() - 1); zrn++) {
			if (zone_table[zrn].vnum == GET_OBJ_VNUM_ZONE_FROM(obj))
				break;
		}

		if (zone_table[zrn].under_construction && zone_table[world[obj->get_in_room()]->zone_rn].under_construction) {
			return true;
		}
	}
*/
// а почему бы не так?
	if (zone_table[world[obj->get_in_room()]->zone_rn].under_construction && !obj->has_flag(EObjFlag::kTicktimer))
		return true;
	return false;
}

int up_obj_where(ObjData *obj) {
	if (obj->get_in_obj()) {
		return up_obj_where(obj->get_in_obj());
	} else {
		return OBJ_WHERE(obj);
	}
}

void hour_update() {
	DescriptorData *i;

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING || i->character == nullptr || PLR_FLAGGED(i->character, EPlrFlag::kWriting))
			continue;
		sprintf(buf, "%sМинул час.%s\r\n", CCIRED(i->character, C_NRM), CCNRM(i->character, C_NRM));
		SEND_TO_Q(buf, i);
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
			world[count]->fires -= MIN(mana, world[count]->fires);
			if (world[count]->fires <= 0) {
				act("Костер затух.",
					false, world[count]->first_character(), nullptr, nullptr, kToRoom);
				act("Костер затух.",
					false, world[count]->first_character(), nullptr, nullptr, kToChar);

				world[count]->fires = 0;
			}
		}

		if (world[count]->portal_time) {
			world[count]->portal_time--;
			if (!world[count]->portal_time) {
				OneWayPortal::remove(world[count]);
				world[count]->pkPenterUnique = 0;
				act("Пентаграмма медленно растаяла.",
					false, world[count]->first_character(), nullptr, nullptr, kToRoom);
				act("Пентаграмма медленно растаяла.",
					false, world[count]->first_character(), nullptr, nullptr, kToChar);
			}
		}
		if (world[count]->holes) {
			world[count]->holes--;
			if (!world[count]->holes || roundup(world[count]->holes) == world[count]->holes) {
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

		world[count]->glight = (world[count]->glight < 0 ? 0 : world[count]->glight);
		world[count]->gdark = (world[count]->gdark < 0 ? 0 : world[count]->gdark);

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

		check_death_ice(count, nullptr);
	}
}

void exchange_point_update() {
	ExchangeItem *exch_item, *next_exch_item;
	for (exch_item = exchange_item_list; exch_item; exch_item = next_exch_item) {
		next_exch_item = exch_item->next;
		if (GET_EXCHANGE_ITEM(exch_item)->get_timer() > 0) {
			GET_EXCHANGE_ITEM(exch_item)->dec_timer(1, true, true);
		}

		if (GET_EXCHANGE_ITEM(exch_item)->get_timer() <= 0) {
			std::string cap = GET_EXCHANGE_ITEM(exch_item)->get_PName(0);
			cap[0] = UPPER(cap[0]);
			sprintf(buf, "Exchange: - %s рассыпал%s от длительного использования.\r\n",
					cap.c_str(), GET_OBJ_SUF_2(GET_EXCHANGE_ITEM(exch_item)));
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
			&& STATE(d) == CON_PLAYING
			&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
			&& PRF_FLAGGED(d->character, EPrf::kDecayMode)
			&& CLAN(d->character)
			&& CLAN(d->character)->GetRent() == room) {
			SendMsgToChar(d->character.get(), "[Хранилище]: %s'%s%s рассыпал%s в прах'%s\r\n",
						  CCIRED(d->character, C_NRM),
						  j->get_short_description().c_str(),
						  clan_get_custom_label(j, CLAN(d->character)).c_str(),
						  GET_OBJ_SUF_2(j),
						  CCNRM(d->character, C_NRM));
		}
	}

	for (const auto &i : Clan::ClanList) {
		if (i->GetRent() == room) {
			std::string log_text = boost::str(boost::format("%s%s рассыпал%s в прах\r\n")
												  % j->get_short_description()
												  % clan_get_custom_label(j, i)
												  % GET_OBJ_SUF_2(j));
			i->chest_log.add(log_text);
			return;
		}
	}
}

// * Дикей шмоток в клан-хране.
void clan_chest_point_update(ObjData *j) {
	// если это шмотка из большого набора предметов и она такая одна в хране, то таймер тикает в 2 раза быстрее
	if (SetSystem::is_big_set(j, true)) {
		SetSystem::init_vnum_list(GET_OBJ_VNUM(j));
		if (!SetSystem::find_set_item(j->get_in_obj()) && j->get_timer() > 0)
			j->dec_timer();
	}

	if (j->get_timer() > 0) {
		j->dec_timer();
	}

	if (j->get_timer() <= 0
		|| (j->has_flag(EObjFlag::kZonedacay)
			&& GET_OBJ_VNUM_ZONE_FROM(j)
			&& up_obj_where(j->get_in_obj()) != kNowhere
			&& GET_OBJ_VNUM_ZONE_FROM(j) != zone_table[world[up_obj_where(j->get_in_obj())]->zone_rn].vnum)) {
		clan_chest_invoice(j);
		RemoveObjFromObj(j);
		ExtractObjFromWorld(j);
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
				 obj->get_in_obj()->get_PName(5).c_str(),
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
	std::string cap = obj->get_PName(0);
	cap[0] = UPPER(cap[0]);
	snprintf(local_buf, kMaxStringLength, "%s сказал%s вам : '%s%s рассыпал%s %s...'",
			 GET_NAME(charmee),
			 GET_CH_SUF_1(charmee),
			 cap.c_str(),
			 char_get_custom_label(obj, charmee->get_master()).c_str(),
			 GET_OBJ_SUF_2(obj),
			 local_buf1);
	SendMsgToChar(charmee->get_master(),
				  "%s%s%s\r\n",
				  CCICYN(charmee->get_master(), C_NRM),
				  CAP(local_buf),
				  CCNRM(charmee->get_master(), C_NRM));
}

void obj_point_update() {
	int count;

	world_objects.foreach_on_copy([&](const ObjData::shared_ptr &j) {
		// смотрим клан-сундуки
		if (j->get_in_obj() && Clan::is_clan_chest(j->get_in_obj())) {
			clan_chest_point_update(j.get());
			return;
		}

		// контейнеры на земле с флагом !дикей, но не загружаемые в этой комнате, а хз кем брошенные
		// извращение конечно перебирать на каждый объект команды резета зоны, но в голову ниче интересного
		// не лезет, да и не так уж и много на самом деле таких предметов будет, условий порядочно
		// а так привет любителям оставлять книги в клановых сумках или лоадить в замке столы
		if (j->get_in_obj()
			&& !j->get_in_obj()->get_carried_by()
			&& !j->get_in_obj()->get_worn_by()
			&& j->get_in_obj()->has_flag(EObjFlag::kNodecay)
			&& GET_ROOM_VNUM(j->get_in_obj()->get_in_room()) % 100 != 99) {
			int zone = world[j->get_in_obj()->get_in_room()]->zone_rn;
			auto find{false};
			const auto clan = Clan::GetClanByRoom(j->get_in_obj()->get_in_room());
			if (!clan)   // внутри замков даже и смотреть не будем
			{
				for (int cmd_no = 0; zone_table[zone].cmd[cmd_no].command != 'S'; ++cmd_no) {
					if (zone_table[zone].cmd[cmd_no].command == 'O'
						&& zone_table[zone].cmd[cmd_no].arg1 == GET_OBJ_RNUM(j->get_in_obj())
						&& zone_table[zone].cmd[cmd_no].arg3 == j->get_in_obj()->get_in_room()) {
						find = true;
						break;
					}
				}
			}

			if (!find && j->get_timer() > 0) {
				j->dec_timer();
			}
		}

		// If this is a corpse
		if (IS_CORPSE(j))    // timer count down
		{
			if (j->get_timer() > 0) {
				j->dec_timer();
			}

			if (j->get_timer() <= 0) {
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
							jj->get_PName(0).c_str(), j->get_PName(0).c_str());
						// core_dump();
						ExtractObjFromWorld(jj);
					}
				}

				if (j->get_carried_by()) {
					act("$p рассыпал$U в ваших руках.",
						false, j->get_carried_by(), j.get(), nullptr, kToChar);
					RemoveObjFromChar(j.get());
				} else if (j->get_in_room() != kNowhere) {
					if (!world[j->get_in_room()]->people.empty()) {
						act("Черви полностью сожрали $o3.",
							true, world[j->get_in_room()]->first_character(), j.get(), nullptr, kToRoom);
						act("Черви не оставили от $o1 и следа.",
							true, world[j->get_in_room()]->first_character(), j.get(), nullptr, kToChar);
					}

					RemoveObjFromRoom(j.get());
				} else if (j->get_in_obj()) {
					RemoveObjFromObj(j.get());
				}
				ExtractObjFromWorld(j.get());
			}
		} else {
			if (CheckSript(j.get(), OTRIG_TIMER)) {
				if (j->get_timer() > 0 && j->has_flag(EObjFlag::kTicktimer)) {
					j->dec_timer();
				}
				if (!j->get_timer()) {
					timer_otrigger(j.get());
					return;
				}
			} else if (GET_OBJ_DESTROY(j) > 0
				&& !NO_DESTROY(j.get())) {
				j->dec_destroyer();
			}
			if (j && (j->get_in_room() != kNowhere) && j->get_timer() > 0 && !NO_TIMER(j.get())) {
				j->dec_timer();
			}
			if (j && ((j->has_flag(EObjFlag::kZonedacay)
					&& GET_OBJ_VNUM_ZONE_FROM(j)
					&& up_obj_where(j.get()) != kNowhere
					&& GET_OBJ_VNUM_ZONE_FROM(j) != zone_table[world[up_obj_where(j.get())]->zone_rn].vnum)
					|| j->get_timer() <= 0
					|| GET_OBJ_DESTROY(j) == 0)) {
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
							jj->get_PName(0).c_str(),
							j->get_PName(0).c_str());
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
							if (IS_CHARMICE(j->get_worn_by())) {
								charmee_obj_decay_tell(j->get_worn_by(), j.get(), ECharmeeObjPos::kHandsOrEquip);
							} else {
								snprintf(buf, kMaxStringLength, "$o%s рассыпал$U в ваших руках...",
										 char_get_custom_label(j.get(), j->get_worn_by()).c_str());
								act(buf, false, j->get_worn_by(), j.get(), nullptr, kToChar);
							}
							break;

						default:
							if (IS_CHARMICE(j->get_worn_by())) {
								charmee_obj_decay_tell(j->get_worn_by(), j.get(), ECharmeeObjPos::kInventory);
							} else {
								snprintf(buf, kMaxStringLength, "$o%s рассыпал$U прямо на вас...",
										 char_get_custom_label(j.get(), j->get_worn_by()).c_str());
								act(buf, false, j->get_worn_by(), j.get(), nullptr, kToChar);
							}
							break;
					}
					UnequipChar(j->get_worn_by(), j->get_worn_on(), CharEquipFlags());
				} else if (j->get_carried_by()) {
					if (IS_CHARMICE(j->get_carried_by())) {
						charmee_obj_decay_tell(j->get_carried_by(), j.get(), ECharmeeObjPos::kHandsOrEquip);
					} else {
						snprintf(buf, kMaxStringLength, "$o%s рассыпал$U в ваших руках...",
								 char_get_custom_label(j.get(), j->get_carried_by()).c_str());
						act(buf, false, j->get_carried_by(), j.get(), nullptr, kToChar);
					}
					RemoveObjFromChar(j.get());
				} else if (j->get_in_room() != kNowhere) {
					if (j->get_timer() <= 0 && j->has_flag(EObjFlag::kNodecay)) {
						snprintf(buf, kMaxStringLength, "ВНИМАНИЕ!!! Объект: %s VNUM: %d рассыпался по таймеру на земле в комнате: %d",
								 GET_OBJ_PNAME(j.get(), 0).c_str(), GET_OBJ_VNUM(j.get()), world[j->get_in_room()]->room_vn);
						mudlog(buf, CMP, kLvlGreatGod, ERRLOG, true);

					}
					if (!world[j->get_in_room()]->people.empty()) {
						act("$o рассыпал$U в прах, который был развеян ветром...",
							false, world[j->get_in_room()]->first_character(), j.get(), nullptr, kToChar);
						act("$o рассыпал$U в прах, который был развеян ветром...",
							false, world[j->get_in_room()]->first_character(), j.get(), nullptr, kToRoom);
					}

					RemoveObjFromRoom(j.get());
				} else if (j->get_in_obj()) {
					// если сыпется в находящемся у чара или чармиса контейнере, то об этом тоже сообщаем
					CharData *cont_owner = nullptr;
					if (j->get_in_obj()->get_carried_by()) {
						cont_owner = j->get_in_obj()->get_carried_by();
					} else if (j->get_in_obj()->get_worn_by()) {
						cont_owner = j->get_in_obj()->get_worn_by();
					}

					if (cont_owner) {
						if (IS_CHARMICE(cont_owner)) {
							charmee_obj_decay_tell(cont_owner, j.get(), ECharmeeObjPos::kContainer);
						} else {
							char buf[kMaxStringLength];
							snprintf(buf, kMaxStringLength, "$o%s рассыпал$U в %s%s...",
									 char_get_custom_label(j.get(), cont_owner).c_str(),
									 j->get_in_obj()->get_PName(5).c_str(),
									 char_get_custom_label(j->get_in_obj(), cont_owner).c_str());
							act(buf, false, cont_owner, j.get(), nullptr, kToChar);
						}
					}
					RemoveObjFromObj(j.get());
				}
				ExtractObjFromWorld(j.get());
			} else {
				if (!j) {
					return;
				}

				// decay poison && other affects
				for (count = 0; count < kMaxObjAffect; count++) {
					if (j->get_affected(count).location == EApply::kPoison) {
						j->dec_affected_value(count);
						if (j->get_affected(count).modifier <= 0) {
							j->set_affected(count, EApply::kNone, 0);
						}
					}
				}
			}
		}
	});

	// Тонущие, падающие, и сыпящиеся обьекты.
	world_objects.foreach_on_copy([&](const ObjData::shared_ptr &j) {
		CheckObjDecay(j.get());
	});
}

void point_update() {
	MemoryRecord *mem, *nmem, *pmem;

	std::vector<ESpell> real_spell(to_underlying(ESpell::kLast) + 1);
	auto count{0};
	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		real_spell[count] = spell_id;
		++count;
	}
	std::shuffle(real_spell.begin(), real_spell.end(), std::mt19937(std::random_device()()));

	for (const auto &character : character_list) {
		const auto i = character.get();

		if (i->IsNpc()) {
			i->inc_restore_timer(kSecsPerMudHour);
		}

		/* Если чар или моб попытался проснуться а на нем аффект сон,
		то он снова должен валиться в сон */
		if (AFF_FLAGGED(i, EAffect::kSleep)
			&& GET_POS(i) > EPosition::kSleep) {
			GET_POS(i) = EPosition::kSleep;
			SendMsgToChar("Вы попытались очнуться, но снова заснули и упали наземь.\r\n", i);
			act("$n попытал$u очнуться, но снова заснул$a и упал$a наземь.",
				true, i, nullptr, nullptr, kToRoom);
		}

		if (!i->IsNpc()) {
			gain_condition(i, DRUNK, -1);

			if (average_day_temp() < -20) {
				gain_condition(i, FULL, +2);
			} else if (average_day_temp() < -5) {
				gain_condition(i, FULL, number(+2, +1));
			} else {
				gain_condition(i, FULL, +1);
			}

			if (average_day_temp() > 25) {
				gain_condition(i, THIRST, +2);
			} else if (average_day_temp() > 20) {
				gain_condition(i, THIRST, number(+2, +1));
			} else {
				gain_condition(i, THIRST, +1);
			}
		}
	}

	character_list.foreach_on_copy([&](const auto &character) {
		const auto i = character.get();

		if (GET_POS(i) >= EPosition::kStun)    // Restore hit points
		{
			if (i->IsNpc()
				|| !UPDATE_PC_ON_BEAT) {
				const int count = hit_gain(i);
				if (GET_HIT(i) < GET_REAL_MAX_HIT(i)) {
					GET_HIT(i) = MIN(GET_HIT(i) + count, GET_REAL_MAX_HIT(i));
				}
			}

			// Restore mobs
			if (i->IsNpc()
				&& !i->GetEnemy())    // Restore horse
			{
				if (IS_HORSE(i)) {
					int mana = 0;

					switch (real_sector(IN_ROOM(i))) {
						case ESector::kOnlyFlying:
						case ESector::kUnderwater:
						case ESector::kSecret:
						case ESector::kWaterSwim:
						case ESector::kWaterNoswim:
						case ESector::kThickIce:
						case ESector::kNormalIce: [[fallthrough]];
						case ESector::kThinIce: mana = 0;
							break;
						case ESector::kCity: mana = 20;
							break;
						case ESector::kField: [[fallthrough]];
						case ESector::kFieldRain: mana = 100;
							break;
						case ESector::kFieldSnow: mana = 40;
							break;
						case ESector::kForest:
						case ESector::kForestRain: mana = 80;
							break;
						case ESector::kForestSnow: mana = 30;
							break;
						case ESector::kHills: [[fallthrough]];
						case ESector::kHillsRain: mana = 70;
							break;
						case ESector::kHillsSnow: [[fallthrough]];
						case ESector::kMountain: mana = 25;
							break;
						case ESector::kMountainSnow: mana = 10;
							break;
						default: mana = 10;
					}

					if (i->get_master()->IsOnHorse()) {
						mana /= 2;
					}

					GET_HORSESTATE(i) = std::min(800, GET_HORSESTATE(i) + mana);
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
				const auto mob_num = GET_MOB_RNUM(i);
				if (mob_num >= 0) {
					auto mana{0};
					auto count{0};
					const auto max_mana = GetRealInt(i) * 10;
					while (count <= to_underlying(ESpell::kLast) && mana < max_mana) {
						const auto spell_id = real_spell[count];
						if (GET_SPELL_MEM(mob_proto + mob_num, spell_id) > GET_SPELL_MEM(i, spell_id)) {
							GET_SPELL_MEM(i, spell_id)++;
							mana += ((MUD::Spell(spell_id).GetMaxMana() + MUD::Spell(spell_id).GetMinMana()) / 2);
							i->caster_level += (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE) ? 1 : 0);
						}

						++count;
					}
				}
			}

			// Restore moves
			if (i->IsNpc()
				|| !UPDATE_PC_ON_BEAT) {
				if (GET_MOVE(i) < GET_REAL_MAX_MOVE(i)) {
					GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_REAL_MAX_MOVE(i));
				}
			}

			// Update PC/NPC position
			if (GET_POS(i) <= EPosition::kStun) {
				update_pos(i);
			}
		} else if (GET_POS(i) == EPosition::kIncap) {
			Damage dmg(SimpleDmg(kTypeSuffering), 1, fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);

			if (dmg.Process(i, i) == -1) {
				return;
			}
		} else if (GET_POS(i) == EPosition::kPerish) {
			Damage dmg(SimpleDmg(kTypeSuffering), 2, fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);

			if (dmg.Process(i, i) == -1) {
				return;
			}
		}

		UpdateCharObjects(i);

		if (!i->IsNpc()
			&& GetRealLevel(i) < idle_max_level
			&& !PRF_FLAGGED(i, EPrf::kCoderinfo)) {
			check_idling(i);
		}
	});
}
void ExtractObjRepopDecay(const ObjData::shared_ptr obj) {
	if (obj->get_worn_by()) {
		act("$o рассыпал$U, вспыхнув ярким светом...",
			false, obj->get_worn_by(), obj.get(), nullptr, kToChar);
	} else if (obj->get_carried_by()) {
		act("$o рассыпал$U в ваших руках, вспыхнув ярким светом...",
				false, obj->get_carried_by(), obj.get(), nullptr, kToChar);
	} else if (obj->get_in_room() != kNowhere) {
		if (!world[obj->get_in_room()]->people.empty()) {
			act("$o рассыпал$U, вспыхнув ярким светом...",
					false, world[obj->get_in_room()]->first_character(), obj.get(), nullptr, kToChar);
			act("$o рассыпал$U, вспыхнув ярким светом...",
					false, world[obj->get_in_room()]->first_character(), obj.get(), nullptr, kToRoom);
		}
	} else if (obj->get_in_obj()) {
		CharData *owner = nullptr;
		if (obj->get_in_obj()->get_carried_by()) {
			owner = obj->get_in_obj()->get_carried_by();
		} else if (obj->get_in_obj()->get_worn_by()) {
			owner = obj->get_in_obj()->get_worn_by();
			}

		if (owner) {
			char buf[kMaxStringLength];
			snprintf(buf, kMaxStringLength, "$o рассыпал$U в %s...", obj->get_in_obj()->get_PName(5).c_str());
			act(buf, false, owner, obj.get(), nullptr, kToChar);
		}
	}
	ExtractObjFromWorld(obj.get());
}

void RepopDecay(std::vector<ZoneRnum> zone_list) {
	world_objects.foreach_on_copy([&](const ObjData::shared_ptr &j) {
		if (j->has_flag(EObjFlag::kRepopDecay)) {
			const ZoneVnum obj_zone_num = j->get_vnum() / 100;
			for (auto it = zone_list.begin(); it != zone_list.end(); ++it) {
				if (obj_zone_num == zone_table[*it].vnum) {
					ExtractObjRepopDecay(j);
				}
			}
		}
	});
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
