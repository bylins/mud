//
// Created by Sventovit on 07.09.2024.
//
#include "engine/entities/char_data.h"
#include "do_toggle.h"
#include "gameplay/clans/house.h"
#include "engine/ui/table_wrapper.h"
#include "gameplay/communication/remember.h"

const int kToggleOff{0};
const int kToggleOn{1};

extern int nameserver_is_slow; //config.cpp

void do_gen_tog(CharData *ch, char *argument, int/* cmd*/, int subcmd);
void SetScreen(CharData *ch, char *argument, int flag);
void SetNotifyEchange(CharData *ch, char *argument);
void SetAutolootMode(CharData *ch, char *argument);
bool TogglePrfFlag(CharData *ch, EPrf flag);

enum EScmd {
  kScmdNosummon,
  kScmdNohassle,
  kScmdBrief,
  kScmdCompact,
  kScmdNotell,
  kScmdNoauction,
  kScmdNoholler,
  kScmdNogossip,
  kScmdNogratz,
  kScmdNowiz,
  kScmdQuest,
  kScmdRoomflags,
  kScmdNorepeat,
  kScmdHolylight,
  kScmdSlowns,
  kScmdAutoexit,
  kScmdTrack,
  kScmdCoderinfo,
  kScmdAutomem,
  kScmdCompress,
  kScmdNoshout,
  kScmdGoahead,
  kScmdShowgroup,
  kScmdAutoassist,
  kScmdAutoloot,
  kScmdAutosplit,
  kScmdAutomoney,
  kScmdNoarena,
  kScmdNoexchange,
  kScmdNoclones,
  kScmdNoinvistell,
  kScmdLength,
  kScmdWidth,
  kScmdScreen,
  kScmdNewsMode,
  kScmdBoardMode,
  kScmdChestMode,
  kScmdPklMode,
  kScmdPolitMode,
  kScmdPkformatMode,
  kScmdWorkmateMode,
  kScmdOfftopMode,
  kScmdAntidcMode,
  kScmdNoingrMode,
  kScmdRemember,
  kScmdNotifyExch,
  kScmdDrawMap,
  kScmdEnterZone,
  kScmdMisprint,
  kScmdBriefShields,
  kScmdAutoNosummon,
  kScmdSdemigod,
  kScmdBlind,
  kScmdMapper,
  kScmdTester,
  kScmdIpcontrol
};

const char *gen_tog_type[] = {"автовыходы", "autoexits",
							  "краткий", "brief",
							  "сжатый", "compact",
							  "повтор", "norepeat",
							  "обращения", "notell",
							  "кто-то", "noinvistell",
							  "болтать", "nogossip",
							  "кричать", "noshout",
							  "орать", "noholler",
							  "поздравления", "nogratz",
							  "аукцион", "noauction",
							  "базар", "exchange",
							  "задание", "quest",
							  "автозаучивание", "automem",
							  "нет агров", "nohassle",
							  "призыв", "nosummon",
							  "частный", "nowiz",
							  "флаги комнат", "roomflags",
							  "замедление", "slowns",
							  "выслеживание", "trackthru",
							  "супервидение", "holylight",
							  "кодер", "coder",
							  "автозавершение", "goahead",
							  "группа", "showgroup",
							  "без двойников", "noclones",
							  "автопомощь", "autoassist",
							  "автограбеж", "autoloot",
							  "автодележ", "autosplit",
							  "брать куны", "automoney",
							  "арена", "arena",
							  "ширина", "length",
							  "высота", "width",
							  "экран", "screen",
							  "новости", "news",
							  "доски", "boards",
							  "хранилище", "chest",
							  "пклист", "pklist",
							  "политика", "politics",
							  "пкформат", "pkformat",
							  "соклановцы", "workmate",
							  "оффтоп", "offtop",
							  "потеря связи", "disconnect",
							  "ингредиенты", "ingredient",
							  "вспомнить", "remember",
							  "уведомления", "notify",
							  "карта", "map",
							  "вход в зону", "enter zone",
							  "опечатки", "misprint",
							  "магщиты", "mageshields",
							  "автопризыв", "autonosummon",
							  "сдемигодам", "sdemigod",
							  "незрячий", "blind",
							  "маппер", "mapper",
							  "тестер", "tester",
							  "контроль IP", "IP control",
							  "\n"
};

