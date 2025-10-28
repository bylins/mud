/**
\file wizutil.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_WIZUTIL_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_WIZUTIL_H_

enum EWizutilScmd {
  kScmdReroll,
  kScmdNotitle,
  kScmdSquelch,
  kScmdFreeze,
  kScmdUnaffect,
  kScmdHell,
  kScmdName,
  kScmdRegister,
  kScmdMute,
  kScmdDumb,
  kScmdUnregister
};

class CharData;
void DoWizutil(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_WIZUTIL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
