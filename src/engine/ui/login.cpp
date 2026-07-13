/**
 \file login.cpp - a part of the Bylins engine.
 \brief issue.interpreter-cleaning: pre-game connection dialogue (login / menu / character creation),
        extracted from interpreter.cpp. Entry point ProcessLoginInput (was nanny).
*/
#include "interpreter.h"
#include "engine/ui/system_messages.h"
#include "engine/core/config.h"
#include "gameplay/mechanics/condition.h"
#include "administration/dupe_check.h"

#include "engine/core/char_movement.h"
#include "administration/ban.h"
#include "administration/karma.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "gameplay/communication/insult.h"
#include "gameplay/communication/offtop.h"
#ifdef ENABLE_ADMIN_API
#include "engine/network/admin_api.h"
#endif
#include "engine/ui/cmd/do_telegram.h"
#include "engine/core/comm.h"
#include "gameplay/core/constants.h"
#include "gameplay/crafting/craft_commands.h"
#include "gameplay/crafting/fry.h"
#include "gameplay/crafting/jewelry.h"
#include "engine/db/db.h"
#include "gameplay/mechanics/depot.h"
#include "engine/scripting/dg_scripts.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/fight/assist.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/fight/pk.h"
#include "gameplay/core/genchar.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/mechanics/glory_misc.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/char_handler.h"
#include "gameplay/mechanics/illumination.h"
#include "utils/utils_parse.h"
#include "engine/core/heartbeat_commands.h"
#include "gameplay/clans/house.h"
#include "gameplay/crafting/item_creation.h"
#include "utils/logger.h"
#include "gameplay/communication/mail.h"
#include "modify.h"
#include "gameplay/mechanics/named_stuff.h"
#include "administration/names.h"
#include "engine/entities/obj_data.h"
#include "engine/db/obj_prototypes.h"
#include "engine/olc/olc.h"
#include "gameplay/communication/parcel.h"
#include "administration/password.h"
#include "administration/privilege.h"
#include "engine/entities/room_data.h"
#include "engine/network/logon.h"
#include "engine/ui/color.h"
#include "gameplay/skills/armoring.h"
#include "gameplay/skills/skills.h"
#include "gameplay/skills/backstab.h"
#include "gameplay/skills/bash.h"
#include "gameplay/skills/shield_block.h"
#include "gameplay/skills/disarm.h"
#include "gameplay/skills/deviate.h"
#include "gameplay/skills/fit.h"
#include "gameplay/skills/firstaid.h"
#include "gameplay/skills/intercept.h"
#include "gameplay/skills/ironwind.h"
#include "gameplay/skills/kick.h"
#include "gameplay/skills/lightwalk.h"
#include "gameplay/skills/manadrain.h"
#include "gameplay/skills/mighthit.h"
#include "gameplay/skills/multyparry.h"
#include "gameplay/skills/parry.h"
#include "gameplay/skills/poisoning.h"
#include "gameplay/skills/protect.h"
#include "gameplay/skills/repair.h"
#include "gameplay/skills/resque.h"
#include "gameplay/skills/sharpening.h"
#include "gameplay/skills/strangle.h"
#include "gameplay/skills/stun.h"
#include "gameplay/skills/overhelm.h"
#include "gameplay/skills/throw.h"
#include "gameplay/skills/throwout.h"
#include "gameplay/skills/track.h"
#include "gameplay/skills/turnundead.h"
#include "gameplay/skills/warcry.h"
#include "gameplay/skills/relocate.h"
#include "gameplay/skills/repair.h"
#include "gameplay/skills/skinning.h"
#include "gameplay/skills/spell_capable.h"
#include "gameplay/mechanics/title.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/skills/townportal.h"
#include "gameplay/mechanics/mem_queue.h"
#include "engine/db/obj_save.h"
#include "engine/core/iosystem.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/player_races.h"
#include "engine/db/help.h"
#include "mapsystem.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/core/reset_stats.h"
#include "gameplay/mechanics/obj_sets.h"
#include "utils/utils.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "gameplay/mechanics/bonus.h"
#include "utils/utils_debug.h"
#include "engine/db/global_objects.h"
#include "administration/accounts.h"
#include "gameplay/fight/pk.h"
#include "gameplay/skills/slay.h"
#include "gameplay/skills/charge.h"
#include "gameplay/skills/dazzle.h"
#include "gameplay/mechanics/cities.h"
#include "administration/proxy.h"
#include "gameplay/communication/check_invoice.h"
#include "gameplay/mechanics/doors.h"
#include "gameplay/skills/frenzy.h"
#include "gameplay/mechanics/groups.h"
#include "alias.h"
#include "engine/db/player_index.h"
#include "gameplay/core/remort.h"

#include <ctime>

#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif

#include <fmt/format.h>

#include <memory>
#include <stdexcept>
#include <algorithm>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern RoomRnum r_frozen_start_room;
extern const char *religion_menu;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern const char *default_race[];
extern struct PCCleanCriteria pclean_criteria[];
extern int rent_file_timeout;

extern struct show_struct show_fields[];


// external functions
void read_saved_vars(CharData *ch);
void oedit_parse(DescriptorData *d, char *arg);
void redit_parse(DescriptorData *d, char *arg);
void zedit_parse(DescriptorData *d, char *arg);
void medit_parse(DescriptorData *d, char *arg);
void trigedit_parse(DescriptorData *d, char *arg);
void init_warcry(CharData *ch);
#include "engine/ui/login.h"
#include "engine/olc/vedun/vedun.h"

// Defined here (sole user) rather than in interpreter.cpp -- issue.interpreter-cleaning Bucket 5.
// здесь храним коды, которые отправили игрокам на почту
// строка - это мыло, если один чар вошел с необычного места, то блочим сразу всех чаров на этом мыле,
// пока не введет код (или до ребута)
std::map<std::string, int> new_loc_codes;

// имя чара на код, отправленный на почту для подтверждения мыла при создании
std::map<std::string, int> new_char_codes;

int _parse_name(char *argument, char *name) {
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*argument) : UPPER(*argument))); argument++, i++, name++) {
		if (*argument == 'ё'
			|| *argument == 'Ё'
			|| !a_isalpha(*argument)
			|| *argument > 0) {
			return (1);
		}
	}

	if (!i) {
		return (1);
	}

	return (0);
}

/**
* Вобщем это пока дублер старого _parse_name для уже созданных ранее чаров,
* чтобы их в игру вообще пускало, а новых с Ё/ё соответственно брило.
*/
int parse_exist_name(char *argument, char *name) {
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*argument) : UPPER(*argument))); argument++, i++, name++)
		if (!a_isalpha(*argument) || *argument > 0)
			return (1);

	if (!i)
		return (1);

	return (0);
}

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
// Connection-takeover modes + per-codepage encoding hint strings (moved from interpreter.cpp,
// issue.after-interpreter-cleaning: their only users -- perform_dupe_check / ShowEncodingPrompt --
// live here now).
enum Mode {
  UNDEFINED,
  RECON,
  USURP,
  UNSWITCH
};

// Фраза 'Русская азбука: "абв...эюя".' в разных кодировках и для разных клиентов
#define ENC_HINT_KOI8R          "\xf2\xd5\xd3\xd3\xcb\xc1\xd1 \xc1\xda\xc2\xd5\xcb\xc1: \"\xc1\xc2\xd7...\xdc\xc0\xd1\"."
#define ENC_HINT_ALT            "\x90\xe3\xe1\xe1\xaa\xa0\xef \xa0\xa7\xa1\xe3\xaa\xa0: \"\xa0\xa1\xa2...\xed\xee\xef\"."
#define ENC_HINT_WIN            "\xd0\xf3\xf1\xf1\xea\xe0\xff\xff \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfe\xff\xff\"."
// обход ошибки с 'я' в zMUD после ver. 6.39+ и CMUD без замены 'я' на 'z'
#define ENC_HINT_WIN_ZMUD       "\xd0\xf3\xf1\xf1\xea\xe0\xff\xff? \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfe\xff\xff?\"."
// замена 'я' на 'z' в zMUD до ver. 6.39a для обхода ошибки,
// а также в zMUD после ver. 6.39a для совместимости
#define ENC_HINT_WIN_ZMUD_z     "\xd0\xf3\xf1\xf1\xea\xe0z \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfez\"."
#define ENC_HINT_WIN_ZMUD_old   ENC_HINT_WIN_ZMUD_z
#define ENC_HINT_UTF8           "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb0\xd1\x8f "\
                                "\xd0\xb0\xd0\xb7\xd0\xb1\xd1\x83\xd0\xba\xd0\xb0: "\
                                "\"\xd0\xb0\xd0\xb1\xd0\xb2...\xd1\x8d\xd1\x8e\xd1\x8f\"."