struct gen_tog_param_type {
  int level;
  int subcmd;
  bool tester;
} gen_tog_param[] =
	{
		{
			0, kScmdAutoexit, false}, {
			0, kScmdBrief, false}, {
			0, kScmdCompact, false}, {
			0, kScmdNorepeat, false}, {
			0, kScmdNotell, false}, {
			0, kScmdNoinvistell, false}, {
			0, kScmdNogossip, false}, {
			0, kScmdNoshout, false}, {
			0, kScmdNoholler, false}, {
			0, kScmdNogratz, false}, {
			0, kScmdNoauction, false}, {
			0, kScmdNoexchange, false}, {
			0, kScmdQuest, false}, {
			0, kScmdAutomem, false}, {
			kLvlGreatGod, kScmdNohassle, false}, {
			0, kScmdNosummon, false}, {
			kLvlGod, kScmdNowiz, false}, {
			kLvlGreatGod, kScmdRoomflags, false}, {
			kLvlImplementator, kScmdSlowns, false}, {
			kLvlGod, kScmdTrack, false}, {
			kLvlGod, kScmdHolylight, false}, {
			kLvlImplementator, kScmdCoderinfo, false}, {
			0, kScmdGoahead, false}, {
			0, kScmdShowgroup, false}, {
			0, kScmdNoclones, false}, {
			0, kScmdAutoassist, false}, {
			0, kScmdAutoloot, false}, {
			0, kScmdAutosplit, false}, {
			0, kScmdAutomoney, false}, {
			0, kScmdNoarena, false}, {
			0, kScmdLength, false}, {
			0, kScmdWidth, false}, {
			0, kScmdScreen, false}, {
			0, kScmdNewsMode, false}, {
			0, kScmdBoardMode, false}, {
			0, kScmdChestMode, false}, {
			0, kScmdPklMode, false}, {
			0, kScmdPolitMode, false}, {
			0, kScmdPkformatMode, false}, {
			0, kScmdWorkmateMode, false}, {
			0, kScmdOfftopMode, false}, {
			0, kScmdAntidcMode, false}, {
			0, kScmdNoingrMode, false}, {
			0, kScmdRemember, false}, {
			0, kScmdNotifyExch, false}, {
			0, kScmdDrawMap, false}, {
			0, kScmdEnterZone, false}, {
			kLvlGod, kScmdMisprint, false}, {
			0, kScmdBriefShields, false}, {
			0, kScmdAutoNosummon, false}, {
			kLvlImplementator, kScmdSdemigod, false}, {
			0, kScmdBlind, false}, {
			0, kScmdMapper, false}, {
			0, kScmdTester, true}, {
			0, kScmdIpcontrol, false}
	};

void DoMode(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}

	argument = one_argument(argument, arg);
