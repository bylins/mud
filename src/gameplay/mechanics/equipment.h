/**
\file equipment.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Сюда нужно перенести все связанное с экипировкой ("кукла", позиции надевания и т.п.)
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_EQUIPMENT_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_EQUIPMENT_H_

class CharData;
class ObjData;
void DamageEquipment(CharData *ch, int pos, int dam, int chance);
void DamageObj(ObjData *obj, int dam, int chance);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_EQUIPMENT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
