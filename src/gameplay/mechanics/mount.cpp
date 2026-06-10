// обслуживание функций езды на всяческих жовтоне
//
#include "mount.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/handler.h"
#include "engine/core/target_resolver.h"

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

}  // namespace mount

void make_horse(CharData *horse, CharData *ch) {
	AFF_FLAGS(horse).set(EAffect::kHorse);
	ch->add_follower(horse);
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
	else if (!IS_HORSE(horse))
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
		if (IsAffectedBySpell(ch, ESpell::kSneak))
			RemoveAffectFromCharAndRecalculate(ch, ESpell::kSneak);
		if (IsAffectedBySpell(ch, ESpell::kCamouflage))
			RemoveAffectFromCharAndRecalculate(ch, ESpell::kCamouflage);
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
	ch->dismount();

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
	else if (!IS_HORSE(horse))
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
	else if (!IS_HORSE(horse))
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
	else if (!ch->IsGod()
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
	} else if (IS_HORSE(horse)) {
		if (!ch->IsImmortal()) {
			SendMsgToChar("Это не ваш скакун.\r\n", ch);
			return;
		}
	}
	if (stop_follower(horse, kSfEmpty))
		return;
	act("Вы оседлали $N3.", false, ch, 0, horse, kToChar);
	act("$n оседлал$g $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
	make_horse(horse, ch);
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
	if (stop_follower(horse, kSfEmpty))
		return;
	act("Вы передали своего скакуна $N2.", false, ch, 0, victim, kToChar);
	act("$n передал$g вам своего скакуна.", false, ch, 0, victim, kToVict);
	act("$n передал$g своего скакуна $N2.", true, ch, 0, victim, kToNotVict | kToArenaListen);
	make_horse(horse, victim);
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
	if (stop_follower(horse, kSfEmpty))
		return;
	act("Вы отпустили $N3.", false, ch, 0, horse, kToChar);
	act("$n отпустил$g $N3.", false, ch, 0, horse, kToRoom | kToArenaListen);
	if (GET_MOB_VNUM(horse) == kHorseVnum) {
		act("$n убежал$g в свою конюшню.\r\n", false, horse, 0, 0, kToRoom | kToArenaListen);
		ExtractCharFromWorld(horse, false);
	}
}