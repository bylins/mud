/**
\file native_text.h - a part of the Bylins engine.
\brief Character-semantic operations in the engine's *native runtime encoding* (issue #3681).

The engine holds text in UTF-8, where one character is one to four bytes. Code that reasons about
characters -- counting width, capitalising a letter, truncating without splitting a character,
comparing case-insensitively -- goes through here instead of touching bytes directly: a plain
LOWER(*s) or s[0] = UPPER(s[0]) is wrong on a multibyte letter and was the single largest source
of bugs in the migration.

The conversions at the bottom of this header are boundaries, not helpers for everyday code:
from_disk_* / to_disk for the world files (still KOI8-R on disk), to_koi8 for legacy client code
pages (their tables are indexed by KOI8-R bytes).
*/

#ifndef BYLINS_SRC_UTILS_NATIVE_TEXT_H_
#define BYLINS_SRC_UTILS_NATIVE_TEXT_H_

#include <cstddef>
#include <string>
#include <string_view>

namespace native_text {


// Number of display characters in a byte range / view. KOI8-R: the byte count. UTF-8: the number
// of code points (malformed bytes counted as one each, so it never stalls on legacy data).
std::size_t char_count(const char *begin, const char *end);
std::size_t char_count(std::string_view s);

// Uppercase the first character of the null-terminated string `s` in place (ASCII + Russian
// Cyrillic incl. Yo). No-op on an empty string, on a non-cased first character, or in the (never
// occurring for these alphabets) case where the uppercase form has a different byte length.
void capitalize_first(char *s);
void capitalize_first(std::string &s);

// Largest byte offset <= max_bytes that lands on a character boundary, so cutting the string
// there never splits a multibyte character. KOI8-R: min(max_bytes, s.size()).
std::size_t truncate_offset(std::string_view s, std::size_t max_bytes);

// Byte offset of the `chars`-th character (clamped to the end), so `s.substr(0, char_offset(s, n))`
// keeps exactly n characters. KOI8-R: min(chars, s.size()). Use this, not substr(0, n), wherever
// text is cut to fit a column: cutting by bytes both halves the visible width under UTF-8 and can
// split a character in two (issue #3681).
std::size_t char_offset(std::string_view s, std::size_t chars);

// Byte length of the character that starts at `s` (KOI8-R: 1), for stepping over a whole
// character byte-by-byte. Always >= 1; on a malformed/truncated UTF-8 lead it returns only the
// bytes actually present (never counts past a terminator or a non-continuation byte).
std::size_t char_bytes(const char *s);

// Numeric identity of the character starting at `s`, for dispatching a switch on a letter:
// the raw byte under KOI8-R, the code point under UTF-8. Compare against the constants in
// utils/russian_keys.h (Cyrillic) or ordinary character literals (ASCII, identical in both).
// Returns 0 on an empty string.
char32_t first_char_code(const char *s);

// The same, case-folded -- the replacements for switch (LOWER(*s)) / switch (UPPER(*s)).
char32_t first_char_code_lower(const char *s);
char32_t first_char_code_upper(const char *s);

// Case-insensitive comparison in the native encoding: lexicographic over lowered characters,
// the shorter string orders first, returns the signed difference at the first mismatch (0 when
// equal). KOI8-R: per byte, via LOWER() -- matches str_cmp/str/str semantics. UTF-8: per code
// point, folded via utf8::to_lower (so the sign is meaningful; the magnitude is a code-point
// difference).
int compare_ci(std::string_view a, std::string_view b);

// Is the character starting at `s` alphanumeric? KOI8-R: the a_isalnum byte table. UTF-8: ASCII
// letters/digits plus the Russian Cyrillic block -- so a multibyte letter is classified as one
// alphanumeric character rather than a lead byte followed by "punctuation" trail bytes.
bool is_alnum_char(const char *s);

// Is the character starting at `s` a letter? Same contract as is_alnum_char, minus the digits.
bool is_alpha_char(const char *s);

// Is the character starting at `s` an uppercase letter? KOI8-R: the a_isupper byte table.
// UTF-8: ASCII A-Z plus the uppercase Cyrillic range (and Yo) as whole code points -- the byte
// table cannot see these at all, since a UTF-8 Cyrillic lead byte is not in its uppercase range.
bool is_upper_char(const char *s);

// Do the characters starting at `a` and `b` match ignoring case? KOI8-R: LOWER(*a) == LOWER(*b).
// UTF-8: compares whole folded code points, so "P" matches "p" in Cyrillic too.
bool chars_equal_ci(const char *a, const char *b);

// Copy the character starting at `src` to `dst`, lowercased, and return how many bytes were
// consumed (always the same number written, so a caller's buffer accounting is unaffected).
// KOI8-R: one byte through a_lcc_table. UTF-8: folds the code point; in the rare case where the
// lowercase form would not be the same byte length, the character is copied unchanged rather
// than resized. `dst` may alias `src` (the length-preserving property makes that safe).
std::size_t copy_lower_char(const char *src, char *dst);

// Uppercase counterpart of copy_lower_char, with the same contract.
std::size_t copy_upper_char(const char *src, char *dst);

// Byte offset at which the final character of `s` begins (0 for an empty string), so that
// s.substr(0, last_char_offset(s)) drops exactly one character and s.substr(last_char_offset(s))
// is that character. KOI8-R: s.size() - 1.
std::size_t last_char_offset(std::string_view s);

// Range-for over the characters of `s`: each element is a string_view covering exactly one
// character, so scanning code never does pointer arithmetic and never lands mid-character.
//
//   for (auto ch : native_text::chars(name)) { ... }   // ch is one character, whatever its size
//
// The view must outlive the loop (it is not copied). Malformed bytes yield one element each.
class CharRange {
 public:
	explicit CharRange(std::string_view s) : m_str(s) {}

