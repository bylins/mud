//
// Created by Sventovit on 07.09.2024.
//

#include "utils/native_text.h"
#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"

extern int nameserver_is_slow; //config.cpp
const char *BoolToOnOffStr(bool value);

void do_toggle(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;
	// Вёрстка таблицы считается printf'ом в байтах; перевод на fmt сдвинул бы колонки,
	// поэтому здесь меняются только буферы -- формат оставлен как есть (#3814).
	char out[kMaxStringLength];
	char wimpy[kMaxInputLength];
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(wimpy, "нет");
	else
		snprintf(wimpy, sizeof(wimpy), "%-3d", GET_WIMP_LEV(ch));

	if (GetRealLevel(ch) >= kLvlImmortal || ch->IsFlagged(EPrf::kCoderinfo)) {
		snprintf(out, sizeof(out),
				 " Нет агров     : %-3s     "
				 " Супервидение  : %-3s     "
				 " Флаги комнат  : %-3s \r\n"
				 " Частный режим : %-3s     "
				 " Замедление    : %-3s     "
				 " Кодер         : %-3s \r\n"
				 " Опечатки      : %-3s \r\n",
				 BoolToOnOffStr(ch->IsFlagged(EPrf::kNohassle)),
				 BoolToOnOffStr(ch->IsFlagged(EPrf::kHolylight)),
				 BoolToOnOffStr(ch->IsFlagged(EPrf::kRoomFlags)),
				 BoolToOnOffStr(ch->IsFlagged(EPrf::kNoWiz)),
				 BoolToOnOffStr(nameserver_is_slow),
				 BoolToOnOffStr(ch->IsFlagged(EPrf::kCoderinfo)),
				 BoolToOnOffStr(ch->IsFlagged(EPrf::kShowUnread)));
		SendMsgToChar(out, ch);
	}

	snprintf(out, sizeof(out),
			 " Автовыходы    : %-3s     "
			 " Краткий режим : %-3s     "
			 " Сжатый режим  : %-3s \r\n"
			 " Повтор команд : %-3s     "
			 " Обращения     : %-3s     "
			 " Кто-то        : %s \r\n"
			 " Болтать       : %-3s     "
			 " Орать         : %-3s \r\n"
			 " Аукцион       : %-3s     "
			 " Базар         : %-3s     "
			 " Автозаучивание: %-3s \r\n"
			 " Призыв        : %-3s     "
			 " Автозавершение: %-3s     "
			 " Группа (вид)  : %s \r\n"
			 " Без двойников : %-3s     "
			 " Автопомощь    : %-3s     "
			 " Автодележ     : %-3s \r\n"
			 " Автограбеж    : %-7s "
			 " Брать куны    : %-3s     "
			 " Арена         : %-3s \r\n"
			 " Трусость      : %-3s     "
			 " Ширина экрана : %-3d     "
			 " Высота экрана : %-3d \r\n"
			 " Сжатие        : %s  "
			 " Новости (вид) : %-5s   "
			 " Доски         : %-3s \r\n"
			 " Хранилище     : %-8s"
			 " Пклист        : %-3s     "
			 " Политика      : %-3s \r\n"
			 " Пкформат      : %s  "
			 " Соклановцы    : %-8s"
			 " Оффтоп        : %-3s \r\n"
			 " Потеря связи  : %-3s     "
			 " Ингредиенты   : %-3s     "
			 " Вспомнить     : %-3u \r\n",
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAutoexit)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kBrief)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kCompact)),
			 (ch->IsFlagged(EPrf::kNoRepeat) ? "NO" : "YES"),
			 BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoTell)),
			 native_text::pad_right(ch->IsFlagged(EPrf::kNoInvistell) ? "нельзя" : "можно", 6).c_str(),
			 BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoGossip)),
			 BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoHoller)),
			 BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoAuction)),
			 BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoExchange)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAutomem)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::KSummonable)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kGoAhead)),
			 native_text::pad_right(ch->IsFlagged(EPrf::kShowGroup) ? "полный" : "краткий", 7).c_str(),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kNoClones)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAutoassist)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAutosplit)),
			 ch->IsFlagged(EPrf::kAutoloot) ? ch->IsFlagged(EPrf::kNoIngrLoot) ? "NO-INGR" : "ALL    " : "OFF    ",
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAutomoney)),
			 BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoArena)),
			 wimpy,
			 (ch)->player_specials->saved.stringLength,
			 (ch)->player_specials->saved.stringWidth,
#if defined(HAVE_ZLIB)
			 native_text::pad_right(ch->desc->deflate == nullptr ? "нет" : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"), 6).c_str(),
#else
		"N/A",
#endif
			 ch->IsFlagged(EPrf::kNewsMode) ? "доска" : "лента",
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kBoardMode)),
			 GetChestMode(ch).c_str(),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kPklMode)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kPolitMode)),
			 native_text::pad_right(ch->IsFlagged(EPrf::kPkFormatMode) ? "краткий" : "полный", 6).c_str(),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kClanmembersMode)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kOfftopMode)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAntiDcMode)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kNoIngrMode)),
			 ch->remember_get_num());
	SendMsgToChar(out, ch);
	if ((ch)->player_specials->saved.ntfyExchangePrice > 0) {
		snprintf(out, sizeof(out), " Уведомления   : %-7ld ", (ch)->player_specials->saved.ntfyExchangePrice);
	} else {
		snprintf(out, sizeof(out), " Уведомления   : %s ", native_text::pad_right("Нет", 7).c_str());
	}
	SendMsgToChar(out, ch);
	snprintf(out, sizeof(out),
			 " Карта         : %-3s     "
			 " Вход в зону   : %-3s   \r\n"
			 " Магщиты (вид) : %s"
			 " Автопризыв    : %-5s   "
			 " Маппер        : %-3s   \r\n"
			 " Контроль IP   : %-6s  ",
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kDrawMap)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kShowZoneNameOnEnter)),
			 native_text::pad_right(ch->IsFlagged(EPrf::kBriefShields) ? "краткий" : "полный", 8).c_str(),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kAutonosummon)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kMapper)),
			 BoolToOnOffStr(ch->IsFlagged(EPrf::kIpControl)));
	SendMsgToChar(out, ch);
	if (GET_GOD_FLAG(ch, EGf::kAllowTesterMode))
		snprintf(out, sizeof(out), " Тестер        : %-3s\r\n", BoolToOnOffStr(ch->IsFlagged(EPrf::kTester)));
	else
		snprintf(out, sizeof(out), "\r\n");
	SendMsgToChar(out, ch);
}

const char *BoolToOnOffStr(bool value) {
	return (value ? "ON" : "OFF");
}