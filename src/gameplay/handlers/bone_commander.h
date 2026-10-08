/**
 \file bone_commander.h - a part of the Bylins engine.
 \brief issue #4032: костяная подать -- нежить чернокнижника слабеет за живую костяную свиту.
*/

#ifndef GAMEPLAY_HANDLERS_BONE_COMMANDER_H_
#define GAMEPLAY_HANDLERS_BONE_COMMANDER_H_

class CharData;

namespace bone_commander {

// Пересчитывает штраф к урону нежити хозяина по сумме долей его живых скелетов. Зовётся в трёх
// точках: подняли скелета, подняли нежить, скелет отцепился или погиб. Безопасна для любого
// персонажа: если скелетов нет, штраф снимается, а не ставится нулевым.
void RefreshLevy(CharData *master);

}  // namespace bone_commander

#endif  // GAMEPLAY_HANDLERS_BONE_COMMANDER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
