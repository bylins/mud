//
// Created by Sventovit on 07.09.2024.
//
#include "entities/char_data.h"
#include "cmd/do_toggle.h"
#include "house.h"
#include "utils/table_wrapper.h"
#include "communication/remember.h"

const int kToggleOff{0};
const int kToggleOn{1};

extern int nameserver_is_slow; //config.cpp

void do_gen_tog(CharData *ch, char *argument, int/* cmd*/, int subcmd);
void SetScreen(CharData *ch, char *argument, int flag);
void setNotifyEchange(CharData *ch, char *argument);
void set_autoloot_mode(CharData *ch, char *argument);
bool TogglePrfFlag(CharData *ch, EPrf flag);

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
			0, SCMD_AUTOEXIT, false}, {
			0, SCMD_BRIEF, false}, {
			0, SCMD_COMPACT, false}, {
			0, SCMD_NOREPEAT, false}, {
			0, SCMD_NOTELL, false}, {
			0, SCMD_NOINVISTELL, false}, {
			0, SCMD_NOGOSSIP, false}, {
			0, SCMD_NOSHOUT, false}, {
			0, SCMD_NOHOLLER, false}, {
			0, SCMD_NOGRATZ, false}, {
			0, SCMD_NOAUCTION, false}, {
			0, SCMD_NOEXCHANGE, false}, {
			0, SCMD_QUEST, false}, {
			0, SCMD_AUTOMEM, false}, {
			kLvlGreatGod, SCMD_NOHASSLE, false}, {
			0, SCMD_NOSUMMON, false}, {
			kLvlGod, SCMD_NOWIZ, false}, {
			kLvlGreatGod, SCMD_ROOMFLAGS, false}, {
			kLvlImplementator, SCMD_SLOWNS, false}, {
			kLvlGod, SCMD_TRACK, false}, {
			kLvlGod, SCMD_HOLYLIGHT, false}, {
			kLvlImplementator, SCMD_CODERINFO, false}, {
			0, SCMD_GOAHEAD, false}, {
			0, SCMD_SHOWGROUP, false}, {
			0, SCMD_NOCLONES, false}, {
			0, SCMD_AUTOASSIST, false}, {
			0, SCMD_AUTOLOOT, false}, {
			0, SCMD_AUTOSPLIT, false}, {
			0, SCMD_AUTOMONEY, false}, {
			0, SCMD_NOARENA, false}, {
			0, SCMD_LENGTH, false}, {
			0, SCMD_WIDTH, false}, {
			0, SCMD_SCREEN, false}, {
			0, SCMD_NEWS_MODE, false}, {
			0, SCMD_BOARD_MODE, false}, {
			0, SCMD_CHEST_MODE, false}, {
			0, SCMD_PKL_MODE, false}, {
			0, SCMD_POLIT_MODE, false}, {
			0, SCMD_PKFORMAT_MODE, false}, {
			0, SCMD_WORKMATE_MODE, false}, {
			0, SCMD_OFFTOP_MODE, false}, {
			0, SCMD_ANTIDC_MODE, false}, {
			0, SCMD_NOINGR_MODE, false}, {
			0, SCMD_REMEMBER, false}, {
			0, SCMD_NOTIFY_EXCH, false}, {
			0, SCMD_DRAW_MAP, false}, {
			0, SCMD_ENTER_ZONE, false}, {
			kLvlGod, SCMD_MISPRINT, false}, {
			0, SCMD_BRIEF_SHIELDS, false}, {
			0, SCMD_AUTO_NOSUMMON, false}, {
			kLvlImplementator, SCMD_SDEMIGOD, false}, {
			0, SCMD_BLIND, false}, {
			0, SCMD_MAPPER, false}, {
			0, SCMD_TESTER, true}, {
			0, SCMD_IPCONTROL, false}
	};

