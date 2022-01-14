/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для комнат.
 \details Код для парсинга названимй констант из конфигурационных файлов. В теории, потому что в настоящий момент
 этот код отсутствует и поскольку, вроде бы. константы комнат нигде пока не парсятся, я не вижу смысла его писать.
 Поэтому просто перенес сюда массив констант, который кто-то создал.
*/

#include "room_constants.h"

std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE = {
	{kSectInside, "inside"},
	{kSectCity, "city"},
	{kSectField, "field"},
	{kSectForest, "forest"},
	{kSectHills, "hills"},
	{kSectMountain, "mountain"},
	{kSectWaterSwim, "swim water"},
	{kSectWaterNoswim, "no swim water"},
	{kSectOnlyFlying, "flying"},
	{kSectUnderwater, "underwater"},
	{kSectSecret, "secret"},
	{kSectStoneroad, "stone road"},
	{kSectRoad, "road"},
	{kSectWildroad, "wild road"},
	{kSectFieldSnow, "snow field"},
	{kSectFieldRain, "rain field"},
	{kSectForestSnow, "snow forest"},
	{kSectForestRain, "rain forest"},
	{kSectHillsSnow, "snow hills"},
	{kSectHillsRain, "rain hills"},
	{kSectMountainSnow, "snow mountain"},
	{kSectThinIce, "thin ice"},
	{kSectNormalIce, "normal ice"},
	{kSectThickIce, "thick ice"}
};

