/**
\file punishments.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "administration/punishments.h"

#include "administration/karma.h"
#include "administration/proxy.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/sight.h"

extern int check_dupes_host(DescriptorData *d, bool autocheck = false);

namespace punishments {

bool IsVictimIncorrect(CharData *ch, CharData *vict);
bool IsPunishmentIncorrect(CharData *ch, Punish *pundata, const char *reason);
void SendPunishmentActMessages(CharData *ch, CharData *vict);
void SetPunisherParamsToPundata(CharData *ch, Punish *pundata, const char *reason);
void ClearPundata(Punish *pundata);
void MoveToStartRoom(CharData *vict);
RoomRnum GetStartRoomRnum(CharData *vict);
inline bool IsPunishCannotBeApplied(CharData *ch, CharData *vict, Punish *pundata, const char *reason);

bool SetMute(CharData *ch, CharData *vict, char *reason, long times) {
	auto pundata = &(vict)->player_specials->pmute;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	if (times == 0) {
		ClearPundata(pundata);
		if (!vict->IsFlagged(EPlrFlag::kMuted)) {
			SendMsgToChar("Ваша жертва и так может кричать.\r\n", ch);
			return false;
		};
		vict->UnsetFlag(EPlrFlag::kMuted);
		sprintf(buf, "Mute OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Mute OFF by %s", GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s разрешил$G вам кричать.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n2 вернулся голос.");
	} else {
		vict->SetFlag(EPlrFlag::kMuted);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		sprintf(buf, "Mute ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Mute ON (%ldh) by %s", times, GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s запретил$G вам кричать.%s", kColorBoldRed, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n подавился своим криком.");
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict);
	return true;
}

bool SetDumb(CharData *ch, CharData *vict, char *reason, long times) {
	auto pundata = &(vict)->player_specials->pdumb;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	if (times == 0) {
		ClearPundata(pundata);
		if (!vict->IsFlagged(EPlrFlag::kDumbed)) {
			SendMsgToChar("Ваша жертва и так может издавать звуки.\r\n", ch);
			return false;
		};
		vict->UnsetFlag(EPlrFlag::kDumbed);
		sprintf(buf, "Dumb OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Dumb OFF by %s", GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s разрешил$G вам издавать звуки.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n нарушил$g обет молчания.");
	} else {
		vict->SetFlag(EPlrFlag::kDumbed);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 : MAX_TIME;
		sprintf(buf, "Dumb ON for %s by %s(%ldm).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Dumb ON (%ldm) by %s", times, GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s запретил$G вам издавать звуки.%s", kColorBoldRed, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n дал$g обет молчания.");
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict);
	return true;
}

bool SetHell(CharData *ch, CharData *vict, char *reason, long times) {
	auto pundata = &(vict)->player_specials->phell;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	if (times == 0) {
		ClearPundata(pundata);
		if (!vict->IsFlagged(EPlrFlag::kHelled)) {
			SendMsgToChar("Ваша жертва и так на свободе.\r\n", ch);
			return false;
		};
		vict->UnsetFlag(EPlrFlag::kHelled);
		sprintf(buf, "%s removed FROM hell by %s.", GET_NAME(vict), GET_NAME(ch));
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Removed FROM hell by %s", GET_NAME(ch));
		AddKarma(vict, buf, reason);
		if (vict->in_room != kNowhere) {
			act("$n выпущен$a из темницы!", false, vict, nullptr, nullptr, kToRoom);
			MoveToStartRoom(vict);
		};
		sprintf(buf, "%s%s выпустил$G вас из темницы.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n выпущен$a из темницы!");
	} else {
		vict->SetFlag(EPlrFlag::kHelled);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		if (vict->in_room != kNowhere) {
			act("$n водворен$a в темницу!", false, vict, nullptr, nullptr, kToRoom);
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, r_helled_start_room);
			look_at_room(vict, r_helled_start_room);
		};
		vict->set_was_in_room(kNowhere);
		sprintf(buf, "%s moved TO hell by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Moved TO hell (%ldh) by %s", times, GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s поместил$G вас в темницу.%s", GET_NAME(ch), kColorBoldRed, kColorNrm);
		sprintf(buf2, "$n водворен$a в темницу!");
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict);
	return true;
}

bool SetFreeze(CharData *ch, CharData *vict, char *reason, long times) {
	auto pundata = &(vict)->player_specials->pfreeze;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	if (times == 0) {
		ClearPundata(pundata);
		if (!vict->IsFlagged(EPlrFlag::kFrozen)) {
			SendMsgToChar("Ваша жертва уже разморожена.\r\n", ch);
			return false;
		};
		vict->UnsetFlag(EPlrFlag::kFrozen);
		Glory::remove_freeze(vict->get_uid());
		if (vict->IsFlagged(EPlrFlag::kHelled)) {
			vict->UnsetFlag(EPlrFlag::kHelled);
		}
		sprintf(buf, "Freeze OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Freeze OFF by %s", GET_NAME(ch));
		AddKarma(vict, buf, reason);
		if (vict->in_room != kNowhere) {
			act("$n выпущен$a из темницы!", false, vict, nullptr, nullptr, kToRoom);
			MoveToStartRoom(vict);
		};
		sprintf(buf, "%s%s выпустил$G вас из темницы.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n выпущен$a из темницы!");
		sprintf(buf, "%sЛедяные оковы растаяли под добрым взглядом $N1.%s", kColorBoldYel, kColorNrm);
		sprintf(buf2, "$n освободил$u из ледяного плена.");
	} else {
		vict->SetFlag(EPlrFlag::kFrozen);
		Glory::set_freeze(vict->get_uid());
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		sprintf(buf, "Freeze ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Freeze ON (%ldh) by %s", times, GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%sАдский холод сковал ваше тело ледяным панцирем.\r\n%s", kColorBoldBlu, kColorNrm);
		sprintf(buf2, "Ледяной панцирь покрыл тело $n1! Стало очень тихо и холодно.");
		if (vict->in_room != kNowhere) {
			act("$n водворен$a в темницу!", false, vict, nullptr, nullptr, kToRoom);
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, r_helled_start_room);
			look_at_room(vict, r_helled_start_room);
		};
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict);
	return true;
}

bool SetNameRoom(CharData *ch, CharData *vict, char *reason, long times) {
	auto pundata = &(vict)->player_specials->pname;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	if (times == 0) {
		ClearPundata(pundata);
		if (!vict->IsFlagged(EPlrFlag::kNameDenied)) {
			SendMsgToChar("Вашей жертвы нет в комнате имени.\r\n", ch);
			return false;
		};
		vict->UnsetFlag(EPlrFlag::kNameDenied);
		sprintf(buf, "%s removed FROM name room by %s.", GET_NAME(vict), GET_NAME(ch));
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Removed FROM name room by %s", GET_NAME(ch));
		AddKarma(vict, buf, reason);
		if (vict->in_room != kNowhere) {
			MoveToStartRoom(vict);
			act("$n выпущен$a из комнаты имени!", false, vict, nullptr, nullptr, kToRoom);
		};
		sprintf(buf, "%s%s выпустил$G вас из комнаты имени.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n выпущен$a из комнаты имени!");
	} else {
		vict->SetFlag(EPlrFlag::kNameDenied);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		if (vict->in_room != kNowhere) {
			act("$n водворен$a в комнату имени!", false, vict, nullptr, nullptr, kToRoom);
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, r_named_start_room);
			look_at_room(vict, r_named_start_room);
		};
		vict->set_was_in_room(kNowhere);
		sprintf(buf, "%s removed to nameroom by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Removed TO nameroom (%ldh) by %s", times, GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s поместил$G вас в комнату имени.%s", kColorBoldRed, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n помещен$a в комнату имени!");
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict);
	return true;
}

bool SetRegister(CharData *ch, CharData *vict, char *reason) {
	auto pundata = &(vict)->player_specials->phell;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	ClearPundata(pundata);
	if (vict->IsFlagged(EPlrFlag::kRegistred)) {
		SendMsgToChar("Вашей жертва уже зарегистрирована.\r\n", ch);
		return false;
	};
	sprintf(buf, "%s registered by %s.", GET_NAME(vict), GET_NAME(ch));
	mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
	imm_log("%s", buf);
	sprintf(buf, "Registered by %s", GET_NAME(ch));
	RegisterSystem::add(vict, buf, reason);
	AddKarma(vict, buf, reason);
	if (vict->in_room != kNowhere) {
		act("$n зарегистрирован$a!", false, vict, nullptr, nullptr, kToRoom);
		MoveToStartRoom(vict);
	};
	sprintf(buf, "%s%s зарегистрировал$G вас.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
	sprintf(buf2, "$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!");
	SendPunishmentActMessages(ch, vict);
	return true;
}

bool SetUnregister(CharData *ch, CharData *vict, char *reason, long times) {
	auto pundata = &(vict)->player_specials->punreg;
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	skip_spaces(&reason);
	if (times == 0) {
		if (!vict->IsFlagged(EPlrFlag::kRegistred)) {
			SendMsgToChar("Ваша цель и так не зарегистрирована.\r\n", ch);
			return false;
		};
		sprintf(buf, "%s unregistered by %s.", GET_NAME(vict), GET_NAME(ch));
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Unregistered by %s", GET_NAME(ch));
		RegisterSystem::remove(vict);
		AddKarma(vict, buf, reason);
		if (vict->in_room != kNowhere) {
			act("C $n1 снята метка регистрации!", false, vict, nullptr, nullptr, kToRoom);
		}
		sprintf(buf, "&W%s снял$G с вас метку регистрации.&n", GET_NAME(ch));
		sprintf(buf2, "$n лишен$g регистрации!");
	} else {
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		RegisterSystem::remove(vict);
		if (vict->in_room != kNowhere) {
			if (vict->desc && !check_dupes_host(vict->desc) && vict->in_room != r_unreg_start_room) {
				act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.",
					false, vict, nullptr, nullptr, kToRoom);
				RemoveCharFromRoom(vict);
				PlaceCharToRoom(vict, r_unreg_start_room);
				look_at_room(vict, r_unreg_start_room);
			}
		}
		vict->set_was_in_room(kNowhere);
		sprintf(buf, "%s unregistred by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", buf);
		sprintf(buf, "Unregistered (%ldh) by %s", times, GET_NAME(ch));
		AddKarma(vict, buf, reason);
		sprintf(buf, "%s%s снял$G с вас... регистрацию :).%s", kColorBoldRed, GET_NAME(ch), kColorNrm);
		sprintf(buf2, "$n лишен$a регистрации!");
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict);
	return true;
}

inline bool IsPunishCannotBeApplied(CharData *ch, CharData *vict, Punish *pundata, const char *reason) {
	return (IsVictimIncorrect(ch, vict) || IsPunishmentIncorrect(ch, pundata, reason));
}

bool IsVictimIncorrect(CharData *ch, CharData *vict) {
	if (ch == vict) {
		SendMsgToChar("Это слишком жестоко...\r\n", ch);
		return true;
	}
	if ((GetRealLevel(vict) >= kLvlImmortal && !IS_IMPL(ch)) || IS_IMPL(vict)) {
		SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
		return true;
	}
	return false;
}

bool IsPunishmentIncorrect(CharData *ch, Punish *pundata, const char *reason) {
	assert(pundata);
	if (GetRealLevel(ch) < pundata->level) {
		SendMsgToChar("Да кто ты такой?!! Чтобы оспаривать волю СТАРШИХ БОГОВ!!!\r\n", ch);
		return true;
	}
	if (!reason || !*reason) {
		SendMsgToChar("Укажите причину такой (не)милости.\r\n", ch);
		return true;
	}
	return false;
}

void SendPunishmentActMessages(CharData *ch, CharData *vict) {
	if (ch->in_room != kNowhere) {
		act(buf, false, vict, nullptr, ch, kToChar);
		act(buf2, false, vict, nullptr, ch, kToRoom);
	};
}

void SetPunisherParamsToPundata(CharData *ch, Punish *pundata, const char *reason) {
	pundata->level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
	pundata->godid = ch->get_uid();
	sprintf(buf, "%s : %s", ch->get_name().c_str(), reason);
	pundata->reason = str_dup(buf);
}

void ClearPundata(Punish *pundata) {
	pundata->duration = 0;
	pundata->level = 0;
	pundata->godid = 0;
	if (pundata->reason) {
		free(pundata->reason);
	}
	pundata->reason = nullptr;
}

void MoveToStartRoom(CharData *vict) {
	RemoveCharFromRoom(vict);
	RoomRnum room_rnum = GetStartRoomRnum(vict);
	PlaceCharToRoom(vict, room_rnum);
	look_at_room(vict, room_rnum);
}

RoomRnum GetStartRoomRnum(CharData *vict) {
	RoomRnum result;
	if ((result = GET_LOADROOM(vict)) == kNowhere) {
		result = calc_loadroom(vict);
	}
	result = GetRoomRnum(result);
	if (result == kNowhere) {
		if (GetRealLevel(vict) >= kLvlImmortal) {
			result = r_immort_start_room;
		} else {
			result = r_mortal_start_room;
		}
	}
	return result;
}

} // namespace punishments

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
