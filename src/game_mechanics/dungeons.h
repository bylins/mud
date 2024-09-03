//
// Created by Sventovit on 03.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_DUNGEONS_H_
#define BYLINS_SRC_GAME_MECHANICS_DUNGEONS_H_

#include "structs/structs.h"

class CharData;
namespace dungeons {

extern const int kNumberOfZoneDungeons;
extern const ZoneVnum kZoneStartDungeons;

void ListDungeons(CharData *ch);
void DoZoneCopy(CharData *, char *argument, int, int);
ZoneRnum ZoneCopy(ZoneVnum zvn_from);
void DoDungeonReset(CharData * /*ch*/, char *argument, int /*cmd*/, int /*subcmd*/);
void SwapObjectDungeon(CharData *ch);

void CreateBlankZoneDungeon();
void CreateBlankTrigsDungeon();
void CreateBlankRoomDungeon();
void CreateBlankMobsDungeon();
void CreateBlankObjsDungeon();

void DungeonReset(int zrn);

std::string CreateComplexDungeon(Trigger *trig, const std::vector<std::string>& tokens);

} // namespace dungeons

#endif //BYLINS_SRC_GAME_MECHANICS_DUNGEONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
