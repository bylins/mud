#ifndef __ACT_OTHER_HPP__
#define __ACT_OTHER_HPP__

class CharacterData;    // to avoid inclusion of "char.hpp"

int perform_group(CharacterData *ch, CharacterData *vict);
int max_group_size(CharacterData *ch);
#endif // __ACT_OTHER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
