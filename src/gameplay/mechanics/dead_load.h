//
// Created by Sventovit on 03.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_DEAD_LOAD_H_
#define BYLINS_SRC_GAME_MECHANICS_DEAD_LOAD_H_

#include "engine/structs/structs.h"

#include <list>

namespace dead_load {

enum EDeadLoadType {
	kOrdinary		= 0,	// обычная загрузка
	kProgression	= 1,	// вероятность падает до 0.01, но не ноль никогда, даже если достигнут MiW
	kSkin			= 2		// загрузка по освежеванию
};

enum EDeadLoadCmdType {
	kAnyway = 0,					// загружать всегда.
	kPreviuosSuccess = 1,			// если предыдущий предмет списка был загружен.
  	kAnywaySaveState = 2,			// загружать всегда, не менять результата предыдущей загрузки.
  	kPreviuosSuccessSaveState = 3	// загружать если был загружен предыдущий, не менять результата.
};

// ===============================================================
// Structure used for on_dead object loading //
// Эту механику следует вырезать.
struct LoadingItem {
  ObjVnum obj_vnum{0};
  int load_prob{0};
  int load_type{0};
  int spec_param{0};
};

using OnDeadLoadList = std::list<struct LoadingItem>;

bool ParseDeadLoadLine(OnDeadLoadList &dl_list, char *line);
bool LoadObjFromDeadLoad(ObjData *corpse, CharData *ch, CharData *chr, EDeadLoadType load_type);
int ResolveTagsInObjName(ObjData *obj, CharData *ch);
} // namespace dead_load

#endif //BYLINS_SRC_GAME_MECHANICS_DEAD_LOAD_H_


