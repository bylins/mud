#ifndef __CHAR__UTILITIES_HPP__
#define __CHAR__UTILITIES_HPP__

#include <char.hpp>

CHAR_DATA::shared_ptr create_character();
CHAR_DATA::shared_ptr add_poison(const CHAR_DATA::shared_ptr& character);
CHAR_DATA::shared_ptr add_sleep(const CHAR_DATA::shared_ptr& character);
CHAR_DATA::shared_ptr add_detect_invis(const CHAR_DATA::shared_ptr& character);
CHAR_DATA::shared_ptr add_detect_align(const CHAR_DATA::shared_ptr& character);
CHAR_DATA::shared_ptr create_character_with_one_removable_affect();
CHAR_DATA::shared_ptr create_character_with_two_removable_affects();
CHAR_DATA::shared_ptr create_character_with_two_removable_and_two_not_removable_affects();

#endif // __CHAR__UTILITIES_HPP__