	class Iterator {
	 public:
		Iterator(std::string_view s, std::size_t pos) : m_str(s), m_pos(pos), m_len(step(s, pos)) {}
		std::string_view operator*() const { return m_str.substr(m_pos, m_len); }
		Iterator &operator++() {
			m_pos += m_len;
			m_len = step(m_str, m_pos);
			return *this;
		}
		bool operator!=(const Iterator &other) const { return m_pos != other.m_pos; }

	 private:
		static std::size_t step(std::string_view s, std::size_t pos);
		std::string_view m_str;
		std::size_t m_pos;
		std::size_t m_len;
	};

	[[nodiscard]] Iterator begin() const { return Iterator(m_str, 0); }
	[[nodiscard]] Iterator end() const { return Iterator(m_str, m_str.size()); }

 private:
	std::string_view m_str;
};

inline CharRange chars(std::string_view s) { return CharRange(s); }

// Whole-string case conversion in place. Prefer these over hand-rolled per-character loops.
// Length-preserving for ASCII and the Russian alphabet, so no reallocation happens.
void to_lower(std::string &s);
void to_upper(std::string &s);
void to_lower(char *s);
void to_upper(char *s);

// Bring text stored on disk in KOI8-R (world files, configs, saves) into the engine's native
// encoding. Identity under KOI8-R, a transcode under UTF-8. Having it here keeps the loaders
// free of #ifdefs and gives one place to revisit when the data files themselves move.
std::string from_koi8(const std::string &text);

// The inverse: take native text down to KOI8-R. Needed wherever something downstream is defined
// in terms of KOI8-R bytes -- the legacy client code pages are KOI8-R -> target byte tables, and
// the on-disk formats are KOI8-R. Identity under KOI8-R. A character KOI8-R does not have is
// first reduced to the closest one it does (see translit_koi8.h): a typographic dash to "-", a
// rounded frame corner to a square one, an accented letter to its base. Only what has no
// equivalent at all becomes the converter's placeholder.
std::string to_koi8(const std::string &text);

// Bring text that is UTF-8 on disk into the native encoding. The counterpart of from_koi8 for
// the files that are deliberately kept in UTF-8 rather than KOI8-R (the login screen). Identity
// under UTF-8; under KOI8-R it goes through the same reduction as to_koi8, so a file written
// with the full Unicode repertoire still renders sensibly on a KOI8-R build.
std::string from_utf8(const std::string &text);

// Collation key for sorting Russian text. The Russian letters are not in alphabetical order in
// KOI8-R, so sorting has always gone through Windows-1251 bytes, where they are. The key
// reproduces exactly that order and does not depend on the native encoding: compare two keys
// instead of transcoding the strings themselves (which corrupted them under UTF-8, since the
// KOI8-R -> Windows-1251 byte table means nothing for a multibyte string).
std::string sort_key(const std::string &text);

// Read a data file (all of which are stored in KOI8-R) and hand back its contents in the
// engine's native encoding. The counterpart of from_koi8 for whole files: parsers that take a
// buffer should go through this instead of reading the path themselves, so the boundary stays
// in one place. Returns an empty string if the file cannot be read -- callers report that the
// same way they did when the parser failed to open it.
std::string read_data_file(const std::string &path);

// Bring one line read from a data file into the native encoding, tolerating a file that has
// already been converted. Under KOI8-R this is the identity. Under UTF-8 the text is taken as
// already-native when it is well-formed UTF-8 and transcoded otherwise -- Cyrillic in KOI8-R is
// almost never valid UTF-8, which makes validity a reliable discriminator and lets old and new
// player files coexist without a version field.
std::string from_disk_line(const char *line);

// The same for a whole file's contents. Use this, not from_koi8, for anything the engine also
// WRITES back: from_koi8 transcodes unconditionally, so a file the engine already saved in the
// native encoding would be transcoded a second time -- and since the save then writes that back,
// every Cyrillic byte doubles on each load/save cycle and the file grows exponentially (that is
// exactly what happened to cfg/mechanics/obj_sets.xml, issue #3681).
std::string from_disk_text(const std::string &text);

// The write side of the same boundary, and the exact mirror of from_disk_text: whatever the
// engine puts on disk goes out in the encoding the disk format is in, which during the migration
// is still KOI8-R. Identity under KOI8-R; under UTF-8 the text is reduced and transcoded exactly
// as it is for a legacy client (see to_koi8).
//
// Read and write MUST stay symmetric. If the engine writes the native encoding while the rest of
// the world is KOI8-R, then the first save quietly converts every file it touches, rolling back
// to a KOI8-R build stops being possible, and the world is no longer the world we started with.
// (issue #3681).
std::string to_disk(const std::string &text);

// Записать текст в файл в кодировке мира. Однострочная обёртка над to_disk для тех, кто иначе
// звал бы pugi::save_file или свой ofstream и уносил бы на диск нативную кодировку. Возвращает
// false, если файл не открылся (issue #3681).
bool write_file(const std::string &path, const std::string &text);

// Pad `s` on the right with spaces to `width` CHARACTERS. The replacement for printf's "%-Ns"
// wherever the value can hold Russian: printf counts the field width in bytes, so under UTF-8 a
// Cyrillic word ate twice its share and the column drifted. Under KOI8-R this is byte-for-byte
// what "%-Ns" did. Longer input is returned untouched, exactly like printf (issue #3681).
std::string pad_right(std::string_view s, std::size_t width);

// Transliterate `name` into the ASCII form used for save-file names: Russian letters become
// Latin ones, ASCII is lowercased. The mapping is fixed by what the byte-wise implementation
// produced before the migration and MUST NOT drift -- the result is the on-disk file name of a
// player, so a change would orphan every existing character. Pinned by a test in both encodings.
std::string translit_to_filename(std::string_view name);

// Does the single character `ch` occur in `list`? The replacement for strchr() over a literal
// list of letters: `list` is walked one whole character at a time, so a multibyte character can
// never match on a partial byte sequence. Comparison is exact (case-sensitive), like strchr.
bool list_contains_char(std::string_view list, std::string_view ch);

}  // namespace native_text

#endif  // BYLINS_SRC_UTILS_NATIVE_TEXT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
