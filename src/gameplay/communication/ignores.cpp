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

char *text_ignore_modes(unsigned long mode, char *buf) {
	buf[0] = 0;

	if (IS_SET(mode, EIgnore::kTell))
		strcat(buf, " сказать");
	if (IS_SET(mode, EIgnore::kSay))
		strcat(buf, " говорить");
	if (IS_SET(mode, EIgnore::kWhisper))
		strcat(buf, " шептать");
	if (IS_SET(mode, EIgnore::kAsk))
		strcat(buf, " спросить");
	if (IS_SET(mode, EIgnore::kEmote))
		strcat(buf, " эмоция");
	if (IS_SET(mode, EIgnore::kShout))
		strcat(buf, " кричать");
	if (IS_SET(mode, EIgnore::kGossip))
		strcat(buf, " болтать");
	if (IS_SET(mode, EIgnore::kHoller))
		strcat(buf, " орать");
	if (IS_SET(mode, EIgnore::kGroup))
		strcat(buf, " группа");
	if (IS_SET(mode, EIgnore::kClan))
		strcat(buf, " дружина");
	if (IS_SET(mode, EIgnore::kAlliance))
		strcat(buf, " союзники");
	if (IS_SET(mode, EIgnore::kOfftop))
		strcat(buf, " оффтопик");
	return buf;
}

int ign_find_id(char *name, long *id) {
	for (auto &i : player_table) {
		if (!str_cmp(name, i.name())) {
			if (i.level >= kLvlImmortal) {
				return 0;
			}

			*id = i.uid();
			return 1;
		}
	}
	return -1;
}

const char *ign_find_name(long id) {
	for (const auto &i : player_table) {
		if (id == i.uid()) {
			return i.name();
		}
	}

	return "кто-то";
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
