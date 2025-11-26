//
// Created by Sventovit on 05.09.2024.
//

#include "administration/karma.h"
#include "administration/names.h"
#include "administration/privilege.h"
#include "administration/punishments.h"
#include "gameplay/mechanics/depot.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/olc/olc.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/mechanics/player_races.h"
#include "administration/password.h"
#include "gameplay/statistics/top.h"
#include "gameplay/fight/fight.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/weather.h"
#include "engine/db/player_index.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <memory>

enum class ESetVict {
  kPc,
  kNpc,
  kBoth
};

enum class ESetValue {
  kMisc,
  kBinary,
  kNumber
};

struct SetCmdStruct {
  const char *cmd{nullptr};
  const char level{0};
  const ESetVict pcnpc{ESetVict::kPc};
  const ESetValue type{ESetValue::kMisc};
};

SetCmdStruct set_fields[] =
	{
		{"brief", kLvlGod, ESetVict::kPc, ESetValue::kBinary},    // 0
		{"invstart", kLvlGod, ESetVict::kPc, ESetValue::kBinary},    // 1
		{"nosummon", kLvlGreatGod, ESetVict::kPc, ESetValue::kBinary},
		{"maxhit", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},
		{"maxmana", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"maxmove", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},    // 5
		{"hit", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"mana", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"move", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"race", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"size", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},    // 10
		{"ac", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"gold", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},
		{"bank", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber},
		{"exp", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},
		{"hitroll", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber}, // 15
		{"damroll", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},
		{"invis", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber},
		{"nohassle", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary},
		{"frozen", kLvlGreatGod, ESetVict::kPc, ESetValue::kMisc},
		{"practices", kLvlGreatGod, ESetVict::kPc, ESetValue::kNumber}, // 20
		{"lessons", kLvlGreatGod, ESetVict::kPc, ESetValue::kNumber},
		{"drunk", kLvlGreatGod, ESetVict::kBoth, ESetValue::kMisc},
		{"hunger", kLvlGreatGod, ESetVict::kBoth, ESetValue::kMisc},
		{"thirst", kLvlGreatGod, ESetVict::kBoth, ESetValue::kMisc},
		{"thief", kLvlGod, ESetVict::kPc, ESetValue::kBinary}, // 25
		{"level", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},
		{"room", kLvlImplementator, ESetVict::kBoth, ESetValue::kNumber},
		{"roomflag", kLvlGreatGod, ESetVict::kPc, ESetValue::kBinary},
		{"siteok", kLvlGreatGod, ESetVict::kPc, ESetValue::kBinary},
		{"deleted", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 30
		{"class", kLvlImplementator, ESetVict::kBoth, ESetValue::kMisc},
		{"demigod", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary},
		{"loadroom", kLvlGreatGod, ESetVict::kPc, ESetValue::kMisc},
		{"color", kLvlGod, ESetVict::kPc, ESetValue::kBinary},
		{"idnum", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber}, // 35
		{"passwd", kLvlImplementator, ESetVict::kPc, ESetValue::kMisc},
		{"nodelete", kLvlGod, ESetVict::kPc, ESetValue::kBinary},
		{"sex", kLvlGreatGod, ESetVict::kBoth, ESetValue::kMisc},
		{"age", kLvlGreatGod, ESetVict::kBoth, ESetValue::kNumber},
		{"height", kLvlGod, ESetVict::kBoth, ESetValue::kNumber}, // 40
		{"weight", kLvlGod, ESetVict::kBoth, ESetValue::kNumber},
		{"godslike", kLvlImplementator, ESetVict::kBoth, ESetValue::kBinary},
		{"godscurse", kLvlImplementator, ESetVict::kBoth, ESetValue::kBinary},
		{"olc", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber},
		{"name", kLvlGreatGod, ESetVict::kPc, ESetValue::kMisc}, // 45
		{"trgquest", kLvlImplementator, ESetVict::kPc, ESetValue::kMisc},
		{"mkill", kLvlImplementator, ESetVict::kPc, ESetValue::kMisc},
		{"highgod", kLvlImplementator, ESetVict::kPc, ESetValue::kMisc},
		{"hell", kLvlGod, ESetVict::kPc, ESetValue::kMisc},
		{"email", kLvlGod, ESetVict::kPc, ESetValue::kMisc}, //50
		{"religion", kLvlGod, ESetVict::kPc, ESetValue::kMisc},
		{"perslog", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary},
		{"mute", kLvlGod, ESetVict::kPc, ESetValue::kMisc},
		{"dumb", kLvlGod, ESetVict::kPc, ESetValue::kMisc},
		{"karma", kLvlImplementator, ESetVict::kPc, ESetValue::kMisc},
		{"unreg", kLvlGod, ESetVict::kPc, ESetValue::kMisc}, // 56
		{"палач", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 57
		{"killer", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 58
		{"remort", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber}, // 59
		{"tester", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 60
		{"autobot", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 61
		{"hryvn", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber}, // 62
		{"scriptwriter", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 63
		{"spammer", kLvlGod, ESetVict::kPc, ESetValue::kBinary}, // 64
		{"gloryhide", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, // 65
		{"telegram", kLvlImplementator, ESetVict::kPc, ESetValue::kMisc}, // 66
		{"nogata", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber}, // 67
		{"position", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber},
		{"skilltester", kLvlImplementator, ESetVict::kPc, ESetValue::kBinary}, //69
		{"quest", kLvlImplementator, ESetVict::kPc, ESetValue::kNumber}, //70
		{"\n", 0, ESetVict::kBoth, ESetValue::kMisc}
	};

int PerformSet(CharData *ch, CharData *vict, int mode, char *val_arg);
void RenamePlayer(CharData *ch, char *oname);

extern int _parse_name(char *arg, char *name);
extern int reserved_word(const char *argument);

void DoSet(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;
	char field[kMaxInputLength], name[kMaxInputLength], val_arg[kMaxInputLength], OName[kMaxInputLength];
	int mode, player_i = 0, retval;
	char is_file = 0, is_player = 0;

	half_chop(argument, name, buf);

	if (!*name) {
		strcpy(buf, "Возможные поля для изменения:\r\n");
		for (int i = 0; set_fields[i].level; i++)
			if (privilege::HasPrivilege(ch, std::string(set_fields[i].cmd), 0, 1))
				sprintf(buf + strlen(buf), "%-15s%s", set_fields[i].cmd, (!((i + 1) % 5) ? "\r\n" : ""));
		strcat(buf, "\r\n");
		SendMsgToChar(buf, ch);
		return;
	}

	if (!strcmp(name, "file")) {
		is_file = 1;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "player")) {
		is_player = 1;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "mob")) {
		half_chop(buf, name, buf);
	} else
		is_player = 1;

	half_chop(buf, field, buf);
	strcpy(val_arg, buf);

	if (!*name || !*field) {
		SendMsgToChar("Usage: set [mob|player|file] <victim> <field> <value>\r\n", ch);
		return;
	}

	CharData::shared_ptr cbuf;
	// find the target
	if (!is_file) {
		if (is_player) {

			if (!(vict = get_player_pun(ch, name, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}

			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, EGf::kDemigod)) {
				if ((GetRealLevel(ch) <= GetRealLevel(vict)) && !(is_head(ch->get_name_str()))) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			} else {
				if (GetRealLevel(vict) >= kLvlImmortal) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
		} else    // is_mob
		{
			if (!(vict = get_char_vis(ch, name, EFind::kCharInWorld))
				|| !vict->IsNpc()) {
				SendMsgToChar("Нет такой твари Божьей.\r\n", ch);
				return;
			}
		}
	} else if (is_file)    // try to load the player off disk
	{
		if (get_player_pun(ch, name, EFind::kCharInWorld)) {
			SendMsgToChar("Да разуй же глаза! Оно в сети!\r\n", ch);
			return;
		}

		cbuf = std::make_unique<Player>();
		if ((player_i = LoadPlayerCharacter(name, cbuf.get(), ELoadCharFlags::kFindId)) > -1) {
			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, EGf::kDemigod)) {
				if (GetRealLevel(ch) <= GetRealLevel(cbuf) && !(is_head(ch->get_name_str()))) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			} else {
				if (GetRealLevel(cbuf) >= kLvlImmortal) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
			vict = cbuf.get();
		} else {
			SendMsgToChar("Нет такого игрока.\r\n", ch);
			return;
		}
	}

	// find the command in the list
	size_t len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++) {
		if (!strncmp(field, set_fields[mode].cmd, len)) {
			break;
		}
	}

	// perform the set
	strcpy(OName, GET_NAME(vict));
	retval = PerformSet(ch, vict, mode, val_arg);

	// save the character if a change was made
	if (retval && !vict->IsNpc()) {
		if (retval == 2) {
			RenamePlayer(vict, OName);
		} else {
			if (!is_file && !vict->IsNpc()) {
				vict->save_char();
			}
			if (is_file) {
				vict->save_char();
				SendMsgToChar("Файл сохранен.\r\n", ch);
			}
		}
	}

	log("(GC) %s try to set: %s", GET_NAME(ch), argument);
	imm_log("%s try to set: %s", GET_NAME(ch), argument);
}

int PerformSet(CharData *ch, CharData *vict, int mode, char *val_arg) {
	int i, j, c, value = 0, return_code = 1, ptnum, times = 0;
//	bool on = false;
//	bool off = false;
	char npad[ECase::kLastCase + 1][256];
	char *reason;
	RoomRnum rnum;
	RoomVnum rvnum;
	char output[kMaxStringLength], num[kMaxInputLength];
	int rod;

	// Check to make sure all the levels are correct
	if (!IS_IMPL(ch)) {
		if (!vict->IsNpc() && vict != ch) {
			if (!GET_GOD_FLAG(ch, EGf::kDemigod)) {
				if (GetRealLevel(ch) <= GetRealLevel(vict) && !ch->IsFlagged(EPrf::kCoderinfo)) {
					SendMsgToChar("Это не так просто, как вам кажется...\r\n", ch);
					return (0);
				}
			} else {
				if (GetRealLevel(vict) >= kLvlImmortal || vict->IsFlagged(EPrf::kCoderinfo)) {
					SendMsgToChar("Это не так просто, как вам кажется...\r\n", ch);
					return (0);
				}
			}
		}
	}
	if (!privilege::HasPrivilege(ch, std::string(set_fields[mode].cmd), 0, 1)) {
		SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
		return (0);
	}

	// Make sure the PC/NPC is correct
	if (vict->IsNpc() && (set_fields[mode].pcnpc == ESetVict::kPc)) {
		SendMsgToChar("Эта тварь недостойна такой чести!\r\n", ch);
		return (0);
	} else if (!vict->IsNpc() && (set_fields[mode].pcnpc == ESetVict::kNpc)) {
		act("Вы оскорбляете $S - $E ведь не моб!", false, ch, nullptr, vict, kToChar);
		return (0);
	}

	// Find the value of the argument
	bool on_off_mode{false};
	if (set_fields[mode].type == ESetValue::kBinary) {
		if (!strn_cmp(val_arg, "on", 2) || !strn_cmp(val_arg, "yes", 3) || !strn_cmp(val_arg, "вкл", 3)) {
			on_off_mode = true;
		} else if (!strn_cmp(val_arg, "off", 3) || !strn_cmp(val_arg, "no", 2) || !strn_cmp(val_arg, "выкл", 4)) {
			on_off_mode = false;
		} else {
			SendMsgToChar("Значение может быть 'on' или 'off'.\r\n", ch);
			return 0;
		}
		sprintf(output, "%s %s для %s.", set_fields[mode].cmd, (on_off_mode ? "ON" : "OFF"), GET_PAD(vict, 1));
	} else if (set_fields[mode].type == ESetValue::kNumber) {
		value = atoi(val_arg);
		sprintf(output, "У %s %s установлено в %d.", GET_PAD(vict, 1), set_fields[mode].cmd, value);
	} else {
		strcpy(output, "Хорошо.");
	}
	switch (mode) {
		case 0: on_off_mode ? vict->SetFlag(EPrf::kBrief) : vict->UnsetFlag(EPrf::kBrief);
			break;
		case 1: on_off_mode ? vict->SetFlag(EPlrFlag::kInvStart) : vict->UnsetFlag(EPlrFlag::kInvStart);
			break;
		case 2: on_off_mode ? vict->SetFlag(EPrf::KSummonable) : vict->UnsetFlag(EPrf::KSummonable);
			sprintf(output, "Возможность призыва %s для %s.\r\n", (on_off_mode ? "ON" : "OFF"), GET_PAD(vict, 1));
			break;
		case 3: vict->points.max_hit = std::clamp(value, 1, 5000);
			affect_total(vict);
			break;
		case 4: break;
		case 5: vict->points.max_move = std::clamp(value, 1, 5000);
			affect_total(vict);
			break;
		case 6: vict->points.hit = std::clamp(value, -9, vict->points.max_hit);
			affect_total(vict);
			update_pos(vict);
			break;
		case 7: [[fallthrough]];
		case 8: break;
		case 9:
			// Выставляется род для РС
			rod = PlayerRace::CheckRace(GET_KIN(ch), val_arg);
			if (rod == RACE_UNDEFINED) {
				SendMsgToChar("Не было таких на земле русской!\r\n", ch);
				SendMsgToChar(PlayerRace::ShowRacesMenu(GET_KIN(ch)), ch);
				return (0);
			} else {
				GET_RACE(vict) = rod;
				affect_total(vict);

			}
			break;
		case 10: vict->real_abils.size = std::clamp(value, 1, 100);
			affect_total(vict);
			break;
		case 11: vict->real_abils.armor = std::clamp(value, -100, 100);
			affect_total(vict);
			break;
		case 12: vict->set_gold(value);
			break;
		case 13: vict->set_bank(value);
			break;
		case 14: {
			auto new_value = static_cast<long>(value);
			new_value = std::clamp(new_value, 0L, GetExpUntilNextLvl(vict, kLvlImmortal) - 1) - vict->get_exp();
			gain_exp_regardless(vict, new_value);
			break;
		}
		case 15: vict->real_abils.hitroll = std::clamp(value, -20, 20);
			affect_total(vict);
			break;
		case 16: vict->real_abils.damroll = std::clamp(value, -20, 20);
			affect_total(vict);
			break;
		case 17:
			if (!IS_IMPL(ch) && ch != vict && !ch->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
				return (0);
			}
			SET_INVIS_LEV(vict, std::clamp(value, 0, GetRealLevel(vict)));
			break;
		case 18:
			if (!IS_IMPL(ch) && ch != vict && !ch->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
				return (0);
			}
			on_off_mode ? vict->SetFlag(EPrf::kNohassle) : vict->UnsetFlag(EPrf::kNohassle);
			break;
		case 19: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!strcmp(num, "0")) {
				if (!punishments::SetFreeze(ch, vict, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!punishments::SetFreeze(ch, vict, reason, times))
					return (0);
			}
			break;
		case 20:
		case 21: return_code = 0;
			break;
		case 22:
		case 23:
		case 24: {
			const unsigned circle_magic_number = mode - 22;
			if (circle_magic_number >= (ch)->player_specials->saved.conditions.size()) {
				SendMsgToChar("Ошибка: num >= saved.conditions.size(), сообщите кодерам.\r\n", ch);
				return 0;
			}
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				GET_COND(vict, circle_magic_number) = -1;
				sprintf(output, "Для %s %s сейчас отключен.", GET_PAD(vict, 1), set_fields[mode].cmd);
			} else if (is_number(val_arg)) {
				value = atoi(val_arg);
				value = std::clamp(value, 0, kMaxCondition);
				GET_COND(vict, circle_magic_number) = value;
				sprintf(output, "Для %s %s установлен в %d.", GET_PAD(vict, 1), set_fields[mode].cmd, value);
			} else {
				SendMsgToChar("Должно быть 'off' или значение от 0 до 24.\r\n", ch);
				return 0;
			}
			break;
		}
		case 25: on_off_mode ? vict->SetFlag(EPlrFlag::kBurglar) : vict->UnsetFlag(EPlrFlag::kBurglar);
			break;
		case 26:
			if (!ch->IsFlagged(EPrf::kCoderinfo)
				&& (value > GetRealLevel(ch) || value > kLvlImplementator || GetRealLevel(vict) > GetRealLevel(ch))) {
				SendMsgToChar("Вы не можете установить уровень игрока выше собственного.\r\n", ch);
				return (0);
			}
			value = std::clamp(value, 0, kLvlImplementator);
			vict->set_level(value);
			break;
		case 27:
			if ((rnum = GetRoomRnum(value)) == kNowhere) {
				SendMsgToChar("Поищите другой МУД. В этом МУДе нет такой комнаты.\r\n", ch);
				return (0);
			}
			if (vict->in_room != kNowhere) {
				RemoveCharFromRoom(vict);
			}
			PlaceCharToRoom(vict, rnum);
			vict->dismount();
			break;
		case 28: on_off_mode ? vict->SetFlag(EPrf::kRoomFlags) : vict->UnsetFlag(EPrf::kRoomFlags);
			break;
		case 29: on_off_mode ? vict->SetFlag(EPlrFlag::kSiteOk) : vict->UnsetFlag(EPlrFlag::kSiteOk);
			break;
		case 30:
			if (IS_IMPL(vict) || vict->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Истинные боги вечны!\r\n", ch);
				return 0;
			}
			on_off_mode ? vict->SetFlag(EPlrFlag::kDeleted) : vict->UnsetFlag(EPlrFlag::kDeleted);
			if (vict->IsFlagged(EPlrFlag::kDeleted)) {
				if (vict->IsFlagged(EPlrFlag::kNoDelete)) {
					vict->UnsetFlag(EPlrFlag::kNoDelete);
					SendMsgToChar("NODELETE flag also removed.\r\n", ch);
				}
			}
			break;
		case 31: {
			auto class_id = FindAvailableCharClassId(val_arg);
			if (class_id == ECharClass::kUndefined) {
				SendMsgToChar("Нет такого класса в этой игре. Найдите себе другую.\r\n", ch);
				return (0);
			}
			vict->set_class(class_id);
			break;
		}
		case 32:
			// Флаг для морталов с привилегиями
			if (!IS_IMPL(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
				return 0;
			}
			if (on_off_mode) {
				SET_GOD_FLAG(vict, EGf::kDemigod);
			} else {
				CLR_GOD_FLAG(vict, EGf::kDemigod);
			}
			break;
		case 33:
			if (is_number(val_arg)) {
				rvnum = atoi(val_arg);
				if (GetRoomRnum(rvnum) != kNowhere) {
					GET_LOADROOM(vict) = rvnum;
					sprintf(output, "%s будет входить в игру из комнаты #%d.",
							GET_NAME(vict), GET_LOADROOM(vict));
				} else {
					SendMsgToChar
						("Прежде чем кого-то куда-то поместить, надо это КУДА-ТО создать.\r\n"
						 "Скажите Стрибогу - пусть зоны рисует, а не пьянствует.\r\n", ch);
					return (0);
				}
			} else {
				SendMsgToChar("Должен быть виртуальный номер комнаты.\r\n", ch);
				return (0);
			}
			break;
		case 34: on_off_mode ? vict->SetFlag(EPrf::kColor1) : vict->UnsetFlag(EPrf::kColor1);
			on_off_mode ? vict->SetFlag(EPrf::kColor2) : vict->UnsetFlag(EPrf::kColor2);
			break;
		case 35:
			SendMsgToChar("Самое то чтоб поломать мад!\r\n", ch);
			return 0;
			break;
		case 36:
			if (!IS_IMPL(ch) && !ch->IsFlagged(EPrf::kCoderinfo) && ch != vict) {
				SendMsgToChar("Давайте не будем экспериментировать.\r\n", ch);
				return (0);
			}
			if (IS_IMPL(vict) && ch != vict) {
				SendMsgToChar("Вы не можете ЭТО изменить.\r\n", ch);
				return (0);
			}
			if (!Password::check_password(vict, val_arg)) {
				SendMsgToChar(ch, "%s\r\n", Password::BAD_PASSWORD);
				return 0;
			}
			Password::set_password(vict, val_arg);
			Password::send_password(GET_EMAIL(vict), val_arg, std::string(GET_NAME(vict)));
			sprintf(buf, "%s заменен пароль богом.", GET_PAD(vict, 2));
			AddKarma(vict, buf, GET_NAME(ch));
			sprintf(output, "Пароль изменен на '%s'.", val_arg);
			break;
		case 37: on_off_mode ? vict->SetFlag(EPlrFlag::kNoDelete) : vict->UnsetFlag(EPlrFlag::kNoDelete);
			break;
		case 38:
			if ((i = search_block(val_arg, genders, false)) < 0) {
				SendMsgToChar
					("Может быть 'мужчина', 'женщина', или 'бесполое'(а вот это я еще не оценил :).\r\n", ch);
				return (0);
			}
			vict->set_sex(static_cast<EGender>(i));
			break;

		case 39:        // set age
			if (value < 2 || value > 200)    // Arbitrary limits.
			{
				SendMsgToChar("Поддерживаются возрасты от 2 до 200.\r\n", ch);
				return (0);
			}
			/*
		 * NOTE: May not display the exact age specified due to the integer
		 * division used elsewhere in the code.  Seems to only happen for
		 * some values below the starting age (17) anyway. -gg 5/27/98
		 */
			vict->player_data.time.birth = time(nullptr) - ((value - 17) * kSecsPerMudYear);
			break;

		case 40:        // Blame/Thank Rick Glover. :)
			GET_HEIGHT(vict) = value;
			affect_total(vict);
			break;

		case 41: GET_WEIGHT(vict) = value;
			affect_total(vict);
			break;

		case 42:
			if (on_off_mode) {
				SET_GOD_FLAG(vict, EGf::kGodsLike);
				if (sscanf(val_arg, "%s %d", npad[0], &i) != 0)
					GCURSE_DURATION(vict) = (i > 0) ? time(nullptr) + i * 60 * 60 : MAX_TIME;
				else
					GCURSE_DURATION(vict) = 0;
				sprintf(buf, "%s установил GUDSLIKE персонажу %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, 0);

			} else {
				CLR_GOD_FLAG(vict, EGf::kGodsLike);
			}
			break;
		case 43:
			if (on_off_mode) {
				SET_GOD_FLAG(vict, EGf::kGodscurse);
				if (sscanf(val_arg, "%s %d", npad[0], &i) != 0) {
					GCURSE_DURATION(vict) = (i > 0) ? time(nullptr) + i * 60 * 60 : MAX_TIME;
				} else {
					GCURSE_DURATION(vict) = 0;
				}
			} else {
				CLR_GOD_FLAG(vict, EGf::kGodscurse);
			}
			break;
		case 44:
			if (ch->IsFlagged(EPrf::kCoderinfo) || IS_IMPL(ch))
				GET_OLC_ZONE(vict) = value;
			else {
				sprintf(buf, "Слишком низкий уровень чтоб раздавать права OLC.\r\n");
				SendMsgToChar(buf, ch);
			}
			break;

		case 45:
			// изменение имени !!!

			if ((i = sscanf(val_arg, "%s %s %s %s %s %s", npad[0], npad[1], npad[2], npad[3], npad[4], npad[5])) != 6) {
				sprintf(buf, "Требуется указать 6 падежей, найдено %d\r\n", i);
				SendMsgToChar(buf, ch);
				return (0);
			}

			if (*npad[0] == '*')    // Only change pads
			{
				for (i = ECase::kGen; i <= ECase::kLastCase; i++)
					if (!_parse_name(npad[i], npad[i])) {
						vict->player_data.PNames[i] = std::string(npad[i]);
					}
				sprintf(buf, "Произведена замена падежей.\r\n");
				SendMsgToChar(buf, ch);
			} else {
				for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
					if (strlen(npad[i]) < kMinNameLength || strlen(npad[i]) > kMaxNameLength) {
						sprintf(buf, "Падеж номер %d некорректен.\r\n", ++i);
						SendMsgToChar(buf, ch);
						return (0);
					}
				}

				if (_parse_name(npad[0], npad[0]) ||
					strlen(npad[0]) < kMinNameLength ||
					strlen(npad[0]) > kMaxNameLength ||
					!IsNameAvailable(npad[0]) || reserved_word(npad[0]) || fill_word(npad[0])) {
					SendMsgToChar("Некорректное имя.\r\n", ch);
					return (0);
				}

				if ((GetPlayerIdByName(npad[0]) >= 0) && !vict->IsFlagged(EPlrFlag::kDeleted)) {
					SendMsgToChar("Это имя совпадает с именем другого персонажа.\r\n"
								  "Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
					return (0);
				}
				// выносим из листа неодобренных имен, если есть
				NewNames::remove(vict);

				ptnum = GetPlayerTablePosByName(GET_NAME(vict));
				if (ptnum < 0)
					return (0);

				if (!vict->IsFlagged(EPlrFlag::kFrozen)
					&& !vict->IsFlagged(EPlrFlag::kDeleted)
					&& !IS_IMMORTAL(vict)) {
					TopPlayer::Remove(vict);
				}

				for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
					if (!_parse_name(npad[i], npad[i])) {
						vict->player_data.PNames[i] = std::string(npad[i]);
					}
				}
				sprintf(buf, "Name changed from %s to %s", GET_NAME(vict), npad[0]);
				vict->set_name(npad[0]);
				AddKarma(vict, buf, GET_NAME(ch));

				if (!vict->IsFlagged(EPlrFlag::kFrozen)
					&& !vict->IsFlagged(EPlrFlag::kDeleted)
					&& !IS_IMMORTAL(vict)) {
					TopPlayer::Refresh(vict);
				}

				player_table.SetName(ptnum, npad[0]);

				return_code = 2;
				vict->SetFlag(EPlrFlag::kCrashSave);
			}
			break;

		case 46:

			npad[1][0] = '\0';
			if (sscanf(val_arg, "%d %s %[^\n]", &ptnum, npad[0], npad[1]) != 3) {
				if (sscanf(val_arg, "%d %s", &ptnum, npad[0]) != 2) {
					SendMsgToChar("Формат : set <имя> trgquest <quest_num> <on|off> <строка данных>\r\n", ch);
					return 0;
				}
			}

			if (!str_cmp(npad[0], "off") || !str_cmp(npad[0], "выкл")) {
				if (!vict->quested_remove(ptnum)) {
					act("$N не выполнял$G этого квеста.", false, ch, nullptr, vict, kToChar);
					return 0;
				}
			} else if (!str_cmp(npad[0], "on") || !str_cmp(npad[0], "вкл")) {
				vict->quested_add(vict, ptnum, npad[1]);
			} else {
				SendMsgToChar("Требуется on или off.\r\n", ch);
				return 0;
			}
			break;

		case 47:

			if (sscanf(val_arg, "%d %s", &ptnum, npad[0]) != 2) {
				SendMsgToChar("Формат : set <имя> mkill <MobVnum> <off|num>\r\n", ch);
				return (0);
			}
			if (!str_cmp(npad[0], "off") || !str_cmp(npad[0], "выкл"))
				vict->mobmax_remove(ptnum);
			else if ((j = atoi(npad[0])) > 0) {
				if ((c = vict->mobmax_get(ptnum)) != j)
					vict->mobmax_add(vict, ptnum, j - c, MobMax::get_level_by_vnum(ptnum));
				else {
					act("$N убил$G именно столько этих мобов.", false, ch, nullptr, vict, kToChar);
					return (0);
				}
			} else {
				SendMsgToChar("Требуется off или значение больше 0.\r\n", ch);
				return (0);
			}
			break;

		case 48: return (0);
			break;

		case 49: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!str_cmp(num, "0")) {
				if (!punishments::SetHell(ch, vict, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан неверно.\r\n");
				return (0);
			} else {
				if (!punishments::SetHell(ch, vict, reason, times))
					return (0);
			}
			break;
		case 50:
			if (IsValidEmail(val_arg)) {
				utils::ConvertToLow(val_arg);
				sprintf(buf, "Email changed from %s to %s", GET_EMAIL(vict), val_arg);
				AddKarma(vict, buf, GET_NAME(ch));
				strncpy(GET_EMAIL(vict), val_arg, 127);
				*(GET_EMAIL(vict) + 127) = '\0';
			} else {
				SendMsgToChar("Wrong E-Mail.\r\n", ch);
				return (0);
			}
			break;

		case 51:
			// Выставляется род для РС
			rod = (*val_arg);
			if (rod != '0' && rod != '1') {
				SendMsgToChar("Не было таких на земле русской!\r\n", ch);
				SendMsgToChar("0 - Язычество, 1 - Христианство\r\n", ch);
				return (0);
			} else {
				GET_RELIGION(vict) = rod - '0';
			}
			break;

		case 52:
			// Отдельный лог команд персонажа
			if (on_off_mode) {
				SET_GOD_FLAG(vict, EGf::kPerslog);
			} else {
				CLR_GOD_FLAG(vict, EGf::kPerslog);
			}
			break;

		case 53: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!str_cmp(num, "0")) {
				if (!punishments::SetMute(ch, vict, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!punishments::SetMute(ch, vict, reason, times))
					return (0);
			}
			break;

		case 54: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!str_cmp(num, "0")) {
				if (!punishments::SetDumb(ch, vict, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!punishments::SetDumb(ch, vict, reason, times))
					return (0);
			}
			break;
		case 55:
			if (GetRealLevel(vict) >= kLvlImmortal && !IS_IMPL(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
				return 0;
			}
			reason = strdup(val_arg);
			if (reason && *reason) {
				skip_spaces(&reason);
				sprintf(buf, "add by %s", GET_NAME(ch));
				if (!strcmp(reason, "clear")) {
					if KARMA(vict)
						free(KARMA(vict));

					KARMA(vict) = nullptr;
					act("Вы отпустили $N2 все грехи.", false, ch, nullptr, vict, kToChar);
					sprintf(buf, "%s", GET_NAME(ch));
					AddKarma(vict, "Очистка грехов", buf);

				} else AddKarma(vict, buf, reason);
			} else {
				SendMsgToChar("Формат команды: set [ file | player ] <character> karma <reason>\r\n", ch);
				return (0);
			}
			break;

		case 56:      // Разрегистрация персонажа
			reason = one_argument(val_arg, num);
			if (!punishments::SetUnregister(ch, vict, reason, 0)) return (0);
			break;

		case 57:      // Установка флага палач
			reason = one_argument(val_arg, num);
			skip_spaces(&reason);
			sprintf(buf, "executor %s by %s", (on_off_mode ? "on" : "off"), GET_NAME(ch));
//			AddKarma(vict, buf, reason);
			if (on_off_mode) {
				vict->SetFlag(EPrf::kExecutor);
			} else {
				vict->UnsetFlag(EPrf::kExecutor);
			}
			break;

		case 58: on_off_mode ? vict->SetFlag(EPlrFlag::kKiller) : vict->UnsetFlag(EPlrFlag::kKiller);
			break;
		case 59: // флаг реморта
			if (value > 1 && value < 75) {
				sprintf(buf, "Иммортал %s установил реморт %d  для игрока %s ", GET_NAME(ch), value, GET_NAME(vict));
				AddKarma(vict, buf, GET_NAME(ch));
				AddKarma(ch, buf, GET_NAME(vict));
				vict->set_remort(value);
				SendMsgToGods(buf);
			} else {
				SendMsgToChar(ch, "Неправильно указан реморт.\r\n");
			}
			break;
		case 60: // флаг тестера
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				CLR_GOD_FLAG(vict, EGf::kAllowTesterMode);
				vict->UnsetFlag(EPrf::kTester); // обнулим реж тестер
				sprintf(buf, "%s убрал флаг тестера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			} else {
				SET_GOD_FLAG(vict, EGf::kAllowTesterMode);
				sprintf(buf, "%s установил флаг тестера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				//			send_to_gods(buf);
			}
			break;
		case 61: on_off_mode ? vict->SetFlag(EPlrFlag::kAutobot) : vict->UnsetFlag(EPlrFlag::kAutobot);
			break;
		case 62: vict->set_hryvn(value);
			break;
		case 63: // флаг скриптера
			sprintf(buf, "%s", GET_NAME(ch));
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				vict->UnsetFlag(EPlrFlag::kScriptWriter);
				AddKarma(vict, "Снятие флага скриптера", buf);
				sprintf(buf, "%s убрал флаг скриптера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				return (1);
			} else if (!str_cmp(val_arg, "on") || !str_cmp(val_arg, "вкл")) {
				vict->SetFlag(EPlrFlag::kScriptWriter);
				AddKarma(vict, "Установка флага скриптера", buf);
				sprintf(buf, "%s установил  флаг скриптера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				return (1);
			} else {
				SendMsgToChar(ch, "Значение может быть только on/off или вкл/выкл.\r\n");
				return (0); // не пишем в пфайл ничего
			}
			break;
		case 64: on_off_mode ? vict->SetFlag(EPlrFlag::kSpamer) : vict->UnsetFlag(EPlrFlag::kSpamer);
			break;
		case 65: { // спрятать отображение славы
			if (!str_cmp(val_arg, "on") || !str_cmp(val_arg, "вкл")) {
				GloryConst::glory_hide(vict, true);
			} else {
				GloryConst::glory_hide(vict, false);
			}
			break;
		}
		case 66: { // идентификатор чата телеграма

			unsigned long int id = strtoul(val_arg, nullptr, 10);
			if (!ch->IsNpc() && id != 0) {
				sprintf(buf, "Telegram chat_id изменен с %lu на %lu\r\n", vict->player_specials->saved.telegram_id, id);
				SendMsgToChar(buf, ch);
				vict->setTelegramId(id);
			} else
				SendMsgToChar("Ошибка, указано неверное число или персонаж.\r\n", ch);
			break;
		}
		case 67: vict->set_nogata(value);
			break;
		case 68: {
			auto tmpval = (EPosition) value;
			if (tmpval > EPosition::kDead && tmpval < EPosition::kLast) {
				sprinttype(value, position_types, smallBuf);
				sprintf(buf, "Для персонажа %s установлена позиция: %s.\r\n", GET_NAME(vict), smallBuf);
				SendMsgToChar(buf, ch);
				vict->SetPosition(tmpval);
			} else {
				const auto msg = fmt::format("Позиция может принимать значения от {} до {}.\r\n",
											 static_cast<int>(EPosition::kPerish), static_cast<int>(EPosition::kStand));
				SendMsgToChar(msg, ch);
				return 0;
			}
			break;
		}
		case 69: // флаг скилл тестера
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				CLR_GOD_FLAG(vict, EGf::kSkillTester);
				sprintf(buf, "%s убрал флаг &Rскилл тестера&n для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			} else {
				SET_GOD_FLAG(vict, EGf::kSkillTester);
				sprintf(buf, "%s установил флаг &Rскилл тестера&n для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			}
			break;
		case 70: //quest
			vict->complete_quest(value);
			SendMsgToChar(ch, "Добавил игроку %s выполнение квеста %d\r\n", GET_NAME(vict), value);
			break;
		default: SendMsgToChar("Не могу установить это!\r\n", ch);
			return (0);
	}

	strcat(output, "\r\n");
	SendMsgToChar(utils::CAP(output), ch);
	return (return_code);
}

void RenamePlayer(CharData *ch, char *oname) {
	char filename[kMaxInputLength], ofilename[kMaxInputLength];

	// 1) Rename(if need) char and pkill file - directly
	log("Rename char %s->%s", GET_NAME(ch), oname);
	get_filename(oname, ofilename, kPlayersFile);
	get_filename(GET_NAME(ch), filename, kPlayersFile);
	rename(ofilename, filename);

	ch->save_char();

	// 2) Rename all other files
	get_filename(oname, ofilename, kTextCrashFile);
	get_filename(GET_NAME(ch), filename, kTextCrashFile);
	rename(ofilename, filename);

	get_filename(oname, ofilename, kTimeCrashFile);
	get_filename(GET_NAME(ch), filename, kTimeCrashFile);
	rename(ofilename, filename);

	get_filename(oname, ofilename, kAliasFile);
	get_filename(GET_NAME(ch), filename, kAliasFile);
	rename(ofilename, filename);

	get_filename(oname, ofilename, kScriptVarsFile);
	get_filename(GET_NAME(ch), filename, kScriptVarsFile);
	rename(ofilename, filename);

	// хранилища
	Depot::rename_char(ch);
	get_filename(oname, ofilename, kPersDepotFile);
	get_filename(GET_NAME(ch), filename, kPersDepotFile);
	rename(ofilename, filename);
	get_filename(oname, ofilename, kPurgeDepotFile);
	get_filename(GET_NAME(ch), filename, kPurgeDepotFile);
	rename(ofilename, filename);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
