#ifndef __ACT_OTHER_HPP__
#define __ACT_OTHER_HPP__

class CharData;    // to avoid inclusion of "char.hpp"

int perform_group(CharData *ch, CharData *vict);
int max_group_size(CharData *ch);
#endif // __ACT_OTHER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
