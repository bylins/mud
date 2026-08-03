/**
\file native_text.h - a part of the Bylins engine.
\brief Character-semantic operations in the engine's *native runtime encoding* (issue #3681).

The migration flips the engine's internal string encoding from KOI8-R (1 byte == 1 character) to
UTF-8 (multibyte). Code that must reason about characters -- counting display width, capitalising
a letter, truncating without splitting a character -- should go through this thin dispatch layer
instead of assuming bytes. The active encoding is chosen at build time by the `internal_encoding`
Meson option, which defines INTERNAL_ENCODING_UTF8 for the UTF-8 build:

  * KOI8-R (default, current behaviour): every helper is byte-for-byte identical to the open-coded
    byte logic it replaces, so routing call sites through it is a no-op.
  * UTF-8: helpers operate on code points via the utf8:: primitives.

This lets the whole byte-vs-char refactor land and ship on KOI8-R (safely, as a no-op) ahead of
the encoding flip. Once the flip is permanent the KOI8-R branch and this indirection are removed.
*/

#ifndef BYLINS_SRC_UTILS_NATIVE_TEXT_H_
#define BYLINS_SRC_UTILS_NATIVE_TEXT_H_

#include <cstddef>
#include <string_view>

namespace native_text {

// True iff the engine was built with UTF-8 as its native runtime encoding. Reflects the flag the
// library itself was compiled with (not the translation unit that calls this), so callers/tests
// can branch reliably regardless of their own compile flags.
bool native_is_utf8();

// Number of display characters in a byte range / view. KOI8-R: the byte count. UTF-8: the number
// of code points (malformed bytes counted as one each, so it never stalls on legacy data).
std::size_t char_count(const char *begin, const char *end);
std::size_t char_count(std::string_view s);

// Uppercase the first character of the null-terminated string `s` in place (ASCII + Russian
// Cyrillic incl. Yo). No-op on an empty string, on a non-cased first character, or in the (never
// occurring for these alphabets) case where the uppercase form has a different byte length.
void capitalize_first(char *s);

// Largest byte offset <= max_bytes that lands on a character boundary, so cutting the string
// there never splits a multibyte character. KOI8-R: min(max_bytes, s.size()).
std::size_t truncate_offset(std::string_view s, std::size_t max_bytes);

// Byte length of the character that starts at `s` (KOI8-R: 1), for stepping over a whole
// character byte-by-byte. Always >= 1; on a malformed/truncated UTF-8 lead it returns only the
// bytes actually present (never counts past a terminator or a non-continuation byte).
std::size_t char_bytes(const char *s);

// Case-insensitive comparison in the native encoding: lexicographic over lowered characters,
// the shorter string orders first, returns the signed difference at the first mismatch (0 when
// equal). KOI8-R: per byte, via LOWER() -- matches str_cmp/str/str semantics. UTF-8: per code
// point, folded via utf8::to_lower (so the sign is meaningful; the magnitude is a code-point
// difference). In ncompare_ci, `n` is a byte budget -- the callers pass strlen()/length() -- and
// the comparison stops (as a match) once that many bytes of equal text have been consumed.
int compare_ci(std::string_view a, std::string_view b);
int ncompare_ci(std::string_view a, std::string_view b, std::size_t n);

// Is the character starting at `s` alphanumeric? KOI8-R: the a_isalnum byte table. UTF-8: ASCII
// letters/digits plus the Russian Cyrillic block -- so a multibyte letter is classified as one
// alphanumeric character rather than a lead byte followed by "punctuation" trail bytes.
bool is_alnum_char(const char *s);

// Do the characters starting at `a` and `b` match ignoring case? KOI8-R: LOWER(*a) == LOWER(*b).
// UTF-8: compares whole folded code points, so "P" matches "p" in Cyrillic too.
bool chars_equal_ci(const char *a, const char *b);

// Copy the character starting at `src` to `dst`, lowercased, and return how many bytes were
// consumed (always the same number written, so a caller's buffer accounting is unaffected).
// KOI8-R: one byte through a_lcc_table. UTF-8: folds the code point; in the rare case where the
// lowercase form would not be the same byte length, the character is copied unchanged rather
// than resized. `dst` may alias `src` (the length-preserving property makes that safe).
std::size_t copy_lower_char(const char *src, char *dst);

// Byte offset at which the final character of `s` begins (0 for an empty string), so that
// s.substr(0, last_char_offset(s)) drops exactly one character and s.substr(last_char_offset(s))
// is that character. KOI8-R: s.size() - 1.
std::size_t last_char_offset(std::string_view s);

// Does the single character `ch` occur in `list`? The replacement for strchr() over a literal
// list of letters: `list` is walked one whole character at a time, so a multibyte character can
// never match on a partial byte sequence. Comparison is exact (case-sensitive), like strchr.
bool list_contains_char(std::string_view list, std::string_view ch);

}  // namespace native_text

#endif  // BYLINS_SRC_UTILS_NATIVE_TEXT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
