/**
\file inventory.h - a part of the Bylins engine.
\brief Inventory mechanic: carrying capacity, stacking, take/put, inventory display.
\details issue.handler-cleaning (Bucket 2): gathers what/how much a character can carry
 (weight + count), object stacking, taking objects into inventory, and the "inventory"
 command's output -- previously scattered across handler.cpp, base_stats and the command.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_INVENTORY_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_INVENTORY_H_

class CharData;
class ObjData;

// Carrying capacity (count and weight).
int CAN_CARRY_N(const CharData *ch);
int CAN_CARRY_W(const CharData *ch);

// Whether ch may pick up obj (capacity, wear flag, class/named restrictions) -- emits the reason.
bool CanTakeObj(CharData *ch, ObjData *obj);

// Put an object into a character's inventory (capacity checks + stacking).
void PlaceObjToInventory(ObjData *object, CharData *ch);
void RemoveObjFromChar(ObjData *object);
bool IsObjsStackable(ObjData *obj_one, ObjData *obj_two);
void ArrangeObjs(ObjData *obj, ObjData **list_start);   // stack/arrange in an obj list
void can_carry_obj(CharData *ch, ObjData *obj);

// Render the character's inventory (text + list). The "inventory" command only calls this.
void ShowInventory(CharData *ch);

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_INVENTORY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
