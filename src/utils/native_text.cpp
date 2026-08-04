/**
\file native_text.cpp - a part of the Bylins engine.
\brief Native-encoding character helpers declared in native_text.h (issue #3681).

Two implementations selected by the INTERNAL_ENCODING_UTF8 build macro. The KOI8-R branch is kept
byte-for-byte identical to the open-coded logic these helpers replace so that routing call sites
through them changes nothing until the encoding flip.
*/

#include "native_text.h"

#ifdef INTERNAL_ENCODING_UTF8
#include "utf8.h"
#else
// The KOI8-R case tables (defined in utils.cpp). Declared directly instead of including utils.h,
// which drags in fmt/ and much of the engine for what is just two 256-byte lookups.
extern const char a_ucc_table[];
extern const char a_lcc_table[];
extern const bool a_isalnum_table[];
extern const bool a_isalpha_table[];
extern const bool a_isupper_table[];
#endif

#include <string>

namespace native_text {

#ifdef INTERNAL_ENCODING_UTF8

bool native_is_utf8() {
	return true;
}

std::size_t char_count(const char *begin, const char *end) {
	return utf8::length(std::string_view(begin, static_cast<std::size_t>(end - begin)));
}

std::size_t char_count(std::string_view s) {
	return utf8::length(s);
}

void capitalize_first(char *s) {
	if (s == nullptr || *s == '\0') {
		return;
	}
	const std::string_view sv(s);
	char32_t cp = 0;
	const std::size_t len = utf8::decode(sv, 0, cp);
	if (len == 0) {
		return;
	}
	const char32_t upper = utf8::to_upper(cp);
	if (upper == cp) {
		return;
	}
	std::string encoded;
	if (utf8::encode(upper, encoded) == len) {
		for (std::size_t i = 0; i < len; ++i) {
			s[i] = encoded[i];
		}
	}
}

std::size_t truncate_offset(std::string_view s, std::size_t max_bytes) {
	if (max_bytes >= s.size()) {
		return s.size();
	}
	std::size_t pos = 0;
	while (true) {
		char32_t cp = 0;
		const std::size_t len = utf8::decode(s, pos, cp);
		if (len == 0 || pos + len > max_bytes) {
			break;
		}
		pos += len;
	}
	return pos;
}

std::size_t char_bytes(const char *s) {
	const unsigned char lead = static_cast<unsigned char>(*s);
	if (lead < 0x80) {
		return 1;
	}
	const std::size_t want = static_cast<std::size_t>(utf8::sequence_length(lead));
	std::size_t n = 1;
	while (n < want && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) {
		++n;
	}
	return n;
}

namespace {

// Shared driver for the two case-insensitive comparisons. `limit` caps how many bytes of `a` may
// be consumed (npos = unlimited): once that budget is spent the strings count as equal, which is
// what the strn_cmp callers -- who pass a prefix length in bytes -- expect.
int compare_folded(std::string_view a, std::string_view b, std::size_t limit) {
	std::size_t pa = 0;
	std::size_t pb = 0;
	while (true) {
		if (limit != std::string_view::npos && pa >= limit) {
			return 0;
		}
		char32_t ca = 0;
		char32_t cb = 0;
		const std::size_t la = utf8::decode(a, pa, ca);
		const std::size_t lb = utf8::decode(b, pb, cb);
		if (la == 0 && lb == 0) {
			return 0;
		}
		if (la == 0) {
			return -1;
		}
		if (lb == 0) {
			return 1;
		}
		const char32_t fa = utf8::to_lower(ca);
		const char32_t fb = utf8::to_lower(cb);
		if (fa != fb) {
			return fa < fb ? -1 : 1;
		}
		pa += la;
		pb += lb;
	}
}

}  // namespace

int compare_ci(std::string_view a, std::string_view b) {
	return compare_folded(a, b, std::string_view::npos);
}

int ncompare_ci(std::string_view a, std::string_view b, std::size_t n) {
	return compare_folded(a, b, n);
}

bool is_alnum_char(const char *s) {
	const unsigned char lead = static_cast<unsigned char>(*s);
	if (lead < 0x80) {
		return (lead >= '0' && lead <= '9') || (lead >= 'A' && lead <= 'Z') || (lead >= 'a' && lead <= 'z');
	}
	char32_t cp = 0;
	if (utf8::decode(std::string_view(s, char_bytes(s)), 0, cp) == 0) {
		return false;
	}
	// Russian Cyrillic block, including Yo.
	return (cp >= 0x0410 && cp <= 0x044F) || cp == 0x0401 || cp == 0x0451;
}

bool is_alpha_char(const char *s) {
	const unsigned char lead = static_cast<unsigned char>(*s);
	if (lead < 0x80) {
		return (lead >= 'A' && lead <= 'Z') || (lead >= 'a' && lead <= 'z');
	}
	char32_t cp = 0;
	if (utf8::decode(std::string_view(s, char_bytes(s)), 0, cp) == 0) {
		return false;
	}
	return (cp >= 0x0410 && cp <= 0x044F) || cp == 0x0401 || cp == 0x0451;
}

bool is_upper_char(const char *s) {
	const unsigned char lead = static_cast<unsigned char>(*s);
	if (lead < 0x80) {
		return lead >= 'A' && lead <= 'Z';
	}
	char32_t cp = 0;
	if (utf8::decode(std::string_view(s, char_bytes(s)), 0, cp) == 0) {
		return false;
	}
	return (cp >= 0x0410 && cp <= 0x042F) || cp == 0x0401;
}

bool chars_equal_ci(const char *a, const char *b) {
	char32_t ca = 0;
	char32_t cb = 0;
	if (utf8::decode(std::string_view(a, char_bytes(a)), 0, ca) == 0
		|| utf8::decode(std::string_view(b, char_bytes(b)), 0, cb) == 0) {
		return false;
	}
	return utf8::to_lower(ca) == utf8::to_lower(cb);
}

namespace {

std::size_t copy_folded_char(const char *src, char *dst, char32_t (*fold)(char32_t)) {
	const std::size_t len = char_bytes(src);
	char32_t cp = 0;
	if (utf8::decode(std::string_view(src, len), 0, cp) != 0) {
		std::string folded;
		// Only rewrite when the folded form keeps the byte length -- true for ASCII and for the
		// whole Russian alphabet, so callers never see a character change size.
		if (utf8::encode(fold(cp), folded) == len) {
			for (std::size_t i = 0; i < len; ++i) {
				dst[i] = folded[i];
			}
			return len;
		}
	}
	for (std::size_t i = 0; i < len; ++i) {
		dst[i] = src[i];
	}
	return len;
}

}  // namespace

std::size_t copy_lower_char(const char *src, char *dst) {
	return copy_folded_char(src, dst, utf8::to_lower);
}

std::size_t copy_upper_char(const char *src, char *dst) {
	return copy_folded_char(src, dst, utf8::to_upper);
}

#else  // KOI8-R: 1 byte == 1 character

bool native_is_utf8() {
	return false;
}

std::size_t char_count(const char *begin, const char *end) {
	return static_cast<std::size_t>(end - begin);
}

std::size_t char_count(std::string_view s) {
	return s.size();
}

void capitalize_first(char *s) {
	if (s != nullptr && *s != '\0') {
		*s = a_ucc_table[static_cast<unsigned char>(*s)];
	}
}

std::size_t truncate_offset(std::string_view s, std::size_t max_bytes) {
	return max_bytes < s.size() ? max_bytes : s.size();
}

std::size_t char_bytes(const char *) {
	return 1;
}

namespace {

// Byte-wise fold-and-subtract, identical to the open-coded `LOWER(a[i]) - LOWER(b[i])` loops in
// utils_string.cpp: the magnitude of the result (not just its sign) is preserved, since some
// callers propagate it. A string that ended compares as LOWER('\0') against the other's byte.
int compare_bytes(std::string_view a, std::string_view b, std::size_t limit) {
	std::size_t i = 0;
	while (true) {
		if (limit != std::string_view::npos && i >= limit) {
			return 0;
		}
		const bool a_end = i >= a.size();
		const bool b_end = i >= b.size();
		if (a_end && b_end) {
			return 0;
		}
		const unsigned char ca = a_end ? '\0' : static_cast<unsigned char>(a[i]);
		const unsigned char cb = b_end ? '\0' : static_cast<unsigned char>(b[i]);
		const int chk = a_lcc_table[ca] - a_lcc_table[cb];
		if (chk != 0) {
			return chk;
		}
		++i;
	}
}

}  // namespace

int compare_ci(std::string_view a, std::string_view b) {
	return compare_bytes(a, b, std::string_view::npos);
}

int ncompare_ci(std::string_view a, std::string_view b, std::size_t n) {
	return compare_bytes(a, b, n);
}

bool is_alnum_char(const char *s) {
	return a_isalnum_table[static_cast<unsigned char>(*s)];
}

bool is_alpha_char(const char *s) {
	return a_isalpha_table[static_cast<unsigned char>(*s)];
}

bool is_upper_char(const char *s) {
	return a_isupper_table[static_cast<unsigned char>(*s)];
}

bool chars_equal_ci(const char *a, const char *b) {
	return a_lcc_table[static_cast<unsigned char>(*a)] == a_lcc_table[static_cast<unsigned char>(*b)];
}

std::size_t copy_lower_char(const char *src, char *dst) {
	*dst = a_lcc_table[static_cast<unsigned char>(*src)];
	return 1;
}

std::size_t copy_upper_char(const char *src, char *dst) {
	*dst = a_ucc_table[static_cast<unsigned char>(*src)];
	return 1;
}

#endif

// ---------------------------------------------------------------------------------------------
// Encoding-independent helpers: expressed purely in terms of the primitives above, so they need
// no per-encoding branch. Unlike char_bytes() these take a bounded view, not a C string, so they
// are safe on a string_view that is not null-terminated.
// ---------------------------------------------------------------------------------------------

namespace {

// Byte length of the character at `pos`, clamped to the end of `s`.
std::size_t char_bytes_at(std::string_view s, std::size_t pos) {
#ifdef INTERNAL_ENCODING_UTF8
	const unsigned char lead = static_cast<unsigned char>(s[pos]);
	if (lead < 0x80) {
		return 1;
	}
	const std::size_t want = static_cast<std::size_t>(utf8::sequence_length(lead));
	std::size_t n = 1;
	while (n < want && pos + n < s.size() && (static_cast<unsigned char>(s[pos + n]) & 0xC0) == 0x80) {
		++n;
	}
	return n;
#else
	(void) s;
	(void) pos;
	return 1;
#endif
}

}  // namespace

void capitalize_first(std::string &s) {
	if (s.empty()) {
		return;
	}
	// The uppercase form keeps the byte length for ASCII and the whole Russian alphabet, so
	// capitalising in place never resizes the string.
	capitalize_first(&s[0]);
}

std::size_t CharRange::Iterator::step(std::string_view s, std::size_t pos) {
	return pos < s.size() ? char_bytes_at(s, pos) : 0;
}

void to_lower(std::string &s) {
	for (std::size_t i = 0; i < s.size();) {
		i += copy_lower_char(&s[i], &s[i]);
	}
}

void to_upper(std::string &s) {
	for (std::size_t i = 0; i < s.size();) {
		i += copy_upper_char(&s[i], &s[i]);
	}
}

void to_lower(char *s) {
	while (*s) {
		s += copy_lower_char(s, s);
	}
}

void to_upper(char *s) {
	while (*s) {
		s += copy_upper_char(s, s);
	}
}

std::size_t last_char_offset(std::string_view s) {
	std::size_t last = 0;
	std::size_t pos = 0;
	while (pos < s.size()) {
		last = pos;
		pos += char_bytes_at(s, pos);
	}
	return last;
}

bool list_contains_char(std::string_view list, std::string_view ch) {
	if (ch.empty()) {
		return false;
	}
	std::size_t pos = 0;
	while (pos < list.size()) {
		const std::size_t len = char_bytes_at(list, pos);
		if (len == ch.size() && list.compare(pos, len, ch) == 0) {
			return true;
		}
		pos += len;
	}
	return false;
}

}  // namespace native_text

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
