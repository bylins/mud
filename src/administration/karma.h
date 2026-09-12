/**
\file karma.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ADMINISTRATION_KARMA_H_
#define BYLINS_SRC_ADMINISTRATION_KARMA_H_

#include <string>

class CharData;

/**
 * Дописать строку в карму персонажа.
 * Причина, начинающаяся с точки, означает "не писать" -- так команды богов гасят запись.
 */
void AddKarma(CharData *ch, const std::string &punish, const std::string &reason);

#endif //BYLINS_SRC_ADMINISTRATION_KARMA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
