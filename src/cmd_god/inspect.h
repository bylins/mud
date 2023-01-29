//
// Created by Sventovit on 15.01.2023.
//

#include <memory>
#include <map>

#ifndef BYLINS_SRC_CMD_GOD_INSPECT_H_
#define BYLINS_SRC_CMD_GOD_INSPECT_H_

class CharData;
struct InspectRequest;

using InspReqPtr = std::shared_ptr<InspectRequest>;
/**
 * filepos - позиция в player_table перса который делает запрос
 * InspReqPtr - сам запрос
*/
using InspReqListType = std::map<int, InspReqPtr>;

//extern InspReqListType &inspect_list;

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void Inspecting();

#endif //BYLINS_SRC_CMD_GOD_INSPECT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
