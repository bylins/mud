/**
 \brief issue.runestones: cfg loaders for the runestone registry and its messages.
 \details Kept out of rune_stones.h (a widely-included header) so cfg_manager.h doesn't propagate.
		  Included by rune_stones.cpp (definitions) and cfg_manager.cpp (registration).
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_LOADERS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_LOADERS_H_

#include "engine/boot/cfg_manager.h"

// cfg id "rune_stone_messages" -> cfg/messages/ru/rune_stone_msg.xml. Loaded BEFORE rune_stones, so the
// per-stone names are available when the registry is built.
class RuneStoneMessagesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

// cfg id "rune_stones" -> cfg/mechanics/rune_stones.xml (the registry). Populates MUD::Runestones().
class RuneStonesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_LOADERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
