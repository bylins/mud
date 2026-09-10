/**
\file punishments.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "administration/punishments.h"
#include "administration/privilege.h"

#include "administration/karma.h"
#include "administration/proxy.h"
#include "engine/entities/char_data.h"
#include "engine/core/char_handler.h"
#include "engine/ui/color.h"
#include "administration/dupe_check.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/sight.h"

#include <fmt/format.h>


namespace punishments {

Punish &Get(CharData *ch, EType type) { return ch->player_specials->punishments[type]; }
const Punish &Get(const CharData *ch, EType type) { return ch->player_specials->punishments[type]; }

bool IsVictimIncorrect(CharData *ch, CharData *vict);
bool IsPunishmentIncorrect(CharData *ch, Punish *pundata, const char *reason);
// Сообщения передаются параметрами: раньше вызывающий складывал их в глобальные buf и buf2,
// а SetPunisherParamsToPundata между делом затирала buf своей строкой (#3814).
void SendPunishmentActMessages(CharData *ch, CharData *vict,
							   const std::string &to_vict, const std::string &to_room);
void SetPunisherParamsToPundata(CharData *ch, Punish *pundata, const char *reason);
void ClearPundata(Punish *pundata);
void MoveToStartRoom(CharData *vict);
RoomRnum GetStartRoomRnum(CharData *vict);
inline bool IsPunishCannotBeApplied(CharData *ch, CharData *vict, Punish *pundata, const char *reason);

bool SetMute(CharData *ch, CharData *vict, char *reason, long times) {
	std::string to_vict, to_room;
	auto pundata = &Get(vict, EType::kMute);
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
		const std::string log_line = fmt::format("Mute OFF for {} by {}.", GET_NAME(vict), GET_NAME(ch));
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Mute OFF by {}", GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&G{} разрешил$G вам кричать.&n", GET_NAME(ch));
		to_room = "$n2 вернулся голос.";
	} else {
		vict->SetFlag(EPlrFlag::kMuted);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		const std::string log_line = fmt::format("Mute ON for {} by {}({}h).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Mute ON ({}h) by {}", times, GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&R{} запретил$G вам кричать.&n", GET_NAME(ch));
		to_room = "$n подавился своим криком.";
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict, to_vict, to_room);
	return true;
}

bool SetDumb(CharData *ch, CharData *vict, char *reason, long times) {
	std::string to_vict, to_room;
	auto pundata = &Get(vict, EType::kDumb);
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
		const std::string log_line = fmt::format("Dumb OFF for {} by {}.", GET_NAME(vict), GET_NAME(ch));
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Dumb OFF by {}", GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&G{} разрешил$G вам издавать звуки.&n", GET_NAME(ch));
		to_room = "$n нарушил$g обет молчания.";
	} else {
		vict->SetFlag(EPlrFlag::kDumbed);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 : MAX_TIME;
		const std::string log_line = fmt::format("Dumb ON for {} by {}({}m).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Dumb ON ({}m) by {}", times, GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&R{} запретил$G вам издавать звуки.&n", GET_NAME(ch));
		to_room = "$n дал$g обет молчания.";
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict, to_vict, to_room);
	return true;
}

bool SetHell(CharData *ch, CharData *vict, char *reason, long times) {
	std::string to_vict, to_room;
	auto pundata = &Get(vict, EType::kHell);
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
		const std::string log_line = fmt::format("{} removed FROM hell by {}.", GET_NAME(vict), GET_NAME(ch));
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Removed FROM hell by {}", GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		if (vict->in_room != kNowhere) {
			act("$n выпущен$a из темницы!", false, vict, nullptr, nullptr, kToRoom);
			MoveToStartRoom(vict);
		};
		to_vict = fmt::format("&G{} выпустил$G вас из темницы.&n", GET_NAME(ch));
		to_room = "$n выпущен$a из темницы!";
	} else {
		vict->SetFlag(EPlrFlag::kHelled);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		if (vict->in_room != kNowhere) {
			act("$n водворен$a в темницу!", false, vict, nullptr, nullptr, kToRoom);
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, r_helled_start_room);
			sight::look_at_room(vict, r_helled_start_room);
		};
		vict->set_was_in_room(kNowhere);
		const std::string log_line = fmt::format("{} moved TO hell by {}({}h).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Moved TO hell ({}h) by {}", times, GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&R{} поместил$G вас в темницу.&n", GET_NAME(ch));
		to_room = "$n водворен$a в темницу!";
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict, to_vict, to_room);
	return true;
}

bool SetFreeze(CharData *ch, CharData *vict, char *reason, long times) {
	std::string to_vict, to_room;
	auto pundata = &Get(vict, EType::kFreeze);
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
		const std::string log_line = fmt::format("Freeze OFF for {} by {}.", GET_NAME(vict), GET_NAME(ch));
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Freeze OFF by {}", GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		if (vict->in_room != kNowhere) {
			act("$n выпущен$a из темницы!", false, vict, nullptr, nullptr, kToRoom);
			MoveToStartRoom(vict);
		};
		to_vict = fmt::format("&G{} выпустил$G вас из темницы.&n", GET_NAME(ch));
		to_room = "$n выпущен$a из темницы!";
		to_vict = "&YЛедяные оковы растаяли под добрым взглядом $N1.&n";
		to_room = "$n освободил$u из ледяного плена.";
	} else {
		vict->SetFlag(EPlrFlag::kFrozen);
		Glory::set_freeze(vict->get_uid());
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		const std::string log_line = fmt::format("Freeze ON for {} by {}({}h).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Freeze ON ({}h) by {}", times, GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = "&BАдский холод сковал ваше тело ледяным панцирем.\r\n&n";
		to_room = "Ледяной панцирь покрыл тело $n1! Стало очень тихо и холодно.";
		if (vict->in_room != kNowhere) {
			act("$n водворен$a в темницу!", false, vict, nullptr, nullptr, kToRoom);
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, r_helled_start_room);
			sight::look_at_room(vict, r_helled_start_room);
		};
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict, to_vict, to_room);
	return true;
}

bool SetNameRoom(CharData *ch, CharData *vict, char *reason, long times) {
	std::string to_vict, to_room;
	auto pundata = &Get(vict, EType::kName);
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
		const std::string log_line = fmt::format("{} removed FROM name room by {}.", GET_NAME(vict), GET_NAME(ch));
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Removed FROM name room by {}", GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		if (vict->in_room != kNowhere) {
			MoveToStartRoom(vict);
			act("$n выпущен$a из комнаты имени!", false, vict, nullptr, nullptr, kToRoom);
		};
		to_vict = fmt::format("&G{} выпустил$G вас из комнаты имени.&n", GET_NAME(ch));
		to_room = "$n выпущен$a из комнаты имени!";
	} else {
		vict->SetFlag(EPlrFlag::kNameDenied);
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		if (vict->in_room != kNowhere) {
			act("$n водворен$a в комнату имени!", false, vict, nullptr, nullptr, kToRoom);
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, r_named_start_room);
			sight::look_at_room(vict, r_named_start_room);
		};
		vict->set_was_in_room(kNowhere);
		const std::string log_line = fmt::format("{} removed to nameroom by {}({}h).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Removed TO nameroom ({}h) by {}", times, GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&R{} поместил$G вас в комнату имени.&n", GET_NAME(ch));
		to_room = "$n помещен$a в комнату имени!";
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict, to_vict, to_room);
	return true;
}

bool SetRegister(CharData *ch, CharData *vict, char *reason) {
	auto pundata = &Get(vict, EType::kHell);
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	ClearPundata(pundata);
	if (vict->IsFlagged(EPlrFlag::kRegistred)) {
		SendMsgToChar("Ваша жертва уже зарегистрирована.\r\n", ch);
		return false;
	};
	const std::string log_line = fmt::format("{} registered by {}.", GET_NAME(vict), GET_NAME(ch));
	mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
	imm_log("%s", log_line.c_str());
	const std::string karma = fmt::format("Registered by {}", GET_NAME(ch));
	RegisterSystem::add(vict, karma.c_str(), reason);
	AddKarma(vict, karma.c_str(), reason);
	if (vict->in_room != kNowhere) {
		act("$n зарегистрирован$a!", false, vict, nullptr, nullptr, kToRoom);
		MoveToStartRoom(vict);
	};
	SendPunishmentActMessages(ch, vict,
							  fmt::format("&G{} зарегистрировал$G вас.&n", GET_NAME(ch)),
							  "$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!");
	return true;
}

bool SetUnregister(CharData *ch, CharData *vict, char *reason, long times) {
	std::string to_vict, to_room;
	auto pundata = &Get(vict, EType::kUnreg);
	if (IsPunishCannotBeApplied(ch, vict, pundata, reason)) {
		return false;
	}
	skip_spaces(&reason);
	if (times == 0) {
		if (!vict->IsFlagged(EPlrFlag::kRegistred)) {
			SendMsgToChar("Ваша цель и так не зарегистрирована.\r\n", ch);
			return false;
		};
		const std::string log_line = fmt::format("{} unregistered by {}.", GET_NAME(vict), GET_NAME(ch));
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Unregistered by {}", GET_NAME(ch));
		RegisterSystem::remove(vict);
		AddKarma(vict, karma.c_str(), reason);
		if (vict->in_room != kNowhere) {
			act("C $n1 снята метка регистрации!", false, vict, nullptr, nullptr, kToRoom);
		}
		to_vict = fmt::format("&W{} снял$G с вас метку регистрации.&n", GET_NAME(ch));
		to_room = "$n лишен$g регистрации!";
	} else {
		pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
		RegisterSystem::remove(vict);
		if (vict->in_room != kNowhere) {
			if (vict->desc && !check_dupes_host(vict->desc) && vict->in_room != r_unreg_start_room) {
				act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.",
					false, vict, nullptr, nullptr, kToRoom);
				RemoveCharFromRoom(vict);
				PlaceCharToRoom(vict, r_unreg_start_room);
				sight::look_at_room(vict, r_unreg_start_room);
			}
		}
		vict->set_was_in_room(kNowhere);
		const std::string log_line = fmt::format("{} unregistred by {}({}h).", GET_NAME(vict), GET_NAME(ch), times);
		mudlog(log_line, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s", log_line.c_str());
		const std::string karma = fmt::format("Unregistered ({}h) by {}", times, GET_NAME(ch));
		AddKarma(vict, karma.c_str(), reason);
		to_vict = fmt::format("&R{} снял$G с вас... регистрацию :).&n", GET_NAME(ch));
		to_room = "$n лишен$a регистрации!";
		SetPunisherParamsToPundata(ch, pundata, reason);
	}
	SendPunishmentActMessages(ch, vict, to_vict, to_room);
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
	if ((GetRealLevel(vict) >= kLvlImmortal && !privilege::IsImpl(ch)) || privilege::IsImpl(vict)) {
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

void SendPunishmentActMessages(CharData *ch, CharData *vict,
							   const std::string &to_vict, const std::string &to_room) {
	if (ch->in_room != kNowhere) {
		act(to_vict, false, vict, nullptr, ch, kToChar);
		act(to_room, false, vict, nullptr, ch, kToRoom);
	};
}

void SetPunisherParamsToPundata(CharData *ch, Punish *pundata, const char *reason) {
	pundata->level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
	pundata->godid = ch->get_uid();
	pundata->reason = fmt::format("{} : {}", ch->get_name(), reason);
}

void ClearPundata(Punish *pundata) {
	pundata->duration = 0;
	pundata->level = 0;
	pundata->godid = 0;
	pundata->reason.clear();
}

void MoveToStartRoom(CharData *vict) {
	RemoveCharFromRoom(vict);
	RoomRnum room_rnum = GetStartRoomRnum(vict);
	PlaceCharToRoom(vict, room_rnum);
	sight::look_at_room(vict, room_rnum);
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