void do_mode(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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
		case SCMD_NOSUMMON: result = TogglePrfFlag(ch, EPrf::KSummonable);
			break;
		case SCMD_NOHASSLE: result = TogglePrfFlag(ch, EPrf::kNohassle);
			break;
		case SCMD_BRIEF: result = TogglePrfFlag(ch, EPrf::kBrief);
			break;
		case SCMD_COMPACT: result = TogglePrfFlag(ch, EPrf::kCompact);
			break;
		case SCMD_NOTELL: result = TogglePrfFlag(ch, EPrf::kNoTell);
			break;
		case SCMD_NOAUCTION: result = TogglePrfFlag(ch, EPrf::kNoAuction);
			break;
		case SCMD_NOHOLLER: result = TogglePrfFlag(ch, EPrf::kNoHoller);
			break;
		case SCMD_NOGOSSIP: result = TogglePrfFlag(ch, EPrf::kNoGossip);
			break;
		case SCMD_NOSHOUT: result = TogglePrfFlag(ch, EPrf::kNoShout);
			break;
		case SCMD_NOGRATZ: result = TogglePrfFlag(ch, EPrf::kNoGossip);
			break;
		case SCMD_NOWIZ: result = TogglePrfFlag(ch, EPrf::kNoWiz);
			break;
		case SCMD_QUEST: result = TogglePrfFlag(ch, EPrf::kQuest);
			break;
		case SCMD_ROOMFLAGS: result = TogglePrfFlag(ch, EPrf::kRoomFlags);
			break;
		case SCMD_NOREPEAT: result = TogglePrfFlag(ch, EPrf::kNoRepeat);
			break;
		case SCMD_HOLYLIGHT: result = TogglePrfFlag(ch, EPrf::kHolylight);
			break;
		case SCMD_SLOWNS: result = (nameserver_is_slow = !nameserver_is_slow);
			break;
		case SCMD_AUTOEXIT:
			if (ch->IsFlagged(EPlrFlag::kScriptWriter)) {
				SendMsgToChar("Скриптерам запрещено видеть автовыходы.\r\n", ch);
				return;
			}
			result = TogglePrfFlag(ch, EPrf::kAutoexit);
			break;
		case SCMD_CODERINFO: result = TogglePrfFlag(ch, EPrf::kCoderinfo);
			break;
		case SCMD_AUTOMEM: result = TogglePrfFlag(ch, EPrf::kAutomem);
			break;
		case SCMD_SDEMIGOD: result = TogglePrfFlag(ch, EPrf::kDemigodChat);
			break;
		case SCMD_BLIND: break;
		case SCMD_MAPPER:
			if (ch->IsFlagged(EPlrFlag::kScriptWriter)) {
				SendMsgToChar("Скриптерам запрещено видеть vnum комнаты.\r\n", ch);
				return;
			}
			result = TogglePrfFlag(ch, EPrf::kMapper);
			break;
		case SCMD_TESTER:
			//if (GET_GOD_FLAG(ch, EGodFlag::TESTER))
			//{
			result = TogglePrfFlag(ch, EPrf::kTester);
			//return;
			//}
			break;
		case SCMD_IPCONTROL: result = TogglePrfFlag(ch, EPrf::kIpControl);
			break;
#if defined(HAVE_ZLIB)
		case SCMD_COMPRESS: result = toggle_compression(ch->desc);
			break;
#else
			case SCMD_COMPRESS:
				SendMsgToChar("Compression not supported.\r\n", ch);
				return;
#endif
		case SCMD_GOAHEAD: result = TogglePrfFlag(ch, EPrf::kGoAhead);
			break;
		case SCMD_SHOWGROUP: result = TogglePrfFlag(ch, EPrf::kShowGroup);
			break;
		case SCMD_AUTOASSIST: result = TogglePrfFlag(ch, EPrf::kAutoassist);
			break;
		case SCMD_AUTOLOOT: set_autoloot_mode(ch, argument);
			return;
		case SCMD_AUTOSPLIT: result = TogglePrfFlag(ch, EPrf::kAutosplit);
			break;
		case SCMD_AUTOMONEY: result = TogglePrfFlag(ch, EPrf::kAutomoney);
			break;
		case SCMD_NOARENA: result = TogglePrfFlag(ch, EPrf::kNoArena);
			break;
		case SCMD_NOEXCHANGE: result = TogglePrfFlag(ch, EPrf::kNoExchange);
			break;
		case SCMD_NOCLONES: result = TogglePrfFlag(ch, EPrf::kNoClones);
			break;
		case SCMD_NOINVISTELL: result = TogglePrfFlag(ch, EPrf::kNoInvistell);
			break;
		case SCMD_LENGTH: SetScreen(ch, argument, 0);
			return;
		case SCMD_WIDTH: SetScreen(ch, argument, 1);
			return;
		case SCMD_SCREEN: SetScreen(ch, argument, 2);
			return;
		case SCMD_NEWS_MODE: result = TogglePrfFlag(ch, EPrf::kNewsMode);
			break;
		case SCMD_BOARD_MODE: result = TogglePrfFlag(ch, EPrf::kBoardMode);
			break;
		case SCMD_CHEST_MODE: {
			std::string buffer = argument;
			SetChestMode(ch, buffer);
			break;
		}
		case SCMD_PKL_MODE: result = TogglePrfFlag(ch, EPrf::kPklMode);
			break;
		case SCMD_POLIT_MODE: result = TogglePrfFlag(ch, EPrf::kPolitMode);
			break;
		case SCMD_PKFORMAT_MODE: result = TogglePrfFlag(ch, EPrf::kPkFormatMode);
			break;
		case SCMD_WORKMATE_MODE: result = TogglePrfFlag(ch, EPrf::kClanmembersMode);
			break;
		case SCMD_OFFTOP_MODE: result = TogglePrfFlag(ch, EPrf::kOfftopMode);
			break;
		case SCMD_ANTIDC_MODE: result = TogglePrfFlag(ch, EPrf::kAntiDcMode);
			break;
		case SCMD_NOINGR_MODE: result = TogglePrfFlag(ch, EPrf::kNoIngrMode);
			break;
		case SCMD_REMEMBER: {
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
		case SCMD_NOTIFY_EXCH: {
			setNotifyEchange(ch, argument);
			return;
		}
		case SCMD_DRAW_MAP: {
			if (ch->IsFlagged(EPrf::kBlindMode)) {
				SendMsgToChar("В режиме слепого игрока карта недоступна.\r\n", ch);
				return;
			}
			result = TogglePrfFlag(ch, EPrf::kDrawMap);
			break;
		}
		case SCMD_ENTER_ZONE: result = TogglePrfFlag(ch, EPrf::kShowZoneNameOnEnter);
			break;
		case SCMD_MISPRINT: result = TogglePrfFlag(ch, EPrf::kShowUnread);
			break;
		case SCMD_BRIEF_SHIELDS: result = TogglePrfFlag(ch, EPrf::kBriefShields);
			break;
		case SCMD_AUTO_NOSUMMON: result = TogglePrfFlag(ch, EPrf::kAutonosummon);
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

void setNotifyEchange(CharData *ch, char *argument) {
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

void set_autoloot_mode(CharData *ch, char *argument) {
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
