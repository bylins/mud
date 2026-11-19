/**
\file trigger_type_names.h - a part of the Bylins engine.
\brief Имена типов триггеров в YAML: выбор имени по attach_type.
\detail Биты типов у мобов, предметов и комнат переиспользованы под разный смысл: бит 4 --
это Act у моба, Fighting round у предмета и Enter PC у комнаты. Пока имя в словаре одно
на все три типа, оно обязано врать двум из них. Поэтому имена несут префикс своего типа
(kMobAct, kObjFight, kWldEnterPc), а без префикса остаются только биты, у которых смысл и
номер общие для всех типов (kRandom, kCommand, kAuto).
*/

#ifndef ENGINE_DB_TRIGGER_TYPE_NAMES_H_
#define ENGINE_DB_TRIGGER_TYPE_NAMES_H_

#include <string>
#include <string_view>
#include <unordered_map>

namespace world_format {

// "kMob" / "kObj" / "kWld"; пусто для неизвестного attach_type.
std::string_view TriggerTypePrefix(int attach_type);

// Имя бита для данного attach_type среди записей словаря trigger_types: сперва имя со своим
// префиксом, затем общее (без чужого префикса). Пусто -- имени нет, зовущий решает сам.
// Словарь без префиксов (старого образца) даёт прежнее поведение: находится общее имя.
std::string PickTriggerTypeName(const std::unordered_map<std::string, long> &entries,
								int attach_type,
								int bit);

// false, если имя несёт префикс чужого типа -- kObjFight в мобовом триггере и тому подобное.
bool TriggerTypeNameFitsAttach(std::string_view name, int attach_type);

}  // namespace world_format

#endif  // ENGINE_DB_TRIGGER_TYPE_NAMES_H_
