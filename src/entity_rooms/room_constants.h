/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для комнат.
 \details Тут должны располагаться исключительно константы для полей структуры комнаты - типы секторов, дверей и так далее.
*/

#ifndef BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_
#define BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

#include "structs/structs.h"

extern std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE;

// The cardinal directions: used as index to room_data.dir_option[]
const __uint8_t kDirNorth = 0;
const __uint8_t kDirEast = 1;
const __uint8_t kDirSouth = 2;
const __uint8_t kDirWest = 3;
const __uint8_t kDirUp = 4;
const __uint8_t kDirDown = 5;

// Room flags: used in room_data.room_flags //
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") //
constexpr bitvector_t ROOM_DARK = 1 << 0;
constexpr bitvector_t ROOM_DEATH =  1 << 1;    // Death trap      //
constexpr bitvector_t ROOM_NOMOB = 1 << 2;
constexpr bitvector_t ROOM_INDOORS = 1 << 3;
constexpr bitvector_t ROOM_PEACEFUL = 1 << 4;
constexpr bitvector_t ROOM_SOUNDPROOF = 1 << 5;
constexpr bitvector_t ROOM_NOTRACK = 1 << 6;
constexpr bitvector_t ROOM_NOMAGIC = 1 << 7;
constexpr bitvector_t ROOM_TUNNEL = 1 << 8;
constexpr bitvector_t ROOM_NOTELEPORTIN = 1 << 9;
constexpr bitvector_t ROOM_GODROOM = 1 << 10;    // LVL_GOD+ only allowed //
constexpr bitvector_t ROOM_HOUSE = 1 << 11;    // (R) Room is a house  //
constexpr bitvector_t ROOM_HOUSE_CRASH = 1 << 12;    // (R) House needs saving   //
constexpr bitvector_t ROOM_ATRIUM = 1 << 13;    // (R) The door to a house //
constexpr bitvector_t ROOM_OLC = 1 << 14;    // (R) Modifyable/!compress   //
constexpr bitvector_t ROOM_BFS_MARK = 1 << 15;    // (R) breath-first srch mrk   //
constexpr bitvector_t ROOM_MAGE = 1 << 16;
constexpr bitvector_t ROOM_CLERIC = 1 << 17;
constexpr bitvector_t ROOM_THIEF = 1 << 18;
constexpr bitvector_t ROOM_WARRIOR = 1 << 19;
constexpr bitvector_t ROOM_ASSASINE = 1 << 20;
constexpr bitvector_t ROOM_GUARD = 1 << 21;
constexpr bitvector_t ROOM_PALADINE = 1 << 22;
constexpr bitvector_t ROOM_RANGER = 1 << 23;
constexpr bitvector_t ROOM_POLY = 1 << 24;
constexpr bitvector_t ROOM_MONO = 1 << 25;
constexpr bitvector_t ROOM_SMITH = 1 << 26;
constexpr bitvector_t ROOM_MERCHANT = 1 << 27;
constexpr bitvector_t ROOM_DRUID = 1 << 28;
constexpr bitvector_t ROOM_ARENA = 1 << 29;

constexpr bitvector_t ROOM_NOSUMMON = kIntOne | (1 << 0);
constexpr bitvector_t ROOM_NOTELEPORTOUT = kIntOne | (1 << 1);
constexpr bitvector_t ROOM_NOHORSE = kIntOne | (1 << 2);
constexpr bitvector_t ROOM_NOWEATHER = kIntOne | (1 << 3);
constexpr bitvector_t ROOM_SLOWDEATH = kIntOne | (1 << 4);
constexpr bitvector_t ROOM_ICEDEATH = kIntOne | (1 << 5);
constexpr bitvector_t ROOM_NORELOCATEIN = kIntOne | (1 << 6);
constexpr bitvector_t ROOM_ARENARECV = kIntOne | (1 << 7);  // комната в которой слышно сообщения арены
constexpr bitvector_t ROOM_ARENASEND = kIntOne | (1 << 8);   // комната из которой отправляются сообщения арены
constexpr bitvector_t ROOM_NOBATTLE = kIntOne | (1 << 9);
constexpr bitvector_t ROOM_QUEST = kIntOne | (1 << 10);
constexpr bitvector_t ROOM_LIGHT = kIntOne | (1 << 11);
constexpr bitvector_t ROOM_NOMAPPER = kIntOne | (1 << 12);  //нет внумов комнат

constexpr bitvector_t ROOM_NOITEM = kIntTwo | (1 << 0);    // Передача вещей в комнате запрещена

// Exit info: used in room_data.dir_option.exit_info //
constexpr bitvector_t EX_ISDOOR = 1 << 0;    	// Exit is a door     //
constexpr bitvector_t EX_CLOSED = 1 << 1;   	// The door is closed //
constexpr bitvector_t EX_LOCKED = 1 << 2; 	   	// The door is locked //
constexpr bitvector_t EX_PICKPROOF = 1 << 3;    // Lock can't be picked  //
constexpr bitvector_t EX_HIDDEN = 1 << 4;
constexpr bitvector_t EX_BROKEN = 1 << 5; 		//Polud замок двери сломан
constexpr bitvector_t EX_DUNGEON_ENTRY = 1 << 6;    // When character goes through this door then he will get into a copy of the zone behind the door.

// Sector types: used in room_data.sector_type //
const __uint8_t kSectInside = 0;
const __uint8_t kSectCity = 1;
const __uint8_t kSectField = 2;
const __uint8_t kSectForest = 3;
const __uint8_t kSectHills = 4;
const __uint8_t kSectMountain = 5;
const __uint8_t kSectWaterSwim = 6;		// Swimmable water      //
const __uint8_t kSectWaterNoswim = 7;	// Water - need a boat  //
const __uint8_t kSectOnlyFlying = 8;	// Wheee!         //
const __uint8_t kSectUnderwater = 9;
const __uint8_t kSectSecret = 10;
const __uint8_t kSectStoneroad = 11;
const __uint8_t kSectRoad = 12;
const __uint8_t kSectWildroad = 13;
// надо не забывать менять NUM_ROOM_SECTORS в olc.h
// Values for weather changes //
const __uint8_t kSectFieldSnow = 20;
const __uint8_t kSectFieldRain = 21;
const __uint8_t kSectForestSnow = 22;
const __uint8_t kSectForestRain = 23;
const __uint8_t kSectHillsSnow = 24;
const __uint8_t kSectHillsRain = 25;
const __uint8_t kSectMountainSnow = 26;
const __uint8_t kSectThinIce = 27;
const __uint8_t kSectNormalIce = 28;
const __uint8_t kSectThickIce = 29;

#endif //BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :