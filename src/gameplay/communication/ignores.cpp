#include "engine/entities/char_data.h"

bool ignores(CharData *who, CharData *whom, unsigned int flag) {
	if (who->IsNpc()) return false;

	long ign_id;

	if (IS_IMMORTAL(whom)) {
		return false;
	}

// чармисы игнорируемого хозяина тоже должны быть проигнорированы
	if (whom->IsNpc()
		&& AFF_FLAGGED(whom, EAffect::kCharmed)) {
		return ignores(who, whom->get_master(), flag);
	}

	ign_id = GET_UID(whom);
	for (const auto &ignore : who->get_ignores()) {
		if ((ignore->id == ign_id || ignore->id == -1)
			&& IS_SET(ignore->mode, flag)) {
			return true;
		}
	}
	return false;
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
