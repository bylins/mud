/**
\file do_generic_page.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_DO_GENERIC_PAGE_H_
#define BYLINS_SRC_ENGINE_UI_CMD_DO_GENERIC_PAGE_H_

class CharData;

extern const int kScmdInfo;
extern const int kScmdHandbook;
extern const int kScmdCredits;
extern const int kScmdPolicies;
extern const int kScmdVersion;
extern const int kScmdImmlist;
extern const int kScmdMotd;
extern const int kScmdRules;
extern const int kScmdClear;

void DoGenericPage(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_DO_GENERIC_PAGE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
