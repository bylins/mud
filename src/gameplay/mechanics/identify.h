/**
 \file identify.h - a part of the Bylins engine.
 \brief issue.spellhandlers: item/character identify-display mechanic (shared by the identify
        spells, the identify skill, and the trading systems: auction/exchange/shops/clan house).
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_IDENTIFY_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_IDENTIFY_H_

class CharData;
class ObjData;

// Render an object's full stat block to `ch` (detail level scaled by `fullness`).
void MortShowObjValues(const ObjData *obj, CharData *ch, int fullness);
// Render a character's stat block to `ch` (detail level scaled by `fullness`).
void MortShowCharValues(CharData *victim, CharData *ch, int fullness);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_IDENTIFY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