//	skip_spaces(&argument);
	int i{0};
	bool showhelp{false};
	if (!*arg) {
		do_toggle(ch, argument, 0, 0);
		return;
	} else if (*arg == '?') {
		showhelp = true;
	} else if ((i = search_block(arg, gen_tog_type, false)) < 0) {
		showhelp = true;
	} else if ((GetRealLevel(ch) < gen_tog_param[i >> 1].level)
		|| (!GET_GOD_FLAG(ch, EGf::kAllowTesterMode) && gen_tog_param[i >> 1].tester)) {
		SendMsgToChar("Эта команда вам недоступна.\r\n", ch);
		return;
	} else {
		do_gen_tog(ch, argument, 0, gen_tog_param[i >> 1].subcmd);
	}

	if (showhelp) {
		SendMsgToChar(" Можно установить:\r\n", ch);
		table_wrapper::Table table;
		for (i = 0; *gen_tog_type[i << 1] != '\n'; i++) {
			if ((GetRealLevel(ch) >= gen_tog_param[i].level)
				&& (GET_GOD_FLAG(ch, EGf::kAllowTesterMode) || !gen_tog_param[i].tester)) {
				table << gen_tog_type[i << 1] << gen_tog_type[(i << 1) + 1] << table_wrapper::kEndRow;
			}
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToChar(ch, table);
	}
}

