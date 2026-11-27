//
// Created by Sventovit on 04.09.2024.
//

#include "administration/ban.h"
#include "engine/db/player_index.h"
#include <map>
#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "gameplay/economics/shop_ext.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/mechanics/sets_drop.h"
#include "gameplay/mechanics/named_stuff.h"
#include "gameplay/communication/offtop.h"
#include "gameplay/communication/social.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/mechanics/depot.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/stuff.h"
#include "gameplay/mechanics/title.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/db/help.h"
#include "utils/utils.h"
#include "gameplay/mechanics/bonus.h"
#include "gameplay/mechanics/mob_races.h"
#include "gameplay/mechanics/rune_stones.h"   // issue.runestones phase 3: SpawnStones()
#include "gameplay/ai/spec_assign.h"
#include "engine/db/zone_reload.h"   // issue #4201: перечитывание зоны с диска

#include <fmt/format.h>
#include "administration/proxy.h"

extern char *help;

void DoReload(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char target[kMaxInputLength];
	argument = one_argument(argument, target);

	// issue.thing-names: list the available reload targets when called with no argument. Option names
	// avoid '_' on purpose -- the client renders '_' as a space, which is ambiguous (is "spell
	// messages" one option or two?), so e.g. spellmsg / skillmsg / hitmsg / mobclasses.
	if (!*target) {
		SendMsgToChar(
			"Usage: reload <what>. Available targets:\r\n"
			"  all  *  -- everything below\r\n"
			"  cfg data : abilities skills spells feats classes mobclasses guilds currencies\r\n"
			"             ztypes runes mobraces objsets affects animatedead\r\n"
			"  messages : spellmsg skillmsg hitmsg affectmsg roomaffectmsg\r\n"
			"  systems  : portals imagic oloadtable specials schedule clan proxy boards\r\n"
			"             globaldrop offtop shop named celebrates setsdrop remort daily resetstats\r\n"
			"             digging guards jewelry makeitems basic cases\r\n"
			"  text     : systemmsg help xhelp socials noobhelp titles emails privilege\r\n"
			"  depot <char-name>\r\n"
			"  zone <vnum> [применить]  -- сверить зону на диске с памятью; без второго\r\n"
			"             слова ничего не меняется, с ним зона перечитывается с диска\r\n", ch);
		return;
	}

	if (!str_cmp(target, "all") || *target == '*') {
		// Все зарегистрированные конфиги разом и в порядке регистрации: прежний список
		// перечислял их руками и отставал от реестра -- из 71 файла перезагружалось 20.
		MUD::CfgManager().ReloadAll();
		AllocateBufferForFile(HELP_PAGE_FILE, &help);
		initIngredientsMagic();
		ReloadSpecProcs();
		MUD::Runestones().SpawnStones();   // phase 3: (re)place physical stones for any new rooms
		LoadSheduledReboot();
		oload_table.init();
		GlobalDrop::init();
		offtop_system::Init();
		HelpSystem::reload_all();
		Bonus::bonus_log_load();
	} else if (!str_cmp(target, "portals")) {
		MUD::CfgManager().ReloadCfg("rune_stone_msg");
		MUD::CfgManager().ReloadCfg("rune_stones");
		MUD::Runestones().SpawnStones();   // phase 3: (re)place physical stones for any new rooms
	} else if (!str_cmp(target, "abilities")) {
		MUD::CfgManager().ReloadCfg("abilities");
	} else if (!str_cmp(target, "stableobjs")) {
		MUD::CfgManager().ReloadCfg("stable_objs");
	} else if (!str_cmp(target, "piety")) {
		MUD::CfgManager().ReloadCfg("piety");   // issue #2689
	} else if (!str_cmp(target, "skills")) {
		MUD::CfgManager().ReloadCfg("skills");
	} else if (!str_cmp(target, "spells")) {
		MUD::CfgManager().ReloadCfg("spells");
	} else if (!str_cmp(target, "spellmsg")) {
		MUD::CfgManager().ReloadCfg("spell_msg");
	} else if (!str_cmp(target, "objaffects")) {
		MUD::CfgManager().ReloadCfg("obj_affects");   // issue.obj-affects
	} else if (!str_cmp(target, "skillmsg")) {
		MUD::CfgManager().ReloadCfg("skill_msg");
    } else if (!str_cmp(target, "hitmsg")) {
        MUD::CfgManager().ReloadCfg("hit_msg");
	} else if (!str_cmp(target, "affectmsg")) {
		MUD::CfgManager().ReloadCfg("affect_msg");
	} else if (!str_cmp(target, "roomaffectmsg")) {
		MUD::CfgManager().ReloadCfg("room_affect_msg");
	} else if (!str_cmp(target, "feats")) {
		MUD::CfgManager().ReloadCfg("feats");
	} else if (!str_cmp(target, "affects")) {
		// Аффекты крутят при настройке баланса, а перечитать их можно было только перезапуском.
		MUD::CfgManager().ReloadCfg("affects");
	} else if (!str_cmp(target, "animatedead")) {
		MUD::CfgManager().ReloadCfg("animate_dead");   // issue.animate-dead
	} else if (!str_cmp(target, "classes")) {
		MUD::CfgManager().ReloadCfg("pc_classes");
	} else if (!str_cmp(target, "mobclasses")) {
		MUD::CfgManager().ReloadCfg("mob_classes");
	} else if (!str_cmp(target, "guilds")) {
		MUD::CfgManager().ReloadCfg("guilds");
	} else if (!str_cmp(target, "cities")) {
		MUD::CfgManager().ReloadCfg("cities_msg");
		MUD::CfgManager().ReloadCfg("cities");
	} else if (!str_cmp(target, "regions")) {
		MUD::CfgManager().ReloadCfg("region_msg");
		MUD::CfgManager().ReloadCfg("regions");
	} else if (!str_cmp(target, "pcraces")) {
		MUD::CfgManager().ReloadCfg("pc_race_msg");
		MUD::CfgManager().ReloadCfg("pc_races");
	} else if (!str_cmp(target, "currencies")) {
		MUD::CfgManager().ReloadCfg("currencies");
	} else if (!str_cmp(target, "imagic"))
		initIngredientsMagic();
	else if (!str_cmp(target, "ztypes"))
		MUD::CfgManager().ReloadCfg("zone_types");
	else if (!str_cmp(target, "runes"))
		MUD::CfgManager().ReloadCfg("rune_spells");
	else if (!str_cmp(target, "oloadtable"))
		oload_table.init();
	else if (!str_cmp(target, "systemmsg"))
		MUD::CfgManager().ReloadCfg("system_msg");
	else if (!str_cmp(target, "help"))
		AllocateBufferForFile(HELP_PAGE_FILE, &help);
	else if (!str_cmp(target, "xhelp")) {
		HelpSystem::reload_all();
	} else if (!str_cmp(target, "socials"))
		MUD::CfgManager().ReloadCfg("social_msg");
	else if (!str_cmp(target, "specials"))
		ReloadSpecProcs();
	else if (!str_cmp(target, "schedule"))
		LoadSheduledReboot();
	else if (!str_cmp(target, "clan")) {
		skip_spaces(&argument);
		if (!*argument) {
			Clan::ClanLoad();
			return;
		}
		Clan::ClanListType::iterator clan;
		std::string buffer(argument);
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
			if (CompareParam(buffer, (*clan)->get_abbrev())) {
				CreateFileName(buffer);
				Clan::ClanReload(buffer);
				SendMsgToChar("Перезагрузка клана.\r\n", ch);
				break;
			}
		}
	} else if (!str_cmp(target, "proxy"))
		RegisterSystem::LoadProxyList();
	else if (!str_cmp(target, "boards"))
		Boards::Static::reload_all();
	else if (!str_cmp(target, "titles"))
		TitleSystem::load_title_list();
	else if (!str_cmp(target, "emails"))
		RegisterSystem::load();
	else if (!str_cmp(target, "privilege")) {
		MUD::CfgManager().ReloadCfg("privilege");
	}
	else if (!str_cmp(target, "mobraces"))
		MUD::CfgManager().ReloadCfg("mob_races");
	else if (!str_cmp(target, "depot") && ch->IsFlagged(EPrf::kCoderinfo)) {
		skip_spaces(&argument);
		if (*argument) {
			long uid = GetUniqueByName(argument);
			if (uid > 0) {
				Depot::reload_char(uid, ch);
			} else {
				SendMsgToChar("Указанный чар не найден\r\n"
							  "Формат команды: reload depot <имя чара>.\r\n", ch);
			}
		} else {
			SendMsgToChar("Формат команды: reload depot <имя чара>.\r\n", ch);
		}
	} else if (!str_cmp(target, "globaldrop")) {
		GlobalDrop::init();
	} else if (!str_cmp(target, "grouping")) {
		MUD::CfgManager().ReloadCfg("group_exp_handicap");
	} else if (!str_cmp(target, "offtop")) {
		offtop_system::Init();
	} else if (!str_cmp(target, "shop")) {
		// Каталог наборов, а следом магазины: ShopItemSetsLoader::Reload сам зовет
		// ShopExt::load(true). Звать load(true) отсюда мало -- внумы и цены живут в
		// наборах, и правка в них иначе не доедет (issue #3700).
		MUD::CfgManager().ReloadCfg("shop_item_sets");
	} else if (!str_cmp(target, "named")) {
		NamedStuff::load();
	} else if (!str_cmp(target, "celebrates")) {
		MUD::CfgManager().ReloadCfg("celebrates");
	} else if (!str_cmp(target, "setsdrop") && ch->IsFlagged(EPrf::kCoderinfo)) {
		skip_spaces(&argument);
		if (*argument && is_number(argument)) {
			SetsDrop::reload(atoi(argument));
		} else {
			SetsDrop::reload();
		}
	} else if (!str_cmp(target, "noobhelp")) {
		MUD::CfgManager().ReloadCfg("noob");
	} else if (!str_cmp(target, "resetstats")) {
		MUD::CfgManager().ReloadCfg("reset_stats");
	} else if (!str_cmp(target, "remort")) {
		MUD::CfgManager().ReloadCfg("remort");
	} else if (!str_cmp(target, "digging")) {
		MUD::CfgManager().ReloadCfg("digging");
	} else if (!str_cmp(target, "guards")) {
		MUD::CfgManager().ReloadCfg("guards");
	} else if (!str_cmp(target, "jewelry")) {
		MUD::CfgManager().ReloadCfg("jewelry");
	} else if (!str_cmp(target, "makeitems")) {
		MUD::CfgManager().ReloadCfg("item_creation");
	} else if (!str_cmp(target, "basic")) {
		MUD::CfgManager().ReloadCfg("basic");
	} else if (!str_cmp(target, "objsets")) {
		MUD::CfgManager().ReloadCfg("obj_sets");
	} else if (!str_cmp(target, "cases")) {
		MUD::CfgManager().ReloadCfg("cases");
	} else if (!str_cmp(target, "daily")) {
		MUD::CfgManager().ReloadCfg("daily_quest");
		SendMsgToChar(DailyQuest::GetLastLoadMessage(), ch);
		SendMsgToChar("\r\n", ch);
	} else if (!str_cmp(target, "zone")) {
		// issue #4201, шаг 1: пока только разведка -- читаем зону с диска и показываем, чем
		// её состав отличается от того, что в памяти. Мир не меняется, так что команду
		// безопасно давать на боевом.
		char zone_arg[kMaxInputLength];
		argument = one_argument(argument, zone_arg);
		if (!*zone_arg || !a_isdigit(*zone_arg)) {
			SendMsgToChar("Укажите внум зоны: reload zone <vnum>.\r\n", ch);
			return;
		}
		char mode_arg[kMaxInputLength];
		one_argument(argument, mode_arg);
		// Без второго слова -- разведка: только показать разницу. Со словом "применить"
		// (или "apply") зона действительно перечитывается с диска.
		const bool apply = *mode_arg
			&& (!str_cmp(mode_arg, "применить") || !str_cmp(mode_arg, "apply"));
		const auto zvn = atoi(zone_arg);
		const auto report = apply ? zone_reload::Apply(zvn) : zone_reload::Inspect(zvn);
		SendMsgToChar(zone_reload::Format(report), ch);
		if (!report.ok) {
			return;
		}
	} else {
		SendMsgToChar("Неверный параметр для перезагрузки файлов.\r\n", ch);
		return;
	}

	mudlog(fmt::format("{} reload {}.", ch->get_name(), target), NRM, kLvlImmortal, SYSLOG, true);

	// issue.thing-names: some containers cache data from another file at build time, so reloading the
	// source string file alone is not enough -- remind the admin to reload the dependent file too.
	// Add entries here as more such cross-file dependencies appear.
	static const std::map<std::string, std::string> kReloadReminders{
		{"spellmsg", "Reminder: spell names are cached when spells.xml is loaded. "
		                   "Run 'reload spells' as well for the name changes to take effect."},
	};
	if (const auto it = kReloadReminders.find(target); it != kReloadReminders.end()) {
		SendMsgToChar(it->second + "\r\n", ch);
	}
	SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
