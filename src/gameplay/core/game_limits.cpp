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

#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/clans/house.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/mechanics/deathtrap.h"
#include "administration/ban.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/mechanics/glory.h"
#include "engine/entities/char_player.h"
#include "gameplay/fight/fight.h"
#include "gameplay/economics/ext_money.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/observability/helpers.h"
#include "engine/observability/metrics.h"
#include "utils/tracing/trace_manager.h"
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

#include <third_party_libs/fmt/include/fmt/format.h>

#include <random>

const int kRecallSpellsInterval = 28;

extern int check_dupes_host(DescriptorData *d, bool autocheck = false);
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
void decrease_level(CharData *ch);
int max_exp_gain_pc(CharData *ch);
int max_exp_loss_pc(CharData *ch);
int average_day_temp();

namespace {

void CollectCharmiceBoxesForIdleExtract(CharData *master) {
	if (!master || master->IsNpc()) {
		return;
	}

	const auto followers = master->get_followers_list();
	for (const auto follower : followers) {
		if (!follower || !IS_CHARMICE(follower) || follower->get_master() != master) {
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
			auto restore = int_app[GetRealInt(ch)].mana_per_tic;
			gain = graf(CalcCharAge(ch)->year, restore - 8, restore - 4, restore,
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
		percent *= ch->get_cond_penalty(P_MEM_GAIN);
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
		percent *= ch->get_cond_penalty(P_HIT_GAIN);
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
		percent *= ch->get_cond_penalty(P_HIT_GAIN);
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
	if (i->IsFlagged(EPlrFlag::kHelled) && HELL_DURATION(i) && HELL_DURATION(i) <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kHelled);
		if (HELL_REASON(i))
			free(HELL_REASON(i));
		HELL_REASON(i) = nullptr;
		GET_HELL_LEV(i) = 0;
		HELL_GODID(i) = 0;
		HELL_DURATION(i) = 0;
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
		look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	if (i->IsFlagged(EPlrFlag::kNameDenied)
		&& NAME_DURATION(i)
		&& NAME_DURATION(i) <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kNameDenied);
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
		look_at_room(i.get(), restore);
		act("С ревом \"Имья, сестра, имья...\", $n появил$u в центре комнаты.",
			false, i.get(), nullptr, nullptr, kToRoom);
	}

	if (i->IsFlagged(EPlrFlag::kMuted)
		&& MUTE_DURATION(i) != 0
		&& MUTE_DURATION(i) <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kMuted);
		if (MUTE_REASON(i))
			free(MUTE_REASON(i));
		MUTE_REASON(i) = nullptr;
		GET_MUTE_LEV(i) = 0;
		MUTE_GODID(i) = 0;
		MUTE_DURATION(i) = 0;
		SendMsgToChar("Вы можете орать.\r\n", i.get());
	}

	if (i->IsFlagged(EPlrFlag::kDumbed)
		&& DUMB_DURATION(i) != 0
		&& DUMB_DURATION(i) <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kDumbed);
		if (DUMB_REASON(i))
			free(DUMB_REASON(i));
		DUMB_REASON(i) = nullptr;
		GET_DUMB_LEV(i) = 0;
		DUMB_GODID(i) = 0;
		DUMB_DURATION(i) = 0;
		SendMsgToChar("Вы можете говорить.\r\n", i.get());
	}

	if (!i->IsFlagged(EPlrFlag::kRegistred)
		&& UNREG_DURATION(i) != 0
		&& UNREG_DURATION(i) <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kRegistred);
		if (UNREG_REASON(i))
			free(UNREG_REASON(i));
		UNREG_REASON(i) = nullptr;
		GET_UNREG_LEV(i) = 0;
		UNREG_GODID(i) = 0;
		UNREG_DURATION(i) = 0;
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
			look_at_room(i.get(), restore);

			act("$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!",
				false, i.get(), nullptr, nullptr, kToRoom);
		};

	}

	if (GET_GOD_FLAG(i, EGf::kGodsLike)
		&& GCURSE_DURATION(i) != 0
		&& GCURSE_DURATION(i) <= time(nullptr)) {
		REMOVE_BIT(i->player_specials->saved.GodsLike, EGf::kGodsLike);
		SendMsgToChar("Вы более не под защитой Богов.\r\n", i.get());
	}

	if (GET_GOD_FLAG(i, EGf::kGodscurse)
		&& GCURSE_DURATION(i) != 0
		&& GCURSE_DURATION(i) <= time(nullptr)) {
		REMOVE_BIT(i->player_specials->saved.GodsLike, EGf::kGodscurse);
		SendMsgToChar("Боги более не в обиде на вас.\r\n", i.get());
	}

	if (i->IsFlagged(EPlrFlag::kFrozen)
		&& FREEZE_DURATION(i) != 0
		&& FREEZE_DURATION(i) <= time(nullptr)) {
		i->UnsetFlag(EPlrFlag::kFrozen);
		if (FREEZE_REASON(i)) {
			free(FREEZE_REASON(i));
		}
		FREEZE_REASON(i) = nullptr;
		GET_FREEZE_LEV(i) = 0;
		FREEZE_GODID(i) = 0;
		FREEZE_DURATION(i) = 0;
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
		look_at_room(i.get(), restore);
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
				look_at_room(i.get(), r_helled_start_room);

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
				look_at_room(i.get(), r_named_start_room);

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
				look_at_room(i.get(), r_unreg_start_room);

				i->set_was_in_room(kNowhere);
			};
		} else if (restore == r_unreg_start_room && check_dupes_host(i->desc, true) && !i->IsImmortal()) {
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
			look_at_room(i.get(), restore);

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
				if (aff->type == ESpell::kBandage) {
					restore += std::min(d->character.get()->get_real_max_hit() / 10, aff->modifier);
					break;
				}
			}
		}
		if (AFF_FLAGGED(d->character.get(), EAffect::kFrenzy)) {
			for (const auto &aff : d->character->affected) {
				if (aff->type == ESpell::kFrenzy && aff->location == EApply::kHpRegen) {
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
		if (IS_MANA_CASTER(d->character.get()) && d->character->mem_queue.stored < mana[MIN(50, GetRealWis(d->character.get()))]) {
			d->character->mem_queue.stored += CalcManaGain(d->character.get());
			if (d->character->mem_queue.stored >= mana[MIN(50, GetRealWis(d->character.get()))]) {
				d->character->mem_queue.stored = mana[MIN(50, GetRealWis(d->character.get()))];
				SendMsgToChar("Ваша магическая энергия полностью восстановилась\r\n", d->character.get());
			}
		}

		if (IS_MANA_CASTER(d->character.get()) && d->character->mem_queue.stored > mana[MIN(50, GetRealWis(d->character.get()))]) {
			d->character->mem_queue.stored = mana[MIN(50, GetRealWis(d->character.get()))];
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
		ZoneExpStat::add(GetZoneVnumByCharPlace(ch), gain);
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
								  kColorBoldGrn, kColorNrm);
				} else {
					SendMsgToChar(ch,
								  "%sПоздравляем, вы набрали максимальное количество опыта!\r\n"
								  "%s%s\r\n", kColorBoldGrn, Remort::WHERE_TO_REMORT_STR.c_str(), kColorNrm);
				}
				SET_BIT(ch->player_specials->saved.GodsLike, EGf::kRemort);
			}
		}
		ch->set_exp(std::min(ch->get_exp(), GetExpUntilNextLvl(ch, kLvlImmortal) - 1));
		while (GetRealLevel(ch) < kLvlImmortal && ch->get_exp() >= GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
			ch->set_level(ch->GetLevel() + 1);
			num_levels++;
			sprintf(local_buf, "%sВы достигли следующего уровня!%s\r\n", kColorWht, kColorNrm);
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
		while (GetRealLevel(ch) > 1 && ch->get_exp() < GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
			ch->set_level(ch->GetLevel() - 1);
			num_levels++;
			sprintf(local_buf,
					"%sВы потеряли уровень. Вам должно быть стыдно!%s\r\n",
					kColorBoldRed, kColorNrm);
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
	if ((ch->get_exp() < GetExpUntilNextLvl(ch, kLvlImmortal) - 1)
		&& GET_GOD_FLAG(ch, EGf::kRemort)
		&& gain
		&& (GetRealLevel(ch) < kLvlImmortal)) {
		if (Remort::can_remort_now(ch)) {
			SendMsgToChar(ch, "%sВы потеряли право на перевоплощение!%s\r\n",
						  kColorBoldRed, kColorNrm);
		}
		REMOVE_BIT(ch->player_specials->saved.GodsLike, EGf::kRemort);
	}

	char_stat::AddClassExp(ch->GetClass(), gain);
	update_clan_exp(ch, gain);
}

// юзается исключительно в act.wizards.cpp в имм командах "advance" и "set exp".
// \todo Сделать файл с механикой опыта и все по опыту собрать туда. Вероятно, в core.
void gain_exp_regardless(CharData *ch, int gain) {
	int is_altered = false;
	int num_levels = 0;

	ch->set_exp(ch->get_exp() + gain);
	if (!ch->IsNpc()) {
		if (gain > 0) {
			while (GetRealLevel(ch) < kLvlImplementator && ch->get_exp() >= GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1)) {
				ch->set_level(ch->GetLevel() + 1);
				num_levels++;
				sprintf(buf, "%sВы достигли следующего уровня!%s\r\n",
						kColorWht, kColorNrm);
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
			//			gain = std::max(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
			//			ch->get_exp() += gain;
			//			if (ch->get_exp() < 0)
			//				ch->get_exp() = 0;
			while (GetRealLevel(ch) > 1 && ch->get_exp() < GetExpUntilNextLvl(ch, GetRealLevel(ch))) {
				ch->set_level(ch->GetLevel() - 1);
				num_levels++;
				sprintf(buf,
						"%sВы потеряли уровень!%s\r\n",
						kColorBoldRed, kColorNrm);
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

	if (ch->IsGod() && condition != DRUNK) {
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

	if (ch->IsFlagged(EPlrFlag::kWriting))
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
			&& !d->character->IsGod()
			&& !AFF_FLAGGED(d->character, EAffect::kWaterBreath)) {
			sprintf(buf, "Player %s died under water (room %d)",
					GET_NAME(d->character), GET_ROOM_VNUM(d->character->in_room));

			Damage dmg(SimpleDmg(kTypeWaterdeath), std::max(1, d->character->get_real_max_hit() >> 2), fight::kUndefDmg);
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
				RemoveCharFromRoom(ch);
				PlaceCharToRoom(ch, kStrangeRoom);
				room_spells::RemoveSingleAffectFromWorld(ch, ESpell::kRuneLabel);
			} else if (ch->char_specials.timer > idle_rent_time) {
				if (ch->in_room != kNowhere)
					RemoveCharFromRoom(ch);
				PlaceCharToRoom(ch, kStrangeRoom);
				CollectCharmiceBoxesForIdleExtract(ch);
				Crash_idlesave(ch);
				Depot::exit_char(ch);
				Clan::clan_invoice(ch, false);
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
		|| (obj->has_flag(EObjFlag::kNodecay)
			&& !ROOM_FLAGGED(obj->get_in_room(), ERoomFlag::kDeathTrap)));
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
				act("Костер затух.",
					false, world[count]->first_character(), nullptr, nullptr, kToRoom);
				act("Костер затух.",
					false, world[count]->first_character(), nullptr, nullptr, kToChar);

				world[count]->fires = 0;
			}
		}
/*
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
*/
		if (world[count]->holes) {
			world[count]->holes--;
			if (!world[count]->holes || round_up(world[count]->holes) == world[count]->holes) {
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
			std::string cap = GET_EXCHANGE_ITEM(exch_item)->get_PName(ECase::kNom);
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
			&& d->state == EConState::kPlaying
			&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
			&& d->character->IsFlagged(EPrf::kDecayMode)
			&& CLAN(d->character)
			&& CLAN(d->character)->GetRent() == room) {
			SendMsgToChar(d->character.get(), "[Хранилище]: %s'%s%s рассыпал%s в прах'%s\r\n",
						  kColorBoldRed,
						  j->get_short_description().c_str(),
						  clan_get_custom_label(j, CLAN(d->character)).c_str(),
						  GET_OBJ_SUF_2(j),
						  kColorNrm);
		}
	}

	for (const auto &i : Clan::ClanList) {
		if (i->GetRent() == room) {
			std::string log_text = fmt::format("{}{} рассыпал{} в прах\r\n",
												  j->get_short_description(),
												  clan_get_custom_label(j, i),
												  GET_OBJ_SUF_2(j));
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
				 obj->get_in_obj()->get_PName(ECase::kPre).c_str(),
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
	std::string cap = obj->get_PName(ECase::kNom);
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
				  kColorBoldCyn,
				  utils::CAP(local_buf),
				  kColorNrm);
}

void obj_point_update() {
	std::list<ObjData *> obj_destroy;
	std::list<ObjData *> obj_decay_timer;

	log("!!!obj_point_update!!!");
	world_objects.foreach([&obj_destroy, &obj_decay_timer](const ObjData::shared_ptr &j) {
		if (j->get_where_obj() == EWhereObj::kSeller) {
			return;
		}
		if (CheckObjDecay(j.get(), false)) {
			obj_destroy.push_back(j.get());
			return;
		}
		if (j->get_destroyer() > 0 && !NO_DESTROY(j.get())) {
			j->dec_destroyer();
		}
		if (j->get_timer() > 0 && !NO_TIMER(j.get())) {
			j->dec_timer();
		}
		if (j->get_destroyer() == 0
				|| j->get_timer() == 0
				|| (j->has_flag(EObjFlag::kZonedecay)
						&& j->get_vnum_zone_from()
						&& up_obj_where(j.get()) != kNowhere
						&& j->get_vnum_zone_from() != zone_table[world[up_obj_where(j.get())]->zone_rn].vnum)) {
			obj_decay_timer.push_back(j.get());
		}
// п╟ я│ п╨п╟п╨п╦я┘ п©п╬я─ я┐ п╫п╟я│ п╫п╟ я┬п╪п╬я┌п╨я┐ EApply::kPoison п╡п╣я┬п╟п╣я┌я│я▐?
/*		else {
			for (int count = 0; count < kMaxObjAffect; count++) {
				if (j->get_affected(count).location == EApply::kPoison) {
					j->dec_affected_value(count);
					if (j->get_affected(count).modifier <= 0) {
						j->set_affected(count, EApply::kNone, 0);
					}
				}
			}
		}
*/
	});
	for (auto it : obj_destroy) {
		ExtractObjFromWorld(it);
	}
	Parcel::update_timers();
	Depot::update_timers();
	exchange_point_update();
	for (auto j : obj_decay_timer) {
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
							jj->get_PName(ECase::kNom).c_str(), j->get_PName(ECase::kNom).c_str());
						// core_dump();
						ExtractObjFromWorld(jj);
					}
				}
				if (j->get_carried_by()) {
					act("$p я─п╟я│я│я▀п©п╟п╩$U п╡ п╡п╟я┬п╦я┘ я─я┐п╨п╟я┘.",
						false, j->get_carried_by(), j, nullptr, kToChar);
					RemoveObjFromChar(j);
				} else if (j->get_in_room() != kNowhere) {
					if (!world[j->get_in_room()]->people.empty()) {
						act("п╖п╣я─п╡п╦ п©п╬п╩п╫п╬я│я┌я▄я▌ я│п╬п╤я─п╟п╩п╦ $o3.",
							true, world[j->get_in_room()]->first_character(), j, nullptr, kToRoom);
						act("п╖п╣я─п╡п╦ п╫п╣ п╬я│я┌п╟п╡п╦п╩п╦ п╬я┌ $o1 п╦ я│п╩п╣п╢п╟.",
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
					return;
			}
			// *** я─п╟я│я│я▀п©п╟п╫п╦п╣ п╬п╠я┼п╣п╨я┌п╟
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
						jj->get_PName(ECase::kNom).c_str(),
						j->get_PName(ECase::kNom).c_str());
					// core_dump();
					ExtractObjFromWorld(jj);
				}
			}
			// п п╬п╫п╣я├ п⌡п╟п╢п╫п╦п╨
			if (j->get_worn_by()) {
				switch (j->get_worn_on()) {
					case EEquipPos::kLight:
					case EEquipPos::kShield:
					case EEquipPos::kWield:
					case EEquipPos::kHold:
					case EEquipPos::kBoths:
						if (IS_CHARMICE(j->get_worn_by())) {
							charmee_obj_decay_tell(j->get_worn_by(), j, ECharmeeObjPos::kHandsOrEquip);
						} else {
							snprintf(buf, kMaxStringLength, "$o%s я─п╟я│я│я▀п©п╟п╩$U п╡ п╡п╟я┬п╦я┘ я─я┐п╨п╟я┘...",
									 char_get_custom_label(j, j->get_worn_by()).c_str());
							act(buf, false, j->get_worn_by(), j, nullptr, kToChar);
						}
						break;
						default:
						if (IS_CHARMICE(j->get_worn_by())) {
							charmee_obj_decay_tell(j->get_worn_by(), j, ECharmeeObjPos::kInventory);
						} else {
							snprintf(buf, kMaxStringLength, "$o%s я─п╟я│я│я▀п©п╟п╩$U п©я─я▐п╪п╬ п╫п╟ п╡п╟я│...",
									 char_get_custom_label(j, j->get_worn_by()).c_str());
							act(buf, false, j->get_worn_by(), j, nullptr, kToChar);
						}
						break;
				}
				UnequipChar(j->get_worn_by(), j->get_worn_on(), CharEquipFlags());
			} else if (j->get_carried_by()) {
				if (IS_CHARMICE(j->get_carried_by())) {
					charmee_obj_decay_tell(j->get_carried_by(), j, ECharmeeObjPos::kHandsOrEquip);
				} else {
					snprintf(buf, kMaxStringLength, "$o%s я─п╟я│я│я▀п©п╟п╩$U п╡ п╡п╟я┬п╦я┘ я─я┐п╨п╟я┘...",
							 char_get_custom_label(j, j->get_carried_by()).c_str());
					act(buf, false, j->get_carried_by(), j, nullptr, kToChar);
				}
				RemoveObjFromChar(j);
			} else if (j->get_in_room() != kNowhere) {
				if (!world[j->get_in_room()]->people.empty()) {
					act("$o я─п╟я│я│я▀п©п╟п╩$U п╡ п©я─п╟я┘, п╨п╬я┌п╬я─я▀п╧ п╠я▀п╩ я─п╟п╥п╡п╣я▐п╫ п╡п╣я┌я─п╬п╪...",
						false, world[j->get_in_room()]->first_character(), j, nullptr, kToChar);
					act("$o я─п╟я│я│я▀п©п╟п╩$U п╡ п©я─п╟я┘, п╨п╬я┌п╬я─я▀п╧ п╠я▀п╩ я─п╟п╥п╡п╣я▐п╫ п╡п╣я┌я─п╬п╪...",
						false, world[j->get_in_room()]->first_character(), j, nullptr, kToRoom);
				}
			} else if (j->get_in_obj()) {
				// п╣я│п╩п╦ я│я▀п©п╣я┌я│я▐ п╡ п╫п╟я┘п╬п╢я▐я┴п╣п╪я│я▐ я┐ я┤п╟я─п╟ п╦п╩п╦ я┤п╟я─п╪п╦я│п╟ п╨п╬п╫я┌п╣п╧п╫п╣я─п╣, я┌п╬ п╬п╠ я█я┌п╬п╪ я┌п╬п╤п╣ я│п╬п╬п╠я┴п╟п╣п╪
				CharData *cont_owner = nullptr;
				if (j->get_in_obj()->get_carried_by()) {
					cont_owner = j->get_in_obj()->get_carried_by();
				} else if (j->get_in_obj()->get_worn_by()) {
					cont_owner = j->get_in_obj()->get_worn_by();
				}
				if (cont_owner) {
					if (IS_CHARMICE(cont_owner)) {
						charmee_obj_decay_tell(cont_owner, j, ECharmeeObjPos::kContainer);
					} else {
							char buf[kMaxStringLength];
							snprintf(buf, kMaxStringLength, "$o%s я─п╟я│я│я▀п©п╟п╩$U п╡ %s%s...",
									 char_get_custom_label(j, cont_owner).c_str(),
									 j->get_in_obj()->get_PName(ECase::kPre).c_str(),
									 char_get_custom_label(j->get_in_obj(), cont_owner).c_str());
							act(buf, false, cont_owner, j, nullptr, kToChar);
					}
				}
				RemoveObjFromObj(j);
			}
			ExtractObjFromWorld(j);
		}
	}
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

	for (auto &ch : character_list) {
		const auto i = ch.get();

		if (i->purged() || (i->IsNpc() && !i->in_used_zone())) {
			continue;
	  	}

		if (i->IsNpc()) {
			i->inc_restore_timer(kSecsPerMudHour);
		}
		/* Если чар или моб попытался проснуться а на нем аффект сон,
		то он снова должен валиться в сон */
		if (AFF_FLAGGED(i, EAffect::kSleep) && i->GetPosition() > EPosition::kSleep) {
			i->SetPosition(EPosition::kSleep);
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
			UpdateCharObjects(i);
		}
		if (i->GetPosition() >= EPosition::kStun)    // Restore hit points
		{
			if (i->IsNpc() || !UPDATE_PC_ON_BEAT) {
				const int count = hit_gain(i);

				if (i->get_hit() < i->get_real_max_hit()) {
					i->set_hit(std::min(i->get_hit() + count, i->get_real_max_hit()));
				}
			}
			// Restore mobs
			if (i->IsNpc() && !i->GetEnemy())    // Restore horse
			{
				if (IS_HORSE(i)) {
					int mana = 0;
					switch (real_sector(i->in_room)) {
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
				if (i->mob_specials.have_spell) {
					auto mana{0};
					auto count{0};
					const auto max_mana = GetRealInt(i) * 10;
					while (count <= to_underlying(ESpell::kLast) && mana < max_mana) {
						const auto spell_id = real_spell[count];
						if (GET_SPELL_MEM(i, spell_id) > GET_SPELL_MEM(i, spell_id)) {
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
				if (i->get_move() < i->get_real_max_move()) {
					i->set_move(std::min(i->get_move() + move_gain(i), i->get_real_max_move()));
				}
			}
		} else if (i->GetPosition() == EPosition::kIncap) {
			i->points.hit += 1;
			act("$n, пуская слюни, забил$u в судорогах.", true, i, nullptr, nullptr, kToRoom | kToArenaListen);
		} else if (i->GetPosition() == EPosition::kPerish) {
			act("$n, пуская слюни, забил$u в судорогах.", true, i, nullptr, nullptr, kToRoom | kToArenaListen);
			i->points.hit += 2;
		}
		update_pos(i);
		if (!i->IsNpc()
			&& GetRealLevel(i) < idle_max_level
			&& !i->IsFlagged(EPrf::kCoderinfo)) {
			check_idling(i);
		}
	}
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
			const auto msg = fmt::format("$o рассыпал$U в {}...", obj->get_in_obj()->get_PName(ECase::kPre));
			act(msg, false, owner, obj, nullptr, kToChar);
		}
	}
	ExtractObjFromWorld(obj);
}

void DecayObjectsOnRepop(UniqueList<ZoneRnum> &zone_list) {
	std::list<ObjData *> extract_list;

	for (auto j : world_objects) {
		if (j->has_flag(EObjFlag::kRepopDecay)) {
			const ZoneVnum obj_zone_num = j->get_vnum() / 100;
			for (auto &it : zone_list) {
				if (obj_zone_num == zone_table[it].vnum) {
					extract_list.push_back(j.get());
				}
			}
		}
	}
	for (auto it : extract_list) {
		ExtractRepopDecayObject(it);
	}
}

void gain_battle_exp(CharData *ch, CharData *victim, int dam) {
	// не даем получать батлу с себя по зеркалу?
	if (ch == victim) {
		return;
	}
	if (!victim->IsNpc()) { return; }
	// не даем получать экспу с !эксп мобов
	if (victim->IsFlagged(EMobFlag::kNoBattleExp)) {
		return;
	}
	// если цель не нпс то тоже не даем экспы
	// если цель под чармом не даем экспу
	if (AFF_FLAGGED(victim, EAffect::kCharmed)) {
		return;
	}
	// получение игроками экспы
	if (!ch->IsNpc() && OK_GAIN_EXP(ch, victim)) {
		int max_exp = std::min(max_exp_gain_pc(ch), (GetRealLevel(victim) * victim->get_max_hit() + 4) /
			(5 * std::max(1, GetRealRemort(ch) - kMaxExpCoefficientsUsed - 1)));
		double coeff = std::min(dam, victim->get_hit()) / static_cast<double>(victim->get_max_hit());
		int battle_exp = std::max(1, static_cast<int>(max_exp * coeff));
		if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_WEAPON_EXP) && Bonus::can_get_bonus_exp(ch)) {
			battle_exp *= Bonus::get_mult_bonus();
		}
		EndowExpToChar(ch, battle_exp);
		ch->dps_add_exp(battle_exp, true);
	}

	// перенаправляем батлэкспу чармиса в хозяина, цифры те же что и у файтеров.
	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed)) {
		CharData *master = ch->get_master();
		// проверяем что есть мастер и он может получать экспу с данной цели
		if (master && OK_GAIN_EXP(master, victim)) {
			int max_exp = std::min(max_exp_gain_pc(master), (GetRealLevel(victim) * victim->get_max_hit() + 4) /
				(5 * std::max(1, GetRealRemort(master) - kMaxExpCoefficientsUsed - 1)));

			double coeff = std::min(dam, victim->get_hit()) / static_cast<double>(victim->get_max_hit());
			int battle_exp = std::max(1, static_cast<int>(max_exp * coeff));
			if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_WEAPON_EXP) && Bonus::can_get_bonus_exp(master)) {
				battle_exp *= Bonus::get_mult_bonus();
			}
			EndowExpToChar(master, battle_exp);
			master->dps_add_exp(battle_exp, true);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
