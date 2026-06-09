/**
 \brief issue.common-msg: loader for the shared common engine messages (cfg/messages/ru/common_msg.xml).
 \details The ECommonMsg enum and the CommonMsg() accessor live in config.h (widely included, so call
		  sites need no extra include). This header only declares the cfg loader, to keep cfg_manager.h
		  out of config.h.
*/

#ifndef BYLINS_SRC_ENGINE_CORE_COMMON_MESSAGES_H_
#define BYLINS_SRC_ENGINE_CORE_COMMON_MESSAGES_H_

#include "engine/boot/cfg_manager.h"

class CommonMessagesLoader : public cfg_manager::ICfgLoader {  // cfg id "common_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

#endif  // BYLINS_SRC_ENGINE_CORE_COMMON_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
