//
// Created by Sventovit on 04.09.2024.
//

#include "administration/ban.h"
#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "gameplay/economics/ext_money.h"
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

#include <third_party_libs/fmt/include/fmt/format.h>
#include "administration/proxy.h"

extern char *credits;
extern char *info;
extern char *motd;
extern char *rules;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *name_rules;
extern char *background;
extern char *help;
extern char *greetings;

void DoReload(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	argument = one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		if (AllocateBufferForFile(GREETINGS_FILE, &greetings) == 0) {
			PruneCrlf(greetings);
		}
		AllocateBufferForFile(IMMLIST_FILE, &immlist);
		AllocateBufferForFile(CREDITS_FILE, &credits);
		AllocateBufferForFile(MOTD_FILE, &motd);
		AllocateBufferForFile(RULES_FILE, &rules);
		AllocateBufferForFile(HELP_PAGE_FILE, &help);
		AllocateBufferForFile(INFO_FILE, &info);
		AllocateBufferForFile(POLICIES_FILE, &policies);
		AllocateBufferForFile(HANDBOOK_FILE, &handbook);
		AllocateBufferForFile(BACKGROUND_FILE, &background);
		AllocateBufferForFile(NAME_RULES_FILE, &name_rules);
		GoBootSocials();
		initIngredientsMagic();
		InitZoneTypes();
		MUD::Runestones().LoadRunestones();
		LoadSheduledReboot();
		oload_table.init();
		ObjData::InitSetTable();
		mob_races::LoadMobraces();
		load_morphs();
		GlobalDrop::init();
		offtop_system::Init();
		celebrates::Load();
		HelpSystem::reload_all();
		Remort::init();
		Noob::init();
		stats_reset::init();
		Bonus::bonus_log_load();
		DailyQuest::LoadFromFile();
	} else if (!str_cmp(arg, "portals"))
		MUD::Runestones().LoadRunestones();
	else if (!str_cmp(arg, "abilities")) {
		MUD::CfgManager().ReloadCfg("abilities");
	} else if (!str_cmp(arg, "skills")) {
		MUD::CfgManager().ReloadCfg("skills");
	} else if (!str_cmp(arg, "spells")) {
		MUD::CfgManager().ReloadCfg("spells");
	} else if (!str_cmp(arg, "feats")) {
		MUD::CfgManager().ReloadCfg("feats");
	} else if (!str_cmp(arg, "classes")) {
		MUD::CfgManager().ReloadCfg("classes");
	} else if (!str_cmp(arg, "mob_classes")) {
		MUD::CfgManager().ReloadCfg("mob_classes");
	} else if (!str_cmp(arg, "guilds")) {
		MUD::CfgManager().ReloadCfg("guilds");
	} else if (!str_cmp(arg, "currencies")) {
		MUD::CfgManager().ReloadCfg("currencies");
	} else if (!str_cmp(arg, "imagic"))
		initIngredientsMagic();
	else if (!str_cmp(arg, "ztypes"))
		InitZoneTypes();
	else if (!str_cmp(arg, "oloadtable"))
		oload_table.init();
	else if (!str_cmp(arg, "setstuff")) {
		ObjData::InitSetTable();
		HelpSystem::reload(HelpSystem::STATIC);
	} else if (!str_cmp(arg, "immlist"))
		AllocateBufferForFile(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "credits"))
		AllocateBufferForFile(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		AllocateBufferForFile(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "rules"))
		AllocateBufferForFile(RULES_FILE, &rules);
	else if (!str_cmp(arg, "help"))
		AllocateBufferForFile(HELP_PAGE_FILE, &help);
	else if (!str_cmp(arg, "info"))
		AllocateBufferForFile(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		AllocateBufferForFile(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		AllocateBufferForFile(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		AllocateBufferForFile(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "namerules"))
		AllocateBufferForFile(NAME_RULES_FILE, &name_rules);
	else if (!str_cmp(arg, "greetings")) {
		if (AllocateBufferForFile(GREETINGS_FILE, &greetings) == 0)
			PruneCrlf(greetings);
	} else if (!str_cmp(arg, "xhelp")) {
		HelpSystem::reload_all();
	} else if (!str_cmp(arg, "socials"))
		GoBootSocials();
	else if (!str_cmp(arg, "schedule"))
		LoadSheduledReboot();
	else if (!str_cmp(arg, "clan")) {
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
	} else if (!str_cmp(arg, "proxy"))
		RegisterSystem::LoadProxyList();
	else if (!str_cmp(arg, "boards"))
		Boards::Static::reload_all();
	else if (!str_cmp(arg, "titles"))
		TitleSystem::load_title_list();
	else if (!str_cmp(arg, "emails"))
		RegisterSystem::load();
	else if (!str_cmp(arg, "privilege"))
		privilege::Load();
	else if (!str_cmp(arg, "mobraces"))
		mob_races::LoadMobraces();
	else if (!str_cmp(arg, "morphs"))
		load_morphs();
	else if (!str_cmp(arg, "depot") && ch->IsFlagged(EPrf::kCoderinfo)) {
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
	} else if (!str_cmp(arg, "globaldrop")) {
		GlobalDrop::init();
	} else if (!str_cmp(arg, "offtop")) {
		offtop_system::Init();
	} else if (!str_cmp(arg, "shop")) {
		ShopExt::load(true);
	} else if (!str_cmp(arg, "named")) {
		NamedStuff::load();
	} else if (!str_cmp(arg, "celebrates")) {
		celebrates::Load();
	} else if (!str_cmp(arg, "setsdrop") && ch->IsFlagged(EPrf::kCoderinfo)) {
		skip_spaces(&argument);
		if (*argument && is_number(argument)) {
			SetsDrop::reload(atoi(argument));
		} else {
			SetsDrop::reload();
		}
	} else if (!str_cmp(arg, "remort")) {
		Remort::init();
	} else if (!str_cmp(arg, "noobhelp")) {
		Noob::init();
	} else if (!str_cmp(arg, "resetstats")) {
		stats_reset::init();
	} else if (!str_cmp(arg, "objsets")) {
		obj_sets::load();
	} else if (!str_cmp(arg, "daily")) {
		DailyQuest::LoadFromFile(ch);
	} else {
		SendMsgToChar("Неверный параметр для перезагрузки файлов.\r\n", ch);
		return;
	}

	std::string str = fmt::format("{} reload {}.", ch->get_name(), arg);
	mudlog(str.c_str(), NRM, kLvlImmortal, SYSLOG, true);

	SendMsgToChar(OK, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