void do_gen_tog(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	long result = 0;

	const char *tog_messages[][2] =
		{
			{"Вы защищены от призыва.\r\n",
			 "Вы можете быть призваны.\r\n"},
			{"Nohassle disabled.\r\n",
			 "Nohassle enabled.\r\n"},
			{"Краткий режим выключен.\r\n",
			 "Краткий режим включен.\r\n"},
			{"Сжатый режим выключен.\r\n",
			 "Сжатый режим включен.\r\n"},
			{"К вам можно обратиться.\r\n",
			 "Вы глухи к обращениям.\r\n"},
			{"Вам будут выводиться сообщения аукциона.\r\n",
			 "Вы отключены от участия в аукционе.\r\n"},
			{"Вы слышите то, что орут.\r\n",
			 "Вы глухи к тому, что орут.\r\n"},
			{"Вы слышите всю болтовню.\r\n",
			 "Вы глухи к болтовне.\r\n"},
			{"Вы слышите все поздравления.\r\n",
			 "Вы глухи к поздравлениям.\r\n"},
			{"You can now hear the Wiz-channel.\r\n",
			 "You are now deaf to the Wiz-channel.\r\n"},
			{"Вы больше не выполняете задания.\r\n",
			 "Вы выполняете задание!\r\n"},
			{"Вы больше не будете видеть флаги комнат.\r\n",
			 "Вы способны различать флаги комнат.\r\n"},
			{"Ваши сообщения будут дублироваться.\r\n",
			 "Ваши сообщения не будут дублироваться вам.\r\n"},
			{"HolyLight mode off.\r\n",
			 "HolyLight mode on.\r\n"},
			{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
			 "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
			{"Показ выходов автоматически выключен.\r\n",
			 "Показ выходов автоматически включен.\r\n"},
			{"Вы не можете более выследить сквозь двери.\r\n",
			 "Вы способны выслеживать сквозь двери.\r\n"},
			{"\r\n",
			 "\r\n"},
			{"Режим показа дополнительной информации выключен.\r\n",
			 "Режим показа дополнительной информации включен.\r\n"},
			{"Автозаучивание заклинаний выключено.\r\n",
			 "Автозаучивание заклинаний включено.\r\n"},
			{"Сжатие запрещено.\r\n",
			 "Сжатие разрешено.\r\n"},
			{"\r\n",
			 "\r\n"},
			{"Вы слышите все крики.\r\n",
			 "Вы глухи к крикам.\r\n"},
			{"Режим автозавершения (IAC GA) выключен.\r\n",
			 "Режим автозавершения (IAC GA) включен.\r\n"},
			{"При просмотре состава группы будут отображаться только персонажи и ваши последователи.\r\n",
			 "При просмотре состава группы будут отображаться все персонажи и последователи.\r\n"},
			{"Вы не будете автоматически помогать согрупникам.\r\n",
			 "Вы будете автоматически помогать согрупникам.\r\n"},
			{"", ""}, // SCMD_AUTOLOOT
			{"Вы не будете делить деньги, поднятые с трупов.\r\n",
			 "Вы будете автоматически делить деньги, поднятые с трупов.\r\n"},
			{"Вы не будете брать куны, оставшиеся в трупах.\r\n",
			 "Вы будете автоматически брать куны, оставшиеся в трупах.\r\n"},
			{"Вы будете слышать сообщения с арены.\r\n",
			 "Вы не будете слышать сообщения с арены.\r\n"},
			{"Вам будут выводиться сообщения базара.\r\n",
			 "Вы отключены от участия в базаре.\r\n"},
			{"При просмотре состава группы будут отображаться все последователи.\r\n",
			 "При просмотре состава группы не будут отображаться чужие двойники и хранители.\r\n"},
			{"К вам сможет обратиться кто угодно.\r\n",
			 "К вам смогут обратиться только те, кого вы видите.\r\n"},
			{"", ""}, // SCMD_LENGTH
			{"", ""}, // SCMD_WIDTH
			{"", ""}, // SCMD_SCREEN
			{"Вариант чтения новостей былин и дружины: лента.\r\n",
			 "Вариант чтения новостей былин и дружины: доска.\r\n"},
			{"Вы не видите уведомлений о новых сообщениях на досках.\r\n",
			 "Вы получаете уведомления о новых сообщениях на досках.\r\n"},
			{"", ""}, // SCMD_CHEST_MODE
			{"Вы игнорируете уведомления о добавлении или очистке вас из листов дружин.\r\n",
			 "Вы получаете уведомления о добавлении или очистке вас из листов дружин.\r\n"},
			{"Вы игнорируете уведомления об изменениях политики вашей и к вашей дружине.\r\n",
			 "Вы получаете уведомления об изменениях политики вашей и к вашей дружине.\r\n"},
			{"Формат показа пкл/дрл установлен как 'полный'.\r\n",
			 "Формат показа пкл/дрл установлен как 'краткий'.\r\n"},
			{"Вам не будут показываться входы и выходы из игры ваших соклановцев.\r\n",
			 "Вы видите входы и выходы из игры ваших соклановцев.\r\n"},
			{"Вы отключены от канала оффтоп.\r\n",
			 "Вы будете слышать болтовню в канале оффтоп.\r\n"},
			{"Вы отключили защиту от потери связь во время боя.\r\n",
			 "Защита от потери связи во время боя включена.\r\n"},
			{"Показ покупок и продаж ингредиентов в режиме базара отключен.\r\n",
			 "Показ покупок и продаж ингредиентов в режиме базара включен.\r\n"},
			{"", ""},        // SCMD_REMEMBER
			{"", ""},        //SCMD_NOTIFY_EXCH
			{"Показ карты окрестностей при перемещении отключен.\r\n",
			 "Вы будете видеть карту окрестностей при перемещении.\r\n"},
			{"Показ информации при входе в новую зону отключен.\r\n",
			 "Вы будете видеть информацию при входе в новую зону.\r\n"},
			{"Показ уведомлений доски опечаток отключен.\r\n",
			 "Вы будете видеть уведомления доски опечаток.\r\n"},
			{"Показ сообщений при срабатывании магических щитов: полный.\r\n",
			 "Показ сообщений при срабатывании магических щитов: краткий.\r\n"},
			{"Автоматический режим защиты от призыва выключен.\r\n",
			 "Вы будете автоматически включать режим защиты от призыва после его использования.\r\n"},
			{"Канал для демигодов выключен.\r\n",
			 "Канал для демигодов включен.\r\n"},
			{"Режим слепого игрока недоступен. Выключайте его в главном меню.\r\n",
			 "Режим слепого игрока недоступен. Включайте его в главном меню.\r\n"},
			{"Режим для мапперов выключен.\r\n",
			 "Режим для мапперов включен.\r\n"},
			{"Режим вывода тестовой информации выключен.\r\n",
			 "Режим вывода тестовой информации включен.\r\n"},
			{"Режим контроля смены IP-адреса персонажа выключен.\r\n",
			 "Режим контроля смены IP-адреса персонажа включен.\r\n"}
		};

	if (ch->IsNpc())
		return;

	switch (subcmd) {
		case kScmdNosummon: result = TogglePrfFlag(ch, EPrf::KSummonable);
			break;
		case kScmdNohassle: result = TogglePrfFlag(ch, EPrf::kNohassle);
			break;
		case kScmdBrief: result = TogglePrfFlag(ch, EPrf::kBrief);
			break;
		case kScmdCompact: result = TogglePrfFlag(ch, EPrf::kCompact);
			break;
		case kScmdNotell: result = TogglePrfFlag(ch, EPrf::kNoTell);
			break;
		case kScmdNoauction: result = TogglePrfFlag(ch, EPrf::kNoAuction);
			break;
		case kScmdNoholler: result = TogglePrfFlag(ch, EPrf::kNoHoller);
			break;
		case kScmdNogossip: result = TogglePrfFlag(ch, EPrf::kNoGossip);
			break;
		case kScmdNoshout: result = TogglePrfFlag(ch, EPrf::kNoShout);
			break;
		case kScmdNogratz: result = TogglePrfFlag(ch, EPrf::kNoGossip);
			break;
		case kScmdNowiz: result = TogglePrfFlag(ch, EPrf::kNoWiz);
			break;
		case kScmdQuest: result = TogglePrfFlag(ch, EPrf::kQuest);
			break;
		case kScmdRoomflags: result = TogglePrfFlag(ch, EPrf::kRoomFlags);
			break;
		case kScmdNorepeat: result = TogglePrfFlag(ch, EPrf::kNoRepeat);
			break;
		case kScmdHolylight: result = TogglePrfFlag(ch, EPrf::kHolylight);
			break;
		case kScmdSlowns: result = (nameserver_is_slow = !nameserver_is_slow);
			break;
		case kScmdAutoexit:
			if (ch->IsFlagged(EPlrFlag::kScriptWriter)) {
				SendMsgToChar("Скриптерам запрещено видеть автовыходы.\r\n", ch);
				return;
			}
			result = TogglePrfFlag(ch, EPrf::kAutoexit);
			break;
		case kScmdCoderinfo: result = TogglePrfFlag(ch, EPrf::kCoderinfo);
			break;
		case kScmdAutomem: result = TogglePrfFlag(ch, EPrf::kAutomem);
			break;
		case kScmdSdemigod: result = TogglePrfFlag(ch, EPrf::kDemigodChat);
			break;
		case kScmdBlind: break;
		case kScmdMapper:
			if (ch->IsFlagged(EPlrFlag::kScriptWriter)) {
				SendMsgToChar("Скриптерам запрещено видеть vnum комнаты.\r\n", ch);
				return;
			}
			result = TogglePrfFlag(ch, EPrf::kMapper);
			break;
		case kScmdTester:
			//if (GET_GOD_FLAG(ch, EGodFlag::TESTER))
			//{
			result = TogglePrfFlag(ch, EPrf::kTester);
			//return;
			//}
			break;
		case kScmdIpcontrol: result = TogglePrfFlag(ch, EPrf::kIpControl);
			break;
#if defined(HAVE_ZLIB)
		case kScmdCompress: result = iosystem::toggle_compression(ch->desc);
			break;
#else
			case SCMD_COMPRESS:
				SendMsgToChar("Compression not supported.\r\n", ch);
				return;
#endif
		case kScmdGoahead: result = TogglePrfFlag(ch, EPrf::kGoAhead);
			break;
		case kScmdShowgroup: result = TogglePrfFlag(ch, EPrf::kShowGroup);
			break;
		case kScmdAutoassist: result = TogglePrfFlag(ch, EPrf::kAutoassist);
			break;
		case kScmdAutoloot: SetAutolootMode(ch, argument);
			return;
		case kScmdAutosplit: result = TogglePrfFlag(ch, EPrf::kAutosplit);
			break;
		case kScmdAutomoney: result = TogglePrfFlag(ch, EPrf::kAutomoney);
			break;
		case kScmdNoarena: result = TogglePrfFlag(ch, EPrf::kNoArena);
			break;
		case kScmdNoexchange: result = TogglePrfFlag(ch, EPrf::kNoExchange);
			break;
		case kScmdNoclones: result = TogglePrfFlag(ch, EPrf::kNoClones);
			break;
		case kScmdNoinvistell: result = TogglePrfFlag(ch, EPrf::kNoInvistell);
			break;
		case kScmdLength: SetScreen(ch, argument, 0);
			return;
		case kScmdWidth: SetScreen(ch, argument, 1);
			return;
		case kScmdScreen: SetScreen(ch, argument, 2);
			return;
		case kScmdNewsMode: result = TogglePrfFlag(ch, EPrf::kNewsMode);
			break;
		case kScmdBoardMode: result = TogglePrfFlag(ch, EPrf::kBoardMode);
			break;
		case kScmdChestMode: {
			std::string buffer = argument;
			SetChestMode(ch, buffer);
			break;
		}
		case kScmdPklMode: result = TogglePrfFlag(ch, EPrf::kPklMode);
			break;
		case kScmdPolitMode: result = TogglePrfFlag(ch, EPrf::kPolitMode);
			break;
		case kScmdPkformatMode: result = TogglePrfFlag(ch, EPrf::kPkFormatMode);
			break;
		case kScmdWorkmateMode: result = TogglePrfFlag(ch, EPrf::kClanmembersMode);
			break;
		case kScmdOfftopMode: result = TogglePrfFlag(ch, EPrf::kOfftopMode);
			break;
		case kScmdAntidcMode: result = TogglePrfFlag(ch, EPrf::kAntiDcMode);
			break;
		case kScmdNoingrMode: result = TogglePrfFlag(ch, EPrf::kNoIngrMode);
			break;
		case kScmdRemember: {
			skip_spaces(&argument);
			if (!*argument) {
				SendMsgToChar("Формат команды: режим вспомнить <число строк от 1 до 100>.\r\n", ch);
				return;
			}
			unsigned int size = atoi(argument);
			if (ch->remember_set_num(size)) {
				SendMsgToChar(ch, "Количество выводимых строк по команде 'вспомнить' установлено в %d.\r\n", size);
				ch->save_char();
			} else {
				SendMsgToChar(ch,
							  "Количество строк для вывода может быть в пределах от 1 до %d.\r\n",
							  Remember::MAX_REMEMBER_NUM);
			}
			return;
		}
		case kScmdNotifyExch: {
			SetNotifyEchange(ch, argument);
			return;
		}
		case kScmdDrawMap: {
			if (ch->IsFlagged(EPrf::kBlindMode)) {
				SendMsgToChar("В режиме слепого игрока карта недоступна.\r\n", ch);
				return;
			}
			result = TogglePrfFlag(ch, EPrf::kDrawMap);
			break;
		}
		case kScmdEnterZone: result = TogglePrfFlag(ch, EPrf::kShowZoneNameOnEnter);
			break;
		case kScmdMisprint: result = TogglePrfFlag(ch, EPrf::kShowUnread);
			break;
		case kScmdBriefShields: result = TogglePrfFlag(ch, EPrf::kBriefShields);
			break;
		case kScmdAutoNosummon: result = TogglePrfFlag(ch, EPrf::kAutonosummon);
			break;
		default: SendMsgToChar(ch, "Введите параметр режима полностью.\r\n");
//		log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
			return;
	}
	if (result)
		SendMsgToChar(tog_messages[subcmd][kToggleOn], ch);
	else
		SendMsgToChar(tog_messages[subcmd][kToggleOff], ch);
}

// установки экрана flag: 0 - ширина, 1 - высота
void SetScreen(CharData *ch, char *argument, int flag) {
	if (ch->IsNpc())
		return;
	skip_spaces(&argument);
	int size = atoi(argument);

	if (!flag && (size < 30 || size > 300))
		SendMsgToChar("Ширина экрана должна быть в пределах 30 - 300 символов.\r\n", ch);
	else if (flag == 1 && (size < 10 || size > 100))
		SendMsgToChar("Высота экрана должна быть в пределах 10 - 100 строк.\r\n", ch);
	else if (!flag) {
		STRING_LENGTH(ch) = size;
		SendMsgToChar("Ладушки.\r\n", ch);
		ch->save_char();
	} else if (flag == 1) {
		STRING_WIDTH(ch) = size;
		SendMsgToChar("Ладушки.\r\n", ch);
		ch->save_char();
	} else {
		std::ostringstream buffer;
		for (int i = 50; i > 0; --i)
			buffer << i << "\r\n";
		SendMsgToChar(buffer.str(), ch);
	}
}

void SetNotifyEchange(CharData *ch, char *argument) {
	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar(ch, "Формат команды: режим уведомления <минимальная цена, число от 0 до %d>.\r\n", 0x7fffffff);
		return;
	}
	long size = atol(argument);
	if (size >= 100) {
		SendMsgToChar(ch,
					  "Вам будут приходить уведомления о продаже с базара ваших лотов стоимостью не менее чем %ld %s.\r\n",
					  size,
					  GetDeclensionInNumber(size, EWhat::kMoneyA));
		NOTIFY_EXCH_PRICE(ch) = size;
		ch->save_char();
	} else if (size >= 0 && size < 100) {
		SendMsgToChar(ch,
					  "Вам не будут приходить уведомления о продаже с базара ваших лотов, так как указана цена меньше 100 кун.\r\n");
		NOTIFY_EXCH_PRICE(ch) = 0;
		ch->save_char();
	} else {
		SendMsgToChar(ch, "Укажите стоимость лота от 0 до %d\r\n", 0x7fffffff);
	}

}

