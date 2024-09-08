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

	if (GetRealLevel(ch) >= kLvlImmortal || PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		snprintf(buf, kMaxStringLength,
				 " Нет агров     : %-3s     "
				 " Супервидение  : %-3s     "
				 " Флаги комнат  : %-3s \r\n"
				 " Частный режим : %-3s     "
				 " Замедление    : %-3s     "
				 " Кодер         : %-3s \r\n"
				 " Опечатки      : %-3s \r\n",
				 ONOFF(PRF_FLAGGED(ch, EPrf::kNohassle)),
				 ONOFF(PRF_FLAGGED(ch, EPrf::kHolylight)),
				 ONOFF(PRF_FLAGGED(ch, EPrf::kRoomFlags)),
				 ONOFF(PRF_FLAGGED(ch, EPrf::kNoWiz)),
				 ONOFF(nameserver_is_slow),
				 ONOFF(PRF_FLAGGED(ch, EPrf::kCoderinfo)),
				 ONOFF(PRF_FLAGGED(ch, EPrf::kShowUnread)));
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
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAutoexit)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kBrief)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kCompact)),
			 YESNO(!PRF_FLAGGED(ch, EPrf::kNoRepeat)),
			 ONOFF(!PRF_FLAGGED(ch, EPrf::kNoTell)),
			 ctypes[COLOR_LEV(ch)],
			 PRF_FLAGGED(ch, EPrf::kNoInvistell) ? "нельзя" : "можно",
			 ONOFF(!PRF_FLAGGED(ch, EPrf::kNoGossip)),
			 ONOFF(!PRF_FLAGGED(ch, EPrf::kNoHoller)),
			 ONOFF(!PRF_FLAGGED(ch, EPrf::kNoAuction)),
			 ONOFF(!PRF_FLAGGED(ch, EPrf::kNoExchange)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAutomem)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::KSummonable)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kGoAhead)),
			 PRF_FLAGGED(ch, EPrf::kShowGroup) ? "полный" : "краткий",
			 ONOFF(PRF_FLAGGED(ch, EPrf::kNoClones)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAutoassist)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAutosplit)),
			 PRF_FLAGGED(ch, EPrf::kAutoloot) ? PRF_FLAGGED(ch, EPrf::kNoIngrLoot) ? "NO-INGR" : "ALL    " : "OFF    ",
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAutomoney)),
			 ONOFF(!PRF_FLAGGED(ch, EPrf::kNoArena)),
			 buf2,
			 STRING_LENGTH(ch),
			 STRING_WIDTH(ch),
#if defined(HAVE_ZLIB)
			 ch->desc->deflate == nullptr ? "нет" : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"),
#else
		"N/A",
#endif
			 PRF_FLAGGED(ch, EPrf::kNewsMode) ? "доска" : "лента",
			 ONOFF(PRF_FLAGGED(ch, EPrf::kBoardMode)),
			 GetChestMode(ch).c_str(),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kPklMode)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kPolitMode)),
			 PRF_FLAGGED(ch, EPrf::kPkFormatMode) ? "краткий" : "полный",
			 ONOFF(PRF_FLAGGED(ch, EPrf::kClanmembersMode)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kOfftopMode)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAntiDcMode)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kNoIngrMode)),
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
			 ONOFF(PRF_FLAGGED(ch, EPrf::kDrawMap)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kShowZoneNameOnEnter)),
			 (PRF_FLAGGED(ch, EPrf::kBriefShields) ? "краткий" : "полный"),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kAutonosummon)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kMapper)),
			 ONOFF(PRF_FLAGGED(ch, EPrf::kIpControl)));
	SendMsgToChar(buf, ch);
	if (GET_GOD_FLAG(ch, EGf::kAllowTesterMode))
		sprintf(buf, " Тестер        : %-3s\r\n", ONOFF(PRF_FLAGGED(ch, EPrf::kTester)));
	else
		sprintf(buf, "\r\n");
	SendMsgToChar(buf, ch);
}
