/**
\file do_debug_queues.h - a part of the Bylins engine.
\brief The `debug_queues` implementor command: dump the debug log queues to ERRLOG.
\details Moved out of interpreter.cpp (issue.interpreter-cleaning Bucket 5); it is not a dispatch
         concern -- just a thin command over the debug:: log-queue API.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_DEBUG_QUEUES_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_DEBUG_QUEUES_H_

class CharData;
void do_debug_queues(CharData *ch, char *argument, int cmd, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_DEBUG_QUEUES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
