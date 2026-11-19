/**
 \file bone_commander.h - a part of the Bylins engine.
 \brief issue #4032: костяная подать -- нежить чернокнижника слабеет за живую костяную свиту.
*/

#ifndef GAMEPLAY_HANDLERS_BONE_COMMANDER_H_
#define GAMEPLAY_HANDLERS_BONE_COMMANDER_H_

class CharData;

namespace bone_commander {

// Пересчитывает штраф к урону нежити хозяина по сумме долей его живых скелетов. Подать берётся со
// свиты один раз: сумма долей делится между всей нежитью, а не вычитается у каждой по отдельности.
// Зовётся в трёх точках: подняли скелета, подняли нежить, кто-то из свиты отцепился или погиб.
// `leaving` -- тот, кто как раз выбывает: его не считаем ни в долях, ни в делителе, потому что в
// списке последователей он ещё числится. Безопасна для любого персонажа: нет скелетов -- штраф
// снимается, а не ставится нулевым.
void RefreshLevy(CharData *master, const CharData *leaving = nullptr);

}  // namespace bone_commander

#endif  // GAMEPLAY_HANDLERS_BONE_COMMANDER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
