/**
\file utils_encoding.h - a part of the Bylins engine.
\brief Codepage conversion (KOI8-R <-> Alt/CP866, Windows-1251, Latin, UTF-8).
\detail issue.utils-cleaning: the conversion tables, single-char converters and string converters,
        gathered out of utils.h. Specialized -- needed only where user I/O is (de)coded.
*/

#ifndef BYLINS_SRC_UTILS_UTILS_ENCODING_H_
#define BYLINS_SRC_UTILS_UTILS_ENCODING_H_

#include <string>

namespace codepages {

// High-half conversion tables (128 entries; index = byte - 128). Defined in utils_encoding.cpp.
extern char AltToKoi[];
extern char KoiToAlt[];
extern char WinToKoi[];
extern char KoiToWin[];
extern char KoiToWin2[];
extern char AltToLat[];

// Single-character converters; ASCII (< 128) passes through unchanged.
inline char KtoW(char c)  { return (unsigned char)(c) < 128 ? c : KoiToWin[(unsigned char)(c) - 128]; }
inline char KtoW2(char c) { return (unsigned char)(c) < 128 ? c : KoiToWin2[(unsigned char)(c) - 128]; }
inline char KtoA(char c)  { return (unsigned char)(c) < 128 ? c : KoiToAlt[(unsigned char)(c) - 128]; }
inline char WtoK(char c)  { return (unsigned char)(c) < 128 ? c : WinToKoi[(unsigned char)(c) - 128]; }
inline char AtoK(char c)  { return (unsigned char)(c) < 128 ? c : AltToKoi[(unsigned char)(c) - 128]; }
inline char AtoL(char c)  { return (unsigned char)(c) < 128 ? c : AltToLat[(unsigned char)(c) - 128]; }

// In-place and string converters.
void koi_to_alt(char *str, int len);
std::string koi_to_alt(const std::string &input);
void koi_to_win(char *str, int len);
void koi_to_utf8(char *str_i, char *str_o);
void utf8_to_koi(char *str_i, char *str_o);

}  // namespace codepages

#endif  // BYLINS_SRC_UTILS_UTILS_ENCODING_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
