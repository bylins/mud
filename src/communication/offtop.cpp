//
// Created by Sventovit on 03.09.2024.
//

#include "boot/boot_constants.h"
#include "comm.h"
#include "entities/char_data.h"
#include "entities/entities_constants.h"
#include "structs/descriptor_data.h"
#include "utils/logger.h"
#include "utils/utils_string.h"

#include <vector>
#include <string>

namespace offtop_system {

const char *BLOCK_FILE{LIB_MISC"offtop.lst"};
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
	std::ifstream file(BLOCK_FILE);
	if (!file.is_open()) {
		log("SYSERROR: не удалось открыть файл на чтение: %s", BLOCK_FILE);
		return;
	}
	std::string buffer;
	while (file >> buffer) {
		utils::ConvertToLow(buffer);
		block_list.push_back(buffer);
	}

	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character) {
			SetStopOfftopFlag(d->character.get());
		}
	}
}

} // namespace offtop_system

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
