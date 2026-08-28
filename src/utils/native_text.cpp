/**
\file native_text.cpp - a part of the Bylins engine.
\brief Native-encoding character helpers declared in native_text.h (issue #3681).

The hot paths (case folding, comparison) walk bytes directly instead of decoding to code points
and back: the straightforward version cost 17x on a string-heavy benchmark.
*/

#include "native_text.h"

#include <algorithm>
#include "utf8.h"
#include "utils_encoding.h"
#include "translit_koi8.h"

#include "utf8.h"

#include "logger.h"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace native_text {


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

namespace {

// Local copy of the lead-byte length table. utf8::sequence_length lives in another translation
// unit, and this runs once per character in every scan -- a cross-module call there costs more
// than the work itself.
inline std::size_t lead_len(unsigned char c) {
	if (c < 0x80) {
		return 1;
	}
	if (c >= 0xC0 && c <= 0xDF) {
		return 2;
	}
	if (c >= 0xE0 && c <= 0xEF) {
		return 3;
	}
	if (c >= 0xF0 && c <= 0xF7) {
		return 4;
	}
	return 1;
}

}  // namespace

std::size_t char_bytes(const char *s) {
	const unsigned char lead = static_cast<unsigned char>(*s);
	if (lead < 0x80) {
		return 1;
	}
	const std::size_t want = lead_len(lead);
	std::size_t n = 1;
	while (n < want && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) {
		++n;
	}
	return n;
}

namespace {

// Shared driver for the two case-insensitive comparisons. `limit` caps how many bytes of `a` may
// be consumed (npos = unlimited): once that budget is spent the strings count as equal, which is
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

// Case folding for the repertoire the engine actually carries -- ASCII and the two-byte Cyrillic
// block -- done directly on the bytes. This runs per character in hot paths (whole-string case
// conversion, argument parsing), so it must not decode, allocate, or make a cross-module call:
//
//   A-Z / a-z : one byte, +-0x20
//   A-P (D0 90..D0 9F) <-> a-p (D0 B0..D0 BF) : lead stays D0, trail +-0x20
//   R-Ya(D0 A0..D0 AF) <-> r-ya(D1 80..D1 8F) : lead flips D0<->D1, trail -+0x20
//   Yo  (D0 81)        <-> yo  (D1 91)
//
// Anything else (other scripts, malformed bytes) falls through to the general path below, which
// is correct but slower -- and effectively never taken by this codebase.
inline bool fold_fast(const char *src, char *dst, std::size_t len, bool upper) {
	const unsigned char c0 = static_cast<unsigned char>(src[0]);
	if (c0 < 0x80) {
		char c = src[0];
		if (upper) {
			if (c >= 'a' && c <= 'z') {
				c = static_cast<char>(c - 0x20);
			}
		} else if (c >= 'A' && c <= 'Z') {
			c = static_cast<char>(c + 0x20);
		}
		dst[0] = c;
		return true;
	}
	if (len != 2) {
		return false;
	}
	const unsigned char c1 = static_cast<unsigned char>(src[1]);
	unsigned char o0 = c0;
	unsigned char o1 = c1;
	if (upper) {
		if (c0 == 0xD0 && c1 >= 0xB0 && c1 <= 0xBF) {          // a-p -> A-P
			o1 = static_cast<unsigned char>(c1 - 0x20);
		} else if (c0 == 0xD1 && c1 >= 0x80 && c1 <= 0x8F) {   // r-ya -> R-Ya
			o0 = 0xD0;
			o1 = static_cast<unsigned char>(c1 + 0x20);
		} else if (c0 == 0xD1 && c1 == 0x91) {                 // yo -> Yo
			o0 = 0xD0;
			o1 = 0x81;
		} else if (!((c0 == 0xD0 && c1 >= 0x90 && c1 <= 0xAF) || (c0 == 0xD0 && c1 == 0x81))) {
			return false;                                       // not Cyrillic: general path
		}
	} else {
		if (c0 == 0xD0 && c1 >= 0x90 && c1 <= 0x9F) {          // A-P -> a-p
			o1 = static_cast<unsigned char>(c1 + 0x20);
		} else if (c0 == 0xD0 && c1 >= 0xA0 && c1 <= 0xAF) {   // R-Ya -> r-ya
			o0 = 0xD1;
			o1 = static_cast<unsigned char>(c1 - 0x20);
		} else if (c0 == 0xD0 && c1 == 0x81) {                 // Yo -> yo
			o0 = 0xD1;
			o1 = 0x91;
		} else if (!((c0 == 0xD0 && c1 >= 0xB0) || (c0 == 0xD1 && c1 <= 0x8F) || (c0 == 0xD1 && c1 == 0x91))) {
			return false;
		}
	}
	dst[0] = static_cast<char>(o0);
	dst[1] = static_cast<char>(o1);
	return true;
}

std::size_t copy_folded_char(const char *src, char *dst, bool upper) {
	const std::size_t len = char_bytes(src);
	if (fold_fast(src, dst, len, upper)) {
		return len;
	}
	// General path: decode, fold, re-encode; only taken for characters outside ASCII+Cyrillic.
	char32_t cp = 0;
	if (utf8::decode(std::string_view(src, len), 0, cp) != 0) {
		const char32_t folded = upper ? utf8::to_upper(cp) : utf8::to_lower(cp);
		char tmp[4];
		if (folded != cp && utf8::encode(folded, tmp) == len) {
			for (std::size_t i = 0; i < len; ++i) {
				dst[i] = tmp[i];
			}
			return len;
		}
	}
	if (dst != src) {
		for (std::size_t i = 0; i < len; ++i) {
			dst[i] = src[i];
		}
	}
	return len;
}

}  // namespace

