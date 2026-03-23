#include "engine/ui/cmd_god/do_profile.h"

#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"
#include "gameplay/affects/mobile_affect_update_profiler.h"
#include "gameplay/affects/player_affect_update_profiler.h"

void do_profile(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	char arg[kMaxInputLength];
	char arg2[kMaxInputLength];
	argument = one_argument(argument, arg);
	one_argument(argument, arg2);

	auto show_usage = [ch]() {
		SendMsgToChar("Usage: profile npc | pc | all | clear npc|pc|all\r\n", ch);
	};

	auto show_page = [ch](const std::string &text) {
		if (!ch->desc) {
			return;
		}
		page_string(ch->desc, text);
	};

	if (!str_cmp(arg, "clear")) {
		if (!*arg2 || !str_cmp(arg2, "all")) {
			mobile_affect_update_profiler::clear();
			player_affect_update_profiler::clear();
			SendMsgToChar("Profile stats cleared: all.\r\n", ch);
			return;
		}

		if (!str_cmp(arg2, "npc")) {
			mobile_affect_update_profiler::clear();
			SendMsgToChar("Profile stats cleared: npc.\r\n", ch);
			return;
		}

		if (!str_cmp(arg2, "pc")) {
			player_affect_update_profiler::clear();
			SendMsgToChar("Profile stats cleared: pc.\r\n", ch);
			return;
		}

		show_usage();
		return;
	}

	if (!*arg || !str_cmp(arg, "npc")) {
		show_page(mobile_affect_update_profiler::render_report());
		return;
	}

	if (!str_cmp(arg, "pc")) {
		show_page(player_affect_update_profiler::render_report());
		return;
	}

	if (!str_cmp(arg, "all")) {
		std::string out;
		out.reserve(8192);
		out += mobile_affect_update_profiler::render_report();
		out += "\r\n";
		out += player_affect_update_profiler::render_report();
		show_page(out);
		return;
	}

	if (!str_cmp(arg, "help")) {
		show_usage();
		return;
	}

	show_usage();
}
