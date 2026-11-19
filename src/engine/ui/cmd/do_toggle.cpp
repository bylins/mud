//
// Created by Sventovit on 07.09.2024.
//

#include "utils/native_text.h"
#include "engine/entities/char_data.h"
#include "engine/ui/table_wrapper.h"
#include "gameplay/clans/house.h"

#include <string>
#include <utility>
#include <vector>

extern int nameserver_is_slow; //config.cpp
const char *BoolToOnOffStr(bool value);

namespace {

// Настройка экрана "режимы": подпись и её значение. Раскладка по колонкам больше не
// считается вручную -- добавить режим значит добавить сюда одну строку.
using ModeRow = std::pair<std::string, std::string>;

// Сколько пар "подпись: значение" в строке. Три пары -- шесть колонок таблицы.
constexpr std::size_t kPairsPerRow = 3;

void AddModes(CharData *ch, std::vector<ModeRow> &rows) {
	if (GetRealLevel(ch) >= kLvlImmortal || ch->IsFlagged(EPrf::kCoderinfo)) {
		rows.emplace_back("Нет агров", BoolToOnOffStr(ch->IsFlagged(EPrf::kNohassle)));
		rows.emplace_back("Супервидение", BoolToOnOffStr(ch->IsFlagged(EPrf::kHolylight)));
		rows.emplace_back("Флаги комнат", BoolToOnOffStr(ch->IsFlagged(EPrf::kRoomFlags)));
		rows.emplace_back("Частный режим", BoolToOnOffStr(ch->IsFlagged(EPrf::kNoWiz)));
		rows.emplace_back("Замедление", BoolToOnOffStr(nameserver_is_slow));
		rows.emplace_back("Кодер", BoolToOnOffStr(ch->IsFlagged(EPrf::kCoderinfo)));
		rows.emplace_back("Опечатки", BoolToOnOffStr(ch->IsFlagged(EPrf::kShowUnread)));
	}

	rows.emplace_back("Автовыходы", BoolToOnOffStr(ch->IsFlagged(EPrf::kAutoexit)));
	rows.emplace_back("Краткий режим", BoolToOnOffStr(ch->IsFlagged(EPrf::kBrief)));
	rows.emplace_back("Сжатый режим", BoolToOnOffStr(ch->IsFlagged(EPrf::kCompact)));
	rows.emplace_back("Повтор команд", ch->IsFlagged(EPrf::kNoRepeat) ? "NO" : "YES");
	rows.emplace_back("Обращения", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoTell)));
	rows.emplace_back("Кто-то", ch->IsFlagged(EPrf::kNoInvistell) ? "нельзя" : "можно");
	rows.emplace_back("Болтать", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoGossip)));
	rows.emplace_back("Орать", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoHoller)));
	rows.emplace_back("Аукцион", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoAuction)));
	rows.emplace_back("Базар", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoExchange)));
	rows.emplace_back("Автозаучивание", BoolToOnOffStr(ch->IsFlagged(EPrf::kAutomem)));
	rows.emplace_back("Призыв", BoolToOnOffStr(ch->IsFlagged(EPrf::KSummonable)));
	rows.emplace_back("Автозавершение", BoolToOnOffStr(ch->IsFlagged(EPrf::kGoAhead)));
	rows.emplace_back("Группа (вид)", ch->IsFlagged(EPrf::kShowGroup) ? "полный" : "краткий");
	rows.emplace_back("Без двойников", BoolToOnOffStr(ch->IsFlagged(EPrf::kNoClones)));
	rows.emplace_back("Автопомощь", BoolToOnOffStr(ch->IsFlagged(EPrf::kAutoassist)));
	rows.emplace_back("Автодележ", BoolToOnOffStr(ch->IsFlagged(EPrf::kAutosplit)));
	rows.emplace_back("Автограбеж", ch->IsFlagged(EPrf::kAutoloot)
									? (ch->IsFlagged(EPrf::kNoIngrLoot) ? "NO-INGR" : "ALL")
									: "OFF");
	rows.emplace_back("Брать куны", BoolToOnOffStr(ch->IsFlagged(EPrf::kAutomoney)));
	rows.emplace_back("Арена", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoArena)));
	rows.emplace_back("Трусость", GET_WIMP_LEV(ch) == 0 ? "нет" : std::to_string(GET_WIMP_LEV(ch)));
	rows.emplace_back("Ширина экрана", std::to_string(ch->player_specials->saved.stringLength));
	// флаг -- выключатель, поэтому показываем обратное ему
	rows.emplace_back("Перенос строк", BoolToOnOffStr(!ch->IsFlagged(EPrf::kNoLineWrap)));
	rows.emplace_back("Высота экрана", std::to_string(ch->player_specials->saved.stringWidth));