std::size_t copy_lower_char(const char *src, char *dst) {
	return copy_folded_char(src, dst, false);
}

std::size_t copy_upper_char(const char *src, char *dst) {
	return copy_folded_char(src, dst, true);
}


char32_t first_char_code(const char *s) {
	if (s == nullptr || *s == '\0') {
		return 0;
	}
	char32_t cp = 0;
	utf8::decode(std::string_view(s, char_bytes(s)), 0, cp);
	return cp;
}

char32_t first_char_code_lower(const char *s) {
	return utf8::to_lower(first_char_code(s));
}

char32_t first_char_code_upper(const char *s) {
	return utf8::to_upper(first_char_code(s));
}

namespace {

// Whole-buffer case conversion as one tight loop with no calls in the hot path: ASCII and the
// two-byte Cyrillic block are folded straight on the bytes. Anything else falls back to the
// general helper, which this codebase never hits in practice. Written this way deliberately --
// a per-character dispatch measured several times slower than the byte loop it replaces.
inline void fold_range_utf8(char *p, char *const end, bool upper) {
	while (p < end) {
		const unsigned char c0 = static_cast<unsigned char>(*p);
		if (c0 < 0x80) {
			char c = *p;
			if (upper) {
				if (c >= 'a' && c <= 'z') {
					c = static_cast<char>(c - 0x20);
				}
			} else if (c >= 'A' && c <= 'Z') {
				c = static_cast<char>(c + 0x20);
			}
			*p++ = c;
			continue;
		}
		if ((c0 == 0xD0 || c0 == 0xD1) && p + 1 < end) {
			const unsigned char c1 = static_cast<unsigned char>(p[1]);
			if (upper) {
				if (c0 == 0xD0 && c1 >= 0xB0) {
					p[1] = static_cast<char>(c1 - 0x20);
				} else if (c0 == 0xD1 && c1 <= 0x8F) {
					p[0] = static_cast<char>(0xD0);
					p[1] = static_cast<char>(c1 + 0x20);
				} else if (c0 == 0xD1 && c1 == 0x91) {
					p[0] = static_cast<char>(0xD0);
					p[1] = static_cast<char>(0x81);
				}
			} else {
				if (c0 == 0xD0 && c1 >= 0x90 && c1 <= 0x9F) {
					p[1] = static_cast<char>(c1 + 0x20);
				} else if (c0 == 0xD0 && c1 >= 0xA0 && c1 <= 0xAF) {
					p[0] = static_cast<char>(0xD1);
					p[1] = static_cast<char>(c1 - 0x20);
				} else if (c0 == 0xD0 && c1 == 0x81) {
					p[0] = static_cast<char>(0xD1);
					p[1] = static_cast<char>(0x91);
				}
			}
			p += 2;
			continue;
		}
		p += upper ? copy_upper_char(p, p) : copy_lower_char(p, p);
	}
}

}  // namespace

void to_lower(std::string &s) { fold_range_utf8(s.data(), s.data() + s.size(), false); }
void to_upper(std::string &s) { fold_range_utf8(s.data(), s.data() + s.size(), true); }
void to_lower(char *s) { fold_range_utf8(s, s + std::char_traits<char>::length(s), false); }
void to_upper(char *s) { fold_range_utf8(s, s + std::char_traits<char>::length(s), true); }


