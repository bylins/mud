//
// Created by Sventovit on 03.09.2024.
//

#ifndef BYLINS_SRC_COMMUNICATION_OFFTOP_H_
#define BYLINS_SRC_COMMUNICATION_OFFTOP_H_

class CharData;
namespace offtop_system {
const int kMinOfftopLvl{6};
void Init();
void SetStopOfftopFlag(CharData *ch);
} // namespace offtop_system

#endif //BYLINS_SRC_COMMUNICATION_OFFTOP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
