//
// Created by Sventovit on 07.09.2024.
//

#include "entities/char_data.h"
#include "color.h"
#include "house.h"

const char *ctypes[] = {"выключен", "простой", "обычный", "полный", "\n"};

extern int nameserver_is_slow; //config.cpp

void do_toggle(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "нет");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

	if (GetRealLevel(ch) >= kLvlImmortal || ch->IsFlagged(EPrf::kCoderinfo)) {
		snprintf(buf, kMaxStringLength,
				 " Нет агров     : %-3s     "
				 " Супервидение  : %-3s     "
				 " Флаги комнат  : %-3s \r\n"
				 " Частный режим : %-3s     "
				 " Замедление    : %-3s     "
				 " Кодер         : %-3s \r\n"
				 " Опечатки      : %-3s \r\n",
				 ONOFF(ch->IsFlagged(EPrf::kNohassle)),
				 ONOFF(ch->IsFlagged(EPrf::kHolylight)),
				 ONOFF(ch->IsFlagged(EPrf::kRoomFlags)),
				 ONOFF(ch->IsFlagged(EPrf::kNoWiz)),
				 ONOFF(nameserver_is_slow),
				 ONOFF(ch->IsFlagged(EPrf::kCoderinfo)),
				 ONOFF(ch->IsFlagged(EPrf::kShowUnread)));
		SendMsgToChar(buf, ch);
	}

	snprintf(buf, kMaxStringLength,
			 " Автовыходы    : %-3s     "
			 " Краткий режим : %-3s     "
			 " Сжатый режим  : %-3s \r\n"
			 " Повтор команд : %-3s     "
			 " Обращения     : %-3s     "
			 " Цвет          : %-8s \r\n"
			 " Кто-то        : %-6s  "
			 " Болтать       : %-3s     "
			 " Орать         : %-3s \r\n"
			 " Аукцион       : %-3s     "
			 " Базар         : %-3s     "
			 " Автозаучивание: %-3s \r\n"
			 " Призыв        : %-3s     "
			 " Автозавершение: %-3s     "
			 " Группа (вид)  : %-7s \r\n"
			 " Без двойников : %-3s     "
			 " Автопомощь    : %-3s     "
			 " Автодележ     : %-3s \r\n"
			 " Автограбеж    : %-7s "
			 " Брать куны    : %-3s     "
			 " Арена         : %-3s \r\n"
			 " Трусость      : %-3s     "
			 " Ширина экрана : %-3d     "
			 " Высота экрана : %-3d \r\n"
			 " Сжатие        : %-6s  "
			 " Новости (вид) : %-5s   "
			 " Доски         : %-3s \r\n"
			 " Хранилище     : %-8s"
			 " Пклист        : %-3s     "
			 " Политика      : %-3s \r\n"
			 " Пкформат      : %-6s  "
			 " Соклановцы    : %-8s"
			 " Оффтоп        : %-3s \r\n"
			 " Потеря связи  : %-3s     "
			 " Ингредиенты   : %-3s     "
			 " Вспомнить     : %-3u \r\n",
			 ONOFF(ch->IsFlagged(EPrf::kAutoexit)),
			 ONOFF(ch->IsFlagged(EPrf::kBrief)),
			 ONOFF(ch->IsFlagged(EPrf::kCompact)),
			 YESNO(!ch->IsFlagged(EPrf::kNoRepeat)),
			 ONOFF(!ch->IsFlagged(EPrf::kNoTell)),
			 ctypes[COLOR_LEV(ch)],
			 ch->IsFlagged(EPrf::kNoInvistell) ? "нельзя" : "можно",
			 ONOFF(!ch->IsFlagged(EPrf::kNoGossip)),
			 ONOFF(!ch->IsFlagged(EPrf::kNoHoller)),
			 ONOFF(!ch->IsFlagged(EPrf::kNoAuction)),
			 ONOFF(!ch->IsFlagged(EPrf::kNoExchange)),
			 ONOFF(ch->IsFlagged(EPrf::kAutomem)),
			 ONOFF(ch->IsFlagged(EPrf::KSummonable)),
			 ONOFF(ch->IsFlagged(EPrf::kGoAhead)),
			 ch->IsFlagged(EPrf::kShowGroup) ? "полный" : "краткий",
			 ONOFF(ch->IsFlagged(EPrf::kNoClones)),
			 ONOFF(ch->IsFlagged(EPrf::kAutoassist)),
			 ONOFF(ch->IsFlagged(EPrf::kAutosplit)),
			 ch->IsFlagged(EPrf::kAutoloot) ? ch->IsFlagged(EPrf::kNoIngrLoot) ? "NO-INGR" : "ALL    " : "OFF    ",
			 ONOFF(ch->IsFlagged(EPrf::kAutomoney)),
			 ONOFF(!ch->IsFlagged(EPrf::kNoArena)),
			 buf2,
			 STRING_LENGTH(ch),
			 STRING_WIDTH(ch),
#if defined(HAVE_ZLIB)
			 ch->desc->deflate == nullptr ? "нет" : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"),
#else
		"N/A",
#endif
			 ch->IsFlagged(EPrf::kNewsMode) ? "доска" : "лента",
			 ONOFF(ch->IsFlagged(EPrf::kBoardMode)),
			 GetChestMode(ch).c_str(),
			 ONOFF(ch->IsFlagged(EPrf::kPklMode)),
			 ONOFF(ch->IsFlagged(EPrf::kPolitMode)),
			 ch->IsFlagged(EPrf::kPkFormatMode) ? "краткий" : "полный",
			 ONOFF(ch->IsFlagged(EPrf::kClanmembersMode)),
			 ONOFF(ch->IsFlagged(EPrf::kOfftopMode)),
			 ONOFF(ch->IsFlagged(EPrf::kAntiDcMode)),
			 ONOFF(ch->IsFlagged(EPrf::kNoIngrMode)),
			 ch->remember_get_num());
	SendMsgToChar(buf, ch);
	if (NOTIFY_EXCH_PRICE(ch) > 0) {
		sprintf(buf, " Уведомления   : %-7ld ", NOTIFY_EXCH_PRICE(ch));
	} else {
		sprintf(buf, " Уведомления   : %-7s ", "Нет");
	}
	SendMsgToChar(buf, ch);
	snprintf(buf, kMaxStringLength,
			 " Карта         : %-3s     "
			 " Вход в зону   : %-3s   \r\n"
			 " Магщиты (вид) : %-8s"
			 " Автопризыв    : %-5s   "
			 " Маппер        : %-3s   \r\n"
			 " Контроль IP   : %-6s  ",
			 ONOFF(ch->IsFlagged(EPrf::kDrawMap)),
			 ONOFF(ch->IsFlagged(EPrf::kShowZoneNameOnEnter)),
			 (ch->IsFlagged(EPrf::kBriefShields) ? "краткий" : "полный"),
			 ONOFF(ch->IsFlagged(EPrf::kAutonosummon)),
			 ONOFF(ch->IsFlagged(EPrf::kMapper)),
			 ONOFF(ch->IsFlagged(EPrf::kIpControl)));
	SendMsgToChar(buf, ch);
	if (GET_GOD_FLAG(ch, EGf::kAllowTesterMode))
		sprintf(buf, " Тестер        : %-3s\r\n", ONOFF(ch->IsFlagged(EPrf::kTester)));
	else
		sprintf(buf, "\r\n");
	SendMsgToChar(buf, ch);
}
