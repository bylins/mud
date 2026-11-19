/**
\file trigger_type_names.cpp - a part of the Bylins engine.
*/

#include "trigger_type_names.h"

#include <array>

namespace world_format {

namespace {

// Порядок совпадает с attach_types: 0 -- моб, 1 -- предмет, 2 -- комната.
constexpr std::array<std::string_view, 3> kPrefixes = {"kMob", "kObj", "kWld"};

bool HasPrefix(std::string_view name, std::string_view prefix) {
	return !prefix.empty() && name.size() > prefix.size() && name.substr(0, prefix.size()) == prefix;
}

bool HasAnyPrefix(std::string_view name) {
	for (const auto prefix : kPrefixes) {
		if (HasPrefix(name, prefix)) {
			return true;
		}
	}
	return false;
}

}  // namespace

std::string_view TriggerTypePrefix(int attach_type) {
	if (attach_type < 0 || attach_type >= static_cast<int>(kPrefixes.size())) {
		return {};
	}
	return kPrefixes[attach_type];
}

std::string PickTriggerTypeName(const std::unordered_map<std::string, long> &entries,
								int attach_type,
								int bit) {
	const auto prefix = TriggerTypePrefix(attach_type);
	std::string own;
	std::string shared;
	for (const auto &[name, value] : entries) {
		if (value != bit) {
			continue;
		}
		// Имён с одним номером в словаре может оказаться несколько, а обход хэш-таблицы
		// не упорядочен: берём лексикографически первое, иначе перезапись мира давала бы
		// разные файлы при одном и том же словаре.
		if (HasPrefix(name, prefix)) {
			if (own.empty() || name < own) {
				own = name;
			}
		} else if (!HasAnyPrefix(name) && (shared.empty() || name < shared)) {
			shared = name;
		}
	}
	return own.empty() ? shared : own;
}

bool TriggerTypeNameFitsAttach(std::string_view name, int attach_type) {
	const auto own = TriggerTypePrefix(attach_type);
	for (const auto prefix : kPrefixes) {
		if (prefix != own && HasPrefix(name, prefix)) {
			return false;
		}
	}
	return true;
}

}  // namespace world_format
