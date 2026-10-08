/**
 \file bone_commander.cpp - a part of the Bylins engine.
 \brief issue #4000: что костяные скелеты чернокнижника делают своим ударом.
 \details У прототипов скелетов есть врождённый аффект kBoneServant, его действие стреляет на
		  kPostHit и зовёт отсюда SkeletonHit. По виду скелета (ярус из animate_dead.xml) удар
		  кладёт на жертву свой дебафф: свирепый разъедает доспех (физ. защита), зловонный --
		  оберег (маг. защита), ловкий режет (порезы, после порога жертва не переключается).
		  Сила дебаффа считается в момент наложения по числу живых скелетов хозяина: один скелет --
		  12%, два -- 10%, три -- 8%; порогов порезов соответственно 4, 6 и 8. Пересчитывать уже
		  поднятых не нужно -- погиб один, у остальных дебафф сразу крепче.
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/handlers/bone_commander.h"

#include "engine/entities/char_data.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_handler.h"   // RemoveAffect
#include "gameplay/mechanics/animate_dead.h"
#include "engine/db/global_objects.h"
#include "utils/utils.h"

#include <cmath>
#include <vector>

namespace {

// Сколько скелетов из костяной свиты сейчас живо у хозяина (включая того, кто ударил).
int CountBoneServants(CharData *master) {
	if (!master) {
		return 1;
	}
	int count = 0;
	for (const auto *follower : master->followers) {
		if (follower->IsNpc() && AFF_FLAGGED(follower, EAffect::kBoneServant)) {
			++count;
		}
	}
	return std::max(1, count);
}

// Накопить дебафф на жертве: свой стак добавляется, пока не достигнут потолок. Прибавку правим на
// месте и перекладываем аффект заново -- affect_total считает итоги по списку, иначе правка не
// попадёт в статы (тот же приём, что у ядов в poison_affect_join).
// Возвращает true, если дебафф лёг на жертву впервые -- по этому вызывающий решает, писать ли
// сообщение в комнату: на каждый удар писать нельзя, потоком зальёт бой.
bool StackDebuff(CharData *mob, CharData *victim, EAffect type, EApply location,
				 int per_stack, int max_stacks, int duration) {
	for (auto affect_i = victim->affected.begin(); affect_i != victim->affected.end(); ++affect_i) {
		const auto affect = *affect_i;
		if (affect->affect_type != type || affect->location != location) {
			continue;
		}
		if (affect->stacks < max_stacks) {
			++affect->stacks;
			affect->modifier += per_stack;
		} else {
			// Потолок мог опуститься (подняли ещё скелета) -- подтянем прибавку к новому.
			affect->stacks = max_stacks;
			affect->modifier = per_stack * max_stacks;
		}
		affect->duration = duration;
		RemoveAffect(victim, affect_i);
		affect_to_char(victim, *affect);
		return false;
	}

	Affect<EApply> af;
	af.affect_type = type;
	af.location = location;
	af.modifier = per_stack;
	af.stacks = 1;
	af.duration = duration;
	af.caster_id = mob->get_uid();
	af.battleflag = {kAfBattledec, kAfCurable};
	affect_to_char(victim, af);
	return true;
}

// Сколько стаков порезов уже на жертве.
int CountLacerations(CharData *victim) {
	for (const auto &affect : victim->affected) {
		if (affect->affect_type == EAffect::kLacerations) {
			return affect->stacks;
		}
	}
	return 0;
}

}  // namespace

namespace handlers {

EStageResult SkeletonHit(ActionContext &ctx) {
	CharData *mob = ctx.caster();           // носитель метки -- сам скелет
	if (!mob || !mob->IsNpc()) {
		return EStageResult::kSuccess;
	}
	CharData *victim = ctx.Event().actor;
	if (!victim || victim->purged() || victim == mob) {
		return EStageResult::kSuccess;
	}
	// Против игроков дебаффы защит не кладём (решение по #4000), а запрет переключения на них всё
	// равно не действует: гейт стоит только на мобском переключении.
	if (!victim->IsNpc()) {
		return EStageResult::kSuccess;
	}

	const auto *tier = MUD::AnimateDead().ByProtoVnum(GET_MOB_VNUM(mob));
	if (!tier) {
		return EStageResult::kSuccess;
	}

	const int servants = CountBoneServants(mob->get_master());
	// Один скелет бьёт глубже, трое -- слабее каждый: 12 / 10 / 8 процентов и 4 / 6 / 8 порезов.
	const int depth = std::max(8, 14 - 2 * servants);
	const int cuts_needed = std::min(8, 2 + 2 * servants);
	const int duration = 10;

	if (tier->id == "kFierceSkeleton") {
		if (StackDebuff(mob, victim, EAffect::kCorrodedArmor, EApply::kPhysicResist, -1, depth, duration)) {
			act("Ядовитые когти $n1 разъедают доспехи $N1.",
				false, mob, nullptr, victim, kToRoom | kToArenaListen);
		}
	} else if (tier->id == "kFetidSkeleton") {
		if (StackDebuff(mob, victim, EAffect::kCorrodedWard, EApply::kMagicResist, -1, depth, duration)) {
			act("Смрадное дыхание $n1 разъедает оберег $N1.",
				false, mob, nullptr, victim, kToRoom | kToArenaListen);
		}
	} else if (tier->id == "kNimbleSkeleton") {
		// Порезы копятся по удару; набралось нужное число -- жертва обессилела и не переключается.
		StackDebuff(mob, victim, EAffect::kLacerations, EApply::kNone, 0, cuts_needed, duration);
		if (CountLacerations(victim) >= cuts_needed && !AFF_FLAGGED(victim, EAffect::kNoBattleSwitch)) {
			Affect<EApply> af;
			af.affect_type = EAffect::kNoBattleSwitch;
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = duration;
			af.caster_id = mob->get_uid();
			af.battleflag = {kAfBattledec, kAfCurable};
			affect_to_char(victim, af);
			act("$n затих$q, не в силах оторваться от своего противника.",
				false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
		}
	}

	return EStageResult::kSuccess;
}

}  // namespace handlers

namespace bone_commander {

void RefreshLevy(CharData *master, const CharData *leaving) {
	if (!master || master->purged()) {
		return;
	}
	// Сумма долей живых скелетов -- сколько процентов урона свита забирает у нежити, и список самой
	// нежити, между которой эта цена делится. Слабеет только нежить от "поднять труп": у её ярусов
	// spell == kAnimateDead. Умертвие от "оживить труп" поднимается по внуму трупа и в ярусах не
	// числится, его это не касается.
	double share = 0.0;
	std::vector<CharData *> undead;
	for (auto *follower : master->followers) {
		if (!follower->IsNpc() || follower->purged() || follower == leaving) {
			continue;
		}
		const auto *tier = MUD::AnimateDead().ByProtoVnum(GET_MOB_VNUM(follower));
		if (!tier) {
			continue;
		}
		if (AFF_FLAGGED(follower, EAffect::kBoneServant)) {
			share += tier->damage_share;
		} else if (tier->spell == ESpell::kAnimateDead) {
			undead.push_back(follower);
		}
	}
	// Подать платится один раз со всей свиты: цену делим между нежитью, а не берём с каждой целиком.
	const int levy = undead.empty() ? 0 : static_cast<int>(std::lround(share / undead.size()));

	for (auto *follower : undead) {
		int had = 0;
		for (const auto &af : follower->affected) {
			if (af->affect_type == EAffect::kBoneLevy && af->location == EApply::kPhysicDamagePercent) {
				had = -af->modifier;
				break;
			}
		}
		if (had == levy) {
			continue;   // ничего не поменялось -- не трогаем и не сообщаем
		}
		if (had > 0) {
			RemoveAffectFromCharAndRecalculate(follower, EAffect::kBoneLevy);
		}
		if (levy > 0) {
			// Срок не тикает: подать держится кодом и живёт, пока жива свита.
			Affect<EApply> af;
			af.affect_type = EAffect::kBoneLevy;
			af.duration = -1;
			af.caster_id = master->get_uid();
			af.battleflag = {kAfDeadkeep};
			af.modifier = -levy;
			af.location = EApply::kPhysicDamagePercent;
			affect_to_char(follower, af);
			af.location = EApply::kMagicDamagePercent;
			affect_to_char(follower, af);
		}
		if (levy > had) {
			act("$n поник$q, отдав часть своей силы костяной свите.",
				false, follower, nullptr, nullptr, kToRoom | kToArenaListen);
		} else if (levy < had) {
			act("$n расправил$u: костяная свита убыла, и сила вернулась.",
				false, follower, nullptr, nullptr, kToRoom | kToArenaListen);
		}
	}
}

}  // namespace bone_commander

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