std::string from_koi8(const std::string &text) {
	// This runs per attribute/field while the world and the configs load, and the overwhelming
	// majority of those are pure ASCII (keys, aliases, numbers), which KOI8-R and UTF-8 spell
	// identically. Detect that first and hand the text back untouched instead of transcoding.
	bool has_high_byte = false;
	for (const char c : text) {
		if (static_cast<unsigned char>(c) >= 0x80) {
			has_high_byte = true;
			break;
		}
	}
	if (!has_high_byte) {
		return text;
	}
	// Every KOI8-R character lives below U+FFFF, so three bytes per input byte is a hard bound.
	std::vector<char> out(text.size() * 3 + 1, '\0');
	codepages::koi_to_utf8(const_cast<char *>(text.c_str()), out.data());
	return std::string(out.data());
}

std::string to_koi8(const std::string &text) {
	if (text.empty()) {
		return text;
	}
	bool has_high_byte = false;
	for (const char c : text) {
		if (static_cast<unsigned char>(c) >= 0x80) {
			has_high_byte = true;
			break;
		}
	}
	if (!has_high_byte) {
		return text;   // pure ASCII is spelled identically in both encodings
	}
	return codepages::Utf8ToKoi8(text);
}

std::string from_utf8(const std::string &text) {
	return text;   // the native encoding already is UTF-8
}

std::string translit_to_filename(std::string_view name) {
	// Code point -> the very same Latin character the KOI8-R byte table yields, so a player's
	// file name is identical before and after the flip. Upper and lower case collapse together
	// because the byte-wise original lowercased after transliterating.
	static const struct { char32_t cp; char latin; } kMap[] = {
		{0x0430, 'a'}, {0x0410, 'a'},
		{0x0431, 'b'}, {0x0411, 'b'},
		{0x0432, 'v'}, {0x0412, 'v'},
		{0x0433, 'g'}, {0x0413, 'g'},
		{0x0434, 'd'}, {0x0414, 'd'},
		{0x0435, 'e'}, {0x0415, 'e'},
		{0x0451, '9'}, {0x0401, '9'},
		{0x0436, '1'}, {0x0416, '1'},
		{0x0437, 'z'}, {0x0417, 'z'},
		{0x0438, 'i'}, {0x0418, 'i'},
		{0x0439, 'j'}, {0x0419, 'j'},
		{0x043A, 'k'}, {0x041A, 'k'},
		{0x043B, 'l'}, {0x041B, 'l'},
		{0x043C, 'm'}, {0x041C, 'm'},
		{0x043D, 'n'}, {0x041D, 'n'},
		{0x043E, 'o'}, {0x041E, 'o'},
		{0x043F, 'p'}, {0x041F, 'p'},
		{0x0440, 'r'}, {0x0420, 'r'},
		{0x0441, 's'}, {0x0421, 's'},
		{0x0442, 't'}, {0x0422, 't'},
		{0x0443, 'y'}, {0x0423, 'y'},
		{0x0444, 'f'}, {0x0424, 'f'},
		{0x0445, 'h'}, {0x0425, 'h'},
		{0x0446, 'c'}, {0x0426, 'c'},
		{0x0447, '7'}, {0x0427, '7'},
		{0x0448, '4'}, {0x0428, '4'},
		{0x0449, '6'}, {0x0429, '6'},
		{0x044A, '8'}, {0x042A, '8'},
		{0x044B, '3'}, {0x042B, '3'},
		{0x044C, '2'}, {0x042C, '2'},
		{0x044D, '5'}, {0x042D, '5'},
		{0x044E, '0'}, {0x042E, '0'},
		{0x044F, 'q'}, {0x042F, 'q'},
	};
	std::string out;
	out.reserve(name.size());
	std::size_t pos = 0;
	while (pos < name.size()) {
		char32_t cp = 0;
		const std::size_t len = utf8::decode(name, pos, cp);   // decode() already reports the length
		if (len == 0) {
			break;
		}
		if (cp < 0x80) {
			char c = static_cast<char>(cp);
			if (c >= 'A' && c <= 'Z') {
				c = static_cast<char>(c + 0x20);
			}
			out.push_back(c);
		} else {
			char mapped = '_';
			for (const auto &e : kMap) {
				if (e.cp == cp) {
					mapped = e.latin;
					break;
				}
			}
			out.push_back(mapped);
		}
		pos += len;
	}
	return out;
}


