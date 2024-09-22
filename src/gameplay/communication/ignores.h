#ifndef IGNORES_HPP_
#define IGNORES_HPP_

#include <memory>

/**
 *	Modes of ignoring
 */
enum EIgnore : Bitvector {
  kTell = 1 << 0,
  kSay = 1 << 1,
  kClan = 1 << 2,
  kAlliance = 1 << 3,
  kGossip = 1 << 4,
  kShout = 1 << 5,
  kHoller = 1 << 6,
  kGroup = 1 << 7,
  kWhisper = 1 << 8,
  kAsk = 1 << 9,
  kEmote = 1 << 10,
  kOfftop = 1 << 11,
};

struct ignore_data {
	using shared_ptr = std::shared_ptr<ignore_data>;

	ignore_data() : id(0), mode(0) {}

	long id;
	unsigned long mode;
};

bool ignores(CharData *who, CharData *whom, unsigned int flag);
char *text_ignore_modes(unsigned long mode, char *buf);
int ign_find_id(char *name, long *id);
const char *ign_find_name(long id);

#endif // __IGNORES_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
