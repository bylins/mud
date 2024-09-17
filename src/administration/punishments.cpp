/**
\file punishments.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "administration/punishments.h"

#include "administration/proxy.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/sight.h"

extern int check_dupes_host(DescriptorData *d, bool autocheck = false);
extern void AddKarma(CharData *ch, const char *punish, const char *reason);

namespace punishments {

int set_punish(CharData *ch, CharData *vict, int punish, char *reason, long times) {
	Punish *pundata = nullptr;
	int result;

	if (ch == vict) {
		SendMsgToChar("Это слишком жестоко...\r\n", ch);
		return 0;
	}
	if ((GetRealLevel(vict) >= kLvlImmortal && !IS_IMPL(ch)) || IS_IMPL(vict)) {
		SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
		return 0;
	}
	// Проверяем а может ли чар вообще работать с этим наказанием.
	switch (punish) {
		case SCMD_MUTE: pundata = &(vict)->player_specials->pmute;
			break;
		case SCMD_DUMB: pundata = &(vict)->player_specials->pdumb;
			break;
		case SCMD_HELL: pundata = &(vict)->player_specials->phell;
			break;
		case SCMD_NAME: pundata = &(vict)->player_specials->pname;
			break;
		case SCMD_FREEZE: pundata = &(vict)->player_specials->pfreeze;
			break;
		case SCMD_REGISTER:
		case SCMD_UNREGISTER: pundata = &(vict)->player_specials->punreg;
			break;
	}
	assert(pundata);
	if (GetRealLevel(ch) < pundata->level) {
		SendMsgToChar("Да кто ты такой?!! Чтобы оспаривать волю СТАРШИХ БОГОВ!!!\r\n", ch);
		return 0;
	}
	// Проверяем наказание или освобождение.
	if (times == 0) {
		// Чара досрочно освобождают от наказания.
		if (!reason || !*reason) {
			SendMsgToChar("Укажите причину такой милости.\r\n", ch);
			return 0;
		} else
			skip_spaces(&reason);
		//
		pundata->duration = 0;
		pundata->level = 0;
		pundata->godid = 0;
		if (pundata->reason)
			free(pundata->reason);
		pundata->reason = nullptr;
		switch (punish) {
			case SCMD_MUTE:
				if (!vict->IsFlagged(EPlrFlag::kMuted)) {
					SendMsgToChar("Ваша жертва и так может кричать.\r\n", ch);
					return (0);
				};
				vict->UnsetFlag(EPlrFlag::kMuted);
				sprintf(buf, "Mute OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Mute OFF by %s", GET_NAME(ch));
				AddKarma(vict, buf, reason);
				sprintf(buf, "%s%s разрешил$G вам кричать.%s", kColorBoldGrn, GET_NAME(ch), kColorNrm);
				sprintf(buf2, "$n2 вернулся голос.");
				break;
			case SCMD_FREEZE:
				if (!vict->IsFlagged(EPlrFlag::kFrozen)) {
					SendMsgToChar("Ваша жертва уже разморожена.\r\n", ch);
					return (0);
				};
				vict->UnsetFlag(EPlrFlag::kFrozen);
				Glory::remove_freeze(GET_UID(vict));
				if (vict->IsFlagged(EPlrFlag::kHelled)) //заодно снимем ад
					vict->UnsetFlag(EPlrFlag::kHelled);
				sprintf(buf, "Freeze OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Freeze OFF by %s", GET_NAME(ch));
				AddKarma(vict, buf, reason);
				if (vict->in_room != kNowhere) {
					act("$n выпущен$a из темницы!", false, vict, nullptr, nullptr, kToRoom);
					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);
					result = GetRoomRnum(result);
					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}
					RemoveCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
				};
				sprintf(buf, "%s%s выпустил$G вас из темницы.%s",
						kColorBoldGrn, GET_NAME(ch), kColorNrm);
				sprintf(buf2, "$n выпущен$a из темницы!");
				sprintf(buf, "%sЛедяные оковы растаяли под добрым взглядом $N1.%s",
						kColorBoldYel, kColorNrm);
				sprintf(buf2, "$n освободил$u из ледяного плена.");
				break;
			case SCMD_DUMB:
				if (!vict->IsFlagged(EPlrFlag::kDumbed)) {
					SendMsgToChar("Ваша жертва и так может издавать звуки.\r\n", ch);
					return (0);
				};
				vict->UnsetFlag(EPlrFlag::kDumbed);

				sprintf(buf, "Dumb OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Dumb OFF by %s", GET_NAME(ch));
				AddKarma(vict, buf, reason);

				sprintf(buf, "%s%s разрешил$G вам издавать звуки.%s",
						kColorBoldGrn, GET_NAME(ch), kColorNrm);

				sprintf(buf2, "$n нарушил$g обет молчания.");

				break;

			case SCMD_HELL:
				if (!vict->IsFlagged(EPlrFlag::kHelled)) {
					SendMsgToChar("Ваша жертва и так на свободе.\r\n", ch);
					return (0);
				};
				vict->UnsetFlag(EPlrFlag::kHelled);

				sprintf(buf, "%s removed FROM hell by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Removed FROM hell by %s", GET_NAME(ch));
				AddKarma(vict, buf, reason);

				if (vict->in_room != kNowhere) {
					act("$n выпущен$a из темницы!",
						false, vict, nullptr, nullptr, kToRoom);

					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = GetRoomRnum(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}
					RemoveCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
				};

				sprintf(buf, "%s%s выпустил$G вас из темницы.%s",
						kColorBoldGrn, GET_NAME(ch), kColorNrm);

				sprintf(buf2, "$n выпущен$a из темницы!");
				break;

			case SCMD_NAME:

				if (!vict->IsFlagged(EPlrFlag::kNameDenied)) {
					SendMsgToChar("Вашей жертвы там нет.\r\n", ch);
					return (0);
				};
				vict->UnsetFlag(EPlrFlag::kNameDenied);

				sprintf(buf, "%s removed FROM name room by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Removed FROM name room by %s", GET_NAME(ch));
				AddKarma(vict, buf, reason);

				if (vict->in_room != kNowhere) {
					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = GetRoomRnum(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}

					RemoveCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
					act("$n выпущен$a из комнаты имени!",
						false, vict, nullptr, nullptr, kToRoom);
				};
				sprintf(buf, "%s%s выпустил$G вас из комнаты имени.%s",
						kColorBoldGrn, GET_NAME(ch), kColorNrm);

				sprintf(buf2, "$n выпущен$a из комнаты имени!");
				break;

			case SCMD_REGISTER:
				// Регистриуем чара
				if (vict->IsFlagged(EPlrFlag::kRegistred)) {
					SendMsgToChar("Вашей жертва уже зарегистрирована.\r\n", ch);
					return (0);
				};

				sprintf(buf, "%s registered by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Registered by %s", GET_NAME(ch));
				RegisterSystem::add(vict, buf, reason);
				AddKarma(vict, buf, reason);

				if (vict->in_room != kNowhere) {

					act("$n зарегистрирован$a!", false, vict,
						nullptr, nullptr, kToRoom);

					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = GetRoomRnum(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}

					RemoveCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
				};
				sprintf(buf, "%s%s зарегистрировал$G вас.%s",
						kColorBoldGrn, GET_NAME(ch), kColorNrm);
				sprintf(buf2, "$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!");
				break;
			case SCMD_UNREGISTER:
				if (!vict->IsFlagged(EPlrFlag::kRegistred)) {
					SendMsgToChar("Ваша цель и так не зарегистрирована.\r\n", ch);
					return (0);
				};

				sprintf(buf, "%s unregistered by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Unregistered by %s", GET_NAME(ch));
				RegisterSystem::remove(vict);
				AddKarma(vict, buf, reason);

				if (vict->in_room != kNowhere) {
					act("C $n1 снята метка регистрации!",
						false, vict, nullptr, nullptr, kToRoom);
					/*				if ((result = GET_LOADROOM(vict)) == kNowhere)
									result = r_unreg_start_room;

								result = GetRoomRnum(result);

								if (result == kNowhere)
								{
									if (GetRealLevel(vict) >= kLevelImmortal)
										result = r_immort_start_room;
									else
										result = r_mortal_start_room;
								}

								char_from_room(vict);
								char_to_room(vict, result);
								look_at_room(vict, result);
				*/
				}
				sprintf(buf, "&W%s снял$G с вас метку регистрации.&n", GET_NAME(ch));
				sprintf(buf2, "$n лишен$g регистрации!");
				break;
		}
	} else {
		// Чара наказывают.
		if (!reason || !*reason) {
			SendMsgToChar("Укажите причину наказания.\r\n", ch);
			return 0;
		} else
			skip_spaces(&reason);

		pundata->level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		pundata->godid = GET_UID(ch);

		// Добавляем в причину имя имма

		sprintf(buf, "%s : %s", GET_NAME(ch), reason);
		pundata->reason = str_dup(buf);

		switch (punish) {
			case SCMD_MUTE: vict->SetFlag(EPlrFlag::kMuted);
				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				sprintf(buf, "Mute ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Mute ON (%ldh) by %s", times, GET_NAME(ch));
				AddKarma(vict, buf, reason);

				sprintf(buf, "%s%s запретил$G вам кричать.%s",
						kColorBoldRed, GET_NAME(ch), kColorNrm);

				sprintf(buf2, "$n подавился своим криком.");

				break;

			case SCMD_FREEZE: vict->SetFlag(EPlrFlag::kFrozen);
				Glory::set_freeze(GET_UID(vict));
				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				sprintf(buf, "Freeze ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Freeze ON (%ldh) by %s", times, GET_NAME(ch));
				AddKarma(vict, buf, reason);

				sprintf(buf, "%sАдский холод сковал ваше тело ледяным панцирем.\r\n%s",
						kColorBoldBlu, kColorNrm);
				sprintf(buf2, "Ледяной панцирь покрыл тело $n1! Стало очень тихо и холодно.");
				if (vict->in_room != kNowhere) {
					act("$n водворен$a в темницу!",
						false, vict, nullptr, nullptr, kToRoom);

					RemoveCharFromRoom(vict);
					PlaceCharToRoom(vict, r_helled_start_room);
					look_at_room(vict, r_helled_start_room);
				};
				break;

			case SCMD_DUMB: vict->SetFlag(EPlrFlag::kDumbed);
				pundata->duration = (times > 0) ? time(nullptr) + times * 60 : MAX_TIME;

				sprintf(buf, "Dumb ON for %s by %s(%ldm).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Dumb ON (%ldm) by %s", times, GET_NAME(ch));
				AddKarma(vict, buf, reason);

				sprintf(buf, "%s%s запретил$G вам издавать звуки.%s",
						kColorBoldRed, GET_NAME(ch), kColorNrm);

				sprintf(buf2, "$n дал$g обет молчания.");
				break;
			case SCMD_HELL: vict->SetFlag(EPlrFlag::kHelled);

				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				if (vict->in_room != kNowhere) {
					act("$n водворен$a в темницу!",
						false, vict, nullptr, nullptr, kToRoom);

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

				sprintf(buf, "%s%s поместил$G вас в темницу.%s", GET_NAME(ch),
						kColorBoldRed, kColorNrm);
				sprintf(buf2, "$n водворен$a в темницу!");
				break;

			case SCMD_NAME: vict->SetFlag(EPlrFlag::kNameDenied);

				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				if (vict->in_room != kNowhere) {
					act("$n водворен$a в комнату имени!",
						false, vict, nullptr, nullptr, kToRoom);
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

				sprintf(buf, "%s%s поместил$G вас в комнату имени.%s",
						kColorBoldRed, GET_NAME(ch), kColorNrm);
				sprintf(buf2, "$n помещен$a в комнату имени!");
				break;

			case SCMD_UNREGISTER: pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
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

				sprintf(buf, "%s%s снял$G с вас... регистрацию :).%s",
						kColorBoldRed, GET_NAME(ch), kColorNrm);
				sprintf(buf2, "$n лишен$a регистрации!");

				break;

		}
	}
	if (ch->in_room != kNowhere) {
		act(buf, false, vict, nullptr, ch, kToChar);
		act(buf2, false, vict, nullptr, ch, kToRoom);
	};
	return 1;
}

} // namespace punishments

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
