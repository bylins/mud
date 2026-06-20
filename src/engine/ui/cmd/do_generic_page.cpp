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
#include "engine/ui/system_messages.h"

const int kScmdInfo{0};
const int kScmdCredits{2};
const int kScmdPolicies{3};
const int kScmdVersion{4};
const int kScmdImmlist{5};
const int kScmdMotd{6};
const int kScmdRules{7};
const int kScmdClear{8};

// credits/info/motd/immlist/policies and immortal rules moved to the system-message
// container (system_messages::GetText); the dead "handbook" command was removed.

extern void ShowBuildInfo(CharData *ch);

void DoGenericPage(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	switch (subcmd) {
		case kScmdCredits: page_string(ch->desc, system_messages::GetText(system_messages::ESystemMsg::kCredits));
			break;
		case kScmdInfo: page_string(ch->desc, system_messages::GetText(system_messages::ESystemMsg::kInfo));
			break;
		case kScmdImmlist: page_string(ch->desc, system_messages::GetText(system_messages::ESystemMsg::kImmList));
			break;
		case kScmdPolicies: page_string(ch->desc, system_messages::GetText(system_messages::ESystemMsg::kPolicies));
			break;
		case kScmdMotd: page_string(ch->desc, system_messages::GetText(system_messages::ESystemMsg::kMotd));
			break;
		case kScmdRules: page_string(ch->desc, system_messages::GetText(system_messages::ESystemMsg::kImmRules));
			break;
		case kScmdClear: SendMsgToChar("\033[H\033[J", ch);
			break;
		case kScmdVersion: ShowBuildInfo(ch);
			break;
		default: log("SYSERR: Unhandled case in DoGenericPage. (%d)", subcmd);
			return;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
