/**
\file do_generic_page.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd/do_generic_page.h"
#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"

const int kScmdInfo{0};
const int kScmdHandbook{1};
const int kScmdCredits{2};
const int kScmdPolicies{3};
const int kScmdVersion{4};
const int kScmdImmlist{5};
const int kScmdMotd{6};
const int kScmdRules{7};
const int kScmdClear{8};

extern char *credits;
extern char *info;
extern char *motd;
extern char *rules;
extern char *immlist;
extern char *policies;
extern char *handbook;

extern void show_code_date(CharData *ch);

void DoGenericPage(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	switch (subcmd) {
		case kScmdCredits: page_string(ch->desc, credits, 0);
			break;
		case kScmdInfo: page_string(ch->desc, info, 0);
			break;
		case kScmdImmlist: page_string(ch->desc, immlist, 0);
			break;
		case kScmdHandbook: page_string(ch->desc, handbook, 0);
			break;
		case kScmdPolicies: page_string(ch->desc, policies, 0);
			break;
		case kScmdMotd: page_string(ch->desc, motd, 0);
			break;
		case kScmdRules: page_string(ch->desc, rules, 0);
			break;
		case kScmdClear: SendMsgToChar("\033[H\033[J", ch);
			break;
		case kScmdVersion: show_code_date(ch);
			break;
		default: log("SYSERR: Unhandled case in DoGenericPage. (%d)", subcmd);
			return;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