void SetAutolootMode(CharData *ch, char *argument) {
	static const char *message_on = "Автоматический грабеж трупов включен.\r\n";
	static const char
		*message_no_ingr = "Автоматический грабеж трупов, исключая ингредиенты и магические компоненты, включен.\r\n";
	static const char *message_off = "Автоматический грабеж трупов выключен.\r\n";

	skip_spaces(&argument);
	if (!*argument) {
		if (TogglePrfFlag(ch, EPrf::kAutoloot)) {
			SendMsgToChar(ch->IsFlagged(EPrf::kNoIngrLoot) ? message_no_ingr : message_on, ch);
		} else {
			SendMsgToChar(message_off, ch);
		}
	} else if (utils::IsAbbr(argument, "все")) {
		ch->SetFlag(EPrf::kAutoloot);
		ch->UnsetFlag(EPrf::kNoIngrLoot);
		SendMsgToChar(message_on, ch);
	} else if (utils::IsAbbr(argument, "ингредиенты")) {
		ch->SetFlag(EPrf::kAutoloot);
		ch->SetFlag(EPrf::kNoIngrLoot);
		SendMsgToChar(message_no_ingr, ch);
	} else if (utils::IsAbbr(argument, "нет")) {
		ch->UnsetFlag(EPrf::kAutoloot);
		ch->UnsetFlag(EPrf::kNoIngrLoot);
		SendMsgToChar(message_off, ch);
	} else {
		SendMsgToChar("Формат команды: режим автограбеж <без-параметров|все|ингредиенты|нет>\r\n", ch);
	}
}

bool TogglePrfFlag(CharData *ch, EPrf flag) {
	const auto flagged = ch->IsFlagged(flag);
	ch->IsFlagged(flag) ? ch->UnsetFlag(flag) : ch->SetFlag(flag);
	return (!flagged);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