#if defined(HAVE_ZLIB)
	rows.emplace_back("Сжатие", ch->desc->deflate == nullptr
							   ? "нет"
							   : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"));
#else
	rows.emplace_back("Сжатие", "N/A");
#endif
	rows.emplace_back("Новости (вид)", ch->IsFlagged(EPrf::kNewsMode) ? "доска" : "лента");
	rows.emplace_back("Доски", BoolToOnOffStr(ch->IsFlagged(EPrf::kBoardMode)));
	rows.emplace_back("Хранилище", GetChestMode(ch));
	rows.emplace_back("Пклист", BoolToOnOffStr(ch->IsFlagged(EPrf::kPklMode)));
	rows.emplace_back("Политика", BoolToOnOffStr(ch->IsFlagged(EPrf::kPolitMode)));
	rows.emplace_back("Пкформат", ch->IsFlagged(EPrf::kPkFormatMode) ? "краткий" : "полный");
	rows.emplace_back("Соклановцы", BoolToOnOffStr(ch->IsFlagged(EPrf::kClanmembersMode)));
	rows.emplace_back("Оффтоп", BoolToOnOffStr(ch->IsFlagged(EPrf::kOfftopMode)));
	rows.emplace_back("Потеря связи", BoolToOnOffStr(ch->IsFlagged(EPrf::kAntiDcMode)));
	rows.emplace_back("Ингредиенты", BoolToOnOffStr(ch->IsFlagged(EPrf::kNoIngrMode)));
	rows.emplace_back("Вспомнить", std::to_string(ch->remember_get_num()));
	rows.emplace_back("Уведомления", ch->player_specials->saved.ntfyExchangePrice > 0
									 ? std::to_string(ch->player_specials->saved.ntfyExchangePrice)
									 : "Нет");
	rows.emplace_back("Карта", BoolToOnOffStr(ch->IsFlagged(EPrf::kDrawMap)));
	rows.emplace_back("Вход в зону", BoolToOnOffStr(ch->IsFlagged(EPrf::kShowZoneNameOnEnter)));
	rows.emplace_back("Магщиты (вид)", ch->IsFlagged(EPrf::kBriefShields) ? "краткий" : "полный");
	rows.emplace_back("Автопризыв", BoolToOnOffStr(ch->IsFlagged(EPrf::kAutonosummon)));
	rows.emplace_back("Маппер", BoolToOnOffStr(ch->IsFlagged(EPrf::kMapper)));
	rows.emplace_back("Контроль IP", BoolToOnOffStr(ch->IsFlagged(EPrf::kIpControl)));

	if (GET_GOD_FLAG(ch, EGf::kAllowTesterMode)) {
		rows.emplace_back("Тестер", BoolToOnOffStr(ch->IsFlagged(EPrf::kTester)));
	}
}

}  // namespace

void do_toggle(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;

	std::vector<ModeRow> rows;
	AddModes(ch, rows);

	table_wrapper::Table table;
	for (std::size_t i = 0; i < rows.size(); ++i) {
		// Двоеточие -- отдельной колонкой: так они выстраиваются по вертикали, и глазу
		// есть за что зацепиться. Приписанное к подписи, оно ездило бы вслед за её длиной.
		table << rows[i].first << ":" << rows[i].second;
		if ((i + 1) % kPairsPerRow == 0) {
			table << table_wrapper::kEndRow;
		}
	}
	if (rows.size() % kPairsPerRow != 0) {
		table << table_wrapper::kEndRow;
	}

	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToChar(ch, table);
}

const char *BoolToOnOffStr(bool value) {
	return (value ? "ON" : "OFF");
}
