// обслуживание функций езды на всяческих жовтоне
//
#include "mount.h"
#include "administration/privilege.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/char_handler.h"
#include "engine/core/char_movement.h"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "gameplay/skills/skills.h"
#include "gameplay/mechanics/saving.h"

#include <algorithm>

namespace mount {

CharData *GetHorse(CharData *ch) {
	if (ch->IsNpc()) {
		return nullptr;
	}
	for (auto *f : ch->followers) {
		if (f->IsNpc() && AFF_FLAGGED(f, EAffect::kHorse)) {
			return f;
		}
	}
	return nullptr;
}

bool HasHorse(const CharData *ch, bool same_room) {
	if (ch->IsNpc()) {
		return false;
	}
	for (auto *f : ch->followers) {
		if (f->IsNpc() && AFF_FLAGGED(f, EAffect::kHorse)
			&& (!same_room || ch->in_room == f->in_room)) {
			return true;
		}
	}
	return false;
}

bool IsOnHorse(const CharData *ch) {
	return AFF_FLAGGED(ch, EAffect::kHorse) && HasHorse(ch, true);
}

bool IsHorse(const CharData *ch) {
	return ch->IsNpc() && ch->has_master() && AFF_FLAGGED(ch, EAffect::kHorse);
}

bool IsHorse(const std::shared_ptr<CharData> &ch) { return IsHorse(ch.get()); }

bool DropFromHorse(CharData *ch) {
	CharData *plr;
	// вызвали для лошади
	if (IsHorse(ch) && IsOnHorse(ch->get_master())) {
		plr = ch->get_master();
		act("$N сбросил$G вас со своей спины.", false, plr, 0, ch, kToChar);
	} else if (IsOnHorse(ch)) {  // вызвали для седока
		plr = ch;
		act("Вы упали с $N1.", false, plr, 0, GetHorse(ch), kToChar);
	} else {  // не лошадь и не всадник
		return false;
	}
	sprintf(buf, "%s свалил%s со своего скакуна.", GET_PAD(plr, 0), grammar::SexEnding((plr)->get_sex(), 2));
	act(buf, false, plr, 0, 0, kToRoom | kToArenaListen);
	AFF_FLAGS(plr).unset(EAffect::kHorse);
	SetBattleLag(plr, 3);
	if (plr->GetPosition() > EPosition::kSit) {
		plr->SetPosition(EPosition::kSit);
	}
	return true;
}

void Dismount(CharData *ch) {
	if (!IsOnHorse(ch) || GetHorse(ch) == nullptr) {
		return;
	}
	if (!ch->IsNpc() && HasHorse(ch, true)) {
		AFF_FLAGS(ch).unset(EAffect::kHorse);
	}
	act("Вы слезли со спины $N1.", false, ch, 0, GetHorse(ch), kToChar);
	act("$n соскочил$g с $N1.", false, ch, 0, GetHorse(ch), kToRoom | kToArenaListen);
}

bool IsBlockedByHorse(CharData *ch) {
	if (IsOnHorse(ch)) {
		act("Вам мешает $N.", false, ch, 0, GetHorse(ch), kToChar);
		return true;
	}
	return false;
}

void ApplyRiderToHit(CharData *ch, CharData *victim, int &calc_thaco) {
	TrainSkill(ch, ESkill::kRiding, true, victim);
	calc_thaco += 10 - GetSkill(ch, ESkill::kRiding) / 20;
}

void ApplyRiderHitAndDamage(CharData *ch, int &dam, int &calc_thaco) {
	const int prob = GetSkill(ch, ESkill::kRiding);
	const int range = MUD::Skill(ESkill::kRiding).difficulty / 2;
	dam += (prob + 19) / 10;
	if (range > prob) {
		calc_thaco += (range - prob) + 19 / 20;
	} else {
		calc_thaco -= (prob - range) + 19 / 20;
	}
}

void ApplyRiderDamageMult(CharData *ch, int &dam) {
	if (IsOnHorse(ch) && GetSkill(ch, ESkill::kRiding) > 100) {
		dam *= 1.0 + (GetSkill(ch, ESkill::kRiding) - 100) / 500.0;
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага с учетом лошади (при скилле 200 +20 процентов)== %d&n\r\n", dam);
	}
}

int SavingModifier(const CharData *ch, ESaving saving) {
	if (!IsOnHorse(ch)) {
		return 0;
	}
	switch (saving) {
		case ESaving::kReflex: return 20;
		case ESaving::kStability: return -20 - GetSkill(ch, ESkill::kRiding) / 25;
		default: return 0;
	}
}

void RestoreHorseState(CharData *horse) {
	int mana = 0;
	switch (real_sector(horse->in_room)) {
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
	if (IsOnHorse(horse->get_master())) {
		mana /= 2;
	}
	GET_HORSESTATE(horse) = std::min(800, GET_HORSESTATE(horse) + mana);
}

}  // namespace mount

void mount::MakeHorse(CharData *horse, CharData *ch) {
	AFF_FLAGS(horse).set(EAffect::kHorse);
	follow::AddFollower(ch, horse);
	horse->UnsetFlag(EMobFlag::kWimpy);
	horse->UnsetFlag(EMobFlag::kSentinel);
	horse->UnsetFlag(EMobFlag::kHelper);
	horse->UnsetFlag(EMobFlag::kAgressive);
	horse->UnsetFlag(EMobFlag::kMounting);
	AFF_FLAGS(horse).unset(EAffect::kTethered);
}

void do_horseon(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *horse;

	if (ch->IsNpc())
		return;

	if (!mount::GetHorse(ch)) {
		SendMsgToChar("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Не пытайтесь усидеть на двух стульях.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
		horse = target_resolver::FindCharInRoom(ch, arg);
	else
		horse = mount::GetHorse(ch);

	if (horse == nullptr)
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	else if (horse->in_room != ch->in_room)
		SendMsgToChar("Ваш скакун далеко от вас.\r\n", ch);
	else if (!mount::IsHorse(horse))
		SendMsgToChar("Это не скакун.\r\n", ch);
	else if (horse->get_master() != ch)
		SendMsgToChar("Это не ваш скакун.\r\n", ch);
	else if (horse->GetPosition() < EPosition::kFight)
		act("$N не сможет вас нести в таком состоянии.", false, ch, 0, horse, kToChar);
	else if (AFF_FLAGGED(horse, EAffect::kTethered))
		act("Вам стоит отвязать $N3.", false, ch, 0, horse, kToChar);
		//чтоб не вскакивали в ванрумах
	else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kTunnel))
		SendMsgToChar("Слишком мало места.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNohorse))
		act("$Z $N взбрыкнул$G и отказал$U вас слушаться.", false, ch, 0, horse, kToChar);
	else {
		if (IsAffectedFlagOnly(ch, EAffect::kSneak))
			RemoveAffectFromCharAndRecalculate(ch, EAffect::kSneak);
		if (IsAffectedFlagOnly(ch, EAffect::kDisguise))
			RemoveAffectFromCharAndRecalculate(ch, EAffect::kDisguise);
		act("Вы взобрались на спину $N1.", false, ch, 0, horse, kToChar);
		act("$n вскочил$g на $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
		AFF_FLAGS(ch).set(EAffect::kHorse);
	}
}

void do_horseoff(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *horse;

	if (ch->IsNpc())
		return;
	if (!(horse = mount::GetHorse(ch))) {
		SendMsgToChar("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (!mount::IsOnHorse(ch)) {
		SendMsgToChar("Вы ведь и так не на лошади.\r\n", ch);
		return;
	}
	mount::Dismount(ch);

}

void do_horseget(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *horse;

	if (ch->IsNpc())
		return;

	if (!mount::GetHorse(ch)) {
		SendMsgToChar("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Вы уже сидите на скакуне.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
		horse = target_resolver::FindCharInRoom(ch, arg);
	else
		horse = mount::GetHorse(ch);

	if (horse == nullptr)
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	else if (horse->in_room != ch->in_room)
		SendMsgToChar("Ваш скакун далеко от вас.\r\n", ch);
	else if (!mount::IsHorse(horse))
		SendMsgToChar("Это не скакун.\r\n", ch);
	else if (horse->get_master() != ch)
		SendMsgToChar("Это не ваш скакун.\r\n", ch);
	else if (!AFF_FLAGGED(horse, EAffect::kTethered))
		act("А $N и не привязан$A.", false, ch, 0, horse, kToChar);
	else {
		act("Вы отвязали $N3.", false, ch, 0, horse, kToChar);
		act("$n отвязал$g $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
		AFF_FLAGS(horse).unset(EAffect::kTethered);
	}
}

void do_horseput(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *horse;

	if (ch->IsNpc())
		return;
	if (!mount::GetHorse(ch)) {
		SendMsgToChar("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Вам стоит слезть со скакуна.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
		horse = target_resolver::FindCharInRoom(ch, arg);
	else
		horse = mount::GetHorse(ch);
	if (horse == nullptr)
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	else if (horse->in_room != ch->in_room)
		SendMsgToChar("Ваш скакун далеко от вас.\r\n", ch);
	else if (!mount::IsHorse(horse))
		SendMsgToChar("Это не скакун.\r\n", ch);
	else if (horse->get_master() != ch)
		SendMsgToChar("Это не ваш скакун.\r\n", ch);
	else if (AFF_FLAGGED(horse, EAffect::kTethered))
		act("А $N уже и так привязан$A.", false, ch, 0, horse, kToChar);
	else {
		act("Вы привязали $N3.", false, ch, 0, horse, kToChar);
		act("$n привязал$g $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
		AFF_FLAGS(horse).set(EAffect::kTethered);
	}
}

void do_horsetake(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *horse = nullptr;

	if (ch->IsNpc())
		return;

	if (mount::GetHorse(ch)) {
		SendMsgToChar("Зачем вам столько скакунов?\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg) {
		horse = target_resolver::FindCharInRoom(ch, arg);
	}

	if (horse == nullptr) {
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
		return;
	} else if (!horse->IsNpc()) {
		SendMsgToChar("Господи, не чуди...\r\n", ch);
		return;
	}
		// Исправил ошибку не дававшую воровать коняжек. -- Четырь (13.10.10)
	else if (!privilege::IsGod(ch)
		&& !horse->IsFlagged(EMobFlag::kMounting)
		&& !(horse->has_master()
			&& AFF_FLAGGED(horse, EAffect::kHorse))) {
		act("Вы не сможете оседлать $N3.", false, ch, 0, horse, kToChar);
		return;
	} else if (mount::GetHorse(ch)) {
		if (mount::GetHorse(ch) == horse)
			act("Не стоит седлать $S еще раз.", false, ch, 0, horse, kToChar);
		else
			SendMsgToChar("Вам не усидеть сразу на двух скакунах.\r\n", ch);
		return;
	} else if (horse->GetPosition() < EPosition::kStand) {
		act("$N не сможет стать вашим скакуном.", false, ch, 0, horse, kToChar);
		return;
	} else if (mount::IsHorse(horse)) {
		if (!privilege::IsImmortal(ch)) {
			SendMsgToChar("Это не ваш скакун.\r\n", ch);
			return;
		}
	}
	if (follow::StopFollower(horse, follow::kSfEmpty))
		return;
	act("Вы оседлали $N3.", false, ch, 0, horse, kToChar);
	act("$n оседлал$g $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
	mount::MakeHorse(horse, ch);
}

void do_givehorse(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *horse, *victim;

	if (ch->IsNpc())
		return;

	if (!(horse = mount::GetHorse(ch))) {
		SendMsgToChar("Да нету у вас скакуна.\r\n", ch);
		return;
	}
	if (!mount::HasHorse(ch, true)) {
		SendMsgToChar("Ваш скакун далеко от вас.\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Кому вы хотите передать скакуна?\r\n", ch);
		return;
	}
	victim = target_resolver::FindCharInRoom(ch, arg);
	if (!victim) {
		SendMsgToChar("Вам некому передать скакуна.\r\n", ch);
		return;
	} else if (victim->IsNpc()) {
		SendMsgToChar("Он и без этого обойдется.\r\n", ch);
		return;
	}
	if (mount::GetHorse(victim)) {
		act("У $N1 уже есть скакун.\r\n", false, ch, 0, victim, kToChar);
		return;
	}
	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Вам стоит слезть со скакуна.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(horse, EAffect::kTethered)) {
		SendMsgToChar("Вам стоит прежде отвязать своего скакуна.\r\n", ch);
		return;
	}
	// Долбанные умертвия при передаче рассыпаются и весело роняют мад на проходе по последователям чара -- Krodo
	if (follow::StopFollower(horse, follow::kSfEmpty))
		return;
	act("Вы передали своего скакуна $N2.", false, ch, 0, victim, kToChar);
	act("$n передал$g вам своего скакуна.", false, ch, 0, victim, kToVict);
	act("$n передал$g своего скакуна $N2.", true, ch, 0, victim, kToNotVict | kToArenaListen);
	mount::MakeHorse(horse, victim);
}

void do_stophorse(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *horse;

	if (ch->IsNpc())
		return;

	if (!(horse = mount::GetHorse(ch))) {
		SendMsgToChar("Да нету у вас скакуна.\r\n", ch);
		return;
	}
	if (!mount::HasHorse(ch, true)) {
		SendMsgToChar("Ваш скакун далеко от вас.\r\n", ch);
		return;
	}
	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Вам стоит слезть со скакуна.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(horse, EAffect::kTethered)) {
		SendMsgToChar("Вам стоит прежде отвязать своего скакуна.\r\n", ch);
		return;
	}
	if (follow::StopFollower(horse, follow::kSfEmpty))
		return;
	act("Вы отпустили $N3.", false, ch, 0, horse, kToChar);
	act("$n отпустил$g $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
	if (GET_MOB_VNUM(horse) == mount::kHorseVnum) {
		act("$n убежал$g в свою конюшню.\r\n", false, horse, 0, 0, kToRoom | kToArenaListen);
		ExtractCharFromWorld(horse, false);
	}
}