// ---------------------------------------------------------------------------------------------
// Encoding-independent helpers: expressed purely in terms of the primitives above, so they need
// no per-encoding branch. Unlike char_bytes() these take a bounded view, not a C string, so they
// are safe on a string_view that is not null-terminated.
// ---------------------------------------------------------------------------------------------

namespace {

inline std::size_t lead_len_shared(unsigned char c) {
	if (c < 0x80) {
		return 1;
	}
	if (c >= 0xC0 && c <= 0xDF) {
		return 2;
	}
	if (c >= 0xE0 && c <= 0xEF) {
		return 3;
	}
	if (c >= 0xF0 && c <= 0xF7) {
		return 4;
	}
	return 1;
}

// Byte length of the character at `pos`, clamped to the end of `s`.
std::size_t char_bytes_at(std::string_view s, std::size_t pos) {
	const unsigned char lead = static_cast<unsigned char>(s[pos]);
	if (lead < 0x80) {
		return 1;
	}
	const std::size_t want = lead_len_shared(lead);
	std::size_t n = 1;
	while (n < want && pos + n < s.size() && (static_cast<unsigned char>(s[pos + n]) & 0xC0) == 0x80) {
		++n;
	}
	return n;
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

std::string from_disk_line(const char *line) {
	if (line == nullptr || *line == '\0') {
		return {};
	}
	const std::string_view view(line);
	return utf8::is_valid(view) ? std::string(view) : from_koi8(std::string(view));
}

std::string from_disk_text(const std::string &text) {
	// Same discriminator as from_disk_line, applied to the whole file: well-formed UTF-8 is taken
	// as already native, anything else as KOI8-R. Cyrillic in KOI8-R is almost never valid UTF-8,
	// which makes validity a reliable test, and it is what keeps a load/save cycle idempotent.
	return utf8::is_valid(text) ? text : from_koi8(text);
}

std::size_t char_offset(std::string_view s, std::size_t chars) {
	return utf8::byte_offset(s, chars);
}

bool write_file(const std::string &path, const std::string &text) {
	const std::string on_disk = to_disk(text);
	std::ofstream out(path, std::ios::binary);
	if (!out) {
		return false;
	}
	out.write(on_disk.data(), static_cast<std::streamsize>(on_disk.size()));
	return out.good();
}

std::string pad_right(std::string_view s, std::size_t width) {
	const std::size_t len = char_count(s);
	std::string out(s);
	if (len < width) {
		out.append(width - len, ' ');
	}
	return out;
}

std::string to_disk(const std::string &text) {
	// Предохранитель. Всё нативное -- валидный UTF-8; если сюда пришло что-то другое, значит
	// строка не проходила границу чтения и держит дисковые байты (KOI8-R) как есть.
	// Транслитерировать их нельзя: to_koi8 разберёт такие байты как Latin-1 и прогонит через
	// словарь замен, а это необратимо -- 'верий.свет' превращается в 'AIEUAxAOA.OxAO'. Именно
	// так были съедены метки вещей, сундуки дружин и списки имён (issue #3681).
	//
	// Поэтому пишем байты как есть -- для диска они уже в нужной кодировке, файл остаётся цел, --
	// и жалуемся в лог: дыру видно сразу, без нагрузочного прогона и без потери данных.
	if (!utf8::is_valid(text)) {
		static std::atomic<unsigned long> seen{0};
		const unsigned long n = seen.fetch_add(1);
		if (n < 10 || n % 10000 == 0) {
			// Байты печатаются шестнадцатеричными нарочно: сунуть их в сообщение как есть
			// значило бы отдать логгеру невалидный UTF-8, а он пишет через этот же to_disk --
			// и жалоба принялась бы жаловаться сама на себя без конца.
			std::string head;
			const std::size_t show = std::min<std::size_t>(text.size(), 16);
			char byte[4];
			for (std::size_t i = 0; i < show; ++i) {
				std::snprintf(byte, sizeof(byte), "%02x", static_cast<unsigned char>(text[i]));
				head += byte;
				head += ' ';
			}
			log("SYSERR: to_disk got non-UTF-8 text (#%lu, %zu bytes) -- a read boundary is missing "
				"somewhere; writing the bytes through unchanged. First bytes: %s",
				n + 1, text.size(), head.c_str());
		}
		return text;
	}
	return to_koi8(text);
}

std::string read_data_file(const std::string &path) {
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		return {};
	}
	std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	return from_disk_text(raw);
}


std::string sort_key(const std::string &text) {
	std::string key = to_koi8(text);
	for (char &c : key) {
		c = codepages::KtoW(c);
	}
	return key;
}

}  // namespace native_text

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
