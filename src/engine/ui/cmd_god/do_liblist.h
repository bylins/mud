/**
\file liblist.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_LIBLIST_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_LIBLIST_H_

inline constexpr int kScmdOlist{0};
inline constexpr int kScmdMlist{1};
inline constexpr int kScmdRlist{2};
inline constexpr int kScmdZlist{3};
inline constexpr int kScmdClist{4};

class CharData;
void do_liblist(CharData *ch, char *argument, int cmd, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_LIBLIST_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