int perform_dupe_check(DescriptorData *d) {
	DescriptorData *k, *next_k;
	Mode mode = UNDEFINED;

	int id = d->character->get_uid();

	/*
	   * Now that this descriptor has successfully logged in, disconnect all
	   * other descriptors controlling a character with the same ID number.
	   */

	CharData::shared_ptr target;
	for (k = descriptor_list; k; k = next_k) {
		next_k = k->next;
		if (k == d) {
			continue;
		}

		if (k->original && (k->original->get_uid() == id))    // switched char
		{
			if (str_cmp(d->host, k->host)) {
				sprintf(buf, "ПОВТОРНЫЙ ВХОД! Id = %ld Персонаж = %s Хост = %s(был %s)",
						d->character->get_uid(), GET_NAME(d->character), k->host, d->host);
				mudlog(buf, BRF, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
				//send_to_gods(buf);
			}

			iosystem::write_to_output("\r\nПопытка второго входа - отключаемся.\r\n", k);
			k->state = EConState::kClose;

			if (!target) {
				target = k->original;
				mode = UNSWITCH;
			}

			if (k->character) {
				k->character->desc = nullptr;
			}

			k->character = nullptr;
			k->original = nullptr;
		} else if (k->character && (k->character->get_uid() == id)) {
			if (str_cmp(d->host, k->host)) {
				sprintf(buf, "ПОВТОРНЫЙ ВХОД! Id = %ld Name = %s Host = %s(был %s)",
						d->character->get_uid(), GET_NAME(d->character), k->host, d->host);
				mudlog(buf, BRF, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
				//send_to_gods(buf);
			}

			if (!target &&  k->state == EConState::kPlaying) {
				iosystem::write_to_output("\r\nВаше тело уже кем-то занято!\r\n", k);
				target = k->character;
				mode = USURP;
			}
			k->character->desc = nullptr;
			k->character = nullptr;
			k->original = nullptr;
			iosystem::write_to_output("\r\nПопытка второго входа - отключаемся.\r\n", k);
			k->state = EConState::kClose;
		}
	}

	/*
	   * now, go through the character list, deleting all characters that
	   * are not already marked for deletion from the above step (i.e., in the
	   * CON_HANGUP state), and have not already been selected as a target for
	   * switching into.  In addition, if we haven't already found a target,
	   * choose one if one is available (while still deleting the other
	   * duplicates, though theoretically none should be able to exist).
	   */

	character_list.foreach([&target, &mode, id](const CharData::shared_ptr &ch) {
	  if (ch->IsNpc()) {
		  return;
	  }

	  if (ch->get_uid() != id) {
		  return;
	  }

	  // ignore entities with descriptors (already handled by above step) //
	  if (ch->desc)
		  return;

	  // don't extract the target char we've found one already //
	  if (ch == target)
		  return;

	  // we don't already have a target and found a candidate for switching //
	  if (!target) {
		  target = ch;
		  mode = RECON;
		  return;
	  }

	  // we've found a duplicate - blow him away, dumping his eq in limbo. //
	  if (ch->in_room != kNowhere) {
		  char_from_room(ch);
	  }
		char_to_room(ch, kStrangeRoom);
		mudlog(fmt::format("Обнаружен дубликат игрока %s, перемещен в ад и очищен.", ch->get_name().c_str()));
		character_list.AddToExtractedList(ch.get());
	});

	// no target for switching into was found - allow login to continue //
	if (!target) {
		return 0;
	}

	// Okay, we've found a target.  Connect d to target. //

	d->character = target;
	d->character->desc = d;
	d->original = nullptr;
	d->character->char_specials.timer = 0;
	d->character->UnsetFlag(EPlrFlag::kMailing);
	d->character->UnsetFlag(EPlrFlag::kWriting);
	d->state = EConState::kPlaying;

	switch (mode) {
		case RECON: iosystem::write_to_output("Пересоединяемся.\r\n", d);
			CheckLight(d->character.get(), kLightNo, kLightNo, kLightNo, kLightNo, 1);
			act("$n восстановил$g связь.",
				true, d->character.get(), nullptr, nullptr, kToRoom);
			sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			login_change_invoice(d->character.get());
			break;

		case USURP: iosystem::write_to_output("Ваша душа вновь вернулась в тело, которое так ждало ее возвращения!\r\n", d);
			act("$n надломил$u от боли, окруженн$w белой аурой...\r\n"
				"Тело $s было захвачено новым духом!",
				true, d->character.get(), nullptr, nullptr, kToRoom);
			sprintf(buf, "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;

		case UNSWITCH: iosystem::write_to_output("Пересоединяемся для перевключения игрока.", d);
			sprintf(buf, "%s [%s] has reconnected (UNSWITCH).", GET_NAME(d->character), d->host);
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;

		default:
			// ??? what does this case mean ???
			break;
	}

	network::add_logon_record(d);
	return 1;
}

int pre_help(CharData *ch, char *argument) {
	char command[kMaxInputLength], topic[kMaxInputLength];

	half_chop(argument, command, topic);

	if (!*command || strlen(command) < 2 || !*topic || strlen(topic) < 2)
		return (0);
	if (isname(command, "помощь help справка")) {
		do_help(ch, topic, 0, 0);
		return (1);
	}
	return (0);
}

// * Проверка на доступные религии конкретной профе (из текущей генерации чара).
void check_religion(CharData *ch) {
	if (class_religion[to_underlying(ch->GetClass())] == kReligionPoly && GET_RELIGION(ch) != kReligionPoly) {
		GET_RELIGION(ch) = kReligionPoly;
		log("Change religion to poly: %s", ch->get_name().c_str());
	} else if (class_religion[to_underlying(ch->GetClass())] == kReligionMono && GET_RELIGION(ch) != kReligionMono) {
		GET_RELIGION(ch) = kReligionMono;
		log("Change religion to mono: %s", ch->get_name().c_str());
	}
}

void do_entergame(DescriptorData *d) {
	int load_room, cmd, flag = 0;

	d->character->reset();
	ReadAliases(d->character.get());

	if (GetRealLevel(d->character) == kLvlImmortal) {
		d->character->set_level(kLvlGod);
	}

	if (GetRealLevel(d->character) > kLvlImplementator) {
		d->character->set_level(1);
	}

	if (GET_INVIS_LEV(d->character) > kLvlImplementator
		|| GET_INVIS_LEV(d->character) < 0) {
		SET_INVIS_LEV(d->character, 0);
	}

	if (GetRealLevel(d->character) > kLvlImmortal
		&& GetRealLevel(d->character) < kLvlBuilder
		&& (currencies::GetHand(*d->character, currencies::kGold) > 0 || currencies::GetBank(*d->character, currencies::kGold) > 0)) {
		currencies::SetHand(*d->character, currencies::kGold, 0);
		currencies::SetBank(*d->character, currencies::kGold, 0);
	}

	if (GetRealLevel(d->character) >= kLvlImmortal && GetRealLevel(d->character) < kLvlImplementator) {
		for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
			if (!strcmp(cmd_info[cmd].command, "syslog")) {
				if (privilege::HasPrivilege(d->character.get(), std::string(cmd_info[cmd].command), cmd, 0)) {
					flag = 1;
					break;
				}
			}
		}

		if (!flag) {
			GET_LOGS(d->character)[0] = 0;
		}
	}

	if (GetRealLevel(d->character) < kLvlImplementator) {
		if (d->character->IsFlagged(EPlrFlag::kInvStart)) {
			SET_INVIS_LEV(d->character, kLvlImmortal);
		}
		if (GET_INVIS_LEV(d->character) > GetRealLevel(d->character)) {
			SET_INVIS_LEV(d->character, GetRealLevel(d->character));
		}

		if (d->character->IsFlagged(EPrf::kCoderinfo)) {
			d->character->UnsetFlag(EPrf::kCoderinfo);
		}
		if (GetRealLevel(d->character) < kLvlGod) {
			if (d->character->IsFlagged(EPrf::kHolylight)) {
				d->character->UnsetFlag(EPrf::kHolylight);
			}
		}
		if (GetRealLevel(d->character) < kLvlGod) {
			if (d->character->IsFlagged(EPrf::kNohassle)) {
				d->character->UnsetFlag(EPrf::kNohassle);
			}
			if (d->character->IsFlagged(EPrf::kRoomFlags)) {
				d->character->UnsetFlag(EPrf::kRoomFlags);
			}
		}

		if (GET_INVIS_LEV(d->character) > 0
			&& GetRealLevel(d->character) < kLvlImmortal) {
			SET_INVIS_LEV(d->character, 0);
		}
	}

	offtop_system::SetStopOfftopFlag(d->character.get());
	// пересчет максимального хп, если нужно
	check_max_hp(d->character.get());
	// проверка и сет религии
	check_religion(d->character.get());

	/*
	   * We have to place the character in a room before equipping them
	   * or equip_char() will gripe about the person in kNowhere.
	   */
	if (d->character->IsFlagged(EPlrFlag::kHelled))
		load_room = r_helled_start_room;
	else if (d->character->IsFlagged(EPlrFlag::kNameDenied))
		load_room = r_named_start_room;
	else if (d->character->IsFlagged(EPlrFlag::kFrozen))
		load_room = r_frozen_start_room;
	else if (!check_dupes_host(d))
		load_room = r_unreg_start_room;
	else {
		if ((load_room = GET_LOADROOM(d->character)) == kNowhere) {
			load_room = calc_loadroom(d->character.get());
		}
		load_room = GetRoomRnum(load_room);

		if (!Clan::MayEnter(d->character.get(), load_room, kHousePortal)) {
			load_room = Clan::CloseRent(load_room);
		}

		if (!is_rent(load_room)) {
			load_room = kNowhere;
		}
	}

	if (load_room == kNowhere) {
		if (GetRealLevel(d->character) >= kLvlImmortal)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}

	SendMsgToChar(system_messages::GetText(system_messages::ESystemMsg::kWelcome), d->character.get());

	CharData *character = nullptr;
	for (const auto &character_i : character_list) {
		if (character_i == d->character) {
			character = character_i.get();
			break;
		}
	}

	if (!character) {
		character_list.push_front(d->character);
	} else {
		character->UnsetFlag(EMobFlag::kMobDeleted);
		character->UnsetFlag(EMobFlag::kMobFreed);
	}

	// а потом уже вычитаем за ренту
	if (GetRealLevel(d->character) != 0) {
		Crash_load(d->character.get());
		d->character->obj_bonus().update(d->character.get());
	}

	Depot::enter_char(d->character.get());
	Glory::check_freeze(d->character.get());
	Clan::clan_invoice(d->character.get(), true);

	// Чистим стили если не знаем их
	if (d->character->IsFlagged(EPrf::kShadowThrow)) {
		d->character->UnsetFlag(EPrf::kShadowThrow);
	}

	if (d->character->IsFlagged(EPrf::kPunctual)
		&& !GetSkill(d->character.get(), ESkill::kPunctual)) {
		d->character->UnsetFlag(EPrf::kPunctual);
	}

	if (d->character->IsFlagged(EPrf::kAwake)
		&& !GetSkill(d->character.get(), ESkill::kAwake)) {
		d->character->UnsetFlag(EPrf::kAwake);
	}

	if (d->character->IsFlagged(EPrf::kPerformPowerAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kPowerAttack)) {
		d->character->UnsetFlag(EPrf::kPerformPowerAttack);
	}
	if (d->character->IsFlagged(EPrf::kPerformGreatPowerAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kGreatPowerAttack)) {
		d->character->UnsetFlag(EPrf::kPerformGreatPowerAttack);
	}
	if (d->character->IsFlagged(EPrf::kPerformAimingAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kAimingAttack)) {
		d->character->UnsetFlag(EPrf::kPerformAimingAttack);
	}
	if (d->character->IsFlagged(EPrf::kPerformGreatAimingAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kGreatAimingAttack)) {
		d->character->UnsetFlag(EPrf::kPerformGreatAimingAttack);
	}
	if (d->character->IsFlagged(EPrf::kDoubleThrow) &&
		!CanUseFeat(d->character.get(), EFeat::kDoubleThrower)) {
		d->character->UnsetFlag(EPrf::kDoubleThrow);
	}
	if (d->character->IsFlagged(EPrf::kTripleThrow) &&
		!CanUseFeat(d->character.get(), EFeat::kTripleThrower)) {
		d->character->UnsetFlag(EPrf::kTripleThrow);
	}
	if (d->character->IsFlagged(EPrf::kPerformSerratedBlade) &&
		!CanUseFeat(d->character.get(), EFeat::kSerratedBlade)) {
		d->character->UnsetFlag(EPrf::kPerformSerratedBlade);
	}
	if (d->character->IsFlagged(EPrf::kSkirmisher)) {
		d->character->UnsetFlag(EPrf::kSkirmisher);
	}
	if (d->character->IsFlagged(EPrf::kIronWind)) {
		d->character->UnsetFlag(EPrf::kIronWind);
	}

	// Check & remove/add natural, race & unavailable features
	UnsetInaccessibleFeats(d->character.get());
	SetInbornAndRaceFeats(d->character.get());
	if (!privilege::IsImmortal(d->character.get())) {
		for (const auto &skill : MUD::Skills()) {
			if (MUD::Class((d->character)->GetClass()).skills[skill.GetId()].IsInvalid()) {
				SetSkill(d->character.get(), skill.GetId(), 0);
			}
		}

		for (const auto &spell : MUD::Spells()) {
			if (IS_SPELL_SET(d->character, spell.GetId(), ESpellType::kKnow)) {
				if (MUD::Class((d->character)->GetClass()).spells[spell.GetId()].IsInvalid()) {
					UNSET_SPELL_TYPE(d->character, spell.GetId(), ESpellType::kKnow);
				}
			}
		}
	}

	temporary_spells::update_char_times(d->character.get(), time(nullptr));

	// Сбрасываем явно не нужные аффекты.
	d->character->remove_affect(EAffect::kGroup);
	d->character->remove_affect(EAffect::kHorse);

	DeleteIrrelevantRunestones(d->character.get());

	// with the copyover patch, this next line goes in enter_player_game()
	chardata_by_uid[d->character->get_uid()] = d->character.get();
	GET_ACTIVITY(d->character) = number(0, kPlayerSaveActivity - 1);
	d->character->set_last_logon(time(nullptr));
//	player_table[GetPtableByUnique(d->character->get_uid())].last_logon = d->character->get_last_logon();
	player_table[d->character->get_pfilepos()].last_logon = d->character->get_last_logon();
	network::add_logon_record(d);
	// чтобы восстановление маны спам-контроля "кто" не шло, когда чар заходит после
	// того, как повисел на менюшке; важно, чтобы этот вызов шел раньше save_char()
	d->character->set_who_last(time(nullptr));
	d->character->save_char();
	// with the copyover patch, this next line goes in enter_player_game()
	read_saved_vars(d->character.get());
	enter_wtrigger(world[d->character->in_room], d->character.get(), -1);
	greet_mtrigger(d->character.get(), -1);
	greet_otrigger(d->character.get(), -1);
	d->state = EConState::kPlaying;
	d->character->SetFlag(EPrf::kColor2); // цвет всегда полный
// режимы по дефолту у нового чара
	const bool new_char = d->character->GetLevel() <= 0;
	if (new_char) {
		d->character->SetFlag(EPrf::kDrawMap);
		d->character->SetFlag(EPrf::kGoAhead); //IAC GA
		d->character->SetFlag(EPrf::kAutomem);
		d->character->SetFlag(EPrf::kAutoloot);
		d->character->SetFlag(EPrf::kPklMode);
		d->character->SetFlag(EPrf::kClanmembersMode); // соклан
		// По умолчанию для нового игрока включаем всё, кроме depth-флагов
		// и godmode, аналогично пункту "включить все" в OLC карты (#3202).
		for (int i = 0; i < MapSystem::TOTAL_MAP_OPTIONS; ++i) {
			if (i == MapSystem::MAP_MODE_1_DEPTH
				|| i == MapSystem::MAP_MODE_2_DEPTH
				|| i == MapSystem::MAP_MODE_DEPTH_FIXED
				|| i == MapSystem::MAP_MODE_GOD_BIG) {
				continue;
			}
			d->character->map_set_option(i);
		}
		d->character->SetFlag(EPrf::kShowZoneNameOnEnter);
		d->character->SetFlag(EPrf::kBoardMode);
		d->character->set_last_exchange(time(nullptr));
		DoPcInit(d->character.get(), true);
		d->character->mem_queue.stored = 0;
		SendMsgToChar(system_messages::GetText(system_messages::ESystemMsg::kStartMessage), d->character.get());
	}

	init_warcry(d->character.get());

	// На входе в игру вешаем флаг (странно, что он до этого нигде не вешался
	if (privilege::IsContainedInGodsList(GET_NAME(d->character), d->character->get_uid())
		&& (GetRealLevel(d->character) < kLvlGod)) {
		SET_BIT(d->character->player_specials->saved.GodsLike, EGf::kDemigod);
	}
	// Насильственно забираем этот флаг у иммов (если он, конечно же, есть
	if ((GET_GOD_FLAG(d->character, EGf::kDemigod) && GetRealLevel(d->character) >= kLvlGod)) {
		REMOVE_BIT(d->character->player_specials->saved.GodsLike, EGf::kDemigod);
	}

	switch (d->character->get_sex()) {
		case EGender::kLast: [[fallthrough]];
		case EGender::kNeutral: sprintf(buf, "%s вошло в игру.", GET_NAME(d->character));
			break;
		case EGender::kMale: sprintf(buf, "%s вошел в игру.", GET_NAME(d->character));
			break;
		case EGender::kFemale: sprintf(buf, "%s вошла в игру.", GET_NAME(d->character));
			break;
		case EGender::kPoly: sprintf(buf, "%s вошли в игру.", GET_NAME(d->character));
			break;
	}
	mudlog(buf, NRM, std::max(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
	d->has_prompt = 0;
	login_change_invoice(d->character.get());
	log("Player %s enter at room %d", GET_NAME(d->character), GET_ROOM_VNUM(load_room));
	char_to_room(d->character, load_room);
	// Городской стартовый стаф выдаем уже после помещения в комнату -- по реальному городу
	// появления (cities.xml <start_item>).
	if (new_char) {
		Noob::give_city_start_outfit(d->character.get());
	}
	act("$n вступил$g в игру.", true, d->character.get(), nullptr, nullptr, kToRoom);
	affect_total(d->character.get());
	CheckLight(d->character.get(), kLightNo, kLightNo, kLightNo, kLightNo, 0);
	sight::look_at_room(d->character.get(), false);

	if (new_char) {
		SendMsgToChar("\r\nВоспользуйтесь командой НОВИЧОК для получения вводной информации игроку.\r\n",
					  d->character.get());
		SendMsgToChar(
			"Включен режим автоматического показа карты, наберите 'справка карта' для ознакомления.\r\n"
			"Если вы заблудились и не можете самостоятельно найти дорогу назад - прочтите 'справка возврат'.\r\n",
			d->character.get());
	}
	Noob::check_help_message(d->character.get());
}

//По кругу проверяем корректность параметров
//Все это засунуто в одну функцию для того
//Чтобы в случае некорректности сразу нескольких параметров
//Они все были корректно обработаны
bool ValidateStats(DescriptorData *d) {
	//Требуется рерол статов
	if (!GloryMisc::check_stats(d->character.get())) {
		return false;
	}

	//Некорректный номер рода
	if (!MUD::PcRaces().IsAvailable(GET_RACE(d->character))) {
		iosystem::write_to_output("\r\nКакого роду-племени вы будете?\r\n", d);
		iosystem::write_to_output(string(player_races::FormatRacesMenu()).c_str(), d);
		iosystem::write_to_output("\r\nИз чьих вы будете: ", d);
		d->state = EConState::kResetRace;
		return false;
	}

	// не корректный номер религии
	if (GET_RELIGION(d->character) > kReligionMono) {
		iosystem::write_to_output(religion_menu, d);
		iosystem::write_to_output("\n\rРелигия :", d);
		d->state = EConState::kResetReligion;
		return false;
	}

	return true;
}

void DoAfterPassword(DescriptorData *d) {
	int load_result;

	// Password was correct.
	load_result = GET_BAD_PWS(d->character);
	GET_BAD_PWS(d->character) = 0;
	d->bad_pws = 0;

	if (ban->IsBanned(d->host) == BanList::BAN_SELECT && !d->character->IsFlagged(EPlrFlag::kSiteOk)) {
		iosystem::write_to_output("Извините, вы не можете выбрать этого игрока с данного IP!\r\n", d);
		d->state = EConState::kClose;
		sprintf(buf, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, kLvlGod, SYSLOG, true);
		return;
	}
	if (GetRealLevel(d->character) < circle_restrict) {
		iosystem::write_to_output("Игра временно приостановлена.. Ждем вас немного позже.\r\n", d);
		d->state = EConState::kClose;
		sprintf(buf, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, kLvlGod, SYSLOG, true);
		return;
	}
	if (new_loc_codes.count(GET_EMAIL(d->character)) != 0) {
		iosystem::write_to_output("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
		d->state = EConState::kRandomNumber;
		return;
	}
	// нам нужен массив сетей с маской /24
	std::set<uint32_t> subnets;

	const uint32_t MASK = 16777215;
	for (const auto &logon : LOGON_LIST(d->character)) {
		uint32_t current_subnet = inet_addr(logon.ip.c_str()) & MASK;
		subnets.insert(current_subnet);
	}

	if (!subnets.empty()) {
		if (subnets.count(inet_addr(d->host) & MASK) == 0) {
			sprintf(buf, "Персонаж %s вошел с необычного места!", GET_NAME(d->character));
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
			if (d->character->IsFlagged(EPrf::kIpControl)) {
				int random_number = number(1000000, 9999999);
				new_loc_codes[GET_EMAIL(d->character)] = random_number;
				std::string cmd_line =
					fmt::format("python3 send_code.py {} {} &", GET_EMAIL(d->character), random_number);
				auto result = system(cmd_line.c_str());
				UNUSED_ARG(result);
				iosystem::write_to_output("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
				d->state = EConState::kRandomNumber;
				return;
			}
		}
	}
	// check and make sure no other copies of this player are logged in
	if (perform_dupe_check(d)) {
		Clan::SetClanData(d->character.get());
		return;
	}

	// тут несколько вариантов как это проставить и все одинаково корявые с учетом релоада, без уверенности не трогать
	Clan::SetClanData(d->character.get());

	log("%s [%s] has connected.", GET_NAME(d->character), d->host);

	if (load_result) {
		sprintf(buf, "\r\n\r\n\007\007\007"
					 "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
				kColorRed, load_result, (load_result > 1) ? "S" : "", kColorNrm);
		iosystem::write_to_output(buf, d);
		GET_BAD_PWS(d->character) = 0;
	}
	time_t tmp_time = d->character->get_last_logon();
	sprintf(buf, "\r\nПоследний раз вы заходили к нам в %s с адреса (%s).\r\n",
			rustime(localtime(&tmp_time)), d->character->player_specials->saved.LastIP);
	iosystem::write_to_output(buf, d);

	//if (!GloryMisc::check_stats(d->character))
	if (!ValidateStats(d)) {
		return;
	}

	iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	d->state = EConState::kRmotd;
}

void CreateChar(DescriptorData *d) {
	if (d->character) {
		return;
	}

	d->character = std::make_shared<Player>();
	d->character->player_specials = std::make_shared<player_special_data>();
	d->character->desc = d;
}

// initialize a new character only if class is set
void init_char(CharData *ch, PlayerIndexElement &element) {
	int i;

#ifdef TEST_BUILD
	if (0 == player_table.size())
  {
  // При собирании через make test первый чар в маде становится иммом 34
  ch->set_level(kLvlImplementator);
  }
#endif

	CREATE(GET_LOGS(ch), 1 + LAST_LOG);
	ch->set_npc_name(nullptr);
	ch->player_data.long_descr = "";
	ch->player_data.description = "";
	ch->player_data.time.birth = time(nullptr);
	ch->player_data.time.played = 0;
	ch->player_data.time.logon = time(nullptr);

	// make favors for sex
	if (ch->get_sex() == EGender::kMale) {
		ch->player_data.weight = number(120, 180);
		ch->player_data.height = number(160, 200);
	} else {
		ch->player_data.weight = number(100, 160);
		ch->player_data.height = number(150, 180);
	}

	ch->set_hit(ch->get_max_hit());
	ch->set_max_move(82);
	ch->set_move(ch->get_max_move());
	ch->real_abils.armor = 100;

	ch->set_uid(++top_idnum);
	element.level = 0;
	element.remorts = 0;
	element.last_logon = -1;
	element.mail.clear();
	element.last_ip.clear();

	if (GetRealLevel(ch) > kLvlGod) {
		SetGodSkills(ch);
	}

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		if (GetRealLevel(ch) < kLvlGreatGod) {
			GET_SPELL_TYPE(ch, spell_id) = ESpellType::kUnknowm;
		} else {
			GET_SPELL_TYPE(ch, spell_id) = ESpellType::kKnow;
		}
	}

	ch->char_specials.saved.affected_by.clear();
	for (auto save = ESaving::kFirst; save <= ESaving::kLast; ++save) {
		SetSave(ch, save, 0);
	}
	for (i = EResist::kFirstResist; i <= EResist::kLastResist; ++i) {
		GET_RESIST(ch, i) = 0;
	}

	if (GetRealLevel(ch) == kLvlImplementator) {
		ch->set_str(25);
		ch->set_int(25);
		ch->set_wis(25);
		ch->set_dex(25);
		ch->set_con(25);
		ch->set_cha(25);
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++) {
		GET_COND(ch, i) = (GetRealLevel(ch) == kLvlImplementator ? -1 : i == condition::kDrunk ? 0 : 24);
	}
	ch->player_specials->saved.LastIP[0] = 0;
	//	GET_LOADROOM(ch) = start_room;
	ch->SetFlag(EPrf::kDispHp);
	ch->SetFlag(EPrf::kDispMana);
	ch->SetFlag(EPrf::kDispExits);
	ch->SetFlag(EPrf::kDispMove);
	ch->SetFlag(EPrf::kDispExp);
	ch->SetFlag(EPrf::kDispFight);
	ch->UnsetFlag(EPrf::KSummonable);
	ch->SetFlag(EPrf::kColor2);
	(ch)->player_specials->saved.stringLength = 80;
	(ch)->player_specials->saved.stringWidth = 30;
	(ch)->player_specials->saved.ntfyExchangePrice = 0;

	ch->save_char();
}

/*
* Create a new entry in the in-memory index table for the player file.
* If the name already exists, by overwriting a deleted character, then
* we re-use the old position.
*/
int create_entry(PlayerIndexElement &element) {
	// create new save activity
	element.activity = number(0, kObjectSaveActivity - 1);
	element.timer = nullptr;

	return static_cast<int>(player_table.Append(element));
}

void DoAfterEmailConfirm(DescriptorData *d) {
	PlayerIndexElement element(GET_PC_NAME(d->character));

	// Now GET_NAME() will work properly.
	init_char(d->character.get(), element);

	if (d->character->get_pfilepos() < 0) {
		d->character->set_pfilepos(create_entry(element));
	}
	d->character->save_char();
	d->character->get_account()->set_last_login();
	d->character->get_account()->add_player(d->character->get_uid());

	// добавляем в список ждущих одобрения
	if (!(int) NAME_FINE(d->character)) {
		sprintf(buf, "%s - новый игрок. Падежи: %s/%s/%s/%s/%s/%s Email: %s Пол: %s. ]\r\n"
					 "[ %s ждет одобрения имени.",
				GET_NAME(d->character), GET_PAD(d->character, 0),
				GET_PAD(d->character, 1), GET_PAD(d->character, 2),
				GET_PAD(d->character, 3), GET_PAD(d->character, 4),
				GET_PAD(d->character, 5), GET_EMAIL(d->character),
				genders[(int) d->character->get_sex()], GET_NAME(d->character));
		NewNames::add(d->character.get());
	}

	// remove from free names
	player_table.GetNameAdviser().remove(GET_NAME(d->character));

	iosystem::write_to_output(system_messages::GetText(system_messages::ESystemMsg::kMotd).c_str(), d);
	iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	d->state = EConState::kRmotd;
	d->character->set_who_mana(0);
	d->character->set_who_last(time(nullptr));

}

static void ShowEncodingPrompt(DescriptorData *d, bool withHints = false) {
	if (withHints) {
		iosystem::write_to_output(
			"\r\n"
			"Using keytable           TECT. CTPOKA\r\n"
			"  0) Koi-8               " ENC_HINT_KOI8R "\r\n"
													   "  1) Alt                 " ENC_HINT_ALT "\r\n"
																								"  2) Windows(JMC,MMC)    " ENC_HINT_WIN "\r\n"
																																		 "  3) Windows(zMUD)       " ENC_HINT_WIN_ZMUD "\r\n"
																																													   "  4) Windows(zMUD 'z')   " ENC_HINT_WIN_ZMUD_z "\r\n"
																																																									   "  5) UTF-8               " ENC_HINT_UTF8 "\r\n"
																																																																				 "  6) Windows(zMUD <6.39) " ENC_HINT_WIN_ZMUD_old "\r\n"
																																																																																   //			"Select one : ", d);
																																																																																   "\r\n"
																																																																																   "KAKOE HAnuCAHuE ECTb BEPHOE, PA3yMEEMOE HA PyCCKOM? BBEguTE HOMEP : ",
			d);
	} else {
		iosystem::write_to_output(
			"\r\n"
			"Using keytable\r\n"
			"  0) Koi-8\r\n"
			"  1) Alt\r\n"
			"  2) Windows(JMC,MMC)\r\n"
			"  3) Windows(zMUD)\r\n"
			"  4) Windows(zMUD 'z')\r\n"
			"  5) UTF-8\r\n"
			"  6) Windows(zMUD <6.39)\r\n"
			"  9) TECT...\r\n"
			"Select one : ", d);
	}
}

void DisplaySelectCharClassMenu(DescriptorData *d) {
	std::ostringstream out;
	out << "\r\n" << "Выберите профессию:" << "\r\n";
	std::vector<ECharClass> char_classes;
	char_classes.reserve(kNumPlayerClasses);
	for (const auto &it : MUD::Classes()) {
		if (it.IsAvailable()) {
			char_classes.push_back(it.GetId());
		}
	}
	std::sort(char_classes.begin(), char_classes.end());
	for (const auto &it : char_classes) {
		out << "  " << kColorCyn << std::right << std::setw(3) << to_underlying(it) + 1 << kColorNrm << ") "
			<< kColorGrn << std::left << MUD::Class(it).GetName() << "\r\n" << kColorNrm;
	}
	iosystem::write_to_output(out.str().c_str(), d);
}

// deal with newcomers and other non-playing sockets
static void HandleGetName(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	int player_i = 0;
	char tmp_name[kMaxInputLength], pwd_name[kMaxInputLength], pwd_pwd[kMaxInputLength];
	if (!d->character) {
		CreateChar(d);
	}

	if (!*argument) {
		d->state = EConState::kClose;
	} else if (!str_cmp("новый", argument)) {
		iosystem::write_to_output(system_messages::GetText(system_messages::ESystemMsg::kNameRules).c_str(), d);

		std::stringstream ss;
		ss << "Введите имя";
		const auto free_name_list = player_table.GetNameAdviser().get_random_name_list();
		if (!free_name_list.empty()) {
			ss << " (примеры доступных имен : ";
			ss << JoinRange(free_name_list);
			ss << ")";
		}

		ss << ": ";

		iosystem::write_to_output(ss.str().c_str(), d);
		d->state = EConState::kNewChar;
		return;
	} else {
		if (sscanf(argument, "%s %s", pwd_name, pwd_pwd) == 2) {
			if (parse_exist_name(pwd_name, tmp_name)
				|| (player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId))
					< 0) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			if (d->character->IsFlagged(EPlrFlag::kDeleted)
				|| !Password::compare_password(d->character.get(), pwd_pwd)) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				if (!d->character->IsFlagged(EPlrFlag::kDeleted)) {
					sprintf(buffer, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
					mudlog(buffer, BRF, kLvlImmortal, SYSLOG, true);
				}

				d->character.reset();
				return;
			}

			d->character->UnsetFlag(EPlrFlag::kMailing);
			d->character->UnsetFlag(EPlrFlag::kWriting);
			d->character->UnsetFlag(EPlrFlag::kCryo);
			d->character->set_pfilepos(player_i);
			DoAfterPassword(d);

			return;
		} else {
			if (parse_exist_name(argument, tmp_name) ||
				strlen(tmp_name) < (kMinNameLength - 1) || // дабы можно было войти чарам с 4 буквами
				strlen(tmp_name) > kMaxNameLength ||
				!IsValidName(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name)) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			} else if (!IsNameOffline(tmp_name)) {
				player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId);
				d->character->set_pfilepos(player_i);
				if (privilege::IsImmortal(d->character.get()) || d->character->IsFlagged(EPrf::kCoderinfo)) {
					iosystem::write_to_output("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
				} else {
					iosystem::write_to_output("Игрок с подобным именем находится в игре.\r\n", d);
				}
				iosystem::write_to_output("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
				iosystem::write_to_output("Имя и пароль через пробел : ", d);

				d->character.reset();
				return;
			}
		}

		player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId);
		if (player_i > -1) {
			d->character->set_pfilepos(player_i);
			if (d->character->IsFlagged(EPlrFlag::kDeleted)) {
				d->character.reset();

				if (!IsNameAvailable(tmp_name) || _parse_name(tmp_name, tmp_name)) {
					iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					return;
				}

				if (strlen(tmp_name) < (kMinNameLength)) {
					iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					return;
				}

				CreateChar(d);
				d->character->SetCharAliases(utils::CAP(tmp_name));
				d->character->player_data.PNames[grammar::ECase::kNom] = std::string(utils::CAP(tmp_name));
				d->character->set_pfilepos(player_i);
				sprintf(buffer, "Вы действительно выбрали имя %s [ Y(Д) / N(Н) ]? ", tmp_name);
				log("New player %s ip %s", d->character->player_data.PNames[grammar::ECase::kNom].c_str(), d->host);
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kNameConfirm;
			} else    // undo it just in case they are set
			{
				if (privilege::IsImmortal(d->character.get()) || d->character->IsFlagged(EPrf::kCoderinfo)) {
					iosystem::write_to_output("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
					iosystem::write_to_output("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
					iosystem::write_to_output("Имя и пароль через пробел : ", d);
					d->character.reset();

					return;
				}

				d->character->UnsetFlag(EPlrFlag::kMailing);
				d->character->UnsetFlag(EPlrFlag::kWriting);
				d->character->UnsetFlag(EPlrFlag::kCryo);
				iosystem::write_to_output("Персонаж с таким именем уже существует. Введите пароль : ", d);
				d->idle_tics = 0;
				d->state = EConState::kPassword;
			}
		} else    // player unknown -- make new character
		{
			// еще одна проверка
			if (strlen(tmp_name) < (kMinNameLength)) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			// Check for multiple creations of a character.
			if (!IsNameAvailable(tmp_name) || _parse_name(tmp_name, tmp_name)) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			if (CmpPtableByName(tmp_name, kMinNameLength) >= 0) {
				iosystem::write_to_output("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
				 "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
				 "Имя  : ", d);
				return;
			}

			d->character->SetCharAliases(utils::CAP(tmp_name));
			d->character->player_data.PNames[grammar::ECase::kNom] = std::string(utils::CAP(tmp_name));
			iosystem::write_to_output(system_messages::GetText(system_messages::ESystemMsg::kNameRules).c_str(), d);
			sprintf(buffer, "Вы действительно выбрали имя  %s [ Y(Д) / N(Н) ]? ", tmp_name);
			log("New player %s ip %s", d->character->player_data.PNames[grammar::ECase::kNom].c_str(), d->host);
			iosystem::write_to_output(buffer, d);
			d->state = EConState::kNameConfirm;
		}
	}

}

static void HandleNewChar(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	int player_i = 0;
	char tmp_name[kMaxInputLength];
	bool is_player_deleted;
	if (!*argument) {
		d->state = EConState::kClose;
		return;
	}

	if (!d->character) {
		CreateChar(d);
	}

	if (_parse_name(argument, tmp_name) ||
		strlen(tmp_name) < kMinNameLength ||
		strlen(tmp_name) > kMaxNameLength ||
		!IsValidName(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name)) {
		iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
		return;
	}

	player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId);
	is_player_deleted = false;
	if (player_i > -1) {
		is_player_deleted = d->character->IsFlagged(EPlrFlag::kDeleted);
		if (is_player_deleted) {
			d->character.reset();
			CreateChar(d);
		} else {
			iosystem::write_to_output("Такой персонаж уже существует. Выберите другое имя : ", d);
			d->character.reset();

			return;
		}
	}

	if (!IsNameAvailable(tmp_name)) {
		iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
		return;
	}

	// skip name check for deleted players
	if (!is_player_deleted && CmpPtableByName(tmp_name, kMinNameLength) >= 0) {
		iosystem::write_to_output("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
				  "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
				  "Имя  : ", d);
		return;
	}

	d->character->SetCharAliases(utils::CAP(tmp_name));
	d->character->player_data.PNames[grammar::ECase::kNom] = std::string(utils::CAP(tmp_name));
	if (is_player_deleted) {
		d->character->set_pfilepos(player_i);
	}
	if (ban->IsBanned(d->host) >= BanList::BAN_NEW) {
		sprintf(buffer, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
				GET_PC_NAME(d->character), d->host);
		mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
		iosystem::write_to_output("Извините, создание нового персонажа для вашего IP !!!ЗАПРЕЩЕНО!!!\r\n", d);
		d->state = EConState::kClose;
		return;
	}

	if (circle_restrict) {
		iosystem::write_to_output("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
		sprintf(buffer,
				"Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
				GET_PC_NAME(d->character), d->host);
		mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
		d->state = EConState::kClose;
		return;
	}

	switch (NewNames::auto_authorize(d)) {
		case NewNames::AUTO_ALLOW:
			sprintf(buffer,
					"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
					GET_PAD(d->character, 1));
			iosystem::write_to_output(buffer, d);
			d->state = EConState::kNewpasswd;
			return;

		case NewNames::AUTO_BAN: d->character.reset();
			iosystem::write_to_output("Выберите другое имя : ", d);
			return;

		default: break;
	}

	iosystem::write_to_output("Ваш пол [ М(M)/Ж(F) ]? ", d);
	d->state = EConState::kQsex;
	return;

}

// --- Main connection menu (issue.exit-menu) ---------------------------------
// One handler per entry; kMainMenu drives BOTH the auto-numbered display
// (ShowMainMenu) and the dispatch (HandleMainMenu), so the number a player sees
// and the action it triggers can never drift apart. Line labels live in common_msg.

static void HandleMenuExit(DescriptorData *d) {
	char buffer[kMaxStringLength];
	iosystem::write_to_output("\r\nДо встречи на земле Киевской.\r\n", d);
	if (remort::GetRealRemort(d->character) == 0
		&& GetRealLevel(d->character) <= 25
		&& !d->character->IsFlagged(EPlrFlag::kNoDelete)) {
		int timeout = -1;
		for (int ci = 0; GetRealLevel(d->character) > pclean_criteria[ci].level; ci++) {
			timeout = pclean_criteria[ci + 1].days;
		}
		if (timeout > 0) {
			time_t deltime = time(nullptr) + timeout * 60 * rent_file_timeout * 24;
			sprintf(buffer,
					"В случае вашего отсутствия персонаж будет храниться до %s нашей эры :).\r\n",
					rustime(localtime(&deltime)));
			iosystem::write_to_output(buffer, d);
		}
	}
	d->state = EConState::kClose;
}

static void HandleMenuEnterGame(DescriptorData *d) {
	if (!check_dupes_email(d)) {
		d->state = EConState::kClose;
		return;
	}
	do_entergame(d);
}

static void HandleMenuDescription(DescriptorData *d) {
	if (!d->character->player_data.description.empty()) {
		iosystem::write_to_output("Ваше ТЕКУЩЕЕ описание:\r\n", d);
		iosystem::write_to_output(d->character->player_data.description.c_str(), d);
		// keep the old description as the editor's ABORT buffer
		d->backstr = str_dup(d->character->player_data.description.c_str());
	}
	iosystem::write_to_output("Введите описание вашего героя, которое будет выводиться по команде <осмотреть>.\r\n", d);
	iosystem::write_to_output("(/s сохранить /h помощь)\r\n", d);
	d->writer =
		std::make_shared<utils::DelegatedStdStringWriter>(d->character->player_data.description);
	d->max_str = kExdscrLength;
	d->state = EConState::kExdesc;
}

static void HandleMenuHistory(DescriptorData *d) {
	page_string(d, system_messages::GetText(system_messages::ESystemMsg::kBackground));
	d->state = EConState::kRmotd;
}

static void HandleMenuChangePassword(DescriptorData *d) {
	iosystem::write_to_output("\r\nВведите СТАРЫЙ пароль : ", d);
	d->state = EConState::kChpwdGetOld;
}

static void HandleMenuDelete(DescriptorData *d) {
	if (privilege::IsImmortal(d->character.get())) {
		iosystem::write_to_output("\r\nБоги бессмертны (с) Стрибог, просите чтоб пофризили :)))\r\n", d);
		ShowMainMenu(d);
		return;
	}
	if (d->character->IsFlagged(EPlrFlag::kHelled)
		|| d->character->IsFlagged(EPlrFlag::kFrozen)) {
		iosystem::write_to_output("\r\nВы находитесь в АДУ!!! Амнистии подобным образом не будет.\r\n", d);
		ShowMainMenu(d);
		return;
	}
	if (remort::GetRealRemort(d->character) > 5) {
		iosystem::write_to_output("\r\nНельзя удалить себя достигнув шестого перевоплощения.\r\n", d);
		ShowMainMenu(d);
		return;
	}
	iosystem::write_to_output("\r\nДля подтверждения введите свой пароль : ", d);
	d->state = EConState::kDelcnf1;
}

static void HandleMenuResetStats(DescriptorData *d) {
	if (privilege::IsImmortal(d->character.get())) {
		iosystem::write_to_output("\r\nВам это ни к чему...\r\n", d);
		ShowMainMenu(d);
		d->state = EConState::kMenu;
	} else {
		stats_reset::print_menu(d);
		d->state = EConState::kMenuStats;
	}
}

static void HandleMenuBlind(DescriptorData *d) {
	if (!d->character->IsFlagged(EPrf::kBlindMode)) {
		d->character->SetFlag(EPrf::kBlindMode);
		iosystem::write_to_output("\r\nСпециальный режим слепого игрока ВКЛЮЧЕН.\r\n", d);
	} else {
		d->character->UnsetFlag(EPrf::kBlindMode);
		iosystem::write_to_output("\r\nСпециальный режим слепого игрока ВЫКЛЮЧЕН.\r\n", d);
	}
	ShowMainMenu(d);
	d->state = EConState::kMenu;
}

static void HandleMenuEmailList(DescriptorData *d) {
	d->character->get_account()->list_players(d);
}

static const struct {
	ECommonMsg label;                   // menu-line text (common_msg)
	void (*handler)(DescriptorData *);  // action when this entry is chosen
} kMainMenu[] = {
	{ECommonMsg::kMenuExit,           HandleMenuExit},
	{ECommonMsg::kMenuEnterGame,      HandleMenuEnterGame},
	{ECommonMsg::kMenuDescription,    HandleMenuDescription},
	{ECommonMsg::kMenuHistory,        HandleMenuHistory},
	{ECommonMsg::kMenuChangePassword, HandleMenuChangePassword},
	{ECommonMsg::kMenuDelete,         HandleMenuDelete},
	{ECommonMsg::kMenuResetStats,     HandleMenuResetStats},
	{ECommonMsg::kMenuBlind,          HandleMenuBlind},
	{ECommonMsg::kMenuEmailList,      HandleMenuEmailList},
};
static constexpr size_t kMainMenuSize = sizeof(kMainMenu) / sizeof(kMainMenu[0]);

// Render the menu with sequential 0-based numbers (line index == the digit typed).
void ShowMainMenu(DescriptorData *d) {
	std::string out = "\r\n";
	char line[kMaxInputLength];
	for (size_t i = 0; i < kMainMenuSize; ++i) {
		snprintf(line, sizeof(line), "%zu) %s\r\n", i, CommonMsg(kMainMenu[i].label).c_str());
		out += line;
	}
	out += "\r\n   " + CommonMsg(ECommonMsg::kMenuPrompt) + " ";
	iosystem::write_to_output(out.c_str(), d);
}

static void HandleMainMenu(DescriptorData *d, char *argument) {
	const unsigned char c = static_cast<unsigned char>(*argument);
	if (c >= '0' && c < '0' + static_cast<int>(kMainMenuSize)) {
		kMainMenu[c - '0'].handler(d);
		return;
	}
	iosystem::write_to_output("\r\nЭто не есть правильный ответ!\r\n", d);
	ShowMainMenu(d);
}

static void HandleNameCase(DescriptorData *d, char *argument, int step) {
	// Russian name-declension entry: genitive -> dative -> accusative -> instrumental ->
	// prepositional. One table-driven handler in place of the former HandleName2..HandleName6.
	static const struct {
		grammar::ECase ecase;    // PNames slot stored on success
		int idx;                 // GetCase declension index for this case
		const char *prompt;      // prompt body: "<case> падеже (...)"
		EConState state;         // the EConState routed to this step (used as the next-step target)
	} kSteps[] = {
		{grammar::ECase::kGen, 1, "родительном падеже (меч КОГО?)",         EConState::kName2},
		{grammar::ECase::kDat, 2, "дательном падеже (отправить КОМУ?)",     EConState::kName3},
		{grammar::ECase::kAcc, 3, "винительном падеже (ударить КОГО?)",     EConState::kName4},
		{grammar::ECase::kIns, 4, "творительном падеже (сражаться с КЕМ?)", EConState::kName5},
		{grammar::ECase::kPre, 5, "предложном падеже (говорить о КОМ?)",    EConState::kName6},
	};
	constexpr int kLast = 4;
	char buffer[kMaxStringLength];
	char tmp_name[kMaxInputLength];
	const auto &cur = kSteps[step];

	skip_spaces(&argument);
	if (strlen(argument) == 0) {
		GetCase(GET_PC_NAME(d->character), d->character->get_sex(), cur.idx, argument);
	}
	if (!_parse_name(argument, tmp_name)
		&& strlen(tmp_name) >= kMinNameLength
		&& strlen(tmp_name) <= kMaxNameLength
		&& !strn_cmp(tmp_name,
					 GET_PC_NAME(d->character),
					 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
		d->character->player_data.PNames[cur.ecase] = std::string(utils::CAP(tmp_name));
		if (step < kLast) {
			const auto &next = kSteps[step + 1];
			GetCase(GET_PC_NAME(d->character), d->character->get_sex(), next.idx, tmp_name);
			sprintf(buffer, "Имя в %s [%s]: ", next.prompt, tmp_name);
			iosystem::write_to_output(buffer, d);
			d->state = next.state;
		} else {
			sprintf(buffer,
					"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
					GET_PAD(d->character, 1));
			iosystem::write_to_output(buffer, d);
			d->state = EConState::kNewpasswd;
		}
	} else {
		iosystem::write_to_output("Некорректно.\r\n", d);
		GetCase(GET_PC_NAME(d->character), d->character->get_sex(), cur.idx, tmp_name);
		sprintf(buffer, "Имя в %s [%s]: ", cur.prompt, tmp_name);
		iosystem::write_to_output(buffer, d);
	}
}

static void HandleInit(DescriptorData *d, char * /*argument*/) {
	char buffer[kMaxStringLength];
	// just connected
{
	int online_players = 0;
	for (auto i = descriptor_list; i; i = i->next) {
		online_players++;
	}
	sprintf(buffer, "Online: %d\r\n", online_players);
}

	iosystem::write_to_output(buffer, d);
	ShowEncodingPrompt(d, false);
	d->state = EConState::kGetKeytable;
	return;
}

static void HandleGetKeytable(DescriptorData *d, char *argument) {
	if (strlen(argument) > 0)
		argument[0] = argument[strlen(argument) - 1];
	if (*argument == '9') {
		ShowEncodingPrompt(d, true);
		return;
	}
	if (!*argument || *argument < '0' || *argument >= '0' + kCodePageLast) {
		iosystem::write_to_output("\r\nUnknown key table. Retry, please : ", d);
		return;
	}
	d->keytable = (ubyte) *argument - (ubyte) '0';
	ip_log(d->host);
	iosystem::write_to_output(system_messages::GetText(system_messages::ESystemMsg::kGreetings).c_str(), d);
	d->state = EConState::kGetName;
	return;
}

static void HandleNameConfirm(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	if (UPPER(*argument) == 'Y' || UPPER(*argument) == 'Д') {
		if (ban->IsBanned(d->host) >= BanList::BAN_NEW) {
			sprintf(buffer, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
					GET_PC_NAME(d->character), d->host);
			mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
			iosystem::write_to_output("Извините, создание нового персонажа для вашего IP !!! ЗАПРЕЩЕНО !!!\r\n", d);
			d->state = EConState::kClose;
			return;
		}

		if (circle_restrict) {
			iosystem::write_to_output("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
			sprintf(buffer, "Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
					GET_PC_NAME(d->character), d->host);
			mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
			d->state = EConState::kClose;
			return;
		}

		switch (NewNames::auto_authorize(d)) {
			case NewNames::AUTO_ALLOW:
				sprintf(buffer,
						"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
						GET_PAD(d->character, 1));
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kNewpasswd;
				return;

			case NewNames::AUTO_BAN: d->state = EConState::kClose;
				return;

			default: break;
		}

		iosystem::write_to_output("Ваш пол [ М(M)/Ж(F) ]? ", d);
		d->state = EConState::kQsex;
		return;

	} else if (UPPER(*argument) == 'N' || UPPER(*argument) == 'Н') {
		iosystem::write_to_output("Итак, чего изволите? Учтите, бананов нет :)\r\n" "Имя : ", d);
		d->character->SetCharAliases(nullptr);
		d->state = EConState::kGetName;
	} else {
		iosystem::write_to_output("Ответьте Yes(Да) or No(Нет) : ", d);
	}
	return;
}

static void HandlePassword(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	/*
		   * To really prevent duping correctly, the player's record should
		   * be reloaded from disk at this point (after the password has been
		   * typed). However, I'm afraid that trying to load a character over
		   * an already loaded character is going to cause some problem down the
		   * road that I can't see at the moment.  So to compensate, I'm going to
		   * (1) add a 15 or 20-second time limit for entering a password, and (2)
		   * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
		   */

	iosystem::write_to_output("\r\n", d);

	if (!*argument) {
		d->state = EConState::kClose;
	} else {
		if (!Password::compare_password(d->character.get(), argument)) {
			sprintf(buffer, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
			mudlog(buffer, BRF, kLvlImmortal, SYSLOG, true);
			GET_BAD_PWS(d->character)++;
			d->character->save_char();
			if (++(d->bad_pws) >= max_bad_pws)    // 3 strikes and you're out.
			{
				iosystem::write_to_output("Неверный пароль... Отсоединяемся.\r\n", d);
				d->state = EConState::kClose;
			} else {
				iosystem::write_to_output("Неверный пароль.\r\nПароль : ", d);
			}
			return;
		}
		DoAfterPassword(d);
	}
	return;
}

static void HandleGetNewPassword(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	if (!Password::check_password(d->character.get(), argument)) {
		sprintf(buffer, "\r\n%s\r\n", Password::BAD_PASSWORD);
		iosystem::write_to_output(buffer, d);
		iosystem::write_to_output("Пароль : ", d);
		return;
	}

	Password::set_password(d->character.get(), argument);

	iosystem::write_to_output("\r\nПовторите пароль, пожалуйста : ", d);
	if (d->state == EConState::kNewpasswd) {
		d->state = EConState::kCnfpasswd;
	} else {
		d->state = EConState::kChpwdVrfy;
	}

	return;
}

static void HandleConfirmNewPassword(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	if (!Password::compare_password(d->character.get(), argument)) {
		iosystem::write_to_output("\r\nПароли не соответствуют... повторим.\r\n", d);
		iosystem::write_to_output("Пароль: ", d);
		if (d->state == EConState::kCnfpasswd) {
			d->state = EConState::kNewpasswd;
		} else {
			d->state = EConState::kChpwdGetNew;
		}
		return;
	}

	if (d->state == EConState::kCnfpasswd) {
		DisplaySelectCharClassMenu(d);
		iosystem::write_to_output(
			"\r\nВаша профессия? (Для более полной информации вы можете набрать 'справка <интересующая профессия>'): ",
			d);
		d->state = EConState::kQclass;
	} else {
		sprintf(buffer, "%s заменил себе пароль.", GET_NAME(d->character));
		AddKarma(d->character.get(), buffer, "");
		d->character->save_char();
		iosystem::write_to_output("\r\nГотово.\r\n", d);
		ShowMainMenu(d);
		d->state = EConState::kMenu;
	}

	return;
}

static void HandleQuerySex(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	char tmp_name[kMaxInputLength];
	if (pre_help(d->character.get(), argument)) {
		iosystem::write_to_output("\r\nВаш пол [ М(M)/Ж(F) ]? ", d);
		d->state = EConState::kQsex;
		return;
	}

	switch (UPPER(*argument)) {
		case 'М':
		case 'M': d->character->set_sex(EGender::kMale);
			break;

		case 'Ж':
		case 'F': d->character->set_sex(EGender::kFemale);
			break;

		default: iosystem::write_to_output("Это может быть и пол, но явно не ваш :)\r\n" "А какой у ВАС пол? ", d);
			return;
	}
	iosystem::write_to_output("Проверьте правильность склонения имени. В случае ошибки введите свой вариант.\r\n", d);
	GetCase(d->character->GetCharAliases(), d->character->get_sex(), 1, tmp_name);
	sprintf(buffer, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
	iosystem::write_to_output(buffer, d);
	d->state = EConState::kName2;
	return;
}

static void HandleQueryReligion(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	if (pre_help(d->character.get(), argument)) {
		iosystem::write_to_output(religion_menu, d);
		iosystem::write_to_output("\n\rРелигия :", d);
		d->state = EConState::kQreligion;
		return;
	}

	switch (UPPER(*argument)) {
		case 'Я':
		case 'З':
		case 'P':
			if (class_religion[to_underlying(d->character->GetClass())] == kReligionMono) {
				iosystem::write_to_output("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}
			GET_RELIGION(d->character) = kReligionPoly;
			break;

		case 'Х':
		case 'C':
			if (class_religion[to_underlying(d->character->GetClass())] == kReligionPoly) {
				iosystem::write_to_output("Персонажу выбранной вами профессии противно христианство!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}
			GET_RELIGION(d->character) = kReligionMono;
			break;

		default: iosystem::write_to_output("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
			return;
	}

	iosystem::write_to_output("\r\nКакой род вам ближе всего по духу:\r\n", d);
	iosystem::write_to_output(string(player_races::FormatRacesMenu()).c_str(), d);
	sprintf(buffer, "Для вашей профессией больше всего подходит %s",
			default_race[to_underlying(d->character->GetClass())]);
	iosystem::write_to_output(buffer, d);
	iosystem::write_to_output("\r\nИз чьих вы будете : ", d);
	d->state = EConState::kRace;

	return;
}

static void HandleQueryClass(DescriptorData *d, char *argument) {
	{
	if (pre_help(d->character.get(), argument)) {
		DisplaySelectCharClassMenu(d);
		iosystem::write_to_output("\r\nВаша профессия : ", d);
		d->state = EConState::kQclass;
		return;
	}

	int class_num{-1};
	ECharClass class_id{ECharClass::kUndefined};
	try {
		class_num = std::stoi(argument);
	} catch (std::exception &) {
		class_id = FindAvailableCharClassId(argument);
	}
	if (class_num != -1) {
		class_id = MUD::Classes().FindAvailableItem(class_num - 1).GetId();
	}

	if (class_id == ECharClass::kUndefined) {
		iosystem::write_to_output("\r\nЭто не профессия.\r\nПрофессия : ", d);
		return;
	} else {
		d->character->set_class(class_id);
	}

	iosystem::write_to_output(religion_menu, d);
	iosystem::write_to_output("\n\rРелигия :", d);
	d->state = EConState::kQreligion;
	return;
}
}

static void HandleQueryRace(DescriptorData *d, char *argument) {
	int load_result;
	if (pre_help(d->character.get(), argument)) {
		iosystem::write_to_output("Какой род вам ближе всего по духу:\r\n", d);
		iosystem::write_to_output(string(player_races::FormatRacesMenu()).c_str(), d);
		iosystem::write_to_output("\r\nРод: ", d);
		d->state = EConState::kRace;
		return;
	}

	load_result = player_races::RaceVnumByMenuChoice(argument);

	if (load_result == player_races::kRaceUndefined) {
		iosystem::write_to_output("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
		return;
	}

	GET_RACE(d->character) = load_result;
	iosystem::write_to_output(string(player_races::FormatStartRegionsMenu(GET_RACE(d->character))).c_str(), d);
	iosystem::write_to_output("\r\nГде вы хотите начать свои приключения: ", d);
	d->state = EConState::kBirthplace;

	return;
}

static void HandleBirthplace(DescriptorData *d, char *argument) {
	int load_result;
	if (pre_help(d->character.get(), argument)) {
		iosystem::write_to_output(string(player_races::FormatStartRegionsMenu(GET_RACE(d->character))).c_str(),
				  d);
		iosystem::write_to_output("\r\nГде вы хотите начать свои приключения: ", d);
		d->state = EConState::kBirthplace;
		return;
	}

	load_result = player_races::StartRegionByMenuChoice(GET_RACE(d->character), argument);

	{
		const int start_room = player_races::StartRoomForRaceRegion(GET_RACE(d->character), load_result);
		if (load_result == player_races::kRaceUndefined || start_room == kNowhere) {
			iosystem::write_to_output("Не уверены? Бывает.\r\n"
					  "Подумайте еще разок, и выберите:", d);
			return;
		}
		GET_LOADROOM(d->character) = start_room;
	}
	iosystem::write_to_output(genchar_help, d);
	iosystem::write_to_output("\r\n\r\nНажмите любую клавишу.\r\n", d);
	d->state = EConState::kRollStats;
	SetStartAbils(d->character.get());
	return;
}

static void HandleRollStats(DescriptorData *d, char *argument) {
	if (pre_help(d->character.get(), argument)) {
		genchar_disp_menu(d->character.get());
		d->state = EConState::kRollStats;
		return;
	}

	switch (genchar_parse(d->character.get(), argument)) {
		case kGencharContinue: genchar_disp_menu(d->character.get());
			break;
		default: iosystem::write_to_output("\r\nВведите ваш E-mail"
						   "\r\n(ВСЕ ВАШИ ПЕРСОНАЖИ ДОЛЖНЫ ИМЕТЬ ОДИНАКОВЫЙ E-mail)."
						   "\r\nНа этот адрес вам будет отправлен код для подтверждения: ", d);
			d->state = EConState::kGetEmail;
			break;
	}
	return;
}

static void HandleGetEmail(DescriptorData *d, char *argument) {
	if (!*argument) {
		iosystem::write_to_output("\r\nВаш E-mail : ", d);
		return;
	} else if (!IsValidEmail(argument)) {
		iosystem::write_to_output("\r\nНекорректный E-mail!" "\r\nВаш E-mail :  ", d);
		return;
	}
#ifdef TEST_BUILD
	strncpy(GET_EMAIL(d->character), argument, 127);
	  *(GET_EMAIL(d->character) + 127) = '\0';
	  utils::ConvertToLow(GET_EMAIL(d->character));
	  DoAfterEmailConfirm(d);
	  return;
#endif
	{
		int random_number = number(1000000, 9999999);
		new_char_codes[d->character->GetCharAliases()] = random_number;
		strncpy(GET_EMAIL(d->character), argument, 127);
		*(GET_EMAIL(d->character) + 127) = '\0';
		utils::ConvertToLow(GET_EMAIL(d->character));
		std::string cmd_line =
			fmt::format("python3 send_code.py {} {} &", GET_EMAIL(d->character), random_number);
		auto result = system(cmd_line.c_str());
		UNUSED_ARG(result);
		iosystem::write_to_output("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
		d->state = EConState::kRandomNumber;
	}
	return;
}

static void HandleReadMotd(DescriptorData *d, char * /*argument*/) {
	if (!check_dupes_email(d)) {
		d->state = EConState::kClose;
		return;
	}

	do_entergame(d);

	return;
}

static void HandleRandomNumber(DescriptorData *d, char *argument) {
	{
	int code_rand = atoi(argument);

	if (new_char_codes.count(d->character->GetCharAliases()) != 0) {
		if (new_char_codes[d->character->GetCharAliases()] != code_rand) {
			iosystem::write_to_output("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
			return;
		}
		new_char_codes.erase(d->character->GetCharAliases());
		DoAfterEmailConfirm(d);
		return;
	}

	if (new_loc_codes.count(GET_EMAIL(d->character)) == 0) {
		return;
	}

	if (new_loc_codes[GET_EMAIL(d->character)] != code_rand) {
		iosystem::write_to_output("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
		d->state = EConState::kClose;
		return;
	}

	new_loc_codes.erase(GET_EMAIL(d->character));
	network::add_logon_record(d);
	DoAfterPassword(d);

	return;
}
}

static void HandleGetOldPassword(DescriptorData *d, char *argument) {
	if (!Password::compare_password(d->character.get(), argument)) {
		iosystem::write_to_output("\r\nНеверный пароль.\r\n", d);
		ShowMainMenu(d);
		d->state = EConState::kMenu;
	} else {
		iosystem::write_to_output("\r\nВведите НОВЫЙ пароль : ", d);
		d->state = EConState::kChpwdGetNew;
	}

	return;
}

static void HandleDeleteConfirm1(DescriptorData *d, char *argument) {
	if (!Password::compare_password(d->character.get(), argument)) {
		iosystem::write_to_output("\r\nНеверный пароль.\r\n", d);
		ShowMainMenu(d);
		d->state = EConState::kMenu;
	} else {
		iosystem::write_to_output("\r\n!!! ВАШ ПЕРСОНАЖ БУДЕТ УДАЛЕН !!!\r\n"
				  "Вы АБСОЛЮТНО В ЭТОМ УВЕРЕНЫ?\r\n\r\n"
				  "Наберите \"YES / ДА\" для подтверждения: ", d);
		d->state = EConState::kDelcnf2;
	}

	return;
}

static void HandleDeleteConfirm2(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	if (!strcmp(argument, "yes")
		|| !strcmp(argument, "YES")
		|| !strcmp(argument, "да")
		|| !strcmp(argument, "ДА")) {
		if (d->character->IsFlagged(EPlrFlag::kFrozen)) {
			iosystem::write_to_output("Вы решились на суицид, но Боги остановили вас.\r\n", d);
			iosystem::write_to_output("Персонаж не удален.\r\n", d);
			d->state = EConState::kClose;
			return;
		}
		if (GetRealLevel(d->character) >= kLvlGreatGod) {
			return;
		}
		DeletePcByHimself(GET_NAME(d->character));
		sprintf(buffer, "Персонаж '%s' удален!\r\n" "До свидания.\r\n", GET_NAME(d->character));
		iosystem::write_to_output(buffer, d);
		sprintf(buffer, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GetRealLevel(d->character));
		mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
		d->character->get_account()->remove_player(d->character->get_uid());
		d->state = EConState::kClose;
		return;
	} else {
		iosystem::write_to_output("\r\nПерсонаж не удален.\r\n", d);
		ShowMainMenu(d);
		d->state = EConState::kMenu;
	}
	return;
}

static void HandleResetStats(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	if (pre_help(d->character.get(), argument)) {
		return;
	}

	switch (genchar_parse(d->character.get(), argument)) {
		case kGencharContinue: genchar_disp_menu(d->character.get());
			break;

		default:
			// после перераспределения и сейва в genchar_parse стартовых статов надо учесть морты и славу
			GloryMisc::recalculate_stats(d->character.get());
			// статы срезетили и новые выбрали
			sprintf(buffer, "\r\n%sБлагодарим за сотрудничество. Ж)%s\r\n", kColorBoldGrn, kColorNrm);
			iosystem::write_to_output(buffer, d);

			// Проверяем корректность статов
			// Если что-то некорректно, функция проверки сама вернет чара на доработку.
			if (!ValidateStats(d)) {
				return;
			}

			iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			d->state = EConState::kRmotd;
	}

	return;
}

static void HandleResetRace(DescriptorData *d, char *argument) {
	int load_result;
	if (pre_help(d->character.get(), argument)) {
		iosystem::write_to_output("Какой род вам ближе всего по духу:\r\n", d);
		iosystem::write_to_output(string(player_races::FormatRacesMenu()).c_str(), d);
		iosystem::write_to_output("\r\nРод: ", d);
		d->state = EConState::kResetRace;
		return;
	}

	load_result = player_races::RaceVnumByMenuChoice(argument);

	if (load_result == player_races::kRaceUndefined) {
		iosystem::write_to_output("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
		return;
	}

	GET_RACE(d->character) = load_result;

	if (!ValidateStats(d)) {
		return;
	}

	// способности нового рода проставятся дальше в do_entergame
	iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	d->state = EConState::kRmotd;

	return;
}

static void HandleResetReligion(DescriptorData *d, char *argument) {
	if (pre_help(d->character.get(), argument)) {
		iosystem::write_to_output(religion_menu, d);
		iosystem::write_to_output("\n\rРелигия :", d);
		return;
	}

	switch (UPPER(*argument)) {
		case 'Я':
		case 'З':
		case 'P':
			if (class_religion[to_underlying(d->character->GetClass())] == kReligionMono) {
				iosystem::write_to_output("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}
			GET_RELIGION(d->character) = kReligionPoly;
			break;

		case 'Х':
		case 'C':
			if (class_religion[to_underlying(d->character->GetClass())] == kReligionPoly) {
				iosystem::write_to_output("Персонажу выбранной вами профессии противно христианство!\r\n"
						   "Так каким Богам вы хотите служить? ", d);
				return;
			}

			GET_RELIGION(d->character) = kReligionMono;

			break;

		default: iosystem::write_to_output("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
			return;
	}

	if (!ValidateStats(d)) {
		return;
	}

	iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	d->state = EConState::kRmotd;

	return;
}

void ProcessLoginInput(DescriptorData *d, char *argument) {
	if (d->state != EConState::kConsole)
		skip_spaces(&argument);

	switch (d->state) {
		case EConState::kInit:
			HandleInit(d, argument);
			break;

#ifdef ENABLE_ADMIN_API
		case EConState::kAdminAPI:
			admin_api_parse(d, argument);
			break;
#endif

			//. OLC states .
		case EConState::kOedit: oedit_parse(d, argument);
			break;

		case EConState::kRedit: redit_parse(d, argument);
			break;

		case EConState::kZedit: zedit_parse(d, argument);
			break;

		case EConState::kMedit: medit_parse(d, argument);
			break;

		case EConState::kTrigedit: trigedit_parse(d, argument);
			break;


		case EConState::kVedun: vedun::vedun_parse(d, argument);
			break;

		case EConState::kClanedit: d->clan_olc->clan->Manage(d, argument);
			break;

		case EConState::kSpendGlory:
			if (!Glory::parse_spend_glory_menu(d->character.get(), argument)) {
				Glory::spend_glory_menu(d->character.get());
			}
			break;

		case EConState::kGloryConst:
			if (!GloryConst::parse_spend_glory_menu(d->character.get(), argument)) {
				GloryConst::spend_glory_menu(d->character.get());
			}
			break;

		case EConState::kNamedStuff:
			if (!NamedStuff::parse_nedit_menu(d->character.get(), argument)) {
				NamedStuff::nedit_menu(d->character.get());
			}
			break;

		case EConState::kMapMenu: d->map_options->parse_menu(d->character.get(), argument);
			break;

		case EConState::kSedit: {
			try {
				obj_sets_olc::parse_input(d->character.get(), argument);
			}
			catch (const std::out_of_range &e) {
				SendMsgToChar(d->character.get(), "Редактирование прервано: %s", e.what());
				d->sedit.reset();
				d->state = EConState::kPlaying;
			}
			break;
		}
			//. End of OLC states .*/

		case EConState::kGetKeytable:
			HandleGetKeytable(d, argument);
			break;

		case EConState::kGetName:    // wait for input of name
			HandleGetName(d, argument);
			break;
		case EConState::kNameConfirm:    // wait for conf. of new name
			HandleNameConfirm(d, argument);
			break;

		case EConState::kNewChar:
			HandleNewChar(d, argument);
			break;
		case EConState::kPassword:    // get pwd for known player
			HandlePassword(d, argument);
			break;

		case EConState::kNewpasswd:
		case EConState::kChpwdGetNew:
			HandleGetNewPassword(d, argument);
			break;

		case EConState::kCnfpasswd:
		case EConState::kChpwdVrfy:
			HandleConfirmNewPassword(d, argument);
			break;

		case EConState::kQsex:    // query sex of new user
			HandleQuerySex(d, argument);
			break;

		case EConState::kQreligion:    // query religion of new user
			HandleQueryReligion(d, argument);
			break;

		case EConState::kQclass:
			HandleQueryClass(d, argument);
			break;

		case EConState::kRace:    // query race
			HandleQueryRace(d, argument);
			break;

		case EConState::kBirthplace:
			HandleBirthplace(d, argument);
			break;

		case EConState::kRollStats:
			HandleRollStats(d, argument);
			break;

		case EConState::kGetEmail:
			HandleGetEmail(d, argument);
			break;

		case EConState::kRmotd:    // read CR after printing motd
			HandleReadMotd(d, argument);
			break;

		case EConState::kRandomNumber:
			HandleRandomNumber(d, argument);
			break;

		case EConState::kMenu:    // get selection from main menu
			HandleMainMenu(d, argument);
			break;
		case EConState::kChpwdGetOld:
			HandleGetOldPassword(d, argument);
			break;

		case EConState::kDelcnf1:
			HandleDeleteConfirm1(d, argument);
			break;

		case EConState::kDelcnf2:
			HandleDeleteConfirm2(d, argument);
			break;

		case EConState::kName2:
			HandleNameCase(d, argument, 0);
			break;
		case EConState::kName3:
			HandleNameCase(d, argument, 1);
			break;
		case EConState::kName4:
			HandleNameCase(d, argument, 2);
			break;
		case EConState::kName5:
			HandleNameCase(d, argument, 3);
			break;
		case EConState::kName6:
			HandleNameCase(d, argument, 4);
			break;
		case EConState::kClose: break;

		case EConState::kResetStats:
			HandleResetStats(d, argument);
			break;

		case EConState::kResetRace:
			HandleResetRace(d, argument);
			break;

		case EConState::kMenuStats: stats_reset::parse_menu(d, argument);
			break;

		case EConState::kResetReligion:
			HandleResetReligion(d, argument);
			break;

		default:
			log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
				static_cast<int>(d->state), d->character ? GET_NAME(d->character) : "<unknown>");
			d->state = EConState::kDisconnect;    // Safest to do.

			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
