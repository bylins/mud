//
// Created by Sventovit on 03.09.2024.
//

#include "engine/boot/boot_constants.h"
#include "engine/db/global_objects.h"
#include "engine/core/comm.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/network/descriptor_data.h"
#include "utils/logger.h"
#include "utils/utils_string.h"

#include <vector>
#include <sstream>
#include <string>

namespace offtop_system {

std::vector<std::string> block_list;

/// Проверка на наличие чара в стоп-списке и сет флага
void SetStopOfftopFlag(CharData *ch) {
	std::string mail(GET_EMAIL(ch));
	utils::ConvertToLow(mail);
	auto i = std::find(block_list.begin(), block_list.end(), mail);
	if (i != block_list.end()) {
		ch->SetFlag(EPrf::kStopOfftop);
	} else {
		ch->UnsetFlag(EPrf::kStopOfftop);
	}
}

/// Лоад/релоад списка нежелательных для оффтопа товарисчей.
void Init() {
	block_list.clear();
	// issue.misc-migrate: StateManager owns the file I/O; entries are tokenized/lowercased here
	// (preserving the previous `>> buffer` semantics). A missing file just yields an empty list.
	for (const auto &line : MUD::StateManager().LoadLines(state::EStateFile::kStopOfftop)) {
		std::istringstream iss(line);
		std::string buffer;
		while (iss >> buffer) {
			utils::ConvertToLow(buffer);
			block_list.push_back(buffer);
		}
	}

	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character) {
			SetStopOfftopFlag(d->character.get());
		}
	}
}

} // namespace offtop_system

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
